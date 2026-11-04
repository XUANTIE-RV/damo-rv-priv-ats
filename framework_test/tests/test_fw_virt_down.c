/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 6: Virtualization downward switching (M-mode -> VS/VU-mode)
 *
 * Tests FW-VIRT-DOWN-01 through FW-VIRT-DOWN-09
 * Requires ENABLE_HYP=1
 *
 * Entering VS/VU needs no G-stage page table: with hgatp.MODE=Bare and
 * vsatp.MODE=Bare a V=1 fetch translates to the physical address as-is, so
 * the code below runs in the guest exactly as it would in S/U.
 */

#include "test_framework.h"

#ifdef ENABLE_HYP

/* ===================================================================
 * Observing the mstatus fields an mret into a V=1 mode consumed
 *
 * lower_priv() writes MPP/MPV and the mret that performs the switch
 * consumes both, so nothing at the target can read them back.  Hardware
 * re-writes them on the NEXT trap into M-mode - MPP <- nominal privilege
 * of the trapping mode, MPV <- its V bit - and m_trap_handler snapshots
 * both before it touches them (trap_get_mpp()/trap_get_mpv()).  Taking an
 * illegal-instruction trap from inside VS/VU and reading that snapshot
 * therefore observes the mode the switch actually entered.
 *
 * illegal-instruction has to leave the delegation baseline first, or the
 * trap stops at VS/HS and mstatus is never written.  reset_state() in
 * TEST_END() restores the baseline.
 *
 * The stimulus is the 16-bit reserved encoding, not `.4byte 0`: with C
 * enabled a 32-bit zero word decodes as two c.illegal, and the handler's
 * next_instruction() would advance xEPC by 2 into the second one with the
 * record already disarmed.
 * =================================================================== */
static bool fw_illegal_to_m(void)
{
    trap_expect_begin();
    asm volatile(".half 0x0000");
    trap_expect_end();
    return trap_was_triggered();
}

/* FW-VIRT-DOWN-01: M->VS switch, verify current_priv */
bool test_fw_virt_down_01(void)
{
    TEST_BEGIN("FW-VIRT-DOWN-01: M->VS current_priv verification");

    /* main() runs in S-mode, so every case in this file raises to M first.
     * Without it the descent below starts from HS and becomes S->VS, which
     * FW-VIRT-IND-01 already covers - the M->VS path would go untested. */
    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("raised to M before descending", get_current_priv(), PRIV_M);

    goto_priv(PRIV_VS);
    unsigned priv = get_current_priv();
    TEST_ASSERT_EQ("current_priv should be PRIV_VS", priv, PRIV_VS);

    TEST_END();
}

/* FW-VIRT-DOWN-02: M->VU switch, verify current_priv */
bool test_fw_virt_down_02(void)
{
    TEST_BEGIN("FW-VIRT-DOWN-02: M->VU current_priv verification");

    goto_priv(PRIV_M);

    goto_priv(PRIV_VU);
    unsigned priv = get_current_priv();
    TEST_ASSERT_EQ("current_priv should be PRIV_VU", priv, PRIV_VU);

    TEST_END();
}

/* FW-VIRT-DOWN-03: M->VS switch, verify mstatus.MPV=1 before mret */
bool test_fw_virt_down_03(void)
{
    TEST_BEGIN("FW-VIRT-DOWN-03: M->VS mstatus.MPV=1 verification");

    goto_priv(PRIV_M);
    bool routed = trap_undelegate_exc(CAUSE_ILLEGAL_INST);

    goto_priv(PRIV_VS);
    TEST_ASSERT_EQ("current_priv should be PRIV_VS", get_current_priv(), PRIV_VS);

    /* Trap back into M from VS-mode: the MPV hardware writes there is the
     * V bit the switch above put the hart in. */
    bool trapped = fw_illegal_to_m();
    bool mpv = trap_get_mpv();

    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("should return to M-mode", get_current_priv(), PRIV_M);
    TEST_ASSERT("illegal-instruction routed to M for the snapshot", routed);
    TEST_ASSERT("trap from VS-mode reached M", trapped);
    TEST_ASSERT("MPV was 1 on that trap: the mret entered V=1", mpv);

    /* Control for the observation above: the same trap taken from HS-mode
     * (V=0) has to snapshot MPV=0, otherwise "MPV was 1" could be satisfied
     * by a snapshot that never moves.  medeleg[2] is still clear. */
    goto_priv(PRIV_S);
    bool ctrl_trapped = fw_illegal_to_m();
    bool ctrl_mpv = trap_get_mpv();
    goto_priv(PRIV_M);
    TEST_ASSERT("control trap from HS-mode reached M", ctrl_trapped);
    TEST_ASSERT("control MPV was 0 for the V=0 trap", !ctrl_mpv);

    TEST_END();
}

/* FW-VIRT-DOWN-04: M->VS switch, verify mstatus.MPP=S before mret */
bool test_fw_virt_down_04(void)
{
    TEST_BEGIN("FW-VIRT-DOWN-04: M->VS mstatus.MPP=S verification");

    goto_priv(PRIV_M);
    bool routed = trap_undelegate_exc(CAUSE_ILLEGAL_INST);

    goto_priv(PRIV_VS);
    bool trapped = fw_illegal_to_m();
    uintptr_t mpp = trap_get_mpp();

    goto_priv(PRIV_M);
    TEST_ASSERT("illegal-instruction routed to M for the snapshot", routed);
    TEST_ASSERT("trap from VS-mode reached M", trapped);
    /* target & 3 == PRIV_VS & 3 == S: the nominal privilege the mret used,
     * re-observed as the MPP hardware wrote on the trap back out. */
    TEST_ASSERT_EQ("MPP was S on that trap (VS nominal privilege)", mpp,
                   (uintptr_t)PRIV_S);

    TEST_END();
}

/* FW-VIRT-DOWN-05: M->VU switch, verify mstatus.MPP=U before mret */
bool test_fw_virt_down_05(void)
{
    TEST_BEGIN("FW-VIRT-DOWN-05: M->VU mstatus.MPP=U verification");

    goto_priv(PRIV_M);
    bool routed = trap_undelegate_exc(CAUSE_ILLEGAL_INST);

    goto_priv(PRIV_VU);
    bool trapped = fw_illegal_to_m();
    uintptr_t mpp = trap_get_mpp();

    goto_priv(PRIV_M);
    TEST_ASSERT("illegal-instruction routed to M for the snapshot", routed);
    TEST_ASSERT("trap from VU-mode reached M", trapped);
    /* target & 3 == PRIV_VU & 3 == U */
    TEST_ASSERT_EQ("MPP was U on that trap (VU nominal privilege)", mpp,
                   (uintptr_t)PRIV_U);

    TEST_END();
}

/* FW-VIRT-DOWN-06: M->VU switch, verify mstatus.MPV=1 before mret */
bool test_fw_virt_down_06(void)
{
    TEST_BEGIN("FW-VIRT-DOWN-06: M->VU mstatus.MPV=1 verification");

    goto_priv(PRIV_M);
    bool routed = trap_undelegate_exc(CAUSE_ILLEGAL_INST);

    goto_priv(PRIV_VU);
    TEST_ASSERT_EQ("current_priv should be PRIV_VU", get_current_priv(), PRIV_VU);

    bool trapped = fw_illegal_to_m();
    bool mpv = trap_get_mpv();

    goto_priv(PRIV_M);
    TEST_ASSERT("illegal-instruction routed to M for the snapshot", routed);
    TEST_ASSERT("trap from VU-mode reached M", trapped);
    TEST_ASSERT("MPV was 1 on that trap: the mret entered V=1", mpv);

    TEST_END();
}

/* FW-VIRT-DOWN-07: M->VS switch, verify hstatus.SPV on the trap into HS */
bool test_fw_virt_down_07(void)
{
    TEST_BEGIN("FW-VIRT-DOWN-07: M->VS hstatus.SPV verification");

    goto_priv(PRIV_M);

    goto_priv(PRIV_VS);
    TEST_ASSERT_EQ("current_priv should be PRIV_VS", get_current_priv(), PRIV_VS);

    /* hstatus is an HS-level CSR, so reading it with V=1 raises a
     * virtual-instruction exception.  hedeleg[22] is read-only-zero, which
     * architecturally forces that trap into HS-mode - the only delivery that
     * writes hstatus.SPV, and it writes 1 because the trap came from a V=1
     * mode (norm:H_trap_hs_csrwrites).  s_trap_handler snapshots SPV/SPVP
     * (hyp_capture_s) before the sret that clears them. */
    PRIV_DO(CSRR(hstatus));
    bool trapped = trap_was_triggered();
    uintptr_t cause = trap_get_cause();
    bool spv = trap_get_spv_snap();

    goto_priv(PRIV_M);
    TEST_ASSERT_EQ("should return to M-mode", get_current_priv(), PRIV_M);
    TEST_ASSERT("hstatus read from VS-mode trapped", trapped);
    TEST_ASSERT_EQ("cause is virtual-instruction", cause,
                   (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    TEST_ASSERT("hstatus.SPV was 1: the trap came from a V=1 mode", spv);

    TEST_END();
}

/* FW-VIRT-DOWN-08: M->VS switch, verify VS-mode CSR access */
bool test_fw_virt_down_08(void)
{
    TEST_BEGIN("FW-VIRT-DOWN-08: M->VS VS-mode CSR access verification");

    goto_priv(PRIV_M);

    /* Control: a VS CSR is reachable by its own address from M-mode and
     * HS-mode only (norm:H_vscsrs_acc_m_hs). */
    PRIV_DO(CSRR(vsstatus));
    bool m_ok = !trap_was_triggered();

    goto_priv(PRIV_VS);

    /* In VS-mode the ordinary S address reaches the VS CSR: "instructions
     * that normally read or modify a supervisor CSR shall instead access
     * the corresponding VS CSR" (norm:H_vscsrs_sub). */
    PRIV_DO(CSRR(sstatus));
    bool vs_sstatus_ok = !trap_was_triggered();

    /* The VS CSR's own address must be rejected instead
     * (norm:H_vscsrs_acc_vs). */
    PRIV_DO(CSRR(vsstatus));
    bool vs_trapped = trap_was_triggered();
    uintptr_t vs_cause = trap_get_cause();

    goto_priv(PRIV_M);
    TEST_ASSERT("M-mode reads vsstatus by its own address", m_ok);
    TEST_ASSERT("VS-mode reads vsstatus through the sstatus address",
                vs_sstatus_ok);
    TEST_ASSERT("VS-mode read of vsstatus by its own address trapped",
                vs_trapped);
    TEST_ASSERT_EQ("cause is virtual-instruction", vs_cause,
                   (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);

    TEST_END();
}

/* FW-VIRT-DOWN-09: M->VU switch, verify VU-mode restrictions */
bool test_fw_virt_down_09(void)
{
    TEST_BEGIN("FW-VIRT-DOWN-09: M->VU VU-mode restriction verification");

    /* sstatus (0x100) is an implemented non-high-half supervisor CSR whose
     * read would be allowed in HS-mode, so a VU-mode read must raise a
     * virtual-instruction exception and not an illegal-instruction one
     * (norm:H_virtinst_vu_nonhigh_supervisor_allowedhs_tvm0).  The two are
     * kept strictly apart: cause 2 here would mean the implementation
     * treated the guest as plain U-mode, i.e. lost the V=1 distinction. */
    goto_priv(PRIV_M);

    goto_priv(PRIV_VU);
    PRIV_DO(CSRR(sstatus));
    goto_priv(PRIV_M);

    CHECK_TRAP("sstatus read in VU-mode traps as virtual-instruction",
               CAUSE_VIRTUAL_INSTRUCTION);

    TEST_END();
}

TEST_REGISTER(test_fw_virt_down_01);
TEST_REGISTER(test_fw_virt_down_02);
TEST_REGISTER(test_fw_virt_down_03);
TEST_REGISTER(test_fw_virt_down_04);
TEST_REGISTER(test_fw_virt_down_05);
TEST_REGISTER(test_fw_virt_down_06);
TEST_REGISTER(test_fw_virt_down_07);
TEST_REGISTER(test_fw_virt_down_08);
TEST_REGISTER(test_fw_virt_down_09);

#else /* !ENABLE_HYP */

bool test_fw_virt_down_skip(void)
{
    TEST_BEGIN("FW-VIRT-DOWN: Virtualization tests");
    TEST_SKIP("ENABLE_HYP not set");
}

TEST_REGISTER(test_fw_virt_down_skip);

#endif /* ENABLE_HYP */
