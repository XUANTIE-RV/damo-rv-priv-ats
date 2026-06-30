**[中文](../testplan/Shvsatpa_test_plan.md) | English**

# Shvsatpa Extension Test Plan

This document describes the test plan for the Shvsatpa (Translation Mode Support for vsatp, Version 1.0) extension. Shvsatpa requires that all translation modes supported by `satp` must also be supported by `vsatp`. That is, any legal value that `satp.MODE` can hold, `vsatp.MODE` must also be able to hold.

---

## Test Scope

### Specification Sources

- `SPEC/shvsatpa.adoc` — Shvsatpa Extension for Translation Mode Support, Version 1.0
- `SPEC/hypervisor.adoc` lines 1382–1425 (`vsatp` register definition) — Provides `vsatp` field layout, substitution behavior for `satp` when V=1, and handling of unsupported MODE value writes
- `SPEC/supervisor.adoc` lines 997–1065 (`satp` register and MODE field encoding) — Provides MODE encoding table and WARL write semantics

### Key Reference Files

| Path | Description |
|------|-------------|
| `SPEC/shvsatpa.adoc` | Full Shvsatpa specification (7 lines total) |
| `SPEC/hypervisor.adoc:1382-1425` | `vsatp` register specification: VSXLEN-bit RW, substitutes for `satp` when V=1, MODE write handling |
| `SPEC/supervisor.adoc:997-1065` | `satp` register specification: MODE encoding table (Bare=0, Sv39=8, Sv48=9, Sv57=10), WARL behavior |
| `common/encoding.h` | `CSR_VSATP = 0x280`, `CSR_SATP = 0x180` |
| `common/hyp/hyp_priv.h:21` | `run_in_vs_mode(fn, arg)` — Execute test function in VS-mode (V=1) |
| `common/hyp/hyp_csr.h` | Hypervisor CSR read/write helper functions |
| `common/hyp/hyp_test.h` | Hypervisor test macros (`HYP_TEST_END`, etc.) |
| `common/hyp/hyp_reset.c` | `hyp_reset_state()` resets hypervisor CSR state |
| `DOCS/framework/hypervisor_framework.md` | Hypervisor test framework overview |

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:shvsatpa_satp_vsatp_modes` | If the Shvsatpa extension is implemented, all translation modes supported in `satp` must be supported in `vsatp`. |
| `norm:vsatp_sz_acc_op` | The `vsatp` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `satp`. When V=1, `vsatp` substitutes for the usual `satp`. `vsatp` controls VS-stage address translation, the first stage of two-stage translation for guest virtual addresses. |
| `norm:vsatp_mode_unsupported_v0` | When V=0, a write to `vsatp` with an unsupported MODE value is either ignored as it is for `satp`, or the fields of `vsatp` are treated as WARL in the normal way. |
| `norm:vsatp_mode_unsupported_v1` | However, when V=1, a write to `satp` with an unsupported MODE value is ignored and no write to `vsatp` is effected. |
| `norm:vsatp_v0` | When V=0, `vsatp` does not directly affect the behavior of the machine, unless a virtual-machine load/store (HLV, HLVX, or HSV) or the MPRV feature in the `mstatus` register is used to execute a load or store as though V=1. |
| `norm:satp_mode` | When MODE=Bare, supervisor virtual addresses are equal to supervisor physical addresses, and there is no additional memory protection. To select MODE=Bare, software must write zero to the remaining fields of `satp`. Attempting to select MODE=Bare with a nonzero pattern in the remaining fields has an UNSPECIFIED effect. |
| `norm:satp_mode_sxlen64` | When SXLEN=64, three paged virtual-memory schemes are defined: Sv39, Sv48, and Sv57. One additional scheme, Sv64, will be defined in a later version. The remaining MODE settings are reserved for future use. |
| `norm:satp_mode_op_unsupported` | If a write to `satp` specifies a MODE value that is not supported, the entire write has no effect; no fields in `satp` are modified. |

> [!IMPORTANT]
> The core constraint of Shvsatpa is singular: whatever MODE `satp` supports, `vsatp` must also support. This plan confirms consistency by "first probing the set of MODEs supported by satp, then verifying vsatp supports the same set." Group 4 verifies V=1 passthrough semantics (VS-mode operates actual vsatp via satp instruction names), and Group 5 verifies that handling of unsupported MODE values conforms to the specification under both V=0 and V=1.

### Out of Scope

- **Address translation correctness of satp/vsatp**: This plan only verifies whether the MODE field can be held, not translation table lookup behavior
- **M-mode satp behavior**: Shvsatpa only constrains mode consistency between satp and vsatp
- **Comprehensive ASID / PPN field width testing**: Only basic write-readback verification is performed, no exhaustive enumeration
- **Multi-hart scenarios**: Project uses single-core test environment
- **SXLEN=32 scenarios**: Only SXLEN=64 (RV64 environment) is covered
- **G-stage address translation verification**: This test does not involve G-stage correctness of two-stage translation
- **SFENCE.VMA / HFENCE.VVMA behavior**: TLB management is out of scope for this plan

---

## Design Key Points

### 1. MODE Probing Method

The MODE field of `satp`/`vsatp` is WARL: writing an unsupported value causes the entire write to be ignored. Therefore, the probing strategy is:

1. Backup current CSR value
2. Write target MODE (write MODE to target value, ASID=0, PPN=0)
3. Read back CSR
4. If readback MODE == written MODE → that MODE is supported
5. If readback MODE != written MODE → that MODE is not supported (write was ignored)
6. Restore CSR

> [!NOTE]
> Probing Bare (MODE=0) requires special handling: the specification requires that when selecting Bare, all other fields must be zero (`SPEC/supervisor.adoc:1014-1016`), so all zeros are written when probing Bare.

### 2. Access Paths for satp and vsatp

| CSR | Address | Direct access when V=0 | Access via satp instruction when V=1 |
|-----|---------|------------------------|--------------------------------------|
| `satp` | 0x180 | HS-mode `csrr/csrw satp` | N/A (when V=1, satp instruction operates vsatp) |
| `vsatp` | 0x280 | HS/M-mode `csrr/csrw 0x280` | VS-mode `csrr/csrw satp` actually operates vsatp |

- **Group 1 & 2 & 5**: M/HS-mode directly operates CSR 0x180 (satp) and CSR 0x280 (vsatp)
- **Group 3 & 4**: VS-mode (V=1) indirectly operates vsatp via `satp` instruction name

### 3. MODE Encoding (SXLEN=64)

| Value | Name | Description |
|-------|------|-------------|
| 0 | Bare | No translation or protection |
| 1-7 | — | Reserved (standard use) |
| 8 | Sv39 | 39-bit paged virtual address |
| 9 | Sv48 | 48-bit paged virtual address |
| 10 | Sv57 | 57-bit paged virtual address |
| 11 | Sv64 | 64-bit (future specification, currently reserved) |
| 12-15 | — | Reserved |

### 4. Isolation from hyp_reset_state

`common/hyp/hyp_reset.c::hyp_reset_state()` resets hypervisor CSR state. Each test case in this plan backs up the target CSR before operation and restores it after completion. Operations within VS-mode return to M-mode via the ecall return mechanism of `run_in_vs_mode` before assertions and cleanup are performed.

### 5. Passing Probe Results

After Group 1 probes the set of MODEs supported by `satp`, the results are stored in global variables (e.g., `g_satp_supported_modes[]`) for use by Group 2/3/4/5. The support status of each MODE is recorded as a bool array or bitmask.

---

## Test Groups

> [!IMPORTANT]
> There are 6 test groups and 19 test cases in total. Group 1 probes the set of modes supported by satp in M/HS-mode; Group 2 directly verifies in M/HS-mode that vsatp supports the same mode set; Group 3 verifies vsatp mode support in VS-mode (V=1) via satp instructions; Group 4 verifies V=1 passthrough semantics; Group 5 verifies write handling for unsupported MODE values; Group 6 verifies ASID field width consistency. Each group provides: specification basis, test responsibilities, test case table (ID/name/description/expected result); each group provides 1 key C code example.

---

### Group 1: `satp` MODE Support Probing (Baseline Establishment)

**Specification Basis**:
- `norm:satp_mode` (`SPEC/supervisor.adoc:1009-1018`): MODE field encoding definition
- `norm:satp_mode_sxlen64` (`SPEC/supervisor.adoc:1044-1050`): Defines Sv39/Sv48/Sv57 when SXLEN=64
- `norm:satp_mode_op_unsupported` (`SPEC/supervisor.adoc:1052-1055`): Writing unsupported MODE renders entire write ineffective

**Test Responsibilities**: Probe the set of translation modes supported by `satp` in M/HS-mode to establish a comparison baseline for Group 2/3/4/5. Perform write-readback tests for each standard MODE value (Bare=0, Sv39=8, Sv48=9, Sv57=10).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSATP-PROBE-01 | satp MODE=Bare probe | Write `satp = 0` (MODE=0, ASID=0, PPN=0), read back MODE field | Readback MODE=0 (Bare is typically supported) |
| VSATP-PROBE-02 | satp MODE=Sv39 probe | Write `satp = 8 << 60` (MODE=8), read back MODE field | Readback MODE ∈ {0, 8}; record support status |
| VSATP-PROBE-03 | satp MODE=Sv48 probe | Write `satp = 9 << 60` (MODE=9), read back MODE field | Readback MODE ∈ {previous valid value, 9}; record support status |
| VSATP-PROBE-04 | satp MODE=Sv57 probe | Write `satp = 10 << 60` (MODE=10), read back MODE field | Readback MODE ∈ {previous valid value, 10}; record support status |

> [!NOTE]
> Probing logic: Set `satp` to a known state (Bare=0) before writing, then write the target MODE. If readback MODE == target value, it is supported; if readback MODE == previous value (0), it is not supported. At least one non-Bare mode should be supported (otherwise the H extension can hardly run guest OS).

---

### Group 2: Consistency Verification of `vsatp` MODE Against `satp` Supported Modes (V=0 Direct Access)

**Specification Basis**:
- `norm:shvsatpa_satp_vsatp_modes` (`SPEC/shvsatpa.adoc:4-6`): All translation modes supported by `satp` must also be supported by `vsatp`
- `norm:vsatp_sz_acc_op` (`SPEC/hypervisor.adoc:1384-1392`): `vsatp` is a VSXLEN-bit RW register

**Test Responsibilities**: For each MODE detected as supported by satp in Group 1, directly write to `vsatp` (CSR 0x280) in M/HS-mode and verify readback MODE consistency, confirming vsatp supports the same mode set.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSATP-MODE-01 | vsatp MODE=Bare support | Write `vsatp = 0` (MODE=0), read back MODE | `MODE == 0` (Bare must be supported by vsatp) |
| VSATP-MODE-02 | vsatp MODE=Sv39 support | Execute only when `g_satp_supports_sv39 == true`; write `vsatp = 8 << 60`, read back MODE | `MODE == 8` (Shvsatpa requires consistency) |
| VSATP-MODE-03 | vsatp MODE=Sv48 support | Execute only when `g_satp_supports_sv48 == true`; write `vsatp = 9 << 60`, read back MODE | `MODE == 9` (Shvsatpa requires consistency) |
| VSATP-MODE-04 | vsatp MODE=Sv57 support | Execute only when `g_satp_supports_sv57 == true`; write `vsatp = 10 << 60`, read back MODE | `MODE == 10` (Shvsatpa requires consistency) |

> [!IMPORTANT]
> This is the core verification group for the Shvsatpa extension. If satp supports Sv39 but vsatp does not, the Shvsatpa constraint is violated. The assertion condition is: **for each MODE supported by satp, the MODE read back after writing to vsatp must equal the written value**.

---

### Group 3: VS-mode (V=1) Verification of `vsatp` Mode Support via `satp` Instruction

**Specification Basis**:
- `norm:shvsatpa_satp_vsatp_modes` (`SPEC/shvsatpa.adoc:4-6`): Modes supported by satp must also be supported by vsatp
- `norm:vsatp_sz_acc_op` (`SPEC/hypervisor.adoc:1384-1392`): When V=1, `vsatp` substitutes for `satp`
- `norm:vsatp_mode_unsupported_v1` (`SPEC/hypervisor.adoc:1417-1418`): When V=1, writing unsupported MODE causes write to be ignored

**Test Responsibilities**: In VS-mode (V=1), write each MODE supported by satp via the `satp` instruction name (which actually operates vsatp) and confirm via readback. This simultaneously verifies the V=1 passthrough path and Shvsatpa consistency constraint.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSATP-VS-01 | VS-mode write satp MODE=Bare | VS-mode `csrw satp, 0`, read back `satp` (actually operates vsatp) | Readback MODE=0 |
| VSATP-VS-02 | VS-mode write satp MODE=Sv39 | Only when satp supports Sv39; VS-mode `csrw satp, 8<<60`, read back | Readback MODE=8 |
| VSATP-VS-03 | VS-mode write satp MODE=Sv48 | Only when satp supports Sv48; VS-mode `csrw satp, 9<<60`, read back | Readback MODE=9 |
| VSATP-VS-04 | VS-mode write satp MODE=Sv57 | Only when satp supports Sv57; VS-mode `csrw satp, 10<<60`, read back | Readback MODE=10 |

> [!NOTE]
> Reading back `satp` in VS-mode actually reads `vsatp` (V=1 passthrough). Therefore, the "write→readback→judge" closed loop can be completed within VS-mode. However, M-mode must also perform cross-verification via CSR 0x280 (see Group 4).

---

### Group 4: V=1 Passthrough Semantics Verification (`satp` Access Actually Operates `vsatp`)

**Specification Basis**:
- `norm:vsatp_sz_acc_op` (`SPEC/hypervisor.adoc:1384-1392`): When V=1, `vsatp` substitutes for the usual `satp`, so instructions that normally read or modify `satp` actually access `vsatp` instead

**Test Responsibilities**: Verify that values written by VS-mode (V=1) via `satp` instruction name can be read back from M/HS-mode via `vsatp` (CSR 0x280); and vice versa. Confirm that satp operations when V=1 do not affect HS-mode's real satp.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSATP-TRANS-01 | VS-mode writes satp → M-mode reads vsatp | VS-mode writes `satp = TEST_VAL` (with supported MODE), return to M-mode and read CSR 0x280 | `vsatp == TEST_VAL` |
| VSATP-TRANS-02 | M-mode writes vsatp → VS-mode reads satp | M-mode writes `vsatp = TEST_VAL`, enter VS-mode and read `satp` | VS-mode readback == `TEST_VAL` |
| VSATP-TRANS-03 | VS-mode write satp does not affect HS-mode satp | Backup HS `satp`; VS-mode writes `satp = TEST_VAL`; return to HS-mode and read real `satp` (V=0) | HS-mode `satp` retains original value |

---

### Group 5: Write Handling for Unsupported MODE Values

**Specification Basis**:
- `norm:vsatp_mode_unsupported_v0` (`SPEC/hypervisor.adoc:1415-1416`): When V=0, writing unsupported MODE value to `vsatp` either ignores the entire write (same as satp behavior) or treats fields as WARL normally
- `norm:vsatp_mode_unsupported_v1` (`SPEC/hypervisor.adoc:1417-1418`): When V=1, writing unsupported MODE value to `satp` (actually writes vsatp) causes the write to be **ignored**, vsatp is not modified
- `norm:satp_mode_op_unsupported` (`SPEC/supervisor.adoc:1052-1055`): Writing unsupported MODE to satp renders entire write ineffective

**Test Responsibilities**: Verify that when writing unsupported MODE values to vsatp, the behavior under V=0 and V=1 paths respectively conforms to the specification.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSATP-UNSUP-01 | V=0 write vsatp reserved MODE=7 | M-mode first sets vsatp=Bare, then writes MODE=7, read back | Readback MODE != 7 (WARL: either entire write is ignored with readback as 0, or MODE is forced to legal value) |
| VSATP-UNSUP-02 | V=0 write vsatp reserved MODE=15 | M-mode first sets vsatp=Bare, then writes MODE=15, read back | Readback MODE != 15 (same as above) |
| VSATP-UNSUP-03 | V=1 write satp unsupported MODE (entire write ignored) | First M-mode sets vsatp=known value; VS-mode writes satp with unsupported MODE; M-mode reads back vsatp | vsatp retains original value unchanged (under V=1 entire write is ignored) |

> [!NOTE]
> VSATP-UNSUP-03 is V=1 specific behavior: unlike V=0, when V=1 writing unsupported MODE causes the **write to be completely ignored** (rather than WARL processing). This means even if the ASID/PPN fields in the write are legal, they will not be written.

---

### Group 6: ASID Field Width Verification

**Specification Basis**:
- `norm:vsatp_asid_op` (`SPEC/hypervisor.adoc:1394-1400`): The ASID field of vsatp is WARL, behavior similar to satp's ASID
- `norm:shvsatpa_satp_vsatp_modes` (`SPEC/shvsatpa.adoc:4-6`): Shvsatpa implies "MODEs supported by vsatp also means ASID field is usable"

**Test Responsibilities**: Verify that the ASID field width of vsatp is consistent with satp (Shvsatpa requires mode consistency, ASID width should be synchronized).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSATP-ASID-01 | vsatp ASID width matches satp | Probe satp ASID width (write all 1s and read back); similarly probe vsatp ASID width; compare both | vsatp ASID width == satp ASID width (both are WARL, supported bit widths should be identical) |

---

## Appendix: Related Constants and API Reference

### CSR and Bit Definitions

| Name | Value | Description |
|------|-------|-------------|
| `CSR_VSATP` | `0x280` | Virtual supervisor address translation and protection, see `common/encoding.h` |
| `CSR_SATP` | `0x180` | Supervisor address translation and protection (access when V=1 actually operates vsatp) |
| `SATP_MODE_SHIFT` | 60 | Starting bit position of MODE field when SXLEN=64 |
| `SATP_MODE_MASK` | `0xF << 60` | MODE field mask (4 bits) |

### MODE Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `MODE_BARE` | 0 | No translation protection |
| `MODE_SV39` | 8 | 39-bit paged virtual address |
| `MODE_SV48` | 9 | 48-bit paged virtual address |
| `MODE_SV57` | 10 | 57-bit paged virtual address |
| `MODE_SV64` | 11 | 64-bit (future specification) |

### Global Variables

| Variable | Type | Description |
|----------|------|-------------|
| `g_satp_supports_bare` | `volatile bool` | Whether satp supports Bare mode |
| `g_satp_supports_sv39` | `volatile bool` | Whether satp supports Sv39 mode |
| `g_satp_supports_sv48` | `volatile bool` | Whether satp supports Sv48 mode |
| `g_satp_supports_sv57` | `volatile bool` | Whether satp supports Sv57 mode |
| `g_vsmode_satp_readback` | `volatile uintptr_t` | Pass-through variable for VS-mode satp readback value |

### Test Framework API

- `run_in_vs_mode(fn, arg)`: Execute fn(arg) in VS-mode (V=1), return to M-mode via ecall
- `hyp_reset_state()`: Reset all hypervisor CSRs
- `HYP_TEST_END()`: Test end macro, includes hyp_reset_state + result recording
- `TEST_REGISTER` / `TEST_BEGIN` / `TEST_ASSERT` / `TEST_SKIP` / `TEST_END`: Test case registration and assertion macros
- `TEST_LOG(fmt, ...)`: Test log output

---

## Test Execution Notes

### Subdirectory and Build Integration (Implementation Phase Reference, Not Produced by This Plan)

> [!IMPORTANT]
> This plan phase **only produces `DOCS/testplan/shvsatpa_test_plan.md`**, does not create `shvsatpa/` subdirectory, does not modify top-level `Makefile`, does not write C test code. Subsequent implementation phase creates per this document:
> - `shvsatpa/main.c` (refer to existing extensions such as `svadu/main.c`)
> - `shvsatpa/Makefile` (`SPIKE_ISA_EXT = _shvsatpa`)
> - `shvsatpa/kernel.ld`
> - `shvsatpa/tests/test_probe.c`, `test_mode.c`, `test_vsmode.c`, `test_transparent.c`, `test_unsupported.c`
> And add `shvsatpa` to the `EXTENSIONS` list in top-level `Makefile`.

### Runtime Environment

- Group 1: M/HS-mode directly operates CSR 0x180 (satp), probes supported modes
- Group 2: M/HS-mode directly operates CSR 0x280 (vsatp), verifies mode consistency
- Group 3: Enter VS-mode via `run_in_vs_mode`, VS-mode operates vsatp via satp instructions
- Group 4: M-mode ↔ VS-mode alternating execution, verifies CSR passthrough
- Group 5: M/HS-mode + VS-mode verifies write behavior for unsupported MODE
- Single-core environment, no IPI required
- Uses G-stage identity mapping to ensure VS-mode execution environment

### Failure Diagnosis Guide

| Symptom | Possible Cause |
|---------|----------------|
| VSATP-PROBE-02/03/04 all show unsupported | Platform has not implemented any paging mode (only Bare), need to confirm hardware capability |
| VSATP-MODE-02 fails (satp supports Sv39 but vsatp does not) | **Shvsatpa constraint violation**: vsatp.MODE is excessively truncated by WARL |
| VSATP-MODE-03/04 fail | Same as above, vsatp has not implemented advanced translation modes supported by satp |
| VSATP-VS-02/03/04 fail | MODE write may have additional restrictions under V=1 path (non-compliant with Shvsatpa) |
| VSATP-TRANS-01/02 fail | satp instruction did not correctly map to vsatp when V=1, H extension implementation anomaly |
| VSATP-TRANS-03 fails | VS-mode write to satp incorrectly modified HS-mode's satp |
| VSATP-UNSUP-01/02 readback MODE equals written reserved value | vsatp.MODE WARL implementation anomaly (should not retain illegal values) |
| VSATP-UNSUP-03 fails (vsatp was modified) | Write with unsupported MODE under V=1 was not completely ignored, violates `norm:vsatp_mode_unsupported_v1` |
