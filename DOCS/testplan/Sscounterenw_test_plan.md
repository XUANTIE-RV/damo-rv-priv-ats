**中文 | [English](../testplan_en/Sscounterenw_test_plan_en.md)**

# Sscounterenw 扩展测试计划

本文档描述 Sscounterenw（Counter-Enable Writability, Version 1.0）扩展的测试计划。Sscounterenw 扩展规定：如果实现了该扩展，则对于任何不是 read-only zero 的 `hpmcounter`，`scounteren` 中对应的 bit 必须是可写的。

---

## 概述

RISC-V 特权级规范中，`scounteren` 寄存器控制 S-mode 对硬件性能计数器（`hpmcounter3`–`hpmcounter31`）以及 `cycle`、`time`、`instret` 的访问权限。当 `scounteren` 中某个 bit 为 0 时，U-mode 访问对应的计数器将触发 illegal-instruction 异常。

然而，基础规范并未强制要求 `scounteren` 的所有 bit 都必须可写——实现可以将某些 bit 硬连线为 0 或 1。这导致软件无法可靠地控制 U-mode 对计数器的访问权限。

**Sscounterenw 扩展的核心约束**：

> 如果某个 `hpmcounter` 不是 read-only zero（即该计数器被实现且有意义），则 `scounteren` 中对应的 bit **必须**是可写的（即软件可以将其设置为 0 或 1）。

这一约束确保了：
1. 软件可以精确控制 U-mode 对已实现计数器的访问权限
2. OS 可以按需授予或撤销 U-mode 对性能计数器的读取能力

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/sscounterenw.adoc` — Sscounterenw Extension for Counter-Enable Writability, Version 1.0
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — scounteren 寄存器定义与访问控制行为
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — mcounteren 对 S-mode 计数器访问的门控

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内 src/priv/sscounterenw.adoc、src/priv/supervisor.adoc、src/priv/machine.adoc）

---

## 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sscounterenw_hpmcounter_scounteren` | If the Sscounterenw extension is implemented, then for any `hpmcounter` that is not read-only zero, the corresponding bit in `scounteren` must be writable. | 如果实现了 Sscounterenw 扩展，则对于任何非只读零的 `hpmcounter`，`scounteren` 中对应的位必须是可写的。 |

**自行拆解的规范点**（来自 `supervisor.adoc`、`machine.adoc` 中相关文本的拆解，SPEC 无独立 norm 标签）：

| Norm ID | 中文说明 |
|---------|----------|
| `scounteren_bit_set_to_1` | scounteren bit 可被设为 1，随后 U-mode 可无异常访问对应计数器 |
| `scounteren_bit_set_to_0` | scounteren bit 可被设为 0，随后 U-mode 访问对应计数器触发 illegal-instruction |
| `scounteren_bit_toggle` | scounteren bit 可在 0 和 1 之间反复切换，行为一致 |
| `scounteren_readonly_zero_counter` | 对于 read-only zero 的 hpmcounter，scounteren 对应 bit 行为不受 Sscounterenw 约束（可以是 read-only） |
| `mcounteren_gate_scounteren` | mcounteren 仍然 gate S-mode 对计数器的访问，Sscounterenw 不改变这一层级关系 |

---

## 不在测试范围内

- **计数器事件计数的正确性**：Sscounterenw 仅约束 scounteren 的可写性，不涉及计数器是否正确计数
- **Sscofpmf（Count Overflow and Mode-Based Filtering）**：由独立的 `Sscofpmf_test_plan.md` 覆盖
- **hcounteren（Hypervisor Counter-Enable）**：VS/VU-mode 的计数器访问控制由 hypervisor 测试覆盖
- **Sv32 模式**：本计划仅覆盖 RV64
- **多 hart 场景**：项目为单核测试环境
- **mcounteren 可写性**：mcounteren 的可写性由基础特权级规范定义，非 Sscounterenw 特有

---

## 前提与约束

> [!IMPORTANT]
> Sscounterenw 的验证需要先确定哪些 `hpmcounter` 是"已实现的"（不是 read-only zero）。验证策略为：在 M-mode 下设置 `mcounteren` 对应 bit 为 1，然后在 S-mode 尝试读取 `hpmcounter`，如果读到非零值或不触发异常则认为该计数器已实现。对于已实现的计数器，验证 `scounteren` 对应 bit 的可写性。

### 关键 CSR

| CSR | 地址 | 说明 |
|-----|------|------|
| `mcounteren` | 0x306 | M-mode 控制 S-mode 对计数器的访问 |
| `scounteren` | 0x106 | S-mode 控制 U-mode 对计数器的访问 |
| `cycle` | 0xC00 | 周期计数器（只读，bit 0） |
| `time` | 0xC01 | 时钟计数器（只读，bit 1） |
| `instret` | 0xC02 | 已退休指令计数器（只读，bit 2） |
| `hpmcounter3`–`hpmcounter31` | 0xC03–0xC1F | 硬件性能计数器（只读，bit 3–31） |

### 设计原则

1. **动态发现法**：运行时先探测哪些 hpmcounter 是已实现的（非 read-only zero），再对这些计数器验证 scounteren 可写性
2. **端到端验证**：不仅验证 scounteren bit 的读写回环，还验证设置后的实际权限控制效果（U-mode 访问成功/失败）
3. **边界覆盖**：覆盖 cycle（bit 0）、time（bit 1）、instret（bit 2）和 hpmcounter3–31（bit 3–31）

---

## 测试分组

### Group 1：scounteren 可写性验证（M-mode 读写回环）

**规范依据**：
- `norm:sscounterenw_hpmcounter_scounteren`：已实现计数器对应的 scounteren bit 必须可写

**测试职责**：在 M-mode 下，对每个已实现的 hpmcounter 对应的 scounteren bit 进行写 1/读回、写 0/读回验证。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCNTW-WR-01 | cycle 对应 scounteren[0] 可写 | 探测 cycle 是否已实现，若是则验证 scounteren[0] 可写 1 和写 0 | 写入值回读一致 |
| SSCNTW-WR-02 | time 对应 scounteren[1] 可写 | 探测 time 是否已实现，若是则验证 scounteren[1] 可写 1 和写 0 | 写入值回读一致 |
| SSCNTW-WR-03 | instret 对应 scounteren[2] 可写 | 探测 instret 是否已实现，若是则验证 scounteren[2] 可写 1 和写 0 | 写入值回读一致 |
| SSCNTW-WR-04 | hpmcounter3–31 对应 scounteren[3:31] 可写 | 逐个探测 hpmcounter3–31，对已实现的验证 scounteren 对应 bit 可写 | 已实现计数器的对应 bit 写入值回读一致 |

---

### Group 2：scounteren 控制 U-mode 访问（端到端验证）

**规范依据**：
- `scounteren_bit_set_to_1`：bit 为 1 时 U-mode 可访问
- `scounteren_bit_set_to_0`：bit 为 0 时 U-mode 访问触发 illegal-instruction

**测试职责**：对已实现的 hpmcounter，验证 scounteren bit 设为 1 时 U-mode 读取成功，设为 0 时 U-mode 读取触发 illegal-instruction（cause=2）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCNTW-ACCESS-01 | scounteren[0]=1 时 U-mode 读 cycle 成功 | 设置 mcounteren[0]=1、scounteren[0]=1，U-mode 读 cycle | 无异常 |
| SSCNTW-ACCESS-02 | scounteren[0]=0 时 U-mode 读 cycle 触发异常 | 设置 mcounteren[0]=1、scounteren[0]=0，U-mode 读 cycle | 触发 illegal-instruction（cause=2） |
| SSCNTW-ACCESS-03 | scounteren[1]=1 时 U-mode 读 time 成功 | 设置 mcounteren[1]=1、scounteren[1]=1，U-mode 读 time | 无异常 |
| SSCNTW-ACCESS-04 | scounteren[1]=0 时 U-mode 读 time 触发异常 | 设置 mcounteren[1]=1、scounteren[1]=0，U-mode 读 time | 触发 illegal-instruction（cause=2） |
| SSCNTW-ACCESS-05 | scounteren[2]=1 时 U-mode 读 instret 成功 | 设置 mcounteren[2]=1、scounteren[2]=1，U-mode 读 instret | 无异常 |
| SSCNTW-ACCESS-06 | scounteren[2]=0 时 U-mode 读 instret 触发异常 | 设置 mcounteren[2]=1、scounteren[2]=0，U-mode 读 instret | 触发 illegal-instruction（cause=2） |
| SSCNTW-ACCESS-07 | scounteren[N]=1 时 U-mode 读 hpmcounterN 成功 | 对已实现的 hpmcounterN，设置对应 bit=1，U-mode 读取 | 无异常 |
| SSCNTW-ACCESS-08 | scounteren[N]=0 时 U-mode 读 hpmcounterN 触发异常 | 对已实现的 hpmcounterN，设置对应 bit=0，U-mode 读取 | 触发 illegal-instruction（cause=2） |

---

### Group 3：scounteren bit 反复切换一致性

**规范依据**：
- `scounteren_bit_toggle`：scounteren bit 可反复切换，行为一致

**测试职责**：对已实现的计数器，反复设置和清除 scounteren bit，验证每次切换后 U-mode 访问行为一致。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCNTW-TOGGLE-01 | cycle 对应 bit 反复切换 | 对 scounteren[0] 做 1→0→1→0 切换，每次验证 U-mode 行为 | 每次切换后行为符合当前 bit 值 |
| SSCNTW-TOGGLE-02 | instret 对应 bit 反复切换 | 对 scounteren[2] 做 1→0→1→0 切换，每次验证 U-mode 行为 | 每次切换后行为符合当前 bit 值 |
| SSCNTW-TOGGLE-03 | hpmcounterN 对应 bit 反复切换 | 对已实现的 hpmcounterN 做切换验证 | 每次切换后行为符合当前 bit 值 |

---

### Group 4：mcounteren 与 scounteren 层级交互

**规范依据**：
- `mcounteren_gate_scounteren`：mcounteren 仍然 gate S-mode 对计数器的访问

**测试职责**：验证即使 scounteren bit 为 1，如果 mcounteren 对应 bit 为 0，S-mode 访问计数器仍触发异常（Sscounterenw 不改变层级关系）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCNTW-HIER-01 | mcounteren[0]=0 时 S-mode 读 cycle 异常 | 设置 mcounteren[0]=0、scounteren[0]=1，S-mode 读 cycle | 触发 illegal-instruction（cause=2） |
| SSCNTW-HIER-02 | mcounteren[0]=1 时 S-mode 读 cycle 成功 | 设置 mcounteren[0]=1、scounteren[0]=1，S-mode 读 cycle | 无异常 |
| SSCNTW-HIER-03 | mcounteren[N]=0 时 S-mode 读 hpmcounterN 异常 | 设置 mcounteren[N]=0、scounteren[N]=1，S-mode 读取 | 触发 illegal-instruction（cause=2） |
| SSCNTW-HIER-04 | mcounteren=0 scounteren=1 时 U-mode 读 cycle 异常 | mcounteren 阻断后，即使 scounteren=1，U-mode 也无法访问 | 触发 illegal-instruction（cause=2） |

---

### Group 5：read-only zero 计数器的 scounteren bit 行为

**规范依据**：
- `scounteren_readonly_zero_counter`：对于 read-only zero 的 hpmcounter，Sscounterenw 不强制其 scounteren bit 可写

**测试职责**：对于探测为 read-only zero 的 hpmcounter，记录其 scounteren bit 行为（可能是 read-only 0 或 read-only 1），不作 pass/fail 判断，仅作信息收集。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCNTW-RO-01 | read-only zero 计数器 scounteren bit 探测 | 对所有 read-only zero 的 hpmcounter，尝试写 scounteren bit 并报告结果 | 信息性输出，不做 pass/fail |

---

## 计数器实现探测策略

由于不同实现可能支持不同的 hpmcounter 子集，测试需在运行时动态探测。探测流程如下：

1. 对每个计数器索引 i (0..31)：
   - M-mode：设置 `mcounteren[i] = 1`
   - 对于 i=0 (cycle)、i=1 (time)、i=2 (instret)：直接在 M-mode 读取对应 CSR，若返回值非零或读取不产生异常则认为已实现
   - 对于 i=3..31 (hpmcounter3-31)：在 M-mode 写 `mhpmcounter[i]` 为非零值，回读 `mhpmcounter[i]`，若回读值非零则认为已实现，随后恢复原值
2. 记录已实现的计数器位图，供后续测试使用

任一平台违反 SPEC 时用例保持 FAIL，实现缺陷记录至 `bugs/` 目录。

---

## 测试统计

| 分组 | 测试数量 | 说明 |
|------|----------|------|
| Group 1：可写性验证 | 4 | 基础读写回环 |
| Group 2：U-mode 访问控制 | 8 | 端到端权限验证 |
| Group 3：切换一致性 | 3 | 反复切换验证 |
| Group 4：层级交互 | 4 | mcounteren 门控 |
| Group 5：read-only zero 报告 | 1 | 信息收集 |
| **总计** | **20** | |

---

## 附录 A：规范点覆盖矩阵

| Norm ID | 覆盖的测试 ID | 覆盖状态 | 备注 |
|---------|--------------|----------|------|
| `norm:sscounterenw_hpmcounter_scounteren` | SSCNTW-WR-01、SSCNTW-WR-02、SSCNTW-WR-03、SSCNTW-WR-04、SSCNTW-ACCESS-01 ~ SSCNTW-ACCESS-08、SSCNTW-TOGGLE-01 ~ SSCNTW-TOGGLE-03 | 已覆盖 | 核心规范：已实现计数器对应 scounteren bit 可写 |
| `scounteren_bit_set_to_1` | SSCNTW-ACCESS-01、SSCNTW-ACCESS-03、SSCNTW-ACCESS-05、SSCNTW-ACCESS-07、SSCNTW-HIER-02 | 已覆盖 | bit=1 时 U-mode 可访问 |
| `scounteren_bit_set_to_0` | SSCNTW-ACCESS-02、SSCNTW-ACCESS-04、SSCNTW-ACCESS-06、SSCNTW-ACCESS-08 | 已覆盖 | bit=0 时 U-mode 触发 illegal-instruction |
| `scounteren_bit_toggle` | SSCNTW-TOGGLE-01、SSCNTW-TOGGLE-02、SSCNTW-TOGGLE-03 | 已覆盖 | bit 反复切换一致性 |
| `scounteren_readonly_zero_counter` | SSCNTW-RO-01 | 已覆盖 | read-only zero 计数器信息性报告 |
| `mcounteren_gate_scounteren` | SSCNTW-HIER-01、SSCNTW-HIER-02、SSCNTW-HIER-03、SSCNTW-HIER-04 | 已覆盖 | mcounteren 门控层级 |
