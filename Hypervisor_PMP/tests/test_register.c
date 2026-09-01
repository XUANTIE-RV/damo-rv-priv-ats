/*
 * Hypervisor_PMP - Test Registration
 *
 * This file includes all test source files for the Hypervisor x PMP
 * cross test suite. Tests are organized by Group as defined in
 * DOCS/testplan/Hypervisor_Sm_test_plan.md Group 5.
 *
 * Group 5: machine-level PMP behavior in Hypervisor scenarios
 *   Group 5.1 (01-04): two-stage translation succeeds, final SPA is
 *                    denied by PMP -> access fault (HCROSS-PMP-01~04)
 *   Group 5.2 (05-06): PMP constraints on implicit page-table walks
 *                    (HCROSS-PMP-05~06)
 *   Group 5.3 (07):    HFENCE.GVMA synchronization after PMP changes
 *                    (HCROSS-PMP-07)
 *   Group 5.4 (08):    HLVX cannot override PMP (HCROSS-PMP-08)
 */

#include "test_helpers.h"

/* Group 5: Hypervisor x PMP cross tests */
#include "test_hcross_pmp.c"
