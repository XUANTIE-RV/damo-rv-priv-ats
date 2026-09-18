**中文 | [English](../testplan_en/Ssccptr_test_plan_en.md)**

# Ssccptr 扩展测试计划

本文档描述 Ssccptr（Main Memory Page-Table Reads, Version 1.0）扩展的测试计划。Ssccptr 扩展规定：如果实现了该扩展，则具有 cacheability（可缓存性）和 coherence（一致性）两个 PMA 属性的主存区域，必须支持硬件页表读取（hardware page-table reads）。

---

## 概述

RISC-V 特权级规范中，MMU 在执行虚拟地址翻译时需要从内存中读取页表条目（PTE），即 hardware page-table walk。页表数据所在的物理内存区域是否支持此类隐式读操作，取决于该区域的 Physical Memory Attributes（PMA）。

Ssccptr 扩展对此做出了明确约束：

- **核心要求**：同时具有 cacheability 和 coherence 两个 PMA 的主存区域，**必须**能够被 MMU 的 page-table walker 正确读取。
- **隐含语义**：在符合 Ssccptr 的实现上，软件只要将页表放置在满足上述 PMA 条件的常规主存中，即可保证硬件 page walk 正常工作，无需额外的 PMA 配置或特殊处理。

这一约束看似简单，但其验证涉及多个维度：不同页表级别、不同虚拟内存模式、不同访问类型、多级 page walk 的每一级读取、以及与 PMP 的交互。

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/ssccptr.adoc` — Ssccptr Extension for Main Memory Page-Table Reads, Version 1.0
- `SPEC/riscv-isa-manual/src/priv/sv.adoc` — Sv39/Sv48/Sv57 分页方案、多级页表结构、SFENCE.VMA 与 TLB 行为
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — satp、sstatus.SUM/MXR 等 S-mode 地址翻译控制

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内 src/priv/ssccptr.adoc、src/priv/sv.adoc、src/priv/supervisor.adoc）

---

## 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:ssccptr_memory_pte_reads` | If the Ssccptr extension is implemented, then main memory regions with both the cacheability and coherence PMAs must support hardware page-table reads. | 如果实现了 Ssccptr 扩展，则同时具有可缓存性和一致性 PMA 的主存区域必须支持硬件页表读取。 |

**自行拆解的规范点**（从 `norm:ssccptr_memory_pte_reads` 派生的可测断言，SPEC 无独立 norm 标签）：

| Norm ID | 中文说明 |
|---------|----------|
| `Ssccptr_all_pt_levels` | 硬件页表读取在所有页表级别（Sv39 的 L0/L1/L2、Sv48 增加的 L3、Sv57 增加的 L4）均须正常工作 |
| `Ssccptr_all_access_types` | 硬件页表读取对 load、store、instruction fetch 三种触发 page walk 的访问类型均须正常工作 |
| `Ssccptr_all_priv_modes` | 硬件页表读取在 S-mode 和 U-mode 触发的 page walk 中均须正常工作 |
| `Ssccptr_multiLevel_walk` | 多级页表遍历中，每一级非叶 PTE 的读取和叶 PTE 的读取均须成功 |
| `Ssccptr_superpage` | 超级页（megapage / gigapage / terapage / petapage）的叶 PTE 读取须正常工作 |

---

## 不在测试范围内

- **非 cacheable / 非 coherent 区域的行为**：Ssccptr 仅约束同时满足 cacheability + coherence 的区域，对其他 PMA 组合不做要求
- **I/O 区域的页表放置**：将页表放在 I/O 空间（不满足 PMA 条件）的行为不在 Ssccptr 约束范围内
- **Sv32 模式**：本计划仅覆盖 RV64（Sv39 / Sv48 / Sv57），与项目其它扩展测试计划保持一致
- **多 hart 场景**：项目为单核测试环境
- **PMA 寄存器的配置与发现**：PMA 通常是平台级别的硬连线属性，非软件可编程，测试计划依赖平台主存默认满足 PMA 条件这一前提
- **Hypervisor 两级翻译（VS-stage + G-stage）**：由独立的 hypervisor 测试计划覆盖

---

## 前提与约束

> [!IMPORTANT]
> Ssccptr 的核心约束是关于 PMA（Physical Memory Attributes）的，而 PMA 通常是平台硬连线属性，不是软件可动态配置的。本测试计划的验证策略是：**在默认主存（满足 PMA 条件）中建立页表，通过端到端的 page walk 成功（或失败符合预期）来间接验证硬件页表读取能力**。

### 设计原则

1. **间接验证法**：由于无法直接观测 MMU 的 page-table read 操作，通过设置页表 → 开启虚拟内存 → 执行不同类型的访问 → 验证结果正确（无 access fault）来间接证明 page walk 成功
2. **对照组设计**：在验证 page walk 成功的同时，通过 PMP 阻断页表读取的对照组，证明测试有能力检测到 page walk 失败（排除假阳性）
3. **逐级覆盖**：针对每一级页表分别验证，确保不是仅仅因为 TLB 命中而绕过了 page walk
4. **平台可行性检查**：Sv48 512 GiB terapage、Sv57 512 GiB / 256 TiB 页对平台内存基址的对齐要求可能无法满足，测试需在运行时检查可行性并对不可行的用例 TEST_SKIP

任一平台违反 SPEC 时用例保持 FAIL，实现缺陷记录至 `bugs/` 目录。

---

## 测试分组

### Group 1：基本页表遍历验证（Sv39, 4 KiB 页）

**规范依据**：
- `norm:ssccptr_memory_pte_reads`：主存区域必须支持硬件页表读取
- `Ssccptr_all_access_types`：load / store / fetch 均须正常
- `Ssccptr_all_priv_modes`：S-mode 和 U-mode 均须正常

**测试职责**：在默认主存上建立 Sv39 三级页表（identity mapping，L0/L1/L2 三级），验证 4 KiB 页面的 page walk 在不同访问类型和特权模式下均能成功完成。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-BASIC-01 | Sv39 4K 页 S-mode load | 建立 Sv39 identity mapping（4 KiB 页，PTE 带 RWXAD），S-mode load | 读取成功，无异常 |
| SSCCPTR-BASIC-02 | Sv39 4K 页 S-mode store | 同上配置，S-mode store | 写入成功，无异常 |
| SSCCPTR-BASIC-03 | Sv39 4K 页 S-mode fetch | 同上配置（X=1），S-mode 跳转执行 | 取指执行成功 |
| SSCCPTR-BASIC-04 | Sv39 4K 页 U-mode load | 建立带 PTE_U 标记的 4 KiB mapping，U-mode load | 读取成功，无异常 |
| SSCCPTR-BASIC-05 | Sv39 4K 页 U-mode store | 同上配置，U-mode store | 写入成功，无异常 |
| SSCCPTR-BASIC-06 | Sv39 4K 页 U-mode fetch | 同上配置（X=1、U=1），U-mode 跳转执行 | 取指执行成功 |

**实现要点**：
- S-mode 访问辅助函数（load/store/exec）内部使用 trap 预期机制捕获异常：无异常返回 0，有异常返回 scause 值
- 测试数据区域通过链接脚本提供，包含读写数据区、异常测试页、可执行代码区三部分，需 2 MiB 对齐以便与代码映射区分

---

### Group 2：超级页 page walk 验证（Sv39, 2 MiB / 1 GiB）

**规范依据**：
- `norm:ssccptr_memory_pte_reads`
- `Ssccptr_superpage`：超级页的叶 PTE 读取须正常工作
- `Ssccptr_multiLevel_walk`：不同深度的 page walk

**测试职责**：验证 2 MiB megapage（L1 叶 PTE）和 1 GiB gigapage（L2 叶 PTE）的 page walk 成功。超级页的 page walk 层级更少，覆盖不同深度的遍历路径。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-SUPER-01 | Sv39 2M megapage S-mode load | L1 叶 PTE identity mapping，S-mode load | 成功 |
| SSCCPTR-SUPER-02 | Sv39 2M megapage S-mode store | 同上，S-mode store | 成功 |
| SSCCPTR-SUPER-03 | Sv39 2M megapage S-mode fetch | 同上（X=1），S-mode fetch | 成功 |
| SSCCPTR-SUPER-04 | Sv39 1G gigapage S-mode load | L2 叶 PTE identity mapping，S-mode load | 成功 |
| SSCCPTR-SUPER-05 | Sv39 1G gigapage S-mode store | 同上，S-mode store | 成功 |
| SSCCPTR-SUPER-06 | Sv39 1G gigapage S-mode fetch | 同上（X=1），S-mode fetch | 成功 |

**实现要点**：使用批量恒等映射覆盖代码和数据区域，测试目标 VA 由链接脚本提供的对齐段决定（2 MiB / 1 GiB 对齐）。

---

### Group 3：多级 page walk 逐级验证（Sv39）

**规范依据**：
- `Ssccptr_multiLevel_walk`：多级页表遍历中，每一级 PTE 的读取均须成功
- `Ssccptr_all_pt_levels`：所有页表级别均须支持

**测试职责**：通过 SFENCE.VMA 清除 TLB 后，执行访问以强制触发完整的 page walk，验证每一级非叶 PTE 和叶 PTE 的读取。通过 PMP 对照组验证测试有效性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-LEVEL-01 | L0 PTE 读取验证 | 4 KiB 页，SFENCE.VMA 清 TLB 后 load，page walk 经过 L2→L1→L0 | 成功 |
| SSCCPTR-LEVEL-02 | L1 PTE 读取验证 | 2 MiB 页，SFENCE.VMA 清 TLB 后 load，page walk 经过 L2→L1 | 成功 |
| SSCCPTR-LEVEL-03 | L2 PTE 读取验证 | 1 GiB 页，SFENCE.VMA 清 TLB 后 load，page walk 经过 L2 | 成功 |
| SSCCPTR-LEVEL-04 | PMP 阻断 L0 PT 对照 | PMP 设置 L0 页表页为不可读，SFENCE.VMA 后 load | load access fault |
| SSCCPTR-LEVEL-05 | PMP 阻断 L1 PT 对照 | PMP 设置 L1 页表页为不可读，SFENCE.VMA 后 load | load access fault |
| SSCCPTR-LEVEL-06 | PMP 阻断 L2 PT（root）对照 | PMP 设置 root 页表页为不可读，SFENCE.VMA 后 load | load access fault |

> [!NOTE]
> SSCCPTR-LEVEL-04/05/06 是**对照组**：通过 PMP 故意阻断某一级页表的读取，验证测试框架能检测到 page walk 失败。如果对照组触发 access fault，而正常组（01/02/03）不触发，则能有效证明正常组的 page walk 确实经过了该级页表。

**实现要点**：
- VM 进入/退出辅助每次都会执行 `vm_enable` + SFENCE.VMA，退出后 `vm_disable`，因此每次进入都触发完整的 page walk
- PMP 对照组：使用 NAPOT 模式将目标页表页标记为 X-only（不可读），阻断 MMU 的 PTE 读取；同时使用一个覆盖全地址空间的 PMP 条目允许其他访问

---

### Group 4：Sv48 页表遍历验证

**规范依据**：
- `norm:ssccptr_memory_pte_reads`
- `Ssccptr_all_pt_levels`：Sv48 增加第 4 级（L3）页表

**测试职责**：验证 Sv48 模式下所有页表级别的 page walk 成功，覆盖 L3 根页表的读取。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-SV48-01 | Sv48 4K 页 S-mode load | Sv48 identity mapping（4 KiB），S-mode load | 成功 |
| SSCCPTR-SV48-02 | Sv48 4K 页 S-mode store | 同上，S-mode store | 成功 |
| SSCCPTR-SV48-03 | Sv48 4K 页 S-mode fetch | 同上（X=1），S-mode fetch | 成功 |
| SSCCPTR-SV48-04 | Sv48 2M megapage load | Sv48 2M 页 mapping，S-mode load | 成功 |
| SSCCPTR-SV48-05 | Sv48 1G gigapage load | Sv48 1G 页 mapping，S-mode load | 成功 |
| SSCCPTR-SV48-06 | Sv48 512G terapage load | Sv48 L3 叶 PTE mapping，S-mode load | 成功；平台内存基址未按 512 GiB 对齐时 SKIP |

> [!NOTE]
> Sv48 的 512 GiB terapage 要求平台内存基址按 512 GiB 对齐。当平台内存基址不满足该对齐条件时（例如常见仿真平台的 MEM_BASE 为 2 GiB），该用例应检测到并 TEST_SKIP。

---

### Group 5：Sv57 页表遍历验证

**规范依据**：
- `norm:ssccptr_memory_pte_reads`
- `Ssccptr_all_pt_levels`：Sv57 增加第 5 级（L4）页表

**测试职责**：验证 Sv57 模式下所有页表级别的 page walk 成功，覆盖 L4 根页表的读取。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-SV57-01 | Sv57 4K 页 S-mode load | Sv57 identity mapping（4 KiB），S-mode load | 成功 |
| SSCCPTR-SV57-02 | Sv57 4K 页 S-mode store | 同上，S-mode store | 成功 |
| SSCCPTR-SV57-03 | Sv57 4K 页 S-mode fetch | 同上（X=1），S-mode fetch | 成功 |
| SSCCPTR-SV57-04 | Sv57 2M megapage load | Sv57 2M 页 mapping，S-mode load | 成功 |
| SSCCPTR-SV57-05 | Sv57 1G gigapage load | Sv57 1G 页 mapping，S-mode load | 成功 |
| SSCCPTR-SV57-06 | Sv57 512G terapage load | Sv57 L3 叶 PTE mapping，S-mode load | 成功；平台内存基址未按 512 GiB 对齐时 SKIP |
| SSCCPTR-SV57-07 | Sv57 256T petapage load | Sv57 L4 叶 PTE mapping，S-mode load | 成功；平台内存基址未按 256 TiB 对齐时 SKIP |

> [!NOTE]
> Sv57 的 512 GiB terapage 和 256 TiB petapage 分别要求 512 GiB 和 256 TiB 对齐。在常规平台上这些对齐条件通常无法满足，测试应检测到并 TEST_SKIP。Sv57 本身也需要平台支持 `SATP_MODE_SV57`，若不支持则整组 SKIP。

---

### Group 6：TLB 刷新后的重复 page walk

**规范依据**：
- `norm:ssccptr_memory_pte_reads`：每次 page walk 均须成功
- `Ssccptr_multiLevel_walk`

**测试职责**：验证在 SFENCE.VMA 清除 TLB 后，重复触发 page walk 均能成功完成，排除 TLB 缓存掩盖 page walk 失败的可能。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-TLB-01 | 连续 TLB flush + load | 4 KiB 页，循环 N 次：SFENCE.VMA → S-mode load | 每次均成功 |
| SSCCPTR-TLB-02 | 连续 TLB flush + store | 4 KiB 页，循环 N 次：SFENCE.VMA → S-mode store | 每次均成功 |
| SSCCPTR-TLB-03 | 连续 TLB flush + fetch | 4 KiB 页，循环 N 次：SFENCE.VMA → S-mode fetch | 每次均成功 |
| SSCCPTR-TLB-04 | 不同地址的交替 page walk | 两个不同 VA 的 4 KiB 页，交替 SFENCE.VMA + load | 每次均成功 |

**实现要点**：VM 进入/退出辅助内部会在每次退出后禁用 VM，重新进入时启用 VM + SFENCE.VMA 强制全新 page walk。

---

### Group 7：页表动态修改后的 page walk

**规范依据**：
- `norm:ssccptr_memory_pte_reads`：主存必须支持硬件页表读取
- 派生：动态修改页表后 SFENCE.VMA + 再次 page walk 也须成功

**测试职责**：验证在运行时修改 PTE 内容（如更改权限、更改映射目标），执行 SFENCE.VMA 后重新 page walk 能正确读取更新后的 PTE。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-DYN-01 | PTE 权限变更后 walk | R-only → RW，SFENCE.VMA 后 store | 第一次 store fault，修改后 store 成功 |
| SSCCPTR-DYN-02 | PTE 目标 PA 变更后 walk | 重映射 VA 到不同 PA，SFENCE.VMA 后 load | 读到新 PA 的数据 |
| SSCCPTR-DYN-03 | 添加新映射后 walk | 初始无映射 → 添加 PTE → SFENCE.VMA → load | 第一次 page fault，添加后成功 |
| SSCCPTR-DYN-04 | 删除映射后 walk | 有映射 → 清除 PTE.V → SFENCE.VMA → load | 第一次成功，删除后 page fault |

---

### Group 8：PMP 对 page walk 阻断的对照验证

**规范依据**：
- `norm:ssccptr_memory_pte_reads`（反面验证）
- 对照：PMP 可以阻断 page walk 中的 PTE 读取

**测试职责**：通过 PMP 阻断页表所在物理内存的读取权限，验证 page walk 被正确阻断（产生 access fault 而非 page fault）。此组作为 Group 1-3 的对照，证明正常组的成功确实依赖于 page walk 的正确完成。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-PMP-01 | PMP 阻断 L0 PT load | PMP 设 L0 PT 页 X-only，S-mode load | load access fault |
| SSCCPTR-PMP-02 | PMP 阻断 L0 PT store | PMP 设 L0 PT 页 X-only，S-mode store | store access fault |
| SSCCPTR-PMP-03 | PMP 阻断 L0 PT fetch | PMP 设 L0 PT 页 X-only，S-mode fetch | instruction access fault |
| SSCCPTR-PMP-04 | PMP 阻断 L1 PT load | PMP 设 L1 PT 页 X-only，S-mode load | load access fault |
| SSCCPTR-PMP-05 | PMP 阻断 root PT load | PMP 设 root PT 页 X-only，S-mode load | load access fault |
| SSCCPTR-PMP-06 | PMP 允许后 walk 恢复 | 先阻断 → 移除 PMP 限制 → SFENCE.VMA → load | load 成功 |

---

## 测试矩阵总览

| Group | 测试数 | VM 模式 | 页粒度 | 权限模式 | 核心验证点 |
|-------|--------|---------|--------|----------|-----------|
| 1 - 基本 page walk | 6 | Sv39 | 4 KiB | S / U | load / store / fetch 三种访问类型 |
| 2 - 超级页 | 6 | Sv39 | 2M / 1G | S | megapage / gigapage 叶 PTE 读取 |
| 3 - 逐级验证 | 6 | Sv39 | 4K / 2M / 1G | S | 每一级 PTE 读取 + PMP 对照 |
| 4 - Sv48 | 6 | Sv48 | 4K / 2M / 1G / 512G | S | L3 根页表读取 |
| 5 - Sv57 | 7 | Sv57 | 4K / 2M / 1G / 512G / 256T | S | L4 根页表读取 |
| 6 - TLB 刷新 | 4 | Sv39 | 4 KiB | S | 重复 page walk 稳定性 |
| 7 - 动态修改 | 4 | Sv39 | 4 KiB | S | PTE 修改后重新 page walk |
| 8 - PMP 对照 | 6 | Sv39 | 4 KiB | S | PMP 阻断 page walk 反向验证 |

**总计：45 个测试用例**

---

## 实现注意事项

### 1. 构建配置

Ssccptr 是一个 PMA 层面的约束扩展，不引入新指令或 CSR。测试项目需在构建配置中启用虚拟内存支持，并根据平台情况声明 Ssccptr 扩展。若平台不识别 `_ssccptr` 扩展名，可仅启用虚拟内存支持即可运行测试。

### 2. 与已有能力的复用

- **页表管理**：直接使用公共框架的页表初始化、单页映射、批量恒等映射能力
- **VM 进入/退出**：使用公共框架的 S-mode / U-mode 执行辅助，其内部处理 VM 启用/禁用与 SFENCE.VMA
- **S-mode 访问辅助**：复用现有测试项目中 load/store/exec 三类辅助函数的模式（内部使用 trap 预期机制捕获异常）
- **PMP 配置**（Group 3/8）：使用公共框架的 PMP 条目设置能力（NAPOT 模式）
- **页表页地址获取**（Group 3/8）：使用公共框架提供的按 VA 与层级获取页表页物理地址的能力
- **PTE 检查/修改**（Group 7）：使用公共框架提供的按 VA 与层级获取 PTE 内容的能力
- **异常原因常量**：使用公共框架中定义的短别名 `CAUSE_LAF` / `CAUSE_SAF` / `CAUSE_IAF` / `CAUSE_LPF` / `CAUSE_SPF` / `CAUSE_IPF`

### 3. PMA 前提假设

> [!WARNING]
> 本测试计划假设目标平台的主存区域默认满足 cacheability + coherence PMA 条件。如果在特定平台上主存 PMA 不满足此条件，相关测试可能会因 page walk 失败而非 Ssccptr 不合规。在解读测试结果时需关注此前提。

---

## 附录 A：规范点覆盖矩阵

| Norm ID | 覆盖的测试 ID | 覆盖状态 | 备注 |
|---------|--------------|----------|------|
| `norm:ssccptr_memory_pte_reads` | SSCCPTR-BASIC-01 ~ SSCCPTR-BASIC-06、SSCCPTR-SUPER-01 ~ SSCCPTR-SUPER-06、SSCCPTR-LEVEL-01 ~ SSCCPTR-LEVEL-06、SSCCPTR-SV48-01 ~ SSCCPTR-SV48-06、SSCCPTR-SV57-01 ~ SSCCPTR-SV57-07、SSCCPTR-TLB-01 ~ SSCCPTR-TLB-04、SSCCPTR-DYN-01 ~ SSCCPTR-DYN-04、SSCCPTR-PMP-01 ~ SSCCPTR-PMP-06 | 已覆盖 | 主存必须支持硬件页表读取（含正/反向验证） |
| `Ssccptr_all_pt_levels` | SSCCPTR-LEVEL-01 ~ SSCCPTR-LEVEL-06、SSCCPTR-SV48-01 ~ SSCCPTR-SV48-06、SSCCPTR-SV57-01 ~ SSCCPTR-SV57-07 | 已覆盖 | Sv39/Sv48/Sv57 各级页表 |
| `Ssccptr_all_access_types` | SSCCPTR-BASIC-01 ~ SSCCPTR-BASIC-06、SSCCPTR-SUPER-01 ~ SSCCPTR-SUPER-06、SSCCPTR-TLB-01 ~ SSCCPTR-TLB-03 | 已覆盖 | load/store/fetch 三种访问 |
| `Ssccptr_all_priv_modes` | SSCCPTR-BASIC-01 ~ SSCCPTR-BASIC-06 | 已覆盖 | S-mode 与 U-mode |
| `Ssccptr_multiLevel_walk` | SSCCPTR-LEVEL-01 ~ SSCCPTR-LEVEL-03、SSCCPTR-TLB-01 ~ SSCCPTR-TLB-04、SSCCPTR-DYN-01 ~ SSCCPTR-DYN-04 | 已覆盖 | 多级遍历每一级读取 |
| `Ssccptr_superpage` | SSCCPTR-SUPER-01 ~ SSCCPTR-SUPER-06、SSCCPTR-SV48-04 ~ SSCCPTR-SV48-06、SSCCPTR-SV57-04 ~ SSCCPTR-SV57-07 | 已覆盖 | megapage/gigapage/terapage/petapage |
