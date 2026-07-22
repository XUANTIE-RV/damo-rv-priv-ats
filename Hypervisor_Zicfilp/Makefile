# =====================================================================
# Hypervisor x Zicfilp Cross Test
#
# Tests for the Hypervisor (H) extension interacting with Zicfilp
# (Forward-edge CFI / Landing Pad).
# See DOCS/testplan/Hypervisor_CFI_test_plan.md Part A.
# =====================================================================

TARGET     = hypervisor_zicfilp_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_TWO_STAGE = 1

# QEMU virt: explicitly request RV64 + H extension + Zicfilp support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension + Zicfilp.
SPIKE_ISA = rv64imach_zicsr_zifencei_zicntr_sstc_svinval_zicfilp

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# G-stage mode: Sv39x4
CFLAGS += -DSUITE_HGATP_MODE=HGATP_MODE_SV39X4
