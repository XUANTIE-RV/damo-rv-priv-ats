# =====================================================================
# Hypervisor × Svadu Cross Test
#
# Tests for the Hypervisor extension interacting with Svadu
# (Hardware Update of PTE A/D bits in two-stage translation).
# See DOCS/testplan/Hypervisor_cross_test_plan.md Group 1.
# =====================================================================

TARGET     = hypervisor_svadu_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_TWO_STAGE = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension + Svadu.
SPIKE_ISA = rv64imach_zicsr_zifencei_svadu

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# G-stage mode: Sv39x4
CFLAGS += -DSUITE_HGATP_MODE=HGATP_MODE_SV39X4
