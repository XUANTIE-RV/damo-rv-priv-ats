# =====================================================================
# Hypervisor x Zca Cross Verification Test
# =====================================================================
#
# Tests for the Zca (integer compressed instruction) extension in
# Hypervisor scenarios, matching
# DOCS/testplan_en/Hypervisor_Zc_test_plan_en.md Group 1 (HZCA-01~29):
#   - 1.1 HS/VS/VU-mode compressed exec + cause=22 exclusion
#   - 1.2 htinst transformed of register-based c.lw/c.sw/c.ld/c.sd
#   - 1.3 htinst transformed of stack-pointer c.lwsp/c.swsp/c.ldsp/c.sdsp
#   - 1.4 transformed load/store format distinction + field retention
#   - 1.5 Addr. Offset semantics + htinst may be zero
#   - 1.6 mtinst compressed transformed on M-mode trap
#   - 1.7 control: interrupt writes zero
#   - 1.8 IALIGN=16 + instruction-address-misaligned exclusion
#   - 1.9 fetch-class exceptions do not write a transformed htinst
#
# Non-hypervisor Zca behavior (encodings, HINT space, base semantics)
# is covered by the standalone Zca test plan. The existence of the
# compressed htinst transformation mechanism (single c.lw sample) is
# TINST-08 of Hypervisor_Exceptions; this suite traverses all eight
# compressed load/store variants (including the store and stack-pointer
# branches) to verify each expansion mapping.
#
# The compressed ('c') subset is already part of the base MARCH
# (rv64imac -> rv64imach after ENABLE_HYP), so the assembler accepts
# the c.* mnemonics directly. Fault probes wrap a single compressed
# instruction in `.option rvc` so the trapping instruction is exactly
# 2 bytes wide; the framework trap handler advances xEPC by +2 for a
# compressed trapping instruction (next_instruction() in common/trap.c).
#
# Two-stage (VS-stage Sv39 + G-stage Sv39x4) permission/htinst cases
# reuse common/hyp/two_stage_helpers.h.
# =====================================================================

TARGET     = hypervisor_zca_test.elf
ENABLE_VM  = 1
ENABLE_HYP = 1
ENABLE_TWO_STAGE = 1
ENABLE_PMP = 1

# QEMU virt: full-featured CPU model (includes C / Zca).
QEMU_CPU   = max

# Spike: RV64 + H + C (Zca is the integer subset of 'c', already in imach).

EXT_OBJS = \
    main.o \
    tests/test_register.o

include ../common/Makefile.common

# Suite G-stage / VS-stage paging mode: follow the platform-selected mode
# by default; override per build with `make HGATP_MODE=sv39|sv48|sv57`
# and/or `VSATP_MODE=sv39|sv48|sv57` (see common/Makefile.common).
SUITE_VSATP_MODE ?= PLATFORM_SATP_MODE
SUITE_HGATP_MODE ?= PLATFORM_HGATP_MODE
