/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/* ===================================================================
 * Group 6: VMID field width verification
 *
 * Spec anchors:
 *   norm:hgatp_vmid_op (hypervisor.adoc:1000-1010):
 *     hgatp VMID field is WARL, up to 14 bits (HSXLEN=64).
 *     Implementation may support fewer bits (high bits read zero).
 *
 * Test list (matches DOCS/testplan/shgatpa_test_plan.md HGATP-VMID):
 *   HGATP-VMID-01  VMID field width probe
 *   HGATP-VMID-02  VMID retention after MODE switch (observe)
 * =================================================================== */

extern volatile bool g_satp_supports_sv39;

/* ---- HGATP-VMID-01 ---- */
TEST_REGISTER(test_shgatpa_vmid_width);
bool test_shgatpa_vmid_width(void) {
    TEST_BEGIN("HGATP-VMID-01: VMID field width probe");

    if (!g_satp_supports_sv39) {
        TEST_SKIP("satp does not support Sv39, need Sv39x4 for VMID probe");
    }

    uintptr_t saved = hgatp_read();

    /* Write Sv39x4 + VMID = all 1s (14-bit max = 0x3FFF) + PPN = 0 */
    hgatp_write_raw(MAKE_HGATP(HGATP_MODE_SV39X4, 0x3FFF, 0));
    uintptr_t readback = hgatp_read();
    uintptr_t vmid = HGATP_GET_VMID(readback);

    /* VMID should be at least 1 bit */
    TEST_ASSERT("VMID field supports at least 1 bit", vmid > 0);

    /* Compute actual width from readback */
    unsigned width = 0;
    for (uintptr_t v = vmid; v & 1; v >>= 1) width++;

    LOG_I("hgatp VMID width = %u bits (readback=0x%lx)\n",
          width, (unsigned long)vmid);

    /* Cross-check with framework function */
    unsigned fw_width = hgatp_vmid_width();
    TEST_ASSERT_EQ("VMID width matches framework probe",
                   (uintptr_t)width, (uintptr_t)fw_width);

    hgatp_write_raw(saved);
    HYP_TEST_END();
}

/* ---- HGATP-VMID-02 ---- */
TEST_REGISTER(test_shgatpa_vmid_mode_switch);
bool test_shgatpa_vmid_mode_switch(void) {
    TEST_BEGIN("HGATP-VMID-02: VMID retention after MODE switch (observe)");

    if (!g_satp_supports_sv39) {
        TEST_SKIP("satp does not support Sv39, need Sv39x4 for VMID test");
    }

    uintptr_t saved = hgatp_read();

    /* Write Sv39x4 + VMID=0x1234 + PPN=0 (VMID may be truncated) */
    hgatp_write_raw(MAKE_HGATP(HGATP_MODE_SV39X4, 0x1234, 0));
    uintptr_t vmid_before = HGATP_GET_VMID(hgatp_read());
    LOG_I("VMID before switch: 0x%lx\n", (unsigned long)vmid_before);

    /* Switch to Bare */
    hgatp_write_raw(0UL);

    /* Switch back to Sv39x4 with same VMID */
    hgatp_write_raw(MAKE_HGATP(HGATP_MODE_SV39X4, 0x1234, 0));
    uintptr_t vmid_after = HGATP_GET_VMID(hgatp_read());
    LOG_I("VMID after switch: 0x%lx\n", (unsigned long)vmid_after);

    /* WARL allows the VMID to be cleared on MODE switch; we observe
     * and log but assert that the re-written value is consistent. */
    TEST_ASSERT_EQ("VMID consistent when re-written with same value",
                   vmid_after, vmid_before);

    hgatp_write_raw(saved);
    HYP_TEST_END();
}
