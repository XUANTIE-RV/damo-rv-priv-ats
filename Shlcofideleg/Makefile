# =====================================================================
# Shlcofideleg Extension Compliance Test
#
# Tests for the Shlcofideleg extension (LCOFI Interrupt Delegation to
# VS-mode). See DOCS/testplan/Shlcofideleg_test_plan.md for the plan.
#
# Shlcofideleg controls whether LCOFI (Local Count Overflow Interrupt)
# can be delegated from HS-mode to VS-mode via hideleg[13].
# =====================================================================

TARGET     = shlcofideleg_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = xt-c930v-cp
QEMU_MCH   = virt,aia=none

# Spike: explicitly request RV64 + H extension support.
SPIKE_ISA = rv64imach_zicsr_zifencei

EXT_OBJS = \
    main.o \
    shlcofideleg_helpers.o \
    tests/test_register.o

include ../common/Makefile.common
