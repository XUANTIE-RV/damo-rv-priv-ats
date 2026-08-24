/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * ===================================================================
 * common/act4/act4_config.h — build-time settings for the ACT4 trap
 *                             handler
 *
 * Included from both C and assembly. ACT4_TRAP_HANDLER itself comes
 * from the build system (TRAP_HANDLER=act4).
 * ===================================================================
 */

#ifndef ACT4_CONFIG_H
#define ACT4_CONFIG_H

/* Assertion mode: the handler records the trapping state for the test
 * suite to assert on and writes no signature. This is how the suite
 * runs. Clearing it (RVTEST_SIGNATURE_MODE=1) selects stock ACT4
 * signature behaviour, which does not link yet — see the note in
 * common/Makefile.common. */
#if defined(ACT4_TRAP_HANDLER) && !defined(RVTEST_SIGNATURE_MODE)
  #ifndef RVTEST_ASSERTION_MODE
    #define RVTEST_ASSERTION_MODE 1
  #endif
#endif

/* Signature region size, in words. Unused while assertion mode is on;
 * sized for 64 six-word entries. */
#ifndef ACT4_SIG_REGION_WORDS
  #define ACT4_SIG_REGION_WORDS 384
#endif

/* ACT4's utils.h uses TEST_FLEN to pick the floating-point store width
 * in its signature macros, and refuses to compile without it. This
 * suite builds with no F, D or Q in its -march and the trap handler
 * emits no floating-point instructions, so derive it from what the DUT
 * advertises, which is enough to satisfy the CONFIG_FLEN <= TEST_FLEN
 * check in utils.h. */
#ifndef TEST_FLEN
  #if defined(Q_SUPPORTED)
    #define TEST_FLEN 128
  #elif defined(D_SUPPORTED)
    #define TEST_FLEN 64
  #else
    #define TEST_FLEN 32
  #endif
#endif

#endif /* ACT4_CONFIG_H */
