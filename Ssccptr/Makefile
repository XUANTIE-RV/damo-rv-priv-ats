# =====================================================================
# Ssccptr Extension Verification Test
# =====================================================================
#
# Tests for the Ssccptr extension (Main Memory Page-Table Reads, v1.0).
# See docs/ssccptr_test_plan.md for the test plan.
# =====================================================================

TARGET = ssccptr_test.elf
ENABLE_VM = 1

# Spike ISA extensions: enable ssccptr for this test

# We need pmp_cfg.o for PMP API but NOT pmp_init.o (which references
# __pmp_test_code/__pmp_test_nomap symbols we don't define).
EXT_OBJS = \
    main.o \
    tests/test_register.o \
    ../common/pmp/pmp_cfg.o

include ../common/Makefile.common

# Suite S-stage paging mode: follow the platform-selected mode by default;
# override per build with `make SATP_MODE=sv39|sv48|sv57` (see
# common/Makefile.common).
SUITE_SATP_MODE ?= PLATFORM_SATP_MODE

# Add PMP include path for pmp_cfg.h (must be after include to append)
CFLAGS += -I../common/pmp
