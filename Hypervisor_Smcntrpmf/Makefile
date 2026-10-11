# =====================================================================
# Hypervisor x Smcntrpmf Cross Test
#
# Tests for Smcntrpmf extension behavior in Hypervisor scenarios:
#   - VSINH/VUINH read-only zero without H ext (PMF-CSR-05)
#   - VSINH/VUINH inhibit VS/VU-mode cycle counting (PMF-CYC-08/09)
#   - VSINH/VUINH inhibit VS/VU-mode instret counting (PMF-INS-06/07)
#   - hcounteren.CY control of VS-mode cycle access (PMF-CTR-04)
#   - VSINH inhibition orthogonal to hcounteren (HCROSS-PMF-01)
#
# See DOCS/testplan/Hypervisor_cross_test_plan.md Group 16.
# =====================================================================

TARGET     = hypervisor_smcntrpmf_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension + Smcntrpmf.

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# Hypervisor x Sm* cross suites manipulate machine-level CSRs directly
# (mcounteren/mcountinhibit, mseccfg/menvcfg PM, mstatus.MPRV/MPV, mstateen0,
# pmpcfg) and own their M-mode trap entry, so they keep the historical M-mode
# boot instead of the T-SBI default drop to S-mode (see common/entry.S:
# TSBI_BOOT_MODE_M).  Under this macro common/tsbi/tsbi_deleg.c zeroes the
# delegation baseline, reproducing the pre-T-SBI all-traps-land-in-M model the
# statistics baseline was recorded against.
CFLAGS  += -DTSBI_BOOT_MODE_M
ASFLAGS += -DTSBI_BOOT_MODE_M
