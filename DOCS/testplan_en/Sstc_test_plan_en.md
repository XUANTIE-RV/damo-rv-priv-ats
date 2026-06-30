**[中文](../testplan/Sstc_test_plan.md) | English**

# Sstc Extension Test Plan

This document describes the test plan for the Sstc (Supervisor-mode Timer Interrupts, Version 1.0) extension. Sstc provides S-mode with its own CSR-based timer interrupt mechanism (`stimecmp` register), enabling S-mode to directly manage its own timer service without proxying through M-mode via SBI calls. Additionally, this extension provides a similar timer mechanism (`vstimecmp` register) for VS-mode under the Hypervisor extension, and introduces new `STCE` control bits in `menvcfg` and `henvcfg`.

---

## Test Scope

### Specification Source

- `SPEC/sstc.adoc` -- Sstc Extension for Supervisor-mode Timer Interrupts, Version 1.0
- `SPEC/supervisor.adoc:1158-1192` -- Supervisor Timer (`stimecmp`) Register definition and STIP behavior
- `SPEC/supervisor.adoc:430-440` -- `sip`.STIP / `sie`.STIE behavior under Sstc
- `SPEC/machine.adoc:1505-1515` -- `mip`.STIP behavior change when stimecmp is implemented
- `SPEC/machine.adoc:1630-1645` -- `mcounteren`.TM access control for stimecmp/vstimecmp
- `SPEC/machine.adoc:2265-2280` -- `menvcfg`.STCE field definition
- `SPEC/hypervisor.adoc:575-585` -- `hip`.VSTIP relationship with vstimecmp
- `SPEC/hypervisor.adoc:745-755` -- `henvcfg`.STCE field definition
- `SPEC/csrs.adoc:373-376` -- stimecmp/stimecmph CSR addresses (0x14D/0x15D)
- `SPEC/csrs.adoc:622-625` -- vstimecmp/vstimecmph CSR addresses (0x24D/0x25D)

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

### Key Reference Files

| Path | Description |
|------|-------------|
| `SPEC/sstc.adoc` | Sstc specification full text |
| `SPEC/supervisor.adoc` | stimecmp register definition and STIP behavior |
| `SPEC/machine.adoc` | menvcfg.STCE, mcounteren.TM, mip.STIP behavior |
| `SPEC/hypervisor.adoc` | henvcfg.STCE, hip.VSTIP, vstimecmp behavior |
| `common/test_framework.h` | Test framework (TEST_BEGIN / TEST_ASSERT / TEST_END) |
| `common/encoding.h` | CSR address definitions (stimecmp/vstimecmp/STCE need to be added) |
| `common/csr_accessors.c` | Dynamic CSR read/write (stimecmp/vstimecmp need to be added) |
| `common/trap.c` | Trap handler (must support S-mode timer interrupt) |

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:sstc_purpose` | This extension serves to provide supervisor mode with its own CSR-based timer interrupt facility that it can directly manage to provide its own timer service (in the form of having its own `stimecmp` register) - thus eliminating the large overheads for emulating S/HS-mode timers and timer interrupt generation up in M-mode. |
| `norm:sstc_vs_facility` | Further, this extension adds a similar facility to the Hypervisor extension for VS-mode. |
| `norm:stimecmp_exist` | This extension adds the S-level `stimecmp` CSR. |
| `norm:vstimecmp_exist` | This extension adds the VS-level `vstimecmp` CSR. |
| `norm:stce_bit_exist` | This extension adds the `STCE` bit to the `menvcfg` and `henvcfg` CSRs. |
| `norm:stimecmp_stimecmph_sz_acc` | The `stimecmp` CSR is a 64-bit register and has 64-bit precision on all RV32 and RV64 systems. In RV32 only, accesses to the `stimecmp` CSR access the low 32 bits, while accesses to the `stimecmph` CSR access the high 32 bits of `stimecmp`. |
| `norm:mip_sip_stip_op` | A supervisor timer interrupt becomes pending whenever `time` contains a value greater than or equal to `stimecmp`, treating the values as unsigned integers. If the result of this comparison changes, it is guaranteed to be reflected in STIP eventually, but not necessarily immediately. The interrupt remains posted until `stimecmp` becomes greater than `time`. |
| `norm:sip_stip_sie_stie` | Bits `sip`.STIP and `sie`.STIE are the interrupt-pending and interrupt-enable bits for supervisor-level timer interrupts. If implemented, STIP is read-only in `sip`. When Sstc is not implemented, STIP is set and cleared by the execution environment. When Sstc is implemented, STIP reflects the timer interrupt signal resulting from `stimecmp`. |
| `norm:mip_stip_stimecmp_acc` | If the `stimecmp` (supervisor-mode timer compare) register is implemented, STIP is read-only in mip |
| `norm:mip_stip_stimecmp_op2` | reflects the supervisor-level timer interrupt signal resulting from stimecmp. |
| `norm:mip_stip_stimecmp_clr` | This timer interrupt signal is cleared by writing `stimecmp` with a value greater than the current time value. |
| `norm:mcounteren_tm_clr` | when the TM bit in the `mcounteren` register is clear, attempts to access the `stimecmp` or `vstimecmp` register while executing in a mode less privileged than M will cause an illegal-instruction exception. |
| `norm:mcounteren_tm_set` | When this bit is set, access to the `stimecmp` or `vstimecmp` register is permitted in S-mode if implemented, and access to the `vstimecmp` register (via `stimecmp`) is permitted in VS-mode if implemented and not otherwise prevented by the TM bit in `hcounteren`. |
| `norm:menvcfg_stce_op1` | The Sstc extension adds the `STCE` (STimecmp Enable) bit to `menvcfg` CSR. |
| `norm:menvcfg_stce_rdonly0` | When the Sstc extension is not implemented, `STCE` is read-only zero. |
| `norm:menvcfg_stce_op2` | The `STCE` bit enables `stimecmp` for S-mode when set to one. When this extension is implemented and `STCE` in `menvcfg` is zero, an attempt to access `stimecmp` in a mode other than M-mode raises an illegal-instruction exception, `STCE` in `henvcfg` is read-only zero, and `STIP` in `mip` and `sip` reverts to its defined behavior as if this extension is not implemented. Further, if the H extension is implemented, then `hip`.VSTIP also reverts its defined behavior as if this extension is not implemented. |
| `norm:hip_vstip_vstie_acc_op` | Bits `hip`.VSTIP and `hie`.VSTIE are the interrupt-pending and interrupt-enable bits for VS-level timer interrupts. VSTIP is read-only in `hip`, and is the logical-OR of `hvip`.VSTIP and, when the Sstc extension is implemented, the timer interrupt signal resulting from `vstimecmp`. |
| `norm:henvcfg_stce` | The Sstc extension adds the STCE (STimecmp Enable) bit to `henvcfg` CSR. When the Sstc extension is not implemented, STCE is read-only zero. The STCE bit enables `vstimecmp` for VS-mode when set to one. When STCE is zero, an attempt to access `stimecmp` (really `vstimecmp`) when V=1 raises a virtual-instruction exception, and VSTIP in `hip` reverts to its defined behavior as if this extension is not implemented. |

### Out of Scope

- **RV32 / stimecmph / vstimecmph**: Only RV64 is covered; RV32 high-32-bit split access is out of scope for this plan
- **Multi-hart scenarios**: The project is a single-core test environment
- **SBI timer interface compatibility**: Only CSR-level behavior is verified; SBI call paths are not tested
- **time CSR precise value verification**: The exact increment rate of `time` is not verified; only the stimecmp-to-time comparison logic is verified
- **Spurious timer interrupt handling**: The specification allows STIP changes to be delayed ("eventually, but not necessarily immediately"); tests handle this through reasonable delays and retry mechanisms
- **Sv32 / Sv48 / Sv57**: Only RV64 + Sv39 is covered, consistent with other extension plans in the project

---

## Prerequisites and Constraints

> [!IMPORTANT]
> The following framework-level support must be added before implementing test code:
> 1. `common/encoding.h` must add `CSR_STIMECMP` (0x14D), `CSR_STIMECMPH` (0x15D), `CSR_VSTIMECMP` (0x24D), `CSR_VSTIMECMPH` (0x25D) address definitions, as well as `MENVCFG_STCE` (`1ULL << 63`) and `HENVCFG_STCE` (`1ULL << 63`) field masks
> 2. `common/csr_accessors.c` must add dynamic read/write cases for stimecmp/vstimecmp
> 3. `common/trap.c` must correctly identify S-mode timer interrupt (cause = interrupt | 5, i.e., `0x8000000000000005`)
> 4. The top-level `Makefile` `EXTENSIONS` list must add an `sstc` entry

### Timer Interrupt Trigger Strategy

Since the `time` register increments continuously, tests use the following strategies to reliably trigger and control timer interrupts:

1. **Trigger interrupt**: Read the current `time` value and set `stimecmp` to `time - 1` (or the current time value), ensuring the `time >= stimecmp` condition is immediately satisfied
2. **Clear interrupt**: Set `stimecmp` to `0xFFFFFFFF_FFFFFFFF` (maximum value), ensuring the `stimecmp > time` condition is satisfied
3. **Delay wait**: The specification allows STIP changes to not be reflected immediately; tests execute several NOP instructions after setting stimecmp before checking STIP
4. **Interrupt capture**: Capture S-mode timer interrupt (scause bit 63 = 1, code = 5) through the M-mode trap handler or S-mode trap handler

### menvcfg.STCE Enable Prerequisite

All tests that require access to `stimecmp` (Groups 2-5) must first set `menvcfg.STCE` to 1 in M-mode and set `mcounteren.TM` to 1 (when tests involve S-mode access). Some test cases in Groups 1 and 3 deliberately test exception behavior when STCE=0 or TM=0.

### VS-mode Test Prerequisites (Group 6) -- Migrated

> **[Migrated]** All Group 6 tests have been migrated to `Hypervisor_cross_test_plan.md` Group 9 and are implemented in the `Hypervisor_Sstc/` subproject. The following prerequisite description is retained for reference.

Group 6 requires H extension support. Tests should determine whether to skip by checking the `misa.H` bit at runtime. VS-mode related tests additionally require `henvcfg.STCE=1` and `hcounteren.TM=1`.

---

## Design Points

### 1. stimecmp and time comparison semantics

Specification definition: A supervisor timer interrupt becomes pending (STIP=1) whenever the value of `time` is greater than or equal to the value of `stimecmp` (both treated as unsigned integers). The interrupt remains posted until `stimecmp` becomes greater than `time`.

Key points:
- The comparison is **unsigned**
- STIP changes are guaranteed to be reflected **eventually**, but **not necessarily immediately**
- STIP is **read-only** (when Sstc is implemented); it cannot be cleared by writing mip/sip
- The only way to clear the interrupt is to write `stimecmp` with a value greater than the current `time`

### 2. mip.STIP read-only behavior

When Sstc is implemented (`menvcfg.STCE=1`), the STIP bit in `mip` becomes read-only and reflects the hardware comparison result of stimecmp and time. This contrasts with the writable STIP behavior in mip when Sstc is not implemented.

Test verification method: In M-mode, attempt to set STIP via `csrs mip, STIP`, then read back to verify that the STIP value is still determined by the stimecmp-to-time comparison.

### 3. Access control hierarchy

Access to stimecmp is governed by two layers of control:

```
M-mode access: always permitted
S-mode access: requires menvcfg.STCE=1 AND mcounteren.TM=1
VS-mode access (via stimecmp): requires menvcfg.STCE=1 AND mcounteren.TM=1 AND henvcfg.STCE=1 AND hcounteren.TM=1
```

- `menvcfg.STCE=0` -> S-mode access to stimecmp raises **illegal-instruction exception**
- `mcounteren.TM=0` -> S-mode access to stimecmp raises **illegal-instruction exception**
- `henvcfg.STCE=0` -> VS-mode (V=1) access to stimecmp (actually accessing vstimecmp) raises **virtual-instruction exception**

### 4. VSTIP synthesis logic

`hip`.VSTIP is a synthesized read-only bit:
```
hip.VSTIP = hvip.VSTIP | vstimecmp_timer_signal
```
where `vstimecmp_timer_signal` is 1 when `(time + htimedelta) >= vstimecmp`.

---

## Test Groups

### Group 1: menvcfg/henvcfg STCE Field Read/Write (M-mode CSR Read/Write)

> **[Migrated]** SSTC-STCE-03 and SSTC-STCE-04 depend on the Hypervisor extension (henvcfg CSR) and have been migrated to `Hypervisor_cross_test_plan.md` Group 9 (HCROSS-SSTC-01, HCROSS-SSTC-02), with corresponding code moved to the `Hypervisor_Sstc/` subproject.

**Spec Reference**:
- `norm:stce_bit_exist`: Sstc adds STCE bit to menvcfg and henvcfg
- `norm:menvcfg_stce_op1`: Sstc adds STCE bit to menvcfg
- `norm:menvcfg_stce_rdonly0`: STCE is read-only zero when Sstc is not implemented
- ~~`norm:henvcfg_stce`: henvcfg.STCE enables vstimecmp~~ (Migrated to Hypervisor_cross_test_plan.md)

**Test Scope**: In M-mode, verify the read/write behavior of the menvcfg STCE bit. (henvcfg portion has been migrated to Hypervisor_Sstc)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSTC-STCE-01 | menvcfg.STCE read-write loopback | In M-mode, write menvcfg.STCE=1 and read back to verify, then write STCE=0 and read back to verify | STCE bit written value matches read-back value |
| SSTC-STCE-02 | menvcfg.STCE does not affect other fields | Before and after writing menvcfg.STCE, verify that other set fields in menvcfg remain unchanged | Other field values remain unchanged |
| ~~SSTC-STCE-03~~ | ~~henvcfg.STCE read-write loopback~~ | ~~Migrated to `Hypervisor_cross_test_plan.md` HCROSS-SSTC-01, code moved to `Hypervisor_Sstc/`~~ | -- |
| ~~SSTC-STCE-04~~ | ~~henvcfg.STCE constrained by menvcfg.STCE~~ | ~~Migrated to `Hypervisor_cross_test_plan.md` HCROSS-SSTC-02, code moved to `Hypervisor_Sstc/`~~ | -- |
| SSTC-STCE-05 | menvcfg.STCE initial value | Read the initial value of menvcfg.STCE after reset | STCE initial value is 0 (implementation-dependent, verify readability) |

---

### Group 2: stimecmp CSR Read/Write (M-mode / S-mode)

**Spec Reference**:
- `norm:stimecmp_exist`: Sstc adds S-level stimecmp CSR
- `norm:stimecmp_stimecmph_sz_acc`: stimecmp has 64-bit precision

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

> **[Migrated]** SSTC-ACC-08, SSTC-ACC-09, SSTC-ACC-10 depend on the Hypervisor extension (VS-mode access control) and have been migrated to `Hypervisor_cross_test_plan.md` Group 9 (HCROSS-SSTC-03, HCROSS-SSTC-04, HCROSS-SSTC-05), with corresponding code moved to the `Hypervisor_Sstc/` subproject.

**Spec Reference**:
- `norm:menvcfg_stce_op2`: When STCE=0, non-M-mode access to stimecmp raises illegal-instruction exception
- `norm:mcounteren_tm_clr`: When mcounteren.TM=0, less-privileged mode access to stimecmp raises illegal-instruction exception
- `norm:mcounteren_tm_set`: When mcounteren.TM=1, S-mode access to stimecmp is permitted
- ~~`norm:henvcfg_stce`: When henvcfg.STCE=0, V=1 access to stimecmp raises virtual-instruction exception~~ (Migrated to Hypervisor_cross_test_plan.md)

**Test Scope**: Verify the multi-layer access control mechanism for stimecmp. (VS-mode / vstimecmp portion has been migrated to Hypervisor_Sstc)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSTC-ACC-01 | S-mode read stimecmp with STCE=0 | menvcfg.STCE=0, S-mode reads stimecmp | Triggers illegal-instruction exception (cause=2) |
| SSTC-ACC-02 | S-mode write stimecmp with STCE=0 | menvcfg.STCE=0, S-mode writes stimecmp | Triggers illegal-instruction exception (cause=2) |
| SSTC-ACC-03 | S-mode read stimecmp with STCE=1, TM=0 | menvcfg.STCE=1, mcounteren.TM=0, S-mode reads stimecmp | Triggers illegal-instruction exception (cause=2) |
| SSTC-ACC-04 | S-mode write stimecmp with STCE=1, TM=0 | menvcfg.STCE=1, mcounteren.TM=0, S-mode writes stimecmp | Triggers illegal-instruction exception (cause=2) |
| SSTC-ACC-05 | S-mode read stimecmp with STCE=1, TM=1 | menvcfg.STCE=1, mcounteren.TM=1, S-mode reads stimecmp | No exception, read succeeds |
| SSTC-ACC-06 | S-mode write stimecmp with STCE=1, TM=1 | menvcfg.STCE=1, mcounteren.TM=1, S-mode writes stimecmp, M-mode reads back | No exception, read-back value matches |
| SSTC-ACC-07 | M-mode always has stimecmp access | With menvcfg.STCE=0, M-mode reads and writes stimecmp | No exception, read/write is normal |
| ~~SSTC-ACC-08~~ | ~~VS-mode access to stimecmp with henvcfg.STCE=0~~ | ~~Migrated to `Hypervisor_cross_test_plan.md` HCROSS-SSTC-03, code moved to `Hypervisor_Sstc/`~~ | -- |
| ~~SSTC-ACC-09~~ | ~~VS-mode access to stimecmp with henvcfg.STCE=1~~ | ~~Migrated to `Hypervisor_cross_test_plan.md` HCROSS-SSTC-04, code moved to `Hypervisor_Sstc/`~~ | -- |
| ~~SSTC-ACC-10~~ | ~~VS-mode access to stimecmp with hcounteren.TM=0~~ | ~~Migrated to `Hypervisor_cross_test_plan.md` HCROSS-SSTC-05, code moved to `Hypervisor_Sstc/`~~ | -- |

> [!NOTE]
> ~~SSTC-ACC-08/09/10 require H extension support; skip if `misa.H` is not set. Accessing the `stimecmp` CSR in VS-mode is actually accessing `vstimecmp` (transparently remapped by hardware).~~ Migrated to Hypervisor_cross_test_plan.md Group 9.

---

### Group 4: Timer Interrupt Generation

**Spec Reference**:
- `norm:mip_sip_stip_op`: STIP is set when time >= stimecmp, cleared when stimecmp > time
- `norm:mip_stip_stimecmp_op2`: STIP reflects the timer interrupt signal generated by stimecmp
- `norm:mip_stip_stimecmp_clr`: Writing stimecmp greater than current time clears the timer interrupt signal
- `norm:sstc_purpose`: Sstc provides S-mode with its own timer interrupt mechanism

**Test Scope**: Verify the stimecmp-to-time comparison logic, and the generation and clearing of S-mode timer interrupts.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSTC-TMR-01 | STIP set when stimecmp <= time | Read time, write stimecmp = time - 1, check mip.STIP after delay | STIP = 1 |
| SSTC-TMR-02 | STIP cleared when stimecmp > time | First trigger STIP=1, then write stimecmp = 0xFFFFFFFF_FFFFFFFF, check mip.STIP after delay | STIP = 0 |
| SSTC-TMR-03 | stimecmp = time boundary value | Read time and immediately write stimecmp = time, check STIP after delay (since time continues incrementing, stimecmp should soon be <= time) | STIP = 1 |
| SSTC-TMR-04 | STIP readable in sip | After triggering STIP=1, read STIP bit from sip | sip.STIP = 1 |
| SSTC-TMR-05 | S-mode timer interrupt capture (M-mode handler) | Enable STCE=1, do not delegate STI (mideleg.STIP=0), set mie.STIE=1 and mstatus.MIE=1, set stimecmp to a past value, verify M-mode trap handler captures STI | trap cause = interrupt \| 5 (0x8000000000000005) |
| SSTC-TMR-06 | S-mode timer interrupt delegation to S-mode | Set mideleg bit 5 = 1, enable sie.STIE, sstatus.SIE, set stimecmp to a past value, switch to S-mode | S-mode trap handler captures cause = interrupt \| 5 |
| SSTC-TMR-07 | No interrupt when STIE=0 | Clear mie.STIE / sie.STIE, set stimecmp to a past value, verify no interrupt occurs (only STIP is set) | STIP = 1, but no interrupt is generated |
| SSTC-TMR-08 | Re-trigger after clearing interrupt | First trigger STIP=1, write stimecmp maximum value to clear, verify STIP=0; set stimecmp to a past value again | STIP is set again on second occurrence |
| SSTC-TMR-09 | Interrupt immediately clearable after stimecmp write | After triggering interrupt, write stimecmp to maximum value in trap handler, verify normal return from handler | Trap handler successfully clears interrupt and returns |
| SSTC-TMR-10 | Unsigned comparison semantics | Write stimecmp = 0x8000000000000000 (largest signed negative), verify STIP=0 when time is much less than this value | STIP = 0 (unsigned comparison: time < stimecmp) |

---

### Group 5: STIP Read-Only Behavior

**Spec Reference**:
- `norm:mip_stip_stimecmp_acc`: When stimecmp is implemented, STIP is read-only in mip
- `norm:sip_stip_sie_stie`: When Sstc is implemented, STIP is read-only and reflects the stimecmp comparison result

**Test Scope**: Verify that when Sstc is implemented (STCE=1), the STIP bit in mip/sip becomes read-only and can only be controlled by modifying stimecmp.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSTC-STIP-01 | mip.STIP write ignored (write 1 when STIP=0) | STCE=1, stimecmp set to maximum value (STIP=0), attempt csrs mip STIP, read back mip.STIP | STIP remains 0 (write ignored) |
| SSTC-STIP-02 | mip.STIP write ignored (write 0 when STIP=1) | STCE=1, stimecmp set to past value (STIP=1), attempt csrc mip STIP, read back mip.STIP | STIP remains 1 (write ignored) |
| SSTC-STIP-03 | sip.STIP read-only | STCE=1, TM=1, switch to S-mode and attempt to write sip.STIP, return to M-mode and read mip.STIP | STIP value is still determined by stimecmp-to-time comparison |
| SSTC-STIP-04 | STIP controlled exclusively by stimecmp | Alternately set stimecmp to past value and maximum value, verify STIP follows the changes | STIP precisely follows the stimecmp-to-time comparison result |
| SSTC-STIP-05 | STIP reverts to writable when STCE=0 | After setting menvcfg.STCE=0, attempt csrs mip STIP, verify STIP can be set by software | STIP = 1 (reverts to M-mode writable behavior) |

---

## Framework Modification Requirements

> [!WARNING]
> The following modifications are prerequisites for implementing test code and must be completed before writing test cases.

### 1. `common/encoding.h` New Definitions

### 2. `common/csr_accessors.c` New Cases

The following CSR cases must be added to the dynamic CSR read/write function:
- `CSR_STIMECMP` (0x14D)
- `CSR_VSTIMECMP` (0x24D)

### 3. `common/trap.c` Interrupt Identification

Ensure the M-mode trap handler can correctly identify and record S-mode timer interrupt:
- cause = `0x8000000000000005` (interrupt bit | code 5)
- The handler must clear the interrupt source (write stimecmp to maximum value) to prevent interrupt re-entry

### 4. New `sstc/` Subdirectory

```
sstc/
├── Makefile          # Reference svadu/Makefile, SPIKE_ISA_EXT = _sstc
├── kernel.ld         # Reuse common/kernel_common.ld or reference other extensions
├── main.c            # Test entry point, contains test_table traversal logic
└── tests/
    ├── test_register.c     # All TEST_REGISTER declarations
    ├── test_stce.c         # Group 1 tests
    ├── test_rw.c           # Group 2 tests
    ├── test_access.c       # Group 3 tests
    ├── test_timer.c        # Group 4 tests
    └── test_stip.c         # Group 5 tests
```

### 5. Top-Level `Makefile`

Add `sstc` to the `EXTENSIONS` list:
