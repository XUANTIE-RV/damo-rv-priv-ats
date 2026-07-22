**[中文](../testplan/Hypervisor_Zpm_test_plan.md) | English**

# Hypervisor × Zpm (Pointer Masking) Cross Test Plan

> This document describes the test plan for cross-scenarios between the Hypervisor (H) extension and the Pointer Masking (Zpm) extension family.
> Zpm basic functionality (ignore transformation in U/S/M-mode, CSR WARL behavior, MPRV/MXR basic interaction, etc.) is already covered by `zpm_test_plan.md`; this document only covers the new cross-behavior introduced by the H extension.

---

## Overview

### Intersection Analysis of H Extension and Zpm Extension Family

Zpm is a family of extensions (`SPEC/zpm.adoc` sec:pm-exts). The intersection with the H extension is analyzed per sub-extension as follows:

#### H × Ssnpm (Core Intersection, Largest Intersection)

Ssnpm definition (`norm:ssnpm_definition`): "A supervisor-level extension that provides pointer masking for the next lower privilege mode (U-mode), **and for VS- and VU-modes if the H extension is present**". When the H extension is implemented, Ssnpm adds the following hardware behaviors:

1. **`henvcfg.PMM` (bits [33:32])**: Controls PM for VS-mode (`norm:henvcfg_pmm_op`). Read-only zero when Ssnpm is not implemented; read-only zero for RV32.
2. **`hstatus.HUPMM` (bits [49:48], RV64)**: When U-mode executes `HLV.*`/`HSV.*` (`hstatus.HU=1`) and the explicit access is performed as VU-mode, controls PM for these instructions (`hypervisor.adoc` sec:hstatus HUPMM). Read-only zero when Ssnpm is not implemented; read-only zero for RV32.
3. **`senvcfg.PMM` control target extended to U/VU** (`norm:senvcfg_pmm_Ssnpm`: "next-lower privilege mode (U/VU)"). `senvcfg` has no corresponding VS CSR (`norm:H_scsrs_nomatch`); when V=1, VS-mode can directly read/write `senvcfg.PMM` to control PM for VU-mode.
4. **PM selection for HLV/HSV instructions** (sec:hstatus HUPMM + `norm:mstatus_mprv_hlsv`): Explicit access of HLV/HSV is always performed as V=1, nominal privilege = `hstatus.SPVP`, with PM settings selected by effective privilege:
   - SPVP=1 (access as VS-mode) → controlled by `henvcfg.PMM` (regardless of HS/M/U execution mode);
   - SPVP=0 (access as VU-mode) and execution mode is HS/M → controlled by `senvcfg.PMM`;
   - SPVP=0 (access as VU-mode) and execution mode is U (HU=1) → controlled by `hstatus.HUPMM`.
5. **Two-stage address translation interaction** (`hypervisor.adoc` [[pm-two-stage]]): GPA is 2 bits wider than VA translation mode (Sv39x4/Sv48x4/Sv57x4). When VS/VU-mode runs with `vsatp.MODE=Bare`, the effective address is GPA, and these extra 2 bits may be masked by PM (depending on `hgatp.MODE` and `senvcfg.PMM`/`henvcfg.PMM`); this issue does not apply when `vsatp.MODE!=Bare`. When `henvcfg.PMM` changes between values where "(XLEN-PMLEN) is less than the GPA width supported by hgatp" (PMLEN=7+sv57x4, PMLEN=16+sv57x4, PMLEN=16+sv48x4), the hypervisor should execute `HFENCE.GVMA(rs1=x0)`.
6. **GPA ignore transformation is zero-extend** (`norm:pm_ignore_pa`): "including guest-physical addresses (i.e., all cases except when the active satp register's MODE field != Bare)". When V=1, the active satp is vsatp, so when `vsatp.MODE=Bare`, the effective address of VS/VU is transformed as a physical address using **zero-extend** (rather than VA's sign-extend) — this is a unique transformation path introduced by the H extension.

#### H × Smnpm (Smaller Intersection)

Smnpm definition (`norm:smnpm_definition`): "provides pointer masking for the next lower privilege mode (**S/HS** if S-mode is implemented, or U-mode otherwise)". `norm:menvcfg_pmm_op` explicitly states that `menvcfg.PMM` controls PM for S-/**HS**-mode. Intersection points:

1. PM for explicit access in HS-mode (the hypervisor itself) is controlled by `menvcfg.PMM`;
2. `menvcfg.PMM` does **not** control VS/VU-mode (VS is controlled by `henvcfg.PMM`, VU by `senvcfg.PMM`); the three-level settings must be verified as mutually independent (`norm:pm_per_mode_control`, `norm:pm_mode_only_dependency`);
3. MPRV interaction (`norm:pm_mprv_spvp` + `norm:mstatus_mprv_hypervisor`): In M-mode with `MPRV=1, MPV=0, MPP=S`, the effective privilege is HS-mode, applying `menvcfg.PMM`.

#### H × Smmpm (Minimal Intersection)

Smmpm definition (`norm:smmpm_definition`): `mseccfg.PMM` only controls PM for M-mode itself. The only intersection point is "negative" verification in virtualization scenarios:

1. In M-mode with `MPRV=1, MPV=1`, the effective privilege is VS/VU (`norm:mstatus_mprv_hypervisor`), at which point `henvcfg.PMM`/`senvcfg.PMM` is applied, **not** `mseccfg.PMM` (`norm:pm_mprv_spvp`);
2. When M-mode executes HLV/HSV, PM is controlled by `senvcfg.PMM`/`henvcfg.PMM` (per SPVP), independent of `mseccfg.PMM` (`norm:mstatus_mprv_hlsv` + sec:hstatus HUPMM).

#### Sspm / Supm (No Intersection with H Extension)

`norm:ssnpm_definition`/`norm:supm_definition` explicitly state that these are **pure profile description extensions**: "describe an execution environment but **have no bearing on hardware implementations**". The specific facilities are defined by their respective execution environments. They do not introduce any CSR fields or hardware behaviors, and therefore have **no testable intersection** with the H extension. This plan does not write test cases for them (consistent with `zpm_test_plan.md`).

### Document Scope

- CSR behavior of `henvcfg.PMM`, `hstatus.HUPMM` (WARL, read-only zero conditions)
- VS-mode PM (controlled by `henvcfg.PMM`, VA sign-extend / GPA zero-extend transformation paths)
- VU-mode PM (controlled by `senvcfg.PMM`, VS-mode direct access to `senvcfg`)
- PM selection logic for HLV/HSV instructions (SPVP × execution mode × HUPMM/senvcfg.PMM/henvcfg.PMM)
- Two-stage address translation and PM interaction (pm-two-stage, GPA extra 2 bits, HFENCE.GVMA synchronization)
- HS-mode PM (`menvcfg.PMM`) independence from VS/VU PM
- Effective privilege PM selection under MPRV+MPV
- Trap-related PM behavior in virtualization scenarios (vstval/stval hardware transformation, trap vector not transformed, MXR suppression)

### Covered by Other Test Plans

- Zpm basic functionality (U/S/M-mode transformation, three CSR PMM basic WARL, MPRV/MXR basic interaction, non-virtualization stval/mtval) → `zpm_test_plan.md`
- H extension basic functionality (CSR, trap, HLV/HSV exception scenarios) → `Hypervisor_test_plan.md`
- Two-stage address translation itself → `Hypervisor_2_stage_test_plan.md`, `Hypervisor_gstage_test_plan.md`

### Out of Scope

- **Sspm / Supm**: Pure profile description extensions, no hardware behavior
- **RV32 scenarios**: PM only applies to RV64 (`norm:pm_rv64_only`), `henvcfg.PMM`/`hstatus.HUPMM` read-only zero for RV32
- **Debug trigger address matching** (`norm:pm_debug_trigger`): Involves Debug extension
- **IOMMU/device access** (device side of `norm:pm_cpu_only`): No device model in current framework

---

## Spec Reference

| Norm ID | Source | English Description |
|---------|--------|---------------------|
| `norm:ssnpm_definition` | `zpm.adoc` | A supervisor-level extension that provides pointer masking for the next lower privilege mode (U-mode), and for VS- and VU-modes if the H extension is present. |
| `norm:smnpm_definition` | `zpm.adoc` | A machine-level extension that provides pointer masking for the next lower privilege mode (S/HS if S-mode is implemented, or U-mode otherwise). |
| `norm:smmpm_definition` | `zpm.adoc` | A machine-level extension that provides pointer masking for M-mode. |
| `norm:sspm_definition` / `norm:supm_definition` | `zpm.adoc` | Extensions that describe an execution environment but have no bearing on hardware implementations. |
| `norm:henvcfg_pmm_op` | `hypervisor.adoc` | If the Ssnpm extension is implemented, the PMM field enables or disables pointer masking for VS-mode. When not implemented, PMM is read-only zero. PMM is read-only zero for RV32. |
| (sec:hstatus HUPMM) | `hypervisor.adoc` | When Ssnpm is implemented, HUPMM enables/disables PM for HLV.\*/HSV.\* in U-mode when their access is performed as though in VU-mode. In HS- and M-modes, PM for these instructions as though in VU-mode is controlled by senvcfg.PMM; as though in VS-mode by henvcfg.PMM. HUPMM is read-only zero without Ssnpm and for RV32. |
| `norm:senvcfg_pmm_Ssnpm` | `supervisor.adoc` | If Ssnpm is implemented, the PMM field enables or disables pointer masking for the next-lower privilege mode (U/VU). If not implemented, PMM is read-only zero. |
| `norm:menvcfg_pmm_op` | `machine.adoc` | If Smnpm is implemented, the PMM field enables or disables pointer masking for the next-lower privilege mode (S-/HS-mode if S-mode is implemented, or U-mode otherwise). |
| `norm:mseccfg_pmm_presence_op` | `machine.adoc` | If Smmpm is implemented, the PMM field enables or disables pointer masking for M-mode. |
| `norm:pm_ignore_va` | `zpm.adoc` | For virtual addresses, the ignore transformation replaces the upper PMLEN bits with the sign extension of the PMLEN+1st bit. |
| `norm:pm_ignore_pa` | `zpm.adoc` | When applied to a physical address, including guest-physical addresses (all cases except when the active satp's MODE != Bare), the ignore transformation replaces the upper PMLEN bits with 0. |
| `norm:pm_apply_explicit` | `zpm.adoc` | When pointer masking is enabled, the ignore transformation will be applied to every explicit memory access. |
| `norm:pm_not_apply_implicit` | `zpm.adoc` | The transformation does not apply to implicit accesses such as page-table walks or instruction fetches. |
| `norm:pm_per_mode_control` | `zpm.adoc` | Pointer masking is controlled separately for different privilege modes. |
| `norm:pm_mode_only_dependency` | `zpm.adoc` | The pointer masking setting that is applied only depends on the active privilege mode, not on the address that is being masked. |
| `norm:pm_mprv_spvp` | `zpm.adoc` | MPRV and SPVP affect pointer masking as well, causing the pointer masking settings of the effective privilege mode to be applied. |
| `norm:pm_mxr_exception` | `zpm.adoc` | When MXR is in effect at the effective privilege mode where explicit memory access is performed, pointer masking does not apply. |
| `norm:pm_csr_hw_apply` | `zpm.adoc` | Pointer masking, when applicable, is applied for hardware writes to a CSR (e.g., the transformed address to stval when taking an exception). |
| `norm:pm_no_trap_vector_mask` | `zpm.adoc` | On trap delivery, pointer masking will not be applied to the address of the trap handler. |
| `norm:pm_no_csr_sw` | `zpm.adoc` | No pointer masking operations are applied when software reads/writes to CSRs. |
| `norm:pmlen_supported_values` | `zpm.adoc` | The current standard only supports PMLEN=XLEN-48 and PMLEN=XLEN-57. |
| `norm:pmlen_illegal_warl` | `zpm.adoc` | Trying to enable pointer masking in an unsupported scenario represents an illegal write and follows WARL semantics. |
| `norm:H_scsrs_nomatch` | `hypervisor.adoc` | Some standard supervisor CSRs (senvcfg, ...) have no matching VS CSR. These CSRs continue to have their usual function and accessibility even when V=1, with VS/VU-mode substituting for HS/U-mode. |
| `norm:mstatus_mprv_hlsv` | `hypervisor.adoc` | MPRV does not affect HLV/HLVX/HSV. Their explicit loads/stores always act as though V=1 and the nominal privilege mode were hstatus.SPVP, overriding MPRV. |
| `norm:mstatus_mprv_hypervisor` | `hypervisor.adoc` | When MPRV=1, explicit memory accesses are translated and protected as though the current virtualization mode were set to MPV and the nominal privilege mode were set to MPP. |
| ([[pm-two-stage]]) | `hypervisor.adoc` | GPAs are 2-bit wider than the corresponding VA translation modes. With vsatp[mode]=Bare in VS/VU mode, those 2 bits may be subject to pointer masking depending on hgatp[mode] and senvcfg/henvcfg[pmm]. If vsatp[mode]!=Bare, this issue does not apply. Hypervisors should execute HFENCE.GVMA(rs1=x0) when henvcfg.PMM changes from/to a value where (XLEN-PMLEN) < GPA width of hgatp.MODE. |

---

## Test Groups

> [!IMPORTANT]
> There are 8 test groups in total. VS/VU-mode tests use `run_in_vs_mode()`/`run_in_vu_mode()` from `common/hyp/hyp_priv.h`; two-stage scenarios use `two_stage_run_in_vs()` from `common/hyp/two_stage.c`; HLV/HSV uses `hlv_d()`/`hsv_d()` from `common/hyp/hyp_ldst.h`. PM configuration and tagged address construction reuse `common/pm/pm_cfg.h` and `common/pm/pm_addr.h`.
>
> Before executing all test cases, extension implementation must be probed (`detect_ssnpm()`/`detect_smnpm()`/`detect_smmpm()` + H extension availability). If not implemented, TEST_SKIP; do not lower SPEC requirements to pass tests.

---

## Group 1: Capability Detection and H-Level PM CSR Field Control

**Spec Reference**:
- `norm:henvcfg_pmm_op`: `henvcfg.PMM` controls VS-mode PM; read-only zero when Ssnpm not implemented
- sec:hstatus HUPMM: `hstatus.HUPMM` controls PM for HLV/HSV in U-mode; read-only zero when Ssnpm not implemented
- `norm:pmlen_supported_values` / `norm:pmlen_illegal_warl`: Legal PMM values are 00/10/11, 01 reserved, WARL semantics
- `norm:H_virtinst_vu_vs_nonhigh_allowedhs_tvm0`: V=1 access to hypervisor CSRs raises virtual-instruction exception

**Test Scope**: Verify in HS-mode the writability, WARL constraints, and field isolation of `henvcfg.PMM` (bits [33:32]) and `hstatus.HUPMM` (bits [49:48]), and that guest (VS-mode) cannot modify its own VS-level PM settings.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HZPM-CAP-01 | Probe henvcfg.PMM implementation | HS-mode writes henvcfg.PMM=0b11, reads back | Non-zero readback when Ssnpm implemented; read-only zero when not implemented (record for subsequent SKIP) |
| HZPM-CAP-02 | Probe hstatus.HUPMM implementation | HS-mode writes hstatus.HUPMM=0b11, reads back | Non-zero readback when Ssnpm implemented; read-only zero when not implemented |
| HZPM-CAP-03 | henvcfg.PMM supported PMLEN | Write 0b10/0b11 to henvcfg.PMM respectively, read back | Record PMLEN=7/16 support, at least one must be supported |
| HZPM-CAP-04 | hstatus.HUPMM supported PMLEN | Write 0b10/0b11 to hstatus.HUPMM respectively, read back | Record PMLEN=7/16 support |
| HZPM-CAP-05 | henvcfg.PMM reserved encoding | Write PMM=0b01 (reserved), read back | PMM retains original value (WARL ignores illegal write) |
| HZPM-CAP-06 | hstatus.HUPMM reserved encoding | Write HUPMM=0b01, read back | HUPMM retains original value |
| HZPM-CAP-07 | henvcfg.PMM field isolation | Read FIOM/CBIE/CBCFE/CBZE/PBMTE/ADUE/STCE before and after toggling PMM | Other field values unchanged |
| HZPM-CAP-08 | hstatus.HUPMM field isolation | Read VSBE/GVA/SPV/SPVP/HU/VGEIN/VTVM/VTW/VTSR/VSXL before and after toggling HUPMM | Other field values unchanged |
| HZPM-CAP-09 | Read-only zero when Ssnpm not implemented | If HZPM-CAP-01 probes as not implemented, write non-zero to henvcfg.PMM and hstatus.HUPMM | Both fields read-only zero |
| HZPM-CAP-10 | VS-mode cannot write henvcfg.PMM | VS-mode executes `csrrw`/`csrrs` to access henvcfg | virtual-instruction exception (cause=22), guest cannot modify its own VS-level PM settings |

> [!NOTE]
> HZPM-CAP-10 is a security-critical test case: VS-level PM configuration authority must remain at HS-level (`norm:pm_config_next_higher`). If guest can modify `henvcfg.PMM`, it can bypass the hypervisor's PM policy.

---

## Group 2: H × Ssnpm — VS-mode PM (henvcfg.PMM)

**Spec Reference**:
- `norm:henvcfg_pmm_op`: `henvcfg.PMM` controls VS-mode PM
- `norm:pm_ignore_va`: VA sign-extend transformation when vsatp.MODE!=Bare
- `norm:pm_ignore_pa`: Effective address is GPA when vsatp.MODE=Bare, zero-extend transformation
- `norm:pm_apply_explicit` / `norm:pm_mode_only_dependency` / `norm:pm_per_mode_control`

**Test Scope**: Verify the correctness of ignore transformation for tagged load/store/AMO in VS-mode. Two paths: (a) `vsatp=Sv39` (VA → sign-extend); (b) `vsatp=Bare` + `hgatp=Sv39x4` (GPA → zero-extend). PM configuration can only be done by HS-mode via `henvcfg.PMM`.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HZPM-VS-01 | PMLEN7 VS-mode tagged load (VA path) | vsatp=Sv39, henvcfg.PMM=PMLEN7, VS-mode load tagged VA (tag=all 1s) | load succeeds, value matches untagged address (sign-extend transformation) |
| HZPM-VS-02 | PMLEN16 VS-mode tagged load (VA path) | Same as above, PMM=PMLEN16 | load succeeds, value correct |
| HZPM-VS-03 | PMLEN7 VS-mode tagged store | vsatp=Sv39, PMM=PMLEN7, VS-mode store via tagged VA | store succeeds, untagged readback value correct |
| HZPM-VS-04 | PMLEN7 VS-mode AMO | vsatp=Sv39, PMM=PMLEN7, VS-mode amoadd.d via tagged VA | Old value correct, memory update correct |
| HZPM-VS-05 | Different tags accessing same location | Same base with tag=0x55 and tag=0x7F embedded, VS-mode load respectively | Both loads read the same value |
| HZPM-VS-06 | Tagged VA when PM disabled | henvcfg.PMM=DISABLED, VS-mode load tagged VA | page-fault (tagged VA has no valid mapping) |
| HZPM-VS-07 | sign-extend correctness | PMLEN7, base with bit 56=1, embed tag then VS-mode load | After transformation bits[63:57] all 1s, accesses correct address |
| HZPM-VS-08 | VS PM independent of HS PM | menvcfg.PMM=DISABLED (HS PM off), henvcfg.PMM=PMLEN7 | VS-mode tagged load still succeeds (VS PM depends only on henvcfg.PMM) |
| HZPM-VS-09 | GPA path zero-extend transformation | vsatp=Bare + hgatp=Sv39x4, henvcfg.PMM=PMLEN7, VS-mode load tagged GPA | zero-extend per PA rules (not sign-extend), accesses correct SPA |
| HZPM-VS-10 | GPA path PM disabled | vsatp=Bare + hgatp=Sv39x4, henvcfg.PMM=DISABLED, VS-mode load tagged GPA | guest-page-fault (cause 20/21/23, tagged GPA exceeds hgatp mapping) |

> [!IMPORTANT]
> HZPM-VS-09/10 are transformation paths unique to the H extension: when `vsatp.MODE=Bare`, VS-mode effective address is GPA, which must be **zero-extended** per `norm:pm_ignore_pa` (different from VA's sign-extend semantics). If the implementation incorrectly sign-extends GPA, when tag high bits are 1, it will access the wrong physical address. When constructing test cases, ensure the zero-extended GPA falls within the hgatp mapping range.

---

## Group 3: H × Ssnpm — VU-mode PM (senvcfg.PMM)

**Spec Reference**:
- `norm:senvcfg_pmm_Ssnpm`: `senvcfg.PMM` controls U/VU-mode PM
- `norm:H_scsrs_nomatch`: `senvcfg` has no VS CSR, VS-mode can directly access when V=1, function substitutes VS/VU for HS/U
- `norm:pm_ignore_va` / `norm:pm_ignore_pa` / `norm:pm_per_mode_control`

**Test Scope**: Verify that VU-mode PM is controlled by `senvcfg.PMM`, and that VS-mode (guest OS) can directly write `senvcfg.PMM` to manage PM for its VU user-space; verify VU and VS PM settings are mutually independent.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HZPM-VU-01 | PMLEN7 VU-mode tagged load | vsatp=Sv39 (PTE_U), senvcfg.PMM=PMLEN7, VU-mode load tagged VA | load succeeds, value correct |
| HZPM-VU-02 | PMLEN16 VU-mode tagged load | Same as above, PMM=PMLEN16 | load succeeds, value correct |
| HZPM-VU-03 | PMLEN7 VU-mode tagged store | senvcfg.PMM=PMLEN7, VU-mode store via tagged VA | store succeeds, value correct |
| HZPM-VU-04 | Tagged VA when VU PM disabled | senvcfg.PMM=DISABLED, VU-mode load tagged VA | page-fault |
| HZPM-VU-05 | VS-mode directly writes senvcfg.PMM | VS-mode writes senvcfg.PMM=PMLEN7 (without HS), then enters VU-mode tagged load | VS-mode write succeeds (no virtual-instruction), VU tagged load succeeds |
| HZPM-VU-06 | VU PM independent of VS PM | henvcfg.PMM=DISABLED (VS PM off), senvcfg.PMM=PMLEN7 (VU PM on) | VU tagged load succeeds; under same config VS tagged load faults |
| HZPM-VU-07 | VU-mode GPA path zero-extend | vsatp=Bare + hgatp=Sv39x4, senvcfg.PMM=PMLEN7, VU-mode load tagged GPA | zero-extend transformation, accesses correct SPA |

> [!NOTE]
> HZPM-VU-05 verifies the application of `norm:H_scsrs_nomatch` in PM scenarios: `senvcfg` retains its original accessibility when V=1, allowing guest OS (VS-mode) to directly manage PM settings for its user-space (VU) without hypervisor intervention. This is also a prerequisite for Linux guests to self-enable HWASAN-like mechanisms.

---

## Group 4: H × Ssnpm — PM Selection for HLV/HSV Instructions

**Spec Reference**:
- sec:hstatus HUPMM: PM for HLV/HSV selected by effective privilege — access as VS uses `henvcfg.PMM`; HS/M-mode access as VU uses `senvcfg.PMM`; U-mode access as VU uses `hstatus.HUPMM`
- `norm:mstatus_mprv_hlsv`: HLV/HSV always execute as V=1, privilege=SPVP, MPRV does not affect
- `norm:pm_mprv_spvp`: SPVP affects PM, applying effective privilege mode's PM settings

**Test Scope**: Verify the PM selection logic for HLV.D/HSV.D under three execution modes (HS/M/U) × two SPVP values. This is the most error-prone part of the H × Zpm intersection (three different CSR fields selected by scenario).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HZPM-HLV-01 | HS-mode SPVP=1 uses henvcfg.PMM | hstatus.SPVP=1, henvcfg.PMM=PMLEN7, HS-mode executes hlv_d(tagged_gva) | Transformed per VS PM, reads correct guest data |
| HZPM-HLV-02 | HS-mode SPVP=1 PM disabled | SPVP=1, henvcfg.PMM=DISABLED, hlv_d(tagged_gva) | fault (tagged address not transformed, no mapping) |
| HZPM-HLV-03 | HS-mode SPVP=0 uses senvcfg.PMM | SPVP=0, senvcfg.PMM=PMLEN7, HS-mode hlv_d(tagged_gva) | Transformed per VU PM, reads correct data |
| HZPM-HLV-04 | HS-mode SPVP=0 HUPMM ineffective | SPVP=0, senvcfg.PMM=DISABLED, hstatus.HUPMM=PMLEN7, HS-mode hlv_d(tagged_gva) | fault — HUPMM does not take effect in HS-mode, only senvcfg.PMM controls |
| HZPM-HLV-05 | M-mode SPVP=0 uses senvcfg.PMM | M-mode, SPVP=0, senvcfg.PMM=PMLEN7, hlv_d(tagged_gva) | Succeeds (M-mode also uses senvcfg.PMM, not mseccfg.PMM) |
| HZPM-HLV-06 | M-mode SPVP=1 uses henvcfg.PMM | M-mode, SPVP=1, henvcfg.PMM=PMLEN7, hlv_d(tagged_gva) | Succeeds |
| HZPM-HLV-07 | U-mode SPVP=0 uses HUPMM | hstatus.HU=1, SPVP=0, hstatus.HUPMM=PMLEN7, U-mode hlv_d(tagged_gva) | Succeeds (HUPMM takes effect in U-mode) |
| HZPM-HLV-08 | U-mode SPVP=0 senvcfg.PMM ineffective | HU=1, SPVP=0, HUPMM=DISABLED, senvcfg.PMM=PMLEN7, U-mode hlv_d(tagged_gva) | fault — only HUPMM controls HLV/HSV PM in U-mode |
| HZPM-HLV-09 | HSV.D tagged store | SPVP=1, henvcfg.PMM=PMLEN7, HS-mode hsv_d(tagged_gva, val) | store succeeds, guest-side untagged readback correct |
| HZPM-HLV-10 | MPRV does not change HLV PM selection | M-mode, MPRV=1 and MPP=M, SPVP=1, henvcfg.PMM=PMLEN7, hlv_d(tagged_gva) | Still applies henvcfg.PMM per SPVP=1 (VS), succeeds |

> [!WARNING]
> HZPM-HLV-04 and HZPM-HLV-08 verify the asymmetric rule of the SPEC: `hstatus.HUPMM` takes effect **only** when U-mode executes HLV/HSV; in HS/M-mode, access as VU uses `senvcfg.PMM`. The SPEC notes require the hypervisor to copy the guest-written `senvcfg.PMM` value to `hstatus.HUPMM` before U-mode calls HLV/HSV. If the implementation confuses the selection conditions for these two fields, PM will be incorrectly enabled or disabled.

---

## Group 5: H × Ssnpm — Two-stage Address Translation and PM (pm-two-stage)

**Spec Reference**:
- [[pm-two-stage]] (hypervisor.adoc:2127-2161): GPA is 2 bits wider than VA mode; when vsatp=Bare the extra 2 bits may be masked by PM; when `henvcfg.PMM` changes across "(XLEN-PMLEN) < GPA width" boundary, `HFENCE.GVMA(rs1=x0)` is required; address-specific HFENCE.GVMA should ignore address parameters or ignore masked GPA high bits
- `norm:pm_ignore_pa`: GPA zero-extend transformation
- `norm:pm_not_apply_implicit`: Page table walks (including G-stage walk) do not apply PM

**Test Scope**: Verify the interaction between PM and GPA extra 2 bits under two-stage translation (hgatp + vsatp), and the TLB synchronization requirement after PMM changes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HZPM-2STG-01 | GPA extra 2 bits masked by PM | hgatp=Sv39x4, vsatp=Bare, henvcfg.PMM=PMLEN7, VS-mode accesses address with tag covering GPA bits[40:39] | After zero-extend, accesses correct SPA (extra 2 bits do not affect addressing) |
| HZPM-2STG-02 | Extra 2 bits issue does not apply when vsatp!=Bare | hgatp=Sv39x4, vsatp=Sv39, henvcfg.PMM=PMLEN7, VS-mode tagged VA load | VA sign-extend transformation then two-stage translation accesses correct SPA |
| HZPM-2STG-03 | PMLEN=16 + hgatp=sv48x4 change synchronization | hgatp=Sv48x4 (if supported), henvcfg.PMM changes from 0 to PMLEN16, execute HFENCE.GVMA(x0,x0) then VS-mode tagged GPA access | After fence, transforms correctly per new PMLEN, no stale TLB entry causing errors |
| HZPM-2STG-04 | PMLEN=7 + hgatp=sv57x4 change synchronization | hgatp=Sv57x4 (if supported), PMM changes from PMLEN16 to PMLEN7, HFENCE.GVMA(x0,x0) then access | After fence, behavior per new PMLEN |
| HZPM-2STG-05 | G-stage page table walk not affected by PM | hgatp next-level page table pointer GPA high bits overlap with PM mask bits, henvcfg.PMM=PMLEN7 | G-stage walk uses original PTE address (not transformed), translation normal |

> [!NOTE]
> - HZPM-2STG-03/04 depend on platform-supported hgatp modes (sv48x4/sv57x4), TEST_SKIP when not supported. SPEC requires synchronization for: PMLEN=7+sv57x4, PMLEN=16+sv57x4, PMLEN=16+sv48x4.
> - HZPM-2STG-05 is a negative security test case: if PM is incorrectly applied to G-stage page table walks (implicit access), page table pointers with overlapping tag bits will be incorrectly transformed, causing translation to the wrong page table.

---

## Group 6: H × Smnpm — HS-mode PM (menvcfg.PMM) and Independence

**Spec Reference**:
- `norm:menvcfg_pmm_op` / `norm:smnpm_definition`: `menvcfg.PMM` controls S/HS-mode PM
- `norm:pm_per_mode_control` / `norm:pm_mode_only_dependency`: PM for each privilege mode is independent, depends only on the active mode

**Test Scope**: Verify that HS-mode (hypervisor itself) PM is controlled by `menvcfg.PMM`, and is completely independent from VS/VU PM settings (`henvcfg.PMM`/`senvcfg.PMM`).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HZPM-HS-01 | PMLEN7 HS-mode tagged load (VA) | satp=Sv39, menvcfg.PMM=PMLEN7, HS-mode load tagged VA | sign-extend transformation, load succeeds |
| HZPM-HS-02 | HS-mode Bare tagged load (PA) | satp=Bare, menvcfg.PMM=PMLEN7, HS-mode load tagged PA | zero-extend transformation, load succeeds |
| HZPM-HS-03 | HS PM independent of VS PM | menvcfg.PMM=DISABLED, henvcfg.PMM=PMLEN7 | HS-mode tagged load faults; after entering VS-mode tagged load succeeds |
| HZPM-HS-04 | menvcfg.PMM does not affect VS | menvcfg.PMM=PMLEN7, henvcfg.PMM=DISABLED | VS-mode tagged load faults (menvcfg.PMM does not apply to VS) |
| HZPM-HS-05 | menvcfg.PMM does not affect VU | menvcfg.PMM=PMLEN7, senvcfg.PMM=DISABLED | VU-mode tagged load faults (menvcfg.PMM does not apply to VU) |
| HZPM-HS-06 | PMLEN7 HS-mode AMO | satp=Sv39, menvcfg.PMM=PMLEN7, HS-mode amoadd.d via tagged VA | Old value correct, memory update correct |

---

## Group 7: H × Smmpm — Effective Privilege PM Selection under MPRV+MPV

**Spec Reference**:
- `norm:pm_mprv_spvp`: MPRV/SPVP cause the effective privilege mode's PM settings to be applied
- `norm:mstatus_mprv_hypervisor`: When MPRV=1, accesses are translated/protected as V=MPV, privilege=MPP
- `norm:mseccfg_pmm_presence_op` / `norm:smmpm_definition`: `mseccfg.PMM` only controls M-mode

**Test Scope**: Verify that when M-mode sets `mstatus.MPRV=1`, load/store applies the PM settings of the effective privilege determined by MPV/MPP, rather than `mseccfg.PMM`. This is the only intersection path between Smmpm and the H extension.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HZPM-MPRV-01 | MPRV=1 MPV=1 MPP=S uses henvcfg.PMM | mseccfg.PMM=DISABLED, henvcfg.PMM=PMLEN7, M-mode MPRV=1/MPV=1/MPP=S tagged load | Applies henvcfg.PMM per effective VS-mode, load succeeds |
| HZPM-MPRV-02 | MPRV=1 MPV=1 MPP=U uses senvcfg.PMM | mseccfg.PMM=DISABLED, senvcfg.PMM=PMLEN7, MPRV=1/MPV=1/MPP=U tagged load | Applies senvcfg.PMM per effective VU-mode, load succeeds |
| HZPM-MPRV-03 | mseccfg.PMM does not apply to effective VS | mseccfg.PMM=PMLEN7, henvcfg.PMM=DISABLED, MPRV=1/MPV=1/MPP=S tagged load | fault — M-mode's own PM not applied to effective VS access |
| HZPM-MPRV-04 | MPRV=1 MPV=0 MPP=S uses menvcfg.PMM | mseccfg.PMM=DISABLED, menvcfg.PMM=PMLEN7, MPRV=1/MPV=0/MPP=S tagged load | Applies menvcfg.PMM per effective HS-mode, load succeeds |
| HZPM-MPRV-05 | MPRV=0 baseline | MPRV=0, mseccfg.PMM=PMLEN7, M-mode tagged load | Applies M-mode's own PM (consistent with zpm_test_plan Group 8) |

> [!WARNING]
> MPRV tests directly manipulate mstatus in M-mode. After modifying MPRV/MPV/MPP, the tagged load must be executed immediately and restored immediately. If an exception is triggered during MPRV=1, the trap handler is also affected by MPRV, which may cause unpredictable behavior.

---

## Group 8: Trap and PM Interaction in Virtualization Scenarios

**Spec Reference**:
- `norm:pm_csr_hw_apply`: Hardware CSR writes (vstval/stval on exception) apply PM transformation
- `norm:pm_no_trap_vector_mask`: Trap handler address (vstvec/stvec) does not apply PM on trap delivery
- `norm:pm_no_csr_sw`: Software CSR reads/writes do not apply PM
- `norm:pm_not_apply_implicit`: Instruction fetch does not apply PM
- `norm:pm_mxr_exception`: PM does not apply when MXR is in effect at effective privilege mode

**Test Scope**: Verify hardware transformation of exception addresses, trap vector not transformed, and VS-mode MXR suppression of PM in V=1 scenarios.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|-------------------|-----------------|
| HZPM-TRAP-01 | vstval written with transformed address | medeleg+hedeleg delegate load page-fault to VS, henvcfg.PMM=PMLEN7, VS-mode tagged VA triggers fault | vstval == PM-transformed (sign-extended) address |
| HZPM-TRAP-02 | stval written with transformed GVA | hgatp does not map tagged GPA region, vsatp=Bare, henvcfg.PMM=PMLEN7, VS-mode tagged GPA load triggers guest-page-fault to HS | stval == zero-extend transformed GVA, hstatus.GVA=1 |
| HZPM-TRAP-03 | Trap delivery not affected by PM | henvcfg.PMM=PMLEN7, vstvec is normal handler address, VS-mode triggers ecall | Trap normally delivered to vstvec original address, handler executes correctly |
| HZPM-TRAP-04 | Tagged vstvec not transformed | Software writes tagged handler address to vstvec (`norm:pm_no_csr_sw` allows), VS-mode triggers trap | Delivered to original tagged address, instruction fetch not transformed → instruction page-fault (proves PM not applied to trap vector) |
| HZPM-TRAP-05 | VS-mode MXR suppresses PM | henvcfg.PMM=PMLEN7, vsstatus.MXR=1, VS-mode tagged load | PM not applied, tagged address accessed directly → fault |
| HZPM-TRAP-06 | VS-mode instruction fetch not affected by PM | henvcfg.PMM=PMLEN7, VS-mode jumps to tagged function address | Instruction fetch not transformed → instruction page-fault |

> [!IMPORTANT]
> HZPM-TRAP-01/02 verify the difference of `norm:pm_csr_hw_apply` on two trap paths in virtualization: exceptions delegated to VS write `vstval` (VA, sign-extend); guest-page-faults entering HS write `stval` (GVA, zero-extend). The expected transformation differs between the two registers; when constructing expected values, use `pm_transform_va()` and `pm_transform_pa()` respectively.

---

## Test Case Summary

| Group | Test ID Range | Count | Covered Intersection |
|-------|--------------|-------|---------------------|
| 1 | HZPM-CAP-01~10 | 10 | Shared (CSR basics) |
| 2 | HZPM-VS-01~10 | 10 | H × Ssnpm (VS-mode) |
| 3 | HZPM-VU-01~07 | 7 | H × Ssnpm (VU-mode) |
| 4 | HZPM-HLV-01~10 | 10 | H × Ssnpm (HLV/HSV) |
| 5 | HZPM-2STG-01~05 | 5 | H × Ssnpm (two-stage) |
| 6 | HZPM-HS-01~06 | 6 | H × Smnpm (HS-mode) |
| 7 | HZPM-MPRV-01~05 | 5 | H × Smmpm (MPRV+MPV) |
| 8 | HZPM-TRAP-01~06 | 6 | Shared (trap interaction) |
| **Total** | | **59** | |

---

## Framework Dependencies

| Component | File | Purpose |
|-----------|------|---------|
| PM Control API | `common/pm/pm_cfg.h` + `pm_cfg.c` | `pm_set_umode()`/`detect_ssnpm()` etc. (senvcfg/menvcfg/mseccfg) |
| Tagged Address Utilities | `common/pm/pm_addr.h` | `pm_tag_address()`/`pm_transform_va()`/`pm_transform_pa()` |
| VS/VU-mode Execution | `common/hyp/hyp_priv.h` + `hyp_priv.c` | `run_in_vs_mode()`/`run_in_vu_mode()` |
| Two-stage Translation | `common/hyp/two_stage.c` + `two_stage_helpers.h` | `two_stage_run_in_vs()`, hgatp/vsatp configuration |
| HLV/HSV Instructions | `common/hyp/hyp_ldst.h` + `hyp_ldst.c` | `hlv_d()`/`hsv_d()` etc. |
| VS Trap Handling | `common/hyp/hyp_vs_trap.h` | VS-mode trap expectation and capture |
| Page Table Construction | `common/vm/satp.c` | `pt_init()`/`pt_setup_identity_mapping()` |
| CSR Definitions | `common/encoding.h` | `HENVCFG_PMM_MASK`/`HSTATUS_HUPMM_MASK`/`PMM_PMLEN7` etc. (HUPMM-related definitions may need to be added) |

> [!NOTE]
> During implementation, check whether `common/encoding.h` already defines masks/offset macros for `hstatus.HUPMM` (bits [49:48]) and `henvcfg.PMM` (bits [33:32]); if missing, add them following existing naming conventions. `pm_cfg.h` currently only wraps senvcfg/menvcfg/mseccfg three-level APIs; henvcfg.PMM and hstatus.HUPMM read/write can initially use `read_csr`/`write_csr` directly, or extend `pm_cfg` style with `pm_set_vsmode()`/`pm_set_hupmm()` helper functions.
