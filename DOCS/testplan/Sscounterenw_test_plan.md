**中文 | [English](../testplan_en/Sscounterenw_test_plan_en.md)**

# Sscounterenw 扩展测试计划

本文档描述 Sscounterenw（Counter-Enable Writability, Version 1.0）扩展的测试计划。Sscounterenw 扩展规定：如果实现了该扩展，则对于任何不是 read-only zero 的 `hpmcounter`，`scounteren` 中对应的 bit 必须是可写的。

---

## 概述

RISC-V 特权级规范中，`scounteren` 寄存器控制 S-mode 对硬件性能计数器（`hpmcounter3`–`hpmcounter31`）以及 `cycle`、`time`、`instret` 的访问权限。当 `scounteren` 中某个 bit 为 0 时，U-mode 访问对应的计数器将触发 illegal instruction exception。

然而，基础规范并未强制要求 `scounteren` 的所有 bit 都必须可写——实现可以将某些 bit 硬连线为 0 或 1。这导致软件无法可靠地控制 U-mode 对计数器的访问权限。

**Sscounterenw 扩展的核心约束**：

> 如果某个 `hpmcounter` 不是 read-only zero（即该计数器被实现且有意义），则 `scounteren` 中对应的 bit **必须**是可写的（即软件可以将其设置为 0 或 1）。

这一约束确保了：
1. 软件可以精确控制 U-mode 对已实现计数器的访问权限
2. OS 可以按需授予或撤销 U-mode 对性能计数器的读取能力

---

## 测试范围

### 规范来源

- `SPEC/sscounterenw.adoc` — Sscounterenw Extension for Counter-Enable Writability, Version 1.0

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sscounterenw_hpmcounter_scounteren` | If the Sscounterenw extension is implemented, then for any `hpmcounter` that is not read-only zero, the corresponding bit in `scounteren` must be writable. | 如果实现了 Sscounterenw 扩展，则对于任何非只读零的 `hpmcounter`，`scounteren` 中对应的位必须是可写的。 |
| `norm:scounteren_bit_set_to_1` | — | scounteren bit 可被设为 1，随后 U-mode 可无异常访问对应计数器 |
| `norm:scounteren_bit_set_to_0` | — | scounteren bit 可被设为 0，随后 U-mode 访问对应计数器触发 illegal instruction |
| `norm:scounteren_bit_toggle` | — | scounteren bit 可在 0 和 1 之间反复切换，行为一致 |
| `norm:scounteren_readonly_zero_counter` | — | 对于 read-only zero 的 hpmcounter，scounteren 对应 bit 行为不受 Sscounterenw 约束（可以是 read-only） |
| `norm:mcounteren_gate_scounteren` | — | mcounteren 仍然 gate S-mode 对计数器的访问，Sscounterenw 不改变这一层级关系 |

### 不在测试范围内

- **计数器事件计数的正确性**：Sscounterenw 仅约束 scounteren 的可写性，不涉及计数器是否正确计数
- **Sscofpmf（Count Overflow and Mode-Based Filtering）**：由独立的 sscofpmf 测试计划覆盖
- **hcounteren（Hypervisor Counter-Enable）**：VS/VU-mode 的计数器访问控制由 hypervisor 测试覆盖
- **Sv32 模式**：本计划仅覆盖 RV64
- **多 hart 场景**：项目为单核测试环境
- **mcounteren 可写性**：mcounteren 的可写性由基础特权级规范定义，非 Sscounterenw 特有

---

## 前提与约束

> [!IMPORTANT]
> Sscounterenw 的验证需要先确定哪些 `hpmcounter` 是"已实现的"（不是 read-only zero）。验证策略为：在 M-mode 下设置 `mcounteren` 对应 bit 为 1，然后在 S-mode 尝试读取 `hpmcounter`，如果读到非零值或不触发异常则认为该计数器已实现。对于已实现的计数器，验证 `scounteren` 对应 bit 的可写性。

### 关键 CSR

| CSR | 地址 | 说明 |
|-----|------|------|
| `mcounteren` | 0x306 | M-mode 控制 S-mode 对计数器的访问 |
| `scounteren` | 0x106 | S-mode 控制 U-mode 对计数器的访问 |
| `cycle` | 0xC00 | 周期计数器（只读，bit 0） |
| `time` | 0xC01 | 时钟计数器（只读，bit 1） |
| `instret` | 0xC02 | 已退休指令计数器（只读，bit 2） |
| `hpmcounter3`–`hpmcounter31` | 0xC03–0xC1F | 硬件性能计数器（只读，bit 3–31） |

### 关键参考文件

| 路径 | 说明 |
|------|------|
| `SPEC/sscounterenw.adoc` | Sscounterenw 规范全文 |
| `common/test_framework.h` | 测试框架（TEST_BEGIN / TEST_ASSERT / TEST_END） |
| `common/encoding.h` | CSR 地址定义（CSR_MCOUNTEREN / CSR_SCOUNTEREN） |

### 设计原则

1. **动态发现法**：运行时先探测哪些 hpmcounter 是已实现的（非 read-only zero），再对这些计数器验证 scounteren 可写性
2. **端到端验证**：不仅验证 scounteren bit 的读写回环，还验证设置后的实际权限控制效果（U-mode 访问成功/失败）
3. **边界覆盖**：覆盖 cycle（bit 0）、time（bit 1）、instret（bit 2）和 hpmcounter3–31（bit 3–31）

---

## 测试分组

### Group 1：scounteren 可写性验证（M-mode 读写回环）

**规范依据**：
- `norm:scounteren_writable_for_implemented_counter`：已实现计数器对应的 scounteren bit 必须可写

**测试职责**：在 M-mode 下，对每个已实现的 hpmcounter 对应的 scounteren bit 进行写 1/读回、写 0/读回验证。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCNTW-WR-01 | cycle 对应 scounteren[0] 可写 | 探测 cycle 是否已实现，若是则验证 scounteren[0] 可写 1 和写 0 | 写入值回读一致 |
| SSCNTW-WR-02 | time 对应 scounteren[1] 可写 | 探测 time 是否已实现，若是则验证 scounteren[1] 可写 1 和写 0 | 写入值回读一致 |
| SSCNTW-WR-03 | instret 对应 scounteren[2] 可写 | 探测 instret 是否已实现，若是则验证 scounteren[2] 可写 1 和写 0 | 写入值回读一致 |
| SSCNTW-WR-04 | hpmcounter3–31 对应 scounteren[3:31] 可写 | 逐个探测 hpmcounter3–31，对已实现的验证 scounteren 对应 bit 可写 | 已实现计数器的对应 bit 写入值回读一致 |

```c
/* SSCNTW-WR-01 示例 */

TEST_REGISTER(test_sscounterenw_cycle_bit_writable);
bool test_sscounterenw_cycle_bit_writable(void) {
    TEST_BEGIN("SSCNTW-WR-01: scounteren[0] writable when cycle is implemented");

    /* 先确保 mcounteren[0]=1，使 S-mode 可访问 cycle */
    uintptr_t mcen = csr_read(CSR_MCOUNTEREN);
    csr_write(CSR_MCOUNTEREN, mcen | (1UL << 0));

    /* 探测 cycle 是否已实现（读取不触发异常且值非恒零） */
    /* 对于 cycle，几乎所有实现都提供，直接认为已实现 */

    /* 测试 scounteren[0] 写 1 */
    csr_write(CSR_SCOUNTEREN, csr_read(CSR_SCOUNTEREN) | (1UL << 0));
    TEST_ASSERT("scounteren[0] can be set to 1",
                (csr_read(CSR_SCOUNTEREN) & (1UL << 0)) != 0);

    /* 测试 scounteren[0] 写 0 */
    csr_write(CSR_SCOUNTEREN, csr_read(CSR_SCOUNTEREN) & ~(1UL << 0));
    TEST_ASSERT("scounteren[0] can be cleared to 0",
                (csr_read(CSR_SCOUNTEREN) & (1UL << 0)) == 0);

    /* 恢复 mcounteren */
    csr_write(CSR_MCOUNTEREN, mcen);

    TEST_END();
}
```

---

### Group 2：scounteren 控制 U-mode 访问（端到端验证）

**规范依据**：
- `norm:scounteren_bit_set_to_1`：bit 为 1 时 U-mode 可访问
- `norm:scounteren_bit_set_to_0`：bit 为 0 时 U-mode 访问触发 illegal instruction

**测试职责**：对已实现的 hpmcounter，验证 scounteren bit 设为 1 时 U-mode 读取成功，设为 0 时 U-mode 读取触发 illegal instruction（cause=2）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCNTW-ACCESS-01 | scounteren[0]=1 时 U-mode 读 cycle 成功 | 设置 mcounteren[0]=1, scounteren[0]=1，U-mode 读 cycle | 无异常 |
| SSCNTW-ACCESS-02 | scounteren[0]=0 时 U-mode 读 cycle 触发异常 | 设置 mcounteren[0]=1, scounteren[0]=0，U-mode 读 cycle | 触发 illegal instruction (cause=2) |
| SSCNTW-ACCESS-03 | scounteren[1]=1 时 U-mode 读 time 成功 | 设置 mcounteren[1]=1, scounteren[1]=1，U-mode 读 time | 无异常 |
| SSCNTW-ACCESS-04 | scounteren[1]=0 时 U-mode 读 time 触发异常 | 设置 mcounteren[1]=1, scounteren[1]=0，U-mode 读 time | 触发 illegal instruction (cause=2) |
| SSCNTW-ACCESS-05 | scounteren[2]=1 时 U-mode 读 instret 成功 | 设置 mcounteren[2]=1, scounteren[2]=1，U-mode 读 instret | 无异常 |
| SSCNTW-ACCESS-06 | scounteren[2]=0 时 U-mode 读 instret 触发异常 | 设置 mcounteren[2]=1, scounteren[2]=0，U-mode 读 instret | 触发 illegal instruction (cause=2) |
| SSCNTW-ACCESS-07 | scounteren[N]=1 时 U-mode 读 hpmcounterN 成功 | 对已实现的 hpmcounterN，设置对应 bit=1，U-mode 读取 | 无异常 |
| SSCNTW-ACCESS-08 | scounteren[N]=0 时 U-mode 读 hpmcounterN 触发异常 | 对已实现的 hpmcounterN，设置对应 bit=0，U-mode 读取 | 触发 illegal instruction (cause=2) |

```c
/* SSCNTW-ACCESS-01 示例 */

TEST_REGISTER(test_sscounterenw_umode_cycle_allowed);
bool test_sscounterenw_umode_cycle_allowed(void) {
    TEST_BEGIN("SSCNTW-ACCESS-01: U-mode reads cycle when scounteren[0]=1");

    /* 配置：mcounteren[0]=1, scounteren[0]=1 */
    csr_set(CSR_MCOUNTEREN, 1UL << 0);
    csr_set(CSR_SCOUNTEREN, 1UL << 0);

    /* 切换到 U-mode 读取 cycle */
    goto_priv(PRIV_U);
    PRIV_DO_NO_TRAP(csr_read(CSR_CYCLE));
    goto_priv(PRIV_M);
    CHECK_NO_TRAP("U-mode read cycle with scounteren[0]=1");

    TEST_END();
}
```

---

### Group 3：scounteren bit 反复切换一致性

**规范依据**：
- `norm:scounteren_bit_toggle`：scounteren bit 可反复切换，行为一致

**测试职责**：对已实现的计数器，反复设置和清除 scounteren bit，验证每次切换后 U-mode 访问行为一致。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCNTW-TOGGLE-01 | cycle 对应 bit 反复切换 | 对 scounteren[0] 做 1→0→1→0 切换，每次验证 U-mode 行为 | 每次切换后行为符合当前 bit 值 |
| SSCNTW-TOGGLE-02 | instret 对应 bit 反复切换 | 对 scounteren[2] 做 1→0→1→0 切换，每次验证 U-mode 行为 | 每次切换后行为符合当前 bit 值 |
| SSCNTW-TOGGLE-03 | hpmcounterN 对应 bit 反复切换 | 对已实现的 hpmcounterN 做切换验证 | 每次切换后行为符合当前 bit 值 |

```c
/* SSCNTW-TOGGLE-01 示例 */

TEST_REGISTER(test_sscounterenw_toggle_cycle);
bool test_sscounterenw_toggle_cycle(void) {
    TEST_BEGIN("SSCNTW-TOGGLE-01: scounteren[0] toggle consistency");

    csr_set(CSR_MCOUNTEREN, 1UL << 0);

    for (int i = 0; i < 4; i++) {
        bool enable = (i % 2 == 0);

        if (enable)
            csr_set(CSR_SCOUNTEREN, 1UL << 0);
        else
            csr_clear(CSR_SCOUNTEREN, 1UL << 0);

        goto_priv(PRIV_U);
        PRIV_DO_TRAP(csr_read(CSR_CYCLE));
        goto_priv(PRIV_M);

        if (enable) {
            CHECK_NO_TRAP("cycle accessible after enable");
        } else {
            CHECK_TRAP("cycle blocked after disable", CAUSE_ILLEGAL_INST);
        }
    }

    TEST_END();
}
```

---

### Group 4：mcounteren 与 scounteren 层级交互

**规范依据**：
- `norm:mcounteren_gate_scounteren`：mcounteren 仍然 gate S-mode 对计数器的访问

**测试职责**：验证即使 scounteren bit 为 1，如果 mcounteren 对应 bit 为 0，S-mode 访问计数器仍触发异常（Sscounterenw 不改变层级关系）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCNTW-HIER-01 | mcounteren[0]=0 时 S-mode 读 cycle 异常 | 设置 mcounteren[0]=0, scounteren[0]=1，S-mode 读 cycle | 触发 illegal instruction (cause=2) |
| SSCNTW-HIER-02 | mcounteren[0]=1 时 S-mode 读 cycle 成功 | 设置 mcounteren[0]=1, scounteren[0]=1，S-mode 读 cycle | 无异常 |
| SSCNTW-HIER-03 | mcounteren[N]=0 时 S-mode 读 hpmcounterN 异常 | 设置 mcounteren[N]=0, scounteren[N]=1，S-mode 读取 | 触发 illegal instruction (cause=2) |
| SSCNTW-HIER-04 | mcounteren=0 scounteren=1 时 U-mode 读 cycle 异常 | mcounteren 阻断后，即使 scounteren=1，U-mode 也无法访问 | 触发 illegal instruction (cause=2) |

```c
/* SSCNTW-HIER-01 示例 */

TEST_REGISTER(test_sscounterenw_mcounteren_gates_smode);
bool test_sscounterenw_mcounteren_gates_smode(void) {
    TEST_BEGIN("SSCNTW-HIER-01: mcounteren[0]=0 blocks S-mode cycle access");

    /* mcounteren[0]=0, scounteren[0]=1 */
    csr_clear(CSR_MCOUNTEREN, 1UL << 0);
    csr_set(CSR_SCOUNTEREN, 1UL << 0);

    /* S-mode 尝试读 cycle */
    goto_priv(PRIV_S);
    PRIV_DO_TRAP(csr_read(CSR_CYCLE));
    goto_priv(PRIV_M);
    CHECK_TRAP("S-mode cycle blocked by mcounteren",
               CAUSE_ILLEGAL_INST);

    TEST_END();
}
```

---

### Group 5：read-only zero 计数器的 scounteren bit 行为

**规范依据**：
- `norm:scounteren_readonly_zero_counter`：对于 read-only zero 的 hpmcounter，Sscounterenw 不强制其 scounteren bit 可写

**测试职责**：对于探测为 read-only zero 的 hpmcounter，记录其 scounteren bit 行为（可能是 read-only 0 或 read-only 1），不作 pass/fail 判断，仅作信息收集。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCNTW-RO-01 | read-only zero 计数器 scounteren bit 探测 | 对所有 read-only zero 的 hpmcounter，尝试写 scounteren bit 并报告结果 | 信息性输出，不做 pass/fail |

```c
/* SSCNTW-RO-01 示例 */

TEST_REGISTER(test_sscounterenw_readonly_zero_report);
bool test_sscounterenw_readonly_zero_report(void) {
    TEST_BEGIN("SSCNTW-RO-01: Report scounteren bits for read-only-zero counters");

    for (int i = 3; i <= 31; i++) {
        /* 先探测 hpmcounter 是否为 read-only zero */
        csr_set(CSR_MCOUNTEREN, 1UL << i);

        /* 在 M-mode 读 mhpmcounter 判断是否为 read-only zero */
        /* 如果 mhpmcounter 写后回读仍为 0，则认为是 read-only zero */
        /* ... 探测逻辑 ... */

        if (is_readonly_zero) {
            /* 仅报告 scounteren bit 行为，不做 pass/fail */
            csr_write(CSR_SCOUNTEREN, csr_read(CSR_SCOUNTEREN) | (1UL << i));
            bool can_set = (csr_read(CSR_SCOUNTEREN) & (1UL << i)) != 0;
            printf("  hpmcounter%d: read-only zero, scounteren[%d] %s\n",
                   i, i, can_set ? "writable" : "hardwired");
        }
    }

    TEST_END();
}
```

---

## 计数器实现探测策略

由于不同实现可能支持不同的 hpmcounter 子集，测试需在运行时动态探测。探测算法如下：

```
for each counter index i (0..31):
    1. M-mode: 设置 mcounteren[i] = 1
    2. 对于 i=0(cycle), i=1(time), i=2(instret):
       - 直接在 M-mode 读取对应 CSR，如果非零则已实现
    3. 对于 i=3..31 (hpmcounter3-31):
       - 在 M-mode 写 mhpmcounter[i] 为非零值
       - 回读 mhpmcounter[i]，如果回读值非零则已实现
       - 恢复原值
    4. 记录已实现的计数器位图，供后续测试使用
```

---

## 测试统计

| 分组 | 测试数量 | 说明 |
|------|----------|------|
| Group 1：可写性验证 | 4 | 基础读写回环 |
| Group 2：U-mode 访问控制 | 8 | 端到端权限验证 |
| Group 3：切换一致性 | 3 | 反复切换验证 |
| Group 4：层级交互 | 4 | mcounteren 门控 |
| Group 5：read-only zero 报告 | 1 | 信息收集 |
| **总计** | **20** | |

---

## 框架层面需要的支持

### 需要新增的 CSR 定义（`common/encoding.h`）

当前 `encoding.h` 已有 `CSR_MCOUNTEREN`（0x306）和 `CSR_SCOUNTEREN`（0x106），但缺少以下定义：

```c
/* User-level read-only counter CSRs */
#define CSR_CYCLE       0xC00
#define CSR_TIME        0xC01
#define CSR_INSTRET     0xC02
#define CSR_HPMCOUNTER3 0xC03
#define CSR_HPMCOUNTER4 0xC04
/* ... */
#define CSR_HPMCOUNTER31 0xC1F

/* Machine-level counter CSRs (read-write) */
#define CSR_MCYCLE      0xB00
#define CSR_MINSTRET    0xB02
#define CSR_MHPMCOUNTER3  0xB03
/* ... */
#define CSR_MHPMCOUNTER31 0xB1F
```

### 需要新增的 CSR 动态访问能力

由于 `hpmcounter3`–`hpmcounter31` 有 29 个 CSR，逐一硬编码不现实。需要一个按索引读取 CSR 的机制：

```c
/**
 * read_hpmcounter - 按索引读取 hpmcounter CSR
 * @idx: 计数器索引 (0=cycle, 1=time, 2=instret, 3-31=hpmcounter3-31)
 * @return: 计数器值
 *
 * 实现方式：使用 switch-case 或函数指针表映射到具体的 csrr 指令
 */
uintptr_t read_hpmcounter(unsigned int idx);

/**
 * write_mhpmcounter - 按索引写入 mhpmcounter CSR (M-mode)
 * @idx: 计数器索引 (3-31)
 * @val: 要写入的值
 */
void write_mhpmcounter(unsigned int idx, uintptr_t val);

/**
 * read_mhpmcounter - 按索引读取 mhpmcounter CSR (M-mode)
 * @idx: 计数器索引 (3-31)
 * @return: 计数器值
 */
uintptr_t read_mhpmcounter(unsigned int idx);
```

### U-mode 执行 CSR 读取

当前框架支持 `goto_priv(PRIV_U)` + `PRIV_DO_TRAP`/`PRIV_DO_NO_TRAP` 模式。但 U-mode 下执行 `csrr` 指令读取计数器需要特别注意：

- U-mode 下 `csrr` 如果目标 CSR 被 scounteren 禁止，会触发 illegal instruction
- 需要确保 trap handler 能正确处理并跳过该指令（当前框架的 `trap_expect_begin/end` 机制已支持）

**结论**：当前框架的 trap 机制已能支持，无需修改 trap 处理逻辑。

---

## 目录结构建议

```
sscounterenw/
├── Makefile
├── main.c
├── kernel.ld              (可复用 sstvecd/kernel.ld)
└── tests/
    ├── test_helpers.h     (计数器探测、动态 CSR 访问辅助)
    ├── test_writable.c    (Group 1: 可写性验证)
    ├── test_access.c      (Group 2: U-mode 访问控制)
    ├── test_toggle.c      (Group 3: 切换一致性)
    ├── test_hierarchy.c   (Group 4: 层级交互)
    └── test_readonly.c    (Group 5: read-only zero 报告)
```
