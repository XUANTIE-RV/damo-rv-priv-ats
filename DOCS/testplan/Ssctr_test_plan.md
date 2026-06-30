# Ssctr 扩展测试计划（Supervisor Mode）

> 本文档描述 Ssctr（Control Transfer Records — Supervisor-level）扩展的测试计划。聚焦于 S-mode 下的 CTR CSR（`sctrctl`/`sctrdepth`/`sctrstatus`）、Entry Registers、SCTRCLR 指令、录制行为、特权级模式转换、外部陷阱、转换类型过滤、周期计数、RAS 仿真、Freeze 行为、State Enable 访问控制和 Custom Extensions。M-mode 层面的 Smctr 行为由 `Smctr_test_plan.md` 覆盖。Hypervisor 相关的 CTR 测试（`vsctrctl`、VS/VU-mode 外部陷阱、虚拟化模式转换、VS-mode Freeze）已迁移至 `Hypervisor_cross_test_plan.md` Group 14。
>
> 生成时间：2026-06-25

---

## 概述

Ssctr 是 RISC-V Control Transfer Records (CTR) 扩展的 Supervisor-level 部分。本质上与 Smctr 相同，但排除了 Machine-level CSR 和行为。Ssctr 提供在寄存器可访问的片上存储中录制有限的控制流转换历史的能力，通过间接 CSR 接口（Sscsrind）访问 CTR 缓冲区。

### 本文档覆盖的 SPEC 章节

- `sctrctl` CSR（Supervisor Control Transfer Records Control Register）
- `sctrdepth` CSR（Supervisor CTR Depth Register）
- `sctrstatus` CSR（Supervisor CTR Status Register）
- Entry Registers（ctrsource / ctrtarget / ctrdata，通过 siselect 0x200-0x2FF 和 sireg* 访问）
- SCTRCLR 指令
- 录制行为（qualified transfers, circular buffer）
- 特权级模式转换（traps / trap returns between enabled/disabled modes）
- 外部陷阱（External Traps — STE 字段，S-mode 视角）
- 转换类型过滤（Transfer Type Filtering）
- 周期计数（Cycle Counting — CCV, CC, CCE, CCM）
- RAS 仿真模式（RAS Emulation Mode）
- Freeze 行为（BPFRZ, LCOFIFRZ，S-mode 视角）
- State Enable 访问控制（S-mode 视角）
- Custom Extensions

### 由其他测试计划覆盖

- M-mode CTR 行为 → `Smctr_test_plan.md`
- `mctrctl` CSR（全部字段） → `Smctr_test_plan.md`
- `mstateen0.CTR` / `hstateen0.CTR` M-mode 控制 → `Smctr_test_plan.md`
- M-mode 录制使能和 MTE 外部陷阱 → `Smctr_test_plan.md`
- Debug Mode 录制抑制 → `Smctr_test_plan.md`
- **Hypervisor × Ssctr 交叉测试**（`vsctrctl` CSR、VS/VU-mode 外部陷阱、虚拟化模式转换、VS-mode Freeze、VS/VU-mode 访问限制） → `Hypervisor_cross_test_plan.md` Group 14

---

## 覆盖的规范点

本章节列出本文档所有测试组中引用的规范点（norm ID），已去重并按字母顺序排列。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:ctr_behavior` | CTR records qualified control transfers. | CTR 录制合格的控制转换。 |
| `norm:ctr_behavior_criteria0` | The current privilege mode is enabled | 当前特权模式已启用录制。 |
| `norm:ctr_behavior_criteria1` | The transfer type is not inhibited | 转换类型未被抑制。 |
| `norm:ctr_behavior_criteria2` | `sctrstatus`.FROZEN is not set | `sctrstatus`.FROZEN 未置位。 |
| `norm:ctr_behavior_criteria3` | The transfer completes/retires | 转换完成/退休。 |
| `norm:ctr_custom_bits` | Any custom CTR extension must be associated with a non-zero value within the designated custom bits in `xctrctl`. When the custom bits hold a non-zero value, the extension may alter standard CTR behavior, and may define new custom status fields. All custom status fields must revert to standard behavior when the custom bits hold zero. | 任何自定义 CTR 扩展必须与 `xctrctl` 中指定的 custom 非零值关联。当 custom 为非零值时，扩展可改变标准行为并定义新的自定义状态字段。custom 为零时所有自定义状态字段必须恢复标准行为。 |
| `norm:ctr_ccounter_ccv` | The CC value is valid only when the Cycle Count Valid (CCV) bit is set. If CCV=0, the CC value might not hold the correct count. The next record will have CCV=0 after a write to `xctrctl`, or execution of SCTRCLR. CCV should additionally be cleared after any other implementation-specific scenarios where active cycles might not be counted. | CC 值仅在 CCV 位置位时有效。CCV=0 时 CC 值可能不正确。写 `xctrctl` 或执行 SCTRCLR 后下一条记录的 CCV=0。 |
| `norm:ctr_ccounter_inc` | The elapsed cycle counter (CtrCycleCounter) increments at the same rate as the `mcycle` counter. Only cycles while CTR is active are counted. The CC field is encoded such that CCE holds 0 if the CtrCycleCounter value is less than 4096, otherwise it holds the index of the most significant one bit minus 11. CCM holds CtrCycleCounter bits CCE+10:CCE-1. | 经过周期计数器与 `mcycle` 同速率递增。仅 CTR active 时计数。CC 字段编码：CtrCycleCounter < 4096 时 CCE=0，否则 CCE 为最高有效位索引减 11。CCM 持有 CtrCycleCounter 位 CCE+10:CCE-1。 |
| `norm:ctr_ccounter_impl` | An implementation that supports cycle counting must implement CCV and all CCM bits, but may implement 0..4 exponent bits in CCE. Unimplemented CCE bits are read-only 0. | 支持周期计数的实现必须实现 CCV 和所有 CCM 位，但可实现 0..4 个 CCE 指数位。未实现的 CCE 位只读零。 |
| `norm:ctr_ccounter_reset` | The CtrCycleCounter is reset on writes to `xctrctl`, and on execution of SCTRCLR. | 写 `xctrctl` 或执行 SCTRCLR 时重置 CtrCycleCounter。 |
| `norm:ctr_ccounter_sat` | The CC value saturates when all implemented bits in CCM and CCE are 1. | 当 CCM 和 CCE 中所有已实现位都为 1 时 CC 值饱和。 |
| `norm:ctr_freeze_bp` | On a breakpoint exception that traps to M-mode or S-mode with `sctrctl`.BPFRZ=1, FROZEN is set by hardware. The breakpoint exception itself is not recorded. | 当 `sctrctl`.BPFRZ=1 且 breakpoint 异常陷阱到 M-mode 或 S-mode 时，硬件设置 FROZEN。breakpoint 异常本身不被录制。 |
| `norm:ctr_freeze_vs` | If the H extension is implemented, freeze behavior for LCOFIs and breakpoint exceptions that trap to VS-mode is determined by the LCOFIFRZ and BPFRZ values, respectively, in `vsctrctl`. This includes virtual LCOFIs pended by a hypervisor. | 若实现 H 扩展，VS-mode 的 LCOFI 和 breakpoint freeze 行为由 `vsctrctl` 中的 LCOFIFRZ 和 BPFRZ 决定，包括 hypervisor 挂起的虚拟 LCOFI。 |
| `norm:ctr_stack` | Such qualified transfers update the Entry Registers at logical entry 0. As a result, older entries are pushed down the stack. If the CTR buffer is full, the oldest recorded entry is lost. | 合格的转换更新 logical entry 0 的 Entry Registers。较老的 entry 被推入栈底。若缓冲区已满，最老的 entry 丢失。 |
| `norm:ctr_ttf_default` | Default CTR behavior, when all transfer type filter bits are unimplemented or 0, is to record all control transfers within enabled privileged modes. By setting transfer type filter bits, software can opt out of recording select transfer types, or opt into recording non-default operations. | 默认行为（所有过滤位为 0）是录制启用特权模式内的所有控制转换。通过设置过滤位，软件可选择退出某些类型的录制，或选择加入非默认操作的录制。 |
| `norm:ctr_ttype0` | Not used by CTR | CTR 不使用（TYPE=0） |
| `norm:ctr_ttype1` | Exception | 异常（TYPE=1） |
| `norm:ctr_ttype2` | Interrupt | 中断（TYPE=2） |
| `norm:ctr_ttype3` | Trap return | 陷阱返回（TYPE=3） |
| `norm:ctr_ttype4` | Not-taken branch | 未取分支（TYPE=4） |
| `norm:ctr_ttype5` | Taken branch | 已取分支（TYPE=5） |
| `norm:ctr_ttype8` | Indirect call | 间接调用（TYPE=8） |
| `norm:ctr_ttype9` | Direct call | 直接调用（TYPE=9） |
| `norm:ctr_ttype10` | Indirect jump (without linkage) | 间接跳转（无链接）（TYPE=10） |
| `norm:ctr_ttype11` | Direct jump (without linkage) | 直接跳转（无链接）（TYPE=11） |
| `norm:ctr_ttype12` | Co-routine swap | 协程交换（TYPE=12） |
| `norm:ctr_ttype13` | Function return | 函数返回（TYPE=13） |
| `norm:ctr_ttype14` | Other indirect jump (with linkage) | 其他间接跳转（带链接）（TYPE=14） |
| `norm:ctr_ttype15` | Other direct jump (with linkage) | 其他直接跳转（带链接）（TYPE=15） |
| `norm:ctr_validbit` | Recorded transfers will set the `ctrsource`.V bit to 1, and will update all implemented record fields. | 录制的转换将 `ctrsource`.V 位设为 1，并更新所有已实现的记录字段。 |
| `norm:ctr_various_jump_enc` | Encodings 8 through 15 refer to various encodings of jump instructions. | 编码 8-15 引用各种跳转指令编码。 |
| `norm:ctr_trap_dd` | Trap from disabled source to disabled target: Not recorded. | 禁用源到禁用目标的陷阱：不录制。 |
| `norm:ctr_trap_de` | Trap from disabled source to enabled target: Recorded, `ctrsource`.PC is 0. | 禁用源到启用目标的陷阱：录制，`ctrsource`.PC 为 0。 |
| `norm:ctr_trap_disabled_src` | Traps from a disabled privilege mode to an enabled privilege mode are partially recorded, such that the `ctrsource`.PC is 0. | 禁用特权级到启用特权级的陷阱被部分录制，`ctrsource`.PC 为 0。 |
| `norm:ctr_trap_disabled_tgt` | Traps from an enabled mode to a disabled mode, known as external traps, are not recorded by default. | 启用模式到禁用模式的陷阱（外部陷阱）默认不录制。 |
| `norm:ctr_trap_ee` | Trap between enabled privilege modes: Recorded. | 启用特权级间的陷阱：正常录制。 |
| `norm:ctr_trap_ed` | Trap from enabled source to disabled target: External trap. Not recorded by default, but see External Traps. | 启用源到禁用目标的陷阱：外部陷阱。默认不录制，参见外部陷阱。 |
| `norm:ctr_trap_enabled` | Traps between enabled privilege modes are recorded as normal. | 启用特权级间的陷阱正常录制。 |
| `norm:ctr_trapret_dd` | Trap return from disabled source to disabled target: Not recorded. | 禁用源到禁用目标的陷阱返回：不录制。 |
| `norm:ctr_trapret_de` | Trap return from disabled source to enabled target: Not recorded. | 禁用源到启用目标的陷阱返回：不录制。 |
| `norm:ctr_trapret_ed` | Trap return from enabled source to disabled target: Recorded, `ctrtarget`.PC is 0. | 启用源到禁用目标的陷阱返回：录制，`ctrtarget`.PC 为 0。 |
| `norm:ctr_trapret_ee` | Trap return between enabled privilege modes: Recorded. | 启用特权级间的陷阱返回：正常录制。 |
| `norm:ctr_trapret_enabled` | Trap returns between enabled privilege modes are recorded as normal. | 启用特权级间的陷阱返回正常录制。 |
| `norm:ctr_trapret_from_disabled` | Trap returns from a disabled mode to an enabled mode are not recorded. | 禁用模式到启用模式的陷阱返回不录制。 |
| `norm:ctr_trapret_to_disabled` | Trap returns from an enabled mode back to a disabled mode are partially recorded, such that `ctrtarget`.PC is 0. | 启用模式到禁用模式的陷阱返回被部分录制，`ctrtarget`.PC 为 0。 |
| `norm:ctrctl_rasemu_op` | When the optional `xctrctl`.RASEMU bit is implemented and set to 1, transfer recording behavior is altered to emulate RAS behavior. | 当可选的 RASEMU 位置位为 1 时，转换录制行为改变为仿真 RAS 行为。 |
| `norm:ctrtarget_misp` | The optional MISP bit is set by the hardware when the recorded transfer is an instruction whose target or taken/not-taken direction was mispredicted by the branch predictor. MISP is read-only 0 when not implemented. | 当录制的转换是被分支预测器误预测目标或方向的指令时，硬件设置 MISP 位。未实现时 MISP 只读零。 |
| `norm:ctrtarget_op` | The `ctrtarget` register contains the target (destination) program counter of the recorded transfer. | `ctrtarget` 寄存器包含录制转换的目标程序计数器。 |
| `norm:ctrtarget_pc_next_br` | For a not-taken branch, `ctrtarget` holds the PC of the next sequential instruction following the branch. | 对于未取分支，`ctrtarget` 持有分支后下一条顺序指令的 PC。 |
| `norm:ctrtarget_sz_acc` | `ctrtarget` is an MXLEN-bit WARL register that must be able to hold all valid virtual or physical addresses that can serve as a `pc`. | `ctrtarget` 是 MXLEN 位 WARL 寄存器，必须能持有所有可作为 `pc` 的有效地址。 |
| `norm:ctrdata_cc` | Cycle Count, composed of the Cycle Count Exponent (CCE, in CC[15:12]) and Cycle Count Mantissa (CCM, in CC[11:0]). | 周期计数，由周期计数指数（CCE，CC[15:12]）和周期计数尾数（CCM，CC[11:0]）组成。 |
| `norm:ctrdata_cc_supported` | The `ctrdata` register may optionally include a count of CPU cycles elapsed since the prior CTR record. | `ctrdata` 寄存器可选地包含自上一条 CTR 记录以来经过的 CPU 周期计数。 |
| `norm:ctrdata_ccv` | Cycle Count Valid. | 周期计数有效位。 |
| `norm:ctrdata_sz_acc` | The `ctrdata` register contains metadata for the recorded transfer. This register must be implemented, though all fields within it are optional. `ctrdata` is a 64-bit register. | `ctrdata` 包含录制转换的元数据。此寄存器必须实现，但其中所有字段可选。`ctrdata` 为 64 位寄存器。 |
| `norm:ctrdata_type` | Identifies the type of the control flow transfer recorded in the entry. Implementations that do not support this field will report 0. | 标识 entry 中录制的控制流转换类型。不支持此字段的实现报告 0。 |
| `norm:ctrdata_undef` | Undefined bits in `ctrdata` are WPRI. Undefined bits must be implemented as read-only 0, unless a custom extension is implemented and enabled. | `ctrdata` 中未定义的位为 WPRI。未定义的位必须实现为只读零，除非实现并启用了自定义扩展。 |
| `norm:ctrsource_ctrtartget_ctrdata_Vbit` | The valid (V) bit is set by the hardware when a transfer is recorded in the selected CTR buffer entry, and implies that data in `ctrsource`, `ctrtarget`, and `ctrdata` is valid for this entry. | 当转换被录制到选定的 CTR 缓冲区 entry 时，硬件设置有效（V）位，表示该 entry 中 `ctrsource`、`ctrtarget` 和 `ctrdata` 的数据有效。 |
| `norm:ctrsource_op` | The `ctrsource` register contains the source program counter, which is the `pc` of the recorded control transfer instruction, or the epc of the recorded trap. | `ctrsource` 寄存器包含源程序计数器，即录制的控制转换指令的 `pc` 或录制的陷阱的 epc。 |
| `norm:ctrctl_dircallinh_op` | Inhibit recording of direct calls. | 抑制直接调用的录制。 |
| `norm:ctrctl_dirjmpinh_op` | Inhibit recording of direct jumps (without linkage). | 抑制直接跳转（无链接）的录制。 |
| `norm:ctrctl_dirljmpinh_op` | Inhibit recording of other direct jumps (with linkage). | 抑制其他直接跳转（带链接）的录制。 |
| `norm:ctrctl_excinh_op` | Inhibit recording of exceptions. | 抑制异常的录制。 |
| `norm:ctrctl_indcallinh_op` | Inhibit recording of indirect calls. | 抑制间接调用的录制。 |
| `norm:ctrctl_indjmpinh_op` | Inhibit recording of indirect jumps (without linkage). | 抑制间接跳转（无链接）的录制。 |
| `norm:ctrctl_indljmpinh_op` | Inhibit recording of other indirect jumps (with linkage). | 抑制其他间接跳转（带链接）的录制。 |
| `norm:ctrctl_intrinh_op` | Inhibit recording of interrupts. | 抑制中断的录制。 |
| `norm:ctrctl_corswapinh_op` | Inhibit recording of co-routine swaps. | 抑制协程交换的录制。 |
| `norm:ctrctl_ntbren_op` | Enable recording of not-taken branches. | 启用未取分支的录制。 |
| `norm:ctrctl_retinh_op` | Inhibit recording of function returns. | 抑制函数返回的录制。 |
| `norm:ctrctl_tkbrinh_op` | Inhibit recording of taken branches. | 抑制已取分支的录制。 |
| `norm:ctrctl_tretinh_op` | Inhibit recording of trap returns. | 抑制陷阱返回的录制。 |
| `norm:exttrap_ctrtarget0` | In records for external traps, the `ctrtarget`.PC is 0. | 外部陷阱的记录中，`ctrtarget`.PC 为 0。 |
| `norm:exttrap_us` | U-mode to S-mode: `sctrctl`.STE | U-mode 到 S-mode 需要 `sctrctl`.STE 置位。 |
| `norm:exttrap_vshs` | VS-mode to HS-mode: `sctrctl`.STE | VS-mode 到 HS-mode 需要 `sctrctl`.STE 置位。 |
| `norm:exttrap_vuhs` | VU-mode to HS-mode: `sctrctl`.STE, `vsctrctl`.STE | VU-mode 到 HS-mode 需要 STE 和 vsctrctl.STE 都置位。 |
| `norm:exttrap_vuvs` | VU-mode to VS-mode: `vsctrctl`.STE | VU-mode 到 VS-mode 需要 `vsctrctl`.STE 置位。 |
| `norm:Ssctr_ctrsource_sz_acc_op` | `ctrsource` is an MXLEN-bit WARL register that must be able to hold all valid virtual or physical addresses that can serve as a `pc`. When XLEN < MXLEN, both explicit writes and implicit writes will be zero-extended. | `ctrsource` 是 MXLEN 位 WARL 寄存器，必须能持有所有可作为 `pc` 的有效地址。XLEN < MXLEN 时，显式和隐式写入都零扩展。 |
| `norm:Ssctr_sctrctl_acc` | Bits 2 and 9 in `sctrctl` are read-only 0. As a result, the M and MTE fields in `mctrctl` are not accessible through `sctrctl`. All other `mctrctl` fields are accessible through `sctrctl`. | `sctrctl` 的 bit 2 和 bit 9 只读零。因此 `mctrctl` 的 M 和 MTE 字段无法通过 `sctrctl` 访问。所有其他字段可通过 `sctrctl` 访问。 |
| `norm:Ssctr_sctrctl_op` | The `sctrctl` register provides supervisor mode access to a subset of `mctrctl`. | `sctrctl` 寄存器提供对 `mctrctl` 子集的 supervisor mode 访问。 |
| `norm:Ssctr_sctrstatus_acc` | Undefined bits in `sctrstatus` are WPRI. Undefined bits must be implemented as read-only 0, unless a custom extension is implemented and enabled. | `sctrstatus` 中未定义的位为 WPRI。必须实现为只读零，除非实现并启用了自定义扩展。 |
| `norm:Ssctr_scope` | The corresponding supervisor-level extension, Ssctr, is essentially identical to Smctr, except that it excludes machine-level CSRs and behaviors not intended to be directly accessible at the supervisor level. | Ssctr 本质上与 Smctr 相同，但排除了不在 supervisor 级别直接访问的 machine-level CSR 和行为。 |
| `norm:Ssctr_transfer_steps` | Recorded transfers are inserted at the write pointer, which is then incremented, while older recorded transfers may be overwritten once the buffer is full. Or the user can enable RAS emulation mode. The source PC, target PC, and some optional metadata are stored for each recorded transfer. | 录制的转换插入写指针处并递增，缓冲区满时旧转换可能被覆写。或用户可启用 RAS 仿真模式。每条记录存储源 PC、目标 PC 和可选元数据。 |
| `norm:Ssctr_vsctrctl_sz_acc_op` | If the H extension is implemented, the `vsctrctl` register is a 64-bit read/write register that is VS-mode's version of `sctrctl`. When V=1, `vsctrctl` substitutes for the usual `sctrctl`. | 若实现 H 扩展，`vsctrctl` 是 64 位读写寄存器，是 VS-mode 版本的 `sctrctl`。V=1 时 `vsctrctl` 替代 `sctrctl`。 |
| `norm:sctrclr_acc` | Any read of `ctrsource`, `ctrtarget`, or `ctrdata` that follows SCTRCLR, such that it precedes the next qualified control transfer, will return the value 0. Further, the first recorded transfer following SCTRCLR will have `ctrdata`.CCV=0. | SCTRCLR 之后、下一条合格转换之前对 entry 寄存器的读取返回 0。SCTRCLR 后的第一条录制记录的 CCV=0。 |
| `norm:sctrclr_exceptions` | SCTRCLR raises an illegal-instruction exception in U-mode, and a virtual-instruction exception in VU-mode, unless CTR state enable access restrictions apply. | SCTRCLR 在 U-mode 触发 illegal-instruction，在 VU-mode 触发 virtual-instruction，除非 CTR state enable 访问限制适用。 |
| `norm:sctrclr_op1` | Zeroes all CTR Entry Registers, for all DEPTH values | 清零所有 CTR Entry Registers（所有 DEPTH 值） |
| `norm:sctrclr_op2` | Zeroes the CTR cycle counter and CCV | 清零 CTR 周期计数器和 CCV |
| `norm:Ssctr_CTR_CSR_interface` | The CTR buffer is accessible through an indirect CSR interface, such that software can specify which logical entry in the buffer it wishes to read or write. Logical entry 0 always corresponds to the youngest recorded transfer. | CTR 缓冲区通过间接 CSR 接口访问。Logical entry 0 始终对应最新录制的转换。 |
| `norm:sctrdepth` | The 32-bit `sctrdepth` register specifies the depth of the CTR buffer. | 32 位 `sctrdepth` 寄存器指定 CTR 缓冲区深度。 |
| `norm:sctrdepth_depth` | It is implementation-specific which DEPTH value(s) are supported. | 支持的 DEPTH 值由实现决定。 |
| `norm:sctrdepth_depth_op0` | WARL field that selects the depth of the CTR buffer. Encodings: 000=16, 001=32, 010=64, 011=128, 100=256, 11x=reserved. | WARL 字段选择 CTR 缓冲区深度。编码：000=16, 001=32, 010=64, 011=128, 100=256, 11x=保留。 |
| `norm:sctrdepth_depth_op1` | The depth of the CTR buffer dictates the number of entries. For a depth of N, the hardware records transfers to entries 0..N-1. All Entry Registers read as '0' and are read-only when the selected entry is in the range N to 255. When the depth is increased, the newly accessible entries contain unspecified but legal values. | CTR 缓冲区深度决定 entry 数量。深度为 N 时，硬件录制到 entry 0..N-1。选择 entry 在 N 到 255 范围时 Entry Registers 读为 0 且只读。深度增加时新可访问 entry 包含未指定但合法的值。 |
| `norm:sctrdepth_mode` | Attempts to access `sctrdepth` from VS-mode or VU-mode raise a virtual-instruction exception, unless CTR state enable access restrictions apply. | 从 VS-mode 或 VU-mode 访问 `sctrdepth` 触发 virtual-instruction 异常，除非 CTR state enable 访问限制适用。 |
| `norm:sctrstatus` | The 32-bit `sctrstatus` register grants access to CTR status information and is updated by the hardware whenever CTR is active. CTR is active when the current privilege mode is enabled for recording and CTR is not frozen. | 32 位 `sctrstatus` 寄存器提供 CTR 状态信息的访问，在 CTR active 时由硬件更新。CTR 在当前特权模式启用录制且未冻结时为 active。 |
| `norm:sctrstatus_frozen_op` | Inhibit transfer recording. | 抑制转换录制。 |
| `norm:sctrstatus_frozen_set` | When `sctrctl`.LCOFIFRZ=1 and a local-counter-overflow interrupt (LCOFI) traps to M-mode or to S-mode, `sctrstatus`.FROZEN is set by hardware. The LCOFI trap itself is not recorded. | 当 LCOFIFRZ=1 且 LCOFI 陷阱到 M-mode 或 S-mode 时，硬件设置 FROZEN。LCOFI 陷阱本身不被录制。 |
| `norm:sctrstatus_wrptr` | WARL field that indicates the physical CTR buffer entry to be written next. It is incremented after new transfers are recorded. For a given CTR depth, WRPTR wraps to 0 on an increment when the value matches depth-1, and to depth-1 on a decrement when the value is 0. Bits above those needed to represent depth-1 are read-only 0. On depth changes, WRPTR holds an unspecified but legal value. | WARL 字段指示下一个要写入的物理 CTR 缓冲区 entry。新转换录制后递增。深度为 depth 时，WRPTR 在 depth-1 时递增回绕到 0，在 0 时递减回绕到 depth-1。高于表示 depth-1 所需位数的位只读零。深度变化时 WRPTR 持有未指定但合法的值。 |
| `norm:siselect_acc_op` | The `siselect` index range 0x200 through 0x2FF is reserved for CTR logical entries 0 through 255. When `siselect` holds a value in this range, `sireg` provides access to `ctrsource`, `sireg2` provides access to `ctrtarget`, and `sireg3` provides access to `ctrdata`. `sireg4`, `sireg5`, and `sireg6` are read-only 0. | `siselect` 索引范围 0x200-0x2FF 保留给 CTR logical entries 0-255。`sireg` 访问 `ctrsource`，`sireg2` 访问 `ctrtarget`，`sireg3` 访问 `ctrdata`。`sireg4`/`sireg5`/`sireg6` 只读零。 |
| `norm:vsctrctl-bpfrz_op` | Set `sctrstatus`.FROZEN on a breakpoint exception that traps to VS-mode. | 在 breakpoint 异常陷阱到 VS-mode 时设置 FROZEN。 |
| `norm:vsctrctl-lcofifrz_op` | Set `sctrstatus`.FROZEN on local-counter-overflow interrupt (LCOFI) that traps to VS-mode. | 在 LCOFI 陷阱到 VS-mode 时设置 FROZEN。 |
| `norm:vsctrctl-ste_op` | Enables recording of traps to VS-mode when S=0. | 当 S=0 时启用陷阱到 VS-mode 的录制。 |
| `norm:vsctr-s_op` | Enable transfer recording in VS-mode. | 启用 VS-mode 的转换录制。 |
| `norm:vsctrctl-u_op` | Enable transfer recording in VU-mode. | 启用 VU-mode 的转换录制。 |
| `norm:vsiselect_op` | When `vsiselect` holds a value in 0x200..0x2FF, the `vsireg*` registers provide access to the same CTR entry register state as the analogous `sireg*` registers. There is not a separate set of entry registers for V=1. | 当 `vsiselect` 在 0x200..0x2FF 时，`vsireg*` 寄存器提供与 `sireg*` 相同的 CTR entry 寄存器状态访问。V=1 时没有单独的 entry 寄存器集。 |

---

## Group 1. sctrctl CSR

**规范依据**：
- `norm:Ssctr_sctrctl_op`：`sctrctl` 提供对 `mctrctl` 子集的 supervisor mode 访问
- `norm:Ssctr_sctrctl_acc`：bits 2（M）和 9（MTE）只读零，其余字段可通过 `sctrctl` 访问

**测试职责**：验证 `sctrctl` 寄存器的读写行为和 M-mode 专有字段的不可访问性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCTR-CTL-01 | sctrctl 基本读写 | S-mode 写 sctrctl 各可写字段，读回验证 | 已实现字段读回一致，未实现字段只读零 |
| SSCTR-CTL-02 | sctrctl.S 字段可写性 | S-mode 写 sctrctl.S=1 后读回 | S 字段必须实现且可写 |
| SSCTR-CTL-03 | sctrctl.U 字段可写性 | S-mode 写 sctrctl.U=1 后读回 | U 字段必须实现且可写 |
| SSCTR-CTL-04 | sctrctl bit 2（M）只读零 | S-mode 尝试写 sctrctl bit 2 = 1 | bit 2 读回 0（M 字段不可通过 sctrctl 访问） |
| SSCTR-CTL-05 | sctrctl bit 9（MTE）只读零 | S-mode 尝试写 sctrctl bit 9 = 1 | bit 9 读回 0（MTE 字段不可通过 sctrctl 访问） |
| SSCTR-CTL-06 | sctrctl.BPFRZ 字段可写性 | S-mode 写 sctrctl.BPFRZ=1 后读回 | BPFRZ 字段必须实现且可写 |
| SSCTR-CTL-07 | sctrctl.STE 字段实现检测 | S-mode 写 sctrctl.STE=1 后读回 | 若实现外部陷阱录制，STE 可写；否则只读零 |
| SSCTR-CTL-08 | sctrctl.RASEMU 字段实现检测 | S-mode 写 sctrctl.RASEMU=1 后读回 | 若实现 RAS 仿真，RASEMU 可写；否则只读零 |
| SSCTR-CTL-09 | sctrctl.LCOFIFRZ 可写性（Sscofpmf） | 若实现 Sscofpmf，写 sctrctl.LCOFIFRZ=1 后读回 | LCOFIFRZ 必须可写 |
| SSCTR-CTL-10 | sctrctl 传输类型过滤位检测 | 逐位写 sctrctl[47:32] 各过滤位，读回确定实现 | 已实现位可写，未实现位只读零 |

---

## Group 2. vsctrctl CSR（H 扩展）

> **本组已迁移**：原 Group 2（VSCTL-01~10）的所有测试用例依赖 Hypervisor 扩展，已迁移至 `Hypervisor_cross_test_plan.md` Group 14.1（HCROSS-SSCTR-01~10）。

---

## Group 3. sctrdepth CSR

**规范依据**：
- `norm:sctrdepth`：32-bit 寄存器指定 CTR 缓冲区深度
- `norm:sctrdepth_depth_op0`：DEPTH WARL 字段编码（000=16, 001=32, 010=64, 011=128, 100=256, 11x=reserved）
- `norm:sctrdepth_depth_op1`：深度决定 entry 数量和可见范围
- `norm:sctrdepth_depth`：支持的 DEPTH 值由实现决定
- `norm:sctrdepth_mode`：VS-mode/VU-mode 访问触发 virtual-instruction

**测试职责**：验证 `sctrdepth` 的 DEPTH 字段编码、实现值检测和访问控制（S-mode 视角）。

> **注意**：Hypervisor 相关的 VS/VU-mode 访问限制测试（原 DEP-07~08）已迁移至 `Hypervisor_cross_test_plan.md` Group 14.2。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCTR-DEP-01 | sctrdepth 基本读写 | S-mode 写 sctrdepth 合法 DEPTH 值后读回 | 支持的 DEPTH 值可写且读回一致 |
| SSCTR-DEP-02 | DEPTH 编码验证 | 依次写 DEPTH=000/001/010/011/100 并读回 | 各编码对应 16/32/64/128/256（仅支持的编码可写） |
| SSCTR-DEP-03 | DEPTH 实现值检测 | 写各 DEPTH 值确定哪些被支持 | 至少一个 DEPTH 值被支持（WARL 行为） |
| SSCTR-DEP-04 | DEPTH 控制 entry 可见范围 | 设 DEPTH=000（depth=16），通过 siselect=0x210 读 logical entry 16 | 读为 0 且只读（超出深度范围） |
| SSCTR-DEP-05 | DEPTH 增加时新 entry 包含合法值 | 先设 DEPTH=000（depth=16），后设 DEPTH=001（depth=32），读 logical entry 16 | entry 16 包含未指定但合法的值（ctrsource.V=0 或 1） |
| SSCTR-DEP-06 | DEPTH 11x 编码保留 | 写 DEPTH=110 或 111 | WARL 行为：写入被拒绝或映射到合法值 |

---

## Group 4. sctrstatus CSR

**规范依据**：
- `norm:sctrstatus`：32-bit 寄存器，硬件在 CTR active 时更新
- `norm:sctrstatus_wrptr`：WRPTR 字段（WARL），指示下一个写入的物理 entry
- `norm:sctrstatus_frozen_op`：FROZEN 字段抑制录制
- `norm:Ssctr_sctrstatus_acc`：未定义位为 WPRI，只读零

**测试职责**：验证 `sctrstatus` 的 WRPTR 和 FROZEN 字段行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCTR-STS-01 | sctrstatus 基本读写 | S-mode 写 sctrstatus 各字段并读回 | 已实现字段读回一致，未定义位只读零 |
| SSCTR-STS-02 | WRPTR 初始值合法性 | 读 sctrstatus.WRPTR | WRPTR 值在 0 到 depth-1 范围内 |
| SSCTR-STS-03 | WRPTR 在录制后递增 | 启用录制（S=1），执行一次控制转换，比较转换前后 WRPTR | WRPTR 递增 1 |
| SSCTR-STS-04 | WRPTR 在 depth-1 时回绕到 0 | 设 depth=16，WRPTR=15，执行一次转换使 WRPTR 递增 | WRPTR 回绕到 0 |
| SSCTR-STS-05 | WRPTR 高位只读零（depth=16） | 设 depth=16（需 4 位表示 0-15），读 WRPTR[7:4] | bits 7:4 只读零 |
| SSCTR-STS-06 | WRPTR 高位只读零（depth=256） | 设 depth=256（需 8 位表示 0-255），读 WRPTR[7:0] | 所有 8 位可用 |
| SSCTR-STS-07 | FROZEN 字段软件可写 | S-mode 写 sctrstatus.FROZEN=1 后读回 | FROZEN 可写且读回 1 |
| SSCTR-STS-08 | FROZEN=1 抑制录制 | sctrctl.S=1，sctrstatus.FROZEN=1，S-mode 执行控制转换 | 转换不被录制 |
| SSCTR-STS-09 | FROZEN=0 不抑制录制 | sctrctl.S=1，sctrstatus.FROZEN=0，S-mode 执行控制转换 | 转换被录制 |
| SSCTR-STS-10 | DEPTH 变化时 WRPTR 合法 | 改变 sctrdepth.DEPTH 值后读 sctrstatus.WRPTR | WRPTR 持有未指定但在新 depth 范围内的合法值 |

---

## Group 5. Entry Registers 与 SCTRCLR 指令

**规范依据**：
- `norm:siselect_acc_op`：siselect 0x200-0x2FF 映射 CTR entries，sireg→ctrsource，sireg2→ctrtarget，sireg3→ctrdata，sireg4/5/6 只读零
- `norm:vsiselect_op`：vsiselect 0x200-0x2FF 通过 vsireg* 访问相同状态
- `norm:Ssctr_CTR_CSR_interface`：logical entry 0 对应最新录制的转换
- `norm:ctrsource_op` / `norm:Ssctr_ctrsource_sz_acc_op`：ctrsource MXLEN-bit WARL
- `norm:ctrtarget_op` / `norm:ctrtarget_sz_acc`：ctrtarget MXLEN-bit WARL
- `norm:ctrdata_sz_acc`：ctrdata 64-bit 寄存器，必须实现但字段可选
- `norm:ctrsource_ctrtartget_ctrdata_Vbit`：V 位表示 entry 有效
- `norm:sctrclr_op1` / `norm:sctrclr_op2`：SCTRCLR 清零所有 entry 和 cycle counter
- `norm:sctrclr_acc`：SCTRCLR 后 entry 读为 0，首条录制 CCV=0
- `norm:sctrclr_exceptions`：U-mode 触发 illegal-instruction，VU-mode 触发 virtual-instruction

**测试职责**：验证 CTR entry 寄存器的间接访问机制、SCTRCLR 指令行为和访问控制（S-mode 视角）。

> **注意**：Hypervisor 相关的 vsireg* 共享状态测试（原 ENT-10）和 VU-mode SCTRCLR 异常测试（原 ENT-18）已迁移至 `Hypervisor_cross_test_plan.md` Group 14.2。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCTR-ENT-01 | siselect 0x200 映射 logical entry 0 | S-mode 设 siselect=0x200，读 sireg | 访问 logical entry 0 的 ctrsource |
| SSCTR-ENT-02 | siselect 0x201 映射 logical entry 1 | S-mode 设 siselect=0x201，读 sireg | 访问 logical entry 1 的 ctrsource |
| SSCTR-ENT-03 | siselect 0x2FF 映射 logical entry 255 | S-mode 设 siselect=0x2FF，读 sireg | 访问 logical entry 255 的 ctrsource |
| SSCTR-ENT-04 | sireg 访问 ctrsource | 设 siselect=0x200，通过 sireg 读写 | sireg 对应 ctrsource |
| SSCTR-ENT-05 | sireg2 访问 ctrtarget | 设 siselect=0x200，通过 sireg2 读写 | sireg2 对应 ctrtarget |
| SSCTR-ENT-06 | sireg3 访问 ctrdata | 设 siselect=0x200，通过 sireg3 读写 | sireg3 对应 ctrdata |
| SSCTR-ENT-07 | sireg4 只读零 | 设 siselect=0x200，读 sireg4 | 返回 0 |
| SSCTR-ENT-08 | sireg5 只读零 | 设 siselect=0x200，读 sireg5 | 返回 0 |
| SSCTR-ENT-09 | sireg6 只读零 | 设 siselect=0x200，读 sireg6 | 返回 0 |
| SSCTR-ENT-11 | logical entry 0 对应最新转换 | 启用录制，执行转换 A 后执行转换 B，读 entry 0 | entry 0 对应转换 B（最新的） |
| SSCTR-ENT-12 | logical entry >= depth 只读零 | 设 depth=16，siselect=0x210（entry 16），读 sireg/sireg2/sireg3 | 均返回 0 |
| SSCTR-ENT-13 | ctrsource.V 位在录制后设为 1 | 启用录制，执行转换，读 entry 0 的 ctrsource | ctrsource.V=1 |
| SSCTR-ENT-14 | SCTRCLR 清零所有 entry | 启用录制并产生若干转换记录，执行 SCTRCLR，读所有 entry | 所有 ctrsource/ctrtarget/ctrdata 为 0 |
| SSCTR-ENT-15 | SCTRCLR 清零 cycle counter | 执行 SCTRCLR 后执行第一次转换，检查 ctrdata.CCV | CCV=0（cycle counter 已重置） |
| SSCTR-ENT-16 | SCTRCLR 后第二次转换 CCV=1 | 执行 SCTRCLR 后执行两次转换，检查第二次转换的 ctrdata.CCV | 若实现 CCV，第二次转换 CCV=1 |
| SSCTR-ENT-17 | U-mode 执行 SCTRCLR 触发异常 | U-mode 执行 SCTRCLR | 触发 illegal-instruction 异常（cause=2） |

---

## Group 6. 基本录制行为

**规范依据**：
- `norm:ctr_behavior`：CTR 录制合格的控制转换
- `norm:ctr_behavior_criteria0` / `criteria1` / `criteria2` / `criteria3`：合格条件
- `norm:ctr_stack`：循环缓冲区行为（entry 下推，满时最老丢失）
- `norm:ctr_validbit`：录制转换设置 ctrsource.V=1
- `norm:ctrsource_op`：ctrsource.PC = 转换指令的 pc 或陷阱的 epc
- `norm:ctrtarget_op`：ctrtarget.PC = 转换目标 PC
- `norm:ctrtarget_misp`：MISP 位（可选）
- `norm:ctrdata_type`：TYPE 字段标识转换类型

**测试职责**：验证 CTR 的基本录制行为、循环缓冲区机制和 entry 字段正确性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCTR-REC-01 | 合格转换录制条件全部满足 | sctrctl.S=1，FROZEN=0，S-mode 执行跳转 | 转换被录制 |
| SSCTR-REC-02 | 当前模式未启用时不录制 | sctrctl.S=0，S-mode 执行跳转 | 转换不被录制 |
| SSCTR-REC-03 | FROZEN=1 时不录制 | sctrctl.S=1，FROZEN=1，S-mode 执行跳转 | 转换不被录制 |
| SSCTR-REC-04 | 循环缓冲区 entry 下推 | 启用录制，连续执行转换 A、B、C | entry 0=C（最新），entry 1=B，entry 2=A |
| SSCTR-REC-05 | 缓冲区满时最老 entry 丢失 | 设 depth=16，连续执行 17 次转换 | entry 0 为第 17 次转换，第 1 次转换不再在缓冲区中 |
| SSCTR-REC-06 | ctrsource.V 位录制后为 1 | 清空缓冲区后执行一次转换，读 entry 0 | ctrsource.V=1 |
| SSCTR-REC-07 | ctrsource.PC 字段正确性 | 执行已知地址的 JAL 指令，检查 entry 0 | ctrsource.PC = JAL 指令的地址 |
| SSCTR-REC-08 | ctrtarget.PC 字段正确性 | 执行 JAL 跳转到已知目标地址，检查 entry 0 | ctrtarget.PC = 跳转目标地址 |
| SSCTR-REC-09 | ctrtarget.MISP 字段（若实现） | 执行一次转换后读 ctrtarget.MISP | 若实现 MISP，返回 0 或 1；若未实现，只读零 |
| SSCTR-REC-10 | ctrdata.TYPE 字段正确性 | 执行 JAL x1（直接调用），检查 entry 0 的 ctrdata.TYPE | TYPE=9（Direct call） |
| SSCTR-REC-11 | ctrdata.TYPE 字段（异常） | S-mode 触发异常，检查 entry 0 的 ctrdata.TYPE | TYPE=1（Exception） |
| SSCTR-REC-12 | ctrdata.TYPE 字段（中断） | S-mode 接收中断，检查 entry 0 的 ctrdata.TYPE | TYPE=2（Interrupt） |
| SSCTR-REC-13 | ctrsource 零扩展（XLEN < MXLEN） | 若 XLEN=32 且 MXLEN=64，写入 ctrsource 低 32 位 | 高 32 位零扩展 |

---

## Group 7. 特权级模式转换（Traps / Trap Returns）

**规范依据**：
- `norm:ctr_trap_enabled` / `norm:ctr_trap_ee`：启用模式间陷阱正常录制
- `norm:ctr_trap_disabled_src` / `norm:ctr_trap_de`：禁用→启用陷阱部分录制（ctrsource.PC=0）
- `norm:ctr_trap_disabled_tgt` / `norm:ctr_trap_ed`：启用→禁用陷阱默认不录制（外部陷阱）
- `norm:ctr_trap_dd`：禁用→禁用不录制
- `norm:ctr_trapret_enabled` / `norm:ctr_trapret_ee`：启用模式间 trap return 正常录制
- `norm:ctr_trapret_to_disabled` / `norm:ctr_trapret_ed`：启用→禁用 trap return 部分录制（ctrtarget.PC=0）
- `norm:ctr_trapret_from_disabled` / `norm:ctr_trapret_de`：禁用→启用 trap return 不录制
- `norm:ctr_trapret_dd`：禁用→禁用不录制

**测试职责**：验证特权级模式转换时 CTR 的录制行为（traps 和 trap returns）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCTR-TRAP-01 | 启用模式间陷阱正常录制 | sctrctl.S=1, sctrctl.U=1，U-mode 触发 ecall 陷阱到 S-mode | 转换被录制，ctrsource.PC=ecall 地址，ctrtarget.PC=trap handler 地址 |
| SSCTR-TRAP-02 | 禁用→启用陷阱部分录制 | sctrctl.S=1, sctrctl.U=0，U-mode 触发陷阱到 S-mode | 转换被录制，ctrsource.PC=0 |
| SSCTR-TRAP-03 | 启用→禁用陷阱默认不录制 | sctrctl.S=0, sctrctl.U=1，U-mode 触发陷阱到 S-mode（S-mode 未启用） | 转换不被录制（外部陷阱，默认不录制） |
| SSCTR-TRAP-04 | 禁用→禁用陷阱不录制 | sctrctl.S=0, sctrctl.U=0，U-mode 触发陷阱到 S-mode | 转换不被录制 |
| SSCTR-TRAP-05 | 启用模式间 trap return 正常录制 | sctrctl.S=1, sctrctl.U=1，S-mode 执行 SRET 返回 U-mode | 转换被录制，ctrtarget.PC=U-mode 恢复地址 |
| SSCTR-TRAP-06 | 启用→禁用 trap return 部分录制 | sctrctl.S=1, sctrctl.U=0，S-mode 执行 SRET 返回 U-mode | 转换被录制，ctrtarget.PC=0 |
| SSCTR-TRAP-07 | 禁用→启用 trap return 不录制 | sctrctl.S=0, sctrctl.U=1，S-mode 执行 SRET 返回 U-mode | 转换不被录制 |
| SSCTR-TRAP-08 | 禁用→禁用 trap return 不录制 | sctrctl.S=0, sctrctl.U=0，S-mode 执行 SRET 返回 U-mode | 转换不被录制 |
| SSCTR-TRAP-09 | 陷阱的 ctrsource.PC 为 epc | sctrctl.S=1, sctrctl.U=1，U-mode 执行 ECALL | entry 的 ctrsource.PC = ECALL 指令地址 |
| SSCTR-TRAP-10 | 陷阱的 ctrtarget.PC 为 handler 地址 | sctrctl.S=1, sctrctl.U=1，U-mode ECALL 陷阱到 S-mode | entry 的 ctrtarget.PC = stvec 中的 trap handler 地址 |

---

## Group 8. 外部陷阱（External Traps）

**规范依据**：
- `norm:exttrap_us`：U→S 需要 `sctrctl`.STE
- `norm:exttrap_vshs`：VS→HS 需要 `sctrctl`.STE
- `norm:exttrap_vuhs`：VU→HS 需要 `sctrctl`.STE + `vsctrctl`.STE
- `norm:exttrap_vuvs`：VU→VS 需要 `vsctrctl`.STE
- `norm:exttrap_ctrtarget0`：外部陷阱记录中 ctrtarget.PC=0
- `norm:ctr_trap_ed`：启用→禁用陷阱默认不录制

**测试职责**：验证 S-mode 的外部陷阱（STE 字段）录制行为。

> **注意**：Hypervisor 相关的 VS/VU-mode 外部陷阱测试（原 EXT-04~09）已迁移至 `Hypervisor_cross_test_plan.md` Group 14.3。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCTR-EXT-01 | STE=0 时 U→S 外部陷阱不录制 | sctrctl.S=0, sctrctl.U=1, STE=0，U-mode 触发陷阱到 S-mode | 外部陷阱不被录制 |
| SSCTR-EXT-02 | STE=1 时 U→S 外部陷阱录制 | sctrctl.S=0, sctrctl.U=1, STE=1，U-mode 触发陷阱到 S-mode | 外部陷阱被录制 |
| SSCTR-EXT-03 | 外部陷阱 ctrtarget.PC=0 | STE=1，U-mode 触发外部陷阱到 S-mode，检查 entry | ctrtarget.PC = 0 |
| SSCTR-EXT-10 | 外部陷阱不受 EXCINH 影响 | STE=1, sctrctl.EXCINH=1，U-mode 异常陷阱到 S-mode | 外部陷阱仍被录制（EXCINH 不影响外部陷阱） |
| SSCTR-EXT-11 | 外部陷阱不受 INTRINH 影响 | STE=1, sctrctl.INTRINH=1，U-mode 中断陷阱到 S-mode | 外部陷阱仍被录制（INTRINH 不影响外部陷阱） |

---

## Group 9. 转换类型过滤（Transfer Type Filtering）

**规范依据**：
- `norm:ctr_ttf_default`：默认行为（所有过滤位=0）录制所有控制转换
- `norm:ctrctl_excinh_op`：EXCINH 抑制异常
- `norm:ctrctl_intrinh_op`：INTRINH 抑制中断
- `norm:ctrctl_tretinh_op`：TRETINH 抑制 trap return
- `norm:ctrctl_ntbren_op`：NTBREN 启用未取分支（opt-in）
- `norm:ctrctl_tkbrinh_op`：TKBRINH 抑制已取分支
- `norm:ctrctl_indcallinh_op`：INDCALLINH 抑制间接调用
- `norm:ctrctl_dircallinh_op`：DIRCALLINH 抑制直接调用
- `norm:ctrctl_indjmpinh_op`：INDJMPINH 抑制间接跳转（无链接）
- `norm:ctrctl_dirjmpinh_op`：DIRJMPINH 抑制直接跳转（无链接）
- `norm:ctrctl_corswapinh_op`：CORSWAPINH 抑制协程交换
- `norm:ctrctl_retinh_op`：RETINH 抑制函数返回
- `norm:ctrctl_indljmpinh_op`：INDLJMPINH 抑制其他间接跳转（带链接）
- `norm:ctrctl_dirljmpinh_op`：DIRLJMPINH 抑制其他直接跳转（带链接）

**测试职责**：验证各转换类型过滤位对录制的抑制/启用效果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCTR-TTF-01 | 默认行为录制所有转换 | 所有过滤位=0，S-mode 执行各种类型转换 | 所有转换被录制 |
| SSCTR-TTF-02 | EXCINH=1 抑制异常录制 | sctrctl.EXCINH=1，S-mode 触发异常陷阱 | 异常不被录制（TYPE=1 的转换缺失） |
| SSCTR-TTF-03 | INTRINH=1 抑制中断录制 | sctrctl.INTRINH=1，S-mode 接收中断 | 中断不被录制（TYPE=2 的转换缺失） |
| SSCTR-TTF-04 | TRETINH=1 抑制 trap return 录制 | sctrctl.TRETINH=1，S-mode 执行 SRET | SRET 不被录制（TYPE=3 的转换缺失） |
| SSCTR-TTF-05 | NTBREN=1 启用未取分支录制 | sctrctl.NTBREN=1，S-mode 执行未取分支 | 未取分支被录制（TYPE=4） |
| SSCTR-TTF-06 | NTBREN=0 时未取分支不录制 | sctrctl.NTBREN=0，S-mode 执行未取分支 | 未取分支不被录制（默认行为） |
| SSCTR-TTF-07 | TKBRINH=1 抑制已取分支录制 | sctrctl.TKBRINH=1，S-mode 执行已取分支 | 已取分支不被录制（TYPE=5 的转换缺失） |
| SSCTR-TTF-08 | TKBRINH=0 时已取分支正常录制 | sctrctl.TKBRINH=0，S-mode 执行已取分支 | 已取分支被录制（TYPE=5） |
| SSCTR-TTF-09 | INDCALLINH=1 抑制间接调用 | sctrctl.INDCALLINH=1，S-mode 执行 JALR x1, rs（rs≠x5） | 间接调用不被录制（TYPE=8 缺失） |
| SSCTR-TTF-10 | DIRCALLINH=1 抑制直接调用 | sctrctl.DIRCALLINH=1，S-mode 执行 JAL x1 | 直接调用不被录制（TYPE=9 缺失） |
| SSCTR-TTF-11 | INDJMPINH=1 抑制间接跳转（无链接） | sctrctl.INDJMPINH=1，S-mode 执行 JALR x0, rs | 间接跳转不被录制（TYPE=10 缺失） |
| SSCTR-TTF-12 | DIRJMPINH=1 抑制直接跳转（无链接） | sctrctl.DIRJMPINH=1，S-mode 执行 JAL x0 | 直接跳转不被录制（TYPE=11 缺失） |
| SSCTR-TTF-13 | CORSWAPINH=1 抑制协程交换 | sctrctl.CORSWAPINH=1，S-mode 执行 JALR x1, x5 | 协程交换不被录制（TYPE=12 缺失） |
| SSCTR-TTF-14 | RETINH=1 抑制函数返回 | sctrctl.RETINH=1，S-mode 执行 JALR x0, x1（ret） | 函数返回不被录制（TYPE=13 缺失） |
| SSCTR-TTF-15 | INDLJMPINH=1 抑制其他间接跳转（带链接） | sctrctl.INDLJMPINH=1，S-mode 执行 JALR x6, rs（rd≠x0/x1/x5） | 不被录制（TYPE=14 缺失） |
| SSCTR-TTF-16 | DIRLJMPINH=1 抑制其他直接跳转（带链接） | sctrctl.DIRLJMPINH=1，S-mode 执行 JAL x6 | 不被录制（TYPE=15 缺失） |
| SSCTR-TTF-17 | 多位过滤组合 | EXCINH=1, INTRINH=1, RETINH=1，S-mode 执行异常/中断/返回/跳转 | 仅异常/中断/返回被抑制，跳转正常录制 |

---

## Group 10. 周期计数（Cycle Counting）

**规范依据**：
- `norm:ctrdata_cc_supported`：可选的周期计数功能
- `norm:ctrdata_ccv`：CCV 位表示 CC 有效
- `norm:ctrdata_cc`：CC 字段由 CCE（CC[15:12]）和 CCM（CC[11:0]）组成
- `norm:ctr_ccounter_inc`：CtrCycleCounter 与 mcycle 同速率递增，仅 CTR active 时计数
- `norm:ctr_ccounter_reset`：写 xctrctl 或执行 SCTRCLR 时重置
- `norm:ctr_ccounter_impl`：CCV 和 CCM 必须实现（若支持 CC），CCE 可实现 0-4 位
- `norm:ctr_ccounter_sat`：CC 饱和行为
- `norm:ctr_ccounter_ccv`：CCV 在写 xctrctl 或 SCTRCLR 后清零

**测试职责**：验证周期计数功能的正确性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCTR-CC-01 | CCV 字段实现检测 | 执行两次转换后读 ctrdata.CCV | 若实现 CC，CCV=1；否则 CCV 只读零 |
| SSCTR-CC-02 | CC 字段编码验证 | 执行两次间隔已知的转换，检查 ctrdata.CC | 若 CCV=1，CC 值与实际经过周期数匹配（按 CCE/CCM 编码规则） |
| SSCTR-CC-03 | CtrCycleCounter 递增率 | 执行两次转换，比较 CC 值与 mcycle 差值 | CC 反映的周期数约等于 mcycle 差值 |
| SSCTR-CC-04 | 仅 CTR active 时计数 | sctrctl.S=1，S-mode 转换后切换到未启用模式执行若干指令，再切回 S-mode 执行转换 | CC 仅反映 active 期间的周期 |
| SSCTR-CC-05 | 写 sctrctl 重置 cycle counter | 写 sctrctl 后执行第一次转换，检查 CCV | CCV=0（cycle counter 已重置） |
| SSCTR-CC-06 | SCTRCLR 重置 cycle counter | 执行 SCTRCLR 后执行第一次转换，检查 CCV | CCV=0 |
| SSCTR-CC-07 | SCTRCLR 后第二次转换 CCV=1 | 执行 SCTRCLR 后连续执行两次转换 | 第二次转换的 CCV=1（若实现 CC） |
| SSCTR-CC-08 | CC 饱和行为 | 在两次转换间插入大量 NOP（超过最大 CC 值） | CC 值为所有已实现 CCM 和 CCE 位全 1（饱和值） |
| SSCTR-CC-09 | CCE 实现位数检测 | 读 ctrdata.CC[15:12]（CCE 字段） | 确定实现了多少 CCE 位（0-4），未实现位只读零 |
| SSCTR-CC-10 | CCE=0 时 CC = CCM | 若 CCE=0，读 CC 字段 | CC 值等于 CCM（CC[11:0]），即直接表示周期数 |
| SSCTR-CC-11 | FROZEN 期间不计数 | 启用录制，执行转换 A，设 FROZEN=1，等待，清 FROZEN=0，执行转换 B | B 的 CC 不包含 FROZEN 期间的周期 |

---

## Group 11. RAS 仿真模式

**规范依据**：
- `norm:ctrctl_rasemu_op`：RASEMU=1 时改变录制行为为 RAS 仿真

**测试职责**：验证 RAS 仿真模式下的特殊录制行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCTR-RAS-01 | RASEMU 字段实现检测 | 写 sctrctl.RASEMU=1 后读回 | 若实现 RAS 仿真，RASEMU 可写；否则只读零 |
| SSCTR-RAS-02 | RASEMU=1 时调用正常录制 | RASEMU=1，S-mode 执行 JAL x1（直接调用） | 调用被正常录制 |
| SSCTR-RAS-03 | RASEMU=1 时间接调用正常录制 | RASEMU=1，S-mode 执行 JALR x1, rs | 间接调用被正常录制 |
| SSCTR-RAS-04 | RASEMU=1 时返回弹出调用 | RASEMU=1，执行调用后执行 JALR x0, x1（ret） | WRPTR 递减，logical entry 0 被无效化（ctrsource.V=0），原 entry 1 变为 entry 0 |
| SSCTR-RAS-05 | RASEMU=1 时协程交换行为 | RASEMU=1，执行 JALR x1, x5（协程交换） | logical entry 0 被覆写（同时影响调用和返回），WRPTR 不变 |
| SSCTR-RAS-06 | RASEMU=1 时其他转换类型被抑制 | RASEMU=1，S-mode 执行已取分支 | 已取分支不被录制 |
| SSCTR-RAS-07 | RASEMU=1 时异常被抑制 | RASEMU=1，S-mode 触发异常 | 异常不被录制 |
| SSCTR-RAS-08 | RASEMU=1 时过滤位被忽略 | RASEMU=1，设置 DircallINH=1，执行直接调用 | 直接调用仍被录制（RASEMU 模式下过滤位被忽略） |
| SSCTR-RAS-09 | RASEMU=1 时外部陷阱位被忽略 | RASEMU=1，STE=1，触发外部陷阱 | 外部陷阱不被录制（RASEMU 模式下外部陷阱位被忽略） |
| SSCTR-RAS-10 | RASEMU=0 恢复正常行为 | 从 RASEMU=1 切回 RASEMU=0，执行已取分支 | 已取分支被录制（恢复正常行为） |

---

## Group 12. Freeze 行为

**规范依据**：
- `norm:sctrstatus_frozen_set`：LCOFIFRZ=1 时 LCOFI 陷阱设置 FROZEN
- `norm:ctr_freeze_bp`：BPFRZ=1 时 breakpoint 异常设置 FROZEN
- `norm:ctr_freeze_vs`：VS-mode 的 freeze 由 vsctrctl 控制
- `norm:sctrstatus_frozen_op`：FROZEN 抑制录制

**测试职责**：验证硬件自动设置 FROZEN 和软件手动控制 FROZEN 的行为（S-mode 视角）。

> **注意**：Hypervisor 相关的 VS-mode Freeze 行为测试（原 FRZ-09~12）已迁移至 `Hypervisor_cross_test_plan.md` Group 14.4。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCTR-FRZ-01 | BPFRZ=1 时 breakpoint 陷阱设置 FROZEN | sctrctl.BPFRZ=1，S-mode 执行 EBREAK 陷阱到 S-mode | sctrstatus.FROZEN=1 |
| SSCTR-FRZ-02 | BPFRZ=1 时 breakpoint 本身不被录制 | sctrctl.BPFRZ=1，S-mode 执行 EBREAK | breakpoint 异常不被录制到 CTR |
| SSCTR-FRZ-03 | BPFRZ=0 时 breakpoint 不设置 FROZEN | sctrctl.BPFRZ=0，S-mode 执行 EBREAK | sctrstatus.FROZEN 不变 |
| SSCTR-FRZ-04 | LCOFIFRZ=1 时 LCOFI 陷阱设置 FROZEN | sctrctl.LCOFIFRZ=1，HPM 计数器溢出触发 LCOFI 到 S-mode | sctrstatus.FROZEN=1 |
| SSCTR-FRZ-05 | LCOFIFRZ=1 时 LCOFI 本身不被录制 | sctrctl.LCOFIFRZ=1，LCOFI 陷阱到 S-mode | LCOFI 不被录制到 CTR |
| SSCTR-FRZ-06 | LCOFIFRZ=0 时 LCOFI 不设置 FROZEN | sctrctl.LCOFIFRZ=0，LCOFI 陷阱到 S-mode | sctrstatus.FROZEN 不变 |
| SSCTR-FRZ-07 | 软件可写 FROZEN=1 抑制录制 | S-mode 写 sctrstatus.FROZEN=1，启用录制，执行转换 | 转换不被录制 |
| SSCTR-FRZ-08 | 软件可写 FROZEN=0 恢复录制 | 先设 FROZEN=1 后写 FROZEN=0，启用录制，执行转换 | 转换被录制 |

---

## Group 13. 虚拟化模式转换（H 扩展）

> **本组已迁移**：原 Group 13（VIRT-01~05）的所有测试用例依赖 Hypervisor 扩展，已迁移至 `Hypervisor_cross_test_plan.md` Group 14.5（HCROSS-SSCTR-25~29）。

---

## Group 14. State Enable 访问控制（S-mode 视角）

**规范依据**：
- `norm:mstateen_ctr0` / `norm:mstateen_ctr0_execpt1/2/3`：mstateen0.CTR=0 时 S-mode 访问被阻止
- `norm:mstateen_ctr0_qualified_transfer`：CTR=0 时隐式更新继续

**测试职责**：从 S-mode 视角验证 State Enable 访问控制的效果。

> **注意**：Hypervisor 相关的 VS-mode State Enable 测试（原 SEA-02, SEA-06, SEA-07）已迁移至 `Hypervisor_cross_test_plan.md` Group 14.6（HCROSS-SSCTR-30~32）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCTR-SEA-01 | S-mode 检测 CTR 可访问性 | S-mode 尝试访问 sctrctl，观察是否触发异常 | 若 mstateen0.CTR=1 则访问成功；若 CTR=0 则触发 illegal-instruction |
| SSCTR-SEA-03 | mstateen0.CTR=0 时 S-mode sctrdepth 访问 | mstateen0.CTR=0，S-mode 尝试访问 sctrdepth | 触发 illegal-instruction（而非 virtual-instruction） |
| SSCTR-SEA-04 | mstateen0.CTR=0 时 S-mode SCTRCLR 执行 | mstateen0.CTR=0，S-mode 执行 SCTRCLR | 触发 illegal-instruction |
| SSCTR-SEA-05 | mstateen0.CTR=0 时 S-mode sireg* 访问 | mstateen0.CTR=0，S-mode 设 siselect=0x200 后读 sireg | 触发 illegal-instruction |

---

## Group 15. Custom Extensions

**规范依据**：
- `norm:ctr_custom_bits`：Custom[3:0] 非零值启用自定义扩展，可改变标准行为和定义新状态字段；Custom=0 时恢复标准行为

**测试职责**：验证 CTR 自定义扩展的行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCTR-CUST-01 | Custom[3:0]=0 时标准行为 | sctrctl.Custom=0，验证录制行为 | 标准 CTR 行为 |
| SSCTR-CUST-02 | Custom[3:0] 字段可写性检测 | 写 sctrctl.Custom=非零值后读回 | 若实现 custom 扩展，字段可写；否则只读零 |
| SSCTR-CUST-03 | Custom 非零时可能改变行为 | 若 custom 扩展实现，设 Custom=非零值，验证录制行为变化 | 行为可能按自定义扩展规范改变 |
| SSCTR-CUST-04 | Custom=0 恢复标准行为 | 从 Custom=非零值切回 Custom=0 | 所有自定义状态字段恢复标准行为（包括未定义位恢复只读零） |
| SSCTR-CUST-05 | Custom 扩展可定义 sctrstatus 新字段 | 若 custom 扩展实现，检查 sctrstatus 是否有新字段 | 新字段仅在 Custom 非零时有效 |
| SSCTR-CUST-06 | Custom 扩展可定义 entry register 新字段 | 若 custom 扩展实现，检查 ctrdata 是否有新字段 | 新字段仅在 Custom 非零时有效 |

---

## 测试实现说明

### 文件组织

```
damo-priv-test/
├── ssctr/
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_sctrctl_csr.c        # Group 1: sctrctl CSR
│       ├── test_sctrdepth.c          # Group 3: sctrdepth CSR
│       ├── test_sctrstatus.c         # Group 4: sctrstatus CSR
│       ├── test_entry_sctrclr.c      # Group 5: Entry Registers 与 SCTRCLR
│       ├── test_recording.c          # Group 6: 基本录制行为
│       ├── test_priv_transitions.c   # Group 7: 特权级模式转换
│       ├── test_exttrap.c            # Group 8: 外部陷阱（S-mode）
│       ├── test_transfer_filter.c    # Group 9: 转换类型过滤
│       ├── test_cycle_count.c        # Group 10: 周期计数
│       ├── test_ras_emulation.c      # Group 11: RAS 仿真模式
│       ├── test_freeze.c             # Group 12: Freeze 行为（S-mode）
│       ├── test_state_enable.c       # Group 14: State Enable 访问控制
│       └── test_custom_ext.c         # Group 15: Custom Extensions
├── hypervisor_cross/                  # Hypervisor 交叉测试（包含 Ssctr）
│   └── tests/
│       └── test_hcross_ssctr.c       # Group 14: Hypervisor × Ssctr
└── common/                            # 复用通用框架
```

> **注意**：原 `test_vsctrctl_csr.c`（Group 2）、`test_virt_transitions.c`（Group 13）和 Hypervisor 相关测试已迁移至 `hypervisor_cross/` 项目。

### 运行时检测

```c
static bool check_ssctr_extension(void) {
    /* Probe sctrctl writability */
    trap_expect_begin();
    uintptr_t old = CSRR(sctrctl);
    CSRW(sctrctl, old | SCTRCTL_S);  /* try to set S bit */
    uintptr_t new_val = CSRR(sctrctl);
    CSRW(sctrctl, old);  /* restore */
    trap_expect_end();
    return (new_val & SCTRCTL_S) != 0;  /* S bit was set => Ssctr exists */
}

static bool check_mstateen_extension(void) {
    /* Probe mstateen0 CSR existence */
    trap_expect_begin();
    uintptr_t val = CSRR(mstateen0);
    (void)val;
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}
```

### 关键注意事项

1. **扩展检测**：所有测试必须在运行时检测 Ssctr 的可用性（通过 `sctrctl` 可写性），不可用时 TEST_SKIP。

2. **Smstateen 依赖**：Group 14 测试需要 Smstateen 扩展。通过检测 `mstateen0` CSR 的存在性判断，不存在时 TEST_SKIP。

3. **STE 字段检测**：STE 是可选字段。测试前需通过写回检测确定是否实现，未实现时相关测试 TEST_SKIP。

4. **Hypervisor 测试**：需要 Hypervisor 扩展的测试（`vsctrctl`、VS/VU-mode 外部陷阱、虚拟化模式转换）已迁移至 `Hypervisor_cross_test_plan.md`。

5. **CTR 清理**：每次测试前应调用 `sctrclr()` 清空 entry 寄存器，避免前次测试干扰。

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1 (sctrctl CSR) | CTL-01~10 | sctrctl 是 Ssctr 扩展的核心控制寄存器 |
| P0（必须） | Group 3 (sctrdepth) | DEP-01~06 | CTR 缓冲区深度配置是基础功能 |
| P0（必须） | Group 4 (sctrstatus) | STS-01~10 | CTR 状态寄存器是录制控制的核心 |
| P1（重要） | Group 5 (Entry/SCTRCLR) | ENT-01~09, 11~17 | Entry 寄存器访问和清理是数据读取的基础 |
| P1（重要） | Group 6 (基本录制) | REC-01~13 | 录制行为是扩展的核心功能 |
| P1（重要） | Group 8 (外部陷阱) | EXT-01~03, 10~11 | S-mode 外部陷阱录制是关键功能 |
| P2（建议） | Group 7 (特权级转换) | TRAP-01~10 | 特权级转换录制是重要功能 |
| P2（建议） | Group 9 (类型过滤) | TTF-01~17 | 转换类型过滤是高级功能 |
| P2（建议） | Group 10 (周期计数) | CC-01~11 | 周期计数是可选但重要的功能 |
| P2（建议） | Group 11 (RAS 仿真) | RAS-01~10 | RAS 仿真是可选功能 |
| P2（建议） | Group 12 (Freeze) | FRZ-01~08 | Freeze 行为是可选功能 |
| P2（建议） | Group 14 (State Enable) | SEA-01, 03~05 | State Enable 访问控制是安全隔离的保证 |
| P3（可选） | Group 15 (Custom) | CUST-01~06 | Custom 扩展是实现相关功能 |

> **注意**：Hypervisor 相关测试的优先级见 `Hypervisor_cross_test_plan.md`。

---

## 参考

- `SPEC/ssctr.adoc` — Ssctr (Control Transfer Records - Supervisor-level) Extension
- `SPEC/smctr.adoc` — Smctr (Control Transfer Records - Machine-level) Extension
- `SPEC/smstateen.adoc` — Smstateen Extension Specification
- `SPEC/sscsrind.adoc` — Sscsrind (Supervisor-level Indirect CSR Access) Extension
- `DOCS/testplan/Smctr_test_plan.md` — Smctr Machine Mode 测试计划
- `DOCS/testplan/Hypervisor_cross_test_plan.md` — Hypervisor 与其他扩展交叉测试计划（含 Hypervisor × Ssctr，Group 14）

---

*生成时间：2026-06-25*
