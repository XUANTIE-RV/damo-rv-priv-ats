# =====================================================================
# Hypervisor x PMP Cross Test
#
# Tests for machine-level PMP behavior in Hypervisor scenarios:
#   Group 5.1 (01-04): two-stage translation succeeds, final SPA is
#                   denied by PMP -> access fault (norm:H_pmp)
#   Group 5.2 (05-06): PMP constraints on implicit page-table walks
#                   (norm:pmp_with_paging)
#   Group 5.3 (07):    HFENCE.GVMA synchronization after PMP changes
#                   (norm:pmp_sfence_required)
#   Group 5.4 (08):    HLVX cannot override PMP
#
# See DOCS/testplan/Hypervisor_Sm_test_plan.md Group 5
# =====================================================================

TARGET     = hypervisor_pmp_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_PMP = 1
ENABLE_TWO_STAGE = 1

QEMU_CPU   = max
SPIKE_ISA  = rv64imach_zicsr_zifencei

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common
