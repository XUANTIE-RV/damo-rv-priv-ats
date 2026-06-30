/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_accessfault.c - Group 2: Access-Fault address-type exceptions
 *
 * Per Sstvala (SPEC/sstvala.adoc:4-6):
 *   For load/store/instruction access-fault, stval must contain
 *   the faulting virtual address.
 *
 * Access-faults are triggered via PMP restrictions on S-mode access.
 *
 * Tests:
 *   TVAL-LAF-01: load access-fault via PMP no-read region
 *   TVAL-LAF-02: load access-fault with different address
 *   TVAL-SAF-01: store access-fault via PMP no-write region
 *   TVAL-IAF-01: instruction access-fault via PMP no-execute region
 */

/*
 * PMP test regions: we use addresses within the platform memory range
 * that are far enough from code/data to avoid conflicts. These must
 * be valid physical memory addresses for the PMP configuration to work.
 *
 * We use test_data_area (from .vm_test_region) as the target because
 * it is valid physical memory but will be PMP-restricted.
 */

/* ===================================================================
 * TVAL-LAF-01: load access-fault via PMP no-read region
 *
 * Configure PMP entry 0 to deny read access to a specific 4KiB
 * region for S-mode. PMP entry 1 provides baseline RWX for everything
 * else. vm_run_in_smode uses PMP entry 15 (no conflict).
 * =================================================================== */
TEST_REGISTER(test_sstvala_laf_01);
bool test_sstvala_laf_01(void) {
    TEST_BEGIN("TVAL-LAF-01: load access-fault stval == faulting addr");

    uintptr_t target_addr = (uintptr_t)test_data_area;

    /* Set up PMP:
     * entry 0: target_addr NAPOT 4KiB, W-only (no R, no X)
     * entry 1: full address space NAPOT, RWX (baseline) */
    pmp_clear_all();
    pmp_entry_t e0 = PMP_ENTRY_NAPOT(target_addr, 0x1000, PMP_W);
    pmp_set_entry(0, &e0);
    pmp_entry_t e1 = PMP_ENTRY_NAPOT(0x0, 0x100000000UL, PMP_RWX);
    pmp_set_entry(1, &e1);

    /* Switch to S-mode and attempt a load */
    goto_priv(PRIV_S);
    PRIV_DO_TRAP(mem_load64(target_addr));
    goto_priv(PRIV_M);

    CHECK_TRAP("load access-fault triggered", CAUSE_LOAD_ACCESS_FAULT);
    TEST_ASSERT_EQ("stval == faulting addr",
                   trap_get_tval(), target_addr);

    pmp_clear_all();
    TEST_END();
}

/* ===================================================================
 * TVAL-LAF-02: load access-fault with a different address
 *
 * Confirms stval tracks the actual access address, not a fixed value.
 * =================================================================== */
TEST_REGISTER(test_sstvala_laf_02);
bool test_sstvala_laf_02(void) {
    TEST_BEGIN("TVAL-LAF-02: load access-fault stval tracks address");

    /* Use a different offset within the test region */
    uintptr_t target_addr = (uintptr_t)test_fault_page;

    pmp_clear_all();
    pmp_entry_t e0 = PMP_ENTRY_NAPOT(target_addr, 0x1000, PMP_W);
    pmp_set_entry(0, &e0);
    pmp_entry_t e1 = PMP_ENTRY_NAPOT(0x0, 0x100000000UL, PMP_RWX);
    pmp_set_entry(1, &e1);

    goto_priv(PRIV_S);
    PRIV_DO_TRAP(mem_load64(target_addr));
    goto_priv(PRIV_M);

    CHECK_TRAP("load access-fault triggered", CAUSE_LOAD_ACCESS_FAULT);
    TEST_ASSERT_EQ("stval == different faulting addr",
                   trap_get_tval(), target_addr);

    pmp_clear_all();
    TEST_END();
}

/* ===================================================================
 * TVAL-SAF-01: store access-fault via PMP no-write region
 * =================================================================== */
TEST_REGISTER(test_sstvala_saf_01);
bool test_sstvala_saf_01(void) {
    TEST_BEGIN("TVAL-SAF-01: store access-fault stval == faulting addr");

    uintptr_t target_addr = (uintptr_t)test_data_area;

    /* entry 0: target region R-only (no W, no X) */
    pmp_clear_all();
    pmp_entry_t e0 = PMP_ENTRY_NAPOT(target_addr, 0x1000, PMP_R);
    pmp_set_entry(0, &e0);
    pmp_entry_t e1 = PMP_ENTRY_NAPOT(0x0, 0x100000000UL, PMP_RWX);
    pmp_set_entry(1, &e1);

    goto_priv(PRIV_S);
    PRIV_DO_TRAP(mem_store64(target_addr, 0xDEADUL));
    goto_priv(PRIV_M);

    CHECK_TRAP("store access-fault triggered", CAUSE_STORE_ACCESS_FAULT);
    TEST_ASSERT_EQ("stval == faulting addr",
                   trap_get_tval(), target_addr);

    pmp_clear_all();
    TEST_END();
}

/* ===================================================================
 * TVAL-IAF-01: instruction access-fault via PMP no-execute region
 * =================================================================== */
TEST_REGISTER(test_sstvala_iaf_01);
bool test_sstvala_iaf_01(void) {
    TEST_BEGIN("TVAL-IAF-01: instruction access-fault stval == faulting addr");

    /* Fill exec page with nop;ret so it is a valid instruction stream */
    init_exec_page();
    uintptr_t target_addr = (uintptr_t)test_exec_page;

    /* entry 0: target region RW (no X) */
    pmp_clear_all();
    pmp_entry_t e0 = PMP_ENTRY_NAPOT(target_addr, 0x1000, PMP_RW);
    pmp_set_entry(0, &e0);
    pmp_entry_t e1 = PMP_ENTRY_NAPOT(0x0, 0x100000000UL, PMP_RWX);
    pmp_set_entry(1, &e1);

    goto_priv(PRIV_S);
    PRIV_DO_EXEC_TRAP(target_addr);
    goto_priv(PRIV_M);

    CHECK_TRAP("instruction access-fault triggered", CAUSE_INST_ACCESS_FAULT);
    TEST_ASSERT_EQ("stval == faulting addr",
                   trap_get_tval(), target_addr);

    pmp_clear_all();
    TEST_END();
}
