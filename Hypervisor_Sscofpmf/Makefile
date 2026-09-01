# =====================================================================
# Hypervisor x Sscofpmf Cross Test
#
# Tests for Sscofpmf extension behavior in Hypervisor scenarios:
#   - VS-mode scountovf dual gating by mcounteren + hcounteren
#     (HCROSS-SSCOFPMF-01/02/03, migrated from COFPMF-SOV-08/09/10)
#   - VSINH/VUINH counting inhibition in VS/VU-mode
#     (HCROSS-SSCOFPMF-04/05, new)
#   - VSINH/VUINH read-only zero when H extension is absent
#     (HCROSS-SSCOFPMF-06, new)
#
# See DOCS/testplan/Hypervisor_Ss_test_plan.md Group 10.
# =====================================================================

TARGET     = hypervisor_sscofpmf_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension + Sscofpmf.
SPIKE_ISA = rv64imach_zicsr_zifencei_sscofpmf

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
