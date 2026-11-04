/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 2: hgatp SvNNx4 MODE consistency with satp
 *
 * Spec anchors:
 *   norm:shgatpa_satp_hgatp_mode_support (shgatpa.adoc:4-7)
 *   norm:hgatp_sz_acc_op                 (hypervisor.adoc:989-994)
 *   norm:hgatp_mode_sv                   (hypervisor.adoc:1020-1027)
 *
 * Test list (matches DOCS/testplan/shgatpa_test_plan.md HGATP-MODE):
 *   HGATP-MODE-01  hgatp Sv39x4 support (if satp supports Sv39)
 *   HGATP-MODE-02  hgatp Sv48x4 support (if satp supports Sv48)
 *   HGATP-MODE-03  hgatp Sv57x4 support (if satp supports Sv57)
 *   HGATP-MODE-04  hgatp mode cross-switching
 *
 * Core Shgatpa assertion: for every satp SvNN, writing hgatp SvNNx4
 * must read back the same MODE value.
 * =================================================================== */

/* The SvNN antecedent of "hgatp supports SvNNx4 if satp supports SvNN" is
 * taken from the platform config's compile-time SV39/48/57_AVAILABLE
 * declarations (common/capabilities.h) through REQUIRE_SATP_MODE
 * (hyp/hyp_test.h), not from a runtime satp probe result handed over by
 * Group 1. A config that declares a mode the DUT then rejects makes the
 * case FAIL instead of silently skipping it. */

/* ---- HGATP-MODE-01 ---- */
TEST_REGISTER(test_shgatpa_mode_sv39x4);
bool test_shgatpa_mode_sv39x4(void) {
    TEST_BEGIN("HGATP-MODE-01: hgatp supports Sv39x4 if satp supports Sv39");

    REQUIRE_SATP_MODE(SATP_MODE_SV39);

    uintptr_t saved = hgatp_read();

    hgatp_set_bare();
    hgatp_write_raw(MAKE_HGATP(HGATP_MODE_SV39X4, 0, 0));
    uintptr_t mode = HGATP_GET_MODE(hgatp_read());

    TEST_ASSERT_EQ("hgatp.MODE == Sv39x4 (Shgatpa: satp Sv39 -> hgatp Sv39x4)",
                   mode, (uintptr_t)HGATP_MODE_SV39X4);

    hgatp_write_raw(saved);
    HYP_TEST_END();
}

/* ---- HGATP-MODE-02 ---- */
TEST_REGISTER(test_shgatpa_mode_sv48x4);
bool test_shgatpa_mode_sv48x4(void) {
    TEST_BEGIN("HGATP-MODE-02: hgatp supports Sv48x4 if satp supports Sv48");

    REQUIRE_SATP_MODE(SATP_MODE_SV48);

    uintptr_t saved = hgatp_read();

    hgatp_set_bare();
    hgatp_write_raw(MAKE_HGATP(HGATP_MODE_SV48X4, 0, 0));
    uintptr_t mode = HGATP_GET_MODE(hgatp_read());

    TEST_ASSERT_EQ("hgatp.MODE == Sv48x4 (Shgatpa: satp Sv48 -> hgatp Sv48x4)",
                   mode, (uintptr_t)HGATP_MODE_SV48X4);

    hgatp_write_raw(saved);
    HYP_TEST_END();
}

/* ---- HGATP-MODE-03 ---- */
TEST_REGISTER(test_shgatpa_mode_sv57x4);
bool test_shgatpa_mode_sv57x4(void) {
    TEST_BEGIN("HGATP-MODE-03: hgatp supports Sv57x4 if satp supports Sv57");

    REQUIRE_SATP_MODE(SATP_MODE_SV57);

    uintptr_t saved = hgatp_read();

    hgatp_set_bare();
    hgatp_write_raw(MAKE_HGATP(HGATP_MODE_SV57X4, 0, 0));
    uintptr_t mode = HGATP_GET_MODE(hgatp_read());

    TEST_ASSERT_EQ("hgatp.MODE == Sv57x4 (Shgatpa: satp Sv57 -> hgatp Sv57x4)",
                   mode, (uintptr_t)HGATP_MODE_SV57X4);

    hgatp_write_raw(saved);
    HYP_TEST_END();
}

/* ---- HGATP-MODE-04 ---- */
TEST_REGISTER(test_shgatpa_mode_cross_switch);
bool test_shgatpa_mode_cross_switch(void) {
    TEST_BEGIN("HGATP-MODE-04: hgatp mode cross-switching");

    uintptr_t saved = hgatp_read();
    uintptr_t mode;

    /* Step 1: Set Bare */
    hgatp_set_bare();
    mode = HGATP_GET_MODE(hgatp_read());
    TEST_ASSERT_EQ("hgatp.MODE == Bare after initial set",
                   mode, (uintptr_t)HGATP_MODE_BARE);

    /* Step 2: Switch to each supported SvNNx4 mode and back */
    if (SV39_AVAILABLE) {
        hgatp_write_raw(MAKE_HGATP(HGATP_MODE_SV39X4, 0, 0));
        mode = HGATP_GET_MODE(hgatp_read());
        TEST_ASSERT_EQ("hgatp.MODE == Sv39x4 after switch",
                       mode, (uintptr_t)HGATP_MODE_SV39X4);
    }

    if (SV48_AVAILABLE) {
        hgatp_write_raw(MAKE_HGATP(HGATP_MODE_SV48X4, 0, 0));
        mode = HGATP_GET_MODE(hgatp_read());
        TEST_ASSERT_EQ("hgatp.MODE == Sv48x4 after switch",
                       mode, (uintptr_t)HGATP_MODE_SV48X4);
    }

    if (SV57_AVAILABLE) {
        hgatp_write_raw(MAKE_HGATP(HGATP_MODE_SV57X4, 0, 0));
        mode = HGATP_GET_MODE(hgatp_read());
        TEST_ASSERT_EQ("hgatp.MODE == Sv57x4 after switch",
                       mode, (uintptr_t)HGATP_MODE_SV57X4);
    }

    /* Step 3: Switch back to Bare */
    hgatp_set_bare();
    mode = HGATP_GET_MODE(hgatp_read());
    TEST_ASSERT_EQ("hgatp.MODE == Bare after final switch",
                   mode, (uintptr_t)HGATP_MODE_BARE);

    hgatp_write_raw(saved);
    HYP_TEST_END();
}
