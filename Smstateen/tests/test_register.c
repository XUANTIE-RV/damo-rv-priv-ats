/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Smstateen Extension Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/smstateen_test_plan.md.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 1 (EXIST)     - mstateen CSR existence and accessibility
 *   Group 2 (INIT)      - Reset initialization behavior
 *   Group 3 (WARL)      - WARL and read-only zero behavior
 *   Group 4 (PROP)      - Read-only zero propagation to lower privilege
 *   Group 5 (B63)       - Bit 63 control behavior
 *   Group 6 (FUNC)      - Functional bit control (11 sub-groups)
 *   Group 7 (EXC)       - Access control exception behavior
 */

#include "test_helpers.h"

/* --- Group 1: mstateen CSR existence (MSTA-EXIST-01~05) --- */
#include "test_exist.c"

/* --- Group 2: Reset initialization (MSTA-INIT-01~06) --- */
#include "test_init.c"

/* --- Group 3: WARL and RO0 behavior (MSTA-WARL-01~06) --- */
#include "test_warl.c"

/* --- Group 4: RO0 propagation (MSTA-PROP-01~04) --- */
#include "test_prop.c"

/* --- Group 5: Bit 63 control (MSTA-B63-01~07) --- */
#include "test_bit63.c"

/* --- Group 6: Functional bit control (MSTA-C/FCSR/JVT/ENVCFG/...) --- */
#include "test_func_bits.c"

/* --- Group 7: Exception behavior (MSTA-EXC-01~06) --- */
#include "test_exception.c"
