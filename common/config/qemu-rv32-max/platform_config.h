/*
 * QEMU virt platform configuration (RV32)
 */

#ifndef PLATFORM_QEMU_RV32_MAX_H
#define PLATFORM_QEMU_RV32_MAX_H

/* ===== QEMU virt platform address map ===== */

/* UART (NS16550 compatible) */
#define PLATFORM_UART0_BASE     0x10000000UL


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


#endif /* PLATFORM_QEMU_RV32_MAX_H */
