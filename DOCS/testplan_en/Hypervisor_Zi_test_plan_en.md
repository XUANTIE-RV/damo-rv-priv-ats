**[中文](../testplan/Hypervisor_Zi_test_plan.md) | English**

# Hypervisor × Z* Extensions Cross Test Plan

> This document describes the test plan for cross-scenarios between the Hypervisor (H) extension and other Z* extension families (including Zk* cryptography extensions). This plan was split out from `Hypervisor_cross_test_plan.md` and retains only the content at the intersection of the Hypervisor and Z* extensions. These test scenarios were originally marked in the standalone test plans of the respective extensions as "covered by the Hypervisor test plan" or "excluded due to the absence of the H extension", but analysis showed that the existing Hypervisor test plans do not fully cover them. Hypervisor × Zawrs cross scenarios (Group 4), Hypervisor × V vector family cross scenarios (Group 5), Hypervisor × Zicntr cross scenarios (Group 6), and Hypervisor × Zihpm cross scenarios (Group 7) were supplemented afterwards.
>
> Generation date: 2026-06-22

---

## SPEC Sections Covered by This Document

This plan is based on the following official RISC-V specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — Hypervisor (H) extension: virtual-instruction exception mechanism for VS/VU-mode access to controlled CSRs
- `SPEC/riscv-isa-manual/src/unpriv/zk.adoc` — Zkr entropy source extension: `seed` CSR, `mseccfg.SSEED/USEED` access control
- `SPEC/riscv-isa-manual/src/unpriv/zihintntl.adoc` — Zihintntl extension: no-architectural-side-effect semantics and trap behavior of NTL HINT instructions
- `SPEC/riscv-isa-manual/src/unpriv/zcmt.adoc` — Zcmt extension: cm.jt/cm.jalt table jump instructions, jvt CSR, and two-implicit-fetch semantics
- `SPEC/riscv-isa-manual/src/unpriv/zawrs.adoc` — Zawrs extension: wrs.nto/wrs.sto wait-on-reservation-set instructions, virtual-instruction mechanism gated by hstatus.VTW
- `SPEC/riscv-isa-manual/src/unpriv/vector-common.adoc` — V vector family common definitions: vsstatus.vs vector context status field, Off gating of vector instructions/vector CSRs and dual Dirty updates when V=1, vsstatus.fs interaction for vector floating point
- `SPEC/riscv-isa-manual/src/unpriv/zicntr.adoc` — Zicntr extension: cycle/time/instret base counters, the intersection of `rdtime` semantics and the htimedelta time offset, continued control of VU-mode by `scounteren` when V=1 (no corresponding VS CSR)
- `SPEC/riscv-isa-manual/src/unpriv/zihpm.adoc` — Zihpm extension: gating interaction when V=1 of the access behavior of unimplemented hpmcounter3–31 (illegal-instruction or constant value)

Official repository:

- https://github.com/riscv/riscv-isa-manual (files at the above paths within the repository)

---

## Scope

### Covered Extension Intersections

- **Hypervisor × Zkr**: `mseccfg.SSEED` control over VS/VU-mode access to the `seed` CSR; distinguishing virtual-instruction from illegal-instruction exception types in VS/VU-mode; SSEED control of HS-mode access to seed; read-only access exceptions taking precedence over virtual-instruction
- **Hypervisor × Zihintntl**: normal execution of NTL HINTs in HS/VS/VU-mode (must not spuriously raise a virtual-instruction exception); NTL applied to H-extension virtual-machine memory access instructions (HLV/HSV/HLVX); virtual-instruction reporting of NTL + CMO in VS-mode; G-stage guest-page-fault reporting of NTL + target in VS-mode
- **Hypervisor × Zcmt**: normal execution of table jump instructions (cm.jt/cm.jalt) in HS/VS/VU-mode; jvt CSR access and Smstateen (JVT bit) gating in VS/VU-mode; two-stage translation of JVT entry fetches in VS-mode and G-stage guest instruction page fault reporting
- **Hypervisor × Zawrs**: normal execution of wrs.nto/wrs.sto in HS/VS/VU-mode (must not spuriously raise a virtual-instruction exception); `hstatus.VTW` virtual-instruction gating of `wrs.nto` in VS/VU-mode; exception type determination with `mstatus.TW` taking precedence over `hstatus.VTW`; the VTW clause applies only to `wrs.nto` and not to `wrs.sto`
- **Hypervisor × V vector family**: presence and read/write of the `vsstatus.vs` field (bits[10:9]) when the H extension is implemented; illegal-instruction gating of vector instructions and vector CSRs when either `vsstatus.vs` or `mstatus.vs` is Off at V=1; modifying vector state sets both to Dirty; linkage between `vsstatus.sd` and `vsstatus.vs`; `vsstatus.fs` gating and dual Dirty updates for vector floating-point instructions; presence of `vsstatus.vs` when `misa.v` is writable (applicable to all vector extensions sharing `vector-common.adoc`: V, Zve*, Zv*)
- **Hypervisor × Zicntr**: the `rdtime` instruction returns `time + htimedelta` in VS/VU-mode (instruction-level delta semantics); gating of `cycle`/`time`/`instret` by the CY/TM/IR bits of `hcounteren` (as a precondition and exception type distinction); continued control of VU-mode visibility of base counters by `scounteren` (which has no corresponding VS CSR) when V=1 (including the blocked branch when `hcounteren`=0)
- **Hypervisor × Zihpm**: access behavior of unimplemented `hpmcounter3–31` at V=1 (both constant value and exception are legal) and its interaction with `hcounteren` HPMn gating; the three-level `mcounteren → hcounteren → scounteren` gating chain for VU-mode access to `hpmcounter`

### Out of Scope for This Document

- Hypervisor basic functionality already covered by `Hypervisor_CSR_test_plan.md`, `Hypervisor_Interrupts_test_plan.md`, `Hypervisor_Exceptions_test_plan.md`, `Hypervisor_2_stage_test_plan.md`, `Hypervisor_gstage_test_plan.md`
- Behavior of each extension in non-Hypervisor scenarios (covered by their respective standalone test plans)
- Cross tests between the Hypervisor and Ss\*/Sv\*/Sm\* extensions (covered by `Hypervisor_Ss_test_plan.md`, `Hypervisor_Sv_test_plan.md`, `Hypervisor_Sm_test_plan.md` respectively)
- Zkr non-Hypervisor scenarios (basic control of seed access in M/S/U-mode) — covered by `Zkr_test_plan.md`
- Zihintntl non-Hypervisor scenarios (basic semantics in M/S/U-mode, encodings, compressed variants, CMO interaction, LR/SC forward progress guarantees, etc.) — covered by `zihintntl_test_plan.md`
- Zcmt non-Hypervisor scenarios (jvt WARL behavior, encoding and operational semantics, PMP/page-table fault handling, table update visibility and endianness, etc.) — covered by `zcmt_test_plan.md`
- Zawrs non-Hypervisor scenarios (instruction encoding and availability, stall and resume semantics, mstatus.TW timeout illegal-instruction behavior, etc.) — covered by `Zawrs_test_plan.md`
- V vector family non-Hypervisor scenarios (vtype/vl behavior, basic semantics of vector instructions, non-virtualized gating of mstatus.vs, etc.) — no standalone test plan currently exists and they are out of scope for this document; `norm:vsstatus_vs_op`/`norm:vsstatus_fs_op` in `hypervisor.adoc` (equivalent to `vector-common.adoc`) are already covered by `Hypervisor_CSR_test_plan.md` (VSST-03~09) and are not duplicated here
- Zicntr non-Hypervisor scenarios (cycle/time/instret counter semantics, M/S/U-mode counteren control matrix) — covered by `Zicntr_test_plan.md`, `Sm_CSR_test_plan.md`, `Ss_CSR_test_plan.md`
- Zihpm non-Hypervisor scenarios (hpmcounter semantics and event configuration, M/S/U-mode counteren control matrix) — covered by `Zihpm_test_plan.md`, `Sm_CSR_test_plan.md`, `Ss_CSR_test_plan.md`
- Bit-level writability of `hcounteren` and the write-back/gating matrix of VS/VU-mode counter access — covered by `Shcounterenw_test_plan.md` and `Hypervisor_Ss_test_plan.md` Group 3 (Hypervisor × Sscounterenw); this document (Group 6/7) only supplements the instruction-level and unimplemented-counter behavior cases not covered there, without re-verifying the gating matrix itself

---

## Covered Specification Points

The following table lists the specification points covered by this plan. Entries with the `norm:` prefix are official SPEC labels; entries without the prefix are specification points decomposed from the SPEC text.

| Norm ID | Source | Description |
|---------|--------|-------------|
| `norm:mseccfg_sseed_VSorVU-mode_op` | `zk.adoc` | When the H extension is also implemented, access to the seed CSR from an HS-qualified instruction leads to a virtual-instruction exception in VS and VU modes; all other types of accesses raise an illegal-instruction exception. |
| `norm:mseccfg_sseed_SorHS-mode_op` | `zk.adoc` | When SSEED is 0, access to the seed CSR from S-/HS-mode raises an illegal-instruction exception. When SSEED is 1, read-write access to the seed CSR from S-/HS-mode is allowed; all other types of accesses raise an illegal-instruction exception. |
| `norm:mseccfg_sseed_useed_op_tbl` | `zk.adoc` | Entropy Source Access Control table: M always available; U controlled by USEED; S/HS controlled by SSEED; VS/VU controlled by SSEED with virtual-instruction exception for HS-qualified read-write. |
| `norm:seed_ro_illegal` | `zk.adoc` | Attempts to access the seed CSR using a read-only CSR-access instruction (csrrs/csrrc with rs1=x0 or csrrsi/csrrci with uimm=0) raise an illegal-instruction exception; any other CSR-access instruction may be used to access seed. |
| `norm:NTL_target_definition` | `zihintntl.adoc` | The insn:ntl[] instructions do not change architectural state, nor do they alter the architecturally visible effects of the target instruction. |
| `norm:NTL_range` | `zihintntl.adoc` | The insn:ntl[] instructions affect all memory-access instructions except the cache-management instructions in the ext:zicbom[] extension. |
| `norm:cm-jt_op` | `zcmt.adoc` | cm.jt reads an entry from the jump vector table in memory and jumps to the address that was read. |
| `norm:cm-jalt_op` | `zcmt.adoc` | cm.jalt reads an entry from the jump vector table in memory and jumps to the address that was read, linking to _ra_. |
| `norm:jvt_base_vm` | `zcmt.adoc` | jvt[base] is a virtual address, whenever virtual memory is enabled. |
| `norm:Zcmt_fetch` | `zcmt.adoc` | ... the execution of a table jump instruction involves two instruction fetches, the first to read the instruction (cm.jt/cm.jalt) and the second to read from the jump vector table (JVT). Both instruction fetches are _implicit_ reads, and both require execute permission; read permission is irrelevant. |
| `norm:Zcmt_trap` | `zcmt.adoc` | If an exception occurs on either instruction fetch, xEPC is set to the PC of the table jump instruction, xCAUSE is set as expected for the type of fault and xTVAL (if not set to zero) contains the fetch address which caused the fault. |
| `norm:Zawrs_exec_resume_rules` | `zawrs.adoc` | The wrs.nto and wrs.sto instructions follow the rules of the wfi instruction for resuming execution on a locally enabled pending interrupt. |
| `norm:Zawrs_virtual_instr_excp` | `zawrs.adoc` | When executing in VS- or VU-mode, if the vtw bit is set in hstatus, the tw bit in mstatus is clear, and the wrs.nto does not complete within an implementation-specific bounded time limit, the wrs.nto instruction will cause a virtual-instruction exception. |
| `norm:Zawrs_priv_illegal_instr_excp` | `zawrs.adoc` | When the tw (timeout wait) bit in mstatus is set and wrs.nto is executed in any privilege mode other than M-mode, and it does not complete within an implementation-specific bounded time limit, the wrs.nto instruction will cause an illegal-instruction exception. |
| `norm:Zawrs_stall_terminate` | `zawrs.adoc` | While stalled, an implementation is permitted to occasionally terminate the stall and complete execution for any reason. |
| `norm:vsstatus_vs_sz_acc` | `vector-common.adoc` | When the hypervisor extension is present, a vector context status field, vs, is added to vsstatus[10:9]. It is defined analogously to the floating-point context status field, fs. |
| `norm:vsstatus_vs_mstatus_vs_op_off` | `vector-common.adoc` | When V=1, both vsstatus.vs and mstatus.vs are in effect: attempts to execute any vector instruction, or to access the vector CSRs, raise an illegal-instruction exception when either field is set to Off. |
| `norm:vsstatus_vs_mstatus_vs_op_active` | `vector-common.adoc` | When V=1 and neither vsstatus.vs nor mstatus.vs is set to Off, executing any instruction that changes vector state, including the vector CSRs, will change both mstatus.vs and vsstatus.vs to Dirty. |
| `norm:hw_mstatus_vs_dirty_update` | `vector-common.adoc` | Implementations may also change mstatus.vs or vsstatus.vs from Initial or Clean to Dirty at any time, even when there is no change in vector state. |
| `norm:vsstatus_sd_op_vs` | `vector-common.adoc` | If vsstatus.vs is Dirty, vsstatus.sd is 1; otherwise, vsstatus.sd is set in accordance with existing specifications. |
| `norm:vsstatus_vs_exists` | `vector-common.adoc` | For implementations with a writable misa.v field, the vsstatus.vs field may exist even if misa.v is clear. |
| `norm:vsstatus_mstatus_FS_off_hypervisor_V_fp_ill` | `vector-common.adoc` | If the hypervisor extension is implemented and V=1, the vsstatus.fs field is additionally in effect for vector floating-point instructions. If vsstatus.fs or mstatus.fs is Off then any attempt to execute a vector floating-point instruction will raise an illegal-instruction exception. |
| `norm:vsstatus_mstatus_FS_dirty_hypervisor_V_fp` | `vector-common.adoc` | Any vector floating-point instruction that modifies any floating-point extension state (i.e., floating-point CSRs or f registers) must set both mstatus.fs and vsstatus.fs to Dirty. |
| `norm:zicntr_rdtime_op` | `zicntr.adoc` | The rdtime pseudoinstruction reads the low XLEN bits of the time CSR, which counts wall-clock real time that has passed from an arbitrary start time in the past. |
| `norm:hpm_unimplemented_counter_access` | `zihpm.adoc` | Accessing an unimplemented counter may cause an illegal-instruction exception or may return a constant value. |
| `H_scsrs_nomatch_vu_counter` | `hypervisor.adoc` (counter specialization of `norm:H_scsrs_nomatch`) | Some standard supervisor CSRs (senvcfg, scounteren, and scontext, possibly others) have no matching VS CSR. These supervisor CSRs continue to have their usual function and accessibility even when V=1, except with VS-mode and VU-mode substituting for HS-mode and U-mode. |
| `hcounteren_gate_v1_counter` | `hypervisor.adoc` (specialization of `norm:hcounteren_op`) | When the CY, TM, IR, or HPMn bit in hcounteren is clear, attempts to read the corresponding counter while V=1 will cause a virtual-instruction exception if the same bit in mcounteren is 1. |
| `norm:stateen0_jvt_op` | `smstateen.adoc` | The JVT bit controls access to the `jvt` CSR provided by the Zcmt extension. |
| `norm:H_virtinst_xtval` | `hypervisor.adoc` | On a virtual-instruction trap, `mtval` or `stval` is written the same as for an illegal-instruction trap. |
| `norm:htval_trapval` | `hypervisor.adoc` | htval trap value reporting for guest-page faults (implementation may write zero or the faulting GPA>>2). |

---

## Group 1. Hypervisor × Zkr Cross Tests

**Spec Reference**:
- `norm:mseccfg_sseed_VSorVU-mode_op`: When the H extension is implemented, HS-qualified instruction access to seed in VS/VU-mode raises a virtual-instruction exception; other access types raise an illegal-instruction exception
- `norm:mseccfg_sseed_useed_op_tbl`: With VS/VU + SSEED=0, any access raises an illegal-instruction exception; with VS/VU + SSEED=1, read-write access raises a virtual-instruction exception
- `norm:mseccfg_sseed_SorHS-mode_op`: With SSEED=0, HS-mode access to seed raises an illegal-instruction exception; with SSEED=1, read-write access is allowed
- `norm:seed_ro_illegal`: read-only access raises an illegal-instruction exception in any mode

**Test Scope**: Verify the exception type distinction and access control of VS/VU-mode and HS-mode access to the seed CSR under the H extension.

### 1.1 HS-mode Access Control (SSEED)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| ZKR-HYP-01 | SSEED=1 HS-mode csrrw access to seed succeeds | Set mseccfg.SSEED=1 (HS-mode under the H extension), HS-mode executes csrrw rd, seed, x0 | returns the seed value normally |
| ZKR-HYP-02 | SSEED=0 HS-mode csrrw access to seed raises an exception | Set mseccfg.SSEED=0, HS-mode executes csrrw rd, seed, x0 | illegal-instruction exception (cause=2) |
| ZKR-HYP-13 | SSEED=1 HS-mode read-only access raises an exception | Set mseccfg.SSEED=1, HS-mode executes csrrs rd, seed, x0 | illegal-instruction exception (cause=2) |

### 1.2 VS/VU-mode Access Control (SSEED + H Extension)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| ZKR-HYP-03 | SSEED=0 VS-mode csrrw access to seed raises illegal | Set mseccfg.SSEED=0, VS-mode executes csrrw rd, seed, x0 | illegal-instruction exception (cause=2) |
| ZKR-HYP-04 | SSEED=1 VS-mode csrrw access to seed raises virtual-instruction | Set mseccfg.SSEED=1, VS-mode executes csrrw rd, seed, x0 | virtual-instruction exception (cause=22) |
| ZKR-HYP-05 | SSEED=0 VU-mode csrrw access to seed raises illegal | Set mseccfg.SSEED=0, VU-mode executes csrrw rd, seed, x0 | illegal-instruction exception (cause=2) |
| ZKR-HYP-06 | SSEED=1 VU-mode csrrw access to seed raises virtual-instruction | Set mseccfg.SSEED=1, VU-mode executes csrrw rd, seed, x0 | virtual-instruction exception (cause=22) |
| ZKR-HYP-07 | SSEED=1 VS-mode read-only access raises illegal (not virtual) | Set mseccfg.SSEED=1, VS-mode executes csrrs rd, seed, x0 | illegal-instruction exception (cause=2) (read-only access condition takes precedence) |
| ZKR-HYP-08 | SSEED=1 VS-mode csrrsi uimm=0 raises illegal | Set mseccfg.SSEED=1, VS-mode executes csrrsi rd, seed, 0 | illegal-instruction exception (cause=2) |
| ZKR-HYP-09 | SSEED=1 VS-mode csrrs(rs1≠x0) raises virtual-instruction | Set mseccfg.SSEED=1, VS-mode executes csrrs rd, seed, t0 (t0≠0) | virtual-instruction exception (cause=22) (HS-qualified read-write) |
| ZKR-HYP-10 | SSEED=0 VS-mode csrrs(rs1≠x0) raises illegal | Set mseccfg.SSEED=0, VS-mode executes csrrs rd, seed, t0 | illegal-instruction exception (cause=2) |
| ZKR-HYP-11 | SSEED does not affect M-mode (in VS/VU scenarios) | Set mseccfg.SSEED=0, M-mode csrrw seed | access succeeds |
| ZKR-HYP-14 | SSEED=1 VU-mode read-only access raises illegal (not virtual) | Set mseccfg.SSEED=1, VU-mode executes csrrs rd, seed, x0 | illegal-instruction exception (cause=2) (read-only access condition takes precedence) |
| ZKR-HYP-15 | SSEED=1 VU-mode csrrsi uimm=0 raises illegal | Set mseccfg.SSEED=1, VU-mode executes csrrsi rd, seed, 0 | illegal-instruction exception (cause=2) |
| ZKR-HYP-16 | SSEED=1 VU-mode csrrs(rs1≠x0) raises virtual-instruction | Set mseccfg.SSEED=1, VU-mode executes csrrs rd, seed, t0 (t0≠0) | virtual-instruction exception (cause=22) (HS-qualified read-write) |
| ZKR-HYP-17 | SSEED=0 VU-mode csrrs(rs1≠x0) raises illegal | Set mseccfg.SSEED=0, VU-mode executes csrrs rd, seed, t0 | illegal-instruction exception (cause=2) |
| ZKR-HYP-18 | SSEED=1 VS-mode csrrci uimm=0 raises illegal | Set mseccfg.SSEED=1, VS-mode executes csrrci rd, seed, 0 | illegal-instruction exception (cause=2) |

### 1.3 Exception Priority and Combined Scenarios

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| ZKR-HYP-12 | VU-mode read-only exception takes precedence over virtual-instruction | SSEED=1, VU-mode executes csrrs rd, seed, x0 | illegal-instruction exception (cause=2) (read-only condition → illegal, not virtual-instruction) |

> [!NOTE]
> - The tests in this group verify the behavior of the Zkr extension under Hypervisor scenarios. All tests must detect the availability of the H extension at runtime via `HAS_H_EXT()`; if unavailable, TEST_SKIP.
> - ZKR-HYP-03~18 are migrated from `Zkr_test_plan.md` and specifically target cases that depend on the H extension. Core semantics: when VS/VU-mode accesses seed, `mseccfg.SSEED` determines whether access is granted, and an HS-qualified read-write (SSEED=1) raises **virtual-instruction** (cause=22), while SSEED=0 or read-only access raises **illegal-instruction** (cause=2).
> - **Distinguishing virtual-instruction from illegal-instruction**: Test assertions must use accurate cause constants, distinguishing virtual-instruction (cause=22) with SSEED=1 from illegal-instruction (cause=2) for read-only access / SSEED=0.

---

## Group 2. Hypervisor × Zihintntl Cross Tests

**Spec Reference**:
- `norm:NTL_target_definition`: NTL instructions do not change architectural state, nor do they alter the architecturally visible effects of the target instruction; in virtualization environments, the architectural behavior of NTL prefix sequences must be identical to the non-virtualized case
- `norm:NTL_range`: NTL affects all memory-access instructions, and the H extension's HLV/HSV/HLVX virtual-machine memory access instructions are also within its scope

**Test Scope**: Verify that the behavior of NTL HINTs in virtualization environments (HS/VS/VU-mode) is consistent with non-virtualized scenarios: normal execution, no spurious virtual-instruction exception, and trap reporting information (cause/epc/tval/GVA/htval) identical to the case without the prefix. The cases in this group are migrated from `zihintntl_test_plan.md` (the virtualization portion of the original NTL-12, NTL-RG-07, NTL-CMO-08, NTL-TRAP-08).

### 2.1 NTL Execution in HS/VS/VU-mode

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| NTL-HYP-01 | HS-mode executes NTL + load | HS-mode executes ntl.all + ld, compare the destination register with memory | executes normally, no exception, result identical to the case without the prefix |
| NTL-HYP-02 | VS-mode executes NTL + load | Enter VS-mode and execute ntl.all + ld | executes normally, no virtual-instruction exception, result identical to the case without the prefix |
| NTL-HYP-03 | VU-mode executes NTL + load | Enter VU-mode and execute ntl.all + ld | executes normally, no exception, result identical to the case without the prefix |

### 2.2 NTL Applied to H-extension Virtual-Machine Memory Access Instructions

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| NTL-HYP-04 | NTL applied to HLV/HSV/HLVX | HS-mode executes HLV/HSV/HLVX with an ntl.all prefix, compared with the same sequence without the prefix | executes normally, memory-access effects correct, architectural behavior of both identical |

### 2.3 Trap Reporting of NTL + CMO in VS-mode

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| NTL-HYP-05 | ntl prefix does not change the virtual-instruction reporting of CMO instructions | If Zicbom is implemented: in VS-mode with henvcfg.CBIE=0 and CBCFE=0, execute "ntl.all + cbo.inval/cbo.clean", compared with the same sequence without the prefix | raises virtual-instruction exception (cause=22) just like without the prefix; cause/epc point to the CMO instruction, not the NTL |

### 2.4 G-stage Page Fault of NTL + target in VS-mode

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| NTL-HYP-06 | G-stage page fault of NTL + target in VS-mode | VS-mode executes "ntl.p1 + ld" triggering a G-stage guest-page-fault, compared with the same sequence without the prefix | delivered to HS-mode, load guest-page fault (cause=21), hstatus.GVA=1, stval/htval correct, behavior identical to the case without the prefix |

> [!NOTE]
> - All tests in this group must detect the availability of the H extension at runtime via `HAS_H_EXT()`; if unavailable, TEST_SKIP.
> - Zihintntl has no independent misa/CSR detection flag; it is enabled by platform configuration declaration. NTL instructions are injected into the execution stream with raw encoding (.word 0x00200033/0x00300033/0x00400033/0x00500033) to avoid toolchain alias interference.
> - NTL-HYP-05 additionally needs to probe Zicbom; NTL-HYP-06 needs to construct a mapping that is valid in VS-stage but invalid in G-stage. The core assertion strategy follows the "HINT no-side-effect comparison method" of `zihintntl_test_plan.md`: the architecturally visible behavior (including exception information) with and without the NTL prefix must be completely identical.

---

## Group 3. Hypervisor × Zcmt Cross Tests

**Spec Reference**:
- `norm:cm-jt_op` / `norm:cm-jalt_op`: table jumps are ordinary instructions with no privilege-level restrictions; they should execute normally in HS/VS/VU-mode
- `norm:jvt_base_vm`: when virtual memory is enabled, jvt.base is a virtual address; in VS-mode it undergoes two-stage translation via vsatp
- `norm:Zcmt_fetch` / `norm:Zcmt_trap`: the second fetch (JVT entry) is also translated; on a fault, xEPC points to the table jump instruction and xTVAL is the faulting fetch address
- `norm:stateen0_jvt_op`: the JVT bit of stateen0 controls jvt CSR access; it gates only CSR access, not instruction execution
- `norm:htval_trapval`: htval trap value reporting rules on G-stage faults

**Test Scope**: Verify the behavior of table jump instructions and the jvt CSR in virtualization environments: normal execution in HS/VS/VU-mode without spuriously raising a virtual-instruction exception; in VS-mode, JVT entry fetches undergo two-stage translation and G-stage faults are reported as guest instruction page fault; hstateen0.JVT gates VS/VU jvt access but not instruction execution. The cases in this group are migrated from `zcmt_test_plan.md` (the virtualization portion of the original ZCMT-27/28, ZACC-03/04/05, and the virtualization portion of ZACC-06; HZCMT-08 is a VS-stage fault case supplemented as a counterpart to ZCMT-25).

### 3.1 Table Jump Execution in HS/VS/VU-mode

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HZCMT-01 | HS-mode executes table jumps | HS-mode executes cm.jt and cm.jalt (jvt points to a valid table) | both jump and link normally, no exception |
| HZCMT-02 | VS-mode executes table jumps | VS-mode executes cm.jt and cm.jalt | both execute normally, no virtual-instruction exception |
| HZCMT-03 | VU-mode executes table jumps | VU-mode executes cm.jt and cm.jalt | both execute normally, no exception |

### 3.2 jvt Access and stateen Gating in VS/VU-mode

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HZCMT-04 | VS/VU-mode accesses jvt | With stateen enabled, VS/VU-mode csrr/csrw jvt | access succeeds (jvt permission URW + stateen enabled) |
| HZCMT-05 | hstateen0.JVT gates VS/VU access | When Smstateen is implemented, zero out the JVT bits of hstateen0/sstateen0 per hierarchy, VS/VU accesses jvt | VS/VU raises virtual-instruction/illegal-instruction (see `Smstateen_test_plan.md` / `Ssstateen_test_plan.md` for detailed cases) |
| HZCMT-06 | stateen does not gate table jump instruction execution | When Smstateen is implemented, zero out the JVT bits of stateen at all levels, VS-mode executes cm.jt/cm.jalt | instructions execute normally (state enable gates only jvt CSR access, not the instruction itself) |

### 3.3 Table Jumps under Two-Stage Translation in VS-mode

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HZCMT-07 | VS-mode translation path jumps normally | VS-mode with vsatp enabled, VS-stage maps the table page with X=1, execute cm.jt | jump succeeds (VS-stage translation effective) |
| HZCMT-08 | VS-mode table page with VS-stage X=0 raises a fault | The table page is mapped in VS-stage but X=0, VS-mode executes cm.jt | instruction page fault delivered per the delegation path, sepc=cm.jt PC, stval=table entry virtual address |
| HZCMT-09 | VS-mode second fetch G-stage fault | The G-stage mapping of the table page is invalid, VS-mode executes cm.jt | guest instruction page fault (cause=20) delivered to HS-mode, hstatus.GVA=1, htval=faulting table entry GPA>>2 (norm:htval_trapval allows zero) |

> [!NOTE]
> - All tests in this group must detect the availability of the H extension at runtime via `HAS_H_EXT()`; if unavailable, TEST_SKIP. Zcmt support follows the platform configuration macro `ZCMT_SUPPORTED`; the jvt writability probe result (read-only implementation, allowed by `norm:jvt_op`) determines whether functional cases are applicable.
> - HZCMT-05 additionally needs to probe Smstateen; HZCMT-07~09 need to enable vsatp (and hgatp) to construct two-stage translation, using a mapping combination of VS-stage valid + G-stage invalid to isolate G-stage faults.
> - HZCMT-08/09 verify the fault path of the second fetch (JVT entry): vsepc must point to the cm.jt instruction itself rather than the table address (`norm:Zcmt_trap`), and vstval/htval report the table entry fetch address.

---

## Group 4. Hypervisor × Zawrs Cross Tests

**Spec Reference**:
- `norm:Zawrs_virtual_instr_excp`: in VS/VU-mode with `hstatus.VTW`=1, `mstatus.TW`=0, and `wrs.nto` not completing within the implementation-defined time limit → virtual-instruction exception
- `norm:Zawrs_exec_resume_rules`: wrs instructions follow the locally-enabled-interrupt resume rules of `wfi`; when a locally enabled pending interrupt exists, no stall occurs and the VTW exception path is not triggered (cross-referenced with the `norm:hstatus_vtw_op`/`norm:vtw_virtinstr` WFI semantics in `Hypervisor_CSR_test_plan.md`; `norm:vtw_virtinstr` also permits the implementation to always raise virtual-instruction when VTW=1, even if a globally masked pending interrupt exists, so HZWRS-06 is a recording-type case)
- `zawrs.adoc` (`norm:Zawrs_priv_illegal_instr_excp`): with `mstatus.TW`=1, non-M-mode execution of `wrs.nto` that does not complete raises illegal-instruction — the TW clause takes precedence over VTW, and VS/VU-mode also report illegal (consistent with the WFI semantics of HSTAT-06)
- `zawrs.adoc`: the VTW clause names only `wrs.nto`; `wrs.sto` is bounded by its short timeout and is not gated by VTW; wrs instructions are available in all privilege modes, and HS-mode is not constrained by VTW (VTW applies only when V=1)

**Test Scope**: Verify the behavior of Zawrs instructions in virtualization environments: normal execution in HS/VS/VU-mode without spuriously raising a virtual-instruction exception; `hstatus.VTW` virtual-instruction gating of `wrs.nto` in VS/VU-mode; the precedence and exception type distinction between `mstatus.TW` and `hstatus.VTW`; the instruction scope of the VTW clause (only `wrs.nto`). Instructions are injected with raw encoding: `wrs.nto` = 0x00D00073 (SYSTEM opcode=0x73, funct3=0, rd=0, funct12=0x0d), `wrs.sto` = 0x01D00073 (funct12=0x1d).

### 4.1 Normal Execution of wrs Instructions in HS/VS/VU-mode (VTW=0)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HZWRS-01 | HS-mode executes wrs.nto/wrs.sto | With `hstatus.VTW`=0, HS-mode establishes a reservation set with `lr` and pre-stages a locally enabled pending software interrupt, then executes `wrs.nto` and `wrs.sto` in sequence (raw encoding) | both complete normally, no exception |
| HZWRS-02 | VS-mode executes wrs.nto/wrs.sto | With `hstatus.VTW`=0 and `mstatus.TW`=0, VS-mode executes both instructions in the same scenario | both complete normally without spuriously raising a virtual-instruction exception |
| HZWRS-03 | VU-mode executes wrs.nto/wrs.sto | Same configuration, VU-mode executes both instructions | both complete normally, no exception |

### 4.2 hstatus.VTW Gating (VS/VU-mode wrs.nto)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HZWRS-04 | VS-mode VTW=1 wrs.nto raises virtual-instruction | With `hstatus.VTW`=1 and `mstatus.TW`=0, mask all locally enabled interrupts, VS-mode executes `wrs.nto` (trap-armed) | virtual-instruction exception (cause=22) |
| HZWRS-05 | VU-mode VTW=1 wrs.nto raises virtual-instruction | Same configuration, VU-mode executes `wrs.nto` | virtual-instruction exception (cause=22) |
| HZWRS-06 | VTW=1 but a locally enabled interrupt is already pending (recording type) | With `hstatus.VTW`=1, after setting and locally enabling an interrupt, VS-mode executes `wrs.nto` | both behaviors are legal: the instruction completes immediately (no stall, `norm:Zawrs_exec_resume_rules`); or reports virtual-instruction (cause=22) per the VTW interception permission of `norm:vtw_virtinstr`. Record the implementation choice; if an exception is raised it must be cause=22, with no mandatory verdict |
| HZWRS-07 | Trap reporting of the VTW exception | Following the HZWRS-04 scenario, check the HS-mode trap context | cause=22, stval written per the `norm:H_virtinst_xtval` rule (instruction encoding or 0), hstatus.SPV=1, normal recovery after the handler skips |

### 4.3 TW Precedence and Instruction Scope (VS/VU-mode)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HZWRS-08 | VS-mode reports illegal when VTW=1 and TW=1 | With `hstatus.VTW`=1 and `mstatus.TW`=1, VS-mode executes `wrs.nto` | illegal-instruction exception (cause=2) (TW clause takes precedence, `norm:Zawrs_priv_illegal_instr_excp`) |
| HZWRS-09 | VU-mode reports illegal when VTW=1 and TW=1 | Same configuration, VU-mode executes `wrs.nto` | illegal-instruction exception (cause=2) |
| HZWRS-10 | VTW=1 applies only to wrs.nto | With `hstatus.VTW`=1 and `mstatus.TW`=0, VS-mode executes `wrs.sto` | completes normally after the short timeout, no exception (the VTW clause names only `wrs.nto`) |
| HZWRS-11 | VTW does not affect HS-mode | With `hstatus.VTW`=1, HS-mode executes `wrs.nto` | completes normally, no exception (VTW applies only when V=1) |
| HZWRS-12 | TW=1 alone takes effect in VS-mode | With `mstatus.TW`=1 and `hstatus.VTW`=0, VS-mode executes `wrs.nto` | illegal-instruction exception (cause=2) (cross-referenced with the WFI semantics of HSTAT-06 in `Hypervisor_CSR_test_plan.md`) |

> [!NOTE]
> - All tests in this group must detect the H extension at runtime via `HAS_H_EXT()` and probe Zawrs support with trap-armed raw encoding (if unimplemented, the whole group TEST_SKIP); Zawrs depends on Zalrsc, and cases must first establish a reservation set with `lr`.
> - As with the WFI/VTW cases (HSTAT-04), there is a timing dependency: the VTW exception requires that `wrs.nto` "does not complete within the implementation-defined time limit". If the implementation terminates the stall prematurely with a very short duration per `norm:Zawrs_stall_terminate` so that the exception is not raised, the case must remain failed and be recorded in the `bugs/` directory; assertions must not be relaxed.
> - HZWRS-06 is a recording-type case: `norm:vtw_virtinstr` permits the implementation to always raise virtual-instruction when VTW=1 (even if a globally masked pending interrupt exists), so both "completes immediately" and "reports cause=22" are legal implementations; the case only records the implementation choice and constrains the exception type (if raised it must be cause=22), while avoiding any dependency on the implementation's stall duration.
> - HZWRS-08/09 verify the exception type determination: with `mstatus.TW`=1, illegal-instruction (cause=2) must be reported per `norm:Zawrs_priv_illegal_instr_excp`; virtual-instruction must not be reported. Assertions must use precise cause constants.
> - Zawrs non-Hypervisor scenarios (encoding and availability, stall and resume, basic TW behavior) are covered by `Zawrs_test_plan.md`.

---

## Group 5. Hypervisor × V Vector Family Cross Tests

**Spec Reference**:
- `norm:vsstatus_vs_sz_acc`: when the H extension is implemented, vsstatus gains the vector context status field vs (bits[10:9]), defined analogously to fs
- `norm:vsstatus_vs_mstatus_vs_op_off`: when V=1, both vsstatus.vs and mstatus.vs are in effect; if either is Off, executing any vector instruction or accessing vector CSRs → illegal-instruction
- `norm:vsstatus_vs_mstatus_vs_op_active`: when V=1 and neither is Off, any instruction that changes vector state sets both to Dirty
- `norm:hw_mstatus_vs_dirty_update`: the implementation is permitted to promote Initial/Clean to Dirty at any time (permissive behavior; related cases are recording type)
- `norm:vsstatus_sd_op_vs`: vsstatus.vs=Dirty → vsstatus.sd=1
- `norm:vsstatus_vs_exists`: for implementations with a writable misa.v, vsstatus.vs may exist when misa.v=0 (conditional case)
- `norm:vsstatus_mstatus_FS_off_hypervisor_V_fp_ill` / `norm:vsstatus_mstatus_FS_dirty_hypervisor_V_fp`: vsstatus.fs gating and dual Dirty updates for vector floating-point instructions when V=1
- Division of labor with `Hypervisor_CSR_test_plan.md`: `norm:vsstatus_vs_op`/`norm:vsstatus_fs_op` in `hypervisor.adoc` (equivalent statements) are already covered by VSST-03~09 of that plan; this group only covers the specification points specific to `vector-common.adoc` (field presence, vector CSR access gating, SD linkage, FP-side dual Dirty updates) and does not re-verify equivalent content. This group applies to all vector extensions sharing `vector-common.adoc` (V, Zve32x/f, Zve64x/f/d, Zv*)

**Test Scope**: Verify the behavior of the VS-level copies of vector context status (vsstatus.vs) and vector floating-point status (vsstatus.fs) in V=1 scenarios: field presence and read/write, Off gating (instructions and vector CSRs), dual Dirty updates, SD linkage, and vector floating-point gating. Vector/vector floating-point instructions are injected with raw encoding (.word), avoiding a build march dependency on the v extension.

### 5.1 vsstatus.vs Field and Vector Instruction/CSR Gating (VS/VU)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HVEC-01 | vsstatus.vs field read/write | With the H extension implemented, HS-mode writes vsstatus.vs=0b11/0b01/0b00 and reads back | the field exists and is writable (bits[10:9]), read-back is consistent |
| HVEC-02 | vsstatus.vs=Off gates vector instructions | With `mstatus.vs`≠Off, set `vsstatus.vs`=Off, VS-mode executes a vector instruction (raw encoding) | illegal-instruction exception (cause=2) |
| HVEC-03 | mstatus.vs=Off gates vector instructions | With `vsstatus.vs`≠Off, set `mstatus.vs`=Off, VS-mode executes a vector instruction | illegal-instruction exception (cause=2) |
| HVEC-04 | Off gating of vector CSR access | Set `vsstatus.vs`=Off, VS-mode accesses vector CSRs (vstart/vl/vtype/vcsr) | illegal-instruction exception (cause=2) (Off gating covers vector CSR access) |
| HVEC-05 | VS/VU normal execution when neither is Off | With `vsstatus.vs` and `mstatus.vs` both Initial/Dirty, VS-mode and VU-mode each execute a vector instruction | both execute normally, no exception |
| HVEC-06 | Modifying vector state sets both to Dirty | Set both to Initial (0b01), VS-mode executes an instruction that changes vector state (including vector CSR writes) | `mstatus.vs`=3 (Dirty) and `vsstatus.vs`=3 (Dirty) |
| HVEC-07 | Linkage between vsstatus.sd and vs | HS-mode writes `vsstatus.vs`=Dirty and reads `vsstatus.sd`; then writes `vsstatus.vs`=Initial (other context fields not Dirty) and reads again | vs=Dirty → sd=1; vs=Initial → sd=0 (`norm:vsstatus_sd_op_vs`) |
| HVEC-08 | (Recording type) the implementation may promote Clean to Dirty at any time | Set both to Clean (0b02), VS-mode executes a vector instruction then reads both fields back | staying Clean or being promoted to Dirty are both legal (`norm:hw_mstatus_vs_dirty_update`); record the implementation behavior with no mandatory verdict |

### 5.2 Vector Floating-Point Gating (vsstatus.fs, VS/VU)

**Precondition**: F extension (`misa.f`) and vector floating-point instruction support (trap-armed raw encoding probe); if not satisfied, the whole subsection TEST_SKIP.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HVEC-09 | vsstatus.fs=Off gates vector floating-point instructions | With `mstatus.fs`≠Off, set `vsstatus.fs`=Off, VS-mode executes a vector floating-point instruction (raw encoding) | illegal-instruction exception (cause=2) |
| HVEC-10 | mstatus.fs=Off gates vector floating-point instructions | With `vsstatus.fs`≠Off, set `mstatus.fs`=Off, VS-mode executes a vector floating-point instruction | illegal-instruction exception (cause=2) |
| HVEC-11 | VU-mode vector floating-point gating | With `vsstatus.fs`=Off, VU-mode executes a vector floating-point instruction | illegal-instruction exception (cause=2) |
| HVEC-12 | Modifying floating-point state sets both to Dirty | Set both to Initial, VS-mode executes a vector floating-point instruction that modifies floating-point state | `mstatus.fs`=3 (Dirty) and `vsstatus.fs`=3 (Dirty) |

### 5.3 Conditional Cases (misa.v writable)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HVEC-13 | (Conditional) vsstatus.vs presence with misa.v=0 | Probe `misa.v` writability: if not writable, TEST_SKIP; if writable, write `misa.v`=0 then trap-armed read/write of `vsstatus.vs` | the field "may exist" or may not (`norm:vsstatus_vs_exists`); both successful read/write and illegal-instruction are legal implementations; record the implementation behavior |

> [!NOTE]
> - Extension detection: the V extension is determined by `misa.v`; vector floating point additionally requires `misa.f` and trap-armed probing of vector floating-point instructions; if not satisfied, the corresponding subsection/cases TEST_SKIP.
> - Vector/vector floating-point instructions are injected with raw encoding (.word) (e.g., fixed encodings of vsetivli/vadd.vv/vfadd.vv), avoiding toolchain dependencies.
> - The exception type of Off gating is fixed as illegal-instruction (cause=2): this is extension context status gating, not the virtual-instruction (cause=22) of Hypervisor controlled access; assertions must not confuse the two.
> - HVEC-08/HVEC-13 are recording-type cases: `norm:hw_mstatus_vs_dirty_update`/`norm:vsstatus_vs_exists` are both implementation-permissive behaviors, with no mandatory verdict.
> - Interrupt environment: VS/VU cases execute in a zero-pending-interrupt environment (same principle as Group 4, avoiding interrupt delivery polluting trap records).
> - Non-virtualized scenarios of the V vector family (vtype/vl, basic semantics) have no standalone test plan and are out of scope for this group.

---

## Group 6. Hypervisor × Zicntr Cross Tests

**Intersection points with the Hypervisor**:
1. **`time` read offset**: when V=1, VS/VU-mode reads of `time` (including the `rdtime` instruction) return `time + htimedelta` (`norm:htimedelta_sz_acc_op`); CSR-level semantics are already covered by HTDLT-01~05 of `Hypervisor_CSR_test_plan.md`, and this group supplements the instruction-level (raw encoding) path
2. **htimedelta implementation requirement**: if the `time` CSR is implemented, `htimedelta` must be implemented (`norm:time_htimedelta_req`, implicitly verified by successful access)
3. **hcounteren CY/TM/IR gating**: when the corresponding bit is clear and the same bit of `mcounteren` is 1, reading `cycle`/`time`/`instret` while V=1 raises virtual-instruction (`norm:hcounteren_op`); the gating matrix is covered by `Shcounterenw_test_plan.md`, and this group only uses it as a precondition and verifies the exception type distinction (HZCNT-05/06)
4. **hcounteren.TM gating of vstimecmp**: `norm:hcounteren_acc`, covered by HCROSS-SSTC-05 of `Hypervisor_Ss_test_plan.md`, not duplicated in this group; vstimecmp/VSTIP synthesis (`(time + htimedelta) >= vstimecmp`) is covered by `Hypervisor_CSR_test_plan.md` Group 7
5. **Continued control of VU-mode by scounteren**: `scounteren` has no corresponding VS CSR (`norm:H_scsrs_nomatch`) and continues to control VU-mode visibility of `cycle`/`time`/`instret` when V=1; VCSR-17 of `Hypervisor_CSR_test_plan.md` only covers the `hcounteren`=1 branch, and this group supplements the blocked branch with `hcounteren`=0 and the reverse verification that `scounteren` does not affect VS-mode (HZCNT-07~09)

**Spec Reference**:
- `norm:zicntr_rdtime_op`: `rdtime` reads the low XLEN bits of the `time` CSR; when V=1 the read-back value is `time + htimedelta`, and HS-mode is not affected by the delta
- `H_scsrs_nomatch_vu_counter`: `scounteren` has no corresponding VS CSR and continues to take effect when V=1 with VS substituting for HS and VU substituting for U, controlling VU-mode counter visibility; VU-mode counter access exceptions are delivered to HS-mode (`norm:htval_trapval` does not apply)
- `hcounteren_gate_v1_counter`: `hcounteren`/`mcounteren` gating serves as the precondition for the cases in this group; the gating matrix itself is covered by `Shcounterenw_test_plan.md` and not re-verified (exception: HZCNT-06 verifies that with `mcounteren`=0 the exception type is illegal rather than virtual-instruction, which belongs to exception type distinction rather than the gating matrix)

**Test Scope**: Verify the instruction-level time offset semantics of `rdtime` and the continued control of VU-mode by `scounteren` (`cycle`/`time`/`instret`) when V=1. Counter instructions are injected with raw encoding (`rdcycle`=0xC0002xx3, `rdtime`=0xC0102xx3, `rdinstret`=0xC0202xx3, funct3=2 SYSTEM/csrrs rd, csr, x0), avoiding toolchain alias interference.

### 6.1 Instruction-Level htimedelta Semantics of rdtime (VS/VU)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HZCNT-01 | VS-mode rdtime returns time+delta | Set `htimedelta`=N (N is a non-zero identifiable value), VS-mode executes `rdtime` (raw encoding), then after returning HS-mode reads the real `time` | the `rdtime` return value ≈ real `time` + N (allowing for clock progression error); consistent with the `csrr time` path (HTDLT-02) |
| HZCNT-02 | VU-mode rdtime returns time+delta | Same configuration, VU-mode executes `rdtime` | return value ≈ real `time` + N |
| HZCNT-03 | HS-mode rdtime contains no delta | With `htimedelta`=N, HS-mode executes `rdtime` | returns the real `time` without the offset (control) |
| HZCNT-04 | Negative offset rdtime | Set `htimedelta` to a negative value (e.g., 0xFFFFFFFFFFFF0000), VS-mode executes `rdtime` | the return value is less than the real `time` (unsigned comparison, corresponding to truncation semantics) |
| HZCNT-05 | VS-mode rdtime raises virtual-instruction with hcounteren.TM=0 | With `mcounteren.TM`=1 and `hcounteren.TM`=0, VS-mode executes `rdtime` (trap-armed) | virtual-instruction exception (cause=22) (`hcounteren_gate_v1_counter`) |
| HZCNT-06 | VS-mode rdtime reports illegal with mcounteren.TM=0 | With `mcounteren.TM`=0 and `hcounteren.TM`=0, VS-mode executes `rdtime` | illegal-instruction exception (cause=2) (`mcounteren` level precondition; must not report cause=22) |

### 6.2 Continued Control of VU-mode by scounteren when V=1 (cycle)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HZCNT-07 | scounteren controls VU counters with hcounteren=1 (CY bit) | With `mcounteren.CY`=1 and `hcounteren.CY`=1, first set `scounteren.CY`=1 and verify VU read of `cycle` succeeds, then set `scounteren.CY`=0 and read again | no exception when set to 1; exception raised when set to 0 (`H_scsrs_nomatch_vu_counter`, cross-referenced with VCSR-17 of `Hypervisor_CSR_test_plan.md`) |
| HZCNT-08 | Blocking takes effect on VU with hcounteren=0 | With `mcounteren.CY`=1, `hcounteren.CY`=0, and `scounteren.CY`=1, VU-mode reads `cycle` | exception raised (the `hcounteren` level shutdown blocks first; verifies the branch not covered by VCSR-17) |
| HZCNT-09 | scounteren does not affect VS-mode | With `scounteren.CY`=0, `hcounteren.CY`=1, and `mcounteren.CY`=1, VS-mode reads `cycle` | reads normally (`scounteren` constrains only VU; VS-mode is not affected by it) |

> [!NOTE]
> - All tests in this group must detect the H extension at runtime via `HAS_H_EXT()`; Zicntr support follows trap-armed probing of the `cycle`/`time` CSRs, and if not satisfied the whole group TEST_SKIP.
> - Counter accesses such as `rdtime`/`rdcycle` are injected with raw encoding (csrrs rd, csr, x0 form), avoiding toolchain pseudo-instruction expansion differences; the read-only access form (rs1=x0) is legal for counters.
> - The clock comparison of HZCNT-01/02 must tolerate natural progression of the real `time` within the test window; assertions use a difference interval (e.g., `N <= rdtime - time <= N + bound`) and must not require exact equality.
> - HZCNT-05/06 verify the exception type distinction: gating exceptions must be virtual-instruction (cause=22), and `mcounteren` level exceptions are illegal-instruction (cause=2); assertions use precise cause constants.
> - RV32 high-half access (`rdtimeh`/`htimedeltah`, `norm:zicntr_rdtimeh_op`) is a conditional specification point, not triggered on RV64 platforms; no cases are defined in this document.
> - The gating matrix (per-bit write-back + VS/VU combinations) is already covered by `Shcounterenw_test_plan.md`; this group only references gating as a precondition and does not re-verify it.

---

## Group 7. Hypervisor × Zihpm Cross Tests

**Intersection points with the Hypervisor**:
1. **hcounteren HPMn gating**: when bit N of `hcounteren` is clear and the same bit of `mcounteren` is 1, reading `hpmcounterN` while V=1 raises virtual-instruction (`norm:hcounteren_op`); the gating matrix is covered by `Shcounterenw_test_plan.md`, and this group only uses it as a precondition (exception: HZHPM-03 verifies that with gating closed the exception is triggered by gating, regardless of whether the counter is implemented)
2. **Behavior of unimplemented counters when V=1**: accessing an unimplemented `hpmcounter` may raise an exception or return a constant value (`norm:hpm_unimplemented_counter_access`, both legal); previously no test plan covered this, and it is newly added in this group (HZHPM-01/02/04)
3. **VU-mode three-level gating chain**: VU-mode access to `hpmcounter` requires `mcounteren[N]`/`hcounteren[N]`/`scounteren[N]` all to be 1, where the `scounteren` level constraint comes from `norm:H_scsrs_nomatch` (no corresponding VS CSR, still in effect when V=1); previously only VCSR-17 of `Hypervisor_CSR_test_plan.md` covered the `hcounteren`=1 branch of `cycle`, and the `hpmcounter` chain was uncovered (HZHPM-05)

**Spec Reference**:
- `norm:hpm_unimplemented_counter_access`: access to an unimplemented `hpmcounter` may raise an illegal-instruction exception or return a constant value (both implementations are legal); when V=1 this behavior is still gated by `hcounteren` (and `scounteren` in VU-mode), and when gating is open no virtual-instruction may be forcibly reported for "the unimplemented counter itself"
- `hcounteren_gate_v1_counter`: HPMn bit gating serves as the precondition for the cases in this group; the gating matrix itself is covered by `Shcounterenw_test_plan.md` and not re-verified (exception: HZHPM-03 verifies that with gating closed the exception is triggered by gating, regardless of whether the counter is implemented; when `mcounteren[N]` itself is read-only zero, the `mcounteren` level precondition applies and V=1 access reports illegal-instruction (cause=2) rather than virtual-instruction; HZHPM-01~03 all verify this branch)
- `H_scsrs_nomatch_vu_counter`: VU-mode access to `hpmcounter` is additionally constrained by `scounteren` (no corresponding VS CSR, still in effect when V=1)
- Division of labor with existing plans: bit-level writability of `hcounteren` and the write-back/gating matrix of implemented counters are covered by `Shcounterenw_test_plan.md` and `Hypervisor_Ss_test_plan.md` Group 3, not duplicated in this group.

**Test Scope**: Verify the legal behavior space of unimplemented `hpmcounter` when V=1 and the VU three-level gating chain of `hpmcounter`. `hpmcounterN` access is injected with raw encoding (CSR 0xC00+N, csrrs rd, csr, x0 form), avoiding toolchain alias interference.

### 7.1 Behavior of Unimplemented hpmcounter when V=1

**Precondition**: probe by writing a non-zero value to `mhpmcounterN` in M-mode and reading it back (following the probing strategy of `Shcounterenw_test_plan.md`), selecting one unimplemented (read-only zero) `hpmcounterN` (N∈3..31); if the platform implements all of them, the whole subsection TEST_SKIP.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HZHPM-01 | VS-mode access to an unimplemented counter (attempt to open gating) | Attempt to set `mcounteren[N]`/`hcounteren[N]`=1 and read back to probe: when both take effect (gating open), VS-mode executes `csrr hpmcounterN` (raw encoding, trap-armed); when `mcounteren[N]` is read-only zero, verify per the `mcounteren` level precondition path | gating open: both behaviors are legal — returning a constant value (including 0) or raising illegal-instruction (cause=2); virtual-instruction must not be reported; `mcounteren[N]` read-only zero: an exception must be raised and it must be illegal-instruction (cause=2). Record the implementation choice; both comply with `norm:hpm_unimplemented_counter_access` and the `mcounteren` precondition rule |
| HZHPM-02 | VU-mode access to an unimplemented counter (attempt to open three-level gating) | Attempt to set `mcounteren[N]`/`hcounteren[N]`/`scounteren[N]`=1 and read back to probe, VU-mode executes `csrr hpmcounterN` | all three levels effective: constant value or illegal-instruction (cause=2) are both legal; `mcounteren[N]` read-only zero: must report illegal-instruction (cause=2); only `hcounteren[N]`/`scounteren[N]` read-only zero: must report virtual-instruction (cause=22). Record the implementation behavior |
| HZHPM-03 | hcounteren[N]=0 blocks an unimplemented counter | Attempt to set `mcounteren[N]`=1; clear `hcounteren[N]`, VS-mode executes `csrr hpmcounterN` | with gating closed the exception is triggered by gating, regardless of whether the counter is implemented: with `mcounteren[N]`=1, virtual-instruction (cause=22); with `mcounteren[N]` read-only zero, illegal-instruction (cause=2) reported by the `mcounteren` level precondition |
| HZHPM-04 | Recording type: read-back value consistency | Repeat the access of HZHPM-01 several times | if the implementation returns a constant value, multiple read-backs are consistent; if the implementation reports an exception, the exception cause is consistent each time. Record the implementation behavior with no mandatory verdict |

### 7.2 VU Three-Level Gating Chain of hpmcounter (Implemented Counters)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HZHPM-05 | hpmcounter VU three-level gating | Select an implemented `hpmcounterN`, set all three levels `mcounteren[N]`/`hcounteren[N]`/`scounteren[N]` to 1 then read in VU; then clear only `scounteren[N]` and read again | no exception when all are 1; exception raised after clearing `scounteren[N]` (hierarchical chain `mcounteren → hcounteren → scounteren`; the `scounteren` constraint on VU comes from `H_scsrs_nomatch_vu_counter`) |

> [!NOTE]
> - All tests in this group must detect the H extension at runtime via `HAS_H_EXT()`; the implemented/unimplemented status of `hpmcounterN` is probed by writing a non-zero value to `mhpmcounterN` in M-mode and reading back (following the strategy of `Shcounterenw_test_plan.md`); if not satisfied, the corresponding subsection/cases TEST_SKIP.
> - `hpmcounterN` access is injected with raw encoding (csrrs rd, csr, x0 form), avoiding toolchain pseudo-instruction expansion differences.
> - HZHPM-01/02/04 are recording-type cases: `norm:hpm_unimplemented_counter_access` permits both constant value and exception implementations, and neither result may be judged a failure; however, with gating open no virtual-instruction may be reported (mandatory assertion), and the exception type with gating closed is a mandatory assertion (HZHPM-03: `mcounteren[N]`=1 → cause=22; `mcounteren[N]` read-only zero → cause=2 reported by the `mcounteren` level precondition).
> - The gating matrix (per-bit write-back + VS/VU combinations) is already covered by `Shcounterenw_test_plan.md`; this group only references gating as a precondition and does not re-verify it.
> - RV32 high-half access (`hpmcounter3h`–`hpmcounter31h`, `norm:hpm_counter_op_sz_mode_xlen32`) is a conditional specification point, not triggered on RV64 platforms; no cases are defined in this document.

---

## Key Considerations

1. **Extension detection**: All tests must detect the availability of the required extensions (H, Zkr, Zihintntl, Zcmt, etc.) at runtime; if unavailable, TEST_SKIP. Zkr is detected by probing the presence of the `seed` CSR (0x015); Zihintntl has no independent probe flag and is enabled by platform configuration declaration; Zcmt follows the platform configuration macro `ZCMT_SUPPORTED` and trap-armed probing of the jvt CSR (0x017).

2. **SSEED control**: `mseccfg.SSEED` controls S/HS/VS/VU-mode access to the seed CSR. M-mode access is not affected by SSEED (ZKR-HYP-11).

3. **Read-only access precedence**: read-only CSR-access instructions (csrrs/csrrc with rs1=x0, or csrrsi/csrrci with uimm=0) accessing seed raise illegal-instruction (cause=2) in any mode; this condition takes precedence over the virtual-instruction determination.

4. **Distinguishing virtual-instruction from illegal-instruction**: When VS/VU-mode accesses a controlled CSR, HS-qualified read-write with SSEED=1 raises virtual-instruction (cause=22); SSEED=0 or read-only access raises illegal-instruction (cause=2).

5. **Zawrs timing dependency**: The `wrs.nto` cases of Group 4 depend on the timing condition of "does not complete within the implementation-defined time limit" (same style as the WFI/VTW case HSTAT-04 of `Hypervisor_CSR_test_plan.md`). If the implementation terminates the stall prematurely per `norm:Zawrs_stall_terminate`, the case must remain failed and be recorded in `bugs/`; assertions must not be relaxed.

6. **Vector instruction injection and context gating**: The vector/vector floating-point instructions of Group 5 are injected with raw encoding; Off gating of `vsstatus.vs`/`vsstatus.fs` reports illegal-instruction (cause=2), not virtual-instruction; the implementation is permitted to promote Initial/Clean to Dirty at any time (`norm:hw_mstatus_vs_dirty_update`), and the related cases are recording type — they must not be judged failed on the grounds that "state was promoted".

7. **Group 5 implementation and platform verification status** (implementation: `Hypervisor_Vector/` suite, 13 cases): platform V/F support follows the `V_SUPPORTED`/`F_SUPPORTED` macros of `config/<platform>/rvtest_config.h` (no runtime probing). Spike (`rv64imafdcvh_zicsr_zifencei`) passes 13/13; QEMU (qemu-rv64-max, `-cpu max`) shows 10 PASS / 2 FAIL / 1 SKIP: HVEC-07 (`vsstatus.sd` is not set to 1 when `vsstatus.vs`=Dirty, `norm:vsstatus_sd_op_vs`) and HVEC-12 (a vector floating-point instruction that modifies floating-point state does not set both `fs` fields to Dirty, `norm:vsstatus_mstatus_FS_dirty_hypervisor_V_fp`) are QEMU implementation defects (Spike passes as a reference), archived in `bugs/qemu_hypervisor_vector_bugs.md`, and the cases remain failed; HVEC-13 is conditionally SKIPped because QEMU `misa.v` is not writable (actually executed and passed on Spike).

8. **Group 6/7 counter intersection key points**: `rdtime`/counter accesses are injected with raw encoding; clock comparisons use a difference interval rather than exact equality; both the constant-value and exception behaviors of unimplemented `hpmcounter` are legal implementations (recording-type cases — neither relaxed nor misjudged); the cause distinction of gating exceptions (cause=22 vs cause=2) is a mandatory assertion; the gating matrix itself is not re-verified (covered by `Shcounterenw_test_plan.md`). Note: the assertion of negative-offset `rdtime` must use a signed difference rather than an unsigned comparison — early after boot, when the real `time` is smaller than the offset magnitude, truncation wrap-around makes an unsigned comparison invalid (implementation key point of HZCNT-04).

9. **Group 6/7 implementation and platform verification status** (implementation: `Hypervisor_Zicntr/` suite with 9 cases, `Hypervisor_Zihpm/` suite with 5 cases): QEMU (qemu-rv64-max, `-cpu max`): Zicntr 9/9 PASS; Zihpm 4 PASS / 1 SKIP (QEMU implements `hpmcounter3-18`, `mhpmcounter19-31` are read-only zero; the probe selected `hpmcounter19` whose `mcounteren[19]` is read-only zero, HZHPM-01~03 verify per the `mcounteren` level precondition path and report cause=2; HZHPM-04 is compliantly SKIPped because gating cannot be opened; HZHPM-05 verifies the three-level gating chain on the implemented `hpmcounter3` and passes). Spike (`rv64imach_zicsr_zifencei_zicntr`/`_zihpm`): Zicntr 9/9 PASS; Zihpm 4 PASS / 1 SKIP (Spike's `mhpmcounter3-31` mirrors are read-only zero, permitted by `norm:mhpmcounter_mhpmevent_rdonly0` and treated as unimplemented per the convention of this plan; HZHPM-01~04 execute in the gating-open branch, and both reading back the constant value 0 and reporting cause=22 with gating closed are legal paths; HZHPM-05 is compliantly SKIPped because no implemented counter exists). The two platforms cover the `mcounteren` precondition branch and the gating-open constant-value branch respectively, 0 FAIL. The implemented/unimplemented counter probe is M-mode writing a non-zero value to `mhpmcounterN` and reading it back (not read-back probing of `mcounteren` bits: the latter only tests gating bit writability and, per `norm:mcounteren_flds_rdonly0`, is not equivalent to counter existence).

---

## References

- `SPEC/hypervisor.adoc` — RISC-V Hypervisor Extension, Version 1.0
- `SPEC/riscv-isa-manual/src/unpriv/zk.adoc` — Zkr Entropy Source Extension
- `SPEC/riscv-isa-manual/src/unpriv/zihintntl.adoc` — Zihintntl Extension for Non-Temporal Locality Hints
- `SPEC/riscv-isa-manual/src/unpriv/zcmt.adoc` — Zcmt Extension for Compressed Table Jumps
- `SPEC/riscv-isa-manual/src/unpriv/vector-common.adoc` — V Vector Extension common definitions (vector context status and Hypervisor interaction)
- `SPEC/riscv-isa-manual/src/unpriv/zicntr.adoc` — Zicntr Extension for Base Counters and Timers (cycle/time/instret intersection with the Hypervisor)
- `SPEC/riscv-isa-manual/src/unpriv/zihpm.adoc` — Zihpm Extension for Hardware Performance Counters (hpmcounter intersection with the Hypervisor)
- `DOCS/testplan/Zkr_test_plan.md` — Zkr standalone test plan
- `DOCS/testplan/zihintntl_test_plan.md` — Zihintntl standalone test plan
- `DOCS/testplan/zcmt_test_plan.md` — Zcmt standalone test plan
- `DOCS/testplan/Zawrs_test_plan.md` — Zawrs standalone test plan (non-Hypervisor scenarios)
- `DOCS/testplan/Hypervisor_CSR_test_plan.md` — Hypervisor CSR subset test plan
- `DOCS/testplan/Hypervisor_Interrupts_test_plan.md` — Hypervisor interrupts subset test plan
- `DOCS/testplan/Hypervisor_Exceptions_test_plan.md` — Hypervisor exceptions and trap subset test plan
- `DOCS/testplan/Hypervisor_2_stage_test_plan.md` — Two-stage translation test plan
- `DOCS/testplan/Hypervisor_gstage_test_plan.md` — G-stage standalone test plan
- `DOCS/testplan/Zicntr_test_plan.md` — Zicntr standalone test plan (non-Hypervisor scenarios)
- `DOCS/testplan/Zihpm_test_plan.md` — Zihpm standalone test plan (non-Hypervisor scenarios)
- `DOCS/testplan/Shcounterenw_test_plan.md` — Shcounterenw test plan (hcounteren writability and gating matrix)
- `DOCS/testplan/Hypervisor_Ss_test_plan.md` — Hypervisor × Ss* cross test plan (including Hypervisor × Sscounterenw Group 3, hcounteren.TM gating of vstimecmp)

---

## Appendix A: Specification Point Coverage Matrix

The following table indicates which test cases cover each specification point in the "Covered Specification Points" section.

| Norm ID | Covered Test IDs |
|---------|------------------|
| `norm:mseccfg_sseed_SorHS-mode_op` | ZKR-HYP-01, ZKR-HYP-02, ZKR-HYP-13 |
| `norm:mseccfg_sseed_VSorVU-mode_op` | ZKR-HYP-03~06, ZKR-HYP-09, ZKR-HYP-10, ZKR-HYP-16, ZKR-HYP-17 |
| `norm:mseccfg_sseed_useed_op_tbl` | ZKR-HYP-03~06, ZKR-HYP-11 |
| `norm:seed_ro_illegal` | ZKR-HYP-07, ZKR-HYP-08, ZKR-HYP-12, ZKR-HYP-13, ZKR-HYP-14, ZKR-HYP-15, ZKR-HYP-18 |
| `norm:NTL_target_definition` | NTL-HYP-01 ~ NTL-HYP-06 |
| `norm:NTL_range` | NTL-HYP-04 |
| `norm:cm-jt_op` | HZCMT-01 ~ HZCMT-03, HZCMT-06 ~ HZCMT-09 |
| `norm:cm-jalt_op` | HZCMT-01 ~ HZCMT-03, HZCMT-06 |
| `norm:jvt_base_vm` | HZCMT-07 ~ HZCMT-09 |
| `norm:Zcmt_fetch` | HZCMT-08, HZCMT-09 |
| `norm:Zcmt_trap` | HZCMT-08, HZCMT-09 |
| `norm:stateen0_jvt_op` | HZCMT-04 ~ HZCMT-06 |
| `norm:htval_trapval` | HZCMT-09 |
| `norm:Zawrs_exec_resume_rules` | HZWRS-01 ~ HZWRS-03, HZWRS-06 (HZWRS-06 is recording type: both legal behaviors are accepted) |
| `norm:Zawrs_virtual_instr_excp` | HZWRS-04, HZWRS-05, HZWRS-06, HZWRS-07 |
| `norm:Zawrs_priv_illegal_instr_excp` | HZWRS-08, HZWRS-09, HZWRS-12 |
| `norm:Zawrs_stall_terminate` | — (implementation-permitted premature stall termination behavior, used as the failure-handling basis of the Group 4 NOTE and Key Consideration 5, no direct case) |
| `norm:H_virtinst_xtval` | HZWRS-07 |
| `norm:vsstatus_vs_sz_acc` | HVEC-01 |
| `norm:vsstatus_vs_mstatus_vs_op_off` | HVEC-02, HVEC-03, HVEC-04 |
| `norm:vsstatus_vs_mstatus_vs_op_active` | HVEC-05, HVEC-06 |
| `norm:hw_mstatus_vs_dirty_update` | HVEC-08 (recording type: both promotion and staying are legal) |
| `norm:vsstatus_sd_op_vs` | HVEC-07 |
| `norm:vsstatus_vs_exists` | HVEC-13 (conditional: probed when misa.v is writable; recording type) |
| `norm:vsstatus_mstatus_FS_off_hypervisor_V_fp_ill` | HVEC-09, HVEC-10, HVEC-11 |
| `norm:vsstatus_mstatus_FS_dirty_hypervisor_V_fp` | HVEC-12 |
| `norm:zicntr_rdtime_op` | HZCNT-01 ~ HZCNT-06 |
| `norm:hpm_unimplemented_counter_access` | HZHPM-01, HZHPM-02, HZHPM-04 (HZHPM-03 is the gating-closed branch, and the exception is triggered by `hcounteren_gate_v1_counter`) |
| `H_scsrs_nomatch_vu_counter` | HZCNT-07, HZCNT-08, HZCNT-09, HZHPM-05 |
| `hcounteren_gate_v1_counter` | HZCNT-05, HZCNT-06, HZCNT-08, HZHPM-03, HZHPM-05 (referenced only as a precondition gating condition; the gating matrix itself is covered by `Shcounterenw_test_plan.md`) |
