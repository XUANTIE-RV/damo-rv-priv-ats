# =====================================================================
# Hypervisor × Sstvala Cross Test
#
# Tests for Sstvala extension behavior in Hypervisor guest-page-fault
# scenarios. Verifies stval/vstval precision for instruction fetch,
# store, and AMO guest-page-faults.
#
# See DOCS/testplan/Hypervisor_cross_test_plan.md Group 2.
# =====================================================================

TARGET     = hypervisor_sstvala_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension + Sstvala.

EXT_OBJS = \
    main.o \
    tests/test_helpers.o \
    tests/test_register.o

include ../common/Makefile.common

# Suite G-stage / VS-stage paging mode: follow the platform-selected mode
# by default; override per build with `make HGATP_MODE=sv39|sv48|sv57`
# and/or `VSATP_MODE=sv39|sv48|sv57` (see common/Makefile.common).
SUITE_VSATP_MODE ?= PLATFORM_SATP_MODE
SUITE_HGATP_MODE ?= PLATFORM_HGATP_MODE
