# =====================================================================
# Hypervisor x Smstateen Cross Verification Test
# =====================================================================
#
# Tests for the Smstateen extension in Hypervisor scenarios.
# See DOCS/testplan/Hypervisor_cross_test_plan.md Group 8 for the test plan.
# =====================================================================

TARGET = hypervisor_smstateen_test.elf

# Enable Hypervisor (H extension) support in the common build system.
# This adds -DENABLE_HYP to CFLAGS/ASFLAGS, links common/hyp/*.o,
# and appends _h to the MARCH ISA string.
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
