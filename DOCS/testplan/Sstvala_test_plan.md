**中文 | [English](../testplan_en/Sstvala_test_plan_en.md)**

# Sstvala 扩展测试计划

本文档描述 Sstvala（Trap Value Reporting, Version 1.0）扩展的测试计划。Sstvala 扩展规定了 `stval` CSR 在不同异常类型下必须写入的值：对于地址类异常（page-fault、access-fault、misaligned、非 EBREAK 的 breakpoint），`stval` 必须写入故障虚拟地址；对于指令类异常（illegal-instruction、virtual-instruction），`stval` 必须写入故障指令编码。

---

## 测试范围

### 规范来源

- `SPEC/sstvala.adoc` — Sstvala Extension for Trap Value Reporting, Version 1.0

### 关键参考文件

| 路径 | 说明 |
|------|------|
| `SPEC/sstvala.adoc` | Sstvala 规范全文（共 10 行） |
| `common/trap.c:28` | `trap_record.tval` 字段，M/S-mode trap handler 中捕获 `mtval`/`stval` |
| `common/trap.c:218,347` | M-mode handler 读取 `mtval`、S-mode handler 读取 `stval`，存入 `trap_record.tval` |
| `common/trap.c:447-448` | `trap_get_tval()` 函数，返回最近一次 trap 的 tval 值 |
| `common/test_framework.h:126-139` | `TEST_ASSERT_EQ` 宏，用于断言值相等 |
| `common/test_framework.h:169-180` | `EXPECT_TRAP` 宏，M-mode 下触发并验证异常 |
| `common/test_framework.h:234-275` | `PRIV_DO_TRAP` / `CHECK_TRAP` 宏，S/U-mode 安全的 trap 宏 |
| `common/test_framework.h:154` | `trap_get_tval()` 声明 |
| `common/encoding.h:179-206` | 各类异常 cause 值定义 |
| `common/vm/vm.h` | 页表管理 API（`pt_init`、`pt_map_page`、`vm_run_in_smode` 等） |
| `common/vm/vm_defs.h` | VM 常量定义（`PTE_V`、`PTE_R`、`PTE_W`、`PTE_X`、`PAGE_SIZE_4K` 等） |

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sstvala_stval_faulting_vaddr` | If the Sstvala extension is implemented, then `stval` must be written with the faulting virtual address for load, store, and instruction page-fault, access-fault, and misaligned exceptions, and for breakpoint exceptions that are defined to write an address to stval, other than those caused by execution of the `EBREAK` or `C.EBREAK` instructions. | 如果实现了 Sstvala 扩展，则对于加载、存储和指令页错误、访问错误及非对齐异常，以及定义为向 stval 写入地址的断点异常（由 `EBREAK` 或 `C.EBREAK` 指令引起的除外），`stval` 必须写入故障虚拟地址。 |
| `norm:sstvala_stval_faulting_instruction` | For virtual-instruction and illegal-instruction exceptions, `stval` must be written with the faulting instruction. | 对于虚拟指令和非法指令异常，`stval` 必须写入故障指令。 |

> [!IMPORTANT]
> Sstvala 规范的核心分为两大类：
> 1. **地址类异常**（page-fault、access-fault、misaligned、breakpoint）→ `stval` = 故障虚拟地址
> 2. **指令类异常**（illegal-instruction、virtual-instruction）→ `stval` = 故障指令编码
>
> 所有测试用例围绕"触发特定异常 → 验证 `stval` 值"展开。

### 不在测试范围内

- **`mtval` 的行为**：Sstvala 仅约束 `stval`（S-mode trap value），不涉及 `mtval`（M-mode）。但由于测试框架在 M-mode 捕获 trap 时使用 `mtval`，而规范中 `mtval` 的行为与 `stval` 对称，因此 M-mode trap 的 `mtval` 验证可作为等效证据
- **EBREAK / C.EBREAK 导致的 breakpoint**：规范明确排除，`stval` 行为由其他规范定义
- **多 hart 场景**：项目为单核测试环境
- **Sv32 / Sv48 / Sv57 模式**：仅覆盖 RV64 + Sv39，与项目其它扩展计划保持一致
- **Guest page-fault（cause 20/21/23）**：涉及 H 扩展的两级翻译，由 Hypervisor 测试计划独立覆盖

---

## 设计要点

### 1. stval 验证的通用模式

所有测试用例遵循统一模式：

```
1. 设置触发条件（页表配置 / PMP 配置 / 特殊指令）
2. arm trap → 执行触发指令 → disarm trap
3. 断言 trap_was_triggered() == true
4. 断言 trap_get_cause() == 预期 cause
5. 断言 trap_get_tval() == 预期 stval 值（地址或指令编码）
```

框架中 `trap_record.tval` 在 M-mode handler（`common/trap.c:264`）和 S-mode handler（`common/trap.c:363`）中都会被设置，对应读取 `mtval` 和 `stval`。`trap_get_tval()` 函数（第 447 行）直接返回该值。

### 2. 地址类异常的 stval 验证

对于 page-fault、access-fault、misaligned 异常，`stval` 应等于触发异常的虚拟地址。验证时需要：

- **已知目标地址**：测试代码显式构造访问目标地址（如 `TEST_REGION_BASE`），然后断言 `trap_get_tval() == target_addr`
- **M-mode vs S-mode**：page-fault 需要 VM 启用（S-mode），access-fault 可在 M-mode（PMP）或 S-mode 触发，misaligned 在任何模式下都可触发

### 3. 指令类异常的 stval 验证

对于 illegal-instruction 异常，`stval` 应等于故障指令的编码值。验证时：

- **已知指令编码**：在内存中预置已知的非法指令字节序列，然后执行或在 trap 后读取 `trap_get_tval()` 与预期编码比较
- **32 位指令**：`stval` 应为完整的 32 位指令编码（零扩展到 XLEN）
- **16 位压缩指令**：`stval` 应为 16 位指令编码（零扩展到 XLEN）

### 4. VM 配置（Page-Fault 场景）

Page-fault 测试需要启用 Sv39 页表：

- **代码/数据区域**：identity mapping，`PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D`，确保测试代码和 trap handler 可正常运行
- **测试区域**：故意缺少映射（unmapped VA）或权限不足（只读页写入），触发 page-fault
- 使用 `vm_run_in_smode()` 进入 S-mode 执行，异常被 S-mode trap handler 捕获

### 5. PMP 配置（Access-Fault 场景）

Access-fault 通过 PMP 限制触发：

- 配置 PMP 条目，使特定地址区域对 S/U-mode 不可读/不可写/不可执行
- 在 S-mode 或 U-mode 下访问该区域，触发 access-fault
- 验证 `stval` == 被拒绝访问的地址

### 6. EBREAK 排除说明

规范明确排除 EBREAK/C.EBREAK 导致的 breakpoint。EBREAK 的 `stval` 行为由基础特权级规范定义（通常为 0 或 PC），不属于 Sstvala 测试范围。Group 4 仅测试由 trigger 模块（如果实现）或其他机制触发的 breakpoint 异常。

> [!NOTE]
> 若平台未实现 trigger 模块（Sdtrig 扩展），Group 4 的用例将被 `TEST_SKIP` 跳过，不影响整体测试结论。

### 7. Virtual Instruction 异常说明

Virtual-instruction 异常（cause=22）需要 H 扩展（Hypervisor）支持，在 VS-mode 下执行某些 HS-level CSR 访问指令时触发。Group 6 的 3 个测试已迁移至 [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) Group 2（ID 为 HCROSS-SSTVALA-06~08），实现在 `Hypervisor_Sstvala/` 项目中。本文件保留 Group 6 的描述供参考。

---

## 测试分组

> [!IMPORTANT]
> 共 6 个测试组、25 个测试用例。Group 1–3 为地址类异常核心测试，Group 4 为 breakpoint（条件性），Group 5 为指令类异常核心测试，Group 6 为可选（需 H 扩展）。

---

### Group 1：Page-Fault 地址类异常（stval = 故障虚拟地址）

**规范依据**：
- `norm:Sstvala_pagefault_tval_addr`（`SPEC/sstvala.adoc:4-6`）：load/store/instruction page-fault 时，`stval` 必须写入故障虚拟地址

**测试职责**：验证在 Sv39 虚拟内存模式下，当 S-mode 访问未映射或权限不足的虚拟地址触发 page-fault 时，`stval`（通过 M-mode `mtval` 等效捕获）等于触发异常的虚拟地址。

**前置条件**：
- 启用 Sv39 页表，代码/栈区域 identity mapping（`PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D`）
- 测试目标虚拟地址未映射或权限受限
- M-mode `medeleg` 委托 page-fault 到 S-mode（或由 M-mode handler 直接捕获）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TVAL-LPF-01 | load page-fault：未映射 VA | 在 Sv39 下，S-mode 对未映射的 VA（如 `0x40000000`）执行 `ld`，触发 load page-fault（cause=13） | `trap_get_cause() == 13`；`trap_get_tval() == 0x40000000` |
| TVAL-LPF-02 | load page-fault：写权限页读取（反向验证） | 4 KiB PTE：V=1, W=1, R=1, X=0, A=1, D=1，S-mode load 应成功 | 无异常（反向验证，确认正确映射不触发 fault） |
| TVAL-SPF-01 | store page-fault：只读页写入 | 4 KiB PTE：V=1, R=1, W=0, A=1, D=1，S-mode 对该 VA 执行 `sd`，触发 store page-fault（cause=15） | `trap_get_cause() == 15`；`trap_get_tval() == test_va` |
| TVAL-SPF-02 | store page-fault：未映射 VA | S-mode 对未映射 VA 执行 `sd`，触发 store page-fault（cause=15） | `trap_get_cause() == 15`；`trap_get_tval() == unmapped_va` |
| TVAL-IPF-01 | instruction page-fault：未映射 VA fetch | S-mode 跳转到未映射的 VA 执行取指，触发 instruction page-fault（cause=12） | `trap_get_cause() == 12`；`trap_get_tval() == target_pc` |
| TVAL-IPF-02 | instruction page-fault：不可执行页 fetch | 4 KiB PTE：V=1, R=1, W=0, X=0, A=1, D=1，S-mode 跳转执行，触发 instruction page-fault（cause=12） | `trap_get_cause() == 12`；`trap_get_tval() == target_pc` |
| TVAL-LPF-03 | load page-fault：非规范地址 | S-mode 对非规范 VA（如 Sv39 下 bit[63:39] 不一致的地址 `0x4000000000`）执行 load | `trap_get_cause() == 13`；`trap_get_tval() == 0x4000000000` |

#### 关键代码示例：TVAL-LPF-01

```c
/* tests/test_pagefault.c — TVAL-LPF-01 */

#include "test_framework.h"
#include "vm/vm.h"

#define UNMAPPED_VA  0x40000000UL

static uintptr_t smode_load_unmapped(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    (void)*ptr;  /* 触发 load page-fault */
    return 0;
}

TEST_REGISTER(test_sstvala_load_pagefault_tval);
bool test_sstvala_load_pagefault_tval(void) {
    TEST_BEGIN("TVAL-LPF-01: load page-fault stval == faulting VA");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* identity mapping for code/stack */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D,
                              PT_LEVEL_1G);
    /* UNMAPPED_VA 不映射 */

    uintptr_t scause = vm_run_in_smode(&ctx, smode_load_unmapped,
                                        UNMAPPED_VA);

    TEST_ASSERT_EQ("scause == load page-fault",
                   scause, CAUSE_LOAD_PAGE_FAULT);
    TEST_ASSERT_EQ("stval == faulting VA",
                   trap_get_tval(), UNMAPPED_VA);

    pt_pool_reset();
    TEST_END();
}
```

#### 关键代码示例：TVAL-SPF-01

```c
/* tests/test_pagefault.c — TVAL-SPF-01 */

#define TEST_VA_READONLY  0x80400000UL

static uintptr_t smode_store_readonly(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    *ptr = 0xDEAD;  /* 触发 store page-fault */
    return 0;
}

TEST_REGISTER(test_sstvala_store_pagefault_tval);
bool test_sstvala_store_pagefault_tval(void) {
    TEST_BEGIN("TVAL-SPF-01: store page-fault on R-only page, stval == VA");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D,
                              PT_LEVEL_1G);

    /* 映射只读页：V=1, R=1, W=0 */
    pt_map_page(&ctx, TEST_VA_READONLY, TEST_VA_READONLY,
                PTE_V|PTE_R|PTE_A|PTE_D, PT_LEVEL_4K);

    uintptr_t scause = vm_run_in_smode(&ctx, smode_store_readonly,
                                        TEST_VA_READONLY);

    TEST_ASSERT_EQ("scause == store page-fault",
                   scause, CAUSE_STORE_PAGE_FAULT);
    TEST_ASSERT_EQ("stval == faulting VA",
                   trap_get_tval(), TEST_VA_READONLY);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 2：Access-Fault 地址类异常（stval = 故障虚拟地址）

**规范依据**：
- `norm:Sstvala_accessfault_tval_addr`（`SPEC/sstvala.adoc:4-6`）：load/store/instruction access-fault 时，`stval` 必须写入故障虚拟地址

**测试职责**：验证当 PMP 限制导致 access-fault 时，`stval` 等于被拒绝访问的虚拟地址。Access-fault 通过 PMP 配置在 M-mode 下限制 S/U-mode 对特定区域的访问权限来触发。

**前置条件**：
- PMP 配置：至少一个条目限制特定地址区域对 S-mode 不可读/不可写/不可执行
- `medeleg` 委托 access-fault 到 S-mode（或由 M-mode handler 直接捕获）
- 可选择启用或不启用 VM（access-fault 在物理地址层面检查）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TVAL-LAF-01 | load access-fault：PMP 禁读区域 | 配置 PMP 使特定区域对 S-mode 不可读，S-mode 对该区域执行 `ld` | `trap_get_cause() == 5`（LAF）；`trap_get_tval() == target_addr` |
| TVAL-LAF-02 | load access-fault：不同地址验证 | 同 LAF-01 但使用不同的目标地址，确认 stval 跟随实际访问地址变化 | `trap_get_cause() == 5`；`trap_get_tval() == different_addr` |
| TVAL-SAF-01 | store access-fault：PMP 禁写区域 | 配置 PMP 使特定区域对 S-mode 不可写，S-mode 对该区域执行 `sd` | `trap_get_cause() == 7`（SAF）；`trap_get_tval() == target_addr` |
| TVAL-IAF-01 | instruction access-fault：PMP 禁执行区域 | 配置 PMP 使特定区域对 S-mode 不可执行，S-mode 跳转到该区域取指 | `trap_get_cause() == 1`（IAF）；`trap_get_tval() == target_pc` |

#### 关键代码示例：TVAL-LAF-01

```c
/* tests/test_accessfault.c — TVAL-LAF-01 */

#include "test_framework.h"

#define PMP_NOREAD_ADDR  0x80600000UL

TEST_REGISTER(test_sstvala_load_accessfault_tval);
bool test_sstvala_load_accessfault_tval(void) {
    TEST_BEGIN("TVAL-LAF-01: load access-fault stval == faulting addr");

    /* 配置 PMP：
     * entry 0: PMP_NOREAD_ADDR 区域 NAPOT 4KiB，权限 W 只写（禁读）
     * entry 1: 全地址空间 NAPOT，权限 RWX（S-mode 基线）
     * PMP 优先级：entry 0 先匹配，覆盖该区域的 R 权限 */
    pmp_clear_all();
    pmp_entry_t e0 = PMP_ENTRY_NAPOT(PMP_NOREAD_ADDR, 0x1000, PMP_W);
    pmp_set_entry(0, &e0);
    pmp_entry_t e1 = PMP_ENTRY_NAPOT(0x0, 0x100000000UL, PMP_RWX);
    pmp_set_entry(1, &e1);

    goto_priv(PRIV_S);
    PRIV_DO_TRAP(mem_load8(PMP_NOREAD_ADDR));
    goto_priv(PRIV_M);

    CHECK_TRAP("load access-fault triggered", CAUSE_LOAD_ACCESS_FAULT);
    TEST_ASSERT_EQ("stval == faulting addr",
                   trap_get_tval(), PMP_NOREAD_ADDR);

    reset_state();
    TEST_END();
}
```

---

### Group 3：Misaligned 地址类异常（stval = 故障虚拟地址）

**规范依据**：
- `norm:Sstvala_misaligned_tval_addr`（`SPEC/sstvala.adoc:4-6`）：load/store/instruction misaligned 异常时，`stval` 必须写入故障虚拟地址

**测试职责**：验证当 load/store 执行非自然对齐的内存访问触发 misaligned 异常时，`stval` 等于未对齐的访问地址。

**前置条件**：
- 平台必须不支持硬件非对齐访问（即非对齐访问会触发异常而非硬件透明处理）。若平台支持硬件非对齐访问，相关用例应被 `TEST_SKIP` 跳过
- Instruction misaligned（cause=0）仅在跳转到非 2 字节对齐地址时触发（若支持 C 扩展则需非 2 字节对齐，否则需非 4 字节对齐）

> [!WARNING]
> 大多数现代 RISC-V 实现（包括 Spike、QEMU）在默认配置下**支持硬件非对齐 load/store 访问**，不会触发 misaligned 异常。因此 TVAL-LMA-01 和 TVAL-SMA-01 在这些平台上会被跳过。TVAL-IMA-01（instruction misaligned）通常可以在任何平台上测试，因为非 2 字节对齐的 PC 总是触发异常。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TVAL-LMA-01 | load misaligned：非对齐 load | M-mode 对非 8 字节对齐地址执行 `ld`（如 addr+3），若平台不支持非对齐访问则触发 cause=4 | `trap_get_cause() == 4`；`trap_get_tval() == (addr+3)`；若平台支持非对齐访问则 `TEST_SKIP` |
| TVAL-SMA-01 | store misaligned：非对齐 store | M-mode 对非 8 字节对齐地址执行 `sd`（如 addr+5），若平台不支持非对齐访问则触发 cause=6 | `trap_get_cause() == 6`；`trap_get_tval() == (addr+5)`；若平台支持非对齐访问则 `TEST_SKIP` |
| TVAL-IMA-01 | instruction misaligned：跳转到奇数地址 | 使用 `jalr` 跳转到奇数地址（如 `target \| 1`），触发 instruction address misaligned（cause=0） | `trap_get_cause() == 0`；`trap_get_tval() == (target \| 1)` |

#### 关键代码示例：TVAL-IMA-01

```c
/* tests/test_misaligned.c — TVAL-IMA-01 */

#include "test_framework.h"

TEST_REGISTER(test_sstvala_inst_misaligned_tval);
bool test_sstvala_inst_misaligned_tval(void) {
    TEST_BEGIN("TVAL-IMA-01: instruction misaligned stval == faulting addr");

    /* 构造一个奇数目标地址（非 2 字节对齐） */
    extern void _start(void);
    uintptr_t target = ((uintptr_t)&_start) | 0x1UL;

    trap_expect_begin();
    /* 使用 jalr 跳转到非对齐地址 */
    asm volatile (
        "jalr zero, %0, 0"
        :
        : "r"(target)
        : "memory"
    );
    trap_expect_end();

    TEST_ASSERT("trap triggered", trap_was_triggered());
    TEST_ASSERT_EQ("cause == instruction address misaligned",
                   trap_get_cause(), CAUSE_INST_ADDR_MISALIGN);
    TEST_ASSERT_EQ("stval == misaligned target address",
                   trap_get_tval(), target);

    TEST_END();
}
```

#### 关键代码示例：TVAL-LMA-01（带平台探测）

```c
/* tests/test_misaligned.c — TVAL-LMA-01 */

TEST_REGISTER(test_sstvala_load_misaligned_tval);
bool test_sstvala_load_misaligned_tval(void) {
    TEST_BEGIN("TVAL-LMA-01: load misaligned stval == faulting addr");

    /* 探测平台是否支持硬件非对齐访问 */
    static volatile uint64_t test_buf[2] = {0x1122334455667788ULL, 0};
    uintptr_t misaligned_addr = ((uintptr_t)test_buf) + 3;

    trap_expect_begin();
    volatile uint64_t val = *(volatile uint64_t *)misaligned_addr;
    (void)val;

    if (!trap_was_triggered()) {
        trap_expect_end();
        TEST_SKIP("platform supports hardware misaligned load");
    }

    TEST_ASSERT_EQ("cause == load address misaligned",
                   trap_get_cause(), CAUSE_LOAD_ADDR_MISALIGN);
    TEST_ASSERT_EQ("stval == misaligned address",
                   trap_get_tval(), misaligned_addr);
    trap_expect_end();

    TEST_END();
}
```

---

### Group 4：Breakpoint 异常（stval = 故障地址，非 EBREAK）

**规范依据**：
- `norm:Sstvala_breakpoint_tval_addr`（`SPEC/sstvala.adoc:6-8`）：对于 breakpoint 异常（cause=3），若非由 EBREAK/C.EBREAK 指令触发，且规范定义应写地址到 stval，则 `stval` 必须写入故障地址

**测试职责**：验证由硬件 trigger（Sdtrig 扩展）触发的 breakpoint 异常中，`stval` 写入断点命中的地址。

**前置条件**：
- 平台必须实现 Sdtrig 扩展（Debug Trigger 模块），提供 `tselect`、`tdata1`、`tdata2` 等 CSR
- 若平台未实现 trigger 模块，所有用例 `TEST_SKIP`

> [!NOTE]
> Sdtrig 扩展不在所有平台上可用。Spike 默认支持 trigger，QEMU 部分支持。若平台不支持，本组所有用例自动跳过，不影响整体测试结论。EBREAK/C.EBREAK 触发的 breakpoint 被规范明确排除，不在本组测试范围内。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TVAL-BKP-01 | trigger breakpoint：地址匹配 load | 配置 trigger 在特定地址触发 load breakpoint（type=6 mcontrol6，load=1），M-mode 执行 load 到该地址 | `trap_get_cause() == 3`；`trap_get_tval() == trigger_addr` |
| TVAL-BKP-02 | trigger breakpoint：地址匹配 instruction | 配置 trigger 在特定 PC 触发 execute breakpoint（execute=1），执行到该 PC | `trap_get_cause() == 3`；`trap_get_tval() == trigger_pc` |
| TVAL-BKP-03 | EBREAK 排除验证（反向） | 执行 EBREAK 指令，验证 breakpoint 被触发但**不断言** stval 为特定值（Sstvala 不约束 EBREAK 的 stval） | `trap_get_cause() == 3`；stval 值不做断言（仅记录） |

#### 关键代码示例：TVAL-BKP-01

```c
/* tests/test_breakpoint.c — TVAL-BKP-01 */

#include "test_framework.h"

#define CSR_TSELECT  0x7A0
#define CSR_TDATA1   0x7A1
#define CSR_TDATA2   0x7A2

/* mcontrol6 type=6 字段布局（RV64, Sdtrig 1.0）:
 *   [63:60] type    = 6 (mcontrol6)
 *   [59]    dmode
 *   [10:7]  match   (0 = exact address match)
 *   [6]     M       (enable in M-mode)
 *   [4]     S       (enable in S-mode)
 *   [3]     U       (enable in U-mode)
 *   [2]     execute (match on instruction fetch)
 *   [1]     store   (match on store)
 *   [0]     load    (match on load)
 */
#define MCONTROL6_TYPE     (6UL << 60)  /* type = 6 */
#define MCONTROL6_LOAD     (1UL << 0)   /* match on load */
#define MCONTROL6_STORE    (1UL << 1)   /* match on store */
#define MCONTROL6_EXECUTE  (1UL << 2)   /* match on execute */
#define MCONTROL6_M        (1UL << 6)   /* enable in M-mode */
#define MCONTROL6_S        (1UL << 4)   /* enable in S-mode */
#define MCONTROL6_MATCH_EQ (0UL << 7)   /* exact address match */

TEST_REGISTER(test_sstvala_trigger_breakpoint_tval);
bool test_sstvala_trigger_breakpoint_tval(void) {
    TEST_BEGIN("TVAL-BKP-01: trigger breakpoint stval == trigger addr");

    /* 探测 trigger 模块是否存在 */
    trap_expect_begin();
    CSRW(CSR_TSELECT, 0);
    if (trap_was_triggered()) {
        trap_expect_end();
        TEST_SKIP("platform does not implement trigger module");
    }
    trap_expect_end();

    static volatile uint64_t trigger_target = 0xCAFEBABE;
    uintptr_t trigger_addr = (uintptr_t)&trigger_target;

    /* 配置 trigger：在 trigger_addr 上 load 时触发 breakpoint */
    CSRW(CSR_TSELECT, 0);
    CSRW(CSR_TDATA2, trigger_addr);
    CSRW(CSR_TDATA1, MCONTROL6_TYPE | MCONTROL6_LOAD |
                      MCONTROL6_M | MCONTROL6_MATCH_EQ);

    trap_expect_begin();
    volatile uint64_t val = trigger_target;  /* 触发 breakpoint */
    (void)val;

    TEST_ASSERT("breakpoint triggered", trap_was_triggered());
    TEST_ASSERT_EQ("cause == breakpoint",
                   trap_get_cause(), CAUSE_BREAKPOINT);
    TEST_ASSERT_EQ("stval == trigger address",
                   trap_get_tval(), trigger_addr);
    trap_expect_end();

    /* 清理 trigger */
    CSRW(CSR_TDATA1, 0);

    TEST_END();
}
```

---

### Group 5：Illegal Instruction 指令类异常（stval = 故障指令编码）

**规范依据**：
- `norm:Sstvala_illegal_inst_tval_inst`（`SPEC/sstvala.adoc:9-10`）：illegal-instruction 异常时，`stval` 必须写入故障指令编码

**测试职责**：验证当执行非法指令触发 illegal-instruction 异常（cause=2）时，`stval` 等于触发异常的指令编码值（32 位或 16 位指令，零扩展到 XLEN）。

**前置条件**：
- 在内存中预置已知编码的非法指令
- 使用 `exec_at()` 跳转到该地址执行，或直接用 inline asm 嵌入非法指令

> [!IMPORTANT]
> 指令编码验证是 Sstvala 测试的另一核心部分。对于 32 位指令，`stval` 应为完整的 32 位编码（零扩展到 XLEN 宽度）；对于 16 位压缩非法指令，`stval` 应为 16 位编码（零扩展到 XLEN 宽度）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TVAL-ILL-01 | 32 位非法指令：custom-0 操作码 | 在内存预置 `0x0000000B`（bits[1:0]=11 表示 32 位指令，opcode[6:0]=0001011 即 custom-0，通常未实现），跳转执行 | `trap_get_cause() == 2`；`trap_get_tval() == 0x0000000B` |
| TVAL-ILL-02 | 32 位非法指令：写只读 CSR | 在内存预置 `0xC0001073`（`csrrw x0, 0xC00, x0`，写入只读 CSR `cycle`），跳转执行 | `trap_get_cause() == 2`；`trap_get_tval() == 0xC0001073` |
| TVAL-ILL-03 | 32 位非法指令：访问不存在的 CSR | 在内存预置 `0xFFF022F3`（`csrrs x5, 0xFFF, x0`，访问不存在的 CSR 0xFFF），跳转执行 | `trap_get_cause() == 2`；`trap_get_tval() == 0xFFF022F3` |
| TVAL-ILL-04 | 16 位压缩非法指令 | 在内存预置 16 位全零编码 `0x0000`（C 扩展中全零为非法压缩指令，bits[1:0]=00 表示 16 位指令），跳转执行 | `trap_get_cause() == 2`；`trap_get_tval() == 0x0000`（零扩展到 XLEN） |
| TVAL-ILL-05 | 连续两次非法指令 stval 不同 | 依次执行 `0x0000000B`（custom-0）和 `0xC0001073`（写只读 CSR）两条不同非法指令，验证 stval 每次对应实际指令 | 第 1 次 `trap_get_tval() == 0x0000000B`；第 2 次 `trap_get_tval() == 0xC0001073` |

#### 关键代码示例：TVAL-ILL-01（32 位 custom-0 非法指令）

```c
/* tests/test_illegal.c — TVAL-ILL-01 */

#include "test_framework.h"

/*
 * 0x0000000B: bits[1:0]=11 表示 32 位指令长度
 * opcode[6:0] = 0001011 = custom-0 操作码（通常未实现，触发 illegal instruction）
 *
 * 注意：0x00000000 的 bits[1:0]=00，属于 16 位压缩指令格式，
 * 不能用作 32 位非法指令测试。
 */
static uint32_t illegal_custom0 __attribute__((aligned(4))) = 0x0000000B;

TEST_REGISTER(test_sstvala_illegal_custom0_tval);
bool test_sstvala_illegal_custom0_tval(void) {
    TEST_BEGIN("TVAL-ILL-01: illegal custom-0 (0x0000000B) stval == encoding");

    uintptr_t addr = (uintptr_t)&illegal_custom0;

    EXPECT_EXEC_TRAP(CAUSE_ILLEGAL_INST, addr);
    TEST_ASSERT_EQ("stval == 0x0000000B (32-bit instruction encoding)",
                   trap_get_tval(), 0x0000000BUL);

    TEST_END();
}
```

#### 关键代码示例：TVAL-ILL-03（访问不存在的 CSR）

```c
/* tests/test_illegal.c — TVAL-ILL-03 */

/*
 * csrrs x5, 0xFFF, x0 — 访问不存在的 CSR 0xFFF
 *
 * 编码推导：
 *   [31:20] = 0xFFF      (CSR address = 0xFFF, 不存在)
 *   [19:15] = 00000      (rs1 = x0)
 *   [14:12] = 010        (funct3 = CSRRS)
 *   [11:7]  = 00101      (rd = x5)
 *   [6:0]   = 1110011    (SYSTEM opcode)
 *
 * 二进制: 1111_1111_1111 00000 010 00101 1110011
 * 十六进制: 0xFFF022F3
 */
static uint32_t illegal_csr_inst __attribute__((aligned(4))) = 0xFFF022F3;

TEST_REGISTER(test_sstvala_illegal_csr_tval);
bool test_sstvala_illegal_csr_tval(void) {
    TEST_BEGIN("TVAL-ILL-03: illegal CSR access stval == instruction encoding");

    uintptr_t addr = (uintptr_t)&illegal_csr_inst;

    EXPECT_EXEC_TRAP(CAUSE_ILLEGAL_INST, addr);
    TEST_ASSERT_EQ("stval == 0xFFF022F3 (csrrs x5, 0xFFF, x0)",
                   trap_get_tval(), 0xFFF022F3UL);

    TEST_END();
}
```

#### 关键代码示例：TVAL-ILL-04（16 位压缩非法指令）

```c
/* tests/test_illegal.c — TVAL-ILL-04 */

/*
 * 0x0000 是 C 扩展中的非法指令（全零 16 位编码）
 * 在 RV64IC 下执行将触发 illegal-instruction
 * stval 应为 0x0000（零扩展到 64 位）
 *
 * 注意：需要在后面紧跟一条合法指令（如 c.nop = 0x0001），
 * 避免连续非法指令导致 trap handler 推进 PC 后再次触发
 */
static uint16_t illegal_compressed[2] __attribute__((aligned(2))) = {
    0x0000,  /* 非法 16 位指令 */
    0x0001,  /* c.nop（合法，用于 trap 后推进 PC 的落脚点） */
};

TEST_REGISTER(test_sstvala_illegal_compressed_tval);
bool test_sstvala_illegal_compressed_tval(void) {
    TEST_BEGIN("TVAL-ILL-04: compressed illegal (0x0000) stval == 0x0000");

    uintptr_t addr = (uintptr_t)&illegal_compressed[0];

    EXPECT_EXEC_TRAP(CAUSE_ILLEGAL_INST, addr);
    TEST_ASSERT_EQ("stval == 0x0000 (16-bit encoding, zero-extended)",
                   trap_get_tval(), 0x0000UL);

    TEST_END();
}
```

---

### Group 6：Virtual Instruction 指令类异常（stval = 故障指令编码，可选）

> **[迁移]** 本组 3 个测试已迁移至 [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) **Group 2 (Hypervisor × Sstvala 交叉测试)**，ID 映射：TVAL-VI-01~03 → HCROSS-SSTVALA-06~08。本文件保留原始描述供参考，实际实现和运行以交叉测试计划为准（位于 `Hypervisor_Sstvala/` 项目）。

**规范依据**：
- `norm:Sstvala_virtual_inst_tval_inst`（`SPEC/sstvala.adoc:9-10`）：virtual-instruction 异常时，`stval` 必须写入故障指令编码

**测试职责**：验证当 VS-mode 执行需要 HS 权限的 CSR 指令触发 virtual-instruction 异常（cause=22）时，`stval` 等于触发异常的指令编码。

**前置条件**：
- 平台必须实现 H 扩展（`ENABLE_HYP=1`）
- 测试运行在 VS-mode 下，执行 HS-level CSR 访问（如 `hgatp`、`hstatus`）

> [!WARNING]
> 本组为**可选测试组**，仅在 `ENABLE_HYP=1` 时编译和执行。若未启用 H 扩展，整组 `TEST_SKIP`。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TVAL-VI-01 | virtual-instruction：VS-mode 读 hstatus | VS-mode 执行 `csrrs x5, hstatus, x0`（CSR 0x600），触发 virtual-instruction（cause=22） | `trap_get_cause() == 22`；`trap_get_tval() == 0x600022F3` |
| TVAL-VI-02 | virtual-instruction：VS-mode 写 hgatp | VS-mode 执行 `csrrw x0, hgatp, x0`（CSR 0x680），触发 virtual-instruction（cause=22） | `trap_get_cause() == 22`；`trap_get_tval() == 0x68001073` |
| TVAL-VI-03 | virtual-instruction：VS-mode 读 hideleg | VS-mode 执行 `csrrs x5, hideleg, x0`（CSR 0x603），触发 virtual-instruction（cause=22） | `trap_get_cause() == 22`；`trap_get_tval() == 0x603022F3` |

> [!NOTE]
> **TVAL-VI-03 CSR 选择说明**：原始设计曾考虑使用 `vsstatus`（CSR 0x200），但 VS-mode 下 `vsstatus` 实际是 `sstatus` 的透明别名，VS-mode 可正常访问，不会触发 virtual-instruction 异常。因此改用 `hideleg`（CSR 0x603），这是 HS-level CSR，VS-mode 访问时必定触发 virtual-instruction。

> [!NOTE]
> **指令编码推导**：
> - `csrrs x5, 0x600, x0`：`[31:20]=0x600 [19:15]=00000 [14:12]=010 [11:7]=00101 [6:0]=1110011` = `0x600022F3`
> - `csrrw x0, 0x680, x0`：`[31:20]=0x680 [19:15]=00000 [14:12]=001 [11:7]=00000 [6:0]=1110011` = `0x68001073`
> - `csrrs x5, 0x603, x0`：`[31:20]=0x603 [19:15]=00000 [14:12]=010 [11:7]=00101 [6:0]=1110011` = `0x603022F3`

#### 关键代码示例：TVAL-VI-01

```c
/* tests/test_virtual_inst.c — TVAL-VI-01 */

#ifdef ENABLE_HYP
#include "test_framework.h"
#include "hyp/hyp_priv.h"

/*
 * csrrs x5, 0x600, x0 → 编码：0x600022F3
 *   [31:20] = 0x600 (hstatus)
 *   [19:15] = 00000 (rs1 = x0)
 *   [14:12] = 010   (funct3 = CSRRS)
 *   [11:7]  = 00101 (rd = x5)
 *   [6:0]   = 1110011 (SYSTEM)
 */
static uint32_t vsmode_read_hstatus __attribute__((aligned(4))) = 0x600022F3;

/* VS-mode 辅助函数：跳转到预置的 csrr hstatus 指令执行 */
static uintptr_t vsmode_exec_read_hstatus(uintptr_t arg) {
    (void)arg;
    void (*fn)(void) = (void (*)(void))&vsmode_read_hstatus;
    fn();  /* 触发 virtual-instruction，trap handler 跳过后返回 */
    return trap_get_cause();
}

TEST_REGISTER(test_sstvala_virtual_inst_hstatus_tval);
bool test_sstvala_virtual_inst_hstatus_tval(void) {
    TEST_BEGIN("TVAL-VI-01: virtual-instruction (read hstatus) stval == encoding");

    /* 使用 run_in_vs_mode() 进入 VS-mode 执行 */
    trap_expect_begin();
    uintptr_t cause = run_in_vs_mode(vsmode_exec_read_hstatus, 0);
    (void)cause;

    TEST_ASSERT("trap triggered", trap_was_triggered());
    TEST_ASSERT_EQ("cause == virtual-instruction",
                   trap_get_cause(), CAUSE_VIRTUAL_INSTRUCTION);
    TEST_ASSERT_EQ("stval == instruction encoding (csrrs x5, hstatus, x0)",
                   trap_get_tval(), 0x600022F3UL);
    trap_expect_end();

    TEST_END();
}
#endif /* ENABLE_HYP */
```

---

## 附录：相关常量与 API 参考

### 异常 Cause 值

| 名称 | 值 | 说明 | stval 含义（Sstvala） |
|------|-----|------|------|
| `CAUSE_INST_ADDR_MISALIGN` | 0 | 指令地址未对齐 | 故障虚拟地址 |
| `CAUSE_INST_ACCESS_FAULT` | 1 | 指令访问故障（PMP/PMA） | 故障虚拟地址 |
| `CAUSE_ILLEGAL_INST` | 2 | 非法指令 | 故障指令编码 |
| `CAUSE_BREAKPOINT` | 3 | 断点（非 EBREAK 时） | 故障地址 |
| `CAUSE_LOAD_ADDR_MISALIGN` | 4 | Load 地址未对齐 | 故障虚拟地址 |
| `CAUSE_LOAD_ACCESS_FAULT` | 5 | Load 访问故障（PMP/PMA） | 故障虚拟地址 |
| `CAUSE_STORE_ADDR_MISALIGN` | 6 | Store 地址未对齐 | 故障虚拟地址 |
| `CAUSE_STORE_ACCESS_FAULT` | 7 | Store 访问故障（PMP/PMA） | 故障虚拟地址 |
| `CAUSE_INST_PAGE_FAULT` | 12 | 指令页面故障 | 故障虚拟地址 |
| `CAUSE_LOAD_PAGE_FAULT` | 13 | Load 页面故障 | 故障虚拟地址 |
| `CAUSE_STORE_PAGE_FAULT` | 15 | Store 页面故障 | 故障虚拟地址 |
| `CAUSE_VIRTUAL_INSTRUCTION` | 22 | 虚拟指令异常（H 扩展） | 故障指令编码 |

### 框架 API 参考

| 函数/宏 | 路径 | 说明 |
|---------|------|------|
| `trap_get_tval()` | `common/trap.c:447` | 返回最近一次 trap 的 `mtval`/`stval` 值 |
| `trap_get_cause()` | `common/trap.c:443` | 返回最近一次 trap 的 cause 值 |
| `trap_expect_begin()` | `common/trap.c:413` | 开始 trap 预期（arm） |
| `trap_expect_end()` | `common/trap.c:423` | 结束 trap 预期（disarm） |
| `trap_was_triggered()` | `common/trap.c:427` | 返回上次 arm 期间是否触发了 trap |
| `EXPECT_TRAP(cause, stmt)` | `common/test_framework.h:173` | M-mode：执行 stmt 并断言触发指定 cause |
| `EXPECT_EXEC_TRAP(cause, addr)` | `common/test_framework.h:188` | M-mode：跳转到 addr 并断言触发指定 cause |
| `PRIV_DO_TRAP(stmt)` | `common/test_framework.h:234` | S/U-mode：arm + 执行 + disarm（无 printf） |
| `CHECK_TRAP(msg, cause)` | `common/test_framework.h:272` | M-mode：断言上次 PRIV_DO_TRAP 触发了指定 cause |
| `TEST_ASSERT_EQ(msg, actual, expected)` | `common/test_framework.h:128` | 断言两个值相等 |
| `vm_run_in_smode(ctx, fn, arg)` | `common/vm/satp.c` | 启用 VM 后在 S-mode 执行函数，返回 scause |
| `pt_init(ctx, mode)` | `common/vm/page_table.c` | 初始化页表上下文 |
| `pt_map_page(ctx, va, pa, flags, level)` | `common/vm/page_table.c` | 映射单个页面 |
| `exec_at(addr)` | `common/test_framework.h` | 跳转到指定地址执行（带 trap 恢复） |

### CSR 定义

| CSR 名称 | 编号 | 说明 |
|----------|------|------|
| `stval` | `0x143` | Supervisor trap value |
| `mtval` | `0x343` | Machine trap value |
| `scause` | `0x142` | Supervisor cause register |
| `mcause` | `0x342` | Machine cause register |
| `stvec` | `0x105` | Supervisor trap vector base |
| `tselect` | `0x7A0` | Debug trigger select |
| `tdata1` | `0x7A1` | Debug trigger data 1 |
| `tdata2` | `0x7A2` | Debug trigger data 2 |

### 测试用例总览

| Group | 测试数量 | 异常类型 | stval 语义 | 依赖 |
|-------|---------|----------|------------|------|
| Group 1 | 7 | Page-Fault（cause 12/13/15） | 故障虚拟地址 | VM（Sv39） |
| Group 2 | 4 | Access-Fault（cause 1/5/7） | 故障虚拟地址 | PMP |
| Group 3 | 3 | Misaligned（cause 0/4/6） | 故障虚拟地址 | 平台相关 |
| Group 4 | 3 | Breakpoint（cause 3） | 故障地址 | Sdtrig（可选） |
| Group 5 | 5 | Illegal Instruction（cause 2） | 故障指令编码 | 无 |
| Group 6 | 3 | Virtual Instruction（cause 22） | 故障指令编码 | H 扩展（可选，**已迁移**） |
| **总计** | **25** | | | |
