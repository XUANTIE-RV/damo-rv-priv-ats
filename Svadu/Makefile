# =====================================================================
# Svadu Extension Verification Test
# =====================================================================
#
# Tests for the Svadu extension (Hardware Update of PTE A/D bits).
# See docs/svadu_test_plan.md for the test plan.
# =====================================================================

TARGET = svadu_test.elf
ENABLE_VM = 1

# Spike ISA extensions: enable svadu for this test

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# Suite S-stage paging mode: follow the platform-selected mode by default;
# override per build with `make SATP_MODE=sv39|sv48|sv57` (see
# common/Makefile.common).
SUITE_SATP_MODE ?= PLATFORM_SATP_MODE
