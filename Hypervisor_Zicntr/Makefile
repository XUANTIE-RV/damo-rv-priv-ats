# =====================================================================
# Hypervisor x Zicntr Cross Verification Test
# =====================================================================
#
# Tests for the Zicntr (Base Counters and Timers) extension in
# Hypervisor scenarios:
#   - rdtime instruction-level htimedelta semantics in VS/VU-mode
#     (HZCNT-01~06)
#   - scounteren continued control of VU-mode base counter visibility
#     when V=1 (HZCNT-07~09)
#
# See DOCS/testplan/Hypervisor_Zi_test_plan.md Group 6.
#
# The hcounteren/mcounteren gating matrix itself is covered by the
# Shcounterenw suite; this suite only uses gating as a precondition
# and verifies exception-type distinction.
# =====================================================================

TARGET = hypervisor_zicntr_test.elf

# Enable Hypervisor (H extension) support in the common build system.
# ENABLE_VM is required because the hyp framework (two_stage/gstage_pt)
# depends on the VM page-table module.
ENABLE_HYP = 1
ENABLE_VM = 1

# QEMU virt: explicitly request RV64 + H extension support.
QEMU_CPU = max

# Spike: explicitly request RV64 + H extension + Zicntr.
SPIKE_ISA = rv64imach_zicsr_zifencei_zicntr

# Extension-specific object files
EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
