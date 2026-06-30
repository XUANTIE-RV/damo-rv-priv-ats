**中文 | [English](../testplan_en/Shcounterenw_test_plan_en.md)**

# Shcounterenw 扩展测试计划

本文档描述 Shcounterenw（Counter-Enable Writability for Hypervisor, Version 1.0）扩展的测试计划。Shcounterenw 扩展规定：如果实现了该扩展，则对于任何不是 read-only zero 的 `hpmcounter`，`hcounteren` 中对应的 bit 必须是可写的。

---

## 概述

RISC-V Hypervisor 扩展中，`hcounteren` 寄存器控制 VS/VU-mode（V=1）对硬件性能计数器（`cycle`、`time`、`instret`、`hpmcounter3`–`hpmcounter31`）的访问权限。当 `hcounteren` 中某个 bit 为 0 时，VS-mode 访问对应的计数器将触发 virtual-instruction exception（cause=22），前提是 `mcounteren` 中同一 bit 为 1。

然而，H 扩展基础规范（`norm:hcounteren_warl`）允许 `hcounteren` 的任何 bit 为 read-only zero。这导致 hypervisor 软件无法可靠地向 guest 授予对计数器的访问权限。

**Shcounterenw 扩展的核心约束**：

> 如果某个 `hpmcounter` 不是 read-only zero（即该计数器被实现且有意义），则 `hcounteren` 中对应的 bit **必须**是可写的（即 hypervisor 可以将其设置为 0 或 1）。

这一约束确保了：
1. Hypervisor 可以精确控制 guest（VS/VU-mode）对已实现计数器的访问权限
2. Hypervisor 可以按需授予或撤销 guest 对性能计数器的读取能力

---

## 测试范围

### 规范来源

- `SPEC/shcounterenw.adoc` — Shcounterenw Extension for Counter-Enable Writability, Version 1.0
- `SPEC/hypervisor.adoc` 第 846–877 行 — `hcounteren` 寄存器定义、WARL 语义、访问控制行为
- `SPEC/hypervisor.adoc` 第 2348–2365 行 — VS/VU-mode 计数器访问触发 virtual-instruction exception 的条件

### 关键参考文件

| 路径 | 说明 |
|------|------|
| `SPEC/shcounterenw.adoc` | Shcounterenw 规范全文（共 6 行） |
| `SPEC/hypervisor.adoc:846-877` | `hcounteren` 寄存器规范（32-bit RW WARL，访问控制逻辑） |
| `SPEC/hypervisor.adoc:2348-2365` | VS/VU-mode 计数器访问触发 virtual-instruction exception 的精确条件 |
| `common/encoding.h:275` | `CSR_HCOUNTEREN = 0x606` |
| `common/encoding.h:15` | `CSR_MCOUNTEREN = 0x306` |
| `common/encoding.h:391-450` | `CSR_MHPMCOUNTER3`–`CSR_MHPMCOUNTER31` / `CSR_HPMCOUNTER3`–`CSR_HPMCOUNTER31` |
| `common/encoding.h:305` | `CAUSE_VIRTUAL_INSTRUCTION = 22` |
| `common/hyp/hyp_priv.h:21,24` | `run_in_vs_mode(fn, arg)` / `run_in_vu_mode(fn, arg)` |
| `common/hyp/hyp_csr.h:64-66` | `hcounteren_write()` / `hcounteren_read()` / `hcounteren_set()` / `hcounteren_clear()` |
| `common/hyp/hyp_test.h:79-88` | `EXPECT_VIRTUAL_INST(stmt)` — 期望 virtual-instruction exception |
| `common/hyp/hyp_test.h:95-100` | `VS_EXPECT_NO_TRAP(stmt)` — 期望无异常 |
| `DOCS/framework/hypervisor_framework.md` | Hypervisor 测试框架总览 |
| `DOCS/testplan/sscounterenw_test_plan.md` | 同族 S-mode scounteren 测试方案，结构参照基准 |

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:shcounterenw_hpmcounter_hcounteren` | If the Shcounterenw extension is implemented, then for any `hpmcounter` that is not read-only zero, the corresponding bit in `hcounteren` must be writable. | 若实现了 Shcounterenw 扩展，则对于任何非只读零的 `hpmcounter`，`hcounteren` 中的对应位必须可写。 |
| `norm:hcounteren_sz` | The counter-enable register `hcounteren` is a 32-bit register that controls the availability of the hardware performance monitoring counters to the guest virtual machine. | `hcounteren` 是一个 32 位寄存器，控制硬件性能监控计数器对客户虚拟机的可用性。 |
| `norm:hcounteren_op` | When the CY, TM, IR, or HPMn bit in the `hcounteren` register is clear, attempts to read the corresponding counter while V=1 will cause a virtual-instruction exception if the same bit in `mcounteren` is 1. | 当 `hcounteren` 中的 CY、TM、IR 或 HPMn 位清零时，V=1 下读取对应计数器若 `mcounteren` 中同一位为 1 则引发虚拟指令异常。 |
| `norm:hcounteren_warl` | `hcounteren` must be implemented. However, any of the bits may be read-only zero, indicating reads to the corresponding counter will cause an exception when V=1. Hence, they are effectively WARL fields. | `hcounteren` 必须实现。但任何位可以为只读零，表示 V=1 时读取对应计数器会引发异常。因此它们实际上是 WARL 字段。 |
| `norm:H_virtinst_vs_nonhighctr_h0_m1` | In VS-mode, attempts to access a non-high-half counter CSR when the corresponding bit in `hcounteren` is 0 and the same bit in `mcounteren` is 1. | VS 模式下，访问非高半计数器 CSR 时 `hcounteren` 对应位为 0 且 `mcounteren` 同一位为 1。 |
| `norm:H_virtinst_vu_nonhighctr_h0_s0_m1` | In VU-mode, attempts to access a non-high-half counter CSR when the corresponding bit in either `hcounteren` or `scounteren` is 0 and the same bit in `mcounteren` is 1. | VU 模式下，访问非高半计数器 CSR 时 `hcounteren` 或 `scounteren` 对应位为 0 且 `mcounteren` 同一位为 1。 |

### 不在测试范围内

- **计数器事件计数的正确性**：Shcounterenw 仅约束 hcounteren 的可写性，不涉及计数器是否正确计数
- **scounteren 可写性**：由 Sscounterenw 测试计划覆盖（`DOCS/testplan/sscounterenw_test_plan.md`）
- **mcounteren 可写性**：mcounteren 的可写性由基础特权级规范定义，非 Shcounterenw 特有
- **Sscofpmf / Smcntrpmf（计数器溢出和模式过滤）**：由独立测试计划覆盖
- **hcounteren.TM 对 vstimecmp 的门控**：该行为由 `norm:hcounteren_acc` 定义，非 Shcounterenw 特有约束
- **RV32 / 高半部 CSR（hpmcounterNh）**：仅覆盖 RV64
- **多 hart 场景**：项目为单核测试环境
- **G-stage 地址翻译**：本测试仅使用恒等映射建立基本执行环境

---

## 前提与约束

> [!IMPORTANT]
> Shcounterenw 的验证需要先确定哪些 `hpmcounter` 是"已实现的"（不是 read-only zero）。验证策略为：在 M-mode 下尝试写 `mhpmcounter` 为非零值并回读，如果回读值非零则认为该计数器已实现。对于已实现的计数器，验证 `hcounteren` 对应 bit 的可写性。

### 计数器实现探测策略

```
for each counter index i (0..31):
    1. 对于 i=0(cycle): 几乎所有实现都提供 cycle，直接视为已实现
    2. 对于 i=1(time): 几乎所有实现都提供 time，直接视为已实现
    3. 对于 i=2(instret): 几乎所有实现都提供 instret，直接视为已实现
    4. 对于 i=3..31 (hpmcounter3-31):
       - 在 M-mode 写 mhpmcounter[i] 为非零值（如 0xDEADBEEF）
       - 回读 mhpmcounter[i]，如果回读值非零则已实现
       - 恢复原值
    5. 记录已实现的计数器位图，供后续测试使用
```

---

## 设计要点

### 1. hcounteren 与 mcounteren 的层级关系

访问控制层级链为 `mcounteren → hcounteren → scounteren`：

- **mcounteren** gates HS-mode 对计数器的访问，同时也 gate VS/VU-mode（mcounteren bit=0 时所有低特权级都无法访问）
- **hcounteren** gates VS-mode 对计数器的访问（前提 mcounteren 对应 bit=1）
- **scounteren** gates VU-mode 对计数器的访问（前提 hcounteren 对应 bit=1）

Shcounterenw 约束的是 **hcounteren** 这一层的可写性。

### 2. virtual-instruction exception vs illegal-instruction exception

- `hcounteren bit=0, mcounteren bit=1`：VS-mode 访问触发 **virtual-instruction exception (cause=22)**
- `mcounteren bit=0`：VS-mode 访问触发 **illegal-instruction exception (cause=2)**

测试中必须确保 `mcounteren` 对应 bit=1，这样 `hcounteren` 的控制效果才能被观察到（触发 cause=22 而非 cause=2）。

### 3. VS-mode 访问的框架支持

使用 `run_in_vs_mode(fn, arg)` 进入 VS-mode 执行计数器读取操作。当 `hcounteren` bit=0 时：
- VS-mode 执行 `csrr cycle/time/instret/hpmcounterN` 触发 virtual-instruction exception
- trap 会 escalate 到 HS-mode（因为 virtual-instruction exception 不会被 hedeleg 委托到 VS）
- 使用框架的 `trap_expect_begin/end` + `EXPECT_VIRTUAL_INST` 宏验证

### 4. 与 hyp_reset_state 的隔离

`common/hyp/hyp_csr.h` 提供 `hcounteren_write()`、`hcounteren_set()`、`hcounteren_clear()` API。测试开始前保存 `hcounteren`，结束时还原。`hyp_reset_state()` 中调用 `hcounteren_write(0)` 重置为全 0（所有计数器对 guest 不可见）。

### 5. VU-mode 端到端验证的额外条件

VU-mode 下访问计数器需同时满足：
- `mcounteren` bit=1
- `hcounteren` bit=1
- `scounteren` bit=1

三层都为 1 时 VU-mode 才能无异常访问。本测试 Group 4 验证 hcounteren 层的控制效果时，需确保 mcounteren=1 且 scounteren=1，仅变化 hcounteren。

---

## 测试分组

> [!IMPORTANT]
> 共 5 个测试组、20 个测试用例。Group 1 在 M-mode 验证 hcounteren bit 的基础可写性；Group 2 在 VS-mode 端到端验证 hcounteren 的访问控制效果；Group 3 验证 bit 反复切换一致性；Group 4 验证 VU-mode 下的层级交互；Group 5 记录 read-only zero 计数器的 hcounteren bit 行为。

---

### Group 1：hcounteren 可写性验证（M-mode 读写回环）

**规范依据**：
- `norm:shcounterenw_hpmcounter_hcounteren`（`SPEC/shcounterenw.adoc:4-6`）：已实现计数器对应的 hcounteren bit 必须可写

**测试职责**：在 M-mode 下，对每个已实现的 hpmcounter 对应的 hcounteren bit 进行写 1/读回、写 0/读回验证。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SHCNTW-WR-01 | cycle 对应 hcounteren[0] 可写 | 探测 cycle 是否已实现，若是则验证 hcounteren[0] 可写 1 和写 0 | 写入值回读一致 |
| SHCNTW-WR-02 | time 对应 hcounteren[1] 可写 | 探测 time 是否已实现，若是则验证 hcounteren[1] 可写 1 和写 0 | 写入值回读一致 |
| SHCNTW-WR-03 | instret 对应 hcounteren[2] 可写 | 探测 instret 是否已实现，若是则验证 hcounteren[2] 可写 1 和写 0 | 写入值回读一致 |
| SHCNTW-WR-04 | hpmcounter3–31 对应 hcounteren[3:31] 可写 | 逐个探测 hpmcounter3–31，对已实现的验证 hcounteren 对应 bit 可写 | 已实现计数器的对应 bit 写入值回读一致 |

#### 关键代码示例：SHCNTW-WR-01

```c
/* tests/test_writable.c — SHCNTW-WR-01 */

#include "test_framework.h"
#include "hyp/hyp_csr.h"

TEST_REGISTER(test_shcounterenw_cycle_bit_writable);
bool test_shcounterenw_cycle_bit_writable(void) {
    TEST_BEGIN("SHCNTW-WR-01: hcounteren[0] writable when cycle is implemented");

    /* cycle 几乎所有实现都提供，直接认为已实现 */
    uintptr_t saved = hcounteren_read();

    /* 测试 hcounteren[0] 写 1 */
    hcounteren_write(saved | (1UL << 0));
    TEST_ASSERT("hcounteren[0] can be set to 1",
                (hcounteren_read() & (1UL << 0)) != 0);

    /* 测试 hcounteren[0] 写 0 */
    hcounteren_write(hcounteren_read() & ~(1UL << 0));
    TEST_ASSERT("hcounteren[0] can be cleared to 0",
                (hcounteren_read() & (1UL << 0)) == 0);

    /* 恢复 */
    hcounteren_write(saved);
    TEST_END();
}
```

---

### Group 2：hcounteren 控制 VS-mode 访问（端到端验证）

**规范依据**：
- `norm:hcounteren_op`（`SPEC/hypervisor.adoc:856-864`）：hcounteren bit=0 + mcounteren bit=1 时，VS-mode 访问触发 virtual-instruction exception
- `norm:H_virtinst_vs_nonhighctr_h0_m1`（`SPEC/hypervisor.adoc:2351-2353`）：精确触发条件

**测试职责**：对已实现的 hpmcounter，验证 hcounteren bit 设为 1 时 VS-mode 读取成功，设为 0 时 VS-mode 读取触发 virtual-instruction exception（cause=22）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SHCNTW-ACCESS-01 | hcounteren[0]=1 时 VS-mode 读 cycle 成功 | 设置 mcounteren[0]=1, hcounteren[0]=1，VS-mode 读 cycle | 无异常 |
| SHCNTW-ACCESS-02 | hcounteren[0]=0 时 VS-mode 读 cycle 触发异常 | 设置 mcounteren[0]=1, hcounteren[0]=0，VS-mode 读 cycle | 触发 virtual-instruction exception (cause=22) |
| SHCNTW-ACCESS-03 | hcounteren[1]=1 时 VS-mode 读 time 成功 | 设置 mcounteren[1]=1, hcounteren[1]=1，VS-mode 读 time | 无异常 |
| SHCNTW-ACCESS-04 | hcounteren[1]=0 时 VS-mode 读 time 触发异常 | 设置 mcounteren[1]=1, hcounteren[1]=0，VS-mode 读 time | 触发 virtual-instruction exception (cause=22) |
| SHCNTW-ACCESS-05 | hcounteren[2]=1 时 VS-mode 读 instret 成功 | 设置 mcounteren[2]=1, hcounteren[2]=1，VS-mode 读 instret | 无异常 |
| SHCNTW-ACCESS-06 | hcounteren[2]=0 时 VS-mode 读 instret 触发异常 | 设置 mcounteren[2]=1, hcounteren[2]=0，VS-mode 读 instret | 触发 virtual-instruction exception (cause=22) |
| SHCNTW-ACCESS-07 | hcounteren[N]=1 时 VS-mode 读 hpmcounterN 成功 | 对已实现的 hpmcounterN，设置对应 bit=1，VS-mode 读取 | 无异常 |
| SHCNTW-ACCESS-08 | hcounteren[N]=0 时 VS-mode 读 hpmcounterN 触发异常 | 对已实现的 hpmcounterN，设置对应 bit=0，VS-mode 读取 | 触发 virtual-instruction exception (cause=22) |

#### 关键代码示例：SHCNTW-ACCESS-02

```c
/* tests/test_access.c — SHCNTW-ACCESS-02 */

#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"

static uintptr_t vsmode_read_cycle(uintptr_t arg) {
    (void)arg;
    uintptr_t val;
    asm volatile ("csrr %0, cycle" : "=r"(val));
    return val;
}

TEST_REGISTER(test_shcounterenw_vs_cycle_blocked);
bool test_shcounterenw_vs_cycle_blocked(void) {
    TEST_BEGIN("SHCNTW-ACCESS-02: VS-mode read cycle blocked when hcounteren[0]=0");

    /* 确保 mcounteren[0]=1（使 hcounteren 层控制生效） */
    uintptr_t saved_mcen = csr_read(CSR_MCOUNTEREN);
    csr_write(CSR_MCOUNTEREN, saved_mcen | (1UL << 0));

    /* 设置 hcounteren[0]=0 */
    uintptr_t saved_hcen = hcounteren_read();
    hcounteren_write(saved_hcen & ~(1UL << 0));

    /* VS-mode 读 cycle 应触发 virtual-instruction exception */
    trap_expect_begin();
    run_in_vs_mode(vsmode_read_cycle, 0);
    TEST_ASSERT("virtual-inst trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=22 (virtual-instruction)",
                       trap_get_cause(), (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }
    trap_expect_end();

    /* 恢复 */
    hcounteren_write(saved_hcen);
    csr_write(CSR_MCOUNTEREN, saved_mcen);
    HYP_TEST_END();
}
```

---

### Group 3：hcounteren bit 反复切换一致性

**规范依据**：
- `norm:shcounterenw_hpmcounter_hcounteren`：可写性意味着可以在 0 和 1 之间反复切换

**测试职责**：对已实现的计数器，反复设置和清除 hcounteren bit，验证每次切换后 VS-mode 访问行为一致。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SHCNTW-TOGGLE-01 | cycle 对应 bit 反复切换 | 对 hcounteren[0] 做 1→0→1→0 切换，每次验证 VS-mode 行为 | 每次切换后行为符合当前 bit 值 |
| SHCNTW-TOGGLE-02 | instret 对应 bit 反复切换 | 对 hcounteren[2] 做 1→0→1→0 切换，每次验证 VS-mode 行为 | 每次切换后行为符合当前 bit 值 |
| SHCNTW-TOGGLE-03 | hpmcounterN 对应 bit 反复切换 | 对已实现的 hpmcounterN 做切换验证 | 每次切换后行为符合当前 bit 值 |

#### 关键代码示例：SHCNTW-TOGGLE-01

```c
/* tests/test_toggle.c — SHCNTW-TOGGLE-01 */

#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"

TEST_REGISTER(test_shcounterenw_toggle_cycle);
bool test_shcounterenw_toggle_cycle(void) {
    TEST_BEGIN("SHCNTW-TOGGLE-01: hcounteren[0] toggle consistency");

    uintptr_t saved_mcen = csr_read(CSR_MCOUNTEREN);
    csr_write(CSR_MCOUNTEREN, saved_mcen | (1UL << 0));
    uintptr_t saved_hcen = hcounteren_read();

    for (int i = 0; i < 4; i++) {
        bool enable = (i % 2 == 0);

        if (enable)
            hcounteren_set(1UL << 0);
        else
            hcounteren_clear(1UL << 0);

        trap_expect_begin();
        run_in_vs_mode(vsmode_read_cycle, 0);

        if (enable) {
            TEST_ASSERT("cycle accessible after enable",
                        !trap_was_triggered());
        } else {
            TEST_ASSERT("cycle blocked after disable",
                        trap_was_triggered());
            if (trap_was_triggered()) {
                TEST_ASSERT_EQ("cause=22",
                               trap_get_cause(),
                               (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
            }
        }
        trap_expect_end();
    }

    hcounteren_write(saved_hcen);
    csr_write(CSR_MCOUNTEREN, saved_mcen);
    HYP_TEST_END();
}
```

---

### Group 4：mcounteren / hcounteren / scounteren 层级交互

**规范依据**：
- `norm:hcounteren_op`（`SPEC/hypervisor.adoc:856-864`）：mcounteren 仍然 gate VS/VU-mode 的访问
- `norm:H_virtinst_vu_nonhighctr_h0_s0_m1`（`SPEC/hypervisor.adoc:2359-2361`）：VU-mode 需 hcounteren 和 scounteren 同时为 1

**测试职责**：验证 mcounteren/hcounteren/scounteren 的层级门控关系：mcounteren=0 时即使 hcounteren=1 也无法访问；VU-mode 需三层同时为 1。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SHCNTW-HIER-01 | mcounteren[0]=0 时 VS-mode 读 cycle 异常 | mcounteren[0]=0, hcounteren[0]=1，VS-mode 读 cycle | 触发 illegal-instruction exception (cause=2) |
| SHCNTW-HIER-02 | mcounteren[0]=1, hcounteren[0]=1 时 VS-mode 读 cycle 成功 | 双层使能，VS-mode 读 cycle | 无异常 |
| SHCNTW-HIER-03 | VU-mode: mcounteren=1, hcounteren=1, scounteren=1 时读 cycle 成功 | 三层使能，VU-mode 读 cycle | 无异常 |
| SHCNTW-HIER-04 | VU-mode: mcounteren=1, hcounteren=0, scounteren=1 时读 cycle 异常 | hcounteren 阻断，VU-mode 读 cycle | 触发 virtual-instruction exception (cause=22) |
| SHCNTW-HIER-05 | VU-mode: mcounteren=1, hcounteren=1, scounteren=0 时读 cycle 异常 | scounteren 阻断，VU-mode 读 cycle | 触发 virtual-instruction exception (cause=22) |

> [!NOTE]
> **SHCNTW-HIER-01 的异常类型**：当 `mcounteren` bit=0 时，无论 `hcounteren` 如何设置，VS-mode 访问计数器触发的是 **illegal-instruction exception (cause=2)** 而非 virtual-instruction exception。这是因为 mcounteren=0 意味着"该计数器在 S-mode 以下都不可见"，H 扩展不再介入（不视为 hypervisor 层面的虚拟化拦截）。

> [!NOTE]
> **SHCNTW-HIER-04/05 的异常类型**：VU-mode 访问时，若 `hcounteren` 或 `scounteren` 对应 bit=0（且 mcounteren=1），触发的都是 virtual-instruction exception (cause=22)，因为此时 trap 的语义是"hypervisor 拦截了 guest 对计数器的访问"。

> [!IMPORTANT]
> **SHCNTW-HIER-04/05 前置条件：scounteren 可写性验证**
> SHCNTW-HIER-05 依赖 `scounteren` bit 0 可清零。但 Shcounterenw 仅保证 **hcounteren** 的可写性，`scounteren` 的可写性由 Smcntrpmf 或平台策略决定。测试实现必须在执行 HIER-04/05 前：
> 1. 尝试写 `scounteren[0] = 0` 并回读验证
> 2. 若 `scounteren[0]` 无法清零（read-only 1），则 SHCNTW-HIER-05 应执行 `TEST_SKIP("scounteren[0] is not writable on this platform")`
> 3. SHCNTW-HIER-04 不受此影响（它依赖 hcounteren=0 阻断，scounteren=1 是保持默认值）
>
> 伪代码：
> ```
> csrr saved, scounteren
> csrc scounteren, 0x1        // 尝试清除 bit 0
> csrr readback, scounteren
> if (readback & 0x1) != 0:
>     TEST_SKIP("scounteren[0] not writable")
> csrw scounteren, saved      // 恢复
> ```

#### 关键代码示例：SHCNTW-HIER-01

```c
/* tests/test_hierarchy.c — SHCNTW-HIER-01 */

#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"

TEST_REGISTER(test_shcounterenw_mcounteren_gates_vs);
bool test_shcounterenw_mcounteren_gates_vs(void) {
    TEST_BEGIN("SHCNTW-HIER-01: mcounteren[0]=0 blocks VS-mode cycle access");

    /* mcounteren[0]=0, hcounteren[0]=1 */
    uintptr_t saved_mcen = csr_read(CSR_MCOUNTEREN);
    csr_write(CSR_MCOUNTEREN, saved_mcen & ~(1UL << 0));
    uintptr_t saved_hcen = hcounteren_read();
    hcounteren_write(saved_hcen | (1UL << 0));

    /* VS-mode 读 cycle：mcounteren=0 时触发 illegal-instruction (cause=2) */
    trap_expect_begin();
    run_in_vs_mode(vsmode_read_cycle, 0);
    TEST_ASSERT("illegal-inst trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("cause=2 (illegal-instruction)",
                       trap_get_cause(), (uintptr_t)CAUSE_ILLEGAL_INSTRUCTION);
    }
    trap_expect_end();

    /* 恢复 */
    hcounteren_write(saved_hcen);
    csr_write(CSR_MCOUNTEREN, saved_mcen);
    HYP_TEST_END();
}
```

---

### Group 5：read-only zero 计数器的 hcounteren bit 行为

**规范依据**：
- `norm:hcounteren_warl`（`SPEC/hypervisor.adoc:873-876`）：hcounteren 的任何 bit 可以是 read-only zero
- Shcounterenw **不约束** read-only zero 计数器对应的 hcounteren bit

**测试职责**：对于探测为 read-only zero 的 hpmcounter，记录其 hcounteren bit 行为（可能是 read-only 0 或可写），不作 pass/fail 判断，仅作信息收集。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SHCNTW-RO-01 | read-only zero 计数器 hcounteren bit 探测 | 对所有 read-only zero 的 hpmcounter，尝试写 hcounteren bit 并报告结果 | 信息性输出，不做 pass/fail |

#### 关键代码示例：SHCNTW-RO-01

```c
/* tests/test_readonly.c — SHCNTW-RO-01 */

#include "test_framework.h"
#include "hyp/hyp_csr.h"

TEST_REGISTER(test_shcounterenw_readonly_zero_report);
bool test_shcounterenw_readonly_zero_report(void) {
    TEST_BEGIN("SHCNTW-RO-01: Report hcounteren bits for read-only-zero counters");

    uintptr_t saved_hcen = hcounteren_read();

    for (int i = 3; i <= 31; i++) {
        /* 探测 mhpmcounter[i] 是否为 read-only zero */
        uintptr_t mhpm_addr = 0xB00 + i;  /* CSR_MHPMCOUNTER3 = 0xB03 */
        uintptr_t orig = csr_read_dynamic(mhpm_addr);
        csr_write_dynamic(mhpm_addr, 0xDEADBEEFUL);
        uintptr_t readback = csr_read_dynamic(mhpm_addr);
        csr_write_dynamic(mhpm_addr, orig);

        bool is_readonly_zero = (readback == 0 && orig == 0);

        if (is_readonly_zero) {
            /* 仅报告 hcounteren bit 行为，不做 pass/fail */
            hcounteren_write(hcounteren_read() | (1UL << i));
            bool can_set = (hcounteren_read() & (1UL << i)) != 0;
            printf("  hpmcounter%d: read-only zero, hcounteren[%d] %s\n",
                   i, i, can_set ? "writable" : "hardwired-0");
        }
    }

    hcounteren_write(saved_hcen);
    TEST_END();
}
```

---

## 附录：相关常量与 API 参考

### CSR 与位定义

| 名称 | 值 | 说明 |
|------|-----|------|
| `CSR_HCOUNTEREN` | `0x606` | Hypervisor counter-enable register，见 `common/encoding.h:275` |
| `CSR_MCOUNTEREN` | `0x306` | Machine counter-enable register，见 `common/encoding.h:15` |
| `CSR_SCOUNTEREN` | `0x106` | Supervisor counter-enable register，见 `common/encoding.h:139` |
| `CSR_CYCLE` | `0xC00` | 只读 cycle 计数器（user-level） |
| `CSR_TIME` | `0xC01` | 只读 time 计数器（user-level） |
| `CSR_INSTRET` | `0xC02` | 只读 instret 计数器（user-level） |
| `CSR_HPMCOUNTER3`–`31` | `0xC03`–`0xC1F` | 只读 HPM 计数器（user-level） |
| `CSR_MHPMCOUNTER3`–`31` | `0xB03`–`0xB1F` | 可写 HPM 计数器（machine-level） |
| `CAUSE_VIRTUAL_INSTRUCTION` | `22` | Virtual-instruction exception |
| `CAUSE_ILLEGAL_INSTRUCTION` | `2` | Illegal-instruction exception |

### hcounteren bit 索引对应

| Bit | 名称 | 对应计数器 |
|-----|------|-----------|
| 0 | CY | cycle |
| 1 | TM | time |
| 2 | IR | instret |
| 3–31 | HPM3–HPM31 | hpmcounter3–hpmcounter31 |

### 测试框架 API

- `hcounteren_read()` / `hcounteren_write(value)`：hcounteren 读/写
- `hcounteren_set(mask)` / `hcounteren_clear(mask)`：hcounteren 置位/清位
- `run_in_vs_mode(fn, arg)`：在 VS-mode (V=1) 执行 fn(arg)
- `run_in_vu_mode(fn, arg)`：在 VU-mode (V=1, U) 执行 fn(arg)
- `trap_expect_begin()` / `trap_expect_end()`：trap 捕获窗口
- `trap_was_triggered()` / `trap_get_cause()`：trap 状态查询
- `EXPECT_VIRTUAL_INST(stmt)`：期望 virtual-instruction exception 宏
- `VS_EXPECT_NO_TRAP(stmt)`：期望 VS-mode 无异常宏
- `HYP_TEST_END()`：测试结束宏（含 hyp_reset_state）
- `csr_read_dynamic(addr)` / `csr_write_dynamic(addr, val)`：按地址动态读写 CSR

---

## 测试统计

| 分组 | 测试数量 | 说明 |
|------|----------|------|
| Group 1：可写性验证 | 4 | M-mode 基础读写回环 |
| Group 2：VS-mode 访问控制 | 8 | 端到端权限验证 |
| Group 3：切换一致性 | 3 | 反复切换验证 |
| Group 4：层级交互 | 5 | mcounteren/hcounteren/scounteren 门控 |
| Group 5：read-only zero 报告 | 1 | 信息收集 |
| **总计** | **21** | |

---

## 测试执行说明

### 子目录与构建集成（实施阶段参考，本计划不产出）

> [!IMPORTANT]
> 本计划阶段**仅产出 `DOCS/testplan/shcounterenw_test_plan.md`**，不创建 `shcounterenw/` 子目录、不修改顶层 `Makefile`、不写 C 测试代码。后续实施阶段按本文档创建：
> - `shcounterenw/main.c`（参考 `svadu/main.c`）
> - `shcounterenw/Makefile`（参考 `svadu/Makefile`，`SPIKE_ISA_EXT = _shcounterenw`）
> - `shcounterenw/kernel.ld`（参考 `svadu/kernel.ld`）
> - `shcounterenw/tests/test_helpers.h`（计数器探测、动态 CSR 访问辅助）
> - `shcounterenw/tests/test_writable.c`（Group 1）
> - `shcounterenw/tests/test_access.c`（Group 2）
> - `shcounterenw/tests/test_toggle.c`（Group 3）
> - `shcounterenw/tests/test_hierarchy.c`（Group 4）
> - `shcounterenw/tests/test_readonly.c`（Group 5）
> 并把 `shcounterenw` 加入顶层 `Makefile` 的 `EXTENSIONS` 列表。

### 运行环境

- Group 1：M-mode 直接操作 hcounteren（CSR 0x606），无需进入 VS-mode
- Group 2/3：通过 `run_in_vs_mode` 进入 VS-mode 验证计数器访问
- Group 4：涉及 VS-mode 和 VU-mode 的层级验证
- Group 5：M-mode 探测并报告
- 单核环境，无需 IPI

### 失败定位指引

| 现象 | 可能原因 |
|------|----------|
| SHCNTW-WR-01/02/03 失败 | Shcounterenw 未启用，hcounteren bit 仍为 read-only zero |
| SHCNTW-WR-04 中某些 bit 失败 | 对应 hpmcounter 实现探测有误（误判为已实现），或该 bit 确实是 read-only |
| SHCNTW-ACCESS-01 失败（有异常） | hcounteren_write 未生效，或 mcounteren 未正确设置 |
| SHCNTW-ACCESS-02 失败（无异常） | hcounteren bit=0 未生效，VS-mode 仍能访问计数器 |
| SHCNTW-ACCESS-02 失败（cause≠22） | mcounteren 可能为 0 导致触发 cause=2 而非 cause=22 |
| SHCNTW-HIER-01 触发 cause=22 而非 cause=2 | mcounteren 清零未生效；实现可能在 mcounteren=0 时仍走 virtual-inst 路径（不合规） |
| SHCNTW-HIER-04/05 失败（无异常） | hideleg/hedeleg 配置问题，或 scounteren/hcounteren 写入未生效 |
| SHCNTW-TOGGLE 失败 | hcounteren bit 写入存在竞争条件或缓存问题（理论上单核不应出现） |
