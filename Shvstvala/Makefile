# =====================================================================
# Shvstvala Extension Compliance Test
#
# Tests for the Shvstvala extension (Trap Value Reporting for VS-mode,
# Version 1.0). See DOCS/testplan/shvstvala_test_plan.md for the test plan.
#
# Spec summary:
#   vstval must be written with the faulting virtual address (for address
#   exceptions) or the faulting instruction encoding (for illegal-instruction)
#   when a trap enters VS-mode, matching Sstvala's stval behavior.
# =====================================================================

TARGET     = shvstvala_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_PMP = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension support.
SPIKE_ISA = rv64imach_zicsr_zifencei

EXT_OBJS = \
    main.o \
    tests/shvstvala_strap.o \
    tests/test_register.o

include ../common/Makefile.common
