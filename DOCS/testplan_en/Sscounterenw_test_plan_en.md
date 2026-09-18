**[中文](../testplan/Sscounterenw_test_plan.md) | English**

# Sscounterenw Extension Test Plan

This document describes the test plan for the Sscounterenw (Counter-Enable Writability, Version 1.0) extension. The Sscounterenw extension mandates that if this extension is implemented, then for any `hpmcounter` that is not read-only zero, the corresponding bit in `scounteren` must be writable.

---

## Overview

In the RISC-V Privileged Specification, the `scounteren` register controls S-mode access to hardware performance counters (`hpmcounter3`–`hpmcounter31`) as well as `cycle`, `time`, and `instret`. When a bit in `scounteren` is 0, U-mode access to the corresponding counter triggers an illegal-instruction exception.

However, the base specification does not mandate that all bits in `scounteren` be writable — implementations may hardwire certain bits to 0 or 1. This prevents software from reliably controlling U-mode access to counters.

**Core constraint of the Sscounterenw extension**:

> If a `hpmcounter` is not read-only zero (i.e., the counter is implemented and meaningful), the corresponding bit in `scounteren` **must** be writable (i.e., software can set it to 0 or 1).

This constraint ensures:
1. Software can precisely control U-mode access permissions to implemented counters.
2. The OS can grant or revoke U-mode read access to performance counters as needed.

---

## Specification Sections Covered by This Document

This plan is based on the following RISC-V official specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/sscounterenw.adoc` — Sscounterenw Extension for Counter-Enable Writability, Version 1.0
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — scounteren register definition and access-control behavior
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — mcounteren gating of S-mode counter access

Official repository:

- https://github.com/riscv/riscv-isa-manual (corresponding files `src/priv/sscounterenw.adoc`, `src/priv/supervisor.adoc`, `src/priv/machine.adoc` in the repository)

---

## Covered Specification Points

| Norm ID | Original Text | Description |
|---------|---------------|-------------|
| `norm:sscounterenw_hpmcounter_scounteren` | If the Sscounterenw extension is implemented, then for any `hpmcounter` that is not read-only zero, the corresponding bit in `scounteren` must be writable. | If the Sscounterenw extension is implemented, then for any non-read-only-zero `hpmcounter`, the corresponding bit in `scounteren` must be writable. |

Self-derived specification points (derived from related text in `supervisor.adoc` and `machine.adoc`; no independent norm tag exists in SPEC):

| Norm ID | Description |
|---------|-------------|
| `scounteren_bit_set_to_1` | An scounteren bit can be set to 1, after which U-mode can access the corresponding counter without an exception |
| `scounteren_bit_set_to_0` | An scounteren bit can be set to 0, after which U-mode access to the corresponding counter triggers an illegal-instruction exception |
| `scounteren_bit_toggle` | An scounteren bit can be toggled repeatedly between 0 and 1 with consistent behavior |
| `scounteren_readonly_zero_counter` | For read-only-zero hpmcounters, the corresponding scounteren bit is not constrained by Sscounterenw (may be read-only) |
| `mcounteren_gate_scounteren` | mcounteren still gates S-mode access to counters; Sscounterenw does not change this hierarchy |

---

## Out of Scope

- **Counter event counting correctness**: Sscounterenw only constrains scounteren writability, not whether counters count correctly.
- **Sscofpmf (Count Overflow and Mode-Based Filtering)**: Covered by a separate `Sscofpmf_test_plan_en.md`.
- **hcounteren (Hypervisor Counter-Enable)**: VS/VU-mode counter access control is covered by hypervisor tests.
- **Sv32 mode**: This plan covers only RV64.
- **Multi-hart scenarios**: The project is a single-core test environment.
- **mcounteren writability**: mcounteren writability is defined by the base privileged specification, not specific to Sscounterenw.

---

## Prerequisites and Constraints

> [!IMPORTANT]
> Sscounterenw verification requires first determining which `hpmcounter` instances are "implemented" (not read-only zero). The verification strategy is: in M-mode, set the corresponding `mcounteren` bit to 1, then in S-mode attempt to read the `hpmcounter`; if a non-zero value is read or no exception is triggered, the counter is considered implemented. For implemented counters, verify the writability of the corresponding `scounteren` bit.

### Key CSRs

| CSR | Address | Description |
|-----|---------|-------------|
| `mcounteren` | 0x306 | M-mode controls S-mode access to counters |
| `scounteren` | 0x106 | S-mode controls U-mode access to counters |
| `cycle` | 0xC00 | Cycle counter (read-only, bit 0) |
| `time` | 0xC01 | Time counter (read-only, bit 1) |
| `instret` | 0xC02 | Instructions-retired counter (read-only, bit 2) |
| `hpmcounter3`–`hpmcounter31` | 0xC03–0xC1F | Hardware performance counters (read-only, bit 3–31) |

### Design Principles

1. **Dynamic discovery**: At runtime, first probe which hpmcounters are implemented (not read-only zero), then verify scounteren writability for those counters.
2. **End-to-end verification**: Not only verify scounteren bit read-write loopback, but also verify the actual access-control effect after setting (U-mode access success/failure).
3. **Boundary coverage**: Cover cycle (bit 0), time (bit 1), instret (bit 2), and hpmcounter3–31 (bit 3–31).

---

## Test Groups

### Group 1: scounteren Writability Verification (M-mode Read-Write Loopback)

**Spec Reference**:
- `norm:sscounterenw_hpmcounter_scounteren`: The scounteren bit corresponding to an implemented counter must be writable.

**Test Scope**: In M-mode, perform write-1/read-back and write-0/read-back verification for each implemented hpmcounter's corresponding scounteren bit.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCNTW-WR-01 | cycle corresponding scounteren[0] writable | Probe whether cycle is implemented; if so, verify scounteren[0] can be written to 1 and 0 | Read-back matches written value |
| SSCNTW-WR-02 | time corresponding scounteren[1] writable | Probe whether time is implemented; if so, verify scounteren[1] can be written to 1 and 0 | Read-back matches written value |
| SSCNTW-WR-03 | instret corresponding scounteren[2] writable | Probe whether instret is implemented; if so, verify scounteren[2] can be written to 1 and 0 | Read-back matches written value |
| SSCNTW-WR-04 | hpmcounter3–31 corresponding scounteren[3:31] writable | Probe each hpmcounter3–31; for implemented ones, verify the corresponding scounteren bit is writable | Read-back matches written value for implemented counters |

---

### Group 2: scounteren Controls U-mode Access (End-to-End Verification)

**Spec Reference**:
- `scounteren_bit_set_to_1`: When bit is 1, U-mode can access.
- `scounteren_bit_set_to_0`: When bit is 0, U-mode access triggers illegal-instruction.

**Test Scope**: For implemented hpmcounters, verify that when the scounteren bit is set to 1, U-mode reads succeed; when set to 0, U-mode reads trigger an illegal-instruction exception (cause=2).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCNTW-ACCESS-01 | U-mode reads cycle successfully when scounteren[0]=1 | Set mcounteren[0]=1, scounteren[0]=1; U-mode reads cycle | No exception |
| SSCNTW-ACCESS-02 | U-mode read of cycle triggers exception when scounteren[0]=0 | Set mcounteren[0]=1, scounteren[0]=0; U-mode reads cycle | Illegal-instruction (cause=2) |
| SSCNTW-ACCESS-03 | U-mode reads time successfully when scounteren[1]=1 | Set mcounteren[1]=1, scounteren[1]=1; U-mode reads time | No exception |
| SSCNTW-ACCESS-04 | U-mode read of time triggers exception when scounteren[1]=0 | Set mcounteren[1]=1, scounteren[1]=0; U-mode reads time | Illegal-instruction (cause=2) |
| SSCNTW-ACCESS-05 | U-mode reads instret successfully when scounteren[2]=1 | Set mcounteren[2]=1, scounteren[2]=1; U-mode reads instret | No exception |
| SSCNTW-ACCESS-06 | U-mode read of instret triggers exception when scounteren[2]=0 | Set mcounteren[2]=1, scounteren[2]=0; U-mode reads instret | Illegal-instruction (cause=2) |
| SSCNTW-ACCESS-07 | U-mode reads hpmcounterN successfully when scounteren[N]=1 | For implemented hpmcounterN, set corresponding bit=1; U-mode reads | No exception |
| SSCNTW-ACCESS-08 | U-mode read of hpmcounterN triggers exception when scounteren[N]=0 | For implemented hpmcounterN, set corresponding bit=0; U-mode reads | Illegal-instruction (cause=2) |

---

### Group 3: scounteren Bit Toggle Consistency

**Spec Reference**:
- `scounteren_bit_toggle`: scounteren bits can be toggled repeatedly with consistent behavior.

**Test Scope**: For implemented counters, repeatedly set and clear scounteren bits, verifying that U-mode access behavior is consistent after each toggle.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCNTW-TOGGLE-01 | cycle corresponding bit toggle | Toggle scounteren[0] through 1→0→1→0, verifying U-mode behavior each time | Behavior matches current bit value after each toggle |
| SSCNTW-TOGGLE-02 | instret corresponding bit toggle | Toggle scounteren[2] through 1→0→1→0, verifying U-mode behavior each time | Behavior matches current bit value after each toggle |
| SSCNTW-TOGGLE-03 | hpmcounterN corresponding bit toggle | Toggle implemented hpmcounterN bits and verify | Behavior matches current bit value after each toggle |

---

### Group 4: mcounteren and scounteren Hierarchical Interaction

**Spec Reference**:
- `mcounteren_gate_scounteren`: mcounteren still gates S-mode access to counters.

**Test Scope**: Verify that even when the scounteren bit is 1, if the corresponding mcounteren bit is 0, S-mode access to the counter still triggers an exception (Sscounterenw does not change the hierarchical relationship).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCNTW-HIER-01 | S-mode read of cycle fails when mcounteren[0]=0 | Set mcounteren[0]=0, scounteren[0]=1; S-mode reads cycle | Illegal-instruction (cause=2) |
| SSCNTW-HIER-02 | S-mode read of cycle succeeds when mcounteren[0]=1 | Set mcounteren[0]=1, scounteren[0]=1; S-mode reads cycle | No exception |
| SSCNTW-HIER-03 | S-mode read of hpmcounterN fails when mcounteren[N]=0 | Set mcounteren[N]=0, scounteren[N]=1; S-mode reads | Illegal-instruction (cause=2) |
| SSCNTW-HIER-04 | U-mode read of cycle fails when mcounteren=0 scounteren=1 | When mcounteren blocks, even if scounteren=1, U-mode cannot access | Illegal-instruction (cause=2) |

---

### Group 5: scounteren Bit Behavior for Read-Only Zero Counters

**Spec Reference**:
- `scounteren_readonly_zero_counter`: For read-only-zero hpmcounters, Sscounterenw does not require their scounteren bits to be writable.

**Test Scope**: For hpmcounters probed as read-only zero, record their scounteren bit behavior (may be read-only 0 or read-only 1); no pass/fail judgment, informational collection only.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCNTW-RO-01 | Read-only-zero counter scounteren bit probe | For all read-only-zero hpmcounters, attempt to write scounteren bits and report results | Informational output, no pass/fail |

---

## Counter Implementation Discovery Strategy

Since different implementations may support different subsets of hpmcounters, tests must dynamically probe at runtime. The discovery algorithm is:

1. For each counter index i (0..31):
   - In M-mode: set `mcounteren[i] = 1`.
   - For i=0 (cycle), i=1 (time), i=2 (instret): directly read the corresponding CSR in M-mode; if the returned value is non-zero or the read does not raise an exception, the counter is considered implemented.
   - For i=3..31 (hpmcounter3–31): in M-mode, write `mhpmcounter[i]` to a non-zero value, then read it back; if the read-back value is non-zero, the counter is considered implemented. Restore the original value.
2. Record the bitmap of implemented counters for subsequent tests.

If any platform violates the SPEC, the corresponding case remains FAIL, and implementation defects are recorded to the `bugs/` directory.

---

## Test Statistics

| Group | Test Count | Description |
|-------|------------|-------------|
| Group 1: Writability verification | 4 | Basic read-write loopback |
| Group 2: U-mode access control | 8 | End-to-end permission verification |
| Group 3: Toggle consistency | 3 | Repeated toggle verification |
| Group 4: Hierarchical interaction | 4 | mcounteren gating |
| Group 5: Read-only-zero report | 1 | Informational collection |
| **Total** | **20** | |

---

## Appendix A: Specification Point Coverage Matrix

| Norm ID | Covered Test IDs | Coverage Status | Notes |
|---------|------------------|-----------------|-------|
| `norm:sscounterenw_hpmcounter_scounteren` | SSCNTW-WR-01, SSCNTW-WR-02, SSCNTW-WR-03, SSCNTW-WR-04, SSCNTW-ACCESS-01 ~ SSCNTW-ACCESS-08, SSCNTW-TOGGLE-01 ~ SSCNTW-TOGGLE-03 | Covered | Core spec: scounteren bit corresponding to an implemented counter must be writable |
| `scounteren_bit_set_to_1` | SSCNTW-ACCESS-01, SSCNTW-ACCESS-03, SSCNTW-ACCESS-05, SSCNTW-ACCESS-07, SSCNTW-HIER-02 | Covered | bit=1 permits U-mode access |
| `scounteren_bit_set_to_0` | SSCNTW-ACCESS-02, SSCNTW-ACCESS-04, SSCNTW-ACCESS-06, SSCNTW-ACCESS-08 | Covered | bit=0 triggers illegal-instruction in U-mode |
| `scounteren_bit_toggle` | SSCNTW-TOGGLE-01, SSCNTW-TOGGLE-02, SSCNTW-TOGGLE-03 | Covered | Bit toggle consistency |
| `scounteren_readonly_zero_counter` | SSCNTW-RO-01 | Covered | Read-only-zero counter informational report |
| `mcounteren_gate_scounteren` | SSCNTW-HIER-01, SSCNTW-HIER-02, SSCNTW-HIER-03, SSCNTW-HIER-04 | Covered | mcounteren hierarchical gating |
