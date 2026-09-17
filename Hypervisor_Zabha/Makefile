# =====================================================================
# Hypervisor x Zabha Cross Verification Test
# =====================================================================
#
# Tests for the Zabha (Byte and Halfword Atomic Memory Operations)
# extension in Hypervisor scenarios, matching
# DOCS/testplan/Hypervisor_Za_test_plan.md Group 4 (HZABHA-01~39):
#   - 4.1 HS/VS/VU-mode normal execution + cause=22 exclusion
#   - 4.2 store/AMO exception class + width-independent permission
#   - 4.3 G-stage faults forced to HS-mode + trap context
#   - 4.4 htinst transformed atomic (funct3 width retention, Zabha-specific)
#   - 4.5 byte AMO never misaligned; half AMO misaligned/MAG
#   - 4.6 henvcfg.FIOM / ADUE interaction
#   - 4.7 amocas.b/h (Zabha x Zacas, conditional on Zacas)
#   - 4.8 reserved byte/half lr/sc -> illegal-instruction (cause=2)
#   - 4.9 architectural boundary (no byte/half guest-atomic equivalent)
#
# Non-hypervisor Zabha behavior is covered by the Zabha/ suite.
# Byte/half AMOs use funct3=000/001; the assembler accepts the amo*.b/h
# mnemonics with _zabha in MARCH (and amocas.b/h with _zacas).
# =====================================================================

TARGET     = hypervisor_zabha_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_TWO_STAGE = 1
ENABLE_PMP = 1

# Zabha (and Zacas, for amocas.b/h) are not in the base MARCH; append them
# (plus svadu for Spike).
MARCH      = rv$(XLEN)imac_zicsr_zifencei_svadu_zabha_zacas

# Zama16b fixes the misaligned atomicity granule at 16 bytes; HZABHA-24/25
# derive the intra-granule halfword-AMO expectation from it.
# config/spike-rv64-max declares Zama16b, so launch Spike with it enabled
# (mirrors Zaamo/Makefile SPIKE_ISA_EXT).

# QEMU virt: enable Zabha and Zacas explicitly.
QEMU_CPU   = max,zabha=true,zacas=true

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

CFLAGS += -DSUITE_VSATP_MODE=SATP_MODE_SV39
CFLAGS += -DSUITE_HGATP_MODE=HGATP_MODE_SV39X4
