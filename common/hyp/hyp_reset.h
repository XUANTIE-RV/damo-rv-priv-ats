/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HYP_RESET_H
#define HYP_RESET_H

/* ===================================================================
 * Hypervisor state reset for tests
 *
 * hyp_reset_state() returns ALL hypervisor state to a known clean
 * baseline (hgatp=Bare, hedeleg=0, hideleg=0, hvip=0, hie=0,
 * hstatus=0, menvcfg.ADUE=0 (force Svade G-stage A/D semantics),
 * henvcfg=0, hcounteren=0, htimedelta=0, all VS CSRs=0,
 * HFENCE.GVMA all) and then forwards to the base reset_state().
 *
 * Tests use HYP_TEST_END (defined in hyp_test.h) which calls this
 * helper instead of reset_state(), so each hypervisor-aware test
 * starts from a fully clean H-extension state.
 * =================================================================== */

void hyp_reset_state(void);

#endif /* HYP_RESET_H */
