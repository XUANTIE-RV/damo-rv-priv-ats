# =====================================================================
# Sv48 Virtual Memory Verification Test
#
# Reuses all shared sources from the Sv39/ master directory
# (tests/*, main.c, kernel.ld) via symlinks; only TARGET and the
# -DSUITE_SATP_MODE macro below differ from Sv39/ and Sv57/.
# =====================================================================

TARGET = sv48_test.elf
ENABLE_VM = 1

# Spike ISA extensions: enable svpbmt for VM tests

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# S-stage paging mode: Sv48
CFLAGS += -DSUITE_SATP_MODE=SATP_MODE_SV48
