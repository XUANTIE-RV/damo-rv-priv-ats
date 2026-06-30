**[中文](../testplan/Shcounterenw_test_plan.md) | English**

# Shcounterenw Test Plan

## Overview

This test plan covers the **Shcounterenw** extension, which makes `hcounteren` writable in HS-mode. The extension allows hypervisors to control VS/VU-mode access to performance counters by setting/clearing bits in `hcounteren`.

## Test Scope

- Verify that `hcounteren` is writable (WARL behavior)
- Verify VS-mode counter access control based on `hcounteren` bits
- Verify toggle consistency of `hcounteren` bits
- Verify hierarchical interaction between `mcounteren`, `hcounteren`, and `scounteren`
- Report behavior of `hcounteren` bits for read-only zero counters

## Specification Sources

- RISC-V Hypervisor Extension Specification (`SPEC/hypervisor.adoc`)
- Shcounterenw Extension Specification

## Key Reference Files

- `SPEC/hypervisor.adoc:856-876` — hcounteren definition and WARL behavior
- `SPEC/hypervisor.adoc:2359-2361` — VU-mode counter access requirements
- `common/encoding.h` — CSR address definitions

## Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| norm:shcounterenw_hpmcounter_hcounteren | hcounteren is writable; any bit can be toggled between 0 and 1 |
| norm:hcounteren_op | mcounteren still gates VS/VU-mode access |
| norm:hcounteren_warl | Any bit of hcounteren can be read-only zero |
| norm:H_virtinst_vu_nonhighctr_h0_s0_m1 | VU-mode requires both hcounteren and scounteren to be 1 |

## Out of Scope

- Testing of unimplemented hpmcounters beyond platform-specific implementation
- Performance impact measurement of counter access
- Multi-core synchronization scenarios

## Design Key Points

### Group 1: Writability Verification

**Specification Basis**:
- `norm:shcounterenw_hpmcounter_hcounteren`: hcounteren is writable

**Test Responsibility**: Verify that hcounteren bits can be written and read back correctly in M-mode.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SHCNTW-WR-01 | Write hcounteren[0] and read back | Set hcounteren[0]=1, read back | Read value matches written value |
| SHCNTW-WR-02 | Clear hcounteren[0] and read back | Set hcounteren[0]=0, read back | Read value matches written value |
| SHCNTW-WR-03 | Write all implemented bits | Set all implemented hcounteren bits to 1, read back | All implemented bits read as 1 |
| SHCNTW-WR-04 | Write pattern to hpmcounterN bits | Write alternating pattern to hpmcounterN bits, read back | Pattern preserved for implemented counters |

---

### Group 2: VS-mode Access Control

**Specification Basis**:
- `norm:hcounteren_op`: hcounteren controls VS-mode counter access

**Test Responsibility**: Verify that VS-mode counter access is properly gated by hcounteren bits.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SHCNTW-ACCESS-01 | VS-mode reads cycle when hcounteren[0]=1 | Set mcounteren[0]=1, hcounteren[0]=1, VS-mode reads cycle | No exception |
| SHCNTW-ACCESS-02 | VS-mode reads cycle when hcounteren[0]=0 triggers exception | Set mcounteren[0]=1, hcounteren[0]=0, VS-mode reads cycle | Triggers virtual-instruction exception (cause=22) |
| SHCNTW-ACCESS-03 | VS-mode reads time when hcounteren[1]=1 | Set mcounteren[1]=1, hcounteren[1]=1, VS-mode reads time | No exception |
| SHCNTW-ACCESS-04 | VS-mode reads time when hcounteren[1]=0 triggers exception | Set mcounteren[1]=1, hcounteren[1]=0, VS-mode reads time | Triggers virtual-instruction exception (cause=22) |
| SHCNTW-ACCESS-05 | VS-mode reads instret when hcounteren[2]=1 | Set mcounteren[2]=1, hcounteren[2]=1, VS-mode reads instret | No exception |
| SHCNTW-ACCESS-06 | VS-mode reads instret when hcounteren[2]=0 triggers exception | Set mcounteren[2]=1, hcounteren[2]=0, VS-mode reads instret | Triggers virtual-instruction exception (cause=22) |
| SHCNTW-ACCESS-07 | VS-mode reads hpmcounterN when hcounteren[N]=1 | For implemented hpmcounterN, set corresponding bit=1, VS-mode reads | No exception |
| SHCNTW-ACCESS-08 | VS-mode reads hpmcounterN when hcounteren[N]=0 triggers exception | For implemented hpmcounterN, set corresponding bit=0, VS-mode reads | Triggers virtual-instruction exception (cause=22) |

---

### Group 3: hcounteren Bit Toggle Consistency

**Specification Basis**:
- `norm:shcounterenw_hpmcounter_hcounteren`: Writability implies bits can be toggled between 0 and 1 repeatedly

**Test Responsibility**: For implemented counters, repeatedly set and clear hcounteren bits, verifying VS-mode access behavior remains consistent after each toggle.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SHCNTW-TOGGLE-01 | Toggle cycle bit repeatedly | Toggle hcounteren[0] 1→0→1→0, verify VS-mode behavior each time | Behavior matches current bit value after each toggle |
| SHCNTW-TOGGLE-02 | Toggle instret bit repeatedly | Toggle hcounteren[2] 1→0→1→0, verify VS-mode behavior each time | Behavior matches current bit value after each toggle |
| SHCNTW-TOGGLE-03 | Toggle hpmcounterN bit repeatedly | Toggle implemented hpmcounterN bits, verify behavior | Behavior matches current bit value after each toggle |

---

### Group 4: mcounteren / hcounteren / scounteren Hierarchical Interaction

**Specification Basis**:
- `norm:hcounteren_op` (`SPEC/hypervisor.adoc:856-864`): mcounteren still gates VS/VU-mode access
- `norm:H_virtinst_vu_nonhighctr_h0_s0_m1` (`SPEC/hypervisor.adoc:2359-2361`): VU-mode requires both hcounteren and scounteren to be 1

**Test Responsibility**: Verify hierarchical gating relationship: when mcounteren=0, access is blocked even if hcounteren=1; VU-mode requires all three levels to be 1.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SHCNTW-HIER-01 | VS-mode reads cycle when mcounteren[0]=0 triggers exception | mcounteren[0]=0, hcounteren[0]=1, VS-mode reads cycle | Triggers illegal-instruction exception (cause=2) |
| SHCNTW-HIER-02 | VS-mode reads cycle when mcounteren[0]=1, hcounteren[0]=1 | Both levels enabled, VS-mode reads cycle | No exception |
| SHCNTW-HIER-03 | VU-mode reads cycle when mcounteren=1, hcounteren=1, scounteren=1 | All three levels enabled, VU-mode reads cycle | No exception |
| SHCNTW-HIER-04 | VU-mode reads cycle when mcounteren=1, hcounteren=0, scounteren=1 triggers exception | hcounteren blocks, VU-mode reads cycle | Triggers virtual-instruction exception (cause=22) |
| SHCNTW-HIER-05 | VU-mode reads cycle when mcounteren=1, hcounteren=1, scounteren=0 triggers exception | scounteren blocks, VU-mode reads cycle | Triggers virtual-instruction exception (cause=22) |

> [!NOTE]
> **Exception type for SHCNTW-HIER-01**: When `mcounteren` bit=0, regardless of `hcounteren` setting, VS-mode counter access triggers **illegal-instruction exception (cause=2)** rather than virtual-instruction exception. This is because mcounteren=0 means "the counter is invisible below S-mode", and the H extension does not intervene (not treated as hypervisor-level virtualization interception).

> [!NOTE]
> **Exception type for SHCNTW-HIER-04/05**: During VU-mode access, if `hcounteren` or `scounteren` corresponding bit=0 (with mcounteren=1), both trigger virtual-instruction exception (cause=22), because the trap semantics indicate "hypervisor intercepted guest's counter access".

> [!IMPORTANT]
> **SHCNTW-HIER-04/05 Prerequisite: scounteren Writability Verification**
> SHCNTW-HIER-05 depends on `scounteren` bit 0 being clearable. However, Shcounterenw only guarantees **hcounteren** writability; `scounteren` writability is determined by Smcntrpmf or platform policy. Test implementation must before executing HIER-04/05:
> 1. Attempt to write `scounteren[0] = 0` and verify via readback
> 2. If `scounteren[0]` cannot be cleared (read-only 1), SHCNTW-HIER-05 should execute `TEST_SKIP("scounteren[0] is not writable on this platform")`
> 3. SHCNTW-HIER-04 is unaffected (it relies on hcounteren=0 blocking, scounteren=1 is default value)

---

### Group 5: hcounteren Bit Behavior for Read-Only Zero Counters

**Specification Basis**:
- `norm:hcounteren_warl` (`SPEC/hypervisor.adoc:873-876`): Any bit of hcounteren can be read-only zero
- Shcounterenw **does not constrain** hcounteren bits corresponding to read-only zero counters

**Test Responsibility**: For hpmcounters detected as read-only zero, record their hcounteren bit behavior (may be read-only 0 or writable), without pass/fail judgment, for informational purposes only.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SHCNTW-RO-01 | Probe hcounteren bits for read-only zero counters | For all read-only zero hpmcounters, attempt to write hcounteren bits and report results | Informational output, no pass/fail |

---

## Appendix: Related Constants and API Reference

### CSR and Bit Definitions

| Name | Value | Description |
|------|-------|-------------|
| `CSR_HCOUNTEREN` | `0x606` | Hypervisor counter-enable register, see `common/encoding.h:275` |
| `CSR_MCOUNTEREN` | `0x306` | Machine counter-enable register, see `common/encoding.h:15` |
| `CSR_SCOUNTEREN` | `0x106` | Supervisor counter-enable register, see `common/encoding.h:139` |
| `CSR_CYCLE` | `0xC00` | Read-only cycle counter (user-level) |
| `CSR_TIME` | `0xC01` | Read-only time counter (user-level) |
| `CSR_INSTRET` | `0xC02` | Read-only instret counter (user-level) |
| `CSR_HPMCOUNTER3`–`31` | `0xC03`–`0xC1F` | Read-only HPM counters (user-level) |
| `CSR_MHPMCOUNTER3`–`31` | `0xB03`–`0xB1F` | Writable HPM counters (machine-level) |
| `CAUSE_VIRTUAL_INSTRUCTION` | `22` | Virtual-instruction exception |
| `CAUSE_ILLEGAL_INSTRUCTION` | `2` | Illegal-instruction exception |

### hcounteren Bit Index Mapping

| Bit | Name | Corresponding Counter |
|-----|------|----------------------|
| 0 | CY | cycle |
| 1 | TM | time |
| 2 | IR | instret |
| 3–31 | HPM3–HPM31 | hpmcounter3–hpmcounter31 |

### Test Framework API

- `hcounteren_read()` / `hcounteren_write(value)`: hcounteren read/write
- `hcounteren_set(mask)` / `hcounteren_clear(mask)`: hcounteren set/clear bits
- `run_in_vs_mode(fn, arg)`: Execute fn(arg) in VS-mode (V=1)
- `run_in_vu_mode(fn, arg)`: Execute fn(arg) in VU-mode (V=1, U)
- `trap_expect_begin()` / `trap_expect_end()`: Trap capture window
- `trap_was_triggered()` / `trap_get_cause()`: Trap status query
- `EXPECT_VIRTUAL_INST(stmt)`: Expect virtual-instruction exception macro
- `VS_EXPECT_NO_TRAP(stmt)`: Expect VS-mode no exception macro
- `HYP_TEST_END()`: Test end macro (includes hyp_reset_state)
- `csr_read_dynamic(addr)` / `csr_write_dynamic(addr, val)`: Dynamic CSR read/write by address

---

## Test Statistics

| Group | Test Count | Description |
|-------|------------|-------------|
| Group 1: Writability Verification | 4 | M-mode basic read-write loopback |
| Group 2: VS-mode Access Control | 8 | End-to-end permission verification |
| Group 3: Toggle Consistency | 3 | Repeated toggle verification |
| Group 4: Hierarchical Interaction | 5 | mcounteren/hcounteren/scounteren gating |
| Group 5: Read-Only Zero Report | 1 | Information collection |
| **Total** | **21** | |

---

## Test Execution Notes

### Subdirectory and Build Integration (Implementation Phase Reference, Not Produced by This Plan)

> [!IMPORTANT]
> This planning phase **only produces `DOCS/testplan/shcounterenw_test_plan.md`**, does not create `shcounterenw/` subdirectory, does not modify top-level `Makefile`, does not write C test code. Subsequent implementation phase creates per this document:
> - `shcounterenw/main.c` (reference `svadu/main.c`)
> - `shcounterenw/Makefile` (reference `svadu/Makefile`, `SPIKE_ISA_EXT = _shcounterenw`)
> - `shcounterenw/kernel.ld` (reference `svadu/kernel.ld`)
> - `shcounterenw/tests/test_helpers.h` (counter probing, dynamic CSR access helpers)
> - `shcounterenw/tests/test_writable.c` (Group 1)
> - `shcounterenw/tests/test_access.c` (Group 2)
> - `shcounterenw/tests/test_toggle.c` (Group 3)
> - `shcounterenw/tests/test_hierarchy.c` (Group 4)
> - `shcounterenw/tests/test_readonly.c` (Group 5)
> And add `shcounterenw` to top-level `Makefile`'s `EXTENSIONS` list.

### Runtime Environment

- Group 1: M-mode directly operates hcounteren (CSR 0x606), no need to enter VS-mode
- Group 2/3: Enter VS-mode via `run_in_vs_mode` to verify counter access
- Group 4: Involves VS-mode and VU-mode hierarchical verification
- Group 5: M-mode probing and reporting
- Single-core environment, no IPI required

### Failure Diagnosis Guide

| Symptom | Possible Cause |
|---------|----------------|
| SHCNTW-WR-01/02/03 failure | Shcounterenw not enabled, hcounteren bits remain read-only zero |
| Some bits fail in SHCNTW-WR-04 | Corresponding hpmcounter implementation detection error (misidentified as implemented), or that bit is indeed read-only |
| SHCNTW-ACCESS-01 failure (exception occurs) | hcounteren_write did not take effect, or mcounteren not properly set |
| SHCNTW-ACCESS-02 failure (no exception) | hcounteren bit=0 did not take effect, VS-mode can still access counter |
| SHCNTW-ACCESS-02 failure (cause≠22) | mcounteren may be 0 causing cause=2 instead of cause=22 |
| SHCNTW-HIER-01 triggers cause=22 instead of cause=2 | mcounteren clearing did not take effect; implementation may still take virtual-inst path when mcounteren=0 (non-compliant) |
| SHCNTW-HIER-04/05 failure (no exception) | hideleg/hedeleg configuration issue, or scounteren/hcounteren write did not take effect |
| SHCNTW-TOGGLE failure | Race condition or caching issue in hcounteren bit write (theoretically should not occur in single-core) |
