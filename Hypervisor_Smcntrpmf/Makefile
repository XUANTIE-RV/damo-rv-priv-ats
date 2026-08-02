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
SPIKE_ISA = rv64imach_zicsr_zifencei_smcntrpmf

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
