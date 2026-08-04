# =====================================================================
# Hypervisor Exceptions Subset Test
#
# Tests for the RISC-V Hypervisor (H) Extension exception and trap behavior.
# See DOCS/testplan/Hypervisor_Exceptions_test_plan.md for the full test plan.
#
# =====================================================================

TARGET     = hypervisor_exceptions_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension support.
SPIKE_ISA = rv64imach_zicsr_zifencei_zicntr_sstc_svinval

EXT_OBJS = \
    main.o \
    ../common/hyp/hyp_test_helpers_region.o \
    tests/test_register.o

include ../common/Makefile.common
