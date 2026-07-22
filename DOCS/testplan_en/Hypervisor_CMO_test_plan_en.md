# Hypervisor × CMO Cross Test Plan

## Overview

This test plan covers the cross-functional interaction points between the RISC-V Hypervisor (H) extension and the CMO (Cache Management Operations) extension.

Reference specifications:
- `SPEC/hypervisor.adoc`: henvcfg CBIE/CBCFE/CBZE field definitions
- `SPEC/riscv-isa-manual/src/unpriv/cmo.adoc`: CMO instruction behavior when V=1, htinst/mtinst standard transformation

The CMO extension comprises three sub-extensions:
- **Zicbom**: Cache-Block Management instructions (cbo.inval, cbo.clean, cbo.flush)
- **Zicboz**: Cache-Block Zero instruction (cbo.zero)
- **Zicbop**: Cache-Block Prefetch instructions (prefetch.r, prefetch.w, prefetch.i)

### SPEC Sections Covered by This Document
- henvcfg CBIE/CBCFE/CBZE field control of VS/VU-mode CBO instructions
- cbo.inval flush override semantics when V=1 (HS-mode CBIE=01 forces flush)
- htinst/mtinst standard transformation format for CBO instructions
- CMO instruction guest-page-fault behavior under G-stage address translation
- prefetch instructions do not trigger virtual-instruction in VS/VU-mode

### Covered by Other Test Plans
- CMO base functionality (M/HS/U-mode CSR control, functional semantics, encoding) → `CMO_test_plan.md`
- Hypervisor core functionality (hstatus/hedeleg/hideleg/VS CSRs) → `Hypervisor_test_plan.md`

---

## Covered Specification Points

| Norm ID | Original Text |
|---------|---------------|
| `norm:henvcfg_cbze` | The Zicboz extension adds the CBZE field to henvcfg. The CBZE field applies to execution of CBO.ZERO in privilege modes VS and VU, and only when the instruction is HS-qualified. If not HS-qualified, raises illegal-instruction. If HS-qualified and CBZE=1, enabled; if CBZE=0, raises virtual-instruction. When Zicboz not implemented, CBZE is read-only zero. |
| `norm:henvcfg_cbcfe` | The Zicbom extension adds the CBCFE field to henvcfg. When V=1, if CBO.CLEAN and CBO.FLUSH are not HS-qualified, they raise illegal-instruction. If HS-qualified and CBCFE=1, enabled; if CBCFE=0, raises virtual-instruction. When Zicbom not implemented, CBCFE is read-only zero. |
| `norm:henvcfg_cbie` | The Zicbom extension adds the CBIE WARL field to henvcfg. The CBIE field controls execution of CBO.INVAL in privilege modes VS and VU. The encoding 10b is reserved. When Zicbom not implemented, CBIE is read-only zero. |
| `norm:cbo-inval_h-mode_veq1_op` | When V=1, if CBO.INVAL is not HS-qualified, raises illegal-instruction. If HS-qualified and CBIE=01b or 11b, enabled; otherwise raises virtual-instruction. |
| `norm:cbo-inval_h-mode_op0` | If CBO.INVAL is enabled in HS-mode to perform a flush operation, then when enabled in VS/VU-mode it performs a flush operation, even if CBIE=11b. Otherwise, behavior depends on CBIE encoding. |
| `norm:cbo-inval_h-mode_op1` | CBIE=01b: The instruction is executed and performs a flush operation, even if configured by VS-mode to perform an invalidate operation. |
| `norm:cbo-inval_h-mode_op2` | CBIE=11b: The instruction is executed and performs an invalidate operation, unless configured by VS-mode to perform a flush operation. |
| `norm:h_trans_cache` | For writing mtinst or htinst on a trap, the standard transformation for cache-block management and zero instructions: {operation[11:0], 0x0, funct3, 0x0, opcode}. The operation field is the 12 most significant bits of the trapping instruction. |
| `norm:cbm_unperm_fault` | If access not permitted, a cache-block management instruction raises store page-fault or store guest-page-fault if address translation does not permit any access. |
| `norm:cbz_unperm_fault` | If access not permitted, a cache-block zero instruction raises store page-fault or store guest-page-fault if address translation does not permit write access. |
| `norm:cbp_unperm_noexcep` | If access not permitted, a cache-block prefetch instruction does not raise any exceptions. |
| `norm:fault_excep_csr` | When a page-fault, guest-page-fault, or access-fault exception is taken, the relevant *tval CSR is written with the faulting effective address (i.e. the value of rs1). |
| `norm:cbxe_unaffected` | The CBIE/CBCFE/CBZE fields in each envcfg register do not affect the read and write behavior of the same fields in the other envcfg registers. |

---

## Group 1. henvcfg CBIE Control — VS/VU-mode cbo.inval

**Spec Reference**:
- `norm:henvcfg_cbie`: henvcfg.CBIE WARL field, encoding 10b reserved, read-only zero when not implemented
- `norm:cbo-inval_h-mode_veq1_op`: CBO.INVAL exception/execution determination when V=1
- `norm:cbo-inval_h-mode_op0`: HS-mode flush configuration overrides VS/VU behavior
- `norm:cbo-inval_h-mode_op1`: henvcfg.CBIE=01 performs flush
- `norm:cbo-inval_h-mode_op2`: henvcfg.CBIE=11 performs invalidate (unless VS-mode configures flush)

**Test Scope**: Verify henvcfg.CBIE control of VS/VU-mode cbo.inval, including flush override semantics.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCBIE-01 | VS-mode henvcfg.CBIE=00 triggers virtual-instruction | menvcfg.CBIE=11, henvcfg.CBIE=00, VS-mode executes cbo.inval | virtual-instruction exception (cause=22) |
| HCBIE-02 | VS-mode henvcfg.CBIE=01 executes flush | menvcfg.CBIE=11, henvcfg.CBIE=01, VS-mode executes cbo.inval | Normal execution (flush operation) |
| HCBIE-03 | VS-mode henvcfg.CBIE=11 executes invalidate | menvcfg.CBIE=11, henvcfg.CBIE=11, VS-mode executes cbo.inval | Normal execution (invalidate) |
| HCBIE-04 | VU-mode henvcfg.CBIE=00 triggers virtual-instruction | menvcfg.CBIE=11, henvcfg.CBIE=00, VU-mode executes cbo.inval | virtual-instruction exception (cause=22) |
| HCBIE-05 | VU-mode senvcfg.CBIE=00 triggers virtual-instruction | menvcfg.CBIE=11, henvcfg.CBIE=11, senvcfg.CBIE=00, VU-mode executes cbo.inval | virtual-instruction exception (cause=22) |
| HCBIE-06 | VU-mode two-level CBIE both non-zero executes | menvcfg.CBIE=11, henvcfg.CBIE=11, senvcfg.CBIE=11, VU-mode executes cbo.inval | Normal execution (invalidate) |
| HCBIE-07 | VU-mode henvcfg.CBIE=01 executes flush | menvcfg.CBIE=11, henvcfg.CBIE=01, senvcfg.CBIE=11, VU-mode executes cbo.inval | Normal execution (flush operation) |
| HCBIE-08 | VU-mode senvcfg.CBIE=01 executes flush | menvcfg.CBIE=11, henvcfg.CBIE=11, senvcfg.CBIE=01, VU-mode executes cbo.inval | Normal execution (flush operation) |
| HCBIE-09 | henvcfg.CBIE WARL reserved encoding 10 | Write henvcfg.CBIE=10, read back | WARL: does not retain encoding 10b |
| HCBIE-10 | henvcfg.CBIE read-only zero when Zicbom not implemented | If Zicbom not implemented, read henvcfg.CBIE | Read-only zero |
| HCBIE-11 | menvcfg.CBIE=01 forces VS-mode flush (overrides henvcfg.CBIE=11) | menvcfg.CBIE=01, henvcfg.CBIE=11, VS-mode executes cbo.inval | Executes flush (HS-mode flush config override) |
| HCBIE-12 | menvcfg.CBIE=01 forces VU-mode flush | menvcfg.CBIE=01, henvcfg.CBIE=11, senvcfg.CBIE=11, VU-mode executes cbo.inval | Executes flush (HS-mode flush config override) |
| HCBIE-13 | henvcfg.CBIE does not affect menvcfg/senvcfg.CBIE | Write henvcfg.CBIE=00, read menvcfg.CBIE/senvcfg.CBIE | Other envcfg CBIE fields unaffected |

---

## Group 2. henvcfg CBCFE Control — VS/VU-mode cbo.clean / cbo.flush

**Spec Reference**:
- `norm:henvcfg_cbcfe`: henvcfg.CBCFE controls VS/VU-mode CBO.CLEAN/CBO.FLUSH, read-only zero when not implemented
- `norm:cbxe_unaffected`: CBCFE fields in each envcfg register do not affect each other

**Test Scope**: Verify henvcfg.CBCFE control of VS/VU-mode cbo.clean and cbo.flush.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCBCFE-01 | VS-mode henvcfg.CBCFE=0 cbo.clean triggers virtual-instruction | menvcfg.CBCFE=1, henvcfg.CBCFE=0, VS-mode executes cbo.clean | virtual-instruction exception (cause=22) |
| HCBCFE-02 | VS-mode henvcfg.CBCFE=0 cbo.flush triggers virtual-instruction | menvcfg.CBCFE=1, henvcfg.CBCFE=0, VS-mode executes cbo.flush | virtual-instruction exception (cause=22) |
| HCBCFE-03 | VS-mode henvcfg.CBCFE=1 cbo.clean executes normally | menvcfg.CBCFE=1, henvcfg.CBCFE=1, VS-mode executes cbo.clean | Normal execution |
| HCBCFE-04 | VS-mode henvcfg.CBCFE=1 cbo.flush executes normally | menvcfg.CBCFE=1, henvcfg.CBCFE=1, VS-mode executes cbo.flush | Normal execution |
| HCBCFE-05 | VU-mode henvcfg.CBCFE=0 triggers virtual-instruction | menvcfg.CBCFE=1, henvcfg.CBCFE=0, VU-mode executes cbo.clean | virtual-instruction exception (cause=22) |
| HCBCFE-06 | VU-mode senvcfg.CBCFE=0 triggers virtual-instruction | menvcfg.CBCFE=1, henvcfg.CBCFE=1, senvcfg.CBCFE=0, VU-mode executes cbo.clean | virtual-instruction exception (cause=22) |
| HCBCFE-07 | VU-mode two-level CBCFE=1 cbo.clean executes normally | menvcfg.CBCFE=1, henvcfg.CBCFE=1, senvcfg.CBCFE=1, VU-mode executes cbo.clean | Normal execution |
| HCBCFE-08 | VU-mode two-level CBCFE=1 cbo.flush executes normally | menvcfg.CBCFE=1, henvcfg.CBCFE=1, senvcfg.CBCFE=1, VU-mode executes cbo.flush | Normal execution |
| HCBCFE-09 | henvcfg.CBCFE read-only zero when Zicbom not implemented | If Zicbom not implemented, read henvcfg.CBCFE | Read-only zero |
| HCBCFE-10 | henvcfg.CBCFE does not affect menvcfg/senvcfg.CBCFE | Write henvcfg.CBCFE=0, read menvcfg.CBCFE/senvcfg.CBCFE | Other envcfg CBCFE fields unaffected |

---

## Group 3. henvcfg CBZE Control — VS/VU-mode cbo.zero

**Spec Reference**:
- `norm:henvcfg_cbze`: henvcfg.CBZE controls VS/VU-mode CBO.ZERO, read-only zero when not implemented
- `norm:cbxe_unaffected`: CBZE fields in each envcfg register do not affect each other

**Test Scope**: Verify henvcfg.CBZE control of VS/VU-mode cbo.zero.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCBZE-01 | VS-mode henvcfg.CBZE=0 triggers virtual-instruction | menvcfg.CBZE=1, henvcfg.CBZE=0, VS-mode executes cbo.zero | virtual-instruction exception (cause=22) |
| HCBZE-02 | VS-mode henvcfg.CBZE=1 executes normally | menvcfg.CBZE=1, henvcfg.CBZE=1, VS-mode executes cbo.zero | Normal execution |
| HCBZE-03 | VU-mode henvcfg.CBZE=0 triggers virtual-instruction | menvcfg.CBZE=1, henvcfg.CBZE=0, VU-mode executes cbo.zero | virtual-instruction exception (cause=22) |
| HCBZE-04 | VU-mode senvcfg.CBZE=0 triggers virtual-instruction | menvcfg.CBZE=1, henvcfg.CBZE=1, senvcfg.CBZE=0, VU-mode executes cbo.zero | virtual-instruction exception (cause=22) |
| HCBZE-05 | VU-mode two-level CBZE=1 executes normally | menvcfg.CBZE=1, henvcfg.CBZE=1, senvcfg.CBZE=1, VU-mode executes cbo.zero | Normal execution |
| HCBZE-06 | henvcfg.CBZE read-only zero when Zicboz not implemented | If Zicboz not implemented, read henvcfg.CBZE | Read-only zero |
| HCBZE-07 | henvcfg.CBZE does not affect menvcfg/senvcfg.CBZE | Write henvcfg.CBZE=0, read menvcfg.CBZE/senvcfg.CBZE | Other envcfg CBZE fields unaffected |

---

## Group 4. CMO htinst/mtinst Standard Transformation

**Spec Reference**:
- `norm:h_trans_cache`: Standard transformation format for CBO instructions is {operation[11:0], 0x0, funct3, 0x0, opcode}
- Hypervisor SPEC: htinst may be written as zero instead of the standard transformation

**Standard Transformation Format**:
```
[31:20] operation  (12 most significant bits of the instruction)
[19:15] 0x0
[14:12] funct3 (=2 for CBO)
[11:7]  0x0
[6:0]   opcode (=0x0F for MISC-MEM)
```

Transformation values:
- cbo.inval → 0x0000200F
- cbo.clean → 0x0010200F
- cbo.flush → 0x0020200F
- cbo.zero  → 0x0040200F

**Test Scope**: Verify correct htinst/mtinst write when CBO instructions trigger a trap.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCXINST-01 | cbo.clean page-fault htinst standard transformation | VS-mode executes cbo.clean triggering page-fault to HS-mode, read htinst | htinst = 0x0010200F or 0 |
| HCXINST-02 | cbo.flush page-fault htinst standard transformation | VS-mode executes cbo.flush triggering page-fault, read htinst | htinst = 0x0020200F or 0 |
| HCXINST-03 | cbo.inval page-fault htinst standard transformation | VS-mode executes cbo.inval triggering page-fault, read htinst | htinst = 0x0000200F or 0 |
| HCXINST-04 | cbo.zero page-fault htinst standard transformation | VS-mode executes cbo.zero triggering page-fault, read htinst | htinst = 0x0040200F or 0 |
| HCXINST-05 | cbo.clean trap to M-mode mtinst | VS-mode executes cbo.clean triggering trap to M-mode, read mtinst | mtinst = 0x0010200F or 0 |
| HCXINST-06 | cbo.zero trap to M-mode mtinst | VS-mode executes cbo.zero triggering trap to M-mode, read mtinst | mtinst = 0x0040200F or 0 |
| HCXINST-07 | cbo.inval virtual-instruction htinst | henvcfg.CBIE=00, VS-mode executes cbo.inval triggering virtual-instruction, read htinst | htinst = 0x0000200F or 0 |
| HCXINST-08 | cbo.clean virtual-instruction htinst | henvcfg.CBCFE=0, VS-mode executes cbo.clean triggering virtual-instruction, read htinst | htinst = 0x0010200F or 0 |
| HCXINST-09 | cbo.zero virtual-instruction htinst | henvcfg.CBZE=0, VS-mode executes cbo.zero triggering virtual-instruction, read htinst | htinst = 0x0040200F or 0 |
| HCXINST-10 | VU-mode cbo.inval virtual-instruction htinst | henvcfg.CBIE=00, VU-mode executes cbo.inval, read htinst | htinst = 0x0000200F or 0 |
| HCXINST-11 | VU-mode cbo.zero virtual-instruction htinst | henvcfg.CBZE=0, VU-mode executes cbo.zero, read htinst | htinst = 0x0040200F or 0 |

---

## Group 5. CMO G-stage Address Translation Faults

**Spec Reference**:
- `norm:cbm_unperm_fault`: Management instructions raise store guest-page-fault when G-stage translation does not permit access
- `norm:cbz_unperm_fault`: cbo.zero raises store guest-page-fault when G-stage lacks write permission
- `norm:fault_excep_csr`: *tval written with rs1 value
- `norm:cbp_unperm_noexcep`: prefetch does not raise exceptions

**Test Scope**: Verify CMO instruction exception behavior under G-stage address translation when V=1.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HCGSTAGE-01 | cbo.clean G-stage no permission triggers guest-page-fault | VS-mode, G-stage page table no permission, execute cbo.clean | store guest-page-fault (cause=21) |
| HCGSTAGE-02 | cbo.flush G-stage no permission triggers guest-page-fault | VS-mode, G-stage page table no permission, execute cbo.flush | store guest-page-fault (cause=21) |
| HCGSTAGE-03 | cbo.inval G-stage no permission triggers guest-page-fault | VS-mode, G-stage page table no permission, execute cbo.inval | store guest-page-fault (cause=21) |
| HCGSTAGE-04 | cbo.zero G-stage no write permission triggers guest-page-fault | VS-mode, G-stage page table read-only, execute cbo.zero | store guest-page-fault (cause=21) |
| HCGSTAGE-05 | cbo.clean G-stage guest-page-fault stval=rs1 | Trigger guest-page-fault, check stval | stval = value of rs1 |
| HCGSTAGE-06 | cbo.zero G-stage guest-page-fault stval=rs1 | Trigger guest-page-fault, check stval | stval = value of rs1 |
| HCGSTAGE-07 | cbo.zero G-stage guest-page-fault htval | Trigger guest-page-fault, check htval | htval = faulting GPA >> 2 |
| HCGSTAGE-08 | cbo.clean with read-only permission executes (G-stage) | G-stage page table read-only, execute cbo.clean | Normal execution (management only requires any access permitted) |
| HCGSTAGE-09 | cbo.inval with read-only permission executes (G-stage) | G-stage page table read-only, execute cbo.inval | Normal execution |
| HCGSTAGE-10 | cbo.zero with read-only permission triggers guest-page-fault | G-stage page table read-only, execute cbo.zero | store guest-page-fault (cbo.zero requires write permission) |
| HCGSTAGE-11 | prefetch G-stage no permission does not trigger exception | VS-mode, G-stage page table no permission, execute prefetch.r | No exception triggered |
| HCGSTAGE-12 | cbo.clean VS-stage page-fault (not G-stage) | VS-mode, VS-stage page table no permission (G-stage has permission), execute cbo.clean | store page-fault (cause=15), not guest-page-fault |

---

## Group 6. CMO virtual-instruction Exception stval Behavior

**Spec Reference**:
- Hypervisor SPEC: stval write rules for virtual-instruction exception are the same as illegal-instruction
- `norm:fault_excep_csr`: *tval written with faulting address on exception

**Test Scope**: Verify stval write behavior when CMO instructions trigger virtual-instruction exceptions.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HVINST-01 | cbo.inval virtual-instruction stval | henvcfg.CBIE=00, VS-mode executes cbo.inval | stval written per illegal-instruction rules (instruction encoding or 0) |
| HVINST-02 | cbo.clean virtual-instruction stval | henvcfg.CBCFE=0, VS-mode executes cbo.clean | stval written per illegal-instruction rules |
| HVINST-03 | cbo.flush virtual-instruction stval | henvcfg.CBCFE=0, VS-mode executes cbo.flush | stval written per illegal-instruction rules |
| HVINST-04 | cbo.zero virtual-instruction stval | henvcfg.CBZE=0, VS-mode executes cbo.zero | stval written per illegal-instruction rules |
| HVINST-05 | VU-mode cbo.inval virtual-instruction stval | henvcfg.CBIE=00, VU-mode executes cbo.inval | stval written per illegal-instruction rules |

---

## Group 7. Zicbop Prefetch VS/VU-mode Behavior

**Spec Reference**:
- `norm:cbp_unperm_noexcep`: prefetch does not raise any exceptions
- CMO SPEC: prefetch does not raise illegal-instruction or virtual-instruction

**Test Scope**: Verify that prefetch instructions in VS/VU-mode are not controlled by henvcfg CMO fields and do not trigger virtual-instruction.

| Test ID | Test Name | Test Description | Expected Result |
|---------|-----------|------------------|-----------------|
| HPREFETCH-01 | VS-mode prefetch.r does not trigger virtual-instruction | henvcfg.CBIE=00, CBCFE=0, CBZE=0, VS-mode executes prefetch.r | Normal execution, no exception |
| HPREFETCH-02 | VS-mode prefetch.w does not trigger virtual-instruction | Same configuration, VS-mode executes prefetch.w | Normal execution, no exception |
| HPREFETCH-03 | VS-mode prefetch.i does not trigger virtual-instruction | Same configuration, VS-mode executes prefetch.i | Normal execution, no exception |
| HPREFETCH-04 | VU-mode prefetch.r does not trigger virtual-instruction | henvcfg all CBO fields=0, senvcfg all CBO fields=0, VU-mode executes prefetch.r | Normal execution, no exception |
| HPREFETCH-05 | VU-mode prefetch.w does not trigger virtual-instruction | Same configuration, VU-mode executes prefetch.w | Normal execution, no exception |
| HPREFETCH-06 | VU-mode prefetch.i does not trigger virtual-instruction | Same configuration, VU-mode executes prefetch.i | Normal execution, no exception |
| HPREFETCH-07 | VS-mode prefetch G-stage no permission does not trigger exception | G-stage page table no permission, VS-mode executes prefetch.r | No exception triggered |
| HPREFETCH-08 | VS-mode prefetch does not check A/D bits | VS-stage page table A=0 D=0, VS-mode executes prefetch.w | No exception triggered, A/D bits not set |
