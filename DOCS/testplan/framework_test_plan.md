# 测试框架特权态切换功能测试计划

> 本文档描述对 `common/` 目录中测试框架的特权态切换功能进行验证的测试计划。测试覆盖 `goto_priv()`、`get_current_priv()`、`run_in_priv()` 等核心 API，以及底层 `mret`/`sret` 与 T-SBI `ecall` 服务通道在所有特权级（M/S/U/VS/VU）之间的切换行为。

---

## 执行模型（T-SBI，S-mode 常态）

本套件运行在 T-SBI 的 **S-mode 常态模型**下（见 `DOCS/framework/tsbi_design_brief.md` 第 1 节），
这是理解下文所有用例前置条件的前提：

1. `common/entry.S` 在 M-mode 完成 boot 期设置（PMP 全空间 RWX、三级 trap 向量、委托基线）后
   用 `mret` 降入 S-mode，`main()` 在 **S-mode**（`ENABLE_HYP=1` 时即 HS-mode）执行。
2. **升权是请求，降权是权利**：`goto_priv()` 向更高特权级切换时发 T-SBI `ecall`
   （`a0` = `TSBI_GOTO_xMODE`，服务号走寄存器而非全局变量），由上级 handler 置 `xPP` 后 `xret`；
   向更低特权级切换仍是本地 `mret`/`sret`，不经过 T-SBI。
3. `TEST_END()` / `TEST_SKIP()` 先 `goto_priv(PRIV_M)` 让 `reset_state()` 拿到 M 权限，
   `reset_state()` 收尾再 `goto_priv(PRIV_S)` 交还控制权，因此**每个用例结束后特权级回到 S**，
   `main()` 也必须在 S-mode 返回（`entry.S` 的 `_s_main_return` 依赖这一点完成 halt）。
4. 因此用例名中的 “M→X” 起点必须由用例自己建立：`main()` 已在 S-mode，直接写
   `goto_priv(PRIV_X)` 会把 “M→S 下降” 退化成 no-op。Group 1/4/5/6/9/10 中所有以 M 为起点的
   用例都在开头显式 `goto_priv(PRIV_M)`。
5. 确需 M-mode 语义的用例采用 **M-excursion**：`goto_priv(PRIV_M)` 或
   `run_in_priv(PRIV_M, fn, arg)`，执行完立即回 S（`run_in_priv` 在 `fn` 返回后自动回落到
   进入时的特权级）。
6. 本套件**不得**通过 `-DTSBI_BOOT_MODE_M` 退回 M-mode 常态启动：该开关是给拥有早期 M 环境
   或自定义 trap 入口的套件（Smdbltrp / Smrnmi / ntrace / iopmp / clic 等）用的，
   而本套件被测对象正是 T-SBI 的 S-mode 常态切换链路本身。

> [!NOTE]
> T-SBI 协议层的独立验收由 `tsbi_test/` 承担（服务号编码、CSR 代理、三级 dispatch、委托基线）。
> 本套件只验证建立在该协议之上的**特权态切换 API 行为**，两者不重复。

---

## 范围

### 覆盖的功能

- **基本特权级切换**：M↔S、M↔U、S↔U 的双向切换（`goto_priv()`）
- **虚拟化特权级切换**（需 `ENABLE_HYP=1`）：M↔VS、M↔VU、S↔VS、S↔VU、VS↔VU、VS↔HS 的双向切换
- **`current_priv` 软件跟踪**：验证 `get_current_priv()` 返回正确的当前特权级
- **`run_in_priv()` 函数执行**：在目标特权级执行函数并正确返回
- **CSR 状态验证**：切换后 `mstatus.MPP`/`MPV`/`SPP` 等字段的正确性
- **同特权级切换**：`goto_priv()` 传入当前特权级时应为 no-op
- **连续切换**：多次连续切换后状态一致性
- **切换后 ecall 机制**：低特权级通过 ecall 回到高特权级的正确性

### 不在本文档范围

- 各扩展（Smcsrind、Sscsrind、Hypervisor 等）的具体功能测试 — 由各自测试计划覆盖
- 页表翻译（VS-stage、G-stage）的正确性 — 由 VM 相关测试覆盖
- PMP 权限检查行为 — 由 PMP 测试覆盖
- trap 框架的异常捕获功能（`trap_expect_begin`/`trap_was_triggered` 等）的独立测试 — 属于 trap 框架测试范围（部分在本计划中作为切换副作用被间接验证）

---

## 框架实现依据

本测试计划基于以下实现（不标行号，以符号名为准）：

| 组件 | 文件 | 描述 |
|------|------|------|
| `goto_priv()` | `common/privilege.c` | 双向特权级切换入口：向上发 T-SBI GOTO 请求，向下本地 mret/sret |
| `lower_priv()` | `common/privilege.c` | 向下切换；虚拟化目标且当前非 M 时先 ecall 升到 M 再 mret |
| `set_prev_priv()` | `common/privilege.c` | 配置 `mstatus.MPP`/`MPV` 或 `sstatus.SPP` |
| `do_ecall()` | `common/privilege.c` | 将目标特权级映射为 T-SBI GOTO 操作码并发 `ecall` |
| `run_in_priv()` | `common/privilege.c` | 在目标特权级执行函数，返回后自动回落到进入时的特权级 |
| `get_current_priv()` | `common/privilege.c` | 返回 `current_priv` 软件跟踪变量 |
| `tsbi_ecall()` / GOTO 操作码 | `common/tsbi/tsbi.h` | 协议单一真相源：`a0` = 操作码，`a1` = 参数 |
| `m_trap_handler()` | `common/trap.c` | M 级 handler：P0 嵌套 fault → P1 raw-ecall 逃逸 → P2 GOTO/SYSTEM dispatch |
| `s_trap_handler()` | `common/trap.c` | S/HS 级 handler：本地可办的就地办，其余转发 M |
| `tsbi_deleg_*_baseline()` | `common/tsbi/tsbi_deleg.c` | 委托优先基线（medeleg/mideleg/hedeleg/hideleg） |
| boot 降权与收尾 | `common/entry.S` | PMP 上提、装三级向量、`mret` 降 S；`_s_main_return` 请求升 M 后 halt |

### 特权级常量定义

```
PRIV_U  = 0   /* User mode */
PRIV_S  = 1   /* Supervisor mode (HS-mode when ENABLE_HYP=1) */
PRIV_M  = 3   /* Machine mode */
PRIV_VU = 4   /* V=1, nominal U-mode (bit 2 = V flag) */
PRIV_VS = 5   /* V=1, nominal S-mode (bit 2 = V flag) */
```

### 切换路径矩阵

```
向下切换（本地 mret/sret，不经 T-SBI）：
  M  -> S:    mret (MPP=S, MPV=0)
  M  -> U:    mret (MPP=U, MPV=0)
  S  -> U:    sret (SPP=U)
  M  -> VS:   mret (MPP=S, MPV=1)
  M  -> VU:   mret (MPP=U, MPV=1)

向上切换（T-SBI ecall 服务请求，a0 = GOTO 操作码 1..5；能在本级兑现就不上转）：
  U  -> S:    ecall(GOTO_SMODE, cause=8) -> medeleg[8]=1 递至 S handler；
              S 有权兑现：置 sstatus.SPP=1 后 sret（不经 M）
  U  -> M:    ecall(GOTO_MMODE, cause=8) -> S handler 无权 -> 转发 M -> mret(MPP=M)
  S  -> M:    ecall(GOTO_MMODE, cause=9) -> medeleg[9] 恒为 0，直达 M handler
  VS -> HS:   ecall(GOTO_SMODE, cause=10) -> hedeleg[10] read-only-zero 故停在 HS；
              HS 有权兑现：清 hstatus.SPV、置 SPP=1 后 sret（不经 M）
  VS -> M:    ecall(GOTO_MMODE, cause=10) -> HS 无权 -> 转发 M -> mret(MPP=M)
  VU -> VS:   ecall(GOTO_VSMODE, cause=8) -> medeleg[8]+hedeleg[8] 递至 VS handler；
              VS 无权 -> 转发 HS，HS 兑现：置 hstatus.SPV=1、SPP=1 后 sret（不经 M）
  VU -> M:    ecall(GOTO_MMODE, cause=8) -> VS handler -> HS -> 转发 M -> mret(MPP=M)

虚拟化目标的间接路径（这是 privilege.c 的 lower_priv 实现选择，不是架构限制）：
  S  -> VS/VU: goto_priv 将其归为“下降” -> lower_priv 先 ecall(GOTO_MMODE) 升到 M，
               再由 M 置 MPV=1、MPP=S/U 后 mret
  VS -> VU:    同上，先 ecall(GOTO_MMODE) 到 M，再 mret(MPV=1, MPP=U)
```

> [!IMPORTANT]
> 进入 V=1 有**两条共存但不同源**的路径，不要混为一谈：
>
> - `tsbi_s_dispatch()` 在 HS 级就地兑现 `GOTO_VSMODE`/`GOTO_VUMODE`：置 `hstatus.SPV=1` +
>   `sstatus.SPP` 后 `sret`。代码注释将其定义为**强制而非优化**：SPV+sret 是真实 hypervisor
>   进入 guest 的唯一方式，走 M 的 mret+MPV 会完全绕过 `hstatus.SPV`，使 SPV 语义无法被观测。
> - `goto_priv(PRIV_VS/PRIV_VU)` 从 S 或 VS 出发时，因 `priv_cmp` 把虚拟化目标归为“下降”，
>   走的是 `lower_priv()` 的 ecall→M→mret(MPV=1) 路径。
>
> 因此 Group 8 的 S→VS / S→VU / VS→VU **不**经过 HS 级 SPV 兑现；只有 VU→VS
> （`priv_cmp` 判为上升，发 GOTO_VSMODE）会走 HS 就地兑现的 SPV+sret 路径。

---

## Group 1. 基本向下切换（M-mode → S/U-mode）

**测试职责**：验证从 M-mode 通过 `goto_priv()` 向下切换到 S-mode 和 U-mode 的正确性，包括 `current_priv` 跟踪和 `mstatus.MPP` 配置。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FW-DOWN-01 | M→S 切换后 current_priv 验证 | M-mode 调用 `goto_priv(PRIV_S)`，检查 `get_current_priv()` | 返回 `PRIV_S` (1) |
| FW-DOWN-02 | M→U 切换后 current_priv 验证 | M-mode 调用 `goto_priv(PRIV_U)`，检查 `get_current_priv()` | 返回 `PRIV_U` (0) |
| FW-DOWN-03 | M→S 切换后 mstatus.MPP 验证 | M-mode 调用 `goto_priv(PRIV_S)`，回到 M-mode 后检查切换过程中 MPP 是否正确配置 | MPP 在 mret 前被设为 S (01)，mret 后 MPP 被硬件清零 |
| FW-DOWN-04 | M→U 切换后 mstatus.MPP 验证 | M-mode 调用 `goto_priv(PRIV_U)`，验证 MPP 配置 | MPP 在 mret 前被设为 U (00) |
| FW-DOWN-05 | M→S 切换后 MPRV 清零验证 | M-mode 先设 MPRV=1，调用 `goto_priv(PRIV_S)`，检查 MPRV 是否被清零 | MPRV=0（`set_prev_priv` 中清除了 MPP 和 MPRV） |
| FW-DOWN-06 | M→S 切换后 S-mode 执行验证 | M-mode 用 `run_in_priv(PRIV_S, s_mode_probe_fn, 0)` 执行双向探针：先 trap 保护读 `sstatus`，再 trap 保护读 `mstatus` | 探针返回 0x3：sstatus 可读 **且** mstatus 被拒（两者合起来才能把执行钉在 S：单看 sstatus 在 M-mode 也成立，单看 mstatus 被拒在 U-mode 也成立） |
| FW-DOWN-07 | M→U 切换后 U-mode 执行验证 | M-mode 调用 `goto_priv(PRIV_U)`，在 U-mode 尝试读取 `sstatus` CSR | 触发 illegal-instruction 异常 (cause=2)（U-mode 不可访问 S-mode CSR） |
| FW-DOWN-08 | M→S 切换后 mstatus.MPV 清零 | M-mode 调用 `goto_priv(PRIV_S)`（非虚拟化目标），检查 MPV 位 | MPV=0（非虚拟化切换不应设置 V=1） |

> [!NOTE]
> - FW-DOWN-03/04 验证 `set_prev_priv()` 对 `mstatus.MPP` 的配置。由于 `mret` 会自动将 MPP 清零，需要在 `mret` 前（即 `lower_priv` 内部）或回到 M-mode 后通过间接方式验证。
> - FW-DOWN-05 验证 `set_prev_priv()` 中 `ms &= ~((3UL << 11) | (1UL << 17))` 的 MPRV 清除行为。`mret` 在 MPP=M 时不清除 MPRV，但 MPP≠M 时 `mret` 会自动清除 MPRV。
> - FW-DOWN-07 利用 U-mode 无法访问 `sstatus` 的特性来验证确实切换到了 U-mode。

---

## Group 2. 基本向上切换（S/U-mode → M-mode）

**测试职责**：验证从低特权级通过 `goto_priv()` 向上切换到高特权级的正确性（ecall 路径）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FW-UP-01 | S→M 切换后 current_priv 验证 | S-mode 调用 `goto_priv(PRIV_M)`，检查 `get_current_priv()` | 返回 `PRIV_M` (3) |
| FW-UP-02 | U→M 切换后 current_priv 验证 | U-mode 调用 `goto_priv(PRIV_M)`，检查 `get_current_priv()` | 返回 `PRIV_M` (3) |
| FW-UP-03 | S→M ecall 路径验证 | S-mode 调用 `goto_priv(PRIV_M)`，回 M 后用 `PRIV_DO(CSRR(mstatus))` + `CHECK_NO_TRAP` 验证 | ecall-from-S (cause=9)，medeleg[9] 恒为 0 所以直达 M handler；handler 设 MPP=M 后 mret，mstatus 可读 |
| FW-UP-04 | U→M ecall 路径验证 | U-mode 调用 `goto_priv(PRIV_M)`，同上验证 | ecall-from-U (cause=8) 先递到 S handler，S 无权升权故转发 M；mstatus 可读 |
| FW-UP-05 | U→S ecall 路径验证 | U-mode 调用 `goto_priv(PRIV_S)`，在 S-mode 双向探针：`sstatus` 可读 + `mstatus` 被拒 | 两个断言均成立，证明落在 S 而不是 M（SRET 已将 SPP 清零，无法用状态位反推） |
| FW-UP-06 | S→M 切换后 M-mode CSR 可访问 | S-mode 用 `run_in_priv(PRIV_M, m_mode_probe_fn, 0)` 在 M-mode trap 保护读 `mstatus` | helper 返回 1（读取未触发异常），证明确实在 M-mode |
| FW-UP-07 | U→S 切换后 S-mode CSR 可访问 | 先在 M-mode 取 `stvec` 参考值；经 U→S 后在 S-mode trap 保护读 `sstatus`/`stvec` 并比对 `stvec` | 两次读取均无异常，且 S-mode 读到的 `stvec` 等于 M-mode 参考值（被仿真/重定向/编造的读取不会相等） |

> [!NOTE]
> - 向上切换始终走 T-SBI 服务请求：`do_ecall()` 把目标特权级映射成 GOTO 操作码放进 `a0`
>   （`TSBI_GOTO_MMODE`=1 … `TSBI_GOTO_VUMODE`=5）；handler 只从 trap frame 读寄存器，
>   不再使用全局 `ecall_args[]`。
> - 服务该请求的 handler 设置 `mstatus.MPP` 为目标特权级、把 `mepc` 推进到 ecall 后一条指令，
>   然后 `mret` 返回；返回后 MPP 被硬件清零为最低实现特权级。
> - FW-UP-05 验证 U→S 的路径：U-mode ecall（cause=8，GOTO_SMODE）由 medeleg[8] 递到 S handler，
>   **升权到 S 属于 S handler 自己的权限范围**，它置 `sstatus.SPP=1` 后直接 `sret`，全程不经 M。
> - 本组用例的常态起点是 S-mode（`main()` 所在特权级），`goto_priv(PRIV_S)` 是 no-op；
>   U 起点由 `goto_priv(PRIV_U)` 建立。
> - **不得用 `TEST_ASSERT(..., true)` 表达“读取成功”**：未加保护的 CSR 读取一旦 fault 会被
>   当成 UNEXPECTED TRAP 直接 halt，用例根本没机会失败。正确写法是 `PRIV_DO(...)` +
>   `CHECK_NO_TRAP(...)`，或 helper 回报的 cause/标志位。
> - 需要证明“到达的是哪一级”时用**双向探针**：读一个该级可访问的 CSR 加一个不可访问的，
>   单向读取在相邻特权级上往往同样成立。

---

## Group 3. 同特权级切换（No-op）

**测试职责**：验证 `goto_priv()` 传入当前特权级时不产生任何效果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FW-NOOP-01 | M→M 同特权级切换 | M-mode 调用 `goto_priv(PRIV_M)`，检查 `get_current_priv()` | 返回 `PRIV_M`，无 ecall/mret 执行 |
| FW-NOOP-02 | S→S 同特权级切换 | S-mode 调用 `goto_priv(PRIV_S)`，检查 `get_current_priv()` | 返回 `PRIV_S`，无异常 |
| FW-NOOP-03 | U→U 同特权级切换 | U-mode 调用 `goto_priv(PRIV_U)`，检查 `get_current_priv()` | 返回 `PRIV_U`，无异常 |

> [!NOTE]
> - `goto_priv()` 第一行检查 `if (target == current_priv) return;`，因此同特权级切换不会执行任何 ecall 或 mret/sret。
> - 验证这些用例确保 no-op 路径不产生副作用。

---

## Group 4. 往返切换一致性

**测试职责**：验证从高特权级切换到低特权级再切回高特权级后，状态一致性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FW-RT-01 | M→S→M 往返切换 | M-mode 切到 S-mode 再切回 M-mode，检查 `get_current_priv()` | 返回 `PRIV_M` |
| FW-RT-02 | M→U→M 往返切换 | M-mode 切到 U-mode 再切回 M-mode，检查 `get_current_priv()` | 返回 `PRIV_M` |
| FW-RT-03 | M→S→U→M 三级往返 | M→S→U→M 连续切换，每步验证 `get_current_priv()` | 每步返回值分别为 S、U、M |
| FW-RT-04 | M→U→S→M 跨级往返 | M→U→S→M 连续切换（注意 U→S 需要经过 M-mode ecall），每步验证 | 每步返回值分别为 U、S、M |
| FW-RT-05 | M→S→U→S→M 多次往返 | M→S→U→S→M 五次切换，验证最终状态 | 最终 `get_current_priv()` 返回 `PRIV_M` |
| FW-RT-06 | 往返切换后 mstatus 完整性 | M→S→M 往返后，检查 mstatus 的关键位（MIE、SIE、MPRV）是否保持不变 | mstatus 关键位与切换前一致（ecall handler 仅修改 MPP/MPV/MPRV） |

> [!NOTE]
> - FW-RT-04 的 U→S 路径：U-mode `goto_priv(PRIV_S)` → ecall → M-mode handler 设置 MPP=S → mret 到 S-mode。这条路径经过 M-mode handler 但不经过 S-mode handler。
> - FW-RT-06 验证 ecall handler 不会意外修改 mstatus 的其他位。

---

## Group 5. run_in_priv() 函数执行

**测试职责**：验证 `run_in_priv()` 在目标特权级正确执行函数并返回结果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FW-RUN-01 | run_in_priv S-mode 执行 | M-mode 调用 `run_in_priv(PRIV_S, fn, arg)`，fn 返回固定值 | 返回值与 fn 的返回值一致 |
| FW-RUN-02 | run_in_priv U-mode 执行 | M-mode 调用 `run_in_priv(PRIV_U, fn, arg)`，fn 返回固定值 | 返回值与 fn 的返回值一致 |
| FW-RUN-03 | run_in_priv S-mode 中验证特权级 | fn 内部调用 `get_current_priv()` 并返回 | fn 返回 `PRIV_S` |
| FW-RUN-04 | run_in_priv U-mode 中验证特权级 | fn 内部调用 `get_current_priv()` 并返回 | fn 返回 `PRIV_U` |
| FW-RUN-05 | run_in_priv 执行后回到调用方特权级 | 先 `goto_priv(PRIV_M)` 建立 M-mode 调用方，`run_in_priv(PRIV_S, fn, arg)` 返回后检查 `get_current_priv()` | 返回 `PRIV_M`（`run_in_priv` 回落到**进入时**的特权级，不是固定回 M） |
| FW-RUN-06 | run_in_priv 参数传递 | M-mode 下 fn 接收 arg 并原样返回 | 返回值与传入的 arg 一致（参数确实跨过了特权级下降） |
| FW-RUN-07 | run_in_priv M-mode 直接调用 | 在 M-mode 调用 `run_in_priv(PRIV_M, fn, arg)` | fn 直接在 M-mode 执行，返回值正确（不走 mret 路径） |
| FW-RUN-08 | run_in_priv 同特权级直接调用 | 在 S-mode 调用 `run_in_priv(PRIV_S, fn, arg)` | fn 直接执行（priv >= current_priv 走直接调用路径） |

> [!NOTE]
> - `run_in_priv()` 内部使用 `_run_trampoline`：在目标特权级执行 fn(arg) 后，通过 T-SBI
>   `GOTO_MMODE` ecall 回到 M-mode，再把控制权交回进入时的特权级。
> - **调用方特权级决定走哪条路**：`priv >= current_priv` 时直接调用 fn（不走 mret/sret），
>   否则才是真正的特权级下降。因此 FW-RUN-01~04/06 必须在开头显式 `goto_priv(PRIV_M)`：
>   `main()` 已在 S-mode，否则 `run_in_priv(PRIV_S, ...)` 会退化成同级直接调用，
>   与 FW-RUN-08 重复，而方案要测的 mret 下降路径反而没人覆盖。
> - FW-RUN-07/08 测的正是 `priv >= current_priv` 的直接调用分支：07 在 M-mode 调 PRIV_M，
>   08 在 S-mode 调 PRIV_S。

---

## Group 6. 虚拟化向下切换（M-mode → VS/VU-mode）

**测试职责**：验证从 M-mode 通过 `goto_priv()` 切换到虚拟化特权级（VS/VU）的正确性。需要 `ENABLE_HYP=1`。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FW-VIRT-DOWN-01 | M→VS 切换后 current_priv | M-mode 调用 `goto_priv(PRIV_VS)`，检查 `get_current_priv()` | 返回 `PRIV_VS` (5) |
| FW-VIRT-DOWN-02 | M→VU 切换后 current_priv | M-mode 调用 `goto_priv(PRIV_VU)`，检查 `get_current_priv()` | 返回 `PRIV_VU` (4) |
| FW-VIRT-DOWN-03 | M→VS mstatus.MPV=1 验证 | M→VS 后在 VS-mode 触发一次落在 M 的 illegal-instruction trap，读 handler 入口快照 `trap_get_mpv()` | MPV=1（norm:mstatus_mpv_op：陷入 M 时 MPV ← 当时的 V）；同法从 HS-mode 取快照作为对照应为 MPV=0 |
| FW-VIRT-DOWN-04 | M→VS mstatus.MPP=S 验证 | 同 DOWN-03 的观测窗口，读 `trap_get_mpp()` | MPP=S (01)，即 `PRIV_VS & 3`；硬件在陷入 M 时重写 MPP 为陷入方的名义特权级 |
| FW-VIRT-DOWN-05 | M→VU mstatus.MPP=U 验证 | 同上，从 VU-mode 取快照 | MPP=U (00)，即 `PRIV_VU & 3` |
| FW-VIRT-DOWN-06 | M→VU mstatus.MPV=1 验证 | 同上，从 VU-mode 取快照 | MPV=1 |
| FW-VIRT-DOWN-07 | M→VS 后 hstatus.SPV 验证 | M→VS 后在 VS-mode 读 `hstatus`（HS 级 CSR），该访问招 virtual-instruction 异常并因 hedeleg[22] read-only-zero 而必然递到 HS-mode；读 `trap_get_spv_snap()` | cause=22；SPV=1（硬件在“从 V=1 陷入 HS”时写 SPV，norm:H_trap_hs_csrwrites） |
| FW-VIRT-DOWN-08 | M→VS 后 VS-mode CSR 访问 | M→VS 后在 VS-mode 分别读 `sstatus`(0x100) 与 `vsstatus`(0x200)；另在 M-mode 读 `vsstatus` 作对照 | M-mode 读 `vsstatus` 成功；VS-mode 读 `sstatus` 成功（即 vsstatus 的虚拟化视图，norm:H_vscsrs_sub）；VS-mode 直接按 0x200 读 `vsstatus` 必须报 cause=22（norm:H_vscsrs_acc_vs / norm:H_vscsrs_acc_m_hs） |
| FW-VIRT-DOWN-09 | M→VU 后 VU-mode 限制验证 | M→VU 切换后，在 VU-mode 尝试读取 `sstatus` | 触发 **virtual-instruction 异常 (cause=22)**，不得模糊为 illegal-instruction（norm:H_virtinst_vu_nonhigh_supervisor_allowedhs_tvm0） |

> [!NOTE]
> - 虚拟化目标的编码：`target & 3` 提取名义特权级（MPP），`target & 0x4` 检测 V=1 标记。`PRIV_VS=5 (101b)` → MPP=S, V=1；`PRIV_VU=4 (100b)` → MPP=U, V=1。
> - `set_prev_priv()` 中 M-mode 路径：先清除 MPP 和 MPV，然后设置 `MPP = target & 3`，若 `target & 0x4` 则设置 MPV=1。
> - **DOWN-03~06 的观测窗口**：MPP/MPV 都被执行切换的那条 `mret` 消费掉，目标侧无法回读。
>   硬件会在**下一次陷入 M-mode** 时重写二者（MPP ← 陷入方名义特权级，MPV ← 陷入方 V），
>   而 `m_trap_handler` 在自己触碰它们之前已快照（`trap_get_mpp()` / `trap_get_mpv()`）。
>   因此先在 M-mode 用 `trap_undelegate_exc(CAUSE_ILLEGAL_INST)` 把 cause 2 移出委托基线
>   （否则 trap 停在 VS/HS，mstatus 根本不会被写），再在 VS/VU 内执行一条保留编码
>   `.half 0x0000` 取回快照；`reset_state()` 在 `TEST_END()` 里恢复基线。
> - 激励用 16 位保留编码而不是 `.4byte 0`：开启 C 扩展后 4 字节全零会被解为两条 `c.illegal`，
>   handler 的 `next_instruction()` 只推进 2 字节，会在记录已解除武装后撞上第二条。
> - DOWN-08 的预期以 SPEC 为准：“VS-mode 可访问虚拟化 S-mode CSR” 指的是通过 `sstatus`
>   这类 **S 地址** 访问 vsstatus；直接按 VS CSR 自己的地址（0x200）访问在 V=1 时必须报
>   virtual-instruction。VS CSR “作为自己” 只能从 M-mode 或 HS-mode 访问。
> - 进入 VS/VU **不需要 G-stage 页表**：`hgatp.MODE=Bare` 且 `vsatp.MODE=Bare` 时 V=1 的取指
>   直接按物理地址翻译，Guest 侧代码照常执行。Group 6/7/8/9 均据此实现，不得以
>   “需要 hgatp” 为由 SKIP。

---

## Group 7. 虚拟化向上切换（VS/VU → M/HS-mode）

**测试职责**：验证从虚拟化特权级向上切换到高特权级的正确性。需要 `ENABLE_HYP=1`。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FW-VIRT-UP-01 | VS→M 切换后 current_priv | VS-mode 调用 `goto_priv(PRIV_M)`，检查 `get_current_priv()` | 返回 `PRIV_M` (3) |
| FW-VIRT-UP-02 | VU→M 切换后 current_priv | VU-mode 调用 `goto_priv(PRIV_M)`，检查 `get_current_priv()` | 返回 `PRIV_M` (3) |
| FW-VIRT-UP-03 | VS→HS (PRIV_S) 切换后 current_priv | VS-mode 调用 `goto_priv(PRIV_S)`，检查 `get_current_priv()` | 返回 `PRIV_S` (1)（退出虚拟化，V=0） |
| FW-VIRT-UP-04 | VU→VS 切换后 current_priv | VU-mode 调用 `goto_priv(PRIV_VS)`，检查 `get_current_priv()` | 返回 `PRIV_VS` (5)（保持 V=1，名义特权级提升到 S） |
| FW-VIRT-UP-05 | VS→M ecall 路径验证 | VS-mode 调用 `goto_priv(PRIV_M)`，回到 M 后用 trap 保护读 `hstatus` | ecall-from-VS (cause=10) 因 hedeleg[10] read-only-zero 停在 HS 并转发至 M；mret 时 MPP=M 所以 V 保持 0（norm:mstatus_mpv_op），hstatus 读取不触发异常 |
| FW-VIRT-UP-06 | VS→M 后 M-mode 可访问 H-ext CSR | VS→M 后，在 M-mode 用 trap 保护读取 `hstatus` | 读取不触发异常（证明确实在 M-mode 且 V=0） |
| FW-VIRT-UP-07 | VS→HS 后 S-mode 行为验证 | VS→HS 后，用 `run_in_priv(PRIV_S, read_hstatus_fn, 0)` 在 HS-mode 读 `hstatus`；helper 返回 0 或本次 trap 的 cause | 返回 0，即 HS-mode 读 hstatus 不触发异常（HS-mode 可访问 hstatus，norm:H_csrs_hs_not_vs） |
| FW-VIRT-UP-08 | VU→VS 后 VS-mode CSR 访问 | 先在 M-mode 清掉 `vsstatus.SUM`；经 VU→VS 后在 VS-mode 对 `sstatus` 置 SUM；回 M-mode 读 `vsstatus` | VS-mode 经 `sstatus` 地址的写入落到 `vsstatus.SUM`（norm:H_vscsrs_sub），且写入本身不触发异常 |

> [!NOTE]
> - VS→M 路径：VS-mode ecall（cause=10，GOTO_MMODE）→ HS handler（hedeleg[10] read-only-zero）→ 转发 M，handler 设 MPP=M 后 mret。
> - VS→HS 路径：VS-mode ecall（cause=10，GOTO_SMODE）→ HS handler **就地兑现**：清 `hstatus.SPV`、置 `sstatus.SPP=1` 后 `sret`，不经 M。
> - VU→VS 路径：VU-mode ecall（cause=8，GOTO_VSMODE）→ VS handler 无权 → 转发 HS → HS **就地兑现**：置 `hstatus.SPV=1`、`SPP=1` 后 `sret`，不经 M。
> - 本组断言不使用 `TEST_ASSERT(..., true)` 这类恒真式：“读取成功” 必须表达为
>   `PRIV_DO(...)` + `CHECK_NO_TRAP(...)`，或 helper 回报的 cause 等于 0，否则用例无法失败。
> - UP-08 的写入用 `trap_expect_begin()` / `trap_expect_end()` 手工开窗，不走 `PRIV_DO`：
>   `PRIV_DO(stmt)` 展开为 `(void)(stmt)`，而 `CSRS`/`CSRW`/`CSRC` 是 asm 语句不是表达式。

---

## Group 8. 虚拟化间接切换路径

**测试职责**：验证需要间接路由的虚拟化切换路径（从 S-mode 切换到 VS/VU，从 VS 切换到 VU）。需要 `ENABLE_HYP=1`。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FW-VIRT-IND-01 | S→VS 间接切换 | S-mode (HS-mode) 调用 `goto_priv(PRIV_VS)`，检查 `get_current_priv()` | 返回 `PRIV_VS` (5)；路径：ecall to M → mret with MPV=1,MPP=S |
| FW-VIRT-IND-02 | S→VU 间接切换 | S-mode 调用 `goto_priv(PRIV_VU)`，检查 `get_current_priv()` | 返回 `PRIV_VU` (4)；路径：ecall to M → mret with MPV=1,MPP=U |
| FW-VIRT-IND-03 | VS→VU 间接切换 | VS-mode 调用 `goto_priv(PRIV_VU)`，检查 `get_current_priv()` | 返回 `PRIV_VU` (4)；路径：`lower_priv()` → ecall to M → mret with MPV=1,MPP=U |
| FW-VIRT-IND-04 | S→VS 切换后 V=1 验证 | 先在 HS-mode 读 `hstatus` 作对照，再 S→VS 后在 VS-mode 读同一 CSR | HS-mode 读取不触发异常；VS-mode 读取被拒且 cause=22——两次访问之间只有 V 变了，这就是 V=1 的证据（norm:H_virtinst_vu_vs_nonhigh_allowedhs_tvm0） |
| FW-VIRT-IND-05 | S→VU 切换后 V=1 验证 | S→VU 后，验证处于 VU-mode | `get_current_priv()` = PRIV_VU，V=1 模式 |
| FW-VIRT-IND-06 | VS→VU 切换后 V=1 保持 | VS→VU 后，验证仍然处于 V=1 模式 | `get_current_priv()` = PRIV_VU，MPV 保持为 1 |
| FW-VIRT-IND-07 | 完整虚拟化往返 M→VS→M | M→VS→M 往返切换 | 最终 `get_current_priv()` = PRIV_M，V=0 |
| FW-VIRT-IND-08 | 完整虚拟化往返 M→VU→M | M→VU→M 往返切换 | 最终 `get_current_priv()` = PRIV_M，V=0 |
| FW-VIRT-IND-09 | 完整虚拟化往返 S→VS→S | S→VS→S 往返切换（HS-mode 进出虚拟化） | 最终 `get_current_priv()` = PRIV_S，V=0 |
| FW-VIRT-IND-10 | 完整虚拟化往返 VS→VU→VS | VS→VU→VS 往返切换 | 最终 `get_current_priv()` = PRIV_VS |

> [!NOTE]
> - S→VS/VU 与 VS→VU 的间接路径来自 `lower_priv()`：`priv_cmp` 把虚拟化目标归为“下降”，
>   而当前不在 M，于是先 `ecall(GOTO_MMODE)` 升到 M，再由 M 置 MPV=1 + MPP 后 mret。
>   注意这不是架构必然：HS 级 `tsbi_s_dispatch()` 能用 `hstatus.SPV` + `sret` 就地进 V=1，
>   只是 `goto_priv()` 从 S/VS 出发时不会选那条路（见开头“切换路径矩阵”下的 IMPORTANT）。
> - `goto_priv()` 中 `target > current_priv` 的判断用的是 `priv_cmp()` 而不是原始数值：
>   PRIV_VS(5) 相对 PRIV_M(3) 被判为**更低**（虚拟化级总是低于 M/HS），所以 M→VS 走 mret 下降路径。

---

## Group 9. run_in_priv() 虚拟化扩展

**测试职责**：验证 `run_in_priv()` 在虚拟化特权级的函数执行。需要 `ENABLE_HYP=1`。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FW-RUN-VIRT-01 | run_in_priv VS-mode 执行 | M-mode 调用 `run_in_priv(PRIV_VS, fn, arg)` | fn 在 VS-mode 执行，返回值正确 |
| FW-RUN-VIRT-02 | run_in_priv VU-mode 执行 | M-mode 调用 `run_in_priv(PRIV_VU, fn, arg)` | fn 在 VU-mode 执行，返回值正确 |
| FW-RUN-VIRT-03 | run_in_priv VS-mode 中验证特权级 | fn 内部调用 `get_current_priv()` | 返回 `PRIV_VS` (5) |
| FW-RUN-VIRT-04 | run_in_priv VU-mode 中验证特权级 | fn 内部调用 `get_current_priv()` | 返回 `PRIV_VU` (4) |
| FW-RUN-VIRT-05 | run_in_priv VS 执行后回到 M-mode | `run_in_priv(PRIV_VS, fn, arg)` 返回后检查 `get_current_priv()` | 返回 `PRIV_M` |
| FW-RUN-VIRT-06 | run_in_priv VU 执行后回到 M-mode | `run_in_priv(PRIV_VU, fn, arg)` 返回后检查 `get_current_priv()` | 返回 `PRIV_M` |

> [!NOTE]
> - `run_in_priv()` 的虚拟化路径：从 M-mode 设置 MPV=1 + MPP 后 mret 到 _run_trampoline，fn 执行后通过 ecall 回到 M-mode。
> - 从 S-mode 调用 `run_in_priv(PRIV_VS/PRIV_VU, ...)` 会先 ecall 到 M-mode，再从 M-mode 执行上述流程（代码中 `is_virt_target` 检测触发间接路径）。
> - 本组六个用例都在开头显式 `goto_priv(PRIV_M)` 建立 M-mode 调用方：`main()` 在 S-mode，
>   否则 FW-RUN-VIRT-05/06 断言的“回到 M-mode”会变成“回到 S-mode”。
> - 进入 VS/VU 执行 fn 不需要 G-stage 页表（同 Group 6 的说明），也不需要在 fn 内部做任何
>   翻译相关设置；fn 只需是普通 C 函数，返回 `uintptr_t`。

---

## Group 10. CSR 状态完整性

**测试职责**：验证特权级切换过程中 mstatus 关键字段的正确性和完整性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FW-CSR-01 | 切换不清除 mstatus.MIE | M-mode 设 MIE=1，M→S→M 往返后检查 MIE | MIE 保持为 1 |
| FW-CSR-02 | 切换不清除 mstatus.SIE | M-mode 设 SIE=1，M→S→M 往返后检查 SIE | SIE 保持为 1 |
| FW-CSR-03 | ecall handler 仅修改 MPP/MPV/MPRV | M-mode 设特定 mstatus 值（保留位），M→S→M 后检查保留位 | 保留位不变（ecall handler 仅修改 bits[12:11] MPP、bit[17] MPRV、bit[39] MPV） |
| FW-CSR-04 | M→S mret 后 mstatus.MPP 清零 | M→S 后在 S-mode 通过 ecall 回 M-mode，检查 MPP | MPP=0（mret 硬件清零 MPP） |
| FW-CSR-05 | M→U mret 后 mstatus.MPP 清零 | M→U 后在 U-mode 通过 ecall 回 M-mode，检查 MPP | MPP=0 |
| FW-CSR-06 | S→U sret 后 sstatus.SPP 清零 | S→U 后在 U-mode 通过 ecall 回 M-mode，检查 SPP | SPP=0（sret 硬件清零 SPP） |
| FW-CSR-07 | mtvec 在切换后不变 | 设 mtvec，M→S→M 后检查 mtvec | mtvec 值不变 |
| FW-CSR-08 | stvec 在切换后不变 | 设 stvec，M→S→M 后检查 stvec | stvec 值不变 |

> [!NOTE]
> - FW-CSR-03 验证 ecall handler 中的 `ms &= ~((3UL << 11) | (1UL << 17))` 仅清除 MPP 和 MPRV，不影响其他位。在 ENABLE_HYP 下还会修改 MPV (bit 39)。
> - FW-CSR-04/05/06 验证硬件 mret/sret 的 MPP/SPP 清零行为。RISC-V 规范要求 mret 将 MPP 清零为最低特权级（U=0），sret 将 SPP 清零为 U。

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1 (向下切换) | FW-DOWN-01~08 | 基本切换是所有测试的基础 |
| P0（必须） | Group 2 (向上切换) | FW-UP-01~07 | ecall 路径是特权级提升的唯一机制 |
| P0（必须） | Group 3 (No-op) | FW-NOOP-01~03 | 同特权级切换不应有副作用 |
| P0（必须） | Group 4 (往返切换) | FW-RT-01~06 | 状态一致性是框架可靠性的基础 |
| P1（重要） | Group 5 (run_in_priv) | FW-RUN-01~08 | 函数执行封装是测试用例的核心工具 |
| P1（重要） | Group 6 (虚拟化向下) | FW-VIRT-DOWN-01~09 | H-ext 测试依赖虚拟化切换 |
| P1（重要） | Group 7 (虚拟化向上) | FW-VIRT-UP-01~08 | 虚拟化 ecall 路径正确性 |
| P1（重要） | Group 10 (CSR 完整性) | FW-CSR-01~08 | 确保切换不破坏 CSR 状态 |
| P2（建议） | Group 8 (虚拟化间接) | FW-VIRT-IND-01~10 | 间接路由路径的正确性 |
| P2（建议） | Group 9 (run_in_priv 虚拟化) | FW-RUN-VIRT-01~06 | 虚拟化函数执行封装 |

---

## 测试实现说明

### 文件组织

```
damo-priv-test/
├── framework_test/
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_fw_down.c          # Group 1: 基本向下切换
│       ├── test_fw_up.c            # Group 2: 基本向上切换
│       ├── test_fw_noop.c          # Group 3: 同特权级切换
│       ├── test_fw_roundtrip.c     # Group 4: 往返切换
│       ├── test_fw_run.c           # Group 5: run_in_priv
│       ├── test_fw_virt_down.c     # Group 6: 虚拟化向下切换
│       ├── test_fw_virt_up.c       # Group 7: 虚拟化向上切换
│       ├── test_fw_virt_indirect.c # Group 8: 虚拟化间接切换
│       ├── test_fw_run_virt.c      # Group 9: run_in_priv 虚拟化
│       └── test_fw_csr.c           # Group 10: CSR 状态完整性
```

### 关键注意事项

1. **printf 与断言位置**：S-mode 常态下 UART 可访问（boot 期 PMP 已放开全空间），`TEST_BEGIN`/`TEST_ASSERT`
   在 S-mode 直接打印。但 U/VS/VU 侧不应依赖打印：用 `PRIV_DO` + `CHECK_*`，或把观测结果存进
   局部/全局变量，回 M-mode 再断言。

2. **get_current_priv() 的局限性**：`current_priv` 是软件变量，可能与硬件实际特权级不一致（如 bug 导致）。
   高可信度验证应结合硬件行为（如 CSR 访问权限、trap 入口快照的 MPP/MPV/SPV）进行交叉验证。

3. **虚拟化测试需要 ENABLE_HYP**：Group 6-9 的所有测试必须在编译时启用 `ENABLE_HYP=1`。
   H 扩展是否实现由平台配置在**编译期**判定（`rvtest_config.h` 的 `H_SUPPORTED` →
   `common/capabilities.h` 的 `H_AVAILABLE`），不做运行时探测。

4. **PMP 配置**：PMP 全空间 RWX 由 `common/entry.S` 在 boot 期（M-mode，降权之前）统一配置，
   `reset_state()` 不再碰 PMP；用例内不得自行写 `pmpcfg`/`pmpaddr`——那是 M 级 CSR，
   在 S-mode 常态下必然报 scause=2。

5. **ecall handler 的 MPP 设置**：服务 GOTO 请求的 handler 中 `ms |= ((uintptr_t)(target & 3) << 11)`
   使用 `target & 3` 提取名义特权级。对于虚拟化目标 `PRIV_VS=5 (101b)`，`5 & 3 = 1 = S`，正确设置 MPP=S。

6. **mret 后 MPP 清零行为**：RISC-V 规范要求 `mret` 后 MPP 被设为最低实现的特权级（通常为 U=0）。
   测试验证这一点时需要注意：mret 已经执行，MPP 已经是清零后的值——它**不能**用来反推
   切换前 MPP 被设成了什么，后者只能靠 Group 6 NOTE 里的 trap 入口快照观测。

7. **TEST_END() 的收尾特权级**：`TEST_END()` 先 `goto_priv(PRIV_M)` 让 `reset_state()` 取得 M 权限，
   `reset_state()` 结束前再 `goto_priv(PRIV_S)` 交还控制权，所以每个用例都以 **S-mode** 收尾，
   下一个用例的起点因此是 S 而不是 M；需要 M 起点的用例必须自己 `goto_priv(PRIV_M)`。

8. **委托优先基线**：框架在 boot 期与每次 `reset_state()` 应用 `tsbi_deleg_*_baseline()`（**不是**
   `medeleg=0`）：普通激励（含 illegal-instruction、page fault、ecall-from-U）被委托到 S/HS/VS
   就地处理，只有 `medeleg[9]`（ecall-from-S）恒为 0 以保住 T-SBI 服务通道。用例若要让某个
   cause 落到 M（例如观测 `mstatus.MPP/MPV` 的入口快照），必须先 `trap_undelegate_exc(cause)`，
   `reset_state()` 会在 `TEST_END()` 里恢复基线。

9. **cause 不得模糊匹配**：`CAUSE_ILLEGAL_INST`(2) 与 `CAUSE_VIRTUAL_INSTRUCTION`(22) 必须严格区分：
   V=1 下一个 HS-qualified 的访问被拒属于 22，报 2 意味着实现丢失了 V=1 的区分。所有断言必须
   明确写出预期 cause。

### 辅助函数

各文件内的 `static` helper，均为 `uintptr_t (*)(uintptr_t)` 签名（`run_in_priv` 要求）或本文件私有的观测封装：

```c
/* test_fw_down.c: S-mode 双向探针，返回 bit0=sstatus 可读 | bit1=mstatus 被拒 */
static uintptr_t s_mode_probe_fn(uintptr_t arg);

/* test_fw_up.c: M-mode 探针，返回 1 表示 mstatus 可读 */
static uintptr_t m_mode_probe_fn(uintptr_t arg);

/* test_fw_run.c / test_fw_run_virt.c: run_in_priv 返回值与特权级回报 */
static uintptr_t return_magic_fn(uintptr_t arg);   /* 0xDEADBEEF / 0xCAFEBABE */
static uintptr_t return_arg_fn(uintptr_t arg);
static uintptr_t get_priv_fn(uintptr_t arg);

/* test_fw_virt_down.c: 在 VS/VU 内取一次落在 M 的 illegal-instruction trap，
 * 以便回读 trap_get_mpp()/trap_get_mpv() 的入口快照。调用前需先在 M-mode
 * trap_undelegate_exc(CAUSE_ILLEGAL_INST)。 */
static bool fw_illegal_to_m(void);

/* test_fw_virt_up.c: 返回 0 表示 hstatus 读取未触发异常，否则返回 cause */
static uintptr_t read_hstatus_fn(uintptr_t arg);
```

---

## 测试判定标准

| 判定 | 条件 |
|------|------|
| PASS | `get_current_priv()` 返回预期值，CSR 状态符合 RISC-V 规范，函数执行结果正确 |
| FAIL | `get_current_priv()` 返回错误值，CSR 状态异常，预期 cause 不匹配，或切换过程中发生意外 trap |
| SKIP | **仅**用于编译期能力缺失（`ENABLE_HYP` 未置位时 Group 6-9 整体 SKIP）；不得以“需要页表/hgatp 设置”等可回避理由 SKIP |
| HALT | 切换过程中触发 UNEXPECTED TRAP（框架 halt），表明存在严重 bug |

---

## 覆盖率矩阵

| 框架组件 | 覆盖的测试 ID |
|---------|--------------|
| `goto_priv()` 向下路径 | FW-DOWN-01~08, FW-VIRT-DOWN-01~09, FW-VIRT-IND-01~03 |
| `goto_priv()` 向上路径 | FW-UP-01~07, FW-VIRT-UP-01~08 |
| `goto_priv()` no-op 路径 | FW-NOOP-01~03 |
| `lower_priv()` | FW-DOWN-01~08, FW-VIRT-DOWN-01~09 |
| `set_prev_priv()` M-mode | FW-DOWN-03~05, FW-VIRT-DOWN-03~06, FW-CSR-01~06 |
| `set_prev_priv()` S-mode | FW-CSR-06 |
| `do_ecall()` | FW-UP-01~07, FW-VIRT-UP-01~08 |
| `run_in_priv()` | FW-RUN-01~08, FW-RUN-VIRT-01~06 |
| `get_current_priv()` | 所有测试（每步验证） |
| `m_trap_handler()` ecall 路径 | FW-UP-01~04, FW-UP-06, FW-VIRT-UP-01~08, FW-VIRT-IND-01~03 |
| `s_trap_handler()` ecall 路径 | FW-UP-05, FW-UP-07 |
| `current_priv` 变量 | 所有测试 |
| `mstatus.MPP` 配置 | FW-DOWN-03~04, FW-VIRT-DOWN-04~05, FW-CSR-04~05 |
| `mstatus.MPV` 配置 | FW-DOWN-08, FW-VIRT-DOWN-03/06, FW-VIRT-IND-07 |
| `mstatus.MPRV` 清除 | FW-DOWN-05, FW-CSR-03 |
| `sstatus.SPP` 配置 | FW-CSR-06 |
| `trap_get_mpp()` / `trap_get_mpv()` 入口快照 | FW-VIRT-DOWN-03~06 |
| `trap_get_spv_snap()`（hstatus.SPV/SPVP） | FW-VIRT-DOWN-07 |
| `trap_undelegate_exc()` 与委托基线恢复 | FW-VIRT-DOWN-03~06 |
| VS CSR 替代规则（sstatus ↔ vsstatus） | FW-VIRT-DOWN-08, FW-VIRT-UP-08 |
| VS/VU 对 HS 级 CSR 的访问限制 | FW-VIRT-DOWN-07~09, FW-VIRT-IND-04, FW-VIRT-UP-07 |
| T-SBI GOTO 服务链路（S/U/VS/VU → M） | 所有向上切换用例，尤以 FW-UP-*, FW-VIRT-UP-*, FW-VIRT-IND-* |

---

## 参考

- `DOCS/framework/tsbi_design_brief.md` — T-SBI 协议与 S-mode 常态执行模型
- `DOCS/framework/tsbi_status.md` — T-SBI 代码分布与回归范围
- `common/privilege.c` — 特权级切换核心实现
- `common/trap.c` — M/HS 级 trap handler 与 T-SBI dispatch
- `common/hyp/hyp_vs_trap.c` — VS 级 handler
- `common/tsbi/tsbi.h` — 协议常量与调用方原语
- `common/tsbi/tsbi_deleg.c` — 委托优先基线
- `common/trap_asm.S` — trap 入口和返回汇编代码
- `common/test_framework.h` — 特权级常量定义和测试宏
- `common/test_framework.c` — `reset_state()` 和测试生命周期
- `common/encoding.h` — CSR 地址和 mstatus 位定义
- `common/entry.S` — 启动入口与 boot 期 PMP/向量/委托设置
- `common/platform_init.S` — 平台初始化
- `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — VS CSR 替代与 virtual-instruction 规则
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — mret 对 MPP/MPRV 的影响
