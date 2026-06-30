**中文 | [English](../testplan_en/Sscofpmf_test_plan_en.md)**

# Sscofpmf 扩展测试计划

本文档描述 Sscofpmf（Count Overflow and Privilege Mode Filtering, Version 1.0）扩展的测试计划。Sscofpmf 为 `mhpmevent` CSR 的高位（bit 63–56）定义了标准字段，提供两大核心功能：(1) 计数溢出检测与中断生成（OF bit + LCOFIP/LCOFIE）；(2) 基于特权模式的事件计数过滤（MINH/SINH/UINH/VSINH/VUINH）。同时新增只读 `scountovf` CSR，使 S-mode 可快速查询哪些计数器已溢出。

---

## 测试范围

### 规范来源

- `SPEC/sscofpmf.adoc` — Sscofpmf Extension for Count Overflow and Mode-Based Filtering, Version 1.0

### 关键 CSR

| CSR | 地址 | 说明 |
|-----|------|------|
| `mhpmevent3`–`mhpmevent31` | 0x323–0x33F | 事件选择器，高位新增 OF/xINH 字段 |
| `mhpmcounter3`–`mhpmcounter31` | 0xB03–0xB1F | 硬件性能计数器（M-mode 读写） |
| `hpmcounter3`–`hpmcounter31` | 0xC03–0xC1F | 硬件性能计数器（S/U-mode 只读影子） |
| `scountovf` | 0xDA0 | 32-bit 只读，OF bit 影子拷贝 |
| `mcounteren` | 0x306 | M-mode 控制 S-mode 对计数器的访问 |
| `hcounteren` | 0x606 | HS-mode 控制 VS-mode 对计数器的访问 |
| `mip` / `sip` | 0x344 / 0x144 | 中断 pending，bit 13 = LCOFIP |
| `mie` / `sie` | 0x304 / 0x104 | 中断 enable，bit 13 = LCOFIE |
| `mideleg` | 0x303 | 中断委托，bit 13 控制 LCOFI 委托至 S-mode |

### 关键参考文件

| 路径 | 说明 |
|------|------|
| `SPEC/sscofpmf.adoc` | Sscofpmf 规范全文 |
| `common/test_framework.h` | 测试框架（TEST_BEGIN / TEST_ASSERT / TEST_END） |
| `common/encoding.h` | CSR 地址定义（需新增 mhpmevent/hpmcounter/scountovf 等） |
| `common/csr_accessors.c` | 动态 CSR 读写（需新增 mhpmevent/mhpmcounter/scountovf） |
| `common/trap.c` | trap handler（需增强中断识别能力） |

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
| `norm:hpmcounter_overflow` | Since hpmcounter values are unsigned values, overflow is defined as unsigned overflow of the implemented counter bits. Note that there is no loss of information after an overflow since the counter wraps around and keeps counting while the sticky OF bit remains set. | 由于 hpmcounter 值是无符号值，溢出定义为已实现计数器位的无符号溢出。注意溢出后没有信息丢失，因为计数器会回绕并继续计数，同时粘性 OF 位保持置位。 |
| `norm:count_overflow_interrupt` | If an hpmcounter overflows while the associated OF bit is zero, then a "count overflow interrupt request" is generated. If the OF bit is one, then no interrupt request is generated. Consequently the OF bit also functions as a count overflow interrupt disable for the associated hpmcounter. | 如果 hpmcounter 在关联 OF 位为零时溢出，则生成"计数溢出中断请求"。如果 OF 位为 1，则不生成中断请求。因此 OF 位也充当关联 hpmcounter 的计数溢出中断禁用位。 |
| `norm:count_overflow_trigger` | Count overflow never results from writes to the mhpmcounter_n or mhpmevent_n registers, only from hardware increments of counter registers. | 计数溢出永远不会由对 mhpmcounter_n 或 mhpmevent_n 寄存器的写入引起，仅由计数器寄存器的硬件递增引起。 |
| `norm:mhpmevent_of_bit_set` | Generation of a count-overflow-interrupt request by an `hpmcounter` sets the associated OF bit. | `hpmcounter` 生成计数溢出中断请求会设置关联的 OF 位。 |
| `norm:LCOFIP_op` | When an OF bit is set, it eventually, but not necessarily immediately, sets the LCOFIP bit in the `mip`/`sip` registers. | 当 OF 位置位时，它最终（但不一定立即）设置 `mip`/`sip` 寄存器中的 LCOFIP 位。 |
| `norm:scountovf_op` | This extension adds the `scountovf` CSR, a 32-bit read-only register that contains shadow copies of the OF bits in the 29 mhpmevent CSRs (mhpmevent_3 - mhpmevent_31) - where scountovf bit X corresponds to mhpmevent_X. | 此扩展添加了 `scountovf` CSR，这是一个 32 位只读寄存器，包含 29 个 mhpmevent CSR（mhpmevent_3 - mhpmevent_31）中 OF 位的影子副本——其中 scountovf 位 X 对应 mhpmevent_X。 |
| `norm:scountovf_smode_read_access_control` | Read access to bit X is subject to the same mcounteren (or mcounteren and hcounteren) CSRs that mediate access to the hpmcounter CSRs by S-mode (or VS-mode). | 对位 X 的读取访问受与 S 模式（或 VS 模式）访问 hpmcounter CSR 相同的 mcounteren（或 mcounteren 和 hcounteren）CSR 控制。 |
| `norm:scountovf_mmode_read_access` | In M-mode, scountovf bit X is always readable. | 在 M 模式下，scountovf 位 X 始终可读。 |
| `norm:scountovf_smode_read_access` | In S/HS-mode, scountovf bit X is readable when mcounteren bit X is set, and otherwise reads as zero. | 在 S/HS 模式下，当 mcounteren 位 X 置位时 scountovf 位 X 可读，否则读为零。 |
| `norm:scountovf_vsmode_read_access` | Similarly, in VS mode, scountovf bit X is readable when mcounteren bit X and hcounteren bit X are both set, and otherwise reads as zero. | 类似地，在 VS 模式下，当 mcounteren 位 X 和 hcounteren 位 X 都置位时 scountovf 位 X 可读，否则读为零。 |

### 不在测试范围内

- **计数器事件选择的正确性**：mhpmevent 低位的事件选择器字段是实现特定的，不在 Sscofpmf 标准化范围内
- **Vectored 模式的中断分发细节**：仅验证 LCOFI 中断能被正确生成和捕获，不验证 Vectored 模式下的跳转偏移
- **多 hart 场景**：项目为单核测试环境
- **RV32 / mhpmeventh**：仅覆盖 RV64（RV32 需要 mhpmeventh CSR 访问高 32 位）
- **计数器精确值验证**：不验证计数器递增的精确事件数量，仅验证溢出行为

---

## 前提与约束

> [!IMPORTANT]
> 框架层面需要先补齐以下支持，才能实施测试代码（详见"设计要点"章节）：
> 1. `common/encoding.h` 新增 mhpmevent/mhpmcounter/scountovf CSR 地址及字段掩码
> 2. `common/csr_accessors.c` 新增 mhpmevent/mhpmcounter/scountovf 的动态读写 case
> 3. `common/trap.c` 增强异步中断（interrupt bit）的识别与记录能力

### 计数器发现策略

由于不同实现支持的 hpmcounter 数量不同（3–31），测试用例采用**动态发现法**：在 M-mode 下尝试写 `mhpmcounter` 为非零值再读回，如果读回为零则认为该计数器未实现，对应测试跳过。

### 事件触发策略

为可靠地使计数器递增，测试使用以下策略：
1. **配置 mhpmevent 为"已退休指令"事件**（事件编号由实现决定，Spike 上通常为 event=2）
2. **执行已知指令序列**以产生可预测的计数器增量
3. **初始化 mhpmcounter 为接近最大值**（如 `0xFFFFFFFF_FFFFFFFE`）以快速触发溢出

---

## 测试分组

### Group 1：mhpmevent 字段读写验证（M-mode CSR 读写回环）

**规范依据**：
- `norm:mhpmevent_of_op`：OF bit 可写可读
- `norm:mhpmevent_inh_op`：xINH bit 可写可读
- 未实现的特权模式对应的 xINH bit 为 read-only zero

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
| COFPMF-RW-08 | 多字段组合写入 | 同时写入 OF=1, MINH=1, SINH=1, UINH=1 后读回所有字段 | 各字段独立保持正确值 |
| COFPMF-RW-09 | 低位字段保留 | 写 mhpmevent 高位不影响低 56 位（写高位前后低位值不变） | 低位字段不受高位写入影响 |
| COFPMF-RW-10 | 多计数器遍历 | 对 mhpmevent3–31 逐个执行 OF bit 写 1/读回验证（跳过未实现的） | 所有已实现的 mhpmevent 的 OF bit 可正常读写 |

```c
/* COFPMF-RW-01 示例伪代码 */
TEST_REGISTER(test_cofpmf_of_rw);
bool test_cofpmf_of_rw(void) {
    TEST_BEGIN("COFPMF-RW-01: OF bit read-write");

    /* 动态发现：尝试写 mhpmcounter3 */
    CSRW(mhpmcounter3, 0x1);
    uintptr_t val = CSRR(mhpmcounter3);
    if (val == 0) TEST_SKIP("hpmcounter3 not implemented");

    /* 保存原始值 */
    uintptr_t orig = CSRR(mhpmevent3);

    /* 写 OF=1 */
    CSRW(mhpmevent3, orig | MHPMEVENT_OF);
    val = CSRR(mhpmevent3);
    TEST_ASSERT("OF bit set to 1", (val & MHPMEVENT_OF) != 0);

    /* 写 OF=0 */
    CSRW(mhpmevent3, orig & ~MHPMEVENT_OF);
    val = CSRR(mhpmevent3);
    TEST_ASSERT("OF bit cleared to 0", (val & MHPMEVENT_OF) == 0);

    /* 还原 */
    CSRW(mhpmevent3, orig);
    TEST_END();
}
```

---

### Group 2：特权模式过滤

**规范依据**：
- `norm:mhpmevent_inh_op`：xINH bit 置位时抑制对应模式下的事件计数；全零则所有模式均计数

**测试职责**：验证各 xINH bit 能正确抑制/允许对应特权模式下的事件计数。

**测试前提**：需要先确认至少一个 hpmcounter 已实现，并找到一个可用的事件编号（在 Spike 上使用 event=2 即已退休指令）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| COFPMF-INH-01 | 全零 xINH 时 M-mode 计数 | 设置所有 xINH=0，在 M-mode 执行指令序列，验证计数器递增 | 计数器值增加 |
| COFPMF-INH-02 | MINH=1 抑制 M-mode 计数 | 设置 MINH=1，在 M-mode 执行指令序列，验证计数器不递增 | 计数器值不变 |
| COFPMF-INH-03 | MINH=0 恢复 M-mode 计数 | 清除 MINH 后在 M-mode 再次执行，验证计数恢复 | 计数器值增加 |
| COFPMF-INH-04 | SINH=1 抑制 S-mode 计数 | 设置 SINH=1，切换至 S-mode 执行指令序列，回到 M-mode 读取计数器 | 计数器值不变（忽略模式切换本身的开销） |
| COFPMF-INH-05 | SINH=0 允许 S-mode 计数 | 设置 SINH=0，切换至 S-mode 执行指令序列，回到 M-mode 读取计数器 | 计数器值增加 |
| COFPMF-INH-06 | UINH=1 抑制 U-mode 计数 | 设置 UINH=1，切换至 U-mode 执行指令序列，回到 M-mode 读取计数器 | 计数器值不变 |
| COFPMF-INH-07 | UINH=0 允许 U-mode 计数 | 设置 UINH=0，切换至 U-mode 执行指令序列，回到 M-mode 读取计数器 | 计数器值增加 |
| COFPMF-INH-08 | 多 xINH 组合：仅 M-mode 计数 | 设置 SINH=1, UINH=1（抑制 S/U），仅在 M-mode 执行，验证计数 | M-mode 计数正常 |
| COFPMF-INH-09 | 多 xINH 组合：仅 S-mode 计数 | 设置 MINH=1, UINH=1（抑制 M/U），切换至 S-mode 执行 | S-mode 计数正常 |
| COFPMF-INH-10 | 全部 xINH=1 时无计数 | 设置 MINH=SINH=UINH=1，在所有模式执行均不应递增 | 计数器值不变 |

```c
/* COFPMF-INH-02 示例伪代码 */
TEST_REGISTER(test_cofpmf_minh_inhibit);
bool test_cofpmf_minh_inhibit(void) {
    TEST_BEGIN("COFPMF-INH-02: MINH inhibits M-mode counting");

    /* 配置 mhpmevent3 为已退休指令事件, MINH=1 */
    CSRW(mhpmevent3, SPIKE_EVENT_INSTRET | MHPMEVENT_MINH);

    /* 初始化计数器 */
    CSRW(mhpmcounter3, 0);

    /* 执行已知指令序列（10 条 NOP） */
    asm volatile("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop");

    /* 读取计数器 */
    uintptr_t count = CSRR(mhpmcounter3);
    TEST_ASSERT("counter did not increment with MINH=1", count == 0);

    /* 清理 */
    CSRW(mhpmevent3, 0);
    TEST_END();
}
```

> [!NOTE]
> 特权模式切换（ecall / mret / sret）本身会产生指令计数。测试中通过比较"抑制"和"不抑制"两种场景的计数差异来验证功能，而非验证精确计数值。对于 S/U-mode 测试，应先在 M-mode 禁用计数（MINH=1），切换到目标模式执行后再返回，仅比较目标模式执行期间的计数增量。

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
| COFPMF-OVF-01 | 计数器溢出设置 OF | 初始化 mhpmcounter 为接近最大值，配置事件并执行指令使其溢出，检查 OF bit | OF bit 被硬件置为 1 |
| COFPMF-OVF-02 | OF bit sticky 特性 | 溢出后 OF=1，继续执行指令，验证 OF 保持为 1 且计数器继续计数（wrap around） | OF=1 保持不变，计数器值从 0 附近继续递增 |
| COFPMF-OVF-03 | 软件清除 OF bit | 溢出后将 OF 写为 0，验证清除成功 | OF bit 变为 0 |
| COFPMF-OVF-04 | 溢出生成 LCOFIP | 初始化 mhpmcounter 接近最大值，OF=0，使能 LCOFIE，触发溢出后检查 LCOFIP | mip.LCOFIP = 1 |
| COFPMF-OVF-05 | OF=1 时溢出不生成中断 | 先设 OF=1，再触发溢出，验证 LCOFIP 不被设置 | mip.LCOFIP = 0 |
| COFPMF-OVF-06 | 写 mhpmcounter 不触发溢出 | 直接将 mhpmcounter 写为 0（从大值到 0 的"回绕"），验证 OF 不被设置 | OF bit 保持 0 |
| COFPMF-OVF-07 | 写 mhpmevent 不触发溢出 | 修改 mhpmevent 的事件选择/xINH 字段，验证 OF 不被设置 | OF bit 保持 0 |
| COFPMF-OVF-08 | LCOFIP 软件清除 | 置位 LCOFIP 后，软件通过写 mip 清除，验证清除成功 | mip.LCOFIP = 0 |
| COFPMF-OVF-09 | LCOFI 中断委托至 S-mode | 设置 mideleg bit 13 = 1，触发溢出中断，验证在 S-mode handler 中被捕获 | S-mode trap handler 收到 cause = interrupt\|13 |
| COFPMF-OVF-10 | LCOFI 中断在 M-mode 处理 | mideleg bit 13 = 0，触发溢出中断，验证在 M-mode handler 中被捕获 | M-mode trap handler 收到 cause = interrupt\|13 |
| COFPMF-OVF-11 | LCOFIE 禁用时不产生中断 | 清除 mie.LCOFIE，触发溢出，验证不发生中断（仅 OF 置位） | OF=1, 但无中断产生 |
| COFPMF-OVF-12 | 多计数器同时溢出 | 配置两个计数器均接近最大值，同时触发溢出，验证两个 OF bit 均被设置 | 两个 mhpmevent 的 OF bit 均为 1 |

```c
/* COFPMF-OVF-01 示例伪代码 */
TEST_REGISTER(test_cofpmf_overflow_sets_of);
bool test_cofpmf_overflow_sets_of(void) {
    TEST_BEGIN("COFPMF-OVF-01: Counter overflow sets OF bit");

    /* 配置事件：已退休指令，OF=0，无模式过滤 */
    CSRW(mhpmevent3, SPIKE_EVENT_INSTRET);

    /* 初始化计数器为接近最大值 */
    CSRW(mhpmcounter3, (uintptr_t)-10);

    /* 执行足够多的指令以触发溢出 */
    for (volatile int i = 0; i < 20; i++) { }

    /* 停止计数 */
    uintptr_t event = CSRR(mhpmevent3);

    /* 验证 OF bit 被设置 */
    TEST_ASSERT("OF bit set after overflow", (event & MHPMEVENT_OF) != 0);

    /* 清理 */
    CSRW(mhpmevent3, 0);
    CSRW(mhpmcounter3, 0);
    TEST_END();
}
```

> [!WARNING]
> `norm:LCOFIP_op` 规范指出 OF 置位后"最终但不一定立即"设置 LCOFIP。测试中需在溢出后插入足够的延迟或内存屏障，以确保 LCOFIP 已传播。若 LCOFIP 在合理延迟后仍未置位，测试应标注为实现相关的时序问题而非直接失败。

---

### Group 4：scountovf 寄存器

**规范依据**：
- `norm:scountovf_op`：32-bit 只读，bit X 对应 mhpmevent X 的 OF bit
- `norm:scountovf_mmode_read_access`：M-mode 始终可读
- `norm:scountovf_smode_read_access`：S-mode 受 mcounteren 控制
- `norm:scountovf_vsmode_read_access`：VS-mode 受 mcounteren 和 hcounteren 双重控制
- `norm:scountovf_smode_read_access_control`：访问控制与 hpmcounter 的 mcounteren/hcounteren 规则一致

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| COFPMF-SOV-01 | scountovf 反映 OF bit | 设置 mhpmevent3 的 OF=1，读取 scountovf，验证 bit 3 = 1 | scountovf bit 3 = 1 |
| COFPMF-SOV-02 | scountovf 只读验证 | 尝试写 scountovf（S-mode 下 csrw），应触发 illegal instruction | 触发 CAUSE_ILLEGAL_INST |
| COFPMF-SOV-03 | scountovf 多 bit 映射 | 同时设置 mhpmevent3 和 mhpmevent5 的 OF=1，验证 scountovf bit 3 和 bit 5 均为 1 | scountovf = (1<<3) \| (1<<5) |
| COFPMF-SOV-04 | scountovf 清除跟踪 | 设置 OF=1 后清除 mhpmevent3 的 OF，再读 scountovf bit 3 | scountovf bit 3 = 0 |
| COFPMF-SOV-05 | M-mode 读取不受 mcounteren 限制 | 清除 mcounteren bit 3，M-mode 读 scountovf bit 3 仍可见 | scountovf bit 3 反映真实 OF 值 |
| COFPMF-SOV-06 | S-mode mcounteren=1 可读 | 设置 mcounteren bit 3 = 1，S-mode 读 scountovf bit 3 | 读到真实 OF 值 |
| COFPMF-SOV-07 | S-mode mcounteren=0 读为零 | 清除 mcounteren bit 3，S-mode 读 scountovf，bit 3 应为 0（即使 OF=1） | scountovf bit 3 = 0 |
| COFPMF-SOV-08 | VS-mode 双重 gate（均允许） | mcounteren bit 3 = 1, hcounteren bit 3 = 1，VS-mode 读 scountovf bit 3 | 读到真实 OF 值 |
| COFPMF-SOV-09 | VS-mode mcounteren=0 读为零 | mcounteren bit 3 = 0（不论 hcounteren），VS-mode 读 scountovf bit 3 | scountovf bit 3 = 0 |
| COFPMF-SOV-10 | VS-mode hcounteren=0 读为零 | mcounteren bit 3 = 1, hcounteren bit 3 = 0，VS-mode 读 scountovf bit 3 | scountovf bit 3 = 0 |

```c
/* COFPMF-SOV-01 示例伪代码 */
TEST_REGISTER(test_cofpmf_scountovf_reflects_of);
bool test_cofpmf_scountovf_reflects_of(void) {
    TEST_BEGIN("COFPMF-SOV-01: scountovf reflects OF bit");

    /* 设置 mhpmevent3 的 OF=1 */
    uintptr_t orig = CSRR(mhpmevent3);
    CSRW(mhpmevent3, orig | MHPMEVENT_OF);

    /* 读取 scountovf */
    uint32_t sovf = (uint32_t)CSRR(scountovf);
    TEST_ASSERT("scountovf bit 3 set", (sovf & (1u << 3)) != 0);

    /* 清除 OF 后验证 scountovf 也清除 */
    CSRW(mhpmevent3, orig & ~MHPMEVENT_OF);
    sovf = (uint32_t)CSRR(scountovf);
    TEST_ASSERT("scountovf bit 3 cleared", (sovf & (1u << 3)) == 0);

    /* 还原 */
    CSRW(mhpmevent3, orig);
    TEST_END();
}
```

---

## 设计要点

### 1. 计数器动态发现

由于 hpmcounter3–31 的实现数量是可选的，所有测试用例在开始前必须先动态探测目标计数器是否存在：

```c
/* 探测 mhpmcounter N 是否已实现 */
static bool is_counter_implemented(unsigned n) {
    /* 尝试写入非零值 */
    csr_write(CSR_MHPMCOUNTER3 + (n - 3), 0x1);
    uintptr_t val = csr_read(CSR_MHPMCOUNTER3 + (n - 3));
    csr_write(CSR_MHPMCOUNTER3 + (n - 3), 0);
    return val != 0;
}
```

若计数器未实现，用例应使用 `TEST_SKIP()` 跳过。

### 2. 事件配置策略

不同实现的 mhpmevent 低位事件编号不同。测试应提供平台可配置的事件编号宏：

```c
/* 在 sscofpmf_helpers.h 中定义，可被平台覆盖 */
#ifndef COFPMF_EVENT_INSTRET
#define COFPMF_EVENT_INSTRET  2   /* Spike 默认：已退休指令 */
#endif
```

### 3. 中断测试的 trap handler 增强

当前 `common/trap.c` 主要处理同步异常。为支持 LCOFI 中断测试（Group 3），需要：

1. **trap handler 识别中断**：检查 `mcause` 最高位（interrupt bit），若为 1 则为中断
2. **记录中断信息**：将 interrupt cause（低位）记录到 `trap_record`
3. **LCOFIP 清除**：handler 中清除 `mip.LCOFIP` 以避免无限中断循环
4. **LCOFIE 管理**：提供 `lcofie_enable()` / `lcofie_disable()` 辅助函数

### 4. 溢出触发方法

为可靠触发溢出，推荐的方法：

```c
/* 设置计数器为接近溢出值，执行足够指令 */
CSRW(mhpmcounter3, (uintptr_t)-MARGIN);
CSRW(mhpmevent3, COFPMF_EVENT_INSTRET);  /* 开始计数 */
/* 执行 MARGIN+10 条以上指令 */
for (volatile int i = 0; i < MARGIN + 20; i++) { asm volatile("nop"); }
/* 停止计数 */
CSRW(mhpmevent3, 0);
```

`MARGIN` 建议设为 50–100，足够覆盖循环开销。

### 5. scountovf 访问控制测试（Group 4 SOV-06~10）

需要在不同特权模式下读取 `scountovf`。对于 S-mode 测试，可复用框架的 `goto_priv(PRIV_S)` + `PRIV_DO_NO_TRAP` 模式；VS-mode 测试需要 Hypervisor 扩展支持（`ENABLE_HYP`），使用 `goto_priv(PRIV_VS)` 进入 VS-mode。

---

## 框架调整清单

> [!IMPORTANT]
> 以下调整是测试实施的前置条件。

### 1. `common/encoding.h` 新增定义

```c
/* ----- HPM Event CSRs ----- */
#define CSR_MHPMEVENT3   0x323
/* ... through ... */
#define CSR_MHPMEVENT31  0x33F

/* ----- HPM Counter CSRs (M-mode read/write) ----- */
#define CSR_MHPMCOUNTER3   0xB03
/* ... through ... */
#define CSR_MHPMCOUNTER31  0xB1F

/* ----- HPM Counter CSRs (S/U-mode read-only shadow) ----- */
#define CSR_HPMCOUNTER3   0xC03
/* ... through ... */
#define CSR_HPMCOUNTER31  0xC1F

/* ----- Supervisor Count Overflow ----- */
#define CSR_SCOUNTOVF     0xDA0

/* ----- mhpmevent high-bit field masks (RV64) ----- */
#define MHPMEVENT_OF      (1ULL << 63)
#define MHPMEVENT_MINH    (1ULL << 62)
#define MHPMEVENT_SINH    (1ULL << 61)
#define MHPMEVENT_UINH    (1ULL << 60)
#define MHPMEVENT_VSINH   (1ULL << 59)
#define MHPMEVENT_VUINH   (1ULL << 58)

/* ----- LCOFI interrupt bit (bit 13) ----- */
#define MIP_LCOFIP        (1ULL << 13)
#define MIE_LCOFIE        (1ULL << 13)
#define IRQ_LCOFI         13
```

### 2. `common/csr_accessors.c` 新增 case

在 `csr_read()` 中新增：
- `0x323–0x33F`（mhpmevent3–31）
- `0xB03–0xB1F`（mhpmcounter3–31）
- `0xDA0`（scountovf，仅 read）

在 `csr_write()` 中新增：
- `0x323–0x33F`（mhpmevent3–31）
- `0xB03–0xB1F`（mhpmcounter3–31）

### 3. `common/trap.c` 中断支持

在 M-mode trap handler (`m_trap_handler`) 中增加中断分支：

```c
if (cause & CAUSE_INTERRUPT_BIT) {
    uintptr_t irq = cause & ~CAUSE_INTERRUPT_BIT;
    if (irq == IRQ_LCOFI) {
        /* 清除 LCOFIP 以防止无限循环 */
        CSRC(mip, MIP_LCOFIP);
    }
    /* 记录中断信息到 trap_record */
    trap_record.triggered = true;
    trap_record.cause = cause;
    return;  /* 中断处理完毕，不需要调整 epc */
}
```

---

## 测试文件组织

```
sscofpmf/
├── Makefile                      # 构建配置（SPIKE_ISA_EXT = _sscofpmf）
├── main.c                        # 入口，打印 banner，遍历 test_table
├── sscofpmf_helpers.h            # 辅助宏/函数（事件编号、计数器发现）
└── tests/
    ├── test_register.c           # Group 1: mhpmevent 字段读写
    ├── test_mode_filter.c        # Group 2: 特权模式过滤
    ├── test_overflow.c           # Group 3: 计数溢出与中断
    └── test_scountovf.c          # Group 4: scountovf 寄存器
```
