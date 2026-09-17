# =====================================================================
# Hypervisor x Zalasr Cross Verification Test
# =====================================================================
#
# Tests for the Zalasr (Atomic Load-Acquire / Store-Release) extension
# in Hypervisor scenarios, matching
# DOCS/testplan/Hypervisor_Za_test_plan.md Group 5 (HZLASR-01~36):
#   - 5.1 HS/VS/VU-mode normal execution + cause=22 exclusion
#   - 5.2 exception-class split (load-acquire -> load class, only needs
#         read; store-release -> store/AMO class, needs write)
#   - 5.3 G-stage faults forced to HS-mode + trap context
#   - 5.4 htinst transformed atomic (unified opcode 0x2F format, keeps
#         funct5=00110/00111 to distinguish load-acquire/store-release)
#   - 5.5 misaligned load-acquire/store-release + MAG relaxation
#   - 5.6 henvcfg.FIOM / ADUE interaction (Zalasr always carries aq/rl)
#   - 5.7 reserved encodings (load w/o aq, store w/o rl) -> cause=2
#   - 5.8 architectural boundary (no atomic-ordered guest load/store
#         equivalent, but HLV/HSV can replicate the data transfer)
#
# Non-hypervisor Zalasr behavior is covered by the Zalasr/ suite.
#
# The toolchain does NOT recognise "zalasr" as an -march extension and
# has no lb.aq / sd.rl mnemonics, so MARCH is left without zalasr and
# every Zalasr instruction is injected as a raw encoding with the
# ".insn r" pseudo-op (opcode 0x2f, funct3 = width,
# funct7 = funct5<<2 | aq<<1 | rl). See tests/test_helpers.h.
#
# The simulators DO implement Zalasr, so it must be enabled on the
# simulator command line even though the assembler cannot name it.
# Zama16b is enabled alongside it because config/<platform> declares
# ZAMA16B_SUPPORTED and HZLASR-27 derives the intra-granule relaxation
# from the 16-byte granule that norm:zama16b_mag fixes:
#   QEMU  : -cpu max,zalasr=true,zama16b=true
#   Spike : --isa=...(_zalasr_zama16b)
# =====================================================================

TARGET     = hypervisor_zalasr_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_TWO_STAGE = 1
ENABLE_PMP = 1

# Zalasr is injected as raw encodings, so it is NOT added to MARCH; svadu
# is appended for the Spike ADUE cases (HZLASR-23/33).
MARCH      = rv$(XLEN)imac_zicsr_zifencei_svadu

# Spike: MARCH is passed to spike --isa directly; append the Zalasr and
# Zama16b suffixes so spike decodes the raw-encoded load-acquire /
# store-release and models the declared misaligned atomicity granule.

# QEMU virt: enable Zalasr and Zama16b explicitly (max does not enable
# zalasr by default).
QEMU_CPU   = max,zalasr=true,zama16b=true

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

CFLAGS += -DSUITE_VSATP_MODE=SATP_MODE_SV39
CFLAGS += -DSUITE_HGATP_MODE=HGATP_MODE_SV39X4
