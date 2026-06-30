/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Shlcofideleg Test Registration File
 *
 * All test cases are organized by Group, matching Shlcofideleg_test_plan.md.
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 1 (WR)    - hideleg[13] writability
 *   Group 2 (RO)    - hideleg[13]=0 read-only zero
 *   Group 3 (ALIAS) - hideleg[13]=1 alias verification
 *   Group 4 (VS)    - VS-mode end-to-end
 *   Group 5 (DYN)   - hideleg dynamic toggle consistency
 */

#include "shlcofideleg_helpers.h"

/* --- Diagnostic: Counter overflow mechanism --- */
#include "test_diag_overflow.c"

/* --- Group 1: hideleg[13] Writability (LCFIDLG-WR-01 ~ WR-02) --- */
#include "test_hideleg_writable.c"

/* --- Group 2: Read-only Zero when hideleg[13]=0 (LCFIDLG-RO-01 ~ RO-06) --- */
#include "test_ro_zero.c"

/* --- Group 3: Alias Verification when hideleg[13]=1 (LCFIDLG-ALIAS-01 ~ ALIAS-07) --- */
#include "test_alias.c"

/* --- Group 4: VS-mode End-to-End (LCFIDLG-VS-01 ~ VS-06) --- */
#include "test_vs_mode.c"

/* --- Group 5: Dynamic Toggle Consistency (LCFIDLG-DYN-01 ~ DYN-04) --- */
#include "test_dynamic_toggle.c"
