# Sail RISC-V model platform configuration

# Cross compiler
CROSS_COMPILER ?= riscv64-unknown-elf-

# Sail simulator configuration
SAIL ?= sail_riscv_sim
SAIL_OPTS = $(TARGET)

# Memory base (Sail default)
MEM_BASE ?= 0x80000000
