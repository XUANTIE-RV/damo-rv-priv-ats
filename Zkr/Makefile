# =====================================================================
# Zkr Entropy Source Extension Verification Test
# =====================================================================
#
# Tests for the Zkr extension (Entropy Source - seed CSR).
# See DOCS/testplan/Zkr_test_plan.md for the test plan.
# =====================================================================

TARGET = zkr_test.elf

# Spike ISA extensions
SPIKE_ISA_EXT = _zkr

# Add tests/ to include path for #include of test .c files
CFLAGS += -Itests

# Extension-specific object files
EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
