# =====================================================================
# Sstvecd Extension Verification Test
# =====================================================================
#
# Tests for the Sstvecd extension (Direct Trap Vectoring, v1.0).
# See docs/sstvecd_test_plan.md for the test plan.
#
# Spec summary:
#   - stvec.MODE must be capable of holding 0 (Direct).
#   - When stvec.MODE=Direct, stvec.BASE must be capable of holding any
#     valid four-byte-aligned address.
# =====================================================================

TARGET = sstvecd_test.elf
ENABLE_VM = 1

# Spike ISA extensions: enable sstvecd for this test.
# NOTE: If the local Spike build does not recognize "_sstvecd" and
#       startup fails, fall back to leaving this empty -- most of the
#       Sstvecd-mandated WARL behaviors (Direct + 4-byte aligned BASE)
#       are also satisfied by stock RISC-V implementations; only the
#       high-bit BASE holding tests (STVEC-BASE-04 / STVEC-BASE-07)
#       may differ.
SPIKE_ISA_EXT = _sstvecd

EXT_OBJS = \
    main.o \
    tests/test_register.o \
    tests/sstvecd_strap.o

include ../common/Makefile.common
