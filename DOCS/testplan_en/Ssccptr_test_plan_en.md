**[中文](../testplan/Ssccptr_test_plan.md) | English**

# Ssccptr Extension Test Plan

This document describes the test plan for the Ssccptr (Main Memory Page-Table Reads, Version 1.0) extension. The Ssccptr extension specifies that if the extension is implemented, main memory regions with both the cacheability and coherence PMA attributes must support hardware page-table reads.

---

## Overview

In the RISC-V Privileged Specification, the MMU reads page table entries (PTEs) from memory during virtual address translation, i.e., a hardware page-table walk. Whether the physical memory region containing the page table data supports such implicit read operations depends on the Physical Memory Attributes (PMA) of that region.

The Ssccptr extension imposes the following explicit constraint:

- **Core requirement**: Main memory regions with both the cacheability and coherence PMA attributes **must** be correctly readable by the MMU's page-table walker.
- **Implied semantics**: On an Ssccptr-compliant implementation, software may place page tables in regular main memory satisfying the above PMA conditions, and the hardware page walk will work correctly without additional PMA configuration or special handling.

Although this constraint appears straightforward, its verification spans multiple dimensions: different page table levels, different virtual memory modes, different access types, reads at each level of a multi-level page walk, and interactions with PMP.

---

## Specification Sections Covered by This Document

This plan is based on the following RISC-V official specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/ssccptr.adoc` — Ssccptr Extension for Main Memory Page-Table Reads, Version 1.0
- `SPEC/riscv-isa-manual/src/priv/sv.adoc` — Sv39/Sv48/Sv57 paging schemes, multi-level page table structure, SFENCE.VMA and TLB behavior
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — satp, sstatus.SUM/MXR and other S-mode address translation controls

Official repository:

- https://github.com/riscv/riscv-isa-manual (corresponding files `src/priv/ssccptr.adoc`, `src/priv/sv.adoc`, `src/priv/supervisor.adoc` in the repository)

---

## Covered Specification Points

| Norm ID | Original Text | Description |
|---------|---------------|-------------|
| `norm:ssccptr_memory_pte_reads` | If the Ssccptr extension is implemented, then main memory regions with both the cacheability and coherence PMAs must support hardware page-table reads. | If the Ssccptr extension is implemented, main memory regions with both cacheability and coherence PMAs must support hardware page-table reads. |

Self-derived specification points (testable assertions derived from `norm:ssccptr_memory_pte_reads`; no independent norm tag exists in SPEC):

| Norm ID | Description |
|---------|-------------|
| `Ssccptr_all_pt_levels` | Hardware page-table reads must work correctly at every page table level (L0/L1/L2 in Sv39, L3 added in Sv48, L4 added in Sv57) |
| `Ssccptr_all_access_types` | Hardware page-table reads must work correctly for load, store, and instruction fetch — the three access types that trigger page walks |
| `Ssccptr_all_priv_modes` | Hardware page-table reads must work correctly in page walks triggered from S-mode and U-mode |
| `Ssccptr_multiLevel_walk` | In a multi-level page-table traversal, the reads of each non-leaf PTE and the leaf PTE must all succeed |
| `Ssccptr_superpage` | Leaf PTE reads for superpages (megapage / gigapage / terapage / petapage) must work correctly |

---

## Out of Scope

- **Non-cacheable / non-coherent region behavior**: Ssccptr only constrains regions satisfying both cacheability + coherence; no requirements are imposed on other PMA combinations.
- **Page table placement in I/O regions**: Placing page tables in I/O space (which does not satisfy the PMA conditions) is not within the scope of Ssccptr.
- **Sv32 mode**: This plan covers RV64 only (Sv39 / Sv48 / Sv57), consistent with other extension test plans in the project.
- **Multi-hart scenarios**: The project uses a single-core test environment.
- **PMA register configuration and discovery**: PMA is typically a platform-level hardwired attribute and is not software-programmable. The test plan relies on the assumption that the platform's main memory satisfies the PMA conditions by default.
- **Hypervisor two-stage translation (VS-stage + G-stage)**: Covered by a separate hypervisor test plan.

---

## Prerequisites and Constraints

> [!IMPORTANT]
> The core constraint of Ssccptr pertains to PMA (Physical Memory Attributes), which are typically platform hardwired attributes and not dynamically configurable by software. The verification strategy of this test plan is: **establish page tables in default main memory (which satisfies the PMA conditions), and indirectly verify hardware page-table read capability through end-to-end page walk success (or expected failure)**.

### Design Principles

1. **Indirect verification**: Since MMU page-table read operations cannot be directly observed, page walks are verified indirectly by setting up page tables, enabling virtual memory, performing different types of accesses, and confirming correct results (no access fault).
2. **Control group design**: Alongside verifying page walk success, a control group that blocks page table reads via PMP demonstrates the test's ability to detect page walk failures (eliminating false positives).
3. **Level-by-level coverage**: Each page table level is verified individually to ensure that page walks are not being skipped due to TLB hits alone.
4. **Platform feasibility check**: Alignment requirements for the Sv48 512 GiB terapage and Sv57 512 GiB / 256 TiB pages may not be satisfiable on the platform's memory base; tests must check feasibility at runtime and TEST_SKIP infeasible cases.

If any platform violates the SPEC, the corresponding case remains FAIL, and implementation defects are recorded to the `bugs/` directory.

---

## Test Groups

### Group 1: Basic Page Table Walk Verification (Sv39, 4 KiB Pages)

**Spec Reference**:
- `norm:ssccptr_memory_pte_reads`: Main memory regions must support hardware page-table reads
- `Ssccptr_all_access_types`: load / store / fetch must all work correctly
- `Ssccptr_all_priv_modes`: S-mode and U-mode must both work correctly

**Test Scope**: Establish Sv39 three-level page tables (identity mapping, L0/L1/L2) on default main memory, and verify that page walks for 4 KiB pages complete successfully across different access types and privilege modes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCCPTR-BASIC-01 | Sv39 4K page S-mode load | Set up Sv39 identity mapping (4 KiB page, PTE with RWXAD), S-mode load | Read succeeds, no exception |
| SSCCPTR-BASIC-02 | Sv39 4K page S-mode store | Same configuration as above, S-mode store | Write succeeds, no exception |
| SSCCPTR-BASIC-03 | Sv39 4K page S-mode fetch | Same configuration (X=1), S-mode jump-and-execute | Fetch and execute succeed |
| SSCCPTR-BASIC-04 | Sv39 4K page U-mode load | Set up 4 KiB mapping with PTE_U flag, U-mode load | Read succeeds, no exception |
| SSCCPTR-BASIC-05 | Sv39 4K page U-mode store | Same configuration as above, U-mode store | Write succeeds, no exception |
| SSCCPTR-BASIC-06 | Sv39 4K page U-mode fetch | Same configuration (X=1, U=1), U-mode jump-and-execute | Fetch and execute succeed |

**Implementation notes**:
- S-mode access helpers (load/store/exec) internally use the trap-expectation mechanism to capture exceptions: no exception returns 0, an exception returns the scause value.
- The test data region is provided by the linker script and contains three parts — read/write data area, exception test page, and executable code area — 2 MiB aligned so that it is distinguishable from the code mapping.

---

### Group 2: Superpage Page Walk Verification (Sv39, 2 MiB / 1 GiB)

**Spec Reference**:
- `norm:ssccptr_memory_pte_reads`
- `Ssccptr_superpage`: Superpage leaf PTE reads must work correctly
- `Ssccptr_multiLevel_walk`: Page walks at different depths

**Test Scope**: Verify that page walks succeed for 2 MiB megapages (L1 leaf PTE) and 1 GiB gigapages (L2 leaf PTE). Superpage page walks traverse fewer levels, covering different traversal depths.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCCPTR-SUPER-01 | Sv39 2M megapage S-mode load | L1 leaf PTE identity mapping, S-mode load | Success |
| SSCCPTR-SUPER-02 | Sv39 2M megapage S-mode store | Same as above, S-mode store | Success |
| SSCCPTR-SUPER-03 | Sv39 2M megapage S-mode fetch | Same as above (X=1), S-mode fetch | Success |
| SSCCPTR-SUPER-04 | Sv39 1G gigapage S-mode load | L2 leaf PTE identity mapping, S-mode load | Success |
| SSCCPTR-SUPER-05 | Sv39 1G gigapage S-mode store | Same as above, S-mode store | Success |
| SSCCPTR-SUPER-06 | Sv39 1G gigapage S-mode fetch | Same as above (X=1), S-mode fetch | Success |

**Implementation notes**: Use batch identity mapping to cover the code and data regions; the target VA is determined by the aligned segments provided by the linker script (2 MiB / 1 GiB alignment).

---

### Group 3: Multi-Level Page Walk Level-by-Level Verification (Sv39)

**Spec Reference**:
- `Ssccptr_multiLevel_walk`: In a multi-level page table traversal, PTE reads at each level must succeed
- `Ssccptr_all_pt_levels`: All page table levels must be supported

**Test Scope**: After clearing the TLB with SFENCE.VMA, perform accesses to force a full page walk, and verify PTE reads at each non-leaf and leaf level. Use PMP control groups to verify test validity.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCCPTR-LEVEL-01 | L0 PTE read verification | 4 KiB page, load after SFENCE.VMA TLB flush, page walk traverses L2->L1->L0 | Success |
| SSCCPTR-LEVEL-02 | L1 PTE read verification | 2 MiB page, load after SFENCE.VMA TLB flush, page walk traverses L2->L1 | Success |
| SSCCPTR-LEVEL-03 | L2 PTE read verification | 1 GiB page, load after SFENCE.VMA TLB flush, page walk traverses L2 | Success |
| SSCCPTR-LEVEL-04 | PMP blocks L0 PT control | PMP sets L0 page table page to non-readable, load after SFENCE.VMA | load access fault |
| SSCCPTR-LEVEL-05 | PMP blocks L1 PT control | PMP sets L1 page table page to non-readable, load after SFENCE.VMA | load access fault |
| SSCCPTR-LEVEL-06 | PMP blocks L2 PT (root) control | PMP sets root page table page to non-readable, load after SFENCE.VMA | load access fault |

> [!NOTE]
> SSCCPTR-LEVEL-04/05/06 are **control groups**: by deliberately blocking reads to a specific page table level via PMP, they verify that the test framework can detect page walk failures. If the control groups trigger an access fault while the normal groups (01/02/03) do not, this effectively demonstrates that the normal groups' page walks indeed traversed the corresponding page table level.

**Implementation notes**:
- Each entry into the VM execution helper performs VM enable + SFENCE.VMA and disables VM on exit, so every entry triggers a full page walk.
- PMP control group: use NAPOT mode to mark the target page table page as X-only (non-readable), blocking MMU PTE reads; also use a PMP entry covering the entire address space to permit other accesses.

---

### Group 4: Sv48 Page Table Walk Verification

**Spec Reference**:
- `norm:ssccptr_memory_pte_reads`
- `Ssccptr_all_pt_levels`: Sv48 adds a 4th level (L3) page table

**Test Scope**: Verify that page walks succeed at all page table levels in Sv48 mode, covering L3 root page table reads.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCCPTR-SV48-01 | Sv48 4K page S-mode load | Sv48 identity mapping (4 KiB), S-mode load | Success |
| SSCCPTR-SV48-02 | Sv48 4K page S-mode store | Same as above, S-mode store | Success |
| SSCCPTR-SV48-03 | Sv48 4K page S-mode fetch | Same as above (X=1), S-mode fetch | Success |
| SSCCPTR-SV48-04 | Sv48 2M megapage load | Sv48 2M page mapping, S-mode load | Success |
| SSCCPTR-SV48-05 | Sv48 1G gigapage load | Sv48 1G page mapping, S-mode load | Success |
| SSCCPTR-SV48-06 | Sv48 512G terapage load | Sv48 L3 leaf PTE mapping, S-mode load | Success; SKIP when the platform memory base is not 512 GiB aligned |

> [!NOTE]
> The Sv48 512 GiB terapage requires the platform memory base to be 512 GiB aligned. When the platform memory base does not satisfy this alignment (for example, common simulation platforms use MEM_BASE = 2 GiB), the test should detect this and TEST_SKIP.

---

### Group 5: Sv57 Page Table Walk Verification

**Spec Reference**:
- `norm:ssccptr_memory_pte_reads`
- `Ssccptr_all_pt_levels`: Sv57 adds a 5th level (L4) page table

**Test Scope**: Verify that page walks succeed at all page table levels in Sv57 mode, covering L4 root page table reads.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCCPTR-SV57-01 | Sv57 4K page S-mode load | Sv57 identity mapping (4 KiB), S-mode load | Success |
| SSCCPTR-SV57-02 | Sv57 4K page S-mode store | Same as above, S-mode store | Success |
| SSCCPTR-SV57-03 | Sv57 4K page S-mode fetch | Same as above (X=1), S-mode fetch | Success |
| SSCCPTR-SV57-04 | Sv57 2M megapage load | Sv57 2M page mapping, S-mode load | Success |
| SSCCPTR-SV57-05 | Sv57 1G gigapage load | Sv57 1G page mapping, S-mode load | Success |
| SSCCPTR-SV57-06 | Sv57 512G terapage load | Sv57 L3 leaf PTE mapping, S-mode load | Success; SKIP when the platform memory base is not 512 GiB aligned |
| SSCCPTR-SV57-07 | Sv57 256T petapage load | Sv57 L4 leaf PTE mapping, S-mode load | Success; SKIP when the platform memory base is not 256 TiB aligned |

> [!NOTE]
> The Sv57 512 GiB terapage and 256 TiB petapage require 512 GiB and 256 TiB alignment, respectively. These alignment requirements typically cannot be satisfied on standard platforms; the test should detect this and TEST_SKIP. Sv57 itself also requires the platform to support `SATP_MODE_SV57`; if unsupported, the entire group is SKIPPED.

---

### Group 6: Repeated Page Walk After TLB Flush

**Spec Reference**:
- `norm:ssccptr_memory_pte_reads`: Every page walk must succeed
- `Ssccptr_multiLevel_walk`

**Test Scope**: Verify that after clearing the TLB with SFENCE.VMA, repeatedly triggered page walks all complete successfully, eliminating the possibility that TLB caching masks page walk failures.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCCPTR-TLB-01 | Consecutive TLB flush + load | 4 KiB page, loop N times: SFENCE.VMA then S-mode load | Each iteration succeeds |
| SSCCPTR-TLB-02 | Consecutive TLB flush + store | 4 KiB page, loop N times: SFENCE.VMA then S-mode store | Each iteration succeeds |
| SSCCPTR-TLB-03 | Consecutive TLB flush + fetch | 4 KiB page, loop N times: SFENCE.VMA then S-mode fetch | Each iteration succeeds |
| SSCCPTR-TLB-04 | Alternating page walks at different addresses | Two different VAs with 4 KiB pages, alternating SFENCE.VMA + load | Each iteration succeeds |

**Implementation notes**: The VM execution helper internally disables VM on each exit and enables VM + SFENCE.VMA on re-entry, forcing a fresh page walk.

---

### Group 7: Page Walk After Dynamic Page Table Modification

**Spec Reference**:
- `norm:ssccptr_memory_pte_reads`: Main memory must support hardware page-table reads
- Derived: After dynamically modifying page tables, SFENCE.VMA + subsequent page walk must also succeed

**Test Scope**: Verify that after modifying PTE content at runtime (e.g., changing permissions, changing mapping targets), executing SFENCE.VMA and re-walking correctly reads the updated PTE.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCCPTR-DYN-01 | Walk after PTE permission change | R-only then RW, store after SFENCE.VMA | First store faults, store succeeds after modification |
| SSCCPTR-DYN-02 | Walk after PTE target PA change | Remap VA to a different PA, load after SFENCE.VMA | Reads data from the new PA |
| SSCCPTR-DYN-03 | Walk after adding new mapping | No mapping initially then add PTE then SFENCE.VMA then load | First page-fault, succeeds after addition |
| SSCCPTR-DYN-04 | Walk after removing mapping | Mapping exists then clear PTE.V then SFENCE.VMA then load | First succeeds, page-fault after removal |

---

### Group 8: PMP Page Walk Blocking Control Verification

**Spec Reference**:
- `norm:ssccptr_memory_pte_reads` (negative verification)
- Control: PMP can block PTE reads during a page walk

**Test Scope**: By using PMP to block read access to the physical memory containing page tables, verify that page walks are correctly blocked (producing an access fault rather than a page fault). This group serves as a control for Groups 1-3, demonstrating that the success of the normal groups truly depends on correct page walk completion.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCCPTR-PMP-01 | PMP blocks L0 PT load | PMP sets L0 PT page to X-only, S-mode load | load access fault |
| SSCCPTR-PMP-02 | PMP blocks L0 PT store | PMP sets L0 PT page to X-only, S-mode store | store access fault |
| SSCCPTR-PMP-03 | PMP blocks L0 PT fetch | PMP sets L0 PT page to X-only, S-mode fetch | instruction access fault |
| SSCCPTR-PMP-04 | PMP blocks L1 PT load | PMP sets L1 PT page to X-only, S-mode load | load access fault |
| SSCCPTR-PMP-05 | PMP blocks root PT load | PMP sets root PT page to X-only, S-mode load | load access fault |
| SSCCPTR-PMP-06 | Walk resumes after PMP allows | Block first then remove PMP restriction then SFENCE.VMA then load | load succeeds |

---

## Test Matrix Overview

| Group | Test Count | VM Mode | Page Granularity | Privilege Mode | Core Verification Point |
|-------|-----------|---------|-------------------|----------------|------------------------|
| 1 - Basic page walk | 6 | Sv39 | 4 KiB | S / U | load / store / fetch access types |
| 2 - Superpage | 6 | Sv39 | 2M / 1G | S | megapage / gigapage leaf PTE reads |
| 3 - Level-by-level | 6 | Sv39 | 4K / 2M / 1G | S | PTE reads at each level + PMP control |
| 4 - Sv48 | 6 | Sv48 | 4K / 2M / 1G / 512G | S | L3 root page table reads |
| 5 - Sv57 | 7 | Sv57 | 4K / 2M / 1G / 512G / 256T | S | L4 root page table reads |
| 6 - TLB flush | 4 | Sv39 | 4 KiB | S | Repeated page walk stability |
| 7 - Dynamic modification | 4 | Sv39 | 4 KiB | S | Page walk after PTE modification |
| 8 - PMP control | 6 | Sv39 | 4 KiB | S | PMP blocking page walk (negative verification) |

**Total: 45 test cases**

---

## Implementation Notes

### 1. Build Configuration

Ssccptr is a PMA-level constraint extension that does not introduce new instructions or CSRs. The test project must enable virtual memory support in the build configuration and, depending on the platform, declare the Ssccptr extension. If the platform does not recognize the `_ssccptr` extension name, enabling virtual memory support alone is sufficient to run the tests.

### 2. Reuse of Existing Capabilities

- **Page table management**: Directly use the page-table initialization, single-page mapping, and batch identity-mapping capabilities of the common framework.
- **VM entry/exit**: Use the S-mode / U-mode execution helpers of the common framework, which internally handle VM enable/disable and SFENCE.VMA.
- **S-mode access helpers**: Reuse the load/store/exec helper pattern from existing test projects (internally using the trap-expectation mechanism to capture exceptions).
- **PMP configuration** (Group 3/8): Use the common framework's PMP entry configuration capability (NAPOT mode).
- **Page-table page address retrieval** (Group 3/8): Use the common framework's capability to obtain the physical address of a page-table page by VA and level.
- **PTE inspection/modification** (Group 7): Use the common framework's capability to obtain PTE contents by VA and level.
- **Exception cause constants**: Use the short aliases `CAUSE_LAF` / `CAUSE_SAF` / `CAUSE_IAF` / `CAUSE_LPF` / `CAUSE_SPF` / `CAUSE_IPF` defined in the common framework.

### 3. PMA Prerequisite Assumption

> [!WARNING]
> This test plan assumes that the target platform's main memory regions satisfy the cacheability + coherence PMA conditions by default. If the main memory PMA does not satisfy these conditions on a specific platform, related tests may fail due to page walk failure rather than Ssccptr non-compliance. This prerequisite should be taken into account when interpreting test results.

---

## Appendix A: Specification Point Coverage Matrix

| Norm ID | Covered Test IDs | Coverage Status | Notes |
|---------|------------------|-----------------|-------|
| `norm:ssccptr_memory_pte_reads` | SSCCPTR-BASIC-01 ~ SSCCPTR-BASIC-06, SSCCPTR-SUPER-01 ~ SSCCPTR-SUPER-06, SSCCPTR-LEVEL-01 ~ SSCCPTR-LEVEL-06, SSCCPTR-SV48-01 ~ SSCCPTR-SV48-06, SSCCPTR-SV57-01 ~ SSCCPTR-SV57-07, SSCCPTR-TLB-01 ~ SSCCPTR-TLB-04, SSCCPTR-DYN-01 ~ SSCCPTR-DYN-04, SSCCPTR-PMP-01 ~ SSCCPTR-PMP-06 | Covered | Main memory must support hardware page-table reads (positive/negative verification) |
| `Ssccptr_all_pt_levels` | SSCCPTR-LEVEL-01 ~ SSCCPTR-LEVEL-06, SSCCPTR-SV48-01 ~ SSCCPTR-SV48-06, SSCCPTR-SV57-01 ~ SSCCPTR-SV57-07 | Covered | Sv39/Sv48/Sv57 page table levels |
| `Ssccptr_all_access_types` | SSCCPTR-BASIC-01 ~ SSCCPTR-BASIC-06, SSCCPTR-SUPER-01 ~ SSCCPTR-SUPER-06, SSCCPTR-TLB-01 ~ SSCCPTR-TLB-03 | Covered | load/store/fetch access types |
| `Ssccptr_all_priv_modes` | SSCCPTR-BASIC-01 ~ SSCCPTR-BASIC-06 | Covered | S-mode and U-mode |
| `Ssccptr_multiLevel_walk` | SSCCPTR-LEVEL-01 ~ SSCCPTR-LEVEL-03, SSCCPTR-TLB-01 ~ SSCCPTR-TLB-04, SSCCPTR-DYN-01 ~ SSCCPTR-DYN-04 | Covered | Every level of a multi-level walk |
| `Ssccptr_superpage` | SSCCPTR-SUPER-01 ~ SSCCPTR-SUPER-06, SSCCPTR-SV48-04 ~ SSCCPTR-SV48-06, SSCCPTR-SV57-04 ~ SSCCPTR-SV57-07 | Covered | megapage/gigapage/terapage/petapage |
