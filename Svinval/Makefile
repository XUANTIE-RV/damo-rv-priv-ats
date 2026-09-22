# =====================================================================
# Svinval Extension Verification Test
# =====================================================================

TARGET = svinval_test.elf
ENABLE_VM = 1
ENABLE_PMP = 1

# Spike ISA extensions: enable svinval for this test

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# Suite S-stage paging mode: follow the platform-selected mode by default;
# override per build with `make SATP_MODE=sv39|sv48|sv57` (see
# common/Makefile.common).
SUITE_SATP_MODE ?= PLATFORM_SATP_MODE
