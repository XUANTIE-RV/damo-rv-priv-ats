# =====================================================================
# Hypervisor x V-vector Cross Verification Test
# =====================================================================
#
# Tests for the V vector family in Hypervisor scenarios.
# See DOCS/testplan/Hypervisor_Zi_test_plan.md Group 5:
#   - 5.1 vsstatus.vs field and vector instruction/CSR gating
#     (HVEC-01~08)
#   - 5.2 vector floating-point gating via vsstatus.fs (HVEC-09~12)
#   - 5.3 conditional case: vsstatus.vs existence with misa.v
#     writable (HVEC-13)
#
# Spec: SPEC/riscv-isa-manual/src/unpriv/vector-common.adoc
#       (norm:vsstatus_vs_sz_acc, norm:vsstatus_vs_mstatus_vs_op_off,
#        norm:vsstatus_vs_mstatus_vs_op_active,
#        norm:hw_mstatus_vs_dirty_update, norm:vsstatus_sd_op_vs,
#        norm:vsstatus_vs_exists,
#        norm:vsstatus_mstatus_FS_off_hypervisor_V_fp_ill,
#        norm:vsstatus_mstatus_FS_dirty_hypervisor_V_fp)
#
# Vector and vector-FP instructions are injected as raw encodings
# (vsetivli / vadd.vv / vfadd.vv), so the build march does not need
# the v extension. Applies to all vector extensions sharing
# vector-common.adoc (V, Zve*, Zv*).
# =====================================================================

TARGET = hypervisor_vector_test.elf

# ENABLE_HYP/ENABLE_VM: VS/VU execution framework (hyp requires VM).
ENABLE_HYP = 1
ENABLE_VM = 1

# Platform support (V/F/H) is declared by V_SUPPORTED/F_SUPPORTED/
# H_SUPPORTED in config/<platform>/rvtest_config.h; the suite gates
# on those macros and performs no runtime probing.

# QEMU virt: the max CPU model implements V (per rvtest_config.h).
QEMU_CPU = max

# Spike: canonical ISA string with V before H.
SPIKE_ISA = rv64imafdcvh_zicsr_zifencei

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
