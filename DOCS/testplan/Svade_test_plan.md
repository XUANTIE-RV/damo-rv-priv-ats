**中文 | [English](../testplan_en/Svade_test_plan_en.md)**

# Svade 扩展测试计划

本文档描述 Svade（Page-Fault Exceptions on A/D Bit Updates）扩展的测试计划。Svade 扩展规定：当虚拟页被访问且 PTE.A=0，或被写入且 PTE.D=0 时，硬件**不再原子地更新** A/D bit，而是抛出 page-fault 异常，由软件负责设置 A/D bit 后重试。

---

## 概述

RISC-V 特权级规范中定义了两种 PTE A/D 位管理方案：

1. **非 Svade 方案**（默认硬件更新）：当 A=0 被访问或 D=0 被写入时，硬件原子地更新 PTE 的 A/D 位。
2. **Svade 方案**：当 A=0 被访问或 D=0 被写入时，硬件抛出 page-fault 异常，PTE 的 A/D 位保持不变，由软件 trap handler 显式置位后重试访问。

实现 Svade 扩展的处理器，在虚拟地址翻译过程的步骤 9（参见 `SPEC/supervisor.adoc` 翻译算法）会检查 `pte.a` 与 `pte.d`：

- 若 `pte.a=0`，或原始访问为 store 且 `pte.d=0`，则**停止翻译并抛出对应原始访问类型的 page-fault 异常**。

本测试计划聚焦该规范要求在不同访问类型（load / store / instruction fetch / AMO）、不同页面粒度（4 KiB / 2 MiB / 1 GiB）下的覆盖。

---

## 测试范围

### 规范来源

- `SPEC/supervisor.adoc`，第 1622–1731 行（Svade 扩展定义、A/D 位语义说明）
- `SPEC/supervisor.adoc`，第 1758 行（虚拟地址翻译算法步骤 9，Svade 触发 page-fault 的位置）

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:svade_access_ad_bit_clear` | The Svade extension: when a virtual page is accessed and the A bit is clear, or is written and the D bit is clear, a page-fault exception is raised. | Svade 扩展：当虚拟页面被访问且 A 位为 0，或被写入且 D 位为 0 时，触发页错误异常。 |
| `Svade_store_d_bit_clear_pagefault` | — | 当虚拟页被写入且 PTE.D=0 时，抛出 store/AMO page-fault |
| `Svade_no_hw_update_ad` | — | Svade 实现下硬件不得自动设置 A/D bit，PTE 内容保持不变 |
| `Svade_pagefault_cause_match_access_type` | — | page-fault 的 scause 必须与原始访问类型一致（load=13、store=15、fetch=12） |
| `Svade_applies_all_levels` | — | A/D 检查在叶 PTE 上执行，4 KiB / 2 MiB megapage / 1 GiB gigapage 三种粒度均适用 |
| `Svade_software_set_ad_then_access` | — | 软件在 PTE 中预置 A=1（store 场景下还需 D=1）后访问应成功 |
| `Svade_non_leaf_ad_reserved` | — | 非叶 PTE 的 A/D 位保留，不参与 Svade 检查（叶 PTE A/D 决定行为） |

### 不在测试范围内

- **Hypervisor 两级翻译场景**：VS-stage 与 G-stage 下的 Svade 行为（涉及 H 扩展，本测试平台不包含）。
- **Svadu 交互**：`menvcfg.ADUE` / `henvcfg.ADUE` 控制硬件 A/D 更新与 Svade 切换的行为，由独立的 svadu 测试计划覆盖。
- **多 hart 一致性**：规范要求所有 hart 必须采用相同的 PTE 更新方案，当前测试框架为单核环境。
- **Sv32 / Sv48 / Sv57 模式**：本计划仅覆盖 Sv39，与现有 svnapot/svpbmt 测试计划保持一致。

---

## 测试分组

### Group 1：A bit 清零触发 load / fetch page-fault（4 KiB）

**规范依据**：
- `norm:Svade_access_a_bit_clear_pagefault`：A=0 时被访问触发 page-fault
- `norm:Svade_software_set_ad_then_access`：A=1 时访问应成功
- `norm:Svade_pagefault_cause_match_access_type`：load 触发 scause=13，fetch 触发 scause=12

**测试职责**：验证在 4 KiB 叶 PTE 上，A=0 时各类读取访问（load、instruction fetch）均会触发对应类型的 page-fault；A=1 时访问应成功。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADE-A-01 | A=0 R-only 页 load | 4 KiB PTE：V=1, R=1, A=0, D=0，S-mode load | load page-fault（scause=13） |
| SVADE-A-02 | A=0 但 D=1 load | 4 KiB PTE：V=1, R=1, A=0, D=1，S-mode load | load page-fault（D=1 不影响 A 检查） |
| SVADE-A-03 | A=1 load 成功 | 4 KiB PTE：V=1, R=1, A=1, D=0，S-mode load | 读取成功，无异常 |
| SVADE-A-04 | A=0 X-only 页 fetch | 4 KiB PTE：V=1, R=0, X=1, A=0，S-mode 跳转执行 | instruction page-fault（scause=12） |
| SVADE-A-05 | A=1 X 页 fetch 成功 | 4 KiB PTE：V=1, R=0, X=1, A=1，S-mode 跳转执行 | 取指执行成功 |
| SVADE-A-06 | A=0 RW 页 load | 4 KiB PTE：V=1, R=1, W=1, A=0, D=0，S-mode load | load page-fault（scause=13） |

```c
/* SVADE-A-01 示例：A=0 R-only 页 load 触发 page-fault */

/* ---- S-mode 辅助函数 ---- */

/* 在 S-mode 下执行 load，预期触发 fault，
 * 由 S-mode trap handler 捕获并返回 scause */
static uintptr_t smode_load_expect_fault(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    (void)*ptr;
    return 0;  /* 若未触发 fault 则返回 0 */
}

/* ---- 测试用例 ---- */

TEST_REGISTER(test_svade_a_clear_load_fault);
bool test_svade_a_clear_load_fault(void) {
    TEST_BEGIN("SVADE-A-01: A=0 leaf PTE load triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：V=1, R=1, A=0, D=0 */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R,  /* 注意：不带 PTE_A、PTE_D */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("A=0 load triggers load page-fault",
                result == CAUSE_LOAD_PAGE_FAULT);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 2：D bit 清零触发 store page-fault（4 KiB）

**规范依据**：
- `norm:Svade_store_d_bit_clear_pagefault`：D=0 时 store 触发 page-fault
- `norm:Svade_pagefault_cause_match_access_type`：store 触发 scause=15
- `norm:Svade_software_set_ad_then_access`：D=1 时 store 应成功

**测试职责**：验证在 4 KiB 叶 PTE 上，D=0 时 store / AMO 访问会触发 store/AMO page-fault；load 不受 D 位影响；D=1 时 store 成功。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADE-D-01 | D=0 RW 页 store | 4 KiB PTE：V=1, R=1, W=1, A=1, D=0，S-mode store | store page-fault（scause=15） |
| SVADE-D-02 | D=0 RW 页 load | 4 KiB PTE：V=1, R=1, W=1, A=1, D=0，S-mode load | 读取成功（D 不影响 load） |
| SVADE-D-03 | D=1 RW 页 store | 4 KiB PTE：V=1, R=1, W=1, A=1, D=1，S-mode store | 写入成功 |
| SVADE-D-04 | A=0 D=0 RW 页 store | 4 KiB PTE：V=1, R=1, W=1, A=0, D=0，S-mode store | store page-fault（scause=15，A 同时缺失也触发） |
| SVADE-D-05 | D=0 AMO 操作 | 4 KiB PTE：V=1, R=1, W=1, A=1, D=0，S-mode `amoadd.w` | store/AMO page-fault（scause=15） |

```c
/* SVADE-D-01 示例：D=0 RW 页 store 触发 page-fault */

static uintptr_t smode_store_expect_fault(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    *ptr = 0xDEADBEEF;
    return 0;
}

TEST_REGISTER(test_svade_d_clear_store_fault);
bool test_svade_d_clear_store_fault(void) {
    TEST_BEGIN("SVADE-D-01: D=0 leaf PTE store triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射 RW 页：A=1, D=0 */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A,  /* 不带 PTE_D */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, smode_store_expect_fault,
                                        test_va);
    TEST_ASSERT("D=0 store triggers store page-fault",
                result == CAUSE_STORE_PAGE_FAULT);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 3：硬件不更新 A/D bit

**规范依据**：
- `norm:Svade_no_hw_update_ad`：Svade 实现下，触发 page-fault 后硬件不得自动设置 A/D 位
- `norm:Svade_software_set_ad_then_access`：软件设置 A/D 位 + SFENCE.VMA 后访问应成功

**测试职责**：验证 Svade 与"非 Svade（硬件更新）"方案的根本区别——发生 A/D 触发的 page-fault 后，PTE 内容保持原状；软件设置后重试应成功。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADE-NOUPD-01 | A=0 fault 后 PTE 不变 | 触发 SVADE-A-01 后回到 M-mode 读取 PTE 检查 A bit | PTE.A 仍为 0 |
| SVADE-NOUPD-02 | D=0 fault 后 PTE 不变 | 触发 SVADE-D-01 后回到 M-mode 读取 PTE 检查 D bit | PTE.D 仍为 0 |
| SVADE-NOUPD-03 | 软件置 A=1 重试 load | A=0 load fault → trap handler 设置 A=1 + SFENCE.VMA → 重试 | 读取成功 |
| SVADE-NOUPD-04 | 软件置 D=1 重试 store | D=0 store fault → trap handler 设置 D=1 + SFENCE.VMA → 重试 | 写入成功 |
| SVADE-NOUPD-05 | 多次 load 同一 A=0 页 | 连续 N 次 load A=0 页面 | 每次均触发 page-fault（PTE 永远不变） |

```c
/* SVADE-NOUPD-01 示例：A=0 fault 后 PTE.A 仍为 0 */

extern uintptr_t pt_read_pte(pt_context_t *ctx, uintptr_t va, int level);

TEST_REGISTER(test_svade_no_hw_update_a);
bool test_svade_no_hw_update_a(void) {
    TEST_BEGIN("SVADE-NOUPD-01: A bit not updated by HW after page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R, PT_LEVEL_4K);

    /* 触发 load page-fault */
    uintptr_t result = vm_run_in_smode(&ctx, smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("Load triggers page-fault",
                result == CAUSE_LOAD_PAGE_FAULT);

    /* 回到 M-mode 后读取 PTE，验证 A 位仍为 0 */
    uintptr_t pte = pt_read_pte(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("A bit remains 0 after fault", (pte & PTE_A) == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 4：2 MiB megapage 上的 A/D 行为

**规范依据**：
- `norm:Svade_applies_all_levels`：A/D 检查在叶 PTE 上执行，2 MiB megapage 同样适用

**测试职责**：验证 Svade 行为在 2 MiB megapage（Sv39 中 level 1 叶 PTE）上同样生效。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADE-2M-01 | 2M A=0 load fault | 2 MiB leaf PTE：V=1, R=1, A=0，S-mode load | load page-fault（scause=13） |
| SVADE-2M-02 | 2M D=0 store fault | 2 MiB leaf PTE：V=1, R=1, W=1, A=1, D=0，S-mode store | store page-fault（scause=15） |
| SVADE-2M-03 | 2M A=1 D=1 访问成功 | 2 MiB leaf PTE：V=1, R=1, W=1, A=1, D=1，S-mode load + store | 读写均成功 |
| SVADE-2M-04 | 2M A=0 fetch fault | 2 MiB leaf PTE：V=1, X=1, A=0，S-mode 跳转执行 | instruction page-fault（scause=12） |
| SVADE-2M-05 | 2M 区域内多偏移访问 | A=0 megapage，访问区域内偏移 0、0x1000、0x100000、0x1FF000 | 每次访问均触发 page-fault |

```c
/* SVADE-2M-01 示例：2 MiB megapage A=0 load 触发 page-fault */
TEST_REGISTER(test_svade_2m_a_clear_load_fault);
bool test_svade_2m_a_clear_load_fault(void) {
    TEST_BEGIN("SVADE-2M-01: 2 MiB megapage A=0 load triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 2 MiB megapage 映射：A=0 */
    uintptr_t test_va = TEST_REGION_BASE & ~(PAGE_SIZE_2M - 1);
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R,  /* A=0, D=0 */
                PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("2 MiB A=0 load triggers load page-fault",
                result == CAUSE_LOAD_PAGE_FAULT);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 5：1 GiB gigapage 上的 A/D 行为

**规范依据**：
- `norm:Svade_applies_all_levels`：A/D 检查在叶 PTE 上执行，1 GiB gigapage 同样适用

**测试职责**：验证 Svade 行为在 1 GiB gigapage（Sv39 中 level 2 叶 PTE）上同样生效。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADE-1G-01 | 1G A=0 load fault | 1 GiB leaf PTE：V=1, R=1, A=0，S-mode load | load page-fault（scause=13） |
| SVADE-1G-02 | 1G D=0 store fault | 1 GiB leaf PTE：V=1, R=1, W=1, A=1, D=0，S-mode store | store page-fault（scause=15） |
| SVADE-1G-03 | 1G A=1 D=1 访问成功 | 1 GiB leaf PTE：V=1, R=1, W=1, A=1, D=1，S-mode load + store | 读写均成功 |
| SVADE-1G-04 | 1G 区域内多偏移访问 | A=0 gigapage，访问区域内多个偏移 | 每次访问均触发 page-fault |

> [!NOTE]
> 1 GiB gigapage 测试需要确保测试 VA 选择在远离 M-mode 代码段与栈的恒等映射区域，避免与代码段所在的 1 GiB 区域冲突。

```c
/* SVADE-1G-01 示例：1 GiB gigapage A=0 load 触发 page-fault */
TEST_REGISTER(test_svade_1g_a_clear_load_fault);
bool test_svade_1g_a_clear_load_fault(void) {
    TEST_BEGIN("SVADE-1G-01: 1 GiB gigapage A=0 load triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* 代码段所在 1 GiB 区域：完整属性 */
    uintptr_t code_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_map_page(&ctx, code_base, code_base, code_flags, PT_LEVEL_1G);

    /* 测试用 1 GiB gigapage：选用与代码段不同的 1 GiB 区域，A=0 */
    uintptr_t test_va = code_base + PAGE_SIZE_1G;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R,  /* A=0, D=0 */
                PT_LEVEL_1G);

    uintptr_t result = vm_run_in_smode(&ctx, smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("1 GiB A=0 load triggers load page-fault",
                result == CAUSE_LOAD_PAGE_FAULT);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 6：非叶 PTE 的 A/D bit 不参与 Svade 检查

**规范依据**：
- `norm:Svade_non_leaf_ad_reserved`：非叶 PTE 的 D、A、U 位保留为未来标准使用，软件应清零；Svade 检查仅在叶 PTE 上进行

**测试职责**：验证 Svade 的 A/D 检查仅作用于叶 PTE，与中间层非叶 PTE 的 A/D 位无关。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADE-NL-01 | 非叶 A=0 叶 A=1 load | 中间层 PTE A=0、D=0（标准用法），4 KiB 叶 PTE A=1，S-mode load | 访问成功（叶 PTE A=1 决定） |
| SVADE-NL-02 | 非叶 A=0 叶 A=0 load | 中间层 PTE A=0，叶 PTE A=0，S-mode load | load page-fault（叶 PTE A=0 触发） |
| SVADE-NL-03 | 非叶 D=0 叶 D=1 store | 中间层 PTE D=0，叶 PTE A=1, D=1, W=1，S-mode store | 写入成功（叶 PTE D=1 决定） |

```c
/* SVADE-NL-01 示例：非叶 PTE A=0、叶 PTE A=1 时访问成功 */
TEST_REGISTER(test_svade_non_leaf_a_ignored);
bool test_svade_non_leaf_a_ignored(void) {
    TEST_BEGIN("SVADE-NL-01: Non-leaf PTE A bit does not affect Svade check");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 4 KiB 叶 PTE：A=1, D=1；中间层 PTE 由 pt_map_page 自动创建并按规范清零 A/D */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 显式确认中间层 PTE 的 A/D 位为 0（标准用法） */
    uintptr_t mid_pte = pt_read_pte(&ctx, test_va, PT_LEVEL_2M);
    TEST_ASSERT("Mid-level PTE A=0", (mid_pte & PTE_A) == 0);
    TEST_ASSERT("Mid-level PTE D=0", (mid_pte & PTE_D) == 0);

    /* load 应成功 */
    uintptr_t result = vm_run_in_smode(&ctx, smode_load, test_va);
    TEST_ASSERT("Load succeeds when leaf A=1 regardless of non-leaf A",
                result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 7：scause 与访问类型的对应关系

**规范依据**：
- `norm:Svade_pagefault_cause_match_access_type`：page-fault 的 scause 必须与原始访问类型对应

**测试职责**：集中验证 Svade 触发的 page-fault 的 scause 在 load / store / fetch / AMO 四种访问类型下分别为 13 / 15 / 12 / 15。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADE-CAUSE-01 | load → scause=13 | A=0 R 页执行 `lw` | scause = `CAUSE_LOAD_PAGE_FAULT` (13) |
| SVADE-CAUSE-02 | store → scause=15 | A=1 D=0 RW 页执行 `sw` | scause = `CAUSE_STORE_PAGE_FAULT` (15) |
| SVADE-CAUSE-03 | fetch → scause=12 | A=0 X 页跳转执行 | scause = `CAUSE_FETCH_PAGE_FAULT` (12) |
| SVADE-CAUSE-04 | AMO → scause=15 | A=1 D=0 RW 页执行 `amoadd.w` | scause = `CAUSE_STORE_PAGE_FAULT` (15) |
| SVADE-CAUSE-05 | A=0 store → scause=15 | A=0 D=0 RW 页执行 `sw` | scause = `CAUSE_STORE_PAGE_FAULT` (15)（store 类型优先） |

> [!IMPORTANT]
> SVADE-CAUSE-05 验证当一个 store 操作同时触发 A=0 与 D=0 时，由于步骤 9 的判定条件是「A=0 或（store 且 D=0）」，整个判定属于 store 访问，因此 scause 应为 store page-fault（15），而非 load page-fault（13）。

```c
/* SVADE-CAUSE-04 示例：AMO 在 D=0 时触发 store/AMO page-fault */
static uintptr_t smode_amoadd_expect_fault(uintptr_t addr) {
    uint32_t result;
    asm volatile ("amoadd.w %0, %1, (%2)"
                  : "=r"(result)
                  : "r"(1), "r"(addr)
                  : "memory");
    return 0;
}

TEST_REGISTER(test_svade_cause_amo);
bool test_svade_cause_amo(void) {
    TEST_BEGIN("SVADE-CAUSE-04: AMO with D=0 triggers store page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A,  /* A=1, D=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, smode_amoadd_expect_fault,
                                        test_va);
    TEST_ASSERT("AMO with D=0 triggers store/AMO page-fault",
                result == CAUSE_STORE_PAGE_FAULT);

    pt_pool_reset();
    TEST_END();
}
```

---

## 附录：相关常量与 API 参考

### scause 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| `CAUSE_FETCH_PAGE_FAULT` | 12 | Instruction page fault |
| `CAUSE_LOAD_PAGE_FAULT` | 13 | Load page fault |
| `CAUSE_STORE_PAGE_FAULT` | 15 | Store/AMO page fault |

### 测试框架 API（与 svnapot/svpbmt 一致）

- `pt_context_t` / `pt_init` / `pt_pool_reset`：页表上下文管理
- `pt_setup_identity_mapping(ctx, base, size, flags, level)`：批量恒等映射
- `pt_map_page(ctx, va, pa, flags, level)`：单页映射，level 取 `PT_LEVEL_4K` / `PT_LEVEL_2M` / `PT_LEVEL_1G`
- `pt_read_pte(ctx, va, level)`：读取指定层级的 PTE 值（用于验证 A/D 位）
- `vm_run_in_smode(ctx, fn, arg)`：以指定页表上下文进入 S-mode 执行 fn(arg)，发生异常时返回 scause
- `TEST_REGISTER` / `TEST_BEGIN` / `TEST_ASSERT` / `TEST_END`：测试用例注册与断言宏

### PTE flag 宏

- `PTE_V`、`PTE_R`、`PTE_W`、`PTE_X`、`PTE_U`、`PTE_G`、`PTE_A`、`PTE_D`

---

## 测试执行说明

- 所有测试用例运行于 Sv39 模式（`SATP_MODE_SV39`）
- 测试入口位于 `sv39/main.c`，可在 `sv39/tests/test_svade.c`（待实现）中添加上述用例
- M-mode 下使能 Svade 扩展（具体方式取决于实现，例如通过 `menvcfg` 或硬连线启用）
- S-mode trap handler 需要捕获 page-fault 并将 scause 通过 `vm_run_in_smode` 的返回值传递回 M-mode
