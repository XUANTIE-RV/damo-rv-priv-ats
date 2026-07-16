**[中文](../testplan/Hypervisor_2_stage_test_plan.md) | English**

# Hypervisor Two-Stage Address Translation Test Plan

## Overview

This document defines the test plan for **RISC-V Hypervisor Extension Two-Stage Address Translation** (VS-stage + G-stage), covering 23 test groups and approximately 164 test cases.

---

## Normative Rules

The following table lists all specification norms referenced by this test plan and their descriptions (quoted from `SPEC/hypervisor.adoc`):

| Norm ID | Original Specification Text |
|---------|---------------------------|
| `norm:H_vm_twostage` | When V=1, two-stage address translation is in effect: VS-stage translation is controlled by `vsatp` and G-stage translation is controlled by `hgatp`. Either stage can be "effectively disabled" by setting the corresponding ATP to Bare. |
| `norm:H_vm_gstagetrans` | When V=1, any implicit memory access for VS-stage address translation (i.e., a read or write of a VS-level page table) is itself subject to G-stage address translation. |
| `norm:hgatp_mode_sv39x4` | When `hgatp`.MODE=Sv39x4, the G-stage address translation uses the Sv39x4 scheme. |
| `norm:hgatp_mode_sv48x4` | When `hgatp`.MODE=Sv48x4, the G-stage address translation uses the Sv48x4 scheme. |
| `norm:hgatp_mode_sv57x4` | When `hgatp`.MODE=Sv57x4, the G-stage address translation uses the Sv57x4 scheme. |
| `norm:H_vm_gpatrans` | The conversion of an Sv32x4, Sv39x4, Sv48x4, or Sv57x4 guest physical address uses the same algorithm as Sv32, Sv39, Sv48, or Sv57, except: `hgatp` substitutes for `satp`; the effective privilege mode must be VS-mode or VU-mode; the current privilege mode is always taken to be U-mode when checking the U bit; and guest-page-fault exceptions are raised instead of regular page-fault exceptions. |
| `norm:H_vm_gpapriv` | For G-stage address translation, all memory accesses are considered to be user-level accesses. Access type permissions are checked during G-stage translation the same as for VS-stage. For memory accesses supporting VS-stage translation, permissions and A/D bit needs are checked as though for an implicit load or store, not for the original access type. However, any exception is always reported for the original access type. |
| `norm:vsstatus_mxr_vm` | The `vsstatus` field MXR, which makes execute-only pages readable by explicit loads, only overrides VS-stage page protection. Setting MXR at VS-level does not override guest-physical page protections. |
| `norm:sstatus_mxr_vm` | Setting MXR at HS-level, however, overrides both VS-stage and G-stage execute-only permissions. |
| `norm:vsatp_sz_acc_op` | The `vsatp` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `satp`. When V=1, `vsatp` substitutes for the usual `satp`. `vsatp` controls VS-stage address translation, the first stage of two-stage translation for guest virtual addresses. |
| `norm:vsatp_v0` | When V=0, `vsatp` does not directly affect the behavior of the machine, unless a virtual-machine load/store (HLV, HLVX, or HSV) or the MPRV feature in the `mstatus` register is used to execute a load or store as though V=1. |
| `norm:vsatp_mode_unsupported_v0` | When V=0, a write to `vsatp` with an unsupported MODE value is either ignored as it is for `satp`, or the fields of `vsatp` are treated as WARL in the normal way. |
| `norm:vsatp_mode_unsupported_v1` | However, when V=1, a write to `satp` with an unsupported MODE value is ignored and no write to `vsatp` is effected. |
| `norm:vs_stage_speculative_a_bit` | When `vsatp` is active, VS-stage page-table entries' A bits must not be set as a result of speculative execution, unless the effective privilege mode is VS or VU. |
| `norm:hlsv_mode` | The hypervisor virtual-machine load and store instructions are valid only in M-mode or HS-mode, or in U-mode when `hstatus`.HU=1. |
| `norm:hlsv_priv` | Each instruction performs an explicit memory access with an effective privilege mode of VS or VU. The effective privilege mode is VU when `hstatus`.SPVP=0, and VS when `hstatus`.SPVP=1. |
| `norm:hlsv_trans` | As usual for VS-mode and VU-mode, two-stage address translation is applied, and the HS-level `sstatus`.SUM is ignored. |
| `norm:hlsv_sstatus_mxr` | HS-level `sstatus`.MXR makes execute-only pages readable by explicit loads for both stages of address translation (VS-stage and G-stage). |
| `norm:hlsv_vsstatus_mxr` | `vsstatus`.MXR affects only the first translation stage (VS-stage). |
| `norm:hlsv_u_op` | Instructions HLVX.HU and HLVX.WU are the same as HLV.HU and HLV.WU, except that execute permission takes the place of read permission during address translation. The supervisor physical memory attributes must grant both execute and read permissions. |
| `norm:hlsv_virtinst` | Attempts to execute a virtual-machine load/store instruction (HLV, HLVX, or HSV) when V=1 cause a virtual-instruction exception. |
| `norm:hlsv_illegalinst` | Attempts to execute one of these same instructions from U-mode when `hstatus`.HU=0 cause an illegal-instruction exception. |
| `norm:hfence-vvma_hfence-gvma_op` | HFENCE.VVMA and HFENCE.GVMA perform a function similar to SFENCE.VMA, except applying to the VS-level memory-management data structures controlled by CSR `vsatp` (HFENCE.VVMA) or the guest-physical memory-management data structures controlled by CSR `hgatp` (HFENCE.GVMA). |
| `norm:hfence-vvma_mode` | HFENCE.VVMA is valid only in M-mode or HS-mode. Executing an HFENCE.VVMA guarantees that any previous stores already visible to the current hart are ordered before all implicit reads by that hart done for VS-stage address translation for subsequent instructions when `hgatp`.VMID has the same setting. |
| `norm:hfence-vvma_limits` | Implicit reads need not be ordered when `hgatp`.VMID is different than at the time HFENCE.VVMA executed. If rs1≠x0, it specifies a single guest virtual address, and if rs2≠x0, it specifies a single guest address-space identifier (ASID). |
| `norm:hfence-vvma_asid` | When rs2≠x0, bits XLEN-1:ASIDMAX of the value held in rs2 are reserved for future standard use. If ASIDLEN < ASIDMAX, the implementation shall ignore bits ASIDMAX-1:ASIDLEN of the value held in rs2. |
| `norm:hfence-vvma_tvm` | Neither `mstatus`.TVM nor `hstatus`.VTVM causes HFENCE.VVMA to trap. |
| `norm:hfence-gvma_op` | HFENCE.GVMA is valid only in HS-mode when `mstatus`.TVM=0, or in M-mode (irrespective of `mstatus`.TVM). Executing an HFENCE.GVMA guarantees that any previous stores already visible to the current hart are ordered before all implicit reads done for G-stage address translation for subsequent instructions. If rs1≠x0, it specifies a single guest physical address, shifted right by 2 bits, and if rs2≠x0, it specifies a single VMID. |
| `norm:hfence-gvma_mode` | If `hgatp`.MODE is changed for a given VMID, an HFENCE.GVMA with rs1=x0 (and rs2 set to either x0 or the VMID) must be executed to order subsequent guest translations with the MODE change—even if the old MODE or new MODE is Bare. |
| `norm:hfence-gvma_vmid` | When rs2≠x0, bits XLEN-1:VMIDMAX of the value held in rs2 are reserved. If VMIDLEN < VMIDMAX, the implementation shall ignore bits VMIDMAX-1:VMIDLEN. |
| `norm:hfence-vvma_hfence-gvma_exceptions` | Attempts to execute HFENCE.VVMA or HFENCE.GVMA when V=1 cause a virtual-instruction exception, while attempts in U-mode cause an illegal-instruction exception. Attempting HFENCE.GVMA in HS-mode when `mstatus`.TVM=1 also causes an illegal-instruction exception. |
| `norm:sfence_vma_v0` | When V=0, the virtual-address argument to SFENCE.VMA is an HS-level virtual address, and the ASID argument is an HS-level ASID. The instruction orders stores only to HS-level address-translation structures with subsequent HS-level address translations. |
| `norm:sfence_vma_v1` | When V=1, the virtual-address argument is a guest virtual address within the current virtual machine, and the ASID argument is a VS-level ASID. The instruction orders stores only to the VS-level address-translation structures with subsequent VS-stage address translations for the same virtual machine. |
| `norm:mstatus_mprv_hypervisor` | The hypervisor extension changes the behavior of MPRV. When MPRV=0, normal translation. When MPRV=1, explicit memory accesses are translated and protected as though the current virtualization mode were set to MPV and the current nominal privilege mode were set to MPP. |
| `norm:mstatus_mprv_hlsv` | MPRV does not affect the virtual-machine load/store instructions, HLV, HLVX, and HSV. The explicit loads and stores of these instructions always act as though V=1 and the nominal privilege mode were `hstatus`.SPVP, overriding MPRV. |
| `norm:H_guest_page_fault` | Guest-page-fault traps may be delegated from M-mode to HS-mode under the control of `medeleg`, but cannot be delegated to other privilege modes. On a guest-page fault, `mtval` or `stval` is written with the faulting guest virtual address, and `mtval2` or `htval` is written either with zero or with the faulting guest physical address, shifted right by 2 bits. |
| `norm:htval_trapval` | When a guest-page-fault trap is taken into HS-mode, `htval` is written with either zero or the guest physical address that faulted, shifted right by 2 bits. For other traps, `htval` is set to zero. |
| `norm:H_trap_xtinst_guestpage` | For guest-page faults, the trap instruction register is written with a special pseudoinstruction value if: (a) the fault is caused by an implicit memory access for VS-stage address translation, and (b) a nonzero value is written to `mtval2` or `htval`. If both conditions are met, zero is not allowed. |
| `norm:H_trap_xtinst_guestpage_rw` | A write pseudoinstruction (0x00002020 or 0x00003020) is used for the case that the machine is attempting automatically to update bits A and/or D in VS-level page tables. All other implicit memory accesses for VS-stage address translation will be reads. |
| `norm:henvcfg_adue_op` | If the Svadu extension is implemented, the ADUE bit controls whether hardware updating of PTE A/D bits is enabled for VS-stage address translation. When ADUE=1, hardware updating is enabled. When ADUE=0, the implementation behaves as though Svade were implemented for VS-stage address translation. If Svadu is not implemented, ADUE is read-only zero. |
| `norm:henvcfg_pbmte_op` | The PBMTE bit controls whether the Svpbmt extension is available for use in VS-stage address translation. When PBMTE=1, Svpbmt is available for VS-stage address translation. When PBMTE=0, the implementation behaves as though Svpbmt were not implemented for VS-stage address translation. If Svpbmt is not implemented, PBMTE is read-only zero. |
| `norm:H_straddle` | When an instruction fetch or a misaligned memory access straddles a page boundary, two different address translations are involved. When a guest-page fault occurs, the faulting virtual address may be a page-boundary address that is higher than the instruction's original virtual address. |
| `norm:mtval2_htval_virtaddr` | When a guest-page fault is not due to an implicit memory access for VS-stage address translation, a nonzero guest physical address written to `mtval2`/`htval` shall correspond to the exact virtual address written to `mtval`/`stval`. |
| `norm:H_vm_gpa_g` | The G bit in all G-stage PTEs is currently not used. It should be cleared by software for forward compatibility, and must be ignored by hardware. |
| `norm:H_pmp` | Machine-level physical memory protection applies to supervisor physical addresses and is in effect regardless of virtualization mode. |
| `norm:H_exception_priority` | If an instruction may raise multiple synchronous exceptions, the decreasing priority order indicates which exception is taken and reported in `mcause` or `scause`. |
| `norm:hgatp_ppn_op` | For the paged virtual-memory schemes, the root page table is 16 KiB and must be aligned to a 16-KiB boundary. In these modes, the lowest two bits of the physical page number (PPN) in `hgatp` always read as zeros. |
| `norm:hgatp_mode_warl` | A write to `hgatp` with an unsupported MODE value is not ignored as it is for `satp`. Instead, the fields of `hgatp` are WARL in the normal way, when so indicated. |

---

## Test Group Definitions

### Group 1: VS-stage Single Stage when V=1 (hgatp = Bare)

**Specification Basis**:
- `norm:H_vm_twostage`: Two-stage is enforced when V=1, but either stage can be "effectively disabled" by setting the corresponding ATP to Bare
- `norm:vsatp_sz_acc_op`: `vsatp` is VS-mode's `satp`, using the same algorithm and PTE format
- When hgatp=Bare, GPA equals SPA directly, and two-stage degenerates to single VS-stage translation

**Test Responsibility**: When hgatp=Bare, VS-stage behavior should be identical to normal S-mode translation controlled by satp. This group uses the core Groups from `docs/vm_test_plan.md` as baseline to verify vsatp equivalence under V=1.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-VS-01 | Sv39 vsatp 1GB gigapage | hgatp=Bare, vsatp=Sv39, 1GB identity mapping, VS-mode R/W | R/W success |
| TS-VS-02 | Sv39 vsatp 2MB megapage | Same as above, 2MB | R/W success |
| TS-VS-03 | Sv39 vsatp 4KB page | Same as above, 4KB | R/W success |
| TS-VS-04 | Sv48 vsatp 4KB | hgatp=Bare, vsatp=Sv48, 4KB mapping | R/W success |
| TS-VS-05 | Sv57 vsatp 4KB | hgatp=Bare, vsatp=Sv57, 4KB mapping | R/W success |
| TS-VS-06 | VS-stage U-bit: VS accesses U=0 | vsatp=Sv39, PTE U=0, VS-mode (nominal S) access, `vsstatus.SUM=0` | Success (VS-mode is S-level, U=0 PTE denies U-mode but allows VS-mode) |
| TS-VS-07 | VS-stage U-bit: VS accesses U=1 + SUM=0 | vsatp=Sv39, PTE U=1, VS-mode access, `vsstatus.SUM=0` | store/load page-fault (cause 13/15, equivalent to vm_test_plan SUM-02) |
| TS-VS-08 | VS-stage U-bit: VS accesses U=1 + SUM=1 | Same as above but `vsstatus.SUM=1` | Success |
| TS-VS-09 | VS-stage MXR: vsstatus.MXR=1 reads X-only | PTE R=0,X=1, `vsstatus.MXR=1`, VS-mode load | Success |
| TS-VS-10 | VS-stage PTE V=0/RW=01 | vsatp=Sv39, PTE V=0 or R=0,W=1 | page-fault (cause 12/13/15, **not** guest-page-fault) |

> [!NOTE]
> All Group 1 test cases use the corresponding Groups from `vm_test_plan.md` as baseline, but the trap context switches from `s_trap_handler` to `hs_trap_handler`, and the trap source is `hstatus.SPV=1`. The fault cause remains 12/13/15 (regular page-fault) because G-stage Bare does not trigger guest-page-fault.

---

### Group 2: vsatp CSR Behavior

**Specification Basis**:
- `norm:vsatp_sz_acc_op`: When V=1, `satp` actually accesses `vsatp`
- `norm:vsatp_mode_unsupported_v0`: When V=0, writing unsupported MODE to `vsatp` behaves like `satp` (ignored or WARL)
- `norm:vsatp_mode_unsupported_v1`: When V=1, writing unsupported MODE to `satp` is **ignored** and does not write to `vsatp`
- `norm:vs_stage_speculative_a_bit`: VS-stage A bit must not be set by speculative execution (unless actually in VS/VU-mode)
- `norm:vsatp_v0`: When V=0, `vsatp` does not directly affect machine behavior, but HLV/HSV/MPRV can trigger as-if V=1

**Test Responsibility**: Verify vsatp access, MODE WARL/ignore behavior, ASID field, and V=0/V=1 differences.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-VSATP-01 | satp accesses vsatp when V=1 | HS-mode writes a value to vsatp, switch to VS-mode and read with `csrr satp` | Reads vsatp value |
| TS-VSATP-02 | satp writes to vsatp when V=1 | VS-mode writes a legal value with `csrw satp`, switch back to HS-mode and read with `csrr vsatp` | vsatp has been written |
| TS-VSATP-03 | Writing unsupported MODE ignored when V=1 | VS-mode writes MODE=2 (reserved) via `csrw satp` | vsatp not written (different from V=0 behavior) |
| TS-VSATP-04 | Writing reserved MODE to vsatp when V=0 | HS-mode directly writes MODE=7 (reserved encoding, never supported) with `csrw vsatp` | (a) Entire write ignored (satp semantics), or (b) MODE adjusted to legal value (0/8/9/10) by WARL; reserved encoding must not persist (norm:vsatp_mode_unsupported_v0) |
| TS-VSATP-05 | vsatp ASID field read/write | HS-mode writes vsatp ASID=0xFF, reads back | ASID preserves implementation-supported bits |
| TS-VSATP-06 | vsatp PPN field read/write | HS-mode writes legal PPN, reads back | PPN field reads back correctly (no PPN[1:0] forced to zero like hgatp) |
| TS-VSATP-07 | VS accesses satp when hstatus.VTVM=1 | Set `hstatus.VTVM=1`, VS-mode `csrr satp` | virtual-instruction exception (cause=22) |


---

### Group 3: Full Two-Stage Identity Mapping (Same Width)

**Specification Basis**:
- `norm:H_vm_twostage`: Both stages are effective when V=1
- `norm:H_vm_gstagetrans`: When V=1, even implicit accesses supporting VS-stage translation (reading/writing VS-level page tables) go through G-stage translation

**Test Responsibility**: Verify the core translation path for three same-width two-stage combinations under VA = GPA = SPA identity mapping.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-MAP-01 | Sv39+Sv39x4 1GB two-stage identity | Two-stage 1GB identity mapping, VS-mode R/W | Success |
| TS-MAP-02 | Sv39+Sv39x4 2MB | Two-stage 2MB | Success |
| TS-MAP-03 | Sv39+Sv39x4 4KB | Two-stage 4KB | Success |
| TS-MAP-04 | Sv39+Sv39x4 superpage mixed | VS-stage uses 1GB, G-stage uses 4KB; and vice versa | Success (effective page size takes the smaller) |
| TS-MAP-05 | Sv48+Sv48x4 1GB | Same-mode 1GB two-stage | Success |
| TS-MAP-06 | Sv48+Sv48x4 2MB | Same-mode 2MB | Success |
| TS-MAP-07 | Sv48+Sv48x4 4KB | Same-mode 4KB | Success |
| TS-MAP-08 | Sv48+Sv48x4 512GB terapage | Large superpage (subject to platform memory limits) | Success (within legal subrange) |
| TS-MAP-09 | Sv57+Sv57x4 1GB | Same-mode 1GB | Success |
| TS-MAP-10 | Sv57+Sv57x4 2MB | Same-mode 2MB | Success |
| TS-MAP-11 | Sv57+Sv57x4 4KB | Same-mode 4KB | Success |
| TS-MAP-12 | VU-mode two-stage access | Sv39+Sv39x4, both VS-stage and G-stage U=1, enter VU-mode access | Success |

---

### Group 4: Cross-Width Two-Stage Combinations

**Specification Basis**:
- VS-stage and G-stage MODE selection are independent; the specification does not require matching widths (typical usage is hypervisor selecting wider or narrower scheme in hgatp than vsatp)
- `norm:hgatp_mode_sv39x4`, `norm:hgatp_mode_sv48x4`, `norm:hgatp_mode_sv57x4`: Define three G-stage modes

**Test Responsibility**: Cover all 9 cross-width combinations (including 3 VS<=G and 3 VS>G normal flows, plus 3 VS>G GPA out-of-bounds fault flows), with each pair verified using multiple page granularity sub-variants (4K/2M/1G) to ensure combinations are usable and translation is correct.

#### VS<=G Normal Flow (VS address space no wider than G)

| Test ID | Test Name | VS-stage | G-stage | Test Description | Expected Result |
|---------|-----------|----------|---------|------------------|-----------------|
| TS-XMODE-01.a | Sv39+Sv48x4 VS=4K G=4K | Sv39 | Sv48x4 | 4KB identity mapping R/W | Success |
| TS-XMODE-01.b | Sv39+Sv48x4 VS=4K G=2M | Sv39 | Sv48x4 | G-stage 2MB superpage | Success |
| TS-XMODE-01.c | Sv39+Sv48x4 VS=4K G=1G | Sv39 | Sv48x4 | G-stage 1GB superpage | Success |
| TS-XMODE-02.a | Sv39+Sv57x4 VS=4K G=4K | Sv39 | Sv57x4 | 4KB identity mapping R/W | Success |
| TS-XMODE-02.b | Sv39+Sv57x4 VS=4K G=2M | Sv39 | Sv57x4 | G-stage 2MB superpage | Success |
| TS-XMODE-02.c | Sv39+Sv57x4 VS=4K G=1G | Sv39 | Sv57x4 | G-stage 1GB superpage | Success |
| TS-XMODE-04.a | Sv48+Sv57x4 VS=4K G=4K | Sv48 | Sv57x4 | 4KB identity mapping R/W | Success |
| TS-XMODE-04.b | Sv48+Sv57x4 VS=4K G=2M | Sv48 | Sv57x4 | G-stage 2MB superpage | Success |
| TS-XMODE-04.c | Sv48+Sv57x4 VS=4K G=1G | Sv48 | Sv57x4 | G-stage 1GB superpage | Success |

#### VS>G Normal Flow (VS address space wider than G, but GPA falls within G-stage addressable range)

| Test ID | Test Name | VS-stage | G-stage | Test Description | Expected Result |
|---------|-----------|----------|---------|------------------|-----------------|
| TS-XMODE-03.a | Sv48+Sv39x4 VS=4K G=4K | Sv48 | Sv39x4 | 4KB identity mapping R/W; GPA falls within [0, 2^41) | Success |
| TS-XMODE-03.b | Sv48+Sv39x4 VS=4K G=2M | Sv48 | Sv39x4 | G-stage 2MB superpage; GPA < 2^41 | Success |
| TS-XMODE-03.c | Sv48+Sv39x4 VS=4K G=1G | Sv48 | Sv39x4 | G-stage 1GB superpage; GPA < 2^41 | Success |
| TS-XMODE-05.a | Sv57+Sv39x4 VS=4K G=4K | Sv57 | Sv39x4 | 4KB identity mapping R/W; GPA < 2^41 | Success |
| TS-XMODE-05.b | Sv57+Sv39x4 VS=4K G=2M | Sv57 | Sv39x4 | G-stage 2MB superpage; GPA < 2^41 | Success |
| TS-XMODE-05.c | Sv57+Sv39x4 VS=4K G=1G | Sv57 | Sv39x4 | G-stage 1GB superpage; GPA < 2^41 | Success |
| TS-XMODE-06.a | Sv57+Sv48x4 VS=4K G=4K | Sv57 | Sv48x4 | 4KB identity mapping R/W; GPA < 2^50 | Success |
| TS-XMODE-06.b | Sv57+Sv48x4 VS=4K G=2M | Sv57 | Sv48x4 | G-stage 2MB superpage; GPA < 2^50 | Success |
| TS-XMODE-06.c | Sv57+Sv48x4 VS=4K G=1G | Sv57 | Sv48x4 | G-stage 1GB superpage; GPA < 2^50 | Success |

#### VS>G Out-of-Bounds Fault Flow (VS translation produces GPA exceeding G-stage addressable range)

| Test ID | Test Name | VS-stage | G-stage | Test Description | Expected Result |
|---------|-----------|----------|---------|------------------|-----------------|
| TS-XMODE-07 | Sv48+Sv39x4 GPA out-of-bounds | Sv48 | Sv39x4 | VS-stage PTE PPN points to GPA >= 2^41 | guest-page-fault (cause=21) |
| TS-XMODE-08 | Sv57+Sv39x4 GPA out-of-bounds | Sv57 | Sv39x4 | VS-stage PTE PPN points to GPA >= 2^41 | guest-page-fault (cause=21) |
| TS-XMODE-09 | Sv57+Sv48x4 GPA out-of-bounds | Sv57 | Sv48x4 | VS-stage PTE PPN points to GPA >= 2^50 | guest-page-fault (cause=21) |

> [!NOTE]
> - TS-XMODE-03/05/06 key scenario: Although VS-stage address space is wider than G-stage, as long as the GPA produced by VS translation falls within G-stage addressable range (low address region), two-stage translation should complete normally. Tests use identity mapping in low memory region (test_data_area < 4GB) to verify this scenario.
> - TS-XMODE-07/08/09 key scenario: When VS-stage translation produces a GPA exceeding what G-stage mode can handle (e.g., Sv48 outputs GPA >= 2^41 but hgatp=Sv39x4 only supports 41-bit GPA), guest-page-fault (cause=21) should be triggered. Implementation tampers with VS leaf PTE PPN to point to out-of-bounds address.

---

### Group 5: Non-Identity Two-Stage Mapping

**Specification Basis**:
- `norm:H_vm_twostage`: Two-stage independent translation, final physical address = composition of two-stage mappings
- Specification does not restrict VA, GPA, SPA to be equal

**Test Responsibility**: Verify end-to-end correctness of two-stage chain under non-identity mapping, and verify that VS-stage output GPA strictly matches G-stage input GPA.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-NID-01 | VS-stage non-identity | VS-stage maps VA1 -> GPA_X, G-stage maps GPA_X -> SPA_X (GPA_X = SPA_X identity); VS-mode writes VA1, physical SPA_X should be modified | Physical read of SPA_X sees written value |
| TS-NID-02 | G-stage non-identity | VS-stage maps VA1 -> GPA_X identity; G-stage maps GPA_X -> SPA_Y (different address); VS-mode writes VA1, physical SPA_Y should be modified | Physical read of SPA_Y sees written value; physical read of GPA_X unchanged |
| TS-NID-03 | Both stages non-identity | VS-stage VA1 -> GPA_X, G-stage GPA_X -> SPA_Y | VS-mode access to VA1 affects physical SPA_Y |
| TS-NID-04 | Multi-page mapping continuity | VS-stage and G-stage each map 4 consecutive pages to different base addresses, verify all 4 page mappings take effect | Each page R/W succeeds and data is isolated |

---

### Group 6: G-stage Fault Triggered by Implicit Access

**Specification Basis**:
- `norm:H_vm_gstagetrans`: When V=1, implicit accesses supporting VS-stage translation (reading VS-level page tables) are also subject to G-stage translation
- `norm:H_vm_gpapriv`: Implicit accesses check permissions and A/D bits as implicit load/store
- `norm:H_guest_page_fault`: On fault, stval=GVA, htval may be written with GPA of implicit access
- `norm:htval_trapval`: When fault is triggered by implicit VS-stage access, htval is written with **PTE's GPA** (not GPA corresponding to original VA)
- `norm:H_trap_xtinst_guestpage`: In this scenario, htinst must be written with pseudoinstruction (0 not allowed)
- `norm:H_trap_xtinst_guestpage_rw`: read uses `0x00002000`/`0x00003000`, write (A/D auto-update) uses `0x00002020`/`0x00003020`

**Test Responsibility**: Place VS-stage page table itself at a GPA that G-stage does not map or has no permissions, trigger implicit access fault, verify htval/htinst/cause correctness.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-IMPL-01 | VS-stage PT at unmapped GPA in G-stage | VS-stage root page table allocated at GPA_PT, G-stage does not map GPA_PT; VS-mode load VA | cause=21 (load guest-page-fault), stval=VA, htval=GPA_PT>>2, htinst=`0x00003000` (RV64 read) |
| TS-IMPL-03 | VS-stage PT read-only in G-stage + D=0 store | VS-stage PTE needs hardware D bit update (store access), but G-stage maps corresponding GPA as read-only | cause=23 (HW A/D implementation) or cause=21 (no HW A/D implementation), both compliant |
| TS-IMPL-04 | VS-stage PT G-stage U=0 | VS-stage page table GPA mapped with U=0 in G-stage | Implicit access fails, guest-page-fault |
| TS-IMPL-06 | inst guest-page-fault from implicit | VS-stage translation fetch PT implicit access fails | cause=20, htinst must be pseudoinst (read: `0x00003000`) |

> [!NOTE]
> TS-IMPL-01 obtains VS-level root page table physical address via `ctx.vs_ctx.root_pt` field (provided by `pt_context` in `common/vm/page_table.c`), **without depending on any unimplemented interfaces**. Linker symbols like `__text_start/__text_end` are provided by existing `kernel.ld` (see `sv39/kernel.ld`); new test directories must maintain the same symbol exports. `map_range_4k` is a local helper function within the test file.

---

### Group 7: Permission Intersection (VS-stage RWX x G-stage RWX)

**Specification Basis**:
- Two-stage permissions take intersection: any stage denial triggers fault
- Fault cause distinction: VS-stage failure -> page-fault (12/13/15); G-stage failure -> guest-page-fault (20/21/23)

**Test Responsibility**: Cover key intersections of VS-stage and G-stage RWX permissions, verify correspondence between fault cause and stage source.

| Test ID | Test Name | VS-stage PTE | G-stage PTE | Access Type | Expected Result |
|---------|-----------|--------------|-------------|-------------|-----------------|
| TS-PERM-01 | Dual-stage RWX | RWX | RWXU | R/W/X | All success |
| TS-PERM-02 | VS-stage R-only, G-stage RWX | R | RWXU | store | store page-fault (cause=15, **VS-stage source**) |
| TS-PERM-03 | VS-stage RWX, G-stage R-only | RWX | RU | store | store guest-page-fault (cause=23, **G-stage source**) |
| TS-PERM-04 | VS-stage X-only, G-stage RWX | X | RWXU | load (no MXR) | load page-fault (cause=13) |
| TS-PERM-05 | VS-stage RWX, G-stage X-only | RWX | XU | load (no MXR) | load guest-page-fault (cause=21) |
| TS-PERM-06 | VS-stage RX, G-stage RX | RX | RXU | fetch | Success |
| TS-PERM-07 | VS-stage V=0 | V=0 | RWXU | load | page-fault (cause=13) |
| TS-PERM-08 | G-stage V=0 | RWX | V=0 | load | guest-page-fault (cause=21) |
| TS-PERM-09 | G-stage U=0 | RWX | RWX (U=0) | load | guest-page-fault (cause=21) |
| TS-PERM-10 | VS-stage U=0 (VU access) | RWX (U=0) | RWXU | VU-mode load | page-fault (cause=13, U-mode cannot access U=0) |
| TS-PERM-11 | VS-stage U=1 (VS access, SUM=0) | RWX (U=1) | RWXU | VS-mode load with vsstatus.SUM=0 | page-fault (cause=13) |
| TS-PERM-12 | VS-stage U=1 (VS access, SUM=1) | RWX (U=1) | RWXU | VS-mode load with vsstatus.SUM=1 | Success |

---

### Group 8: MXR Differential Effects in Two-Stage

**Specification Basis**:
- `norm:vsstatus_mxr_vm`: "The vsstatus field MXR ... only overrides VS-stage page protection. Setting MXR at VS-level does not override guest-physical page protections."
- `norm:sstatus_mxr_vm`: "Setting MXR at HS-level, however, overrides both VS-stage and G-stage execute-only permissions."

**Test Responsibility**: Verify MXR's dual semantics in two-stage.

| Test ID | Test Name | VS-stage | G-stage | sstatus.MXR | vsstatus.MXR | Access | Expected Result |
|---------|-----------|----------|---------|-------------|--------------|--------|-----------------|
| TS-MXR-01 | No MXR, X-only unreadable | X-only | RWXU | 0 | 0 | load | load page-fault |
| TS-MXR-02 | vsstatus.MXR=1, VS X-only readable | X-only | RWXU | 0 | 1 | load | Success |
| TS-MXR-03 | vsstatus.MXR=1 cannot override G-stage X-only | RWX | X-only U | 0 | 1 | load | guest-page-fault (vsstatus.MXR does not affect G-stage) |
| TS-MXR-04 | sstatus.MXR=1 overrides both stages | X-only | X-only U | 1 | 0 | load | Success |
| TS-MXR-05 | sstatus.MXR=1 + vsstatus.MXR=1 | X-only | X-only U | 1 | 1 | load | Success (equivalent to TS-MXR-04) |

---

### Group 9: SUM Effects in Two-Stage

**Specification Basis**:
- `vsstatus.SUM` controls VS-stage U-bit checking (whether VS-mode can access U=1 PTEs)
- G-stage always treats as U-mode, no SUM concept exists
- `norm:hlsv_trans`: HS-level `sstatus.SUM` is ignored during HLV/HSV (see Group 13)

**Test Responsibility**: Verify vsstatus.SUM only affects VS-stage.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-SUM-01 | vsstatus.SUM=0, VS accesses U=1 | VS-stage PTE U=1, VS-mode load with SUM=0 | page-fault (cause=13) |
| TS-SUM-02 | vsstatus.SUM=1, VS accesses U=1 | Same as above with SUM=1 | Success |
| TS-SUM-03 | SUM does not affect G-stage U-bit | VS-stage U=1 (VS access OK with SUM=1), G-stage U=0 | guest-page-fault (G-stage not affected by SUM) |

---

### Group 10: HFENCE.VVMA Semantics

**Specification Basis**:
- `norm:hfence-vvma_hfence-gvma_op`: HFENCE.VVMA is similar to executing SFENCE.VMA in VS-mode, but guarantees VS-level page table writes are visible before subsequent VS-stage translations
- `norm:hfence-vvma_mode`: Valid only in M-mode / HS-mode
- `norm:hfence-vvma_limits`: Only affects VM corresponding to current hgatp.VMID; rs1/rs2 select VA/ASID
- `norm:hfence-vvma_tvm`: mstatus.TVM and hstatus.VTVM do not affect HFENCE.VVMA
- `norm:hfence-vvma_hfence-gvma_exceptions`: V=1 execution -> virtual-instruction exception (cause=22); U-mode execution -> illegal-instruction exception

**Test Responsibility**: Verify HFENCE.VVMA flush effectiveness and exception behavior.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-HV-01 | Global flush (rs1=0, rs2=0) | VS-stage modifies PTE, HS-mode executes `hfence.vvma`, then VS-mode accesses | New PTE takes effect |
| TS-HV-02 | Flush by VA (rs1!=0) | Modify VS-stage PTE, then `hfence.vvma vaddr, x0` | New PTE for that VA takes effect |
| TS-HV-03 | Flush by ASID (rs2!=0) | `hfence.vvma x0, asid` | Specified ASID translation is flushed |
| TS-HV-04 | mstatus.TVM=1 has no effect | HS-mode sets TVM=1, then executes `hfence.vvma` | Executes normally (no exception) |
| TS-HV-05 | hstatus.VTVM=1 has no effect | HS-mode sets VTVM=1, then executes `hfence.vvma` | Executes normally |
| TS-HV-06 | V=1 execution -> virtual-inst exception | VS-mode executes `hfence.vvma` | virtual-instruction exception (cause=22) |

---

### Group 11: HFENCE.GVMA Semantics

**Specification Basis**:
- `norm:hfence-gvma_op`: HFENCE.GVMA flushes G-stage translation cache
- Valid only in HS-mode (mstatus.TVM=0) or M-mode
- `norm:hfence-gvma_mode`: Must execute HFENCE.GVMA (rs1=0) after modifying hgatp.MODE
- `norm:hfence-gvma_vmid`: rs2 selects VMID
- rs1 is GPA>>2
- `norm:hfence-vvma_hfence-gvma_exceptions`: V=1 -> virtual-inst; mstatus.TVM=1 in HS-mode -> illegal-inst; U-mode -> illegal-inst

**Test Responsibility**: Verify HFENCE.GVMA flush effectiveness and exception behavior.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-HG-01 | Global flush (rs1=0, rs2=0) | HS-mode modifies G-stage PTE, then `hfence.gvma`, VS-mode accesses | New G-stage PTE takes effect |
| TS-HG-02 | Flush by GPA (rs1=GPA>>2) | `hfence.gvma gpa>>2, x0` | Specified GPA translation is flushed |
| TS-HG-03 | Flush by VMID (rs2=vmid) | `hfence.gvma x0, vmid` | Specified VMID translation is flushed |
| TS-HG-04 | Must fence after hgatp.MODE switch | Switch hgatp.MODE Sv39x4<->Sv48x4 | Works normally only after executing `hfence.gvma` post-switch |
| TS-HG-05 | mstatus.TVM=1 triggers illegal | HS-mode sets TVM=1, then `hfence.gvma` | illegal-instruction (cause=2) |
| TS-HG-06 | V=1 execution -> virtual-inst | VS-mode executes `hfence.gvma` | virtual-instruction (cause=22) |

---

### Group 12: SFENCE.VMA Behavior under V=1

**Specification Basis**:
- `norm:sfence_vma_v0`: When V=0, SFENCE.VMA operates on HS-level page tables
- `norm:sfence_vma_v1`: When V=1, SFENCE.VMA's VA is GVA, ASID is VS-level ASID, only affects VM corresponding to hgatp.VMID
- `norm:hstatus_vtvm_op`: When hstatus.VTVM=1, VS-mode executing SFENCE.VMA -> virtual-instruction exception

**Test Responsibility**: Verify SFENCE.VMA semantics switching under V=1.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-SF-01 | V=1 SFENCE.VMA flushes VS-stage | VS-mode modifies PTE controlled by vsatp, then executes `sfence.vma` | New PTE takes effect |
| TS-SF-02 | V=1 SFENCE.VMA does not flush G-stage | VS-mode `sfence.vma` does not affect G-stage translation cache | G-stage modifications do not take effect immediately (requires HFENCE.GVMA) |
| TS-SF-03 | hstatus.VTVM=1 -> virtual-inst | Set VTVM=1, VS-mode executes `sfence.vma` | virtual-instruction (cause=22) |
| TS-SF-04 | V=0 SFENCE.VMA does not affect VS-stage | HS-mode executes `sfence.vma` | Only flushes HS-level translation cache |

---

### Group 13: HLV/HLVX/HSV Two-Stage Translation

**Specification Basis**:
- `norm:hlsv_mode`: HLV/HLVX/HSV valid only in M-mode or HS-mode; U-mode only when `hstatus.HU=1`
- `norm:hlsv_priv`: Effective privilege determined by `hstatus.SPVP` (0=VU, 1=VS)
- `norm:hlsv_trans`: Always uses two-stage translation (vsatp + hgatp), HS-level `sstatus.SUM` is ignored
- `norm:hlsv_sstatus_mxr`: HS-level `sstatus.MXR` affects both stages
- `norm:hlsv_vsstatus_mxr`: `vsstatus.MXR` affects only VS-stage
- `norm:hlsv_u_op`: HLVX.HU/WU uses execute permission instead of read permission
- `norm:hlvx-wu_valid32`: HLVX.WU is valid on RV32 as well
- `norm:hlsv_virtinst`: V=1 execution of HLV/HLVX/HSV -> virtual-instruction exception
- `norm:hlsv_illegalinst`: U-mode with hstatus.HU=0 -> illegal-instruction exception

**Test Responsibility**: Verify HLV/HLVX/HSV behavior under two-stage translation, effective privilege control, MXR/SUM differences, exception triggering.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-HLV-01 | HS-mode HLV.D reads guest memory | Two-stage mapping established, HS-mode executes `hlv.d` to read guest VA | Reads correct value |
| TS-HLV-02 | HS-mode HSV.D writes guest memory | HS-mode executes `hsv.d` to write guest VA | Guest memory is written |
| TS-HLV-03 | SPVP=0 (VU) accesses U=0 -> fault | VS-stage PTE U=0, SPVP=0, HS-mode `hlv.d` | load page-fault (U-mode cannot access U=0) |
| TS-HLV-04 | SPVP=1 (VS) accesses U=0 -> success | VS-stage PTE U=0, SPVP=1, HS-mode `hlv.d` | Success (VS-mode is S-level) |
| TS-HLV-05 | sstatus.SUM is ignored | VS-stage PTE U=1, SPVP=1, sstatus.SUM=0 but vsstatus.SUM=1, HS-mode `hlv.d` | Success (HS sstatus.SUM does not affect, controlled by vsstatus.SUM) |
| TS-HLV-06 | sstatus.MXR=1 affects VS-stage X-only | VS-stage X-only, G-stage RWX, sstatus.MXR=1, HS-mode `hlv.d` | Success |
| TS-HLV-07 | sstatus.MXR=1 affects G-stage X-only | VS-stage RWX, G-stage X-only U, sstatus.MXR=1, HS-mode `hlv.d` | Success (HS-MXR affects both stages) |
| TS-HLV-08 | vsstatus.MXR=1 does not affect G-stage X-only | VS-stage RWX, G-stage X-only U, vsstatus.MXR=1, sstatus.MXR=0, HS-mode `hlv.d` | guest-page-fault |
| TS-HLV-09 | HLVX.WU uses X permission instead of R | VS-stage X-only (R=0,X=1), G-stage X-only U, HS-mode `hlvx.wu` | Success (X permission suffices) |
| TS-HLV-10 | V=1 executes HLV -> virtual-inst | VS-mode executes `hlv.d` | virtual-instruction exception (cause=22) |
| TS-HLV-11 | U-mode + HU=0 -> illegal | hstatus.HU=0, U-mode executes `hlv.d` | illegal-instruction exception (cause=2) |
| TS-HLV-12 | U-mode + HU=1 -> normal | hstatus.HU=1, U-mode executes `hlv.d`, correct two-stage mapping | Success |

> [!NOTE]
> TS-HLV-09 (HLVX) test requires test page to have at least X=1 in both VS-stage and G-stage (read permission may be absent). Additionally, SPA corresponding physical memory attributes must satisfy both X+R permissions (`norm:hlsv_u_op`), satisfied in practice through PMP configuration of full RWX.

---

### Group 14: mstatus.MPRV+MPV Triggered Two-Stage

**Specification Basis**:
- `norm:mstatus_mprv_hypervisor`: In M-mode when `mstatus.MPRV=1`, explicit memory accesses are translated according to virtualization mode and privilege level determined by `mstatus.MPV` and `mstatus.MPP`
- h-mprv table: MPRV=1 + MPV=1 + MPP=0 -> VU-level two-stage; MPRV=1 + MPV=1 + MPP=1 -> VS-level two-stage; MPP=3 (M) -> no translation
- `norm:mstatus_mprv_hlsv`: MPRV does not affect HLV/HLVX/HSV (these instructions always act as V=1 + SPVP)

**Test Responsibility**: Verify two-stage translation triggered by MPRV+MPV combinations in M-mode, covering key rows of h-mprv table.

| Test ID | Test Name | MPRV | MPV | MPP | Test Description | Expected Result |
|---------|-----------|------|-----|-----|------------------|-----------------|
| TS-MPRV-01 | MPRV=0 -> no translation | 0 | 1 | 1 | M-mode normal ld access, MPRV=0 | Direct physical access, no two-stage triggered |
| TS-MPRV-02 | MPRV=1 + MPV=1 + MPP=S -> VS-level two-stage | 1 | 1 | S | Two-stage mapping ready, M-mode normal ld accesses guest VA | Translated via two-stage, accesses SPA |
| TS-MPRV-03 | MPRV=1 + MPV=1 + MPP=U -> VU-level two-stage | 1 | 1 | U | Two-stage mapping U=1, M-mode normal ld | Translated via two-stage from VU perspective; U=0 PTE triggers fault |
| TS-MPRV-04 | MPRV=1 + MPP=M -> no translation | 1 | x | M | M-mode normal ld, MPP=M | Direct physical access (independent of MPV) |
| TS-MPRV-05 | MPRV does not affect HLV | 1 | 0 | M | M-mode sets MPRV=1 + MPV=0, but executes HLV.D | HLV still translates as V=1 + SPVP, independent of MPRV/MPV/MPP |

> [!WARNING]
> TS-MPRV series tests require special attention: After M-mode sets MPRV=1, **all** explicit load/store in M-mode itself are translated, including stack accesses and CSR reads/writes within trap handlers. Must enable MPRV within minimal code block and clear immediately after target access to avoid affecting stack/local variable accesses. Recommend using inline asm to strictly control MPRV enable window.

---

### Group 15: A/D Bit Two-Stage Handling

**Specification Basis**:
- `norm:henvcfg_adue_op`: When ADUE=0, VS-stage behaves as Svade (A/D=0 triggers page-fault instead of hardware update); when ADUE=1, hardware can auto-update VS-stage PTE A/D
- `norm:H_vm_gpapriv`: Implicit memory accesses supporting VS-stage translation (including implicit store for hardware A/D update) check G-stage permissions and A/D bits as implicit load/store

**Test Responsibility**: Verify henvcfg.ADUE control effect on VS-stage A/D bit hardware update, and G-stage permission interception behavior for implicit A/D writes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-AD-01 | VS-stage A=0 when ADUE=0 | henvcfg.ADUE=0, VS-stage PTE A=0, VS-mode load | page-fault (cause=13), hardware does not auto-update A bit |
| TS-AD-02 | VS-stage A=0 normal when ADUE=1 | henvcfg.ADUE=1, VS-stage PTE A=0, G-stage maps PTE GPA as RWU | Access succeeds, VS-stage PTE A bit auto-set to 1 by hardware |
| TS-AD-03 | A bit update intercepted by G-stage read-only when ADUE=1 | henvcfg.ADUE=1, VS-stage PTE A=0, G-stage maps PTE GPA as read-only U | cause=23 (HW A/D implementation) or cause=21 (no HW A/D implementation), both compliant |
| TS-AD-04 | D bit update intercepted by G-stage read-only when ADUE=1 | henvcfg.ADUE=1, VS-stage PTE A=1,D=0, VS-mode store, G-stage maps PTE GPA as read-only U | guest-page-fault (cause=23), htinst=`0x00003020` (write pseudoinst) |
| TS-AD-05 | G-stage PTE A=0 | G-stage PTE A=0 (other permissions normal RWXU), VS-mode load | guest-page-fault (cause=21) |
| TS-AD-06 | G-stage PTE D=0 + store | G-stage PTE D=0 (R=1,W=1,U=1,A=1), VS-mode store | guest-page-fault (cause=23) |

---

### Group 16: Page Boundary Straddle

**Specification Basis**:
- `norm:H_straddle`: When instruction fetch or misaligned memory access straddles page boundary, two address translations are involved; on guest-page-fault, stval may be page boundary address
- `norm:mtval2_htval_virtaddr`: On non-implicit access guest-page-fault, nonzero GPA in htval must correspond to exact virtual address pointed to by stval

**Test Responsibility**: Verify fault behavior and htval/stval precision for cross-page-boundary accesses under two-stage translation.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-STRD-01 | Load crosses page, second page G-stage unmapped | 4-byte load starting at page_end-2, first page two-stage normal, second page G-stage invalid | guest-page-fault (cause=21), stval=second page start GVA, htval=second page GPA>>2 |
| TS-STRD-02 | Fetch crosses page, second page G-stage no X | 2-byte compressed instruction starting at last byte of page, second page G-stage X=0 | inst guest-page-fault (cause=20), stval=second page start GVA |
| TS-STRD-03 | Store crosses page, second page VS-stage no W | 4-byte store crosses page, second page VS-stage PTE W=0 | store page-fault (cause=15), stval=original VA |

---

### Group 17: G-stage PTE G Bit Ignored

**Specification Basis**:
- `norm:H_vm_gpa_g`: G bit in G-stage PTEs is currently unused, software should clear for forward compatibility, hardware must ignore

**Test Responsibility**: Verify G-stage PTE G bit being set does not affect translation behavior.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-GBIT-01 | G-stage PTE G=1 does not affect translation | G-stage PTE sets G=1, other permissions normal (V=1,R=1,W=1,X=1,U=1,A=1,D=1) | Two-stage translation succeeds normally, G bit ignored by hardware |

---

### Group 18: henvcfg.PBMTE Control over VS-stage

**Specification Basis**:
- `norm:henvcfg_pbmte_op`: When PBMTE=0, behaves as Svpbmt not implemented (VS-stage PTE PBMT field bit[62:61] is reserved, nonzero triggers exception); when PBMTE=1, Svpbmt available for VS-stage address translation

**Test Responsibility**: Verify PBMTE switch effect on VS-stage PTE PBMT field.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-PBMT-01 | PBMT nonzero triggers fault when PBMTE=0 | henvcfg.PBMTE=0, VS-stage PTE bit[62:61]=01 (NC) | page-fault (reserved bits illegal) |
| TS-PBMT-02 | PBMT=NC translates normally when PBMTE=1 | henvcfg.PBMTE=1, VS-stage PTE bit[62:61]=01 (NC), other permissions normal | Translation succeeds |

---

### Group 19: PMP Interaction with Two-Stage

**Specification Basis**:
- `norm:H_pmp`: Machine-level physical memory protection applies to supervisor physical addresses and is in effect regardless of virtualization mode.

**Test Responsibility**: Verify that after successful two-stage translation, final SPA is still subject to PMP constraints.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-PMP-01 | SPA falls in PMP denied region | Both two-stage translations succeed, final SPA configured as inaccessible by PMP | access-fault (cause=5 load / 7 store), not page-fault nor guest-page-fault |

---

### Group 20: Exception Priority

**Specification Basis**:
- `norm:H_exception_priority`: When multiple synchronous exceptions may trigger simultaneously, decreasing priority order determines which is reported

**Test Responsibility**: Verify exception priority correctness in two-stage translation scenarios.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-PRIO-01 | Misaligned + VS-stage fault priority | VS-mode loads from misaligned address, VS-stage PTE V=0 simultaneously | page-fault (cause=13) prioritized over misaligned (per spec priority table) |
| TS-PRIO-02 | G-stage implicit access interception prioritized over VS-stage PTE check | VS-stage root page table GPA marked invalid by G-stage + VS-stage PTE itself also illegal encoding | guest-page-fault (G-stage implicit root table read intercepted first, VS-stage PTE content not checked) |

---

### Group 21: hgatp Root Page Table Alignment and WARL Behavior

**Specification Basis**:
- `norm:hgatp_ppn_op`: G-stage root page table is 16 KiB and must be aligned to 16 KiB boundary; PPN lowest two bits always read as zero
- `norm:hgatp_mode_warl`: Writing unsupported MODE value is not ignored like satp, but handled as WARL

**Test Responsibility**: Verify hgatp CSR alignment constraints and WARL semantics.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-HGATP-01 | hgatp.PPN[1:0] forced to zero | Write hgatp.PPN lowest 2 bits as 1, read back | PPN[1:0] always reads as 0 (16 KiB alignment WARL enforced constraint, spec explicitly requires; non-compliance is FAIL) |
| TS-HGATP-02 | hgatp MODE WARL | Write hgatp.MODE with unsupported value (e.g., reserved encoding) | WARL behavior (field adjusted, no exception; different from vsatp's "ignore" semantics) |

---

### Group 22: Svinval Instruction Exception Behavior

**Specification Basis**:
- `norm:hstatus_vtvm_op` (original text): "When VTVM=1, an attempt in VS-mode to execute SFENCE.VMA or **SINVAL.VMA** or to access CSR `satp` raises a virtual-instruction exception."
- `norm:mstatus_tvm_hs` (original text): "Setting TVM=1 prevents HS-mode from accessing `hgatp` or executing HFENCE.GVMA or **HINVAL.GVMA**..."

**Test Responsibility**: Verify Svinval extension instruction exception triggering behavior in two-stage scenarios.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TS-SINV-01 | VS executes SINVAL.VMA when VTVM=1 | hstatus.VTVM=1, VS-mode executes `sinval.vma` | virtual-instruction exception (cause=22) |
| TS-SINV-02 | HS executes HINVAL.GVMA when TVM=1 | mstatus.TVM=1, HS-mode executes `hinval.gvma` | illegal-instruction exception (cause=2) |

---

### Group 23: Large Page Granularity Two-Stage Combinations

**Specification Basis**:
- `norm:H_vm_twostage`: During two-stage translation, VS-stage and G-stage leaf nodes can be any supported superpage granularity
- Maximum leaf node granularity supported by VS-stage determined by vsatp MODE: Sv39 max 1G (level 2), Sv48 max 512G (level 3), Sv57 max 256T (level 4)
- Maximum leaf node granularity supported by G-stage determined by hgatp MODE: Sv39x4 max 1G (level 2), Sv48x4 max 512G (level 3), Sv57x4 max 256T (level 4)
- Effective page size of two-stage translation takes the smaller of VS-stage and G-stage leaf node granularities

**Test Responsibility**: Verify two-stage translation correctness when VS-stage and G-stage use different large superpage granularities (1G/512G/256T), supplementing Group 3 which only covers up to 4K/2M (and partial 1G).

| Test ID | Test Name | VS Granularity | G Granularity | VS-stage | G-stage | Test Description | Expected Result |
|---------|-----------|----------------|---------------|----------|---------|------------------|-----------------|
| TS-LP-01 | VS=1G G=4K identity mapping | 1G | 4K | Sv39+ | Sv39x4+ | VS-stage 1GB superpage, G-stage 4KB per-page mapping | R/W success |
| TS-LP-02 | VS=1G G=2M identity mapping | 1G | 2M | Sv39+ | Sv39x4+ | VS-stage 1GB superpage, G-stage 2MB superpage | R/W success |
| TS-LP-03 | VS=1G G=1G identity mapping | 1G | 1G | Sv39+ | Sv39x4+ | Both stages 1GB superpage | R/W success |
| TS-LP-04 | VS=4K G=512G identity mapping | 4K | 512G | Sv39+ | Sv48x4+ | G-stage single 512GB superpage covering all physical memory | R/W success |
| TS-LP-05 | VS=2M G=512G identity mapping | 2M | 512G | Sv39+ | Sv48x4+ | VS-stage 2MB + G-stage 512GB superpage | R/W success |
| TS-LP-06 | VS=1G G=512G identity mapping | 1G | 512G | Sv39+ | Sv48x4+ | VS-stage 1GB + G-stage 512GB superpage | R/W success |
| TS-LP-07 | VS=4K G=256T identity mapping | 4K | 256T | Sv39+ | Sv57x4 | G-stage single 256TB superpage covering all physical memory | R/W success |
| TS-LP-08 | VS=2M G=256T identity mapping | 2M | 256T | Sv39+ | Sv57x4 | VS-stage 2MB + G-stage 256TB superpage | R/W success |
| TS-LP-09 | VS=512G G=4K identity mapping | 512G | 4K | Sv48+ | Sv39x4+ | VS-stage 512GB superpage, G-stage 4KB per-page mapping | R/W success |
| TS-LP-10 | VS=256T G=4K identity mapping | 256T | 4K | Sv57 | Sv39x4+ | VS-stage 256TB superpage, G-stage 4KB per-page mapping | R/W success |

> [!NOTE]
> - TS-LP-04~08 (G=512G/256T): G-stage uses `gpt_map_page(gpa=0, spa=0, level=PT_LEVEL_512G/256T)` to establish single superpage identity mapping covering [0, 512G) or [0, 256T). Since PLATFORM_MEM_BASE (QEMU 0x80000000 / HAPS 0x60000000) falls within this range, mapping is feasible.
> - TS-LP-09/10 (VS=512G/256T): VS-stage uses `pt_map_page(va=0, gpa=0, level=PT_LEVEL_512G/256T)` to establish single superpage identity mapping. Requires `page_table.c`'s `page_size_for_level()` extension to support level 3/4.
> - All test cases use identity mapping (VA=GPA=SPA), success criterion is VS-mode read/write to `test_data_area` returning correct values.
> - 512G/256T superpage requires corresponding hgatp/vsatp MODE support: 512G requires Sv48x4+ or Sv48+ vsatp; 256T requires Sv57x4 or Sv57 vsatp. When unsupported, auto-SKIP via `REQUIRE_HGATP_MODE` / `REQUIRE_VSATP_MODE` macros.

---

## Test Priority

| Priority | Test Groups | Covered Test IDs | Rationale |
|----------|-------------|------------------|-----------|
| P0 (Must) | Group 1, Group 3, Group 6, Group 7, Group 15 | TS-VS-01~10, TS-MAP-01~12, TS-IMPL-01~06, TS-PERM-01~12, TS-AD-01~06 | VS-stage baseline + same-width two-stage + implicit G-stage fault + permission intersection + A/D bit handling |
| P1 (Important) | Group 8, Group 9, Group 10, Group 11, Group 13, Group 16, Group 19 | TS-MXR-01~05, TS-SUM-01~03, TS-HV-01~06, TS-HG-01~06, TS-HLV-01~12, TS-STRD-01~03, TS-PMP-01~02 | MXR/SUM dual-stage semantics, HFENCE flush, HLV/HSV, page boundary straddle, PMP interaction |
| P2 (Recommended) | Group 2, Group 5, Group 12, Group 14, Group 17, Group 18, Group 20, Group 21, Group 22 | TS-VSATP-01~07, TS-NID-01~04, TS-SF-01~04, TS-MPRV-01~05, TS-GBIT-01, TS-PBMT-01~02, TS-PRIO-01~02, TS-HGATP-01~02, TS-SINV-01~02 | vsatp CSR, non-identity mapping, SFENCE V=1, MPRV, G bit, PBMTE, exception priority, hgatp WARL, Svinval exceptions |
| P3 (Optional) | Group 4, Group 23 | TS-XMODE-01~09, TS-LP-01~10 | Cross-width combinations + large page granularity combinations (compatibility verification) |

---

## Test Implementation Notes

### File Organization (9-Directory Split Layout)

Two-stage (VS+G) test cases are extracted from the original `Sv39x4/`, `Sv48x4/`, `Sv57x4/` three G-stage directories, and organized into **9 directories** based on (VS-mode, G-mode) Cartesian product, with each directory independently testing one (VS, G) combination and fully traversing all supported page granularities under that combination.

Adopts "main directory holds + others borrow" model:

- **Main directory** `Sv39_Sv39x4/`: Physically holds all 22 group test `.c` files + 1 new `test_granular_matrix.c` Cartesian product driver file.
- **8 borrowing directories**: `Sv39_Sv48x4/`, `Sv39_Sv57x4/`, `Sv48_Sv39x4/`, `Sv48_Sv48x4/`, `Sv48_Sv57x4/`, `Sv57_Sv39x4/`, `Sv57_Sv48x4/`, `Sv57_Sv57x4/`. Each directory borrows source code via Makefile `CFLAGS += -I../Sv39_Sv39x4/tests`, only customizing `SUITE_VSATP_MODE` / `SUITE_HGATP_MODE` macros in its own `tests/test_register.c`.
- Original three G-stage directories (`Sv39x4/`, `Sv48x4/`, `Sv57x4/`) degrade to **pure G-stage test directories** (retaining 11 G-stage groups: HCSR/ROOT/MAP/HIGH/VALID/RWX/UBIT/AD/ALIGN/GBIT/FAULT), no longer handling two-stage tests.

#### 9-Directory Mapping Table

| Directory | SUITE_VSATP_MODE | SUITE_HGATP_MODE | Cartesian Product Granularity Count |
|-----------|------------------|------------------|-------------------------------------|
| `Sv39_Sv39x4` *(main)* | `SATP_MODE_SV39` | `HGATP_MODE_SV39X4` | 3 x 3 = **9** |
| `Sv39_Sv48x4` | `SATP_MODE_SV39` | `HGATP_MODE_SV48X4` | 3 x 4 = **12** |
| `Sv39_Sv57x4` | `SATP_MODE_SV39` | `HGATP_MODE_SV57X4` | 3 x 5 = **15** |
| `Sv48_Sv39x4` | `SATP_MODE_SV48` | `HGATP_MODE_SV39X4` | 4 x 3 = **12** |
| `Sv48_Sv48x4` | `SATP_MODE_SV48` | `HGATP_MODE_SV48X4` | 4 x 4 = **16** |
| `Sv48_Sv57x4` | `SATP_MODE_SV48` | `HGATP_MODE_SV57X4` | 4 x 5 = **20** |
| `Sv57_Sv39x4` | `SATP_MODE_SV57` | `HGATP_MODE_SV39X4` | 5 x 3 = **15** |
| `Sv57_Sv48x4` | `SATP_MODE_SV57` | `HGATP_MODE_SV48X4` | 5 x 4 = **20** |
| `Sv57_Sv57x4` | `SATP_MODE_SV57` | `HGATP_MODE_SV57X4` | 5 x 5 = **25** |
| **Total Granularity Combinations** | | | **144** |

#### VS x G Granularity Matrix (Full Traversal per Directory)

Each directory runs 25 `MATRIX_CASE` entries in `test_granular_matrix.c` (covering maximum set 5x5), each subject to `vs_max_level(SUITE_VSATP_MODE)` / `g_max_level(SUITE_HGATP_MODE)` boundary auto-SKIP -- legal subset within directory PASSes, out-of-boundary SKIPs.

- **VS levels**: Sv39 -> {4K, 2M, 1G}; Sv48 -> {4K, 2M, 1G, 512G}; Sv57 -> {4K, 2M, 1G, 512G, 256T}
- **G levels**: Sv39x4 -> {4K, 2M, 1G}; Sv48x4 -> {4K, 2M, 1G, 512G}; Sv57x4 -> {4K, 2M, 1G, 512G, 256T}
- **Matrix driver**: Based on framework helpers `vs_max_level()` / `g_max_level()` / `PAGE_SIZE_AT_LEVEL()` (`common/hyp/two_stage_helpers.h`).
- **Underlying API**: Each case calls `ts2_setup_granular(ctx, vs_mode, g_mode, vs_level, g_level)` then `ts2_run_check_no_fault(ctx, test_vs_read_write, va)` to verify R/W.

#### 22 Group Distribution Across 9 Directories

All 9 directories contain all 22 group test files (total ~139 `TEST_REGISTER`) + 1 `test_granular_matrix.c` file (25 `TEST_REGISTER`). Each test case internally auto-SKIPs in non-matching directories via `REQUIRE_VSATP_MODE` / `REQUIRE_HGATP_MODE`, so the test case set is **homologous across all directories**, with inter-directory differences only controlled by `SUITE_*MODE` macro runtime branches.

| Group | File | TEST Count | Primary (VS, G) | Behavior in Other Directories |
|-------|------|------------|-----------------|-------------------------------|
| Group 1 (VS-only) | `test_vs_only.c` | 10 | Determined by SUITE | All run |
| Group 2 (VSATP CSR) | `test_vsatp_csr.c` | 7 | All modes | All run |
| Group 3 (Same width) | `test_same_width.c` | 12 | VS=G diagonal | Non-diagonal SKIP |
| Group 4 (Cross width) | `test_cross_width.c` | 21 | Cross-width combinations | Non-matching SKIP |
| Group 5 (Non-identity) | `test_non_identity.c` | 4 | By SUITE | All run |
| Group 6 (Implicit fault) | `test_implicit_gstage_fault.c` | 4 | By SUITE | All run |
| Group 7 (Perm cross) | `test_perm_cross.c` | 12 | By SUITE | All run |
| Group 8 (MXR) | `test_mxr.c` | 5 | By SUITE | All run |
| Group 9 (SUM) | `test_sum.c` | 3 | By SUITE | All run |
| Group 10 (HFENCE.VVMA) | `test_hfence_vvma.c` | 6 | By SUITE | All run |
| Group 11 (HFENCE.GVMA) | `test_hfence_gvma.c` | 6 | By SUITE | All run |
| Group 12 (SFENCE.VMA) | `test_sfence_vma.c` | 4 | By SUITE | All run |
| Group 13 (HLV/HSV) | `test_hlv_hsv.c` | 12 | By SUITE | All run |
| Group 14 (MPRV+MPV) | `test_mprv_mpv.c` | 5 | By SUITE | All run |
| Group 15 (A/D bits) | `test_ad_two_stage.c` | 6 | By SUITE | All run |
| Group 16 (Page straddle) | `test_page_straddle.c` | 3 | By SUITE | All run |
| Group 17 (G bit) | `test_g_bit.c` | 1 | By SUITE | All run |
| Group 18 (PBMTE) | `test_pbmte.c` | 2 | By SUITE | All run |
| Group 19 (PMP) | `test_pmp.c` | 1 | By SUITE | All run |
| Group 20 (Priority) | `test_priority.c` | 2 | By SUITE | All run |
| Group 21 (HGATP WARL) | `test_hgatp_warl.c` | 2 | All G-modes | All run |
| Group 22 (Svinval) | `test_svinval.c` | 2 | By SUITE | All run |
| Group 23 (Large page) | `test_large_page.c` | 10 | Large page granularity | Non-matching SKIP |
| **New (Granular matrix)** | `test_granular_matrix.c` | 25 | VS x G full Cartesian product | Beyond (vs_max, g_max) SKIP |
| **Total** | 23 files | **~164** | | |

#### Framework Layer Minimal Increment

`common/hyp/two_stage_helpers.h` adds (**helpers only, no `TEST_REGISTER` or assertions**):

- `vs_max_level(int vs_mode)` -- Sv39->1G / Sv48->512G / Sv57->256T
- `g_max_level(int g_mode)` -- Sv39x4->1G / Sv48x4->512G / Sv57x4->256T
- `PAGE_SIZE_AT_LEVEL(level)` macro -- 4K/2M/1G/512G/256T -> byte size

Existing but critical dependencies for this refactoring: `ts2_setup_granular()`, `ts2_map_region_g()`, `ts2_map_region_vs()`, `ts2_run_check_no_fault()`, `ts2_finish()`, all already mode-agnostic (accept `vs_mode`/`g_mode` parameters), no modification needed.

### Common Test Patterns

#### Pattern 1: Two-Stage Translation under VS-mode (Group 1/3/4/5/6/7/8/9)

#### Pattern 2: HFENCE / SFENCE Tests (Group 10/11/12)

#### Pattern 3: HLV/HSV Tests (Group 13)

#### Pattern 4: MPRV+MPV Tests (Group 14)

### VS-mode Test Helper Functions

| Function Name | Purpose | Return Value |
|---------------|---------|--------------|
| `test_vs_read_write` | VS-mode writes magic value and reads back to verify | 0=success |
| `test_vs_load` | VS-mode executes load | 0=success |
| `test_vs_store` | VS-mode executes store | 0=success |
| `test_vs_load_expect_fault` | VS-mode load, expects fault | fault cause |
| `test_vs_store_expect_fault` | VS-mode store, expects fault | fault cause |
| `test_vs_exec_expect_fault` | VS-mode jumps to execute, expects fault | fault cause |

### Key Considerations

1. **Fault cause distinguishes stage source**:
   - VS-stage failure -> cause 12/13/15 (regular page-fault)
   - G-stage failure -> cause 20/21/23 (guest-page-fault)
   - Test assertions must use accurate cause constants, no fuzzy handling

2. **htval dual meaning** (`norm:htval_trapval`):
   - Explicit access fault: htval = original GPA >> 2, corresponds to same access as stval
   - Implicit VS-stage access fault: htval = VS-level PTE's GPA >> 2, does not correspond to same address as stval
   - Determine if implicit access via htinst

3. **htinst pseudoinstruction encoding** (`norm:H_trap_xtinst_guestpage_rw`):
   - RV64 implicit read: `0x00003000`
   - RV64 implicit write (A/D auto-update): `0x00003020`
   - When mtval2/htval is nonzero and is implicit VS-stage access, **must** write pseudoinst (0 not allowed)

4. **MXR dual semantics** (`norm:vsstatus_mxr_vm`, `norm:sstatus_mxr_vm`):
   - HS-level `sstatus.MXR` affects both VS-stage and G-stage
   - `vsstatus.MXR` affects only VS-stage
   - Tests need to set/clear both MXR fields separately to verify differences

5. **SUM only affects VS-stage**:
   - `vsstatus.SUM` controls VS-stage U-bit checking
   - G-stage always U-mode perspective, no SUM concept

6. **A/D bit hardware update control** (`norm:henvcfg_adue_op`):
   - henvcfg.ADUE=0: VS-stage behaves as Svade (A/D=0 triggers page-fault, hardware does not auto-update)
   - henvcfg.ADUE=1: Hardware can auto-update VS-stage PTE A/D bits
   - G-stage PTE A/D bits always require hardware update support

7. **HFENCE instruction semantics**:
   - HFENCE.VVMA: Flushes VS-stage translation cache, valid in M/HS-mode, not affected by TVM/VTVM
   - HFENCE.GVMA: Flushes G-stage translation cache, valid in M-mode or HS-mode with TVM=0
   - SFENCE.VMA under V=1: Flushes VS-stage only, not G-stage

8. **MPRV+MPV two-stage triggering** (`norm:mstatus_mprv_hypervisor`):
   - M-mode MPRV=1 + MPV=1 + MPP=S: VS-level two-stage
   - M-mode MPRV=1 + MPV=1 + MPP=U: VU-level two-stage
   - M-mode MPRV=1 + MPP=M: No translation (direct physical access)
   - MPRV does not affect HLV/HLVX/HSV instructions

9. **Page boundary straddle** (`norm:H_straddle`):
   - Instruction fetch or misaligned access crossing page boundary involves two translations
   - On guest-page-fault, stval may be page boundary address (higher than original VA)
   - htval must correspond to exact virtual address in stval (for non-implicit access)

10. **hgatp alignment and WARL** (`norm:hgatp_ppn_op`, `norm:hgatp_mode_warl`):
    - G-stage root page table is 16 KiB, must be 16 KiB aligned
    - hgatp.PPN[1:0] always read as zero (WARL enforced)
    - Writing unsupported MODE is WARL (not ignored like satp)

11. **G-stage PTE G bit ignored** (`norm:H_vm_gpa_g`):
    - G bit in G-stage PTEs is currently unused
    - Software should clear for forward compatibility
    - Hardware must ignore (setting G=1 does not affect translation)

---

## References

- RISC-V Privileged Specification (Version 20240411)
- RISC-V Hypervisor Extension Specification
- `docs/vm_test_plan.md` (S-mode single-stage translation test plan)
- `common/hyp/two_stage_helpers.h` (Two-stage translation test framework)
- `common/vm/page_table.c` (Page table manipulation utilities)
