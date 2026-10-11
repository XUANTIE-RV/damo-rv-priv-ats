# =====================================================================
# Hypervisor × Smcsrind Cross Test
#
# Tests for Smcsrind extension in Hypervisor scenarios:
#   Group 1.1 (01-08): mstateen0[60] (CSRIND) control over S-mode
#                   (HS-mode) access to vsiselect/vsireg*
#   Group 1.2 (09-11): hstateen0[60] (CSRIND) control over VS-mode
#                   access to siselect/sireg*
#   M-mode access is not affected by mstateen0.
#
# See DOCS/testplan/Hypervisor_Sm_test_plan.md Group 1
# =====================================================================

TARGET     = hypervisor_smcsrind_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension + Smcsrind + Smstateen.

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# Hypervisor x Sm* cross suites manipulate machine-level CSRs directly
# (mstateen0 CSRIND gating of vsiselect/vsireg*, miselect/mireg) and own their
# M-mode trap entry, so they keep the historical M-mode boot instead of the
# T-SBI default drop to S-mode (see common/entry.S: TSBI_BOOT_MODE_M).  Under
# this macro common/tsbi/tsbi_deleg.c zeroes the delegation baseline,
# reproducing the pre-T-SBI all-traps-land-in-M model the statistics baseline
# was recorded against.
CFLAGS  += -DTSBI_BOOT_MODE_M
ASFLAGS += -DTSBI_BOOT_MODE_M
