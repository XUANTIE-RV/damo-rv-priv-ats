# =====================================================================
# Hypervisor x Sstc Cross Test
#
# Tests for Sstc extension behavior in Hypervisor scenarios:
#   - henvcfg.STCE field control (HCROSS-SSTC-01/02)
#   - VS-mode stimecmp/vstimecmp access control (HCROSS-SSTC-03/04/05)
#   - vstimecmp CSR read/write (HCROSS-SSTC-06/07/08)
#   - VSTIP synthesis logic (HCROSS-SSTC-09/10/11/12)
#   - VS-mode timer functions (HCROSS-SSTC-13/14/15)
#
# See DOCS/testplan/Hypervisor_cross_test_plan.md Group 9.
# =====================================================================

TARGET     = hypervisor_sstc_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension + Sstc.
SPIKE_ISA = rv64imach_zicsr_zifencei_sstc

EXT_OBJS = \
    main.o \
    tests/test_register.o \
    tests/sstc_strap.o

include ../common/Makefile.common
