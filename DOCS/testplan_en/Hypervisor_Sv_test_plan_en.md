**[中文](../testplan/Hypervisor_Sv_test_plan.md) | English**

# Hypervisor × Sv* Extensions Cross Test Plan

> This document describes the test plan for cross-scenarios between the Hypervisor (H) extension and other Sv* (Supervisor virtual address translation related) extension families.

---

## SPEC Sections Covered by This Document

This plan is based on the following official RISC-V specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — Hypervisor (H) extension: henvcfg.ADUE, HLV/HSV, HFENCE.GVMA, VS/VU-mode virtual-instruction mechanism
- `SPEC/riscv-isa-manual/src/priv/svadu.adoc` — Svadu: hardware A/D bit updates and henvcfg.ADUE
- `SPEC/riscv-isa-manual/src/priv/svinval.adoc` — Svinval: HINVAL.VVMA/GVMA, SFENCE.W.INVAL/SFENCE.INVAL.IR
- `SPEC/riscv-isa-manual/src/priv/svnapot.adoc` — Svnapot: NAPOT PTEs and G-stage support
- `SPEC/riscv-isa-manual/src/priv/svpbmt.adoc` — Svpbmt: PBMT override rules in two-stage translation

Official repository:

- https://github.com/riscv/riscv-isa-manual (files at the above paths within the repository)

---

## Scope

### Covered Extension Intersections

- **Hypervisor × Svadu**: `henvcfg.ADUE` writability, HLV/HSV interaction with hardware A/D updates, HFENCE.GVMA synchronization after modifying `menvcfg.ADUE`
- **Hypervisor × Svinval**: HINVAL.VVMA/GVMA instruction functionality, VMID replacing ASID, VS/VU-mode virtual-instruction triggering
- **Hypervisor × Svnapot**: NAPOT PTE support in G-stage translation, reserved-encoding fault, simultaneous NAPOT usage in both stages
- **Hypervisor × Svpbmt**: superimposed override behavior of PBMT attributes in two-stage address translation, G-stage/VS-stage PBMT override rules

### Out of Scope for This Document

- Hypervisor basic functionality already covered by `Hypervisor_CSR_test_plan.md`, `Hypervisor_Interrupts_test_plan.md`, `Hypervisor_Exceptions_test_plan.md`, `Hypervisor_2_stage_test_plan.md`, `Hypervisor_gstage_test_plan.md`
- Behavior of each extension in non-Hypervisor scenarios (covered by their respective standalone test plans)
- Cross tests between the Hypervisor and Ss\*/Sm\*/Z\* extensions (covered by `Hypervisor_Ss_test_plan.md`, `Hypervisor_Sm_test_plan.md`, `Hypervisor_Zi_test_plan.md` respectively)

---

## Covered Specification Points

The following table lists the specification points covered by this plan. Entries with the `norm:` prefix are official SPEC normative rule labels; entries without the prefix are specification points summarized by this plan based on the SPEC text.

| Norm ID | Source | Description |
|---------|--------|-------------|
| `norm:henvcfg_adue_op` | `hypervisor.adoc` | If the Svadu extension is implemented, the ADUE bit controls whether hardware updating of PTE A/D bits is enabled for VS-stage address translation. When ADUE=1, hardware updating is enabled. When ADUE=0, the implementation behaves as though Svade were implemented for VS-stage address translation. If Svadu is not implemented, ADUE is read-only zero. |
| `norm:Svadu_hypervisor_adue_writable` | `svadu.adoc` | When Svadu is implemented, `henvcfg.ADUE` must be writable. |
| `svadu_hfence_gvma_sync` | `hypervisor.adoc` | After modifying `menvcfg.ADUE`, a `HFENCE.GVMA(x0,x0)` is required to synchronize the change across all VMIDs. |
| `norm:Svinval_hinval_vvma_gvma` | `svinval.adoc` | HINVAL.VVMA and HINVAL.GVMA have the same semantics as SINVAL.VMA, except that they combine with SFENCE.W.INVAL and SFENCE.INVAL.IR to replace HFENCE.VVMA and HFENCE.GVMA, respectively. |
| `norm:Svinval_hinval_gvma_uses_vmid` | `svinval.adoc` | HINVAL.GVMA uses VMIDs instead of ASIDs. |
| `norm:Svinval_virtual_instruction_vu_vs` | `svinval.adoc` | An attempt to execute HINVAL.VVMA or HINVAL.GVMA in VS-mode or VU-mode, or to execute SINVAL.VMA in VU-mode, raises a virtual-instruction exception. |
| `norm:Svinval_sfence_w_inval_inval_vu_mode` | `svinval.adoc` | An attempt to execute SFENCE.W.INVAL or SFENCE.INVAL.IR in VU-mode raises a virtual-instruction exception. |
| `norm:Svnapot_hyp_gstage` | `svnapot.adoc` | If the Hypervisor extension is also implemented, Svnapot is supported in G-stage translation. |
| `norm:Svpbmt_hgatp_stage_override_rule` | `svpbmt.adoc` | When `hgatp.MODE` is not Bare, a nonzero PBMT field in a G-stage leaf PTE overrides the PMA to produce intermediate memory attributes. |
| `norm:Svpbmt_vsatp_stage_override_rule` | `svpbmt.adoc` | When `vsatp.MODE` is not Bare, a nonzero PBMT field in a VS-stage leaf PTE overrides the intermediate memory attributes to produce the final memory attributes. |

---

## Group 1. Hypervisor × Svadu Cross Tests

**Spec Reference**:
- `norm:henvcfg_adue_op`: The ADUE bit controls hardware updating of VS-stage A/D bits
- `norm:Svadu_hypervisor_adue_writable`: `henvcfg.ADUE` must be writable
- `svadu_hfence_gvma_sync`: HFENCE.GVMA synchronization is required after modifying `menvcfg.ADUE`

**Test Scope**: Verify CSR writability, HLV/HSV instruction interaction, and cross-VMID synchronization behavior of the Svadu extension under the Hypervisor two-stage translation scenario.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SVADU-01 | henvcfg.ADUE writability verification | HS-mode writes henvcfg.ADUE=1 and reads back; then writes ADUE=0 and reads back | If Svadu is implemented, ADUE is writable and reads back consistently; if Svadu is not implemented, ADUE is read-only zero |
| HCROSS-SVADU-02 | HLV instruction interaction with Svadu (ADUE=1) | henvcfg.ADUE=1, VS-stage PTE A=0, HS-mode executes HLV.D to read that GPA | access succeeds, the VS-stage PTE A bit is automatically set to 1 by hardware (implicit accesses triggered by HLV also follow ADUE control) |
| HCROSS-SVADU-03 | HSV instruction interaction with Svadu (ADUE=1) | henvcfg.ADUE=1, VS-stage PTE A=1,D=0, HS-mode executes HSV.D to write that GPA | access succeeds, the VS-stage PTE D bit is automatically set to 1 by hardware |
| HCROSS-SVADU-04 | HLV instruction interaction with Svade (ADUE=0) | henvcfg.ADUE=0, VS-stage PTE A=0, HS-mode executes HLV.D to read that GPA | page-fault (cause=13), hardware does not automatically update the A bit (behaves as Svade). Note: VS-stage translation exceptions produce page-fault (cause=13), not guest-page-fault (cause=21). guest-page-fault is only used for G-stage translation exceptions (norm:H_vm_gpatrans) |
| HCROSS-SVADU-05 | HFENCE.GVMA synchronization after modifying menvcfg.ADUE | Change menvcfg.ADUE from 0 to 1, do not execute HFENCE.GVMA, VS-mode accesses a page with A=0; then execute HFENCE.GVMA(x0,x0) and repeat the access | the first access may still follow the old behavior (implementation defined); after HFENCE.GVMA the behavior must follow the new ADUE value (A bit updated by hardware) |
| HCROSS-SVADU-06 | Specific-VMID synchronization after modifying menvcfg.ADUE | Change menvcfg.ADUE from 1 to 0, execute HFENCE.GVMA(vmid, x0) for a specific VMID only, verify the behavior of that VMID and other VMIDs | the behavior of the specified VMID must follow the new ADUE value; the behavior of other VMIDs is implementation defined (may still follow the old value) |

> [!NOTE]
> - HCROSS-SVADU-02~04 require executing HLV/HSV instructions in HS-mode, verifying that the A/D update behavior of implicit accesses (page-table walks) is also controlled by `henvcfg.ADUE`.
> - In HCROSS-SVADU-04~07, VS-stage translation exceptions (A=0 + Svade) produce **page-fault (cause=13)**, not guest-page-fault (cause=21). According to norm:H_vm_gpatrans, guest-page-fault is only used for **G-stage** translation exceptions; VS-stage translation exceptions use the regular page-fault cause code.
> - HCROSS-SVADU-05~06 verify the synchronization semantics after a `menvcfg.ADUE` change. `HFENCE.GVMA(x0,x0)` flushes all VMIDs, while `HFENCE.GVMA(vmid,x0)` flushes only the specific VMID.
> - If the platform does not implement the Svadu extension, HCROSS-SVADU-01 should verify that ADUE is read-only zero, and HCROSS-SVADU-02~07 should TEST_SKIP.

---

## Group 2. Hypervisor × Svinval Cross Tests

**Spec Reference**:
- `norm:Svinval_hinval_vvma_gvma`: HINVAL.VVMA/GVMA instruction functionality
- `norm:Svinval_hinval_gvma_uses_vmid`: HINVAL.GVMA uses VMIDs instead of ASIDs
- `norm:Svinval_virtual_instruction_vu_vs`: VS/VU-mode execution of HINVAL raises virtual-instruction
- `norm:Svinval_sfence_w_inval_inval_vu_mode`: VU-mode execution of SFENCE.W.INVAL/SFENCE.INVAL.IR raises virtual-instruction

**Test Scope**: Verify the functionality of the Svinval extension's HINVAL.VVMA/GVMA instructions under Hypervisor scenarios, VMID semantics, and the exception-triggering behavior in VS/VU-mode.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SINVAL-01 | HINVAL.VVMA basic functionality | HS-mode modifies a VS-stage PTE, executes HINVAL.VVMA(va, asid), VS-mode verifies the new PTE takes effect | VS-mode access to va uses the new PTE (HINVAL.VVMA correctly flushes the VS-stage TLB) |
| HCROSS-SINVAL-02 | HINVAL.GVMA basic functionality | HS-mode modifies a G-stage PTE, executes HINVAL.GVMA(gpa, vmid), VS-mode verifies the new PTE takes effect | VS-mode access to the corresponding GPA uses the new PTE (HINVAL.GVMA correctly flushes the G-stage TLB) |
| HCROSS-SINVAL-03 | HINVAL.VVMA combined with SFENCE.W.INVAL/SFENCE.INVAL.IR | HS-mode modifies multiple VS-stage PTEs, executes multiple HINVAL.VVMA, then executes SFENCE.W.INVAL + SFENCE.INVAL.IR, VS-mode verifies all new PTEs take effect | all modified PTEs take effect (combined semantics correct) |
| HCROSS-SINVAL-04 | HINVAL.GVMA combined with SFENCE.W.INVAL/SFENCE.INVAL.IR | HS-mode modifies multiple G-stage PTEs, executes multiple HINVAL.GVMA, then executes SFENCE.W.INVAL + SFENCE.INVAL.IR, VS-mode verifies all new PTEs take effect | all modified PTEs take effect (combined semantics correct) |
| HCROSS-SINVAL-05 | HINVAL.GVMA uses VMID (specific-VMID flush) | HS-mode modifies a G-stage PTE, executes HINVAL.GVMA(gpa, vmid=5), verifies the TLB of VMID=5 is flushed and the TLB of VMID=6 is not | accesses of VMID=5 use the new PTE; accesses of VMID=6 may still use the old PTE (implementation defined) |
| HCROSS-SINVAL-06 | HINVAL.GVMA uses VMID=0 (all-VMID flush) | HS-mode modifies a G-stage PTE, executes HINVAL.GVMA(gpa, vmid=0), verifies the TLB of all VMIDs is flushed | accesses of all VMIDs use the new PTE |
| HCROSS-SINVAL-07 | VS-mode execution of HINVAL.VVMA raises virtual-instruction | VS-mode executes HINVAL.VVMA | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-08 | VS-mode execution of HINVAL.GVMA raises virtual-instruction | VS-mode executes HINVAL.GVMA | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-09 | VU-mode execution of HINVAL.VVMA raises virtual-instruction | VU-mode executes HINVAL.VVMA | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-10 | VU-mode execution of HINVAL.GVMA raises virtual-instruction | VU-mode executes HINVAL.GVMA | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-11 | VU-mode execution of SFENCE.W.INVAL raises virtual-instruction | VU-mode executes SFENCE.W.INVAL | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-12 | VU-mode execution of SFENCE.INVAL.IR raises virtual-instruction | VU-mode executes SFENCE.INVAL.IR | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-13 | VS-mode execution of SFENCE.W.INVAL succeeds (VTVM=0) | hstatus.VTVM=0, VS-mode executes SFENCE.W.INVAL | executes normally, no exception (SFENCE.W.INVAL is not controlled by VTVM) |
| HCROSS-SINVAL-14 | VS-mode execution of SFENCE.INVAL.IR succeeds (VTVM=0) | hstatus.VTVM=0, VS-mode executes SFENCE.INVAL.IR | executes normally, no exception |
| HCROSS-SINVAL-15 | VU-mode execution of SINVAL.VMA raises virtual-instruction | VU-mode executes SINVAL.VMA | virtual-instruction exception (cause=22) |

> [!NOTE]
> - HINVAL.VVMA/GVMA are fine-grained TLB flush instructions provided by the Svinval extension for Hypervisor scenarios, functionally equivalent to HFENCE.VVMA/GVMA but supporting batch-flush optimization.
> - HCROSS-SINVAL-05~06 verify the VMID semantics of HINVAL.GVMA: a nonzero VMID flushes only the TLB of that specific VMID, while VMID=0 flushes all VMIDs. This is consistent with the semantics of HFENCE.GVMA.
> - HCROSS-SINVAL-07~12 verify the virtual-instruction exception triggering when VS/VU-mode execute HINVAL and SFENCE.W.INVAL/SFENCE.INVAL.IR; this is a key guarantee of Hypervisor security isolation.
> - HCROSS-SINVAL-15 verifies that VU-mode execution of SINVAL.VMA raises a virtual-instruction exception, consistent with the exception behavior of HINVAL.VVMA/GVMA in VU-mode, fully covering the `norm:Svinval_virtual_instruction_vu_vs` specification.
> - Difference from Group 22 of `Hypervisor_2_stage_test_plan.md`: Group 22 only covers exception triggering of SINVAL.VMA with VTVM=1 and HINVAL.GVMA with TVM=1 (2 cases); this group supplements HINVAL instruction functionality, VMID semantics, and full virtual-instruction coverage in VS/VU-mode (15 cases).

---

## Group 3. Hypervisor × Svnapot Cross Tests

**Spec Reference**:
- `norm:Svnapot_hyp_gstage`: If the Hypervisor extension is also implemented, Svnapot is supported in G-stage translation

**Test Scope**: Verify the behavior of Svnapot in G-stage translation, including basic translation of G-stage NAPOT PTEs, reserved-encoding exceptions, and two-stage translation correctness when both VS-stage and G-stage use NAPOT simultaneously.

> **Note**: The tests in this group are migrated from `Svnapot_test_plan.md` Group 10. The H extension and the Svnapot extension must both be available.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SVNAPOT-01 | G-stage 64 KiB NAPOT basic translation | Configure a 64 KiB NAPOT PTE in the G-stage page table | GPA→SPA translation is correct |
| HCROSS-SVNAPOT-02 | G-stage NAPOT reserved-encoding fault | G-stage NAPOT PTE uses a reserved encoding | guest page-fault |
| HCROSS-SVNAPOT-03 | Both G-stage and VS-stage use NAPOT simultaneously | Both VS-stage and G-stage use NAPOT PTEs | two-stage translation is correct |

> [!NOTE]
> - The tests in this group verify the behavior of the Svnapot extension in Hypervisor G-stage translation. The Svnapot specification explicitly states that if the Hypervisor extension is also implemented, NAPOT translation is equally supported in G-stage page tables.
> - HCROSS-SVNAPOT-01 verifies the basic GPA→SPA translation functionality of 64 KiB NAPOT PTEs in G-stage, complementing the regular PTE tests in `Hypervisor_gstage_test_plan.md`.
> - HCROSS-SVNAPOT-02 verifies that when a G-stage NAPOT PTE uses a reserved encoding (low 4 bits of ppn[0] not `1000` and N=1), the hardware should raise a guest-page-fault. This is consistent with the reserved-encoding behavior in VS-stage (see `Svnapot_test_plan.md` Group 3).
> - HCROSS-SVNAPOT-03 verifies the scenario where both VS-stage and G-stage use NAPOT PTEs in two-stage translation: VS-stage maps GVA→GPA using NAPOT, G-stage maps GPA→SPA also using NAPOT, and the final GVA→SPA translation should be correct.

---

## Group 4. Hypervisor × Svpbmt Cross Tests

**Spec Reference**:
- `norm:Svpbmt_hgatp_stage_override_rule`: When `hgatp.MODE` is not Bare, the nonzero PBMT field of the G-stage PTE overrides the PMA to produce intermediate attributes
- `norm:Svpbmt_vsatp_stage_override_rule`: When `vsatp.MODE` is not Bare, the nonzero PBMT field of the VS-stage PTE overrides the intermediate attributes to produce the final attributes

**Test Scope**: Verify the superimposed override behavior of PBMT attributes in two-stage address translation, including G-stage PBMT overriding PMA, VS-stage PBMT overriding intermediate attributes, two-stage superposition, and the scenario where G-stage override is skipped when `hgatp.MODE=0`.

> **Note**: The tests in this group are migrated from `Svpbmt_test_plan.md` Group 10. The H extension and the Svpbmt extension must both be available.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SVPBMT-01 | G-stage PBMT=NC overrides PMA | hgatp.MODE nonzero, G-stage PTE sets PBMT=NC, VS-stage PTE PBMT=0 (no override). Logic chain: G-stage PBMT=NC overrides PMA → intermediate=NC; VS-stage PBMT=0 does not trigger override → final=NC | final attribute is NC |
| HCROSS-SVPBMT-02 | VS-stage PBMT=IO overrides intermediate attributes | hgatp.MODE nonzero, G-stage PTE PBMT=0 (no override) → intermediate=PMA; VS-stage PTE PBMT=IO overrides intermediate → final=IO | final attribute is IO |
| HCROSS-SVPBMT-03 | Both stages nonzero superposition | G-stage PTE PBMT=NC → intermediate=NC; VS-stage PTE PBMT=IO overrides intermediate → final=IO | final attribute is IO |
| HCROSS-SVPBMT-04 | hgatp.MODE=0 skips G-stage | hgatp.MODE=0 → G-stage inactive, intermediate=PMA; VS-stage PTE PBMT=NC overrides intermediate → final=NC | final attribute is NC |

> [!NOTE]
> - The tests in this group verify the PBMT attribute superimposition and override rules of the Svpbmt extension in Hypervisor two-stage translation. The Svpbmt specification defines a two-stage override chain: PMA → G-stage PBMT override → intermediate → VS-stage PBMT override → final.
> - HCROSS-SVPBMT-01 verifies that G-stage PBMT overrides PMA to produce intermediate attributes, while VS-stage PBMT=0 does not trigger a secondary override, so the final attribute remains NC.
> - HCROSS-SVPBMT-02 verifies VS-stage PBMT overriding intermediate attributes: G-stage PBMT=0 does not override PMA (intermediate=PMA), and VS-stage PBMT=IO overrides intermediate to produce final=IO.
> - HCROSS-SVPBMT-03 verifies the superposition behavior when both stages' PBMT are nonzero: G-stage PBMT=NC produces intermediate=NC, and VS-stage PBMT=IO overrides intermediate to produce final=IO. The VS-stage override has higher priority than G-stage.
> - HCROSS-SVPBMT-04 verifies that with `hgatp.MODE=0` (Bare), G-stage is inactive, and the PBMT override chain starts from VS-stage: intermediate=PMA, and VS-stage PBMT=NC overrides to produce final=NC.

---

## Test Priorities

| Priority | Test Group | Covered Test IDs | Rationale |
|----------|------------|------------------|-----------|
| P0 (Required) | Group 2 (Svinval) | HCROSS-SINVAL-01~15 | HINVAL instruction functionality and virtual-instruction exceptions are core to Hypervisor security isolation; existing plan coverage is severely insufficient |
| P1 (Important) | Group 1 (Svadu) | HCROSS-SVADU-01~06 | henvcfg.ADUE writability and HLV/HSV interaction are key behaviors of Svadu in virtualization scenarios |
| P3 (Optional) | Group 3 (Svnapot) | HCROSS-SVNAPOT-01~03 | G-stage NAPOT translation depends on the H extension and the Svnapot extension both being available; conditional implementation |
| P3 (Optional) | Group 4 (Svpbmt) | HCROSS-SVPBMT-01~04 | Two-stage PBMT override depends on the H extension and the Svpbmt extension both being available; conditional implementation |

---

## Key Considerations

1. **Extension detection**: All tests must detect the availability of the required extensions (H, Svadu, Svinval, Svnapot, Svpbmt, etc.) at runtime; if unavailable, TEST_SKIP.

2. **Mutual exclusion of Svadu and Svade**: With `henvcfg.ADUE=0`, the behavior is as Svade (A/D=0 triggers a fault); with `ADUE=1`, hardware A/D updating is enabled. The current ADUE state must be explicit during testing.

3. **Svinval instruction encodings**:
   - `hinval.vvma rs1, rs2`: `0011011 rs2 rs1 000 00000 1110011`
   - `hinval.gvma rs1, rs2`: `0111011 rs2 rs1 000 00000 1110011`
   - `sfence.w.inval`: `0001100 00000 00000 000 00000 1110011`
   - `sfence.inval.ir`: `0001100 00001 00000 000 00000 1110011`

4. **VMID semantics of HINVAL.GVMA**: the `rs2` register specifies the VMID (not the ASID). VMID=0 flushes the TLB of all VMIDs; a nonzero VMID flushes only the specific VMID.

5. **Distinguishing virtual-instruction from illegal-instruction**: VS/VU-mode execution of HINVAL raises virtual-instruction (cause=22), not illegal-instruction (cause=2). Test assertions must use accurate cause constants.

---

## References

- `hypervisor.adoc` — RISC-V Hypervisor Extension, Version 1.0
- `svadu.adoc` — Svadu Extension
- `svinval.adoc` — Svinval Extension
- `svnapot.adoc` — Svnapot Extension
- `svpbmt.adoc` — Svpbmt Extension
- `DOCS/testplan/Hypervisor_CSR_test_plan.md` — Hypervisor CSR subset test plan
- `DOCS/testplan/Hypervisor_Interrupts_test_plan.md` — Hypervisor interrupts subset test plan
- `DOCS/testplan/Hypervisor_Exceptions_test_plan.md` — Hypervisor exceptions and trap subset test plan
- `DOCS/testplan/Hypervisor_2_stage_test_plan.md` — Two-stage translation test plan
- `DOCS/testplan/Hypervisor_gstage_test_plan.md` — G-stage standalone test plan
- `DOCS/testplan/Svadu_test_plan.md` — Svadu standalone test plan
- `DOCS/testplan/Svinval_test_plan.md` — Svinval standalone test plan
- `DOCS/testplan/Svnapot_test_plan.md` — Svnapot standalone test plan
- `DOCS/testplan/Svpbmt_test_plan.md` — Svpbmt standalone test plan
- `ideas/hypervisor_gap.md` — Hypervisor test gap analysis

---

## Appendix A: Specification Point Coverage Matrix

The following table indicates which test cases cover each specification point in the "Covered Specification Points" section.

| Norm ID | Covered Test IDs |
|---------|------------------|
| `norm:henvcfg_adue_op` | HCROSS-SVADU-01~04 |
| `norm:Svadu_hypervisor_adue_writable` | HCROSS-SVADU-01 |
| `svadu_hfence_gvma_sync` | HCROSS-SVADU-05, HCROSS-SVADU-06 |
| `norm:Svinval_hinval_vvma_gvma` | HCROSS-SINVAL-01~04 |
| `norm:Svinval_hinval_gvma_uses_vmid` | HCROSS-SINVAL-05, HCROSS-SINVAL-06 |
| `norm:Svinval_virtual_instruction_vu_vs` | HCROSS-SINVAL-07~10, HCROSS-SINVAL-15 |
| `norm:Svinval_sfence_w_inval_inval_vu_mode` | HCROSS-SINVAL-11, HCROSS-SINVAL-12 (VS-mode normal-execution references: HCROSS-SINVAL-13, HCROSS-SINVAL-14) |
| `norm:Svnapot_hyp_gstage` | HCROSS-SVNAPOT-01~03 |
| `norm:Svpbmt_hgatp_stage_override_rule` | HCROSS-SVPBMT-01, HCROSS-SVPBMT-03, HCROSS-SVPBMT-04 |
| `norm:Svpbmt_vsatp_stage_override_rule` | HCROSS-SVPBMT-02, HCROSS-SVPBMT-03, HCROSS-SVPBMT-04 |
