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

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# Suite G-stage / VS-stage paging mode: follow the platform-selected mode
# by default; override per build with `make HGATP_MODE=sv39|sv48|sv57`
# and/or `VSATP_MODE=sv39|sv48|sv57` (see common/Makefile.common).
SUITE_VSATP_MODE ?= PLATFORM_SATP_MODE
SUITE_HGATP_MODE ?= PLATFORM_HGATP_MODE
