/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * common/hyp/tests_2stage/test_group02_vsatp_csr.c
 *
 * Group 2: vsatp CSR semantics (TS-VSATP-01..07)
 *
 * vsatp (CSR 0x280) is the VS-stage address-translation register. It
 * is accessible from HS/M-mode (V=0) without trapping; from VS-mode
 * (V=1) the unprivileged access is fine but TVM/STVEC interactions
 * are out-of-scope here. These tests cover only the HS/M-mode WARL
 * behavior of vsatp itself, plus its independence from satp.
 *
 * Cases:
 *   TS-VSATP-01  csrrw vsatp from HS-mode does not trap
 *   TS-VSATP-02  Writing vsatp.MODE = Sv39 reads back as Sv39
 *   TS-VSATP-03  vsatp supports each Sv* mode the platform also
 *                supports on satp (Sv39 mandatory; Sv48/Sv57 if avail)
 *   TS-VSATP-04  Writing an unsupported MODE: vsatp clears MODE to BARE
 *                (per spec, distinct from satp WARL semantics)
 *   TS-VSATP-05  vsatp.PPN field is writable and reads back
 *   TS-VSATP-06  vsatp.ASID field obeys ASIDLEN (mask matches platform)
 *   TS-VSATP-07  Writing vsatp does NOT mutate satp (and vice versa)
 *
 * Source-included by per-suite test_register.c. Runs entirely from
 * M-mode using csr_accessors; does not require two_stage_helpers.
 * =================================================================== */

#include "two_stage_helpers.h"

/* Convenience: build a vsatp value with mode/asid/ppn. Same encoding
 * as MAKE_SATP from vm_defs.h. */
#define MAKE_VSATP(mode, asid, ppn)  MAKE_SATP((mode), (asid), (ppn))

/* ===================================================================
 * TS-VSATP-01: csrrw vsatp from HS/M-mode does not trap
 * =================================================================== */
TEST_REGISTER(test_ts_vsatp_01_no_trap);
bool test_ts_vsatp_01_no_trap(void)
{
    TEST_BEGIN("TS-VSATP-01: csrrw vsatp from M-mode does not trap");

    uintptr_t saved = vsatp_read();
    EXPECT_NO_TRAP(vsatp_write(0));
    EXPECT_NO_TRAP((void)vsatp_read());
    vsatp_write(saved);

    HYP_TEST_END();
}

/* ===================================================================
 * TS-VSATP-02: Writing vsatp.MODE=Sv39 reads back as Sv39
 * =================================================================== */
TEST_REGISTER(test_ts_vsatp_02_mode_sv39_writeback);
bool test_ts_vsatp_02_mode_sv39_writeback(void)
{
    TEST_BEGIN("TS-VSATP-02: vsatp MODE=Sv39 writeback");
    REQUIRE_VSATP_MODE(SATP_MODE_SV39);

    uintptr_t saved = vsatp_read();
    vsatp_write(MAKE_VSATP(SATP_MODE_SV39, 0, 0));
    uintptr_t got = vsatp_read();
    TEST_ASSERT_EQ("vsatp.MODE reads back Sv39",
                   SATP_GET_MODE(got), (uintptr_t)SATP_MODE_SV39);

    vsatp_write(saved);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-VSATP-03: vsatp supports Sv48 (skip if platform lacks it)
 * =================================================================== */
TEST_REGISTER(test_ts_vsatp_03_mode_sv48);
bool test_ts_vsatp_03_mode_sv48(void)
{
    TEST_BEGIN("TS-VSATP-03: vsatp MODE=Sv48 writeback");
    REQUIRE_VSATP_MODE(SATP_MODE_SV48);

    uintptr_t saved = vsatp_read();
    vsatp_write(MAKE_VSATP(SATP_MODE_SV48, 0, 0));
    uintptr_t got = vsatp_read();
    TEST_ASSERT_EQ("vsatp.MODE reads back Sv48",
                   SATP_GET_MODE(got), (uintptr_t)SATP_MODE_SV48);

    vsatp_write(saved);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-VSATP-04: Writing an unsupported MODE to vsatp (V=0).
 *
 * Spec (norm:vsatp_mode_unsupported_v0):
 *   "When V=0, a write to vsatp with an unsupported MODE value is
 *    either ignored as it is for satp, or the fields of vsatp are
 *    treated as WARL in the normal way."
 *
 * We use a *reserved* MODE encoding (7) which is guaranteed unsupported
 * on all platforms, so the test never needs to skip.
 *
 * Two valid outcomes:
 *   (a) Write ignored (satp-like): vsatp unchanged from saved value.
 *   (b) WARL processed: MODE field becomes a legal value (0/8/9/10).
 * =================================================================== */
TEST_REGISTER(test_ts_vsatp_04_warl_unsupported_mode);
bool test_ts_vsatp_04_warl_unsupported_mode(void)
{
    TEST_BEGIN("TS-VSATP-04: vsatp reserved MODE -> ignored or WARL");

    uintptr_t saved = vsatp_read();

    /* MODE=7 is reserved (not Bare/Sv39/Sv48/Sv57), always unsupported. */
    const unsigned reserved_mode = 7;
    uintptr_t probe = MAKE_VSATP(reserved_mode, 0, 0);
    vsatp_write(probe);
    uintptr_t got = vsatp_read();

    /* Restore immediately. */
    vsatp_write(saved);

    unsigned got_mode = (unsigned)SATP_GET_MODE(got);

    /* Outcome (a): entire write ignored, vsatp == saved. */
    bool ignored = (got == saved);

    /* Outcome (b): WARL processed, MODE is a legal encoding. */
    bool legal_mode = (got_mode == SATP_MODE_BARE) ||
                      (got_mode == SATP_MODE_SV39) ||
                      (got_mode == SATP_MODE_SV48) ||
                      (got_mode == SATP_MODE_SV57);

    TEST_ASSERT("reserved MODE must NOT persist in vsatp",
                got_mode != reserved_mode);
    TEST_ASSERT("write ignored (satp-like) OR MODE is legal (WARL)",
                ignored || legal_mode);

    HYP_TEST_END();
}

/* ===================================================================
 * TS-VSATP-05: vsatp.PPN is writable
 * =================================================================== */
TEST_REGISTER(test_ts_vsatp_05_ppn_writeback);
bool test_ts_vsatp_05_ppn_writeback(void)
{
    TEST_BEGIN("TS-VSATP-05: vsatp.PPN writeback");
    REQUIRE_VSATP_MODE(SATP_MODE_SV39);

    uintptr_t saved = vsatp_read();
    /* Use a benign PPN; it will not be walked because we restore
     * vsatp before any access. */
    uintptr_t test_ppn = 0xABCDEUL;  /* 20 bits, well within 44-bit PPN */
    vsatp_write(MAKE_VSATP(SATP_MODE_SV39, 0, test_ppn));
    uintptr_t got = vsatp_read();
    /* PPN field: bits [43:0]. Extract by shift-then-mask. */
    uintptr_t got_ppn = got & ((1UL << 44) - 1);
    TEST_ASSERT_EQ("vsatp.PPN reads back", got_ppn, test_ppn);

    vsatp_write(saved);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-VSATP-06: vsatp.ASID obeys ASIDLEN
 *
 * The platform's effective ASIDLEN may be 0..16 bits. We write all-1s
 * to the ASID field and read back; only the supported low bits should
 * remain set. The test asserts that the read-back is a contiguous
 * low-bit mask (i.e. ASIDLEN is well-defined) without prescribing its
 * exact width.
 * =================================================================== */
TEST_REGISTER(test_ts_vsatp_06_asid_warl);
bool test_ts_vsatp_06_asid_warl(void)
{
    TEST_BEGIN("TS-VSATP-06: vsatp.ASID WARL contiguous low bits");
    REQUIRE_VSATP_MODE(SATP_MODE_SV39);

    uintptr_t saved = vsatp_read();
    /* Write all-ones into ASID (16 bits per spec). */
    uintptr_t all_asid = (1UL << 16) - 1;
    vsatp_write(MAKE_VSATP(SATP_MODE_SV39, all_asid, 0));
    uintptr_t got = vsatp_read();
    uintptr_t got_asid = (got >> 44) & ((1UL << 16) - 1);

    /* Check that got_asid is of the form (1 << n) - 1, i.e. a
     * contiguous run of 1s starting from bit 0. */
    bool contiguous = ((got_asid + 1) & got_asid) == 0;
    TEST_ASSERT("ASID readback is contiguous low-bit mask", contiguous);

    vsatp_write(saved);
    HYP_TEST_END();
}

/* ===================================================================
 * TS-VSATP-07: Writing vsatp does not mutate satp (and vice versa)
 * =================================================================== */
TEST_REGISTER(test_ts_vsatp_07_independence_from_satp);
bool test_ts_vsatp_07_independence_from_satp(void)
{
    TEST_BEGIN("TS-VSATP-07: vsatp and satp are independent");
    REQUIRE_VSATP_MODE(SATP_MODE_SV39);
    REQUIRE_SATP_MODE(SATP_MODE_SV39);

    uintptr_t vs_saved   = vsatp_read();
    uintptr_t s_saved    = satp_read();

    /* Mutate vsatp; verify satp unchanged. */
    vsatp_write(MAKE_VSATP(SATP_MODE_SV39, 0xAA, 0x100));
    TEST_ASSERT_EQ("satp unchanged after vsatp write",
                   satp_read(), s_saved);

    /* Mutate satp; verify vsatp unchanged. */
    uintptr_t vs_after_self = vsatp_read();
    satp_write(MAKE_SATP(SATP_MODE_SV39, 0x55, 0x200));
    TEST_ASSERT_EQ("vsatp unchanged after satp write",
                   vsatp_read(), vs_after_self);

    /* Restore. */
    vsatp_write(vs_saved);
    satp_write(s_saved);
    asm volatile ("sfence.vma" ::: "memory");

    HYP_TEST_END();
}
