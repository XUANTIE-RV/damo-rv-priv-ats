/*
 * Hypervisor_Smcsrind - Test Registration
 *
 * This file includes all test source files for the Hypervisor × Smcsrind
 * cross test suite. Tests are organized by Group as defined in
 * DOCS/testplan/Hypervisor_Sm_test_plan.md Group 1.
 *
 * Group 1: Smcsrind CSRIND access control in Hypervisor scenarios
 *   Group 1.1 (01-08):  mstateen0[60] controls S-mode (HS-mode) access
 *                    to vsiselect/vsireg* (HCROSS-SMCSRIND-01~08)
 *   Group 1.2 (09-11):  hstateen0[60] controls VS-mode access to
 *                    siselect/sireg* (HCROSS-SMCSRIND-09~11)
 */

#include "test_helpers.h"

/* Group 1: Hypervisor × Smcsrind cross tests */
#include "test_hcross_smcsrind.c"
