# =====================================================================
# Zicboz (Cache-Block Zero) Extension Compliance Test
# =====================================================================
#
# Tests for the Zicboz extension (cbo.zero).
# See DOCS/testplan/CMO_test_plan.md for the test plan (Groups 3, 5).
# =====================================================================

TARGET = zicboz_test.elf

# Spike ISA extensions: enable Zicboz
SPIKE_ISA_EXT = _zicboz

# Enable CMO framework support
ENABLE_CMO = 1

EXT_OBJS = \
    main.o \
    tests/test_cbze.o \
    tests/test_func.o

include ../common/Makefile.common
