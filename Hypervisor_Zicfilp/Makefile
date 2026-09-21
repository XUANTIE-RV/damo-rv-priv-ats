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

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# Suite G-stage / VS-stage paging mode: follow the platform-selected mode
# by default; override per build with `make HGATP_MODE=sv39|sv48|sv57`
# and/or `VSATP_MODE=sv39|sv48|sv57` (see common/Makefile.common).
SUITE_VSATP_MODE ?= PLATFORM_SATP_MODE
SUITE_HGATP_MODE ?= PLATFORM_HGATP_MODE
