# =====================================================================
# Hypervisor x Zaamo Cross Verification Test
# =====================================================================
#
# Tests for the Zaamo (Atomic Memory Operations) extension in Hypervisor
# scenarios, matching DOCS/testplan/Hypervisor_Za_test_plan.md Group 2
# (HZAMO-01~31): normal execution + cause=22 exclusion, store/AMO
# exception class, VS-stage/G-stage delegation, htinst transformed vs
# pseudoinstruction, misaligned AMO + MAG relaxation, FIOM/ADUE
# interaction, and the no-guest-atomic-equivalent boundary.
#
# Non-hypervisor Zaamo behavior is covered by the Zaamo/ suite.
# The base AMOs are the A extension already in the base MARCH (rv64imach
# after ENABLE_HYP), so common/mem_ops.h primitives and mnemonics work
# directly; aq/rl variants are injected as raw encodings.
# =====================================================================

TARGET     = hypervisor_zaamo_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_TWO_STAGE = 1
ENABLE_PMP = 1

QEMU_CPU   = max
# Zama16b fixes the misaligned atomicity granule at 16 bytes; HZAMO-22/23
# derive the intra-granule expectation from it. config/spike-rv64-max declares
# Zama16b, so launch Spike with it enabled to exercise the declared platform
# (mirrors Zaamo/Makefile SPIKE_ISA_EXT).

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# Suite G-stage / VS-stage paging mode: follow the platform-selected mode
# by default; override per build with `make HGATP_MODE=sv39|sv48|sv57`
# and/or `VSATP_MODE=sv39|sv48|sv57` (see common/Makefile.common).
SUITE_VSATP_MODE ?= PLATFORM_SATP_MODE
SUITE_HGATP_MODE ?= PLATFORM_HGATP_MODE
