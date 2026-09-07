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

# Spike: explicitly request RV64 + H extension + Smcsrind + Sscsrind + Smstateen.
# These must be in the launch ISA because rvtest_config.h declares
# SMCSRIND_SUPPORTED / SSCSRIND_SUPPORTED / SMSTATEEN_SUPPORTED; the tests
# gate miselect/mireg, vsiselect/vsireg and mstateen0/hstateen0 access on the
# compile-time *_AVAILABLE macros, so the CSRs must actually exist.
SPIKE_ISA = rv64imach_zicsr_zifencei_smcsrind_sscsrind_smstateen

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
