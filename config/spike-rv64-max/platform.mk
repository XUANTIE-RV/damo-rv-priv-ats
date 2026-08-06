# Spike RISC-V ISA simulator platform configuration

# Cross compiler
CROSS_COMPILER ?= riscv64-unknown-elf-

# Spike simulator configuration
SPIKE ?= spike
SPIKE_ISA ?= rv$(XLEN)imac_zicsr_zifencei

SPIKE_OPTS = --isa=$(SPIKE_ISA) $(TARGET)


# Memory base (Spike default)
MEM_BASE ?= 0x80000000
