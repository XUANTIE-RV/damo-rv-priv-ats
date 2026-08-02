# Smcntrpmf 扩展测试计划

## 概述

本测试计划覆盖 RISC-V Smcntrpmf（Cycle and Instret Privilege Mode Filtering）扩展的所有核心功能点。

Smcntrpmf 扩展为 cycle 和 instret 计数器引入特权模式过滤功能。默认情况下，这两个计数器不按特权模式过滤，在 trap 到更高特权级代码处理期间仍继续递增。这会引入不可预测的噪声并向用户模式泄露特权软件执行信息。Smcntrpmf 通过 mcyclecfg/minstretcfg CSR 配置各特权模式的计数抑制。

本测试计划依据 `SPEC/smcntrpmf.adoc` 中的规范点（norm 标记）编写。

### 本文档覆盖的 SPEC 章节
- Machine Counter Configuration（mcyclecfg, minstretcfg CSR 行为）
- Counter Behavior（cycle/instret 在 inhibited 模式下的计数行为）
- Mode Transition Counting（模式切换时的计数定义）

### 前置依赖
- 需要实现 cycle 和 instret 计数器（mcounteren 相关配置）
- 若测试 Smcdeleg/Ssccfg 交互，需要对应扩展实现

### 由其他测试计划覆盖
- VSINH/VUINH 位相关的 VS/VU-mode 计数抑制测试（依赖 H 扩展）→ `Hypervisor_cross_test_plan.md` Group 16（Hypervisor × Smcntrpmf）

---

## 覆盖的规范点

本章节列出本文档所有测试组中引用的规范点（norm ID），已去重并按字母顺序排列。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:all_xinh_zero` | When all xINH bits are zero, event counting is enabled in all modes. | 当所有 xINH 位为零时，所有模式下事件计数均使能。 |
| `norm:counter_inhibited_behavior` | The fundamental behavior of cycle and instret is modified in that counting does not occur while executing in an inhibited privilege mode. | cycle 和 instret 的基本行为被修改：在被抑制的特权模式下执行时不发生计数。 |
| `norm:csr_supervisor_access` | The content of these registers may be accessible from Supervisor level if the Smcdeleg/Ssccfg extensions are implemented. | 若实现了 Smcdeleg/Ssccfg 扩展，这些寄存器的内容可从 Supervisor 级访问。 |
| `norm:cycle_counting` | The cycle counter will simply count CPU cycles while the CPU is in a non-inhibited privilege mode. Mode transition operations (traps and trap returns) may take multiple clock cycles, and the change of privilege mode may be reported as occurring in any one of those cycles. | cycle 计数器仅在 CPU 处于非抑制特权模式时计数。模式切换操作可能花费多个时钟周期，特权模式变化可报告为发生在其中任一周期。 |
| `norm:instret_exception` | The former (instructions that cause synchronous exceptions) are not considered to retire, and hence do not increment instret. | 引发同步异常的指令不被视为退休，因此不递增 instret。 |
| `norm:instret_non_inhibited` | Instructions that retire in a non-inhibited mode increment instret, and instructions that retire in an inhibited mode do not. | 在非抑制模式下退休的指令递增 instret，在抑制模式下退休的指令不递增。 |
| `norm:instret_xret` | The latter (xRET instructions) do retire, and should increment instret only if the originating privilege mode is not inhibited. | xRET 指令确实退休，但仅当源特权模式未被抑制时才递增 instret。 |
| `norm:mcyclecfg_op` | mcyclecfg and minstretcfg configure privilege mode filtering for the cycle and instret counters, respectively. | mcyclecfg 和 minstretcfg 分别配置 cycle 和 instret 计数器的特权模式过滤。 |
| `norm:mcyclecfg_sz` | mcyclecfg and minstretcfg are 64-bit registers. | mcyclecfg 和 minstretcfg 是 64 位寄存器。 |
| `norm:rv32_high_access` | For RV32, bits 63:32 of mcyclecfg can be accessed via the mcyclecfgh CSR, and bits 63:32 of minstretcfg can be accessed via the minstretcfgh CSR. | RV32 下，mcyclecfg 的 63:32 位通过 mcyclecfgh 访问，minstretcfg 的 63:32 位通过 minstretcfgh 访问。 |
| `norm:transition_counting_defined` | The following defines how transitions between a non-inhibited privilege mode and an inhibited privilege mode are counted. | 以下定义了非抑制特权模式与抑制特权模式之间切换的计数方式。 |
| `norm:unimplemented_mode_bits` | For each bit in 61:58, if the associated privilege mode is not implemented, the bit is read-only zero. | 61:58 位中，若对应特权模式未实现，该位为只读零。 |

---

## Group 1. mcyclecfg / minstretcfg CSR 访问与字段约束

**规范依据**：
- `norm:mcyclecfg_sz`：mcyclecfg 和 minstretcfg 是 64 位寄存器
- `norm:mcyclecfg_op`：配置 cycle 和 instret 计数器的特权模式过滤
- `norm:unimplemented_mode_bits`：61:58 位中未实现模式对应位为只读零
- `norm:rv32_high_access`：RV32 下高 32 位通过 mcyclecfgh/minstretcfgh 访问

**测试职责**：验证 mcyclecfg/minstretcfg CSR 的基本读写属性、字段布局及 WARL 约束。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PMF-CSR-01 | mcyclecfg 基本读写 | M-mode 写 mcyclecfg 的 MINH/SINH/UINH/VSINH/VUINH 各字段，读回验证 | 已实现字段可写可读，WPRI 位（57:0）读回为零 |
| PMF-CSR-02 | minstretcfg 基本读写 | M-mode 写 minstretcfg 的 MINH/SINH/UINH/VSINH/VUINH 各字段，读回验证 | 已实现字段可写可读，WPRI 位（57:0）读回为零 |
| PMF-CSR-03 | mcyclecfg bit 63 (OF) 只读零 | 写 mcyclecfg bit 63 = 1，读回 | bit 63 始终为 0（cycle 计数器不产生溢出中断） |
| PMF-CSR-04 | minstretcfg bit 63 (OF) 只读零 | 写 minstretcfg bit 63 = 1，读回 | bit 63 始终为 0 |
| PMF-CSR-06 | 未实现 S 模式时 SINH 只读零 | 若 S 模式未实现，写 SINH=1，读回 | SINH 为只读零 |
| PMF-CSR-07 | 未实现 U 模式时 UINH 只读零 | 若 U 模式未实现，写 UINH=1，读回 | UINH 为只读零 |
| PMF-CSR-08 | mcyclecfg WPRI 字段写忽略 | 写 mcyclecfg bits 57:0 为全 1，读回 | bits 57:0 读回全零 |
| PMF-CSR-09 | minstretcfg WPRI 字段写忽略 | 写 minstretcfg bits 57:0 为全 1，读回 | bits 57:0 读回全零 |
| PMF-CSR-10 | S-mode 访问 mcyclecfg 触发异常 | S-mode 尝试 csrr mcyclecfg | illegal-instruction exception (cause=2) |
| PMF-CSR-11 | U-mode 访问 mcyclecfg 触发异常 | U-mode 尝试 csrr mcyclecfg | illegal-instruction exception (cause=2) |
| PMF-CSR-12 | RV32 mcyclecfgh 访问高 32 位 | （RV32）M-mode 通过 mcyclecfgh 写 MINH/SINH/UINH 等位，读回 | 高 32 位正确读写 |
| PMF-CSR-13 | RV32 minstretcfgh 访问高 32 位 | （RV32）M-mode 通过 minstretcfgh 写 MINH/SINH/UINH 等位，读回 | 高 32 位正确读写 |

> [!NOTE]
> VSINH/VUINH 位相关的测试（原 PMF-CSR-05：未实现 H 扩展时 VSINH/VUINH 只读零）已移至 `Hypervisor_cross_test_plan.md` Group 16（Hypervisor × Smcntrpmf），因其依赖 H 扩展的实现状态。

---

## Group 2. cycle 计数器特权模式过滤

**规范依据**：
- `norm:counter_inhibited_behavior`：在被抑制的特权模式下不发生计数
- `norm:all_xinh_zero`：所有 xINH 为零时所有模式计数使能
- `norm:cycle_counting`：cycle 计数器仅在非抑制模式下计数 CPU 周期
- `norm:transition_counting_defined`：模式切换的计数定义

**测试职责**：验证 mcyclecfg 各 xINH 位对 cycle 计数器在不同特权模式下的抑制效果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PMF-CYC-01 | 所有 xINH=0 时全模式计数 | mcyclecfg 所有 xINH=0，在 M/S/U 各模式执行固定循环，读 cycle 差值 | 各模式下 cycle 均正常递增 |
| PMF-CYC-02 | MINH=1 抑制 M-mode cycle 计数 | 设 mcyclecfg.MINH=1，M-mode 执行固定循环，读 cycle 差值 | M-mode 下 cycle 不递增（或递增量远小于未抑制时） |
| PMF-CYC-03 | MINH=1 不影响 S-mode cycle 计数 | 设 mcyclecfg.MINH=1（其余 xINH=0），S-mode 执行固定循环 | S-mode 下 cycle 正常递增 |
| PMF-CYC-04 | SINH=1 抑制 S-mode cycle 计数 | 设 mcyclecfg.SINH=1，S-mode 执行固定循环，读 cycle 差值 | S-mode 下 cycle 不递增 |
| PMF-CYC-05 | SINH=1 不影响 M-mode cycle 计数 | 设 mcyclecfg.SINH=1（其余 xINH=0），M-mode 执行固定循环 | M-mode 下 cycle 正常递增 |
| PMF-CYC-06 | UINH=1 抑制 U-mode cycle 计数 | 设 mcyclecfg.UINH=1，U-mode 执行固定循环，读 cycle 差值 | U-mode 下 cycle 不递增 |
| PMF-CYC-07 | UINH=1 不影响 M-mode cycle 计数 | 设 mcyclecfg.UINH=1（其余 xINH=0），M-mode 执行固定循环 | M-mode 下 cycle 正常递增 |
| PMF-CYC-10 | 多模式同时抑制 | 设 mcyclecfg.MINH=1, SINH=1，M-mode 和 S-mode 分别执行循环 | 两个模式下 cycle 均不递增；U-mode 正常递增 |
| PMF-CYC-11 | 模式切换期间 cycle 计数不确定但有限 | MINH=1，从 U-mode（非抑制）ecall 到 M-mode（抑制）再返回 | U-mode 期间 cycle 递增；M-mode 期间不递增；切换开销导致的周期数为实现定义 |

> [!NOTE]
> VS/VU-mode cycle 过滤测试（原 PMF-CYC-08：VSINH=1 抑制 VS-mode；PMF-CYC-09：VUINH=1 抑制 VU-mode）已移至 `Hypervisor_cross_test_plan.md` Group 16（Hypervisor × Smcntrpmf），因其依赖 H 扩展。

---

## Group 3. instret 计数器特权模式过滤

**规范依据**：
- `norm:counter_inhibited_behavior`：在被抑制的特权模式下不发生计数
- `norm:all_xinh_zero`：所有 xINH 为零时所有模式计数使能
- `norm:instret_non_inhibited`：非抑制模式下退休的指令递增 instret，抑制模式下不递增
- `norm:instret_exception`：引发同步异常的指令不退休，不递增 instret
- `norm:instret_xret`：xRET 指令退休，仅当源特权模式未抑制时递增 instret

**测试职责**：验证 minstretcfg 各 xINH 位对 instret 计数器在不同特权模式下的抑制效果，以及异常/xRET 指令的计数行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PMF-INS-01 | 所有 xINH=0 时全模式 instret 计数 | minstretcfg 所有 xINH=0，M/S/U 各模式执行 N 条指令，读 instret 差值 | 各模式下 instret 递增约 N |
| PMF-INS-02 | MINH=1 抑制 M-mode instret 计数 | 设 minstretcfg.MINH=1，M-mode 执行 N 条指令 | M-mode 下 instret 不递增 |
| PMF-INS-03 | MINH=1 不影响 S-mode instret 计数 | 设 minstretcfg.MINH=1（其余 xINH=0），S-mode 执行 N 条指令 | S-mode 下 instret 正常递增 |
| PMF-INS-04 | SINH=1 抑制 S-mode instret 计数 | 设 minstretcfg.SINH=1，S-mode 执行 N 条指令 | S-mode 下 instret 不递增 |
| PMF-INS-05 | UINH=1 抑制 U-mode instret 计数 | 设 minstretcfg.UINH=1，U-mode 执行 N 条指令 | U-mode 下 instret 不递增 |
| PMF-INS-08 | 异常指令不递增 instret（非抑制模式） | 所有 xINH=0，U-mode 执行一条触发 ecall 的指令 | 该 ecall 指令不递增 instret（不退休） |
| PMF-INS-09 | 异常指令不递增 instret（抑制模式） | 设 minstretcfg.MINH=1，M-mode 执行一条触发异常的指令 | instret 不递增（双重原因：异常不退休 + 模式被抑制） |
| PMF-INS-10 | xRET 从非抑制模式退休递增 instret | 所有 xINH=0，M-mode 执行 MRET | MRET 递增 instret（源模式 M 未被抑制） |
| PMF-INS-11 | xRET 从抑制模式退休不递增 instret | 设 minstretcfg.MINH=1，M-mode 执行 MRET | MRET 不递增 instret（源模式 M 被抑制） |
| PMF-INS-12 | xRET 从抑制 S-mode 退休不递增 instret | 设 minstretcfg.SINH=1，S-mode 执行 SRET | SRET 不递增 instret（源模式 S 被抑制） |
| PMF-INS-13 | xRET 从非抑制 S-mode 退休递增 instret | 设 minstretcfg.SINH=0，S-mode 执行 SRET | SRET 递增 instret |
| PMF-INS-14 | 仅 U-mode 非抑制时 page-fault 场景 | 设 minstretcfg 仅 UINH=0（其余全 1），U-mode load 触发 page-fault 后在 handler 中处理并返回重新执行 | 故障 load 不递增（不退休）；handler 指令不递增（抑制模式）；xRET 不递增（源模式抑制）；重新执行的 load 递增 1 次 |

> [!NOTE]
> VS/VU-mode instret 过滤测试（原 PMF-INS-06：VSINH=1 抑制 VS-mode；PMF-INS-07：VUINH=1 抑制 VU-mode）已移至 `Hypervisor_cross_test_plan.md` Group 16（Hypervisor × Smcntrpmf），因其依赖 H 扩展。

---

## Group 4. 模式切换与计数边界行为

**规范依据**：
- `norm:transition_counting_defined`：模式切换的计数定义
- `norm:cycle_counting`：模式切换操作可能花费多个时钟周期，特权模式变化可报告为发生在其中任一周期
- `norm:instret_xret`：xRET 指令的计数取决于源特权模式

**测试职责**：验证 trap 和 trap return 过程中计数器的边界行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PMF-TR-01 | trap 进入抑制模式后 cycle 停止 | mcyclecfg.MINH=1，U-mode（非抑制）触发 ecall 到 M-mode（抑制），M-mode 执行若干指令后 MRET | M-mode 期间 cycle 不递增；U-mode 恢复后 cycle 继续递增 |
| PMF-TR-02 | trap 进入非抑制模式后 cycle 恢复 | mcyclecfg.SINH=0（S 非抑制），U-mode 触发委托到 S-mode 的 ecall | S-mode handler 期间 cycle 正常递增 |
| PMF-TR-03 | trap 进入抑制模式后 instret 停止 | minstretcfg.MINH=1，U-mode 触发 ecall 到 M-mode，M-mode 执行 N 条指令后 MRET | M-mode handler 中 N 条指令不递增 instret |
| PMF-TR-04 | trap 进入非抑制模式后 instret 恢复 | minstretcfg.SINH=0，U-mode 触发委托到 S-mode 的 ecall，S-mode 执行 N 条指令 | S-mode handler 中 instret 正常递增 |
| PMF-TR-05 | MRET 从抑制模式返回的 instret 行为 | minstretcfg.MINH=1，M-mode 执行 MRET 返回 U-mode | MRET 本身不递增 instret（源模式 M 被抑制） |
| PMF-TR-06 | MRET 从非抑制模式返回的 instret 行为 | minstretcfg.MINH=0，M-mode 执行 MRET 返回 U-mode | MRET 递增 instret（源模式 M 未被抑制） |
| PMF-TR-07 | 中断进入抑制模式 | mcyclecfg.MINH=1，U-mode 触发中断到 M-mode | 中断 handler 期间 cycle 不递增 |
| PMF-TR-08 | 中断进入非抑制模式 | mcyclecfg.SINH=0，U-mode 触发委托到 S-mode 的中断 | 中断 handler 期间 cycle 正常递增 |
| PMF-TR-09 | 连续模式切换的 cycle 计数一致性 | mcyclecfg.MINH=1，UINH=0，反复 U→M→U 切换多次 | 总 cycle 增量仅反映 U-mode 执行时间（M-mode 期间不计数） |
| PMF-TR-10 | 连续模式切换的 instret 计数一致性 | minstretcfg.MINH=1，UINH=0，U-mode 执行 K 条指令后 ecall 到 M-mode，M-mode 执行 L 条后 MRET | instret 增量 = K（U-mode 指令）；M-mode 的 L 条 + MRET 不计数；ecall 本身不计数（异常不退休） |

---

## Group 5. Smcdeleg/Ssccfg 交互（条件测试）

**规范依据**：
- `norm:csr_supervisor_access`：若实现 Smcdeleg/Ssccfg 扩展，mcyclecfg/minstretcfg 内容可从 Supervisor 级访问

**测试职责**：验证 Smcdeleg/Ssccfg 扩展实现时，Supervisor 级对 mcyclecfg/minstretcfg 的访问行为。

**前置条件**：仅在 Smcdeleg 和 Ssccfg 扩展均已实现时执行本组测试。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PMF-DELEG-01 | Smcdeleg 委托后 S-mode 访问 cyclecfg | 通过 Smcdeleg 委托 cycle 计数器配置，S-mode 通过 scyclecfg 访问 | 正常读写，与 M-mode mcyclecfg 内容一致 |
| PMF-DELEG-02 | Smcdeleg 委托后 S-mode 访问 instretcfg | 通过 Smcdeleg 委托 instret 计数器配置，S-mode 通过 sinstretcfg 访问 | 正常读写，与 M-mode minstretcfg 内容一致 |
| PMF-DELEG-03 | 未委托时 S-mode 访问触发异常 | 未通过 Smcdeleg 委托，S-mode 尝试访问 mcyclecfg | illegal-instruction exception (cause=2) |
| PMF-DELEG-04 | S-mode 配置过滤后功能生效 | S-mode 通过 scyclecfg 设 SINH=1，S-mode 执行循环 | S-mode cycle 不递增（配置生效） |

---

## Group 6. 与 mcountinhibit 的交互

**规范依据**：
- `norm:counter_inhibited_behavior`：Smcntrpmf 修改 cycle/instret 的基本计数行为
- SPEC NOTE：mcyclecfg 的自然 CSR 编号为 0x320，但该编号已分配给 mcountinhibit

**测试职责**：验证 Smcntrpmf 与 mcountinhibit 同时作用时的行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PMF-INH-01 | mcountinhibit 全局禁止优先 | mcountinhibit.CY=1，mcyclecfg 所有 xINH=0，各模式执行循环 | cycle 在所有模式下均不递增（mcountinhibit 全局禁止） |
| PMF-INH-02 | mcountinhibit 与 mcyclecfg 独立作用 | mcountinhibit.CY=0，mcyclecfg.MINH=1，M-mode 执行循环 | M-mode cycle 不递增（mcyclecfg 抑制） |
| PMF-INH-03 | mcountinhibit.IR 全局禁止 instret | mcountinhibit.IR=1，minstretcfg 所有 xINH=0，各模式执行指令 | instret 在所有模式下均不递增 |
| PMF-INH-04 | mcountinhibit.IR=0 且 minstretcfg 抑制 | mcountinhibit.IR=0，minstretcfg.SINH=1，S-mode 执行指令 | S-mode instret 不递增 |

---

## Group 7. 与 mcounteren/scounteren/hcounteren 的交互

**规范依据**：
- `norm:mcyclecfg_op`：mcyclecfg/minstretcfg 配置特权模式过滤
- 计数器可见性由 mcounteren/scounteren/hcounteren 控制（基础特权架构）

**测试职责**：验证计数器访问权限控制与特权模式过滤的正交性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PMF-CTR-01 | mcounteren.CY=0 时 S-mode 不可读 cycle | mcounteren.CY=0，S-mode 读 cycle | illegal-instruction exception |
| PMF-CTR-02 | mcounteren.CY=1 时 S-mode 可读 cycle（被抑制） | mcounteren.CY=1，mcyclecfg.SINH=1，S-mode 读 cycle | 可读但值不递增（过滤生效） |
| PMF-CTR-03 | scounteren.CY=0 时 U-mode 不可读 cycle | mcounteren.CY=1，scounteren.CY=0，U-mode 读 cycle | illegal-instruction exception |
| PMF-CTR-05 | 过滤与访问控制正交 | mcyclecfg.UINH=1，mcounteren.CY=1，scounteren.CY=1，U-mode 读 cycle | 可读但值不递增 |

> [!NOTE]
> hcounteren 与 VS-mode 计数器访问的测试（原 PMF-CTR-04：hcounteren.CY=0 时 VS-mode 不可读 cycle）已移至 `Hypervisor_cross_test_plan.md` Group 16（Hypervisor × Smcntrpmf），因其依赖 H 扩展。

---
