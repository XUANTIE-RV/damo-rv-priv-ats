/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 1: satp MODE support probing (baseline establishment)
 *
 * Spec anchors:
 *   norm:satp_mode_sxlen64       (supervisor.adoc:1044-1050)
 *   norm:satp_mode_op_unsupported (supervisor.adoc:1052-1055)
 *
 * Test list (matches DOCS/testplan/shgatpa_test_plan.md HGATP-PROBE):
 *   HGATP-PROBE-01  satp MODE=Bare probe
 *   HGATP-PROBE-02  satp MODE=Sv39 probe
 *   HGATP-PROBE-03  satp MODE=Sv48 probe
 *   HGATP-PROBE-04  satp MODE=Sv57 probe
 *
 * These tests probe satp for each standard MODE, recording results
 * into global booleans for Groups 2-6 to consume.
 * =================================================================== */

/* Global state: which MODEs does satp support? */
volatile bool g_satp_supports_bare = false;
volatile bool g_satp_supports_sv39 = false;
volatile bool g_satp_supports_sv48 = false;
volatile bool g_satp_supports_sv57 = false;

/* ---- HGATP-PROBE-01 ---- */
TEST_REGISTER(test_shgatpa_probe_bare);
bool test_shgatpa_probe_bare(void) {
    TEST_BEGIN("HGATP-PROBE-01: probe satp MODE=Bare support");

    uintptr_t saved = satp_read();

    satp_write(MAKE_SATP(SATP_MODE_BARE, 0, 0));
    uintptr_t mode = SATP_GET_MODE(satp_read());

    if (mode == SATP_MODE_BARE) {
        g_satp_supports_bare = true;
    }

    TEST_ASSERT_EQ("satp MODE readback is Bare (0)", mode,
                   (uintptr_t)SATP_MODE_BARE);

    satp_write(saved);
    HYP_TEST_END();
}

/* ---- HGATP-PROBE-02 ---- */
TEST_REGISTER(test_shgatpa_probe_sv39);
bool test_shgatpa_probe_sv39(void) {
    TEST_BEGIN("HGATP-PROBE-02: probe satp MODE=Sv39 support");

    uintptr_t saved = satp_read();

    satp_write(MAKE_SATP(SATP_MODE_BARE, 0, 0));
    satp_write(MAKE_SATP(SATP_MODE_SV39, 0, 0));
    uintptr_t mode = SATP_GET_MODE(satp_read());

    if (mode == SATP_MODE_SV39) {
        g_satp_supports_sv39 = true;
        LOG_I("satp supports Sv39\n");
    } else {
        g_satp_supports_sv39 = false;
        LOG_I("satp does NOT support Sv39 (readback MODE=%lu)\n",
              (unsigned long)mode);
    }

    TEST_ASSERT("satp MODE readback is legal (0 or 8)",
                mode == SATP_MODE_BARE || mode == SATP_MODE_SV39);

    satp_write(saved);
    HYP_TEST_END();
}

/* ---- HGATP-PROBE-03 ---- */
TEST_REGISTER(test_shgatpa_probe_sv48);
bool test_shgatpa_probe_sv48(void) {
    TEST_BEGIN("HGATP-PROBE-03: probe satp MODE=Sv48 support");

    uintptr_t saved = satp_read();

    satp_write(MAKE_SATP(SATP_MODE_BARE, 0, 0));
    satp_write(MAKE_SATP(SATP_MODE_SV48, 0, 0));
    uintptr_t mode = SATP_GET_MODE(satp_read());

    if (mode == SATP_MODE_SV48) {
        g_satp_supports_sv48 = true;
        LOG_I("satp supports Sv48\n");
    } else {
        g_satp_supports_sv48 = false;
        LOG_I("satp does NOT support Sv48 (readback MODE=%lu)\n",
              (unsigned long)mode);
    }

    TEST_ASSERT("satp MODE readback is legal (0 or 9)",
                mode == SATP_MODE_BARE || mode == SATP_MODE_SV48);

    satp_write(saved);
    HYP_TEST_END();
}

/* ---- HGATP-PROBE-04 ---- */
TEST_REGISTER(test_shgatpa_probe_sv57);
bool test_shgatpa_probe_sv57(void) {
    TEST_BEGIN("HGATP-PROBE-04: probe satp MODE=Sv57 support");

    uintptr_t saved = satp_read();

    satp_write(MAKE_SATP(SATP_MODE_BARE, 0, 0));
    satp_write(MAKE_SATP(SATP_MODE_SV57, 0, 0));
    uintptr_t mode = SATP_GET_MODE(satp_read());

    if (mode == SATP_MODE_SV57) {
        g_satp_supports_sv57 = true;
        LOG_I("satp supports Sv57\n");
    } else {
        g_satp_supports_sv57 = false;
        LOG_I("satp does NOT support Sv57 (readback MODE=%lu)\n",
              (unsigned long)mode);
    }

    TEST_ASSERT("satp MODE readback is legal (0 or 10)",
                mode == SATP_MODE_BARE || mode == SATP_MODE_SV57);

    satp_write(saved);
    HYP_TEST_END();
}
