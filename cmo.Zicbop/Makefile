# =====================================================================
# Zicbop (Cache-Block Prefetch) Extension Compliance Test
# =====================================================================
#
# Tests for the Zicbop extension (prefetch.r, prefetch.w, prefetch.i).
# See DOCS/testplan/CMO_test_plan.md for the test plan (Group 6).
# =====================================================================

TARGET = zicbop_test.elf

# Spike ISA extensions: enable Zicbop
SPIKE_ISA_EXT = _zicbop

# Enable CMO framework support
ENABLE_CMO = 1

EXT_OBJS = \
    main.o \
    tests/test_prefetch.o

include ../common/Makefile.common
