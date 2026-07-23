**[中文](../testplan/Hypervisor_test_plan.md) | English**

# Hypervisor Extension Comprehensive Test Plan

## Overview

This test plan covers all core functionality points of the RISC-V Hypervisor (H) extension, excluding G-stage / VS-stage address translation. Address translation related tests are already covered by `hyp_gstage_translation_test_plan.md` and `hyp_2_stage_translation_test_plan.md`, and are not repeated here.

Sub-extensions already covered by independent test plans (Sha/Shcounterenw/Shgatpa/Shvsatpa/Shtvala/Shvstvala/Shvstvecd) are also out of scope for this document.

This test plan is written based on specification points (norm tags) in `SPEC/hypervisor.adoc`.

### SPEC Chapters Covered by This Document
- Hypervisor and Virtual Supervisor CSRs (hstatus, hedeleg, hideleg, hvip, hip, hie, hgeip, hgeie, henvcfg, hcounteren, htimedelta, htval, htinst, hgatp CSR behavior)
- Virtual Supervisor CSRs (vsstatus, vsip, vsie, vstvec, vsscratch, vsepc, vscause, vstval, vsatp CSR behavior, vstimecmp)
- Machine-Level CSR Enhancements (mstatus MPV/GVA/TVM/MPRV, mideleg, mip/mie, mtval2, mtinst)
- Hypervisor Instructions (HLV/HLVX/HSV exception scenarios, HFENCE.VVMA/HFENCE.GVMA exception scenarios)
- Traps (virtual-instruction exception full scenarios, trap entry/return, exception priority, htinst/mtinst transformed instructions)

### Covered by Other Test Plans
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

This section lists all specification points (norm IDs) referenced in Groups 1-19 of this document, deduplicated and sorted alphabetically.

| Norm ID | English Description |
|---------|---------------------|
| `norm:H_cause_ecall` | HS-mode and VS-mode ECALLs use different cause values so they can be delegated separately. |
| `norm:H_cause_virtual_instruction` | When V=1, a virtual-instruction exception (code 22) is normally raised instead of an illegal-instruction exception if the attempted instruction is HS-qualified but is prevented from executing when V=1. An instruction is HS-qualified if it would be valid to execute in HS-mode, assuming TSR and TVM of `mstatus` are both zero. |
| `norm:H_cause_virtual_instruction_high` | When V=1 and XLEN=32, an invalid attempt to access a high-half CSR raises a virtual-instruction exception instead of an illegal-instruction exception if the same CSR instruction for the corresponding low-half CSR is HS-qualified. |
| `norm:H_exception_priority` | If an instruction may raise multiple synchronous exceptions, the decreasing priority order indicates which exception is taken and reported in `mcause` or `scause`. |
| `norm:H_illegalinst_xstatus_fs_vs` | Fields FS and VS in registers `sstatus` and `vsstatus` deviate from the usual HS-qualified rule. If an instruction is prevented from executing because FS or VS is zero in either `sstatus` or `vsstatus`, the exception raised is always an illegal-instruction exception, never a virtual-instruction exception. |
| `norm:H_scsrs_nomatch` | Some standard supervisor CSRs (`senvcfg`, `scounteren`, and `scontext`, possibly others) have no matching VS CSR. These supervisor CSRs continue to have their usual function and accessibility even when V=1, except with VS-mode and VU-mode substituting for HS-mode and U-mode. |
| `norm:H_trap_deleg` | When a trap occurs in HS-mode or U-mode, it goes to M-mode, unless delegated by `medeleg` or `mideleg`, in which case it goes to HS-mode. When a trap occurs in VS-mode or VU-mode, it goes to M-mode, unless delegated by `medeleg`/`mideleg` to HS-mode, unless further delegated by `hedeleg`/`hideleg` to VS-mode. |
| `norm:H_trap_hs_csrwrites` | When a trap is taken into HS-mode, V is set to 0, and `hstatus`.SPV and `sstatus`.SPP are set accordingly. If V was 1 before the trap, SPVP is set the same as `sstatus`.SPP; otherwise, SPVP is left unchanged. A trap into HS-mode also writes GVA in `hstatus`, SPIE and SIE in `sstatus`, and CSRs `sepc`, `scause`, `stval`, `htval`, and `htinst`. |
| `norm:H_trap_m_csrwrites` | When a trap is taken into M-mode, V gets set to 0, and fields MPV and MPP in `mstatus` are set accordingly. A trap into M-mode also writes fields GVA, MPIE, and MIE in `mstatus` and writes CSRs `mepc`, `mcause`, `mtval`, `mtval2`, and `mtinst`. |
| `norm:H_trap_vs_csrwrites` | When a trap is taken into VS-mode, `vsstatus`.SPP is set accordingly. Register `hstatus` and the HS-level `sstatus` are not modified, and V remains 1. A trap into VS-mode also writes SPIE and SIE in `vsstatus` and writes CSRs `vsepc`, `vscause`, and `vstval`. |
| `norm:H_trap_xtinst` | On any trap into M-mode or HS-mode, one of these values is written to `mtinst` or `htinst`: zero; a transformation of the trapping instruction; a custom value (only if the trapping instruction is non-standard); or a special pseudoinstruction. |
| `norm:H_trap_xtinst_guestpage` | For guest-page faults, the trap instruction register is written with a special pseudoinstruction value if: (a) the fault is caused by an implicit memory access for VS-stage address translation, and (b) a nonzero value is written to `mtval2` or `htval`. If both conditions are met, zero is not allowed. |
| `norm:H_trap_xtinst_guestpage_rw` | A write pseudoinstruction (0x00002020 or 0x00003020) is used for the case that the machine is attempting automatically to update bits A and/or D in VS-level page tables. All other implicit memory accesses for VS-stage address translation will be reads. |
| `norm:H_trap_xtinst_interrupt` | On an interrupt, the value written to the trap instruction register is always zero. |
| `norm:hcounteren_sz_acc_op` | The `hcounteren` CSR is a 32-bit read/write register that controls availability of performance monitoring counters to VS-mode and VU-mode. |
| `norm:hedeleg_sz_acc_op` | The `hedeleg` register is an HSXLEN-bit read/write register that allows traps to be delegated from HS-mode to VS-mode. |
| `norm:henvcfg_sz_acc_op` | The `henvcfg` CSR is an HSXLEN-bit read/write register that controls environment configuration for VS-mode and VU-mode. |
| `norm:hgeie_sz_acc_op` | The `hgeie` register is an HSXLEN-bit read/write register that enables guest external interrupts. |
| `norm:hgeip_sz_acc_op` | The `hgeip` register is an HSXLEN-bit read-only register that indicates pending guest external interrupts. |
| `norm:hgatp_sz_acc_op` | The `hgatp` CSR is an HSXLEN-bit read/write register that controls G-stage address translation. |
| `norm:hideleg_sz_acc_op` | The `hideleg` register is an HSXLEN-bit read/write register that allows interrupts to be delegated from HS-mode to VS-mode. |
| `norm:hie_sz_acc_op` | The `hie` register is an HSXLEN-bit read/write register that enables hypervisor interrupts. |
| `norm:hip_sz_acc_op` | The `hip` register is an HSXLEN-bit read/write register that indicates pending hypervisor interrupts. |
| `norm:hstatus_gva_op` | Field GVA is written by the implementation whenever a trap is taken into HS-mode. For any trap that writes a guest virtual address to `htval`, GVA is set to 1. For any other trap into HS-mode, GVA is set to 0. |
| `norm:hstatus_hu_op` | The HU field controls whether virtual-machine load/store instructions (HLV, HLVX, HSV) may also be executed in U-mode. When HU=1, these instructions may be executed in U-mode the same as in HS-mode. When HU=0, all hypervisor instructions cause an illegal-instruction exception in U-mode. |
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
| `norm:hvip_acc` | The standard portion (bits 15:0) of `hvip` is formatted as shown. Bits VSEIP, VSTIP, and VSSIP of `hvip` are writable. |
| `norm:hvip_sz_op` | Register `hvip` is an HSXLEN-bit read/write register that a hypervisor can write to indicate virtual interrupts intended for VS-mode. Bits of `hvip` that are not writable are read-only zeros. |
| `norm:mideleg_acc_h` | When the hypervisor extension is implemented, bits 10, 6, and 2 of `mideleg` are each read-only one. Furthermore, if GEILEN is nonzero, bit 12 of `mideleg` is also read-only one. VS-level interrupts and guest external interrupts are always delegated past M-mode to HS-mode. |
| `norm:mideleg_hroz` | For bits of `mideleg` that are zero, the corresponding bits in `hideleg`, `hip`, and `hie` are read-only zeros. |
| `norm:mip_mie_alias` | Bits SGEIP, VSEIP, VSTIP, and VSSIP in `mip` are aliases for the same bits in hypervisor CSR `hip`, while SGEIE, VSEIE, VSTIE, and VSSIE in `mie` are aliases for the same bits in `hie`. |
| `norm:mip_mie_vs` | The hypervisor extension gives registers `mip` and `mie` additional active bits for the hypervisor-added interrupts. |
| `norm:mret_h` | MRET first determines the new privilege mode according to MPP and MPV in `mstatus`. MRET then sets MPV=0, MPP=0, MIE=MPIE, and MPIE=1. Lastly, MRET sets the privilege mode as previously determined, and sets pc=mepc. |
| `norm:mstatus_gva_op` | Field GVA is written by the implementation whenever a trap is taken into M-mode. For any trap that writes a guest virtual address to `mtval`, GVA is set to 1. For any other trap into M-mode, GVA is set to 0. |
| `norm:mstatus_modes` | The TSR and TVM fields of `mstatus` affect execution only in HS-mode, not in VS-mode. The TW field affects execution in all modes except M-mode. |
| `norm:mstatus_mprv_hlsv` | MPRV does not affect the virtual-machine load/store instructions, HLV, HLVX, and HSV. The explicit loads and stores of these instructions always act as though V=1 and the nominal privilege mode were `hstatus`.SPVP, overriding MPRV. |
| `norm:mstatus_mprv_hypervisor` | The hypervisor extension changes the behavior of MPRV. When MPRV=0, normal translation. When MPRV=1, explicit memory accesses are translated and protected as though the current virtualization mode were set to MPV and the current nominal privilege mode were set to MPP. |
| `norm:mstatus_mpv_op` | The MPV bit is written by the implementation whenever a trap is taken into M-mode, set to the value of V at the time of the trap. When an MRET instruction is executed, V is set to MPV, unless MPP=3, in which case V remains 0. |
| `norm:mstatus_tvm_hs` | Setting TVM=1 prevents HS-mode from accessing `hgatp` or executing HFENCE.GVMA or HINVAL.GVMA, but has no effect on accesses to `vsatp` or instructions HFENCE.VVMA or HINVAL.VVMA. |
| `norm:mtinst_sz_acc_op` | The `mtinst` register is an MXLEN-bit read/write register. When a trap is taken into M-mode, `mtinst` is written with a value that, if nonzero, provides information about the instruction that trapped. |
| `norm:mtinst_val` | `mtinst` is a WARL register that need only be able to hold the values that the implementation may automatically write to it on a trap. |
| `norm:mtval2_sz_acc_op` | The `mtval2` register is an MXLEN-bit read/write register. When a trap is taken into M-mode, `mtval2` is written with additional exception-specific information, alongside `mtval`. |
| `norm:mtval2_trapval` | When a guest-page-fault trap is taken into M-mode, `mtval2` is written with either zero or the guest physical address that faulted, shifted right by 2 bits. For other traps, `mtval2` is set to zero. |
| `norm:mtval2_trapval_vstrans` | If a guest-page fault is due to an implicit memory access during first-stage (VS-stage) address translation, a guest physical address written to `mtval2` is that of the implicit memory access that faulted. |
| `norm:sie_hip_hie_mutex` | For each writable bit in `sie`, the corresponding bit shall be read-only zero in both `hip` and `hie`. Hence, the nonzero bits in `sie` and `hie` are always mutually exclusive, and likewise for `sip` and `hip`. |
| `norm:sret_dt` | If the Ssdbltrp extension is implemented, when SRET is executed in HS-mode, if the new privilege mode is VU, the SRET instruction sets `vsstatus`.SDT to 0. When executed in VS-mode, `vsstatus`.SDT is set to 0. |
| `norm:sret_v0` | When executed in M-mode or HS-mode (V=0), SRET first determines the new privilege mode according to `hstatus`.SPV and `sstatus`.SPP. SRET then sets `hstatus`.SPV=0, and in `sstatus` sets SPP=0, SIE=SPIE, and SPIE=1. Lastly, SRET sets the privilege mode and sets pc=sepc. |
| `norm:sret_v1` | When executed in VS-mode (V=1), SRET sets the privilege mode accordingly, in `vsstatus` sets SPP=0, SIE=SPIE, and SPIE=1, and lastly sets pc=vsepc. |
| `norm:time_htimedelta_req` | If the `time` CSR is implemented, `htimedelta` (and `htimedeltah` for XLEN=32) must be implemented. |
| `norm:vscause_sz_acc_op` | The `vscause` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `scause`. When V=1, `vscause` substitutes for the usual `scause`. When V=0, `vscause` does not directly affect the behavior of the machine. |
| `norm:vscause_wlrl` | `vscause` is a WLRL register that must be able to hold the same set of values that `scause` can hold. |
| `norm:vsepc_warl` | `vsepc` is a WARL register that must be able to hold the same set of values that `sepc` can hold. |
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
## Group 1. CSR Substitution Mechanism when V=1

**Specification References**:
- `norm:H_vscsrs_sub`: When V=1, VS CSRs substitute for corresponding S CSRs; instructions accessing S CSRs actually access VS CSRs
- `norm:H_vscsrs_acc_vs`: When V=1, direct access to VS CSR by its own address triggers virtual-instruction exception
- `norm:H_vscsrs_acc_u`: U-mode access triggers illegal-instruction exception
- `norm:H_vscsrs_acc_m_hs`: VS CSRs can only be accessed by their own address in M/HS-mode
- `norm:H_vscsrs_v1`: When V=1, HS-level S CSRs retain values but do not affect behavior
- `norm:H_vscsrs_v0`: When V=0, VS CSRs do not affect behavior
- `norm:H_scsrs_nomatch`: Behavior of S CSRs without matching VS CSRs (senvcfg/scounteren/scontext) when V=1

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

## Group 3. hedeleg / hideleg Delegation Registers

**Specification References**:
- `norm:hedeleg_sz_acc`: hedeleg is a 64-bit read/write register
- `norm:hideleg_sz_acc`: hideleg is an HSXLEN-bit read/write register
- `norm:hedeleg_op`: When V=1, exceptions delegated by medeleg are further delegated to VS-mode if the corresponding hedeleg bit is set
- `norm:hedeleg_acc`: Writable/read-only constraints for each hedeleg bit (hedeleg-bits table)
- `norm:hideleg_op`: Interrupts delegated by mideleg are delegated to VS-mode if the corresponding hideleg bit is set
- `norm:hideleg_acc`: hideleg bits 10/6/2 are writable, bits 12/9/5/1 are read-only zero
- `norm:hideleg_trans`: VS-level interrupt number translation (10→9, 6→5, 2→1)

**Test Responsibilities**: Verify exception/interrupt delegation chain and CSR field constraints.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| DELEG-01 | hedeleg writable bit verification | Write hedeleg bit by bit, read back to verify writable/read-only attributes | bits 1-8,12,13,15,18 writable; bits 9-11,16,19-23 read-only zero |
| DELEG-02 | hedeleg bit 0 writability depends on IALIGN | Write hedeleg bit 0 and read back | Writable when IALIGN=32, otherwise read-only zero |
| DELEG-03 | hideleg writable bit verification | Write hideleg bits 0-15, read back to verify | bits 10/6/2 writable, bits 12/9/5/1 read-only zero |
| DELEG-04 | hedeleg delegates illegal-instruction to VS | Set medeleg[2]=1, hedeleg[2]=1, VS-mode triggers illegal instruction | Trap enters VS-mode (vscause=2) |
| DELEG-05 | Trap to HS when hedeleg not delegating | Set medeleg[2]=1, hedeleg[2]=0, VS-mode triggers illegal instruction | Trap enters HS-mode (scause=2) |
| DELEG-06 | hedeleg delegates breakpoint to VS | Set medeleg[3]=1, hedeleg[3]=1, VS-mode executes EBREAK | Trap enters VS-mode (vscause=3) |
| DELEG-07 | hedeleg delegates ecall-from-VU to VS | Set medeleg[8]=1, hedeleg[8]=1, VU-mode executes ECALL | Trap enters VS-mode (vscause=8) |
| DELEG-08 | hideleg delegates VSSI to VS | Set hideleg[2]=1, inject VSSIP, VS-mode enables interrupt | Trap enters VS-mode (vscause=1, after translation) |
| DELEG-09 | hideleg delegates VSTI to VS | Set hideleg[6]=1, inject VSTIP, VS-mode enables interrupt | Trap enters VS-mode (vscause=5, after translation) |
| DELEG-10 | hideleg delegates VSEI to VS | Set hideleg[10]=1, inject VSEIP, VS-mode enables interrupt | Trap enters VS-mode (vscause=9, after translation) |
| DELEG-11 | Interrupt traps to HS when hideleg not delegating | Set hideleg[2]=0, inject VSSIP | Trap enters HS-mode |
| DELEG-12 | Interrupt number translation verification VSSI→SSI | hideleg[2]=1 delegates VSSIP to VS-mode | vscause records cause=1 (not 2) |
| DELEG-13 | Interrupt number translation verification VSTI→STI | hideleg[6]=1 delegates VSTIP to VS-mode | vscause records cause=5 (not 6) |
| DELEG-14 | Interrupt number translation verification VSEI→SEI | hideleg[10]=1 delegates VSEIP to VS-mode | vscause records cause=9 (not 10) |
| DELEG-15 | guest-page-fault cannot be delegated to VS | Verify hedeleg bits 20/21/23 are read-only zero | guest-page-fault always traps to HS-mode |
| DELEG-16 | virtual-instruction cannot be delegated to VS | Verify hedeleg bit 22 is read-only zero | virtual-instruction exception always traps to HS-mode |

---

## Group 4. hvip / hip / hie Interrupt Registers

**Specification References**:
- `norm:hvip_sz_op` / `norm:hvip_acc`: hvip VSEIP/VSTIP/VSSIP are writable
- `norm:hip_hie_sz_acc` / `norm:hip_op` / `norm:hie_op`: hip indicates pending, hie indicates enable
- `norm:sie_hip_hie_mutex`: Writable bits in sie are read-only zero in hip/hie, and vice versa
- `norm:hideleg_hs`: Conditions for interrupt trap to HS-mode
- `norm:hip_vseip_vseie_op`: VSEIP = hvip.VSEIP OR hgeip[VGEIN] OR platform signal
- `norm:hip_vstip_vstie_acc_op`: VSTIP = hvip.VSTIP OR vstimecmp trigger
- `norm:hip_vssip_vssie_op`: hip.VSSIP is an alias of hvip.VSSIP
- `norm:hsint_priority`: HS-mode interrupt priority SEI > SSI > STI > SGEI > VSEI > VSSI > VSTI > LCOFI

**Test Responsibilities**: Verify interrupt injection, pending/enable mechanism, priority, and mutual exclusion relationships.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HINT-01 | hvip.VSSIP write injects VS software interrupt | HS-mode writes hvip.VSSIP=1, configure hideleg/hie/vsie | VS-mode receives VS software interrupt |
| HINT-02 | hvip.VSTIP write injects VS timer interrupt | HS-mode writes hvip.VSTIP=1, configure hideleg/hie/vsie | VS-mode receives VS timer interrupt |
| HINT-03 | hvip.VSEIP write injects VS external interrupt | HS-mode writes hvip.VSEIP=1, configure hideleg/hie/vsie | VS-mode receives VS external interrupt |
| HINT-04 | hip.VSSIP is bidirectional writable alias of hvip.VSSIP | Read direction: write hvip.VSSIP=1, read hip.VSSIP=1; Write direction: clear hvip.VSSIP=0, then write hip.VSSIP=1 directly, read hvip.VSSIP | Bidirectional: writing hip.VSSIP is equivalent to writing hvip.VSSIP |
| HINT-05 | hip.VSEIP read-only (multi-source OR) | Combo 1: hvip.VSEIP=1, write hip.VSEIP=0, hip.VSEIP remains 1; Combo 2: hvip.VSEIP=0, write hip.VSEIP=1, hip.VSEIP remains 0 | Write ignored in both combos; hip.VSEIP always reflects hvip.VSEIP |
| HINT-06 | hip.VSTIP read-only | Combo 1: hvip.VSTIP=1, write hip.VSTIP=0, hip.VSTIP remains 1; Combo 2: hvip.VSTIP=0, write hip.VSTIP=1, hip.VSTIP remains 0 | Write ignored in both combos; hip.VSTIP always reflects hvip.VSTIP |
| HINT-07 | hie VSEIE/VSTIE/VSSIE writable | Write hie VSEIE/VSTIE/VSSIE bits | Normal read/write |
| HINT-08 | sie and hip/hie mutual exclusion | Check if writable bits in sie are read-only zero in hip/hie | Mutual exclusion relationship holds |
| HINT-09 | Clearing hvip.VSSIP clears interrupt | Write hvip.VSSIP=0 | VS software interrupt cleared |
| HINT-10 | Interrupt traps to HS when hideleg=0 | hideleg[2]=0, inject VSSIP | Interrupt traps to HS-mode |
| HINT-11 | Interrupt traps to VS when hideleg=1 | hideleg[2]=1, inject VSSIP | Interrupt traps to VS-mode |
| HINT-12 | HS-mode interrupt priority SEI > SSI | SEI and SSI pending simultaneously | SEI handled first |
| HINT-13 | HS-mode interrupt priority VSEI > VSSI > VSTI | Multiple VS interrupts pending simultaneously | In order VSEI > VSSI > VSTI |
| HINT-14 | hip/hie non-standard bits read-only zero | Read reserved bits of hip/hie | All zero |
| HINT-15 | sstatus.SIE=0 masks interrupt in HS-mode | hideleg[2]=0, inject VSSIP, hie.VSSIE=1, enter HS-mode with SIE=0 | Interrupt not delivered; then set SIE=1, interrupt fires immediately |
| HINT-16 | hvip non-writable bits read-only zero | Write hvip all 1s, read back | Only bits 2/6/10 (VSSIP/VSTIP/VSEIP) are 1, all others are 0 |
| HINT-17 | hip.VSTIP remains defined at V=0 | In V=0 (HS-mode), set/clear hvip.VSTIP, read hip.VSTIP | hip.VSTIP reflects hvip.VSTIP at V=0 (defined behavior) |
| HINT-18 | HS-mode interrupt priority SSI > STI | Both SSI and STI pending and enabled, enter HS-mode | SSI (cause=1) delivered first, proving SSI > STI |

---

## Group 5. hgeip / hgeie Registers

**Specification References**:
- `norm:hgeip_sz_acc_op`: hgeip is an HSXLEN-bit read-only register
- `norm:hgeie_sz_acc_op`: hgeie is an HSXLEN-bit read/write register
- `norm:hgeip_hgeie_fields`: Guest external interrupt number i corresponds to bit i
- `norm:geilen`: GEILEN count is implementation-defined, can be zero
- `norm:hgeie_op`: hgeie selects subset that triggers SGEI, does not affect VS-level external interrupt selected by VGEIN

**Test Responsibilities**: Verify guest external interrupt register field constraints and basic functionality.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HGEI-01 | hgeip read-only verification | Attempt to write hgeip | Write ignored |
| HGEI-02 | hgeie writable bit range | Write hgeie all ones, read back to determine GEILEN | bits GEILEN:1 writable, bit 0 read-only zero |
| HGEI-03 | hgeie bit 0 read-only zero | Write hgeie bit 0 = 1 | bit 0 reads back 0 |
| HGEI-04 | hgeip AND hgeie non-zero triggers SGEIP | If GEILEN>0, configure hgeie to enable corresponding bit, trigger guest external interrupt | hip.SGEIP=1 |
| HGEI-05 | hgeie/hgeip all zero when GEILEN=0 | If GEILEN=0, read hgeie/hgeip | All zero |
## Group 6. henvcfg Register

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

---

## Group 7. htimedelta Register

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

## Group 8. vsstatus Register

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

## Group 9. vsip / vsie Registers

**Specification References**:
- `norm:vsip_vsie_sz_acc_op`: Substitutes for sip/sie when V=1
- `norm:vsip_vsie_sei`: When hideleg[10]=0, vsip.SEIP/vsie.SEIE are read-only zero; otherwise they are aliases of hip.VSEIP/hie.VSEIE
- `norm:vsip_vsie_sti`: When hideleg[6]=0, vsip.STIP/vsie.STIE are read-only zero; otherwise they are aliases of hip.VSTIP/hie.VSTIE
- `norm:vsip_vsie_ssi`: When hideleg[2]=0, vsip.SSIP/vsie.SSIE are read-only zero; otherwise they are aliases of hip.VSSIP/hie.VSSIE

**Test Responsibilities**: Verify the substitution mechanism and alias relationships of vsip/vsie.

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

---

## Group 10. vstimecmp Register

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
## Group 11. vsscratch / vsepc / vscause / vstval Registers

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

---

## Group 12. Virtual-Instruction Exception

**Specification References**:
- `norm:H_cause_virtual_instruction`: HS-qualified instruction causes virtual-instruction exception (cause=22) when V=1 due to insufficient privilege or being disabled
- `norm:H_cause_virtual_instruction_high`: Special rules for high-half CSR when V=1 and XLEN=32
- `norm:H_virtinst_vu_vs_hinst`: VS/VU-mode executes HLV/HLVX/HSV/HFENCE
- `norm:H_virtinst_vu_vs_nonhigh_allowedhs_tvm0`: VS/VU-mode accesses H CSR / VS CSR
- `norm:H_virtinst_vu_wfi_tw0` / `norm:H_virtinst_vu_sret_sfence`: VU-mode executes WFI/SRET/SFENCE
- `norm:H_virtinst_vu_nonhigh_supervisor_allowedhs_tvm0`: VU-mode accesses S CSR
- `norm:H_virtinst_wfi_vtw1_tw0`: VS-mode WFI + VTW=1 + TW=0
- `norm:H_virtinst_vs_sret_vtsr1`: VS-mode SRET + VTSR=1
- `norm:H_virtinst_vs_sfence_sinval_satp_vtvm1`: VS-mode SFENCE/SINVAL.VMA/satp + VTVM=1
- `norm:H_virtinst_xtval`: stval/mtval same as illegal-instruction on virtual-instruction trap
- `norm:H_illegalinst_xstatus_fs_vs`: Always illegal-instruction (not virtual-instruction) when FS/VS=0

**Test Responsibilities**: Systematically cover all scenarios that trigger virtual-instruction exception and verify distinction from illegal-instruction.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VINST-01 | VS-mode executes HLV | VS-mode executes HLV.W | virtual-instruction exception (cause=22) |
| VINST-02 | VS-mode executes HSV | VS-mode executes HSV.W | virtual-instruction exception (cause=22) |
| VINST-03 | VS-mode executes HLVX | VS-mode executes HLVX.WU | virtual-instruction exception (cause=22) |
| VINST-04 | VS-mode executes HFENCE.VVMA | VS-mode executes HFENCE.VVMA | virtual-instruction exception (cause=22) |
| VINST-05 | VS-mode executes HFENCE.GVMA | VS-mode executes HFENCE.GVMA | virtual-instruction exception (cause=22) |
| VINST-06 | VU-mode executes HLV | VU-mode executes HLV.W | virtual-instruction exception (cause=22) |
| VINST-07 | VU-mode executes HSV | VU-mode executes HSV.W | virtual-instruction exception (cause=22) |
| VINST-08 | VU-mode executes HLVX | VU-mode executes HLVX.WU | virtual-instruction exception (cause=22) |
| VINST-09 | VU-mode executes HFENCE.VVMA | VU-mode executes HFENCE.VVMA | virtual-instruction exception (cause=22) |
| VINST-10 | VU-mode executes HFENCE.GVMA | VU-mode executes HFENCE.GVMA | virtual-instruction exception (cause=22) |
| VINST-11 | VS-mode accesses hstatus | VS-mode csrr hstatus | virtual-instruction exception (cause=22) |
| VINST-12 | VS-mode accesses hideleg | VS-mode csrr hideleg | virtual-instruction exception (cause=22) |
| VINST-13 | VS-mode accesses hie | VS-mode csrr hie | virtual-instruction exception (cause=22) |
| VINST-14 | VS-mode accesses hgeie | VS-mode csrr hgeie | virtual-instruction exception (cause=22) |
| VINST-15 | VS-mode accesses hcounteren | VS-mode csrr hcounteren | virtual-instruction exception (cause=22) |
| VINST-16 | VS-mode accesses htval | VS-mode csrr htval | virtual-instruction exception (cause=22) |
| VINST-17 | VS-mode accesses hip | VS-mode csrr hip | virtual-instruction exception (cause=22) |
| VINST-18 | VS-mode accesses hvip | VS-mode csrr hvip | virtual-instruction exception (cause=22) |
| VINST-19 | VS-mode accesses htinst | VS-mode csrr htinst | virtual-instruction exception (cause=22) |
| VINST-20 | VS-mode accesses hgatp | VS-mode csrr hgatp | virtual-instruction exception (cause=22) |
| VINST-21 | VU-mode accesses S CSR | VU-mode csrr sstatus | virtual-instruction exception (cause=22) |
| VINST-22 | Vector extension disabled in VS-mode | mstatus.VS=0 or vsstatus.VS=0, VS-mode executes Vector | illegal-instruction exception (cause=2), not cause=22 |
| VINST-23 | stval correct on virtual-instruction trap | VS-mode triggers virtual-instruction exception | stval same as illegal-instruction trap encoding |
| VINST-24 | mstatus.TW=1 overrides VTW (illegal not virtual) | mstatus.TW=1, VS-mode WFI | illegal-instruction exception (cause=2) |

---

## Group 13. Trap Entry Behavior

**Specification References**:
- `norm:H_trap_deleg`: Delegation chain M→HS→VS
- `norm:H_trap_m_csrwrites`: On trap to M-mode: V→0, MPV/MPP←current mode, GVA/MPIE/MIE updated, mepc/mcause/mtval/mtval2/mtinst written
- `norm:H_trap_hs_csrwrites`: On trap to HS-mode: V→0, SPV/SPP←current mode, SPVP (updated only when V=1), GVA/SPIE/SIE updated, sepc/scause/stval/htval/htinst written
- `norm:H_trap_vs_csrwrites`: On trap to VS-mode: V remains 1, vsstatus.SPP updated, SPIE/SIE updated, vsepc/vscause/vstval written

**Test Responsibilities**: Verify automatic CSR write behavior during trap entry.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TENT-01 | CSR write on VS-mode trap to HS-mode | VS-mode ecall, trap to HS-mode | V=0, SPV=1, SPP=1, sepc=fault PC, scause=10 |
| TENT-02 | CSR write on VU-mode trap to HS-mode | VU-mode ecall, trap to HS-mode | V=0, SPV=1, SPP=0, sepc=fault PC, scause=8 |
| TENT-03 | SPVP update on VS-mode trap to HS-mode | VS-mode trap to HS-mode | hstatus.SPVP=1 (VS-mode is S-level) |
| TENT-04 | SPVP update on VU-mode trap to HS-mode | VU-mode trap to HS-mode | hstatus.SPVP=0 (VU-mode is U-level) |
| TENT-05 | SPVP unchanged on U-mode trap to HS-mode | Set SPVP=1 first, U-mode trap to HS-mode | hstatus.SPVP remains 1 (not updated when V=0) |
| TENT-06 | HS-mode trap to HS-mode | HS-mode ecall not delegated | SPV=0, SPP=1 |
| TENT-07 | GVA correct on trap to HS-mode | VS-mode guest-page-fault trap to HS-mode | hstatus.GVA=1 |
| TENT-08 | GVA=0 on trap to HS-mode | VS-mode ecall trap to HS-mode | hstatus.GVA=0 |
| TENT-09 | SIE/SPIE correct on trap to HS-mode | Record old sstatus.SIE value, trigger trap | SPIE=old SIE, SIE=0 |
| TENT-10 | CSR write on trap to VS-mode | Exception delegated to VS-mode | vsstatus.SPP correct, vsepc/vscause/vstval written |
| TENT-11 | hstatus not modified on trap to VS-mode | Exception delegated to VS-mode | hstatus unchanged, V remains 1 |
| TENT-12 | CSR write on VS-mode trap to M-mode | VS-mode exception not delegated by medeleg | MPV=1, MPP=1, mtval2/mtinst written |
| TENT-13 | VU-mode trap to M-mode | VU-mode exception not delegated by medeleg | MPV=1, MPP=0 |
| TENT-14 | HS-mode trap to M-mode | HS-mode exception | MPV=0, MPP=1 |
| TENT-15 | htval/htinst zero on non-guest-page-fault | VS-mode ecall trap to HS-mode | htval=0, htinst=0 |

---

## Group 14. Trap Return Behavior

**Specification References**:
- `norm:mret_h`: MRET determines new privilege level based on MPP/MPV, then MPV=0, MPP=0, MIE=MPIE, MPIE=1
- `norm:sret_v0`: When V=0, SRET determines new mode based on SPV/SPP, SPV=0, SPP=0, SIE=SPIE, SPIE=1
- `norm:sret_v1`: When V=1, SRET determines mode based on vsstatus.SPP, SPP=0, SIE=SPIE, SPIE=1
- `norm:sret_dt`: With Ssdbltrp, SRET clears vsstatus.SDT when in HS-mode and new mode is VU; clears vsstatus.SDT when in VS-mode

**Test Responsibilities**: Verify mode switching and CSR restoration behavior of MRET/SRET under H extension.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TRET-01 | MRET returns to VS-mode | Set MPV=1, MPP=1, execute MRET | Enter VS-mode (V=1), MPV=0, MPP=0 |
| TRET-02 | MRET returns to VU-mode | Set MPV=1, MPP=0, execute MRET | Enter VU-mode (V=1), MPV=0, MPP=0 |
| TRET-03 | MRET returns to HS-mode | Set MPV=0, MPP=1, execute MRET | Enter HS-mode (V=0) |
| TRET-04 | MRET returns to M-mode | Set MPP=3, execute MRET | Enter M-mode, V remains 0 |
| TRET-05 | MRET MIE/MPIE restoration | Set MPIE=1, execute MRET | MIE=1, MPIE=1 |
| TRET-06 | SRET(V=0) returns to VS-mode | Set hstatus.SPV=1, sstatus.SPP=1, execute SRET | Enter VS-mode (V=1), SPV=0, SPP=0 |
| TRET-07 | SRET(V=0) returns to VU-mode | Set hstatus.SPV=1, sstatus.SPP=0, execute SRET | Enter VU-mode (V=1), SPV=0, SPP=0 |
| TRET-08 | SRET(V=0) returns to HS-mode | Set hstatus.SPV=0, sstatus.SPP=1, execute SRET | Enter HS-mode (V=0) |
| TRET-09 | SRET(V=0) returns to U-mode | Set hstatus.SPV=0, sstatus.SPP=0, execute SRET | Enter U-mode (V=0) |
| TRET-10 | SRET(V=0) SIE/SPIE restoration | Set sstatus.SPIE=1, execute SRET | SIE=1, SPIE=1 |
| TRET-11 | SRET(V=1) returns to VS-mode | In VS-mode vsstatus.SPP=1, execute SRET | Return to VS-mode, SPP=0 |
| TRET-12 | SRET(V=1) returns to VU-mode | In VS-mode vsstatus.SPP=0, execute SRET | Return to VU-mode, SPP=0 |
| TRET-13 | SRET(V=1) SIE/SPIE restoration | vsstatus.SPIE=1, execute SRET | vsstatus.SIE=1, vsstatus.SPIE=1 |
| TRET-14 | SRET restores PC from sepc | Set sepc=target address, execute SRET | PC=sepc |
| TRET-15 | MRET restores PC from mepc | Set mepc=target address, execute MRET | PC=mepc |

---

## Group 15. htinst / mtinst Transformed Instructions

**Specification References**:
- `norm:H_trap_xtinst`: Value types written to mtinst/htinst on trap (zero/transformed instruction/custom/pseudoinstruction)
- `norm:H_trap_xtinst_interrupt`: Write zero on interrupt
- `norm:H_trap_xtinst_val`: Value types writable for each exception type (tinst-values table)
- `norm:H_trap_xtinst_guestpage`: Must write pseudoinstruction when implicit VS-stage access causes guest-page-fault and htval is non-zero
- `norm:H_trap_xtinst_guestpage_rw`: Use 0x00003000 for read, 0x00003020 for write (A/D update) (RV64)

**Test Responsibilities**: Verify htinst write values in various trap scenarios.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TINST-01 | htinst=0 on interrupt trap | VS-mode interrupt trap to HS-mode | htinst=0 |
| TINST-02 | htinst=0 or custom on ecall trap | VS-mode ecall | htinst=0 |
| TINST-03 | htinst value on load guest-page-fault | VS-mode load triggers guest-page-fault | htinst is 0 or transformed load instruction |
| TINST-04 | htinst value on store guest-page-fault | VS-mode store triggers guest-page-fault | htinst is 0 or transformed store instruction |
| TINST-05 | htinst is pseudoinstruction when implicit VS-stage access fault + htval non-zero | GPA where VS-stage page table resides has no mapping, triggers guest-page-fault | htinst=0x00003000 (RV64 read) |
| TINST-06 | Pseudoinstruction for implicit write (A/D update) | If implementation supports automatic A/D update, trigger implicit write fault | htinst=0x00003020 (RV64 write) |
| TINST-07 | Transformed instruction bit 1:0 encoding verification | 32-bit load triggers fault | htinst bits 1:0 = 11 (32-bit instruction) |
| TINST-08 | Compressed instruction transformed bit 1:0 encoding | 16-bit C.LW triggers fault | htinst bits 1:0 = 01 (compressed instruction) |
| TINST-09 | Page-fault does not produce pseudoinstruction | VS-mode load page-fault (not guest-page-fault) | htinst=0 or transformed instruction (not pseudoinstruction) |

---

## Group 16. mstatus Enhancements (Hypervisor Related)

**Specification References**:
- `norm:mstatus_mpv_op`: MPV field writes old value of V on trap to M-mode; on MRET V←MPV (except V remains 0 when MPP=3)
- `norm:mstatus_gva_op`: M-mode GVA field
- `norm:mstatus_modes`: TSR/TVM only affects HS-mode, TW affects all non-M-mode
- `norm:mstatus_tvm_hs`: TVM=1 prevents HS-mode from accessing hgatp and executing HFENCE.GVMA, does not affect vsatp/HFENCE.VVMA
- `norm:mstatus_mprv_hypervisor`: MPRV=1 + MPV controls two-stage translation triggering
- `norm:mstatus_mprv_hlsv`: MPRV does not affect HLV/HLVX/HSV

**Test Responsibilities**: Verify behavior of Hypervisor-related fields in mstatus.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| MSTAT-01 | MPV written as 1 on VS trap to M-mode | VS-mode exception traps to M-mode | mstatus.MPV=1 |
| MSTAT-02 | MPV written as 0 on HS trap to M-mode | HS-mode exception traps to M-mode | mstatus.MPV=0 |
| MSTAT-03 | V←MPV on MRET | Set MPV=1, MPP=1, MRET | V=1 (enter VS-mode) |
| MSTAT-04 | V remains 0 when MRET MPP=3 | Set MPV=1, MPP=3, MRET | V=0 (enter M-mode, MPV ignored) |
| MSTAT-05 | M-mode GVA correct | VS-mode guest-page-fault traps to M-mode | mstatus.GVA=1 |
| MSTAT-06 | TSR only affects HS-mode | mstatus.TSR=1, VS-mode SRET | Does not trigger illegal-instruction (TSR does not affect VS-mode) |
| MSTAT-07 | TVM=1 prevents HS-mode access to hgatp | mstatus.TVM=1, HS-mode csrr hgatp | illegal-instruction exception |
| MSTAT-08 | TVM=1 prevents HS-mode HFENCE.GVMA | mstatus.TVM=1, HS-mode HFENCE.GVMA | illegal-instruction exception |
| MSTAT-09 | TVM=1 does not affect vsatp access | mstatus.TVM=1, HS-mode csrr vsatp | Normal access |
| MSTAT-10 | TVM=1 does not affect HFENCE.VVMA | mstatus.TVM=1, HS-mode HFENCE.VVMA | Normal execution |
| MSTAT-11 | MPRV=1 MPV=1 MPP=1 triggers two-stage translation | Set MPRV=1, MPV=1, MPP=1, M-mode load | VS-level two-stage translation takes effect |
| MSTAT-12 | MPRV=1 MPV=0 does not trigger two-stage translation | Set MPRV=1, MPV=0, MPP=1, M-mode load | Only HS-level translation |
| MSTAT-13 | MPRV does not affect HLV/HSV | Set MPRV=1 (any MPV), HS-mode HLV | HLV always executes as V=1 + SPVP |
| MSTAT-14 | TW=1 affects VS-mode | mstatus.TW=1, VS-mode WFI | illegal-instruction exception (TW affects all non-M-mode) |

---

## Group 17. mideleg / mip / mie Enhancements

**Specification References**:
- `norm:mideleg_acc_h`: mideleg bits 10/6/2 are read-only 1, bit 12 is read-only 1 (when GEILEN>0)
- `norm:mideleg_hroz`: For bits that are zero in mideleg, corresponding hideleg/hip/hie are read-only zero
- `norm:mip_mie_vs`: mip/mie add VS interrupt bits
- `norm:mip_mie_alias`: SGEIP/VSEIP/VSTIP/VSSIP in mip are aliases of corresponding bits in hip

**Test Responsibilities**: Verify Hypervisor enhancement behavior of M-level interrupt registers.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| MIDLG-01 | mideleg bits 10/6/2 are read-only 1 | Attempt to clear mideleg bits 10/6/2 by writing | These bits remain 1 |
| MIDLG-02 | mideleg bit 12 is read-only 1 (GEILEN>0) | If GEILEN>0, attempt to clear mideleg bit 12 | Bit 12 remains 1 |
| MIDLG-03 | hideleg read-only zero for bits where mideleg=0 | A bit in mideleg=0, attempt to write that bit in hideleg | That bit in hideleg is read-only zero |
| MIDLG-04 | mip VSSIP is alias of hip.VSSIP | Write hvip.VSSIP=1, read mip.VSSIP | mip.VSSIP=1 |
| MIDLG-05 | mip VSEIP is alias of hip.VSEIP | Write hvip.VSEIP=1, read mip.VSEIP | mip.VSEIP=1 |
| MIDLG-06 | mie VSSIE is alias of hie.VSSIE | Write hie.VSSIE=1, read mie.VSSIE | mie.VSSIE=1 |
| MIDLG-07 | New VS interrupt bits visible in mip/mie | Read VS interrupt bit positions in mip/mie | Corresponding bits can be read |

---

## Group 18. mtval2 / mtinst Registers (M-mode Trap)

**Specification References**:
- `norm:mtval2_sz_acc_op`: MXLEN-bit read/write register
- `norm:mtval2_trapval`: On guest-page-fault trap to M-mode, mtval2 is written with GPA >> 2 or zero
- `norm:mtval2_trapval_vstrans`: On guest-page-fault caused by implicit VS-stage access, mtval2 is written with GPA of the implicit access
- `norm:mtinst_sz_acc_op` / `norm:mtinst_val`: mtinst format and WARL

**Test Responsibilities**: Verify write behavior of mtval2/mtinst on M-mode trap.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| MTVAL-01 | Basic read/write of mtval2 | M-mode writes mtval2 and reads back | WARL behavior correct |
| MTVAL-02 | mtval2 on guest-page-fault trap to M-mode | VS-mode triggers guest-page-fault, traps to M-mode | mtval2 = GPA >> 2 or 0 |
| MTVAL-03 | mtval2=0 on non-guest-page-fault | VS-mode ecall traps to M-mode | mtval2=0 |
| MTVAL-04 | Basic read/write of mtinst | M-mode writes mtinst and reads back | WARL behavior correct |
| MTVAL-05 | mtinst value on M-mode guest-page-fault trap | VS-mode triggers guest-page-fault, traps to M-mode | mtinst = 0 or transformed instruction/pseudoinstruction |

---

## Group 19. Exception Priority

**Specification References**:
- `norm:H_exception_priority`: Synchronous exception priority table (HSyncExcPrio)
- `norm:H_cause_ecall`: HS-mode and VS-mode ECALL use different cause values

**Test Responsibilities**: Verify correct positioning of exception types introduced by H extension in priority ordering.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| PRIO-01 | virtual-instruction priority lower than illegal-instruction | Scenario where both can be triggered | Final cause determined by priority rules |
| PRIO-02 | Priority between guest-page-fault and page-fault | Both page-fault and guest-page-fault can be triggered during two-stage translation | First encountered fault reported first |
| PRIO-03 | VS-mode ECALL cause=10 | VS-mode executes ECALL | scause/mcause=10 (not 9) |
| PRIO-04 | HS-mode ECALL cause=9 | HS-mode executes ECALL | scause/mcause=9 |
| PRIO-05 | VU-mode ECALL cause=8 | VU-mode executes ECALL | scause/mcause=8 |

---
