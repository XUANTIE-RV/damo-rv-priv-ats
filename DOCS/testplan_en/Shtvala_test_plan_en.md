**[中文](../testplan/Shtvala_test_plan.md) | English**

# Shtvala Test Plan

## Overview

This test plan verifies the **Shtvala extension** behavior: when a guest-page-fault (GPF) trap is taken into HS-mode, the `htval` register **must** be written with the faulting guest physical address (GPA >> 2), and **must not** be zero. This tightens the baseline H-extension rule that allowed implementations to write either zero or the GPA.

The test suite covers explicit G-stage accesses, implicit VS-stage translation faults, cross-page/misaligned accesses, HLV/HLVX/HSV instruction paths, G-stage MODE variants with high-bit overflow, non-GPF trap clearing behavior, htval/stval consistency for GPA reconstruction, and hstatus.GVA linkage.

---

## Test Scope

### Specification Sources

- `SPEC/shtvala.adoc` — Shtvala extension definition
- `SPEC/hypervisor.adoc` — H-extension baseline (htval/mtval2/hstatus/hgatp/vsatp semantics)

### Key Reference Files

| File Path | Description |
|-----------|-------------|
| `common/hyp/hyp_test.h:28-37` | `EXPECT_GUEST_PAGE_FAULT(cause, stmt)` macro |
| `common/hyp/hyp_test.h:39-42` | `CHECK_HTVAL(msg, expected_gpa_shifted)` macro, asserts htval == expected GPA>>2 |
| `common/hyp/hyp_test.h:44-46` | `CHECK_HTINST(msg, expected)` macro |
| `common/hyp/hyp_test.h:48-50` | `CHECK_GVA(msg, expected)` macro |
| `common/hyp/hyp_priv.h` | `run_in_vs_mode()`, `run_in_vu_mode()` |
| `common/hyp/gstage_pt.h` | G-stage page table API (`gstage_pt_init`, `gstage_pt_map_page`) |
| `common/hyp/two_stage.h` | vsatp+hgatp two-stage joint configuration |
| `common/encoding.h` | `CAUSE_INST_GUEST_PAGE_FAULT (20)` / `CAUSE_LOAD_GUEST_PAGE_FAULT (21)` / `CAUSE_STORE_GUEST_PAGE_FAULT (23)` |
| `DOCS/framework/hypervisor_framework.md` | Hypervisor test framework overview |
| `DOCS/testplan/sstvala_test_plan.md` | Sibling S-mode stval test plan, structural reference baseline |
| `DOCS/testplan/hyp_2_stage_translation_test_plan.md` | Two-stage translation baseline (including implicit access scenarios) |
| `DOCS/testplan/hyp_gstage_translation_test_plan.md` | G-stage translation baseline |

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:shtvala_htval_faulting_gpa` | If the Shtvala extension is implemented, `htval` must be written with the faulting guest physical address in all circumstances permitted by the ISA. |
| `norm:htval_sz_acc_op` | The `htval` register is an HSXLEN-bit read/write register. When a trap is taken into HS-mode, `htval` is written with additional exception-specific information, alongside `stval`, to assist software in handling the trap. |
| `norm:htval_trapval` | When a guest-page-fault trap is taken into HS-mode, `htval` is written with either zero or the guest physical address that faulted, shifted right by 2 bits. For other traps, `htval` is set to zero. |
| `norm:htval_val` | `htval` is a WARL register that must be able to hold zero and may be capable of holding only an arbitrary subset of other 2-bit-shifted guest physical addresses, if any. |
| `norm:H_guest_page_fault` | Guest-page-fault traps may be delegated from M-mode to HS-mode under the control of `medeleg`, but cannot be delegated to other privilege modes. On a guest-page fault, `mtval` or `stval` is written with the faulting guest virtual address, and `mtval2` or `htval` is written either with zero or with the faulting guest physical address, shifted right by 2 bits. |
| `norm:H_straddle` | When an instruction fetch or a misaligned memory access straddles a page boundary, two different address translations are involved. When a guest-page fault occurs, the faulting virtual address may be a page-boundary address that is higher than the instruction's original virtual address. |
| `norm:mtval2_htval_virtaddr` | When a guest-page fault is not due to an implicit memory access for VS-stage address translation, a nonzero guest physical address written to `mtval2`/`htval` shall correspond to the exact virtual address written to `mtval`/`stval`. |
| `norm:hgatp_mode_x4` | When `hgatp`.MODE specifies a translation scheme of Sv32x4, Sv39x4, Sv48x4, or Sv57x4, G-stage address translation is a variation on the usual page-based scheme. In each case, the size of the incoming address is widened by 2 bits. The root page table is expanded by a factor of 4 to be 16 KiB and must be aligned to a 16 KiB boundary. |
| `norm:hgatp_mode_sv39x4` | For Sv39x4, partitioning is identical to Sv39, except with 2 more bits at the high end in VPN[2]. Address bits 63:41 must all be zeros, or else a guest-page-fault exception occurs. |
| `norm:hgatp_mode_sv48x4` | For Sv48x4, partitioning is identical to Sv48, except with 2 more bits at the high end in VPN[3]. Address bits 63:50 must all be zeros, or else a guest-page-fault exception occurs. |
| `norm:hgatp_mode_sv57x4` | For Sv57x4, partitioning is identical to Sv57, except with 2 more bits at the high end in VPN[4]. Address bits 63:59 must all be zeros, or else a guest-page-fault exception occurs. |
| `norm:hstatus_gva_op` | Field GVA (Guest Virtual Address) is written by the implementation whenever a trap is taken into HS-mode. For any trap that writes a guest virtual address to `stval`, GVA is set to 1. For any other trap into HS-mode, GVA is set to 0. |

> [!IMPORTANT]
> The core of the Shtvala specification is a **single tightening rule**: changing the H-extension original text from "either zero or the guest physical address" to "must be the guest physical address". All test cases revolve around "trigger GPF → verify htval must equal GPA>>2 and != 0", and leverage adjacent norms from the H-extension to achieve complete coverage in scenarios where Shtvala does not directly constrain but interacts with htval behavior (such as Sv*x4 high-bit overflow, cross-page accesses, non-GPF trap clearing, etc.).

### Out of Scope

- **`mtval2` behavior**: The Shtvala SPEC original text does not cover `mtval2`. `mtval2` is constrained by the H-extension baseline `norm:mtval2_trapval` series (including medeleg=0 non-delegation, `mstatus.MPRV+MPV` triggering M-mode two-stage access), and belongs to the H-extension baseline test plan. However, the GPF path for HLV/HLVX/HSV instructions in Group 5 (trap to M-mode, writing mtval2) is included in this plan as symmetric coverage of Shtvala constraints.
- **Strong constraint on whether `stval` writes GVA**: Belongs to the Sstvala extension (`SPEC/sstvala.adoc`)
- **`htinst` encoding**: Belongs to the H-extension baseline (`norm:htinst_val`)
- **Sv*x4 G-stage translation path correctness**: Belongs to `hyp_gstage_translation_test_plan.md`
- **VS-stage translation path correctness**: Belongs to `hyp_2_stage_translation_test_plan.md`
- **`medeleg` / `hedeleg` delegation bit WARL**: Belongs to M/H-extension baseline
- **VMID, ASID behavior**: Belongs to H-extension baseline
- **Multi-hart scenarios**: Project is a single-core test environment
- **RV32 / Sv32x4**: Only covers RV64 + Sv39x4/Sv48x4/Sv57x4, consistent with other extension plans in the project

---

## Design Key Points

### 1. Generic Pattern for htval Verification

All test cases follow a unified pattern:

```
1. Set medeleg + hedeleg to delegate cause 20/21/23 to HS-mode (avoid falling to M-mode writing mtval2)
2. Configure vsatp + hgatp (two_stage.h), construct target GPF scenario
3. EXPECT_GUEST_PAGE_FAULT(cause, { code entering VS-mode to trigger fault })
4. CHECK_HTVAL("htval == faulting GPA >> 2", expected_gpa >> 2)
5. Additional assertion: trap_get_htval() != 0 (Shtvala incremental core — exclude write-0 implementations)
6. HYP_TEST_END()
```

In the framework, `trap_record.htval` is read from `htval` in the HS-mode handler (`common/trap.c:189`) and from `mtval2` in the M-mode handler (`common/trap.c:180`), uniformly storing GPA>>2. This test plan **only uses the HS-mode path** (since mtval2 is out of scope).

### 2. Classification of Faulting GPA Sources

There are three types of faulting GPA sources in tests, which must be explicitly annotated:

| Source Type | GPA Calculation Method | Anchored Norm |
|-------------|------------------------|---------------|
| Explicit G-stage access | GPA obtained after GVA translation via vsatp | `norm:htval_trapval` main paragraph |
| Implicit VS-stage translation | GPA of the PTE itself (not the GPA corresponding to the original GVA) | `norm:htval_trapval` second paragraph |
| Cross-page / misaligned | GPA of the faulting portion (bytes after page-boundary) | `norm:htval_trapval` third paragraph + `norm:H_straddle` |

### 3. vsatp=Bare Simplification Technique

To simplify expected GPA calculation, **some test cases can set `vsatp.MODE=Bare`**, making GVA directly equal to GPA. In this case, the expected htval for G-stage GPF equals stval >> 2, facilitating assertions. Borrowed from qinghao's plan.

### 4. WARL Verification Strategy

`norm:htval_val` allows htval to hold only a subset of GPAs. Verification strategy:
- Writing 0 must preserve 0 (mandatory requirement)
- Writing faulting GPA>>2 within implementation width (IMPL_HTVAL_WIDTH) must be fully preserved
- Writing values obviously exceeding IMPL_HTVAL_WIDTH, readback returns WARL subset (no exception thrown)
- Shtvala joint constraint: The implementation's WARL subset must be able to hold **all possible faulting GPAs** within implementation width; otherwise Shtvala cannot be satisfied

### 5. Delegation Configuration (hedeleg/medeleg)

Prerequisites for all GPF test cases:
- `medeleg[20] = medeleg[21] = medeleg[23] = 1` (M-mode delegates to HS)
- `hedeleg[20] = hedeleg[21] = hedeleg[23] = 0` (HS does not delegate to VS; otherwise GPF goes to VS handler without writing htval)

### 6. Platform Capability Detection

- Test case entry detects whether Shtvala is implemented (specific method determined by platform ABI, such as ISA string `"_shtvala"` parsing, feature register bits, or compile-time macro `ENABLE_SHTVALA`)
- When Shtvala is not implemented, the entire file `TEST_SKIP`s (baseline H-extension may write 0, assertion `htval != 0` would fail)

---

## Test Groups

> [!IMPORTANT]
> A total of 9 test groups and 30 test cases.
> Group 1 verifies htval register attributes (basic prerequisite); Groups 2-4 are Shtvala core constraints (htval must write GPA on GPF); Group 5 covers HLV/HLVX/HSV instruction paths (V=0 trap to M-mode, verifying mtval2); Group 6 covers three G-stage MODEs and GPA high-bit overflow; Group 7 verifies non-GPF traps do not pollute htval; Group 8 verifies htval/stval GPA complete reconstruction; Group 9 verifies hstatus.GVA linkage.

---

### Group 1: htval Register Attributes (Basic RW and WARL)

**Specification Basis**:
- `norm:htval_sz_acc_op` (`SPEC/hypervisor.adoc:906-910`): htval is an HSXLEN-bit RW register
- `norm:htval_val` (`SPEC/hypervisor.adoc:956-959`): htval is WARL, must hold 0

**Test Responsibility**: Verify that the htval register is readable and writable in M-mode/HS-mode, WARL behavior is legal, and VS/VU-mode access triggers illegal-instruction. This is the prerequisite for subsequent Shtvala assertions.

**Preconditions**:
- After M-mode boot, switch to HS-mode, confirm misa.H = 1
- No delegation or two-stage translation required

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HTVAL-REG-01 | M-mode write-read htval | M-mode explicitly `csrw htval, 0xDEADBEEF` then `csrr` readback, confirm htval is readable/writable | Readback value falls within WARL subset (implementation-allowed GPA>>2 subset) |
| HTVAL-REG-02 | HS-mode write-read htval | HS-mode explicitly writes `0` then readback must be `0`; writes `MAX_VALID_GPA >> 2` then readback fully preserved | Write 0 readback = 0; write legal GPA>>2 readback fully preserved |
| HTVAL-REG-03 | WARL subset legality | HS-mode writes obviously illegal all-ones value `~0UL`, readback returns WARL subset (no exception thrown) | No exception triggered; readback value ⊆ written value (bitwise) |
| HTVAL-REG-04 | VS/VU-mode access htval | VS-mode executes `csrr x5, htval` → cause 22 (virtual-instruction); VU-mode executes same → cause 2 (illegal-instruction) | VS: cause=22; VU: cause=2 |

---

### Group 2: Explicit G-stage Access GPF (Shtvala Core: htval Must Be Non-zero)

**Specification Basis**:
- `norm:shtvala_htval_faulting_gpa` (`SPEC/shtvala.adoc:4-6`): htval must write faulting GPA
- `norm:htval_trapval` (`SPEC/hypervisor.adoc:916-921`): On GPF, htval writes GPA>>2
- `norm:H_guest_page_fault` (`SPEC/hypervisor.adoc:2043-2051`): GPF general rule
- `norm:mtval2_htval_virtaddr` (`SPEC/hypervisor.adoc:2063-2067`): When not implicit, GPA corresponds to stval

**Test Responsibility**: When VS-mode explicit access triggers G-stage GPF (cause 20/21/23), verify `htval == faulting GPA >> 2` and `htval != 0`. This is the core distinction between Shtvala and baseline H-extension.

**Preconditions**:
- vsatp = Sv39 or Bare (some cases use Bare to simplify expected values)
- hgatp = Sv39x4, construct target GPA with Invalid / missing permission PTE in G-stage
- medeleg[20]=medeleg[21]=medeleg[23]=1, hedeleg[20]=hedeleg[21]=hedeleg[23]=0

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HTVAL-LGP-01 | Load GPF: G-stage Invalid PTE | vsatp=Bare, VS-mode load to GVA=GPA=0x80100000, G-stage PTE V=0 → cause 21 | `cause == 21`; `htval == 0x80100000 >> 2 != 0` |
| HTVAL-LGP-02 | Load GPF: G-stage missing R permission | vsatp=Bare, G-stage PTE V=1, R=0, X=1, VS-mode load → cause 21 | `cause == 21`; `htval == GPA >> 2 != 0` |
| HTVAL-SGP-01 | Store GPF: G-stage missing W permission | vsatp=Bare, G-stage PTE V=1, R=1, W=0, VS-mode store → cause 23 | `cause == 23`; `htval == GPA >> 2 != 0` |
| HTVAL-SGP-02 | Store GPF: G-stage A=1 D=0 write access | G-stage PTE V=1, R=1, W=1, A=1, D=0, VS-mode store → cause 23 (without Svadu) | `cause == 23`; `htval == GPA >> 2 != 0` |
| HTVAL-IGP-01 | Inst GPF: G-stage missing X permission | vsatp=Bare, G-stage PTE V=1, R=1, X=0, VS-mode jump execute → cause 20 | `cause == 20`; `htval == fault PC GPA >> 2 != 0` |
| HTVAL-LGP-03 | Load GPF: vsatp=Sv39 + G-stage Invalid | vsatp=Sv39 sets GVA→GPA translation, G-stage has Invalid for that GPA → cause 21 | `cause == 21`; `htval == translated GPA >> 2 != 0`, different from stval (GVA) |
| HTVAL-AMO-01 | AMO instruction Store GPF | vsatp=Bare, VS-mode executes `amoadd.d` on GPA missing W permission in G-stage → cause 23 | `cause == 23`; `htval == GPA >> 2 != 0` (AMO shares cause 23 with Store but trigger path differs: AMO requires R+W permissions) |

---

### Group 3: Implicit VS-stage Translation GPF (htval = PTE's Own GPA)

**Specification Basis**:
- `norm:shtvala_htval_faulting_gpa` (`SPEC/shtvala.adoc:4-6`)
- `norm:htval_trapval` (`SPEC/hypervisor.adoc:922-929`): "a guest physical address written to `htval` is that of the implicit memory access that faulted—for example, the address of a VS-level page table entry that could not be read"

**Test Responsibility**: Verify that when implicit PTE read during VS-stage translation triggers G-stage GPF, `htval` equals the **PTE's own** GPA (not the GPA after VS-stage translation of the original GVA, since that value is unknown when VS-stage translation fails).

**Preconditions**:
- vsatp = Sv39 (mandatory, Bare does not trigger implicit access)
- Root/intermediate/leaf PTEs at each level of VS-stage page tables each occupy one GPA
- hgatp intentionally omits mapping for the GPA where a certain level PTE resides

> [!NOTE]
> Shtvala enforces the implicit-VS-stage path already described in H-extension original text but previously allowed to "write zero". In implicit access scenarios, the low 2 bits of htval are 0 (PTEs are physically 8-byte aligned).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HTVAL-IMP-01 | Implicit L0: VS root table PTE unmapped | vsatp Sv39 root page table GPA unmapped in hgatp, VS-mode load triggers implicit read → cause 21 | `cause == 21`; `htval == VS root table PTE GPA >> 2 != 0`; unrelated to original GVA |
| HTVAL-IMP-02 | Implicit L1: VS intermediate PTE unmapped | vsatp Sv39 intermediate page table GPA unmapped in hgatp, VS-mode load → cause 21 | `cause == 21`; `htval == intermediate PTE GPA >> 2 != 0` |
| HTVAL-IMP-03 | Implicit L2: VS leaf PTE unmapped | vsatp Sv39 leaf page table GPA unmapped in hgatp, VS-mode load → cause 21 | `cause == 21`; `htval == leaf PTE GPA >> 2 != 0` |
| HTVAL-IMP-04 | Implicit + store/inst | Same as HTVAL-IMP-01 but using store/instruction fetch respectively → cause 23/20 | `cause == 23/20`; `htval == root table PTE GPA >> 2 != 0` |
| HTVAL-IMP-05 | htinst == pseudoinstruction (read) on implicit read GPF | Reuse HTVAL-IMP-01 scenario (implicit PTE read triggers GPF), additionally verify `htinst` | `htinst == 0x00003000` (RV64 read pseudoinstruction, `norm:H_trap_xtinst_guestpage`) |
| HTVAL-IMP-06 | htinst == pseudoinstruction (write) on implicit write GPF | vsatp Sv39 constructs VS-stage PTE A=0 (requires hardware auto-set A), implicit write triggers GPF → cause 23 | `htinst == 0x00003020` (RV64 write pseudoinstruction); if platform does not auto-update A/D then `TEST_SKIP` |
| HTVAL-IMP-07 | Sv48x4 implicit GPF path | vsatp=Sv48, construct L2 PTE GPA unmapped in G-stage (Sv48x4) → cause 21 | `htval == L2 PTE GPA >> 2 != 0`; if platform does not support Sv48 then `TEST_SKIP` |

---

### Group 4: Cross-page / Misaligned Access (htval = faulting portion)

**Specification Basis**:
- `norm:htval_trapval` (`SPEC/hypervisor.adoc:931-936`): "for misaligned loads and stores that cause guest-page faults, a nonzero guest physical address in `htval` corresponds to the faulting portion of the access"
- `norm:H_straddle` (`SPEC/hypervisor.adoc:2053-2061`): Cross-page access stval writes page-boundary GVA
- `norm:mtval2_htval_virtaddr` (`SPEC/hypervisor.adoc:2063-2067`): When not implicit, GPA corresponds to exact GVA in stval

**Test Responsibility**: When load/store/instruction fetch access straddles a page boundary, with the first half succeeding and the second half triggering G-stage GPF, verify `htval` equals the **faulting portion's** GPA (i.e., the GPA starting from the byte after the page boundary), not the original access start address.

**Preconditions**:
- vsatp=Bare simplification (GVA=GPA)
- hgatp Sv39x4: first half page (page before faulting page) normally mapped, target page Invalid
- Platform does not support hardware transparent unaligned access (otherwise skip some cases)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HTVAL-STR-01 | Cross-4KB load GPF | 8-byte load starting at `0x80000FFC`, crossing 4KB boundary into unmapped `0x80001000` page → cause 21 | `cause == 21`; `htval == 0x80001000 >> 2 != 0`; `stval == 0x80001000` |
| HTVAL-STR-02 | Cross-2MB superpage store GPF | 8-byte store starting at `0x801FFFFC`, crossing 2MB boundary into unmapped `0x80200000` superpage → cause 23 | `cause == 23`; `htval == 0x80200000 >> 2 != 0` |
| HTVAL-STR-03 | Cross-4KB instruction GPF (variable length) | 4-byte uncompressed instruction at `0x80000FFE`, crossing 4KB boundary into unmapped page for execution → cause 20 | `cause == 20`; `htval == 0x80001000 >> 2 != 0` |

> [!WARNING]
> Most RISC-V implementations support hardware transparent unaligned access. HTVAL-STR-01 / STR-02 need prior detection on such platforms: if no misaligned exception is triggered but GPF is triggered, continue; if hardware ignores unalignment resulting in single-page access, the case `TEST_SKIP`s.

---

### Group 5: G-stage GPF Triggered by HLV / HLVX / HSV Instructions (mtval2 = GPA >> 2)

**Specification Basis**:
- `norm:shtvala_htval_faulting_gpa` (`SPEC/shtvala.adoc:4-6`): htval / mtval2 must write faulting GPA
- `norm:htval_trapval` / `norm:mtval2_trapval` (`SPEC/hypervisor.adoc:916-921`): On GPF, write GPA>>2
- `norm:hlsv_trans` (`SPEC/hypervisor.adoc:1492`): HLV/HLVX/HSV always perform two-stage translation
- `norm:hlsv_mode` (`SPEC/hypervisor.adoc:1488`): HLV/HLVX/HSV are valid only in M-mode, HS-mode, or U-mode (hstatus.HU=1)
- `SPEC/hypervisor.adoc:1505-1507` (NOTE): HLVX exception type is the same as load (cause=21), **not** instruction fetch exception (cause=20)

**Test Responsibility**: Verify that when executing HLV/HLVX/HSV instructions in HS-mode (V=0) performing two-stage translation, if G-stage mapping miss causes GPF, mtval2 correctly writes GPA >> 2. HLV/HSV executed at V=0 trap GPF to M-mode, so verification targets are `mcause` and `mtval2` (semantically symmetric to `htval`).

**Preconditions**:
- Execute HLV/HLVX/HSV instructions in HS-mode (V=0)
- hgatp=Sv39x4, target GPA unmapped in G-stage (PTE.V=0)
- HLV/HSV use vsatp=Bare (address is GPA)
- HLVX uses vsatp=Sv39 (identity-mapped VS-stage), enabling deterministic cause=21 on G-stage failure
- HLVX effective access privilege controlled by `hstatus.SPVP`: SPVP=0 for VU (PTE.U=1), SPVP=1 for VS (PTE.U=0)
- Additionally cover vsatp=BARE path, verifying HLVX exception classification without VS-stage translation

> [!NOTE]
> GPF triggered by HLV/HSV executed in HS-mode (V=0) traps to M-mode, so `mcause` and `mtval2` are verified. The framework's `trap_get_cause()` / `trap_get_htval()` return `mcause` / `mtval2` values respectively on M-mode trap.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HTVAL-HLV-01a | HLVX.WU @ VU (SPVP=0) | HS-mode executes HLVX.WU, SPVP=0 (VU identity), VS PTE U=1, target GPA unmapped | `mcause == 21` (load GPF); `mtval2 == GPA>>2` |
| HTVAL-HLV-01b | HLVX.WU @ VS (SPVP=1) | HS-mode executes HLVX.WU, SPVP=1 (VS identity), VS PTE U=0, target GPA unmapped | `mcause == 21` (load GPF); `mtval2 == GPA>>2` |
| HTVAL-HLV-02 | HLV.D G-stage load GPF | HS-mode executes HLV.D, target GPA unmapped | `mcause == 21` (load GPF); `mtval2 == GPA>>2` |
| HTVAL-HLV-03 | HSV.D G-stage store GPF | HS-mode executes HSV.D, target GPA unmapped | `mcause == 23` (store GPF); `mtval2 == GPA>>2` |
| HTVAL-HLV-04 | HLVX.WU (vsatp=BARE) | HS-mode executes HLVX.WU, vsatp=BARE (VA directly as GPA), target GPA unmapped | `mcause == 21` (load GPF); `mtval2 == GPA>>2` |

---

### Group 6: G-stage MODE Coverage and GPA High-bit Overflow

**Specification Basis**:
- `norm:hgatp_mode_x4` (`SPEC/hypervisor.adoc:1914-1924`): Sv*x4 input GPA has 2 more bits than Sv*
- `norm:hgatp_mode_sv39x4` (`SPEC/hypervisor.adoc:1943-1947`): bits 63:41 must be 0, otherwise GPF
- `norm:hgatp_mode_sv48x4` (`SPEC/hypervisor.adoc:1953-1959`): bits 63:50 must be 0, otherwise GPF
- `norm:hgatp_mode_sv57x4` (`SPEC/hypervisor.adoc:1965-1971`): bits 63:59 must be 0, otherwise GPF
- `norm:shtvala_htval_faulting_gpa`: Linkage — even in overflow scenarios, htval must write GPA

**Test Responsibility**: Verify that under three G-stage MODEs (Sv39x4/Sv48x4/Sv57x4), regardless of whether GPF is caused by legal-range GPA or high-bit overflow GPA, htval must write complete faulting GPA>>2.

**Preconditions**:
- Cases configure hgatp.MODE = Sv39x4 / Sv48x4 / Sv57x4 respectively
- For overflow cases, platform must implement corresponding GPA bus width (otherwise overflow bits are truncated in hardware, untestable)

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HTVAL-MOD-01 | Sv39x4 legal range GPF | hgatp Sv39x4, access GPA = `0x100_0000_0000` (legal within 41-bit) but unmapped → cause 21 | `cause == 21`; `htval == GPA >> 2 != 0` |
| HTVAL-MOD-02 | Sv39x4 high-bit overflow GPF | hgatp Sv39x4, access GPA bit 41=1 (e.g., `0x200_0000_0000`), overflow → cause 21 | `cause == 21`; `htval == overflow GPA >> 2 != 0` (fully preserved within implementation width) |
| HTVAL-MOD-03 | Sv48x4 high-bit overflow GPF | hgatp Sv48x4, access GPA bit 50=1, overflow → cause 21 | `cause == 21`; `htval == overflow GPA >> 2 != 0` |
| HTVAL-MOD-04 | Sv57x4 high-bit overflow GPF | hgatp Sv57x4, access GPA bit 59=1, overflow → cause 21 | `cause == 21`; `htval == overflow GPA >> 2 != 0` |

---

### Group 7: Non-GPF Trap Does Not Pollute htval (Must Write 0)

**Specification Basis**:
- `norm:htval_trapval` (`SPEC/hypervisor.adoc:919`): "For other traps, `htval` is set to zero"

**Test Responsibility**: Shtvala strengthens that htval must be non-zero on GPF; conversely, non-GPF traps entering HS-mode must **actively** clear htval (to prevent reading residual values from previous GPF).

**Preconditions**:
- First trigger a GPF to make htval non-zero as "pollution" preparation
- Immediately trigger a non-GPF trap (ecall / illegal / page-fault / virtual-instruction), which also enters HS-mode

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HTVAL-CLR-01 | GPF then ecall | Trigger cause 21 (htval writes non-zero); ecall from VS-mode/VU-mode (cause 8/9/10) enters HS | Second trap: `cause==8/9/10`; `htval == 0` |
| HTVAL-CLR-02 | GPF then illegal-instruction | Trigger cause 21; HS-mode executes illegal instruction (cause 2) | Second trap: `cause==2`; `htval == 0` |
| HTVAL-CLR-03 | GPF then normal page-fault | Trigger cause 21; HS-mode triggers cause 12/13/15 (normal page-fault, non-guest) | Second trap: `cause==12/13/15`; `htval == 0` |
| HTVAL-CLR-04 | GPF then virtual-instruction | Trigger cause 21; VS-mode executes HS-level CSR (cause 22) | Second trap: `cause==22`; `htval == 0` |

---

### Group 8: htval / stval Consistency + Low 2 Bits Recovery via stval

**Specification Basis**:
- `norm:mtval2_htval_virtaddr` (`SPEC/hypervisor.adoc:2063-2067`): When not implicit, GPA must correspond to stval
- `norm:htval_trapval` NOTE paragraph (`SPEC/hypervisor.adoc:948-953`): "the least-significant two bits are ordinarily the same as the least-significant two bits of the faulting virtual address in `stval`. For faults due to implicit memory accesses for VS-stage address translation, the least-significant two bits are instead zeros"

**Test Responsibility**: Since htval stores GPA>>2, complete GPA reconstruction requires:
- **Explicit access**: `(htval << 2) | (stval & 0x3) == faulting GPA`
- **Implicit VS-stage access**: Low 2 bits forced to 0 (PTE 8-byte aligned), unrelated to stval low 2 bits

**Preconditions**:
- Explicit cases: vsatp=Bare, construct GVA=GPA with low 2 bits ≠ 00 (typically `0x???????3`)
- Implicit cases: reuse Group 3 setup

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HTVAL-CON-01 | Explicit GPF low 2 bits recovery via stval | vsatp=Bare, access GVA=`0x80100003` (low 2 bits=0b11) → cause 21 | `(htval << 2) \| (stval & 0x3) == 0x80100003`; `stval & 0x3 == 0x3` |
| HTVAL-CON-02 | Explicit GPF different low 2 bit combinations | Run once each with `0x...01`, `0x...02`, `0x...03` three low 2 bit combinations | Each time `(htval << 2) \| (stval & 0x3) == GVA` |
| HTVAL-CON-03 | Implicit GPF low 2 bits must be 0 | Reuse HTVAL-IMP-03 setup, verify `(htval << 2) & 0x3 == 0`, unrelated to stval low 2 bits | `htval << 2` low 2 bits are 0; not necessarily equal to stval low 2 bits |

---

### Group 9: hstatus.GVA Linkage

**Specification Basis**:
- `norm:hstatus_gva_op` (`SPEC/hypervisor.adoc:321-326`): On entering HS-mode, if GVA is written to stval, then hstatus.GVA=1; otherwise GVA=0

**Test Responsibility**: Shtvala does not directly govern hstatus.GVA, but on GPF, stval writing GVA and htval writing GPA are a pair of linked information. Verify both states are consistent: GPF → GVA=1 and htval≠0; non-GPF → GVA=0 and htval=0.

**Preconditions**:
- Reuse Group 2 / Group 7 setup

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HTVAL-GVA-01 | GVA=1 && htval≠0 on GPF | Trigger cause 21 | `hstatus.GVA == 1`; `stval != 0`; `htval != 0` |
| HTVAL-GVA-02 | GVA=0 && htval=0 on non-GPF | VS/VU ecall enters HS (cause 8/9/10) | `hstatus.GVA == 0`; `htval == 0` |

---

## Appendix: Related Constants and API Reference

### Exception Cause Values (Related to Shtvala)

| Name | Value | Description | htval Semantics (Shtvala) |
|------|-------|-------------|---------------------------|
| `CAUSE_INST_GUEST_PAGE_FAULT` | 20 | Instruction guest-page-fault | **Must** write faulting GPA >> 2 |
| `CAUSE_LOAD_GUEST_PAGE_FAULT` | 21 | Load guest-page-fault | **Must** write faulting GPA >> 2 |
| `CAUSE_STORE_GUEST_PAGE_FAULT` | 23 | Store/AMO guest-page-fault | **Must** write faulting GPA >> 2 |
| `CAUSE_VIRTUAL_INSTRUCTION` | 22 | Virtual instruction exception | Write 0 (non-GPF) |
| `CAUSE_S_ECALL` / `VS_ECALL` / `U_ECALL` | 9 / 10 / 8 | ecall | Write 0 (non-GPF) |
| `CAUSE_ILLEGAL_INST` | 2 | Illegal instruction | Write 0 (non-GPF) |
| `CAUSE_LOAD_PAGE_FAULT` | 13 | Normal load page-fault | Write 0 (non-GPF) |
| `CAUSE_STORE_PAGE_FAULT` | 15 | Normal store page-fault | Write 0 (non-GPF) |

### Framework API Reference

| Function/Macro | Path | Description |
|----------------|------|-------------|
| `trap_get_htval()` | `common/trap.c:502` | Returns htval/mtval2 value (GPA>>2) of most recent trap |
| `trap_get_stval()` | `common/trap.c` | Returns stval (GVA) of most recent trap |
| `trap_get_cause()` | `common/trap.c` | Returns cause of most recent trap |
| `EXPECT_GUEST_PAGE_FAULT(cause, stmt)` | `common/hyp/hyp_test.h:28-37` | Expects stmt to trigger specified GPF cause |
| `CHECK_HTVAL(msg, expected)` | `common/hyp/hyp_test.h:39-42` | Asserts htval == expected (GPA>>2) |
| `CHECK_GVA(msg, expected)` | `common/hyp/hyp_test.h:48-50` | Asserts hstatus.GVA == expected |
| `HYP_TEST_END()` | `common/hyp/hyp_test.h:21-25` | Resets hypervisor state and ends test |
| `run_in_vs_mode(fn, arg)` | `common/hyp/hyp_priv.h` | Enters VS-mode to execute fn, returns after trap |
| `run_in_vu_mode(fn, arg)` | `common/hyp/hyp_priv.h` | Enters VU-mode to execute fn |
| `two_stage_init(ts, vs_mode, gs_mode)` | `common/hyp/two_stage.h` | Initializes two-stage translation context |
| `two_stage_map_vs(ts, gva, gpa, flags)` | `common/hyp/two_stage.h` | Adds mapping in VS-stage |
| `two_stage_map_gs_4k(ts, gpa, hpa, flags)` | `common/hyp/two_stage.h` | Adds 4K mapping in G-stage |
| `two_stage_activate(ts)` | `common/hyp/two_stage.h` | Writes vsatp + hgatp to activate |
| `delegate_gpf_to_hs()` | Test helper | Sets medeleg/hedeleg to delegate cause 20/21/23 to HS |
| `SHTVALA_REQUIRE()` | Test helper | `TEST_SKIP` when platform does not implement Shtvala |
| `REQUIRE_HGATP_MODE(mode)` | Test helper | `TEST_SKIP` when hgatp does not support specified MODE |

### CSR Definitions

| CSR Name | Number | Description |
|----------|--------|-------------|
| `htval` | `0x643` | Hypervisor trap value (GPA>>2) |
| `htinst` | `0x64A` | Hypervisor trap instruction |
| `hstatus` | `0x600` | Hypervisor status (includes GVA, SPV, SPVP) |
| `hgatp` | `0x680` | Hypervisor guest address translation and protection |
| `hedeleg` | `0x602` | Hypervisor exception delegation |
| `vsatp` | `0x280` | Virtual supervisor address translation and protection |
| `stval` | `0x143` | Supervisor trap value (stores GVA in HS-mode) |
| `medeleg` | `0x302` | Machine exception delegation |
| `mtval2` | `0x34B` | Machine second trap value (**out-of-scope**) |

### Test Case Summary

| Group | Test Count | Primary Exception Type | htval Semantics | Primary Anchored Norm |
|-------|------------|------------------------|-----------------|----------------------|
| Group 1 | 4 | Register attributes (no exception) | 0 / GPA>>2 (RW) | `htval_sz_acc_op` / `htval_val` |
| Group 2 | 7 | GPF cause 20/21/23 explicit access + AMO | Explicit GPA >> 2 (≠0) | `shtvala_htval_faulting_gpa` |
| Group 3 | 7 | GPF implicit VS-stage + htinst linkage | PTE GPA >> 2 (≠0) | `shtvala_htval_faulting_gpa` / `H_trap_xtinst_guestpage` |
| Group 4 | 3 | GPF cross-page / misaligned | Faulting portion GPA >> 2 (≠0) | `htval_trapval` (misaligned) / `H_straddle` |
| Group 5 | 3 | HLV/HLVX/HSV instruction GPF (V=0→M-mode) | mtval2 == GPA >> 2 | `hlsv_trans` / `mtval2_trapval` / `hypervisor.adoc:1505-1507` |
| Group 6 | 4 | Sv*x4 mode / high-bit overflow GPF | Complete GPA >> 2 (≠0) | `hgatp_mode_sv39x4/sv48x4/sv57x4` |
| Group 7 | 4 | Non-GPF trap | 0 | `htval_trapval` (other trap paragraph) |
| Group 8 | 3 | Explicit / implicit GPF low-bit reconstruction | Reconstructed GPA | `mtval2_htval_virtaddr` / `htval_trapval` (NOTE) |
| Group 9 | 2 | GPF / non-GPF GVA linkage | ≠0 / 0 | `hstatus_gva_op` |
| **Total** | **37** | | | |

### Differences Between Shtvala and Baseline H-extension

| Dimension | H-extension Baseline (without Shtvala) | This Plan (with Shtvala enabled) |
|-----------|----------------------------------------|----------------------------------|
| htval on GPF | Implementation allowed to write 0 | **Must** write faulting GPA>>2, and `!= 0` |
| Implicit VS-stage translation GPF | Implementation allowed to write 0 | **Must** write PTE's own GPA>>2 |
| Cross-page / misaligned GPF | Implementation allowed to write 0 | **Must** write faulting portion GPA>>2 |
| Sv*x4 high-bit overflow GPF | Implementation allowed to write 0 | **Must** write complete overflow GPA>>2 (subject to WARL subset limitation) |
| WARL subset | Allows minimal subset (even just 0) | Must hold all faulting GPAs within implementation width |
| htval on non-GPF trap | Write 0 (baseline already requires) | Same (Shtvala does not tighten) |
| `mtval2` behavior | Constrained by `mtval2_trapval` series | **Not constrained by Shtvala** (out-of-scope) |
