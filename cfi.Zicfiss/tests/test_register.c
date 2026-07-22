/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Zicfiss Extension Test Registration
 *
 * All test source files are #included here so they share the same
 * translation unit and can access static helpers from test_helpers.h.
 */

#include "test_helpers.h"

/* Group 4: Zicfiss CSR control tests */
#include "test_zicfiss_csr.c"

/* Group 5: Zicfiss Shadow Stack functional tests */
#include "test_zicfiss_shadow.c"

/* Group 6: Zicfiss exception behavior tests */
#include "test_zicfiss_trap.c"
