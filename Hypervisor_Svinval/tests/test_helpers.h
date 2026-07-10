/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_helpers.h - Common helpers for Hypervisor × Svinval cross tests
 *
 * Provides H/Svinval extension detection, HINVAL instruction wrappers,
 * VS/VU-mode trampolines, and PTE inspection utilities.
 *
 * Design: All test files are #included into test_register.c, so static
 * functions and variables are visible across all tests within the same
 * compilation unit.
 */

#ifndef HYPERVISOR_SVINVAL_TEST_HELPERS_H
#define HYPERVISOR_SVINVAL_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_fence.h"
#include "hyp/hyp_ldst.h"
#include "hyp/gstage_pt.h"
#include "hyp/two_stage.h"
#include "hyp/two_stage_helpers.h"
#include "hyp/test_vs_helpers.h"
#include "hyp/hyp_trap.h"
#include "hyp/hyp_vs_trap.h"
#include "hyp/hyp_platform.h"
#include "hyp_svinval_insn.h"

/* ===================================================================
 * Linker-provided test-region symbols (see Hypervisor_Svinval/kernel.ld).
 * =================================================================== */
extern uint8_t test_data_area[];
extern uint8_t test_fault_page[];
extern uint8_t test_exec_page[];
extern uint8_t test_exec_target[];
extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];

#define TEST_REGION_BASE   ((uintptr_t)__vm_test_region_start)

/* ===================================================================
 * Exception cause codes (from RISC-V Privileged Spec)
 *
 * Note: When hedeleg=0, VS-mode exceptions are routed to M-mode,
 * which uses S-mode cause codes (13/15) rather than VS-mode codes (5/7).
 * This test suite sets hedeleg=0 in hyp_reset.c, so we expect:
 *   - VS-mode load page fault = 13 (S-mode encoding)
 *   - VS-mode store/AMO page fault = 15 (S-mode encoding)
 * =================================================================== */
#define CAUSE_VIRTUAL_INSTRUCTION      22
#define CAUSE_VS_STORE_PAGE_FAULT      15  /* S-mode encoding when hedeleg=0 */
#define CAUSE_VS_LOAD_PAGE_FAULT       13  /* S-mode encoding when hedeleg=0 */
#define CAUSE_INST_GUEST_PAGE_FAULT    20
#define CAUSE_LOAD_GUEST_PAGE_FAULT    21
#define CAUSE_STORE_GUEST_PAGE_FAULT   23

/* ===================================================================
 * HINVAL diagnostic helper
 *
 * Prints detailed trap information (cause, epc, tval, instruction
 * decoding) to confirm whether an illegal-instruction exception was
 * caused by HINVAL.VVMA.
 *
 * Uses half-word reads to avoid misaligned-load faults on platforms
 * where epc may not be 4-byte aligned (e.g. Spike with compressed
 * instructions preceding the faulting instruction).
 *
 * Matches instructions by funct7 field rather than exact encoding,
 * because the compiler may allocate non-zero registers for rs1/rs2
 * even when the source operands are constant 0.
 * =================================================================== */
static void hinval_print_diag(void) {
    uintptr_t cause = trap_get_cause();
    uintptr_t epc   = trap_get_epc();
    uintptr_t tval  = trap_get_tval();

    printf("  [HINVAL DIAG] Trap details:\n");
    printf("    cause  = 0x%lx", (unsigned long)cause);
    if (cause == 2)       printf(" (Illegal Instruction)");
    else if (cause == 22) printf(" (Virtual Instruction)");
    else if (cause == 1)  printf(" (Instruction Access Fault)");
    else if (cause == 4)  printf(" (Load Address Misaligned)");
    printf("\n");
    printf("    epc    = 0x%lx\n", (unsigned long)epc);
    printf("    tval   = 0x%lx\n", (unsigned long)tval);

    /* Read instruction at epc using two half-word reads to avoid
     * misaligned-load faults when epc is not 4-byte aligned. */
    uint16_t inst_lo = *(volatile uint16_t *)epc;
    if ((inst_lo & 0x3) == 0x3) {
        /* 32-bit instruction: read as two half-words */
        uint16_t inst_hi = *(volatile uint16_t *)(epc + 2);
        uint32_t inst = ((uint32_t)inst_hi << 16) | (uint32_t)inst_lo;
        uint32_t opcode = inst & 0x7F;
        uint32_t funct7 = (inst >> 25) & 0x7F;
        uint32_t rs2    = (inst >> 20) & 0x1F;
        uint32_t rs1    = (inst >> 15) & 0x1F;
        uint32_t rd     = (inst >> 7)  & 0x1F;

        printf("    inst   = 0x%08lx (at epc)\n", (unsigned long)inst);
        printf("    decode: opcode=0x%02lx funct7=0x%02lx rs1=x%lu rs2=x%lu rd=x%lu\n",
               (unsigned long)opcode, (unsigned long)funct7,
               (unsigned long)rs1, (unsigned long)rs2, (unsigned long)rd);

        /* Also decode tval if it looks like an instruction encoding
         * (QEMU/Spike write the faulting instruction into tval for
         * illegal-instruction exceptions). */
        if (cause == 2 && tval != 0) {
            uint32_t tval_funct7 = ((uint32_t)tval >> 25) & 0x7F;
            uint32_t tval_opcode = (uint32_t)tval & 0x7F;
            printf("    tval decode: opcode=0x%02lx funct7=0x%02lx\n",
                   (unsigned long)tval_opcode, (unsigned long)tval_funct7);
        }

        /* Match by funct7 + opcode (SYSTEM=0x73) */
        bool is_hinval_vvma = false;
        if (opcode == 0x73) {
            if (funct7 == 0x13) {
                printf("    >>> MATCH: HINVAL.VVMA rs1=x%lu, rs2=x%lu\n",
                       (unsigned long)rs1, (unsigned long)rs2);
                is_hinval_vvma = true;
            } else if (funct7 == 0x33) {
                printf("    >>> MATCH: HINVAL.GVMA rs1=x%lu, rs2=x%lu\n",
                       (unsigned long)rs1, (unsigned long)rs2);
            } else if (funct7 == 0x0B) {
                printf("    >>> MATCH: SINVAL.VMA rs1=x%lu, rs2=x%lu\n",
                       (unsigned long)rs1, (unsigned long)rs2);
            } else if (funct7 == 0x0C && rs2 == 0) {
                printf("    >>> MATCH: SFENCE.W.INVAL\n");
            } else if (funct7 == 0x0C && rs2 == 1) {
                printf("    >>> MATCH: SFENCE.INVAL.IR\n");
            } else {
                printf("    >>> UNKNOWN SYSTEM instruction\n");
            }
        } else {
            printf("    >>> NOT a SYSTEM instruction (opcode != 0x73)\n");
        }

        if (cause == 2) {
            printf("  [HINVAL DIAG] CONCLUSION: Illegal Instruction (cause=2) ");
            if (is_hinval_vvma)
                printf("CONFIRMED from HINVAL.VVMA\n");
            else
                printf("from a DIFFERENT instruction (0x%08lx)\n",
                       (unsigned long)inst);
        }
    } else {
        printf("    inst   = 0x%04x (compressed, not Svinval)\n",
               (unsigned)inst_lo);
    }
}

/* ===================================================================
 * H extension detection
 * =================================================================== */
static bool check_h_extension(void) {
    uint64_t misa = CSRR(misa);
    if (!(misa & (1UL << ('H' - 'A')))) {
        return false;
    }
    return true;
}

#define H_REQUIRED_OR_SKIP() do { \
    if (!check_h_extension()) { \
        TEST_SKIP("H extension not available"); \
    } \
} while (0)

/* ===================================================================
 * Svinval extension detection
 *
 * Two-level detection:
 * 1. Base Svinval: probe SINVAL.VMA(0, 0) - the core Svinval instruction
 * 2. Hypervisor Svinval: probe HINVAL.VVMA(0, 0) - Hypervisor extension
 *
 * QEMU -cpu max supports base Svinval (SINVAL.VMA) but may not
 * implement the Hypervisor-specific HINVAL.VVMA/GVMA instructions.
 * =================================================================== */
static bool svinval_detected = false;
static bool svinval_detection_done = false;
static bool hinval_detected = false;
static bool hinval_detection_done = false;

/* Check base Svinval support (SINVAL.VMA) */
static bool check_svinval_extension(void) {
    if (svinval_detection_done)
        return svinval_detected;

    /* Try to execute HINVAL.VVMA(0, 0) - the Hypervisor Svinval instruction */
    trap_expect_begin();
    HINVAL_VVMA(0, 0);
    trap_expect_end();

    bool triggered = trap_was_triggered();

    if (triggered) {
        printf("  [Svinval Detection] HINVAL.VVMA(0,0) triggered trap:\n");
        hinval_print_diag();
    }

    svinval_detected = !triggered;
    svinval_detection_done = true;
    return svinval_detected;
}

/* Check Hypervisor Svinval support (HINVAL.VVMA) */
static bool check_hinval_extension(void) {
    if (hinval_detection_done)
        return hinval_detected;

    trap_expect_begin();
    HINVAL_VVMA(0, 0);
    trap_expect_end();

    if (trap_was_triggered()) {
        printf("  [HINVAL Detection] HINVAL.VVMA(0,0) triggered trap:\n");
        hinval_print_diag();
    }

    hinval_detected = !trap_was_triggered();
    hinval_detection_done = true;
    return hinval_detected;
}

#define SVINVAL_REQUIRED_OR_SKIP() do { \
    if (!check_svinval_extension()) { \
        TEST_SKIP("Platform does not implement Svinval"); \
    } \
} while (0)

#define HINVAL_REQUIRED_OR_SKIP() do { \
    if (!check_hinval_extension()) { \
        TEST_SKIP("Platform does not implement HINVAL.VVMA/GVMA"); \
    } \
} while (0)

/* ===================================================================
 * PTE inspection / modification helpers
 * =================================================================== */

/* Read VS-stage PTE at the given VA and level */
static uintptr_t vs_pte_read(two_stage_ctx_t *ctx, uintptr_t va, int level) {
    uintptr_t *pte = pt_get_pte(&ctx->vs_ctx, va, level);
    return pte ? *pte : 0;
}

/* Read G-stage PTE at the given GPA and level */
static uintptr_t g_pte_read(two_stage_ctx_t *ctx, uintptr_t gpa, int level) {
    uintptr_t *pte = gpt_get_pte(&ctx->g_ctx, gpa, level);
    return pte ? *pte : 0;
}

/* Modify VS-stage PTE at the given VA and level */
static void vs_pte_modify(two_stage_ctx_t *ctx, uintptr_t va, int level, uintptr_t new_flags) {
    uintptr_t *pte = pt_get_pte(&ctx->vs_ctx, va, level);
    if (pte) {
        /* Preserve PPN, update flags */
        *pte = (*pte & ~(PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)) | new_flags;
    }
}

/* Modify G-stage PTE at the given GPA and level */
static void g_pte_modify(two_stage_ctx_t *ctx, uintptr_t gpa, int level, uintptr_t new_flags) {
    uintptr_t *pte = gpt_get_pte(&ctx->g_ctx, gpa, level);
    if (pte) {
        /* Preserve PPN, update flags */
        *pte = (*pte & ~(PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)) | new_flags;
    }
}

/* ===================================================================
 * VS-mode test trampolines for HINVAL instructions
 * =================================================================== */

/* VS-mode: execute HINVAL.VVMA(arg, 0) */
static uintptr_t vs_exec_hinval_vvma(uintptr_t arg) {
    trap_expect_begin();
    HINVAL_VVMA(arg, 0);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VS-mode: execute HINVAL.GVMA(arg, 0) */
static uintptr_t vs_exec_hinval_gvma(uintptr_t arg) {
    trap_expect_begin();
    HINVAL_GVMA(arg, 0);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VS-mode: execute SFENCE.W.INVAL */
static uintptr_t vs_exec_sfence_w_inval(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    SFENCE_W_INVAL();
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VS-mode: execute SFENCE.INVAL.IR */
static uintptr_t vs_exec_sfence_inval_ir(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    SFENCE_INVAL_IR();
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VU-mode test trampolines for HINVAL instructions
 * =================================================================== */

/* VU-mode: execute HINVAL.VVMA(arg, 0) */
static uintptr_t vu_exec_hinval_vvma(uintptr_t arg) {
    trap_expect_begin();
    HINVAL_VVMA(arg, 0);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VU-mode: execute HINVAL.GVMA(arg, 0) */
static uintptr_t vu_exec_hinval_gvma(uintptr_t arg) {
    trap_expect_begin();
    HINVAL_GVMA(arg, 0);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VU-mode: execute SFENCE.W.INVAL */
static uintptr_t vu_exec_sfence_w_inval(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    SFENCE_W_INVAL();
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VU-mode: execute SFENCE.INVAL.IR */
static uintptr_t vu_exec_sfence_inval_ir(uintptr_t arg) {
    (void)arg;
    trap_expect_begin();
    SFENCE_INVAL_IR();
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VU-mode: execute SINVAL.VMA(arg, 0) */
static uintptr_t vu_exec_sinval_vma(uintptr_t arg) {
    trap_expect_begin();
    SINVAL_VMA(arg, 0);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * VS-mode memory access trampolines
 * =================================================================== */

/* VS-mode load: returns 0 on success, cause on trap */
static uintptr_t vs_load(uintptr_t arg) {
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)arg;
    (void)val;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* VS-mode store: returns 0 on success, cause on trap */
static uintptr_t vs_store(uintptr_t arg) {
    trap_expect_begin();
    *(volatile uintptr_t *)arg = 0xDEADBEEF;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

#endif /* HYPERVISOR_SVINVAL_TEST_HELPERS_H */
