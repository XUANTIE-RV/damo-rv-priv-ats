# =====================================================================
# Ssstateen Extension Verification Test
# =====================================================================
#
# Tests for the Ssstateen extension (Supervisor-level State Enable).
# See DOCS/testplan/ssstateen_test_plan.md for the test plan.
# =====================================================================

TARGET = ssstateen_test.elf

# Enable Hypervisor (H extension) support — required because:
#   - main.c calls hyp_reset_state() (defined in common/hyp/hyp_reset.c)
#   - tests use mstateen/sstateen CSR wrappers (defined in common/hyp/hyp_csr.c)
ENABLE_HYP = 1

# Enable VM support (needed by two_stage page table functions)
ENABLE_VM = 1

# Spike ISA extensions
SPIKE_ISA_EXT = _smstateen

# Extension-specific object files
EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
