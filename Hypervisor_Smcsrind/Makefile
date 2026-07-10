# =====================================================================
# Hypervisor × Smcsrind Cross Test
#
# Tests for Smcsrind extension in Hypervisor scenarios:
#   Part A (01-08): mstateen0[60] (CSRIND) control over S-mode
#                   (HS-mode) access to vsiselect/vsireg*
#   Part B (09-11): hstateen0[60] (CSRIND) control over VS-mode
#                   access to siselect/sireg*
#   M-mode access is not affected by mstateen0.
#
# See DOCS/testplan/Hypervisor_cross_test_plan.md Group 10
# =====================================================================

TARGET     = hypervisor_smcsrind_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU   = max

# Spike: explicitly request RV64 + H extension + Smcsrind + Smstateen.
SPIKE_ISA = rv64imach_zicsr_zifencei_smcsrind_sscsrind_smstateen

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
