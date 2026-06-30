# =====================================================================
# Hypervisor x Svnapot Cross Test
#
# Tests for the Hypervisor extension interacting with Svnapot
# (NAPOT PTE in G-stage translation).
# See DOCS/testplan/Hypervisor_cross_test_plan.md Group 6.
# =====================================================================

TARGET     = hypervisor_svnapot_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_TWO_STAGE = 1

# QEMU virt: explicitly request RV64 + H extension + Svnapot support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension + Svnapot.
SPIKE_ISA = rv64imach_zicsr_zifencei_svnapot

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# G-stage mode: Sv39x4
CFLAGS += -DSUITE_HGATP_MODE=HGATP_MODE_SV39X4
