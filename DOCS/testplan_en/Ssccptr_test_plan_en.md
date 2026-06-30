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

## Test Scope

### Specification Source

- `SPEC/ssccptr.adoc` — Ssccptr Extension for Main Memory Page-Table Reads, Version 1.0

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:ssccptr_memory_pte_reads` | If the Ssccptr extension is implemented, then main memory regions with both the cacheability and coherence PMAs must support hardware page-table reads. |
| `norm:Ssccptr_all_pt_levels` | — |
| `norm:Ssccptr_all_access_types` | — |
| `norm:Ssccptr_all_priv_modes` | — |
| `norm:Ssccptr_multiLevel_walk` | — |
| `norm:Ssccptr_superpage` | — |

### Out of Scope

- **Non-cacheable / non-coherent region behavior**: Ssccptr only constrains regions satisfying both cacheability + coherence; no requirements are imposed on other PMA combinations.
- **Page table placement in I/O regions**: Placing page tables in I/O space (which does not satisfy the PMA conditions) is not within the scope of Ssccptr.
- **Sv32 mode**: This plan covers RV64 only (Sv39 / Sv48 / Sv57), consistent with other extension test plans in the project.
- **Multi-hart scenarios**: The project uses a single-core test environment.
- **PMA register configuration and discovery**: PMA is typically a platform-level hardwired attribute and is not software-programmable. The test plan relies on the assumption that the platform's main memory satisfies the PMA conditions by default.
- **Hypervisor two-stage translation (VS-stage + G-stage)**: Covered by a separate hypervisor test plan.

---

## Prerequisites and Constraints

> [!IMPORTANT]
> The core constraint of Ssccptr pertains to PMA (Physical Memory Attributes), which are typically platform hardwired attributes and not dynamically configurable by software. On the Spike simulator, main memory regions have the cacheability and coherence PMA by default. Therefore, the verification strategy of this test plan is: **establish page tables in default main memory (which satisfies the PMA conditions), and indirectly verify hardware page-table read capability through end-to-end page walk success (or expected failure)**.

### Key Reference Files

| Path | Description |
|------|-------------|
| `SPEC/ssccptr.adoc` | Full Ssccptr specification |
| `common/vm/page_table.c` | Page table management implementation (allocation, mapping, traversal) |
| `common/vm/vm.h` | VM public API (`vm_run_in_smode`, etc.) |
| `common/vm/vm_defs.h` | PTE bit definitions, VA/PA macros |
| `common/vm/satp.c` | `vm_enable` / `vm_run_in_smode` implementation |
| `common/test_framework.c` | Test framework (TEST_BEGIN / TEST_ASSERT / TEST_END) |
| `common/pmp/pmp_cfg.h` | PMP configuration API |
| `pmp_sv39/pmp_vm_common.c` | PMP+VM interaction test reference (comparison cases) |

### Design Principles

1. **Indirect verification**: Since MMU page-table read operations cannot be directly observed, page walks are verified indirectly by setting up page tables, enabling virtual memory, performing different types of accesses, and confirming correct results (no access fault).
2. **Control group design**: Alongside verifying page walk success, a control group that blocks page table reads via PMP demonstrates the test's ability to detect page walk failures (eliminating false positives).
3. **Level-by-level coverage**: Each page table level is verified individually to ensure that page walks are not being skipped due to TLB hits alone.

---

## Test Groups

### Group 1: Basic Page Table Walk Verification (Sv39, 4 KiB Pages)

**Spec Reference**:
- `norm:Ssccptr_cacheable_coherent_supports_pt_read`: Main memory regions must support hardware page-table reads
- `norm:Ssccptr_all_access_types`: load / store / fetch must all work correctly
- `norm:Ssccptr_all_priv_modes`: S-mode and U-mode must both work correctly

**Test Scope**: Establish Sv39 three-level page tables (identity mapping, L0/L1/L2) on default main memory, and verify that page walks for 4 KiB pages complete successfully across different access types and privilege modes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCCPTR-BASIC-01 | Sv39 4K page S-mode load | Set up Sv39 identity mapping (4 KiB page, PTE with RWXAD), S-mode load | Read succeeds, no exception |
| SSCCPTR-BASIC-02 | Sv39 4K page S-mode store | Same configuration as above, S-mode store | Write succeeds, no exception |
| SSCCPTR-BASIC-03 | Sv39 4K page S-mode fetch | Same configuration (X=1), S-mode jump-and-execute | Fetch and execute succeed |
| SSCCPTR-BASIC-04 | Sv39 4K page U-mode load | Set up 4 KiB mapping with PTE_U flag, U-mode load | Read succeeds, no exception |
| SSCCPTR-BASIC-05 | Sv39 4K page U-mode store | Same configuration as above, U-mode store | Write succeeds, no exception |
| SSCCPTR-BASIC-06 | Sv39 4K page U-mode fetch | Same configuration (X=1, U=1), U-mode jump-and-execute | Fetch and execute succeed |

---

### Group 2: Superpage Page Walk Verification (Sv39, 2 MiB / 1 GiB)

**Spec Reference**:
- `norm:Ssccptr_cacheable_coherent_supports_pt_read`
- `norm:Ssccptr_superpage`: Superpage leaf PTE reads must work correctly
- `norm:Ssccptr_multiLevel_walk`: Page walks at different depths

**Test Scope**: Verify that page walks succeed for 2 MiB megapages (L1 leaf PTE) and 1 GiB gigapages (L2 leaf PTE). Superpage page walks traverse fewer levels, covering different traversal depths.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCCPTR-SUPER-01 | Sv39 2M megapage S-mode load | L1 leaf PTE identity mapping, S-mode load | Success |
| SSCCPTR-SUPER-02 | Sv39 2M megapage S-mode store | Same as above, S-mode store | Success |
| SSCCPTR-SUPER-03 | Sv39 2M megapage S-mode fetch | Same as above (X=1), S-mode fetch | Success |
| SSCCPTR-SUPER-04 | Sv39 1G gigapage S-mode load | L2 leaf PTE identity mapping, S-mode load | Success |
| SSCCPTR-SUPER-05 | Sv39 1G gigapage S-mode store | Same as above, S-mode store | Success |
| SSCCPTR-SUPER-06 | Sv39 1G gigapage S-mode fetch | Same as above (X=1), S-mode fetch | Success |

---

### Group 3: Multi-Level Page Walk Level-by-Level Verification (Sv39)

**Spec Reference**:
- `norm:Ssccptr_multiLevel_walk`: In a multi-level page table traversal, PTE reads at each level must succeed
- `norm:Ssccptr_all_pt_levels`: All page table levels must be supported

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

---

### Group 4: Sv48 Page Table Walk Verification

**Spec Reference**:
- `norm:Ssccptr_cacheable_coherent_supports_pt_read`
- `norm:Ssccptr_all_pt_levels`: Sv48 adds a 4th level (L3) page table

**Test Scope**: Verify that page walks succeed at all page table levels in Sv48 mode, covering L3 root page table reads.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCCPTR-SV48-01 | Sv48 4K page S-mode load | Sv48 identity mapping (4 KiB), S-mode load | Success |
| SSCCPTR-SV48-02 | Sv48 4K page S-mode store | Same as above, S-mode store | Success |
| SSCCPTR-SV48-03 | Sv48 4K page S-mode fetch | Same as above (X=1), S-mode fetch | Success |
| SSCCPTR-SV48-04 | Sv48 2M megapage load | Sv48 2M page mapping, S-mode load | Success |
| SSCCPTR-SV48-05 | Sv48 1G gigapage load | Sv48 1G page mapping, S-mode load | Success |
| SSCCPTR-SV48-06 | Sv48 512G terapage load | Sv48 L3 leaf PTE mapping, S-mode load | Success |

> [!NOTE]
> The Sv48 512G terapage (`PT_LEVEL_512G`) requires `PLATFORM_MEM_BASE` to be 512 GiB aligned. On the QEMU virt platform (MEM_BASE=0x80000000 = 2 GiB), this alignment requirement cannot be satisfied; the test should detect this and SKIP. Refer to the 512G level feasibility check logic in `pmp_sv48/pmp_vm_common.c`.

---

### Group 5: Sv57 Page Table Walk Verification

**Spec Reference**:
- `norm:Ssccptr_cacheable_coherent_supports_pt_read`
- `norm:Ssccptr_all_pt_levels`: Sv57 adds a 5th level (L4) page table

**Test Scope**: Verify that page walks succeed at all page table levels in Sv57 mode, covering L4 root page table reads.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCCPTR-SV57-01 | Sv57 4K page S-mode load | Sv57 identity mapping (4 KiB), S-mode load | Success |
| SSCCPTR-SV57-02 | Sv57 4K page S-mode store | Same as above, S-mode store | Success |
| SSCCPTR-SV57-03 | Sv57 4K page S-mode fetch | Same as above (X=1), S-mode fetch | Success |
| SSCCPTR-SV57-04 | Sv57 2M megapage load | Sv57 2M page mapping, S-mode load | Success |
| SSCCPTR-SV57-05 | Sv57 1G gigapage load | Sv57 1G page mapping, S-mode load | Success |
| SSCCPTR-SV57-06 | Sv57 512G terapage load | Sv57 L3 leaf PTE mapping, S-mode load | Success |
| SSCCPTR-SV57-07 | Sv57 256T petapage load | Sv57 L4 leaf PTE mapping, S-mode load | Success |

> [!NOTE]
> The Sv57 512G terapage and 256T petapage require 512 GiB and 256 TiB alignment, respectively. These alignment requirements typically cannot be satisfied on standard simulation platforms; the test should detect this and SKIP. Sv57 itself also requires Spike/QEMU support for `SATP_MODE_SV57`; if unsupported, the entire group should SKIP.

---

### Group 6: Repeated Page Walk After TLB Flush

**Spec Reference**:
- `norm:Ssccptr_cacheable_coherent_supports_pt_read`: Every page walk must succeed
- `norm:Ssccptr_multiLevel_walk`

**Test Scope**: Verify that after clearing the TLB with SFENCE.VMA, repeatedly triggered page walks all complete successfully, eliminating the possibility that TLB caching masks page walk failures.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SSCCPTR-TLB-01 | Consecutive TLB flush + load | 4 KiB page, loop N times: SFENCE.VMA then S-mode load | Each iteration succeeds |
| SSCCPTR-TLB-02 | Consecutive TLB flush + store | 4 KiB page, loop N times: SFENCE.VMA then S-mode store | Each iteration succeeds |
| SSCCPTR-TLB-03 | Consecutive TLB flush + fetch | 4 KiB page, loop N times: SFENCE.VMA then S-mode fetch | Each iteration succeeds |
| SSCCPTR-TLB-04 | Alternating page walks at different addresses | Two different VAs with 4 KiB pages, alternating SFENCE.VMA + load | Each iteration succeeds |

---

### Group 7: Page Walk After Dynamic Page Table Modification

**Spec Reference**:
- `norm:Ssccptr_cacheable_coherent_supports_pt_read`: Main memory must support hardware page-table reads
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
- `norm:Ssccptr_cacheable_coherent_supports_pt_read` (negative verification)
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

> [!TIP]
> The test logic for SSCCPTR-PMP-01 through 05 can reuse the pattern from `run_pmp_on_pte_test()` in `pmp_sv39/pmp_vm_common.c`, setting a PMP entry to mark the page table page as X-only (non-readable), thereby blocking MMU PTE reads. The Group 3 SSCCPTR-LEVEL-04 example already demonstrates the complete PMP blocking pattern, so it is not repeated here. Only the SSCCPTR-PMP-06 (walk resumes after PMP allows) example is shown, as it involves dynamic PMP removal.

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

### 1. SPIKE_ISA_EXT Configuration

Ssccptr is a PMA-level constraint extension that does not introduce new instructions or CSRs. In Spike, main memory regions satisfy the cacheability + coherence PMA conditions by default, so Ssccptr semantics are already met under the default configuration. The Makefile can declare the extension via `SPIKE_ISA_EXT = _ssccptr` (following the pattern of `SPIKE_ISA_EXT = _svadu` in `svadu/Makefile`), but this requires confirmation that the Spike version recognizes this extension name. If Spike does not recognize `_ssccptr`, this configuration can be omitted temporarily; enabling virtual memory support via `ENABLE_VM = 1` alone is sufficient to run the tests.

### 2. Directory Structure

Follow the organization of the `svade/` directory (`main.c` + `kernel.ld` + `tests/` subdirectory):

```
ssccptr/
+-- Makefile                    # Build configuration (ENABLE_VM=1, SPIKE_ISA_EXT)
+-- main.c                      # Test entry point (iterates _test_table for auto-execution)
+-- kernel.ld                   # Linker script (includes .vm_test_region section)
+-- tests/
    +-- test_helpers.h          # S-mode helper functions, test region macros, setup_code_mapping
    +-- test_register.c         # Test registration file (#includes all test groups)
    +-- test_basic.c            # Group 1: Basic page walk verification
    +-- test_superpage.c        # Group 2: Superpage page walk
    +-- test_level.c            # Group 3: Level-by-level verification + PMP control
    +-- test_sv48.c             # Group 4: Sv48 page table walk
    +-- test_sv57.c             # Group 5: Sv57 page table walk
    +-- test_tlb.c              # Group 6: Repeated page walk after TLB flush
    +-- test_dynamic.c          # Group 7: Page walk after dynamic modification
    +-- test_pmp_block.c        # Group 8: PMP control verification
```

### 3. Reuse of Existing Tests

- **Page table management**: Directly use `pt_init` / `pt_map_page` / `pt_setup_identity_mapping` from `common/vm/page_table.c`
- **VM entry/exit**: Use `vm_run_in_smode` / `vm_run_in_umode` from `common/vm/satp.c`
- **S-mode helper functions**: Follow the pattern of `test_smode_load` / `test_smode_store` / `test_smode_exec` in `svade/tests/test_helpers.h` (internally using `trap_expect_begin/end` + `trap_get_cause()`)
- **PMP configuration** (Group 3/8): Use `pmp_set_entry` / `PMP_ENTRY_NAPOT` / `pmp_clear_all` from `common/pmp/pmp_cfg.h`
- **Page table page address retrieval** (Group 3/8): Use `get_pt_page_addr` from `common/vm/vm.h`
- **PTE inspection/modification** (Group 7): Use `pt_get_pte` from `common/vm/vm.h` to directly modify PTE content
- **Exception cause constants**: Use the short aliases `CAUSE_LAF` / `CAUSE_SAF` / `CAUSE_IAF` / `CAUSE_LPF` / `CAUSE_SPF` / `CAUSE_IPF` defined in `common/encoding.h`

### 4. PMA Prerequisite Assumption

> [!WARNING]
> This test plan assumes that the Spike simulator and target hardware main memory regions satisfy the cacheability + coherence PMA conditions by default. If the main memory PMA does not satisfy these conditions on a specific platform, related tests may fail due to page walk failure rather than Ssccptr non-compliance. This prerequisite should be taken into account when interpreting test results.
