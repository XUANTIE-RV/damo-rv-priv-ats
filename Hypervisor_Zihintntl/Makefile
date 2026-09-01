# =====================================================================
# Hypervisor x Zihintntl Cross Verification Test
# =====================================================================
#
# Tests for the Zihintntl (Non-Temporal Locality Hints) extension in
# Hypervisor scenarios.
# See DOCS/testplan/Hypervisor_Zi_test_plan.md Group 2:
#   - 2.1 NTL + load normal execution in HS/VS/VU modes (NTL-HYP-01~03)
#   - 2.2 NTL applied to HLV/HSV/HLVX in HS-mode      (NTL-HYP-04)
#   - 2.3 NTL + CMO virtual-instruction report in VS  (NTL-HYP-05)
#   - 2.4 NTL + load G-stage guest-page-fault in VS   (NTL-HYP-06)
#
# Non-hypervisor Zihintntl behavior is out of scope (see
# DOCS/testplan/zihintntl_test_plan.md); the hypervisor cross cases
# live here.
#
# NTL instructions are injected as raw encodings (ADD with rd=x0):
#   ntl.p1=0x00200033, ntl.pall=0x00300033, ntl.s1=0x00400033,
#   ntl.all=0x00500033. They are legal HINT encodings in the base ISA,
#   so no probe is required for the hints themselves.
# =====================================================================

TARGET = hypervisor_zihintntl_test.elf

# ENABLE_VM/ENABLE_HYP: VS/VU execution framework.
# ENABLE_TWO_STAGE: NTL-HYP-06 needs VS-valid + G-invalid mappings.
# ENABLE_CMO: NTL-HYP-05 uses the CBO_INVAL/CBO_CLEAN macros.
ENABLE_VM = 1
ENABLE_HYP = 1
ENABLE_TWO_STAGE = 1
ENABLE_CMO = 1

# QEMU virt: the max CPU model includes H and Zicbom.
QEMU_CPU = max

# Spike: add Zicbom for the CMO cross case (NTL-HYP-05).
SPIKE_ISA_EXT = _zicbom

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# G-stage mode for the two-stage cases: Sv39x4.
CFLAGS += -DSUITE_HGATP_MODE=HGATP_MODE_SV39X4
