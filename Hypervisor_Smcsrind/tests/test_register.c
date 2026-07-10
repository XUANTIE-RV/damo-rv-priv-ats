/*
 * Hypervisor_Smcsrind - Test Registration
 *
 * This file includes all test source files for the Hypervisor × Smcsrind
 * cross test suite. Tests are organized by Group as defined in
 * DOCS/testplan/Hypervisor_cross_test_plan.md Group 10.
 *
 * Group 10: Smcsrind CSRIND access control in Hypervisor scenarios
 *   Part A (01-08):  mstateen0[60] controls S-mode (HS-mode) access
 *                    to vsiselect/vsireg* (HCROSS-SMCSRIND-01~08)
 *   Part B (09-11):  hstateen0[60] controls VS-mode access to
 *                    siselect/sireg* (HCROSS-SMCSRIND-09~11)
 */

#include "test_helpers.h"

/* Group 10: Hypervisor × Smcsrind cross tests */
#include "test_hcross_smcsrind.c"
