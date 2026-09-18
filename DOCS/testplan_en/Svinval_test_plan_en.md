**[中文](../testplan/Svinval_test_plan.md) | English**

# Svinval Extension Test Plan

This document describes the test plan for the Svinval (Fine-Grained Address-Translation Cache Invalidation) extension. The Svinval extension splits the SFENCE.VMA instruction into finer-grained invalidation and ordering operations (SINVAL.VMA, SFENCE.W.INVAL, SFENCE.INVAL.IR), enabling more efficient batched or pipelined TLB invalidation on high-performance implementations.

---

## SPEC Sections Covered by This Document

This plan is based on the following official RISC-V specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/svinval.adoc` — Svinval extension: SINVAL.VMA / SFENCE.W.INVAL / SFENCE.INVAL.IR semantics, equivalence of the three-instruction sequence to SFENCE.VMA, illegal-instruction / virtual-instruction triggering rules at each privilege level
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — SFENCE.VMA definition (used for sequence-equivalence comparison)

Official repository:

- https://github.com/riscv/riscv-isa-manual (the above path files within the repository)

---

## Covered Specification Points

The table below lists the specification points covered by this plan. Entries with the `norm:` prefix are official SPEC normative rule tags.

| Norm ID | Original Text | Description |
|---------|---------------|-------------|
| `norm:Svinval_split_fine_grained` | ...that can be more efficiently batched or pipelined on certain classes of high-performance implementation. | Can be more efficiently batched or pipelined on certain classes of high-performance implementations. |
| `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma` | However, unlike SFENCE.VMA, SINVAL.VMA instructions are only ordered with respect to SFENCE.VMA, SFENCE.W.INVAL, and SFENCE.INVAL.IR instructions as defined below. | Unlike SFENCE.VMA, SINVAL.VMA instructions are only ordered with respect to SFENCE.VMA, SFENCE.W.INVAL, and SFENCE.INVAL.IR instructions. |
| `norm:Svinval_sfence_w_inval_orders_before_sinval_vma` | The SFENCE.INVAL.IR instruction guarantees that any previous SINVAL.VMA instructions executed by the current hart are ordered before subsequent implicit references by that hart to the memory-management data structures. | SFENCE.INVAL.IR guarantees that any previous SINVAL.VMA instructions executed by the current hart are ordered before that hart's subsequent implicit references to the memory-management data structures. |
| `norm:Svinval_sequence_rs1_rs2` | the values of rs1 and rs2 for the SFENCE.VMA are the same as those used in the SINVAL.VMA. | The rs1 and rs2 values of the SFENCE.VMA are the same as those used in the SINVAL.VMA. |
| `norm:Svinval_sequence_reads_writes_before` | reads and writes prior to the SFENCE.W.INVAL are considered to be those prior to the SFENCE.VMA. | Reads and writes prior to the SFENCE.W.INVAL are considered to be those prior to the SFENCE.VMA. |
| `norm:Svinval_sequence_reads_writes_after` | reads and writes following the SFENCE.INVAL.IR are considered to be those subsequent to the SFENCE.VMA. | Reads and writes following the SFENCE.INVAL.IR are considered to be those subsequent to the SFENCE.VMA. |
| `norm:Svinval_hinval_vvma_gvma` | These have the same semantics as SINVAL.VMA, except that they combine with SFENCE.W.INVAL and SFENCE.INVAL.IR to replace HFENCE.VVMA and HFENCE.GVMA, respectively. | HINVAL.VVMA/GVMA have the same semantics as SINVAL.VMA, combining with SFENCE.W.INVAL/SFENCE.INVAL.IR to replace HFENCE.VVMA/HFENCE.GVMA respectively. |
| `norm:Svinval_hinval_gvma_uses_vmid` | HINVAL.GVMA uses VMIDs instead of ASIDs. | HINVAL.GVMA uses VMIDs instead of ASIDs. |
| `norm:Svinval_illegal_instruction_u_mode` | In particular, an attempt to execute any of these instructions in U-mode always raises an illegal-instruction exception. | An attempt to execute any of these instructions in U-mode always raises an illegal-instruction exception. |
| `norm:Svinval_illegal_instruction_tvm` | An attempt to execute SINVAL.VMA or HINVAL.GVMA in S-mode or HS-mode when `mstatus`.TVM=1 also raises an illegal-instruction exception. | When `mstatus`.TVM=1, an attempt to execute SINVAL.VMA or HINVAL.GVMA in S-mode or HS-mode also raises an illegal-instruction exception. |
| `norm:Svinval_virtual_instruction_vu_vs` | An attempt to execute HINVAL.VVMA or HINVAL.GVMA in VS-mode or VU-mode, or to execute SINVAL.VMA in VU-mode, raises a virtual-instruction exception. | Executing HINVAL.VVMA/GVMA in VS-mode/VU-mode, or executing SINVAL.VMA in VU-mode, raises a virtual-instruction exception. |
| `norm:Svinval_virtual_instruction_vtvms` | When `hstatus`.VTVM=1, an attempt to execute SINVAL.VMA in VS-mode also raises a virtual-instruction exception. | When `hstatus`.VTVM=1, executing SINVAL.VMA in VS-mode also raises a virtual-instruction exception. |
| `norm:Svinval_sfence_w_inval_inval_u_mode` | Attempting to execute SFENCE.W.INVAL or SFENCE.INVAL.IR in U-mode raises an illegal-instruction exception. | Executing SFENCE.W.INVAL or SFENCE.INVAL.IR in U-mode raises an illegal-instruction exception. |
| `norm:Svinval_sfence_w_inval_inval_vu_mode` | Doing so in VU-mode raises a virtual-instruction exception. | Executing SFENCE.W.INVAL or SFENCE.INVAL.IR in VU-mode raises a virtual-instruction exception. |
| `norm:Svinval_sfence_w_inval_inval_s_vs_mode` | SFENCE.W.INVAL and SFENCE.INVAL.IR are unaffected by the `mstatus`.TVM and `hstatus`.VTVM fields and hence are always permitted in S-mode and VS-mode. | SFENCE.W.INVAL and SFENCE.INVAL.IR are unaffected by `mstatus`.TVM and `hstatus`.VTVM, and are always permitted in S-mode and VS-mode. |

---

## Out of Scope

- **HINVAL.VVMA / HINVAL.GVMA instructions**: These require Hypervisor extension support; their functionality, VMID semantics, and VS/VU-mode virtual-instruction triggering are covered by `Hypervisor_Sv_test_plan.md` (Hypervisor x Svinval cross tests). This plan only covers SINVAL.VMA / SFENCE.W.INVAL / SFENCE.INVAL.IR in non-virtualized scenarios.
- **TLB coherence in multi-core scenarios**: This plan focuses on single-hart behavior.

---

## Test Groups

### Group 1: SINVAL.VMA Basic Functionality

**Specification basis**:
- `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma`: SINVAL.VMA invalidates the same address-translation cache entries as SFENCE.VMA
- `norm:Svinval_sequence_rs1_rs2`: The rs1/rs2 of SINVAL.VMA in the three-instruction sequence are equivalent to the rs1/rs2 of the hypothetical SFENCE.VMA

**Test responsibilities**: Verify that the SFENCE.W.INVAL + SINVAL.VMA + SFENCE.INVAL.IR sequence is functionally equivalent to SFENCE.VMA -- i.e., after modifying page tables, flushing the TLB via this sequence causes subsequent accesses to use the new translation.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SINVAL-01 | Sequence flush after permission upgrade | Upgrade an R-only page to RW, execute SFENCE.W.INVAL + SINVAL.VMA(va, asid=0) + SFENCE.INVAL.IR, then write | Write succeeds |
| SINVAL-02 | Sequence flush after mapping invalidation | Change a valid mapping to invalid (V=0), execute the three-instruction sequence, then access | page-fault |
| SINVAL-03 | Sequence flush after permission downgrade | Downgrade an RWX page to R-only, execute the three-instruction sequence, then write | store page-fault |
| SINVAL-04 | Sequence flush after physical address remapping | Modify a PTE to point to a different physical page, execute the three-instruction sequence, then read | Data from the new physical page is read |
| SINVAL-05 | Full-address flush with rs1=x0 | Modify PTEs of multiple pages, execute SINVAL.VMA(rs1=x0, rs2=x0) for a global flush | New permissions take effect on all modified pages |
| SINVAL-06 | 2M megapage SINVAL.VMA invalidation | Modify the PTE permissions of a 2M megapage, then execute the three-instruction sequence | New translation takes effect across the entire 2M region |
| SINVAL-07 | 1G gigapage SINVAL.VMA invalidation | Modify the PTE permissions of a 1G gigapage, then execute the three-instruction sequence | New translation takes effect across the entire 1G region |

---

### Group 2: Ordering Semantics Verification

**Specification basis**:
- `norm:Svinval_sfence_w_inval_orders_before_sinval_vma`: SFENCE.W.INVAL guarantees that prior stores are ordered before subsequent SINVAL.VMA; SFENCE.INVAL.IR guarantees that prior SINVAL.VMA instructions are ordered before subsequent implicit references to memory-management data structures
- `norm:Svinval_sequence_reads_writes_before`: Reads and writes prior to SFENCE.W.INVAL are equivalent to reads and writes prior to SFENCE.VMA
- `norm:Svinval_sequence_reads_writes_after`: Reads and writes following SFENCE.INVAL.IR are equivalent to reads and writes following SFENCE.VMA

**Test responsibilities**: Verify the ordering guarantees of the three-instruction sequence -- stores before SFENCE.W.INVAL (page table modifications) are visible to SINVAL.VMA, and accesses after SFENCE.INVAL.IR use the updated translation.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| ORDER-01 | store -> W.INVAL -> SINVAL -> INVAL.IR -> load | Modify a PTE (store), execute the complete three-instruction sequence, then load to verify the new translation is in effect | Load uses the new translation |
| ORDER-02 | Sequence without SFENCE.W.INVAL | After modifying a PTE, execute only SINVAL.VMA + SFENCE.INVAL.IR (omitting SFENCE.W.INVAL) | Behavior undefined (may use the old translation) |
| ORDER-03 | Sequence without SFENCE.INVAL.IR | After modifying a PTE, execute SFENCE.W.INVAL + SINVAL.VMA (omitting SFENCE.INVAL.IR) | Behavior undefined (may use the old translation) |
| ORDER-04 | Full sequence equivalence with SFENCE.VMA | Flush the same page using SFENCE.VMA and the three-instruction sequence separately, verify the results are identical | Both methods produce the same result |
| ORDER-05 | SFENCE.VMA as ordering fence for SINVAL.VMA | After modifying a PTE, execute SINVAL.VMA, then SFENCE.VMA (substituting for SFENCE.INVAL.IR), then access the page | SFENCE.VMA guarantees SINVAL.VMA takes effect; the new translation is in effect |
| ORDER-06 | Multiple SINVAL.VMA followed by SFENCE.VMA | Modify PTEs of multiple pages, execute multiple SINVAL.VMA in batch, and use SFENCE.VMA as the final fence instead of SFENCE.INVAL.IR | New translations take effect on all pages |

> [!NOTE]
> ORDER-02 and ORDER-03 test the behavior of incomplete sequences. The specification does not guarantee the correctness of incomplete sequences, but it explicitly permits simpler implementations to implement SINVAL.VMA identically to SFENCE.VMA and to implement SFENCE.W.INVAL and SFENCE.INVAL.IR as NOPs, so incomplete sequences may still work correctly on such implementations. These two cases are intended solely to probe implementation behavior and are not used as compliance criteria.

---

### Group 3: Batch Invalidation Operations

**Specification basis**:
- `norm:Svinval_split_fine_grained`: The design goal of Svinval is to support batched or pipelined TLB invalidation operations
- `norm:Svinval_sfence_w_inval_orders_before_sinval_vma`: Multiple SINVAL.VMA instructions can be batched between SFENCE.W.INVAL and SFENCE.INVAL.IR

**Test responsibilities**: Verify the correctness of executing multiple SINVAL.VMA instructions between SFENCE.W.INVAL and SFENCE.INVAL.IR, which is the core usage scenario of the Svinval extension.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| BATCH-01 | Batch invalidation of multiple pages | Modify PTEs of 3 pages, execute 3 SINVAL.VMA between W.INVAL and INVAL.IR | New permissions take effect on all 3 pages |
| BATCH-02 | Batch invalidation with different permission changes | Page A upgraded R->RW, page B downgraded RW->R, page C invalidated V->0, batch flush | A is writable, B write triggers a fault, C access triggers a fault |
| BATCH-03 | Batch invalidation of a large number of pages | Modify PTEs of 16 consecutive pages, execute 16 SINVAL.VMA in batch | New permissions take effect on all 16 pages |
| BATCH-04 | Batch invalidation with mixed rs1 arguments | Some SINVAL.VMA specify a concrete address, others use rs1=x0 | All specified pages are flushed correctly |

---

### Group 4: rs1/rs2 Parameter Combinations

**Specification basis**:
- `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma`: SINVAL.VMA uses the same rs1/rs2 semantics as SFENCE.VMA
- `norm:Svinval_sequence_rs1_rs2`: The rs1/rs2 of SINVAL.VMA in the three-instruction sequence are equivalent to the rs1/rs2 of the hypothetical SFENCE.VMA

**Test responsibilities**: Verify different combinations of the rs1 (virtual address) and rs2 (ASID) parameters of SINVAL.VMA, ensuring consistency with SFENCE.VMA parameter semantics.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| PARAM-01 | rs1=addr, rs2=x0 | Specify an address, ASID=0 (all ASIDs), modify the PTE at that address then flush | New translation takes effect at that address |
| PARAM-02 | rs1=x0, rs2=x0 | Global flush (all addresses, all ASIDs), modify multiple PTEs then flush | New translations take effect on all modified pages |
| PARAM-03 | rs1=addr, rs2=asid | Specify an address and ASID, modify the PTE at that address then flush | New translation takes effect at that address |
| PARAM-04 | rs1=x0, rs2=asid | All addresses for a specified ASID, modify multiple PTEs then flush | New translations take effect on all modified pages under that ASID |
| PARAM-05 | Flush with non-matching address | Modify the PTE of page A, but SINVAL.VMA specifies the address of page B | Page A may still use the old translation (behavior undefined) |

---

### Group 5: SINVAL.VMA Privilege Exception Checks

**Specification basis**:
- `norm:Svinval_illegal_instruction_u_mode`: Executing SINVAL.VMA in U-mode always raises an illegal-instruction exception
- `norm:Svinval_illegal_instruction_tvm`: When mstatus.TVM=1, executing SINVAL.VMA in S-mode raises an illegal-instruction exception

**Test responsibilities**: Verify the exception behavior of the SINVAL.VMA instruction under different privilege levels and CSR configurations.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| PRIV-01 | SINVAL.VMA in U-mode | Execute SINVAL.VMA in U-mode | illegal-instruction exception (cause=2) |
| PRIV-02 | SINVAL.VMA in S-mode with TVM=0 | Execute SINVAL.VMA in S-mode when mstatus.TVM=0 | Executes normally, no exception |
| PRIV-03 | SINVAL.VMA in S-mode with TVM=1 | Execute SINVAL.VMA in S-mode when mstatus.TVM=1 | illegal-instruction exception (cause=2) |
| PRIV-04 | SINVAL.VMA in M-mode | Execute SINVAL.VMA in M-mode | Executes normally, no exception |

---

### Group 6: SFENCE.W.INVAL / SFENCE.INVAL.IR Privilege Behavior

**Specification basis**:
- `norm:Svinval_sfence_w_inval_inval_u_mode`: Executing SFENCE.W.INVAL or SFENCE.INVAL.IR in U-mode raises an illegal-instruction exception
- `norm:Svinval_sfence_w_inval_inval_s_vs_mode`: SFENCE.W.INVAL and SFENCE.INVAL.IR are unaffected by mstatus.TVM and hstatus.VTVM; they are always permitted in S-mode and VS-mode

**Test responsibilities**: Verify the privilege-level access control of the SFENCE.W.INVAL and SFENCE.INVAL.IR instructions, in particular that they are unaffected by the TVM bit.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| FENCE-01 | SFENCE.W.INVAL in U-mode | Execute SFENCE.W.INVAL in U-mode | illegal-instruction exception (cause=2) |
| FENCE-02 | SFENCE.INVAL.IR in U-mode | Execute SFENCE.INVAL.IR in U-mode | illegal-instruction exception (cause=2) |
| FENCE-03 | SFENCE.W.INVAL in S-mode with TVM=0 | Execute SFENCE.W.INVAL in S-mode when mstatus.TVM=0 | Executes normally, no exception |
| FENCE-04 | SFENCE.INVAL.IR in S-mode with TVM=0 | Execute SFENCE.INVAL.IR in S-mode when mstatus.TVM=0 | Executes normally, no exception |
| FENCE-05 | SFENCE.W.INVAL in S-mode with TVM=1 | Execute SFENCE.W.INVAL in S-mode when mstatus.TVM=1 | Executes normally, no exception (unaffected by TVM) |
| FENCE-06 | SFENCE.INVAL.IR in S-mode with TVM=1 | Execute SFENCE.INVAL.IR in S-mode when mstatus.TVM=1 | Executes normally, no exception (unaffected by TVM) |
| FENCE-07 | SFENCE.W.INVAL in M-mode | Execute SFENCE.W.INVAL in M-mode | Executes normally, no exception |
| FENCE-08 | SFENCE.INVAL.IR in M-mode | Execute SFENCE.INVAL.IR in M-mode | Executes normally, no exception |

---

### Group 7: Instruction Encoding Verification

**Specification basis**:
- The Svinval extension introduces 3 new instructions (SINVAL.VMA, SFENCE.W.INVAL, SFENCE.INVAL.IR); it must be verified that the instruction encodings are correctly recognized by hardware

**Test responsibilities**: Verify that Svinval instructions can execute normally in M-mode without raising an illegal-instruction exception. This is a prerequisite for all other functional tests -- if the instruction encodings themselves are incorrect or the platform does not support the Svinval extension, all functional tests will fail to run correctly.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| ENC-01 | SINVAL.VMA instruction encoding verification | Execute SINVAL.VMA in M-mode, confirm no illegal-instruction is raised | Executes normally, no exception |
| ENC-02 | SFENCE.W.INVAL instruction encoding verification | Execute SFENCE.W.INVAL in M-mode, confirm no illegal-instruction is raised | Executes normally, no exception |
| ENC-03 | SFENCE.INVAL.IR instruction encoding verification | Execute SFENCE.INVAL.IR in M-mode, confirm no illegal-instruction is raised | Executes normally, no exception |

---

## Test Priority

| Priority | Test Group | Covered Test IDs | Rationale |
|----------|------------|------------------|-----------|
| P0 (Required) | Group 7 (encoding verification), Group 1 (SINVAL.VMA basic functionality), Group 5 (privilege exceptions) | ENC-01~03, SINVAL-01~07, PRIV-01~04 | Instruction encoding baseline verification, core functionality verification, and security guarantees |
| P1 (Important) | Group 2 (ordering semantics), Group 6 (SFENCE.W.INVAL/INVAL.IR privilege) | ORDER-01~06, FENCE-01~08 | Ordering correctness and privilege-level access control |
| P2 (Recommended) | Group 3 (batch invalidation), Group 4 (rs1/rs2 parameter combinations) | BATCH-01~04, PARAM-01~05 | Batch operations and parameter coverage |

---

## Result Determination Principles

- If the platform implements Svinval but its behavior deviates from the SPEC (e.g., the three-instruction sequence is not equivalent to SFENCE.VMA, no illegal-instruction is raised under U-mode/TVM=1, SFENCE.W.INVAL/SFENCE.INVAL.IR are affected by TVM, etc.): keep the test case failing, compare against the SPEC, and record the issue in the `bugs/` directory. Modifying the test case or adding a workaround to accommodate an incorrect implementation is prohibited.
- "Behavior undefined" cases such as ORDER-02/03 and PARAM-05 only probe implementation behavior and are not used as compliance criteria.

---

## Appendix: Normative References

### Svinval Instruction Encodings

The table below gives the encodings of the instructions introduced by the Svinval extension (R-type, opcode=SYSTEM=0x73, funct3=000); this is a normative reference:

| Instruction | funct7 (31..25) | rs2 (24..20) | rs1 (19..15) | rd (11..7) | opcode (6..0) |
|-------------|-----------------|--------------|--------------|------------|---------------|
| `sinval.vma rs1, rs2` | 0001011 | rs2 | rs1 | 00000 | 1110011 |
| `sfence.w.inval` | 0001100 | 00000 | 00000 | 00000 | 1110011 |
| `sfence.inval.ir` | 0001100 | 00001 | 00000 | 00000 | 1110011 |
| `hinval.vvma rs1, rs2` | 0011011 | rs2 | rs1 | 00000 | 1110011 |
| `hinval.gvma rs1, rs2` | 0111011 | rs2 | rs1 | 00000 | 1110011 |

> The functionality and exception verification of HINVAL.VVMA / HINVAL.GVMA are covered by `Hypervisor_Sv_test_plan.md`.

### Related scause Constants

| Constant | Value | Description |
|----------|-------|-------------|
| Illegal instruction | 2 | Illegal instruction |
| Instruction page fault | 12 | Instruction page fault |
| Load page fault | 13 | Load page fault |
| Store/AMO page fault | 15 | Store/AMO page fault |
| Virtual instruction | 22 | Virtual instruction (Hypervisor scenario, covered by `Hypervisor_Sv_test_plan.md`) |

---

## References

- `svinval.adoc` — Svinval extension definition
- `supervisor.adoc` — SFENCE.VMA definition
- `Hypervisor_Sv_test_plan.md` — Hypervisor x Svinval cross test plan (HINVAL.VVMA/GVMA)

---

## Appendix A: Specification Point Coverage Matrix

The table below indicates which test cases cover each specification point listed in the "Covered Specification Points" section. Hypervisor-related specification points are covered by `Hypervisor_Sv_test_plan.md`.

| Norm ID | Covered Test IDs |
|---------|------------------|
| `norm:Svinval_split_fine_grained` | BATCH-01~04 |
| `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma` | SINVAL-01~07, PARAM-01~05, ORDER-05~06 |
| `norm:Svinval_sfence_w_inval_orders_before_sinval_vma` | ORDER-01~06 |
| `norm:Svinval_sequence_rs1_rs2` | SINVAL-01~07, PARAM-01~05 |
| `norm:Svinval_sequence_reads_writes_before` | ORDER-01~06 |
| `norm:Svinval_sequence_reads_writes_after` | ORDER-01~06 |
| `norm:Svinval_hinval_vvma_gvma` | Covered by `Hypervisor_Sv_test_plan.md` (HCROSS-SINVAL-01~04) |
| `norm:Svinval_hinval_gvma_uses_vmid` | Covered by `Hypervisor_Sv_test_plan.md` (HCROSS-SINVAL-05~06) |
| `norm:Svinval_illegal_instruction_u_mode` | PRIV-01 |
| `norm:Svinval_illegal_instruction_tvm` | PRIV-03 |
| `norm:Svinval_virtual_instruction_vu_vs` | Covered by `Hypervisor_Sv_test_plan.md` (HCROSS-SINVAL-07~10, HCROSS-SINVAL-15) |
| `norm:Svinval_virtual_instruction_vtvms` | Covered by `Hypervisor_Sv_test_plan.md` (VS-mode VTVM scenario) |
| `norm:Svinval_sfence_w_inval_inval_u_mode` | FENCE-01, FENCE-02 |
| `norm:Svinval_sfence_w_inval_inval_vu_mode` | Covered by `Hypervisor_Sv_test_plan.md` (HCROSS-SINVAL-11~12) |
| `norm:Svinval_sfence_w_inval_inval_s_vs_mode` | FENCE-03~06 |
