**[中文](../testplan/Sscofpmf_test_plan.md) | English**

# Sscofpmf Extension Test Plan

This document describes the test plan for the Sscofpmf (Count Overflow and Privilege Mode Filtering, Version 1.0) extension. Sscofpmf defines standard fields in the high bits (bits 63–56) of the `mhpmevent` CSR, providing two core capabilities: (1) count overflow detection and interrupt generation (OF bit + LCOFIP/LCOFIE); (2) privilege-mode-based event counting filtering (MINH/SINH/UINH/VSINH/VUINH). It also introduces a read-only `scountovf` CSR, enabling S-mode to quickly query which counters have overflowed.

---

## Specification Sections Covered by This Document

This plan is based on the following RISC-V official specifications (local paths):

- `SPEC/riscv-isa-manual/src/priv/sscofpmf.adoc` — Sscofpmf Extension for Count Overflow and Mode-Based Filtering, Version 1.0
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — mhpmevent/mhpmcounter CSRs, mip/mie LCOFIP/LCOFIE, mideleg, mcounteren
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — LCOFIP/LCOFIE bits in sip/sie

Official repository:

- https://github.com/riscv/riscv-isa-manual (corresponding files `src/priv/sscofpmf.adoc`, `src/priv/machine.adoc`, `src/priv/supervisor.adoc` in the repository)

---

## Test Scope

### Key CSRs

| CSR | Address | Description |
|-----|---------|-------------|
| `mhpmevent3`–`mhpmevent31` | 0x323–0x33F | Event selectors; high bits add OF/xINH fields |
| `mhpmcounter3`–`mhpmcounter31` | 0xB03–0xB1F | Hardware performance counters (M-mode read/write) |
| `hpmcounter3`–`hpmcounter31` | 0xC03–0xC1F | Hardware performance counters (S/U-mode read-only shadow) |
| `scountovf` | 0xDA0 | 32-bit read-only; shadow copy of OF bits |
| `mcounteren` | 0x306 | M-mode control of S-mode counter access |
| `hcounteren` | 0x606 | HS-mode control of VS-mode counter access (VS-mode gating cases have been migrated to `Hypervisor_Ss_test_plan_en.md` Group 10) |
| `mip` / `sip` | 0x344 / 0x144 | Interrupt pending; bit 13 = LCOFIP |
| `mie` / `sie` | 0x304 / 0x104 | Interrupt enable; bit 13 = LCOFIE |
| `mideleg` | 0x303 | Interrupt delegation; bit 13 controls LCOFI delegation to S-mode |

### mhpmevent High-Bit Field Layout

```
  63   62   61   60   59   58   57   56
+----+----+----+----+-----+-----+----+----+
| OF |MINH|SINH|UINH|VSINH|VUINH|WPRI|WPRI|
+----+----+----+----+-----+-----+----+----+
```

### Covered Specification Points

| Norm ID | Original Text | Description |
|---------|---------------|-------------|
| `norm:mhpmevent_inh_op` | Each of the five `x`INH bits, when set, inhibit counting of events while in privilege mode `x`. All-zeroes for these bits results in counting of events in all modes. | Each of the five `x`INH bits, when set, inhibits event counting in privilege mode `x`. All-zero results in counting in all modes. |
| `norm:mhpmevent_of_op` | The OF bit is set when the corresponding hpmcounter overflows, and remains set until written by software. | The OF bit is set when the corresponding hpmcounter overflows, and remains set until written by software. |
| `norm:hpmcounter_overflow` | Since hpmcounter values are unsigned values, overflow is defined as unsigned overflow of the implemented counter bits. Note that there is no loss of information after an overflow since the counter wraps around and keeps counting while the sticky OF bit remains set. | Overflow is defined as unsigned overflow of the implemented counter bits; after overflow the counter wraps and keeps counting while the sticky OF bit remains set. |
| `norm:count_overflow_interrupt` | If an hpmcounter overflows while the associated OF bit is zero, then a "count overflow interrupt request" is generated. If the OF bit is one, then no interrupt request is generated. Consequently the OF bit also functions as a count overflow interrupt disable for the associated hpmcounter. | An hpmcounter overflow with OF=0 generates a "count overflow interrupt request"; with OF=1 no request is generated. OF also functions as an overflow interrupt disable. |
| `norm:count_overflow_trigger` | Count overflow never results from writes to the mhpmcounter_n or mhpmevent_n registers, only from hardware increments of counter registers. | Count overflow never results from writes to mhpmcounter_n / mhpmevent_n; only from hardware increments. |
| `norm:mhpmevent_of_bit_set` | Generation of a count-overflow-interrupt request by an `hpmcounter` sets the associated OF bit. | Generation of a count-overflow-interrupt request by an hpmcounter sets the associated OF bit. |
| `norm:LCOFIP_op` | When an OF bit is set, it eventually, but not necessarily immediately, sets the LCOFIP bit in the `mip`/`sip` registers. | When an OF bit is set, it eventually — but not necessarily immediately — sets the LCOFIP bit in `mip`/`sip`. |
| `norm:scountovf_op` | This extension adds the `scountovf` CSR, a 32-bit read-only register that contains shadow copies of the OF bits in the 29 mhpmevent CSRs (mhpmevent_3 - mhpmevent_31) - where scountovf bit X corresponds to mhpmevent_X. | The extension adds `scountovf`, a 32-bit read-only register containing shadow copies of the OF bits of mhpmevent_3..31; scountovf bit X corresponds to mhpmevent_X. |
| `norm:scountovf_smode_read_access_control` | Read access to bit X is subject to the same mcounteren (or mcounteren and hcounteren) CSRs that mediate access to the hpmcounter CSRs by S-mode (or VS-mode). | Read access to bit X is subject to the same mcounteren (or mcounteren + hcounteren) gating that mediates hpmcounter access for S-mode (or VS-mode). |
| `norm:scountovf_mmode_read_access` | In M-mode, scountovf bit X is always readable. | In M-mode, scountovf bit X is always readable. |
| `norm:scountovf_smode_read_access` | In S/HS-mode, scountovf bit X is readable when mcounteren bit X is set, and otherwise reads as zero. | In S/HS-mode, scountovf bit X is readable when mcounteren bit X is set; otherwise it reads as zero. |

### Out of Scope

- **Counter event selection correctness**: The low-bit event selector fields of mhpmevent are implementation-defined and not within the scope of Sscofpmf standardization.
- **Vectored-mode interrupt dispatch details**: Only verifies that the LCOFI interrupt can be correctly generated and captured; does not verify jump offsets in vectored mode.
- **Multi-hart scenarios**: The project targets a single-core test environment.
- **RV32 / mhpmeventh**: Only RV64 is covered (RV32 requires mhpmeventh CSR access for the upper 32 bits).
- **Exact counter value verification**: Does not verify the precise number of counter increments; only verifies overflow behavior.
- **VS-mode scenarios (VSINH/VUINH counting inhibition, VS-mode scountovf double gating)**: Migrated to `Hypervisor_Ss_test_plan_en.md` Group 10.

---

## Prerequisites and Constraints

### Extension Gating and Counter Selection Strategy

Whether a case skips is decided solely by **compile-time config capability macros**, with no runtime probing: `SSCOFPMF_AVAILABLE` (normalized by `common/capabilities.h` from `SSCOFPMF_SUPPORTED` in `config/<platform>/rvtest_config.h`) determines whether Sscofpmf is implemented, and the whole group TEST_SKIPs when it is not declared; S/U-mode cases are additionally gated by `S_AVAILABLE`/`U_AVAILABLE`. The target `mhpmcounter` uses a fixed index `COFPMF_TEST_COUNTER` (default 3), with `COFPMF_TEST_COUNTER2` (default 4) for multi-counter cases, overridable via `-DCOFPMF_TEST_COUNTER=<n>`; counters are no longer dynamically discovered, and no `scountovf`/counting/WPRI functional probe is performed. The sole exception is COFPMF-RW-10 (iterate all implemented counters): because the counter count is implementation-optional and cannot be expressed in config, it keeps `is_counter_implemented()` to enumerate them one by one.

**Rationale**: runtime capability probing (trap-protected write/read-back, `scountovf`/counting/WPRI functional probes, dynamic counter discovery) silently downgrades "the platform declared the extension in config but implements it non-compliantly" into a SKIP, masking non-compliance; with compile-time gating, a platform that declares support but implements it incorrectly makes the case **fail loudly**, consistent with this document's "if any platform violates the SPEC the case remains FAIL" and the framework convention "detect extension capability via the compile-time `<EXT>_AVAILABLE` gate rather than runtime trap probing".

### Two-Layer Gating: Extension Declaration and Counting Precondition

Compile-time gating has two layers; both read config only and neither probes the DUT:

1. **Layer 1 - is the extension under test**: `SSCOFPMF_AVAILABLE` (from `SSCOFPMF_SUPPORTED` in `rvtest_config.h`). When undeclared, all 39 cases TEST_SKIP.
2. **Layer 2 - is the counting precondition available**: `SSCOFPMF_COUNTING_AVAILABLE`, normalized by `common/capabilities.h` from `SSCOFPMF_COUNT_EVENT` in `config/<platform>/platform_config.h`. That macro declares "the event encoding measured to really increment `mhpmcounter` on this platform" and also supplies the value of `COFPMF_EVENT_INSTRET`, so the event number is no longer hardcoded in helpers. **An undeclared platform has no counting event**, so the 19 cases that depend on "the counter really increments" (COFPMF-INH-01~10, COFPMF-OVF-01/02/04/05/08/09/10/11/12) TEST_SKIP with the fixed reason `platform declares no counting HPM event (implementation-defined)`.

Layer 2 must be a config declaration rather than a runtime probe: event encodings and the number of implemented counters are implementation-defined and the SPEC never requires a given event number to count, so "whether counting works" is a **platform fact** (same class as `QEMU_SKIP_TESTS` and `CLINT_SUPPORTED`) belonging to `platform_config.h`; `rvtest_config.h` is auto-generated by udb-gen and must not be hand-edited.

**Decision rules** (whether a precondition leads to SKIP or FAIL):

| Situation | Handling | Basis |
|-----------|----------|-------|
| config **declares** a counting event but the DUT measures read-only-zero counters / no increment | **FAIL**, printing the measured evidence | Declaration contradicts implementation - either config is wrong or there is a DUT/simulator gap; it must be exposed loudly |
| config **does not declare** one, and the property is SPEC-allowed implementation freedom (event encoding, counter count) | **SKIP**, with a compile-time auditable reason | The SPEC does not require any particular event to count |
| After writing `mcounteren`/`hcounteren`, the target bit **reads back 0** | **SKIP** (precondition unreachable) | `norm:mcounteren_flds_rdonly0` explicitly allows read-only zero |
| Sscofpmf semantics themselves (OF/xINH read-write, `scountovf` mirroring and gating, LCOFI behaviour) | Always assert; violations **FAIL** | Standards are not relaxed per platform |

Runtime code **keeps evidence collection only, never skip decisions**: on assertion failure the counter writability, `mhpmevent` read-back and count delta are printed so the failure explains its own root cause; whether to skip is decided solely by the compile-time macros above.

### Event Trigger Strategy

To increment counters and trigger overflow reliably and **deterministically**, the tests use:
1. **Event number taken from platform config**: `COFPMF_EVENT_INSTRET` <- `SSCOFPMF_COUNT_EVENT` (temporarily overridable via `-DCOFPMF_EVENT_INSTRET=<n>` for experiments).
2. **Stimulus privilege must match the assertion's claim, and no other privilege may leak into the window**: an instruction sequence asserting "M-mode counting" must run entirely inside one `run_in_priv(PRIV_M, ...)` excursion (using bare CSR access inside, so no proxy ecall lands in the window); S/U-mode sequences use `run_in_priv(PRIV_S/PRIV_U, ...)` and must not rely on the one-way `goto_priv()`. Every mode except the target (including the M-mode proxy code and S-mode switch code that serve the measurement) must have its xINH bit set: measured on QEMU (whose counter advances with host time), extra framework code inside the window contributes pollution on the order of 10^7~10^8 counts, which turns negative assertions into false failures.
3. **Frozen overflow trigger** (removes the preload race): freeze via `mcountinhibit.n` -> write `mhpmevent` (OF=0, xINH set for the target mode) -> preload `mhpmcounter = -MARGIN` -> unfreeze -> run a bounded instruction sequence -> re-freeze, then read `mhpmevent`/`mhpmcounter`. Even implementations with an extremely high counting rate (e.g. QEMU's host-level free-running count) cannot wash the preload away.
4. **Positive and negative assertions share one precondition**: negative assertions of the "count is 0" kind (INH-02/04/06/10, OVF-05) must be gated by layer 2 exactly like the positive ones in the same group, otherwise they pass vacuously on platforms that never count.

If any platform violates the SPEC, the corresponding case remains FAIL, and implementation defects are recorded to the `bugs/` directory.

---

## Test Groups

### Group 1: mhpmevent Field Read/Write Verification (M-mode CSR Read/Write Round-Trip)

**Spec Reference**:
- `norm:mhpmevent_of_op`: OF bit is writable and readable.
- `norm:mhpmevent_inh_op`: xINH bits are writable and readable; xINH bits corresponding to unimplemented privilege modes are read-only zero.

**Test Scope**: In M-mode, verify the read/write behavior of the upper 8-bit fields of the mhpmevent CSR.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| COFPMF-RW-01 | OF bit read-write round-trip | For an implemented mhpmevent, write OF=1 and read back to verify, then write OF=0 and read back to verify | OF bit written value matches read-back value |
| COFPMF-RW-02 | MINH bit read-write round-trip | Write MINH=1 and read back, then write 0 and read back | MINH bit read/write consistent |
| COFPMF-RW-03 | SINH bit read-write round-trip | Write SINH=1 and read back, then write 0 and read back; if S-mode is not implemented, should be read-only zero | Read/write consistent when S-mode is implemented; always 0 when not implemented |
| COFPMF-RW-04 | UINH bit read-write round-trip | Write UINH=1 and read back, then write 0 and read back; if U-mode is not implemented, should be read-only zero | Read/write consistent when U-mode is implemented; always 0 when not implemented |
| COFPMF-RW-05 | VSINH bit read-write round-trip | Write VSINH=1 and read back, then write 0 and read back; if VS-mode is not implemented, should be read-only zero | Read/write consistent when H-ext is implemented; always 0 when not implemented |
| COFPMF-RW-06 | VUINH bit read-write round-trip | Write VUINH=1 and read back, then write 0 and read back; if VU-mode is not implemented, should be read-only zero | Read/write consistent when H-ext is implemented; always 0 when not implemented |
| COFPMF-RW-07 | WPRI fields are zero | Write bits 57–56 to 1; read-back should be 0 | WPRI fields are read-only zero |
| COFPMF-RW-08 | Multi-field combined write | Write OF=1, MINH=1, SINH=1, UINH=1 simultaneously, then read back all fields | Each field independently holds the correct value |
| COFPMF-RW-09 | Low-bit field preservation | Writing mhpmevent high bits does not affect the low 56 bits (low-bit value unchanged before and after high-bit write) | Low-bit fields are unaffected by high-bit writes |
| COFPMF-RW-10 | Multi-counter traversal | Iterate over mhpmevent3–31, performing OF bit write-1/read-back verification for each (skip unimplemented) | OF bit of all implemented mhpmevent CSRs is readable and writable |

> [!NOTE]
> COFPMF-RW-05/06 verify the positive WARL branch of VSINH/VUINH (independent of the H extension); on platforms without the H extension, these two bits are expected to be read-only zero (negative branch). The full H-extension-dependent semantics (VSINH/VUINH counting inhibition, read-only-zero probing when H is unimplemented) are covered by `Hypervisor_Ss_test_plan_en.md` Group 10 (HCROSS-SSCOFPMF-04~06).

---

### Group 2: Privilege Mode Filtering

**Spec Reference**:
- `norm:mhpmevent_inh_op`: When an xINH bit is set, event counting is inhibited in the corresponding privilege mode; when all are zero, counting occurs in all modes.

**Test Scope**: Verify that each xINH bit correctly inhibits or permits event counting in the corresponding privilege mode.

**Prerequisites**: gated by both compile-time `SSCOFPMF_AVAILABLE` and `SSCOFPMF_COUNTING_AVAILABLE` (all 10 cases in this group depend on the counter really incrementing); the target counter is the fixed `COFPMF_TEST_COUNTER`, and the event number is provided by the platform configuration macro `SSCOFPMF_COUNT_EVENT` -> `COFPMF_EVENT_INSTRET`. The privilege in which the instruction sequence executes must match the assertion's claim (see Event Trigger Strategy item 2).

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| COFPMF-INH-01 | M-mode counting with all-zero xINH | Set all xINH=0, execute instruction sequence in M-mode, verify counter increments | Counter value increases |
| COFPMF-INH-02 | MINH=1 inhibits M-mode counting | Set MINH=1, execute instruction sequence in M-mode, verify counter does not increment | Counter value unchanged |
| COFPMF-INH-03 | MINH=0 restores M-mode counting | Clear MINH, execute again in M-mode, verify counting resumes | Counter value increases |
| COFPMF-INH-04 | SINH=1 inhibits S-mode counting | Set SINH=1, switch to S-mode and execute instruction sequence, return to M-mode and read counter | Counter value unchanged (ignoring overhead of mode switch itself) |
| COFPMF-INH-05 | SINH=0 permits S-mode counting | Set SINH=0, switch to S-mode and execute instruction sequence, return to M-mode and read counter | Counter value increases |
| COFPMF-INH-06 | UINH=1 inhibits U-mode counting | Set UINH=1, switch to U-mode and execute instruction sequence, return to M-mode and read counter | Counter value unchanged |
| COFPMF-INH-07 | UINH=0 permits U-mode counting | Set UINH=0, switch to U-mode and execute instruction sequence, return to M-mode and read counter | Counter value increases |
| COFPMF-INH-08 | Multi-xINH combination: M-mode only counting | Set SINH=1, UINH=1 (inhibit S/U), execute only in M-mode, verify counting | M-mode counting is normal |
| COFPMF-INH-09 | Multi-xINH combination: S-mode only counting | Set MINH=1, UINH=1 (inhibit M/U), switch to S-mode and execute | S-mode counting is normal |
| COFPMF-INH-10 | No counting when all xINH=1 | Set MINH=SINH=UINH=1, execute in all modes; no increment should occur | Counter value unchanged |

> [!NOTE]
> Privilege mode switches (ecall / mret / sret) themselves produce instruction counts. Tests verify functionality by comparing the count difference between "inhibited" and "non-inhibited" scenarios, rather than verifying exact count values. For S/U-mode tests, counting should first be disabled in M-mode (MINH=1), then switch to the target mode for execution, and upon return only compare the count delta during target-mode execution.
>
> **Stimulus privilege must match the assertion's claim**: COFPMF-INH-01/02/03/08 assert M-mode counting behaviour, so their instruction sequences must run entirely inside one `run_in_priv(PRIV_M, ...)` excursion (under the S-mode-normal model the case body itself runs in S-mode, so executing nops directly in the body measures S-mode counting); the "only one mode counts" claim of COFPMF-INH-08/09 must be built from a **pair** of assertions (target mode increments + inhibited mode does not), since a single-sided assertion cannot prove "only".
>
> This group does not cover VSINH/VUINH counting-inhibition cases (which depend on the H extension and require entering VS/VU-mode); that portion is covered by `Hypervisor_Ss_test_plan_en.md` Group 10 (HCROSS-SSCOFPMF-04~05).

---

### Group 3: Count Overflow and Interrupts

**Spec Reference**:
- `norm:mhpmevent_of_op`: OF bit is set on hpmcounter overflow; sticky until cleared by software.
- `norm:hpmcounter_overflow`: Overflow is defined as unsigned overflow of the implemented bit width.
- `norm:count_overflow_interrupt`: An interrupt request is generated on overflow when OF=0; no request is generated when OF=1.
- `norm:count_overflow_trigger`: Writes to mhpmcounter/mhpmevent do not trigger overflow; only hardware increments do.
- `norm:mhpmevent_of_bit_set`: Overflow interrupt request sets the OF bit.
- `norm:LCOFIP_op`: After OF is set, LCOFIP is eventually set.

**Prerequisites**: except for COFPMF-OVF-03/06/07 (whose stimulus is only CSR writes and does not depend on counter increments), the other 9 cases of this group are gated by both `SSCOFPMF_AVAILABLE` and `SSCOFPMF_COUNTING_AVAILABLE`; overflow is always produced by the frozen trigger (Event Trigger Strategy item 3) for determinism.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| COFPMF-OVF-01 | Counter overflow sets OF | Initialize mhpmcounter near maximum, configure event and execute instructions to cause overflow, check OF bit | OF bit is set to 1 by hardware |
| COFPMF-OVF-02 | OF bit sticky behavior | After overflow OF=1, continue executing instructions; verify OF remains 1 and counter continues counting (wrap around) | OF=1 remains unchanged; counter value continues incrementing from near 0 |
| COFPMF-OVF-03 | Software clears OF bit | After overflow, write OF to 0; verify successful clear | OF bit becomes 0 |
| COFPMF-OVF-04 | Overflow generates LCOFIP | Initialize mhpmcounter near maximum with OF=0, enable LCOFIE, trigger overflow and check LCOFIP | mip.LCOFIP = 1 |
| COFPMF-OVF-05 | Overflow with OF=1 does not generate interrupt | Pre-set OF=1, then trigger overflow; verify LCOFIP is not set | mip.LCOFIP = 0 |
| COFPMF-OVF-06 | Writing mhpmcounter does not trigger overflow | Directly write mhpmcounter to 0 (a "wrap" from a large value); verify OF is not set | OF bit remains 0 |
| COFPMF-OVF-07 | Writing mhpmevent does not trigger overflow | Modify mhpmevent event selection / xINH fields; verify OF is not set | OF bit remains 0 |
| COFPMF-OVF-08 | LCOFIP software clear | After setting LCOFIP, software clears it by writing mip; verify successful clear | mip.LCOFIP = 0 |
| COFPMF-OVF-09 | LCOFI interrupt delegated to S-mode | Set mideleg bit 13 = 1; first complete the overflow with the frozen trigger while LCOFIE=0 and confirm OF/LCOFIP are set, then enable sie.LCOFIE + sstatus.SIE and run a bounded instruction sequence | Must receive cause = interrupt \| 13 in the S-mode handler (the request is already pending, so delivery after enabling is deterministic - no timing exemption) |
| COFPMF-OVF-10 | LCOFI interrupt handled in M-mode | mideleg bit 13 = 0; same pend-then-enable order as OVF-09 (mie.LCOFIE + mstatus.MIE) | Must receive cause = interrupt \| 13 in the M-mode handler |
| COFPMF-OVF-11 | No interrupt when LCOFIE is disabled | Clear mie.LCOFIE, trigger overflow; verify no interrupt occurs (only OF is set) | OF=1, but no interrupt is generated |
| COFPMF-OVF-12 | Multiple counters overflow simultaneously | Configure two counters both near maximum, trigger overflow simultaneously; verify both OF bits are set | OF bits of both mhpmevent CSRs are 1 |

> [!WARNING]
> The `norm:LCOFIP_op` specification states that after OF is set, LCOFIP is set "eventually, but not necessarily immediately." Tests therefore use a **bounded poll** (`COFPMF_LCOFIP_POLL_LIMIT`, default 64 rounds with nops between rounds) to wait for propagation; if LCOFIP is still clear when the poll limit is reached the case is judged **FAIL** and prints the measured OF/LCOFIP values - at that point the precondition "overflow really happened and OF is set" has already been guaranteed by the frozen trigger, so it must not be excused as an "implementation-dependent timing issue", and constant-true fallbacks such as `TEST_ASSERT(..., true)` are **forbidden**.
>
> LCOFI delivery cases (COFPMF-OVF-09/10) must first complete the overflow with LCOFIE=0 and confirm OF/LCOFIP are set, and only **then** enable interrupts: the request is already pending at that point, so it must be delivered immediately once enabled, and receiving `cause = interrupt|13` is a deterministic expectation with no timing exemption.

---

### Group 4: scountovf Register

**Spec Reference**:
- `norm:scountovf_op`: 32-bit read-only; bit X corresponds to the OF bit of mhpmevent X.
- `norm:scountovf_mmode_read_access`: Always readable in M-mode.
- `norm:scountovf_smode_read_access`: S-mode access is controlled by mcounteren.
- `norm:scountovf_smode_read_access_control`: Access control is consistent with the mcounteren/hcounteren rules for hpmcounter.
- `norm:mcounteren_flds_rdonly0`: mcounteren fields are WARL and may be read-only zero (in which case a less-privileged access to the corresponding counter raises an illegal-instruction exception).

**Prerequisites**: this group does not depend on counter increments (OF is set by software writing `mhpmevent`) and is gated only by `SSCOFPMF_AVAILABLE`. However, any case that observes a counter's OF bit from S-mode (SOV-01/03/04/06) must write `mcounteren.n` and **read it back**; a read-back of 0 means the field is read-only zero (allowed by `norm:mcounteren_flds_rdonly0`), i.e. the precondition is unreachable, and the case must TEST_SKIP with that reason instead of being reported as a `scountovf` mirroring failure.

**Privilege alignment**: SOV-05 asserts `norm:scountovf_mmode_read_access` (M-mode is not gated by mcounteren), so its `scountovf` read must run entirely inside a `run_in_priv(PRIV_M, ...)` excursion; under the S-mode-normal model the case body runs in S-mode, and a bare `CSRR(scountovf)` in the body measures `norm:scountovf_smode_read_access` instead (the bit reads 0 when mcounteren.n=0), which is a name/behaviour mismatch. SOV-06/07 must carry the value read in S-mode back and assert the **bit value** (SOV-06 expects 1, SOV-07 expects 0); asserting only "no trap" does not verify the gating semantics.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| COFPMF-SOV-01 | scountovf reflects OF bit | Set mhpmevent3 OF=1, read scountovf; verify bit 3 = 1 | scountovf bit 3 = 1 |
| COFPMF-SOV-02 | scountovf read-only verification | Attempt to write scountovf (csrw in S-mode); should trigger illegal-instruction | Triggers illegal-instruction (cause=2) |
| COFPMF-SOV-03 | scountovf multi-bit mapping | Set OF=1 for both `COFPMF_TEST_COUNTER` and `COFPMF_TEST_COUNTER2` (defaults 3 and 4); verify both corresponding scountovf bits are 1 | scountovf = (1<<3) \| (1<<4) |
| COFPMF-SOV-04 | scountovf clear tracking | Set OF=1 then clear mhpmevent3 OF; read scountovf bit 3 | scountovf bit 3 = 0 |
| COFPMF-SOV-05 | M-mode read is not restricted by mcounteren | Clear mcounteren bit 3; read scountovf bit 3 **inside an M-mode excursion**, which still reflects the true value | scountovf bit 3 reflects the true OF value |
| COFPMF-SOV-06 | S-mode readable when mcounteren=1 | Set mcounteren bit 3 = 1 (confirmed by read-back); S-mode reads scountovf and the value is carried back for assertion | Reads bit 3 = 1 |
| COFPMF-SOV-07 | S-mode reads zero when mcounteren=0 | Clear mcounteren bit 3 (first confirm OF is visible inside an M-mode excursion); S-mode reads scountovf and the value is carried back for assertion | No trap, and bit 3 = 0 (even though OF=1) |

> [!NOTE]
> VS-mode scountovf access control cases (`norm:scountovf_vsmode_read_access`) depend on the Hypervisor extension and have been migrated to `Hypervisor_Ss_test_plan_en.md` Group 10 (HCROSS-SSCOFPMF-01~03).

---

## Design Points

### 1. Two-Layer Gating and Fixed Counter

Whether a case skips is decided by compile-time macros, never by probing the DUT: layer 1 `SSCOFPMF_AVAILABLE` (plus the required `S_AVAILABLE`/`U_AVAILABLE`/`H_AVAILABLE`) decides whether the extension is under test; layer 2 `SSCOFPMF_COUNTING_AVAILABLE` (normalized from `SSCOFPMF_COUNT_EVENT` in `platform_config.h`) decides whether the counting precondition is available and applies only to the 19 cases that depend on the counter really incrementing. The counters under test use the fixed indices `COFPMF_TEST_COUNTER` (default 3) and `COFPMF_TEST_COUNTER2` (default 4), overridable via `-D`. The only remaining runtime enumeration is COFPMF-RW-10, whose job is exactly to "iterate all implemented counters and verify OF read/write"; since the counter count is implementation-optional and cannot be expressed in config, it keeps `is_counter_implemented()` to enumerate and skip unimplemented ones.

(The former approach of "dynamically discovering counters + functional probes (`is_counting_functional`/`is_scountovf_functional`/`is_wpri_masked`) then TEST_SKIP" is deprecated: it silently downgrades platform non-compliance into a SKIP. Layer 1 alone, however, would treat an unmet implementation-defined precondition as non-compliance too, hence the layer-2 config declaration; see the decision rules in "Prerequisites and Constraints".)

### 2. Event Configuration Strategy

The low-bit event number of mhpmevent differs per implementation and is implementation-defined; the SPEC never requires a particular number to count. The event number must therefore be declared by platform config: `config/<platform>/platform_config.h` defines `SSCOFPMF_COUNT_EVENT <n>` (only when measurement on that platform shows it really increments), `common/capabilities.h` normalizes it to `SSCOFPMF_COUNTING_AVAILABLE`, and helpers derive `COFPMF_EVENT_INSTRET` from it; neither the plan nor the cases may hardcode a specific number.

**A declaration that contradicts the implementation must FAIL**: if config declares an event but measurement shows read-only-zero counters or no increment, the case stays failed and prints the measured evidence (counter writability / `mhpmevent` read-back / count delta), leaving a human to decide whether to fix config or report a DUT/simulator gap; it must not be downgraded to SKIP.

### 3. Trap Handler Requirements for Interrupt Testing

LCOFI interrupt testing (Group 3) requires the trap handler to:

1. **Interrupt identification**: Check the most significant bit of `mcause` (interrupt bit); if set to 1, the trap is an interrupt.
2. **Record interrupt information**: Log the interrupt cause (low bits) to trap state.
3. **LCOFIP clear**: Clear `mip.LCOFIP` in the handler to prevent an infinite interrupt loop.
4. **LCOFIE management**: Provide helpers to enable/disable LCOFIE.

### 4. Overflow Trigger Method

Overflow is triggered deterministically by the **frozen sequence** (helpers' `cofpmf_trigger_overflow_det()`):
1. Set `mcountinhibit.n` to freeze the counter (gated by `SMCNTRPMF_AVAILABLE`; when unavailable, degrade to the "write counter before event" order).
2. Write `mhpmevent` (event number + the xINH combination for the target mode, OF=0).
3. Preload `mhpmcounter = -MARGIN`.
4. Clear `mcountinhibit.n` to unfreeze and execute enough instructions (more than MARGIN).
5. Re-freeze, then read `mhpmevent` (for OF) and `mhpmcounter` (wrap evidence), and stop counting.

`MARGIN` is recommended to be 50-100, enough to cover loop overhead. The order of steps 1-3 must not be inverted: arming the event before preloading the counter lets an implementation with a very high counting rate (e.g. QEMU mapping `mhpmcounter` to a host-level free-running quantity, measured at roughly 10^4 per nop) wash the preloaded value away between the write and the read-back, which makes a multi-counter case (OVF-12) show a false "one side does not count" symptom.

### 5. scountovf Access Control Testing

Group 4 SOV-06~07 requires reading `scountovf` in S-mode; the framework's privilege-switch capability can be reused, but the value read in S-mode must be carried back and the bit value asserted (the return value of `run_in_priv()` suffices) - asserting only "no trap" is not enough; SOV-05's M-mode read must likewise be placed inside `run_in_priv(PRIV_M, ...)`. Any case needing S-mode visibility must read `mcounteren.n` back after writing it; a read-back of 0 means the precondition is unreachable and the case SKIPs (`norm:mcounteren_flds_rdonly0` allows read-only zero). VS-mode double-gating cases (which depend on the H extension) have been migrated to `Hypervisor_Ss_test_plan_en.md` Group 10; this plan no longer includes VS-mode tests.

### 6. Insufficient Simulator Coverage and the Hardware Home

Measured HPM counting capability of the four simulators (instrumented values, not inference):

- **spike**: `mhpmcounter3..31` are read-only zero (writing `0xfffffffffffffff6` reads back 0) and the `mhpmevent` selector field reads back 0 (WARL) while OF/xINH bits are writable - no counting event, so `SSCOFPMF_COUNT_EVENT` is **not declared**; the `Zihpm` suite reaches the same conclusion for the same fact ("read-only-zero mirror is SPEC-legal").
- **sail / whisper**: counters and selectors are both writable (read-back == written), but scanning events 1..64 plus sparse vendor encodings (0x80/0x100/0x1000/0x10000/0xFFFFFFFF) shows no counting in M- or S-mode (while `minstret` works: 2219 for 200 nops) - **not declared**.
- **whisper, additional limit**: `mcounteren` bits >= 3 are read-only zero (writing `0xFFFFFFFF` reads back `0x7`; corroborated by S-mode reads of `hpmcounter3` always trapping), so COFPMF-SOV-01/03/04/06 SKIP as "precondition unreachable" (allowed by `norm:mcounteren_flds_rdonly0`).
- **qemu**: events 1/2 count, but the quantity is a host-level free-running value rather than retired instructions (a preloaded value is washed away while counting), and **only one counter advances at a time** (with ctr3/ctr4 armed together only ctr3 increments while ctr4 stays at its preload) - so `SSCOFPMF_COUNT_EVENT 2` **is declared**: the OF/LCOFIP/LCOFI delivery paths are genuinely verifiable, while COFPMF-OVF-12 (multiple counters overflowing together) stays failed because of that single-counter limit and is recorded in `bugs/`.

Effective simulator coverage of the 19 counting cases is therefore limited to qemu; full verification of `xINH` filtering and overflow semantics belongs to the hardware platform configs, whose `platform_config.h` should carry the event encoding from the hardware manual.

---

## Appendix A: Specification Point Coverage Matrix

| Norm ID | Covered Test IDs | Coverage Status | Notes |
|---------|------------------|-----------------|-------|
| `norm:mhpmevent_inh_op` | COFPMF-RW-02 ~ COFPMF-RW-08, COFPMF-INH-01 ~ COFPMF-INH-10 | Covered | xINH bit read/write and mode filtering |
| `norm:mhpmevent_of_op` | COFPMF-RW-01, COFPMF-RW-08, COFPMF-RW-10, COFPMF-OVF-01 ~ COFPMF-OVF-03, COFPMF-SOV-01, COFPMF-SOV-03, COFPMF-SOV-04 | Covered | OF bit read/write and sticky semantics |
| `norm:hpmcounter_overflow` | COFPMF-OVF-01, COFPMF-OVF-02, COFPMF-OVF-12 | Covered | Unsigned overflow definition |
| `norm:count_overflow_interrupt` | COFPMF-OVF-04, COFPMF-OVF-05, COFPMF-OVF-11 | Covered | Overflow interrupt request generation condition |
| `norm:count_overflow_trigger` | COFPMF-OVF-06, COFPMF-OVF-07 | Covered | CSR writes do not trigger overflow |
| `norm:mhpmevent_of_bit_set` | COFPMF-OVF-01, COFPMF-OVF-04, COFPMF-OVF-12 | Covered | Overflow interrupt request sets OF |
| `norm:LCOFIP_op` | COFPMF-OVF-04, COFPMF-OVF-05, COFPMF-OVF-08 ~ COFPMF-OVF-11 | Covered | LCOFIP propagation after OF is set |
| `norm:scountovf_op` | COFPMF-SOV-01, COFPMF-SOV-02, COFPMF-SOV-03, COFPMF-SOV-04 | Covered | scountovf 32-bit read-only semantics |
| `norm:scountovf_mmode_read_access` | COFPMF-SOV-01, COFPMF-SOV-03, COFPMF-SOV-04, COFPMF-SOV-05 | Covered | M-mode always readable |
| `norm:scountovf_smode_read_access` | COFPMF-SOV-06, COFPMF-SOV-07 | Covered | S-mode gated by mcounteren |
| `norm:scountovf_smode_read_access_control` | COFPMF-SOV-05, COFPMF-SOV-06, COFPMF-SOV-07 | Covered | Access control rule consistency |
| `norm:mcounteren_flds_rdonly0` | COFPMF-SOV-01, COFPMF-SOV-03, COFPMF-SOV-04, COFPMF-SOV-06 | Covered | Read `mcounteren` back after writing; SKIP as unreachable precondition when read-only zero |

**Uncovered specification points**:
- `norm:scountovf_vsmode_read_access`: VS-mode double-gating cases have been migrated to `Hypervisor_Ss_test_plan_en.md` Group 10; this plan does not duplicate coverage.
