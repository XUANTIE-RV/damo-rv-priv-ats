# RISC-V 双重陷阱（Double Trap）机制详解：Smdbltrp、Smrnmi 与 Ssdbltrp

> 本文基于 `SPEC/riscv-isa-manual`（Machine 1.13 / Supervisor 1.13）、`SPEC/riscv-debug`
> 与 `SPEC/riscv-sbi` 整理，所有结论均可追溯到 SPEC 中的 normative rule 标签（形如
> `norm:xxx`）。文末给出本仓库对应测试套件索引。

---

## 1. 背景：什么是"双重陷阱"，它为什么危险？

### 1.1 问题的根源——trap handler 的"非可重入临界区"

当 hart 发生一次 trap（异常或中断）时，硬件会把现场写入一组固定的 CSR：

- `mepc` / `sepc`：被中断指令的地址
- `mcause` / `scause`：trap 原因
- `mtval` / `stval`、`mtval2`：附加信息（出错地址、guest 物理地址等）
- `mstatus.MPP/MPV`（或 `sstatus.SPP`）：被中断的特权级 / 虚拟化模式

trap handler 进入后的**早期阶段**，还没来得及把上述现场保存到内存（软件栈），此时
这些 CSR 就是**唯一一份可恢复信息**。SPEC 把这一段称为 handler 的
**非可重入（non-reentrant）状态**（见 `machine-double-trap`，`src/priv/machine.adoc`）：

> A double trap typically arises during a sensitive phase in trap handling operations —
> when an exception or interrupt occurs while the trap handler is in a non-reentrant state.

### 1.2 二次 trap 会直接摧毁恢复现场

如果在 handler 尚未保存现场时**又来一次 trap**（称为 _unexpected trap_，"意外陷阱"），
硬件会用第二个 trap 的信息**覆盖** `mepc`/`mcause`/`mtval` 等。于是：

- 第一个 trap 的恢复现场**永久丢失**；
- handler 可能反复触发同一异常 → **无限 trap 循环**；
- 系统在关键状态被破坏后**静默失控**（silent corruption）。

普通异常/中断可以由软件通过"关中断、避免在临界区取指访存出错"来规避，但
**Hardware-Error 异常**（硬件错误，cause 19）与**硬件 NMI**是**不可预测**的，无法靠
软件纪律避免。这正是双重陷阱机制要解决的核心痛点（`machine-double-trap`）：

> managing Hardware-Error exceptions, which may occur unpredictably, presents
> significant challenges ... due to the potential risk of a double trap.

### 1.3 引入的架构能力

RISC-V 通过一组 `*dbltrp` 扩展，把"检测到二次 trap"变成一个**明确的架构事件**，并
提供**受控的降级路径**（进入 RNMI handler 或 critical-error 状态），而不是任由现场被
破坏。涉及三个扩展：

| 扩展 | 覆盖特权级 | 核心贡献 | 状态 |
|------|-----------|----------|------|
| **Smdbltrp** | M-mode | `mstatus.MDT` 位、critical-error 状态、Debug 集成 | 已批准 1.0 |
| **Ssdbltrp** | S/HS/VS/U 等 M 以下 | `sstatus.SDT`、`vsstatus.SDT`、`menvcfg.DTE`、`henvcfg.DTE`、double-trap 异常码 16 | 已批准 1.0 |
| **Smrnmi** | M-mode（配合 Smdbltrp） | 可恢复 NMI（RNMI）：`mnepc/mncause/mnstatus/mnscratch` + `MNRET` | 已批准 1.0 |

---

## 2. Smdbltrp：M-mode 的双重陷阱控制

（SPEC：`src/priv/smdbltrp.adoc` + `machine-double-trap` 段，`src/priv/machine.adoc`）

### 2.1 设计意图

Smdbltrp 的目标是：**在 M-mode 检测到双重陷阱，并把 hart 带入一个受控的
"critical-error（严重错误）状态"，同时向平台发出可诊断的信号**，避免不可控的现场破坏。
它引入两种降级路径：

1. 若实现了 Smrnmi → 把双重陷阱**转交给 RNMI handler**处理（最后一次软件挽救机会）；
2. 若未实现 Smrnmi，或 RNMI handler 执行期间再次双重陷阱 → 进入 **critical-error 状态**
   并向平台断言 `critical-error` 信号。

### 2.2 `mstatus.MDT` 位（M-mode Disable Trap）

- MDT 是 Smdbltrp 引入的 **WARL** 字段（`norm:mstatus_mdt_sz_warl`）。
- 位位置：RV64 下为 `mstatus` **bit 42**；RV32 下位于 `mstatush`（见 `mstatushreg.edn`）。
- **复位值为 1**（`norm:mstatus_mdt_rst`）——即上电后 M-mode 默认处于"禁止二次 trap"保护态。

MDT 的语义可以理解为一个"我已经进入 trap 临界区"的硬件标志：

- 取 trap 进入 M-mode 时：若当前 `MDT==0`，则硬件把它置 1，并正常投递该 trap
  （`norm:trap_exp`）；
- 若当前 `MDT==1` 又要取 trap → 判定为 **unexpected trap（意外陷阱）**
  （`norm:trap_unexp_mdt_1`）。

### 2.3 MDT 与 MIE 的互斥关系

这是 Smdbltrp 一个关键的一致性约束：

- 当通过**显式 CSR 写**把 `MDT` 置 1 时，`MIE` 被强制清 0
  （`norm:mstatus_mie_clr_by_mdt`）；RV64 下无论同一次写对 MIE 写了什么值都会清 0
  （`norm:mstatus_mie_clr_by_mdt_rv64`）。
- 反过来，只有当 `MDT` 已经是 0（RV64：或同一次写把 MDT 置 0）时，才允许把 `MIE`
  显式写 1（`norm:mstatus_mie_set_mdt_0`）。

含义：**MDT=1（处于 trap 临界保护）时，中断一定被关**，从而避免在临界区被普通中断
再次打入 trap。

### 2.4 unexpected trap 的处理：两条分支

当判定为 unexpected trap 时（`norm:trap_unexp_hndl_lead-in`）：

**分支 A —— 有 Smrnmi 且 `mnstatus.NMIE==1`**（`norm:trap_unexp_hndl_rnmi`）：
hart 陷入 **RNMI handler**。投递方式：
- 把这个 unexpected trap 本应写入 `mepc`/`mcause` 的值，分别写入 `mnepc`/`mncause`；
- `mnstatus` 的特权级字段写成 M-mode，且 `mnstatus.NMIE` 清 0。

> 注意（`machine-double-trap` NOTE）：走 RNMI 路径时，double trap 报告**不带** `mtval`
> / `mtval2` 信息。RNMI handler 若需要，得自己解码 `mnepc` 处指令并检查其源寄存器。

**分支 B —— 无 Smrnmi，或有 Smrnmi 但 `mnstatus.NMIE==0`**（`norm:trap_unexp_hndl_no_rnmi`）：
hart 进入 **critical-error 状态**：
- **不更新任何架构状态**（包括 `pc`）；
- 停止执行、关闭所有中断（含 NMI）；
- 向平台断言 `critical-error` 信号；
- 性能计数器/定时器是否继续更新为 UNSPECIFIED。

平台收到 `critical-error` 后的动作是**平台相关**的（`norm:critical_error`）：可重启该
hart、重启整个平台等。

### 2.5 RNMI 自身触发的 trap 不算 unexpected trap

为避免"RNMI 打死自己"，SPEC 特别规定：

- 由 RNMI 引起的 trap **不**被视为 unexpected trap，无论 MDT 状态如何
  （`norm:trap_unexp_rnmi`）；
- RNMI 引起的 trap **不**设置 MDT（`norm:mstatus_mdt_not_set_rnmi`）；
- 但是：在 M-mode 且 `mnstatus.NMIE==0` 时发生的 trap，**是** unexpected trap
  （`norm:trap_unexp_mnstatus_nmie_0`）——即"RNMI handler 执行期间再次出事"会直接升级到
  critical-error（分支 B）。

### 2.6 `mret` / `mnret` 清除 MDT

- 执行 `mret`，或在 M-mode 执行 `sret`：把 `MDT` 清 0（`norm:mstatus_mdt_clr_mret_sret`）。
  - 若同时实现 Ssdbltrp，且新特权级为 U/VS/VU，则 `sstatus.SDT` 也清 0
    （`norm:sstatus_sdt_clr_mret_sret`）；若为 VU，`vsstatus.SDT` 也清 0
    （`norm:vsstatus_sdt_clr_mret_sret`）。
- 执行 `mnret`（由 Smrnmi 提供）：若新特权级不是 M，则把 `MDT` 清 0
  （`norm:mstatus_mdt_clr_mnret`），并同样级联清 `sstatus.SDT`/`vsstatus.SDT`。

含义：MDT 的"关闭"是随 trap **返回**自动完成的，保证只有真正回到被中断上下文后才解除
临界保护。

### 2.7 critical-error 状态与 Debug 的集成

Smdbltrp 支持对处于 critical-error 状态的 hart 进行**调试**（`smdbltrp.adoc`）：

- Debug 规范（`SPEC/riscv-debug/xml/core_registers.xml`）新增 `dcsr.cetrig`（bit 19，
  仅当实现 Smdbltrp 时存在）：
  - `cetrig=0`（disabled）：critical-error 状态**不进入 Debug Mode**，直接向平台断言
    `critical-error` 信号；
  - `cetrig=1`（enabled）：critical-error 状态**进入 Debug Mode**（而不是断言信号），
    此时 `dcsr.cause=7`、`dcsr.extcause=0`（"critical error"），且该 cause 在所有进入
    Debug Mode 的原因中**优先级最高**。从 Debug Mode resume 会**回到** critical-error 状态
    （即 `cetrig=1` 时 resume 会立即再次进入 Debug Mode）。
- 另外，Debug 的 `dret` 返回流程也遵守 MDT/SDT 清除规则：若实现 Smdbltrp 且新特权级不是
  M，则 `MDT` 清 0；若实现 Ssdbltrp 且新特权级为 U/VS/VU，则 `sstatus.SDT`（VU 再加
  `vsstatus.SDT`）清 0（`SPEC/riscv-debug/Sdext.adoc`）。

### 2.8 double trap 不可委托

- double-trap 异常码为 **16**（`scause`/`mcause` 编码表，`src/priv/machine.adoc`）。
- `medeleg[16]` 为**只读零**——double trap **不可委托**给 S-mode
  （`norm:medeleg_16_no_rd0`）。

---

## 3. Smrnmi：可恢复的非屏蔽中断

（SPEC：`src/priv/smrnmi.adoc`）

### 3.1 设计意图

基础架构的 NMI 是**不可恢复的（UNMI）**：NMI 直接跳到 M-mode handler，并**覆盖**
`mepc`/`mcause`。如果 NMI 打断的正是 M-mode 的 trap handler，之前的 `mepc`/`mcause`
就丢了，执行无法恢复。

Smrnmi 提供**可恢复 NMI（RNMI）**：用一套**独立的** CSR 保存被中断现场，从而与普通
trap 的现场互不干扰。

### 3.2 新增 CSR 与指令

- 四个 M-mode CSR（`norm:Smrnmi_csrs`）：
  - `mnscratch`：RNMI handler 保存/恢复上下文用的 scratch（`norm:mnscratch_op`）；
  - `mnepc`：被中断指令的 PC（`norm:mnepc_op`），WARL、低位有 IALIGN 掩码规则；
  - `mncause`：RNMI 原因（`norm:mncause_op`）；
  - `mnstatus`：保存被中断特权级 `MNPP`、虚拟化模式 `MNPV`、`NMIE` 等。
- 一条新指令 **`MNRET`**（`norm:mnret_exist`）：M-mode-only，用 `mnepc`/`mnstatus`
  返回被中断的 PC、特权级、虚拟化模式，并置 `mnstatus.NMIE`（重新开中断）。

### 3.3 优先级与不可屏蔽性

- RNMI 中断信号是 hart 的输入，**优先级高于 hart 上任何其他中断或异常**
  （`norm:rnmi_priority`）；
- **软件无法屏蔽** RNMI——即使清 `mstatus.MIE` 也拦不住（`norm:rnmi_not_disabled`）；
- RNMI 中断 trap handler 地址与 RNMI 异常 trap handler 地址均为**实现定义**
  （`norm:rnmi_trap_addr` / `norm:rnmi_exc_trap_addr`）。

### 3.4 `mnstatus.NMIE`

- `NMIE=1`：NMI 使能；`NMIE=0`：**所有**中断被屏蔽（`norm:mnstatus_nmie_enable/disable`）。
- `NMIE=0` 时，hart 表现得像 `mstatus.MPRV` 被清（`norm:mnstatus_mprv_clear`）。
- 复位后 `NMIE=0`（`norm:mnstatus_nmie_reset`）——给软件初始化 RNMI 处理结构的时间窗。
- 软件可把 NMIE 置 1，但**清 0 无效**（`norm:mnstatus_nmie_set_clear`）；因此 RNMI 不支持
  直接嵌套，推荐 top-half/bottom-half 模型。

### 3.5 `mncause` 对 double trap 的专门编码

`mncause` 除了表示中断，还能表示"由 Smdbltrp 定义的双重陷阱"（`norm:mncause_doubletrap`）：

- 若原因是 M-mode 内导致 double trap 的异常：`mncause` 的 **bit MXLEN-1 = 0**，低位填
  **触发该 double trap 的异常码**。

这就是 Smdbltrp 与 Smrnmi 在 CSR 层面的直接耦合点：RNMI handler 可通过 `mncause`
判断"我是被一次 double trap 叫来的"，并知道原始异常类型。

---

## 4. 核心问题：Smdbltrp 与 Smrnmi 是不是"必须搭配"？

### 4.1 结论

**不是强制搭配，但两者高度协同、通常成对实现。** 具体地：

- **Smdbltrp 可以独立于 Smrnmi 存在**：没有 Smrnmi 时，M-mode 的 double trap 直接走
  分支 B → critical-error 状态（`norm:trap_unexp_hndl_no_rnmi`）。这条路径本身是自洽的。
- **Smrnmi 也可以独立存在**：它首先是一个"可恢复 NMI"扩展，用于硬件错误等场景，本身就
  有完整价值，不依赖 Smdbltrp。
- **但两者同时实现时产生 1+1>2 的效果**：Smdbltrp 把 double trap 变成一次 RNMI，让
  软件在 hart 彻底"死机"前有**最后一次可恢复的挽救机会**（分支 A）。SPEC 在
  `smdbltrp.adoc` 中把这条明确列为 normative：
  > `norm:Smdbltrp_with_Smrnmi_op`：When the Smrnmi extension is implemented, it enables
  > invocation of the RNMI handler on a double trap in M-mode to handle the critical error.

### 4.2 两者的交叉引用点（说明"相关性很高"）

| 交叉点 | Smdbltrp 侧 | Smrnmi 侧 |
|--------|-------------|-----------|
| double trap → RNMI handler | `norm:trap_unexp_hndl_rnmi` | RNMI 入口写 `mnepc/mncause/mnstatus` |
| RNMI 触发的 trap 不算意外陷阱 | `norm:trap_unexp_rnmi` | — |
| RNMI 不设 MDT | `norm:mstatus_mdt_not_set_rnmi` | — |
| NMIE=0 时 M-mode trap 是意外陷阱 | `norm:trap_unexp_mnstatus_nmie_0` | `mnstatus.NMIE` 语义 |
| `mncause` 承载 double-trap 原因 | — | `norm:mncause_doubletrap` |
| `mnret` 清 MDT | `norm:mstatus_mdt_clr_mnret` | `MNRET` 指令 |

可见二者在**控制流（谁调用谁）**、**CSR 状态（MDT ↔ NMIE、mncause）**、**返回路径
（mret/mnret）**三个层面深度耦合——这正是"相关性很高"的直接证据。

### 4.3 组合行为矩阵

| Smdbltrp | Smrnmi | `mnstatus.NMIE` | M-mode double trap 行为 |
|:--------:|:------:|:---------------:|--------------------------|
| 否 | 任意 | — | 无 MDT 概念；沿用基础架构（现场可能被覆盖） |
| 是 | 否 | — | 直接进入 **critical-error 状态** + 断言信号 |
| 是 | 是 | 1 | 转入 **RNMI handler**（分支 A，可恢复挽救） |
| 是 | 是 | 0 | 进入 **critical-error 状态**（RNMI handler 内再出事） |

---

## 5. 还有哪些扩展与它们高度相关？

### 5.1 Ssdbltrp（已发布，1.0）——把双重陷阱下沉到 M 以下特权级

（SPEC：`src/priv/ssdbltrp.adoc` + `supv-double-trap` 段，`src/priv/supervisor.adoc`）

Ssdbltrp 是 Smdbltrp 的"姊妹扩展"，处理 **M 以下特权级**（S/HS/VS/U）的双重陷阱：

- 引入 `sstatus.SDT`（S-mode Disable Trap，**bit 24**）：语义与 MDT 平行——
  SDT 由写置 1 时清 SIE，只有 SDT 已为 0（或同次写清 0）才能置 SIE
  （`norm:sstatus_sdt_sstatus_sie_overwrite`）。
- 取 trap 进入 S-mode 时若 `SDT==0` → 置 1 正常投递；若 `SDT==1` → unexpected trap，
  向 **M-mode 投递 double-trap 异常（`mcause=16`）**，并把"原本要写进 `mcause` 的值"
  改写进 `mtval2`（`norm:sstatus_sdt_trap`）。
- `SRET` 把 SDT 清 0（`norm:sstatus_sdt_sret`）。
- 引入 `menvcfg.DTE`（Double-Trap-Enable，**bit 59**）：`DTE=0` 时表现得像未实现
  Ssdbltrp，此时 `sstatus.SDT`/`vsstatus.SDT`/`henvcfg.DTE` 均为只读零
  （`norm:menvcfg_dte_op`）。
- M-mode 收到来自 S 的 double trap 后，**应重定向回 HS-mode**（拷贝 M-mode CSR 到 HS CSR，
  用 `MRET` 跳到 `stvec`）——见 `supv-double-trap` NOTE。

> 与 Smdbltrp 的分工：Smdbltrp 管 M-mode 自身的双重陷阱（终点是 critical-error/RNMI）；
> Ssdbltrp 管 M 以下特权级的双重陷阱（终点是把 double-trap 异常上抛给 M-mode，由固件决定
> 如何 invoke critical-error handler）。三者共享同一套"Disable-Trap 位 + unexpected trap"
> 设计范式。

### 5.2 Hypervisor 扩展（H，已发布）——虚拟化下的双重陷阱

当同时实现 H 扩展时，Ssdbltrp 额外增加（`ssdbltrp.adoc` 的 `norm:henvcfg_DTE` /
`norm:vsstatus_SDT`）：

- `henvcfg.DTE`：HS-mode 对某个 guest 开启双重陷阱保护；
- `vsstatus.SDT`：VS-mode 的 Disable-Trap 位。

由此支持两条虚拟化关键路径（`ssdbltrp.adoc`）：
- `norm:HS_mode_invoke_error`：VS-mode 双重陷阱时，**HS-mode** 可 invoke 虚拟机内的
  critical-error handler；
- `norm:M_mode_invoke_error`：S/HS-mode 双重陷阱时，**M-mode** 可 invoke OS/Hypervisor 的
  critical-error handler。

### 5.3 Debug / Sdext（已发布）——critical-error 的可调试性

见 §2.7。Smdbltrp 通过 `dcsr.cetrig` + `dcsr.cause=7/extcause=0` 与 Debug 规范集成，使
critical-error 的 hart 可以被外部调试器接管分析，而不必直接复位平台。Sdext 的 `dret`
返回也遵守 MDT/SDT 清除规则。

### 5.4 SBI SSE（Supervisor Software Events，开发中/草案）——软件侧 critical-error 通道

（SPEC：`SPEC/riscv-sbi/src/ext-sse.adoc`）

SSE 是 SBI 的一个扩展，允许 supervisor 软件注册并处理来自固件/hypervisor 的系统事件。
`supv-double-trap` NOTE 明确指出：

> In the event of a double trap, HS-mode and M-mode can utilize the SSE mechanism to invoke
> a critical-error handler in VS-mode or S/HS-mode, respectively.

即 SSE 是双重陷阱"落地到软件处理器"的**推荐通道之一**（可选的恢复辅助手段）。SSE 的事件
上下文里也专门保存/恢复了 `sstatus.SDT`（`INTERRUPTED_FLAGS[5]`），说明它是围绕 Ssdbltrp
设计的。SSE 目前仍在演进中，属于"尚未完全定型"的相关规范。

### 5.5 Zicfilp（CFI 前向边，已发布）——`mnstatus.MNPELP`

Smrnmi 与 Zicfilp 有直接交互：若实现 Zicfilp，`mnstatus` 增加 `MNPELP` 字段，保存进入
RNMI 前的 `ELP`（`norm:mnstatus_mnpelp_op`）；取 RNMI 时 `MNPELP=ELP` 且 `ELP=0`
（`norm:mnstatus_mnpelp_update`）；`MNRET` 返回时按目标特权级的 LPE 与 MNPELP 恢复 ELP
（`norm:mnret_zicfilp`）。这保证 RNMI/双重陷阱路径不会破坏 CFI 的着陆垫状态机。

### 5.6 RAS / Hardware Error / NMI（可靠性主题）

双重陷阱机制本质是 **RAS（Reliability, Availability, Serviceability）** 的一部分：

- Hardware-Error 异常（cause 19）与不可预测的硬件错误是双重陷阱的主要诱因；
- 基础 NMI "仅用于硬件错误场景，不复位处理器状态，以便诊断/上报/遏制错误"
  （`src/priv/machine.adoc` Non-Maskable Interrupts 段，`norm:nmi_op`/`norm:nmi_no_rst`）；
- Smrnmi 让这类硬件错误响应**可恢复**，Smdbltrp 让错误处理过程中的**二次故障可被遏制**。
- 仓库中 `SPEC/riscv-ras-eri`（RAS Error-record Register Interface）是同一主题下的配套规范。

---

## 6. CSR / 编码速查表

| 名称 | 位置 | 引入者 | 说明 |
|------|------|--------|------|
| `mstatus.MDT` | RV64: bit 42；RV32: `mstatush` | Smdbltrp | M-mode Disable Trap，复位=1 |
| `sstatus.SDT` | bit 24 | Ssdbltrp | S-mode Disable Trap |
| `vsstatus.SDT` | bit 24 | Ssdbltrp + H | VS-mode Disable Trap |
| `menvcfg.DTE` | bit 59 | Ssdbltrp | Double-Trap-Enable，0=当作未实现 |
| `henvcfg.DTE` | — | Ssdbltrp + H | 对 guest 开启双重陷阱 |
| `medeleg[16]` | bit 16 | — | 只读零，double trap 不可委托 |
| double-trap 异常码 | `mcause`=16 | Ssdbltrp | S/VS 双重陷阱上抛 M-mode |
| `mnepc/mncause/mnstatus/mnscratch` | — | Smrnmi | RNMI 独立现场 |
| `mnstatus.NMIE` | — | Smrnmi | 0=屏蔽所有中断，复位=0，清 0 无效 |
| `mnstatus.MNPELP` | — | Smrnmi + Zicfilp | 保存进入前 ELP |
| `MNRET` | 指令 | Smrnmi | 从 RNMI 返回并置 NMIE |
| `dcsr.cetrig` | bit 19 | Smdbltrp + Debug | 1=critical-error 进 Debug Mode |

---

## 7. 什么情况下需要用到这两个扩展？

### 7.1 Smdbltrp 的适用场景

- **功能安全 / 高可靠系统**（ISO 26262、IEC 61508 等）：需要"故障可遏制（fault
  containment）"，即软件出错或硬件错误时不能静默破坏系统，而应进入明确的 critical-error
  状态并通知平台。
- **M-mode 固件 / SBI 实现**：其 trap handler 会处理不可预测的 Hardware-Error，需要架构级
  的双重陷阱保护，避免 handler 临界区被二次 trap 摧毁。
- **需要可诊断的"死机"**：结合 Debug 的 `cetrig`，让 critical-error 的 hart 可被外部
  调试器接管，定位根因，而不是直接复位。

### 7.2 Smrnmi 的适用场景

- **硬件错误 / 严重事件的即时响应**：需要优先级最高、软件无法屏蔽的中断通道。
- **可恢复的 NMI**：希望 NMI 打断 M-mode trap handler 后仍能保存现场、事后恢复执行
  （基础 UNMI 做不到）。
- **与 Smdbltrp 协同做"最后一搏"**：把 M-mode 双重陷阱转成一次 RNMI，让软件有机会记录
  日志、隔离故障、优雅重启，而非直接 critical-error。

### 7.3 Ssdbltrp / H 的适用场景

- **操作系统内核（S-mode）**：内核 trap 入口临界区需要双重陷阱保护，出错时上抛固件处理。
- **Hypervisor（HS-mode）+ 虚拟化**：需要区分"guest（VS）内双重陷阱"与"OS/HS 内双重
  陷阱"，分别 invoke 对应层级的 critical-error handler，实现**故障隔离**——一个 VM 崩溃
  不应拖垮整机。

### 7.4 一句话总结

> **Smdbltrp/Ssdbltrp** 负责"**检测并遏制**"双重陷阱（把不可控的现场破坏变成受控的
> critical-error / double-trap 异常）；**Smrnmi** 负责在 M-mode 提供一个"**可恢复的最后
> 处理机会**"。二者面向同一目标——**在硬件/软件关键错误下保持系统的可诊断性与可遏制性
> （RAS）**，因此高度相关、常成对出现，但各自也能独立成立。

---

## 8. 本仓库相关测试套件索引

| 套件目录 | 覆盖内容 |
|----------|----------|
| `Smdbltrp/` | MDT 字段、MDT/MIE 互斥、`medeleg[16]` 只读零、M-mode double trap、`mret` 清 MDT、critical-error、**Smrnmi×Smdbltrp 联合**（`tests/test_smrnmi_dbltrap.c`） |
| `Smrnmi/` | RNMI CSR 基础、`mncause`、`MNRET`、RNMI 入口、RNMI 操作语义 |
| `Ssdbltrp/` | SDT 字段、SDT/SIE 互斥、`menvcfg.DTE`、`mtval2` 依赖、`sret`/`xret` 清 SDT、S-mode trap、`medeleg[16]` |
| `Hypervisor_Ssdbltrp/` | 虚拟化交叉：`henvcfg`、`menvcfg`、`vsstatus.SDT`、`sret`/`xret` 在 VS/VU 下的 SDT 行为 |

> 框架侧注意点（见 `common/entry.S`、`common/trap.c`、`common/test_framework.h`）：
> 复位后 `mstatus.MDT=1`，M-mode 中任何同步异常都会变成致命 double trap，因此框架在
> 启动早期（任何可能 trap 的指令之前）显式清 MDT；在需要"主动触发 trap 做探测"的用例中，
> 也必须先清 MDT，否则会误入 critical-error 而挂死。
