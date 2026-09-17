**中文 | [English](../testplan_en/Sstc_test_plan_en.md)**

# Sstc 扩展测试计划

本文档描述 Sstc（Supervisor-mode Timer Interrupts, Version 1.0）扩展的测试计划。Sstc 为 S-mode 提供独立的 CSR-based 定时器中断机制（`stimecmp` 寄存器），使 S-mode 可直接管理自身的定时器服务，无需通过 SBI 调用由 M-mode 代理。同时，该扩展为 Hypervisor 扩展的 VS-mode 提供了类似的定时器机制（`vstimecmp` 寄存器），并在 `menvcfg` 和 `henvcfg` 中新增 `STCE` 控制位。

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/sstc.adoc` — Sstc Extension for Supervisor-mode Timer Interrupts, Version 1.0
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — Supervisor Timer (`stimecmp`) Register 定义及 STIP 行为、`sip`.STIP / `sie`.STIE 在 Sstc 下的行为
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — `mip`.STIP 在 stimecmp 实现时的行为变化、`mcounteren`.TM 对 stimecmp/vstimecmp 的访问控制、`menvcfg`.STCE 字段定义
- `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — `hip`.VSTIP 与 vstimecmp 的关系、`henvcfg`.STCE 字段定义
- `SPEC/riscv-isa-manual/src/priv/csrs.adoc` — stimecmp/stimecmph/vstimecmp/vstimecmph CSR 地址

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内 src/priv/sstc.adoc、src/priv/supervisor.adoc、src/priv/machine.adoc、src/priv/hypervisor.adoc、src/priv/csrs.adoc）

---

## 测试范围

### 关键 CSR

| CSR | 地址 | 说明 |
|-----|------|------|
| `stimecmp` | 0x14D | S-level 定时器比较寄存器（64-bit） |
| `stimecmph` | 0x15D | stimecmp 高 32 位（RV32 only） |
| `vstimecmp` | 0x24D | VS-level 定时器比较寄存器（64-bit） |
| `vstimecmph` | 0x25D | vstimecmp 高 32 位（RV32 only） |
| `menvcfg` | 0x30A | Machine Environment Configuration，bit 63 = STCE |
| `henvcfg` | 0x60A | Hypervisor Environment Configuration，bit 63 = STCE |
| `mip` / `sip` | 0x344 / 0x144 | 中断 pending，bit 5 = STIP |
| `mie` / `sie` | 0x304 / 0x104 | 中断 enable，bit 5 = STIE |
| `mcounteren` | 0x306 | M-mode 计数器使能，bit 1 (TM) 控制 stimecmp 访问 |
| `hcounteren` | 0x606 | HS-mode 计数器使能，bit 1 (TM) 控制 vstimecmp 访问 |
| `time` | 0xC01 | 只读，mtime 的影子寄存器 |
| `hvip` | 0x645 | Hypervisor virtual interrupt pending，bit 6 = VSTIP |
| `hip` | 0x644 | Hypervisor interrupt pending，bit 6 = VSTIP = hvip.VSTIP OR vstimecmp 信号 |

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sstc_purpose` | This extension serves to provide supervisor mode with its own CSR-based timer interrupt facility that it can directly manage to provide its own timer service (in the form of having its own `stimecmp` register). | 此扩展旨在为 S-mode 提供其自己的基于 CSR 的定时器中断设施，使其能直接管理以提供自己的定时器服务（拥有自己的 `stimecmp` 寄存器）。 |
| `norm:stimecmp_exist` | This extension adds the S-level `stimecmp` CSR. | 此扩展添加了 S 级 `stimecmp` CSR。 |
| `norm:stce_bit_exist` | This extension adds the `STCE` bit to the `menvcfg` and `henvcfg` CSRs. | 此扩展向 `menvcfg` 和 `henvcfg` CSR 添加了 `STCE` 位。 |
| `norm:stimecmp_stimecmph_sz_acc` | The `stimecmp` CSR is a 64-bit register and has 64-bit precision on all RV32 and RV64 systems. In RV32 only, accesses to the `stimecmp` CSR access the low 32 bits, while accesses to the `stimecmph` CSR access the high 32 bits of `stimecmp`. | `stimecmp` 是 64 位寄存器，在 RV32 和 RV64 上均具有 64 位精度。仅 RV32 下：访问 `stimecmp` 访问低 32 位，访问 `stimecmph` 访问高 32 位。 |
| `norm:mip_sip_stip_op` | A supervisor timer interrupt becomes pending whenever `time` contains a value greater than or equal to `stimecmp`, treating the values as unsigned integers. If the result of this comparison changes, it is guaranteed to be reflected in STIP eventually, but not necessarily immediately. The interrupt remains posted until `stimecmp` becomes greater than `time`. | 当 `time` ≥ `stimecmp`（无符号比较）时 S 模式计时器中断变为待处理。比较结果变化保证最终反映在 STIP 中（不一定立即）。中断保持待处理直到 `stimecmp` > `time`。 |
| `norm:sip_stip_sie_stie` | Bits `sip`.STIP and `sie`.STIE are the interrupt-pending and interrupt-enable bits for supervisor-level timer interrupts. If implemented, STIP is read-only in `sip`. When Sstc is not implemented, STIP is set and cleared by the execution environment. When Sstc is implemented, STIP reflects the timer interrupt signal resulting from `stimecmp`. | STIP/STIE 是 S 级计时器中断的待处理/使能位。STIP 只读。未实现 Sstc 时由执行环境控制；实现 Sstc 时 STIP 反映 `stimecmp` 产生的计时器中断信号。 |
| `norm:mip_stip_stimecmp_acc` | If the `stimecmp` (supervisor-mode timer compare) register is implemented, STIP is read-only in mip. | 如果实现了 `stimecmp` 寄存器，STIP 在 `mip` 中为只读。 |
| `norm:mip_stip_stimecmp_op2` | STIP reflects the supervisor-level timer interrupt signal resulting from stimecmp. | STIP 反映由 `stimecmp` 产生的监管者级定时器中断信号。 |
| `norm:mip_stip_stimecmp_clr` | This timer interrupt signal is cleared by writing `stimecmp` with a value greater than the current time value. | 该定时器中断信号通过向 `stimecmp` 写入大于当前时间值的值来清除。 |
| `norm:mcounteren_tm_clr` | When the TM bit in the `mcounteren` register is clear, attempts to access the `stimecmp` or `vstimecmp` register while executing in a mode less privileged than M will cause an illegal-instruction exception. | 当 TM 位为 0 时，低于 M 模式的特权级访问 `stimecmp` 或 `vstimecmp` 会触发 illegal-instruction 异常。 |
| `norm:mcounteren_tm_set` | When this bit is set, access to the `stimecmp` or `vstimecmp` register is permitted in S-mode if implemented, and access to the `vstimecmp` register (via `stimecmp`) is permitted in VS-mode if implemented and not otherwise prevented by the TM bit in `hcounteren`. | 当 TM 位为 1 时，S-mode 允许访问 `stimecmp`；若 `hcounteren`.TM 未阻止，VS-mode 允许通过 `stimecmp` 访问 `vstimecmp`。 |
| `norm:menvcfg_stce_op1` | The Sstc extension adds the `STCE` (STimecmp Enable) bit to `menvcfg` CSR. | Sstc 扩展向 `menvcfg` CSR 添加 STCE（STimecmp Enable）位。 |
| `norm:menvcfg_stce_rdonly0` | When the Sstc extension is not implemented, `STCE` is read-only zero. | 如果未实现 Sstc 扩展，STCE 为只读零。 |
| `norm:menvcfg_stce_op2` | The `STCE` bit enables `stimecmp` for S-mode when set to one. When this extension is implemented and `STCE` in `menvcfg` is zero, an attempt to access `stimecmp` in a mode other than M-mode raises an illegal-instruction exception, `STCE` in `henvcfg` is read-only zero, and `STIP` in `mip` and `sip` reverts to its defined behavior as if this extension is not implemented. | 当 STCE=1 时使能 S-mode 使用 `stimecmp`。STCE=0 时非 M-mode 访问 `stimecmp` 触发 illegal-instruction 异常；`henvcfg`.STCE 变为只读零；`mip`/`sip` 中 STIP 恢复未实现 Sstc 时的行为。 |

> **已迁移的规范点**：`norm:sstc_vs_facility`、`norm:vstimecmp_exist`、`norm:hip_vstip_vstie_acc_op`、`norm:henvcfg_stce` 依赖 Hypervisor 扩展，已迁移至 `Hypervisor_cross_test_plan.md` Group 9。

### 不在测试范围内

- **RV32 / stimecmph / vstimecmph**：仅覆盖 RV64，RV32 的高 32 位分离访问不在本计划范围
- **多 hart 场景**：项目为单核测试环境
- **SBI 定时器接口兼容性**：仅验证 CSR 层面的行为，不测试 SBI 调用路径
- **time CSR 精确值验证**：不验证 time 递增的精确速率，仅验证 stimecmp 与 time 的比较逻辑
- **Spurious timer interrupt 处理**：规范允许 STIP 变化有延迟（"eventually, but not necessarily immediately"），测试中通过合理延迟和重试机制处理
- **Sv32 / Sv48 / Sv57**：仅覆盖 RV64 + Sv39，与项目其它扩展计划保持一致

---

## 前提与约束

### 定时器中断触发策略

由于 `time` 寄存器持续递增，测试使用以下策略可靠地触发和控制定时器中断：

1. **触发中断**：读取当前 `time` 值，将 `stimecmp` 设为 `time - 1`（或当前 time 的值），确保 `time >= stimecmp` 条件立即成立
2. **清除中断**：将 `stimecmp` 设为 `0xFFFFFFFF_FFFFFFFF`（最大值），确保 `stimecmp > time` 条件成立
3. **延迟等待**：规范允许 STIP 变化不一定立即反映，测试在设置 stimecmp 后执行若干空指令后再检查 STIP
4. **中断捕获**：通过 M-mode trap handler 或 S-mode trap handler 捕获 S-mode timer interrupt（scause bit 63 = 1, code = 5）

### menvcfg.STCE 使能前提

所有需要访问 `stimecmp` 的测试（Group 2–5），必须先在 M-mode 下将 `menvcfg.STCE` 设为 1，并将 `mcounteren.TM` 设为 1（当测试涉及 S-mode 访问时）。Group 1 和 Group 3 的部分用例会故意测试 STCE=0 或 TM=0 时的异常行为。

### VS-mode 测试前提 — 已迁移

> **[已迁移]** 原 Group 6（VS-mode 相关测试）已全部迁移至 `Hypervisor_cross_test_plan.md` Group 9。

---

## 设计要点

### 1. stimecmp 与 time 的比较语义

规范定义：当 `time` 的值大于或等于 `stimecmp` 的值时（均作为无符号整数），supervisor timer interrupt 变为 pending（STIP=1）。中断保持 posted 直到 `stimecmp` 变得大于 `time`。

关键点：
- 比较是**无符号**的
- STIP 变化保证**最终**反映，但**不一定立即**
- STIP 是**只读**的（Sstc 实现时），不能通过写 mip/sip 来清除
- 清除中断的唯一方式是写 `stimecmp` 使其大于当前 `time`

### 2. mip.STIP 只读性

当 Sstc 实现时（`menvcfg.STCE=1`），`mip` 中的 STIP bit 变为只读，反映 stimecmp 与 time 的比较结果。这与未实现 Sstc 时 STIP 在 mip 中可写的行为形成对比。

测试验证方法：在 M-mode 下尝试通过写 mip 置位 STIP，再读回验证 STIP 值仍由 stimecmp 与 time 的比较决定。

### 3. 访问控制层级

stimecmp 的访问受两层控制：

- M-mode 访问：始终允许
- S-mode 访问：需要 `menvcfg.STCE=1` 且 `mcounteren.TM=1`
- VS-mode 访问（via stimecmp）：需要 `menvcfg.STCE=1` 且 `mcounteren.TM=1` 且 `henvcfg.STCE=1` 且 `hcounteren.TM=1`

异常行为：
- `menvcfg.STCE=0` → S-mode 访问 stimecmp 产生 **illegal-instruction** 异常
- `mcounteren.TM=0` → S-mode 访问 stimecmp 产生 **illegal-instruction** 异常
- `henvcfg.STCE=0` → VS-mode (V=1) 访问 stimecmp（实际访问 vstimecmp）产生 **virtual-instruction** 异常

### 4. VSTIP 的合成逻辑（已迁移）

`hip`.VSTIP 是合成只读位，等于 `hvip.VSTIP` 与 vstimecmp 定时器信号的逻辑或。相关测试用例已迁移至 `Hypervisor_cross_test_plan.md` Group 9。

---

## 测试分组

### Group 1：menvcfg/henvcfg STCE 字段读写（M-mode CSR 读写）

> **[已迁移]** SSTC-STCE-03、SSTC-STCE-04 依赖 Hypervisor 扩展（henvcfg CSR），已迁移至 `Hypervisor_cross_test_plan.md` Group 9（HCROSS-SSTC-01、HCROSS-SSTC-02）。

**规范依据**：
- `norm:stce_bit_exist`：Sstc 新增 STCE bit 到 menvcfg 和 henvcfg
- `norm:menvcfg_stce_op1`：Sstc 新增 STCE bit 到 menvcfg
- `norm:menvcfg_stce_rdonly0`：Sstc 未实现时 STCE 为 read-only zero

**测试职责**：在 M-mode 下，验证 menvcfg 的 STCE bit 的读写行为。（henvcfg 部分已迁移至 Hypervisor_cross_test_plan.md Group 9）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSTC-STCE-01 | menvcfg.STCE 读写回环 | 在 M-mode 下写 menvcfg.STCE=1 后读回验证，再写 STCE=0 读回验证 | STCE bit 写入值与读回值一致 |
| SSTC-STCE-02 | menvcfg.STCE 不影响其他字段 | 写 menvcfg.STCE 前后，验证 menvcfg 的其他已设置字段不变 | 其他字段值保持不变 |
| SSTC-STCE-05 | menvcfg.STCE 初始值 | 复位后读取 menvcfg.STCE 的初始值 | STCE 初始值为 0（实现相关，验证可读） |

---

### Group 2：stimecmp CSR 读写（M-mode / S-mode）

**规范依据**：
- `norm:stimecmp_exist`：Sstc 新增 S-level stimecmp CSR
- `norm:stimecmp_stimecmph_sz_acc`：stimecmp 为 64-bit 精度

**测试职责**：验证 stimecmp CSR 的 64-bit 读写行为，包括在 M-mode 和 S-mode（STCE=1, TM=1）下的读写。

**测试前提**：menvcfg.STCE=1（M-mode 访问始终允许；S-mode 访问还需 mcounteren.TM=1）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSTC-RW-01 | M-mode stimecmp 全 1 读写 | M-mode 写 stimecmp=0xFFFFFFFF_FFFFFFFF 后读回 | 读回值等于写入值 |
| SSTC-RW-02 | M-mode stimecmp 全 0 读写 | M-mode 写 stimecmp=0 后读回 | 读回值为 0 |
| SSTC-RW-03 | M-mode stimecmp 交替位模式 | M-mode 写 stimecmp=0x5555555555555555 后读回，再写 0xAAAAAAAAAAAAAAAA 读回 | 读回值与写入值一致 |
| SSTC-RW-04 | M-mode stimecmp 高位验证 | M-mode 写 stimecmp 高 32 位为非零值（如 0x12345678_00000000）后读回 | 高 32 位正确保持 |
| SSTC-RW-05 | S-mode stimecmp 读写 | 设置 STCE=1、TM=1，切换到 S-mode 写 stimecmp 后回到 M-mode 读回验证 | S-mode 写入值在 M-mode 读回一致 |
| SSTC-RW-06 | stimecmp 写入不影响 time | 写 stimecmp 前后读取 time，验证 time 单调递增且不受 stimecmp 影响 | time 持续递增，与 stimecmp 无关 |

---

### Group 3：访问控制

> **[已迁移]** SSTC-ACC-08、SSTC-ACC-09、SSTC-ACC-10 依赖 Hypervisor 扩展（VS-mode 访问控制），已迁移至 `Hypervisor_cross_test_plan.md` Group 9（HCROSS-SSTC-03、HCROSS-SSTC-04、HCROSS-SSTC-05）。

**规范依据**：
- `norm:menvcfg_stce_op2`：STCE=0 时非 M-mode 访问 stimecmp 产生 illegal-instruction 异常
- `norm:mcounteren_tm_clr`：mcounteren.TM=0 时，低特权模式访问 stimecmp 产生 illegal-instruction 异常
- `norm:mcounteren_tm_set`：mcounteren.TM=1 时，S-mode 允许访问 stimecmp

**测试职责**：验证 stimecmp 的多层访问控制机制。（VS-mode / vstimecmp 部分已迁移至 Hypervisor_cross_test_plan.md Group 9）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSTC-ACC-01 | STCE=0 时 S-mode 读 stimecmp | menvcfg.STCE=0，S-mode 读 stimecmp | 触发 illegal-instruction 异常（cause=2） |
| SSTC-ACC-02 | STCE=0 时 S-mode 写 stimecmp | menvcfg.STCE=0，S-mode 写 stimecmp | 触发 illegal-instruction 异常（cause=2） |
| SSTC-ACC-03 | STCE=1、TM=0 时 S-mode 读 stimecmp | menvcfg.STCE=1、mcounteren.TM=0，S-mode 读 stimecmp | 触发 illegal-instruction 异常（cause=2） |
| SSTC-ACC-04 | STCE=1、TM=0 时 S-mode 写 stimecmp | menvcfg.STCE=1、mcounteren.TM=0，S-mode 写 stimecmp | 触发 illegal-instruction 异常（cause=2） |
| SSTC-ACC-05 | STCE=1、TM=1 时 S-mode 读 stimecmp | menvcfg.STCE=1、mcounteren.TM=1，S-mode 读 stimecmp | 无异常，成功读取 |
| SSTC-ACC-06 | STCE=1、TM=1 时 S-mode 写 stimecmp | menvcfg.STCE=1、mcounteren.TM=1，S-mode 写 stimecmp 后 M-mode 读回 | 无异常，读回值一致 |
| SSTC-ACC-07 | M-mode 始终可访问 stimecmp | menvcfg.STCE=0 时，M-mode 读写 stimecmp | 无异常，读写正常 |

---

### Group 4：定时器中断生成

**规范依据**：
- `norm:mip_sip_stip_op`：time >= stimecmp 时 STIP 置位，stimecmp > time 时清除
- `norm:mip_stip_stimecmp_op2`：STIP 反映 stimecmp 产生的定时器中断信号
- `norm:mip_stip_stimecmp_clr`：写 stimecmp 大于当前 time 可清除定时器中断信号
- `norm:sstc_purpose`：Sstc 提供 S-mode 自有的定时器中断机制

**测试职责**：验证 stimecmp 与 time 的比较逻辑，以及 S-mode timer interrupt 的生成和清除。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSTC-TMR-01 | stimecmp <= time 时 STIP 置位 | 读 time，写 stimecmp = time - 1，延迟后检查 mip.STIP | STIP = 1 |
| SSTC-TMR-02 | stimecmp > time 时 STIP 清除 | 先触发 STIP=1，再写 stimecmp = 0xFFFFFFFF_FFFFFFFF，延迟后检查 mip.STIP | STIP = 0 |
| SSTC-TMR-03 | stimecmp = time 边界值 | 读 time 并立即写 stimecmp = time，延迟后检查 STIP（由于 time 持续递增，stimecmp 应很快 <= time） | STIP = 1 |
| SSTC-TMR-04 | STIP 在 sip 中可读 | 触发 STIP=1 后，从 sip 读取 STIP bit | sip.STIP = 1 |
| SSTC-TMR-05 | S-mode timer interrupt 捕获（M-mode handler） | 使能 STCE=1，不委托 STI（mideleg.STIP=0），设置 mie.STIE=1 和 mstatus.MIE=1，设 stimecmp 为过去值，验证 M-mode trap handler 捕获 STI | trap cause = interrupt \| 5 |
| SSTC-TMR-06 | S-mode timer interrupt 委托到 S-mode | 设置 mideleg bit 5 = 1，使能 sie.STIE、sstatus.SIE，设 stimecmp 为过去值，切换到 S-mode | S-mode trap handler 捕获 cause = interrupt \| 5 |
| SSTC-TMR-07 | STIE=0 时不产生中断 | 清除 mie.STIE / sie.STIE，设 stimecmp 为过去值，验证不发生中断（仅 STIP 置位） | STIP = 1，但无中断产生 |
| SSTC-TMR-08 | 清除中断后再次触发 | 先触发 STIP=1，写 stimecmp 最大值清除，验证 STIP=0；再次设 stimecmp 为过去值 | 第二次 STIP 再次置位 |
| SSTC-TMR-09 | stimecmp 写入后中断立即可清除 | 触发中断后，在 trap handler 中写 stimecmp 为最大值，验证从 handler 正常返回 | trap handler 成功清除中断并返回 |
| SSTC-TMR-10 | 无符号比较语义 | 写 stimecmp = 0x8000000000000000（最大有符号负数），当 time 远小于此值时验证 STIP=0 | STIP = 0（无符号比较：time < stimecmp） |

> [!NOTE]
> SSTC-TMR-06 需要在 S-mode 安装本地 trap entry：入口需保存 scause 到全局变量，写 stimecmp 为最大值以清除中断源（防止重入），按压缩指令宽度推进 sepc 后 sret 返回；`stvec.MODE=Direct` 时 BASE 指向该入口。

---

### Group 5：STIP 只读行为

**规范依据**：
- `norm:mip_stip_stimecmp_acc`：stimecmp 实现时，STIP 在 mip 中只读
- `norm:sip_stip_sie_stie`：Sstc 实现时 STIP 只读，反映 stimecmp 比较结果

**测试职责**：验证当 Sstc 实现（STCE=1）时，mip/sip 中的 STIP bit 变为只读，只能通过修改 stimecmp 来控制。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSTC-STIP-01 | mip.STIP 写入被忽略（STIP=0 时写 1） | STCE=1，stimecmp 设为最大值（STIP=0），尝试写 mip.STIP=1，读回 mip.STIP | STIP 仍为 0（写入被忽略） |
| SSTC-STIP-02 | mip.STIP 写入被忽略（STIP=1 时写 0） | STCE=1，stimecmp 设为过去值（STIP=1），尝试写 mip.STIP=0，读回 mip.STIP | STIP 仍为 1（写入被忽略） |
| SSTC-STIP-03 | sip.STIP 只读性 | STCE=1、TM=1，切换 S-mode 尝试写 sip.STIP，回到 M-mode 读 mip.STIP | STIP 值仍由 stimecmp 与 time 比较决定 |
| SSTC-STIP-04 | STIP 仅由 stimecmp 控制 | 交替设置 stimecmp 为过去值和最大值，验证 STIP 跟随变化 | STIP 精确跟随 stimecmp 与 time 的比较结果 |
| SSTC-STIP-05 | STCE=0 时 STIP 恢复可写 | menvcfg.STCE=0 后，尝试写 mip.STIP=1，验证 STIP 可被软件置位 | STIP = 1（恢复 M-mode 可写行为） |

---

## 关键注意事项

1. **扩展检测**：所有测试必须在运行时检测 Sstc 的可用性（通过 `menvcfg.STCE` 可写性），不可用时 TEST_SKIP。

2. **STIP 时序容忍**：`norm:mip_sip_stip_op` 明确 STIP 变化"eventually, but not necessarily immediately"。测试中通过合理延迟（若干空指令 + 内存屏障）后再检查 STIP，避免因时序问题误判。

3. **中断重入防护**：S-mode timer interrupt handler 中必须先写 `stimecmp` 为最大值以清除中断源，再执行 `sret`，否则会立即重入。

4. **mideleg 配置**：SSTC-TMR-05（M-mode 捕获）要求 mideleg bit 5 = 0；SSTC-TMR-06（S-mode 捕获）要求 mideleg bit 5 = 1。测试前后需保存/恢复 mideleg。

5. **Hypervisor 测试**：需要 Hypervisor 扩展的测试（`henvcfg.STCE`、`vstimecmp`、`hip.VSTIP` 等）已迁移至 `Hypervisor_cross_test_plan.md` Group 9。

任一平台违反 SPEC 时用例保持 FAIL，实现缺陷记录至 `bugs/` 目录。

---

## 附录 A：规范点覆盖矩阵

| Norm ID | 覆盖的测试 ID | 覆盖状态 | 备注 |
|---------|--------------|----------|------|
| `norm:sstc_purpose` | SSTC-TMR-01 ~ SSTC-TMR-10 | 已覆盖 | S-mode 自有定时器中断设施 |
| `norm:stimecmp_exist` | SSTC-RW-01 ~ SSTC-RW-06、SSTC-ACC-05 ~ SSTC-ACC-07 | 已覆盖 | stimecmp CSR 存在性 |
| `norm:stce_bit_exist` | SSTC-STCE-01、SSTC-STCE-02、SSTC-STCE-05 | 已覆盖 | menvcfg.STCE 位存在 |
| `norm:stimecmp_stimecmph_sz_acc` | SSTC-RW-01 ~ SSTC-RW-04 | 已覆盖 | 64-bit 精度读写 |
| `norm:mip_sip_stip_op` | SSTC-TMR-01 ~ SSTC-TMR-04、SSTC-TMR-08、SSTC-TMR-10、SSTC-STIP-04 | 已覆盖 | STIP 与 time/stimecmp 比较关系 |
| `norm:sip_stip_sie_stie` | SSTC-TMR-04、SSTC-TMR-06、SSTC-STIP-03 | 已覆盖 | STIP 只读、STIE 使能语义 |
| `norm:mip_stip_stimecmp_acc` | SSTC-STIP-01、SSTC-STIP-02、SSTC-STIP-05 | 已覆盖 | mip.STIP 只读性 |
| `norm:mip_stip_stimecmp_op2` | SSTC-STIP-01、SSTC-STIP-02、SSTC-STIP-04 | 已覆盖 | STIP 反映 stimecmp 信号 |
| `norm:mip_stip_stimecmp_clr` | SSTC-TMR-02、SSTC-TMR-08、SSTC-TMR-09 | 已覆盖 | 写 stimecmp 清除中断信号 |
| `norm:mcounteren_tm_clr` | SSTC-ACC-03、SSTC-ACC-04 | 已覆盖 | TM=0 时访问非法 |
| `norm:mcounteren_tm_set` | SSTC-ACC-05、SSTC-ACC-06、SSTC-RW-05 | 已覆盖 | TM=1 时 S-mode 允许访问 |
| `norm:menvcfg_stce_op1` | SSTC-STCE-01、SSTC-STCE-02 | 已覆盖 | menvcfg.STCE 位定义 |
| `norm:menvcfg_stce_rdonly0` | SSTC-STCE-05 | 已覆盖 | 未实现 Sstc 时 STCE 只读零（通过扩展探测覆盖） |
| `norm:menvcfg_stce_op2` | SSTC-ACC-01、SSTC-ACC-02、SSTC-STIP-05 | 已覆盖 | STCE=0 时非法访问与 STIP 行为恢复 |

**未覆盖规范点说明**：
- Hypervisor 相关规范点（`norm:sstc_vs_facility`、`norm:vstimecmp_exist`、`norm:hip_vstip_vstie_acc_op`、`norm:henvcfg_stce`）已迁移至 `Hypervisor_cross_test_plan.md` Group 9，本方案不重复覆盖。
