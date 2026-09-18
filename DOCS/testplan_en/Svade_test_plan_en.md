**[中文](../testplan/Svade_test_plan.md) | English**

# Svade Extension Test Plan

This document describes the test plan for the Svade (Page-Fault Exceptions on A/D Bit Updates) extension. The Svade extension specifies that when a virtual page is accessed with PTE.A=0, or is written with PTE.D=0, the hardware shall **not atomically update** the A/D bits; instead, a page-fault exception is raised, and software is responsible for setting the A/D bits before retrying.

---

## SPEC Sections Covered by This Document

This plan is based on the following official RISC-V specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — Svade extension definition, PTE A/D bit semantics, virtual address translation algorithm step 9 (where A/D triggers a page-fault)

Official repository:

- https://github.com/riscv/riscv-isa-manual (the above path file within the repository)

---

## Overview

The RISC-V Privileged Specification defines two PTE A/D bit management schemes:

1. **Non-Svade scheme** (default hardware update): When A=0 on access or D=0 on write, the hardware atomically updates the PTE's A/D bits.
2. **Svade scheme**: When A=0 on access or D=0 on write, the hardware raises a page-fault exception. The PTE's A/D bits remain unchanged, and a software trap handler explicitly sets the bits before retrying the access.

For processors implementing the Svade extension, step 9 of the virtual address translation process (see the translation algorithm in `supervisor.adoc`) checks `pte.a` and `pte.d`:

- If `pte.a=0`, or the original access is a store and `pte.d=0`, **translation is halted and a page-fault exception matching the original access type is raised**.

This test plan focuses on coverage of this specification requirement across different access types (load / store / instruction fetch / AMO) and different page granularities (4 KiB / 2 MiB / 1 GiB).

---

## Covered Specification Points

The table below lists the specification points covered by this plan. Entries with the `norm:` prefix are official SPEC normative rule tags; entries without the prefix are specification points summarized by this plan from the SPEC text.

| Norm ID | Original Text | Description |
|---------|---------------|-------------|
| `norm:svade_access_ad_bit_clear` | The Svade extension: when a virtual page is accessed and the A bit is clear, or is written and the D bit is clear, a page-fault exception is raised. | Svade extension: a page-fault exception is raised when a virtual page is accessed with A=0, or written with D=0. |
| `Svade_store_d_bit_clear_pagefault` | — | A store/AMO page-fault is raised when a virtual page is written with PTE.D=0 |
| `Svade_no_hw_update_ad` | — | Under a Svade implementation the hardware must not automatically set the A/D bits; the PTE content remains unchanged |
| `Svade_pagefault_cause_match_access_type` | — | The scause of the page-fault must match the original access type (load=13, store=15, fetch=12) |
| `Svade_applies_all_levels` | — | The A/D check is performed on leaf PTEs; it applies to all three granularities: 4 KiB / 2 MiB megapage / 1 GiB gigapage |
| `Svade_software_set_ad_then_access` | — | Access should succeed after software pre-sets A=1 (and also D=1 for the store case) in the PTE |
| `Svade_non_leaf_ad_reserved` | — | The A/D bits of non-leaf PTEs are reserved and do not participate in the Svade check (the leaf PTE A/D determines behavior) |

---

## Out of Scope

- **Hypervisor two-stage translation**: Svade behavior under VS-stage and G-stage is covered by `Hypervisor_Sv_test_plan.md`.
- **Svadu interaction**: `menvcfg.ADUE` / `henvcfg.ADUE` controlling hardware A/D update and Svade switching behavior is covered by `Svadu_test_plan.md`.
- **Multi-hart consistency**: The specification requires all harts to adopt the same PTE update scheme; this plan focuses on single-hart behavior.
- **Sv32 / Sv48 / Sv57 modes**: This plan takes Sv39 as the primary coverage target; A/D semantics are identical under the other Sv modes.

---

## Test Groups

### Group 1: A Bit Clear Triggers Load / Fetch Page-Fault (4 KiB)

**Spec Reference**:
- `norm:svade_access_ad_bit_clear`: Page-fault when A=0 on access
- `Svade_software_set_ad_then_access`: Access succeeds when A=1
- `Svade_pagefault_cause_match_access_type`: Load triggers scause=13, fetch triggers scause=12

**Test Scope**: Verify that on a 4 KiB leaf PTE with A=0, all read-type accesses (load, instruction fetch) trigger the corresponding page-fault type; access succeeds when A=1.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADE-A-01 | A=0 R-only page load | 4 KiB PTE: V=1, R=1, A=0, D=0, S-mode load | load page-fault (scause=13) |
| SVADE-A-02 | A=0 with D=1 load | 4 KiB PTE: V=1, R=1, A=0, D=1, S-mode load | load page-fault (D=1 does not affect the A check) |
| SVADE-A-03 | A=1 load succeeds | 4 KiB PTE: V=1, R=1, A=1, D=0, S-mode load | Read succeeds, no exception |
| SVADE-A-04 | A=0 X-only page fetch | 4 KiB PTE: V=1, R=0, X=1, A=0, S-mode jump-and-execute | instruction page-fault (scause=12) |
| SVADE-A-05 | A=1 X page fetch succeeds | 4 KiB PTE: V=1, R=0, X=1, A=1, S-mode jump-and-execute | Fetch and execute succeed |
| SVADE-A-06 | A=0 RW page load | 4 KiB PTE: V=1, R=1, W=1, A=0, D=0, S-mode load | load page-fault (scause=13) |

---

### Group 2: D Bit Clear Triggers Store Page-Fault (4 KiB)

**Spec Reference**:
- `Svade_store_d_bit_clear_pagefault`: Page-fault when D=0 on store
- `Svade_pagefault_cause_match_access_type`: Store triggers scause=15
- `Svade_software_set_ad_then_access`: Store succeeds when D=1

**Test Scope**: Verify that on a 4 KiB leaf PTE with D=0, store / AMO accesses trigger a store/AMO page-fault; load is not affected by the D bit; store succeeds when D=1.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADE-D-01 | D=0 RW page store | 4 KiB PTE: V=1, R=1, W=1, A=1, D=0, S-mode store | store page-fault (scause=15) |
| SVADE-D-02 | D=0 RW page load | 4 KiB PTE: V=1, R=1, W=1, A=1, D=0, S-mode load | Read succeeds (D does not affect load) |
| SVADE-D-03 | D=1 RW page store | 4 KiB PTE: V=1, R=1, W=1, A=1, D=1, S-mode store | Write succeeds |
| SVADE-D-04 | A=0 D=0 RW page store | 4 KiB PTE: V=1, R=1, W=1, A=0, D=0, S-mode store | store page-fault (scause=15, A also missing triggers the fault) |
| SVADE-D-05 | D=0 AMO operation | 4 KiB PTE: V=1, R=1, W=1, A=1, D=0, S-mode `amoadd.w` | store/AMO page-fault (scause=15) |

---

### Group 3: Hardware Does Not Update A/D Bits

**Spec Reference**:
- `Svade_no_hw_update_ad`: Under Svade, the hardware shall not automatically set the A/D bits after triggering a page-fault
- `Svade_software_set_ad_then_access`: Access succeeds after software sets the A/D bits + SFENCE.VMA

**Test Scope**: Verify the fundamental distinction between the Svade and non-Svade (hardware update) schemes — after an A/D-triggered page-fault, the PTE content remains unchanged; retry succeeds after software sets the bits.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADE-NOUPD-01 | PTE unchanged after A=0 fault | After triggering SVADE-A-01, return to M-mode and read the PTE to check the A bit | PTE.A remains 0 |
| SVADE-NOUPD-02 | PTE unchanged after D=0 fault | After triggering SVADE-D-01, return to M-mode and read the PTE to check the D bit | PTE.D remains 0 |
| SVADE-NOUPD-03 | Software sets A=1 and retries load | A=0 load fault -> trap handler sets A=1 + SFENCE.VMA -> retry | Read succeeds |
| SVADE-NOUPD-04 | Software sets D=1 and retries store | D=0 store fault -> trap handler sets D=1 + SFENCE.VMA -> retry | Write succeeds |
| SVADE-NOUPD-05 | Multiple loads on the same A=0 page | Load the A=0 page N consecutive times | Each triggers a page-fault (the PTE never changes) |

---

### Group 4: A/D Behavior on 2 MiB Megapage

**Spec Reference**:
- `Svade_applies_all_levels`: The A/D check is performed on leaf PTEs; a 2 MiB megapage is equally applicable

**Test Scope**: Verify that Svade behavior also applies to a 2 MiB megapage (level 1 leaf PTE in Sv39).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADE-2M-01 | 2M A=0 load fault | 2 MiB leaf PTE: V=1, R=1, A=0, S-mode load | load page-fault (scause=13) |
| SVADE-2M-02 | 2M D=0 store fault | 2 MiB leaf PTE: V=1, R=1, W=1, A=1, D=0, S-mode store | store page-fault (scause=15) |
| SVADE-2M-03 | 2M A=1 D=1 access succeeds | 2 MiB leaf PTE: V=1, R=1, W=1, A=1, D=1, S-mode load + store | Both read and write succeed |
| SVADE-2M-04 | 2M A=0 fetch fault | 2 MiB leaf PTE: V=1, X=1, A=0, S-mode jump-and-execute | instruction page-fault (scause=12) |
| SVADE-2M-05 | 2M multi-offset access within region | A=0 megapage, access offsets 0, 0x1000, 0x100000, 0x1FF000 | Each access triggers a page-fault |

---

### Group 5: A/D Behavior on 1 GiB Gigapage

**Spec Reference**:
- `Svade_applies_all_levels`: The A/D check is performed on leaf PTEs; a 1 GiB gigapage is equally applicable

**Test Scope**: Verify that Svade behavior also applies to a 1 GiB gigapage (level 2 leaf PTE in Sv39).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADE-1G-01 | 1G A=0 load fault | 1 GiB leaf PTE: V=1, R=1, A=0, S-mode load | load page-fault (scause=13) |
| SVADE-1G-02 | 1G D=0 store fault | 1 GiB leaf PTE: V=1, R=1, W=1, A=1, D=0, S-mode store | store page-fault (scause=15) |
| SVADE-1G-03 | 1G A=1 D=1 access succeeds | 1 GiB leaf PTE: V=1, R=1, W=1, A=1, D=1, S-mode load + store | Both read and write succeed |
| SVADE-1G-04 | 1G multi-offset access within region | A=0 gigapage, access multiple offsets within the region | Each access triggers a page-fault |

> [!NOTE]
> 1 GiB gigapage tests require that the test VA be chosen in an identity-mapped region far from the M-mode code segment and stack, to avoid conflicting with the 1 GiB region containing the code segment.

---

### Group 6: Non-Leaf PTE A/D Bits Do Not Participate in the Svade Check

**Spec Reference**:
- `Svade_non_leaf_ad_reserved`: The D, A, and U bits in non-leaf PTEs are reserved for future standard use and should be cleared by software; the Svade check is performed only on leaf PTEs

**Test Scope**: Verify that the Svade A/D check applies only to leaf PTEs, independent of the A/D bits on intermediate non-leaf PTEs.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADE-NL-01 | Non-leaf A=0, leaf A=1 load | Mid-level PTE A=0, D=0 (standard usage), 4 KiB leaf PTE A=1, S-mode load | Access succeeds (leaf PTE A=1 determines) |
| SVADE-NL-02 | Non-leaf A=0, leaf A=0 load | Mid-level PTE A=0, leaf PTE A=0, S-mode load | load page-fault (leaf PTE A=0 triggers) |
| SVADE-NL-03 | Non-leaf D=0, leaf D=1 store | Mid-level PTE D=0, leaf PTE A=1, D=1, W=1, S-mode store | Write succeeds (leaf PTE D=1 determines) |

---

### Group 7: scause Correspondence with Access Type

**Spec Reference**:
- `Svade_pagefault_cause_match_access_type`: The scause of a page-fault must correspond to the original access type

**Test Scope**: Verify that the scause of Svade-triggered page-faults is 13 / 15 / 12 / 15 for load / store / fetch / AMO access types respectively.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADE-CAUSE-01 | load -> scause=13 | Execute `lw` on an A=0 R page | scause = 13 (load page-fault) |
| SVADE-CAUSE-02 | store -> scause=15 | Execute `sw` on an A=1 D=0 RW page | scause = 15 (store page-fault) |
| SVADE-CAUSE-03 | fetch -> scause=12 | Jump and execute on an A=0 X page | scause = 12 (instruction page-fault) |
| SVADE-CAUSE-04 | AMO -> scause=15 | Execute `amoadd.w` on an A=1 D=0 RW page | scause = 15 (store/AMO page-fault) |
| SVADE-CAUSE-05 | A=0 store -> scause=15 | Execute `sw` on an A=0 D=0 RW page | scause = 15 (store type takes priority) |

> [!IMPORTANT]
> SVADE-CAUSE-05 verifies that when a store operation simultaneously triggers both A=0 and D=0, since step 9's condition is "A=0 or (store and D=0)", the entire determination belongs to a store access, so scause should be store page-fault (15), not load page-fault (13).

---

## Test Priority

| Priority | Test Group | Covered Test IDs | Rationale |
|----------|------------|------------------|-----------|
| P0 (Required) | Group 1 (A bit), Group 2 (D bit), Group 7 (scause correspondence) | SVADE-A-01~06, SVADE-D-01~05, SVADE-CAUSE-01~05 | Core Svade semantics: A/D clear triggers the corresponding page-fault type |
| P1 (Important) | Group 3 (hardware does not update) | SVADE-NOUPD-01~05 | The fundamental distinction between Svade and the hardware update scheme |
| P2 (Recommended) | Group 4 (2 MiB), Group 5 (1 GiB), Group 6 (non-leaf PTE) | SVADE-2M-01~05, SVADE-1G-01~04, SVADE-NL-01~03 | Large-page granularity coverage and non-leaf PTE boundaries |

---

## Result Determination Principles

- If the platform implements Svade but its behavior deviates from the SPEC (e.g., A=0/D=0 access does not trigger a page-fault, the hardware updates the A/D bits on its own, scause does not match the access type, etc.): keep the test case failing, compare against the SPEC, and record the issue in the `bugs/` directory. Modifying the test case or adding a workaround to accommodate an incorrect implementation is prohibited.

---

## References

- `supervisor.adoc` — Svade extension definition, PTE A/D bit semantics, virtual address translation algorithm
- `Svadu_test_plan.md` — Hardware A/D update extension test plan
- `Hypervisor_Sv_test_plan.md` — Hypervisor x Sv* cross test plan

---

## Appendix A: Specification Point Coverage Matrix

The table below indicates which test cases cover each specification point listed in the "Covered Specification Points" section.

| Norm ID | Covered Test IDs |
|---------|------------------|
| `norm:svade_access_ad_bit_clear` | SVADE-A-01~06, SVADE-D-01~05, SVADE-2M-01~05, SVADE-1G-01~04 |
| `Svade_store_d_bit_clear_pagefault` | SVADE-D-01, SVADE-D-04, SVADE-D-05, SVADE-2M-02, SVADE-1G-02, SVADE-CAUSE-02, SVADE-CAUSE-04, SVADE-CAUSE-05 |
| `Svade_no_hw_update_ad` | SVADE-NOUPD-01~05 |
| `Svade_pagefault_cause_match_access_type` | SVADE-CAUSE-01~05, SVADE-A-01, SVADE-A-04, SVADE-D-01 |
| `Svade_applies_all_levels` | SVADE-2M-01~05, SVADE-1G-01~04 |
| `Svade_software_set_ad_then_access` | SVADE-A-03, SVADE-A-05, SVADE-D-03, SVADE-NOUPD-03, SVADE-NOUPD-04 |
| `Svade_non_leaf_ad_reserved` | SVADE-NL-01~03 |
