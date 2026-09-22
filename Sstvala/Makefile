# =====================================================================
# Sstvala Extension Verification Test
# =====================================================================
#
# Tests for the Sstvala extension (Trap Value Reporting, Version 1.0).
# See docs/sstvala_test_plan.md for the test plan.
#
# Spec summary:
#   - For address-type exceptions (page-fault, access-fault, misaligned,
#     non-EBREAK breakpoint), stval must contain the faulting virtual address.
#   - For instruction-type exceptions (illegal-instruction,
#     virtual-instruction), stval must contain the faulting instruction
#     encoding.
# =====================================================================

TARGET = sstvala_test.elf
ENABLE_VM = 1
ENABLE_PMP = 1

# Spike ISA extensions

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# Suite S-stage paging mode: follow the platform-selected mode by default;
# override per build with `make SATP_MODE=sv39|sv48|sv57` (see
# common/Makefile.common).
SUITE_SATP_MODE ?= PLATFORM_SATP_MODE
