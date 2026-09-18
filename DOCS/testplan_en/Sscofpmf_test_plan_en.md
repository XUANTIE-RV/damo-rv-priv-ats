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

### Counter Discovery Strategy

Since different implementations support varying numbers of hpmcounters (3–31), test cases employ **dynamic discovery**: in M-mode, attempt to write a non-zero value to `mhpmcounter` and read it back; if the read-back value is zero, the counter is considered unimplemented and the corresponding test is skipped.

### Event Trigger Strategy

To reliably increment counters, the tests use the following strategy:
1. **Configure mhpmevent for the "retired instructions" event** (event number is implementation-defined; provided by platform configuration macros).
2. **Execute a known instruction sequence** to produce a predictable counter increment.
3. **Initialize mhpmcounter to a value near the maximum** (e.g., `0xFFFFFFFF_FFFFFFFE`) to quickly trigger overflow.

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

**Prerequisites**: At least one hpmcounter must be confirmed as implemented, and a usable event number must be identified (provided by platform configuration, typically the retired-instructions event).

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
| COFPMF-OVF-09 | LCOFI interrupt delegated to S-mode | Set mideleg bit 13 = 1, trigger overflow interrupt; verify it is caught in the S-mode handler | S-mode trap handler receives cause = interrupt \| 13 |
| COFPMF-OVF-10 | LCOFI interrupt handled in M-mode | mideleg bit 13 = 0, trigger overflow interrupt; verify it is caught in the M-mode handler | M-mode trap handler receives cause = interrupt \| 13 |
| COFPMF-OVF-11 | No interrupt when LCOFIE is disabled | Clear mie.LCOFIE, trigger overflow; verify no interrupt occurs (only OF is set) | OF=1, but no interrupt is generated |
| COFPMF-OVF-12 | Multiple counters overflow simultaneously | Configure two counters both near maximum, trigger overflow simultaneously; verify both OF bits are set | OF bits of both mhpmevent CSRs are 1 |

> [!WARNING]
> The `norm:LCOFIP_op` specification states that after OF is set, LCOFIP is set "eventually, but not necessarily immediately." Tests must insert sufficient delay or memory barriers after overflow to ensure LCOFIP has propagated. If LCOFIP is not set after a reasonable delay, the test should flag this as an implementation-dependent timing issue rather than an outright failure.

---

### Group 4: scountovf Register

**Spec Reference**:
- `norm:scountovf_op`: 32-bit read-only; bit X corresponds to the OF bit of mhpmevent X.
- `norm:scountovf_mmode_read_access`: Always readable in M-mode.
- `norm:scountovf_smode_read_access`: S-mode access is controlled by mcounteren.
- `norm:scountovf_smode_read_access_control`: Access control is consistent with the mcounteren/hcounteren rules for hpmcounter.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| COFPMF-SOV-01 | scountovf reflects OF bit | Set mhpmevent3 OF=1, read scountovf; verify bit 3 = 1 | scountovf bit 3 = 1 |
| COFPMF-SOV-02 | scountovf read-only verification | Attempt to write scountovf (csrw in S-mode); should trigger illegal-instruction | Triggers illegal-instruction (cause=2) |
| COFPMF-SOV-03 | scountovf multi-bit mapping | Set mhpmevent3 and mhpmevent5 OF=1 simultaneously; verify scountovf bit 3 and bit 5 are both 1 | scountovf = (1<<3) \| (1<<5) |
| COFPMF-SOV-04 | scountovf clear tracking | Set OF=1 then clear mhpmevent3 OF; read scountovf bit 3 | scountovf bit 3 = 0 |
| COFPMF-SOV-05 | M-mode read is not restricted by mcounteren | Clear mcounteren bit 3; M-mode read of scountovf bit 3 still reflects the true value | scountovf bit 3 reflects the true OF value |
| COFPMF-SOV-06 | S-mode readable when mcounteren=1 | Set mcounteren bit 3 = 1; S-mode reads scountovf bit 3 | Reads the true OF value |
| COFPMF-SOV-07 | S-mode reads zero when mcounteren=0 | Clear mcounteren bit 3; S-mode reads scountovf; bit 3 should be 0 (even if OF=1) | scountovf bit 3 = 0 |

> [!NOTE]
> VS-mode scountovf access control cases (`norm:scountovf_vsmode_read_access`) depend on the Hypervisor extension and have been migrated to `Hypervisor_Ss_test_plan_en.md` Group 10 (HCROSS-SSCOFPMF-01~03).

---

## Design Points

### 1. Dynamic Counter Discovery

Since the number of implemented hpmcounter3–31 CSRs is optional, all test cases must dynamically probe whether the target counter exists before proceeding:

1. Attempt to write a non-zero value to the target mhpmcounter in M-mode.
2. Read back the counter.
3. Restore the original value.
4. If the read-back is non-zero, the counter is considered implemented; otherwise unimplemented.

If a counter is not implemented, the test case should use `TEST_SKIP()` to skip.

### 2. Event Configuration Strategy

Different implementations use different low-bit event numbers for mhpmevent. Tests should provide a platform-configurable event number macro (for example, the "retired instructions" event number) rather than hardcoding a specific implementation in the plan.

### 3. Trap Handler Requirements for Interrupt Testing

LCOFI interrupt testing (Group 3) requires the trap handler to:

1. **Interrupt identification**: Check the most significant bit of `mcause` (interrupt bit); if set to 1, the trap is an interrupt.
2. **Record interrupt information**: Log the interrupt cause (low bits) to trap state.
3. **LCOFIP clear**: Clear `mip.LCOFIP` in the handler to prevent an infinite interrupt loop.
4. **LCOFIE management**: Provide helpers to enable/disable LCOFIE.

### 4. Overflow Trigger Method

To reliably trigger overflow:
1. Set mhpmcounter to a value near overflow (e.g., `-MARGIN`).
2. Configure mhpmevent to start counting.
3. Execute enough instructions (greater than MARGIN).
4. Stop counting (clear mhpmevent).

`MARGIN` is recommended to be set to 50–100, enough to cover loop overhead.

### 5. scountovf Access Control Testing

Group 4 SOV-06~07 requires reading `scountovf` in S-mode; the framework's privilege-switch + no-trap assertion capability can be reused. VS-mode double-gating cases (which depend on the H extension) have been migrated to `Hypervisor_Ss_test_plan_en.md` Group 10; this plan no longer includes VS-mode tests.

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

**Uncovered specification points**:
- `norm:scountovf_vsmode_read_access`: VS-mode double-gating cases have been migrated to `Hypervisor_Ss_test_plan_en.md` Group 10; this plan does not duplicate coverage.
