**中文 | [English](../testplan_en/Shvstvala_test_plan_en.md)**

# Shvstvala 扩展测试计划

本文档描述 Shvstvala（Trap Value Reporting for VS-mode, Version 1.0）扩展的测试计划。Shvstvala 扩展规定：如果实现了该扩展，则 `vstval` 必须在 Sstvala 规范所描述的所有 `stval` 写入场景下被写入相同的值。即：当 trap 进入 VS-mode 时，`vstval` 必须按照 Sstvala 对 `stval` 的要求，写入故障虚拟地址或故障指令编码。

---

## 概述

RISC-V Hypervisor 扩展中，`vstval`（CSR 0x243）是 VS-mode 版本的 `stval`。当 V=1 时，VS-mode 访问 `stval` 实际操作的是 `vstval`。当 trap 被委托到 VS-mode（通过 `hedeleg`/`hideleg`）时，硬件写入 `vsepc`、`vscause` 和 `vstval`。

基础 H 扩展规范（`norm:vstval_warl`）允许 `vstval` 在某些场景下写入 0，这使得 guest OS 无法获取有意义的 trap 诊断信息。

**Shvstvala 扩展的核心约束**：

> `vstval` 必须在 Sstvala 规范（`SPEC/sstvala.adoc`）为 `stval` 描述的所有场景下被写入：
> 1. **地址类异常**（page-fault、access-fault、misaligned、非 EBREAK 的 breakpoint）→ `vstval` = 故障虚拟地址
> 2. **指令类异常**（illegal-instruction）→ `vstval` = 故障指令编码

这一约束确保了 guest OS 在 VS-mode 下处理 trap 时，能获得与 S-mode 下 Sstvala 相同质量的诊断信息。

---

## 测试范围

### 规范来源

- `SPEC/shvstvala.adoc` — Shvstvala Extension for Trap Value Reporting, Version 1.0
- `SPEC/sstvala.adoc` — Sstvala Extension for Trap Value Reporting, Version 1.0（Shvstvala 引用 Sstvala 的写入场景定义）
- `SPEC/hypervisor.adoc` 第 1364–1380 行 — `vstval` 寄存器定义
- `SPEC/hypervisor.adoc` 第 2549–2554 行 — trap 进入 VS-mode 时写 `vstval`

### 关键参考文件

| 路径 | 说明 |
|------|------|
| `SPEC/shvstvala.adoc` | Shvstvala 规范全文（共 6 行） |
| `SPEC/sstvala.adoc` | Sstvala 规范全文，定义 stval 必须写入的场景（Shvstvala 引用） |
| `SPEC/hypervisor.adoc:1364-1380` | `vstval` 寄存器规范：VSXLEN-bit RW WARL，V=1 时替代 `stval` |
| `SPEC/hypervisor.adoc:2549-2554` | `norm:H_trap_vs_csrwrites`：trap 进入 VS-mode 时写 `vsepc`、`vscause`、`vstval` |
| `common/encoding.h:293` | `CSR_VSTVAL = 0x243` |
| `common/encoding.h:143` | `CSR_STVAL = 0x143` |
| `common/hyp/hyp_priv.h:21,24` | `run_in_vs_mode(fn, arg)` / `run_in_vu_mode(fn, arg)` |
| `common/hyp/hyp_csr.h:124` | `hyp_delegate_to_vs(hedeleg_mask, hideleg_mask)` |
| `common/hyp/hyp_test.h` | Hypervisor 测试宏 |
| `common/hyp/hyp_trap.h` | HS-mode trap handler，捕获 stval（来自 VS/VU 的 trap） |
| `common/hyp/hyp_reset.c:102` | `hyp_reset_state()` 重置 `vstval` 为 0 |
| `DOCS/framework/hypervisor_framework.md` | Hypervisor 测试框架总览 |
| `DOCS/testplan/sstvala_test_plan.md` | 同族 S-mode stval 测试方案，结构参照基准 |

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:shvstvala_vstval_written` | If the Shvstvala extension is implemented, `vstval` must be written in all cases described for `stval`. | 若实现了 Shvstvala 扩展，`vstval` 必须在所有为 `stval` 描述的情况下被写入。 |
| `norm:sstvala_stval_faulting_vaddr` | If the Sstvala extension is implemented, then `stval` must be written with the faulting virtual address for load, store, and instruction page-fault, access-fault, and misaligned exceptions, and for breakpoint exceptions that are defined to write an address to stval, other than those caused by execution of the `EBREAK` or `C.EBREAK` instructions. | 如果实现了 Sstvala 扩展，则对于加载、存储和指令页错误、访问错误及非对齐异常，以及定义为向 stval 写入地址的断点异常（由 `EBREAK` 或 `C.EBREAK` 指令引起的除外），`stval` 必须写入故障虚拟地址。 |
| `norm:sstvala_stval_faulting_instruction` | For virtual-instruction and illegal-instruction exceptions, `stval` must be written with the faulting instruction. | 对于虚拟指令和非法指令异常，`stval` 必须写入故障指令。 |
| `norm:vstval_sz_acc_op` | The `vstval` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `stval`. When V=1, `vstval` substitutes for the usual `stval`. When V=0, `vstval` does not directly affect the behavior of the machine. | `vstval` 是 VSXLEN 位读写寄存器，VS 模式版本的 `stval`。V=1 时替代 `stval`；V=0 时不直接影响机器行为。 |
| `norm:vstval_warl` | `vstval` is a WARL register that must be able to hold the same set of values that `stval` can hold. | `vstval` 是一个 WARL 寄存器，必须能保持与 `stval` 相同的值集合。 |
| `norm:H_trap_vs_csrwrites` | When a trap is taken into VS-mode, `vsstatus`.SPP is set accordingly. Register `hstatus` and the HS-level `sstatus` are not modified, and V remains 1. A trap into VS-mode also writes SPIE and SIE in `vsstatus` and writes CSRs `vsepc`, `vscause`, and `vstval`. | 陷阱进入 VS 模式时，`vsstatus`.SPP 相应设置。`hstatus` 和 HS 级 `sstatus` 不修改，V 保持 1。同时写入 `vsstatus` 的 SPIE/SIE 及 CSR `vsepc`、`vscause`、`vstval`。 |

> [!IMPORTANT]
> Shvstvala 规范的核心是**引用 Sstvala 的写入场景定义**，将其约束从 `stval`（S-mode trap）扩展到 `vstval`（VS-mode trap）。所有测试用例围绕"在 VS-mode 内触发 trap → 验证 `vstval` 值"展开。关键区别是：trap 必须被委托到 VS-mode（通过 `hedeleg`），这样硬件才会写 `vstval` 而非 `stval`。

### 不在测试范围内

- **`stval` 的 S-mode 行为**：由 Sstvala 测试计划覆盖（`DOCS/testplan/sstvala_test_plan.md`）
- **`htval`（Hypervisor trap value）**：由 Shtvala 测试计划覆盖（`DOCS/testplan/shtvala_test_plan.md`）
- **Guest page-fault（cause 20/21/23）**：这些 trap 不会被委托到 VS-mode（VS-mode 不感知 G-stage），由 HS-mode 处理
- **EBREAK / C.EBREAK 导致的 breakpoint**：Sstvala 规范明确排除
- **多 hart 场景**：项目为单核测试环境
- **Sv32 / Sv48 / Sv57 模式**：仅覆盖 RV64 + VS-stage Sv39
- **V=0 时 vstval 的直接读写**：vstval 在 V=0 时不直接影响机器行为，Group 5 仅验证透传语义

---

## 设计要点

### 1. Trap 委托到 VS-mode 的机制

Shvstvala 验证的前提是 trap 必须**进入 VS-mode**，这样硬件才会写 `vstval`。委托链：

```
M-mode ──medeleg──> HS-mode ──hedeleg──> VS-mode
```

- `medeleg`：把异常从 M-mode 委托到 HS-mode
- `hedeleg`：把异常从 HS-mode 进一步委托到 VS-mode

对于每种异常类型，需设置 `hedeleg` 中对应 bit 为 1：
- page-fault (12/13/15)：`hedeleg` bit 12、13、15
- access-fault (1/5/7)：`hedeleg` bit 1、5、7
- misaligned (0/4/6)：`hedeleg` bit 0、4、6
- illegal-instruction (2)：`hedeleg` bit 2
- breakpoint (3)：`hedeleg` bit 3

### 2. VS-mode 内 trap 的捕获与 vstval 读取

当 trap 被委托到 VS-mode 后：
- 硬件写 `vscause`、`vsepc`、`vstval`
- 跳转到 `vstvec` 指向的 trap entry

验证方案：在 VS-mode 中预设自定义 trap handler（通过 `vstvec`/stvec in V=1），该 handler：
1. 读取 `stval`（V=1 时实际读 `vstval`）→ 保存到全局变量 `g_shvstvala_vstval`
2. 读取 `scause`（V=1 时实际读 `vscause`）→ 保存到 `g_shvstvala_cause`
3. 推进 `sepc`（V=1 时实际写 `vsepc`）跳过故障指令
4. `sret` 返回

### 3. VS-mode trap entry 的设计

```asm
.section .text
.globl  shvstvala_trap_entry
.align  2
shvstvala_trap_entry:
    csrrw   t0, sscratch, t0       /* V=1 时实际操作 vsscratch */
    /* 读取 stval (V=1 时实际读 vstval) */
    csrr    t1, stval
    la      t0, g_shvstvala_vstval
    sd      t1, 0(t0)
    /* 读取 scause (V=1 时实际读 vscause) */
    csrr    t1, scause
    la      t0, g_shvstvala_cause
    sd      t1, 0(t0)
    /* sepc += 4 (简化：假设 4 字节指令; 实际应判断指令长度) */
    csrr    t1, sepc
    addi    t1, t1, 4
    csrw    sepc, t1
    csrrw   t0, sscratch, t0
    sret
```

### 4. VS-stage 页表配置

VS-mode 内的 page-fault 测试需要 VS-stage 页表（`vsatp`）：
- 使用 `two_stage_run_in_vs()` 或手动配置 `vsatp` + `hgatp` 两阶段翻译
- VS-stage：Sv39 页表，部分 VA 不映射或权限受限 → 触发 VS-level page-fault
- G-stage：Sv39x4 恒等映射，确保 GPA→PA 正常翻译

### 5. Access-Fault 在 VS-mode 的触发

VS-mode 下的 access-fault 触发方式：
- **PMP 限制**：PMP 在 M-mode 层面工作，对 VS-mode 同样有效。配置 PMP 禁止特定 PA 区域的读/写/执行，VS-mode 访问该区域时触发 access-fault
- 前提：VS-stage + G-stage 翻译的最终 PA 落在 PMP 禁止区域内

### 6. Illegal Instruction 在 VS-mode 的触发

VS-mode 中执行非法指令（如未实现的 opcode、写只读 CSR）会触发 illegal-instruction exception（cause=2）。当 `hedeleg` bit 2 = 1 时，该异常委托到 VS-mode，`vstval` 应写入故障指令编码。

### 7. 与 Sstvala 测试计划的对称性

本测试计划的 Group 结构与 `sstvala_test_plan.md` 对称，主要差异：
- 执行环境从 S-mode 变为 VS-mode
- trap 目标从 S-mode handler 变为 VS-mode handler（自定义 vstvec）
- 验证的 CSR 从 `stval` 变为 `vstval`（通过 V=1 时的 stval 指令名访问）
- 需要额外的 hedeleg 委托配置和 G-stage 页表

---

## 测试分组

> [!IMPORTANT]
> 共 5 个测试组、20 个测试用例。所有测试在 VS-mode (V=1) 内触发异常，异常通过 `hedeleg` 委托到 VS-mode，由自定义 VS-mode trap handler 捕获 `vstval`。回到 M/HS-mode 后执行断言。

---

### Group 1：Page-Fault 地址类异常（vstval = 故障虚拟地址）

**规范依据**：
- `norm:shvstvala_vstval_written`（`SPEC/shvstvala.adoc:4-6`）：vstval 必须在 Sstvala 为 stval 描述的场景下被写入
- `norm:sstvala_stval_faulting_vaddr`（`SPEC/sstvala.adoc:4-9`）：page-fault 时写入故障虚拟地址

**测试职责**：验证在 VS-mode 中，当 VS-stage 翻译触发 page-fault 时，`vstval` 等于触发异常的 guest 虚拟地址。

**前置条件**：
- `hedeleg` 委托 page-fault（bit 12/13/15）到 VS-mode
- VS-stage 页表（vsatp = Sv39）配置：代码/栈区域恒等映射，测试目标 VA 未映射或权限受限
- G-stage 页表（hgatp = Sv39x4）恒等映射，确保 GPA→PA 正常
- VS-mode 自定义 trap handler 通过 vstvec 设置

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSTVAL-LPF-01 | load page-fault：未映射 VA | VS-mode 对 VS-stage 未映射的 VA 执行 `ld`，触发 load page-fault（cause=13） | `g_shvstvala_cause == 13`；`g_shvstvala_vstval == unmapped_va` |
| VSTVAL-SPF-01 | store page-fault：只读页写入 | VS-stage PTE：V=1, R=1, W=0，VS-mode 对该 VA 执行 `sd`，触发 store page-fault（cause=15） | `g_shvstvala_cause == 15`；`g_shvstvala_vstval == test_va` |
| VSTVAL-IPF-01 | instruction page-fault：不可执行页 fetch | VS-stage PTE：V=1, R=1, W=0, X=0，VS-mode 跳转到该 VA 取指，触发 instruction page-fault（cause=12） | `g_shvstvala_cause == 12`；`g_shvstvala_vstval == target_pc` |
| VSTVAL-LPF-02 | load page-fault：不同地址验证 | 同 LPF-01 但使用不同的未映射 VA，确认 vstval 跟随实际访问地址变化 | `g_shvstvala_cause == 13`；`g_shvstvala_vstval == different_va` |
| VSTVAL-SPF-02 | store page-fault：未映射 VA | VS-mode 对未映射 VA 执行 `sd`，触发 store page-fault（cause=15） | `g_shvstvala_cause == 15`；`g_shvstvala_vstval == unmapped_va` |

#### 关键代码示例：VSTVAL-LPF-01

```c
/* tests/test_pagefault.c — VSTVAL-LPF-01 */

#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"
#include "hyp/two_stage.h"

extern void shvstvala_trap_entry(void);
extern volatile uintptr_t g_shvstvala_vstval;
extern volatile uintptr_t g_shvstvala_cause;

#define UNMAPPED_VA  0x40000000UL

static uintptr_t vsmode_load_unmapped(uintptr_t addr) {
    /* 先设置 vstvec（V=1 时通过 stvec 指令名） */
    uintptr_t entry = (uintptr_t)&shvstvala_trap_entry;
    asm volatile ("csrw stvec, %0" :: "r"(entry));
    /* 触发 load page-fault */
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    (void)*ptr;
    return 0;
}

TEST_REGISTER(test_shvstvala_load_pagefault_vstval);
bool test_shvstvala_load_pagefault_vstval(void) {
    TEST_BEGIN("VSTVAL-LPF-01: load page-fault vstval == faulting VA");

    /* 委托 load page-fault (cause=13) 到 VS-mode */
    hyp_delegate_to_vs(1UL << 13, 0);

    g_shvstvala_vstval = 0;
    g_shvstvala_cause  = 0;

    /* 配置两阶段翻译：
     * - VS-stage (vsatp): Sv39，代码/栈恒等映射，UNMAPPED_VA 不映射
     * - G-stage (hgatp): Sv39x4 恒等映射 */
    two_stage_context_t ts_ctx;
    two_stage_init(&ts_ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    two_stage_identity_map_all(&ts_ctx);
    /* UNMAPPED_VA 在 VS-stage 不映射（不调用 vs_stage_map） */

    two_stage_run_in_vs(&ts_ctx, vsmode_load_unmapped, UNMAPPED_VA);

    /* 验证 vstval == 故障虚拟地址 */
    TEST_ASSERT_EQ("vscause == load page-fault (13)",
                   g_shvstvala_cause, (uintptr_t)CAUSE_LOAD_PAGE_FAULT);
    TEST_ASSERT_EQ("vstval == faulting VA",
                   g_shvstvala_vstval, UNMAPPED_VA);

    hyp_undelegate();
    HYP_TEST_END();
}
```

---

### Group 2：Access-Fault 地址类异常（vstval = 故障虚拟地址）

**规范依据**：
- `norm:shvstvala_vstval_written`（`SPEC/shvstvala.adoc:4-6`）
- `norm:sstvala_stval_faulting_vaddr`（`SPEC/sstvala.adoc:4-9`）：access-fault 时写入故障虚拟地址

**测试职责**：验证在 VS-mode 中，当 PMP 限制导致 access-fault 时，`vstval` 等于被拒绝访问的 guest 虚拟地址。

**前置条件**：
- `hedeleg` 委托 access-fault（bit 1/5/7）到 VS-mode
- PMP 配置：特定 PA 区域对 S/VS-mode 禁止访问
- 两阶段翻译确保最终 PA 落在 PMP 禁止区域

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSTVAL-LAF-01 | load access-fault：PMP 禁读区域 | PMP 禁止特定 PA 读取，VS-mode 通过 VA→GPA→PA 映射到该区域执行 `ld` | `g_shvstvala_cause == 5`；`g_shvstvala_vstval == target_va` |
| VSTVAL-SAF-01 | store access-fault：PMP 禁写区域 | PMP 禁止特定 PA 写入，VS-mode 对该区域执行 `sd` | `g_shvstvala_cause == 7`；`g_shvstvala_vstval == target_va` |
| VSTVAL-IAF-01 | instruction access-fault：PMP 禁执行区域 | PMP 禁止特定 PA 执行，VS-mode 跳转到该区域取指 | `g_shvstvala_cause == 1`；`g_shvstvala_vstval == target_pc` |

#### 关键代码示例：VSTVAL-LAF-01

```c
/* tests/test_accessfault.c — VSTVAL-LAF-01 */

#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"
#include "hyp/two_stage.h"

extern void shvstvala_trap_entry(void);
extern volatile uintptr_t g_shvstvala_vstval;
extern volatile uintptr_t g_shvstvala_cause;

#define PMP_NOREAD_PA    0x80600000UL
#define TEST_VA_LAF      0x80600000UL  /* VA == GPA == PA (恒等映射) */

static uintptr_t vsmode_load_pmp_blocked(uintptr_t addr) {
    uintptr_t entry = (uintptr_t)&shvstvala_trap_entry;
    asm volatile ("csrw stvec, %0" :: "r"(entry));
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    (void)*ptr;  /* 触发 load access-fault */
    return 0;
}

TEST_REGISTER(test_shvstvala_load_accessfault_vstval);
bool test_shvstvala_load_accessfault_vstval(void) {
    TEST_BEGIN("VSTVAL-LAF-01: load access-fault vstval == faulting VA");

    /* 委托 load access-fault (cause=5) 到 VS-mode */
    hyp_delegate_to_vs(1UL << 5, 0);

    /* PMP 配置：PMP_NOREAD_PA 区域禁读 */
    pmp_clear_all();
    pmp_entry_t e0 = PMP_ENTRY_NAPOT(PMP_NOREAD_PA, 0x1000, PMP_W);
    pmp_set_entry(0, &e0);
    pmp_entry_t e1 = PMP_ENTRY_NAPOT(0x0, 0x100000000UL, PMP_RWX);
    pmp_set_entry(1, &e1);

    g_shvstvala_vstval = 0;
    g_shvstvala_cause  = 0;

    /* 两阶段恒等映射 */
    two_stage_context_t ts_ctx;
    two_stage_init(&ts_ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);
    two_stage_identity_map_all(&ts_ctx);

    two_stage_run_in_vs(&ts_ctx, vsmode_load_pmp_blocked, TEST_VA_LAF);

    TEST_ASSERT_EQ("vscause == load access-fault (5)",
                   g_shvstvala_cause, (uintptr_t)CAUSE_LOAD_ACCESS_FAULT);
    TEST_ASSERT_EQ("vstval == faulting VA",
                   g_shvstvala_vstval, TEST_VA_LAF);

    pmp_clear_all();
    hyp_undelegate();
    HYP_TEST_END();
}
```

---

### Group 3：Misaligned 地址类异常（vstval = 故障虚拟地址）

**规范依据**：
- `norm:shvstvala_vstval_written`（`SPEC/shvstvala.adoc:4-6`）
- `norm:sstvala_stval_faulting_vaddr`（`SPEC/sstvala.adoc:4-9`）：misaligned 异常时写入故障虚拟地址

**测试职责**：验证在 VS-mode 中，当 load/store/instruction misaligned 异常被触发并委托到 VS-mode 时，`vstval` 等于未对齐的访问地址。

**前置条件**：
- `hedeleg` 委托 misaligned 异常（bit 0/4/6）到 VS-mode
- 平台不支持硬件非对齐访问（否则 load/store misaligned 用例跳过）
- instruction misaligned 通过跳转到奇数地址触发

> [!WARNING]
> 大多数现代 RISC-V 实现在默认配置下支持硬件非对齐 load/store 访问。VSTVAL-LMA-01 和 VSTVAL-SMA-01 在这些平台上会被跳过。VSTVAL-IMA-01（instruction misaligned）通常可以在任何平台上测试。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSTVAL-IMA-01 | instruction misaligned：跳转到奇数地址 | VS-mode 使用 `jalr` 跳转到奇数地址，触发 instruction address misaligned（cause=0） | `g_shvstvala_cause == 0`；`g_shvstvala_vstval == (target | 1)` |
| VSTVAL-LMA-01 | load misaligned：非对齐 load | VS-mode 对非 8 字节对齐地址执行 `ld`，若平台不支持非对齐则触发 cause=4 | `g_shvstvala_cause == 4`；`g_shvstvala_vstval == misaligned_addr`；若平台支持非对齐则 `TEST_SKIP` |
| VSTVAL-SMA-01 | store misaligned：非对齐 store | VS-mode 对非 8 字节对齐地址执行 `sd`，若平台不支持非对齐则触发 cause=6 | `g_shvstvala_cause == 6`；`g_shvstvala_vstval == misaligned_addr`；若平台支持非对齐则 `TEST_SKIP` |

#### 关键代码示例：VSTVAL-IMA-01

```c
/* tests/test_misaligned.c — VSTVAL-IMA-01 */

#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"

extern void shvstvala_trap_entry(void);
extern volatile uintptr_t g_shvstvala_vstval;
extern volatile uintptr_t g_shvstvala_cause;

static uintptr_t vsmode_jump_misaligned(uintptr_t target) {
    uintptr_t entry = (uintptr_t)&shvstvala_trap_entry;
    asm volatile ("csrw stvec, %0" :: "r"(entry));
    /* 跳转到奇数地址（非 2 字节对齐）→ instruction misaligned */
    asm volatile ("jalr zero, %0, 0" :: "r"(target) : "memory");
    return 0;
}

TEST_REGISTER(test_shvstvala_inst_misaligned_vstval);
bool test_shvstvala_inst_misaligned_vstval(void) {
    TEST_BEGIN("VSTVAL-IMA-01: instruction misaligned vstval == faulting addr");

    /* 委托 instruction address misaligned (cause=0) 到 VS-mode */
    hyp_delegate_to_vs(1UL << 0, 0);

    g_shvstvala_vstval = 0;
    g_shvstvala_cause  = ~0UL;

    /* 构造奇数目标地址 */
    extern void _start(void);
    uintptr_t target = ((uintptr_t)&_start) | 0x1UL;

    run_in_vs_mode(vsmode_jump_misaligned, target);

    TEST_ASSERT_EQ("vscause == instruction address misaligned (0)",
                   g_shvstvala_cause, (uintptr_t)CAUSE_INST_ADDR_MISALIGN);
    TEST_ASSERT_EQ("vstval == misaligned target address",
                   g_shvstvala_vstval, target);

    hyp_undelegate();
    HYP_TEST_END();
}
```

---

### Group 4：Illegal Instruction 指令类异常（vstval = 故障指令编码）

**规范依据**：
- `norm:shvstvala_vstval_written`（`SPEC/shvstvala.adoc:4-6`）
- `norm:sstvala_stval_faulting_instruction`（`SPEC/sstvala.adoc:11-13`）：illegal-instruction 异常时写入故障指令编码

**测试职责**：验证在 VS-mode 中执行非法指令触发 illegal-instruction 异常（cause=2）并委托到 VS-mode 时，`vstval` 等于故障指令的编码值。

**前置条件**：
- `hedeleg` bit 2 = 1，委托 illegal-instruction 到 VS-mode
- VS-mode 中预置已知编码的非法指令，跳转执行

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSTVAL-ILL-01 | 32 位非法指令：custom-0 操作码 | VS-mode 执行 `0x0000000B`（custom-0），触发 illegal-instruction（cause=2） | `g_shvstvala_cause == 2`；`g_shvstvala_vstval == 0x0000000B` |
| VSTVAL-ILL-02 | 32 位非法指令：写只读 CSR | VS-mode 执行 `0xC0001073`（`csrrw x0, cycle, x0`，写只读 CSR），触发 cause=2 | `g_shvstvala_cause == 2`；`g_shvstvala_vstval == 0xC0001073` |
| VSTVAL-ILL-03 | 32 位非法指令：访问不存在的 CSR | VS-mode 执行 `0xFFF022F3`（`csrrs x5, 0xFFF, x0`），触发 cause=2 | `g_shvstvala_cause == 2`；`g_shvstvala_vstval == 0xFFF022F3` |
| VSTVAL-ILL-04 | 16 位压缩非法指令 | VS-mode 执行全零 16 位编码 `0x0000`（非法压缩指令），触发 cause=2 | `g_shvstvala_cause == 2`；`g_shvstvala_vstval == 0x0000` |
| VSTVAL-ILL-05 | 连续两次非法指令 vstval 不同 | VS-mode 依次执行 `0x0000000B` 和 `0xFFF022F3`，验证 vstval 每次跟随 | 第 1 次 `vstval == 0x0000000B`；第 2 次 `vstval == 0xFFF022F3` |

> [!NOTE]
> **指令编码说明**：
> - `0x0000000B`：bits[1:0]=11 表示 32 位指令，opcode[6:0]=0001011 为 custom-0 操作码（通常未实现）
> - `0xC0001073`：`csrrw x0, 0xC00, x0` — 写入只读 CSR `cycle`（0xC00）
> - `0xFFF022F3`：`csrrs x5, 0xFFF, x0` — 访问不存在的 CSR 0xFFF
> - `0x0000`：C 扩展中全零 16 位编码，defined as illegal

#### 关键代码示例：VSTVAL-ILL-01

```c
/* tests/test_illegal.c — VSTVAL-ILL-01 */

#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"

extern void shvstvala_trap_entry(void);
extern volatile uintptr_t g_shvstvala_vstval;
extern volatile uintptr_t g_shvstvala_cause;

/* 预置非法指令 */
static uint32_t illegal_custom0 __attribute__((aligned(4))) = 0x0000000B;

static uintptr_t vsmode_exec_illegal(uintptr_t addr) {
    uintptr_t entry = (uintptr_t)&shvstvala_trap_entry;
    asm volatile ("csrw stvec, %0" :: "r"(entry));
    /* 跳转到预置的非法指令地址执行 */
    void (*fn)(void) = (void (*)(void))addr;
    fn();
    return 0;
}

TEST_REGISTER(test_shvstvala_illegal_custom0_vstval);
bool test_shvstvala_illegal_custom0_vstval(void) {
    TEST_BEGIN("VSTVAL-ILL-01: illegal custom-0 vstval == instruction encoding");

    /* 委托 illegal-instruction (cause=2) 到 VS-mode */
    hyp_delegate_to_vs(1UL << 2, 0);

    g_shvstvala_vstval = 0;
    g_shvstvala_cause  = 0;

    uintptr_t addr = (uintptr_t)&illegal_custom0;
    run_in_vs_mode(vsmode_exec_illegal, addr);

    TEST_ASSERT_EQ("vscause == illegal-instruction (2)",
                   g_shvstvala_cause, (uintptr_t)CAUSE_ILLEGAL_INST);
    TEST_ASSERT_EQ("vstval == 0x0000000B (32-bit instruction encoding)",
                   g_shvstvala_vstval, 0x0000000BUL);

    hyp_undelegate();
    HYP_TEST_END();
}
```

---

### Group 5：vstval 透传验证（V=1 时 stval 访问实际操作 vstval）

**规范依据**：
- `norm:vstval_sz_acc_op`（`SPEC/hypervisor.adoc:1366-1372`）：When V=1, `vstval` substitutes for the usual `stval`
- `norm:vstval_warl`（`SPEC/hypervisor.adoc:1374-1376`）：vstval 是 WARL，必须能持有与 stval 相同的值集合

**测试职责**：验证 V=1 时通过 `stval` 指令名写入的值能从 M/HS-mode 通过 `vstval`（CSR 0x243）读回；验证 trap 后 vstval 的值不被 VS-mode 外的操作覆盖。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSTVAL-TRANS-01 | VS-mode 写 stval → M-mode 读 vstval | VS-mode 写 `stval = 0xDEADBEEF`，返回 M-mode 后读 `vstval`（CSR 0x243） | `vstval == 0xDEADBEEF` |
| VSTVAL-TRANS-02 | M-mode 写 vstval → VS-mode 读 stval | M-mode 写 `vstval = 0xCAFE0000`，进入 VS-mode 后读 `stval`（V=1） | VS-mode 读回 == `0xCAFE0000` |
| VSTVAL-TRANS-03 | VS-mode trap 后 vstval 不被清零 | VS-mode 触发 trap，VS-mode handler 读 vstval；返回 M-mode 后再次读 vstval 确认值未被覆盖 | M-mode 读 vstval == 预期值（trap 时写入的故障信息） |
| VSTVAL-TRANS-04 | vstval 持有地址级宽值 | M-mode 写 vstval = 高位地址（如 `0x7FFFFFFFFF`），回读确认 WARL 不截断 | 回读 == 写入值 |

#### 关键代码示例：VSTVAL-TRANS-01

```c
/* tests/test_transparent.c — VSTVAL-TRANS-01 */

#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"

static uintptr_t vsmode_write_stval(uintptr_t val) {
    /* V=1：csrw stval 实际写 vstval */
    asm volatile ("csrw stval, %0" :: "r"(val));
    return 0;
}

TEST_REGISTER(test_shvstvala_transparent_vs_write);
bool test_shvstvala_transparent_vs_write(void) {
    TEST_BEGIN("VSTVAL-TRANS-01: VS writes stval, M reads vstval");

    uintptr_t saved;
    asm volatile ("csrr %0, 0x243" : "=r"(saved));

    uintptr_t test_val = 0xDEADBEEFUL;
    run_in_vs_mode(vsmode_write_stval, test_val);

    uintptr_t readback;
    asm volatile ("csrr %0, 0x243" : "=r"(readback));
    TEST_ASSERT_EQ("vstval == value written by VS via stval",
                   readback, test_val);

    asm volatile ("csrw 0x243, %0" :: "r"(saved));
    HYP_TEST_END();
}
```

---

### Group 6：Breakpoint 场景（非 EBREAK 的断点异常）

**规范依据**：
- `norm:sstvala_stval_faulting_vaddr`（`SPEC/sstvala.adoc:4-9`）：stval must be written with the faulting virtual address for ... breakpoint exceptions that are defined to write an address to stval, other than those caused by execution of the `EBREAK` or `C.EBREAK` instructions.
- `norm:shvstvala_vstval_written`（`SPEC/shvstvala.adoc:4-6`）：vstval 必须在 Sstvala 为 stval 描述的所有场景下被写入

**测试职责**：验证 VS-mode 触发地址匹配断点（非 EBREAK）时 vstval 被正确写入断点地址；验证 EBREAK/C.EBREAK 触发的 breakpoint 不属于 Shvstvala 的写入要求。

**前置条件**：
- Sdtrig（Debug Trigger）扩展必须实现，否则 VSTVAL-BRK-01 应 `TEST_SKIP`
- `hedeleg` bit 3（breakpoint）已配置委托到 VS-mode

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSTVAL-BRK-01 | 地址匹配断点写入 vstval | 配置 trigger type 6（mcontrol6）地址匹配断点指向 VS-mode 中特定地址；VS-mode 执行到该地址触发 breakpoint（cause=3） | `vstval == 断点地址`；若 Sdtrig 未实现则 `TEST_SKIP` |
| VSTVAL-BRK-02 | EBREAK 不要求写入故障地址 | VS-mode 执行 `EBREAK` 指令触发 breakpoint（cause=3），检查 vstval | vstval 的值不由 Shvstvala 约束（可为 0 或 EBREAK 的 PC，取决于实现）；仅验证 trap 正常发生且 cause==3 |

> [!NOTE]
> VSTVAL-BRK-01 依赖 Sdtrig 扩展提供的 trigger 机制。如果平台不支持 trigger type 2 或 type 6（mcontrol/mcontrol6），测试应通过读取 `tinfo` CSR 探测支持的 trigger 类型，不支持时执行 `TEST_SKIP`。
> VSTVAL-BRK-02 验证的是 Sstvala 的排除条款（"other than those caused by execution of the EBREAK or C.EBREAK instructions"），确认 EBREAK 场景下 vstval 不受 Shvstvala 约束。

#### 关键代码示例：VSTVAL-BRK-01

```c
/* tests/test_breakpoint.c — VSTVAL-BRK-01 */

#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"
#include "hyp/hyp_trap.h"

#define CSR_TSELECT    0x7A0
#define CSR_TDATA1     0x7A1
#define CSR_TDATA2     0x7A2
#define CSR_TINFO      0x7A5

#define CAUSE_BREAKPOINT  3

/* mcontrol6 type=6, DMODE=0, EXECUTE=1, M=0, S=0, VS=1 */
#define MCONTROL6_TYPE6    (6UL << 60)
#define MCONTROL6_EXECUTE  (1UL << 2)
#define MCONTROL6_VS       (1UL << 24)
#define MCONTROL6_MATCH_EQ (0UL << 7)

extern volatile uintptr_t g_shvstvala_vstval;
extern volatile uintptr_t g_shvstvala_cause;

static void vs_breakpoint_target(void) {
    /* 此函数地址将被配置为断点目标 */
    asm volatile ("nop");
}

static uintptr_t vsmode_trigger_breakpoint(uintptr_t target_addr) {
    /* VS-mode：跳转到 target_addr 触发断点 */
    void (*fn)(void) = (void (*)(void))target_addr;
    fn();
    return 0;
}

TEST_REGISTER(test_shvstvala_breakpoint_addr_match);
bool test_shvstvala_breakpoint_addr_match(void) {
    TEST_BEGIN("VSTVAL-BRK-01: address-match breakpoint writes vstval");

    /* 探测 Sdtrig 支持 */
    uintptr_t tinfo;
    asm volatile ("csrr %0, 0x7A5" : "=r"(tinfo));
    if ((tinfo & (1UL << 6)) == 0 && (tinfo & (1UL << 2)) == 0) {
        TEST_SKIP("platform does not support mcontrol6 (type 6) or mcontrol (type 2)");
    }

    /* 配置 trigger：在 vs_breakpoint_target 地址触发 */
    uintptr_t target = (uintptr_t)&vs_breakpoint_target;
    asm volatile ("csrw 0x7A0, zero");  /* tselect = 0 */
    uintptr_t tdata1_val = MCONTROL6_TYPE6 | MCONTROL6_EXECUTE |
                           MCONTROL6_VS | MCONTROL6_MATCH_EQ;
    asm volatile ("csrw 0x7A1, %0" :: "r"(tdata1_val));
    asm volatile ("csrw 0x7A2, %0" :: "r"(target));

    /* 委托 breakpoint 到 VS-mode */
    hyp_delegate_to_vs((1UL << CAUSE_BREAKPOINT), 0);

    g_shvstvala_vstval = 0;
    g_shvstvala_cause = 0;
    run_in_vs_mode(vsmode_trigger_breakpoint, target);

    TEST_ASSERT_EQ("cause == breakpoint (3)",
                   g_shvstvala_cause, CAUSE_BREAKPOINT);
    TEST_ASSERT_EQ("vstval == breakpoint address",
                   g_shvstvala_vstval, target);

    /* 清除 trigger */
    asm volatile ("csrw 0x7A1, zero");
    hyp_undelegate();
    HYP_TEST_END();
}
```

---

## 附录：相关常量与 API 参考

### 异常 Cause 值

| 名称 | 值 | 说明 | vstval 含义（Shvstvala） |
|------|-----|------|------|
| `CAUSE_INST_ADDR_MISALIGN` | 0 | 指令地址未对齐 | 故障虚拟地址 |
| `CAUSE_INST_ACCESS_FAULT` | 1 | 指令访问故障（PMP） | 故障虚拟地址 |
| `CAUSE_ILLEGAL_INST` | 2 | 非法指令 | 故障指令编码 |
| `CAUSE_BREAKPOINT` | 3 | 断点（非 EBREAK 时） | 故障地址 |
| `CAUSE_LOAD_ADDR_MISALIGN` | 4 | Load 地址未对齐 | 故障虚拟地址 |
| `CAUSE_LOAD_ACCESS_FAULT` | 5 | Load 访问故障（PMP） | 故障虚拟地址 |
| `CAUSE_STORE_ADDR_MISALIGN` | 6 | Store 地址未对齐 | 故障虚拟地址 |
| `CAUSE_STORE_ACCESS_FAULT` | 7 | Store 访问故障（PMP） | 故障虚拟地址 |
| `CAUSE_INST_PAGE_FAULT` | 12 | 指令页面故障 | 故障虚拟地址 |
| `CAUSE_LOAD_PAGE_FAULT` | 13 | Load 页面故障 | 故障虚拟地址 |
| `CAUSE_STORE_PAGE_FAULT` | 15 | Store 页面故障 | 故障虚拟地址 |

### CSR 定义

| CSR 名称 | 编号 | 说明 |
|----------|------|------|
| `vstval` | `0x243` | Virtual supervisor trap value，见 `common/encoding.h` |
| `stval` | `0x143` | Supervisor trap value（V=1 时访问实际操作 vstval） |
| `vscause` | `0x242` | Virtual supervisor cause |
| `vsepc` | `0x241` | Virtual supervisor exception PC |
| `vstvec` | `0x205` | Virtual supervisor trap vector base |
| `hedeleg` | `0x602` | Hypervisor exception delegation |

### hedeleg 委托位配置

| 异常类型 | hedeleg bit | 说明 |
|----------|-------------|------|
| Instruction address misaligned | 0 | cause=0 |
| Instruction access fault | 1 | cause=1 |
| Illegal instruction | 2 | cause=2 |
| Breakpoint | 3 | cause=3 |
| Load address misaligned | 4 | cause=4 |
| Load access fault | 5 | cause=5 |
| Store address misaligned | 6 | cause=6 |
| Store access fault | 7 | cause=7 |
| Instruction page fault | 12 | cause=12 |
| Load page fault | 13 | cause=13 |
| Store page fault | 15 | cause=15 |

### 测试框架 API

- `run_in_vs_mode(fn, arg)`：在 VS-mode (V=1) 执行 fn(arg)
- `two_stage_init(ctx, vs_mode, g_mode)`：初始化两阶段翻译上下文
- `two_stage_identity_map_all(ctx)`：VS-stage + G-stage 全域恒等映射
- `two_stage_run_in_vs(ctx, fn, arg)`：以两阶段页表进入 VS-mode 执行
- `hyp_delegate_to_vs(hedeleg_mask, hideleg_mask)`：配置异常/中断委托到 VS-mode
- `hyp_undelegate()`：清除所有 hypervisor 委托
- `HYP_TEST_END()`：测试结束宏（含 hyp_reset_state）

### 全局变量（`shvstvala/tests/shvstvala_strap.S` 提供）

| 变量 | 类型 | 说明 |
|------|------|------|
| `g_shvstvala_vstval` | `volatile uintptr_t` | VS-mode trap handler 读取的 vstval 值 |
| `g_shvstvala_cause` | `volatile uintptr_t` | VS-mode trap handler 读取的 vscause 值 |
| `shvstvala_trap_entry` | `void(void)` | 4 字节对齐的 VS-mode trap entry 符号 |

---

## 测试统计

| 分组 | 测试数量 | 异常类型 | vstval 语义 | 依赖 |
|------|----------|----------|-------------|------|
| Group 1 | 5 | Page-Fault（cause 12/13/15） | 故障虚拟地址 | VS-stage 页表 + hedeleg |
| Group 2 | 3 | Access-Fault（cause 1/5/7） | 故障虚拟地址 | PMP + hedeleg |
| Group 3 | 3 | Misaligned（cause 0/4/6） | 故障虚拟地址 | 平台相关 + hedeleg |
| Group 4 | 5 | Illegal Instruction（cause 2） | 故障指令编码 | hedeleg |
| Group 5 | 4 | N/A（透传验证） | WARL 语义 | H 扩展基础 |
| Group 6 | 2 | Breakpoint（cause 3） | 故障虚拟地址 | Sdtrig + hedeleg |
| **总计** | **22** | | | |

---

## 测试执行说明

### 运行环境

- 所有测试需 H 扩展支持（`ENABLE_HYP=1`）
- Group 1：需 VS-stage 页表（vsatp=Sv39）+ G-stage 恒等映射（hgatp=Sv39x4）
- Group 2：需 PMP 配置 + 两阶段恒等映射
- Group 3：需 hedeleg 委托 + 平台不支持硬件非对齐（部分用例可选）
- Group 4：需 hedeleg bit 2 委托 illegal-instruction
- Group 5：M-mode ↔ VS-mode 交替执行
- 单核环境，无需 IPI

### 失败定位指引

| 现象 | 可能原因 |
|------|----------|
| VSTVAL-LPF-01 失败（vstval=0） | Shvstvala 未启用，vstval 在 page-fault 时被写 0 |
| VSTVAL-LPF-01 失败（trap 未到 VS） | hedeleg 未正确配置 bit 13，trap escalate 到 HS-mode |
| VSTVAL-LAF-01 失败（cause≠5） | PMP 配置错误或 G-stage 翻译把目标 PA 映射到了不同区域 |
| VSTVAL-ILL-01 失败（vstval≠指令编码） | 实现未正确将指令编码写入 vstval |
| VSTVAL-ILL-04 失败（vstval 高位非零） | 16 位指令编码未正确零扩展到 XLEN |
| VSTVAL-TRANS-01 失败 | V=1 时 stval 未正确映射到 vstval |
| VSTVAL-BRK-01 SKIP | 平台未实现 Sdtrig 扩展（tinfo 探测失败），无法配置地址匹配断点 |
| VSTVAL-BRK-01 失败（vstval≠target） | trigger 配置错误（tdata1 type/VS 位设置不正确）或 hedeleg bit 3 未委托 |
| 所有用例均 cause 不匹配 | medeleg 未委托到 HS-mode，或 hedeleg 未进一步委托到 VS-mode |
