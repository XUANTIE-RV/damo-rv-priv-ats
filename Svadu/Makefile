# =====================================================================
# Svadu Extension Verification Test
# =====================================================================
#
# Tests for the Svadu extension (Hardware Update of PTE A/D bits).
# See docs/svadu_test_plan.md for the test plan.
# =====================================================================

TARGET = svadu_test.elf
ENABLE_VM = 1

# Spike ISA extensions: enable svadu for this test
SPIKE_ISA_EXT = _svadu

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
