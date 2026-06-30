/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Smstateen Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_cross_test_plan.md Group 8.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   HCROSS-SMSTA-01 (INIT)     - hstateen0 reset initialization
 *   HCROSS-SMSTA-02 (PROP)     - RO0 propagation to hstateen0
 *   HCROSS-SMSTA-03~04 (B63)   - Bit 63 control behavior
 *   HCROSS-SMSTA-05~12 (FUNC)  - Functional bit control (HS-mode CSRs)
 *   HCROSS-SMSTA-13~14 (EXC)   - VS/VU-mode exception behavior
 */

#include "test_helpers.h"

/* --- HCROSS-SMSTA-01: hstateen0 initialization --- */
#include "test_hcross_smstateen_init.c"

/* --- HCROSS-SMSTA-02: RO0 propagation to hstateen0 --- */
#include "test_hcross_smstateen_prop.c"

/* --- HCROSS-SMSTA-03~04: Bit 63 control --- */
#include "test_hcross_smstateen_bit63.c"

/* --- HCROSS-SMSTA-05~12: Functional bit control (HS-mode CSRs) --- */
#include "test_hcross_smstateen_func_bits.c"

/* --- HCROSS-SMSTA-13~14: VS/VU-mode exception behavior --- */
#include "test_hcross_smstateen_exception.c"
