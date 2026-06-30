# =====================================================================
# Shtvala Extension Compliance Test
#
# Tests for the Shtvala extension (Trap Value Reporting for Hypervisor,
# Version 1.0). See DOCS/testplan/shtvala_test_plan.md for the test plan.
#
# Spec summary:
#   On a guest-page-fault, htval MUST be written with the faulting
#   guest physical address (right-shifted by 2). The H-extension
#   baseline allows implementations to write 0; Shtvala forbids that
#   relaxation.
# =====================================================================

TARGET     = shtvala_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension support.
SPIKE_ISA = rv64imach_zicsr_zifencei


EXT_OBJS = \
    main.o \
    tests/test_helpers.o \
    tests/test_register.o

include ../common/Makefile.common
