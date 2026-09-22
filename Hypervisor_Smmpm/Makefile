# =====================================================================
# Hypervisor x Smmpm Cross Test
#
# Tests for the Hypervisor extension interacting with the Smmpm
# pointer-masking extension:
#   - MPRV+MPV effective-privilege PM selection (mseccfg.PMM must
#     NOT apply to effective VS/VU/HS accesses)
# See DOCS/testplan/Hypervisor_Zpm_test_plan.md Group 7.
# =====================================================================

TARGET     = hypervisor_smmpm_test.elf
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

# Suite G-stage / VS-stage paging mode: follow the platform-selected mode
# by default; override per build with `make HGATP_MODE=sv39|sv48|sv57`
# and/or `VSATP_MODE=sv39|sv48|sv57` (see common/Makefile.common).
SUITE_VSATP_MODE ?= PLATFORM_SATP_MODE
SUITE_HGATP_MODE ?= PLATFORM_HGATP_MODE
