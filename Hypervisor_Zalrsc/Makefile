# =====================================================================
# Hypervisor x Zalrsc Cross Verification Test
# =====================================================================
#
# Tests for the Zalrsc (Load-Reserved / Store-Conditional) extension in
# Hypervisor scenarios, matching
# DOCS/testplan/Hypervisor_Za_test_plan.md Group 1 (HZLRSC-01~40):
#   - 1.1 HS/VS/VU-mode normal execution + cause=22 exclusion
#   - 1.2 exception-class split (LR->load, SC->store/AMO) + VS-stage
#         delegation path
#   - 1.3 G-stage faults forced to HS-mode + trap context (GVA/SPV/htval)
#   - 1.4 htinst: transformed atomic instruction vs pseudoinstruction
#   - 1.5 SC retire permission check (failed SC still store-class fault)
#   - 1.6 misaligned LR/SC exception path (no MAG relaxation)
#   - 1.7 henvcfg.FIOM / ADUE interaction
#   - 1.8 constrained-loop forward progress + virtualization trap
#   - 1.9 architectural boundary (no HLR/HSC guest-atomic equivalent)
#
# Non-hypervisor Zalrsc behavior is covered by the Zalrsc/ suite
# (DOCS/testplan/Zalrsc_test_plan.md).
#
# LR/SC are the A-extension subset already present in the base MARCH
# (rv64imach after ENABLE_HYP), so the assembler accepts the lr.w/sc.w
# mnemonics and their .aq/.rl/.aqrl annotations directly.
#
# Two-stage (VS-stage Sv39 + G-stage Sv39x4) permission/htinst cases
# reuse common/hyp/two_stage_helpers.h. Spike needs _svadu so the
# henvcfg.ADUE hardware A/D-update cases can run.
# =====================================================================

TARGET     = hypervisor_zalrsc_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_TWO_STAGE = 1
ENABLE_PMP = 1

# QEMU virt: full-featured CPU model (includes A / Zalrsc + Svadu).
QEMU_CPU   = max

# Spike: RV64 + H + Svadu (hardware A/D update for the ADUE cases).
# Zama16b is enabled so the misaligned LR/SC cases (Group 1.6) run against the
# platform config/spike-rv64-max declares: per norm:pma_mag_op_rsrv LR/SC are
# NEVER relaxed by the misaligned atomicity granule, so they must still fault
# even with Zama16b present.

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# Suite G-stage / VS-stage paging mode: follow the platform-selected mode
# by default; override per build with `make HGATP_MODE=sv39|sv48|sv57`
# and/or `VSATP_MODE=sv39|sv48|sv57` (see common/Makefile.common).
SUITE_VSATP_MODE ?= PLATFORM_SATP_MODE
SUITE_HGATP_MODE ?= PLATFORM_HGATP_MODE
