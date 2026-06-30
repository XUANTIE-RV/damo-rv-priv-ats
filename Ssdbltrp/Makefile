# =====================================================================
# Ssdbltrp Extension Verification Test
# =====================================================================

TARGET = ssdbltrp_test.elf

# QEMU xt-c930v-cp CPU implements Ssdbltrp extension
QEMU_CPU   = xt-c930v-cp

SPIKE_ISA_EXT = _ssdbltrp

EXT_OBJS = \
    ssdbltrp_trap_asm.o \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
