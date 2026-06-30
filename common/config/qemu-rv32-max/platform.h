/*
 * QEMU virt platform configuration (RV32)
 */

#ifndef PMP_PLATFORM_QEMU_VIRT_RV32_H
#define PMP_PLATFORM_QEMU_VIRT_RV32_H

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

/* QEMU test device address */
#define PLATFORM_TEST_DEVICE            0x100000UL

/* Platform name */
#define CONFIG_NAME           "QEMU RV32 MAX"

/* Some Case will crash qemu, mark those test as SKIP */
#define QEMU_SKIP_TESTS  1

/* QEMU RV32 MAX CPU implements Smdbltrp (Double Trap) extension. */
#define PLATFORM_DOUBLE_TRAP  1

#endif /* PMP_PLATFORM_QEMU_VIRT_RV32_H */
