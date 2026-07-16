**[中文](../testplan/Svinval_test_plan.md) | English**

# Svinval Extension Test Plan

This document describes the test plan for the Svinval (Fine-Grained Address-Translation Cache Invalidation) extension. The Svinval extension splits the SFENCE.VMA instruction into finer-grained invalidation and ordering operations (SINVAL.VMA, SFENCE.W.INVAL, SFENCE.INVAL.IR), enabling more efficient batched or pipelined TLB invalidation on high-performance implementations.

---

## Test Scope

### Specification Source

- `SPEC/svinval.adoc` -- Svinval Extension for Fine-Grained Address-Translation Cache Invalidation, Version 1.0

### Covered Specification Points

| Norm ID | Original Text |
|---------|------|
| `norm:Svinval_split_fine_grained` | that can be more efficiently batched or pipelined on certain classes of high-performance implementation. |
| `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma` | However, unlike SFENCE.VMA, SINVAL.VMA instructions are only ordered with respect to SFENCE.VMA, SFENCE.W.INVAL, and SFENCE.INVAL.IR instructions as defined below. |
| `norm:Svinval_sfence_w_inval_orders_before_sinval_vma` | The SFENCE.INVAL.IR instruction guarantees that any previous SINVAL.VMA instructions executed by the current hart are ordered before subsequent implicit references by that hart to the memory-management data structures. |
| `norm:Svinval_sequence_rs1_rs2` | the values of _rs1_ and _rs2_ for the SFENCE.VMA are the same as those used in the SINVAL.VMA. |
| `norm:Svinval_sequence_reads_writes_before` | reads and writes prior to the SFENCE.W.INVAL are considered to be those prior to the SFENCE.VMA. |
| `norm:Svinval_sequence_reads_writes_after` | reads and writes following the SFENCE.INVAL.IR are considered to be those subsequent to the SFENCE.VMA. |
| `norm:Svinval_hinval_vvma_gvma` | These have the same semantics as SINVAL.VMA, except that they combine with SFENCE.W.INVAL and SFENCE.INVAL.IR to replace HFENCE.VVMA and HFENCE.GVMA, respectively, instead of SFENCE.VMA. |
| `norm:Svinval_hinval_gvma_uses_vmid` | HINVAL.GVMA uses VMIDs instead of ASIDs. |
| `norm:Svinval_illegal_instruction_u_mode` | In particular, an attempt to execute any of these instructions in U-mode always raises an illegal-instruction exception. |
| `norm:Svinval_illegal_instruction_tvm` | An attempt to execute SINVAL.VMA or HINVAL.GVMA in S-mode or HS-mode when `mstatus`.TVM=1 also raises an illegal-instruction exception. |
| `norm:Svinval_virtual_instruction_vu_vs` | An attempt to execute HINVAL.VVMA or HINVAL.GVMA in VS-mode or VU-mode, or to execute SINVAL.VMA in VU-mode, raises a virtual-instruction exception. |
| `norm:Svinval_virtual_instruction_vtvms` | When `hstatus`.VTVM=1, an attempt to execute SINVAL.VMA in VS-mode also raises a virtual-instruction exception. |
| `norm:Svinval_sfence_w_inval_inval_u_mode` | raises an illegal-instruction exception. |
| `norm:Svinval_sfence_w_inval_inval_vu_mode` | Doing so in VU-mode raises a virtual-instruction exception. |
| `norm:Svinval_sfence_w_inval_inval_s_vs_mode` | SFENCE.W.INVAL and SFENCE.INVAL.IR are unaffected by the `mstatus`.TVM and `hstatus`.VTVM fields and hence are always permitted in S-mode and VS-mode. |

### Out of Scope

- HINVAL.VVMA / HINVAL.GVMA instructions (require Hypervisor extension support; the current test platform does not include the H extension)
- TLB coherence in multi-core scenarios (the current test framework is a single-core environment)

---

## Test Cases

### Group 1: SINVAL.VMA Basic Functionality

**Specification Basis**:
- `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma`: SINVAL.VMA invalidates the same address-translation cache entries as SFENCE.VMA
- `norm:Svinval_sequence_rs1_rs2`: The rs1/rs2 of SINVAL.VMA in the three-instruction sequence are equivalent to the rs1/rs2 of the hypothetical SFENCE.VMA

**Test Responsibility**: Verify that the SFENCE.W.INVAL + SINVAL.VMA + SFENCE.INVAL.IR sequence is functionally equivalent to SFENCE.VMA -- i.e., after modifying page tables, flushing the TLB via this sequence causes subsequent accesses to use the new translation.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| SINVAL-01 | Sequence flush after permission upgrade | Upgrade an R-only page to RW, execute SFENCE.W.INVAL + SINVAL.VMA(va, asid=0) + SFENCE.INVAL.IR, then write | Write succeeds |
| SINVAL-02 | Sequence flush after mapping invalidation | Change a valid mapping to invalid (V=0), execute the three-instruction sequence, then access | page-fault |
| SINVAL-03 | Sequence flush after permission downgrade | Downgrade an RWX page to R-only, execute the three-instruction sequence, then write | store page-fault |
| SINVAL-04 | Sequence flush after physical address remapping | Modify a PTE to point to a different physical page, execute the three-instruction sequence, then read | Data from the new physical page is read |
| SINVAL-05 | Full-address flush with rs1=x0 | Modify PTEs of multiple pages, execute SINVAL.VMA(rs1=x0, rs2=x0) for a global flush | New permissions take effect on all modified pages |
| SINVAL-06 | 2M megapage SINVAL.VMA invalidation | Modify the PTE permissions of a 2M megapage, then execute the three-instruction sequence | New translation takes effect across the entire 2M region |
| SINVAL-07 | 1G gigapage SINVAL.VMA invalidation | Modify the PTE permissions of a 1G gigapage, then execute the three-instruction sequence | New translation takes effect across the entire 1G region |

---

### Group 2: Ordering Semantics Verification

**Specification Basis**:
- `norm:Svinval_sfence_w_inval_orders_before_sinval_vma`: SFENCE.W.INVAL guarantees that prior stores are ordered before subsequent SINVAL.VMA; SFENCE.INVAL.IR guarantees that prior SINVAL.VMA instructions are ordered before subsequent implicit references to memory-management data structures
- `norm:Svinval_sequence_reads_writes_before`: Reads and writes prior to SFENCE.W.INVAL are equivalent to reads and writes prior to SFENCE.VMA
- `norm:Svinval_sequence_reads_writes_after`: Reads and writes following SFENCE.INVAL.IR are equivalent to reads and writes following SFENCE.VMA

**Test Responsibility**: Verify the ordering guarantees of the three-instruction sequence -- stores before SFENCE.W.INVAL (page table modifications) are visible to SINVAL.VMA, and accesses after SFENCE.INVAL.IR use the updated translation.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| ORDER-01 | store -> W.INVAL -> SINVAL -> INVAL.IR -> load | Modify PTE (store), execute the complete three-instruction sequence, then load to verify the new translation is in effect | Load uses the new translation |
| ORDER-02 | Sequence without SFENCE.W.INVAL | After modifying PTE, execute only SINVAL.VMA + SFENCE.INVAL.IR (omitting SFENCE.W.INVAL) | Behavior undefined (may use old translation) |
| ORDER-03 | Sequence without SFENCE.INVAL.IR | After modifying PTE, execute SFENCE.W.INVAL + SINVAL.VMA (omitting SFENCE.INVAL.IR) | Behavior undefined (may use old translation) |
| ORDER-04 | Full sequence equivalence with SFENCE.VMA | Flush the same page using SFENCE.VMA and the three-instruction sequence separately, verify results are identical | Both methods produce the same result |
| ORDER-05 | SFENCE.VMA as ordering fence for SINVAL.VMA | After modifying PTE, execute SINVAL.VMA, then SFENCE.VMA (substituting for SFENCE.INVAL.IR), then access the page | SFENCE.VMA guarantees SINVAL.VMA takes effect; new translation is in effect |
| ORDER-06 | Multiple SINVAL.VMA followed by SFENCE.VMA | Modify PTEs of multiple pages, execute multiple SINVAL.VMA in batch, and use SFENCE.VMA as the final fence instead of SFENCE.INVAL.IR | New translations take effect on all pages |

> **Note**: ORDER-02 and ORDER-03 test the behavior of incomplete sequences. The specification does not guarantee correctness of incomplete sequences. However, simple implementations may implement SINVAL.VMA identically to SFENCE.VMA and implement SFENCE.W.INVAL and SFENCE.INVAL.IR as NOPs, so incomplete sequences may still work correctly on such implementations. These two tests are intended solely to probe implementation behavior and are not used as compliance criteria.

---

### Group 3: Batch Invalidation Operations

**Specification Basis**:
- `norm:Svinval_split_fine_grained`: The design goal of Svinval is to support batched or pipelined TLB invalidation operations
- `norm:Svinval_sfence_w_inval_orders_before_sinval_vma`: Multiple SINVAL.VMA instructions can be batched between SFENCE.W.INVAL and SFENCE.INVAL.IR

**Test Responsibility**: Verify the correctness of executing multiple SINVAL.VMA instructions between SFENCE.W.INVAL and SFENCE.INVAL.IR, which is the core usage scenario of the Svinval extension.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| BATCH-01 | Batch invalidation of multiple pages | Modify PTEs of 3 pages, execute 3 SINVAL.VMA between W.INVAL and INVAL.IR | New permissions take effect on all 3 pages |
| BATCH-02 | Batch invalidation with different permission changes | Page A upgraded R->RW, page B downgraded RW->R, page C invalidated V->0, batch flush | A is writable, B write triggers fault, C access triggers fault |
| BATCH-03 | Batch invalidation of a large number of pages | Modify PTEs of 16 consecutive pages, execute 16 SINVAL.VMA in batch | New permissions take effect on all 16 pages |
| BATCH-04 | Batch invalidation with mixed rs1 arguments | Some SINVAL.VMA specify a concrete address, others use rs1=x0 | All specified pages are flushed correctly |

---

### Group 4: rs1/rs2 Parameter Combinations

**Specification Basis**:
- `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma`: SINVAL.VMA uses the same rs1/rs2 semantics as SFENCE.VMA
- `norm:Svinval_sequence_rs1_rs2`: The rs1/rs2 of SINVAL.VMA in the three-instruction sequence are equivalent to the rs1/rs2 of the hypothetical SFENCE.VMA

**Test Responsibility**: Verify different combinations of the rs1 (virtual address) and rs2 (ASID) parameters of SINVAL.VMA, ensuring consistency with SFENCE.VMA parameter semantics.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| PARAM-01 | rs1=addr, rs2=x0 | Specify address, ASID=0 (all ASIDs), modify PTE at that address then flush | New translation takes effect at that address |
| PARAM-02 | rs1=x0, rs2=x0 | Global flush (all addresses, all ASIDs), modify multiple PTEs then flush | New translations take effect on all modified pages |
| PARAM-03 | rs1=addr, rs2=asid | Specify address and ASID, modify PTE at that address then flush | New translation takes effect at that address |
| PARAM-04 | rs1=x0, rs2=asid | All addresses for a specified ASID, modify multiple PTEs then flush | New translations take effect on all modified pages under that ASID |
| PARAM-05 | Flush with non-matching address | Modify PTE of page A, but SINVAL.VMA specifies the address of page B | Page A may still use the old translation (behavior undefined) |

---

### Group 5: SINVAL.VMA Privilege Exception Checks

**Specification Basis**:
- `norm:Svinval_illegal_instruction_u_mode`: Executing SINVAL.VMA in U-mode always raises an illegal-instruction exception
- `norm:Svinval_illegal_instruction_tvm`: When mstatus.TVM=1, executing SINVAL.VMA in S-mode raises an illegal-instruction exception

**Test Responsibility**: Verify the exception behavior of the SINVAL.VMA instruction under different privilege levels and CSR configurations.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| PRIV-01 | SINVAL.VMA in U-mode | Execute SINVAL.VMA in U-mode | illegal-instruction exception (cause=2) |
| PRIV-02 | SINVAL.VMA in S-mode with TVM=0 | Execute SINVAL.VMA in S-mode when mstatus.TVM=0 | Executes normally, no exception |
| PRIV-03 | SINVAL.VMA in S-mode with TVM=1 | Execute SINVAL.VMA in S-mode when mstatus.TVM=1 | illegal-instruction exception (cause=2) |
| PRIV-04 | SINVAL.VMA in M-mode | Execute SINVAL.VMA in M-mode | Executes normally, no exception |

---

### Group 6: SFENCE.W.INVAL / SFENCE.INVAL.IR Privilege Behavior

**Specification Basis**:
- `norm:Svinval_sfence_w_inval_inval_u_mode`: Executing SFENCE.W.INVAL or SFENCE.INVAL.IR in U-mode raises an illegal-instruction exception
- `norm:Svinval_sfence_w_inval_inval_s_vs_mode`: SFENCE.W.INVAL and SFENCE.INVAL.IR are unaffected by mstatus.TVM and hstatus.VTVM; they are always permitted in S-mode and VS-mode

**Test Responsibility**: Verify the privilege-level access control of SFENCE.W.INVAL and SFENCE.INVAL.IR instructions, in particular that they are unaffected by the TVM bit.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
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

**Specification Basis**:
- The Svinval extension introduces 3 new instructions (SINVAL.VMA, SFENCE.W.INVAL, SFENCE.INVAL.IR); instruction encodings must be correctly recognized by hardware

**Test Responsibility**: Verify that Svinval instructions can execute normally in M-mode without raising an illegal-instruction exception. This is a prerequisite for all other functional tests -- if the instruction encodings are incorrect or the platform does not support the Svinval extension, all functional tests will fail to run correctly.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| ENC-01 | SINVAL.VMA instruction encoding verification | Execute SINVAL.VMA in M-mode, confirm no illegal-instruction is raised | Executes normally, no exception |
| ENC-02 | SFENCE.W.INVAL instruction encoding verification | Execute SFENCE.W.INVAL in M-mode, confirm no illegal-instruction is raised | Executes normally, no exception |
| ENC-03 | SFENCE.INVAL.IR instruction encoding verification | Execute SFENCE.INVAL.IR in M-mode, confirm no illegal-instruction is raised | Executes normally, no exception |

---

## Test Priority

| Priority | Test Group | Covered Test IDs | Rationale |
|--------|--------|--------------|------|
| P0 (Mandatory) | Group 7 (Encoding Verification), Group 1 (SINVAL.VMA Basic Functionality), Group 5 (Privilege Exceptions) | ENC-01~03, SINVAL-01~07, PRIV-01~04 | Instruction encoding baseline verification, core functionality verification, and security guarantees |
| P1 (Important) | Group 2 (Ordering Semantics), Group 6 (SFENCE.W.INVAL/INVAL.IR Privilege) | ORDER-01~06, FENCE-01~08 | Ordering correctness and privilege-level access control |
| P2 (Recommended) | Group 3 (Batch Invalidation), Group 4 (rs1/rs2 Parameter Combinations) | BATCH-01~04, PARAM-01~05 | Batch operations and parameter coverage |

---

## Test Implementation Notes

### Instruction Encoding

The instructions introduced by the Svinval extension require toolchain support. According to the RISC-V specification and binutils implementation, the instruction encodings are:

- **SINVAL.VMA**: funct7=0001011, rs2, rs1, funct3=000, rd=x0, opcode=1110011
- **SFENCE.W.INVAL**: funct7=0001100, rs2=x0, rs1=x0, funct3=000, rd=x0, opcode=1110011
- **SFENCE.INVAL.IR**: funct7=0001100, rs2=x0, rs1=x0, funct3=000, rd=x0, opcode=1110011 (rs2=x1 distinguishes from SFENCE.W.INVAL)
- **HINVAL.VVMA**: funct7=0011011, rs2, rs1, funct3=000, rd=x0, opcode=1110011
- **HINVAL.GVMA**: funct7=0111011, rs2, rs1, funct3=000, rd=x0, opcode=1110011

Test code should define macros to encapsulate instruction emission, preferring assembler mnemonics when available and falling back to `.insn` encoding when the toolchain does not support them.

### S-mode Helper Functions

The S-mode helper functions used in the test plan need to be defined in the test code. These functions are executed in S-mode with virtual memory enabled via `vm_run_in_smode()`.

> **Note**: The `smode_store_expect_fault` and `smode_load_expect_fault` functions do not handle exceptions themselves. Exceptions are caught by the S-mode trap handler set up internally by `vm_run_in_smode()`, and the trap handler returns the `scause` value as the return value of `vm_run_in_smode()`. Therefore, the caller determines whether the expected fault was triggered by checking the return value.

### File Organization

Svinval tests depend on the virtual memory test framework and the PMP configuration library. The recommended file organization is as follows:

- **svinval/**
  - `Makefile` (ENABLE_VM=1, ENABLE_PMP=1, TARGET=svinval_test.elf)
  - `kernel.ld` -- Linker script (reuse the sv39 template)
  - `main.c` -- Svinval test case entry
- **common/vm/** (existing, no modifications needed)
  - `vm_defs.h` -- Constant definitions
  - `vm.h` -- API declarations
  - `page_table.c` -- Page table management
  - `satp.c` -- satp control and vm_run_in_smode
- **common/pmp/** (existing, no modifications needed)
  - `pmp_cfg.h` -- PMP configuration API (pmp_set_entry, PMP_ENTRY_NAPOT, etc.)
  - `pmp_cfg.c` -- PMP configuration implementation

### Makefile Configuration

> **Note**: Both `ENABLE_VM=1` (for page table management and `vm_run_in_smode`) and `ENABLE_PMP=1` (for PMP configuration in privilege tests) must be enabled simultaneously. The `-march` parameter must include Svinval extension support at compile time; it may be necessary to append `MARCH += _svinval` in the Makefile.

### Common Test Pattern

Svinval test cases follow the pattern below:

1. Initialize the page table and set up identity mapping
2. Map the test page (at an address within the identity-mapped region, far from the code segment)
3. Perform an initial access to establish a TLB entry
4. Modify the PTE
5. Execute the Svinval three-instruction sequence
6. Verify the new translation is in effect
7. Clean up

### Key Considerations

1. **Toolchain Support**: Ensure the GCC/binutils version in use supports Svinval instruction mnemonics (GCC 12+ / binutils 2.38+). If not supported, define `CFLAGS += -DUSE_SVINVAL_ASM=0` in the Makefile and use `.insn` pseudo-instructions for manual encoding (see the "Instruction Encoding" section).

2. **QEMU Support**: QEMU 7.0+ supports the Svinval extension. Enable `svinval=true` in the `-cpu` parameter at startup.

3. **Simple Implementation Equivalence**: The specification permits simple implementations to implement SINVAL.VMA identically to SFENCE.VMA, and to implement SFENCE.W.INVAL and SFENCE.INVAL.IR as NOPs. Therefore, ORDER-02 and ORDER-03 (sequences with missing instructions) may still work correctly on simple implementations. These tests are marked as "behavior undefined" and are intended solely to probe implementation behavior, not as compliance criteria.

4. **Hypervisor Extension**: Testing HINVAL.VVMA and HINVAL.GVMA instructions requires Hypervisor extension support (`norm:Svinval_hinval_vvma_gvma`, `norm:Svinval_hinval_gvma_uses_vmid`, `norm:Svinval_virtual_instruction_vu_vs`, `norm:Svinval_virtual_instruction_vtvms`, `norm:Svinval_sfence_w_inval_inval_vu_mode`). The current test platform does not include the H extension, so these tests are not implemented at this time. Once the H extension test framework is ready, the following tests should be added:
   - HINVAL.VVMA raises virtual-instruction exception in VS-mode/VU-mode
   - HINVAL.GVMA raises virtual-instruction exception in VS-mode/VU-mode
   - HINVAL.GVMA uses VMID instead of ASID
   - SINVAL.VMA raises virtual-instruction exception in VS-mode when hstatus.VTVM=1
   - SFENCE.W.INVAL/SFENCE.INVAL.IR raise virtual-instruction exception in VU-mode

5. **PTE_A and PTE_D Bits**: Unless the test target is the A/D bits themselves, always set `PTE_A | PTE_D` when mapping, to avoid unnecessary page-faults caused by Svade semantics.

6. **PMP Configuration**: Privilege tests (Group 5, Group 6) require PMP configuration to allow S/U-mode code execution. Use the `pmp_set_entry()` and `PMP_ENTRY_NAPOT()` macros (defined in `common/pmp/pmp_cfg.h`) to configure PMP entry 15 as a NAPOT RWX rule covering the firmware code region. The `TEST_END()` macro automatically calls `reset_state()` to clear the PMP configuration.

7. **Test Page Address**: Test pages use addresses within the identity-mapped region that are far from the code segment, such as `PLATFORM_MEM_BASE + 0x800000` (8MB from the base address, i.e., `0x80800000`). This address is within the 256MB physical memory range of the QEMU virt platform (`0x80000000` ~ `0x8FFFFFFF`) and is well clear of the code segment (typically located in the first few MB starting at `0x80000000`), avoiding conflicts with the code execution region.

8. **Page-fault Cause Codes**:
   - `CAUSE_IPF` (12): Instruction page-fault
   - `CAUSE_LPF` (13): Load page-fault
   - `CAUSE_SPF` (15): Store/AMO page-fault
   - `CAUSE_ILLEGAL_INST` (2): Illegal instruction

---

## Spec Coverage Matrix

| Specification Label | Covered Test IDs |
|----------|--------------|
| `norm:Svinval_split_fine_grained` | BATCH-01~04 |
| `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma` | SINVAL-01~07, PARAM-01~05, ORDER-05~06 |
| `norm:Svinval_sfence_w_inval_orders_before_sinval_vma` | ORDER-01~06 |
| `norm:Svinval_sequence_rs1_rs2` | SINVAL-01~07, PARAM-01~05 |
| `norm:Svinval_sequence_reads_writes_before` | ORDER-01~06 |
| `norm:Svinval_sequence_reads_writes_after` | ORDER-01~06 |
| `norm:Svinval_hinval_vvma_gvma` | Pending H extension support |
| `norm:Svinval_hinval_gvma_uses_vmid` | Pending H extension support |
| `norm:Svinval_illegal_instruction_u_mode` | PRIV-01 |
| `norm:Svinval_illegal_instruction_tvm` | PRIV-03 |
| `norm:Svinval_virtual_instruction_vu_vs` | Pending H extension support |
| `norm:Svinval_virtual_instruction_vtvms` | Pending H extension support |
| `norm:Svinval_sfence_w_inval_inval_u_mode` | FENCE-01, FENCE-02 |
| `norm:Svinval_sfence_w_inval_inval_vu_mode` | Pending H extension support |
| `norm:Svinval_sfence_w_inval_inval_s_vs_mode` | FENCE-03~06 |

---

## References

- RISC-V Privileged Specification -- Svinval Extension (`SPEC/svinval.adoc`)
- `SPEC/supervisor.adoc` -- Supervisor-Level ISA (SFENCE.VMA definition)
- `docs/vm_test_framework.md` -- Virtual memory test framework documentation
- `docs/vm_test_plan.md` -- Virtual memory test plan (SFENCE.VMA test reference)
- `common/vm/vm.h` -- VM API declarations
- `common/vm/vm_defs.h` -- Constants and macro definitions
- `common/pmp/pmp_cfg.h` -- PMP configuration API
- `common/platform/qemu_virtplatfrom_config.h` -- QEMU virt platform definitions (`PLATFORM_MEM_BASE`, etc.)
