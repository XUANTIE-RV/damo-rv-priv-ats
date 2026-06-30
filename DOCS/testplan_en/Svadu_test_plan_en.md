**[中文](../testplan/Svadu_test_plan.md) | English**

# Svadu Extension Test Plan

This document describes the test plan for the Svadu (Hardware Updating of A/D Bits) extension. The Svadu extension provides support for **hardware automatic updating** of PTE A/D bits, and allows enabling/disabling this behavior at runtime through the `menvcfg.ADUE` field; when hardware updating is disabled, the processor falls back to Svade behavior (A/D triggers page-fault, set by software).

---

## Overview

The RISC-V Privileged Specification defines two PTE A/D bit management schemes:

1. **Hardware update scheme (Svadu enabled)**: When a page with A=0 is accessed or a page with D=0 is written, the hardware atomically updates the PTE A/D bits, the access proceeds, and no exception is raised.
2. **Software update scheme (Svade behavior)**: When a page with A=0 is accessed or a page with D=0 is written, the hardware raises a page-fault exception, and the software trap handler explicitly sets the bit and retries.

The Svadu extension serves as a **unified control layer** for both schemes:

- When Svadu is implemented, the `menvcfg.ADUE` (bit 61) field is writable (WARL).
- When `menvcfg.ADUE=1`, the processor is in "hardware update" mode: accessing a PTE with A=0 / D=0 in S/U-mode causes the hardware to atomically update the corresponding bit, and the access completes successfully.
- When `menvcfg.ADUE=0`, the processor is in "Svade mode": the hardware does not update A/D, the access triggers a page-fault, which is fully consistent with the standalone Svade extension behavior.

For a processor implementing the Svadu extension, step 9 of the virtual address translation process (see `SPEC/supervisor.adoc:1758`, "If the Svade extension is implemented, stop and raise a page-fault exception corresponding to the original access type") selects one of two paths based on `menvcfg.ADUE` (`SPEC/machine.adoc:2236–2245`):
- ADUE=0: behavior is equivalent to Svade -- when `pte.a=0` or (store and `pte.d=0`) is detected, translation stops and a page-fault corresponding to the original access type is raised;
- ADUE=1: the hardware atomically sets `pte.a` to 1, and for stores also sets `pte.d` to 1, then continues translation without raising a page-fault.

This test plan focuses on the CSR control behavior of this extension, access semantics under ADUE=1 / ADUE=0 modes, coverage across different page granularities (4 KiB / 2 MiB / 1 GiB), and behavioral changes after dynamically switching ADUE at runtime.

---

## Test Scope

### Specification Sources

| File | Line/Section | Content |
|------|---------|------|
| `SPEC/svadu.adoc` | Full text (lines 1-15) | Svadu extension definition; `menvcfg.ADUE` / `henvcfg.ADUE` writability; ADUE=0 fallback to Svade semantics |
| `SPEC/machine.adoc` | Line 2170 (`[[sec:menvcfg]]`); lines 2236-2247 (ADUE field semantics) | `menvcfg` CSR field encoding (ADUE=bit 61); `norm:menvcfg_adue_op` describes ADUE controlling hardware A/D update during S-mode translation; `norm:menvcfg_adue_rdonly0` specifies "ADUE is read-only zero when Svadu is not implemented" |
| `SPEC/supervisor.adoc` | From line 1623 (Svade concept); line 1758 (translation algorithm step 9 page-fault raise point) | "If the Svade extension is implemented, stop and raise a page-fault exception corresponding to the original access type" -- translation endpoint shared by Svade/Svadu |
| `SPEC/hypervisor.adoc` | Lines 2109-2111 | After modifying `menvcfg.PBMTE/ADUE`, `HFENCE.GVMA(x0,x0)` must be executed to synchronize G/VS-stage translation; for non-Hyp platforms the corresponding instruction is `SFENCE.VMA(x0,x0)` |
| `common/encoding.h` | Lines 25-30 | Repository already defines `CSR_MENVCFG = 0x30A` and `MENVCFG_ADUE = (1ULL << 61)` macros, ready for direct use |

### Covered Specification Points

| Norm ID | Original Text |
|---------|------|
| `norm:Svadu_hw_update_a_d_bits` | If the Svadu extension is implemented, the `menvcfg`.ADUE field is writable. |
| `norm:menvcfg_adue_rdonly0` | If Svadu is not implemented, ADUE is read-only zero. |
| `norm:menvcfg_adue_op` | If the Svadu extension is implemented, the ADUE bit controls whether hardware updating of PTE A/D bits is enabled for S-mode and G-stage address translations. When ADUE=1, hardware updating of PTE A/D bits is enabled during S-mode address translation, and the implementation behaves as though the Svade extension were not implemented for S-mode address translation. When the hypervisor extension is implemented, if ADUE=1, hardware updating of PTE A/D bits is enabled during G-stage address translation, and the implementation behaves as though the Svade extension were not implemented for G-stage address translation. When ADUE=0, the implementation behaves as though Svade were implemented for S-mode and G-stage address translation. |
| `norm:Svadu_disabled_hw_update_falls_back_to_svade` | When hardware updating of A/D bits is disabled, the Svade extension, which mandates exceptions when A/D bits need be set, instead takes effect. |
| `norm:svade_access_ad_bit_clear` | The Svade extension: when a virtual page is accessed and the A bit is clear, or is written and the D bit is clear, a page-fault exception is raised. |
| `norm:menvcfg_adue_fence` | After changing `menvcfg`.ADUE, executing an SFENCE.VMA instruction with _rs1_=`x0` and _rs2_=`x0` suffices to synchronize address-translation caches with respect to the altered interpretation of page-table entries' A/D bits. |

### Out of Scope

- **Hypervisor two-stage translation scenarios**: `henvcfg.ADUE` writability, Svadu behavior under VS-stage and G-stage, HLV/HSV instruction interactions (involves H extension, which is not included in this test platform).
- **Sv32 / Sv48 / Sv57 modes**: This plan only covers Sv39, consistent with the existing svade/svnapot/svpbmt test plans.
- **Multi-hart consistency**: The specification requires all harts to use the same PTE update scheme; the current test framework is a single-core environment.
- **PMP / Smepmp and Svadu interactions**: Related interactions are independently covered by the PMP test plan series.

---

## Test Groups

> [!IMPORTANT]
> A total of 7 test groups, each providing: specification basis, test responsibilities, test case table (ID/name/description/expected result), and key code examples. All tests run in Sv39 mode.

---

### Group 1: menvcfg.ADUE Field Control Tests

**Specification basis**:
- `norm:Svadu_hw_update_a_d_bits`: `menvcfg.ADUE` must be writable when Svadu is implemented

**Test responsibilities**: Verify the writability, read-write consistency, non-interference with other fields, and reset initial value of `menvcfg.ADUE` (bit 61) in M-mode. This is the most basic check for Svadu implementability.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| SVADU-CSR-01 | menvcfg.ADUE writable 0->1 | M-mode writes ADUE=1, reads back menvcfg | Read-back bit 61 = 1 |
| SVADU-CSR-02 | menvcfg.ADUE writable 1->0 | Set ADUE=1 first, then clear ADUE=0, read back | Read-back bit 61 = 0 |
| SVADU-CSR-03 | ADUE does not affect other fields | Record PBMTE/STCE/CBIE/FIOM and other fields while toggling ADUE | Other field values remain unchanged before and after ADUE toggle |
| SVADU-CSR-04 | ADUE reset behavior check | Save the original menvcfg value at test suite entry (before iterating `_test_table` in `main.c`); this test reads bit 61 of that saved value | Implementation-defined; record and print the value (no specific value enforced); constraint: save logic must complete before any other test runs |

> [!NOTE]
> If the test platform does not implement Svadu, writing ADUE=1 should still read back as 0 (invalid write ignored per WARL semantics). SVADU-CSR-01 serves as the "hard" determination of Svadu implementability: when it returns 0, all tests in Groups 2/3/4/5/7 are SKIPped via `detect_svadu()`; Group 6 (ADUE=0 fallback) is also meaningless when Svadu is not implemented (since it is already the default behavior), and is likewise SKIPped.

---

### Group 2: Hardware A Bit Update on 4 KiB Leaf PTEs with ADUE=1

**Specification basis**:
- `norm:Svadu_adue_enabled_hw_updates_a_on_access`: Hardware atomically sets PTE.A=1 when ADUE=1
- `norm:Svadu_adue_no_pagefault_when_enabled`: A=0 no longer triggers a page-fault under ADUE=1

**Test responsibilities**: Verify that on 4 KiB leaf PTEs with ADUE=1, various accesses (load, instruction fetch) to pages with A=0 do not trigger a page-fault, and that PTE.A has been set to 1 by hardware upon returning to M-mode.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| SVADU-A4K-01 | A=0 R page load | ADUE=1, 4 KiB PTE: V=1, R=1, A=0, D=0, S-mode load | Load succeeds (result=0), PTE.A=1 |
| SVADU-A4K-02 | A=0 X page fetch | ADUE=1, 4 KiB PTE: V=1, X=1, A=0, S-mode jump and execute | Instruction fetch succeeds, PTE.A=1 |
| SVADU-A4K-03 | A=0 RW page load | ADUE=1, 4 KiB PTE: V=1, R=1, W=1, A=0, D=1, S-mode load | Load succeeds, PTE.A=1, PTE.D remains 1 (load only does not affect D) |
| SVADU-A4K-04 | A set only once after multiple loads | ADUE=1, perform N loads on A=0 page | Every load succeeds, final PTE.A=1 (subsequent loads trigger no additional side effects) |

---

### Group 3: Hardware D Bit Update on 4 KiB Leaf PTEs with ADUE=1

**Specification basis**:
- `norm:Svadu_adue_enabled_hw_updates_d_on_store`: Hardware sets PTE.D=1 after store/AMO when ADUE=1
- `norm:Svadu_adue_no_pagefault_when_enabled`: D=0 store no longer triggers a page-fault under ADUE=1

**Test responsibilities**: Verify that on 4 KiB leaf PTEs with ADUE=1, store and AMO operations on pages with D=0 do not trigger a page-fault, and that PTE.D has been set to 1 by hardware upon returning to M-mode. Key focus: verifying that a single store access with A=0+D=0 can simultaneously set both flags.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| SVADU-D4K-01 | A=1 D=0 RW page store | ADUE=1, PTE: V=1, R=1, W=1, A=1, D=0, S-mode store | Store succeeds, PTE.D=1 (A remains 1) |
| SVADU-D4K-02 | A=0 D=0 RW page store | ADUE=1, PTE: V=1, R=1, W=1, A=0, D=0, S-mode store | Store succeeds, PTE.A=1 and PTE.D=1 (single store sets both) |
| SVADU-D4K-03 | A=1 D=0 RW page amoadd | ADUE=1, PTE: V=1, R=1, W=1, A=1, D=0, S-mode amoadd.w | AMO succeeds, PTE.D=1 |
| SVADU-D4K-04 | A=1 D=1 RW page store (no side effect) | ADUE=1, PTE: A=1, D=1, S-mode store | Store succeeds, PTE.A and PTE.D remain 1 |

> [!IMPORTANT]
> SVADU-D4K-02 verifies that a single store atomically sets both A=1 and D=1 in hardware, which is the key difference between Svadu and the software "set A first, then trigger D fault" two-step approach.

---

### Group 4: Hardware A/D Update on 2 MiB Megapages with ADUE=1

**Specification basis**:
- `norm:Svadu_adue_applies_all_levels`: Hardware A/D update is performed on leaf PTEs; 2 MiB megapages are equally applicable

**Test responsibilities**: Verify that hardware A/D update behavior with ADUE=1 also applies to 2 MiB megapages (level 1 leaf PTE in Sv39), covering load / store / fetch / AMO access types.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| SVADU-2M-01 | 2M A=0 load | 2 MiB leaf PTE: V=1, R=1, A=0, D=0, S-mode load | Load succeeds, megapage PTE.A=1 |
| SVADU-2M-02 | 2M A=1 D=0 store | 2 MiB leaf PTE: V=1, R=1, W=1, A=1, D=0, S-mode store | Store succeeds, megapage PTE.D=1 |
| SVADU-2M-03 | 2M A=0 X page fetch | 2 MiB leaf PTE: V=1, X=1, A=0, S-mode jump and execute | Instruction fetch succeeds, megapage PTE.A=1 |
| SVADU-2M-04 | 2M A=0 D=0 store | Single store sets both bits | Store succeeds, PTE.A=1 and PTE.D=1 |
| SVADU-2M-05 | 2M A=1 D=0 amoadd.w | 2 MiB leaf PTE: V=1, R=1, W=1, A=1, D=0, S-mode amoadd.w | AMO succeeds, megapage PTE.D=1 |

---

### Group 5: Hardware A/D Update on 1 GiB Gigapages with ADUE=1

**Specification basis**:
- `norm:Svadu_adue_applies_all_levels`: Hardware A/D update is performed on leaf PTEs; 1 GiB gigapages are equally applicable

**Test responsibilities**: Verify that hardware A/D update behavior with ADUE=1 also applies to 1 GiB gigapages (level 2 leaf PTE in Sv39), covering load / store / AMO access types. A 1 GiB X-permission page typically spans the entire code segment, making it impractical as a standalone fetch test case; fetch testing is sufficiently covered by SVADU-2M-03 and SVADU-A4K-02.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| SVADU-1G-01 | 1G A=0 load | 1 GiB leaf PTE: V=1, R=1, A=0, D=0, S-mode load | Load succeeds, gigapage PTE.A=1 |
| SVADU-1G-02 | 1G A=1 D=0 store | 1 GiB leaf PTE: V=1, R=1, W=1, A=1, D=0, S-mode store | Store succeeds, gigapage PTE.D=1 |
| SVADU-1G-03 | 1G A=0 D=0 store | Single store sets both A and D | Store succeeds, PTE.A=1 and PTE.D=1 |
| SVADU-1G-04 | 1G A=1 D=0 amoadd.w | 1 GiB leaf PTE: V=1, R=1, W=1, A=1, D=0, S-mode amoadd.w | AMO succeeds, gigapage PTE.D=1 |

> [!NOTE]
> 1 GiB gigapage tests require ensuring the test VA is chosen in an identity-mapped region far from the M-mode code segment and stack, to avoid conflicts with the 1 GiB region containing the code segment.

---

### Group 6: Fallback to Svade Behavior with ADUE=0

**Specification basis**:
- `norm:Svadu_disabled_hw_update_falls_back_to_svade`: When hardware A/D updating is disabled, Svade behavior takes effect

**Test responsibilities**: Test that when `menvcfg.ADUE=0`, a processor implementing Svadu behaves identically to Svade -- A=0 / D=0 triggers a page-fault and PTE content remains unchanged. This group's test cases mirror Groups 1/2/3 of `docs/svade_test_plan.md`.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| SVADU-FB-01 | ADUE=0 A=0 load | ADUE=0, 4 KiB PTE: V=1, R=1, A=0, S-mode load | Load page-fault (scause=13) |
| SVADU-FB-02 | ADUE=0 D=0 store | ADUE=0, 4 KiB PTE: V=1, R=1, W=1, A=1, D=0, S-mode store | Store page-fault (scause=15) |
| SVADU-FB-03 | ADUE=0 A=0 fetch | ADUE=0, 4 KiB PTE: V=1, X=1, A=0, S-mode jump and execute | Instruction page-fault (scause=12) |
| SVADU-FB-04 | ADUE=0 AMO D=0 | ADUE=0, 4 KiB PTE: V=1, R=1, W=1, A=1, D=0, S-mode amoadd.w | Store/AMO page-fault (scause=15) |
| SVADU-FB-05 | ADUE=0 PTE unchanged after fault | After triggering any of the above faults, return to M-mode and check PTE | Corresponding A/D bits retain original values (not updated by hardware) |

> [!IMPORTANT]
> Group 6 is the "ADUE=0 mirror" of Groups 1/2/3 in svade_test_plan.md, verifying that a Svadu implementation with hardware updating disabled must be equivalent to Svade. Any test that passes under a Svade implementation must also pass under the Svadu + ADUE=0 configuration.

---

### Group 7: Runtime Dynamic ADUE Switching

**Specification basis**:
- `norm:Svadu_runtime_switch_requires_sfence` (source: `SPEC/hypervisor.adoc:2109–2111`): After modifying `menvcfg.ADUE`, `SFENCE.VMA(x0,x0)` must be executed (non-Hyp platforms) for the new setting to take effect

**Test responsibilities**: Verify that after dynamically switching `menvcfg.ADUE` in M-mode at runtime and executing SFENCE.VMA, S-mode access behavior immediately follows the new configuration. Covers 0->1, 1->0 switching and multi-switch stability.

> [!IMPORTANT]
> Since `menvcfg` is only writable from M-mode, all ADUE switching is performed in M-mode (not in the S-mode trap handler). The test pattern is: M-mode sets ADUE -> `vm_run_in_smode` enters S-mode access -> return to M-mode for verification -> switch ADUE -> re-enter S-mode for verification.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| SVADU-SW-01 | 0->1 switch: enable HW update after fault | M-mode sets ADUE=0, S-mode triggers A=0 load fault -> return to M-mode, set ADUE=1 + SFENCE.VMA -> same PTE unchanged (PTE.A still 0), re-enter S-mode to access same VA | First: scause=13; second: load succeeds, PTE.A=1 (set by hardware) |
| SVADU-SW-02 | 1->0 switch: disable HW update | M-mode sets ADUE=1, S-mode accesses A=0 page P1 completing HW update -> return to M-mode, set ADUE=0 + SFENCE.VMA -> map new A=0 page P2 -> S-mode accesses P2 | P1 access succeeds and P1.A=1; P2 access triggers scause=13 and P2.A remains 0 |
| SVADU-SW-03 | SFENCE.VMA required after switch | Set ADUE=0 -> do not execute SFENCE.VMA -> immediately set ADUE=1 -> execute SFENCE.VMA -> S-mode accesses A=0 page | Access succeeds, PTE.A=1 (verifies final effect; this test case does not assert "stale behavior without fence", only verifies correct behavior after the final fence) |
| SVADU-SW-04 | Multi-switch stability | Toggle ADUE between 0/1 >= 4 times, after each switch + SFENCE.VMA access A=0 page with a fresh PTE context | After each switch, behavior strictly matches current ADUE value (ADUE=1 succeeds in setting bits, ADUE=0 triggers fault) |

> [!NOTE]
> Regarding "whether ADUE modification is immediately visible without SFENCE.VMA" -- `SPEC/hypervisor.adoc:2109` explicitly requires executing a synchronization instruction. This means implementations may return stale behavior without a fence; this test plan adopts a **conservative policy**, enforcing SFENCE.VMA after every ADUE switch, and does not test unfenced edge behavior (to avoid coupling tests with implementation-defined behavior).

---

## Appendix: Related Constants and API Reference

### menvcfg.ADUE Field

| Field | Position | Meaning |
|------|------|------|
| `ADUE` | menvcfg[61] | Enable hardware updating of PTE A/D bits (0=disabled, falls back to Svade; 1=hardware updating enabled) |

### scause Constants

All defined in `common/encoding.h` (verified).

| Constant | Value | Description |
|------|-----|------|
| `CAUSE_FETCH_PAGE_FAULT` | 12 | Instruction page fault |
| `CAUSE_LOAD_PAGE_FAULT` (alias `CAUSE_LPF`, `encoding.h:185`) | 13 | Load page fault |
| `CAUSE_STORE_PAGE_FAULT` (alias `CAUSE_SPF`, `encoding.h:186`) | 15 | Store/AMO page fault |

> [!NOTE]
> The svade `test_helpers.h` uses the `CAUSE_LPF` abbreviation; this test plan's example code uniformly uses the full name `CAUSE_LOAD_PAGE_FAULT`; both are fully equivalent.

### menvcfg CSR Access Reference Implementation

The repository currently does not have a generic `csr_read/csr_write` macro; `menvcfg` (CSR 0x30A) is accessed via inline assembly. The Svadu test helpers should provide the following static inline functions (refer to the Group 1 code examples):

> [!NOTE]
> `common/hyp/hyp_csr.h` already has identically named `menvcfg_read/menvcfg_write` functions, but they belong to the Hypervisor module; svadu tests do not depend on the hyp module and should provide the above static inline implementations in `svadu/tests/test_helpers.h`.

### Reusing svade Helper Functions

All helpers in svade's `tests/test_helpers.h` are `static` functions (see the file header comment in `svade/tests/test_helpers.h`: `All test files are #included into test_register.c`). Svadu adopts the same "single TU #include" organization.

> [!IMPORTANT]
> **Linker constraints for reusing svade helpers**: svade's `setup_code_mapping()`, `test_data_area`, `test_fault_page`, `test_exec_page`, `test_region_2m_va` and other interfaces depend on the following two linker section symbols provided by `svade/kernel.ld`:
> - `__vm_test_region_start` / `__vm_test_region_end` (3 x 4KB pages, 2MB aligned)
> - `__vm_test_region_2m_start` / `__vm_test_region_2m_end` (2MB aligned 2MB region)
>
> Svadu must **fully replicate** these two section definitions in `svadu/kernel.ld` (see `svade/kernel.ld:67–96`); otherwise cross-directory `#include` of svade helpers will result in `undefined symbol` linker errors.
>
> Recommended organization (choose one; this plan prefers option 1):
> 1. **Copy svade helpers to `svadu/tests/test_helpers.h`**: Physically copy the svade helpers content and extend with ADUE operation macros and `detect_svadu`, keeping decoupled from svade; also copy the same .vm_test_region* sections to `svadu/kernel.ld`.
> 2. **Cross-directory #include**: At the top of `svadu/tests/test_helpers.h`, `#include "../../svade/tests/test_helpers.h"`, and copy the .vm_test_region* sections in `svadu/kernel.ld`.

The table below is based on the premise of "reusing with `static` inline visibility" (whether physical copy or cross-directory include, the interface is identical to the caller):

| Function/Macro | Source | Description |
|---------|------|------|
| `pt_context_t` / `pt_init` / `pt_pool_reset` / `pt_destroy` | `common/vm/vm.h` | Page table context management |
| `pt_map_page(ctx, va, pa, flags, level)` | `common/vm/vm.h` | Single page mapping, level takes `PT_LEVEL_4K`/`PT_LEVEL_2M`/`PT_LEVEL_1G` |
| `pt_setup_identity_mapping(ctx, base, size, flags, level)` | `common/vm/vm.h` | Region identity mapping |
| `pt_get_pte(ctx, va, level)` | `common/vm/vm.h` | Returns pointer to PTE at specified level; returns NULL if unmapped |
| `vm_run_in_smode(ctx, fn, arg)` | `common/vm/vm.h` | Enters S-mode with the given page table context to execute `fn(arg)`, returns `fn`'s return value. The svade helpers' `test_smode_load/store/exec/amoadd` and similar fn wrappers encapsulate `trap_expect_begin/end` and `trap_get_cause()`, so "returns 0 on no exception, returns scause on exception" is the helpers' semantics, not `vm_run_in_smode`'s own semantics. |
| `vm_sfence_vma(vaddr, asid)` | `common/vm/vm.h` | Executes SFENCE.VMA; 0/0 means global flush |
| `setup_code_mapping(ctx)` | `svade/tests/test_helpers.h:55` | Sets up M-mode code segment, stack, UART, and 2M identity mapping skipping .vm_test_region* |
| `pte_read(ctx, va, level)` | `svade/tests/test_helpers.h:225` (wraps `pt_get_pte`) | Reads PTE value at specified level; returns 0 if unmapped |
| `pte_set_bits(ctx, va, level, bits)` | `svade/tests/test_helpers.h:234` | OR-sets PTE bits and executes SFENCE.VMA |
| `test_smode_load/store/exec/load_and_store/amoadd` | `svade/tests/test_helpers.h:106–195` | Executes the corresponding access in S-mode; returns 0 on no exception, returns scause on exception |
| `test_data_area` / `test_fault_page` / `test_exec_page` / `test_region_2m_va` | `svade/tests/test_helpers.h:34–42` | 4K/2M test region pointers within `.vm_test_region` |
| `init_exec_page()` | `svade/tests/test_helpers.h:50` | Fills test_exec_page with `nop;ret` for use by SVADU-A4K-02 / 2M-03 |
| `TEST_REGISTER` / `TEST_BEGIN` / `TEST_ASSERT` / `TEST_END` / `TEST_SKIP` | `common/test_framework.h` | Test case registration, assertion, and lifecycle macros |
| `CAUSE_LPF` / `CAUSE_LOAD_PAGE_FAULT` / `CAUSE_STORE_PAGE_FAULT` / `CAUSE_FETCH_PAGE_FAULT` | `common/encoding.h` | scause constants (svade helpers use `CAUSE_LPF` abbreviation) |

### Svadu-Specific Helpers (new additions in `svadu/tests/test_helpers.h`)

| Function | Implementation Notes |
|------|----------|
| `menvcfg_read/menvcfg_set/menvcfg_clear` | See "menvcfg CSR Access Reference Implementation" above |
| `set_menvcfg_adue(int enable)` | Calls `menvcfg_set(MENVCFG_ADUE)` or `menvcfg_clear(MENVCFG_ADUE)` followed by `vm_sfence_vma(0,0)` (satisfies `SPEC/hypervisor.adoc:2109`) |
| `get_menvcfg_adue()` | `(menvcfg_read() & MENVCFG_ADUE) ? 1 : 0` |
| `detect_svadu()` | Two-step detection, see complete example below |

`detect_svadu()` complete reference implementation (modeled after `detect_svade()` in `svade/tests/test_helpers.h:208–227`, but with inverted detection logic):

### PTE Flag Macros

- `PTE_V`, `PTE_R`, `PTE_W`, `PTE_X`, `PTE_U`, `PTE_G`, `PTE_A`, `PTE_D`

---

## Test Execution Instructions

- All test cases run in Sv39 mode (`SATP_MODE_SV39`)
- Test code is planned to be placed in a new `svadu/` directory (parallel to `svade/`), with structure consistent with `svade/`:

- **Svadu detection logic** (two-step detection consistent with the `detect_svadu()` complete implementation in the appendix):
  - **Step 1**: M-mode writes `menvcfg.ADUE=1` then reads back bit 61. The specification `norm:menvcfg_adue_rdonly0` (`SPEC/machine.adoc:2247`) mandates that ADUE is read-only zero when Svadu is not implemented, so reading back 0 determines that Svadu is not implemented.
  - **Step 2**: Construct a 4K leaf PTE with A=0 and execute a load in S-mode. If it returns 0 and PTE.A is set by hardware, then Svadu is confirmed implemented and ADUE=1 behavior is correct.
  - **Skip strategy**: Upon detection failure, all test groups that depend on Svadu behavior (Groups 2/3/4/5/7) and Group 6 which verifies fallback semantics are all `TEST_SKIP`ped. Group 1's SVADU-CSR-01 / 02 / 03 are themselves writability tests and do not depend on the `detect_svadu()` result (their failure is itself evidence that the platform lacks Svadu); SVADU-CSR-04 only prints the reset value.
- **trap handler requirements**: The S-mode trap handler must capture page-faults and relay scause back to M-mode via `vm_run_in_smode`'s return value (shares the trap framework with svade).
- **Simulator requirements**:
  - **Spike**: `SPIKE_ISA_EXT = _svadu`, launch command must include `--isa=rv64imac_zicsr_zifencei_svadu`
  - **QEMU**: Use `-cpu max` or explicitly enable the svadu extension (depending on QEMU version support for Svadu)
  - **Sail**: Requires the Sail RISC-V model to support the Svadu extension (refer to `riscv-isa-sim` implementation)
- **Top-level Makefile registration**: Add `svadu` to the `EXTENSIONS` list in the repository root `Makefile`.
