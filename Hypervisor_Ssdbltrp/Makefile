# =====================================================================
# Hypervisor x Ssdbltrp Cross Test
#
# Tests for Ssdbltrp extension behavior in Hypervisor scenarios:
#   - henvcfg.DTE field control (HCROSS-SSDBLTRP-01~06)
#   - vsstatus.SDT field behavior (HCROSS-SSDBLTRP-07~12)
#   - SRET clears vsstatus.SDT (HCROSS-SSDBLTRP-13~16)
#   - menvcfg.DTE control (HCROSS-SSDBLTRP-17~18)
#   - MRET/SRET/MNRET cross-mode clear (HCROSS-SSDBLTRP-19~24)
#
# See DOCS/testplan/Hypervisor_cross_test_plan.md Group 12.
# =====================================================================

TARGET     = hypervisor_ssdbltrp_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1

# QEMU xt-c930v-cp CPU implements Ssdbltrp extension
QEMU_CPU   = xt-c930v-cp

# Spike: explicitly request RV64 + H extension + Ssdbltrp.
SPIKE_ISA = rv64imach_zicsr_zifencei_ssdbltrp

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
