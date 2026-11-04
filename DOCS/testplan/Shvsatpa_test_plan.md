**中文 | [English](../testplan_en/Shvsatpa_test_plan_en.md)**

# Shvsatpa 扩展测试计划

本文档描述 Shvsatpa（Translation Mode Support for vsatp, Version 1.0）扩展的测试计划。Shvsatpa 要求：所有 `satp` 支持的翻译模式（translation modes）在 `vsatp` 中也必须被支持。即 `satp.MODE` 能持有的合法值，`vsatp.MODE` 同样能持有。

---

## 测试范围

### 本文档覆盖的 SPEC 章节

本方案依据 RISC-V Privileged Architecture 规范（Shvsatpa 扩展章节与 Hypervisor/Supervisor 扩展 vsatp/satp 相关章节）编写：

- 本地 SPEC 路径：
  - `SPEC/riscv-isa-manual/src/priv/shvsatpa.adoc` — Shvsatpa Extension for Translation Mode Support, Version 1.0
  - `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — `vsatp` 寄存器定义（第 1382–1425 行）：字段布局、V=1 时替代 `satp` 的行为、不支持 MODE 值的写入处理
  - `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — `satp` 寄存器与 MODE 字段编码（第 997–1065 行）：MODE 编码表及 WARL 写入语义
- 官方 GitHub 仓库：https://github.com/riscv/riscv-isa-manual （按 `.gitmodules` 中 `SPEC/riscv-isa-manual` 映射）

### 关键参考文件

| 路径 | 说明 |
|------|------|
| `shvsatpa.adoc` | Shvsatpa 规范全文（共 7 行） |
| `hypervisor.adoc:1382-1425` | `vsatp` 寄存器规范：VSXLEN-bit RW、V=1 时替代 `satp`、MODE 写入处理 |
| `supervisor.adoc:997-1065` | `satp` 寄存器规范：MODE 编码表（Bare=0, Sv39=8, Sv48=9, Sv57=10）、WARL 行为 |
| `common/encoding.h` | `CSR_VSATP = 0x280`、`CSR_SATP = 0x180` |
| `common/hyp/hyp_priv.h:21` | `run_in_vs_mode(fn, arg)` — 在 VS-mode (V=1) 执行测试函数 |
| `common/hyp/hyp_csr.h` | Hypervisor CSR 读写辅助函数 |
| `common/hyp/hyp_test.h` | Hypervisor 测试宏（`HYP_TEST_END` 等） |
| `common/hyp/hyp_reset.c` | `hyp_reset_state()` 重置 hypervisor CSR 状态 |
| `DOCS/framework/hypervisor_framework.md` | Hypervisor 测试框架总览 |

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:shvsatpa_satp_vsatp_modes` | If the Shvsatpa extension is implemented, all translation modes supported in `satp` must be supported in `vsatp`. | 若实现了 Shvsatpa 扩展，`satp` 中支持的所有翻译模式在 `vsatp` 中也必须支持。 |
| `norm:vsatp_sz_acc_op` | The `vsatp` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `satp`. When V=1, `vsatp` substitutes for the usual `satp`. `vsatp` controls VS-stage address translation, the first stage of two-stage translation for guest virtual addresses. | `vsatp` 是 VSXLEN 位读写寄存器，VS 模式版本的 `satp`。V=1 时替代 `satp`。`vsatp` 控制 VS 阶段地址翻译。 |
| `norm:vsatp_mode_unsupported_v0` | When V=0, a write to `vsatp` with an unsupported MODE value is either ignored as it is for `satp`, or the fields of `vsatp` are treated as WARL in the normal way. | V=0 时，使用不支持的 MODE 值写入 `vsatp`，要么像 `satp` 一样被忽略，要么按常规 WARL 方式处理。 |
| `norm:vsatp_mode_unsupported_v1` | However, when V=1, a write to `satp` with an unsupported MODE value is ignored and no write to `vsatp` is effected. | V=1 时，使用不支持的 MODE 值写入 `satp` 被忽略，不会对 `vsatp` 生效。 |
| `norm:vsatp_v0` | When V=0, `vsatp` does not directly affect the behavior of the machine, unless a virtual-machine load/store (HLV, HLVX, or HSV) or the MPRV feature in the `mstatus` register is used to execute a load or store as though V=1. | V=0 时 `vsatp` 不直接影响机器行为，除非使用虚拟机 load/store 或 MPRV 功能以 V=1 方式执行。 |
| `norm:satp_mode` | When MODE=Bare, supervisor virtual addresses are equal to supervisor physical addresses, and there is no additional memory protection. To select MODE=Bare, software must write zero to the remaining fields of `satp`. Attempting to select MODE=Bare with a nonzero pattern in the remaining fields has an UNSPECIFIED effect. | MODE=Bare 时，S 虚拟地址等于 S 物理地址，无额外内存保护。选择 Bare 模式时软件必须将 `satp` 其余字段写零。非零模式时效果未指定。 |
| `norm:satp_mode_sxlen64` | When SXLEN=64, three paged virtual-memory schemes are defined: Sv39, Sv48, and Sv57. One additional scheme, Sv64, will be defined in a later version. The remaining MODE settings are reserved for future use. | SXLEN=64 时定义了三种分页方案：Sv39、Sv48、Sv57。Sv64 将在后续版本定义。其余 MODE 设置保留。 |
| `norm:satp_mode_op_unsupported` | If a write to `satp` specifies a MODE value that is not supported, the entire write has no effect; no fields in `satp` are modified. | 如果写入 `satp` 的 MODE 值不被支持，整个写入无效；`satp` 中没有任何字段被修改。 |
| `vsatp_asid_warl` | The ASID field of `vsatp` is WARL, behaving like the ASID field of `satp`.（源自 hypervisor.adoc vsatp 字段描述，该处无官方 norm: 标签） | vsatp 的 ASID 字段为 WARL，行为类似 satp 的 ASID。 |

> [!IMPORTANT]
> Shvsatpa 的核心约束只有一条：`satp` 支持什么 MODE，`vsatp` 就必须支持什么 MODE。本计划通过"先探测 satp 支持的 MODE 集合，再验证 vsatp 对同一集合的支持"来确认一致性。Group 4 验证 V=1 透传语义（VS-mode 通过 satp 指令名操作实际 vsatp），Group 5 验证不支持 MODE 值的处理行为在 V=0 和 V=1 下分别符合规范。

### 不在测试范围内

- **satp/vsatp 的地址翻译功能正确性**：本计划只验证 MODE 字段是否可持有，不验证翻译查表行为
- **M-mode 的 satp 行为**：Shvsatpa 仅约束 satp 与 vsatp 之间的模式一致性
- **ASID / PPN 字段宽度全面测试**：仅做基本写回读验证，不穷举
- **多 hart 场景**：项目为单核测试环境
- **SXLEN=32 场景**：仅覆盖 SXLEN=64（RV64 环境）
- **G-stage 地址翻译验证**：本测试不涉及两级翻译的 G-stage 正确性
- **SFENCE.VMA / HFENCE.VVMA 行为**：TLB 管理不在本计划范围

---

## 设计要点

### 1. MODE 探测方法

`satp`/`vsatp` 的 MODE 字段是 WARL：写入一个不支持的值则整个写被忽略。因此探测策略为：

1. 备份当前 CSR 值
2. 写入目标 MODE（将 MODE 写为目标值，ASID=0，PPN=0）
3. 回读 CSR
4. 如果回读的 MODE == 写入的 MODE → 该 MODE 被支持
5. 如果回读的 MODE != 写入的 MODE → 该 MODE 不被支持（写被忽略）
6. 还原 CSR

> [!NOTE]
> 对于 Bare（MODE=0）的探测需特殊处理：规范要求选择 Bare 时其余字段必须为 0（`supervisor.adoc:1014-1016`），因此探测 Bare 时写入全零。

### 2. satp 与 vsatp 的访问路径

| CSR | 地址 | V=0 时直接访问 | V=1 时通过 satp 指令访问 |
|-----|------|---------------|------------------------|
| `satp` | 0x180 | HS-mode `csrr/csrw satp` | N/A（V=1 时 satp 指令操作 vsatp） |
| `vsatp` | 0x280 | HS/M-mode `csrr/csrw 0x280` | VS-mode `csrr/csrw satp` 实际操作 vsatp |

- **Group 1 & 2 & 5**：M/HS-mode 直接操作 CSR 0x180（satp）和 CSR 0x280（vsatp）
- **Group 3 & 4**：VS-mode (V=1) 通过 `satp` 指令名间接操作 vsatp

### 3. MODE 编码（SXLEN=64）

| 值 | 名称 | 描述 |
|----|------|------|
| 0 | Bare | 无翻译或保护 |
| 1-7 | — | 保留（标准使用） |
| 8 | Sv39 | 39 位分页虚拟地址 |
| 9 | Sv48 | 48 位分页虚拟地址 |
| 10 | Sv57 | 57 位分页虚拟地址 |
| 11 | Sv64 | 64 位（未来规范，当前保留） |
| 12-15 | — | 保留 |

### 4. 与 hyp_reset_state 的隔离

`common/hyp/hyp_reset.c::hyp_reset_state()` 重置 hypervisor CSR 状态。本测试每个用例操作前先备份目标 CSR，操作完成后还原。VS-mode 内的操作通过 `run_in_vs_mode` 的 ecall 返回机制回到 M-mode 后进行断言与清理。

### 5. 模式支持的前置条件

Group 1 探测 `satp` 各 MODE 的 WARL 回读行为，探测本身即被测对象，不向其它组传递状态。其它组以某个 SvNN 可用为前置条件时，以平台配置声明的 `SV39_SUPPORTED` / `SV48_SUPPORTED` / `SV57_SUPPORTED` 为准。

---

## 测试分组

> [!IMPORTANT]
> 共 6 个测试组、19 个测试用例。Group 1 在 M/HS-mode 探测 satp 支持的模式集；Group 2 在 M/HS-mode 直接验证 vsatp 支持相同模式集；Group 3 在 VS-mode (V=1) 通过 satp 指令验证 vsatp 模式支持；Group 4 验证 V=1 透传语义；Group 5 验证不支持 MODE 值的写入处理；Group 6 验证 ASID 字段宽度一致性。每组提供：规范依据、测试职责、测试用例表（ID/名称/描述/预期结果）。

---

### Group 1：`satp` MODE 支持探测

**规范依据**：
- `norm:satp_mode`（`supervisor.adoc:1009-1018`）：MODE 字段编码定义
- `norm:satp_mode_sxlen64`（`supervisor.adoc:1044-1050`）：SXLEN=64 时定义 Sv39/Sv48/Sv57
- `norm:satp_mode_op_unsupported`（`supervisor.adoc:1052-1055`）：写入不支持的 MODE 整个写无效

**测试职责**：在 M/HS-mode 探测 `satp` 各翻译模式 MODE 字段的 WARL 回读行为，该行为本身即被测对象。对每个标准 MODE 值（Bare=0, Sv39=8, Sv48=9, Sv57=10）进行写入-回读测试。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSATP-PROBE-01 | satp MODE=Bare 探测 | 写 `satp = 0`（MODE=0, ASID=0, PPN=0），回读 MODE 字段 | 回读 MODE=0（Bare 通常被支持） |
| VSATP-PROBE-02 | satp MODE=Sv39 探测 | 写 `satp = 8 << 60`（MODE=8），回读 MODE 字段 | 回读 MODE ∈ {0, 8} |
| VSATP-PROBE-03 | satp MODE=Sv48 探测 | 写 `satp = 9 << 60`（MODE=9），回读 MODE 字段 | 回读 MODE ∈ {之前有效值, 9} |
| VSATP-PROBE-04 | satp MODE=Sv57 探测 | 写 `satp = 10 << 60`（MODE=10），回读 MODE 字段 | 回读 MODE ∈ {之前有效值, 10} |

> [!NOTE]
> 探测逻辑：写入前先把 `satp` 设为已知状态（Bare=0），然后写入目标 MODE。如果回读 MODE == 目标值，则支持；如果回读 MODE == 之前值（0），则不支持。至少一个非 Bare 模式应被支持（否则 H 扩展几乎无法运行 guest OS）。

---

### Group 2：`vsatp` MODE 对 `satp` 支持模式的一致性验证（V=0 直接访问）

**规范依据**：
- `norm:shvsatpa_satp_vsatp_modes`（`shvsatpa.adoc:4-6`）：所有 `satp` 支持的翻译模式在 `vsatp` 中也必须被支持
- `norm:vsatp_sz_acc_op`（`hypervisor.adoc:1384-1392`）：`vsatp` 是 VSXLEN-bit RW 寄存器

**测试职责**：对平台配置声明 `satp` 支持的每个 MODE，在 M/HS-mode 直接写入 `vsatp`（CSR 0x280）并验证回读 MODE 一致，确认 vsatp 支持同样的模式集合。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSATP-MODE-01 | vsatp MODE=Bare 支持 | 写 `vsatp = 0`（MODE=0），回读 MODE | `MODE == 0`（Bare 必须被 vsatp 支持） |
| VSATP-MODE-02 | vsatp MODE=Sv39 支持 | 仅当平台配置声明支持 Sv39 时执行；写 `vsatp = 8 << 60`，回读 MODE | `MODE == 8`（Shvsatpa 要求一致） |
| VSATP-MODE-03 | vsatp MODE=Sv48 支持 | 仅当平台配置声明支持 Sv48 时执行；写 `vsatp = 9 << 60`，回读 MODE | `MODE == 9`（Shvsatpa 要求一致） |
| VSATP-MODE-04 | vsatp MODE=Sv57 支持 | 仅当平台配置声明支持 Sv57 时执行；写 `vsatp = 10 << 60`，回读 MODE | `MODE == 10`（Shvsatpa 要求一致） |

> [!IMPORTANT]
> 这是 Shvsatpa 扩展的核心验证组。如果 satp 支持 Sv39 但 vsatp 不支持，则 Shvsatpa 约束被违反。断言条件为：**对于 satp 支持的每个 MODE，写入 vsatp 后回读的 MODE 必须等于写入值**。

---

### Group 3：VS-mode (V=1) 通过 `satp` 指令验证 `vsatp` 模式支持

**规范依据**：
- `norm:shvsatpa_satp_vsatp_modes`（`shvsatpa.adoc:4-6`）：satp 支持的模式 vsatp 也必须支持
- `norm:vsatp_sz_acc_op`（`hypervisor.adoc:1384-1392`）：V=1 时 `vsatp` 替代 `satp`
- `norm:vsatp_mode_unsupported_v1`（`hypervisor.adoc:1417-1418`）：V=1 时写不支持 MODE 则写被忽略

**测试职责**：在 VS-mode (V=1) 通过 `satp` 指令名（实际操作 vsatp）写入 satp 支持的各 MODE 并回读确认。这同时验证了 V=1 透传路径和 Shvsatpa 一致性约束。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSATP-VS-01 | VS-mode 写 satp MODE=Bare | VS-mode `csrw satp, 0`，回读 `satp`（实际操作 vsatp） | 回读 MODE=0 |
| VSATP-VS-02 | VS-mode 写 satp MODE=Sv39 | 仅当 satp 支持 Sv39；VS-mode `csrw satp, 8<<60`，回读 | 回读 MODE=8 |
| VSATP-VS-03 | VS-mode 写 satp MODE=Sv48 | 仅当 satp 支持 Sv48；VS-mode `csrw satp, 9<<60`，回读 | 回读 MODE=9 |
| VSATP-VS-04 | VS-mode 写 satp MODE=Sv57 | 仅当 satp 支持 Sv57；VS-mode `csrw satp, 10<<60`，回读 | 回读 MODE=10 |

> [!NOTE]
> VS-mode 中回读 `satp` 实际读的是 `vsatp`（V=1 透传）。因此可在 VS-mode 内完成“写入→回读→判断”闭环。但最终 M-mode 也要通过 CSR 0x280 进行交叉验证（见 Group 4）。

---

### Group 4：V=1 透传语义验证（`satp` 访问实际操作 `vsatp`）

**规范依据**：
- `norm:vsatp_sz_acc_op`（`hypervisor.adoc:1384-1392`）：When V=1, `vsatp` substitutes for the usual `satp`, so instructions that normally read or modify `satp` actually access `vsatp` instead

**测试职责**：验证 VS-mode (V=1) 通过 `satp` 指令名写入的值能从 M/HS-mode 通过 `vsatp`（CSR 0x280）读回；反向亦然。确认 V=1 时的 satp 操作不影响 HS-mode 的真实 satp。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSATP-TRANS-01 | VS-mode 写 satp → M-mode 读 vsatp | VS-mode 写 `satp = TEST_VAL`（含支持的 MODE），返回 M-mode 后读 CSR 0x280 | `vsatp == TEST_VAL` |
| VSATP-TRANS-02 | M-mode 写 vsatp → VS-mode 读 satp | M-mode 写 `vsatp = TEST_VAL`，进入 VS-mode 后读 `satp` | VS-mode 读回 == `TEST_VAL` |
| VSATP-TRANS-03 | VS-mode 写 satp 不影响 HS-mode 的 satp | 备份 HS 的 `satp`；VS-mode 写 `satp = TEST_VAL`；返回 HS-mode 后读真实 `satp`（V=0） | HS-mode 的 `satp` 保持原值 |

---

### Group 5：不支持 MODE 值的写入处理

**规范依据**：
- `norm:vsatp_mode_unsupported_v0`（`hypervisor.adoc:1415-1416`）：V=0 时写 `vsatp` 不支持的 MODE 值，要么整个写被忽略（同 satp 行为），要么字段按 WARL 正常处理
- `norm:vsatp_mode_unsupported_v1`（`hypervisor.adoc:1417-1418`）：V=1 时写 `satp`（实际写 vsatp）不支持的 MODE 值，该写**被忽略**，vsatp 不发生修改
- `norm:satp_mode_op_unsupported`（`supervisor.adoc:1052-1055`）：satp 写入不支持 MODE 整个写无效果

**测试职责**：验证对 vsatp 写入不支持的 MODE 值时，V=0 和 V=1 两种路径的行为分别符合规范。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSATP-UNSUP-01 | V=0 写 vsatp 保留 MODE=7 | M-mode 先设 vsatp=Bare，再写 MODE=7，回读 | 回读 MODE != 7（WARL：要么整个写被忽略回读为 0，要么 MODE 被强制为合法值） |
| VSATP-UNSUP-02 | V=0 写 vsatp 保留 MODE=15 | M-mode 先设 vsatp=Bare，再写 MODE=15，回读 | 回读 MODE != 15（同上） |
| VSATP-UNSUP-03 | V=1 写 satp 不支持 MODE（整个写被忽略） | 先 M-mode 设 vsatp=已知值；VS-mode 写 satp 带不支持的 MODE；M-mode 回读 vsatp | vsatp 保持原值不变（V=1 下整个写被忽略） |

> [!NOTE]
> VSATP-UNSUP-03 是 V=1 特有行为：与 V=0 不同，V=1 时写不支持 MODE 会**完全忽略写入**（而非 WARL 处理）。这意味着即使写入中的 ASID/PPN 字段是合法的，也不会被写入。

---

### Group 6：ASID 字段宽度验证

**规范依据**：
- `vsatp_asid_warl`（`hypervisor.adoc:1394-1400` ）：vsatp 的 ASID 字段为 WARL，行为类似 satp 的 ASID
- `norm:shvsatpa_satp_vsatp_modes`（`shvsatpa.adoc:4-6`）：Shvsatpa 隐含"vsatp 支持的 MODE 也意味着 ASID 字段可用"

**测试职责**：验证 vsatp 的 ASID 字段宽度与 satp 一致（Shvsatpa 要求模式一致性，ASID 宽度应同步）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSATP-ASID-01 | vsatp ASID 宽度与 satp 一致 | 探测 satp ASID 宽度（写全 1 回读）；同样探测 vsatp ASID 宽度；比较两者 | vsatp ASID 宽度 == satp ASID 宽度（均为 WARL，支持的位数应相同） |

---

## 附录：相关常量与 API 参考

### CSR 与位定义

| 名称 | 值 | 说明 |
|------|-----|------|
| `CSR_VSATP` | `0x280` | Virtual supervisor address translation and protection，见 `common/encoding.h` |
| `CSR_SATP` | `0x180` | Supervisor address translation and protection（V=1 时访问实际操作 vsatp） |
| `SATP_MODE_SHIFT` | 60 | MODE 字段在 SXLEN=64 时的起始 bit 位置 |
| `SATP_MODE_MASK` | `0xF << 60` | MODE 字段掩码（4 bit） |

### MODE 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| `MODE_BARE` | 0 | 无翻译保护 |
| `MODE_SV39` | 8 | 39-bit 分页虚拟地址 |
| `MODE_SV48` | 9 | 48-bit 分页虚拟地址 |
| `MODE_SV57` | 10 | 57-bit 分页虚拟地址 |
| `MODE_SV64` | 11 | 64-bit（未来规范） |

### 测试框架 API

- `run_in_vs_mode(fn, arg)`：在 VS-mode (V=1) 执行 fn(arg)，通过 ecall 返回 M-mode
- `hyp_reset_state()`：重置所有 hypervisor CSR
- `HYP_TEST_END()`：测试结束宏，含 hyp_reset_state + 结果记录
- `TEST_REGISTER` / `TEST_BEGIN` / `TEST_ASSERT` / `TEST_SKIP` / `TEST_END`：测试用例注册与断言宏
- `TEST_LOG(fmt, ...)`：测试日志输出

---

## 测试执行说明

### 运行环境

- Group 1：M/HS-mode 直接操作 CSR 0x180（satp），探测支持模式
- Group 2：M/HS-mode 直接操作 CSR 0x280（vsatp），验证模式一致性
- Group 3：通过 `run_in_vs_mode` 进入 VS-mode，VS-mode 中通过 satp 指令操作 vsatp
- Group 4：M-mode ↔ VS-mode 交替执行，验证 CSR 透传
- Group 5：M/HS-mode + VS-mode 验证不支持 MODE 的写入行为
- 单核环境，无需 IPI
- 使用 G-stage 恒等映射保证 VS-mode 执行环境

### 失败定位指引

| 现象 | 可能原因 |
|------|----------|
| VSATP-PROBE-02/03/04 全部显示不支持 | 平台未实现任何分页模式（仅 Bare），需确认硬件能力 |
| VSATP-MODE-02 失败（satp 支持 Sv39 但 vsatp 不支持） | **Shvsatpa 约束违反**：vsatp.MODE 被过度 WARL 截断 |
| VSATP-MODE-03/04 失败 | 同上，vsatp 未实现 satp 支持的高级翻译模式 |
| VSATP-VS-02/03/04 失败 | V=1 路径下 MODE 写入可能有额外限制（不符合 Shvsatpa） |
| VSATP-TRANS-01/02 失败 | V=1 时 satp 指令未正确映射到 vsatp，H 扩展实现异常 |
| VSATP-TRANS-03 失败 | VS-mode 写 satp 错误地修改了 HS-mode 的 satp |
| VSATP-UNSUP-01/02 回读 MODE 等于写入的保留值 | vsatp.MODE 的 WARL 实现异常（不应保持非法值） |
| VSATP-UNSUP-03 失败（vsatp 被修改） | V=1 下写不支持 MODE 未被完全忽略，违反 `norm:vsatp_mode_unsupported_v1` |

---

## 附录：规范点覆盖矩阵

| Norm ID | 覆盖用例 | 备注 |
|---------|----------|------|
| `norm:shvsatpa_satp_vsatp_modes` | VSATP-MODE-01 ~ VSATP-MODE-04, VSATP-VS-01 ~ VSATP-VS-04, VSATP-ASID-01 | 核心约束：satp 支持的 MODE 在 vsatp 中也必须支持 |
| `norm:vsatp_sz_acc_op` | VSATP-MODE-01 ~ VSATP-MODE-04, VSATP-VS-01 ~ VSATP-VS-04, VSATP-TRANS-01 ~ VSATP-TRANS-03 | 寄存器属性与 V=1 替代语义 |
| `norm:vsatp_mode_unsupported_v0` | VSATP-UNSUP-01, VSATP-UNSUP-02 | V=0 时两种合法处理均接受（忽略或 WARL） |
| `norm:vsatp_mode_unsupported_v1` | VSATP-VS-01 ~ VSATP-VS-04, VSATP-UNSUP-03 | V=1 时写不支持 MODE 必须被完全忽略 |
| `norm:vsatp_v0` | VSATP-MODE-01 ~ VSATP-MODE-04, VSATP-UNSUP-01 ~ VSATP-UNSUP-02 | V=0 时写 vsatp 不直接影响机器行为（用例在 V=0 反复写回读不触发翻译副作用） |
| `norm:satp_mode` | VSATP-PROBE-01 | MODE 编码定义，Bare 探测时其余字段写零 |
| `norm:satp_mode_sxlen64` | VSATP-PROBE-01 ~ VSATP-PROBE-04 | 探测基线：SXLEN=64 定义的 MODE 集合 |
| `norm:satp_mode_op_unsupported` | VSATP-PROBE-01 ~ VSATP-PROBE-04 | 探测机制依赖：satp 写不支持 MODE 整个写被忽略 |
| `vsatp_asid_warl` | VSATP-ASID-01 | vsatp 与 satp 的 ASID 宽度一致性 |
| satp/vsatp 翻译查表正确性 | — | 不覆盖：仅验证 MODE 可持有，见“不在测试范围内” |
