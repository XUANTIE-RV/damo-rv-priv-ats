**中文 | [English](../testplan_en/Svade_test_plan_en.md)**

# Svade 扩展测试计划

本文档描述 Svade（Page-Fault Exceptions on A/D Bit Updates）扩展的测试计划。Svade 扩展规定：当虚拟页被访问且 PTE.A=0，或被写入且 PTE.D=0 时，硬件**不再原子地更新** A/D bit，而是抛出 page-fault 异常，由软件负责设置 A/D bit 后重试。

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — Svade 扩展定义、PTE A/D 位语义、虚拟地址翻译算法步骤 9（A/D 触发 page-fault 的位置）

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内上述路径文件）

---

## 概述

RISC-V 特权级规范中定义了两种 PTE A/D 位管理方案：

1. **非 Svade 方案**（默认硬件更新）：当 A=0 被访问或 D=0 被写入时，硬件原子地更新 PTE 的 A/D 位。
2. **Svade 方案**：当 A=0 被访问或 D=0 被写入时，硬件抛出 page-fault 异常，PTE 的 A/D 位保持不变，由软件 trap handler 显式置位后重试访问。

实现 Svade 扩展的处理器，在虚拟地址翻译过程的步骤 9（参见 `supervisor.adoc` 翻译算法）会检查 `pte.a` 与 `pte.d`：

- 若 `pte.a=0`，或原始访问为 store 且 `pte.d=0`，则**停止翻译并抛出对应原始访问类型的 page-fault 异常**。

本测试计划聚焦该规范要求在不同访问类型（load / store / instruction fetch / AMO）、不同页面粒度（4 KiB / 2 MiB / 1 GiB）下的覆盖。

---

## 覆盖的规范点

下表列出本方案覆盖的规范点。带 `norm:` 前缀的为 SPEC 官方 normative rule 标签；不带前缀的为本方案依据 SPEC 原文归纳的规范点。

| Norm ID | 原文（要点） | 中文说明 |
|---------|------|----------|
| `norm:svade_access_ad_bit_clear` | The Svade extension: when a virtual page is accessed and the A bit is clear, or is written and the D bit is clear, a page-fault exception is raised. | Svade 扩展：当虚拟页面被访问且 A 位为 0，或被写入且 D 位为 0 时，触发页错误异常。 |
| `Svade_store_d_bit_clear_pagefault` | — | 当虚拟页被写入且 PTE.D=0 时，抛出 store/AMO page-fault |
| `Svade_no_hw_update_ad` | — | Svade 实现下硬件不得自动设置 A/D bit，PTE 内容保持不变 |
| `Svade_pagefault_cause_match_access_type` | — | page-fault 的 scause 必须与原始访问类型一致（load=13、store=15、fetch=12） |
| `Svade_applies_all_levels` | — | A/D 检查在叶 PTE 上执行，4 KiB / 2 MiB megapage / 1 GiB gigapage 三种粒度均适用 |
| `Svade_software_set_ad_then_access` | — | 软件在 PTE 中预置 A=1（store 场景下还需 D=1）后访问应成功 |
| `Svade_non_leaf_ad_reserved` | — | 非叶 PTE 的 A/D 位保留，不参与 Svade 检查（叶 PTE A/D 决定行为） |

---

## 不在测试范围内

- **Hypervisor 两级翻译场景**：VS-stage 与 G-stage 下的 Svade 行为由 `Hypervisor_Sv_test_plan.md` 覆盖。
- **Svadu 交互**：`menvcfg.ADUE` / `henvcfg.ADUE` 控制硬件 A/D 更新与 Svade 切换的行为，由 `Svadu_test_plan.md` 覆盖。
- **多 hart 一致性**：规范要求所有 hart 必须采用相同的 PTE 更新方案，本方案聚焦单 hart 行为。
- **Sv32 / Sv48 / Sv57 模式**：本计划以 Sv39 为主覆盖对象，其他 Sv 模式下 A/D 语义一致。

---

## 测试分组

### Group 1：A bit 清零触发 load / fetch page-fault（4 KiB）

**规范依据**：
- `norm:svade_access_ad_bit_clear`：A=0 时被访问触发 page-fault
- `Svade_software_set_ad_then_access`：A=1 时访问应成功
- `Svade_pagefault_cause_match_access_type`：load 触发 scause=13，fetch 触发 scause=12

**测试职责**：验证在 4 KiB 叶 PTE 上，A=0 时各类读取访问（load、instruction fetch）均会触发对应类型的 page-fault；A=1 时访问应成功。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADE-A-01 | A=0 R-only 页 load | 4 KiB PTE：V=1, R=1, A=0, D=0，S-mode load | load page-fault（scause=13） |
| SVADE-A-02 | A=0 但 D=1 load | 4 KiB PTE：V=1, R=1, A=0, D=1，S-mode load | load page-fault（D=1 不影响 A 检查） |
| SVADE-A-03 | A=1 load 成功 | 4 KiB PTE：V=1, R=1, A=1, D=0，S-mode load | 读取成功，无异常 |
| SVADE-A-04 | A=0 X-only 页 fetch | 4 KiB PTE：V=1, R=0, X=1, A=0，S-mode 跳转执行 | instruction page-fault（scause=12） |
| SVADE-A-05 | A=1 X 页 fetch 成功 | 4 KiB PTE：V=1, R=0, X=1, A=1，S-mode 跳转执行 | 取指执行成功 |
| SVADE-A-06 | A=0 RW 页 load | 4 KiB PTE：V=1, R=1, W=1, A=0, D=0，S-mode load | load page-fault（scause=13） |

---

### Group 2：D bit 清零触发 store page-fault（4 KiB）

**规范依据**：
- `Svade_store_d_bit_clear_pagefault`：D=0 时 store 触发 page-fault
- `Svade_pagefault_cause_match_access_type`：store 触发 scause=15
- `Svade_software_set_ad_then_access`：D=1 时 store 应成功

**测试职责**：验证在 4 KiB 叶 PTE 上，D=0 时 store / AMO 访问会触发 store/AMO page-fault；load 不受 D 位影响；D=1 时 store 成功。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADE-D-01 | D=0 RW 页 store | 4 KiB PTE：V=1, R=1, W=1, A=1, D=0，S-mode store | store page-fault（scause=15） |
| SVADE-D-02 | D=0 RW 页 load | 4 KiB PTE：V=1, R=1, W=1, A=1, D=0，S-mode load | 读取成功（D 不影响 load） |
| SVADE-D-03 | D=1 RW 页 store | 4 KiB PTE：V=1, R=1, W=1, A=1, D=1，S-mode store | 写入成功 |
| SVADE-D-04 | A=0 D=0 RW 页 store | 4 KiB PTE：V=1, R=1, W=1, A=0, D=0，S-mode store | store page-fault（scause=15，A 同时缺失也触发） |
| SVADE-D-05 | D=0 AMO 操作 | 4 KiB PTE：V=1, R=1, W=1, A=1, D=0，S-mode `amoadd.w` | store/AMO page-fault（scause=15） |

---

### Group 3：硬件不更新 A/D bit

**规范依据**：
- `Svade_no_hw_update_ad`：Svade 实现下，触发 page-fault 后硬件不得自动设置 A/D 位
- `Svade_software_set_ad_then_access`：软件设置 A/D 位 + SFENCE.VMA 后访问应成功

**测试职责**：验证 Svade 与"非 Svade（硬件更新）"方案的根本区别——发生 A/D 触发的 page-fault 后，PTE 内容保持原状；软件设置后重试应成功。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADE-NOUPD-01 | A=0 fault 后 PTE 不变 | 触发 SVADE-A-01 后回到 M-mode 读取 PTE 检查 A bit | PTE.A 仍为 0 |
| SVADE-NOUPD-02 | D=0 fault 后 PTE 不变 | 触发 SVADE-D-01 后回到 M-mode 读取 PTE 检查 D bit | PTE.D 仍为 0 |
| SVADE-NOUPD-03 | 软件置 A=1 重试 load | A=0 load fault → trap handler 设置 A=1 + SFENCE.VMA → 重试 | 读取成功 |
| SVADE-NOUPD-04 | 软件置 D=1 重试 store | D=0 store fault → trap handler 设置 D=1 + SFENCE.VMA → 重试 | 写入成功 |
| SVADE-NOUPD-05 | 多次 load 同一 A=0 页 | 连续 N 次 load A=0 页面 | 每次均触发 page-fault（PTE 永远不变） |

---

### Group 4：2 MiB megapage 上的 A/D 行为

**规范依据**：
- `Svade_applies_all_levels`：A/D 检查在叶 PTE 上执行，2 MiB megapage 同样适用

**测试职责**：验证 Svade 行为在 2 MiB megapage（Sv39 中 level 1 叶 PTE）上同样生效。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADE-2M-01 | 2M A=0 load fault | 2 MiB leaf PTE：V=1, R=1, A=0，S-mode load | load page-fault（scause=13） |
| SVADE-2M-02 | 2M D=0 store fault | 2 MiB leaf PTE：V=1, R=1, W=1, A=1, D=0，S-mode store | store page-fault（scause=15） |
| SVADE-2M-03 | 2M A=1 D=1 访问成功 | 2 MiB leaf PTE：V=1, R=1, W=1, A=1, D=1，S-mode load + store | 读写均成功 |
| SVADE-2M-04 | 2M A=0 fetch fault | 2 MiB leaf PTE：V=1, X=1, A=0，S-mode 跳转执行 | instruction page-fault（scause=12） |
| SVADE-2M-05 | 2M 区域内多偏移访问 | A=0 megapage，访问区域内偏移 0、0x1000、0x100000、0x1FF000 | 每次访问均触发 page-fault |

---

### Group 5：1 GiB gigapage 上的 A/D 行为

**规范依据**：
- `Svade_applies_all_levels`：A/D 检查在叶 PTE 上执行，1 GiB gigapage 同样适用

**测试职责**：验证 Svade 行为在 1 GiB gigapage（Sv39 中 level 2 叶 PTE）上同样生效。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADE-1G-01 | 1G A=0 load fault | 1 GiB leaf PTE：V=1, R=1, A=0，S-mode load | load page-fault（scause=13） |
| SVADE-1G-02 | 1G D=0 store fault | 1 GiB leaf PTE：V=1, R=1, W=1, A=1, D=0，S-mode store | store page-fault（scause=15） |
| SVADE-1G-03 | 1G A=1 D=1 访问成功 | 1 GiB leaf PTE：V=1, R=1, W=1, A=1, D=1，S-mode load + store | 读写均成功 |
| SVADE-1G-04 | 1G 区域内多偏移访问 | A=0 gigapage，访问区域内多个偏移 | 每次访问均触发 page-fault |

> [!NOTE]
> 1 GiB gigapage 测试需要确保测试 VA 选择在远离 M-mode 代码段与栈的恒等映射区域，避免与代码段所在的 1 GiB 区域冲突。

---

### Group 6：非叶 PTE 的 A/D bit 不参与 Svade 检查

**规范依据**：
- `Svade_non_leaf_ad_reserved`：非叶 PTE 的 D、A、U 位保留为未来标准使用，软件应清零；Svade 检查仅在叶 PTE 上进行

**测试职责**：验证 Svade 的 A/D 检查仅作用于叶 PTE，与中间层非叶 PTE 的 A/D 位无关。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADE-NL-01 | 非叶 A=0 叶 A=1 load | 中间层 PTE A=0、D=0（标准用法），4 KiB 叶 PTE A=1，S-mode load | 访问成功（叶 PTE A=1 决定） |
| SVADE-NL-02 | 非叶 A=0 叶 A=0 load | 中间层 PTE A=0，叶 PTE A=0，S-mode load | load page-fault（叶 PTE A=0 触发） |
| SVADE-NL-03 | 非叶 D=0 叶 D=1 store | 中间层 PTE D=0，叶 PTE A=1, D=1, W=1，S-mode store | 写入成功（叶 PTE D=1 决定） |

---

### Group 7：scause 与访问类型的对应关系

**规范依据**：
- `Svade_pagefault_cause_match_access_type`：page-fault 的 scause 必须与原始访问类型对应

**测试职责**：集中验证 Svade 触发的 page-fault 的 scause 在 load / store / fetch / AMO 四种访问类型下分别为 13 / 15 / 12 / 15。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADE-CAUSE-01 | load → scause=13 | A=0 R 页执行 `lw` | scause = 13（load page-fault） |
| SVADE-CAUSE-02 | store → scause=15 | A=1 D=0 RW 页执行 `sw` | scause = 15（store page-fault） |
| SVADE-CAUSE-03 | fetch → scause=12 | A=0 X 页跳转执行 | scause = 12（instruction page-fault） |
| SVADE-CAUSE-04 | AMO → scause=15 | A=1 D=0 RW 页执行 `amoadd.w` | scause = 15（store/AMO page-fault） |
| SVADE-CAUSE-05 | A=0 store → scause=15 | A=0 D=0 RW 页执行 `sw` | scause = 15（store 类型优先） |

> [!IMPORTANT]
> SVADE-CAUSE-05 验证当一个 store 操作同时触发 A=0 与 D=0 时，由于步骤 9 的判定条件是「A=0 或（store 且 D=0）」，整个判定属于 store 访问，因此 scause 应为 store page-fault（15），而非 load page-fault（13）。

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1（A 位）、Group 2（D 位）、Group 7（scause 对应） | SVADE-A-01~06、SVADE-D-01~05、SVADE-CAUSE-01~05 | Svade 核心语义：A/D 清零触发对应类型 page-fault |
| P1（重要） | Group 3（硬件不更新） | SVADE-NOUPD-01~05 | Svade 与硬件更新方案的根本区别 |
| P2（建议） | Group 4（2 MiB）、Group 5（1 GiB）、Group 6（非叶 PTE） | SVADE-2M-01~05、SVADE-1G-01~04、SVADE-NL-01~03 | 大页粒度覆盖与非叶 PTE 边界 |

---

## 结果判定原则

- 平台实现 Svade 但行为偏离 SPEC（如 A=0/D=0 访问不触发 page-fault、硬件擅自更新 A/D 位、scause 与访问类型不符等）：保持用例失败，与 SPEC 比对后将问题记录至 `bugs/` 目录，禁止修改用例或加 workaround 适配错误实现。

---

## 参考

- `supervisor.adoc` — Svade 扩展定义、PTE A/D 位语义、虚拟地址翻译算法
- `Svadu_test_plan.md` — 硬件 A/D 更新扩展测试计划
- `Hypervisor_Sv_test_plan.md` — Hypervisor × Sv* 交叉测试计划

---

## 附录 A：规范点覆盖矩阵

下表标明"覆盖的规范点"章节中每条规范点被哪些测试用例覆盖。

| Norm ID | 覆盖的测试 ID |
|---------|---------------|
| `norm:svade_access_ad_bit_clear` | SVADE-A-01~06、SVADE-D-01~05、SVADE-2M-01~05、SVADE-1G-01~04 |
| `Svade_store_d_bit_clear_pagefault` | SVADE-D-01、SVADE-D-04、SVADE-D-05、SVADE-2M-02、SVADE-1G-02、SVADE-CAUSE-02、SVADE-CAUSE-04、SVADE-CAUSE-05 |
| `Svade_no_hw_update_ad` | SVADE-NOUPD-01~05 |
| `Svade_pagefault_cause_match_access_type` | SVADE-CAUSE-01~05、SVADE-A-01、SVADE-A-04、SVADE-D-01 |
| `Svade_applies_all_levels` | SVADE-2M-01~05、SVADE-1G-01~04 |
| `Svade_software_set_ad_then_access` | SVADE-A-03、SVADE-A-05、SVADE-D-03、SVADE-NOUPD-03、SVADE-NOUPD-04 |
| `Svade_non_leaf_ad_reserved` | SVADE-NL-01~03 |
