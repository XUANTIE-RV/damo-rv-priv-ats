**[中文](../testplan/Svpbmt_test_plan.md) | English**

# Svpbmt Extension Test Plan

This document describes the test plan for the Svpbmt (Page-Based Memory Types) extension. The Svpbmt extension uses bits 62-61 (the PBMT field) in the leaf PTEs of Sv39/Sv48/Sv57 to override the page's physical memory attributes (PMA), covering PBMT encoding verification, reserved value exceptions, non-leaf PTE checks, memory ordering semantics, aliasing coherence, and other specification requirements.

---

## SPEC Sections Covered by This Document

This plan is based on the following official RISC-V specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/svpbmt.adoc` — Svpbmt extension: PBMT encoding table (pbmt), leaf/non-leaf PTE PBMT rules, reserved value page-fault, memory ordering and aliasing coherence, two-stage translation PBMT override rules
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — PTE format definition, SFENCE.VMA definition (TLB synchronization after PBMT changes)

Official repository:

- https://github.com/riscv/riscv-isa-manual (the above path files within the repository)

---

## Overview

The Svpbmt extension uses bits 62-61 (the PBMT field) in leaf page table entries (leaf PTE) of Sv39, Sv48, and Sv57 to override the page's physical memory attributes (PMA). The PBMT field encoding is as follows (a normative reference, from the pbmt encoding table in `svpbmt.adoc`):

| Mode | Value | Requested Memory Attribute |
|------|-------|---------------------------|
| PMA  | 0     | No override, use the underlying PMA |
| NC   | 1     | Non-cacheable, idempotent, weakly ordered (RVWMO), main memory |
| IO   | 2     | Non-cacheable, non-idempotent, strongly ordered (I/O ordering), I/O |
| —    | 3     | Reserved (for future standard use); using it in a leaf PTE triggers a page-fault |

**Dependency**: The Svpbmt extension depends on the Sv39 extension (`norm:Svpbmt_depends_Sv39`).

---

## Covered Specification Points

The table below lists the specification points covered by this plan. Entries with the `norm:` prefix are official SPEC normative rule tags; entries without the prefix are specification points summarized by this plan from the SPEC text.

| Norm ID | Original Text | Description |
|---------|---------------|-------------|
| `norm:Svpbmt_depends_Sv39` | The Svpbmt extension depends on the Sv39 extension. | The Svpbmt extension depends on the Sv39 extension. |
| `norm:Svpbmt_impl_may_override_pmas` | Implementations may override additional PMAs not explicitly listed in <<pbmt>>. | Implementations may override additional PMAs not explicitly listed in the pbmt table (e.g., unaligned accesses to PBMT=IO pages may trigger exceptions). |
| `norm:Svpbmt_nonleaf_pte_pbmt_must_be_zero` | Until their use is defined by a standard extension, they must be cleared by software for forward compatibility, or else a page-fault exception is raised. | Bits 62-61 of non-leaf PTEs must be cleared by software before their use is defined by a standard extension, otherwise a page-fault exception is raised. |
| `norm:Svpbmt_leaf_pte_pbmt_reserved_3_fault` | Until this value is defined by a standard extension, using this reserved value in a leaf PTE raises a page-fault exception. | Using the reserved value 3 in a leaf PTE raises a page-fault exception. |
| `norm:Svpbmt_obeys_mem_ordering` | memory accesses to such pages obey the memory ordering rules of the final effective attribute. | Memory accesses to such pages obey the memory ordering rules of the final effective attribute. |
| `norm:Svpbmt_io_pma_nc_pbmt_obey_rvwmo` | If the underlying physical memory attribute for a page is I/O, and the page has PBMT=NC, then accesses to that page obey RVWMO. | When the underlying PMA is I/O and PBMT=NC, accesses to that page obey RVWMO. |
| `norm:Svpbmt_io_pma_nc_pbmt_treated_as_io_and_memory` | accesses to such pages are considered to be both I/O and main memory accesses for the purposes of FENCE, .aq, and .rl. | For the purposes of FENCE, .aq, and .rl, accesses to such pages are considered to be both I/O and main memory accesses. |
| `norm:Svpbmt_memory_pma_io_pbmt_strong_io_ordering` | accesses to that page obey strong channel 0 I/O ordering rules. | When the underlying PMA is main memory and PBMT=IO, accesses to that page obey strong channel 0 I/O ordering rules. |
| `norm:Svpbmt_memory_pma_io_pbmt_treated_as_io_and_memory` | accesses to such pages are considered to be both I/O and main memory accesses for the purposes of FENCE, .aq, and .rl. | For the purposes of FENCE, .aq, and .rl, accesses to such pages are considered to be both I/O and main memory accesses. |
| `norm:Svpbmt_aliasing_attribute` | When Svpbmt is used with non-zero PBMT encodings, it is possible for multiple virtual aliases of the same physical page to exist simultaneously with different memory attributes ... the behaviors dictated by the attributes (including coherence) may be violated. | When non-zero PBMT encodings are used, multiple virtual aliases of the same physical page may simultaneously have different memory attributes; the behaviors dictated by the attributes (including coherence) may be violated. |
| `norm:Svpbmt_noncacheable_aliasing_no_coherence_loss` | Accessing the same location using different attributes that are both non-cacheable (e.g., NC and IO) does not cause loss of coherence. | Accessing the same location using different attributes that are both non-cacheable (e.g., NC and IO) does not cause loss of coherence. |
| `norm:Svpbmt_noncacheable_aliasing_may_weaken_ordering` | might result in weaker memory ordering than the stricter attribute ordinarily guarantees. | But it might result in weaker memory ordering than the stricter attribute ordinarily guarantees. |
| `norm:Svpbmt_noncacheable_aliasing_fence_prevents_ordering_loss` | `fence iorw, iorw` instruction between such accesses suffices to prevent loss of memory ordering. | A `fence iorw, iorw` instruction between such accesses suffices to prevent loss of memory ordering. |
| `norm:Svpbmt_cacheable_aliasing_may_cause_coherence_loss` | may cause loss of coherence. | Accessing the same location using different cacheability attributes may cause loss of coherence. |
| `norm:Svpbmt_cacheable_aliasing_fence_flush_fence_required` | prevents both loss of coherence and loss of memory ordering: `fence iorw, iorw`, followed by `cbo.flush` to an address of that location, followed by a `fence iorw, iorw`. | The sequence preventing both loss of coherence and memory ordering: `fence iorw, iorw` + `cbo.flush` to that location + `fence iorw, iorw`. |
| `norm:Svpbmt_hgatp_stage_override_rule` | if `hgatp`.MODE is not equal to zero, non-zero G-stage PTE PBMT bits override the attributes in the PMA to produce an intermediate set of attributes. | When `hgatp`.MODE is non-zero, non-zero G-stage leaf PTE PBMT bits override the PMA attributes to produce an intermediate set of attributes. |
| `norm:Svpbmt_vsatp_stage_override_rule` | if `vsatp`.MODE is not equal to zero, non-zero VS-stage PTE PBMT bits override the intermediate attributes to produce the final set of attributes. | When `vsatp`.MODE is non-zero, non-zero VS-stage leaf PTE PBMT bits override the intermediate attributes to produce the final set of attributes. |
| `Svpbmt_leaf_pte_pbmt_overrides_pma` | bits 62-61 of a leaf PTE indicate the use of page-based memory types that override the PMA(s) for the associated memory pages (per <<pbmt>>). | Bits 62-61 of a leaf PTE override the PMA(s) of the associated memory pages per the pbmt encoding table; permission (R/W/X) checks proceed independently, and SFENCE.VMA synchronization is required after a PBMT change. |

---

## Out of Scope

- **Hypervisor two-stage translation**: The G-stage / VS-stage PBMT stacked override rules are covered by `Hypervisor_Sv_test_plan.md` (Hypervisor x Svpbmt cross tests).
- **Coherence in multi-core scenarios**: This plan focuses on single-hart behavior; the aliasing coherence cases primarily verify correct execution of the fence/flush sequences and basic data visibility.

---

## Test Groups

### Group 1: PBMT Basic Encoding Verification

**Spec Reference**:
- `Svpbmt_leaf_pte_pbmt_overrides_pma`: Bits 62-61 of a leaf PTE encode the PBMT field; valid values are 0 (PMA), 1 (NC), 2 (IO)
- `norm:Svpbmt_depends_Sv39`: Svpbmt depends on the Sv39 extension

**Test Scope**: Verify the basic functionality of the three valid PBMT encodings (PMA, NC, IO) in leaf PTEs, confirming that pages with these values set can be accessed normally.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| PBMT-01 | PBMT=PMA load/store | Leaf PTE with PBMT=0 (PMA), execute load/store | Normal access, uses the underlying PMA |
| PBMT-02 | PBMT=NC load/store | Leaf PTE with PBMT=1 (NC), execute load/store on a main memory region | Normal access, non-cacheable semantics |
| PBMT-03 | PBMT=IO load/store | Leaf PTE with PBMT=2 (IO), execute load/store on a main memory region | Normal access, strongly ordered semantics |
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

**Test Scope**: Verify that a page-fault is triggered when the PBMT bits are non-zero in a non-leaf PTE (intermediate page table pointer).

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
- `Svpbmt_leaf_pte_pbmt_overrides_pma`: PBMT only overrides memory attributes and does not affect the PTE's R/W/X permission checks (permission checks proceed independently per the standard page table walk algorithm)
- `norm:Svpbmt_impl_may_override_pmas`: Implementations may override additional PMA constraints based on PBMT (e.g., unaligned accesses to PBMT=IO pages may trigger exceptions)

**Test Scope**: Verify that PBMT attributes do not affect RWX permission semantics, and verify PBMT interaction with the U-bit and unaligned accesses.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| PERM-01 | PBMT=NC + R-only | Leaf PTE with PBMT=NC, R-only permission, execute store | store page-fault |
| PERM-02 | PBMT=IO + R-only | Leaf PTE with PBMT=IO, R-only permission, execute store | store page-fault |
| PERM-03 | PBMT=NC + no-exec | Leaf PTE with PBMT=NC, no X permission, execute instruction fetch | instruction page-fault |
| PERM-04 | PBMT=IO + no-exec | Leaf PTE with PBMT=IO, no X permission, execute instruction fetch | instruction page-fault |
| PERM-05 | PBMT=NC + U-bit S-mode | Leaf PTE with PBMT=NC and U=1, S-mode (SUM=0) access | page-fault |
| PERM-06 | PBMT=NC + U-bit SUM=1 | Leaf PTE with PBMT=NC and U=1, S-mode (SUM=1) access | Normal access |
| ALIGN-01 | PBMT=IO unaligned load | Execute an unaligned load on a PBMT=IO page (e.g., 8-byte read from addr+1) | Implementation-defined: may trigger a load access-fault or normal access |
| ALIGN-02 | PBMT=IO unaligned store | Execute an unaligned store on a PBMT=IO page (e.g., 8-byte write to addr+1) | Implementation-defined: may trigger a store access-fault or normal access |
| ALIGN-03 | PBMT=PMA unaligned load (control) | Execute the same unaligned load on a PBMT=PMA page | Normal access (control baseline) |

---

### Group 5: PBMT with Different Page Sizes

**Spec Reference**:
- `Svpbmt_leaf_pte_pbmt_overrides_pma`: PBMT is defined in bits 62-61 of leaf PTEs, applicable to all leaf PTE levels
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

> [!NOTE]
> Memory ordering tests are difficult to fully validate in a single-hart environment with respect to ordering strength differences. The following tests primarily verify that accesses after a PBMT override do not cause exceptions or incorrect data, and that FENCE instructions operate correctly on PBMT-overridden pages.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| ORDER-01 | Memory + PBMT=IO access correctness | Main memory region with PBMT=IO, execute multiple sequential load/store | Data correct (strong I/O ordering) |
| ORDER-02 | Memory + PBMT=IO + FENCE | Main memory region with PBMT=IO, insert `fence iorw, iorw` between load/store | Data correct |
| ORDER-03 | Memory + PBMT=NC access correctness | Main memory region with PBMT=NC, execute multiple sequential load/store | Data correct (RVWMO) |
| ORDER-04 | Memory + PBMT=NC + FENCE | Main memory region with PBMT=NC, insert `fence rw, rw` between load/store | Data correct |
| ORDER-05 | I/O region + PBMT=NC access (discouraged) | I/O region (e.g., UART) with PBMT=NC, verify RVWMO ordering takes effect. The specification discourages this configuration: I/O device drivers relying on strong ordering rules will not function correctly | Normal access, but ordering weaker than PBMT=IO |
| ORDER-06 | PBMT=IO page FENCE coverage | Execute `fence iorw, iorw` on a main memory + PBMT=IO page, verify that accesses to this page are covered by FENCE | FENCE is effective for both I/O and main memory accesses |

---

### Group 7: Aliasing and Coherence

**Spec Reference**:
- `norm:Svpbmt_aliasing_attribute`: Different virtual aliases may have different memory attributes, in which case the attribute behavior (including coherence) may be violated
- `norm:Svpbmt_noncacheable_aliasing_no_coherence_loss`: Two non-cacheable attribute aliases do not cause loss of coherence
- `norm:Svpbmt_noncacheable_aliasing_may_weaken_ordering`: Two non-cacheable attribute aliases may weaken ordering guarantees
- `norm:Svpbmt_noncacheable_aliasing_fence_prevents_ordering_loss`: `fence iorw, iorw` prevents loss of ordering between non-cacheable aliases
- `norm:Svpbmt_cacheable_aliasing_may_cause_coherence_loss`: Aliases with different cacheability attributes may cause loss of coherence
- `norm:Svpbmt_cacheable_aliasing_fence_flush_fence_required`: The `fence iorw, iorw` + `cbo.flush` + `fence iorw, iorw` sequence prevents coherence and ordering loss for cacheable aliases

**Test Scope**: Verify virtual alias behavior with different PBMT attributes, and the effectiveness of the fence/flush sequences required by the specification.

> [!NOTE]
> Alias tests require mapping the same physical page to two different virtual addresses, each with different PBMT attributes. In a single-hart test environment, coherence issues may not manifest; the following tests primarily verify correct execution of the fence/flush sequences (without triggering exceptions) and basic data visibility. ALIAS-04~06 use the `cbo.flush` instruction from the Zicbom extension; if the target platform does not implement Zicbom, these cases should be skipped.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| ALIAS-01 | NC + IO non-cacheable alias | Same physical page mapped to VA1 (PBMT=NC) and VA2 (PBMT=IO), write through VA1 then read through VA2 | Data coherent (non-cacheable aliases do not lose coherence) |
| ALIAS-02 | NC + IO alias + fence | Same physical page NC/IO alias, insert `fence iorw, iorw` between write and read | Data coherent, ordering guaranteed |
| ALIAS-03 | PMA + NC cacheability alias | Same physical page mapped to VA1 (PBMT=PMA) and VA2 (PBMT=NC), write through VA1 then read through VA2 | Behavior undefined (possible coherence loss) |
| ALIAS-04 | PMA + NC alias + fence+flush+fence | Same as above, but insert `fence iorw,iorw` + `cbo.flush` + `fence iorw,iorw` between write and read | Data coherent |
| ALIAS-05 | PMA + IO cacheability alias | Same physical page mapped to VA1 (PBMT=PMA) and VA2 (PBMT=IO), write through VA1 then read through VA2 | Behavior undefined (possible coherence loss) |
| ALIAS-06 | PMA + IO alias + fence+flush+fence | Same as above, but insert the full fence+flush+fence sequence between write and read | Data coherent |
| ALIAS-07 | M-mode PMA vs S-mode PBMT=NC | M-mode writes directly to the physical page with PMA attributes; S-mode maps the same page with PBMT=NC and reads (requires manual privilege-level management) | Behavior undefined (the specification permits attribute behavior violation, including coherence) |

---

### Group 8: SFENCE.VMA and PBMT

**Spec Reference**:
- `Svpbmt_leaf_pte_pbmt_overrides_pma`: After modifying the PBMT field of a PTE, SFENCE.VMA must be executed to flush the translation entries cached in the TLB; otherwise the TLB may still cache the old PBMT attributes

**Test Scope**: Verify the flush effect of SFENCE.VMA after PBMT attribute changes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SFENCE-01 | Global flush after PBMT change | Change PBMT from PMA to NC, execute a global sfence.vma, verify the new attributes take effect | New attributes effective, normal access |
| SFENCE-02 | Address-specific flush after PBMT change | Change PBMT from PMA to IO, execute an address-specific sfence.vma, verify the new attributes take effect | New attributes effective, normal access |
| SFENCE-03 | Flush after PBMT changed to reserved value | Change PBMT from NC to 3 (reserved), execute sfence.vma then access. Verify the TLB flush correctly reflects the PTE's invalid state (reserved encoding) | page-fault |
| SFENCE-04 | Flush after PBMT recovery from reserved value | Change PBMT from 3 (reserved) back to PMA, execute sfence.vma then access. Verify the TLB flush correctly reflects the PTE recovering from an invalid to a valid state | Normal access |

---

### Group 9: Multi-Mode Compatibility

**Spec Reference**:
- `norm:Svpbmt_depends_Sv39`: Svpbmt depends on the Sv39 extension
- `Svpbmt_leaf_pte_pbmt_overrides_pma`: The PBMT field (bits 62-61) is identically defined in the Sv39, Sv48, and Sv57 PTE formats

**Test Scope**: Verify that PBMT attributes work correctly in Sv39, Sv48, and Sv57 modes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| MODE-01 | Sv39 + PBMT=NC | Sv39 mode leaf PTE with PBMT=NC, load/store | Normal access |
| MODE-02 | Sv48 + PBMT=NC | Sv48 mode leaf PTE with PBMT=NC, load/store | Normal access |
| MODE-03 | Sv57 + PBMT=NC | Sv57 mode leaf PTE with PBMT=NC, load/store | Normal access |

---

## Test Priority

| Priority | Test Group | Covered Test IDs | Rationale |
|----------|------------|------------------|-----------|
| P0 (Required) | Group 1 (basic encoding), Group 2 (reserved value exception), Group 3 (non-leaf PTE check) | PBMT-01~06, RSVD-01~04, NLPTE-01~05 | Core functionality: PBMT encoding recognition, reserved value fault, non-leaf PTE compliance |
| P1 (Important) | Group 4 (permission interaction + unaligned), Group 5 (page sizes), Group 6 basic ordering (ORDER-01~04), Group 8 (SFENCE.VMA) | PERM-01~06, ALIGN-01~03, PGSZ-01~06, ORDER-01~04, SFENCE-01~04 | PBMT orthogonality with permissions/page sizes, unaligned access constraints, basic ordering semantics, TLB flush correctness |
| P2 (Recommended) | Group 6 advanced ordering (ORDER-05~06), Group 7 (aliasing coherence ALIAS-01~06), Group 9 (multi-mode compatibility) | ORDER-05~06, ALIAS-01~06, MODE-01~03 | Advanced ordering and discouraged configuration, cache coherence, cross-mode verification |
| P3 (Optional) | ALIAS-07 (cross-privilege alias) | ALIAS-07 | Depends on manual privilege-level management, conditional implementation |

---

## Result Determination Principles

- If the platform implements Svpbmt but its behavior deviates from the SPEC (e.g., reserved value 3 does not trigger a page-fault, non-zero non-leaf PTE PBMT does not trigger a page-fault, the TLB is not flushed after a PBMT change, etc.): keep the test case failing, compare against the SPEC, and record the issue in the `bugs/` directory. Modifying the test case or adding a workaround to accommodate an incorrect implementation is prohibited.
- "Implementation-defined / behavior undefined" cases (PBMT-06, ALIGN-01~02, ALIAS-03/05/07): record the actual observed behavior without making strong assertions.

---

## References

- `svpbmt.adoc` — Svpbmt extension definition
- `supervisor.adoc` — PTE format, SFENCE.VMA definition
- `Svnapot_test_plan.md` — Svnapot test plan (NAPOT x PBMT interaction reference)
- `Hypervisor_Sv_test_plan.md` — Hypervisor x Svpbmt cross test plan (two-stage PBMT override)

---

## Appendix A: Specification Point Coverage Matrix

The table below indicates which test cases cover each specification point listed in the "Covered Specification Points" section. Two-stage translation related specification points are covered by `Hypervisor_Sv_test_plan.md`.

| Norm ID | Covered Test IDs |
|---------|------------------|
| `norm:Svpbmt_depends_Sv39` | MODE-01~03, PBMT-01~06 |
| `norm:Svpbmt_impl_may_override_pmas` | ALIGN-01~03 |
| `norm:Svpbmt_nonleaf_pte_pbmt_must_be_zero` | NLPTE-01~05 |
| `norm:Svpbmt_leaf_pte_pbmt_reserved_3_fault` | RSVD-01~04, SFENCE-03, SFENCE-04 |
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
| `norm:Svpbmt_hgatp_stage_override_rule` | Covered by `Hypervisor_Sv_test_plan.md` (HCROSS-SVPBMT-01, HCROSS-SVPBMT-03, HCROSS-SVPBMT-04) |
| `norm:Svpbmt_vsatp_stage_override_rule` | Covered by `Hypervisor_Sv_test_plan.md` (HCROSS-SVPBMT-02, HCROSS-SVPBMT-03, HCROSS-SVPBMT-04) |
| `Svpbmt_leaf_pte_pbmt_overrides_pma` | PBMT-01~06, PERM-01~06, PGSZ-01~06, SFENCE-01~04, MODE-01~03 |
