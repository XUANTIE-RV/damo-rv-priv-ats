**[中文](../testplan/Svbare_test_plan.md) | English**

# Svbare Extension Test Plan

This document describes the test plan for the Svbare (Supervisor Bare Mode Support) extension. The Svbare extension mandates that implementations must support the `satp` register's MODE field being capable of holding the Bare value (MODE=0). When MODE=Bare, supervisor virtual addresses equal supervisor physical addresses, no page table translation is performed, and no additional memory protection beyond PMP is applied.

---

## SPEC Sections Covered by This Document

This plan is based on the following official RISC-V specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — `satp` register MODE field semantics, Bare mode definition, Svbare extension definition, `sstatus`.SUM/MXR bit semantics

Official repository:

- https://github.com/riscv/riscv-isa-manual (the above path file within the repository)

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

## Covered Specification Points

The table below lists the specification points covered by this plan. Entries with the `norm:` prefix are official SPEC normative rule tags.

| Norm ID | Original Text | Description |
|---------|---------------|-------------|
| `norm:svbare_satp_mode_bare` | If an implementation supports the Svbare extension, then the `satp` register's MODE field must be capable of holding the value Bare. | If an implementation supports the Svbare extension, the `satp` MODE field must be capable of holding the Bare value. |
| `norm:satp_mode` | When MODE=Bare, supervisor virtual addresses are equal to supervisor physical addresses, and there is no additional memory protection. To select MODE=Bare, software must write zero to the remaining fields of `satp`. Attempting to select MODE=Bare with a nonzero pattern in the remaining fields has an UNSPECIFIED effect. | When MODE=Bare, supervisor virtual addresses equal supervisor physical addresses with no additional memory protection. Software must write zero to the remaining `satp` fields to select Bare; a nonzero pattern has an UNSPECIFIED effect. |
| `norm:satp_mode_op_unsupported` | Implementations are not required to support all MODE settings, and if `satp` is written with an unsupported MODE, the entire write has no effect; no fields in `satp` are modified. | Writing an unsupported MODE makes the entire write take no effect; `satp` is not modified. |
| `norm:satp_op_active` | The `satp` CSR is considered active when the effective privilege mode is S-mode or U-mode. | `satp` is active only when the effective privilege mode is S-mode or U-mode. |
| `norm:sstatus_sum` | The SUM bit modifies the privilege with which S-mode loads and stores access virtual memory. SUM has no effect when page-based virtual memory is not in effect, nor when executing in U-mode. | The SUM bit controls S-mode access to U-mode pages; it has no effect when paging is not in effect or in U-mode. |
| `norm:sstatus_mxr` | The MXR bit modifies the privilege with which loads access virtual memory. MXR has no effect when page-based virtual memory is not in effect. | The MXR bit modifies the privilege with which loads access virtual memory; it has no effect when paging is not in effect. |

---

## Out of Scope

- **Hypervisor two-stage translation**: Bare mode behavior under VS-stage and G-stage is covered by `Hypervisor_Sv_test_plan.md` and related Hypervisor plans.
- **Sv32 (RV32)**: This plan covers only RV64 (SXLEN=64); the Bare MODE encoding and ASID field layout differ under RV32.
- **Multi-hart consistency**: This plan focuses on single-hart behavior.
- **Sv48 / Sv57 mode switching**: This plan uses only Sv39 as the comparison switching mode for Bare; other Sv modes behave consistently.

---

## Test Groups

### Group 1: satp.MODE=Bare Writability Verification

**Specification basis**:
- `norm:svbare_satp_mode_bare`: `satp.MODE` must be capable of holding the Bare value
- `norm:satp_mode`: Selecting Bare requires writing zero to the remaining fields; nonzero remaining fields have an UNSPECIFIED effect

**Test responsibilities**: Verify the ability to write `satp` MODE=Bare in M-mode, including readback consistency, the zero-constraint on remaining fields, and switching with other modes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVBARE-CSR-01 | satp write MODE=Bare readback | M-mode writes satp=0 (MODE=Bare, all remaining fields zero), then reads back | Readback MODE field is 0 (Bare) |
| SVBARE-CSR-02 | ASID/PPN fields are zero in Bare mode | Write satp=0 and read back the full value | All 64 bits of satp are 0 |
| SVBARE-CSR-03 | Switch from Sv39 to Bare | First write satp MODE=Sv39 (valid configuration), then write satp=0 | Readback MODE=Bare, satp=0 |
| SVBARE-CSR-04 | Switch from Bare to Sv39 and back to Bare | Bare->Sv39->Bare multiple switches | Each Bare readback is 0 |
| SVBARE-CSR-05 | MODE=Bare with nonzero remaining fields | Write satp with MODE=0 but nonzero ASID/PPN | UNSPECIFIED: record actual behavior (may ignore, may zero out, may reject the write) |

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

**Test responsibilities**: Verify that Bare mode does not trigger page-faults, that the SUM/MXR bits have no effect on Bare mode, and that SFENCE.VMA has no side effects.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVBARE-NOPT-01 | Bare mode does not trigger page-fault | satp=0, S-mode accesses any PMP-allowed physical address | No page-fault (scause != 12/13/15) |
| SVBARE-NOPT-02 | SUM=0 does not affect Bare S-mode access | satp=0, sstatus.SUM=0, S-mode load | Access succeeds (SUM has no effect in Bare mode) |
| SVBARE-NOPT-03 | SUM=1 does not affect Bare S-mode access | satp=0, sstatus.SUM=1, S-mode load | Access succeeds (SUM has no effect in Bare mode) |
| SVBARE-NOPT-04 | MXR does not affect Bare mode behavior | satp=0, sstatus.MXR=1 or 0, S-mode load | Access behavior is consistent (MXR has no effect in Bare mode) |
| SVBARE-NOPT-05 | SFENCE.VMA has no side effects in Bare mode | satp=0, execute SFENCE.VMA, then perform a normal access | Access succeeds, no exception |

---

### Group 4: Bare Mode and PMP Interaction

**Specification basis**:
- `norm:satp_mode`: When MODE=Bare, no additional memory protection; only PMP is active

**Test responsibilities**: Verify that in Bare mode, PMP remains the only memory protection mechanism. Accesses succeed when PMP allows, and trigger an access-fault (not a page-fault) when PMP denies.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVBARE-PMP-01 | PMP allow + Bare S-mode load | satp=0, PMP RWX fully open, S-mode load | Read succeeds |
| SVBARE-PMP-02 | PMP deny + Bare S-mode load | satp=0, PMP does not cover the target address, S-mode load | load access-fault (scause=5) |
| SVBARE-PMP-03 | PMP read-only + Bare S-mode store | satp=0, PMP allows R only, S-mode store | store access-fault (scause=7) |
| SVBARE-PMP-04 | PMP no-execute + Bare S-mode fetch | satp=0, PMP allows RW but not X, S-mode fetch | instruction access-fault (scause=1) |
| SVBARE-PMP-05 | PMP allow + Bare U-mode load | satp=0, PMP RWX fully open, U-mode load | Read succeeds |
| SVBARE-PMP-06 | PMP deny + Bare U-mode store | satp=0, PMP does not cover the target address, U-mode store | store access-fault (scause=7) |

---

### Group 5: satp MODE Switching and Bare Mode Transitions

**Specification basis**:
- `norm:svbare_satp_mode_bare`: The MODE field must be capable of holding the Bare value
- `norm:satp_mode_op_unsupported`: Writing an unsupported MODE causes the entire write to be ignored; `satp` is not modified

**Test responsibilities**: Verify the correctness of Bare mode when satp switches between different MODEs, and the retention of Bare after writing an invalid MODE.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVBARE-SW-01 | Bare->Sv39->Bare normal switching | Switch three times, verify readback each time | The MODE field correctly reflects the switch result |
| SVBARE-SW-02 | Writing reserved MODE leaves satp unchanged | satp=0 (Bare), write MODE=7 (reserved), read back | satp remains Bare unchanged (the entire write is ignored) |
| SVBARE-SW-03 | S-mode remains Bare after writing reserved MODE | Following SW-02, S-mode load | Passthrough access succeeds (still Bare) |
| SVBARE-SW-04 | VA=PA after Sv39->Bare switch | Switch from Sv39 mode back to Bare, S-mode access | Passthrough physical address access succeeds |
| SVBARE-SW-05 | Multiple Bare/Sv39 switch stability | 10 cyclic switches between Bare and Sv39 | Each Bare readback is correct, VA=PA after each switch back |

---

### Group 6: Bare Mode Special Scenarios

**Specification basis**:
- `norm:satp_mode`: Bare with nonzero remaining fields has an UNSPECIFIED effect
- `norm:satp_op_active`: `satp` is only active in S/U-mode; M-mode is not affected by `satp`

**Test responsibilities**: Cover boundary conditions and special scenarios, including M-mode being unaffected by satp, and the TVM bit controlling S-mode satp writes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVBARE-EDGE-01 | M-mode unaffected by Bare | satp=0, M-mode access | M-mode is unaffected by satp, normal access |
| SVBARE-EDGE-02 | Write all 1s to satp (invalid MODE) | Write all bits of satp to 1 | Implementation-defined: satp may remain unchanged (the entire write is ignored) or the MODE field may be truncated |
| SVBARE-EDGE-03 | Consecutive satp read/write consistency in Bare mode | Write satp=0, then read back N consecutive times | Each readback is 0 |
| SVBARE-EDGE-04 | S-mode writes satp after Bare (TVM=0) | mstatus.TVM=0, S-mode attempts to write satp=0 | Write succeeds, satp remains Bare |
| SVBARE-EDGE-05 | S-mode writes satp triggers exception when TVM=1 | mstatus.TVM=1, S-mode writes satp | Triggers an illegal instruction exception (scause=2) |

> [!NOTE]
> SVBARE-EDGE-02 involves implementation-defined behavior; test cases should record actual behavior rather than making strong assertions.

---

## Test Priority

| Priority | Test Group | Covered Test IDs | Rationale |
|----------|------------|------------------|-----------|
| P0 (Required) | Group 1 (writability), Group 2 (VA=PA passthrough) | SVBARE-CSR-01~05, SVBARE-VA-01~06 | Svbare core: MODE can hold Bare and VA=PA under Bare |
| P1 (Important) | Group 3 (no page table translation), Group 4 (PMP interaction), Group 5 (MODE switching) | SVBARE-NOPT-01~05, SVBARE-PMP-01~06, SVBARE-SW-01~05 | Bare semantic boundaries, the only protection mechanism, switching stability |
| P2 (Recommended) | Group 6 (special scenarios) | SVBARE-EDGE-01~05 | Implementation-defined and boundary conditions |

---

## Result Determination Principles

- If the platform behavior deviates from the SPEC (e.g., satp.MODE cannot hold Bare, a page-fault is produced under Bare, writing an unsupported MODE nevertheless modifies satp, etc.): keep the test case failing, compare against the SPEC, and record the issue in the `bugs/` directory. Modifying the test case or adding a workaround to accommodate an incorrect implementation is prohibited.
- UNSPECIFIED / implementation-defined scenarios (SVBARE-CSR-05, SVBARE-EDGE-02): record the actual observed behavior without making strong assertions.

---

## Appendix: Normative References

### satp Register RV64 Layout

```
 63    60 59    44 43                 0
+--------+--------+-------------------+
|  MODE  |  ASID  |       PPN         |
| (4 bit)|(16 bit)|     (44 bit)      |
+--------+--------+-------------------+
```

### satp MODE Encoding Table

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

### Related scause Constants

| Constant | Value | Description |
|----------|-------|-------------|
| Instruction access fault | 1 | Instruction access fault (PMP) |
| Illegal instruction | 2 | Illegal instruction |
| Load access fault | 5 | Load access fault (PMP) |
| Store/AMO access fault | 7 | Store/AMO access fault (PMP) |
| Instruction page fault | 12 | Instruction page fault |
| Load page fault | 13 | Load page fault |
| Store/AMO page fault | 15 | Store/AMO page fault |

### Related mstatus/sstatus Bits

| Bit | Name | Description |
|-----|------|-------------|
| bit 18 | SUM | Permit Supervisor User Memory access |
| bit 19 | MXR | Make eXecutable Readable |
| bit 20 | TVM | Trap Virtual Memory (traps S-mode access to satp) |

---

## References

- `supervisor.adoc` — `satp` register, Bare mode, Svbare extension, `sstatus`.SUM/MXR definitions
- `Svadu_test_plan.md`, `Svnapot_test_plan.md` — Other Sv* extension test plans
- `Hypervisor_Sv_test_plan.md` — Hypervisor x Sv* cross test plan

---

## Appendix A: Specification Point Coverage Matrix

The table below indicates which test cases cover each specification point listed in the "Covered Specification Points" section.

| Norm ID | Covered Test IDs |
|---------|------------------|
| `norm:svbare_satp_mode_bare` | SVBARE-CSR-01~05, SVBARE-SW-01~05 |
| `norm:satp_mode` | SVBARE-CSR-02, SVBARE-CSR-05, SVBARE-VA-01~06, SVBARE-NOPT-01~05, SVBARE-PMP-01~06 |
| `norm:satp_mode_op_unsupported` | SVBARE-SW-02, SVBARE-SW-03, SVBARE-EDGE-02 |
| `norm:satp_op_active` | SVBARE-VA-01~06, SVBARE-EDGE-01, SVBARE-EDGE-04, SVBARE-EDGE-05 |
| `norm:sstatus_sum` | SVBARE-NOPT-02, SVBARE-NOPT-03 |
| `norm:sstatus_mxr` | SVBARE-NOPT-04 |
