/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * csr_accessors.h - runtime-address CSR dispatcher
 *
 * csr_read()/csr_write() access a CSR whose number is only known at
 * RUNTIME. RISC-V CSR instructions encode the register number as a
 * compile-time immediate, so the implementation (csr_accessors.c) maps
 * each supported address to a fixed csrr/csrw via a switch-case.
 *
 * Use this ONLY when the CSR number is a variable (loops over a range,
 * table-driven tests, generic WARL probes). For a fixed, known CSR
 * prefer the named accessors in csr_ops.h (single instruction, no
 * dispatch, no coverage gap): an address absent from the switch table
 * silently no-ops.
 */

#ifndef COMMON_CSR_ACCESSORS_H
#define COMMON_CSR_ACCESSORS_H

#include "types.h"

uintptr_t csr_read(uint16_t csr);
void      csr_write(uint16_t csr, uintptr_t val);

#endif /* COMMON_CSR_ACCESSORS_H */
