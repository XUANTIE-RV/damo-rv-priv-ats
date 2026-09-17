**中文 | [English](../testplan_en/Sscofpmf_test_plan_en.md)**

# Sscofpmf 扩展测试计划

本文档描述 Sscofpmf（Count Overflow and Privilege Mode Filtering, Version 1.0）扩展的测试计划。Sscofpmf 为 `mhpmevent` CSR 的高位（bit 63–56）定义了标准字段，提供两大核心功能：(1) 计数溢出检测与中断生成（OF bit + LCOFIP/LCOFIE）；(2) 基于特权模式的事件计数过滤（MINH/SINH/UINH/VSINH/VUINH）。同时新增只读 `scountovf` CSR，使 S-mode 可快速查询哪些计数器已溢出。

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/sscofpmf.adoc` — Sscofpmf Extension for Count Overflow and Mode-Based Filtering, Version 1.0
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — mhpmevent/mhpmcounter CSR、mip/mie LCOFIP/LCOFIE、mideleg、mcounteren
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — sip/sie 中 LCOFIP/LCOFIE 位

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内 src/priv/sscofpmf.adoc、src/priv/machine.adoc、src/priv/supervisor.adoc）

---

## 测试范围

### 关键 CSR

| CSR | 地址 | 说明 |
|-----|------|------|
| `mhpmevent3`–`mhpmevent31` | 0x323–0x33F | 事件选择器，高位新增 OF/xINH 字段 |
| `mhpmcounter3`–`mhpmcounter31` | 0xB03–0xB1F | 硬件性能计数器（M-mode 读写） |
| `hpmcounter3`–`hpmcounter31` | 0xC03–0xC1F | 硬件性能计数器（S/U-mode 只读影子） |
| `scountovf` | 0xDA0 | 32-bit 只读，OF bit 影子拷贝 |
| `mcounteren` | 0x306 | M-mode 控制 S-mode 对计数器的访问 |
| `hcounteren` | 0x606 | HS-mode 控制 VS-mode 对计数器的访问（VS-mode 门控用例已迁移至 `Hypervisor_Ss_test_plan.md` Group 10） |
| `mip` / `sip` | 0x344 / 0x144 | 中断 pending，bit 13 = LCOFIP |
| `mie` / `sie` | 0x304 / 0x104 | 中断 enable，bit 13 = LCOFIE |
| `mideleg` | 0x303 | 中断委托，bit 13 控制 LCOFI 委托至 S-mode |

### mhpmevent 高位字段布局

```
  63   62   61   60   59   58   57   56
┌────┬────┬────┬────┬────┬────┬────┬────┐
│ OF │MINH│SINH│UINH│VSINH│VUINH│WPRI│WPRI│
└────┴────┴────┴────┴────┴────┴────┴────┘
```

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mhpmevent_inh_op` | Each of the five `x`INH bits, when set, inhibit counting of events while in privilege mode `x`. All-zeroes for these bits results in counting of events in all modes. | 五个 `x`INH 位中的每一个，当置位时，禁止在特权模式 `x` 下计数事件。这些位全零时在所有模式下计数事件。 |
| `norm:mhpmevent_of_op` | The OF bit is set when the corresponding hpmcounter overflows, and remains set until written by software. | 当对应的 hpmcounter 溢出时 OF 位置位，并保持置位直到被软件写入。 |
| `norm:hpmcounter_overflow` | Since hpmcounter values are unsigned values, overflow is defined as unsigned overflow of the implemented counter bits. Note that there is no loss of information after an overflow since the counter wraps around and keeps counting while the sticky OF bit remains set. | 由于 hpmcounter 值是无符号值，溢出定义为已实现计数器位的无符号溢出。溢出后计数器回绕并继续计数，粘性 OF 位保持置位。 |
| `norm:count_overflow_interrupt` | If an hpmcounter overflows while the associated OF bit is zero, then a "count overflow interrupt request" is generated. If the OF bit is one, then no interrupt request is generated. Consequently the OF bit also functions as a count overflow interrupt disable for the associated hpmcounter. | 如果 hpmcounter 在关联 OF 位为零时溢出，则生成"计数溢出中断请求"。如果 OF 位为 1，则不生成中断请求。因此 OF 位也充当关联 hpmcounter 的计数溢出中断禁用位。 |
| `norm:count_overflow_trigger` | Count overflow never results from writes to the mhpmcounter_n or mhpmevent_n registers, only from hardware increments of counter registers. | 计数溢出永远不会由对 mhpmcounter_n 或 mhpmevent_n 寄存器的写入引起，仅由计数器寄存器的硬件递增引起。 |
| `norm:mhpmevent_of_bit_set` | Generation of a count-overflow-interrupt request by an `hpmcounter` sets the associated OF bit. | `hpmcounter` 生成计数溢出中断请求会设置关联的 OF 位。 |
| `norm:LCOFIP_op` | When an OF bit is set, it eventually, but not necessarily immediately, sets the LCOFIP bit in the `mip`/`sip` registers. | 当 OF 位置位时，它最终（但不一定立即）设置 `mip`/`sip` 寄存器中的 LCOFIP 位。 |
| `norm:scountovf_op` | This extension adds the `scountovf` CSR, a 32-bit read-only register that contains shadow copies of the OF bits in the 29 mhpmevent CSRs (mhpmevent_3 - mhpmevent_31) - where scountovf bit X corresponds to mhpmevent_X. | 此扩展添加了 `scountovf` CSR，这是一个 32 位只读寄存器，包含 29 个 mhpmevent CSR（mhpmevent_3 - mhpmevent_31）中 OF 位的影子副本 —— 其中 scountovf 位 X 对应 mhpmevent_X。 |
| `norm:scountovf_smode_read_access_control` | Read access to bit X is subject to the same mcounteren (or mcounteren and hcounteren) CSRs that mediate access to the hpmcounter CSRs by S-mode (or VS-mode). | 对位 X 的读取访问受与 S 模式（或 VS 模式）访问 hpmcounter CSR 相同的 mcounteren（或 mcounteren 和 hcounteren）CSR 控制。 |
| `norm:scountovf_mmode_read_access` | In M-mode, scountovf bit X is always readable. | 在 M 模式下，scountovf 位 X 始终可读。 |
| `norm:scountovf_smode_read_access` | In S/HS-mode, scountovf bit X is readable when mcounteren bit X is set, and otherwise reads as zero. | 在 S/HS 模式下，当 mcounteren 位 X 置位时 scountovf 位 X 可读，否则读为零。 |

### 不在测试范围内

- **计数器事件选择的正确性**：mhpmevent 低位的事件选择器字段是实现特定的，不在 Sscofpmf 标准化范围内
- **Vectored 模式的中断分发细节**：仅验证 LCOFI 中断能被正确生成和捕获，不验证 Vectored 模式下的跳转偏移
- **多 hart 场景**：项目为单核测试环境
- **RV32 / mhpmeventh**：仅覆盖 RV64（RV32 需要 mhpmeventh CSR 访问高 32 位）
- **计数器精确值验证**：不验证计数器递增的精确事件数量，仅验证溢出行为
- **VS-mode 场景（VSINH/VUINH 计数抑制、VS-mode scountovf 双重门控）**：已迁移至 `Hypervisor_Ss_test_plan.md` Group 10

---

## 前提与约束

### 计数器发现策略

由于不同实现支持的 hpmcounter 数量不同（3–31），测试用例采用**动态发现法**：在 M-mode 下尝试写 `mhpmcounter` 为非零值再读回，如果读回为零则认为该计数器未实现，对应测试跳过。

### 事件触发策略

为可靠地使计数器递增，测试使用以下策略：
1. **配置 mhpmevent 为"已退休指令"事件**（事件编号由实现决定，测试通过平台配置宏提供）
2. **执行已知指令序列**以产生可预测的计数器增量
3. **初始化 mhpmcounter 为接近最大值**（如 `0xFFFFFFFF_FFFFFFFE`）以快速触发溢出

任一平台违反 SPEC 时用例保持 FAIL，实现缺陷记录至 `bugs/` 目录。

---

## 测试分组

### Group 1：mhpmevent 字段读写验证（M-mode CSR 读写回环）

**规范依据**：
- `norm:mhpmevent_of_op`：OF bit 可写可读
- `norm:mhpmevent_inh_op`：xINH bit 可写可读；未实现的特权模式对应的 xINH bit 为 read-only zero

**测试职责**：在 M-mode 下，验证 mhpmevent CSR 高 8 位字段的读写行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| COFPMF-RW-01 | OF bit 读写回环 | 对已实现的 mhpmevent，写 OF=1 后读回验证，再写 OF=0 读回验证 | OF bit 写入值与读回值一致 |
| COFPMF-RW-02 | MINH bit 读写回环 | 写 MINH=1 后读回，再写 0 读回 | MINH bit 读写一致 |
| COFPMF-RW-03 | SINH bit 读写回环 | 写 SINH=1 后读回，再写 0 读回；若 S-mode 未实现则应为 read-only zero | 已实现 S-mode 时读写一致；未实现时恒为 0 |
| COFPMF-RW-04 | UINH bit 读写回环 | 写 UINH=1 后读回，再写 0 读回；若 U-mode 未实现则应为 read-only zero | 已实现 U-mode 时读写一致；未实现时恒为 0 |
| COFPMF-RW-05 | VSINH bit 读写回环 | 写 VSINH=1 后读回，再写 0 读回；若 VS-mode 未实现则应为 read-only zero | 已实现 H-ext 时读写一致；未实现时恒为 0 |
| COFPMF-RW-06 | VUINH bit 读写回环 | 写 VUINH=1 后读回，再写 0 读回；若 VU-mode 未实现则应为 read-only zero | 已实现 H-ext 时读写一致；未实现时恒为 0 |
| COFPMF-RW-07 | WPRI 字段为零 | 写 bit 57–56 为 1 后读回应为 0 | WPRI 字段 read-only zero |
| COFPMF-RW-08 | 多字段组合写入 | 同时写入 OF=1、MINH=1、SINH=1、UINH=1 后读回所有字段 | 各字段独立保持正确值 |
| COFPMF-RW-09 | 低位字段保留 | 写 mhpmevent 高位不影响低 56 位（写高位前后低位值不变） | 低位字段不受高位写入影响 |
| COFPMF-RW-10 | 多计数器遍历 | 对 mhpmevent3–31 逐个执行 OF bit 写 1/读回验证（跳过未实现的） | 所有已实现的 mhpmevent 的 OF bit 可正常读写 |

> [!NOTE]
> COFPMF-RW-05/06 验证 VSINH/VUINH 的 WARL 读写正分支（不依赖 H 扩展）；若平台未实现 H 扩展，这两个位应为 read-only zero（负分支）。依赖 H 扩展的完整语义（VSINH/VUINH 计数抑制、未实现 H 扩展时只读零探测）见 `Hypervisor_Ss_test_plan.md` Group 10（HCROSS-SSCOFPMF-04~06）。

---

### Group 2：特权模式过滤

**规范依据**：
- `norm:mhpmevent_inh_op`：xINH bit 置位时抑制对应模式下的事件计数；全零则所有模式均计数

**测试职责**：验证各 xINH bit 能正确抑制/允许对应特权模式下的事件计数。

**测试前提**：需要先确认至少一个 hpmcounter 已实现，并找到一个可用的事件编号（由平台配置提供，通常为已退休指令事件）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| COFPMF-INH-01 | 全零 xINH 时 M-mode 计数 | 设置所有 xINH=0，在 M-mode 执行指令序列，验证计数器递增 | 计数器值增加 |
| COFPMF-INH-02 | MINH=1 抑制 M-mode 计数 | 设置 MINH=1，在 M-mode 执行指令序列，验证计数器不递增 | 计数器值不变 |
| COFPMF-INH-03 | MINH=0 恢复 M-mode 计数 | 清除 MINH 后在 M-mode 再次执行，验证计数恢复 | 计数器值增加 |
| COFPMF-INH-04 | SINH=1 抑制 S-mode 计数 | 设置 SINH=1，切换至 S-mode 执行指令序列，回到 M-mode 读取计数器 | 计数器值不变（忽略模式切换本身的开销） |
| COFPMF-INH-05 | SINH=0 允许 S-mode 计数 | 设置 SINH=0，切换至 S-mode 执行指令序列，回到 M-mode 读取计数器 | 计数器值增加 |
| COFPMF-INH-06 | UINH=1 抑制 U-mode 计数 | 设置 UINH=1，切换至 U-mode 执行指令序列，回到 M-mode 读取计数器 | 计数器值不变 |
| COFPMF-INH-07 | UINH=0 允许 U-mode 计数 | 设置 UINH=0，切换至 U-mode 执行指令序列，回到 M-mode 读取计数器 | 计数器值增加 |
| COFPMF-INH-08 | 多 xINH 组合：仅 M-mode 计数 | 设置 SINH=1、UINH=1（抑制 S/U），仅在 M-mode 执行，验证计数 | M-mode 计数正常 |
| COFPMF-INH-09 | 多 xINH 组合：仅 S-mode 计数 | 设置 MINH=1、UINH=1（抑制 M/U），切换至 S-mode 执行 | S-mode 计数正常 |
| COFPMF-INH-10 | 全部 xINH=1 时无计数 | 设置 MINH=SINH=UINH=1，在所有模式执行均不应递增 | 计数器值不变 |

> [!NOTE]
> 特权模式切换（ecall / mret / sret）本身会产生指令计数。测试中通过比较"抑制"和"不抑制"两种场景的计数差异来验证功能，而非验证精确计数值。对于 S/U-mode 测试，应先在 M-mode 禁用计数（MINH=1），切换到目标模式执行后再返回，仅比较目标模式执行期间的计数增量。
>
> 本组不覆盖 VSINH/VUINH 的计数抑制功能用例（依赖 H 扩展，需进入 VS/VU-mode 执行），该部分由 `Hypervisor_Ss_test_plan.md` Group 10（HCROSS-SSCOFPMF-04~05）覆盖。

---

### Group 3：计数溢出与中断

**规范依据**：
- `norm:mhpmevent_of_op`：OF bit 在 hpmcounter 溢出时置位，sticky 直到软件清除
- `norm:hpmcounter_overflow`：溢出定义为已实现位宽的无符号溢出
- `norm:count_overflow_interrupt`：溢出且 OF=0 时生成中断请求；OF=1 时不生成
- `norm:count_overflow_trigger`：写 mhpmcounter/mhpmevent 不触发溢出，仅硬件递增触发
- `norm:mhpmevent_of_bit_set`：溢出中断请求设置 OF bit
- `norm:LCOFIP_op`：OF 置位后最终设置 LCOFIP

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| COFPMF-OVF-01 | 计数器溢出设置 OF | 初始化 mhpmcounter 为接近最大值，配置事件并执行指令使其溢出，检查 OF bit | OF bit 被置为 1 |
| COFPMF-OVF-02 | OF bit sticky 特性 | 溢出后 OF=1，继续执行指令，验证 OF 保持为 1 且计数器继续计数（wrap around） | OF=1 保持不变，计数器值从 0 附近继续递增 |
| COFPMF-OVF-03 | 软件清除 OF bit | 溢出后将 OF 写为 0，验证清除成功 | OF bit 变为 0 |
| COFPMF-OVF-04 | 溢出生成 LCOFIP | 初始化 mhpmcounter 接近最大值，OF=0，使能 LCOFIE，触发溢出后检查 LCOFIP | mip.LCOFIP = 1 |
| COFPMF-OVF-05 | OF=1 时溢出不生成中断 | 先设 OF=1，再触发溢出，验证 LCOFIP 不被设置 | mip.LCOFIP = 0 |
| COFPMF-OVF-06 | 写 mhpmcounter 不触发溢出 | 直接将 mhpmcounter 写为 0（从大值到 0 的"回绕"），验证 OF 不被设置 | OF bit 保持 0 |
| COFPMF-OVF-07 | 写 mhpmevent 不触发溢出 | 修改 mhpmevent 的事件选择/xINH 字段，验证 OF 不被设置 | OF bit 保持 0 |
| COFPMF-OVF-08 | LCOFIP 软件清除 | 置位 LCOFIP 后，软件通过写 mip 清除，验证清除成功 | mip.LCOFIP = 0 |
| COFPMF-OVF-09 | LCOFI 中断委托至 S-mode | 设置 mideleg bit 13 = 1，触发溢出中断，验证在 S-mode handler 中被捕获 | S-mode trap handler 收到 cause = interrupt\|13 |
| COFPMF-OVF-10 | LCOFI 中断在 M-mode 处理 | mideleg bit 13 = 0，触发溢出中断，验证在 M-mode handler 中被捕获 | M-mode trap handler 收到 cause = interrupt\|13 |
| COFPMF-OVF-11 | LCOFIE 禁用时不产生中断 | 清除 mie.LCOFIE，触发溢出，验证不发生中断（仅 OF 置位） | OF=1，但无中断产生 |
| COFPMF-OVF-12 | 多计数器同时溢出 | 配置两个计数器均接近最大值，同时触发溢出，验证两个 OF bit 均被设置 | 两个 mhpmevent 的 OF bit 均为 1 |

> [!WARNING]
> `norm:LCOFIP_op` 规范指出 OF 置位后"最终但不一定立即"设置 LCOFIP。测试中需在溢出后插入足够的延迟或内存屏障，以确保 LCOFIP 已传播。若 LCOFIP 在合理延迟后仍未置位，测试应标注为实现相关的时序问题而非直接失败。

---

### Group 4：scountovf 寄存器

**规范依据**：
- `norm:scountovf_op`：32-bit 只读，bit X 对应 mhpmevent X 的 OF bit
- `norm:scountovf_mmode_read_access`：M-mode 始终可读
- `norm:scountovf_smode_read_access`：S-mode 受 mcounteren 控制
- `norm:scountovf_smode_read_access_control`：访问控制与 hpmcounter 的 mcounteren/hcounteren 规则一致

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| COFPMF-SOV-01 | scountovf 反映 OF bit | 设置 mhpmevent3 的 OF=1，读取 scountovf，验证 bit 3 = 1 | scountovf bit 3 = 1 |
| COFPMF-SOV-02 | scountovf 只读验证 | 尝试写 scountovf（S-mode 下 csrw），应触发 illegal-instruction | 触发 illegal-instruction（cause=2） |
| COFPMF-SOV-03 | scountovf 多 bit 映射 | 同时设置 mhpmevent3 和 mhpmevent5 的 OF=1，验证 scountovf bit 3 和 bit 5 均为 1 | scountovf = (1<<3) \| (1<<5) |
| COFPMF-SOV-04 | scountovf 清除跟踪 | 设置 OF=1 后清除 mhpmevent3 的 OF，再读 scountovf bit 3 | scountovf bit 3 = 0 |
| COFPMF-SOV-05 | M-mode 读取不受 mcounteren 限制 | 清除 mcounteren bit 3，M-mode 读 scountovf bit 3 仍可见 | scountovf bit 3 反映真实 OF 值 |
| COFPMF-SOV-06 | S-mode mcounteren=1 可读 | 设置 mcounteren bit 3 = 1，S-mode 读 scountovf bit 3 | 读到真实 OF 值 |
| COFPMF-SOV-07 | S-mode mcounteren=0 读为零 | 清除 mcounteren bit 3，S-mode 读 scountovf，bit 3 应为 0（即使 OF=1） | scountovf bit 3 = 0 |

> [!NOTE]
> VS-mode scountovf 访问控制用例（`norm:scountovf_vsmode_read_access`）依赖 Hypervisor 扩展，已迁移至 `Hypervisor_Ss_test_plan.md` Group 10（HCROSS-SSCOFPMF-01~03）。

---

## 设计要点

### 1. 计数器动态发现

由于 hpmcounter3–31 的实现数量是可选的，所有测试用例在开始前必须先动态探测目标计数器是否存在：

1. 尝试在 M-mode 写入非零值到目标 mhpmcounter
2. 回读该计数器
3. 恢复为 0
4. 若回读非零则认为已实现，否则认为未实现

若计数器未实现，用例应使用 `TEST_SKIP()` 跳过。

### 2. 事件配置策略

不同实现的 mhpmevent 低位事件编号不同。测试应通过平台配置宏提供可覆盖的事件编号（例如"已退休指令"事件编号），避免在方案中固化具体实现。

### 3. 中断测试的 trap handler 要求

LCOFI 中断测试（Group 3）要求 trap handler 具备：

1. **中断识别**：检查 `mcause` 最高位（interrupt bit），若为 1 则为中断
2. **中断信息记录**：将 interrupt cause（低位）记录到 trap 状态
3. **LCOFIP 清除**：handler 中清除 `mip.LCOFIP` 以避免无限中断循环
4. **LCOFIE 管理**：提供使能/禁用 LCOFIE 的辅助能力

### 4. 溢出触发方法

为可靠触发溢出：
1. 设置 mhpmcounter 为接近溢出的值（例如 `-MARGIN`）
2. 配置 mhpmevent 开始计数
3. 执行足够多的指令（大于 MARGIN）
4. 停止计数（清除 mhpmevent）

`MARGIN` 建议设为 50–100，足够覆盖循环开销。

### 5. scountovf 访问控制测试

Group 4 SOV-06~07 需要在 S-mode 下读取 `scountovf`，可复用框架的特权级切换 + 无 trap 断言能力。VS-mode 双重门控用例（依赖 H 扩展）已迁移至 `Hypervisor_Ss_test_plan.md` Group 10，本方案不再包含 VS-mode 测试。

---

## 附录 A：规范点覆盖矩阵

| Norm ID | 覆盖的测试 ID | 覆盖状态 | 备注 |
|---------|--------------|----------|------|
| `norm:mhpmevent_inh_op` | COFPMF-RW-02 ~ COFPMF-RW-08、COFPMF-INH-01 ~ COFPMF-INH-10 | 已覆盖 | xINH 位读写与模式过滤 |
| `norm:mhpmevent_of_op` | COFPMF-RW-01、COFPMF-RW-08、COFPMF-RW-10、COFPMF-OVF-01 ~ COFPMF-OVF-03、COFPMF-SOV-01、COFPMF-SOV-03、COFPMF-SOV-04 | 已覆盖 | OF 位读写与 sticky 语义 |
| `norm:hpmcounter_overflow` | COFPMF-OVF-01、COFPMF-OVF-02、COFPMF-OVF-12 | 已覆盖 | 无符号溢出定义 |
| `norm:count_overflow_interrupt` | COFPMF-OVF-04、COFPMF-OVF-05、COFPMF-OVF-11 | 已覆盖 | 溢出中断请求生成条件 |
| `norm:count_overflow_trigger` | COFPMF-OVF-06、COFPMF-OVF-07 | 已覆盖 | 写 CSR 不触发溢出 |
| `norm:mhpmevent_of_bit_set` | COFPMF-OVF-01、COFPMF-OVF-04、COFPMF-OVF-12 | 已覆盖 | 溢出中断请求设置 OF |
| `norm:LCOFIP_op` | COFPMF-OVF-04、COFPMF-OVF-05、COFPMF-OVF-08 ~ COFPMF-OVF-11 | 已覆盖 | OF 置位后 LCOFIP 传播 |
| `norm:scountovf_op` | COFPMF-SOV-01、COFPMF-SOV-02、COFPMF-SOV-03、COFPMF-SOV-04 | 已覆盖 | scountovf 32-bit 只读语义 |
| `norm:scountovf_mmode_read_access` | COFPMF-SOV-01、COFPMF-SOV-03、COFPMF-SOV-04、COFPMF-SOV-05 | 已覆盖 | M-mode 始终可读 |
| `norm:scountovf_smode_read_access` | COFPMF-SOV-06、COFPMF-SOV-07 | 已覆盖 | S-mode 受 mcounteren 控制 |
| `norm:scountovf_smode_read_access_control` | COFPMF-SOV-05、COFPMF-SOV-06、COFPMF-SOV-07 | 已覆盖 | 访问控制规则一致性 |

**未覆盖规范点说明**：
- `norm:scountovf_vsmode_read_access`：VS-mode 双重门控用例已迁移至 `Hypervisor_Ss_test_plan.md` Group 10，本方案不重复覆盖。
