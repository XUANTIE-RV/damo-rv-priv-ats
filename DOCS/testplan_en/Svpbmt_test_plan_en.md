**[中文](../testplan/Svpbmt_test_plan.md) | English**

# Svpbmt Extension Test Plan

This document describes the test plan for the Svpbmt (Page-Based Memory Types) extension. The tests are based on the Svpbmt extension definition in the RISC-V Privileged Specification (`SPEC/svpbmt.adoc`), covering PBMT encoding verification, reserved value exceptions, non-leaf PTE checks, memory ordering semantics, aliasing coherence, and other specification requirements.

---

## Overview

The Svpbmt extension uses bits 62-61 (the PBMT field) in leaf page table entries (leaf PTE) of Sv39, Sv48, and Sv57 to override the page's physical memory attributes (PMA). The PBMT field encoding is as follows:

| Mode | Value | Requested Memory Attribute |
|------|-------|---------------------------|
| PMA  | 0     | No override, use underlying PMA |
| NC   | 1     | Non-cacheable, idempotent, weakly ordered (RVWMO), main memory |
| IO   | 2     | Non-cacheable, non-idempotent, strongly ordered (I/O ordering), I/O |
| —    | 3     | Reserved, triggers page-fault |

**Dependency**: The Svpbmt extension depends on the Sv39 extension (`norm:Svpbmt_depends_Sv39`).

---

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:Svpbmt_depends_Sv39` | The Svpbmt extension depends on the Sv39 extension. |
| `norm:Svpbmt_impl_may_override_pmas` | Implementations may override additional PMAs not explicitly listed in <<pbmt>>. |
| `norm:Svpbmt_nonleaf_pte_pbmt_must_be_zero` | Until their use is defined by a standard extension, they must be cleared by software for forward compatibility, or else a page-fault exception is raised. |
| `norm:Svpbmt_leaf_pte_pbmt_reserved_3_fault` | Until this value is defined by a standard extension, using this reserved value in a leaf PTE raises a page-fault exception. |
| `norm:Svpbmt_obeys_mem_ordering` | memory accesses to such pages obey the memory ordering rules of the final effective attribute. |
| `norm:Svpbmt_io_pma_nc_pbmt_obey_rvwmo` | If the underlying physical memory attribute for a page is I/O, and the page has PBMT=NC, then accesses to that page obey RVWMO. |
| `norm:Svpbmt_io_pma_nc_pbmt_treated_as_io_and_memory` | accesses to such pages are considered to be _both_ I/O and main memory accesses for the purposes of FENCE, _.aq_, and _.rl_. |
| `norm:Svpbmt_memory_pma_io_pbmt_strong_io_ordering` | accesses to that page obey strong channel 0 I/O ordering rules. |
| `norm:Svpbmt_memory_pma_io_pbmt_treated_as_io_and_memory` | accesses to such pages are considered to be _both_ I/O and main memory accesses for the purposes of FENCE, _.aq_, and _.rl_. |
| `norm:Svpbmt_aliasing_attribute` | When Svpbmt is used with non-zero PBMT encodings, it is possible for multiple virtual aliases of the same physical page to exist simultaneously with different memory attributes. It is also possible for a U-mode or S-mode mapping through a PTE with Svpbmt enabled to observe different memory attributes for a given region of physical memory than a concurrent access to the same page performed by M-mode or when MODE=Bare. In such cases, the behaviors dictated by the attributes (including coherence, which is otherwise unaffected) may be violated. |
| `norm:Svpbmt_noncacheable_aliasing_no_coherence_loss` | Accessing the same location using different attributes that are both non-cacheable (e.g., NC and IO) does not cause loss of coherence. |
| `norm:Svpbmt_noncacheable_aliasing_may_weaken_ordering` | might result in weaker memory ordering than the stricter attribute ordinarily guarantees. |
| `norm:Svpbmt_noncacheable_aliasing_fence_prevents_ordering_loss` | `fence iorw, iorw` instruction between such accesses suffices to prevent loss of memory ordering. |
| `norm:Svpbmt_cacheable_aliasing_may_cause_coherence_loss` | may cause loss of coherence. |
| `norm:Svpbmt_cacheable_aliasing_fence_flush_fence_required` | prevents both loss of coherence and loss of memory ordering: `fence iorw, iorw`, followed by `cbo.flush` to an address of that location, followed by a `fence iorw, iorw`. |
| `norm:Svpbmt_hgatp_stage_override_rule` | if `hgatp`.MODE is not equal to zero, non-zero G-stage PTE PBMT bits override the attributes in the PMA to produce an intermediate set of attributes. |
| `norm:Svpbmt_vsatp_stage_override_rule` | if `vsatp`.MODE is not equal to zero, non-zero VS-stage PTE PBMT bits override the intermediate attributes to produce the final set of attributes used by accesses to the page in question. |

---

## Test Groups

### Group 1: PBMT Basic Encoding Verification

**Spec Reference**:
- PTE bits 62-61 encode the PBMT field; valid values in leaf PTEs are 0 (PMA), 1 (NC), 2 (IO)
- `norm:Svpbmt_depends_Sv39`: Svpbmt depends on the Sv39 extension
- `norm:Svpbmt_impl_may_override_pmas`: Implementations may override additional PMAs not explicitly listed

**Test Scope**: Verify the basic functionality of the three valid PBMT encodings (PMA, NC, IO) in leaf PTEs, confirming that pages with these values set can be accessed normally.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| PBMT-01 | PBMT=PMA load/store | Leaf PTE with PBMT=0 (PMA), execute load/store | Normal access, uses underlying PMA |
| PBMT-02 | PBMT=NC load/store | Leaf PTE with PBMT=1 (NC), execute load/store on main memory region | Normal access, non-cacheable semantics |
| PBMT-03 | PBMT=IO load/store | Leaf PTE with PBMT=2 (IO), execute load/store on main memory region | Normal access, strongly ordered semantics |
| PBMT-04 | PBMT=PMA exec | Leaf PTE with PBMT=0 (PMA), execute instruction fetch | Normal execution |
| PBMT-05 | PBMT=NC exec | Leaf PTE with PBMT=1 (NC), execute instruction fetch | Normal execution (non-cacheable semantics) |
| PBMT-06 | PBMT=IO exec | Leaf PTE with PBMT=2 (IO), execute instruction fetch | Implementation-defined (I/O regions may not support instruction fetch) |

---

### Group 2: PBMT Reserved Value Exception

**Spec Reference**:
- `norm:Svpbmt_leaf_pte_pbmt_reserved_3_fault`: PBMT=3 (bits 62-61 = 11) in a leaf PTE is a reserved value, triggering a page-fault

**Test Scope**: Verify that when PBMT is set to the reserved value 3 in a leaf PTE, any type of access triggers a page-fault.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| RSVD-01 | PBMT=3 load fault | Leaf PTE with PBMT=3, execute load | load page-fault |
| RSVD-02 | PBMT=3 store fault | Leaf PTE with PBMT=3, execute store | store page-fault |
| RSVD-03 | PBMT=3 exec fault | Leaf PTE with PBMT=3, execute instruction fetch | instruction page-fault |
| RSVD-04 | PBMT=3 superpage fault | 2MB superpage leaf PTE with PBMT=3, execute load | load page-fault |

---

### Group 3: Non-Leaf PTE PBMT Bit Check

**Spec Reference**:
- `norm:Svpbmt_nonleaf_pte_pbmt_must_be_zero`: Bits 62-61 of non-leaf PTEs are reserved and must be cleared, otherwise a page-fault is raised

**Test Scope**: Verify that a page-fault is triggered when PBMT bits are non-zero in a non-leaf PTE (intermediate page table pointer).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| NLPTE-01 | Non-leaf PTE PBMT=1 | Intermediate PTE (non-leaf) with PBMT=NC (01), execute load through this PTE | load page-fault |
| NLPTE-02 | Non-leaf PTE PBMT=2 | Intermediate PTE (non-leaf) with PBMT=IO (10), execute load through this PTE | load page-fault |
| NLPTE-03 | Non-leaf PTE PBMT=3 | Intermediate PTE (non-leaf) with PBMT=3 (11), execute load through this PTE | load page-fault |
| NLPTE-04 | Non-leaf PTE PBMT=0 normal | Intermediate PTE (non-leaf) with PBMT=0 (compliant), execute load through this PTE | Normal access |
| NLPTE-05 | Multi-level non-leaf PTE PBMT check | Non-leaf PTEs at different levels each set with non-zero PBMT | Each triggers a page-fault |

---

### Group 4: PBMT and Page Permission Interaction

**Spec Reference**:
- PBMT settings do not affect PTE R/W/X permission checks
- PBMT only overrides memory attributes; permission checks proceed independently per the standard page table walk algorithm
- `norm:Svpbmt_impl_may_override_pmas`: Implementations may override additional PMA constraints based on PBMT (e.g., unaligned accesses to PBMT=IO pages may trigger exceptions)

**Test Scope**: Verify that PBMT attributes do not affect RWX permission semantics, and verify PBMT interaction with the U-bit.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| PERM-01 | PBMT=NC + R-only | Leaf PTE with PBMT=NC, R-only permission, execute store | store page-fault |
| PERM-02 | PBMT=IO + R-only | Leaf PTE with PBMT=IO, R-only permission, execute store | store page-fault |
| PERM-03 | PBMT=NC + no-exec | Leaf PTE with PBMT=NC, no X permission, execute instruction fetch | instruction page-fault |
| PERM-04 | PBMT=IO + no-exec | Leaf PTE with PBMT=IO, no X permission, execute instruction fetch | instruction page-fault |
| PERM-05 | PBMT=NC + U-bit S-mode | Leaf PTE with PBMT=NC and U=1, S-mode (SUM=0) access | page-fault |
| PERM-06 | PBMT=NC + U-bit SUM=1 | Leaf PTE with PBMT=NC and U=1, S-mode (SUM=1) access | Normal access |
| ALIGN-01 | PBMT=IO unaligned load | Execute unaligned load on PBMT=IO page (e.g., 8-byte read from addr+1) | Implementation-defined: may trigger load access-fault or normal access |
| ALIGN-02 | PBMT=IO unaligned store | Execute unaligned store on PBMT=IO page (e.g., 8-byte write to addr+1) | Implementation-defined: may trigger store access-fault or normal access |
| ALIGN-03 | PBMT=PMA unaligned load (control) | Execute the same unaligned load on a PBMT=PMA page | Normal access (control baseline) |

---

### Group 5: PBMT with Different Page Sizes

**Spec Reference**:
- PBMT is defined in bits 62-61 of leaf PTEs, applicable to all leaf PTE levels
- Sv39 supports 4KB, 2MB, and 1GB pages; Sv48 additionally supports 512GB; Sv57 additionally supports 256TB
- `norm:Svpbmt_depends_Sv39`: Svpbmt depends on the Sv39 extension

**Test Scope**: Verify that PBMT attributes work correctly on 4KB pages, 2MB megapages, and 1GB gigapages.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| PGSZ-01 | 4KB page + PBMT=NC | 4KB leaf PTE with PBMT=NC, load/store | Normal access |
| PGSZ-02 | 4KB page + PBMT=IO | 4KB leaf PTE with PBMT=IO, load/store | Normal access |
| PGSZ-03 | 2MB megapage + PBMT=NC | 2MB superpage leaf PTE with PBMT=NC, load/store | Normal access |
| PGSZ-04 | 2MB megapage + PBMT=IO | 2MB superpage leaf PTE with PBMT=IO, load/store | Normal access |
| PGSZ-05 | 1GB gigapage + PBMT=NC | 1GB superpage leaf PTE with PBMT=NC, load/store | Normal access |
| PGSZ-06 | 1GB gigapage + PBMT=IO | 1GB superpage leaf PTE with PBMT=IO, load/store | Normal access |

---

### Group 6: Memory Ordering Semantics

**Spec Reference**:
- `norm:Svpbmt_obeys_mem_ordering`: When PBMT overrides memory attributes, accesses obey the ordering rules of the final effective attribute
- `norm:Svpbmt_io_pma_nc_pbmt_obey_rvwmo`: I/O PMA + PBMT=NC obeys RVWMO
- `norm:Svpbmt_io_pma_nc_pbmt_treated_as_io_and_memory`: I/O PMA + PBMT=NC is treated as both I/O and main memory access
- `norm:Svpbmt_memory_pma_io_pbmt_strong_io_ordering`: Main memory PMA + PBMT=IO obeys strong I/O ordering
- `norm:Svpbmt_memory_pma_io_pbmt_treated_as_io_and_memory`: Main memory PMA + PBMT=IO is treated as both I/O and main memory access

**Test Scope**: Verify ordering semantics after PBMT overrides memory attributes.

> **Note**: Memory ordering tests are difficult to fully validate in a single-hart environment with respect to ordering strength differences. The following tests primarily verify that accesses after PBMT override do not cause exceptions or incorrect data, and that FENCE instructions operate correctly on PBMT-overridden pages.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| ORDER-01 | Memory + PBMT=IO access correctness | Main memory region with PBMT=IO, execute multiple sequential load/store | Data correct (strong I/O ordering) |
| ORDER-02 | Memory + PBMT=IO + FENCE | Main memory region with PBMT=IO, insert `fence iorw, iorw` between load/store | Data correct |
| ORDER-03 | Memory + PBMT=NC access correctness | Main memory region with PBMT=NC, execute multiple sequential load/store | Data correct (RVWMO) |
| ORDER-04 | Memory + PBMT=NC + FENCE | Main memory region with PBMT=NC, insert `fence rw, rw` between load/store | Data correct |
| ORDER-05 | I/O region + PBMT=NC access (discouraged) | I/O region (e.g., UART) with PBMT=NC, verify RVWMO ordering takes effect. **Note: The specification discourages this configuration**; I/O device drivers relying on strong ordering rules will not function correctly | Normal access, but ordering weaker than PBMT=IO |
| ORDER-06 | PBMT=IO page FENCE coverage | Execute `fence iorw, iorw` on main memory + PBMT=IO page, verify that accesses to this page are covered by FENCE | FENCE is effective for both I/O and main memory accesses |

---

### Group 7: Aliasing and Coherence

**Spec Reference**:
- `norm:Svpbmt_aliasing_attribute`: Different virtual aliases may have different memory attributes, in which case the attribute behavior (including coherence) may be violated
- `norm:Svpbmt_noncacheable_aliasing_no_coherence_loss`: Two non-cacheable attribute aliases do not cause loss of coherence
- `norm:Svpbmt_noncacheable_aliasing_may_weaken_ordering`: Two non-cacheable attribute aliases may weaken ordering guarantees
- `norm:Svpbmt_noncacheable_aliasing_fence_prevents_ordering_loss`: `fence iorw, iorw` prevents loss of ordering between non-cacheable aliases
- `norm:Svpbmt_cacheable_aliasing_may_cause_coherence_loss`: Aliases with different cacheability attributes may cause loss of coherence
- `norm:Svpbmt_cacheable_aliasing_fence_flush_fence_required`: `fence iorw, iorw` + `cbo.flush` + `fence iorw, iorw` sequence prevents coherence and ordering loss for cacheable aliases

**Test Scope**: Verify virtual alias behavior with different PBMT attributes, and the effectiveness of the fence/flush sequences required by the specification.

> **Note**: Alias tests require mapping the same physical page to two different virtual addresses, each with different PBMT attributes. In a single-hart test environment, coherence issues may not manifest. The following tests primarily verify correct execution of fence/flush sequences (without triggering exceptions) and basic data visibility.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| ALIAS-01 | NC + IO non-cacheable alias | Same physical page mapped to VA1 (PBMT=NC) and VA2 (PBMT=IO), write through VA1 then read through VA2 | Data coherent (non-cacheable alias does not lose coherence) |
| ALIAS-02 | NC + IO alias + fence | Same physical page NC/IO alias, insert `fence iorw, iorw` between write and read | Data coherent, ordering guaranteed |
| ALIAS-03 | PMA + NC cacheability alias | Same physical page mapped to VA1 (PBMT=PMA) and VA2 (PBMT=NC), write through VA1 then read through VA2 | Behavior undefined (possible coherence loss) |
| ALIAS-04 | PMA + NC alias + fence+flush+fence | Same as above, but insert `fence iorw,iorw` + `cbo.flush` + `fence iorw,iorw` between write and read | Data coherent |
| ALIAS-05 | PMA + IO cacheability alias | Same physical page mapped to VA1 (PBMT=PMA) and VA2 (PBMT=IO), write through VA1 then read through VA2 | Behavior undefined (possible coherence loss) |
| ALIAS-06 | PMA + IO alias + fence+flush+fence | Same as above, but insert full fence+flush+fence sequence between write and read | Data coherent |
| ALIAS-07 | M-mode PMA vs S-mode PBMT=NC | M-mode writes directly to the physical page with PMA attributes; S-mode maps the same page with PBMT=NC and reads. Requires manual privilege-level management; does not use `vm_run_in_smode()` | Behavior undefined (specification permits attribute behavior violation, including coherence) |

---

### Group 8: SFENCE.VMA and PBMT

**Spec Reference**:
- After modifying the PBMT field of a PTE, SFENCE.VMA must be executed to flush the translation entries cached in the TLB
- TLB entries may have cached the old PBMT attributes, causing inconsistent behavior

**Test Scope**: Verify the flush effect of SFENCE.VMA after PBMT attribute changes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SFENCE-01 | Global flush after PBMT change | Change PBMT from PMA to NC, execute global sfence.vma, verify new attributes take effect | New attributes effective, normal access |
| SFENCE-02 | Address-specific flush after PBMT change | Change PBMT from PMA to IO, execute address-specific sfence.vma, verify new attributes take effect | New attributes effective, normal access |
| SFENCE-03 | Flush after PBMT changed to reserved value | Change PBMT from NC to 3 (reserved), execute sfence.vma then access. Verify TLB flush correctly reflects the PTE's invalid state (reserved encoding) | page-fault |
| SFENCE-04 | Flush after PBMT recovery from reserved value | Change PBMT from 3 (reserved) back to PMA, execute sfence.vma then access. Verify TLB flush correctly reflects PTE recovery from invalid to valid state | Normal access |

---

### Group 9: Multi-Mode Compatibility

**Spec Reference**:
- `norm:Svpbmt_depends_Sv39`: Svpbmt depends on the Sv39 extension
- The PBMT field (bits 62-61) is identically defined in Sv39, Sv48, and Sv57 PTE formats

**Test Scope**: Verify that PBMT attributes work correctly in Sv39, Sv48, and Sv57 modes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| MODE-01 | Sv39 + PBMT=NC | Sv39 mode leaf PTE with PBMT=NC, load/store | Normal access |
| MODE-02 | Sv48 + PBMT=NC | Sv48 mode leaf PTE with PBMT=NC, load/store | Normal access |
| MODE-03 | Sv57 + PBMT=NC | Sv57 mode leaf PTE with PBMT=NC, load/store | Normal access |

---

### ~~Group 10: Two-Stage Address Translation (H Extension, Conditional)~~ [Migrated]

> **Note**: Please refer to `DOCS/testplan/Hypervisor_cross_test_plan.md` Group 7 (Hypervisor x Svpbmt Cross Test). Please check the latest content in that document.

---

## Test Priority

| Priority | Test Group | Covered Test IDs | Rationale |
|----------|------------|-------------------|-----------|
| P0 (Required) | Group 1 (Basic Encoding), Group 2 (Reserved Value Exception), Group 3 (Non-Leaf PTE Check) | PBMT-01~06, RSVD-01~04, NLPTE-01~05 | Core functionality: PBMT encoding recognition, reserved value fault, non-leaf PTE compliance |
| P1 (Important) | Group 4 (Permission Interaction + Unaligned), Group 5 (Page Sizes), Group 6 Basic Ordering (ORDER-01~04), Group 8 (SFENCE.VMA) | PERM-01~06, ALIGN-01~03, PGSZ-01~06, ORDER-01~04, SFENCE-01~04 | PBMT orthogonality with permissions/page sizes, unaligned access constraints, basic ordering semantics, TLB flush correctness |
| P2 (Recommended) | Group 6 Advanced Ordering (ORDER-05~06), Group 7 (Aliasing Coherence ALIAS-01~06), Group 9 (Multi-Mode Compatibility) | ORDER-05~06, ALIAS-01~06, MODE-01~03 | Advanced ordering and discouraged configuration, cache coherence, cross-mode verification |
| P3 (Optional) | ALIAS-07 (Cross-Privilege Alias) | ALIAS-07 | Depends on manual privilege-level management, conditional implementation |

---

## Test Implementation Notes

### PBMT PTE Constant Definitions

The Svpbmt extension uses PTE bits 62-61 to encode the PBMT field. The following constants should be defined in test code:

### PBMT PTE Helper Functions

The following helper functions are needed for non-leaf PTE tests (Group 3) and alias tests (Group 7):

### S-mode Helper Functions

S-mode helper functions used in the test plan:

| Function | Description | Return Value |
|----------|-------------|--------------|
| `test_smode_load` | Execute load operation | 0=success |
| `test_smode_store` | Execute store operation | 0=success |
| `test_smode_read_write` | Write magic value and read back to verify | 0=success, non-0=failure |
| `test_smode_load_expect_fault` | Execute load, expecting a fault | fault cause (e.g., CAUSE_LPF) |
| `test_smode_store_expect_fault` | Execute store, expecting a fault | fault cause (e.g., CAUSE_SPF) |
| `test_smode_exec_expect_fault` | Jump to execute, expecting a fault | fault cause (e.g., CAUSE_IPF) |
| `test_smode_sequential_rw` | Multiple sequential stores then load back to verify | 0=all correct |
| `test_smode_store_fence_load` | store + fence iorw,iorw + load back to verify | 0=data correct |
| `test_smode_alias_write_read` | Write through VA1 then read through VA2 to verify | 0=data coherent |
| `test_smode_alias_flush_sequence` | Write through VA1 + fence+cbo.flush+fence + read through VA2 | 0=data coherent |

### File Organization

Svpbmt tests depend on the virtual memory test framework. The recommended file organization is as follows:

### Makefile Configuration

### Common Test Pattern

Svpbmt test cases follow the following pattern:

### Key Considerations

1. **PBMT constants need to be added**: The current `common/vm/vm_defs.h` does not define PBMT-related constants. The following macro definitions need to be added: `PTE_PBMT_SHIFT`, `PTE_PBMT_MASK`, `PBMT_PMA`, `PBMT_NC`, `PBMT_IO`, `PBMT_RSVD` (see "PBMT PTE Constant Definitions" section).

2. **pt_get_pte function needs to be added**: The current `common/vm/page_table.c` does not provide an API for obtaining a pointer to a PTE at a specified level. The `pt_get_pte()` function needs to be implemented for manually modifying the PBMT field of PTEs in non-leaf PTE tests (Group 3).

3. **pt_map_page PTE_FLAGS_MASK truncation issue (must fix)**: The current `PTE_FLAGS_MASK` value is `0x3FF` (covering only bits 9-0), while PBMT is located at bits 62-61. If `pt_map_page()` internally uses this mask to truncate the flags parameter, PBMT values will be completely discarded, causing all PBMT tests to fail. **The `pt_map_page()` implementation must be modified** to ensure PBMT high-order flags are not truncated. Specific approach: when constructing the PTE, handle PBMT bits and low-order flags separately then merge them, or extend the flags mask to include PBMT bits (e.g., define `PTE_FLAGS_MASK_EXT` covering bits 63-54 and bits 9-0).

4. **Identity mapping coverage**: Identity mappings in tests must cover the code segment, data segment, stack, page table pool, and UART I/O region. Identity mappings use the default PBMT=PMA (bits 62-61 = 00), which does not affect memory attributes of code execution regions.

5. **PTE_A and PTE_D bits**: Unless the test target is the A/D bit itself, always set `PTE_A | PTE_D` when mapping, to avoid unnecessary page-faults caused by Svade semantics.

6. **Test page isolation**: Test pages for verifying PBMT attributes should be mapped separately from code execution regions, using an independent `TEST_REGION_BASE` address, to avoid setting code execution regions to NC or IO attributes which could cause performance issues or exceptions.

7. **PBMT=IO effect on main memory**: After setting a main memory region to PBMT=IO, accesses to that region obey strong I/O ordering rules. Some implementations may trigger exceptions for unaligned accesses to PBMT=IO pages (`norm:Svpbmt_impl_may_override_pmas`), even when the underlying region is main memory.

8. **Alias test physical page addresses**: Alias tests (Group 7) require mapping the same physical page to two different virtual addresses. The two virtual addresses should reside in different 2MB regions to ensure that different level-0 page table pages are used.

9. **cbo.flush instruction dependency**: The `cbo.flush` instruction used in alias tests (ALIAS-04~06) comes from the Zicbom extension. If the target platform does not implement Zicbom, these tests should be skipped.

10. **Page-fault cause encodings**:
    - `CAUSE_IPF` (12): Instruction page-fault
    - `CAUSE_LPF` (13): Load page-fault
    - `CAUSE_SPF` (15): Store/AMO page-fault

11. **QEMU support**: QEMU 7.0+ supports the Svpbmt extension. When launching, `svpbmt=true` must be enabled in the `-cpu` parameter.

12. **PMP configuration**: `vm_run_in_smode()` automatically configures PMP entry 15 as full-address-space NAPOT RWX. If testing PMP and PBMT interaction is required, PMP configuration must be managed manually.

13. **Hypervisor extension**: Two-stage translation tests (formerly Group 10, HSTG-01~04) have been migrated to `DOCS/testplan/Hypervisor_cross_test_plan.md` Group 7.

---

## Specification Coverage Matrix

| Spec Label | Covered Test IDs |
|------------|------------------|
| `norm:Svpbmt_depends_Sv39` | MODE-01, MODE-02, MODE-03 |
| `norm:Svpbmt_impl_may_override_pmas` | ALIGN-01~03 |
| `norm:Svpbmt_nonleaf_pte_pbmt_must_be_zero` | NLPTE-01~05 |
| `norm:Svpbmt_leaf_pte_pbmt_reserved_3_fault` | RSVD-01~04 |
| `norm:Svpbmt_obeys_mem_ordering` | ORDER-01~06 |
| `norm:Svpbmt_io_pma_nc_pbmt_obey_rvwmo` | ORDER-05 |
| `norm:Svpbmt_io_pma_nc_pbmt_treated_as_io_and_memory` | ORDER-05, ORDER-06 |
| `norm:Svpbmt_memory_pma_io_pbmt_strong_io_ordering` | ORDER-01, ORDER-02 |
| `norm:Svpbmt_memory_pma_io_pbmt_treated_as_io_and_memory` | ORDER-02, ORDER-06 |
| `norm:Svpbmt_aliasing_attribute` | ALIAS-01~07 |
| `norm:Svpbmt_noncacheable_aliasing_no_coherence_loss` | ALIAS-01 |
| `norm:Svpbmt_noncacheable_aliasing_may_weaken_ordering` | ALIAS-01, ALIAS-02 |
| `norm:Svpbmt_noncacheable_aliasing_fence_prevents_ordering_loss` | ALIAS-02 |
| `norm:Svpbmt_cacheable_aliasing_may_cause_coherence_loss` | ALIAS-03, ALIAS-05 |
| `norm:Svpbmt_cacheable_aliasing_fence_flush_fence_required` | ALIAS-04, ALIAS-06 |
| `norm:Svpbmt_hgatp_stage_override_rule` | HSTG-01, HSTG-03 (migrated to `Hypervisor_cross_test_plan.md` Group 7) |
| `norm:Svpbmt_vsatp_stage_override_rule` | HSTG-02, HSTG-03, HSTG-04 (migrated to `Hypervisor_cross_test_plan.md` Group 7) |

---

## References

- RISC-V Privileged Specification — Svpbmt Extension (`SPEC/svpbmt.adoc`)
- `SPEC/supervisor.adoc` — Supervisor-Level ISA (PTE format definition, SFENCE.VMA definition)
- `docs/vm_test_framework.md` — Virtual memory test framework documentation
- `docs/vm_test_plan.md` — Virtual memory test plan (PTE permissions, SFENCE.VMA reference)
- `docs/svnapot_test_plan.md` — Svnapot test plan (PBMT interaction test reference)
- `common/vm/vm.h` — VM API declarations
- `common/vm/vm_defs.h` — Constants and macro definitions
