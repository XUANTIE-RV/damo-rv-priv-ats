# =====================================================================
# Hypervisor x Zihpm Cross Verification Test
# =====================================================================
#
# Tests for the Zihpm (Hardware Performance Counters) extension in
# Hypervisor scenarios:
#   - Unimplemented hpmcounter behavior when V=1 (constant value or
#     exception are both legal) and its interaction with hcounteren
#     gating (HZHPM-01~04)
#   - VU-mode three-layer gating chain
#     mcounteren -> hcounteren -> scounteren for implemented
#     hpmcounters (HZHPM-05)
#
# See DOCS/testplan/Hypervisor_Zi_test_plan.md Group 7.
#
# The hcounteren/mcounteren gating matrix itself is covered by the
# Shcounterenw suite; this suite only uses gating as a precondition.
# =====================================================================

TARGET = hypervisor_zihpm_test.elf

# Enable Hypervisor (H extension) support in the common build system.
# ENABLE_VM is required because the hyp framework (two_stage/gstage_pt)
# depends on the VM page-table module.
ENABLE_HYP = 1
ENABLE_VM = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU = max

# Spike: explicitly request RV64 + H extension + Zicntr + Zihpm.
SPIKE_ISA = rv64imach_zicsr_zifencei_zicntr_zihpm

# Extension-specific object files
EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
