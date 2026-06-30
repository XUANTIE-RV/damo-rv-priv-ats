**[中文](../testplan/Sscounterenw_test_plan.md) | English**

# Sscounterenw Extension Test Plan

This document describes the test plan for the Sscounterenw (Counter-Enable Writability, Version 1.0) extension. The Sscounterenw extension mandates that if this extension is implemented, then for any `hpmcounter` that is not read-only zero, the corresponding bit in `scounteren` must be writable.

---

## Overview

In the RISC-V Privileged Specification, the `scounteren` register controls S-mode access to hardware performance counters (`hpmcounter3`-`hpmcounter31`) as well as `cycle`, `time`, and `instret`. When a bit in `scounteren` is 0, U-mode access to the corresponding counter triggers an illegal instruction exception.

However, the base specification does not mandate that all bits in `scounteren` be writable -- implementations may hardwire certain bits to 0 or 1. This prevents software from reliably controlling U-mode access to counters.

**Core constraint of the Sscounterenw extension**:

> If a `hpmcounter` is not read-only zero (i.e., the counter is implemented and meaningful), the corresponding bit in `scounteren` **must** be writable (i.e., software can set it to 0 or 1).

This constraint ensures:
1. Software can precisely control U-mode access permissions to implemented counters
2. The OS can grant or revoke U-mode read access to performance counters as needed

---

## Test Scope

### Specification Source

- `SPEC/sscounterenw.adoc` -- Sscounterenw Extension for Counter-Enable Writability, Version 1.0

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:sscounterenw_hpmcounter_scounteren` | If the Sscounterenw extension is implemented, then for any `hpmcounter` that is not read-only zero, the corresponding bit in `scounteren` must be writable. |
| `norm:scounteren_bit_set_to_1` | -- |
| `norm:scounteren_bit_set_to_0` | -- |
| `norm:scounteren_bit_toggle` | -- |
| `norm:scounteren_readonly_zero_counter` | -- |
| `norm:mcounteren_gate_scounteren` | -- |

### Out of Scope

- **Counter event counting correctness**: Sscounterenw only constrains scounteren writability, not whether counters count correctly
- **Sscofpmf (Count Overflow and Mode-Based Filtering)**: Covered by a separate sscofpmf test plan
- **hcounteren (Hypervisor Counter-Enable)**: VS/VU-mode counter access control is covered by hypervisor tests
- **Sv32 mode**: This plan covers only RV64
- **Multi-hart scenarios**: The project is a single-core test environment
- **mcounteren writability**: mcounteren writability is defined by the base privileged specification, not specific to Sscounterenw

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
| `hpmcounter3`-`hpmcounter31` | 0xC03-0xC1F | Hardware performance counters (read-only, bit 3-31) |

### Key Reference Files

| Path | Description |
|------|-------------|
| `SPEC/sscounterenw.adoc` | Full Sscounterenw specification |
| `common/test_framework.h` | Test framework (TEST_BEGIN / TEST_ASSERT / TEST_END) |
| `common/encoding.h` | CSR address definitions (CSR_MCOUNTEREN / CSR_SCOUNTEREN) |

### Design Principles

1. **Dynamic discovery**: At runtime, first probe which hpmcounters are implemented (not read-only zero), then verify scounteren writability for those counters
2. **End-to-end verification**: Not only verify scounteren bit read-write loopback, but also verify the actual access control effect after setting (U-mode access success/failure)
3. **Boundary coverage**: Cover cycle (bit 0), time (bit 1), instret (bit 2), and hpmcounter3-31 (bit 3-31)

---

## Test Groups

### Group 1: scounteren Writability Verification (M-mode Read-Write Loopback)

**Spec Reference**:
- `norm:scounteren_writable_for_implemented_counter`: The scounteren bit corresponding to an implemented counter must be writable

**Test Scope**: In M-mode, perform write-1/read-back and write-0/read-back verification for each implemented hpmcounter's corresponding scounteren bit.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCNTW-WR-01 | cycle corresponding scounteren[0] writable | Probe whether cycle is implemented; if so, verify scounteren[0] can be written to 1 and 0 | Read-back matches written value |
| SSCNTW-WR-02 | time corresponding scounteren[1] writable | Probe whether time is implemented; if so, verify scounteren[1] can be written to 1 and 0 | Read-back matches written value |
| SSCNTW-WR-03 | instret corresponding scounteren[2] writable | Probe whether instret is implemented; if so, verify scounteren[2] can be written to 1 and 0 | Read-back matches written value |
| SSCNTW-WR-04 | hpmcounter3-31 corresponding scounteren[3:31] writable | Probe each hpmcounter3-31; for implemented ones, verify the corresponding scounteren bit is writable | Read-back matches written value for implemented counters |

---

### Group 2: scounteren Controls U-mode Access (End-to-End Verification)

**Spec Reference**:
- `norm:scounteren_bit_set_to_1`: When bit is 1, U-mode can access
- `norm:scounteren_bit_set_to_0`: When bit is 0, U-mode access triggers illegal instruction

**Test Scope**: For implemented hpmcounters, verify that when the scounteren bit is set to 1, U-mode reads succeed; when set to 0, U-mode reads trigger an illegal instruction (cause=2).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCNTW-ACCESS-01 | U-mode reads cycle successfully when scounteren[0]=1 | Set mcounteren[0]=1, scounteren[0]=1; U-mode reads cycle | No exception |
| SSCNTW-ACCESS-02 | U-mode read of cycle triggers exception when scounteren[0]=0 | Set mcounteren[0]=1, scounteren[0]=0; U-mode reads cycle | Illegal instruction (cause=2) |
| SSCNTW-ACCESS-03 | U-mode reads time successfully when scounteren[1]=1 | Set mcounteren[1]=1, scounteren[1]=1; U-mode reads time | No exception |
| SSCNTW-ACCESS-04 | U-mode read of time triggers exception when scounteren[1]=0 | Set mcounteren[1]=1, scounteren[1]=0; U-mode reads time | Illegal instruction (cause=2) |
| SSCNTW-ACCESS-05 | U-mode reads instret successfully when scounteren[2]=1 | Set mcounteren[2]=1, scounteren[2]=1; U-mode reads instret | No exception |
| SSCNTW-ACCESS-06 | U-mode read of instret triggers exception when scounteren[2]=0 | Set mcounteren[2]=1, scounteren[2]=0; U-mode reads instret | Illegal instruction (cause=2) |
| SSCNTW-ACCESS-07 | U-mode reads hpmcounterN successfully when scounteren[N]=1 | For implemented hpmcounterN, set corresponding bit=1; U-mode reads | No exception |
| SSCNTW-ACCESS-08 | U-mode read of hpmcounterN triggers exception when scounteren[N]=0 | For implemented hpmcounterN, set corresponding bit=0; U-mode reads | Illegal instruction (cause=2) |

---

### Group 3: scounteren Bit Toggle Consistency

**Spec Reference**:
- `norm:scounteren_bit_toggle`: scounteren bits can be toggled repeatedly with consistent behavior

**Test Scope**: For implemented counters, repeatedly set and clear scounteren bits, verifying that U-mode access behavior is consistent after each toggle.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCNTW-TOGGLE-01 | cycle corresponding bit toggle | Toggle scounteren[0] through 1->0->1->0, verifying U-mode behavior each time | Behavior matches current bit value after each toggle |
| SSCNTW-TOGGLE-02 | instret corresponding bit toggle | Toggle scounteren[2] through 1->0->1->0, verifying U-mode behavior each time | Behavior matches current bit value after each toggle |
| SSCNTW-TOGGLE-03 | hpmcounterN corresponding bit toggle | Toggle implemented hpmcounterN bits and verify | Behavior matches current bit value after each toggle |

---

### Group 4: mcounteren and scounteren Hierarchical Interaction

**Spec Reference**:
- `norm:mcounteren_gate_scounteren`: mcounteren still gates S-mode access to counters

**Test Scope**: Verify that even when the scounteren bit is 1, if the corresponding mcounteren bit is 0, S-mode access to the counter still triggers an exception (Sscounterenw does not change the hierarchical relationship).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCNTW-HIER-01 | S-mode read of cycle fails when mcounteren[0]=0 | Set mcounteren[0]=0, scounteren[0]=1; S-mode reads cycle | Illegal instruction (cause=2) |
| SSCNTW-HIER-02 | S-mode read of cycle succeeds when mcounteren[0]=1 | Set mcounteren[0]=1, scounteren[0]=1; S-mode reads cycle | No exception |
| SSCNTW-HIER-03 | S-mode read of hpmcounterN fails when mcounteren[N]=0 | Set mcounteren[N]=0, scounteren[N]=1; S-mode reads | Illegal instruction (cause=2) |
| SSCNTW-HIER-04 | U-mode read of cycle fails when mcounteren=0 scounteren=1 | When mcounteren blocks, even if scounteren=1, U-mode cannot access | Illegal instruction (cause=2) |

---

### Group 5: scounteren Bit Behavior for Read-Only Zero Counters

**Spec Reference**:
- `norm:scounteren_readonly_zero_counter`: For read-only zero hpmcounters, Sscounterenw does not require their scounteren bits to be writable

**Test Scope**: For hpmcounters probed as read-only zero, record their scounteren bit behavior (may be read-only 0 or read-only 1); no pass/fail judgment, informational collection only.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCNTW-RO-01 | Read-only zero counter scounteren bit probe | For all read-only zero hpmcounters, attempt to write scounteren bits and report results | Informational output, no pass/fail |

---

## Counter Implementation Discovery Strategy

Since different implementations may support different subsets of hpmcounters, tests must dynamically probe at runtime. The discovery algorithm is as follows:

```
for each counter index i (0..31):
    1. M-mode: set mcounteren[i] = 1
    2. For i=0(cycle), i=1(time), i=2(instret):
       - Directly read the corresponding CSR in M-mode; if non-zero, it is implemented
    3. For i=3..31 (hpmcounter3-31):
       - Write mhpmcounter[i] to a non-zero value in M-mode
       - Read back mhpmcounter[i]; if the read-back value is non-zero, it is implemented
       - Restore original value
    4. Record the bitmap of implemented counters for subsequent tests
```

---

## Test Statistics

| Group | Test Count | Description |
|-------|------------|-------------|
| Group 1: Writability verification | 4 | Basic read-write loopback |
| Group 2: U-mode access control | 8 | End-to-end permission verification |
| Group 3: Toggle consistency | 3 | Repeated toggle verification |
| Group 4: Hierarchical interaction | 4 | mcounteren gating |
| Group 5: Read-only zero report | 1 | Informational collection |
| **Total** | **20** | |

---

## Framework Support Required

### New CSR Definitions Needed (`common/encoding.h`)

Currently `encoding.h` has `CSR_MCOUNTEREN` (0x306) and `CSR_SCOUNTEREN` (0x106), but lacks the following definitions:

### New Dynamic CSR Access Capability Needed

Since `hpmcounter3`-`hpmcounter31` comprise 29 CSRs, hardcoding each one individually is impractical. A mechanism to read CSRs by index is needed:

### U-mode CSR Read Execution

The current framework supports the `goto_priv(PRIV_U)` + `PRIV_DO_TRAP`/`PRIV_DO_NO_TRAP` pattern. However, executing `csrr` instructions to read counters in U-mode requires special attention:

- In U-mode, if the target CSR is prohibited by scounteren, `csrr` will trigger an illegal instruction
- The trap handler must correctly handle and skip the instruction (the current framework's `trap_expect_begin/end` mechanism already supports this)

**Conclusion**: The current framework's trap mechanism is sufficient; no modifications to trap handling logic are needed.

---

## Suggested Directory Structure

```
sscounterenw/
├── Makefile
├── main.c
├── kernel.ld              (can reuse sstvecd/kernel.ld)
└── tests/
    ├── test_helpers.h     (counter probing, dynamic CSR access helpers)
    ├── test_writable.c    (Group 1: writability verification)
    ├── test_access.c      (Group 2: U-mode access control)
    ├── test_toggle.c      (Group 3: toggle consistency)
    ├── test_hierarchy.c   (Group 4: hierarchical interaction)
    └── test_readonly.c    (Group 5: read-only zero report)
```
