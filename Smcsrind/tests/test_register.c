/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * test_register.c - Test registration for Smcsrind test suite.
 *
 * All test files are #included here to form a single compilation unit.
 */

#include "test_helpers.h"

/* Group 1: M-mode CSR existence and accessibility */
#include "test_mcsrind_exist.c"

/* Group 2: miselect register behavior */
#include "test_mcsrind_miselect.c"

/* Group 3: mireg* indirect access mechanism */
#include "test_mcsrind_mireg.c"

/* Group 4: Smstateen access control */
#include "test_mcsrind_stateen.c"
