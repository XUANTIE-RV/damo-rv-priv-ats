**[中文](../testplan/Sha_test_plan.md) | English**

# Sha Extension Test Plan

This document describes the test plan for the Sha (Augmented Hypervisor Extension) extension. Sha is a profile-level composite extension that aggregates all sub-features required by the H extension into a single entity, comprising the following 8 sub-extensions:

1. **H** — Hypervisor Extension
2. **Ssstateen** — Supervisor-mode view state enable extension (sstateen0-3 and hstateen0-3 must be provided)
3. **Shcounterenw** — For any hpmcounter that is not read-only zero, the corresponding bit in hcounteren must be writable
4. **Shvstvala** — vstval must be written in all cases described for stval
5. **Shtvala** — htval must be written with the faulting guest physical address in all circumstances permitted by the ISA
6. **Shvstvecd** — vstvec.MODE must be capable of holding Direct(0); when MODE=Direct, BASE must be capable of holding any valid four-byte-aligned address
7. **Shvsatpa** — All translation modes supported in satp must also be supported in vsatp
8. **Shgatpa** — For each SvNN supported in satp, the corresponding hgatp SvNNx4 mode must be supported; hgatp Bare must also be supported

---

## Test Scope

### Specification Sources

- `SPEC/sha.adoc` — Sha Augmented Hypervisor Extension definition
- `SPEC/hypervisor.adoc` — H extension specification (vsatp/vstvec/vstval/hgatp/hcounteren register definitions)
- `SPEC/smstateen.adoc` — Smstateen/Ssstateen extension specification (sstateen/hstateen CSR definitions and behavior)
- `SPEC/shcounterenw.adoc` — Shcounterenw specification
- `SPEC/shvstvala.adoc` — Shvstvala specification
- `SPEC/shtvala.adoc` — Shtvala specification
- `SPEC/shvstvecd.adoc` — Shvstvecd specification
- `SPEC/shvsatpa.adoc` — Shvsatpa specification
- `SPEC/shgatpa.adoc` — Shgatpa specification
- `SPEC/supervisor.adoc` — Baseline register definitions (satp/stvec/stval, etc.)

### Key Reference Files

| Path | Description |
|------|-------------|
| `SPEC/sha.adoc` | Sha composite extension definition (lists 8 sub-extension dependencies) |
| `SPEC/smstateen.adoc:46-183` | Ssstateen specification: sstateen0-3/hstateen0-3 CSR definitions and hierarchical control |
| `SPEC/hypervisor.adoc:986-1089` | hgatp register specification |
| `SPEC/hypervisor.adoc:1382-1425` | vsatp register specification |
| `SPEC/hypervisor.adoc:1300-1312` | vstvec register specification |
| `SPEC/hypervisor.adoc:1364-1380` | vstval register specification |
| `SPEC/hypervisor.adoc:846-877` | hcounteren register specification |
| `common/encoding.h` | CSR address definitions |
| `common/hyp/hyp_defs.h` | MAKE_HGATP and other macro definitions |
| `common/hyp/hyp_priv.h` | run_in_vs_mode / run_in_vu_mode |
| `common/hyp/hyp_csr.h` | hcounteren_read/write, hyp_delegate_to_vs, etc. |
| `common/hyp/hyp_test.h` | HYP_TEST_END and other test macros |
| `DOCS/testplan/shstvecd_test_plan.md` | Shvstvecd sub-extension detailed test plan |
| `DOCS/testplan/shcounterenw_test_plan.md` | Shcounterenw sub-extension detailed test plan |
| `DOCS/testplan/shvstvala_test_plan.md` | Shvstvala sub-extension detailed test plan |
| `DOCS/testplan/shtvala_test_plan.md` | Shtvala sub-extension detailed test plan |
| `DOCS/testplan/shvsatpa_test_plan.md` | Shvsatpa sub-extension detailed test plan |
| `DOCS/testplan/shgatpa_test_plan.md` | Shgatpa sub-extension detailed test plan |

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:shvstvecd_vstvec_mode_direct` | If the Shvstvecd extension is implemented, then `vstvec.MODE` must be capable of holding the value 0 (Direct). |
| `norm:shvstvecd_vstvec_base_aligned_address` | Furthermore, when `vstvec.MODE`=Direct, `vstvec.BASE` must be capable of holding any valid four-byte-aligned address. |
| `norm:shcounterenw_hpmcounter_hcounteren` | If the Shcounterenw extension is implemented, then for any `hpmcounter` that is not read-only zero, the corresponding bit in `hcounteren` must be writable. |
| `norm:shvstvala_vstval_written` | If the Shvstvala extension is implemented, `vstval` must be written in all cases described for `stval`. |
| `norm:shtvala_htval_faulting_gpa` | If the Shtvala extension is implemented, `htval` must be written with the faulting guest physical address in all circumstances permitted by the ISA. |
| `norm:shvsatpa_satp_vsatp_modes` | If the Shvsatpa extension is implemented, all translation modes supported in `satp` must be supported in `vsatp`. |
| `norm:shgatpa_satp_hgatp_mode_support` | If the Shgatpa extension is implemented, then for each supported virtual memory scheme Sv_NN_ supported in `satp`, the corresponding hgatp Sv_NN_x4 mode must be supported. |
| `norm:shgatpa_hgatp_bare_mode` | Furthermore, the `hgatp` mode Bare must also be supported. |
| `norm:sstateen_user_access_control` | Each bit of a supervisor-level `sstateen` CSR controls user-level access (from U-mode or VU-mode) to an extension's state. |
| `norm:hstateen_encoding` | With the hypervisor extension, the `hstateen` CSRs have identical encodings to the `mstateen` CSRs, except controlling accesses for a virtual machine (from VS and VU modes). |
| `norm:stateen_op` | The `stateen` registers at each level control access to state at all less-privileged levels, but not at its own level. |
| `norm:stateen_illegal_state_access` | Just as with the `counteren` CSRs, when a `stateen` CSR prevents access to state by less-privileged levels, an attempt in one of those privilege modes to execute an instruction that would read or write the protected state raises an illegal-instruction exception, or, if executing in VS or VU mode and the circumstances for a virtual-instruction exception apply, raises a virtual-instruction exception instead of an illegal-instruction exception. |
| `norm:hstateen_bit_63_writable` | Bit 63 of each `hstateen` CSR is always writable (not read-only). |
| `norm:mstateen_zero_initialization` | On reset, all writable `mstateen` bits are initialized by the hardware to zeros. |
| `norm:hstateen_sstateen_zero_initialization` | If machine-level software changes these values, it is responsible for initializing the corresponding writable bits of the `hstateen` and `sstateen` CSRs to zeros too. |

> [!IMPORTANT]
> As a composite extension, each sub-extension of Sha has its own independent detailed test plan (see reference files above). The positioning of this test plan is: (1) Verify that all sub-extensions **coexist** and function correctly (integration-level existence testing); (2) Verify the functionality of the Ssstateen extension in the context of the H extension (this extension currently has no independent test plan); (3) Verify **interaction behavior** between sub-extensions (e.g., the impact on vstvec/vstval/vsatp access when hstateen prohibits access).

### Out of Scope

- **Complete functional testing of each sub-extension**: Covered by their respective independent test plans (shstvecd_test_plan.md, etc.)
- **Basic H extension functionality**: Such as hedeleg/hideleg delegation, V-mode switching, etc., covered by hyp basic tests
- **Complete M-mode stateen (mstateen) testing**: Sha only requires Ssstateen (sstateen + hstateen)
- **Multi-hart scenarios**: Project uses single-core test environment
- **HSXLEN=32 / SXLEN=32 scenarios**: Only RV64 environment is covered
- **Additional features of Smstateen beyond Ssstateen**: Specific mstateen behaviors are out of scope for Sha

---

## Design Key Points

### 1. Layered Testing Strategy for Sha

As a composite extension, Sha testing is divided into two layers:

- **Layer 1: Existence Verification** — Confirm that core features of each sub-extension are **simultaneously available** on the platform (no repetition of complete tests, only "smoke test" level quick confirmation)
- **Layer 2: Ssstateen Functional Verification** — Ssstateen is the only sub-extension in Sha without an independent `Sh*` test plan, requiring detailed verification in this plan
- **Layer 3: Cross-Extension Interaction** — Verify the access control effects of hstateen on other Sh* sub-extension CSRs

### 2. Ssstateen Behavior in H Extension Context

Ssstateen requires both sstateen0-3 and hstateen0-3 to exist. Key behaviors:

- `hstateen` controls VS/VU mode access to state (`norm:hstateen_encoding`)
- When a bit in `hstateen` is 0, the corresponding `sstateen` bit in VS-mode is forced to read-only zero (`norm:sstateen_vsmode_access_roz`)
- Bit 63 is special: `hstateen0` bit 63 controls VS/VU access to `sstateen0` (`norm:hstateen_bit_63_op`)
- `hstateen` bit 63 is always writable (`norm:hstateen_bit_63_writable`)

### 3. CSR Addresses

| CSR | Address | Description |
|-----|---------|-------------|
| sstateen0 | 0x10C | Supervisor state enable 0 |
| sstateen1 | 0x10D | Supervisor state enable 1 |
| sstateen2 | 0x10E | Supervisor state enable 2 |
| sstateen3 | 0x10F | Supervisor state enable 3 |
| hstateen0 | 0x60C | Hypervisor state enable 0 |
| hstateen1 | 0x60D | Hypervisor state enable 1 |
| hstateen2 | 0x60E | Hypervisor state enable 2 |
| hstateen3 | 0x60F | Hypervisor state enable 3 |
| vstvec | 0x205 | Virtual supervisor trap vector |
| vstval | 0x243 | Virtual supervisor trap value |
| vsatp | 0x280 | Virtual supervisor address translation |
| hgatp | 0x680 | Hypervisor guest address translation |
| hcounteren | 0x606 | Hypervisor counter enable |

### 4. Quick Verification Methods for Sub-Extension Existence

| Sub-Extension | Quick Verification Method |
|---------------|---------------------------|
| H | Read/write hstatus (CSR 0x600) without triggering exception |
| Ssstateen | Read/write hstateen0 (CSR 0x60C) and sstateen0 (CSR 0x10C) without triggering exception |
| Shcounterenw | hcounteren CY bit (bit 0) is writable (cycle counter is typically not read-only zero) |
| Shvstvala | vstval is non-zero after triggering page fault in VS-mode |
| Shtvala | htval is non-zero after triggering guest page fault in G-stage |
| Shvstvecd | vstvec.MODE reads back as 0 after writing 0 |
| Shvsatpa | MODE supported by satp reads back consistently after writing to vsatp |
| Shgatpa | hgatp Bare reads back MODE=0 after writing; hgatp SvNNx4 corresponding to satp-supported SvNN can hold value |

---

## Test Groups

> [!IMPORTANT]
> Total of 6 test groups and 28 test cases. Group 1 verifies existence of all sub-extensions (smoke test); Group 2 verifies basic accessibility and WARL behavior of Ssstateen CSRs; Group 3 verifies hstateen access control for VS/VU modes; Group 4 verifies hstateen bit 63 gating behavior on sstateen; Group 5 verifies interaction between hstateen and other Sh* sub-extension CSRs; Group 6 verifies exception priority in two-stage translation. Each group provides: specification basis, test responsibilities, test case table (ID/name/description/expected result); each group provides 1 key C code example.

---

### Group 1: Sub-Extension Existence Verification (Integration Smoke Test)

**Specification Basis**:
- `SPEC/sha.adoc:4-14`: Sha depends on H + Ssstateen + Shcounterenw + Shvstvala + Shtvala + Shvstvecd + Shvsatpa + Shgatpa

**Test Responsibilities**: Quickly verify that core features of each sub-extension are available on the platform, confirming completeness of the Sha composite extension. Make a minimal "capability existence" assertion for each sub-extension.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SHA-EXIST-01 | H Extension Exists | M-mode reads hstatus (CSR 0x600), no exception triggered | Read successful, hstatus accessible |
| SHA-EXIST-02 | Ssstateen Exists (hstateen0) | M-mode reads/writes hstateen0 (CSR 0x60C), no exception triggered | Read/write successful |
| SHA-EXIST-03 | Ssstateen Exists (sstateen0) | HS-mode reads/writes sstateen0 (CSR 0x10C), no exception triggered | Read/write successful |
| SHA-EXIST-04 | Shcounterenw Exists | M-mode writes hcounteren bit 0 (CY)=1, reads back=1 | hcounteren.CY is writable |
| SHA-EXIST-05 | Shvstvecd Exists | M-mode writes vstvec=0 (MODE=Direct), reads back MODE=0 | vstvec.MODE holds Direct |
| SHA-EXIST-06 | Shvsatpa Exists | Probe one MODE supported by satp, write same MODE to vsatp, read back consistently | vsatp.MODE == written value |
| SHA-EXIST-07 | Shgatpa Exists (Bare) | M-mode writes hgatp=0 (Bare), reads back MODE=0 | hgatp Bare can hold value |
| SHA-EXIST-08 | Shgatpa Exists (SvNNx4) | For SvNN supported by satp, write corresponding SvNNx4 to hgatp, read back MODE consistently | hgatp supports corresponding SvNNx4 |

> [!NOTE]
> Existence verification for Shvstvala and Shtvala requires triggering traps, which is covered in Group 5 interaction tests (involving hstateen interaction). Trap environment setup is not duplicated here.

---

### Group 2: Ssstateen CSR Basic Accessibility and WARL Behavior

**Specification Basis**:
- `norm:sstateen_rv64_csrs` (`SPEC/smstateen.adoc:56-59`): sstateen0-3 exist
- `norm:hstateen_rv64_csrs` (`SPEC/smstateen.adoc:61-63`): hstateen0-3 exist
- `norm:stateen_warl_access` (`SPEC/smstateen.adoc:134-136`): stateen bits are WARL
- `norm:stateen_reserved_roz` (`SPEC/smstateen.adoc:139-140`): Reserved bits are read-only zero
- `norm:mstateen_zero_initialization` (`SPEC/smstateen.adoc:153`): On reset, writable mstateen bits are zero
- `norm:hstateen_bit_63_writable` (`SPEC/smstateen.adoc:182-183`): hstateen bit 63 is always writable

**Test Responsibilities**: Verify that all 8 CSRs (sstateen0-3 and hstateen0-3) are accessible; verify hstateen bit 63 is writable; verify reserved bits exhibit read-only zero behavior.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SHA-STATEEN-01 | hstateen0-3 Readable | Sequentially read CSR 0x60C-0x60F, no exceptions triggered | All 4 CSRs accessible |
| SHA-STATEEN-02 | sstateen0-3 Readable | Sequentially read CSR 0x10C-0x10F, no exceptions triggered | All 4 CSRs accessible |
| SHA-STATEEN-03 | hstateen0 bit 63 Writable | Write hstateen0 bit 63=1, read back; write 0, read back | bit 63 toggles with written value |
| SHA-STATEEN-04 | hstateen1/2/3 bit 63 Writable | For hstateen1-3, write bit 63=1 then 0, verify toggle | bit 63 of each CSR is writable |
| SHA-STATEEN-05 | hstateen0 Reserved Bits Read-Only Zero | Write hstateen0 all ones (0xFFFFFFFFFFFFFFFF), read back to check reserved bits | Defined bits may be 1, reserved bits (undefined purpose) read back as 0 |

> [!NOTE]
> `norm:stateen_warl_access` specifies that stateen bits are WARL. Defined bits (such as bit 63, bit 0 C bit, etc.) are writable or read-only zero according to function; undefined/reserved bits are read-only zero. SHA-STATEEN-05 probes which bits are implemented by writing all ones.

---

### Group 3: hstateen Access Control for VS/VU Modes

**Specification Basis**:
- `norm:hstateen_encoding` (`SPEC/smstateen.adoc:129-132`): hstateen controls VS/VU mode access to state
- `norm:stateen_illegal_state_access` (`SPEC/smstateen.adoc:89-95`): Access triggers illegal-instruction or virtual-instruction exception when stateen prohibits access
- `norm:sstateen_vsmode_access_roz` (`SPEC/smstateen.adoc:144-146`): When hstateen bit is 0, VS-mode access to corresponding sstateen bit appears as read-only zero

**Test Responsibilities**: Verify that when hstateen0 bit 63=0, VS-mode access to sstateen0 triggers virtual-instruction exception; when bit 63=1, VS-mode can normally access sstateen0.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SHA-HCTL-01 | hstateen0 bit63=0 → VS access to sstateen0 triggers virtual-inst | Set hstateen0 bit63=0; VS-mode `csrr sstateen0` | Triggers virtual-instruction exception (cause=22) |
| SHA-HCTL-02 | hstateen0 bit63=1 → VS can access sstateen0 | Set hstateen0 bit63=1; VS-mode `csrr sstateen0` | Normal read, no exception |
| SHA-HCTL-03 | hstateen0 bit63=0 → VS write to sstateen0 triggers virtual-inst | Set hstateen0 bit63=0; VS-mode `csrw sstateen0, val` | Triggers virtual-instruction exception (cause=22) |
| SHA-HCTL-04 | hstateen0 bit63=0 makes sstateen0 read-only zero from VS perspective | Set hstateen0 bit63=0; HS-mode checks VS-mode view of sstateen0 | From VS-mode perspective, sstateen0 appears as read-only zero |
| SHA-HCTL-05 | hstateen0 bit59 gates AIA CSR access | If AIA (Ssaia) is implemented: set hstateen0 bit59=0; VS-mode accesses siselect/sireg | Triggers virtual-instruction exception (cause=22); if AIA not implemented, `TEST_SKIP` |

> [!NOTE]
> `norm:stateen_illegal_state_access` states: In VS/VU mode, if hstateen prohibits access to certain state and conditions for virtual-instruction exception are met (V=1 and hstateen bit is 0 while mstateen bit is 1), then virtual-instruction exception (cause=22) is triggered instead of illegal-instruction.

---

### Group 4: hstateen bit 63 Hierarchical Gating on sstateen

**Specification Basis**:
- `norm:mstateen_bit_63_op` (`SPEC/smstateen.adoc:162-164`): mstateen0 bit 63 controls access to sstateen0 and hstateen0
- `norm:hstateen_bit_63_op` (`SPEC/smstateen.adoc:165-166`): hstateen0 bit 63 controls access to sstateen0 as seen by VS-mode
- `norm:mstateen_lower_priv_roz` (`SPEC/smstateen.adoc:141-143`): When mstateen bit is 0, corresponding hstateen and sstateen bits appear as read-only zero

**Test Responsibilities**: Verify the hierarchical gating chain: mstateen0 → hstateen0 → sstateen0(VS).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SHA-HIER-01 | mstateen0 bit63=1 → hstateen0 bit63 writable | Set mstateen0 bit63=1; write hstateen0 bit63=1, read back | hstateen0 bit63 == 1 |
| SHA-HIER-02 | mstateen0 bit63=0 → hstateen0 bit63 read-only zero | Set mstateen0 bit63=0; read hstateen0 bit63 | hstateen0 bit63 == 0 (read-only zero) |
| SHA-HIER-03 | mstateen0 bit63=0 → HS access to hstateen0 causes exception | Set mstateen0 bit63=0; HS-mode accesses hstateen0 | Triggers illegal-instruction (cause=2) |

> [!NOTE]
> SHA-HIER-03 verifies mstateen gating on HS-mode. When mstateen0 bit63=0, HS-mode attempting to access hstateen0 will trigger illegal-instruction exception. This is a hierarchical security feature of Smstateen/Ssstateen.

---

### Group 5: Ssstateen Interaction with Other Sh* Sub-Extension CSRs

**Specification Basis**:
- `norm:stateen_op` (`SPEC/smstateen.adoc:86-88`): stateen controls lower-privilege access to state
- `norm:stateen_illegal_state_access` (`SPEC/smstateen.adoc:89-95`): Violation triggers exception
- Various sub-extension norms (see respective sub-extension specifications)

**Test Responsibilities**: Verify that when hstateen-related bits prohibit access to specific state, VS-mode access to relevant CSRs (if controlled by stateen) behaves correctly; also verify existence of Shvstvala and Shtvala (by triggering traps to verify vstval/htval are correctly written).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SHA-CROSS-01 | Shvstvala Existence: VS page fault writes vstval | Establish G-stage identity mapping; trigger load page fault in VS-mode (delegated to VS); check vstval | vstval contains faulting virtual address (non-zero) |
| SHA-CROSS-02 | Shtvala Existence: G-stage fault writes htval | Access unmapped GPA in VS-mode to trigger guest page fault | htval contains faulting GPA >> 2 (non-zero) |
| SHA-CROSS-03 | Shvsatpa + Shgatpa Coordination: vsatp and hgatp simultaneously support corresponding modes | Probe satp supports Sv39; verify vsatp supports Sv39 and hgatp supports Sv39x4 | Both hold corresponding MODE simultaneously |
| SHA-CROSS-04 | Shvstvecd + Shvstvala Coordination: Direct trap writes vstval | VS-mode sets vstvec=Direct custom entry; trigger page fault delegated to VS; verify trap jumps to BASE and vstval is written | trap PC == BASE and vstval contains faulting address |
| SHA-CROSS-05 | Shtvala + Shvstvala Non-Interference | Consecutively trigger G-stage fault (verify htval) and VS page fault (verify vstval), confirm no cross-contamination | First trap: htval==GPA>>2 and vstval unchanged; Second trap: vstval==fault addr and htval unchanged |

> [!NOTE]
> SHA-CROSS-01/02 simultaneously verify existence of Shvstvala/Shtvala (these two verifications requiring trap environment were skipped in Group 1). SHA-CROSS-03 verifies that mode mapping consistency of Shvsatpa and Shgatpa holds simultaneously on the same platform. SHA-CROSS-04 verifies end-to-end coordination between Shvstvecd (Direct trap jump) and Shvstvala (vstval write).

---

### Group 6: Exception Priority Verification

**Specification Basis**:
- `norm:HSyncExcPrio` (`SPEC/hypervisor.adoc:2201-2230`): Synchronous exception priority table, G-stage fault takes priority over VS-stage fault
- `SPEC/hypervisor.adoc:2043-2051`: In two-stage translation, G-stage check completes before VS-stage

**Test Responsibilities**: Verify that exception priority relationships defined in Hypervisor extension specification are correctly observed in two-stage translation scenarios.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| SHA-PRIO-01 | G-stage fault takes priority over VS-stage fault | Construct scenario: On VS-stage PTE walk path, VS root table PTE itself is VS-invalid (would cause VS page fault) but the GPA where this PTE resides is also unmapped in G-stage (would cause G-stage fault); VS-mode accesses target address | Actually triggers G-stage guest-page-fault (cause=20/21/23) rather than VS page fault (cause=12/13/15), htval==PTE GPA>>2 |
| SHA-PRIO-02 | Instruction fetch fault takes priority over illegal-instruction | Construct scenario: Certain GPA lacks X permission in G-stage; place illegal instruction encoding at this address; VS-mode jumps to execute | Actually triggers inst guest-page-fault (cause=20) rather than illegal-instruction (cause=2), htval==GPA>>2 |

---

## Appendix: Related Constants and API Reference

### CSR Addresses

| Name | Value | Description |
|------|-------|-------------|
| `CSR_HSTATUS` | `0x600` | Hypervisor status |
| `CSR_HSTATEEN0` | `0x60C` | Hypervisor state enable 0 |
| `CSR_HSTATEEN1` | `0x60D` | Hypervisor state enable 1 |
| `CSR_HSTATEEN2` | `0x60E` | Hypervisor state enable 2 |
| `CSR_HSTATEEN3` | `0x60F` | Hypervisor state enable 3 |
| `CSR_SSTATEEN0` | `0x10C` | Supervisor state enable 0 |
| `CSR_SSTATEEN1` | `0x10D` | Supervisor state enable 1 |
| `CSR_SSTATEEN2` | `0x10E` | Supervisor state enable 2 |
| `CSR_SSTATEEN3` | `0x10F` | Supervisor state enable 3 |
| `CSR_MSTATEEN0` | `0x30C` | Machine state enable 0 |
| `CSR_VSTVEC` | `0x205` | Virtual supervisor trap vector |
| `CSR_VSTVAL` | `0x243` | Virtual supervisor trap value |
| `CSR_VSATP` | `0x280` | Virtual supervisor address translation |
| `CSR_HGATP` | `0x680` | Hypervisor guest address translation |
| `CSR_HCOUNTEREN` | `0x606` | Hypervisor counter enable |
| `CSR_SATP` | `0x180` | Supervisor address translation |

### Exception Codes

| Constant | Value | Description |
|----------|-------|-------------|
| `CAUSE_ILLEGAL_INSTRUCTION` | 2 | Illegal instruction |
| `CAUSE_VIRTUAL_INSTRUCTION` | 22 | Virtual instruction (privilege violation under VS/VU) |
| `CAUSE_LOAD_PAGE_FAULT` | 13 | Load page fault |
| `CAUSE_GUEST_LOAD_PAGE_FAULT` | 21 | Guest load page fault (G-stage fault) |

### Sha Sub-Extension Test Plan Index

| Sub-Extension | Independent Test Plan | Case Count |
|---------------|----------------------|------------|
| H | `hyp_2_stage_translation_test_plan.md`, etc. | — |
| Ssstateen | **This document Group 2-4** (no independent plan) | 12 |
| Shcounterenw | `shcounterenw_test_plan.md` | 21 |
| Shvstvala | `shvstvala_test_plan.md` | 20 |
| Shtvala | `shtvala_test_plan.md` | — |
| Shvstvecd | `shstvecd_test_plan.md` | 14 |
| Shvsatpa | `shvsatpa_test_plan.md` | 18 |
| Shgatpa | `shgatpa_test_plan.md` | 19 |

### Test Framework API

- `run_in_vs_mode(fn, arg)`: Execute fn(arg) in VS-mode (V=1), return to M-mode via ecall
- `run_in_vu_mode(fn, arg)`: Execute fn(arg) in VU-mode (V=1, U)
- `hyp_delegate_to_vs(hedeleg_mask, hideleg_mask)`: Configure exception/interrupt delegation to VS-mode
- `hyp_reset_state()`: Reset all hypervisor CSRs
- `HYP_TEST_END()`: Test end macro, includes hyp_reset_state + result recording
- `MAKE_HGATP(mode, vmid, ppn)`: Construct hgatp value
- `HGATP_GET_MODE(v)` / `HGATP_GET_PPN(v)`: Extract hgatp fields
- `TEST_REGISTER` / `TEST_BEGIN` / `TEST_ASSERT` / `TEST_SKIP` / `TEST_END`: Test case registration and assertion macros

---

## Test Execution Notes

### Subdirectory and Build Integration (Implementation Phase Reference, Not Produced by This Plan)

> [!IMPORTANT]
> This planning phase **only produces `DOCS/testplan/sha_test_plan.md`**, does not create `sha/` subdirectory, does not modify top-level `Makefile`, and does not write C test code. Subsequent implementation phase should create per this document:
> - `sha/main.c` (reference existing extensions like `svadu/main.c`)
> - `sha/Makefile` (`SPIKE_ISA_EXT = _sha`, must enable all sub-extensions simultaneously)
> - `sha/kernel.ld`
> - `sha/tests/test_exist.c`, `test_stateen.c`, `test_hctl.c`, `test_hierarchy.c`, `test_cross.c`
> And add `sha` to the `EXTENSIONS` list in top-level `Makefile`.

### Runtime Environment

- Group 1: M-mode directly operates various CSRs for quick existence verification
- Group 2: M-mode directly operates hstateen/sstateen CSRs
- Group 3: M-mode configures hstateen then enters VS-mode via `run_in_vs_mode` to verify gating behavior
- Group 4: M-mode verifies mstateen → hstateen hierarchical relationship
- Group 5: Comprehensive environment (M-mode + VS-mode + G-stage mapping), verifies cross-sub-extension interaction
- Must enable all sub-extensions via `SPIKE_ISA_EXT = _sha` or equivalent
- Single-core environment, no IPI required
- Group 5 trap-related tests require G-stage identity mapping

### Relationship with Independent Sub-Extension Tests

> [!IMPORTANT]
> This test plan **does not replace** independent test plans for each sub-extension. Goals of Sha testing are:
> 1. **Integration Verification**: Confirm all 8 sub-extensions are simultaneously available on the same platform
> 2. **Ssstateen Functional Coverage**: Serve as the only detailed test for Ssstateen in H extension context
> 3. **Interaction Verification**: Cover scenarios where sub-extensions work together
>
> Complete sub-extension functional testing should run their respective independent test plans.

### Failure Diagnosis Guide

| Symptom | Possible Cause |
|---------|----------------|
| SHA-EXIST-01 fails | H extension not implemented or not enabled |
| SHA-EXIST-02/03 fails | Ssstateen not implemented (sstateen/hstateen CSRs do not exist) |
| SHA-EXIST-04 fails | Shcounterenw not satisfied: hcounteren.CY not writable |
| SHA-EXIST-05 fails | Shvstvecd not satisfied: vstvec.MODE cannot hold Direct |
| SHA-EXIST-06 fails | Shvsatpa not satisfied: vsatp does not support modes supported by satp |
| SHA-EXIST-07/08 fails | Shgatpa not satisfied: hgatp does not support Bare or corresponding SvNNx4 |
| SHA-STATEEN-03/04 fails | hstateen bit 63 not writable, violates `norm:hstateen_bit_63_writable` |
| SHA-HCTL-01 fails (no trap triggered) | hstateen0 bit63=0 did not correctly gate VS-mode access to sstateen0 |
| SHA-HCTL-01 cause != 22 | Triggered illegal-instruction instead of virtual-instruction (mstateen configuration issue) |
| SHA-HCTL-02 fails (trap triggered) | VS-mode still cannot access sstateen0 when hstateen0 bit63=1 |
| SHA-HIER-02/03 fails | mstateen → hstateen hierarchical gating chain abnormal |
| SHA-CROSS-01 fails (vstval=0) | Shvstvala not satisfied, vstval not written during page fault |
| SHA-CROSS-02 fails (htval=0) | Shtvala not satisfied, htval not written during guest page fault |
| SHA-CROSS-03 fails | Shvsatpa or Shgatpa mode mapping inconsistent on same platform |
| SHA-CROSS-04 fails | Shvstvecd Direct jump or Shvstvala vstval write coordination abnormal |
