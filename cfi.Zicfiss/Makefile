# =====================================================================
# Zicfiss (Shadow Stack) Extension Compliance Test
# =====================================================================
#
# Tests for the Zicfiss extension (backward-edge CFI / shadow stack).
# See DOCS/testplan/cfi_test_plan.md for the test plan.
# =====================================================================

ENABLE_VM = 1

TARGET = zicfiss_test.elf

# Spike ISA extensions: enable Zicfiss, Zimop, Zcmop
# Zimop/Zcmop needed for FN-03/FN-04 (32/16-bit instruction reversion
# to Zimop/Zcmop no-op behavior when SSE=0)
SPIKE_ISA_EXT = _zicfiss_zimop_zcmop

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
