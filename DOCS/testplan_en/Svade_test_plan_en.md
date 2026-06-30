**[中文](../testplan/Svade_test_plan.md) | English**

# Svade Extension Test Plan

This document describes the test plan for the Svade (Page-Fault Exceptions on A/D Bit Updates) extension. The Svade extension specifies that when a virtual page is accessed with PTE.A=0, or is written with PTE.D=0, the hardware shall **not atomically update** the A/D bits; instead, a page-fault exception is raised, and software is responsible for setting the A/D bits before retrying.

---

## Overview

The RISC-V Privileged Specification defines two PTE A/D bit management schemes:

1. **Non-Svade scheme** (default hardware update): When A=0 on access or D=0 on write, the hardware atomically updates the PTE's A/D bits.
2. **Svade scheme**: When A=0 on access or D=0 on write, the hardware raises a page-fault exception. The PTE's A/D bits remain unchanged, and a software trap handler explicitly sets the bits before retrying the access.

For processors implementing the Svade extension, step 9 of the virtual address translation process (see `SPEC/supervisor.adoc` translation algorithm) checks `pte.a` and `pte.d`:

- If `pte.a=0`, or the original access is a store and `pte.d=0`, **translation is halted and a page-fault exception matching the original access type is raised**.

This test plan focuses on coverage of this specification requirement across different access types (load / store / instruction fetch / AMO) and different page granularities (4 KiB / 2 MiB / 1 GiB).

---

## Test Scope

### Specification Source

- `SPEC/supervisor.adoc`, lines 1622–1731 (Svade extension definition, A/D bit semantics)
- `SPEC/supervisor.adoc`, line 1758 (virtual address translation algorithm step 9, where Svade triggers page-fault)

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:svade_access_ad_bit_clear` | The Svade extension: when a virtual page is accessed and the A bit is clear, or is written and the D bit is clear, a page-fault exception is raised. |
| `Svade_store_d_bit_clear_pagefault` | — |
| `Svade_no_hw_update_ad` | — |
| `Svade_pagefault_cause_match_access_type` | — |
| `Svade_applies_all_levels` | — |
| `Svade_software_set_ad_then_access` | — |
| `Svade_non_leaf_ad_reserved` | — |

### Out of Scope

- **Hypervisor two-stage translation**: VS-stage and G-stage Svade behavior (involves the H extension, not available on this test platform).
- **Svadu interaction**: `menvcfg.ADUE` / `henvcfg.ADUE` controlling hardware A/D update and Svade switching behavior, covered by a separate svadu test plan.
- **Multi-hart consistency**: The specification requires all harts to adopt the same PTE update scheme; the current test framework is single-core.
- **Sv32 / Sv48 / Sv57 modes**: This plan covers Sv39 only, consistent with the existing svnapot/svpbmt test plans.

---

## Test Groups

### Group 1: A Bit Clear Triggers Load / Fetch Page-Fault (4 KiB)

**Spec Reference**:
- `norm:Svade_access_a_bit_clear_pagefault`: Page-fault when A=0 on access
- `norm:Svade_software_set_ad_then_access`: Access succeeds when A=1
- `norm:Svade_pagefault_cause_match_access_type`: Load triggers scause=13, fetch triggers scause=12

**Test Scope**: Verify that on a 4 KiB leaf PTE with A=0, all read-type accesses (load, instruction fetch) trigger the corresponding page-fault type; access succeeds when A=1.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADE-A-01 | A=0 R-only page load | 4 KiB PTE: V=1, R=1, A=0, D=0, S-mode load | load page-fault (scause=13) |
| SVADE-A-02 | A=0 with D=1 load | 4 KiB PTE: V=1, R=1, A=0, D=1, S-mode load | load page-fault (D=1 does not affect A check) |
| SVADE-A-03 | A=1 load succeeds | 4 KiB PTE: V=1, R=1, A=1, D=0, S-mode load | Read succeeds, no exception |
| SVADE-A-04 | A=0 X-only page fetch | 4 KiB PTE: V=1, R=0, X=1, A=0, S-mode jump-and-execute | instruction page-fault (scause=12) |
| SVADE-A-05 | A=1 X page fetch succeeds | 4 KiB PTE: V=1, R=0, X=1, A=1, S-mode jump-and-execute | Fetch and execute succeed |
| SVADE-A-06 | A=0 RW page load | 4 KiB PTE: V=1, R=1, W=1, A=0, D=0, S-mode load | load page-fault (scause=13) |

---

### Group 2: D Bit Clear Triggers Store Page-Fault (4 KiB)

**Spec Reference**:
- `norm:Svade_store_d_bit_clear_pagefault`: Page-fault when D=0 on store
- `norm:Svade_pagefault_cause_match_access_type`: Store triggers scause=15
- `norm:Svade_software_set_ad_then_access`: Store succeeds when D=1

**Test Scope**: Verify that on a 4 KiB leaf PTE with D=0, store / AMO accesses trigger store/AMO page-fault; load is not affected by the D bit; store succeeds when D=1.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADE-D-01 | D=0 RW page store | 4 KiB PTE: V=1, R=1, W=1, A=1, D=0, S-mode store | store page-fault (scause=15) |
| SVADE-D-02 | D=0 RW page load | 4 KiB PTE: V=1, R=1, W=1, A=1, D=0, S-mode load | Read succeeds (D does not affect load) |
| SVADE-D-03 | D=1 RW page store | 4 KiB PTE: V=1, R=1, W=1, A=1, D=1, S-mode store | Write succeeds |
| SVADE-D-04 | A=0 D=0 RW page store | 4 KiB PTE: V=1, R=1, W=1, A=0, D=0, S-mode store | store page-fault (scause=15, A also missing triggers fault) |
| SVADE-D-05 | D=0 AMO operation | 4 KiB PTE: V=1, R=1, W=1, A=1, D=0, S-mode `amoadd.w` | store/AMO page-fault (scause=15) |

---

### Group 3: Hardware Does Not Update A/D Bits

**Spec Reference**:
- `norm:Svade_no_hw_update_ad`: Under Svade, hardware shall not automatically set A/D bits after triggering a page-fault
- `norm:Svade_software_set_ad_then_access`: Access succeeds after software sets A/D bits + SFENCE.VMA

**Test Scope**: Verify the fundamental distinction between Svade and non-Svade schemes — after an A/D-triggered page-fault, PTE contents remain unchanged; retry succeeds after software sets the bits.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADE-NOUPD-01 | PTE unchanged after A=0 fault | After triggering SVADE-A-01, return to M-mode and read PTE to check A bit | PTE.A remains 0 |
| SVADE-NOUPD-02 | PTE unchanged after D=0 fault | After triggering SVADE-D-01, return to M-mode and read PTE to check D bit | PTE.D remains 0 |
| SVADE-NOUPD-03 | Software sets A=1 and retries load | A=0 load fault → trap handler sets A=1 + SFENCE.VMA → retry | Read succeeds |
| SVADE-NOUPD-04 | Software sets D=1 and retries store | D=0 store fault → trap handler sets D=1 + SFENCE.VMA → retry | Write succeeds |
| SVADE-NOUPD-05 | Multiple loads on same A=0 page | Load A=0 page N consecutive times | Each triggers page-fault (PTE never changes) |

---

### Group 4: A/D Behavior on 2 MiB Megapage

**Spec Reference**:
- `norm:Svade_applies_all_levels`: A/D check is performed on leaf PTEs; 2 MiB megapage is equally applicable

**Test Scope**: Verify that Svade behavior also applies to 2 MiB megapage (level 1 leaf PTE in Sv39).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADE-2M-01 | 2M A=0 load fault | 2 MiB leaf PTE: V=1, R=1, A=0, S-mode load | load page-fault (scause=13) |
| SVADE-2M-02 | 2M D=0 store fault | 2 MiB leaf PTE: V=1, R=1, W=1, A=1, D=0, S-mode store | store page-fault (scause=15) |
| SVADE-2M-03 | 2M A=1 D=1 access succeeds | 2 MiB leaf PTE: V=1, R=1, W=1, A=1, D=1, S-mode load + store | Both read and write succeed |
| SVADE-2M-04 | 2M A=0 fetch fault | 2 MiB leaf PTE: V=1, X=1, A=0, S-mode jump-and-execute | instruction page-fault (scause=12) |
| SVADE-2M-05 | 2M multi-offset access within region | A=0 megapage, access offsets 0, 0x1000, 0x100000, 0x1FF000 | Each access triggers page-fault |

---

### Group 5: A/D Behavior on 1 GiB Gigapage

**Spec Reference**:
- `norm:Svade_applies_all_levels`: A/D check is performed on leaf PTEs; 1 GiB gigapage is equally applicable

**Test Scope**: Verify that Svade behavior also applies to 1 GiB gigapage (level 2 leaf PTE in Sv39).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADE-1G-01 | 1G A=0 load fault | 1 GiB leaf PTE: V=1, R=1, A=0, S-mode load | load page-fault (scause=13) |
| SVADE-1G-02 | 1G D=0 store fault | 1 GiB leaf PTE: V=1, R=1, W=1, A=1, D=0, S-mode store | store page-fault (scause=15) |
| SVADE-1G-03 | 1G A=1 D=1 access succeeds | 1 GiB leaf PTE: V=1, R=1, W=1, A=1, D=1, S-mode load + store | Both read and write succeed |
| SVADE-1G-04 | 1G multi-offset access within region | A=0 gigapage, access multiple offsets within region | Each access triggers page-fault |

> [!NOTE]
> 1 GiB gigapage tests require that the test VA be chosen in an identity-mapped region far from the M-mode code segment and stack, to avoid conflicting with the 1 GiB region containing the code segment.

---

### Group 6: Non-Leaf PTE A/D Bits Do Not Participate in Svade Check

**Spec Reference**:
- `norm:Svade_non_leaf_ad_reserved`: D, A, and U bits in non-leaf PTEs are reserved for future standard use and should be cleared by software; Svade check is performed only on leaf PTEs

**Test Scope**: Verify that Svade's A/D check applies only to leaf PTEs, independent of A/D bits on intermediate non-leaf PTEs.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADE-NL-01 | Non-leaf A=0, leaf A=1 load | Mid-level PTE A=0, D=0 (standard usage), 4 KiB leaf PTE A=1, S-mode load | Access succeeds (leaf PTE A=1 determines) |
| SVADE-NL-02 | Non-leaf A=0, leaf A=0 load | Mid-level PTE A=0, leaf PTE A=0, S-mode load | load page-fault (leaf PTE A=0 triggers) |
| SVADE-NL-03 | Non-leaf D=0, leaf D=1 store | Mid-level PTE D=0, leaf PTE A=1, D=1, W=1, S-mode store | Write succeeds (leaf PTE D=1 determines) |

---

### Group 7: scause Correspondence with Access Type

**Spec Reference**:
- `norm:Svade_pagefault_cause_match_access_type`: The scause of a page-fault must correspond to the original access type

**Test Scope**: Verify that the scause of Svade-triggered page-faults is 13 / 15 / 12 / 15 for load / store / fetch / AMO access types respectively.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SVADE-CAUSE-01 | load → scause=13 | Execute `lw` on A=0 R page | scause = `CAUSE_LOAD_PAGE_FAULT` (13) |
| SVADE-CAUSE-02 | store → scause=15 | Execute `sw` on A=1 D=0 RW page | scause = `CAUSE_STORE_PAGE_FAULT` (15) |
| SVADE-CAUSE-03 | fetch → scause=12 | Jump and execute on A=0 X page | scause = `CAUSE_FETCH_PAGE_FAULT` (12) |
| SVADE-CAUSE-04 | AMO → scause=15 | Execute `amoadd.w` on A=1 D=0 RW page | scause = `CAUSE_STORE_PAGE_FAULT` (15) |
| SVADE-CAUSE-05 | A=0 store → scause=15 | Execute `sw` on A=0 D=0 RW page | scause = `CAUSE_STORE_PAGE_FAULT` (15) (store type takes priority) |

> [!IMPORTANT]
> SVADE-CAUSE-05 verifies that when a store operation simultaneously triggers both A=0 and D=0, since step 9's condition is "A=0 or (store and D=0)", the entire determination belongs to a store access, so scause should be store page-fault (15), not load page-fault (13).

---

## Appendix: Related Constants and API Reference

### scause Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `CAUSE_FETCH_PAGE_FAULT` | 12 | Instruction page fault |
| `CAUSE_LOAD_PAGE_FAULT` | 13 | Load page fault |
| `CAUSE_STORE_PAGE_FAULT` | 15 | Store/AMO page fault |

### Test Framework API (consistent with svnapot/svpbmt)

- `pt_context_t` / `pt_init` / `pt_pool_reset`: Page table context management
- `pt_setup_identity_mapping(ctx, base, size, flags, level)`: Bulk identity mapping
- `pt_map_page(ctx, va, pa, flags, level)`: Single page mapping, level takes `PT_LEVEL_4K` / `PT_LEVEL_2M` / `PT_LEVEL_1G`
- `pt_read_pte(ctx, va, level)`: Read PTE value at specified level (for verifying A/D bits)
- `vm_run_in_smode(ctx, fn, arg)`: Enter S-mode with given page table context to execute fn(arg), returns scause on exception
- `TEST_REGISTER` / `TEST_BEGIN` / `TEST_ASSERT` / `TEST_END`: Test case registration and assertion macros

### PTE Flag Macros

- `PTE_V`, `PTE_R`, `PTE_W`, `PTE_X`, `PTE_U`, `PTE_G`, `PTE_A`, `PTE_D`

---

## Test Execution Notes

- All test cases run in Sv39 mode (`SATP_MODE_SV39`)
- Test entry point is in `sv39/main.c`; test cases can be added in `sv39/tests/test_svade.c` (to be implemented)
- Svade extension is enabled under M-mode (specific method depends on implementation, e.g., via `menvcfg` or hardwired enable)
- S-mode trap handler needs to capture page-faults and pass scause back to M-mode via the return value of `vm_run_in_smode`
