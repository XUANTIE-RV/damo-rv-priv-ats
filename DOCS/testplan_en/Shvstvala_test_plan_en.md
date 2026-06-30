**[中文](../testplan/Shvstvala_test_plan.md) | English**

# Shvstvala Extension Test Plan

This document describes the test plan for the Shvstvala (Trap Value Reporting for VS-mode, Version 1.0) extension. The Shvstvala extension specifies that if this extension is implemented, `vstval` must be written with the same value in all scenarios where `stval` is written as described by the Sstvala specification. That is: when a trap enters VS-mode, `vstval` must be written with the faulting virtual address or faulting instruction encoding, following the same requirements that Sstvala imposes on `stval`.

---

## Overview

In the RISC-V Hypervisor extension, `vstval` (CSR 0x243) is the VS-mode version of `stval`. When V=1, VS-mode accesses to `stval` actually operate on `vstval`. When a trap is delegated to VS-mode (via `hedeleg`/`hideleg`), hardware writes `vsepc`, `vscause`, and `vstval`.

The base H extension specification (`norm:vstval_warl`) allows `vstval` to be written as 0 in certain scenarios, which prevents the guest OS from obtaining meaningful trap diagnostic information.

**Core constraint of the Shvstvala extension**:

> `vstval` must be written in all scenarios described for `stval` by the Sstvala specification (`SPEC/sstvala.adoc`):
> 1. **Address-class exceptions** (page-fault, access-fault, misaligned, breakpoint other than EBREAK) → `vstval` = faulting virtual address
> 2. **Instruction-class exceptions** (illegal-instruction) → `vstval` = faulting instruction encoding

This constraint ensures that when the guest OS handles traps in VS-mode, it receives diagnostic information of the same quality as Sstvala provides in S-mode.

---

## Test Scope

### Specification Sources

- `SPEC/shvstvala.adoc` — Shvstvala Extension for Trap Value Reporting, Version 1.0
- `SPEC/sstvala.adoc` — Sstvala Extension for Trap Value Reporting, Version 1.0 (Shvstvala references Sstvala's write scenario definitions)
- `SPEC/hypervisor.adoc` lines 1364–1380 — `vstval` register definition
- `SPEC/hypervisor.adoc` lines 2549–2554 — Writing `vstval` when trap enters VS-mode

### Key Reference Files

| Path | Description |
|------|-------------|
| `SPEC/shvstvala.adoc` | Full text of the Shvstvala specification (6 lines total) |
| `SPEC/sstvala.adoc` | Full text of the Sstvala specification, defining scenarios where stval must be written (referenced by Shvstvala) |
| `SPEC/hypervisor.adoc:1364-1380` | `vstval` register specification: VSXLEN-bit RW WARL, substitutes for `stval` when V=1 |
| `SPEC/hypervisor.adoc:2549-2554` | `norm:H_trap_vs_csrwrites`: Writes `vsepc`, `vscause`, `vstval` when trap enters VS-mode |
| `common/encoding.h:293` | `CSR_VSTVAL = 0x243` |
| `common/encoding.h:143` | `CSR_STVAL = 0x143` |
| `common/hyp/hyp_priv.h:21,24` | `run_in_vs_mode(fn, arg)` / `run_in_vu_mode(fn, arg)` |
| `common/hyp/hyp_csr.h:124` | `hyp_delegate_to_vs(hedeleg_mask, hideleg_mask)` |
| `common/hyp/hyp_test.h` | Hypervisor test macros |
| `common/hyp/hyp_trap.h` | HS-mode trap handler, captures stval (from VS/VU traps) |
| `common/hyp/hyp_reset.c:102` | `hyp_reset_state()` resets `vstval` to 0 |
| `DOCS/framework/hypervisor_framework.md` | Hypervisor test framework overview |
| `DOCS/testplan/sstvala_test_plan.md` | Sibling S-mode stval test plan, structural reference baseline |

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:shvstvala_vstval_written` | If the Shvstvala extension is implemented, `vstval` must be written in all cases described for `stval`. |
| `norm:sstvala_stval_faulting_vaddr` | If the Sstvala extension is implemented, then `stval` must be written with the faulting virtual address for load, store, and instruction page-fault, access-fault, and misaligned exceptions, and for breakpoint exceptions that are defined to write an address to stval, other than those caused by execution of the `EBREAK` or `C.EBREAK` instructions. |
| `norm:sstvala_stval_faulting_instruction` | For virtual-instruction and illegal-instruction exceptions, `stval` must be written with the faulting instruction. |
| `norm:vstval_sz_acc_op` | The `vstval` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `stval`. When V=1, `vstval` substitutes for the usual `stval`. When V=0, `vstval` does not directly affect the behavior of the machine. |
| `norm:vstval_warl` | `vstval` is a WARL register that must be able to hold the same set of values that `stval` can hold. |
| `norm:H_trap_vs_csrwrites` | When a trap is taken into VS-mode, `vsstatus`.SPP is set accordingly. Register `hstatus` and the HS-level `sstatus` are not modified, and V remains 1. A trap into VS-mode also writes SPIE and SIE in `vsstatus` and writes CSRs `vsepc`, `vscause`, and `vstval`. |

> [!IMPORTANT]
> The core of the Shvstvala specification is **referencing Sstvala's write scenario definitions**, extending its constraints from `stval` (S-mode trap) to `vstval` (VS-mode trap). All test cases revolve around "triggering a trap within VS-mode → verifying the `vstval` value". The key distinction is: the trap must be delegated to VS-mode (via `hedeleg`), so that hardware writes `vstval` instead of `stval`.

### Out of Scope

- **`stval` S-mode behavior**: Covered by the Sstvala test plan (`DOCS/testplan/sstvala_test_plan.md`)
- **`htval` (Hypervisor trap value)**: Covered by the Shtvala test plan (`DOCS/testplan/shtvala_test_plan.md`)
- **Guest page-faults (cause 20/21/23)**: These traps are not delegated to VS-mode (VS-mode is unaware of G-stage); handled by HS-mode
- **Breakpoints caused by EBREAK / C.EBREAK**: Explicitly excluded by the Sstvala specification
- **Multi-hart scenarios**: Project uses single-core test environment
- **Sv32 / Sv48 / Sv57 modes**: Only covers RV64 + VS-stage Sv39
- **Direct read/write of vstval when V=0**: vstval does not directly affect machine behavior when V=0; Group 5 only verifies pass-through semantics

---

## Design Key Points

### 1. Mechanism for Delegating Traps to VS-mode

The prerequisite for Shvstvala verification is that traps must **enter VS-mode**, so that hardware writes `vstval`. Delegation chain:

```
M-mode ──medeleg──> HS-mode ──hedeleg──> VS-mode
```

- `medeleg`: Delegates exceptions from M-mode to HS-mode
- `hedeleg`: Further delegates exceptions from HS-mode to VS-mode

For each exception type, the corresponding bit in `hedeleg` must be set to 1:
- page-fault (12/13/15): `hedeleg` bits 12, 13, 15
- access-fault (1/5/7): `hedeleg` bits 1, 5, 7
- misaligned (0/4/6): `hedeleg` bits 0, 4, 6
- illegal-instruction (2): `hedeleg` bit 2
- breakpoint (3): `hedeleg` bit 3

### 2. Trap Capture and vstval Reading within VS-mode

When a trap is delegated to VS-mode:
- Hardware writes `vscause`, `vsepc`, `vstval`
- Jumps to the trap entry pointed to by `vstvec`

Verification approach: Set up a custom trap handler in VS-mode (via `vstvec`/stvec when V=1). This handler:
1. Reads `stval` (actually reads `vstval` when V=1) → saves to global variable `g_shvstvala_vstval`
2. Reads `scause` (actually reads `vscause` when V=1) → saves to `g_shvstvala_cause`
3. Advances `sepc` (actually writes `vsepc` when V=1) to skip the faulting instruction
4. Returns via `sret`

### 3. VS-mode Trap Entry Design

### 4. VS-stage Page Table Configuration

Page-fault tests within VS-mode require VS-stage page tables (`vsatp`):
- Use `two_stage_run_in_vs()` or manually configure `vsatp` + `hgatp` two-stage translation
- VS-stage: Sv39 page table, some VAs unmapped or permission-restricted → triggers VS-level page-fault
- G-stage: Sv39x4 identity mapping, ensuring GPA→PA translates correctly

### 5. Triggering Access-Fault in VS-mode

Methods to trigger access-fault in VS-mode:
- **PMP restrictions**: PMP operates at the M-mode level and is equally effective for VS-mode. Configure PMP to prohibit read/write/execute on specific PA regions; when VS-mode accesses such regions, an access-fault is triggered
- Prerequisite: The final PA after VS-stage + G-stage translation falls within a PMP-prohibited region

### 6. Triggering Illegal Instruction in VS-mode

Executing an illegal instruction in VS-mode (e.g., unimplemented opcode, writing to a read-only CSR) triggers an illegal-instruction exception (cause=2). When `hedeleg` bit 2 = 1, this exception is delegated to VS-mode, and `vstval` should be written with the faulting instruction encoding.

### 7. Symmetry with Sstvala Test Plan

The Group structure of this test plan is symmetric with `sstvala_test_plan.md`, with the following main differences:
- Execution environment changes from S-mode to VS-mode
- Trap target changes from S-mode handler to VS-mode handler (custom vstvec)
- Verified CSR changes from `stval` to `vstval` (accessed via stval instruction name when V=1)
- Additional hedeleg delegation configuration and G-stage page tables are required

---

## Test Groups

> [!IMPORTANT]
> A total of 6 test groups with 22 test cases. All tests trigger exceptions within VS-mode (V=1); exceptions are delegated to VS-mode via `hedeleg` and captured by a custom VS-mode trap handler to read `vstval`. Assertions are executed after returning to M/HS-mode.

---

### Group 1: Page-Fault Address-Class Exceptions (vstval = Faulting Virtual Address)

**Specification basis**:
- `norm:shvstvala_vstval_written` (`SPEC/shvstvala.adoc:4-6`): vstval must be written in all scenarios described for stval by Sstvala
- `norm:sstvala_stval_faulting_vaddr` (`SPEC/sstvala.adoc:4-9`): Write faulting virtual address on page-fault

**Test responsibility**: Verify that in VS-mode, when VS-stage translation triggers a page-fault, `vstval` equals the guest virtual address that triggered the exception.

**Prerequisites**:
- `hedeleg` delegates page-faults (bits 12/13/15) to VS-mode
- VS-stage page table (vsatp = Sv39) configuration: code/stack regions identity-mapped, test target VA unmapped or permission-restricted
- G-stage page table (hgatp = Sv39x4) identity-mapped, ensuring GPA→PA translates correctly
- Custom VS-mode trap handler set via vstvec

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSTVAL-LPF-01 | load page-fault: unmapped VA | VS-mode executes `ld` on a VS-stage unmapped VA, triggering load page-fault (cause=13) | `g_shvstvala_cause == 13`; `g_shvstvala_vstval == unmapped_va` |
| VSTVAL-SPF-01 | store page-fault: write to read-only page | VS-stage PTE: V=1, R=1, W=0; VS-mode executes `sd` on this VA, triggering store page-fault (cause=15) | `g_shvstvala_cause == 15`; `g_shvstvala_vstval == test_va` |
| VSTVAL-IPF-01 | instruction page-fault: fetch from non-executable page | VS-stage PTE: V=1, R=1, W=0, X=0; VS-mode jumps to this VA for instruction fetch, triggering instruction page-fault (cause=12) | `g_shvstvala_cause == 12`; `g_shvstvala_vstval == target_pc` |
| VSTVAL-LPF-02 | load page-fault: different address verification | Same as LPF-01 but uses a different unmapped VA, confirming vstval follows the actual accessed address | `g_shvstvala_cause == 13`; `g_shvstvala_vstval == different_va` |
| VSTVAL-SPF-02 | store page-fault: unmapped VA | VS-mode executes `sd` on an unmapped VA, triggering store page-fault (cause=15) | `g_shvstvala_cause == 15`; `g_shvstvala_vstval == unmapped_va` |

---

### Group 2: Access-Fault Address-Class Exceptions (vstval = Faulting Virtual Address)

**Specification basis**:
- `norm:shvstvala_vstval_written` (`SPEC/shvstvala.adoc:4-6`)
- `norm:sstvala_stval_faulting_vaddr` (`SPEC/sstvala.adoc:4-9`): Write faulting virtual address on access-fault

**Test responsibility**: Verify that in VS-mode, when PMP restrictions cause an access-fault, `vstval` equals the guest virtual address whose access was denied.

**Prerequisites**:
- `hedeleg` delegates access-faults (bits 1/5/7) to VS-mode
- PMP configuration: Specific PA regions prohibited for S/VS-mode access
- Two-stage translation ensures the final PA falls within a PMP-prohibited region

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSTVAL-LAF-01 | load access-fault: PMP no-read region | PMP prohibits reading from a specific PA; VS-mode maps VA→GPA→PA to this region and executes `ld` | `g_shvstvala_cause == 5`; `g_shvstvala_vstval == target_va` |
| VSTVAL-SAF-01 | store access-fault: PMP no-write region | PMP prohibits writing to a specific PA; VS-mode executes `sd` on this region | `g_shvstvala_cause == 7`; `g_shvstvala_vstval == target_va` |
| VSTVAL-IAF-01 | instruction access-fault: PMP no-execute region | PMP prohibits execution from a specific PA; VS-mode jumps to this region for instruction fetch | `g_shvstvala_cause == 1`; `g_shvstvala_vstval == target_pc` |

---

### Group 3: Misaligned Address-Class Exceptions (vstval = Faulting Virtual Address)

**Specification basis**:
- `norm:shvstvala_vstval_written` (`SPEC/shvstvala.adoc:4-6`)
- `norm:sstvala_stval_faulting_vaddr` (`SPEC/sstvala.adoc:4-9`): Write faulting virtual address on misaligned exceptions

**Test responsibility**: Verify that in VS-mode, when load/store/instruction misaligned exceptions are triggered and delegated to VS-mode, `vstval` equals the misaligned access address.

**Prerequisites**:
- `hedeleg` delegates misaligned exceptions (bits 0/4/6) to VS-mode
- Platform does not support hardware unaligned access (otherwise load/store misaligned cases are skipped)
- Instruction misaligned triggered by jumping to an odd address

> [!WARNING]
> Most modern RISC-V implementations support hardware unaligned load/store access in default configurations. VSTVAL-LMA-01 and VSTVAL-SMA-01 will be skipped on these platforms. VSTVAL-IMA-01 (instruction misaligned) can typically be tested on any platform.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSTVAL-IMA-01 | instruction misaligned: jump to odd address | VS-mode uses `jalr` to jump to an odd address, triggering instruction address misaligned (cause=0) | `g_shvstvala_cause == 0`; `g_shvstvala_vstval == (target \| 1)` |
| VSTVAL-LMA-01 | load misaligned: unaligned load | VS-mode executes `ld` on a non-8-byte-aligned address; if platform does not support unaligned access, triggers cause=4 | `g_shvstvala_cause == 4`; `g_shvstvala_vstval == misaligned_addr`; if platform supports unaligned access, `TEST_SKIP` |
| VSTVAL-SMA-01 | store misaligned: unaligned store | VS-mode executes `sd` on a non-8-byte-aligned address; if platform does not support unaligned access, triggers cause=6 | `g_shvstvala_cause == 6`; `g_shvstvala_vstval == misaligned_addr`; if platform supports unaligned access, `TEST_SKIP` |

---

### Group 4: Illegal Instruction Instruction-Class Exceptions (vstval = Faulting Instruction Encoding)

**Specification basis**:
- `norm:shvstvala_vstval_written` (`SPEC/shvstvala.adoc:4-6`)
- `norm:sstvala_stval_faulting_instruction` (`SPEC/sstvala.adoc:11-13`): Write faulting instruction encoding on illegal-instruction exceptions

**Test responsibility**: Verify that when an illegal instruction is executed in VS-mode, triggering an illegal-instruction exception (cause=2) delegated to VS-mode, `vstval` equals the encoding of the faulting instruction.

**Prerequisites**:
- `hedeleg` bit 2 = 1, delegating illegal-instruction to VS-mode
- Pre-placed illegal instruction with known encoding in VS-mode, jump to execute

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSTVAL-ILL-01 | 32-bit illegal instruction: custom-0 opcode | VS-mode executes `0x0000000B` (custom-0), triggering illegal-instruction (cause=2) | `g_shvstvala_cause == 2`; `g_shvstvala_vstval == 0x0000000B` |
| VSTVAL-ILL-02 | 32-bit illegal instruction: write to read-only CSR | VS-mode executes `0xC0001073` (`csrrw x0, cycle, x0`, writing to read-only CSR), triggering cause=2 | `g_shvstvala_cause == 2`; `g_shvstvala_vstval == 0xC0001073` |
| VSTVAL-ILL-03 | 32-bit illegal instruction: access non-existent CSR | VS-mode executes `0xFFF022F3` (`csrrs x5, 0xFFF, x0`), triggering cause=2 | `g_shvstvala_cause == 2`; `g_shvstvala_vstval == 0xFFF022F3` |
| VSTVAL-ILL-04 | 16-bit compressed illegal instruction | VS-mode executes all-zero 16-bit encoding `0x0000` (illegal compressed instruction), triggering cause=2 | `g_shvstvala_cause == 2`; `g_shvstvala_vstval == 0x0000` |
| VSTVAL-ILL-05 | Two consecutive illegal instructions with different vstval | VS-mode sequentially executes `0x0000000B` and `0xFFF022F3`, verifying vstval follows each time | 1st time `vstval == 0x0000000B`; 2nd time `vstval == 0xFFF022F3` |

> [!NOTE]
> **Instruction encoding notes**:
> - `0x0000000B`: bits[1:0]=11 indicates 32-bit instruction, opcode[6:0]=0001011 is custom-0 opcode (typically unimplemented)
> - `0xC0001073`: `csrrw x0, 0xC00, x0` — writes to read-only CSR `cycle` (0xC00)
> - `0xFFF022F3`: `csrrs x5, 0xFFF, x0` — accesses non-existent CSR 0xFFF
> - `0x0000`: All-zero 16-bit encoding in C extension, defined as illegal

---

### Group 5: vstval Pass-Through Verification (stval Access Operates on vstval When V=1)

**Specification basis**:
- `norm:vstval_sz_acc_op` (`SPEC/hypervisor.adoc:1366-1372`): When V=1, `vstval` substitutes for the usual `stval`
- `norm:vstval_warl` (`SPEC/hypervisor.adoc:1374-1376`): vstval is WARL and must be able to hold the same set of values as stval

**Test responsibility**: Verify that values written via the `stval` instruction name when V=1 can be read back from M/HS-mode via `vstval` (CSR 0x243); verify that vstval values after a trap are not overwritten by operations outside VS-mode.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSTVAL-TRANS-01 | VS-mode writes stval → M-mode reads vstval | VS-mode writes `stval = 0xDEADBEEF`; after returning to M-mode, read `vstval` (CSR 0x243) | `vstval == 0xDEADBEEF` |
| VSTVAL-TRANS-02 | M-mode writes vstval → VS-mode reads stval | M-mode writes `vstval = 0xCAFE0000`; after entering VS-mode, read `stval` (V=1) | VS-mode readback == `0xCAFE0000` |
| VSTVAL-TRANS-03 | vstval not cleared after VS-mode trap | VS-mode triggers a trap; VS-mode handler reads vstval; after returning to M-mode, read vstval again to confirm value is not overwritten | M-mode readback of vstval == expected value (fault information written during trap) |
| VSTVAL-TRANS-04 | vstval holds address-width values | M-mode writes vstval = high-address value (e.g., `0x7FFFFFFFFF`); read back to confirm WARL does not truncate | Readback == written value |

---

### Group 6: Breakpoint Scenarios (Breakpoint Exceptions Other Than EBREAK)

**Specification basis**:
- `norm:sstvala_stval_faulting_vaddr` (`SPEC/sstvala.adoc:4-9`): stval must be written with the faulting virtual address for ... breakpoint exceptions that are defined to write an address to stval, other than those caused by execution of the `EBREAK` or `C.EBREAK` instructions.
- `norm:shvstvala_vstval_written` (`SPEC/shvstvala.adoc:4-6`): vstval must be written in all scenarios described for stval by Sstvala

**Test responsibility**: Verify that when VS-mode triggers an address-match breakpoint (non-EBREAK), vstval is correctly written with the breakpoint address; verify that breakpoints triggered by EBREAK/C.EBREAK are not subject to Shvstvala's write requirements.

**Prerequisites**:
- Sdtrig (Debug Trigger) extension must be implemented; otherwise VSTVAL-BRK-01 should `TEST_SKIP`
- `hedeleg` bit 3 (breakpoint) configured for delegation to VS-mode

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| VSTVAL-BRK-01 | Address-match breakpoint writes vstval | Configure trigger type 6 (mcontrol6) address-match breakpoint pointing to a specific address in VS-mode; VS-mode executes to that address, triggering breakpoint (cause=3) | `vstval == breakpoint address`; if Sdtrig is not implemented, `TEST_SKIP` |
| VSTVAL-BRK-02 | EBREAK does not require writing faulting address | VS-mode executes `EBREAK` instruction, triggering breakpoint (cause=3); check vstval | The value of vstval is not constrained by Shvstvala (may be 0 or the PC of EBREAK, depending on implementation); only verify that the trap occurs normally and cause==3 |

> [!NOTE]
> VSTVAL-BRK-01 depends on the trigger mechanism provided by the Sdtrig extension. If the platform does not support trigger type 2 or type 6 (mcontrol/mcontrol6), the test should probe supported trigger types by reading the `tinfo` CSR, and execute `TEST_SKIP` if unsupported.
> VSTVAL-BRK-02 verifies Sstvala's exclusion clause ("other than those caused by execution of the EBREAK or C.EBREAK instructions"), confirming that vstval is not constrained by Shvstvala in EBREAK scenarios.

---

## Appendix: Related Constants and API Reference

### Exception Cause Values

| Name | Value | Description | vstval Meaning (Shvstvala) |
|------|-------|-------------|----------------------------|
| `CAUSE_INST_ADDR_MISALIGN` | 0 | Instruction address misaligned | Faulting virtual address |
| `CAUSE_INST_ACCESS_FAULT` | 1 | Instruction access fault (PMP) | Faulting virtual address |
| `CAUSE_ILLEGAL_INST` | 2 | Illegal instruction | Faulting instruction encoding |
| `CAUSE_BREAKPOINT` | 3 | Breakpoint (when not EBREAK) | Faulting address |
| `CAUSE_LOAD_ADDR_MISALIGN` | 4 | Load address misaligned | Faulting virtual address |
| `CAUSE_LOAD_ACCESS_FAULT` | 5 | Load access fault (PMP) | Faulting virtual address |
| `CAUSE_STORE_ADDR_MISALIGN` | 6 | Store address misaligned | Faulting virtual address |
| `CAUSE_STORE_ACCESS_FAULT` | 7 | Store access fault (PMP) | Faulting virtual address |
| `CAUSE_INST_PAGE_FAULT` | 12 | Instruction page fault | Faulting virtual address |
| `CAUSE_LOAD_PAGE_FAULT` | 13 | Load page fault | Faulting virtual address |
| `CAUSE_STORE_PAGE_FAULT` | 15 | Store page fault | Faulting virtual address |

### CSR Definitions

| CSR Name | Number | Description |
|----------|--------|-------------|
| `vstval` | `0x243` | Virtual supervisor trap value, see `common/encoding.h` |
| `stval` | `0x143` | Supervisor trap value (accesses operate on vstval when V=1) |
| `vscause` | `0x242` | Virtual supervisor cause |
| `vsepc` | `0x241` | Virtual supervisor exception PC |
| `vstvec` | `0x205` | Virtual supervisor trap vector base |
| `hedeleg` | `0x602` | Hypervisor exception delegation |

### hedeleg Delegation Bit Configuration

| Exception Type | hedeleg bit | Description |
|----------------|-------------|-------------|
| Instruction address misaligned | 0 | cause=0 |
| Instruction access fault | 1 | cause=1 |
| Illegal instruction | 2 | cause=2 |
| Breakpoint | 3 | cause=3 |
| Load address misaligned | 4 | cause=4 |
| Load access fault | 5 | cause=5 |
| Store address misaligned | 6 | cause=6 |
| Store access fault | 7 | cause=7 |
| Instruction page fault | 12 | cause=12 |
| Load page fault | 13 | cause=13 |
| Store page fault | 15 | cause=15 |

### Test Framework API

- `run_in_vs_mode(fn, arg)`: Execute fn(arg) in VS-mode (V=1)
- `two_stage_init(ctx, vs_mode, g_mode)`: Initialize two-stage translation context
- `two_stage_identity_map_all(ctx)`: Identity-map all regions for VS-stage + G-stage
- `two_stage_run_in_vs(ctx, fn, arg)`: Enter VS-mode with two-stage page tables and execute
- `hyp_delegate_to_vs(hedeleg_mask, hideleg_mask)`: Configure exception/interrupt delegation to VS-mode
- `hyp_undelegate()`: Clear all hypervisor delegations
- `HYP_TEST_END()`: Test end macro (includes hyp_reset_state)

### Global Variables (Provided by `shvstvala/tests/shvstvala_strap.S`)

| Variable | Type | Description |
|----------|------|-------------|
| `g_shvstvala_vstval` | `volatile uintptr_t` | vstval value read by VS-mode trap handler |
| `g_shvstvala_cause` | `volatile uintptr_t` | vscause value read by VS-mode trap handler |
| `shvstvala_trap_entry` | `void(void)` | 4-byte aligned VS-mode trap entry symbol |

---

## Test Statistics

| Group | Test Count | Exception Type | vstval Semantics | Dependencies |
|-------|------------|----------------|------------------|--------------|
| Group 1 | 5 | Page-Fault (cause 12/13/15) | Faulting virtual address | VS-stage page table + hedeleg |
| Group 2 | 3 | Access-Fault (cause 1/5/7) | Faulting virtual address | PMP + hedeleg |
| Group 3 | 3 | Misaligned (cause 0/4/6) | Faulting virtual address | Platform-dependent + hedeleg |
| Group 4 | 5 | Illegal Instruction (cause 2) | Faulting instruction encoding | hedeleg |
| Group 5 | 4 | N/A (pass-through verification) | WARL semantics | H extension base |
| Group 6 | 2 | Breakpoint (cause 3) | Faulting virtual address | Sdtrig + hedeleg |
| **Total** | **22** | | | |

---

## Test Execution Notes

### Runtime Environment

- All tests require H extension support (`ENABLE_HYP=1`)
- Group 1: Requires VS-stage page table (vsatp=Sv39) + G-stage identity mapping (hgatp=Sv39x4)
- Group 2: Requires PMP configuration + two-stage identity mapping
- Group 3: Requires hedeleg delegation + platform without hardware unaligned access support (some cases optional)
- Group 4: Requires hedeleg bit 2 to delegate illegal-instruction
- Group 5: Alternating execution between M-mode and VS-mode
- Single-core environment; no IPI required

### Failure Diagnosis Guide

| Symptom | Possible Cause |
|---------|----------------|
| VSTVAL-LPF-01 fails (vstval=0) | Shvstvala not enabled; vstval written as 0 during page-fault |
| VSTVAL-LPF-01 fails (trap did not reach VS) | hedeleg bit 13 not correctly configured; trap escalated to HS-mode |
| VSTVAL-LAF-01 fails (cause≠5) | PMP configuration error or G-stage translation mapped target PA to a different region |
| VSTVAL-ILL-01 fails (vstval≠instruction encoding) | Implementation did not correctly write instruction encoding to vstval |
| VSTVAL-ILL-04 fails (vstval high bits non-zero) | 16-bit instruction encoding not correctly zero-extended to XLEN |
| VSTVAL-TRANS-01 fails | stval not correctly mapped to vstval when V=1 |
| VSTVAL-BRK-01 SKIP | Platform does not implement Sdtrig extension (tinfo probe failed); cannot configure address-match breakpoint |
| VSTVAL-BRK-01 fails (vstval≠target) | Trigger configuration error (tdata1 type/VS bit set incorrectly) or hedeleg bit 3 not delegated |
| All cases show cause mismatch | medeleg did not delegate to HS-mode, or hedeleg did not further delegate to VS-mode |
