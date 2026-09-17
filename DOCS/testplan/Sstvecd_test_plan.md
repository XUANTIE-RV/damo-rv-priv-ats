**中文 | [English](../testplan_en/Sstvecd_test_plan_en.md)**

# Sstvecd 扩展测试计划

本文档描述 Sstvecd（Direct Trap Vectoring, Version 1.0）扩展的测试计划。Sstvecd 是一个对 `stvec` 寄存器行为的窄约束扩展，规范要求实现必须保证：(1) `stvec.MODE` 能写入并保持 0（Direct）；(2) 在 `stvec.MODE=Direct` 时，`stvec.BASE` 必须能保持任意 4 字节对齐的地址。

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/sstvecd.adoc` — Sstvecd Extension for Direct Trap Vectoring, Version 1.0
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — `stvec` 寄存器与 MODE 字段编码、Direct/Vectored 行为定义、`sip`.SSIP 自激发行为

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内 src/priv/sstvecd.adoc、src/priv/supervisor.adoc）

---

## 测试范围

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sstvecd_stvec_mode_direct` | If the Sstvecd extension is implemented, then `stvec.MODE` must be capable of holding the value 0 (Direct). | 如果实现了 Sstvecd 扩展，则 `stvec.MODE` 必须能够保存值 0（直接模式）。 |
| `norm:sstvecd_stvec_base_aligned_address` | Furthermore, when `stvec.MODE=Direct`, `stvec.BASE` must be capable of holding any valid four-byte-aligned address. | 此外，当 `stvec.MODE=Direct` 时，`stvec.BASE` 必须能够保存任何有效的四字节对齐地址。 |
| `norm:stvec_op` | The BASE field in `stvec` is a field that can hold any valid virtual or physical address, subject to the following alignment constraints: the address must be 4-byte aligned, and MODE settings other than Direct might impose additional alignment constraints on the value in the BASE field. | BASE 字段可保存任何有效虚拟或物理地址，但必须 4 字节对齐，非 Direct 模式可能施加更严格的对齐约束。 |
| `norm:stvec_sz_base` | The CSR contains only bits XLEN-1 through 2 of the address BASE. When used as an address, the lower two bits are filled with zeroes to obtain an XLEN-bit address that is always aligned on a 4-byte boundary. | CSR 仅保存 BASE 地址的 [XLEN-1:2] 位。用作地址时低两位填零，获得始终 4 字节对齐的 XLEN 位地址。 |

> [!IMPORTANT]
> Sstvecd 规范本身只有两条强约束（Direct 可保持、BASE 4 字节对齐持有能力）。Group 3 的 trap 跳转测试是对"BASE 设置确实生效、Direct 模式行为正确"的端到端验证，规范依据来自 `supervisor.adoc` 中 stvec MODE 编码定义 —— Sstvecd 强约束 Direct 模式可用，自然要求该模式行为符合 supervisor 规范。

### 不在测试范围内

- **Vectored 模式的功能性行为**：Sstvecd 不要求实现 Vectored 模式；本计划只在 Group 1 中"探测"Vectored 是否被实现，不验证其正确性
- **VS-mode 的 `vstvec`**：Sstvecd 仅约束 `stvec`，不涉及 `vstvec`
- **M-mode 的 `mtvec`**：不在 Sstvecd 规范范畴
- **多 hart 场景**：项目为单核测试环境
- **Sv32 / Sv48 / Sv57 模式**：仅覆盖 RV64 + Sv39，与项目其它扩展计划保持一致
- **不同 SXLEN 下的 BASE 范围**：仅覆盖 SXLEN=64

---

## 设计要点

### 1. 实现存在性的处理

平台必须在构建配置中启用 Sstvecd 扩展。若未启用，Group 1 的"MODE=0 写后回读=0"在大多数实现下仍然能通过（任何合规 RISC-V 实现都至少支持 Direct=0），因此该用例实际上无法独立证明 Sstvecd 已启用。

为强化"Sstvecd 必须启用"的前提，Group 2 STVEC-BASE-04 用例中**故意写入一个高位（如 bit 38 / bit 47 / 接近 SXLEN 上界）的 4 字节对齐地址** —— 若实现未声明 Sstvecd，可能将 BASE 的高位视为 WARL 截断（read-only 0），从而暴露实现差异。该用例使用 hard-assert，失败即说明平台未真正实现 Sstvecd。

### 2. trap entry 的安置（Group 3 专用）

Group 3 需要一个用户可控的 S-mode trap entry，要求：

- **4 字节对齐**：满足 `stvec.BASE` 对齐约束
- **位于可执行代码段**：链接脚本无需特殊处理
- **最小化处理路径**：保存通用寄存器与 `sscratch`，将命中地址（即当前 PC，等于 `stvec.BASE`）记录到全局变量、将 `scause` 记录到另一全局变量，随后按压缩指令宽度推进 `sepc` 并 `sret` 返回

关键设计要点：
- trap entry 内部通过 `auipc` + 偏移计算得到自身 PC，写入全局变量供断言"trap 命中地址 = stvec.BASE"
- `sepc` 推进需适配压缩指令（读取 `sepc` 处指令长度决定推进 2 或 4 字节），保证返回点正确

### 3. 与默认框架的隔离

框架默认 `reset_state()` 会把 `stvec` 设为默认 S-mode trap entry。本测试每个用例应遵循以下模板：

1. 进入用例后，先保存当前 `stvec` 值
2. 执行测试体（可能修改 `stvec`）
3. 退出前恢复 `stvec`，避免污染后续测试

不依赖 `medeleg`：Sstvecd 不涉及异常委托语义。Group 3 的同步异常通过框架提供的 S-mode 执行辅助在 S-mode 内主动触发；触发时 trap 直接进入 S-mode（而非被 M-mode 委托），命中我们设置的 `stvec`。

### 4. 异步中断的触发（Group 3 STVEC-INT-01）

为验证"Direct 模式下中断也跳到 BASE 而非 BASE+4×cause"，使用 SSIP（supervisor software interrupt）：

1. M-mode 设置 `mideleg.SSIE`（bit 1）= 1，把 supervisor software interrupt 委托到 S-mode
2. M-mode 准备好记录命中地址与 cause 的全局变量为 0
3. 进入 S-mode 后：先写 `stvec = BASE`（MODE=0），然后使能 `sie.SSIE`、`sstatus.SIE`，最后自触发 `sip.SSIP`
4. trap 命中后，断言：命中 PC == BASE（Direct）；若实现是 Vectored，命中地址应为 `BASE + 4*1 = BASE+0x4`，断言失败即可定位到 Direct 行为不正确

> [!NOTE]
> SSIP 是 supervisor 内部可写的中断标志（`norm:sip_ssip_sie_ssie`），最适合在单核环境下自激发。STIP（timer）需要 Sstc 或 SBI，SEIP（external）需要 PLIC/AIA，相对复杂。

### 5. WARL 写入回读约定

`stvec.MODE` 字段是 WARL：

- 写 0（Direct）必然成功（Sstvecd 强约束）
- 写 1（Vectored）：若实现支持，回读为 1；若不支持，回读为某个合法值（最自然的选择是 0，因为 Direct 必然支持）。两种行为都不算违反 Sstvecd
- 写 ≥2（Reserved）：必须回读为合法值（0 或 1），不应被锁死

`stvec.BASE` 字段（[XLEN-1:2]）是 WARL：

- 写入低 2 bit 非 0 的值，实现应把低 2 bit 强制为 0（`norm:stvec_sz_base`）
- 写入的高位 BASE 应原样回读（Sstvecd 要求"any valid 4-byte aligned address"）

---

## 测试分组

> [!IMPORTANT]
> 共 3 个测试组、16 个测试用例。所有测试运行于 RV64 + M-mode 设置 + 部分用例进入 S-mode（Group 3）。

---

### Group 1：`stvec.MODE` 可写性

**规范依据**：
- `norm:sstvecd_stvec_mode_direct`：`stvec.MODE` 必须能保持值 0（Direct）

**测试职责**：验证 `stvec.MODE` 能稳定写入并保持 Direct 值（0）；探测 Vectored（1）是否被实现；验证写入保留值（≥2）后 WARL 行为合理。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| STVEC-MODE-01 | MODE 写 0（Direct）回读 | 写 `stvec = 0`（BASE=0, MODE=0），回读 `stvec[1:0]` | `stvec[1:0] == 0`（Direct 必然可保持） |
| STVEC-MODE-02 | MODE 0→1→0 切换 | 先写 MODE=0、回读确认；再写 MODE=1、回读记录；最后写 MODE=0、回读确认 | 第 1、3 次回读 `MODE==0`；第 2 次回读 ∈ {0, 1}（实现自由） |
| STVEC-MODE-03 | MODE 写保留值（=2） | 写 `stvec = 0x2`（BASE=0, MODE=2），回读 `stvec[1:0]` | 回读 ∈ {0, 1}，不应为 2（WARL，保留值不应被锁死） |
| STVEC-MODE-04 | MODE 写保留值（=3） | 写 `stvec = 0x3`（BASE=0, MODE=3），回读 `stvec[1:0]` | 回读 ∈ {0, 1}，不应为 3 |

---

### Group 2：`stvec.BASE` 在 Direct 模式下的持有能力

**规范依据**：
- `norm:sstvecd_stvec_base_aligned_address`：当 `stvec.MODE=Direct` 时，`stvec.BASE` 必须能保持任意有效的 4 字节对齐地址
- `norm:stvec_sz_base`：CSR 仅存 BASE 的 [XLEN-1:2]，低 2 bit 写入时被强制为 0

**测试职责**：验证在 MODE=Direct 下，BASE 字段能保持各类 4 字节对齐地址（含跨 1 GiB / 512 GiB 边界、近 SXLEN 上界）；验证非 4 字节对齐地址写入时低 2 bit 被强制清零；验证 BASE 与 MODE 写入相互独立。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| STVEC-BASE-01 | BASE 写 0 回读 | 写 `stvec = 0x0`（BASE=0, MODE=0），回读 | `stvec == 0x0` |
| STVEC-BASE-02 | BASE 写平台内存基址回读 | 写 `stvec = PLATFORM_MEM_BASE`（MODE=0），回读 | 回读高位 == 平台内存基址、低 2 bit == 0 |
| STVEC-BASE-03 | BASE 写跨 1 GiB 边界 | 写 `stvec = 0x40000004`（跨 1 GiB），回读 | 回读 == `0x40000004` |
| STVEC-BASE-04 | BASE 写跨 512 GiB 边界（高位探测） | 写 `stvec = 0x8000000000`（bit 39），回读 | 回读 == `0x8000000000`（若高位被 WARL 截断，证明 Sstvecd 未真正启用，失败） |
| STVEC-BASE-05 | BASE 写非 4 字节对齐地址 | 写 `stvec = 0x40000003`（低 2 bit = 0b11, MODE 字段），回读 | 回读 BASE 部分 == `0x40000000`、MODE 字段回读为合法值（0 或 1） |
| STVEC-BASE-06 | BASE 多次改写不影响 MODE | 固定 MODE=0，依次写 BASE=0x1000、0x2000、0x4000、0x8000，每次回读 MODE | 每次 `MODE == 0` 且 BASE 与写入值一致 |
| STVEC-BASE-07 | BASE 高位扫描 | 依次写入 `1ULL << k | 0x4`（k 从 12 到 SXLEN-1，只取 4 字节对齐位），每次回读 | 每次回读 BASE == 写入值（Sstvecd 要求"any valid"） |

> [!IMPORTANT]
> **STVEC-BASE-05 的低 2 bit 语义说明**：写入 `0x40000003` 时，[1:0]=0b11 实际落在 MODE 字段（不是 BASE 的 bit[1:0]）。规范要求 BASE 是 [XLEN-1:2]，所以这条用例**实际验证的是**：BASE 字段保留 = `0x40000000`，MODE 字段 = 0b11。MODE=3 是保留值，按 STVEC-MODE-04 的规则回读应为合法值（0 或 1）。因此 BASE-05 的精确断言为：`(readback & ~0x3) == 0x40000000` 且 `(readback & 0x3) ∈ {0, 1}`。

> [!NOTE]
> **STVEC-BASE-04 的高位选择**：bit 39 对应 Sv39 的虚地址上界附近，是探测高位 WARL 截断的典型选择。若实现把 BASE 限制在 [38:2]（如某些 RV64 实现可能默认按 Sv39 上界截断），该用例会失败。Sstvecd 规范要求 BASE 能保持"any valid"4 字节对齐地址，因此该限制应被 Sstvecd 实现解除。

> [!NOTE]
> **STVEC-BASE-07 的扫描范围**：按"平台内存基址之上、不与 trap entry 冲突"的原则选取若干 k 值（如 k=12, 16, 20, 24, 28, 32, 36, 38），不必穷举到 63 位。若 SXLEN=64 但虚地址有效位仅 39/48/57，写入超过有效位的高位地址行为由实现定义，本用例只覆盖到 bit 38（Sv39 范围内）。

---

### Group 3：`MODE=Direct` 下 trap 跳转到 BASE

**规范依据**：
- `supervisor.adoc` 中 `stvec` MODE 编码：MODE=Direct 时，所有 trap（同步异常 + 异步中断）都把 `pc` 设为 BASE
- Sstvecd 强约束 Direct 必然可用（`norm:sstvecd_stvec_mode_direct`），因此 Direct 行为正确性是 Sstvecd 的隐含承诺

**测试职责**：验证当 `stvec.MODE=Direct` 时，同步异常与异步中断都跳转到 BASE（而不是 BASE+4×cause）；验证多次 trap 后 BASE 仍保持原值。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| STVEC-DIR-01 | 同步异常：S-mode ecall | 在 S-mode 执行 `ecall`（cause=9），`stvec` 设为自定义 4 字节对齐 entry，BASE=entry 地址 | 命中 PC == BASE（entry 起点）；`scause == 9` |
| STVEC-DIR-02 | 同步异常：illegal instruction | S-mode 执行未知指令（cause=2），同上 entry | 命中 PC == BASE；`scause == 2` |
| STVEC-DIR-03 | 同步异常：load page-fault | 启用 Sv39 + 不映射的 va，S-mode load 触发 cause=13 | 命中 PC == BASE；`scause == 13` |
| STVEC-INT-01 | 异步中断：SSIP | M-mode 设置 `mideleg.SSIE=1`，S-mode 自激发 SSIP（cause=1，中断位置位） | 命中 PC == BASE（Direct 不加 4×1=4） |
| STVEC-MULTI-01 | 多次 trap BASE 不变 | 连续触发 5 次 ecall，每次 trap 后 M-mode 回读 `stvec` 检查 BASE 与 MODE | 5 次后 `BASE == 原值` 且 `MODE == 0` |

---

## 运行环境

- 所有测试运行于 RV64 + Sv39（Group 3）/ M-mode 直接 CSR 操作（Group 1、Group 2）
- 单核环境，无需 IPI
- 任一平台违反 SPEC 时用例保持 FAIL，实现缺陷记录至 `bugs/` 目录

### 失败定位指引

| 现象 | 可能原因 |
|------|----------|
| STVEC-MODE-01 失败 | 平台连基本的 stvec 都未实现，或 CSR 写入路径异常 |
| STVEC-BASE-04 / 07 失败 | Sstvecd 未真正启用，BASE 高位被 WARL 截断 |
| STVEC-DIR-01/02/03 失败（命中地址 ≠ BASE） | trap 路径异常，或框架 S-mode 执行辅助内部覆盖了 `stvec` |
| STVEC-INT-01 失败（命中地址 = BASE+4） | 实现错误地按 Vectored 处理中断；说明 MODE=0 写入未生效 |
| STVEC-INT-01 失败（无命中） | `mideleg` 未委托 SSIP，或 `sstatus.SIE` 未使能 |

---

## 附录 A：规范点覆盖矩阵

| Norm ID | 覆盖的测试 ID | 覆盖状态 | 备注 |
|---------|--------------|----------|------|
| `norm:sstvecd_stvec_mode_direct` | STVEC-MODE-01、STVEC-MODE-02、STVEC-DIR-01、STVEC-DIR-02、STVEC-DIR-03、STVEC-INT-01、STVEC-MULTI-01 | 已覆盖 | Direct 模式可写入并保持 |
| `norm:sstvecd_stvec_base_aligned_address` | STVEC-BASE-01 ~ STVEC-BASE-07、STVEC-DIR-01 ~ STVEC-DIR-03、STVEC-INT-01、STVEC-MULTI-01 | 已覆盖 | BASE 能保持任意 4 字节对齐地址 |
| `norm:stvec_op` | STVEC-MODE-01 ~ STVEC-MODE-04、STVEC-BASE-01 ~ STVEC-BASE-07 | 已覆盖 | BASE 字段行为与对齐约束 |
| `norm:stvec_sz_base` | STVEC-BASE-05、STVEC-BASE-06、STVEC-BASE-07 | 已覆盖 | CSR 仅存 [XLEN-1:2]，低 2 bit 强制为 0 |
