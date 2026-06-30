**中文 | [English](../testplan_en/Svadu_test_plan_en.md)**

# Svadu 扩展测试计划

本文档描述 Svadu（Hardware Updating of A/D Bits）扩展的测试计划。Svadu 扩展为 PTE A/D 位的**硬件自动更新**提供支持，并通过 `menvcfg.ADUE` 字段允许在运行时启用/禁用该行为；当硬件更新被禁用时，处理器回退到 Svade 行为（A/D 触发 page-fault，由软件设置）。

---

## 概述

RISC-V 特权级规范中定义了两种 PTE A/D 位管理方案：

1. **硬件更新方案（Svadu 启用）**：当 A=0 被访问或 D=0 被写入时，硬件原子地更新 PTE 的 A/D 位，访问继续进行，不抛出异常。
2. **软件更新方案（Svade 行为）**：当 A=0 被访问或 D=0 被写入时，硬件抛出 page-fault 异常，由软件 trap handler 显式置位后重试。

Svadu 扩展是上述两种方案的**统一控制层**：

- 实现 Svadu 后，`menvcfg.ADUE`（bit 61）字段为可写位（WARL）。
- 当 `menvcfg.ADUE=1` 时，处于"硬件更新"模式：S/U-mode 下访问 A=0 / D=0 的 PTE，硬件原子地更新对应位，访问成功完成。
- 当 `menvcfg.ADUE=0` 时，处于"Svade 模式"：硬件不更新 A/D，访问触发 page-fault，与独立的 Svade 扩展行为完全一致。

实现 Svadu 扩展的处理器，在虚拟地址翻译过程的步骤 9（参见 `SPEC/supervisor.adoc:1758`，"If the Svade extension is implemented, stop and raise a page-fault exception corresponding to the original access type"）会根据 `menvcfg.ADUE` 选择两条路径（`SPEC/machine.adoc:2236–2245`）：
- ADUE=0：行为等同于 Svade —— 检测 `pte.a=0` 或（store 且 `pte.d=0`）时停止翻译并抛出对应原始访问类型的 page-fault；
- ADUE=1：硬件原子地将 `pte.a` 置 1，对 store 同时将 `pte.d` 置 1，然后继续翻译，不抛出 page-fault。

本测试计划聚焦该扩展的 CSR 控制行为、ADUE=1 / ADUE=0 两种模式下的访问语义、不同页面粒度（4 KiB / 2 MiB / 1 GiB）下的覆盖，以及运行时动态切换 ADUE 后的行为变化。

---

## 测试范围

### 规范来源

| 文件 | 行/章节 | 内容 |
|------|---------|------|
| `SPEC/svadu.adoc` | 全文（1–15 行） | Svadu 扩展定义；`menvcfg.ADUE` / `henvcfg.ADUE` 可写性；ADUE=0 回退到 Svade 的语义 |
| `SPEC/machine.adoc` | 2170 行（`[[sec:menvcfg]]`）；2236–2247 行（ADUE 字段语义） | `menvcfg` CSR 字段编码（ADUE=bit 61）；`norm:menvcfg_adue_op` 描述 ADUE 控制 S-mode 翻译时的硬件 A/D 更新；`norm:menvcfg_adue_rdonly0` 规定"Svadu 未实现时 ADUE 是 read-only zero" |
| `SPEC/supervisor.adoc` | 1623 行起（Svade 概念）；1758 行（翻译算法步骤 9 的 page-fault 抛出点） | "If the Svade extension is implemented, stop and raise a page-fault exception corresponding to the original access type" — Svade/Svadu 共用的翻译终点 |
| `SPEC/hypervisor.adoc` | 2109–2111 行 | 修改 `menvcfg.PBMTE/ADUE` 后必须执行 `HFENCE.GVMA(x0,x0)` 同步 G/VS-stage 翻译；非 Hyp 平台对应 `SFENCE.VMA(x0,x0)` |
| `common/encoding.h` | 第 25–30 行 | 仓库内已定义 `CSR_MENVCFG = 0x30A`、`MENVCFG_ADUE = (1ULL << 61)` 宏，可直接使用 |

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svadu_hw_update_a_d_bits` | If the Svadu extension is implemented, the `menvcfg`.ADUE field is writable. | 如果实现了 Svadu 扩展，则 `menvcfg`.ADUE 字段是可写的。 |
| `norm:menvcfg_adue_rdonly0` | If Svadu is not implemented, ADUE is read-only zero. | 如果未实现 Svade 或 Ssade 扩展，ADUE 为只读零。 |
| `norm:menvcfg_adue_op` | If the Svadu extension is implemented, the ADUE bit controls whether hardware updating of PTE A/D bits is enabled for S-mode and G-stage address translations. When ADUE=1, hardware updating of PTE A/D bits is enabled during S-mode address translation, and the implementation behaves as though the Svade extension were not implemented for S-mode address translation. When the hypervisor extension is implemented, if ADUE=1, hardware updating of PTE A/D bits is enabled during G-stage address translation, and the implementation behaves as though the Svade extension were not implemented for G-stage address translation. When ADUE=0, the implementation behaves as though Svade were implemented for S-mode and G-stage address translation. | ADUE 位控制低特权模式是否可以使用硬件 A/D 位更新。 |
| `norm:Svadu_disabled_hw_update_falls_back_to_svade` | When hardware updating of A/D bits is disabled, the Svade extension, which mandates exceptions when A/D bits need be set, instead takes effect. | 当禁用 A/D 位的硬件更新时，Svade 扩展生效，该扩展要求在需要设置 A/D 位时触发异常。 |
| `norm:svade_access_ad_bit_clear` | The Svade extension: when a virtual page is accessed and the A bit is clear, or is written and the D bit is clear, a page-fault exception is raised. | Svade 扩展：当虚拟页面被访问且 A 位为 0，或被写入且 D 位为 0 时，触发页错误异常。 |
| `norm:menvcfg_adue_fence` | After changing `menvcfg`.ADUE, executing an SFENCE.VMA instruction with _rs1_=`x0` and _rs2_=`x0` suffices to synchronize address-translation caches with respect to the altered interpretation of page-table entries' A/D bits. | 修改 ADUE 后需要执行 SFENCE.VMA 以确保一致性。 |

### 不在测试范围内

- **Hypervisor 两级翻译场景**：`henvcfg.ADUE` 的可写性、VS-stage 与 G-stage 下的 Svadu 行为、HLV/HSV 指令交互（涉及 H 扩展，本测试平台不包含）。
- **Sv32 / Sv48 / Sv57 模式**：本计划仅覆盖 Sv39，与现有 svade/svnapot/svpbmt 测试计划保持一致。
- **多 hart 一致性**：规范要求所有 hart 必须采用相同的 PTE 更新方案，当前测试框架为单核环境。
- **PMP / Smepmp 与 Svadu 的交叉**：相关交互由 PMP 系列测试计划独立覆盖。

---

## 测试分组

> [!IMPORTANT]
> 共 7 个测试组，每组提供：规范依据、测试职责、测试用例表（ID/名称/描述/预期结果）、关键代码示例。所有测试均运行于 Sv39 模式。

---

### Group 1：menvcfg.ADUE 字段控制测试

**规范依据**：
- `norm:Svadu_hw_update_a_d_bits`：实现 Svadu 时 `menvcfg.ADUE` 必须可写

**测试职责**：在 M-mode 下验证 `menvcfg.ADUE`（bit 61）的可写性、读写一致性、对其他字段的不干扰，以及复位后的初值。这是 Svadu 实现性的最基本检查。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADU-CSR-01 | menvcfg.ADUE 可写 0→1 | M-mode 写入 ADUE=1，回读 menvcfg | 回读 bit 61 = 1 |
| SVADU-CSR-02 | menvcfg.ADUE 可写 1→0 | 先置 ADUE=1，再清 ADUE=0，回读 | 回读 bit 61 = 0 |
| SVADU-CSR-03 | ADUE 不影响其他字段 | 切换 ADUE 时记录 PBMTE/STCE/CBIE/FIOM 等字段 | 其他字段值在 ADUE 切换前后保持不变 |
| SVADU-CSR-04 | ADUE 复位行为检查 | 在测试套件入口（`main.c` 中遍历 `_test_table` 之前）保存 menvcfg 原始值；本测试读取该保存值的 bit 61 | 实现定义；记录值并打印（不强制为特定值），约束：保存逻辑必须在任何其他测试运行之前完成 |

```c
/* SVADU-CSR-01 示例：menvcfg.ADUE 可写性
 *
 * 直接使用 common/encoding.h 中已定义的：
 *   #define CSR_MENVCFG  0x30A
 *   #define MENVCFG_ADUE (1ULL << 61)
 *
 * 通过 csrr/csrs/csrc 内联汇编访问 menvcfg（CSR 编号 0x30A）。
 * 注意 menvcfg 仅 M-mode 可访问。
 */
static inline uintptr_t menvcfg_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x30A" : "=r"(v));
    return v;
}

static inline void menvcfg_set(uintptr_t mask) {
    asm volatile ("csrs 0x30A, %0" :: "r"(mask));
}

static inline void menvcfg_clear(uintptr_t mask) {
    asm volatile ("csrc 0x30A, %0" :: "r"(mask));
}

static inline void set_menvcfg_adue(int enable) {
    if (enable) menvcfg_set(MENVCFG_ADUE);
    else        menvcfg_clear(MENVCFG_ADUE);
    /* 规范要求（SPEC/hypervisor.adoc:2109）：修改 menvcfg.ADUE 后必须
     * 执行 SFENCE.VMA(x0,x0)（非 Hyp 平台）以同步 S-stage 翻译。 */
    asm volatile ("sfence.vma zero, zero" ::: "memory");
}

static inline int get_menvcfg_adue(void) {
    return (menvcfg_read() & MENVCFG_ADUE) ? 1 : 0;
}

TEST_REGISTER(test_svadu_csr_adue_writable_set);
bool test_svadu_csr_adue_writable_set(void) {
    TEST_BEGIN("SVADU-CSR-01: menvcfg.ADUE writable 0->1");

    /* 先清零，避免依赖复位值 */
    set_menvcfg_adue(0);
    TEST_ASSERT("ADUE cleared to 0 before write", get_menvcfg_adue() == 0);

    set_menvcfg_adue(1);
    TEST_ASSERT("ADUE reads back as 1 after write", get_menvcfg_adue() == 1);

    TEST_END();
}
```

> [!NOTE]
> 若测试平台未实现 Svadu，写入 ADUE=1 后回读应仍为 0（按 WARL 语义忽略无效写）。SVADU-CSR-01 即作为 Svadu 实现性的"硬"判定：返回 0 时整个 Group 2/3/4/5/7 的测试通过 `detect_svadu()` SKIP；Group 6（ADUE=0 回退）在 Svadu 未实现时也无意义（因为本就是默认行为），同样 SKIP。

---

### Group 2：ADUE=1 时 4 KiB 叶 PTE 的硬件 A 位更新

**规范依据**：
- `norm:Svadu_adue_enabled_hw_updates_a_on_access`：ADUE=1 时硬件原子地置 PTE.A=1
- `norm:Svadu_adue_no_pagefault_when_enabled`：ADUE=1 下 A=0 不再触发 page-fault

**测试职责**：在 4 KiB 叶 PTE 上验证 ADUE=1 时各类访问（load、instruction fetch）对 A=0 页面均不触发 page-fault，且回到 M-mode 后 PTE.A 已被硬件置为 1。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADU-A4K-01 | A=0 R 页 load | ADUE=1，4 KiB PTE：V=1, R=1, A=0, D=0，S-mode load | load 成功（result=0），PTE.A=1 |
| SVADU-A4K-02 | A=0 X 页 fetch | ADUE=1，4 KiB PTE：V=1, X=1, A=0，S-mode 跳转执行 | 取指执行成功，PTE.A=1 |
| SVADU-A4K-03 | A=0 RW 页 load | ADUE=1，4 KiB PTE：V=1, R=1, W=1, A=0, D=1，S-mode load | load 成功，PTE.A=1，PTE.D 维持 1（仅 load 不影响 D） |
| SVADU-A4K-04 | 多次 load 后 A 仅设置一次 | ADUE=1，A=0 页面执行 N 次 load | 每次 load 均成功，最终 PTE.A=1（再次 load 不触发额外副作用） |

```c
/* SVADU-A4K-01 示例：ADUE=1 时 A=0 load 成功且硬件置 A=1 */
TEST_REGISTER(test_svadu_a4k_load_updates_a);
bool test_svadu_a4k_load_updates_a(void) {
    TEST_BEGIN("SVADU-A4K-01: ADUE=1 load on A=0 PTE sets A bit");

    if (!detect_svadu()) TEST_SKIP("Svadu not implemented");
    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    setup_code_mapping(&ctx);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_D,  /* A=0, D=1 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("load should succeed under ADUE=1", result == 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A becomes 1 after access", (pte & PTE_A) != 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 3：ADUE=1 时 4 KiB 叶 PTE 的硬件 D 位更新

**规范依据**：
- `norm:Svadu_adue_enabled_hw_updates_d_on_store`：ADUE=1 时 store/AMO 后硬件置 PTE.D=1
- `norm:Svadu_adue_no_pagefault_when_enabled`：ADUE=1 下 D=0 store 不再触发 page-fault

**测试职责**：在 4 KiB 叶 PTE 上验证 ADUE=1 时 store 与 AMO 操作对 D=0 页面不触发 page-fault，且回到 M-mode 后 PTE.D 已被硬件置为 1。重点验证 A=0+D=0 的 store 单次访问可同时置位两个标志。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADU-D4K-01 | A=1 D=0 RW 页 store | ADUE=1，PTE：V=1, R=1, W=1, A=1, D=0，S-mode store | store 成功，PTE.D=1（A 维持 1） |
| SVADU-D4K-02 | A=0 D=0 RW 页 store | ADUE=1，PTE：V=1, R=1, W=1, A=0, D=0，S-mode store | store 成功，PTE.A=1 且 PTE.D=1（一次 store 同时置位） |
| SVADU-D4K-03 | A=1 D=0 RW 页 amoadd | ADUE=1，PTE：V=1, R=1, W=1, A=1, D=0，S-mode amoadd.w | AMO 成功，PTE.D=1 |
| SVADU-D4K-04 | A=1 D=1 RW 页 store（无副作用） | ADUE=1，PTE：A=1, D=1，S-mode store | store 成功，PTE.A、PTE.D 维持 1 |

> [!IMPORTANT]
> SVADU-D4K-02 验证一次 store 同时硬件置 A=1 与 D=1，是 Svadu 与"软件先置 A、再触发 D fault"两步法的关键差异。

```c
/* SVADU-D4K-02 示例：ADUE=1 时一次 store 同时置 A 与 D */
TEST_REGISTER(test_svadu_d4k_store_sets_both_a_and_d);
bool test_svadu_d4k_store_sets_both_a_and_d(void) {
    TEST_BEGIN("SVADU-D4K-02: ADUE=1 store on A=0 D=0 PTE sets both bits");

    if (!detect_svadu()) TEST_SKIP("Svadu not implemented");
    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    setup_code_mapping(&ctx);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W,  /* A=0, D=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("store should succeed under ADUE=1", result == 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A set by HW", (pte & PTE_A) != 0);
    TEST_ASSERT("PTE.D set by HW", (pte & PTE_D) != 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 4：ADUE=1 时 2 MiB megapage 上的硬件 A/D 更新

**规范依据**：
- `norm:Svadu_adue_applies_all_levels`：硬件 A/D 更新在叶 PTE 上执行，2 MiB megapage 同样适用

**测试职责**：验证 ADUE=1 时硬件 A/D 更新行为在 2 MiB megapage（Sv39 中 level 1 叶 PTE）上同样生效，覆盖 load / store / fetch / AMO 四种访问类型。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADU-2M-01 | 2M A=0 load | 2 MiB 叶 PTE：V=1, R=1, A=0, D=0，S-mode load | load 成功，megapage PTE.A=1 |
| SVADU-2M-02 | 2M A=1 D=0 store | 2 MiB 叶 PTE：V=1, R=1, W=1, A=1, D=0，S-mode store | store 成功，megapage PTE.D=1 |
| SVADU-2M-03 | 2M A=0 X 页 fetch | 2 MiB 叶 PTE：V=1, X=1, A=0，S-mode 跳转执行 | 取指成功，megapage PTE.A=1 |
| SVADU-2M-04 | 2M A=0 D=0 store | 一次 store 同时置位 | store 成功，PTE.A=1 且 PTE.D=1 |
| SVADU-2M-05 | 2M A=1 D=0 amoadd.w | 2 MiB 叶 PTE：V=1, R=1, W=1, A=1, D=0，S-mode amoadd.w | AMO 成功，megapage PTE.D=1 |

```c
/* SVADU-2M-01 示例：ADUE=1 时 2 MiB megapage A=0 load 成功且置 A */
TEST_REGISTER(test_svadu_2m_load_updates_a);
bool test_svadu_2m_load_updates_a(void) {
    TEST_BEGIN("SVADU-2M-01: ADUE=1 2 MiB megapage A=0 load sets A bit");

    if (!detect_svadu()) TEST_SKIP("Svadu not implemented");
    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    setup_code_mapping(&ctx);

    uintptr_t test_va = test_region_2m_va;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R,  /* A=0, D=0 */
                PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("2 MiB load should succeed under ADUE=1", result == 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_2M);
    TEST_ASSERT("megapage PTE.A becomes 1", (pte & PTE_A) != 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 5：ADUE=1 时 1 GiB gigapage 上的硬件 A/D 更新

**规范依据**：
- `norm:Svadu_adue_applies_all_levels`：硬件 A/D 更新在叶 PTE 上执行，1 GiB gigapage 同样适用

**测试职责**：验证 ADUE=1 时硬件 A/D 更新行为在 1 GiB gigapage（Sv39 中 level 2 叶 PTE）上同样生效，覆盖 load / store / AMO 三种访问类型。1 GiB X 权限页面通常占据整个代码段，作为独立 fetch 用例不实用，故 fetch 测试由 SVADU-2M-03 与 SVADU-A4K-02 充分覆盖。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADU-1G-01 | 1G A=0 load | 1 GiB 叶 PTE：V=1, R=1, A=0, D=0，S-mode load | load 成功，gigapage PTE.A=1 |
| SVADU-1G-02 | 1G A=1 D=0 store | 1 GiB 叶 PTE：V=1, R=1, W=1, A=1, D=0，S-mode store | store 成功，gigapage PTE.D=1 |
| SVADU-1G-03 | 1G A=0 D=0 store | 一次 store 同时置位 A 与 D | store 成功，PTE.A=1 且 PTE.D=1 |
| SVADU-1G-04 | 1G A=1 D=0 amoadd.w | 1 GiB 叶 PTE：V=1, R=1, W=1, A=1, D=0，S-mode amoadd.w | AMO 成功，gigapage PTE.D=1 |

> [!NOTE]
> 1 GiB gigapage 测试需要确保测试 VA 选择在远离 M-mode 代码段与栈的恒等映射区域，避免与代码段所在的 1 GiB 区域冲突。

```c
/* SVADU-1G-01 示例：ADUE=1 时 1 GiB gigapage A=0 load 成功且置 A */
TEST_REGISTER(test_svadu_1g_load_updates_a);
bool test_svadu_1g_load_updates_a(void) {
    TEST_BEGIN("SVADU-1G-01: ADUE=1 1 GiB gigapage A=0 load sets A bit");

    if (!detect_svadu()) TEST_SKIP("Svadu not implemented");
    set_menvcfg_adue(1);

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* 代码段所在 1 GiB 区域：完整属性 */
    uintptr_t code_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_map_page(&ctx, code_base, code_base, code_flags, PT_LEVEL_1G);

    /* 测试用 1 GiB gigapage：与代码段不同的 1 GiB 区域，A=0 */
    uintptr_t test_va = code_base + PAGE_SIZE_1G;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R, PT_LEVEL_1G);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("1 GiB load should succeed under ADUE=1", result == 0);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_1G);
    TEST_ASSERT("gigapage PTE.A becomes 1", (pte & PTE_A) != 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 6：ADUE=0 时回退到 Svade 行为

**规范依据**：
- `norm:Svadu_disabled_hw_update_falls_back_to_svade`：当硬件 A/D 更新被禁用时，Svade 行为生效

**测试职责**：测试当 `menvcfg.ADUE=0` 时，Svadu 实现的处理器行为应与 Svade 完全一致——A=0 / D=0 触发 page-fault 且 PTE 内容保持不变。本组用例与 `docs/svade_test_plan.md` 的 Group 1/2/3 形成对照。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADU-FB-01 | ADUE=0 A=0 load | ADUE=0，4 KiB PTE：V=1, R=1, A=0，S-mode load | load page-fault（scause=13） |
| SVADU-FB-02 | ADUE=0 D=0 store | ADUE=0，4 KiB PTE：V=1, R=1, W=1, A=1, D=0，S-mode store | store page-fault（scause=15） |
| SVADU-FB-03 | ADUE=0 A=0 fetch | ADUE=0，4 KiB PTE：V=1, X=1, A=0，S-mode 跳转执行 | instruction page-fault（scause=12） |
| SVADU-FB-04 | ADUE=0 AMO D=0 | ADUE=0，4 KiB PTE：V=1, R=1, W=1, A=1, D=0，S-mode amoadd.w | store/AMO page-fault（scause=15） |
| SVADU-FB-05 | ADUE=0 fault 后 PTE 不变 | 触发上述任一 fault 后回到 M-mode 检查 PTE | 对应 A/D 位均保持原值（未被硬件更新） |

> [!IMPORTANT]
> Group 6 是 svade_test_plan.md 中 Group 1/2/3 的"ADUE=0 镜像"，验证 Svadu 实现关闭硬件更新后必须等价于 Svade。任何在 Svade 实现下能通过的测试，在 Svadu + ADUE=0 配置下也必须通过。

```c
/* SVADU-FB-01 示例：ADUE=0 时 A=0 load 触发 page-fault 且 PTE.A 不变 */
TEST_REGISTER(test_svadu_fb_a_clear_load_fault);
bool test_svadu_fb_a_clear_load_fault(void) {
    TEST_BEGIN("SVADU-FB-01: ADUE=0 fallback to Svade on A=0 load");

    if (!detect_svadu()) TEST_SKIP("Svadu not implemented");
    set_menvcfg_adue(0);  /* 显式关闭硬件更新，回退到 Svade */

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    setup_code_mapping(&ctx);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R,  /* A=0, D=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("ADUE=0 A=0 load triggers page-fault",
                result == CAUSE_LOAD_PAGE_FAULT);

    uintptr_t pte = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A remains 0 (no HW update)", (pte & PTE_A) == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 7：运行时动态切换 ADUE

**规范依据**：
- `norm:Svadu_runtime_switch_requires_sfence`（来源：`SPEC/hypervisor.adoc:2109–2111`）：修改 `menvcfg.ADUE` 后必须执行 `SFENCE.VMA(x0,x0)`（非 Hyp 平台），新设置才能保证生效

**测试职责**：验证在 M-mode 中运行时动态切换 `menvcfg.ADUE` 并执行 SFENCE.VMA 后，S-mode 访问行为应立即按新配置工作。覆盖 0→1、1→0 切换以及多次切换稳定性。

> [!IMPORTANT]
> 由于 `menvcfg` 仅 M-mode 可写，所有 ADUE 切换均在 M-mode 完成（不在 S-mode trap handler 中切换）。测试模式为：M-mode 设置 ADUE → `vm_run_in_smode` 进入 S-mode 访问 → 返回 M-mode 验证 → 切换 ADUE → 再次进入 S-mode 验证。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVADU-SW-01 | 0→1 切换：fault 后启用硬件更新 | M-mode 置 ADUE=0，S-mode 触发 A=0 load fault → 返回 M-mode 置 ADUE=1 + SFENCE.VMA → 同一 PTE 不变（PTE.A 仍 0），再次进入 S-mode 访问同一 VA | 第一次：scause=13；第二次：load 成功，PTE.A=1（被硬件置位） |
| SVADU-SW-02 | 1→0 切换：禁用硬件更新 | M-mode 置 ADUE=1，S-mode 访问 A=0 页 P1 完成硬件置位 → 返回 M-mode 置 ADUE=0 + SFENCE.VMA → 映射新的 A=0 页 P2 → S-mode 访问 P2 | P1 访问成功且 P1.A=1；P2 访问触发 scause=13 且 P2.A 仍 0 |
| SVADU-SW-03 | 切换后必须 SFENCE.VMA 才生效 | 置 ADUE=0 → 不执行 SFENCE.VMA → 立即置 ADUE=1 → 执行 SFENCE.VMA → S-mode 访问 A=0 页 | 访问成功，PTE.A=1（验证最终生效；本用例不强断言"未 fence 时 stale 行为"，仅验证最终一次 fence 后正确生效） |
| SVADU-SW-04 | 多次切换稳定性 | ADUE 在 0/1 之间切换 ≥4 次，每次切换 + SFENCE.VMA 后用全新 PTE 上下文访问 A=0 页 | 每次切换后行为严格符合当前 ADUE 值（ADUE=1 成功置位、ADUE=0 触发 fault） |

> [!NOTE]
> 关于"未执行 SFENCE.VMA 时 ADUE 修改是否立即可见"——`SPEC/hypervisor.adoc:2109` 明确要求执行同步指令。这意味着实现可以在未 fence 时返回旧行为；本测试计划采取**保守策略**，每次 ADUE 切换后均强制 SFENCE.VMA，不测试未 fence 的边界行为（避免测试与实现定义行为耦合）。

```c
/* SVADU-SW-01 示例：M-mode 切换 ADUE=0->1 后 S-mode 重试成功
 *
 * 注意 set_menvcfg_adue() 内部已包含 SFENCE.VMA(x0,x0)。
 */
TEST_REGISTER(test_svadu_sw_0_to_1_retry);
bool test_svadu_sw_0_to_1_retry(void) {
    TEST_BEGIN("SVADU-SW-01: ADUE 0->1 switch then retry succeeds");

    if (!detect_svadu()) TEST_SKIP("Svadu not implemented");

    set_menvcfg_adue(0);  /* 含 SFENCE.VMA */

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    setup_code_mapping(&ctx);

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R, PT_LEVEL_4K);  /* A=0, D=0 */

    /* 第一次访问：ADUE=0 触发 fault */
    uintptr_t r1 = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("first load faults under ADUE=0",
                r1 == CAUSE_LOAD_PAGE_FAULT);

    /* 验证 PTE.A 仍为 0（fault 时硬件未更新） */
    uintptr_t pte0 = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A still 0 after fault", (pte0 & PTE_A) == 0);

    /* M-mode 切换 ADUE=1（含 SFENCE.VMA） */
    set_menvcfg_adue(1);

    /* 第二次访问：ADUE=1 应成功，硬件置 PTE.A=1 */
    uintptr_t r2 = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("retry succeeds under ADUE=1", r2 == 0);

    uintptr_t pte1 = pte_read(&ctx, test_va, PT_LEVEL_4K);
    TEST_ASSERT("PTE.A set by HW after switch", (pte1 & PTE_A) != 0);

    pt_pool_reset();
    TEST_END();
}
```

---

## 附录：相关常量与 API 参考

### menvcfg.ADUE 字段

| 字段 | 位置 | 含义 |
|------|------|------|
| `ADUE` | menvcfg[61] | 启用硬件更新 PTE A/D 位（0=禁用回退到 Svade，1=启用硬件更新） |

### scause 常量

均定义于 `common/encoding.h`（已验证）。

| 常量 | 值 | 说明 |
|------|-----|------|
| `CAUSE_FETCH_PAGE_FAULT` | 12 | Instruction page fault |
| `CAUSE_LOAD_PAGE_FAULT`（别名 `CAUSE_LPF`，`encoding.h:185`） | 13 | Load page fault |
| `CAUSE_STORE_PAGE_FAULT`（别名 `CAUSE_SPF`，`encoding.h:186`） | 15 | Store/AMO page fault |

> [!NOTE]
> svade 的 `test_helpers.h` 中使用 `CAUSE_LPF` 缩写；本测试计划示例代码统一使用全名 `CAUSE_LOAD_PAGE_FAULT`，两者完全等价。

### menvcfg CSR 访问参考实现

仓库内当前没有通用 `csr_read/csr_write` 宏；`menvcfg`（CSR 0x30A）通过内联汇编访问。Svadu 测试 helpers 中应提供如下静态内联函数（参考 Group 1 代码示例）：

```c
/* 直接使用 common/encoding.h 中已有的宏：
 *   #define CSR_MENVCFG  0x30A
 *   #define MENVCFG_ADUE (1ULL << 61)
 *
 * 由于内联汇编要求 CSR 编号为立即数，下方使用字面量 0x30A 而非宏名。
 */
static inline uintptr_t menvcfg_read(void) {
    uintptr_t v;
    asm volatile ("csrr %0, 0x30A" : "=r"(v));
    return v;
}
static inline void menvcfg_set(uintptr_t mask) {
    asm volatile ("csrs 0x30A, %0" :: "r"(mask));
}
static inline void menvcfg_clear(uintptr_t mask) {
    asm volatile ("csrc 0x30A, %0" :: "r"(mask));
}
```

> [!NOTE]
> `common/hyp/hyp_csr.h` 已有同名 `menvcfg_read/menvcfg_write` 函数，但属于 Hypervisor 模块；svadu 测试不依赖 hyp 模块，应在 `svadu/tests/test_helpers.h` 内自行提供上述静态内联实现。

### 复用 svade 的辅助函数

svade 的 `tests/test_helpers.h` 中所有 helper 均为 `static` 函数（参见 `svade/tests/test_helpers.h` 文件头部注释：`All test files are #included into test_register.c`）。svadu 采用同样的"单 TU 内 #include"组织方式。

> [!IMPORTANT]
> **复用 svade helpers 的链接器约束**：svade 的 `setup_code_mapping()`、`test_data_area`、`test_fault_page`、`test_exec_page`、`test_region_2m_va` 等接口依赖以下两个由 `svade/kernel.ld` 提供的链接段符号：
> - `__vm_test_region_start` / `__vm_test_region_end`（3 × 4KB 页，2MB 对齐）
> - `__vm_test_region_2m_start` / `__vm_test_region_2m_end`（2MB 对齐的 2MB 区域）
>
> svadu 必须在 `svadu/kernel.ld` 中**完整复制**这两个段定义（参见 `svade/kernel.ld:67–96`），否则跨目录 `#include` svade 的 helpers 将出现 `undefined symbol` 链接错误。
>
> 推荐组织方式（二选一，本计划首选第 1 种）：
> 1. **复制 svade helpers 到 `svadu/tests/test_helpers.h`**：将 svade helpers 内容物理拷贝并扩展 ADUE 操作宏与 `detect_svadu`，保持与 svade 解耦；同时复制相同的 .vm_test_region* 段到 `svadu/kernel.ld`。
> 2. **跨目录 #include**：`svadu/tests/test_helpers.h` 顶部 `#include "../../svade/tests/test_helpers.h"`，并在 `svadu/kernel.ld` 中复制 .vm_test_region* 段。

本文档下表所列均基于"以 `static` 内联可见的方式复用"的前提（无论物理拷贝还是跨目录 include，对调用方均一致）：

| 函数/宏 | 来源 | 说明 |
|---------|------|------|
| `pt_context_t` / `pt_init` / `pt_pool_reset` / `pt_destroy` | `common/vm/vm.h` | 页表上下文管理 |
| `pt_map_page(ctx, va, pa, flags, level)` | `common/vm/vm.h` | 单页映射，level 取 `PT_LEVEL_4K`/`PT_LEVEL_2M`/`PT_LEVEL_1G` |
| `pt_setup_identity_mapping(ctx, base, size, flags, level)` | `common/vm/vm.h` | 区间恒等映射 |
| `pt_get_pte(ctx, va, level)` | `common/vm/vm.h` | 返回指定层级 PTE 的指针，未映射时返回 NULL |
| `vm_run_in_smode(ctx, fn, arg)` | `common/vm/vm.h` | 以指定页表上下文进入 S-mode 执行 `fn(arg)`，返回 `fn` 的返回值。在 svade helpers 提供的 `test_smode_load/store/exec/amoadd` 等 fn 中已封装 `trap_expect_begin/end` 与 `trap_get_cause()`，故"无异常返回 0、有异常返回 scause"是 helpers 的语义而非 `vm_run_in_smode` 自身的语义。 |
| `vm_sfence_vma(vaddr, asid)` | `common/vm/vm.h` | 执行 SFENCE.VMA，0/0 表示全局刷新 |
| `setup_code_mapping(ctx)` | `svade/tests/test_helpers.h:55` | 建立 M-mode 代码段、栈、UART、跳过 .vm_test_region* 的 2M 恒等映射 |
| `pte_read(ctx, va, level)` | `svade/tests/test_helpers.h:225`（封装 `pt_get_pte`） | 读取指定层级 PTE 值；未映射时返回 0 |
| `pte_set_bits(ctx, va, level, bits)` | `svade/tests/test_helpers.h:234` | OR 设置 PTE 位并 SFENCE.VMA |
| `test_smode_load/store/exec/load_and_store/amoadd` | `svade/tests/test_helpers.h:106–195` | 在 S-mode 中执行对应访问，无异常返回 0、有异常返回 scause |
| `test_data_area` / `test_fault_page` / `test_exec_page` / `test_region_2m_va` | `svade/tests/test_helpers.h:34–42` | `.vm_test_region` 内的 4K/2M 测试区域指针 |
| `init_exec_page()` | `svade/tests/test_helpers.h:50` | 在 test_exec_page 填充 `nop;ret`，供 SVADU-A4K-02 / 2M-03 使用 |
| `TEST_REGISTER` / `TEST_BEGIN` / `TEST_ASSERT` / `TEST_END` / `TEST_SKIP` | `common/test_framework.h` | 测试用例注册、断言、生命周期宏 |
| `CAUSE_LPF` / `CAUSE_LOAD_PAGE_FAULT` / `CAUSE_STORE_PAGE_FAULT` / `CAUSE_FETCH_PAGE_FAULT` | `common/encoding.h` | scause 常量（svade helpers 使用 `CAUSE_LPF` 缩写） |

### Svadu 专用辅助（在 `svadu/tests/test_helpers.h` 中新增）

| 函数 | 实现要点 |
|------|----------|
| `menvcfg_read/menvcfg_set/menvcfg_clear` | 见上方"menvcfg CSR 访问参考实现" |
| `set_menvcfg_adue(int enable)` | `menvcfg_set(MENVCFG_ADUE)` 或 `menvcfg_clear(MENVCFG_ADUE)` 后调用 `vm_sfence_vma(0,0)`（满足 `SPEC/hypervisor.adoc:2109`） |
| `get_menvcfg_adue()` | `(menvcfg_read() & MENVCFG_ADUE) ? 1 : 0` |
| `detect_svadu()` | 两步检测，参见下方完整示例 |

`detect_svadu()` 完整参考实现（仿照 `svade/tests/test_helpers.h:208–227` 的 `detect_svade()`，但取相反的判定逻辑）：

```c
static bool svadu_detected = false;
static bool svadu_detection_done = false;

static bool detect_svadu(void) {
    if (svadu_detection_done)
        return svadu_detected;

    /* 步骤 1：写 menvcfg.ADUE=1 后回读，判定 WARL 是否接受 */
    set_menvcfg_adue(1);
    if (get_menvcfg_adue() == 0) {
        svadu_detected = false;
        svadu_detection_done = true;
        return false;
    }

    /* 步骤 2：构造 A=0 的 4K 叶 PTE，在 S-mode 执行 load。
     *   - ADUE=1 + Svadu 实现：load 应成功（result == 0），且 PTE.A 被硬件置 1
     *   - 任何其他情况（ADUE 写入被吞掉、step 1 假阳性等）：load 触发 CAUSE_LPF
     */
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    if (setup_code_mapping(&ctx) != 0) {
        svadu_detected = false;
        svadu_detection_done = true;
        pt_pool_reset();
        return false;
    }

    uintptr_t test_va = (uintptr_t)test_fault_page;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_D, PT_LEVEL_4K);  /* A=0 */

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    uintptr_t pte    = pte_read(&ctx, test_va, PT_LEVEL_4K);
    pt_pool_reset();

    svadu_detected = (result == 0) && ((pte & PTE_A) != 0);
    svadu_detection_done = true;
    return svadu_detected;
}
```

### PTE flag 宏

- `PTE_V`、`PTE_R`、`PTE_W`、`PTE_X`、`PTE_U`、`PTE_G`、`PTE_A`、`PTE_D`

---

## 测试执行说明

- 所有测试用例运行于 Sv39 模式（`SATP_MODE_SV39`）
- 测试代码计划放在新建的 `svadu/` 目录（与 `svade/` 平行），结构与 `svade/` 一致：

  ```
  svadu/
  ├── Makefile             # SPIKE_ISA_EXT = _svadu，ENABLE_VM = 1
  ├── kernel.ld            # 复用 svade 的链接脚本结构
  ├── main.c               # 测试入口：banner + 遍历 _test_table
  └── tests/
      ├── test_helpers.h        # 物理拷贝 svade test_helpers.h 内容（推荐方式）+ 扩展 menvcfg/ADUE 操作宏与 detect_svadu；详见附录"复用 svade 的辅助函数"段落
      ├── test_register.c       # #include 所有测试文件
      ├── test_csr_adue.c       # Group 1：menvcfg.ADUE 控制
      ├── test_a_bit_4k.c       # Group 2：4K 硬件 A 位更新
      ├── test_d_bit_4k.c       # Group 3：4K 硬件 D 位更新
      ├── test_2m_megapage.c    # Group 4：2 MiB megapage
      ├── test_1g_gigapage.c    # Group 5：1 GiB gigapage
      ├── test_fallback_svade.c # Group 6：ADUE=0 回退
      └── test_runtime_switch.c # Group 7：运行时切换
  ```

- **Svadu 检测逻辑**（与附录 `detect_svadu()` 完整实现保持一致的两步检测）：
  - **步骤 1**：M-mode 写 `menvcfg.ADUE=1` 后回读 bit 61。规范 `norm:menvcfg_adue_rdonly0`（`SPEC/machine.adoc:2247`）规定 Svadu 未实现时 ADUE 为 read-only zero，因此回读 0 即可判定未实现。
  - **步骤 2**：构造 A=0 的 4K 叶 PTE，在 S-mode 执行 load。若返回 0 且 PTE.A 被硬件置位，则确认 Svadu 已实现且 ADUE=1 行为正确。
  - **跳过策略**：检测失败时，依赖 Svadu 行为的所有测试组（Group 2/3/4/5/7）以及验证回退语义的 Group 6 全部 `TEST_SKIP`。Group 1 的 SVADU-CSR-01 / 02 / 03 自身即为可写性检测，不依赖 `detect_svadu()` 结果（其失败本身就是平台无 Svadu 的证据）；SVADU-CSR-04 仅打印复位值。
- **trap handler 要求**：S-mode trap handler 需要捕获 page-fault 并将 scause 通过 `vm_run_in_smode` 的返回值传递回 M-mode（与 svade 共用 trap 框架）。
- **模拟器要求**：
  - **Spike**：`SPIKE_ISA_EXT = _svadu`，启动命令需带 `--isa=rv64imac_zicsr_zifencei_svadu`
  - **QEMU**：使用 `-cpu max` 或显式启用 svadu 扩展（取决于 QEMU 版本对 Svadu 的支持情况）
  - **Sail**：需 Sail RISC-V 模型支持 Svadu 扩展（参考 `riscv-isa-sim` 实现）
- **顶层 Makefile 注册**：在仓库根 `Makefile` 的 `EXTENSIONS` 列表中添加 `svadu`。
