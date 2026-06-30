# RISC-V 虚拟内存测试计划

本文档定义 RISC-V Sv39/Sv48/Sv57 虚拟内存系统的测试计划，覆盖页表翻译算法、PTE 权限位、superpage 对齐、satp 模式切换、SUM/MXR 控制位、A/D 位管理、SFENCE.VMA 等核心规范点。

> **注意**：Sv 扩展（Svnapot、Svpbmt、Svadu、Svinval、Svvptc、Svrsw60t59b）暂不纳入本测试计划，后续将单独制定测试计划。

---

## 适用范围

- **Sv39**：3 级页表，39 位虚拟地址空间，支持 4KB / 2MB / 1GB 页
- **Sv48**：4 级页表，48 位虚拟地址空间，支持 4KB / 2MB / 1GB / 512GB 页
- **Sv57**：5 级页表，57 位虚拟地址空间，支持 4KB / 2MB / 1GB / 512GB / 256TB 页

---

## 规范引用

- `SPEC/supervisor.adoc` — Supervisor-Level ISA, Version 1.13
  - 4.1 Sv32: Page-Based 32-bit Virtual-Memory System
  - 4.3 Sv39: Page-Based 39-bit Virtual-Memory System
  - 4.4 Sv48: Page-Based 48-bit Virtual-Memory System
  - 4.5 Sv57: Page-Based 57-bit Virtual-Memory System
  - 4.1.11 Virtual Address Translation Process (`sv32algorithm`)
  - `sstatus` 寄存器：SUM、MXR 字段
  - `satp` 寄存器：MODE、ASID、PPN 字段
  - `sfence.vma` 指令

---

## 测试组定义

### Group 1：基础页表映射验证

**规范依据**：
- `norm:Sv39_leaf_any_level`：任何级别的 PTE 都可以是叶 PTE
- `norm:Sv39_page_sizes`：Sv39 支持 4KB、2MB megapage、1GB gigapage
- `norm:Sv48_page_sizes`：Sv48 额外支持 512GB terapage
- `norm:Sv57_page_sizes`：Sv57 额外支持 256TB petapage
- `norm:Sv39_LEVELS`：Sv39 LEVELS=3, PTESIZE=8
- `norm:Sv48_LEVELS`：Sv48 LEVELS=4, PTESIZE=8
- `norm:Sv57_LEVELS`：Sv57 LEVELS=5, PTESIZE=8

**测试职责**：验证各模式下不同页大小的恒等映射能正确完成读写操作。这是最基础的功能验证，确认页表翻译算法在各级别叶 PTE 上均能正确工作。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MAP-01 | Sv39 1GB gigapage 映射 | 建立 1GB gigapage 恒等映射，S-mode 下读写验证 | 读写成功，数据一致 |
| MAP-02 | Sv39 2MB megapage 映射 | 建立 2MB megapage 恒等映射，S-mode 下读写验证 | 读写成功，数据一致 |
| MAP-03 | Sv39 4KB page 映射 | 建立 4KB page 恒等映射，S-mode 下读写验证 | 读写成功，数据一致 |
| MAP-04 | Sv48 512GB terapage 映射 | 建立 512GB terapage 恒等映射，S-mode 下读写验证 | 读写成功，数据一致 |
| MAP-05 | Sv48 1GB gigapage 映射 | 建立 1GB gigapage 恒等映射，S-mode 下读写验证 | 读写成功，数据一致 |
| MAP-06 | Sv48 2MB megapage 映射 | 建立 2MB megapage 恒等映射，S-mode 下读写验证 | 读写成功，数据一致 |
| MAP-07 | Sv48 4KB page 映射 | 建立 4KB page 恒等映射，S-mode 下读写验证 | 读写成功，数据一致 |
| MAP-08 | Sv57 1GB gigapage 映射 | 建立 1GB gigapage 恒等映射，S-mode 下读写验证 | 读写成功，数据一致 |
| MAP-09 | Sv57 2MB megapage 映射 | 建立 2MB megapage 恒等映射，S-mode 下读写验证 | 读写成功，数据一致 |
| MAP-10 | Sv57 4KB page 映射 | 建立 4KB page 恒等映射，S-mode 下读写验证 | 读写成功，数据一致 |

```c
/* MAP-01 示例：Sv39 1GB gigapage 恒等映射 */
TEST_REGISTER(test_sv39_1g_identity);
bool test_sv39_1g_identity(void) {
    TEST_BEGIN("MAP-01: Sv39 1GB gigapage identity mapping");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                                        flags, PT_LEVEL_1G);
    TEST_ASSERT("identity mapping setup", ret == 0);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("S-mode read/write with 1G pages", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

> **注意**：MAP-04（512GB terapage）和 Sv57 的 petapage 在 QEMU virt 平台上可能受限于物理内存大小，需要根据平台实际情况决定是否跳过。

---

### Group 2：虚拟地址符号扩展

**规范依据**：
- `norm:Sv39_va_signext`：Sv39 要求 bits 63-39 全部等于 bit 38，否则触发 page-fault
- `norm:Sv48_va_signext`：Sv48 要求 bits 63-48 全部等于 bit 47，否则触发 page-fault
- `norm:Sv57_va_signext`：Sv57 要求 bits 63-57 全部等于 bit 56，否则触发 page-fault

**测试职责**：验证虚拟地址的高位符号扩展要求。当高位不满足符号扩展规则时，必须触发 page-fault exception。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SIGN-01 | Sv39 合法低地址 | 访问 VA=0x0000003FFFFFFFFF（bit 38=0，高位全 0） | 正常访问（如有映射） |
| SIGN-02 | Sv39 合法高地址 | 访问 VA=0xFFFFFFC000000000（bit 38=1，高位全 1） | 正常访问（如有映射） |
| SIGN-03 | Sv39 非法地址（高位不一致） | 访问 VA=0x0000004000000000（bit 38=0，bit 39=1） | page-fault exception |
| SIGN-04 | Sv48 合法低地址 | 访问 VA=0x00007FFFFFFFFFFF（bit 47=0，高位全 0） | 正常访问（如有映射） |
| SIGN-05 | Sv48 非法地址（高位不一致） | 访问 VA=0x0000800000000000（bit 47=0，bit 48=1） | page-fault exception |
| SIGN-06 | Sv57 合法低地址 | 访问 VA=0x00FFFFFFFFFFFFFF（bit 56=0，高位全 0） | 正常访问（如有映射） |
| SIGN-07 | Sv57 非法地址（高位不一致） | 访问 VA=0x0100000000000000（bit 56=0，bit 57=1） | page-fault exception |

```c
/* SIGN-03 示例：Sv39 非法虚拟地址 */
TEST_REGISTER(test_sv39_invalid_va_sign_ext);
bool test_sv39_invalid_va_sign_ext(void) {
    TEST_BEGIN("SIGN-03: Sv39 non-canonical VA triggers page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* 建立正常恒等映射 */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G, flags, PT_LEVEL_1G);

    /* 在 S-mode 下访问非规范地址 */
    /* bit 38=0 但 bit 39=1 => 非法 */
    uintptr_t bad_va = 0x0000004000000000UL;
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        bad_va);
    TEST_ASSERT("non-canonical VA triggers page fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 3：PTE 有效性检查

**规范依据**（翻译算法步骤 3）：
- 如果 `pte.v=0`，停止并触发 page-fault exception
- 如果 `pte.r=0` 且 `pte.w=1`，停止并触发 page-fault exception
- 如果 PTE 中设置了保留位或保留编码，停止并触发 page-fault exception

**测试职责**：验证页表翻译算法在遇到无效 PTE 时正确触发 page-fault。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VALID-01 | V=0 无效 PTE（load） | 映射页面但设置 V=0，S-mode 执行 load | load page-fault |
| VALID-02 | V=0 无效 PTE（store） | 映射页面但设置 V=0，S-mode 执行 store | store page-fault |
| VALID-03 | V=0 无效 PTE（fetch） | 映射页面但设置 V=0，S-mode 执行 instruction fetch | instruction page-fault |
| VALID-04 | R=0,W=1 保留编码（load） | 映射页面设置 R=0,W=1，S-mode 执行 load | load page-fault |
| VALID-05 | R=0,W=1 保留编码（store） | 映射页面设置 R=0,W=1，S-mode 执行 store | store page-fault |
| VALID-06 | R=0,W=1,X=0 保留编码 | 映射页面设置 W=1 但 R=0,X=0 | page-fault |
| VALID-07 | R=0,W=1,X=1 保留编码 | 映射页面设置 W=1,X=1 但 R=0 | page-fault |

```c
/* VALID-01 示例：V=0 无效 PTE */
TEST_REGISTER(test_pte_v0_load_fault);
bool test_pte_v0_load_fault(void) {
    TEST_BEGIN("VALID-01: V=0 PTE triggers load page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* 建立正常恒等映射用于代码执行 */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G, flags, PT_LEVEL_1G);

    /* 映射测试页面但 V=0（无效） */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_R | PTE_W | PTE_A | PTE_D,  /* 注意：没有 PTE_V */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("V=0 triggers load page fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

/* VALID-04 示例：R=0,W=1 保留编码 */
TEST_REGISTER(test_pte_r0w1_fault);
bool test_pte_r0w1_fault(void) {
    TEST_BEGIN("VALID-04: R=0,W=1 reserved encoding triggers page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G, flags, PT_LEVEL_1G);

    /* 映射测试页面：V=1, R=0, W=1 => 保留编码 */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_W | PTE_A | PTE_D,  /* R=0, W=1 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("R=0,W=1 triggers page fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 4：PTE 权限位 (RWX) 组合

**规范依据**（翻译算法步骤 5、8）：
- 叶 PTE 由 `pte.r=1` 或 `pte.x=1` 标识
- 步骤 8：根据 `pte.r`、`pte.w`、`pte.x` 判断请求的内存访问是否被允许，不允许则触发 page-fault

**测试职责**：系统性验证 7 种合法 RWX 权限组合（排除 R=0,W=1 的保留编码）下，S-mode 的读/写/执行访问控制行为。

| 测试 ID | 权限组合 | S-mode 读 | S-mode 写 | S-mode 执行 |
|---------|---------|-----------|-----------|-------------|
| RWX-01 | R only | 成功 | store page-fault | instruction page-fault |
| RWX-02 | R+W | 成功 | 成功 | instruction page-fault |
| RWX-03 | X only | load page-fault | store page-fault | 成功 |
| RWX-04 | R+X | 成功 | store page-fault | 成功 |
| RWX-05 | R+W+X | 成功 | 成功 | 成功 |
| RWX-06 | W+X（R=0,W=1,X=1） | page-fault（保留编码） | page-fault | page-fault |
| RWX-07 | W only（R=0,W=1,X=0） | page-fault（保留编码） | page-fault | page-fault |

> **注意**：RWX-06 和 RWX-07 是保留编码（R=0,W=1），与 Group 3 的 VALID-06/07 重叠，此处列出以保证 RWX 组合的完整性。

```c
/* RWX-01 示例：R only 权限 */
TEST_REGISTER(test_pte_r_only);
bool test_pte_r_only(void) {
    TEST_BEGIN("RWX-01: R-only page, read succeeds, write/exec faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* 恒等映射用于代码执行 */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：R only */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 读：成功 */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("R-only: read succeeds", result == 0);

    /* 写：store page-fault */
    result = vm_run_in_smode(&ctx, test_smode_store_expect_fault, test_va);
    TEST_ASSERT("R-only: write faults", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 5：U 位与特权级访问控制

**规范依据**（翻译算法步骤 6）：
- 根据 `pte.u` 位和当前特权模式以及 `mstatus.SUM`、`mstatus.MXR` 字段判断访问是否被允许
- U=1 的页面：U-mode 可访问；S-mode 默认不可访问（除非 SUM=1）
- U=0 的页面：S-mode 可访问；U-mode 不可访问
- S-mode 永远不能从 U=1 的页面执行指令，无论 SUM 的值

**测试职责**：验证 U 位对不同特权级访问的控制效果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| UPRIV-01 | U=1 页面 U-mode 读写 | U=1 页面，U-mode 执行 load/store | 读写成功 |
| UPRIV-02 | U=1 页面 U-mode 执行 | U=1 页面，U-mode 执行 instruction fetch | 执行成功 |
| UPRIV-03 | U=1 页面 S-mode 读（SUM=0） | U=1 页面，SUM=0，S-mode 执行 load | load page-fault |
| UPRIV-04 | U=1 页面 S-mode 写（SUM=0） | U=1 页面，SUM=0，S-mode 执行 store | store page-fault |
| UPRIV-05 | U=1 页面 S-mode 执行 | U=1 页面，S-mode 执行 instruction fetch（任何 SUM 值） | instruction page-fault |
| UPRIV-06 | U=0 页面 S-mode 读写 | U=0 页面，S-mode 执行 load/store | 读写成功 |
| UPRIV-07 | U=0 页面 S-mode 执行 | U=0 页面，S-mode 执行 instruction fetch | 执行成功 |
| UPRIV-08 | U=0 页面 U-mode 读 | U=0 页面，U-mode 执行 load | load page-fault |
| UPRIV-09 | U=0 页面 U-mode 写 | U=0 页面，U-mode 执行 store | store page-fault |
| UPRIV-10 | U=0 页面 U-mode 执行 | U=0 页面，U-mode 执行 instruction fetch | instruction page-fault |

```c
/* UPRIV-03 示例：U=1 页面 S-mode 读（SUM=0） */
TEST_REGISTER(test_u1_smode_read_sum0);
bool test_u1_smode_read_sum0(void) {
    TEST_BEGIN("UPRIV-03: U=1 page, S-mode read with SUM=0 faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* 恒等映射用于代码执行（S-mode 页面） */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：U=1 */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 确保 SUM=0 */
    clear_csr(sstatus, SSTATUS_SUM);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("U=1 page, SUM=0: S-mode read faults", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 6：SUM 位控制

**规范依据**：
- `norm:sstatus_sum`：SUM=0 时，S-mode 访问 U=1 页面会触发 fault；SUM=1 时允许访问
- `norm:sstatus_sum`：SUM 对 U-mode 无效
- `norm:sstatus_sum`：S-mode 永远不能从 U=1 页面执行指令，无论 SUM 值
- `norm:sstatus_sum_satp_mode`：如果 `satp.MODE` 为只读 0，则 SUM 为只读 0

**测试职责**：验证 SUM 位对 S-mode 访问 U-mode 页面的控制效果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SUM-01 | SUM=1 S-mode 读 U 页面 | SUM=1，S-mode 读 U=1 页面 | 读成功 |
| SUM-02 | SUM=1 S-mode 写 U 页面 | SUM=1，S-mode 写 U=1 页面 | 写成功 |
| SUM-03 | SUM=0 S-mode 读 U 页面 | SUM=0，S-mode 读 U=1 页面 | load page-fault |
| SUM-04 | SUM=0 S-mode 写 U 页面 | SUM=0，S-mode 写 U=1 页面 | store page-fault |
| SUM-05 | SUM=1 S-mode 执行 U 页面 | SUM=1，S-mode 从 U=1 页面执行 | instruction page-fault |
| SUM-06 | SUM 对 U-mode 无效 | SUM=0，U-mode 读 U=1 页面 | 读成功（SUM 不影响 U-mode） |
| SUM-07 | SUM 对 S-mode 非 U 页面无效 | SUM=0/1，S-mode 读 U=0 页面 | 读成功（SUM 仅影响 U=1 页面） |

```c
/* SUM-01 示例：SUM=1 S-mode 读 U 页面 */
TEST_REGISTER(test_sum1_smode_read_u_page);
bool test_sum1_smode_read_u_page(void) {
    TEST_BEGIN("SUM-01: SUM=1, S-mode read U-page succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：U=1 */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 设置 SUM=1 */
    set_csr(sstatus, SSTATUS_SUM);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("SUM=1: S-mode read U-page succeeds", result == 0);

    clear_csr(sstatus, SSTATUS_SUM);
    pt_pool_reset();
    TEST_END();
}
```

---

### Group 7：MXR 位控制

**规范依据**：
- `norm:sstatus_mxr`：MXR=0 时，只有 R=1 的页面 load 才能成功
- `norm:sstatus_mxr`：MXR=1 时，R=1 或 X=1 的页面 load 都能成功
- `norm:sstatus_mxr`：MXR 在未启用页表虚拟内存时无效

**测试职责**：验证 MXR 位对 load 操作的权限扩展效果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MXR-01 | MXR=0 读 X-only 页面 | MXR=0，S-mode 读 X-only（R=0,X=1）页面 | load page-fault |
| MXR-02 | MXR=1 读 X-only 页面 | MXR=1，S-mode 读 X-only（R=0,X=1）页面 | 读成功 |
| MXR-03 | MXR=1 读 R 页面 | MXR=1，S-mode 读 R=1 页面 | 读成功（无变化） |
| MXR-04 | MXR=1 写 X-only 页面 | MXR=1，S-mode 写 X-only 页面 | store page-fault（MXR 不影响写） |
| MXR-05 | MXR=0 读 R+X 页面 | MXR=0，S-mode 读 R=1,X=1 页面 | 读成功（R=1 即可） |

```c
/* MXR-02 示例：MXR=1 读 X-only 页面 */
TEST_REGISTER(test_mxr1_read_xonly);
bool test_mxr1_read_xonly(void) {
    TEST_BEGIN("MXR-02: MXR=1, read X-only page succeeds");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：X-only（R=0, X=1） */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_X | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 设置 MXR=1 */
    set_csr(sstatus, SSTATUS_MXR);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("MXR=1: read X-only page succeeds", result == 0);

    clear_csr(sstatus, SSTATUS_MXR);
    pt_pool_reset();
    TEST_END();
}
```

---

### Group 8：Superpage 对齐验证

**规范依据**（翻译算法步骤 5）：
- `norm:Sv39_superpage_align`：superpage 必须虚拟和物理对齐到等于其大小的边界
- `norm:Sv39_superpage_align_fault`：如果物理地址对齐不足，触发 page-fault exception
- 步骤 5：如果 i>0 且 `pte.ppn[i-1:0] != 0`，这是一个 misaligned superpage，触发 page-fault

**测试职责**：验证 misaligned superpage 正确触发 page-fault。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ALIGN-01 | Sv39 misaligned 2MB megapage | 2MB megapage 的 ppn[0] 非零（PA 未对齐 2MB） | page-fault |
| ALIGN-02 | Sv39 misaligned 1GB gigapage | 1GB gigapage 的 ppn[1:0] 非零（PA 未对齐 1GB） | page-fault |
| ALIGN-03 | Sv48 misaligned 512GB terapage | 512GB terapage 的 ppn[2:0] 非零 | page-fault |
| ALIGN-04 | Sv39 aligned 2MB megapage | 2MB megapage PA 正确对齐 | 正常访问 |
| ALIGN-05 | Sv39 aligned 1GB gigapage | 1GB gigapage PA 正确对齐 | 正常访问 |

```c
/* ALIGN-01 示例：misaligned 2MB megapage */
TEST_REGISTER(test_misaligned_2m_megapage);
bool test_misaligned_2m_megapage(void) {
    TEST_BEGIN("ALIGN-01: Misaligned 2MB megapage triggers page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* 恒等映射用于代码执行 */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /*
     * 手动构造 misaligned megapage：
     * VA 对齐到 2MB，但 PA 的 ppn[0] 非零（PA 未对齐 2MB）
     */
    uintptr_t va = TEST_REGION_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t pa_misaligned = va + PAGE_SIZE_4K;  /* 偏移 4KB，ppn[0] != 0 */
    pt_map_page(&ctx, va, pa_misaligned,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault, va);
    TEST_ASSERT("misaligned megapage triggers page fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 9：多级页表遍历

**规范依据**（翻译算法步骤 2、4）：
- 步骤 2：通过 `a + va.vpn[i] * PTESIZE` 计算 PTE 地址
- 步骤 4：如果 PTE 不是叶节点（R=0 且 X=0），则为指向下一级页表的指针；令 i=i-1，若 i<0 则触发 page-fault
- 步骤 2 中如果访问 PTE 违反 PMA 或 PMP 检查，触发 access-fault

**测试职责**：验证多级页表遍历的正确性，包括深度限制和非叶 PTE 的处理。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| WALK-01 | Sv39 三级完整遍历 | 建立 3 级页表（根→L1→L0→叶 4KB PTE），验证读写 | 正常访问 |
| WALK-02 | Sv48 四级完整遍历 | 建立 4 级页表（根→L2→L1→L0→叶 4KB PTE），验证读写 | 正常访问 |
| WALK-03 | Sv57 五级完整遍历 | 建立 5 级页表（根→L3→L2→L1→L0→叶 4KB PTE），验证读写 | 正常访问 |
| WALK-04 | 非叶 PTE 链过深（i<0） | Sv39 中所有 3 级 PTE 都是非叶（R=0,X=0），遍历到 i<0 | page-fault |
| WALK-05 | 中间级叶 PTE（megapage） | Sv39 在 L1 级设置叶 PTE（2MB megapage） | 正常访问 |
| WALK-06 | PTE 访问违反 PMP | 页表页位于 PMP 禁止区域 | access-fault |

```c
/* WALK-04 示例：非叶 PTE 链过深 */
TEST_REGISTER(test_walk_too_deep);
bool test_walk_too_deep(void) {
    TEST_BEGIN("WALK-04: Non-leaf PTE chain exceeds depth limit");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* 恒等映射用于代码执行 */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /*
     * 手动构造：在最后一级（L0）放置非叶 PTE（R=0,X=0）
     * 翻译算法会尝试 i=i-1，此时 i<0，触发 page-fault
     */
    uintptr_t test_va = TEST_REGION_BASE;
    /* 构造一个指向另一个页表页的非叶 PTE 放在 L0 级 */
    uintptr_t dummy_pt = (uintptr_t)pt_alloc_page();
    pt_map_page_raw(&ctx, test_va, dummy_pt,
                    PTE_V,  /* V=1, R=0, X=0 => 非叶 */
                    PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("non-leaf at last level triggers page fault",
                result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 10：A/D 位管理

**规范依据**（翻译算法步骤 9）：
- 如果 `pte.a=0`，或者原始内存访问是 store 且 `pte.d=0`：
  - 如果实现了 Svade 扩展：停止并触发 page-fault
  - 否则：硬件原子更新 A/D 位
- A 和 D 位永远不会被实现清除
- 建议软件始终设置 A=1, D=1 以提高性能

**测试职责**：验证 A/D 位在不同状态下的行为。在 QEMU 上默认实现 Svade 语义（A=0 或 D=0 时触发 page-fault）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| AD-01 | A=0 load 触发 page-fault | 映射页面 A=0,D=1，S-mode 执行 load | load page-fault（Svade） |
| AD-02 | A=0 store 触发 page-fault | 映射页面 A=0,D=1，S-mode 执行 store | store page-fault（Svade） |
| AD-03 | A=1,D=0 store 触发 page-fault | 映射页面 A=1,D=0，S-mode 执行 store | store page-fault（Svade） |
| AD-04 | A=1,D=0 load 成功 | 映射页面 A=1,D=0，S-mode 执行 load | 读成功（D 位不影响 load） |
| AD-05 | A=1,D=1 正常读写 | 映射页面 A=1,D=1，S-mode 执行 load/store | 读写成功 |
| AD-06 | A=0 instruction fetch | 映射页面 A=0，S-mode 执行 instruction fetch | instruction page-fault（Svade） |

> **注意**：A/D 位行为取决于是否实现 Svade 扩展。QEMU virt 平台默认实现 Svade（A=0 或 D=0 时触发 page-fault 而非硬件更新）。如果平台支持硬件 A/D 更新（Svadu），则 AD-01~AD-03 和 AD-06 的预期结果应为硬件自动设置 A/D 位而非触发 fault。

```c
/* AD-01 示例：A=0 load 触发 page-fault */
TEST_REGISTER(test_ad_a0_load_fault);
bool test_ad_a0_load_fault(void) {
    TEST_BEGIN("AD-01: A=0 load triggers page fault (Svade)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：A=0, D=1 */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_D,  /* A=0 */
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("A=0 triggers load page fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 11：satp CSR 控制

**规范依据**：
- `satp.MODE`：控制地址翻译模式（Bare/Sv39/Sv48/Sv57）
- `satp.ASID`：地址空间标识符
- `satp.PPN`：根页表的物理页号
- `norm:Sv48_requires_Sv39`：实现 Sv48 必须同时支持 Sv39
- `norm:Sv57_requires_Sv48`：实现 Sv57 必须同时支持 Sv48
- satp 必须在 active 状态（effective privilege 为 S-mode 或 U-mode）时才生效

**测试职责**：验证 satp CSR 各字段的功能和模式切换行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SATP-01 | MODE=Bare 禁用 VM | 设置 satp.MODE=0（Bare），S-mode 访问物理地址 | 直接物理地址访问，无翻译 |
| SATP-02 | MODE=Sv39 启用 VM | 设置 satp.MODE=8（Sv39），验证地址翻译 | 通过 Sv39 页表翻译 |
| SATP-03 | MODE=Sv48 启用 VM | 设置 satp.MODE=9（Sv48），验证地址翻译 | 通过 Sv48 页表翻译 |
| SATP-04 | MODE=Sv57 启用 VM | 设置 satp.MODE=10（Sv57），验证地址翻译 | 通过 Sv57 页表翻译 |
| SATP-05 | 模式切换 Sv39→Sv48 | 从 Sv39 切换到 Sv48，重建页表后验证 | 切换成功，翻译正确 |
| SATP-06 | 模式切换 Sv48→Sv57 | 从 Sv48 切换到 Sv57，重建页表后验证 | 切换成功，翻译正确 |
| SATP-07 | ASID 基本功能 | 设置不同 ASID 值，验证 satp 写入读回 | ASID 值正确保留 |
| SATP-08 | PPN 字段验证 | 设置根页表 PPN，验证翻译使用正确的根页表 | 使用指定根页表翻译 |
| SATP-09 | 不支持的 MODE 值 | 写入不支持的 MODE 值（WARL 字段） | MODE 保持合法值 |

```c
/* SATP-05 示例：模式切换 Sv39→Sv48 */
TEST_REGISTER(test_satp_mode_switch_sv39_sv48);
bool test_satp_mode_switch_sv39_sv48(void) {
    TEST_BEGIN("SATP-05: Mode switch Sv39 -> Sv48");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G, flags, PT_LEVEL_1G);

    /* Sv39 下验证 */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("Sv39 read/write succeeds", result == 0);

    /* 切换到 Sv48 */
    vm_switch_mode(&ctx, SATP_MODE_SV48);

    /* Sv48 下验证 */
    result = vm_run_in_smode(&ctx, test_smode_read_write,
                              (uintptr_t)test_data_area);
    TEST_ASSERT("Sv48 read/write succeeds after switch", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 12：SFENCE.VMA 指令

**规范依据**：
- `sfence.vma` 用于刷新地址翻译缓存（TLB）
- rs1=x0, rs2=x0：刷新所有 TLB 条目
- rs1≠x0, rs2=x0：刷新指定虚拟地址的 TLB 条目（所有 ASID）
- rs1=x0, rs2≠x0：刷新指定 ASID 的所有 TLB 条目
- rs1≠x0, rs2≠x0：刷新指定虚拟地址和 ASID 的 TLB 条目
- 修改页表后必须执行 sfence.vma 才能保证后续访问使用新的翻译

**测试职责**：验证 SFENCE.VMA 指令的 TLB 刷新效果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SFENCE-01 | 全局刷新（rs1=x0, rs2=x0） | 修改 PTE 权限后执行全局 sfence.vma，验证新权限生效 | 新权限生效 |
| SFENCE-02 | 按地址刷新（rs1≠x0） | 修改特定地址的 PTE 后按地址刷新，验证该地址新权限生效 | 指定地址新权限生效 |
| SFENCE-03 | 不刷新时旧翻译可能仍有效 | 修改 PTE 但不执行 sfence.vma | 行为不确定（可能使用旧翻译） |
| SFENCE-04 | 映射变更后刷新 | 将有效映射改为无效（V=0），执行 sfence.vma 后访问 | page-fault |
| SFENCE-05 | 权限升级后刷新 | 将 R-only 页面升级为 RW，执行 sfence.vma 后写入 | 写入成功 |

```c
/* SFENCE-01 示例：全局刷新 */
TEST_REGISTER(test_sfence_vma_global);
bool test_sfence_vma_global(void) {
    TEST_BEGIN("SFENCE-01: Global sfence.vma flushes TLB");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面：R only */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 验证 R-only：写应失败 */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store_expect_fault,
                                        test_va);
    TEST_ASSERT("R-only: write faults", result == CAUSE_SPF);

    /* 修改 PTE 为 RW */
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    /* 执行全局 sfence.vma */
    vm_sfence_vma(0, 0);

    /* 验证 RW：写应成功 */
    result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("After sfence.vma: write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 13：PTE 保留位检查

**规范依据**：
- `norm:Sv39_pte_future_rsv`：Sv39/Sv48/Sv57 PTE 的 bits 60-54 保留，必须为零，否则触发 page-fault
- `norm:Sv39_pte_svnapot_rsv`：如果未实现 Svnapot，bit 63 保留，必须为零
- `norm:Sv39_pte_svpbmt_rsv`：如果未实现 Svpbmt，bits 62-61 保留，必须为零

**测试职责**：验证 PTE 保留位非零时正确触发 page-fault。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| RSVD-01 | bits 60-54 非零 | 设置 PTE 的 bit 54 为 1 | page-fault |
| RSVD-02 | bits 60-54 多位非零 | 设置 PTE 的 bits 60-54 全为 1 | page-fault |
| RSVD-03 | bit 63 非零（无 Svnapot） | 设置 PTE 的 bit 63 为 1（未实现 Svnapot 时） | page-fault |
| RSVD-04 | bits 62-61 非零（无 Svpbmt） | 设置 PTE 的 bits 62-61 为非零（未实现 Svpbmt 时） | page-fault |
| RSVD-05 | 所有保留位为零 | PTE 保留位全部为零 | 正常访问 |

```c
/* RSVD-01 示例：bits 60-54 非零 */
TEST_REGISTER(test_pte_reserved_bits_54);
bool test_pte_reserved_bits_54(void) {
    TEST_BEGIN("RSVD-01: PTE reserved bit 54 set triggers page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射测试页面，手动设置保留位 bit 54 */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D
                          | (1UL << 54);  /* 保留位 */
    pt_map_page(&ctx, test_va, test_va, pte_flags, PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("reserved bit 54 triggers page fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
```

---

### Group 14：非叶 PTE 保留字段

**规范依据**：
- 对于非叶 PTE，D、A 和 U 位保留供未来标准使用
- 在其用途被标准扩展定义之前，软件必须将它们清零以保持前向兼容性

**测试职责**：验证非叶 PTE 中 D/A/U 位非零时的行为。

> **注意**：规范要求软件将非叶 PTE 的 D/A/U 位清零，但未明确规定硬件在这些位非零时的行为（是否触发 page-fault）。不同实现可能有不同行为。本组测试验证实现的实际行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| NLPTE-01 | 非叶 PTE D=1 | 在非叶 PTE 中设置 D=1 | 实现定义（可能正常或 page-fault） |
| NLPTE-02 | 非叶 PTE A=1 | 在非叶 PTE 中设置 A=1 | 实现定义（可能正常或 page-fault） |
| NLPTE-03 | 非叶 PTE U=1 | 在非叶 PTE 中设置 U=1 | 实现定义（可能正常或 page-fault） |
| NLPTE-04 | 非叶 PTE D=A=U=0 | 非叶 PTE 的 D/A/U 位全部为 0（合规配置） | 正常遍历 |

```c
/* NLPTE-04 示例：非叶 PTE D=A=U=0（合规配置） */
TEST_REGISTER(test_nonleaf_pte_dau_zero);
bool test_nonleaf_pte_dau_zero(void) {
    TEST_BEGIN("NLPTE-04: Non-leaf PTE with D=A=U=0 works correctly");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* 使用 4KB 页映射，确保存在非叶 PTE */
    uintptr_t base = PLATFORM_MEM_BASE;
    uintptr_t size = 2 * PAGE_SIZE_2M;
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_setup_identity_mapping(&ctx, base, size,
                                        flags, PT_LEVEL_4K);
    TEST_ASSERT("identity mapping setup", ret == 0);

    /* 验证非叶 PTE 的 D/A/U 位为 0 时正常工作 */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        (uintptr_t)test_data_area);
    TEST_ASSERT("non-leaf PTE with D=A=U=0 works", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1（基础映射）、Group 3（PTE 有效性）、Group 4（RWX 权限） | MAP-01~10、VALID-01~07、RWX-01~07 | 核心翻译和访问控制机制 |
| P1（重要） | Group 5（U 位）、Group 6（SUM）、Group 7（MXR）、Group 8（Superpage 对齐） | UPRIV-01~10、SUM-01~07、MXR-01~05、ALIGN-01~05 | 特权级访问控制和 superpage 正确性 |
| P2（建议） | Group 9（多级遍历）、Group 10（A/D 位）、Group 11（satp 控制）、Group 12（SFENCE.VMA） | WALK-01~06、AD-01~06、SATP-01~09、SFENCE-01~05 | 页表遍历、A/D 位和 TLB 管理 |
| P3（可选） | Group 2（符号扩展）、Group 13（保留位）、Group 14（非叶 PTE 保留字段） | SIGN-01~07、RSVD-01~05、NLPTE-01~04 | 边界和保留位检查 |

---

## 测试实现说明

### 文件组织

每个 Sv 模式（Sv39/Sv48/Sv57）共享同一套测试逻辑，通过参数化区分模式。建议的文件组织如下：

```
sv39/
├── Makefile
├── kernel.ld
└── main.c              # Sv39 测试用例入口

sv48/
├── Makefile
├── kernel.ld
└── main.c              # Sv48 测试用例入口

sv57/
├── Makefile
├── kernel.ld
└── main.c              # Sv57 测试用例入口

common/vm/
├── vm_defs.h           # 常量定义
├── vm.h                # API 声明
├── page_table.c        # 页表管理
└── satp.c              # satp 控制和 vm_run_in_smode
```

### 通用测试模式

每个测试用例遵循以下模式：

```c
#include "test_framework.h"
#include "vm/vm.h"

/* S-mode 下执行的测试函数 */
static uintptr_t test_smode_xxx(uintptr_t arg) {
    /* 在 S-mode + VM 启用状态下执行 */
    return 0;  /* 成功 */
}

TEST_REGISTER(test_xxx);
bool test_xxx(void) {
    TEST_BEGIN("ID: description");

    /* 1. 重置页表池并初始化上下文 */
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);  /* 或 SV48 / SV57 */

    /* 2. 设置恒等映射（用于代码执行） */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              flags, PT_LEVEL_1G);

    /* 3. 映射测试页面（可选，用于验证特定权限/属性） */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_A | PTE_D,  /* 特定权限 */
                PT_LEVEL_4K);

    /* 4. 在 S-mode + VM 下执行测试 */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_xxx,
                                        (uintptr_t)test_data);
    TEST_ASSERT("description", result == expected);

    /* 5. 清理 */
    pt_pool_reset();
    TEST_END();
}
```

### S-mode 测试辅助函数

测试计划中引用的 S-mode 辅助函数说明：

| 函数名 | 功能 | 返回值 |
|--------|------|--------|
| `test_smode_read_write` | 写入 magic value 并读回验证 | 0=成功，非 0=失败 |
| `test_smode_load` | 执行 load 操作 | 0=成功 |
| `test_smode_store` | 执行 store 操作 | 0=成功 |
| `test_smode_load_expect_fault` | 执行 load，预期触发 fault | fault cause（如 CAUSE_LPF） |
| `test_smode_store_expect_fault` | 执行 store，预期触发 fault | fault cause（如 CAUSE_SPF） |
| `test_smode_exec_expect_fault` | 跳转执行，预期触发 fault | fault cause（如 CAUSE_IPF） |

### 关键注意事项

1. **恒等映射覆盖范围**：测试中的恒等映射必须覆盖代码段、数据段、栈、页表池和 UART I/O 区域。使用 `pt_setup_identity_mapping()` 会自动映射 UART。

2. **PTE_A 和 PTE_D 位**：除 Group 10（A/D 位管理）的测试外，其他测试应始终设置 `PTE_A | PTE_D`，避免因 Svade 语义导致不必要的 page-fault。

3. **测试页面隔离**：验证权限/属性的测试页面应与代码执行区域分开映射，使用独立的 `TEST_REGION_BASE` 地址。

4. **Page-fault cause 编码**：
   - `CAUSE_IPF` (12)：Instruction page-fault
   - `CAUSE_LPF` (13)：Load page-fault
   - `CAUSE_SPF` (15)：Store/AMO page-fault

5. **PMP 与虚拟内存**：`vm_run_in_smode()` 自动配置 PMP entry 15 为全地址空间 NAPOT RWX。如需测试 PMP 与 VM 的交互，需手动管理 PMP 配置。

6. **Sv 模式兼容性**：
   - 实现 Sv48 必须同时支持 Sv39（`norm:Sv48_requires_Sv39`）
   - 实现 Sv57 必须同时支持 Sv48（`norm:Sv57_requires_Sv48`）
   - 测试应验证这些兼容性要求

7. **QEMU 平台限制**：
   - QEMU virt 平台默认实现 Svade（A=0 或 D=0 时触发 page-fault）
   - 512GB terapage 和 256TB petapage 测试可能受限于物理内存大小
   - 部分保留位检查行为可能因 QEMU 版本而异

---

## 参考

- RISC-V Privileged Specification — Supervisor-Level ISA (`SPEC/supervisor.adoc`)
- `SPEC/machine.adoc` — Machine-Level ISA（satp CSR 定义）
- `docs/vm_test_framework.md` — 虚拟内存测试框架文档
- `common/vm/vm.h` — VM API 声明
- `common/vm/vm_defs.h` — 常量和宏定义
