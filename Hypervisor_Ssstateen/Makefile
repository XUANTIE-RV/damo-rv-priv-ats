# =====================================================================
# Hypervisor x Ssstateen Cross Verification Test
# =====================================================================
#
# Tests for the Ssstateen extension in Hypervisor scenarios.
# See DOCS/testplan/Hypervisor_cross_test_plan.md Group 8 for the test plan.
#
# These tests were extracted from Ssstateen/ Groups 7-12 (H-mode tests)
# and renumbered to HCROSS-SSSTA-01~50.
# =====================================================================

TARGET = hypervisor_ssstateen_test.elf

# Enable Hypervisor (H extension) support in the common build system.
ENABLE_HYP = 1
ENABLE_VM = 1
ENABLE_TWO_STAGE = 1

# Spike ISA extensions
SPIKE_ISA = rv64imach_zicsr_zifencei_smstateen

# Extension-specific object files
EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
