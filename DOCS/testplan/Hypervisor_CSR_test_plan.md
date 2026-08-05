**中文 | [English](../testplan_en/Hypervisor_CSR_test_plan_en.md)**

# Hypervisor CSR 测试计划（Hypervisor_CSR 子集）

> 本文档拆分自已删除的原 Hypervisor 综合测试计划（Hypervisor_test_plan.md），测试用例编号保持不变，Group 序号为本子集内重新排列。
> 兄弟子集：[Hypervisor_Interrupts_test_plan.md](Hypervisor_Interrupts_test_plan.md) | [Hypervisor_Exceptions_test_plan.md](Hypervisor_Exceptions_test_plan.md)

## 概述

本测试计划覆盖 RISC-V Hypervisor (H) 扩展中 Hypervisor CSR 与 Virtual Supervisor CSR 的寄存器行为，包括 CSR 替代机制、字段 WARL/WLRL 约束、环境配置与时间偏移等功能点。中断递送/委托机制、异常与 trap 行为分别由兄弟子集覆盖。

本测试计划依据 `SPEC/hypervisor.adoc` 中的规范点（norm 标记）编写。

### 本文档覆盖的 SPEC 章节
- Hypervisor and Virtual Supervisor CSRs（hstatus, hedeleg, hideleg, henvcfg, htimedelta CSR 行为）
- Virtual Supervisor CSRs（vsstatus, vsip, vsie, vsscratch, vsepc, vscause, vstval CSR 行为, vstimecmp）

### 由其他测试计划覆盖
- Hypervisor 中断（hvip/hip/hie, hgeip/hgeie, mideleg/mip/mie 增强, hideleg 中断委托） → `Hypervisor_Interrupts_test_plan.md`
- Hypervisor 异常与 trap（virtual-instruction exception, trap entry/return, htinst/mtinst, mstatus 增强, mtval2/mtinst, 异常优先级, hedeleg 异常委托） → `Hypervisor_Exceptions_test_plan.md`
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

本章节列出本文档 Groups 1-9 所有测试组中引用的规范点（norm ID），已去重并按字母顺序排列。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:H_mtval_nrz` | CSR `mtval` must not be read-only zero. | 实现 H 扩展时 CSR `mtval` 不得为只读零。 |
| `norm:hedeleg_acc` | Each bit of `hedeleg` shall be either writable or read-only zero. Many bits of `hedeleg` are required specifically to be writable or zero, as enumerated in the table. Bit 0, corresponding to instruction address-misaligned exceptions, must be writable if IALIGN=32. | `hedeleg` 的每一位要么可写要么为只读零。第 0 位（指令地址未对齐异常）在 IALIGN=32 时必须可写。 |
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
| `norm:hideleg_acc` | Among bits 15:0 of `hideleg`, bits 10, 6, and 2 (corresponding to the standard VS-level interrupts) are writable, and bits 12, 9, 5, and 1 (corresponding to the standard S-level interrupts) are read-only zeros. | `hideleg` 的 15:0 位中，第 10、6、2 位（标准 VS 级中断）可写，第 12、9、5、1 位（标准 S 级中断）为只读零。 |
| `norm:hideleg_sz_acc` | Register `hideleg` is an HSXLEN-bit read/write register. | `hideleg` 是一个 HSXLEN 位读写寄存器。 |
| `norm:hip_vstip_clear` | If the result of this comparison changes, it is guaranteed to be reflected in VSTIP eventually, but not necessarily immediately. The interrupt remains posted until `vstimecmp` becomes greater than (`time` + `htimedelta`), typically as a result of writing `vstimecmp`. | 比较结果变化保证最终反映在 VSTIP 中，但不一定立即。中断保持到 `vstimecmp` 大于 (`time` + `htimedelta`)，通常通过写入 `vstimecmp` 实现。 |
| `norm:hip_vstip_enable` | The interrupt will be taken based on the standard interrupt enable and delegation rules while V=1. | 中断将在 V=1 时根据标准中断使能和委托规则被接收。 |
| `norm:hip_vstip_op` | A virtual supervisor timer interrupt becomes pending, as reflected in the VSTIP bit in the `hip` register, whenever (`time` + `htimedelta`), truncated to 64 bits, contains a value greater than or equal to `vstimecmp`, treating the values as unsigned integers. | 当 (`time` + `htimedelta`) 截断为 64 位后的值大于或等于 `vstimecmp`（无符号比较）时，虚拟 supervisor 定时器中断变为待处理，反映在 `hip` 的 VSTIP 位中。 |
| `norm:H_scsrs_nomatch` | Some standard supervisor CSRs (`senvcfg`, `scounteren`, and `scontext`, possibly others) have no matching VS CSR. These supervisor CSRs continue to have their usual function and accessibility even when V=1, except with VS-mode and VU-mode substituting for HS-mode and U-mode. | 某些标准 supervisor CSR（如 `senvcfg`、`scounteren`、`scontext` 等）没有对应的 VS CSR。这些 CSR 即使在 V=1 时也保持原有功能和可访问性，只是 VS 模式和 VU 模式分别替代 HS 模式和 U 模式。 |
| `norm:H_vscsrs_acc_m_hs` | The VS CSRs can be accessed as themselves only from M-mode or HS-mode. | VS CSR 只能从 M 模式或 HS 模式以其自身地址访问。 |
| `norm:H_vscsrs_acc_u` | Attempts from U-mode cause an illegal-instruction exception as usual. | 来自 U 模式的访问尝试照常引发非法指令异常。 |
| `norm:H_vscsrs_acc_vs` | When V=1, an attempt to read or write a VS CSR directly by its own separate CSR address causes a virtual-instruction exception. | 当 V=1 时，尝试通过 VS CSR 自身的独立 CSR 地址直接读写它会引发虚拟指令异常。 |
| `norm:H_vscsrs_sub` | When V=1, the VS CSRs substitute for the corresponding supervisor CSRs, taking over all functions of the usual supervisor CSRs except as specified otherwise. Instructions that normally read or modify a supervisor CSR shall instead access the corresponding VS CSR. | 当 V=1 时，VS CSR 替代对应的 supervisor CSR，接管其所有功能（除非另有规定）。通常读写 supervisor CSR 的指令将改为访问对应的 VS CSR。 |
| `norm:H_vscsrs_v0` | When V=0, the VS CSRs do not ordinarily affect the behavior of the machine other than being readable and writable by CSR instructions. | 当 V=0 时，VS CSR 通常不影响机器行为，仅可被 CSR 指令读写。 |
| `norm:H_vscsrs_v1` | While V=1, the normal HS-level supervisor CSRs that are replaced by VS CSRs retain their values but do not affect the behavior of the machine unless specifically documented to do so. | 当 V=1 时，被 VS CSR 替代的普通 HS 级 supervisor CSR 保留其值，但不影响机器行为（除非特别说明）。 |
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
| `norm:htimedelta_sz_acc_op` | The `htimedelta` CSR is a 64-bit read/write register that contains the delta between the value of the `time` CSR and the value returned in VS-mode or VU-mode. Reading the `time` CSR in VS or U mode returns the sum of `htimedelta` and the actual value of `time`. | `htimedelta` 是一个 64 位读写寄存器，包含 `time` CSR 的值与 VS/VU 模式下返回值之间的差值。VS/VU 模式下读取 `time` 返回 `htimedelta` 与实际 `time` 值之和。 |
| `norm:time_htimedelta_req` | If the `time` CSR is implemented, `htimedelta` (and `htimedeltah` for XLEN=32) must be implemented. | 若实现了 `time` CSR，则 `htimedelta`（XLEN=32 时还有 `htimedeltah`）必须实现。 |
| `norm:vscause_sz_acc_op` | The `vscause` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `scause`. When V=1, `vscause` substitutes for the usual `scause`. When V=0, `vscause` does not directly affect the behavior of the machine. | `vscause` 是 VSXLEN 位读写寄存器，VS 模式版本的 `scause`。V=1 时替代 `scause`；V=0 时不直接影响机器行为。 |
| `norm:vscause_wlrl` | `vscause` is a WLRL register that must be able to hold the same set of values that `scause` can hold. | `vscause` 是一个 WLRL 寄存器，必须能保持与 `scause` 相同的值集合。 |
| `norm:vsepc_warl` | `vsepc` is a WARL register that must be able to hold the same set of values that `sepc` can hold. | `vsepc` 是一个 WARL 寄存器，必须能保持与 `sepc` 相同的值集合。 |
| `norm:vsip_vsie_lcofi` | Extension Shlcofideleg supports delegating LCOFI interrupts to VS-mode. If implemented, `hideleg` bit 13 is writable; otherwise read-only zero. When bit 13 of `hideleg` is zero, `vsip`.LCOFIP and `vsie`.LCOFIE are read-only zeros. Else, they are aliases of `sip`.LCOFIP and `sie`.LCOFIE. | Shlcofideleg 扩展支持将 LCOFI 中断委托给 VS 模式。若实现，`hideleg` 第 13 位可写；否则只读零。第 13 位为零时，`vsip`.LCOFIP 和 `vsie`.LCOFIE 为只读零；否则为 `sip`/`sie` 中对应位的别名。 |
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
- `norm:H_mtval_nrz`：实现 H 扩展时 mtval 不得为只读零（VCSR-18）

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
| VCSR-18 | mtval 非只读零（H 扩展） | H 扩展存在时写 mtval 模式值与地址值并回读 | 必须保持写入值（`norm:H_mtval_nrz`，无 read-only zero 降级分支） |

---

## Group 2. hstatus 寄存器

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

## Group 3. henvcfg 寄存器

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
| HENV-20 | CBIE=11 CBO.INVAL 执行 invalidate | henvcfg.CBIE=11，VS-mode 执行 CBO.INVAL | 执行 invalidate 操作 |
| HENV-21 | CBIE=10 保留编码 WARL 行为 | 写 CBIE=10（保留值），读回 | WARL：不保留保留编码 10b |
| HENV-22 | PMM 字段读写（Ssnpm） | 写 PMM=00/10/11，读回；写 PMM=01（保留） | 合法值可写，保留值 WARL |
| HENV-23 | LPE/SSE 字段读写 | 写 LPE/SSE=1，读回 | 已实现时可写；未实现时只读零 |
| HENV-24 | PMM 未实现时只读零 | 若 Ssnpm 未实现，读 henvcfg.PMM | 只读零 |
| HENV-25 | CBIE 未实现时只读零 | 若 Zicbom 未实现，读 henvcfg.CBIE | 只读零 |
| HENV-26 | CBCFE 未实现时只读零 | 若 Zicbom 未实现，读 henvcfg.CBCFE | 只读零 |
| HENV-27 | CBZE 未实现时只读零 | 若 Zicboz 未实现，读 henvcfg.CBZE | 只读零 |
| HENV-28 | DTE 未实现时只读零 | 若 Ssdbltrp 未实现，读 henvcfg.DTE | 只读零 |
| HENV-29 | ADUE 未实现时只读零 | 若 Svadu 未实现，读 henvcfg.ADUE | 只读零 |

---

## Group 4. htimedelta 寄存器

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

## Group 5. vsstatus 寄存器

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

## Group 6. vsip / vsie 寄存器

**规范依据**：
- `norm:vsip_vsie_sz_acc_op`：V=1 时替代 sip/sie
- `norm:vsip_vsie_sei`：hideleg[10]=0 时 vsip.SEIP/vsie.SEIE 只读零；否则是 hip.VSEIP/hie.VSEIE 的 alias
- `norm:vsip_vsie_sti`：hideleg[6]=0 时 vsip.STIP/vsie.STIE 只读零；否则是 hip.VSTIP/hie.VSTIE 的 alias
- `norm:vsip_vsie_ssi`：hideleg[2]=0 时 vsip.SSIP/vsie.SSIE 只读零；否则是 hip.VSSIP/hie.VSSIE 的 alias
- `norm:vsip_vsie_lcofi`：Shlcofideleg 扩展，hideleg[13] 控制 vsip.LCOFIP/vsie.LCOFIE 的 alias

**测试职责**：验证 vsip/vsie 的替代机制、alias 关系、读方向只读零 WARL、写方向无效屏蔽、直接 CSR 读写，以及 LCOFI 扩展。

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
| VSIE-11 | hideleg[10]=0 时 vsie.SEIE 只读零 | 设 hie.VSEIE=1, hideleg[10]=0，VS-mode 读 sie.SEIE | 读到 0 |
| VSIE-12 | hideleg[6]=0 时 vsie.STIE 只读零 | 设 hie.VSTIE=1, hideleg[6]=0，VS-mode 读 sie.STIE | 读到 0 |
| VSIE-13 | hideleg[2]=0 时 vsie.SSIE 只读零 | 设 hie.VSSIE=1, hideleg[2]=0，VS-mode 读 sie.SSIE | 读到 0 |
| VSIE-14 | hideleg[10]=0 时 VS 写 sie.SEIE 无效 | hideleg[10]=0，VS-mode 写 sie.SEIE=1，读 hie.VSEIE | hie.VSEIE=0（写无效）|
| VSIE-15 | hideleg[6]=0 时 VS 写 sie.STIE 无效 | hideleg[6]=0，VS-mode 写 sie.STIE=1，读 hie.VSTIE | hie.VSTIE=0（写无效）|
| VSIE-16 | hideleg[2]=0 时 VS 写 sie.SSIE 无效 | hideleg[2]=0，VS-mode 写 sie.SSIE=1，读 hie.VSSIE | hie.VSSIE=0（写无效）|
| VSIE-17 | vsie 直接 CSR 读写验证 | M-mode 写 vsie(0x204) SEIE|STIE|SSIE，读回 | 读回匹配写入值 |
| VSIE-18 | vsip.SSIP M-mode 可写验证 | hideleg[2]=1，M-mode 写 vsip(0x244) SSIP=1，读回 | vsip.SSIP=1 |
| VSIE-19 | vsip alias 链 M-mode 视角验证（VSSI） | hideleg[2]=1, hvip.VSSIP=1，M-mode 读 vsip | vsip.SSIP=1 |
| VSIE-20 | vsip alias 链 M-mode 视角验证（VSEI） | hideleg[10]=1, hvip.VSEIP=1，M-mode 读 vsip | vsip.SEIP=1（bit 9）|
| VSIE-21 | hideleg[13] 可写性探测（Shlcofideleg） | 写 hideleg[13]=1，读回检测 | 位粘滞表示扩展已实现 |
| VSIE-22 | hideleg[13]=0 时 vsip/vsie LCOFI 只读零 | hideleg[13]=0，读 vsip.LCOFIP 和 vsie.LCOFIE | 两者均为 0 |

---

## Group 7. vstimecmp 寄存器

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

## Group 8. vsscratch / vsepc / vscause / vstval 寄存器

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
| VSCR-07 | V=1 CSR 替代（sscratch/sepc/scause/stval） | V=1 时 VS-mode 访问 sscratch/sepc/scause/stval | 实际访问 vsscratch/vsepc/vscause/vstval（替代生效） |

---

## Group 9. hedeleg / hideleg 委托寄存器字段约束

> 本组拆分自原 `Hypervisor_test_plan.md` 的 Group 3，仅保留 CSR 位域属性验证用例（DELEG-01/02/03）；异常委托链路用例见 `Hypervisor_Exceptions_test_plan.md`，中断委托与中断号翻译用例见 `Hypervisor_Interrupts_test_plan.md`。

**规范依据**：
- `norm:hedeleg_sz_acc`：hedeleg 为 64-bit 读写寄存器
- `norm:hideleg_sz_acc`：hideleg 为 HSXLEN-bit 读写寄存器
- `norm:hedeleg_acc`：hedeleg 各 bit 的可写/只读约束（hedeleg-bits 表），bit 0 可写性依赖 IALIGN
- `norm:hideleg_acc`：hideleg bits 10/6/2 可写，bits 12/9/5/1 只读零

**测试职责**：验证 hedeleg/hideleg 委托寄存器的位域 WARL 属性（可写位与只读零位），不涉及 trap 递送行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| DELEG-01 | hedeleg 可写 bit 验证 | 逐 bit 写 hedeleg，读回验证可写/只读属性 | bits 1-8,12,13,15,18 可写；bits 9-11,16,19-23 只读零 |
| DELEG-02 | hedeleg bit 0 可写性依赖 IALIGN | 写 hedeleg bit 0 并读回 | IALIGN=32 时可写，否则只读零 |
| DELEG-03 | hideleg 可写 bit 验证 | 写 hideleg bits 0-15，读回验证 | bits 10/6/2 可写，bits 12/9/5/1 只读零 |
