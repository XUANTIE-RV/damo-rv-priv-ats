/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HYPERVISOR_INTERRUPTS_TEST_HELPERS_H
#define HYPERVISOR_INTERRUPTS_TEST_HELPERS_H

#include "test_framework.h"

/* ===================================================================
 * Shared declarations for Hypervisor interrupt tests.
 *
 * VS-mode interrupt delivery fixture, shared by Group 1-2 (hvip/hip/hie
 * injection, test_interrupts.c) and Group 4 (hideleg delegation and
 * cause translation, test_deleg_interrupt.c). Both groups inject a
 * VS-level interrupt and observe it through the same VS-mode handler,
 * so the handler, its record and the setup routine live in
 * tests/test_helpers.c -- a separate object linked into every image --
 * instead of inside one group's test file.
 *
 * The record variables are deliberately NOT static: the naked handler
 * that writes them and the test cases that assert on them are compiled
 * into different objects, so a static definition in this header would
 * give each object its own copy and the assertions would read a
 * variable the handler never touched.
 * =================================================================== */

/* VS-level interrupt bit positions in hvip/hip/hie */
#define VS_VSSIP  (1UL << 2)
#define VS_VSTIP  (1UL << 6)
#define VS_VSEIP  (1UL << 10)
#define VS_ALL    (VS_VSSIP | VS_VSTIP | VS_VSEIP)

/* Recorded by vs_int_handler on the first VS-level interrupt it takes.
 * setup_vs_int_test() clears both before arming the handler. */
extern volatile uintptr_t g_vs_int_cause;
extern volatile bool g_vs_int_triggered;

/* Configure mideleg/hideleg delegation, install the VS-mode handler at
 * vstvec (direct mode) and enable vsstatus.SIE, so a VS-level interrupt
 * injected through hvip is delivered to VS-mode. Also disarms the
 * vstimecmp source so a timer-driven VSTIP cannot mix with the injected
 * one. */
extern void setup_vs_int_test(uintptr_t hideleg_mask);

#endif /* HYPERVISOR_INTERRUPTS_TEST_HELPERS_H */
