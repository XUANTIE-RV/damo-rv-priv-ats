/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Smcntrpmf Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_cross_test_plan.md Group 16.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution
 * by main().
 *
 * Execution order:
 *   PMF-CSR-05  - VSINH/VUINH read-only zero without H ext
 *   PMF-CYC-08/09 - VSINH/VUINH inhibit VS/VU-mode cycle counting
 *   PMF-INS-06/07 - VSINH/VUINH inhibit VS/VU-mode instret counting
 *   PMF-CTR-04  - hcounteren.CY=0, VS-mode cannot read cycle
 *   HCROSS-PMF-01 - VSINH inhibition orthogonal to hcounteren
 */

#include "test_helpers.h"

/* PMF-CSR-05: VSINH/VUINH read-only zero without H ext */
#include "test_csr_rw.c"

/* PMF-CYC-08/09: VSINH/VUINH inhibit VS/VU-mode cycle counting */
#include "test_cycle_filter.c"

/* PMF-INS-06/07: VSINH/VUINH inhibit VS/VU-mode instret counting */
#include "test_instret_filter.c"

/* PMF-CTR-04 + HCROSS-PMF-01: hcounteren interaction */
#include "test_counteren.c"
