# =====================================================================
# Hypervisor x Smnpm Cross Test
#
# Tests for the Hypervisor extension interacting with the Smnpm
# pointer-masking extension:
#   - menvcfg.PMM controls HS-mode pointer masking
#   - Independence of HS/VS/VU pointer-masking settings
# See DOCS/testplan/Hypervisor_Zpm_test_plan.md Group 6.
# =====================================================================

TARGET     = hypervisor_smnpm_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_TWO_STAGE = 1
ENABLE_PM  = 1

# QEMU virt: explicitly request RV64 + H extension.
QEMU_CPU   = max

# Spike: request H + pointer-masking extensions + Smstateen.

# Spike: request H + pointer-masking extensions + Smstateen.

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# Hypervisor x Sm* cross suites manipulate machine-level CSRs directly
# (menvcfg.PMM for HS-mode, mseccfg.PMM, henvcfg PM) and own their M-mode trap
# entry, so they keep the historical M-mode boot instead of the T-SBI default
# drop to S-mode (see common/entry.S: TSBI_BOOT_MODE_M).  Under this macro
# common/tsbi/tsbi_deleg.c zeroes the delegation baseline, reproducing the
# pre-T-SBI all-traps-land-in-M model the statistics baseline was recorded
# against.
CFLAGS  += -DTSBI_BOOT_MODE_M
ASFLAGS += -DTSBI_BOOT_MODE_M

# Suite paging modes: this suite has HS/S-stage satp tests (test_hs.c) as
# well as two-stage VS/G tests, so it parameterizes all three. Follow the
# platform-selected modes by default; override per build with
# `make SATP_MODE=sv39|sv48|sv57` and/or `HGATP_MODE=`/`VSATP_MODE=`
# (see common/Makefile.common).
SUITE_SATP_MODE  ?= PLATFORM_SATP_MODE
SUITE_VSATP_MODE ?= PLATFORM_SATP_MODE
SUITE_HGATP_MODE ?= PLATFORM_HGATP_MODE
