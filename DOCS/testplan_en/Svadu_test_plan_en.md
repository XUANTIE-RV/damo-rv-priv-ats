**[中文](../testplan/Svadu_test_plan.md) | English**

# Svadu Extension Test Plan

This document describes the test plan for the Svadu (Hardware Updating of A/D Bits) extension. The Svadu extension provides support for **hardware automatic updating** of PTE A/D bits, and allows enabling/disabling this behavior at runtime through the `menvcfg.ADUE` field; when hardware updating is disabled, the processor falls back to Svade behavior (A/D triggers a page-fault, set by software).

---

## SPEC Sections Covered by This Document

This plan is based on the following official RISC-V specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/svadu.adoc` — Svadu extension definition, `menvcfg.ADUE` writability, ADUE=0 fallback to Svade semantics
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — `menvcfg` CSR field encoding (ADUE=bit 61) and its hardware A/D update control semantics, the SFENCE.VMA synchronization requirement after modifying ADUE
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — Svade concept and virtual address translation algorithm step 9 (where A/D triggers a page-fault)

Official repository:

- https://github.com/riscv/riscv-isa-manual (the above path files within the repository)

---

## Overview

The RISC-V Privileged Specification defines two PTE A/D bit management schemes:

1. **Hardware update scheme (Svadu enabled)**: When a page with A=0 is accessed or a page with D=0 is written, the hardware atomically updates the PTE A/D bits, the access proceeds, and no exception is raised.
2. **Software update scheme (Svade behavior)**: When a page with A=0 is accessed or a page with D=0 is written, the hardware raises a page-fault exception, and the software trap handler explicitly sets the bit and retries.

The Svadu extension serves as a **unified control layer** for both schemes:

- When Svadu is implemented, the `menvcfg.ADUE` (bit 61) field is writable (WARL).
- When `menvcfg.ADUE=1`, the processor is in "hardware update" mode: accessing a PTE with A=0 / D=0 in S/U-mode causes the hardware to atomically update the corresponding bit, and the access completes successfully.
- When `menvcfg.ADUE=0`, the processor is in "Svade mode": the hardware does not update A/D, the access triggers a page-fault, which is fully consistent with the standalone Svade extension behavior.

For a processor implementing the Svadu extension, step 9 of the virtual address translation process (see `supervisor.adoc`) selects one of two paths based on `menvcfg.ADUE` (see `machine.adoc`):
- ADUE=0: behavior is equivalent to Svade -- when `pte.a=0` or (store and `pte.d=0`) is detected, translation stops and a page-fault corresponding to the original access type is raised;
- ADUE=1: the hardware atomically sets `pte.a` to 1, and for stores also sets `pte.d` to 1, then continues translation without raising a page-fault.

This test plan focuses on the CSR control behavior of this extension, access semantics under ADUE=1 / ADUE=0 modes, coverage across different page granularities (4 KiB / 2 MiB / 1 GiB), and behavioral changes after dynamically switching ADUE at runtime.

---

## Covered Specification Points

The table below lists the specification points covered by this plan. Entries with the `norm:` prefix are official SPEC normative rule tags.

| Norm ID | Original Text | Description |
|---------|---------------|-------------|
| `norm:Svadu_hw_update_a_d_bits` | If the Svadu extension is implemented, the `menvcfg`.ADUE field is writable. | If the Svadu extension is implemented, the `menvcfg`.ADUE field is writable. |
| `norm:menvcfg_adue_rdonly0` | If Svadu is not implemented, ADUE is read-only zero. | If Svadu is not implemented, ADUE is read-only zero. |
| `norm:menvcfg_adue_op` | If the Svadu extension is implemented, the ADUE bit controls whether hardware updating of PTE A/D bits is enabled for S-mode and G-stage address translations. When ADUE=1, hardware updating is enabled and the implementation behaves as though Svade were not implemented; when ADUE=0, the implementation behaves as though Svade were implemented. | The ADUE bit controls whether hardware A/D bit updating is enabled for S-mode translation: ADUE=1 enables it (as though Svade were not implemented), ADUE=0 disables it (as though Svade were implemented). |
| `norm:Svadu_disabled_hw_update_falls_back_to_svade` | When hardware updating of A/D bits is disabled, the Svade extension, which mandates exceptions when A/D bits need be set, instead takes effect. | When hardware updating of A/D bits is disabled, the Svade extension takes effect, which mandates exceptions when the A/D bits need to be set. |
| `norm:svade_access_ad_bit_clear` | The Svade extension: when a virtual page is accessed and the A bit is clear, or is written and the D bit is clear, a page-fault exception is raised. | Svade extension: a page-fault exception is raised when a virtual page is accessed with A=0, or written with D=0. |
| `norm:menvcfg_adue_fence` | After changing `menvcfg`.ADUE, executing an SFENCE.VMA instruction with rs1=`x0` and rs2=`x0` suffices to synchronize address-translation caches with respect to the altered interpretation of page-table entries' A/D bits. | After modifying ADUE, SFENCE.VMA(x0,x0) must be executed to ensure the address-translation caches are synchronized. |

---

## Out of Scope

- **Hypervisor two-stage translation**: `henvcfg.ADUE` writability, Svadu behavior under VS-stage and G-stage, and HLV/HSV instruction interactions are covered by `Hypervisor_Sv_test_plan.md`.
- **Sv32 / Sv48 / Sv57 modes**: This plan takes Sv39 as the primary coverage target; ADUE semantics are identical under the other Sv modes.
- **Multi-hart consistency**: The specification requires all harts to use the same PTE update scheme; this plan focuses on single-hart behavior.
- **PMP / Smepmp and Svadu interactions**: Related interactions are independently covered by the PMP test plan series.

---

## Preconditions and Detection Strategy

1. **Extension detection**: Svadu implementability is determined by the writability of `menvcfg.ADUE` -- M-mode writes ADUE=1 then reads back bit 61; per `norm:menvcfg_adue_rdonly0`, ADUE is read-only zero when Svadu is not implemented, so reading back 0 determines it is not implemented. This can be further confirmed functionally by "an A=0 leaf PTE load succeeding under ADUE=1 with PTE.A set to 1 by hardware".
2. **Skip strategy**: Upon detection failure, all test groups that depend on Svadu behavior (Groups 2/3/4/5/7) and Group 6 which verifies fallback semantics are uniformly `TEST_SKIP`ped; the ADUE writability cases in Group 1 are themselves the detection means, and their failure is itself evidence that the platform lacks Svadu, so it must be reported explicitly rather than silently passing.
3. **ADUE switching synchronization**: `menvcfg` is only writable from M-mode, so all ADUE switching is performed in M-mode; after switching, SFENCE.VMA(x0,x0) is executed per `norm:menvcfg_adue_fence` before entering S-mode for verification.

---

## Test Groups

### Group 1: menvcfg.ADUE Field Control Tests

**Specification basis**:
- `norm:Svadu_hw_update_a_d_bits`: `menvcfg.ADUE` must be writable when Svadu is implemented
- `norm:menvcfg_adue_rdonly0`: ADUE is read-only zero when Svadu is not implemented

**Test responsibilities**: Verify the writability, read-write consistency, non-interference with other fields, and reset initial value of `menvcfg.ADUE` (bit 61) in M-mode. This is the most basic check for Svadu implementability.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADU-CSR-01 | menvcfg.ADUE writable 0->1 | M-mode writes ADUE=1, reads back menvcfg | Read-back bit 61 = 1 (reads back 0 as read-only zero when Svadu is not implemented) |
| SVADU-CSR-02 | menvcfg.ADUE writable 1->0 | Set ADUE=1 first, then clear ADUE=0, read back | Read-back bit 61 = 0 |
| SVADU-CSR-03 | ADUE does not affect other fields | Record PBMTE/STCE/CBIE/FIOM and other fields while toggling ADUE | Other field values remain unchanged before and after the ADUE toggle |
| SVADU-CSR-04 | ADUE reset value record | Save the original menvcfg value at the test suite entry, read and record its bit 61 (informational) | Record the reset value (the specification does not define a reset value; observe only) |

---

### Group 2: Hardware A Bit Update on 4 KiB Leaf PTEs with ADUE=1

**Specification basis**:
- `norm:menvcfg_adue_op`: When ADUE=1, S-mode translation enables hardware A/D updating and behaves as though Svade were not implemented (A=0 access no longer triggers a page-fault; the hardware atomically sets A=1)

**Test responsibilities**: Verify that on 4 KiB leaf PTEs with ADUE=1, various accesses (load, instruction fetch) to pages with A=0 do not trigger a page-fault, and that PTE.A has been set to 1 by hardware upon returning to M-mode.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADU-A4K-01 | A=0 R page load | ADUE=1, 4 KiB PTE: V=1, R=1, A=0, D=0, S-mode load | Load succeeds (result=0), PTE.A=1 |
| SVADU-A4K-02 | A=0 X page fetch | ADUE=1, 4 KiB PTE: V=1, X=1, A=0, S-mode jump and execute | Instruction fetch succeeds, PTE.A=1 |
| SVADU-A4K-03 | A=0 RW page load | ADUE=1, 4 KiB PTE: V=1, R=1, W=1, A=0, D=1, S-mode load | Load succeeds, PTE.A=1, PTE.D remains 1 (load only does not affect D) |
| SVADU-A4K-04 | A set only once after multiple loads | ADUE=1, perform N loads on an A=0 page | Every load succeeds, final PTE.A=1 (subsequent loads trigger no additional side effects) |

---

### Group 3: Hardware D Bit Update on 4 KiB Leaf PTEs with ADUE=1

**Specification basis**:
- `norm:menvcfg_adue_op`: When ADUE=1, the hardware sets PTE.D=1 after store/AMO; a D=0 store no longer triggers a page-fault

**Test responsibilities**: Verify that on 4 KiB leaf PTEs with ADUE=1, store and AMO operations on pages with D=0 do not trigger a page-fault, and that PTE.D has been set to 1 by hardware upon returning to M-mode. Key focus: verifying that a single store access with A=0+D=0 can simultaneously set both flags.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADU-D4K-01 | A=1 D=0 RW page store | ADUE=1, PTE: V=1, R=1, W=1, A=1, D=0, S-mode store | Store succeeds, PTE.D=1 (A remains 1) |
| SVADU-D4K-02 | A=0 D=0 RW page store | ADUE=1, PTE: V=1, R=1, W=1, A=0, D=0, S-mode store | Store succeeds, PTE.A=1 and PTE.D=1 (a single store sets both) |
| SVADU-D4K-03 | A=1 D=0 RW page amoadd | ADUE=1, PTE: V=1, R=1, W=1, A=1, D=0, S-mode amoadd.w | AMO succeeds, PTE.D=1 |
| SVADU-D4K-04 | A=1 D=1 RW page store (no side effect) | ADUE=1, PTE: A=1, D=1, S-mode store | Store succeeds, PTE.A and PTE.D remain 1 |

> [!IMPORTANT]
> SVADU-D4K-02 verifies that a single store atomically sets both A=1 and D=1 in hardware, which is the key difference between Svadu and the software "set A first, then trigger a D fault" two-step approach.

---

### Group 4: Hardware A/D Update on 2 MiB Megapages with ADUE=1

**Specification basis**:
- `norm:menvcfg_adue_op`: Hardware A/D update is performed on leaf PTEs; 2 MiB megapages are equally applicable

**Test responsibilities**: Verify that hardware A/D update behavior with ADUE=1 also applies to 2 MiB megapages (level 1 leaf PTE in Sv39), covering load / store / fetch / AMO access types.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADU-2M-01 | 2M A=0 load | 2 MiB leaf PTE: V=1, R=1, A=0, D=0, S-mode load | Load succeeds, megapage PTE.A=1 |
| SVADU-2M-02 | 2M A=1 D=0 store | 2 MiB leaf PTE: V=1, R=1, W=1, A=1, D=0, S-mode store | Store succeeds, megapage PTE.D=1 |
| SVADU-2M-03 | 2M A=0 X page fetch | 2 MiB leaf PTE: V=1, X=1, A=0, S-mode jump and execute | Instruction fetch succeeds, megapage PTE.A=1 |
| SVADU-2M-04 | 2M A=0 D=0 store | A single store sets both bits | Store succeeds, PTE.A=1 and PTE.D=1 |
| SVADU-2M-05 | 2M A=1 D=0 amoadd.w | 2 MiB leaf PTE: V=1, R=1, W=1, A=1, D=0, S-mode amoadd.w | AMO succeeds, megapage PTE.D=1 |

---

### Group 5: Hardware A/D Update on 1 GiB Gigapages with ADUE=1

**Specification basis**:
- `norm:menvcfg_adue_op`: Hardware A/D update is performed on leaf PTEs; 1 GiB gigapages are equally applicable

**Test responsibilities**: Verify that hardware A/D update behavior with ADUE=1 also applies to 1 GiB gigapages (level 2 leaf PTE in Sv39), covering load / store / AMO access types. A 1 GiB X-permission page typically spans the entire code segment, making it impractical as a standalone fetch test case; fetch testing is sufficiently covered by SVADU-2M-03 and SVADU-A4K-02.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADU-1G-01 | 1G A=0 load | 1 GiB leaf PTE: V=1, R=1, A=0, D=0, S-mode load | Load succeeds, gigapage PTE.A=1 |
| SVADU-1G-02 | 1G A=1 D=0 store | 1 GiB leaf PTE: V=1, R=1, W=1, A=1, D=0, S-mode store | Store succeeds, gigapage PTE.D=1 |
| SVADU-1G-03 | 1G A=0 D=0 store | A single store sets both A and D | Store succeeds, PTE.A=1 and PTE.D=1 |
| SVADU-1G-04 | 1G A=1 D=0 amoadd.w | 1 GiB leaf PTE: V=1, R=1, W=1, A=1, D=0, S-mode amoadd.w | AMO succeeds, gigapage PTE.D=1 |

> [!NOTE]
> 1 GiB gigapage tests require ensuring the test VA is chosen in an identity-mapped region far from the M-mode code segment and stack, to avoid conflicts with the 1 GiB region containing the code segment.

---

### Group 6: Fallback to Svade Behavior with ADUE=0

**Specification basis**:
- `norm:Svadu_disabled_hw_update_falls_back_to_svade`: When hardware A/D updating is disabled, Svade behavior takes effect
- `norm:svade_access_ad_bit_clear`: A page-fault is triggered when A=0 is accessed or D=0 is written

**Test responsibilities**: Test that when `menvcfg.ADUE=0`, a processor implementing Svadu behaves identically to Svade -- A=0 / D=0 triggers a page-fault and the PTE content remains unchanged. This group's test cases mirror Groups 1/2/3 of `Svade_test_plan.md`.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADU-FB-01 | ADUE=0 A=0 load | ADUE=0, 4 KiB PTE: V=1, R=1, A=0, S-mode load | Load page-fault (scause=13) |
| SVADU-FB-02 | ADUE=0 D=0 store | ADUE=0, 4 KiB PTE: V=1, R=1, W=1, A=1, D=0, S-mode store | Store page-fault (scause=15) |
| SVADU-FB-03 | ADUE=0 A=0 fetch | ADUE=0, 4 KiB PTE: V=1, X=1, A=0, S-mode jump and execute | Instruction page-fault (scause=12) |
| SVADU-FB-04 | ADUE=0 AMO D=0 | ADUE=0, 4 KiB PTE: V=1, R=1, W=1, A=1, D=0, S-mode amoadd.w | Store/AMO page-fault (scause=15) |
| SVADU-FB-05 | ADUE=0 PTE unchanged after fault | After triggering any of the above faults, return to M-mode and check the PTE | The corresponding A/D bits retain their original values (not updated by hardware) |

> [!IMPORTANT]
> Group 6 is the "ADUE=0 mirror" of Groups 1/2/3 in `Svade_test_plan.md`, verifying that a Svadu implementation with hardware updating disabled must be equivalent to Svade. Any test that passes under a Svade implementation must also pass under the Svadu + ADUE=0 configuration.

---

### Group 7: Runtime Dynamic ADUE Switching

**Specification basis**:
- `norm:menvcfg_adue_fence`: After modifying `menvcfg.ADUE`, SFENCE.VMA(x0,x0) must be executed for the new setting to take effect
- `norm:menvcfg_adue_op`: The ADUE value determines whether the hardware update path or the Svade path is currently adopted

**Test responsibilities**: Verify that after dynamically switching `menvcfg.ADUE` in M-mode at runtime and executing SFENCE.VMA, S-mode access behavior immediately follows the new configuration. Covers 0->1, 1->0 switching and multi-switch stability.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADU-SW-01 | 0->1 switch: enable HW update after fault | M-mode sets ADUE=0, S-mode triggers an A=0 load fault -> return to M-mode, set ADUE=1 + SFENCE.VMA -> the same PTE is unchanged (PTE.A still 0), re-enter S-mode to access the same VA | First: scause=13; second: load succeeds, PTE.A=1 (set by hardware) |
| SVADU-SW-02 | 1->0 switch: disable HW update | M-mode sets ADUE=1, S-mode accesses an A=0 page P1 completing the HW update -> return to M-mode, set ADUE=0 + SFENCE.VMA -> map a new A=0 page P2 -> S-mode accesses P2 | P1 access succeeds and P1.A=1; P2 access triggers scause=13 and P2.A remains 0 |
| SVADU-SW-03 | SFENCE.VMA required after switch | Set ADUE=0 -> immediately set ADUE=1 -> execute SFENCE.VMA -> S-mode accesses an A=0 page | Access succeeds, PTE.A=1 (verifies correct effect after the final fence) |
| SVADU-SW-04 | Multi-switch stability | Toggle ADUE between 0/1 >= 4 times, after each switch + SFENCE.VMA access an A=0 page with a fresh PTE context | After each switch, behavior strictly matches the current ADUE value (ADUE=1 succeeds in setting bits, ADUE=0 triggers a fault) |

> [!NOTE]
> Regarding "whether ADUE modification is immediately visible without SFENCE.VMA" -- `norm:menvcfg_adue_fence` explicitly requires executing a synchronization instruction. This means implementations may return stale behavior without a fence; this test plan adopts a **conservative policy**, enforcing SFENCE.VMA after every ADUE switch, and does not test unfenced edge behavior (to avoid coupling tests with implementation-defined behavior).

---

## Test Priority

| Priority | Test Group | Covered Test IDs | Rationale |
|----------|------------|------------------|-----------|
| P0 (Required) | Group 1 (ADUE control), Group 2 (4K A bit), Group 3 (4K D bit) | SVADU-CSR-01~04, SVADU-A4K-01~04, SVADU-D4K-01~04 | Extension determination and core hardware update semantics |
| P1 (Important) | Group 6 (ADUE=0 fallback), Group 7 (runtime switching) | SVADU-FB-01~05, SVADU-SW-01~04 | Svade fallback equivalence and ADUE dynamic switching + SFENCE synchronization |
| P2 (Recommended) | Group 4 (2 MiB), Group 5 (1 GiB) | SVADU-2M-01~05, SVADU-1G-01~04 | Large-page granularity coverage |

---

## Result Determination Principles

- If the platform does not implement Svadu (ADUE read-only zero): the Group 1 detection cases explicitly report "not implemented", the remaining groups are SKIPped, and this is not treated as a failure (Svadu is an optional extension).
- If the platform implements Svadu but its behavior deviates from the SPEC (e.g., A/D not updated under ADUE=1, no fallback to Svade under ADUE=0, asserting effect without a fence after switching, etc.): keep the test case failing, compare against the SPEC, and record the issue in the `bugs/` directory. Modifying the test case or adding a workaround to accommodate an incorrect implementation is prohibited.

---

## Appendix: Normative References

### menvcfg.ADUE Field

| Field | Position | Meaning |
|-------|----------|---------|
| `ADUE` | menvcfg[61] | Enable hardware updating of PTE A/D bits (0=disabled, falls back to Svade; 1=hardware updating enabled) |

### Related scause Constants

| Constant | Value | Description |
|----------|-------|-------------|
| Instruction page fault | 12 | Instruction page fault |
| Load page fault | 13 | Load page fault |
| Store/AMO page fault | 15 | Store/AMO page fault |

---

## References

- `svadu.adoc` — Svadu extension definition
- `machine.adoc` — `menvcfg` CSR and ADUE field semantics
- `supervisor.adoc` — Svade concept and translation algorithm
- `Svade_test_plan.md` — Standalone Svade test plan (Group 6 fallback reference)
- `Hypervisor_Sv_test_plan.md` — Hypervisor x Svadu cross test plan

---

## Appendix A: Specification Point Coverage Matrix

The table below indicates which test cases cover each specification point listed in the "Covered Specification Points" section.

| Norm ID | Covered Test IDs |
|---------|------------------|
| `norm:Svadu_hw_update_a_d_bits` | SVADU-CSR-01~04 |
| `norm:menvcfg_adue_rdonly0` | SVADU-CSR-01 |
| `norm:menvcfg_adue_op` | SVADU-A4K-01~04, SVADU-D4K-01~04, SVADU-2M-01~05, SVADU-1G-01~04, SVADU-FB-01~05 |
| `norm:Svadu_disabled_hw_update_falls_back_to_svade` | SVADU-FB-01~05, SVADU-SW-01, SVADU-SW-02 |
| `norm:svade_access_ad_bit_clear` | SVADU-FB-01~04 |
| `norm:menvcfg_adue_fence` | SVADU-SW-01~04 |
