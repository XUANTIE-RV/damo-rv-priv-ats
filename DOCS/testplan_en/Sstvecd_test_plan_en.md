**[中文](../testplan/Sstvecd_test_plan.md) | English**

# Sstvecd Extension Test Plan

This document describes the test plan for the Sstvecd (Direct Trap Vectoring, Version 1.0) extension. Sstvecd is a narrow constraint extension on the `stvec` register behavior. The specification requires that implementations must guarantee: (1) `stvec.MODE` can be written and hold the value 0 (Direct); (2) when `stvec.MODE=Direct`, `stvec.BASE` must be capable of holding any 4-byte-aligned address.

---

## Test Scope

### Specification Sources

- `SPEC/sstvecd.adoc` — Sstvecd Extension for Direct Trap Vectoring, Version 1.0
- `SPEC/supervisor.adoc` lines 318-365 (`stvec` register and MODE field encoding) — provides `stvec` field layout and Direct/Vectored behavior definitions

### Key Reference Files

| Path | Description |
|------|-------------|
| `SPEC/sstvecd.adoc` | Full Sstvecd specification (6 lines total) |
| `SPEC/supervisor.adoc:318-365` | `stvec` register specification, defining BASE/MODE fields + Direct/Vectored behavior |
| `common/test_framework.c:36-38` | Default `stvec` setup to `s_trap_entry`; each test case must save/restore on entry/exit |
| `common/encoding.h:129` | `CSR_STVEC = 0x105` |
| `common/csr_accessors.c:281` | Existing `_CSR_WRITE_CASE(0x105)` generic CSR write entry |
| `common/vm/satp.c::vm_run_in_smode` | Enters S-mode to execute test function, captures exceptions and returns scause |
| `svadu/Makefile` | Subdirectory Makefile template (`SPIKE_ISA_EXT = _svadu` to enable extension); reference for implementation phase |
| `svvptc/tests/...` | Subdirectory local trap-handler implementation examples (refer to "Design Note 2: Trap Entry Placement") |

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:sstvecd_stvec_mode_direct` | If the Sstvecd extension is implemented, then `stvec.MODE` must be capable of holding the value 0 (Direct). |
| `norm:sstvecd_stvec_base_aligned_address` | Furthermore, when `stvec.MODE=Direct`, `stvec.BASE` must be capable of holding any valid four-byte-aligned address. |
| `norm:stvec_op` | The BASE field in `stvec` is a field that can hold any valid virtual or physical address, subject to the following alignment constraints: the address must be 4-byte aligned, and MODE settings other than Direct might impose additional alignment constraints on the value in the BASE field. |
| `norm:stvec_sz_base` | The CSR contains only bits XLEN-1 through 2 of the address BASE. When used as an address, the lower two bits are filled with zeroes to obtain an XLEN-bit address that is always aligned on a 4-byte boundary. |

> [!IMPORTANT]
> The Sstvecd specification itself has only two hard constraints (Direct mode must be holdable, BASE must support 4-byte-aligned address holding capability). The Group 3 trap vectoring test is an end-to-end verification that "the BASE setting takes effect and Direct mode behavior is correct." The specification basis comes from `SPEC/supervisor.adoc:341-365` (stvec MODE encoding definition) — since Sstvecd mandates Direct mode availability, it naturally requires that mode's behavior conform to the supervisor specification.

### Out of Scope

- **Functional behavior of Vectored mode**: Sstvecd does not require implementation of Vectored mode; this plan only "probes" whether Vectored is implemented in Group 1, without verifying its correctness
- **VS-mode `vstvec`**: Sstvecd constrains only `stvec`, not `vstvec`
- **M-mode `mtvec`**: Not within the scope of the Sstvecd specification
- **Multi-hart scenarios**: The project is a single-core test environment
- **Sv32 / Sv48 / Sv57 modes**: Only covers RV64 + Sv39, consistent with other extension plans in this project
- **BASE range under different SXLEN values**: Only covers SXLEN=64

---

## Design Notes

### 1. Handling Implementation Presence

The platform must enable Sstvecd via `SPIKE_ISA_EXT = _sstvecd` (refer to `svadu/Makefile` for the `SPIKE_ISA_EXT = _svadu` pattern). If not enabled, Group 1's "write MODE=0, read back=0" test will still pass on most implementations (any compliant RISC-V implementation supports at least Direct=0), so this test case cannot independently prove that Sstvecd is enabled.

To strengthen the "Sstvecd must be enabled" prerequisite, the plan **intentionally writes a high-bit (e.g., bit 38 / bit 47 / near the SXLEN upper bound) 4-byte-aligned address** in the Group 2 BASE-04 test case — if the implementation has not declared Sstvecd, it may treat the upper bits of BASE as WARL-truncated (read-only 0), thereby exposing implementation differences. This test case uses a hard assert; failure indicates the platform does not truly implement Sstvecd.

### 2. Trap Entry Placement (Group 3 Specific)

Group 3 requires a user-controllable S-mode trap entry with the following requirements:

- **4-byte aligned**: Satisfies the `stvec.BASE` alignment constraint (ensured via `__attribute__((aligned(4)))` or assembly `.align 2`)
- **Located in `.text` section**: No special linker script handling required
- **Minimal handling path**: Save `sscratch`, record hit address to global variable `g_sstvecd_trap_pc`, record `scause` to `g_sstvecd_trap_cause`, then `sepc += 4` and `sret` to return

> [!NOTE]
> During the implementation phase, refer to the approach used in `svvptc/tests/svvptc_strap.S` (the local S-mode trap entry example referenced in the svvptc plan). `g_sstvecd_trap_pc` is used to assert "trap hit address = stvec.BASE" and is the core evidence for Direct behavior verification.

### 3. Isolation from the Default Framework

`common/test_framework.c::reset_state()` sets `stvec` to `s_trap_entry` by default (line 37). The standard template for each test case in this test suite saves and restores `stvec` on entry and exit to avoid polluting subsequent tests.

No dependency on `medeleg`: sstvecd does not involve exception delegation semantics. Group 3 synchronous exceptions are triggered actively within S-mode via `vm_run_in_smode`; when triggered, the trap enters S-mode directly (rather than being delegated from M-mode), hitting the `stvec` we have configured.

### 4. Asynchronous Interrupt Triggering (Group 3 STVEC-INT-01)

To verify "in Direct mode, interrupts also vector to BASE rather than BASE+4xcause", SSIP (supervisor software interrupt) is used:

1. M-mode sets `mideleg.SSIE` (bit 1) = 1, delegating the supervisor software interrupt to S-mode
2. M-mode prepares `g_sstvecd_trap_pc = 0`, `g_sstvecd_trap_cause = 0`
3. After entering S-mode: first `csrw stvec, BASE`, then `csrs sie, 1<<1` (enable SSIE), `csrs sstatus, 1<<1` (SIE=1), and finally `csrs sip, 1<<1` to self-trigger SSIP
4. After the trap fires, assert: `g_sstvecd_trap_pc == BASE` (Direct); if the implementation uses Vectored mode, the hit address would be `BASE + 4*1 = BASE+0x4`, and the assertion failure would identify incorrect Direct behavior

> [!NOTE]
> SSIP is a supervisor-internal writable interrupt flag (`SPEC/supervisor.adoc:norm:sip_ssip_sie_ssie`), making it the most suitable choice for self-triggering in a single-core environment. STIP (timer) requires Sstc or SBI, and SEIP (external) requires PLIC/AIA, both of which are relatively complex.

### 5. WARL Write-Readback Convention

The `stvec.MODE` field is WARL:

- Writing 0 (Direct) must succeed (Sstvecd hard constraint)
- Writing 1 (Vectored): if the implementation supports it, readback is 1; if not supported, readback is some legal value (the most natural choice is 0, since Direct is always supported). Neither behavior violates Sstvecd
- Writing >= 2 (Reserved): must read back as a legal value (0 or 1); must not be latched

The `stvec.BASE` field ([XLEN-1:2]) is WARL:

- Writing a value with the low 2 bits non-zero: hardware should force the low 2 bits to 0 (`SPEC/supervisor.adoc:norm:stvec_sz_base`)
- The upper BASE bits written should read back as-is (Sstvecd requires "any valid 4-byte aligned address")

---

## Test Groups

> [!IMPORTANT]
> A total of 3 test groups and 12 test cases. All tests run under RV64 + M-mode setup, with some cases entering S-mode (Group 3). Each group provides: specification basis, test scope, test case table (ID/Name/Description/Expected Result); each group provides 1 key C code example.

---

### Group 1: `stvec.MODE` Writability

**Spec Reference**:
- `norm:Sstvecd_mode_direct_writable` (`SPEC/sstvecd.adoc:4-5`): `stvec.MODE` must be capable of holding the value 0 (Direct)

**Test Scope**: Verify that `stvec.MODE` can be stably written and hold the Direct value (0); probe whether Vectored (1) is implemented; verify that WARL behavior is reasonable after writing reserved values (>= 2).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| STVEC-MODE-01 | MODE write 0 (Direct) readback | `csrw stvec, 0` (BASE=0, MODE=0), read back `stvec[1:0]` | `stvec[1:0] == 0` (Direct must always be holdable) |
| STVEC-MODE-02 | MODE 0->1->0 toggle | Write MODE=0, readback and confirm; then write MODE=1, readback and record; finally write MODE=0, readback and confirm | 1st and 3rd readbacks `MODE==0`; 2nd readback is an element of {0, 1} (implementation-defined) |
| STVEC-MODE-03 | MODE write reserved value (=2) | `csrw stvec, 0x2` (BASE=0, MODE=2), read back `stvec[1:0]` | Readback is an element of {0, 1}, must not be 2 (WARL, reserved values must not be latched) |
| STVEC-MODE-04 | MODE write reserved value (=3) | `csrw stvec, 0x3` (BASE=0, MODE=3), read back `stvec[1:0]` | Readback is an element of {0, 1}, must not be 3 |

---

### Group 2: `stvec.BASE` Holding Capability in Direct Mode

**Spec Reference**:
- `norm:Sstvecd_base_holds_any_4byte_aligned` (`SPEC/sstvecd.adoc:5-6`): When `stvec.MODE=Direct`, `stvec.BASE` must be capable of holding any valid 4-byte-aligned address
- `norm:stvec_sz_base` (`SPEC/supervisor.adoc:335-338`): CSR stores only BASE[XLEN-1:2]; the low 2 bits are forced to 0 on write

**Test Scope**: Verify that under MODE=Direct, the BASE field can hold various 4-byte-aligned addresses (including those crossing 1 GiB / 512 GiB boundaries, near the SXLEN upper bound); verify that writing a non-4-byte-aligned address forces the low 2 bits to zero; verify that BASE and MODE writes are independent of each other.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| STVEC-BASE-01 | BASE write 0 readback | `csrw stvec, 0x0` (BASE=0, MODE=0), read back | `stvec == 0x0` |
| STVEC-BASE-02 | BASE write PLATFORM_MEM_BASE readback | `csrw stvec, PLATFORM_MEM_BASE | 0`, read back | Readback upper bits == `PLATFORM_MEM_BASE`, low 2 bits == 0 |
| STVEC-BASE-03 | BASE write across 1 GiB boundary | `csrw stvec, 0x40000004` (cross 1 GiB), read back | Readback == `0x40000004` |
| STVEC-BASE-04 | BASE write across 512 GiB boundary (high-bit probe) | `csrw stvec, 0x8000000000` (bit 39), read back | Readback == `0x8000000000` (if upper bits are WARL-truncated, proves Sstvecd is not truly enabled; failure) |
| STVEC-BASE-05 | BASE write non-4-byte-aligned address | `csrw stvec, 0x40000003` (low 2 bits = 0b11, MODE field), read back | Readback BASE portion == `0x40000000`, low 2 bits behavior per description below |
| STVEC-BASE-06 | BASE multiple rewrites do not affect MODE | Fix MODE=0, write BASE=0x1000, 0x2000, 0x4000, 0x8000 in sequence, read back MODE each time | Each time `MODE == 0` and BASE matches the written value |
| STVEC-BASE-07 | BASE high-bit scan | Write `1ULL << k | 0x4` in sequence (k from 12 to SXLEN-1, only 4-byte-aligned positions), read back each time | Each readback BASE == written value (Sstvecd requires "any valid") |

> [!IMPORTANT]
> **STVEC-BASE-05 low 2 bits semantic note**: When writing `0x40000003`, [1:0]=0b11 actually falls in the MODE field (not BASE bit[1:0]). The specification defines BASE as [XLEN-1:2], so this test case **actually verifies**: BASE field retention = `0x40000000 >> 2 << 2 = 0x40000000`, MODE field = 0b11. MODE=3 is a reserved value; per STVEC-MODE-04 rules, readback should be a legal value (0 or 1). Therefore, the precise assertion for BASE-05 is: `(readback & ~0x3) == 0x40000000` and `(readback & 0x3)` is an element of {0, 1}.

> [!NOTE]
> **STVEC-BASE-04 high-bit selection**: Bit 39 corresponds to near the Sv39 virtual address upper bound, a typical choice for probing high-bit WARL truncation. If the implementation constrains BASE to [38:2] (as some RV64 implementations may default to truncating at the Sv39 upper bound), this test case will fail. The Sstvecd specification requires BASE to hold "any valid" 4-byte-aligned address, so such a restriction should be lifted by a compliant Sstvecd implementation.

> [!NOTE]
> **STVEC-BASE-07 scan range**: Following the principle of "above `PLATFORM_MEM_BASE`, not conflicting with trap entry," select several k values (e.g., k=12, 16, 20, 24, 28, 32, 36, 38); exhaustive scanning up to bit 63 is not required. If SXLEN=64 but the valid virtual address bits are only 39/48/57, the behavior of writing addresses beyond the valid bits is implementation-defined; this test case covers only up to bit 38 (within Sv39 range).

---

### Group 3: Trap Vectoring to BASE under `MODE=Direct`

**Spec Reference**:
- `SPEC/supervisor.adoc:355-365`: When MODE=Direct, all traps (synchronous exceptions + asynchronous interrupts) set `pc` to BASE
- Sstvecd hard-constrains Direct to always be available (`norm:Sstvecd_mode_direct_writable`), so correct Direct behavior is an implicit commitment of Sstvecd

**Test Scope**: Verify that when `stvec.MODE=Direct`, both synchronous exceptions and asynchronous interrupts vector to BASE (rather than BASE+4xcause); verify that BASE retains its original value after multiple traps.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| STVEC-DIR-01 | Synchronous exception: S-mode ecall | Execute `ecall` in S-mode (cause=9), `stvec` set to custom 4-byte-aligned entry, BASE=entry address | `g_sstvecd_trap_pc == BASE` (hits entry start); `g_sstvecd_trap_cause == 9` |
| STVEC-DIR-02 | Synchronous exception: illegal instruction | Execute unknown instruction in S-mode (cause=2), same entry as above | `g_sstvecd_trap_pc == BASE`; `g_sstvecd_trap_cause == 2` |
| STVEC-DIR-03 | Synchronous exception: load page-fault | Enable Sv39 + unmapped VA, S-mode `lw` triggers cause=13 | `g_sstvecd_trap_pc == BASE`; `g_sstvecd_trap_cause == 13` |
| STVEC-INT-01 | Asynchronous interrupt: SSIP | M-mode sets `mideleg.SSIE=1`, S-mode self-triggers SSIP (cause=1, interrupt bit set) | `g_sstvecd_trap_pc == BASE` (Direct does not add 4x1=4) |
| STVEC-MULTI-01 | Multiple traps, BASE unchanged | Trigger 5 consecutive ecalls; after each trap, M-mode reads back `stvec` to check BASE and MODE | After 5 times, `BASE == original value` and `MODE == 0` |

---

## Appendix: Related Constants and API Reference

### CSRs and Bit Definitions

| Name | Value | Description |
|------|-------|-------------|
| `CSR_STVEC` | `0x105` | Supervisor trap vector base address, see `common/encoding.h:129` |
| `STVEC_MODE_MASK` | `0x3` | `stvec[1:0]`, i.e., the MODE field |
| `STVEC_MODE_DIRECT` | `0x0` | Direct mode |
| `STVEC_MODE_VECTORED` | `0x1` | Vectored mode (Sstvecd does not require implementation) |
| `MIDELEG_SSIE_BIT` | `1UL << 1` | Delegate SSIP to S-mode |
| `SIP_SSIP_BIT` / `SIE_SSIE_BIT` / `SSTATUS_SIE_BIT` | `1UL << 1` | SSIP self-trigger and S-mode interrupt enable |

### scause Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `CAUSE_S_ECALL` | 9 | Environment call from S-mode |
| `CAUSE_ILLEGAL_INSTRUCTION` | 2 | Illegal instruction |
| `CAUSE_LOAD_PAGE_FAULT` | 13 | Load page fault |
| `CAUSE_S_SOFTWARE_INTERRUPT` (interrupt number) | 1 | Supervisor software interrupt (scause high bit set to 1) |

### Test Framework API

- `pt_context_t` / `pt_init` / `pt_pool_reset`: Page table context management
- `pt_setup_identity_mapping(ctx, base, size, flags, level)`: Batch identity mapping setup
- `vm_run_in_smode(ctx, fn, arg)`: Enter S-mode with the specified page table context to execute fn(arg); on exception, returns scause (trap entry is taken over by the local `stvec` setting)
- `TEST_REGISTER` / `TEST_BEGIN` / `TEST_ASSERT` / `TEST_END`: Test case registration and assertion macros
- `CSRR(csr)` / `CSRW(csr, val)` / `CSRS(csr, mask)` / `CSRC(csr, mask)`: CSR read / write / set-bit / clear-bit

### Global Variables (provided by `sstvecd/tests/sstvecd_strap.S`)

| Variable | Type | Description |
|----------|------|-------------|
| `g_sstvecd_trap_pc` | `volatile uintptr_t` | PC at trap entry hit (should equal `stvec.BASE`) |
| `g_sstvecd_trap_cause` | `volatile uintptr_t` | `scause` at trap hit |
| `sstvecd_trap_entry` | `void(void)` | 4-byte-aligned S-mode trap entry symbol |

---

## Test Execution Instructions

### Subdirectory and Build Integration (Implementation Phase Reference; Not Produced by This Plan)

> [!IMPORTANT]
> This planning phase **produces only `docs/sstvecd_test_plan.md`**; it does not create the `sstvecd/` subdirectory, does not modify the top-level `Makefile`, and does not write C test code. The subsequent implementation phase will create the following per this document:
> - `sstvecd/main.c` (refer to `svadu/main.c`)
> - `sstvecd/Makefile` (refer to `svadu/Makefile`, `SPIKE_ISA_EXT = _sstvecd`)
> - `sstvecd/kernel.ld` (refer to `svadu/kernel.ld`)
> - `sstvecd/tests/test_mode.c`, `test_base.c`, `test_direct.c`, `sstvecd_strap.S`, `test_helpers.h`
> And add `sstvecd` to the top-level `Makefile` `EXTENSIONS` list.

### Runtime Environment

- All tests run under RV64 + Sv39 (Group 3) / M-mode direct CSR operations (Group 1, 2)
- M-mode enables the extension via `SPIKE_ISA_EXT = _sstvecd`; when not enabled, STVEC-BASE-04 / 07 test cases are expected to fail
- Single-core environment; no IPI required

### Failure Diagnosis Guide

| Symptom | Possible Cause |
|---------|----------------|
| STVEC-MODE-01 fails | Platform does not even implement basic stvec, or `csrw stvec` path is abnormal |
| STVEC-BASE-04 / 07 fails | Sstvecd not truly enabled; BASE upper bits are WARL-truncated |
| STVEC-DIR-01/02/03 fails (hit address != BASE) | Trap path abnormal, or `vm_run_in_smode` internally overwrites `stvec` |
| STVEC-INT-01 fails (hit address = BASE+4) | Implementation incorrectly handles interrupts as Vectored; indicates MODE=0 write did not take effect |
| STVEC-INT-01 fails (no hit) | `mideleg` did not delegate SSIP, or `sstatus.SIE` not enabled |
