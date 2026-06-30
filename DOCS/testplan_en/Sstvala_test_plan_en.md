**[中文](../testplan/Sstvala_test_plan.md) | English**

# Sstvala Extension Test Plan

This document describes the test plan for the Sstvala (Trap Value Reporting, Version 1.0) extension. The Sstvala extension specifies the value that the `stval` CSR must be written with under different exception types: for address-type exceptions (page-fault, access-fault, misaligned, and breakpoint other than EBREAK), `stval` must be written with the faulting virtual address; for instruction-type exceptions (illegal-instruction, virtual-instruction), `stval` must be written with the faulting instruction encoding.

---

## Test Scope

### Spec Reference

- `SPEC/sstvala.adoc` — Sstvala Extension for Trap Value Reporting, Version 1.0

### Key Reference Files

| Path | Description |
|------|------|
| `SPEC/sstvala.adoc` | Full Sstvala specification (10 lines total) |
| `common/trap.c:28` | `trap_record.tval` field, captures `mtval`/`stval` in M/S-mode trap handler |
| `common/trap.c:218,347` | M-mode handler reads `mtval`, S-mode handler reads `stval`, stored in `trap_record.tval` |
| `common/trap.c:447-448` | `trap_get_tval()` function, returns the tval value of the most recent trap |
| `common/test_framework.h:126-139` | `TEST_ASSERT_EQ` macro, for asserting value equality |
| `common/test_framework.h:169-180` | `EXPECT_TRAP` macro, triggers and verifies exceptions in M-mode |
| `common/test_framework.h:234-275` | `PRIV_DO_TRAP` / `CHECK_TRAP` macros, safe trap macros for S/U-mode |
| `common/test_framework.h:154` | `trap_get_tval()` declaration |
| `common/encoding.h:179-206` | Exception cause value definitions |
| `common/vm/vm.h` | Page table management API (`pt_init`, `pt_map_page`, `vm_run_in_smode`, etc.) |
| `common/vm/vm_defs.h` | VM constant definitions (`PTE_V`, `PTE_R`, `PTE_W`, `PTE_X`, `PAGE_SIZE_4K`, etc.) |

### Covered Specification Points

| Norm ID | Original Text |
|---------|------|
| `norm:sstvala_stval_faulting_vaddr` | If the Sstvala extension is implemented, then `stval` must be written with the faulting virtual address for load, store, and instruction page-fault, access-fault, and misaligned exceptions, and for breakpoint exceptions that are defined to write an address to stval, other than those caused by execution of the `EBREAK` or `C.EBREAK` instructions. |
| `norm:sstvala_stval_faulting_instruction` | For virtual-instruction and illegal-instruction exceptions, `stval` must be written with the faulting instruction. |

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

In the framework, `trap_record.tval` is set in both M-mode handler (`common/trap.c:264`) and S-mode handler (`common/trap.c:363`), reading `mtval` and `stval` respectively. The `trap_get_tval()` function (line 447) directly returns this value.

### 2. stval Verification for Address-Type Exceptions

For page-fault, access-fault, and misaligned exceptions, `stval` should equal the virtual address that triggered the exception. Verification requires:

- **Known target address**: Test code explicitly constructs the access target address (e.g., `TEST_REGION_BASE`), then asserts `trap_get_tval() == target_addr`
- **M-mode vs S-mode**: Page-faults require VM enabled (S-mode), access-faults can be triggered in M-mode (PMP) or S-mode, and misaligned exceptions can be triggered in any mode.

### 3. stval Verification for Instruction-Type Exceptions

For illegal-instruction exceptions, `stval` should equal the encoding of the faulting instruction. Verification:

- **Known instruction encoding**: Pre-place a known illegal instruction byte sequence in memory, then execute it or read `trap_get_tval()` after the trap to compare with the expected encoding.
- **32-bit instructions**: `stval` should be the complete 32-bit instruction encoding (zero-extended to XLEN).
- **16-bit compressed instructions**: `stval` should be the 16-bit instruction encoding (zero-extended to XLEN).

### 4. VM Configuration (Page-Fault Scenarios)

Page-fault tests require Sv39 page tables to be enabled:

- **Code/data regions**: Identity mapping, `PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D`, to ensure test code and trap handler can execute normally.
- **Test regions**: Intentionally unmapped VA or insufficient permissions (write to read-only page) to trigger page-faults.
- Use `vm_run_in_smode()` to enter S-mode execution; exceptions are caught by the S-mode trap handler.

### 5. PMP Configuration (Access-Fault Scenarios)

Access-faults are triggered via PMP restrictions:

- Configure PMP entries to make specific address regions non-readable/non-writable/non-executable for S/U-mode.
- Access the restricted region in S-mode or U-mode to trigger an access-fault.
- Verify `stval` == the address that was denied access.

### 6. EBREAK Exclusion

The specification explicitly excludes EBREAK/C.EBREAK caused breakpoints. The `stval` behavior for EBREAK is defined by the base privileged specification (typically 0 or PC) and is not within Sstvala test scope. Group 4 only tests breakpoint exceptions triggered by the trigger module (if implemented) or other mechanisms.

> [!NOTE]
> If the platform does not implement the trigger module (Sdtrig extension), Group 4 test cases will be `TEST_SKIP`-ed, without affecting the overall test conclusion.

### 7. Virtual Instruction Exception Notes

Virtual-instruction exceptions (cause=22) require H extension (Hypervisor) support, triggered when executing certain HS-level CSR access instructions in VS-mode. The 3 tests in Group 6 have been migrated to [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) Group 2 (IDs HCROSS-SSTVALA-06~08), implemented in the `Hypervisor_Sstvala/` project. This document retains the Group 6 description for reference.

---

## Test Groups

> [!IMPORTANT]
> A total of 6 test groups and 25 test cases. Groups 1-3 are core address-type exception tests, Group 4 is breakpoint (conditional), Group 5 is core instruction-type exception tests, and Group 6 is optional (requires H extension).

---

### Group 1: Page-Fault Address-Type Exceptions (stval = Faulting Virtual Address)

**Spec Reference**:
- `norm:Sstvala_pagefault_tval_addr` (`SPEC/sstvala.adoc:4-6`): On load/store/instruction page-fault, `stval` must be written with the faulting virtual address.

**Test Scope**: Verify that in Sv39 virtual memory mode, when S-mode accesses an unmapped or insufficient-permission virtual address triggering a page-fault, `stval` (captured equivalently via M-mode `mtval`) equals the faulting virtual address.

**Preconditions**:
- Sv39 page table enabled, code/stack regions identity-mapped (`PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D`)
- Target test virtual address unmapped or with restricted permissions
- M-mode `medeleg` delegates page-faults to S-mode (or M-mode handler captures directly)

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| TVAL-LPF-01 | load page-fault: unmapped VA | Under Sv39, S-mode executes `ld` on an unmapped VA (e.g., `0x40000000`), triggering load page-fault (cause=13) | `trap_get_cause() == 13`; `trap_get_tval() == 0x40000000` |
| TVAL-LPF-02 | load page-fault: read from writable page (negative test) | 4 KiB PTE: V=1, W=1, R=1, X=0, A=1, D=1; S-mode load should succeed | No exception (negative test, confirming correct mapping does not trigger fault) |
| TVAL-SPF-01 | store page-fault: write to read-only page | 4 KiB PTE: V=1, R=1, W=0, A=1, D=1; S-mode executes `sd` on this VA, triggering store page-fault (cause=15) | `trap_get_cause() == 15`; `trap_get_tval() == test_va` |
| TVAL-SPF-02 | store page-fault: unmapped VA | S-mode executes `sd` on an unmapped VA, triggering store page-fault (cause=15) | `trap_get_cause() == 15`; `trap_get_tval() == unmapped_va` |
| TVAL-IPF-01 | instruction page-fault: unmapped VA fetch | S-mode jumps to an unmapped VA for instruction fetch, triggering instruction page-fault (cause=12) | `trap_get_cause() == 12`; `trap_get_tval() == target_pc` |
| TVAL-IPF-02 | instruction page-fault: non-executable page fetch | 4 KiB PTE: V=1, R=1, W=0, X=0, A=1, D=1; S-mode jumps to execute, triggering instruction page-fault (cause=12) | `trap_get_cause() == 12`; `trap_get_tval() == target_pc` |
| TVAL-LPF-03 | load page-fault: non-canonical address | S-mode loads from a non-canonical VA (e.g., address `0x4000000000` where bit[63:39] is inconsistent under Sv39) | `trap_get_cause() == 13`; `trap_get_tval() == 0x4000000000` |

---

### Group 2: Access-Fault Address-Type Exceptions (stval = Faulting Virtual Address)

**Spec Reference**:
- `norm:Sstvala_accessfault_tval_addr` (`SPEC/sstvala.adoc:4-6`): On load/store/instruction access-fault, `stval` must be written with the faulting virtual address.

**Test Scope**: Verify that when PMP restrictions cause an access-fault, `stval` equals the virtual address that was denied access. Access-faults are triggered by configuring PMP in M-mode to restrict S/U-mode access to specific regions.

**Preconditions**:
- PMP configuration: At least one entry restricting specific address regions to be non-readable/non-writable/non-executable for S-mode.
- `medeleg` delegates access-faults to S-mode (or M-mode handler captures directly).
- VM may or may not be enabled (access-faults are checked at the physical address level).

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| TVAL-LAF-01 | load access-fault: PMP no-read region | Configure PMP to make a specific region non-readable for S-mode; S-mode executes `ld` on this region | `trap_get_cause() == 5` (LAF); `trap_get_tval() == target_addr` |
| TVAL-LAF-02 | load access-fault: different address verification | Same as LAF-01 but with a different target address, confirming stval tracks the actual access address | `trap_get_cause() == 5`; `trap_get_tval() == different_addr` |
| TVAL-SAF-01 | store access-fault: PMP no-write region | Configure PMP to make a specific region non-writable for S-mode; S-mode executes `sd` on this region | `trap_get_cause() == 7` (SAF); `trap_get_tval() == target_addr` |
| TVAL-IAF-01 | instruction access-fault: PMP no-execute region | Configure PMP to make a specific region non-executable for S-mode; S-mode jumps to fetch from this region | `trap_get_cause() == 1` (IAF); `trap_get_tval() == target_pc` |

---

### Group 3: Misaligned Address-Type Exceptions (stval = Faulting Virtual Address)

**Spec Reference**:
- `norm:Sstvala_misaligned_tval_addr` (`SPEC/sstvala.adoc:4-6`): On load/store/instruction misaligned exceptions, `stval` must be written with the faulting virtual address.

**Test Scope**: Verify that when a load/store executes a naturally misaligned memory access triggering a misaligned exception, `stval` equals the misaligned access address.

**Preconditions**:
- Platform must not support hardware misaligned access (i.e., misaligned access triggers an exception rather than being transparently handled by hardware). If the platform supports hardware misaligned access, relevant cases should be `TEST_SKIP`-ed.
- Instruction misaligned (cause=0) is only triggered when jumping to a non-2-byte-aligned address (if C extension is supported, non-2-byte alignment is required; otherwise non-4-byte alignment is required).

> [!WARNING]
> Most modern RISC-V implementations (including Spike and QEMU) **support hardware misaligned load/store access** in default configuration and do not trigger misaligned exceptions. Therefore, TVAL-LMA-01 and TVAL-SMA-01 will be skipped on these platforms. TVAL-IMA-01 (instruction misaligned) can typically be tested on any platform, as a non-2-byte-aligned PC always triggers an exception.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| TVAL-LMA-01 | load misaligned: misaligned load | M-mode executes `ld` on a non-8-byte-aligned address (e.g., addr+3); if the platform does not support misaligned access, cause=4 is triggered | `trap_get_cause() == 4`; `trap_get_tval() == (addr+3)`; `TEST_SKIP` if platform supports misaligned access |
| TVAL-SMA-01 | store misaligned: misaligned store | M-mode executes `sd` on a non-8-byte-aligned address (e.g., addr+5); if the platform does not support misaligned access, cause=6 is triggered | `trap_get_cause() == 6`; `trap_get_tval() == (addr+5)`; `TEST_SKIP` if platform supports misaligned access |
| TVAL-IMA-01 | instruction misaligned: jump to odd address | Use `jalr` to jump to an odd address (e.g., `target \| 1`), triggering instruction address misaligned (cause=0) | `trap_get_cause() == 0`; `trap_get_tval() == (target \| 1)` |

---

### Group 4: Breakpoint Exceptions (stval = Faulting Address, Non-EBREAK)

**Spec Reference**:
- `norm:Sstvala_breakpoint_tval_addr` (`SPEC/sstvala.adoc:6-8`): For breakpoint exceptions (cause=3), if not triggered by EBREAK/C.EBREAK instructions and the specification defines that an address should be written to stval, then `stval` must be written with the faulting address.

**Test Scope**: Verify that for breakpoint exceptions triggered by hardware triggers (Sdtrig extension), `stval` is written with the breakpoint hit address.

**Preconditions**:
- Platform must implement the Sdtrig extension (Debug Trigger module), providing `tselect`, `tdata1`, `tdata2` and related CSRs.
- If the platform does not implement the trigger module, all test cases are `TEST_SKIP`-ed.

> [!NOTE]
> The Sdtrig extension is not available on all platforms. Spike supports triggers by default; QEMU has partial support. If the platform does not support triggers, all test cases in this group are automatically skipped without affecting the overall test conclusion. Breakpoints triggered by EBREAK/C.EBREAK are explicitly excluded by the specification and are not within the scope of this group.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| TVAL-BKP-01 | trigger breakpoint: address match load | Configure trigger to fire a load breakpoint at a specific address (type=6 mcontrol6, load=1); M-mode executes a load to this address | `trap_get_cause() == 3`; `trap_get_tval() == trigger_addr` |
| TVAL-BKP-02 | trigger breakpoint: address match instruction | Configure trigger to fire an execute breakpoint at a specific PC (execute=1); execute to that PC | `trap_get_cause() == 3`; `trap_get_tval() == trigger_pc` |
| TVAL-BKP-03 | EBREAK exclusion verification (negative test) | Execute the EBREAK instruction, verify that a breakpoint is triggered but **do not assert** stval to any specific value (Sstvala does not constrain EBREAK's stval) | `trap_get_cause() == 3`; stval value not asserted (logged only) |

---

### Group 5: Illegal Instruction Instruction-Type Exceptions (stval = Faulting Instruction Encoding)

**Spec Reference**:
- `norm:Sstvala_illegal_inst_tval_inst` (`SPEC/sstvala.adoc:9-10`): On illegal-instruction exceptions, `stval` must be written with the faulting instruction encoding.

**Test Scope**: Verify that when executing an illegal instruction triggers an illegal-instruction exception (cause=2), `stval` equals the encoding value of the faulting instruction (32-bit or 16-bit instruction, zero-extended to XLEN).

**Preconditions**:
- Pre-place illegal instruction encodings with known values in memory.
- Use `exec_at()` to jump to that address for execution, or directly embed illegal instructions via inline asm.

> [!IMPORTANT]
> Instruction encoding verification is another core part of Sstvala testing. For 32-bit instructions, `stval` should be the complete 32-bit encoding (zero-extended to XLEN width); for 16-bit compressed illegal instructions, `stval` should be the 16-bit encoding (zero-extended to XLEN width).

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| TVAL-ILL-01 | 32-bit illegal instruction: custom-0 opcode | Pre-place `0x0000000B` in memory (bits[1:0]=11 indicates 32-bit instruction, opcode[6:0]=0001011 i.e. custom-0, typically unimplemented); jump to execute | `trap_get_cause() == 2`; `trap_get_tval() == 0x0000000B` |
| TVAL-ILL-02 | 32-bit illegal instruction: write to read-only CSR | Pre-place `0xC0001073` in memory (`csrrw x0, 0xC00, x0`, writing to read-only CSR `cycle`); jump to execute | `trap_get_cause() == 2`; `trap_get_tval() == 0xC0001073` |
| TVAL-ILL-03 | 32-bit illegal instruction: access non-existent CSR | Pre-place `0xFFF022F3` in memory (`csrrs x5, 0xFFF, x0`, accessing non-existent CSR 0xFFF); jump to execute | `trap_get_cause() == 2`; `trap_get_tval() == 0xFFF022F3` |
| TVAL-ILL-04 | 16-bit compressed illegal instruction | Pre-place the 16-bit all-zeros encoding `0x0000` in memory (in C extension, all-zeros is an illegal compressed instruction, bits[1:0]=00 indicates 16-bit instruction); jump to execute | `trap_get_cause() == 2`; `trap_get_tval() == 0x0000` (zero-extended to XLEN) |
| TVAL-ILL-05 | Two consecutive illegal instructions with different stval | Sequentially execute `0x0000000B` (custom-0) and `0xC0001073` (write to read-only CSR), two different illegal instructions; verify stval corresponds to the actual instruction each time | 1st: `trap_get_tval() == 0x0000000B`; 2nd: `trap_get_tval() == 0xC0001073` |

---

### Group 6: Virtual Instruction Instruction-Type Exceptions (stval = Faulting Instruction Encoding, Optional)

> **[Migrated]** The 3 tests in this group have been migrated to [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) **Group 2 (Hypervisor x Sstvala Cross Tests)**, ID mapping: TVAL-VI-01~03 → HCROSS-SSTVALA-06~08. This document retains the original descriptions for reference; actual implementation and execution follow the cross-test plan (located in the `Hypervisor_Sstvala/` project).

**Spec Reference**:
- `norm:Sstvala_virtual_inst_tval_inst` (`SPEC/sstvala.adoc:9-10`): On virtual-instruction exceptions, `stval` must be written with the faulting instruction encoding.

**Test Scope**: Verify that when VS-mode executes CSR instructions requiring HS privilege, triggering a virtual-instruction exception (cause=22), `stval` equals the encoding of the faulting instruction.

**Preconditions**:
- Platform must implement the H extension (`ENABLE_HYP=1`).
- Tests run in VS-mode, executing HS-level CSR accesses (e.g., `hgatp`, `hstatus`).

> [!WARNING]
> This is an **optional test group**, compiled and executed only when `ENABLE_HYP=1`. If the H extension is not enabled, the entire group is `TEST_SKIP`-ed.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| TVAL-VI-01 | virtual-instruction: VS-mode reads hstatus | VS-mode executes `csrrs x5, hstatus, x0` (CSR 0x600), triggering virtual-instruction (cause=22) | `trap_get_cause() == 22`; `trap_get_tval() == 0x600022F3` |
| TVAL-VI-02 | virtual-instruction: VS-mode writes hgatp | VS-mode executes `csrrw x0, hgatp, x0` (CSR 0x680), triggering virtual-instruction (cause=22) | `trap_get_cause() == 22`; `trap_get_tval() == 0x68001073` |
| TVAL-VI-03 | virtual-instruction: VS-mode reads hideleg | VS-mode executes `csrrs x5, hideleg, x0` (CSR 0x603), triggering virtual-instruction (cause=22) | `trap_get_cause() == 22`; `trap_get_tval() == 0x603022F3` |

> [!NOTE]
> **TVAL-VI-03 CSR Selection Rationale**: The original design considered using `vsstatus` (CSR 0x200), but in VS-mode, `vsstatus` is actually a transparent alias of `sstatus`, which VS-mode can access normally and would not trigger a virtual-instruction exception. Therefore, `hideleg` (CSR 0x603) was chosen instead — this is an HS-level CSR that always triggers a virtual-instruction when accessed from VS-mode.

> [!NOTE]
> **Instruction Encoding Derivation**:
> - `csrrs x5, 0x600, x0`: `[31:20]=0x600 [19:15]=00000 [14:12]=010 [11:7]=00101 [6:0]=1110011` = `0x600022F3`
> - `csrrw x0, 0x680, x0`: `[31:20]=0x680 [19:15]=00000 [14:12]=001 [11:7]=00000 [6:0]=1110011` = `0x68001073`
> - `csrrs x5, 0x603, x0`: `[31:20]=0x603 [19:15]=00000 [14:12]=010 [11:7]=00101 [6:0]=1110011` = `0x603022F3`

---

## Appendix: Related Constants and API Reference

### Exception Cause Values

| Name | Value | Description | stval Meaning (Sstvala) |
|------|-----|------|------|
| `CAUSE_INST_ADDR_MISALIGN` | 0 | Instruction address misaligned | Faulting virtual address |
| `CAUSE_INST_ACCESS_FAULT` | 1 | Instruction access fault (PMP/PMA) | Faulting virtual address |
| `CAUSE_ILLEGAL_INST` | 2 | Illegal instruction | Faulting instruction encoding |
| `CAUSE_BREAKPOINT` | 3 | Breakpoint (when non-EBREAK) | Faulting address |
| `CAUSE_LOAD_ADDR_MISALIGN` | 4 | Load address misaligned | Faulting virtual address |
| `CAUSE_LOAD_ACCESS_FAULT` | 5 | Load access fault (PMP/PMA) | Faulting virtual address |
| `CAUSE_STORE_ADDR_MISALIGN` | 6 | Store address misaligned | Faulting virtual address |
| `CAUSE_STORE_ACCESS_FAULT` | 7 | Store access fault (PMP/PMA) | Faulting virtual address |
| `CAUSE_INST_PAGE_FAULT` | 12 | Instruction page-fault | Faulting virtual address |
| `CAUSE_LOAD_PAGE_FAULT` | 13 | Load page-fault | Faulting virtual address |
| `CAUSE_STORE_PAGE_FAULT` | 15 | Store page-fault | Faulting virtual address |
| `CAUSE_VIRTUAL_INSTRUCTION` | 22 | Virtual instruction (H extension) | Faulting instruction encoding |

### Framework API Reference

| Function/Macro | Path | Description |
|---------|------|------|
| `trap_get_tval()` | `common/trap.c:447` | Returns the `mtval`/`stval` value of the most recent trap |
| `trap_get_cause()` | `common/trap.c:443` | Returns the cause value of the most recent trap |
| `trap_expect_begin()` | `common/trap.c:413` | Begin trap expectation (arm) |
| `trap_expect_end()` | `common/trap.c:423` | End trap expectation (disarm) |
| `trap_was_triggered()` | `common/trap.c:427` | Returns whether a trap was triggered during the last arm period |
| `EXPECT_TRAP(cause, stmt)` | `common/test_framework.h:173` | M-mode: execute stmt and assert that the specified cause is triggered |
| `EXPECT_EXEC_TRAP(cause, addr)` | `common/test_framework.h:188` | M-mode: jump to addr and assert that the specified cause is triggered |
| `PRIV_DO_TRAP(stmt)` | `common/test_framework.h:234` | S/U-mode: arm + execute + disarm (no printf) |
| `CHECK_TRAP(msg, cause)` | `common/test_framework.h:272` | M-mode: assert that the last PRIV_DO_TRAP triggered the specified cause |
| `TEST_ASSERT_EQ(msg, actual, expected)` | `common/test_framework.h:128` | Assert two values are equal |
| `vm_run_in_smode(ctx, fn, arg)` | `common/vm/satp.c` | Execute function in S-mode with VM enabled; returns scause |
| `pt_init(ctx, mode)` | `common/vm/page_table.c` | Initialize page table context |
| `pt_map_page(ctx, va, pa, flags, level)` | `common/vm/page_table.c` | Map a single page |
| `exec_at(addr)` | `common/test_framework.h` | Jump to specified address for execution (with trap recovery) |

### CSR Definitions

| CSR Name | Number | Description |
|----------|------|------|
| `stval` | `0x143` | Supervisor trap value |
| `mtval` | `0x343` | Machine trap value |
| `scause` | `0x142` | Supervisor cause register |
| `mcause` | `0x342` | Machine cause register |
| `stvec` | `0x105` | Supervisor trap vector base |
| `tselect` | `0x7A0` | Debug trigger select |
| `tdata1` | `0x7A1` | Debug trigger data 1 |
| `tdata2` | `0x7A2` | Debug trigger data 2 |

### Test Case Overview

| Group | Test Count | Exception Type | stval Semantics | Dependency |
|-------|---------|----------|------------|------|
| Group 1 | 7 | Page-Fault (cause 12/13/15) | Faulting virtual address | VM (Sv39) |
| Group 2 | 4 | Access-Fault (cause 1/5/7) | Faulting virtual address | PMP |
| Group 3 | 3 | Misaligned (cause 0/4/6) | Faulting virtual address | Platform-dependent |
| Group 4 | 3 | Breakpoint (cause 3) | Faulting address | Sdtrig (optional) |
| Group 5 | 5 | Illegal Instruction (cause 2) | Faulting instruction encoding | None |
| Group 6 | 3 | Virtual Instruction (cause 22) | Faulting instruction encoding | H extension (optional, **migrated**) |
| **Total** | **25** | | | |
