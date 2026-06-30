/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Svadu Extension Test Registration
 *
 * All test cases are organized by Group, matching docs/svadu_test_plan.md.
 * Each test file is #included here so TEST_REGISTER macros place function
 * pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 1 (CSR-ADUE)      - menvcfg.ADUE field control
 *   Group 2 (A-BIT 4K)      - ADUE=1 HW updates PTE.A on 4 KiB pages
 *   Group 3 (D-BIT 4K)      - ADUE=1 HW updates PTE.D on 4 KiB pages
 *   Group 4 (2M megapage)   - ADUE=1 HW updates A/D on 2 MiB megapages
 *   Group 5 (1G gigapage)   - ADUE=1 HW updates A/D on 1 GiB gigapages
 *   Group 6 (FALLBACK)      - ADUE=0 falls back to Svade behavior
 *   Group 7 (RUNTIME SW)    - Runtime switching of ADUE
 */

#include "test_helpers.h"

/* Snapshot of menvcfg populated by main.c before any test runs. */
uintptr_t g_menvcfg_reset_value = 0;

/* --- Group 1: menvcfg.ADUE CSR control (SVADU-CSR-01~04) --- */
#include "test_csr_adue.c"

/* --- Group 2: ADUE=1 HW updates PTE.A on 4 KiB pages (SVADU-A4K-01~04) --- */
#include "test_a_bit_4k.c"

/* --- Group 3: ADUE=1 HW updates PTE.D on 4 KiB pages (SVADU-D4K-01~04) --- */
#include "test_d_bit_4k.c"

/* --- Group 4: ADUE=1 on 2 MiB megapages (SVADU-2M-01~05) --- */
#include "test_2m_megapage.c"

/* --- Group 5: ADUE=1 on 1 GiB gigapages (SVADU-1G-01~04) --- */
#include "test_1g_gigapage.c"

/* --- Group 6: ADUE=0 falls back to Svade (SVADU-FB-01~05) --- */
#include "test_fallback_svade.c"

/* --- Group 7: Runtime switching of ADUE (SVADU-SW-01~04) --- */
#include "test_runtime_switch.c"
