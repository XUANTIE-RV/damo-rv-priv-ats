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

# Extension-specific object files
EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# Hypervisor x Sm* cross suites manipulate machine-level CSRs directly
# (mstateen0/hstateen0 gating, pmpcfg) and own their M-mode trap entry, so they
# keep the historical M-mode boot instead of the T-SBI default drop to S-mode
# (see common/entry.S: TSBI_BOOT_MODE_M).  Under this macro
# common/tsbi/tsbi_deleg.c zeroes the delegation baseline, reproducing the
# pre-T-SBI all-traps-land-in-M model the statistics baseline was recorded
# against.
CFLAGS  += -DTSBI_BOOT_MODE_M
ASFLAGS += -DTSBI_BOOT_MODE_M
