**中文 | [English](../testplan_en/Hypervisor_Interrupts_test_plan_en.md)**

# Hypervisor 中断测试计划（Hypervisor_Interrupts 子集）

> 本文档拆分自已删除的原 Hypervisor 综合测试计划（Hypervisor_test_plan.md），测试用例编号保持不变，Group 序号为本子集内重新排列。
> 兄弟子集：[Hypervisor_CSR_test_plan.md](Hypervisor_CSR_test_plan.md) | [Hypervisor_Exceptions_test_plan.md](Hypervisor_Exceptions_test_plan.md)

## 概述

本测试计划覆盖 RISC-V Hypervisor (H) 扩展的中断相关功能点，包括虚拟中断注入（hvip/hip/hie）、guest external interrupts（hgeip/hgeie）、M-level 中断寄存器增强（mideleg/mip/mie）以及 hideleg 中断委托与 VS 中断号翻译。CSR 寄存器字段行为、异常与 trap 行为分别由兄弟子集覆盖。

本测试计划依据 `SPEC/hypervisor.adoc` 中的规范点（norm 标记）编写。

### 本文档覆盖的 SPEC 章节
- Hypervisor and Virtual Supervisor CSRs（hvip, hip, hie, hgeip, hgeie, hideleg 中断委托行为）
- Machine-Level CSR 增强（mideleg, mip/mie）
- Traps（中断委托链路与中断号翻译）

### 由其他测试计划覆盖
- Hypervisor CSR（CSR 替代机制, hstatus, henvcfg, htimedelta, VS CSR, hedeleg/hideleg 位域属性） → `Hypervisor_CSR_test_plan.md`
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

本章节列出本文档 Groups 1-4 所有测试组中引用的规范点（norm ID），已去重并按字母顺序排列。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:geilen` | The number of bits implemented in `hgeip` and `hgeie` for guest external interrupts is UNSPECIFIED and may be zero. This number is known as GEILEN. The least-significant bits are implemented first, apart from bit 0. Hence, if GEILEN is nonzero, bits GEILEN:1 shall be writable in `hgeie`, and all other bit positions shall be read-only zeros in both `hgeip` and `hgeie`. | `hgeip` 和 `hgeie` 中客户外部中断实现的位数未指定，可为零。该数目称为 GEILEN。除第 0 位外，最低有效位先实现。若 GEILEN 非零，`hgeie` 的 GEILEN:1 位可写，其余位在两个寄存器中均为只读零。 |
| `norm:hgeie_op` | Register `hgeie` selects the subset of guest external interrupts that cause a supervisor-level (HS-level) guest external interrupt. The enable bits in `hgeie` do not affect the VS-level external interrupt signal selected from `hgeip` by `hstatus`.VGEIN. | `hgeie` 选择引起 HS 级客户外部中断的客户外部中断子集。`hgeie` 中的使能位不影响由 `hstatus`.VGEIN 从 `hgeip` 中选择的 VS 级外部中断信号。 |
| `norm:hgeie_sz_acc_op` | The `hgeie` register is an HSXLEN-bit read/write register that contains enable bits for the guest external interrupts at this hart. | `hgeie` 是一个 HSXLEN 位读写寄存器，包含此 hart 的客户外部中断使能位。 |
| `norm:hgeip_hgeie_fields` | Guest external interrupt number _i_ corresponds with bit _i_ in both `hgeip` and `hgeie`. | 客户外部中断号 _i_ 对应 `hgeip` 和 `hgeie` 中的位 _i_。 |
| `norm:hgeip_sz_acc_op` | The `hgeip` register is an HSXLEN-bit read-only register that indicates pending guest external interrupts for this hart. | `hgeip` 是一个 HSXLEN 位只读寄存器，指示此 hart 的待处理客户外部中断。 |
| `norm:hideleg_hs` | An interrupt _i_ will trap to HS-mode whenever all of the following are true: (a) either the current operating mode is HS-mode and the SIE bit in the `sstatus` register is set, or the current operating mode has less privilege than HS-mode; (b) bit _i_ is set in both `sip` and `sie`, or in both `hip` and `hie`; and (c) bit _i_ is not set in `hideleg`. | 中断 _i_ 在以下条件全部满足时陷入 HS 模式：(a) 当前为 HS 模式且 `sstatus`.SIE=1，或当前特权低于 HS 模式；(b) 位 _i_ 在 `sip`/`sie` 或 `hip`/`hie` 中都已设置；(c) 位 _i_ 在 `hideleg` 中未设置。 |
| `norm:hideleg_op` | An interrupt that has been delegated to HS-mode (using `mideleg`) is further delegated to VS-mode if the corresponding `hideleg` bit is set. | 已通过 `mideleg` 委托给 HS 模式的中断，若对应 `hideleg` 位已设置，则进一步委托给 VS 模式。 |
| `norm:hideleg_trans` | When a virtual supervisor external interrupt (code 10) is delegated to VS-mode, it is automatically translated by the machine into a supervisor external interrupt (code 9) for VS-mode. Likewise, virtual supervisor timer interrupt (6) is translated into supervisor timer interrupt (5), and virtual supervisor software interrupt (2) into supervisor software interrupt (1). | 当虚拟 supervisor 外部中断（代码 10）被委托给 VS 模式时，自动翻译为 supervisor 外部中断（代码 9）。类似地，虚拟定时器中断(6)→定时器中断(5)，虚拟软件中断(2)→软件中断(1)。 |
| `norm:hie_op` | `hie` contains enable bits for the same interrupts. | `hie` 包含相同中断的使能位。 |
| `norm:hip_hie_sz_acc` | Registers `hip` and `hie` are HSXLEN-bit read/write registers that supplement HS-level's `sip` and `sie` respectively. | `hip` 和 `hie` 是 HSXLEN 位读写寄存器，分别补充 HS 级的 `sip` 和 `sie`。 |
| `norm:hip_op` | The `hip` register indicates pending VS-level and hypervisor-specific interrupts. | `hip` 寄存器指示待处理的 VS 级和 hypervisor 特定中断。 |
| `norm:hip_vseip_vseie_op` | Bits `hip`.VSEIP and `hie`.VSEIE are the interrupt-pending and interrupt-enable bits for VS-level external interrupts. VSEIP is read-only in `hip`, and is the logical-OR of: bit VSEIP of `hvip`; the bit of `hgeip` selected by `hstatus`.VGEIN; and any other platform-specific external interrupt signal directed to VS-level. | `hip`.VSEIP 和 `hie`.VSEIE 是 VS 级外部中断的中断待处理和使能位。VSEIP 在 `hip` 中为只读，是 `hvip`.VSEIP、`hgeip` 中由 `hstatus`.VGEIN 选择的位、以及其他平台特定的 VS 级外部中断信号的逻辑或。 |
| `norm:hip_vssip_vssie_op` | Bits `hip`.VSSIP and `hie`.VSSIE are the interrupt-pending and interrupt-enable bits for VS-level software interrupts. VSSIP in `hip` is an alias (writable) of the same bit in `hvip`. | `hip`.VSSIP 和 `hie`.VSSIE 是 VS 级软件中断的中断待处理和使能位。`hip` 中的 VSSIP 是 `hvip` 中相同位的别名（可写）。 |
| `norm:hip_vstip_vstie_acc_op` | Bits `hip`.VSTIP and `hie`.VSTIE are the interrupt-pending and interrupt-enable bits for VS-level timer interrupts. VSTIP is read-only in `hip`, and is the logical-OR of `hvip`.VSTIP and, when the Sstc extension is implemented, the timer interrupt signal resulting from `vstimecmp`. | `hip`.VSTIP 和 `hie`.VSTIE 是 VS 级定时器中断的中断待处理和使能位。VSTIP 在 `hip` 中为只读，是 `hvip`.VSTIP 与（当实现 Sstc 扩展时）`vstimecmp` 产生的定时器中断信号的逻辑或。 |
| `norm:hsint_priority` | Multiple simultaneous interrupts destined for HS-mode are handled in the following decreasing priority order: SEI, SSI, STI, SGEI, VSEI, VSSI, VSTI, LCOFI. | 多个同时到达 HS 模式的中断按以下优先级递减顺序处理：SEI、SSI、STI、SGEI、VSEI、VSSI、VSTI、LCOFI。 |
| `norm:hvip_acc` | The standard portion (bits 15:0) of `hvip` is formatted as shown. Bits VSEIP, VSTIP, and VSSIP of `hvip` are writable. | `hvip` 的标准部分（15:0 位）中，VSEIP、VSTIP 和 VSSIP 位可写。 |
| `norm:hvip_sz_op` | Register `hvip` is an HSXLEN-bit read/write register that a hypervisor can write to indicate virtual interrupts intended for VS-mode. Bits of `hvip` that are not writable are read-only zeros. | `hvip` 是一个 HSXLEN 位读写寄存器，hypervisor 可写入以指示用于 VS 模式的虚拟中断。不可写的位为只读零。 |
| `norm:mideleg_acc_h` | When the hypervisor extension is implemented, bits 10, 6, and 2 of `mideleg` are each read-only one. Furthermore, if GEILEN is nonzero, bit 12 of `mideleg` is also read-only one. VS-level interrupts and guest external interrupts are always delegated past M-mode to HS-mode. | 实现 hypervisor 扩展时，`mideleg` 的第 10、6、2 位为只读 1。若 GEILEN 非零，第 12 位也为只读 1。VS 级中断和客户外部中断始终委托到 HS 模式。 |
| `norm:mideleg_hroz` | For bits of `mideleg` that are zero, the corresponding bits in `hideleg`, `hip`, and `hie` are read-only zeros. | `mideleg` 中为零的位，`hideleg`、`hip`、`hie` 中对应位为只读零。 |
| `norm:mip_mie_alias` | Bits SGEIP, VSEIP, VSTIP, and VSSIP in `mip` are aliases for the same bits in hypervisor CSR `hip`, while SGEIE, VSEIE, VSTIE, and VSSIE in `mie` are aliases for the same bits in `hie`. | `mip` 中的 SGEIP、VSEIP、VSTIP、VSSIP 是 `hip` 中相同位的别名；`mie` 中的 SGEIE、VSEIE、VSTIE、VSSIE 是 `hie` 中相同位的别名。 |
| `norm:mip_mie_vs` | The hypervisor extension gives registers `mip` and `mie` additional active bits for the hypervisor-added interrupts. | hypervisor 扩展为 `mip` 和 `mie` 添加 hypervisor 新增中断的额外活跃位。 |
| `norm:sie_hip_hie_mutex` | For each writable bit in `sie`, the corresponding bit shall be read-only zero in both `hip` and `hie`. Hence, the nonzero bits in `sie` and `hie` are always mutually exclusive, and likewise for `sip` and `hip`. | 对于 `sie` 中的每个可写位，`hip` 和 `hie` 中的对应位必须为只读零。`sie` 和 `hie` 的非零位始终互斥，`sip` 和 `hip` 同理。 |

---

## Group 1. hvip / hip / hie 中断寄存器

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
| HINT-04 | hip.VSSIP 是 hvip.VSSIP 的双向可写 alias | 读方向：写 hvip.VSSIP=1，读 hip.VSSIP=1；写方向：清 hvip.VSSIP=0 后直接写 hip.VSSIP=1，读 hvip.VSSIP | 双向一致：写 hip.VSSIP 等价于写 hvip.VSSIP |
| HINT-05 | hip.VSEIP 只读（多源 OR） | 组合1：hvip.VSEIP=1，写 hip.VSEIP=0，hip.VSEIP 仍为 1；组合2：hvip.VSEIP=0，写 hip.VSEIP=1，hip.VSEIP 仍为 0 | 两种组合下写入均被忽略，hip.VSEIP 始终反映 hvip.VSEIP |
| HINT-06 | hip.VSTIP 只读 | 组合1：hvip.VSTIP=1，写 hip.VSTIP=0，hip.VSTIP 仍为 1；组合2：hvip.VSTIP=0，写 hip.VSTIP=1，hip.VSTIP 仍为 0 | 两种组合下写入均被忽略，hip.VSTIP 始终反映 hvip.VSTIP |
| HINT-07 | hie VSEIE/VSTIE/VSSIE 可写 | 写 hie 的 VSEIE/VSTIE/VSSIE bits | 正常读写 |
| HINT-08 | sie 与 hip/hie 互斥 | 检查 sie 可写 bit 在 hip/hie 中是否只读零 | 互斥关系成立 |
| HINT-09 | 清除 hvip.VSSIP 清除中断 | 写 hvip.VSSIP=0 | VS software interrupt 被清除 |
| HINT-10 | hideleg=0 时中断 trap 到 HS | hideleg[2]=0，注入 VSSIP | 中断 trap 到 HS-mode |
| HINT-11 | hideleg=1 时中断 trap 到 VS | hideleg[2]=1，注入 VSSIP | 中断 trap 到 VS-mode |
| HINT-12 | HS-mode 中断优先级 SEI > SSI | 同时 pending SEI 和 SSI | SEI 先被处理 |
| HINT-13 | HS-mode 中断优先级 VSEI > VSSI > VSTI | 同时 pending 多个 VS 中断 | 按 VSEI > VSSI > VSTI 顺序 |
| HINT-14 | hip/hie 非标准 bit 只读零 | 读 hip/hie 的保留 bit | 均为零 |
| HINT-15 | sstatus.SIE=0 时中断不递送到 HS-mode | hideleg[2]=0，注入 VSSIP，hie.VSSIE=1，进入 HS-mode 但 SIE=0 | 中断不触发；随后设 SIE=1，中断立即触发 |
| HINT-16 | hvip 不可写位只读零 | 写 hvip 全 1，读回 | 仅 bits 2/6/10 (VSSIP/VSTIP/VSEIP) 为 1，其余均为 0 |
| HINT-17 | hip.VSTIP 在 V=0 时仍有效 | V=0（HS-mode）下设置/清除 hvip.VSTIP，读 hip.VSTIP | hip.VSTIP 在 V=0 时始终反映 hvip.VSTIP（已定义） |
| HINT-18 | HS-mode 中断优先级 SSI > STI | 同时 pending SSI 和 STI，使能两者，进入 HS-mode | SSI (cause=1) 先被递送，证明 SSI > STI |

---

## Group 2. hgeip / hgeie 寄存器

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

## Group 3. mideleg / mip / mie 增强

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

## Group 4. hideleg 中断委托与中断号翻译

> 本组拆分自原 `Hypervisor_test_plan.md` 的 Group 3，仅保留中断委托与 cause 翻译用例（DELEG-08~14）；CSR 位域属性用例见 `Hypervisor_CSR_test_plan.md`，异常委托链路用例见 `Hypervisor_Exceptions_test_plan.md`。

**规范依据**：
- `norm:hideleg_op`：被 mideleg 委托的中断，若 hideleg 对应 bit 置位则进一步委托到 VS-mode
- `norm:hideleg_trans`：VS-level 中断号翻译（10→9, 6→5, 2→1）

**测试职责**：验证 hideleg 中断委托链路（HS→VS）的正确性与 VS 中断号翻译行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| DELEG-08 | hideleg 委托 VSSI 到 VS | 设 hideleg[2]=1，注入 VSSIP，VS-mode 使能中断 | trap 进入 VS-mode（vscause=1，经翻译） |
| DELEG-09 | hideleg 委托 VSTI 到 VS | 设 hideleg[6]=1，注入 VSTIP，VS-mode 使能中断 | trap 进入 VS-mode（vscause=5，经翻译） |
| DELEG-10 | hideleg 委托 VSEI 到 VS | 设 hideleg[10]=1，注入 VSEIP，VS-mode 使能中断 | trap 进入 VS-mode（vscause=9，经翻译） |
| DELEG-11 | hideleg 未委托时中断 trap 到 HS | 设 hideleg[2]=0，注入 VSSIP | trap 进入 HS-mode |
| DELEG-12 | 中断号翻译验证 VSSI→SSI | hideleg[2]=1 委托 VSSIP 到 VS-mode | vscause 记录 cause=1（非 2） |
| DELEG-13 | 中断号翻译验证 VSTI→STI | hideleg[6]=1 委托 VSTIP 到 VS-mode | vscause 记录 cause=5（非 6） |
| DELEG-14 | 中断号翻译验证 VSEI→SEI | hideleg[10]=1 委托 VSEIP 到 VS-mode | vscause 记录 cause=9（非 10） |
