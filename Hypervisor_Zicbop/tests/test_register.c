/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Zicbop Cross Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_CMO_test_plan.md.
 *
 * Execution order:
 *   Group 7 (HPREFETCH) - Prefetch VS/VU-mode behavior
 */

#include "test_helpers.h"

/* --- Group 7: Zicbop Prefetch VS/VU-mode (HPREFETCH-01~08) --- */
#include "test_hprefetch.c"
