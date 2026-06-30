**[中文](../testplan/Shvstvecd_test_plan.md) | English**

# Shvstvecd Extension Test Plan

This document describes the test plan for the Shvstvecd (Direct Trap Vectoring for VS-mode, Version 1.0) extension. Shvstvecd is a narrow constraint extension on `vstvec` register behavior. The specification requires implementations to guarantee: (1) `vstvec.MODE` can be written and hold the value 0 (Direct); (2) when `vstvec.MODE=Direct`, `vstvec.BASE` must be capable of holding any valid four-byte-aligned address.

---

## Test Scope

### Specification Sources

- `SPEC/shvstvecd.adoc` — Shvstvecd Extension for Direct Trap Vectoring, Version 1.0
- `SPEC/hypervisor.adoc` lines 1300–1312 (`vstvec` register definition) — Provides `vstvec` field layout and the behavior of substituting for `stvec` when V=1
- `SPEC/supervisor.adoc` lines 318–365 (`stvec` register and MODE field encoding) — Provides BASE/MODE field layout and Direct/Vectored behavior definitions (vstvec format is identical to stvec)

### Key Reference Files

| Path | Description |
|------|-------------|
| `SPEC/shvstvecd.adoc` | Full Shvstvecd specification (10 lines total) |
| `SPEC/hypervisor.adoc:1300-1312` | `vstvec` register specification, defined as VSXLEN-bit RW, substitutes for `stvec` when V=1 |
| `SPEC/supervisor.adoc:318-365` | `stvec` register specification, defines BASE/MODE fields + Direct/Vectored behavior (vstvec format is identical) |
| `common/encoding.h:289` | `CSR_VSTVEC = 0x205` |
| `common/csr_accessors.c:216,414` | Existing `_CSR_READ_CASE(0x205)` / `_CSR_WRITE_CASE(0x205)` generic CSR read/write entry points |
| `common/hyp/hyp_priv.h:21` | `run_in_vs_mode(fn, arg)` — Execute test function in VS-mode (V=1) |
| `common/hyp/hyp_csr.h:124` | `hyp_delegate_to_vs(hedeleg_mask, hideleg_mask)` — Delegate exceptions/interrupts to VS-mode |
| `common/hyp/hyp_csr.h:135` | `hvip_set_vssi(pending)` — Inject VS software interrupt |
| `common/hyp/hyp_test.h` | Hypervisor test macros (`HYP_TEST_END`, `EXPECT_GUEST_PAGE_FAULT`, etc.) |
| `common/hyp/hyp_reset.c:98` | `csrw 0x205, zero` resets vstvec in `hyp_reset_state()` |
| `DOCS/framework/hypervisor_framework.md` | Hypervisor test framework overview |
| `DOCS/testplan/sstvecd_test_plan.md` | Sibling S-mode stvec test plan, used as structural reference |

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:shvstvecd_vstvec_mode_direct` | If the Shvstvecd extension is implemented, then `vstvec.MODE` must be capable of holding the value 0 (Direct). |
| `norm:shvstvecd_vstvec_base_aligned_address` | Furthermore, when `vstvec.MODE`=Direct, `vstvec.BASE` must be capable of holding any valid four-byte-aligned address. |
| `norm:vstvec_sz_acc_op` | The `vstvec` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `stvec`. When V=1, `vstvec` substitutes for the usual `stvec`. When V=0, `vstvec` does not directly affect the behavior of the machine. |
| `norm:stvec_op` | The BASE field in `stvec` is a field that can hold any valid virtual or physical address, subject to the following alignment constraints: the address must be 4-byte aligned, and MODE settings other than Direct might impose additional alignment constraints on the value in the BASE field. |
| `norm:stvec_sz_base` | The CSR contains only bits XLEN-1 through 2 of the address BASE. When used as an address, the lower two bits are filled with zeroes to obtain an XLEN-bit address that is always aligned on a 4-byte boundary. |

> [!IMPORTANT]
> The Shvstvecd specification itself has only two strong constraints (Direct mode must be holdable, BASE must be capable of holding 4-byte aligned addresses). Group 3's trap jump tests provide end-to-end verification that "BASE setting actually takes effect and Direct mode behavior is correct", with normative basis from `SPEC/supervisor.adoc:355-364` (stvec MODE encoding definition, vstvec format is identical) — since Shvstvecd strongly constrains Direct mode to be available, it naturally requires that mode's behavior to conform to the supervisor specification. Group 4 verifies that when V=1, accessing via the `stvec` instruction name actually accesses `vstvec`, with normative basis from `SPEC/hypervisor.adoc:1305-1307`.

### Out of Scope

- **Functional behavior of Vectored mode**: Shvstvecd does not require implementation of Vectored mode; this plan only "probes" whether Vectored is implemented in Group 1, without verifying its correctness
- **S-mode `stvec`**: Shvstvecd only constrains `vstvec`, not `stvec`; `stvec` is covered by Sstvecd
- **M-mode `mtvec`**: Not within the scope of Shvstvecd specification
- **Multi-hart scenarios**: Project uses single-core test environment
- **Sv32 / Sv48 / Sv57 modes**: Only covers RV64 + Sv39x4, consistent with other extension plans in the project
- **BASE range under different VSXLEN values**: Only covers VSXLEN=64
- **G-stage address translation verification**: This test only uses identity mapping to establish basic execution environment, without verifying translation correctness

---

## Design Key Points

### 1. Access Paths for vstvec

`vstvec` (CSR 0x205) can be accessed through two paths:

- **HS/M-mode direct access**: `csrr/csrw 0x205` (using the `vstvec` CSR number) — Groups 1 & 2 use this path, operating vstvec directly in M-mode
- **VS-mode indirect access**: When V=1, VS-mode executing `csrr/csrw stvec` (CSR 0x105) actually accesses `vstvec` — Groups 3 & 4 use this path, setting vstvec through the stvec instruction name in VS-mode

### 2. Trap Entry Placement (Group 3 Specific)

Group 3 requires a custom trap entry usable in VS-mode, similar to sstvecd but running in VS-mode context. Requirements:

- **4-byte aligned**: Satisfies `vstvec.BASE` alignment constraint (ensured with `.align 2`)
- **Within identity-mapped range**: Ensures trap entry address is valid in VS-mode (covered by G-stage identity mapping)
- **Minimal handler path**: Records hit PC to global variable `g_shvstvecd_trap_pc`, records `vscause` to `g_shvstvecd_trap_cause`, then increments `vsepc += 4` and returns via `sret` (when V=1, `sepc/scause/sret` actually operate on `vsepc/vscause`)

> [!NOTE]
> During actual implementation, refer to `sstvecd_strap.S` from the `sstvecd` test plan. In VS-mode, instructions like `scause/sepc/sscratch/sret` automatically map to VS CSRs like `vscause/vsepc/vsscratch` when V=1.

### 3. Trap Delegation to VS-mode

To ensure traps are handled within VS-mode (rather than escalating to HS-mode), delegation via `hedeleg` is required:

- **ecall from VU-mode (cause=8)**: Set `hedeleg` bit 8 to 1 (delegate environment call exception to VS)
- **illegal instruction (cause=2)**: Set `hedeleg` bit 2 to 1
- **VS software interrupt (VSSIP)**: Set `hideleg` bit 2 to 1 (delegate VS-level software interrupt to VS)

Use the `hyp_delegate_to_vs(hedeleg_mask, hideleg_mask)` API to complete delegation configuration.

### 4. VS-mode Interrupt Injection (Group 3 VSTVEC-INT-01)

To verify "in Direct mode, interrupts also jump to BASE rather than BASE+4×cause", VSSIP (virtual supervisor software interrupt) is used:

1. M/HS-mode injects VSSIP pending via `hvip_set_vssi(true)`
2. `hideleg` bit 2 = 1, ensuring VSSIP is delegated to VS-mode
3. After entering VS-mode: first `csrw stvec, BASE` (actually writes vstvec), then enable `sie.SSIE` (actually operates `vsie.SSIE`) + `sstatus.SIE` (actually operates `vsstatus.SIE`), interrupt triggers immediately
4. After trap hits, assert: `g_shvstvecd_trap_pc == BASE` (Direct); if implementation is Vectored, hit address should be `BASE + 4*2 = BASE+0x8` (offset for VSSIP cause=2), assertion failure can locate incorrect Direct behavior

> [!NOTE]
> Injecting VSSIP via `hvip` is the simplest way to trigger VS-level interrupts in hypervisor testing, without AIA/PLIC dependencies.

### 5. Isolation from hyp_reset_state

`common/hyp/hyp_reset.c::hyp_reset_state()` defaults to setting `vstvec` to 0 (line 98). Each test case in this plan backs up vstvec at M-mode level before operations and restores it afterward. For operations within VS-mode (Group 3), assertions and cleanup are performed after returning to M-mode via the ecall return mechanism of `run_in_vs_mode`.

### 6. MODE Field Reserved Value Behavior

The SPEC does not declare `vstvec` as a WARL register (`hypervisor.adoc:1302-1308` only describes it as "read/write"), so the behavior of writing reserved values (2, 3) to the MODE field is undefined and out of test scope.

`vstvec.MODE` field:

- Writing 0 (Direct) must succeed (Shvstvecd strong constraint)
- Writing 1 (Vectored): if implementation supports it, reads back as 1; if not supported, reads back as 0. Neither behavior violates Shvstvecd
- Writing ≥2 (Reserved): behavior is undefined (SPEC does not declare WARL), no assertions made

`vstvec.BASE` field ([XLEN-1:2]):

- Shvstvecd requires capability to hold "any valid 4-byte aligned address" when MODE=Direct
- Written high-order BASE bits should read back unchanged

---

## Test Groups

> [!IMPORTANT]
> Total of 5 test groups, 16 test cases. Groups 1 & 2 run in M-mode directly operating `vstvec` (CSR 0x205); Group 3 operates `vstvec` indirectly through the `stvec` instruction name within VS-mode (V=1) and verifies trap behavior; Group 4 verifies V=1 pass-through semantics; Group 5 probes Vectored mode support (non-normative). Each group provides: normative basis, test responsibilities, test case table (ID/name/description/expected result); each group provides 1 key C code example.

---

### Group 1: `vstvec.MODE` Writability

**Normative Basis**:
- `norm:shvstvecd_vstvec_mode_direct` (`SPEC/shvstvecd.adoc:4-6`): `vstvec.MODE` must be capable of holding the value 0 (Direct)

**Test Responsibilities**: Verify that `vstvec.MODE` can be stably written and hold the Direct value (0); probe whether Vectored (1) is implemented.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSTVEC-MODE-01 | MODE write 0 (Direct) readback | M-mode `csrw 0x205, 0` (BASE=0, MODE=0), read back `vstvec[1:0]` | `vstvec[1:0] == 0` (Direct must be holdable) |
| VSTVEC-MODE-02 | MODE 0→1→0 switching | First write MODE=0, read back to confirm; then write MODE=1, read back and record; finally write MODE=0, read back to confirm | 1st and 3rd readbacks `MODE==0`; 2nd readback ∈ {0, 1} (implementation-defined) |

---

### Group 2: `vstvec.BASE` Holding Capability in Direct Mode

**Normative Basis**:
- `norm:shvstvecd_vstvec_base_aligned_address` (`SPEC/shvstvecd.adoc:8-10`): When `vstvec.MODE`=Direct, `vstvec.BASE` must be capable of holding any valid four-byte-aligned address
[id:g4xguh]
- `norm:stvec_sz_base` (`SPEC/supervisor.adoc:335-338`): CSR only stores BASE bits [XLEN-1:2], lower 2 bits are forced to 0 on write

**Test Responsibilities**: Verify that under MODE=Direct, the BASE field can hold various 4-byte aligned addresses (including those crossing 1 GiB / 512 GiB boundaries, near VSXLEN upper bound); verify that BASE and MODE writes are independent.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSTVEC-BASE-01 | BASE write 0 readback | `csrw 0x205, 0x0` (BASE=0, MODE=0), read back | `vstvec == 0x0` |
| VSTVEC-BASE-02 | BASE write PLATFORM_MEM_BASE readback | `csrw 0x205, PLATFORM_MEM_BASE | 0`, read back | Readback high bits == `PLATFORM_MEM_BASE`, low 2 bits == 0 |
| VSTVEC-BASE-03 | BASE write crossing 1 GiB boundary | `csrw 0x205, 0x40000004` (crossing 1 GiB), read back | Readback == `0x40000004` |
| VSTVEC-BASE-04 | BASE write crossing 512 GiB boundary (high-bit probe) | `csrw 0x205, 0x8000000000` (bit 39), read back | Readback == `0x8000000000` (if high bits are truncated, proves Shvstvecd is not truly enabled, failure) |
| VSTVEC-BASE-06 | Multiple BASE rewrites do not affect MODE | Fix MODE=0, sequentially write BASE=0x1000, 0x2000, 0x4000, 0x8000, read back MODE each time | Each time `MODE == 0` and BASE matches written value |
| VSTVEC-BASE-07 | BASE high-bit scan | Sequentially write `1ULL << k | 0x4` (k from 12 to VSXLEN-1, only taking 4-byte aligned bits), read back each time | Each readback BASE == written value (Shvstvecd requires "any valid") |
| VSTVEC-BASE-08 | BASE maximum legal address (address space upper bound) | Write `0x7FFFFFFFFFFFFFFC` (VSXLEN-1 bits all 1 and 4-byte aligned), read back | Readback == `0x7FFFFFFFFFFFFFFC` (Shvstvecd requires "any valid 4-byte aligned address", should cover address space upper bound) |

> [!NOTE]
> **VSTVEC-BASE-04 high-bit selection**: Bit 39 corresponds to near the virtual address upper bound of Sv39. In VSXLEN=64 environment, Shvstvecd specification requires BASE to hold "any valid" 4-byte aligned address, so high bits should not be truncated.

> [!NOTE]
> **VSTVEC-BASE-07 scan range**: Select several k values (e.g., k=12, 16, 20, 24, 28, 32, 36, 38) following the principle of "above `PLATFORM_MEM_BASE`, not conflicting with trap entry", exhaustive enumeration to 63 bits is not required.

---

### Group 3: VS-mode Trap Jumps to BASE under `MODE=Direct`

**Normative Basis**:
- `SPEC/supervisor.adoc:355-364`: When MODE=Direct, all traps (synchronous exceptions + asynchronous interrupts) set `pc` to BASE (vstvec format is identical to stvec)
- `SPEC/hypervisor.adoc:1305-1307`: When V=1, `vstvec` substitutes for `stvec`
- `norm:H_trap_vs_csrwrites` (`SPEC/hypervisor.adoc:2549-2554`): When trap enters VS-mode, writes `vsepc`, `vscause`, `vstval`, jump target is the address specified by `vstvec`
- Shvstvecd strongly constrains Direct to be available (`norm:shvstvecd_vstvec_mode_direct`), therefore Direct behavior correctness is an implicit commitment of Shvstvecd

**Test Responsibilities**: Verify that when `vstvec.MODE=Direct`, synchronous exceptions and asynchronous interrupts triggered in VS-mode both jump to BASE (rather than BASE+4×cause).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSTVEC-DIR-01 | Synchronous exception: VU-mode ecall | In VS-mode, set custom 4-byte aligned entry via `vstvec` (actually through stvec instruction); trigger ecall from VU-mode (cause=8), trap delegated to VS-mode | `g_shvstvecd_trap_pc == BASE`; `g_shvstvecd_trap_cause == 8` |
| VSTVEC-DIR-02 | Synchronous exception: illegal instruction | VS-mode executes unknown instruction (cause=2), delegated to VS-mode | `g_shvstvecd_trap_pc == BASE`; `g_shvstvecd_trap_cause == 2` |
| VSTVEC-INT-01 | Asynchronous interrupt: VSSIP | `hvip` injects VSSIP, `hideleg` bit 2 = 1 delegates to VS-mode; VS-mode sets vstvec=entry then enables interrupts | `g_shvstvecd_trap_pc == BASE` (Direct does not add 4×cause) |
| VSTVEC-DIR-03 | Synchronous exception: load page-fault | VS-mode accesses unmapped virtual address triggering load page-fault (cause=13), `hedeleg` bit 13 delegates to VS-mode | `g_shvstvecd_trap_pc == BASE`; `g_shvstvecd_trap_cause == 13` |

> [!NOTE]
> **VSTVEC-DIR-01 trigger method**: VS-mode first sets up vstvec (via `csrw stvec, entry`, which actually operates vstvec when V=1), then switches to VU-mode to execute `ecall` (cause=8). Since `hedeleg` bit 8 is already set to 1, ecall from VU will be delegated to VS-mode, and trap entry jumps to vstvec.BASE.

> [!NOTE]
> **VSTVEC-INT-01 interrupt source**: Use `hvip_set_vssi(true)` to inject VSSIP pending from HS/M-mode, then enter VS-mode and enable `vsie.SSIE` + `vsstatus.SIE`. This is the most concise way to trigger VS-level interrupts in H extension, without external interrupt controller.

---

### Group 4: V=1 Pass-through Verification (`stvec` Access Actually Operates `vstvec`)

**Normative Basis**:
- `norm:vstvec_sz_acc_op` (`SPEC/hypervisor.adoc:1302-1308`): When V=1, `vstvec` substitutes for the usual `stvec`, so instructions that normally read or modify `stvec` actually access `vstvec` instead

**Test Responsibilities**: Verify that values written via `stvec` instruction name in VS-mode (V=1) can be read back via `vstvec` (CSR 0x205) from HS/M-mode; and vice versa.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSTVEC-TRANS-01 | VS-mode writes stvec → M-mode reads vstvec | VS-mode writes `stvec = 0xDEAD0000`, after returning to M-mode read `vstvec` (CSR 0x205) | `vstvec == 0xDEAD0000` (MODE=0, BASE=0xDEAD0000) |
| VSTVEC-TRANS-02 | M-mode writes vstvec → VS-mode reads stvec | M-mode writes `vstvec = 0xBEEF0004`, after entering VS-mode read `stvec` | VS-mode readback == `0xBEEF0004` |
| VSTVEC-TRANS-03 | VS-mode writes stvec does not affect HS-mode stvec | VS-mode writes `stvec = 0x12340000`, after returning to HS-mode read real `stvec` (CSR 0x105 in V=0) | HS-mode `stvec` retains original value (not modified by VS-mode operation) |

---

### Group 5: Vectored Mode Probing (Non-normative, Completeness Verification)

**Normative Basis**:
- Shvstvecd specification **does not require** implementation of Vectored mode (MODE=1), only requires Direct (MODE=0) to be holdable
- `SPEC/supervisor.adoc:355-364`: When stvec MODE=1 (Vectored), interrupts jump to `BASE + 4×cause`

**Test Responsibilities**: Probe whether vstvec.MODE supports Vectored (information gathering only); if supported, verify interrupt jump behavior differs from Direct.

> [!NOTE]
> This group of tests is not within the normative coverage of Shvstvecd. VSTVEC-VEC-01 only probes and records results, without pass/fail judgment. VSTVEC-VEC-02 only executes functional verification when Vectored is available.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSTVEC-VEC-01 | Vectored mode writability probe | M-mode writes `vstvec = BASE | 1` (MODE=Vectored), read back to check MODE field | If readback `MODE == 1`, record "Vectored supported"; if readback `MODE == 0`, record "Vectored not supported, TEST_SKIP VEC-02" |
| VSTVEC-VEC-02 | Vectored interrupt jumps to BASE+4×cause | Prerequisite: VEC-01 confirms Vectored is available. VS-mode sets `vstvec = entry | 1`; inject VSSIP (cause=1) | `g_shvstvecd_trap_pc == BASE + 4*1` (Vectored mode interrupt offset) |

---

## Appendix: Related Constants and API Reference

### CSR and Bit Definitions

| Name | Value | Description |
|------|-------|-------------|
| `CSR_VSTVEC` | `0x205` | Virtual supervisor trap vector base address, see `common/encoding.h:289` |
| `CSR_STVEC` | `0x105` | Supervisor trap vector base address (access actually operates vstvec when V=1) |
| `VSTVEC_MODE_MASK` | `0x3` | `vstvec[1:0]` i.e., MODE field |
| `VSTVEC_MODE_DIRECT` | `0x0` | Direct mode |
| `VSTVEC_MODE_VECTORED` | `0x1` | Vectored mode (Shvstvecd does not require implementation) |
| `HIDELEG_VSSIP_BIT` | `1UL << 2` | Delegate VSSIP to VS-mode |
| `HVIP_VSSIP_BIT` | `1UL << 2` | VSSIP pending bit in hvip |

### vscause Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `CAUSE_ECALL_FROM_VU` | 8 | Environment call from VU-mode |
| `CAUSE_ILLEGAL_INSTRUCTION` | 2 | Illegal instruction |
| `CAUSE_VS_SOFTWARE_INTERRUPT` (interrupt number) | 1 | VS software interrupt (vscause high bit set to 1) |

### Test Framework API

- `run_in_vs_mode(fn, arg)`: Execute fn(arg) in VS-mode (V=1), return to M-mode via ecall
- `run_in_vu_mode(fn, arg)`: Execute fn(arg) in VU-mode (V=1, U)
- `hyp_delegate_to_vs(hedeleg_mask, hideleg_mask)`: Configure exception/interrupt delegation to VS-mode
- `hyp_undelegate()`: Clear all hypervisor delegations
- `hvip_set_vssi(pending)`: Inject/clear VSSIP
- `hyp_reset_state()`: Reset all hypervisor CSRs (including vstvec=0)
- `HYP_TEST_END()`: Test end macro, includes hyp_reset_state + result recording
- `TEST_REGISTER` / `TEST_BEGIN` / `TEST_ASSERT` / `TEST_END`: Test case registration and assertion macros

### Global Variables (Provided by `shvstvecd/tests/shvstvecd_strap.S`)

| Variable | Type | Description |
|----------|------|-------------|
| `g_shvstvecd_trap_pc` | `volatile uintptr_t` | PC when VS-mode trap entry is hit (should equal `vstvec.BASE`) |
| `g_shvstvecd_trap_cause` | `volatile uintptr_t` | `vscause` when trap is hit |
| `shvstvecd_trap_entry` | `void(void)` | 4-byte aligned VS-mode trap entry symbol |

---

## Test Execution Notes

### Runtime Environment

- Groups 1 & 2: M-mode directly operates CSR 0x205 (vstvec), no need to enter VS-mode
- Group 3: Enter VS-mode via `run_in_vs_mode`, use G-stage identity mapping (Sv39x4) to ensure trap entry address is accessible
- Group 4: Alternating execution between M-mode ↔ VS-mode, verify CSR pass-through
- Single-core environment, no IPI required

### Failure Diagnosis Guide

| Symptom | Possible Cause |
|---------|----------------|
| VSTVEC-MODE-01 fails | Platform does not even implement basic vstvec, or `csrw 0x205` path is abnormal |
| VSTVEC-BASE-04 / 07 fails | Shvstvecd is not truly enabled, BASE high bits are WARL-truncated |
| VSTVEC-DIR-01/02 fails (hit address ≠ BASE) | Trap delegation path is abnormal, or hedeleg is not correctly configured |
| VSTVEC-INT-01 fails (hit address = BASE+offset) | Implementation incorrectly handles interrupt as Vectored; indicates MODE=0 write did not take effect |
| VSTVEC-INT-01 fails (no hit) | `hideleg` did not delegate VSSIP, or `hvip` did not inject, or `vsstatus.SIE` was not enabled |
| VSTVEC-TRANS-01/02 fails | stvec did not correctly map to vstvec when V=1, H extension implementation is abnormal |
| VSTVEC-TRANS-03 fails | VS-mode writing stvec incorrectly modified HS-mode stvec |
