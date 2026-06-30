**中文 | [English](../testplan_en/Ssccptr_test_plan_en.md)**

# Ssccptr 扩展测试计划

本文档描述 Ssccptr（Main Memory Page-Table Reads, Version 1.0）扩展的测试计划。Ssccptr 扩展规定：如果实现了该扩展，则具有 cacheability（可缓存性）和 coherence（一致性）两个 PMA 属性的主存区域，必须支持硬件页表读取（hardware page-table reads）。

---

## 概述

RISC-V 特权级规范中，MMU 在执行虚拟地址翻译时需要从内存中读取页表条目（PTE），即 hardware page-table walk。页表数据所在的物理内存区域是否支持此类隐式读操作，取决于该区域的 Physical Memory Attributes（PMA）。

Ssccptr 扩展对此做出了明确约束：

- **核心要求**：同时具有 cacheability 和 coherence 两个 PMA 的主存区域，**必须**能够被 MMU 的 page-table walker 正确读取。
- **隐含语义**：在符合 Ssccptr 的实现上，软件只要将页表放置在满足上述 PMA 条件的常规主存中，即可保证硬件 page walk 正常工作，无需额外的 PMA 配置或特殊处理。

这一约束看似简单，但其验证涉及多个维度：不同页表级别、不同虚拟内存模式、不同访问类型、多级 page walk 的每一级读取、以及与 PMP 的交互。

---

## 测试范围

### 规范来源

- `SPEC/ssccptr.adoc` — Ssccptr Extension for Main Memory Page-Table Reads, Version 1.0

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:ssccptr_memory_pte_reads` | If the Ssccptr extension is implemented, then main memory regions with both the cacheability and coherence PMAs must support hardware page-table reads. | 如果实现了 Ssccptr 扩展，则同时具有可缓存性和一致性 PMA 的主存区域必须支持硬件页表读取。 |
| `norm:Ssccptr_all_pt_levels` | — | 硬件页表读取在所有页表级别（L0/L1/L2 for Sv39, 加 L3 for Sv48, 加 L4 for Sv57）均须正常工作 |
| `norm:Ssccptr_all_access_types` | — | 硬件页表读取对 load、store、instruction fetch 三种触发 page walk 的访问类型均须正常工作 |
| `norm:Ssccptr_all_priv_modes` | — | 硬件页表读取在 S-mode 和 U-mode 触发的 page walk 中均须正常工作 |
| `norm:Ssccptr_multiLevel_walk` | — | 多级页表遍历中，每一级非叶 PTE 的读取和叶 PTE 的读取均须成功 |
| `norm:Ssccptr_superpage` | — | 超级页（megapage / gigapage）的叶 PTE 读取须正常工作 |

### 不在测试范围内

- **非 cacheable / 非 coherent 区域的行为**：Ssccptr 仅约束同时满足 cacheability + coherence 的区域，对其他 PMA 组合不做要求
- **I/O 区域的页表放置**：将页表放在 I/O 空间（不满足 PMA 条件）的行为不在 Ssccptr 约束范围内
- **Sv32 模式**：本计划仅覆盖 RV64（Sv39 / Sv48 / Sv57），与项目其它扩展测试计划保持一致
- **多 hart 场景**：项目为单核测试环境
- **PMA 寄存器的配置与发现**：PMA 通常是平台级别的硬连线属性，非软件可编程，测试计划依赖平台主存默认满足 PMA 条件这一前提
- **Hypervisor 两级翻译（VS-stage + G-stage）**：由独立的 hypervisor 测试计划覆盖

---

## 前提与约束

> [!IMPORTANT]
> Ssccptr 的核心约束是关于 PMA（Physical Memory Attributes）的，而 PMA 通常是平台硬连线属性，不是软件可动态配置的。在 Spike 仿真器上，主存区域默认具有 cacheability 和 coherence PMA。因此本测试计划的验证策略是：**在默认主存（满足 PMA 条件）中建立页表，通过端到端的 page walk 成功（或失败符合预期）来间接验证硬件页表读取能力**。

### 关键参考文件

| 路径 | 说明 |
|------|------|
| `SPEC/ssccptr.adoc` | Ssccptr 规范全文 |
| `common/vm/page_table.c` | 页表管理实现（分配、映射、遍历） |
| `common/vm/vm.h` | VM 公共 API（`vm_run_in_smode` 等） |
| `common/vm/vm_defs.h` | PTE 位定义、VA/PA 宏 |
| `common/vm/satp.c` | `vm_enable` / `vm_run_in_smode` 实现 |
| `common/test_framework.c` | 测试框架（TEST_BEGIN / TEST_ASSERT / TEST_END） |
| `common/pmp/pmp_cfg.h` | PMP 配置 API |
| `pmp_sv39/pmp_vm_common.c` | PMP+VM 交互测试参考（对比用例） |

### 设计原则

1. **间接验证法**：由于无法直接观测 MMU 的 page-table read 操作，通过设置页表 → 开启虚拟内存 → 执行不同类型的访问 → 验证结果正确（无 access fault）来间接证明 page walk 成功
2. **对照组设计**：在验证 page walk 成功的同时，通过 PMP 阻断页表读取的对照组，证明测试有能力检测到 page walk 失败（排除假阳性）
3. **逐级覆盖**：针对每一级页表分别验证，确保不是仅仅因为 TLB 命中而跳过了 page walk

---

## 测试分组

### Group 1：基本页表遍历验证（Sv39, 4 KiB 页）

**规范依据**：
- `norm:Ssccptr_cacheable_coherent_supports_pt_read`：主存区域必须支持硬件页表读取
- `norm:Ssccptr_all_access_types`：load / store / fetch 均须正常
- `norm:Ssccptr_all_priv_modes`：S-mode 和 U-mode 均须正常

**测试职责**：在默认主存上建立 Sv39 三级页表（identity mapping，L0/L1/L2 三级），验证 4 KiB 页面的 page walk 在不同访问类型和特权模式下均能成功完成。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-BASIC-01 | Sv39 4K 页 S-mode load | 建立 Sv39 identity mapping（4 KiB 页，PTE 带 RWXAD），S-mode load | 读取成功，无异常 |
| SSCCPTR-BASIC-02 | Sv39 4K 页 S-mode store | 同上配置，S-mode store | 写入成功，无异常 |
| SSCCPTR-BASIC-03 | Sv39 4K 页 S-mode fetch | 同上配置（X=1），S-mode 跳转执行 | 取指执行成功 |
| SSCCPTR-BASIC-04 | Sv39 4K 页 U-mode load | 建立带 PTE_U 标记的 4 KiB mapping，U-mode load | 读取成功，无异常 |
| SSCCPTR-BASIC-05 | Sv39 4K 页 U-mode store | 同上配置，U-mode store | 写入成功，无异常 |
| SSCCPTR-BASIC-06 | Sv39 4K 页 U-mode fetch | 同上配置（X=1, U=1），U-mode 跳转执行 | 取指执行成功 |

```c
/* SSCCPTR-BASIC-01 示例 */

/* ===================================================================
 * S-mode 辅助函数（定义在 test_helpers.h 中）
 *
 * 每个辅助函数内部使用 trap_expect_begin/end + trap_get_cause()
 * 来检测异常。vm_run_in_smode() 返回辅助函数的返回值：
 *   - 返回 0 表示操作成功（无异常）
 *   - 返回非 0 值为 scause（异常原因码）
 * =================================================================== */

static uintptr_t test_smode_load(uintptr_t arg) {
    trap_expect_begin();
    volatile uintptr_t val = *(volatile uintptr_t *)arg;
    (void)val;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

static uintptr_t test_smode_store(uintptr_t arg) {
    trap_expect_begin();
    *(volatile uintptr_t *)arg = 0xDEADBEEF12345678UL;
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

static uintptr_t test_smode_exec(uintptr_t arg) {
    trap_expect_begin();
    exec_at(arg);
    trap_expect_end();
    if (trap_was_triggered())
        return trap_get_cause();
    return 0;
}

/* ===================================================================
 * 测试数据区域（通过链接脚本 kernel.ld 提供）
 *
 * .vm_test_region 包含 3 个连续的 4 KiB 页面：
 *   - test_data_area:  读写数据区
 *   - test_fault_page: 异常测试页（用于自定义权限映射）
 *   - test_exec_page:  可执行代码区（需调用 init_exec_page() 填充 nop;ret）
 *
 * 链接脚本中需 2 MiB 对齐，以便 setup_code_mapping() 跳过该区域，
 * 允许各测试对这些页面设置不同的 PTE 属性。
 * =================================================================== */

TEST_REGISTER(test_ssccptr_basic_sv39_4k_smode_load);
bool test_ssccptr_basic_sv39_4k_smode_load(void) {
    TEST_BEGIN("SSCCPTR-BASIC-01: Sv39 4K page S-mode load succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("S-mode load via page walk succeeds",
                   result, 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 2：超级页 page walk 验证（Sv39, 2 MiB / 1 GiB）

**规范依据**：
- `norm:Ssccptr_cacheable_coherent_supports_pt_read`
- `norm:Ssccptr_superpage`：超级页的叶 PTE 读取须正常工作
- `norm:Ssccptr_multiLevel_walk`：不同深度的 page walk

**测试职责**：验证 2 MiB megapage（L1 叶 PTE）和 1 GiB gigapage（L2 叶 PTE）的 page walk 成功。超级页的 page walk 层级更少，覆盖不同深度的遍历路径。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-SUPER-01 | Sv39 2M megapage S-mode load | L1 叶 PTE identity mapping，S-mode load | 成功 |
| SSCCPTR-SUPER-02 | Sv39 2M megapage S-mode store | 同上，S-mode store | 成功 |
| SSCCPTR-SUPER-03 | Sv39 2M megapage S-mode fetch | 同上（X=1），S-mode fetch | 成功 |
| SSCCPTR-SUPER-04 | Sv39 1G gigapage S-mode load | L2 叶 PTE identity mapping，S-mode load | 成功 |
| SSCCPTR-SUPER-05 | Sv39 1G gigapage S-mode store | 同上，S-mode store | 成功 |
| SSCCPTR-SUPER-06 | Sv39 1G gigapage S-mode fetch | 同上（X=1），S-mode fetch | 成功 |

```c
/* SSCCPTR-SUPER-01 示例 */

TEST_REGISTER(test_ssccptr_super_sv39_2m_load);
bool test_ssccptr_super_sv39_2m_load(void) {
    TEST_BEGIN("SSCCPTR-SUPER-01: Sv39 2M megapage S-mode load succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* 使用 2 MiB megapage identity mapping 覆盖代码和数据区域 */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, 16 * PAGE_SIZE_2M,
                              flags, PT_LEVEL_2M);

    /* test_region_2m_va 由链接脚本 .vm_test_region_2m 段提供，
     * 自然 2 MiB 对齐，作为 megapage 的测试目标地址 */
    uintptr_t test_va = test_region_2m_va;
    pt_map_page(&ctx, test_va, test_va, flags, PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("2M megapage load via page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 3：多级 page walk 逐级验证（Sv39）

**规范依据**：
- `norm:Ssccptr_multiLevel_walk`：多级页表遍历中，每一级 PTE 的读取均须成功
- `norm:Ssccptr_all_pt_levels`：所有页表级别均须支持

**测试职责**：通过 SFENCE.VMA 清除 TLB 后，执行访问以强制触发完整的 page walk，验证每一级非叶 PTE 和叶 PTE 的读取。通过 PMP 对照组验证测试有效性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-LEVEL-01 | L0 PTE 读取验证 | 4 KiB 页，SFENCE.VMA 清 TLB 后 load，page walk 经过 L2→L1→L0 | 成功 |
| SSCCPTR-LEVEL-02 | L1 PTE 读取验证 | 2 MiB 页，SFENCE.VMA 清 TLB 后 load，page walk 经过 L2→L1 | 成功 |
| SSCCPTR-LEVEL-03 | L2 PTE 读取验证 | 1 GiB 页，SFENCE.VMA 清 TLB 后 load，page walk 经过 L2 | 成功 |
| SSCCPTR-LEVEL-04 | PMP 阻断 L0 PT 对照 | PMP 设置 L0 页表页为不可读，SFENCE.VMA 后 load | load access fault |
| SSCCPTR-LEVEL-05 | PMP 阻断 L1 PT 对照 | PMP 设置 L1 页表页为不可读，SFENCE.VMA 后 load | load access fault |
| SSCCPTR-LEVEL-06 | PMP 阻断 L2 PT（root）对照 | PMP 设置 root 页表页为不可读，SFENCE.VMA 后 load | load access fault |

> [!NOTE]
> SSCCPTR-LEVEL-04/05/06 是**对照组**：通过 PMP 故意阻断某一级页表的读取，验证测试框架能检测到 page walk 失败。如果对照组触发 access fault，而正常组（01/02/03）不触发，则能有效证明正常组的 page walk 确实经过了该级页表。

```c
/* SSCCPTR-LEVEL-01 示例：使用 4 KiB 页映射，强制 page walk 经过 L2→L1→L0 */

TEST_REGISTER(test_ssccptr_level_l0);
bool test_ssccptr_level_l0(void) {
    TEST_BEGIN("SSCCPTR-LEVEL-01: L0 PTE read via page walk succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    /* 4 KiB 页映射：page walk 需经 L2(root)→L1→L0 三级 */
    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* vm_run_in_smode 每次调用都会先 vm_enable → SFENCE.VMA，
     * 退出后 vm_disable，因此每次进入都触发完整的 page walk */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("L0 PTE read succeeds (full L2→L1→L0 walk)",
                   result, 0);

    pt_pool_reset();
    TEST_END();
}

/* SSCCPTR-LEVEL-04 示例：PMP 阻断 L0 页表页读取（对照组）
 *
 * 复用 pmp_sv39/pmp_vm_common.c 中 run_pmp_on_pte_test() 的模式：
 *   1. 建立 identity mapping
 *   2. 用 get_pt_page_addr() 获取 L0 页表页的物理地址
 *   3. 用 PMP 将该页标记为 X-only（不可读），阻断 MMU 的 PTE 读取
 *   4. 验证 load 触发 CAUSE_LAF（load access fault）
 */
TEST_REGISTER(test_ssccptr_level_l0_pmp_block);
bool test_ssccptr_level_l0_pmp_block(void) {
    TEST_BEGIN("SSCCPTR-LEVEL-04: PMP blocks L0 PT read -> access fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 获取 L0 页表页的物理地址 */
    uintptr_t pt_page_pa = get_pt_page_addr(&ctx, test_va, 0);

    /* PMP: 将 L0 页表页设为 X-only（阻断读取） */
    pmp_entry_t e0 = PMP_ENTRY_NAPOT(pt_page_pa, PAGE_SIZE_4K, PMP_X);
    pmp_set_entry(0, &e0);

    pmp_entry_t e15 = PMP_ENTRY_NAPOT(0,
        (uintptr_t)1 << (__riscv_xlen - 1), PMP_RWX);
    pmp_set_entry(15, &e15);

    vm_enable(&ctx, 0);

    goto_priv(PRIV_S);
    PRIV_DO_TRAP(mem_load8(test_va));
    goto_priv(PRIV_M);
    CHECK_TRAP("load: PTE read blocked", CAUSE_LAF);

    vm_disable();
    pmp_clear_all();
    pt_pool_reset();
    TEST_END();
}
```

---

### Group 4：Sv48 页表遍历验证

**规范依据**：
- `norm:Ssccptr_cacheable_coherent_supports_pt_read`
- `norm:Ssccptr_all_pt_levels`：Sv48 增加第 4 级（L3）页表

**测试职责**：验证 Sv48 模式下所有页表级别的 page walk 成功，覆盖 L3 根页表的读取。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-SV48-01 | Sv48 4K 页 S-mode load | Sv48 identity mapping（4 KiB），S-mode load | 成功 |
| SSCCPTR-SV48-02 | Sv48 4K 页 S-mode store | 同上，S-mode store | 成功 |
| SSCCPTR-SV48-03 | Sv48 4K 页 S-mode fetch | 同上（X=1），S-mode fetch | 成功 |
| SSCCPTR-SV48-04 | Sv48 2M megapage load | Sv48 2M 页 mapping，S-mode load | 成功 |
| SSCCPTR-SV48-05 | Sv48 1G gigapage load | Sv48 1G 页 mapping，S-mode load | 成功 |
| SSCCPTR-SV48-06 | Sv48 512G terapage load | Sv48 L3 叶 PTE mapping，S-mode load | 成功 |

> [!NOTE]
> Sv48 的 512G terapage（`PT_LEVEL_512G`）要求 `PLATFORM_MEM_BASE` 按 512 GiB 对齐。在 QEMU virt 平台上（MEM_BASE=0x80000000 = 2 GiB），该对齐条件无法满足，测试应检测到并 SKIP。参考 `pmp_sv48/pmp_vm_common.c` 中 512G 级别的可行性检查逻辑。

```c
/* SSCCPTR-SV48-01 示例 */

TEST_REGISTER(test_ssccptr_sv48_4k_load);
bool test_ssccptr_sv48_4k_load(void) {
    TEST_BEGIN("SSCCPTR-SV48-01: Sv48 4K page S-mode load succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);
    TEST_ASSERT("code mapping", setup_code_mapping_sv48(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("Sv48 4K page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 5：Sv57 页表遍历验证

**规范依据**：
- `norm:Ssccptr_cacheable_coherent_supports_pt_read`
- `norm:Ssccptr_all_pt_levels`：Sv57 增加第 5 级（L4）页表

**测试职责**：验证 Sv57 模式下所有页表级别的 page walk 成功，覆盖 L4 根页表的读取。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-SV57-01 | Sv57 4K 页 S-mode load | Sv57 identity mapping（4 KiB），S-mode load | 成功 |
| SSCCPTR-SV57-02 | Sv57 4K 页 S-mode store | 同上，S-mode store | 成功 |
| SSCCPTR-SV57-03 | Sv57 4K 页 S-mode fetch | 同上（X=1），S-mode fetch | 成功 |
| SSCCPTR-SV57-04 | Sv57 2M megapage load | Sv57 2M 页 mapping，S-mode load | 成功 |
| SSCCPTR-SV57-05 | Sv57 1G gigapage load | Sv57 1G 页 mapping，S-mode load | 成功 |
| SSCCPTR-SV57-06 | Sv57 512G terapage load | Sv57 L3 叶 PTE mapping，S-mode load | 成功 |
| SSCCPTR-SV57-07 | Sv57 256T petapage load | Sv57 L4 叶 PTE mapping，S-mode load | 成功 |

> [!NOTE]
> Sv57 的 512G terapage 和 256T petapage 分别要求 512 GiB 和 256 TiB 对齐。在常规仿真平台上这些对齐条件通常无法满足，测试应检测到并 SKIP。Sv57 本身也需要 Spike/QEMU 支持 `SATP_MODE_SV57`，若不支持则整组 SKIP。

```c
/* SSCCPTR-SV57-01 示例 */

TEST_REGISTER(test_ssccptr_sv57_4k_load);
bool test_ssccptr_sv57_4k_load(void) {
    TEST_BEGIN("SSCCPTR-SV57-01: Sv57 4K page S-mode load succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV57);
    TEST_ASSERT("code mapping", setup_code_mapping_sv57(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("Sv57 4K page walk succeeds", result, 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 6：TLB 刷新后的重复 page walk

**规范依据**：
- `norm:Ssccptr_cacheable_coherent_supports_pt_read`：每次 page walk 均须成功
- `norm:Ssccptr_multiLevel_walk`

**测试职责**：验证在 SFENCE.VMA 清除 TLB 后，重复触发 page walk 均能成功完成，排除 TLB 缓存掩盖 page walk 失败的可能。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-TLB-01 | 连续 TLB flush + load | 4 KiB 页，循环 N 次：SFENCE.VMA → S-mode load | 每次均成功 |
| SSCCPTR-TLB-02 | 连续 TLB flush + store | 4 KiB 页，循环 N 次：SFENCE.VMA → S-mode store | 每次均成功 |
| SSCCPTR-TLB-03 | 连续 TLB flush + fetch | 4 KiB 页，循环 N 次：SFENCE.VMA → S-mode fetch | 每次均成功 |
| SSCCPTR-TLB-04 | 不同地址的交替 page walk | 两个不同 VA 的 4 KiB 页，交替 SFENCE.VMA + load | 每次均成功 |

```c
/* SSCCPTR-TLB-01 示例 */

TEST_REGISTER(test_ssccptr_tlb_repeated_walk);
bool test_ssccptr_tlb_repeated_walk(void) {
    TEST_BEGIN("SSCCPTR-TLB-01: Repeated page walk after TLB flush");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 重复 page walk 10 次 */
    for (int i = 0; i < 10; i++) {
        uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
        TEST_ASSERT_EQ("repeated page walk succeeds", result, 0);
        /* vm_run_in_smode 内部会在每次退出后 vm_disable()，
         * 重新进入时 vm_enable() + sfence.vma 强制全新 page walk */
    }

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 7：页表动态修改后的 page walk

**规范依据**：
- `norm:Ssccptr_cacheable_coherent_supports_pt_read`：主存必须支持硬件页表读取
- 派生：动态修改页表后 SFENCE.VMA + 再次 page walk 也须成功

**测试职责**：验证在运行时修改 PTE 内容（如更改权限、更改映射目标），执行 SFENCE.VMA 后重新 page walk 能正确读取更新后的 PTE。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-DYN-01 | PTE 权限变更后 walk | R-only → RW，SFENCE.VMA 后 store | 第一次 store fault，修改后 store 成功 |
| SSCCPTR-DYN-02 | PTE 目标 PA 变更后 walk | 重映射 VA 到不同 PA，SFENCE.VMA 后 load | 读到新 PA 的数据 |
| SSCCPTR-DYN-03 | 添加新映射后 walk | 初始无映射 → 添加 PTE → SFENCE.VMA → load | 第一次 page fault，添加后成功 |
| SSCCPTR-DYN-04 | 删除映射后 walk | 有映射 → 清除 PTE.V → SFENCE.VMA → load | 第一次成功，删除后 page fault |

```c
/* SSCCPTR-DYN-01 示例
 *
 * test_smode_store 定义在 test_helpers.h 中（见 Group 1 示例），
 * 内部使用 trap_expect_begin/end + trap_get_cause() 捕获异常。
 */

TEST_REGISTER(test_ssccptr_dyn_perm_change);
bool test_ssccptr_dyn_perm_change(void) {
    TEST_BEGIN("SSCCPTR-DYN-01: PTE permission change after SFENCE.VMA");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;

    /* 初始映射为 R-only */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("R-only page store triggers page-fault",
                   result, CAUSE_SPF);

    /* 更新为 RW（使用 pte_set_bits 添加 W 位，再刷新 TLB） */
    pte_set_bits(&ctx, test_va, PT_LEVEL_4K, PTE_W);

    result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT_EQ("RW page store succeeds after PTE update",
                   result, 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 8：PMP 对 page walk 阻断的对照验证

**规范依据**：
- `norm:Ssccptr_cacheable_coherent_supports_pt_read`（反面验证）
- 对照：PMP 可以阻断 page walk 中的 PTE 读取

**测试职责**：通过 PMP 阻断页表所在物理内存的读取权限，验证 page walk 被正确阻断（产生 access fault 而非 page fault）。此组作为 Group 1-3 的对照，证明正常组的成功确实依赖于 page walk 的正确完成。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCCPTR-PMP-01 | PMP 阻断 L0 PT load | PMP 设 L0 PT 页 X-only，S-mode load | load access fault |
| SSCCPTR-PMP-02 | PMP 阻断 L0 PT store | PMP 设 L0 PT 页 X-only，S-mode store | store access fault |
| SSCCPTR-PMP-03 | PMP 阻断 L0 PT fetch | PMP 设 L0 PT 页 X-only，S-mode fetch | instruction access fault |
| SSCCPTR-PMP-04 | PMP 阻断 L1 PT load | PMP 设 L1 PT 页 X-only，S-mode load | load access fault |
| SSCCPTR-PMP-05 | PMP 阻断 root PT load | PMP 设 root PT 页 X-only，S-mode load | load access fault |
| SSCCPTR-PMP-06 | PMP 允许后 walk 恢复 | 先阻断 → 移除 PMP 限制 → SFENCE.VMA → load | load 成功 |

> [!TIP]
> SSCCPTR-PMP-01~05 的测试逻辑可复用 `pmp_sv39/pmp_vm_common.c` 中 `run_pmp_on_pte_test()` 的模式，设置 PMP entry 将页表页标记为 X-only（不可读），从而阻断 MMU 的 PTE 读取。Group 3 的 SSCCPTR-LEVEL-04 示例已展示了完整的 PMP 阻断模式，此处不再重复。以下仅展示 SSCCPTR-PMP-06（PMP 允许后 walk 恢复）的示例，因为它涉及 PMP 的动态解除。

```c
/* SSCCPTR-PMP-06 示例：先阻断页表读取，再移除 PMP 限制，验证 walk 恢复 */

TEST_REGISTER(test_ssccptr_pmp_restore);
bool test_ssccptr_pmp_restore(void) {
    TEST_BEGIN("SSCCPTR-PMP-06: PMP remove restores page walk");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    TEST_ASSERT("code mapping", setup_code_mapping(&ctx) == 0);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 获取 L0 页表页的物理地址 */
    uintptr_t pt_page_pa = get_pt_page_addr(&ctx, test_va, 0);

    /* 阶段 1：PMP 阻断 L0 页表页读取 → 预期 access fault */
    pmp_entry_t e0 = PMP_ENTRY_NAPOT(pt_page_pa, PAGE_SIZE_4K, PMP_X);
    pmp_set_entry(0, &e0);

    pmp_entry_t e15 = PMP_ENTRY_NAPOT(0,
        (uintptr_t)1 << (__riscv_xlen - 1), PMP_RWX);
    pmp_set_entry(15, &e15);

    vm_enable(&ctx, 0);

    goto_priv(PRIV_S);
    PRIV_DO_TRAP(mem_load8(test_va));
    goto_priv(PRIV_M);
    CHECK_TRAP("blocked: PTE read fails", CAUSE_LAF);

    vm_disable();

    /* 阶段 2：移除 PMP 限制 → 预期 walk 恢复成功 */
    pmp_clear_all();

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT_EQ("restored: page walk succeeds after PMP removal",
                   result, 0);

    pt_pool_reset();
    TEST_END();
}
```

---

## 测试矩阵总览

| Group | 测试数 | VM 模式 | 页粒度 | 权限模式 | 核心验证点 |
|-------|--------|---------|--------|----------|-----------|
| 1 - 基本 page walk | 6 | Sv39 | 4 KiB | S / U | load / store / fetch 三种访问类型 |
| 2 - 超级页 | 6 | Sv39 | 2M / 1G | S | megapage / gigapage 叶 PTE 读取 |
| 3 - 逐级验证 | 6 | Sv39 | 4K / 2M / 1G | S | 每一级 PTE 读取 + PMP 对照 |
| 4 - Sv48 | 6 | Sv48 | 4K / 2M / 1G / 512G | S | L3 根页表读取 |
| 5 - Sv57 | 7 | Sv57 | 4K / 2M / 1G / 512G / 256T | S | L4 根页表读取 |
| 6 - TLB 刷新 | 4 | Sv39 | 4 KiB | S | 重复 page walk 稳定性 |
| 7 - 动态修改 | 4 | Sv39 | 4 KiB | S | PTE 修改后重新 page walk |
| 8 - PMP 对照 | 6 | Sv39 | 4 KiB | S | PMP 阻断 page walk 反向验证 |

**总计：45 个测试用例**

---

## 实现注意事项

### 1. SPIKE_ISA_EXT 配置

Ssccptr 是一个 PMA 层面的约束扩展，不引入新指令或 CSR。在 Spike 中，主存区域默认满足 cacheability + coherence PMA 条件，因此 Ssccptr 的语义在默认配置下已被满足。Makefile 中可通过 `SPIKE_ISA_EXT = _ssccptr` 声明该扩展（参考 `svadu/Makefile` 的 `SPIKE_ISA_EXT = _svadu` 模式），但需确认 Spike 版本是否识别该扩展名。若 Spike 不识别 `_ssccptr`，可暂时省略此配置，仅通过 `ENABLE_VM = 1` 启用虚拟内存支持即可运行测试。

### 2. 目录结构

参考 `svade/` 目录的组织方式（`main.c` + `kernel.ld` + `tests/` 子目录）：

```
ssccptr/
├── Makefile                    # 构建配置（ENABLE_VM=1, SPIKE_ISA_EXT）
├── main.c                      # 测试入口（遍历 _test_table 自动执行）
├── kernel.ld                   # 链接脚本（含 .vm_test_region 段）
└── tests/
    ├── test_helpers.h          # S-mode 辅助函数、测试区域宏、setup_code_mapping
    ├── test_register.c         # 测试注册文件（#include 各组测试）
    ├── test_basic.c            # Group 1: 基本 page walk 验证
    ├── test_superpage.c        # Group 2: 超级页 page walk
    ├── test_level.c            # Group 3: 逐级验证 + PMP 对照
    ├── test_sv48.c             # Group 4: Sv48 页表遍历
    ├── test_sv57.c             # Group 5: Sv57 页表遍历
    ├── test_tlb.c              # Group 6: TLB 刷新后重复 page walk
    ├── test_dynamic.c          # Group 7: 动态修改后 page walk
    └── test_pmp_block.c        # Group 8: PMP 对照验证
```

### 3. 与已有测试的复用

- **页表管理**：直接使用 `common/vm/page_table.c` 的 `pt_init` / `pt_map_page` / `pt_setup_identity_mapping`
- **VM 进入/退出**：使用 `common/vm/satp.c` 的 `vm_run_in_smode` / `vm_run_in_umode`
- **S-mode 辅助函数**：参考 `svade/tests/test_helpers.h` 中的 `test_smode_load` / `test_smode_store` / `test_smode_exec` 模式（内部使用 `trap_expect_begin/end` + `trap_get_cause()`）
- **PMP 配置**（Group 3/8）：使用 `common/pmp/pmp_cfg.h` 的 `pmp_set_entry` / `PMP_ENTRY_NAPOT` / `pmp_clear_all`
- **页表页地址获取**（Group 3/8）：使用 `common/vm/vm.h` 的 `get_pt_page_addr`
- **PTE 检查/修改**（Group 7）：使用 `common/vm/vm.h` 的 `pt_get_pte` 直接修改 PTE 内容
- **异常原因常量**：使用 `common/encoding.h` 中定义的短别名 `CAUSE_LAF` / `CAUSE_SAF` / `CAUSE_IAF` / `CAUSE_LPF` / `CAUSE_SPF` / `CAUSE_IPF`

### 4. PMA 前提假设

> [!WARNING]
> 本测试计划假设 Spike 仿真器和目标硬件的主存区域默认满足 cacheability + coherence PMA 条件。如果在特定平台上主存 PMA 不满足此条件，相关测试可能会因 page walk 失败而非 Ssccptr 不合规。在解读测试结果时需关注此前提。
