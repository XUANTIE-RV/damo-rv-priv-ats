/*
 * Hypervisor_Sscsrind - Test Registration
 *
 * This file includes all test source files for the Hypervisor × Sscsrind
 * cross test suite. Tests are organized by Group as defined in
 * DOCS/testplan/Hypervisor_cross_test_plan.md Group 11.
 *
 * Group 11.1: VS-level CSR basic functionality (HCROSS-SSCSRIND-01~10)
 * Group 11.2: Virtual-instruction exception behavior (HCROSS-SSCSRIND-11~21)
 * Group 11.3: State-enable access control (HCROSS-SSCSRIND-22~27)
 * Group 11.4: Hypervisor cross tests (HCROSS-SSCSRIND-28~33)
 */

#include "test_helpers.h"

/* Group 11.1: VS-level CSR basic functionality */
#include "test_hcross_sscsrind_vscsr.c"

/* Group 11.2: Virtual-instruction exception behavior */
#include "test_hcross_sscsrind_vi.c"

/* Group 11.3: State-enable access control */
#include "test_hcross_sscsrind_stateen.c"

/* Group 11.4: Hypervisor cross tests */
#include "test_hcross_sscsrind_hyp.c"
