# 测试框架特权态切换功能测试计划

> 本文档描述对 `common/` 目录中测试框架的特权态切换功能进行验证的测试计划。测试覆盖 `goto_priv()`、`get_current_priv()`、`run_in_priv()` 等核心 API，以及底层 `mret`/`sret`/`ecall` 机制在所有特权级（M/S/U/VS/VU）之间的切换行为。
>
> 生成时间：2026-06-26

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

本测试计划基于 `common/privilege.c` 和 `common/trap.c` 中的实现：

| 组件 | 文件 | 描述 |
|------|------|------|
| `goto_priv()` | `privilege.c:164-175` | 双向特权级切换入口，向上用 ecall，向下用 mret/sret |
| `lower_priv()` | `privilege.c:94-134` | 向下切换，配置 MPP/SPP 后执行 mret/sret |
| `set_prev_priv()` | `privilege.c:49-86` | 配置 mstatus.MPP/MPV 或 sstatus.SPP |
| `do_ecall()` | `privilege.c:139-143` | 通过 ecall 陷入 M-mode handler |
| `run_in_priv()` | `privilege.c:199-274` | 在目标特权级执行函数 |
| `get_current_priv()` | `privilege.c:40-42` | 返回 `current_priv` 软件跟踪变量 |
| `m_trap_handler()` | `trap.c:220-379` | M-mode trap handler，处理 ecall 切换 |
| `s_trap_handler()` | `trap.c:386-473` | S-mode trap handler，处理 ecall 切换 |

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
向下切换（mret/sret 路径）：
  M  -> S:    mret (MPP=S, MPV=0)
  M  -> U:    mret (MPP=U, MPV=0)
  S  -> U:    sret (SPP=U)
  M  -> VS:   mret (MPP=S, MPV=1)
  M  -> VU:   mret (MPP=U, MPV=1)

向上切换（ecall 路径）：
  U  -> S:    ecall -> S-mode handler -> goto_priv()
  U  -> M:    ecall -> M-mode handler
  S  -> M:    ecall -> M-mode handler
  VS -> M:    ecall -> M-mode handler (MPV cleared)
  VS -> HS:   ecall -> M-mode handler (MPV=0, MPP=S)
  VU -> VS:   ecall -> M-mode handler (MPV=1, MPP=S)
  VU -> M:    ecall -> M-mode handler (MPV=0, MPP=M)

虚拟化目标的间接路径：
  S  -> VS:   ecall to M -> mret (MPV=1, MPP=S)
  S  -> VU:   ecall to M -> mret (MPV=1, MPP=U)
  VS -> VU:   ecall to M -> mret (MPV=1, MPP=U)
```

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
| FW-DOWN-06 | M→S 切换后 S-mode 执行验证 | M-mode 调用 `goto_priv(PRIV_S)`，在 S-mode 读取 `sstatus` CSR | 读取成功，无异常（证明确实在 S-mode 执行） |
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
| FW-UP-03 | S→M ecall 路径验证 | S-mode 调用 `goto_priv(PRIV_M)`，验证通过 ecall 陷入 M-mode handler | M-mode trap handler 收到 ecall-from-S (cause=9)，handler 设置 MPP=M 后 mret 回到 M-mode |
| FW-UP-04 | U→M ecall 路径验证 | U-mode 调用 `goto_priv(PRIV_M)`，验证通过 ecall 陷入 M-mode handler | M-mode trap handler 收到 ecall-from-U (cause=8) |
| FW-UP-05 | U→S ecall 路径验证 | U-mode 调用 `goto_priv(PRIV_S)`，验证通过 ecall 进入 S-mode handler | S-mode trap handler 收到 ecall-from-U (cause=8)，S-mode handler 调用 `goto_priv(PRIV_S)` |
| FW-UP-06 | S→M 切换后 M-mode CSR 可访问 | S-mode 调用 `goto_priv(PRIV_M)` 后，读取 `mstatus` CSR | 读取成功（证明确实在 M-mode） |
| FW-UP-07 | U→S 切换后 S-mode CSR 可访问 | U-mode 调用 `goto_priv(PRIV_S)` 后，读取 `sstatus` CSR | 读取成功（证明确实在 S-mode） |

> [!NOTE]
> - 向上切换始终通过 ecall 路径。`do_ecall()` 设置 `ecall_args[0] = ECALL_GOTO_PRIV (1)`, `ecall_args[1] = target`，然后执行 `ecall` 指令。
> - M-mode trap handler 识别 ecall 后，设置 `mstatus.MPP` 为目标特权级，`mepc` 推进到 ecall 后一条指令，然后通过 `mret` 返回。
> - FW-UP-05 验证 U→S 的路径：U-mode ecall 陷入 M-mode handler，M-mode handler 设置 MPP=S 后 mret 到 S-mode。

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
| FW-RUN-05 | run_in_priv 执行后回到 M-mode | `run_in_priv(PRIV_S, fn, arg)` 返回后，检查 `get_current_priv()` | 返回 `PRIV_M`（run_in_priv 自动回到 M-mode） |
| FW-RUN-06 | run_in_priv 参数传递 | fn 接收 arg 参数并原样返回 | 返回值与传入的 arg 一致 |
| FW-RUN-07 | run_in_priv M-mode 直接调用 | 在 M-mode 调用 `run_in_priv(PRIV_M, fn, arg)` | fn 直接在 M-mode 执行，返回值正确（不走 mret 路径） |
| FW-RUN-08 | run_in_priv 同特权级直接调用 | 在 S-mode 调用 `run_in_priv(PRIV_S, fn, arg)` | fn 直接执行（priv >= current_priv 走直接调用路径） |

> [!NOTE]
> - `run_in_priv()` 内部使用 `_run_trampoline`：在目标特权级执行 fn(arg) 后，通过 ecall 回到 M-mode。
> - FW-RUN-07/08 验证 `run_in_priv()` 中 `priv >= current_priv` 时直接调用 fn 的路径（不走 mret/sret）。

---

## Group 6. 虚拟化向下切换（M-mode → VS/VU-mode）

**测试职责**：验证从 M-mode 通过 `goto_priv()` 切换到虚拟化特权级（VS/VU）的正确性。需要 `ENABLE_HYP=1`。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FW-VIRT-DOWN-01 | M→VS 切换后 current_priv | M-mode 调用 `goto_priv(PRIV_VS)`，检查 `get_current_priv()` | 返回 `PRIV_VS` (5) |
| FW-VIRT-DOWN-02 | M→VU 切换后 current_priv | M-mode 调用 `goto_priv(PRIV_VU)`，检查 `get_current_priv()` | 返回 `PRIV_VU` (4) |
| FW-VIRT-DOWN-03 | M→VS mstatus.MPV=1 验证 | M-mode 调用 `goto_priv(PRIV_VS)`，验证切换过程中 MPV 被设为 1 | MPV=1（mret 前配置），mret 后进入 V=1 模式 |
| FW-VIRT-DOWN-04 | M→VS mstatus.MPP=S 验证 | M-mode 调用 `goto_priv(PRIV_VS)`，验证 MPP 设为 S | MPP=S (01)（`target & 3 = 5 & 3 = 1 = S`） |
| FW-VIRT-DOWN-05 | M→VU mstatus.MPP=U 验证 | M-mode 调用 `goto_priv(PRIV_VU)`，验证 MPP 设为 U | MPP=U (00)（`target & 3 = 4 & 3 = 0 = U`） |
| FW-VIRT-DOWN-06 | M→VU mstatus.MPV=1 验证 | M-mode 调用 `goto_priv(PRIV_VU)`，验证 MPV 被设为 1 | MPV=1 |
| FW-VIRT-DOWN-07 | M→VS 后 hstatus.SPV 验证 | M→VS 切换后，在 VS-mode 读取 hstatus.SPV | SPV=1（硬件在 V=1 trap 时设置 SPV） |
| FW-VIRT-DOWN-08 | M→VS 后 VS-mode CSR 访问 | M→VS 切换后，在 VS-mode 读取 vsstatus (sstatus 的虚拟化视图) | 读取成功，无 virtual-instruction 异常 |
| FW-VIRT-DOWN-09 | M→VU 后 VU-mode 限制验证 | M→VU 切换后，在 VU-mode 尝试读取 sstatus | 触发 virtual-instruction 或 illegal-instruction 异常（VU-mode 不可访问 S-mode CSR） |

> [!NOTE]
> - 虚拟化目标的编码：`target & 3` 提取名义特权级（MPP），`target & 0x4` 检测 V=1 标记。`PRIV_VS=5 (101b)` → MPP=S, V=1；`PRIV_VU=4 (100b)` → MPP=U, V=1。
> - `set_prev_priv()` 中 M-mode 路径：先清除 MPP 和 MPV，然后设置 `MPP = target & 3`，若 `target & 0x4` 则设置 MPV=1。
> - FW-VIRT-DOWN-03/04/05/06 验证的是 `set_prev_priv()` 在 `mret` 前对 mstatus 的配置。由于 `mret` 后 MPP 被硬件清零，需要通过间接方式验证（如回到 M-mode 后检查 trap record 或 CSRR 快照）。

---

## Group 7. 虚拟化向上切换（VS/VU → M/HS-mode）

**测试职责**：验证从虚拟化特权级向上切换到高特权级的正确性。需要 `ENABLE_HYP=1`。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FW-VIRT-UP-01 | VS→M 切换后 current_priv | VS-mode 调用 `goto_priv(PRIV_M)`，检查 `get_current_priv()` | 返回 `PRIV_M` (3) |
| FW-VIRT-UP-02 | VU→M 切换后 current_priv | VU-mode 调用 `goto_priv(PRIV_M)`，检查 `get_current_priv()` | 返回 `PRIV_M` (3) |
| FW-VIRT-UP-03 | VS→HS (PRIV_S) 切换后 current_priv | VS-mode 调用 `goto_priv(PRIV_S)`，检查 `get_current_priv()` | 返回 `PRIV_S` (1)（退出虚拟化，V=0） |
| FW-VIRT-UP-04 | VU→VS 切换后 current_priv | VU-mode 调用 `goto_priv(PRIV_VS)`，检查 `get_current_priv()` | 返回 `PRIV_VS` (5)（保持 V=1，名义特权级提升到 S） |
| FW-VIRT-UP-05 | VS→M ecall 路径验证 | VS-mode 调用 `goto_priv(PRIV_M)`，验证 ecall 陷入 M-mode handler | M-mode handler 收到 ecall-from-VS (cause=10)，MPV 被清除 |
| FW-VIRT-UP-06 | VS→M 后 M-mode 可访问 H-ext CSR | VS→M 后，在 M-mode 读取 hstatus CSR | 读取成功（证明确实在 M-mode 且 V=0） |
| FW-VIRT-UP-07 | VS→HS 后 S-mode 行为验证 | VS→HS 后，在 S-mode (HS-mode) 读取 hstatus CSR | 读取成功（HS-mode 可访问 hstatus） |
| FW-VIRT-UP-08 | VU→VS 后 VS-mode CSR 访问 | VU→VS 后，在 VS-mode 读取 vsstatus | 读取成功（VS-mode 可访问虚拟化 S-mode CSR） |

> [!NOTE]
> - VS→M 路径：VS-mode ecall → M-mode handler（cause=10, ecall-from-VS），handler 清除 MPV 并设 MPP=M，mret 回到 M-mode。
> - VS→HS 路径：VS-mode ecall → M-mode handler，handler 清除 MPV 并设 MPP=S，mret 回到 HS-mode (V=0, S-mode)。
> - VU→VS 路径：VU-mode ecall → M-mode handler，handler 保持 MPV=1 并设 MPP=S，mret 回到 VS-mode。

---

## Group 8. 虚拟化间接切换路径

**测试职责**：验证需要间接路由的虚拟化切换路径（从 S-mode 切换到 VS/VU，从 VS 切换到 VU）。需要 `ENABLE_HYP=1`。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FW-VIRT-IND-01 | S→VS 间接切换 | S-mode (HS-mode) 调用 `goto_priv(PRIV_VS)`，检查 `get_current_priv()` | 返回 `PRIV_VS` (5)；路径：ecall to M → mret with MPV=1,MPP=S |
| FW-VIRT-IND-02 | S→VU 间接切换 | S-mode 调用 `goto_priv(PRIV_VU)`，检查 `get_current_priv()` | 返回 `PRIV_VU` (4)；路径：ecall to M → mret with MPV=1,MPP=U |
| FW-VIRT-IND-03 | VS→VU 间接切换 | VS-mode 调用 `goto_priv(PRIV_VU)`，检查 `get_current_priv()` | 返回 `PRIV_VU` (4)；路径：`lower_priv()` → ecall to M → mret with MPV=1,MPP=U |
| FW-VIRT-IND-04 | S→VS 切换后 V=1 验证 | S→VS 后，在 VS-mode 检查 hstatus.SPV 或尝试访问虚拟化 CSR | 确认处于 V=1 模式 |
| FW-VIRT-IND-05 | S→VU 切换后 V=1 验证 | S→VU 后，验证处于 VU-mode | `get_current_priv()` = PRIV_VU，V=1 模式 |
| FW-VIRT-IND-06 | VS→VU 切换后 V=1 保持 | VS→VU 后，验证仍然处于 V=1 模式 | `get_current_priv()` = PRIV_VU，MPV 保持为 1 |
| FW-VIRT-IND-07 | 完整虚拟化往返 M→VS→M | M→VS→M 往返切换 | 最终 `get_current_priv()` = PRIV_M，V=0 |
| FW-VIRT-IND-08 | 完整虚拟化往返 M→VU→M | M→VU→M 往返切换 | 最终 `get_current_priv()` = PRIV_M，V=0 |
| FW-VIRT-IND-09 | 完整虚拟化往返 S→VS→S | S→VS→S 往返切换（HS-mode 进出虚拟化） | 最终 `get_current_priv()` = PRIV_S，V=0 |
| FW-VIRT-IND-10 | 完整虚拟化往返 VS→VU→VS | VS→VU→VS 往返切换 | 最终 `get_current_priv()` = PRIV_VS |

> [!NOTE]
> - S→VS/VU 的间接路径：由于 `sret` 不检查 MPV 位，只有 `mret` 能进入 V=1 模式。因此从 S-mode 切换到虚拟化目标需要先 ecall 到 M-mode，再由 M-mode 设置 MPV=1 后 mret。
> - VS→VU 的间接路径：`lower_priv()` 发现当前为 VS 且目标为 VU（虚拟化的更低特权级），先 ecall 到 M-mode，然后从 M-mode 设置 MPV=1,MPP=U 后 mret。
> - `goto_priv()` 中 `target > current_priv` 判断使用原始数值比较：PRIV_VS(5) > PRIV_M(3)，因此 M→VS 走 ecall 路径（正确，因为需要在 M-mode handler 中设置 MPV）。

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

1. **S-mode/U-mode 中无法 printf**：在 S-mode 和 U-mode 中，UART 不可访问。需使用 `PRIV_DO` + `CHECK_*` 模式，或在目标特权级存储结果到全局变量后回到 M-mode 断言。

2. **get_current_priv() 的局限性**：`current_priv` 是软件变量，可能与硬件实际特权级不一致（如 bug 导致）。高可信度验证应结合硬件行为（如 CSR 访问权限）进行交叉验证。

3. **虚拟化测试需要 ENABLE_HYP**：Group 6-9 的所有测试必须在编译时启用 `ENABLE_HYP=1`，并在运行时检测 H 扩展可用性。

4. **PMP 配置**：切换到 S-mode 或 U-mode 前需要确保 PMP 允许目标模式访问代码和数据区域。框架的 `reset_state()` 会配置 PMP，但测试中可能需要额外配置。

5. **ecall handler 的 MPP 设置**：M-mode ecall handler 中 `ms |= ((uintptr_t)(target & 3) << 11)` 使用 `target & 3` 提取名义特权级。对于虚拟化目标 `PRIV_VS=5 (101b)`，`5 & 3 = 1 = S`，正确设置 MPP=S。

6. **mret 后 MPP 清零行为**：RISC-V 规范要求 `mret` 后 MPP 被设为最低实现的特权级（通常为 U=0）。测试验证这一点时需要注意：mret 已经执行，MPP 已经是清零后的值。

7. **TEST_END() 自动回到 M-mode**：`TEST_END()` 宏会调用 `goto_priv(PRIV_M)` 和 `reset_state()`。如果测试用例结束时不在 M-mode，`TEST_END()` 会自动处理。

8. **trap handler 的 medeleg/mideleg**：框架默认 `medeleg=0, mideleg=0`（所有 trap 到 M-mode）。测试 ecall 时需注意 ecall 异常不被委托到低特权级 handler。

### 辅助函数

```c
/* Check if running in expected privilege by attempting CSR access */
static bool verify_in_m_mode(void) {
    /* M-mode can access mstatus */
    trap_expect_begin();
    CSRR(mstatus);
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

static uintptr_t read_sstatus_fn(uintptr_t arg) {
    (void)arg;
    return CSRR(sstatus);
}

static uintptr_t get_priv_fn(uintptr_t arg) {
    (void)arg;
    return (uintptr_t)get_current_priv();
}

static uintptr_t return_arg_fn(uintptr_t arg) {
    return arg;
}

static uintptr_t return_magic_fn(uintptr_t arg) {
    (void)arg;
    return 0xDEADBEEF;
}
```

---

## 测试判定标准

| 判定 | 条件 |
|------|------|
| PASS | `get_current_priv()` 返回预期值，CSR 状态符合 RISC-V 规范，函数执行结果正确 |
| FAIL | `get_current_priv()` 返回错误值，CSR 状态异常，或切换过程中发生意外 trap |
| SKIP | 所需扩展未实现（如 H 扩展不可用时 Group 6-9 SKIP） |
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
| `mstatus.MPV` 配置 | FW-DOWN-08, FW-VIRT-DOWN-03~06, FW-VIRT-UP-05 |
| `mstatus.MPRV` 清除 | FW-DOWN-05, FW-CSR-03 |
| `sstatus.SPP` 配置 | FW-CSR-06 |

---

## 参考

- `common/privilege.c` — 特权级切换核心实现
- `common/trap.c` — M-mode/S-mode trap handler
- `common/trap_asm.S` — trap 入口和返回汇编代码
- `common/test_framework.h` — 特权级常量定义和测试宏
- `common/test_framework.c` — `reset_state()` 和测试生命周期
- `common/encoding.h` — CSR 地址和 mstatus 位定义
- `common/entry.S` — 启动入口
- `common/platform_init.S` — 平台初始化
