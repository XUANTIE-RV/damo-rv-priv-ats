#include "encoding.h"

#if __riscv_xlen == 64
    #define REGSIZE 8
    #define LOAD    ld
    #define STORE   sd
#else
    #define REGSIZE 4
    #define LOAD    lw
    #define STORE   sw
#endif

/*
 * Common platform hardware initialization
 *
 * _platform_hw_init: Performs platform-specific CSR setup that all
 * projects need (e.g., clearing T-Head MAEE bit). This is a strong
 * symbol that can be called by any project's _platform_init.
 *
 * _platform_init (weak): Default platform init that just calls
 * _platform_hw_init. Projects with additional init needs (e.g., PMP
 * test code region filling) can override this weak symbol with their
 * own strong _platform_init that calls _platform_hw_init first.
 */

/* ===================================================================
 * _platform_hw_init - Hardware-level platform initialization
 *
 * Called by _platform_init (or overrides thereof) to perform
 * platform-specific CSR setup common to all projects.
 * =================================================================== */
    .section .text
    .global _platform_hw_init
_platform_hw_init:

#ifdef PLATFORM_CLEAR_MAEE
    /*
     * T-Head C908/C910: clear MAEE (mxstatus bit 21) so PTE uses
     * standard RISC-V format. When MAEE=0, PTE[63:59] are reserved
     * and DDR uses default cacheable attribute, ensuring AMO/LR/SC
     * work correctly via local monitor without explicit PTE cache bits.
     * Ref: C908 User Manual Section 6.2.3, 7.3.3, Appendix C.
     */
    csrr    t1, 0x7C0
    li      t2, (1 << 21)
    not     t2, t2
    and     t1, t1, t2
    csrw    0x7C0, t1
#endif

    ret

/* ===================================================================
 * _platform_init (weak) - Default platform initialization
 *
 * Simply calls _platform_hw_init. Projects that need additional
 * initialization (e.g., PMP test code filling) should provide their
 * own strong _platform_init that calls _platform_hw_init first,
 * then performs project-specific setup.
 * =================================================================== */
    .weak   _platform_init
    .global _platform_init
_platform_init:
    addi    sp, sp, -REGSIZE
    STORE   ra, 0(sp)

    call    _platform_hw_init

    LOAD    ra, 0(sp)
    addi    sp, sp, REGSIZE
    ret
