**[中文](../testplan/Hypervisor_CFI_test_plan.md) | English**

# Hypervisor × CFI Cross Test Plan

## Overview

This test plan covers the cross-functional points between the RISC-V Hypervisor (H) extension and the CFI (Control-Flow Integrity) extension. The CFI extension contains two sub-extensions:

- **Zicfilp** (Forward-edge control flow protection / Landing Pad): ensures that indirect jump/call targets are legitimate
- **Zicfiss** (Backward-edge control flow protection / Shadow Stack): ensures that function return addresses have not been tampered with

In a Hypervisor environment, CFI enablement, state save/restore, exception delegation, and page table interaction are all affected by the H extension. This test plan focuses on the intersection behavior of the H extension with the Zicfilp and Zicfiss sub-extensions respectively.

This test plan is written based on specification points (norm tags) in `SPEC/hypervisor.adoc` and `SPEC/cfi.adoc`.

### SPEC Sections Covered by This Document

**From cfi.adoc:**
- Landing-Pad-Enabled (LPE) State (xLPE determination table; VS-mode is controlled by henvcfg.LPE)
- Preserving Expected Landing Pad State on Traps (ELP/xPELP save/restore, vsstatus.SPELP)
- Shadow Stack Pointer (ssp) CSR access control (VS/VU-mode access control)
- Shadow-Stack-Enabled (SSE) State (xSSE determination table; VS-mode is controlled by henvcfg.SSE)
- Shadow Stack Memory Protection (SS page type, G-stage interaction, satp Bare mode)

**From hypervisor.adoc:**
- henvcfg register (LPE/SSE fields control VS-mode CFI enablement)
- vsstatus register (SPELP field saves VS-mode previous ELP)
- hedeleg delegation register (software-check exception delegation)
- Trap Entry/Return behavior (ELP save/restore on VS-mode trap)

### Covered by Other Test Plans
- Hypervisor core functionality → `Hypervisor_test_plan.md`
- CFI basic functionality (non-virtualization scenarios) → `cfi_test_plan.md`
- G-stage / VS-stage address translation → `hyp_gstage_translation_test_plan.md` / `hyp_2_stage_translation_test_plan.md`

---

## Covered Specification Points

This section lists all specification points (norm IDs) referenced in the test groups of this document, deduplicated and arranged by source.

| Norm ID | Source | Original Text |
|---------|--------|---------------|
| `norm:henvcfg_lpe_op` | hypervisor.adoc | The Zicfilp extension adds the LPE field in henvcfg. When LPE=1, the Zicfilp extension is enabled in VS-mode. When LPE=0, the hart does not update the ELP state and LPAD operates as a no-op. |
| `norm:henvcfg_sse_op` | hypervisor.adoc | The Zicfiss extension adds the SSE field in henvcfg. If SSE=1, the Zicfiss extension is activated in VS-mode. When SSE=0, 32-bit Zicfiss instructions revert to Zimop behavior, 16-bit to Zcmop, pte.xwr=010b becomes reserved, senvcfg.SSE reads zero, and SSAMOSWAP.W/D raises virtual-instruction exception when menvcfg.SSE=1. |
| `norm:vsstatus_spelp_op` | hypervisor.adoc | The Zicfilp extension adds the SPELP field that holds the previous ELP, and is updated as specified in ZICFILP_FORWARD_TRAPS. SPELP encoded as: 0=NO_LP_EXPECTED, 1=LP_EXPECTED. |
| `norm:vsstatus_spelp_op2` | cfi.adoc | To store the previous ELP state on traps to VS-mode, a spelp bit is defined in the vsstatus (VS-modes version of sstatus). |
| `norm:cfi_mstatus_mpelp_op` | cfi.adoc | To store the previous ELP state on trap delivery to M-mode, an mpelp bit is provided in the mstatus CSR. |
| `norm:cfi_mstatus_spelp_op` | cfi.adoc | To store the previous ELP state on trap delivery to S/HS-mode, an spelp bit is provided in the mstatus CSR. |
| `norm:cfi_sstatus_spelp_op` | cfi.adoc | The spelp bit in mstatus can be accessed through the sstatus CSR. |
| `norm:Zicfilp_pelp_trap` | cfi.adoc | When a trap is taken into privilege mode x, the xpelp is set to ELP and ELP is set to NO_LP_EXPECTED. |
| `norm:Zicfilp_pelp_trap_return` | cfi.adoc | When executing an xret instruction, if the new privilege mode is y, then ELP is set to the value of xpelp if yLPE is 1; otherwise, it is set to NO_LP_EXPECTED; xpelp is set to NO_LP_EXPECTED. |
| `norm:Zicfilp_forward_traps` | cfi.adoc | A trap may need to be delivered upon completion of jalr/c.jalr/c.jr but before the target instruction was decoded. The ELP prior to the trap must be preserved. |
| `norm:lpad_sw_exception` | cfi.adoc | The software-check exception due to the instruction not being an lpad instruction when ELP is LP_EXPECTED leads to a trap. |
| `norm:Zicfilp_exception_priority` | cfi.adoc | The software-check exception caused by Zicfilp has higher priority than an illegal-instruction exception but lower priority than instruction access-fault. |
| `norm:zicfiss_ssp_csr` | cfi.adoc | Attempts to access the ssp CSR may result in either an illegal-instruction exception or a virtual-instruction exception, contingent upon the state of the envcfg sse fields. |
| `norm:zicfiss_m_menvcfg_sse` | cfi.adoc | If the privilege mode is less than M and menvcfg.sse is 0, an illegal-instruction exception is raised. |
| `norm:zicfiss_vs_henvcfg_sse` | cfi.adoc | Otherwise, if in VS-mode and henvcfg.sse is 0, a virtual-instruction exception is raised. |
| `norm:zicfiss_vu_henvcfg_senvcfg_sse` | cfi.adoc | Otherwise, if in VU-mode and either henvcfg.sse or senvcfg.sse is 0, a virtual-instruction exception is raised. |
| `norm:zicfiss_sse_access` | cfi.adoc | Otherwise, the access is allowed. |
| `norm:ss_page_enc` | cfi.adoc | The encoding R=0, W=1, and X=0, is defined to represent an SS page. |
| `norm:ssmp_henvcfg_sse` | cfi.adoc | When V=1 and henvcfg.sse=0, this encoding remains reserved at VS and VU levels. |
| `norm:ssmp_menvcfg_sse` | cfi.adoc | When menvcfg.sse=0, this encoding remains reserved. |
| `norm:satp_mode_bare` | cfi.adoc | If satp.mode (or vsatp.mode when V=1) is set to Bare and the effective privilege mode is less than M, shadow stack instructions raise a store/AMO access-fault exception. |
| `norm:ssmp_ssamoswap` | cfi.adoc | When the effective privilege mode is M, memory access by an ssamoswap.w or ssamoswap.d instruction results in a store/AMO access-fault exception. |
| `norm:ssmp_ss_page_access_fault` | cfi.adoc | Memory mapped as an SS page cannot be written to by instructions other than ssamoswap, sspush, and c.sspush. Attempts will raise a store/AMO access-fault exception. |
| `norm:ssmp_ss_page_illegeal_access` | cfi.adoc | Should a shadow stack instruction access a page that is not designated as a shadow stack page and is not marked as read-only, a store/AMO access-fault exception will be invoked. |
| `norm:ssmp_ss_read_only_page` | cfi.adoc | If the page being accessed by a shadow stack instruction is a read-only page, a store/AMO page-fault exception will be triggered. |
| `norm:ss_fault_exception_code` | cfi.adoc | If a shadow stack instruction raises an access-fault, page-fault, or guest-page-fault exception, the reported exception cause is respectively store/AMO access fault (7), store/AMO page fault (15), or store/AMO guest-page fault (23). |
| `norm:ssp_xlen_aligned` | cfi.adoc | If the virtual address in ssp is not XLEN aligned, then sspush/c.sspush/sspopchk/c.sspopchk cause a store/AMO access-fault exception. |
| `norm:ssmp_ss_idempotent_memory` | cfi.adoc | If the memory referenced by sspush/c.sspush/sspopchk/c.sspopchk/ssamoswap.w/ssamoswap.d is not idempotent, then the instructions cause a store/AMO access-fault exception. |
| `norm:active_g_stage_pte` | cfi.adoc | When G-stage page tables are active, the shadow stack instructions that access memory require the G-stage page table to have read-write permission; else a store/AMO guest-page-fault exception is raised. |
| `norm:hedeleg_op` | hypervisor.adoc | A synchronous trap delegated to HS-mode is further delegated to VS-mode if V=1 before the trap and the corresponding hedeleg bit is set. |
| `norm:hedeleg_acc` | hypervisor.adoc | Each bit of hedeleg shall be either writable or read-only zero. |
| `norm:H_trap_vs_csrwrites` | hypervisor.adoc | When a trap is taken into VS-mode, vsstatus.SPP is set accordingly. hstatus and HS-level sstatus are not modified, V remains 1. |
| `norm:H_trap_hs_csrwrites` | hypervisor.adoc | When a trap is taken into HS-mode, V is set to 0, hstatus.SPV and sstatus.SPP are set accordingly. |
| `norm:H_trap_m_csrwrites` | hypervisor.adoc | When a trap is taken into M-mode, V gets set to 0, MPV and MPP in mstatus are set accordingly. |
| `norm:sret_v1` | hypervisor.adoc | When executed in VS-mode (V=1), SRET sets the privilege mode accordingly, in vsstatus sets SPP=0, SIE=SPIE, and SPIE=1, and sets pc=vsepc. |
| `norm:sret_v0` | hypervisor.adoc | When executed in M-mode or HS-mode (V=0), SRET determines new mode according to hstatus.SPV and sstatus.SPP, sets SPV=0, SPP=0, SIE=SPIE, SPIE=1, pc=sepc. |

---

# Part A: Hypervisor × Zicfilp (Forward-Edge Control Flow / Landing Pad)

---

## Group A1. henvcfg.LPE Field and VS-mode Zicfilp Enable Control

**Spec Reference**:
- `norm:henvcfg_lpe_op`: LPE=1 enables Zicfilp in VS-mode; LPE=0 keeps ELP at NO_LP_EXPECTED, LPAD operates as no-op
- cfi.adoc xLPE determination table: VS-mode xLPE = henvcfg.LPE; VU-mode xLPE = senvcfg.LPE

**Test Scope**: Verify the enable control of the henvcfg.LPE field over the VS-mode Zicfilp extension, including the behavior that ELP is not updated and LPAD degrades to no-op when LPE=0.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HCFI-LP-01 | henvcfg.LPE basic read/write | HS-mode writes henvcfg.LPE=1 and reads back | LPE=1 (if Zicfilp is implemented); read-only zero if not implemented |
| HCFI-LP-02 | VS-mode ELP not updated when henvcfg.LPE=0 | Set henvcfg.LPE=0, menvcfg.LPE=1, VS-mode executes indirect jump (JALR) to LPAD target | ELP remains NO_LP_EXPECTED, LPAD executes as no-op, no exception |
| HCFI-LP-03 | VS-mode ELP updated when henvcfg.LPE=1 | Set henvcfg.LPE=1, menvcfg.LPE=1, mseccfg.MLPE=1, VS-mode executes indirect jump (JALR) to LPAD target | ELP cleared from LP_EXPECTED to NO_LP_EXPECTED by LPAD, normal execution with no exception |
| HCFI-LP-04 | VS-mode LPAD as no-op when henvcfg.LPE=0 | Set henvcfg.LPE=0, VS-mode executes LPAD instruction | LPAD executes as no-op, no exception triggered, ELP unchanged |
| HCFI-LP-05 | VS-mode illegal indirect jump triggers LP Fault when henvcfg.LPE=1 | Set henvcfg.LPE=1, VS-mode indirect jump to non-LPAD target | software-check exception (cause=18, stval/vstval=2) |
| HCFI-LP-06 | VS-mode illegal indirect jump does not trigger LP Fault when henvcfg.LPE=0 | Set henvcfg.LPE=0, VS-mode indirect jump to non-LPAD target | No software-check exception, ELP remains NO_LP_EXPECTED |
| HCFI-LP-07 | VU-mode xLPE controlled by senvcfg.LPE | V=1, henvcfg.LPE=1, senvcfg.LPE=0, VU-mode indirect jump to non-LPAD target | No software-check exception (VU-mode xLPE=0) |
| HCFI-LP-08 | VU-mode xLPE=1 triggers LP Fault | V=1, henvcfg.LPE=1, senvcfg.LPE=1, VU-mode indirect jump to non-LPAD target | software-check exception (cause=18, stval/vstval=2) |
| HCFI-LP-09 | henvcfg.LPE=1 but menvcfg.LPE=0 | Set menvcfg.LPE=0, henvcfg.LPE=1, VS-mode executes indirect jump | ELP not updated (menvcfg.LPE=0 makes HS-level xLPE=0, but VS-level xLPE=henvcfg.LPE should take effect independently); verify whether VS-mode Zicfilp works normally |
| HCFI-LP-10 | henvcfg.LPE read-only zero when Zicfilp not implemented | If Zicfilp is not implemented, read henvcfg.LPE | Read-only zero |

---

## Group A2. vsstatus.SPELP Field and VS-mode Trap Save/Restore

**Spec Reference**:
- `norm:vsstatus_spelp_op`: vsstatus.SPELP saves VS-mode previous ELP, encoding 0=NO_LP_EXPECTED, 1=LP_EXPECTED
- `norm:vsstatus_spelp_op2`: spelp bit defined in vsstatus for saving previous ELP on traps to VS-mode
- `norm:Zicfilp_pelp_trap`: On trap into mode x, xpelp←ELP, ELP←NO_LP_EXPECTED
- `norm:Zicfilp_pelp_trap_return`: On xRET, if new mode y has yLPE=1 then ELP←xpelp, otherwise ELP←NO_LP_EXPECTED

**Test Scope**: Verify the save/restore behavior of vsstatus.SPELP on VS-mode trap entry/return, and the ELP state transfer across privilege levels (VS↔HS, VS↔M).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HCFI-LP-11 | vsstatus.SPELP basic read/write | HS-mode writes vsstatus.SPELP=1 and reads back | SPELP=1 |
| HCFI-LP-12 | SPELP saves ELP on VS-mode trap to VS-mode | Set henvcfg.LPE=1, trigger delegated exception in VS-mode with ELP=LP_EXPECTED | vsstatus.SPELP=1 (saves LP_EXPECTED), ELP set to NO_LP_EXPECTED |
| HCFI-LP-13 | VS-mode SRET restores ELP (VSLPE=1) | Following HCFI-LP-12, execute SRET in VS-mode trap handler | ELP restored to vsstatus.SPELP value (LP_EXPECTED), SPELP set to NO_LP_EXPECTED |
| HCFI-LP-14 | VS-mode SRET does not restore ELP (VSLPE=0) | Set henvcfg.LPE=0, SPELP saved ELP on VS-mode trap, execute SRET | ELP set to NO_LP_EXPECTED (VSLPE=0), SPELP set to NO_LP_EXPECTED |
| HCFI-LP-15 | mstatus.SPELP saves ELP on VS-mode trap to HS-mode | Set henvcfg.LPE=1, trigger non-delegated exception in VS-mode with ELP=LP_EXPECTED | mstatus.SPELP=1 (saves LP_EXPECTED), ELP set to NO_LP_EXPECTED |
| HCFI-LP-16 | HS-mode SRET returns to VS-mode restoring ELP (VSLPE=1) | Following HCFI-LP-15, HS-mode executes SRET returning to VS-mode | ELP restored to mstatus.SPELP (LP_EXPECTED), mstatus.SPELP set to NO_LP_EXPECTED |
| HCFI-LP-17 | HS-mode SRET returns to VS-mode without restoring ELP (VSLPE=0) | Set henvcfg.LPE=0, HS-mode SRET returns to VS-mode | ELP set to NO_LP_EXPECTED (VSLPE=0) |
| HCFI-LP-18 | mstatus.MPELP saves ELP on VS-mode trap to M-mode | Set henvcfg.LPE=1, trigger exception in VS-mode with ELP=LP_EXPECTED entering M-mode | mstatus.MPELP=1 (saves LP_EXPECTED), ELP set to NO_LP_EXPECTED |
| HCFI-LP-19 | M-mode MRET returns to VS-mode restoring ELP (VSLPE=1) | Following HCFI-LP-18, M-mode executes MRET returning to VS-mode | ELP restored to mstatus.MPELP (LP_EXPECTED), mstatus.MPELP set to NO_LP_EXPECTED |
| HCFI-LP-20 | M-mode MRET returns to VS-mode without restoring ELP (VSLPE=0) | Set henvcfg.LPE=0, M-mode MRET returns to VS-mode | ELP set to NO_LP_EXPECTED (VSLPE=0) |
| HCFI-LP-21 | Trap saves SPELP=0 when ELP=NO_LP_EXPECTED | Set henvcfg.LPE=1, trigger trap to VS-mode in VS-mode with ELP=NO_LP_EXPECTED | vsstatus.SPELP=0 (saves NO_LP_EXPECTED) |
| HCFI-LP-22 | ELP restored on VS-mode SRET returning to VU-mode | Set henvcfg.LPE=1, senvcfg.LPE=1, vsstatus.SPELP=1 in VS-mode, SRET returns to VU-mode (SPP=0) | ELP restored to vsstatus.SPELP (when VULPE=1), SPELP set to NO_LP_EXPECTED |
| HCFI-LP-23 | VS-mode SRET returning to VU-mode with VULPE=0 | Set senvcfg.LPE=0, VS-mode SRET returns to VU-mode | ELP set to NO_LP_EXPECTED (VULPE=0) |
| HCFI-LP-24 | vsstatus.SPELP does not affect behavior when V=0 | Set vsstatus.SPELP=1 when V=0, HS-mode executes indirect jump | ELP not affected by vsstatus.SPELP |

---

## Group A3. VS-mode Landing Pad Functionality and software-check exception

**Spec Reference**:
- `norm:lpad_sw_exception`: ELP=LP_EXPECTED and target instruction is not LPAD → software-check exception
- `norm:Zicfilp_exception_priority`: software-check has higher priority than illegal-instruction, lower than instruction access-fault
- `norm:Zicfilp_forward_traps`: Trap may be delivered after JALR completes but before target is decoded, ELP must be preserved

**Test Scope**: Verify the landing pad check functionality of Zicfilp in VS-mode, the trigger conditions for software-check exception, and exception priority.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HCFI-LP-25 | VS-mode legal indirect call to LPAD target | henvcfg.LPE=1, VS-mode JALR to function beginning with LPAD | Normal execution, ELP cleared from LP_EXPECTED to NO_LP_EXPECTED |
| HCFI-LP-26 | VS-mode illegal indirect call triggers LP Fault | henvcfg.LPE=1, VS-mode JALR to non-LPAD target | software-check exception (cause=18, vstval=2) |
| HCFI-LP-27 | VS-mode legal indirect jump to LPAD target | henvcfg.LPE=1, VS-mode JALR (non-call) to LPAD target | Normal execution |
| HCFI-LP-28 | VS-mode illegal indirect jump triggers LP Fault | henvcfg.LPE=1, VS-mode JALR (non-call) to non-LPAD target | software-check exception (cause=18, vstval=2) |
| HCFI-LP-29 | VS-mode LP Fault vs instruction access-fault priority | VS-mode indirect jump to inaccessible address (triggers both access-fault and LP check) | instruction access-fault takes priority over software-check exception |
| HCFI-LP-30 | VS-mode LP Fault vs illegal-instruction priority | VS-mode indirect jump target is illegal instruction (triggers both illegal and LP check) | software-check exception takes priority over illegal-instruction exception |
| HCFI-LP-31 | VS-mode interrupt during indirect jump saves ELP | henvcfg.LPE=1, interrupt received after VS-mode JALR execution but before LPAD decode | ELP=LP_EXPECTED saved to vsstatus.SPELP (trap to VS) or mstatus.SPELP (trap to HS) |
| HCFI-LP-32 | VS-mode C.JALR illegal jump triggers LP Fault | henvcfg.LPE=1, VS-mode C.JALR to non-LPAD target | software-check exception (cause=18, vstval=2) |
| HCFI-LP-33 | VS-mode C.JR illegal jump triggers LP Fault | henvcfg.LPE=1, VS-mode C.JR to non-LPAD target | software-check exception (cause=18, vstval=2) |

---

## Group A4. VS-mode software-check exception Delegation

**Spec Reference**:
- `norm:hedeleg_op`: Synchronous exception already delegated to HS when V=1 is further delegated to VS if corresponding hedeleg bit is set
- `norm:hedeleg_acc`: hedeleg bit 18 (software-check) is writable
- `norm:H_trap_vs_csrwrites`: vsepc/vscause/vstval written on trap to VS-mode
- `norm:H_trap_hs_csrwrites`: sepc/scause/stval written on trap to HS-mode

**Test Scope**: Verify the three-level delegation chain (M→HS→VS) of software-check exception (cause=18) in the Hypervisor environment, and the correct writing of vstval/stval.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HCFI-LP-34 | hedeleg[18] writability verification | HS-mode writes hedeleg bit 18=1 and reads back | bit 18 writable (if Zicfilp is implemented); otherwise read-only zero |
| HCFI-LP-35 | LP Fault delegated to VS-mode | medeleg[18]=1, hedeleg[18]=1, henvcfg.LPE=1, VS-mode triggers LP Fault | trap enters VS-mode, vscause=18, vstval=2 |
| HCFI-LP-36 | LP Fault delegated to HS-mode (hedeleg=0) | medeleg[18]=1, hedeleg[18]=0, VS-mode triggers LP Fault | trap enters HS-mode, scause=18, stval=2 |
| HCFI-LP-37 | LP Fault delegated to M-mode (medeleg=0) | medeleg[18]=0, VS-mode triggers LP Fault | trap enters M-mode, mcause=18, mtval=2 |
| HCFI-LP-38 | vsepc correct on LP Fault delegated to VS-mode | medeleg[18]=1, hedeleg[18]=1, VS-mode triggers LP Fault | vsepc = address of target instruction (non-LPAD instruction) |
| HCFI-LP-39 | GVA=0 on LP Fault delegated to VS-mode | medeleg[18]=1, hedeleg[18]=1, VS-mode triggers LP Fault | hstatus.GVA=0 (non-address-translation-related exception) |
| HCFI-LP-40 | sepc correct on LP Fault delegated to HS-mode | medeleg[18]=1, hedeleg[18]=0, VS-mode triggers LP Fault | sepc = address of target instruction (non-LPAD instruction), hstatus.SPV=1 |
| HCFI-LP-41 | MPV=1 on LP Fault trap to M-mode | medeleg[18]=0, VS-mode triggers LP Fault | mstatus.MPV=1, mstatus.MPP=1 |

---

## Group A5. VS-mode Zicfilp Trap Interrupt and Async Event Interaction

**Spec Reference**:
- `norm:Zicfilp_forward_traps`: Async interrupts and sync exceptions with higher priority than software-check may be delivered after JALR but before target decode
- `norm:Zicfilp_forward_trap_async_interrupt`: Async interrupts may be delivered after indirect jump but before target decode
- `norm:Zicfilp_forward_trap_async_exception`: Sync exceptions with higher priority than software-check may be delivered first

**Test Scope**: Verify the correct save and restore of ELP when VS-mode indirect jump is interrupted by async events.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HCFI-LP-42 | VS-mode async interrupt after JALR saves ELP | henvcfg.LPE=1, async interrupt injected after VS-mode JALR execution but before LPAD decode | ELP=LP_EXPECTED saved to vsstatus.SPELP (delegated to VS) or mstatus.SPELP (to HS) |
| HCFI-LP-43 | ELP restored and LP check continues after interrupt return | Following HCFI-LP-42, after interrupt handler returns | ELP restored to LP_EXPECTED, LP check continues on target instruction |
| HCFI-LP-44 | VS-mode high-priority exception after JALR saves ELP | henvcfg.LPE=1, VS-mode JALR to non-executable page (instruction access-fault has higher priority than software-check) | ELP saved, trap reports instruction access-fault rather than software-check |

---

# Part B: Hypervisor × Zicfiss (Backward-Edge Control Flow / Shadow Stack)

---

## Group B1. henvcfg.SSE Field and VS-mode Zicfiss Enable Control

**Spec Reference**:
- `norm:henvcfg_sse_op`: SSE=1 activates VS-mode Zicfiss; SSE=0 causes instruction reversion, page table encoding reserved, senvcfg.SSE read-only zero, SSAMOSWAP triggers virtual-instruction exception
- cfi.adoc xSSE determination table: VS-mode xSSE = henvcfg.SSE; VU-mode xSSE = senvcfg.SSE

**Test Scope**: Verify the enable control of the henvcfg.SSE field over the VS-mode Zicfiss extension, including instruction reversion, page table encoding reservation, and other behaviors when SSE=0.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HCFI-SS-01 | henvcfg.SSE basic read/write | HS-mode writes henvcfg.SSE=1 and reads back | SSE=1 (if Zicfiss is implemented); read-only zero if not implemented |
| HCFI-SS-02 | VS-mode Zicfiss instructions execute normally when henvcfg.SSE=1 | Set henvcfg.SSE=1, menvcfg.SSE=1, VS-mode executes SSPUSH | SSPUSH executes normally, no exception |
| HCFI-SS-03 | 32-bit Zicfiss instructions revert to Zimop when henvcfg.SSE=0 | Set henvcfg.SSE=0, VS-mode executes 32-bit SSPUSH instruction | Instruction executes as Zimop no-op, no exception |
| HCFI-SS-04 | 16-bit Zicfiss instructions revert to Zcmop when henvcfg.SSE=0 | Set henvcfg.SSE=0, VS-mode executes C.SSPUSH instruction | Instruction executes as Zcmop no-op, no exception |
| HCFI-SS-05 | VS-stage pte.xwr=010 reserved when henvcfg.SSE=0 | Set henvcfg.SSE=0, VS-stage page table uses pte.xwr=010 encoding | Encoding reserved, triggers page-fault |
| HCFI-SS-06 | senvcfg.SSE read-only zero when henvcfg.SSE=0 | Set henvcfg.SSE=0, VS-mode reads senvcfg.SSE | senvcfg.SSE=0 and not writable |
| HCFI-SS-07 | senvcfg.SSE write ineffective when henvcfg.SSE=0 | Set henvcfg.SSE=0, VS-mode writes senvcfg.SSE=1 and reads back | senvcfg.SSE remains 0 |
| HCFI-SS-08 | senvcfg.SSE writable when henvcfg.SSE=1 | Set henvcfg.SSE=1, VS-mode writes senvcfg.SSE=1 and reads back | senvcfg.SSE=1 |
| HCFI-SS-09 | VS-mode SSAMOSWAP triggers exception when henvcfg.SSE=0 + menvcfg.SSE=1 | Set henvcfg.SSE=0, menvcfg.SSE=1, VS-mode executes SSAMOSWAP.W | virtual-instruction exception (cause=22) |
| HCFI-SS-10 | VS-mode SSAMOSWAP executes normally when henvcfg.SSE=1 | Set henvcfg.SSE=1, menvcfg.SSE=1, VS-mode executes SSAMOSWAP.W (target is SS page) | Normal execution, no exception |
| HCFI-SS-11 | VS-mode SSAMOSWAP behavior when henvcfg.SSE=0 + menvcfg.SSE=0 | Set henvcfg.SSE=0, menvcfg.SSE=0, VS-mode executes SSAMOSWAP.W | illegal-instruction exception (cause=2) (SSAMOSWAP is AMO encoding, not Zimop; privilege < M and menvcfg.SSE=0 triggers illegal-instruction per cfi.adoc pseudocode) |
| HCFI-SS-12 | VU-mode xSSE controlled by senvcfg.SSE | V=1, henvcfg.SSE=1, senvcfg.SSE=0, VU-mode executes SSPUSH | 32-bit instruction reverts to Zimop no-op (VU-mode xSSE=0) |
| HCFI-SS-13 | henvcfg.SSE read-only zero when Zicfiss not implemented | If Zicfiss is not implemented, read henvcfg.SSE | Read-only zero |
| HCFI-SS-14 | pte.xwr=010 exception type when henvcfg.SSE=0 | Set henvcfg.SSE=0, VS-stage page table pte.xwr=010, VS-mode accesses the page | Triggers page-fault (encoding reserved), not access-fault |

---

## Group B2. ssp CSR Access Control in VS/VU-mode

**Spec Reference**:
- `norm:zicfiss_ssp_csr`: ssp CSR access controlled by envcfg sse fields
- `norm:zicfiss_m_menvcfg_sse`: Privilege < M and menvcfg.SSE=0 → illegal-instruction exception
- `norm:zicfiss_vs_henvcfg_sse`: VS-mode and henvcfg.SSE=0 → virtual-instruction exception
- `norm:zicfiss_vu_henvcfg_senvcfg_sse`: VU-mode and henvcfg.SSE=0 or senvcfg.SSE=0 → virtual-instruction exception
- `norm:zicfiss_sse_access`: Otherwise access is allowed

**Test Scope**: Verify ssp CSR access control in VS/VU-mode, including exception type distinction (illegal vs virtual) and multi-level enable gating.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HCFI-SS-15 | VS-mode + henvcfg.SSE=0 accessing ssp triggers exception | Set henvcfg.SSE=0, menvcfg.SSE=1, VS-mode csrr ssp | virtual-instruction exception (cause=22) |
| HCFI-SS-16 | VS-mode + henvcfg.SSE=1 + menvcfg.SSE=1 accessing ssp normal | Set henvcfg.SSE=1, menvcfg.SSE=1, VS-mode csrr/csrw ssp | Normal read/write |
| HCFI-SS-17 | VS-mode + menvcfg.SSE=0 accessing ssp triggers exception | Set menvcfg.SSE=0 (henvcfg.SSE arbitrary), VS-mode csrr ssp | illegal-instruction exception (cause=2) (menvcfg.SSE=0 has highest priority) |
| HCFI-SS-18 | VU-mode + henvcfg.SSE=0 accessing ssp triggers exception | Set henvcfg.SSE=0, senvcfg.SSE=1, menvcfg.SSE=1, VU-mode csrr ssp | virtual-instruction exception (cause=22) |
| HCFI-SS-19 | VU-mode + senvcfg.SSE=0 accessing ssp triggers exception | Set henvcfg.SSE=1, senvcfg.SSE=0, menvcfg.SSE=1, VU-mode csrr ssp | virtual-instruction exception (cause=22) |
| HCFI-SS-20 | VU-mode + henvcfg.SSE=1 + senvcfg.SSE=1 + menvcfg.SSE=1 accessing ssp normal | All enabled, VU-mode csrr/csrw ssp | Normal read/write |
| HCFI-SS-21 | VU-mode + menvcfg.SSE=0 accessing ssp triggers exception | Set menvcfg.SSE=0, VU-mode csrr ssp | illegal-instruction exception (cause=2) |
| HCFI-SS-22 | VS-mode writes ssp, HS-mode can read | Set henvcfg.SSE=1, menvcfg.SSE=1, VS-mode csrw ssp=VAL, HS-mode csrr ssp | Reads VAL |
| HCFI-SS-23 | VS-mode ssp alignment check | Set henvcfg.SSE=1, VS-mode writes non-XLEN-aligned ssp then executes SSPUSH | store/AMO access-fault exception (`norm:ssp_xlen_aligned`) |

---

## Group B3. VS-stage Page Table Shadow Stack Page Type

**Spec Reference**:
- `norm:ss_page_enc`: R=0, W=1, X=0 represents SS page
- `norm:ssmp_henvcfg_sse`: V=1 and henvcfg.SSE=0 makes this encoding reserved
- `norm:ssmp_ss_page_access_fault`: Non-SS instruction writing to SS page → store/AMO access-fault
- `norm:ssmp_ss_page_illegeal_access`: SS instruction accessing non-SS non-read-only page → store/AMO access-fault
- `norm:ssmp_ss_read_only_page`: SS instruction accessing read-only page → store/AMO page-fault
- `norm:ss_fault_exception_code`: SS instruction exception cause is 7/15/23
- `norm:ssmp_ss_idempotent_memory`: Non-idempotent memory → store/AMO access-fault

**Test Scope**: Verify the encoding, protection mechanism, and exception behavior of Shadow Stack page type in VS-stage page tables.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HCFI-SS-24 | pte.xwr=010 is SS page when henvcfg.SSE=1 | Set henvcfg.SSE=1, VS-stage page table pte.xwr=010, VS-mode SSPUSH to the page | Normal execution (SS page valid) |
| HCFI-SS-25 | pte.xwr=010 reserved when henvcfg.SSE=0 | Set henvcfg.SSE=0, VS-stage page table pte.xwr=010, VS-mode accesses the page | page-fault (encoding reserved) |
| HCFI-SS-26 | Normal store to SS page triggers access-fault | Set henvcfg.SSE=1, VS-stage page table pte.xwr=010, VS-mode executes normal SW | store/AMO access-fault (cause=7) |
| HCFI-SS-27 | Normal load from SS page succeeds | Set henvcfg.SSE=1, VS-stage page table pte.xwr=010, VS-mode executes LW | Normal read (SS page readable by all load instructions) |
| HCFI-SS-28 | SS instruction accessing non-SS non-read-only page triggers access-fault | Set henvcfg.SSE=1, VS-stage page table pte.xwr=011 (RW), VS-mode SSPUSH to the page | store/AMO access-fault (cause=7) |
| HCFI-SS-29 | SS instruction accessing read-only page triggers page-fault | Set henvcfg.SSE=1, VS-stage page table pte.xwr=001 (R), VS-mode SSPUSH to the page | store/AMO page-fault (cause=15) |
| HCFI-SS-30 | CBO instruction accessing SS page triggers access-fault | Set henvcfg.SSE=1, VS-stage page table pte.xwr=010, VS-mode executes CBO.INVAL | store/AMO access-fault (cause=7) |
| HCFI-SS-31 | Instruction fetch from SS page triggers access-fault | Set henvcfg.SSE=1, VS-stage page table pte.xwr=010, VS-mode jumps to the page | instruction access-fault (SS page does not allow instruction fetch) |
| HCFI-SS-32 | SSPOPCHK reading SS page succeeds | Set henvcfg.SSE=1, VS-stage page table pte.xwr=010, VS-mode SSPUSH then SSPOPCHK | Normal execution (SSPOPCHK reads SS page) |
| HCFI-SS-33 | SSPOPCHK reading SS page exception reported as store type | Set henvcfg.SSE=1, VS-stage SS page has no mapping, VS-mode SSPOPCHK | store/AMO page-fault (cause=15), not load page-fault |
| HCFI-SS-34 | SS page COW (copy-on-write) scenario | Set henvcfg.SSE=1, VS-stage page table pte.xwr=001 (read-only, COW marked), VS-mode SSPUSH | store/AMO page-fault (cause=15), allowing OS to handle COW |
| HCFI-SS-35 | U/SUM bit effect on SS page access | Set henvcfg.SSE=1, vsstatus.SUM=1, VS-mode accesses VU-mode SS page | U/SUM bits take effect normally for SS instruction access |
| HCFI-SS-36 | MXR bit does not affect SS page read | Set henvcfg.SSE=1, vsstatus.MXR=1, verify MXR does not affect SS page load behavior | SS page always readable by load instructions, MXR has no additional effect |

---

## Group B4. G-stage Translation and Shadow Stack Interaction

**Spec Reference**:
- cfi.adoc: G-stage address translation and protection are not affected by Zicfiss extension, G-stage pte.xwr=010 remains reserved
- `norm:active_g_stage_pte`: When G-stage page tables are active, shadow stack instructions require G-stage read-write permission, otherwise store/AMO guest-page-fault

**Test Scope**: Verify the interaction between G-stage address translation and VS-mode shadow stack instructions.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HCFI-SS-37 | G-stage page table pte.xwr=010 remains reserved | Set G-stage page table pte.xwr=010, VS-mode SSPUSH | guest-page-fault (G-stage does not support SS page encoding) |
| HCFI-SS-38 | SS instruction executes normally with G-stage RW permission | Set henvcfg.SSE=1, VS-stage SS page valid, G-stage page table pte.xwr=011 (RW), VS-mode SSPUSH | Normal execution |
| HCFI-SS-39 | SS instruction triggers guest-page-fault with G-stage read-only permission | Set VS-stage SS page valid, G-stage page table pte.xwr=001 (read-only), VS-mode SSPUSH | store/AMO guest-page-fault (cause=23) |
| HCFI-SS-40 | SS instruction triggers guest-page-fault with G-stage no permission | Set VS-stage SS page valid, G-stage page table pte.xwr=000 (no permission), VS-mode SSPUSH | store/AMO guest-page-fault (cause=23) |
| HCFI-SS-41 | G-stage guest-page-fault exception delegation | VS-mode SSPUSH triggers G-stage guest-page-fault | trap to HS-mode (guest-page-fault cannot be delegated to VS), hstatus.GVA=1 |
| HCFI-SS-42 | SSPOPCHK triggers G-stage guest-page-fault | VS-mode SSPOPCHK, G-stage page table has no write permission | store/AMO guest-page-fault (cause=23) (not load guest-page-fault) |

---

## Group B5. Shadow Stack Behavior in satp/vsatp Bare Mode

**Spec Reference**:
- `norm:satp_mode_bare`: When satp.mode (vsatp.mode when V=1) is Bare and effective privilege < M, shadow stack instructions raise store/AMO access-fault

**Test Scope**: Verify the exception behavior of shadow stack instructions in VS/VU-mode when vsatp.mode=Bare.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HCFI-SS-43 | VS-mode SSPUSH triggers access-fault when vsatp.mode=Bare | Set henvcfg.SSE=1, vsatp.mode=Bare, V=1, VS-mode executes SSPUSH | store/AMO access-fault (cause=7) |
| HCFI-SS-44 | VS-mode SSPOPCHK triggers access-fault when vsatp.mode=Bare | Set henvcfg.SSE=1, vsatp.mode=Bare, V=1, VS-mode executes SSPOPCHK | store/AMO access-fault (cause=7) |
| HCFI-SS-45 | VS-mode SSAMOSWAP triggers access-fault when vsatp.mode=Bare | Set henvcfg.SSE=1, vsatp.mode=Bare, V=1, VS-mode executes SSAMOSWAP.W | store/AMO access-fault (cause=7) |
| HCFI-SS-46 | VU-mode SS instruction triggers access-fault when vsatp.mode=Bare | Set henvcfg.SSE=1, senvcfg.SSE=1, vsatp.mode=Bare, V=1, VU-mode executes SSPUSH | store/AMO access-fault (cause=7) |
| HCFI-SS-47 | SS instruction normal when vsatp.mode is not Bare | Set henvcfg.SSE=1, vsatp.mode=Sv39, VS-stage SS page valid, VS-mode SSPUSH | Normal execution |
| HCFI-SS-48 | ssp CSR still accessible when vsatp.mode=Bare | Set henvcfg.SSE=1, vsatp.mode=Bare, V=1, VS-mode csrr ssp | Normal read (CSR access not affected by translation mode) |

---

## Group B6. VS-mode Zicfiss Exception Behavior and Delegation

**Spec Reference**:
- `norm:hedeleg_op`: Synchronous exception already delegated to HS when V=1 is further delegated to VS if corresponding hedeleg bit is set
- `norm:hedeleg_acc`: hedeleg bit 18 is writable
- `norm:ss_fault_exception_code`: SS instruction exception cause is 7/15/23
- cfi.adoc: software-check exception (cause=18, mtval/stval=3) for shadow stack fault

**Test Scope**: Verify the delegation chain and CSR writing of various exceptions raised by Zicfiss in VS-mode (software-check exception, access-fault, page-fault, guest-page-fault).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HCFI-SS-49 | VS-mode SSPOPCHK mismatch triggers SS Fault | henvcfg.SSE=1, VS-mode SSPUSH then tamper shadow stack, then SSPOPCHK | software-check exception (cause=18, vstval=3) |
| HCFI-SS-50 | SS Fault delegated to VS-mode | medeleg[18]=1, hedeleg[18]=1, VS-mode triggers SS Fault | trap enters VS-mode, vscause=18, vstval=3 |
| HCFI-SS-51 | SS Fault delegated to HS-mode | medeleg[18]=1, hedeleg[18]=0, VS-mode triggers SS Fault | trap enters HS-mode, scause=18, stval=3 |
| HCFI-SS-52 | SS Fault delegated to M-mode | medeleg[18]=0, VS-mode triggers SS Fault | trap enters M-mode, mcause=18, mtval=3 |
| HCFI-SS-53 | VS-mode SS access-fault delegated to VS | medeleg[7]=1, hedeleg[7]=1, VS-mode normal store to SS page | trap enters VS-mode, vscause=7 |
| HCFI-SS-54 | VS-mode SS access-fault delegated to HS | medeleg[7]=1, hedeleg[7]=0, VS-mode normal store to SS page | trap enters HS-mode, scause=7 |
| HCFI-SS-55 | VS-mode SS page-fault delegated to VS | medeleg[15]=1, hedeleg[15]=1, VS-mode SS instruction accesses read-only page | trap enters VS-mode, vscause=15 |
| HCFI-SS-56 | VS-mode SS guest-page-fault cannot be delegated to VS | VS-mode SS instruction triggers G-stage guest-page-fault | trap enters HS-mode (hedeleg[20/21/23] read-only zero), hstatus.GVA=1 |
| HCFI-SS-57 | vsepc correct on SS Fault trap to VS | medeleg[18]=1, hedeleg[18]=1, VS-mode triggers SS Fault | vsepc = address of SSPOPCHK instruction |
| HCFI-SS-58 | sepc/scause correct on SS access-fault trap to HS | medeleg[7]=1, hedeleg[7]=0, VS-mode normal store to SS page | sepc = store instruction address, scause=7, hstatus.SPV=1 |

---

## Group B7. VS-mode Zicfiss Functional Completeness

**Spec Reference**:
- `norm:henvcfg_sse_op`: SSE=1 fully activates VS-mode Zicfiss
- cfi.adoc: SSPUSH/SSPOPCHK/SSAMOSWAP instruction behavior
- `norm:ssmp_ss_idempotent_memory`: Shadow stack memory must be idempotent

**Test Scope**: Verify the complete functional chain of Zicfiss in VS-mode, including shadow stack push, pop, atomic swap, and other operations.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HCFI-SS-59 | VS-mode SSPUSH/SSPOPCHK basic flow | henvcfg.SSE=1, VS-mode sets ssp pointing to SS page, SSPUSH then SSPOPCHK | No exception on match, normal return |
| HCFI-SS-60 | VS-mode C.SSPUSH/C.SSPOPCHK basic flow | henvcfg.SSE=1, VS-mode uses compressed instruction versions | Normal execution |
| HCFI-SS-61 | VS-mode SSAMOSWAP.W atomic swap | henvcfg.SSE=1, menvcfg.SSE=1, VS-mode executes SSAMOSWAP.W on SS page | Atomic swap succeeds |
| HCFI-SS-62 | VS-mode SSAMOSWAP.D atomic swap | henvcfg.SSE=1, menvcfg.SSE=1, VS-mode executes SSAMOSWAP.D on SS page | Atomic swap succeeds |
| HCFI-SS-63 | ssp state after SSPOPCHK mismatch exception in VS-mode | henvcfg.SSE=1, VS-mode SSPUSH then modify ssp to cause SSPOPCHK mismatch | software-check exception (cause=18, vstval=3) |
| HCFI-SS-64 | VS-mode Shadow Stack guard page detection | henvcfg.SSE=1, VS-mode SSPUSH out of bounds to non-SS page (guard page) | store/AMO access-fault (cause=7) |
| HCFI-SS-65 | VS-mode SS instruction on non-idempotent memory triggers exception | henvcfg.SSE=1, VS-mode SSPUSH to device memory (non-idempotent) | store/AMO access-fault (cause=7) |
| HCFI-SS-66 | VS-mode SSAMOSWAP requires AMOSwap-level PMA support | henvcfg.SSE=1, VS-mode SSAMOSWAP.W to memory region without AMOSwap support | store/AMO access-fault (cause=7) |
| HCFI-SS-67 | VU-mode SSPUSH/SSPOPCHK basic flow | henvcfg.SSE=1, senvcfg.SSE=1, menvcfg.SSE=1, VU-mode SSPUSH/SSPOPCHK | Normal execution |
| HCFI-SS-68 | VS-mode ssp preserved across traps | henvcfg.SSE=1, VS-mode sets ssp=VAL, triggers trap to HS-mode then returns | ssp remains VAL (ssp is a regular CSR, not auto-saved/restored on trap) |

---

## Group B8. Comprehensive VS-mode Zicfiss Reversion Behavior when henvcfg.SSE=0

**Spec Reference**:
- `norm:henvcfg_sse_op`: Complete reversion rule list when SSE=0
- cfi.adoc: Zicfiss instructions revert to Zimop/Zcmop behavior when extension is not enabled

**Test Scope**: Comprehensively verify the consistency of all Zicfiss-related behaviors in VS-mode when henvcfg.SSE=0.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HCFI-SS-69 | 32-bit SS instruction behavior when SSE=0 | henvcfg.SSE=0, menvcfg.SSE=1, VS-mode executes SSPUSH/SSPOPCHK/SSAMOSWAP.W/SSAMOSWAP.D one by one | SSPUSH/SSPOPCHK revert to Zimop no-op; SSAMOSWAP.W/D trigger virtual-instruction exception (cause=22) (AMO encoding does not revert, see norm:henvcfg_sse_op) |
| HCFI-SS-70 | All 16-bit SS instructions revert when SSE=0 | henvcfg.SSE=0, VS-mode executes C.SSPUSH/C.SSPOPCHK one by one | All execute as Zcmop no-op |
| HCFI-SS-71 | ssp CSR access still triggers exception when SSE=0 | henvcfg.SSE=0, menvcfg.SSE=1, VS-mode csrr ssp | virtual-instruction exception (cause=22) (ssp access is controlled, not instruction reversion) |
| HCFI-SS-72 | SS page encoding reserved when SSE=0 (VS-stage and VU-stage) | henvcfg.SSE=0, VS/VU-stage page table pte.xwr=010 | Encoding reserved, access triggers page-fault |
| HCFI-SS-73 | SS functionality immediately available after SSE=0→1 switch | First set henvcfg.SSE=0 and execute SS instruction (no-op), then set SSE=1 and execute same instruction | After SSE=1, instruction executes normally as Zicfiss behavior |
| HCFI-SS-74 | SS instruction reverts after SSE=1→0 switch | First set henvcfg.SSE=1 and execute SS instruction normally, then set SSE=0 and execute same instruction | After SSE=0, instruction reverts to no-op |
| HCFI-SS-75 | SSE switch does not require address translation cache synchronization | Switch henvcfg.SSE from 0 to 1 without executing SFENCE.VMA/HFENCE.VVMA, immediately execute SSPUSH | Normal execution (SSE change takes effect immediately, no cache synchronization needed) |

---

## Test Prerequisites

### Framework-Level Additions Required

> [!IMPORTANT]
> The following modifications are prerequisites for Hypervisor × CFI cross testing.

1. **henvcfg LPE/SSE field macros**: Add `HENVCFG_LPE` and `HENVCFG_SSE` macros in `common/encoding.h`.
2. **vsstatus.SPELP field macro**: Add `VSSTATUS_SPELP_BIT` macro in `common/encoding.h`.
3. **software-check exception cause**: Add `CAUSE_SOFTWARE_CHECK` (18) macro in `common/encoding.h`.
4. **mtval/stval/vstval CFI encoding**: Add `SWCHECK_LANDING_PAD_FAULT` (2) and `SWCHECK_SHADOW_STACK_FAULT` (3) macros.
5. **CSR_SSP macro**: Add `CSR_SSP` (0x011) macro in `common/encoding.h`.
6. **trap handler enhancement**: Recognize software-check exception (cause=18), correctly capture and record `mtval`/`stval`/`vstval` values in armed trap scenarios.
7. **VS-mode CFI test infrastructure**: Add compilation and execution support for VS-mode Zicfilp/Zicfiss instructions in the Hypervisor test framework.

### Test File Structure

The actual implementation is split into two independent test directories (Zicfilp and Zicfiss built separately):

```
Hypervisor_Zicfilp/
├── Makefile
├── kernel.ld
├── main.c
└── tests/
    ├── test_helpers.h
    ├── test_hyp_zicfilp_envcfg.c     # Group A1: henvcfg.LPE enable control
    ├── test_hyp_zicfilp_spelp.c      # Group A2: vsstatus.SPELP save/restore
    ├── test_hyp_zicfilp_lpad.c       # Group A3: VS-mode Landing Pad functionality
    ├── test_hyp_zicfilp_deleg.c      # Group A4: software-check exception delegation
    └── test_hyp_zicfilp_async.c      # Group A5: async events and ELP interaction
Hypervisor_Zicfiss/
├── Makefile
├── kernel.ld
├── main.c
└── tests/
    ├── test_helpers.h
    ├── test_hyp_zicfiss_envcfg.c     # Group B1: henvcfg.SSE enable control
    ├── test_hyp_zicfiss_ssp_csr.c    # Group B2: ssp CSR access control
    ├── test_hyp_zicfiss_ss_page.c    # Group B3: VS-stage SS page type
    ├── test_hyp_zicfiss_gstage.c     # Group B4: G-stage translation interaction
    ├── test_hyp_zicfiss_bare.c       # Group B5: satp/vsatp Bare mode
    ├── test_hyp_zicfiss_deleg.c      # Group B6: Zicfiss exception delegation
    ├── test_hyp_zicfiss_func.c       # Group B7: Zicfiss functional completeness
    └── test_hyp_zicfiss_revert.c     # Group B8: SSE=0 reversion behavior
```

---

## Verification Plan

### Automated Tests
- `cd Hypervisor_Zicfilp && make clean && make qemu` — QEMU simulator (requires H extension + Zicfilp support)
- `cd Hypervisor_Zicfiss && make clean && make qemu` — QEMU simulator (requires H extension + Zicfiss support)
- `cd Hypervisor_Zicfilp && make clean && make spike` / `cd Hypervisor_Zicfiss && make clean && make spike` — Spike simulator
- `cd Hypervisor_Zicfilp && make clean && make sail` / `cd Hypervisor_Zicfiss && make clean && make sail` — Sail simulator

### Manual Verification
- Compile with `make PLATFORM=haps_xiaohui` on hardware platform, then deploy tests using `remote_debug.py`

### Notes
1. Confirm that the target platform implements H extension and Zicfilp/Zicfiss extensions before testing.
2. If the platform does not implement CFI extensions, henvcfg.LPE/SSE should be read-only zero, and related tests should report "extension not implemented" rather than fail.
3. Delegation tests for software-check exception (cause=18) require confirming the writability of hedeleg bit 18 on the platform.
4. Shadow Stack page type tests require fine-grained control of VS-stage page tables to ensure correct pte.xwr encoding.
5. Do not lower SPEC standards to pass tests. If QEMU/Sail/hardware behavior is inconsistent with SPEC, report the issue rather than modifying the test.
6. Current known implementation status (known gap):
   - Group A5 (HCFI-LP-31/42/43) async interrupt injection window (after JALR, before LPAD decode) is non-deterministic; currently uses sync software-check exception as a proxy to verify ELP save/restore mechanism.
   - HCFI-SS-04/60/70 (16-bit compressed instruction reversion/flow), HCFI-SS-30 (real CBO instruction), HCFI-SS-65/66 (non-idempotent memory / AMOSwap PMA) are not directly implemented yet, marked as known gap in comments.
7. Current QEMU (cskysim) confirmed SPEC deviations (tests remain FAIL to report issues, no workaround):
   - VS→HS trap saves ELP to vsstatus.SPELP instead of mstatus.SPELP (affects HCFI-LP-15/16/17/31/42).
   - VS-mode xLPE is gated by menvcfg.LPE (SPEC requires VS xLPE = henvcfg.LPE to take effect independently, affects HCFI-LP-09).
