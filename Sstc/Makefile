# =====================================================================
# Sstc Extension Verification Test
# =====================================================================
#
# Tests for the Sstc extension (Supervisor-mode Timer Interrupts, v1.0).
# See docs/sstc_test_plan.md for the test plan.
# =====================================================================

TARGET = sstc_test.elf

# Hypervisor-dependent tests are in Hypervisor_Sstc/; this suite leaves
# ENABLE_HYP undefined so it can build on RV32 platforms without H ext.
ENABLE_VM  = 1

# Spike: explicitly request RV64 + Sstc (+H preserved for compatibility) support.

# Extension-specific object files
EXT_OBJS = \
    main.o \
    tests/test_register.o \
    tests/sstc_strap.o

include ../common/Makefile.common
