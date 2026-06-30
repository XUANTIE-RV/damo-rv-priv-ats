# =====================================================================
# Svinval Extension Verification Test
# =====================================================================

TARGET = svinval_test.elf
ENABLE_VM = 1
ENABLE_PMP = 1

# Spike ISA extensions: enable svinval for this test
SPIKE_ISA_EXT = _svinval

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
