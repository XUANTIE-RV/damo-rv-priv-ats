# =====================================================================
# Hypervisor x Zacas Cross Verification Test
# =====================================================================
#
# Tests for the Zacas (Atomic Compare-and-Swap) extension in Hypervisor
# scenarios, matching DOCS/testplan/Hypervisor_Za_test_plan.md Group 3
# (HZACAS-01~36): normal execution + cause=22 exclusion, store/AMO
# exception class + the unconditional write-permission requirement (a
# FAILED CAS still needs write permission), VS-stage/G-stage delegation,
# htinst transformed-vs-pseudoinstruction (funct5=00101), misaligned
# amocas + MAG, FIOM/ADUE interaction, the no-guest-atomic-equivalent
# boundary, and the hstateen0 non-gating check (HZACAS-36).
#
# Non-hypervisor Zacas behavior is covered by the Zacas/ suite.
# amocas.w/d/q use funct5=00101; amocas.q (RV64) uses register pairs.
# The assembler accepts the amocas mnemonics with _zacas in MARCH.
# =====================================================================

TARGET     = hypervisor_zacas_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_TWO_STAGE = 1
ENABLE_PMP = 1

# Zacas is not in the base MARCH; append it (plus svadu for Spike).
MARCH      = rv$(XLEN)imac_zicsr_zifencei_svadu_zacas

# Spike-only extensions (SPIKE_ISA = MARCH + SPIKE_ISA_EXT). Keep them out of
# MARCH: the assembler rejects zama16b in -march and neither is a codegen ext.
#   _zama16b   - fixes the misaligned atomicity granule at 16 bytes; HZACAS-24/25
#                derive the intra-granule expectation from it.
#   _smstateen - config/spike-rv64-max declares Smstateen (SMSTATEEN_SUPPORTED),
#                which HZACAS-36 gates on before touching mstateen0/hstateen0.

# QEMU virt: enable Zacas explicitly (max does not enable it by default).
QEMU_CPU   = max,zacas=true

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# Suite G-stage / VS-stage paging mode: follow the platform-selected mode
# by default; override per build with `make HGATP_MODE=sv39|sv48|sv57`
# and/or `VSATP_MODE=sv39|sv48|sv57` (see common/Makefile.common).
SUITE_VSATP_MODE ?= PLATFORM_SATP_MODE
SUITE_HGATP_MODE ?= PLATFORM_HGATP_MODE
