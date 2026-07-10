# =====================================================================
# Shvstvecd Extension Compliance Test
#
# Tests for the Shvstvecd extension (Direct Trap Vectoring for VS-mode,
# Version 1.0). See DOCS/testplan/shvstvecd_test_plan.md for the test plan.
#
# Spec summary:
#   - vstvec.MODE must be capable of holding 0 (Direct).
#   - When vstvec.MODE=Direct, vstvec.BASE must be capable of holding any
#     valid four-byte-aligned address.
# =====================================================================

TARGET     = shvstvecd_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_TWO_STAGE = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension support.
SPIKE_ISA = rv64imach_zicsr_zifencei_zicntr_smstateen_sstc

EXT_OBJS = \
    main.o \
    tests/test_register.o \
    tests/shvstvecd_strap.o

include ../common/Makefile.common
