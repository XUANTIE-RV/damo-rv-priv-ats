# =====================================================================
# Hypervisor × Sscsrind Cross Test
#
# Tests for Sscsrind extension behavior in Hypervisor scenarios:
#   - VS-level CSR (vsiselect/vsireg*) basic functionality
#   - Virtual-instruction exception behavior
#   - hstateen0[60] state-enable control
#   - VS-mode transparent remapping via sireg*
#
# See DOCS/testplan/Hypervisor_cross_test_plan.md Group 11
# =====================================================================

TARGET     = hypervisor_sscsrind_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension + Sscsrind.
SPIKE_ISA = rv64imach_zicsr_zifencei_sscsrind

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
