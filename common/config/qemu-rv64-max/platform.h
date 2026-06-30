/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PMP_PLATFORM_QEMU_VIRT_H
#define PMP_PLATFORM_QEMU_VIRT_H

/* ===== QEMU virt platform address map ===== */

/* UART (NS16550 compatible) */
#define PLATFORM_UART0_BASE     0x10000000UL

/* CLINT (Core Local Interruptor) */
#define PLATFORM_CLINT_BASE     0x02000000UL

/* Memory */
#ifndef PLATFORM_MEM_BASE
#define PLATFORM_MEM_BASE       0x80000000UL
#endif

#define PLATFORM_MEM_SIZE       0x10000000UL    /* 256 MB */

/* QEMU test device address (used by PMP rules that need to cover the
 * finisher region; the actual halt behavior is in rvmodel_macros.h) */
#define PLATFORM_TEST_DEVICE            0x100000UL

/* Platform name */
#define CONFIG_NAME           "QEMU RV64 MAX"


/* Some Case will crash qemu，mark those test as SKIP */
#define QEMU_SKIP_TESTS  1

/* QEMU xt-c930v-cp implements Smdbltrp (Double Trap) extension.
 * mstatus.MDT (bit 42) is set to 1 on reset and on M-mode trap entry.
 * Must clear MDT before any M-mode operation that may trigger a trap,
 * otherwise the CPU raises a fatal double trap. */
#define PLATFORM_DOUBLE_TRAP  1

#endif /* PMP_PLATFORM_QEMU_VIRT_H */
