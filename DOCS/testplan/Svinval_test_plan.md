**中文 | [English](../testplan_en/Svinval_test_plan_en.md)**

# Svinval 扩展测试计划

本文档描述 Svinval（Fine-Grained Address-Translation Cache Invalidation）扩展的测试计划。Svinval 扩展将 SFENCE.VMA 指令拆分为更细粒度的失效和排序操作（SINVAL.VMA、SFENCE.W.INVAL、SFENCE.INVAL.IR），以便在高性能实现中更高效地进行批量或流水线化的 TLB 失效操作。

---

## 测试范围

### 规范来源

- `SPEC/svinval.adoc` — Svinval Extension for Fine-Grained Address-Translation Cache Invalidation, Version 1.0

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svinval_split_fine_grained` | that can be more efficiently batched or pipelined on certain classes of high-performance implementation. | 可以在某些高性能实现类别上更高效地批处理或流水线化。 |
| `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma` | However, unlike SFENCE.VMA, SINVAL.VMA instructions are only ordered with respect to SFENCE.VMA, SFENCE.W.INVAL, and SFENCE.INVAL.IR instructions as defined below. | 然而，与 SFENCE.VMA 不同，SINVAL.VMA 指令仅相对于 SFENCE.VMA、SFENCE.W.INVAL 和 SFENCE.INVAL.IR 指令有序。 |
| `norm:Svinval_sfence_w_inval_orders_before_sinval_vma` | The SFENCE.INVAL.IR instruction guarantees that any previous SINVAL.VMA instructions executed by the current hart are ordered before subsequent implicit references by that hart to the memory-management data structures. | SFENCE.INVAL.IR 指令保证当前 hart 执行的任何先前 SINVAL.VMA 指令在该 hart 后续对内存管理数据结构的隐式引用之前排序。 |
| `norm:Svinval_sequence_rs1_rs2` | the values of _rs1_ and _rs2_ for the SFENCE.VMA are the same as those used in the SINVAL.VMA. | SFENCE.VMA 的 _rs1_ 和 _rs2_ 值与 SINVAL.VMA 中使用的相同。 |
| `norm:Svinval_sequence_reads_writes_before` | reads and writes prior to the SFENCE.W.INVAL are considered to be those prior to the SFENCE.VMA. | SFENCE.W.INVAL 之前的读写被视为 SFENCE.VMA 之前的读写。 |
| `norm:Svinval_sequence_reads_writes_after` | reads and writes following the SFENCE.INVAL.IR are considered to be those subsequent to the SFENCE.VMA. | SFENCE.INVAL.IR 之后的读写被视为 SFENCE.VMA 之后的读写。 |
| `norm:Svinval_hinval_vvma_gvma` | These have the same semantics as SINVAL.VMA, except that they combine with SFENCE.W.INVAL and SFENCE.INVAL.IR to replace HFENCE.VVMA and HFENCE.GVMA, respectively, instead of SFENCE.VMA. | 这些指令与 SINVAL.VMA 具有相同的语义，只是它们与 SFENCE.W.INVAL 和 SFENCE.INVAL.IR 结合分别替换 HFENCE.VVMA 和 HFENCE.GVMA，而不是 SFENCE.VMA。 |
| `norm:Svinval_hinval_gvma_uses_vmid` | HINVAL.GVMA uses VMIDs instead of ASIDs. | HINVAL.GVMA 使用 VMID 而不是 ASID。 |
| `norm:Svinval_illegal_instruction_u_mode` | In particular, an attempt to execute any of these instructions in U-mode always raises an illegal-instruction exception. | 特别是，在 U 模式下尝试执行这些指令中的任何一个总是触发非法指令异常。 |
| `norm:Svinval_illegal_instruction_tvm` | An attempt to execute SINVAL.VMA or HINVAL.GVMA in S-mode or HS-mode when `mstatus`.TVM=1 also raises an illegal-instruction exception. | 当 `mstatus`.TVM=1 时，在 S 模式或 HS 模式下尝试执行 SINVAL.VMA 或 HINVAL.GVMA 也会触发非法指令异常。 |
| `norm:Svinval_virtual_instruction_vu_vs` | An attempt to execute HINVAL.VVMA or HINVAL.GVMA in VS-mode or VU-mode, or to execute SINVAL.VMA in VU-mode, raises a virtual-instruction exception. | 在 VS 模式或 VU 模式下尝试执行 HINVAL.VVMA 或 HINVAL.GVMA，或在 VU 模式下执行 SINVAL.VMA，会触发虚拟指令异常。 |
| `norm:Svinval_virtual_instruction_vtvms` | When `hstatus`.VTVM=1, an attempt to execute SINVAL.VMA in VS-mode also raises a virtual-instruction exception. | 当 `hstatus`.VTVM=1 时，在 VS 模式下尝试执行 SINVAL.VMA 也会触发虚拟指令异常。 |
| `norm:Svinval_sfence_w_inval_inval_u_mode` | raises an illegal-instruction exception. | 在 U 模式下执行 SFENCE.W.INVAL 或 SFENCE.INVAL.IR 会触发非法指令异常。 |
| `norm:Svinval_sfence_w_inval_inval_vu_mode` | Doing so in VU-mode raises a virtual-instruction exception. | 在 VU 模式下执行会触发虚拟指令异常。 |
| `norm:Svinval_sfence_w_inval_inval_s_vs_mode` | SFENCE.W.INVAL and SFENCE.INVAL.IR are unaffected by the `mstatus`.TVM and `hstatus`.VTVM fields and hence are always permitted in S-mode and VS-mode. | SFENCE.W.INVAL 和 SFENCE.INVAL.IR 不受 `mstatus`.TVM 和 `hstatus`.VTVM 字段的影响，因此在 S 模式和 VS 模式下始终允许。 |

### 不在测试范围内

- HINVAL.VVMA / HINVAL.GVMA 指令（需要 Hypervisor 扩展支持，当前测试平台不包含 H 扩展）
- 多核场景下的 TLB 一致性（当前测试框架为单核环境）

---

## 测试用例

### Group 1：SINVAL.VMA 基本功能

**规范依据**：
- `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma`：SINVAL.VMA 失效与 SFENCE.VMA 相同的地址翻译缓存条目
- `norm:Svinval_sequence_rs1_rs2`：三指令序列中 SINVAL.VMA 的 rs1/rs2 等同于假设的 SFENCE.VMA 的 rs1/rs2

**测试职责**：验证 SFENCE.W.INVAL + SINVAL.VMA + SFENCE.INVAL.IR 序列在功能上等同于 SFENCE.VMA，即修改页表后通过该序列刷新 TLB，后续访问使用新的翻译。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SINVAL-01 | 权限升级后序列刷新 | 将 R-only 页面升级为 RW，执行 SFENCE.W.INVAL + SINVAL.VMA(va, asid=0) + SFENCE.INVAL.IR 后写入 | 写入成功 |
| SINVAL-02 | 映射失效后序列刷新 | 将有效映射改为无效（V=0），执行三指令序列后访问 | page-fault |
| SINVAL-03 | 权限降级后序列刷新 | 将 RWX 页面降级为 R-only，执行三指令序列后写入 | store page-fault |
| SINVAL-04 | 物理地址重映射后序列刷新 | 修改 PTE 指向不同的物理页，执行三指令序列后读取 | 读到新物理页的数据 |
| SINVAL-05 | rs1=x0 全地址刷新 | 修改多个页面的 PTE，执行 SINVAL.VMA(rs1=x0, rs2=x0) 全局刷新 | 所有修改的页面新权限生效 |
| SINVAL-06 | 2M 大页面 SINVAL.VMA 失效 | 修改 2M megapage 的 PTE 权限后执行三指令序列 | 整个 2M 区域新翻译生效 |
| SINVAL-07 | 1G 大页面 SINVAL.VMA 失效 | 修改 1G gigapage 的 PTE 权限后执行三指令序列 | 整个 1G 区域新翻译生效 |

```c
/* SINVAL-01 示例：权限升级后序列刷新 */

/* ---- S-mode 辅助函数 ---- */

/* 在 S-mode 下执行 store 操作，成功返回 0 */
static uintptr_t smode_store(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    *ptr = 0xDEADBEEF;
    return 0;
}

/* 在 S-mode 下执行 store，预期触发 fault，
 * 由 S-mode trap handler 捕获并返回 scause */
static uintptr_t smode_store_expect_fault(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    *ptr = 0xDEADBEEF;
    return 0;  /* 若未触发 fault 则返回 0 */
}

/* 在 S-mode 下执行 load 操作，成功返回 0 */
static uintptr_t smode_load(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    (void)*ptr;
    return 0;
}

/* ---- 测试用例 ---- */

TEST_REGISTER(test_sinval_vma_permission_upgrade);
bool test_sinval_vma_permission_upgrade(void) {
    TEST_BEGIN("SINVAL-01: Permission upgrade with SINVAL.VMA sequence");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：R-only
     * 使用恒等映射区域内、距代码段较远的地址作为测试页面 */
    uintptr_t test_va = (PLATFORM_MEM_BASE + 0x800000);  /* 基地址 + 8MB */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 验证 R-only：写应失败 */
    uintptr_t result = vm_run_in_smode(&ctx, smode_store_expect_fault,
                                        test_va);
    TEST_ASSERT("R-only: write faults", result == CAUSE_SPF);

    /* 修改 PTE 为 RW */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 执行 SFENCE.W.INVAL + SINVAL.VMA + SFENCE.INVAL.IR 序列 */
    SFENCE_W_INVAL();
    SINVAL_VMA(test_va, 0);
    SFENCE_INVAL_IR();

    /* 验证 RW：写应成功 */
    result = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("After SINVAL.VMA sequence: write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 2：排序语义验证

**规范依据**：
- `norm:Svinval_sfence_w_inval_orders_before_sinval_vma`：SFENCE.W.INVAL 保证先前的 store 在后续 SINVAL.VMA 之前排序；SFENCE.INVAL.IR 保证先前的 SINVAL.VMA 在后续隐式内存管理数据结构引用之前排序
- `norm:Svinval_sequence_reads_writes_before`：SFENCE.W.INVAL 之前的读写等同于 SFENCE.VMA 之前的读写
- `norm:Svinval_sequence_reads_writes_after`：SFENCE.INVAL.IR 之后的读写等同于 SFENCE.VMA 之后的读写

**测试职责**：验证三指令序列的排序保证——SFENCE.W.INVAL 之前的 store（页表修改）对 SINVAL.VMA 可见，SFENCE.INVAL.IR 之后的访问使用更新后的翻译。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ORDER-01 | store -> W.INVAL -> SINVAL -> INVAL.IR -> load | 修改 PTE（store），执行完整三指令序列，然后 load 验证新翻译生效 | load 使用新翻译 |
| ORDER-02 | 缺少 SFENCE.W.INVAL 的序列 | 修改 PTE 后仅执行 SINVAL.VMA + SFENCE.INVAL.IR（省略 SFENCE.W.INVAL） | 行为不确定（可能使用旧翻译） |
| ORDER-03 | 缺少 SFENCE.INVAL.IR 的序列 | 修改 PTE 后执行 SFENCE.W.INVAL + SINVAL.VMA（省略 SFENCE.INVAL.IR） | 行为不确定（可能使用旧翻译） |
| ORDER-04 | 完整序列与 SFENCE.VMA 等价性 | 对同一页面分别用 SFENCE.VMA 和三指令序列刷新，验证结果一致 | 两种方式结果相同 |
| ORDER-05 | SFENCE.VMA 作为 SINVAL.VMA 的排序 fence | 修改 PTE 后执行 SINVAL.VMA，再执行 SFENCE.VMA（替代 SFENCE.INVAL.IR），然后访问页面 | SFENCE.VMA 保证 SINVAL.VMA 生效，新翻译生效 |
| ORDER-06 | 多个 SINVAL.VMA 后跟 SFENCE.VMA | 修改多个页面 PTE，批量执行多个 SINVAL.VMA，最后用 SFENCE.VMA 替代 SFENCE.INVAL.IR | 所有页面新翻译生效 |

> **注意**：ORDER-02 和 ORDER-03 测试的是不完整序列的行为。规范不保证不完整序列的正确性，但简单实现可能将 SINVAL.VMA 实现为与 SFENCE.VMA 完全相同、将 SFENCE.W.INVAL 和 SFENCE.INVAL.IR 实现为 NOP，因此不完整序列在这些实现上可能仍然正确工作。这两个测试仅用于探测实现行为，不作为合规性判定依据。

```c
/* ORDER-01 示例：完整序列排序验证 */
TEST_REGISTER(test_sinval_ordering_complete);
bool test_sinval_ordering_complete(void) {
    TEST_BEGIN("ORDER-01: Complete sequence ordering guarantee");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 初始映射：RW */
    uintptr_t test_va = (PLATFORM_MEM_BASE + 0x800000);  /* 基地址 + 8MB */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 首次访问，建立 TLB 条目 */
    uintptr_t result = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("Initial write succeeds", result == 0);

    /* 修改 PTE：降级为 R-only（store 操作） */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 完整三指令序列 */
    SFENCE_W_INVAL();
    SINVAL_VMA(test_va, 0);
    SFENCE_INVAL_IR();

    /* SFENCE.INVAL.IR 之后的 store 应使用新翻译（R-only），触发 fault */
    result = vm_run_in_smode(&ctx, smode_store_expect_fault, test_va);
    TEST_ASSERT("After complete sequence: write faults (R-only)",
                result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 3：批量失效操作

**规范依据**：
- `norm:Svinval_split_fine_grained`：Svinval 的设计目标是支持批量或流水线化的 TLB 失效操作
- `norm:Svinval_sfence_w_inval_orders_before_sinval_vma`：多个 SINVAL.VMA 可以在 SFENCE.W.INVAL 和 SFENCE.INVAL.IR 之间批量执行

**测试职责**：验证在 SFENCE.W.INVAL 和 SFENCE.INVAL.IR 之间执行多个 SINVAL.VMA 指令的正确性，这是 Svinval 扩展的核心使用场景。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| BATCH-01 | 批量失效多个页面 | 修改 3 个页面的 PTE，在 W.INVAL 和 INVAL.IR 之间执行 3 个 SINVAL.VMA | 所有 3 个页面新权限生效 |
| BATCH-02 | 批量失效不同权限变更 | 页面 A 升级 R->RW，页面 B 降级 RW->R，页面 C 失效 V->0，批量刷新 | A 可写，B 写触发 fault，C 访问触发 fault |
| BATCH-03 | 批量失效大量页面 | 修改 16 个连续页面的 PTE，批量执行 16 个 SINVAL.VMA | 所有 16 个页面新权限生效 |
| BATCH-04 | 批量失效混合 rs1 参数 | 部分 SINVAL.VMA 指定具体地址，部分使用 rs1=x0 | 所有指定的页面被正确刷新 |

```c
/* BATCH-01 示例：批量失效多个页面 */
TEST_REGISTER(test_sinval_batch_invalidation);
bool test_sinval_batch_invalidation(void) {
    TEST_BEGIN("BATCH-01: Batch invalidation of multiple pages");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射 3 个测试页面：初始为 R-only */
    uintptr_t test_base = (PLATFORM_MEM_BASE + 0x800000);  /* 基地址 + 8MB */
    uintptr_t va0 = test_base;
    uintptr_t va1 = test_base + PAGE_SIZE_4K;
    uintptr_t va2 = test_base + 2 * PAGE_SIZE_4K;

    pt_map_page(&ctx, va0, va0, PTE_V|PTE_R|PTE_A|PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va1, va1, PTE_V|PTE_R|PTE_A|PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va2, va2, PTE_V|PTE_R|PTE_A|PTE_D, PT_LEVEL_4K);

    /* 首次访问建立 TLB */
    vm_run_in_smode(&ctx, smode_load, va0);
    vm_run_in_smode(&ctx, smode_load, va1);
    vm_run_in_smode(&ctx, smode_load, va2);

    /* 修改所有 3 个页面为 RW */
    pt_map_page(&ctx, va0, va0, PTE_V|PTE_R|PTE_W|PTE_A|PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va1, va1, PTE_V|PTE_R|PTE_W|PTE_A|PTE_D, PT_LEVEL_4K);
    pt_map_page(&ctx, va2, va2, PTE_V|PTE_R|PTE_W|PTE_A|PTE_D, PT_LEVEL_4K);

    /* 批量失效：W.INVAL + 3x SINVAL.VMA + INVAL.IR */
    SFENCE_W_INVAL();
    SINVAL_VMA(va0, 0);
    SINVAL_VMA(va1, 0);
    SINVAL_VMA(va2, 0);
    SFENCE_INVAL_IR();

    /* 验证所有 3 个页面可写 */
    uintptr_t r0 = vm_run_in_smode(&ctx, smode_store, va0);
    uintptr_t r1 = vm_run_in_smode(&ctx, smode_store, va1);
    uintptr_t r2 = vm_run_in_smode(&ctx, smode_store, va2);
    TEST_ASSERT("Page 0 write succeeds", r0 == 0);
    TEST_ASSERT("Page 1 write succeeds", r1 == 0);
    TEST_ASSERT("Page 2 write succeeds", r2 == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 4：rs1/rs2 参数组合

**规范依据**：
- `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma`：SINVAL.VMA 使用与 SFENCE.VMA 相同的 rs1/rs2 语义
- `norm:Svinval_sequence_rs1_rs2`：三指令序列中 SINVAL.VMA 的 rs1/rs2 等同于假设的 SFENCE.VMA 的 rs1/rs2

**测试职责**：验证 SINVAL.VMA 的 rs1（虚拟地址）和 rs2（ASID）参数的不同组合，确保与 SFENCE.VMA 的参数语义一致。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PARAM-01 | rs1=addr, rs2=x0 | 指定地址，ASID=0（所有 ASID），修改该地址 PTE 后刷新 | 该地址新翻译生效 |
| PARAM-02 | rs1=x0, rs2=x0 | 全局刷新（所有地址、所有 ASID），修改多个 PTE 后刷新 | 所有修改的页面新翻译生效 |
| PARAM-03 | rs1=addr, rs2=asid | 指定地址和 ASID，修改该地址 PTE 后刷新 | 该地址新翻译生效 |
| PARAM-04 | rs1=x0, rs2=asid | 指定 ASID 的所有地址，修改多个 PTE 后刷新 | 该 ASID 下所有修改的页面新翻译生效 |
| PARAM-05 | 刷新不匹配的地址 | 修改页面 A 的 PTE，但 SINVAL.VMA 指定页面 B 的地址 | 页面 A 可能仍使用旧翻译（行为不确定） |

```c
/* PARAM-01 示例：指定地址刷新 */
TEST_REGISTER(test_sinval_param_addr_only);
bool test_sinval_param_addr_only(void) {
    TEST_BEGIN("PARAM-01: SINVAL.VMA with specific address, ASID=0");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：R-only */
    uintptr_t test_va = (PLATFORM_MEM_BASE + 0x800000);  /* 基地址 + 8MB */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 首次访问建立 TLB */
    vm_run_in_smode(&ctx, smode_load, test_va);

    /* 修改 PTE 为 RW */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 使用指定地址的 SINVAL.VMA 刷新 */
    SFENCE_W_INVAL();
    SINVAL_VMA(test_va, 0);
    SFENCE_INVAL_IR();

    /* 验证新权限生效 */
    uintptr_t result = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("Specific address flush: write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 5：SINVAL.VMA 特权级异常检查

**规范依据**：
- `norm:Svinval_illegal_instruction_u_mode`：U-mode 执行 SINVAL.VMA 始终触发 illegal-instruction 异常
- `norm:Svinval_illegal_instruction_tvm`：mstatus.TVM=1 时，S-mode 执行 SINVAL.VMA 触发 illegal-instruction 异常

**测试职责**：验证 SINVAL.VMA 指令在不同特权级和 CSR 配置下的异常行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PRIV-01 | U-mode 执行 SINVAL.VMA | 在 U-mode 下执行 SINVAL.VMA 指令 | illegal-instruction 异常（cause=2） |
| PRIV-02 | S-mode TVM=0 执行 SINVAL.VMA | mstatus.TVM=0 时，S-mode 执行 SINVAL.VMA | 正常执行，无异常 |
| PRIV-03 | S-mode TVM=1 执行 SINVAL.VMA | mstatus.TVM=1 时，S-mode 执行 SINVAL.VMA | illegal-instruction 异常（cause=2） |
| PRIV-04 | M-mode 执行 SINVAL.VMA | 在 M-mode 下执行 SINVAL.VMA 指令 | 正常执行，无异常 |

```c
/* PRIV-01 示例：U-mode 执行 SINVAL.VMA 触发异常 */
TEST_REGISTER(test_sinval_umode_illegal);
bool test_sinval_umode_illegal(void) {
    TEST_BEGIN("PRIV-01: SINVAL.VMA in U-mode triggers illegal instruction");

    /* 配置 PMP entry 15 允许 U-mode 执行代码 */
    pmp_entry_t fw = PMP_ENTRY_NAPOT(PLATFORM_MEM_BASE, 0x4000000, PMP_RWX);
    pmp_set_entry(15, &fw);

    goto_priv(PRIV_U);

    /* U-mode 执行 SINVAL.VMA */
    PRIV_DO_TRAP(SINVAL_VMA(0, 0));

    goto_priv(PRIV_M);
    CHECK_TRAP("PRIV-01: U-mode SINVAL.VMA triggers illegal instruction",
               CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* PRIV-02 示例：S-mode TVM=0 执行 SINVAL.VMA 正常 */
TEST_REGISTER(test_sinval_smode_tvm0);
bool test_sinval_smode_tvm0(void) {
    TEST_BEGIN("PRIV-02: SINVAL.VMA in S-mode with TVM=0 succeeds");

    /* 确保 TVM=0 */
    CSRC(mstatus, MSTATUS_TVM_BIT);

    /* 配置 PMP entry 15 允许 S-mode 执行代码 */
    pmp_entry_t fw = PMP_ENTRY_NAPOT(PLATFORM_MEM_BASE, 0x4000000, PMP_RWX);
    pmp_set_entry(15, &fw);

    goto_priv(PRIV_S);

    /* S-mode 执行 SINVAL.VMA，TVM=0 应正常执行 */
    PRIV_DO_NO_TRAP(SINVAL_VMA(0, 0));

    goto_priv(PRIV_M);
    CHECK_NO_TRAP("PRIV-02: S-mode TVM=0 SINVAL.VMA succeeds");

    TEST_END();
}

/* PRIV-03 示例：S-mode TVM=1 执行 SINVAL.VMA 触发异常 */
TEST_REGISTER(test_sinval_smode_tvm1);
bool test_sinval_smode_tvm1(void) {
    TEST_BEGIN("PRIV-03: SINVAL.VMA in S-mode with TVM=1 triggers illegal instruction");

    /* 设置 mstatus.TVM=1 */
    CSRS(mstatus, MSTATUS_TVM_BIT);

    /* 配置 PMP entry 15 允许 S-mode 执行代码 */
    pmp_entry_t fw = PMP_ENTRY_NAPOT(PLATFORM_MEM_BASE, 0x4000000, PMP_RWX);
    pmp_set_entry(15, &fw);

    goto_priv(PRIV_S);

    /* S-mode 执行 SINVAL.VMA，TVM=1 应触发异常 */
    PRIV_DO_TRAP(SINVAL_VMA(0, 0));

    goto_priv(PRIV_M);
    CHECK_TRAP("PRIV-03: S-mode TVM=1 SINVAL.VMA triggers illegal instruction",
               CAUSE_ILLEGAL_INST);

    /* 清除 TVM */
    CSRC(mstatus, MSTATUS_TVM_BIT);

    TEST_END();
}
```

---

### Group 6：SFENCE.W.INVAL / SFENCE.INVAL.IR 特权级行为

**规范依据**：
- `norm:Svinval_sfence_w_inval_inval_u_mode`：U-mode 执行 SFENCE.W.INVAL 或 SFENCE.INVAL.IR 触发 illegal-instruction 异常
- `norm:Svinval_sfence_w_inval_inval_s_vs_mode`：SFENCE.W.INVAL 和 SFENCE.INVAL.IR 不受 mstatus.TVM 和 hstatus.VTVM 影响，S-mode 和 VS-mode 始终允许执行

**测试职责**：验证 SFENCE.W.INVAL 和 SFENCE.INVAL.IR 指令的特权级访问控制，特别是它们不受 TVM 位影响的特性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FENCE-01 | U-mode 执行 SFENCE.W.INVAL | 在 U-mode 下执行 SFENCE.W.INVAL | illegal-instruction 异常（cause=2） |
| FENCE-02 | U-mode 执行 SFENCE.INVAL.IR | 在 U-mode 下执行 SFENCE.INVAL.IR | illegal-instruction 异常（cause=2） |
| FENCE-03 | S-mode TVM=0 执行 SFENCE.W.INVAL | mstatus.TVM=0 时，S-mode 执行 SFENCE.W.INVAL | 正常执行，无异常 |
| FENCE-04 | S-mode TVM=0 执行 SFENCE.INVAL.IR | mstatus.TVM=0 时，S-mode 执行 SFENCE.INVAL.IR | 正常执行，无异常 |
| FENCE-05 | S-mode TVM=1 执行 SFENCE.W.INVAL | mstatus.TVM=1 时，S-mode 执行 SFENCE.W.INVAL | 正常执行，无异常（不受 TVM 影响） |
| FENCE-06 | S-mode TVM=1 执行 SFENCE.INVAL.IR | mstatus.TVM=1 时，S-mode 执行 SFENCE.INVAL.IR | 正常执行，无异常（不受 TVM 影响） |
| FENCE-07 | M-mode 执行 SFENCE.W.INVAL | 在 M-mode 下执行 SFENCE.W.INVAL | 正常执行，无异常 |
| FENCE-08 | M-mode 执行 SFENCE.INVAL.IR | 在 M-mode 下执行 SFENCE.INVAL.IR | 正常执行，无异常 |

```c
/* FENCE-01 示例：U-mode 执行 SFENCE.W.INVAL 触发异常 */
TEST_REGISTER(test_sfence_w_inval_umode);
bool test_sfence_w_inval_umode(void) {
    TEST_BEGIN("FENCE-01: SFENCE.W.INVAL in U-mode triggers illegal instruction");

    /* 配置 PMP entry 15 允许 U-mode 执行代码 */
    pmp_entry_t fw = PMP_ENTRY_NAPOT(PLATFORM_MEM_BASE, 0x4000000, PMP_RWX);
    pmp_set_entry(15, &fw);

    goto_priv(PRIV_U);

    PRIV_DO_TRAP(SFENCE_W_INVAL());

    goto_priv(PRIV_M);
    CHECK_TRAP("FENCE-01: U-mode SFENCE.W.INVAL triggers illegal instruction",
               CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* FENCE-02 示例：U-mode 执行 SFENCE.INVAL.IR 触发异常 */
TEST_REGISTER(test_sfence_inval_ir_umode);
bool test_sfence_inval_ir_umode(void) {
    TEST_BEGIN("FENCE-02: SFENCE.INVAL.IR in U-mode triggers illegal instruction");

    /* 配置 PMP entry 15 允许 U-mode 执行代码 */
    pmp_entry_t fw = PMP_ENTRY_NAPOT(PLATFORM_MEM_BASE, 0x4000000, PMP_RWX);
    pmp_set_entry(15, &fw);

    goto_priv(PRIV_U);

    PRIV_DO_TRAP(SFENCE_INVAL_IR());

    goto_priv(PRIV_M);
    CHECK_TRAP("FENCE-02: U-mode SFENCE.INVAL.IR triggers illegal instruction",
               CAUSE_ILLEGAL_INST);

    TEST_END();
}

/* FENCE-05 示例：S-mode TVM=1 执行 SFENCE.W.INVAL 不受影响 */
TEST_REGISTER(test_sfence_w_inval_smode_tvm1);
bool test_sfence_w_inval_smode_tvm1(void) {
    TEST_BEGIN("FENCE-05: SFENCE.W.INVAL in S-mode with TVM=1 still permitted");

    /* 设置 mstatus.TVM=1 */
    CSRS(mstatus, MSTATUS_TVM_BIT);

    /* 配置 PMP entry 15 允许 S-mode 执行代码 */
    pmp_entry_t fw = PMP_ENTRY_NAPOT(PLATFORM_MEM_BASE, 0x4000000, PMP_RWX);
    pmp_set_entry(15, &fw);

    goto_priv(PRIV_S);

    /* S-mode 执行 SFENCE.W.INVAL，即使 TVM=1 也应正常执行 */
    PRIV_DO_NO_TRAP(SFENCE_W_INVAL());

    goto_priv(PRIV_M);
    CHECK_NO_TRAP("FENCE-05: S-mode TVM=1 SFENCE.W.INVAL permitted");

    /* 清除 TVM */
    CSRC(mstatus, MSTATUS_TVM_BIT);

    TEST_END();
}

/* FENCE-06 示例：S-mode TVM=1 执行 SFENCE.INVAL.IR 不受影响 */
TEST_REGISTER(test_sfence_inval_ir_smode_tvm1);
bool test_sfence_inval_ir_smode_tvm1(void) {
    TEST_BEGIN("FENCE-06: SFENCE.INVAL.IR in S-mode with TVM=1 still permitted");

    /* 设置 mstatus.TVM=1 */
    CSRS(mstatus, MSTATUS_TVM_BIT);

    /* 配置 PMP entry 15 允许 S-mode 执行代码 */
    pmp_entry_t fw = PMP_ENTRY_NAPOT(PLATFORM_MEM_BASE, 0x4000000, PMP_RWX);
    pmp_set_entry(15, &fw);

    goto_priv(PRIV_S);

    /* S-mode 执行 SFENCE.INVAL.IR，即使 TVM=1 也应正常执行 */
    PRIV_DO_NO_TRAP(SFENCE_INVAL_IR());

    goto_priv(PRIV_M);
    CHECK_NO_TRAP("FENCE-06: S-mode TVM=1 SFENCE.INVAL.IR permitted");

    /* 清除 TVM */
    CSRC(mstatus, MSTATUS_TVM_BIT);

    TEST_END();
}
```

---

### Group 7：指令编码验证

**规范依据**：
- Svinval 扩展引入了 3 条新指令（SINVAL.VMA、SFENCE.W.INVAL、SFENCE.INVAL.IR），需要验证指令编码被硬件正确识别

**测试职责**：验证 Svinval 指令在 M-mode 下可以正常执行而不触发 illegal-instruction 异常。这是所有其他功能测试的前提——如果指令编码本身有误或平台不支持 Svinval 扩展，所有功能测试都将无法正确运行。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ENC-01 | SINVAL.VMA 指令编码验证 | 在 M-mode 下执行 SINVAL.VMA，确认不触发 illegal-instruction | 正常执行，无异常 |
| ENC-02 | SFENCE.W.INVAL 指令编码验证 | 在 M-mode 下执行 SFENCE.W.INVAL，确认不触发 illegal-instruction | 正常执行，无异常 |
| ENC-03 | SFENCE.INVAL.IR 指令编码验证 | 在 M-mode 下执行 SFENCE.INVAL.IR，确认不触发 illegal-instruction | 正常执行，无异常 |

```c
/* ENC-01 示例：SINVAL.VMA 指令编码验证 */
TEST_REGISTER(test_sinval_vma_encoding);
bool test_sinval_vma_encoding(void) {
    TEST_BEGIN("ENC-01: SINVAL.VMA instruction encoding valid");

    /* M-mode 下执行 SINVAL.VMA，不应触发异常 */
    EXPECT_NO_TRAP(SINVAL_VMA(0, 0));

    TEST_END();
}

/* ENC-02 示例：SFENCE.W.INVAL 指令编码验证 */
TEST_REGISTER(test_sfence_w_inval_encoding);
bool test_sfence_w_inval_encoding(void) {
    TEST_BEGIN("ENC-02: SFENCE.W.INVAL instruction encoding valid");

    EXPECT_NO_TRAP(SFENCE_W_INVAL());

    TEST_END();
}

/* ENC-03 示例：SFENCE.INVAL.IR 指令编码验证 */
TEST_REGISTER(test_sfence_inval_ir_encoding);
bool test_sfence_inval_ir_encoding(void) {
    TEST_BEGIN("ENC-03: SFENCE.INVAL.IR instruction encoding valid");

    EXPECT_NO_TRAP(SFENCE_INVAL_IR());

    TEST_END();
}
```

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 7（编码验证）、Group 1（SINVAL.VMA 基本功能）、Group 5（特权级异常） | ENC-01~03、SINVAL-01~07、PRIV-01~04 | 指令编码基础验证、核心功能验证和安全性保证 |
| P1（重要） | Group 2（排序语义）、Group 6（SFENCE.W.INVAL/INVAL.IR 特权级） | ORDER-01~06、FENCE-01~08 | 排序正确性和特权级访问控制 |
| P2（建议） | Group 3（批量失效）、Group 4（rs1/rs2 参数组合） | BATCH-01~04、PARAM-01~05 | 批量操作和参数覆盖 |

---

## 测试实现说明

### 指令编码

Svinval 扩展引入的指令需要工具链支持。根据 RISC-V 规范和 binutils 实现，指令编码如下：

```
31..25    24..20  19..15  14..12  11..7   6..0
sinval.vma       0001011  rs2     rs1     000     00000   1110011
sfence.w.inval   0001100  00000   00000   000     00000   1110011
sfence.inval.ir  0001100  00001   00000   000     00000   1110011
hinval.vvma      0011011  rs2     rs1     000     00000   1110011
hinval.gvma      0111011  rs2     rs1     000     00000   1110011
```

测试代码中应定义以下宏来封装指令发射，优先使用汇编器助记符，当工具链不支持时回退到 `.insn` 编码：

```c
/*
 * Svinval 指令宏定义
 *
 * 如果汇编器支持 Svinval 助记符（GCC 12+ / binutils 2.38+），
 * 可直接使用助记符形式。否则使用 .insn 手动编码。
 *
 * 编码格式（R-type）：
 *   .insn r opcode, funct3, funct7, rd, rs1, rs2
 *
 * sinval.vma rs1, rs2:
 *   opcode=0x73(SYSTEM), funct3=0, funct7=0x0B(0001011), rd=x0
 *
 * sfence.w.inval:
 *   opcode=0x73(SYSTEM), funct3=0, funct7=0x0C(0001100), rd=x0, rs1=x0, rs2=x0
 *
 * sfence.inval.ir:
 *   opcode=0x73(SYSTEM), funct3=0, funct7=0x0C(0001100), rd=x0, rs1=x0, rs2=x1
 */

#ifdef USE_SVINVAL_ASM
/* 汇编器原生支持 */
#define SINVAL_VMA(vaddr, asid) \
    asm volatile("sinval.vma %0, %1" :: "r"((uintptr_t)(vaddr)), \
                 "r"((uintptr_t)(asid)) : "memory")

#define SFENCE_W_INVAL() \
    asm volatile("sfence.w.inval" ::: "memory")

#define SFENCE_INVAL_IR() \
    asm volatile("sfence.inval.ir" ::: "memory")

#else
/* 使用 .insn 手动编码 */
#define SINVAL_VMA(vaddr, asid) \
    asm volatile(".insn r 0x73, 0, 0x0B, x0, %0, %1" \
                 :: "r"((uintptr_t)(vaddr)), "r"((uintptr_t)(asid)) : "memory")

#define SFENCE_W_INVAL() \
    asm volatile(".insn r 0x73, 0, 0x0C, x0, x0, x0" ::: "memory")

#define SFENCE_INVAL_IR() \
    asm volatile(".insn r 0x73, 0, 0x0C, x0, x0, x1" ::: "memory")

#endif /* USE_SVINVAL_ASM */
```

### S-mode 辅助函数

测试计划中使用的 S-mode 辅助函数需要在测试代码中定义。这些函数通过 `vm_run_in_smode()` 在启用虚拟内存的 S-mode 下执行：

```c
/* 在 S-mode 下执行 load 操作，成功返回 0 */
static uintptr_t smode_load(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    (void)*ptr;
    return 0;
}

/* 在 S-mode 下执行 store 操作，成功返回 0 */
static uintptr_t smode_store(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    *ptr = 0xDEADBEEF;
    return 0;
}

/* 在 S-mode 下执行 store，预期触发 fault。
 * S-mode trap handler 捕获 page-fault 后将 scause 作为返回值。
 * 若未触发 fault 则返回 0。 */
static uintptr_t smode_store_expect_fault(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    *ptr = 0xDEADBEEF;
    return 0;
}

/* 在 S-mode 下执行 load，预期触发 fault。
 * S-mode trap handler 捕获 page-fault 后将 scause 作为返回值。
 * 若未触发 fault 则返回 0。 */
static uintptr_t smode_load_expect_fault(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    (void)*ptr;
    return 0;
}

/* 在 S-mode 下执行读写验证，写入 magic value 并读回验证 */
static uintptr_t smode_read_write(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    *ptr = 0xDEADBEEF;
    if (*ptr != 0xDEADBEEF)
        return 1;
    return 0;
}
```

> **注意**：`smode_store_expect_fault` 和 `smode_load_expect_fault` 函数本身不处理异常。异常由 `vm_run_in_smode()` 内部设置的 S-mode trap handler 捕获，trap handler 将 `scause` 值作为 `vm_run_in_smode()` 的返回值。因此调用者通过检查返回值来判断是否触发了预期的 fault。

### 文件组织

Svinval 测试依赖虚拟内存测试框架和 PMP 配置库，建议的文件组织如下：

```
svinval/
├── Makefile            # ENABLE_VM=1, ENABLE_PMP=1, TARGET=svinval_test.elf
├── kernel.ld           # 链接脚本（复用 sv39 的模板）
└── main.c              # Svinval 测试用例入口

common/vm/              # 已有，无需修改
├── vm_defs.h           # 常量定义
├── vm.h                # API 声明
├── page_table.c        # 页表管理
└── satp.c              # satp 控制和 vm_run_in_smode

common/pmp/             # 已有，无需修改
├── pmp_cfg.h           # PMP 配置 API（pmp_set_entry、PMP_ENTRY_NAPOT 等）
└── pmp_cfg.c           # PMP 配置实现
```

### Makefile 配置

```makefile
# svinval/Makefile
TARGET = svinval_test.elf
ENABLE_VM = 1
ENABLE_PMP = 1
EXT_OBJS = main.o
include ../common/Makefile.common
```

> **注意**：需要同时启用 `ENABLE_VM=1`（用于页表管理和 `vm_run_in_smode`）和 `ENABLE_PMP=1`（用于特权级测试中的 PMP 配置）。编译时需确保 `-march` 参数包含 Svinval 扩展支持，可能需要在 Makefile 中追加 `MARCH += _svinval`。

### 通用测试模式

Svinval 测试用例遵循以下模式：

```c
#include "test_framework.h"
#include "vm/vm.h"
#include "pmp_cfg.h"

/* Svinval 指令宏（见"指令编码"部分） */
#include "svinval_insn.h"

TEST_REGISTER(test_xxx);
bool test_xxx(void) {
    TEST_BEGIN("ID: description");

    /* 1. 初始化页表并设置恒等映射 */
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              flags, PT_LEVEL_1G);

    /* 2. 映射测试页面（恒等映射区域内、距代码段较远的地址） */
    uintptr_t test_va = (PLATFORM_MEM_BASE + 0x800000);  /* 基地址 + 8MB */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 3. 首次访问建立 TLB 条目 */
    vm_run_in_smode(&ctx, smode_load, test_va);

    /* 4. 修改 PTE */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 5. 执行 Svinval 三指令序列 */
    SFENCE_W_INVAL();
    SINVAL_VMA(test_va, 0);
    SFENCE_INVAL_IR();

    /* 6. 验证新翻译生效 */
    uintptr_t result = vm_run_in_smode(&ctx, smode_store, test_va);
    TEST_ASSERT("description", result == 0);

    /* 7. 清理 */
    pt_pool_reset();
    TEST_END();
}
```

### 关键注意事项

1. **工具链支持**：确保使用的 GCC/binutils 版本支持 Svinval 指令助记符（GCC 12+ / binutils 2.38+）。如不支持，在 Makefile 中定义 `CFLAGS += -DUSE_SVINVAL_ASM=0` 并使用 `.insn` 伪指令手动编码（参见"指令编码"部分）。

2. **QEMU 支持**：QEMU 7.0+ 支持 Svinval 扩展。启动时需在 `-cpu` 参数中启用 `svinval=true`，例如：
   ```bash
   qemu-system-riscv64 -machine virt -cpu rv64,svinval=true \
       -nographic -bios none -kernel svinval/svinval_test.elf
   ```

3. **简单实现的等价性**：规范允许简单实现将 SINVAL.VMA 实现为与 SFENCE.VMA 完全相同，将 SFENCE.W.INVAL 和 SFENCE.INVAL.IR 实现为 NOP。因此 ORDER-02 和 ORDER-03（缺少部分指令的序列）在简单实现上可能仍然正确工作。这些测试标记为"行为不确定"，仅用于探测实现行为，不作为合规性判定依据。

4. **Hypervisor 扩展**：HINVAL.VVMA 和 HINVAL.GVMA 指令的测试需要 Hypervisor 扩展支持（`norm:Svinval_hinval_vvma_gvma`、`norm:Svinval_hinval_gvma_uses_vmid`、`norm:Svinval_virtual_instruction_vu_vs`、`norm:Svinval_virtual_instruction_vtvms`、`norm:Svinval_sfence_w_inval_inval_vu_mode`）。当前测试平台不包含 H 扩展，这些测试暂不实现。待 H 扩展测试框架就绪后，应补充以下测试：
   - HINVAL.VVMA 在 VS-mode/VU-mode 触发 virtual-instruction 异常
   - HINVAL.GVMA 在 VS-mode/VU-mode 触发 virtual-instruction 异常
   - HINVAL.GVMA 使用 VMID 而非 ASID
   - hstatus.VTVM=1 时 SINVAL.VMA 在 VS-mode 触发 virtual-instruction 异常
   - SFENCE.W.INVAL/SFENCE.INVAL.IR 在 VU-mode 触发 virtual-instruction 异常

5. **PTE_A 和 PTE_D 位**：除非测试目标是 A/D 位本身，否则映射时始终设置 `PTE_A | PTE_D`，避免因 Svade 语义导致不必要的 page-fault。

6. **PMP 配置**：特权级测试（Group 5、Group 6）需要配置 PMP 以允许 S/U-mode 执行代码。使用 `pmp_set_entry()` 和 `PMP_ENTRY_NAPOT()` 宏（定义在 `common/pmp/pmp_cfg.h`）配置 PMP entry 15 为覆盖固件代码区域的 NAPOT RWX 规则。`TEST_END()` 宏会自动调用 `reset_state()` 清除 PMP 配置。

7. **测试页面地址**：测试页面使用恒等映射区域内、距代码段较远的地址，如 `PLATFORM_MEM_BASE + 0x800000`（距基地址 8MB 处，即 `0x80800000`）。该地址在 QEMU virt 平台的 256MB 物理内存范围内（`0x80000000` ~ `0x8FFFFFFF`），且远离代码段（通常位于 `0x80000000` 起始的前几 MB），避免与代码执行区域冲突。

8. **Page-fault cause 编码**：
   - `CAUSE_IPF` (12)：Instruction page-fault
   - `CAUSE_LPF` (13)：Load page-fault
   - `CAUSE_SPF` (15)：Store/AMO page-fault
   - `CAUSE_ILLEGAL_INST` (2)：Illegal instruction

---

## 规范覆盖矩阵

| 规范标签 | 覆盖的测试 ID |
|----------|--------------|
| `norm:Svinval_split_fine_grained` | BATCH-01~04 |
| `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma` | SINVAL-01~07、PARAM-01~05、ORDER-05~06 |
| `norm:Svinval_sfence_w_inval_orders_before_sinval_vma` | ORDER-01~06 |
| `norm:Svinval_sequence_rs1_rs2` | SINVAL-01~07、PARAM-01~05 |
| `norm:Svinval_sequence_reads_writes_before` | ORDER-01~06 |
| `norm:Svinval_sequence_reads_writes_after` | ORDER-01~06 |
| `norm:Svinval_hinval_vvma_gvma` | 待 H 扩展支持后补充 |
| `norm:Svinval_hinval_gvma_uses_vmid` | 待 H 扩展支持后补充 |
| `norm:Svinval_illegal_instruction_u_mode` | PRIV-01 |
| `norm:Svinval_illegal_instruction_tvm` | PRIV-03 |
| `norm:Svinval_virtual_instruction_vu_vs` | 待 H 扩展支持后补充 |
| `norm:Svinval_virtual_instruction_vtvms` | 待 H 扩展支持后补充 |
| `norm:Svinval_sfence_w_inval_inval_u_mode` | FENCE-01、FENCE-02 |
| `norm:Svinval_sfence_w_inval_inval_vu_mode` | 待 H 扩展支持后补充 |
| `norm:Svinval_sfence_w_inval_inval_s_vs_mode` | FENCE-03~06 |

---

## 参考

- RISC-V Privileged Specification — Svinval Extension (`SPEC/svinval.adoc`)
- `SPEC/supervisor.adoc` — Supervisor-Level ISA（SFENCE.VMA 定义）
- `docs/vm_test_framework.md` — 虚拟内存测试框架文档
- `docs/vm_test_plan.md` — 虚拟内存测试计划（SFENCE.VMA 测试参考）
- `common/vm/vm.h` — VM API 声明
- `common/vm/vm_defs.h` — 常量和宏定义
- `common/pmp/pmp_cfg.h` — PMP 配置 API
- `common/platform/qemu_virtplatfrom_config.h` — QEMU virt 平台定义（`PLATFORM_MEM_BASE` 等）
