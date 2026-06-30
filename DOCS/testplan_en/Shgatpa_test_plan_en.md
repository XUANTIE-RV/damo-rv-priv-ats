**[中文](../testplan/Shgatpa_test_plan.md) | English**

# Shgatpa Extension Test Plan

This document describes the test plan for the Shgatpa (Translation Mode Support for hgatp, Version 1.0) extension. Shgatpa requires: (1) For each virtual memory scheme SvNN supported in `satp`, the corresponding SvNNx4 mode must also be supported in `hgatp`; (2) The Bare mode of `hgatp` must be supported.

---

## Test Scope

### Specification Sources

- `SPEC/shgatpa.adoc` — Shgatpa Extension for Translation Mode Support, Version 1.0
- `SPEC/hypervisor.adoc` lines 986–1089 (`hgatp` register definition) — Provides `hgatp` field layout, MODE encoding table, WARL write semantics
- `SPEC/supervisor.adoc` lines 997–1065 (`satp` register and MODE field encoding) — Provides satp MODE encoding table, which is the source of Shgatpa mapping relationships

### Key Reference Files

| Path | Description |
|------|-------------|
| `SPEC/shgatpa.adoc` | Full Shgatpa specification (11 lines total, 2 norms) |
| `SPEC/hypervisor.adoc:986-1089` | `hgatp` register specification: HSXLEN-bit RW, MODE encoding (Bare/Sv39x4/Sv48x4/Sv57x4), WARL behavior |
| `SPEC/supervisor.adoc:997-1065` | `satp` register specification: MODE encoding table (Bare=0, Sv39=8, Sv48=9, Sv57=10), WARL behavior |
| `common/encoding.h:284` | `CSR_HGATP = 0x680` |
| `common/encoding.h:145` | `CSR_SATP = 0x180` |
| `common/encoding.h:332-340` | `HGATP64_MODE_SHIFT=60`, `HGATP_MODE_BARE=0`, `HGATP_MODE_SV39X4=8`, `HGATP_MODE_SV48X4=9`, `HGATP_MODE_SV57X4=10` |
| `common/hyp/hyp_defs.h:28-40` | `MAKE_HGATP(mode, vmid, ppn)` macro, `HGATP_GET_MODE()` and other field extraction macros |
| `common/hyp/hyp_priv.h:21` | `run_in_vs_mode(fn, arg)` — Execute test function in VS-mode (V=1) |
| `common/hyp/hyp_test.h` | Hypervisor test macros (`HYP_TEST_END`, etc.) |
| `common/hyp/hyp_reset.c` | `hyp_reset_state()` resets hypervisor CSR state |
| `DOCS/framework/hypervisor_framework.md` | Hypervisor test framework overview |
| `DOCS/testplan/shvsatpa_test_plan.md` | Sibling vsatp mode support test plan, structural reference baseline |

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:shgatpa_satp_hgatp_mode_support` | If the Shgatpa extension is implemented, then for each supported virtual memory scheme Sv_NN_ supported in `satp`, the corresponding hgatp Sv_NN_x4 mode must be supported. |
| `norm:shgatpa_hgatp_bare_mode` | Furthermore, the `hgatp` mode Bare must also be supported. |
| `norm:hgatp_sz_acc_op` | The `hgatp` register is an HSXLEN-bit read/write register which controls G-stage address translation and protection, the second stage of two-stage translation for guest virtual addresses. |
| `norm:hgatp_mode_bare` | When MODE=Bare, guest physical addresses are equal to supervisor physical addresses, and there is no further memory protection for a guest virtual machine beyond the physical memory protection scheme. In this case, software must write zero to the remaining fields in `hgatp`. |
| `norm:hgatp_mode_sv` | When HSXLEN=32, the only other valid setting for MODE is Sv32x4. When HSXLEN=64, modes Sv39x4, Sv48x4, and Sv57x4 are defined. |
| `norm:hgatp_mode_warl` | A write to `hgatp` with an unsupported MODE value is not ignored as it is for `satp`. Instead, the fields of `hgatp` are WARL in the normal way, when so indicated. |
| `norm:hgatp_ppn_op` | For the paged virtual-memory schemes, the root page table is 16 KiB and must be aligned to a 16-KiB boundary. In these modes, the lowest two bits of the physical page number (PPN) in `hgatp` always read as zeros. |
| `norm:satp_mode_sxlen64` | When SXLEN=64, three paged virtual-memory schemes are defined: Sv39, Sv48, and Sv57. One additional scheme, Sv64, will be defined in a later version. The remaining MODE settings are reserved for future use. |
| `norm:satp_mode_op_unsupported` | If a write to `satp` specifies a MODE value that is not supported, the entire write has no effect; no fields in `satp` are modified. |

> [!IMPORTANT]
> Shgatpa has two core constraints: (1) If satp supports SvNN → hgatp must support SvNNx4; (2) hgatp.MODE=Bare must be available. Note that the WARL behavior of hgatp and satp is **different**: when writing an unsupported MODE to satp, the entire write is ignored, whereas when writing an unsupported MODE to hgatp, fields are processed according to normal WARL rules (`norm:hgatp_mode_warl`).

### Mode Mapping

| satp MODE | Value | Corresponding hgatp MODE | Value | Relationship |
|-----------|-------|--------------------------|-------|--------------|
| Bare | 0 | Bare | 0 | hgatp Bare independent requirement (`norm:shgatpa_hgatp_bare_mode`) |
| Sv39 | 8 | Sv39x4 | 8 | satp supports Sv39 → hgatp must support Sv39x4 |
| Sv48 | 9 | Sv48x4 | 9 | satp supports Sv48 → hgatp must support Sv48x4 |
| Sv57 | 10 | Sv57x4 | 10 | satp supports Sv57 → hgatp must support Sv57x4 |

> [!NOTE]
> When HSXLEN=64, the satp MODE encoding values are **identical** to the corresponding hgatp MODE encoding values (Bare=0, Sv39/Sv39x4=8, Sv48/Sv48x4=9, Sv57/Sv57x4=10), but they represent different translation schemes (hgatp's SvNNx4 provides 2 additional bits of GPA support, with a 16KB root page table).

### Out of Scope

- **G-stage address translation table lookup correctness**: This plan only verifies whether MODE fields can hold values, not translation table lookup behavior
- **vsatp MODE consistency**: Covered by Shvsatpa extension (`shvsatpa_test_plan.md`)
- **Complete PPN field address range verification**: Only basic write-readback verification, no exhaustive testing
- **VMID field width testing**: Not within the core scope of this plan
- **Illegal-instruction behavior under mstatus.TVM=1**: `norm:hgatp_tvm_illegal` belongs to TVM functional testing
- **Multi-hart scenarios**: Project uses single-core test environment
- **HSXLEN=32 scenarios**: Only covers HSXLEN=64 (RV64 environment)
- **G-stage activation behavior under HLV/HLVX/HSV/MPRV scenarios**: Not within the scope of this plan

---

## Design Key Points

### 1. MODE Detection Method

**satp detection** (same as shvsatpa):
- satp MODE is WARL: writing an unsupported value causes the **entire write to be ignored** (`norm:satp_mode_op_unsupported`)
- Strategy: First write Bare (0) to establish baseline, then write target MODE, read back to determine support

**hgatp detection** (different from satp!):
- hgatp MODE is WARL but behaves differently: writing an unsupported MODE value is **not** ignored; instead, fields are processed according to normal WARL rules (`norm:hgatp_mode_warl`)
- Strategy: Write target MODE, read back MODE field. If readback == written value → supported; if readback != written value (forced to another legal value by WARL) → not supported

> [!IMPORTANT]
> This is the key difference between hgatp and satp/vsatp WARL behavior. When writing an unsupported MODE to satp, the entire write is ignored (including ASID/PPN fields), whereas hgatp fields are independently processed according to WARL rules (MODE is forced to a legal value, but PPN/VMID may still be written).

### 2. hgatp Access Path

`hgatp` (CSR 0x680) can only be directly accessed in M-mode or HS-mode (V=0). VS-mode (V=1) cannot directly access hgatp (triggers virtual-instruction exception).

| Privilege Mode | Access Method | Description |
|----------------|---------------|-------------|
| M-mode | `csrr/csrw 0x680` | Direct read/write hgatp |
| HS-mode (V=0) | `csrr/csrw hgatp` | Direct read/write (unless mstatus.TVM=1) |
| VS-mode (V=1) | Not accessible | Accessing hgatp triggers virtual-instruction exception |

### 3. MODE Encoding (HSXLEN=64)

| Value | Name | Description |
|-------|------|-------------|
| 0 | Bare | No translation protection (GPA = SPA) |
| 1-7 | — | Reserved |
| 8 | Sv39x4 | 41-bit GPA (2-bit extension of Sv39) |
| 9 | Sv48x4 | 50-bit GPA (2-bit extension of Sv48) |
| 10 | Sv57x4 | 59-bit GPA (2-bit extension of Sv57) |
| 11-15 | — | Reserved |

### 4. PPN Field Lower 2 Bits Constraint

In Sv*x4 modes, hgatp.PPN[1:0] always reads as zero (`norm:hgatp_ppn_op`) because the root page table must be 16KB aligned. Test considerations:
- When writing non-zero values to PPN lower 2 bits, readback should be zero
- This constraint may not apply when MODE=Bare

### 5. Isolation from hyp_reset_state

`common/hyp/hyp_reset.c::hyp_reset_state()` resets hypervisor CSR state. Each test case in this plan backs up `hgatp` before operations and restores it after completion.

### 6. Detection Result Propagation

After Group 1 detects the set of MODEs supported by `satp`, results are stored in global variables (e.g., `g_satp_supports_sv39`) for Group 2 to determine which hgatp modes should be verified.

---

## Test Groups

> [!IMPORTANT]
> Total of 6 test groups, 21 test cases. Group 1 detects satp supported mode set in M/HS-mode; Group 2 verifies hgatp corresponding SvNNx4 mode support consistency (core verification group); Group 3 verifies mandatory hgatp Bare mode support; Group 4 verifies hgatp MODE WARL behavior; Group 5 verifies hgatp PPN field lower 2 bits behavior; Group 6 verifies VMID field width. Each group provides: specification basis, test responsibilities, test case table (ID/name/description/expected result); each group provides 1 key C code example.

---

### Group 1: `satp` MODE Support Detection (Baseline Establishment)

**Specification Basis**:
- `norm:satp_mode_sxlen64` (`SPEC/supervisor.adoc:1044-1050`): Defines Sv39/Sv48/Sv57 when SXLEN=64
- `norm:satp_mode_op_unsupported` (`SPEC/supervisor.adoc:1052-1055`): Writing unsupported MODE causes entire write to be invalid

**Test Responsibilities**: Detect the set of translation modes supported by `satp` in M/HS-mode to establish comparison baseline for Group 2. Perform write-readback tests for each standard MODE value (Bare=0, Sv39=8, Sv48=9, Sv57=10).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HGATP-PROBE-01 | satp MODE=Bare detection | Write `satp = 0` (MODE=0, ASID=0, PPN=0), read back MODE field | Readback MODE=0 (Bare is typically supported) |
| HGATP-PROBE-02 | satp MODE=Sv39 detection | First write Bare baseline, then write `satp = 8 << 60` (MODE=8), read back MODE | Readback MODE ∈ {0, 8}; record support status |
| HGATP-PROBE-03 | satp MODE=Sv48 detection | First write Bare baseline, then write `satp = 9 << 60` (MODE=9), read back MODE | Readback MODE ∈ {previous valid value, 9}; record support status |
| HGATP-PROBE-04 | satp MODE=Sv57 detection | First write Bare baseline, then write `satp = 10 << 60` (MODE=10), read back MODE | Readback MODE ∈ {previous valid value, 10}; record support status |

> [!NOTE]
> Detection logic is identical to Group 1 in shvsatpa test plan. At least one non-Bare mode should be supported (otherwise H extension can hardly run guest OS).

---

### Group 2: `hgatp` SvNNx4 MODE Consistency Verification Against `satp` Supported Modes

**Specification Basis**:
- `norm:shgatpa_satp_hgatp_mode_support` (`SPEC/shgatpa.adoc:4-7`): satp supports SvNN → hgatp must support SvNNx4
- `norm:hgatp_sz_acc_op` (`SPEC/hypervisor.adoc:989-994`): `hgatp` is HSXLEN-bit RW register
- `norm:hgatp_mode_sv` (`SPEC/hypervisor.adoc:1020-1027`): Defines Sv39x4/Sv48x4/Sv57x4 when HSXLEN=64

**Test Responsibilities**: For each SvNN MODE detected as supported by satp in Group 1, directly write the corresponding SvNNx4 value to `hgatp` (CSR 0x680) in M/HS-mode and verify readback MODE matches, confirming Shgatpa mapping relationship holds.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HGATP-MODE-01 | hgatp MODE=Sv39x4 support | Execute only when `g_satp_supports_sv39 == true`; write `hgatp = MAKE_HGATP(8, 0, 0)`, read back MODE | `MODE == 8` (Shgatpa requires Sv39→Sv39x4 consistency) |
| HGATP-MODE-02 | hgatp MODE=Sv48x4 support | Execute only when `g_satp_supports_sv48 == true`; write `hgatp = MAKE_HGATP(9, 0, 0)`, read back MODE | `MODE == 9` (Shgatpa requires Sv48→Sv48x4 consistency) |
| HGATP-MODE-03 | hgatp MODE=Sv57x4 support | Execute only when `g_satp_supports_sv57 == true`; write `hgatp = MAKE_HGATP(10, 0, 0)`, read back MODE | `MODE == 10` (Shgatpa requires Sv57→Sv57x4 consistency) |
| HGATP-MODE-04 | hgatp mode cross-switching | Sequentially write all hgatp modes corresponding to satp supported modes (e.g., Bare→Sv39x4→Sv48x4→Bare), verify readback each time | Each readback MODE == written value |

> [!IMPORTANT]
> This is the core verification group for Shgatpa extension. If satp supports Sv39 but hgatp does not support Sv39x4, the Shgatpa constraint is violated. Assertion condition: **For each SvNN supported by satp, after writing the corresponding SvNNx4 to hgatp, the readback MODE must equal the written value**.

---

### Group 3: `hgatp` Bare Mode Mandatory Support Verification

**Specification Basis**:
- `norm:shgatpa_hgatp_bare_mode` (`SPEC/shgatpa.adoc:9-10`): `hgatp` Bare mode must be supported
- `norm:hgatp_mode_bare` (`SPEC/hypervisor.adoc:1010-1015`): No translation protection when MODE=Bare; remaining fields must be written as zero when selecting Bare

**Test Responsibilities**: Verify that `hgatp` Bare mode (MODE=0) can always be written and read back correctly, and that remaining fields (VMID/PPN) are zero after selecting Bare.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HGATP-BARE-01 | hgatp MODE=Bare write-readback | Write `hgatp = 0` (MODE=0, VMID=0, PPN=0), read back | `hgatp == 0` (all zeros) |
| HGATP-BARE-02 | hgatp switch from SvNNx4 back to Bare | First write supported SvNNx4 mode, then write Bare (all zeros), read back | `MODE == 0`, entire register is 0 |
| HGATP-BARE-03 | hgatp Bare mode PPN/VMID zeroed | Write `hgatp = 0` (Bare), read back to verify PPN=0, VMID=0 | `HGATP_GET_PPN(readback) == 0` and `HGATP_GET_VMID(readback) == 0` |

> [!NOTE]
> `norm:hgatp_mode_bare` requires that when selecting Bare, "software must write zero to the remaining fields". If software writes Bare but PPN/VMID are non-zero, behavior is UNSPECIFIED. Therefore HGATP-BARE-01/03 ensure correct state after compliant writes.

---

### Group 4: `hgatp` MODE WARL Behavior Verification

**Specification Basis**:
- `norm:hgatp_mode_warl` (`SPEC/hypervisor.adoc:1073-1076`): Writing unsupported MODE value to hgatp is **not** ignored (unlike satp), fields are processed according to normal WARL rules

**Test Responsibilities**: Verify hgatp WARL behavior when writing unsupported MODE values—MODE is forced to a legal value, but the write is not completely ignored (other fields may be updated).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HGATP-WARL-01 | Write reserved MODE=7 | First set hgatp=Bare; write `hgatp = MAKE_HGATP(7, 0, 0x2000)`, read back MODE | MODE != 7 (MODE forced to legal value by WARL) |
| HGATP-WARL-02 | Write reserved MODE=15 | First set hgatp=Bare; write `hgatp = MAKE_HGATP(15, 0, 0x3000)`, read back MODE | MODE != 15 (legal value should be one of implemented 0/8/9/10) |
| HGATP-WARL-03 | Write reserved MODE=1 | First set hgatp=Bare; write `hgatp = MAKE_HGATP(1, 0, 0x4000)`, read back MODE | MODE != 1 (1 is reserved value, should not be held) |
| HGATP-WARL-04 | WARL behavior comparison: hgatp does not fully ignore write | First set hgatp=Bare(PPN=0); write unsupported MODE + PPN=0x5000; read back PPN | Readback PPN may be 0x5000 (WARL processes independently, unlike satp where entire write is ignored) or 0 (implementation-defined) |
| HGATP-WARL-05 | MODE readback always legal | Sequentially write all values from MODE=1,2,...,15, read back MODE each time | Each readback MODE ∈ {0, 8, 9, 10} (only legal/implemented values) |

> [!IMPORTANT]
> **Specificity of HGATP-WARL-04**: This test case verifies the WARL behavior difference between hgatp and satp. `norm:hgatp_mode_warl` explicitly states that writing unsupported MODE to hgatp is "not ignored", but fields are processed according to normal WARL rules. This means even if MODE is forced to a legal value, PPN/VMID fields may still be written. However, specific behavior is implementation-defined (WARL allows two choices), so this test case adopts "observe and record" rather than hard assertion that PPN must be written.

---

### Group 5: `hgatp` PPN Field Lower 2 Bits Behavior Verification

**Specification Basis**:
- `norm:hgatp_ppn_op` (`SPEC/hypervisor.adoc:1078-1085`): In Sv*x4 modes, PPN lower 2 bits always read as zero (root page table 16KB alignment constraint)

**Test Responsibilities**: Verify that when writing non-zero values to hgatp.PPN lower 2 bits in Sv*x4 modes, readback of these bits is always zero; record behavior in Bare mode (specification does not mandate constraint).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HGATP-PPN-01 | Sv39x4 mode PPN[1:0]=0b11 write | Write `hgatp = MAKE_HGATP(8, 0, 0x2003)` (PPN lower 2 bits = 0b11), read back PPN | `PPN & 0x3 == 0` (lower 2 bits forced to zero) |
| HGATP-PPN-02 | Sv39x4 mode PPN high bits preserved | Write `hgatp = MAKE_HGATP(8, 0, 0x12340)` (PPN lower 2 bits = 0), read back PPN | `PPN == 0x12340` (high bits preserved, lower 2 bits zero) |
| HGATP-PPN-03 | Sv48x4 mode PPN[1:0] forced to zero | Only when Sv48x4 supported; write PPN=0x5001 (lower bits=01), read back PPN | `PPN & 0x3 == 0` (lower 2 bits forced to zero) |

---

### Group 6: VMID Field Width Verification

**Specification Basis**:
- `norm:hgatp_vmid_op` (`SPEC/hypervisor.adoc:1000-1010`): hgatp VMID field is WARL, implementation may support fewer than 14 bits (RV64) VMID width
- Shgatpa implies "MODE supported by hgatp also means VMID field is available"

**Test Responsibilities**: Verify actual supported width of VMID field (confirmed by writing all 1s and reading back); verify whether VMID field is preserved after MODE switching.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HGATP-VMID-01 | VMID width probe | Write hgatp VMID field to all 1s (14-bit maximum 0x3FFF), read back to confirm actual supported width | Readback VMID is consecutive lower 1s (e.g., 0x3FFF indicates full 14-bit support, 0x003F indicates only 6-bit support) |
| HGATP-VMID-02 | VMID preservation after MODE switch | Write hgatp MODE=Sv39x4 + VMID=0x1234; switch MODE to Bare then back to Sv39x4; read back VMID | VMID value may be cleared (WARL allows), record behavior without pass/fail judgment |

---

## Appendix: Related Constants and API Reference

### CSR and Bit Definitions

| Name | Value | Description |
|------|-------|-------------|
| `CSR_HGATP` | `0x680` | Hypervisor guest address translation and protection, see `common/encoding.h:284` |
| `CSR_SATP` | `0x180` | Supervisor address translation and protection, see `common/encoding.h:145` |
| `HGATP64_MODE_SHIFT` | 60 | MODE field starting bit position when HSXLEN=64 |
| `HGATP64_VMID_SHIFT` | 44 | VMID field starting bit |
| `HGATP64_PPN_MASK` | `(1UL << 44) - 1` | PPN field mask (44 bits) |
| `HGATP64_VMID_MASK` | `(1UL << 14) - 1` | VMID field mask (up to 14 bits) |

### MODE Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `HGATP_MODE_BARE` | 0 | No translation protection (GPA=SPA) |
| `HGATP_MODE_SV39X4` | 8 | 41-bit GPA paging |
| `HGATP_MODE_SV48X4` | 9 | 50-bit GPA paging |
| `HGATP_MODE_SV57X4` | 10 | 59-bit GPA paging |

### satp → hgatp Mode Mapping

| satp MODE Constant | satp Value | hgatp MODE Constant | hgatp Value | Mapping Basis |
|--------------------|------------|---------------------|-------------|---------------|
| `MODE_BARE` | 0 | `HGATP_MODE_BARE` | 0 | `norm:shgatpa_hgatp_bare_mode` (independent requirement) |
| `MODE_SV39` | 8 | `HGATP_MODE_SV39X4` | 8 | `norm:shgatpa_satp_hgatp_mode_support` |
| `MODE_SV48` | 9 | `HGATP_MODE_SV48X4` | 9 | `norm:shgatpa_satp_hgatp_mode_support` |
| `MODE_SV57` | 10 | `HGATP_MODE_SV57X4` | 10 | `norm:shgatpa_satp_hgatp_mode_support` |

### Global Variables

| Variable | Type | Description |
|----------|------|-------------|
| `g_satp_supports_bare` | `volatile bool` | Whether satp supports Bare mode |
| `g_satp_supports_sv39` | `volatile bool` | Whether satp supports Sv39 mode |
| `g_satp_supports_sv48` | `volatile bool` | Whether satp supports Sv48 mode |
| `g_satp_supports_sv57` | `volatile bool` | Whether satp supports Sv57 mode |

### Test Framework API

- `MAKE_HGATP(mode, vmid, ppn)`: Construct hgatp register value
- `HGATP_GET_MODE(v)` / `HGATP_GET_PPN(v)` / `HGATP_GET_VMID(v)`: Extract hgatp fields
- `hyp_reset_state()`: Reset all hypervisor CSRs
- `HYP_TEST_END()`: Test end macro, includes hyp_reset_state + result recording
- `TEST_REGISTER` / `TEST_BEGIN` / `TEST_ASSERT` / `TEST_SKIP` / `TEST_END`: Test case registration and assertion macros
- `TEST_LOG(fmt, ...)`: Test log output

---

## Test Execution Notes

### Runtime Environment

- Group 1: M/HS-mode directly operates CSR 0x180 (satp), detects supported modes
- Group 2: M/HS-mode directly operates CSR 0x680 (hgatp), verifies SvNNx4 mode consistency
- Group 3: M/HS-mode directly operates CSR 0x680, verifies Bare mode support
- Group 4: M/HS-mode directly operates CSR 0x680, verifies WARL behavior
- Group 5: M/HS-mode directly operates CSR 0x680, verifies PPN lower 2 bits behavior
- Single-core environment, no IPI required
- All tests execute in M-mode or HS-mode (V=0) (hgatp can only be accessed from V=0)

### Failure Diagnosis Guide

| Symptom | Possible Cause |
|---------|----------------|
| HGATP-PROBE-02/03/04 all show unsupported | Platform implements no paging modes (only Bare), need to confirm hardware capability |
| HGATP-MODE-01 fails (satp supports Sv39 but hgatp does not support Sv39x4) | **Shgatpa constraint violation**: hgatp.MODE does not implement Sv39x4 |
| HGATP-MODE-02/03 fail | Same as above, hgatp does not implement x4 versions corresponding to advanced translation modes supported by satp |
| HGATP-BARE-01/02 fail (MODE written as 0 but readback non-zero) | hgatp Bare mode not implemented, **violates `norm:shgatpa_hgatp_bare_mode`** |
| HGATP-WARL-01/02/03 readback MODE equals reserved value | hgatp.MODE WARL implementation abnormal (should not hold illegal values) |
| HGATP-WARL-05 fails | MODE readback is reserved value from 1-7 or 11-15, WARL not working correctly |
| HGATP-PPN-01/03 fail (PPN[1:0] != 0) | hgatp.PPN lower 2 bits not correctly implemented as read-only zero, violates `norm:hgatp_ppn_op` |
