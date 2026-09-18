# Ssdbltrp 扩展测试计划

## 概述

本测试计划覆盖 RISC-V Ssdbltrp（Supervisor-level Double Trap）扩展的所有核心功能点。该扩展通过在 `sstatus` 中引入 SDT（S-mode-disable-trap）位和在 `menvcfg`/`henvcfg` 中引入 DTE（Double-Trap Enable）位来解决低于 M 特权级的双陷阱问题。

### 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/ssdbltrp.adoc` — Ssdbltrp 扩展：DTE/SDT 字段引入、M-mode 关键错误处理路径
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — sstatus.SDT 字段操作、SDT/SIE 互斥、trap 交付、SRET 清除
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — menvcfg.DTE 使能控制、medeleg[16] 只读零、MRET/SRET/MNRET 对 SDT/MDT 的跨模式清除、mtval2 依赖

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内 src/priv/ssdbltrp.adoc、src/priv/supervisor.adoc、src/priv/machine.adoc）

本方案覆盖的 SPEC 章节要点：

- Ssdbltrp Extension（`ssdbltrp.adoc`：扩展概述、DTE/SDT 字段引入）
- Double Trap Control in `sstatus` Register（`supervisor.adoc`：SDT 字段操作、SDT/SIE 互斥、trap 交付、SRET 清除）
- `menvcfg`.DTE（`machine.adoc`：DTE 使能控制、DTE=0 时全局禁用）
- `medeleg`[16] 只读零（`machine.adoc`：double trap 不可委托）
- MRET/SRET/MNRET 清除 SDT（`machine.adoc`：跨模式返回时 SDT 清除行为）

### 由其他测试计划覆盖

- M-mode double trap（`mstatus`.MDT 字段行为） → `Smdbltrp_test_plan.md`
- Hypervisor 基本功能 → `Hypervisor_CSR_test_plan.md`、`Hypervisor_Interrupts_test_plan.md`、`Hypervisor_Exceptions_test_plan.md`
- Smrnmi 交互 → `smrnmi_test_plan.md`
- **Hypervisor × Ssdbltrp 交叉测试**（`henvcfg`.DTE、`vsstatus`.SDT、SRET 清除 `vsstatus`.SDT、MRET/SRET/MNRET 在 Hypervisor 场景下的跨模式清除） → `Hypervisor_cross_test_plan.md` Group 12

---

## 覆盖的规范点

本章节列出本文档 Groups 1-9 所有测试组中引用的规范点（norm ID），已去重并按字母顺序排列。

> **注意**：Hypervisor 相关规范点（`norm:henvcfg_DTE`、`norm:henvcfg_dte_op`、`norm:vsstatus_SDT`、`norm:vsstatus_sdt_op`、`norm:vsstatus_sdt_clr_mret_sret`、`norm:vsstatus_sdt_clr_mnret`、`norm:sret_dt`、`norm:HS_mode_invoke_error`）已迁移至 `Hypervisor_cross_test_plan.md` Group 12。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:M_mode_invoke_error` | It also allows M-mode to invoke a critical error handler in the OS/Hypervisor on a double trap in S/HS-mode. | 使 M-mode 能够在 S/HS-mode 发生双陷阱时调用 OS/Hypervisor 中的关键错误处理器。 |
| `norm:menvcfg_DTE` | The Ssdbltrp extension adds the `menvcfg`.DTE field. | Ssdbltrp 扩展添加 `menvcfg`.DTE 字段。 |
| `norm:menvcfg_dte_op` | The Ssdbltrp extension adds the double-trap-enable (DTE) field in `menvcfg`. When `menvcfg`.DTE is zero, the implementation behaves as though Ssdbltrp is not implemented. When Ssdbltrp is not implemented `sstatus`.SDT, `vsstatus`.SDT, and `henvcfg`.DTE bits are read-only zero. | Ssdbltrp 扩展向 `menvcfg` 添加 DTE 字段。`menvcfg`.DTE=0 时行为如同未实现 Ssdbltrp；此时 `sstatus`.SDT、`vsstatus`.SDT 和 `henvcfg`.DTE 均为只读零。 |
| `norm:medeleg_16_no_rd0` | The `medeleg`[16] is read-only zero as double trap is not delegatable. | `medeleg`[16] 为只读零，因为 double trap 异常不可委托。 |
| `norm:mstatus_mdt_clr_mret_sret` | If the Ssdbltrp extension is implemented, and the new privilege mode is U, VS, or VU, then `mstatus`.MDT is also set to 0 by MRET or SRET in M-mode. | 若实现 Ssdbltrp，MRET 或 M-mode 中的 SRET 新模式为 U、VS 或 VU 时，`mstatus`.MDT 也被清零。 |
| `norm:mtval2_Ssdbltrap` | The Ssdbltrp extension requires the implementation of the `mtval2` CSR. | Ssdbltrp 扩展要求实现 `mtval2` CSR。 |
| `norm:mtval2_val` | The `mtval2` register is an SXLEN-bit read-write register. | `mtval2` 是 SXLEN 位读写寄存器，其值在 double-trap 交付时保存原始 trap 的 mcause 值。 |
| `norm:sstatus_SDT` | The Ssdbltrp extension adds the `sstatus`.SDT field. | Ssdbltrp 扩展添加 `sstatus`.SDT 字段。 |
| `norm:sstatus_sdt` | The S-mode-disable-trap (SDT) bit is a WARL field introduced by the Ssdbltrp extension to address double trap at privilege modes lower than M. | SDT 位是 Ssdbltrp 扩展引入的 WARL 字段，用于处理低于 M 特权级的双陷阱。 |
| `norm:sstatus_sdt_clr_mnret` | If the Ssdbltrp extension is also implemented, and the new privilege mode is U, VS, or VU, then `sstatus`.SDT is also set to 0 (by MNRET). | 若同时实现 Ssdbltrp，MNRET 新模式为 U、VS 或 VU 时，`sstatus`.SDT 也被清零。 |
| `norm:sstatus_sdt_clr_mret_sret` | If the Ssdbltrp extension is also implemented, and the new privilege mode is U, VS, or VU, then `sstatus`.SDT is also set to 0 (by MRET or SRET in M-mode). | 若同时实现 Ssdbltrp，MRET 或 M-mode 中的 SRET 新模式为 U、VS 或 VU 时，`sstatus`.SDT 也被清零。 |
| `norm:sstatus_sdt_sret` | An SRET instruction sets the SDT bit to 0. | SRET 指令将 SDT 位清零。 |
| `norm:sstatus_sdt_sstatus_sie_overwrite` | When the SDT bit is set to 1 by an explicit CSR write, the SIE bit is cleared to 0. This clearing occurs regardless of the value written, if any, to the SIE bit by the same write. The SIE bit can only be set to 1 by an explicit CSR write if the SDT bit is being set to 0 by the same write or is already 0. | 通过显式 CSR 写入将 SDT 设为 1 时，SIE 被清零。无论同次写入中 SIE 的值如何，此清零均发生。仅当 SDT 在同次写入中被设为 0 或已为 0 时，SIE 才能被显式 CSR 写入设为 1。 |
| `norm:sstatus_sdt_trap` | When a trap is to be taken into S-mode, if the SDT bit is currently 0, it is then set to 1, and the trap is delivered as expected. However, if SDT is already set to 1, then this is an unexpected trap. In the event of an unexpected trap, a double-trap exception trap is delivered into M-mode. The `mtval2` register is then set to what would be otherwise written into the `mcause` register by the unexpected trap. The `mcause` register is set to 16, the double-trap exception code. | trap 交付到 S-mode 时，若 SDT=0，则设 SDT=1 并正常交付。若 SDT=1，则为 unexpected trap，产生 double-trap 异常交付到 M-mode：`mtval2` 设为 unexpected trap 原本写入 `mcause` 的值；`mcause` 设为 16（double-trap 异常码）。 |

---

## Group 1. sstatus.SDT 字段行为

**规范依据**：
- `norm:sstatus_sdt`：SDT 是 WARL 字段
- `norm:sstatus_sdt_sstatus_sie_overwrite`：SDT=1 时 SIE 被清零；SIE=1 仅当 SDT=0 或同次写入正在清零 SDT
- `norm:sstatus_sdt_trap`：trap 交付到 S-mode 时 SDT=0 则设 SDT=1 正常交付

**测试职责**：验证 `sstatus`.SDT 字段的 WARL 读写属性及 SDT 与 SIE 的互斥约束。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SDT-01 | sstatus.SDT WARL 读写 | M-mode 写 sstatus.SDT=1 后读回验证；再写 SDT=0 读回验证 | 若实现 Ssdbltrp（menvcfg.DTE=1），SDT 可写且读回一致；若未实现，SDT 为只读零 |
| SDT-02 | SDT=1 写时自动清零 SIE | 先设 sstatus.SIE=1，然后写 sstatus.SDT=1（同时写 SIE=1） | SIE 被强制清零，无论同次写入中 SIE 的值 |
| SDT-03 | SDT=1 时无法设置 SIE=1 | 设 sstatus.SDT=1，然后写 sstatus.SIE=1（SDT 保持 1） | SIE 读回为 0（SDT=1 时 SIE 无法被设为 1） |
| SDT-04 | SDT 同次写入清零时允许 SIE=1 | 同时写 sstatus.SDT=0 和 SIE=1 | SIE=1 且 SDT=0（同次写入清零 SDT 允许设置 SIE） |
| SDT-05 | SDT=0 时允许 SIE=1 | 设 sstatus.SDT=0，然后写 sstatus.SIE=1 | SIE=1（SDT=0 时 SIE 可正常设置） |
| SDT-06 | trap 到 S-mode 时 SDT 自动设为 1 | 设 sstatus.SDT=0，从 U-mode 触发 ecall trap 到 S-mode | S-mode trap handler 中读 sstatus.SDT=1 |
| SDT-07 | trap 正常交付时 SDT 从 0 变 1 | 设 SDT=0、SIE=1，触发 S-mode timer interrupt | trap 正常交付，SDT 被设为 1，SIE 被清零 |

---

## Group 2. S-mode Double Trap 交付

**规范依据**：
- `norm:sstatus_sdt_trap`：SDT=1 时为 unexpected trap，double-trap 异常交付 M-mode（mcause=16, mtval2=原始 cause）
- `norm:M_mode_invoke_error`：M-mode 可在 S/HS-mode 双陷阱时调用关键错误处理器
- `norm:mstatus_mdt_clr_mret_sret`：MRET 新模式为 U 时清 mstatus.MDT（M-mode double-trap handler 返回时使用）

**测试职责**：验证 SDT=1 时 double-trap 异常的交付机制，包括 mcause=16 和 mtval2 中保存的原始 cause。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| DT-01 | SDT=1 时 ecall 触发 double-trap | 设 sstatus.SDT=1，S-mode 执行 ECALL | M-mode trap handler 收到 mcause=16（double-trap exception） |
| DT-02 | double-trap 时 mtval2 保存原始 cause | 设 SDT=1，S-mode ECALL 触发 double-trap | mtval2 = 9（ecall-from-S 的 cause 值），mcause = 16 |
| DT-03 | double-trap 时 mepc 保存正确 PC | 设 SDT=1，S-mode ECALL 触发 double-trap | mepc = ECALL 指令地址 |
| DT-04 | double-trap 时 M-mode CSR 写入正确 | 设 SDT=1，S-mode 触发 trap 导致 double-trap | mstatus.MPP=S-mode, mstatus.MPIE=旧 MIE, mstatus.MIE=0 |
| DT-05 | SDT=1 时非法指令触发 double-trap | 设 SDT=1，S-mode 执行非法指令 | M-mode trap handler 收到 mcause=16, mtval2=2（illegal-instruction cause） |
| DT-06 | SDT=1 时中断触发 double-trap | 设 SDT=1、SIE=0（SDT=1 强制），在 S-mode 触发被委托到 S-mode 的中断 | 若中断被委托到 S-mode 且 SDT=1，则 double-trap 到 M-mode |
| DT-07 | SDT=0 时 trap 正常交付（无 double-trap） | 设 SDT=0，S-mode ECALL | trap 正常交付到 S-mode（或 M-mode，取决于 medeleg），mcause≠16 |
| DT-08 | double-trap 时 mstatus.MDT 被设置 | 设 SDT=1，S-mode ECALL 触发 double-trap 到 M-mode | mstatus.MDT=1（M-mode 接收 trap 时 MDT 从 0 设为 1） |

---

## Group 3. SRET 对 SDT 的清除

**规范依据**：
- `norm:sstatus_sdt_sret`：SRET 指令将 SDT 位清零

**测试职责**：验证 SRET 指令在非 Hypervisor 场景下对 sstatus.SDT 的清除行为。

> **注意**：Hypervisor 场景下的 SRET 对 `vsstatus.SDT` 的清除行为（原 SRET-03~06）已迁移至 `Hypervisor_cross_test_plan.md` Group 12.3。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SRET-01 | S-mode SRET 清除 sstatus.SDT | S-mode 设 sstatus.SDT=1，执行 SRET 到 U-mode | sstatus.SDT=0 |
| SRET-02 | M-mode SRET 清除 sstatus.SDT | M-mode 设 sstatus.SDT=1，执行 SRET 到 S-mode | sstatus.SDT=0 |

---

## Group 4. menvcfg.DTE 控制

**规范依据**：
- `norm:menvcfg_DTE`：Ssdbltrp 添加 menvcfg.DTE 字段
- `norm:menvcfg_dte_op`：DTE=0 时行为如同 Ssdbltrp 未实现；sstatus.SDT、vsstatus.SDT、henvcfg.DTE 只读零

**测试职责**：验证 `menvcfg`.DTE 字段的读写及对 Ssdbltrp 功能的全局使能/禁用控制（非 Hypervisor 部分）。

> **注意**：Hypervisor 相关的 `menvcfg.DTE` 对 `vsstatus.SDT` 和 `henvcfg.DTE` 的控制测试（原 DTE-03~04）已迁移至 `Hypervisor_cross_test_plan.md` Group 12.4。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| DTE-01 | menvcfg.DTE 读写 | M-mode 写 menvcfg.DTE=1 后读回验证；再写 DTE=0 读回 | 若实现 Ssdbltrp，DTE 可写且读回一致；若未实现，DTE 为只读零 |
| DTE-02 | DTE=0 时 sstatus.SDT 只读零 | menvcfg.DTE=0，尝试写 sstatus.SDT=1 | sstatus.SDT 读回 0（只读零） |
| DTE-05 | DTE=0 时 SDT/SIE 互斥不生效 | menvcfg.DTE=0，写 sstatus.SIE=1 | SIE=1 可正常设置（SDT 不存在，无互斥约束） |
| DTE-06 | DTE=1 后 SDT/SIE 互斥生效 | menvcfg.DTE=1，写 sstatus.SDT=1 再写 SIE=1 | SIE 被强制清零（SDT 功能启用后互斥生效） |
| DTE-07 | DTE 动态切换 | 先设 DTE=1 验证 SDT 可写；再设 DTE=0 验证 SDT 只读零 | DTE=1 时 SDT 可写；DTE=0 后 SDT 只读零 |

---

## Group 5. henvcfg.DTE 控制（Hypervisor）

> **本组已迁移**：原 Group 5（HDTE-01~06）的所有测试用例依赖 Hypervisor 扩展，已迁移至 `Hypervisor_cross_test_plan.md` Group 12.1（HCROSS-SSDBLTRP-01~06）。

---

## Group 6. vsstatus.SDT（VS-mode）

> **本组已迁移**：原 Group 6（VSDT-01~06）的所有测试用例依赖 Hypervisor 扩展，已迁移至 `Hypervisor_cross_test_plan.md` Group 12.2（HCROSS-SSDBLTRP-07~12）。

---

## Group 7. MRET/SRET/MNRET 对 SDT 的跨模式清除

**规范依据**：
- `norm:sstatus_sdt_clr_mret_sret`：MRET 或 M-mode SRET 新模式为 U/VS/VU 时清 sstatus.SDT
- `norm:sstatus_sdt_clr_mnret`：MNRET 新模式为 U/VS/VU 时清 sstatus.SDT

**测试职责**：验证跨模式返回指令对 sstatus.SDT 的清除行为（非 Hypervisor 部分）。

> **注意**：Hypervisor 场景下的跨模式清除测试（原 XRET-03~06, 08, 10）已迁移至 `Hypervisor_cross_test_plan.md` Group 12.5。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| XRET-01 | MRET 到 U-mode 清除 sstatus.SDT | 设 sstatus.SDT=1, mstatus.MPP=0(U), MPV=0, 执行 MRET | sstatus.SDT=0 |
| XRET-02 | MRET 到 HS-mode 不清 sstatus.SDT | 设 sstatus.SDT=1, mstatus.MPP=1(S), MPV=0, 执行 MRET | sstatus.SDT 保持 1 |
| XRET-07 | M-mode SRET 到 U-mode 清除 sstatus.SDT | M-mode 设 SDT=1, mstatus.MPP=0, hstatus.SPV=0, 执行 SRET | sstatus.SDT=0 |
| XRET-09 | MNRET 到 U-mode 清除 sstatus.SDT | 设 SDT=1, mnstatus.MNPP=0(U), MNPV=0, 执行 MNRET（需 Smrnmi） | sstatus.SDT=0 |
| XRET-11 | MNRET 到 M-mode 不清 sstatus.SDT | 设 SDT=1, MNPP=3(M), 执行 MNRET | sstatus.SDT 保持 1 |

---

## Group 8. medeleg[16] 只读零

**规范依据**：
- `norm:medeleg_16_no_rd0`：medeleg[16] 为只读零，double trap 不可委托

**测试职责**：验证 double-trap 异常（cause=16）不可通过 medeleg 委托。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| DELEG-01 | medeleg[16] 只读零 | M-mode 尝试写 medeleg bit 16 = 1 | medeleg[16] 读回 0（只读零） |
| DELEG-02 | double-trap 始终交付到 M-mode | 设 medeleg 全 1（包括尝试 bit 16），触发 double-trap | double-trap 始终交付到 M-mode（不会被委托到 S-mode） |

---

## Group 9. mtval2 CSR 依赖

**规范依据**：
- `norm:mtval2_Ssdbltrap`：Ssdbltrp 扩展要求 mtval2 CSR 的实现
- `norm:mtval2_val`：mtval2 是 SXLEN 位读写寄存器
- `norm:sstatus_sdt_trap`：double-trap 交付时 mtval2 写入原始 trap 的 cause

**测试职责**：验证实现 Ssdbltrp 时 mtval2 CSR 的存在性，以及 double-trap / S-mode 交付路径对 mtval2 的写入行为。

**实现要点**：本组是 Group 2 中 double-trap 交付行为（DT-02 的 mtval2 部分）的实际落地用例。探针在 S-mode 执行被委托的非法指令触发；mtval2 的期望值取自 M-mode handler 入口捕获，因为后续任何 M-mode trap 入口都会重写 mtval2。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MTVAL2-01 | mtval2 CSR 存在性与读写 | M-mode 读 mtval2（缺失即 illegal-instruction，显性失败）；WARL 写回稳定性；保持零 | Ssdbltrp 实现时 mtval2 必须存在、WARL 稳定、可保持 0 |
| MTVAL2-02 | double-trap 时 mtval2 写入原始 cause | 阶段 1（canary）：SDT=0 探测，合规实现不得升级；阶段 2：SDT=1 触发 double-trap 到 M-mode，检查 mcause=16、mtval2=原始 cause（illegal=2）、入口 mstatus.MPP=S | SDT=0 无升级；SDT=1 时 mcause=16、mtval2=2、MPP=S |
| MTVAL2-03 | S-mode 交付不修改 mtval2 | 预置 mtval2 哨兵值，SDT=0 时委托 trap 交付到 S-mode（SDT 0→1），返回 M-mode 后验证 | 无 double-trap 升级记录；S-mode 侧记录 cause=2 且 status 快照 SDT=1；mtval2 为哨兵值或 0（非 2，证明无 M-mode 升级参与） |

**测试环境要求**：
- double-trap 探针要求环境严格保持 `mstatus.MDT=0`（无 Smrnmi 时 MDT=1 下的 M-mode 异常即不可恢复 critical error）：套件构建时启用 Smdbltrp 依赖支持，测试结束与状态复位阶段均清除 MDT，探针执行前显式清 SDT/MDT。

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1 (sstatus.SDT 字段) | SDT-01~07 | SDT 是 Ssdbltrp 的核心机制，SDT/SIE 互斥是安全保证 |
| P0（必须） | Group 2 (Double Trap 交付) | DT-01~08 | Double-trap 异常交付是该扩展的核心功能 |
| P0（必须） | Group 4 (menvcfg.DTE) | DTE-01~02、DTE-05~07 | DTE 是全局使能控制，决定功能是否生效 |
| P1（重要） | Group 3 (SRET 清除) | SRET-01~02 | SRET 清除 SDT 是 trap 返回的关键路径 |
| P1（重要） | Group 7 (跨模式清除) | XRET-01~02、XRET-07、XRET-09、XRET-11 | MRET/SRET/MNRET 对 SDT 的跨模式影响 |
| P2（建议） | Group 8 (medeleg[16]) | DELEG-01~02 | 验证不可委托约束 |
| P2（建议） | Group 9 (mtval2 依赖) | MTVAL2-01~03 | CSR 依赖关系验证 |

> **注意**：Hypervisor 相关测试的优先级见 `Hypervisor_cross_test_plan.md`。

---

## 关键注意事项

1. **扩展检测**：所有测试必须在运行时检测 Ssdbltrp 的可用性（通过 `menvcfg.DTE` 可写性），不可用时 TEST_SKIP。

2. **Smdbltrp 依赖**：Double-trap 测试需要在 M-mode 处理 double-trap 异常。如果 Smdbltrp 未实现，M-mode 自身没有 MDT 保护，double-trap 到 M-mode 将导致 critical-error state（系统挂死），因此 double-trap 交付测试需同时检测 Smdbltrp。

3. **MDT 清除**：在 M-mode 测试 double-trap 前，必须先清除 `mstatus.MDT`，否则 M-mode trap 本身也会成为 unexpected trap。

4. **Smrnmi 依赖**：Group 7 中 XRET-09~11（MNRET 测试）需要 Smrnmi 扩展。通过检测 `mnstatus` CSR 的存在性判断，不存在时 TEST_SKIP。

5. **SIE 设置前提**：设置 `sstatus.SIE=1` 需要先确保 `sstatus.SDT=0`。在测试中应注意操作顺序。

6. **medeleg 配置**：Double-trap 测试（Group 2）中需要正确配置 `medeleg` 以控制原始 trap 的委托路径。double-trap 发生在"原始 trap 尝试交付到 S-mode 但 SDT=1"的场景，因此 medeleg 对应位需要设为 1（委托到 S-mode）。

7. **Hypervisor 测试**：需要 Hypervisor 扩展的测试（`henvcfg.DTE`、`vsstatus.SDT` 等）已迁移至 `Hypervisor_cross_test_plan.md`。

任一平台违反 SPEC 时用例保持 FAIL，实现缺陷记录至 `bugs/` 目录。

---

## 附录 A：规范点覆盖矩阵

| Norm ID | 覆盖的测试 ID | 覆盖状态 | 备注 |
|---------|--------------|----------|------|
| `norm:sstatus_SDT` | SDT-01、SDT-06、SDT-07 | 已覆盖 | sstatus.SDT 字段引入 |
| `norm:sstatus_sdt` | SDT-01~05 | 已覆盖 | SDT WARL 属性 |
| `norm:sstatus_sdt_sstatus_sie_overwrite` | SDT-02、SDT-03、SDT-04、SDT-05 | 已覆盖 | SDT/SIE 互斥 |
| `norm:sstatus_sdt_trap` | SDT-06、SDT-07、DT-01~08、MTVAL2-02、MTVAL2-03 | 已覆盖 | trap 交付与 double-trap 升级 |
| `norm:sstatus_sdt_sret` | SRET-01、SRET-02 | 已覆盖 | SRET 清零 SDT |
| `norm:sstatus_sdt_clr_mret_sret` | XRET-01、XRET-02、XRET-07 | 已覆盖 | MRET/M-mode SRET 跨模式清零 |
| `norm:sstatus_sdt_clr_mnret` | XRET-09、XRET-11 | 已覆盖 | MNRET 跨模式清零 |
| `norm:mstatus_mdt_clr_mret_sret` | DT-04、DT-08、XRET-01 | 已覆盖 | M-mode 侧 MDT 清除路径 |
| `norm:M_mode_invoke_error` | DT-01~08 | 已覆盖 | M-mode 关键错误处理入口 |
| `norm:menvcfg_DTE` | DTE-01、DTE-07 | 已覆盖 | menvcfg.DTE 字段引入 |
| `norm:menvcfg_dte_op` | DTE-01、DTE-02、DTE-05、DTE-06、DTE-07 | 已覆盖 | DTE 全局使能/禁用控制 |
| `norm:medeleg_16_no_rd0` | DELEG-01、DELEG-02 | 已覆盖 | medeleg[16] 只读零 |
| `norm:mtval2_Ssdbltrap` | MTVAL2-01、MTVAL2-02 | 已覆盖 | mtval2 CSR 依赖 |
| `norm:mtval2_val` | MTVAL2-01、MTVAL2-02、MTVAL2-03 | 已覆盖 | mtval2 读写与写入语义 |

**未覆盖规范点说明**：
- Hypervisor 相关规范点（`norm:henvcfg_DTE`、`norm:henvcfg_dte_op`、`norm:vsstatus_SDT`、`norm:vsstatus_sdt_op`、`norm:vsstatus_sdt_clr_mret_sret`、`norm:vsstatus_sdt_clr_mnret`、`norm:sret_dt`、`norm:HS_mode_invoke_error`）：迁移至 `Hypervisor_cross_test_plan.md` Group 12，本方案不重复覆盖。

---

## 参考

- `ssdbltrp.adoc` — Ssdbltrp Double Trap Extension
- `supervisor.adoc` — Double Trap Control in sstatus Register
- `machine.adoc` — menvcfg.DTE、medeleg[16]、MRET/SRET SDT 清除
- `smrnmi.adoc` — Smrnmi Extension（MNRET 交互）
- `DOCS/testplan/Smdbltrp_test_plan.md` — Smdbltrp 扩展测试计划（Machine-level Double Trap）
- `DOCS/testplan/Hypervisor_CSR_test_plan.md` — Hypervisor CSR 子集测试计划
- `DOCS/testplan/Hypervisor_Interrupts_test_plan.md` — Hypervisor 中断子集测试计划
- `DOCS/testplan/Hypervisor_Exceptions_test_plan.md` — Hypervisor 异常与 trap 子集测试计划
- `DOCS/testplan/Hypervisor_cross_test_plan.md` — Hypervisor 与其他扩展交叉测试计划（含 Hypervisor × Ssdbltrp，Group 12）
