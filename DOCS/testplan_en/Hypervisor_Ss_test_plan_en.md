**[中文](../testplan/Hypervisor_Ss_test_plan.md) | English**

# Hypervisor × Ss* Extensions Cross Test Plan

> This document describes the test plan for cross-scenarios between the Hypervisor (H) extension and other Ss* (Supervisor-level) extension families. This plan was split out from `Hypervisor_cross_test_plan.md` and retains only the content at the intersection of the Hypervisor and Ss* extensions. These test scenarios were originally marked in the standalone test plans of the respective extensions as "covered by the Hypervisor test plan" or "excluded due to the absence of the H extension", but analysis showed that the existing Hypervisor test plans (`Hypervisor_CSR_test_plan.md`, `Hypervisor_Interrupts_test_plan.md`, `Hypervisor_Exceptions_test_plan.md`, `Hypervisor_2_stage_test_plan.md`, `Hypervisor_gstage_test_plan.md`) do not fully cover them.
>
> Generation date: 2026-06-22

---

## SPEC Sections Covered by This Document

This plan is based on the following official RISC-V specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — Hypervisor (H) extension: hstateen/henvcfg/hcounteren control over VS/VU-mode, virtual-instruction mechanism, VSTIP synthesis
- `SPEC/riscv-isa-manual/src/priv/sstvala.adoc` — Sstvala: precise stval writes on traps
- `SPEC/riscv-isa-manual/src/priv/ssccptr.adoc` — Ssccptr: cacheability/coherence PMA requirements for page-table reads
- `SPEC/riscv-isa-manual/src/priv/sscounterenw.adoc` — Sscounterenw: writability of counter-enable bits
- `SPEC/riscv-isa-manual/src/priv/smstateen.adoc` — Smstateen: hstateen CSR behavior (Ssstateen cross portion)
- `SPEC/riscv-isa-manual/src/priv/sstc.adoc` — Sstc: vstimecmp and VS-mode timers
- `SPEC/riscv-isa-manual/src/priv/smcsrind.adoc` — Smcsrind/Sscsrind: vsiselect/vsireg* indirect CSR access
- `SPEC/riscv-isa-manual/src/priv/ssdbltrp.adoc` — Ssdbltrp: henvcfg.DTE, vsstatus.SDT, VS-mode double-trap
- `SPEC/riscv-isa-manual/src/priv/smctr.adoc` — Ssctr: vsctrctl and VS/VU-mode control transfer recording (Ssctr is defined in the same volume as Smctr in this file)
- `SPEC/riscv-isa-manual/src/priv/sscofpmf.adoc` — Sscofpmf: mhpmevent VSINH/VUINH counting inhibition, dual gating of VS-mode `scountovf`
- `SPEC/riscv-isa-manual/src/priv/smcdeleg.adoc` — Smcdeleg/Ssccfg: virtualization of scountovf/scountinhibit, hvip/hvien LCOFI bits, vsiselect/vsireg* access rules
- `SPEC/riscv-ssqosid/sqosid.adoc` — Ssqosid: srmcfg and mstateen0[55] gating

Official repositories:

- https://github.com/riscv/riscv-isa-manual (files at the above paths under src/priv within the repository)
- https://github.com/riscv/riscv-ssqosid (sqosid.adoc within the repository)

---

## Scope

### Covered Extension Intersections

- **Hypervisor × Sstvala**: precise `stval` write behavior on guest page-faults (cause 20/23)
- **Hypervisor × Ssccptr**: cacheability/coherence verification of two-stage page-table walks
- **Hypervisor × Sscounterenw**: `hcounteren` control over VS/VU-mode counter writes
- **Hypervisor × Ssstateen**: `hstateen0-3` CSR presence and accessibility, `hstateen` bit 63 control over VS-mode access to `sstateen`, `hstateen` read-only zero propagation to VS-mode, control over VS-mode state by each function bit of `hstateen0` (SE0/ENVCFG/CSRIND/IMSIC/AIA/CONTEXT), `hstateen` read-only constraints and encoding consistency
- **Hypervisor × Sstc**: `henvcfg.STCE` writability and constraints, VS-mode access control over `stimecmp`/`vstimecmp`, `vstimecmp` CSR read/write, VSTIP synthesis logic, `henvcfg.STCE` control over VS-mode timers, VS-mode timer interrupt capture
- **Hypervisor × Sscsrind**: VS-level CSR (vsiselect/vsireg*) basic functionality, Virtual-instruction exception behavior, `hstateen0[60]` control over VS/VU-mode access, remapping behavior of VS-mode transparent access to vsireg* via sireg*
- **Hypervisor × Ssdbltrp**: `henvcfg.DTE` enable/disable control over VS-mode, `vsstatus.SDT` field behavior and SDT/SIE mutual exclusion, SRET clearing of `vsstatus.SDT`, cross-mode clearing of SDT/vsstatus.SDT by MRET/SRET/MNRET under Hypervisor scenarios
- **Hypervisor × Ssctr**: `vsctrctl` CSR basic functionality and field verification, VS/VU-mode external trap recording (STE/vsSTE), configuration sources of virtualized mode transitions, VS-mode Freeze behavior (vsctrctl control), VS-mode access restrictions on sctrdepth/SCTRCLR, hstateen0.CTR control over VS-mode CTR access
- **Hypervisor × Ssqosid**: VS/VU-mode access to `srmcfg` raising virtual-instruction exception when V=1, precedence between mstateen0[55] gating and the V=1 rule, stval/htinst values on virtual-instruction traps
- **Hypervisor × Sscofpmf**: inhibition of VS/VU-mode counting by the VSINH/VUINH bits of `mhpmevent`, dual gating of VS-mode `scountovf` by `mcounteren`+`hcounteren`
- **Hypervisor × Smcdeleg/Ssccfg**: VS/VU-mode reads of `scountovf` and virtual-instruction raised by access to `scountinhibit` when CDE=1, implementation and writability of the LCOFI bit (bit 13) of `hvip`/`hvien`, multi-privilege access rules of `vsiselect`/`vsireg*` in the 0x40-0x5F range, hstateen0 bit 60 control over VS-mode

### Out of Scope for This Document

- Hypervisor basic functionality already covered by `Hypervisor_CSR_test_plan.md`, `Hypervisor_Interrupts_test_plan.md`, `Hypervisor_Exceptions_test_plan.md`, `Hypervisor_2_stage_test_plan.md`, `Hypervisor_gstage_test_plan.md`
- Sha sub-extensions already covered by `Shcounterenw_test_plan.md` (a different extension family from Sscounterenw)
- Behavior of each extension in non-Hypervisor scenarios (covered by their respective standalone test plans)
- Cross tests between the Hypervisor and Sv\*/Sm\*/Z\* extensions (covered by `Hypervisor_Sv_test_plan.md`, `Hypervisor_Sm_test_plan.md`, `Hypervisor_Zi_test_plan.md` respectively)
- Ssdbltrp non-Hypervisor tests (`sstatus`.SDT field, S-mode double-trap, `menvcfg`.DTE basic control, `medeleg`[16], `mtval2`) — covered by `Ssdbltrp_test_plan.md`

---

## Covered Specification Points

The following table lists the core specification points covered by this plan. Entries with the `norm:` prefix are official SPEC labels; entries without the prefix are specification points decomposed from the SPEC text. Other norm points directly cited in the Spec Reference of each Group (Sscsrind/Ssdbltrp/Ssctr/Ssqosid, etc.) are also within the coverage of this plan and are uniformly listed in the coverage matrix of Appendix A at the end of this document.

| Norm ID | Source | Description |
|---------|--------|-------------|
| `norm:H_guest_page_fault` | `hypervisor.adoc` | On a guest-page fault, `mtval` or `stval` is written with the faulting guest virtual address. |
| `norm:sstvala_stval_faulting_vaddr` | `sstvala.adoc` | When a page-fault is triggered by an instruction fetch, `stval` is written with the faulting virtual address (PC). |
| `norm:sstvala_stval_faulting_instruction` | `sstvala.adoc` | When a virtual-instruction exception is raised, `stval` must be written with the faulting instruction encoding. |
| `norm:ssccptr_memory_pte_reads` | `ssccptr.adoc` | If the Ssccptr extension is implemented, then main memory regions with both the cacheability and coherence PMAs must support hardware page-table reads. |
| `norm:sscounterenw_hpmcounter_scounteren` | `sscounterenw.adoc` | If the Sscounterenw extension is implemented, then for any `hpmcounter` that is not read-only zero, the corresponding bit in `scounteren` must be writable. |
| `hcounteren_vs_vu_control` | `hypervisor.adoc` | The `hcounteren` CSR controls availability of performance monitoring counters to VS-mode and VU-mode. |
| `norm:hstateen_rv64_csrs` | `smstateen.adoc` | When H extension is implemented, hstateen0-3 CSRs are added. |
| `norm:stateen_rv32_upper_bits_csrs` | `smstateen.adoc` | RV32 provides additional hstateen0h-3h CSRs for upper 32 bits. |
| `norm:hstateen_encoding` | `smstateen.adoc` | hstateen CSRs have the same encoding as mstateen CSRs. |
| `norm:hstateen_bit_63_op` | `smstateen.adoc` | Bit 63 of each hstateen CSR controls whether VS-mode may access the corresponding sstateen CSR. |
| `norm:hstateen_bit_63_writable` | `smstateen.adoc` | Bit 63 of each hstateen CSR is always writable (not read-only). |
| `norm:sstateen_vsmode_access_roz` | `smstateen.adoc` | For any bit that is zero in hstateen (whether read-only zero or written to zero), the corresponding bit in sstateen appears as read-only zero when accessed from VS-mode. |
| `norm:sstateen_ro1_bits` | `smstateen.adoc` | A bit in sstateen cannot be read-only one unless the same bit in both mstateen and hstateen (when H is implemented) is also read-only one. |
| `norm:hstateen_ro1_bits` | `smstateen.adoc` | A bit in hstateen cannot be read-only one unless the same bit in mstateen is also read-only one. |
| `norm:stateen_warl_access` | `smstateen.adoc` | Each standard-defined bit in stateen CSRs is WARL (Write Any Values, Reads Legal Values). |
| `norm:stateen_unimplemented_state_roz` | `smstateen.adoc` | Bits that control state for unimplemented extensions are read-only zero. |
| `norm:stateen_reserved_roz` | `smstateen.adoc` | Reserved bits in stateen CSRs are read-only zero. |
| `norm:hstateen0_SE0_op` | `smstateen.adoc` | hstateen0.SE0 (bit 63) controls whether VS-mode may access sstateen0. |
| `norm:hstateen0_envcfg_op` | `smstateen.adoc` | hstateen0.ENVCFG (bit 62) controls whether VS-mode may access senvcfg. |
| `norm:hstateen0_csrind_op` | `smstateen.adoc` | hstateen0.CSRIND (bit 60) controls whether VS-mode may access siselect and sireg* (which are actually vsiselect and vsireg*). |
| `norm:hstateen0_imsic_op` | `smstateen.adoc` | hstateen0.IMSIC (bit 58) controls access to guest IMSIC state and vstopei in VS-mode. |
| `norm:hstateen0_aia_op` | `smstateen.adoc` | hstateen0.AIA (bit 59) controls VS-mode access to Ssaia state not covered by CSRIND or IMSIC bits. |
| `norm:hstateen0_context_op` | `smstateen.adoc` | hstateen0.CONTEXT (bit 57) controls whether VS-mode may access scontext. |
| `norm:hcounteren_acc` | `hypervisor.adoc` | When the TM bit in `hcounteren` is clear, attempts to access the `vstimecmp` register (via `stimecmp`) while executing in VS-mode will cause a virtual-instruction exception if the same bit in `mcounteren` is set. |
| `norm:henvcfg_stce` | `hypervisor.adoc` | henvcfg.STCE=1 enables vstimecmp; when STCE=0, VS-mode (V=1) access to stimecmp raises virtual-instruction exception. |
| `norm:vstimecmp_exist` | `sstc.adoc` | Sstc adds a new VS-level vstimecmp CSR. |
| `norm:sstc_vs_facility` | `sstc.adoc` | Sstc provides a similar timer mechanism for VS-mode via the Hypervisor extension. |
| `norm:hip_vstip_vstie_acc_op` | `hypervisor.adoc` | VSTIP = hvip.VSTIP OR vstimecmp timer signal. |

---

## Group 1. Hypervisor × Sstvala Cross Tests

**Spec Reference**:
- `norm:H_guest_page_fault`: on guest-page-fault, `stval` is written with the faulting GVA
- `norm:sstvala_stval_faulting_vaddr`: on instruction-fetch page-fault, `stval` is written with the faulting PC
- `norm:sstvala_stval_faulting_instruction`: on virtual-instruction exception, `stval` must be written with the faulting instruction encoding

**Test Scope**: Verify the precise write behavior of `stval`/`vstval` for various exceptions under Hypervisor scenarios for the Sstvala extension. For address-class exceptions (guest-page-fault cause 20/21/23), `stval` must equal the faulting GVA; for instruction-class exceptions (virtual-instruction cause=22), `stval` must equal the faulting instruction encoding. The existing Hypervisor test plans already cover stval verification for cause=21 (load guest-page-fault); this group supplements precise verification for cause=20 (instruction) and cause=23 (store/AMO), as well as precise instruction-encoding verification for virtual-instruction.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSTVALA-01 | Precise stval value on instruction guest-page-fault (cause=20) | VS-mode jumps to execute a GPA address not mapped in G-stage, triggering an inst guest-page-fault trap to HS-mode | scause=20, stval == faulting GVA (i.e., the jump target address), not 0 or other values |
| HCROSS-SSTVALA-02 | Precise stval value on store guest-page-fault (cause=23) | VS-mode performs a store to a GPA address not mapped in G-stage, triggering a store guest-page-fault trap to HS-mode | scause=23, stval == faulting GVA (i.e., the store target address), not 0 |
| HCROSS-SSTVALA-03 | Precise stval value on AMO guest-page-fault (cause=23) | VS-mode performs an AMO (e.g., AMOADD.W) to a GPA address not mapped in G-stage, triggering a guest-page-fault trap to HS-mode | scause=23, stval == faulting GVA (i.e., the AMO target address) |
| HCROSS-SSTVALA-04 | Precise vstval value when VS-stage inst page-fault (cause=12) is delegated to VS-mode | Enable VS-stage translation (SV39), configure medeleg+hedeleg to delegate inst page-fault (cause=12) to VS-mode. VS-mode jumps to an address not mapped in VS-stage, triggering an inst page-fault delegated to the VS-mode handler | vscause=12, vstval == faulting VA (jump target virtual address) |
| HCROSS-SSTVALA-05 | Precise vstval value when VS-stage load page-fault (cause=13) is delegated to VS-mode | Enable VS-stage translation (SV39), configure medeleg+hedeleg to delegate load page-fault (cause=13) to VS-mode. VS-mode loads an address not mapped in VS-stage, triggering a load page-fault delegated to the VS-mode handler | vscause=13, vstval == faulting VA (load target virtual address) |
| HCROSS-SSTVALA-06 | Precise stval value on virtual-instruction exception: VS-mode reads hstatus | VS-mode executes `csrrs x5, hstatus, x0` (encoding 0x600022F3), triggering virtual-instruction (cause=22) | scause=22, stval == 0x600022F3 (faulting instruction encoding) |
| HCROSS-SSTVALA-07 | Precise stval value on virtual-instruction exception: VS-mode writes hgatp | VS-mode executes `csrrw x0, hgatp, x0` (encoding 0x68001073), triggering virtual-instruction (cause=22) | scause=22, stval == 0x68001073 (faulting instruction encoding) |
| HCROSS-SSTVALA-08 | Precise stval value on virtual-instruction exception: VS-mode reads hideleg | VS-mode executes `csrrs x5, hideleg, x0` (encoding 0x603022F3), triggering virtual-instruction (cause=22) | scause=22, stval == 0x603022F3 (faulting instruction encoding) |

> [!NOTE]
> - The core semantics of Sstvala is to guarantee that `stval` is written with the faulting address (rather than 0). The base H extension specification allows `stval` to be 0 in some scenarios, while the Sstvala extension mandates writing the precise address.
> - Norm point correspondence: stval precision for address-class exceptions (page-fault/guest-page-fault) corresponds to `norm:sstvala_stval_faulting_vaddr`; stval precision for instruction-class exceptions (virtual-instruction) corresponds to `norm:sstvala_stval_faulting_instruction`.
> - HCROSS-SSTVALA-01~03 verify the precision of `stval` when guest-page-fault (G-stage fault) traps to HS-mode; HCROSS-SSTVALA-04~05 verify the precision of `vstval` when VS-stage page-faults are delegated to VS-mode via medeleg+hedeleg. 04/05 use VS-stage page-faults (cause 12/13) rather than guest-page-faults (cause 20/23), because the RISC-V SPEC stipulates that guest-page-faults cannot be delegated to VS-mode via hedeleg (hedeleg bits 20/21/23 are read-only zero; see `Hypervisor_Exceptions_test_plan.md` DELEG-15/16).
> - Difference from the GFAULT series cases in `Hypervisor_gstage_test_plan.md`: the GFAULT series mainly verifies fault triggering and htval encoding, while this group focuses on precise value assertions of stval/vstval.
> - HCROSS-SSTVALA-06~08 verify Sstvala's precision requirement for **instruction-class exceptions**: on virtual-instruction exception (cause=22), `stval` must contain the encoding of the instruction that raised the exception (zero-extended to XLEN). This differs from the stval semantics of guest-page-fault (address-class exception) — the latter requires writing the faulting virtual address. Instruction encoding derivation: `csrrs x5, 0x600, x0` = `[31:20]=0x600 [19:15]=00000 [14:12]=010 [11:7]=00101 [6:0]=1110011` = `0x600022F3`.
> - HCROSS-SSTVALA-06~08 are migrated from `sstvala_test_plan.md` Group 6 (TVAL-VI-01~03). The H extension (`ENABLE_HYP=1`) is required.

---

## Group 2. Hypervisor × Ssccptr Cross Tests

**Spec Reference**:
- `norm:ssccptr_memory_pte_reads`: main memory regions with cacheability and coherence PMAs must support hardware page-table reads

**Test Scope**: Verify that, under the Hypervisor two-stage translation scenario, VS-stage and G-stage page-table walks can complete correctly in main memory regions that satisfy the PMA conditions.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSCCPTR-01 | VS-stage page-table walk in cacheable+coherent main memory | Establish VS-stage page tables in default main memory (satisfying cacheability+coherence PMAs), enable two-stage translation, VS-mode executes a load | the page-table walk succeeds, the load returns the correct value (hardware can correctly read VS-stage page tables) |
| HCROSS-SSCCPTR-02 | G-stage page-table walk in cacheable+coherent main memory | Establish G-stage page tables in default main memory, enable two-stage translation, VS-mode executes a load | the page-table walk succeeds, the load returns the correct value (hardware can correctly read G-stage page tables) |
| HCROSS-SSCCPTR-03 | Both-stage page-table walk in cacheable+coherent main memory | Both VS-stage and G-stage page tables are allocated in default main memory, VS-mode executes load/store | both-stage page-table walks succeed, R/W return correct values |
| HCROSS-SSCCPTR-04 | PMA attribute verification of the physical page holding G-stage page tables | Attempt to allocate G-stage page tables in a non-cacheable or non-coherent region (if the platform supports it), verify the behavior | if the PMAs do not satisfy the Ssccptr requirements, the behavior is implementation defined (page walk may fail); if the platform does not support configuring PMAs, TEST_SKIP |

> [!NOTE]
> - Ssccptr is a PMA-level constraint extension and introduces no new instructions or CSRs. On platforms where main memory regions by default satisfy the cacheability+coherence PMA conditions, HCROSS-SSCCPTR-01~03 are expected to PASS.
> - HCROSS-SSCCPTR-04 requires the platform to support dynamic PMA attribute configuration; otherwise the case TEST_SKIP.
> - The core value of the tests in this group is: ensuring that page-table walks of Hypervisor two-stage translation do not fail due to PMA constraints; this is a key correctness guarantee in virtualization scenarios.

---

## Group 3. Hypervisor × Sscounterenw Cross Tests

**Spec Reference**:
- `hcounteren_vs_vu_control`: `hcounteren` controls VS/VU-mode counter availability
- `norm:sscounterenw_hpmcounter_scounteren`: the corresponding bits for non-read-only-zero hpmcounters must be writable

**Test Scope**: Verify the control behavior of the `hcounteren` register over VS/VU-mode performance monitoring counter (hpmcounter) access, as well as the writability of the corresponding bits of `hcounteren`.

> [!NOTE]
> - The Sscounterenw extension requires: for any `hpmcounter` that is not read-only zero, the corresponding bit of `scounteren` (and `hcounteren`) must be writable. The tests in this group verify the writability and control behavior of `hcounteren`.
> - Please refer to the `Shcounterenw_test_plan.md` extension tests.

---

## Group 4. Hypervisor × Ssstateen Cross Tests

**Spec Reference**:
- `norm:hstateen_rv64_csrs`: hstateen0-3 CSRs are added when the H extension is implemented
- `norm:hstateen_bit_63_op`: hstateen bit 63 controls VS-mode access to the corresponding sstateen
- `norm:hstateen_bit_63_writable`: hstateen bit 63 is always writable
- `norm:sstateen_vsmode_access_roz`: bits that are zero in hstateen appear as read-only zero when VS-mode accesses sstateen
- `norm:hstateen0_SE0_op`: hstateen0.SE0 controls VS-mode access to sstateen0
- `norm:hstateen0_envcfg_op`: hstateen0.ENVCFG controls VS-mode access to senvcfg
- `norm:hstateen0_csrind_op`: hstateen0.CSRIND controls VS-mode access to siselect/sireg*
- `norm:hstateen0_imsic_op`: hstateen0.IMSIC controls guest IMSIC state and vstopei
- `norm:hstateen0_aia_op`: hstateen0.AIA controls the remaining Ssaia state not covered by CSRIND/IMSIC
- `norm:hstateen0_context_op`: hstateen0.CONTEXT controls VS-mode access to scontext
- `norm:hstateen_ro1_bits`: a bit in hstateen cannot be RO1 unless the same bit in mstateen is also RO1
- `norm:hstateen_encoding`: hstateen encoding is consistent with mstateen

**Test Scope**: Verify the behavior of the Ssstateen extension's hstateen CSRs under Hypervisor scenarios, including hstateen presence and accessibility, bit 63 control over VS-mode access to sstateen, hstateen read-only zero propagation to VS-mode, control over VS-mode state by each function bit of hstateen0 (SE0/ENVCFG/CSRIND/IMSIC/AIA/CONTEXT), and hstateen read-only constraints and encoding consistency.

> **Note**: The tests in this group are migrated from `ssstateen_test_plan.md` Groups 7-12. The H extension and the Ssstateen extension must both be available. Prerequisite configuration: M-mode must pre-set the corresponding mstateen bits to 1 to allow HS-mode access to hstateen.

#### 4.1 hstateen CSR Presence and Accessibility

**Spec Reference**: `norm:hstateen_rv64_csrs`, `norm:stateen_rv32_upper_bits_csrs`, `norm:hstateen_encoding`

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-01 | hstateen0 readable in HS-mode | Configure mstateen0.SE0=1, HS-mode reads hstateen0 | no exception |
| HCROSS-SSSTA-02 | hstateen0 writable in HS-mode | Configure mstateen0.SE0=1, HS-mode writes hstateen0 then reads back | no exception, writable bits take effect |
| HCROSS-SSSTA-03 | hstateen1 readable/writable in HS-mode | Configure mstateen1 bit63=1, HS-mode reads/writes hstateen1 | no exception |
| HCROSS-SSSTA-04 | hstateen2 readable/writable in HS-mode | Configure mstateen2 bit63=1, HS-mode reads/writes hstateen2 | no exception |
| HCROSS-SSSTA-05 | hstateen3 readable/writable in HS-mode | Configure mstateen3 bit63=1, HS-mode reads/writes hstateen3 | no exception |
| HCROSS-SSSTA-06 | hstateen0h readable/writable in HS-mode (RV32) | On RV32 configure mstateen0.SE0=1, HS-mode reads/writes hstateen0h | no exception |

#### 4.2 hstateen bit 63 Controls sstateen Access

**Spec Reference**: `norm:hstateen_bit_63_op`, `norm:hstateen_bit_63_writable`

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-07 | hstateen0 bit 63 writable (write 0) | HS-mode writes hstateen0 bit 63 to 0 then reads back | bit 63 reads back 0 |
| HCROSS-SSSTA-08 | hstateen0 bit 63 writable (write 1) | HS-mode writes hstateen0 bit 63 to 1 then reads back | bit 63 reads back 1 |
| HCROSS-SSSTA-09 | hstateen0.SE0=0 blocks VS-mode access to sstateen0 | Set hstateen0 bit63=0, VS-mode reads sstateen0 | virtual-instruction exception raised (cause=22) |
| HCROSS-SSSTA-10 | hstateen0.SE0=1 allows VS-mode access to sstateen0 | Set hstateen0 bit63=1, VS-mode reads sstateen0 | access succeeds, no exception |
| HCROSS-SSSTA-11 | hstateen0.SE0=0 blocks VS-mode write of sstateen0 | Set hstateen0 bit63=0, VS-mode writes sstateen0 | virtual-instruction exception raised (cause=22) |
| HCROSS-SSSTA-12 | hstateen1 bit 63 writable | HS-mode writes hstateen1 bit 63 to 0 and 1 | each read-back value matches the written value |
| HCROSS-SSSTA-13 | hstateen1 bit63=0 blocks VS-mode access to sstateen1 | Set hstateen1 bit63=0, VS-mode reads sstateen1 | virtual-instruction exception raised |
| HCROSS-SSSTA-14 | hstateen2 bit 63 controls sstateen2 | Set hstateen2 bit63=0/1, VS-mode accesses sstateen2 | bit63=0 raises exception, bit63=1 succeeds |
| HCROSS-SSSTA-15 | hstateen3 bit 63 controls sstateen3 | Set hstateen3 bit63=0/1, VS-mode accesses sstateen3 | bit63=0 raises exception, bit63=1 succeeds |

#### 4.3 hstateen Read-Only Zero Propagation to VS-mode sstateen

**Spec Reference**: `norm:sstateen_vsmode_access_roz`

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-16 | hstateen0.C=0 propagates to VS-mode sstateen0.C | Set hstateen0.C=0 and bit63=1, VS-mode writes sstateen0.C=1 then reads back | sstateen0.C reads back 0 in VS-mode |
| HCROSS-SSSTA-17 | hstateen0.C=1 releases VS-mode propagation | Set hstateen0.C=1 and bit63=1, VS-mode writes sstateen0.C=1 then reads back | sstateen0.C reads back 1 in VS-mode |
| HCROSS-SSSTA-18 | hstateen0.JVT=0 propagates to VS-mode sstateen0.JVT | Set hstateen0.JVT=0 and bit63=1, VS-mode writes sstateen0.JVT=1 then reads back | sstateen0.JVT reads back 0 in VS-mode |
| HCROSS-SSSTA-19 | hstateen0 multiple bits propagate simultaneously | Set multiple function bits of hstateen0 to 0, VS-mode verifies the corresponding bits of sstateen0 one by one | all corresponding bits are read-only zero in VS-mode |
| HCROSS-SSSTA-20 | Propagation released after hstateen0 bit changes from 0 to 1 | First set hstateen0.C=0 and verify propagation, then change to 1 | sstateen0.C becomes writable in VS-mode |

#### 4.4 hstateen0 Function Bit Control

**Spec Reference**: `norm:hstateen0_SE0_op`, `norm:hstateen0_envcfg_op`, `norm:hstateen0_csrind_op`, `norm:hstateen0_imsic_op`, `norm:hstateen0_aia_op`, `norm:hstateen0_context_op`

##### SE0 bit (bit 63) — sstateen0 VS-mode Access

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-21 | hstateen0.SE0=0 blocks VS-mode read of sstateen0 | Set hstateen0.SE0=0, VS-mode reads sstateen0 | virtual-instruction exception raised |
| HCROSS-SSSTA-22 | hstateen0.SE0=0 blocks VS-mode write of sstateen0 | Set hstateen0.SE0=0, VS-mode writes sstateen0 | virtual-instruction exception raised |
| HCROSS-SSSTA-23 | hstateen0.SE0=1 allows VS-mode access to sstateen0 | Set hstateen0.SE0=1, VS-mode reads/writes sstateen0 | access succeeds |

##### ENVCFG bit (bit 62) — senvcfg VS-mode Access

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-24 | hstateen0.ENVCFG=0 blocks VS-mode read of senvcfg | Set ENVCFG=0, VS-mode reads senvcfg | virtual-instruction exception raised |
| HCROSS-SSSTA-25 | hstateen0.ENVCFG=0 blocks VS-mode write of senvcfg | Set ENVCFG=0, VS-mode writes senvcfg | virtual-instruction exception raised |
| HCROSS-SSSTA-26 | hstateen0.ENVCFG=1 allows VS-mode access to senvcfg | Set ENVCFG=1, VS-mode reads/writes senvcfg | access succeeds |

##### CSRIND bit (bit 60) — siselect/sireg VS-mode Access

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-27 | hstateen0.CSRIND=0 blocks VS-mode read of siselect | Set CSRIND=0, VS-mode reads siselect (actually vsiselect) | virtual-instruction exception raised |
| HCROSS-SSSTA-28 | hstateen0.CSRIND=0 blocks VS-mode read of sireg* | Set CSRIND=0, VS-mode reads sireg (actually vsireg) | virtual-instruction exception raised |
| HCROSS-SSSTA-29 | hstateen0.CSRIND=1 allows VS-mode access | Set CSRIND=1, VS-mode reads siselect/sireg* | access succeeds |

##### IMSIC bit (bit 58) — Guest IMSIC Control

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-30 | hstateen0.IMSIC=0 blocks VS-mode access to IMSIC | Set IMSIC=0, VS-mode reads stopei (actually vstopei) | virtual-instruction exception raised |
| HCROSS-SSSTA-31 | hstateen0.IMSIC=1 allows VS-mode access to IMSIC | Set IMSIC=1, VS-mode reads stopei | access succeeds |
| HCROSS-SSSTA-32 | hstateen0.IMSIC=0 equivalent to VGEIN=0 | Set IMSIC=0, verify VS-mode cannot access IMSIC, equivalent to hstatus.VGEIN=0 | VS-mode cannot access guest IMSIC |

##### AIA bit (bit 59) — Ssaia Remaining State VS-mode Control

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-33 | hstateen0.AIA=0 blocks VS-mode Ssaia state | Set AIA=0, VS-mode accesses Ssaia state not covered by CSRIND/IMSIC | virtual-instruction exception raised |
| HCROSS-SSSTA-34 | hstateen0.AIA=1 allows VS-mode Ssaia state | Set AIA=1, VS-mode accesses the remaining Ssaia state | access succeeds |
| HCROSS-SSSTA-35 | hstateen0.AIA does not affect CSRIND/IMSIC control | Set AIA=0 but CSRIND=1, IMSIC=1, VS-mode accesses siselect/stopei | access succeeds (AIA does not control state governed by CSRIND/IMSIC) |

##### CONTEXT bit (bit 57) — scontext VS-mode Control

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-36 | hstateen0.CONTEXT=0 blocks VS-mode read of scontext | Set CONTEXT=0, VS-mode reads scontext | virtual-instruction exception raised |
| HCROSS-SSSTA-37 | hstateen0.CONTEXT=0 blocks VS-mode write of scontext | Set CONTEXT=0, VS-mode writes scontext | virtual-instruction exception raised |
| HCROSS-SSSTA-38 | hstateen0.CONTEXT=1 allows VS-mode access to scontext | Set CONTEXT=1, VS-mode reads/writes scontext | access succeeds |

#### 4.5 hstateen Read-Only Constraints

**Spec Reference**: `norm:hstateen_ro1_bits`, `norm:stateen_warl_access`, `norm:stateen_unimplemented_state_roz`, `norm:stateen_reserved_roz`

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-39 | hstateen0 RO1 bit constraint | Identify any RO1 bit in hstateen0, verify the same bit of mstateen0 is also RO1 | the same bit of mstateen0 is also RO1 |
| HCROSS-SSSTA-40 | hstateen1 RO1 bit constraint | Identify any RO1 bit in hstateen1, verify the same bit of mstateen1 is also RO1 | the same bit of mstateen1 is also RO1 |
| HCROSS-SSSTA-41 | hstateen2 RO1 bit constraint | Identify any RO1 bit in hstateen2, verify the same bit of mstateen2 is also RO1 | the same bit of mstateen2 is also RO1 |
| HCROSS-SSSTA-42 | hstateen3 RO1 bit constraint | Identify any RO1 bit in hstateen3, verify the same bit of mstateen3 is also RO1 | the same bit of mstateen3 is also RO1 |
| HCROSS-SSSTA-43 | hstateen0 reserved bits read-only zero | Write 1 to the WPRI fields of hstateen0 then read back | reserved bits read back 0 |
| HCROSS-SSSTA-44 | hstateen0 unimplemented extension bits read-only zero | For bits corresponding to unimplemented extensions, write 1 then read back | the corresponding bits read back 0 |
| HCROSS-SSSTA-45 | hstateen0 WARL writes legal values | Write a legal value to hstateen0 then read back | the read-back value matches the written value (limited to writable bits) |

#### 4.6 hstateen Encoding Consistency with mstateen

**Spec Reference**: `norm:hstateen_encoding`

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCROSS-SSSTA-46 | hstateen0 bit fields consistent with mstateen0 | Compare the writable bit masks of hstateen0 and mstateen0 | bit field definitions consistent (C/FCSR/JVT/SE0/ENVCFG/CSRIND/IMSIC/AIA/CONTEXT etc. at the same bit positions) |
| HCROSS-SSSTA-47 | hstateen0 function bits symmetric with mstateen0 | Write all 1s to hstateen0 and read back the valid bits; write all 1s to mstateen0 and read back the valid bits; compare the overlapping part | the valid bits of hstateen0 should be a subset of the valid bits of mstateen0 |
| HCROSS-SSSTA-48 | hstateen1 encoding consistent with mstateen1 | Compare the writable bit masks of hstateen1 and mstateen1 | bit field definitions consistent |
| HCROSS-SSSTA-49 | hstateen2 encoding consistent with mstateen2 | Compare the writable bit masks of hstateen2 and mstateen2 | bit field definitions consistent |
| HCROSS-SSSTA-50 | hstateen3 encoding consistent with mstateen3 | Compare the writable bit masks of hstateen3 and mstateen3 | bit field definitions consistent |

> [!NOTE]
> - The tests in this group verify the hstateen CSR behavior of the Ssstateen extension under Hypervisor scenarios. hstateen CSRs are the supervisor-level counterparts of mstateen, used to control VS/VU-mode access to extension state and prevent covert channels.
> - Prerequisite configuration for all tests: M-mode must set the corresponding mstateen bits to 1 to allow HS-mode access to hstateen. If the corresponding mstateen bit is 0, the corresponding hstateen bit is read-only zero.
> - hstateen0 bit 63 (SE0) controls VS-mode access permission to sstateen0. When SE0=0, VS-mode read/write of sstateen0 raises a virtual-instruction exception (cause=22), not an illegal-instruction exception (cause=2).
> - Each function bit of hstateen0 (SE0/ENVCFG/CSRIND/IMSIC/AIA/CONTEXT) controls VS-mode access to different supervisor-level state respectively. These control bits operate independently; the AIA bit does not affect state governed by CSRIND/IMSIC (HCROSS-SSSTA-35).
> - Read-only zero propagation rule of hstateen: bits that are zero in hstateen (whether read-only zero or written to zero) appear as read-only zero when VS-mode accesses sstateen (HCROSS-SSSTA-16~20). This is a key mechanism for preventing covert channels.
> - The sstateen tests under S-mode (including SS-UCTL-09 and SS-EXC-04 where VU-mode raises virtual-instruction, and SS-ALLOC-03 for RO1 bit constraints) remain in Groups 1-6 of `ssstateen_test_plan.md`.
> - The RV32-specific hstateen0h test (HCROSS-SSSTA-06) requires RV32 platform support and should TEST_SKIP on RV64 platforms.

---

## Group 5. Hypervisor × Sstc Cross Tests

**Spec Reference**:
- `norm:henvcfg_stce`: henvcfg.STCE=1 enables vstimecmp; when STCE=0, V=1 access to stimecmp raises a virtual-instruction exception
- `norm:hcounteren_acc`: when hcounteren.TM=0 and mcounteren.TM=1, VS-mode access to stimecmp (vstimecmp) raises a virtual-instruction exception (HCROSS-SSTC-05)
- `norm:vstimecmp_exist`: Sstc adds a new VS-level vstimecmp CSR
- `norm:sstc_vs_facility`: Sstc provides a similar timer mechanism for VS-mode via the Hypervisor extension
- `norm:hip_vstip_vstie_acc_op`: VSTIP = hvip.VSTIP OR vstimecmp timer signal

**Test Scope**: Verify the behavior of the Sstc extension under Hypervisor scenarios, including `henvcfg.STCE` writability and constraints, VS-mode access control over `stimecmp`/`vstimecmp`, `vstimecmp` CSR read/write, VSTIP synthesis logic, and VS-mode timer interrupt capture.

> **Note**: The tests in this group are migrated from `sstc_test_plan.md` Group 1 (SSTC-STCE-03/04), Group 3 (SSTC-ACC-08/09/10), and Group 6 (SSTC-VS-01~10). The H extension and the Sstc extension must both be available.

### Test ID Mapping Table

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| SSTC-STCE-03 | HCROSS-SSTC-01 | henvcfg.STCE read/write roundtrip |
| SSTC-STCE-04 | HCROSS-SSTC-02 | henvcfg.STCE constrained by menvcfg.STCE |
| SSTC-ACC-08 | HCROSS-SSTC-03 | VS-mode access to stimecmp with henvcfg.STCE=0 |
| SSTC-ACC-09 | HCROSS-SSTC-04 | VS-mode access to stimecmp with henvcfg.STCE=1 |
| SSTC-ACC-10 | HCROSS-SSTC-05 | VS-mode access to stimecmp with hcounteren.TM=0 |
| SSTC-VS-01 | HCROSS-SSTC-06 | M-mode vstimecmp read/write roundtrip |
| SSTC-VS-02 | HCROSS-SSTC-07 | vstimecmp all-1 / all-0 read/write |
| SSTC-VS-03 | HCROSS-SSTC-08 | HS-mode vstimecmp read/write |
| SSTC-VS-04 | HCROSS-SSTC-09 | vstimecmp triggers VSTIP |
| SSTC-VS-05 | HCROSS-SSTC-10 | vstimecmp clears VSTIP |
| SSTC-VS-06 | HCROSS-SSTC-11 | VSTIP = hvip.VSTIP OR vstimecmp signal |
| SSTC-VS-07 | HCROSS-SSTC-12 | VSTIP restores legacy behavior with henvcfg.STCE=0 |
| SSTC-VS-08 | HCROSS-SSTC-13 | VS-mode accesses vstimecmp via stimecmp |
| SSTC-VS-09 | HCROSS-SSTC-14 | Effect of htimedelta on vstimecmp comparison |
| SSTC-VS-10 | HCROSS-SSTC-15 | VS-mode timer interrupt capture |

### Test Case List

#### 5.1 henvcfg.STCE Field Control

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSTC-01 | henvcfg.STCE read/write roundtrip | In M-mode set menvcfg.STCE=1, then write henvcfg.STCE=1 and read back, then write 0 and read back | STCE bit read/write consistent | `norm:henvcfg_stce` |
| HCROSS-SSTC-02 | henvcfg.STCE constrained by menvcfg.STCE | With menvcfg.STCE=0, writing henvcfg.STCE=1 should be ignored (read-only zero) | henvcfg.STCE reads back 0 | `norm:henvcfg_stce` |

#### 5.2 VS-mode Access Control

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSTC-03 | VS-mode access to stimecmp with henvcfg.STCE=0 | menvcfg.STCE=1, henvcfg.STCE=0, VS-mode (V=1) reads stimecmp | virtual-instruction exception raised (cause=22) | `norm:henvcfg_stce` |
| HCROSS-SSTC-04 | VS-mode access to stimecmp with henvcfg.STCE=1 | menvcfg.STCE=1, henvcfg.STCE=1, hcounteren.TM=1, VS-mode reads stimecmp | no exception, read succeeds (actually accesses vstimecmp) | `norm:henvcfg_stce` |
| HCROSS-SSTC-05 | VS-mode access to stimecmp with hcounteren.TM=0 | menvcfg.STCE=1, henvcfg.STCE=1, hcounteren.TM=0, VS-mode reads stimecmp | virtual-instruction exception raised (cause=22) | `norm:hcounteren_acc`, `norm:henvcfg_stce` |

#### 5.3 vstimecmp CSR Read/Write

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSTC-06 | M-mode vstimecmp read/write roundtrip | M-mode writes vstimecmp=0x123456789ABCDEF0 then reads back | read-back value consistent | `norm:vstimecmp_exist` |
| HCROSS-SSTC-07 | vstimecmp all-1 / all-0 read/write | M-mode writes all-1 and all-0 to vstimecmp and reads back each | read-back values match the written values | `norm:vstimecmp_exist` |
| HCROSS-SSTC-08 | HS-mode vstimecmp read/write | STCE=1, TM=1, switch to HS-mode and read/write vstimecmp (direct access via CSR 0x24D) | read/write succeeds | `norm:vstimecmp_exist` |

#### 5.4 VSTIP Synthesis Logic

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSTC-09 | vstimecmp triggers VSTIP | Set vstimecmp to a past value (relative to time + htimedelta), check hip.VSTIP | VSTIP = 1 | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-10 | vstimecmp clears VSTIP | Set vstimecmp to the maximum value, check hip.VSTIP (assuming hvip.VSTIP=0) | VSTIP = 0 | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-11 | VSTIP = hvip.VSTIP OR vstimecmp signal | With STCE=1, verify the three-state transitions of the vstimecmp signal: vstimecmp=MAX→VSTIP=0, vstimecmp=expired→VSTIP=1, vstimecmp=MAX→VSTIP=0 | hip.VSTIP matches expectation in each state | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-12 | VSTIP restores legacy behavior with henvcfg.STCE=0 | menvcfg.STCE=1, henvcfg.STCE=0, set vstimecmp to a past value, check hip.VSTIP | VSTIP is determined only by hvip.VSTIP (the vstimecmp signal is not effective) | `norm:henvcfg_stce` |

#### 5.5 VS-mode Timer Functionality

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSTC-13 | VS-mode accesses vstimecmp via stimecmp | STCE=1, henvcfg.STCE=1, hcounteren.TM=1, VS-mode writes stimecmp (=vstimecmp), M-mode reads vstimecmp | read-back value consistent (hardware remapping verification) | `norm:sstc_vs_facility` |
| HCROSS-SSTC-14 | Effect of htimedelta on vstimecmp comparison | Set htimedelta to a large positive value, set vstimecmp such that (time+htimedelta) >= vstimecmp, check VSTIP | VSTIP = 1 | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-15 | VS-mode timer interrupt capture | henvcfg.STCE=1, enable VSTIE, set vstimecmp to a past value, VS-mode should receive a timer interrupt | the VS-mode trap handler captures cause = interrupt \| 5 | `norm:sstc_vs_facility` |

> [!NOTE]
> - The tests in this group verify the behavior of the Sstc extension under Hypervisor scenarios. All tests must detect the availability of the H extension at runtime via `HAS_H_EXT()`; if unavailable, TEST_SKIP.
> - HCROSS-SSTC-01~02 are migrated from `sstc_test_plan.md` Group 1, verifying the writability of `henvcfg.STCE` and its constraint relationship with `menvcfg.STCE`.
> - HCROSS-SSTC-03~05 are migrated from `sstc_test_plan.md` Group 3, verifying the multi-level access control of VS-mode to `stimecmp` (actually `vstimecmp`): when either `henvcfg.STCE` or `hcounteren.TM` is 0, a virtual-instruction exception (cause=22) is raised.
> - HCROSS-SSTC-06~15 are migrated from `sstc_test_plan.md` Group 6 as a whole, verifying `vstimecmp` CSR read/write, VSTIP synthesis logic (`hip.VSTIP = hvip.VSTIP OR vstimecmp_signal`), the effect of `htimedelta` on comparison, and VS-mode timer interrupt capture.
> - HCROSS-SSTC-11 verifies the OR logic of VSTIP: when `henvcfg.STCE=0`, the vstimecmp signal is not effective, and `hvip.VSTIP` becomes software-writable again and affects `hip.VSTIP`.
> - HCROSS-SSTC-13 verifies hardware transparent remapping: a VS-mode write to `stimecmp` (CSR 0x14D) actually writes `vstimecmp` (CSR 0x24D), which can be verified by reading back `vstimecmp` in M-mode.
> - HCROSS-SSTC-15 verifies the complete VS-mode timer interrupt path: VSTIP is delegated to VS-mode via `hideleg`, and the VS-mode trap handler captures cause = `interrupt | 5`.

---

## Group 6. Hypervisor × Sscsrind Cross Tests

**Spec Reference**:
- `norm:vsiselect_min_range`: vsiselect must support at least 0..0xFFF
- `norm:vsiselect_msb_op`: MSB semantics
- `norm:vsireg_access_on_legal_vsiselect`: vsireg* behavior under legal vsiselect values
- `norm:vsireg_access_behaviour`: vsireg_i access to register state, read-only 0 or exception
- `norm:sscsrind_vsmode_csrs_sz`: the width of vsiselect/vsireg* is always the current XLEN
- `norm:sscsrind_virtual_inst_fault`: VS/VU-mode direct access to vsiselect/vsireg* or VU-mode access to siselect/sireg* raises virtual-instruction
- `norm:vsmode_virtual_inst_fault`: when VS-mode accesses vsireg* via sireg*, if vsiselect is implemented at the HS level but not at the VS level, virtual-instruction is normally raised
- `norm:hypervisor_impl_csrs_access_control`: with the H extension, hstateen0[60] controls VS/VU-mode access to siselect/sireg* (actually vsiselect/vsireg*)
- `norm:sscsrind_csrs_access_control`: mstateen0[60] controls S-mode and below access to siselect/sireg*/vsiselect/vsireg*
- `norm:csrs_alias`: M-level and S-level CSRs may be aliases

**Test Scope**: Verify the behavior of the Sscsrind extension under Hypervisor scenarios, including VS-level CSR (vsiselect/vsireg*) basic functionality, Virtual-instruction exception behavior, state-enable access control, and the remapping behavior of VS-mode transparent access to vsireg* via sireg*.

> **Note**: The tests in this group are extracted from `Sscsrind_test_plan.md` Groups 2, 3, 4.2, 4.3, and 5, and specifically target cases that depend on the H extension. The H extension and the Sscsrind extension must both be available.

### Test ID Mapping Table

#### 6.1 VS-level CSR Basic Functionality (Migrated from Sscsrind Group 2)

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
| SSCSRIND-VSCSR-09 | HCROSS-SSCSRIND-09 | vsiselect WARL all-1 write |
| SSCSRIND-VSCSR-10 | HCROSS-SSCSRIND-10 | vsireg* behavior under legal vsiselect |

#### 6.2 Virtual-Instruction Exception Behavior (Migrated from Sscsrind Group 3)

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| SSCSRIND-VI-01 | HCROSS-SSCSRIND-11 | VS-mode direct read of vsiselect raises virtual-inst |
| SSCSRIND-VI-02 | HCROSS-SSCSRIND-12 | VS-mode direct write of vsiselect raises virtual-inst |
| SSCSRIND-VI-03 | HCROSS-SSCSRIND-13 | VS-mode direct read of vsireg raises virtual-inst |
| SSCSRIND-VI-04 | HCROSS-SSCSRIND-14 | VS-mode direct read of vsireg2~vsireg6 raises virtual-inst |
| SSCSRIND-VI-05 | HCROSS-SSCSRIND-15 | VS-mode direct write of vsireg raises virtual-inst |
| SSCSRIND-VI-06 | HCROSS-SSCSRIND-16 | VU-mode direct read of vsiselect raises virtual-inst |
| SSCSRIND-VI-07 | HCROSS-SSCSRIND-17 | VU-mode direct read of vsireg raises virtual-inst |
| SSCSRIND-VI-08 | HCROSS-SSCSRIND-18 | VU-mode read of siselect raises virtual-inst |
| SSCSRIND-VI-09 | HCROSS-SSCSRIND-19 | VU-mode read of sireg raises virtual-inst |
| SSCSRIND-VI-10 | HCROSS-SSCSRIND-20 | VU-mode write of siselect raises virtual-inst |
| SSCSRIND-VI-11 | HCROSS-SSCSRIND-21 | VS-mode access via sireg* with vsiselect implemented at HS level but not VS level |

#### 6.3 State-Enable Access Control (Migrated from Sscsrind Group 4.2/4.3)

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| SSCSRIND-STA-05 | HCROSS-SSCSRIND-22 | mstateen0[60]=0 blocks HS-mode read of vsiselect |
| SSCSRIND-STA-06 | HCROSS-SSCSRIND-23 | mstateen0[60]=0 blocks HS-mode read of vsireg* |
| SSCSRIND-STA-07 | HCROSS-SSCSRIND-24 | hstateen0[60]=0 + mstateen0[60]=1 → VS-mode access to siselect raises virtual-inst |
| SSCSRIND-STA-08 | HCROSS-SSCSRIND-25 | hstateen0[60]=0 + mstateen0[60]=1 → VS-mode access to sireg raises virtual-inst |
| SSCSRIND-STA-09 | HCROSS-SSCSRIND-26 | hstateen0[60]=1 + mstateen0[60]=1 → VS-mode access allowed |
| SSCSRIND-STA-10 | HCROSS-SSCSRIND-27 | Exception type with hstateen0[60]=0 is virtual-inst, not illegal-inst |

#### 6.4 Hypervisor Cross Tests (Migrated from Sscsrind Group 5)

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| SSCSRIND-HYP-01 | HCROSS-SSCSRIND-28 | VS-mode transparent remapping of vsireg* access via sireg* |
| SSCSRIND-HYP-02 | HCROSS-SSCSRIND-29 | VS-mode transparent remapping of vsiselect access via siselect |
| SSCSRIND-HYP-03 | HCROSS-SSCSRIND-30 | vsireg* read/write correct under legal vsiselect |
| SSCSRIND-HYP-04 | HCROSS-SSCSRIND-31 | vsireg* behavior under unimplemented vsiselect |
| SSCSRIND-HYP-05 | HCROSS-SSCSRIND-32 | HS-mode and VS-mode select spaces independent |
| SSCSRIND-HYP-06 | HCROSS-SSCSRIND-33 | M-mode and S-mode select spaces may be aliases |

### Test Case List

#### 6.1 VS-level CSR Basic Functionality

**Spec Reference**: `norm:vsiselect_min_range`, `norm:vsiselect_msb_op`, `norm:vsireg_access_on_legal_vsiselect`, `norm:vsireg_access_behaviour`, `norm:sscsrind_vsmode_csrs_sz`, `norm:sscsrind_csrs_access_control`, `norm:mstateen_zero_initialization`

**Preconditions**: If Smstateen is implemented, all writable `mstateen` bits are initialized to 0 at reset (`norm:mstateen_zero_initialization`), and with `mstateen0[60]`=0, access to `siselect`/`sireg*`/`vsiselect`/`vsireg*` from privilege levels lower than M-mode raises illegal-instruction (`norm:sscsrind_csrs_access_control`). Therefore, before executing the cases in this group, M-mode must first set `mstateen0[60]`=1 (`mstateen0.CSRIND`) and then enter HS-mode testing; the WARL/range behavior of `vsiselect`/`vsireg*` themselves is unrelated to stateen.

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCSRIND-01 | vsiselect readable in HS-mode | HS-mode reads vsiselect (0x250) | no exception, returns a legal value | `norm:vsiselect_min_range` |
| HCROSS-SSCSRIND-02 | vsiselect writable in HS-mode | HS-mode writes vsiselect then reads back | no exception, write 0 reads back 0 | `norm:vsiselect_min_range` |
| HCROSS-SSCSRIND-03 | vsiselect minimum range verification (0..0xFFF) | HS-mode writes vsiselect=0, 1, 0x100, 0x800, 0xFFF one by one and reads back | all values are accepted (WARL), read-back is a legal value | `norm:vsiselect_min_range` |
| HCROSS-SSCSRIND-04 | vsiselect MSB=1 custom region | HS-mode writes a value with MSB=1 to vsiselect | WARL behavior: no exception, read-back is a legal value | `norm:vsiselect_msb_op` |
| HCROSS-SSCSRIND-05 | vsiselect MSB=0 standard reserved region | HS-mode writes an unassigned value with MSB=0 to vsiselect | WARL behavior: no exception, read-back is a legal value | `norm:vsiselect_msb_op` |
| HCROSS-SSCSRIND-06 | vsireg accessible in HS-mode | HS-mode reads vsireg (0x251) (vsiselect set to 0) | behavior UNSPECIFIED: illegal-instruction or read-only 0 expected | `norm:vsireg_access_behaviour` |
| HCROSS-SSCSRIND-07 | vsireg2~vsireg6 accessible in HS-mode | HS-mode reads vsireg2~vsireg6 one by one (vsiselect set to 0) | behavior UNSPECIFIED: similar to vsireg | `norm:vsireg_access_behaviour` |
| HCROSS-SSCSRIND-08 | vsiselect/vsireg* width = current XLEN | Verify the vsiselect width in HS-mode (XLEN=64) and VS-mode (possibly XLEN=32) respectively | the width is always the current XLEN | `norm:sscsrind_vsmode_csrs_sz` |
| HCROSS-SSCSRIND-09 | vsiselect WARL all-1 write | HS-mode writes all-1 to vsiselect | no exception raised, read-back is a legal value | `norm:vsiselect_min_range` |
| HCROSS-SSCSRIND-10 | vsireg* behavior under legal vsiselect | If an implemented vsiselect value range exists, HS-mode sets that value then reads vsireg | behavior defined by the corresponding extension | `norm:vsireg_access_on_legal_vsiselect` |

#### 6.2 Virtual-Instruction Exception Behavior

**Spec Reference**: `norm:sscsrind_virtual_inst_fault`, `norm:vsmode_virtual_inst_fault`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCSRIND-11 | VS-mode direct read of vsiselect raises virtual-inst | VS-mode reads vsiselect (0x250) | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-12 | VS-mode direct write of vsiselect raises virtual-inst | VS-mode writes vsiselect | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-13 | VS-mode direct read of vsireg raises virtual-inst | VS-mode reads vsireg (0x251) | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-14 | VS-mode direct read of vsireg2~vsireg6 raises virtual-inst | VS-mode reads vsireg2~vsireg6 one by one | each access raises a virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-15 | VS-mode direct write of vsireg raises virtual-inst | VS-mode writes vsireg | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-16 | VU-mode direct read of vsiselect raises virtual-inst | VU-mode reads vsiselect (0x250) | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-17 | VU-mode direct read of vsireg raises virtual-inst | VU-mode reads vsireg (0x251) | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-18 | VU-mode read of siselect raises virtual-inst | VU-mode reads siselect (0x150) | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-19 | VU-mode read of sireg raises virtual-inst | VU-mode reads sireg (0x151) | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-20 | VU-mode write of siselect raises virtual-inst | VU-mode writes siselect | virtual-instruction exception (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-21 | VS-mode access via sireg* with vsiselect implemented at HS level but not VS level | HS-mode sets vsiselect to a value implemented at the HS level but not the VS level, VS-mode accesses via sireg* | virtual-instruction exception (cause=22) | `norm:vsmode_virtual_inst_fault` |

#### 6.3 State-Enable Access Control

**Spec Reference**: `norm:sscsrind_csrs_access_control`, `norm:hypervisor_impl_csrs_access_control`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCSRIND-22 | mstateen0[60]=0 blocks HS-mode read of vsiselect | mstateen0[60]=0, HS-mode (S-mode with V=0) reads vsiselect | illegal-instruction exception (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SSCSRIND-23 | mstateen0[60]=0 blocks HS-mode read of vsireg* | mstateen0[60]=0, HS-mode reads vsireg | illegal-instruction exception (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SSCSRIND-24 | hstateen0[60]=0 + mstateen0[60]=1 → VS-mode access to siselect raises virtual-inst | hstateen0[60]=0 and mstateen0[60]=1, VS-mode reads siselect (actually vsiselect) | virtual-instruction exception (cause=22), not illegal-instruction | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SSCSRIND-25 | hstateen0[60]=0 + mstateen0[60]=1 → VS-mode access to sireg raises virtual-inst | hstateen0[60]=0 and mstateen0[60]=1, VS-mode reads sireg (actually vsireg) | virtual-instruction exception (cause=22) | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SSCSRIND-26 | hstateen0[60]=1 + mstateen0[60]=1 → VS-mode access allowed | hstateen0[60]=1 and mstateen0[60]=1, VS-mode reads siselect/sireg* | access succeeds (constrained by the vsiselect value) | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SSCSRIND-27 | Exception type with hstateen0[60]=0 is virtual-inst, not illegal-inst | hstateen0[60]=0 and mstateen0[60]=1, VS-mode accesses siselect | the exception cause must be 22 (virtual-inst), not 2 (illegal-inst) | `norm:hypervisor_impl_csrs_access_control` |

#### 6.4 Hypervisor Cross Tests

**Spec Reference**: `norm:vsireg_access_on_legal_vsiselect`, `norm:vsireg_access_behaviour`, `norm:csrs_alias`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCSRIND-28 | VS-mode transparent remapping of vsireg* access via sireg* | hstateen0[60]=1, mstateen0[60]=1, HS-mode writes a value to vsireg, VS-mode reads back via sireg (address 0x151) | the value read by VS-mode matches the value HS-mode wrote to vsireg (hardware remaps sireg to vsireg) | `norm:vsireg_access_on_legal_vsiselect` |
| HCROSS-SSCSRIND-29 | VS-mode transparent remapping of vsiselect access via siselect | hstateen0[60]=1, mstateen0[60]=1, HS-mode writes vsiselect=0x100, VS-mode reads back via siselect (address 0x150) | the value read by VS-mode matches the value HS-mode wrote to vsiselect | `norm:vsireg_access_on_legal_vsiselect` |
| HCROSS-SSCSRIND-30 | vsireg* read/write correct under legal vsiselect | HS-mode sets vsiselect to a legal value defined by an implemented extension, writes vsireg and reads back | the read-back value matches the written value | `norm:vsireg_access_on_legal_vsiselect` |
| HCROSS-SSCSRIND-31 | vsireg* behavior under unimplemented vsiselect | HS-mode sets vsiselect to an unimplemented value (e.g., a reserved range), reads vsireg | behavior UNSPECIFIED: illegal-instruction or read-only 0 expected | `norm:vsireg_access_behaviour` |
| HCROSS-SSCSRIND-32 | HS-mode and VS-mode select spaces independent | HS-mode writes vsiselect=0x100, VS-mode reads 0x100 via siselect; VS-mode writes siselect=0x200 (raising virtual-inst or being remapped), HS-mode reads vsiselect | the vsiselect value is controlled by HS-mode; VS-mode sees the current value of vsiselect through siselect | `norm:vsireg_access_on_legal_vsiselect` |
| HCROSS-SSCSRIND-33 | M-mode and S-mode select spaces may be aliases | If the dependent extension defines an M-level and S-level alias relationship, access the same select value via miselect+sireg and siselect+sireg | M-level and S-level CSRs with the same select value may access the same or partially the same register state | `norm:csrs_alias` |

> [!NOTE]
> - The tests in this group verify the behavior of the Sscsrind extension under Hypervisor scenarios. All tests must detect the availability of the H extension at runtime via `HAS_H_EXT()`; if unavailable, TEST_SKIP.
> - HCROSS-SSCSRIND-01~10 are migrated from `Sscsrind_test_plan.md` Group 2, verifying the basic functionality of vsiselect/vsireg*. The minimum range 0..0xFFF of vsiselect is consistent with siselect, ensuring the hypervisor can emulate indirect-access registers within a VM. **Note**: if Smstateen is implemented, the cases must first set `mstateen0[60]`=1 in M-mode (the reset default of 0 would block HS-mode access to vsiselect/vsireg*).
> - HCROSS-SSCSRIND-11~21 are migrated from `Sscsrind_test_plan.md` Group 3, verifying Virtual-instruction exception behavior. **Key distinction**: VS/VU-mode direct access to vsiselect/vsireg* always raises virtual-instruction (cause=22), **regardless of** the values of mstateen0[60] or hstateen0[60]. This is the explicit requirement of `norm:sscsrind_virtual_inst_fault` in the Sscsrind SPEC.
> - HCROSS-SSCSRIND-22~27 are migrated from `Sscsrind_test_plan.md` Group 4.2/4.3, verifying state-enable access control. Key distinction: when mstateen0[60]=1 but hstateen0[60]=0, VS/VU-mode access to siselect/sireg* raises **virtual-instruction** (cause=22), not illegal-instruction (cause=2). This is because M-mode has already granted access (mstateen=1), but the hypervisor in HS-mode chooses not to grant it (hstateen=0), so the exception type reflects that the hypervisor needs to trap and handle it.
> - HCROSS-SSCSRIND-28~33 are migrated from `Sscsrind_test_plan.md` Group 5, verifying Hypervisor cross tests. HCROSS-SSCSRIND-28~29 verify the core behavior of hardware transparent remapping: within a VM, when VS-mode accesses siselect/sireg* (addresses 0x150~0x157), the hardware automatically remaps them to vsiselect/vsireg* (addresses 0x250~0x257). This is transparent to the guest OS.
> - Difference from Group 4.4 (HCROSS-SSSTA-27~29) and Smcsrind Group 1 (HCROSS-SMCSRIND-01~08) in `Hypervisor_Sm_test_plan.md`: Group 4.4 verifies hstateen0[60] control from the Ssstateen perspective, the Smcsrind group verifies mstateen0[60] control over HS-mode access to vsiselect/vsireg*, and this group verifies hstateen0[60] control and VS/VU-mode exception behavior from the Sscsrind perspective. Duplication should be avoided during implementation and can be marked as cross-references.

---

## Group 7. Hypervisor × Ssdbltrp Cross Tests

**Spec Reference**:
- `norm:henvcfg_DTE`: if the H extension is implemented, the henvcfg.DTE field is added
- `norm:henvcfg_dte_op`: when henvcfg.DTE=0, VS-mode behaves as though Ssdbltrp were not implemented; vsstatus.SDT is read-only zero
- `norm:menvcfg_dte_op`: when menvcfg.DTE=0, henvcfg.DTE is read-only zero
- `norm:vsstatus_SDT`: if the H extension is implemented, the vsstatus.SDT field is added
- `norm:vsstatus_sdt_op`: vsstatus.SDT is used for handling VS-mode double traps
- `norm:sstatus_sdt_trap`: the SDT mechanism also applies in VS-mode (via vsstatus.SDT)
- `norm:sstatus_sdt_sstatus_sie_overwrite`: mutual exclusion constraint between vsstatus.SDT and vsstatus.SIE
- `norm:sret_dt`: HS-mode SRET to VU clears vsstatus.SDT; VS-mode SRET clears vsstatus.SDT
- `norm:vsstatus_sdt_clr_mret_sret`: MRET or M-mode SRET with new mode VU clears vsstatus.SDT
- `norm:vsstatus_sdt_clr_mnret`: MNRET with new mode VU clears vsstatus.SDT
- `norm:sstatus_sdt_clr_mret_sret`: MRET or M-mode SRET with new mode VS/VU clears sstatus.SDT
- `norm:HS_mode_invoke_error`: HS-mode can invoke a critical error handler in the VM on VS-mode double trap

**Test Scope**: Verify the behavior of the Ssdbltrp extension under Hypervisor scenarios, including `henvcfg.DTE` enable/disable control over VS-mode, `vsstatus.SDT` field behavior, SRET clearing of `vsstatus.SDT`, and cross-mode clearing of SDT/vsstatus.SDT by MRET/SRET/MNRET under Hypervisor scenarios.

> **Note**: The tests in this group are extracted from `Ssdbltrp_test_plan.md` Groups 3/4/5/6/7 and specifically target cases that depend on the H extension. The H extension and the Ssdbltrp extension must both be available.

### Test ID Mapping Table

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| HDTE-01 | HCROSS-SSDBLTRP-01 | henvcfg.DTE read/write |
| HDTE-02 | HCROSS-SSDBLTRP-02 | vsstatus.SDT read-only zero with henvcfg.DTE=0 |
| HDTE-03 | HCROSS-SSDBLTRP-03 | vsstatus.SDT writable with henvcfg.DTE=1 |
| HDTE-04 | HCROSS-SSDBLTRP-04 | VS-mode trap does not set SDT with henvcfg.DTE=0 |
| HDTE-05 | HCROSS-SSDBLTRP-05 | menvcfg.DTE=0 overrides henvcfg.DTE |
| HDTE-06 | HCROSS-SSDBLTRP-06 | henvcfg.DTE dynamic switching |
| VSDT-01 | HCROSS-SSDBLTRP-07 | vsstatus.SDT WARL read/write |
| VSDT-02 | HCROSS-SSDBLTRP-08 | Writing vsstatus.SDT=1 automatically clears vsstatus.SIE |
| VSDT-03 | HCROSS-SSDBLTRP-09 | vsstatus.SIE=1 cannot be set with vsstatus.SDT=1 |
| VSDT-04 | HCROSS-SSDBLTRP-10 | vsstatus.SDT automatically set to 1 on VS-mode trap |
| VSDT-05 | HCROSS-SSDBLTRP-11 | Trap triggers double-trap with VS-mode SDT=1 |
| VSDT-06 | HCROSS-SSDBLTRP-12 | M-mode CSRs correct on VS-mode double-trap |
| SRET-03 | HCROSS-SSDBLTRP-13 | HS-mode SRET to VU clears vsstatus.SDT |
| SRET-04 | HCROSS-SSDBLTRP-14 | VS-mode SRET clears vsstatus.SDT |
| SRET-05 | HCROSS-SSDBLTRP-15 | HS-mode SRET to VS does not clear vsstatus.SDT |
| SRET-06 | HCROSS-SSDBLTRP-16 | HS-mode SRET to HS does not clear vsstatus.SDT |
| DTE-03 | HCROSS-SSDBLTRP-17 | vsstatus.SDT read-only zero with menvcfg.DTE=0 |
| DTE-04 | HCROSS-SSDBLTRP-18 | henvcfg.DTE read-only zero with menvcfg.DTE=0 |
| XRET-03 | HCROSS-SSDBLTRP-19 | MRET to VS-mode clears sstatus.SDT |
| XRET-04 | HCROSS-SSDBLTRP-20 | MRET to VU-mode clears sstatus.SDT and vsstatus.SDT |
| XRET-05 | HCROSS-SSDBLTRP-21 | MRET to VU clears only vsstatus.SDT |
| XRET-06 | HCROSS-SSDBLTRP-22 | MRET to VS-mode does not clear vsstatus.SDT |
| XRET-08 | HCROSS-SSDBLTRP-23 | M-mode SRET to VU clears sstatus.SDT and vsstatus.SDT |
| XRET-10 | HCROSS-SSDBLTRP-24 | MNRET to VU clears sstatus.SDT and vsstatus.SDT |

### Test Case List

#### 7.1 henvcfg.DTE Control

**Spec Reference**: `norm:henvcfg_DTE`, `norm:henvcfg_dte_op`, `norm:menvcfg_dte_op`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSDBLTRP-01 | henvcfg.DTE read/write | HS-mode writes henvcfg.DTE=1 then reads back (menvcfg.DTE=1) | if both Ssdbltrp and the H extension are implemented, DTE is writable and reads back consistently | `norm:henvcfg_DTE` |
| HCROSS-SSDBLTRP-02 | vsstatus.SDT read-only zero with henvcfg.DTE=0 | henvcfg.DTE=0 (menvcfg.DTE=1), VS-mode attempts to write vsstatus.SDT=1 | vsstatus.SDT reads back 0 (read-only zero) | `norm:henvcfg_dte_op` |
| HCROSS-SSDBLTRP-03 | vsstatus.SDT writable with henvcfg.DTE=1 | henvcfg.DTE=1 (menvcfg.DTE=1), write vsstatus.SDT=1 | vsstatus.SDT=1 is writable | `norm:henvcfg_dte_op` |
| HCROSS-SSDBLTRP-04 | VS-mode trap does not set SDT with henvcfg.DTE=0 | henvcfg.DTE=0, VS-mode triggers an ecall trap normally | vsstatus.SDT is not modified by hardware (feature does not exist) | `norm:henvcfg_dte_op` |
| HCROSS-SSDBLTRP-05 | menvcfg.DTE=0 overrides henvcfg.DTE | menvcfg.DTE=0, attempt to write henvcfg.DTE=1 | henvcfg.DTE is read-only zero (menvcfg.DTE is the global enable) | `norm:menvcfg_dte_op` |
| HCROSS-SSDBLTRP-06 | henvcfg.DTE dynamic switching | menvcfg.DTE=1, first set henvcfg.DTE=1 and verify vsstatus.SDT is writable; then set henvcfg.DTE=0 and verify read-only zero | writable with DTE=1; read-only zero after DTE=0 | `norm:henvcfg_dte_op` |

#### 7.2 vsstatus.SDT (VS-mode)

**Spec Reference**: `norm:vsstatus_SDT`, `norm:vsstatus_sdt_op`, `norm:sstatus_sdt_trap`, `norm:sstatus_sdt_sstatus_sie_overwrite`, `norm:HS_mode_invoke_error`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSDBLTRP-07 | vsstatus.SDT WARL read/write | HS-mode writes vsstatus.SDT=1 (henvcfg.DTE=1, menvcfg.DTE=1) then reads back | vsstatus.SDT is writable and reads back consistently | `norm:vsstatus_SDT` |
| HCROSS-SSDBLTRP-08 | Writing vsstatus.SDT=1 automatically clears vsstatus.SIE | Write vsstatus.SDT=1 (writing SIE=1 at the same time) | vsstatus.SIE is forced to zero | `norm:sstatus_sdt_sstatus_sie_overwrite` |
| HCROSS-SSDBLTRP-09 | vsstatus.SIE=1 cannot be set with vsstatus.SDT=1 | Set vsstatus.SDT=1, then write vsstatus.SIE=1 | vsstatus.SIE reads back 0 | `norm:sstatus_sdt_sstatus_sie_overwrite` |
| HCROSS-SSDBLTRP-10 | vsstatus.SDT automatically set to 1 on VS-mode trap | henvcfg.DTE=1, set vsstatus.SDT=0, trigger an ecall trap from VU-mode to VS-mode | vsstatus.SDT=1 | `norm:sstatus_sdt_trap` |
| HCROSS-SSDBLTRP-11 | Trap triggers double-trap with VS-mode SDT=1 | henvcfg.DTE=1, set vsstatus.SDT=1, VU-mode triggers an ecall (delegated to VS-mode) | double-trap exception delivered to M-mode (mcause=16), mtval2=8 (ecall-from-VU) | `norm:sstatus_sdt_trap` |
| HCROSS-SSDBLTRP-12 | M-mode CSRs correct on VS-mode double-trap | VS-mode double-trap to M-mode | mstatus.MPV=1, MPP correct, mepc=triggering address, mcause=16, mtval2=original cause | `norm:sstatus_sdt_trap` |

#### 7.3 SRET Clearing of vsstatus.SDT

**Spec Reference**: `norm:sret_dt`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSDBLTRP-13 | HS-mode SRET to VU clears vsstatus.SDT | HS-mode sets vsstatus.SDT=1, configures SRET to return to VU-mode (SPV=1, SPP=0) | vsstatus.SDT=0 | `norm:sret_dt` |
| HCROSS-SSDBLTRP-14 | VS-mode SRET clears vsstatus.SDT | VS-mode sets vsstatus.SDT=1, executes SRET | vsstatus.SDT=0 | `norm:sret_dt` |
| HCROSS-SSDBLTRP-15 | HS-mode SRET to VS does not clear vsstatus.SDT | HS-mode sets vsstatus.SDT=1, configures SRET to return to VS-mode (SPV=1, SPP=1) | vsstatus.SDT stays 1 (only VU mode clears vsstatus.SDT) | `norm:sret_dt` |
| HCROSS-SSDBLTRP-16 | HS-mode SRET to HS does not clear vsstatus.SDT | HS-mode sets vsstatus.SDT=1, configures SRET to return to HS-mode (SPV=0) | vsstatus.SDT stays 1 | `norm:sret_dt` |

#### 7.4 menvcfg.DTE Control over Hypervisor CSRs

**Spec Reference**: `norm:menvcfg_dte_op`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSDBLTRP-17 | vsstatus.SDT read-only zero with menvcfg.DTE=0 | menvcfg.DTE=0, attempt to write vsstatus.SDT=1 (with the H extension present) | vsstatus.SDT reads back 0 (read-only zero) | `norm:menvcfg_dte_op` |
| HCROSS-SSDBLTRP-18 | henvcfg.DTE read-only zero with menvcfg.DTE=0 | menvcfg.DTE=0, attempt to write henvcfg.DTE=1 (with the H extension present) | henvcfg.DTE reads back 0 (read-only zero) | `norm:menvcfg_dte_op` |

#### 7.5 MRET/SRET/MNRET Cross-Mode Clearing (Hypervisor Scenarios)

**Spec Reference**: `norm:sstatus_sdt_clr_mret_sret`, `norm:vsstatus_sdt_clr_mret_sret`, `norm:vsstatus_sdt_clr_mnret`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSDBLTRP-19 | MRET to VS-mode clears sstatus.SDT | Set sstatus.SDT=1, mstatus.MPP=1(S), MPV=1, execute MRET | sstatus.SDT=0 | `norm:sstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-20 | MRET to VU-mode clears sstatus.SDT and vsstatus.SDT | Set sstatus.SDT=1, vsstatus.SDT=1, MPP=0(U), MPV=1, execute MRET | sstatus.SDT=0 and vsstatus.SDT=0 | `norm:sstatus_sdt_clr_mret_sret` `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-21 | MRET to VU clears only vsstatus.SDT | Set sstatus.SDT=0, vsstatus.SDT=1, MPP=0(U), MPV=1, execute MRET | vsstatus.SDT=0 | `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-22 | MRET to VS-mode does not clear vsstatus.SDT | Set vsstatus.SDT=1, MPP=1(S), MPV=1, execute MRET | vsstatus.SDT stays 1 (only VU clears vsstatus.SDT) | `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-23 | M-mode SRET to VU clears sstatus.SDT and vsstatus.SDT | M-mode sets SDT=1, vsstatus.SDT=1, MPP=0, SPV=1, executes SRET | sstatus.SDT=0 and vsstatus.SDT=0 | `norm:sstatus_sdt_clr_mret_sret` `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-24 | MNRET to VU clears sstatus.SDT and vsstatus.SDT | Set SDT=1, vsstatus.SDT=1, MNPP=0(U), MNPV=1, execute MNRET | sstatus.SDT=0 and vsstatus.SDT=0 | `norm:vsstatus_sdt_clr_mnret` |

> [!NOTE]
> - The tests in this group verify the behavior of the Ssdbltrp extension under Hypervisor scenarios. All tests must detect the availability of the H extension at runtime via `HAS_H_EXT()`; if unavailable, TEST_SKIP.
> - HCROSS-SSDBLTRP-01~06 are migrated from `Ssdbltrp_test_plan.md` Group 5, verifying the writability of `henvcfg.DTE` and its enable/disable control over VS-mode Ssdbltrp functionality. `menvcfg.DTE` is the global enable, and `henvcfg.DTE` is the VS-mode-level enable.
> - HCROSS-SSDBLTRP-07~12 are migrated from `Ssdbltrp_test_plan.md` Group 6, verifying the WARL read/write of `vsstatus.SDT`, SDT/SIE mutual exclusion, and the VS-mode double-trap delivery mechanism. A VS-mode double-trap is ultimately delivered to M-mode (mcause=16), and `mstatus.MPV=1` identifies that it came from a virtualization mode.
> - HCROSS-SSDBLTRP-13~16 are migrated from `Ssdbltrp_test_plan.md` Group 3, verifying the SRET clearing behavior of `vsstatus.SDT`. Key rule: HS-mode SRET clears vsstatus.SDT **only when returning to VU-mode**; returning to VS-mode or HS-mode does not clear it. VS-mode's own SRET always clears vsstatus.SDT.
> - HCROSS-SSDBLTRP-17~18 are migrated from `Ssdbltrp_test_plan.md` Group 4, verifying the global disabling effect on Hypervisor CSRs (vsstatus.SDT, henvcfg.DTE) when `menvcfg.DTE=0`.
> - HCROSS-SSDBLTRP-19~24 are migrated from `Ssdbltrp_test_plan.md` Group 7, verifying cross-mode SDT clearing by MRET/SRET/MNRET under Hypervisor scenarios. Core rules: sstatus.SDT is cleared when the new mode is U/VS/VU; vsstatus.SDT is cleared **only when the new mode is VU**.
> - Difference from Svnapot Group 3 (HCROSS-SVNAPOT) and Group 5 (HCROSS-SSTC) in `Hypervisor_Sv_test_plan.md`: this group focuses on the behavior of the double-trap mechanism in virtualization scenarios and does not involve address translation or timer functionality.

---

## Group 8. Hypervisor × Ssctr Cross Tests

**Spec Reference**:
- `norm:Ssctr_vsctrctl_sz_acc_op`: if the H extension is implemented, `vsctrctl` is a 64-bit read-write register that replaces `sctrctl` when V=1
- `norm:vsctr-s_op`: the S field enables VS-mode recording
- `norm:vsctrctl-u_op`: the U field enables VU-mode recording
- `norm:vsctrctl-ste_op`: the STE field enables recording of traps to VS-mode
- `norm:vsctrctl-bpfrz_op`: the BPFRZ field sets FROZEN on VS-mode breakpoints
- `norm:vsctrctl-lcofifrz_op`: the LCOFIFRZ field sets FROZEN on VS-mode LCOFI
- `norm:exttrap_vshs`: VS→HS external traps require `sctrctl.STE`
- `norm:exttrap_vuhs`: VU→HS external traps require `sctrctl.STE + vsctrctl.STE`
- `norm:exttrap_vuvs`: VU→VS external traps require `vsctrctl.STE`
- `norm:ctr_freeze_vs`: VS-mode freeze behavior is determined by LCOFIFRZ/BPFRZ in `vsctrctl`
- `norm:sctrdepth_mode`: VS-mode/VU-mode access to `sctrdepth` raises virtual-instruction
- `norm:sctrclr_exceptions`: VU-mode execution of SCTRCLR raises virtual-instruction
- `norm:vsiselect_op`: when V=1, `vsireg*` provides the same CTR entry state access as `sireg*`
- `norm:hstateen_ctr` / `norm:hstateen_vs`: `hstateen0.CTR` controls VS-mode access to CTR state

**Test Scope**: Verify the behavior of the Ssctr extension under Hypervisor scenarios, including `vsctrctl` CSR functionality, VS/VU-mode external trap recording, configuration sources of virtualized mode transitions, VS-mode Freeze behavior, VS-mode access restrictions on sctrdepth/SCTRCLR, and `hstateen0.CTR` control over VS-mode CTR access.

> **Note**: The tests in this group are extracted from `Ssctr_test_plan.md` Groups 2/3/5/8/12/13/14 and specifically target cases that depend on the H extension. The H extension and the Ssctr extension must both be available.

### Test ID Mapping Table

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| SSCTR-VSCTL-01 | HCROSS-SSCTR-01 | vsctrctl basic read/write (HS-mode) |
| SSCTR-VSCTL-02 | HCROSS-SSCTR-02 | With V=1, sctrctl actually accesses vsctrctl |
| SSCTR-VSCTL-03 | HCROSS-SSCTR-03 | With V=1, writing sctrctl actually writes vsctrctl |
| SSCTR-VSCTL-04 | HCROSS-SSCTR-04 | vsctrctl.S field (VS-mode recording) |
| SSCTR-VSCTL-05 | HCROSS-SSCTR-05 | vsctrctl.U field (VU-mode recording) |
| SSCTR-VSCTL-06 | HCROSS-SSCTR-06 | vsctrctl.STE field |
| SSCTR-VSCTL-07 | HCROSS-SSCTR-07 | vsctrctl.BPFRZ field |
| SSCTR-VSCTL-08 | HCROSS-SSCTR-08 | vsctrctl.LCOFIFRZ field |
| SSCTR-VSCTL-09 | HCROSS-SSCTR-09 | vsctrctl does not affect behavior with V=0 |
| SSCTR-VSCTL-10 | HCROSS-SSCTR-10 | vsctrctl fields match sctrctl |
| SSCTR-DEP-07 | HCROSS-SSCTR-11 | VS-mode access to sctrdepth raises an exception |
| SSCTR-DEP-08 | HCROSS-SSCTR-12 | VU-mode access to sctrdepth raises an exception |
| SSCTR-ENT-10 | HCROSS-SSCTR-13 | vsireg* and sireg* access the same state |
| SSCTR-ENT-18 | HCROSS-SSCTR-14 | VU-mode execution of SCTRCLR raises an exception |
| SSCTR-EXT-04 | HCROSS-SSCTR-15 | VS→HS external trap requires STE |
| SSCTR-EXT-05 | HCROSS-SSCTR-16 | VS→HS external trap not recorded with STE=0 |
| SSCTR-EXT-06 | HCROSS-SSCTR-17 | VU→VS external trap requires vsctrctl.STE |
| SSCTR-EXT-07 | HCROSS-SSCTR-18 | VU→VS external trap not recorded with vsSTE=0 |
| SSCTR-EXT-08 | HCROSS-SSCTR-19 | VU→HS external trap requires STE + vsSTE |
| SSCTR-EXT-09 | HCROSS-SSCTR-20 | VU→HS not recorded without vsSTE |
| SSCTR-FRZ-09 | HCROSS-SSCTR-21 | VS-mode BPFRZ controlled by vsctrctl |
| SSCTR-FRZ-10 | HCROSS-SSCTR-22 | VS-mode BPFRZ=0 does not set FROZEN |
| SSCTR-FRZ-11 | HCROSS-SSCTR-23 | VS-mode LCOFIFRZ controlled by vsctrctl |
| SSCTR-FRZ-12 | HCROSS-SSCTR-24 | Virtual LCOFI also triggers freeze |
| SSCTR-VIRT-01 | HCROSS-SSCTR-25 | VU→HS transition uses vsctrctl filter bits |
| SSCTR-VIRT-02 | HCROSS-SSCTR-26 | VS→HS transition uses vsctrctl/sctrctl fields |
| SSCTR-VIRT-03 | HCROSS-SSCTR-27 | HS→VS/VU trap return uses sctrctl |
| SSCTR-VIRT-04 | HCROSS-SSCTR-28 | Freeze uses sctrctl (trap to HS-mode) |
| SSCTR-VIRT-05 | HCROSS-SSCTR-29 | VS-mode freeze uses vsctrctl |
| SSCTR-SEA-02 | HCROSS-SSCTR-30 | VS-mode detects CTR accessibility |
| SSCTR-SEA-06 | HCROSS-SSCTR-31 | VS-mode sctrstatus access with hstateen0.CTR=0 |
| SSCTR-SEA-07 | HCROSS-SSCTR-32 | VS-mode SCTRCLR execution with hstateen0.CTR=0 |

### Test Case List

#### 8.1 vsctrctl CSR (VS-mode CTR Control Register)

**Spec Reference**: `norm:Ssctr_vsctrctl_sz_acc_op`, `norm:vsctr-s_op`, `norm:vsctrctl-u_op`, `norm:vsctrctl-ste_op`, `norm:vsctrctl-bpfrz_op`, `norm:vsctrctl-lcofifrz_op`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCTR-01 | vsctrctl basic read/write (HS-mode) | HS-mode writes each field via the vsctrctl address and reads back | implemented fields read back consistently | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-02 | With V=1, sctrctl actually accesses vsctrctl | HS-mode writes a specific value to vsctrctl, enters VS-mode and reads with csrr sctrctl | reads the value of vsctrctl | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-03 | With V=1, writing sctrctl actually writes vsctrctl | VS-mode writes a value with csrw sctrctl, returns to HS-mode and reads with csrr vsctrctl | vsctrctl is modified | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-04 | vsctrctl.S field (VS-mode recording) | vsctrctl.S=1, VS-mode performs control transfers | transfers are recorded | `norm:vsctr-s_op` |
| HCROSS-SSCTR-05 | vsctrctl.U field (VU-mode recording) | vsctrctl.U=1, VU-mode performs control transfers | transfers are recorded | `norm:vsctrctl-u_op` |
| HCROSS-SSCTR-06 | vsctrctl.STE field | Write vsctrctl.STE=1 then read back | if the H extension is implemented, STE is writable | `norm:vsctrctl-ste_op` |
| HCROSS-SSCTR-07 | vsctrctl.BPFRZ field | Write vsctrctl.BPFRZ=1 then read back | BPFRZ must be implemented and writable | `norm:vsctrctl-bpfrz_op` |
| HCROSS-SSCTR-08 | vsctrctl.LCOFIFRZ field | Write vsctrctl.LCOFIFRZ=1 then read back | if Sscofpmf is implemented, LCOFIFRZ must be writable | `norm:vsctrctl-lcofifrz_op` |
| HCROSS-SSCTR-09 | vsctrctl does not affect behavior with V=0 | HS-mode writes vsctrctl.S=1, HS-mode itself performs transitions | HS-mode transitions are not affected by vsctrctl | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-10 | vsctrctl fields match sctrctl | Check whether the optional fields of vsctrctl are consistent with sctrctl | optional fields implemented in vsctrctl should match sctrctl | `norm:Ssctr_vsctrctl_sz_acc_op` |

#### 8.2 VS/VU-mode Access Restrictions on CTR CSRs

**Spec Reference**: `norm:sctrdepth_mode`, `norm:sctrclr_exceptions`, `norm:vsiselect_op`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCTR-11 | VS-mode access to sctrdepth raises an exception | VS-mode attempts to access sctrdepth | virtual-instruction exception raised (cause=22) | `norm:sctrdepth_mode` |
| HCROSS-SSCTR-12 | VU-mode access to sctrdepth raises an exception | VU-mode attempts to access sctrdepth | virtual-instruction exception raised (cause=22) | `norm:sctrdepth_mode` |
| HCROSS-SSCTR-13 | vsireg* and sireg* access the same state | HS-mode sets vsiselect=0x200 and writes a value via vsireg; then sets siselect=0x200 and reads via sireg | reads the same value (shared entry register state) | `norm:vsiselect_op` |
| HCROSS-SSCTR-14 | VU-mode execution of SCTRCLR raises an exception | VU-mode executes SCTRCLR | virtual-instruction exception raised (cause=22) | `norm:sctrclr_exceptions` |

#### 8.3 VS/VU-mode External Trap Recording

**Spec Reference**: `norm:exttrap_vshs`, `norm:exttrap_vuhs`, `norm:exttrap_vuvs`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCTR-15 | VS→HS external trap requires STE | sctrctl.S=0, vsctrctl.S=1, sctrctl.STE=1, VS-mode raises a trap to HS-mode | the external trap is recorded | `norm:exttrap_vshs` |
| HCROSS-SSCTR-16 | VS→HS external trap not recorded with STE=0 | sctrctl.S=0, vsctrctl.S=1, sctrctl.STE=0, VS-mode raises a trap to HS-mode | the external trap is not recorded | `norm:exttrap_vshs` |
| HCROSS-SSCTR-17 | VU→VS external trap requires vsctrctl.STE | vsctrctl.S=0, vsctrctl.U=1, vsctrctl.STE=1, VU-mode raises a trap to VS-mode | the external trap is recorded | `norm:exttrap_vuvs` |
| HCROSS-SSCTR-18 | VU→VS external trap not recorded with vsSTE=0 | vsctrctl.S=0, vsctrctl.U=1, vsctrctl.STE=0, VU-mode raises a trap to VS-mode | the external trap is not recorded | `norm:exttrap_vuvs` |
| HCROSS-SSCTR-19 | VU→HS external trap requires STE + vsSTE | sctrctl.S=0, sctrctl.STE=1, vsctrctl.U=1, vsctrctl.STE=1, VU-mode raises a trap to HS-mode | the external trap is recorded | `norm:exttrap_vuhs` |
| HCROSS-SSCTR-20 | VU→HS not recorded without vsSTE | sctrctl.S=0, sctrctl.STE=1, vsctrctl.U=1, vsctrctl.STE=0, VU-mode raises a trap to HS-mode | the external trap is not recorded | `norm:exttrap_vuhs` |

#### 8.4 VS-mode Freeze Behavior (vsctrctl Control)

**Spec Reference**: `norm:ctr_freeze_vs`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCTR-21 | VS-mode BPFRZ controlled by vsctrctl | vsctrctl.BPFRZ=1, VS-mode executes EBREAK trapping to VS-mode | sctrstatus.FROZEN=1 | `norm:ctr_freeze_vs` |
| HCROSS-SSCTR-22 | VS-mode BPFRZ=0 does not set FROZEN | vsctrctl.BPFRZ=0, VS-mode executes EBREAK | sctrstatus.FROZEN unchanged | `norm:ctr_freeze_vs` |
| HCROSS-SSCTR-23 | VS-mode LCOFIFRZ controlled by vsctrctl | vsctrctl.LCOFIFRZ=1, LCOFI traps to VS-mode | sctrstatus.FROZEN=1 | `norm:ctr_freeze_vs` |
| HCROSS-SSCTR-24 | Virtual LCOFI also triggers freeze | vsctrctl.LCOFIFRZ=1, hypervisor injects a virtual LCOFI, trapping to VS-mode | sctrstatus.FROZEN=1 | `norm:ctr_freeze_vs` |

#### 8.5 Configuration Sources of Virtualized Mode Transitions

**Spec Reference**: virtualized mode transitions use the fields in vsctrctl (except M/MTE/LCOFIFRZ/BPFRZ, which are controlled by sctrctl/mctrctl)

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCTR-25 | VU→HS transition uses vsctrctl filter bits | vsctrctl.U=1, sctrctl.S=1, set a filter bit in vsctrctl, VU-mode raises a trap to HS-mode | whether the transition is recorded is determined by the filter bit in vsctrctl (not sctrctl) | `norm:vsctr-s_op` |
| HCROSS-SSCTR-26 | VS→HS transition uses vsctrctl/sctrctl fields | vsctrctl.S=1, sctrctl.S=1, VS-mode raises a trap to HS-mode | source-mode enable is determined by vsctrctl.S, destination-mode enable by sctrctl.S | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-27 | HS→VS/VU trap return uses sctrctl | sctrctl.S=1, vsctrctl.S=1, HS-mode executes SRET returning to VS-mode | source-mode enable is determined by sctrctl.S, destination mode by vsctrctl.S | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-28 | Freeze uses sctrctl (trap to HS-mode) | sctrctl.BPFRZ=1, VS-mode breakpoint traps to HS-mode | freeze is determined by sctrctl.BPFRZ | `norm:ctr_freeze_bp` |
| HCROSS-SSCTR-29 | VS-mode freeze uses vsctrctl | vsctrctl.BPFRZ=1, VS-mode breakpoint traps to VS-mode | freeze is determined by vsctrctl.BPFRZ | `norm:ctr_freeze_vs` |

#### 8.6 hstateen0.CTR Control over VS-mode CTR Access

**Spec Reference**: `norm:hstateen_ctr`, `norm:hstateen_vs`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCTR-30 | VS-mode detects CTR accessibility | VS-mode attempts to access sctrctl and observes whether an exception is raised | if hstateen0.CTR=1, access succeeds; if CTR=0, virtual-instruction is raised | `norm:hstateen_vs` |
| HCROSS-SSCTR-31 | VS-mode sctrstatus access with hstateen0.CTR=0 | hstateen0.CTR=0, VS-mode accesses sctrstatus | virtual-instruction raised | `norm:hstateen_vs` |
| HCROSS-SSCTR-32 | VS-mode SCTRCLR execution with hstateen0.CTR=0 | hstateen0.CTR=0, VS-mode executes SCTRCLR | virtual-instruction raised | `norm:hstateen_vs` |

> [!NOTE]
> - The tests in this group verify the behavior of the Ssctr extension under Hypervisor scenarios. All tests must detect the availability of the H extension at runtime via `HAS_H_EXT()`; if unavailable, TEST_SKIP.
> - HCROSS-SSCTR-01~10 are migrated from `Ssctr_test_plan.md` Group 2, verifying the basic functionality of the `vsctrctl` CSR. `vsctrctl` is a CSR introduced by the H extension; it replaces `sctrctl` when V=1, and does not affect behavior when V=0.
> - HCROSS-SSCTR-11~14 are migrated from `Ssctr_test_plan.md` Groups 3/5, verifying VS/VU-mode access restrictions on CTR CSRs. VS/VU-mode access to `sctrdepth` and execution of SCTRCLR raise virtual-instruction (cause=22). HCROSS-SSCTR-13 verifies that `vsireg*` and `sireg*` share the same set of CTR entry register state (there is no separate entry register set when V=1).
> - HCROSS-SSCTR-15~20 are migrated from `Ssctr_test_plan.md` Group 8, verifying VS/VU-mode external trap recording. External trap recording depends on the TE bits of intermediate modes: VS→HS requires sctrctl.STE; VU→VS requires vsctrctl.STE; VU→HS requires sctrctl.STE + vsctrctl.STE (both TE bits must be set).
> - HCROSS-SSCTR-21~24 are migrated from `Ssctr_test_plan.md` Group 12, verifying that VS-mode Freeze behavior is controlled by BPFRZ/LCOFIFRZ in `vsctrctl` (not `sctrctl`). Note: when trapping to HS-mode, freeze is determined by `sctrctl.BPFRZ` (HCROSS-SSCTR-28).
> - HCROSS-SSCTR-25~29 are migrated from `Ssctr_test_plan.md` Group 13, verifying the configuration source selection on virtualized mode transitions. Core rules: source-mode enable is determined by the xctrctl corresponding to that mode (VS-mode uses vsctrctl, HS-mode uses sctrctl); freeze is determined by the xctrctl of the trap destination mode (trapping to VS-mode uses vsctrctl, trapping to HS-mode uses sctrctl).
> - HCROSS-SSCTR-30~32 are migrated from `Ssctr_test_plan.md` Group 14, verifying the `hstateen0.CTR` control over VS-mode CTR access. With `hstateen0.CTR=0`, VS-mode access to CTR state raises virtual-instruction (cause=22), not illegal-instruction.
> - Difference from Smctr Group 2 (HCROSS-SMCTR) in `Hypervisor_Sm_test_plan.md`: the Smctr group verifies, from the M-mode perspective, the control of `mstateen0.CTR` and `hstateen0.CTR` over lower privilege levels, and MTE external trap recording to M-mode; this group verifies, from the S-mode/VS-mode perspective, `vsctrctl` functionality and VS/VU-mode CTR behavior.

---

## Group 9. Hypervisor × Ssqosid Cross Tests

The tests in this group verify the behavior of the Ssqosid extension under Hypervisor scenarios, namely the exception behavior of VS/VU-mode access to the `srmcfg` CSR when V=1. These tests are migrated from `Ssqosid_test_plan.md` and specifically target cases that depend on the H extension.

**Spec Reference**:
- `ssqosid_virtinst`: if mstateen0[55]=1 or Smstateen is not implemented, attempting to access srmcfg when V=1 raises a virtual-instruction exception
- `ssqosid_smstateen_bit55_0`: if mstateen0[55]=0, access to srmcfg from privilege levels lower than M-mode raises an illegal-instruction exception

**Test Scope**: Verify the access exception behavior of srmcfg under virtualization modes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SRMCFG-19 | V=1 VS-mode read of srmcfg raises virtual-instruction exception | Smstateen not implemented or mstateen0[55]=1, VS-mode executes csrr srmcfg | virtual-instruction exception (cause=22) |
| SRMCFG-20 | V=1 VS-mode write of srmcfg raises virtual-instruction exception | Smstateen not implemented or mstateen0[55]=1, VS-mode executes csrw srmcfg | virtual-instruction exception (cause=22) |
| SRMCFG-21 | V=1 VU-mode access to srmcfg raises an exception | VU-mode attempts to access srmcfg (CSR 0x181 is an S-level CSR) | illegal-instruction (cause=2) or virtual-instruction (cause=22) (ambiguity exists in specification precedence) |
| SRMCFG-22 | V=0 HS-mode accesses srmcfg normally | V=0 (HS-mode), csrr/csrw srmcfg | read/write succeeds normally, no exception |
| SRMCFG-23 | V=1 access with Smstateen implemented and mstateen0[55]=0 | mstateen0[55]=0, VS-mode attempts csrr srmcfg | illegal-instruction exception (cause=2) (mstateen0 gating takes precedence over the V=1 rule) |
| SRMCFG-24 | stval/htinst values on virtual-instruction trap | VS-mode accesses srmcfg raising a virtual-instruction exception, check stval and htinst | stval is 0 or the faulting instruction encoding (SYSTEM opcode), htinst is 0 or the transformed value |

### Test ID Mapping Table

| Original ID | New Location | Test Name |
|-------------|--------------|-----------|
| SRMCFG-19 | Group 9 | V=1 VS-mode read of srmcfg |
| SRMCFG-20 | Group 9 | V=1 VS-mode write of srmcfg |
| SRMCFG-21 | Group 9 | V=1 VU-mode access to srmcfg |
| SRMCFG-22 | Group 9 | V=0 HS-mode normal access |
| SRMCFG-23 | Group 9 | mstateen0[55]=0 V=1 access |
| SRMCFG-24 | Group 9 | stval/htinst value verification |

### Implementation Notes

1. **Extension detection**: Before testing, the availability of Ssqosid (presence of the srmcfg CSR 0x181) and the H extension (misa.H) must be detected; if unavailable, TEST_SKIP.

2. **Smstateen interaction**: If Smstateen is implemented, mstateen0[55]=1 must be set before tests SRMCFG-19/20/21/22/24 to ensure that the V=1 rule rather than mstateen gating is being tested.

3. **SRMCFG-21 specification ambiguity**: When VU-mode (effective privilege = U) accesses an S-level CSR, the standard CSR access rules give cause=2 (illegal-instruction), while the "when V=1" wording of the Ssqosid SPEC suggests cause=22 (virtual-instruction). The test accepts both cause values.

4. **SRMCFG-23 precedence**: The gating rule of mstateen0[55]=0 takes precedence over the V=1 virtual-instruction rule, so VS-mode access should raise illegal-instruction (cause=2).

---

## Group 10. Hypervisor × Sscofpmf Cross Tests

**Spec Reference**:
- `norm:mhpmevent_inh_op`: when each of the five xINH bits is set, event counting in the corresponding privilege mode is inhibited; VSINH/VUINH inhibit VS/VU-mode counting respectively; when the corresponding privilege mode is not implemented, the bit is read-only zero
- `norm:scountovf_vsmode_read_access`: in VS-mode, `scountovf` bit X is readable if and only if both `mcounteren` bit X and `hcounteren` bit X are set; otherwise it reads as zero
- `norm:scountovf_smode_read_access_control`: read access to `scountovf` bit X is controlled by the same `mcounteren`/`hcounteren` rules as hpmcounter access

**Test Scope**: Verify the behavior of the Sscofpmf extension under Hypervisor scenarios, including the dual gating of `mcounteren`+`hcounteren` for VS-mode reads of `scountovf`, and the inhibition of VS/VU-mode event counting by the VSINH/VUINH bits of `mhpmevent`.

> **Note**: Cases 01~03 of this group are migrated from `Sscofpmf_test_plan.md` Group 4 (COFPMF-SOV-08~10) and specifically target cases that depend on the H extension; 04~06 supplement the VSINH/VUINH functional cases missing from Group 2 (privilege mode filtering) of the original plan. The H extension and the Sscofpmf extension must both be available (except 06, see below).

### Test ID Mapping Table

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| COFPMF-SOV-08 | HCROSS-SSCOFPMF-01 | VS-mode scountovf dual gate (both allowed) |
| COFPMF-SOV-09 | HCROSS-SSCOFPMF-02 | VS-mode mcounteren=0 reads zero |
| COFPMF-SOV-10 | HCROSS-SSCOFPMF-03 | VS-mode hcounteren=0 reads zero |
| — (new) | HCROSS-SSCOFPMF-04 | VSINH=1 inhibits VS-mode counting |
| — (new) | HCROSS-SSCOFPMF-05 | VUINH=1 inhibits VU-mode counting |
| — (new) | HCROSS-SSCOFPMF-06 | VSINH/VUINH read-only zero when H extension is not implemented |

### Test Case List

#### 10.1 VS-mode scountovf Dual Gating (Migrated from Sscofpmf Group 4)

**Spec Reference**: `norm:scountovf_vsmode_read_access`, `norm:scountovf_smode_read_access_control`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCOFPMF-01 | VS-mode scountovf dual gate (both allowed) | mcounteren bit 3 = 1, hcounteren bit 3 = 1, M-mode sets mhpmevent3 OF=1, VS-mode reads scountovf | scountovf bit 3 = 1 (reads the real OF value) | `norm:scountovf_vsmode_read_access` |
| HCROSS-SSCOFPMF-02 | VS-mode mcounteren=0 reads zero | mcounteren bit 3 = 0 (regardless of hcounteren), OF=1, VS-mode reads scountovf | scountovf bit 3 = 0 | `norm:scountovf_vsmode_read_access` |
| HCROSS-SSCOFPMF-03 | VS-mode hcounteren=0 reads zero | mcounteren bit 3 = 1, hcounteren bit 3 = 0, OF=1, VS-mode reads scountovf | scountovf bit 3 = 0 | `norm:scountovf_vsmode_read_access` |

#### 10.2 VSINH/VUINH Counting Inhibition (Supplementing the Gap in Group 2 of the Original Plan)

**Spec Reference**: `norm:mhpmevent_inh_op`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCOFPMF-04 | VSINH=1 inhibits VS-mode counting | Set mhpmevent VSINH=1 and configure a retired-instruction event, VS-mode executes a fixed instruction loop, M-mode reads the counter delta | the counter does not increment during VS-mode execution | `norm:mhpmevent_inh_op` |
| HCROSS-SSCOFPMF-05 | VUINH=1 inhibits VU-mode counting | Set mhpmevent VUINH=1, VU-mode executes a fixed instruction loop, M-mode reads the counter delta | the counter does not increment during VU-mode execution | `norm:mhpmevent_inh_op` |
| HCROSS-SSCOFPMF-06 | VSINH/VUINH read-only zero when H extension is not implemented | If the H extension is not implemented, write mhpmevent VSINH/VUINH=1 then read back | VSINH/VUINH are read-only zero (TEST_SKIP on platforms with the H extension implemented) | `norm:mhpmevent_inh_op` |

> [!NOTE]
> - This group of tests verifies the behavior of the Sscofpmf extension under Hypervisor scenarios. HCROSS-SSCOFPMF-01~05 must detect the availability of the H extension at runtime via `HAS_H_EXT()`; if unavailable, TEST_SKIP; HCROSS-SSCOFPMF-06 runs only when the H extension is **not implemented**, and TEST_SKIP when the H extension is implemented.
> - All cases must first probe whether Sscofpmf is implemented (trap-protected write of the `mhpmevent` OF bit and read back; if the write raises illegal-instruction it is not implemented); if not implemented, the whole group TEST_SKIP; and adopt the dynamic discovery method to probe the target `mhpmcounter` (writing a non-zero value and reading back zero means unimplemented; switch to another counter or TEST_SKIP).
> - The access control semantics of HCROSS-SSCOFPMF-01~03 are consistent with `hpmcounter`: for VS-mode to read the real OF value of `scountovf` bit X, both `mcounteren[X]` and `hcounteren[X]` must be 1 **simultaneously**; otherwise it reads zero (note that it reads zero rather than raising a trap).
> - HCROSS-SSCOFPMF-04~05 need to use `goto_priv(PRIV_VS)`/`goto_priv(PRIV_VU)` to enter virtual privilege levels to execute counting loops, and return to M-mode to read the `mhpmcounter` delta; no precise counting assertions are made. Mode switching itself generates instruction counts; MINH=1 can be set to inhibit M-mode counting to eliminate interference (consistent with the handling in the NOTE of `Sscofpmf_test_plan.md` Group 2); the `ENABLE_HYP` macro must be enabled at compile time and two-stage translation configured so that VS/VU-mode can execute.
> - Relationship with COFPMF-RW-05/06 of `Sscofpmf_test_plan.md`: the original cases verify the positive branch of WARL read/write of VSINH/VUINH (not dependent on the H extension) and remain in the standalone plan; 06 of this group takes over the negative branch of "read-only zero when the H extension is not implemented", and 04~05 take over the counting inhibition functional verification that depends on the H extension. VSINH/VUINH share the same bit encoding (bit 59/58) with the VSINH/VUINH of `mcyclecfg`/`minstretcfg` of Smcntrpmf, but they belong to different registers, with no overlap with `Hypervisor_Sm_test_plan.md` Group 3.
> - Relationship with Group 11 (Smcdeleg/Ssccfg): this group verifies the `mcounteren`+`hcounteren` gating (read-zero semantics) of VS-mode reads of `scountovf` when **counter delegation is not enabled** (CDE not involved); Group 11 verifies that VS/VU-mode reads of `scountovf` are virtualized when **CDE=1** (virtual-instruction semantics, `norm:ssccfg_virtual_scountovf_vs_vu`). The two are complementary; the precondition state of `menvcfg.CDE` must be noted during implementation.

---

## Group 11. Hypervisor × Smcdeleg/Ssccfg Cross Tests

**Spec Reference**:
- `norm:ssccfg_virtual_scountovf_vs_vu`: for implementations supporting Smcdeleg/Ssccfg, Sscofpmf, and the H extension, when `menvcfg.CDE=1`, VS/VU-mode reads of `scountovf` raise a virtual-instruction exception
- `norm:ssccfg_illegal_scountinhibit_vs_vu`: when counter delegation is enabled (CDE=1), VS/VU-mode access to `scountinhibit` raises a virtual-instruction exception
- `norm:ssccfg_lcofi_hvip_hvien`: for implementations supporting Smcdeleg/Ssccfg, Sscofpmf, Smaia/Ssaia, and the H extension, the LCOFI bit (bit 13) of `hvip` and `hvien` is implemented and writable; this implies that `vsie`/`vsip` bit 13 is also implemented
- `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal`: when the H extension is implemented, direct VS/VU-mode access to `vsiselect`/`vsireg*`, or VU-mode access to `siselect`/`sireg*`, raises virtual-instruction
- `norm:ssccfg_hyp_m_s_vsireg_illegal`: when `vsiselect` is in the 0x40-0x5F range, M or S mode access to any `vsireg*` raises illegal-instruction
- `norm:ssccfg_hyp_vs_access_sireg_conditional`: when VS-mode accesses `sireg*` (actually `vsireg*`), `menvcfg.CDE=0` raises illegal-instruction and CDE=1 raises virtual-instruction
- `norm:hstateen0_csrind_op`: hstateen0 bit 60 controls VS-mode access to siselect/sireg* (actually vsiselect/vsireg*)

**Test Scope**: Verify the virtualization behavior of the Smcdeleg/Ssccfg counter delegation extension under Hypervisor scenarios, including VS/VU-mode virtualization of `scountovf`/`scountinhibit`, the `hvip`/`hvien` LCOFI virtual interrupt bits, the multi-privilege access rules of `vsiselect`/`vsireg*`, and the cross control of hstateen0 bit 60.

> **Note**: The cases of this group are migrated from `Ssccfg_test_plan.md` Groups 4/5/6/8 (SSCFG-OVF-01~04, SSCFG-HLCOFI-01~05, SSCFG-HYP-01~10, SSCFG-STA-04~06) and specifically target cases that depend on the H extension; 05~06 supplement the gap of no corresponding case for `norm:ssccfg_illegal_scountinhibit_vs_vu` in the original plan. The H extension and Smcdeleg/Ssccfg (including `menvcfg.CDE`) must both be available; some sub-groups require additional extensions (see the description of each sub-group).
>
> **Prerequisite configuration**: M-mode must pre-set `menvcfg.CDE` to the desired value and delegate the target counters (the corresponding bits of `mcounteren`).

### Test ID Mapping Table

| Original ID | New ID | Test Name |
|-------------|--------|-----------|
| SSCFG-OVF-01 | HCROSS-SSCCFG-01 | VS-mode read of scountovf (CDE=1) raises virtual-instruction |
| SSCFG-OVF-02 | HCROSS-SSCCFG-02 | VU-mode read of scountovf (CDE=1) raises virtual-instruction |
| SSCFG-OVF-03 | HCROSS-SSCCFG-03 | HS-mode read of scountovf (CDE=1) succeeds |
| SSCFG-OVF-04 | HCROSS-SSCCFG-04 | VS-mode read of scountovf (CDE=0) behavior |
| — (new) | HCROSS-SSCCFG-05 | VS-mode access to scountinhibit (CDE=1) raises virtual-instruction |
| — (new) | HCROSS-SSCCFG-06 | VU-mode access to scountinhibit (CDE=1) raises virtual-instruction |
| SSCFG-HLCOFI-01 | HCROSS-SSCCFG-07 | hvip bit 13 (LCOFI) writability |
| SSCFG-HLCOFI-02 | HCROSS-SSCCFG-08 | hvien bit 13 (LCOFI) writability |
| SSCFG-HLCOFI-03 | HCROSS-SSCCFG-09 | hvip.LCOFI independent verification |
| SSCFG-HLCOFI-04 | HCROSS-SSCCFG-10 | hvien.LCOFI independent verification |
| SSCFG-HLCOFI-05 | HCROSS-SSCCFG-11 | vsie/vsip LCOFI bits implicitly implemented |
| SSCFG-HYP-01 | HCROSS-SSCCFG-12 | VS-mode direct access to vsiselect raises virtual-instruction |
| SSCFG-HYP-02 | HCROSS-SSCCFG-13 | VS-mode direct access to vsireg raises virtual-instruction |
| SSCFG-HYP-03 | HCROSS-SSCCFG-14 | VU-mode direct access to vsiselect raises virtual-instruction |
| SSCFG-HYP-04 | HCROSS-SSCCFG-15 | VU-mode direct access to vsireg raises virtual-instruction |
| SSCFG-HYP-05 | HCROSS-SSCCFG-16 | VU-mode access to siselect raises virtual-instruction |
| SSCFG-HYP-06 | HCROSS-SSCCFG-17 | VU-mode access to sireg raises virtual-instruction |
| SSCFG-HYP-07 | HCROSS-SSCCFG-18 | M-mode access to vsireg illegal with vsiselect 0x40-0x5F |
| SSCFG-HYP-08 | HCROSS-SSCCFG-19 | HS-mode access to vsireg illegal with vsiselect 0x40-0x5F |
| SSCFG-HYP-09 | HCROSS-SSCCFG-20 | VS-mode access via sireg* (CDE=0) → illegal-instruction |
| SSCFG-HYP-10 | HCROSS-SSCCFG-21 | VS-mode access via sireg* (CDE=1) → virtual-instruction |
| SSCFG-STA-04 | HCROSS-SSCCFG-22 | hstateen0 bit 60=0 blocks VS-mode write of siselect |
| SSCFG-STA-05 | HCROSS-SSCCFG-23 | hstateen0 bit 60=0 blocks VS-mode read of sireg |
| SSCFG-STA-06 | HCROSS-SSCCFG-24 | hstateen0 bit 60=1 allows VS-mode access |

### Test Case List

#### 11.1 scountovf Virtualization (Migrated from Ssccfg Group 4)

**Spec Reference**: `norm:ssccfg_virtual_scountovf_vs_vu`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCCFG-01 | VS-mode read of scountovf (CDE=1) raises virtual-instruction | CDE=1, VS-mode reads scountovf (0xDA0) | virtual-instruction exception raised (cause=22) | `norm:ssccfg_virtual_scountovf_vs_vu` |
| HCROSS-SSCCFG-02 | VU-mode read of scountovf (CDE=1) raises virtual-instruction | CDE=1, VU-mode reads scountovf | virtual-instruction exception raised (cause=22) | `norm:ssccfg_virtual_scountovf_vs_vu` |
| HCROSS-SSCCFG-03 | HS-mode read of scountovf (CDE=1) succeeds | CDE=1, HS-mode (S-mode with V=0) reads scountovf | access succeeds, no exception | `norm:ssccfg_virtual_scountovf_vs_vu` |
| HCROSS-SSCCFG-04 | VS-mode read of scountovf (CDE=0) behavior | CDE=0, VS-mode reads scountovf | not constrained by this virtualization clause; follows the basic Sscofpmf rules (see the gating semantics of Group 10 HCROSS-SSCOFPMF-01~03) | `norm:ssccfg_virtual_scountovf_vs_vu` (negative) |

#### 11.2 scountinhibit VS/VU Virtualization (Supplementing the Gap)

**Spec Reference**: `norm:ssccfg_illegal_scountinhibit_vs_vu`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCCFG-05 | VS-mode access to scountinhibit (CDE=1) raises virtual-instruction | CDE=1, VS-mode reads/writes scountinhibit (0x120) | virtual-instruction exception raised (cause=22) | `norm:ssccfg_illegal_scountinhibit_vs_vu` |
| HCROSS-SSCCFG-06 | VU-mode access to scountinhibit (CDE=1) raises virtual-instruction | CDE=1, VU-mode reads scountinhibit | virtual-instruction exception raised (cause=22) | `norm:ssccfg_illegal_scountinhibit_vs_vu` |

#### 11.3 LCOFI Virtualization: hvip/hvien bit 13 (Migrated from Ssccfg Group 5)

**Spec Reference**: `norm:ssccfg_lcofi_hvip_hvien`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCCFG-07 | hvip bit 13 (LCOFI) writability | HS-mode writes hvip bit 13 = 1 then reads back, then writes 0 and reads back | bit 13 is writable and reads back consistently | `norm:ssccfg_lcofi_hvip_hvien` |
| HCROSS-SSCCFG-08 | hvien bit 13 (LCOFI) writability | HS-mode writes hvien bit 13 = 1 then reads back, then writes 0 and reads back | bit 13 is writable and reads back consistently | `norm:ssccfg_lcofi_hvip_hvien` |
| HCROSS-SSCCFG-09 | hvip.LCOFI independent verification | Write all 1s to hvip then read back, check bit 13 | bit 13 reads back as 1 | `norm:ssccfg_lcofi_hvip_hvien` |
| HCROSS-SSCCFG-10 | hvien.LCOFI independent verification | Write all 1s to hvien then read back, check bit 13 | bit 13 reads back as 1 | `norm:ssccfg_lcofi_hvip_hvien` |
| HCROSS-SSCCFG-11 | vsie/vsip LCOFI bits implicitly implemented | Verify the presence of vsie bit 13 and vsip bit 13 (read/write raises no exception) | vsie/vsip bit 13 exist (the implementation of hvip.LCOFI implies the implementation of these bits) | `norm:ssccfg_lcofi_hvip_hvien` |

#### 11.4 vsiselect/vsireg* Multi-Privilege Access Rules (Migrated from Ssccfg Group 6)

**Spec Reference**: `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal`, `norm:ssccfg_hyp_m_s_vsireg_illegal`, `norm:ssccfg_hyp_vs_access_sireg_conditional`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCCFG-12 | VS-mode direct access to vsiselect raises virtual-instruction | VS-mode directly reads/writes vsiselect (0x240) (vsiselect is in 0x40-0x5F) | virtual-instruction exception raised (cause=22) | `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` |
| HCROSS-SSCCFG-13 | VS-mode direct access to vsireg raises virtual-instruction | VS-mode directly reads/writes vsireg (0x245) | virtual-instruction exception raised (cause=22) | `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` |
| HCROSS-SSCCFG-14 | VU-mode direct access to vsiselect raises virtual-instruction | VU-mode directly reads/writes vsiselect | virtual-instruction exception raised (cause=22) | `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` |
| HCROSS-SSCCFG-15 | VU-mode direct access to vsireg raises virtual-instruction | VU-mode directly reads/writes vsireg | virtual-instruction exception raised (cause=22) | `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` |
| HCROSS-SSCCFG-16 | VU-mode access to siselect raises virtual-instruction | VU-mode reads/writes siselect | virtual-instruction exception raised (cause=22) | `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` |
| HCROSS-SSCCFG-17 | VU-mode access to sireg raises virtual-instruction | VU-mode reads/writes sireg | virtual-instruction exception raised (cause=22) | `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` |
| HCROSS-SSCCFG-18 | M-mode access to vsireg illegal with vsiselect 0x40-0x5F | M-mode sets vsiselect=0x40 and accesses vsireg* | illegal-instruction exception raised | `norm:ssccfg_hyp_m_s_vsireg_illegal` |
| HCROSS-SSCCFG-19 | HS-mode access to vsireg illegal with vsiselect 0x40-0x5F | HS-mode sets vsiselect=0x40 and accesses vsireg* | illegal-instruction exception raised | `norm:ssccfg_hyp_m_s_vsireg_illegal` |
| HCROSS-SSCCFG-20 | VS-mode access via sireg* (CDE=0) → illegal-instruction | menvcfg.CDE=0, VS-mode accesses sireg (actually vsireg) via siselect=0x40 | illegal-instruction exception raised | `norm:ssccfg_hyp_vs_access_sireg_conditional` |
| HCROSS-SSCCFG-21 | VS-mode access via sireg* (CDE=1) → virtual-instruction | menvcfg.CDE=1, VS-mode accesses sireg (actually vsireg) via siselect=0x40 | virtual-instruction exception raised | `norm:ssccfg_hyp_vs_access_sireg_conditional` |

#### 11.5 hstateen0 bit 60 Cross Control (Migrated from Ssccfg Group 8)

**Spec Reference**: `norm:hstateen0_csrind_op`

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| HCROSS-SSCCFG-22 | hstateen0 bit 60=0 blocks VS-mode write of siselect | mstateen0 bit 60=1, hstateen0 bit 60=0, VS-mode writes siselect | virtual-instruction exception raised | `norm:hstateen0_csrind_op` |
| HCROSS-SSCCFG-23 | hstateen0 bit 60=0 blocks VS-mode read of sireg | mstateen0 bit 60=1, hstateen0 bit 60=0, VS-mode reads sireg | virtual-instruction exception raised | `norm:hstateen0_csrind_op` |
| HCROSS-SSCCFG-24 | hstateen0 bit 60=1 allows VS-mode access | mstateen0 bit 60=1, hstateen0 bit 60=1, VS-mode accesses siselect/sireg* | access is not blocked by hstateen0 (when CDE=1 a virtual-instruction may still be raised, covered by HCROSS-SSCCFG-21) | `norm:hstateen0_csrind_op` |

> [!NOTE]
> - All cases of this group must detect the H extension at runtime via `HAS_H_EXT()` and probe Smcdeleg/Ssccfg (`menvcfg.CDE` writability + `siselect` presence, depending on Sscsrind); if either is unavailable, the whole group TEST_SKIP.
> - 11.1/11.2 additionally require Sscofpmf to be implemented (`scountovf` present); 11.3 additionally requires Sscofpmf + Smaia/Ssaia (`hvien` present); if missing, the corresponding sub-group TEST_SKIP.
> - The key distinction between HCROSS-SSCCFG-01~02 and Group 10 (HCROSS-SSCOFPMF-01~03) is `menvcfg.CDE`: with CDE=0, VS-mode reads of `scountovf` read zero per mcounteren+hcounteren gating; with CDE=1, they are virtualized and read access directly raises virtual-instruction, with the hypervisor intervening. The CDE state must be explicitly set before implementing the cases.
> - HCROSS-SSCCFG-05~06 supplement the gap of no corresponding case for `norm:ssccfg_illegal_scountinhibit_vs_vu` in `Ssccfg_test_plan.md`: the illegal access (illegal-instruction) of `scountinhibit` with CDE=0 is covered by SSCFG-SINH-12/13 of the original plan, and this group covers the VS/VU virtualization branch with CDE=1.
> - Relationship between HCROSS-SSCCFG-12~17 and Group 6 Sscsrind (HCROSS-SSCSRIND-11~20): the basic behavior of VS/VU direct access to vsiselect/vsireg* raising virtual-instruction belongs to `norm:sscsrind_virtual_inst_fault`; this group verifies the same-source behavior from the Ssccfg perspective when vsiselect is in the delegated counter region (0x40-0x5F); cross-references can be used during implementation to avoid duplication.
> - HCROSS-SSCCFG-18~19 are newly added rules specific to Ssccfg: when `vsiselect` is in 0x40-0x5F, M/S-mode also **must not** directly access `vsireg*` (illegal-instruction), because the state of this region belongs to VS-mode delegated counters and should be managed indirectly by modifying guest state.
> - HCROSS-SSCCFG-22~24 verify the same `hstateen0[60]` control as Group 4.4 (HCROSS-SSSTA-27~29) and Group 6.3 (HCROSS-SSCSRIND-24~27); this group supplements it from the Ssccfg delegated counter perspective; cross-references can be used during implementation.
> - M-mode level `menvcfg.CDE` enabling, `mcounteren` delegation bit settings, and `mvip`/`mvien` LCOFI verification are covered by `Smcdeleg_test_plan.md` and are out of scope for this group.
> - The CSR address of `vsiselect` is 0x240, `vsireg` is 0x245; `scountinhibit` is 0x120; `scountovf` is 0xDA0; `hvip`/`hvien` are 0x645/0x648 respectively.

---

## Test Priorities

| Priority | Test Group | Covered Test IDs | Rationale |
|----------|------------|------------------|-----------|
| P0 (Required) | Group 6.2 (Virtual-inst) | HCROSS-SSCSRIND-11~21 | virtual-instruction exceptions are a core guarantee of virtualization security isolation |
| P0 (Required) | Group 7.2 (vsstatus.SDT) | HCROSS-SSDBLTRP-07~12 | VS-mode double-trap is a key security mechanism in virtualization scenarios |
| P1 (Important) | Group 1 (Sstvala) | HCROSS-SSTVALA-01~08 | precise stval/vstval writes are the foundation for guest OS debugging and exception handling |
| P1 (Important) | Group 6.3 (State-Enable) | HCROSS-SSCSRIND-22~27 | hstateen0[60] access control is key to security isolation |
| P1 (Important) | Group 6.1 (VS CSR) | HCROSS-SSCSRIND-01~10 | vsiselect/vsireg* are foundational for H-extension scenarios |
| P1 (Important) | Group 7.1 (henvcfg.DTE) | HCROSS-SSDBLTRP-01~06 | henvcfg.DTE is the enable control for VS-mode double-trap |
| P1 (Important) | Group 7.3 (SRET vsstatus.SDT) | HCROSS-SSDBLTRP-13~16 | SRET clearing of vsstatus.SDT is a key path for VS-mode trap return |
| P1 (Important) | Group 7.5 (XRET Hyp) | HCROSS-SSDBLTRP-19~24 | cross-mode SDT clearing by MRET/SRET/MNRET under virtualization scenarios |
| P1 (Important) | Group 8.1 (vsctrctl) | HCROSS-SSCTR-01~10 | vsctrctl is the core control register for VS-mode CTR |
| P1 (Important) | Group 8.3 (VS/VU ext traps) | HCROSS-SSCTR-15~20 | VS/VU-mode external trap recording is key in virtualization scenarios |
| P1 (Important) | Group 8.5 (Virt transitions) | HCROSS-SSCTR-25~29 | configuration sources of virtualized mode transitions are the correctness guarantee of CTR under the Hypervisor |
| P2 (Recommended) | Group 3 (Sscounterenw) | HCROSS-SSCOUNTERENW-01~09 | hcounteren control behavior is the guarantee of performance monitoring isolation |
| P2 (Recommended) | Group 5 (Sstc) | HCROSS-SSTC-01~15 | vstimecmp and VS-mode timers are the core facility of Hypervisor virtualization timers |
| P2 (Recommended) | Group 6.4 (Hyp cross) | HCROSS-SSCSRIND-28~33 | transparent remapping and alias behavior depend on the H extension |
| P2 (Recommended) | Group 7.4 (menvcfg.DTE Hyp) | HCROSS-SSDBLTRP-17~18 | global control of Hypervisor CSRs by menvcfg.DTE |
| P2 (Recommended) | Group 8.2 (VS/VU access) | HCROSS-SSCTR-11~14 | VS/VU-mode access restrictions on CTR CSRs |
| P2 (Recommended) | Group 8.4 (VS Freeze) | HCROSS-SSCTR-21~24 | VS-mode Freeze behavior controlled by vsctrctl |
| P2 (Recommended) | Group 8.6 (hstateen VS) | HCROSS-SSCTR-30~32 | hstateen0.CTR control over VS-mode CTR access |
| P2 (Recommended) | Group 10 (Sscofpmf) | HCROSS-SSCOFPMF-01~06 | VS-mode scountovf dual gating and VSINH/VUINH counting inhibition are the guarantee of performance monitoring isolation |
| P2 (Recommended) | Group 11 (Smcdeleg/Ssccfg) | HCROSS-SSCCFG-01~24 | virtualization of scountovf/scountinhibit, LCOFI virtual interrupt bits, and vsireg* access rules are the guarantee of counter delegation isolation |
| P3 (Optional) | Group 2 (Ssccptr) | HCROSS-SSCCPTR-01~04 | PMA-level constraints depend on platform guarantees; cases with limited dynamic PMA configuration capability TEST_SKIP per platform capability |

> Note: The test cases of Ssqosid (Group 9) (SRMCFG-19~24) were not assigned individual priorities in the original merged plan; it is recommended to follow the priorities in `Ssqosid_test_plan.md`. The priority of the hstateen control cases (HCROSS-SSSTA-01~50) in Group 4 (Ssstateen) follows the P1 rating of Group 8 in the original merged plan.

---

## Key Considerations

1. **Extension detection**: All tests must detect the availability of the required extensions (H, Sstvala, Ssccptr, Sscounterenw, Ssstateen, Sstc, Sscsrind, Ssdbltrp, Ssctr, Ssqosid, Sscofpmf, Smcdeleg/Ssccfg, etc.) at runtime; if unavailable, TEST_SKIP.

2. **Sstvala precision requirements**: The Sstvala extension mandates that `stval` be written with the faulting address, not 0. Test assertions must use `TEST_ASSERT_EQ` for precise comparison; fuzzy verification with `TEST_ASSERT(stval != 0)` is not allowed.

3. **Ssccptr platform dependency**: PMAs are platform hardwired attributes; main memory by default satisfies cacheability+coherence. HCROSS-SSCCPTR-04 requires the platform to support dynamic PMA configuration; otherwise TEST_SKIP.

4. **Sscounterenw probing logic**: Before testing, it must be probed which `hpmcounter`s are not read-only zero, and only for those counters is the writability of the corresponding `hcounteren` bits verified.

5. **Distinguishing virtual-instruction from illegal-instruction**: When VS/VU-mode accesses a controlled CSR, if mstateen grants access but hstateen blocks it, virtual-instruction (cause=22) is raised; if mstateen also blocks it, illegal-instruction (cause=2) is raised. Test assertions must use accurate cause constants.

---

## References

- `SPEC/hypervisor.adoc` — RISC-V Hypervisor Extension, Version 1.0
- `SPEC/sstvala.adoc` — Sstvala Extension
- `SPEC/ssccptr.adoc` — Ssccptr Extension
- `SPEC/sscounterenw.adoc` — Sscounterenw Extension
- `SPEC/smstateen.adoc` — Smstateen Extension Specification
- `SPEC/sstc.adoc` — Sstc Extension Specification (Supervisor-mode Timer Interrupts)
- `SPEC/smcsrind.adoc` — Smcsrind/Sscsrind Extension for Indirect CSR Access
- `SPEC/ssdbltrp.adoc` — Ssdbltrp Double Trap Extension
- `SPEC/ssctr.adoc` — Ssctr (Control Transfer Records - Supervisor-level) Extension
- `SPEC/riscv-ssqosid/sqosid.adoc` — Ssqosid (QoS Identifiers) Extension Specification
- `SPEC/sscofpmf.adoc` — Sscofpmf Extension Specification (Count Overflow and Mode-Based Filtering)
- `SPEC/smcdeleg.adoc` — Smcdeleg and Ssccfg Counter Delegation Extensions
- `DOCS/testplan/Hypervisor_CSR_test_plan.md` — Hypervisor CSR subset test plan
- `DOCS/testplan/Hypervisor_Interrupts_test_plan.md` — Hypervisor interrupts subset test plan
- `DOCS/testplan/Hypervisor_Exceptions_test_plan.md` — Hypervisor exceptions and trap subset test plan
- `DOCS/testplan/Hypervisor_2_stage_test_plan.md` — Two-stage translation test plan
- `DOCS/testplan/Hypervisor_gstage_test_plan.md` — G-stage standalone test plan
- `DOCS/testplan/sstvala_test_plan.md` — Sstvala standalone test plan
- `DOCS/testplan/ssccptr_test_plan.md` — Ssccptr standalone test plan
- `DOCS/testplan/sscounterenw_test_plan.md` — Sscounterenw standalone test plan
- `DOCS/testplan/ssstateen_test_plan.md` — Ssstateen standalone test plan
- `DOCS/testplan/sstc_test_plan.md` — Sstc standalone test plan
- `DOCS/testplan/Sscsrind_test_plan.md` — Sscsrind Supervisor Mode test plan
- `DOCS/testplan/Ssdbltrp_test_plan.md` — Ssdbltrp standalone test plan
- `DOCS/testplan/Ssctr_test_plan.md` — Ssctr Supervisor Mode test plan
- `DOCS/testplan/Ssqosid_test_plan.md` — Ssqosid standalone test plan
- `DOCS/testplan/Sscofpmf_test_plan.md` — Sscofpmf standalone test plan
- `DOCS/testplan/Ssccfg_test_plan.md` — Ssccfg standalone test plan
- `ideas/hypervisor_gap.md` — Hypervisor test gap analysis

---

## Appendix A: Specification Point Coverage Matrix

The following table indicates which test cases cover each specification point in the "Covered Specification Points" section and in the Spec Reference of each Group. Entries marked with (self-decomposed) are non-official-label entries.

| Norm ID | Covered Test IDs |
|---------|------------------|
| `norm:H_guest_page_fault` | HCROSS-SSTVALA-01~03 |
| `norm:sstvala_stval_faulting_vaddr` | HCROSS-SSTVALA-01~05 |
| `norm:sstvala_stval_faulting_instruction` | HCROSS-SSTVALA-06~08 |
| `norm:ssccptr_memory_pte_reads` | HCROSS-SSCCPTR-01~04 |
| `norm:sscounterenw_hpmcounter_scounteren` | Group 3 (Sscounterenw, cases to be refined following the pattern of `Shcounterenw_test_plan.md`) |
| `hcounteren_vs_vu_control` (self-decomposed) | Group 3 (Sscounterenw) |
| `norm:hstateen_rv64_csrs` | HCROSS-SSSTA-01~05 |
| `norm:stateen_rv32_upper_bits_csrs` | HCROSS-SSSTA-06 (RV32 platforms; TEST_SKIP on RV64) |
| `norm:hstateen_encoding` | HCROSS-SSSTA-46~50 |
| `norm:hstateen_bit_63_op` | HCROSS-SSSTA-09~15, HCROSS-SSSTA-21~23 |
| `norm:hstateen_bit_63_writable` | HCROSS-SSSTA-07, HCROSS-SSSTA-08, HCROSS-SSSTA-12 |
| `norm:sstateen_vsmode_access_roz` | HCROSS-SSSTA-16~20 |
| `norm:sstateen_ro1_bits` | HCROSS-SSSTA-39~42 (joint verification combined with hstateen RO1 constraints) |
| `norm:hstateen_ro1_bits` | HCROSS-SSSTA-39~42 |
| `norm:stateen_warl_access` | HCROSS-SSSTA-45 |
| `norm:stateen_unimplemented_state_roz` | HCROSS-SSSTA-44 |
| `norm:stateen_reserved_roz` | HCROSS-SSSTA-43 |
| `norm:hstateen0_SE0_op` | HCROSS-SSSTA-21~23 |
| `norm:hstateen0_envcfg_op` | HCROSS-SSSTA-24~26 |
| `norm:hstateen0_csrind_op` | HCROSS-SSSTA-27~29 |
| `norm:hstateen0_imsic_op` | HCROSS-SSSTA-30~32 |
| `norm:hstateen0_aia_op` | HCROSS-SSSTA-33~35 |
| `norm:hstateen0_context_op` | HCROSS-SSSTA-36~38 |
| `norm:hcounteren_acc` | HCROSS-SSTC-05 |
| `norm:henvcfg_stce` | HCROSS-SSTC-01~04, HCROSS-SSTC-12 |
| `norm:vstimecmp_exist` | HCROSS-SSTC-06~08 |
| `norm:sstc_vs_facility` | HCROSS-SSTC-13, HCROSS-SSTC-15 |
| `norm:hip_vstip_vstie_acc_op` | HCROSS-SSTC-09~11, HCROSS-SSTC-14 |
| `norm:vsiselect_min_range` | HCROSS-SSCSRIND-01~03, HCROSS-SSCSRIND-09 |
| `norm:vsiselect_msb_op` | HCROSS-SSCSRIND-04, HCROSS-SSCSRIND-05 |
| `norm:vsireg_access_on_legal_vsiselect` | HCROSS-SSCSRIND-10, HCROSS-SSCSRIND-28~30, HCROSS-SSCSRIND-32 |
| `norm:vsireg_access_behaviour` | HCROSS-SSCSRIND-06, HCROSS-SSCSRIND-07, HCROSS-SSCSRIND-31 |
| `norm:sscsrind_vsmode_csrs_sz` | HCROSS-SSCSRIND-08 |
| `norm:sscsrind_virtual_inst_fault` | HCROSS-SSCSRIND-11~20 |
| `norm:vsmode_virtual_inst_fault` | HCROSS-SSCSRIND-21 |
| `norm:hypervisor_impl_csrs_access_control` | HCROSS-SSCSRIND-24~27 |
| `norm:sscsrind_csrs_access_control` | HCROSS-SSCSRIND-22, HCROSS-SSCSRIND-23 |
| `norm:csrs_alias` | HCROSS-SSCSRIND-33 |
| `norm:henvcfg_DTE` | HCROSS-SSDBLTRP-01 |
| `norm:henvcfg_dte_op` | HCROSS-SSDBLTRP-02~04, HCROSS-SSDBLTRP-06 |
| `norm:menvcfg_dte_op` | HCROSS-SSDBLTRP-05, HCROSS-SSDBLTRP-17, HCROSS-SSDBLTRP-18 |
| `norm:vsstatus_SDT` | HCROSS-SSDBLTRP-07 |
| `norm:vsstatus_sdt_op` | HCROSS-SSDBLTRP-07~12 |
| `norm:sstatus_sdt_trap` | HCROSS-SSDBLTRP-10~12 |
| `norm:sstatus_sdt_sstatus_sie_overwrite` | HCROSS-SSDBLTRP-08, HCROSS-SSDBLTRP-09 |
| `norm:sret_dt` | HCROSS-SSDBLTRP-13~16 |
| `norm:vsstatus_sdt_clr_mret_sret` | HCROSS-SSDBLTRP-20~23 |
| `norm:vsstatus_sdt_clr_mnret` | HCROSS-SSDBLTRP-24 |
| `norm:sstatus_sdt_clr_mret_sret` | HCROSS-SSDBLTRP-19~21, HCROSS-SSDBLTRP-23, HCROSS-SSDBLTRP-24 |
| `norm:HS_mode_invoke_error` | HCROSS-SSDBLTRP-11, HCROSS-SSDBLTRP-12 (double-trap delivery path) |
| `norm:Ssctr_vsctrctl_sz_acc_op` | HCROSS-SSCTR-01~03, HCROSS-SSCTR-09, HCROSS-SSCTR-10, HCROSS-SSCTR-26, HCROSS-SSCTR-27 |
| `norm:vsctr-s_op` | HCROSS-SSCTR-04, HCROSS-SSCTR-25 |
| `norm:vsctrctl-u_op` | HCROSS-SSCTR-05 |
| `norm:vsctrctl-ste_op` | HCROSS-SSCTR-06 |
| `norm:vsctrctl-bpfrz_op` | HCROSS-SSCTR-07 |
| `norm:vsctrctl-lcofifrz_op` | HCROSS-SSCTR-08 |
| `norm:exttrap_vshs` | HCROSS-SSCTR-15, HCROSS-SSCTR-16 |
| `norm:exttrap_vuhs` | HCROSS-SSCTR-19, HCROSS-SSCTR-20 |
| `norm:exttrap_vuvs` | HCROSS-SSCTR-17, HCROSS-SSCTR-18 |
| `norm:ctr_freeze_vs` | HCROSS-SSCTR-21~24, HCROSS-SSCTR-29 |
| `norm:ctr_freeze_bp` | HCROSS-SSCTR-28 |
| `norm:sctrdepth_mode` | HCROSS-SSCTR-11, HCROSS-SSCTR-12 |
| `norm:sctrclr_exceptions` | HCROSS-SSCTR-14 |
| `norm:vsiselect_op` | HCROSS-SSCTR-13 |
| `norm:hstateen_ctr` | HCROSS-SSCTR-30~32 |
| `norm:hstateen_vs` | HCROSS-SSCTR-30~32 |
| `norm:mhpmevent_inh_op` | HCROSS-SSCOFPMF-04, HCROSS-SSCOFPMF-05, HCROSS-SSCOFPMF-06 (VSINH/VUINH portion; M/S/U mode filtering is covered by `Sscofpmf_test_plan.md` Group 2) |
| `norm:scountovf_vsmode_read_access` | HCROSS-SSCOFPMF-01~03 |
| `norm:scountovf_smode_read_access_control` | HCROSS-SSCOFPMF-01~03 (VS-mode side; S/HS-mode side is covered by `Sscofpmf_test_plan.md` Group 4) |
| `norm:ssccfg_virtual_scountovf_vs_vu` | HCROSS-SSCCFG-01~04 |
| `norm:ssccfg_illegal_scountinhibit_vs_vu` | HCROSS-SSCCFG-05, HCROSS-SSCCFG-06 (CDE=1 virtualization branch; the CDE=0 branch is covered by SSCFG-SINH-12/13 of `Ssccfg_test_plan.md`) |
| `norm:ssccfg_lcofi_hvip_hvien` | HCROSS-SSCCFG-07~11 |
| `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` | HCROSS-SSCCFG-12~17 |
| `norm:ssccfg_hyp_m_s_vsireg_illegal` | HCROSS-SSCCFG-18, HCROSS-SSCCFG-19 |
| `norm:ssccfg_hyp_vs_access_sireg_conditional` | HCROSS-SSCCFG-20, HCROSS-SSCCFG-21 |
| `norm:hstateen0_csrind_op` | HCROSS-SSCCFG-22~24 (Ssccfg perspective; see HCROSS-SSSTA-27~29 and HCROSS-SSCSRIND-24~27 for the same-source verification) |
| `ssqosid_virtinst` (self-decomposed) | SRMCFG-19, SRMCFG-20, SRMCFG-21, SRMCFG-24 |
| `ssqosid_smstateen_bit55_0` (self-decomposed) | SRMCFG-23 |
