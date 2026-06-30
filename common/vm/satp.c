/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm.h"
#include "uart.h"

/* ===================================================================
 * External dependencies from privilege.c and trap.c
 * =================================================================== */

/* Privilege level definitions (must match privilege.c) */
#define PRIV_U  0
#define PRIV_S  1
#define PRIV_M  3

/* From privilege.c */
extern uintptr_t run_in_priv(unsigned priv, uintptr_t (*fn)(uintptr_t),
                              uintptr_t arg);

/* From trap_asm.S */
extern void s_trap_entry(void);

/* Linker-provided S-mode stack */
extern uintptr_t __smode_stack_end;

/* From csr_accessors.c */
extern uintptr_t csr_read(int csr_num);
extern void csr_write(int csr_num, uintptr_t val);

/* ===================================================================
 * PMP helpers for VM
 *
 * When running S-mode code, PMP must allow access. If no PMP entries
 * are configured, S/U-mode accesses will fail with access faults.
 * We use a high-numbered PMP entry (entry 15) with NAPOT covering
 * the entire address space to grant RWX permissions.
 * =================================================================== */

/* PMP configuration bits (duplicated here to avoid pmp_cfg.h dependency) */
#define VM_PMP_R        (1U << 0)
#define VM_PMP_W        (1U << 1)
#define VM_PMP_X        (1U << 2)
#define VM_PMP_A_NAPOT  (3U << 3)
#define VM_PMP_RWX      (VM_PMP_R | VM_PMP_W | VM_PMP_X)

/* PMP entry index used by VM framework */
#define VM_PMP_ENTRY    15

/**
 * vm_pmp_allow_all - Set up PMP entry to allow S/U-mode full access
 *
 * Configures PMP entry VM_PMP_ENTRY as a NAPOT region covering the
 * entire address space with RWX permissions.
 *
 * pmpaddr = all ones => covers entire address space (2^(XLEN+2) bytes)
 * pmpcfg  = A=NAPOT | R | W | X
 */
static void vm_pmp_allow_all(void) {
    /* pmpaddr15 = all 1s => NAPOT covering entire address space */
    csr_write(CSR_PMPADDR0 + VM_PMP_ENTRY, (uintptr_t)-1);

    /* Read pmpcfg, set entry 15's byte to NAPOT|RWX */
#if __riscv_xlen == 64
    /* RV64: pmpcfg2 holds entries 8-15, entry 15 is byte 7 */
    int cfg_csr = CSR_PMPCFG0 + 2;  /* pmpcfg2 */
    int byte_offset = (VM_PMP_ENTRY % 8) * 8;  /* entry 15 => byte 7 => shift 56 */
#else
    /* RV32: pmpcfg3 holds entries 12-15, entry 15 is byte 3 */
    int cfg_csr = CSR_PMPCFG0 + 3;  /* pmpcfg3 */
    int byte_offset = (VM_PMP_ENTRY % 4) * 8;  /* entry 15 => byte 3 => shift 24 */
#endif

    uintptr_t cfg_val = csr_read(cfg_csr);
    uintptr_t mask = ~((uintptr_t)0xFF << byte_offset);
    cfg_val = (cfg_val & mask) | ((uintptr_t)(VM_PMP_A_NAPOT | VM_PMP_RWX) << byte_offset);
    csr_write(cfg_csr, cfg_val);
}

/**
 * vm_pmp_clear - Clear the PMP entry used by VM framework
 */
static void vm_pmp_clear(void) {
    csr_write(CSR_PMPADDR0 + VM_PMP_ENTRY, 0);

#if __riscv_xlen == 64
    int cfg_csr = CSR_PMPCFG0 + 2;
    int byte_offset = (VM_PMP_ENTRY % 8) * 8;
#else
    int cfg_csr = CSR_PMPCFG0 + 3;
    int byte_offset = (VM_PMP_ENTRY % 4) * 8;
#endif

    uintptr_t cfg_val = csr_read(cfg_csr);
    uintptr_t mask = ~((uintptr_t)0xFF << byte_offset);
    cfg_val = cfg_val & mask;
    csr_write(cfg_csr, cfg_val);
}

/* ===================================================================
 * sfence.vma helpers
 * =================================================================== */

/**
 * vm_sfence_vma - Execute sfence.vma with optional address/ASID
 */
void vm_sfence_vma(uintptr_t vaddr, uintptr_t asid) {
    if (vaddr == 0 && asid == 0) {
        /* Global flush */
        asm volatile("sfence.vma" ::: "memory");
    } else if (asid == 0) {
        /* Flush by address */
        asm volatile("sfence.vma %0, zero" :: "r"(vaddr) : "memory");
    } else if (vaddr == 0) {
        /* Flush by ASID */
        asm volatile("sfence.vma zero, %0" :: "r"(asid) : "memory");
    } else {
        /* Flush by address and ASID */
        asm volatile("sfence.vma %0, %1" :: "r"(vaddr), "r"(asid) : "memory");
    }
}

/* ===================================================================
 * satp Control
 * =================================================================== */

/**
 * vm_enable - Enable virtual memory by writing satp
 */
void vm_enable(pt_context_t *ctx, unsigned asid) {
    if (!ctx || !ctx->root_pt) {
        printf("ERROR: vm_enable: invalid context\n");
        return;
    }

    uintptr_t root_ppn = (uintptr_t)ctx->root_pt >> PAGE_SHIFT;
    uintptr_t satp_val = MAKE_SATP(ctx->mode, asid, root_ppn);

    CSRW(satp, satp_val);
    vm_sfence_vma(0, 0);
}

/**
 * vm_disable - Disable virtual memory (set satp to Bare mode)
 */
void vm_disable(void) {
    CSRW(satp, 0);
    vm_sfence_vma(0, 0);
}

/* ===================================================================
 * Mode Switching
 * =================================================================== */

/**
 * vm_switch_mode - Switch to a different Sv mode
 *
 * Disables VM, reinitializes page tables for the new mode,
 * and rebuilds the identity mapping with the same parameters.
 */
void vm_switch_mode(pt_context_t *ctx, int new_mode) {
    /* Save current mapping parameters */
    uintptr_t saved_base  = ctx->map_base;
    uintptr_t saved_size  = ctx->map_size;
    int       saved_level = ctx->map_level;

    /* Disable VM first */
    vm_disable();

    /* Reset page table pool and reinitialize */
    pt_pool_reset();
    pt_init(ctx, new_mode);

    /* Rebuild identity mapping if one was previously set up */
    if (saved_size > 0) {
        uintptr_t flags = PTE_V | PTE_RWXAD;
        pt_setup_identity_mapping(ctx, saved_base, saved_size,
                                  flags, saved_level);
    }
}

/* ===================================================================
 * S-mode Execution with VM
 * =================================================================== */

/**
 * vm_run_in_smode - Execute a function in S-mode with VM enabled
 *
 * Flow:
 * 1. Configure trap delegation (page faults -> S-mode)
 * 2. Set stvec to s_trap_entry
 * 3. Enable virtual memory (write satp)
 * 4. Switch to S-mode and execute fn(arg) via run_in_priv()
 * 5. On return, disable VM and restore delegation
 *
 * Note: This function must be called from M-mode.
 * The page table context must have identity mapping set up
 * so that code and data addresses are valid in both M and S modes.
 */
uintptr_t vm_run_in_smode(pt_context_t *ctx,
                           uintptr_t (*fn)(uintptr_t),
                           uintptr_t arg) {
    /*
     * Step 1: Set up PMP to allow S-mode access
     *
     * RISC-V requires PMP rules for S/U-mode access. Without any
     * matching PMP entry, all S/U-mode accesses fail with access faults.
     * We set up a catch-all NAPOT entry covering the entire address space.
     */
    vm_pmp_allow_all();

    /*
     * Step 2: Configure trap delegation
     *
     * Delegate page faults and ecalls from U-mode to S-mode.
     * This allows the S-mode trap handler to handle page faults
     * while VM is enabled.
     *
     * IMPORTANT: do NOT delegate CAUSE_ECALL_FROM_S (cause 9). The
     * framework's _run_trampoline exits S-mode via `ecall` to ask
     * M-mode to switch current_priv back to M; that exit ecall MUST
     * be intercepted by the M-mode handler. Delegating it to S-mode
     * leaves the framework's ecall to be re-delivered to S-mode,
     * which causes infinite recursion (s_trap_handler -> goto_priv(M)
     * -> do_ecall -> ECALL_FROM_S -> s_trap_handler -> ...).
     *
     * Tests that need an S-mode-installed stvec to intercept S-mode
     * ecall (e.g. Sstvecd STVEC-DIR-01 / MULTI-01) cannot be expressed
     * with the current vm_run_in_smode shape and should be marked
     * SKIP at the call site.
     */
    uintptr_t medeleg = CSRR(medeleg);
    uintptr_t new_medeleg = medeleg |
        BIT(CAUSE_INST_PAGE_FAULT)  |
        BIT(CAUSE_LOAD_PAGE_FAULT)  |
        BIT(CAUSE_STORE_PAGE_FAULT) |
        BIT(CAUSE_ECALL_FROM_U);
    CSRW(medeleg, new_medeleg);

    /*
     * Step 3: Set up S-mode trap vector
     */
    CSRW(stvec, (uintptr_t)s_trap_entry);

    /*
     * Step 4: Enable virtual memory
     */
    vm_enable(ctx, 0);

    /*
     * Step 5: Execute function in S-mode
     *
     * run_in_priv handles the M->S transition via mret and
     * the S->M return via ecall. Since we have identity mapping,
     * all addresses (code, stack, data) are valid in both modes.
     */
    uintptr_t result = run_in_priv(PRIV_S, fn, arg);

    /*
     * Step 6: Clean up
     */
    vm_disable();
    CSRW(medeleg, medeleg);  /* Restore original delegation */
    vm_pmp_clear();          /* Remove VM PMP entry */

    return result;
}

/* ===================================================================
 * U-mode Execution with VM
 * =================================================================== */

/**
 * vm_run_in_umode - Execute a function in U-mode with VM enabled
 *
 * Similar to vm_run_in_smode but targets U-mode.
 *
 * Flow:
 * 1. Set up PMP to allow U-mode access
 * 2. Enable virtual memory (write satp)
 * 3. Switch to U-mode via run_in_priv(PRIV_U)
 * 4. On return, disable VM and clean up
 *
 * Note: This function must be called from M-mode.
 * The page table context must have identity mapping set up with
 * PTE_U flag on all pages that fn will access, so that U-mode
 * can read/write/execute them.
 *
 * The ecall from U-mode (cause 8) is handled by the M-mode trap
 * handler directly -- no S-mode delegation of ECALL_FROM_U is
 * needed for the framework's privilege-switch ecall. Page faults
 * are NOT delegated to S-mode here; they are caught by M-mode's
 * trap_record mechanism for test assertions.
 */
uintptr_t vm_run_in_umode(pt_context_t *ctx,
                           uintptr_t (*fn)(uintptr_t),
                           uintptr_t arg) {
    /*
     * Step 1: Set up PMP to allow U-mode access
     */
    vm_pmp_allow_all();

    /*
     * Step 2: Enable virtual memory
     */
    vm_enable(ctx, 0);

    /*
     * Step 3: Execute function in U-mode
     *
     * run_in_priv(PRIV_U) uses mret with MPP=0 to jump directly
     * from M-mode to U-mode. The trampoline's ecall (cause 8,
     * ECALL_FROM_U) returns to M-mode via the M-mode trap handler.
     *
     * Since we use identity mapping, all addresses are valid in
     * both M and U modes, provided PTE_U is set on the pages.
     */
    uintptr_t result = run_in_priv(PRIV_U, fn, arg);

    /*
     * Step 4: Clean up
     */
    vm_disable();
    vm_pmp_clear();

    return result;
}
