/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HYPERVISOR_TEST_HELPERS_H
#define HYPERVISOR_TEST_HELPERS_H

/* ===================================================================
 * Shared declarations for Hypervisor Extension comprehensive tests.
 *
 * Test plan layout (see DOCS/testplan/Hypervisor_test_plan.md):
 *   Group  1  VCSR   : VS CSR substitution
 *   Group  2  HSTAT  : hstatus register
 *   Group  3  DELEG  : hedeleg/hideleg delegation
 *   Group  4  HINT   : hvip/hip/hie interrupts
 *   Group  5  HGEI   : hgeip/hgeie guest external interrupts
 *   Group  6  HENV   : henvcfg configuration
 *   Group  7  HCNT   : hcounteren counter enable
 *   Group  8  HTDL   : htimedelta time offset
 *   Group  9  VSIE   : vsip/vsie alias verification
 *   Group 10  VSTC   : vstimecmp timer
 *   Group 11  VSCR   : vsscratch/vsepc/vscause/vstval
 *   Group 12  VINST  : virtual-instruction exception
 *   Group 13  TENT   : trap entry behaviour
 *   Group 14  TRET   : trap return behaviour
 *   Group 15  TINST  : htinst/mtinst transformed instructions
 *   Group 16  MSTAT  : mstatus hypervisor enhancements
 *   Group 17  MIDLG  : mideleg/mip/mie enhancements
 *   Group 18  MTVAL  : mtval2/mtinst registers
 *   Group 19  PRIO   : exception priority
 * =================================================================== */

#include "test_framework.h"
#include "vm/vm.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_reset.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_ldst.h"
#include "hyp/hyp_fence.h"
#include "hyp/gstage_pt.h"
#include "hyp/two_stage.h"
#include "hyp/test_vs_helpers.h"
#include "hyp/hyp_trap.h"
#include "hyp/hyp_vs_trap.h"
#include "hyp/hyp_platform.h"

/* Linker-provided test-region symbols (see Hypervisor/kernel.ld). */
extern uint8_t test_data_area[];
extern uint8_t test_fault_page[];
extern uint8_t test_exec_page[];
extern uint8_t test_exec_target[];
extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];

#define TEST_REGION_BASE   ((uintptr_t)__vm_test_region_start)

/* ===================================================================
 * VS/VU-mode CSR access trampoline functions.
 *
 * These run inside VS or VU-mode via run_in_vs_mode/run_in_vu_mode.
 * Each attempts to access a specific CSR and returns the value read
 * (or 0 if it is a write-only operation). When the access is
 * illegal/virtual, the trap handler captures it.
 * =================================================================== */

/* Read sstatus (maps to vsstatus in V=1). */
uintptr_t vs_read_sstatus(uintptr_t arg);

/* Write sstatus (maps to vsstatus in V=1). */
uintptr_t vs_write_sstatus(uintptr_t val);

/* Read sie (maps to vsie in V=1). */
uintptr_t vs_read_sie(uintptr_t arg);

/* Read sip (maps to vsip in V=1). */
uintptr_t vs_read_sip(uintptr_t arg);

/* Read satp (maps to vsatp in V=1). */
uintptr_t vs_read_satp(uintptr_t arg);

/* Read stvec (maps to vstvec in V=1). */
uintptr_t vs_read_stvec(uintptr_t arg);

/* Read sscratch (maps to vsscratch in V=1). */
uintptr_t vs_read_sscratch(uintptr_t arg);

/* Read sepc (maps to vsepc in V=1). */
uintptr_t vs_read_sepc(uintptr_t arg);

/* Read scause (maps to vscause in V=1). */
uintptr_t vs_read_scause(uintptr_t arg);

/* Read stval (maps to vstval in V=1). */
uintptr_t vs_read_stval(uintptr_t arg);

/* Write satp with given value (maps to vsatp in V=1). */
uintptr_t vs_write_satp(uintptr_t val);

/* Write sie with given value (maps to vsie in V=1). */
uintptr_t vs_write_sie(uintptr_t val);

/* Write sip.SSIP (maps to vsip.SSIP in V=1). */
uintptr_t vs_write_sip_ssip(uintptr_t val);

/* Write sscratch with given value (maps to vsscratch in V=1). */
uintptr_t vs_write_sscratch(uintptr_t val);

/* Write sepc with given value (maps to vsepc in V=1). */
uintptr_t vs_write_sepc(uintptr_t val);

/* Write scause with given value (maps to vscause in V=1). */
uintptr_t vs_write_scause(uintptr_t val);

/* Write stval with given value (maps to vstval in V=1). */
uintptr_t vs_write_stval(uintptr_t val);

/* Write stvec with given value (maps to vstvec in V=1). */
uintptr_t vs_write_stvec(uintptr_t val);

/* Read senvcfg (no VS counterpart, accesses real HS CSR in V=1). */
uintptr_t vs_read_senvcfg(uintptr_t arg);

/* Write senvcfg with given value (no VS counterpart). */
uintptr_t vs_write_senvcfg(uintptr_t val);

/* Read scounteren (no VS counterpart, accesses real HS CSR in V=1). */
uintptr_t vs_read_scounteren(uintptr_t arg);

/* Write scounteren with given value (no VS counterpart). */
uintptr_t vs_write_scounteren(uintptr_t val);

/* ===================================================================
 * VS/VU-mode instruction execution trampolines.
 * =================================================================== */

/* Execute SRET in VS-mode (triggers virtual-instruction if VTSR=1). */
uintptr_t vs_exec_sret(uintptr_t arg);

/* Execute SRET in VU-mode (no sepc write, avoids double-trap). */
uintptr_t vu_exec_sret(uintptr_t arg);

/* Simple identity function safe for VU-mode (no CSR access). */
uintptr_t vu_nop(uintptr_t arg);

/* Execute WFI in VS-mode (triggers virtual-instruction if VTW=1). */
uintptr_t vs_exec_wfi(uintptr_t arg);

/* Execute SFENCE.VMA in VS-mode (triggers virtual-instruction if VTVM=1). */
uintptr_t vs_exec_sfence_vma(uintptr_t arg);

/* Execute SINVAL.VMA in VS-mode. */
uintptr_t vs_exec_sinval_vma(uintptr_t arg);

/* Execute ECALL in VS-mode (not privilege-switch). */
uintptr_t vs_exec_ecall(uintptr_t arg);

/* Execute EBREAK in VS-mode. */
uintptr_t vs_exec_ebreak(uintptr_t arg);

/* Execute an illegal instruction (UNIMP) in VS-mode. */
uintptr_t vs_exec_illegal(uintptr_t arg);

/* Simple NOP function safe for VS-mode (returns immediately). */
uintptr_t vs_nop_fn(uintptr_t arg);

/* ===================================================================
 * VS/VU-mode H-CSR access trampolines (should cause virtual-inst).
 * =================================================================== */

/* Read hstatus (CSR 0x600) — should trap in V=1. */
uintptr_t vs_read_hstatus(uintptr_t arg);

/* Read hedeleg (CSR 0x602) — should trap in V=1. */
uintptr_t vs_read_hedeleg(uintptr_t arg);

/* Read hgatp (CSR 0x680) — should trap in V=1. */
uintptr_t vs_read_hgatp(uintptr_t arg);

/* Read vsstatus directly (CSR 0x200) — should trap in V=1. */
uintptr_t vs_read_vsstatus_direct(uintptr_t arg);

/* Execute HLV.W — should trap in V=1. */
uintptr_t vs_exec_hlv_w(uintptr_t arg);

/* Execute HSV.W — should trap in V=1. */
uintptr_t vs_exec_hsv_w(uintptr_t arg);

/* Execute HLVX.WU — should trap in V=1. */
uintptr_t vs_exec_hlvx_wu(uintptr_t arg);

/* Execute HFENCE.VVMA — should trap in V=1. */
uintptr_t vs_exec_hfence_vvma(uintptr_t arg);

/* Execute HFENCE.GVMA — should trap in V=1. */
uintptr_t vs_exec_hfence_gvma(uintptr_t arg);

/* Additional H-CSR reads — should trap in V=1. */
uintptr_t vs_read_hideleg(uintptr_t arg);
uintptr_t vs_read_hcounteren(uintptr_t arg);
uintptr_t vs_read_htimedelta(uintptr_t arg);
uintptr_t vs_read_hip(uintptr_t arg);
uintptr_t vs_read_hie(uintptr_t arg);
uintptr_t vs_read_hvip(uintptr_t arg);
uintptr_t vs_read_henvcfg(uintptr_t arg);

/* H-CSR write — should trap in V=1. */
uintptr_t vs_write_hstatus(uintptr_t val);

/* HLV width variants — should trap in V=1. */
uintptr_t vs_exec_hlv_b(uintptr_t arg);
uintptr_t vs_exec_hlv_h(uintptr_t arg);
uintptr_t vs_exec_hlv_d(uintptr_t arg);

/* HLVX width variants — should trap in V=1. */
uintptr_t vs_exec_hlvx_bu(uintptr_t arg);
uintptr_t vs_exec_hlvx_hu(uintptr_t arg);

/* HSV width variants — should trap in V=1. */
uintptr_t vs_exec_hsv_b(uintptr_t arg);
uintptr_t vs_exec_hsv_h(uintptr_t arg);
uintptr_t vs_exec_hsv_d(uintptr_t arg);

/* Execute HLV.W in U-mode (V=0). Tests hstatus.HU behavior:
 * HU=0 -> illegal-inst, HU=1 -> executes HLV. */
uintptr_t u_exec_hlv_w(uintptr_t addr);

/* ===================================================================
 * Counter access trampolines (VS/VU-mode).
 * =================================================================== */

/* Read cycle counter in VS/VU-mode. */
uintptr_t vs_read_cycle(uintptr_t arg);

/* Read time counter in VS/VU-mode. */
uintptr_t vs_read_time(uintptr_t arg);

/* Read instret counter in VS/VU-mode. */
uintptr_t vs_read_instret(uintptr_t arg);

/* ===================================================================
 * Delegation configuration helpers.
 * =================================================================== */

/* Configure dual-layer delegation: medeleg + hedeleg for VS delivery. */
void setup_deleg_to_vs(uintptr_t exc_mask, uintptr_t int_mask);

/* Configure single-layer delegation: medeleg only for HS delivery. */
void setup_deleg_to_hs(uintptr_t exc_mask);

/* Clear all delegation. */
void clear_all_deleg(void);

/* ===================================================================
 * G-stage fault helpers (shared with Shtvala-style tests).
 * =================================================================== */

/* Build G-stage with victim page having specified flags. */
void setup_gstage_with_victim(two_stage_ctx_t *ctx,
                              uintptr_t victim_gpa,
                              uintptr_t victim_flags);

/* Fire a load fault via VS-mode under G-stage. */
bool fire_vs_load_fault(uintptr_t victim_gpa, uintptr_t flags);

/* Fire a store fault via VS-mode under G-stage. */
bool fire_vs_store_fault(uintptr_t victim_gpa, uintptr_t flags);

#endif /* HYPERVISOR_TEST_HELPERS_H */
