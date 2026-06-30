/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_register.c - Test registration for Ssdbltrp test suite
 */

#include "test_helpers.h"

/* Group 1: sstatus.SDT field tests */
#include "test_sdt_field.c"

/* Group 5: SDT/SIE mutex tests */
#include "test_sdt_sie_mutex.c"

/* Group 2: S-mode trap delivery tests */
#include "test_strap.c"

/* Group 3: SRET clears SDT tests */
#include "test_sret_sdt.c"

/* Group 4: menvcfg.DTE control tests */
#include "test_menvcfg_dte.c"

/* Group 7: MRET/SRET/MNRET clear SDT tests */
#include "test_xret_sdt.c"

/* Group 8: medeleg[16] read-only zero tests */
#include "test_medeleg16.c"

/* Group 9: mtval2 dependency tests */
#include "test_mtval2_dep.c"
