/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PLATFORM_SPIKE_H
#define PLATFORM_SPIKE_H

/* ===== Spike RISC-V ISA simulator platform address map ===== */

/* UART: Spike does not have an MMIO UART device.
 * Terminal output uses HTIF (Host-Target Interface) via tohost/fromhost.
 * PLATFORM_UART0_BASE is retained as a placeholder because other code
 * (e.g. page_table.c, smepmp) references it; the HTIF UART driver in
 * uart.c does not use this address. */
#define PLATFORM_UART0_BASE     0x10000000UL

/* Select HTIF-based UART driver (see uart.c UART_TYPE_SAIL) */
#define PLATFORM_UART_TYPE      1

/* Memory */
#ifndef PLATFORM_MEM_BASE
#define PLATFORM_MEM_BASE       0x80000000UL
#endif

#define PLATFORM_MEM_SIZE       0x10000000UL    /* 256 MB */

/* Spike uses tohost/fromhost for termination (no test device) */

/* Platform name */
#define CONFIG_NAME           "Spike RV64 RISC-V ISA simulator"

/* Enable double trap support for Spike (Smdbltrp/Ssdbltrp extensions).
 * Spike implements Smdbltrp which sets MDT=1 on M-mode trap entry.
 * This enables the framework's clear_mdt() and M_TRAP_EXPECT_BEGIN(). */
#define PLATFORM_DOUBLE_TRAP  1

#endif /* PLATFORM_SPIKE_H */
