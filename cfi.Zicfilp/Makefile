# =====================================================================
# Zicfilp (Landing Pad) Extension Compliance Test
# =====================================================================
#
# Tests for the Zicfilp extension (forward-edge CFI / landing pads).
# See DOCS/testplan/cfi_test_plan.md for the test plan.
# =====================================================================

TARGET = zicfilp_test.elf

# Spike ISA extensions: enable Zicfilp
SPIKE_ISA_EXT = _zicfilp

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
