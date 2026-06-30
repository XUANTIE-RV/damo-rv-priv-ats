# Smdbltrp 扩展测试计划

## 概述

本测试计划覆盖 RISC-V Smdbltrp（Machine-level Double Trap）扩展的所有核心功能点。该扩展通过在 `mstatus` 中引入 MDT（M-mode-disable-trap）位来解决 M-mode 的双陷阱问题。当 Smrnmi 扩展同时实现时，M-mode 双陷阱将触发 RNMI handler；否则 hart 进入 critical-error state。

本测试计划依据 `SPEC/smdbltrp.adoc`、`SPEC/machine.adoc`（machine-double-trap 章节：MDT 字段、trap 交付、critical-error、MRET/SRET/MNRET 清除行为）和 `SPEC/smrnmi.adoc`（RNMI handler 交互、mncause 写入）中的规范点（norm 标记）编写。

### 本文档覆盖的 SPEC 章节
- Smdbltrp Extension（`smdbltrp.adoc`：扩展概述、Smrnmi 交互、critical-error state、Debug Mode）
- Double Trap Control in `mstatus` Register（`machine.adoc`：MDT 字段操作、MDT/MIE 互斥、trap 交付、unexpected trap 处理、MRET/SRET 清除 MDT）
- MNRET 清除 MDT（`machine.adoc`：MNRET 新模式非 M 时清除 MDT）
- `medeleg`[16] 只读零（`machine.adoc`：double trap 不可委托）
- RNMI `mncause` double-trap 编码（`smrnmi.adoc`：double-trap 到 RNMI handler 时 mncause 写入）

### 由其他测试计划覆盖
- S-mode double trap（`sstatus`.SDT 字段行为） → `Ssdbltrp_test_plan.md`
- Hypervisor 相关 double trap（`henvcfg`.DTE、`vsstatus`.SDT） → `Ssdbltrp_test_plan.md`
- Smrnmi 基本功能 → `smrnmi_test_plan.md`

---

## 覆盖的规范点

本章节列出本文档 Groups 1-7 所有测试组中引用的规范点（norm ID），已去重并按字母顺序排列。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:critical_error` | The actions performed by the platform when a hart asserts a critical-error signal are platform-specific. The range of possible actions include restarting the affected hart or restarting the entire platform, among others. | 当 hart 发出 critical-error 信号时，平台执行的操作是平台特定的。可能的操作包括重启受影响的 hart 或重启整个平台等。 |
| `norm:medeleg_16_no_rd0` | The `medeleg`[16] is read-only zero as double trap is not delegatable. | `medeleg`[16] 为只读零，因为 double trap 异常不可委托。 |
| `norm:mncause_doubletrap` | If the reason is an exception within M-mode that results in a double trap as specified in the Smdbltrp extension, bit MXLEN-1 is set to 0 and the least-significant bits are set to the cause code corresponding to the exception that precipitated the double trap. | 若原因是 M-mode 内导致 Smdbltrp 规范所述双陷阱的异常，则 bit MXLEN-1 设为 0，最低有效位设为引发双陷阱的异常对应的 cause 码。 |
| `norm:mstatus_mdt_clr_mnret` | When the Smdbltrp extension is implemented, the MNRET instruction, provided by the Smrnmi extension, sets the MDT bit to 0 if the new privilege mode is not M. | 当实现 Smdbltrp 时，Smrnmi 扩展提供的 MNRET 指令在新模式非 M 时将 MDT 清零。 |
| `norm:mstatus_mdt_clr_mret_sret` | When the Smdbltrp extension is implemented, executing an MRET instruction, or executing an SRET instruction while the current privilege mode is M, the MDT bit is set to 0. | 当实现 Smdbltrp 时，执行 MRET 指令或在 M-mode 下执行 SRET 指令，MDT 位被清零。 |
| `norm:mstatus_mdt_not_set_rnmi` | A trap caused by an RNMI does not set the MDT bit. | RNMI 导致的 trap 不设置 MDT 位。 |
| `norm:mstatus_mdt_rst` | Upon reset, the MDT field is set to 1. | 复位时 MDT 字段设为 1。 |
| `norm:mstatus_mdt_sz_warl` | The M-mode-disable-trap (MDT) bit is a WARL field introduced by the Smdbltrp extension. | MDT 位是 Smdbltrp 扩展引入的 WARL 字段。 |
| `norm:mstatus_mie_clr_by_mdt` | When the MDT bit is set to 1 by an explicit CSR write, the MIE (Machine Interrupt Enable) bit is cleared to 0. | 通过显式 CSR 写入将 MDT 设为 1 时，MIE 位被清零。 |
| `norm:mstatus_mie_clr_by_mdt_rv64` | For RV64, this clearing occurs regardless of the value written, if any, to the MIE bit by the same write. | 对于 RV64，无论同次写入中 MIE 的值如何，此清零均发生。 |
| `norm:mstatus_mie_set_mdt_0` | The MIE bit can only be set to 1 by an explicit CSR write if the MDT bit is already 0. | 仅当 MDT 位已为 0 时，MIE 才能被显式 CSR 写入设为 1。 |
| `norm:mstatus_mie_set_mdt_0_rv64` | Or, for RV64, is being set to 0 by the same write. | 或对于 RV64，MDT 在同次写入中正被设为 0。 |
| `norm:Smdbltrp_with_Smrnmi_op` | When the Smrnmi extension is implemented, it enables invocation of the RNMI handler on a double trap in M-mode to handle the critical error. If the Smrnmi extension is not implemented or if a double trap occurs during the RNMI handler's execution, this extension helps transition the hart to a critical error state and enables signaling the critical error to the platform. | 当实现 Smrnmi 时，使 M-mode 双陷阱能调用 RNMI handler 处理关键错误。若未实现 Smrnmi 或 RNMI handler 执行期间发生双陷阱，该扩展帮助 hart 转入 critical-error 状态并发出 critical-error 信号。 |
| `norm:trap_exp` | When a trap is to be taken into M-mode, if the MDT bit is currently 0, it is then set to 1, and the trap is delivered as expected. | trap 交付到 M-mode 时，若 MDT=0，则设 MDT=1 并正常交付 trap。 |
| `norm:trap_unexp_hndl_lead-in` | In the event of an unexpected trap, the handling is as follows. | 发生 unexpected trap 时，处理方式如下。 |
| `norm:trap_unexp_hndl_no_rnmi` | When the Smrnmi extension is not implemented, or if the Smrnmi extension is implemented and `mnstatus`.NMIE is 0, the hart enters a critical-error state without updating any architectural state, including the pc. This state involves ceasing execution, disabling all interrupts (including NMIs), and asserting a critical-error signal to the platform. | 若未实现 Smrnmi，或实现了 Smrnmi 但 `mnstatus`.NMIE=0，hart 进入 critical-error 状态，不更新任何架构状态（包括 pc）。该状态涉及停止执行、禁用所有中断（包括 NMI），并向平台发出 critical-error 信号。 |
| `norm:trap_unexp_hndl_rnmi` | When the Smrnmi extension is implemented and `mnstatus`.NMIE is 1, the hart traps to the RNMI handler. To deliver this trap, the `mnepc` and `mncause` registers are written with the values that the unexpected trap would have written to the `mepc` and `mcause` registers respectively. The privilege mode information fields in the `mnstatus` register are written to indicate M-mode and its NMIE field is set to 0. | 当实现 Smrnmi 且 `mnstatus`.NMIE=1 时，hart 陷入 RNMI handler。`mnepc` 和 `mncause` 分别写入 unexpected trap 原本写入 `mepc` 和 `mcause` 的值。`mnstatus` 的特权模式字段写入 M-mode，NMIE 设为 0。 |
| `norm:trap_unexp_mdt_1` | However, if MDT is already set to 1, then this is an unexpected trap. | 若 MDT 已为 1，则为 unexpected trap。 |
| `norm:trap_unexp_mnstatus_nmie_0` | However, a trap that occurs when executing in M-mode with `mnstatus`.NMIE set to 0 is an unexpected trap. | 在 M-mode 中执行且 `mnstatus`.NMIE=0 时发生的 trap 是 unexpected trap。 |
| `norm:trap_unexp_rnmi` | When the Smrnmi extension is implemented, a trap caused by an RNMI is not considered an unexpected trap irrespective of the state of the MDT bit. | 当实现 Smrnmi 时，RNMI 导致的 trap 无论 MDT 状态如何都不被视为 unexpected trap。 |

---

## Group 1. mstatus.MDT 字段行为

**规范依据**：
- `norm:mstatus_mdt_sz_warl`：MDT 是 WARL 字段
- `norm:mstatus_mdt_rst`：复位时 MDT=1

**测试职责**：验证 `mstatus`.MDT 字段的 WARL 读写属性和复位值。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MDT-01 | mstatus.MDT WARL 读写 | M-mode 写 mstatus.MDT=1 后读回验证；再写 MDT=0 读回 | 若实现 Smdbltrp，MDT 可写且读回一致；若未实现，MDT 为只读零 |
| MDT-02 | 复位后 MDT=1 | 系统复位后（或 hart reset 后），M-mode 入口读 mstatus.MDT | MDT=1（复位后默认禁止 M-mode trap） |
| MDT-03 | MDT 位位置正确 | M-mode 写 MDT 位，确认在 mstatus bit 42（RV64） | MDT 位于 mstatus 的 bit 42（RV64） |

---

## Group 2. MDT/MIE 互斥

**规范依据**：
- `norm:mstatus_mie_clr_by_mdt`：MDT=1 时 MIE 被清零
- `norm:mstatus_mie_clr_by_mdt_rv64`：RV64 下无论 MIE 写何值均被清零
- `norm:mstatus_mie_set_mdt_0`：MIE=1 仅当 MDT 已为 0
- `norm:mstatus_mie_set_mdt_0_rv64`：RV64 下 MIE=1 需 MDT 在同次写入中清零

**测试职责**：验证 MDT 与 MIE 之间的互斥约束。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MMIE-01 | MDT=1 写时自动清零 MIE | 先设 mstatus.MIE=1，然后写 MDT=1（同时写 MIE=1） | MIE 被强制清零，无论同次写入中 MIE 的值 |
| MMIE-02 | MDT=1 时无法设置 MIE=1 | 设 mstatus.MDT=1，然后写 mstatus.MIE=1（MDT 保持 1） | MIE 读回为 0（MDT=1 时 MIE 无法被设为 1） |
| MMIE-03 | MDT 同次写入清零时允许 MIE=1（RV64） | 同时写 mstatus.MDT=0 和 MIE=1 | MIE=1 且 MDT=0（RV64: 同次写入清零 MDT 允许设置 MIE） |
| MMIE-04 | MDT=0 时允许 MIE=1 | 设 mstatus.MDT=0，然后写 mstatus.MIE=1 | MIE=1（MDT=0 时 MIE 可正常设置） |
| MMIE-05 | MDT=1 时 MIE 多次写入均被清零 | 设 MDT=1，连续多次写 MIE=1 | MIE 始终读回 0 |
| MMIE-06 | MDT=0→1 切换清零已有 MIE=1 | 设 MDT=0、MIE=1，然后写 MDT=1 | MIE 从 1 变为 0 |

---

## Group 3. M-mode Trap 交付

**规范依据**：
- `norm:trap_exp`：MDT=0 时设 MDT=1 并正常交付 trap
- `norm:trap_unexp_mdt_1`：MDT=1 时为 unexpected trap

**测试职责**：验证 M-mode trap 交付时 MDT 的自动设置行为，以及 MDT=1 时 unexpected trap 的触发。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MTRAP-01 | MDT=0 时正常 trap 交付并设 MDT=1 | 设 MDT=0，从 S-mode 触发 ecall（未委托）到 M-mode | trap 正常交付，mstatus.MDT=1 |
| MTRAP-02 | MDT=0 时 trap 正常交付中断 | 设 MDT=0、MIE=1，触发 machine timer interrupt 到 M-mode | 中断正常交付，MDT=1 |
| MTRAP-03 | MDT=0 trap 后 MIE 被硬件清零 | 设 MDT=0、MIE=1，触发 trap 到 M-mode | MPIE=旧 MIE(1), MIE=0, MDT=1 |
| MTRAP-04 | MDT=1 时 trap 为 unexpected trap | 设 MDT=1，S-mode 触发 ecall 到 M-mode | unexpected trap 发生（进入 RNMI handler 或 critical-error，取决于 Smrnmi） |
| MTRAP-05 | 连续两次 trap 产生 unexpected trap | 设 MDT=0，第一次 trap 正常交付（MDT→1）；不清 MDT，触发第二次 trap | 第二次 trap 为 unexpected trap |

---

## Group 4. Critical Error State

**规范依据**：
- `norm:trap_unexp_hndl_no_rnmi`：无 Smrnmi 或 NMIE=0 时进入 critical-error state
- `norm:critical_error`：critical-error signal 行为平台特定
- `norm:Smdbltrp_with_Smrnmi_op`：未实现 Smrnmi 或 RNMI handler 执行期间双陷阱时进入 critical-error state

**测试职责**：验证 unexpected trap 在无 Smrnmi 或 NMIE=0 时的 critical-error 行为。

> [!WARNING]
> Critical-error state 会导致 hart 停止执行。以下测试需要平台支持 critical-error recovery（如看门狗重启或外部复位）。若平台不支持恢复，这些测试将导致系统挂死，应标记为平台特定并谨慎执行。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| CERR-01 | 无 Smrnmi 时 unexpected trap 进入 critical-error | Smrnmi 未实现，MDT=1，触发 trap 到 M-mode | hart 进入 critical-error state（停止执行、禁用所有中断） |
| CERR-02 | NMIE=0 时 unexpected trap 进入 critical-error | Smrnmi 已实现，mnstatus.NMIE=0，MDT=1，触发 trap 到 M-mode | hart 进入 critical-error state |
| CERR-03 | critical-error 时不更新架构状态 | 触发 critical-error，复位后检查 M-mode CSR | pc、mepc、mcause 等 CSR 未被 critical-error 修改 |
| CERR-04 | RNMI handler 中双陷阱进入 critical-error | Smrnmi 已实现，NMIE=0（RNMI handler 内部），MDT=1，触发 trap | hart 进入 critical-error state |

---

## Group 5. Smrnmi 交互

**规范依据**：
- `norm:trap_unexp_hndl_rnmi`：Smrnmi + NMIE=1 时 trap 到 RNMI handler（mnepc、mncause 写入，mnstatus.MNPP=M-mode, NMIE=0）
- `norm:trap_unexp_rnmi`：RNMI trap 不是 unexpected trap（无论 MDT 状态）
- `norm:mstatus_mdt_not_set_rnmi`：RNMI trap 不设置 MDT
- `norm:trap_unexp_mnstatus_nmie_0`：mnstatus.NMIE=0 时的 trap 是 unexpected
- `norm:mncause_doubletrap`：double trap 到 RNMI 时 mncause bit MXLEN-1=0, 低位=原始 cause code
- `norm:Smdbltrp_with_Smrnmi_op`：Smrnmi 实现时 M-mode 双陷阱触发 RNMI handler

**测试职责**：验证 Smrnmi 扩展实现时 M-mode double trap 的 RNMI handler 交付行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| RNMI-01 | MDT=1 时 trap 触发 RNMI handler | Smrnmi 已实现，NMIE=1，MDT=1，S-mode ecall 到 M-mode | RNMI handler 被调用（mnstatus.NMIE=0, MNPP=M-mode） |
| RNMI-02 | RNMI handler 中 mnepc 正确 | MDT=1 时 S-mode ecall 触发 RNMI | mnepc = ecall 指令地址（即 unexpected trap 的 mepc 值） |
| RNMI-03 | RNMI handler 中 mncause 正确（double trap） | MDT=1 时 S-mode ecall（cause=9）触发 RNMI | mncause = 0x9（bit MXLEN-1=0，低位=9=ecall-from-S） |
| RNMI-04 | RNMI handler 中 mncause（非法指令 double trap） | MDT=1 时 M-mode 非法指令触发 RNMI | mncause bit MXLEN-1=0，低位=2（illegal-instruction） |
| RNMI-05 | RNMI trap 不设 MDT | 触发 RNMI（非 double-trap），检查 MDT | MDT 不被 RNMI trap 修改 |
| RNMI-06 | RNMI 在 MDT=1 时仍正常交付 | 设 MDT=1，触发外部 RNMI | RNMI 正常交付（不被视为 unexpected trap），MDT 保持 1 |
| RNMI-07 | RNMI handler 中 NMIE=0 时 trap 为 unexpected | RNMI handler 内（NMIE=0），触发异常 | unexpected trap（导致 critical-error，因 NMIE=0） |
| RNMI-08 | mnstatus.MNPP 指示 M-mode | M-mode double trap 触发 RNMI | mnstatus.MNPP = M-mode (3) |
| RNMI-09 | mnstatus.NMIE 被设为 0 | RNMI handler 入口时检查 mnstatus.NMIE | NMIE=0 |

---

## Group 6. MRET/SRET/MNRET 对 MDT 的清除

**规范依据**：
- `norm:mstatus_mdt_clr_mret_sret`：MRET 或 M-mode SRET 将 MDT 清零
- `norm:mstatus_mdt_clr_mnret`：MNRET 新模式非 M 时清 MDT

**测试职责**：验证各 trap 返回指令对 MDT 的清除行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MRET-01 | MRET 清除 MDT | 设 MDT=1, mstatus.MPP=1(S), 执行 MRET | mstatus.MDT=0 |
| MRET-02 | MRET 到 M-mode 清除 MDT | 设 MDT=1, MPP=3(M), 执行 MRET | mstatus.MDT=0（MRET 始终清 MDT） |
| MRET-03 | MRET 到 U-mode 清除 MDT | 设 MDT=1, MPP=0(U), 执行 MRET | mstatus.MDT=0 |
| MRET-04 | M-mode SRET 清除 MDT | 设 MDT=1，在 M-mode 执行 SRET | mstatus.MDT=0 |
| MRET-05 | MNRET 到 S-mode 清除 MDT | 设 MDT=1, mnstatus.MNPP=1(S), 执行 MNRET（需 Smrnmi） | mstatus.MDT=0 |
| MRET-06 | MNRET 到 U-mode 清除 MDT | 设 MDT=1, MNPP=0(U), 执行 MNRET | mstatus.MDT=0 |
| MRET-07 | MNRET 到 M-mode 不清 MDT | 设 MDT=1, MNPP=3(M), 执行 MNRET | mstatus.MDT 保持 1（MNRET 新模式为 M 时不清 MDT） |
| MRET-08 | MRET 清除 MDT 后可正常接收 trap | 设 MDT=1，执行 MRET，然后触发 trap 到 M-mode | trap 正常交付（MDT 已被 MRET 清零，新的 trap 设 MDT=1） |

---

## Group 7. medeleg[16] 只读零

**规范依据**：
- `norm:medeleg_16_no_rd0`：medeleg[16] 为只读零，double trap 不可委托

**测试职责**：验证 double-trap 异常（cause=16）不可通过 medeleg 委托到 S-mode。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MDEL-01 | medeleg[16] 只读零 | M-mode 尝试写 medeleg bit 16 = 1 | medeleg[16] 读回 0（只读零） |
| MDEL-02 | medeleg 全 1 写入验证 bit 16 | M-mode 写 medeleg = 全 1 后读回 | bit 16 读回 0，其他可写位为 1 |
| MDEL-03 | double-trap 始终到 M-mode | 尝试设置 medeleg[16]=1 后触发 double-trap | double-trap 始终交付到 M-mode（不会被委托） |

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1 (mstatus.MDT 字段) | MDT-01~03 | MDT 是 Smdbltrp 的核心机制 |
| P0（必须） | Group 2 (MDT/MIE 互斥) | MMIE-01~06 | MDT/MIE 互斥是 M-mode 中断安全的基础 |
| P0（必须） | Group 3 (M-mode Trap 交付) | MTRAP-01~05 | Trap 交付和 MDT 自动设置是核心功能 |
| P0（必须） | Group 6 (MRET/SRET/MNRET 清除) | MRET-01~08 | 清除 MDT 是 trap 返回的关键路径 |
| P1（重要） | Group 5 (Smrnmi 交互) | RNMI-01~09 | RNMI handler 是 double-trap 恢复的主要手段 |
| P1（重要） | Group 7 (medeleg[16]) | MDEL-01~03 | 验证不可委托约束 |
| P2（建议） | Group 4 (Critical Error) | CERR-01~04 | 需要平台恢复机制支持，执行风险高 |

---

## 测试实现说明

### 文件组织

```
damo-priv-test/
├── smdbltrp/
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_mdt_field.c           # Group 1: mstatus.MDT 字段行为
│       ├── test_mdt_mie_mutex.c      # Group 2: MDT/MIE 互斥
│       ├── test_mmode_trap.c         # Group 3: M-mode Trap 交付
│       ├── test_critical_error.c     # Group 4: Critical Error State
│       ├── test_smrnmi_dbltrap.c     # Group 5: Smrnmi 交互
│       ├── test_mret_mdt.c           # Group 6: MRET/SRET/MNRET 对 MDT 清除
│       └── test_medeleg16.c          # Group 7: medeleg[16] 只读零
└── common/                            # 复用通用框架
```

### 运行时检测

```c
static bool check_smdbltrp_extension(void) {
    /* Probe mstatus.MDT writability */
    uintptr_t old = CSRR(mstatus);
    CSRC(mstatus, MSTATUS_MDT_BIT);  /* try to clear MDT */
    uintptr_t new_val = CSRR(mstatus);
    CSRW(mstatus, old);  /* restore */
    return (new_val & MSTATUS_MDT_BIT) == 0;  /* MDT was cleared => Smdbltrp exists */
}

static bool check_smrnmi_extension(void) {
    /* Probe mnstatus CSR existence via trap */
    trap_expect_begin();
    uintptr_t val = CSRR(mnstatus);
    (void)val;
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}
```

### 通用测试模式

#### 模式 1：MDT/MIE 互斥测试（Group 2）

```c
/* MMIE-01: MDT=1 write clears MIE */
TEST_REGISTER(test_mmie_01);
bool test_mmie_01(void) {
    TEST_BEGIN("MMIE-01: MDT=1 write clears MIE regardless of MIE value");

    if (!check_smdbltrp_extension()) TEST_SKIP("Smdbltrp not available");

    uintptr_t orig = CSRR(mstatus);

    /* Clear MDT first, set MIE=1 */
    CSRC(mstatus, MSTATUS_MDT_BIT);
    CSRS(mstatus, MSTATUS_MIE);

    /* Write MDT=1 with MIE=1 in same write */
    uintptr_t write_val = CSRR(mstatus) | MSTATUS_MDT_BIT | MSTATUS_MIE;
    CSRW(mstatus, write_val);

    uintptr_t val = CSRR(mstatus);
    TEST_ASSERT("MIE cleared when MDT=1 written",
                (val & MSTATUS_MIE) == 0);
    TEST_ASSERT("MDT=1 successfully set",
                (val & MSTATUS_MDT_BIT) != 0);

    /* Restore */
    CSRW(mstatus, orig);
    TEST_END();
}
```

#### 模式 2：Double-trap 到 RNMI handler 测试（Group 5）

```c
/* RNMI-01: MDT=1 trap triggers RNMI handler */
TEST_REGISTER(test_rnmi_01);
bool test_rnmi_01(void) {
    TEST_BEGIN("RNMI-01: MDT=1 trap triggers RNMI handler");

    if (!check_smdbltrp_extension()) TEST_SKIP("Smdbltrp not available");
    if (!check_smrnmi_extension()) TEST_SKIP("Smrnmi not available");

    /* Setup: set MDT=1, ensure NMIE=1 */
    CSRS(mstatus, MSTATUS_MDT_BIT);

    /* Configure RNMI handler, then trigger an exception in M-mode
     * Since MDT=1, this becomes an unexpected trap -> RNMI handler */
    uintptr_t rnmi_handler_addr = (uintptr_t)&_rnmi_trap_handler;
    setup_rnmi_handler(rnmi_handler_addr);

    /* Trigger illegal instruction in M-mode with MDT=1 */
    trigger_rnmi_test_illegal();

    /* Check RNMI handler was invoked */
    TEST_ASSERT("RNMI handler was invoked", rnmi_handler_invoked());

    /* Check mncause: bit MXLEN-1=0 (exception), lower bits = cause code */
    uintptr_t mncause = CSRR(mncause);
    TEST_ASSERT("mncause bit MXLEN-1 = 0 (exception)",
                (mncause & (1UL << (sizeof(uintptr_t)*8 - 1))) == 0);

    /* Restore */
    clear_mdt();
    TEST_END();
}
```

### 关键注意事项

1. **扩展检测**：所有测试必须在运行时检测 Smdbltrp 的可用性（通过 `mstatus.MDT` 可写性），不可用时 TEST_SKIP。

2. **MDT 清除**：在测试 M-mode trap 前，必须确保 `mstatus.MDT=0`，否则测试本身会触发 unexpected trap。使用 `clear_mdt()` 辅助函数。

3. **Critical-error 风险**：Group 4 的测试将导致 hart 进入 critical-error state（停止执行）。这些测试需要平台支持外部复位或看门狗重启。在不支持恢复的平台上，这些测试应 TEST_SKIP 或仅在特定平台执行。

4. **Smrnmi 依赖**：Group 5 的测试需要 Smrnmi 扩展。通过检测 `mnstatus` CSR 的存在性判断，不存在时 TEST_SKIP。

5. **MIE 设置前提**：设置 `mstatus.MIE=1` 需要先确保 `mstatus.MDT=0`。在测试中应注意操作顺序。

6. **RNMI handler 配置**：RNMI handler 需要特殊配置（`mnscratch`、handler 地址等），且 handler 内部 NMIE=0，任何异常都会导致 critical-error。

7. **MDT 位位置**：RV64 下 MDT 在 `mstatus` bit 42；RV32 下 MDT 在 `mstatush` bit 10。测试代码需要根据 XLEN 选择正确的寄存器。

---

## 参考

- `SPEC/smdbltrp.adoc` — Smdbltrp Double Trap Extension
- `SPEC/machine.adoc` — Double Trap Control in mstatus Register (machine-double-trap)
- `SPEC/smrnmi.adoc` — Smrnmi Resumable NMI Extension
- `DOCS/testplan/Ssdbltrp_test_plan.md` — Ssdbltrp 扩展测试计划（Supervisor-level Double Trap）
- `DOCS/testplan/smrnmi_test_plan.md` — Smrnmi 扩展测试计划
- `DOCS/testplan/Hypervisor_test_plan.md` — Hypervisor 扩展综合测试计划
