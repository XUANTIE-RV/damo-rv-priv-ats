/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HYPERVISOR_SSTVALA_TEST_HELPERS_H
#define HYPERVISOR_SSTVALA_TEST_HELPERS_H

/* ===================================================================
 * Shared declarations for Hypervisor × Sstvala cross tests.
 *
 * Test plan layout (see DOCS/testplan/Hypervisor_cross_test_plan.md):
 *   HCROSS-SSTVALA-01: Instruction guest-page-fault stval precision
 *   HCROSS-SSTVALA-02: Store guest-page-fault stval precision
 *   HCROSS-SSTVALA-03: AMO guest-page-fault stval precision
 *   HCROSS-SSTVALA-04: Instruction guest-page-fault vstval (delegated)
 *   HCROSS-SSTVALA-05: Store guest-page-fault vstval (delegated)
 *   HCROSS-SSTVALA-06: Virtual-instruction stval == encoding (read hstatus)
 *   HCROSS-SSTVALA-07: Virtual-instruction stval == encoding (write hgatp)
 *   HCROSS-SSTVALA-08: Virtual-instruction stval == encoding (read hideleg)
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

/* Linker-provided test-region symbols (see kernel.ld). */
extern uint8_t test_data_area[];
extern uint8_t test_fault_page[];
extern uint8_t test_exec_page[];
extern uint8_t test_exec_target[];
extern uint8_t __vm_test_region_start[];
extern uint8_t __vm_test_region_end[];

#define TEST_REGION_BASE   ((uintptr_t)__vm_test_region_start)

/* ===================================================================
 * VS-mode payload functions for Sstvala tests.
 *
 * These run inside VS-mode via two_stage_run_in_vs(). They perform
 * specific memory operations that trigger guest-page-faults, allowing
 * verification of stval/vstval precision.
 * =================================================================== */

/* Jump to address arg and execute (triggers inst guest-page-fault if
 * G-stage unmapped). Returns trap cause or 0 on success. */
uintptr_t test_vs_jump_to(uintptr_t arg);

/* Store to address arg (triggers store guest-page-fault if G-stage
 * unmapped). Returns trap cause or 0 on success. */
uintptr_t test_vs_store_to(uintptr_t arg);

/* AMO operation on address arg (triggers store guest-page-fault if
 * G-stage unmapped). Returns trap cause or 0 on success. */
uintptr_t test_vs_amo_to(uintptr_t arg);

/* VS-mode payload functions for virtual-instruction stval tests
 * (HCROSS-SSTVALA-06~08). Each executes a pre-encoded HS-level CSR
 * access instruction that triggers a virtual-instruction exception
 * (cause=22) in VS-mode. M-mode handler captures stval. */
uintptr_t test_vs_vi_read_hstatus(uintptr_t arg);
uintptr_t test_vs_vi_write_hgatp(uintptr_t arg);
uintptr_t test_vs_vi_read_hideleg(uintptr_t arg);

/* ===================================================================
 * VS-mode trap handler support for delegated fault tests.
 *
 * For HCROSS-SSTVALA-04/05, guest-page-faults are delegated to VS-mode
 * via hedeleg. The VS-mode trap handler records vscause/vstval into
 * global variables, then returns to HS-mode via ecall.
 * =================================================================== */

/* Global variables populated by VS-mode trap handler */
extern uintptr_t g_vs_trap_vstval;
extern uintptr_t g_vs_trap_cause;
extern bool      g_vs_trap_triggered;

/* Configure VS-mode trap handler for delegated guest-page-fault tests.
 * Sets up vstvec and clears the global trap record. */
void setup_vs_trap_handler_for_sstvala(void);

/* VS-mode payload: jump to arg, fault delegated to VS handler.
 * After VS handler runs, returns to HS via ecall. */
uintptr_t test_vs_jump_delegated(uintptr_t arg);

/* VS-mode payload: store to arg, fault delegated to VS handler. */
uintptr_t test_vs_store_delegated(uintptr_t arg);

#endif /* HYPERVISOR_SSTVALA_TEST_HELPERS_H */
