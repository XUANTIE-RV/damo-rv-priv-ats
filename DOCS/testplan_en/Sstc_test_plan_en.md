**[中文](../testplan/Sstc_test_plan.md) | English**

# Sstc Extension Test Plan

This document describes the test plan for the Sstc (Supervisor-mode Timer Interrupts, Version 1.0) extension. Sstc provides S-mode with its own CSR-based timer interrupt mechanism (`stimecmp` register), enabling S-mode to directly manage its own timer service without proxying through M-mode via SBI calls. Additionally, this extension provides a similar timer mechanism (`vstimecmp` register) for VS-mode under the Hypervisor extension, and introduces new `STCE` control bits in `menvcfg` and `henvcfg`.

---

## Specification Sections Covered by This Document

This plan is based on the following RISC-V official specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/sstc.adoc` — Sstc Extension for Supervisor-mode Timer Interrupts, Version 1.0
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — Supervisor Timer (`stimecmp`) register definition and STIP behavior; `sip`.STIP / `sie`.STIE behavior under Sstc
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — `mip`.STIP behavior change when stimecmp is implemented; `mcounteren`.TM access control for stimecmp/vstimecmp; `menvcfg`.STCE field definition
- `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — `hip`.VSTIP relationship with vstimecmp; `henvcfg`.STCE field definition
- `SPEC/riscv-isa-manual/src/priv/csrs.adoc` — stimecmp/stimecmph/vstimecmp/vstimecmph CSR addresses

Official repository:

- https://github.com/riscv/riscv-isa-manual (corresponding files `src/priv/sstc.adoc`, `src/priv/supervisor.adoc`, `src/priv/machine.adoc`, `src/priv/hypervisor.adoc`, `src/priv/csrs.adoc` in the repository)

---

## Test Scope

### Key CSRs

| CSR | Address | Description |
|-----|---------|-------------|
| `stimecmp` | 0x14D | S-level timer compare register (64-bit) |
| `stimecmph` | 0x15D | stimecmp high 32 bits (RV32 only) |
| `vstimecmp` | 0x24D | VS-level timer compare register (64-bit) |
| `vstimecmph` | 0x25D | vstimecmp high 32 bits (RV32 only) |
| `menvcfg` | 0x30A | Machine Environment Configuration, bit 63 = STCE |
| `henvcfg` | 0x60A | Hypervisor Environment Configuration, bit 63 = STCE |
| `mip` / `sip` | 0x344 / 0x144 | Interrupt pending, bit 5 = STIP |
| `mie` / `sie` | 0x304 / 0x104 | Interrupt enable, bit 5 = STIE |
| `mcounteren` | 0x306 | M-mode counter enable, bit 1 (TM) controls stimecmp access |
| `hcounteren` | 0x606 | HS-mode counter enable, bit 1 (TM) controls vstimecmp access |
| `time` | 0xC01 | Read-only, shadow register of mtime |
| `hvip` | 0x645 | Hypervisor virtual interrupt pending, bit 6 = VSTIP |
| `hip` | 0x644 | Hypervisor interrupt pending, bit 6 = VSTIP = hvip.VSTIP OR vstimecmp signal |

### Covered Specification Points

| Norm ID | Original Text | Description |
|---------|---------------|-------------|
| `norm:sstc_purpose` | This extension serves to provide supervisor mode with its own CSR-based timer interrupt facility that it can directly manage to provide its own timer service (in the form of having its own `stimecmp` register). | The extension provides S-mode with its own CSR-based timer interrupt facility (an `stimecmp` register) that it can directly manage. |
| `norm:stimecmp_exist` | This extension adds the S-level `stimecmp` CSR. | The extension adds the S-level `stimecmp` CSR. |
| `norm:stce_bit_exist` | This extension adds the `STCE` bit to the `menvcfg` and `henvcfg` CSRs. | The extension adds the `STCE` bit to `menvcfg` and `henvcfg`. |
| `norm:stimecmp_stimecmph_sz_acc` | The `stimecmp` CSR is a 64-bit register and has 64-bit precision on all RV32 and RV64 systems. In RV32 only, accesses to the `stimecmp` CSR access the low 32 bits, while accesses to the `stimecmph` CSR access the high 32 bits of `stimecmp`. | `stimecmp` is a 64-bit register with 64-bit precision on both RV32 and RV64. Only in RV32: accesses to `stimecmp` reach the low 32 bits and accesses to `stimecmph` reach the high 32 bits. |
| `norm:mip_sip_stip_op` | A supervisor timer interrupt becomes pending whenever `time` contains a value greater than or equal to `stimecmp`, treating the values as unsigned integers. If the result of this comparison changes, it is guaranteed to be reflected in STIP eventually, but not necessarily immediately. The interrupt remains posted until `stimecmp` becomes greater than `time`. | A supervisor timer interrupt is pending whenever `time` ≥ `stimecmp` (unsigned). Any change of the comparison result is eventually — not necessarily immediately — reflected in STIP. The interrupt remains posted until `stimecmp` > `time`. |
| `norm:sip_stip_sie_stie` | Bits `sip`.STIP and `sie`.STIE are the interrupt-pending and interrupt-enable bits for supervisor-level timer interrupts. If implemented, STIP is read-only in `sip`. When Sstc is not implemented, STIP is set and cleared by the execution environment. When Sstc is implemented, STIP reflects the timer interrupt signal resulting from `stimecmp`. | STIP/STIE are the pending/enable bits for supervisor-level timer interrupts; STIP is read-only. Without Sstc it is set/cleared by the execution environment; with Sstc it reflects the timer interrupt signal from `stimecmp`. |
| `norm:mip_stip_stimecmp_acc` | If the `stimecmp` (supervisor-mode timer compare) register is implemented, STIP is read-only in mip. | If `stimecmp` is implemented, STIP is read-only in `mip`. |
| `norm:mip_stip_stimecmp_op2` | STIP reflects the supervisor-level timer interrupt signal resulting from stimecmp. | STIP reflects the supervisor-level timer interrupt signal resulting from `stimecmp`. |
| `norm:mip_stip_stimecmp_clr` | This timer interrupt signal is cleared by writing `stimecmp` with a value greater than the current time value. | The timer interrupt signal is cleared by writing `stimecmp` with a value greater than the current time value. |
| `norm:mcounteren_tm_clr` | When the TM bit in the `mcounteren` register is clear, attempts to access the `stimecmp` or `vstimecmp` register while executing in a mode less privileged than M will cause an illegal-instruction exception. | When `mcounteren`.TM=0, accesses to `stimecmp` or `vstimecmp` from modes less privileged than M raise an illegal-instruction exception. |
| `norm:mcounteren_tm_set` | When this bit is set, access to the `stimecmp` or `vstimecmp` register is permitted in S-mode if implemented, and access to the `vstimecmp` register (via `stimecmp`) is permitted in VS-mode if implemented and not otherwise prevented by the TM bit in `hcounteren`. | When `mcounteren`.TM=1, S-mode access to `stimecmp` is permitted; VS-mode access to `vstimecmp` (via `stimecmp`) is permitted unless blocked by `hcounteren`.TM. |
| `norm:menvcfg_stce_op1` | The Sstc extension adds the `STCE` (STimecmp Enable) bit to `menvcfg` CSR. | Sstc adds the `STCE` (STimecmp Enable) bit to `menvcfg`. |
| `norm:menvcfg_stce_rdonly0` | When the Sstc extension is not implemented, `STCE` is read-only zero. | When Sstc is not implemented, `STCE` is read-only zero. |
| `norm:menvcfg_stce_op2` | The `STCE` bit enables `stimecmp` for S-mode when set to one. When this extension is implemented and `STCE` in `menvcfg` is zero, an attempt to access `stimecmp` in a mode other than M-mode raises an illegal-instruction exception, `STCE` in `henvcfg` is read-only zero, and `STIP` in `mip` and `sip` reverts to its defined behavior as if this extension is not implemented. | STCE=1 enables `stimecmp` for S-mode. With Sstc implemented but STCE=0: non-M-mode access to `stimecmp` raises illegal-instruction; `henvcfg`.STCE is read-only zero; `mip`/`sip`.STIP revert to the behavior defined when Sstc is not implemented. |

> **Migrated specification points**: `norm:sstc_vs_facility`, `norm:vstimecmp_exist`, `norm:hip_vstip_vstie_acc_op`, and `norm:henvcfg_stce` depend on the Hypervisor extension and have been migrated to `Hypervisor_cross_test_plan_en.md` Group 9.

### Out of Scope

- **RV32 / stimecmph / vstimecmph**: Only RV64 is covered; RV32 high-32-bit split access is out of scope for this plan.
- **Multi-hart scenarios**: The project is a single-core test environment.
- **SBI timer interface compatibility**: Only CSR-level behavior is verified; SBI call paths are not tested.
- **time CSR precise value verification**: The exact increment rate of `time` is not verified; only the stimecmp-to-time comparison logic is verified.
- **Spurious timer interrupt handling**: The specification allows STIP changes to be delayed ("eventually, but not necessarily immediately"); tests handle this through reasonable delays and retry mechanisms.
- **Sv32 / Sv48 / Sv57**: Only RV64 + Sv39 is covered, consistent with other extension plans in the project.

---

## Prerequisites and Constraints

### Timer Interrupt Trigger Strategy

Since the `time` register increments continuously, tests use the following strategies to reliably trigger and control timer interrupts:

1. **Trigger interrupt**: Read the current `time` value and set `stimecmp` to `time - 1` (or the current time value), ensuring the `time >= stimecmp` condition is immediately satisfied.
2. **Clear interrupt**: Set `stimecmp` to `0xFFFFFFFF_FFFFFFFF` (maximum value), ensuring the `stimecmp > time` condition is satisfied.
3. **Delay wait**: The specification allows STIP changes to not be reflected immediately; tests execute several NOP instructions after setting stimecmp before checking STIP.
4. **Interrupt capture**: Capture the S-mode timer interrupt (scause bit 63 = 1, code = 5) through the M-mode or S-mode trap handler.

### menvcfg.STCE Enable Prerequisite

All tests that require access to `stimecmp` (Groups 2–5) must first set `menvcfg.STCE` to 1 in M-mode and set `mcounteren.TM` to 1 (when tests involve S-mode access). Some test cases in Groups 1 and 3 deliberately test exception behavior when STCE=0 or TM=0.

### VS-mode Test Prerequisites — Migrated

> **[Migrated]** The former Group 6 (VS-mode related tests) has been fully migrated to `Hypervisor_cross_test_plan_en.md` Group 9.

---

## Design Points

### 1. stimecmp and time comparison semantics

Specification definition: A supervisor timer interrupt becomes pending (STIP=1) whenever the value of `time` is greater than or equal to the value of `stimecmp` (both treated as unsigned integers). The interrupt remains posted until `stimecmp` becomes greater than `time`.

Key points:
- The comparison is **unsigned**.
- STIP changes are guaranteed to be reflected **eventually**, but **not necessarily immediately**.
- STIP is **read-only** when Sstc is implemented; it cannot be cleared by writing mip/sip.
- The only way to clear the interrupt is to write `stimecmp` with a value greater than the current `time`.

### 2. mip.STIP read-only behavior

When Sstc is implemented (`menvcfg.STCE=1`), the STIP bit in `mip` becomes read-only and reflects the hardware comparison result of stimecmp and time. This contrasts with the writable STIP behavior in mip when Sstc is not implemented.

Test verification method: In M-mode, attempt to set STIP by writing `mip`, then read back to verify that the STIP value is still determined by the stimecmp-to-time comparison.

### 3. Access control hierarchy

Access to stimecmp is governed by two layers of control:

- M-mode access: always permitted.
- S-mode access: requires `menvcfg.STCE=1` **and** `mcounteren.TM=1`.
- VS-mode access (via stimecmp): requires `menvcfg.STCE=1` **and** `mcounteren.TM=1` **and** `henvcfg.STCE=1` **and** `hcounteren.TM=1`.

Exception behavior:
- `menvcfg.STCE=0` → S-mode access to stimecmp raises an **illegal-instruction** exception.
- `mcounteren.TM=0` → S-mode access to stimecmp raises an **illegal-instruction** exception.
- `henvcfg.STCE=0` → VS-mode (V=1) access to stimecmp (actually accessing vstimecmp) raises a **virtual-instruction** exception.

### 4. VSTIP synthesis logic (migrated)

`hip`.VSTIP is a synthesized read-only bit equal to the logical OR of `hvip`.VSTIP and the vstimecmp timer signal. Related test cases have been migrated to `Hypervisor_cross_test_plan_en.md` Group 9.

---

## Test Groups

### Group 1: menvcfg/henvcfg STCE Field Read/Write (M-mode CSR Read/Write)

> **[Migrated]** SSTC-STCE-03 and SSTC-STCE-04 depend on the Hypervisor extension (henvcfg CSR) and have been migrated to `Hypervisor_cross_test_plan_en.md` Group 9 (HCROSS-SSTC-01, HCROSS-SSTC-02).

**Spec Reference**:
- `norm:stce_bit_exist`: Sstc adds STCE bit to menvcfg and henvcfg.
- `norm:menvcfg_stce_op1`: Sstc adds STCE bit to menvcfg.
- `norm:menvcfg_stce_rdonly0`: STCE is read-only zero when Sstc is not implemented.

**Test Scope**: In M-mode, verify the read/write behavior of the menvcfg STCE bit. (The henvcfg portion has been migrated to `Hypervisor_cross_test_plan_en.md` Group 9.)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSTC-STCE-01 | menvcfg.STCE read-write loopback | In M-mode, write menvcfg.STCE=1 and read back to verify, then write STCE=0 and read back to verify | STCE bit written value matches read-back value |
| SSTC-STCE-02 | menvcfg.STCE does not affect other fields | Before and after writing menvcfg.STCE, verify that other set fields in menvcfg remain unchanged | Other field values remain unchanged |
| SSTC-STCE-05 | menvcfg.STCE initial value | Read the initial value of menvcfg.STCE after reset | STCE initial value is 0 (implementation-dependent, verify readability) |

---

### Group 2: stimecmp CSR Read/Write (M-mode / S-mode)

**Spec Reference**:
- `norm:stimecmp_exist`: Sstc adds S-level stimecmp CSR.
- `norm:stimecmp_stimecmph_sz_acc`: stimecmp has 64-bit precision.

**Test Scope**: Verify the 64-bit read/write behavior of the stimecmp CSR, including read/write in M-mode and S-mode (STCE=1, TM=1).

**Test Prerequisites**: menvcfg.STCE=1 (M-mode access is always permitted; S-mode access additionally requires mcounteren.TM=1).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSTC-RW-01 | M-mode stimecmp all-ones read/write | In M-mode, write stimecmp=0xFFFFFFFF_FFFFFFFF and read back | Read-back value equals written value |
| SSTC-RW-02 | M-mode stimecmp all-zeros read/write | In M-mode, write stimecmp=0 and read back | Read-back value is 0 |
| SSTC-RW-03 | M-mode stimecmp alternating bit pattern | In M-mode, write stimecmp=0x5555555555555555 and read back, then write 0xAAAAAAAAAAAAAAAA and read back | Read-back value matches written value |
| SSTC-RW-04 | M-mode stimecmp high bits verification | In M-mode, write stimecmp high 32 bits to a non-zero value (e.g., 0x12345678_00000000) and read back | High 32 bits are correctly retained |
| SSTC-RW-05 | S-mode stimecmp read/write | Set STCE=1, TM=1, switch to S-mode, write stimecmp, return to M-mode and read back to verify | S-mode written value matches M-mode read-back |
| SSTC-RW-06 | stimecmp write does not affect time | Read time before and after writing stimecmp to verify time increments monotonically and is unaffected by stimecmp | time continues to increment, independent of stimecmp |

---

### Group 3: Access Control

> **[Migrated]** SSTC-ACC-08, SSTC-ACC-09, and SSTC-ACC-10 depend on the Hypervisor extension (VS-mode access control) and have been migrated to `Hypervisor_cross_test_plan_en.md` Group 9 (HCROSS-SSTC-03, HCROSS-SSTC-04, HCROSS-SSTC-05).

**Spec Reference**:
- `norm:menvcfg_stce_op2`: When STCE=0, non-M-mode access to stimecmp raises illegal-instruction exception.
- `norm:mcounteren_tm_clr`: When mcounteren.TM=0, less-privileged mode access to stimecmp raises illegal-instruction exception.
- `norm:mcounteren_tm_set`: When mcounteren.TM=1, S-mode access to stimecmp is permitted.

**Test Scope**: Verify the multi-layer access control mechanism for stimecmp. (The VS-mode / vstimecmp portion has been migrated to `Hypervisor_cross_test_plan_en.md` Group 9.)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSTC-ACC-01 | S-mode read stimecmp with STCE=0 | menvcfg.STCE=0, S-mode reads stimecmp | Triggers illegal-instruction exception (cause=2) |
| SSTC-ACC-02 | S-mode write stimecmp with STCE=0 | menvcfg.STCE=0, S-mode writes stimecmp | Triggers illegal-instruction exception (cause=2) |
| SSTC-ACC-03 | S-mode read stimecmp with STCE=1, TM=0 | menvcfg.STCE=1, mcounteren.TM=0, S-mode reads stimecmp | Triggers illegal-instruction exception (cause=2) |
| SSTC-ACC-04 | S-mode write stimecmp with STCE=1, TM=0 | menvcfg.STCE=1, mcounteren.TM=0, S-mode writes stimecmp | Triggers illegal-instruction exception (cause=2) |
| SSTC-ACC-05 | S-mode read stimecmp with STCE=1, TM=1 | menvcfg.STCE=1, mcounteren.TM=1, S-mode reads stimecmp | No exception, read succeeds |
| SSTC-ACC-06 | S-mode write stimecmp with STCE=1, TM=1 | menvcfg.STCE=1, mcounteren.TM=1, S-mode writes stimecmp, M-mode reads back | No exception, read-back value matches |
| SSTC-ACC-07 | M-mode always has stimecmp access | With menvcfg.STCE=0, M-mode reads and writes stimecmp | No exception, read/write is normal |

---

### Group 4: Timer Interrupt Generation

**Spec Reference**:
- `norm:mip_sip_stip_op`: STIP is set when time ≥ stimecmp, cleared when stimecmp > time.
- `norm:mip_stip_stimecmp_op2`: STIP reflects the timer interrupt signal generated by stimecmp.
- `norm:mip_stip_stimecmp_clr`: Writing stimecmp greater than current time clears the timer interrupt signal.
- `norm:sstc_purpose`: Sstc provides S-mode with its own timer interrupt mechanism.

**Test Scope**: Verify the stimecmp-to-time comparison logic, and the generation and clearing of S-mode timer interrupts.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSTC-TMR-01 | STIP set when stimecmp ≤ time | Read time, write stimecmp = time - 1, check mip.STIP after delay | STIP = 1 |
| SSTC-TMR-02 | STIP cleared when stimecmp > time | First trigger STIP=1, then write stimecmp = 0xFFFFFFFF_FFFFFFFF, check mip.STIP after delay | STIP = 0 |
| SSTC-TMR-03 | stimecmp = time boundary value | Read time and immediately write stimecmp = time, check STIP after delay (since time continues incrementing, stimecmp should soon be ≤ time) | STIP = 1 |
| SSTC-TMR-04 | STIP readable in sip | After triggering STIP=1, read STIP bit from sip | sip.STIP = 1 |
| SSTC-TMR-05 | S-mode timer interrupt capture (M-mode handler) | Enable STCE=1, do not delegate STI (mideleg.STIP=0), set mie.STIE=1 and mstatus.MIE=1, set stimecmp to a past value, verify M-mode trap handler captures STI | trap cause = interrupt \| 5 |
| SSTC-TMR-06 | S-mode timer interrupt delegation to S-mode | Set mideleg bit 5 = 1, enable sie.STIE and sstatus.SIE, set stimecmp to a past value, switch to S-mode | S-mode trap handler captures cause = interrupt \| 5 |
| SSTC-TMR-07 | No interrupt when STIE=0 | Clear mie.STIE / sie.STIE, set stimecmp to a past value, verify no interrupt occurs (only STIP is set) | STIP = 1, but no interrupt is generated |
| SSTC-TMR-08 | Re-trigger after clearing interrupt | First trigger STIP=1, write stimecmp maximum value to clear, verify STIP=0; set stimecmp to a past value again | STIP is set again on second occurrence |
| SSTC-TMR-09 | Interrupt immediately clearable after stimecmp write | After triggering interrupt, write stimecmp to maximum value in trap handler, verify normal return from handler | Trap handler successfully clears interrupt and returns |
| SSTC-TMR-10 | Unsigned comparison semantics | Write stimecmp = 0x8000000000000000 (largest signed negative), verify STIP=0 when time is much less than this value | STIP = 0 (unsigned comparison: time < stimecmp) |

> [!NOTE]
> SSTC-TMR-06 requires a local S-mode trap entry: on entry it must save scause to a global variable, write stimecmp to the maximum value to clear the interrupt source (preventing re-entry), advance sepc according to the compressed-instruction width, and sret back; when `stvec.MODE=Direct`, BASE points to this entry.

---

### Group 5: STIP Read-Only Behavior

**Spec Reference**:
- `norm:mip_stip_stimecmp_acc`: When stimecmp is implemented, STIP is read-only in mip.
- `norm:sip_stip_sie_stie`: When Sstc is implemented, STIP is read-only and reflects the stimecmp comparison result.

**Test Scope**: Verify that when Sstc is implemented (STCE=1), the STIP bit in mip/sip becomes read-only and can only be controlled by modifying stimecmp.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSTC-STIP-01 | mip.STIP write ignored (write 1 when STIP=0) | STCE=1, stimecmp set to maximum value (STIP=0), attempt to write mip.STIP=1, read back mip.STIP | STIP remains 0 (write ignored) |
| SSTC-STIP-02 | mip.STIP write ignored (write 0 when STIP=1) | STCE=1, stimecmp set to past value (STIP=1), attempt to write mip.STIP=0, read back mip.STIP | STIP remains 1 (write ignored) |
| SSTC-STIP-03 | sip.STIP read-only | STCE=1, TM=1, switch to S-mode and attempt to write sip.STIP, return to M-mode and read mip.STIP | STIP value is still determined by stimecmp-to-time comparison |
| SSTC-STIP-04 | STIP controlled exclusively by stimecmp | Alternately set stimecmp to past value and maximum value, verify STIP follows the changes | STIP precisely follows the stimecmp-to-time comparison result |
| SSTC-STIP-05 | STIP reverts to writable when STCE=0 | After setting menvcfg.STCE=0, attempt to write mip.STIP=1, verify STIP can be set by software | STIP = 1 (reverts to M-mode writable behavior) |

---

## Key Notes

1. **Extension detection**: All tests must detect Sstc availability at runtime (via writability of `menvcfg.STCE`); TEST_SKIP when unavailable.

2. **STIP timing tolerance**: `norm:mip_sip_stip_op` states that STIP changes are "eventually, but not necessarily immediately" reflected. Tests should check STIP only after a reasonable delay (several NOPs + memory barrier) to avoid timing-related misjudgments.

3. **Interrupt re-entry protection**: The S-mode timer interrupt handler must first write `stimecmp` to the maximum value to clear the interrupt source, then execute `sret`; otherwise it will immediately re-enter.

4. **mideleg configuration**: SSTC-TMR-05 (M-mode capture) requires mideleg bit 5 = 0; SSTC-TMR-06 (S-mode capture) requires mideleg bit 5 = 1. Save/restore mideleg before and after the tests.

5. **Hypervisor tests**: Tests requiring the Hypervisor extension (`henvcfg.STCE`, `vstimecmp`, `hip.VSTIP`, etc.) have been migrated to `Hypervisor_cross_test_plan_en.md` Group 9.

If any platform violates the SPEC, the corresponding case remains FAIL, and implementation defects are recorded to the `bugs/` directory.

---

## Appendix A: Specification Point Coverage Matrix

| Norm ID | Covered Test IDs | Coverage Status | Notes |
|---------|------------------|-----------------|-------|
| `norm:sstc_purpose` | SSTC-TMR-01 ~ SSTC-TMR-10 | Covered | S-mode-owned timer interrupt facility |
| `norm:stimecmp_exist` | SSTC-RW-01 ~ SSTC-RW-06, SSTC-ACC-05 ~ SSTC-ACC-07 | Covered | stimecmp CSR existence |
| `norm:stce_bit_exist` | SSTC-STCE-01, SSTC-STCE-02, SSTC-STCE-05 | Covered | menvcfg.STCE bit existence |
| `norm:stimecmp_stimecmph_sz_acc` | SSTC-RW-01 ~ SSTC-RW-04 | Covered | 64-bit precision read/write |
| `norm:mip_sip_stip_op` | SSTC-TMR-01 ~ SSTC-TMR-04, SSTC-TMR-08, SSTC-TMR-10, SSTC-STIP-04 | Covered | STIP vs. time/stimecmp comparison relationship |
| `norm:sip_stip_sie_stie` | SSTC-TMR-04, SSTC-TMR-06, SSTC-STIP-03 | Covered | STIP read-only, STIE enable semantics |
| `norm:mip_stip_stimecmp_acc` | SSTC-STIP-01, SSTC-STIP-02, SSTC-STIP-05 | Covered | mip.STIP read-only behavior |
| `norm:mip_stip_stimecmp_op2` | SSTC-STIP-01, SSTC-STIP-02, SSTC-STIP-04 | Covered | STIP reflects stimecmp signal |
| `norm:mip_stip_stimecmp_clr` | SSTC-TMR-02, SSTC-TMR-08, SSTC-TMR-09 | Covered | Writing stimecmp clears interrupt signal |
| `norm:mcounteren_tm_clr` | SSTC-ACC-03, SSTC-ACC-04 | Covered | TM=0 makes access illegal |
| `norm:mcounteren_tm_set` | SSTC-ACC-05, SSTC-ACC-06, SSTC-RW-05 | Covered | TM=1 permits S-mode access |
| `norm:menvcfg_stce_op1` | SSTC-STCE-01, SSTC-STCE-02 | Covered | menvcfg.STCE bit definition |
| `norm:menvcfg_stce_rdonly0` | SSTC-STCE-05 | Covered | STCE read-only zero when Sstc not implemented (via extension probe) |
| `norm:menvcfg_stce_op2` | SSTC-ACC-01, SSTC-ACC-02, SSTC-STIP-05 | Covered | Illegal access and STIP behavior reversion when STCE=0 |

**Uncovered specification points**:
- Hypervisor-related specification points (`norm:sstc_vs_facility`, `norm:vstimecmp_exist`, `norm:hip_vstip_vstie_acc_op`, `norm:henvcfg_stce`) have been migrated to `Hypervisor_cross_test_plan_en.md` Group 9; this plan does not duplicate coverage.
