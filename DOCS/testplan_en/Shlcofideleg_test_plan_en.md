**[中文](../testplan/Shlcofideleg_test_plan.md) | English**

# Shlcofideleg Extension Test Plan

This document describes the test plan for the Shlcofideleg (LCOFI Interrupt Delegation to VS-mode) extension. The Shlcofideleg extension supports delegating LCOFI (Local Count Overflow Interrupt) to VS-mode, enabling the hypervisor to directly inject hardware performance counter overflow interrupts into guests.

---

## Overview

The RISC-V Sscofpmf extension defines the LCOFI interrupt (corresponding to interrupt bit 13), which generates an interrupt request when a hardware performance counter overflows. This interrupt indicates pending status via the LCOFIP bit in `mip`/`sip`, and controls enablement via the LCOFIE bit in `mie`/`sie`.

In the context of the H extension, the `hideleg` register controls which interrupts are further delegated from HS-mode to VS-mode. **Core constraints of the Shlcofideleg extension**:

> 1. If Shlcofideleg is implemented, bit 13 of `hideleg` is writable; otherwise, this bit is read-only zero.
> 2. When `hideleg` bit 13 = 0, `vsip`.LCOFIP and `vsie`.LCOFIE are read-only zero.
> 3. When `hideleg` bit 13 = 1, `vsip`.LCOFIP and `vsie`.LCOFIE are aliases of `sip`.LCOFIP and `sie`.LCOFIE respectively.

---

## Test Scope

### Specification Sources

- `SPEC/hypervisor.adoc:1276-1283` — `norm:vsip_vsie_lcofi`: Behavior definition of hideleg[13], vsip.LCOFIP, vsie.LCOFIE under the Shlcofideleg extension
- `SPEC/hypervisor.adoc:1248-1275` — Basic definition of vsip/vsie registers (VSXLEN-bit R/W, substitute for sip/sie when V=1)
- `SPEC/sscofpmf.adoc` — Sscofpmf extension: LCOFI interrupt definition (bit 13), OF bit semantics

### Key Reference Files

| Path | Description |
|------|-------------|
| `SPEC/hypervisor.adoc:1276-1283` | `norm:vsip_vsie_lcofi` — Core specification of Shlcofideleg |
| `SPEC/hypervisor.adoc:1248-1258` | vsip/vsie register size, access attributes, and V=1 substitution semantics |
| `SPEC/hypervisor.adoc:1285-1300` | `norm:vsip_vsie_sei/sti/ssi` — Similar delegation patterns for SEI/STI/SSI (structural reference) |
| `SPEC/sscofpmf.adoc` | LCOFI interrupt definition: bit 13, LCOFIP/LCOFIE semantics |
| `common/encoding.h:271` | `CSR_HIDELEG = 0x603` |
| `common/encoding.h:290-291` | `CSR_VSIP = 0x244` / `CSR_VSIE = 0x204` |
| `common/encoding.h:144-145` | `CSR_SIP = 0x144` / `CSR_SIE = 0x104` |
| `Sscofpmf/sscofpmf_encoding.h` | `MIP_LCOFIP` / `IRQ_LCOFI = 13` definitions |
| `common/encoding.h:317` | `STATEEN_BIT58` — Sscofpmf/LCOFI state control bit |
| `common/hyp/hyp_csr.h:53-54` | `hideleg_write()` / `hideleg_read()` |
| `common/hyp/hyp_priv.h:21,24` | `run_in_vs_mode(fn, arg)` / `run_in_vu_mode(fn, arg)` |
| `common/hyp/hyp_test.h:95-100` | `VS_EXPECT_NO_TRAP(stmt)` — Expect no exception |
| `DOCS/framework/hypervisor_framework.md` | Hypervisor test framework overview |

### Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:vsip_vsie_lcofi` (R1) | Extension Shlcofideleg supports delegating LCOFI interrupts to VS-mode. If implemented, `hideleg` bit 13 is writable; otherwise read-only zero. |
| `norm:vsip_vsie_lcofi` (R2) | When bit 13 of `hideleg` is zero, `vsip`.LCOFIP and `vsie`.LCOFIE are read-only zeros. |
| `norm:vsip_vsie_lcofi` (R3) | Else, they are aliases of `sip`.LCOFIP and `sie`.LCOFIE. |
| `norm:vsip_vsie_sz_acc_op` | The `vsip` and `vsie` registers are VSXLEN-bit read/write registers that are VS-mode's versions of supervisor CSRs `sip` and `sie`. When V=1, `vsip` and `vsie` substitute for the usual `sip` and `sie`. However, interrupts directed to HS-level continue to be indicated in the HS-level `sip` register, not in `vsip`, when V=1. |

### Out of Scope

- **Actual triggering and handling of LCOFI interrupts**: Counter overflow generating LCOFI is covered by Sscofpmf tests
- **Control of sip/sie visibility by mideleg[13]**: Belongs to basic privilege specification, not specific to Shlcofideleg
- **Delegation behavior of other hideleg bits**: SEI(bit 10)/STI(bit 6)/SSI(bit 2) delegation is covered by basic H extension tests
- **OF bit and counter overflow mechanism of Sscofpmf extension**: Covered by separate Sscofpmf test plan
- **Multi-hart scenarios**: Project uses single-core test environment
- **RV32 scenarios**: Only RV64 is covered

---

## Prerequisites and Constraints

> [!IMPORTANT]
> Testing Shlcofideleg requires the following prerequisites:
> 1. **Sscofpmf is implemented**: The existence of LCOFI interrupt (bit 13) is a prerequisite for Shlcofideleg
> 2. **mideleg[13] = 1**: LCOFI interrupt must first be delegated from M-mode to S/HS-mode for sip/sie LCOFIP/LCOFIE bits to be visible
> 3. **Use M-mode to operate mip.LCOFIP**: LCOFIP pending bit is set by hardware; in tests, it is manually injected via M-mode `CSRS(mip, MIP_LCOFIP)` to verify alias relationships

### Pre-check Strategy

```
1. Verify Sscofpmf is implemented: Check if bit 13 of mip/mie is operable
2. Set mideleg[13] = 1: Make sip.LCOFIP / sie.LCOFIE visible in HS-mode
3. Verify hideleg[13] is writable (Shlcofideleg existence check):
   Write 1 and read back; if 0, then TEST_SKIP("Shlcofideleg not implemented")
```

---

## Design Key Points

### 1. LCOFI Interrupt and Interrupt Delegation Hierarchy

The delegation chain for LCOFI interrupt (bit 13) is `mideleg → hideleg`:

- **mideleg[13]=1**: Delegates LCOFI from M-mode to HS-mode; `sip`.LCOFIP and `sie`.LCOFIE become visible mappings of `mip`.LCOFIP and `mie`.LCOFIE
- **hideleg[13]=1** (Shlcofideleg): Further delegates LCOFI from HS-mode to VS-mode; `vsip`.LCOFIP and `vsie`.LCOFIE become aliases of `sip`.LCOFIP and `sie`.LCOFIE

### 2. Verification Strategy for Alias Semantics

"Alias" means two CSR bits point to the same underlying hardware bit. Verification methods:

- **Forward alias**: Modify `sip`.LCOFIP → read `vsip`.LCOFIP; values should match
- **Reverse alias**: Modify `vsip`.LCOFIP → read `sip`.LCOFIP; values should match
- **Bidirectional consistency**: At any time, both read values are identical

### 3. VS-mode Perspective

When V=1, VS-mode executing `csrr rd, sip` actually accesses `vsip`:

- When hideleg[13]=1: VS-mode can see the true value of LCOFIP via `sip` instruction
- When hideleg[13]=0: VS-mode always sees LCOFIP as 0 via `sip` instruction

### 4. Operation Method for LCOFIP

The `LCOFIP` pending bit is typically set by hardware upon counter overflow. In tests, we simulate interrupt pending by directly writing bit 13 of `mip` from M-mode, avoiding dependency on actual counter overflow. The `LCOFIE` enable bit can be freely read/written from HS-mode via `sie`.

---

## Test Groups

> [!IMPORTANT]
> Total of 5 test groups with 23 test cases. Group 1 verifies writability of hideleg[13]; Group 2 verifies vsip/vsie LCOFI bits are read-only zero when hideleg[13]=0; Group 3 verifies alias relationships when hideleg[13]=1; Group 4 verifies end-to-end behavior from VS-mode perspective; Group 5 verifies state consistency during dynamic hideleg switching.

---

### Group 1: hideleg[13] Writability Verification

**Specification Basis**:
- `norm:vsip_vsie_lcofi` (R1): If Shlcofideleg is implemented, hideleg bit 13 is writable

**Test Responsibility**: Verify in M-mode that hideleg bit 13 can be written to 1 and written to 0.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| LCFIDLG-WR-01 | hideleg[13] can be set to 1 | Write hideleg bit 13 = 1, read back | bit 13 = 1 |
| LCFIDLG-WR-02 | hideleg[13] can be cleared to 0 | Write hideleg bit 13 = 0, read back | bit 13 = 0 |

---

### Group 2: vsip/vsie LCOFI Bits Read-Only Zero When hideleg[13]=0

**Specification Basis**:
- `norm:vsip_vsie_lcofi` (R2): When hideleg[13]=0, vsip.LCOFIP and vsie.LCOFIE are read-only zero

**Test Responsibility**: When hideleg[13]=0, regardless of LCOFI bits in sip/sie, corresponding bits in vsip/vsie always read as 0 and cannot be written.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| LCFIDLG-RO-01 | vsip.LCOFIP reads 0 | hideleg[13]=0, read vsip bit 13 | vsip.LCOFIP = 0 |
| LCFIDLG-RO-02 | vsie.LCOFIE reads 0 | hideleg[13]=0, read vsie bit 13 | vsie.LCOFIE = 0 |
| LCFIDLG-RO-03 | vsip.LCOFIP write has no effect | hideleg[13]=0, write vsip bit 13 = 1, read back | vsip.LCOFIP remains 0 |
| LCFIDLG-RO-04 | vsie.LCOFIE write has no effect | hideleg[13]=0, write vsie bit 13 = 1, read back | vsie.LCOFIE remains 0 |
| LCFIDLG-RO-05 | sip.LCOFIP=1 does not affect vsip | hideleg[13]=0, M-mode sets mip.LCOFIP=1, read vsip bit 13 | vsip.LCOFIP = 0 |
| LCFIDLG-RO-06 | sie.LCOFIE=1 does not affect vsie | hideleg[13]=0, set sie.LCOFIE=1, read vsie bit 13 | vsie.LCOFIE = 0 |

---

### Group 3: Alias Verification Between vsip/vsie and sip/sie When hideleg[13]=1

**Specification Basis**:
- `norm:vsip_vsie_lcofi` (R3): When hideleg[13]=1, vsip.LCOFIP is an alias of sip.LCOFIP, vsie.LCOFIE is an alias of sie.LCOFIE

**Test Responsibility**: After setting hideleg[13]=1, verify bidirectional synchronization of LCOFI bits between vsip/vsie and sip/sie.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| LCFIDLG-ALIAS-01 | sip→vsip forward alias (LCOFIP) | Set sip.LCOFIP=1, read vsip.LCOFIP | vsip.LCOFIP = 1 |
| LCFIDLG-ALIAS-02 | sip→vsip forward alias clear | Clear sip.LCOFIP=0, read vsip.LCOFIP | vsip.LCOFIP = 0 |
| LCFIDLG-ALIAS-03 | sie→vsie forward alias (LCOFIE) | Set sie.LCOFIE=1, read vsie.LCOFIE | vsie.LCOFIE = 1 |
| LCFIDLG-ALIAS-04 | sie→vsie forward alias clear | Clear sie.LCOFIE=0, read vsie.LCOFIE | vsie.LCOFIE = 0 |
| LCFIDLG-ALIAS-05 | vsie→sie reverse alias (LCOFIE) | Set vsie.LCOFIE=1, read sie.LCOFIE | sie.LCOFIE = 1 |
| LCFIDLG-ALIAS-06 | vsie→sie reverse alias clear | Clear vsie.LCOFIE=0, read sie.LCOFIE | sie.LCOFIE = 0 |
| LCFIDLG-ALIAS-07 | mip.LCOFIP→vsip propagation | M-mode sets mip.LCOFIP=1 (mideleg[13]=1), read vsip.LCOFIP | vsip.LCOFIP = 1 |

> [!NOTE]
> **Write restriction for LCOFIP**: `sip.LCOFIP` may be read-only in some implementations (set by hardware via counter overflow, cleared via `mip`). Tests inject LCOFIP=1 via M-mode `CSRS(mip, MIP_LCOFIP)`, then verify aliases from HS-mode perspective via `sip` and VS-mode perspective via `vsip`.

> [!NOTE]
> **Reverse write to vsip.LCOFIP**: LCOFIP is set by hardware; in the spec, `sip.LCOFIP` may be a read-only bit (writes require going through mip). Therefore, reverse alias from vsip→sip only applies to LCOFIE (enable bit), not to LCOFIP (pending bit).

---

### Group 4: VS-mode End-to-End Verification

**Specification Basis**:
- `norm:vsip_vsie_sz_acc_op`: When V=1, `csrr sip` / `csrr sie` actually access vsip / vsie
- `norm:vsip_vsie_lcofi` (R2)(R3): hideleg[13] controls visibility of LCOFI bits to VS-mode

**Test Responsibility**: In VS-mode (V=1), verify actual visibility of LCOFI bits via `csrr sip` / `csrr sie` instructions, confirming end-to-end behavior of vsip/vsie.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| LCFIDLG-VS-01 | VS-mode reads sip and sees LCOFIP (hideleg[13]=1) | hideleg[13]=1, M-mode injects LCOFIP=1, VS-mode csrr sip | sip (actually vsip) bit 13 = 1 |
| LCFIDLG-VS-02 | VS-mode reads sip and cannot see LCOFIP (hideleg[13]=0) | hideleg[13]=0, M-mode injects LCOFIP=1, VS-mode csrr sip | sip (actually vsip) bit 13 = 0 |
| LCFIDLG-VS-03 | VS-mode reads sie and sees LCOFIE (hideleg[13]=1) | hideleg[13]=1, HS sets sie.LCOFIE=1, VS-mode csrr sie | sie (actually vsie) bit 13 = 1 |
| LCFIDLG-VS-04 | VS-mode reads sie and cannot see LCOFIE (hideleg[13]=0) | hideleg[13]=0, HS sets sie.LCOFIE=1, VS-mode csrr sie | sie (actually vsie) bit 13 = 0 |
| LCFIDLG-VS-05 | VS-mode write to sie.LCOFIE takes effect (hideleg[13]=1) | hideleg[13]=1, VS-mode csrs sie LCOFI_BIT, HS-mode reads sie | sie.LCOFIE = 1 (due to alias) |
| LCFIDLG-VS-06 | VS-mode write to sie.LCOFIE has no effect (hideleg[13]=0) | hideleg[13]=0, VS-mode csrs sie LCOFI_BIT, HS-mode reads sie | sie.LCOFIE unchanged (because vsie is RO-zero) |

---

### Group 5: hideleg Dynamic Switching Consistency

**Specification Basis**:
- `norm:vsip_vsie_lcofi` (R2)(R3): Changes to hideleg[13] should immediately reflect in vsip/vsie behavior

**Test Responsibility**: Verify that when hideleg[13] switches between 1 and 0, vsip/vsie LCOFI bit behavior follows changes immediately and correctly.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| LCFIDLG-DYN-01 | vsip.LCOFIP becomes 0 after hideleg 1→0 switch | First set hideleg[13]=1 to verify vsip visibility, then clear to 0 | After switch, vsip.LCOFIP = 0 |
| LCFIDLG-DYN-02 | vsip.LCOFIP becomes visible after hideleg 0→1 switch | First set hideleg[13]=0, inject LCOFIP, then set to 1 | After switch, vsip.LCOFIP = 1 |
| LCFIDLG-DYN-03 | vsie.LCOFIE consistency during repeated hideleg toggling | Repeatedly toggle hideleg[13], verify vsie.LCOFIE behavior each time | Behavior matches current hideleg[13] after each toggle |
| LCFIDLG-DYN-04 | hideleg switching does not affect sip.LCOFIP | Toggle hideleg[13], verify sip.LCOFIP remains unchanged | sip.LCOFIP always reflects true pending status |

> [!NOTE]
> **Significance of DYN-04**: hideleg only controls whether interrupts are further delegated to VS-mode (i.e., whether vsip/vsie act as aliases); it does not affect the values of sip/sie themselves. After toggling hideleg[13], sip.LCOFIP should remain unchanged.

---

## Exception Scenarios and Edge Cases

### 1. Behavior When Shlcofideleg Is Not Implemented

When the platform does not implement Shlcofideleg:
- hideleg[13] should be read-only zero
- Writing hideleg[13]=1 and reading back yields 0
- All Group 2-5 tests are not applicable; should use `TEST_SKIP`

### 2. Behavior When mideleg[13] Is Not Delegated

If mideleg[13]=0 (LCOFI not delegated from M-mode to HS-mode):
- sip.LCOFIP and sie.LCOFIE are not visible to HS-mode
- Delegation effect of hideleg[13]=1 is meaningless
- Test pre-checks must ensure mideleg[13]=1

### 3. Sscofpmf Not Implemented

If the platform does not support Sscofpmf:
- Bit 13 in mip/sie may not exist
- Test pre-checks should verify bit 13 is operable; otherwise `TEST_SKIP`

### 4. Interaction with Smstateen

If Smstateen/Ssstateen is implemented:
- `mstateen0` bit 58 (STATEEN_BIT58) controls access to Sscofpmf-related state
- When `hstateen0` bit 58 = 0, VS-mode accessing LCOFI-related state may trigger virtual-instruction exception
- This test plan does not cover this interaction (covered by Ssstateen tests), but test prerequisites should ensure `mstateen0`/`hstateen0` bit 58 = 1

---

## Appendix: Related Constants and API Reference

### CSR and Bit Definitions

| Name | Value | Description |
|------|-------|-------------|
| `CSR_HIDELEG` | `0x603` | Hypervisor interrupt delegation register |
| `CSR_MIDELEG` | `0x303` | Machine interrupt delegation register |
| `CSR_VSIP` | `0x244` | Virtual supervisor interrupt-pending register |
| `CSR_VSIE` | `0x204` | Virtual supervisor interrupt-enable register |
| `CSR_SIP` | `0x144` | Supervisor interrupt-pending register |
| `CSR_SIE` | `0x104` | Supervisor interrupt-enable register |
| `CSR_MIP` | `0x344` | Machine interrupt-pending register |
| `CSR_MIE` | `0x304` | Machine interrupt-enable register |
| `IRQ_LCOFI` | `13` | LCOFI interrupt number |
| `MIP_LCOFIP` | `1UL << 13` | LCOFIP bit mask in mip/sip |
| `MIE_LCOFIE` | `1UL << 13` | LCOFIE bit mask in mie/sie |
| `STATEEN_BIT58` | `1ULL << 58` | Sscofpmf/LCOFI control bit in mstateen0/hstateen0 |

### Relationship Between hideleg bit 13 and LCOFI

| hideleg[13] | vsip.LCOFIP | vsie.LCOFIE | Behavior |
|-------------|-------------|-------------|----------|
| 0 | read-only 0 | read-only 0 | LCOFI not delegated to VS-mode |
| 1 | alias of sip.LCOFIP | alias of sie.LCOFIE | LCOFI delegated to VS-mode |

### Comparison of Similar Delegation Patterns (Structural Reference)

| hideleg bit | Interrupt Type | vsip Alias | vsie Alias | Specification Label |
|-------------|----------------|------------|------------|---------------------|
| bit 2 | SSI | hip.VSSIP | hie.VSSIE | `norm:vsip_vsie_ssi` |
| bit 6 | STI | hip.VSTIP | hie.VSTIE | `norm:vsip_vsie_sti` |
| bit 10 | SEI | hip.VSEIP | hie.VSEIE | `norm:vsip_vsie_sei` |
| bit 13 | LCOFI | sip.LCOFIP | sie.LCOFIE | `norm:vsip_vsie_lcofi` |

> [!NOTE]
> **LCOFI alias target differs from SSI/STI/SEI**: After SSI/STI/SEI delegation, vsip/vsie alias to VS* bits in `hip`/`hie` (e.g., hip.VSEIP). However, after LCOFI delegation, vsip/vsie alias to LCOFIP/LCOFIE bits in `sip`/`sie`, without intermediate VS* bits. This is a key distinction in test verification.

### Test Framework API

- `hideleg_read()` / `hideleg_write(value)`: hideleg read/write
- `run_in_vs_mode(fn, arg)`: Execute fn(arg) in VS-mode (V=1)
- `csr_read(addr)` / `csr_write(addr, val)`: Dynamic CSR read/write
- `CSRS(csr, mask)` / `CSRC(csr, mask)`: CSR set/clear bits (compile-time CSR name)
- `CSRR(csr)`: CSR read (compile-time CSR name)
- `trap_expect_begin()` / `trap_expect_end()`: Trap capture window
- `trap_was_triggered()` / `trap_get_cause()`: Trap status query
- `HYP_TEST_END()`: Test end macro (includes hyp_reset_state)
- `TEST_SKIP(msg)`: Skip test (when platform does not support)

---

## Test Statistics

| Group | Test Count | Description |
|-------|------------|-------------|
| Group 1: hideleg[13] Writability | 2 | Basic read-write loopback |
| Group 2: Read-only zero when hideleg[13]=0 | 6 | Read-only zero verification for vsip/vsie LCOFI bits |
| Group 3: Alias verification when hideleg[13]=1 | 7 | Bidirectional alias between sip↔vsip / sie↔vsie |
| Group 4: VS-mode End-to-End | 6 | Actual behavior of csrr sip/sie when V=1 |
| Group 5: Dynamic Switching Consistency | 4 | Immediate consistency after hideleg switching |
| **Total** | **25** | |
