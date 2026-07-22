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
SPIKE_ISA_EXT = _ssnpm_smnpm_smmpm_smstateen

# Spike: request H + pointer-masking extensions + Smstateen.
SPIKE_ISA_EXT = _ssnpm_smnpm_smmpm_smstateen

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# G-stage mode: Sv39x4
CFLAGS += -DSUITE_HGATP_MODE=HGATP_MODE_SV39X4
