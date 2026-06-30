# =====================================================================
# Sv57 Virtual Memory Verification Test
# =====================================================================

TARGET = sv57_test.elf
ENABLE_VM = 1

# Spike ISA extensions: enable svpbmt for VM tests
SPIKE_ISA_EXT = _svpbmt

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
