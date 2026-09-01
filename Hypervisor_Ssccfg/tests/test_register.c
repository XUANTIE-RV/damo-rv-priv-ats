/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor x Ssccfg Test Registration File
 *
 * All test cases are organized by Group, matching
 * Hypervisor_Ss_test_plan.md Group 11 (Hypervisor x Smcdeleg/Ssccfg).
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution.
 *
 * Execution order:
 *   Group 11.1/11.2 (HCROSS-SSCCFG-01 ~ 06) - scountovf/scountinhibit
 *   Group 11.3 (HCROSS-SSCCFG-07 ~ 11) - LCOFI virtualization
 *   Group 11.4 (HCROSS-SSCCFG-12 ~ 21) - vsiselect/vsireg* rules
 *   Group 11.5 (HCROSS-SSCCFG-22 ~ 24) - hstateen0 bit 60 gating
 */

#include "test_helpers.h"

/* --- Group 11.1/11.2: scountovf/scountinhibit virtualization --- */
#include "test_counter_virt.c"

/* --- Group 11.3: LCOFI virtualization (hvip/hvien bit 13) --- */
#include "test_lcofi_hyp.c"

/* --- Group 11.4: vsiselect/vsireg* multi-privilege access rules --- */
#include "test_vsireg_rules.c"

/* --- Group 11.5: hstateen0 bit 60 gating (Ssccfg perspective) --- */
#include "test_hstateen_gate.c"
