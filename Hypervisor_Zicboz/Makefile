# =====================================================================
# Hypervisor x Zicboz Cross Test
#
# Tests for the Hypervisor extension interacting with Zicboz
# (cbo.zero in VS/VU-mode).
# See DOCS/testplan/Hypervisor_CMO_test_plan.md Groups 3, 4, 5, 6.
# =====================================================================

TARGET     = hypervisor_zicboz_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_TWO_STAGE = 1
ENABLE_CMO = 1

# QEMU virt: explicitly request RV64 + H extension + Zicboz.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension + Zicboz.
SPIKE_ISA_EXT = _zicboz

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# G-stage mode: Sv39x4
CFLAGS += -DSUITE_HGATP_MODE=HGATP_MODE_SV39X4
