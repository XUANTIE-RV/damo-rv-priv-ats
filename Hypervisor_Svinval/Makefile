# =====================================================================
# Hypervisor × Svinval Cross Test
#
# Tests for the Hypervisor extension interacting with Svinval
# (Fine-grained address-translation cache invalidation).
# See DOCS/testplan/Hypervisor_cross_test_plan.md Group 5.
# =====================================================================

TARGET     = hypervisor_svinval_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_TWO_STAGE = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension + Svinval.
SPIKE_ISA = rv64imach_zicsr_zifencei_svinval

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# G-stage mode: Sv39x4
CFLAGS += -DSUITE_HGATP_MODE=HGATP_MODE_SV39X4
