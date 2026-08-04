**中文 | [English](../testplan_en/Hypervisor_Exceptions_test_plan_en.md)**

# Hypervisor 异常与 Trap 测试计划（Hypervisor_Exceptions 子集）

> 本文档拆分自已删除的原 Hypervisor 综合测试计划（Hypervisor_test_plan.md），测试用例编号保持不变，Group 序号为本子集内重新排列。
> 兄弟子集：[Hypervisor_CSR_test_plan.md](Hypervisor_CSR_test_plan.md) | [Hypervisor_Interrupts_test_plan.md](Hypervisor_Interrupts_test_plan.md)

## 概述

本测试计划覆盖 RISC-V Hypervisor (H) 扩展的异常与 trap 相关功能点，包括 virtual-instruction exception 全场景、trap entry/return 行为、htinst/mtinst 转换指令、mstatus Hypervisor 增强（MPV/GVA/TVM/MPRV）、mtval2/mtinst 寄存器、异常优先级以及 hedeleg 异常委托链路。CSR 寄存器字段行为、中断递送机制分别由兄弟子集覆盖。

本测试计划依据 `SPEC/hypervisor.adoc` 中的规范点（norm 标记）编写。

### 本文档覆盖的 SPEC 章节
- Hypervisor and Virtual Supervisor CSRs（hedeleg 异常委托行为）
- Machine-Level CSR 增强（mstatus MPV/GVA/TVM/MPRV, mtval2, mtinst）
- Hypervisor Instructions（HLV/HLVX/HSV 异常场景, HFENCE.VVMA/HFENCE.GVMA 异常场景）
- Traps（virtual-instruction exception 全场景, trap entry/return, 异常优先级, htinst/mtinst 转换指令）

### 由其他测试计划覆盖
- Hypervisor CSR（CSR 替代机制, hstatus, henvcfg, htimedelta, VS CSR, hedeleg/hideleg 位域属性） → `Hypervisor_CSR_test_plan.md`
- Hypervisor 中断（hvip/hip/hie, hgeip/hgeie, mideleg/mip/mie 增强, hideleg 中断委托） → `Hypervisor_Interrupts_test_plan.md`
- G-stage 地址翻译 → `hyp_gstage_translation_test_plan.md`
- 两阶段地址翻译 → `hyp_2_stage_translation_test_plan.md`
- Sha 组合扩展 → `sha_test_plan.md`
- Shcounterenw → `shcounterenw_test_plan.md`
- Shgatpa → `shgatpa_test_plan.md`
- Shvsatpa → `shvsatpa_test_plan.md`
- Shtvala → `shtvala_test_plan.md`
- Shvstvala → `shvstvala_test_plan.md`
- Shvstvecd → `shvstvecd_test_plan.md`

---

## 覆盖的规范点

本章节列出本文档 Groups 1-8 所有测试组中引用的规范点（norm ID），已去重并按字母顺序排列。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:H_cause_ecall` | HS-mode and VS-mode ECALLs use different cause values so they can be delegated separately. | HS 模式和 VS 模式的 ECALL 使用不同原因值以便分别委托。 |
| `norm:H_cause_virtual_instruction` | When V=1, a virtual-instruction exception (code 22) is normally raised instead of an illegal-instruction exception if the attempted instruction is HS-qualified but is prevented from executing when V=1. An instruction is HS-qualified if it would be valid to execute in HS-mode, assuming TSR and TVM of `mstatus` are both zero. | V=1 时，若尝试的 HS 限定指令被阻止执行，通常引发虚拟指令异常（代码 22）而非非法指令异常。指令在假设 `mstatus`.TSR 和 TVM 均为零时能在 HS 模式有效执行则为 HS 限定。 |
| `norm:H_cause_virtual_instruction_high` | When V=1 and XLEN=32, an invalid attempt to access a high-half CSR raises a virtual-instruction exception instead of an illegal-instruction exception if the same CSR instruction for the corresponding low-half CSR is HS-qualified. | V=1 且 XLEN=32 时，无效的高半 CSR 访问若对应低半 CSR 指令为 HS 限定，则引发虚拟指令异常而非非法指令异常。 |
| `norm:H_exception_priority` | If an instruction may raise multiple synchronous exceptions, the decreasing priority order indicates which exception is taken and reported in `mcause` or `scause`. | 若指令可能引发多个同步异常，按优先级递减顺序决定哪个异常被接收并报告在 `mcause` 或 `scause` 中。 |
| `norm:H_illegalinst_xstatus_fs_vs` | Fields FS and VS in registers `sstatus` and `vsstatus` deviate from the usual HS-qualified rule. If an instruction is prevented from executing because FS or VS is zero in either `sstatus` or `vsstatus`, the exception raised is always an illegal-instruction exception, never a virtual-instruction exception. | `sstatus` 和 `vsstatus` 中的 FS 和 VS 字段偏离通常的 HS 限定规则。若指令因 FS 或 VS 为零而被阻止，始终引发非法指令异常，不引发虚拟指令异常。 |
| `norm:H_trap_deleg` | When a trap occurs in HS-mode or U-mode, it goes to M-mode, unless delegated by `medeleg` or `mideleg`, in which case it goes to HS-mode. When a trap occurs in VS-mode or VU-mode, it goes to M-mode, unless delegated by `medeleg`/`mideleg` to HS-mode, unless further delegated by `hedeleg`/`hideleg` to VS-mode. | HS/U 模式陷阱进入 M 模式，除非通过 `medeleg`/`mideleg` 委托给 HS 模式。VS/VU 模式陷阱进入 M 模式，除非委托给 HS 模式，除非进一步委托给 VS 模式。 |
| `norm:H_trap_hs_csrwrites` | When a trap is taken into HS-mode, V is set to 0, and `hstatus`.SPV and `sstatus`.SPP are set accordingly. If V was 1 before the trap, SPVP is set the same as `sstatus`.SPP; otherwise, SPVP is left unchanged. A trap into HS-mode also writes GVA in `hstatus`, SPIE and SIE in `sstatus`, and CSRs `sepc`, `scause`, `stval`, `htval`, and `htinst`. | 陷阱进入 HS 模式时，V 设为 0，`hstatus`.SPV 和 `sstatus`.SPP 相应设置。陷阱前 V=1 时 SPVP 与 SPP 相同；否则不变。同时写入相关字段和 CSR。 |
| `norm:H_trap_m_csrwrites` | When a trap is taken into M-mode, V gets set to 0, and fields MPV and MPP in `mstatus` are set accordingly. A trap into M-mode also writes fields GVA, MPIE, and MIE in `mstatus` and writes CSRs `mepc`, `mcause`, `mtval`, `mtval2`, and `mtinst`. | 陷阱进入 M 模式时，V 设为 0，`mstatus` 的 MPV 和 MPP 相应设置。同时写入 GVA、MPIE、MIE 及 CSR `mepc`、`mcause`、`mtval`、`mtval2`、`mtinst`。 |
| `norm:H_trap_vs_csrwrites` | When a trap is taken into VS-mode, `vsstatus`.SPP is set accordingly. Register `hstatus` and the HS-level `sstatus` are not modified, and V remains 1. A trap into VS-mode also writes SPIE and SIE in `vsstatus` and writes CSRs `vsepc`, `vscause`, and `vstval`. | 陷阱进入 VS 模式时，`vsstatus`.SPP 相应设置。`hstatus` 和 HS 级 `sstatus` 不修改，V 保持 1。同时写入 `vsstatus` 的 SPIE/SIE 及 CSR `vsepc`、`vscause`、`vstval`。 |
| `norm:H_trap_xtinst` | On any trap into M-mode or HS-mode, one of these values is written to `mtinst` or `htinst`: zero; a transformation of the trapping instruction; a custom value (only if the trapping instruction is non-standard); or a special pseudoinstruction. | 任何陷阱进入 M/HS 模式时，`mtinst`/`htinst` 写入以下之一：零；陷阱指令的转换；自定义值（仅限非标准指令）；或特殊伪指令。 |
| `norm:H_trap_xtinst_exception` | On a synchronous exception, if a nonzero value is written to the trap instruction register, it must be one of: a standard transformed instruction (bit 0 = 1, replacing bit 1 with 1 yields a valid standard encoding); a custom value (bit 0 = 1, replacing bit 1 with 1 yields a designated custom encoding); or a special pseudoinstruction (bits 1:0 = 00). All other values (e.g. bits 1:0 = 10) are illegal. | 同步异常时，若陷阱指令寄存器写入非零值，必须是以下之一：标准转换指令（bit0=1，将 bit1 置 1 后为标准指令编码）；custom 值（bit0=1，bit1 置 1 后为指定的 custom 编码）；特殊伪指令（bits1:0=00）。其他值（如 bits1:0=10）非法。 |
| `norm:H_trap_xtinst_guestpage` | For guest-page faults, the trap instruction register is written with a special pseudoinstruction value if: (a) the fault is caused by an implicit memory access for VS-stage address translation, and (b) a nonzero value is written to `mtval2` or `htval`. If both conditions are met, zero is not allowed. | 对于客户页错误，若 (a) 故障由 VS 阶段地址翻译的隐式内存访问引起，且 (b) `mtval2`/`htval` 写入非零值，则陷阱指令寄存器必须写入特殊伪指令值，不允许零。 |
| `norm:H_trap_xtinst_guestpage_rw` | A write pseudoinstruction (0x00002020 or 0x00003020) is used for the case that the machine is attempting automatically to update bits A and/or D in VS-level page tables. All other implicit memory accesses for VS-stage address translation will be reads. | 写伪指令（0x00002020 或 0x00003020）用于机器自动更新 VS 级页表 A/D 位的情况。所有其他 VS 阶段翻译的隐式内存访问为读取。 |
| `norm:H_trap_xtinst_interrupt` | On an interrupt, the value written to the trap instruction register is always zero. | 中断时，陷阱指令寄存器写入值始终为零。 |
| `norm:H_trap_xtinst_val` | The values that may be automatically written to the trap instruction register for each standard exception cause are enumerated in the specification table. | 每种标准异常原因可自动写入陷阱指令寄存器的值在规范表中列举。 |
| `norm:H_virtinst_vs_sfence_sinval_satp_vtvm1` | In VS-mode, attempts to execute an SFENCE.VMA or SINVAL.VMA instruction or to access `satp`, when `hstatus`.VTVM=1. | VS 模式下，`hstatus`.VTVM=1 时尝试执行 SFENCE.VMA、SINVAL.VMA 或访问 `satp`。 |
| `norm:H_virtinst_vs_sret_vtsr1` | In VS-mode, attempts to execute SRET when `hstatus`.VTSR=1. | VS 模式下，`hstatus`.VTSR=1 时尝试执行 SRET。 |
| `norm:H_virtinst_vu_nonhigh_supervisor_allowedhs_tvm0` | In VU-mode, attempts to access an implemented non-high-half supervisor CSR when the same access would be allowed in HS-mode, assuming `mstatus`.TVM=0. | VU 模式下，访问已实现的非高半 supervisor CSR，且假设 TVM=0 时该访问在 HS 模式下允许。 |
| `norm:H_virtinst_vu_sret_sfence` | In VU-mode, attempts to execute a supervisor instruction (SRET or SFENCE). | VU 模式下，尝试执行 supervisor 指令（SRET 或 SFENCE）。 |
| `norm:H_virtinst_vu_vs_hinst` | In VS-mode or VU-mode, attempts to execute a hypervisor instruction (HLV, HLVX, HSV, or HFENCE). | VS 或 VU 模式下，尝试执行 hypervisor 指令（HLV、HLVX、HSV 或 HFENCE）。 |
| `norm:H_virtinst_vu_vs_nonhigh_allowedhs_tvm0` | In VS-mode or VU-mode, attempts to access an implemented non-high-half hypervisor CSR or VS CSR when the same access would be allowed in HS-mode, assuming `mstatus`.TVM=0. | VS 或 VU 模式下，访问已实现的非高半 hypervisor/VS CSR，且假设 `mstatus`.TVM=0 时该访问在 HS 模式下允许。 |
| `norm:H_virtinst_vu_wfi_tw0` | In VU-mode, attempts to execute WFI when `mstatus`.TW=0. | VU 模式下，`mstatus`.TW=0 时尝试执行 WFI。 |
| `norm:H_virtinst_wfi_vtw1_tw0` | In VS-mode, attempts to execute WFI when `hstatus`.VTW=1 and `mstatus`.TW=0, unless the instruction completes within an implementation-specific, bounded time. | VS 模式下，`hstatus`.VTW=1 且 `mstatus`.TW=0 时尝试执行 WFI，除非指令在实现特定的有限时间内完成。 |
| `norm:H_virtinst_xtval` | On a virtual-instruction trap, `mtval` or `stval` is written the same as for an illegal-instruction trap. | 虚拟指令陷阱时，`mtval` 或 `stval` 的写入方式与非法指令陷阱相同。 |
| `norm:hedeleg_acc` | Each bit of `hedeleg` shall be either writable or read-only zero. Many bits of `hedeleg` are required specifically to be writable or zero, as enumerated in the table. Bit 0, corresponding to instruction address-misaligned exceptions, must be writable if IALIGN=32. | `hedeleg` 的每一位要么可写要么为只读零。第 0 位（指令地址未对齐异常）在 IALIGN=32 时必须可写。 |
| `norm:hedeleg_op` | A synchronous trap that has been delegated to HS-mode (using `medeleg`) is further delegated to VS-mode if V=1 before the trap and the corresponding `hedeleg` bit is set. | 已通过 `medeleg` 委托给 HS 模式的同步陷阱，若陷阱前 V=1 且对应的 `hedeleg` 位已设置，则进一步委托给 VS 模式。 |
| `norm:mret_h` | MRET first determines the new privilege mode according to MPP and MPV in `mstatus`. MRET then sets MPV=0, MPP=0, MIE=MPIE, and MPIE=1. Lastly, MRET sets the privilege mode as previously determined, and sets pc=mepc. | MRET 先根据 `mstatus` 中 MPP 和 MPV 确定新特权模式，然后设 MPV=0、MPP=0、MIE=MPIE、MPIE=1，最后设置特权模式并 pc=mepc。 |
| `norm:mstatus_gva_op` | Field GVA is written by the implementation whenever a trap is taken into M-mode. For any trap that writes a guest virtual address to `mtval`, GVA is set to 1. For any other trap into M-mode, GVA is set to 0. | GVA 字段在陷阱进入 M 模式时由实现写入。写入客户虚拟地址到 `mtval` 的陷阱设 GVA=1，其他设 GVA=0。 |
| `norm:mstatus_modes` | The TSR and TVM fields of `mstatus` affect execution only in HS-mode, not in VS-mode. The TW field affects execution in all modes except M-mode. | `mstatus` 的 TSR 和 TVM 字段仅影响 HS 模式执行，不影响 VS 模式。TW 字段影响除 M 模式外的所有模式。 |
| `norm:mstatus_mprv_hlsv` | MPRV does not affect the virtual-machine load/store instructions, HLV, HLVX, and HSV. The explicit loads and stores of these instructions always act as though V=1 and the nominal privilege mode were `hstatus`.SPVP, overriding MPRV. | MPRV 不影响虚拟机 load/store 指令。这些指令的显式访问始终如同 V=1 且名义特权模式为 `hstatus`.SPVP，覆盖 MPRV。 |
| `norm:mstatus_mprv_hypervisor` | The hypervisor extension changes the behavior of MPRV. When MPRV=0, normal translation. When MPRV=1, explicit memory accesses are translated and protected as though the current virtualization mode were set to MPV and the current nominal privilege mode were set to MPP. | hypervisor 扩展改变了 MPRV 的行为。MPRV=0 时正常翻译。MPRV=1 时显式内存访问如同当前虚拟化模式为 MPV、名义特权模式为 MPP 般翻译和保护。 |
| `norm:mstatus_mpv_op` | The MPV bit is written by the implementation whenever a trap is taken into M-mode, set to the value of V at the time of the trap. When an MRET instruction is executed, V is set to MPV, unless MPP=3, in which case V remains 0. | MPV 位在陷阱进入 M 模式时由实现写入，设为陷阱时 V 的值。执行 MRET 时 V 设为 MPV，除非 MPP=3 时 V 保持 0。 |
| `norm:mstatus_tvm_hs` | Setting TVM=1 prevents HS-mode from accessing `hgatp` or executing HFENCE.GVMA or HINVAL.GVMA, but has no effect on accesses to `vsatp` or instructions HFENCE.VVMA or HINVAL.VVMA. | TVM=1 阻止 HS 模式访问 `hgatp` 或执行 HFENCE.GVMA/HINVAL.GVMA，但不影响 `vsatp` 访问或 HFENCE.VVMA/HINVAL.VVMA 指令。 |
| `norm:mtinst_sz_acc_op` | The `mtinst` register is an MXLEN-bit read/write register. When a trap is taken into M-mode, `mtinst` is written with a value that, if nonzero, provides information about the instruction that trapped. | `mtinst` 是一个 MXLEN 位读写寄存器。陷阱进入 M 模式时写入关于陷阱指令的信息。 |
| `norm:mtinst_val` | `mtinst` is a WARL register that need only be able to hold the values that the implementation may automatically write to it on a trap. | `mtinst` 是 WARL 寄存器，仅需能保持实现在陷阱时可能自动写入的值。 |
| `norm:mtval2_sz_acc_op` | The `mtval2` register is an MXLEN-bit read/write register. When a trap is taken into M-mode, `mtval2` is written with additional exception-specific information, alongside `mtval`. | `mtval2` 是一个 MXLEN 位读写寄存器。陷阱进入 M 模式时写入额外异常特定信息。 |
| `norm:mtval2_trapval` | When a guest-page-fault trap is taken into M-mode, `mtval2` is written with either zero or the guest physical address that faulted, shifted right by 2 bits. For other traps, `mtval2` is set to zero. | 客户页错误陷阱进入 M 模式时，`mtval2` 写入零或故障客户物理地址右移 2 位。其他陷阱设为零。 |
| `norm:mtval2_trapval_vstrans` | If a guest-page fault is due to an implicit memory access during first-stage (VS-stage) address translation, a guest physical address written to `mtval2` is that of the implicit memory access that faulted. | 若客户页错误由 VS 阶段地址翻译的隐式内存访问引起，写入 `mtval2` 的地址是故障的隐式内存访问地址。 |
| `norm:mtval2_val` | `mtval2` is a WARL register that must be able to hold zero and may be capable of holding only an arbitrary subset of other 2-bit-shifted guest physical addresses, if any. | `mtval2` 是 WARL 寄存器，必须能保持零，且可能只能保持 2 位右移客户物理地址的任意子集。写入任意值后不要求回显原值，但读回值必须稳定。 |
| `norm:sret_dt` | If the Ssdbltrp extension is implemented, when SRET is executed in HS-mode, if the new privilege mode is VU, the SRET instruction sets `vsstatus`.SDT to 0. When executed in VS-mode, `vsstatus`.SDT is set to 0. | 若实现了 Ssdbltrp 扩展，HS 模式执行 SRET 且新特权模式为 VU 时，设 `vsstatus`.SDT=0。VS 模式执行时也设 `vsstatus`.SDT=0。 |
| `norm:sret_v0` | When executed in M-mode or HS-mode (V=0), SRET first determines the new privilege mode according to `hstatus`.SPV and `sstatus`.SPP. SRET then sets `hstatus`.SPV=0, and in `sstatus` sets SPP=0, SIE=SPIE, and SPIE=1. Lastly, SRET sets the privilege mode and sets pc=sepc. | 在 M/HS 模式（V=0）执行时，SRET 根据 `hstatus`.SPV 和 `sstatus`.SPP 确定新特权模式，然后设 SPV=0、SPP=0、SIE=SPIE、SPIE=1，最后设置特权模式并 pc=sepc。 |
| `norm:sret_v1` | When executed in VS-mode (V=1), SRET sets the privilege mode accordingly, in `vsstatus` sets SPP=0, SIE=SPIE, and SPIE=1, and lastly sets pc=vsepc. | 在 VS 模式（V=1）执行时，SRET 相应设置特权模式，在 `vsstatus` 中设 SPP=0、SIE=SPIE、SPIE=1，最后 pc=vsepc。 |

---

## Group 1. Virtual-Instruction Exception

**规范依据**：
- `norm:H_cause_virtual_instruction`：HS-qualified 指令在 V=1 时因权限不足或被禁用 → virtual-instruction exception (cause=22)
- `norm:H_cause_virtual_instruction_high`：V=1 且 XLEN=32 时高半 CSR 的特殊规则
- `norm:H_virtinst_vu_vs_hinst`：VS/VU-mode 执行 HLV/HLVX/HSV/HFENCE
- `norm:H_virtinst_vu_vs_nonhigh_allowedhs_tvm0`：VS/VU-mode 访问 H CSR / VS CSR
- `norm:H_virtinst_vu_wfi_tw0` / `norm:H_virtinst_vu_sret_sfence`：VU-mode 执行 WFI/SRET/SFENCE
- `norm:H_virtinst_vu_nonhigh_supervisor_allowedhs_tvm0`：VU-mode 访问 S CSR
- `norm:H_virtinst_wfi_vtw1_tw0`：VS-mode WFI + VTW=1 + TW=0
- `norm:H_virtinst_vs_sret_vtsr1`：VS-mode SRET + VTSR=1
- `norm:H_virtinst_vs_sfence_sinval_satp_vtvm1`：VS-mode SFENCE/SINVAL.VMA/satp + VTVM=1
- `norm:H_virtinst_xtval`：virtual-instruction trap 时 stval/mtval 与 illegal-instruction 相同
- `norm:H_illegalinst_xstatus_fs_vs`：FS/VS=0 时始终 illegal-instruction（非 virtual-instruction）

**测试职责**：系统覆盖所有触发 virtual-instruction exception 的场景，并验证与 illegal-instruction 的区分。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VINST-01 | VS-mode 执行 HLV | VS-mode 执行 HLV.W | virtual-instruction exception (cause=22) |
| VINST-02 | VS-mode 执行 HSV | VS-mode 执行 HSV.W | virtual-instruction exception (cause=22) |
| VINST-03 | VS-mode 执行 HLVX | VS-mode 执行 HLVX.WU | virtual-instruction exception (cause=22) |
| VINST-04 | VS-mode 执行 HFENCE.VVMA | VS-mode 执行 HFENCE.VVMA | virtual-instruction exception (cause=22) |
| VINST-05 | VS-mode 执行 HFENCE.GVMA | VS-mode 执行 HFENCE.GVMA | virtual-instruction exception (cause=22) |
| VINST-06 | VU-mode 执行 HLV | VU-mode 执行 HLV.W | virtual-instruction exception (cause=22) |
| VINST-07 | VS-mode 访问 hstatus | VS-mode csrr hstatus | virtual-instruction exception (cause=22) |
| VINST-08 | VS-mode 访问 hedeleg | VS-mode csrr hedeleg | virtual-instruction exception (cause=22) |
| VINST-09 | VS-mode 访问 hgatp | VS-mode csrr hgatp | virtual-instruction exception (cause=22) |
| VINST-10 | VS-mode 访问 vsstatus（直接地址） | VS-mode 用 vsstatus 的 CSR 地址（0x200）直接访问 | virtual-instruction exception (cause=22) |
| VINST-11 | VU-mode 执行 WFI（TW=0） | mstatus.TW=0，VU-mode 执行 WFI | virtual-instruction exception (cause=22) |
| VINST-12 | VU-mode 执行 SRET | VU-mode 执行 SRET | virtual-instruction exception (cause=22) |
| VINST-13 | VU-mode 执行 SFENCE.VMA | VU-mode 执行 SFENCE.VMA | virtual-instruction exception (cause=22) |
| VINST-14 | VU-mode 访问 sstatus | VU-mode csrr sstatus | virtual-instruction exception (cause=22) |
| VINST-15 | VU-mode 访问 scause | VU-mode csrr scause | virtual-instruction exception (cause=22) |
| VINST-16 | VS-mode WFI + VTW=1 + TW=0 | hstatus.VTW=1, mstatus.TW=0, VS-mode WFI | virtual-instruction exception (cause=22) |
| VINST-17 | VS-mode SRET + VTSR=1 | hstatus.VTSR=1, VS-mode SRET | virtual-instruction exception (cause=22) |
| VINST-18 | VS-mode SFENCE.VMA + VTVM=1 | hstatus.VTVM=1, VS-mode SFENCE.VMA | virtual-instruction exception (cause=22) |
| VINST-19 | VS-mode 访问 satp + VTVM=1 | hstatus.VTVM=1, VS-mode csrr satp | virtual-instruction exception (cause=22) |
| VINST-20 | VS-mode SINVAL.VMA + VTVM=1 | hstatus.VTVM=1, VS-mode SINVAL.VMA | virtual-instruction exception (cause=22) |
| VINST-21 | FS=0 时是 illegal 而非 virtual | V=1 时 sstatus.FS=0 或 vsstatus.FS=0，VS-mode 执行 FP | illegal-instruction exception (cause=2)，非 cause=22 |
| VINST-22 | VS=0 时是 illegal 而非 virtual | V=1 时 sstatus.VS=0 或 vsstatus.VS=0，VS-mode 执行 Vector | illegal-instruction exception (cause=2)，非 cause=22 |
| VINST-23 | virtual-instruction trap 时 stval 正确 | VS-mode 触发 virtual-instruction exception | stval 与 illegal-instruction trap 写法相同 |
| VINST-24 | mstatus.TW=1 覆盖 VTW（illegal 而非 virtual） | mstatus.TW=1, VS-mode WFI | illegal-instruction exception (cause=2) |
| VINST-25 | VS-mode 访问 hideleg | VS-mode csrr hideleg | virtual-instruction exception (cause=22) |
| VINST-26 | VS-mode 访问 hcounteren | VS-mode csrr hcounteren | virtual-instruction exception (cause=22) |
| VINST-27 | VS-mode 访问 htimedelta | VS-mode csrr htimedelta | virtual-instruction exception (cause=22) |
| VINST-28 | VS-mode 访问 hip | VS-mode csrr hip | virtual-instruction exception (cause=22) |
| VINST-29 | VS-mode 访问 hie | VS-mode csrr hie | virtual-instruction exception (cause=22) |
| VINST-30 | VS-mode 访问 hvip | VS-mode csrr hvip | virtual-instruction exception (cause=22) |
| VINST-31 | VS-mode 访问 henvcfg | VS-mode csrr henvcfg | virtual-instruction exception (cause=22) |
| VINST-32 | VS-mode 写 hstatus | VS-mode csrw hstatus | virtual-instruction exception (cause=22) |
| VINST-33 | VU-mode 访问 sie | VU-mode csrr sie | virtual-instruction exception (cause=22) |
| VINST-34 | VU-mode 访问 sip | VU-mode csrr sip | virtual-instruction exception (cause=22) |
| VINST-35 | VU-mode 访问 stvec | VU-mode csrr stvec | virtual-instruction exception (cause=22) |
| VINST-36 | VU-mode 访问 sepc | VU-mode csrr sepc | virtual-instruction exception (cause=22) |
| VINST-37 | VS-mode 写 satp + VTVM=1 | hstatus.VTVM=1, VS-mode csrw satp | virtual-instruction exception (cause=22) |
| VINST-38 | VS-mode 执行 HLV.B | VS-mode 执行 HLV.B | virtual-instruction exception (cause=22) |
| VINST-39 | VS-mode 执行 HLV.H | VS-mode 执行 HLV.H | virtual-instruction exception (cause=22) |
| VINST-40 | VS-mode 执行 HLV.D | VS-mode 执行 HLV.D | virtual-instruction exception (cause=22) |
| VINST-41 | VS-mode 执行 HSV.B | VS-mode 执行 HSV.B | virtual-instruction exception (cause=22) |
| VINST-42 | VS-mode 执行 HSV.H | VS-mode 执行 HSV.H | virtual-instruction exception (cause=22) |
| VINST-43 | VS-mode 执行 HSV.D | VS-mode 执行 HSV.D | virtual-instruction exception (cause=22) |
| VINST-44 | mstatus.TSR=1 不影响 VS-mode SRET | mstatus.TSR=1, hstatus.VTSR=1, VS-mode SRET。TSR 仅影响 HS-mode（norm:mstatus_modes），VS-mode SRET 仅受 VTSR 控制 | virtual-instruction exception (cause=22) |

---

## Group 2. Trap Entry 行为

**规范依据**：
- `norm:H_trap_deleg`：委托链 M→HS→VS
- `norm:H_trap_m_csrwrites`：trap 到 M-mode 时：V→0, MPV/MPP←当前模式, GVA/MPIE/MIE 更新, mepc/mcause/mtval/mtval2/mtinst 写入
- `norm:H_trap_hs_csrwrites`：trap 到 HS-mode 时：V→0, SPV/SPP←当前模式, SPVP(仅 V=1 时更新), GVA/SPIE/SIE 更新, sepc/scause/stval/htval/htinst 写入
- `norm:H_trap_vs_csrwrites`：trap 到 VS-mode 时：V 保持 1, vsstatus.SPP 更新, SPIE/SIE 更新, vsepc/vscause/vstval 写入

**测试职责**：验证 trap entry 时各 CSR 的自动写入行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TENT-01 | VS-mode trap 到 HS-mode 的 CSR 写入 | VS-mode ecall，trap 到 HS-mode | V=0, SPV=1, SPP=1, sepc=fault PC, scause=10 |
| TENT-02 | VU-mode trap 到 HS-mode 的 CSR 写入 | VU-mode ecall，trap 到 HS-mode | V=0, SPV=1, SPP=0, sepc=fault PC, scause=8 |
| TENT-03 | VS-mode trap 到 HS-mode SPVP 更新 | VS-mode trap 到 HS-mode | hstatus.SPVP=1（VS-mode 是 S 级） |
| TENT-04 | VU-mode trap 到 HS-mode SPVP 更新 | VU-mode trap 到 HS-mode | hstatus.SPVP=0（VU-mode 是 U 级） |
| TENT-05 | U-mode trap 到 HS-mode SPVP 不变 | 先设 SPVP=1，U-mode trap 到 HS-mode | hstatus.SPVP 保持 1（V=0 时不更新） |
| TENT-06 | HS-mode trap 到 HS-mode | HS-mode ecall 未被委托 | SPV=0, SPP=1 |
| TENT-07 | trap 到 HS-mode GVA 正确 | VS-mode guest-page-fault trap 到 HS-mode | hstatus.GVA=1 |
| TENT-08 | trap 到 HS-mode GVA=0 | VS-mode ecall trap 到 HS-mode | hstatus.GVA=0 |
| TENT-09 | trap 到 HS-mode SIE/SPIE 正确 | 记录 sstatus.SIE 旧值，触发 trap | SPIE=旧 SIE, SIE=0 |
| TENT-10 | trap 到 VS-mode 的 CSR 写入 | 委托到 VS-mode 的异常 | vsstatus.SPP 正确, vsepc/vscause/vstval 写入 |
| TENT-11 | trap 到 VS-mode 不修改 hstatus | 委托到 VS-mode 的异常 | hstatus 不变, V 保持 1 |
| TENT-12 | VS-mode trap 到 M-mode 的 CSR 写入 | VS-mode 异常未被 medeleg 委托 | MPV=1, MPP=1, mtval2/mtinst 写入 |
| TENT-13 | VU-mode trap 到 M-mode | VU-mode 异常未被 medeleg 委托 | MPV=1, MPP=0 |
| TENT-14 | HS-mode trap 到 M-mode | HS-mode 异常 | MPV=0, MPP=1 |
| TENT-15 | htval/htinst 在非 guest-page-fault 时为零 | VS-mode ecall trap 到 HS-mode | htval=0, htinst=0 |

---

## Group 3. Trap Return 行为

**规范依据**：
- `norm:mret_h`：MRET 根据 MPP/MPV 确定新特权级，然后 MPV=0, MPP=0, MIE=MPIE, MPIE=1
- `norm:sret_v0`：V=0 时 SRET 根据 SPV/SPP 确定新模式，SPV=0, SPP=0, SIE=SPIE, SPIE=1
- `norm:sret_v1`：V=1 时 SRET 根据 vsstatus.SPP 确定模式，SPP=0, SIE=SPIE, SPIE=1
- `norm:sret_dt`：Ssdbltrp 时 SRET 在 HS-mode 且新模式为 VU 时清 vsstatus.SDT；VS-mode 时清 vsstatus.SDT

**测试职责**：验证 MRET/SRET 在 H 扩展下的模式切换和 CSR 恢复行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TRET-01 | MRET 返回到 VS-mode | 设 MPV=1, MPP=1, 执行 MRET | 进入 VS-mode (V=1), MPV=0, MPP=0 |
| TRET-02 | MRET 返回到 VU-mode | 设 MPV=1, MPP=0, 执行 MRET | 进入 VU-mode (V=1), MPV=0, MPP=0 |
| TRET-03 | MRET 返回到 HS-mode | 设 MPV=0, MPP=1, 执行 MRET | 进入 HS-mode (V=0) |
| TRET-04 | MRET 返回到 M-mode | 设 MPP=3, 执行 MRET | 进入 M-mode, V 保持 0 |
| TRET-05 | MRET MIE/MPIE 恢复 | 设 MPIE=1，执行 MRET | MIE=1, MPIE=1 |
| TRET-06 | SRET(V=0) 返回到 VS-mode | 设 hstatus.SPV=1, sstatus.SPP=1, 执行 SRET | 进入 VS-mode (V=1), SPV=0, SPP=0 |
| TRET-07 | SRET(V=0) 返回到 VU-mode | 设 hstatus.SPV=1, sstatus.SPP=0, 执行 SRET | 进入 VU-mode (V=1), SPV=0, SPP=0 |
| TRET-08 | SRET(V=0) 返回到 HS-mode | 设 hstatus.SPV=0, sstatus.SPP=1, 执行 SRET | 进入 HS-mode (V=0) |
| TRET-09 | SRET(V=0) 返回到 U-mode | 设 hstatus.SPV=0, sstatus.SPP=0, 执行 SRET | 进入 U-mode (V=0) |
| TRET-10 | SRET(V=0) SIE/SPIE 恢复 | 设 sstatus.SPIE=1, 执行 SRET | SIE=1, SPIE=1 |
| TRET-11 | SRET(V=1) 返回到 VS-mode | VS-mode 中 vsstatus.SPP=1, 执行 SRET | 返回 VS-mode, SPP=0 |
| TRET-12 | SRET(V=1) 返回到 VU-mode | VS-mode 中 vsstatus.SPP=0, 执行 SRET | 返回 VU-mode, SPP=0 |
| TRET-13 | SRET(V=1) SIE/SPIE 恢复 | vsstatus.SPIE=1, 执行 SRET | vsstatus.SIE=1, vsstatus.SPIE=1 |
| TRET-14 | SRET sepc 恢复 PC | 设 sepc=目标地址, 执行 SRET | PC=sepc |
| TRET-15 | MRET mepc 恢复 PC | 设 mepc=目标地址, 执行 MRET | PC=mepc |

---

## Group 4. htinst / mtinst 转换指令

**规范依据**：
- `norm:H_trap_xtinst`：trap 时写入 mtinst/htinst 的值类型（零/转换指令/custom/pseudoinstruction）；除强制伪指令场景外，实现始终允许写零
- `norm:H_trap_xtinst_interrupt`：中断时写零
- `norm:H_trap_xtinst_exception`：同步异常写入非零值时必须满足三类合法形式之一（标准转换指令/custom/伪指令）
- `norm:H_trap_xtinst_val`：各异常类型可写入的值类型（tinst-values 表）；custom 值仅限非标准指令，标准指令（如 ecall/illegal-instruction）只允许写零
- `norm:H_trap_xtinst_guestpage`：隐式 VS-stage 访问引发 guest-page-fault 且 htval/mtval2 非零时必须写 pseudoinstruction，不允许零
- `norm:H_trap_xtinst_guestpage_rw`：read 用 0x00003000，write（A/D 更新）用 0x00003020（RV64）

**测试职责**：验证 htinst/mtinst 在各种 trap 场景下的写入值。

**严格验证原则**（针对评审 Gap：原用例对 trap 写入值的检查过于宽松，存在恒真断言）：
1. SPEC 允许实现在非强制场景写零，因此“零”必须被接受；但当实现写入非零值时，必须与 SPEC 推导的期望值精确匹配（golden 值从 mepc 处的实际陷阱指令按转换规则计算），不接受“任意非零值”。
2. 隐式 VS-stage 访问 fault 且 htval/mtval2 非零时，htinst/mtinst 必须精确等于伪指令值，零不合法（norm:H_trap_xtinst_guestpage）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TINST-01 | 中断 trap 时 mtinst/htinst=0 | hvip.VSSIP 注入 VS 软件中断（hideleg.VSSIP=0；mideleg VS 位只读 1，必然 trap 到 HS-mode） | cause 为中断且来自 V=1，htinst=0（严格） |
| TINST-02 | ecall trap 时 htinst=0 | VS-mode ecall（标准指令，custom 不允许） | htinst=0（严格） |
| TINST-03 | load guest-page-fault htinst 值 | VS-mode 确定性 `ld` 触发 guest-page-fault | htinst=0 或精确等于该 `ld` 的 SPEC 转换指令（golden，从 mepc 指令计算） |
| TINST-04 | store guest-page-fault htinst 值 | VS-mode 确定性 `sd` 触发 guest-page-fault | htinst=0 或精确等于该 `sd` 的 SPEC 转换指令（golden） |
| TINST-05 | 隐式 VS-stage 读 fault 的 pseudoinstruction | VS-stage 叶页表页在 G-stage 不可读，触发隐式读 guest-page-fault | htval≠0 时 htinst=0x00003000（零不允许）；htval=0 时接受 |
| TINST-06 | 隐式写（A/D 更新）的 pseudoinstruction | VS-stage 叶页表页在 G-stage D=0，触发隐式写 fault；平台支持 Svadu 时 SKIP | htval≠0 时 htinst=0x00003020（零不允许） |
| TINST-07 | 转换指令字段结构验证 | 32-bit load 触发 fault，htinst 非零时逐字段校验 | opcode/funct3/rd 保留、imm 清零、Addr Offset 正确、bits1:0=11 |
| TINST-08 | 压缩指令转换后 bit 1:0 编码 | 16-bit C.LW 触发 fault | htinst bits 1:0 = 01（压缩指令）；当前框架无压缩探针，保留 SKIP |
| TINST-09 | page-fault 不产生 pseudoinstruction | VS-stage 叶 PTE R=0 触发 load page-fault（cause=13，非 guest-page-fault） | htinst=0 或转换指令（golden 精确匹配）；不允许伪指令值 |
| TINST-10 | illegal-instruction 只允许写零 | VS-mode 执行非法指令（标准异常，tinst-values 表仅允许 Zero） | htinst=0（严格） |

---

## Group 5. mstatus 增强（Hypervisor 相关）

**规范依据**：
- `norm:mstatus_mpv_op`：MPV 字段在 trap 到 M-mode 时写入 V 的旧值；MRET 时 V←MPV（除 MPP=3 时 V 保持 0）
- `norm:mstatus_gva_op`：M-mode GVA 字段
- `norm:mstatus_modes`：TSR/TVM 仅影响 HS-mode，TW 影响所有非 M-mode
- `norm:mstatus_tvm_hs`：TVM=1 阻止 HS-mode 访问 hgatp 和执行 HFENCE.GVMA，不影响 vsatp/HFENCE.VVMA
- `norm:mstatus_mprv_hypervisor`：MPRV=1 + MPV 控制两阶段翻译触发
- `norm:mstatus_mprv_hlsv`：MPRV 不影响 HLV/HLVX/HSV

**测试职责**：验证 mstatus 中 Hypervisor 相关字段的行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MSTAT-01 | MPV 在 VS trap 到 M-mode 时写入 1 | VS-mode 异常 trap 到 M-mode | mstatus.MPV=1 |
| MSTAT-02 | MPV 在 HS trap 到 M-mode 时写入 0 | HS-mode 异常 trap 到 M-mode | mstatus.MPV=0 |
| MSTAT-03 | MRET 时 V←MPV | 设 MPV=1, MPP=1, MRET | V=1（进入 VS-mode） |
| MSTAT-04 | MRET MPP=3 时 V 保持 0 | 设 MPV=1, MPP=3, MRET | V=0（进入 M-mode，忽略 MPV） |
| MSTAT-05 | M-mode GVA 正确 | VS-mode guest-page-fault trap 到 M-mode | mstatus.GVA=1 |
| MSTAT-06 | TSR 仅影响 HS-mode | mstatus.TSR=1, VS-mode SRET | 不触发 illegal-instruction（TSR 不影响 VS-mode） |
| MSTAT-07 | TVM=1 阻止 HS-mode 访问 hgatp | mstatus.TVM=1, HS-mode csrr hgatp | illegal-instruction exception |
| MSTAT-08 | TVM=1 阻止 HS-mode HFENCE.GVMA | mstatus.TVM=1, HS-mode HFENCE.GVMA | illegal-instruction exception |
| MSTAT-09 | TVM=1 不影响 vsatp 访问 | mstatus.TVM=1, HS-mode csrr vsatp | 正常访问 |
| MSTAT-10 | TVM=1 不影响 HFENCE.VVMA | mstatus.TVM=1, HS-mode HFENCE.VVMA | 正常执行 |
| MSTAT-11 | MPRV=1 MPV=1 MPP=1 触发两阶段翻译 | 设 MPRV=1, MPV=1, MPP=1, M-mode load | VS-level 两阶段翻译生效 |
| MSTAT-12 | MPRV=1 MPV=0 不触发两阶段翻译 | 设 MPRV=1, MPV=0, MPP=1, M-mode load | 仅 HS-level 翻译 |
| MSTAT-13 | MPRV 不影响 HLV/HSV | 设 MPRV=1 (任意 MPV), HS-mode HLV | HLV 始终按 V=1 + SPVP 执行 |
| MSTAT-14 | TW=1 影响 VS-mode | mstatus.TW=1, VS-mode WFI | illegal-instruction exception（TW 影响所有非 M-mode） |

---

## Group 6. mtval2 / mtinst 寄存器（M-mode Trap）

**规范依据**：
- `norm:mtval2_sz_acc_op`：MXLEN-bit 读写寄存器
- `norm:mtval2_trapval`：guest-page-fault trap 到 M-mode 时 mtval2 写入 GPA >> 2 或零；其他 trap 必须写零
- `norm:mtval2_trapval_vstrans`：隐式 VS-stage 访问导致 guest-page-fault 时 mtval2 写入隐式访问的 GPA
- `norm:mtval2_val`：WARL，必须能保持零；写入任意值不要求回显，但读回必须稳定
- `norm:mtinst_sz_acc_op` / `norm:mtinst_val`：mtinst 格式与 WARL（仅需能保持 trap 时可能自动写入的值）

**测试职责**：验证 M-mode trap 时 mtval2/mtinst 的写入行为。本套件中 VS/HS trap 默认不委托，统一进入 M-mode，框架在 M-mode trap 入口捕获 mtval2/mtinst（`trap_get_htval()`/`trap_get_htinst()` 在 M-mode 递送路径下即为 mtval2/mtinst）。

**严格验证原则**（针对评审 Gap）：trap 写入值按 SPEC 允许集精确断言 —— GPF 时 mtval2 必须是 `0` 或 `GPA>>2` 二者之一（不允许其他值）；WARL 读写不要求原值回显但必须稳定且零必须可保持；隐式访问 fault 时 mtval2 非零则必须精确等于隐式访问 GPA>>2。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MTVAL-01 | mtval2 WARL 读写 | M-mode 写 0、任意 pattern 并重复写入读回 | 写 0 后读回必为 0；任意值读回稳定（重复写同一值读回一致）；不要求原值回显 |
| MTVAL-02 | guest-page-fault trap 到 M-mode 时 mtval2 | VS-mode 确定性 load 触发 guest-page-fault | mtval2 = GPA >> 2 或 0（严格二选一），且与 trap 记录一致 |
| MTVAL-03 | 非 guest-page-fault 时 mtval2=0 | VS-mode ecall trap 到 M-mode | mtval2=0（严格），htval=0 |
| MTVAL-04 | mtinst WARL 读写 | M-mode 写 0、任意 pattern 并重复写入读回 | 同 MTVAL-01 的 WARL 语义 |
| MTVAL-05 | mtinst 在 M-mode guest-page-fault trap 值 | VS-mode 确定性 load 触发 guest-page-fault | mtinst = 0 或精确等于转换指令（golden） |
| MTVAL-06 | 隐式 VS-stage 访问 fault 时 mtval2 | VS-stage 叶页表页在 G-stage 不可读，触发隐式读 fault | mtval2 = 0 或精确等于隐式访问 PTE GPA>>2；非零时 mtinst 必须为 0x00003000 |

---

## Group 7. 异常优先级

**规范依据**：
- `norm:H_exception_priority`：同步异常优先级表（HSyncExcPrio）
- `norm:H_cause_ecall`：HS-mode 和 VS-mode ECALL 使用不同 cause 值

**测试职责**：验证 H 扩展引入的异常类型在优先级排序中的正确位置。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PRIO-01 | virtual-instruction 优先级低于 illegal-instruction | 同时可触发两者的场景 | 由优先级规则决定最终 cause |
| PRIO-02 | guest-page-fault 与 page-fault 优先级 | 两阶段翻译中同时可触发 page-fault 和 guest-page-fault | first encountered fault 先报告 |
| PRIO-03 | VS-mode ECALL cause=10 | VS-mode 执行 ECALL | scause/mcause=10（非 9） |
| PRIO-04 | HS-mode ECALL cause=9 | HS-mode 执行 ECALL | scause/mcause=9 |
| PRIO-05 | VU-mode ECALL cause=8 | VU-mode 执行 ECALL | scause/mcause=8 |

---

## Group 8. hedeleg 异常委托链路

> 本组拆分自原 `Hypervisor_test_plan.md` 的 Group 3，仅保留异常委托用例（DELEG-04~07、DELEG-15、DELEG-16）；CSR 位域属性用例见 `Hypervisor_CSR_test_plan.md`，中断委托与中断号翻译用例见 `Hypervisor_Interrupts_test_plan.md`。

**规范依据**：
- `norm:hedeleg_op`：V=1 时被 medeleg 委托的异常，若 hedeleg 对应 bit 置位则进一步委托到 VS-mode
- `norm:hedeleg_acc`：hedeleg 各 bit 的可写/只读约束（guest-page-fault/virtual-instruction 等 bit 只读零，不可委托）

**测试职责**：验证 hedeleg 异常委托链路（M→HS→VS）的正确性与不可委托异常的约束。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| DELEG-04 | hedeleg 委托 illegal-instruction 到 VS | 设 medeleg[2]=1, hedeleg[2]=1，VS-mode 触发 illegal instruction | trap 进入 VS-mode（vscause=2） |
| DELEG-05 | hedeleg 未委托时 trap 到 HS | 设 medeleg[2]=1, hedeleg[2]=0，VS-mode 触发 illegal instruction | trap 进入 HS-mode（scause=2） |
| DELEG-06 | hedeleg 委托 breakpoint 到 VS | 设 medeleg[3]=1, hedeleg[3]=1，VS-mode 执行 EBREAK | trap 进入 VS-mode（vscause=3） |
| DELEG-07 | hedeleg 委托 ecall-from-VU 到 VS | 设 medeleg[8]=1, hedeleg[8]=1，VU-mode 执行 ECALL | trap 进入 VS-mode（vscause=8） |
| DELEG-15 | guest-page-fault 不可委托到 VS | 验证 hedeleg bits 20/21/23 只读零 | guest-page-fault 始终 trap 到 HS-mode |
| DELEG-16 | virtual-instruction 不可委托到 VS | 验证 hedeleg bit 22 只读零 | virtual-instruction exception 始终 trap 到 HS-mode |
