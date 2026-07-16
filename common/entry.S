#include "encoding.h"

/*
 * Platform model macros (RVMODEL_BOOT, RVMODEL_HALT_PASS,
 * RVMODEL_HALT_FAIL, RVMODEL_DATA_SECTION) are injected via
 * ASFLAGS -include $(CONFIG_MODEL_HEADER) in Makefile.common.
 * If compiling outside the build system, ensure rvmodel_macros.h
 * from the target platform config directory is included first.
 */
#ifndef RVMODEL_HALT_PASS
#error "RVMODEL_* macros not defined. Include rvmodel_macros.h via -include or ASFLAGS."
#endif

#if __riscv_xlen == 64
    #define REGSIZE 8
    #define LOAD    ld
    #define STORE   sd
#else
    #define REGSIZE 4
    #define LOAD    lw
    #define STORE   sw
#endif

    .section .text.init
    .global rvtest_entry_point
rvtest_entry_point:
    /* ---- Only hart 0 continues; others spin ---- */
    csrr    a0, mhartid
    bnez    a0, _secondary_spin

    /* ---- Set up stack pointer (use full stack for hart 0) ---- */
    la      sp, __stack_end

#ifdef SMDBLTRP_SUPPORTED
    /* ---- Smdbltrp: clear MDT immediately ----
     *
     * When Smdbltrp is enabled, Spike sets mstatus.MDT=1 at reset.
     * If any synchronous trap occurs while MDT=1, it becomes a
     * double trap (critical error), causing an unrecoverable hang.
     *
     * We must clear MDT BEFORE any instruction that could potentially
     * trap. Use csrw mstatus (pure write, no read) to set MDT=0 while
     * keeping MPP=M-mode (0b11 at bits [12:11]).
     */
    li      t0, (3 << 11)       /* MPP=M, MDT=0, all other bits=0 */
    csrw    mstatus, t0
#endif

    /* ---- Set up early trap vector ----
     *
     * Use the common m_trap_entry handler at boot.
     * Projects that need Smdbltrp-specific trap handling (e.g., Smdbltrp,
     * Ssdbltrp) should install smdbltrp_m_trap_entry in their main.c.
     */
    la      t0, m_trap_entry
    csrw    mtvec, t0

    /* ---- Clear BSS ---- */
    la      a0, _bss_start
    la      a1, _bss_end
_clear_bss:
    bgeu    a0, a1, _bss_done
    STORE   zero, 0(a0)
    addi    a0, a0, REGSIZE
    j       _clear_bss
_bss_done:

    /* ---- Platform-specific boot (earliest HW init) ---- */
#ifdef RVMODEL_BOOT
    RVMODEL_BOOT
#endif

    /* ---- Call platform-specific init (weak symbol, default=nop) ---- */
    call    _platform_init

    /* ---- Jump to C main ---- */
    call    main

    /* ---- Halt after main returns ---- */
_halt:
    bnez    a0, _halt_fail
    RVMODEL_HALT_PASS
    j       _halt_loop
_halt_fail:
    RVMODEL_HALT_FAIL
_halt_loop:
    wfi
    j       _halt_loop

_secondary_spin:
    wfi
    j       _secondary_spin

    /* ---- Platform-specific data section (e.g., semihost exit codes, tohost) ---- */
    RVMODEL_DATA_SECTION
