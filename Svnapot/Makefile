# =====================================================================
# Svnapot Extension Verification Test
# =====================================================================

TARGET = svnapot_test.elf
ENABLE_VM = 1

# Spike ISA extensions: enable svnapot and svpbmt for this test
SPIKE_ISA_EXT = _svnapot_svpbmt

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
