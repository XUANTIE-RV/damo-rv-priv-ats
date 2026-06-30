**中文 | [English](../testplan_en/Hypervisor_test_plan_en.md)**

# Hypervisor Extension 综合测试计划

## 概述

本测试计划覆盖 RISC-V Hypervisor (H) 扩展中，除 G-stage / VS-stage 地址翻译以外的所有核心功能点。地址翻译相关测试已由 `hyp_gstage_translation_test_plan.md` 和 `hyp_2_stage_translation_test_plan.md` 覆盖，此处不再重复。

已由独立测试计划覆盖的子扩展（Sha/Shcounterenw/Shgatpa/Shvsatpa/Shtvala/Shvstvala/Shvstvecd）也不在本文档范围内。

本测试计划依据 `SPEC/hypervisor.adoc` 中的规范点（norm 标记）编写。

### 本文档覆盖的 SPEC 章节
- Hypervisor and Virtual Supervisor CSRs（hstatus, hedeleg, hideleg, hvip, hip, hie, hgeip, hgeie, henvcfg, hcounteren, htimedelta, htval, htinst, hgatp CSR 行为）
- Virtual Supervisor CSRs（vsstatus, vsip, vsie, vstvec, vsscratch, vsepc, vscause, vstval, vsatp CSR 行为, vstimecmp）
- Machine-Level CSR 增强（mstatus MPV/GVA/TVM/MPRV, mideleg, mip/mie, mtval2, mtinst）
- Hypervisor Instructions（HLV/HLVX/HSV 异常场景, HFENCE.VVMA/HFENCE.GVMA 异常场景）
- Traps（virtual-instruction exception 全场景, trap entry/return, 异常优先级, htinst/mtinst 转换指令）

### 由其他测试计划覆盖
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

本章节列出本文档 Groups 1-19 所有测试组中引用的规范点（norm ID），已去重并按字母顺序排列。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:H_cause_ecall` | HS-mode and VS-mode ECALLs use different cause values so they can be delegated separately. | HS 模式和 VS 模式的 ECALL 使用不同原因值以便分别委托。 |
| `norm:H_cause_virtual_instruction` | When V=1, a virtual-instruction exception (code 22) is normally raised instead of an illegal-instruction exception if the attempted instruction is HS-qualified but is prevented from executing when V=1. An instruction is HS-qualified if it would be valid to execute in HS-mode, assuming TSR and TVM of `mstatus` are both zero. | V=1 时，若尝试的 HS 限定指令被阻止执行，通常引发虚拟指令异常（代码 22）而非非法指令异常。指令在假设 `mstatus`.TSR 和 TVM 均为零时能在 HS 模式有效执行则为 HS 限定。 |
| `norm:H_cause_virtual_instruction_high` | When V=1 and XLEN=32, an invalid attempt to access a high-half CSR raises a virtual-instruction exception instead of an illegal-instruction exception if the same CSR instruction for the corresponding low-half CSR is HS-qualified. | V=1 且 XLEN=32 时，无效的高半 CSR 访问若对应低半 CSR 指令为 HS 限定，则引发虚拟指令异常而非非法指令异常。 |
| `norm:H_exception_priority` | If an instruction may raise multiple synchronous exceptions, the decreasing priority order indicates which exception is taken and reported in `mcause` or `scause`. | 若指令可能引发多个同步异常，按优先级递减顺序决定哪个异常被接收并报告在 `mcause` 或 `scause` 中。 |
| `norm:H_illegalinst_xstatus_fs_vs` | Fields FS and VS in registers `sstatus` and `vsstatus` deviate from the usual HS-qualified rule. If an instruction is prevented from executing because FS or VS is zero in either `sstatus` or `vsstatus`, the exception raised is always an illegal-instruction exception, never a virtual-instruction exception. | `sstatus` 和 `vsstatus` 中的 FS 和 VS 字段偏离通常的 HS 限定规则。若指令因 FS 或 VS 为零而被阻止，始终引发非法指令异常，不引发虚拟指令异常。 |
| `norm:H_scsrs_nomatch` | Some standard supervisor CSRs (`senvcfg`, `scounteren`, and `scontext`, possibly others) have no matching VS CSR. These supervisor CSRs continue to have their usual function and accessibility even when V=1, except with VS-mode and VU-mode substituting for HS-mode and U-mode. | 某些标准 supervisor CSR（如 `senvcfg`、`scounteren`、`scontext` 等）没有对应的 VS CSR。这些 CSR 即使在 V=1 时也保持原有功能和可访问性，只是 VS 模式和 VU 模式分别替代 HS 模式和 U 模式。 |
| `norm:H_trap_deleg` | When a trap occurs in HS-mode or U-mode, it goes to M-mode, unless delegated by `medeleg` or `mideleg`, in which case it goes to HS-mode. When a trap occurs in VS-mode or VU-mode, it goes to M-mode, unless delegated by `medeleg`/`mideleg` to HS-mode, unless further delegated by `hedeleg`/`hideleg` to VS-mode. | HS/U 模式陷阱进入 M 模式，除非通过 `medeleg`/`mideleg` 委托给 HS 模式。VS/VU 模式陷阱进入 M 模式，除非委托给 HS 模式，除非进一步委托给 VS 模式。 |
| `norm:H_trap_hs_csrwrites` | When a trap is taken into HS-mode, V is set to 0, and `hstatus`.SPV and `sstatus`.SPP are set accordingly. If V was 1 before the trap, SPVP is set the same as `sstatus`.SPP; otherwise, SPVP is left unchanged. A trap into HS-mode also writes GVA in `hstatus`, SPIE and SIE in `sstatus`, and CSRs `sepc`, `scause`, `stval`, `htval`, and `htinst`. | 陷阱进入 HS 模式时，V 设为 0，`hstatus`.SPV 和 `sstatus`.SPP 相应设置。陷阱前 V=1 时 SPVP 与 SPP 相同；否则不变。同时写入相关字段和 CSR。 |
| `norm:H_trap_m_csrwrites` | When a trap is taken into M-mode, V gets set to 0, and fields MPV and MPP in `mstatus` are set accordingly. A trap into M-mode also writes fields GVA, MPIE, and MIE in `mstatus` and writes CSRs `mepc`, `mcause`, `mtval`, `mtval2`, and `mtinst`. | 陷阱进入 M 模式时，V 设为 0，`mstatus` 的 MPV 和 MPP 相应设置。同时写入 GVA、MPIE、MIE 及 CSR `mepc`、`mcause`、`mtval`、`mtval2`、`mtinst`。 |
| `norm:H_trap_vs_csrwrites` | When a trap is taken into VS-mode, `vsstatus`.SPP is set accordingly. Register `hstatus` and the HS-level `sstatus` are not modified, and V remains 1. A trap into VS-mode also writes SPIE and SIE in `vsstatus` and writes CSRs `vsepc`, `vscause`, and `vstval`. | 陷阱进入 VS 模式时，`vsstatus`.SPP 相应设置。`hstatus` 和 HS 级 `sstatus` 不修改，V 保持 1。同时写入 `vsstatus` 的 SPIE/SIE 及 CSR `vsepc`、`vscause`、`vstval`。 |
| `norm:H_trap_xtinst` | On any trap into M-mode or HS-mode, one of these values is written to `mtinst` or `htinst`: zero; a transformation of the trapping instruction; a custom value (only if the trapping instruction is non-standard); or a special pseudoinstruction. | 任何陷阱进入 M/HS 模式时，`mtinst`/`htinst` 写入以下之一：零；陷阱指令的转换；自定义值（仅限非标准指令）；或特殊伪指令。 |
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
| `norm:H_vscsrs_acc_m_hs` | The VS CSRs can be accessed as themselves only from M-mode or HS-mode. | VS CSR 只能从 M 模式或 HS 模式以其自身地址访问。 |
| `norm:H_vscsrs_acc_u` | Attempts from U-mode cause an illegal-instruction exception as usual. | 来自 U 模式的访问尝试照常引发非法指令异常。 |
| `norm:H_vscsrs_acc_vs` | When V=1, an attempt to read or write a VS CSR directly by its own separate CSR address causes a virtual-instruction exception. | 当 V=1 时，尝试通过 VS CSR 自身的独立 CSR 地址直接读写它会引发虚拟指令异常。 |
| `norm:H_vscsrs_sub` | When V=1, the VS CSRs substitute for the corresponding supervisor CSRs, taking over all functions of the usual supervisor CSRs except as specified otherwise. Instructions that normally read or modify a supervisor CSR shall instead access the corresponding VS CSR. | 当 V=1 时，VS CSR 替代对应的 supervisor CSR，接管其所有功能（除非另有规定）。通常读写 supervisor CSR 的指令将改为访问对应的 VS CSR。 |
| `norm:H_vscsrs_v0` | When V=0, the VS CSRs do not ordinarily affect the behavior of the machine other than being readable and writable by CSR instructions. | 当 V=0 时，VS CSR 通常不影响机器行为，仅可被 CSR 指令读写。 |
| `norm:H_vscsrs_v1` | While V=1, the normal HS-level supervisor CSRs that are replaced by VS CSRs retain their values but do not affect the behavior of the machine unless specifically documented to do so. | 当 V=1 时，被 VS CSR 替代的普通 HS 级 supervisor CSR 保留其值，但不影响机器行为（除非特别说明）。 |
| `norm:geilen` | The number of bits implemented in `hgeip` and `hgeie` for guest external interrupts is UNSPECIFIED and may be zero. This number is known as GEILEN. The least-significant bits are implemented first, apart from bit 0. Hence, if GEILEN is nonzero, bits GEILEN:1 shall be writable in `hgeie`, and all other bit positions shall be read-only zeros in both `hgeip` and `hgeie`. | `hgeip` 和 `hgeie` 中客户外部中断实现的位数未指定，可为零。该数目称为 GEILEN。除第 0 位外，最低有效位先实现。若 GEILEN 非零，`hgeie` 的 GEILEN:1 位可写，其余位在两个寄存器中均为只读零。 |
| `norm:hedeleg_acc` | Each bit of `hedeleg` shall be either writable or read-only zero. Many bits of `hedeleg` are required specifically to be writable or zero, as enumerated in the table. Bit 0, corresponding to instruction address-misaligned exceptions, must be writable if IALIGN=32. | `hedeleg` 的每一位要么可写要么为只读零。第 0 位（指令地址未对齐异常）在 IALIGN=32 时必须可写。 |
| `norm:hedeleg_op` | A synchronous trap that has been delegated to HS-mode (using `medeleg`) is further delegated to VS-mode if V=1 before the trap and the corresponding `hedeleg` bit is set. | 已通过 `medeleg` 委托给 HS 模式的同步陷阱，若陷阱前 V=1 且对应的 `hedeleg` 位已设置，则进一步委托给 VS 模式。 |
| `norm:hedeleg_sz_acc` | Register `hedeleg` is a 64-bit read/write register. | `hedeleg` 是一个 64 位读写寄存器。 |
| `norm:henvcfg_adue_op` | If the Svadu extension is implemented, the ADUE bit controls whether hardware updating of PTE A/D bits is enabled for VS-stage address translation. When ADUE=1, hardware updating is enabled. When ADUE=0, the implementation behaves as though Svade were implemented for VS-stage address translation. If Svadu is not implemented, ADUE is read-only zero. | 若实现了 Svadu 扩展，ADUE 位控制 VS 阶段地址翻译的 PTE A/D 位硬件更新是否启用。ADUE=1 时启用，ADUE=0 时行为如同实现了 Svade。未实现 Svadu 时，ADUE 为只读零。 |
| `norm:henvcfg_cbcfe` | The Zicbom extension adds the CBCFE field to `henvcfg`. When V=1, if CBO.CLEAN and CBO.FLUSH are HS-qualified and CBCFE=1, they are enabled; if CBCFE=0, they raise a virtual-instruction exception. When Zicbom is not implemented, CBCFE is read-only zero. | Zicbom 扩展向 `henvcfg` 添加 CBCFE 字段。V=1 时若 HS 限定且 CBCFE=1，则 CBO.CLEAN 和 CBO.FLUSH 启用；CBCFE=0 时引发虚拟指令异常。未实现时为只读零。 |
| `norm:henvcfg_cbie` | The Zicbom extension adds the CBIE WARL field to `henvcfg`. The CBIE field controls execution of CBO.INVAL in privilege modes VS and VU. The encoding `10b` is reserved. When Zicbom is not implemented, CBIE is read-only zero. | Zicbom 扩展向 `henvcfg` 添加 CBIE WARL 字段，控制 VS 和 VU 模式下 CBO.INVAL 的执行。编码 `10b` 保留。未实现时为只读零。 |
| `norm:henvcfg_cbze` | The Zicboz extension adds the CBZE field to `henvcfg`. When CBZE=1, CBO.ZERO is enabled for execution in VS/VU mode; when CBZE=0, it raises a virtual-instruction exception. When the Zicboz extension is not implemented, CBZE is read-only zero. | Zicboz 扩展向 `henvcfg` 添加 CBZE 字段。CBZE=1 时在 VS/VU 模式下启用 CBO.ZERO；CBZE=0 时引发虚拟指令异常。未实现时为只读零。 |
| `norm:henvcfg_dte_op` | The Ssdbltrp extension adds the double-trap-enable (DTE) field in `henvcfg`. When `henvcfg`.DTE is zero, the implementation behaves as though Ssdbltrp is not implemented for VS-mode and the `vsstatus`.SDT bit is read-only zero. | Ssdbltrp 扩展向 `henvcfg` 添加 DTE 字段。`henvcfg`.DTE=0 时，行为如同未为 VS 模式实现 Ssdbltrp，`vsstatus`.SDT 为只读零。 |
| `norm:henvcfg_fiom_op` | If bit FIOM is set to one in `henvcfg`, FENCE instructions executed when V=1 are modified so the requirement to order accesses to device I/O implies also the requirement to order main memory accesses. | 若 `henvcfg` 的 FIOM 位设为 1，V=1 时执行的 FENCE 指令被修改，对设备 I/O 的排序要求也隐含对主存访问的排序要求。 |
| `norm:henvcfg_fiom_order` | Similarly, when FIOM=1 and V=1, if an atomic instruction that accesses a region ordered as device I/O has its _aq_ and/or _rl_ bit set, then that instruction is ordered as though it accesses both device I/O and memory. | 当 FIOM=1 且 V=1 时，若一条原子指令访问设备 I/O 有序区域并设置了 aq/rl 位，该指令的排序如同同时访问设备 I/O 和内存。 |
| `norm:henvcfg_lpe_op` | The Zicfilp extension adds the LPE field in `henvcfg`. When LPE=1, the Zicfilp extension is enabled in VS-mode. When LPE=0, the hart does not update the ELP state and LPAD operates as a no-op. | Zicfilp 扩展向 `henvcfg` 添加 LPE 字段。LPE=1 时在 VS 模式下启用 Zicfilp。LPE=0 时 hart 不更新 ELP 状态，LPAD 作为 no-op 运行。 |
| `norm:henvcfg_pbmte_op` | The PBMTE bit controls whether the Svpbmt extension is available for use in VS-stage address translation. When PBMTE=1, Svpbmt is available for VS-stage address translation. When PBMTE=0, the implementation behaves as though Svpbmt were not implemented for VS-stage address translation. If Svpbmt is not implemented, PBMTE is read-only zero. | PBMTE 位控制 Svpbmt 扩展是否可用于 VS 阶段地址翻译。PBMTE=1 时可用，PBMTE=0 时行为如同未实现。若未实现 Svpbmt，PBMTE 为只读零。 |
| `norm:henvcfg_pmm_op` | If the Ssnpm extension is implemented, the PMM field enables or disables pointer masking for VS-mode. When not implemented, PMM is read-only zero. PMM is read-only zero for RV32. | 若实现了 Ssnpm 扩展，PMM 字段启用或禁用 VS 模式的指针屏蔽。未实现时为只读零。RV32 下也为只读零。 |
| `norm:henvcfg_sse_op` | The Zicfiss extension adds the SSE field in `henvcfg`. If SSE=1, the Zicfiss extension is activated in VS-mode. When SSE=0, the extension remains inactive in VS-mode with specific behavior changes. | Zicfiss 扩展向 `henvcfg` 添加 SSE 字段。SSE=1 时在 VS 模式下激活 Zicfiss。SSE=0 时保持不活跃，具有特定行为变化。 |
| `norm:henvcfg_stce` | The Sstc extension adds the STCE (STimecmp Enable) bit to `henvcfg` CSR. When the Sstc extension is not implemented, STCE is read-only zero. The STCE bit enables `vstimecmp` for VS-mode when set to one. When STCE is zero, an attempt to access `stimecmp` (really `vstimecmp`) when V=1 raises a virtual-instruction exception, and VSTIP in `hip` reverts to its defined behavior as if this extension is not implemented. | Sstc 扩展向 `henvcfg` 添加 STCE 位。未实现时为只读零。STCE=1 时为 VS 模式启用 `vstimecmp`。STCE=0 时 V=1 下访问 `stimecmp`（实际为 `vstimecmp`）引发虚拟指令异常。 |
| `norm:henvcfg_sz_acc_op` | The `henvcfg` CSR is a 64-bit read/write register that controls certain characteristics of the execution environment when virtualization mode V=1. | `henvcfg` 是一个 64 位读写寄存器，控制虚拟化模式 V=1 时执行环境的某些特性。 |
| `norm:hgeie_op` | Register `hgeie` selects the subset of guest external interrupts that cause a supervisor-level (HS-level) guest external interrupt. The enable bits in `hgeie` do not affect the VS-level external interrupt signal selected from `hgeip` by `hstatus`.VGEIN. | `hgeie` 选择引起 HS 级客户外部中断的客户外部中断子集。`hgeie` 中的使能位不影响由 `hstatus`.VGEIN 从 `hgeip` 中选择的 VS 级外部中断信号。 |
| `norm:hgeie_sz_acc_op` | The `hgeie` register is an HSXLEN-bit read/write register that contains enable bits for the guest external interrupts at this hart. | `hgeie` 是一个 HSXLEN 位读写寄存器，包含此 hart 的客户外部中断使能位。 |
| `norm:hgeip_hgeie_fields` | Guest external interrupt number _i_ corresponds with bit _i_ in both `hgeip` and `hgeie`. | 客户外部中断号 _i_ 对应 `hgeip` 和 `hgeie` 中的位 _i_。 |
| `norm:hgeip_sz_acc_op` | The `hgeip` register is an HSXLEN-bit read-only register that indicates pending guest external interrupts for this hart. | `hgeip` 是一个 HSXLEN 位只读寄存器，指示此 hart 的待处理客户外部中断。 |
| `norm:hideleg_acc` | Among bits 15:0 of `hideleg`, bits 10, 6, and 2 (corresponding to the standard VS-level interrupts) are writable, and bits 12, 9, 5, and 1 (corresponding to the standard S-level interrupts) are read-only zeros. | `hideleg` 的 15:0 位中，第 10、6、2 位（标准 VS 级中断）可写，第 12、9、5、1 位（标准 S 级中断）为只读零。 |
| `norm:hideleg_hs` | An interrupt _i_ will trap to HS-mode whenever all of the following are true: (a) either the current operating mode is HS-mode and the SIE bit in the `sstatus` register is set, or the current operating mode has less privilege than HS-mode; (b) bit _i_ is set in both `sip` and `sie`, or in both `hip` and `hie`; and (c) bit _i_ is not set in `hideleg`. | 中断 _i_ 在以下条件全部满足时陷入 HS 模式：(a) 当前为 HS 模式且 `sstatus`.SIE=1，或当前特权低于 HS 模式；(b) 位 _i_ 在 `sip`/`sie` 或 `hip`/`hie` 中都已设置；(c) 位 _i_ 在 `hideleg` 中未设置。 |
| `norm:hideleg_op` | An interrupt that has been delegated to HS-mode (using `mideleg`) is further delegated to VS-mode if the corresponding `hideleg` bit is set. | 已通过 `mideleg` 委托给 HS 模式的中断，若对应 `hideleg` 位已设置，则进一步委托给 VS 模式。 |
| `norm:hideleg_sz_acc` | Register `hideleg` is an HSXLEN-bit read/write register. | `hideleg` 是一个 HSXLEN 位读写寄存器。 |
| `norm:hideleg_trans` | When a virtual supervisor external interrupt (code 10) is delegated to VS-mode, it is automatically translated by the machine into a supervisor external interrupt (code 9) for VS-mode. Likewise, virtual supervisor timer interrupt (6) is translated into supervisor timer interrupt (5), and virtual supervisor software interrupt (2) into supervisor software interrupt (1). | 当虚拟 supervisor 外部中断（代码 10）被委托给 VS 模式时，自动翻译为 supervisor 外部中断（代码 9）。类似地，虚拟定时器中断(6)→定时器中断(5)，虚拟软件中断(2)→软件中断(1)。 |
| `norm:hie_op` | `hie` contains enable bits for the same interrupts. | `hie` 包含相同中断的使能位。 |
| `norm:hip_hie_sz_acc` | Registers `hip` and `hie` are HSXLEN-bit read/write registers that supplement HS-level's `sip` and `sie` respectively. | `hip` 和 `hie` 是 HSXLEN 位读写寄存器，分别补充 HS 级的 `sip` 和 `sie`。 |
| `norm:hip_op` | The `hip` register indicates pending VS-level and hypervisor-specific interrupts. | `hip` 寄存器指示待处理的 VS 级和 hypervisor 特定中断。 |
| `norm:hip_vseip_vseie_op` | Bits `hip`.VSEIP and `hie`.VSEIE are the interrupt-pending and interrupt-enable bits for VS-level external interrupts. VSEIP is read-only in `hip`, and is the logical-OR of: bit VSEIP of `hvip`; the bit of `hgeip` selected by `hstatus`.VGEIN; and any other platform-specific external interrupt signal directed to VS-level. | `hip`.VSEIP 和 `hie`.VSEIE 是 VS 级外部中断的中断待处理和使能位。VSEIP 在 `hip` 中为只读，是 `hvip`.VSEIP、`hgeip` 中由 `hstatus`.VGEIN 选择的位、以及其他平台特定的 VS 级外部中断信号的逻辑或。 |
| `norm:hip_vssip_vssie_op` | Bits `hip`.VSSIP and `hie`.VSSIE are the interrupt-pending and interrupt-enable bits for VS-level software interrupts. VSSIP in `hip` is an alias (writable) of the same bit in `hvip`. | `hip`.VSSIP 和 `hie`.VSSIE 是 VS 级软件中断的中断待处理和使能位。`hip` 中的 VSSIP 是 `hvip` 中相同位的别名（可写）。 |
| `norm:hip_vstip_clear` | If the result of this comparison changes, it is guaranteed to be reflected in VSTIP eventually, but not necessarily immediately. The interrupt remains posted until `vstimecmp` becomes greater than (`time` + `htimedelta`), typically as a result of writing `vstimecmp`. | 比较结果变化保证最终反映在 VSTIP 中，但不一定立即。中断保持到 `vstimecmp` 大于 (`time` + `htimedelta`)，通常通过写入 `vstimecmp` 实现。 |
| `norm:hip_vstip_enable` | The interrupt will be taken based on the standard interrupt enable and delegation rules while V=1. | 中断将在 V=1 时根据标准中断使能和委托规则被接收。 |
| `norm:hip_vstip_op` | A virtual supervisor timer interrupt becomes pending, as reflected in the VSTIP bit in the `hip` register, whenever (`time` + `htimedelta`), truncated to 64 bits, contains a value greater than or equal to `vstimecmp`, treating the values as unsigned integers. | 当 (`time` + `htimedelta`) 截断为 64 位后的值大于或等于 `vstimecmp`（无符号比较）时，虚拟 supervisor 定时器中断变为待处理，反映在 `hip` 的 VSTIP 位中。 |
| `norm:hip_vstip_vstie_acc_op` | Bits `hip`.VSTIP and `hie`.VSTIE are the interrupt-pending and interrupt-enable bits for VS-level timer interrupts. VSTIP is read-only in `hip`, and is the logical-OR of `hvip`.VSTIP and, when the Sstc extension is implemented, the timer interrupt signal resulting from `vstimecmp`. | `hip`.VSTIP 和 `hie`.VSTIE 是 VS 级定时器中断的中断待处理和使能位。VSTIP 在 `hip` 中为只读，是 `hvip`.VSTIP 与（当实现 Sstc 扩展时）`vstimecmp` 产生的定时器中断信号的逻辑或。 |
| `norm:hsint_priority` | Multiple simultaneous interrupts destined for HS-mode are handled in the following decreasing priority order: SEI, SSI, STI, SGEI, VSEI, VSSI, VSTI, LCOFI. | 多个同时到达 HS 模式的中断按以下优先级递减顺序处理：SEI、SSI、STI、SGEI、VSEI、VSSI、VSTI、LCOFI。 |
| `norm:hstatus_gva_op` | Field GVA (Guest Virtual Address) is written by the implementation whenever a trap is taken into HS-mode. For any trap that writes a guest virtual address to `stval`, GVA is set to 1. For any other trap into HS-mode, GVA is set to 0. | GVA 字段在进入 HS 模式的陷阱时由实现写入。写入客户虚拟地址到 `stval` 的陷阱设置 GVA=1，其他陷阱设置 GVA=0。 |
| `norm:hstatus_hu_op` | Field HU (Hypervisor in U-mode) controls whether the virtual-machine load/store instructions, HLV, HLVX, and HSV, can be used also in U-mode. When HU=1, these instructions can be executed in U-mode the same as in HS-mode. When HU=0, all hypervisor instructions cause an illegal-instruction exception in U-mode. | HU 字段控制虚拟机 load/store 指令是否也可在 U 模式下使用。HU=1 时可在 U 模式下执行；HU=0 时所有 hypervisor 指令在 U 模式下引发非法指令异常。 |
| `norm:hstatus_spv_op` | The SPV bit (Supervisor Previous Virtualization mode) is written by the implementation whenever a trap is taken into HS-mode. Just as the SPP bit in `sstatus` is set to the (nominal) privilege mode at the time of the trap, the SPV bit in `hstatus` is set to the value of the virtualization mode V at the time of the trap. | SPV 位在每次进入 HS 模式的陷阱时由实现写入，设置为陷阱发生时虚拟化模式 V 的值。 |
| `norm:hstatus_spv_sret` | When an SRET instruction is executed when V=0, V is set to SPV. | 当 V=0 时执行 SRET 指令，V 被设置为 SPV。 |
| `norm:hstatus_spvp_op` | When V=1 and a trap is taken into HS-mode, bit SPVP (Supervisor Previous Virtual Privilege) is set to the nominal privilege mode at the time of the trap, the same as `sstatus`.SPP. But if V=0 before a trap, SPVP is left unchanged on trap entry. | 当 V=1 且陷阱进入 HS 模式时，SPVP 位设置为陷阱时的名义特权模式。若陷阱前 V=0，SPVP 在陷阱入口不变。 |
| `norm:hstatus_sz_acc_op` | The `hstatus` register is an HSXLEN-bit read/write register... provides facilities analogous to the `mstatus` register for tracking and controlling the exception behavior of a VS-mode guest. | `hstatus` 是一个 HSXLEN 位的读写寄存器，提供类似 `mstatus` 的功能，用于跟踪和控制 VS 模式客户机的异常行为。 |
| `norm:hstatus_vgein_op` | The VGEIN (Virtual Guest External Interrupt Number) field selects a guest external interrupt source for VS-level external interrupts. VGEIN is a WLRL field that must be able to hold values between zero and the maximum guest external interrupt number (known as GEILEN), inclusive. | VGEIN 字段选择 VS 级外部中断的客户外部中断源。VGEIN 是一个 WLRL 字段，必须能保持 0 到 GEILEN（含）之间的值。 |
| `norm:hstatus_vsbe_op` | The VSBE bit is a WARL field that controls the endianness of explicit memory accesses made from VS-mode. | VSBE 位是一个 WARL 字段，控制 VS 模式下显式内存访问的字节序。 |
| `norm:hstatus_vsxl_32` | When HSXLEN=32, the VSXL field does not exist, and VSXLEN=32. | 当 HSXLEN=32 时，VSXL 字段不存在，VSXLEN=32。 |
| `norm:hstatus_vsxl_64` | When HSXLEN=64, VSXL is a WARL field that is encoded the same as the MXL field of `misa`. | 当 HSXLEN=64 时，VSXL 是一个 WARL 字段，编码方式与 `misa` 的 MXL 字段相同。 |
| `norm:hstatus_vsxl_op` | The VSXL field controls the effective XLEN for VS-mode (known as VSXLEN), which may differ from the XLEN for HS-mode (HSXLEN). | VSXL 字段控制 VS 模式的有效 XLEN（即 VSXLEN），可以与 HS 模式的 XLEN（HSXLEN）不同。 |
| `norm:hstatus_vtsr_op` | When VTSR=1, an attempt in VS-mode to execute SRET raises a virtual-instruction exception. | 当 VTSR=1 时，在 VS 模式下尝试执行 SRET 将引发虚拟指令异常。 |
| `norm:hstatus_vtvm_op` | When VTVM=1, an attempt in VS-mode to execute SFENCE.VMA or SINVAL.VMA or to access CSR `satp` raises a virtual-instruction exception. | 当 VTVM=1 时，在 VS 模式下尝试执行 SFENCE.VMA 或 SINVAL.VMA 或访问 CSR `satp` 将引发虚拟指令异常。 |
| `norm:hstatus_vtw_op` | When VTW=1 (and assuming `mstatus`.TW=0), an attempt in VS-mode to execute WFI raises a virtual-instruction exception if the WFI does not complete within an implementation-specific, bounded time limit. | 当 VTW=1（且 `mstatus`.TW=0）时，在 VS 模式下执行 WFI 若未在实现特定的有限时间内完成，将引发虚拟指令异常。 |
| `norm:htimedelta_sz_acc_op` | The `htimedelta` CSR is a 64-bit read/write register that contains the delta between the value of the `time` CSR and the value returned in VS-mode or VU-mode. Reading the `time` CSR in VS or VU mode returns the sum of `htimedelta` and the actual value of `time`. | `htimedelta` 是一个 64 位读写寄存器，包含 `time` CSR 的值与 VS/VU 模式下返回值之间的差值。VS/VU 模式下读取 `time` 返回 `htimedelta` 与实际 `time` 值之和。 |
| `norm:hvip_acc` | The standard portion (bits 15:0) of `hvip` is formatted as shown. Bits VSEIP, VSTIP, and VSSIP of `hvip` are writable. | `hvip` 的标准部分（15:0 位）中，VSEIP、VSTIP 和 VSSIP 位可写。 |
| `norm:hvip_sz_op` | Register `hvip` is an HSXLEN-bit read/write register that a hypervisor can write to indicate virtual interrupts intended for VS-mode. Bits of `hvip` that are not writable are read-only zeros. | `hvip` 是一个 HSXLEN 位读写寄存器，hypervisor 可写入以指示用于 VS 模式的虚拟中断。不可写的位为只读零。 |
| `norm:mideleg_acc_h` | When the hypervisor extension is implemented, bits 10, 6, and 2 of `mideleg` are each read-only one. Furthermore, if GEILEN is nonzero, bit 12 of `mideleg` is also read-only one. VS-level interrupts and guest external interrupts are always delegated past M-mode to HS-mode. | 实现 hypervisor 扩展时，`mideleg` 的第 10、6、2 位为只读 1。若 GEILEN 非零，第 12 位也为只读 1。VS 级中断和客户外部中断始终委托到 HS 模式。 |
| `norm:mideleg_hroz` | For bits of `mideleg` that are zero, the corresponding bits in `hideleg`, `hip`, and `hie` are read-only zeros. | `mideleg` 中为零的位，`hideleg`、`hip`、`hie` 中对应位为只读零。 |
| `norm:mip_mie_alias` | Bits SGEIP, VSEIP, VSTIP, and VSSIP in `mip` are aliases for the same bits in hypervisor CSR `hip`, while SGEIE, VSEIE, VSTIE, and VSSIE in `mie` are aliases for the same bits in `hie`. | `mip` 中的 SGEIP、VSEIP、VSTIP、VSSIP 是 `hip` 中相同位的别名；`mie` 中的 SGEIE、VSEIE、VSTIE、VSSIE 是 `hie` 中相同位的别名。 |
| `norm:mip_mie_vs` | The hypervisor extension gives registers `mip` and `mie` additional active bits for the hypervisor-added interrupts. | hypervisor 扩展为 `mip` 和 `mie` 添加 hypervisor 新增中断的额外活跃位。 |
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
| `norm:sie_hip_hie_mutex` | For each writable bit in `sie`, the corresponding bit shall be read-only zero in both `hip` and `hie`. Hence, the nonzero bits in `sie` and `hie` are always mutually exclusive, and likewise for `sip` and `hip`. | 对于 `sie` 中的每个可写位，`hip` 和 `hie` 中的对应位必须为只读零。`sie` 和 `hie` 的非零位始终互斥，`sip` 和 `hip` 同理。 |
| `norm:sret_dt` | If the Ssdbltrp extension is implemented, when SRET is executed in HS-mode, if the new privilege mode is VU, the SRET instruction sets `vsstatus`.SDT to 0. When executed in VS-mode, `vsstatus`.SDT is set to 0. | 若实现了 Ssdbltrp 扩展，HS 模式执行 SRET 且新特权模式为 VU 时，设 `vsstatus`.SDT=0。VS 模式执行时也设 `vsstatus`.SDT=0。 |
| `norm:sret_v0` | When executed in M-mode or HS-mode (V=0), SRET first determines the new privilege mode according to `hstatus`.SPV and `sstatus`.SPP. SRET then sets `hstatus`.SPV=0, and in `sstatus` sets SPP=0, SIE=SPIE, and SPIE=1. Lastly, SRET sets the privilege mode and sets pc=sepc. | 在 M/HS 模式（V=0）执行时，SRET 根据 `hstatus`.SPV 和 `sstatus`.SPP 确定新特权模式，然后设 SPV=0、SPP=0、SIE=SPIE、SPIE=1，最后设置特权模式并 pc=sepc。 |
| `norm:sret_v1` | When executed in VS-mode (V=1), SRET sets the privilege mode accordingly, in `vsstatus` sets SPP=0, SIE=SPIE, and SPIE=1, and lastly sets pc=vsepc. | 在 VS 模式（V=1）执行时，SRET 相应设置特权模式，在 `vsstatus` 中设 SPP=0、SIE=SPIE、SPIE=1，最后 pc=vsepc。 |
| `norm:time_htimedelta_req` | If the `time` CSR is implemented, `htimedelta` (and `htimedeltah` for XLEN=32) must be implemented. | 若实现了 `time` CSR，则 `htimedelta`（XLEN=32 时还有 `htimedeltah`）必须实现。 |
| `norm:vscause_sz_acc_op` | The `vscause` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `scause`. When V=1, `vscause` substitutes for the usual `scause`. When V=0, `vscause` does not directly affect the behavior of the machine. | `vscause` 是 VSXLEN 位读写寄存器，VS 模式版本的 `scause`。V=1 时替代 `scause`；V=0 时不直接影响机器行为。 |
| `norm:vscause_wlrl` | `vscause` is a WLRL register that must be able to hold the same set of values that `scause` can hold. | `vscause` 是一个 WLRL 寄存器，必须能保持与 `scause` 相同的值集合。 |
| `norm:vsepc_warl` | `vsepc` is a WARL register that must be able to hold the same set of values that `sepc` can hold. | `vsepc` 是一个 WARL 寄存器，必须能保持与 `sepc` 相同的值集合。 |
| `norm:vsip_vsie_sei` | When bit 10 of `hideleg` is zero, `vsip`.SEIP and `vsie`.SEIE are read-only zeros. Else, they are aliases of `hip`.VSEIP and `hie`.VSEIE. | 当 `hideleg` 第 10 位为零时，`vsip`.SEIP 和 `vsie`.SEIE 为只读零。否则为 `hip`.VSEIP 和 `hie`.VSEIE 的别名。 |
| `norm:vsip_vsie_ssi` | When bit 2 of `hideleg` is zero, `vsip`.SSIP and `vsie`.SSIE are read-only zeros. Else, they are aliases of `hip`.VSSIP and `hie`.VSSIE. | 当 `hideleg` 第 2 位为零时，`vsip`.SSIP 和 `vsie`.SSIE 为只读零。否则为 `hip`.VSSIP 和 `hie`.VSSIE 的别名。 |
| `norm:vsip_vsie_sti` | When bit 6 of `hideleg` is zero, `vsip`.STIP and `vsie`.STIE are read-only zeros. Else, they are aliases of `hip`.VSTIP and `hie`.VSTIE. | 当 `hideleg` 第 6 位为零时，`vsip`.STIP 和 `vsie`.STIE 为只读零。否则为 `hip`.VSTIP 和 `hie`.VSTIE 的别名。 |
| `norm:vsip_vsie_sz_acc_op` | The `vsip` and `vsie` registers are VSXLEN-bit read/write registers that are VS-mode's versions of supervisor CSRs `sip` and `sie`. When V=1, `vsip` and `vsie` substitute for the usual `sip` and `sie`. However, interrupts directed to HS-level continue to be indicated in the HS-level `sip` register, not in `vsip`, when V=1. | `vsip` 和 `vsie` 是 VSXLEN 位读写寄存器，是 VS 模式版本的 `sip` 和 `sie`。V=1 时替代 `sip`/`sie`。但 HS 级中断仍在 HS 级 `sip` 中指示。 |
| `norm:vspec_sz_acc_op` | The `vsepc` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `sepc`. When V=1, `vsepc` substitutes for the usual `sepc`. When V=0, `vsepc` does not directly affect the behavior of the machine. | `vsepc` 是 VSXLEN 位读写寄存器，VS 模式版本的 `sepc`。V=1 时替代 `sepc`；V=0 时不直接影响机器行为。 |
| `norm:vsscratch_sz_acc_op` | The `vsscratch` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `sscratch`. When V=1, `vsscratch` substitutes for the usual `sscratch`. The contents of `vsscratch` never directly affect the behavior of the machine. | `vsscratch` 是 VSXLEN 位读写寄存器，VS 模式版本的 `sscratch`。V=1 时替代 `sscratch`。其内容从不直接影响机器行为。 |
| `norm:vsstatus_fs_op` | When V=1, both `vsstatus`.FS and the HS-level `sstatus`.FS are in effect. Attempts to execute a floating-point instruction when either field is 0 (Off) raise an illegal-instruction exception. Modifying the floating-point state when V=1 causes both fields to be set to 3 (Dirty). | V=1 时，`vsstatus`.FS 和 HS 级 `sstatus`.FS 同时生效。任一为 0 时执行浮点指令引发非法指令异常。V=1 时修改浮点状态使两者都设为 3(Dirty)。 |
| `norm:vsstatus_sd_xs_op` | Read-only fields SD and XS summarize the extension context status as it is visible to VS-mode only. For example, the value of the HS-level `sstatus`.FS does not affect `vsstatus`.SD. | 只读字段 SD 和 XS 仅总结对 VS 模式可见的扩展上下文状态。例如 HS 级 `sstatus`.FS 不影响 `vsstatus`.SD。 |
| `norm:vsstatus_sz_acc_op` | The `vsstatus` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `sstatus`. When V=1, `vsstatus` substitutes for the usual `sstatus`, so instructions that normally read or modify `sstatus` actually access `vsstatus` instead. | `vsstatus` 是一个 VSXLEN 位读写寄存器，是 VS 模式版本的 `sstatus`。V=1 时替代 `sstatus`。 |
| `norm:vsstatus_ube` | An implementation may make field UBE be a read-only copy of `hstatus`.VSBE. | 实现可使 UBE 字段为 `hstatus`.VSBE 的只读副本。 |
| `norm:vsstatus_uxl_change` | If VSXLEN is changed from 32 to a wider width, and if field UXL is not restricted to a single value, it gets the value corresponding to the widest supported width not wider than the new VSXLEN. | 若 VSXLEN 从 32 变为更宽宽度且 UXL 未限制为单一值，则取不超过新 VSXLEN 的最宽支持宽度对应值。 |
| `norm:vsstatus_uxl_op` | The UXL field controls the effective XLEN for VU-mode. When VSXLEN=32, the UXL field does not exist, and VU-mode XLEN=32. When VSXLEN=64, UXL is a WARL field. An implementation may make UXL be a read-only copy of field VSXL of `hstatus`, forcing VU-mode XLEN=VSXLEN. | UXL 字段控制 VU 模式的有效 XLEN。VSXLEN=32 时不存在，VU 模式 XLEN=32。VSXLEN=64 时为 WARL 字段。实现可使 UXL 为 `hstatus`.VSXL 的只读副本。 |
| `norm:vsstatus_v0` | When V=0, `vsstatus` does not directly affect the behavior of the machine, unless a virtual-machine load/store (HLV, HLVX, or HSV) or the MPRV feature in the `mstatus` register is used to execute a load or store as though V=1. | V=0 时 `vsstatus` 不直接影响机器行为，除非使用虚拟机 load/store 或 `mstatus` 的 MPRV 功能以 V=1 方式执行访问。 |
| `norm:vsstatus_vs_op` | Similarly, when V=1, both `vsstatus`.VS and the HS-level `sstatus`.VS are in effect. Attempts to execute a vector instruction when either field is 0 (Off) raise an illegal-instruction exception. Modifying the vector state when V=1 causes both fields to be set to 3 (Dirty). | 类似地，V=1 时 `vsstatus`.VS 和 HS 级 `sstatus`.VS 同时生效。任一为 0 时执行向量指令引发非法指令异常。V=1 时修改向量状态使两者都设为 3(Dirty)。 |
| `norm:vstimecmp_acc` | In RV32 only, accesses to the `vstimecmp` CSR access the low 32 bits, while accesses to the `vstimecmph` CSR access the high 32 bits of `vstimecmp`. | 仅在 RV32 中，访问 `vstimecmp` CSR 访问低 32 位，访问 `vstimecmph` CSR 访问高 32 位。 |
| `norm:vstimecmp_sz` | The `vstimecmp` CSR is a 64-bit register and has 64-bit precision on all RV32 and RV64 systems. | `vstimecmp` 是一个 64 位寄存器，在所有 RV32 和 RV64 系统上都有 64 位精度。 |
| `norm:vstval_sz_acc_op` | The `vstval` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `stval`. When V=1, `vstval` substitutes for the usual `stval`. When V=0, `vstval` does not directly affect the behavior of the machine. | `vstval` 是 VSXLEN 位读写寄存器，VS 模式版本的 `stval`。V=1 时替代 `stval`；V=0 时不直接影响机器行为。 |
| `norm:vstval_warl` | `vstval` is a WARL register that must be able to hold the same set of values that `stval` can hold. | `vstval` 是一个 WARL 寄存器，必须能保持与 `stval` 相同的值集合。 |
| `norm:vsxl_ro` | In particular, an implementation may make VSXL be a read-only field whose value always ensures that VSXLEN=HSXLEN. | 实现可以使 VSXL 为只读字段，其值始终确保 VSXLEN=HSXLEN。 |
| `norm:vtw_virtinstr` | An implementation may have WFI always raise a virtual-instruction exception in VS-mode when VTW=1 (and `mstatus`.TW=0), even if there are pending globally-disabled interrupts when the instruction is executed. | 当 VTW=1（且 `mstatus`.TW=0）时，实现可以使 WFI 在 VS 模式下始终引发虚拟指令异常，即使执行时存在全局禁用的待处理中断。 |

---
## Group 1. V=1 时 CSR 替代机制

**规范依据**：
- `norm:H_vscsrs_sub`：V=1 时 VS CSR 替代对应 S CSR，指令访问 S CSR 实际访问 VS CSR
- `norm:H_vscsrs_acc_vs`：V=1 时按 VS CSR 自身地址直接访问触发 virtual-instruction exception
- `norm:H_vscsrs_acc_u`：U-mode 访问触发 illegal-instruction exception
- `norm:H_vscsrs_acc_m_hs`：VS CSR 仅 M/HS-mode 可按自身地址访问
- `norm:H_vscsrs_v1`：V=1 时 HS-level S CSR 保留值但不影响行为
- `norm:H_vscsrs_v0`：V=0 时 VS CSR 不影响行为
- `norm:H_scsrs_nomatch`：无匹配 VS CSR 的 S CSR（senvcfg/scounteren/scontext）在 V=1 时的行为

**测试职责**：验证 V=1/V=0 两种模式下 VS CSR 的替代、访问控制和隔离行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VCSR-01 | V=1 时 sstatus 访问 vsstatus | HS-mode 写 vsstatus 特定值，切入 VS-mode 用 csrr sstatus 读 | 读到 vsstatus 的值 |
| VCSR-02 | V=1 时写 sstatus 实际写 vsstatus | VS-mode 用 csrw sstatus 写值，返回 HS-mode 用 csrr vsstatus 读 | vsstatus 被修改 |
| VCSR-03 | V=1 时 HS-level sstatus 保留 | HS-mode 设置 sstatus 特定值，切入 VS-mode 修改 sstatus，返回后检查 HS-level sstatus | HS-level sstatus 不受 VS-mode 修改影响 |
| VCSR-04 | V=1 时直接访问 VS CSR 地址触发异常 | VS-mode 用 csrr 直接访问 vsstatus 的 CSR 地址（0x200） | virtual-instruction exception (cause=22) |
| VCSR-05 | U-mode 访问 VS CSR 地址触发异常 | U-mode 尝试 csrr vsstatus | illegal-instruction exception (cause=2) |
| VCSR-06 | M-mode 直接访问 VS CSR | M-mode 用 csrr/csrw 直接访问 vsstatus | 正常读写成功 |
| VCSR-07 | HS-mode 直接访问 VS CSR | HS-mode 用 csrr/csrw 直接访问 vsstatus | 正常读写成功 |
| VCSR-08 | V=0 时 VS CSR 不影响行为 | HS-mode 写 vsstatus.SIE=0，检查 HS-mode 自身中断使能 | HS-mode 中断使能不受 vsstatus.SIE 影响 |
| VCSR-09 | V=1 时 sepc 访问 vsepc | HS-mode 写 vsepc=0xDEAD，VS-mode csrr sepc | 读到 0xDEAD（WARL 截断后） |
| VCSR-10 | V=1 时 scause 访问 vscause | 类似 VCSR-09，验证 scause/vscause 替代 | 替代生效 |
| VCSR-11 | V=1 时 stval 访问 vstval | 类似 VCSR-09，验证 stval/vstval 替代 | 替代生效 |
| VCSR-12 | V=1 时 stvec 访问 vstvec | 类似 VCSR-09，验证 stvec/vstvec 替代 | 替代生效 |
| VCSR-13 | V=1 时 sscratch 访问 vsscratch | 类似 VCSR-09，验证 sscratch/vsscratch 替代 | 替代生效 |
| VCSR-14 | V=1 时 sip/sie 访问 vsip/vsie | HS-mode 配置 vsip/vsie，VS-mode 读 sip/sie | 读到 vsip/vsie 的值 |
| VCSR-15 | V=1 时 satp 访问 vsatp | HS-mode 写 vsatp，VS-mode csrr satp | 读到 vsatp 的值 |
| VCSR-16 | 无匹配 VS CSR 的 senvcfg 在 V=1 时功能正常 | V=1 时 VS-mode 读写 senvcfg，验证该 CSR 直接生效而非被替代 | senvcfg 读写正常，hypervisor 需手动 swap |
| VCSR-17 | 无匹配 VS CSR 的 scounteren 在 V=1 时功能正常 | V=1 时设置 scounteren，验证 VU-mode 计数器访问受其控制 | scounteren 控制 VU-mode 计数器可见性 |

---

##  Group 2. hstatus 寄存器

**规范依据**：
- `norm:hstatus_sz_acc_op`：HSXLEN-bit 读写寄存器
- `norm:hstatus_vsxl_op` / `norm:hstatus_vsxl_32` / `norm:hstatus_vsxl_64` / `norm:vsxl_ro`：VSXL 字段
- `norm:hstatus_vtsr_op`：VTSR=1 时 VS-mode SRET 触发 virtual-instruction exception
- `norm:hstatus_vtw_op` / `norm:vtw_virtinstr`：VTW=1 时 VS-mode WFI 触发 virtual-instruction exception
- `norm:hstatus_vtvm_op`：VTVM=1 时 VS-mode 访问 satp/SFENCE.VMA/SINVAL.VMA 触发 virtual-instruction exception
- `norm:hstatus_vgein_op`：VGEIN 字段选择 guest external interrupt source
- `norm:hstatus_hu_op`：HU 字段控制 U-mode 下 HLV/HLVX/HSV 使能
- `norm:hstatus_spv_op` / `norm:hstatus_spv_sret`：SPV 字段
- `norm:hstatus_spvp_op`：SPVP 字段
- `norm:hstatus_gva_op`：GVA 字段
- `norm:hstatus_vsbe_op`：VSBE 字段

**测试职责**：验证 hstatus 各字段在 trap、SRET、VS-mode 执行等场景下的正确行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HSTAT-01 | hstatus 基本读写 | HS-mode 写 hstatus 各可写字段，读回验证 | 可写字段读回一致，WPRI 字段为零 |
| HSTAT-02 | VTSR=1 VS-mode SRET 触发异常 | 设置 hstatus.VTSR=1，VS-mode 执行 SRET | virtual-instruction exception (cause=22) |
| HSTAT-03 | VTSR=0 VS-mode SRET 正常 | 设置 hstatus.VTSR=0，VS-mode 执行 SRET | SRET 正常执行 |
| HSTAT-04 | VTW=1 VS-mode WFI 触发异常 | 设置 hstatus.VTW=1，mstatus.TW=0，VS-mode 执行 WFI | virtual-instruction exception (cause=22) |
| HSTAT-05 | VTW=0 VS-mode WFI 正常 | hstatus.VTW=0，VS-mode 执行 WFI | WFI 正常执行（或超时完成） |
| HSTAT-06 | mstatus.TW=1 覆盖 VTW | mstatus.TW=1，hstatus.VTW=0，VS-mode 执行 WFI | illegal-instruction exception (cause=2) |
| HSTAT-07 | VTVM=1 VS-mode 读 satp 触发异常 | 设置 hstatus.VTVM=1，VS-mode csrr satp | virtual-instruction exception (cause=22) |
| HSTAT-08 | VTVM=1 VS-mode SFENCE.VMA 触发异常 | 设置 hstatus.VTVM=1，VS-mode 执行 SFENCE.VMA | virtual-instruction exception (cause=22) |
| HSTAT-09 | VTVM=1 VS-mode SINVAL.VMA 触发异常 | 设置 hstatus.VTVM=1，VS-mode 执行 SINVAL.VMA | virtual-instruction exception (cause=22) |
| HSTAT-10 | VTVM=0 VS-mode 访问 satp 正常 | hstatus.VTVM=0，VS-mode csrr/csrw satp | 正常访问（实际访问 vsatp） |
| HSTAT-11 | HU=1 U-mode 执行 HLV | 设置 hstatus.HU=1，U-mode 执行 HLV.W | 正常执行 |
| HSTAT-12 | HU=0 U-mode 执行 HLV 触发异常 | 设置 hstatus.HU=0，U-mode 执行 HLV.W | illegal-instruction exception (cause=2) |
| HSTAT-13 | SPV 在 trap 时写入正确 | 从 VS-mode 触发 trap 到 HS-mode | hstatus.SPV=1 |
| HSTAT-14 | SPV 在 trap 时写入正确（从 U-mode） | 从 U-mode 触发 trap 到 HS-mode | hstatus.SPV=0 |
| HSTAT-15 | SRET 时 V 设为 SPV | 设置 hstatus.SPV=1，HS-mode 执行 SRET | V 变为 1，进入 VS-mode |
| HSTAT-16 | SPVP 在 V=1 trap 时写入 | 从 VS-mode（S 级）触发 trap 到 HS-mode | hstatus.SPVP=1 |
| HSTAT-17 | SPVP 在 V=1 trap 时写入（VU-mode） | 从 VU-mode 触发 trap 到 HS-mode | hstatus.SPVP=0 |
| HSTAT-18 | SPVP 在 V=0 trap 时不变 | V=0 时触发 trap 到 HS-mode | hstatus.SPVP 保持不变 |
| HSTAT-19 | SPVP 控制 HLV/HSV effective privilege | 设 hstatus.SPVP=0，执行 HLV；设 SPVP=1 再执行 | SPVP=0 时按 VU-mode，SPVP=1 时按 VS-mode |
| HSTAT-20 | GVA 在 guest-page-fault 时设为 1 | VS-mode 触发 guest-page-fault trap 到 HS-mode | hstatus.GVA=1 |
| HSTAT-21 | GVA 在非地址 fault 时设为 0 | VS-mode 触发 ecall trap 到 HS-mode | hstatus.GVA=0 |
| HSTAT-22 | GVA 在 page-fault 时设为 1（V=1） | VS-mode 触发 page-fault（VS-stage） | hstatus.GVA=1 |
| HSTAT-23 | GVA 在 HLV fault 时 SPV=0 但 GVA=1 | HS-mode 执行 HLV 触发 guest-page-fault | hstatus.SPV=0，hstatus.GVA=1 |
| HSTAT-24 | VSBE 字段读写 | 写 hstatus.VSBE=0/1 并读回 | WARL 行为正确（可能只读固定值） |
| HSTAT-25 | VSXL 字段读写（HSXLEN=64） | 写 hstatus.VSXL 并读回 | WARL 行为正确 |
| HSTAT-26 | VGEIN 字段读写 | 写 hstatus.VGEIN=合法值并读回 | WLRL，值在 0 到 GEILEN 之间 |

---

## Group 3. hedeleg / hideleg 委托寄存器

**规范依据**：
- `norm:hedeleg_sz_acc`：hedeleg 为 64-bit 读写寄存器
- `norm:hideleg_sz_acc`：hideleg 为 HSXLEN-bit 读写寄存器
- `norm:hedeleg_op`：V=1 时被 medeleg 委托的异常，若 hedeleg 对应 bit 置位则进一步委托到 VS-mode
- `norm:hedeleg_acc`：hedeleg 各 bit 的可写/只读约束（hedeleg-bits 表）
- `norm:hideleg_op`：被 mideleg 委托的中断，若 hideleg 对应 bit 置位则委托到 VS-mode
- `norm:hideleg_acc`：hideleg bits 10/6/2 可写，bits 12/9/5/1 只读零
- `norm:hideleg_trans`：VS-level 中断号翻译（10→9, 6→5, 2→1）

**测试职责**：验证异常/中断的委托链路和 CSR 字段约束。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| DELEG-01 | hedeleg 可写 bit 验证 | 逐 bit 写 hedeleg，读回验证可写/只读属性 | bits 1-8,12,13,15,18 可写；bits 9-11,16,19-23 只读零 |
| DELEG-02 | hedeleg bit 0 可写性依赖 IALIGN | 写 hedeleg bit 0 并读回 | IALIGN=32 时可写，否则只读零 |
| DELEG-03 | hideleg 可写 bit 验证 | 写 hideleg bits 0-15，读回验证 | bits 10/6/2 可写，bits 12/9/5/1 只读零 |
| DELEG-04 | hedeleg 委托 illegal-instruction 到 VS | 设 medeleg[2]=1, hedeleg[2]=1，VS-mode 触发 illegal instruction | trap 进入 VS-mode（vscause=2） |
| DELEG-05 | hedeleg 未委托时 trap 到 HS | 设 medeleg[2]=1, hedeleg[2]=0，VS-mode 触发 illegal instruction | trap 进入 HS-mode（scause=2） |
| DELEG-06 | hedeleg 委托 breakpoint 到 VS | 设 medeleg[3]=1, hedeleg[3]=1，VS-mode 执行 EBREAK | trap 进入 VS-mode（vscause=3） |
| DELEG-07 | hedeleg 委托 ecall-from-VU 到 VS | 设 medeleg[8]=1, hedeleg[8]=1，VU-mode 执行 ECALL | trap 进入 VS-mode（vscause=8） |
| DELEG-08 | hideleg 委托 VSSI 到 VS | 设 hideleg[2]=1，注入 VSSIP，VS-mode 使能中断 | trap 进入 VS-mode（vscause=1，经翻译） |
| DELEG-09 | hideleg 委托 VSTI 到 VS | 设 hideleg[6]=1，注入 VSTIP，VS-mode 使能中断 | trap 进入 VS-mode（vscause=5，经翻译） |
| DELEG-10 | hideleg 委托 VSEI 到 VS | 设 hideleg[10]=1，注入 VSEIP，VS-mode 使能中断 | trap 进入 VS-mode（vscause=9，经翻译） |
| DELEG-11 | hideleg 未委托时中断 trap 到 HS | 设 hideleg[2]=0，注入 VSSIP | trap 进入 HS-mode |
| DELEG-12 | 中断号翻译验证 VSSI→SSI | hideleg[2]=1 委托 VSSIP 到 VS-mode | vscause 记录 cause=1（非 2） |
| DELEG-13 | 中断号翻译验证 VSTI→STI | hideleg[6]=1 委托 VSTIP 到 VS-mode | vscause 记录 cause=5（非 6） |
| DELEG-14 | 中断号翻译验证 VSEI→SEI | hideleg[10]=1 委托 VSEIP 到 VS-mode | vscause 记录 cause=9（非 10） |
| DELEG-15 | guest-page-fault 不可委托到 VS | 验证 hedeleg bits 20/21/23 只读零 | guest-page-fault 始终 trap 到 HS-mode |
| DELEG-16 | virtual-instruction 不可委托到 VS | 验证 hedeleg bit 22 只读零 | virtual-instruction exception 始终 trap 到 HS-mode |

---

## Group 4. hvip / hip / hie 中断寄存器

**规范依据**：
- `norm:hvip_sz_op` / `norm:hvip_acc`：hvip VSEIP/VSTIP/VSSIP 可写
- `norm:hip_hie_sz_acc` / `norm:hip_op` / `norm:hie_op`：hip 指示 pending，hie 指示 enable
- `norm:sie_hip_hie_mutex`：sie 可写 bit 在 hip/hie 中只读零，反之亦然
- `norm:hideleg_hs`：中断 trap 到 HS-mode 的条件
- `norm:hip_vseip_vseie_op`：VSEIP = hvip.VSEIP OR hgeip[VGEIN] OR 平台信号
- `norm:hip_vstip_vstie_acc_op`：VSTIP = hvip.VSTIP OR vstimecmp 触发
- `norm:hip_vssip_vssie_op`：hip.VSSIP 是 hvip.VSSIP 的 alias
- `norm:hsint_priority`：HS-mode 中断优先级 SEI > SSI > STI > SGEI > VSEI > VSSI > VSTI > LCOFI

**测试职责**：验证中断注入、pending/enable 机制、优先级和互斥关系。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HINT-01 | hvip.VSSIP 写入注入 VS 软件中断 | HS-mode 写 hvip.VSSIP=1，配置 hideleg/hie/vsie | VS-mode 收到 VS software interrupt |
| HINT-02 | hvip.VSTIP 写入注入 VS 时钟中断 | HS-mode 写 hvip.VSTIP=1，配置 hideleg/hie/vsie | VS-mode 收到 VS timer interrupt |
| HINT-03 | hvip.VSEIP 写入注入 VS 外部中断 | HS-mode 写 hvip.VSEIP=1，配置 hideleg/hie/vsie | VS-mode 收到 VS external interrupt |
| HINT-04 | hip.VSSIP 是 hvip.VSSIP 的 alias | 写 hvip.VSSIP=1，读 hip.VSSIP | hip.VSSIP=1 |
| HINT-05 | hip.VSEIP 只读（多源 OR） | 写 hvip.VSEIP=1，读 hip.VSEIP | hip.VSEIP=1（只读） |
| HINT-06 | hip.VSTIP 只读 | 尝试直接写 hip.VSTIP | 写入被忽略 |
| HINT-07 | hie VSEIE/VSTIE/VSSIE 可写 | 写 hie 的 VSEIE/VSTIE/VSSIE bits | 正常读写 |
| HINT-08 | sie 与 hip/hie 互斥 | 检查 sie 可写 bit 在 hip/hie 中是否只读零 | 互斥关系成立 |
| HINT-09 | 清除 hvip.VSSIP 清除中断 | 写 hvip.VSSIP=0 | VS software interrupt 被清除 |
| HINT-10 | hideleg=0 时中断 trap 到 HS | hideleg[2]=0，注入 VSSIP | 中断 trap 到 HS-mode |
| HINT-11 | hideleg=1 时中断 trap 到 VS | hideleg[2]=1，注入 VSSIP | 中断 trap 到 VS-mode |
| HINT-12 | HS-mode 中断优先级 SEI > SSI | 同时 pending SEI 和 SSI | SEI 先被处理 |
| HINT-13 | HS-mode 中断优先级 VSEI > VSSI > VSTI | 同时 pending 多个 VS 中断 | 按 VSEI > VSSI > VSTI 顺序 |
| HINT-14 | hip/hie 非标准 bit 只读零 | 读 hip/hie 的保留 bit | 均为零 |

---

## Group 5. hgeip / hgeie 寄存器

**规范依据**：
- `norm:hgeip_sz_acc_op`：hgeip 为 HSXLEN-bit 只读寄存器
- `norm:hgeie_sz_acc_op`：hgeie 为 HSXLEN-bit 读写寄存器
- `norm:hgeip_hgeie_fields`：guest external interrupt number i 对应 bit i
- `norm:geilen`：GEILEN 数量不确定，可以为零
- `norm:hgeie_op`：hgeie 选择触发 SGEI 的子集，不影响 VGEIN 选择的 VS-level 外部中断

**测试职责**：验证 guest external interrupt 寄存器的字段约束与基本功能。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HGEI-01 | hgeip 只读验证 | 尝试写 hgeip | 写入被忽略 |
| HGEI-02 | hgeie 可写 bit 范围 | 写 hgeie 全 1，读回确定 GEILEN | bits GEILEN:1 可写，bit 0 只读零 |
| HGEI-03 | hgeie bit 0 只读零 | 写 hgeie bit 0 = 1 | bit 0 读回 0 |
| HGEI-04 | hgeip AND hgeie 非零触发 SGEIP | 若 GEILEN>0，配置 hgeie 使能对应 bit，触发 guest external interrupt | hip.SGEIP=1 |
| HGEI-05 | GEILEN=0 时 hgeie/hgeip 全零 | 若 GEILEN=0，读 hgeie/hgeip | 全零 |

---

## Group 6. henvcfg 寄存器

**规范依据**：
- `norm:henvcfg_sz_acc_op`：64-bit 读写寄存器
- `norm:henvcfg_fiom_op` / `norm:henvcfg_fiom_order`：FIOM 字段修改 V=1 时 FENCE 行为
- `norm:henvcfg_pbmte_op`：PBMTE 控制 VS-stage Svpbmt
- `norm:henvcfg_adue_op`：ADUE 控制 VS-stage A/D 硬件更新
- `norm:henvcfg_stce`：STCE 使能 vstimecmp
- `norm:henvcfg_cbze` / `norm:henvcfg_cbcfe` / `norm:henvcfg_cbie`：CBO 指令控制
- `norm:henvcfg_pmm_op`：PMM 控制 VS-mode 指针遮蔽
- `norm:henvcfg_lpe_op`：LPE 控制 VS-mode Zicfilp
- `norm:henvcfg_sse_op`：SSE 控制 VS-mode Zicfiss
- `norm:henvcfg_dte_op`：DTE 控制 VS-mode Ssdbltrp

**测试职责**：验证 henvcfg 各字段在 V=1 时对 VS-mode 执行环境的控制效果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HENV-01 | henvcfg 基本读写 | 写 henvcfg 各字段并读回 | 已实现字段可读写，未实现字段只读零 |
| HENV-02 | FIOM=1 修改 FENCE 行为 | V=1 时 henvcfg.FIOM=1，VS-mode 执行 FENCE 含 PI/PO | FENCE 隐含 PR/PW（I/O 蕴含 Memory） |
| HENV-03 | FIOM=0 不修改 FENCE | V=1 时 henvcfg.FIOM=0，VS-mode 执行同样 FENCE | FENCE 行为不变 |
| HENV-04 | PBMTE=1 启用 VS-stage Svpbmt | 设 henvcfg.PBMTE=1，VS-stage PTE 使用 PBMT 字段 | PTE PBMT 字段生效 |
| HENV-05 | PBMTE=0 禁用 VS-stage Svpbmt | 设 henvcfg.PBMTE=0，VS-stage PTE 使用 PBMT 字段 | 实现行为如同 Svpbmt 不存在 |
| HENV-06 | ADUE=1 启用 VS-stage A/D 硬件更新 | henvcfg.ADUE=1，VS-stage PTE A=0 | 硬件自动设置 A bit |
| HENV-07 | ADUE=0 禁用 VS-stage A/D 硬件更新 | henvcfg.ADUE=0，VS-stage PTE A=0 | page fault（Svade 行为） |
| HENV-08 | STCE=1 使能 vstimecmp | henvcfg.STCE=1，VS-mode 访问 stimecmp | 正常访问（实际访问 vstimecmp） |
| HENV-09 | STCE=0 禁用 vstimecmp | henvcfg.STCE=0，VS-mode 访问 stimecmp | virtual-instruction exception |
| HENV-10 | CBZE=1 使能 CBO.ZERO | henvcfg.CBZE=1，VS-mode 执行 CBO.ZERO | 正常执行 |
| HENV-11 | CBZE=0 禁用 CBO.ZERO | henvcfg.CBZE=0，VS-mode 执行 CBO.ZERO | virtual-instruction exception |
| HENV-12 | CBCFE=1 使能 CBO.CLEAN/FLUSH | henvcfg.CBCFE=1，VS-mode 执行 CBO.CLEAN | 正常执行 |
| HENV-13 | CBCFE=0 禁用 CBO.CLEAN/FLUSH | henvcfg.CBCFE=0，VS-mode 执行 CBO.CLEAN | virtual-instruction exception |
| HENV-14 | CBIE=01 CBO.INVAL 执行 flush | henvcfg.CBIE=01，VS-mode 执行 CBO.INVAL | 执行 flush 操作 |
| HENV-15 | CBIE=00 禁用 CBO.INVAL | henvcfg.CBIE=00，VS-mode 执行 CBO.INVAL | virtual-instruction exception |
| HENV-16 | DTE=0 禁用 VS-mode Ssdbltrp | henvcfg.DTE=0，读 vsstatus.SDT | SDT 只读零 |
| HENV-17 | DTE=1 启用 VS-mode Ssdbltrp | henvcfg.DTE=1，写 vsstatus.SDT=1 并读回 | SDT 可写可读 |
| HENV-18 | PBMTE 未实现时只读零 | 若 Svpbmt 未实现，读 henvcfg.PBMTE | 只读零 |
| HENV-19 | STCE 未实现时只读零 | 若 Sstc 未实现，读 henvcfg.STCE | 只读零 |

---

## Group 7. htimedelta 寄存器

**规范依据**：
- `norm:htimedelta_sz_acc_op`：64-bit 读写寄存器，VS/VU-mode 读 time 时返回 time + htimedelta
- `norm:time_htimedelta_req`：若 time CSR 实现，htimedelta 必须实现

**测试职责**：验证时间偏移功能。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HTDLT-01 | htimedelta 基本读写 | HS-mode 写 htimedelta=0x1000，读回 | 读回 0x1000 |
| HTDLT-02 | VS-mode 读 time 包含 delta | 设 htimedelta=N，VS-mode 读 time | 返回值 ≈ 真实 time + N |
| HTDLT-03 | VU-mode 读 time 包含 delta | 设 htimedelta=N，VU-mode 读 time | 返回值 ≈ 真实 time + N |
| HTDLT-04 | htimedelta 大值（负偏移） | 设 htimedelta=0xFFFFFFFFFFFF0000 | VS-mode 读 time 返回值小于实际 time |
| HTDLT-05 | HS-mode 读 time 不含 delta | 设 htimedelta=N，HS-mode 读 time | 返回实际 time（不加 delta） |

---

##  Group 8. vsstatus 寄存器

**规范依据**：
- `norm:vsstatus_sz_acc_op`：VSXLEN-bit 读写寄存器，V=1 时替代 sstatus
- `norm:vsstatus_uxl_op` / `norm:vsstatus_uxl_change`：UXL 字段控制 VU-mode XLEN
- `norm:vsstatus_fs_op`：V=1 时 vsstatus.FS 与 sstatus.FS 同时生效
- `norm:vsstatus_vs_op`：V=1 时 vsstatus.VS 与 sstatus.VS 同时生效
- `norm:vsstatus_sd_xs_op`：SD/XS 仅反映 VS-mode 可见状态
- `norm:vsstatus_ube`：UBE 可能是 hstatus.VSBE 的只读拷贝
- `norm:vsstatus_v0`：V=0 时不直接影响行为（除 HLV/HSV/MPRV）

**测试职责**：验证 vsstatus 各字段的功能行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSST-01 | vsstatus 基本读写 | HS-mode 写 vsstatus 各字段，读回 | 可写字段读回一致 |
| VSST-02 | V=1 时 sstatus 实际访问 vsstatus | 见 VCSR-01/02 | 替代生效 |
| VSST-03 | vsstatus.FS=0 时 FP 指令异常 | V=1 时设 vsstatus.FS=0（sstatus.FS≠0），VS-mode 执行 FP 指令 | illegal-instruction exception |
| VSST-04 | sstatus.FS=0 时 FP 指令异常 | V=1 时设 sstatus.FS=0（vsstatus.FS≠0），VS-mode 执行 FP 指令 | illegal-instruction exception |
| VSST-05 | 两个 FS 都非零时 FP 可执行 | V=1 时 sstatus.FS≠0 且 vsstatus.FS≠0 | FP 指令正常执行 |
| VSST-06 | FP 修改使两个 FS 都变 Dirty | V=1 时执行 FP 写指令 | sstatus.FS=3 且 vsstatus.FS=3 |
| VSST-07 | vsstatus.VS=0 时 Vector 指令异常 | V=1 时设 vsstatus.VS=0（sstatus.VS≠0） | illegal-instruction exception |
| VSST-08 | sstatus.VS=0 时 Vector 指令异常 | V=1 时设 sstatus.VS=0（vsstatus.VS≠0） | illegal-instruction exception |
| VSST-09 | Vector 修改使两个 VS 都变 Dirty | V=1 时执行 Vector 写指令 | sstatus.VS=3 且 vsstatus.VS=3 |
| VSST-10 | vsstatus.SD 仅反映 VS-mode 视角 | V=1 时 sstatus.FS=Dirty 但 vsstatus.FS=Clean | vsstatus.SD 基于 vsstatus 字段计算 |
| VSST-11 | V=0 时 vsstatus 不影响行为 | V=0 时设 vsstatus.SIE=0 | HS-mode 不受影响 |
| VSST-12 | UXL 字段读写 | 写 vsstatus.UXL 并读回 | WARL 行为正确 |

---

## Group 9. vsip / vsie 寄存器

**规范依据**：
- `norm:vsip_vsie_sz_acc_op`：V=1 时替代 sip/sie
- `norm:vsip_vsie_sei`：hideleg[10]=0 时 vsip.SEIP/vsie.SEIE 只读零；否则是 hip.VSEIP/hie.VSEIE 的 alias
- `norm:vsip_vsie_sti`：hideleg[6]=0 时 vsip.STIP/vsie.STIE 只读零；否则是 hip.VSTIP/hie.VSTIE 的 alias
- `norm:vsip_vsie_ssi`：hideleg[2]=0 时 vsip.SSIP/vsie.SSIE 只读零；否则是 hip.VSSIP/hie.VSSIE 的 alias

**测试职责**：验证 vsip/vsie 的替代机制和 alias 关系。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSIE-01 | V=1 时 sip/sie 访问 vsip/vsie | VS-mode 读 sip/sie | 实际读到 vsip/vsie 内容 |
| VSIE-02 | hideleg[10]=1 时 vsip.SEIP 是 hip.VSEIP 的 alias | 设 hideleg[10]=1, hvip.VSEIP=1，VS-mode 读 sip.SEIP | 读到 1 |
| VSIE-03 | hideleg[10]=0 时 vsip.SEIP 只读零 | 设 hideleg[10]=0, hvip.VSEIP=1，VS-mode 读 sip.SEIP | 读到 0 |
| VSIE-04 | hideleg[6]=1 时 vsip.STIP alias | 设 hideleg[6]=1, hvip.VSTIP=1，VS-mode 读 sip.STIP | 读到 1 |
| VSIE-05 | hideleg[6]=0 时 vsip.STIP 只读零 | 设 hideleg[6]=0, hvip.VSTIP=1，VS-mode 读 sip.STIP | 读到 0 |
| VSIE-06 | hideleg[2]=1 时 vsip.SSIP alias | 设 hideleg[2]=1, hvip.VSSIP=1，VS-mode 读 sip.SSIP | 读到 1 |
| VSIE-07 | hideleg[2]=0 时 vsip.SSIP 只读零 | 设 hideleg[2]=0，VS-mode 读 sip.SSIP | 读到 0 |
| VSIE-08 | vsie.SEIE alias 验证 | hideleg[10]=1，VS-mode 写 sie.SEIE=1，HS-mode 读 hie.VSEIE | hie.VSEIE=1 |
| VSIE-09 | vsie.STIE alias 验证 | hideleg[6]=1，VS-mode 写 sie.STIE=1，HS-mode 读 hie.VSTIE | hie.VSTIE=1 |
| VSIE-10 | vsie.SSIE alias 验证 | hideleg[2]=1，VS-mode 写 sie.SSIE=1，HS-mode 读 hie.VSSIE | hie.VSSIE=1 |

---

## Group 10. vstimecmp 寄存器

**规范依据**：
- `norm:vstimecmp_sz`：64-bit 寄存器
- `norm:vstimecmp_acc`：RV32 时分 vstimecmp/vstimecmph 访问高低 32 位
- `norm:hip_vstip_op`：当 (time + htimedelta) >= vstimecmp 时 VSTIP 置位
- `norm:hip_vstip_clear`：写入 vstimecmp 使其大于 (time + htimedelta) 时 VSTIP 清除
- `norm:hip_vstip_enable`：V=1 时按标准中断使能/委托规则处理

**测试职责**：验证 VS timer 中断的触发与清除。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSTC-01 | vstimecmp 基本读写 | HS-mode 写 vstimecmp=0x12345678，读回 | 读回正确值 |
| VSTC-02 | vstimecmp 触发 VSTIP | 设 vstimecmp = (time + htimedelta) 的当前值 - 1 | hip.VSTIP=1 |
| VSTC-03 | vstimecmp 清除 VSTIP | VSTIP=1 后写 vstimecmp = 远大于 (time + htimedelta) 的值 | hip.VSTIP=0 |
| VSTC-04 | VS-mode 通过 stimecmp 访问 vstimecmp | VS-mode csrw stimecmp 写值，HS-mode csrr vstimecmp 读回 | 值一致 |
| VSTC-05 | vstimecmp 中断委托到 VS-mode | hideleg[6]=1, hie.VSTIE=1, vsie.STIE=1，触发 vstimecmp | VS-mode 收到 timer interrupt (cause=5) |
| VSTC-06 | vstimecmp 中断 trap 到 HS-mode | hideleg[6]=0，触发 vstimecmp | HS-mode 收到中断 |
| VSTC-07 | htimedelta 影响 vstimecmp 比较 | 设 htimedelta=N，vstimecmp=time+N | 立即触发 VSTIP |

---

## Group 11. vsscratch / vsepc / vscause / vstval 寄存器

**规范依据**：
- `norm:vsscratch_sz_acc_op`：VSXLEN-bit 读写，V=1 时替代 sscratch
- `norm:vspec_sz_acc_op`：VSXLEN-bit 读写，V=1 时替代 sepc
- `norm:vsepc_warl`：vsepc 是 WARL，与 sepc 持有相同值域
- `norm:vscause_sz_acc_op`：VSXLEN-bit 读写，V=1 时替代 scause
- `norm:vscause_wlrl`：vscause 是 WLRL，与 scause 持有相同值域
- `norm:vstval_sz_acc_op` / `norm:vstval_warl`：V=1 时替代 stval

**测试职责**：验证这些 VS CSR 的读写、WARL/WLRL 约束和 V=0 时的隔离。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSCR-01 | vsscratch 基本读写 | HS-mode 写 vsscratch=0xABCD，读回 | 读回 0xABCD |
| VSCR-02 | vsepc WARL 验证 | HS-mode 写 vsepc=奇数地址（如 0x1001） | 读回值符合 WARL 约束（低位可能被清零） |
| VSCR-03 | vscause WLRL 验证 | HS-mode 写 vscause=合法 cause 值，读回 | 读回正确值 |
| VSCR-04 | vstval WARL 验证 | HS-mode 写 vstval=全 1，读回 | 读回符合 WARL 约束 |
| VSCR-05 | V=1 trap 正确写入 vsepc/vscause/vstval | VS-mode 触发异常，trap 委托到 VS-mode | vsepc=故障 PC, vscause=正确 cause, vstval=正确值 |
| VSCR-06 | V=0 时 VS CSR 不影响行为 | V=0 时写 vsepc/vscause/vstval，检查 HS-mode trap 行为 | HS-mode trap 写入 sepc/scause/stval，不受 VS CSR 影响 |

---

## Group 12. Virtual-Instruction Exception

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

---

## Group 13. Trap Entry 行为

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

## Group 14. Trap Return 行为

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

## Group 15. htinst / mtinst 转换指令

**规范依据**：
- `norm:H_trap_xtinst`：trap 时写入 mtinst/htinst 的值类型（零/转换指令/custom/pseudoinstruction）
- `norm:H_trap_xtinst_interrupt`：中断时写零
- `norm:H_trap_xtinst_val`：各异常类型可写入的值类型（tinst-values 表）
- `norm:H_trap_xtinst_guestpage`：隐式 VS-stage 访问引发 guest-page-fault 且 htval 非零时必须写 pseudoinstruction
- `norm:H_trap_xtinst_guestpage_rw`：read 用 0x00003000，write（A/D 更新）用 0x00003020（RV64）

**测试职责**：验证 htinst 在各种 trap 场景下的写入值。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TINST-01 | 中断 trap 时 htinst=0 | VS-mode 中断 trap 到 HS-mode | htinst=0 |
| TINST-02 | ecall trap 时 htinst=0 或 custom | VS-mode ecall | htinst=0 |
| TINST-03 | load guest-page-fault htinst 值 | VS-mode load 触发 guest-page-fault | htinst 为 0 或转换后的 load 指令 |
| TINST-04 | store guest-page-fault htinst 值 | VS-mode store 触发 guest-page-fault | htinst 为 0 或转换后的 store 指令 |
| TINST-05 | 隐式 VS-stage 访问 fault + htval 非零时 htinst 为 pseudoinstruction | VS-stage 页表所在 GPA 无映射，触发 guest-page-fault | htinst=0x00003000（RV64 read） |
| TINST-06 | 隐式写（A/D 更新）的 pseudoinstruction | 如实现支持 A/D 自动更新，触发隐式写 fault | htinst=0x00003020（RV64 write） |
| TINST-07 | 转换指令 bit 1:0 编码验证 | 32-bit load 触发 fault | htinst bits 1:0 = 11（32-bit 指令） |
| TINST-08 | 压缩指令转换后 bit 1:0 编码 | 16-bit C.LW 触发 fault | htinst bits 1:0 = 01（压缩指令） |
| TINST-09 | page-fault 不产生 pseudoinstruction | VS-mode load page-fault（非 guest-page-fault） | htinst=0 或转换指令（非 pseudoinstruction） |

---

## Group 16. mstatus 增强（Hypervisor 相关）

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

## Group 17. mideleg / mip / mie 增强

**规范依据**：
- `norm:mideleg_acc_h`：mideleg bits 10/6/2 只读 1，bit 12 只读 1（GEILEN>0）
- `norm:mideleg_hroz`：mideleg 为零的 bit，对应 hideleg/hip/hie 只读零
- `norm:mip_mie_vs`：mip/mie 新增 VS 中断 bit
- `norm:mip_mie_alias`：mip 中 SGEIP/VSEIP/VSTIP/VSSIP 是 hip 对应 bit 的 alias

**测试职责**：验证 M-level 中断寄存器的 Hypervisor 增强行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MIDLG-01 | mideleg bits 10/6/2 只读 1 | 尝试写 mideleg 清除 bits 10/6/2 | 这些 bit 保持 1 |
| MIDLG-02 | mideleg bit 12 只读 1（GEILEN>0） | 若 GEILEN>0，尝试清除 mideleg bit 12 | bit 12 保持 1 |
| MIDLG-03 | mideleg=0 的 bit 对应 hideleg 只读零 | mideleg 某 bit=0，尝试写 hideleg 该 bit | hideleg 该 bit 只读零 |
| MIDLG-04 | mip VSSIP 是 hip.VSSIP alias | 写 hvip.VSSIP=1，读 mip.VSSIP | mip.VSSIP=1 |
| MIDLG-05 | mip VSEIP 是 hip.VSEIP alias | 写 hvip.VSEIP=1，读 mip.VSEIP | mip.VSEIP=1 |
| MIDLG-06 | mie VSSIE 是 hie.VSSIE alias | 写 hie.VSSIE=1，读 mie.VSSIE | mie.VSSIE=1 |
| MIDLG-07 | mip/mie 新增 VS 中断 bit 可见 | 读 mip/mie 的 VS 中断 bit 位置 | 相应 bit 可被读取 |

---

## Group 18. mtval2 / mtinst 寄存器（M-mode Trap）

**规范依据**：
- `norm:mtval2_sz_acc_op`：MXLEN-bit 读写寄存器
- `norm:mtval2_trapval`：guest-page-fault trap 到 M-mode 时 mtval2 写入 GPA >> 2 或零
- `norm:mtval2_trapval_vstrans`：隐式 VS-stage 访问导致 guest-page-fault 时 mtval2 写入隐式访问的 GPA
- `norm:mtinst_sz_acc_op` / `norm:mtinst_val`：mtinst 格式与 WARL

**测试职责**：验证 M-mode trap 时 mtval2/mtinst 的写入行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MTVAL-01 | mtval2 基本读写 | M-mode 写 mtval2 并读回 | WARL 行为正确 |
| MTVAL-02 | guest-page-fault trap 到 M-mode 时 mtval2 | VS-mode 触发 guest-page-fault，trap 到 M-mode | mtval2 = GPA >> 2 或 0 |
| MTVAL-03 | 非 guest-page-fault 时 mtval2=0 | VS-mode ecall trap 到 M-mode | mtval2=0 |
| MTVAL-04 | mtinst 基本读写 | M-mode 写 mtinst 并读回 | WARL 行为正确 |
| MTVAL-05 | mtinst 在 M-mode guest-page-fault trap 值 | VS-mode 触发 guest-page-fault，trap 到 M-mode | mtinst = 0 或转换指令/pseudoinstruction |

---

## Group 19. 异常优先级

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
