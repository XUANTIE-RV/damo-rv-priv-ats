**中文 | [English](../testplan_en/Svpbmt_test_plan_en.md)**

# Svpbmt 扩展测试计划

本文档描述 Svpbmt（Page-Based Memory Types）扩展的测试计划。测试基于 RISC-V 特权级规范中的 Svpbmt 扩展定义（`SPEC/svpbmt.adoc`），覆盖 PBMT 编码验证、保留值异常、非叶 PTE 检查、内存排序语义、别名一致性等规范要求。

---

## 概述

Svpbmt 扩展在 Sv39、Sv48 和 Sv57 的叶页表条目（leaf PTE）中使用 bits 62-61（PBMT 字段）来覆盖页面的物理内存属性（PMA）。PBMT 字段的编码如下：

| Mode | Value | 请求的内存属性 |
|------|-------|---------------|
| PMA  | 0     | 无覆盖，使用底层 PMA |
| NC   | 1     | 非缓存、幂等、弱排序（RVWMO）、主内存 |
| IO   | 2     | 非缓存、非幂等、强排序（I/O ordering）、I/O |
| —    | 3     | 保留，触发 page-fault |

**依赖关系**：Svpbmt 扩展依赖 Sv39 扩展（`norm:Svpbmt_depends_Sv39`）。

---

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svpbmt_depends_Sv39` | The Svpbmt extension depends on the Sv39 extension. | Svpbmt 扩展依赖于 Sv39 扩展。 |
| `norm:Svpbmt_impl_may_override_pmas` | Implementations may override additional PMAs not explicitly listed in <<pbmt>>. | 实现可以覆盖 <<pbmt>> 中未明确列出的额外 PMA。 |
| `norm:Svpbmt_nonleaf_pte_pbmt_must_be_zero` | Until their use is defined by a standard extension, they must be cleared by software for forward compatibility, or else a page-fault exception is raised. | 在其用途被标准扩展定义之前，软件必须将其清零以保证向前兼容性，否则会触发页错误异常。 |
| `norm:Svpbmt_leaf_pte_pbmt_reserved_3_fault` | Until this value is defined by a standard extension, using this reserved value in a leaf PTE raises a page-fault exception. | 在此值被标准扩展定义之前，在叶 PTE 中使用此保留值会触发页错误异常。 |
| `norm:Svpbmt_obeys_mem_ordering` | memory accesses to such pages obey the memory ordering rules of the final effective attribute. | 对此类页面的内存访问遵循最终有效属性的内存排序规则。 |
| `norm:Svpbmt_io_pma_nc_pbmt_obey_rvwmo` | If the underlying physical memory attribute for a page is I/O, and the page has PBMT=NC, then accesses to that page obey RVWMO. | 如果页面的底层物理内存属性是 I/O，且页面具有 PBMT=NC，则对该页面的访问遵循 RVWMO。 |
| `norm:Svpbmt_io_pma_nc_pbmt_treated_as_io_and_memory` | accesses to such pages are considered to be _both_ I/O and main memory accesses for the purposes of FENCE, _.aq_, and _.rl_. | 出于 FENCE、_.aq_ 和 _.rl_ 的目的，对此类页面的访问被视为同时是 I/O 和主存访问。 |
| `norm:Svpbmt_memory_pma_io_pbmt_strong_io_ordering` | accesses to that page obey strong channel 0 I/O ordering rules. | 对该页面的访问遵循强通道 0 I/O 排序规则。 |
| `norm:Svpbmt_memory_pma_io_pbmt_treated_as_io_and_memory` | accesses to such pages are considered to be _both_ I/O and main memory accesses for the purposes of FENCE, _.aq_, and _.rl_. | 出于 FENCE、_.aq_ 和 _.rl_ 的目的，对此类页面的访问被视为同时是 I/O 和主存访问。 |
| `norm:Svpbmt_aliasing_attribute` | When Svpbmt is used with non-zero PBMT encodings, it is possible for multiple virtual aliases of the same physical page to exist simultaneously with different memory attributes. It is also possible for a U-mode or S-mode mapping through a PTE with Svpbmt enabled to observe different memory attributes for a given region of physical memory than a concurrent access to the same page performed by M-mode or when MODE=Bare. In such cases, the behaviors dictated by the attributes (including coherence, which is otherwise unaffected) may be violated. | 当 Svpbmt 与非零 PBMT 编码一起使用时，同一物理页面的多个虚拟别名可能同时存在并具有不同的内存属性。通过启用 Svpbmt 的 PTE 的 U 模式或 S 模式映射也可能观察到与 M 模式或 MODE=Bare 时并发访问同一页面不同的内存属性。在这种情况下，属性所规定的行为（包括一致性，否则不受影响）可能会被违反。 |
| `norm:Svpbmt_noncacheable_aliasing_no_coherence_loss` | Accessing the same location using different attributes that are both non-cacheable (e.g., NC and IO) does not cause loss of coherence. | 使用均为不可缓存的不同属性（如 NC 和 IO）访问同一位置不会导致一致性丢失。 |
| `norm:Svpbmt_noncacheable_aliasing_may_weaken_ordering` | might result in weaker memory ordering than the stricter attribute ordinarily guarantees. | 但可能导致比更严格的属性通常保证的更弱的内存排序。 |
| `norm:Svpbmt_noncacheable_aliasing_fence_prevents_ordering_loss` | `fence iorw, iorw` instruction between such accesses suffices to prevent loss of memory ordering. | 在此类访问之间执行 `fence iorw, iorw` 指令足以防止内存排序丢失。 |
| `norm:Svpbmt_cacheable_aliasing_may_cause_coherence_loss` | may cause loss of coherence. | 使用不同的可缓存性属性访问同一位置可能导致一致性丢失。 |
| `norm:Svpbmt_cacheable_aliasing_fence_flush_fence_required` | prevents both loss of coherence and loss of memory ordering: `fence iorw, iorw`, followed by `cbo.flush` to an address of that location, followed by a `fence iorw, iorw`. | 防止一致性和内存排序丢失的序列：`fence iorw, iorw`，后跟对该位置地址的 `cbo.flush`，再跟 `fence iorw, iorw`。 |
| `norm:Svpbmt_hgatp_stage_override_rule` | if `hgatp`.MODE is not equal to zero, non-zero G-stage PTE PBMT bits override the attributes in the PMA to produce an intermediate set of attributes. | 如果 `hgatp`.MODE 不等于零，非零的 G 阶段 PTE PBMT 位覆盖 PMA 中的属性以产生中间属性集。 |
| `norm:Svpbmt_vsatp_stage_override_rule` | if `vsatp`.MODE is not equal to zero, non-zero VS-stage PTE PBMT bits override the intermediate attributes to produce the final set of attributes used by accesses to the page in question. | 如果 `vsatp`.MODE 不等于零，非零的 VS 阶段 PTE PBMT 位覆盖中间属性以产生用于访问相关页面的最终属性集。 |

---

## 测试分组

### Group 1：PBMT 基本编码验证

**规范依据**：
- PTE bits 62-61 编码 PBMT 字段，叶 PTE 中有效值为 0（PMA）、1（NC）、2（IO）
- `norm:Svpbmt_depends_Sv39`：Svpbmt 依赖 Sv39 扩展
- `norm:Svpbmt_impl_may_override_pmas`：实现可以覆盖额外的未显式列出的 PMA

**测试职责**：验证 PBMT 三种有效编码（PMA、NC、IO）在叶 PTE 中的基本功能，确认设置这些值的页面可以正常访问。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PBMT-01 | PBMT=PMA load/store | 叶 PTE 设置 PBMT=0（PMA），执行 load/store | 正常访问，使用底层 PMA |
| PBMT-02 | PBMT=NC load/store | 叶 PTE 设置 PBMT=1（NC），对主内存区域执行 load/store | 正常访问，非缓存语义 |
| PBMT-03 | PBMT=IO load/store | 叶 PTE 设置 PBMT=2（IO），对主内存区域执行 load/store | 正常访问，强排序语义 |
| PBMT-04 | PBMT=PMA exec | 叶 PTE 设置 PBMT=0（PMA），执行指令获取 | 正常执行 |
| PBMT-05 | PBMT=NC exec | 叶 PTE 设置 PBMT=1（NC），执行指令获取 | 正常执行（非缓存语义） |
| PBMT-06 | PBMT=IO exec | 叶 PTE 设置 PBMT=2（IO），执行指令获取 | 实现定义（I/O 区域可能不支持指令获取） |

```c
/* PBMT-01 示例：PBMT=PMA load/store */
TEST_REGISTER(test_pbmt_pma_load_store);
bool test_pbmt_pma_load_store(void) {
    TEST_BEGIN("PBMT-01: PBMT=PMA (00) load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：PBMT=PMA (00, 默认) */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_PMA;  /* bits 62-61 = 00 */
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("PBMT=PMA load/store succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* PBMT-02 示例：PBMT=NC load/store */
TEST_REGISTER(test_pbmt_nc_load_store);
bool test_pbmt_nc_load_store(void) {
    TEST_BEGIN("PBMT-02: PBMT=NC (01) load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：PBMT=NC (01) */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_NC;  /* bits 62-61 = 01 */
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("PBMT=NC load/store succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* PBMT-03 示例：PBMT=IO load/store */
TEST_REGISTER(test_pbmt_io_load_store);
bool test_pbmt_io_load_store(void) {
    TEST_BEGIN("PBMT-03: PBMT=IO (10) load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：PBMT=IO (10)
     * 注意：对主内存区域设置 PBMT=IO，访问将遵循强 I/O 排序 */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_IO;  /* bits 62-61 = 10 */
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("PBMT=IO load/store succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 2：PBMT 保留值异常

**规范依据**：
- `norm:Svpbmt_leaf_pte_pbmt_reserved_3_fault`：叶 PTE 中 PBMT=3（bits 62-61 = 11）为保留值，触发 page-fault

**测试职责**：验证叶 PTE 中 PBMT 设置为保留值 3 时，任何类型的访问均触发 page-fault。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| RSVD-01 | PBMT=3 load fault | 叶 PTE 设置 PBMT=3，执行 load | load page-fault |
| RSVD-02 | PBMT=3 store fault | 叶 PTE 设置 PBMT=3，执行 store | store page-fault |
| RSVD-03 | PBMT=3 exec fault | 叶 PTE 设置 PBMT=3，执行指令获取 | instruction page-fault |
| RSVD-04 | PBMT=3 superpage fault | 2MB superpage 叶 PTE 设置 PBMT=3，执行 load | load page-fault |

```c
/* RSVD-01 示例：PBMT=3 (reserved) load triggers page-fault */
TEST_REGISTER(test_pbmt_reserved_load_fault);
bool test_pbmt_reserved_load_fault(void) {
    TEST_BEGIN("RSVD-01: PBMT=3 (reserved) load triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：PBMT=3 (reserved, bits 62-61 = 11) */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_RSVD;  /* bits 62-61 = 11 */
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("PBMT=3 triggers load page-fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

/* RSVD-02 示例：PBMT=3 (reserved) store triggers page-fault */
TEST_REGISTER(test_pbmt_reserved_store_fault);
bool test_pbmt_reserved_store_fault(void) {
    TEST_BEGIN("RSVD-02: PBMT=3 (reserved) store triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：PBMT=3 (reserved) */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_RSVD;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store_expect_fault,
                                        test_va);
    TEST_ASSERT("PBMT=3 triggers store page-fault", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 3：非叶 PTE PBMT 位检查

**规范依据**：
- `norm:Svpbmt_nonleaf_pte_pbmt_must_be_zero`：非叶 PTE 的 bits 62-61 保留，必须清零，否则触发 page-fault

**测试职责**：验证非叶 PTE（中间页表指针）中 PBMT 位非零时触发 page-fault。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| NLPTE-01 | 非叶 PTE PBMT=1 | 中间 PTE（非叶）设置 PBMT=NC（01），通过该 PTE 执行 load | load page-fault |
| NLPTE-02 | 非叶 PTE PBMT=2 | 中间 PTE（非叶）设置 PBMT=IO（10），通过该 PTE 执行 load | load page-fault |
| NLPTE-03 | 非叶 PTE PBMT=3 | 中间 PTE（非叶）设置 PBMT=3（11），通过该 PTE 执行 load | load page-fault |
| NLPTE-04 | 非叶 PTE PBMT=0 正常 | 中间 PTE（非叶）PBMT=0（合规），通过该 PTE 执行 load | 正常访问 |
| NLPTE-05 | 多级非叶 PTE PBMT 检查 | 不同层级的非叶 PTE 分别设置 PBMT 非零 | 每次均触发 page-fault |

```c
/* NLPTE-01 示例：非叶 PTE PBMT=NC 触发 page-fault */
TEST_REGISTER(test_nonleaf_pbmt_nc_fault);
bool test_nonleaf_pbmt_nc_fault(void) {
    TEST_BEGIN("NLPTE-01: Non-leaf PTE PBMT=NC triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面使用 4KB 页（需要中间非叶 PTE） */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 找到 test_va 对应的非叶 PTE，手动设置 PBMT=NC。
     * 注意：PT_LEVEL_2M 在 4KB 映射场景下对应 level 1 的非叶 PTE，
     * 该 PTE 指向 level 0 页表页（非叶），而非 2MB superpage 叶 PTE。 */
    uintptr_t *nonleaf_pte = pt_get_pte(&ctx, test_va, PT_LEVEL_2M);
    *nonleaf_pte |= PBMT_NC;  /* 设置 bits 62-61 = 01 */

    /* 执行 sfence.vma 刷新 TLB */
    vm_sfence_vma(0, 0);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("Non-leaf PTE PBMT=NC triggers page-fault",
                result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 4：PBMT 与页面权限交互

**规范依据**：
- PBMT 设置不影响 PTE 的 R/W/X 权限检查
- PBMT 仅覆盖内存属性，权限检查按照标准页表遍历算法独立进行
- `norm:Svpbmt_impl_may_override_pmas`：实现可能基于 PBMT 覆盖额外的 PMA 约束（如对 PBMT=IO 页面的非对齐访问可能触发异常）

**测试职责**：验证 PBMT 属性不影响 RWX 权限语义，以及 PBMT 与 U 位的交互。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PERM-01 | PBMT=NC + R-only | 叶 PTE 设置 PBMT=NC，仅 R 权限，执行 store | store page-fault |
| PERM-02 | PBMT=IO + R-only | 叶 PTE 设置 PBMT=IO，仅 R 权限，执行 store | store page-fault |
| PERM-03 | PBMT=NC + no-exec | 叶 PTE 设置 PBMT=NC，无 X 权限，执行指令获取 | instruction page-fault |
| PERM-04 | PBMT=IO + no-exec | 叶 PTE 设置 PBMT=IO，无 X 权限，执行指令获取 | instruction page-fault |
| PERM-05 | PBMT=NC + U-bit S-mode | 叶 PTE 设置 PBMT=NC 和 U=1，S-mode（SUM=0）访问 | page-fault |
| PERM-06 | PBMT=NC + U-bit SUM=1 | 叶 PTE 设置 PBMT=NC 和 U=1，S-mode（SUM=1）访问 | 正常访问 |
| ALIGN-01 | PBMT=IO 非对齐 load | 对 PBMT=IO 页面执行非对齐 load（如从 addr+1 读取 8 字节） | 实现定义：可能触发 load access-fault 或正常访问 |
| ALIGN-02 | PBMT=IO 非对齐 store | 对 PBMT=IO 页面执行非对齐 store（如向 addr+1 写入 8 字节） | 实现定义：可能触发 store access-fault 或正常访问 |
| ALIGN-03 | PBMT=PMA 非对齐 load（对照） | 对 PBMT=PMA 页面执行相同的非对齐 load | 正常访问（对照基线） |

```c
/* PERM-01 示例：PBMT=NC + R-only，store 触发 page-fault */
TEST_REGISTER(test_pbmt_nc_readonly_store_fault);
bool test_pbmt_nc_readonly_store_fault(void) {
    TEST_BEGIN("PERM-01: PBMT=NC + R-only, store triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：PBMT=NC, R-only (无 W 权限) */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_A | PTE_D
                          | PBMT_NC;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store_expect_fault,
                                        test_va);
    TEST_ASSERT("PBMT=NC + R-only: store faults", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}

/* PERM-02 示例：PBMT=IO + R-only，store 触发 page-fault */
TEST_REGISTER(test_pbmt_io_readonly_store_fault);
bool test_pbmt_io_readonly_store_fault(void) {
    TEST_BEGIN("PERM-02: PBMT=IO + R-only, store triggers page-fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：PBMT=IO, R-only */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_A | PTE_D
                          | PBMT_IO;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store_expect_fault,
                                        test_va);
    TEST_ASSERT("PBMT=IO + R-only: store faults", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 5：PBMT 与不同页大小

**规范依据**：
- PBMT 定义在叶 PTE 的 bits 62-61，适用于所有叶 PTE 级别
- Sv39 支持 4KB、2MB、1GB 页；Sv48 额外支持 512GB；Sv57 额外支持 256TB
- `norm:Svpbmt_depends_Sv39`：Svpbmt 依赖 Sv39 扩展

**测试职责**：验证 PBMT 属性在 4KB 页、2MB megapage 和 1GB gigapage 上均正常工作。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PGSZ-01 | 4KB 页 + PBMT=NC | 4KB 叶 PTE 设置 PBMT=NC，load/store | 正常访问 |
| PGSZ-02 | 4KB 页 + PBMT=IO | 4KB 叶 PTE 设置 PBMT=IO，load/store | 正常访问 |
| PGSZ-03 | 2MB megapage + PBMT=NC | 2MB superpage 叶 PTE 设置 PBMT=NC，load/store | 正常访问 |
| PGSZ-04 | 2MB megapage + PBMT=IO | 2MB superpage 叶 PTE 设置 PBMT=IO，load/store | 正常访问 |
| PGSZ-05 | 1GB gigapage + PBMT=NC | 1GB superpage 叶 PTE 设置 PBMT=NC，load/store | 正常访问 |
| PGSZ-06 | 1GB gigapage + PBMT=IO | 1GB superpage 叶 PTE 设置 PBMT=IO，load/store | 正常访问 |

```c
/* PGSZ-03 示例：2MB megapage + PBMT=NC */
TEST_REGISTER(test_pbmt_nc_2mb_megapage);
bool test_pbmt_nc_2mb_megapage(void) {
    TEST_BEGIN("PGSZ-03: 2MB megapage + PBMT=NC load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射 2MB megapage 测试区域：PBMT=NC */
    uintptr_t test_va = TEST_REGION_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_NC;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("2MB megapage + PBMT=NC works", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* PGSZ-05 示例：1GB gigapage + PBMT=NC */
TEST_REGISTER(test_pbmt_nc_1gb_gigapage);
bool test_pbmt_nc_1gb_gigapage(void) {
    TEST_BEGIN("PGSZ-05: 1GB gigapage + PBMT=NC load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* 使用带 PBMT=NC 的 1GB gigapage 作为恒等映射 */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D
                          | PBMT_NC;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              pte_flags, PT_LEVEL_1G);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("1GB gigapage + PBMT=NC works", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 6：内存排序语义

**规范依据**：
- `norm:Svpbmt_obeys_mem_ordering`：当 PBMT 覆盖内存属性时，访问遵循最终有效属性的排序规则
- `norm:Svpbmt_io_pma_nc_pbmt_obey_rvwmo`：I/O PMA + PBMT=NC 时遵循 RVWMO
- `norm:Svpbmt_io_pma_nc_pbmt_treated_as_io_and_memory`：I/O PMA + PBMT=NC 时被视为同时是 I/O 和主内存访问
- `norm:Svpbmt_memory_pma_io_pbmt_strong_io_ordering`：主内存 PMA + PBMT=IO 时遵循强 I/O 排序
- `norm:Svpbmt_memory_pma_io_pbmt_treated_as_io_and_memory`：主内存 PMA + PBMT=IO 时被视为同时是 I/O 和主内存访问

**测试职责**：验证 PBMT 覆盖内存属性后的排序语义。

> **注意**：内存排序测试在单 hart 环境下难以完全验证排序强度差异。以下测试主要验证 PBMT 覆盖后的访问不会导致异常或不正确的数据，以及 FENCE 指令对 PBMT 覆盖页面的作用。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ORDER-01 | 主内存 + PBMT=IO 访问正确 | 主内存区域设置 PBMT=IO，执行多次顺序 load/store | 数据正确（强 I/O 排序） |
| ORDER-02 | 主内存 + PBMT=IO + FENCE | 主内存区域设置 PBMT=IO，在 load/store 之间插入 `fence iorw, iorw` | 数据正确 |
| ORDER-03 | 主内存 + PBMT=NC 访问正确 | 主内存区域设置 PBMT=NC，执行多次顺序 load/store | 数据正确（RVWMO） |
| ORDER-04 | 主内存 + PBMT=NC + FENCE | 主内存区域设置 PBMT=NC，在 load/store 之间插入 `fence rw, rw` | 数据正确 |
| ORDER-05 | I/O 区域 + PBMT=NC 访问（discouraged） | I/O 区域（如 UART）设置 PBMT=NC，验证 RVWMO 排序生效。**注意：规范不鼓励此配置**，I/O 设备驱动依赖强排序规则时将无法正确工作 | 正常访问，但排序弱于 PBMT=IO |
| ORDER-06 | PBMT=IO 页 FENCE 覆盖性 | 主内存 + PBMT=IO 页面上执行 `fence iorw, iorw`，验证该页面访问对 FENCE 有效 | FENCE 对 I/O 和主内存访问均有效 |

```c
/* ORDER-01 示例：主内存 + PBMT=IO 多次顺序 load/store */
TEST_REGISTER(test_pbmt_io_sequential_access);
bool test_pbmt_io_sequential_access(void) {
    TEST_BEGIN("ORDER-01: Memory + PBMT=IO sequential load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：PBMT=IO */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_IO;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    /* S-mode 函数：多次顺序 store 后 load 回验证 */
    uintptr_t result = vm_run_in_smode(&ctx,
                                        test_smode_sequential_rw,
                                        test_va);
    TEST_ASSERT("PBMT=IO sequential access data correct", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ORDER-02 示例：主内存 + PBMT=IO + FENCE */
TEST_REGISTER(test_pbmt_io_fence);
bool test_pbmt_io_fence(void) {
    TEST_BEGIN("ORDER-02: Memory + PBMT=IO with fence iorw,iorw");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_IO;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    /* S-mode 函数：store, fence iorw,iorw, load 回验证 */
    uintptr_t result = vm_run_in_smode(&ctx,
                                        test_smode_store_fence_load,
                                        test_va);
    TEST_ASSERT("PBMT=IO with fence data correct", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 7：别名与一致性

**规范依据**：
- `norm:Svpbmt_aliasing_attribute`：不同虚拟别名可以有不同的内存属性，此时属性行为（包括一致性）可能被违反
- `norm:Svpbmt_noncacheable_aliasing_no_coherence_loss`：两个非缓存属性的别名不会导致一致性丧失
- `norm:Svpbmt_noncacheable_aliasing_may_weaken_ordering`：两个非缓存属性的别名可能削弱排序保证
- `norm:Svpbmt_noncacheable_aliasing_fence_prevents_ordering_loss`：`fence iorw, iorw` 可防止非缓存别名间的排序丧失
- `norm:Svpbmt_cacheable_aliasing_may_cause_coherence_loss`：不同缓存属性的别名可能导致一致性丧失
- `norm:Svpbmt_cacheable_aliasing_fence_flush_fence_required`：`fence iorw, iorw` + `cbo.flush` + `fence iorw, iorw` 序列可防止缓存别名的一致性和排序丧失

**测试职责**：验证不同 PBMT 属性的虚拟别名行为，以及规范要求的 fence/flush 序列的有效性。

> **注意**：别名测试需要将同一物理页面映射到两个不同虚拟地址，分别设置不同的 PBMT 属性。在单 hart 测试环境下，一致性问题可能不会暴露，以下测试主要验证 fence/flush 序列的正确执行（不触发异常）以及基本数据可见性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ALIAS-01 | NC + IO 非缓存别名 | 同一物理页映射到 VA1（PBMT=NC）和 VA2（PBMT=IO），通过 VA1 写入后通过 VA2 读取 | 数据一致（非缓存别名不丢失一致性） |
| ALIAS-02 | NC + IO 别名 + fence | 同一物理页 NC/IO 别名，在写入和读取之间插入 `fence iorw, iorw` | 数据一致，排序保证 |
| ALIAS-03 | PMA + NC 缓存属性别名 | 同一物理页映射到 VA1（PBMT=PMA）和 VA2（PBMT=NC），通过 VA1 写入后通过 VA2 读取 | 行为不确定（可能一致性丧失） |
| ALIAS-04 | PMA + NC 别名 + fence+flush+fence | 同上，但在写入和读取之间插入 `fence iorw,iorw` + `cbo.flush` + `fence iorw,iorw` | 数据一致 |
| ALIAS-05 | PMA + IO 缓存属性别名 | 同一物理页映射到 VA1（PBMT=PMA）和 VA2（PBMT=IO），通过 VA1 写入后通过 VA2 读取 | 行为不确定（可能一致性丧失） |
| ALIAS-06 | PMA + IO 别名 + fence+flush+fence | 同上，但在写入和读取之间插入完整的 fence+flush+fence 序列 | 数据一致 |
| ALIAS-07 | M-mode PMA vs S-mode PBMT=NC | M-mode 以 PMA 属性直接写入物理页，S-mode 以 PBMT=NC 映射同一页并读取。需手动管理特权级切换，不使用 `vm_run_in_smode()` | 行为不确定（规范允许属性行为违反，包括一致性） |

```c
/* ALIAS-01 示例：NC + IO 非缓存别名 */
TEST_REGISTER(test_pbmt_alias_nc_io);
bool test_pbmt_alias_nc_io(void) {
    TEST_BEGIN("ALIAS-01: NC + IO non-cacheable alias coherence");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 同一物理页，两个不同虚拟地址，不同 PBMT */
    uintptr_t phys_page = TEST_REGION_BASE;
    uintptr_t va_nc = TEST_REGION_BASE;
    uintptr_t va_io = TEST_REGION_BASE + PAGE_SIZE_2M;  /* 不同 VA */

    /* VA1: PBMT=NC */
    pt_map_page(&ctx, va_nc, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_NC,
                PT_LEVEL_4K);

    /* VA2: PBMT=IO，映射到同一物理页 */
    pt_map_page(&ctx, va_io, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_IO,
                PT_LEVEL_4K);

    /* S-mode 函数：通过 va_nc 写入，通过 va_io 读取 */
    uintptr_t args[2] = { va_nc, va_io };
    uintptr_t result = vm_run_in_smode(&ctx,
                                        test_smode_alias_write_read,
                                        (uintptr_t)args);
    TEST_ASSERT("NC+IO alias: data coherent", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* ALIAS-04 示例：PMA + NC 缓存属性别名 + fence+flush+fence */
TEST_REGISTER(test_pbmt_alias_pma_nc_flush);
bool test_pbmt_alias_pma_nc_flush(void) {
    TEST_BEGIN("ALIAS-04: PMA + NC alias with fence+cbo.flush+fence");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    uintptr_t phys_page = TEST_REGION_BASE;
    uintptr_t va_pma = TEST_REGION_BASE;
    uintptr_t va_nc = TEST_REGION_BASE + PAGE_SIZE_2M;

    /* VA1: PBMT=PMA (默认，可缓存) */
    pt_map_page(&ctx, va_pma, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_PMA,
                PT_LEVEL_4K);

    /* VA2: PBMT=NC (非缓存) */
    pt_map_page(&ctx, va_nc, phys_page,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_NC,
                PT_LEVEL_4K);

    /* S-mode 函数：
     * 1. 通过 va_pma 写入
     * 2. fence iorw, iorw
     * 3. cbo.flush va_pma
     * 4. fence iorw, iorw
     * 5. 通过 va_nc 读取并验证 */
    uintptr_t args[2] = { va_pma, va_nc };
    uintptr_t result = vm_run_in_smode(&ctx,
                                        test_smode_alias_flush_sequence,
                                        (uintptr_t)args);
    TEST_ASSERT("PMA+NC alias with flush sequence: data coherent",
                result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 8：SFENCE.VMA 与 PBMT

**规范依据**：
- 修改 PTE 的 PBMT 字段后，必须执行 SFENCE.VMA 来刷新 TLB 缓存的翻译条目
- TLB 条目可能缓存了旧的 PBMT 属性，导致行为不一致

**测试职责**：验证 PBMT 属性变更后 SFENCE.VMA 的刷新效果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SFENCE-01 | PBMT 变更后全局刷新 | 将 PBMT 从 PMA 改为 NC，执行全局 sfence.vma，验证新属性生效 | 新属性生效，正常访问 |
| SFENCE-02 | PBMT 变更后按地址刷新 | 将 PBMT 从 PMA 改为 IO，按地址执行 sfence.vma，验证新属性生效 | 新属性生效，正常访问 |
| SFENCE-03 | PBMT 改为保留值后刷新 | 将 PBMT 从 NC 改为 3（保留），执行 sfence.vma 后访问。验证 TLB 刷新后正确反映 PTE 的非法状态（保留编码） | page-fault |
| SFENCE-04 | PBMT 从保留值恢复后刷新 | 将 PBMT 从 3（保留）改回 PMA，执行 sfence.vma 后访问。验证 TLB 刷新后正确反映 PTE 从非法状态恢复为合法状态 | 正常访问 |

```c
/* SFENCE-01 示例：PBMT 变更后全局刷新 */
TEST_REGISTER(test_pbmt_sfence_change_pma_to_nc);
bool test_pbmt_sfence_change_pma_to_nc(void) {
    TEST_BEGIN("SFENCE-01: Change PBMT PMA->NC with sfence.vma");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 初始映射：PBMT=PMA */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_PMA,
                PT_LEVEL_4K);

    /* 首次访问建立 TLB */
    vm_run_in_smode(&ctx, test_smode_load, test_va);

    /* 修改 PTE：PBMT 改为 NC */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_NC,
                PT_LEVEL_4K);

    /* 全局 sfence.vma */
    vm_sfence_vma(0, 0);

    /* 验证新属性生效（NC 下 load/store 仍应成功） */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("After PBMT PMA->NC + sfence: access succeeds",
                result == 0);

    pt_pool_reset();
    TEST_END();
}

/* SFENCE-03 示例：PBMT 改为保留值后刷新 */
TEST_REGISTER(test_pbmt_sfence_change_to_reserved);
bool test_pbmt_sfence_change_to_reserved(void) {
    TEST_BEGIN("SFENCE-03: Change PBMT NC->reserved with sfence.vma");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 初始映射：PBMT=NC */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_NC,
                PT_LEVEL_4K);

    /* 首次访问建立 TLB */
    vm_run_in_smode(&ctx, test_smode_load, test_va);

    /* 修改 PTE：PBMT 改为保留值 3 */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_RSVD,
                PT_LEVEL_4K);

    /* 全局 sfence.vma */
    vm_sfence_vma(0, 0);

    /* 验证保留值触发 page-fault */
    uintptr_t result = vm_run_in_smode(&ctx,
                                        test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("After PBMT->reserved + sfence: page-fault",
                result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 9：多模式兼容性

**规范依据**：
- `norm:Svpbmt_depends_Sv39`：Svpbmt 依赖 Sv39 扩展
- PBMT 字段（bits 62-61）在 Sv39、Sv48、Sv57 PTE 格式中定义相同

**测试职责**：验证 PBMT 属性在 Sv39、Sv48 和 Sv57 模式下均正常工作。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MODE-01 | Sv39 + PBMT=NC | Sv39 模式下叶 PTE 设置 PBMT=NC，load/store | 正常访问 |
| MODE-02 | Sv48 + PBMT=NC | Sv48 模式下叶 PTE 设置 PBMT=NC，load/store | 正常访问 |
| MODE-03 | Sv57 + PBMT=NC | Sv57 模式下叶 PTE 设置 PBMT=NC，load/store | 正常访问 |

```c
/* MODE-02 示例：Sv48 + PBMT=NC */
TEST_REGISTER(test_pbmt_sv48_nc);
bool test_pbmt_sv48_nc(void) {
    TEST_BEGIN("MODE-02: Sv48 + PBMT=NC load/store");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：PBMT=NC */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_NC;
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("Sv48 + PBMT=NC works", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### ~~Group 10：两阶段地址翻译（H 扩展，条件性）~~ [已迁移]

> **注意**：请参考 `DOCS/testplan/Hypervisor_cross_test_plan.md` Group 7（Hypervisor × Svpbmt 交叉测试）。请在该文档中查看最新内容。

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1（基本编码）、Group 2（保留值异常）、Group 3（非叶 PTE 检查） | PBMT-01~06、RSVD-01~04、NLPTE-01~05 | 核心功能：PBMT 编码识别、保留值 fault、非叶 PTE 合规性 |
| P1（重要） | Group 4（权限交互+非对齐）、Group 5（页大小）、Group 6 基础排序（ORDER-01~04）、Group 8（SFENCE.VMA） | PERM-01~06、ALIGN-01~03、PGSZ-01~06、ORDER-01~04、SFENCE-01~04 | PBMT 与权限/页大小的正交性、非对齐访问约束、基础排序语义、TLB 刷新正确性 |
| P2（建议） | Group 6 高级排序（ORDER-05~06）、Group 7（别名一致性 ALIAS-01~06）、Group 9（多模式兼容） | ORDER-05~06、ALIAS-01~06、MODE-01~03 | 高级排序与 discouraged 配置、缓存一致性、跨模式验证 |
| P3（可选） | ALIAS-07（跨特权级别名） | ALIAS-07 | 依赖手动特权级管理，条件性实现 |

---

## 测试实现说明

### PBMT PTE 常量定义

Svpbmt 扩展使用 PTE bits 62-61 编码 PBMT 字段。测试代码中应定义以下常量：

```c
/* PBMT field in PTE bits 62-61 */
#define PTE_PBMT_SHIFT  61
#define PTE_PBMT_MASK   (3UL << PTE_PBMT_SHIFT)

/* PBMT encoding values */
#define PBMT_PMA        (0UL << PTE_PBMT_SHIFT)  /* 00: Use underlying PMA */
#define PBMT_NC         (1UL << PTE_PBMT_SHIFT)  /* 01: Non-cacheable, idempotent, RVWMO */
#define PBMT_IO         (2UL << PTE_PBMT_SHIFT)  /* 10: Non-cacheable, non-idempotent, I/O ordering */
#define PBMT_RSVD       (3UL << PTE_PBMT_SHIFT)  /* 11: Reserved (triggers page-fault) */

/* Extract PBMT field from a PTE */
#define PTE_PBMT(pte)   (((pte) & PTE_PBMT_MASK) >> PTE_PBMT_SHIFT)

/**
 * pbmt_set_pte - Set PBMT field in a PTE
 * @pte:  Original PTE value
 * @pbmt: PBMT value (PBMT_PMA / PBMT_NC / PBMT_IO / PBMT_RSVD)
 *
 * Returns the PTE with PBMT field set to the specified value.
 */
static inline uintptr_t pbmt_set_pte(uintptr_t pte, uintptr_t pbmt) {
    return (pte & ~PTE_PBMT_MASK) | (pbmt & PTE_PBMT_MASK);
}
```

### PBMT PTE 辅助函数

为非叶 PTE 测试（Group 3）和别名测试（Group 7），需要以下辅助函数：

```c
/**
 * pt_get_pte - Get pointer to a PTE at a specific level
 * @ctx:   Page table context
 * @va:    Virtual address
 * @level: Target page table level (PT_LEVEL_4K / PT_LEVEL_2M / PT_LEVEL_1G)
 *
 * Walks the page table to the specified level and returns a pointer
 * to the PTE entry. Useful for manually modifying PTE fields like PBMT.
 *
 * Returns NULL if the PTE at the target level does not exist.
 */
uintptr_t *pt_get_pte(pt_context_t *ctx, uintptr_t va, int level);
```

### S-mode 辅助函数

测试计划中使用的 S-mode 辅助函数：

| 函数名 | 功能 | 返回值 |
|--------|------|--------|
| `test_smode_load` | 执行 load 操作 | 0=成功 |
| `test_smode_store` | 执行 store 操作 | 0=成功 |
| `test_smode_read_write` | 写入 magic value 并读回验证 | 0=成功，非 0=失败 |
| `test_smode_load_expect_fault` | 执行 load，预期触发 fault | fault cause（如 CAUSE_LPF） |
| `test_smode_store_expect_fault` | 执行 store，预期触发 fault | fault cause（如 CAUSE_SPF） |
| `test_smode_exec_expect_fault` | 跳转执行，预期触发 fault | fault cause（如 CAUSE_IPF） |
| `test_smode_sequential_rw` | 多次顺序 store 后 load 回验证 | 0=全部正确 |
| `test_smode_store_fence_load` | store + fence iorw,iorw + load 回验证 | 0=数据正确 |
| `test_smode_alias_write_read` | 通过 VA1 写入后通过 VA2 读取验证 | 0=数据一致 |
| `test_smode_alias_flush_sequence` | 通过 VA1 写入 + fence+cbo.flush+fence + 通过 VA2 读取 | 0=数据一致 |

### 文件组织

Svpbmt 测试依赖虚拟内存测试框架，建议的文件组织如下：

```
svpbmt/
├── Makefile                # ENABLE_VM=1, TARGET=svpbmt_test.elf
├── kernel.ld               # 链接脚本（复用 sv39 的模板）
├── main.c                  # Svpbmt 测试用例入口
└── pbmt_helpers.c          # PBMT PTE 构造和修改辅助函数

common/vm/                  # 已有，无需修改
├── vm_defs.h               # 常量定义（需新增 PBMT 常量）
├── vm.h                    # API 声明（需新增 pt_get_pte）
├── page_table.c            # 页表管理（需新增 pt_get_pte 实现）
└── satp.c                  # satp 控制和 vm_run_in_smode
```

### Makefile 配置

```makefile
# svpbmt/Makefile
TARGET = svpbmt_test.elf
ENABLE_VM = 1
EXT_OBJS = main.o pbmt_helpers.o
include ../common/Makefile.common
```

### 通用测试模式

Svpbmt 测试用例遵循以下模式：

```c
#include "test_framework.h"
#include "vm/vm.h"
#include "pbmt_helpers.h"

TEST_REGISTER(test_xxx);
bool test_xxx(void) {
    TEST_BEGIN("ID: description");

    /* 1. 初始化页表并设置恒等映射 */
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);  /* 或 SV48 / SV57 */

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              flags, PT_LEVEL_1G);

    /* 2. 映射测试页面，设置 PBMT 属性 */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | PBMT_NC;  /* 或 PBMT_IO / PBMT_RSVD */
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    /* 3. 在 S-mode + VM 下执行测试 */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_xxx, test_va);
    TEST_ASSERT("description", result == expected);

    /* 4. 清理 */
    pt_pool_reset();
    TEST_END();
}
```

### 关键注意事项

1. **PBMT 常量需要新增**：当前 `common/vm/vm_defs.h` 中未定义 PBMT 相关常量。需要新增 `PTE_PBMT_SHIFT`、`PTE_PBMT_MASK`、`PBMT_PMA`、`PBMT_NC`、`PBMT_IO`、`PBMT_RSVD` 等宏定义（参见"PBMT PTE 常量定义"部分）。

2. **pt_get_pte 函数需要新增**：当前 `common/vm/page_table.c` 中未提供获取指定层级 PTE 指针的 API。需要实现 `pt_get_pte()` 函数，用于非叶 PTE 测试（Group 3）中手动修改 PTE 的 PBMT 字段。

3. **pt_map_page 的 PTE_FLAGS_MASK 截断问题（必须修复）**：当前 `PTE_FLAGS_MASK` 值为 `0x3FF`（仅覆盖 bits 9-0），而 PBMT 位于 bits 62-61。如果 `pt_map_page()` 内部使用此 mask 截断 flags 参数，PBMT 值将被完全丢弃，导致所有 PBMT 测试无法工作。**必须修改 `pt_map_page()` 实现**，确保 PBMT 高位标志不被截断。具体方案：在构造 PTE 时，将 PBMT 位与低位 flags 分别处理后合并，或扩展 flags mask 以包含 PBMT 位（如定义 `PTE_FLAGS_MASK_EXT` 覆盖 bits 63-54 和 bits 9-0）。

4. **恒等映射覆盖范围**：测试中的恒等映射必须覆盖代码段、数据段、栈、页表池和 UART I/O 区域。恒等映射使用默认 PBMT=PMA（bits 62-61 = 00），不影响代码执行区域的内存属性。

5. **PTE_A 和 PTE_D 位**：除非测试目标是 A/D 位本身，否则映射时始终设置 `PTE_A | PTE_D`，避免因 Svade 语义导致不必要的 page-fault。

6. **测试页面隔离**：验证 PBMT 属性的测试页面应与代码执行区域分开映射，使用独立的 `TEST_REGION_BASE` 地址，以避免将代码执行区域设置为 NC 或 IO 属性导致性能问题或异常。

7. **PBMT=IO 对主内存的影响**：将主内存区域设置为 PBMT=IO 后，该区域的访问遵循强 I/O 排序规则。某些实现可能对 PBMT=IO 页面的非对齐访问触发异常（`norm:Svpbmt_impl_may_override_pmas`），即使底层是主内存区域。

8. **别名测试的物理页地址**：别名测试（Group 7）需要将同一物理页映射到两个不同的虚拟地址。两个虚拟地址应位于不同的 2MB 区域内，以确保使用不同的 level-0 页表页。

9. **cbo.flush 指令依赖**：别名测试（ALIAS-04~06）中使用的 `cbo.flush` 指令来自 Zicbom 扩展。如果目标平台未实现 Zicbom，这些测试应跳过。

10. **Page-fault cause 编码**：
    - `CAUSE_IPF` (12)：Instruction page-fault
    - `CAUSE_LPF` (13)：Load page-fault
    - `CAUSE_SPF` (15)：Store/AMO page-fault

11. **QEMU 支持**：QEMU 7.0+ 支持 Svpbmt 扩展。启动时需在 `-cpu` 参数中启用 `svpbmt=true`，例如：
    ```bash
    qemu-system-riscv64 -machine virt -cpu rv64,svpbmt=true \
        -nographic -bios none -kernel svpbmt/svpbmt_test.elf
    ```

12. **PMP 配置**：`vm_run_in_smode()` 自动配置 PMP entry 15 为全地址空间 NAPOT RWX。如需测试 PMP 与 PBMT 的交互，需手动管理 PMP 配置。

13. **Hypervisor 扩展**：两阶段翻译相关测试（原 Group 10，HSTG-01~04）已迁移至 `DOCS/testplan/Hypervisor_cross_test_plan.md` Group 7。

---

## 规范覆盖矩阵

| 规范标签 | 覆盖的测试 ID |
|----------|--------------|
| `norm:Svpbmt_depends_Sv39` | MODE-01、MODE-02、MODE-03 |
| `norm:Svpbmt_impl_may_override_pmas` | ALIGN-01~03 |
| `norm:Svpbmt_nonleaf_pte_pbmt_must_be_zero` | NLPTE-01~05 |
| `norm:Svpbmt_leaf_pte_pbmt_reserved_3_fault` | RSVD-01~04 |
| `norm:Svpbmt_obeys_mem_ordering` | ORDER-01~06 |
| `norm:Svpbmt_io_pma_nc_pbmt_obey_rvwmo` | ORDER-05 |
| `norm:Svpbmt_io_pma_nc_pbmt_treated_as_io_and_memory` | ORDER-05、ORDER-06 |
| `norm:Svpbmt_memory_pma_io_pbmt_strong_io_ordering` | ORDER-01、ORDER-02 |
| `norm:Svpbmt_memory_pma_io_pbmt_treated_as_io_and_memory` | ORDER-02、ORDER-06 |
| `norm:Svpbmt_aliasing_attribute` | ALIAS-01~07 |
| `norm:Svpbmt_noncacheable_aliasing_no_coherence_loss` | ALIAS-01 |
| `norm:Svpbmt_noncacheable_aliasing_may_weaken_ordering` | ALIAS-01、ALIAS-02 |
| `norm:Svpbmt_noncacheable_aliasing_fence_prevents_ordering_loss` | ALIAS-02 |
| `norm:Svpbmt_cacheable_aliasing_may_cause_coherence_loss` | ALIAS-03、ALIAS-05 |
| `norm:Svpbmt_cacheable_aliasing_fence_flush_fence_required` | ALIAS-04、ALIAS-06 |
| `norm:Svpbmt_hgatp_stage_override_rule` | HSTG-01、HSTG-03（已迁移至 `Hypervisor_cross_test_plan.md` Group 7） |
| `norm:Svpbmt_vsatp_stage_override_rule` | HSTG-02、HSTG-03、HSTG-04（已迁移至 `Hypervisor_cross_test_plan.md` Group 7） |

---

## 参考

- RISC-V Privileged Specification — Svpbmt Extension (`SPEC/svpbmt.adoc`)
- `SPEC/supervisor.adoc` — Supervisor-Level ISA（PTE 格式定义、SFENCE.VMA 定义）
- `docs/vm_test_framework.md` — 虚拟内存测试框架文档
- `docs/vm_test_plan.md` — 虚拟内存测试计划（PTE 权限、SFENCE.VMA 参考）
- `docs/svnapot_test_plan.md` — Svnapot 测试计划（PBMT 交互测试参考）
- `common/vm/vm.h` — VM API 声明
- `common/vm/vm_defs.h` — 常量和宏定义
