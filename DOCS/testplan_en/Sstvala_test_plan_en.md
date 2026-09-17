**[中文](../testplan/Sstvala_test_plan.md) | English**

# Sstvala Extension Test Plan

This document describes the test plan for the Sstvala (Trap Value Reporting, Version 1.0) extension. The Sstvala extension specifies the value that the `stval` CSR must be written with under different exception types: for address-type exceptions (page-fault, access-fault, misaligned, and breakpoint other than EBREAK), `stval` must be written with the faulting virtual address; for instruction-type exceptions (illegal-instruction, virtual-instruction), `stval` must be written with the faulting instruction encoding.

---

## Specification Sections Covered by This Document

This plan is based on the following RISC-V official specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/sstvala.adoc` — Sstvala Extension for Trap Value Reporting, Version 1.0
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — `stval` CSR definition and write behavior; exception cause encodings
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — Symmetric behavior of `mtval` and `stval`; medeleg delegation
- `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — Triggering conditions for virtual-instruction exceptions (cause=22) (Group 6 has been migrated)

Official repository:

- https://github.com/riscv/riscv-isa-manual (corresponding files `src/priv/sstvala.adoc`, `src/priv/supervisor.adoc`, `src/priv/machine.adoc`, `src/priv/hypervisor.adoc` in the repository)

---

## Test Scope

### Covered Specification Points

| Norm ID | Original Text | Description |
|---------|---------------|-------------|
| `norm:sstvala_stval_faulting_vaddr` | If the Sstvala extension is implemented, then `stval` must be written with the faulting virtual address for load, store, and instruction page-fault, access-fault, and misaligned exceptions, and for breakpoint exceptions that are defined to write an address to stval, other than those caused by execution of the `EBREAK` or `C.EBREAK` instructions. | If Sstvala is implemented, `stval` must be written with the faulting virtual address for load/store/instruction page-fault, access-fault, and misaligned exceptions, and for breakpoint exceptions that are defined to write an address to stval (excluding those caused by `EBREAK`/`C.EBREAK`). |
| `norm:sstvala_stval_faulting_instruction` | For virtual-instruction and illegal-instruction exceptions, `stval` must be written with the faulting instruction. | For virtual-instruction and illegal-instruction exceptions, `stval` must be written with the faulting instruction. |

Self-derived specification points (per-exception-type testable assertions derived from the two official norms above; no independent norm tag exists in SPEC):

| Norm ID | Description |
|---------|-------------|
| `Sstvala_pagefault_tval_addr` | On load/store/instruction page-fault, `stval` must be written with the faulting virtual address |
| `Sstvala_accessfault_tval_addr` | On load/store/instruction access-fault, `stval` must be written with the faulting virtual address |
| `Sstvala_misaligned_tval_addr` | On load/store/instruction misaligned exceptions, `stval` must be written with the faulting virtual address |
| `Sstvala_breakpoint_tval_addr` | For breakpoint exceptions (cause=3), if not caused by EBREAK/C.EBREAK and the specification defines that an address should be written to stval, then `stval` must be written with the faulting address |
| `Sstvala_illegal_inst_tval_inst` | On illegal-instruction exceptions, `stval` must be written with the faulting instruction encoding |
| `Sstvala_virtual_inst_tval_inst` | On virtual-instruction exceptions, `stval` must be written with the faulting instruction encoding |

> [!IMPORTANT]
> The Sstvala specification core is divided into two categories:
> 1. **Address-type exceptions** (page-fault, access-fault, misaligned, breakpoint) → `stval` = faulting virtual address
> 2. **Instruction-type exceptions** (illegal-instruction, virtual-instruction) → `stval` = faulting instruction encoding
>
> All test cases revolve around "triggering a specific exception → verifying the `stval` value".

### Out of Scope

- **`mtval` behavior**: Sstvala only constrains `stval` (S-mode trap value), not `mtval` (M-mode). However, since the test framework uses `mtval` when capturing traps in M-mode, and the specification defines `mtval` behavior symmetrically to `stval`, `mtval` verification for M-mode traps can serve as equivalent evidence.
- **EBREAK / C.EBREAK caused breakpoints**: Explicitly excluded by the specification; `stval` behavior is defined by other specifications.
- **Multi-hart scenarios**: The project is a single-core test environment.
- **Sv32 / Sv48 / Sv57 modes**: Only RV64 + Sv39 is covered, consistent with other extension plans in the project.
- **Guest page-faults (cause 20/21/23)**: Involves H extension two-level translation, independently covered by the Hypervisor test plan.

---

## Design Notes

### 1. General Pattern for stval Verification

All test cases follow a unified pattern:

1. Set up the trigger condition (page-table configuration / PMP configuration / special instruction).
2. Arm trap → execute the triggering instruction → disarm trap.
3. Assert that the trap was triggered.
4. Assert that the trap cause equals the expected value.
5. Assert that the trap tval equals the expected stval value (address or instruction encoding).

The framework's M-mode and S-mode trap handlers both capture `mtval`/`stval` into the trap state record, and test cases read the value through a unified interface.

### 2. stval Verification for Address-Type Exceptions

For page-fault, access-fault, and misaligned exceptions, `stval` should equal the virtual address that triggered the exception. Verification requires:

- **Known target address**: Test code explicitly constructs the access target address, then asserts that the captured tval equals this address.
- **M-mode vs S-mode**: Page-faults require VM enabled (S-mode); access-faults can be triggered in M-mode (PMP) or S-mode; misaligned exceptions can be triggered in any mode.

### 3. stval Verification for Instruction-Type Exceptions

For illegal-instruction exceptions, `stval` should equal the encoding of the faulting instruction. Verification:

- **Known instruction encoding**: Pre-place a known illegal instruction byte sequence in memory, then execute it or read the captured tval after the trap to compare with the expected encoding.
- **32-bit instructions**: `stval` should be the complete 32-bit instruction encoding (zero-extended to XLEN).
- **16-bit compressed instructions**: `stval` should be the 16-bit instruction encoding (zero-extended to XLEN).

### 4. VM Configuration (Page-Fault Scenarios)

Page-fault tests require Sv39 page tables to be enabled:

- **Code/data regions**: Identity mapping with `PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D` to ensure test code and the trap handler can execute normally.
- **Test regions**: Intentionally unmapped VA or insufficient permissions (write to a read-only page) to trigger page-faults.
- Use the framework's S-mode execution helper to enter S-mode; exceptions are caught by the S-mode trap handler.

### 5. PMP Configuration (Access-Fault Scenarios)

Access-faults are triggered via PMP restrictions:

- Configure PMP entries to make specific address regions non-readable/non-writable/non-executable for S/U-mode.
- Access the restricted region in S-mode or U-mode to trigger an access-fault.
- Verify that the captured tval equals the address that was denied access.

### 6. EBREAK Exclusion

The specification explicitly excludes EBREAK/C.EBREAK caused breakpoints. The `stval` behavior for EBREAK is defined by the base privileged specification (typically 0 or PC) and is not within Sstvala test scope. Group 4 only tests breakpoint exceptions triggered by the trigger module (if implemented) or other mechanisms.

> [!NOTE]
> If the platform does not implement the trigger module (Sdtrig extension), Group 4 test cases will be `TEST_SKIP`-ed, without affecting the overall test conclusion.

### 7. Virtual Instruction Exception Notes

Virtual-instruction exceptions (cause=22) require H extension (Hypervisor) support, triggered when executing certain HS-level CSR access instructions in VS-mode. The 3 tests in Group 6 have been migrated to [`Hypervisor_cross_test_plan_en.md`](./Hypervisor_cross_test_plan_en.md) Group 2 (IDs HCROSS-SSTVALA-06~08). This document retains the Group 6 description for reference.

### 8. Misaligned Platform Capability Probe

Whether the platform supports hardware misaligned access determines whether misaligned exceptions are triggered. Test cases probe platform capability at runtime by attempting a misaligned access and observing whether an exception is raised:

- If the platform does not support hardware misaligned access, a misaligned exception is triggered and the tval value is verified normally.
- If the platform supports hardware misaligned access, misaligned access does not raise an exception; the relevant cases are `TEST_SKIP`-ed.
- Instruction misaligned (cause=0) is normally always triggered when jumping to a non-2-byte-aligned address, regardless of the platform's misaligned load/store capability.

If any platform violates the SPEC, the corresponding case remains FAIL, and implementation defects are recorded to the `bugs/` directory.

---

## Test Groups

> [!IMPORTANT]
> A total of 6 test groups and 25 test cases. Groups 1–3 are core address-type exception tests, Group 4 is breakpoint (conditional), Group 5 is core instruction-type exception tests, and Group 6 is optional (requires H extension, migrated).

---

### Group 1: Page-Fault Address-Type Exceptions (stval = Faulting Virtual Address)

**Spec Reference**:
- `Sstvala_pagefault_tval_addr`: On load/store/instruction page-fault, `stval` must be written with the faulting virtual address.

**Test Scope**: Verify that in Sv39 virtual memory mode, when S-mode accesses an unmapped or insufficient-permission virtual address triggering a page-fault, `stval` (captured equivalently via M-mode `mtval`) equals the faulting virtual address.

**Preconditions**:
- Sv39 page table enabled, code/stack regions identity-mapped (`PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D`).
- Target test virtual address unmapped or with restricted permissions.
- M-mode `medeleg` delegates page-faults to S-mode (or M-mode handler captures directly).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TVAL-LPF-01 | load page-fault: unmapped VA | Under Sv39, S-mode executes `ld` on an unmapped VA (e.g., `0x40000000`), triggering load page-fault (cause=13) | cause == 13; tval == `0x40000000` |
| TVAL-LPF-02 | load page-fault: read from writable page (negative test) | 4 KiB PTE: V=1, W=1, R=1, X=0, A=1, D=1; S-mode load should succeed | No exception (negative test, confirming correct mapping does not trigger fault) |
| TVAL-SPF-01 | store page-fault: write to read-only page | 4 KiB PTE: V=1, R=1, W=0, A=1, D=1; S-mode executes `sd` on this VA, triggering store page-fault (cause=15) | cause == 15; tval == test_va |
| TVAL-SPF-02 | store page-fault: unmapped VA | S-mode executes `sd` on an unmapped VA, triggering store page-fault (cause=15) | cause == 15; tval == unmapped_va |
| TVAL-IPF-01 | instruction page-fault: unmapped VA fetch | S-mode jumps to an unmapped VA for instruction fetch, triggering instruction page-fault (cause=12) | cause == 12; tval == target_pc |
| TVAL-IPF-02 | instruction page-fault: non-executable page fetch | 4 KiB PTE: V=1, R=1, W=0, X=0, A=1, D=1; S-mode jumps to execute, triggering instruction page-fault (cause=12) | cause == 12; tval == target_pc |
| TVAL-LPF-03 | load page-fault: non-canonical address | S-mode loads from a non-canonical VA (e.g., address `0x4000000000` where bit[63:39] is inconsistent under Sv39) | cause == 13; tval == `0x4000000000` |

---

### Group 2: Access-Fault Address-Type Exceptions (stval = Faulting Virtual Address)

**Spec Reference**:
- `Sstvala_accessfault_tval_addr`: On load/store/instruction access-fault, `stval` must be written with the faulting virtual address.

**Test Scope**: Verify that when PMP restrictions cause an access-fault, `stval` equals the virtual address that was denied access. Access-faults are triggered by configuring PMP in M-mode to restrict S/U-mode access to specific regions.

**Preconditions**:
- PMP configuration: At least one entry restricting specific address regions to be non-readable/non-writable/non-executable for S-mode.
- `medeleg` delegates access-faults to S-mode (or M-mode handler captures directly).
- VM may or may not be enabled (access-faults are checked at the physical address level).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TVAL-LAF-01 | load access-fault: PMP no-read region | Configure PMP to make a specific region non-readable for S-mode; S-mode executes `ld` on this region | cause == 5 (LAF); tval == target_addr |
| TVAL-LAF-02 | load access-fault: different address verification | Same as LAF-01 but with a different target address, confirming stval tracks the actual access address | cause == 5; tval == different_addr |
| TVAL-SAF-01 | store access-fault: PMP no-write region | Configure PMP to make a specific region non-writable for S-mode; S-mode executes `sd` on this region | cause == 7 (SAF); tval == target_addr |
| TVAL-IAF-01 | instruction access-fault: PMP no-execute region | Configure PMP to make a specific region non-executable for S-mode; S-mode jumps to fetch from this region | cause == 1 (IAF); tval == target_pc |

---

### Group 3: Misaligned Address-Type Exceptions (stval = Faulting Virtual Address)

**Spec Reference**:
- `Sstvala_misaligned_tval_addr`: On load/store/instruction misaligned exceptions, `stval` must be written with the faulting virtual address.

**Test Scope**: Verify that when a load/store executes a naturally misaligned memory access triggering a misaligned exception, `stval` equals the misaligned access address.

**Preconditions**:
- Platform must not support hardware misaligned access (i.e., misaligned access triggers an exception rather than being transparently handled by hardware). If the platform supports hardware misaligned access, relevant cases should be `TEST_SKIP`-ed.
- Instruction misaligned (cause=0) is only triggered when jumping to a non-2-byte-aligned address (if C extension is supported, non-2-byte alignment is required; otherwise non-4-byte alignment is required).

> [!WARNING]
> Many modern RISC-V implementations **support hardware misaligned load/store access** in default configuration and do not trigger misaligned exceptions. Therefore, TVAL-LMA-01 and TVAL-SMA-01 will be skipped on these platforms. TVAL-IMA-01 (instruction misaligned) can typically be tested on any platform, as a non-2-byte-aligned PC always triggers an exception.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TVAL-LMA-01 | load misaligned: misaligned load | M-mode executes `ld` on a non-8-byte-aligned address (e.g., addr+3); if the platform does not support misaligned access, cause=4 is triggered | cause == 4; tval == (addr+3); `TEST_SKIP` if the platform supports misaligned access |
| TVAL-SMA-01 | store misaligned: misaligned store | M-mode executes `sd` on a non-8-byte-aligned address (e.g., addr+5); if the platform does not support misaligned access, cause=6 is triggered | cause == 6; tval == (addr+5); `TEST_SKIP` if the platform supports misaligned access |
| TVAL-IMA-01 | instruction misaligned: jump to odd address | Use `jalr` to jump to an odd address (e.g., `target \| 1`), triggering instruction address misaligned (cause=0) | cause == 0; tval == (target \| 1) |

---

### Group 4: Breakpoint Exceptions (stval = Faulting Address, Non-EBREAK)

**Spec Reference**:
- `Sstvala_breakpoint_tval_addr`: For breakpoint exceptions (cause=3), if not triggered by EBREAK/C.EBREAK instructions and the specification defines that an address should be written to stval, then `stval` must be written with the faulting address.

**Test Scope**: Verify that for breakpoint exceptions triggered by hardware triggers (Sdtrig extension), `stval` is written with the breakpoint hit address.

**Preconditions**:
- Platform must implement the Sdtrig extension (Debug Trigger module), providing `tselect`, `tdata1`, `tdata2` and related CSRs.
- If the platform does not implement the trigger module, all test cases are `TEST_SKIP`-ed.

> [!NOTE]
> The Sdtrig extension is not available on all platforms. If the platform does not support triggers, all test cases in this group are automatically skipped without affecting the overall test conclusion. Breakpoints triggered by EBREAK/C.EBREAK are explicitly excluded by the specification and are not within the scope of this group.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TVAL-BKP-01 | trigger breakpoint: address match load | Configure trigger to fire a load breakpoint at a specific address (type=6 mcontrol6, load=1); M-mode executes a load to this address | cause == 3; tval == trigger_addr |
| TVAL-BKP-02 | trigger breakpoint: address match instruction | Configure trigger to fire an execute breakpoint at a specific PC (execute=1); execute to that PC | cause == 3; tval == trigger_pc |
| TVAL-BKP-03 | EBREAK exclusion verification (negative test) | Execute the EBREAK instruction, verify that a breakpoint is triggered but **do not assert** stval to any specific value (Sstvala does not constrain EBREAK's stval) | cause == 3; stval value not asserted (logged only) |

---

### Group 5: Illegal Instruction Instruction-Type Exceptions (stval = Faulting Instruction Encoding)

**Spec Reference**:
- `Sstvala_illegal_inst_tval_inst`: On illegal-instruction exceptions, `stval` must be written with the faulting instruction encoding.

**Test Scope**: Verify that when executing an illegal instruction triggers an illegal-instruction exception (cause=2), `stval` equals the encoding value of the faulting instruction (32-bit or 16-bit instruction, zero-extended to XLEN).

**Preconditions**:
- Pre-place illegal instruction encodings with known values in memory.
- Use the framework's execution helper to jump to that address for execution, or directly embed illegal instructions via inline asm.

> [!IMPORTANT]
> Instruction encoding verification is another core part of Sstvala testing. For 32-bit instructions, `stval` should be the complete 32-bit encoding (zero-extended to XLEN width); for 16-bit compressed illegal instructions, `stval` should be the 16-bit encoding (zero-extended to XLEN width).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TVAL-ILL-01 | 32-bit illegal instruction: custom-0 opcode | Pre-place `0x0000000B` in memory (bits[1:0]=11 indicates 32-bit instruction, opcode[6:0]=0001011 i.e. custom-0, typically unimplemented); jump to execute | cause == 2; tval == `0x0000000B` |
| TVAL-ILL-02 | 32-bit illegal instruction: write to read-only CSR | Pre-place `0xC0001073` in memory (`csrrw x0, 0xC00, x0`, writing to read-only CSR `cycle`); jump to execute | cause == 2; tval == `0xC0001073` |
| TVAL-ILL-03 | 32-bit illegal instruction: access non-existent CSR | Pre-place `0xFFF022F3` in memory (`csrrs x5, 0xFFF, x0`, accessing non-existent CSR 0xFFF); jump to execute | cause == 2; tval == `0xFFF022F3` |
| TVAL-ILL-04 | 16-bit compressed illegal instruction | Pre-place the 16-bit all-zeros encoding `0x0000` in memory (in C extension, all-zeros is an illegal compressed instruction, bits[1:0]=00 indicates 16-bit instruction); jump to execute | cause == 2; tval == `0x0000` (zero-extended to XLEN) |
| TVAL-ILL-05 | Two consecutive illegal instructions with different stval | Sequentially execute `0x0000000B` (custom-0) and `0xC0001073` (write to read-only CSR), two different illegal instructions; verify stval corresponds to the actual instruction each time | 1st: tval == `0x0000000B`; 2nd: tval == `0xC0001073` |

---

### Group 6: Virtual Instruction Instruction-Type Exceptions (stval = Faulting Instruction Encoding, Optional)

> **[Migrated]** The 3 tests in this group have been migrated to [`Hypervisor_cross_test_plan_en.md`](./Hypervisor_cross_test_plan_en.md) **Group 2 (Hypervisor × Sstvala Cross Tests)**, ID mapping: TVAL-VI-01~03 → HCROSS-SSTVALA-06~08. This document retains the original descriptions for reference; actual implementation and execution follow the cross-test plan.

**Spec Reference**:
- `Sstvala_virtual_inst_tval_inst`: On virtual-instruction exceptions, `stval` must be written with the faulting instruction encoding.

**Test Scope**: Verify that when VS-mode executes CSR instructions requiring HS privilege, triggering a virtual-instruction exception (cause=22), `stval` equals the encoding of the faulting instruction.

**Preconditions**:
- Platform must implement the H extension.
- Tests run in VS-mode, executing HS-level CSR accesses (e.g., `hgatp`, `hstatus`).

> [!WARNING]
> This is an **optional test group**, compiled and executed only under a build configuration with the H extension enabled. If the H extension is not enabled, the entire group is `TEST_SKIP`-ed.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| TVAL-VI-01 | virtual-instruction: VS-mode reads hstatus | VS-mode executes `csrrs x5, hstatus, x0` (CSR 0x600), triggering virtual-instruction (cause=22) | cause == 22; tval == `0x600022F3` |
| TVAL-VI-02 | virtual-instruction: VS-mode writes hgatp | VS-mode executes `csrrw x0, hgatp, x0` (CSR 0x680), triggering virtual-instruction (cause=22) | cause == 22; tval == `0x68001073` |
| TVAL-VI-03 | virtual-instruction: VS-mode reads hideleg | VS-mode executes `csrrs x5, hideleg, x0` (CSR 0x603), triggering virtual-instruction (cause=22) | cause == 22; tval == `0x603022F3` |

> [!NOTE]
> **TVAL-VI-03 CSR Selection Rationale**: The original design considered using `vsstatus` (CSR 0x200), but in VS-mode, `vsstatus` is actually a transparent alias of `sstatus`, which VS-mode can access normally and would not trigger a virtual-instruction exception. Therefore, `hideleg` (CSR 0x603) was chosen instead — this is an HS-level CSR that always triggers a virtual-instruction when accessed from VS-mode.

> [!NOTE]
> **Instruction Encoding Derivation**:
> - `csrrs x5, 0x600, x0`: `[31:20]=0x600 [19:15]=00000 [14:12]=010 [11:7]=00101 [6:0]=1110011` = `0x600022F3`
> - `csrrw x0, 0x680, x0`: `[31:20]=0x680 [19:15]=00000 [14:12]=001 [11:7]=00000 [6:0]=1110011` = `0x68001073`
> - `csrrs x5, 0x603, x0`: `[31:20]=0x603 [19:15]=00000 [14:12]=010 [11:7]=00101 [6:0]=1110011` = `0x603022F3`

---

## Exception Cause Value Reference

| Name | Value | Description | stval Meaning (Sstvala) |
|------|-------|-------------|-------------------------|
| Instruction address misaligned | 0 | Instruction address misaligned | Faulting virtual address |
| Instruction access fault | 1 | Instruction access fault (PMP/PMA) | Faulting virtual address |
| Illegal instruction | 2 | Illegal instruction | Faulting instruction encoding |
| Breakpoint | 3 | Breakpoint (when non-EBREAK) | Faulting address |
| Load address misaligned | 4 | Load address misaligned | Faulting virtual address |
| Load access fault | 5 | Load access fault (PMP/PMA) | Faulting virtual address |
| Store address misaligned | 6 | Store address misaligned | Faulting virtual address |
| Store access fault | 7 | Store access fault (PMP/PMA) | Faulting virtual address |
| Instruction page fault | 12 | Instruction page-fault | Faulting virtual address |
| Load page fault | 13 | Load page-fault | Faulting virtual address |
| Store page fault | 15 | Store page-fault | Faulting virtual address |
| Virtual instruction | 22 | Virtual instruction (H extension) | Faulting instruction encoding |

---

## Test Case Overview

| Group | Test Count | Exception Type | stval Semantics | Dependency |
|-------|------------|----------------|-----------------|------------|
| Group 1 | 7 | Page-Fault (cause 12/13/15) | Faulting virtual address | VM (Sv39) |
| Group 2 | 4 | Access-Fault (cause 1/5/7) | Faulting virtual address | PMP |
| Group 3 | 3 | Misaligned (cause 0/4/6) | Faulting virtual address | Platform-dependent |
| Group 4 | 3 | Breakpoint (cause 3) | Faulting address | Sdtrig (optional) |
| Group 5 | 5 | Illegal Instruction (cause 2) | Faulting instruction encoding | None |
| Group 6 | 3 | Virtual Instruction (cause 22) | Faulting instruction encoding | H extension (optional, **migrated**) |
| **Total** | **25** | | | |

---

## Appendix A: Specification Point Coverage Matrix

| Norm ID | Covered Test IDs | Coverage Status | Notes |
|---------|------------------|-----------------|-------|
| `norm:sstvala_stval_faulting_vaddr` | TVAL-LPF-01 ~ TVAL-LPF-03, TVAL-SPF-01 ~ TVAL-SPF-02, TVAL-IPF-01 ~ TVAL-IPF-02, TVAL-LAF-01 ~ TVAL-LAF-02, TVAL-SAF-01, TVAL-IAF-01, TVAL-LMA-01, TVAL-SMA-01, TVAL-IMA-01, TVAL-BKP-01, TVAL-BKP-02 | Covered | Address-type exceptions: stval = faulting virtual address |
| `norm:sstvala_stval_faulting_instruction` | TVAL-ILL-01 ~ TVAL-ILL-05, TVAL-VI-01 ~ TVAL-VI-03 | Covered | Instruction-type exceptions: stval = faulting instruction encoding |
| `Sstvala_pagefault_tval_addr` | TVAL-LPF-01 ~ TVAL-LPF-03, TVAL-SPF-01 ~ TVAL-SPF-02, TVAL-IPF-01 ~ TVAL-IPF-02 | Covered | page-fault grouping |
| `Sstvala_accessfault_tval_addr` | TVAL-LAF-01 ~ TVAL-LAF-02, TVAL-SAF-01, TVAL-IAF-01 | Covered | access-fault grouping |
| `Sstvala_misaligned_tval_addr` | TVAL-LMA-01, TVAL-SMA-01, TVAL-IMA-01 | Covered | misaligned grouping |
| `Sstvala_breakpoint_tval_addr` | TVAL-BKP-01, TVAL-BKP-02, TVAL-BKP-03 | Covered | breakpoint grouping (non-EBREAK) |
| `Sstvala_illegal_inst_tval_inst` | TVAL-ILL-01 ~ TVAL-ILL-05 | Covered | illegal-instruction grouping |
| `Sstvala_virtual_inst_tval_inst` | TVAL-VI-01 ~ TVAL-VI-03 | Covered (migrated) | virtual-instruction grouping; actual implementation see `Hypervisor_cross_test_plan_en.md` Group 2 |
