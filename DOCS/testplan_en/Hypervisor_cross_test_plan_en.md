# Hypervisor Cross-Extension Test Plan

> This document describes the test plan for cross-scenarios between the Hypervisor (H) extension and other S-mode extensions. These test scenarios were originally marked as "covered by the Hypervisor test plan" or "excluded due to missing H extension" in the individual extension test plans, but analysis revealed that the existing Hypervisor test plans (`Hypervisor_test_plan.md`, `Hypervisor_2_stage_test_plan.md`, `Hypervisor_gstage_test_plan.md`) do not fully cover them.
>
> Generated: 2026-06-22

---

## Scope

### Covered Extension Crosses

- **Hypervisor x Svadu**: `henvcfg.ADUE` writability, HLV/HSV interaction with hardware A/D updating, HFENCE.GVMA synchronization after `menvcfg.ADUE` modification
- **Hypervisor x Sstvala**: Precise `stval` write behavior on guest page-faults (cause 20/23)
- **Hypervisor x Ssccptr**: Cacheability/coherence verification for two-stage page table walks
- **Hypervisor x Sscounterenw**: `hcounteren` control over VS/VU-mode counter writes
- **Hypervisor x Svinval**: HINVAL.VVMA/GVMA instruction functionality, VMID replacing ASID, VS/VU-mode virtual-instruction triggers
- **Hypervisor x Svnapot**: NAPOT PTE support in G-stage translation, reserved encoding faults, simultaneous NAPOT usage in both stages
- **Hypervisor x Svpbmt**: PBMT attribute layered override behavior in two-stage address translation, G-stage/VS-stage PBMT override rules
- **Hypervisor x Ssstateen**: `hstateen0-3` CSR existence and accessibility, `hstateen` bit 63 controlling VS-mode access to `sstateen`, `hstateen` read-only zero propagation to VS-mode, `hstateen0` function bits (SE0/ENVCFG/CSRIND/IMSIC/AIA/CONTEXT) controlling VS-mode state, `hstateen` read-only constraints and encoding consistency
- **Hypervisor x Sstc**: `henvcfg.STCE` writability and constraints, VS-mode access control for `stimecmp`/`vstimecmp`, `vstimecmp` CSR read/write, VSTIP synthesis logic, `henvcfg.STCE` control over VS-mode timer, VS-mode timer interrupt trapping
- **Hypervisor x Smcsrind**: `mstateen0[60]` (CSRIND) controlling S-mode (HS-mode) access to `vsiselect`/`vsireg*`, verification that M-mode access is not gated by state-enable
- **Hypervisor x Sscsrind**: VS-level CSR (vsiselect/vsireg*) basic functionality, virtual-instruction exception behavior, `hstateen0[60]` control over VS/VU-mode access, VS-mode transparent remapping of sireg* to vsireg*
- **Hypervisor x Ssdbltrp**: `henvcfg.DTE` enable/disable control for VS-mode, `vsstatus.SDT` field behavior and SDT/SIE mutual exclusion, SRET clearing of `vsstatus.SDT`, MRET/SRET/MNRET cross-mode clearing of SDT/vsstatus.SDT in Hypervisor scenarios
- **Hypervisor x Smctr**: `hstateen0.CTR` control over VS-mode CTR state access, `mstateen0.CTR=0` blocking `vsctrctl`, MTE external trap recording behavior from VS/VU-mode to M-mode
- **Hypervisor x Ssctr**: `vsctrctl` CSR basic functionality and field verification, VS/VU-mode external trap recording (STE/vsSTE), virtualization mode transition configuration source, VS-mode Freeze behavior (vsctrctl controlled), VS-mode access restrictions on sctrdepth/SCTRCLR, hstateen0.CTR control over VS-mode CTR access

### Out of Scope

- Hypervisor base functionality already covered by `Hypervisor_test_plan.md`, `Hypervisor_2_stage_test_plan.md`, `Hypervisor_gstage_test_plan.md`
- Sha sub-extension already covered by `Shcounterenw_test_plan.md` (a different extension lineage from Sscounterenw)
- Individual extension behavior in non-Hypervisor scenarios (covered by their respective test plans)
- Ssdbltrp non-Hypervisor tests (`sstatus`.SDT field, S-mode double-trap, `menvcfg`.DTE basic control, `medeleg`[16], `mtval2`) -- covered by `Ssdbltrp_test_plan.md`
- Smcsrind M-mode CSR (miselect/mireg\*) basic functionality and WARL behavior -- covered by `Smcsrind_test_plan.md`
- `mstateen0[60]` control over S-mode access to siselect/sireg\* (non-H-extension CSRs) -- covered by `Smcsrind_test_plan.md` Group 4

---

## Specification References

| Norm ID | Source | Description |
|---------|--------|-------------|
| `norm:henvcfg_adue_op` | `hypervisor.adoc` | If the Svadu extension is implemented, the ADUE bit controls whether hardware updating of PTE A/D bits is enabled for VS-stage address translation. When ADUE=1, hardware updating is enabled. When ADUE=0, the implementation behaves as though Svade were implemented for VS-stage address translation. If Svadu is not implemented, ADUE is read-only zero. |
| `norm:svadu_henvcfg_adue_writable` | `svadu.adoc` | When Svadu is implemented, `henvcfg.ADUE` must be writable. |
| `norm:svadu_hfence_gvma_sync` | `svadu.adoc` | After modifying `menvcfg.ADUE`, a `HFENCE.GVMA(x0,x0)` is required to synchronize the change across all VMIDs. |
| `norm:H_guest_page_fault` | `hypervisor.adoc` | On a guest-page fault, `mtval` or `stval` is written with the faulting guest virtual address. |
| `norm:sstvala_stval_pc` | `sstvala.adoc` | When a page-fault is triggered by an instruction fetch, `stval` is written with the faulting virtual address (PC). |
| `norm:Ssccptr_cacheable_coherent_supports_pt_read` | `ssccptr.adoc` | If the Ssccptr extension is implemented, then main memory regions with both the cacheability and coherence PMAs must support hardware page-table reads. |
| `norm:sscounterenw_hpmcounter_hcounteren` | `sscounterenw.adoc` | If the Sscounterenw extension is implemented, then for any `hpmcounter` that is not read-only zero, the corresponding bit in `scounteren` must be writable. |
| `norm:hcounteren_vs_vu_control` | `hypervisor.adoc` | The `hcounteren` CSR controls availability of performance monitoring counters to VS-mode and VU-mode. |
| `norm:Svinval_hinval_vvma_gvma` | `svinval.adoc` | HINVAL.VVMA and HINVAL.GVMA have the same semantics as SINVAL.VMA, except that they combine with SFENCE.W.INVAL and SFENCE.INVAL.IR to replace HFENCE.VVMA and HFENCE.GVMA, respectively. |
| `norm:Svinval_hinval_gvma_uses_vmid` | `svinval.adoc` | HINVAL.GVMA uses VMIDs instead of ASIDs. |
| `norm:Svinval_virtual_instruction_vu_vs` | `svinval.adoc` | An attempt to execute HINVAL.VVMA or HINVAL.GVMA in VS-mode or VU-mode, or to execute SINVAL.VMA in VU-mode, raises a virtual-instruction exception. |
| `norm:Svinval_sfence_w_inval_inval_vu_mode` | `svinval.adoc` | An attempt to execute SFENCE.W.INVAL or SFENCE.INVAL.IR in VU-mode raises a virtual-instruction exception. |
| `norm:Svnapot_hyp_gstage` | `svnapot.adoc` | If the Hypervisor extension is also implemented, Svnapot is supported in G-stage translation. |
| `norm:Svpbmt_hgatp_stage_override_rule` | `svpbmt.adoc` | When `hgatp.MODE` is not Bare, a nonzero PBMT field in a G-stage leaf PTE overrides the PMA to produce intermediate memory attributes. |
| `norm:Svpbmt_vsatp_stage_override_rule` | `svpbmt.adoc` | When `vsatp.MODE` is not Bare, a nonzero PBMT field in a VS-stage leaf PTE overrides the intermediate memory attributes to produce the final memory attributes. |
| `norm:hstateen_rv64_csrs` | `smstateen.adoc` | When H extension is implemented, hstateen0-3 CSRs are added. |
| `norm:stateen_rv32_upper_bits_csrs` | `smstateen.adoc` | RV32 provides additional hstateen0h-3h CSRs for upper 32 bits. |
| `norm:hstateen_encoding` | `smstateen.adoc` | hstateen CSRs have the same encoding as mstateen CSRs. |
| `norm:hstateen_bit_63_op` | `smstateen.adoc` | Bit 63 of each hstateen CSR controls whether VS-mode may access the corresponding sstateen CSR. |
| `norm:hstateen_bit_63_writable` | `smstateen.adoc` | Bit 63 of each hstateen CSR is always writable (not read-only). |
| `norm:sstateen_vsmode_access_roz` | `smstateen.adoc` | For any bit that is zero in hstateen (whether read-only zero or written to zero), the corresponding bit in sstateen appears as read-only zero when accessed from VS-mode. |
| `norm:sstateen_ro1_bits` | `smstateen.adoc` | A bit in sstateen cannot be read-only one unless the same bit in both mstateen and hstateen (when H is implemented) is also read-only one. |
| `norm:hstateen_ro1_bits` | `smstateen.adoc` | A bit in hstateen cannot be read-only one unless the same bit in mstateen is also read-only one. |
| `norm:hstateen_sstateen_zero_initialization` | `smstateen.adoc` | After M-mode software modifies any mstateen CSR, it is responsible for initializing the corresponding hstateen and sstateen CSRs to zero. |
| `norm:stateen_warl_access` | `smstateen.adoc` | Each standard-defined bit in stateen CSRs is WARL (Write Any Values, Reads Legal Values). |
| `norm:stateen_unimplemented_state_roz` | `smstateen.adoc` | Bits that control state for unimplemented extensions are read-only zero. |
| `norm:stateen_reserved_roz` | `smstateen.adoc` | Reserved bits in stateen CSRs are read-only zero. |
| `norm:hstateen0_SE0_op` | `smstateen.adoc` | hstateen0.SE0 (bit 63) controls whether VS-mode may access sstateen0. |
| `norm:hstateen0_envcfg_op` | `smstateen.adoc` | hstateen0.ENVCFG (bit 62) controls whether VS-mode may access senvcfg. |
| `norm:hstateen0_csrind_op` | `smstateen.adoc` | hstateen0.CSRIND (bit 60) controls whether VS-mode may access siselect and sireg* (which are actually vsiselect and vsireg*). |
| `norm:hstateen0_imsic_op` | `smstateen.adoc` | hstateen0.IMSIC (bit 58) controls access to guest IMSIC state and vstopei in VS-mode. |
| `norm:hstateen0_aia_op` | `smstateen.adoc` | hstateen0.AIA (bit 59) controls VS-mode access to Ssaia state not covered by CSRIND or IMSIC bits. |
| `norm:hstateen0_context_op` | `smstateen.adoc` | hstateen0.CONTEXT (bit 57) controls whether VS-mode may access scontext. |
| `norm:Sstvala_virtual_inst_tval_inst` | `sstvala.adoc` | When a virtual-instruction exception is raised, `stval` must be written with the faulting instruction encoding. |
| `norm:henvcfg_stce` | `hypervisor.adoc` | henvcfg.STCE=1 enables vstimecmp; when STCE=0, VS-mode (V=1) access to stimecmp raises virtual-instruction exception. |
| `norm:vstimecmp_exist` | `sstc.adoc` | Sstc adds a new VS-level vstimecmp CSR. |
| `norm:sstc_vs_facility` | `sstc.adoc` | Sstc provides a similar timer mechanism for VS-mode via the Hypervisor extension. |
| `norm:hip_vstip_vstie_acc_op` | `hypervisor.adoc` | VSTIP = hvip.VSTIP OR vstimecmp timer signal. |

---

## Group 1. Hypervisor x Svadu Cross Tests

**Specification References**:
- `norm:henvcfg_adue_op`: ADUE bit controls VS-stage A/D bit hardware updating
- `norm:svadu_henvcfg_adue_writable`: `henvcfg.ADUE` must be writable
- `norm:svadu_hfence_gvma_sync`: HFENCE.GVMA synchronization required after `menvcfg.ADUE` modification

**Test Responsibility**: Verify Svadu extension behavior in Hypervisor two-stage translation scenarios, including CSR writability, HLV/HSV instruction interaction, and cross-VMID synchronization.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SVADU-01 | henvcfg.ADUE writability verification | HS-mode writes henvcfg.ADUE=1 and reads back; then writes ADUE=0 and reads back | If Svadu is implemented, ADUE is writable and reads back consistently; if Svadu is not implemented, ADUE is read-only zero |
| HCROSS-SVADU-02 | HLV instruction interaction with Svadu (ADUE=1) | henvcfg.ADUE=1, VS-stage PTE A=0, HS-mode executes HLV.D to read the GPA | Access succeeds, VS-stage PTE A bit is automatically set to 1 by hardware (implicit accesses triggered by HLV also follow ADUE control) |
| HCROSS-SVADU-03 | HSV instruction interaction with Svadu (ADUE=1) | henvcfg.ADUE=1, VS-stage PTE A=1,D=0, HS-mode executes HSV.D to write the GPA | Access succeeds, VS-stage PTE D bit is automatically set to 1 by hardware |
| HCROSS-SVADU-04 | HLV instruction interaction with Svade (ADUE=0) | henvcfg.ADUE=0, VS-stage PTE A=0, HS-mode executes HLV.D to read the GPA | guest-page-fault (cause=21), hardware does not automatically update A bit (behaves as Svade) |
| HCROSS-SVADU-05 | HFENCE.GVMA synchronization after menvcfg.ADUE modification | Change menvcfg.ADUE from 0 to 1, without executing HFENCE.GVMA, VS-mode accesses a page with A=0; then execute HFENCE.GVMA(x0,x0) and repeat the access | First access may follow old behavior (implementation-dependent); after HFENCE.GVMA, behavior must follow new ADUE value (A bit updated by hardware) |
| HCROSS-SVADU-06 | VMID-specific synchronization after menvcfg.ADUE modification | Change menvcfg.ADUE from 1 to 0, execute HFENCE.GVMA(vmid, x0) for a specific VMID only, verify behavior for that VMID and other VMIDs | The specified VMID must follow the new ADUE value; other VMIDs are implementation-dependent (may still follow the old value) |

> [!NOTE]
> - HCROSS-SVADU-02~04 require executing HLV/HSV instructions in HS-mode to verify that implicit access (page table walk) A/D updating behavior is also controlled by `henvcfg.ADUE`.
> - HCROSS-SVADU-05~06 verify synchronization semantics after `menvcfg.ADUE` changes. `HFENCE.GVMA(x0,x0)` flushes all VMIDs, `HFENCE.GVMA(vmid,x0)` flushes only a specific VMID.
> - If the platform does not implement the Svadu extension, HCROSS-SVADU-01 should verify ADUE is read-only zero, and HCROSS-SVADU-02~06 should TEST_SKIP.

---

## Group 2. Hypervisor x Sstvala Cross Tests

**Specification References**:
- `norm:H_guest_page_fault`: `stval` written with faulting GVA on guest-page-fault
- `norm:sstvala_stval_pc`: `stval` written with faulting PC on instruction page-fault
- `norm:Sstvala_virtual_inst_tval_inst`: `stval` must be written with faulting instruction encoding on virtual-instruction exception

**Test Responsibility**: Verify Sstvala extension precise write behavior for `stval`/`vstval` on various exceptions in Hypervisor scenarios. For address-type exceptions (guest-page-fault cause 20/21/23), `stval` must equal the faulting GVA; for instruction-type exceptions (virtual-instruction cause=22), `stval` must equal the faulting instruction encoding. The existing Hypervisor test plan already covers cause=21 (load guest-page-fault) stval verification; this group adds precise verification for cause=20 (instruction), cause=23 (store/AMO), and instruction encoding precision for virtual-instruction.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSTVALA-01 | Precise stval on instruction guest-page-fault (cause=20) | VS-mode jumps to execute an address with unmapped GPA in G-stage, triggering inst guest-page-fault trap to HS-mode | scause=20, stval == faulting GVA (i.e., the jump target address), not 0 or other value |
| HCROSS-SSTVALA-02 | Precise stval on store guest-page-fault (cause=23) | VS-mode performs a store to an address with unmapped GPA in G-stage, triggering store guest-page-fault trap to HS-mode | scause=23, stval == faulting GVA (i.e., the store target address), not 0 |
| HCROSS-SSTVALA-03 | Precise stval on AMO guest-page-fault (cause=23) | VS-mode performs an AMO (e.g., AMOADD.W) on an address with unmapped GPA in G-stage, triggering guest-page-fault trap to HS-mode | scause=23, stval == faulting GVA (i.e., the AMO target address) |
| HCROSS-SSTVALA-04 | Precise vstval on instruction guest-page-fault (delegated to VS-mode) | Configure hedeleg to delegate inst guest-page-fault to VS-mode, VS-mode jumps to execute an unmapped address | vscause=20, vstval == faulting GVA |
| HCROSS-SSTVALA-05 | Precise vstval on store guest-page-fault (delegated to VS-mode) | Configure hedeleg to delegate store guest-page-fault to VS-mode, VS-mode stores to an unmapped address | vscause=23, vstval == faulting GVA |
| HCROSS-SSTVALA-06 | Precise stval on virtual-instruction exception: VS-mode reads hstatus | VS-mode executes `csrrs x5, hstatus, x0` (encoding 0x600022F3), triggering virtual-instruction (cause=22) | scause=22, stval == 0x600022F3 (faulting instruction encoding) |
| HCROSS-SSTVALA-07 | Precise stval on virtual-instruction exception: VS-mode writes hgatp | VS-mode executes `csrrw x0, hgatp, x0` (encoding 0x68001073), triggering virtual-instruction (cause=22) | scause=22, stval == 0x68001073 (faulting instruction encoding) |
| HCROSS-SSTVALA-08 | Precise stval on virtual-instruction exception: VS-mode reads hideleg | VS-mode executes `csrrs x5, hideleg, x0` (encoding 0x603022F3), triggering virtual-instruction (cause=22) | scause=22, stval == 0x603022F3 (faulting instruction encoding) |

> [!NOTE]
> - The core semantics of Sstvala guarantee that `stval` is written with the faulting address (rather than 0). The base H extension specification allows `stval` to be 0 in certain scenarios, while the Sstvala extension mandates writing the precise address.
> - HCROSS-SSTVALA-01~03 verify `stval` precision when trapping to HS-mode; HCROSS-SSTVALA-04~05 verify `vstval` precision when delegated to VS-mode.
> - Difference from the GFAULT series in `Hypervisor_gstage_test_plan.md`: The GFAULT series primarily verifies fault triggering and htval encoding; this group focuses on precise stval/vstval value assertions.
> - HCROSS-SSTVALA-06~08 verify Sstvala precision requirements for **instruction-type exceptions**: on virtual-instruction exception (cause=22), `stval` must contain the encoding of the faulting instruction (zero-extended to XLEN). This differs from guest-page-fault (address-type exception) stval semantics, which require writing the faulting virtual address. Instruction encoding derivation: `csrrs x5, 0x600, x0` = `[31:20]=0x600 [19:15]=00000 [14:12]=010 [11:7]=00101 [6:0]=1110011` = `0x600022F3`.
> - HCROSS-SSTVALA-06~08 are migrated from `sstvala_test_plan.md` Group 6 (TVAL-VI-01~03). Requires H extension (`ENABLE_HYP=1`).

---

## Group 3. Hypervisor x Ssccptr Cross Tests

**Specification References**:
- `norm:Ssccptr_cacheable_coherent_supports_pt_read`: Main memory regions with both cacheability and coherence PMAs must support hardware page table reads

**Test Responsibility**: Verify that in Hypervisor two-stage translation scenarios, VS-stage and G-stage page table walks complete correctly in main memory regions satisfying PMA conditions.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSCCPTR-01 | VS-stage page table walk in cacheable+coherent main memory | Establish VS-stage page table in default main memory (satisfying cacheability+coherence PMA), enable two-stage translation, VS-mode executes a load | Page table walk succeeds, load returns correct value (hardware can correctly read VS-stage page table) |
| HCROSS-SSCCPTR-02 | G-stage page table walk in cacheable+coherent main memory | Establish G-stage page table in default main memory, enable two-stage translation, VS-mode executes a load | Page table walk succeeds, load returns correct value (hardware can correctly read G-stage page table) |
| HCROSS-SSCCPTR-03 | Both-stage page tables in cacheable+coherent main memory | Both VS-stage and G-stage page tables allocated in default main memory, VS-mode executes load/store | Both-stage page table walks succeed, R/W returns correct values |
| HCROSS-SSCCPTR-04 | PMA attribute verification for physical pages containing G-stage page table | Attempt to allocate G-stage page table in non-cacheable or non-coherent region (if platform supports), verify behavior | If PMA does not satisfy Ssccptr requirements, behavior is implementation-dependent (page walk may fail); if platform does not support PMA configuration, TEST_SKIP |

> [!NOTE]
> - Ssccptr is a PMA-level constraint extension that does not introduce new instructions or CSRs. On QEMU/Spike simulators, main memory regions satisfy cacheability+coherence PMA conditions by default, so HCROSS-SSCCPTR-01~03 are expected to PASS on simulators.
> - HCROSS-SSCCPTR-04 requires platform support for dynamic PMA attribute configuration, which most simulators and general-purpose hardware do not support; expected TEST_SKIP.
> - The core value of this test group is ensuring that Hypervisor two-stage translation page table walks do not fail due to PMA constraints, which is a critical correctness guarantee for virtualization scenarios.

---

## Group 4. Hypervisor x Sscounterenw Cross Tests

**Specification References**:
- `norm:hcounteren_vs_vu_control`: `hcounteren` controls VS/VU-mode counter availability
- `norm:sscounterenw_hpmcounter_hcounteren`: Corresponding bit must be writable for any non-read-only-zero hpmcounter

**Test Responsibility**: Verify `hcounteren` register control behavior over VS/VU-mode performance monitoring counter (hpmcounter) access, and writability of corresponding `hcounteren` bits.

> [!NOTE]
> - Sscounterenw extension requirement: for any `hpmcounter` that is not read-only zero, the corresponding bit in `scounteren` (and `hcounteren`) must be writable. This group verifies `hcounteren` writability and control behavior.
> - Refer to `Shcounterenw_test_plan.md` for extension tests.

---

## Group 5. Hypervisor x Svinval Cross Tests

**Specification References**:
- `norm:Svinval_hinval_vvma_gvma`: HINVAL.VVMA/GVMA instruction functionality
- `norm:Svinval_hinval_gvma_uses_vmid`: HINVAL.GVMA uses VMID instead of ASID
- `norm:Svinval_virtual_instruction_vu_vs`: VS/VU-mode execution of HINVAL triggers virtual-instruction
- `norm:Svinval_sfence_w_inval_inval_vu_mode`: VU-mode execution of SFENCE.W.INVAL/SFENCE.INVAL.IR triggers virtual-instruction

**Test Responsibility**: Verify Svinval extension HINVAL.VVMA/GVMA instruction functionality, VMID semantics, and VS/VU-mode exception triggering behavior in Hypervisor scenarios.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SINVAL-01 | HINVAL.VVMA basic functionality | HS-mode modifies VS-stage PTE, executes HINVAL.VVMA(va, asid), VS-mode verifies new PTE takes effect | VS-mode uses new PTE when accessing va (HINVAL.VVMA correctly flushes VS-stage TLB) |
| HCROSS-SINVAL-02 | HINVAL.GVMA basic functionality | HS-mode modifies G-stage PTE, executes HINVAL.GVMA(gpa, vmid), VS-mode verifies new PTE takes effect | VS-mode uses new PTE when accessing the corresponding GPA (HINVAL.GVMA correctly flushes G-stage TLB) |
| HCROSS-SINVAL-03 | HINVAL.VVMA combined with SFENCE.W.INVAL/SFENCE.INVAL.IR | HS-mode modifies multiple VS-stage PTEs, executes multiple HINVAL.VVMA, then executes SFENCE.W.INVAL + SFENCE.INVAL.IR, VS-mode verifies all new PTEs take effect | All modified PTEs take effect (combined semantics correct) |
| HCROSS-SINVAL-04 | HINVAL.GVMA combined with SFENCE.W.INVAL/SFENCE.INVAL.IR | HS-mode modifies multiple G-stage PTEs, executes multiple HINVAL.GVMA, then executes SFENCE.W.INVAL + SFENCE.INVAL.IR, VS-mode verifies all new PTEs take effect | All modified PTEs take effect (combined semantics correct) |
| HCROSS-SINVAL-05 | HINVAL.GVMA uses VMID (specific VMID flush) | HS-mode modifies G-stage PTE, executes HINVAL.GVMA(gpa, vmid=5), verifies VMID=5 TLB is flushed and VMID=6 TLB is not flushed | VMID=5 access uses new PTE; VMID=6 access may still use old PTE (implementation-dependent) |
| HCROSS-SINVAL-06 | HINVAL.GVMA uses VMID=0 (all VMIDs flush) | HS-mode modifies G-stage PTE, executes HINVAL.GVMA(gpa, vmid=0), verifies all VMIDs' TLBs are flushed | All VMIDs' accesses use the new PTE |
| HCROSS-SINVAL-07 | VS-mode execution of HINVAL.VVMA triggers virtual-instruction | VS-mode executes HINVAL.VVMA | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-08 | VS-mode execution of HINVAL.GVMA triggers virtual-instruction | VS-mode executes HINVAL.GVMA | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-09 | VU-mode execution of HINVAL.VVMA triggers virtual-instruction | VU-mode executes HINVAL.VVMA | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-10 | VU-mode execution of HINVAL.GVMA triggers virtual-instruction | VU-mode executes HINVAL.GVMA | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-11 | VU-mode execution of SFENCE.W.INVAL triggers virtual-instruction | VU-mode executes SFENCE.W.INVAL | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-12 | VU-mode execution of SFENCE.INVAL.IR triggers virtual-instruction | VU-mode executes SFENCE.INVAL.IR | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-13 | VS-mode execution of SFENCE.W.INVAL succeeds (VTVM=0) | hstatus.VTVM=0, VS-mode executes SFENCE.W.INVAL | Executes normally, no exception (SFENCE.W.INVAL is not gated by VTVM) |
| HCROSS-SINVAL-14 | VS-mode execution of SFENCE.INVAL.IR succeeds (VTVM=0) | hstatus.VTVM=0, VS-mode executes SFENCE.INVAL.IR | Executes normally, no exception |

> [!NOTE]
> - HINVAL.VVMA/GVMA are fine-grained TLB flush instructions provided by the Svinval extension for Hypervisor scenarios, functionally equivalent to HFENCE.VVMA/GVMA but supporting batch flush optimization.
> - HCROSS-SINVAL-05~06 verify HINVAL.GVMA VMID semantics: when VMID is nonzero, only the specific VMID's TLB is flushed; when VMID=0, all VMIDs are flushed. This is consistent with HFENCE.GVMA semantics.
> - HCROSS-SINVAL-07~12 verify virtual-instruction exception triggering when VS/VU-mode executes HINVAL and SFENCE.W.INVAL/SFENCE.INVAL.IR, which is a key guarantee for Hypervisor security isolation.
> - Difference from `Hypervisor_2_stage_test_plan.md` Group 22: Group 22 only covered exception triggering for SINVAL.VMA with VTVM=1 and HINVAL.GVMA with TVM=1 (2 test cases); this group adds HINVAL instruction functionality, VMID semantics, and complete VS/VU-mode virtual-instruction coverage (14 test cases).

---

## Group 6. Hypervisor x Svnapot Cross Tests

**Specification References**:
- `norm:Svnapot_hyp_gstage`: If Hypervisor extension is also implemented, Svnapot is supported in G-stage translation

**Test Responsibility**: Verify Svnapot behavior in G-stage translation, including basic NAPOT PTE translation in G-stage, reserved encoding faults, and correctness of two-stage translation when both VS-stage and G-stage use NAPOT simultaneously.

> **Note**: This test group is migrated from `svnapot_test_plan.md` Group 10. Requires both H extension and Svnapot extension to be available.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SVNAPOT-01 | G-stage 64 KiB NAPOT basic translation | Configure 64 KiB NAPOT PTE in G-stage page table | GPA-to-SPA translation correct |
| HCROSS-SVNAPOT-02 | G-stage NAPOT reserved encoding fault | G-stage NAPOT PTE uses reserved encoding | guest-page-fault |
| HCROSS-SVNAPOT-03 | G-stage and VS-stage both use NAPOT simultaneously | Both VS-stage and G-stage use NAPOT PTEs | Two-stage translation correct |

> [!NOTE]
> - This group verifies Svnapot extension behavior in Hypervisor G-stage translation. The Svnapot specification explicitly states that if the Hypervisor extension is also implemented, NAPOT translation is supported in G-stage page tables.
> - HCROSS-SVNAPOT-01 verifies basic GPA-to-SPA translation for 64 KiB NAPOT PTE in G-stage, complementary to the regular PTE tests in `Hypervisor_gstage_test_plan.md`.
> - HCROSS-SVNAPOT-02 verifies that when G-stage NAPOT PTE uses a reserved encoding (ppn[0] low 4 bits not `1000` with N=1), hardware should trigger a guest-page-fault. This is consistent with the reserved encoding behavior in VS-stage (see `svnapot_test_plan.md` Group 3).
> - HCROSS-SVNAPOT-03 verifies the scenario where both VS-stage and G-stage use NAPOT PTEs simultaneously in two-stage translation: VS-stage maps GVA-to-GPA using NAPOT, G-stage maps GPA-to-SPA also using NAPOT, and the final GVA-to-SPA translation should be correct.

---

## Group 7. Hypervisor x Svpbmt Cross Tests

**Specification References**:
- `norm:Svpbmt_hgatp_stage_override_rule`: When `hgatp.MODE` is nonzero, nonzero PBMT in G-stage PTE overrides PMA to produce intermediate attributes
- `norm:Svpbmt_vsatp_stage_override_rule`: When `vsatp.MODE` is nonzero, nonzero PBMT in VS-stage PTE overrides intermediate attributes to produce final attributes

**Test Responsibility**: Verify PBMT attribute layered override behavior in two-stage address translation, including G-stage PBMT overriding PMA, VS-stage PBMT overriding intermediate attributes, both-stage overlay, and the scenario where `hgatp.MODE=0` skips G-stage override.

> **Note**: This test group is migrated from `svpbmt_test_plan.md` Group 10. Requires both H extension and Svpbmt extension to be available.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SVPBMT-01 | G-stage PBMT=NC overrides PMA | hgatp.MODE nonzero, G-stage PTE sets PBMT=NC, VS-stage PTE PBMT=0 (no override). Chain: G-stage PBMT=NC overrides PMA -> intermediate=NC; VS-stage PBMT=0 does not trigger override -> final=NC | Final attributes are NC |
| HCROSS-SVPBMT-02 | VS-stage PBMT=IO overrides intermediate attributes | hgatp.MODE nonzero, G-stage PTE PBMT=0 (no override) -> intermediate=PMA; VS-stage PTE PBMT=IO overrides intermediate -> final=IO | Final attributes are IO |
| HCROSS-SVPBMT-03 | Both stages nonzero overlay | G-stage PTE PBMT=NC -> intermediate=NC; VS-stage PTE PBMT=IO overrides intermediate -> final=IO | Final attributes are IO |
| HCROSS-SVPBMT-04 | hgatp.MODE=0 skips G-stage | hgatp.MODE=0 -> G-stage inactive, intermediate=PMA; VS-stage PTE PBMT=NC overrides intermediate -> final=NC | Final attributes are NC |

> [!NOTE]
> - This group verifies Svpbmt extension PBMT attribute layered override rules in Hypervisor two-stage translation. The Svpbmt specification defines the two-stage override chain: PMA -> G-stage PBMT override -> intermediate -> VS-stage PBMT override -> final.
> - HCROSS-SVPBMT-01 verifies that G-stage PBMT overrides PMA to produce intermediate attributes, while VS-stage PBMT=0 does not trigger a second override, so final attributes remain NC.
> - HCROSS-SVPBMT-02 verifies VS-stage PBMT overriding intermediate attributes: G-stage PBMT=0 does not override PMA (intermediate=PMA), VS-stage PBMT=IO overrides intermediate to produce final=IO.
> - HCROSS-SVPBMT-03 verifies overlay behavior when both stages have nonzero PBMT: G-stage PBMT=NC produces intermediate=NC, VS-stage PBMT=IO overrides intermediate to produce final=IO. VS-stage override priority is higher than G-stage.
> - HCROSS-SVPBMT-04 verifies that when `hgatp.MODE=0` (Bare), G-stage is inactive and the PBMT override chain starts from VS-stage: intermediate=PMA, VS-stage PBMT=NC overrides to produce final=NC.

---

## Group 8. Hypervisor x Ssstateen Cross Tests

**Specification References**:
- `norm:hstateen_rv64_csrs`: hstateen0-3 CSRs added when H extension is implemented
- `norm:hstateen_bit_63_op`: hstateen bit 63 controls VS-mode access to corresponding sstateen
- `norm:hstateen_bit_63_writable`: hstateen bit 63 is always writable
- `norm:sstateen_vsmode_access_roz`: Bits that are zero in hstateen appear as read-only zero when VS-mode accesses sstateen
- `norm:hstateen0_SE0_op`: hstateen0.SE0 controls VS-mode access to sstateen0
- `norm:hstateen0_envcfg_op`: hstateen0.ENVCFG controls VS-mode access to senvcfg
- `norm:hstateen0_csrind_op`: hstateen0.CSRIND controls VS-mode access to siselect/sireg*
- `norm:hstateen0_imsic_op`: hstateen0.IMSIC controls guest IMSIC state and vstopei
- `norm:hstateen0_aia_op`: hstateen0.AIA controls remaining Ssaia state not covered by CSRIND/IMSIC
- `norm:hstateen0_context_op`: hstateen0.CONTEXT controls VS-mode access to scontext
- `norm:hstateen_ro1_bits`: hstateen bit cannot be RO1 unless mstateen same bit is also RO1
- `norm:hstateen_encoding`: hstateen encoding consistent with mstateen

**Test Responsibility**: Verify Ssstateen extension hstateen CSR behavior in Hypervisor scenarios, including hstateen existence and accessibility, bit 63 control over VS-mode access to sstateen, hstateen read-only zero propagation to VS-mode, hstateen0 function bits (SE0/ENVCFG/CSRIND/IMSIC/AIA/CONTEXT) controlling VS-mode state, and hstateen read-only constraints and encoding consistency.

> **Note**: This test group is migrated from `ssstateen_test_plan.md` Groups 7-12. Requires both H extension and Ssstateen extension to be available. Prerequisite configuration: M-mode must pre-set corresponding mstateen bits to 1 to permit HS-mode access to hstateen.

#### 8.1 hstateen CSR Existence and Accessibility

**Specification References**: `norm:hstateen_rv64_csrs`, `norm:stateen_rv32_upper_bits_csrs`, `norm:hstateen_encoding`

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-01 | hstateen0 readable in HS-mode | Configure mstateen0.SE0=1, HS-mode reads hstateen0 | No exception |
| HCROSS-SSSTA-02 | hstateen0 writable in HS-mode | Configure mstateen0.SE0=1, HS-mode writes hstateen0 and reads back | No exception, writable bits take effect |
| HCROSS-SSSTA-03 | hstateen1 readable/writable in HS-mode | Configure mstateen1 bit63=1, HS-mode reads/writes hstateen1 | No exception |
| HCROSS-SSSTA-04 | hstateen2 readable/writable in HS-mode | Configure mstateen2 bit63=1, HS-mode reads/writes hstateen2 | No exception |
| HCROSS-SSSTA-05 | hstateen3 readable/writable in HS-mode | Configure mstateen3 bit63=1, HS-mode reads/writes hstateen3 | No exception |
| HCROSS-SSSTA-06 | hstateen0h readable/writable in HS-mode (RV32) | Under RV32 configure mstateen0.SE0=1, HS-mode reads/writes hstateen0h | No exception |

#### 8.2 hstateen Bit 63 Controls sstateen Access

**Specification References**: `norm:hstateen_bit_63_op`, `norm:hstateen_bit_63_writable`

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-07 | hstateen0 bit 63 writable (write 0) | HS-mode writes hstateen0 bit 63 to 0 and reads back | bit 63 reads back as 0 |
| HCROSS-SSSTA-08 | hstateen0 bit 63 writable (write 1) | HS-mode writes hstateen0 bit 63 to 1 and reads back | bit 63 reads back as 1 |
| HCROSS-SSSTA-09 | hstateen0.SE0=0 blocks VS-mode access to sstateen0 | Set hstateen0 bit63=0, VS-mode reads sstateen0 | Triggers virtual-instruction exception (cause=22) |
| HCROSS-SSSTA-10 | hstateen0.SE0=1 permits VS-mode access to sstateen0 | Set hstateen0 bit63=1, VS-mode reads sstateen0 | Access succeeds, no exception |
| HCROSS-SSSTA-11 | hstateen0.SE0=0 blocks VS-mode write to sstateen0 | Set hstateen0 bit63=0, VS-mode writes sstateen0 | Triggers virtual-instruction exception (cause=22) |
| HCROSS-SSSTA-12 | hstateen1 bit 63 writable | HS-mode writes hstateen1 bit 63 to 0 and 1 | Each readback value matches the written value |
| HCROSS-SSSTA-13 | hstateen1 bit63=0 blocks VS-mode access to sstateen1 | Set hstateen1 bit63=0, VS-mode reads sstateen1 | Triggers virtual-instruction exception |
| HCROSS-SSSTA-14 | hstateen2 bit 63 controls sstateen2 | Set hstateen2 bit63=0/1, VS-mode accesses sstateen2 | bit63=0 raises exception, bit63=1 succeeds |
| HCROSS-SSSTA-15 | hstateen3 bit 63 controls sstateen3 | Set hstateen3 bit63=0/1, VS-mode accesses sstateen3 | bit63=0 raises exception, bit63=1 succeeds |

#### 8.3 hstateen Read-Only Zero Propagation to VS-mode sstateen

**Specification References**: `norm:sstateen_vsmode_access_roz`

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-16 | hstateen0.C=0 propagates to VS-mode sstateen0.C | Set hstateen0.C=0 and bit63=1, VS-mode writes sstateen0.C=1 and reads back | sstateen0.C reads back as 0 in VS-mode |
| HCROSS-SSSTA-17 | hstateen0.C=1 removes propagation to VS-mode | Set hstateen0.C=1 and bit63=1, VS-mode writes sstateen0.C=1 and reads back | sstateen0.C reads back as 1 in VS-mode |
| HCROSS-SSSTA-18 | hstateen0.JVT=0 propagates to VS-mode sstateen0.JVT | Set hstateen0.JVT=0 and bit63=1, VS-mode writes sstateen0.JVT=1 and reads back | sstateen0.JVT reads back as 0 in VS-mode |
| HCROSS-SSSTA-19 | hstateen0 multiple bits propagate simultaneously | Set multiple hstateen0 function bits to 0, VS-mode verifies each corresponding sstateen0 bit | All corresponding bits are read-only zero in VS-mode |
| HCROSS-SSSTA-20 | hstateen0 bit change from 0 to 1 removes propagation | First set hstateen0.C=0 to verify propagation, then change to 1 | sstateen0.C becomes writable in VS-mode |

#### 8.4 hstateen0 Function Bit Controls

**Specification References**: `norm:hstateen0_SE0_op`, `norm:hstateen0_envcfg_op`, `norm:hstateen0_csrind_op`, `norm:hstateen0_imsic_op`, `norm:hstateen0_aia_op`, `norm:hstateen0_context_op`

##### SE0 Bit (bit 63) -- sstateen0 VS-mode Access

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-21 | hstateen0.SE0=0 blocks VS-mode read of sstateen0 | Set hstateen0.SE0=0, VS-mode reads sstateen0 | Triggers virtual-instruction exception |
| HCROSS-SSSTA-22 | hstateen0.SE0=0 blocks VS-mode write to sstateen0 | Set hstateen0.SE0=0, VS-mode writes sstateen0 | Triggers virtual-instruction exception |
| HCROSS-SSSTA-23 | hstateen0.SE0=1 permits VS-mode access to sstateen0 | Set hstateen0.SE0=1, VS-mode reads/writes sstateen0 | Access succeeds |

##### ENVCFG Bit (bit 62) -- senvcfg VS-mode Access

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-24 | hstateen0.ENVCFG=0 blocks VS-mode read of senvcfg | Set ENVCFG=0, VS-mode reads senvcfg | Triggers virtual-instruction exception |
| HCROSS-SSSTA-25 | hstateen0.ENVCFG=0 blocks VS-mode write to senvcfg | Set ENVCFG=0, VS-mode writes senvcfg | Triggers virtual-instruction exception |
| HCROSS-SSSTA-26 | hstateen0.ENVCFG=1 permits VS-mode access to senvcfg | Set ENVCFG=1, VS-mode reads/writes senvcfg | Access succeeds |

##### CSRIND Bit (bit 60) -- siselect/sireg VS-mode Access

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-27 | hstateen0.CSRIND=0 blocks VS-mode read of siselect | Set CSRIND=0, VS-mode reads siselect (actually vsiselect) | Triggers virtual-instruction exception |
| HCROSS-SSSTA-28 | hstateen0.CSRIND=0 blocks VS-mode read of sireg* | Set CSRIND=0, VS-mode reads sireg (actually vsireg) | Triggers virtual-instruction exception |
| HCROSS-SSSTA-29 | hstateen0.CSRIND=1 permits VS-mode access | Set CSRIND=1, VS-mode reads siselect/sireg* | Access succeeds |

##### IMSIC Bit (bit 58) -- Guest IMSIC Control

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-30 | hstateen0.IMSIC=0 blocks VS-mode access to IMSIC | Set IMSIC=0, VS-mode reads stopei (actually vstopei) | Triggers virtual-instruction exception |
| HCROSS-SSSTA-31 | hstateen0.IMSIC=1 permits VS-mode access to IMSIC | Set IMSIC=1, VS-mode reads stopei | Access succeeds |
| HCROSS-SSSTA-32 | hstateen0.IMSIC=0 equivalent to VGEIN=0 | Set IMSIC=0, verify VS-mode cannot access IMSIC, equivalent to hstatus.VGEIN=0 | VS-mode cannot access guest IMSIC |

##### AIA Bit (bit 59) -- Ssaia Remaining State VS-mode Control

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-33 | hstateen0.AIA=0 blocks VS-mode Ssaia state | Set AIA=0, VS-mode accesses Ssaia non-CSRIND/IMSIC state | Triggers virtual-instruction exception |
| HCROSS-SSSTA-34 | hstateen0.AIA=1 allows VS-mode Ssaia state | Set AIA=1, VS-mode accesses Ssaia remaining state | Access succeeds |
| HCROSS-SSSTA-35 | hstateen0.AIA does not affect CSRIND/IMSIC control | Set AIA=0 but CSRIND=1 and IMSIC=1, VS-mode accesses siselect/stopei | Access succeeds (AIA does not control states governed by CSRIND/IMSIC) |

##### CONTEXT Bit (bit 57) -- scontext VS-mode Control

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-36 | hstateen0.CONTEXT=0 blocks VS-mode read of scontext | Set CONTEXT=0, VS-mode reads scontext | Triggers virtual-instruction exception |
| HCROSS-SSSTA-37 | hstateen0.CONTEXT=0 blocks VS-mode write of scontext | Set CONTEXT=0, VS-mode writes scontext | Triggers virtual-instruction exception |
| HCROSS-SSSTA-38 | hstateen0.CONTEXT=1 allows VS-mode access to scontext | Set CONTEXT=1, VS-mode reads/writes scontext | Access succeeds |

#### 8.5 hstateen Read-Only Constraints

**Specification References**: `norm:hstateen_ro1_bits`, `norm:stateen_warl_access`, `norm:stateen_unimplemented_state_roz`, `norm:stateen_reserved_roz`

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-39 | hstateen0 RO1 bit constraint | Identify any RO1 bit in hstateen0, verify the same bit in mstateen0 is also RO1 | The corresponding bit in mstateen0 is also RO1 |
| HCROSS-SSSTA-40 | hstateen1 RO1 bit constraint | Identify any RO1 bit in hstateen1, verify the same bit in mstateen1 is also RO1 | The corresponding bit in mstateen1 is also RO1 |
| HCROSS-SSSTA-41 | hstateen2 RO1 bit constraint | Identify any RO1 bit in hstateen2, verify the same bit in mstateen2 is also RO1 | The corresponding bit in mstateen2 is also RO1 |
| HCROSS-SSSTA-42 | hstateen3 RO1 bit constraint | Identify any RO1 bit in hstateen3, verify the same bit in mstateen3 is also RO1 | The corresponding bit in mstateen3 is also RO1 |
| HCROSS-SSSTA-43 | hstateen0 reserved bits read-only zero | Write 1 to hstateen0 WPRI fields then read back | Reserved bits read back as 0 |
| HCROSS-SSSTA-44 | hstateen0 unimplemented extension bits read-only zero | For bits corresponding to unimplemented extensions, write 1 then read back | Corresponding bits read back as 0 |
| HCROSS-SSSTA-45 | hstateen0 WARL write of legal value | Write a legal value to hstateen0 then read back | Read-back value matches written value (writable bits only) |

#### 8.6 hstateen Encoding Consistency with mstateen

**Specification References**: `norm:hstateen_encoding`

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-46 | hstateen0 bit fields consistent with mstateen0 | Compare writable bit masks of hstateen0 and mstateen0 | Bit field definitions are consistent (C/FCSR/JVT/SE0/ENVCFG/CSRIND/IMSIC/AIA/CONTEXT bit positions match) |
| HCROSS-SSSTA-47 | hstateen0 functional bits symmetric with mstateen0 | Write all 1s to hstateen0, read back valid bits; write all 1s to mstateen0, read back valid bits; compare the overlapping portion | hstateen0 valid bits should be a subset of mstateen0 valid bits |
| HCROSS-SSSTA-48 | hstateen1 encoding consistent with mstateen1 | Compare writable bit masks of hstateen1 and mstateen1 | Bit field definitions are consistent |
| HCROSS-SSSTA-49 | hstateen2 encoding consistent with mstateen2 | Compare writable bit masks of hstateen2 and mstateen2 | Bit field definitions are consistent |
| HCROSS-SSSTA-50 | hstateen3 encoding consistent with mstateen3 | Compare writable bit masks of hstateen3 and mstateen3 | Bit field definitions are consistent |

> [!NOTE]
> - This test group validates the hstateen CSR behavior for the Ssstateen extension in Hypervisor scenarios. The hstateen CSR is the supervisor-level counterpart of mstateen, used to control VS/VU-mode access to extension state and prevent covert channels.
> - Prerequisite for all tests: M-mode must set the corresponding mstateen bit to 1 to permit HS-mode access to hstateen. If the corresponding mstateen bit is 0, the corresponding hstateen bit is read-only zero.
> - hstateen0 bit 63 (SE0) controls VS-mode access to sstateen0. When SE0=0, VS-mode read/write of sstateen0 triggers a virtual-instruction exception (cause=22), not an illegal-instruction exception (cause=2).
> - Each functional bit of hstateen0 (SE0/ENVCFG/CSRIND/IMSIC/AIA/CONTEXT) independently controls VS-mode access to different supervisor-level state. These control bits operate independently; the AIA bit does not affect states governed by CSRIND/IMSIC (HCROSS-SSSTA-35).
> - hstateen read-only zero propagation rule: bits that are zero in hstateen (whether read-only zero or written to zero) appear as read-only zero when VS-mode accesses sstateen (HCROSS-SSSTA-16~20). This is the key mechanism for preventing covert channels.
> - S-mode sstateen tests (including VU-mode virtual-instruction triggers SS-UCTL-09, SS-EXC-04, and RO1 bit constraint SS-ALLOC-03) remain in `ssstateen_test_plan.md` Groups 1-6.
> - The RV32-specific hstateen0h test (HCROSS-SSSTA-06) requires RV32 platform support and should be TEST_SKIP on RV64 platforms.

---

## Group 9. Hypervisor x Sstc Cross Tests

**Specification References**:
- `norm:henvcfg_stce`: henvcfg.STCE=1 enables vstimecmp; when STCE=0, accessing stimecmp with V=1 produces a virtual-instruction exception
- `norm:vstimecmp_exist`: Sstc introduces the VS-level vstimecmp CSR
- `norm:sstc_vs_facility`: Sstc provides a similar timer mechanism for the Hypervisor extension's VS-mode
- `norm:hip_vstip_vstie_acc_op`: VSTIP = hvip.VSTIP OR vstimecmp timer signal

**Test Scope**: Verify Sstc extension behavior in Hypervisor scenarios, including `henvcfg.STCE` writability and constraints, VS-mode access control for `stimecmp`/`vstimecmp`, `vstimecmp` CSR read/write, VSTIP synthesis logic, and VS-mode timer interrupt delivery.

> **Note**: This test group is migrated from `sstc_test_plan.md` Group 1 (SSTC-STCE-03/04), Group 3 (SSTC-ACC-08/09/10), and Group 6 (SSTC-VS-01~10). Both H extension and Sstc extension must be available.

### Test ID Mapping Table

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| SSTC-STCE-03 | HCROSS-SSTC-01 | henvcfg.STCE read-write round-trip |
| SSTC-STCE-04 | HCROSS-SSTC-02 | henvcfg.STCE constrained by menvcfg.STCE |
| SSTC-ACC-08 | HCROSS-SSTC-03 | VS-mode access to stimecmp when henvcfg.STCE=0 |
| SSTC-ACC-09 | HCROSS-SSTC-04 | VS-mode access to stimecmp when henvcfg.STCE=1 |
| SSTC-ACC-10 | HCROSS-SSTC-05 | VS-mode access to stimecmp when hcounteren.TM=0 |
| SSTC-VS-01 | HCROSS-SSTC-06 | M-mode vstimecmp read-write round-trip |
| SSTC-VS-02 | HCROSS-SSTC-07 | vstimecmp all-1s / all-0s read-write |
| SSTC-VS-03 | HCROSS-SSTC-08 | HS-mode vstimecmp read-write |
| SSTC-VS-04 | HCROSS-SSTC-09 | vstimecmp triggers VSTIP |
| SSTC-VS-05 | HCROSS-SSTC-10 | vstimecmp clears VSTIP |
| SSTC-VS-06 | HCROSS-SSTC-11 | VSTIP = hvip.VSTIP OR vstimecmp signal |
| SSTC-VS-07 | HCROSS-SSTC-12 | VSTIP reverts to legacy behavior when henvcfg.STCE=0 |
| SSTC-VS-08 | HCROSS-SSTC-13 | VS-mode accesses vstimecmp via stimecmp |
| SSTC-VS-09 | HCROSS-SSTC-14 | Effect of htimedelta on vstimecmp comparison |
| SSTC-VS-10 | HCROSS-SSTC-15 | VS-mode timer interrupt delivery |

### Test Case List

#### 9.1 henvcfg.STCE Field Control

| Test ID | Test Name | Test Description | Expected Result | Norm Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSTC-01 | henvcfg.STCE read-write round-trip | In M-mode, set menvcfg.STCE=1, then write henvcfg.STCE=1 and verify read-back, then write 0 and verify read-back | STCE bit reads back as written | `norm:henvcfg_stce` |
| HCROSS-SSTC-02 | henvcfg.STCE constrained by menvcfg.STCE | When menvcfg.STCE=0, writing henvcfg.STCE=1 should be ignored (read-only zero) | henvcfg.STCE reads back as 0 | `norm:henvcfg_stce` |

#### 9.2 VS-mode Access Control

| Test ID | Test Name | Test Description | Expected Result | Norm Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSTC-03 | VS-mode access to stimecmp when henvcfg.STCE=0 | menvcfg.STCE=1, henvcfg.STCE=0, VS-mode (V=1) reads stimecmp | Triggers virtual-instruction exception (cause=22) | `norm:henvcfg_stce` |
| HCROSS-SSTC-04 | VS-mode access to stimecmp when henvcfg.STCE=1 | menvcfg.STCE=1, henvcfg.STCE=1, hcounteren.TM=1, VS-mode reads stimecmp | No exception, read succeeds (actually accesses vstimecmp) | `norm:henvcfg_stce` |
| HCROSS-SSTC-05 | VS-mode access to stimecmp when hcounteren.TM=0 | menvcfg.STCE=1, henvcfg.STCE=1, hcounteren.TM=0, VS-mode reads stimecmp | Triggers virtual-instruction exception (cause=22) | `norm:henvcfg_stce` |

#### 9.3 vstimecmp CSR Read/Write

| Test ID | Test Name | Test Description | Expected Result | Norm Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSTC-06 | M-mode vstimecmp read-write round-trip | M-mode writes vstimecmp=0x123456789ABCDEF0 then reads back | Read-back value matches | `norm:vstimecmp_exist` |
| HCROSS-SSTC-07 | vstimecmp all-1s / all-0s read-write | M-mode writes all 1s and all 0s to vstimecmp, reads back each | Read-back values match written values | `norm:vstimecmp_exist` |
| HCROSS-SSTC-08 | HS-mode vstimecmp read-write | STCE=1, TM=1, switch to HS-mode and read/write vstimecmp (via CSR 0x24D directly) | Read/write succeeds | `norm:vstimecmp_exist` |

#### 9.4 VSTIP Synthesis Logic

| Test ID | Test Name | Test Description | Expected Result | Norm Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSTC-09 | vstimecmp triggers VSTIP | Set vstimecmp to a past value (relative to time + htimedelta), check hip.VSTIP | VSTIP = 1 | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-10 | vstimecmp clears VSTIP | Set vstimecmp to maximum value, check hip.VSTIP (assuming hvip.VSTIP=0) | VSTIP = 0 | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-11 | VSTIP = hvip.VSTIP OR vstimecmp signal | hvip.VSTIP=1, vstimecmp set to maximum, check hip.VSTIP | VSTIP = 1 (contributed by hvip.VSTIP) | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-12 | VSTIP reverts to legacy behavior when henvcfg.STCE=0 | menvcfg.STCE=1, henvcfg.STCE=0, set vstimecmp to a past value, check hip.VSTIP | VSTIP determined solely by hvip.VSTIP (vstimecmp signal is inactive) | `norm:henvcfg_stce` |

#### 9.5 VS-mode Timer Functionality

| Test ID | Test Name | Test Description | Expected Result | Norm Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSTC-13 | VS-mode accesses vstimecmp via stimecmp | STCE=1, henvcfg.STCE=1, hcounteren.TM=1, VS-mode writes stimecmp (=vstimecmp), M-mode reads vstimecmp | Read-back value matches (hardware remapping verified) | `norm:sstc_vs_facility` |
| HCROSS-SSTC-14 | Effect of htimedelta on vstimecmp comparison | Set htimedelta to a large positive value, set vstimecmp such that (time+htimedelta) >= vstimecmp, check VSTIP | VSTIP = 1 | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-15 | VS-mode timer interrupt delivery | henvcfg.STCE=1, enable VSTIE, set vstimecmp to a past value, VS-mode should receive timer interrupt | VS-mode trap handler catches cause = interrupt \| 5 | `norm:sstc_vs_facility` |

> [!NOTE]
> - This test group validates the Sstc extension behavior in Hypervisor scenarios. All tests must detect H extension availability at runtime via `HAS_H_EXT()` and TEST_SKIP if unavailable.
> - HCROSS-SSTC-01~02 are migrated from `sstc_test_plan.md` Group 1, verifying `henvcfg.STCE` writability and the constraint relationship with `menvcfg.STCE`.
> - HCROSS-SSTC-03~05 are migrated from `sstc_test_plan.md` Group 3, verifying multi-layer access control for VS-mode access to `stimecmp` (actually `vstimecmp`): when either `henvcfg.STCE` or `hcounteren.TM` is 0, a virtual-instruction exception (cause=22) is triggered.
> - HCROSS-SSTC-06~15 are migrated entirely from `sstc_test_plan.md` Group 6, verifying `vstimecmp` CSR read/write, VSTIP synthesis logic (`hip.VSTIP = hvip.VSTIP OR vstimecmp_signal`), the effect of `htimedelta` on comparison, and VS-mode timer interrupt delivery.
> - HCROSS-SSTC-11 verifies VSTIP OR logic: when `henvcfg.STCE=0`, the vstimecmp signal is inactive and `hvip.VSTIP` reverts to software-writable and affects `hip.VSTIP`.
> - HCROSS-SSTC-13 verifies hardware transparent remapping: VS-mode writing `stimecmp` (CSR 0x14D) actually writes `vstimecmp` (CSR 0x24D), verifiable by M-mode reading back `vstimecmp`.
> - HCROSS-SSTC-15 verifies the complete VS-mode timer interrupt path: VSTIP is delegated to VS-mode via `hideleg`, and the VS-mode trap handler catches cause = `interrupt | 5`.

---

## Group 10. Hypervisor x Smcsrind Cross Tests

**Specification References**:
- `norm:sscsrind_csrs_access_control`: If Smstateen and Smcsrind are both implemented, `mstateen0[60]` (CSRIND) controls access to `siselect`, `sireg*`, `vsiselect`, `vsireg*`. When `mstateen0[60]=0`, access to these CSRs from privilege levels below M-mode triggers an illegal-instruction exception.

**Test Scope**: Verify Smcsrind extension behavior in Hypervisor scenarios, specifically `mstateen0[60]` (CSRIND) control over S-mode (HS-mode) access to `vsiselect`/`vsireg*`. Note: vsiselect/vsireg* are CSRs introduced by the H extension and are only available when the H extension is present.

> **Note**: This test group is extracted from `Smcsrind_test_plan.md` Group 4, specifically targeting test cases that depend on the H extension. The H extension, Smcsrind extension, and Smstateen extension must all be available simultaneously.
>
> **Prerequisite**: M-mode must pre-set `mstateen0[60]` to the desired value to control HS-mode access to vsiselect/vsireg*.

### Test ID Mapping Table

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| MCSRIND-STA (new) | HCROSS-SMCSRIND-01 | mstateen0[60]=0 blocks S-mode read of vsiselect |
| MCSRIND-STA (new) | HCROSS-SMCSRIND-02 | mstateen0[60]=0 blocks S-mode write of vsiselect |
| MCSRIND-STA (new) | HCROSS-SMCSRIND-03 | mstateen0[60]=0 blocks S-mode read of vsireg |
| MCSRIND-STA (new) | HCROSS-SMCSRIND-04 | mstateen0[60]=0 blocks S-mode read of vsireg2~vsireg6 |
| MCSRIND-STA (new) | HCROSS-SMCSRIND-05 | mstateen0[60]=1 allows S-mode access to vsiselect |
| MCSRIND-STA (new) | HCROSS-SMCSRIND-06 | mstateen0[60]=1 allows S-mode access to vsireg* |
| MCSRIND-STA-07 (supplement) | HCROSS-SMCSRIND-07 | mstateen0[60]=0 does not affect M-mode access to vsiselect |
| MCSRIND-STA-08 (supplement) | HCROSS-SMCSRIND-08 | mstateen0[60]=0 does not affect M-mode access to vsireg* |

### Test Case List

| Test ID | Test Name | Test Description | Expected Result | Norm Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SMCSRIND-01 | mstateen0[60]=0 blocks S-mode read of vsiselect | mstateen0[60]=0, S-mode (HS-mode) reads vsiselect (0x250) | Triggers illegal-instruction exception (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-02 | mstateen0[60]=0 blocks S-mode write of vsiselect | mstateen0[60]=0, S-mode writes vsiselect | Triggers illegal-instruction exception (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-03 | mstateen0[60]=0 blocks S-mode read of vsireg | mstateen0[60]=0, S-mode reads vsireg (0x251) | Triggers illegal-instruction exception (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-04 | mstateen0[60]=0 blocks S-mode read of vsireg2~vsireg6 | mstateen0[60]=0, S-mode reads vsireg2~vsireg6 one by one | Each triggers illegal-instruction exception (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-05 | mstateen0[60]=1 allows S-mode access to vsiselect | mstateen0[60]=1, S-mode reads/writes vsiselect | Access succeeds, no exception | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-06 | mstateen0[60]=1 allows S-mode access to vsireg* | mstateen0[60]=1, S-mode reads/writes vsireg~vsireg6 | Access succeeds, no exception (specific vsireg_i behavior depends on vsiselect value) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-07 | mstateen0[60]=0 does not affect M-mode access to vsiselect | mstateen0[60]=0, M-mode reads/writes vsiselect | Access succeeds, no exception (state-enable does not control M-mode) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-08 | mstateen0[60]=0 does not affect M-mode access to vsireg* | mstateen0[60]=0, M-mode reads/writes vsireg~vsireg6 | Access succeeds, no exception | `norm:sscsrind_csrs_access_control` |

> [!NOTE]
> - This test group validates the Smcsrind extension `mstateen0[60]` access control in Hypervisor scenarios. vsiselect/vsireg* are CSRs introduced by the H extension (vsiselect=0x250, vsireg=0x251, vsireg2=0x252, vsireg3=0x253, vsireg4=0x255, vsireg5=0x256, vsireg6=0x257), available only when the H extension is present.
> - HCROSS-SMCSRIND-01~04 verify that when `mstateen0[60]=0`, S-mode (HS-mode, V=0) access to vsiselect/vsireg* triggers an illegal-instruction exception (cause=2). This is symmetric to S-mode access to siselect/sireg* (covered in `Smcsrind_test_plan.md` Group 4).
> - HCROSS-SMCSRIND-05~06 verify that when `mstateen0[60]=1`, S-mode can access vsiselect/vsireg* normally.
> - HCROSS-SMCSRIND-07~08 verify that the state-enable CSR **does not affect** M-mode's own access. This is explicitly stated in the SPEC: state-enable CSRs only affect privilege levels below M-mode.
> - Distinction from Group 8.4 (HCROSS-SSSTA-27~29): Group 8.4 verifies `hstateen0[60]` control over **VS-mode** access to vsiselect/vsireg* (triggering virtual-instruction), while this group verifies `mstateen0[60]` control over **S-mode (HS-mode)** access to vsiselect/vsireg* (triggering illegal-instruction). These are different levels of control mechanisms.
> - All tests must detect H extension, Smcsrind extension, and Smstateen extension availability at runtime; TEST_SKIP if any is unavailable.

---

## Group 11. Hypervisor x Sscsrind Cross Tests

**Specification References**:
- `norm:vsiselect_min_range`: vsiselect supports at least 0..0xFFF
- `norm:vsiselect_msb_op`: MSB semantics
- `norm:vsireg_access_on_legal_vsiselect`: vsireg* behavior with legal vsiselect value
- `norm:vsireg_access_behaviour`: vsireg_i accesses register state, read-only 0, or exception
- `norm:sscsrind_vsmode_csrs_sz`: vsiselect/vsireg* width is always the current XLEN
- `norm:sscsrind_virtual_inst_fault`: VS/VU-mode direct access to vsiselect/vsireg* or VU-mode access to siselect/sireg* triggers virtual-instruction
- `norm:vsmode_virtual_inst_fault`: When VS-mode accesses vsireg* via sireg*, if vsiselect is implemented at HS-level but not at VS-level, typically triggers virtual-instruction
- `norm:hypervisor_impl_csrs_access_control`: With H extension, hstateen0[60] controls VS/VU-mode access to siselect/sireg* (actually vsiselect/vsireg*)
- `norm:sscsrind_csrs_access_control`: mstateen0[60] controls S-mode and below access to siselect/sireg*/vsiselect/vsireg*
- `norm:csrs_alias`: M-level and S-level CSRs may be aliases

**Test Scope**: Verify Sscsrind extension behavior in Hypervisor scenarios, including VS-level CSR (vsiselect/vsireg*) basic functionality, virtual-instruction exception behavior, state-enable access control, and VS-mode transparent remapping of sireg* to vsireg*.

> **Note**: This test group is extracted from `Sscsrind_test_plan.md` Groups 2, 3, 4.2, 4.3, and 5, specifically targeting test cases that depend on the H extension. Both H extension and Sscsrind extension must be available simultaneously.

### Test ID Mapping Table

#### 11.1 VS-level CSR Basic Functionality (migrated from Sscsrind Group 2)

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| SSCSRIND-VSCSR-01 | HCROSS-SSCSRIND-01 | vsiselect readable in HS-mode |
| SSCSRIND-VSCSR-02 | HCROSS-SSCSRIND-02 | vsiselect writable in HS-mode |
| SSCSRIND-VSCSR-03 | HCROSS-SSCSRIND-03 | vsiselect minimum range verification (0..0xFFF) |
| SSCSRIND-VSCSR-04 | HCROSS-SSCSRIND-04 | vsiselect MSB=1 custom region |
| SSCSRIND-VSCSR-05 | HCROSS-SSCSRIND-05 | vsiselect MSB=0 standard reserved region |
| SSCSRIND-VSCSR-06 | HCROSS-SSCSRIND-06 | vsireg accessible in HS-mode |
| SSCSRIND-VSCSR-07 | HCROSS-SSCSRIND-07 | vsireg2~vsireg6 accessible in HS-mode |
| SSCSRIND-VSCSR-08 | HCROSS-SSCSRIND-08 | vsiselect/vsireg* width = current XLEN |
| SSCSRIND-VSCSR-09 | HCROSS-SSCSRIND-09 | vsiselect WARL all-1s write |
| SSCSRIND-VSCSR-10 | HCROSS-SSCSRIND-10 | vsireg* behavior with legal vsiselect |

#### 11.2 Virtual-Instruction Exception Behavior (migrated from Sscsrind Group 3)

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| SSCSRIND-VI-01 | HCROSS-SSCSRIND-11 | VS-mode direct read of vsiselect triggers virtual-inst |
| SSCSRIND-VI-02 | HCROSS-SSCSRIND-12 | VS-mode direct write of vsiselect triggers virtual-inst |
| SSCSRIND-VI-03 | HCROSS-SSCSRIND-13 | VS-mode direct read of vsireg triggers virtual-inst |
| SSCSRIND-VI-04 | HCROSS-SSCSRIND-14 | VS-mode direct read of vsireg2~vsireg6 triggers virtual-inst |
| SSCSRIND-VI-05 | HCROSS-SSCSRIND-15 | VS-mode direct write of vsireg triggers virtual-inst |
| SSCSRIND-VI-06 | HCROSS-SSCSRIND-16 | VU-mode direct read of vsiselect triggers virtual-inst |
| SSCSRIND-VI-07 | HCROSS-SSCSRIND-17 | VU-mode direct read of vsireg triggers virtual-inst |
| SSCSRIND-VI-08 | HCROSS-SSCSRIND-18 | VU-mode read of siselect triggers virtual-inst |
| SSCSRIND-VI-09 | HCROSS-SSCSRIND-19 | VU-mode read of sireg triggers virtual-inst |
| SSCSRIND-VI-10 | HCROSS-SSCSRIND-20 | VU-mode write of siselect triggers virtual-inst |
| SSCSRIND-VI-11 | HCROSS-SSCSRIND-21 | VS-mode access via sireg* when vsiselect implemented at HS-level but not VS-level |

#### 11.3 State-Enable Access Control (migrated from Sscsrind Group 4.2/4.3)

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| SSCSRIND-STA-05 | HCROSS-SSCSRIND-22 | mstateen0[60]=0 blocks HS-mode read of vsiselect |
| SSCSRIND-STA-06 | HCROSS-SSCSRIND-23 | mstateen0[60]=0 blocks HS-mode read of vsireg* |
| SSCSRIND-STA-07 | HCROSS-SSCSRIND-24 | hstateen0[60]=0 + mstateen0[60]=1 triggers virtual-inst on VS-mode access to siselect |
| SSCSRIND-STA-08 | HCROSS-SSCSRIND-25 | hstateen0[60]=0 + mstateen0[60]=1 triggers virtual-inst on VS-mode access to sireg |
| SSCSRIND-STA-09 | HCROSS-SSCSRIND-26 | hstateen0[60]=1 + mstateen0[60]=1 allows VS-mode access |
| SSCSRIND-STA-10 | HCROSS-SSCSRIND-27 | Exception type is virtual-inst not illegal-inst when hstateen0[60]=0 |

#### 11.4 Hypervisor Cross Tests (migrated from Sscsrind Group 5)

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| SSCSRIND-HYP-01 | HCROSS-SSCSRIND-28 | VS-mode transparent remapping of vsireg* via sireg* |
| SSCSRIND-HYP-02 | HCROSS-SSCSRIND-29 | VS-mode transparent remapping of vsiselect via siselect |
| SSCSRIND-HYP-03 | HCROSS-SSCSRIND-30 | vsireg* read/write correctness with legal vsiselect |
| SSCSRIND-HYP-04 | HCROSS-SSCSRIND-31 | vsireg* behavior with unimplemented vsiselect |
| SSCSRIND-HYP-05 | HCROSS-SSCSRIND-32 | HS-mode and VS-mode select spaces are independent |
| SSCSRIND-HYP-06 | HCROSS-SSCSRIND-33 | M-mode and S-mode select spaces may be aliases |

### Test Case List

#### 11.1 VS-level CSR Basic Functionality

**Specification References**: `norm:vsiselect_min_range`, `norm:vsiselect_msb_op`, `norm:vsireg_access_on_legal_vsiselect`, `norm:vsireg_access_behaviour`, `norm:sscsrind_vsmode_csrs_sz`

| Test ID | Test Name | Test Description | Expected Result | Norm Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCSRIND-01 | vsiselect readable in HS-mode | HS-mode reads vsiselect (0x250) | No exception, returns a legal value | `norm:vsiselect_min_range` |
| HCROSS-SSCSRIND-02 | vsiselect writable in HS-mode | HS-mode writes vsiselect then reads back | No exception, write 0 reads back 0 | `norm:vsiselect_min_range` |
| HCROSS-SSCSRIND-03 | vsiselect minimum range verification (0..0xFFF) | HS-mode writes vsiselect=0, 1, 0x100, 0x800, 0xFFF one by one then reads back | All values accepted (WARL), reads back legal values | `norm:vsiselect_min_range` |
| HCROSS-SSCSRIND-04 | vsiselect MSB=1 custom region | HS-mode writes vsiselect with an MSB=1 value | WARL behavior: no exception, reads back a legal value | `norm:vsiselect_msb_op` |
| HCROSS-SSCSRIND-05 | vsiselect MSB=0 standard reserved region | HS-mode writes vsiselect with an MSB=0 unallocated value | WARL behavior: no exception, reads back a legal value | `norm:vsiselect_msb_op` |
| HCROSS-SSCSRIND-06 | vsireg accessible in HS-mode | HS-mode reads vsireg (0x251) (vsiselect set to 0) | Behavior UNSPECIFIED: expected illegal-instruction or read-only 0 | `norm:vsireg_access_behaviour` |
| HCROSS-SSCSRIND-07 | vsireg2~vsireg6 accessible in HS-mode | HS-mode reads vsireg2~vsireg6 one by one (vsiselect set to 0) | Behavior UNSPECIFIED: similar to vsireg | `norm:vsireg_access_behaviour` |
| HCROSS-SSCSRIND-08 | vsiselect/vsireg* width = current XLEN | Verify vsiselect width in HS-mode (XLEN=64) and VS-mode (possibly XLEN=32) separately | Width is always the current XLEN | `norm:sscsrind_vsmode_csrs_sz` |
| HCROSS-SSCSRIND-09 | vsiselect WARL all-1s write | HS-mode writes all 1s to vsiselect | No exception triggered, reads back a legal value | `norm:vsiselect_min_range` |
| HCROSS-SSCSRIND-10 | vsireg* behavior with legal vsiselect | If a realized vsiselect value range exists, HS-mode sets that value then reads vsireg | Behavior defined by the corresponding extension | `norm:vsireg_access_on_legal_vsiselect` |

#### 11.2 Virtual-Instruction Exception Behavior

**Specification References**: `norm:sscsrind_virtual_inst_fault`, `norm:vsmode_virtual_inst_fault`

| Test ID | Test Name | Test Description | Expected Result | Norm Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCSRIND-11 | VS-mode direct read of vsiselect triggers virtual-inst | VS-mode reads vsiselect (0x250) | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-12 | VS-mode direct write of vsiselect triggers virtual-inst | VS-mode writes vsiselect | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-13 | VS-mode direct read of vsireg triggers virtual-inst | VS-mode reads vsireg (0x251) | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-14 | VS-mode direct read of vsireg2~vsireg6 triggers virtual-inst | VS-mode reads vsireg2~vsireg6 one by one | Each triggers virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-15 | VS-mode direct write of vsireg triggers virtual-inst | VS-mode writes vsireg | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-16 | VU-mode direct read of vsiselect triggers virtual-inst | VU-mode reads vsiselect (0x250) | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-17 | VU-mode direct read of vsireg triggers virtual-inst | VU-mode reads vsireg (0x251) | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-18 | VU-mode read of siselect triggers virtual-inst | VU-mode reads siselect (0x150) | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-19 | VU-mode read of sireg triggers virtual-inst | VU-mode reads sireg (0x151) | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-20 | VU-mode write of siselect triggers virtual-inst | VU-mode writes siselect | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-21 | VS-mode access via sireg* when vsiselect implemented at HS-level but not VS-level | HS-mode sets vsiselect to a value implemented at HS-level but not VS-level, VS-mode accesses via sireg* | virtual-instruction exception (cause=22) | `norm:vsmode_virtual_inst_fault` |

#### 11.3 State-Enable Access Control

**Specification References**: `norm:sscsrind_csrs_access_control`, `norm:hypervisor_impl_csrs_access_control`

| Test ID | Test Name | Test Description | Expected Result | Norm Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCSRIND-22 | mstateen0[60]=0 blocks HS-mode read of vsiselect | mstateen0[60]=0, HS-mode (S-mode with V=0) reads vsiselect | illegal-instruction exception (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SSCSRIND-23 | mstateen0[60]=0 blocks HS-mode read of vsireg* | mstateen0[60]=0, HS-mode reads vsireg | illegal-instruction exception (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SSCSRIND-24 | hstateen0[60]=0 + mstateen0[60]=1 triggers virtual-inst on VS-mode access to siselect | hstateen0[60]=0 and mstateen0[60]=1, VS-mode reads siselect (actually vsiselect) | virtual-instruction exception (cause=22), not illegal-instruction | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SSCSRIND-25 | hstateen0[60]=0 + mstateen0[60]=1 triggers virtual-inst on VS-mode access to sireg | hstateen0[60]=0 and mstateen0[60]=1, VS-mode reads sireg (actually vsireg) | virtual-instruction exception (cause=22) | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SSCSRIND-26 | hstateen0[60]=1 + mstateen0[60]=1 allows VS-mode access | hstateen0[60]=1 and mstateen0[60]=1, VS-mode reads siselect/sireg* | Access succeeds (subject to vsiselect value constraints) | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SSCSRIND-27 | Exception type is virtual-inst not illegal-inst when hstateen0[60]=0 | hstateen0[60]=0 and mstateen0[60]=1, VS-mode accesses siselect | Exception cause must be 22 (virtual-inst), not 2 (illegal-inst) | `norm:hypervisor_impl_csrs_access_control` |

#### 11.4 Hypervisor Cross Tests

**Specification References**: `norm:vsireg_access_on_legal_vsiselect`, `norm:vsireg_access_behaviour`, `norm:csrs_alias`

| Test ID | Test Name | Test Description | Expected Result | Norm Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCSRIND-28 | VS-mode transparent remapping of vsireg* via sireg* | hstateen0[60]=1, mstateen0[60]=1, HS-mode writes a value to vsireg, VS-mode reads back via sireg (address 0x151) | VS-mode reads the same value that HS-mode wrote to vsireg (hardware remaps sireg to vsireg) | `norm:vsireg_access_on_legal_vsiselect` |
| HCROSS-SSCSRIND-29 | VS-mode transparent remapping of vsiselect via siselect | hstateen0[60]=1, mstateen0[60]=1, HS-mode writes vsiselect=0x100, VS-mode reads back via siselect (address 0x150) | VS-mode reads the same value that HS-mode wrote to vsiselect | `norm:vsireg_access_on_legal_vsiselect` |
| HCROSS-SSCSRIND-30 | vsireg* read/write correctness with legal vsiselect | HS-mode sets vsiselect to a legal value defined by an implemented extension, writes vsireg and reads back | Read-back value matches written value | `norm:vsireg_access_on_legal_vsiselect` |
| HCROSS-SSCSRIND-31 | vsireg* behavior with unimplemented vsiselect | HS-mode sets vsiselect to an unimplemented value (e.g., reserved range), reads vsireg | Behavior UNSPECIFIED: expected illegal-instruction or read-only 0 | `norm:vsireg_access_behaviour` |
| HCROSS-SSCSRIND-32 | HS-mode and VS-mode select spaces are independent | HS-mode writes vsiselect=0x100, VS-mode reads 0x100 via siselect; VS-mode writes siselect=0x200 (triggers virtual-inst or is remapped), HS-mode reads vsiselect | vsiselect value is controlled by HS-mode; VS-mode sees the current vsiselect value via siselect | `norm:vsireg_access_on_legal_vsiselect` |
| HCROSS-SSCSRIND-33 | M-mode and S-mode select spaces may be aliases | If a dependent extension defines M-level and S-level aliasing, access the same select value via miselect+sireg and siselect+sireg | M-level and S-level CSRs with the same select value may access the same or partially overlapping register state | `norm:csrs_alias` |

> [!NOTE]
> - This test group validates the Sscsrind extension behavior in Hypervisor scenarios. All tests must detect H extension availability at runtime via `HAS_H_EXT()` and TEST_SKIP if unavailable.
> - HCROSS-SSCSRIND-01~10 are migrated from `Sscsrind_test_plan.md` Group 2, verifying vsiselect/vsireg* basic functionality. The vsiselect minimum range 0..0xFFF is consistent with siselect, ensuring the hypervisor can emulate indirect register access within a VM.
> - HCROSS-SSCSRIND-11~21 are migrated from `Sscsrind_test_plan.md` Group 3, verifying virtual-instruction exception behavior. **Key distinction**: VS/VU-mode direct access to vsiselect/vsireg* always triggers virtual-instruction (cause=22), **regardless of** mstateen0[60] or hstateen0[60] values. This is an explicit requirement of `norm:sscsrind_virtual_inst_fault` in the Sscsrind SPEC.
> - HCROSS-SSCSRIND-22~27 are migrated from `Sscsrind_test_plan.md` Group 4.2/4.3, verifying state-enable access control. Key distinction: when mstateen0[60]=1 but hstateen0[60]=0, VS/VU-mode access to siselect/sireg* triggers **virtual-instruction** (cause=22), not illegal-instruction (cause=2). This is because M-mode has permitted access (mstateen=1), but HS-mode's hypervisor has chosen not to permit it (hstateen=0), so the exception type reflects that the hypervisor needs to trap and handle it.
> - HCROSS-SSCSRIND-28~33 are migrated from `Sscsrind_test_plan.md` Group 5, verifying Hypervisor cross tests. HCROSS-SSCSRIND-28~29 verify the core hardware transparent remapping behavior: within a VM, when VS-mode accesses siselect/sireg* (addresses 0x150~0x157), the hardware automatically remaps them to vsiselect/vsireg* (addresses 0x250~0x257). This is transparent to the guest OS.
> - Distinction from Group 8.4 (HCROSS-SSSTA-27~29) and Group 10 (HCROSS-SMCSRIND-01~08): Group 8.4 verifies hstateen0[60] control from the Smstateen perspective, Group 10 verifies mstateen0[60] control over HS-mode access to vsiselect/vsireg*, and this group verifies hstateen0[60] control and VS/VU-mode exception behavior from the Sscsrind perspective. Implementations should avoid duplication and may use cross-references.

---

## Group 12. Hypervisor x Ssdbltrp Cross Tests

**Specification References**:
- `norm:henvcfg_DTE`: If H extension is implemented, add henvcfg.DTE field
- `norm:henvcfg_dte_op`: When henvcfg.DTE=0, VS-mode behaves as if Ssdbltrp is not implemented; vsstatus.SDT is read-only zero
- `norm:menvcfg_dte_op`: When menvcfg.DTE=0, henvcfg.DTE is read-only zero
- `norm:vsstatus_SDT`: If H extension is implemented, add vsstatus.SDT field
- `norm:vsstatus_sdt_op`: vsstatus.SDT is used for handling VS-mode double traps
- `norm:sstatus_sdt_trap`: SDT mechanism applies equally in VS-mode (via vsstatus.SDT)
- `norm:sstatus_sdt_sstatus_sie_overwrite`: Mutual exclusion constraint between vsstatus.SDT and vsstatus.SIE
- `norm:sret_dt`: HS-mode SRET to VU clears vsstatus.SDT; VS-mode SRET clears vsstatus.SDT
- `norm:vsstatus_sdt_clr_mret_sret`: MRET or M-mode SRET with new mode VU clears vsstatus.SDT
- `norm:vsstatus_sdt_clr_mnret`: MNRET with new mode VU clears vsstatus.SDT
- `norm:sstatus_sdt_clr_mret_sret`: MRET or M-mode SRET with new mode VS/VU clears sstatus.SDT
- `norm:HS_mode_invoke_error`: HS-mode can invoke a critical error handler in the VM during VS-mode double trap

**Test Scope**: Verify Ssdbltrp extension behavior in Hypervisor scenarios, including `henvcfg.DTE` enable/disable control for VS-mode, `vsstatus.SDT` field behavior, SRET clearing of `vsstatus.SDT`, and cross-mode SDT/vsstatus.SDT clearing by MRET/SRET/MNRET in Hypervisor scenarios.

> **Note**: This test group is extracted from `Ssdbltrp_test_plan.md` Groups 3/4/5/6/7, specifically targeting test cases that depend on the H extension. Both H extension and Ssdbltrp extension must be available simultaneously.

### Test ID Mapping Table

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| HDTE-01 | HCROSS-SSDBLTRP-01 | henvcfg.DTE read-write |
| HDTE-02 | HCROSS-SSDBLTRP-02 | vsstatus.SDT read-only zero when henvcfg.DTE=0 |
| HDTE-03 | HCROSS-SSDBLTRP-03 | vsstatus.SDT writable when henvcfg.DTE=1 |
| HDTE-04 | HCROSS-SSDBLTRP-04 | VS-mode trap does not set SDT when henvcfg.DTE=0 |
| HDTE-05 | HCROSS-SSDBLTRP-05 | menvcfg.DTE=0 overrides henvcfg.DTE |
| HDTE-06 | HCROSS-SSDBLTRP-06 | henvcfg.DTE dynamic switching |
| VSDT-01 | HCROSS-SSDBLTRP-07 | vsstatus.SDT WARL read-write |
| VSDT-02 | HCROSS-SSDBLTRP-08 | vsstatus.SDT=1 auto-clears vsstatus.SIE on write |
| VSDT-03 | HCROSS-SSDBLTRP-09 | Cannot set vsstatus.SIE=1 when vsstatus.SDT=1 |
| VSDT-04 | HCROSS-SSDBLTRP-10 | vsstatus.SDT auto-set to 1 on VS-mode trap |
| VSDT-05 | HCROSS-SSDBLTRP-11 | Trap triggers double-trap when VS-mode SDT=1 |
| VSDT-06 | HCROSS-SSDBLTRP-12 | M-mode CSRs correct during VS-mode double-trap |
| SRET-03 | HCROSS-SSDBLTRP-13 | HS-mode SRET to VU clears vsstatus.SDT |
| SRET-04 | HCROSS-SSDBLTRP-14 | VS-mode SRET clears vsstatus.SDT |
| SRET-05 | HCROSS-SSDBLTRP-15 | HS-mode SRET to VS does not clear vsstatus.SDT |
| SRET-06 | HCROSS-SSDBLTRP-16 | HS-mode SRET to HS does not clear vsstatus.SDT |
| DTE-03 | HCROSS-SSDBLTRP-17 | vsstatus.SDT read-only zero when menvcfg.DTE=0 |
| DTE-04 | HCROSS-SSDBLTRP-18 | henvcfg.DTE read-only zero when menvcfg.DTE=0 |
| XRET-03 | HCROSS-SSDBLTRP-19 | MRET to VS-mode clears sstatus.SDT |
| XRET-04 | HCROSS-SSDBLTRP-20 | MRET to VU-mode clears sstatus.SDT and vsstatus.SDT |
| XRET-05 | HCROSS-SSDBLTRP-21 | MRET to VU only clears vsstatus.SDT |
| XRET-06 | HCROSS-SSDBLTRP-22 | MRET to VS-mode does not clear vsstatus.SDT |
| XRET-08 | HCROSS-SSDBLTRP-23 | M-mode SRET to VU clears sstatus.SDT and vsstatus.SDT |
| XRET-10 | HCROSS-SSDBLTRP-24 | MNRET to VU clears sstatus.SDT and vsstatus.SDT |

### Test Case List

#### 12.1 henvcfg.DTE Control

**Specification Basis**: `norm:henvcfg_DTE`, `norm:henvcfg_dte_op`, `norm:menvcfg_dte_op`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSDBLTRP-01 | henvcfg.DTE read-write | HS-mode writes henvcfg.DTE=1 and reads back for verification (menvcfg.DTE=1) | If both Ssdbltrp and H extensions are implemented, DTE is writable and read-back matches | `norm:henvcfg_DTE` |
| HCROSS-SSDBLTRP-02 | vsstatus.SDT read-only zero when henvcfg.DTE=0 | henvcfg.DTE=0 (menvcfg.DTE=1), VS-mode attempts to write vsstatus.SDT=1 | vsstatus.SDT reads back 0 (read-only zero) | `norm:henvcfg_dte_op` |
| HCROSS-SSDBLTRP-03 | vsstatus.SDT writable when henvcfg.DTE=1 | henvcfg.DTE=1 (menvcfg.DTE=1), write vsstatus.SDT=1 | vsstatus.SDT=1 is writable | `norm:henvcfg_dte_op` |
| HCROSS-SSDBLTRP-04 | VS-mode trap does not set SDT when henvcfg.DTE=0 | henvcfg.DTE=0, VS-mode triggers ecall normal trap | vsstatus.SDT is not modified by hardware (feature not present) | `norm:henvcfg_dte_op` |
| HCROSS-SSDBLTRP-05 | menvcfg.DTE=0 overrides henvcfg.DTE | menvcfg.DTE=0, attempt to write henvcfg.DTE=1 | henvcfg.DTE is read-only zero (menvcfg.DTE is the global enable) | `norm:menvcfg_dte_op` |
| HCROSS-SSDBLTRP-06 | henvcfg.DTE dynamic switching | menvcfg.DTE=1, first set henvcfg.DTE=1 and verify vsstatus.SDT is writable; then set henvcfg.DTE=0 and verify read-only zero | Writable when DTE=1; read-only zero after DTE=0 | `norm:henvcfg_dte_op` |

#### 12.2 vsstatus.SDT (VS-mode)

**Specification Basis**: `norm:vsstatus_SDT`, `norm:vsstatus_sdt_op`, `norm:sstatus_sdt_trap`, `norm:sstatus_sdt_sstatus_sie_overwrite`, `norm:HS_mode_invoke_error`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSDBLTRP-07 | vsstatus.SDT WARL read-write | HS-mode writes vsstatus.SDT=1 (henvcfg.DTE=1, menvcfg.DTE=1) and reads back | vsstatus.SDT is writable and read-back matches | `norm:vsstatus_SDT` |
| HCROSS-SSDBLTRP-08 | Writing vsstatus.SDT=1 auto-clears vsstatus.SIE | Write vsstatus.SDT=1 (simultaneously writing SIE=1) | vsstatus.SIE is forced to zero | `norm:sstatus_sdt_sstatus_sie_overwrite` |
| HCROSS-SSDBLTRP-09 | Cannot set vsstatus.SIE=1 when vsstatus.SDT=1 | Set vsstatus.SDT=1, then write vsstatus.SIE=1 | vsstatus.SIE reads back as 0 | `norm:sstatus_sdt_sstatus_sie_overwrite` |
| HCROSS-SSDBLTRP-10 | vsstatus.SDT auto-set to 1 on VS-mode trap | henvcfg.DTE=1, set vsstatus.SDT=0, trigger ecall trap from VU-mode to VS-mode | vsstatus.SDT=1 | `norm:sstatus_sdt_trap` |
| HCROSS-SSDBLTRP-11 | Trap triggers double-trap when VS-mode SDT=1 | henvcfg.DTE=1, set vsstatus.SDT=1, VU-mode triggers ecall (delegated to VS-mode) | Double-trap exception delivered to M-mode (mcause=16), mtval2=8 (ecall-from-VU) | `norm:sstatus_sdt_trap` |
| HCROSS-SSDBLTRP-12 | M-mode CSRs correct on VS-mode double-trap | VS-mode double-trap to M-mode | mstatus.MPV=1, MPP correct, mepc=trigger address, mcause=16, mtval2=original cause | `norm:sstatus_sdt_trap` |

#### 12.3 SRET Clearing of vsstatus.SDT

**Specification Basis**: `norm:sret_dt`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSDBLTRP-13 | HS-mode SRET to VU clears vsstatus.SDT | HS-mode sets vsstatus.SDT=1, configure SRET to return to VU-mode (SPV=1, SPP=0) | vsstatus.SDT=0 | `norm:sret_dt` |
| HCROSS-SSDBLTRP-14 | VS-mode SRET clears vsstatus.SDT | VS-mode sets vsstatus.SDT=1, execute SRET | vsstatus.SDT=0 | `norm:sret_dt` |
| HCROSS-SSDBLTRP-15 | HS-mode SRET to VS does not clear vsstatus.SDT | HS-mode sets vsstatus.SDT=1, configure SRET to return to VS-mode (SPV=1, SPP=1) | vsstatus.SDT remains 1 (vsstatus.SDT cleared only when returning to VU-mode) | `norm:sret_dt` |
| HCROSS-SSDBLTRP-16 | HS-mode SRET to HS does not clear vsstatus.SDT | HS-mode sets vsstatus.SDT=1, configure SRET to return to HS-mode (SPV=0) | vsstatus.SDT remains 1 | `norm:sret_dt` |

#### 12.4 menvcfg.DTE Control of Hypervisor CSRs

**Specification Basis**: `norm:menvcfg_dte_op`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSDBLTRP-17 | vsstatus.SDT read-only zero when menvcfg.DTE=0 | menvcfg.DTE=0, attempt to write vsstatus.SDT=1 (when H extension is present) | vsstatus.SDT reads back 0 (read-only zero) | `norm:menvcfg_dte_op` |
| HCROSS-SSDBLTRP-18 | henvcfg.DTE read-only zero when menvcfg.DTE=0 | menvcfg.DTE=0, attempt to write henvcfg.DTE=1 (when H extension is present) | henvcfg.DTE reads back 0 (read-only zero) | `norm:menvcfg_dte_op` |

#### 12.5 MRET/SRET/MNRET Cross-Mode Clearing (Hypervisor Scenarios)

**Specification Basis**: `norm:sstatus_sdt_clr_mret_sret`, `norm:vsstatus_sdt_clr_mret_sret`, `norm:vsstatus_sdt_clr_mnret`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSDBLTRP-19 | MRET to VS-mode clears sstatus.SDT | Set sstatus.SDT=1, mstatus.MPP=1(S), MPV=1, execute MRET | sstatus.SDT=0 | `norm:sstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-20 | MRET to VU-mode clears sstatus.SDT and vsstatus.SDT | Set sstatus.SDT=1, vsstatus.SDT=1, MPP=0(U), MPV=1, execute MRET | sstatus.SDT=0 and vsstatus.SDT=0 | `norm:sstatus_sdt_clr_mret_sret` `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-21 | MRET to VU only clears vsstatus.SDT | Set sstatus.SDT=0, vsstatus.SDT=1, MPP=0(U), MPV=1, execute MRET | vsstatus.SDT=0 | `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-22 | MRET to VS-mode does not clear vsstatus.SDT | Set vsstatus.SDT=1, MPP=1(S), MPV=1, execute MRET | vsstatus.SDT remains 1 (vsstatus.SDT cleared only for VU) | `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-23 | M-mode SRET to VU clears sstatus.SDT and vsstatus.SDT | M-mode sets SDT=1, vsstatus.SDT=1, MPP=0, SPV=1, execute SRET | sstatus.SDT=0 and vsstatus.SDT=0 | `norm:sstatus_sdt_clr_mret_sret` `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-24 | MNRET to VU clears sstatus.SDT and vsstatus.SDT | Set SDT=1, vsstatus.SDT=1, MNPP=0(U), MNPV=1, execute MNRET | sstatus.SDT=0 and vsstatus.SDT=0 | `norm:vsstatus_sdt_clr_mnret` |

### Code Examples

> [!NOTE]
> - This test group validates Ssdbltrp extension behavior in Hypervisor scenarios. All tests must detect H extension availability at runtime via `HAS_H_EXT()` and TEST_SKIP when unavailable.
> - HCROSS-SSDBLTRP-01~06 are migrated from `Ssdbltrp_test_plan.md` Group 5, validating `henvcfg.DTE` writability and the enable/disable control for VS-mode Ssdbltrp functionality. `menvcfg.DTE` is the global enable, and `henvcfg.DTE` is the VS-mode level enable.
> - HCROSS-SSDBLTRP-07~12 are migrated from `Ssdbltrp_test_plan.md` Group 6, validating `vsstatus.SDT` WARL read-write, SDT/SIE mutual exclusion, and the VS-mode double-trap delivery mechanism. VS-mode double-trap is ultimately delivered to M-mode (mcause=16), with `mstatus.MPV=1` identifying the virtualization mode origin.
> - HCROSS-SSDBLTRP-13~16 are migrated from `Ssdbltrp_test_plan.md` Group 3, validating SRET clearing behavior for `vsstatus.SDT`. Key rule: HS-mode SRET **clears vsstatus.SDT only when returning to VU-mode**; returning to VS-mode or HS-mode does not clear it. VS-mode's own SRET always clears vsstatus.SDT.
> - HCROSS-SSDBLTRP-17~18 are migrated from `Ssdbltrp_test_plan.md` Group 4, validating the global disable effect on Hypervisor CSRs (vsstatus.SDT, henvcfg.DTE) when `menvcfg.DTE=0`.
> - HCROSS-SSDBLTRP-19~24 are migrated from `Ssdbltrp_test_plan.md` Group 7, validating cross-mode SDT clearing for MRET/SRET/MNRET in Hypervisor scenarios. Core rule: sstatus.SDT is cleared when the new mode is U/VS/VU; vsstatus.SDT is **cleared only when the new mode is VU**.
> - Distinction from Group 6 (HCROSS-SVNAPOT) and Group 9 (HCROSS-SSTC): this group focuses on double-trap mechanism behavior in virtualization scenarios, not address translation or timer functionality.

---

## Group 13. Hypervisor x Smctr Cross-Tests

**Specification Basis**:
- `norm:hstateen_ctr`: If the H extension is implemented and `mstateen0.CTR=1`, the `hstateen0.CTR` bit controls access to supervisor CTR state when V=1; when `mstateen0.CTR=0`, `hstateen0.CTR` is read-only zero
- `norm:hstateen_vs`: When `hstateen0.CTR=0`, VS-mode access to CTR state and SCTRCLR triggers a virtual-instruction exception
- `norm:hstateen0_CTR0-V1_op`: When `hstateen0.CTR=0`, qualified control transfers during V=1 still implicitly update entry registers and `sctrstatus`
- `norm:mstateen_ctr0_execpt1`: `mstateen0.CTR=0` blocks access to `vsctrctl`
- `norm:exttrap_vsm`: External traps from VS-mode to M-mode require MTE + STE
- `norm:exttrap_vum`: External traps from VU-mode to M-mode require MTE + STE + vsctrctl.STE
- `norm:exttrap_implreq`: If the H extension is implemented, `vsctrctl.STE` must be implemented

**Test Scope**: Validate Smctr extension behavior in Hypervisor scenarios, including `hstateen0.CTR` control of VS-mode CTR state access, `mstateen0.CTR=0` blocking of `vsctrctl`, and MTE external trap recording from VS/VU-mode to M-mode.

> **Note**: This test group is extracted from `Smctr_test_plan.md` Groups 2/3, specifically targeting test cases that depend on the H extension. Both the H extension and Smctr extension must be available simultaneously.

### Test ID Mapping Table

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| SMCTR-STA-05 | HCROSS-SMCTR-01 | mstateen0.CTR=0 blocks S-mode access to vsctrctl |
| SMCTR-STA-11 | HCROSS-SMCTR-02 | hstateen0.CTR read-write verification |
| SMCTR-STA-12 | HCROSS-SMCTR-03 | hstateen0.CTR read-only zero (mstateen0.CTR=0) |
| SMCTR-STA-13 | HCROSS-SMCTR-04 | hstateen0.CTR=0 blocks VS-mode access to sctrctl |
| SMCTR-STA-14 | HCROSS-SMCTR-05 | hstateen0.CTR=0 blocks VS-mode access to sctrstatus |
| SMCTR-STA-15 | HCROSS-SMCTR-06 | hstateen0.CTR=0 blocks VS-mode access to sireg* |
| SMCTR-STA-16 | HCROSS-SMCTR-07 | hstateen0.CTR=0 blocks VS-mode execution of SCTRCLR |
| SMCTR-STA-17 | HCROSS-SMCTR-08 | hstateen0.CTR=1 allows full VS-mode access |
| SMCTR-STA-18 | HCROSS-SMCTR-09 | Implicit updates continue when hstateen0.CTR=0 |
| SMCTR-MODE-07 | HCROSS-SMCTR-10 | MTE external trap recording (VS to M) |
| SMCTR-MODE-08 | HCROSS-SMCTR-11 | MTE external trap recording (VU to M, requires MTE+STE+vsSTE) |
| SMCTR-MODE-09 | HCROSS-SMCTR-12 | External trap VU to M not recorded when vsSTE is missing |

### Test Case List

#### 13.1 mstateen0.CTR Control of Hypervisor CSRs

**Specification Basis**: `norm:mstateen_ctr0_execpt1`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SMCTR-01 | mstateen0.CTR=0 blocks S-mode access to vsctrctl | mstateen0.CTR=0, HS-mode attempts to read vsctrctl | Triggers illegal-instruction exception (cause=2) | `norm:mstateen_ctr0_execpt1` |

#### 13.2 hstateen0.CTR Access Control

**Specification Basis**: `norm:hstateen_ctr`, `norm:hstateen_vs`, `norm:hstateen0_CTR0-V1_op`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SMCTR-02 | hstateen0.CTR read-write verification | mstateen0.CTR=1, M-mode writes hstateen0.CTR=1 and reads back | If the H extension is implemented and mstateen0.CTR=1, hstateen0.CTR is writable | `norm:hstateen_ctr` |
| HCROSS-SMCTR-03 | hstateen0.CTR read-only zero (mstateen0.CTR=0) | mstateen0.CTR=0, attempt to write hstateen0.CTR=1 | hstateen0.CTR is read-only zero | `norm:hstateen_ctr` |
| HCROSS-SMCTR-04 | hstateen0.CTR=0 blocks VS-mode access to sctrctl | mstateen0.CTR=1, hstateen0.CTR=0, VS-mode accesses sctrctl (actually vsctrctl) | Triggers virtual-instruction exception (cause=22) | `norm:hstateen_vs` |
| HCROSS-SMCTR-05 | hstateen0.CTR=0 blocks VS-mode access to sctrstatus | mstateen0.CTR=1, hstateen0.CTR=0, VS-mode accesses sctrstatus | Triggers virtual-instruction exception (cause=22) | `norm:hstateen_vs` |
| HCROSS-SMCTR-06 | hstateen0.CTR=0 blocks VS-mode access to sireg* | mstateen0.CTR=1, hstateen0.CTR=0, VS-mode sets siselect=0x200 and reads sireg | Triggers virtual-instruction exception (cause=22) | `norm:hstateen_vs` |
| HCROSS-SMCTR-07 | hstateen0.CTR=0 blocks VS-mode execution of SCTRCLR | mstateen0.CTR=1, hstateen0.CTR=0, VS-mode executes SCTRCLR | Triggers virtual-instruction exception (cause=22) | `norm:hstateen_vs` |
| HCROSS-SMCTR-08 | hstateen0.CTR=1 allows full VS-mode access | mstateen0.CTR=1, hstateen0.CTR=1, VS-mode accesses sctrctl/sctrstatus respectively | All accesses succeed | `norm:hstateen_vs` |
| HCROSS-SMCTR-09 | Implicit updates continue when hstateen0.CTR=0 | mstateen0.CTR=1, hstateen0.CTR=0, VS-mode enables recording, executes control transfer, M-mode checks entry registers | Entry registers and sctrstatus are still implicitly updated | `norm:hstateen0_CTR0-V1_op` |

#### 13.3 MTE External Traps (VS/VU-mode to M-mode)

**Specification Basis**: `norm:exttrap_vsm`, `norm:exttrap_vum`, `norm:exttrap_implreq`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SMCTR-10 | MTE external trap recording (VS to M) | mctrctl.M=0, MTE=1, sctrctl.STE=1, VS-mode triggers trap to M-mode | External trap is recorded (requires MTE and STE) | `norm:exttrap_vsm` |
| HCROSS-SMCTR-11 | MTE external trap recording (VU to M, requires MTE+STE+vsSTE) | mctrctl.M=0, MTE=1, sctrctl.STE=1, vsctrctl.STE=1, VU-mode triggers trap to M-mode | External trap is recorded (requires all three TE bits set) | `norm:exttrap_vum` |
| HCROSS-SMCTR-12 | External trap VU to M not recorded when vsSTE is missing | mctrctl.M=0, MTE=1, STE=1, vsctrctl.STE=0, VU-mode triggers trap to M-mode | External trap is not recorded (vsctrctl.STE not set) | `norm:exttrap_vum` |

### Code Examples

> [!NOTE]
> - This test group validates Smctr extension behavior in Hypervisor scenarios. All tests must detect H extension availability at runtime via `HAS_H_EXT()` and TEST_SKIP when unavailable.
> - HCROSS-SMCTR-01 is migrated from `Smctr_test_plan.md` Group 2, validating the blocking of `vsctrctl` (a CSR introduced by the H extension) when `mstateen0.CTR=0`. The triggered exception is illegal-instruction (cause=2), as this is M-mode control over S-mode.
> - HCROSS-SMCTR-02~09 are migrated from `Smctr_test_plan.md` Group 2, validating `hstateen0.CTR` control of VS-mode CTR state access. Core rule: when `hstateen0.CTR=0`, VS-mode access to CTR state triggers virtual-instruction (cause=22), not illegal-instruction. This differs from `mstateen0.CTR=0` which triggers illegal-instruction -- because M-mode has granted access (mstateen0=1) but HS-mode chooses not to grant it (hstateen0=0).
> - HCROSS-SMCTR-09 validates a key semantic: even when `hstateen0.CTR=0` blocks VS-mode **explicit access** to CTR CSRs, qualified control transfers executed during V=1 still **implicitly update** entry registers and `sctrstatus`. This prevents the hypervisor from interfering with guest transition recording by disabling CTR.
> - HCROSS-SMCTR-10~12 are migrated from `Smctr_test_plan.md` Group 3, validating MTE external trap recording in Hypervisor scenarios. VS to M requires MTE+STE; VU to M requires MTE+STE+vsctrctl.STE (all three TE bits). This reflects the dependency of external trap recording on intermediate mode TE bits.
> - Distinction from Group 8 (HCROSS-SMSTA): Group 8 validates SE0/ENVCFG/CSRIND and other functional bits in `hstateen0`, while this group validates the `hstateen0.CTR` bit's control of CTR state.

---

## Group 14. Hypervisor x Ssctr Cross-Tests

**Specification Basis**:
- `norm:Ssctr_vsctrctl_sz_acc_op`: If the H extension is implemented, `vsctrctl` is a 64-bit read-write register that substitutes for `sctrctl` when V=1
- `norm:vsctr-s_op`: S field enables VS-mode recording
- `norm:vsctrctl-u_op`: U field enables VU-mode recording
- `norm:vsctrctl-ste_op`: STE field enables recording of traps to VS-mode
- `norm:vsctrctl-bpfrz_op`: BPFRZ field sets FROZEN on VS-mode breakpoint
- `norm:vsctrctl-lcofifrz_op`: LCOFIFRZ field sets FROZEN on VS-mode LCOFI
- `norm:exttrap_vshs`: VS to HS external traps require `sctrctl.STE`
- `norm:exttrap_vuhs`: VU to HS external traps require `sctrctl.STE + vsctrctl.STE`
- `norm:exttrap_vuvs`: VU to VS external traps require `vsctrctl.STE`
- `norm:ctr_freeze_vs`: VS-mode freeze behavior is determined by LCOFIFRZ/BPFRZ in `vsctrctl`
- `norm:sctrdepth_mode`: VS-mode/VU-mode access to `sctrdepth` triggers virtual-instruction
- `norm:sctrclr_exceptions`: VU-mode execution of SCTRCLR triggers virtual-instruction
- `norm:vsiselect_op`: When V=1, `vsireg*` provides the same CTR entry state access as `sireg*`
- `norm:hstateen_ctr` / `norm:hstateen_vs`: `hstateen0.CTR` controls VS-mode access to CTR state

**Test Scope**: Validate Ssctr extension behavior in Hypervisor scenarios, including `vsctrctl` CSR functionality, VS/VU-mode external trap recording, virtualization mode transition configuration source, VS-mode freeze behavior, VS-mode access restrictions on sctrdepth/SCTRCLR, and `hstateen0.CTR` control of VS-mode CTR access.

> **Note**: This test group is extracted from `Ssctr_test_plan.md` Groups 2/3/5/8/12/13/14, specifically targeting test cases that depend on the H extension. Both the H extension and Ssctr extension must be available simultaneously.

### Test ID Mapping Table

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| SSCTR-VSCTL-01 | HCROSS-SSCTR-01 | vsctrctl basic read-write (HS-mode) |
| SSCTR-VSCTL-02 | HCROSS-SSCTR-02 | sctrctl actually accesses vsctrctl when V=1 |
| SSCTR-VSCTL-03 | HCROSS-SSCTR-03 | Writing sctrctl actually writes vsctrctl when V=1 |
| SSCTR-VSCTL-04 | HCROSS-SSCTR-04 | vsctrctl.S field (VS-mode recording) |
| SSCTR-VSCTL-05 | HCROSS-SSCTR-05 | vsctrctl.U field (VU-mode recording) |
| SSCTR-VSCTL-06 | HCROSS-SSCTR-06 | vsctrctl.STE field |
| SSCTR-VSCTL-07 | HCROSS-SSCTR-07 | vsctrctl.BPFRZ field |
| SSCTR-VSCTL-08 | HCROSS-SSCTR-08 | vsctrctl.LCOFIFRZ field |
| SSCTR-VSCTL-09 | HCROSS-SSCTR-09 | vsctrctl does not affect behavior when V=0 |
| SSCTR-VSCTL-10 | HCROSS-SSCTR-10 | vsctrctl fields match sctrctl |
| SSCTR-DEP-07 | HCROSS-SSCTR-11 | VS-mode access to sctrdepth triggers exception |
| SSCTR-DEP-08 | HCROSS-SSCTR-12 | VU-mode access to sctrdepth triggers exception |
| SSCTR-ENT-10 | HCROSS-SSCTR-13 | vsireg* and sireg* access the same state |
| SSCTR-ENT-18 | HCROSS-SSCTR-14 | VU-mode execution of SCTRCLR triggers exception |
| SSCTR-EXT-04 | HCROSS-SSCTR-15 | VS to HS external trap requires STE |
| SSCTR-EXT-05 | HCROSS-SSCTR-16 | VS to HS external trap not recorded when STE=0 |
| SSCTR-EXT-06 | HCROSS-SSCTR-17 | VU to VS external trap requires vsctrctl.STE |
| SSCTR-EXT-07 | HCROSS-SSCTR-18 | VU to VS external trap not recorded when vsSTE=0 |
| SSCTR-EXT-08 | HCROSS-SSCTR-19 | VU to HS external trap requires STE + vsSTE |
| SSCTR-EXT-09 | HCROSS-SSCTR-20 | VU to HS not recorded when vsSTE is missing |
| SSCTR-FRZ-09 | HCROSS-SSCTR-21 | VS-mode BPFRZ controlled by vsctrctl |
| SSCTR-FRZ-10 | HCROSS-SSCTR-22 | VS-mode BPFRZ=0 does not set FROZEN |
| SSCTR-FRZ-11 | HCROSS-SSCTR-23 | VS-mode LCOFIFRZ controlled by vsctrctl |
| SSCTR-FRZ-12 | HCROSS-SSCTR-24 | Virtual LCOFI also triggers freeze |
| SSCTR-VIRT-01 | HCROSS-SSCTR-25 | VU to HS transition uses vsctrctl filter bits |
| SSCTR-VIRT-02 | HCROSS-SSCTR-26 | VS to HS transition uses vsctrctl/sctrctl fields |
| SSCTR-VIRT-03 | HCROSS-SSCTR-27 | HS to VS/VU trap return uses sctrctl |
| SSCTR-VIRT-04 | HCROSS-SSCTR-28 | Freeze uses sctrctl (trap to HS-mode) |
| SSCTR-VIRT-05 | HCROSS-SSCTR-29 | VS-mode freeze uses vsctrctl |
| SSCTR-SEA-02 | HCROSS-SSCTR-30 | VS-mode detection of CTR accessibility |
| SSCTR-SEA-06 | HCROSS-SSCTR-31 | VS-mode sctrstatus access when hstateen0.CTR=0 |
| SSCTR-SEA-07 | HCROSS-SSCTR-32 | VS-mode SCTRCLR execution when hstateen0.CTR=0 |

### Test Case List

#### 14.1 vsctrctl CSR (VS-mode CTR Control Register)

**Specification Basis**: `norm:Ssctr_vsctrctl_sz_acc_op`, `norm:vsctr-s_op`, `norm:vsctrctl-u_op`, `norm:vsctrctl-ste_op`, `norm:vsctrctl-bpfrz_op`, `norm:vsctrctl-lcofifrz_op`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCTR-01 | vsctrctl basic read-write (HS-mode) | HS-mode writes each field via vsctrctl address and reads back | Implemented fields read back consistently | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-02 | sctrctl actually accesses vsctrctl when V=1 | HS-mode writes specific value to vsctrctl, enter VS-mode and read with csrr sctrctl | Reads vsctrctl value | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-03 | Writing sctrctl actually writes vsctrctl when V=1 | VS-mode writes value using csrw sctrctl, return to HS-mode and read with csrr vsctrctl | vsctrctl is modified | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-04 | vsctrctl.S field (VS-mode recording) | vsctrctl.S=1, VS-mode executes control transfer | Transfer is recorded | `norm:vsctr-s_op` |
| HCROSS-SSCTR-05 | vsctrctl.U field (VU-mode recording) | vsctrctl.U=1, VU-mode executes control transfer | Transfer is recorded | `norm:vsctrctl-u_op` |
| HCROSS-SSCTR-06 | vsctrctl.STE field | Write vsctrctl.STE=1 and read back | If the H extension is implemented, STE is writable | `norm:vsctrctl-ste_op` |
| HCROSS-SSCTR-07 | vsctrctl.BPFRZ field | Write vsctrctl.BPFRZ=1 and read back | BPFRZ must be implemented and writable | `norm:vsctrctl-bpfrz_op` |
| HCROSS-SSCTR-08 | vsctrctl.LCOFIFRZ field | Write vsctrctl.LCOFIFRZ=1 and read back | If Sscofpmf is implemented, LCOFIFRZ must be writable | `norm:vsctrctl-lcofifrz_op` |
| HCROSS-SSCTR-09 | vsctrctl does not affect behavior when V=0 | HS-mode writes vsctrctl.S=1, HS-mode itself executes transfer | HS-mode transfers are not affected by vsctrctl | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-10 | vsctrctl fields match sctrctl | Check whether vsctrctl optional fields are consistent with sctrctl | Optional fields implemented in vsctrctl should match sctrctl | `norm:Ssctr_vsctrctl_sz_acc_op` |

#### 14.2 VS/VU-mode Access Restrictions on CTR CSRs

**Specification Basis**: `norm:sctrdepth_mode`, `norm:sctrclr_exceptions`, `norm:vsiselect_op`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCTR-11 | VS-mode access to sctrdepth triggers exception | VS-mode attempts to access sctrdepth | Triggers virtual-instruction exception (cause=22) | `norm:sctrdepth_mode` |
| HCROSS-SSCTR-12 | VU-mode access to sctrdepth triggers exception | VU-mode attempts to access sctrdepth | Triggers virtual-instruction exception (cause=22) | `norm:sctrdepth_mode` |
| HCROSS-SSCTR-13 | vsireg* and sireg* access the same state | HS-mode sets vsiselect=0x200, writes value via vsireg; then sets siselect=0x200, reads via sireg | Reads the same value (shared entry register state) | `norm:vsiselect_op` |
| HCROSS-SSCTR-14 | VU-mode execution of SCTRCLR triggers exception | VU-mode executes SCTRCLR | Triggers virtual-instruction exception (cause=22) | `norm:sctrclr_exceptions` |

#### 14.3 VS/VU-mode External Trap Recording

**Specification Basis**: `norm:exttrap_vshs`, `norm:exttrap_vuhs`, `norm:exttrap_vuvs`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCTR-15 | VS to HS external trap requires STE | sctrctl.S=0, vsctrctl.S=1, sctrctl.STE=1, VS-mode triggers trap to HS-mode | External trap is recorded | `norm:exttrap_vshs` |
| HCROSS-SSCTR-16 | VS to HS external trap not recorded when STE=0 | sctrctl.S=0, vsctrctl.S=1, sctrctl.STE=0, VS-mode triggers trap to HS-mode | External trap is not recorded | `norm:exttrap_vshs` |
| HCROSS-SSCTR-17 | VU to VS external trap requires vsctrctl.STE | vsctrctl.S=0, vsctrctl.U=1, vsctrctl.STE=1, VU-mode triggers trap to VS-mode | External trap is recorded | `norm:exttrap_vuvs` |
| HCROSS-SSCTR-18 | VU to VS external trap not recorded when vsSTE=0 | vsctrctl.S=0, vsctrctl.U=1, vsctrctl.STE=0, VU-mode triggers trap to VS-mode | External trap is not recorded | `norm:exttrap_vuvs` |
| HCROSS-SSCTR-19 | VU to HS external trap requires STE + vsSTE | sctrctl.S=0, sctrctl.STE=1, vsctrctl.U=1, vsctrctl.STE=1, VU-mode triggers trap to HS-mode | External trap is recorded | `norm:exttrap_vuhs` |
| HCROSS-SSCTR-20 | VU to HS not recorded when vsSTE is missing | sctrctl.S=0, sctrctl.STE=1, vsctrctl.U=1, vsctrctl.STE=0, VU-mode triggers trap to HS-mode | External trap is not recorded | `norm:exttrap_vuhs` |

#### 14.4 VS-mode Freeze Behavior (vsctrctl Control)

**Specification Basis**: `norm:ctr_freeze_vs`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCTR-21 | VS-mode BPFRZ controlled by vsctrctl | vsctrctl.BPFRZ=1, VS-mode executes EBREAK trapping to VS-mode | sctrstatus.FROZEN=1 | `norm:ctr_freeze_vs` |
| HCROSS-SSCTR-22 | VS-mode BPFRZ=0 does not set FROZEN | vsctrctl.BPFRZ=0, VS-mode executes EBREAK | sctrstatus.FROZEN unchanged | `norm:ctr_freeze_vs` |
| HCROSS-SSCTR-23 | VS-mode LCOFIFRZ controlled by vsctrctl | vsctrctl.LCOFIFRZ=1, LCOFI traps to VS-mode | sctrstatus.FROZEN=1 | `norm:ctr_freeze_vs` |
| HCROSS-SSCTR-24 | Virtual LCOFI also triggers freeze | vsctrctl.LCOFIFRZ=1, hypervisor injects virtual LCOFI, traps to VS-mode | sctrstatus.FROZEN=1 | `norm:ctr_freeze_vs` |

#### 14.5 Virtualization Mode Transition Configuration Source

**Specification Basis**: Virtualization mode transitions use fields from vsctrctl (except M/MTE/LCOFIFRZ/BPFRZ which are controlled by sctrctl/mctrctl)

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCTR-25 | VU to HS transition uses vsctrctl filter bits | vsctrctl.U=1, sctrctl.S=1, set a filter bit in vsctrctl, VU-mode triggers trap to HS-mode | Whether the transition is recorded is determined by vsctrctl filter bits (not sctrctl) | `norm:vsctr-s_op` |
| HCROSS-SSCTR-26 | VS to HS transition uses vsctrctl/sctrctl fields | vsctrctl.S=1, sctrctl.S=1, VS-mode triggers trap to HS-mode | Source mode enable determined by vsctrctl.S, destination mode enable determined by sctrctl.S | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-27 | HS to VS/VU trap return uses sctrctl | sctrctl.S=1, vsctrctl.S=1, HS-mode executes SRET returning to VS-mode | Source mode enable determined by sctrctl.S, destination mode determined by vsctrctl.S | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-28 | Freeze uses sctrctl (trap to HS-mode) | sctrctl.BPFRZ=1, VS-mode breakpoint traps to HS-mode | Freeze determined by sctrctl.BPFRZ | `norm:ctr_freeze_bp` |
| HCROSS-SSCTR-29 | VS-mode freeze uses vsctrctl | vsctrctl.BPFRZ=1, VS-mode breakpoint traps to VS-mode | Freeze determined by vsctrctl.BPFRZ | `norm:ctr_freeze_vs` |

#### 14.6 hstateen0.CTR Control of VS-mode CTR Access

**Specification Basis**: `norm:hstateen_ctr`, `norm:hstateen_vs`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCTR-30 | VS-mode detection of CTR accessibility | VS-mode attempts to access sctrctl, observe whether exception is triggered | Access succeeds if hstateen0.CTR=1; triggers virtual-instruction if CTR=0 | `norm:hstateen_vs` |
| HCROSS-SSCTR-31 | VS-mode sctrstatus access when hstateen0.CTR=0 | hstateen0.CTR=0, VS-mode accesses sctrstatus | Triggers virtual-instruction | `norm:hstateen_vs` |
| HCROSS-SSCTR-32 | VS-mode SCTRCLR execution when hstateen0.CTR=0 | hstateen0.CTR=0, VS-mode executes SCTRCLR | Triggers virtual-instruction | `norm:hstateen_vs` |

### Code Examples

> [!NOTE]
> - This test group validates Ssctr extension behavior in Hypervisor scenarios. All tests must detect H extension availability at runtime via `HAS_H_EXT()` and TEST_SKIP when unavailable.
> - HCROSS-SSCTR-01~10 are migrated from `Ssctr_test_plan.md` Group 2, validating `vsctrctl` CSR basic functionality. `vsctrctl` is a CSR introduced by the H extension, substituting for `sctrctl` when V=1, and does not affect behavior when V=0.
> - HCROSS-SSCTR-11~14 are migrated from `Ssctr_test_plan.md` Groups 3/5, validating VS/VU-mode access restrictions on CTR CSRs. VS/VU-mode access to `sctrdepth` and execution of SCTRCLR trigger virtual-instruction (cause=22). HCROSS-SSCTR-13 validates that `vsireg*` and `sireg*` share the same CTR entry register state (no separate entry register set when V=1).
> - HCROSS-SSCTR-15~20 are migrated from `Ssctr_test_plan.md` Group 8, validating VS/VU-mode external trap recording. External trap recording depends on the intermediate mode's TE bits: VS to HS requires sctrctl.STE; VU to VS requires vsctrctl.STE; VU to HS requires sctrctl.STE + vsctrctl.STE (both TE bits must be set).
> - HCROSS-SSCTR-21~24 are migrated from `Ssctr_test_plan.md` Group 12, validating that VS-mode freeze behavior is controlled by BPFRZ/LCOFIFRZ in `vsctrctl` (not `sctrctl`). Note: when trapping to HS-mode, freeze is determined by `sctrctl.BPFRZ` (HCROSS-SSCTR-28).
> - HCROSS-SSCTR-25~29 are migrated from `Ssctr_test_plan.md` Group 13, validating configuration source selection during virtualization mode transitions. Core rule: source mode enable is determined by the xctrctl corresponding to that mode (VS-mode uses vsctrctl, HS-mode uses sctrctl); freeze is determined by the trap destination mode's xctrctl (trap to VS-mode uses vsctrctl, trap to HS-mode uses sctrctl).
> - HCROSS-SSCTR-30~32 are migrated from `Ssctr_test_plan.md` Group 14, validating `hstateen0.CTR` control of VS-mode CTR access. When `hstateen0.CTR=0`, VS-mode access to CTR state triggers virtual-instruction (cause=22), not illegal-instruction.
> - Distinction from Group 13 (HCROSS-SMCTR): Group 13 validates `mstateen0.CTR` and `hstateen0.CTR` control of lower privilege levels from the M-mode perspective, and MTE external trap recording to M-mode; this group validates `vsctrctl` functionality and VS/VU-mode CTR behavior from the S-mode/VS-mode perspective.

---

## Test Priority

| Priority | Test Group | Covered Test IDs | Rationale |
|----------|------------|------------------|-----------|
| P0 (Required) | Group 5 (Svinval) | HCROSS-SINVAL-01~14 | HINVAL instruction functionality and virtual-instruction exceptions are core to Hypervisor security isolation; existing plan coverage is severely insufficient |
| P0 (Required) | Group 11.2 (Virtual-inst) | HCROSS-SSCSRIND-11~21 | Virtual-instruction exceptions are the core guarantee of virtualization security isolation |
| P0 (Required) | Group 12.2 (vsstatus.SDT) | HCROSS-SSDBLTRP-07~12 | VS-mode double-trap is a key security mechanism in virtualization scenarios |
| P1 (Important) | Group 1 (Svadu) | HCROSS-SVADU-01~06 | henvcfg.ADUE writability and HLV/HSV interaction are key behaviors for Svadu in virtualization scenarios |
| P1 (Important) | Group 2 (Sstvala) | HCROSS-SSTVALA-01~08 | Precise stval/vstval writes are fundamental for guest OS debugging and exception handling |
| P1 (Important) | Group 8 (Smstateen) | HCROSS-SMSTA-01~14 | hstateen control and VS/VU-mode exception behavior are key guarantees of Hypervisor state isolation |
| P1 (Important) | Group 10 (Smcsrind) | HCROSS-SMCSRIND-01~08 | mstateen0[60] access control for virtualization CSRs (vsiselect/vsireg*) is a key security isolation guarantee |
| P1 (Important) | Group 11.3 (State-Enable) | HCROSS-SSCSRIND-22~27 | hstateen0[60] access control is key to security isolation |
| P1 (Important) | Group 11.1 (VS CSR) | HCROSS-SSCSRIND-01~10 | vsiselect/vsireg* are fundamental for H extension scenarios |
| P1 (Important) | Group 12.1 (henvcfg.DTE) | HCROSS-SSDBLTRP-01~06 | henvcfg.DTE is the enable control for VS-mode double-trap |
| P1 (Important) | Group 12.3 (SRET vsstatus.SDT) | HCROSS-SSDBLTRP-13~16 | SRET clearing of vsstatus.SDT is a key path for VS-mode trap return |
| P1 (Important) | Group 12.5 (XRET Hyp) | HCROSS-SSDBLTRP-19~24 | Cross-mode SDT clearing for MRET/SRET/MNRET in virtualization scenarios |
| P1 (Important) | Group 13.2 (hstateen0.CTR) | HCROSS-SMCTR-02~09 | hstateen0.CTR control of VS-mode CTR access is the guarantee of virtualization state isolation |
| P1 (Important) | Group 14.1 (vsctrctl) | HCROSS-SSCTR-01~10 | vsctrctl is the core control register for VS-mode CTR |
| P1 (Important) | Group 14.3 (VS/VU ext traps) | HCROSS-SSCTR-15~20 | VS/VU-mode external trap recording is key in virtualization scenarios |
| P1 (Important) | Group 14.5 (Virt transitions) | HCROSS-SSCTR-25~29 | Virtualization mode transition configuration source is the correctness guarantee for CTR under Hypervisor |
| P2 (Recommended) | Group 4 (Sscounterenw) | HCROSS-SSCOUNTERENW-01~09 | hcounteren control behavior is the guarantee for performance monitoring isolation |
| P2 (Recommended) | Group 9 (Sstc) | HCROSS-SSTC-01~15 | vstimecmp and VS-mode timer are core facilities for Hypervisor timer virtualization |
| P2 (Recommended) | Group 11.4 (Hyp cross) | HCROSS-SSCSRIND-28~33 | Transparent remapping and alias behavior depend on H extension |
| P2 (Recommended) | Group 12.4 (menvcfg.DTE Hyp) | HCROSS-SSDBLTRP-17~18 | menvcfg.DTE global control of Hypervisor CSRs |
| P2 (Recommended) | Group 13.1 (mstateen0.CTR Hyp) | HCROSS-SMCTR-01 | mstateen0.CTR blocking of vsctrctl |
| P2 (Recommended) | Group 13.3 (MTE Hyp) | HCROSS-SMCTR-10~12 | MTE external trap recording behavior in VS/VU-mode |
| P2 (Recommended) | Group 14.2 (VS/VU access) | HCROSS-SSCTR-11~14 | VS/VU-mode access restrictions on CTR CSRs |
| P2 (Recommended) | Group 14.4 (VS Freeze) | HCROSS-SSCTR-21~24 | VS-mode freeze behavior controlled by vsctrctl |
| P2 (Recommended) | Group 14.6 (hstateen VS) | HCROSS-SSCTR-30~32 | hstateen0.CTR control of VS-mode CTR access |
| P3 (Optional) | Group 3 (Ssccptr) | HCROSS-SSCCPTR-01~04 | PMA-level constraints are difficult to directly verify on emulators, primarily relying on platform guarantees |
| P3 (Optional) | Group 6 (Svnapot) | HCROSS-SVNAPOT-01~03 | G-stage NAPOT translation depends on both H extension and Svnapot extension being available; conditional implementation |
| P3 (Optional) | Group 7 (Svpbmt) | HCROSS-SVPBMT-01~04 | Two-stage PBMT override depends on both H extension and Svpbmt extension being available; conditional implementation |

---

## Test Implementation Notes

### File Organization

```
damo-priv-test/
├── hypervisor_cross/
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_hcross_svadu.c      # Group 1: Hypervisor × Svadu
│       ├── test_hcross_sstvala.c    # Group 2: Hypervisor × Sstvala
│       ├── test_hcross_ssccptr.c    # Group 3: Hypervisor × Ssccptr
│       ├── test_hcross_sscounterenw.c # Group 4: Hypervisor × Sscounterenw
│       ├── test_hcross_svinval.c    # Group 5: Hypervisor × Svinval
│       ├── test_hcross_svnapot.c    # Group 6: Hypervisor × Svnapot
│       ├── test_hcross_svpbmt.c     # Group 7: Hypervisor × Svpbmt
│       ├── test_hcross_ssdbltrp.c   # Group 12: Hypervisor × Ssdbltrp
│       ├── test_hcross_smctr.c      # Group 13: Hypervisor × Smctr
│       └── test_hcross_ssctr.c      # Group 14: Hypervisor × Ssctr
│
├── Hypervisor_Smstateen/            # Group 8: Hypervisor × Smstateen (standalone project)
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_helpers.h
│       ├── test_register.c
│       ├── test_hcross_smstateen_init.c
│       ├── test_hcross_smstateen_prop.c
│       ├── test_hcross_smstateen_bit63.c
│       ├── test_hcross_smstateen_func_bits.c
│       └── test_hcross_smstateen_exception.c
│
├── Hypervisor_Sstc/                  # Group 9: Hypervisor × Sstc (standalone project)
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_helpers.h
│       ├── test_register.c
│       ├── sstc_strap.S
│       ├── test_hcross_sstc_stce.c   # 9.1: henvcfg.STCE field control
│       ├── test_hcross_sstc_acc.c    # 9.2: VS-mode access control
│       ├── test_hcross_sstc_vs.c     # 9.3-9.5: vstimecmp / VSTIP / VS-mode timer
│
├── Hypervisor_Sscsrind/              # Group 11: Hypervisor × Sscsrind (standalone project)
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_helpers.h
│       ├── test_register.c
│       ├── test_hcross_sscsrind_vscsr.c     # 11.1: VS-level CSR basic functionality (HCROSS-SSCSRIND-01~10)
│       ├── test_hcross_sscsrind_vi.c        # 11.2: Virtual-Instruction exception (HCROSS-SSCSRIND-11~21)
│       ├── test_hcross_sscsrind_stateen.c   # 11.3: State-Enable access control (HCROSS-SSCSRIND-22~27)
│       └── test_hcross_sscsrind_hyp.c       # 11.4: Hypervisor cross-test (HCROSS-SSCSRIND-28~33)
│
├── hypervisor_cross/                 # Group 10: Hypervisor × Smcsrind (can be merged into main hypervisor_cross project)
│   └── tests/
│       └── test_hcross_smcsrind.c    # 10: mstateen0[60] control over vsiselect/vsireg*
│
└── common/hyp/                      # Reuse existing Hypervisor test framework
```

### Runtime Detection

All tests should detect the availability of required extensions at startup:

### Common Test Patterns

#### Pattern 1: Svadu Cross-Test (Group 1)

#### Pattern 2: Sstvala Cross-Test (Group 2)

#### Pattern 3: Svinval Cross-Test (Group 5)

### Key Considerations

1. **Extension Detection**: All tests must detect the availability of required extensions (H, Svadu, Svinval, etc.) at runtime, and TEST_SKIP when unavailable.

2. **Svadu and Svade Mutual Exclusion**: When `henvcfg.ADUE=0`, behavior is identical to Svade (A/D=0 triggers fault); when `ADUE=1`, hardware A/D update is enabled. Tests must explicitly determine the current ADUE state.

3. **Sstvala Precision Requirement**: The Sstvala extension mandates that `stval` is written with the faulting address, not 0. Test assertions must use `TEST_ASSERT_EQ` for exact comparison, not fuzzy validation with `TEST_ASSERT(stval != 0)`.

4. **Ssccptr Platform Dependency**: PMA is a platform hardwired attribute; on QEMU/Spike, main memory defaults to satisfying cacheability+coherence. HCROSS-SSCCPTR-04 requires platform support for dynamic PMA configuration, which most platforms do not support.

5. **Sscounterenw Probing Logic**: Before testing, probe which `hpmcounter` CSRs are not read-only zero, and only validate the writability of the corresponding `hcounteren` bits for those counters.

6. **Svinval Instruction Encoding**:
   - `hinval.vvma rs1, rs2`: `0011011 rs2 rs1 000 00000 1110011`
   - `hinval.gvma rs1, rs2`: `0111011 rs2 rs1 000 00000 1110011`
   - `sfence.w.inval`: `0001100 00000 00000 000 00000 1110011`
   - `sfence.inval.ir`: `0001100 00001 00000 000 00000 1110011`

7. **HINVAL.GVMA VMID Semantics**: The `rs2` register specifies VMID (not ASID). VMID=0 flushes TLB entries for all VMIDs; non-zero VMID flushes only the specific VMID.

8. **virtual-instruction vs. illegal-instruction Distinction**: VS/VU-mode executing HINVAL triggers virtual-instruction (cause=22), not illegal-instruction (cause=2). Test assertions must use the accurate cause constant.

---

## Group 8: Hypervisor × Smstateen

This group of tests verifies the behavior of the Smstateen extension in Hypervisor scenarios, including hstateen CSR access control, HS-mode/VS-mode/VU-mode privilege level interactions, etc. These tests are migrated from `smstateen_test_plan.md`, specifically targeting test cases that depend on the H extension.

### Test ID Mapping Table

| Original ID | New ID | Test Name |
|---------|-------|---------|
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

| Test ID | Test Name | Test Description | Expected Result | Norm Reference |
|---------|----------|----------|----------|----------|
| HCROSS-SMSTA-01 | hstateen0 initialization after reset | After M-mode sets certain mstateen0 bits to 1, write zero to hstateen0 | hstateen0 reads back as zero | `norm:hstateen_sstateen_zero_initialization` |
| HCROSS-SMSTA-02 | mstateen0 zero bit propagation to hstateen0 | Set a functional bit of mstateen0 to 0, then attempt to write that bit to hstateen0 from HS-mode | Corresponding hstateen0 bit reads back as 0 | `norm:mstateen_lower_priv_roz` |
| HCROSS-SMSTA-03 | SE0=0 blocks HS-mode hstateen0 | Set mstateen0 bit 63 to 0, access hstateen0 from HS-mode | Triggers illegal-instruction exception | `norm:mstateen_bit_63_op` |
| HCROSS-SMSTA-04 | bit 63 writability conditions | Check whether mstateen0 bit 63 is writable (H extension present or sstateen not all read-only zero) | Writable when conditions are met, otherwise RO0 | `norm:mstateen_bit_63_roz` |
| HCROSS-SMSTA-05 | SE0=0 blocks hstateen0 | Set mstateen0.SE0=0, HS-mode reads hstateen0 | Triggers illegal-instruction exception | `norm:mstateen0_se0_op` |
| HCROSS-SMSTA-06 | SE0=0 blocks hstateen0h (RV32) | Set mstateen0.SE0=0, HS-mode reads hstateen0h | Triggers illegal-instruction exception | `norm:mstateen0_se0_op` |
| HCROSS-SMSTA-07 | ENVCFG=0 blocks henvcfg | Set ENVCFG=0, HS-mode reads henvcfg | Triggers illegal-instruction exception | `norm:mstateen0_envcfg_op` |
| HCROSS-SMSTA-08 | CSRIND=0 blocks vsiselect | Set CSRIND=0, HS-mode reads vsiselect | Triggers illegal-instruction exception | `norm:mstateen0_csrind_op` |
| HCROSS-SMSTA-09 | IMSIC=0 blocks vstopei | Set IMSIC=0, HS-mode reads vstopei | Triggers illegal-instruction exception | `norm:mstateen0_imsic_op` |
| HCROSS-SMSTA-10 | CONTEXT=0 blocks hcontext | Set CONTEXT=0, HS-mode reads hcontext | Triggers illegal-instruction exception | `norm:mstateen0_context_op` |
| HCROSS-SMSTA-11 | P1P13=0 blocks hedelegh | Set P1P13=0, HS-mode reads hedelegh | Triggers illegal-instruction exception | `norm:mstateen0_p1p13_op` |
| HCROSS-SMSTA-12 | P1P13=1 allows hedelegh | Set P1P13=1, HS-mode reads hedelegh | Access succeeds | `norm:mstateen0_p1p13_op` |
| HCROSS-SMSTA-13 | VS-mode virtual-instruction | A bit of mstateen0=0 and access from VS-mode, meeting virtual-instruction exception conditions | Triggers virtual-instruction exception (cause=22) | `norm:stateen_illegal_state_access` |
| HCROSS-SMSTA-14 | VU-mode virtual-instruction | A bit of mstateen0=0 and access from VU-mode, meeting virtual-instruction exception conditions | Triggers virtual-instruction exception (cause=22) | `norm:stateen_illegal_state_access` |

### Key Considerations

1. **H Extension Detection**: All tests must detect H extension availability at runtime via `HAS_H_EXT()`, and TEST_SKIP when unavailable.

2. **HS-mode and S-mode Relationship**: When the H extension is present and V=0, S-mode is effectively HS-mode. Tests use `goto_priv(PRIV_S)` to simulate HS-mode access.

3. **VS-mode/VU-mode Testing**: Requires the `ENABLE_HYP` macro enabled at compile time, and uses `goto_priv(PRIV_VS)` or `goto_priv(PRIV_VU)` to switch to virtual privilege levels.

4. **virtual-instruction vs. illegal-instruction Distinction**: When VS/VU-mode accesses a controlled CSR, if hstateen allows but mstateen blocks, virtual-instruction (cause=22) should be triggered; if hstateen also blocks, illegal-instruction (cause=2) is triggered.

5. **hstateen CSR Addresses**: hstateen0-3 CSR addresses are 0x60C-0x60F, hstateen0h-3h (RV32) are 0x61C-0x61F.

---

## References

- `SPEC/hypervisor.adoc` -- RISC-V Hypervisor Extension, Version 1.0
- `SPEC/svadu.adoc` -- Svadu Extension
- `SPEC/sstvala.adoc` -- Sstvala Extension
- `SPEC/ssccptr.adoc` -- Ssccptr Extension
- `SPEC/sscounterenw.adoc` -- Sscounterenw Extension
- `SPEC/svinval.adoc` -- Svinval Extension
- `SPEC/svnapot.adoc` -- Svnapot Extension
- `SPEC/svpbmt.adoc` -- Svpbmt Extension
- `DOCS/testplan/Hypervisor_test_plan.md` -- Hypervisor Basic Test Plan
- `DOCS/testplan/Hypervisor_2_stage_test_plan.md` -- Two-Stage Translation Test Plan
- `DOCS/testplan/Hypervisor_gstage_test_plan.md` -- G-stage Standalone Test Plan
- `DOCS/testplan/svadu_test_plan.md` -- Svadu Standalone Test Plan
- `DOCS/testplan/sstvala_test_plan.md` -- Sstvala Standalone Test Plan
- `DOCS/testplan/ssccptr_test_plan.md` -- Ssccptr Standalone Test Plan
- `DOCS/testplan/sscounterenw_test_plan.md` -- Sscounterenw Standalone Test Plan
- `DOCS/testplan/svinval_test_plan.md` -- Svinval Standalone Test Plan
- `DOCS/testplan/svnapot_test_plan.md` -- Svnapot Standalone Test Plan
- `DOCS/testplan/svpbmt_test_plan.md` -- Svpbmt Standalone Test Plan
- `ideas/hypervisor_gap.md` -- Hypervisor Test Gap Analysis
- `SPEC/smstateen.adoc` -- Smstateen Extension Specification
- `DOCS/testplan/ssstateen_test_plan.md` -- Ssstateen Standalone Test Plan
- `SPEC/sstc.adoc` -- Sstc Extension Specification (Supervisor-mode Timer Interrupts)
- `DOCS/testplan/sstc_test_plan.md` -- Sstc Standalone Test Plan
- `SPEC/smcsrind.adoc` -- Smcsrind/Sscsrind Extension for Indirect CSR Access
- `DOCS/testplan/Smcsrind_test_plan.md` -- Smcsrind Machine Mode Test Plan
- `DOCS/testplan/Sscsrind_test_plan.md` -- Sscsrind Supervisor Mode Test Plan
- `SPEC/ssdbltrp.adoc` -- Ssdbltrp Double Trap Extension
- `DOCS/testplan/Ssdbltrp_test_plan.md` -- Ssdbltrp Standalone Test Plan
- `SPEC/smctr.adoc` -- Smctr (Control Transfer Records - Machine-level) Extension
- `SPEC/ssctr.adoc` -- Ssctr (Control Transfer Records - Supervisor-level) Extension
- `DOCS/testplan/Smctr_test_plan.md` -- Smctr Machine Mode Test Plan
- `DOCS/testplan/Ssctr_test_plan.md` -- Ssctr Supervisor Mode Test Plan
