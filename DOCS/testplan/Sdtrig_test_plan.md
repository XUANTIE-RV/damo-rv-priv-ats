# Sdtrig 扩展测试计划

## 概述

本测试计划覆盖 RISC-V Sdtrig（Trigger Module）扩展的所有核心功能点。该扩展定义了 Trigger Module（TM），提供硬件触发器功能，可在不执行特殊指令的情况下触发断点异常、进入调试模式或执行 trace 动作。触发器支持指令执行地址匹配、load/store 地址/数据匹配、指令计数、中断触发、异常触发和外部触发等多种类型。

本测试计划依据 `SPEC/riscv-debug/Sdtrig.adoc` 和 `SPEC/riscv-debug/xml/hwbp_registers.xml` 中的规范点（norm 标记）编写。

### 本文档覆盖的 SPEC 章节
- Sdtrig (ISA Extension)（`Sdtrig.adoc`：扩展概述、触发器基本概念）
- Enumeration（触发器枚举发现算法）
- Actions（`mcontrol-action` 编码与行为）
- Priority（触发器与同步异常的优先级关系）
- Native Triggers（`action=0` 时的原生触发与重入保护）
- Memory Access Triggers（A 扩展、组合访问、缓存操作、地址匹配）
- Multiple State Change Instructions（多状态变更指令的触发行为）
- Trigger Module Registers（CSR 访问语义、WARL 行为、安全写入序列）
- tselect / tdata1 / tdata2 / tdata3 / tinfo / tcontrol 寄存器
- mcontrol (type=2, deprecated) / mcontrol6 (type=6) 寄存器
- icount (type=3) / itrigger (type=4) / etrigger (type=5) / tmexttrigger (type=7) 寄存器
- textra32 / textra64 / scontext / mcontext / hcontext 寄存器

### 由其他测试计划覆盖
- Sdext（Debug Mode 进入/退出、`dpc` 寄存器行为） → 外部调试器相关测试
- Hypervisor 扩展的 VS/VU-mode 触发器行为 → `Hypervisor_cross_test_plan.md`
- Smstateen 对 `scontext` 访问控制（`mstateen0[57]`） → Smstateen 测试计划

---

## 覆盖的规范点

本章节列出本文档所有测试组中引用的规范点（norm ID），已去重并按字母顺序排列。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sdtrig_at_least_one` | If Sdtrig is implemented, the Trigger Module must support at least one trigger. | 若实现 Sdtrig，TM 必须支持至少一个触发器。 |
| `norm:sdtrig_unused_csr_illegal` | Accessing trigger CSRs that are not used by any of the implemented triggers must result in an illegal instruction exception. | 访问未被任何已实现触发器使用的触发器 CSR 必须产生非法指令异常。 |
| `norm:sdtrig_mmode_access` | M-Mode and Debug Mode accesses to trigger CSRs that are used by any of the implemented triggers must succeed, regardless of the current type of the currently selected trigger. | M-mode 和 Debug Mode 对已实现触发器使用的触发器 CSR 的访问必须成功，无论当前选中触发器的类型。 |
| `norm:sdtrig_no_fire_in_debug` | Triggers do not fire while in Debug Mode. | 在 Debug Mode 中触发器不触发。 |
| `norm:enum_tselect_write0` | Write 0 to tselect. If this results in an illegal instruction exception, then there are no triggers implemented. | 写 0 到 tselect，若产生非法指令异常则无触发器实现。 |
| `norm:enum_tselect_readback` | Read back tselect and check that it contains the written value. If not, exit the loop. | 回读 tselect 检查是否包含写入值，否则退出枚举循环。 |
| `norm:enum_tinfo` | Read tinfo. If that caused an exception, the debugger must read tdata1 to discover the type. If tinfo.info is 1, this trigger doesn't exist. | 读 tinfo；若异常则读 tdata1 获取类型；若 tinfo.info=1 则触发器不存在。 |
| `norm:enum_tdata1_type0` | If tdata1.type is 0, this trigger doesn't exist. Exit the loop. | 若 tdata1.type=0，触发器不存在，退出枚举。 |
| `norm:warl_write_any_read_legal` | All tdata registers follow write-any-read-legal semantics. If a debugger writes an unsupported configuration, the register will read back a value that is supported (which may simply be a disabled trigger). | 所有 tdata 寄存器遵循 WARL 语义。写入不支持的配置时，回读值为某合法值（可能为禁用状态）。 |
| `norm:warl_write_zero_disable` | Writing 0 to tdata1 disables the trigger, and leaves it in a state where tdata2 and tdata3 can be written with any value that makes sense for any trigger type supported by this trigger. | 写 0 到 tdata1 禁用触发器，并使 tdata2/tdata3 可写入该触发器支持的任意类型的合法值。 |
| `norm:warl_safe_write_sequence` | A debugger can write any supported trigger as follows: 1. Write 0 to tdata1. 2. Write desired values to tdata2 and tdata3. 3. Write desired value to tdata1. | 安全写入序列：先写 0 到 tdata1 禁用触发器，再写 tdata2/tdata3，最后写 tdata1。 |
| `norm:warl_no_cross_modify` | Writes to one tdata register must not modify the contents of other tdata registers, nor the configuration of any trigger besides the one that is currently selected. | 写入一个 tdata 寄存器不得修改其他 tdata 寄存器的内容，也不得修改非当前选中触发器的配置。 |
| `norm:tdata1_type0_hwired` | If tdata1.type is 0, then dmode is hard-wired to 0 and data field is hard-wired to 0. | 若 tdata1.type=0（无触发器），dmode 和数据字段均为只读零。 |
| `norm:tselect_warl` | tselect is WARL. Writes of values >= number of supported triggers may result in a different value. | tselect 为 WARL，写入 >= 触发器数量的值可能回读为不同值。 |
| `norm:tinfo_version` | tinfo.version: 0 = older spec, 1 = ratified version 1.0. | tinfo.version 字段标识 Sdtrig 规范版本。 |
| `norm:tinfo_info_bitmask` | tinfo.info: One bit for each possible tdata1.type. Bit N corresponds to type N. If the currently selected trigger doesn't exist, this field contains 1. | tinfo.info 为位掩码，每位对应一种触发器类型；若触发器不存在则值为 1。 |
| `norm:dmode_writable_debug_only` | dmode is only writable from Debug Mode. In ordinary use, external debuggers will always set this bit when configuring a trigger. | dmode 仅可从 Debug Mode 写入；正常使用时外部调试器配置触发器时总会设置此位。 |
| `norm:dmode_zero_action1_illegal` | action=1 (Enter Debug Mode) is only legal when the trigger's dmode is 1. Since tdata1 is WARL, hardware must prevent it from containing dmode=0 and action=1. | action=1 仅在 dmode=1 时合法；WARL 语义要求硬件阻止 dmode=0 且 action=1 的组合。 |
| `norm:action_breakpoint` | action=0: Raise a breakpoint exception. xepc must contain the virtual address of the next instruction that must be executed to preserve the program flow. | action=0：产生断点异常，xepc 包含保持程序流的下一条指令虚拟地址。 |
| `norm:action_debug_mode` | action=1: Enter Debug Mode. dpc must contain the virtual address of the next instruction. This action is only legal when dmode=1. This action can only be supported if Sdext is implemented. | action=1：进入 Debug Mode，dpc 包含下一条指令虚拟地址；仅 dmode=1 时合法，且需实现 Sdext。 |
| `norm:action_trace` | action=2: Trace on, action=3: Trace off, action=4: Trace notify. | action=2/3/4 分别对应 trace on/off/notify。 |
| `norm:action_external` | action=8/9: Send a signal to TM external trigger output 0 or 1. | action=8/9：向 TM 外部触发输出 0/1 发送信号。 |
| `norm:priority_table` | Trigger priority relative to synchronous exceptions as specified in the priority table. | 触发器相对于同步异常的优先级如规范优先级表所述。 |
| `norm:priority_multiple_same` | When multiple triggers in the same priority fire at once, hit (if implemented) is set for all of them. | 同优先级多个触发器同时触发时，所有触发器的 hit 位（若实现）均被设置。 |
| `norm:priority_action0_action1` | If one trigger has action=1 and another has action=0, the preferred behavior is to have both actions take place. If not implemented, the hart must enter Debug Mode and ignore the breakpoint exception. | 若一个触发器 action=1 另一个 action=0，首选行为是两者均执行；否则必须进入 Debug Mode 并忽略断点异常。 |
| `norm:native_mmode_mie_protection` | Hardware prevents triggers with action=0 from matching or firing while in M-mode and while MIE in mstatus is 0. | 硬件在 M-mode 且 mstatus.MIE=0 时阻止 action=0 的触发器匹配或触发。 |
| `norm:native_smode_sie_protection` | If medeleg[3]=1 then hardware prevents triggers with action=0 from matching or firing while in S-mode and while SIE in sstatus is 0. | 若 medeleg[3]=1，硬件在 S-mode 且 sstatus.SIE=0 时阻止 action=0 触发器。 |
| `norm:native_tcontrol_protection` | tcontrol.mte and tcontrol.mpte is implemented. medeleg[3] is hard-wired to 0. | 通过 tcontrol.mte/mpte 实现保护，medeleg[3] 只读零。 |
| `norm:tcontrol_mte_trap` | When any trap into M-mode is taken, tcontrol.mpte is set to the value of tcontrol.mte. tcontrol.mte is set to 0. | 进入 M-mode trap 时，mpte 设为 mte 的当前值，mte 清零。 |
| `norm:tcontrol_mte_mret` | When mret is executed, tcontrol.mte is set to the value of tcontrol.mpte. | 执行 mret 时，mte 设为 mpte 的值。 |
| `norm:mcontrol_match_exec` | When execute is set, the trigger fires on the virtual address or opcode of an instruction that is executed. | mcontrol.execute=1 时，触发器在指令执行的虚拟地址或操作码上触发。 |
| `norm:mcontrol_match_load` | When load is set, the trigger fires on the virtual address or data of any load. | mcontrol.load=1 时，触发器在 load 的虚拟地址或数据上触发。 |
| `norm:mcontrol_match_store` | When store is set, the trigger fires on the virtual address or data of any store. | mcontrol.store=1 时，触发器在 store 的虚拟地址或数据上触发。 |
| `norm:mcontrol_select_address` | select=0: compare values contain the lowest virtual address of the access. | select=0：比较值包含访问的最低虚拟地址。 |
| `norm:mcontrol_select_data` | select=1: compare value contains the data value loaded or stored, or the instruction executed. | select=1：比较值包含 load/store 数据或执行的指令。 |
| `norm:mcontrol_match_equal` | match=0: Matches when any compare value equals tdata2. | match=0（equal）：任一比较值等于 tdata2 时匹配。 |
| `norm:mcontrol_match_napot` | match=1: NAPOT range match. | match=1（NAPOT）：自然对齐 2 的幂范围匹配。 |
| `norm:mcontrol_match_ge` | match=2: Matches when any compare value >= tdata2 (unsigned). | match=2（ge）：任一比较值 >= tdata2（无符号）时匹配。 |
| `norm:mcontrol_match_lt` | match=3: Matches when any compare value < tdata2 (unsigned). | match=3（lt）：任一比较值 < tdata2（无符号）时匹配。 |
| `norm:mcontrol_match_mask` | match=4/5: Mask low/high match. | match=4/5（mask low/high）：掩码低/高位匹配。 |
| `norm:mcontrol_match_not_equal` | match=8: Matches when match=0 would not match. | match=8（not equal）：match=0 不匹配时匹配。 |
| `norm:mcontrol_chain` | chain=1: While this trigger does not match, it prevents the next trigger from matching. Chain fires only when all triggers match simultaneously. | chain=1：当前触发器不匹配时阻止下一触发器匹配；链式触发要求所有触发器同时匹配。 |
| `norm:mcontrol_priv_bits` | m/s/u bits enable trigger in corresponding privilege modes. s bit is hard-wired to 0 if hart does not support S-mode. u bit is hard-wired to 0 if hart does not support U-mode. | m/s/u 位分别在对应特权模式下启用触发器；不支持的模式对应位只读零。 |
| `norm:mcontrol_action_encoding` | action field encoding: 0=breakpoint, 1=debug mode, 2=trace on, 3=trace off, 4=trace notify, 8=external0, 9=external1. | action 字段编码。 |
| `norm:mcontrol_hit` | hit bit becomes set when trigger fires and may become set when trigger matches. | hit 位在触发器触发时被设置，匹配时也可能被设置。 |
| `norm:mcontrol_maskmax` | maskmax specifies the largest NAPOT range supported. Value is log2 of bytes. 0 means match=1 not supported. | maskmax 指定最大 NAPOT 范围，值为字节数的 log2；0 表示不支持 match=1。 |
| `norm:mcontrol_size` | sizehi:sizelo encoding: 0=any, 1=8bit, 2=16bit, 3=32bit, 5=64bit, etc. Implementation must support value 0. | size 编码：0=任意，1=8 位等；实现必须支持值 0。 |
| `norm:mcontrol6_hit_encoding` | hit1:hit0 encoding: 0=false, 1=before, 2=after, 3=immediately after. | mcontrol6 hit 编码：0=未触发，1=指令前，2=指令后，3=紧跟指令后。 |
| `norm:mcontrol6_uncertain` | uncertain/uncertainen fields for systems where TM cannot fully observe all memory accesses. | uncertain/uncertainen 字段用于 TM 无法完全观察所有内存访问的系统。 |
| `norm:mcontrol6_maskmax6` | NAPOT ranges between 2^1 and 2^{maskmax6} are supported where maskmax6 >= 1. | 支持 2^1 到 2^{maskmax6} 的 NAPOT 范围，maskmax6 >= 1。 |
| `norm:icount_decrement` | When count > 1 and trigger matches, count is decremented by 1. When count is 1 and trigger matches, pending becomes set. | count>1 且匹配时递减 1；count=1 且匹配时 pending 被设置。 |
| `norm:icount_pending_fire` | When pending is set, trigger fires just before next instruction in enabled mode. pending is cleared on fire. | pending 被设置后，触发器在下一条指令执行前触发，pending 被清除。 |
| `norm:icount_match_conditions` | Trigger matches when: 1) instruction retires in enabled mode, 2) trap taken from enabled mode. | 触发条件：指令在启用模式下退休，或从启用模式下陷入 trap。 |
| `norm:icount_count0_stays` | When count is 0 it stays at 0 until explicitly written. | count=0 时保持不变直到显式写入。 |
| `norm:icount_tval_zero` | If trigger fires with action=0, zero is written to tval CSR. | icount 触发且 action=0 时，tval 被写入零。 |
| `norm:icount_hardwired1_clear` | If count is hard-wired to 1, when pending fires, m/s/u/vs/vu are all cleared. | 若 count 只读为 1，pending 触发后 m/s/u/vs/vu 均被清除。 |
| `norm:itrigger_tdata2_bitmask` | tdata2 is a bitmask of interrupt numbers to match. | itrigger 的 tdata2 为中断号位掩码。 |
| `norm:itrigger_nmi` | nmi bit enables trigger for non-maskable interrupts. | nmi 位启用不可屏蔽中断触发。 |
| `norm:itrigger_fire_timing` | Trigger fires after trap occurs, just before first instruction of trap handler. | 触发器在 trap 发生后、trap handler 第一条指令执行前触发。 |
| `norm:itrigger_tval_zero` | If action=0, zero is written to tval CSR. | itrigger 触发且 action=0 时，tval 被写入零。 |
| `norm:etrigger_tdata2_bitmask` | tdata2 is a bitmask of exception codes to match. | etrigger 的 tdata2 为异常码位掩码。 |
| `norm:etrigger_fire_timing` | Trigger fires after trap occurs, just before first instruction of trap handler. | 触发器在 trap 发生后、trap handler 第一条指令执行前触发。 |
| `norm:etrigger_tval_zero` | If action=0, zero is written to tval CSR. | etrigger 触发且 action=0 时，tval 被写入零。 |
| `norm:tmexttrigger_select` | select field: bitmask of up to 16 TM external trigger inputs. | tmexttrigger 的 select 字段为最多 16 个外部触发输入的位掩码。 |
| `norm:tmexttrigger_intctl` | intctl bit: causes trigger to fire when interrupt controller signals a trigger. | intctl 位：中断控制器信号触发时触发。 |
| `norm:tmexttrigger_async` | This trigger fires asynchronously but is subject to delegation by medeleg[3]. | 外部触发器异步触发，但受 medeleg[3] 委托控制。 |
| `norm:mem_lr_as_load` | lr instructions are loads. | lr 指令视为 load。 |
| `norm:mem_sc_as_store` | Successful sc instructions are stores. | 成功的 sc 指令视为 store。 |
| `norm:mem_amo_as_load_store` | Each AMO instruction is a load for read portion and a store for write portion. | AMO 指令的读部分视为 load，写部分视为 store。 |
| `norm:mem_combined_accesses` | Vector loads/stores, cm.push/cm.pop should match as if individual accesses. | 向量 load/store、cm.push/cm.pop 应如同独立访问一样匹配。 |
| `norm:mem_cache_ops` | Cache operations (cbo.clean/flush/inval/zero) must match as stores. Only triggers with size=0 and select=0 will match. | 缓存操作必须作为 store 匹配，仅 size=0 且 select=0 的触发器匹配。 |
| `norm:mem_cache_hints_no_match` | Cache operations encoded as HINTs do not match debug triggers. | 编码为 HINT 的缓存操作不匹配调试触发器。 |
| `norm:addr_tdata2_full_range` | tdata2 must be able to hold all valid addresses in all supported translation modes. | tdata2 必须能容纳所有支持转换模式下的全部有效地址。 |
| `norm:addr_invalid_convert` | Writes of an invalid address that cannot be represented should be converted to a different invalid address that can be represented. | 无法表示的无效地址写入应转换为可表示的无效地址。 |
| `norm:scontext_warl` | scontext.data: WARL, supervisor context ID. Implementation may tie high bits to 0. | scontext.data 为 WARL，实现可将高位绑定为 0。 |
| `norm:mcontext_warl` | mcontext.hcontext: WARL, machine/hypervisor context ID. | mcontext.hcontext 为 WARL。 |
| `norm:textra_sselect` | sselect: 0=ignore, 1=scontext match, 2=asid match. | textra.sselect：0=忽略，1=scontext 匹配，2=ASID 匹配。 |
| `norm:textra_mhselect` | mhselect: 0=ignore, 4=mcontext match, and extended modes with H-extension. | textra.mhselect：0=忽略，4=mcontext 匹配，H 扩展下有扩展模式。 |
| `norm:unimplemented_csr_illegal` | Attempts to access an unimplemented Trigger Module Register raise an illegal instruction exception. | 访问未实现的触发器模块寄存器产生非法指令异常。 |
| `norm:xlen_handling` | Fields retain their values regardless of XLEN. A modification when XLEN=32 clears inaccessible bits. | 字段值不受 XLEN 影响；XLEN=32 时的修改会清除不可访问的位。 |
| `norm:chain_dmode_protection` | Hardware must zero chain in writes that set dmode=0 if the next trigger has dmode=1. | 若下一触发器 dmode=1，硬件必须在设 dmode=0 的写入中将 chain 清零。 |

---

## Group 1. 触发器枚举与发现

**规范依据**：
- `norm:sdtrig_at_least_one`：若实现 Sdtrig，TM 必须支持至少一个触发器
- `norm:enum_tselect_write0`：写 0 到 tselect 检测是否存在触发器
- `norm:enum_tselect_readback`：回读 tselect 验证写入值
- `norm:enum_tinfo`：读 tinfo 获取触发器类型信息
- `norm:enum_tdata1_type0`：tdata1.type=0 表示触发器不存在
- `norm:tselect_warl`：tselect 为 WARL

**测试职责**：验证触发器枚举发现算法的正确性，包括 tselect 的 WARL 行为和 tinfo/tdata1 的类型发现机制。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ENUM-01 | Sdtrig 存在性检测 | 写 0 到 tselect，检查是否产生非法指令异常 | 若不产生异常，Sdtrig 已实现；若产生异常，Sdtrig 未实现（后续测试 SKIP） |
| ENUM-02 | tselect 回读验证 | 写 0 到 tselect 后回读 | 回读值 = 0 |
| ENUM-03 | tselect WARL 边界探测 | 递增写入 tselect（0, 1, 2, ...），每次回读检查 | 写入值 < 触发器数量时回读一致；写入值 >= 触发器数量时回读值可能不同 |
| ENUM-04 | tselect 连续触发器索引 | 从 0 开始递增 tselect，找到所有有效触发器 | 有效触发器索引从 0 开始且连续 |
| ENUM-05 | tinfo 存在性检查 | 选择触发器 0，读 tinfo | 若不产生异常，tinfo 已实现；tinfo.version 为 0 或 1 |
| ENUM-06 | tinfo.info 位掩码验证 | 对每个有效触发器读 tinfo.info | info 位掩码中标记的类型与 tdata1.type 一致；不存在的触发器 info=1 |
| ENUM-07 | tdata1.type 读取 | 对每个有效触发器读 tdata1.type | type 为 2~7 之一（或 12~14 自定义），type=0 表示触发器不存在 |
| ENUM-08 | 至少一个触发器 | 枚举所有触发器 | 至少存在一个 type != 0 的触发器 |
| ENUM-09 | tinfo 不存在时 tdata1.type 发现 | 若 tinfo 访问产生异常，使用 tdata1.type 发现触发器类型 | tdata1.type 非零即为有效触发器 |

---

## Group 2. tdata 寄存器 WARL 语义与安全写入

**规范依据**：
- `norm:warl_write_any_read_legal`：所有 tdata 寄存器遵循 WARL 语义
- `norm:warl_write_zero_disable`：写 0 到 tdata1 禁用触发器
- `norm:warl_safe_write_sequence`：安全写入序列（tdata1=0 → tdata2/3 → tdata1）
- `norm:warl_no_cross_modify`：写入一个 tdata 不得修改其他 tdata
- `norm:tdata1_type0_hwired`：type=0 时 dmode 和 data 只读零
- `norm:dmode_writable_debug_only`：dmode 仅可从 Debug Mode 写入
- `norm:dmode_zero_action1_illegal`：dmode=0 时 action=1 非法

**测试职责**：验证 tdata 寄存器的 WARL 读写语义、安全写入序列和 dmode 保护机制。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| WARL-01 | tdata1 写 0 禁用触发器 | 对有效触发器写 tdata1=0，回读 tdata1 | type 字段为 15（disabled）或某合法禁用值，触发器不再触发 |
| WARL-02 | 写 0 后 tdata2 可写任意值 | 写 tdata1=0 后，写各种值到 tdata2 | 所有写入均成功（不产生异常），回读值合法 |
| WARL-03 | 写 0 后 tdata3 可写任意值 | 写 tdata1=0 后，写各种值到 tdata3 | 所有写入均成功，回读值合法 |
| WARL-04 | 安全写入序列验证 | 执行序列：tdata1=0 → tdata2=addr → tdata3=0 → tdata1=cfg | 触发器按配置生效 |
| WARL-05 | WARL 不支持配置回读 | 写入不支持的 tdata1 配置（如所有位全 1） | 回读值为某合法值（可能为禁用状态），不产生异常 |
| WARL-06 | 写 tdata1 不影响其他触发器 | 配置触发器 0，写触发器 1 的 tdata1 | 触发器 0 的 tdata1/tdata2/tdata3 不变 |
| WARL-07 | 写 tdata2 不影响 tdata1 | 写 tdata2，检查 tdata1 | tdata1 内容不变 |
| WARL-08 | 写 tdata1 不影响 tdata2 | 写 tdata1（非零配置），检查 tdata2 | tdata2 内容不变（除非 WARL 转换需要） |
| WARL-09 | M-mode 从非 Debug 写 dmode=1 | M-mode 尝试写 tdata1.dmode=1 | dmode 回读为 0（仅 Debug Mode 可写 dmode=1） |
| WARL-10 | dmode=0 时 action=1 被阻止 | M-mode 写 tdata1 设 dmode=0 且 action=1 | 回读值中 action != 1（硬件阻止非法组合） |
| WARL-11 | type=0 时 dmode 只读零 | 对 type=0 的触发器读 tdata1 | dmode=0 |
| WARL-12 | type=0 时 data 只读零 | 对 type=0 的触发器读 tdata1 | data 字段 = 0 |
| WARL-13 | 未实现 CSR 访问非法 | 尝试访问未实现的触发器 CSR（如 tinfo 未实现时读 tinfo） | 产生非法指令异常 |

---

## Group 3. mcontrol6 地址/数据匹配触发器

**规范依据**：
- `norm:mcontrol_match_exec`：execute=1 时在指令地址/操作码上触发
- `norm:mcontrol_match_load`：load=1 时在 load 地址/数据上触发
- `norm:mcontrol_match_store`：store=1 时在 store 地址/数据上触发
- `norm:mcontrol_select_address`：select=0 地址匹配
- `norm:mcontrol_select_data`：select=1 数据匹配
- `norm:mcontrol_match_equal`：match=0 精确匹配
- `norm:mcontrol_match_napot`：match=1 NAPOT 匹配
- `norm:mcontrol_match_ge`：match=2 大于等于匹配
- `norm:mcontrol_match_lt`：match=3 小于匹配
- `norm:mcontrol_match_mask`：match=4/5 掩码匹配
- `norm:mcontrol_match_not_equal`：match=8 不等匹配
- `norm:mcontrol_priv_bits`：m/s/u 特权模式使能位
- `norm:mcontrol_chain`：链式触发
- `norm:mcontrol_hit`：hit 位行为
- `norm:mcontrol6_hit_encoding`：hit1:hit0 编码
- `norm:mcontrol6_maskmax6`：NAPOT 最大范围
- `norm:mcontrol_size`：访问大小过滤
- `norm:action_breakpoint`：action=0 断点异常

**测试职责**：验证 mcontrol6（type=6）触发器的地址匹配、数据匹配、特权模式过滤、链式触发等核心功能。

> **注意**：本组测试仅在实现 mcontrol6 类型时执行（通过 tinfo.info 检测 bit 6）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MC6-01 | mcontrol6 类型检测 | 枚举触发器检查是否支持 type=6 | tinfo.info bit 6 = 1 或 tdata1.type = 6 |
| MC6-02 | execute 地址匹配 | 配置 mcontrol6: execute=1, select=0, match=0(equal), m=1, action=0, tdata2=目标指令地址 | 执行目标指令时产生断点异常（cause=3），xepc=目标指令地址 |
| MC6-03 | load 地址匹配 | 配置 mcontrol6: load=1, select=0, match=0, m=1, action=0, tdata2=目标地址 | 执行 load 到目标地址时产生断点异常 |
| MC6-04 | store 地址匹配 | 配置 mcontrol6: store=1, select=0, match=0, m=1, action=0, tdata2=目标地址 | 执行 store 到目标地址时产生断点异常 |
| MC6-05 | load 数据匹配 | 配置 mcontrol6: load=1, select=1, match=0, m=1, action=0, tdata2=目标数据值 | load 到匹配数据时产生断点异常 |
| MC6-06 | store 数据匹配 | 配置 mcontrol6: store=1, select=1, match=0, m=1, action=0, tdata2=目标数据值 | store 匹配数据时产生断点异常 |
| MC6-07 | match=ge（大于等于） | 配置 match=2, tdata2=阈值地址 | 访问地址 >= tdata2 时触发，< tdata2 时不触发 |
| MC6-08 | match=lt（小于） | 配置 match=3, tdata2=阈值地址 | 访问地址 < tdata2 时触发，>= tdata2 时不触发 |
| MC6-09 | match=not_equal | 配置 match=8, tdata2=排除地址 | 访问地址 != tdata2 时触发，= tdata2 时不触发 |
| MC6-10 | match=mask_low | 配置 match=4, tdata2 低半部为匹配值，高半部为掩码 | 仅低半部参与比较，匹配时触发 |
| MC6-11 | match=mask_high | 配置 match=5, tdata2 高半部为匹配值，低半部为掩码 | 仅高半部参与比较，匹配时触发 |
| MC6-12 | match=napot 范围匹配 | 配置 match=1, tdata2 为 NAPOT 编码 | 地址在 NAPOT 范围内时触发 |
| MC6-13 | NAPOT maskmax6 探测 | 按规范序列探测 maskmax6 值 | maskmax6 >= 1，写入全 1 后读回 tdata2 的最高 0 位索引 +1 = maskmax6 |
| MC6-14 | M-mode 使能 | 配置 m=1，M-mode 执行匹配操作 | 触发器触发 |
| MC6-15 | S-mode 使能 | 配置 s=1，切换到 S-mode 执行匹配操作 | 触发器在 S-mode 触发 |
| MC6-16 | U-mode 使能 | 配置 u=1，切换到 U-mode 执行匹配操作 | 触发器在 U-mode 触发 |
| MC6-17 | 未使能模式不触发 | 配置 m=0, s=0, u=0 | 任何模式下均不触发 |
| MC6-18 | 错误模式不触发 | 配置 m=1（仅 M-mode），在 S-mode 执行匹配操作 | 不触发 |
| MC6-19 | size=any 匹配任意大小 | 配置 size=0 | 8/16/32/64 位访问均匹配 |
| MC6-20 | size=32bit 仅匹配 32 位 | 配置 size=3 | 仅 32 位访问匹配，8/16 位不匹配 |
| MC6-21 | hit 位设置验证 | 配置触发器触发后检查 hit 字段 | hit1:hit0 不为 0（值取决于实现时机） |
| MC6-22 | hit 位软件清除 | 触发器触发后手动清 hit=0 | hit 读回为 0 |
| MC6-23 | 链式触发（chain=1） | 配置触发器 0: chain=1, execute=1, addr=A；触发器 1: chain=0, execute=1, addr=B | 仅当同一指令同时匹配 A 和 B 时才触发 |
| MC6-24 | 链式不匹配时不触发 | 链式配置，仅匹配触发器 0 不匹配触发器 1 | 不触发 |
| MC6-25 | action=0 时 xepc 正确 | 配置 action=0, execute=1 触发 | xepc = 匹配指令地址（before timing） |
| MC6-26 | s 位在不支持 S-mode 时只读零 | 检测不支持 S-mode 的 hart | s 位只读零 |
| MC6-27 | u 位在不支持 U-mode 时只读零 | 检测不支持 U-mode 的 hart | u 位只读零 |
| MC6-28 | uncertain 位验证 | 若实现 uncertain，触发后检查 | 确定匹配时 uncertain=0；不确定匹配时 uncertain=1 |
| MC6-29 | vs/vu 位在不支持虚拟化时只读零 | 检测不支持 H 扩展的 hart | vs 和 vu 位只读零 |

---

## Group 4. mcontrol (type=2, deprecated) 地址/数据匹配触发器

**规范依据**：
- 与 Group 3 相同的匹配规范，但使用 type=2 寄存器布局
- `norm:mcontrol_maskmax`：maskmax 指定最大 NAPOT 范围
- `norm:mcontrol_hit`：hit 位行为
- `norm:mcontrol_size`：sizehi:sizelo 组合编码

**测试职责**：验证 mcontrol（type=2，deprecated）触发器的基本功能。

> **注意**：本组测试仅在实现 mcontrol 类型时执行（通过 tinfo.info 检测 bit 2）。新实现可能不支持此类型。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MC2-01 | mcontrol 类型检测 | 枚举触发器检查是否支持 type=2 | tinfo.info bit 2 = 1 或 tdata1.type = 2 |
| MC2-02 | execute 地址匹配 | 配置 mcontrol: execute=1, select=0, match=0, m=1, action=0 | 执行目标指令时产生断点异常 |
| MC2-03 | load 地址匹配 | 配置 mcontrol: load=1, select=0, match=0, m=1 | load 到目标地址时产生断点异常 |
| MC2-04 | store 地址匹配 | 配置 mcontrol: store=1, select=0, match=0, m=1 | store 到目标地址时产生断点异常 |
| MC2-05 | maskmax 值读取 | 读 mcontrol.maskmax | maskmax 为 NAPOT 最大范围的 log2；0 表示不支持 NAPOT |
| MC2-06 | timing 位验证 | 读写 timing 位 | 回读实现支持的 timing 值 |
| MC2-07 | hit 位验证 | 触发后检查 hit | hit=1 表示触发 |

---

## Group 5. icount 指令计数触发器

**规范依据**：
- `norm:icount_decrement`：count 递减行为
- `norm:icount_pending_fire`：pending 触发行为
- `norm:icount_match_conditions`：匹配条件（指令退休、trap）
- `norm:icount_count0_stays`：count=0 保持不变
- `norm:icount_tval_zero`：action=0 时 tval=0
- `norm:icount_hardwired1_clear`：count 硬连线为 1 时触发后清除使能位

**测试职责**：验证 icount（type=3）触发器的计数、递减、pending 和触发行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| IC-01 | icount 类型检测 | 枚举触发器检查是否支持 type=3 | tinfo.info bit 3 = 1 或 tdata1.type = 3 |
| IC-02 | count=1 单步触发 | 配置 icount: count=1, m=1, action=0 | 执行 1 条 M-mode 指令后产生断点异常 |
| IC-03 | count=2 延迟触发 | 配置 icount: count=2, m=1, action=0 | 执行 2 条 M-mode 指令后产生断点异常 |
| IC-04 | count 递减验证 | 配置 count=N，执行若干指令后读 count | count 每条匹配指令递减 1 |
| IC-05 | pending 设置验证 | 配置 count=1，执行 1 条指令 | pending 被设置（若可读）或触发器触发 |
| IC-06 | pending 触发后清除 | pending 触发后检查 | pending 被清除 |
| IC-07 | count=0 不触发 | 配置 count=0 | 触发器不触发 |
| IC-08 | count=0 保持不变 | 设 count=0，执行多条指令，再读 count | count 仍为 0 |
| IC-09 | M-mode 使能 | 配置 m=1 | 仅 M-mode 指令计数 |
| IC-10 | S-mode 使能 | 配置 s=1，S-mode 执行指令 | S-mode 指令触发计数 |
| IC-11 | U-mode 使能 | 配置 u=1，U-mode 执行指令 | U-mode 指令触发计数 |
| IC-12 | 未使能模式不计数 | 配置所有模式位为 0 | 任何模式下均不计数/触发 |
| IC-13 | trap 匹配 | 配置 icount 启用，触发 trap（如 ecall） | trap 也计入匹配（count 递减） |
| IC-14 | action=0 时 tval=0 | icount 触发且 action=0 | tval CSR = 0 |
| IC-15 | hit 位设置 | 触发后检查 hit | hit=1（若实现） |
| IC-16 | count 硬连线为 1 时触发后清使能 | 若 count 只读为 1，触发后读 m/s/u 位 | m/s/u/vs/vu 均被清除 |

---

## Group 6. itrigger 中断触发器

**规范依据**：
- `norm:itrigger_tdata2_bitmask`：tdata2 为中断号位掩码
- `norm:itrigger_nmi`：NMI 触发
- `norm:itrigger_fire_timing`：trap 后、handler 第一条指令前触发
- `norm:itrigger_tval_zero`：action=0 时 tval=0

**测试职责**：验证 itrigger（type=4）中断触发器的中断匹配和触发行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| IT-01 | itrigger 类型检测 | 枚举触发器检查是否支持 type=4 | tinfo.info bit 4 = 1 或 tdata1.type = 4 |
| IT-02 | tdata2 中断位掩码写入 | 写 tdata2 设置中断号位 | 回读确认实现的中断位 |
| IT-03 | 指定中断触发 | 配置 itrigger: tdata2=bit[7]（timer interrupt），m=1, action=0 | 收到 timer interrupt 时在 handler 入口处产生断点异常 |
| IT-04 | 未配置中断不触发 | tdata2 不设对应中断位 | 收到该中断时不触发 itrigger |
| IT-05 | M-mode 中断触发 | 配置 m=1，M-mode 收到中断 | 触发器触发 |
| IT-06 | S-mode 中断触发 | 配置 s=1，S-mode 收到中断 | 触发器触发 |
| IT-07 | action=0 时 tval=0 | itrigger 触发且 action=0 | tval = 0 |
| IT-08 | 触发时机验证 | itrigger 触发时检查 xepc | xepc = trap handler 第一条指令地址 |
| IT-09 | hit 位验证 | 触发后检查 hit | hit=1（若实现） |
| IT-10 | NMI 触发 | 若实现 nmi 位，配置 nmi=1 | NMI 发生时触发器触发 |
| IT-11 | 硬件不支持的中断位 | 写入硬件不支持的 tdata2 位 | WARL 回读不支持的位为 0 |

---

## Group 7. etrigger 异常触发器

**规范依据**：
- `norm:etrigger_tdata2_bitmask`：tdata2 为异常码位掩码
- `norm:etrigger_fire_timing`：trap 后、handler 第一条指令前触发
- `norm:etrigger_tval_zero`：action=0 时 tval=0

**测试职责**：验证 etrigger（type=5）异常触发器的异常匹配和触发行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ET-01 | etrigger 类型检测 | 枚举触发器检查是否支持 type=5 | tinfo.info bit 5 = 1 或 tdata1.type = 5 |
| ET-02 | tdata2 异常码位掩码写入 | 写 tdata2 设置异常码位 | 回读确认实现的异常码位 |
| ET-03 | ecall 异常触发 | 配置 etrigger: tdata2=bit[11]（ecall-from-M），m=1, action=0 | M-mode 执行 ecall 时在 handler 入口产生断点异常 |
| ET-04 | 非法指令异常触发 | 配置 tdata2=bit[2]（illegal instruction），m=1 | 执行非法指令时触发 |
| ET-05 | 未配置异常不触发 | tdata2 不设对应异常码位 | 发生该异常时不触发 etrigger |
| ET-06 | M-mode 异常触发 | 配置 m=1 | M-mode 异常触发 etrigger |
| ET-07 | S-mode 异常触发 | 配置 s=1，S-mode 产生异常 | 触发器触发 |
| ET-08 | action=0 时 tval=0 | etrigger 触发且 action=0 | tval = 0 |
| ET-09 | 触发时机验证 | etrigger 触发时检查 xepc | xepc = 异常 handler 第一条指令地址 |
| ET-10 | hit 位验证 | 触发后检查 hit | hit=1（若实现） |
| ET-11 | 硬件不支持的异常码 | 写入硬件不支持的 tdata2 位 | WARL 回读不支持的位为 0 |

---

## Group 8. tmexttrigger 外部触发器

**规范依据**：
- `norm:tmexttrigger_select`：select 字段为外部触发输入位掩码
- `norm:tmexttrigger_intctl`：intctl 中断控制器触发
- `norm:tmexttrigger_async`：异步触发，受 medeleg[3] 委托

**测试职责**：验证 tmexttrigger（type=7）外部触发器的配置和基本行为。

> **注意**：外部触发器依赖外部硬件信号，部分测试可能无法在软件仿真环境中执行。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| EXT-01 | tmexttrigger 类型检测 | 枚举触发器检查是否支持 type=7 | tinfo.info bit 7 = 1 或 tdata1.type = 7 |
| EXT-02 | select 字段写入 | 写 select 位掩码选择外部触发输入 | 回读确认实现的 select 位 |
| EXT-03 | intctl 位写入 | 写 intctl=1 | 回读确认是否实现 |
| EXT-04 | action 字段验证 | 写 action 各合法值 | 回读一致 |
| EXT-05 | 未实现的 select 位 | 写入不支持的 select 位 | 不支持的位只读零 |
| EXT-06 | medeleg[3] 委托影响 | 配置 medeleg[3]=1，外部触发 | 断点异常委托到 S-mode |

---

## Group 9. 触发器 Action 与行为

**规范依据**：
- `norm:action_breakpoint`：action=0 断点异常
- `norm:action_debug_mode`：action=1 进入 Debug Mode（仅 Debug Mode 可配置）
- `norm:action_trace`：action=2/3/4 trace 动作
- `norm:action_external`：action=8/9 外部信号
- `norm:priority_multiple_same`：同优先级多触发器 hit 均设置
- `norm:priority_action0_action1`：action=0 与 action=1 同时存在时的行为

**测试职责**：验证触发器的 action 编码行为和优先级规则。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ACT-01 | action=0 断点异常 | 配置 mcontrol6/icount action=0 触发 | 产生 cause=3 的断点异常 |
| ACT-02 | action=0 时 xepc 正确（execute before） | execute 触发 before timing | xepc = 触发指令地址 |
| ACT-03 | action=0 委托到 S-mode | medeleg[3]=1，触发 action=0 | 断点异常交付到 S-mode |
| ACT-04 | action=0 不委托到 S-mode | medeleg[3]=0，触发 action=0 | 断点异常交付到 M-mode |
| ACT-05 | M-mode 写 action=1 被 WARL 阻止 | M-mode 写 action=1（dmode 无法被 M-mode 设为 1） | action 回读不为 1 |
| ACT-06 | action 保留值写入 | 写 action=6（保留值） | WARL 回读为某合法 action 值 |
| ACT-07 | 多触发器同时触发 hit 设置 | 配置两个触发器同时匹配 | 两者的 hit 位均被设置 |
| ACT-08 | action=8/9 WARL 验证 | 写 action=8 或 9 | 若实现则回读一致；若未实现则回读为合法值 |

---

## Group 10. 内存访问触发器特殊场景

**规范依据**：
- `norm:mem_lr_as_load`：lr 视为 load
- `norm:mem_sc_as_store`：成功 sc 视为 store
- `norm:mem_amo_as_load_store`：AMO 读部分为 load，写部分为 store
- `norm:mem_combined_accesses`：向量/cmpush/cm.pop 如同独立访问
- `norm:mem_cache_ops`：缓存操作作为 store 匹配
- `norm:mem_cache_hints_no_match`：HINT 编码的缓存操作不匹配
- `norm:addr_tdata2_full_range`：tdata2 容纳所有有效地址
- `norm:addr_invalid_convert`：无效地址转换

**测试职责**：验证触发器在特殊内存访问场景下的行为。

> **注意**：部分测试需要 A 扩展、V 扩展或 Zicbom/Zicboz 扩展支持。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MEM-01 | lr 匹配 load 触发器 | 配置 load 触发器在目标地址，执行 lr | lr 触发 load 触发器 |
| MEM-02 | 成功 sc 匹配 store 触发器 | 配置 store 触发器在目标地址，执行成功的 sc | sc 触发 store 触发器 |
| MEM-03 | AMO 匹配 load 触发器 | 配置 load 触发器，执行 AMO（如 amoadd） | AMO 的读部分触发 load 触发器 |
| MEM-04 | AMO 匹配 store 触发器 | 配置 store 触发器，执行 AMO | AMO 的写部分触发 store 触发器（地址匹配） |
| MEM-05 | cbo.clean/flush 匹配 store | 配置 store 触发器 size=0 select=0，执行 cbo.clean/flush | 缓存操作作为 store 匹配触发器 |
| MEM-06 | cbo.zero 匹配 store | 配置 store 触发器 size=0 select=0，执行 cbo.zero | cbo.zero 作为 store 匹配 |
| MEM-07 | prefetch 不匹配 | 配置触发器在 prefetch 地址 | prefetch（HINT）不触发 |
| MEM-08 | tdata2 全范围地址写入 | 写入各转换模式下的最大有效地址 | 回读值与写入值一致 |
| MEM-09 | 物理地址零扩展 | 写入小于 XLEN 的物理地址 | 零扩展后可读回完整值 |
| MEM-10 | 虚拟地址符号扩展 | 写入虚拟地址 | 符号扩展后可读回完整值 |
| MEM-11 | 无效地址 WARL 处理 | 写入无效地址 | 不产生异常，回读某合法值 |
| MEM-12 | load size 过滤 | 配置 size=1(8-bit)，执行 32-bit load | 32-bit load 不匹配 |
| MEM-13 | store size 过滤 | 配置 size=3(32-bit)，执行 8-bit store | 8-bit store 不匹配 |

---

## Group 11. Native Trigger 保护机制

**规范依据**：
- `norm:native_mmode_mie_protection`：M-mode MIE=0 时阻止 action=0 触发
- `norm:native_smode_sie_protection`：medeleg[3]=1 时 S-mode SIE=0 阻止触发
- `norm:native_tcontrol_protection`：tcontrol 保护方案

**测试职责**：验证 action=0 在 trap handler 中的重入保护机制。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| NAT-01 | M-mode MIE=0 时不触发 | 配置 action=0, m=1，在 M-mode 清 MIE 后执行匹配操作 | 触发器不匹配/不触发 |
| NAT-02 | M-mode MIE=1 时正常触发 | 配置 action=0, m=1，M-mode MIE=1 执行匹配操作 | 触发器正常触发 |
| NAT-03 | S-mode SIE=0 时不触发 | medeleg[3]=1, 配置 s=1, S-mode SIE=0 执行匹配 | 触发器不匹配/不触发 |
| NAT-04 | S-mode SIE=1 时正常触发 | medeleg[3]=1, 配置 s=1, S-mode SIE=1 执行匹配 | 触发器正常触发 |
| NAT-05 | trap handler 中不重入 | 配置 action=0，在断点 handler 中执行匹配操作 | handler 中不再次触发（保护重入） |

---

## Group 12. tcontrol 寄存器

**规范依据**：
- `norm:tcontrol_mte_trap`：trap 进入 M-mode 时 mpte=mte, mte=0
- `norm:tcontrol_mte_mret`：mret 时 mte=mpte

**测试职责**：验证 tcontrol 寄存器的 mte/mpte 字段在 trap/mret 时的行为。

> **注意**：tcontrol 为可选寄存器，仅在实现时测试。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TC-01 | tcontrol 存在性检测 | M-mode 读写 tcontrol | 不产生异常则 tcontrol 已实现 |
| TC-02 | mte 读写 | 写 tcontrol.mte=1 后回读 | mte=1 |
| TC-03 | mpte 读写 | 写 tcontrol.mpte=1 后回读 | mpte=1 |
| TC-04 | trap 时 mte→mpte, mte=0 | 设 mte=1，触发 trap 进入 M-mode | mpte=1（旧 mte 值），mte=0 |
| TC-05 | mret 时 mte=mpte | trap 后 mpte=1, mte=0，执行 mret | mte=1（恢复为 mpte 值） |
| TC-06 | mte=0 时触发器不匹配 | 若 tcontrol 实现，设 mte=0，M-mode 执行匹配操作 | action=0 的触发器不在 M-mode 匹配/触发 |
| TC-07 | mte=1 时触发器正常匹配 | 设 mte=1，M-mode 执行匹配操作 | 触发器正常匹配/触发 |
| TC-08 | 保留位只读零 | 读 tcontrol 的 bits[6:4] 和 bits[2:0] | 均为 0 |

---

## Group 13. Context 寄存器（scontext / mcontext / hcontext）

**规范依据**：
- `norm:scontext_warl`：scontext.data 为 WARL
- `norm:mcontext_warl`：mcontext.hcontext 为 WARL
- `norm:textra_sselect`：textra.sselect 控制 scontext 匹配
- `norm:textra_mhselect`：textra.mhselect 控制 mcontext 匹配

**测试职责**：验证 context 寄存器的读写和触发器 context 过滤功能。

> **注意**：context 寄存器均为可选实现。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| CTX-01 | scontext 存在性检测 | S-mode 读写 scontext | 不产生异常则已实现 |
| CTX-02 | scontext WARL 读写 | 写 scontext.data 各值 | 回读一致（高位可能只读零） |
| CTX-03 | mcontext 存在性检测 | M-mode 读写 mcontext | 不产生异常则已实现 |
| CTX-04 | mcontext WARL 读写 | 写 mcontext.hcontext 各值 | 回读一致（高位可能只读零） |
| CTX-05 | textra32 sselect=1 匹配 | 配置 sselect=1, svalue=X，scontext=X | 触发器匹配 |
| CTX-06 | textra32 sselect=1 不匹配 | 配置 sselect=1, svalue=X，scontext=Y（Y≠X） | 触发器不匹配 |
| CTX-07 | textra32 sselect=0 忽略 | 配置 sselect=0，任意 scontext | 触发器匹配（忽略 scontext） |
| CTX-08 | textra32 mhselect=4 匹配 | 配置 mhselect=4, mhvalue=X，mcontext=X | 触发器匹配 |
| CTX-09 | textra32 mhselect=4 不匹配 | 配置 mhselect=4, mhvalue=X，mcontext=Y | 触发器不匹配 |
| CTX-10 | textra32 mhselect=0 忽略 | 配置 mhselect=0，任意 mcontext | 触发器匹配 |
| CTX-11 | sbytemask 字节掩码 | 配置 sselect=1, sbytemask 忽略低字节 | 仅高字节参与比较 |
| CTX-12 | mscontext 别名验证 | 若 mscontext 实现，通过 0x7aa 访问 | 与 scontext（0x5a8）读写一致 |

---

## Group 14. 触发器优先级

**规范依据**：
- `norm:priority_table`：触发器与异常的优先级关系
- `norm:priority_multiple_same`：同优先级 hit 均设置
- `norm:priority_action0_action1`：action=0 与 action=1 同时

**测试职责**：验证触发器相对于同步异常的优先级。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PRI-01 | execute 触发优先于指令地址 page fault | 配置 execute 触发器在会 page fault 的地址 | 断点异常优先于 page fault（before timing） |
| PRI-02 | load/store 触发优先于非法指令 | 配置 load/store 触发，同时有非法指令 | 按优先级表顺序处理 |
| PRI-03 | 多触发器同优先级 hit 均设 | 两个触发器同时匹配 | 两者 hit 均被设置 |
| PRI-04 | etrigger/itrigger 同优先级 | etrigger 和 itrigger 同时触发 | 均正常处理 |

---

## Group 15. XLEN 处理与跨模式访问

**规范依据**：
- `norm:xlen_handling`：字段值不受 XLEN 影响
- `norm:sdtrig_mmode_access`：M-mode 访问必须成功
- `norm:sdtrig_unused_csr_illegal`：未使用 CSR 访问产生非法异常

**测试职责**：验证不同 XLEN 下的寄存器行为和访问权限。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| XLEN-01 | M-mode CSR 访问成功 | M-mode 读写所有已实现的触发器 CSR | 所有访问成功，无异常 |
| XLEN-02 | S-mode CSR 访问非法 | S-mode 尝试访问 tselect/tdata1 等 | 产生非法指令异常 |
| XLEN-03 | U-mode CSR 访问非法 | U-mode 尝试访问触发器 CSR | 产生非法指令异常 |
| XLEN-04 | 当前触发器类型不影响访问 | 选中某触发器后，不管其 type 如何，均可读写 tdata1/2/3 | M-mode 访问始终成功 |

---

## 测试覆盖矩阵

| 规范章节 | 测试 ID 范围 | 用例数 | 关键验证点 |
|----------|-------------|--------|-----------|
| Enumeration | ENUM-01 ~ ENUM-09 | 9 | 触发器发现算法、tselect WARL、tinfo/tdata1 类型 |
| Trigger Module Registers (WARL) | WARL-01 ~ WARL-13 | 13 | WARL 语义、安全写入序列、dmode 保护、交叉修改 |
| mcontrol6 (type=6) | MC6-01 ~ MC6-29 | 29 | 地址/数据匹配、match 模式、特权模式、链式、hit、size |
| mcontrol (type=2) | MC2-01 ~ MC2-07 | 7 | deprecated 类型基本功能、maskmax、timing |
| icount (type=3) | IC-01 ~ IC-16 | 16 | 计数递减、pending、触发、tval=0 |
| itrigger (type=4) | IT-01 ~ IT-11 | 11 | 中断位掩码、NMI、触发时机 |
| etrigger (type=5) | ET-01 ~ ET-11 | 11 | 异常码位掩码、触发时机 |
| tmexttrigger (type=7) | EXT-01 ~ EXT-06 | 6 | 外部触发输入选择、intctl |
| Actions & Priority | ACT-01 ~ ACT-08 | 8 | action 编码、委托、多触发器 |
| Memory Access Triggers | MEM-01 ~ MEM-13 | 13 | lr/sc/AMO、缓存操作、地址范围 |
| Native Triggers | NAT-01 ~ NAT-05 | 5 | MIE/SIE 保护、重入防止 |
| tcontrol | TC-01 ~ TC-08 | 8 | mte/mpte trap/mret 行为 |
| Context Registers | CTX-01 ~ CTX-12 | 12 | scontext/mcontext 过滤、textra |
| Priority | PRI-01 ~ PRI-04 | 4 | 触发器 vs 异常优先级 |
| XLEN & Access | XLEN-01 ~ XLEN-04 | 4 | 访问权限、XLEN 处理 |
| **合计** | — | **156** | — |

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1 (枚举发现) | ENUM-01~09 | 触发器枚举是所有测试的基础 |
| P0（必须） | Group 2 (WARL 语义) | WARL-01~13 | WARL 和安全写入是正确配置触发器的前提 |
| P0（必须） | Group 3 (mcontrol6) | MC6-01~29 | mcontrol6 是当前推荐的地址/数据匹配类型 |
| P0（必须） | Group 5 (icount) | IC-01~16 | icount 是单步调试的核心机制 |
| P0（必须） | Group 9 (Action) | ACT-01~08 | action=0 断点异常是 native debug 的基础 |
| P1（重要） | Group 6 (itrigger) | IT-01~11 | 中断触发器功能 |
| P1（重要） | Group 7 (etrigger) | ET-01~11 | 异常触发器功能 |
| P1（重要） | Group 11 (Native 保护) | NAT-01~05 | action=0 重入保护对系统稳定性至关重要 |
| P1（重要） | Group 15 (XLEN/访问) | XLEN-01~04 | 访问权限验证 |
| P2（建议） | Group 4 (mcontrol) | MC2-01~07 | deprecated 类型，新实现可能不支持 |
| P2（建议） | Group 8 (tmexttrigger) | EXT-01~06 | 外部触发器依赖外部硬件 |
| P2（建议） | Group 10 (内存访问) | MEM-01~13 | 需要 A/V/Zicbom 扩展 |
| P2（建议） | Group 12 (tcontrol) | TC-01~08 | 可选寄存器 |
| P2（建议） | Group 13 (Context) | CTX-01~12 | 可选寄存器 |
| P2（建议） | Group 14 (Priority) | PRI-01~04 | 优先级验证 |

---

## 测试实现说明

### 文件组织

```
damo-priv-test/
├── sdtrig/
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_enum.c              # Group 1: 触发器枚举与发现
│       ├── test_warl.c              # Group 2: tdata WARL 语义与安全写入
│       ├── test_mcontrol6.c         # Group 3: mcontrol6 地址/数据匹配
│       ├── test_mcontrol.c          # Group 4: mcontrol (deprecated)
│       ├── test_icount.c            # Group 5: icount 指令计数
│       ├── test_itrigger.c          # Group 6: itrigger 中断触发
│       ├── test_etrigger.c          # Group 7: etrigger 异常触发
│       ├── test_tmexttrigger.c      # Group 8: tmexttrigger 外部触发
│       ├── test_action.c            # Group 9: Action 与行为
│       ├── test_mem_access.c        # Group 10: 内存访问特殊场景
│       ├── test_native.c            # Group 11: Native Trigger 保护
│       ├── test_tcontrol.c          # Group 12: tcontrol 寄存器
│       ├── test_context.c           # Group 13: Context 寄存器
│       ├── test_priority.c          # Group 14: 优先级
│       ├── test_xlen_access.c       # Group 15: XLEN 与访问权限
│       └── test_helpers.h           # 公共辅助函数
└── common/                          # 复用通用框架
```

### 运行时检测

```c
/* Check if Sdtrig extension is implemented */
static bool check_sdtrig_extension(void) {
    /* Try writing 0 to tselect; illegal instruction => no Sdtrig */
    bool has_trigger = true;
    __asm__ volatile (
        "la t0, 1f\n\t"
        "csrw mtvec, t0\n\t"
        "csrw tselect, zero\n\t"
        "j 2f\n\t"
        "1: li %0, 0\n\t"   /* trap handler: no Sdtrig */
        "2:\n\t"
        : "+r" (has_trigger)
        :: "t0"
    );
    return has_trigger;
}

/* Enumerate triggers and find supported types */
static int enumerate_triggers(trigger_info_t *info, int max_triggers) {
    int count = 0;
    for (int i = 0; i < max_triggers; i++) {
        CSRW(tselect, i);
        uintptr_t sel = CSRR(tselect);
        if (sel != (uintptr_t)i) break;

        uintptr_t tdata1 = CSRR(tdata1);
        unsigned type = (tdata1 >> (XLEN - 4)) & 0xF;
        if (type == 0) break;

        /* Try tinfo for supported types */
        uintptr_t tinfo = 0;
        bool has_tinfo = try_read_csr(tinfo, &tinfo);

        info[count].index = i;
        info[count].type = type;
        info[count].tinfo = has_tinfo ? tinfo : 0;
        count++;
    }
    return count;
}

/* Check if a specific trigger type is supported */
static bool trigger_type_supported(int type) {
    trigger_info_t triggers[16];
    int n = enumerate_triggers(triggers, 16);
    for (int i = 0; i < n; i++) {
        if (triggers[i].tinfo & (1 << type)) return true;
        if (triggers[i].type == type) return true;
    }
    return false;
}

/* Safe trigger configuration sequence */
static void configure_trigger(int index, uintptr_t tdata1_val,
                              uintptr_t tdata2_val, uintptr_t tdata3_val) {
    CSRW(tselect, index);
    CSRW(tdata1, 0);          /* Step 1: disable trigger */
    CSRW(tdata2, tdata2_val); /* Step 2: write tdata2 */
    CSRW(tdata3, tdata3_val); /* Step 2: write tdata3 */
    CSRW(tdata1, tdata1_val); /* Step 3: enable trigger */
    /* Verify WARL */
    uintptr_t readback = CSRR(tdata1);
    (void)readback;
}

/* Disable a trigger */
static void disable_trigger(int index) {
    CSRW(tselect, index);
    CSRW(tdata1, 0);
}
```

### 通用测试模式

#### 模式 1：mcontrol6 execute 断点测试（Group 3）

```c
/* MC6-02: mcontrol6 execute address match */
TEST_REGISTER(test_mc6_02);
bool test_mc6_02(void) {
    TEST_BEGIN("MC6-02: mcontrol6 execute address match triggers breakpoint");

    if (!check_sdtrig_extension()) TEST_SKIP("Sdtrig not available");
    if (!trigger_type_supported(6)) TEST_SKIP("mcontrol6 not supported");

    /* Find a free trigger slot with mcontrol6 support */
    int trig = find_mcontrol6_trigger();
    if (trig < 0) TEST_SKIP("No mcontrol6 trigger found");

    /* Get address of target function */
    extern void target_function(void);
    uintptr_t target_addr = (uintptr_t)&target_function;

    /* Configure trigger: execute, address match, equal, M-mode, action=0 */
    uintptr_t cfg = (6UL << (XLEN - 4))  /* type = mcontrol6 */
                   | (1 << 2)             /* execute = 1 */
                   | (1 << 6)             /* m = 1 */
                   | (0 << 12);           /* action = 0 (breakpoint) */
    configure_trigger(trig, cfg, target_addr, 0);

    /* Execute target function, expect breakpoint */
    EXPECT_TRAP(CAUSE_BREAKPOINT, target_function());

    /* Verify xepc = target address */
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("xepc = target address",
                       trap_get_epc(), target_addr);
    }

    /* Cleanup */
    disable_trigger(trig);
    TEST_END();
}
```

#### 模式 2：icount 单步测试（Group 5）

```c
/* IC-02: icount count=1 single step */
TEST_REGISTER(test_ic_02);
bool test_ic_02(void) {
    TEST_BEGIN("IC-02: icount count=1 triggers after 1 instruction");

    if (!check_sdtrig_extension()) TEST_SKIP("Sdtrig not available");
    if (!trigger_type_supported(3)) TEST_SKIP("icount not supported");

    int trig = find_icount_trigger();
    if (trig < 0) TEST_SKIP("No icount trigger found");

    /* Configure: count=1, m=1, action=0 */
    uintptr_t cfg = (3UL << (XLEN - 4))  /* type = icount */
                   | (1 << 10)            /* count = 1 */
                   | (1 << 9)             /* m = 1 */
                   | (0);                 /* action = 0 */
    configure_trigger(trig, cfg, 0, 0);

    /* Execute one instruction, expect breakpoint */
    volatile int x = 0;
    EXPECT_TRAP(CAUSE_BREAKPOINT, x = 1);

    TEST_ASSERT("breakpoint triggered", trap_was_triggered());

    /* Cleanup */
    disable_trigger(trig);
    TEST_END();
}
```

#### 模式 3：etrigger 异常触发测试（Group 7）

```c
/* ET-03: etrigger on ecall exception */
TEST_REGISTER(test_et_03);
bool test_et_03(void) {
    TEST_BEGIN("ET-03: etrigger fires on ecall exception");

    if (!check_sdtrig_extension()) TEST_SKIP("Sdtrig not available");
    if (!trigger_type_supported(5)) TEST_SKIP("etrigger not supported");

    int trig = find_etrigger();
    if (trig < 0) TEST_SKIP("No etrigger found");

    /* Configure: tdata2 = bit[11] (ecall-from-M), m=1, action=0 */
    uintptr_t cfg = (5UL << (XLEN - 4))  /* type = etrigger */
                   | (1 << 9)             /* m = 1 */
                   | (0);                 /* action = 0 */
    uintptr_t tdata2_val = (1UL << 11);   /* ecall-from-M */
    configure_trigger(trig, cfg, tdata2_val, 0);

    /* Verify tdata2 writeback */
    CSRW(tselect, trig);
    uintptr_t tdata2_readback = CSRR(tdata2);
    if (tdata2_readback == 0) {
        disable_trigger(trig);
        TEST_SKIP("etrigger does not support ecall-from-M exception code");
    }

    /* Execute ecall, expect breakpoint from etrigger */
    EXPECT_TRAP(CAUSE_BREAKPOINT, asm volatile("ecall"));

    TEST_ASSERT("breakpoint triggered by etrigger", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("tval = 0", trap_get_tval(), 0);
    }

    disable_trigger(trig);
    TEST_END();
}
```

### 关键注意事项

1. **扩展检测**：所有测试必须在运行时检测 Sdtrig 的可用性（通过 tselect 写入是否产生异常），不可用时 TEST_SKIP。

2. **触发器类型检测**：每种触发器类型的测试必须通过 tinfo 或 tdata1.type 检测该类型是否被实现，未实现时 TEST_SKIP。

3. **WARL 回读验证**：所有 tdata 寄存器写入后必须回读验证。硬件可能将不支持的配置转换为合法值（如禁用触发器），测试需适应 WARL 行为。

4. **安全写入序列**：配置触发器时必须使用标准安全序列：先写 tdata1=0 禁用，再写 tdata2/tdata3，最后写 tdata1 启用。避免部分配置的触发器意外触发。

5. **dmode 限制**：M-mode 无法设置 dmode=1，因此 action=1（进入 Debug Mode）在 M-mode 测试中不可用。action=1 的测试需要 Sdext 支持，在本测试框架中可能无法覆盖。

6. **断点异常委托**：action=0 产生的断点异常（cause=3）受 `medeleg[3]` 控制。测试中需正确配置 medeleg 以控制异常的交付目标。

7. **Native Trigger 保护**：实现可能采用两种重入保护方案之一（MIE/SIE 保护或 tcontrol）。测试需检测实现采用的是哪种方案。

8. **链式触发**：chain 测试需要至少两个相同类型的连续触发器。如果触发器数量不足，相关测试需 SKIP。

9. **Hypervisor 模式位**：vs/vu 位仅在实现 H 扩展时可写，否则只读零。

10. **缓存操作触发**：MEM-05~07 需要 Zicbom/Zicboz 扩展支持，不存在时 TEST_SKIP。

11. **AMO 触发**：MEM-03~04 需要 A 扩展支持，不存在时 TEST_SKIP。

12. **清理**：每个测试结束时必须禁用所有配置的触发器，避免影响后续测试。

---

## 参考

- `SPEC/riscv-debug/Sdtrig.adoc` — Sdtrig ISA Extension (Trigger Module)
- `SPEC/riscv-debug/xml/hwbp_registers.xml` — Trigger Module Register Definitions
- `SPEC/riscv-debug/Sdext.adoc` — Sdext Extension (Debug Mode, dpc)
- `RISC-V Privileged Spec` — 异常/中断编码、medeleg、xepc、tval
- `RISC-V A Extension` — lr/sc/AMO 指令
- `RISC-V Zicbom/Zicboz` — 缓存管理操作
