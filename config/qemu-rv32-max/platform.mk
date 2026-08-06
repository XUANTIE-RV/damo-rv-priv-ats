# QEMU virt platform configuration (RV32)

# XLEN must be set explicitly for RV32 platforms
XLEN ?= 32

# Cross compiler
CROSS_COMPILER ?= riscv32-unknown-elf-

# QEMU configuration
QEMU = qemu-system-riscv$(XLEN)
QEMU_CPU ?= max
QEMU_MEM ?= 256M
QEMU_SMP ?= 1
QEMU_MCH ?= virt,aia=aplic-imsic
QEMU_OPTS = -machine $(QEMU_MCH) \
            -cpu $(QEMU_CPU) \
            -bios none \
            -kernel $(TARGET) \
            -m $(QEMU_MEM) \
            -smp $(QEMU_SMP) \
            -semihosting \
            -nographic

# Memory base (QEMU virt default)
MEM_BASE ?= 0x80000000
