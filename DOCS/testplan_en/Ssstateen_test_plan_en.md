**[中文](../testplan/Ssstateen_test_plan.md) | English**

# Ssstateen Extension Test Plan

## 1. Overview

### 1.1 Specification Source

- **Specification File**: `SPEC/smstateen.adoc`
- **Extension Name**: Ssstateen (Supervisor-level State Enable Extension)
- **Related Extension**: Smstateen (Machine-level superset)

### 1.2 Features Under Test

The Ssstateen extension is the supervisor-level subset of Smstateen, comprising only the sstateen\* and hstateen\* CSRs and their functionality. This extension enables supervisor-level software and hypervisors to control lower-privilege access to extension state, preventing covert channels.

> **Note**: Ssstateen does not include the mstateen\* CSRs. The behavior of mstateen\* is defined and tested by the Smstateen extension. This plan assumes that mstateen\* is correctly configured by higher-privilege software to permit access to sstateen\*/hstateen\*.

### 1.3 Test Scope

| Test Level | CSRs Involved | Test Groups |
|------------|---------------|-------------|
| S-mode | sstateen0-3 | Group 1-6 |
| H-mode | hstateen0-3, hstateen0h-3h (RV32) | Group 7-12 (**[Migrated] to [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) Group 8**) |

### 1.4 Prerequisites

| Item | Requirement |
|------|-------------|
| Architecture | RV64 (RV32 partial tests optional) |
| Privilege Support | S-mode (required), U-mode (required) |
| Optional Dependencies | H extension (Group 7-12), Ssaia, Sscsrind, Zcmt, Sdtrig |
| mstateen Configuration | The corresponding mstateen bits must be set to 1 by M-mode prior to testing to permit access |

---

## 2. Specification Requirements Analysis

### 2.1 Ssstateen-Related Normative Rules

| Norm ID | Original Text |
|---------|---------------|
| `norm:sstateen_rv64_csrs` | When S-mode is implemented, the sstateen0-3 CSRs are added. |
| `norm:hstateen_rv64_csrs` | When the H extension is implemented, the hstateen0-3 CSRs are added. |
| `norm:stateen_rv32_upper_bits_csrs` | For RV32, hstateen0h-3h are additionally provided. |
| `norm:stateen_op` | The `stateen` registers at each level control access to state at all less-privileged levels, but not at its own level. |
| `norm:stateen_illegal_state_access` | Just as with the `counteren` CSRs, when a `stateen` CSR prevents access to state by less-privileged levels, an attempt in one of those privilege modes to execute an instruction that would read or write the protected state raises an illegal-instruction exception, or, if executing in VS or VU mode and the circumstances for a virtual-instruction exception apply, raises a virtual-instruction exception instead of an illegal-instruction exception. |
| `norm:stateen_implicit_state_update` | When a `stateen` CSR prevents access to state for a privilege mode, attempting to execute in that privilege mode an instruction that _implicitly_ updates the state without reading it may or may not raise an illegal-instruction or virtual-instruction exception. Such cases must be disambiguated by being explicitly specified one way or the other. |
| `norm:sstateen_user_access_control` | Each bit of a supervisor-level `sstateen` CSR controls user-level access (from U-mode or VU-mode) to an extension's state. |
| `norm:sstateen_bit_allocation` | The intention is to allocate the bits of `sstateen` CSRs starting at the least-significant end, bit 0, through to bit 31, and then on to the next-higher-numbered `sstateen` CSR. |
| `norm:sstateen_encroachment_bits_roz` | In that case, the bit positions of "encroaching" bits will remain forever read-only zeros in the matching `sstateen` CSRs. |
| `norm:hstateen_encoding` | With the hypervisor extension, the `hstateen` CSRs have identical encodings to the `mstateen` CSRs, except controlling accesses for a virtual machine (from VS and VU modes). |
| `norm:stateen_warl_access` | Each standard-defined bit of a `stateen` CSR is WARL and may be read-only zero or one, subject to the following conditions. |
| `norm:stateen_unimplemented_state_roz` | Bits in any `stateen` CSR that are defined to control state that a hart doesn't implement are read-only zeros for that hart. |
| `norm:stateen_reserved_roz` | Likewise, all reserved bits not yet given a defined meaning are also read-only zeros. |
| `norm:mstateen_lower_priv_roz` | For every bit in an `mstateen` CSR that is zero (whether read-only zero or set to zero), the same bit appears as read-only zero in the matching `hstateen` and `sstateen` CSRs. |
| `norm:sstateen_vsmode_access_roz` | For every bit in an `hstateen` CSR that is zero (whether read-only zero or set to zero), the same bit appears as read-only zero in `sstateen` when accessed in VS-mode. |
| `norm:sstateen_ro1_bits` | A bit in a supervisor-level `sstateen` CSR cannot be read-only one unless the same bit is read-only one in the matching `mstateen` CSR and, if it exists, in the matching `hstateen` CSR. |
| `norm:hstateen_ro1_bits` | A bit in an `hstateen` CSR cannot be read-only one unless the same bit is read-only one in the matching `mstateen` CSR. |
| `norm:hstateen_sstateen_zero_initialization` | If machine-level software changes these values, it is responsible for initializing the corresponding writable bits of the `hstateen` and `sstateen` CSRs to zeros too. |
| `norm:hstateen_bit_63_op` | Likewise, bit 63 of each `hstateen` correspondingly controls access to the matching `sstateen` CSR. |
| `norm:hstateen_bit_63_writable` | Bit 63 of each `hstateen` CSR is always writable (not read-only). |
| `norm:stateen0_c_op` | The C bit controls access to any and all custom state. |
| `norm:stateen0_fcsr_op` | The FCSR bit controls access to `fcsr` for the case when floating-point instructions operate on `x` registers instead of `f` registers as specified by the Zfinx and related extensions (Zdinx, etc.). |
| `norm:stateen0_jvt_op` | The JVT bit controls access to the `jvt` CSR provided by the Zcmt extension. |
| `norm:hstateen0_SE0_op` | The SE0 bit in `hstateen0` controls access to the `sstateen0` CSR. |
| `norm:hstateen0_envcfg_op` | The ENVCFG bit in `hstateen0` controls access to the `senvcfg` CSRs. |
| `norm:hstateen0_csrind_op` | The CSRIND bit in `hstateen0` controls access to the `siselect` and the `sireg*`, (really `vsiselect` and `vsireg*`) CSRs provided by the Sscsrind extensions. |
| `norm:hstateen0_imsic_op` | The IMSIC bit in `hstateen0` controls access to the guest IMSIC state, including CSRs `stopei` (really `vstopei`), provided by the Ssaia extension. |
| `norm:hstateen0_aia_op` | The AIA bit in `hstateen0` controls access to all state introduced by the Ssaia extension and not controlled by either the CSRIND or the IMSIC bits of `hstateen0`. |
| `norm:hstateen0_context_op` | The CONTEXT bit in `hstateen0` controls access to the `scontext` CSR provided by the Sdtrig extension. |
| `norm:mstateen_bit_correspondence` | For every bit with a defined purpose in an `sstateen` CSR, the same bit is defined in the matching `mstateen` CSR to control access below machine level to the same state. |

---

## 3. S-mode Test Groups

### Group 1: sstateen CSR Existence and Accessibility

**Spec Reference**:
- `norm:sstateen_rv64_csrs`: sstateen0-3 are added when S-mode is implemented
- `norm:sstateen_bit_allocation`: sstateen bits are allocated starting from bit 0 toward higher bits

**Prerequisite Configuration**: M-mode must set bit 63 (SE0) of the corresponding mstateen to 1 to permit S-mode access to sstateen.

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-EXIST-01 | sstateen0 readable in S-mode | Configure mstateen0.SE0=1, S-mode reads sstateen0 | No exception | `norm:sstateen_rv64_csrs` |
| SS-EXIST-02 | sstateen0 writable in S-mode | Configure mstateen0.SE0=1, S-mode writes sstateen0 then reads back | No exception, writable bits take effect | `norm:sstateen_rv64_csrs` |
| SS-EXIST-03 | sstateen1 readable/writable in S-mode | Configure mstateen1 bit63=1, S-mode reads/writes sstateen1 | No exception | `norm:sstateen_rv64_csrs` |
| SS-EXIST-04 | sstateen2 readable/writable in S-mode | Configure mstateen2 bit63=1, S-mode reads/writes sstateen2 | No exception | `norm:sstateen_rv64_csrs` |
| SS-EXIST-05 | sstateen3 readable/writable in S-mode | Configure mstateen3 bit63=1, S-mode reads/writes sstateen3 | No exception | `norm:sstateen_rv64_csrs` |
| SS-EXIST-06 | sstateen0 has 32-bit effective width | S-mode writes all-ones 64-bit value to sstateen0 then reads back | Only lower 32 bits are effective, upper bits do not exist | `norm:sstateen_bit_allocation` |

**Test Steps Example (SS-EXIST-01)**:
1. Set mstateen0 bit 63 (SE0) to 1 in M-mode
2. Configure PMP to allow S-mode execution
3. Switch to S-mode
4. Read the sstateen0 CSR
5. Verify the read operation completes without exception
6. Return to M-mode

**Expected Result**:
- S-mode reads sstateen0 normally, no illegal-instruction exception

**Assertions**:

---

### Group 2: sstateen Controls U-mode/VU-mode Access

**Spec Reference**:
- `norm:sstateen_user_access_control`: Each bit of sstateen controls U-mode or VU-mode access to extension state
- `norm:stateen_op`: stateen controls access at all lower privilege levels, not at its own level
- `norm:stateen_illegal_state_access`: Prevented access triggers an illegal-instruction exception; in VS/VU mode, a virtual-instruction exception is raised when conditions apply

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-UCTL-01 | sstateen0.C=0 blocks U-mode custom state | Set sstateen0.C=0, U-mode accesses custom CSR | Illegal-instruction exception triggered | `norm:sstateen_user_access_control` |
| SS-UCTL-02 | sstateen0.C=1 permits U-mode custom state | Set sstateen0.C=1, U-mode accesses custom CSR | Access succeeds (subject to extension implementation) | `norm:sstateen_user_access_control` |
| SS-UCTL-03 | sstateen0 does not control S-mode itself | sstateen0.C=0, S-mode accesses custom state | S-mode access succeeds, no exception | `norm:stateen_op` |
| SS-UCTL-04 | sstateen0.JVT=0 blocks U-mode jvt | Set sstateen0.JVT=0, U-mode reads jvt CSR | Illegal-instruction exception triggered | `norm:sstateen_user_access_control`, `norm:stateen0_jvt_op` |
| SS-UCTL-05 | sstateen0.JVT=1 permits U-mode jvt | Set sstateen0.JVT=1, U-mode reads jvt CSR | Access succeeds | `norm:sstateen_user_access_control`, `norm:stateen0_jvt_op` |
| SS-UCTL-06 | sstateen0.FCSR=0 blocks U-mode fcsr (Zfinx) | Ensure misa.F=0 and sstateen0.FCSR=0, U-mode executes floating-point instruction | All floating-point instructions trigger illegal-instruction exception | `norm:sstateen_user_access_control`, `norm:stateen0_fcsr_op` |
| SS-UCTL-07 | Blocked U-mode read triggers illegal-instruction | sstateen0 bit=0, U-mode reads controlled CSR | Illegal-instruction exception triggered (cause=2) | `norm:stateen_illegal_state_access` |
| SS-UCTL-08 | Blocked U-mode write triggers illegal-instruction | sstateen0 bit=0, U-mode writes controlled CSR | Illegal-instruction exception triggered (cause=2) | `norm:stateen_illegal_state_access` |
| SS-UCTL-09 | Blocked VU-mode access triggers virtual-instruction | sstateen0 bit=0, VU-mode accesses controlled CSR | Virtual-instruction exception triggered (cause=22) | `norm:stateen_illegal_state_access` |

> **[H Extension Dependency]** SS-UCTL-09 requires the Hypervisor extension (VU-mode scenario). See [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) Group 8 for related Hypervisor cross-tests.

**Test Steps Example (SS-UCTL-01)**:
1. In M-mode, set mstateen0.C=1 and mstateen0.SE0=1
2. Switch to S-mode
3. Set sstateen0.C=0
4. Configure PMP to allow U-mode execution
5. Install U-mode trap handler
6. Switch to U-mode
7. Attempt to access a custom CSR
8. Return to S-mode and verify that an illegal-instruction exception was caught

**Expected Result**:
- U-mode access to custom state triggers an illegal-instruction exception

**Assertions**:

---

### Group 3: sstateen0 Feature Bits

**Spec Reference**:
- `norm:stateen0_c_op`: C bit (bit 0) controls custom state access
- `norm:stateen0_fcsr_op`: FCSR bit (bit 1) controls fcsr in Zfinx scenarios
- `norm:stateen0_jvt_op`: JVT bit (bit 2) controls the Zcmt jvt CSR
- `norm:mstateen0_fcsr_roz`: FCSR bit is read-only zero when misa.F=1 (propagates to sstateen0)

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-FUNC-01 | sstateen0.C bit writable | S-mode writes sstateen0.C=1 then reads back | C bit is 1 (prerequisite: mstateen0.C=1) | `norm:stateen0_c_op` |
| SS-FUNC-02 | sstateen0.C=0 blocks U-mode custom access | Set sstateen0.C=0, U-mode accesses custom CSR | Illegal-instruction exception triggered | `norm:stateen0_c_op` |
| SS-FUNC-03 | sstateen0.C=1 permits U-mode custom access | Set sstateen0.C=1, U-mode accesses custom CSR | Access succeeds | `norm:stateen0_c_op` |
| SS-FUNC-04 | sstateen0.FCSR bit behavior (misa.F=1) | When misa.F=1, sstateen0.FCSR is read-only zero | FCSR bit reads back as 0 after writing 1 | `norm:mstateen0_fcsr_roz` |
| SS-FUNC-05 | sstateen0.FCSR=0 makes FP instructions illegal (misa.F=0) | Ensure misa.F=0, sstateen0.FCSR=0, U-mode executes FP instructions | All FP instructions trigger illegal-instruction exception | `norm:stateen0_fcsr_op` |
| SS-FUNC-06 | sstateen0.JVT bit writable | S-mode writes sstateen0.JVT=1 then reads back | JVT bit is 1 (prerequisite: Zcmt implemented and mstateen0.JVT=1) | `norm:stateen0_jvt_op` |
| SS-FUNC-07 | sstateen0.JVT=0 blocks U-mode jvt | Set sstateen0.JVT=0, U-mode reads jvt | Illegal-instruction exception triggered | `norm:stateen0_jvt_op` |
| SS-FUNC-08 | sstateen0.JVT=1 permits U-mode jvt | Set sstateen0.JVT=1, U-mode reads jvt | Access succeeds | `norm:stateen0_jvt_op` |
| SS-FUNC-09 | sstateen0 WPRI bits read-only zero | S-mode writes 1 to sstateen0 reserved bits then reads back | Reserved bits read back as 0 | `norm:stateen_reserved_roz` |

**Test Steps Example (SS-FUNC-05)**:
1. In M-mode, confirm misa.F=0 (Zfinx scenario)
2. Set mstateen0.FCSR=0 and mstateen0.SE0=1
3. Switch to S-mode
4. Confirm sstateen0.FCSR=0
5. Switch to U-mode
6. Execute a floating-point instruction (e.g., `fadd`)
7. Verify that an illegal-instruction exception is triggered

**Expected Result**:
- All floating-point instructions trigger an illegal-instruction exception, as if they all access fcsr

**Assertions**:

---

### Group 4: sstateen Bit Allocation and Read-Only Constraints

**Spec Reference**:
- `norm:sstateen_bit_allocation`: sstateen bits are allocated from bit 0 to bit 31
- `norm:sstateen_ro1_bits`: sstateen bits cannot be RO1 unless the same bit in mstateen and hstateen is also RO1
- `norm:sstateen_encroachment_bits_roz`: Encroaching bits from mstateen upper positions are permanently read-only zero in sstateen
- `norm:mstateen_bit_correspondence`: Defined bits in sstateen are also defined in mstateen
- `norm:mstateen_lower_priv_roz`: Bits that are zero in mstateen appear as read-only zero in sstateen

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-ALLOC-01 | sstateen0 bit allocation starts at bit 0 | Verify that defined bits in sstateen0 are concentrated in low bit positions | C=bit0, FCSR=bit1, JVT=bit2 | `norm:sstateen_bit_allocation` |
| SS-ALLOC-02 | sstateen0 RO1 bit constraint (no H extension) | Identify sstateen0 RO1 bits, verify that the same bit in mstateen0 is also RO1 | mstateen0 same bit is also RO1 | `norm:sstateen_ro1_bits` |
| SS-ALLOC-03 | sstateen0 RO1 bit constraint (with H extension) | Identify sstateen0 RO1 bits, verify that the same bit in both mstateen0 and hstateen0 is also RO1 | Both same bits are RO1 | `norm:sstateen_ro1_bits` |

> **[H Extension Dependency]** SS-ALLOC-03 requires the Hypervisor extension (verifying hstateen0 RO1 constraint). See [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) Group 8 (HCROSS-SSSTA-39~45 read-only constraints) for related Hypervisor cross-tests.
| SS-ALLOC-04 | sstateen0 encroachment bits read-only zero | If mstateen0 high-bit allocation encroaches into the lower 32 bits, check the corresponding bits in sstateen0 | Encroachment bits are read-only zero in sstateen0 | `norm:sstateen_encroachment_bits_roz` |
| SS-ALLOC-05 | sstateen bits correspond to mstateen | For each defined bit in sstateen0, check the corresponding bit in mstateen0 | Corresponding bit in mstateen0 is defined | `norm:mstateen_bit_correspondence` |
| SS-ALLOC-06 | mstateen zero-bit propagation to sstateen | Set a functional bit in mstateen0 to 0, S-mode writes the same bit in sstateen0 | Corresponding bit in sstateen0 is read-only zero | `norm:mstateen_lower_priv_roz` |
| SS-ALLOC-07 | mstateen one-bit releases sstateen propagation | Change a functional bit in mstateen0 from 0 to 1 | Corresponding bit in sstateen0 becomes writable | `norm:mstateen_lower_priv_roz` |
| SS-ALLOC-08 | sstateen unimplemented extension bits read-only zero | For an unimplemented extension, write the corresponding bit in sstateen0 | Corresponding bit reads back as 0 | `norm:stateen_unimplemented_state_roz` |

**Test Steps Example (SS-ALLOC-02)**:
1. In M-mode, set mstateen0.SE0=1
2. In M-mode, write 0 to the corresponding bit in mstateen0
3. Switch to S-mode
4. Attempt to write 0 to the corresponding bit in sstateen0
5. Read back sstateen0
6. If the bit reads back as 1 (RO1), return to M-mode
7. Attempt to clear the same bit in mstateen0
8. Verify that the same bit in mstateen0 must also be RO1

**Expected Result**:
- If a bit in sstateen0 is RO1, then the same bit in mstateen0 must also be RO1

**Assertions**:

---

### Group 5: sstateen Exception Behavior

**Spec Reference**:
- `norm:stateen_illegal_state_access`: Prevented access triggers an illegal-instruction exception; in VS/VU mode, a virtual-instruction exception is triggered when conditions apply
- `norm:stateen_implicit_state_update`: Instructions that implicitly update controlled state without reading it may or may not trigger an exception, as explicitly specified
- `norm:stateen_op`: stateen controls access at all lower privilege levels, not at its own level

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-EXC-01 | sstateen does not control S-mode itself | sstateen0 bit=0, S-mode accesses controlled state | S-mode access succeeds | `norm:stateen_op` |
| SS-EXC-02 | Blocked U-mode read triggers illegal-instruction | sstateen0 bit=0, U-mode reads controlled CSR | cause=2 (illegal-instruction) | `norm:stateen_illegal_state_access` |
| SS-EXC-03 | Blocked U-mode write triggers illegal-instruction | sstateen0 bit=0, U-mode writes controlled CSR | cause=2 (illegal-instruction) | `norm:stateen_illegal_state_access` |
| SS-EXC-04 | Blocked VU-mode access triggers virtual-instruction | sstateen0 bit=0 and VU-mode access | cause=22 (virtual-instruction) | `norm:stateen_illegal_state_access` |

> **[H Extension Dependency]** SS-EXC-04 requires the Hypervisor extension (VU-mode scenario triggering virtual-instruction). See [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) Group 8 for related Hypervisor cross-tests.
| SS-EXC-05 | Implicit state update instruction behavior | Execute an instruction that implicitly updates controlled state without reading it | Behavior depends on the specific specification definition | `norm:stateen_implicit_state_update` |

**Test Steps Example (SS-EXC-02)**:
1. In M-mode, configure mstateen0 (set the corresponding bit=1 to permit S-mode) and mstateen0.SE0=1
2. Switch to S-mode
3. Set sstateen0.JVT=0
4. Install U-mode trap handler
5. Switch to U-mode
6. Attempt to read the jvt CSR
7. Return to S-mode and verify the exception

**Expected Result**:
- U-mode reading jvt triggers an illegal-instruction exception (cause=2)

**Assertions**:

---

### Group 6: sstateen Initialization Requirements

**Spec Reference**:
- `norm:hstateen_sstateen_zero_initialization`: After M-mode software modifies mstateen, it is responsible for initializing the corresponding sstateen to zero

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-INIT-01 | sstateen0 initialized to zero | M-mode configures mstateen0 then writes zero to sstateen0, S-mode reads back | All writable bits are 0 | `norm:hstateen_sstateen_zero_initialization` |
| SS-INIT-02 | sstateen1 initialized to zero | M-mode configures mstateen1 then writes zero to sstateen1, S-mode reads back | All writable bits are 0 | `norm:hstateen_sstateen_zero_initialization` |
| SS-INIT-03 | sstateen2 initialized to zero | M-mode configures mstateen2 then writes zero to sstateen2, S-mode reads back | All writable bits are 0 | `norm:hstateen_sstateen_zero_initialization` |
| SS-INIT-04 | sstateen3 initialized to zero | M-mode configures mstateen3 then writes zero to sstateen3, S-mode reads back | All writable bits are 0 | `norm:hstateen_sstateen_zero_initialization` |
| SS-INIT-05 | sstateen should be zero on OS entry | Simulate OS first entry into S-mode, verify sstateen is initialized | All writable bits are 0 | `norm:hstateen_sstateen_zero_initialization` |

**Test Steps Example (SS-INIT-01)**:
1. In M-mode, set certain bits in mstateen0 to 1 (enable functionality)
2. Write zero to sstateen0
3. Set mstateen0.SE0=1
4. Switch to S-mode
5. Read sstateen0
6. Verify that all writable bits are zero

**Expected Result**:
- All writable bits in the sstateen0 readback value are 0

**Assertions**:

---

## 4. H-mode Test Groups

> **Prerequisite**: The following tests require an H (Hypervisor) extension implementation. M-mode must pre-set the corresponding mstateen bits to 1 to permit HS-mode access to hstateen.

> **[Migration Notice]** The following Group 7-12 H-mode tests have been migrated to [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) **Group 8 (Hypervisor x Ssstateen Cross-Testing)**, with test IDs renumbered to `HCROSS-SSSTA-01~50`. The original descriptions are retained in this file for reference; the actual implementation and execution shall follow the cross-test plan.

### Group 7: hstateen CSR Existence and Accessibility

> **[Migrated]** The 6 tests in this group have been migrated to `Hypervisor_cross_test_plan.md` Group 8.1, ID mapping: SS-HEXIST-01~06 -> HCROSS-SSSTA-01~06.

**Spec Reference**:
- `norm:hstateen_rv64_csrs`: hstateen0-3 are added when the H extension is implemented
- `norm:stateen_rv32_upper_bits_csrs`: For RV32, hstateen0h-3h are additionally provided
- `norm:hstateen_encoding`: hstateen encoding is identical to mstateen (except the controlled target is VS/VU modes)

**Prerequisite Configuration**: M-mode must set bit 63 of the corresponding mstateen to 1.

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-HEXIST-01 | hstateen0 readable in HS-mode | Configure mstateen0.SE0=1, HS-mode reads hstateen0 | No exception | `norm:hstateen_rv64_csrs` |
| SS-HEXIST-02 | hstateen0 writable in HS-mode | Configure mstateen0.SE0=1, HS-mode writes hstateen0 then reads back | No exception, writable bits take effect | `norm:hstateen_rv64_csrs` |
| SS-HEXIST-03 | hstateen1 readable/writable in HS-mode | Configure mstateen1 bit63=1, HS-mode reads/writes hstateen1 | No exception | `norm:hstateen_rv64_csrs` |
| SS-HEXIST-04 | hstateen2 readable/writable in HS-mode | Configure mstateen2 bit63=1, HS-mode reads/writes hstateen2 | No exception | `norm:hstateen_rv64_csrs` |
| SS-HEXIST-05 | hstateen3 readable/writable in HS-mode | Configure mstateen3 bit63=1, HS-mode reads/writes hstateen3 | No exception | `norm:hstateen_rv64_csrs` |
| SS-HEXIST-06 | hstateen0h readable/writable in HS-mode (RV32) | Under RV32, configure mstateen0.SE0=1, HS-mode reads/writes hstateen0h | No exception | `norm:stateen_rv32_upper_bits_csrs` |

**Test Steps Example (SS-HEXIST-01)**:
1. Set mstateen0 bit 63 (SE0) to 1 in M-mode
2. Switch to HS-mode
3. Read the hstateen0 CSR
4. Verify the read operation completes without exception
5. Return to M-mode

**Expected Result**:
- HS-mode reads hstateen0 normally, no illegal-instruction exception

**Assertions**:

---

### Group 8: hstateen Bit 63 Controls sstateen Access

> **[Migrated]** The 9 tests in this group have been migrated to `Hypervisor_cross_test_plan.md` Group 8.2, ID mapping: SS-HB63-01~09 -> HCROSS-SSSTA-07~15.

**Spec Reference**:
- `norm:hstateen_bit_63_op`: Bit 63 of hstateen controls VS-mode access to the corresponding sstateen
- `norm:hstateen_bit_63_writable`: Bit 63 of hstateen is always writable (not read-only)

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-HB63-01 | hstateen0 bit 63 writable (write 0) | HS-mode writes hstateen0 bit 63 to 0 then reads back | bit 63 reads back as 0 | `norm:hstateen_bit_63_writable` |
| SS-HB63-02 | hstateen0 bit 63 writable (write 1) | HS-mode writes hstateen0 bit 63 to 1 then reads back | bit 63 reads back as 1 | `norm:hstateen_bit_63_writable` |
| SS-HB63-03 | hstateen0.SE0=0 blocks VS-mode access to sstateen0 | Set hstateen0 bit63=0, VS-mode reads sstateen0 | Virtual-instruction exception triggered (cause=22) | `norm:hstateen_bit_63_op` |
| SS-HB63-04 | hstateen0.SE0=1 permits VS-mode access to sstateen0 | Set hstateen0 bit63=1, VS-mode reads sstateen0 | Access succeeds, no exception | `norm:hstateen_bit_63_op` |
| SS-HB63-05 | hstateen0.SE0=0 blocks VS-mode write to sstateen0 | Set hstateen0 bit63=0, VS-mode writes sstateen0 | Virtual-instruction exception triggered (cause=22) | `norm:hstateen_bit_63_op` |
| SS-HB63-06 | hstateen1 bit 63 writable | HS-mode writes hstateen1 bit 63 to 0 and 1 | Each readback value matches the written value | `norm:hstateen_bit_63_writable` |
| SS-HB63-07 | hstateen1 bit63=0 blocks VS-mode access to sstateen1 | Set hstateen1 bit63=0, VS-mode reads sstateen1 | Virtual-instruction exception triggered | `norm:hstateen_bit_63_op` |
| SS-HB63-08 | hstateen2 bit 63 controls sstateen2 | Set hstateen2 bit63=0/1, VS-mode accesses sstateen2 | bit63=0 triggers exception, bit63=1 succeeds | `norm:hstateen_bit_63_op` |
| SS-HB63-09 | hstateen3 bit 63 controls sstateen3 | Set hstateen3 bit63=0/1, VS-mode accesses sstateen3 | bit63=0 triggers exception, bit63=1 succeeds | `norm:hstateen_bit_63_op` |

**Test Steps Example (SS-HB63-03)**:
1. In M-mode, set mstateen0.SE0=1 (to permit HS-mode)
2. Switch to HS-mode
3. Set hstateen0 bit 63 to 0
4. Configure VS-mode environment and install trap handler
5. Switch to VS-mode
6. Attempt to read the sstateen0 CSR
7. Return to HS-mode and verify the exception

**Expected Result**:
- VS-mode reading sstateen0 triggers a virtual-instruction exception (cause=22)

**Assertions**:

---

### Group 9: hstateen Read-Only Zero Propagation to VS-mode sstateen

> **[Migrated]** The 5 tests in this group have been migrated to `Hypervisor_cross_test_plan.md` Group 8.3, ID mapping: SS-VSPROP-01~05 -> HCROSS-SSSTA-16~20.

**Spec Reference**:
- `norm:sstateen_vsmode_access_roz`: Bits that are zero in hstateen (whether read-only zero or set to zero) appear as read-only zero when VS-mode accesses sstateen

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-VSPROP-01 | hstateen0.C=0 propagates to VS-mode sstateen0.C | Set hstateen0.C=0 and bit63=1, VS-mode writes sstateen0.C=1 then reads back | sstateen0.C reads back as 0 in VS-mode | `norm:sstateen_vsmode_access_roz` |
| SS-VSPROP-02 | hstateen0.C=1 releases VS-mode propagation | Set hstateen0.C=1 and bit63=1, VS-mode writes sstateen0.C=1 then reads back | sstateen0.C reads back as 1 in VS-mode | `norm:sstateen_vsmode_access_roz` |
| SS-VSPROP-03 | hstateen0.JVT=0 propagates to VS-mode sstateen0.JVT | Set hstateen0.JVT=0 and bit63=1, VS-mode writes sstateen0.JVT=1 then reads back | sstateen0.JVT reads back as 0 in VS-mode | `norm:sstateen_vsmode_access_roz` |
| SS-VSPROP-04 | hstateen0 multiple-bit simultaneous propagation | Set multiple functional bits in hstateen0 to 0, VS-mode verifies each corresponding bit in sstateen0 | All corresponding bits are read-only zero in VS-mode | `norm:sstateen_vsmode_access_roz` |
| SS-VSPROP-05 | hstateen0 bit change from 0 to 1 releases propagation | First set hstateen0.C=0 and verify propagation, then change to 1 | sstateen0.C becomes writable in VS-mode | `norm:sstateen_vsmode_access_roz` |

**Test Steps Example (SS-VSPROP-01)**:
1. In M-mode, set mstateen0.SE0=1 and mstateen0.C=1
2. Switch to HS-mode
3. Set hstateen0 bit63=1 and hstateen0.C=0
4. Configure VS-mode environment
5. Switch to VS-mode
6. Write 1 to the C bit of sstateen0
7. Read back sstateen0
8. Return to HS-mode/M-mode to check the result

**Expected Result**:
- sstateen0.C reads back as 0 in VS-mode (propagated as read-only zero by hstateen0)

**Assertions**:

---

### Group 10: hstateen0 Individual Feature Bit Controls

> **[Migrated]** The 18 tests in this group have been migrated to `Hypervisor_cross_test_plan.md` Group 8.4, ID mapping: SS-HSE0-01~03 -> HCROSS-SSSTA-21~23, SS-HENVCFG-01~03 -> HCROSS-SSSTA-24~26, SS-HCSRIND-01~03 -> HCROSS-SSSTA-27~29, SS-HIMSIC-01~03 -> HCROSS-SSSTA-30~32, SS-HAIA-01~03 -> HCROSS-SSSTA-33~35, SS-HCTX-01~03 -> HCROSS-SSSTA-36~38.

**Spec Reference**:
- `norm:hstateen0_SE0_op`: hstateen0.SE0 (bit 63) controls VS-mode access to sstateen0
- `norm:hstateen0_envcfg_op`: hstateen0.ENVCFG (bit 62) controls VS-mode access to senvcfg
- `norm:hstateen0_csrind_op`: hstateen0.CSRIND (bit 60) controls VS-mode access to siselect/sireg\* (actually vsiselect/vsireg\*)
- `norm:hstateen0_imsic_op`: hstateen0.IMSIC (bit 58) controls guest IMSIC state and vstopei
- `norm:hstateen0_aia_op`: hstateen0.AIA (bit 59) controls remaining Ssaia state not covered by CSRIND/IMSIC
- `norm:hstateen0_context_op`: hstateen0.CONTEXT (bit 57) controls VS-mode access to scontext

#### 10.1 SE0 Bit (bit 63) -- sstateen0 VS-mode Access

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-HSE0-01 | hstateen0.SE0=0 blocks VS-mode read of sstateen0 | Set hstateen0.SE0=0, VS-mode reads sstateen0 | Virtual-instruction exception triggered | `norm:hstateen0_SE0_op` |
| SS-HSE0-02 | hstateen0.SE0=0 blocks VS-mode write to sstateen0 | Set hstateen0.SE0=0, VS-mode writes sstateen0 | Virtual-instruction exception triggered | `norm:hstateen0_SE0_op` |
| SS-HSE0-03 | hstateen0.SE0=1 permits VS-mode access to sstateen0 | Set hstateen0.SE0=1, VS-mode reads/writes sstateen0 | Access succeeds | `norm:hstateen0_SE0_op` |

#### 10.2 ENVCFG Bit (bit 62) -- senvcfg VS-mode Access

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-HENVCFG-01 | hstateen0.ENVCFG=0 blocks VS-mode read of senvcfg | Set ENVCFG=0, VS-mode reads senvcfg | Virtual-instruction exception triggered | `norm:hstateen0_envcfg_op` |
| SS-HENVCFG-02 | hstateen0.ENVCFG=0 blocks VS-mode write to senvcfg | Set ENVCFG=0, VS-mode writes senvcfg | Virtual-instruction exception triggered | `norm:hstateen0_envcfg_op` |
| SS-HENVCFG-03 | hstateen0.ENVCFG=1 permits VS-mode access to senvcfg | Set ENVCFG=1, VS-mode reads/writes senvcfg | Access succeeds | `norm:hstateen0_envcfg_op` |

#### 10.3 CSRIND Bit (bit 60) -- siselect/sireg VS-mode Access

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-HCSRIND-01 | hstateen0.CSRIND=0 blocks VS-mode read of siselect | Set CSRIND=0, VS-mode reads siselect (actually vsiselect) | Virtual-instruction exception triggered | `norm:hstateen0_csrind_op` |
| SS-HCSRIND-02 | hstateen0.CSRIND=0 blocks VS-mode read of sireg\* | Set CSRIND=0, VS-mode reads sireg (actually vsireg) | Virtual-instruction exception triggered | `norm:hstateen0_csrind_op` |
| SS-HCSRIND-03 | hstateen0.CSRIND=1 permits VS-mode access | Set CSRIND=1, VS-mode reads siselect/sireg\* | Access succeeds | `norm:hstateen0_csrind_op` |

#### 10.4 IMSIC Bit (bit 58) -- Guest IMSIC Control

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-HIMSIC-01 | hstateen0.IMSIC=0 blocks VS-mode access to IMSIC | Set IMSIC=0, VS-mode reads stopei (actually vstopei) | Virtual-instruction exception triggered | `norm:hstateen0_imsic_op` |
| SS-HIMSIC-02 | hstateen0.IMSIC=1 permits VS-mode access to IMSIC | Set IMSIC=1, VS-mode reads stopei | Access succeeds | `norm:hstateen0_imsic_op` |
| SS-HIMSIC-03 | hstateen0.IMSIC=0 equivalent to VGEIN=0 | Set IMSIC=0, verify VS-mode cannot access IMSIC, equivalent to hstatus.VGEIN=0 | VS-mode cannot access guest IMSIC | `norm:hstateen0_imsic_op` |

#### 10.5 AIA Bit (bit 59) -- Ssaia Remaining State VS-mode Control

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-HAIA-01 | hstateen0.AIA=0 blocks VS-mode Ssaia state | Set AIA=0, VS-mode accesses Ssaia non-CSRIND/IMSIC state | Virtual-instruction exception triggered | `norm:hstateen0_aia_op` |
| SS-HAIA-02 | hstateen0.AIA=1 permits VS-mode Ssaia state | Set AIA=1, VS-mode accesses Ssaia remaining state | Access succeeds | `norm:hstateen0_aia_op` |
| SS-HAIA-03 | hstateen0.AIA does not affect CSRIND/IMSIC control | Set AIA=0 but CSRIND=1 and IMSIC=1, VS-mode accesses siselect/stopei | Access succeeds (AIA does not control state under CSRIND/IMSIC jurisdiction) | `norm:hstateen0_aia_op` |

#### 10.6 CONTEXT Bit (bit 57) -- scontext VS-mode Control

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-HCTX-01 | hstateen0.CONTEXT=0 blocks VS-mode read of scontext | Set CONTEXT=0, VS-mode reads scontext | Virtual-instruction exception triggered | `norm:hstateen0_context_op` |
| SS-HCTX-02 | hstateen0.CONTEXT=0 blocks VS-mode write to scontext | Set CONTEXT=0, VS-mode writes scontext | Virtual-instruction exception triggered | `norm:hstateen0_context_op` |
| SS-HCTX-03 | hstateen0.CONTEXT=1 permits VS-mode access to scontext | Set CONTEXT=1, VS-mode reads/writes scontext | Access succeeds | `norm:hstateen0_context_op` |

---

### Group 11: hstateen Read-Only Constraints

> **[Migrated]** The 7 tests in this group have been migrated to `Hypervisor_cross_test_plan.md` Group 8.5, ID mapping: SS-HRO-01~07 -> HCROSS-SSSTA-39~45.

**Spec Reference**:
- `norm:hstateen_ro1_bits`: Bits in hstateen cannot be RO1 unless the same bit in mstateen is also RO1
- `norm:stateen_warl_access`: Each standard-defined bit is WARL
- `norm:stateen_unimplemented_state_roz`: Bits controlling unimplemented state are read-only zero
- `norm:stateen_reserved_roz`: Reserved bits are read-only zero

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-HRO-01 | hstateen0 RO1 bit constraint | Identify any RO1 bits in hstateen0, verify the same bit in mstateen0 is also RO1 | mstateen0 same bit is also RO1 | `norm:hstateen_ro1_bits` |
| SS-HRO-02 | hstateen1 RO1 bit constraint | Identify any RO1 bits in hstateen1, verify the same bit in mstateen1 is also RO1 | mstateen1 same bit is also RO1 | `norm:hstateen_ro1_bits` |
| SS-HRO-03 | hstateen2 RO1 bit constraint | Identify any RO1 bits in hstateen2, verify the same bit in mstateen2 is also RO1 | mstateen2 same bit is also RO1 | `norm:hstateen_ro1_bits` |
| SS-HRO-04 | hstateen3 RO1 bit constraint | Identify any RO1 bits in hstateen3, verify the same bit in mstateen3 is also RO1 | mstateen3 same bit is also RO1 | `norm:hstateen_ro1_bits` |
| SS-HRO-05 | hstateen0 reserved bits read-only zero | Write 1 to hstateen0 WPRI fields then read back | Reserved bits read back as 0 | `norm:stateen_reserved_roz` |
| SS-HRO-06 | hstateen0 unimplemented extension bits read-only zero | For bits corresponding to unimplemented extensions, write 1 then read back | Corresponding bits read back as 0 | `norm:stateen_unimplemented_state_roz` |
| SS-HRO-07 | hstateen0 WARL write of legal values | Write legal values to hstateen0 then read back | Readback value matches written value (writable bits only) | `norm:stateen_warl_access` |

**Test Steps Example (SS-HRO-01)**:
1. In M-mode, set mstateen0.SE0=1
2. Switch to HS-mode
3. For hstateen0, attempt to write 0 to each bit individually
4. Read back hstateen0
5. If any bit reads back as 1 (RO1), return to M-mode
6. In M-mode, attempt to clear the same bit in mstateen0
7. Read back mstateen0 and verify the same bit is also RO1

**Expected Result**:
- If a bit in hstateen0 is RO1, then the same bit in mstateen0 must also be RO1

**Assertions**:

---

### Group 12: hstateen Encoding Consistency with mstateen

> **[Migrated]** The 5 tests in this group have been migrated to `Hypervisor_cross_test_plan.md` Group 8.6, ID mapping: SS-HENC-01~05 -> HCROSS-SSSTA-46~50.

**Spec Reference**:
- `norm:hstateen_encoding`: The hstateen CSR encodings are identical to the mstateen CSRs, except that hstateen controls access for VS/VU modes

| Test ID | Test Name | Test Description | Expected Result | Spec Reference |
|---------|-----------|------------------|-----------------|----------------|
| SS-HENC-01 | hstateen0 bit fields match mstateen0 | Compare the writable bit masks of hstateen0 and mstateen0 | Bit field definitions are consistent (C/FCSR/JVT/SE0/ENVCFG/CSRIND/IMSIC/AIA/CONTEXT bit positions are identical) | `norm:hstateen_encoding` |
| SS-HENC-02 | hstateen0 feature bits symmetric with mstateen0 | Write all-ones to hstateen0, read back effective bits; write all-ones to mstateen0, read back effective bits; compare the overlapping portion | hstateen0 effective bits should be a subset of mstateen0 effective bits (mstateen0 may have bits such as P1P13/SRMCFG that have no hstateen0 counterpart) | `norm:hstateen_encoding` |
| SS-HENC-03 | hstateen1 encoding matches mstateen1 | Compare the writable bit masks of hstateen1 and mstateen1 | Bit field definitions are consistent | `norm:hstateen_encoding` |
| SS-HENC-04 | hstateen2 encoding matches mstateen2 | Compare the writable bit masks of hstateen2 and mstateen2 | Bit field definitions are consistent | `norm:hstateen_encoding` |
| SS-HENC-05 | hstateen3 encoding matches mstateen3 | Compare the writable bit masks of hstateen3 and mstateen3 | Bit field definitions are consistent | `norm:hstateen_encoding` |

**Test Steps Example (SS-HENC-01)**:
1. In M-mode, write all-ones to mstateen0, read back to obtain the mstateen0 writable mask
2. Set mstateen0.SE0=1
3. Switch to HS-mode
4. Write all-ones to hstateen0, read back to obtain the hstateen0 writable mask
5. Return to M-mode
6. Compare the overlapping bit field positions between the two masks

**Expected Result**:
- The bit positions for C (bit 0), FCSR (bit 1), JVT (bit 2), CONTEXT (bit 57), IMSIC (bit 58), AIA (bit 59), CSRIND (bit 60), ENVCFG (bit 62), and SE0 (bit 63) in hstateen0 are exactly consistent with mstateen0

**Assertions**:

---

## 5. Verification Plan

### 5.1 Automated Testing

### 5.2 Manual Verification

- Verify that all normative rules are covered by at least one test case
- Verify that H-extension-related tests (Group 7-12) are correctly skipped on platforms without the H extension
- Confirm that virtual-instruction exceptions (cause=22) are correctly triggered in VS/VU mode
- Verify coverage of RV32-specific hstateen0h tests
- Confirm that mstateen pre-configuration is correct and does not affect the accuracy of Ssstateen standalone tests

---

## 6. Test Criteria

### 6.1 Pass Criteria

- All assertions in all test cases pass
- Test framework outputs `RESULT: ALL PASSED`
- No unexpected exceptions occur

### 6.2 Fail Criteria

- Any assertion failure results in the corresponding test case being judged FAIL
- Any unexpected exception (trap) results in a FAIL judgment
- sstateen/hstateen WARL bit behavior does not conform to the specification, resulting in a FAIL judgment

---

## 7. Coverage Matrix

### 7.1 Normative Rules Coverage Traceability

| Spec Reference | Covered Test IDs |
|----------------|------------------|
| `norm:sstateen_rv64_csrs` | SS-EXIST-01~05 |
| `norm:hstateen_rv64_csrs` | SS-HEXIST-01~05 |
| `norm:stateen_rv32_upper_bits_csrs` | SS-HEXIST-06 |
| `norm:stateen_op` | SS-UCTL-03, SS-EXC-01 |
| `norm:stateen_illegal_state_access` | SS-UCTL-01, SS-UCTL-04, SS-UCTL-06~09, SS-EXC-02~04 |
| `norm:stateen_implicit_state_update` | SS-EXC-05 |
| `norm:sstateen_user_access_control` | SS-UCTL-01~09 |
| `norm:sstateen_bit_allocation` | SS-EXIST-06, SS-ALLOC-01 |
| `norm:sstateen_encroachment_bits_roz` | SS-ALLOC-04 |
| `norm:hstateen_encoding` | SS-HENC-01~05 |
| `norm:stateen_warl_access` | SS-HRO-07 |
| `norm:stateen_unimplemented_state_roz` | SS-ALLOC-08, SS-HRO-06 |
| `norm:stateen_reserved_roz` | SS-FUNC-09, SS-HRO-05 |
| `norm:mstateen_lower_priv_roz` | SS-ALLOC-06~07 |
| `norm:sstateen_vsmode_access_roz` | SS-VSPROP-01~05 |
| `norm:sstateen_ro1_bits` | SS-ALLOC-02~03 |
| `norm:hstateen_ro1_bits` | SS-HRO-01~04 |
| `norm:hstateen_sstateen_zero_initialization` | SS-INIT-01~05 |
| `norm:hstateen_bit_63_op` | SS-HB63-03~05, SS-HB63-07~09 |
| `norm:hstateen_bit_63_writable` | SS-HB63-01~02, SS-HB63-06 |
| `norm:stateen0_c_op` | SS-UCTL-01~02, SS-FUNC-01~03 |
| `norm:stateen0_fcsr_op` | SS-UCTL-06, SS-FUNC-04~05 |
| `norm:mstateen0_fcsr_roz` | SS-FUNC-04 |
| `norm:stateen0_jvt_op` | SS-UCTL-04~05, SS-FUNC-06~08 |
| `norm:hstateen0_SE0_op` | SS-HSE0-01~03 |
| `norm:hstateen0_envcfg_op` | SS-HENVCFG-01~03 |
| `norm:hstateen0_csrind_op` | SS-HCSRIND-01~03 |
| `norm:hstateen0_imsic_op` | SS-HIMSIC-01~03 |
| `norm:hstateen0_aia_op` | SS-HAIA-01~03 |
| `norm:hstateen0_context_op` | SS-HCTX-01~03 |
| `norm:mstateen_bit_correspondence` | SS-ALLOC-05 |
