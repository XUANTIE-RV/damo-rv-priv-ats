**[中文](../testplan/Hypervisor_gstage_test_plan.md) | English**

# RISC-V G-stage Address Translation Test Plan

This document defines the test plan for **G-stage** (second-stage) address translation in the RISC-V Hypervisor extension, covering Sv39x4/Sv48x4/Sv57x4 translation algorithms, `hgatp` CSR, 16KB root page table, GPA high-bit checks, U-bit always-effective, guest-page-fault reporting, and other core specification points.

> **Scope**: This plan covers only **G-stage independent translation** (i.e., `vsatp.MODE = Bare`, `hgatp.MODE = Sv39x4 / Sv48x4 / Sv57x4`), with the goal of verifying the Sv*x4 algorithm itself in the simplified scenario where VS-stage is pass-through. Tests involving joint VS-stage and G-stage behavior (complete two-stage chain, HFENCE, HLV/HSV, `mstatus.MPRV+MPV`, etc.) are located in the companion document `docs/two_stage_translation_test_plan.md`.

> **Note**: The current repository is RV64; **Sv32x4 is not within the scope of this plan**.

---

## Applicable Scope

This plan verifies the following three G-stage translation modes under the premise of V=1 and `vsatp.MODE = Bare` (VS-stage pass-through, GVA = GPA):

- **Sv39x4**: 3-level page table, 41-bit GPA, supports 4KB / 2MB megapage / 1GB gigapage
- **Sv48x4**: 4-level page table, 50-bit GPA, supports 4KB / 2MB / 1GB / 512GB terapage
- **Sv57x4**: 5-level page table, 59-bit GPA, supports 4KB / 2MB / 1GB / 512GB / 256TB petapage

Key differences of each mode relative to the corresponding Sv39/Sv48/Sv57:

- Root page table size is expanded from 4KB to **16KB** (4 consecutive 4KB pages), and the root page table address must be **16KB aligned**
- Input address (GPA) is **2 bits wider** than the corresponding Sv mode
- U-bit **always checked as U-mode** (`norm:H_vm_gpapriv`)
- Translation failure triggers **guest-page-fault** (cause 20 / 21 / 23) instead of regular page-fault (12 / 13 / 15)
- ID field is **VMID** rather than ASID

---

## Specification References

- `SPEC/hypervisor.adoc` — "H" Extension for Hypervisor Support, Version 1.0
  - Hypervisor Guest Address Translation and Protection (`hgatp`) Register
  - Two-Stage Address Translation
  - Guest Physical Address Translation
  - Guest-Page Faults
  - Hypervisor Trap Value (`htval`) Register
  - Hypervisor Trap Instruction (`htinst`) Register
  - Trap Cause Codes

## Normative Rules

| Norm ID | Original Text |
|---------|------|
| `norm:hgatp_sz_acc_op` | The `hgatp` register is an HSXLEN-bit read/write register which controls G-stage address translation and protection, the second stage of two-stage translation for guest virtual addresses. |
| `norm:hgatp_mode_bare` | When MODE=Bare, guest physical addresses are equal to supervisor physical addresses, and there is no further memory protection for a guest virtual machine beyond the physical memory protection scheme. In this case, software must write zero to the remaining fields in `hgatp`. |
| `norm:hgatp_mode_sv` | When HSXLEN=32, the only other valid setting for MODE is Sv32x4. When HSXLEN=64, modes Sv39x4, Sv48x4, and Sv57x4 are defined. |
| `norm:hgatp_mode_warl` | A write to `hgatp` with an unsupported MODE value is not ignored as it is for `satp`. Instead, the fields of `hgatp` are WARL in the normal way, when so indicated. |
| `norm:hgatp_ppn_op` | For the paged virtual-memory schemes, the root page table is 16 KiB and must be aligned to a 16-KiB boundary. In these modes, the lowest two bits of the physical page number (PPN) in `hgatp` always read as zeros. |
| `norm:hgatp_vmid` | The number of VMID bits is UNSPECIFIED and may be zero. |
| `norm:hgatp_vmid_lsbs` | The least-significant bits of VMID are implemented first: that is, if VMIDLEN > 0, VMID[VMIDLEN-1:0] is writable. The maximal value of VMIDLEN, termed VMIDMAX, is 7 for Sv32x4 or 14 for Sv39x4, Sv48x4, and Sv57x4. |
| `norm:hgatp_mode_sv39x4` | For Sv39x4, partitioning is identical to Sv39, except with 2 more bits at the high end in VPN[2]. Address bits 63:41 must all be zeros, or else a guest-page-fault exception occurs. |
| `norm:hgatp_mode_sv48x4` | For Sv48x4, partitioning is identical to Sv48, except with 2 more bits at the high end in VPN[3]. Address bits 63:50 must all be zeros, or else a guest-page-fault exception occurs. |
| `norm:hgatp_mode_sv57x4` | For Sv57x4, partitioning is identical to Sv57, except with 2 more bits at the high end in VPN[4]. Address bits 63:59 must all be zeros, or else a guest-page-fault exception occurs. |
| `norm:hgatp_mode_x4` | When `hgatp`.MODE specifies a translation scheme of Sv32x4, Sv39x4, Sv48x4, or Sv57x4, G-stage address translation is a variation on the usual page-based scheme. In each case, the size of the incoming address is widened by 2 bits. The root page table is expanded by a factor of 4 to be 16 KiB and must be aligned to a 16 KiB boundary. |
| `norm:H_vm_gpatrans` | The conversion of an Sv32x4, Sv39x4, Sv48x4, or Sv57x4 guest physical address uses the same algorithm as Sv32, Sv39, Sv48, or Sv57, except: `hgatp` substitutes for `satp`; the effective privilege mode must be VS-mode or VU-mode; the current privilege mode is always taken to be U-mode when checking the U bit; and guest-page-fault exceptions are raised instead of regular page-fault exceptions. |
| `norm:H_vm_gpapriv` | For G-stage address translation, all memory accesses are considered to be user-level accesses. Access type permissions are checked during G-stage translation the same as for VS-stage. For memory accesses supporting VS-stage translation, permissions and A/D bit needs are checked as though for an implicit load or store, not for the original access type. However, any exception is always reported for the original access type. |
| `norm:H_vm_gpa_g` | The G bit in all G-stage PTEs is currently not used. It should be cleared by software for forward compatibility, and must be ignored by hardware. |
| `norm:H_guest_page_fault` | Guest-page-fault traps may be delegated from M-mode to HS-mode under the control of `medeleg`, but cannot be delegated to other privilege modes. On a guest-page fault, `mtval` or `stval` is written with the faulting guest virtual address, and `mtval2` or `htval` is written either with zero or with the faulting guest physical address, shifted right by 2 bits. |
| `norm:htval_trapval` | When a guest-page-fault trap is taken into HS-mode, `htval` is written with either zero or the guest physical address that faulted, shifted right by 2 bits. For other traps, `htval` is set to zero. |
| `norm:mtval2_htval_virtaddr` | When a guest-page fault is not due to an implicit memory access for VS-stage address translation, a nonzero guest physical address written to `mtval2`/`htval` shall correspond to the exact virtual address written to `mtval`/`stval`. |
| `norm:H_trap_xtinst_guestpage` | For guest-page faults, the trap instruction register is written with a special pseudoinstruction value if: (a) the fault is caused by an implicit memory access for VS-stage address translation, and (b) a nonzero value is written to `mtval2` or `htval`. If both conditions are met, zero is not allowed. |
| `norm:H_trap_xtinst_guestpage_rw` | A write pseudoinstruction (0x00002020 or 0x00003020) is used for the case that the machine is attempting automatically to update bits A and/or D in VS-level page tables. All other implicit memory accesses for VS-stage address translation will be reads. |
| `norm:hgatp_tvm_illegal` | When `mstatus`.TVM=1, attempts to read or write `hgatp` while executing in HS-mode will raise an illegal-instruction exception. |
| `norm:hstatus_gva_op` | Field GVA (Guest Virtual Address) is written by the implementation whenever a trap is taken into HS-mode. For any trap that writes a guest virtual address to `stval`, GVA is set to 1. For any other trap into HS-mode, GVA is set to 0. |

---

## Test Group Definitions

### Group 1: hgatp CSR Field Validation

**Specification Basis**:
- `norm:hgatp_sz_acc_op`: `hgatp` is an HSXLEN-bit read/write register containing MODE / VMID / PPN fields
- `norm:hgatp_mode_warl`: Writing an unsupported MODE to `hgatp` is **not ignored** (unlike `satp`), handled per WARL rules
- `norm:hgatp_ppn_op`: PPN[1:0] always reads as 0 in Sv*x4 modes; implementation may make them read-only zero
- `norm:hgatp_vmid`, `norm:hgatp_vmid_lsbs`: VMIDLEN is implementation-defined, maximum 14 (Sv39x4/48x4/57x4)
- `norm:hgatp_mode_bare`: When MODE=Bare, other fields must be zero; behavior is UNSPECIFIED if software does not clear them
- `norm:hgatp_tvm_illegal`: When `mstatus.TVM=1`, HS-mode access to `hgatp` triggers an illegal-instruction exception

**Test Responsibility**: Verify `hgatp` CSR field read/write, WARL behavior, and TVM control.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| GHCSR-01 | MODE=Bare write-read | M-mode writes `hgatp` MODE=0, VMID=0, PPN=0 | Read-back value matches |
| GHCSR-02 | MODE=Sv39x4 write-read | M-mode writes MODE=8 (Sv39x4) + valid PPN | Read-back value matches |
| GHCSR-03 | MODE=Sv48x4 write-read | M-mode writes MODE=9 (Sv48x4) | Read-back value matches |
| GHCSR-04 | MODE=Sv57x4 write-read | M-mode writes MODE=10 (Sv57x4) | Read-back value matches |
| GHCSR-05 | Unsupported MODE handled per WARL rules | First write valid MODE=Sv39x4 and read back original value; then write reserved MODE=2, read back hgatp to verify MODE field is not silently accepted as 2 | Unlike `satp` (where writing an invalid MODE causes the entire write to be ignored): hgatp.MODE field is WARL independently (either retains old valid value or is automatically adjusted to a valid value); the write operation itself is not ignored |
| GHCSR-06 | PPN[1:0] forced to 0 | Write hgatp via `MAKE_HGATP(SV39X4, 0, ppn_with_low2_bits_set)`, read back and extract PPN field | Read-back PPN[1:0]=0 |
| GHCSR-07 | VMIDLEN probing and write-back | Write all 1s to VMID field, read back to determine VMIDLEN; then write-read with valid VMID in 0..VMIDMAX range | VMIDLEN ≤ 14; valid VMIDs can be written and read back |
| GHCSR-08 | HS-mode access to hgatp with TVM=1 | M-mode sets `mstatus.TVM=1`, switch to HS-mode and `csrr` read `hgatp` | illegal-instruction exception (`scause`=2); M-mode access is unaffected |
| GHCSR-09 | MODE=Bare pass-through verification | M-mode writes `hgatp` MODE=Bare, VS-mode accesses arbitrary GPA | GPA = SPA, no translation or protection, access succeeds |

---

### Group 2: Root Page Table 16KB Alignment

**Specification Basis**:
- `norm:hgatp_mode_x4`: Sv32x4/39x4/48x4/57x4 root page table is **16KB** and must be **16KB aligned**
- `norm:hgatp_ppn_op`: Low 2 bits of PPN are forced to 0 in Sv*x4 modes, thereby ensuring 16KB alignment

**Test Responsibility**: Verify root page table 16KB size and alignment requirements; verify implementation behavior with misaligned root page tables.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| GROOT-01 | Sv39x4 16KB-aligned root table works normally | Allocate a 16KB-aligned root table from G-stage page table pool (verify `((uintptr_t)root_pt & 0x3FFF) == 0`), enable Sv39x4 and access a mapped GPA | Root table alignment meets requirements; translation succeeds |
| GROOT-02 | Sv48x4 16KB-aligned root table works normally | Same as GROOT-01 but mode is Sv48x4. Sv48x4 shares the 16KB alignment requirement with Sv39x4 (`norm:hgatp_mode_x4`); this test case verifies the framework indeed allocates a 16KB-aligned root table in this mode | Translation succeeds |
| GROOT-03 | Sv57x4 16KB-aligned root table works normally | Same as GROOT-01 but mode is Sv57x4 | Translation succeeds |
| GROOT-04 | hgatp.PPN[1:0] always reads back 0 | Write PPN bits 1:0 = 0b11 via `MAKE_HGATP`, read back hgatp, then enable translation and access a mapped GPA | hgatp.PPN[1:0] reads back 0; translation uses the aligned PPN, access succeeds |

> [!NOTE]
> There is no standardized test case for "whether a 4KB-aligned root table works" — `hgatp_ppn_op` enforces 16KB alignment at the hardware level by forcing PPN[1:0] to 0, so software cannot construct a truly "non-zero low 2 bits" root page table address to write into hgatp.

---

### Group 3: Sv39x4 Basic Mapping

**Specification Basis**:
- `norm:hgatp_mode_sv39x4`: Sv39x4 is a 2-bit extension of Sv39, 41-bit GPA
- `norm:H_vm_gpatrans`: Uses the same translation algorithm as Sv39, only replacing satp → hgatp, effective privilege restricted to VS/VU, U-bit perspective always U, fault type changed to guest-page-fault

**Test Responsibility**: With Bare VS-stage as the premise, verify Sv39x4 GPA → SPA translation for three page sizes (1GB / 2MB / 4KB).

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| G39-MAP-01 | Sv39x4 1GB gigapage mapping | Establish 1GB GPA→SPA identity mapping, VS-mode read/write | Read/write succeeds, data consistent |
| G39-MAP-02 | Sv39x4 2MB megapage mapping | Establish 2MB identity mapping, VS-mode read/write | Read/write succeeds |
| G39-MAP-03 | Sv39x4 4KB page mapping | Establish 4KB identity mapping, VS-mode read/write | Read/write succeeds |

---

### Group 4: Sv48x4 Basic Mapping

**Specification Basis**:
- `norm:hgatp_mode_sv48x4`: Sv48x4 is a 2-bit extension of Sv48, 50-bit GPA

**Test Responsibility**: Verify Sv48x4 GPA → SPA translation for four page sizes.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| G48-MAP-01 | Sv48x4 512GB terapage mapping | Establish 512GB identity mapping | Read/write succeeds (subject to platform physical memory limits) |
| G48-MAP-02 | Sv48x4 1GB gigapage mapping | Establish 1GB identity mapping | Read/write succeeds |
| G48-MAP-03 | Sv48x4 2MB megapage mapping | Establish 2MB identity mapping | Read/write succeeds |
| G48-MAP-04 | Sv48x4 4KB page mapping | Establish 4KB identity mapping | Read/write succeeds |

> [!WARNING]
> G48-MAP-01 (512GB terapage) may not fully cover the entire 512GB range on the QEMU virt platform due to physical memory size limitations; only sub-ranges within actual physical memory should be accessed.

---

### Group 5: Sv57x4 Basic Mapping

**Specification Basis**:
- `norm:hgatp_mode_sv57x4`: Sv57x4 is a 2-bit extension of Sv57, 59-bit GPA

**Test Responsibility**: Verify basic Sv57x4 translation (256TB petapage is not fully covered due to physical memory limitations).

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| G57-MAP-01 | Sv57x4 1GB gigapage mapping | Establish 1GB identity mapping | Read/write succeeds |
| G57-MAP-02 | Sv57x4 2MB megapage mapping | Establish 2MB identity mapping | Read/write succeeds |
| G57-MAP-03 | Sv57x4 4KB page mapping | Establish 4KB identity mapping | Read/write succeeds |

> [!NOTE]
> 256TB petapage and 512GB terapage tests are limited by QEMU physical memory; supplementary testing is recommended with dedicated large-memory configurations.

---

### Group 6: GPA High-Bit Check (Must Be Zero)

**Specification Basis**:
- `norm:hgatp_mode_sv39x4`: Sv39x4 GPA bits 63:41 must all be zero, otherwise a guest-page-fault is triggered
- `norm:hgatp_mode_sv48x4`: Sv48x4 GPA bits 63:50 must all be zero
- `norm:hgatp_mode_sv57x4`: Sv57x4 GPA bits 63:59 must all be zero

**Test Responsibility**: Verify that nonzero GPA high bits correctly trigger guest-page-fault.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| GHIGH-01 | Sv39x4 bit 41 nonzero | Access GPA = `1UL << 41` (only the lowest illegal bit set, remaining high bits zero) | load guest-page-fault (cause=21) |
| GHIGH-02 | Sv39x4 bit 63 nonzero | Access GPA = `1UL << 63` (highest bit set, verifying bits 63:41 high end) | guest-page-fault |
| GHIGH-03 | Sv48x4 bit 50 nonzero | Access GPA = `1UL << 50` | guest-page-fault |
| GHIGH-04 | Sv48x4 bit 63 nonzero | Access GPA = `1UL << 63` | guest-page-fault |
| GHIGH-05 | Sv57x4 bit 59 nonzero | Access GPA = `1UL << 59` | guest-page-fault |
| GHIGH-06 | Sv57x4 bit 63 nonzero | Access GPA = `1UL << 63` | guest-page-fault |

---

### Group 7: PTE Validity Check

**Specification Basis** (following Sv algorithm step 3, but with different fault type):
- `pte.v = 0` → guest-page-fault
- `pte.r = 0 && pte.w = 1` → guest-page-fault (reserved encoding)
- `norm:H_vm_gpatrans`: On failure, guest-page-fault (cause 20/21/23) is triggered instead of page-fault

**Test Responsibility**: Verify guest-page-fault behavior when G-stage PTE is invalid; confirm cause code is distinguished from regular page-fault.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| GVALID-01 | V=0 PTE (load) | G-stage data test page V=0, VS-mode executes `ld` | load guest-page-fault (cause=21) |
| GVALID-02 | V=0 PTE (store) | Same as GVALID-01, VS-mode executes `sd` | store guest-page-fault (cause=23) |
| GVALID-03 | V=0 PTE (fetch) | Map V=0 in a separate "jump target" GPA range; VS-mode jumps to that GPA for fetch (code region retains valid mapping for execution of the triggering instruction itself) | inst guest-page-fault (cause=20) |
| GVALID-04 | R=0,W=1 reserved encoding (load) | G-stage data test page V=1,R=0,W=1,X=0 | load guest-page-fault |
| GVALID-05 | R=0,W=1 reserved encoding (store) | Same as above, VS-mode store | store guest-page-fault |

---

### Group 8: RWX Permission Combinations

**Specification Basis**:
- `norm:H_vm_gpatrans`: Access type permissions (R/W/X) are checked in the same manner as Sv
- `norm:H_vm_gpapriv`: All G-stage accesses are treated as U-mode

**Test Responsibility**: Verify G-stage PTE RWX permission combinations under VS-mode access.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| GRWX-01 | R=1,W=0,X=0 read-only | load succeeds, store/fetch fails | store/fetch guest-page-fault |
| GRWX-02 | R=1,W=1,X=0 RW | load/store succeeds, fetch fails | fetch guest-page-fault |
| GRWX-03 | R=1,W=0,X=1 RX | load/fetch succeeds, store fails | store guest-page-fault |
| GRWX-04 | R=1,W=1,X=1 RWX | All succeed | No fault |
| GRWX-05 | R=0,W=0,X=1 execute-only | fetch succeeds, load fails (HS-level `sstatus.MXR=0`) | load guest-page-fault |
| GRWX-06 | R=0,W=1,X=1 reserved encoding | Any access fails | guest-page-fault |
| GRWX-07 | R=W=X=0 at lowest-level PTE | R=W=X=0 is a non-leaf PTE (pointer). This encoding at the lowest-level PTE means the algorithm attempts to traverse beyond LEVELS | guest-page-fault (per Sv algorithm step 4: exceeding levels triggers page-fault) |

---

### Group 9: U-bit Always Checked as U-mode (Key G-stage Difference)

**Specification Basis**:
- `norm:H_vm_gpapriv`: "For G-stage address translation, all memory accesses (including those made to access data structures for VS-stage address translation) are considered to be user-level accesses, as though executed in U-mode."

**Test Responsibility**: Verify that G-stage PTE U-bit must be 1; otherwise, even if the effective privilege is VS-mode, a guest-page-fault is triggered. This is one of the most critical differences between G-stage and VS-stage.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| GUBIT-01 | VS-mode access to U=0 G-stage page (load) | G-stage mapping U=0, VS-mode (nominal S) load | load guest-page-fault |
| GUBIT-02 | VS-mode access to U=0 G-stage page (store) | Same as above, store | store guest-page-fault |
| GUBIT-03 | VS-mode access to U=0 G-stage page (fetch) | Same as above, fetch | inst guest-page-fault |
| GUBIT-04 | VU-mode access to U=1 G-stage page | G-stage U=1, switch to VU-mode via framework's `run_in_vu_mode` and access | Succeeds |
| GUBIT-05 | VS-mode access to U=1 G-stage page | G-stage U=1, VS-mode (`run_in_vs_mode` / `two_stage_run_in_vs`) access | Succeeds (G-stage perspective is always U-mode, U=1 always permitted) |

---

### Group 10: A/D Bit Management

**Specification Basis**:
- `norm:H_vm_gpatrans`: "permissions and the need to set A and/or D bits at the G-stage level are checked as though for an implicit load or store, not for the original access type. However, any exception is always reported for the original access type."
- Default (Svadu not implemented): A=0 or store access with D=0 triggers guest-page-fault

**Test Responsibility**: Verify G-stage A/D bit checking and fault cause reporting logic.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| GAD-01 | A=0 triggers load guest-page-fault | G-stage PTE A=0,D=1, VS-mode load | load guest-page-fault (cause=21) |
| GAD-02 | A=0 triggers store guest-page-fault | G-stage PTE A=0,D=1, VS-mode store | store guest-page-fault (cause=23) |
| GAD-03 | D=0 triggers store guest-page-fault | G-stage PTE A=1,D=0, VS-mode store | store guest-page-fault (cause=23) |
| GAD-04 | A=1,D=1 normal access | G-stage PTE A=1,D=1, VS-mode RW | No fault |

> [!NOTE]
> The fault cause for A/D bits is always reported per the "original access type," but permission checking follows the implicit load/store logic. See specification `norm:H_vm_gpatrans`.

---

### Group 11: Superpage Alignment Validation

**Specification Basis**:
- Follows Sv39/Sv48/Sv57 superpage alignment rules (`pte.ppn[i-1:0]` must be zero, otherwise misaligned superpage)
- `norm:H_vm_gpatrans`: On failure, guest-page-fault is triggered

**Test Responsibility**: Verify that G-stage superpage PTE ppn low bits must be cleared per level.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| GALIGN-01 | Sv39x4 1GB superpage misaligned | 1GB PTE ppn[1:0] nonzero | guest-page-fault |
| GALIGN-02 | Sv39x4 2MB superpage misaligned | 2MB PTE ppn[0] nonzero | guest-page-fault |
| GALIGN-03 | Sv48x4 512GB superpage misaligned | 512GB PTE ppn[2:0] nonzero | guest-page-fault |
| GALIGN-04 | Sv57x4 256TB superpage misaligned | 256TB PTE ppn[3:0] nonzero | guest-page-fault |
| GALIGN-05 | Aligned superpage works normally | All superpage levels correctly aligned | No fault |

---

### Group 12: G-bit Ignore Verification

**Specification Basis**:
- `norm:H_vm_gpa_g`: "The G bit in all G-stage PTEs is currently not used. Until its use is defined by a standard extension, it should be cleared by software for forward compatibility, and must be ignored by hardware."

**Test Responsibility**: Verify that the G-stage PTE G-bit is ignored by hardware.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| GGBIT-01 | G=1 does not affect translation | G-stage PTE with G=1, access the page | Translation succeeds, no exception |
| GGBIT-02 | G=0 and G=1 behave identically | Compare access behavior for the same mapping with G=0 and G=1 | Behavior is identical |

---

### Group 13: guest-page-fault Reporting (htval / htinst / GVA)

**Specification Basis**:
- `norm:H_guest_page_fault`: On guest-page-fault, `stval` is written with the faulting GVA, `htval` is written with GPA>>2 or 0
- `norm:htval_trapval`, `norm:mtval2_htval_virtaddr`: For non-implicit accesses, `htval` and `stval` correspond to the same access; GPA is stored right-shifted by 2 bits
- `norm:H_trap_xtinst_guestpage`: On guest-page-fault, `htinst` may be 0 or a transformed instruction; pseudoinstruction is used for implicit VS-stage accesses
- `norm:hstatus_gva_op`: `hstatus.GVA` is set based on whether a GVA is written to stval when trapping into HS-mode
- In this group (G-stage independent, VS-stage Bare), GVA = GPA; there are no VS-stage implicit accesses

**Test Responsibility**: Verify htval / htinst / hstatus.GVA / stval writes on G-stage faults.

| Test ID | Test Name | Test Description | Expected Result |
|---------|----------|----------|----------|
| GFAULT-01 | load fault cause=21 | Trigger load guest-page-fault | scause=21 |
| GFAULT-02 | store fault cause=23 | Trigger store guest-page-fault | scause=23 |
| GFAULT-03 | inst fault cause=20 | Trigger inst guest-page-fault | scause=20 |
| GFAULT-04 | htval = GPA>>2 | Explicit load fault, check htval | htval == faulting GPA >> 2 |
| GFAULT-05 | stval = GVA | Explicit load fault, check stval | stval == faulting GVA (GVA=GPA under VS-stage Bare) |
| GFAULT-06 | hstatus.GVA = 1 | guest-page-fault writes GVA to stval | hstatus.GVA == 1 |
| GFAULT-07 | htinst transformed for load | htinst for load fault is transformed load or 0 | htinst conforms to `norm:H_trap_xtinst_exception_list` |
| GFAULT-08 | htinst on inst guest-page-fault | Check htinst after instruction guest-page-fault | 0 (per tinst-values: inst guest-page-fault does not permit writing a transformed standard instruction; this group has no VS-stage implicit accesses, so it will not be a pseudoinstruction either; therefore the implementation can only write 0 or custom — for standard instruction scenarios it should be 0) |

> [!NOTE]
> The **write pseudoinstruction** (0x00002020 / 0x00003020) scenario in `norm:H_trap_xtinst_guestpage_rw` only occurs when a VS-stage implicit access updating A/D bits triggers a G-stage fault, which is a joint two-stage behavior covered by `docs/two_stage_translation_test_plan.md`. This group (VS-stage Bare) has no VS-stage implicit accesses, so only read pseudoinstruction / transformed instruction / 0 cases are verified.

---

## Test Priority

| Priority | Test Group | Covered Test IDs | Rationale |
|--------|--------|--------------|------|
| P0 (Must) | Group 3, Group 7, Group 8, Group 9 | G39-MAP-01~03, GVALID-01~05, GRWX-01~07, GUBIT-01~05 | Sv39x4 core algorithm + PTE validity + RWX + U-bit key difference |
| P1 (Important) | Group 1, Group 2, Group 6, Group 10, Group 13 | GHCSR-01~08, GROOT-01~04, GHIGH-01~06, GAD-01~04, GFAULT-01~08 | hgatp CSR, root table alignment, GPA high bits, A/D bits, fault reporting |
| P2 (Recommended) | Group 4, Group 5, Group 11 | G48-MAP-01~04, G57-MAP-01~03, GALIGN-01~05 | Sv48x4/Sv57x4 extended modes, superpage alignment |
| P3 (Optional) | Group 12 | GGBIT-01~02 | G-bit ignore verification |

---

## Test Implementation Notes

### File Organization

Each G-stage mode (Sv39x4 / Sv48x4 / Sv57x4) has its own test directory, consistent with the existing `sv39/sv48/sv57` naming convention. This plan shares the same directory structure and framework code with `two_stage_translation_test_plan.md`:

```
sv39x4/
├── Makefile
├── kernel.ld
└── main.c              # G-stage independent + same-width two-stage test cases

sv48x4/
├── Makefile
├── kernel.ld
└── main.c              # Same as above

sv57x4/
├── Makefile
├── kernel.ld
└── main.c              # Same as above

common/hyp/             # Hypervisor test framework (per docs/hypervisor_framework.md)
├── hyp_defs.h
├── hyp_csr.c
├── hyp_priv.c
├── hyp_trap.c
├── hyp_trap_asm.S
├── hyp_fence.c
├── hyp_ldst.c
├── gstage_pt.c         # G-stage page table management (gpt_*)
├── two_stage.c         # Two-stage management (two_stage_*)
├── hyp_reset.c
└── hyp_test.h
```

### Common Test Pattern

Each G-stage test case follows this pattern:

1. Initialize G-stage pool and context
2. Set up code + data region G-stage identity mapping (U=1 required)
3. Optionally map test pages with special permissions
4. Execute in VS-mode + G-stage
5. Clean up

### VS-mode Test Helper Functions

| Function Name | Purpose | Return Value |
|--------|------|--------|
| `test_vs_read_write` | VS-mode writes magic value and reads back for verification | 0=success |
| `test_vs_load` | VS-mode executes load | 0=success |
| `test_vs_store` | VS-mode executes store | 0=success |
| `test_vs_load_expect_fault` | VS-mode load, expects fault | fault cause |
| `test_vs_store_expect_fault` | VS-mode store, expects fault | fault cause |
| `test_vs_exec_expect_fault` | VS-mode jump-execute, expects fault | fault cause |

### Key Considerations

1. **G-stage PTE must set U=1**: Unlike VS-stage, G-stage treats all accesses as U-mode (`norm:H_vm_gpapriv`). Even accesses triggered from VS-mode will trigger a guest-page-fault if the G-stage PTE has U=0.

2. **Fault cause distinction**: G-stage faults use cause 20/21/23 (Instruction/Load/Store guest-page fault), strictly distinguished from regular page-fault causes 12/13/15. Test assertions must use the correct cause constants.

3. **htval encoding**: `htval` is always written with the GPA right-shifted by 2 bits (consistent with PMP / PTE PPN encoding). The low 2-bit information can be recovered from the low 2 bits of `stval` (for non-implicit access scenarios).

4. **hgatp unsupported MODE is not ignored**: Unlike `satp`'s WARL behavior, writing an unsupported MODE to `hgatp` is not silently discarded; software must read back to verify (`norm:hgatp_mode_warl`).

5. **TVM control**: When `mstatus.TVM=1`, HS-mode access to `hgatp` or execution of HFENCE.GVMA triggers an illegal-instruction exception; M-mode is unaffected by TVM.

6. **VMID probing**: The implemented VMIDLEN is not fixed; when testing the VMID field, first write all 1s and read back to probe VMIDLEN, then use valid VMID values.

7. **Code + data region mapping requirements**: Identity mapping must cover code segment, data segment, stack, page table pool, and UART. For G-stage, all these segments must also be mapped in GPA space with U=1.

8. **QEMU platform limitations**:
   - QEMU virt platform requires `-cpu rv64,h=true` (or newer versions with H extension enabled by default)
   - 512GB / 256TB large superpage tests are limited by physical memory
   - QEMU implements Svade by default (A=0 / D=0 triggers fault)

---

## References

- `SPEC/hypervisor.adoc` — RISC-V Hypervisor Extension, Version 1.0
- `docs/two_stage_translation_test_plan.md` — Two-stage joint behavior test plan (companion)
- `docs/vm_test_plan.md` — VS-stage / regular VM test plan (behavior baseline)
- `docs/hypervisor_framework.md` — Hypervisor test framework design
- `common/hyp/gstage_pt.c` — G-stage page table management API
- `common/hyp/two_stage.c` — Two-stage management API
- `common/hyp/hyp_defs.h` — Hypervisor CSR and cause code definitions
