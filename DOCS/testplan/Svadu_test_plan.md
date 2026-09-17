**中文 | [English](../testplan_en/Svadu_test_plan_en.md)**

# Svadu 扩展测试计划

本文档描述 Svadu（Hardware Updating of A/D Bits）扩展的测试计划。Svadu 扩展为 PTE A/D 位的**硬件自动更新**提供支持，并通过 `menvcfg.ADUE` 字段允许在运行时启用/禁用该行为；当硬件更新被禁用时，处理器回退到 Svade 行为（A/D 触发 page-fault，由软件设置）。

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/svadu.adoc` — Svadu 扩展定义、`menvcfg.ADUE` 可写性、ADUE=0 回退到 Svade 的语义
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — `menvcfg` CSR 字段编码（ADUE=bit 61）及其硬件 A/D 更新控制语义、修改 ADUE 后的 SFENCE.VMA 同步要求
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — Svade 概念与虚拟地址翻译算法步骤 9（A/D 触发 page-fault 的位置）

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内上述路径文件）

---

## 概述

RISC-V 特权级规范中定义了两种 PTE A/D 位管理方案：

1. **硬件更新方案（Svadu 启用）**：当 A=0 被访问或 D=0 被写入时，硬件原子地更新 PTE 的 A/D 位，访问继续进行，不抛出异常。
2. **软件更新方案（Svade 行为）**：当 A=0 被访问或 D=0 被写入时，硬件抛出 page-fault 异常，由软件 trap handler 显式置位后重试。

Svadu 扩展是上述两种方案的**统一控制层**：

- 实现 Svadu 后，`menvcfg.ADUE`（bit 61）字段为可写位（WARL）。
- 当 `menvcfg.ADUE=1` 时，处于"硬件更新"模式：S/U-mode 下访问 A=0 / D=0 的 PTE，硬件原子地更新对应位，访问成功完成。
- 当 `menvcfg.ADUE=0` 时，处于"Svade 模式"：硬件不更新 A/D，访问触发 page-fault，与独立的 Svade 扩展行为完全一致。

实现 Svadu 扩展的处理器，在虚拟地址翻译过程的步骤 9（参见 `supervisor.adoc`）会根据 `menvcfg.ADUE` 选择两条路径（参见 `machine.adoc`）：
- ADUE=0：行为等同于 Svade —— 检测 `pte.a=0` 或（store 且 `pte.d=0`）时停止翻译并抛出对应原始访问类型的 page-fault；
- ADUE=1：硬件原子地将 `pte.a` 置 1，对 store 同时将 `pte.d` 置 1，然后继续翻译，不抛出 page-fault。

本测试计划聚焦该扩展的 CSR 控制行为、ADUE=1 / ADUE=0 两种模式下的访问语义、不同页面粒度（4 KiB / 2 MiB / 1 GiB）下的覆盖，以及运行时动态切换 ADUE 后的行为变化。

---

## 覆盖的规范点

下表列出本方案覆盖的规范点。带 `norm:` 前缀的为 SPEC 官方 normative rule 标签。

| Norm ID | 原文（要点） | 中文说明 |
|---------|------|----------|
| `norm:Svadu_hw_update_a_d_bits` | If the Svadu extension is implemented, the `menvcfg`.ADUE field is writable. | 如果实现了 Svadu 扩展，则 `menvcfg`.ADUE 字段是可写的。 |
| `norm:menvcfg_adue_rdonly0` | If Svadu is not implemented, ADUE is read-only zero. | 如果未实现 Svadu 扩展，ADUE 为只读零。 |
| `norm:menvcfg_adue_op` | If the Svadu extension is implemented, the ADUE bit controls whether hardware updating of PTE A/D bits is enabled for S-mode and G-stage address translations. When ADUE=1, hardware updating is enabled and the implementation behaves as though Svade were not implemented; when ADUE=0, the implementation behaves as though Svade were implemented. | ADUE 位控制 S-mode 翻译是否启用硬件 A/D 位更新：ADUE=1 启用（如同未实现 Svade），ADUE=0 禁用（如同实现 Svade）。 |
| `norm:Svadu_disabled_hw_update_falls_back_to_svade` | When hardware updating of A/D bits is disabled, the Svade extension, which mandates exceptions when A/D bits need be set, instead takes effect. | 当禁用 A/D 位的硬件更新时，Svade 扩展生效，该扩展要求在需要设置 A/D 位时触发异常。 |
| `norm:svade_access_ad_bit_clear` | The Svade extension: when a virtual page is accessed and the A bit is clear, or is written and the D bit is clear, a page-fault exception is raised. | Svade 扩展：当虚拟页面被访问且 A 位为 0，或被写入且 D 位为 0 时，触发页错误异常。 |
| `norm:menvcfg_adue_fence` | After changing `menvcfg`.ADUE, executing an SFENCE.VMA instruction with rs1=`x0` and rs2=`x0` suffices to synchronize address-translation caches with respect to the altered interpretation of page-table entries' A/D bits. | 修改 ADUE 后需要执行 SFENCE.VMA(x0,x0) 以确保地址翻译缓存同步。 |

---

## 不在测试范围内

- **Hypervisor 两级翻译场景**：`henvcfg.ADUE` 的可写性、VS-stage 与 G-stage 下的 Svadu 行为、HLV/HSV 指令交互由 `Hypervisor_Sv_test_plan.md` 覆盖。
- **Sv32 / Sv48 / Sv57 模式**：本计划以 Sv39 为主覆盖对象，其他 Sv 模式下 ADUE 语义一致。
- **多 hart 一致性**：规范要求所有 hart 必须采用相同的 PTE 更新方案，本方案聚焦单 hart 行为。
- **PMP / Smepmp 与 Svadu 的交叉**：相关交互由 PMP 系列测试计划独立覆盖。

---

## 前置条件与检测策略

1. **扩展检测**：Svadu 实现性以 `menvcfg.ADUE` 的可写性判定——M-mode 写 ADUE=1 后回读 bit 61；依据 `norm:menvcfg_adue_rdonly0`，未实现 Svadu 时 ADUE 为只读零，回读 0 即判定未实现。可进一步以"A=0 叶 PTE 在 ADUE=1 下 load 成功且 PTE.A 被硬件置 1"作为功能确认。
2. **跳过策略**：检测失败时，依赖 Svadu 行为的所有测试组（Group 2/3/4/5/7）以及验证回退语义的 Group 6 统一 `TEST_SKIP`；Group 1 的 ADUE 可写性用例自身即为检测手段，其失败本身就是平台无 Svadu 的证据，须显性报告，不静默通过。
3. **ADUE 切换同步**：`menvcfg` 仅 M-mode 可写，所有 ADUE 切换均在 M-mode 完成，切换后依据 `norm:menvcfg_adue_fence` 执行 SFENCE.VMA(x0,x0) 再进入 S-mode 验证。

---

## 测试分组

### Group 1：menvcfg.ADUE 字段控制测试

**规范依据**：
- `norm:Svadu_hw_update_a_d_bits`：实现 Svadu 时 `menvcfg.ADUE` 必须可写
- `norm:menvcfg_adue_rdonly0`：未实现 Svadu 时 ADUE 为只读零

**测试职责**：在 M-mode 下验证 `menvcfg.ADUE`（bit 61）的可写性、读写一致性、对其他字段的不干扰，以及复位后的初值。这是 Svadu 实现性的最基本检查。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADU-CSR-01 | menvcfg.ADUE 可写 0→1 | M-mode 写入 ADUE=1，回读 menvcfg | 回读 bit 61 = 1（未实现 Svadu 时按只读零回读 0） |
| SVADU-CSR-02 | menvcfg.ADUE 可写 1→0 | 先置 ADUE=1，再清 ADUE=0，回读 | 回读 bit 61 = 0 |
| SVADU-CSR-03 | ADUE 不影响其他字段 | 切换 ADUE 时记录 PBMTE/STCE/CBIE/FIOM 等字段 | 其他字段值在 ADUE 切换前后保持不变 |
| SVADU-CSR-04 | ADUE 复位值记录 | 在测试套件入口保存 menvcfg 原始值，读取其 bit 61 并记录（信息性） | 记录复位值（规范未规定复位值，仅观察） |

---

### Group 2：ADUE=1 时 4 KiB 叶 PTE 的硬件 A 位更新

**规范依据**：
- `norm:menvcfg_adue_op`：ADUE=1 时 S-mode 翻译启用硬件 A/D 更新，行为如同未实现 Svade（A=0 访问不再触发 page-fault，硬件原子置 A=1）

**测试职责**：在 4 KiB 叶 PTE 上验证 ADUE=1 时各类访问（load、instruction fetch）对 A=0 页面均不触发 page-fault，且回到 M-mode 后 PTE.A 已被硬件置为 1。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADU-A4K-01 | A=0 R 页 load | ADUE=1，4 KiB PTE：V=1, R=1, A=0, D=0，S-mode load | load 成功（result=0），PTE.A=1 |
| SVADU-A4K-02 | A=0 X 页 fetch | ADUE=1，4 KiB PTE：V=1, X=1, A=0，S-mode 跳转执行 | 取指执行成功，PTE.A=1 |
| SVADU-A4K-03 | A=0 RW 页 load | ADUE=1，4 KiB PTE：V=1, R=1, W=1, A=0, D=1，S-mode load | load 成功，PTE.A=1，PTE.D 维持 1（仅 load 不影响 D） |
| SVADU-A4K-04 | 多次 load 后 A 仅设置一次 | ADUE=1，A=0 页面执行 N 次 load | 每次 load 均成功，最终 PTE.A=1（再次 load 不触发额外副作用） |

---

### Group 3：ADUE=1 时 4 KiB 叶 PTE 的硬件 D 位更新

**规范依据**：
- `norm:menvcfg_adue_op`：ADUE=1 时 store/AMO 后硬件置 PTE.D=1，D=0 store 不再触发 page-fault

**测试职责**：在 4 KiB 叶 PTE 上验证 ADUE=1 时 store 与 AMO 操作对 D=0 页面不触发 page-fault，且回到 M-mode 后 PTE.D 已被硬件置为 1。重点验证 A=0+D=0 的 store 单次访问可同时置位两个标志。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADU-D4K-01 | A=1 D=0 RW 页 store | ADUE=1，PTE：V=1, R=1, W=1, A=1, D=0，S-mode store | store 成功，PTE.D=1（A 维持 1） |
| SVADU-D4K-02 | A=0 D=0 RW 页 store | ADUE=1，PTE：V=1, R=1, W=1, A=0, D=0，S-mode store | store 成功，PTE.A=1 且 PTE.D=1（一次 store 同时置位） |
| SVADU-D4K-03 | A=1 D=0 RW 页 amoadd | ADUE=1，PTE：V=1, R=1, W=1, A=1, D=0，S-mode amoadd.w | AMO 成功，PTE.D=1 |
| SVADU-D4K-04 | A=1 D=1 RW 页 store（无副作用） | ADUE=1，PTE：A=1, D=1，S-mode store | store 成功，PTE.A、PTE.D 维持 1 |

> [!IMPORTANT]
> SVADU-D4K-02 验证一次 store 同时硬件置 A=1 与 D=1，是 Svadu 与"软件先置 A、再触发 D fault"两步法的关键差异。

---

### Group 4：ADUE=1 时 2 MiB megapage 上的硬件 A/D 更新

**规范依据**：
- `norm:menvcfg_adue_op`：硬件 A/D 更新在叶 PTE 上执行，2 MiB megapage 同样适用

**测试职责**：验证 ADUE=1 时硬件 A/D 更新行为在 2 MiB megapage（Sv39 中 level 1 叶 PTE）上同样生效，覆盖 load / store / fetch / AMO 四种访问类型。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADU-2M-01 | 2M A=0 load | 2 MiB 叶 PTE：V=1, R=1, A=0, D=0，S-mode load | load 成功，megapage PTE.A=1 |
| SVADU-2M-02 | 2M A=1 D=0 store | 2 MiB 叶 PTE：V=1, R=1, W=1, A=1, D=0，S-mode store | store 成功，megapage PTE.D=1 |
| SVADU-2M-03 | 2M A=0 X 页 fetch | 2 MiB 叶 PTE：V=1, X=1, A=0，S-mode 跳转执行 | 取指成功，megapage PTE.A=1 |
| SVADU-2M-04 | 2M A=0 D=0 store | 一次 store 同时置位 | store 成功，PTE.A=1 且 PTE.D=1 |
| SVADU-2M-05 | 2M A=1 D=0 amoadd.w | 2 MiB 叶 PTE：V=1, R=1, W=1, A=1, D=0，S-mode amoadd.w | AMO 成功，megapage PTE.D=1 |

---

### Group 5：ADUE=1 时 1 GiB gigapage 上的硬件 A/D 更新

**规范依据**：
- `norm:menvcfg_adue_op`：硬件 A/D 更新在叶 PTE 上执行，1 GiB gigapage 同样适用

**测试职责**：验证 ADUE=1 时硬件 A/D 更新行为在 1 GiB gigapage（Sv39 中 level 2 叶 PTE）上同样生效，覆盖 load / store / AMO 三种访问类型。1 GiB X 权限页面通常占据整个代码段，作为独立 fetch 用例不实用，故 fetch 测试由 SVADU-2M-03 与 SVADU-A4K-02 充分覆盖。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADU-1G-01 | 1G A=0 load | 1 GiB 叶 PTE：V=1, R=1, A=0, D=0，S-mode load | load 成功，gigapage PTE.A=1 |
| SVADU-1G-02 | 1G A=1 D=0 store | 1 GiB 叶 PTE：V=1, R=1, W=1, A=1, D=0，S-mode store | store 成功，gigapage PTE.D=1 |
| SVADU-1G-03 | 1G A=0 D=0 store | 一次 store 同时置位 A 与 D | store 成功，PTE.A=1 且 PTE.D=1 |
| SVADU-1G-04 | 1G A=1 D=0 amoadd.w | 1 GiB 叶 PTE：V=1, R=1, W=1, A=1, D=0，S-mode amoadd.w | AMO 成功，gigapage PTE.D=1 |

> [!NOTE]
> 1 GiB gigapage 测试需要确保测试 VA 选择在远离 M-mode 代码段与栈的恒等映射区域，避免与代码段所在的 1 GiB 区域冲突。

---

### Group 6：ADUE=0 时回退到 Svade 行为

**规范依据**：
- `norm:Svadu_disabled_hw_update_falls_back_to_svade`：当硬件 A/D 更新被禁用时，Svade 行为生效
- `norm:svade_access_ad_bit_clear`：A=0 被访问或 D=0 被写入时触发 page-fault

**测试职责**：测试当 `menvcfg.ADUE=0` 时，Svadu 实现的处理器行为应与 Svade 完全一致——A=0 / D=0 触发 page-fault 且 PTE 内容保持不变。本组用例与 `Svade_test_plan.md` 的 Group 1/2/3 形成对照。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADU-FB-01 | ADUE=0 A=0 load | ADUE=0，4 KiB PTE：V=1, R=1, A=0，S-mode load | load page-fault（scause=13） |
| SVADU-FB-02 | ADUE=0 D=0 store | ADUE=0，4 KiB PTE：V=1, R=1, W=1, A=1, D=0，S-mode store | store page-fault（scause=15） |
| SVADU-FB-03 | ADUE=0 A=0 fetch | ADUE=0，4 KiB PTE：V=1, X=1, A=0，S-mode 跳转执行 | instruction page-fault（scause=12） |
| SVADU-FB-04 | ADUE=0 AMO D=0 | ADUE=0，4 KiB PTE：V=1, R=1, W=1, A=1, D=0，S-mode amoadd.w | store/AMO page-fault（scause=15） |
| SVADU-FB-05 | ADUE=0 fault 后 PTE 不变 | 触发上述任一 fault 后回到 M-mode 检查 PTE | 对应 A/D 位均保持原值（未被硬件更新） |

> [!IMPORTANT]
> Group 6 是 `Svade_test_plan.md` Group 1/2/3 的"ADUE=0 镜像"，验证 Svadu 实现关闭硬件更新后必须等价于 Svade。任何在 Svade 实现下能通过的测试，在 Svadu + ADUE=0 配置下也必须通过。

---

### Group 7：运行时动态切换 ADUE

**规范依据**：
- `norm:menvcfg_adue_fence`：修改 `menvcfg.ADUE` 后必须执行 SFENCE.VMA(x0,x0)，新设置才能保证生效
- `norm:menvcfg_adue_op`：ADUE 值决定当前采用硬件更新还是 Svade 路径

**测试职责**：验证在 M-mode 中运行时动态切换 `menvcfg.ADUE` 并执行 SFENCE.VMA 后，S-mode 访问行为应立即按新配置工作。覆盖 0→1、1→0 切换以及多次切换稳定性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADU-SW-01 | 0→1 切换：fault 后启用硬件更新 | M-mode 置 ADUE=0，S-mode 触发 A=0 load fault → 返回 M-mode 置 ADUE=1 + SFENCE.VMA → 同一 PTE 不变（PTE.A 仍 0），再次进入 S-mode 访问同一 VA | 第一次：scause=13；第二次：load 成功，PTE.A=1（被硬件置位） |
| SVADU-SW-02 | 1→0 切换：禁用硬件更新 | M-mode 置 ADUE=1，S-mode 访问 A=0 页 P1 完成硬件置位 → 返回 M-mode 置 ADUE=0 + SFENCE.VMA → 映射新的 A=0 页 P2 → S-mode 访问 P2 | P1 访问成功且 P1.A=1；P2 访问触发 scause=13 且 P2.A 仍 0 |
| SVADU-SW-03 | 切换后必须 SFENCE.VMA 才生效 | 置 ADUE=0 → 立即置 ADUE=1 → 执行 SFENCE.VMA → S-mode 访问 A=0 页 | 访问成功，PTE.A=1（验证最终一次 fence 后正确生效） |
| SVADU-SW-04 | 多次切换稳定性 | ADUE 在 0/1 之间切换 ≥4 次，每次切换 + SFENCE.VMA 后用全新 PTE 上下文访问 A=0 页 | 每次切换后行为严格符合当前 ADUE 值（ADUE=1 成功置位、ADUE=0 触发 fault） |

> [!NOTE]
> 关于"未执行 SFENCE.VMA 时 ADUE 修改是否立即可见"——`norm:menvcfg_adue_fence` 明确要求执行同步指令。这意味着实现可以在未 fence 时返回旧行为；本测试计划采取**保守策略**，每次 ADUE 切换后均强制 SFENCE.VMA，不测试未 fence 的边界行为（避免测试与实现定义行为耦合）。

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1（ADUE 控制）、Group 2（4K A 位）、Group 3（4K D 位） | SVADU-CSR-01~04、SVADU-A4K-01~04、SVADU-D4K-01~04 | 扩展判定与核心硬件更新语义 |
| P1（重要） | Group 6（ADUE=0 回退）、Group 7（运行时切换） | SVADU-FB-01~05、SVADU-SW-01~04 | Svade 回退等价性与 ADUE 动态切换 + SFENCE 同步 |
| P2（建议） | Group 4（2 MiB）、Group 5（1 GiB） | SVADU-2M-01~05、SVADU-1G-01~04 | 大页粒度覆盖 |

---

## 结果判定原则

- 平台未实现 Svadu（ADUE 只读零）：Group 1 检测用例显性报告"未实现"，其余组 SKIP，不做失败处理（Svadu 为可选扩展）。
- 平台实现 Svadu 但行为偏离 SPEC（如 ADUE=1 时不更新 A/D、ADUE=0 时不回退到 Svade、切换后未 fence 即断言生效等）：保持用例失败，与 SPEC 比对后将问题记录至 `bugs/` 目录，禁止修改用例或加 workaround 适配错误实现。

---

## 附录：规范性引用

### menvcfg.ADUE 字段

| 字段 | 位置 | 含义 |
|------|------|------|
| `ADUE` | menvcfg[61] | 启用硬件更新 PTE A/D 位（0=禁用回退到 Svade，1=启用硬件更新） |

### 相关 scause 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| Instruction page fault | 12 | 取指页错误 |
| Load page fault | 13 | load 页错误 |
| Store/AMO page fault | 15 | store/AMO 页错误 |

---

## 参考

- `svadu.adoc` — Svadu 扩展定义
- `machine.adoc` — `menvcfg` CSR 与 ADUE 字段语义
- `supervisor.adoc` — Svade 概念与翻译算法
- `Svade_test_plan.md` — Svade 独立测试计划（Group 6 回退对照）
- `Hypervisor_Sv_test_plan.md` — Hypervisor × Svadu 交叉测试计划

---

## 附录 A：规范点覆盖矩阵

下表标明"覆盖的规范点"章节中每条规范点被哪些测试用例覆盖。

| Norm ID | 覆盖的测试 ID |
|---------|---------------|
| `norm:Svadu_hw_update_a_d_bits` | SVADU-CSR-01~04 |
| `norm:menvcfg_adue_rdonly0` | SVADU-CSR-01 |
| `norm:menvcfg_adue_op` | SVADU-A4K-01~04、SVADU-D4K-01~04、SVADU-2M-01~05、SVADU-1G-01~04、SVADU-FB-01~05 |
| `norm:Svadu_disabled_hw_update_falls_back_to_svade` | SVADU-FB-01~05、SVADU-SW-01、SVADU-SW-02 |
| `norm:svade_access_ad_bit_clear` | SVADU-FB-01~04 |
| `norm:menvcfg_adue_fence` | SVADU-SW-01~04 |
