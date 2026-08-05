**[中文](../testplan/Hypervisor_Exceptions_test_plan.md) | English**

# Hypervisor Exceptions and Traps Test Plan (Hypervisor_Exceptions Subset)

> This document is split from the original Hypervisor comprehensive test plan (Hypervisor_test_plan_en.md, now removed). Test case IDs remain unchanged; Group numbers are re-sequenced within this subset.
> Sibling subsets: [Hypervisor_CSR_test_plan_en.md](Hypervisor_CSR_test_plan_en.md) | [Hypervisor_Interrupts_test_plan_en.md](Hypervisor_Interrupts_test_plan_en.md)

## Overview

This test plan covers the exception and trap related functionality of the RISC-V Hypervisor (H) extension, including full scenarios of the virtual-instruction exception, trap entry/return behavior, htinst/mtinst transformed instructions, mstatus Hypervisor enhancements (MPV/GVA/TVM/MPRV), mtval2/mtinst registers, exception priority, and the hedeleg exception delegation chain. CSR register field behavior and interrupt delivery mechanisms are covered by the sibling subsets respectively.

This test plan is written based on specification points (norm tags) in `SPEC/hypervisor.adoc`.

### SPEC Chapters Covered by This Document
- Hypervisor and Virtual Supervisor CSRs (hedeleg exception delegation behavior)
- Machine-Level CSR Enhancements (mstatus MPV/GVA/TVM/MPRV, mtval2, mtinst)
- Hypervisor Instructions (HLV/HLVX/HSV exception scenarios, HFENCE.VVMA/HFENCE.GVMA exception scenarios)
- Traps (virtual-instruction exception full scenarios, trap entry/return, exception priority, htinst/mtinst transformed instructions)

### Covered by Other Test Plans
- Hypervisor CSR (CSR substitution mechanism, hstatus, henvcfg, htimedelta, VS CSRs, hedeleg/hideleg bit-field attributes) → `Hypervisor_CSR_test_plan_en.md`
- Hypervisor interrupts (hvip/hip/hie, hgeip/hgeie, mideleg/mip/mie enhancements, hideleg interrupt delegation) → `Hypervisor_Interrupts_test_plan_en.md`
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

This section lists all specification points (norm IDs) referenced in Groups 1-8 of this document, deduplicated and sorted alphabetically.

| Norm ID | English Description |
|---------|---------------------|
| `norm:H_cause_ecall` | HS-mode and VS-mode ECALLs use different cause values so they can be delegated separately. |
| `norm:H_cause_virtual_instruction` | When V=1, a virtual-instruction exception (code 22) is normally raised instead of an illegal-instruction exception if the attempted instruction is HS-qualified but is prevented from executing when V=1. An instruction is HS-qualified if it would be valid to execute in HS-mode, assuming TSR and TVM of `mstatus` are both zero. |
| `norm:H_cause_virtual_instruction_high` | When V=1 and XLEN=32, an invalid attempt to access a high-half CSR raises a virtual-instruction exception instead of an illegal-instruction exception if the same CSR instruction for the corresponding low-half CSR is HS-qualified. |
| `norm:H_exception_priority` | If an instruction may raise multiple synchronous exceptions, the decreasing priority order indicates which exception is taken and reported in `mcause` or `scause`. |
| `norm:H_illegalinst_xstatus_fs_vs` | Fields FS and VS in registers `sstatus` and `vsstatus` deviate from the usual HS-qualified rule. If an instruction is prevented from executing because FS or VS is zero in either `sstatus` or `vsstatus`, the exception raised is always an illegal-instruction exception, never a virtual-instruction exception. |
| `norm:H_trap_deleg` | When a trap occurs in HS-mode or U-mode, it goes to M-mode, unless delegated by `medeleg` or `mideleg`, in which case it goes to HS-mode. When a trap occurs in VS-mode or VU-mode, it goes to M-mode, unless delegated by `medeleg`/`mideleg` to HS-mode, unless further delegated by `hedeleg`/`hideleg` to VS-mode. |
| `norm:H_trap_hs_csrwrites` | When a trap is taken into HS-mode, V is set to 0, and `hstatus`.SPV and `sstatus`.SPP are set accordingly. If V was 1 before the trap, SPVP is set the same as `sstatus`.SPP; otherwise, SPVP is left unchanged. A trap into HS-mode also writes GVA in `hstatus`, SPIE and SIE in `sstatus`, and CSRs `sepc`, `scause`, `stval`, `htval`, and `htinst`. |
| `norm:H_trap_m_csrwrites` | When a trap is taken into M-mode, V gets set to 0, and fields MPV and MPP in `mstatus` are set accordingly. A trap into M-mode also writes fields GVA, MPIE, and MIE in `mstatus` and writes CSRs `mepc`, `mcause`, `mtval`, `mtval2`, and `mtinst`. |
| `norm:H_trap_vs_csrwrites` | When a trap is taken into VS-mode, `vsstatus`.SPP is set accordingly. Register `hstatus` and the HS-level `sstatus` are not modified, and V remains 1. A trap into VS-mode also writes SPIE and SIE in `vsstatus` and writes CSRs `vsepc`, `vscause`, and `vstval`. |
| `norm:H_trap_xtinst` | On any trap into M-mode or HS-mode, one of these values is written to `mtinst` or `htinst`: zero; a transformation of the trapping instruction; a custom value (only if the trapping instruction is non-standard); or a special pseudoinstruction. |
| `norm:H_trap_xtinst_exception` | On a synchronous exception, if a nonzero value is written to the trap instruction register, it must be one of: a standard transformed instruction (bit 0 = 1, replacing bit 1 with 1 yields a valid standard encoding); a custom value (bit 0 = 1, replacing bit 1 with 1 yields a designated custom encoding); or a special pseudoinstruction (bits 1:0 = 00). All other values (e.g. bits 1:0 = 10) are illegal. |
| `norm:H_trap_xtinst_guestpage` | For guest-page faults, the trap instruction register is written with a special pseudoinstruction value if: (a) the fault is caused by an implicit memory access for VS-stage address translation, and (b) a nonzero value is written to `mtval2` or `htval`. If both conditions are met, zero is not allowed. |
| `norm:H_trap_xtinst_guestpage_rw` | A write pseudoinstruction (0x00002020 or 0x00003020) is used for the case that the machine is attempting automatically to update bits A and/or D in VS-level page tables. All other implicit memory accesses for VS-stage address translation will be reads. |
| `norm:H_trap_xtinst_interrupt` | On an interrupt, the value written to the trap instruction register is always zero. |
| `norm:H_trap_xtinst_val` | The values that may be automatically written to the trap instruction register for each standard exception cause are enumerated in the specification table. |
| `norm:H_virtinst_vs_sfence_sinval_satp_vtvm1` | In VS-mode, attempts to execute an SFENCE.VMA or SINVAL.VMA instruction or to access `satp`, when `hstatus`.VTVM=1. |
| `norm:H_virtinst_vs_sret_vtsr1` | In VS-mode, attempts to execute SRET when `hstatus`.VTSR=1. |
| `norm:H_virtinst_vu_nonhigh_supervisor_allowedhs_tvm0` | In VU-mode, attempts to access an implemented non-high-half supervisor CSR when the same access would be allowed in HS-mode, assuming `mstatus`.TVM=0. |
| `norm:H_virtinst_vu_sret_sfence` | In VU-mode, attempts to execute a supervisor instruction (SRET or SFENCE). |
| `norm:H_virtinst_vu_vs_hinst` | In VS-mode or VU-mode, attempts to execute a hypervisor instruction (HLV, HLVX, HSV, or HFENCE). |
| `norm:H_virtinst_vu_vs_nonhigh_allowedhs_tvm0` | In VS-mode or VU-mode, attempts to access an implemented non-high-half hypervisor CSR or VS CSR when the same access would be allowed in HS-mode, assuming `mstatus`.TVM=0. |
| `norm:H_virtinst_vu_wfi_tw0` | In VU-mode, attempts to execute WFI when `mstatus`.TW=0. |
| `norm:H_virtinst_wfi_vtw1_tw0` | In VS-mode, attempts to execute WFI when `hstatus`.VTW=1 and `mstatus`.TW=0, unless the instruction completes within an implementation-specific, bounded time. |
| `norm:H_virtinst_xtval` | On a virtual-instruction trap, `mtval` or `stval` is written the same as for an illegal-instruction trap. |
| `norm:hedeleg_acc` | Each bit of `hedeleg` shall be either writable or read-only zero. Many bits of `hedeleg` are required specifically to be writable or zero, as enumerated in the table. Bit 0, corresponding to instruction address-misaligned exceptions, must be writable if IALIGN=32. |
| `norm:hedeleg_op` | A synchronous trap that has been delegated to HS-mode (using `medeleg`) is further delegated to VS-mode if V=1 before the trap and the corresponding `hedeleg` bit is set. |
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
| `norm:mtval2_val` | `mtval2` is a WARL register that must be able to hold zero and may be capable of holding only an arbitrary subset of other 2-bit-shifted guest physical addresses, if any. Echoing an arbitrary written value is not required, but the readback must be stable. |
| `norm:sret_dt` | If the Ssdbltrp extension is implemented, when SRET is executed in HS-mode, if the new privilege mode is VU, the SRET instruction sets `vsstatus`.SDT to 0. When executed in VS-mode, `vsstatus`.SDT is set to 0. |
| `norm:sret_h` | The SRET instruction is used to return from a trap taken into HS-mode or VS-mode. Its behavior depends on the current virtualization mode. SRET returns from a trap taken into HS-mode or VS-mode; its behavior depends on the current virtualization mode V: with V=0 it follows the `norm:sret_v0` path (based on `hstatus`.SPV/`sstatus`.SPP, using `sepc`), with V=1 it follows the `norm:sret_v1` path (based on `vsstatus`.SPP, using `vsepc`), and neither path may modify the CSR state owned by the other. |
| `norm:sret_v0` | When executed in M-mode or HS-mode (V=0), SRET first determines the new privilege mode according to `hstatus`.SPV and `sstatus`.SPP. SRET then sets `hstatus`.SPV=0, and in `sstatus` sets SPP=0, SIE=SPIE, and SPIE=1. Lastly, SRET sets the privilege mode and sets pc=sepc. |
| `norm:sret_v1` | When executed in VS-mode (V=1), SRET sets the privilege mode accordingly, in `vsstatus` sets SPP=0, SIE=SPIE, and SPIE=1, and lastly sets pc=vsepc. |

---

## Group 1. Virtual-Instruction Exception

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
| VINST-07 | VS-mode accesses hstatus | VS-mode csrr hstatus | virtual-instruction exception (cause=22) |
| VINST-08 | VS-mode accesses hedeleg | VS-mode csrr hedeleg | virtual-instruction exception (cause=22) |
| VINST-09 | VS-mode accesses hgatp | VS-mode csrr hgatp | virtual-instruction exception (cause=22) |
| VINST-10 | VS-mode accesses vsstatus (direct address) | VS-mode directly accesses vsstatus by its CSR address (0x200) | virtual-instruction exception (cause=22) |
| VINST-11 | VU-mode executes WFI (TW=0) | mstatus.TW=0, VU-mode executes WFI | virtual-instruction exception (cause=22) |
| VINST-12 | VU-mode executes SRET | VU-mode executes SRET | virtual-instruction exception (cause=22) |
| VINST-13 | VU-mode executes SFENCE.VMA | VU-mode executes SFENCE.VMA | virtual-instruction exception (cause=22) |
| VINST-14 | VU-mode accesses sstatus | VU-mode csrr sstatus | virtual-instruction exception (cause=22) |
| VINST-15 | VU-mode accesses scause | VU-mode csrr scause | virtual-instruction exception (cause=22) |
| VINST-16 | VS-mode WFI + VTW=1 + TW=0 | hstatus.VTW=1, mstatus.TW=0, VS-mode WFI | virtual-instruction exception (cause=22) |
| VINST-17 | VS-mode SRET + VTSR=1 | hstatus.VTSR=1, VS-mode SRET | virtual-instruction exception (cause=22) |
| VINST-18 | VS-mode SFENCE.VMA + VTVM=1 | hstatus.VTVM=1, VS-mode SFENCE.VMA | virtual-instruction exception (cause=22) |
| VINST-19 | VS-mode accesses satp + VTVM=1 | hstatus.VTVM=1, VS-mode csrr satp | virtual-instruction exception (cause=22) |
| VINST-20 | VS-mode SINVAL.VMA + VTVM=1 | hstatus.VTVM=1, VS-mode SINVAL.VMA | virtual-instruction exception (cause=22) |
| VINST-21 | FS=0 yields illegal not virtual | When V=1, sstatus.FS=0 or vsstatus.FS=0, VS-mode executes FP | illegal-instruction exception (cause=2), not cause=22 |
| VINST-22 | VS=0 yields illegal not virtual | When V=1, sstatus.VS=0 or vsstatus.VS=0, VS-mode executes Vector | illegal-instruction exception (cause=2), not cause=22 |
| VINST-23 | stval correct on virtual-instruction trap | VS-mode triggers virtual-instruction exception | stval same as illegal-instruction trap encoding |
| VINST-24 | mstatus.TW=1 overrides VTW (illegal not virtual) | mstatus.TW=1, VS-mode WFI | illegal-instruction exception (cause=2) |
| VINST-25 | VS-mode accesses hideleg | VS-mode csrr hideleg | virtual-instruction exception (cause=22) |
| VINST-26 | VS-mode accesses hcounteren | VS-mode csrr hcounteren | virtual-instruction exception (cause=22) |
| VINST-27 | VS-mode accesses htimedelta | VS-mode csrr htimedelta | virtual-instruction exception (cause=22) |
| VINST-28 | VS-mode accesses hip | VS-mode csrr hip | virtual-instruction exception (cause=22) |
| VINST-29 | VS-mode accesses hie | VS-mode csrr hie | virtual-instruction exception (cause=22) |
| VINST-30 | VS-mode accesses hvip | VS-mode csrr hvip | virtual-instruction exception (cause=22) |
| VINST-31 | VS-mode accesses henvcfg | VS-mode csrr henvcfg | virtual-instruction exception (cause=22) |
| VINST-32 | VS-mode writes hstatus | VS-mode csrw hstatus | virtual-instruction exception (cause=22) |
| VINST-33 | VU-mode accesses sie | VU-mode csrr sie | virtual-instruction exception (cause=22) |
| VINST-34 | VU-mode accesses sip | VU-mode csrr sip | virtual-instruction exception (cause=22) |
| VINST-35 | VU-mode accesses stvec | VU-mode csrr stvec | virtual-instruction exception (cause=22) |
| VINST-36 | VU-mode accesses sepc | VU-mode csrr sepc | virtual-instruction exception (cause=22) |
| VINST-37 | VS-mode writes satp + VTVM=1 | hstatus.VTVM=1, VS-mode csrw satp | virtual-instruction exception (cause=22) |
| VINST-38 | VS-mode executes HLV.B | VS-mode executes HLV.B | virtual-instruction exception (cause=22) |
| VINST-39 | VS-mode executes HLV.H | VS-mode executes HLV.H | virtual-instruction exception (cause=22) |
| VINST-40 | VS-mode executes HLV.D | VS-mode executes HLV.D | virtual-instruction exception (cause=22) |
| VINST-41 | VS-mode executes HSV.B | VS-mode executes HSV.B | virtual-instruction exception (cause=22) |
| VINST-42 | VS-mode executes HSV.H | VS-mode executes HSV.H | virtual-instruction exception (cause=22) |
| VINST-43 | VS-mode executes HSV.D | VS-mode executes HSV.D | virtual-instruction exception (cause=22) |
| VINST-44 | mstatus.TSR=1 does not affect VS-mode SRET | mstatus.TSR=1, hstatus.VTSR=1, VS-mode SRET. TSR only affects HS-mode (norm:mstatus_modes); VS-mode SRET is controlled only by VTSR | virtual-instruction exception (cause=22) |

---

## Group 2. Trap Entry Behavior

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

## Group 3. Trap Return Behavior

**Specification References**:
- `norm:sret_h`: umbrella rule — SRET returns from a trap taken into HS/VS-mode; its behavior depends on the current virtualization mode V (V=0 follows the `sret_v0` path, V=1 follows the `sret_v1` path)
- `norm:mret_h`: MRET determines new privilege level based on MPP/MPV, then MPV=0, MPP=0, MIE=MPIE, MPIE=1
- `norm:sret_v0`: When V=0, SRET determines new mode based on SPV/SPP, SPV=0, SPP=0, SIE=SPIE, SPIE=1, pc=sepc
- `norm:sret_v1`: When V=1, SRET determines mode based on vsstatus.SPP, SPP=0, SIE=SPIE, SPIE=1, pc=vsepc
- `norm:sret_dt`: With Ssdbltrp, SRET clears vsstatus.SDT when in HS-mode and new mode is VU; clears vsstatus.SDT when in VS-mode

**Test Responsibilities**: Verify mode switching and CSR restoration behavior of MRET/SRET under H extension.

**Strict Verification Principles** (norm:sret_h umbrella semantics): SRET-related cases MUST execute a real SRET instruction and verify the landing context plus the before/after CSR state; CSR read/write checks are NOT a substitute for behavioral verification. The landing privilege level is discriminated by two falsifiable probes:
1. **Sentinel probe**: sscratch/vsscratch are pre-loaded with distinct sentinels; after landing, `csrr sscratch` readback distinguishes a VS landing (reads vsscratch) from an HS landing (reads sscratch); a VU/U landing is distinguished by the exception that access raises (virtual-instruction cause=22 / illegal-instruction cause=2).
2. **Poisoned-sepc probe**: before the V=1 path, HS `sepc` is set to an invalid address; if the implementation wrongly returns through HS `sepc` (instead of `vsepc`), execution jumps to the invalid address and faults, directly falsifying the behavior.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TRET-01 | MRET returns to VS-mode | Set MPV=1, MPP=1, execute MRET | Enter VS-mode (V=1), MPV=0, MPP=0 |
| TRET-02 | MRET returns to VU-mode | Set MPV=1, MPP=0, execute MRET | Enter VU-mode (V=1), MPV=0, MPP=0 |
| TRET-03 | MRET returns to HS-mode | Set MPV=0, MPP=1, execute MRET | Enter HS-mode (V=0) |
| TRET-04 | MRET returns to M-mode | Set MPP=3, execute MRET | Enter M-mode, V remains 0 |
| TRET-05 | MRET MIE/MPIE restoration | Set MPIE=1, execute MRET | MIE=1, MPIE=1 |
| TRET-06 | SRET(V=0) returns to VS-mode | In HS-mode set hstatus.SPV=1, sstatus.SPP=1, sepc=landing label, execute a real SRET | Actually lands at the VS-mode landing label (proven by vsscratch sentinel readback); afterwards SPV=0, SPP=0 |
| TRET-07 | SRET(V=0) returns to VU-mode | In HS-mode set SPV=1, SPP=0, sepc=landing label, execute a real SRET | Actually lands in VU-mode (proven by the sscratch access at landing raising virtual-instruction cause=22); SPV=0, SPP=0 |
| TRET-08 | SRET(V=0) returns to HS-mode | In HS-mode set SPV=0, SPP=1, sepc=landing label, execute a real SRET | Actually lands in HS-mode (proven by HS sscratch sentinel readback) |
| TRET-09 | SRET(V=0) returns to U-mode | In HS-mode set SPV=0, SPP=0, sepc=landing label, execute a real SRET | Actually lands in U-mode (proven by the sscratch access at landing raising illegal-instruction cause=2) |
| TRET-10 | SRET(V=0) SIE/SPIE restoration | In HS-mode execute a real SRET with both patterns (a) SPIE=1,SIE=0 and (b) SPIE=0,SIE=1 | After each SRET, sstatus.SIE=old SPIE and SPIE=1 (both patterns falsifiable) |
| TRET-11 | SRET(V=1) returns to VS-mode | Pre-set vsstatus.SPP=1 and poison HS sepc with an invalid address, execute a real SRET in VS-mode | Actually lands at the landing label pointed by vsepc (using HS sepc by mistake would jump to the invalid address and fault); vsstatus.SPP=0; HS sepc unchanged |
| TRET-12 | SRET(V=1) returns to VU-mode | Pre-set vsstatus.SPP=0 and poison HS sepc, execute a real SRET in VS-mode | Actually lands in VU-mode (proven by the sscratch access at landing raising cause=22); vsstatus.SPP=0; HS sepc unchanged |
| TRET-13 | SRET(V=1) SIE/SPIE restoration | In VS-mode execute a real SRET with both patterns (a) SPIE=1,SIE=0 and (b) SPIE=0,SIE=1 | After each SRET, vsstatus.SIE=old SPIE and SPIE=1 |
| TRET-14 | SRET restores PC from sepc and does not modify sepc | In HS-mode set sepc=a non-adjacent landing label and execute a real SRET | pc=sepc (actually jumps to that label); afterwards sepc still equals that label value (SRET does not modify sepc) |
| TRET-15 | MRET restores PC from mepc | Set mepc=target address, execute MRET | PC=mepc |
| TRET-16 | SRET(V=1) does not modify V=0 state | Pre-set hstatus.SPV=1, sstatus.SPP=1 and poison HS sepc, execute a real SRET in VS-mode (vsstatus.SPP=1) | Afterwards hstatus.SPV, sstatus.SPP and HS sepc all remain unchanged (sret_v1 only operates on vsstatus/vsepc) |
| TRET-17 | SRET(V=0) does not modify VS state | Pre-set vsstatus.SPP=1 and poison vsepc, execute a real SRET in HS-mode (SPV=1, SPP=1) returning to VS-mode | Afterwards vsstatus.SPP and vsepc remain unchanged (sret_v0 only operates on hstatus/sstatus/sepc) |
| TRET-18 | Full round-trip: VU trap -> HS handler -> SRET resume | Delegate ebreak to HS-mode via medeleg[3], stvec points to the HS handler, execute ebreak in VU-mode | Trap enters HS-mode (trap_get_spv()=1); the HS handler returns via SRET and resumes VU execution (flag write succeeds); afterwards hstatus.SPV=0 (cleared by SRET) |
| TRET-19 | Full round-trip: nested VS trap -> VS handler SRET resume | Delegate ebreak to VS-mode via medeleg[3]+hedeleg[3], vstvec points to a custom VS handler, execute ebreak in VS-mode | Trap enters the VS handler (vscause=3); the handler's SRET resumes the VS context after the ebreak; HS-level sstatus.SPP is unaffected |


---

## Group 4. htinst / mtinst Transformed Instructions

**Specification References**:
- `norm:H_trap_xtinst`: Value types written to mtinst/htinst on trap (zero/transformed instruction/custom/pseudoinstruction); except for the mandated pseudoinstruction scenario, the implementation is always allowed to write zero
- `norm:H_trap_xtinst_interrupt`: Write zero on interrupt
- `norm:H_trap_xtinst_exception`: When a synchronous exception writes a nonzero value, it must satisfy one of the three legal forms (standard transformed instruction/custom/pseudoinstruction)
- `norm:H_trap_xtinst_val`: Value types writable for each exception type (tinst-values table); custom values are limited to non-standard instructions, standard instructions (e.g. ecall/illegal-instruction) may only write zero
- `norm:H_trap_xtinst_guestpage`: Must write pseudoinstruction (zero not allowed) when an implicit VS-stage access causes guest-page-fault and htval/mtval2 is non-zero
- `norm:H_trap_xtinst_guestpage_rw`: Use 0x00003000 for read, 0x00003020 for write (A/D update) (RV64)

**Test Responsibilities**: Verify htinst/mtinst write values in various trap scenarios.

**Strict Verification Principles** (addressing the review gap: the original cases checked trap-written values too leniently, including tautological assertions):
1. The SPEC allows the implementation to write zero in non-mandated scenarios, so zero must be accepted; but when the implementation writes a nonzero value, it must EXACTLY match the SPEC-derived expected value (the golden value computed from the actual trapping instruction at mepc per the transformation rules) — "any nonzero value" is not accepted.
2. For implicit VS-stage access faults with htval/mtval2 non-zero, htinst/mtinst must be exactly the pseudoinstruction value; zero is illegal (norm:H_trap_xtinst_guestpage).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TINST-01 | mtinst/htinst=0 on interrupt trap | Inject VS software interrupt via hvip.VSSIP (hideleg.VSSIP=0; mideleg VS bits are read-only 1, so the trap necessarily goes to HS-mode) | cause is an interrupt and came from V=1, htinst=0 (strict) |
| TINST-02 | htinst=0 on ecall trap | VS-mode ecall (standard instruction, custom not allowed) | htinst=0 (strict) |
| TINST-03 | htinst value on load guest-page-fault | Deterministic VS-mode `ld` triggers guest-page-fault | htinst=0 or exactly the SPEC transformed instruction of that `ld` (golden, computed from the instruction at mepc) |
| TINST-04 | htinst value on store guest-page-fault | Deterministic VS-mode `sd` triggers guest-page-fault | htinst=0 or exactly the SPEC transformed instruction of that `sd` (golden) |
| TINST-05 | Pseudoinstruction for implicit VS-stage read fault | VS-stage leaf page-table page made unreadable at G-stage, triggering an implicit read guest-page-fault | htinst=0x00003000 when htval≠0 (zero NOT allowed); accepted when htval=0 |
| TINST-06 | Pseudoinstruction for implicit write (A/D update) | VS-stage leaf page-table page has D=0 at G-stage, triggering an implicit write fault; SKIP on platforms with Svadu | htinst=0x00003020 when htval≠0 (zero NOT allowed) |
| TINST-07 | Transformed instruction field structure verification | 32-bit load triggers fault; field-by-field check when htinst is nonzero | opcode/funct3/rd preserved, imm zeroed, Addr. Offset correct, bits 1:0 = 11 |
| TINST-08 | Compressed instruction transformed bit 1:0 encoding | 16-bit C.LW triggers fault | htinst bits 1:0 = 01 (compressed instruction); remains SKIP (no compressed-instruction probe in the current framework) |
| TINST-09 | Page-fault does not produce pseudoinstruction | VS-stage leaf PTE R=0 triggers load page-fault (cause=13, not guest-page-fault) | htinst=0 or transformed instruction (golden exact match); pseudoinstruction values not allowed |
| TINST-10 | illegal-instruction allows only zero | VS-mode executes an illegal instruction (standard exception, tinst-values table allows Zero only) | htinst=0 (strict) |

---

## Group 5. mstatus Enhancements (Hypervisor Related)

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

## Group 6. mtval2 / mtinst Registers (M-mode Trap)

**Specification References**:
- `norm:mtval2_sz_acc_op`: MXLEN-bit read/write register
- `norm:mtval2_trapval`: On guest-page-fault trap to M-mode, mtval2 is written with GPA >> 2 or zero; other traps must write zero
- `norm:mtval2_trapval_vstrans`: On guest-page-fault caused by implicit VS-stage access, mtval2 is written with the GPA of the implicit access
- `norm:mtval2_val`: WARL, must be able to hold zero; echoing an arbitrary written value is not required, but readback must be stable
- `norm:mtinst_sz_acc_op` / `norm:mtinst_val`: mtinst format and WARL (need only hold the values the implementation may automatically write on a trap)

**Test Responsibilities**: Verify write behavior of mtval2/mtinst on M-mode trap. In this suite VS/HS traps are not delegated by default and are all taken into M-mode; the framework captures mtval2/mtinst at M-mode trap entry (`trap_get_htval()`/`trap_get_htinst()` are mtval2/mtinst on the M-mode delivery path).

**Strict Verification Principles** (addressing the review gap): trap-written values are asserted exactly against the SPEC-allowed set — on GPF mtval2 must be exactly one of `0` or `GPA>>2` (no other value allowed); WARL read/write does not require echo of the written value but must be stable and zero must be holdable; on an implicit-access fault a nonzero mtval2 must be exactly the implicit-access GPA>>2.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| MTVAL-01 | WARL read/write of mtval2 | M-mode writes 0 and arbitrary patterns, writes repeatedly and reads back | Readback must be 0 after writing 0; readback of any value is stable (writing the same value twice reads back identically); echoing the original value is not required |
| MTVAL-02 | mtval2 on guest-page-fault trap to M-mode | Deterministic VS-mode load triggers guest-page-fault | mtval2 = GPA >> 2 or 0 (strictly one of the two), consistent with the trap record |
| MTVAL-03 | mtval2=0 on non-guest-page-fault | VS-mode ecall traps to M-mode | mtval2=0 (strict), htval=0 |
| MTVAL-04 | WARL read/write of mtinst | M-mode writes 0 and arbitrary patterns, writes repeatedly and reads back | Same WARL semantics as MTVAL-01 |
| MTVAL-05 | mtinst value on M-mode guest-page-fault trap | Deterministic VS-mode load triggers guest-page-fault | mtinst = 0 or exactly the transformed instruction (golden) |
| MTVAL-06 | mtval2 on implicit VS-stage access fault | VS-stage leaf page-table page made unreadable at G-stage, triggering an implicit read fault | mtval2 = 0 or exactly the implicit-access PTE GPA>>2; when nonzero, mtinst must be 0x00003000 |

---

## Group 7. Exception Priority

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

## Group 8. hedeleg Exception Delegation Chain

> This group is split from Group 3 of the original `Hypervisor_test_plan_en.md`, keeping only the exception delegation cases (DELEG-04~07, DELEG-15, DELEG-16); CSR bit-field attribute cases are in `Hypervisor_CSR_test_plan_en.md`, and interrupt delegation with interrupt number translation cases are in `Hypervisor_Interrupts_test_plan_en.md`.

**Specification References**:
- `norm:hedeleg_op`: When V=1, exceptions delegated by medeleg are further delegated to VS-mode if the corresponding hedeleg bit is set
- `norm:hedeleg_acc`: Writable/read-only constraints for each hedeleg bit (guest-page-fault/virtual-instruction bits are read-only zero, hence not delegable)

**Test Responsibilities**: Verify the correctness of the hedeleg exception delegation chain (M→HS→VS) and the constraints on non-delegable exceptions.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| DELEG-04 | hedeleg delegates illegal-instruction to VS | Set medeleg[2]=1, hedeleg[2]=1, VS-mode triggers illegal instruction | Trap enters VS-mode (vscause=2) |
| DELEG-05 | Trap to HS when hedeleg not delegating | Set medeleg[2]=1, hedeleg[2]=0, VS-mode triggers illegal instruction | Trap enters HS-mode (scause=2) |
| DELEG-06 | hedeleg delegates breakpoint to VS | Set medeleg[3]=1, hedeleg[3]=1, VS-mode executes EBREAK | Trap enters VS-mode (vscause=3) |
| DELEG-07 | hedeleg delegates ecall-from-VU to VS | Set medeleg[8]=1, hedeleg[8]=1, VU-mode executes ECALL | Trap enters VS-mode (vscause=8) |
| DELEG-15 | guest-page-fault cannot be delegated to VS | Verify hedeleg bits 20/21/23 are read-only zero | guest-page-fault always traps to HS-mode |
| DELEG-16 | virtual-instruction cannot be delegated to VS | Verify hedeleg bit 22 is read-only zero | virtual-instruction exception always traps to HS-mode |
