/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * test_register.c - Hypervisor Extension Comprehensive Test Registration
 *
 * All test cases are organized by Group, matching
 * DOCS/testplan/Hypervisor_test_plan.md.
 *
 * Each test file is #included here so TEST_REGISTER macros place
 * function pointers into the .test_table section for auto-execution
 * by main().
 *
 * Execution order:
 *   Group  1  VCSR   - VS CSR substitution
 *   Group  2  HSTAT  - hstatus register
 *   Group  3  DELEG  - hedeleg/hideleg delegation
 *   Group  4  HINT   - hvip/hip/hie interrupts
 *   Group  5  HGEI   - hgeip/hgeie (included in test_interrupts.c)
 *   Group  6  HENV   - henvcfg configuration
 *   Group  7  HTDL   - htimedelta time offset
 *   Group  8  VSST   - vsstatus register
 *   Group  9  VSIE   - vsip/vsie alias verification
 *   Group 10  VSTC   - vstimecmp timer
 *   Group 11  VSCR   - vsscratch/vsepc/vscause/vstval
 *   Group 12  VINST  - virtual-instruction exception
 *   Group 13  TENT   - trap entry behaviour
 *   Group 14  TRET   - trap return behaviour
 *   Group 15  TINST  - htinst/mtinst transformed instructions
 *   Group 16  MSTAT  - mstatus hypervisor enhancements
 *   Group 17  MIDLG  - mideleg/mip/mie enhancements
 *   Group 18  MTVAL  - mtval2/mtinst registers
 *   Group 19  PRIO   - exception priority
 */

#include "test_helpers.h"

/* Group  1 */ #include "test_csr_basics.c"
/* Group  2 */ #include "test_hstatus.c"
/* Group  3 */ #include "test_delegation.c"
/* Group 4-5*/ #include "test_interrupts.c"
/* Group  6 */ #include "test_henvcfg.c"
/* Group  7 */ #include "test_htimedelta.c"
/* Group  8 */ #include "test_vsstatus.c"
/* Group  9 */ #include "test_vsip_vsie.c"
/* Group 10 */ #include "test_vstimecmp.c"
/* Group 11 */ #include "test_vs_scratch.c"
/* Group 12 */ #include "test_virtual_inst.c"
/* Group 13 */ #include "test_trap_entry.c"
/* Group 14 */ #include "test_trap_return.c"
/* Group 15 */ #include "test_htinst.c"
/* Group 16 */ #include "test_mstatus_hyp.c"
/* Group 17 */ #include "test_mideleg_enhance.c"
/* Group 18 */ #include "test_mtval2.c"
/* Group 19 */ #include "test_exception_priority.c"
