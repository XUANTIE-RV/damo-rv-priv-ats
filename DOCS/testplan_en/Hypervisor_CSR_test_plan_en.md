**[中文](../testplan/Hypervisor_CSR_test_plan.md) | English**

# Hypervisor CSR Test Plan (Hypervisor_CSR Subset)

> This document is split from the original Hypervisor comprehensive test plan (Hypervisor_test_plan_en.md, now removed). Test case IDs remain unchanged; Group numbers are re-sequenced within this subset.
> Sibling subsets: [Hypervisor_Interrupts_test_plan_en.md](Hypervisor_Interrupts_test_plan_en.md) | [Hypervisor_Exceptions_test_plan_en.md](Hypervisor_Exceptions_test_plan_en.md)

## Overview

This test plan covers the register behavior of Hypervisor CSRs and Virtual Supervisor CSRs in the RISC-V Hypervisor (H) extension, including the CSR substitution mechanism, field WARL/WLRL constraints, environment configuration, and time offset functionality. Interrupt delivery/delegation mechanisms and exception/trap behavior are covered by the sibling subsets respectively.

This test plan is written based on specification points (norm tags) in `SPEC/hypervisor.adoc`.

### SPEC Chapters Covered by This Document
- Hypervisor and Virtual Supervisor CSRs (hstatus, hedeleg, hideleg, henvcfg, htimedelta CSR behavior)
- Virtual Supervisor CSRs (vsstatus, vsip, vsie, vsscratch, vsepc, vscause, vstval CSR behavior, vstimecmp)

### Covered by Other Test Plans
- Hypervisor interrupts (hvip/hip/hie, hgeip/hgeie, mideleg/mip/mie enhancements, hideleg interrupt delegation) → `Hypervisor_Interrupts_test_plan_en.md`
- Hypervisor exceptions and traps (virtual-instruction exception, trap entry/return, htinst/mtinst, mstatus enhancements, mtval2/mtinst, exception priority, hedeleg exception delegation) → `Hypervisor_Exceptions_test_plan_en.md`
- G-stage address translation → `hyp_gstage_translation_test_plan.md`
- Two-stage address translation → `hyp_2_stage_translation_test_plan.md`
- Sha combined extension → `sha_test_plan.md`
- Shcounterenw → `shcounterenw_test_plan.md`
- Shgatpa → `shgatpa_test_plan.md`
- Shvsatpa → `shvsatpa_test_plan.md`
- Shtvala → `shtvala_test_plan.md`
- Shvstvala → `shvstvala_test_plan.md`
- Shvstvecd → `shvstvecd_test_plan.md`

---

## Covered Specification Points

This section lists all specification points (norm IDs) referenced in Groups 1-9 of this document, deduplicated and sorted alphabetically.

| Norm ID | English Description |
|---------|---------------------|
| `norm:H_mtval_nrz` | CSR `mtval` must not be read-only zero. |
| `norm:hedeleg_acc` | Each bit of `hedeleg` shall be either writable or read-only zero. Many bits of `hedeleg` are required specifically to be writable or zero, as enumerated in the table. Bit 0, corresponding to instruction address-misaligned exceptions, must be writable if IALIGN=32. |
| `norm:hedeleg_sz_acc` | Register `hedeleg` is a 64-bit read/write register. |
| `norm:henvcfg_adue_op` | If the Svadu extension is implemented, the ADUE bit controls whether hardware updating of PTE A/D bits is enabled for VS-stage address translation. When ADUE=1, hardware updating is enabled. When ADUE=0, the implementation behaves as though Svade were implemented for VS-stage address translation. If Svadu is not implemented, ADUE is read-only zero. |
| `norm:henvcfg_cbcfe` | The Zicbom extension adds the CBCFE field to `henvcfg`. When V=1, if CBO.CLEAN and CBO.FLUSH are HS-qualified and CBCFE=1, they are enabled; if CBCFE=0, they raise a virtual-instruction exception. When Zicbom is not implemented, CBCFE is read-only zero. |
| `norm:henvcfg_cbie` | The Zicbom extension adds the CBIE WARL field to `henvcfg`. The CBIE field controls execution of CBO.INVAL in privilege modes VS and VU. The encoding `10b` is reserved. When Zicbom is not implemented, CBIE is read-only zero. |
| `norm:henvcfg_cbze` | The Zicboz extension adds the CBZE field to `henvcfg`. When CBZE=1, CBO.ZERO is enabled for execution in VS/VU mode; when CBZE=0, it raises a virtual-instruction exception. When the Zicboz extension is not implemented, CBZE is read-only zero. |
| `norm:henvcfg_dte_op` | The Ssdbltrp extension adds the double-trap-enable (DTE) field in `henvcfg`. When `henvcfg`.DTE is zero, the implementation behaves as though Ssdbltrp is not implemented for VS-mode and the `vsstatus`.SDT bit is read-only zero. |
| `norm:henvcfg_fiom_op` | If bit FIOM is set to one in `henvcfg`, FENCE instructions executed when V=1 are modified so the requirement to order accesses to device I/O implies also the requirement to order main memory accesses. |
| `norm:henvcfg_fiom_order` | Similarly, when FIOM=1 and V=1, if an atomic instruction that accesses a region ordered as device I/O has its _aq_ and/or _rl_ bit set, then that instruction is ordered as though it accesses both device I/O and memory. |
| `norm:henvcfg_lpe_op` | The Zicfilp extension adds the LPE field in `henvcfg`. When LPE=1, the Zicfilp extension is enabled in VS-mode. When LPE=0, the hart does not update the ELP state and LPAD operates as a no-op. |
| `norm:henvcfg_pbmte_op` | The PBMTE bit controls whether the Svpbmt extension is available for use in VS-stage address translation. When PBMTE=1, Svpbmt is available for VS-stage address translation. When PBMTE=0, the implementation behaves as though Svpbmt were not implemented for VS-stage address translation. If Svpbmt is not implemented, PBMTE is read-only zero. |
| `norm:henvcfg_pmm_op` | If the Ssnpm extension is implemented, the PMM field enables or disables pointer masking for VS-mode. When not implemented, PMM is read-only zero. PMM is read-only zero for RV32. |
| `norm:henvcfg_sse_op` | The Zicfiss extension adds the SSE field in `henvcfg`. If SSE=1, the Zicfiss extension is activated in VS-mode. When SSE=0, the extension remains inactive in VS-mode with specific behavior changes. |
| `norm:henvcfg_stce` | The Sstc extension adds the STCE (STimecmp Enable) bit to `henvcfg` CSR. When the Sstc extension is not implemented, STCE is read-only zero. The STCE bit enables `vstimecmp` for VS-mode when set to one. When STCE is zero, an attempt to access `stimecmp` (really `vstimecmp`) when V=1 raises a virtual-instruction exception, and VSTIP in `hip` reverts to its defined behavior as if this extension is not implemented. |
| `norm:henvcfg_sz_acc_op` | The `henvcfg` CSR is a 64-bit read/write register that controls certain characteristics of the execution environment when virtualization mode V=1. |
| `norm:hideleg_acc` | Among bits 15:0 of `hideleg`, bits 10, 6, and 2 (corresponding to the standard VS-level interrupts) are writable, and bits 12, 9, 5, and 1 (corresponding to the standard S-level interrupts) are read-only zeros. |
| `norm:hideleg_sz_acc` | Register `hideleg` is an HSXLEN-bit read/write register. |
| `norm:hip_vstip_clear` | If the result of this comparison changes, it is guaranteed to be reflected in VSTIP eventually, but not necessarily immediately. The interrupt remains posted until `vstimecmp` becomes greater than (`time` + `htimedelta`), typically as a result of writing `vstimecmp`. |
| `norm:hip_vstip_enable` | The interrupt will be taken based on the standard interrupt enable and delegation rules while V=1. |
| `norm:hip_vstip_op` | A virtual supervisor timer interrupt becomes pending, as reflected in the VSTIP bit in the `hip` register, whenever (`time` + `htimedelta`), truncated to 64 bits, contains a value greater than or equal to `vstimecmp`, treating the values as unsigned integers. |
| `norm:H_scsrs_nomatch` | Some standard supervisor CSRs (`senvcfg`, `scounteren`, and `scontext`, possibly others) have no matching VS CSR. These supervisor CSRs continue to have their usual function and accessibility even when V=1, except with VS-mode and VU-mode substituting for HS-mode and U-mode. |
| `norm:H_vscsrs_acc_m_hs` | The VS CSRs can be accessed as themselves only from M-mode or HS-mode. |
| `norm:H_vscsrs_acc_u` | Attempts from U-mode cause an illegal-instruction exception as usual. |
| `norm:H_vscsrs_acc_vs` | When V=1, an attempt to read or write a VS CSR directly by its own separate CSR address causes a virtual-instruction exception. |
| `norm:H_vscsrs_sub` | When V=1, the VS CSRs substitute for the corresponding supervisor CSRs, taking over all functions of the usual supervisor CSRs except as specified otherwise. Instructions that normally read or modify a supervisor CSR shall instead access the corresponding VS CSR. |
| `norm:H_vscsrs_v0` | When V=0, the VS CSRs do not ordinarily affect the behavior of the machine other than being readable and writable by CSR instructions. |
| `norm:H_vscsrs_v1` | While V=1, the normal HS-level supervisor CSRs that are replaced by VS CSRs retain their values but do not affect the behavior of the machine unless specifically documented to do so. |
| `norm:hstatus_gva_op` | Field GVA (Guest Virtual Address) is written by the implementation whenever a trap is taken into HS-mode. For any trap that writes a guest virtual address to `stval`, GVA is set to 1. For any other trap into HS-mode, GVA is set to 0. |
| `norm:hstatus_hu_op` | Field HU (Hypervisor in U-mode) controls whether the virtual-machine load/store instructions, HLV, HLVX, and HSV, can be used also in U-mode. When HU=1, these instructions can be executed in U-mode the same as in HS-mode. When HU=0, all hypervisor instructions cause an illegal-instruction exception in U-mode. |
| `norm:hstatus_spv_op` | The SPV bit (Supervisor Previous Virtualization mode) is written by the implementation whenever a trap is taken into HS-mode. Just as the SPP bit in `sstatus` is set to the (nominal) privilege mode at the time of the trap, the SPV bit in `hstatus` is set to the value of the virtualization mode V at the time of the trap. |
| `norm:hstatus_spv_sret` | When an SRET instruction is executed when V=0, V is set to SPV. |
| `norm:hstatus_spvp_op` | When V=1 and a trap is taken into HS-mode, bit SPVP (Supervisor Previous Virtual Privilege) is set to the nominal privilege mode at the time of the trap, the same as `sstatus`.SPP. But if V=0 before a trap, SPVP is left unchanged on trap entry. |
| `norm:hstatus_sz_acc_op` | The `hstatus` register is an HSXLEN-bit read/write register... provides facilities analogous to the `mstatus` register for tracking and controlling the exception behavior of a VS-mode guest. |
| `norm:hstatus_vgein_op` | The VGEIN (Virtual Guest External Interrupt Number) field selects a guest external interrupt source for VS-level external interrupts. VGEIN is a WLRL field that must be able to hold values between zero and the maximum guest external interrupt number (known as GEILEN), inclusive. |
| `norm:hstatus_vsbe_op` | The VSBE bit is a WARL field that controls the endianness of explicit memory accesses made from VS-mode. |
| `norm:hstatus_vsxl_32` | When HSXLEN=32, the VSXL field does not exist, and VSXLEN=32. |
| `norm:hstatus_vsxl_64` | When HSXLEN=64, VSXL is a WARL field that is encoded the same as the MXL field of `misa`. |
| `norm:hstatus_vsxl_op` | The VSXL field controls the effective XLEN for VS-mode (known as VSXLEN), which may differ from the XLEN for HS-mode (HSXLEN). |
| `norm:hstatus_vtsr_op` | When VTSR=1, an attempt in VS-mode to execute SRET raises a virtual-instruction exception. |
| `norm:hstatus_vtvm_op` | When VTVM=1, an attempt in VS-mode to execute SFENCE.VMA or SINVAL.VMA or to access CSR `satp` raises a virtual-instruction exception. |
| `norm:hstatus_vtw_op` | When VTW=1 (and assuming `mstatus`.TW=0), an attempt in VS-mode to execute WFI raises a virtual-instruction exception if the WFI does not complete within an implementation-specific, bounded time limit. |
| `norm:htimedelta_sz_acc_op` | The `htimedelta` CSR is a 64-bit read/write register that contains the delta between the value of the `time` CSR and the value returned in VS-mode or VU-mode. Reading the `time` CSR in VS or VU mode returns the sum of `htimedelta` and the actual value of `time`. |
| `norm:time_htimedelta_req` | If the `time` CSR is implemented, `htimedelta` (and `htimedeltah` for XLEN=32) must be implemented. |
| `norm:vscause_sz_acc_op` | The `vscause` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `scause`. When V=1, `vscause` substitutes for the usual `scause`. When V=0, `vscause` does not directly affect the behavior of the machine. |
| `norm:vscause_wlrl` | `vscause` is a WLRL register that must be able to hold the same set of values that `scause` can hold. |
| `norm:vsepc_warl` | `vsepc` is a WARL register that must be able to hold the same set of values that `sepc` can hold. |
| `norm:vsip_vsie_lcofi` | Extension Shlcofideleg supports delegating LCOFI interrupts to VS-mode. If implemented, `hideleg` bit 13 is writable; otherwise read-only zero. When bit 13 of `hideleg` is zero, `vsip`.LCOFIP and `vsie`.LCOFIE are read-only zeros. Else, they are aliases of `sip`.LCOFIP and `sie`.LCOFIE. |
| `norm:vsip_vsie_sei` | When bit 10 of `hideleg` is zero, `vsip`.SEIP and `vsie`.SEIE are read-only zeros. Else, they are aliases of `hip`.VSEIP and `hie`.VSEIE. |
| `norm:vsip_vsie_ssi` | When bit 2 of `hideleg` is zero, `vsip`.SSIP and `vsie`.SSIE are read-only zeros. Else, they are aliases of `hip`.VSSIP and `hie`.VSSIE. |
| `norm:vsip_vsie_sti` | When bit 6 of `hideleg` is zero, `vsip`.STIP and `vsie`.STIE are read-only zeros. Else, they are aliases of `hip`.VSTIP and `hie`.VSTIE. |
| `norm:vsip_vsie_sz_acc_op` | The `vsip` and `vsie` registers are VSXLEN-bit read/write registers that are VS-mode's versions of supervisor CSRs `sip` and `sie`. When V=1, `vsip` and `vsie` substitute for the usual `sip` and `sie`. However, interrupts directed to HS-level continue to be indicated in the HS-level `sip` register, not in `vsip`, when V=1. |
| `norm:vspec_sz_acc_op` | The `vsepc` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `sepc`. When V=1, `vsepc` substitutes for the usual `sepc`. When V=0, `vsepc` does not directly affect the behavior of the machine. |
| `norm:vsscratch_sz_acc_op` | The `vsscratch` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `sscratch`. When V=1, `vsscratch` substitutes for the usual `sscratch`. The contents of `vsscratch` never directly affect the behavior of the machine. |
| `norm:vsstatus_fs_op` | When V=1, both `vsstatus`.FS and the HS-level `sstatus`.FS are in effect. Attempts to execute a floating-point instruction when either field is 0 (Off) raise an illegal-instruction exception. Modifying the floating-point state when V=1 causes both fields to be set to 3 (Dirty). |
| `norm:vsstatus_sd_xs_op` | Read-only fields SD and XS summarize the extension context status as it is visible to VS-mode only. For example, the value of the HS-level `sstatus`.FS does not affect `vsstatus`.SD. |
| `norm:vsstatus_sz_acc_op` | The `vsstatus` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `sstatus`. When V=1, `vsstatus` substitutes for the usual `sstatus`, so instructions that normally read or modify `sstatus` actually access `vsstatus` instead. |
| `norm:vsstatus_ube` | An implementation may make field UBE be a read-only copy of `hstatus`.VSBE. |
| `norm:vsstatus_uxl_change` | If VSXLEN is changed from 32 to a wider width, and if field UXL is not restricted to a single value, it gets the value corresponding to the widest supported width not wider than the new VSXLEN. |
| `norm:vsstatus_uxl_op` | The UXL field controls the effective XLEN for VU-mode. When VSXLEN=32, the UXL field does not exist, and VU-mode XLEN=32. When VSXLEN=64, UXL is a WARL field. An implementation may make UXL be a read-only copy of field VSXL of `hstatus`, forcing VU-mode XLEN=VSXLEN. |
| `norm:vsstatus_v0` | When V=0, `vsstatus` does not directly affect the behavior of the machine, unless a virtual-machine load/store (HLV, HLVX, or HSV) or the MPRV feature in the `mstatus` register is used to execute a load or store as though V=1. |
| `norm:vsstatus_vs_op` | Similarly, when V=1, both `vsstatus`.VS and the HS-level `sstatus`.VS are in effect. Attempts to execute a vector instruction when either field is 0 (Off) raise an illegal-instruction exception. Modifying the vector state when V=1 causes both fields to be set to 3 (Dirty). |
| `norm:vstimecmp_acc` | In RV32 only, accesses to the `vstimecmp` CSR access the low 32 bits, while accesses to the `vstimecmph` CSR access the high 32 bits of `vstimecmp`. |
| `norm:vstimecmp_sz` | The `vstimecmp` CSR is a 64-bit register and has 64-bit precision on all RV32 and RV64 systems. |
| `norm:vstval_sz_acc_op` | The `vstval` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `stval`. When V=1, `vstval` substitutes for the usual `stval`. When V=0, `vstval` does not directly affect the behavior of the machine. |
| `norm:vstval_warl` | `vstval` is a WARL register that must be able to hold the same set of values that `stval` can hold. |
| `norm:vsxl_ro` | In particular, an implementation may make VSXL be a read-only field whose value always ensures that VSXLEN=HSXLEN. |
| `norm:vtw_virtinstr` | An implementation may have WFI always raise a virtual-instruction exception in VS-mode when VTW=1 (and `mstatus`.TW=0), even if there are pending globally-disabled interrupts when the instruction is executed. |

---

## Group 1. CSR Substitution Mechanism when V=1

**Specification References**:
- `norm:H_vscsrs_sub`: When V=1, VS CSRs substitute for corresponding S CSRs; instructions accessing S CSRs actually access VS CSRs
- `norm:H_vscsrs_acc_vs`: When V=1, direct access to VS CSR by its own address triggers virtual-instruction exception
- `norm:H_vscsrs_acc_u`: U-mode access triggers illegal-instruction exception
- `norm:H_vscsrs_acc_m_hs`: VS CSRs can only be accessed by their own address in M/HS-mode
- `norm:H_vscsrs_v1`: When V=1, HS-level S CSRs retain values but do not affect behavior
- `norm:H_vscsrs_v0`: When V=0, VS CSRs do not affect behavior
- `norm:H_scsrs_nomatch`: Behavior of S CSRs without matching VS CSRs (senvcfg/scounteren/scontext) when V=1
- `norm:H_mtval_nrz`: mtval must not be read-only zero when the H extension is implemented (VCSR-18)

**Test Responsibilities**: Verify VS CSR substitution, access control, and isolation behavior in both V=1 and V=0 modes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VCSR-01 | sstatus accesses vsstatus when V=1 | HS-mode writes specific value to vsstatus, enter VS-mode and read with csrr sstatus | Reads the value of vsstatus |
| VCSR-02 | Writing sstatus actually writes vsstatus when V=1 | VS-mode writes value with csrw sstatus, return to HS-mode and read with csrr vsstatus | vsstatus is modified |
| VCSR-03 | HS-level sstatus preserved when V=1 | HS-mode sets specific value in sstatus, enter VS-mode and modify sstatus, return and check HS-level sstatus | HS-level sstatus unaffected by VS-mode modifications |
| VCSR-04 | Direct access to VS CSR address triggers exception when V=1 | VS-mode uses csrr to directly access vsstatus CSR address (0x200) | virtual-instruction exception (cause=22) |
| VCSR-05 | U-mode access to VS CSR address triggers exception | U-mode attempts csrr vsstatus | illegal-instruction exception (cause=2) |
| VCSR-06 | M-mode direct access to VS CSR | M-mode uses csrr/csrw to directly access vsstatus | Normal read/write succeeds |
| VCSR-07 | HS-mode direct access to VS CSR | HS-mode uses csrr/csrw to directly access vsstatus | Normal read/write succeeds |
| VCSR-08 | VS CSR does not affect behavior when V=0 | HS-mode writes vsstatus.SIE=0, check HS-mode's own interrupt enable | HS-mode interrupt enable unaffected by vsstatus.SIE |
| VCSR-09 | sepc accesses vsepc when V=1 | HS-mode writes vsepc=0xDEAD, VS-mode csrr sepc | Reads 0xDEAD (after WARL truncation) |
| VCSR-10 | scause accesses vscause when V=1 | Similar to VCSR-09, verify scause/vscause substitution | Substitution takes effect |
| VCSR-11 | stval accesses vstval when V=1 | Similar to VCSR-09, verify stval/vstval substitution | Substitution takes effect |
| VCSR-12 | stvec accesses vstvec when V=1 | Similar to VCSR-09, verify stvec/vstvec substitution | Substitution takes effect |
| VCSR-13 | sscratch accesses vsscratch when V=1 | Similar to VCSR-09, verify sscratch/vsscratch substitution | Substitution takes effect |
| VCSR-14 | sip/sie access vsip/vsie when V=1 | HS-mode configures vsip/vsie, VS-mode reads sip/sie | Reads values of vsip/vsie |
| VCSR-15 | satp accesses vsatp when V=1 | HS-mode writes vsatp, VS-mode csrr satp | Reads value of vsatp |
| VCSR-16 | senvcfg without matching VS CSR functions normally when V=1 | VS-mode reads/writes senvcfg when V=1, verify this CSR takes effect directly rather than being substituted | senvcfg read/write normal, hypervisor must manually swap |
| VCSR-17 | scounteren without matching VS CSR functions normally when V=1 | Set scounteren when V=1, verify VU-mode counter access is controlled by it | scounteren controls VU-mode counter visibility |
| VCSR-18 | mtval is not read-only zero (H extension) | With the H extension present, write pattern and address values to mtval and read back | Written values must be retained (`norm:H_mtval_nrz`, no read-only zero fallback) |

---

## Group 2. hstatus Register

**Specification References**:
- `norm:hstatus_sz_acc_op`: HSXLEN-bit read/write register
- `norm:hstatus_vsxl_op` / `norm:hstatus_vsxl_32` / `norm:hstatus_vsxl_64` / `norm:vsxl_ro`: VSXL field
- `norm:hstatus_vtsr_op`: When VTSR=1, VS-mode SRET triggers virtual-instruction exception
- `norm:hstatus_vtw_op` / `norm:vtw_virtinstr`: When VTW=1, VS-mode WFI triggers virtual-instruction exception
- `norm:hstatus_vtvm_op`: When VTVM=1, VS-mode access to satp/SFENCE.VMA/SINVAL.VMA triggers virtual-instruction exception
- `norm:hstatus_vgein_op`: VGEIN field selects guest external interrupt source
- `norm:hstatus_hu_op`: HU field controls HLV/HLVX/HSV enable in U-mode
- `norm:hstatus_spv_op` / `norm:hstatus_spv_sret`: SPV field
- `norm:hstatus_spvp_op`: SPVP field
- `norm:hstatus_gva_op`: GVA field
- `norm:hstatus_vsbe_op`: VSBE field

**Test Responsibilities**: Verify correct behavior of hstatus fields in trap, SRET, and VS-mode execution scenarios.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HSTAT-01 | Basic hstatus read/write | HS-mode writes all writable fields of hstatus, read back to verify | Writable fields read back consistently, WPRI fields are zero |
| HSTAT-02 | VTSR=1 VS-mode SRET triggers exception | Set hstatus.VTSR=1, VS-mode executes SRET | virtual-instruction exception (cause=22) |
| HSTAT-03 | VTSR=0 VS-mode SRET normal | Set hstatus.VTSR=0, VS-mode executes SRET | SRET executes normally |
| HSTAT-04 | VTW=1 VS-mode WFI triggers exception | Set hstatus.VTW=1, mstatus.TW=0, VS-mode executes WFI | virtual-instruction exception (cause=22) |
| HSTAT-05 | VTW=0 VS-mode WFI normal | hstatus.VTW=0, VS-mode executes WFI | WFI executes normally (or completes on timeout) |
| HSTAT-06 | mstatus.TW=1 overrides VTW | mstatus.TW=1, hstatus.VTW=0, VS-mode executes WFI | illegal-instruction exception (cause=2) |
| HSTAT-07 | VTVM=1 VS-mode read satp triggers exception | Set hstatus.VTVM=1, VS-mode csrr satp | virtual-instruction exception (cause=22) |
| HSTAT-08 | VTVM=1 VS-mode SFENCE.VMA triggers exception | Set hstatus.VTVM=1, VS-mode executes SFENCE.VMA | virtual-instruction exception (cause=22) |
| HSTAT-09 | VTVM=1 VS-mode SINVAL.VMA triggers exception | Set hstatus.VTVM=1, VS-mode executes SINVAL.VMA | virtual-instruction exception (cause=22) |
| HSTAT-10 | VTVM=0 VS-mode access satp normal | hstatus.VTVM=0, VS-mode csrr/csrw satp | Normal access (actually accesses vsatp) |
| HSTAT-11 | HU=1 U-mode executes HLV | Set hstatus.HU=1, U-mode executes HLV.W | Executes normally |
| HSTAT-12 | HU=0 U-mode executes HLV triggers exception | Set hstatus.HU=0, U-mode executes HLV.W | illegal-instruction exception (cause=2) |
| HSTAT-13 | SPV written correctly on trap | Trap from VS-mode to HS-mode | hstatus.SPV=1 |
| HSTAT-14 | SPV written correctly on trap (from U-mode) | Trap from U-mode to HS-mode | hstatus.SPV=0 |
| HSTAT-15 | V set to SPV on SRET | Set hstatus.SPV=1, HS-mode executes SRET | V becomes 1, enters VS-mode |
| HSTAT-16 | SPVP written on V=1 trap | Trap from VS-mode (S-level) to HS-mode | hstatus.SPVP=1 |
| HSTAT-17 | SPVP written on V=1 trap (VU-mode) | Trap from VU-mode to HS-mode | hstatus.SPVP=0 |
| HSTAT-18 | SPVP unchanged on V=0 trap | Trap to HS-mode when V=0 | hstatus.SPVP remains unchanged |
| HSTAT-19 | SPVP controls HLV/HSV effective privilege | Set hstatus.SPVP=0, execute HLV; set SPVP=1 and execute again | SPVP=0 uses VU-mode, SPVP=1 uses VS-mode |
| HSTAT-20 | GVA set to 1 on guest-page-fault | VS-mode triggers guest-page-fault trap to HS-mode | hstatus.GVA=1 |
| HSTAT-21 | GVA set to 0 on non-address fault | VS-mode triggers ecall trap to HS-mode | hstatus.GVA=0 |
| HSTAT-22 | GVA set to 1 on page-fault (V=1) | VS-mode triggers page-fault (VS-stage) | hstatus.GVA=1 |
| HSTAT-23 | GVA=1 but SPV=0 on HLV fault | HS-mode executes HLV triggering guest-page-fault | hstatus.SPV=0, hstatus.GVA=1 |
| HSTAT-24 | VSBE field read/write | Write hstatus.VSBE=0/1 and read back | WARL behavior correct (may be read-only fixed value) |
| HSTAT-25 | VSXL field read/write (HSXLEN=64) | Write hstatus.VSXL and read back | WARL behavior correct |
| HSTAT-26 | VGEIN field read/write | Write hstatus.VGEIN=legal value and read back | WLRL, value between 0 and GEILEN |

---

## Group 3. henvcfg Register

**Specification References**:
- `norm:henvcfg_sz_acc_op`: 64-bit read-write register
- `norm:henvcfg_fiom_op` / `norm:henvcfg_fiom_order`: FIOM field modifies FENCE behavior when V=1
- `norm:henvcfg_pbmte_op`: PBMTE controls VS-stage Svpbmt
- `norm:henvcfg_adue_op`: ADUE controls VS-stage A/D hardware update
- `norm:henvcfg_stce`: STCE enables vstimecmp
- `norm:henvcfg_cbze` / `norm:henvcfg_cbcfe` / `norm:henvcfg_cbie`: CBO instruction control
- `norm:henvcfg_pmm_op`: PMM controls VS-mode pointer masking
- `norm:henvcfg_lpe_op`: LPE controls VS-mode Zicfilp
- `norm:henvcfg_sse_op`: SSE controls VS-mode Zicfiss
- `norm:henvcfg_dte_op`: DTE controls VS-mode Ssdbltrp

**Test Responsibilities**: Verify the control effect of each henvcfg field on the VS-mode execution environment when V=1.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HENV-01 | Basic read/write of henvcfg | Write each field of henvcfg and read back | Implemented fields are readable/writable, unimplemented fields are read-only zero |
| HENV-02 | FIOM=1 modifies FENCE behavior | When V=1 and henvcfg.FIOM=1, VS-mode executes FENCE with PI/PO | FENCE implies PR/PW (I/O implies Memory) |
| HENV-03 | FIOM=0 does not modify FENCE | When V=1 and henvcfg.FIOM=0, VS-mode executes the same FENCE | FENCE behavior unchanged |
| HENV-04 | PBMTE=1 enables VS-stage Svpbmt | Set henvcfg.PBMTE=1, VS-stage PTE uses PBMT field | PTE PBMT field takes effect |
| HENV-05 | PBMTE=0 disables VS-stage Svpbmt | Set henvcfg.PBMTE=0, VS-stage PTE uses PBMT field | Implementation behaves as if Svpbmt does not exist |
| HENV-06 | ADUE=1 enables VS-stage A/D hardware update | henvcfg.ADUE=1, VS-stage PTE A=0 | Hardware automatically sets A bit |
| HENV-07 | ADUE=0 disables VS-stage A/D hardware update | henvcfg.ADUE=0, VS-stage PTE A=0 | Page fault (Svade behavior) |
| HENV-08 | STCE=1 enables vstimecmp | henvcfg.STCE=1, VS-mode accesses stimecmp | Normal access (actually accesses vstimecmp) |
| HENV-09 | STCE=0 disables vstimecmp | henvcfg.STCE=0, VS-mode accesses stimecmp | Virtual-instruction exception |
| HENV-10 | CBZE=1 enables CBO.ZERO | henvcfg.CBZE=1, VS-mode executes CBO.ZERO | Executes normally |
| HENV-11 | CBZE=0 disables CBO.ZERO | henvcfg.CBZE=0, VS-mode executes CBO.ZERO | Virtual-instruction exception |
| HENV-12 | CBCFE=1 enables CBO.CLEAN/FLUSH | henvcfg.CBCFE=1, VS-mode executes CBO.CLEAN | Executes normally |
| HENV-13 | CBCFE=0 disables CBO.CLEAN/FLUSH | henvcfg.CBCFE=0, VS-mode executes CBO.CLEAN | Virtual-instruction exception |
| HENV-14 | CBIE=01 CBO.INVAL performs flush | henvcfg.CBIE=01, VS-mode executes CBO.INVAL | Performs flush operation |
| HENV-15 | CBIE=00 disables CBO.INVAL | henvcfg.CBIE=00, VS-mode executes CBO.INVAL | Virtual-instruction exception |
| HENV-16 | DTE=0 disables VS-mode Ssdbltrp | henvcfg.DTE=0, read vsstatus.SDT | SDT is read-only zero |
| HENV-17 | DTE=1 enables VS-mode Ssdbltrp | henvcfg.DTE=1, write vsstatus.SDT=1 and read back | SDT is writable and readable |
| HENV-18 | PBMTE is read-only zero when not implemented | If Svpbmt is not implemented, read henvcfg.PBMTE | Read-only zero |
| HENV-19 | STCE is read-only zero when not implemented | If Sstc is not implemented, read henvcfg.STCE | Read-only zero |
| HENV-20 | CBIE=11 CBO.INVAL performs invalidate | henvcfg.CBIE=11, VS-mode executes CBO.INVAL | Performs invalidate operation |
| HENV-21 | CBIE=10 reserved encoding WARL behavior | Write CBIE=10 (reserved value), read back | WARL: reserved encoding 10b is not preserved |
| HENV-22 | PMM field read/write (Ssnpm) | Write PMM=00/10/11 and read back; write PMM=01 (reserved) | Legal values writable, reserved value WARL |
| HENV-23 | LPE/SSE field read/write | Write LPE/SSE=1 and read back | Writable when implemented; read-only zero when not implemented |
| HENV-24 | PMM is read-only zero when not implemented | If Ssnpm is not implemented, read henvcfg.PMM | Read-only zero |
| HENV-25 | CBIE is read-only zero when not implemented | If Zicbom is not implemented, read henvcfg.CBIE | Read-only zero |
| HENV-26 | CBCFE is read-only zero when not implemented | If Zicbom is not implemented, read henvcfg.CBCFE | Read-only zero |
| HENV-27 | CBZE is read-only zero when not implemented | If Zicboz is not implemented, read henvcfg.CBZE | Read-only zero |
| HENV-28 | DTE is read-only zero when not implemented | If Ssdbltrp is not implemented, read henvcfg.DTE | Read-only zero |
| HENV-29 | ADUE is read-only zero when not implemented | If Svadu is not implemented, read henvcfg.ADUE | Read-only zero |

---

## Group 4. htimedelta Register

**Specification References**:
- `norm:htimedelta_sz_acc_op`: 64-bit read-write register; when VS/VU-mode reads time, returns time + htimedelta
- `norm:time_htimedelta_req`: If time CSR is implemented, htimedelta must be implemented

**Test Responsibilities**: Verify the time offset functionality.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HTDLT-01 | Basic read/write of htimedelta | HS-mode writes htimedelta=0x1000, reads back | Reads back 0x1000 |
| HTDLT-02 | VS-mode reading time includes delta | Set htimedelta=N, VS-mode reads time | Return value ≈ actual time + N |
| HTDLT-03 | VU-mode reading time includes delta | Set htimedelta=N, VU-mode reads time | Return value ≈ actual time + N |
| HTDLT-04 | Large htimedelta value (negative offset) | Set htimedelta=0xFFFFFFFFFFFF0000 | VS-mode reading time returns a value less than actual time |
| HTDLT-05 | HS-mode reading time does not include delta | Set htimedelta=N, HS-mode reads time | Returns actual time (without adding delta) |

---

## Group 5. vsstatus Register

**Specification References**:
- `norm:vsstatus_sz_acc_op`: VSXLEN-bit read-write register; substitutes for sstatus when V=1
- `norm:vsstatus_uxl_op` / `norm:vsstatus_uxl_change`: UXL field controls VU-mode XLEN
- `norm:vsstatus_fs_op`: When V=1, vsstatus.FS and sstatus.FS take effect simultaneously
- `norm:vsstatus_vs_op`: When V=1, vsstatus.VS and sstatus.VS take effect simultaneously
- `norm:vsstatus_sd_xs_op`: SD/XS only reflects VS-mode visible state
- `norm:vsstatus_ube`: UBE may be a read-only copy of hstatus.VSBE
- `norm:vsstatus_v0`: When V=0, does not directly affect behavior (except HLV/HSV/MPRV)

**Test Responsibilities**: Verify the functional behavior of each vsstatus field.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSST-01 | Basic read/write of vsstatus | HS-mode writes each field of vsstatus, reads back | Writable fields read back consistently |
| VSST-02 | sstatus actually accesses vsstatus when V=1 | See VCSR-01/02 | Substitution takes effect |
| VSST-03 | FP instruction exception when vsstatus.FS=0 | When V=1, set vsstatus.FS=0 (sstatus.FS≠0), VS-mode executes FP instruction | Illegal-instruction exception |
| VSST-04 | FP instruction exception when sstatus.FS=0 | When V=1, set sstatus.FS=0 (vsstatus.FS≠0), VS-mode executes FP instruction | Illegal-instruction exception |
| VSST-05 | FP executable when both FS are non-zero | When V=1, sstatus.FS≠0 and vsstatus.FS≠0 | FP instructions execute normally |
| VSST-06 | FP modification makes both FS Dirty | When V=1, execute FP write instruction | sstatus.FS=3 and vsstatus.FS=3 |
| VSST-07 | Vector instruction exception when vsstatus.VS=0 | When V=1, set vsstatus.VS=0 (sstatus.VS≠0) | Illegal-instruction exception |
| VSST-08 | Vector instruction exception when sstatus.VS=0 | When V=1, set sstatus.VS=0 (vsstatus.VS≠0) | Illegal-instruction exception |
| VSST-09 | Vector modification makes both VS Dirty | When V=1, execute Vector write instruction | sstatus.VS=3 and vsstatus.VS=3 |
| VSST-10 | vsstatus.SD only reflects VS-mode perspective | When V=1, sstatus.FS=Dirty but vsstatus.FS=Clean | vsstatus.SD is calculated based on vsstatus fields |
| VSST-11 | vsstatus does not affect behavior when V=0 | When V=0, set vsstatus.SIE=0 | HS-mode is unaffected |
| VSST-12 | UXL field read/write | Write vsstatus.UXL and read back | WARL behavior is correct |

---

## Group 6. vsip / vsie Registers

**Specification References**:
- `norm:vsip_vsie_sz_acc_op`: Substitutes for sip/sie when V=1
- `norm:vsip_vsie_sei`: When hideleg[10]=0, vsip.SEIP/vsie.SEIE are read-only zero; otherwise they are aliases of hip.VSEIP/hie.VSEIE
- `norm:vsip_vsie_sti`: When hideleg[6]=0, vsip.STIP/vsie.STIE are read-only zero; otherwise they are aliases of hip.VSTIP/hie.VSTIE
- `norm:vsip_vsie_ssi`: When hideleg[2]=0, vsip.SSIP/vsie.SSIE are read-only zero; otherwise they are aliases of hip.VSSIP/hie.VSSIE
- `norm:vsip_vsie_lcofi`: Shlcofideleg extension, hideleg[13] controls the vsip.LCOFIP/vsie.LCOFIE alias

**Test Responsibilities**: Verify vsip/vsie substitution mechanism, alias relationships, read-side read-only-zero WARL, write-side ineffective masking, direct CSR read/write, and the LCOFI extension.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSIE-01 | sip/sie accesses vsip/vsie when V=1 | VS-mode reads sip/sie | Actually reads vsip/vsie content |
| VSIE-02 | vsip.SEIP is an alias of hip.VSEIP when hideleg[10]=1 | Set hideleg[10]=1, hvip.VSEIP=1, VS-mode reads sip.SEIP | Reads 1 |
| VSIE-03 | vsip.SEIP is read-only zero when hideleg[10]=0 | Set hideleg[10]=0, hvip.VSEIP=1, VS-mode reads sip.SEIP | Reads 0 |
| VSIE-04 | vsip.STIP alias when hideleg[6]=1 | Set hideleg[6]=1, hvip.VSTIP=1, VS-mode reads sip.STIP | Reads 1 |
| VSIE-05 | vsip.STIP is read-only zero when hideleg[6]=0 | Set hideleg[6]=0, hvip.VSTIP=1, VS-mode reads sip.STIP | Reads 0 |
| VSIE-06 | vsip.SSIP alias when hideleg[2]=1 | Set hideleg[2]=1, hvip.VSSIP=1, VS-mode reads sip.SSIP | Reads 1 |
| VSIE-07 | vsip.SSIP is read-only zero when hideleg[2]=0 | Set hideleg[2]=0, VS-mode reads sip.SSIP | Reads 0 |
| VSIE-08 | vsie.SEIE alias verification | hideleg[10]=1, VS-mode writes sie.SEIE=1, HS-mode reads hie.VSEIE | hie.VSEIE=1 |
| VSIE-09 | vsie.STIE alias verification | hideleg[6]=1, VS-mode writes sie.STIE=1, HS-mode reads hie.VSTIE | hie.VSTIE=1 |
| VSIE-10 | vsie.SSIE alias verification | hideleg[2]=1, VS-mode writes sie.SSIE=1, HS-mode reads hie.VSSIE | hie.VSSIE=1 |
| VSIE-11 | vsie.SEIE read-only zero when hideleg[10]=0 | Set hie.VSEIE=1, hideleg[10]=0, VS-mode reads sie.SEIE | Reads 0 |
| VSIE-12 | vsie.STIE read-only zero when hideleg[6]=0 | Set hie.VSTIE=1, hideleg[6]=0, VS-mode reads sie.STIE | Reads 0 |
| VSIE-13 | vsie.SSIE read-only zero when hideleg[2]=0 | Set hie.VSSIE=1, hideleg[2]=0, VS-mode reads sie.SSIE | Reads 0 |
| VSIE-14 | VS write to sie.SEIE ineffective when hideleg[10]=0 | hideleg[10]=0, VS-mode writes sie.SEIE=1, read hie.VSEIE | hie.VSEIE=0 (write ineffective) |
| VSIE-15 | VS write to sie.STIE ineffective when hideleg[6]=0 | hideleg[6]=0, VS-mode writes sie.STIE=1, read hie.VSTIE | hie.VSTIE=0 (write ineffective) |
| VSIE-16 | VS write to sie.SSIE ineffective when hideleg[2]=0 | hideleg[2]=0, VS-mode writes sie.SSIE=1, read hie.VSSIE | hie.VSSIE=0 (write ineffective) |
| VSIE-17 | vsie direct CSR read/write verification | M-mode writes vsie(0x204) SEIE|STIE|SSIE, reads back | Read-back matches written value |
| VSIE-18 | vsip.SSIP M-mode writable verification | hideleg[2]=1, M-mode writes vsip(0x244) SSIP=1, reads back | vsip.SSIP=1 |
| VSIE-19 | vsip alias chain M-mode perspective verification (VSSI) | hideleg[2]=1, hvip.VSSIP=1, M-mode reads vsip | vsip.SSIP=1 |
| VSIE-20 | vsip alias chain M-mode perspective verification (VSEI) | hideleg[10]=1, hvip.VSEIP=1, M-mode reads vsip | vsip.SEIP=1 (bit 9) |
| VSIE-21 | hideleg[13] writability probe (Shlcofideleg) | Write hideleg[13]=1, read back to detect | Bit sticks, indicating extension implemented |
| VSIE-22 | vsip/vsie LCOFI read-only zero when hideleg[13]=0 | hideleg[13]=0, read vsip.LCOFIP and vsie.LCOFIE | Both are 0 |

---

## Group 7. vstimecmp Register

**Specification References**:
- `norm:vstimecmp_sz`: 64-bit register
- `norm:vstimecmp_acc`: In RV32, vstimecmp/vstimecmph access the lower and upper 32 bits respectively
- `norm:hip_vstip_op`: VSTIP is set when (time + htimedelta) >= vstimecmp
- `norm:hip_vstip_clear`: VSTIP is cleared when writing vstimecmp to a value greater than (time + htimedelta)
- `norm:hip_vstip_enable`: When V=1, handled according to standard interrupt enable/delegation rules

**Test Responsibilities**: Verify the triggering and clearing of VS timer interrupts.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSTC-01 | Basic read/write of vstimecmp | HS-mode writes vstimecmp=0x12345678, reads back | Reads back the correct value |
| VSTC-02 | vstimecmp triggers VSTIP | Set vstimecmp = current value of (time + htimedelta) - 1 | hip.VSTIP=1 |
| VSTC-03 | vstimecmp clears VSTIP | After VSTIP=1, write vstimecmp to a value much greater than (time + htimedelta) | hip.VSTIP=0 |
| VSTC-04 | VS-mode accesses vstimecmp via stimecmp | VS-mode csrw stimecmp writes a value, HS-mode csrr vstimecmp reads back | Values match |
| VSTC-05 | vstimecmp interrupt delegated to VS-mode | hideleg[6]=1, hie.VSTIE=1, vsie.STIE=1, trigger vstimecmp | VS-mode receives timer interrupt (cause=5) |
| VSTC-06 | vstimecmp interrupt traps to HS-mode | hideleg[6]=0, trigger vstimecmp | HS-mode receives interrupt |
| VSTC-07 | htimedelta affects vstimecmp comparison | Set htimedelta=N, vstimecmp=time+N | Immediately triggers VSTIP |

---

## Group 8. vsscratch / vsepc / vscause / vstval Registers

**Specification References**:
- `norm:vsscratch_sz_acc_op`: VSXLEN-bit read/write, replaces sscratch when V=1
- `norm:vspec_sz_acc_op`: VSXLEN-bit read/write, replaces sepc when V=1
- `norm:vsepc_warl`: vsepc is WARL, holds the same value range as sepc
- `norm:vscause_sz_acc_op`: VSXLEN-bit read/write, replaces scause when V=1
- `norm:vscause_wlrl`: vscause is WLRL, holds the same value range as scause
- `norm:vstval_sz_acc_op` / `norm:vstval_warl`: Replaces stval when V=1

**Test Responsibilities**: Verify read/write operations, WARL/WLRL constraints, and isolation when V=0 for these VS CSRs.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSCR-01 | Basic read/write of vsscratch | HS-mode writes vsscratch=0xABCD, reads back | Reads back 0xABCD |
| VSCR-02 | vsepc WARL verification | HS-mode writes vsepc=odd address (e.g., 0x1001) | Read-back value complies with WARL constraint (lower bits may be cleared) |
| VSCR-03 | vscause WLRL verification | HS-mode writes vscause=legal cause value, reads back | Reads back correct value |
| VSCR-04 | vstval WARL verification | HS-mode writes vstval=all ones, reads back | Read-back value complies with WARL constraint |
| VSCR-05 | Correct write to vsepc/vscause/vstval on V=1 trap | VS-mode triggers exception, trap delegated to VS-mode | vsepc=fault PC, vscause=correct cause, vstval=correct value |
| VSCR-06 | VS CSR does not affect behavior when V=0 | Write vsepc/vscause/vstval when V=0, check HS-mode trap behavior | HS-mode trap writes sepc/scause/stval, unaffected by VS CSR |
| VSCR-07 | V=1 CSR substitution (sscratch/sepc/scause/stval) | When V=1, VS-mode accesses sscratch/sepc/scause/stval | Actually accesses vsscratch/vsepc/vscause/vstval (substitution takes effect) |

---

## Group 9. hedeleg / hideleg Delegation Register Field Constraints

> This group is split from Group 3 of the original `Hypervisor_test_plan_en.md`, keeping only the CSR bit-field attribute verification cases (DELEG-01/02/03); exception delegation chain cases are in `Hypervisor_Exceptions_test_plan_en.md`, and interrupt delegation with interrupt number translation cases are in `Hypervisor_Interrupts_test_plan_en.md`.

**Specification References**:
- `norm:hedeleg_sz_acc`: hedeleg is a 64-bit read/write register
- `norm:hideleg_sz_acc`: hideleg is an HSXLEN-bit read/write register
- `norm:hedeleg_acc`: Writable/read-only constraints for each hedeleg bit (hedeleg-bits table); bit 0 writability depends on IALIGN
- `norm:hideleg_acc`: hideleg bits 10/6/2 are writable, bits 12/9/5/1 are read-only zero

**Test Responsibilities**: Verify the bit-field WARL attributes (writable bits and read-only-zero bits) of the hedeleg/hideleg delegation registers, without involving trap delivery behavior.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| DELEG-01 | hedeleg writable bit verification | Write hedeleg bit by bit, read back to verify writable/read-only attributes | bits 1-8,12,13,15,18 writable; bits 9-11,16,19-23 read-only zero |
| DELEG-02 | hedeleg bit 0 writability depends on IALIGN | Write hedeleg bit 0 and read back | Writable when IALIGN=32, otherwise read-only zero |
| DELEG-03 | hideleg writable bit verification | Write hideleg bits 0-15, read back to verify | bits 10/6/2 writable, bits 12/9/5/1 read-only zero |
