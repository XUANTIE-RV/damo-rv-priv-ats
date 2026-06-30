**[中文](../testplan/Svbare_test_plan.md) | English**

# Svbare Extension Test Plan

This document describes the test plan for the Svbare (Supervisor Bare Mode Support) extension. The Svbare extension mandates that implementations must support the `satp` register's MODE field being capable of holding the Bare value (MODE=0). When MODE=Bare, supervisor virtual addresses equal supervisor physical addresses, no page table translation is performed, and no additional memory protection beyond PMP is applied.

---

## Overview

In the RISC-V Privileged Specification, the `satp` (Supervisor Address Translation and Protection) register controls the address translation mode. Its MODE field determines the translation scheme:

1. **MODE=Bare (value 0)**: No address translation or protection. Supervisor virtual addresses directly equal physical addresses; the only memory protection mechanism is PMP (Physical Memory Protection).
2. **MODE=Sv39/Sv48/Sv57**: Enables the corresponding level of page table virtual memory system.

The core requirement of the Svbare extension is: **implementations must support the `satp.MODE` field being capable of holding the Bare value**. This means software can disable virtual memory by writing `satp=0`.

When selecting MODE=Bare, software must write zero to all remaining fields of `satp` (ASID, PPN). Attempting to select MODE=Bare with nonzero values in the remaining fields has an UNSPECIFIED effect.

This test plan focuses on the following aspects:
- CSR writability and readback consistency of `satp.MODE=Bare`
- VA=PA passthrough memory access behavior in Bare mode
- Verification that SUM/MXR bits have no effect in Bare mode
- Interaction between Bare mode and PMP
- Switching of satp MODE between Bare and Sv39
- Boundary conditions and special scenarios

---

## Test Scope

### Specification Sources

- `SPEC/supervisor.adoc`, lines 1005-1015 (`satp` MODE field semantics, MODE=Bare definition)
- `SPEC/supervisor.adoc`, lines 1036-1037 (`[[svbare]]` extension definition)
- `SPEC/supervisor.adoc`, lines 1055-1057 (writing an unsupported MODE causes the entire write to be ignored)
- `SPEC/supervisor.adoc`, lines 1100-1103 (`satp` is only active in S/U-mode)

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:svbare_satp_mode_bare` | If an implementation supports the Svbare extension, then the `satp` register's MODE field must be capable of holding the value Bare. |
| `norm:satp_mode` | When MODE=Bare, supervisor virtual addresses are equal to supervisor physical addresses, and there is no additional memory protection. To select MODE=Bare, software must write zero to the remaining fields of `satp`. Attempting to select MODE=Bare with a nonzero pattern in the remaining fields has an UNSPECIFIED effect. |
| `norm:satp_mode_op_unsupported` | -- |
| `norm:satp_op_active` | -- |
| `norm:sstatus_sum` | The SUM (permit Supervisor User Memory access) bit modifies the privilege with which S-mode loads and stores access virtual memory. When SUM=0, S-mode memory accesses to pages that are accessible by U-mode (U=1) will fault. When SUM=1, these accesses are permitted. SUM has no effect when page-based virtual memory is not in effect, nor when executing in U-mode. Note that S-mode can never execute instructions from user pages, regardless of the state of SUM. |
| `norm:sstatus_mxr` | The MXR (Make eXecutable Readable) bit modifies the privilege with which loads access virtual memory. When MXR=0, only loads from pages marked readable (R=1) will succeed. When MXR=1, loads from pages marked either readable or executable (R=1 or X=1) will succeed. MXR has no effect when page-based virtual memory is not in effect. |

### Out of Scope

- **Hypervisor two-stage translation scenarios**: Bare mode behavior under VS-stage and G-stage (involves the H extension).
- **Sv32 (RV32)**: This plan covers only RV64 (SXLEN=64); the Bare MODE encoding and ASID field layout differ under RV32.
- **Multi-hart coherence**: The current test framework operates in a single-core environment.
- **Sv48 / Sv57 mode switching**: This plan uses only Sv39 as the comparison switching mode for Bare; other Sv modes behave consistently.

---

## Test Groups

### Group 1: satp.MODE=Bare Writability Verification

**Specification basis**:
- `svbare`: `satp.MODE` must be capable of holding the Bare value
- `norm:satp_mode`: Selecting Bare requires writing zero to the remaining fields; nonzero remaining fields have UNSPECIFIED effect

**Test responsibilities**: Verify the ability to write `satp` MODE=Bare in M-mode, including readback consistency, the zero-constraint on remaining fields, and switching with other modes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVBARE-CSR-01 | satp write MODE=Bare readback | M-mode writes satp=0 (MODE=Bare, all remaining fields zero), then reads back | Readback MODE field is 0 (Bare) |
| SVBARE-CSR-02 | ASID/PPN fields are zero in Bare mode | Write satp=0 and read back the full value | All 64 bits of satp are 0 |
| SVBARE-CSR-03 | Switch from Sv39 to Bare | First write satp MODE=Sv39 (valid configuration), then write satp=0 | Readback MODE=Bare, satp=0 |
| SVBARE-CSR-04 | Switch from Bare to Sv39 and back to Bare | Bare->Sv39->Bare multiple switches | Each Bare readback is 0 |
| SVBARE-CSR-05 | MODE=Bare with nonzero remaining fields | Write satp with MODE=0 but nonzero ASID/PPN | UNSPECIFIED: record actual behavior (may ignore, may zero out, may reject write) |

---

### Group 2: Bare Mode VA=PA Passthrough Access

**Specification basis**:
- `norm:satp_mode`: When MODE=Bare, supervisor virtual addresses equal supervisor physical addresses
- `norm:satp_op_active`: `satp` is active in S/U-mode

**Test responsibilities**: Verify that in MODE=Bare, S-mode and U-mode memory accesses use physical addresses directly, with no translation. PMP must be configured to allow S/U-mode access.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVBARE-VA-01 | S-mode Bare load | satp=0, S-mode load from a known physical address | Read succeeds, value is correct |
| SVBARE-VA-02 | S-mode Bare store | satp=0, S-mode store to a known physical address | Write succeeds, verified by readback |
| SVBARE-VA-03 | S-mode Bare fetch | satp=0, S-mode instruction fetch from a physical address | Execution succeeds |
| SVBARE-VA-04 | U-mode Bare load | satp=0, U-mode load from a known physical address | Read succeeds (requires PMP permission) |
| SVBARE-VA-05 | U-mode Bare store | satp=0, U-mode store to a known physical address | Write succeeds (requires PMP permission) |
| SVBARE-VA-06 | Bare mode multi-address access | S-mode accesses multiple different physical address regions in sequence | Each access succeeds via passthrough |

---

### Group 3: No Page Table Translation Verification in Bare Mode

**Specification basis**:
- `norm:satp_mode`: When MODE=Bare, no additional memory protection beyond PMP
- `norm:sstatus_sum`: SUM has no effect when page-based virtual memory is not in effect (no effect in Bare mode)
- `norm:sstatus_mxr`: MXR has no effect when page-based virtual memory is not in effect (no effect in Bare mode)

**Test responsibilities**: Verify that Bare mode does not trigger page-faults, that SUM/MXR bits have no effect on Bare mode, and that SFENCE.VMA has no side effects.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVBARE-NOPT-01 | Bare mode does not trigger page-fault | satp=0, S-mode accesses any PMP-allowed physical address | No page-fault (scause != 12/13/15) |
| SVBARE-NOPT-02 | SUM=0 does not affect Bare S-mode access | satp=0, sstatus.SUM=0, S-mode load | Access succeeds (SUM has no effect in Bare mode) |
| SVBARE-NOPT-03 | SUM=1 does not affect Bare S-mode access | satp=0, sstatus.SUM=1, S-mode load | Access succeeds (SUM has no effect in Bare mode) |
| SVBARE-NOPT-04 | MXR does not affect Bare mode behavior | satp=0, sstatus.MXR=1 or 0, S-mode load | Access behavior is consistent (MXR has no effect in Bare mode) |
| SVBARE-NOPT-05 | SFENCE.VMA has no side effects in Bare mode | satp=0, execute SFENCE.VMA, then perform normal access | Access succeeds, no exception |

---

### Group 4: Bare Mode and PMP Interaction

**Specification basis**:
- `norm:satp_mode`: When MODE=Bare, no additional memory protection; only PMP is active

**Test responsibilities**: Verify that in Bare mode, PMP remains the only memory protection mechanism. Accesses succeed when PMP allows, and trigger access-fault (not page-fault) when PMP denies.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVBARE-PMP-01 | PMP allow + Bare S-mode load | satp=0, PMP RWX fully open, S-mode load | Read succeeds |
| SVBARE-PMP-02 | PMP deny + Bare S-mode load | satp=0, PMP does not cover target address, S-mode load | load access-fault (scause=5) |
| SVBARE-PMP-03 | PMP read-only + Bare S-mode store | satp=0, PMP allows R only, S-mode store | store access-fault (scause=7) |
| SVBARE-PMP-04 | PMP no-execute + Bare S-mode fetch | satp=0, PMP allows RW but not X, S-mode fetch | instruction access-fault (scause=1) |
| SVBARE-PMP-05 | PMP allow + Bare U-mode load | satp=0, PMP RWX fully open, U-mode load | Read succeeds |
| SVBARE-PMP-06 | PMP deny + Bare U-mode store | satp=0, PMP does not cover target address, U-mode store | store access-fault (scause=7) |

---

### Group 5: satp MODE Switching and Bare Mode Transitions

**Specification basis**:
- `svbare`: MODE field must be capable of holding the Bare value
- `norm:satp_mode_op_unsupported`: Writing an unsupported MODE causes the entire write to be ignored; `satp` is not modified

**Test responsibilities**: Verify the correctness of Bare mode when satp switches between different MODEs, and the retention of Bare after writing an invalid MODE.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVBARE-SW-01 | Bare->Sv39->Bare normal switching | Switch three times, verify readback each time | MODE field correctly reflects the switch result |
| SVBARE-SW-02 | Writing reserved MODE leaves satp unchanged | satp=0 (Bare), write MODE=7 (reserved), read back | satp remains Bare unchanged (entire write is ignored) |
| SVBARE-SW-03 | S-mode remains Bare after writing reserved MODE | Following SW-02, S-mode load | Passthrough access succeeds (still Bare) |
| SVBARE-SW-04 | VA=PA after Sv39->Bare switch | Switch from Sv39 mode back to Bare, S-mode access | Passthrough physical address access succeeds |
| SVBARE-SW-05 | Multiple Bare/Sv39 switch stability | 10 cyclic switches between Bare and Sv39 | Each Bare readback is correct, VA=PA after each switch back |

---

### Group 6: Bare Mode Special Scenarios

**Specification basis**:
- `norm:satp_mode`: Bare with nonzero remaining fields has UNSPECIFIED effect
- `norm:satp_op_active`: `satp` is only active in S/U-mode; M-mode is not affected by `satp`

**Test responsibilities**: Cover boundary conditions and special scenarios, including M-mode being unaffected by satp, and TVM bit controlling S-mode satp writes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVBARE-EDGE-01 | M-mode unaffected by Bare | satp=0, M-mode access | M-mode is unaffected by satp, normal access |
| SVBARE-EDGE-02 | Write all 1s to satp (invalid MODE) | Write all bits of satp to 1 | Implementation-defined: satp may remain unchanged (entire write ignored) or MODE field may be truncated |
| SVBARE-EDGE-03 | Consecutive satp read/write consistency in Bare mode | Write satp=0, then read back N consecutive times | Each readback is 0 |
| SVBARE-EDGE-04 | S-mode writes satp after Bare (TVM=0) | mstatus.TVM=0, S-mode attempts to write satp=0 | Write succeeds, satp remains Bare |
| SVBARE-EDGE-05 | S-mode writes satp triggers exception when TVM=1 | mstatus.TVM=1, S-mode writes satp | Triggers illegal instruction exception (scause=2) |

> **Note**: SVBARE-EDGE-02 involves implementation-defined behavior; test cases should record actual behavior rather than making strong assertions.

---

## Appendix: Related Constants and API Reference

### satp Register RV64 Layout

### MODE Encoding Table

| Value | Name | Description |
|-------|------|-------------|
| 0 | Bare | No address translation or protection |
| 1-7 | - | Reserved for standard use |
| 8 | Sv39 | 39-bit page table virtual address |
| 9 | Sv48 | 48-bit page table virtual address |
| 10 | Sv57 | 57-bit page table virtual address |
| 11 | Sv64 | Reserved for 64-bit virtual addressing |
| 12-13 | - | Reserved for standard use |
| 14-15 | - | Designated for custom use |

### CSR Access Macros

- `CSRR(satp)`: Read the satp register
- `CSRW(satp, val)`: Write the satp register
- `CSRS(sstatus, bit)`: Set specified bit in sstatus
- `CSRC(sstatus, bit)`: Clear specified bit in sstatus
- `sfence_vma()`: Execute the SFENCE.VMA instruction

### satp Construction Macros

### PMP Helper Functions

- `pmp_allow_all()`: Configure PMP NAPOT to cover the entire address space with RWX permissions
- `pmp_clear()`: Clear PMP configuration
- `pmp_setup_code_and_stack_only()`: Allow only the code segment and stack region
- `pmp_setup_readonly(addr, size)`: Configure the specified region as read-only

### scause Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `CAUSE_INST_ACCESS_FAULT` | 1 | Instruction access fault (PMP) |
| `CAUSE_ILLEGAL_INST` | 2 | Illegal instruction |
| `CAUSE_LOAD_ACCESS_FAULT` | 5 | Load access fault (PMP) |
| `CAUSE_STORE_ACCESS_FAULT` | 7 | Store/AMO access fault (PMP) |
| `CAUSE_INST_PAGE_FAULT` | 12 | Instruction page fault |
| `CAUSE_LOAD_PAGE_FAULT` | 13 | Load page fault |
| `CAUSE_STORE_PAGE_FAULT` | 15 | Store/AMO page fault |

### mstatus Related Bits

| Bit | Name | Description |
|-----|------|-------------|
| bit 18 | SUM | Permit Supervisor User Memory access |
| bit 19 | MXR | Make eXecutable Readable |
| bit 20 | TVM | Trap Virtual Memory (traps S-mode access to satp) |

### Test Framework API

- `run_in_priv(priv, fn, arg)`: Execute fn(arg) at the specified privilege level, returns the function's return value
- `goto_priv(priv)`: Switch to the specified privilege level
- `reset_state()`: Reset PMP and test state, return to M-mode
- `trap_expect_begin()` / `trap_expect_end()`: Begin/end trap recording
- `trap_was_triggered()`: Check whether a trap occurred
- `trap_get_cause()`: Get the scause value of the trap
- `TEST_REGISTER(fn)`: Register a test function
- `TEST_BEGIN(name)` / `TEST_END()`: Begin/end a test case
- `TEST_ASSERT(msg, cond)`: Conditional assertion
- `TEST_ASSERT_EQ(msg, actual, expected)`: Equality assertion
- `TEST_SKIP(reason)`: Skip a test

---

## Test Execution Instructions

### Directory Structure

The test code is planned to be placed in a new `svbare/` directory with the following structure:

### Makefile Configuration

> **Note**: Svbare tests **do not require enabling VM** (`ENABLE_VM = 0`), because Bare mode does not involve page table translation. Tests operate on the `satp` CSR in M-mode and use `run_in_priv()` to switch to S/U-mode to verify memory access behavior.

### Simulator Requirements

- **Spike**: Supports Bare mode by default, no additional ISA extension parameters required. Run directly:
- **QEMU**: Supports Bare mode by default, no additional parameters required.
- **Sail**: Supports Bare mode by default.

### Test Run Commands
