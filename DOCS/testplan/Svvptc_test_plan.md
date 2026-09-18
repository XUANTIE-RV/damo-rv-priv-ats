**中文 | [English](../testplan_en/Svvptc_test_plan_en.md)**

# Svvptc 扩展测试计划

本文档描述 Svvptc（Obviating Memory-Management Instructions after Marking PTEs Valid）扩展的测试计划。Svvptc 扩展规定：当 hart 通过显式 store 把叶/非叶 PTE 的 Valid 位**从 0 置为 1**后，操作系统**不再需要**执行 `SFENCE.VMA` / `SINVAL.VMA` 等地址翻译缓存同步指令；该 PTE 更新会在**有界时间**内对该 hart 后续的隐式访问（地址翻译）变得可见。

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/svvptc.adoc` — Svvptc 扩展定义：叶/非叶 PTE 的 Valid 位 0→1 的显式 store 在有界时间内对后续隐式访问可见；NOTE 说明 OS 可省略 sfence、以及"偶发额外虚假 page-fault"的容忍
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — Sv39 虚拟地址翻译算法（任何中间 PTE 或叶 PTE V=0 时停止并抛出对应类型 page-fault）
- `SPEC/riscv-isa-manual/src/priv/svinval.adoc` — SFENCE.VMA / SINVAL.VMA 语义（用于"反向场景必须 sfence"的边界对照，不在本计划测试范围）

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内上述路径文件）

---

## 概述

RISC-V 特权级规范要求：当软件修改了内存中的 PTE 后，必须执行 `SFENCE.VMA` 或 `SINVAL.VMA` 来同步 hart 的地址翻译缓存（TLB / Page Walk Cache），否则后续访问可能仍然使用过时的翻译。

Svvptc 扩展放宽了**一种特定方向**的同步要求：

- **覆盖场景（Svvptc 保证）**：PTE 的 Valid 位 `V: 0 → 1`，包括叶 PTE 和非叶 PTE。`SFENCE.VMA` 等指令变为**冗余**；硬件保证更新在有界时间内对后续隐式访问可见，代价是**可能偶尔出现一次额外的虚假 page-fault**（spec NOTE: "occasional gratuitous additional page fault"）。
- **不覆盖场景（仍需 sfence）**：PTE 的任何**其他**形式的更新，包括 `V: 1 → 0`、权限降级（如 RW → R）、PA 重映射（PPN 变更）、A/D 位的软件清除等。这些场景仍需调用 `SFENCE.VMA` / `SINVAL.VMA`，由 `Svinval_test_plan.md` 独立覆盖。

本测试计划聚焦该规范在**单 hart 单线程**场景下的行为：在不调用 sfence.vma 的前提下，验证修改 V=0→1 后**最终**能命中（在有界重试上限内），并以"加 sfence 立即命中"为基线对照。

---

## 覆盖的规范点

下表列出本方案覆盖的规范点。带 `norm:` 前缀的为 SPEC 官方 normative rule 标签；不带前缀的为本方案依据 SPEC 原文归纳的规范点。

| Norm ID | 原文（要点） | 中文说明 |
|---------|------|----------|
| `norm:Svvptc_explicit_stores_update_valid_bit` | When the Svvptc extension is implemented, explicit stores by a hart that update the Valid bit of leaf and/or non-leaf PTEs from 0 to 1 and are visible to a hart will eventually become visible within a bounded timeframe to subsequent implicit accesses by that hart to such PTEs. | 当实现 Svvptc 扩展时，hart 将叶和/或非叶 PTE 的 Valid 位从 0 更新到 1 的显式存储，最终将在有界时间范围内对该 hart 后续对此类 PTE 的隐式访问可见。 |
| `Svvptc_bounded_time_eventual_visibility` | "bounded timeframe" 的可观测量化：允许偶发虚假 page-fault，但必须在有界次重试内命中。 | 通过"page-fault 后原地重试 + 最大重试上限"量化验证有界时间：超过上限仍失败则判定不符合 Svvptc。 |
| `Svvptc_reverse_boundary_baseline` | V:0→1 后附加显式 sfence.vma 的传统路径必然立即命中。 | 以"加 sfence 立即命中"作为基线对照，排除用例自身 PTE 设置错误导致的假阳性。 |

---

## 不在测试范围内

- **反向场景**（V: 1→0、权限降级、PPN 重映射、A/D 软件清除）：Svvptc **不**保证此类更新无需 sfence；这些场景由 `Svinval_test_plan.md` 覆盖。
- **Hypervisor 两级翻译场景**：VS-stage / G-stage 下的 Svvptc 行为由 `Hypervisor_Sv_test_plan.md` 及相关 Hypervisor 方案覆盖。
- **Sv32 / Sv48 / Sv57 模式**：本计划以 Sv39 为主覆盖对象，其他 Sv 模式下 V:0→1 语义一致。
- **多 hart 一致性**：Svvptc 规范明确仅约束**同一 hart**的显式 store 与隐式访问可见性；跨 hart 可见性需通过 IPI + sfence 等机制保障，超出 Svvptc 范畴。
- **与 Svinval / Svadu 的复合交互**：由各扩展独立计划覆盖。

---

## 设计要点

### 1. 有界时间验证策略（重试 + 上限）

Svvptc 允许"偶尔的虚假 page-fault"，因此修改 V=0→1 后的**首次**隐式访问可能仍触发 page-fault。测试通过以下方式验证"有界时间内最终可见"：

1. 测试代码在 S-mode 下访问目标 va，预期"最终"成功。
2. S-mode 本地 trap handler 捕获 page-fault 后**不递增 sepc**，而是原地重试同一指令，同时累加重试计数。
3. 当重试计数超过设定的**最大重试上限**时，trap handler 将控制流导向一个逃逸出口并回写失败标记。
4. 用例最终断言：`访问成功` 且 `重试次数 ≤ 最大重试上限`。

> [!NOTE]
> 最大重试上限应设为一个足够大的保守值（可在执行阶段按平台实测分布调整），以容纳真实实现可能出现的多次虚假 page-fault；但**不可下调到 0**——下调到 0 等价于强制要求"首次必命中"，违反 spec NOTE 关于"occasional gratuitous additional page fault"的容忍。

### 2. 不主动调用 sfence.vma

所有正向用例（Group 1~4）**故意省略** `sfence.vma`，依赖 Svvptc 的"有界时间最终可见"保证。这是测试的核心立意——若实现错误地缓存 Invalid PTE 且无淘汰机制，该用例将无限重试直至超出上限并失败。

### 3. 直接 store 修改 PTE

测试通过取得 PTE 槽位指针后用普通 store 修改 V 位，对应规范中"explicit stores by a hart"。同一 hart 内对同一地址的 store 与后续指令读，由 RVWMO 的 program order + 同地址依赖自动保证可见性；PTE store 后**不**额外附加 `fence rw, rw`（它既不替代 `sfence.vma`，也不为 Svvptc 路径提供额外语义）。

### 4. fetch 用例的指令预置流程

X 权限叶页在初始 V=0 时**无法**被任何 store 写入指令字节（V=0 既不可读也不可写）。因此所有 fetch 类用例（SVVPTC-4K-03 / SVVPTC-2M-03 / SVVPTC-NL-03）必须按以下三步流程预置：

1. **预置阶段**：先把目标页临时映射为 RW 权限且 V=1，写入一段以 `ret` 结尾的可执行指令序列，随后执行 `sfence.vma` 同步。
2. **改装阶段**：把权限改为 V=0、X=1、A=1（去掉 R/W、清 V），并执行 `sfence.vma` 同步。**这一步必须 sfence**——V:1→0 与权限降级**不在** Svvptc 保证范围内。
3. **测试阶段**：再次把 V 位改为 1（**不** sfence），进入 S-mode 通过函数指针调用，用重试机制吸收虚假 instruction page-fault；最终断言访问在有界重试内成功。

### 5. 基线对照（Sanity check）

每组核心用例在 Group 5 中都有一条对照：相同的 PTE 修改流程**加上**显式 `sfence.vma`，验证"不依赖 Svvptc 的传统路径"必然首次命中（重试次数 = 0）。这能排除"用例本身 PTE 设置错误"等假阳性，提高测试可信度。

### 6. 多轮切换中 sfence 的必要性

SVVPTC-4K-04 在每轮**结尾**用 `sfence.vma` 把 V 位**清回 0**。这里 sfence 是**必须的**：V:1→0 属于 Svvptc 保证范围**之外**的 PTE 更新，若不 sfence，TLB 仍可能命中旧 V=1 翻译，导致下一轮的"V=0 起点"实际未生效。仅在每轮**起点**的 V=0→1 切换故意省略 sfence，对应 Svvptc 测试目标。

---

## 测试分组

### Group 1：4 KiB 叶 PTE V: 0→1 后无 sfence 的最终可见性

**规范依据**：
- `norm:Svvptc_explicit_stores_update_valid_bit`：叶 PTE V: 0→1 在有界时间内对后续隐式访问可见。

**测试职责**：在 4 KiB 叶 PTE 上验证三种隐式访问类型（load / store / instruction fetch）在 V=0→1 后均能在最大重试上限内命中；并通过"多轮 V 位切换 + sfence 清回 0"用例验证可重复性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVVPTC-4K-01 | 4K V=0→1 load 最终可见 | 4 KiB PTE 初始 V=0, R=1, A=1, D=1；store 改 PTE.V=1（无 sfence）；S-mode 反复 load 直至成功 | load 成功；重试次数 ≤ 上限 |
| SVVPTC-4K-02 | 4K V=0→1 store 最终可见 | 4 KiB PTE 初始 V=0, R=1, W=1, A=1, D=1；store 改 PTE.V=1（无 sfence）；S-mode 反复 store 直至成功 | store 成功；重试次数 ≤ 上限 |
| SVVPTC-4K-03 | 4K V=0→1 fetch 最终可见 | 4 KiB PTE 初始 V=0, X=1, A=1；store 改 PTE.V=1（无 sfence）；S-mode 跳转执行 | fetch 成功；重试次数 ≤ 上限 |
| SVVPTC-4K-04 | 4K V 位多轮切换可见性 | 重复 8 轮：`sfence.vma` 把 V 清 0（V:1→0 不在 Svvptc 范围，**必须 sfence**）→ 不带 sfence 把 V 置 1（V:0→1 受 Svvptc 保护）→ load 直至成功；每轮重试计数独立累加 | 每轮均成功；每轮重试次数 ≤ 上限 |

---

### Group 2：2 MiB megapage 叶 PTE V: 0→1 最终可见性

**规范依据**：
- `norm:Svvptc_explicit_stores_update_valid_bit`：规范不区分 PTE 层级，2 MiB megapage 叶 PTE 同样适用。

**测试职责**：在 2 MiB megapage 叶 PTE 上验证 load / store / fetch 三种访问类型在 V=0→1 后均能在最大重试上限内命中。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVVPTC-2M-01 | 2M V=0→1 load 最终可见 | 2 MiB megapage 初始 V=0, R=1, A=1, D=1；store V=1（无 sfence）；S-mode load | load 成功；重试 ≤ 上限 |
| SVVPTC-2M-02 | 2M V=0→1 store 最终可见 | 2 MiB megapage 初始 V=0, R=1, W=1, A=1, D=1；store V=1（无 sfence）；S-mode store | store 成功；重试 ≤ 上限 |
| SVVPTC-2M-03 | 2M V=0→1 fetch 最终可见 | 2 MiB megapage 初始 V=0, X=1, A=1；store V=1（无 sfence）；S-mode 跳转执行 | fetch 成功；重试 ≤ 上限 |

---

### Group 3：1 GiB gigapage 叶 PTE V: 0→1 最终可见性（占位跳过）

**规范依据**：
- `norm:Svvptc_explicit_stores_update_valid_bit`：1 GiB gigapage 叶 PTE 同样适用。

**测试职责**：在 1 GiB gigapage 叶 PTE 上验证 load / store 在 V=0→1 后能否在有界重试内命中。

> [!IMPORTANT]
> **可测性说明**：要在 1 GiB gigapage 叶 PTE 上验证 Svvptc 的 V=0→1 语义，必须把测试运行所需的代码段、栈、页表池与 UART MMIO 全部安置在被测 1 GiB 子空间**之外**——否则一旦该 1 GiB 叶 PTE 处于 V=0，取指 / 访问栈 / 访问页表都会失败，无法完成测试。当前测试环境可用的连续物理内存不足以同时提供"独立的 1 GiB 承载区"与"被测 1 GiB 子空间"，因此该维度以 `TEST_SKIP` 显式记录跳过原因。规范覆盖维度由 Group 1（4K）+ Group 2（2M）+ Group 4（非叶）共同完成；1 GiB gigapage 与 2 MiB megapage 在 Svvptc 语义上等价（都是 leaf-PTE V:0→1，规范文本 `leaf and/or non-leaf PTEs from 0 to 1` 不区分 leaf 的具体层级），不构成规范覆盖空缺。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVVPTC-1G-01 | 1G V=0→1 load 占位（跳过） | 记录跳过原因（需独立于被测 1 GiB 子空间承载代码/栈） | `[SKIP]` 输出，不计入失败 |
| SVVPTC-1G-02 | 1G V=0→1 store 占位（跳过） | 同 SVVPTC-1G-01 | `[SKIP]` 输出，不计入失败 |

> [!NOTE]
> 保留 SVVPTC-1G-01 / -02 两个 `TEST_SKIP` 用例的目的：在测试报告中显式记录"1G 维度被有意识地跳过及其原因"，避免后续维护者误以为是遗漏——这是确定的、有依据的工程决策，而非未实现的占位。

---

### Group 4：非叶 PTE V: 0→1 最终可见性

**规范依据**：
- `norm:Svvptc_explicit_stores_update_valid_bit`：明确包含 **non-leaf PTEs**。即使叶 PTE 已完全有效，只要路径上任意一个非叶 PTE 的 V=0，翻译就会停在该层并触发 page-fault；对该非叶 PTE 做 V=0→1 显式 store 同样受 Svvptc 保护。

**测试职责**：构造"非叶 PTE V=0、叶 PTE V=1 且权限齐全"的页表布局，再 store 把非叶 PTE V 位置 1，验证最终访问能命中。覆盖中间层（L1）与次顶层（L2）两个非叶层级。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVVPTC-NL-01 | L1 非叶 V=0→1 后访问叶页 | 4K 叶 PTE 完整有效（V=1, R=1, A=1, D=1），但其上层 L1 非叶 PTE V=0 → store L1 PTE V=1（无 sfence）→ S-mode load 4K 叶页 | load 成功；重试 ≤ 上限 |
| SVVPTC-NL-02 | L2 非叶 V=0→1 后访问叶页 | 4K 叶 PTE 完整有效，L1 非叶 V=1，但 L2 非叶 PTE V=0 → store L2 PTE V=1（无 sfence）→ S-mode load | load 成功；重试 ≤ 上限 |
| SVVPTC-NL-03 | L1 非叶 V=0→1 后 fetch | 4K 叶 PTE V=1, X=1, A=1；其上层 L1 非叶 V=0 → store L1 V=1（无 sfence）→ S-mode 跳转执行 | fetch 成功；重试 ≤ 上限 |

> [!NOTE]
> 构造方法：先建立叶页与全部上层非叶 PTE，然后取得目标非叶层级（L1 / L2）的 PTE 指针，清 V 并 sfence，再置 V=1（不 sfence），进入 S-mode 验证。

---

### Group 5：基线对照 / Sanity check

**规范依据**：
- `Svvptc_reverse_boundary_baseline`：Svvptc 之外的传统路径（V: 0→1 + 显式 sfence）必然首次命中，用于排除"用例本身 PTE 设置错误"等假阳性。

**测试职责**：与 Group 1 / 2 / 4 形成 1:1 对照——相同的 PTE 修改流程**加上** `sfence.vma`，验证不依赖 Svvptc 时第 1 次访问就能成功（重试次数 = 0）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVVPTC-BASE-01 | 4K V=0→1 + sfence 立即可见 | 与 SVVPTC-4K-01 相同设置，但在 store V=1 后立即 `sfence.vma zero, zero`；S-mode load | 第 1 次 load 成功；重试次数 == 0 |
| SVVPTC-BASE-02 | 2M V=0→1 + sfence 立即可见 | 与 SVVPTC-2M-01 相同设置，附 sfence；S-mode load | 第 1 次 load 成功；重试次数 == 0 |
| SVVPTC-BASE-03 | 非叶 V=0→1 + sfence 立即可见 | 与 SVVPTC-NL-01 相同设置，附 `sfence.vma zero, zero`（rs1=x0 全局刷新覆盖非叶失效）；S-mode load | 第 1 次 load 成功；重试次数 == 0 |

> [!NOTE]
> SVVPTC-BASE-* 的"重试次数 == 0"是规范无关的工程基线，用于检测"用例本身 PTE 设置错误"或"重试计数统计错误"。Svvptc 路径用例（Group 1~4）的"重试次数 ≤ 上限"则是 Svvptc 的实际合规验证。

---

## 测试用例汇总

| Group | 用例总数 | 实测用例 | TEST_SKIP 用例 | ID 前缀 | 关注点 |
|-------|---------|---------|---------------|---------|--------|
| Group 1 | 4 | 4 | 0 | `SVVPTC-4K-*` | 4 KiB 叶 PTE，三种访问类型 + 多轮切换 |
| Group 2 | 3 | 3 | 0 | `SVVPTC-2M-*` | 2 MiB megapage 叶 PTE |
| Group 3 | 2 | 0 | 2 | `SVVPTC-1G-*` | 1 GiB gigapage 叶 PTE（受可用物理内存约束统一跳过） |
| Group 4 | 3 | 3 | 0 | `SVVPTC-NL-*` | 非叶 PTE（L1 / L2） |
| Group 5 | 3 | 3 | 0 | `SVVPTC-BASE-*` | 基线对照（加 sfence） |
| **合计** | **15** | **13** | **2** | — | — |

---

## 结果判定原则

- 平台实现 Svvptc 但行为偏离 SPEC（如 V=0→1 后在有界重试上限内仍无法命中，说明实现缓存 Invalid PTE 且无淘汰机制）：保持用例失败，与 SPEC 比对后将问题记录至 `bugs/` 目录，禁止修改用例或加 workaround 适配错误实现。
- 基线用例（Group 5）"重试次数 == 0"失败通常指向用例自身 PTE 设置或重试统计缺陷，应先行排除，再判定 Svvptc 路径。

---

## 参考

- `svvptc.adoc` — Svvptc 扩展定义
- `supervisor.adoc` — 虚拟地址翻译算法
- `svinval.adoc` — SFENCE.VMA / SINVAL.VMA 语义（反向边界对照）
- `Svinval_test_plan.md` — Svinval 测试计划（反向场景覆盖）

---

## 附录 A：规范点覆盖矩阵

下表标明"覆盖的规范点"章节中每条规范点被哪些测试用例覆盖。

| Norm ID | 覆盖的测试 ID |
|---------|---------------|
| `norm:Svvptc_explicit_stores_update_valid_bit` | SVVPTC-4K-01~04、SVVPTC-2M-01~03、SVVPTC-NL-01~03、SVVPTC-1G-01~02（占位跳过） |
| `Svvptc_bounded_time_eventual_visibility` | SVVPTC-4K-01~04、SVVPTC-2M-01~03、SVVPTC-NL-01~03 |
| `Svvptc_reverse_boundary_baseline` | SVVPTC-BASE-01~03 |
