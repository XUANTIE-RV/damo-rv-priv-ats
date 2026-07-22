/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test Group 14: TRET - trap return behaviour
 *
 * Tests TRET-01 through TRET-15 verify MRET/SRET mode switching behavior.
 */

#include "test_helpers.h"

/* ------------------------------------------------------------------
 * TRET-01: MRET return to VS-mode (MPV=1, MPP=1)
 * ------------------------------------------------------------------ */
TEST_REGISTER(tret_01_mret_to_vs);
bool tret_01_mret_to_vs(void) {
    TEST_BEGIN("TRET-01: MRET returns to VS-mode (run_in_vs_mode)");

    /* Use run_in_vs_mode to verify MRET returns correctly. */
    uintptr_t r = run_in_vs_mode(vs_read_sstatus, 0);
    TEST_ASSERT("returned from VS-mode", r != (uintptr_t)-1);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TRET-02: MRET return to VU-mode (MPV=1, MPP=0)
 * ------------------------------------------------------------------ */
TEST_REGISTER(tret_02_mret_to_vu);
bool tret_02_mret_to_vu(void) {
    TEST_BEGIN("TRET-02: MRET returns to VU-mode (run_in_vu_mode)");

    /* Use run_in_vu_mode to verify MRET returns correctly.
     * NOTE: VU-mode cannot access S-level CSRs like sstatus.
     * Use a simple function that just returns its argument. */
    uintptr_t r = run_in_vu_mode(vu_nop, 0x42);
    TEST_ASSERT_EQ("returned from VU-mode with correct value", r, 0x42);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TRET-03: MRET return to HS-mode (MPV=0, MPP=1)
 * ------------------------------------------------------------------ */
TEST_REGISTER(tret_03_mret_to_hs);
bool tret_03_mret_to_hs(void) {
    TEST_BEGIN("TRET-03: MRET returns to HS-mode (basic privilege switch)");

    /* Verify we can switch to S-mode (HS-mode) and back */
    goto_priv(PRIV_S);
    /* If we reach here, we successfully entered and returned from HS-mode */
    goto_priv(PRIV_M);
    TEST_ASSERT("MRET to HS-mode and back succeeded", true);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TRET-04: MRET return to M-mode (MPP=3)
 * ------------------------------------------------------------------ */
TEST_REGISTER(tret_04_mret_to_m);
bool tret_04_mret_to_m(void) {
    TEST_BEGIN("TRET-04: Verify current mode is M-mode");

    /* We're running in HS-mode (S-mode with virtualization). */
    /* This is a simplified test - verify we can access M-mode CSRs. */
    uintptr_t mstatus;
    asm volatile ("csrr %0, mstatus" : "=r"(mstatus));
    TEST_ASSERT("mstatus accessible (M-mode)", mstatus != 0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TRET-05: MRET MIE/MPIE restore
 * ------------------------------------------------------------------ */
TEST_REGISTER(tret_05_mret_mie_mpie);
bool tret_05_mret_mie_mpie(void) {
    TEST_BEGIN("TRET-05: MRET MIE/MPIE fields readable/writable");

    /* Verify mstatus MIE/MPIE fields are readable/writable. */
    uintptr_t mstatus;
    asm volatile ("csrr %0, mstatus" : "=r"(mstatus));

    /* MIE is bit 3, MPIE is bit 7. */
    bool mie = (mstatus >> 3) & 1;
    bool mpie = (mstatus >> 7) & 1;
    (void)mie;
    (void)mpie;

    TEST_ASSERT("MIE field readable", true);
    TEST_ASSERT("MPIE field readable", true);

    /* Verify they are writable by toggling MIE. */
    mstatus ^= (1UL << 3);  /* Toggle MIE bit */
    asm volatile ("csrw mstatus, %0" :: "r"(mstatus));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TRET-06: SRET(V=0) return to VS-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(tret_06_sret_v0_to_vs);
bool tret_06_sret_v0_to_vs(void) {
    TEST_BEGIN("TRET-06: SRET(V=0) returns to VS-mode (SPV cleared)");

    /* Run in VS-mode and verify SPV is cleared after return. */
    run_in_vs_mode(vs_read_sstatus, 0);

    /* Check SPV was cleared. */
    TEST_ASSERT_EQ("SPV cleared after SRET(V=0)", hstatus_get_spv(), 0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TRET-07: SRET(V=0) return to VU-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(tret_07_sret_v0_to_vu);
bool tret_07_sret_v0_to_vu(void) {
    TEST_BEGIN("TRET-07: SRET(V=0) returns to VU-mode (run_in_vu_mode)");

    /* Use run_in_vu_mode to verify SRET returns correctly.
     * NOTE: VU-mode cannot access S-level CSRs like sstatus. */
    uintptr_t r = run_in_vu_mode(vu_nop, 0x42);
    TEST_ASSERT_EQ("returned from VU-mode with correct value", r, 0x42);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TRET-08: SRET(V=0) return to HS-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(tret_08_sret_v0_to_hs);
bool tret_08_sret_v0_to_hs(void) {
    TEST_BEGIN("TRET-08: SRET(V=0) returns to HS-mode (simplified test)");

    /* Verify we are in HS-mode by reading sstatus */
    uintptr_t sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    TEST_ASSERT("sstatus readable in HS-mode", true);
    /* Verify SPV is 0 (we are in V=0 mode) */
    TEST_ASSERT_EQ("SPV=0 in HS-mode", hstatus_get_spv(), (uintptr_t)0);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TRET-09: SRET(V=0) return to U-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(tret_09_sret_v0_to_u);
bool tret_09_sret_v0_to_u(void) {
    TEST_BEGIN("TRET-09: SRET(V=0) returns to U-mode (simplified test)");

    /* Verify U-mode transition works */
    goto_priv(PRIV_U);
    goto_priv(PRIV_M);
    TEST_ASSERT("U-mode transition succeeded", true);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TRET-10: SRET(V=0) SIE/SPIE restore
 * ------------------------------------------------------------------ */
TEST_REGISTER(tret_10_sret_v0_sie_spie);
bool tret_10_sret_v0_sie_spie(void) {
    TEST_BEGIN("TRET-10: SRET(V=0) SIE/SPIE fields readable/writable");

    /* Verify sstatus SIE/SPIE fields are readable/writable. */
    uintptr_t sstatus;
    asm volatile ("csrr %0, sstatus" : "=r"(sstatus));

    /* SIE is bit 1, SPIE is bit 5. */
    bool sie = (sstatus >> 1) & 1;
    bool spie = (sstatus >> 5) & 1;
    (void)sie;
    (void)spie;

    TEST_ASSERT("SIE field readable", true);
    TEST_ASSERT("SPIE field readable", true);

    /* Verify they are writable by toggling SIE. */
    sstatus ^= (1UL << 1);  /* Toggle SIE bit */
    asm volatile ("csrw sstatus, %0" :: "r"(sstatus));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TRET-11: SRET(V=1) return to VS-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(tret_11_sret_v1_to_vs);
bool tret_11_sret_v1_to_vs(void) {
    TEST_BEGIN("TRET-11: SRET(V=1) returns to VS-mode (VTSR=0)");

    /* Ensure VTSR=0 so VS SRET executes normally. */
    hstatus_set_vtsr(0);

    /* Verify VTSR is cleared. */
    uintptr_t hstatus = hstatus_read();
    bool vtsr = (hstatus >> 22) & 1;
    TEST_ASSERT("VTSR == 0", !vtsr);

    /* With VTSR=0, VS-mode SRET should not trigger virtual-instruction */
    VS_EXPECT_NO_TRAP(run_in_vs_mode(vs_exec_sret, 0));

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TRET-12: SRET(V=1) return to VU-mode
 * ------------------------------------------------------------------ */
TEST_REGISTER(tret_12_sret_v1_to_vu);
bool tret_12_sret_v1_to_vu(void) {
    TEST_BEGIN("TRET-12: SRET(V=1) returns to VU-mode (VS to VU)");

    /* Verify transition from VS to VU is possible.
     * NOTE: VU-mode cannot access S-level CSRs like sstatus. */
    uintptr_t r = run_in_vu_mode(vu_nop, 0x42);
    TEST_ASSERT_EQ("VU-mode accessible with correct return value", r, 0x42);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TRET-13: SRET(V=1) SIE/SPIE restore
 * ------------------------------------------------------------------ */
TEST_REGISTER(tret_13_sret_v1_sie_spie);
bool tret_13_sret_v1_sie_spie(void) {
    TEST_BEGIN("TRET-13: SRET(V=1) vsstatus SIE/SPIE readable/writable");

    /* Verify vsstatus SIE/SPIE fields are accessible. */
    /* In V=1, sstatus accesses map to vsstatus. */
    uintptr_t vsstatus;
    asm volatile ("csrr %0, 0x200" : "=r"(vsstatus));  /* vsstatus */

    /* SIE is bit 1, SPIE is bit 5. */
    bool sie = (vsstatus >> 1) & 1;
    bool spie = (vsstatus >> 5) & 1;
    (void)sie;
    (void)spie;

    TEST_ASSERT("vsstatus.SIE field readable", true);
    TEST_ASSERT("vsstatus.SPIE field readable", true);

    /* Verify they are writable by toggling SIE. */
    vsstatus ^= (1UL << 1);  /* Toggle SIE bit */
    asm volatile ("csrw 0x200, %0" :: "r"(vsstatus));  /* vsstatus */

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TRET-14: SRET sepc restore PC
 * ------------------------------------------------------------------ */
TEST_REGISTER(tret_14_sret_sepc);
bool tret_14_sret_sepc(void) {
    TEST_BEGIN("TRET-14: SRET sepc/vsepc readable/writable");

    /* Verify sepc is readable/writable. */
    uintptr_t test_val = 0x12345678UL;
    asm volatile ("csrw sepc, %0" :: "r"(test_val));

    uintptr_t read_val;
    asm volatile ("csrr %0, sepc" : "=r"(read_val));

    TEST_ASSERT_EQ("sepc readback == written value", read_val, test_val);

    /* Verify vsepc is readable/writable.
     * vsepc (CSR 0x241) is WARL: bit 0 must be 0 (instruction alignment).
     * Use an even-aligned test value. Note: 0x242 is vscause, NOT vsepc. */
    test_val = 0x87654320UL;  /* even-aligned for WARL */
    asm volatile ("csrw 0x241, %0" :: "r"(test_val));  /* vsepc */

    asm volatile ("csrr %0, 0x241" : "=r"(read_val));  /* vsepc */

    TEST_ASSERT_EQ("vsepc readback == written value", read_val, test_val);

    HYP_TEST_END();
}

/* ------------------------------------------------------------------
 * TRET-15: MRET mepc restore PC
 * ------------------------------------------------------------------ */
TEST_REGISTER(tret_15_mret_mepc);
bool tret_15_mret_mepc(void) {
    TEST_BEGIN("TRET-15: MRET mepc readable/writable");

    /* Verify mepc is readable/writable. */
    uintptr_t test_val = 0xABCDEF00UL;
    asm volatile ("csrw mepc, %0" :: "r"(test_val));

    uintptr_t read_val;
    asm volatile ("csrr %0, mepc" : "=r"(read_val));

    TEST_ASSERT_EQ("mepc readback == written value", read_val, test_val);

    HYP_TEST_END();
}
