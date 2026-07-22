# =====================================================================
# Zicbom (Cache-Block Management) Extension Compliance Test
# =====================================================================
#
# Tests for the Zicbom extension (cbo.inval, cbo.clean, cbo.flush).
# See DOCS/testplan/CMO_test_plan.md for the test plan (Groups 1, 2, 4).
# =====================================================================

TARGET = zicbom_test.elf

# Spike ISA extensions: enable Zicbom
SPIKE_ISA_EXT = _zicbom

# Enable CMO framework support
ENABLE_CMO = 1

EXT_OBJS = \
    main.o \
    tests/test_cbie.o \
    tests/test_cbcfe.o \
    tests/test_func.o

include ../common/Makefile.common
