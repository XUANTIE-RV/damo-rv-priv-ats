/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Sstvecd Extension Test Registration
 *
 * All Sstvecd test cases are organized by Group, matching
 * docs/sstvecd_test_plan.md. Each test file is #included here so the
 * TEST_REGISTER macros place function pointers into the .test_table
 * section for auto-execution by main.c.
 *
 * Execution order:
 *   Group 1 (MODE)   - stvec.MODE writability                (4 cases)
 *   Group 2 (BASE)   - stvec.BASE holding capacity           (7 cases)
 *   Group 3 (DIRECT) - MODE=Direct trap dispatch             (5 cases)
 */

#include "test_helpers.h"

/* --- Group 1: stvec.MODE writability (STVEC-MODE-01~04) --- */
#include "test_mode.c"

/* --- Group 2: stvec.BASE holding capacity (STVEC-BASE-01~07) --- */
#include "test_base.c"

/* --- Group 3: MODE=Direct trap dispatch (STVEC-DIR-01/02/03,
 *              STVEC-INT-01, STVEC-MULTI-01) --- */
#include "test_direct.c"
