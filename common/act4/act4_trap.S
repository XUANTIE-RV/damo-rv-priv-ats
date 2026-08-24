/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * ===================================================================
 * common/act4/act4_trap.S — ACT4 trap handler, assembly side
 *
 * Instantiates the ACT4 trampolines, save areas, prolog and epilog,
 * and supplies the environment symbols the handler expects.
 *
 * The handler hands a finished trap to act4_policy_mret /
 * act4_policy_sret, which live beside the suite's other trap entry
 * points in common/trap_asm.S.
 *
 * Built only when TRAP_HANDLER=act4.
 * ===================================================================
 */

#include "act4/act4_config.h"

#ifdef ACT4_TRAP_HANDLER

#include "act4/act4_trap_entry.h"
#include "act4/encoding.h"
#include "act4/utils.h"
#include "act4/signature.h"
#include "act4/rvtest_trap_handler.h"

/* Position of each mode's save area, in units of sv_area_sz. The
 * epilog needs this to find the area belonging to the mode it is
 * cleaning up; see the note in RVTEST_TRAP_EPILOG. */
        .set ACT4_SVIDX_M, 0
        .set ACT4_SVIDX_S, 1

/* ===================================================================
 * Trap handlers
 *
 * M-mode plus, where S-mode exists, one S-mode handler for stvec.
 *
 * ACT4 can also instantiate an HS-mode handler (hstatus/htval/hgatp
 * aliased onto CSR_X*) and a VS-mode one. Neither is emitted here.
 * HS would record htval where this suite asserts on stval, and htval
 * and htinst are captured separately at trap entry anyway; VS-mode
 * vectors belong to the tests, which install and check their own
 * (common/hyp/hyp_vs_trap.c).
 * =================================================================== */
        .section .text.rvtest, "ax", @progbits

        RVTEST_TRAP_HANDLER M
#ifdef S_SUPPORTED
        RVTEST_TRAP_HANDLER S
#endif

/* ===================================================================
 * act4_trap_init - run the ACT4 prolog
 *
 * Points xTVEC at the trampolines, xSCRATCH at the per-mode save
 * areas, and saves and clears xEDELEG. Where xTVEC is not fully
 * writable the prolog copies the trampoline into the fixed xTVEC
 * target instead and records the address it settled on.
 *
 * The prolog works in T1..T6, which are s0/s1 in the C ABI, and its
 * relocation path also writes sp, so ra and the callee-saved
 * registers are parked in memory rather than on the stack.
 * =================================================================== */
        .section .text, "ax", @progbits
        .balign 4
        .global act4_trap_init
        .type   act4_trap_init, @function
act4_trap_init:
        LA(     T1, act4_init_sv)
        SREG    ra,  0*REGWIDTH(T1)
        SREG    s0,  1*REGWIDTH(T1)
        SREG    s1,  2*REGWIDTH(T1)
        SREG    sp,  3*REGWIDTH(T1)

        RVTEST_TRAP_PROLOG M
#ifdef S_SUPPORTED
        RVTEST_TRAP_PROLOG S
#endif

        LA(     T1, act4_init_sv)
        LREG    sp,  3*REGWIDTH(T1)
        LREG    s1,  2*REGWIDTH(T1)
        LREG    s0,  1*REGWIDTH(T1)
        LREG    ra,  0*REGWIDTH(T1)
        ret
        .size   act4_trap_init, .-act4_trap_init

/* ===================================================================
 * act4_reinstall_tvecs - put xTVEC and xSCRATCH back
 *
 * reset_state() runs before every test, and tests move xTVEC and
 * xSCRATCH around as part of what they check. Restores the trampoline
 * address the prolog settled on, together with the save-area pointer
 * the handler expects to find in xSCRATCH.
 * =================================================================== */
        .balign 4
        .global act4_reinstall_tvecs
        .type   act4_reinstall_tvecs, @function
act4_reinstall_tvecs:
        LA(     T1, Mtramptbl_sv)
        LREG    T2, xtvec_new_off(T1)
        csrw    mtvec, T2
        csrw    mscratch, T1
#ifdef S_SUPPORTED
        LA(     T1, Stramptbl_sv)
        LREG    T2, xtvec_new_off(T1)
        csrw    stvec, T2
        csrw    sscratch, T1
#endif
        ret
        .size   act4_reinstall_tvecs, .-act4_reinstall_tvecs

#ifdef S_SUPPORTED
/* ===================================================================
 * act4_stvec_value - the stvec value that routes traps to the handler
 *
 * Not &Strampoline: where stvec is not fully writable the prolog
 * relocates the trampoline, and this is the address it chose.
 * =================================================================== */
        .balign 4
        .global act4_stvec_value
        .type   act4_stvec_value, @function
act4_stvec_value:
        LA(     T1, Stramptbl_sv)
        LREG    a0, xtvec_new_off(T1)
        ret
        .size   act4_stvec_value, .-act4_stvec_value
#endif

/* ===================================================================
 * act4_trap_fini - run the ACT4 epilog
 *
 * Restores the original xTVEC, xSCRATCH and xEDELEG. The suite exits
 * through RVMODEL_HALT_PASS/FAIL rather than here, but the epilog
 * also defines exit_Xcleanup and rvtest_Xend, which the prolog's
 * abort path and the trampoline's impossible-cause path branch to.
 * =================================================================== */
        .balign 4
        .global act4_trap_fini
        .type   act4_trap_fini, @function
act4_trap_fini:
        LA(     T1, act4_init_sv)
        SREG    ra,  0*REGWIDTH(T1)
        SREG    s0,  1*REGWIDTH(T1)
        SREG    s1,  2*REGWIDTH(T1)
        SREG    sp,  3*REGWIDTH(T1)

        RVTEST_TRAP_EPILOG M
#ifdef S_SUPPORTED
        RVTEST_TRAP_EPILOG S
#endif

        LA(     T1, act4_init_sv)
        LREG    sp,  3*REGWIDTH(T1)
        LREG    s1,  2*REGWIDTH(T1)
        LREG    s0,  1*REGWIDTH(T1)
        LREG    ra,  0*REGWIDTH(T1)
        ret
        .size   act4_trap_fini, .-act4_trap_fini

/* ===================================================================
 * ACT4 environment symbols
 *
 * The handler expects these from ACT4's rvtest_setup.h and
 * rvtest_failure_code.h, which come with test scaffolding this suite
 * replaces with its own. Mapped onto the suite's runtime instead.
 * =================================================================== */
        .section .text.rvmodel, "ax", @progbits

/* Segment bounds, stored in every save area and used to express a
 * captured xEPC as a segment-relative offset. Nothing reads them here:
 * the raw xEPC is recorded instead, because a test may fault on an
 * address that lies outside every segment. Weak, so they resolve to
 * zero unless a linker script defines them. */
        .weak   rvtest_code_begin
        .weak   rvtest_code_end
        .weak   rvtest_data_begin
        .weak   rvtest_data_end

/* Unrecoverable handler states: a trampoline that could not be
 * installed, an xEPC outside every segment, a trap-signature overrun.
 * All end the run with the suite's fail status. */
        .balign 4
        .global abort_test
        .global trap_sig_overflow
        .global cleanup_epilogs
abort_test:
trap_sig_overflow:
cleanup_epilogs:
        LA(     a0, act4_abort_str)
        call    act4_report_fatal              /* does not return */
1:      j       1b

/* Used by the "no dispatch-table entry" report. */
        .balign 4
        .global rvmodel_io_write_str
rvmodel_io_write_str:
        tail    act4_io_write_str

        .balign 4
        .global rvmodel_halt_fail
rvmodel_halt_fail:
        tail    _halt_fail

        .balign 4
        .global failedtest_hex_to_str
failedtest_hex_to_str:
        tail    act4_hex_to_str

/* Signature-mismatch reporter. Unreachable while signature emission is
 * masked -- there is no reference signature to mismatch against -- but
 * TRAP_SIGUPD keeps the reference alive. */
        .balign 4
        .global failedtest_trap_x7_x9
        .global failedtest_print_csr_context
failedtest_trap_x7_x9:
failedtest_print_csr_context:
        LA(     a0, act4_sigmismatch_str)
        call    act4_report_fatal
1:      j       1b

/* ===================================================================
 * Data
 * =================================================================== */
        .section .data
        .balign 8

/* Incremented by every handler entry. */
        .global rvtest_trap_count
rvtest_trap_count:      .dword 0

/* xepc, xcause, xtval and xstatus, snapshotted before the first
 * trap-signature word is written. */
        .global saved_xepc
saved_xepc:             .fill 4, 8, 0

/* Where trap-signature words land while emission is masked. */
        .global act4_trap_sig_shadow
act4_trap_sig_shadow:   .fill ACT4_TRAP_SIG_SHADOW_WORDS, REGWIDTH, 0

/* Trapping state, captured at trap entry. */
        .global act4_trap_entry
act4_trap_entry:        .fill ACT4_ENT_NWORDS, REGWIDTH, 0

/* Whether the hypervisor trap-value CSRs can be read; see act4_glue.c. */
        .global act4_cap_hyp
act4_cap_hyp:           .fill 1, REGWIDTH, 0
        .global act4_cap_xtval2
act4_cap_xtval2:        .fill 1, REGWIDTH, 0

/* Set for one ecall to mark it a T-SBI call. */
        .global act4_tsbi_armed
act4_tsbi_armed:        .fill 1, REGWIDTH, 0

/* ra, s0, s1 and sp across act4_trap_init / act4_trap_fini. */
        .balign 8
act4_init_sv:           .fill 4, REGWIDTH, 0

/* Signature region. Never written while emission is masked; present so
 * trap_sigptr, rvtest_sig_begin/end and sig_end_canary resolve. */
        .balign 8
        .global rvtest_sig_begin
rvtest_sig_begin:
        .global trap_sigptr_area
trap_sigptr_area:       .fill ACT4_SIG_REGION_WORDS, REGWIDTH, 0
        .global sig_end_canary
sig_end_canary:         .fill 1, REGWIDTH, 0
        .global rvtest_sig_end
rvtest_sig_end:
        .balign 8
        .global trap_sigptr
trap_sigptr:            .dword trap_sigptr_area

        .section .rodata
        .balign 8
act4_abort_str:         .asciz "\n!!! ACT4 handler: unrecoverable, aborting !!!\n"
act4_sigmismatch_str:   .asciz "\n!!! ACT4 handler: signature mismatch !!!\n"

        .global tsbi_instr_not_found_str
tsbi_instr_not_found_str: .asciz "\n!!! ACT4 T-SBI: no dispatch-table entry for encoding 0x"
        .global newlinestr
newlinestr:             .asciz "\n"
        .global failstr
failstr:                .asciz "RVCP-SUMMARY: TEST FAILED\n"

        .section .bss
        .balign 8
        .global ascii_buffer
ascii_buffer:           .fill 40, 1, 0

/* ===================================================================
 * Save areas, in the order the handler's offsets index them.
 * =================================================================== */
        .section .data
        .balign 8
        RVTEST_TRAP_SAVEAREA M
#ifdef S_SUPPORTED
        RVTEST_TRAP_SAVEAREA S
#endif

#endif /* ACT4_TRAP_HANDLER */
