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
SPIKE_ISA = rv64imach_zicsr_zifencei_zicntr_sstc

EXT_OBJS = \
    main.o \
    tests/test_helpers.o \
    tests/test_register.o

include ../common/Makefile.common
