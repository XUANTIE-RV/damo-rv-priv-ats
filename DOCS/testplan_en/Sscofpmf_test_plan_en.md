**[中文](../testplan/Sscofpmf_test_plan.md) | English**

# Sscofpmf Extension Test Plan

This document describes the test plan for the Sscofpmf (Count Overflow and Privilege Mode Filtering, Version 1.0) extension. Sscofpmf defines standard fields in the high bits (bits 63–56) of the `mhpmevent` CSR, providing two core capabilities: (1) count overflow detection and interrupt generation (OF bit + LCOFIP/LCOFIE); (2) privilege-mode-based event counting filtering (MINH/SINH/UINH/VSINH/VUINH). It also introduces a read-only `scountovf` CSR, enabling S-mode to quickly query which counters have overflowed.

---

## Test Scope

### Specification Source

- `SPEC/sscofpmf.adoc` — Sscofpmf Extension for Count Overflow and Mode-Based Filtering, Version 1.0

### Key CSRs

| CSR | Address | Description |
|-----|---------|-------------|
| `mhpmevent3`–`mhpmevent31` | 0x323–0x33F | Event selectors; high bits add OF/xINH fields |
| `mhpmcounter3`–`mhpmcounter31` | 0xB03–0xB1F | Hardware performance counters (M-mode read/write) |
| `hpmcounter3`–`hpmcounter31` | 0xC03–0xC1F | Hardware performance counters (S/U-mode read-only shadow) |
| `scountovf` | 0xDA0 | 32-bit read-only; shadow copy of OF bits |
| `mcounteren` | 0x306 | M-mode control of S-mode counter access |
| `hcounteren` | 0x606 | HS-mode control of VS-mode counter access |
| `mip` / `sip` | 0x344 / 0x144 | Interrupt pending; bit 13 = LCOFIP |
| `mie` / `sie` | 0x304 / 0x104 | Interrupt enable; bit 13 = LCOFIE |
| `mideleg` | 0x303 | Interrupt delegation; bit 13 controls LCOFI delegation to S-mode |

### Key Reference Files

| Path | Description |
|------|-------------|
| `SPEC/sscofpmf.adoc` | Full Sscofpmf specification |
| `common/test_framework.h` | Test framework (TEST_BEGIN / TEST_ASSERT / TEST_END) |
| `common/encoding.h` | CSR address definitions (requires additions for mhpmevent/hpmcounter/scountovf, etc.) |
| `common/csr_accessors.c` | Dynamic CSR read/write (requires additions for mhpmevent/mhpmcounter/scountovf) |
| `common/trap.c` | Trap handler (requires enhanced interrupt identification capability) |

### mhpmevent High-Bit Field Layout

```
  63   62   61   60   59   58   57   56
┌────┬────┬────┬────┬────┬────┬────┬────┐
│ OF │MINH│SINH│UINH│VSINH│VUINH│WPRI│WPRI│
└────┴────┴────┴────┴────┴────┴────┴────┘
```

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:mhpmevent_inh_op` | Each of the five `x`INH bits, when set, inhibit counting of events while in privilege mode `x`. All-zeroes for these bits results in counting of events in all modes. |
| `norm:mhpmevent_of_op` | The OF bit is set when the corresponding hpmcounter overflows, and remains set until written by software. |
| `norm:hpmcounter_overflow` | Since hpmcounter values are unsigned values, overflow is defined as unsigned overflow of the implemented counter bits. Note that there is no loss of information after an overflow since the counter wraps around and keeps counting while the sticky OF bit remains set. |
| `norm:count_overflow_interrupt` | If an hpmcounter overflows while the associated OF bit is zero, then a "count overflow interrupt request" is generated. If the OF bit is one, then no interrupt request is generated. Consequently the OF bit also functions as a count overflow interrupt disable for the associated hpmcounter. |
| `norm:count_overflow_trigger` | Count overflow never results from writes to the mhpmcounter_n or mhpmevent_n registers, only from hardware increments of counter registers. |
| `norm:mhpmevent_of_bit_set` | Generation of a count-overflow-interrupt request by an `hpmcounter` sets the associated OF bit. |
| `norm:LCOFIP_op` | When an OF bit is set, it eventually, but not necessarily immediately, sets the LCOFIP bit in the `mip`/`sip` registers. |
| `norm:scountovf_op` | This extension adds the `scountovf` CSR, a 32-bit read-only register that contains shadow copies of the OF bits in the 29 mhpmevent CSRs (mhpmevent_3 - mhpmevent_31) - where scountovf bit X corresponds to mhpmevent_X. |
| `norm:scountovf_smode_read_access_control` | Read access to bit X is subject to the same mcounteren (or mcounteren and hcounteren) CSRs that mediate access to the hpmcounter CSRs by S-mode (or VS-mode). |
| `norm:scountovf_mmode_read_access` | In M-mode, scountovf bit X is always readable. |
| `norm:scountovf_smode_read_access` | In S/HS-mode, scountovf bit X is readable when mcounteren bit X is set, and otherwise reads as zero. |
| `norm:scountovf_vsmode_read_access` | Similarly, in VS mode, scountovf bit X is readable when mcounteren bit X and hcounteren bit X are both set, and otherwise reads as zero. |

### Out of Scope

- **Counter event selection correctness**: The low-bit event selector fields of mhpmevent are implementation-defined and not within the scope of Sscofpmf standardization.
- **Vectored-mode interrupt dispatch details**: Only verifies that the LCOFI interrupt can be correctly generated and captured; does not verify jump offsets in vectored mode.
- **Multi-hart scenarios**: The project targets a single-core test environment.
- **RV32 / mhpmeventh**: Only RV64 is covered (RV32 requires mhpmeventh CSR access for the upper 32 bits).
- **Exact counter value verification**: Does not verify the precise number of counter increments; only verifies overflow behavior.

---

## Prerequisites and Constraints

> [!IMPORTANT]
> The following framework-level support must be in place before test code can be implemented (see "Design Points" section for details):
> 1. `common/encoding.h` requires new mhpmevent/mhpmcounter/scountovf CSR addresses and field masks.
> 2. `common/csr_accessors.c` requires new dynamic read/write cases for mhpmevent/mhpmcounter/scountovf.
> 3. `common/trap.c` requires enhanced identification and recording of asynchronous interrupts (interrupt bit).

### Counter Discovery Strategy

Since different implementations support varying numbers of hpmcounters (3–31), test cases employ **dynamic discovery**: in M-mode, attempt to write a non-zero value to `mhpmcounter` and read it back; if the read-back value is zero, the counter is considered unimplemented and the corresponding test is skipped.

### Event Trigger Strategy

To reliably increment counters, the tests use the following strategy:
1. **Configure mhpmevent for the "retired instructions" event** (event number is implementation-defined; typically event=2 on Spike)
2. **Execute a known instruction sequence** to produce a predictable counter increment.
3. **Initialize mhpmcounter to a value near the maximum** (e.g., `0xFFFFFFFF_FFFFFFFE`) to quickly trigger overflow.

---

## Test Groups

### Group 1: mhpmevent Field Read/Write Verification (M-mode CSR Read/Write Round-Trip)

**Spec Reference**:
- `norm:mhpmevent_of_op`: OF bit is writable and readable.
- `norm:mhpmevent_inh_op`: xINH bits are writable and readable.
- xINH bits corresponding to unimplemented privilege modes are read-only zero.

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

---

### Group 2: Privilege Mode Filtering

**Spec Reference**:
- `norm:mhpmevent_inh_op`: When an xINH bit is set, event counting is inhibited in the corresponding privilege mode; when all are zero, counting occurs in all modes.

**Test Scope**: Verify that each xINH bit correctly inhibits or permits event counting in the corresponding privilege mode.

**Prerequisites**: At least one hpmcounter must be confirmed as implemented, and a usable event number must be identified (on Spike, event=2 for retired instructions).

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
| COFPMF-OVF-09 | LCOFI interrupt delegated to S-mode | Set mideleg bit 13 = 1, trigger overflow interrupt; verify it is caught in the S-mode handler | S-mode trap handler receives cause = interrupt\|13 |
| COFPMF-OVF-10 | LCOFI interrupt handled in M-mode | mideleg bit 13 = 0, trigger overflow interrupt; verify it is caught in the M-mode handler | M-mode trap handler receives cause = interrupt\|13 |
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
- `norm:scountovf_vsmode_read_access`: VS-mode access is controlled by both mcounteren and hcounteren.
- `norm:scountovf_smode_read_access_control`: Access control is consistent with the mcounteren/hcounteren rules for hpmcounter.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| COFPMF-SOV-01 | scountovf reflects OF bit | Set mhpmevent3 OF=1, read scountovf; verify bit 3 = 1 | scountovf bit 3 = 1 |
| COFPMF-SOV-02 | scountovf read-only verification | Attempt to write scountovf (csrw in S-mode); should trigger illegal instruction | Triggers CAUSE_ILLEGAL_INST |
| COFPMF-SOV-03 | scountovf multi-bit mapping | Set mhpmevent3 and mhpmevent5 OF=1 simultaneously; verify scountovf bit 3 and bit 5 are both 1 | scountovf = (1<<3) \| (1<<5) |
| COFPMF-SOV-04 | scountovf clear tracking | Set OF=1 then clear mhpmevent3 OF; read scountovf bit 3 | scountovf bit 3 = 0 |
| COFPMF-SOV-05 | M-mode read is not restricted by mcounteren | Clear mcounteren bit 3; M-mode read of scountovf bit 3 still reflects the true value | scountovf bit 3 reflects the true OF value |
| COFPMF-SOV-06 | S-mode readable when mcounteren=1 | Set mcounteren bit 3 = 1; S-mode reads scountovf bit 3 | Reads the true OF value |
| COFPMF-SOV-07 | S-mode reads zero when mcounteren=0 | Clear mcounteren bit 3; S-mode reads scountovf; bit 3 should be 0 (even if OF=1) | scountovf bit 3 = 0 |
| COFPMF-SOV-08 | VS-mode double gate (both permit) | mcounteren bit 3 = 1, hcounteren bit 3 = 1; VS-mode reads scountovf bit 3 | Reads the true OF value |
| COFPMF-SOV-09 | VS-mode reads zero when mcounteren=0 | mcounteren bit 3 = 0 (regardless of hcounteren); VS-mode reads scountovf bit 3 | scountovf bit 3 = 0 |
| COFPMF-SOV-10 | VS-mode reads zero when hcounteren=0 | mcounteren bit 3 = 1, hcounteren bit 3 = 0; VS-mode reads scountovf bit 3 | scountovf bit 3 = 0 |

---

## Design Points

### 1. Dynamic Counter Discovery

Since the number of implemented hpmcounter3–31 CSRs is optional, all test cases must dynamically probe whether the target counter exists before proceeding:

If a counter is not implemented, the test case should use `TEST_SKIP()` to skip.

### 2. Event Configuration Strategy

Different implementations use different low-bit event numbers for mhpmevent. Tests should provide a platform-configurable event number macro:

### 3. Trap Handler Enhancement for Interrupt Testing

The current `common/trap.c` primarily handles synchronous exceptions. To support LCOFI interrupt testing (Group 3), the following is required:

1. **Trap handler interrupt identification**: Check the most significant bit of `mcause` (interrupt bit); if set to 1, the trap is an interrupt.
2. **Record interrupt information**: Log the interrupt cause (low bits) to `trap_record`.
3. **LCOFIP clear**: Clear `mip.LCOFIP` in the handler to prevent an infinite interrupt loop.
4. **LCOFIE management**: Provide `lcofie_enable()` / `lcofie_disable()` helper functions.

### 4. Overflow Trigger Method

To reliably trigger overflow, the recommended approach is:

Set the counter to a value near overflow and execute a sufficient number of instructions. `MARGIN` is recommended to be set to 50–100, enough to cover loop overhead.

### 5. scountovf Access Control Testing (Group 4 SOV-06 to SOV-10)

Reading `scountovf` in different privilege modes is required. For S-mode tests, the framework's `goto_priv(PRIV_S)` + `PRIV_DO_NO_TRAP` pattern can be reused; VS-mode tests require Hypervisor extension support (`ENABLE_HYP`), using `goto_priv(PRIV_VS)` to enter VS-mode.

---

## Framework Adjustment Checklist

> [!IMPORTANT]
> The following adjustments are prerequisites for test implementation.

### 1. `common/encoding.h` New Definitions

New CSR address definitions are required for mhpmevent3–31 (0x323–0x33F), mhpmcounter3–31 (0xB03–0xB1F), hpmcounter3–31 (0xC03–0xC1F), scountovf (0xDA0), mhpmevent high-bit field masks for RV64 (MHPMEVENT_OF, MHPMEVENT_MINH, MHPMEVENT_SINH, MHPMEVENT_UINH, MHPMEVENT_VSINH, MHPMEVENT_VUINH), and LCOFI interrupt bit definitions (MIP_LCOFIP, MIE_LCOFIE, IRQ_LCOFI = 13).

### 2. `common/csr_accessors.c` New Cases

Add to `csr_read()`:
- `0x323–0x33F` (mhpmevent3–31)
- `0xB03–0xB1F` (mhpmcounter3–31)
- `0xDA0` (scountovf, read-only)

Add to `csr_write()`:
- `0x323–0x33F` (mhpmevent3–31)
- `0xB03–0xB1F` (mhpmcounter3–31)

### 3. `common/trap.c` Interrupt Support

Add an interrupt branch in the M-mode trap handler (`m_trap_handler`): check the interrupt bit of `mcause`; if it is an interrupt and the IRQ is IRQ_LCOFI, clear LCOFIP to prevent an infinite loop, record the interrupt information to `trap_record`, and return without adjusting `epc` since interrupt handling is complete.

---

## Test File Organization

```
sscofpmf/
├── Makefile                      # Build configuration (SPIKE_ISA_EXT = _sscofpmf)
├── main.c                        # Entry point; prints banner, iterates over test_table
├── sscofpmf_helpers.h            # Helper macros/functions (event number, counter discovery)
└── tests/
    ├── test_register.c           # Group 1: mhpmevent field read/write
    ├── test_mode_filter.c        # Group 2: privilege mode filtering
    ├── test_overflow.c           # Group 3: count overflow and interrupts
    └── test_scountovf.c          # Group 4: scountovf register
```
