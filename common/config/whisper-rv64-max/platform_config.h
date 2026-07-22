/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PLATFORM_WHISPER_RV64_MAX_H
#define PLATFORM_WHISPER_RV64_MAX_H

/* This header is included via CFLAGS -include for C code only.
 * Assembly-only constants are in rvmodel_macros.h (included via ASFLAGS). */

/* ===== Whisper RISC-V simulator platform address map ===== */

/* UART: Whisper does not have an MMIO UART device.
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

/* Platform name */
#define CONFIG_NAME           "Whisper RV64 RISC-V simulator"

/* ===== CLINT / Machine Timer addresses (for C code) =====
 * These mirror the values in rvmodel_macros.h for C code use.
 * Assembly code uses the originals in rvmodel_macros.h. */
#define PLATFORM_CLINT_BASE       0x02000000UL
#define PLATFORM_MSIP_ADDR        PLATFORM_CLINT_BASE
#define PLATFORM_MTIMECMP_ADDR    (PLATFORM_CLINT_BASE + 0x4000UL)
#define PLATFORM_MTIME_ADDR       (PLATFORM_CLINT_BASE + 0xBFF8UL)

#endif /* PLATFORM_WHISPER_RV64_MAX_H */
