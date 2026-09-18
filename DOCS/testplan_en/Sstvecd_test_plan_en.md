**[中文](../testplan/Sstvecd_test_plan.md) | English**

# Sstvecd Extension Test Plan

This document describes the test plan for the Sstvecd (Direct Trap Vectoring, Version 1.0) extension. Sstvecd is a narrow constraint extension on the `stvec` register behavior. The specification requires that implementations must guarantee: (1) `stvec.MODE` can be written and hold the value 0 (Direct); (2) when `stvec.MODE=Direct`, `stvec.BASE` must be capable of holding any 4-byte-aligned address.

---

## Specification Sections Covered by This Document

This plan is based on the following RISC-V official specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/sstvecd.adoc` — Sstvecd Extension for Direct Trap Vectoring, Version 1.0
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — `stvec` register and MODE field encoding, Direct/Vectored behavior definitions, `sip`.SSIP self-triggering behavior

Official repository:

- https://github.com/riscv/riscv-isa-manual (corresponding files `src/priv/sstvecd.adoc`, `src/priv/supervisor.adoc` in the repository)

---

## Test Scope

### Covered Specification Points

| Norm ID | Original Text | Description |
|---------|---------------|-------------|
| `norm:sstvecd_stvec_mode_direct` | If the Sstvecd extension is implemented, then `stvec.MODE` must be capable of holding the value 0 (Direct). | If Sstvecd is implemented, `stvec.MODE` must be able to hold the value 0 (Direct). |
| `norm:sstvecd_stvec_base_aligned_address` | Furthermore, when `stvec.MODE=Direct`, `stvec.BASE` must be capable of holding any valid four-byte-aligned address. | When `stvec.MODE=Direct`, `stvec.BASE` must be able to hold any valid four-byte-aligned address. |
| `norm:stvec_op` | The BASE field in `stvec` is a field that can hold any valid virtual or physical address, subject to the following alignment constraints: the address must be 4-byte aligned, and MODE settings other than Direct might impose additional alignment constraints on the value in the BASE field. | The BASE field can hold any valid virtual or physical address, but must be 4-byte aligned; non-Direct modes may impose stricter alignment constraints. |
| `norm:stvec_sz_base` | The CSR contains only bits XLEN-1 through 2 of the address BASE. When used as an address, the lower two bits are filled with zeroes to obtain an XLEN-bit address that is always aligned on a 4-byte boundary. | The CSR stores only BASE[XLEN-1:2]. When used as an address, the low two bits are filled with zeroes to yield an XLEN-bit address that is always 4-byte aligned. |

> [!IMPORTANT]
> The Sstvecd specification itself has only two hard constraints (Direct must be holdable, BASE must be capable of holding any 4-byte-aligned address). The Group 3 trap-vectoring tests are end-to-end verification that "the BASE setting truly takes effect and Direct mode behavior is correct." The specification basis comes from the stvec MODE encoding definition in `supervisor.adoc` — Sstvecd mandates that Direct mode is available, and therefore naturally requires that mode's behavior to conform to the supervisor specification.

### Out of Scope

- **Functional behavior of Vectored mode**: Sstvecd does not require implementation of Vectored mode; this plan only "probes" whether Vectored is implemented in Group 1, without verifying its correctness.
- **VS-mode `vstvec`**: Sstvecd constrains only `stvec`, not `vstvec`.
- **M-mode `mtvec`**: Not within the scope of the Sstvecd specification.
- **Multi-hart scenarios**: The project is a single-core test environment.
- **Sv32 / Sv48 / Sv57 modes**: Only covers RV64 + Sv39, consistent with other extension plans in this project.
- **BASE range under different SXLEN values**: Only covers SXLEN=64.

---

## Design Notes

### 1. Handling Implementation Presence

The platform must enable the Sstvecd extension in the build configuration. If not enabled, Group 1's "write MODE=0, read back=0" test will still pass on most implementations (any compliant RISC-V implementation supports at least Direct=0), so this test case cannot independently prove that Sstvecd is enabled.

To strengthen the "Sstvecd must be enabled" prerequisite, the plan **intentionally writes a high-bit (e.g., bit 38 / bit 47 / near the SXLEN upper bound) 4-byte-aligned address** in the Group 2 BASE-04 test case — if the implementation has not declared Sstvecd, it may treat the upper bits of BASE as WARL-truncated (read-only 0), thereby exposing implementation differences. This test case uses a hard assert; failure indicates the platform does not truly implement Sstvecd.

### 2. Trap Entry Placement (Group 3 Specific)

Group 3 requires a user-controllable S-mode trap entry with the following requirements:

- **4-byte aligned**: Satisfies the `stvec.BASE` alignment constraint.
- **Located in an executable code segment**: No special linker-script handling required.
- **Minimal handling path**: Save general-purpose registers and `sscratch`, record the hit address (i.e., the current PC, equal to `stvec.BASE`) to one global variable, record `scause` to another global variable, then advance `sepc` according to the compressed-instruction width and `sret` back.

Key design points:
- The trap entry internally uses `auipc` + offset to compute its own PC and writes it to a global variable for the assertion "trap hit address = stvec.BASE."
- The `sepc` advance must be adapted to compressed instructions (read the instruction length at `sepc` to decide whether to advance 2 or 4 bytes) so that the return point is correct.

### 3. Isolation from the Default Framework

The framework's default `reset_state()` sets `stvec` to the default S-mode trap entry. Each test case in this suite should follow the template:

1. On test entry, first save the current `stvec` value.
2. Execute the test body (which may modify `stvec`).
3. Restore `stvec` before exit to avoid polluting subsequent tests.

No dependency on `medeleg`: Sstvecd does not involve exception delegation semantics. Group 3 synchronous exceptions are actively triggered within S-mode via the framework's S-mode execution helper; when triggered, the trap enters S-mode directly (rather than being delegated from M-mode), hitting the `stvec` we have configured.

### 4. Asynchronous Interrupt Triggering (Group 3 STVEC-INT-01)

To verify "in Direct mode, interrupts also vector to BASE rather than BASE+4×cause", SSIP (supervisor software interrupt) is used:

1. M-mode sets `mideleg.SSIE` (bit 1) = 1, delegating the supervisor software interrupt to S-mode.
2. M-mode prepares the hit-address and cause global variables as 0.
3. After entering S-mode: first write `stvec = BASE` (MODE=0), then enable `sie.SSIE` and `sstatus.SIE`, and finally self-trigger `sip.SSIP`.
4. After the trap fires, assert: hit PC == BASE (Direct); if the implementation uses Vectored mode, the hit address would be `BASE + 4*1 = BASE+0x4`, and the assertion failure would identify incorrect Direct behavior.

> [!NOTE]
> SSIP is a supervisor-internal writable interrupt flag (`norm:sip_ssip_sie_ssie`), making it the most suitable choice for self-triggering in a single-core environment. STIP (timer) requires Sstc or SBI, and SEIP (external) requires PLIC/AIA, both of which are relatively complex.

### 5. WARL Write-Readback Convention

The `stvec.MODE` field is WARL:

- Writing 0 (Direct) must succeed (Sstvecd hard constraint).
- Writing 1 (Vectored): if the implementation supports it, readback is 1; if not supported, readback is some legal value (the most natural choice is 0, since Direct is always supported). Neither behavior violates Sstvecd.
- Writing ≥2 (Reserved): must read back as a legal value (0 or 1); must not be latched.

The `stvec.BASE` field ([XLEN-1:2]) is WARL:

- Writing a value with the low 2 bits non-zero: hardware should force the low 2 bits to 0 (`norm:stvec_sz_base`).
- The upper BASE bits written should read back as-is (Sstvecd requires "any valid 4-byte-aligned address").

---

## Test Groups

> [!IMPORTANT]
> A total of 3 test groups and 16 test cases. All tests run under RV64 + M-mode setup, with some cases entering S-mode (Group 3).

---

### Group 1: `stvec.MODE` Writability

**Spec Reference**:
- `norm:sstvecd_stvec_mode_direct`: `stvec.MODE` must be capable of holding the value 0 (Direct).

**Test Scope**: Verify that `stvec.MODE` can be stably written and hold the Direct value (0); probe whether Vectored (1) is implemented; verify that WARL behavior is reasonable after writing reserved values (≥2).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| STVEC-MODE-01 | MODE write 0 (Direct) readback | Write `stvec = 0` (BASE=0, MODE=0), read back `stvec[1:0]` | `stvec[1:0] == 0` (Direct must always be holdable) |
| STVEC-MODE-02 | MODE 0→1→0 toggle | Write MODE=0, readback and confirm; then write MODE=1, readback and record; finally write MODE=0, readback and confirm | 1st and 3rd readbacks `MODE==0`; 2nd readback ∈ {0, 1} (implementation-defined) |
| STVEC-MODE-03 | MODE write reserved value (=2) | Write `stvec = 0x2` (BASE=0, MODE=2), read back `stvec[1:0]` | Readback ∈ {0, 1}, must not be 2 (WARL, reserved values must not be latched) |
| STVEC-MODE-04 | MODE write reserved value (=3) | Write `stvec = 0x3` (BASE=0, MODE=3), read back `stvec[1:0]` | Readback ∈ {0, 1}, must not be 3 |

---

### Group 2: `stvec.BASE` Holding Capability in Direct Mode

**Spec Reference**:
- `norm:sstvecd_stvec_base_aligned_address`: When `stvec.MODE=Direct`, `stvec.BASE` must be capable of holding any valid 4-byte-aligned address.
- `norm:stvec_sz_base`: The CSR stores only BASE[XLEN-1:2]; the low 2 bits are forced to 0 on write.

**Test Scope**: Verify that under MODE=Direct, the BASE field can hold various 4-byte-aligned addresses (including those crossing 1 GiB / 512 GiB boundaries and near the SXLEN upper bound); verify that writing a non-4-byte-aligned address forces the low 2 bits to zero; verify that BASE and MODE writes are independent of each other.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| STVEC-BASE-01 | BASE write 0 readback | Write `stvec = 0x0` (BASE=0, MODE=0), read back | `stvec == 0x0` |
| STVEC-BASE-02 | BASE write platform memory base readback | Write `stvec = PLATFORM_MEM_BASE` (MODE=0), read back | Readback upper bits == platform memory base; low 2 bits == 0 |
| STVEC-BASE-03 | BASE write across 1 GiB boundary | Write `stvec = 0x40000004` (cross 1 GiB), read back | Readback == `0x40000004` |
| STVEC-BASE-04 | BASE write across 512 GiB boundary (high-bit probe) | Write `stvec = 0x8000000000` (bit 39), read back | Readback == `0x8000000000` (if upper bits are WARL-truncated, proves Sstvecd is not truly enabled; failure) |
| STVEC-BASE-05 | BASE write non-4-byte-aligned address | Write `stvec = 0x40000003` (low 2 bits = 0b11, MODE field), read back | Readback BASE portion == `0x40000000`; MODE field reads back as a legal value (0 or 1) |
| STVEC-BASE-06 | BASE multiple rewrites do not affect MODE | Fix MODE=0, write BASE=0x1000, 0x2000, 0x4000, 0x8000 in sequence, read back MODE each time | Each time `MODE == 0` and BASE matches the written value |
| STVEC-BASE-07 | BASE high-bit scan | Write `1ULL << k | 0x4` in sequence (k from 12 to SXLEN-1, only 4-byte-aligned positions), read back each time | Each readback BASE == written value (Sstvecd requires "any valid") |

> [!IMPORTANT]
> **STVEC-BASE-05 low 2 bits semantic note**: When writing `0x40000003`, [1:0]=0b11 actually falls in the MODE field (not BASE bit[1:0]). The specification defines BASE as [XLEN-1:2], so this test case **actually verifies**: BASE field retention = `0x40000000`, MODE field = 0b11. MODE=3 is a reserved value; per STVEC-MODE-04 rules, readback should be a legal value (0 or 1). Therefore, the precise assertion for BASE-05 is: `(readback & ~0x3) == 0x40000000` and `(readback & 0x3) ∈ {0, 1}`.

> [!NOTE]
> **STVEC-BASE-04 high-bit selection**: Bit 39 corresponds to near the Sv39 virtual address upper bound, a typical choice for probing high-bit WARL truncation. If the implementation constrains BASE to [38:2] (as some RV64 implementations may default to truncating at the Sv39 upper bound), this test case will fail. The Sstvecd specification requires BASE to hold "any valid" 4-byte-aligned address, so such a restriction should be lifted by a compliant Sstvecd implementation.

> [!NOTE]
> **STVEC-BASE-07 scan range**: Following the principle of "above the platform memory base, not conflicting with the trap entry," select several k values (e.g., k=12, 16, 20, 24, 28, 32, 36, 38); exhaustive scanning up to bit 63 is not required. If SXLEN=64 but the valid virtual address bits are only 39/48/57, the behavior of writing addresses beyond the valid bits is implementation-defined; this test case covers only up to bit 38 (within Sv39 range).

---

### Group 3: Trap Vectoring to BASE under `MODE=Direct`

**Spec Reference**:
- `supervisor.adoc` stvec MODE encoding: When MODE=Direct, all traps (synchronous exceptions + asynchronous interrupts) set `pc` to BASE.
- Sstvecd hard-constrains Direct to always be available (`norm:sstvecd_stvec_mode_direct`), so correct Direct behavior is an implicit commitment of Sstvecd.

**Test Scope**: Verify that when `stvec.MODE=Direct`, both synchronous exceptions and asynchronous interrupts vector to BASE (rather than BASE+4×cause); verify that BASE retains its original value after multiple traps.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| STVEC-DIR-01 | Synchronous exception: S-mode ecall | Execute `ecall` in S-mode (cause=9), `stvec` set to a custom 4-byte-aligned entry, BASE=entry address | Hit PC == BASE (entry start); `scause == 9` |
| STVEC-DIR-02 | Synchronous exception: illegal instruction | Execute an unknown instruction in S-mode (cause=2), same entry as above | Hit PC == BASE; `scause == 2` |
| STVEC-DIR-03 | Synchronous exception: load page-fault | Enable Sv39 + unmapped VA, S-mode load triggers cause=13 | Hit PC == BASE; `scause == 13` |
| STVEC-INT-01 | Asynchronous interrupt: SSIP | M-mode sets `mideleg.SSIE=1`, S-mode self-triggers SSIP (cause=1, interrupt bit set) | Hit PC == BASE (Direct does not add 4×1=4) |
| STVEC-MULTI-01 | Multiple traps, BASE unchanged | Trigger 5 consecutive ecalls; after each trap, M-mode reads back `stvec` to check BASE and MODE | After 5 times, `BASE == original value` and `MODE == 0` |

---

## Runtime Environment

- All tests run under RV64 + Sv39 (Group 3) / M-mode direct CSR operations (Groups 1, 2).
- Single-core environment; no IPI required.
- If any platform violates the SPEC, the corresponding case remains FAIL, and implementation defects are recorded to the `bugs/` directory.

### Failure Diagnosis Guide

| Symptom | Possible Cause |
|---------|----------------|
| STVEC-MODE-01 fails | Platform does not even implement basic stvec, or the CSR write path is abnormal |
| STVEC-BASE-04 / 07 fails | Sstvecd not truly enabled; BASE upper bits are WARL-truncated |
| STVEC-DIR-01/02/03 fails (hit address ≠ BASE) | Trap path abnormal, or the framework's S-mode execution helper internally overwrites `stvec` |
| STVEC-INT-01 fails (hit address = BASE+4) | Implementation incorrectly handles interrupts as Vectored; indicates MODE=0 write did not take effect |
| STVEC-INT-01 fails (no hit) | `mideleg` did not delegate SSIP, or `sstatus.SIE` not enabled |

---

## Appendix A: Specification Point Coverage Matrix

| Norm ID | Covered Test IDs | Coverage Status | Notes |
|---------|------------------|-----------------|-------|
| `norm:sstvecd_stvec_mode_direct` | STVEC-MODE-01, STVEC-MODE-02, STVEC-DIR-01, STVEC-DIR-02, STVEC-DIR-03, STVEC-INT-01, STVEC-MULTI-01 | Covered | Direct mode is writable and holdable |
| `norm:sstvecd_stvec_base_aligned_address` | STVEC-BASE-01 ~ STVEC-BASE-07, STVEC-DIR-01 ~ STVEC-DIR-03, STVEC-INT-01, STVEC-MULTI-01 | Covered | BASE can hold any 4-byte-aligned address |
| `norm:stvec_op` | STVEC-MODE-01 ~ STVEC-MODE-04, STVEC-BASE-01 ~ STVEC-BASE-07 | Covered | BASE field behavior and alignment constraint |
| `norm:stvec_sz_base` | STVEC-BASE-05, STVEC-BASE-06, STVEC-BASE-07 | Covered | CSR stores only [XLEN-1:2], low 2 bits forced to 0 |
