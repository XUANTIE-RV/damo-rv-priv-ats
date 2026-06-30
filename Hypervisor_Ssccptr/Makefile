# =====================================================================
# Hypervisor × Ssccptr Cross Test
#
# Tests for the Hypervisor extension interacting with Ssccptr
# (Cacheability and Coherence PMA support for page table reads).
# See DOCS/testplan/Hypervisor_cross_test_plan.md Group 3.
# =====================================================================

TARGET     = hypervisor_ssccptr_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_TWO_STAGE = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension.
# Ssccptr is a PMA-level constraint extension, no new ISA instructions.
SPIKE_ISA = rv64imach_zicsr_zifencei

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# G-stage mode: Sv39x4
CFLAGS += -DSUITE_HGATP_MODE=HGATP_MODE_SV39X4
