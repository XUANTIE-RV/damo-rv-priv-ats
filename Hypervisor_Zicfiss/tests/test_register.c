/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Zicfiss Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_CFI_test_plan.md Part B.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution
 * by main().
 *
 * Execution order:
 *   Group B1 (HCFI-SS-01~14)  - henvcfg.SSE enable control
 *   Group B2 (HCFI-SS-15~23)  - ssp CSR access control
 *   Group B3 (HCFI-SS-24~36)  - VS-stage SS page type
 *   Group B4 (HCFI-SS-37~42)  - G-stage translation interaction
 *   Group B5 (HCFI-SS-43~48)  - satp/vsatp Bare mode
 *   Group B6 (HCFI-SS-49~58)  - Zicfiss exception delegation
 *   Group B7 (HCFI-SS-59~68)  - Zicfiss functional completeness
 *   Group B8 (HCFI-SS-69~75)  - SSE=0 reversion behavior
 */

#include "test_helpers.h"

/* --- Group B1: henvcfg.SSE enable control (HCFI-SS-01~14) --- */
#include "test_hyp_zicfiss_envcfg.c"

/* --- Group B2: ssp CSR access control (HCFI-SS-15~23) --- */
#include "test_hyp_zicfiss_ssp_csr.c"

/* --- Group B3: VS-stage SS page type (HCFI-SS-24~36) --- */
#include "test_hyp_zicfiss_ss_page.c"

/* --- Group B4: G-stage translation interaction (HCFI-SS-37~42) --- */
#include "test_hyp_zicfiss_gstage.c"

/* --- Group B5: satp/vsatp Bare mode (HCFI-SS-43~48) --- */
#include "test_hyp_zicfiss_bare.c"

/* --- Group B6: Zicfiss exception delegation (HCFI-SS-49~58) --- */
#include "test_hyp_zicfiss_deleg.c"

/* --- Group B7: Zicfiss functional completeness (HCFI-SS-59~68) --- */
#include "test_hyp_zicfiss_func.c"

/* --- Group B8: SSE=0 reversion behavior (HCFI-SS-69~75) --- */
#include "test_hyp_zicfiss_revert.c"
