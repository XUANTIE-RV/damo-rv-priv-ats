**[中文](../testplan/Svnapot_test_plan.md) | English**

## RISC-V Svnapot Compliance Test Plan

**Spec Reference**: RISC-V Privileged Specification, "Svnapot" Extension for NAPOT Translation Contiguity, Version 1.0 (`SPEC/svnapot.adoc`)

---

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:Svnapot_pte_N` | N=1 |
| `norm:Svnapot_range_napot` | Such ranges must be of a naturally aligned power-of-2 (NAPOT) granularity larger than the base page size. |
| `norm:Svnapot_depends_Sv39` | The Svnapot extension depends on the Sv39 extension. |
| `norm:Svnapot_valid_encoding` | valid according to <<ptenapot>> |
| `norm:Svnapot_implicit_read_ppn_subst` | implicit reads of a NAPOT PTE return a copy of _pte_ in which __pte__.__ppn__[__i__][__pte__.__napot_bits__-1:0] is replaced by __vpn__[__i__][__pte__.__napot_bits__-1:0]. |
| `norm:Svnapot_reserved_encoding_fault` | reserved according to <<ptenapot>>, then a page-fault exception must be raised. |
| `norm:Svnapot_cache_entries` | Implicit reads of NAPOT page table entries may create address-translation cache entries mapping _a_ + _j_×PTESIZE to a copy of _pte_ in which _pte_._ppn_[_i_][_pte_.__napot_bits__-1:0] is replaced by _vpn[i][pte.napot_bits_-1:0], for any or all _j_ such that __j__ >> __napot_bits__ = __vpn__[__i__] >> __napot_bits__, all for the address space identified in _satp_ as loaded by step 1. |
| `norm:Svnapot_hyp_gstage` | Svnapot is also supported in G-stage translation. |

---

### Test Objectives

Verify whether the Svnapot extension implementation of a RISC-V processor complies with the specification, focusing on:

1. Basic functionality of the PTE N bit: N=1 enables NAPOT translation contiguity
2. Correct encoding and address translation of 64 KiB NAPOT pages
3. Reserved encoding triggers page-fault exceptions
4. PPN substitution semantics for NAPOT PTEs (low bits of ppn[i] replaced by vpn[i])
5. RWX permission control for NAPOT pages
6. Interaction between NAPOT pages and SFENCE.VMA
7. Dependency of Svnapot on the Sv39 extension
8. A/D bit behavior of NAPOT pages and independence of NAPOT alias A/D bits
9. Multi-mode support (Sv39/Sv48/Sv57)
10. PTE bits 5-0 consistency and NAPOT alias behavior
11. RSW bit reserved semantics in NAPOT PTEs
12. NAPOT region boundary alignment verification
13. PBMT and NAPOT interaction (Conditional, depends on Svpbmt extension)

---

### Test Groups

---

#### Group 1: 64 KiB NAPOT Page Basic Mapping

**Spec Reference**:
- `norm:Svnapot_pte_N`: When N=1, the PTE represents a part of a contiguous virtual-to-physical translation range
- `norm:Svnapot_range_napot`: Contiguous ranges must be of naturally aligned power-of-2 granularity, larger than the base page size
- `norm:Svnapot_valid_encoding`: When i=0 and pte.ppn[0] is encoded as `x xxxx 1000`, it represents a 64 KiB contiguous region with napot_bits=4
- `norm:Svnapot_implicit_read_ppn_subst`: When implicitly reading a NAPOT PTE, pte.ppn[i][napot_bits-1:0] is replaced by vpn[i][napot_bits-1:0]

**Test Scope**: Verify basic address translation functionality of 64 KiB NAPOT pages.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| NAPOT64-01 | 64 KiB NAPOT Page Read Access | Configure a NAPOT PTE with N=1 and ppn[0]=`xxxx1000`, mapping a 64 KiB contiguous region; S-mode reads the base address of the region | Read succeeds, address translation is correct |
| NAPOT64-02 | 64 KiB NAPOT Page Write Access | Same configuration as above; S-mode writes to the base address of the region | Write succeeds |
| NAPOT64-03 | 64 KiB NAPOT Page Last Address Access | S-mode reads the last byte of the 64 KiB region | Read succeeds |
| NAPOT64-04 | Multiple Offset Accesses within 64 KiB NAPOT Region | Access offsets 0x0000, 0x1000, 0x8000, and 0xF000 within the 64 KiB region respectively | All accesses succeed, translating to the corresponding physical addresses |
| NAPOT64-05 | Access Outside NAPOT Region | Access the first byte outside the NAPOT region (base + 64 KiB) | page-fault (unmapped) |
| NAPOT64-06 | Multiple NAPOT 64 KiB Regions | Configure two adjacent 64 KiB NAPOT regions and access each | Both regions translate correctly |

---

#### Group 2: PPN Substitution Semantics Verification

**Spec Reference**:
- `norm:Svnapot_implicit_read_ppn_subst`: Implicit reads of a NAPOT PTE return a copy in which pte.ppn[i][napot_bits-1:0] is replaced by vpn[i][napot_bits-1:0]

**Test Scope**: Verify that the physical address translation of NAPOT PTEs correctly replaces the low bits of ppn with the low bits of vpn.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| PPN-01 | Basic PPN Substitution Verification | Within a 64 KiB NAPOT region, write different magic values to different 4 KiB pages and read them back to verify correct physical addresses | Each 4 KiB page is written to an independent physical location |
| PPN-02 | Offset 0x0 Translation within Region | Access the NAPOT region at VA offset 0x0; verify translation to PA offset 0x0 | PA = base_pa + 0x0 |
| PPN-03 | Offset 0x4000 Translation within Region | Access the NAPOT region at VA offset 0x4000; verify translation to PA offset 0x4000 | PA = base_pa + 0x4000 |
| PPN-04 | Offset 0xF000 Translation within Region | Access the NAPOT region at VA offset 0xF000; verify translation to PA offset 0xF000 | PA = base_pa + 0xF000 |
| PPN-05 | Non-Identity-Mapped PPN Substitution | Set up a 64 KiB NAPOT mapping with different VA and PA; verify that various offsets within the region translate to the correct PAs | PA addresses at all offsets are correct |

---

#### Group 3: Reserved Encoding Exceptions

**Spec Reference**:
- `norm:Svnapot_reserved_encoding_fault`: When the PTE encoding is marked as Reserved in the <<ptenapot>> table, a page-fault exception must be raised

According to the <<ptenapot>> table in the specification (for level i=0), the following ppn[0] encodings are reserved when N=1:
- `x xxxx xxx1`: Reserved
- `x xxxx xx1x`: Reserved
- `x xxxx x1xx`: Reserved
- `x xxxx 0xxx` (ppn[0][3]=0 and not the `1000` pattern): Reserved

**Test Scope**: Verify that all reserved encodings trigger a page-fault.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| RSVD-01 | ppn[0] = xxx1 (bit 0 = 1) | N=1, ppn[0] low 4 bits are 0001 | page-fault |
| RSVD-02 | ppn[0] = xx1x (bit 1 = 1) | N=1, ppn[0] low 4 bits are 0010 | page-fault |
| RSVD-03 | ppn[0] = x1xx (bit 2 = 1) | N=1, ppn[0] low 4 bits are 0100 | page-fault |
| RSVD-04 | ppn[0] = 0000 (bit 3 = 0) | N=1, ppn[0] low 4 bits are 0000 (Reserved: `x xxxx 0xxx`) | page-fault |
| RSVD-05 | ppn[0] = 0101 (multiple reserved bits) | N=1, ppn[0] low 4 bits are 0101 | page-fault |
| RSVD-06 | ppn[0] = 0111 (multiple reserved bits) | N=1, ppn[0] low 4 bits are 0111 | page-fault |
| RSVD-07 | ppn[0] = 1001 (bit 0 and bit 3 both set) | N=1, ppn[0] low 4 bits are 1001 | page-fault |
| RSVD-08 | N=1 at level i>=1 (Reserved, level 1) | NAPOT PTE at level i>=1 (any ppn encoding is reserved); set N=1 at level 1 (2 MB superpage) | page-fault |
| RSVD-09 | N=1 at level i>=1 (Reserved, level 2) | Set N=1 at level 2 (1 GB gigapage); all ppn encodings are reserved | page-fault |

---

#### Group 4: NAPOT PTE Permission Control

**Spec Reference**:
- `norm:Svnapot_pte_N`: When N=1, the PTE represents a contiguous translation range, and PTE bits 5-0 (including V, R, W, X, U, G) are identical across all translations within the range
- NAPOT PTEs behave the same as non-NAPOT PTEs in the address translation algorithm (except for PPN substitution and encoding checks)

**Test Scope**: Verify that the R, W, X, and U permission bits of NAPOT pages take effect correctly.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| PERM-01 | NAPOT Read-Only Page (R only) | N=1 NAPOT page with only R permission set; S-mode writes | store page-fault |
| PERM-02 | NAPOT Read-Write Page (RW) | N=1 NAPOT page with RW permissions; S-mode reads and writes | Read and write succeed |
| PERM-03 | NAPOT Executable Page (RX) | N=1 NAPOT page with RX permissions; S-mode jumps and executes | Execution succeeds |
| PERM-04 | NAPOT Non-Executable Page Execution | N=1 NAPOT page with only RW (no X); S-mode jumps and executes | instruction page-fault |
| PERM-05 | NAPOT U-bit Access Control (S-mode) | N=1 NAPOT page with U=1; S-mode access (sstatus.SUM=0) | page-fault |
| PERM-06 | NAPOT U-bit Access Control (SUM=1) | N=1 NAPOT page with U=1; S-mode read/write (sstatus.SUM=1) | Read and write succeed |
| PERM-07 | NAPOT Permissions Consistent within Region | Verify permissions at different offsets within a 64 KiB NAPOT region | Permission behavior is consistent at all offsets |

---

#### Group 5: N=0 Behavior Verification

**Spec Reference**:
- `norm:Svnapot_pte_N`: N=1 enables NAPOT semantics; when N=0, the PTE behaves identically to a standard non-NAPOT PTE (i.e., standard 4 KiB page translation)
- PTE bit 63 is the N bit (when the Svnapot extension is implemented)

**Test Scope**: Verify that PTE behavior with N=0 is fully consistent with that of a regular PTE.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| NZERO-01 | N=0 Standard 4 KiB Page | Set up a regular 4 KiB PTE with N=0; S-mode reads and writes | Normal 4 KiB page translation |
| NZERO-02 | N=0 with ppn[0]=1000 Has No NAPOT Effect | N=0 but the low-bit encoding of ppn[0] happens to be `1000`; verify no NAPOT semantics are triggered | Standard 4 KiB translation (ppn not substituted) |
| NZERO-03 | N=0: Low Bits of ppn[0] Not Subject to NAPOT Encoding Check | With N=0, ppn[0] low bits are `0001`, which is a standard ppn encoding (representing a physical page frame number); no NAPOT-related checks are triggered | Normal access (no NAPOT encoding check when N=0) |

---

#### Group 6: SFENCE.VMA and NAPOT Page Interaction

**Spec Reference**:
- `norm:Svnapot_cache_entries`: Implicit reads of NAPOT PTEs may create address-translation cache entries mapping a + j×PTESIZE to the substituted pte copy
- Specification note: When updating a NAPOT PTE, the OS should first invalidate all PTEs, then execute SFENCE.VMA for all 4 KiB regions within the range, before updating the PTE
- SFENCE.VMA may use a global flush with rs1=x0, or flush each 4 KiB region individually

**Test Scope**: Verify the TLB cache flush effect of SFENCE.VMA on NAPOT regions.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SFENCE-01 | Global SFENCE.VMA Flushes NAPOT Cache | After modifying NAPOT PTE permissions, execute sfence.vma(0,0) and verify that new permissions take effect | New permissions take effect |
| SFENCE-02a | Single Global SFENCE.VMA Flushes NAPOT Region | After modifying a NAPOT PTE, execute a single sfence.vma with rs1=x0 (global flush); verify new permissions take effect within the 64 KiB region | New permissions take effect |
| SFENCE-02b | Multiple Address-Specific SFENCE.VMA Flushes NAPOT Region Page-by-Page | After modifying a NAPOT PTE, execute sfence.vma with rs1!=x0 for each 4 KiB page within the region (16 instructions total); verify new permissions take effect | New permissions take effect |
| SFENCE-03 | NAPOT Mapping Removal Followed by Flush | Set the NAPOT PTE to invalid (V=0); after sfence.vma, access the region | page-fault |
| SFENCE-04 | NAPOT Permission Upgrade Followed by Flush | Upgrade a NAPOT R-only mapping to RW; after sfence.vma, write to the region | Write succeeds |
| SFENCE-05 | Single-Address SFENCE.VMA Flushes Only the Corresponding 4 KiB | Flush a single 4 KiB address within a 64 KiB NAPOT region; verify that new permissions take effect at that address | At least that 4 KiB address has new permissions in effect |

---

#### Group 7: Sv Mode Compatibility

**Spec Reference**:
- `norm:Svnapot_depends_Sv39`: The Svnapot extension depends on the Sv39 extension
- Specification description: Svnapot applies in Sv39, Sv48, and Sv57

**Test Scope**: Verify the behavioral consistency of NAPOT pages across different Sv modes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| MODE-01 | 64 KiB NAPOT under Sv39 | Configure a 64 KiB NAPOT region in Sv39 mode and access it | Translation is correct |
| MODE-02 | 64 KiB NAPOT under Sv48 | Configure a 64 KiB NAPOT region in Sv48 mode and access it | Translation is correct |
| MODE-03 | 64 KiB NAPOT under Sv57 | Configure a 64 KiB NAPOT region in Sv57 mode and access it | Translation is correct |
| MODE-04 | Sv39 to Sv48 Switch with NAPOT Preservation | Switch from Sv39 to Sv48; rebuild page tables containing NAPOT mappings and verify | NAPOT translation is correct after the switch |
| MODE-05 | Sv39 Dependency Verification | Verify that the Svnapot implementation must support Sv39 | Sv39 mode is available |

---

#### Group 8: A/D Bits and NAPOT Pages

**Spec Reference**:
- Specification note: Implementations may not directly query the PTE specified by the algorithm, so the D and A bits of different mappings within a NAPOT region may be inconsistent
- The OS must query all NAPOT aliases to determine whether a page has been accessed or is dirty
- It is recommended that when the OS sets the A/D bits for a page, it also sets the A/D bits of other NAPOT aliases

**Test Scope**: Verify the basic behavior of A/D bits on NAPOT pages.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| AD-01 | NAPOT A=0 Triggers page-fault (Svade) | 64 KiB NAPOT PTE with A=0; S-mode reads | load page-fault (under Svade semantics) |
| AD-02 | NAPOT D=0 Write Triggers page-fault (Svade) | 64 KiB NAPOT PTE with D=0 (A=1); S-mode writes | store page-fault (under Svade semantics) |
| AD-03 | NAPOT A=1, D=1 Normal Access | 64 KiB NAPOT PTE with A=1, D=1; S-mode reads and writes | Normal access |
| AD-04 | A/D Bit Consistency within NAPOT Region | Set NAPOT PTE A=1, D=1; access multiple 4 KiB offsets within the region | All offsets are accessed normally |
| AD-05 | NAPOT Alias A-bit Independence | Configure two NAPOT PTE aliases covering the same 64 KiB region, one with A=1 and the other with A=0; access the region | The implementation may use either alias -- access behavior depends on the implementation's choice |
| AD-06 | NAPOT Alias D-bit Independence | Similar to AD-05, but tests the D bit: one alias with D=1 and the other with D=0; write to the region | The implementation may use either alias -- write behavior depends on the implementation's choice |
| AD-07 | OS Manually Sets A-bit to Avoid Trap | Manually set A=1 on one NAPOT alias while other aliases have A=0; access and verify trap behavior | If the implementation chooses an A=0 alias, a trap occurs; if it chooses an A=1 alias, no trap occurs |

---

#### Group 9: NAPOT PTE and TLB Cache Behavior

**Spec Reference**:
- `norm:Svnapot_cache_entries`: Implicit reads of NAPOT PTEs may create address-translation cache entries mapping all j within the range such that j >> napot_bits = vpn[i] >> napot_bits
- Specification note: The TLB is allowed to cache V=0 NAPOT PTEs
- Specification note: A 64 KiB NAPOT PTE may trigger the creation of 16 standard 4 KiB TLB entries

**Test Scope**: Verify the TLB caching behavior and multi-alias access of NAPOT PTEs.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TLB-01 | Entire Region Accessible After Single PTE Walk | After accessing the base address within a 64 KiB NAPOT region, verify that other offsets are also accessible | All 16 4 KiB pages are accessible |
| TLB-02 | NAPOT Region Replaced by Regular PTE Followed by Flush | Replace a NAPOT PTE with a regular 4 KiB PTE; flush the TLB and verify | Only the replaced 4 KiB page is accessible |
| TLB-03 | NAPOT V=0 Caching Allowed | Set a NAPOT PTE with V=0; access should trigger a page-fault | page-fault |
| TLB-04 | V=0 NAPOT PTE TLB Cache Consistency | After setting a NAPOT PTE to V=0 and accessing it (triggering a fault), modify the PTE to V=1 without executing SFENCE.VMA, then access directly | The implementation may use the cached V=0 translation (still faulting) or re-query the page table (succeeding) -- verify no unpredictable behavior occurs |

---

#### ~~Group 10: Hypervisor Extension (G-stage)~~ [Migrated]

> **Note**: Please refer to `DOCS/testplan/Hypervisor_cross_test_plan.md` Group 6 (Hypervisor x Svnapot cross tests). Check that document for the latest content.

---

#### Group 11: PTE bits 5-0 Consistency and NAPOT Aliases

**Spec Reference**:
- `norm:Svnapot_pte_N`: NAPOT PTE "represents a translation that is part of a range of contiguous virtual-to-physical translations with the same values for PTE bits 5-0"
- Specification note: All NAPOT PTEs within the same NAPOT region should have the same attributes, the same PPN, and the same bits 5-0 values
- Specification note: If any inconsistencies exist, the effect is the same as when SFENCE.VMA is used incorrectly: one of the translations will be chosen, but the choice is unpredictable

**Test Scope**: Verify the bits 5-0 consistency behavior of multiple alias PTEs within the same NAPOT region, as well as the global translation semantics of the G bit.

> **Note**: BITS50-01 and BITS50-02 are "OS responsibility" tests. The specification states "if any inconsistencies do exist, one of the translations will be chosen, but the choice is unpredictable"; these tests primarily verify that the implementation does not produce unrecoverable errors.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| BITS50-01 | NAPOT Alias R-bit Inconsistency | Configure two NAPOT PTE aliases for a 64 KiB region, one with R=1 and the other with R=0 | Implementation behavior is deterministic (chooses one of the translations, unpredictable but does not crash) |
| BITS50-02 | NAPOT Alias V-bit Inconsistency | One NAPOT PTE with V=1, another alias in the same region with V=0 | Implementation behavior is deterministic (unrecoverable errors should not occur) |
| BITS50-03 | NAPOT G-bit Basic Functionality | 64 KiB NAPOT PTE with G=1; verify global translation semantics | Translation is unaffected by ASID (sfence.vma with ASID-specific flush does not affect G=1 entries) |

---

#### Group 12: RSW Bit Verification

**Spec Reference**:
- Specification note: RSW remains reserved for supervisor software control
- PTE bits 9-8 are RSW (Reserved for Supervisor Software); hardware should ignore this field, and it should not affect address translation behavior

**Test Scope**: Verify that the RSW bits of a NAPOT PTE can be freely written without affecting translation.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| RSW-01 | NAPOT PTE RSW Bits Writable | Write non-zero values (01, 10, 11) to the RSW bits of a NAPOT PTE; verify that address translation is unaffected | Address translation is normal; no fault is triggered |
| RSW-02 | NAPOT PTE RSW Bits Read Back | After writing the RSW bits, read the PTE via a page table walk; verify that the RSW value is preserved | RSW value matches the written value |

---

#### Group 13: NAPOT Region Boundary Alignment Verification

**Spec Reference**:
- `norm:Svnapot_range_napot`: Contiguous ranges must be of naturally aligned power-of-2 granularity, larger than the base page size
- `norm:Svnapot_implicit_read_ppn_subst`: ppn[i][napot_bits-1:0] is replaced by vpn[i][napot_bits-1:0]

**Test Scope**: Verify the translation behavior of NAPOT PTEs in non-64 KiB-aligned scenarios.

> **Note**: Because the NAPOT PPN substitution semantics replace ppn[0][3:0] with vpn[0][3:0], if the VA is not 64 KiB aligned, the low bits of the PA in the translation result will be overwritten by the low bits of the VA. This is not a fault condition, but the translation result needs to be verified.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| ALIGN-01 | NAPOT Translation with Non-64 KiB-Aligned VA | The NAPOT PTE has ppn[0] encoded as `1000`, but the VA is not 64 KiB aligned (e.g., VA offset 0x1000); verify the translation result | In the translation result, the low bits of the PA are overwritten by the low bits of the VA (PPN substitution semantics) |
| ALIGN-02 | NAPOT Translation with Non-64 KiB-Aligned PA | The PA field of the NAPOT PTE is not 64 KiB aligned; the low 4 bits of ppn[0] are still encoded as `1000`; verify the translation result | In the translation result, the low bits of the PA are determined by the low bits of vpn[0] (low bits of ppn are substituted) |

---

#### Group 14: PBMT and NAPOT Interaction Tests (Conditional)

**Spec Reference**:
- PTE bits 62-61 are PBMT (Page-Based Memory Types), defined by the Svpbmt extension
- If the Svpbmt extension is implemented, NAPOT PTEs should also support PBMT attributes
- If Svpbmt is not implemented, the PBMT bits should remain 0

**Test Scope**: Verify the interaction between NAPOT PTEs and the Svpbmt extension (only applicable when Svpbmt is implemented).

> **Note**: The following tests are executed only when the target platform implements the Svpbmt extension. If Svpbmt is not implemented, these tests should be skipped.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| PBMT-01 | NAPOT + PBMT PMA Mapping | 64 KiB NAPOT PTE with PBMT=PMA (00); normal access | Normal access |
| PBMT-02 | NAPOT + PBMT NC Mapping | 64 KiB NAPOT PTE with PBMT=NC (01); verify non-cacheable access | Normal access, non-cacheable semantics |
| PBMT-03 | NAPOT + PBMT IO Mapping | 64 KiB NAPOT PTE with PBMT=IO (10); verify I/O strongly ordered access | Normal access, strongly ordered semantics |

---

## Test Priority

| Priority | Test Groups | Covered Test IDs | Rationale |
|----------|-------------|------------------|-----------|
| P0 (Required) | Group 1 (Basic Mapping), Group 3 (Reserved Encoding Exceptions), Group 4 (Permission Control) | NAPOT64-01~06, RSVD-01~09, PERM-01~07 | Core functionality: NAPOT translation, reserved encoding faults, permission correctness |
| P1 (Important) | Group 2 (PPN Substitution), Group 5 (N=0 Behavior), Group 6 (SFENCE.VMA), Group 12 (RSW Bits) | PPN-01~05, NZERO-01~03, SFENCE-01~05, RSW-01~02, AD-05~07 | PPN substitution semantics correctness, N=0 compatibility, TLB management, RSW bit basic assurance, A/D alias independence |
| P2 (Recommended) | Group 7 (Multi-Mode Compatibility), Group 8 (A/D Bits), Group 9 (TLB Cache), Group 11 (bits 5-0 Consistency), Group 13 (Boundary Alignment) | MODE-01~05, AD-01~04, TLB-01~04, BITS50-01~03, ALIGN-01~02 | Cross-mode verification, advanced behavior, alias consistency, alignment boundaries |
| P3 (Optional) | Group 14 (PBMT Interaction) | PBMT-01~03 | Depends on the Svpbmt extension; conditional implementation |

---

## Test Implementation Notes

### NAPOT PTE Encoding

In the Svnapot extension, PTE bit 63 is the N bit. When N=1, the encoding of ppn[0] determines the NAPOT region size. Version 1.0 standardizes only 64 KiB support:

The low 4 bits of ppn[0] for a 64 KiB NAPOT PTE are encoded as `1000` (i.e., bit3=1, bits[2:0]=0), meaning:
- The PTE covers 16 contiguous 4 KiB pages (16 x 4 KiB = 64 KiB)
- The base address must be 64 KiB aligned
- On implicit reads, ppn[0][3:0] is replaced by vpn[0][3:0]

The test code should define the following constants and helper functions:

### S-mode Helper Functions

S-mode helper functions used in the test plan:

| Function | Description | Return Value |
|----------|-------------|--------------|
| `test_smode_load` | Execute a load operation | 0=success |
| `test_smode_store` | Execute a store operation | 0=success |
| `test_smode_read_write` | Write a magic value and read it back for verification | 0=success, non-0=failure |
| `test_smode_load_expect_fault` | Execute a load, expecting a fault | fault cause |
| `test_smode_store_expect_fault` | Execute a store, expecting a fault | fault cause |
| `test_smode_exec_expect_fault` | Jump and execute, expecting a fault | fault cause |
| `test_smode_ppn_subst_verify` | Write different magic values to all 16 4 KiB pages within a 64 KiB region and read them back for verification | 0=all correct |
| `test_smode_access_all_16_pages` | Traverse and access all 16 4 KiB pages within a 64 KiB region | 0=all accessible |

### File Organization

Svnapot tests depend on the virtual memory test framework. The recommended file organization is as follows:

### Makefile Configuration

### Common Test Pattern

Svnapot test cases follow the pattern below:

### Key Considerations

1. **64 KiB Alignment**: The base addresses (VA and PA) of a NAPOT 64 KiB region must be 64 KiB (0x10000) aligned. `TEST_REGION_BASE` should be defined as an address that satisfies this alignment requirement.

2. **Manual PTE Construction**: Since the existing `pt_map_page()` API does not support NAPOT PTEs, the `napot_install_pte()` helper function is needed to write directly to the page table. This function must walk the page table to level 0, locate the corresponding PTE slot, and write the pre-constructed NAPOT PTE value.

3. **PTE_A and PTE_D Bits**: Except for the tests in Group 8 (A/D bit management), all other tests should always set `PTE_A | PTE_D` to avoid unnecessary page-faults caused by Svade semantics.

4. **Page-fault Cause Codes**:
   - `CAUSE_IPF` (12): Instruction page-fault
   - `CAUSE_LPF` (13): Load page-fault
   - `CAUSE_SPF` (15): Store/AMO page-fault

5. **QEMU Support**: QEMU 7.0+ supports the Svnapot extension. When launching, enable `svnapot=true` in the `-cpu` parameter, for example:

6. **NAPOT PTE Does Not Conflict with Regular PTEs**: Within the same page table, a NAPOT PTE occupies a single PTE slot at level 0 (corresponding to the first 4 KiB page address of the 64 KiB region). The PTE slots for the remaining 15 4 KiB pages within the region should be set to invalid (V=0) or maintain a consistent NAPOT encoding. Tests should be careful not to set conflicting mappings for these slots.

7. **PMP Configuration**: `vm_run_in_smode()` automatically configures PMP entry 15 as all-address-space NAPOT RWX. If testing the interaction between PMP and Svnapot, the PMP configuration must be managed manually.

8. **Hypervisor Extension**: G-stage related tests (formerly Group 10, GSTG-01~03) have been migrated to `DOCS/testplan/Hypervisor_cross_test_plan.md` Group 6.

---

## Specification Coverage Matrix

| Specification Label | Covered Test IDs |
|---------------------|------------------|
| `norm:Svnapot_pte_N` | NAPOT64-01~06, NZERO-01~03, PERM-01~07, BITS50-01~03 |
| `norm:Svnapot_range_napot` | NAPOT64-01~06, PPN-01~05, ALIGN-01~02 |
| `norm:Svnapot_depends_Sv39` | MODE-01, MODE-05 |
| `norm:Svnapot_valid_encoding` | NAPOT64-01~06, PPN-01~05, PERM-01~07 |
| `norm:Svnapot_implicit_read_ppn_subst` | PPN-01~05, ALIGN-01~02 |
| `norm:Svnapot_reserved_encoding_fault` | RSVD-01~09 |
| `norm:Svnapot_cache_entries` | TLB-01~04, SFENCE-01~05 |
| `norm:Svnapot_hyp_gstage` | GSTG-01~03 (migrated to `Hypervisor_cross_test_plan.md` Group 6) |
| RSW Bits (Note reference) | RSW-01~02 |
| A/D Alias Independence (Note reference) | AD-05~07 |
| PBMT Interaction (Svpbmt dependency) | PBMT-01~03 (Conditional, depends on Svpbmt extension) |

---

## References

- RISC-V Privileged Specification -- Svnapot Extension (`SPEC/svnapot.adoc`)
- `SPEC/supervisor.adoc` -- Supervisor-Level ISA (SFENCE.VMA definition, satp CSR definition)
- `docs/vm_test_framework.md` -- Virtual Memory Test Framework Documentation
- `docs/vm_test_plan.md` -- Virtual Memory Test Plan (PTE permissions, SFENCE.VMA reference)
- `common/vm/vm.h` -- VM API Declaration
- `common/vm/vm_defs.h` -- Constants and Macro Definitions
