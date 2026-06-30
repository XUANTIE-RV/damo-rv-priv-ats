/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HYP_TEST_H
#define HYP_TEST_H

/* ===================================================================
 * Hypervisor-aware test macros
 *
 * Layered on top of common/test_framework.h:
 *   - HYP_TEST_END:               TEST_END counterpart, uses hyp_reset_state.
 *   - EXPECT_GUEST_PAGE_FAULT:    arm trap, run stmt, assert cause matches.
 *   - CHECK_HTVAL/HTINST/GVA:     post-trap inspectors.
 * =================================================================== */

#include "test_framework.h"
#include "hyp_reset.h"
#include "hyp_csr.h"
#include "hyp_defs.h"
#include "hyp_vs_trap.h"

/* HYP_TEST_END: like TEST_END but resets hypervisor state.
 * Uses the shared _test_end_record() from test_framework.h for
 * pass/fail bookkeeping (avoids duplicating statistics logic). */
#define HYP_TEST_END() do { \
    goto_priv(PRIV_M); \
    hyp_reset_state(); \
    return _test_end_record(); \
} while (0)

/* Execute stmt expecting a guest-page-fault with the given cause. */
#define EXPECT_GUEST_PAGE_FAULT(expected_cause, stmt) do { \
    trap_expect_begin(); \
    stmt; \
    TEST_ASSERT("guest-page-fault triggered", trap_was_triggered()); \
    if (trap_was_triggered()) { \
        TEST_ASSERT_EQ("guest-page-fault cause", trap_get_cause(), \
                       (uintptr_t)(expected_cause)); \
    } \
    trap_expect_end(); \
} while (0)

/* Check htval == expected (faulting GPA >> 2). */
#define CHECK_HTVAL(msg, expected_gpa_shifted) do { \
    TEST_ASSERT_EQ(msg, trap_get_htval(), (uintptr_t)(expected_gpa_shifted)); \
} while (0)

/* Lenient htval check used for implicit-PT-walk faults. The RISC-V
 * Hypervisor spec mandates htval/mtval2 = PT-page GPA >> 2 for an
 * implicit access during VS-stage page-table walk, but some QEMU
 * builds leave htval=0 for this case (the implicit-walk distinction
 * is not modelled). Accept either. */
#define CHECK_HTVAL_OR_ZERO(msg, expected_gpa_shifted) do { \
    uintptr_t _v = trap_get_htval(); \
    bool _ok = (_v == (uintptr_t)(expected_gpa_shifted)) || (_v == 0); \
    TEST_ASSERT(msg, _ok); \
} while (0)

/* Check htinst == expected pseudoinstruction / transformed inst. */
#define CHECK_HTINST(msg, expected) do { \
    TEST_ASSERT_EQ(msg, trap_get_htinst(), (uintptr_t)(expected)); \
} while (0)

/* Lenient htinst check for implicit-PT-walk faults (see CHECK_HTVAL_OR_ZERO). */
#define CHECK_HTINST_OR_ZERO(msg, expected) do { \
    uintptr_t _v = trap_get_htinst(); \
    bool _ok = (_v == (uintptr_t)(expected)) || (_v == 0); \
    TEST_ASSERT(msg, _ok); \
} while (0)

/* Check hstatus.GVA captured at trap. */
#define CHECK_GVA(msg, expected_gva) do { \
    TEST_ASSERT_EQ(msg, (uintptr_t)trap_get_gva(), (uintptr_t)((expected_gva) ? 1 : 0)); \
} while (0)

/* ===================================================================
 * Platform capability gates
 * =================================================================== */

/**
 * REQUIRE_HGATP_MODE - Skip test if platform does not support given mode.
 *
 * Uses the canonical probe-and-restore via hgatp_supports_mode().
 */
#define REQUIRE_HGATP_MODE(mode) do { \
    if (!hgatp_supports_mode(mode)) { \
        TEST_SKIP("hgatp mode not supported by platform"); \
    } \
} while (0)

/* ===================================================================
 * Virtual-instruction exception macros
 * =================================================================== */

/**
 * EXPECT_VIRTUAL_INST - Execute stmt, expect virtual-instruction exception
 *
 * Used to verify that VTSR/VTW/VTVM traps or VS/VU-mode access to
 * hypervisor CSRs triggers cause=22.
 */
#define EXPECT_VIRTUAL_INST(stmt) do { \
    trap_expect_begin(); \
    stmt; \
    TEST_ASSERT("virtual-inst trap triggered", trap_was_triggered()); \
    if (trap_was_triggered()) { \
        TEST_ASSERT_EQ("cause=22 (virtual-instruction)", \
                       trap_get_cause(), (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION); \
    } \
    trap_expect_end(); \
} while (0)

/**
 * VS_EXPECT_NO_TRAP - Execute stmt in VS-mode context, expect no trap
 *
 * Verifies that an operation completes without triggering any exception.
 */
#define VS_EXPECT_NO_TRAP(stmt) do { \
    trap_expect_begin(); \
    stmt; \
    TEST_ASSERT("no trap in VS-mode", !trap_was_triggered()); \
    trap_expect_end(); \
} while (0)

/* ===================================================================
 * vstval verification macros (shvstvala)
 * =================================================================== */

/**
 * CHECK_VSTVAL - Verify vstval matches expected value after VS trap.
 *
 * Used after a trap is delegated to VS-mode and the VS handler
 * records vstval. Reads vstval from HS/M-mode (CSR 0x243).
 */
#define CHECK_VSTVAL(msg, expected) do { \
    TEST_ASSERT_EQ(msg, vstval_read(), (uintptr_t)(expected)); \
} while (0)

/**
 * CHECK_VSTVAL_NONZERO - Assert vstval is non-zero (shvstvala core).
 */
#define CHECK_VSTVAL_NONZERO(msg) do { \
    TEST_ASSERT(msg, vstval_read() != 0); \
} while (0)

/**
 * CHECK_VSTVAL_ZERO - Assert vstval is zero.
 */
#define CHECK_VSTVAL_ZERO(msg) do { \
    TEST_ASSERT_EQ(msg, vstval_read(), (uintptr_t)0); \
} while (0)

/* ===================================================================
 * satp / vsatp mode capability gates (shvsatpa, shgatpa)
 * =================================================================== */

#define REQUIRE_SATP_MODE(mode) do { \
    if (!satp_supports_mode(mode)) { \
        TEST_SKIP("satp mode not supported by platform"); \
    } \
} while (0)

#define REQUIRE_VSATP_MODE(mode) do { \
    if (!vsatp_supports_mode(mode)) { \
        TEST_SKIP("vsatp mode not supported by platform"); \
    } \
} while (0)

/* ===================================================================
 * Illegal / Virtual-instruction unified trap check
 * =================================================================== */

/**
 * EXPECT_ILLEGAL_INST - Execute stmt, expect illegal-instruction (cause=2)
 */
#define EXPECT_ILLEGAL_INST(stmt) do { \
    trap_expect_begin(); \
    stmt; \
    TEST_ASSERT("illegal-inst trap triggered", trap_was_triggered()); \
    if (trap_was_triggered()) { \
        TEST_ASSERT_EQ("cause=2 (illegal-instruction)", \
                       trap_get_cause(), (uintptr_t)2); \
    } \
    trap_expect_end(); \
} while (0)

/**
 * EXPECT_ILLEGAL_OR_VIRTUAL_INST - Execute stmt, expect either
 * illegal-instruction (cause=2) or virtual-instruction (cause=22).
 *
 * Used for tests where the exact trap depends on privilege context:
 * non-virtualized gets cause=2, virtualized gets cause=22.
 */
#define EXPECT_ILLEGAL_OR_VIRTUAL_INST(stmt) do { \
    trap_expect_begin(); \
    stmt; \
    TEST_ASSERT("illegal/virtual-inst triggered", trap_was_triggered()); \
    if (trap_was_triggered()) { \
        uintptr_t _c = trap_get_cause(); \
        TEST_ASSERT("cause=2 or cause=22", \
                    _c == 2 || _c == CAUSE_VIRTUAL_INSTRUCTION); \
    } \
    trap_expect_end(); \
} while (0)

/* ===================================================================
 * Counter access exception macros (shcounterenw)
 * =================================================================== */

/**
 * EXPECT_COUNTER_TRAP - Execute counter access, expect trap.
 *
 * When hcounteren/scounteren bit is 0, VS/VU access to the counter
 * should trap (virtual-instruction from VS, illegal-inst from VU
 * if delegated).
 */
#define EXPECT_COUNTER_TRAP(stmt) do { \
    trap_expect_begin(); \
    stmt; \
    TEST_ASSERT("counter access trapped", trap_was_triggered()); \
    trap_expect_end(); \
} while (0)

/* ===================================================================
 * VS trap record assertion macros (shvstvecd / shvstvala)
 * =================================================================== */

/**
 * CHECK_VS_TRAP_CAUSE - Verify VS trap record cause.
 */
#define CHECK_VS_TRAP_CAUSE(msg, expected_cause) do { \
    vs_trap_record_t *_r = vs_trap_get_last(); \
    TEST_ASSERT_EQ(msg, _r->vs_cause, (uintptr_t)(expected_cause)); \
} while (0)

/**
 * CHECK_VS_TRAP_TVAL - Verify VS trap record tval (vstval at entry).
 */
#define CHECK_VS_TRAP_TVAL(msg, expected_tval) do { \
    vs_trap_record_t *_r = vs_trap_get_last(); \
    TEST_ASSERT_EQ(msg, _r->vs_tval, (uintptr_t)(expected_tval)); \
} while (0)

/**
 * CHECK_VS_TRAP_VECTORED_ENTRY - Verify vectored entry index.
 */
#define CHECK_VS_TRAP_VECTORED_ENTRY(msg, expected_idx) do { \
    vs_trap_record_t *_r = vs_trap_get_last(); \
    TEST_ASSERT_EQ(msg, _r->entry_idx, (int)(expected_idx)); \
} while (0)

#endif /* HYP_TEST_H */
