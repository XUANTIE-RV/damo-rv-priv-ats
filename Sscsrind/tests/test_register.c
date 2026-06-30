/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_register.c - Test registration for Sscsrind test suite.
 *
 * H-extension dependent tests (Groups 2, 3, 5, and STA-05~10) have been
 * migrated to Hypervisor_Sscsrind/ as HCROSS-SSCSRIND-01~33.
 */

#include "test_helpers.h"

/* Group 1: S-mode CSR basic functionality (SSCSRIND-SCSR-01~10) */
#include "test_sscsrind_smode.c"

/* Group 4.1: State-Enable access control, S-mode only (SSCSRIND-STA-01~04) */
#include "test_sscsrind_stateen.c"
