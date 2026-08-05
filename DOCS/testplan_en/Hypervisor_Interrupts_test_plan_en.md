**[中文](../testplan/Hypervisor_Interrupts_test_plan.md) | English**

# Hypervisor Interrupts Test Plan (Hypervisor_Interrupts Subset)

> This document is split from the original Hypervisor comprehensive test plan (Hypervisor_test_plan_en.md, now removed). Test case IDs remain unchanged; Group numbers are re-sequenced within this subset.
> Sibling subsets: [Hypervisor_CSR_test_plan_en.md](Hypervisor_CSR_test_plan_en.md) | [Hypervisor_Exceptions_test_plan_en.md](Hypervisor_Exceptions_test_plan_en.md)

## Overview

This test plan covers the interrupt-related functionality of the RISC-V Hypervisor (H) extension, including virtual interrupt injection (hvip/hip/hie), guest external interrupts (hgeip/hgeie), M-level interrupt register enhancements (mideleg/mip/mie), and hideleg interrupt delegation with VS interrupt number translation. CSR register field behavior and exception/trap behavior are covered by the sibling subsets respectively.

This test plan is written based on specification points (norm tags) in `SPEC/hypervisor.adoc`.

### SPEC Chapters Covered by This Document
- Hypervisor and Virtual Supervisor CSRs (hvip, hip, hie, hgeip, hgeie, hideleg interrupt delegation behavior)
- Machine-Level CSR Enhancements (mideleg, mip/mie)
- Traps (interrupt delegation chain and interrupt number translation)

### Covered by Other Test Plans
- Hypervisor CSR (CSR substitution mechanism, hstatus, henvcfg, htimedelta, VS CSRs, hedeleg/hideleg bit-field attributes) → `Hypervisor_CSR_test_plan_en.md`
- Hypervisor exceptions and traps (virtual-instruction exception, trap entry/return, htinst/mtinst, mstatus enhancements, mtval2/mtinst, exception priority, hedeleg exception delegation) → `Hypervisor_Exceptions_test_plan_en.md`
- G-stage address translation → `hyp_gstage_translation_test_plan.md`
- Two-stage address translation → `hyp_2_stage_translation_test_plan.md`
- Sha combined extension → `sha_test_plan.md`
- Shcounterenw → `shcounterenw_test_plan.md`
- Shgatpa → `shgatpa_test_plan.md`
- Shvsatpa → `shvsatpa_test_plan.md`
- Shtvala → `shtvala_test_plan.md`
- Shvstvala → `shvstvala_test_plan.md`
- Shvstvecd → `shvstvecd_test_plan.md`

---

## Covered Specification Points

This section lists all specification points (norm IDs) referenced in Groups 1-4 of this document, deduplicated and sorted alphabetically.

| Norm ID | English Description |
|---------|---------------------|
| `norm:H_cause` | The hypervisor extension augments the trap cause encoding. Codes are added for VS-level interrupts (2, 6, 10), for supervisor-level guest external interrupts (12), for virtual-instruction exceptions (22), and for guest-page faults (20, 21, 23). Environment calls from VS-mode are assigned cause 10. This suite covers HS-level delivery of VS-level interrupt causes 2/6/10 (HINT-10/21/22). |
| `norm:geilen` | The number of bits implemented in `hgeip` and `hgeie` for guest external interrupts is UNSPECIFIED and may be zero. This number is known as GEILEN. The least-significant bits are implemented first, apart from bit 0. Hence, if GEILEN is nonzero, bits GEILEN:1 shall be writable in `hgeie`, and all other bit positions shall be read-only zeros in both `hgeip` and `hgeie`. |
| `norm:hgeie_op` | Register `hgeie` selects the subset of guest external interrupts that cause a supervisor-level (HS-level) guest external interrupt. The enable bits in `hgeie` do not affect the VS-level external interrupt signal selected from `hgeip` by `hstatus`.VGEIN. |
| `norm:hgeie_sz_acc_op` | The `hgeie` register is an HSXLEN-bit read/write register that contains enable bits for the guest external interrupts at this hart. |
| `norm:hgeip_hgeie_fields` | Guest external interrupt number _i_ corresponds with bit _i_ in both `hgeip` and `hgeie`. |
| `norm:hgeip_sz_acc_op` | The `hgeip` register is an HSXLEN-bit read-only register that indicates pending guest external interrupts for this hart. |
| `norm:hideleg_hs` | An interrupt _i_ will trap to HS-mode whenever all of the following are true: (a) either the current operating mode is HS-mode and the SIE bit in the `sstatus` register is set, or the current operating mode has less privilege than HS-mode; (b) bit _i_ is set in both `sip` and `sie`, or in both `hip` and `hie`; and (c) bit _i_ is not set in `hideleg`. |
| `norm:hideleg_op` | An interrupt that has been delegated to HS-mode (using `mideleg`) is further delegated to VS-mode if the corresponding `hideleg` bit is set. |
| `norm:hideleg_trans` | When a virtual supervisor external interrupt (code 10) is delegated to VS-mode, it is automatically translated by the machine into a supervisor external interrupt (code 9) for VS-mode. Likewise, virtual supervisor timer interrupt (6) is translated into supervisor timer interrupt (5), and virtual supervisor software interrupt (2) into supervisor software interrupt (1). |
| `norm:hie_acc` | A bit in `hie` shall be writable if the corresponding interrupt can ever become pending in `hip`. Bits of `hie` that are not writable shall be read-only zero. |
| `norm:hie_op` | `hie` contains enable bits for the same interrupts. |
| `norm:hip_acc` | If bit _i_ of `sie` is read-only zero, the same bit in register `hip` may be writable or may be read-only. When bit _i_ in `hip` is writable, a pending interrupt _i_ can be cleared by writing 0 to this bit. (The read-only-bit branch is cleared via `hvip`, see HINT-05/06/09.) |
| `norm:hip_hie_sz_acc` | Registers `hip` and `hie` are HSXLEN-bit read/write registers that supplement HS-level's `sip` and `sie` respectively. |
| `norm:hip_op` | The `hip` register indicates pending VS-level and hypervisor-specific interrupts. |
| `norm:hip_vseip_vseie_op` | Bits `hip`.VSEIP and `hie`.VSEIE are the interrupt-pending and interrupt-enable bits for VS-level external interrupts. VSEIP is read-only in `hip`, and is the logical-OR of: bit VSEIP of `hvip`; the bit of `hgeip` selected by `hstatus`.VGEIN; and any other platform-specific external interrupt signal directed to VS-level. |
| `norm:hip_vssip_vssie_op` | Bits `hip`.VSSIP and `hie`.VSSIE are the interrupt-pending and interrupt-enable bits for VS-level software interrupts. VSSIP in `hip` is an alias (writable) of the same bit in `hvip`. |
| `norm:hip_vstip_vstie_acc_op` | Bits `hip`.VSTIP and `hie`.VSTIE are the interrupt-pending and interrupt-enable bits for VS-level timer interrupts. VSTIP is read-only in `hip`, and is the logical-OR of `hvip`.VSTIP and, when the Sstc extension is implemented, the timer interrupt signal resulting from `vstimecmp`. |
| `norm:hsint_priority` | Multiple simultaneous interrupts destined for HS-mode are handled in the following decreasing priority order: SEI, SSI, STI, SGEI, VSEI, VSSI, VSTI, LCOFI. |
| `norm:hvip_acc` | The standard portion (bits 15:0) of `hvip` is formatted as shown. Bits VSEIP, VSTIP, and VSSIP of `hvip` are writable. |
| `norm:hvip_sz_op` | Register `hvip` is an HSXLEN-bit read/write register that a hypervisor can write to indicate virtual interrupts intended for VS-mode. Bits of `hvip` that are not writable are read-only zeros. |
| `norm:mideleg_acc_h` | When the hypervisor extension is implemented, bits 10, 6, and 2 of `mideleg` are each read-only one. Furthermore, if GEILEN is nonzero, bit 12 of `mideleg` is also read-only one. VS-level interrupts and guest external interrupts are always delegated past M-mode to HS-mode. |
| `norm:mideleg_hroz` | For bits of `mideleg` that are zero, the corresponding bits in `hideleg`, `hip`, and `hie` are read-only zeros. |
| `norm:mip_mie_alias` | Bits SGEIP, VSEIP, VSTIP, and VSSIP in `mip` are aliases for the same bits in hypervisor CSR `hip`, while SGEIE, VSEIE, VSTIE, and VSSIE in `mie` are aliases for the same bits in `hie`. |
| `norm:mip_mie_vs` | The hypervisor extension gives registers `mip` and `mie` additional active bits for the hypervisor-added interrupts. |
| `norm:sie_hip_hie_mutex` | For each writable bit in `sie`, the corresponding bit shall be read-only zero in both `hip` and `hie`. Hence, the nonzero bits in `sie` and `hie` are always mutually exclusive, and likewise for `sip` and `hip`. |

---

## Group 1. hvip / hip / hie Interrupt Registers

**Specification References**:
- `norm:hvip_sz_op` / `norm:hvip_acc`: hvip VSEIP/VSTIP/VSSIP are writable
- `norm:hip_hie_sz_acc` / `norm:hip_op` / `norm:hie_op`: hip indicates pending, hie indicates enable
- `norm:sie_hip_hie_mutex`: Writable bits in sie are read-only zero in hip/hie, and vice versa
- `norm:hideleg_hs`: Conditions for interrupt trap to HS-mode
- `norm:hip_vseip_vseie_op`: VSEIP = hvip.VSEIP OR hgeip[VGEIN] OR platform signal
- `norm:hip_vstip_vstie_acc_op`: VSTIP = hvip.VSTIP OR vstimecmp trigger
- `norm:hip_vssip_vssie_op`: hip.VSSIP is an alias of hvip.VSSIP
- `norm:hsint_priority`: HS-mode interrupt priority SEI > SSI > STI > SGEI > VSEI > VSSI > VSTI > LCOFI
- `norm:hip_acc`: clearing a pending interrupt by writing 0 to a writable hip bit (HINT-19); read-only bits cleared via hvip (HINT-05/06/09)
- `norm:hie_acc`: hie writable bits cover hip pending-capable bits; other bits are read-only zero (HINT-20)
- `norm:H_cause`: HS-level delivery cause encodings 2/6/10 for VS-level interrupts (HINT-10/21/22)

**Test Responsibilities**: Verify interrupt injection, pending/enable mechanism, priority, and mutual exclusion relationships.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HINT-01 | hvip.VSSIP write injects VS software interrupt | HS-mode writes hvip.VSSIP=1, configure hideleg/hie/vsie | VS-mode receives VS software interrupt |
| HINT-02 | hvip.VSTIP write injects VS timer interrupt | HS-mode writes hvip.VSTIP=1, configure hideleg/hie/vsie | VS-mode receives VS timer interrupt |
| HINT-03 | hvip.VSEIP write injects VS external interrupt | HS-mode writes hvip.VSEIP=1, configure hideleg/hie/vsie | VS-mode receives VS external interrupt |
| HINT-04 | hip.VSSIP is bidirectional writable alias of hvip.VSSIP | Read direction: write hvip.VSSIP=1, read hip.VSSIP=1; Write direction: clear hvip.VSSIP=0, then write hip.VSSIP=1 directly, read hvip.VSSIP | Bidirectional: writing hip.VSSIP is equivalent to writing hvip.VSSIP |
| HINT-05 | hip.VSEIP read-only (multi-source OR) | Combo 1: hvip.VSEIP=1, write hip.VSEIP=0, hip.VSEIP remains 1; Combo 2: hvip.VSEIP=0, write hip.VSEIP=1, hip.VSEIP remains 0 | Write ignored in both combos; hip.VSEIP always reflects hvip.VSEIP |
| HINT-06 | hip.VSTIP read-only | Combo 1: hvip.VSTIP=1, write hip.VSTIP=0, hip.VSTIP remains 1; Combo 2: hvip.VSTIP=0, write hip.VSTIP=1, hip.VSTIP remains 0 | Write ignored in both combos; hip.VSTIP always reflects hvip.VSTIP |
| HINT-07 | hie VSEIE/VSTIE/VSSIE writable | Write hie VSEIE/VSTIE/VSSIE bits | Normal read/write |
| HINT-08 | sie and hip/hie mutual exclusion | Check if writable bits in sie are read-only zero in hip/hie | Mutual exclusion relationship holds |
| HINT-09 | Clearing hvip.VSSIP clears interrupt | Write hvip.VSSIP=0 | VS software interrupt cleared |
| HINT-10 | Interrupt traps to HS when hideleg=0 | hideleg[2]=0, inject VSSIP | Interrupt traps to HS-mode |
| HINT-11 | Interrupt traps to VS when hideleg=1 | hideleg[2]=1, inject VSSIP | Interrupt traps to VS-mode |
| HINT-12 | HS-mode interrupt priority SEI > SSI | SEI and SSI pending simultaneously | SEI handled first |
| HINT-13 | HS-mode interrupt priority VSEI > VSSI > VSTI | Multiple VS interrupts pending simultaneously | In order VSEI > VSSI > VSTI |
| HINT-14 | hip/hie non-standard bits read-only zero | Read reserved bits of hip/hie | All zero |
| HINT-15 | sstatus.SIE=0 masks interrupt in HS-mode | hideleg[2]=0, inject VSSIP, hie.VSSIE=1, enter HS-mode with SIE=0 | Interrupt not delivered; then set SIE=1, interrupt fires immediately |
| HINT-16 | hvip non-writable bits read-only zero | Write hvip all 1s, read back | Only bits 2/6/10 (VSSIP/VSTIP/VSEIP) are 1, all others are 0 |
| HINT-17 | hip.VSTIP remains defined at V=0 | In V=0 (HS-mode), set/clear hvip.VSTIP, read hip.VSTIP | hip.VSTIP reflects hvip.VSTIP at V=0 (defined behavior) |
| HINT-18 | HS-mode interrupt priority SSI > STI | Both SSI and STI pending and enabled, enter HS-mode | SSI (cause=1) delivered first, proving SSI > STI |
| HINT-19 | Writable hip bit cleared by writing 0 | Inject VSSIP -> hip.VSSIP=1; write hip.VSSIP=0 directly | hip.VSSIP=0, hvip.VSSIP=0 (alias), and the interrupt is no longer delivered to HS-mode (`norm:hip_acc`) |
| HINT-20 | hie writable bits cover hip pending-capable bits | Probe hvip writable bits (pending-capable set) and hie writable bits (write all-1s readback) | writable(hie) covers writable(hvip) restricted to the base VS interrupt bits; non-writable bits read zero (`norm:hie_acc`) |
| HINT-21 | VSTIP delivery cause encoding at HS-mode | hvip.VSTIP injection + hideleg[6]=0 + hie.VSTIE enabled, enter HS-mode | HS receives the interrupt with cause = interrupt\|6 (`norm:H_cause`) |
| HINT-22 | VSEIP delivery cause encoding at HS-mode | hvip.VSEIP injection + hideleg[10]=0 + hie.VSEIE enabled, enter HS-mode | HS receives the interrupt with cause = interrupt\|10 (`norm:H_cause`) |

---

## Group 2. hgeip / hgeie Registers

**Specification References**:
- `norm:hgeip_sz_acc_op`: hgeip is an HSXLEN-bit read-only register
- `norm:hgeie_sz_acc_op`: hgeie is an HSXLEN-bit read/write register
- `norm:hgeip_hgeie_fields`: Guest external interrupt number i corresponds to bit i
- `norm:geilen`: GEILEN count is implementation-defined, can be zero
- `norm:hgeie_op`: hgeie selects subset that triggers SGEI, does not affect VS-level external interrupt selected by VGEIN

**Test Responsibilities**: Verify guest external interrupt register field constraints and basic functionality.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HGEI-01 | hgeip read-only verification | Attempt to write hgeip | Write ignored |
| HGEI-02 | hgeie writable bit range | Write hgeie all ones, read back to determine GEILEN | bits GEILEN:1 writable, bit 0 read-only zero |
| HGEI-03 | hgeie bit 0 read-only zero | Write hgeie bit 0 = 1 | bit 0 reads back 0 |
| HGEI-04 | hgeip AND hgeie non-zero triggers SGEIP | If GEILEN>0, configure hgeie to enable corresponding bit, trigger guest external interrupt | hip.SGEIP=1 |
| HGEI-05 | hgeie/hgeip all zero when GEILEN=0 | If GEILEN=0, read hgeie/hgeip | All zero |

---

## Group 3. mideleg / mip / mie Enhancements

**Specification References**:
- `norm:mideleg_acc_h`: mideleg bits 10/6/2 are read-only 1, bit 12 is read-only 1 (when GEILEN>0)
- `norm:mideleg_hroz`: For bits that are zero in mideleg, corresponding hideleg/hip/hie are read-only zero
- `norm:mip_mie_vs`: mip/mie add VS interrupt bits
- `norm:mip_mie_alias`: SGEIP/VSEIP/VSTIP/VSSIP in mip are aliases of corresponding bits in hip

**Test Responsibilities**: Verify Hypervisor enhancement behavior of M-level interrupt registers.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| MIDLG-01 | mideleg bits 10/6/2 are read-only 1 | Attempt to clear mideleg bits 10/6/2 by writing | These bits remain 1 |
| MIDLG-02 | mideleg bit 12 is read-only 1 (GEILEN>0) | If GEILEN>0, attempt to clear mideleg bit 12 | Bit 12 remains 1 |
| MIDLG-03 | hideleg read-only zero for bits where mideleg=0 | A bit in mideleg=0, attempt to write that bit in hideleg | That bit in hideleg is read-only zero |
| MIDLG-04 | mip VSSIP is alias of hip.VSSIP | Write hvip.VSSIP=1, read mip.VSSIP | mip.VSSIP=1 |
| MIDLG-05 | mip VSEIP is alias of hip.VSEIP | Write hvip.VSEIP=1, read mip.VSEIP | mip.VSEIP=1 |
| MIDLG-06 | mie VSSIE is alias of hie.VSSIE | Write hie.VSSIE=1, read mie.VSSIE | mie.VSSIE=1 |
| MIDLG-07 | New VS interrupt bits visible in mip/mie | Read VS interrupt bit positions in mip/mie | Corresponding bits can be read |

---

## Group 4. hideleg Interrupt Delegation and Interrupt Number Translation

> This group is split from Group 3 of the original `Hypervisor_test_plan_en.md`, keeping only the interrupt delegation and cause translation cases (DELEG-08~14); CSR bit-field attribute cases are in `Hypervisor_CSR_test_plan_en.md`, and exception delegation chain cases are in `Hypervisor_Exceptions_test_plan_en.md`.

**Specification References**:
- `norm:hideleg_op`: Interrupts delegated by mideleg are further delegated to VS-mode if the corresponding hideleg bit is set
- `norm:hideleg_trans`: VS-level interrupt number translation (10→9, 6→5, 2→1)

**Test Responsibilities**: Verify the correctness of the hideleg interrupt delegation chain (HS→VS) and the VS interrupt number translation behavior.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| DELEG-08 | hideleg delegates VSSI to VS | Set hideleg[2]=1, inject VSSIP, VS-mode enables interrupt | Trap enters VS-mode (vscause=1, after translation) |
| DELEG-09 | hideleg delegates VSTI to VS | Set hideleg[6]=1, inject VSTIP, VS-mode enables interrupt | Trap enters VS-mode (vscause=5, after translation) |
| DELEG-10 | hideleg delegates VSEI to VS | Set hideleg[10]=1, inject VSEIP, VS-mode enables interrupt | Trap enters VS-mode (vscause=9, after translation) |
| DELEG-11 | Interrupt traps to HS when hideleg not delegating | Set hideleg[2]=0, inject VSSIP | Trap enters HS-mode |
| DELEG-12 | Interrupt number translation verification VSSI→SSI | hideleg[2]=1 delegates VSSIP to VS-mode | vscause records cause=1 (not 2) |
| DELEG-13 | Interrupt number translation verification VSTI→STI | hideleg[6]=1 delegates VSTIP to VS-mode | vscause records cause=5 (not 6) |
| DELEG-14 | Interrupt number translation verification VSEI→SEI | hideleg[10]=1 delegates VSEIP to VS-mode | vscause records cause=9 (not 10) |
