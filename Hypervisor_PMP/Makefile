# =====================================================================
# Hypervisor x PMP Cross Test
#
# Tests for machine-level PMP behavior in Hypervisor scenarios:
#   Group 5.1 (01-04): two-stage translation succeeds, final SPA is
#                   denied by PMP -> access fault (norm:H_pmp)
#   Group 5.2 (05-06): PMP constraints on implicit page-table walks
#                   (norm:pmp_with_paging)
#   Group 5.3 (07):    HFENCE.GVMA synchronization after PMP changes
#                   (norm:pmp_sfence_required)
#   Group 5.4 (08):    HLVX cannot override PMP
#
# See DOCS/testplan/Hypervisor_Sm_test_plan.md Group 5
# =====================================================================

TARGET     = hypervisor_pmp_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_PMP = 1
ENABLE_TWO_STAGE = 1

QEMU_CPU   = max

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# Suite G-stage / VS-stage paging mode: follow the platform-selected mode
# by default; override per build with `make HGATP_MODE=sv39|sv48|sv57`
# and/or `VSATP_MODE=sv39|sv48|sv57` (see common/Makefile.common).
SUITE_VSATP_MODE ?= PLATFORM_SATP_MODE
SUITE_HGATP_MODE ?= PLATFORM_HGATP_MODE

# The Hypervisor x PMP cross suite configures machine-level PMP directly
# (pmpcfg/pmpaddr, and reads mseccfg for the MML gate) and owns its M-mode
# PMP setup, so it keeps the historical M-mode boot instead of the T-SBI
# default drop to S-mode (see common/entry.S: TSBI_BOOT_MODE_M).  Under the
# S(HS)-mode default the very first raw M-CSR access (csrrs pmpaddr0) traps
# illegal (scause=2) and aborts HCROSS-PMP-01.  Under this macro
# common/tsbi/tsbi_deleg.c zeroes the delegation baseline
# (medeleg/mideleg/hedeleg/hideleg), reproducing the pre-T-SBI
# all-traps-land-in-M model the statistics baseline was recorded against.
CFLAGS  += -DTSBI_BOOT_MODE_M
ASFLAGS += -DTSBI_BOOT_MODE_M
