# =====================================================================
# Sstc Extension Verification Test
# =====================================================================
#
# Tests for the Sstc extension (Supervisor-mode Timer Interrupts, v1.0).
# See docs/sstc_test_plan.md for the test plan.
# =====================================================================

TARGET = sstc_test.elf

# Enable Hypervisor support for Group 6 (VS-mode) tests.
# Runtime detection via misa.H skips VS tests if H ext unavailable.
ENABLE_VM  = 1
ENABLE_HYP = 1

# Spike: explicitly request RV64 + H extension support.
SPIKE_ISA = rv64imach_zicsr_zifencei_sstc

# Extension-specific object files
EXT_OBJS = \
    main.o \
    tests/test_register.o \
    tests/sstc_strap.o

include ../common/Makefile.common
