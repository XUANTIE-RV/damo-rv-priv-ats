# =====================================================================
# Sv39/Sv48/Sv57 Virtual Memory Verification Test (Master Directory)
#
# This directory holds all shared source files for the single-stage
# Sv39/Sv48/Sv57 virtual-memory tests. Sv48/ and Sv57/ reuse these
# sources (tests/*, main.c, kernel.ld) via symlinks; each directory
# selects the S-stage paging mode at compile time through
# -DSUITE_SATP_MODE in its own Makefile -- mirroring the Sv*x4 suites.
# =====================================================================

TARGET = sv39_test.elf
ENABLE_VM = 1

# Spike ISA extensions: enable svpbmt for VM tests

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# S-stage paging mode: Sv39
CFLAGS += -DSUITE_SATP_MODE=SATP_MODE_SV39
