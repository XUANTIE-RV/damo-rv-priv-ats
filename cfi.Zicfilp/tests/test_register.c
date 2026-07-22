/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Zicfilp Extension Test Registration
 *
 * All test source files are #included here so they share the same
 * translation unit and can access static helpers from test_helpers.h.
 */

#include "test_helpers.h"

/* Group 1: Zicfilp CSR control tests */
#include "test_zicfilp_csr.c"

/* Group 2: Zicfilp Landing Pad functional tests */
#include "test_zicfilp_lpad.c"

/* Group 3: Zicfilp exception behavior tests */
#include "test_zicfilp_trap.c"
