**[中文](../testplan/Hypervisor_Sm_test_plan.md) | English**

# Hypervisor × Sm* Extensions Cross Test Plan

> This document describes the test plan for cross-scenarios between the Hypervisor (H) extension and other Sm* (Machine-level) extension families. This plan was split out from `Hypervisor_cross_test_plan.md` and retains only the content at the intersection of the Hypervisor and Sm* extensions. These test scenarios were originally marked in the standalone test plans of the respective extensions as "covered by the Hypervisor test plan" or "excluded due to the absence of the H extension", but analysis showed that the existing Hypervisor test plans do not fully cover them.
>
> Generation date: 2026-06-22

---

## SPEC Sections Covered by This Document

This plan is based on the following official RISC-V specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — Hypervisor (H) extension: hstateen/hcounteren control over VS/VU-mode, virtual-instruction mechanism
- `SPEC/riscv-isa-manual/src/priv/smstateen.adoc` — Smstateen: mstateen access control over hstateen and Hypervisor CSRs
- `SPEC/riscv-isa-manual/src/priv/smcsrind.adoc` — Smcsrind: mstateen0.CSRIND access control over vsiselect/vsireg*
- `SPEC/riscv-isa-manual/src/priv/smctr.adoc` — Smctr: mstateen0.CTR/hstateen0.CTR, MTE external trap recording
- `SPEC/riscv-isa-manual/src/priv/smcntrpmf.adoc` — Smcntrpmf: VSINH/VUINH counting inhibition of mcyclecfg/minstretcfg
- `SPEC/riscv-isa-manual/src/priv/smcdeleg.adoc` — Smcdeleg: when Smstateen and Smcdeleg/Ssccfg are both implemented, access control of `vsiselect`/`vsireg*` by `mstateen0` bit 60
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — PMP (Physical Memory Protection): interaction of PMP with paging (pmp-vmem), PMP constraints on implicit page-table accesses, synchronization requirements after PMP modifications

Official repository:

- https://github.com/riscv/riscv-isa-manual (files at the above paths within the repository)

---

## Scope

### Covered Extension Intersections

- **Hypervisor × Smcsrind**: `mstateen0[60]` (CSRIND) control over S-mode (HS-mode) access to `vsiselect`/`vsireg*`; verification that M-mode access is not controlled by state-enable; including the same-source norm from the Smcdeleg perspective (`norm:smcdeleg_mstateen0_bit60`, migrated in from `Smcdeleg_test_plan.md`)
- **Hypervisor × Smctr**: `hstateen0.CTR` control over VS-mode CTR state access; `mstateen0.CTR=0` blocking of `vsctrctl`; MTE external trap recording behavior from VS/VU-mode to M-mode
- **Hypervisor × Smcntrpmf**: inhibition of VS/VU-mode cycle/instret counting by the VSINH/VUINH bits of `mcyclecfg`/`minstretcfg`; VSINH/VUINH read-only zero when the H extension is not implemented; orthogonality between `hcounteren` and counting inhibition
- **Hypervisor × Smstateen**: `mstateen0` control over hstateen CSR access; mstateen0 zero bit propagation to hstateen; blocking of Hypervisor CSRs by the function bits (SE0/ENVCFG/CSRIND/IMSIC/CONTEXT/P1P13); VS/VU-mode virtual-instruction
- **Hypervisor × PMP**: the final SPA after two-stage translation is still constrained by machine-level PMP (`norm:H_pmp`); PMP constraints on implicit page-table walks (`norm:pmp_with_paging`); synchronization of PMP modifications with G-stage/VS-stage translation caches (HFENCE.GVMA); PMP being the only protection mechanism when hgatp=Bare; HLVX cannot bypass PMP; htval/GVA/SPV reporting semantics on PMP violation traps

### Out of Scope for This Document

- Hypervisor basic functionality already covered by `Hypervisor_CSR_test_plan.md`, `Hypervisor_Interrupts_test_plan.md`, `Hypervisor_Exceptions_test_plan.md`, `Hypervisor_2_stage_test_plan.md`, `Hypervisor_gstage_test_plan.md`
- Behavior of each extension in non-Hypervisor scenarios (covered by their respective standalone test plans)
- Cross tests between the Hypervisor and Ss\*/Sv\*/Z\* extensions (covered by `Hypervisor_Ss_test_plan.md`, `Hypervisor_Sv_test_plan.md`, `Hypervisor_Zi_test_plan.md` respectively)
- Basic functionality and WARL behavior of Smcsrind M-mode CSRs (miselect/mireg\*) — covered by `Smcsrind_test_plan.md`
- `mstateen0[60]` control over S-mode access to siselect/sireg\* (non-H-extension CSRs) — covered by `Smcsrind_test_plan.md` Group 4
- Basic functionality of PMP/Smepmp/Spmp in non-Hypervisor scenarios — covered by `pmp_test_plan.md`, `Smepmp_test_plan.md`, `spmp_test_plan.md` respectively
- The basic scenario of PMP fallback under dual Bare (GBARE-04) and the basic scenario of the SPA after two-stage translation being rejected by PMP (TS-PMP-01) — already covered by `Hypervisor_gstage_test_plan.md` and `Hypervisor_2_stage_test_plan.md`; Group 5 of this plan only supplements the incremental intersection points not covered by them

---

## Covered Specification Points

The following table lists the specification points covered by this plan. Entries with the `norm:` prefix are official SPEC labels; entries without the prefix are specification points decomposed from the SPEC text. Other norm points directly cited in the Spec Reference of each Group (mstateen/hstateen function bits, CTR, external trap recording, etc.) are also within the coverage of this plan and are uniformly listed in the coverage matrix of Appendix A at the end of this document.

| Norm ID | Source | Description |
|---------|--------|-------------|
| `hstateen_sstateen_zero_initialization` | `smstateen.adoc` | After M-mode software modifies any mstateen CSR, it is responsible for initializing the corresponding hstateen and sstateen CSRs to zero. |
| `norm:unimplemented_mode_bits` | `smcntrpmf.adoc` | For each bit in 61:58, if the associated privilege mode is not implemented, the bit is read-only zero. |
| `norm:counter_inhibited_behavior` | `smcntrpmf.adoc` | The fundamental behavior of cycle and instret is modified in that counting does not occur while executing in an inhibited privilege mode. |
| `hcounteren_vs_vu_control` | `hypervisor.adoc` | The `hcounteren` CSR controls availability of performance monitoring counters to VS-mode and VU-mode. |
| `norm:smcdeleg_mstateen0_bit60` | `smcdeleg.adoc` | If extension Smstateen is implemented, setting bit 60 of CSR `mstateen0` to zero prevents access to registers `siselect`, `sireg*`, `vsiselect`, and `vsireg*` from privileged modes less privileged than M-mode. |
| `norm:H_pmp` | `hypervisor.adoc` | Machine-level physical memory protection applies to supervisor physical addresses and is in effect regardless of virtualization mode. |
| `norm:pmp_with_paging` | `machine.adoc` | When paging is enabled, instructions that access virtual memory may result in multiple physical-memory accesses, including implicit references to the page tables. The PMP checks apply to all of these accesses. The effective privilege mode for implicit page-table accesses is S. |
| `norm:pmp_sfence_required` | `machine.adoc` | When the PMP settings are modified, M-mode software must synchronize the PMP settings with the virtual memory system and any PMP or address-translation caches (SFENCE.VMA with rs1=x0 and rs2=x0 after writing PMP CSRs). |
| `hyp_hfence_gvma_pmp_sync` | `hypervisor.adoc` | Executing an HFENCE.GVMA instruction with rs1=x0 and rs2=x0 suffices to flush all G-stage or VS-stage address-translation cache entries that have cached PMP settings corresponding to the final translated supervisor physical address. An HFENCE.VVMA instruction is not required. |
| `norm:hgatp_mode_bare` | `hypervisor.adoc` | When MODE=Bare, guest physical addresses are equal to supervisor physical addresses, and there is no further memory protection for a guest virtual machine beyond the physical memory protection scheme described in PMP. |
| `hlvx_pmp_override` | `hypervisor.adoc` | HLVX cannot override machine-level physical memory protection (PMP), so attempting to read memory that PMP designates as execute-only still results in an access-fault exception. |
| `norm:hlsv_priv` | `hypervisor.adoc` | Each HLV/HLVX/HSV instruction performs an explicit memory access with an effective privilege mode of VS or VU: VU when hstatus.SPVP=0, VS when hstatus.SPVP=1. |
| `norm:hstatus_spv_op` | `hypervisor.adoc` | The SPV bit is written by the implementation whenever a trap is taken into HS-mode, set to the value of virtualization mode V at the time of the trap. |

---

## Group 1. Hypervisor × Smcsrind Cross Tests

**Spec Reference**:
- `norm:sscsrind_csrs_access_control`: If both Smstateen and Smcsrind are implemented, `mstateen0[60]` (CSRIND) controls access to `siselect`, `sireg*`, `vsiselect`, and `vsireg*`. When `mstateen0[60]=0`, accesses to these CSRs from privilege levels lower than M-mode raise an illegal-instruction exception.
- `norm:hypervisor_impl_csrs_access_control`: If the Hypervisor extension is implemented, `hstateen0[60]` is also defined, but controls only VS/VU-mode access to `siselect`/`sireg*` (actually `vsiselect`/`vsireg*`). When `hstateen0[60]=0` and `mstateen0[60]=1`, VS/VU-mode access to `siselect`/`sireg*` raises a virtual-instruction exception (not illegal-instruction).
- `norm:smcdeleg_mstateen0_bit60`: If the Smstateen extension is implemented, setting bit 60 of `mstateen0` to zero prevents access to `siselect`, `sireg*`, `vsiselect`, and `vsireg*` from privilege levels less privileged than M-mode. This is the statement of the same control rule in `smcdeleg.adoc` (same source as `norm:sscsrind_csrs_access_control`); this plan covers its `vsiselect`/`vsireg*` (Hypervisor CSR) portion. Verification of the `siselect`/`sireg*` portion is in `Smcdeleg_test_plan.md` Group 4 and `Smcsrind_test_plan.md` Group 4.

**Test Scope**: Verify CSRIND access control of the Smcsrind extension under Hypervisor scenarios:
- Group 1.1 (01-08): `mstateen0[60]` control over S-mode (HS-mode) access to `vsiselect`/`vsireg*`. M-mode access is not affected by state-enable.
- Group 1.2 (09-11): `hstateen0[60]` control over VS-mode access to `siselect`/`sireg*` (actually `vsiselect`/`vsireg*`). When `hstateen0[60]=0` and `mstateen0[60]=1`, a virtual-instruction exception is raised.

Note: vsiselect/vsireg* are CSRs introduced by the H extension and are only available when the H extension is present.

> **Note**: The tests in this group are extracted from `Smcsrind_test_plan.md` Group 4 and specifically target cases that depend on the H extension. The H extension, the Smcsrind extension, and the Smstateen extension must all be available at the same time.
>
> **Prerequisite configuration**:
> - Group 1.1: M-mode must pre-set `mstateen0[60]` to the desired value to control HS-mode access to vsiselect/vsireg*.
> - Group 1.2: M-mode must pre-set `mstateen0[60]` and `mstateen0[63]` (SE0) to 1 to allow HS-mode access to hstateen0 and the state controlled by CSRIND.

### Test ID Mapping Table

#### Group 1.1: mstateen0[60] Control over S-mode (HS-mode) Access

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| MCSRIND-STA (new) | HCROSS-SMCSRIND-01 | mstateen0[60]=0 blocks S-mode read of vsiselect |
| MCSRIND-STA (new) | HCROSS-SMCSRIND-02 | mstateen0[60]=0 blocks S-mode write of vsiselect |
| MCSRIND-STA (new) | HCROSS-SMCSRIND-03 | mstateen0[60]=0 blocks S-mode read of vsireg |
| MCSRIND-STA (new) | HCROSS-SMCSRIND-04 | mstateen0[60]=0 blocks S-mode read/write of vsireg2~vsireg6 |
| MCSRIND-STA (new) | HCROSS-SMCSRIND-05 | mstateen0[60]=1 allows S-mode access to vsiselect |
| MCSRIND-STA (new) | HCROSS-SMCSRIND-06 | mstateen0[60]=1 allows S-mode access to vsireg* |
| MCSRIND-STA-07 (supplementary) | HCROSS-SMCSRIND-07 | mstateen0[60]=0 does not affect M-mode access to vsiselect |
| MCSRIND-STA-08 (supplementary) | HCROSS-SMCSRIND-08 | mstateen0[60]=0 does not affect M-mode access to vsireg* |

#### Group 1.2: hstateen0[60] Control over VS-mode Access

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| HCROSS-SMCSRIND-09 (supplementary) | HCROSS-SMCSRIND-09 | hstateen0[60]=0 blocks VS-mode read of siselect |
| HCROSS-SMCSRIND-10 (supplementary) | HCROSS-SMCSRIND-10 | hstateen0[60]=0 blocks VS-mode read of sireg |
| HCROSS-SMCSRIND-11 (supplementary) | HCROSS-SMCSRIND-11 | hstateen0[60]=1 allows VS-mode access to siselect/sireg |

### Test Case List

#### Group 1.1: mstateen0[60] Control over S-mode (HS-mode) Access

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SMCSRIND-01 | mstateen0[60]=0 blocks S-mode read of vsiselect | mstateen0[60]=0, S-mode (HS-mode) reads vsiselect (0x250) | illegal-instruction exception raised (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-02 | mstateen0[60]=0 blocks S-mode write of vsiselect | mstateen0[60]=0, S-mode writes vsiselect | illegal-instruction exception raised (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-03 | mstateen0[60]=0 blocks S-mode read of vsireg | mstateen0[60]=0, S-mode reads vsireg (0x251) | illegal-instruction exception raised (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-04 | mstateen0[60]=0 blocks S-mode read/write of vsireg2~vsireg6 | mstateen0[60]=0, S-mode reads/writes vsireg2~vsireg6 one by one | each access raises an illegal-instruction exception (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-05 | mstateen0[60]=1 allows S-mode access to vsiselect | mstateen0[60]=1, S-mode reads/writes vsiselect | access succeeds, no exception | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-06 | mstateen0[60]=1 allows S-mode access to vsireg* | mstateen0[60]=1, S-mode reads/writes vsireg~vsireg6 | access is not blocked by mstateen0 (vsireg2-6 may raise illegal-instruction due to implementation optionality) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-07 | mstateen0[60]=0 does not affect M-mode access to vsiselect | mstateen0[60]=0, M-mode reads/writes vsiselect | access succeeds, no exception (state-enable does not control M-mode) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-08 | mstateen0[60]=0 does not affect M-mode access to vsireg* | mstateen0[60]=0, M-mode reads/writes vsireg~vsireg6 | access succeeds, no exception | `norm:sscsrind_csrs_access_control` |

#### Group 1.2: hstateen0[60] Control over VS-mode Access

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SMCSRIND-09 | hstateen0[60]=0 blocks VS-mode read of siselect | mstateen0[60]=1, hstateen0[60]=0, VS-mode reads siselect (0x150, actually vsiselect) | virtual-instruction exception raised (cause=22) | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SMCSRIND-10 | hstateen0[60]=0 blocks VS-mode read of sireg | mstateen0[60]=1, hstateen0[60]=0, VS-mode reads sireg (0x151, actually vsireg) | virtual-instruction exception raised (cause=22) | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SMCSRIND-11 | hstateen0[60]=1 allows VS-mode access to siselect/sireg | mstateen0[60]=1, hstateen0[60]=1, VS-mode writes siselect and reads sireg | siselect access succeeds; sireg is not blocked by hstateen0 | `norm:hypervisor_impl_csrs_access_control` |

> [!NOTE]
> - **Group 1.1** verifies the `mstateen0[60]` access control of the Smcsrind extension under Hypervisor scenarios. vsiselect/vsireg* are CSRs introduced by the H extension (vsiselect=0x250, vsireg=0x251, vsireg2=0x252, vsireg3=0x253, vsireg4=0x255, vsireg5=0x256, vsireg6=0x257) and are only available when the H extension is present.
> - HCROSS-SMCSRIND-01~04 verify that with `mstateen0[60]=0`, S-mode (HS-mode, V=0) access to vsiselect/vsireg* raises an illegal-instruction exception (cause=2). 04 covers both read and write operations. This is symmetric with the behavior of S-mode access to siselect/sireg* (covered in `Smcsrind_test_plan.md` Group 4).
> - HCROSS-SMCSRIND-05~06 verify that with `mstateen0[60]=1`, S-mode can access vsiselect/vsireg* normally. Note: vsireg2-6 are optionally implemented CSRs; when the implementation does not support them, an illegal-instruction may be raised (allowed by the SPEC), and the test outputs the trap cause in a diagnostic manner.
> - HCROSS-SMCSRIND-07~08 verify that state-enable CSRs do **not** affect M-mode's own accesses. This is explicitly stated by the SPEC: state-enable CSRs only affect privilege levels lower than M-mode.
> - **Group 1.2** verifies the `hstateen0[60]` control over VS-mode access to siselect/sireg* (actually vsiselect/vsireg*). Difference from Group 1.1: in Group 1.1, `mstateen0[60]` controls HS-mode (V=0) access and raises illegal-instruction (cause=2); in Group 1.2, `hstateen0[60]` controls VS-mode (V=1) access and raises virtual-instruction (cause=22). The two are control mechanisms at different levels, with spec references `norm:sscsrind_csrs_access_control` and `norm:hypervisor_impl_csrs_access_control` respectively.
> - HCROSS-SMCSRIND-09~10 verify that with `hstateen0[60]=0` and `mstateen0[60]=1`, VS-mode access to siselect/sireg raises a virtual-instruction exception. Note: the exception type is virtual-instruction rather than illegal-instruction, because M-mode has already granted access (mstateen0=1), but the hypervisor in HS-mode chooses not to grant it (hstateen0=0).
> - HCROSS-SMCSRIND-11 verifies that with `hstateen0[60]=1`, VS-mode can access siselect/sireg. sireg access may raise other exceptions depending on the vsiselect value, but must not raise a virtual-instruction.
> - Group 1.2 prerequisite configuration: M-mode must set `mstateen0[63]` (SE0) to 1 to allow HS-mode access to hstateen0, and set `mstateen0[60]` (CSRIND) to 1 to allow the state controlled by CSRIND.
> - Relationship with Ssstateen Group 4.4 (HCROSS-SSSTA-27~29) in `Hypervisor_Ss_test_plan.md`: both verify the same `hstateen0[60]` control behavior, but the Ssstateen group verifies it from the Ssstateen perspective, while Group 1.2 of this group verifies it from the Smcsrind perspective. Cross-references can be used during implementation.
> - All tests must detect the availability of the H extension, the Smcsrind extension, and the Smstateen extension at runtime; if any is unavailable, TEST_SKIP. Group 1.2 must also detect the writability of `hstateen0.CSRIND`.

---

## Group 2. Hypervisor × Smctr Cross Tests

**Spec Reference**:
- `norm:hstateen_ctr`: If the H extension is implemented and `mstateen0.CTR=1`, the `hstateen0.CTR` bit controls access to supervisor CTR state when V=1; when `mstateen0.CTR=0`, `hstateen0.CTR` is read-only zero
- `norm:hstateen_vs`: When `hstateen0.CTR=0`, VS-mode access to CTR state and SCTRCLR raises a virtual-instruction exception
- `norm:hstateen0_CTR0-V1_op`: When `hstateen0.CTR=0`, qualified control transfers during V=1 still implicitly update the entry registers and `sctrstatus`
- `norm:mstateen_ctr0_except1`: `mstateen0.CTR=0` blocks access to `vsctrctl`
- `norm:exttrap_vsm`: External traps from VS-mode to M-mode require MTE + STE
- `norm:exttrap_vum`: External traps from VU-mode to M-mode require MTE + STE + vsctrctl.STE
- `norm:exttrap_implreq`: If the H extension is implemented, `vsctrctl.STE` must be implemented

**Test Scope**: Verify the behavior of the Smctr extension under Hypervisor scenarios, including `hstateen0.CTR` control over VS-mode CTR state access, `mstateen0.CTR=0` blocking of `vsctrctl`, and MTE external trap recording behavior from VS/VU-mode to M-mode.

> **Note**: The tests in this group are extracted from `Smctr_test_plan.md` Groups 2/3 and specifically target cases that depend on the H extension. The H extension and the Smctr extension must both be available.

### Test ID Mapping Table

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| SMCTR-STA-05 | HCROSS-SMCTR-01 | mstateen0.CTR=0 blocks S-mode access to vsctrctl |
| SMCTR-STA-11 | HCROSS-SMCTR-02 | hstateen0.CTR read/write verification |
| SMCTR-STA-12 | HCROSS-SMCTR-03 | hstateen0.CTR read-only zero (mstateen0.CTR=0) |
| SMCTR-STA-13 | HCROSS-SMCTR-04 | hstateen0.CTR=0 blocks VS-mode access to sctrctl |
| SMCTR-STA-14 | HCROSS-SMCTR-05 | hstateen0.CTR=0 blocks VS-mode access to sctrstatus |
| SMCTR-STA-15 | HCROSS-SMCTR-06 | hstateen0.CTR=0 blocks VS-mode access to sireg* |
| SMCTR-STA-16 | HCROSS-SMCTR-07 | hstateen0.CTR=0 blocks VS-mode execution of SCTRCLR |
| SMCTR-STA-17 | HCROSS-SMCTR-08 | hstateen0.CTR=1 allows full VS-mode access |
| SMCTR-STA-18 | HCROSS-SMCTR-09 | Implicit updates continue with hstateen0.CTR=0 |
| SMCTR-MODE-07 | HCROSS-SMCTR-10 | MTE external trap recording (VS→M) |
| SMCTR-MODE-08 | HCROSS-SMCTR-11 | MTE external trap recording (VU→M, requires MTE+STE+vsSTE) |
| SMCTR-MODE-09 | HCROSS-SMCTR-12 | External trap VU→M not recorded without vsSTE |

### Test Case List

#### 2.1 mstateen0.CTR Control over Hypervisor CSRs

**Spec Reference**: `norm:mstateen_ctr0_except1`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SMCTR-01 | mstateen0.CTR=0 blocks S-mode access to vsctrctl | mstateen0.CTR=0, HS-mode attempts to read vsctrctl | illegal-instruction exception raised (cause=2) | `norm:mstateen_ctr0_except1` |

#### 2.2 hstateen0.CTR Access Control

**Spec Reference**: `norm:hstateen_ctr`, `norm:hstateen_vs`, `norm:hstateen0_CTR0-V1_op`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SMCTR-02 | hstateen0.CTR read/write verification | mstateen0.CTR=1, M-mode writes hstateen0.CTR=1 then reads back | hstateen0.CTR is writable if the H extension is implemented and mstateen0.CTR=1 | `norm:hstateen_ctr` |
| HCROSS-SMCTR-03 | hstateen0.CTR read-only zero (mstateen0.CTR=0) | mstateen0.CTR=0, attempt to write hstateen0.CTR=1 | hstateen0.CTR is read-only zero | `norm:hstateen_ctr` |
| HCROSS-SMCTR-04 | hstateen0.CTR=0 blocks VS-mode access to sctrctl | mstateen0.CTR=1, hstateen0.CTR=0, VS-mode accesses sctrctl (actually vsctrctl) | virtual-instruction exception raised (cause=22) | `norm:hstateen_vs` |
| HCROSS-SMCTR-05 | hstateen0.CTR=0 blocks VS-mode access to sctrstatus | mstateen0.CTR=1, hstateen0.CTR=0, VS-mode accesses sctrstatus | virtual-instruction exception raised (cause=22) | `norm:hstateen_vs` |
| HCROSS-SMCTR-06 | hstateen0.CTR=0 blocks VS-mode access to sireg* | mstateen0.CTR=1, hstateen0.CTR=0, VS-mode sets siselect=0x200 then reads sireg | virtual-instruction exception raised (cause=22) | `norm:hstateen_vs` |
| HCROSS-SMCTR-07 | hstateen0.CTR=0 blocks VS-mode execution of SCTRCLR | mstateen0.CTR=1, hstateen0.CTR=0, VS-mode executes SCTRCLR | virtual-instruction exception raised (cause=22) | `norm:hstateen_vs` |
| HCROSS-SMCTR-08 | hstateen0.CTR=1 allows full VS-mode access | mstateen0.CTR=1, hstateen0.CTR=1, VS-mode accesses sctrctl/sctrstatus respectively | all accesses succeed | `norm:hstateen_vs` |
| HCROSS-SMCTR-09 | Implicit updates continue with hstateen0.CTR=0 | mstateen0.CTR=1, hstateen0.CTR=0, VS-mode enables recording and performs control transfers, M-mode checks the entry registers | the entry registers and sctrstatus are still implicitly updated | `norm:hstateen0_CTR0-V1_op` |

#### 2.3 MTE External Traps (VS/VU-mode → M-mode)

**Spec Reference**: `norm:exttrap_vsm`, `norm:exttrap_vum`, `norm:exttrap_implreq`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SMCTR-10 | MTE external trap recording (VS→M) | mctrctl.M=0, MTE=1, sctrctl.STE=1, VS-mode raises a trap to M-mode | the external trap is recorded (requires MTE and STE) | `norm:exttrap_vsm` |
| HCROSS-SMCTR-11 | MTE external trap recording (VU→M, requires MTE+STE+vsSTE) | mctrctl.M=0, MTE=1, sctrctl.STE=1, vsctrctl.STE=1, VU-mode raises a trap to M-mode | the external trap is recorded (requires all three TE bits set) | `norm:exttrap_vum` |
| HCROSS-SMCTR-12 | External trap VU→M not recorded without vsSTE | mctrctl.M=0, MTE=1, STE=1, vsctrctl.STE=0, VU-mode raises a trap to M-mode | the external trap is not recorded (vsctrctl.STE not set) | `norm:exttrap_vum` |

> [!NOTE]
> - The tests in this group verify the behavior of the Smctr extension under Hypervisor scenarios. All tests must detect the availability of the H extension at runtime via `HAS_H_EXT()`; if unavailable, TEST_SKIP.
> - HCROSS-SMCTR-01 is migrated from `Smctr_test_plan.md` Group 2 and verifies the blocking of `vsctrctl` (a CSR introduced by the H extension) by `mstateen0.CTR=0`. The exception raised is illegal-instruction (cause=2), because this is M-mode's control over S-mode.
> - HCROSS-SMCTR-02~09 are migrated from `Smctr_test_plan.md` Group 2 and verify the `hstateen0.CTR` control over VS-mode CTR state access. Core rule: with `hstateen0.CTR=0`, VS-mode access to CTR state raises virtual-instruction (cause=22), not illegal-instruction. This differs from the illegal-instruction raised with `mstateen0.CTR=0` — because M-mode has already granted access (mstateen0=1), but HS-mode chooses not to grant it (hstateen0=0).
> - HCROSS-SMCTR-09 verifies the key semantics: even when `hstateen0.CTR=0` blocks VS-mode's **explicit access** to CTR CSRs, qualified control transfers executed during V=1 still **implicitly update** the entry registers and `sctrstatus`. This prevents the hypervisor from interfering with the guest's transfer recording by disabling CTR.
> - HCROSS-SMCTR-10~12 are migrated from `Smctr_test_plan.md` Group 3 and verify MTE external trap recording under Hypervisor scenarios. VS→M requires MTE+STE; VU→M requires MTE+STE+vsctrctl.STE (all three TE bits). This reflects the dependency of external trap recording on the TE bits of intermediate modes.
> - Difference from Group 4 (HCROSS-SMSTA): Group 4 verifies the function bits of `hstateen0` such as SE0/ENVCFG/CSRIND, while this group verifies the `hstateen0.CTR` bit's control over CTR state.

---

## Group 3. Hypervisor × Smcntrpmf Cross Tests

The tests in this group verify the behavior of the Smcntrpmf (Cycle and Instret Privilege Mode Filtering) extension under Hypervisor scenarios, namely the inhibition effect of the VSINH/VUINH bits of `mcyclecfg`/`minstretcfg` on VS/VU-mode counting, and the interaction with `hcounteren`. These tests are migrated from `Smcntrpmf_test_plan.md` and specifically target cases that depend on the H extension.

**Spec Reference**:
- `norm:unimplemented_mode_bits`: For bits 61:58 of `mcyclecfg`/`minstretcfg`, if the associated privilege mode is not implemented, the bit is read-only zero
- `norm:counter_inhibited_behavior`: Counting does not occur while executing in an inhibited privilege mode
- `hcounteren_vs_vu_control`: `hcounteren` controls availability of performance monitoring counters in VS/VU-mode

**Test Scope**: Verify the inhibition effect of the VSINH/VUINH bits on cycle/instret counting in VS/VU-mode, the read-only zero behavior when the H extension is not implemented, and the orthogonality between `hcounteren` access control and counting inhibition.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| PMF-CSR-05 | VSINH/VUINH read-only zero when H extension is not implemented | If the H extension is not implemented, write VSINH/VUINH=1 to `mcyclecfg`/`minstretcfg` and read back | VSINH/VUINH are read-only zero |
| PMF-CYC-08 | VSINH=1 inhibits VS-mode cycle counting | Set `mcyclecfg.VSINH=1`, VS-mode executes a fixed loop, read the cycle delta | cycle does not increment in VS-mode |
| PMF-CYC-09 | VUINH=1 inhibits VU-mode cycle counting | Set `mcyclecfg.VUINH=1`, VU-mode executes a fixed loop, read the cycle delta | cycle does not increment in VU-mode |
| PMF-INS-06 | VSINH=1 inhibits VS-mode instret counting | Set `minstretcfg.VSINH=1`, VS-mode executes N instructions, read the instret delta | instret does not increment in VS-mode |
| PMF-INS-07 | VUINH=1 inhibits VU-mode instret counting | Set `minstretcfg.VUINH=1`, VU-mode executes N instructions, read the instret delta | instret does not increment in VU-mode |
| PMF-CTR-04 | VS-mode cannot read cycle with hcounteren.CY=0 | `hcounteren.CY=0`, VS-mode reads cycle | virtual-instruction exception (cause=22) |
| HCROSS-PMF-01 | VSINH inhibition is orthogonal to hcounteren | `mcyclecfg.VSINH=1`, `hcounteren.CY=1`, VS-mode executes a loop then reads cycle | cycle is readable but the value does not increment (access allowed but counting inhibited) |

> [!NOTE]
> - PMF-CSR-05 verifies the read-only zero behavior of VSINH/VUINH when the H extension is **not implemented** (`norm:unimplemented_mode_bits`); on platforms with the H extension implemented, this case should TEST_SKIP, and the writability and functionality of VSINH/VUINH are verified by PMF-CYC-08/09 and PMF-INS-06/07.
> - PMF-CYC-08/09 and PMF-INS-06/07 need to execute counting loops in VS/VU-mode to verify the inhibition effect of VSINH/VUINH. The VSINH/VUINH bits share the same bit encoding (bit 59/58) with the `mhpmevent` fields of Sscofpmf.
> - PMF-CTR-04 verifies the `hcounteren` control over VS-mode counter access: with `hcounteren.CY=0`, VS-mode reading cycle raises a virtual-instruction exception (cause=22), not illegal-instruction.
> - HCROSS-PMF-01 verifies the orthogonality between counting inhibition (VSINH) and access control (`hcounteren`/`mcounteren`): the two take effect independently; when access is allowed but the mode is inhibited, the counter is readable but does not increment.

### Test ID Mapping Table

| Original ID | New Location | Test Name |
|-------------|--------------|-----------|
| PMF-CSR-05 | Group 3 | VSINH/VUINH read-only zero when H extension is not implemented |
| PMF-CYC-08 | Group 3 | VSINH=1 inhibits VS-mode cycle counting |
| PMF-CYC-09 | Group 3 | VUINH=1 inhibits VU-mode cycle counting |
| PMF-INS-06 | Group 3 | VSINH=1 inhibits VS-mode instret counting |
| PMF-INS-07 | Group 3 | VUINH=1 inhibits VU-mode instret counting |
| PMF-CTR-04 | Group 3 | VS-mode cannot read cycle with hcounteren.CY=0 |
| — (new) | Group 3 | HCROSS-PMF-01 VSINH inhibition is orthogonal to hcounteren |

### Implementation Notes

1. **H extension detection**: Before testing, the availability of the H extension must be detected at runtime via `HAS_H_EXT()` (misa.H). PMF-CYC-08/09, PMF-INS-06/07, PMF-CTR-04, and HCROSS-PMF-01 TEST_SKIP when the H extension is unavailable; PMF-CSR-05 runs only when the H extension is **not implemented** (verifying read-only zero), and TEST_SKIP when the H extension is implemented.

2. **Smcntrpmf detection**: It must first be probed whether `mcyclecfg` (CSR 0x321)/`minstretcfg` (CSR 0x322) are implemented (trap-protected write-read of the MINH bit); if not implemented, the entire group TEST_SKIP.

3. **VS/VU-mode switching**: The `ENABLE_HYP` macro must be enabled at compile time, `goto_priv(PRIV_VS)`/`goto_priv(PRIV_VU)` is used to switch virtual privilege levels, and two-stage translation (`hgatp`/`vsatp`) must be configured so that VS/VU-mode can execute the counting loops.

4. **Counter reading**: VS/VU-mode reading of `cycle`/`instret` (CSR 0xC00/0xC02) requires the corresponding bits in both `mcounteren` and `hcounteren` to be enabled; to verify the inhibition effect, it is recommended to read `mcycle`/`minstret` in M-mode before and after the mode switch and take the delta, avoiding C function calls in VS/VU-mode.

5. **Orthogonality of counting inhibition and access control**: VSINH/VUINH inhibit counter incrementing, while `hcounteren`/`mcounteren` control counter readability; the two are independent. HCROSS-PMF-01 must ensure `mcounteren.CY=1` and `hcounteren.CY=1`, then verify that counting does not increment with VSINH=1.

---

## Group 4. Hypervisor × Smstateen

The tests in this group verify the behavior of the Smstateen extension under Hypervisor scenarios, including hstateen CSR access control and HS-mode/VS-mode/VU-mode privilege level interactions. These tests are migrated from `smstateen_test_plan.md` and specifically target cases that depend on the H extension.

### Test ID Mapping Table

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| MSTA-INIT-05 | HCROSS-SMSTA-01 | hstateen0 initialization after reset |
| MSTA-PROP-02 | HCROSS-SMSTA-02 | mstateen0 zero bit propagation to hstateen0 |
| MSTA-B63-03 | HCROSS-SMSTA-03 | SE0=0 blocks HS-mode hstateen0 |
| MSTA-B63-07 | HCROSS-SMSTA-04 | bit 63 writability conditions |
| MSTA-SE0-02 | HCROSS-SMSTA-05 | SE0=0 blocks hstateen0 |
| MSTA-SE0-03 | HCROSS-SMSTA-06 | SE0=0 blocks hstateen0h (RV32) |
| MSTA-ENVCFG-02 | HCROSS-SMSTA-07 | ENVCFG=0 blocks henvcfg |
| MSTA-CSRIND-03 | HCROSS-SMSTA-08 | CSRIND=0 blocks vsiselect |
| MSTA-IMSIC-02 | HCROSS-SMSTA-09 | IMSIC=0 blocks vstopei |
| MSTA-CTX-02 | HCROSS-SMSTA-10 | CONTEXT=0 blocks hcontext |
| MSTA-P1P13-01 | HCROSS-SMSTA-11 | P1P13=0 blocks hedelegh |
| MSTA-P1P13-02 | HCROSS-SMSTA-12 | P1P13=1 allows hedelegh |
| MSTA-EXC-04 | HCROSS-SMSTA-13 | VS-mode virtual-instruction |
| MSTA-EXC-05 | HCROSS-SMSTA-14 | VU-mode virtual-instruction |

### Test Case List

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SMSTA-01 | hstateen0 initialization after reset | After M-mode sets some mstateen0 bits to 1, write zero to hstateen0 | hstateen0 reads back zero | `hstateen_sstateen_zero_initialization` |
| HCROSS-SMSTA-02 | mstateen0 zero bit propagation to hstateen0 | Set a function bit of mstateen0 to 0, then attempt to write that bit to hstateen0 in HS-mode | the corresponding bit of hstateen0 reads back 0 | `norm:mstateen_lower_priv_roz` |
| HCROSS-SMSTA-03 | SE0=0 blocks HS-mode hstateen0 | Set mstateen0 bit 63 to 0, then access hstateen0 in HS-mode | illegal-instruction exception raised | `norm:mstateen_bit_63_op` |
| HCROSS-SMSTA-04 | bit 63 writability conditions | Check whether mstateen0 bit 63 is writable (H extension present or sstateen not all read-only zero) | writable when the conditions are met, otherwise RO0 | `norm:mstateen_bit_63_roz` |
| HCROSS-SMSTA-05 | SE0=0 blocks hstateen0 | Set mstateen0.SE0=0, HS-mode reads hstateen0 | illegal-instruction exception raised | `norm:mstateen0_se0_op` |
| HCROSS-SMSTA-06 | SE0=0 blocks hstateen0h (RV32) | Set mstateen0.SE0=0, HS-mode reads hstateen0h | illegal-instruction exception raised | `norm:mstateen0_se0_op` |
| HCROSS-SMSTA-07 | ENVCFG=0 blocks henvcfg | Set ENVCFG=0, HS-mode reads henvcfg | illegal-instruction exception raised | `norm:mstateen0_envcfg_op` |
| HCROSS-SMSTA-08 | CSRIND=0 blocks vsiselect | Set CSRIND=0, HS-mode reads vsiselect | illegal-instruction exception raised | `norm:mstateen0_csrind_op` |
| HCROSS-SMSTA-09 | IMSIC=0 blocks vstopei | Set IMSIC=0, HS-mode reads vstopei | illegal-instruction exception raised | `norm:mstateen0_imsic_op` |
| HCROSS-SMSTA-10 | CONTEXT=0 blocks hcontext | Set CONTEXT=0, HS-mode reads hcontext | illegal-instruction exception raised | `norm:mstateen0_context_op` |
| HCROSS-SMSTA-11 | P1P13=0 blocks hedelegh | Set P1P13=0, HS-mode reads hedelegh | illegal-instruction exception raised | `norm:mstateen0_p1p13_op` |
| HCROSS-SMSTA-12 | P1P13=1 allows hedelegh | Set P1P13=1, HS-mode reads hedelegh | access succeeds | `norm:mstateen0_p1p13_op` |
| HCROSS-SMSTA-13 | VS-mode virtual-instruction | Some mstateen0 bit=0 and accessed from VS-mode, satisfying the virtual-instruction exception conditions | virtual-instruction exception raised (cause=22) | `norm:stateen_illegal_state_access` |
| HCROSS-SMSTA-14 | VU-mode virtual-instruction | Some mstateen0 bit=0 and accessed from VU-mode, satisfying the virtual-instruction exception conditions | virtual-instruction exception raised (cause=22) | `norm:stateen_illegal_state_access` |

### Key Considerations

1. **H extension detection**: All tests must detect the availability of the H extension at runtime via `HAS_H_EXT()`; if unavailable, TEST_SKIP.

2. **Relationship between HS-mode and S-mode**: When the H extension is present and V=0, S-mode is actually HS-mode. The tests use `goto_priv(PRIV_S)` to emulate HS-mode access.

3. **VS-mode/VU-mode tests**: The `ENABLE_HYP` macro must be enabled at compile time, and `goto_priv(PRIV_VS)` or `goto_priv(PRIV_VU)` is used to switch to virtual privilege levels.

4. **Distinguishing virtual-instruction from illegal-instruction**: When VS/VU-mode accesses a controlled CSR, if hstateen allows but mstateen blocks, a virtual-instruction (cause=22) should be raised; if hstateen also blocks, an illegal-instruction (cause=2) is raised.

5. **hstateen CSR addresses**: The CSR addresses of hstateen0-3 are 0x60C-0x60F, and hstateen0h-3h (RV32) are 0x61C-0x61F.

---

## Group 5. Hypervisor × PMP Cross Tests

**Spec Reference**:
- `norm:H_pmp`: machine-level physical memory protection applies to supervisor physical addresses, regardless of virtualization mode (still in effect when V=1). That is, after VS/VU-mode accesses obtain the final SPA via two-stage translation, they must still pass the machine-level PMP check.
- `norm:pmp_with_paging`: after paging is enabled, instructions that access virtual memory generate multiple physical accesses, including implicit references to page tables, and PMP checks apply to all of these accesses; the effective privilege mode of implicit page-table accesses is S. Therefore, under the H extension, both VS-stage and G-stage page-table walks are constrained by PMP.
- `norm:pmp_sfence_required`: after modifying PMP settings, M-mode software must synchronize the PMP settings with the virtual memory system and translation caches (execute SFENCE.VMA with rs1=x0, rs2=x0 after writing PMP CSRs).
- `hyp_hfence_gvma_pmp_sync`: when the H extension is implemented, synchronization with G-stage/VS-stage data structures is equally necessary; executing HFENCE.GVMA (rs1=x0, rs2=x0) suffices to flush all G-stage/VS-stage translation cache entries that have cached the PMP settings corresponding to the final SPA; HFENCE.VVMA is not required.
- `norm:hgatp_mode_bare`: when hgatp.MODE=Bare, GPA==SPA, and the guest virtual machine has no memory protection mechanism other than PMP.
- `hlvx_pmp_override`: HLVX cannot bypass machine-level PMP; reading memory that PMP designates as execute-only still raises an access-fault exception.

**Test Scope**: Verify the intersection behavior of machine-level PMP under Hypervisor scenarios:
- Group 5.1 (01~04): after two-stage translation succeeds, when the final SPA is rejected by PMP, explicit VS/VU-mode accesses (load/store/fetch) raise an access fault rather than a page fault/guest page fault.
- Group 5.2 (05~06): PMP constraints on implicit page-table walks (VS-stage, G-stage walks); a PMP violation always raises an access fault (`norm:pmp_access_fault_exception`); `norm:H_vm_gpatrans` only converts page faults to guest page faults and does not cover access faults.
- Group 5.3 (07): after modifying PMP settings, the necessity of HFENCE.GVMA synchronization for G-stage/VS-stage translation caches that have cached PMP attributes.
- Group 5.4 (08): HLVX cannot bypass PMP (execute-only regions still raise an access fault).

> **Note**: This group covers only the incremental intersection points not covered by existing plans. The basic scenario of PMP fallback under dual Bare (GBARE-04) is covered by `Hypervisor_gstage_test_plan.md`; the basic load/store scenario of the SPA being rejected by PMP after two-stage translation (TS-PMP-01) is covered by `Hypervisor_2_stage_test_plan.md` Group 19; this group supplements the incremental dimensions of fetch, VU-mode, implicit walks, fence synchronization, HLVX, etc. on top of it.
>
> **Prerequisite configuration**:
> - The H extension and PMP (at least one entry available) must both be implemented; probed at runtime, TEST_SKIP if either is unavailable.
> - By default, configure two-stage translation (both `vsatp` and `hgatp` non-Bare); the VS-stage and G-stage PTE permissions of the test page both grant access, and only PMP controls the target region.
> - The PMP check targets the final SPA; before testing, the target GPA→SPA mapping must be clarified, and the PMP entry must be configured with the SPA.

### Test Case List

#### Group 5.1: PMP Constraints on the SPA after Two-Stage Translation (Explicit Accesses)

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-PMP-01 | VS-mode load rejected by PMP | Both stages of translation grant access; the target final SPA is configured as inaccessible by PMP (no matching entry or R=0), VS-mode load | load access fault (cause=5), and not within {13, 21} (not a page fault/guest page fault) | `norm:H_pmp` |
| HCROSS-PMP-02 | VS-mode store rejected by PMP | Same configuration, VS-mode store | store access fault (cause=7), and not within {15, 23} | `norm:H_pmp` |
| HCROSS-PMP-03 | VS-mode fetch rejected by PMP | Both stages of translation grant access; the final SPA of the target code page is configured as X=0 by PMP, VS-mode jumps to that page for execution | instruction access fault (cause=1), and not within {12, 20}; when the trap is not delegated, it is delivered to M-mode reporting `mstatus.MPV==1` | `norm:H_pmp` |
| HCROSS-PMP-04 | VU-mode access rejected by PMP | Same configuration as 01, VU-mode load | load access fault (cause=5); if this cause is delegated to HS (`hedeleg` corresponding bit set), the trap is delivered to HS-mode with `hstatus.SPV==1` | `norm:H_pmp` |

#### Group 5.2: PMP Constraints on Implicit Page-Table Walks

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-PMP-05 | VS-stage page-table walk rejected by PMP | Both stages of translation page-table permissions grant access; configure the physical page hosting the VS-stage page table (after G-stage translation) as PMP-inaccessible, VS-mode access triggers a walk | the VS-mode access reports load access fault (cause=5): after the implicit read succeeds via G-stage translation, the PMP check failure is a physical protection violation (not a translation failure), and no guest page fault is raised; after restoring PMP, the access succeeds | `norm:pmp_with_paging`, `norm:H_pmp` |
| HCROSS-PMP-06 | G-stage page-table walk rejected by PMP | Configure the SPA hosting the G-stage page table (hgatp root/intermediate-level page tables) as PMP-inaccessible, with `hgatp` active in HS-mode execute HLV to trigger a G-stage walk | the access reports load access fault (cause=5) (the failure occurs at the physical protection check during translation); accept and diagnose cases where the implementation reports load guest page fault (cause=21) because the walk failure point is inside translation; after restoring PMP, the access succeeds | `norm:pmp_with_paging`, `norm:H_pmp` |

#### Group 5.3: PMP Modification and Translation Cache Synchronization (HFENCE.GVMA)

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-PMP-07 | HFENCE.GVMA synchronization after tightening PMP | Initially both two-stage translation and PMP grant access, VS-mode access succeeds (translation cache takes effect); M-mode tightens PMP to reject the target SPA, executes SFENCE.VMA (x0,x0) and HFENCE.GVMA (x0,x0) in sequence, then re-enters VS-mode to access | the second access reports an access fault (load cause=5), proving that the translation cache entries caching the old PMP attributes have been flushed | `norm:pmp_sfence_required`, `hyp_hfence_gvma_pmp_sync` |

#### Group 5.4: HLVX and PMP

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-PMP-08 | HLVX cannot bypass PMP execute-only | The target page is marked X=1, U=1 in both VS-stage and G-stage page tables, HLVX executes with VU effective privilege (`hstatus.SPVP`=0); PMP configures the final SPA of that page as X only (R=0, a non-locked entry with MML=0 effective for S/U levels); first verify HLVX succeeds with PMP fully granted (sanity), then verify HLVX fails under X-only, and after restoring PMP the same access succeeds again (attribution isolation) | under X-only PMP, HLVX reports load access fault (cause=5); the sanity and isolation steps both succeed | `hlvx_pmp_override` |

> [!NOTE]
> - The key assertion of HCROSS-PMP-01~04 is the exception **type**: a PMP violation raises an access fault (1/5/7) rather than a page fault (12/13/15) or guest page fault (20/21/23), because two-stage translation has already succeeded and the failure occurs at the post-translation physical address protection check stage.
> - The key difference between HCROSS-PMP-05/06 and explicit accesses lies in the failure **location** (implicit page-table reads during translation) rather than the exception type: a PMP violation always raises an access fault (`norm:pmp_access_fault_exception`); `norm:H_vm_gpatrans` only converts page faults to guest page faults and does not cover access faults. If the implementation reports a guest page fault, it violates the SPEC (in 06 the walk failure point is inside the G-stage translation machinery, and the specification does not explicitly constrain its reporting form; cause=21 is permitted but must be diagnosed).
> - The PMP rejection target of HCROSS-PMP-05 is the **final physical address** of the VS-stage page table after G-stage translation (the implicit access itself first goes through G-stage translation); configuration must use the SPA where the page table actually resides, not the GPA.
> - HCROSS-PMP-06 uses HS-mode HLV (hgatp active, V=0) to trigger a G-stage walk instead of an explicit VS-mode access: G-stage intermediate-level page tables and the kernel image usually reside in the same 1GB window; rejecting that page in VS-mode would make the trampoline fetch entering VS fault immediately and be unrecoverable; HLV triggers the same G-stage walk path, and the trap can recover normally in the HS/M context.
> - HCROSS-PMP-07 verifies the synchronization obligation under the premise that translation caches may cache PMP attributes (permitted by `norm:pmp_speculative_translations`): without HFENCE.GVMA (or any equivalent synchronization), the implementation is permitted to continue using the old cached translation (including the old PMP attributes), so this case must assert "must fail after synchronization is completed", and must not assert "fails when not synchronized".
> - HCROSS-PMP-08 adopts VU effective privilege (SPVP=0) + two-level U=1 page tables, so that all page-table permission checks pass and PMP becomes the only possible rejection source (`norm:hlsv_priv`); attribution isolation adopts the three-step comparison of "HLVX succeeds with PMP fully granted (sanity) → X-only rejection → succeeds again after restoration", rather than a VS-mode fetch comparison (U=1 page tables forbid S-level fetches and are unusable).
> - All PMP violation cases must assert the virtualization origin of the trap context: when not delegated, the trap is delivered to M-mode; `hstatus.SPV` is written by hardware only when trapping into HS-mode (`norm:hstatus_spv_op`), so under M-mode delivery the assertion must use `mstatus.MPV==1`; only when delegated to HS is the assertion `hstatus.SPV==1`. Assertions on htval/GVA are limited to the cases explicitly specified by the SPEC (reporting guest physical address information on guest page faults); no mandatory assertions are made on the implementation choice of GVA/htval under access faults.
> - After modifying any PMP CSR, the test itself must also execute SFENCE.VMA (x0,x0) (and HFENCE.GVMA when needed) to synchronize the translation caches of the test environment itself.
> - Runtime probe items: H extension (`HAS_H_EXT()`), PMP entry count and granularity (trap-protected probing of pmpcfg/pmpaddr), two-stage translation availability; TEST_SKIP if any is not satisfied. If the platform has Smepmp enabled (mseccfg.MML=1) and the default rules cannot be restored, the PMP permission semantics change, and the cases in this group should TEST_SKIP with diagnostics.
> - Intersection analysis of PMP with Smepmp/Spmp: Smepmp (mseccfg.MML/MMWP/RLB) redefines S/U-level access rules and applies transitively to VS/VU accesses and implicit page-table walks per `norm:H_pmp`, but the SPEC has no explicit H×Smepmp normative clause, so it is not covered in this group; Spmp is not a standard extension included in the SPEC of this repository, has no normative basis, and no case is created.
> - Implementation suggestion: place it in an independent `test_hyp_pmp.c` under `Sv39x4_Sv39` (or the corresponding two-stage translation test directory), reusing the PMP configuration API of the `pmp/` module and the framework trap assertion mechanism; cases must fail explicitly (ASSERT) and continue execution, and on failure report by comparing with the SPEC rather than adapting to the implementation.

---

## Test Priorities

| Priority | Test Group | Covered Test IDs | Rationale |
|----------|------------|------------------|-----------|
| P1 (Important) | Group 4 (Smstateen) | HCROSS-SMSTA-01~14 | hstateen control and VS/VU-mode exception behavior are key guarantees of Hypervisor state isolation |
| P1 (Important) | Group 1 (Smcsrind) | HCROSS-SMCSRIND-01~08 | mstateen0[60] access control over virtualization CSRs (vsiselect/vsireg*) is a key guarantee of security isolation |
| P1 (Important) | Group 2.2 (hstateen0.CTR) | HCROSS-SMCTR-02~09 | hstateen0.CTR control over VS-mode CTR access is a guarantee of virtualization state isolation |
| P2 (Recommended) | Group 2.1 (mstateen0.CTR Hyp) | HCROSS-SMCTR-01 | mstateen0.CTR blocking of vsctrctl |
| P2 (Recommended) | Group 2.3 (MTE Hyp) | HCROSS-SMCTR-10~12 | MTE external trap recording behavior in VS/VU-mode |
| P1 (Important) | Group 5.1/5.2 (PMP intersection) | HCROSS-PMP-01~06 | `norm:H_pmp` and `norm:pmp_with_paging` are the last line of defense of guest memory isolation; the exception type distinction between explicit accesses and implicit page-table walks is a frequent implementation error point |
| P2 (Recommended) | Group 5.3/5.4 (PMP synchronization and HLVX) | HCROSS-PMP-07~08 | translation cache PMP attribute synchronization and HLVX boundary behavior; dependent on implementation cache structures and relatively platform-specific |

> Note: The test cases of Smcntrpmf (Group 3) (PMF-CSR-05, PMF-CYC-08/09, PMF-INS-06/07, PMF-CTR-04, HCROSS-PMF-01) were not assigned individual priorities in the original merged plan; it is recommended to follow the priorities in `Smcntrpmf_test_plan.md`.

---

## Key Considerations

1. **Extension detection**: All tests must detect the availability of the required extensions (H, Smcsrind, Smctr, Smcntrpmf, Smstateen, etc.) at runtime; if unavailable, TEST_SKIP.

2. **state-enable hierarchy control**: `mstateen` controls HS-mode and below's access to extension state (raising illegal-instruction), while `hstateen` controls VS/VU-mode access (raising virtual-instruction). The two operate independently; when M-mode grants access (mstateen=1) but HS-mode does not (hstateen=0), VS/VU-mode raises virtual-instruction (cause=22).

3. **M-mode is not controlled by state-enable**: state-enable CSRs only affect privilege levels lower than M-mode; M-mode's own accesses are not affected.

4. **Distinguishing virtual-instruction from illegal-instruction**: Test assertions must use accurate cause constants.

5. **Exception type distinction of the PMP intersection**: regardless of explicit accesses or implicit page-table walks, PMP violations report access faults (1/5/7); guest page faults (20/21/23) arise only from translation failures themselves (invalid page-table entries/insufficient permissions), and the page fault→guest page fault conversion of `norm:H_vm_gpatrans` does not cover access faults. After modifying PMP, synchronization fences (SFENCE.VMA and HFENCE.GVMA) must be executed per the specification.

---

## References

- `SPEC/hypervisor.adoc` — RISC-V Hypervisor Extension, Version 1.0
- `SPEC/smstateen.adoc` — Smstateen Extension Specification
- `SPEC/smcsrind.adoc` — Smcsrind/Sscsrind Extension for Indirect CSR Access
- `SPEC/smctr.adoc` — Smctr (Control Transfer Records - Machine-level) Extension
- `SPEC/smcntrpmf.adoc` — Smcntrpmf (Cycle and Instret Privilege Mode Filtering) Extension
- `SPEC/smcdeleg.adoc` — Smcdeleg and Ssccfg Counter Delegation Extensions
- `SPEC/machine.adoc` — Machine-Level ISA (including the PMP chapter and pmp-vmem: interaction of PMP with paging)
- `SPEC/pmp.adoc` — Physical Memory Protection (chapter within machine.adoc)
- `DOCS/testplan/Smcsrind_test_plan.md` — Smcsrind Machine Mode test plan
- `DOCS/testplan/Smctr_test_plan.md` — Smctr Machine Mode test plan
- `DOCS/testplan/Smcntrpmf_test_plan.md` — Smcntrpmf standalone test plan
- `DOCS/testplan/smstateen_test_plan.md` — Smstateen standalone test plan
- `DOCS/testplan/Smcdeleg_test_plan.md` — Smcdeleg extension test plan (Machine Mode)
- `DOCS/testplan/pmp_test_plan.md` — PMP standalone test plan (non-Hypervisor scenarios)
- `DOCS/testplan/Smepmp_test_plan.md` — Smepmp standalone test plan (non-Hypervisor scenarios)
- `DOCS/testplan/Hypervisor_CSR_test_plan.md` — Hypervisor CSR subset test plan
- `DOCS/testplan/Hypervisor_Interrupts_test_plan.md` — Hypervisor interrupts subset test plan
- `DOCS/testplan/Hypervisor_Exceptions_test_plan.md` — Hypervisor exceptions and trap subset test plan
- `DOCS/testplan/Hypervisor_2_stage_test_plan.md` — Two-stage translation test plan
- `DOCS/testplan/Hypervisor_gstage_test_plan.md` — G-stage standalone test plan
- `ideas/hypervisor_gap.md` — Hypervisor test gap analysis

---

## Appendix A: Specification Point Coverage Matrix

The following table indicates which test cases cover each specification point in the "Covered Specification Points" section. Only the norm points directly cited by each Group of this plan are listed; some norm points listed in the main table (such as `hstateen_sstateen_zero_initialization` and `hcounteren_vs_vu_control`) are foundational norm points shared by multiple Groups.

| Norm ID | Covered Test IDs |
|---------|------------------|
| `hstateen_sstateen_zero_initialization` (self-decomposed) | HCROSS-SMSTA-01 |
| `norm:mstateen_lower_priv_roz` | HCROSS-SMSTA-02 |
| `norm:mstateen_bit_63_op` | HCROSS-SMSTA-03 |
| `norm:mstateen_bit_63_roz` | HCROSS-SMSTA-04 |
| `norm:mstateen0_se0_op` | HCROSS-SMSTA-05, HCROSS-SMSTA-06 |
| `norm:mstateen0_envcfg_op` | HCROSS-SMSTA-07 |
| `norm:mstateen0_csrind_op` | HCROSS-SMSTA-08 |
| `norm:mstateen0_imsic_op` | HCROSS-SMSTA-09 |
| `norm:mstateen0_context_op` | HCROSS-SMSTA-10 |
| `norm:mstateen0_p1p13_op` | HCROSS-SMSTA-11, HCROSS-SMSTA-12 |
| `norm:stateen_illegal_state_access` | HCROSS-SMSTA-13, HCROSS-SMSTA-14 |
| `norm:sscsrind_csrs_access_control` | HCROSS-SMCSRIND-01~08 (see the Sscsrind Group of `Hypervisor_Ss_test_plan.md` for the same-source verification from the VS-mode perspective) |
| `norm:smcdeleg_mstateen0_bit60` | HCROSS-SMCSRIND-01~08 (same source as `norm:sscsrind_csrs_access_control`; migrated in from `Smcdeleg_test_plan.md`, and this plan covers its `vsiselect`/`vsireg*` portion) |
| `norm:hypervisor_impl_csrs_access_control` | HCROSS-SMCSRIND-09~11 (see the Sscsrind Group of `Hypervisor_Ss_test_plan.md` for the same-source verification from the VS-mode perspective) |
| `norm:mstateen_ctr0_except1` | HCROSS-SMCTR-01 |
| `norm:hstateen_ctr` | HCROSS-SMCTR-02, HCROSS-SMCTR-03, HCROSS-SMCTR-04~09 |
| `norm:hstateen_vs` | HCROSS-SMCTR-04~08 (see the Ssctr Group of `Hypervisor_Ss_test_plan.md` for the same-source verification from the VS-mode perspective) |
| `norm:hstateen0_CTR0-V1_op` | HCROSS-SMCTR-09 |
| `norm:exttrap_vsm` | HCROSS-SMCTR-10 |
| `norm:exttrap_vum` | HCROSS-SMCTR-11, HCROSS-SMCTR-12 |
| `norm:exttrap_implreq` | HCROSS-SMCTR-10~12 (vsctrctl.STE implementation prerequisite) |
| `norm:unimplemented_mode_bits` | PMF-CSR-05 |
| `norm:counter_inhibited_behavior` | PMF-CYC-08, PMF-CYC-09, PMF-INS-06, PMF-INS-07, HCROSS-PMF-01 |
| `hcounteren_vs_vu_control` (self-decomposed) | PMF-CTR-04, HCROSS-PMF-01 |
| `norm:H_pmp` | HCROSS-PMP-01~06 (see GBARE-04 of `Hypervisor_gstage_test_plan.md` and TS-PMP-01 of `Hypervisor_2_stage_test_plan.md` for the basic scenarios) |
| `norm:pmp_with_paging` | HCROSS-PMP-05, HCROSS-PMP-06 |
| `norm:pmp_sfence_required` | HCROSS-PMP-07 |
| `hyp_hfence_gvma_pmp_sync` | HCROSS-PMP-07 |
| `norm:hgatp_mode_bare` | — (the basic scenarios are covered by GBARE-04/05 of `Hypervisor_gstage_test_plan.md`; this plan does not create duplicate cases) |
| `hlvx_pmp_override` | HCROSS-PMP-08 |
| `norm:hlsv_priv` | HCROSS-PMP-08 (VU effective privilege premise) |
| `norm:hstatus_spv_op` | the SPV/MPV assertion premise of all PMP violation cases in Group 5.1/5.2 |
