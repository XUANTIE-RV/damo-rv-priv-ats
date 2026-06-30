**中文 | [English](../testplan_en/Svnapot_test_plan_en.md)**

## RISC-V Svnapot 合规性测试方案

**规范依据**：RISC-V Privileged Specification, "Svnapot" Extension for NAPOT Translation Contiguity, Version 1.0 (`SPEC/svnapot.adoc`)

---

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svnapot_pte_N` | N=1 | 当 PTE 的 N=1 时，该 PTE 表示一段连续虚拟到物理转换范围的一部分。 |
| `norm:Svnapot_range_napot` | Such ranges must be of a naturally aligned power-of-2 (NAPOT) granularity larger than the base page size. | 此类范围必须是大于基本页面大小的自然对齐的 2 的幂次方（NAPOT）粒度。 |
| `norm:Svnapot_depends_Sv39` | The Svnapot extension depends on the Sv39 extension. | Svnapot 扩展依赖于 Sv39 扩展。 |
| `norm:Svnapot_valid_encoding` | valid according to <<ptenapot>> | 如果 PTE 编码根据 <<ptenapot>> 有效。 |
| `norm:Svnapot_implicit_read_ppn_subst` | implicit reads of a NAPOT PTE return a copy of _pte_ in which __pte__.__ppn__[__i__][__pte__.__napot_bits__-1:0] is replaced by __vpn__[__i__][__pte__.__napot_bits__-1:0]. | NAPOT PTE 的隐式读取返回 PTE 的副本，其中 __pte__.__ppn__[__i__][__pte__.__napot_bits__-1:0] 被替换为 __vpn__[__i__][__pte__.__napot_bits__-1:0]。 |
| `norm:Svnapot_reserved_encoding_fault` | reserved according to <<ptenapot>>, then a page-fault exception must be raised. | 如果 PTE 编码根据 <<ptenapot>> 是保留的，则必须触发页错误异常。 |
| `norm:Svnapot_cache_entries` | Implicit reads of NAPOT page table entries may create address-translation cache entries mapping _a_ + _j_×PTESIZE to a copy of _pte_ in which _pte_._ppn_[_i_][_pte_.__napot_bits__-1:0] is replaced by _vpn[i][pte.napot_bits_-1:0], for any or all _j_ such that __j__ >> __napot_bits__ = __vpn__[__i__] >> __napot_bits__, all for the address space identified in _satp_ as loaded by step 1. | NAPOT 页表条目的隐式读取可以创建地址转换缓存条目，将 _a_ + _j_×PTESIZE 映射到 PTE 的副本（其中 PPN 的低位被 VPN 的对应位替换），适用于满足 __j__ >> __napot_bits__ = __vpn__[__i__] >> __napot_bits__ 的任意或所有 _j_。 |
| `norm:Svnapot_hyp_gstage` | Svnapot is also supported in G-stage translation. | Svnapot 在 G 阶段转换中也得到支持。 |

---

### 测试目标

验证 RISC-V 处理器的 Svnapot 扩展实现是否符合规范，重点覆盖：

1. PTE N 位的基本功能：N=1 时启用 NAPOT 翻译连续性
2. 64 KiB NAPOT 页面的有效编码和地址翻译正确性
3. 保留编码（Reserved encoding）触发 page-fault 异常
4. NAPOT PTE 的 PPN 替换语义（ppn[i] 低位由 vpn[i] 替换）
5. NAPOT 页面的 RWX 权限控制
6. NAPOT 页面与 SFENCE.VMA 的交互
7. Svnapot 对 Sv39 扩展的依赖关系
8. NAPOT 页面的 A/D 位行为及 NAPOT 别名 A/D 位独立性
9. 多模式支持（Sv39/Sv48/Sv57）
10. PTE bits 5-0 一致性与 NAPOT 别名行为
11. PTE RSW 位在 NAPOT PTE 中的保留语义
12. NAPOT 区域边界对齐验证
13. PBMT 与 NAPOT 的交互（条件性，依赖 Svpbmt 扩展）

---

### 测试分组

---

#### Group 1：64 KiB NAPOT 页面基本映射

**规范依据**：
- `norm:Svnapot_pte_N`：N=1 时，PTE 表示一个连续虚拟到物理翻译范围的一部分
- `norm:Svnapot_range_napot`：连续范围必须是自然对齐的 2 的幂次方粒度，且大于基本页大小
- `norm:Svnapot_valid_encoding`：当 i=0 且 pte.ppn[0] 编码为 `x xxxx 1000` 时，表示 64 KiB 连续区域，napot_bits=4
- `norm:Svnapot_implicit_read_ppn_subst`：隐式读取 NAPOT PTE 时，pte.ppn[i][napot_bits-1:0] 被替换为 vpn[i][napot_bits-1:0]

**测试职责**：验证 64 KiB NAPOT 页面的基本地址翻译功能。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| NAPOT64-01 | 64 KiB NAPOT 页面读访问 | 配置 N=1、ppn[0]=`xxxx1000` 的 NAPOT PTE，映射 64 KiB 连续区域，S-mode 读取区域首地址 | 读成功，地址翻译正确 |
| NAPOT64-02 | 64 KiB NAPOT 页面写访问 | 同上配置，S-mode 写入区域首地址 | 写成功 |
| NAPOT64-03 | 64 KiB NAPOT 页面末地址访问 | S-mode 读取 64 KiB 区域最后一个字节 | 读成功 |
| NAPOT64-04 | 64 KiB NAPOT 区域内多偏移访问 | 分别访问 64 KiB 区域内偏移 0x0000、0x1000、0x8000、0xF000 处 | 所有访问成功，翻译到对应物理地址 |
| NAPOT64-05 | 64 KiB NAPOT 区域外访问 | 访问 NAPOT 区域外第一个字节（base + 64 KiB） | page-fault（无映射） |
| NAPOT64-06 | 多个 NAPOT 64 KiB 区域 | 配置两个相邻的 64 KiB NAPOT 区域，分别访问 | 两个区域均正确翻译 |

```c
/* NAPOT64-01 示例：64 KiB NAPOT 页面读访问 */
TEST_REGISTER(test_napot64_read);
bool test_napot64_read(void) {
    TEST_BEGIN("NAPOT64-01: 64 KiB NAPOT page read access");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 映射 64 KiB NAPOT 区域：
     * VA = test_va (64 KiB 对齐), PA = test_va
     * PTE: N=1, ppn[0] 编码为 xxxx1000 (napot_bits=4)
     * 需要手动构造 NAPOT PTE 并写入页表 */
    uintptr_t test_va = TEST_REGION_BASE;  /* 必须 64 KiB 对齐 */
    uintptr_t napot_pte = napot_make_pte(test_va, test_va,
                                          PTE_V | PTE_R | PTE_A | PTE_D,
                                          NAPOT_64K);
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("64 KiB NAPOT read succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

#### Group 2：PPN 替换语义验证

**规范依据**：
- `norm:Svnapot_implicit_read_ppn_subst`：隐式读取 NAPOT PTE 返回一个副本，其中 pte.ppn[i][napot_bits-1:0] 被替换为 vpn[i][napot_bits-1:0]

**测试职责**：验证 NAPOT PTE 的物理地址翻译正确地将 ppn 低位替换为 vpn 低位。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PPN-01 | PPN 替换基本验证 | 64 KiB NAPOT 区域内，写入不同 4 KiB 页面的 magic value，读回验证物理地址正确 | 每个 4 KiB 页面写入独立的物理位置 |
| PPN-02 | 区域内偏移 0x0 翻译验证 | 访问 NAPOT 区域 VA 偏移 0x0，验证翻译到 PA 偏移 0x0 | PA = base_pa + 0x0 |
| PPN-03 | 区域内偏移 0x4000 翻译验证 | 访问 NAPOT 区域 VA 偏移 0x4000，验证翻译到 PA 偏移 0x4000 | PA = base_pa + 0x4000 |
| PPN-04 | 区域内偏移 0xF000 翻译验证 | 访问 NAPOT 区域 VA 偏移 0xF000，验证翻译到 PA 偏移 0xF000 | PA = base_pa + 0xF000 |
| PPN-05 | 非恒等映射 PPN 替换 | VA 和 PA 不同的 64 KiB NAPOT 映射，验证区域内各偏移翻译到正确 PA | 各偏移的 PA 地址正确 |

```c
/* PPN-01 示例：PPN 替换基本验证 */
TEST_REGISTER(test_ppn_subst_basic);
bool test_ppn_subst_basic(void) {
    TEST_BEGIN("PPN-01: PPN substitution within 64 KiB NAPOT region");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 64 KiB NAPOT 恒等映射 */
    uintptr_t test_va = TEST_REGION_BASE;  /* 64 KiB aligned */
    uintptr_t napot_pte = napot_make_pte(test_va, test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                                          NAPOT_64K);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* S-mode: 在区域内不同 4 KiB 页写入不同 magic value 并读回 */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_ppn_subst_verify,
                                        test_va);
    TEST_ASSERT("PPN substitution correct for all 16 pages", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

#### Group 3：保留编码异常

**规范依据**：
- `norm:Svnapot_reserved_encoding_fault`：当 PTE 的编码在 <<ptenapot>> 表中标记为 Reserved 时，必须触发 page-fault 异常

根据规范中 <<ptenapot>> 表（i=0 级别），以下 ppn[0] 编码在 N=1 时为保留：
- `x xxxx xxx1`：Reserved
- `x xxxx xx1x`：Reserved
- `x xxxx x1xx`：Reserved
- `x xxxx 0xxx`（ppn[0][3]=0 且不是 1000 模式）：Reserved

**测试职责**：验证所有保留编码触发 page-fault。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| RSVD-01 | ppn[0] = xxx1（bit 0 = 1） | N=1，ppn[0] 低 4 位为 0001 | page-fault |
| RSVD-02 | ppn[0] = xx1x（bit 1 = 1） | N=1，ppn[0] 低 4 位为 0010 | page-fault |
| RSVD-03 | ppn[0] = x1xx（bit 2 = 1） | N=1，ppn[0] 低 4 位为 0100 | page-fault |
| RSVD-04 | ppn[0] = 0000（bit 3 = 0） | N=1，ppn[0] 低 4 位为 0000（Reserved: `x xxxx 0xxx`） | page-fault |
| RSVD-05 | ppn[0] = 0101（多保留位） | N=1，ppn[0] 低 4 位为 0101 | page-fault |
| RSVD-06 | ppn[0] = 0111（多保留位） | N=1，ppn[0] 低 4 位为 0111 | page-fault |
| RSVD-07 | ppn[0] = 1001（bit 0 和 bit 3 同时置位） | N=1，ppn[0] 低 4 位为 1001 | page-fault |
| RSVD-08 | i≥1 级 N=1（Reserved，level 1） | i≥1 级别的 NAPOT PTE（任意 ppn 编码均保留），在 level 1（2 MB superpage）设置 N=1 | page-fault |
| RSVD-09 | i≥1 级 N=1（Reserved，level 2） | 在 level 2（1 GB gigapage 级别）设置 N=1，所有 ppn 编码均保留 | page-fault |

```c
/* RSVD-01 示例：ppn[0] = xxx1 保留编码 */
TEST_REGISTER(test_napot_reserved_bit0);
bool test_napot_reserved_bit0(void) {
    TEST_BEGIN("RSVD-01: N=1, ppn[0] bit 0 set triggers page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 构造保留编码 PTE：N=1, ppn[0] 低 4 位 = 0001 */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t reserved_pte = napot_make_reserved_pte(test_va, test_va,
                                PTE_V | PTE_R | PTE_A | PTE_D,
                                0x1);  /* ppn[0] 低 4 位 = 0001 */
    napot_install_pte(&ctx, test_va, reserved_pte);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("reserved encoding triggers page fault",
                result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

/* RSVD-08 示例：level 1 (2MB) 的 N=1 保留编码
 * 规范 <<ptenapot>> 表中 i>=1 时所有 ppn 编码均为 Reserved，
 * 因此只要 N=1 且 leaf PTE 在 level>=1，就必须触发 page-fault */
TEST_REGISTER(test_napot_reserved_level1);
bool test_napot_reserved_level1(void) {
    TEST_BEGIN("RSVD-08: N=1 at level 1 (2MB) triggers page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 在 level 1 (2MB superpage) 级别设置 N=1 的 PTE
     * i>=1 时所有 ppn[i] 编码均保留，任意编码都应触发 fault */
    uintptr_t test_va = TEST_REGION_BASE & ~(PAGE_SIZE_2M - 1);
    uintptr_t reserved_pte = napot_make_pte_at_level(test_va, test_va,
                                PTE_V | PTE_R | PTE_A | PTE_D,
                                PT_LEVEL_2M, /*N=*/1);
    napot_install_pte_at_level(&ctx, test_va, reserved_pte, PT_LEVEL_2M);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("N=1 at level 1 triggers page fault",
                result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}

/* RSVD-09 示例：level 2 (1GB) 的 N=1 保留编码 */
TEST_REGISTER(test_napot_reserved_level2);
bool test_napot_reserved_level2(void) {
    TEST_BEGIN("RSVD-09: N=1 at level 2 (1GB) triggers page fault");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* 使用不同的 1GB 区域做恒等映射，避免与测试区域冲突 */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 在 level 2 (1GB gigapage) 级别设置 N=1 的 PTE
     * i>=1 时所有 ppn[i] 编码均保留 */
    uintptr_t test_va = TEST_REGION_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t reserved_pte = napot_make_pte_at_level(test_va, test_va,
                                PTE_V | PTE_R | PTE_A | PTE_D,
                                PT_LEVEL_1G, /*N=*/1);
    napot_install_pte_at_level(&ctx, test_va, reserved_pte, PT_LEVEL_1G);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("N=1 at level 2 triggers page fault",
                result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
```

---

#### Group 4：NAPOT PTE 权限控制

**规范依据**：
- `norm:Svnapot_pte_N`：N=1 时，PTE 表示连续翻译范围，PTE bits 5-0（包含 V、R、W、X、U、G）在范围内所有翻译中相同
- NAPOT PTE 在地址翻译算法中的行为与非 NAPOT PTE 相同（除了 PPN 替换和编码检查外）

**测试职责**：验证 NAPOT 页面的 R、W、X、U 权限位正确生效。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PERM-01 | NAPOT 只读页面（R only） | N=1 NAPOT 页面仅设置 R 权限，S-mode 写入 | store page-fault |
| PERM-02 | NAPOT 读写页面（RW） | N=1 NAPOT 页面设置 RW 权限，S-mode 读写 | 读写成功 |
| PERM-03 | NAPOT 可执行页面（RX） | N=1 NAPOT 页面设置 RX 权限，S-mode 跳转执行 | 执行成功 |
| PERM-04 | NAPOT 不可执行页面写执行 | N=1 NAPOT 页面仅设置 RW（无 X），S-mode 跳转执行 | instruction page-fault |
| PERM-05 | NAPOT U 位访问控制（S-mode） | N=1 NAPOT 页面设置 U=1，S-mode（sstatus.SUM=0）访问 | page-fault |
| PERM-06 | NAPOT U 位访问控制（SUM=1） | N=1 NAPOT 页面设置 U=1，S-mode（sstatus.SUM=1）读写 | 读写成功 |
| PERM-07 | NAPOT 权限在区域内一致 | 64 KiB NAPOT 区域内不同偏移处验证权限一致 | 所有偏移权限行为一致 |

```c
/* PERM-01 示例：NAPOT 只读页面 */
TEST_REGISTER(test_napot_perm_read_only);
bool test_napot_perm_read_only(void) {
    TEST_BEGIN("PERM-01: NAPOT 64 KiB page R-only, write faults");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 64 KiB NAPOT 只读映射 */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t napot_pte = napot_make_pte(test_va, test_va,
                                          PTE_V | PTE_R | PTE_A | PTE_D,
                                          NAPOT_64K);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* 读应成功 */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load, test_va);
    TEST_ASSERT("NAPOT R-only: read succeeds", result == 0);

    /* 写应触发 store page-fault */
    result = vm_run_in_smode(&ctx, test_smode_store_expect_fault, test_va);
    TEST_ASSERT("NAPOT R-only: write faults", result == CAUSE_SPF);

    pt_pool_reset();
    TEST_END();
}
```

---

#### Group 5：N=0 行为验证

**规范依据**：
- `norm:Svnapot_pte_N`：N=1 时启用 NAPOT 语义；当 N=0 时，PTE 行为与标准非 NAPOT PTE 完全相同（即标准 4 KiB 页面翻译）
- PTE bit 63 即为 N 位（在 Svnapot 扩展实现时）

**测试职责**：验证 N=0 时 PTE 行为与普通 PTE 完全一致。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| NZERO-01 | N=0 标准 4 KiB 页面 | 设置 N=0 的普通 4 KiB PTE，S-mode 读写 | 正常 4 KiB 页面翻译 |
| NZERO-02 | N=0 ppn[0]=1000 无 NAPOT 效果 | N=0 但 ppn[0] 低位编码恰好是 `1000`，验证不触发 NAPOT 语义 | 标准 4 KiB 翻译（ppn 不替换） |
| NZERO-03 | N=0 时 ppn[0] 低位不参与 NAPOT 编码检查 | N=0 时 ppn[0] 低位为 `0001` 是标准 ppn 编码（代表物理页帧号），不触发任何 NAPOT 相关检查 | 正常访问（N=0 时无 NAPOT 编码检查） |

```c
/* NZERO-01 示例：N=0 标准 4 KiB 页面 */
TEST_REGISTER(test_napot_n0_standard);
bool test_napot_n0_standard(void) {
    TEST_BEGIN("NZERO-01: N=0 PTE behaves as standard 4 KiB page");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 标准 4 KiB 映射，N=0 */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx, test_va, test_va,
                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                PT_LEVEL_4K);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("N=0 standard 4KB page works", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

#### Group 6：SFENCE.VMA 与 NAPOT 页面交互

**规范依据**：
- `norm:Svnapot_cache_entries`：NAPOT PTE 的隐式读取可能创建地址翻译缓存条目，映射 a + j×PTESIZE 到替换后的 pte 副本
- 规范注释：更新 NAPOT PTE 时，OS 应先使所有 PTE 无效，然后对范围内所有 4 KiB 区域执行 SFENCE.VMA，再更新 PTE
- SFENCE.VMA 可使用 rs1=x0 的全局刷新，或对每个 4 KiB 区域单独刷新

**测试职责**：验证 SFENCE.VMA 对 NAPOT 区域 TLB 缓存的刷新效果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SFENCE-01 | 全局 SFENCE.VMA 刷新 NAPOT 缓存 | 修改 NAPOT PTE 权限后执行 sfence.vma(0,0)，验证新权限生效 | 新权限生效 |
| SFENCE-02a | 单条全局 SFENCE.VMA 刷新 NAPOT 区域 | 修改 NAPOT PTE 后，执行单条 sfence.vma rs1=x0（全局刷新），验证 64 KiB 区域内新权限生效 | 新权限生效 |
| SFENCE-02b | 多条按地址 SFENCE.VMA 逐页刷新 NAPOT 区域 | 修改 NAPOT PTE 后，对区域内每个 4 KiB 页分别执行 sfence.vma rs1≠x0（共 16 条），验证新权限生效 | 新权限生效 |
| SFENCE-03 | NAPOT 映射移除后刷新 | 将 NAPOT PTE 设为无效（V=0），执行 sfence.vma 后访问 | page-fault |
| SFENCE-04 | NAPOT 权限升级后刷新 | 将 NAPOT R-only 升级为 RW，执行 sfence.vma 后写入 | 写入成功 |
| SFENCE-05 | 单地址 SFENCE.VMA 仅刷新对应 4 KiB | 对 64 KiB NAPOT 区域内单个 4 KiB 地址刷新，验证该地址新权限生效 | 至少该 4 KiB 地址新权限生效 |

```c
/* SFENCE-01 示例：全局 SFENCE.VMA 刷新 NAPOT TLB */
TEST_REGISTER(test_napot_sfence_global);
bool test_napot_sfence_global(void) {
    TEST_BEGIN("SFENCE-01: Global sfence.vma flushes NAPOT TLB entries");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 初始映射：64 KiB NAPOT R-only */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t napot_pte = napot_make_pte(test_va, test_va,
                                          PTE_V | PTE_R | PTE_A | PTE_D,
                                          NAPOT_64K);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* 首次访问建立 TLB */
    vm_run_in_smode(&ctx, test_smode_load, test_va);

    /* 验证写入失败 */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_store_expect_fault,
                                        test_va);
    TEST_ASSERT("R-only: write faults", result == CAUSE_SPF);

    /* 修改 NAPOT PTE 为 RW */
    napot_pte = napot_make_pte(test_va, test_va,
                                PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                                NAPOT_64K);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* 全局 sfence.vma */
    vm_sfence_vma(0, 0);

    /* 写入应成功 */
    result = vm_run_in_smode(&ctx, test_smode_store, test_va);
    TEST_ASSERT("After sfence.vma: write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

#### Group 7：Sv 模式兼容性

**规范依据**：
- `norm:Svnapot_depends_Sv39`：Svnapot 扩展依赖 Sv39 扩展
- 规范描述：Svnapot 在 Sv39、Sv48 和 Sv57 中均适用

**测试职责**：验证 NAPOT 页面在不同 Sv 模式下的行为一致性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MODE-01 | Sv39 下 64 KiB NAPOT | Sv39 模式下配置 64 KiB NAPOT 区域并访问 | 翻译正确 |
| MODE-02 | Sv48 下 64 KiB NAPOT | Sv48 模式下配置 64 KiB NAPOT 区域并访问 | 翻译正确 |
| MODE-03 | Sv57 下 64 KiB NAPOT | Sv57 模式下配置 64 KiB NAPOT 区域并访问 | 翻译正确 |
| MODE-04 | Sv39→Sv48 切换 NAPOT 保持 | 从 Sv39 切换到 Sv48，重建含 NAPOT 映射的页表后验证 | 切换后 NAPOT 翻译正确 |
| MODE-05 | Sv39 依赖验证 | 验证 Svnapot 实现必须支持 Sv39 | Sv39 模式可用 |

```c
/* MODE-02 示例：Sv48 下 64 KiB NAPOT */
TEST_REGISTER(test_napot_sv48);
bool test_napot_sv48(void) {
    TEST_BEGIN("MODE-02: 64 KiB NAPOT page under Sv48");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV48);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t napot_pte = napot_make_pte(test_va, test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                                          NAPOT_64K);
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("Sv48 64 KiB NAPOT read/write succeeds", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

#### Group 8：A/D 位与 NAPOT 页面

**规范依据**：
- 规范注释：实现可能不直接查询算法指定的 PTE，因此 NAPOT 区域内不同映射的 D 和 A 位可能不一致
- OS 必须查询所有 NAPOT 别名以确定页面是否被访问或脏
- 建议 OS 设置某页的 A/D 位时，同时设置其他 NAPOT 别名的 A/D 位

**测试职责**：验证 NAPOT 页面 A/D 位的基本行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| AD-01 | NAPOT A=0 触发 page-fault（Svade） | 64 KiB NAPOT PTE A=0，S-mode 读取 | load page-fault（Svade 语义下） |
| AD-02 | NAPOT D=0 写触发 page-fault（Svade） | 64 KiB NAPOT PTE D=0（A=1），S-mode 写入 | store page-fault（Svade 语义下） |
| AD-03 | NAPOT A=1,D=1 正常访问 | 64 KiB NAPOT PTE A=1,D=1，S-mode 读写 | 正常访问 |
| AD-04 | NAPOT 区域内 A/D 位一致性 | 设置 NAPOT PTE A=1,D=1，访问区域内多个 4 KiB 偏移 | 所有偏移正常访问 |
| AD-05 | NAPOT 别名 A 位独立性 | 配置两个 NAPOT PTE 别名覆盖同一 64 KiB 区域，一个 A=1，另一个 A=0，访问区域 | 实现可能使用任一别名 -- 访问行为取决于实现选择 |
| AD-06 | NAPOT 别名 D 位独立性 | 类似 AD-05，但测试 D 位：一个 D=1，另一个 D=0，写入区域 | 实现可能使用任一别名 -- 写入行为取决于实现选择 |
| AD-07 | OS 手动设置 A 位避免 trap | 手动设置一个 NAPOT 别名的 A=1，但其他别名 A=0，访问后验证 trap 行为 | 若实现选择 A=0 的别名则 trap，若选择 A=1 的别名则不 trap |

```c
/* AD-01 示例：NAPOT A=0 触发 page-fault */
TEST_REGISTER(test_napot_ad_a0_fault);
bool test_napot_ad_a0_fault(void) {
    TEST_BEGIN("AD-01: NAPOT A=0 load triggers page fault (Svade)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 64 KiB NAPOT 映射：A=0, D=1 */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t napot_pte = napot_make_pte(test_va, test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_D,
                                          NAPOT_64K);  /* 注意：无 PTE_A */
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_load_expect_fault,
                                        test_va);
    TEST_ASSERT("NAPOT A=0 triggers load page fault", result == CAUSE_LPF);

    pt_pool_reset();
    TEST_END();
}
```

---

#### Group 9：NAPOT PTE 与 TLB 缓存行为

**规范依据**：
- `norm:Svnapot_cache_entries`：NAPOT PTE 的隐式读取可创建地址翻译缓存条目，映射范围内所有满足 j >> napot_bits = vpn[i] >> napot_bits 的 j
- 规范注释：TLB 允许缓存 V=0 的 NAPOT PTE
- 规范注释：64 KiB NAPOT PTE 可能触发创建 16 个标准 4 KiB TLB 条目

**测试职责**：验证 NAPOT PTE 的 TLB 缓存行为和多别名访问。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TLB-01 | 单次 PTE walk 后全区域可访问 | 访问 64 KiB NAPOT 区域内首地址后，验证其他偏移也可访问 | 所有 16 个 4 KiB 页面均可访问 |
| TLB-02 | NAPOT 区域替换为普通 PTE 后刷新 | 将 NAPOT PTE 替换为普通 4 KiB PTE，刷新 TLB 后验证 | 仅替换的 4 KiB 页面可访问 |
| TLB-03 | NAPOT V=0 缓存允许 | 设置 NAPOT PTE V=0，访问应触发 page-fault | page-fault |
| TLB-04 | V=0 NAPOT PTE TLB 缓存一致性 | 设置 NAPOT PTE V=0 并访问触发 fault 后，修改 PTE 为 V=1 但不执行 SFENCE.VMA 直接访问 | 实现可能使用缓存的 V=0 翻译（仍 fault）或重新查询页表（成功）-- 验证不出现不可预测行为 |

```c
/* TLB-01 示例：单次 PTE walk 后全区域可访问 */
TEST_REGISTER(test_napot_tlb_full_region);
bool test_napot_tlb_full_region(void) {
    TEST_BEGIN("TLB-01: Single NAPOT PTE walk covers entire 64 KiB");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t napot_pte = napot_make_pte(test_va, test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                                          NAPOT_64K);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* S-mode: 遍历访问所有 16 个 4 KiB 页面 */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_access_all_16_pages,
                                        test_va);
    TEST_ASSERT("All 16 x 4 KiB pages accessible", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

#### ~~Group 10：Hypervisor 扩展（G-stage）~~ [已迁移]

> **注意**：请参考 `DOCS/testplan/Hypervisor_cross_test_plan.md` Group 6（Hypervisor × Svnapot 交叉测试）。请在该文档中查看最新内容。

---

#### Group 11：PTE bits 5-0 一致性与 NAPOT 别名

**规范依据**：
- `norm:Svnapot_pte_N`：NAPOT PTE "represents a translation that is part of a range of contiguous virtual-to-physical translations with the same values for PTE bits 5-0"
- 规范注释：同一 NAPOT 区域内的所有 NAPOT PTE 应具有相同的属性、相同的 PPN 和相同的 bits 5-0 值
- 规范注释：如果存在不一致，效果与 SFENCE.VMA 使用不正确时相同：将选择其中一个翻译，但选择不可预测

**测试职责**：验证同一 NAPOT 区域内多个别名 PTE 的 bits 5-0 一致性行为，以及 G 位的全局翻译语义。

> **注意**：BITS50-01 和 BITS50-02 属于"OS 责任"类测试。规范指出 "if any inconsistencies do exist, one of the translations will be chosen, but the choice is unpredictable"，此类测试主要验证实现不会产生不可恢复的错误。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| BITS50-01 | NAPOT 别名 R 位不一致 | 在 64 KiB 区域配置两个 NAPOT PTE 别名，一个 R=1，另一个 R=0 | 实现行为确定（选择其中一个翻译，不可预测但不崩溃） |
| BITS50-02 | NAPOT 别名 V 位不一致 | 一个 NAPOT PTE V=1，同区域另一个别名 V=0 | 实现行为确定（不可恢复错误不应发生） |
| BITS50-03 | NAPOT G 位基本功能 | 64 KiB NAPOT PTE 设置 G=1，验证 global 翻译语义 | 翻译不受 ASID 影响（sfence.vma 按 ASID 刷新不影响 G=1 条目） |

```c
/* BITS50-03 示例：NAPOT G 位基本功能 */
TEST_REGISTER(test_napot_global_bit);
bool test_napot_global_bit(void) {
    TEST_BEGIN("BITS50-03: NAPOT PTE G=1 global translation");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 64 KiB NAPOT 映射：G=1 */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t napot_pte = napot_make_pte(test_va, test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_G |
                                          PTE_A | PTE_D,
                                          NAPOT_64K);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* 使用 ASID=1 启用 VM */
    uintptr_t result = vm_run_in_smode_asid(&ctx, test_smode_read_write,
                                             test_va, /*asid=*/1);
    TEST_ASSERT("G=1 NAPOT read/write with ASID=1", result == 0);

    /* 按 ASID=1 刷新 TLB，G=1 条目不应被刷新 */
    vm_sfence_vma(0, 1);

    /* 切换到 ASID=2，G=1 映射应仍然有效 */
    result = vm_run_in_smode_asid(&ctx, test_smode_read_write,
                                   test_va, /*asid=*/2);
    TEST_ASSERT("G=1 NAPOT survives ASID-specific sfence", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

#### Group 12：RSW 位验证

**规范依据**：
- 规范注释：RSW remains reserved for supervisor software control
- PTE bits 9-8 为 RSW（Reserved for Supervisor Software），硬件应忽略此字段，不影响地址翻译行为

**测试职责**：验证 NAPOT PTE 的 RSW 位可被自由写入且不影响翻译。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| RSW-01 | NAPOT PTE RSW 位可写入 | 在 NAPOT PTE 的 RSW 位写入非零值（01、10、11），验证不影响地址翻译 | 地址翻译正常，不触发 fault |
| RSW-02 | NAPOT PTE RSW 位读取回 | 写入 RSW 位后，通过页表 walk 读取 PTE，验证 RSW 值保持 | RSW 值与写入值一致 |

```c
/* RSW-01 示例：NAPOT PTE RSW 位可写入 */
TEST_REGISTER(test_napot_rsw_writable);
bool test_napot_rsw_writable(void) {
    TEST_BEGIN("RSW-01: NAPOT PTE RSW bits do not affect translation");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 64 KiB NAPOT 映射，RSW = 11 (bits 9-8 both set) */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t rsw_bits = (3UL << 8);  /* RSW = 11 */
    uintptr_t napot_pte = napot_make_pte(test_va, test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                                          NAPOT_64K);
    napot_pte |= rsw_bits;  /* 手动设置 RSW 位 */
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("RSW=11 does not affect NAPOT translation", result == 0);

    pt_pool_reset();
    TEST_END();
}

/* RSW-02 示例：NAPOT PTE RSW 位读取回 */
TEST_REGISTER(test_napot_rsw_readback);
bool test_napot_rsw_readback(void) {
    TEST_BEGIN("RSW-02: NAPOT PTE RSW bits are preserved");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t rsw_bits = (2UL << 8);  /* RSW = 10 */
    uintptr_t napot_pte = napot_make_pte(test_va, test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                                          NAPOT_64K);
    napot_pte |= rsw_bits;
    napot_install_pte(&ctx, test_va, napot_pte);

    /* 通过页表 walk 读取 PTE 并验证 RSW 位 */
    uintptr_t pt_page = get_pt_page_addr(&ctx, test_va, PT_LEVEL_4K);
    uintptr_t *pte_ptr = (uintptr_t *)pt_page + VA_VPN0(test_va);
    uintptr_t readback_rsw = (*pte_ptr >> 8) & 0x3;
    TEST_ASSERT("RSW readback is 10", readback_rsw == 2);

    pt_pool_reset();
    TEST_END();
}
```

---

#### Group 13：NAPOT 区域边界对齐验证

**规范依据**：
- `norm:Svnapot_range_napot`：连续范围必须是自然对齐的 2 的幂次方粒度，且大于基本页大小
- `norm:Svnapot_implicit_read_ppn_subst`：ppn[i][napot_bits-1:0] 被替换为 vpn[i][napot_bits-1:0]

**测试职责**：验证非 64 KiB 对齐场景下 NAPOT PTE 的翻译行为。

> **注意**：由于 NAPOT 的 PPN 替换语义是 ppn[0][3:0] 被替换为 vpn[0][3:0]，如果 VA 不是 64 KiB 对齐，翻译结果中 PA 的低位会被 VA 的低位覆盖。这不是 fault 条件，但翻译结果需要验证。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ALIGN-01 | VA 非 64 KiB 对齐 NAPOT 翻译验证 | NAPOT PTE 的 ppn[0] 编码为 1000，但 VA 不是 64 KiB 对齐（如 VA 偏移 0x1000），验证翻译结果 | 翻译结果中 PA 低位被 VA 低位覆盖（PPN 替换语义） |
| ALIGN-02 | PA 非 64 KiB 对齐 NAPOT 翻译验证 | NAPOT PTE 的 PA 字段不是 64 KiB 对齐，ppn[0] 低 4 位仍编码为 1000，验证翻译结果 | 翻译结果中 PA 低位由 vpn[0] 低位决定（ppn 低位被替换） |

```c
/* ALIGN-01 示例：VA 非 64 KiB 对齐的 NAPOT 翻译 */
TEST_REGISTER(test_napot_unaligned_va);
bool test_napot_unaligned_va(void) {
    TEST_BEGIN("ALIGN-01: NAPOT PTE with non-64KiB-aligned VA");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 使用 64 KiB 对齐的基地址构造 NAPOT PTE，
     * 但通过偏移 VA 来测试 PPN 替换行为 */
    uintptr_t napot_base = TEST_REGION_BASE;  /* 64 KiB aligned */
    uintptr_t napot_pte = napot_make_pte(napot_base, napot_base,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                                          NAPOT_64K);
    napot_install_pte(&ctx, napot_base, napot_pte);

    /* 访问 VA = napot_base + 0x5000，vpn[0] 低 4 位 = 5
     * PPN 替换后 PA 低位应为 5（对应偏移 0x5000） */
    uintptr_t test_va = napot_base + 0x5000;
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("Non-aligned VA within NAPOT region translates correctly",
                result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

#### Group 14：PBMT 与 NAPOT 交互测试（条件性）

**规范依据**：
- PTE bits 62-61 为 PBMT（Page-Based Memory Types），由 Svpbmt 扩展定义
- 如果实现了 Svpbmt 扩展，NAPOT PTE 也应支持 PBMT 属性
- 如果 Svpbmt 未实现，PBMT 位应保持为 0

**测试职责**：验证 NAPOT PTE 与 Svpbmt 扩展的交互（仅当 Svpbmt 已实现时适用）。

> **注意**：以下测试仅在目标平台实现了 Svpbmt 扩展时执行。如果 Svpbmt 未实现，这些测试应跳过。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PBMT-01 | NAPOT + PBMT PMA 映射 | 64 KiB NAPOT PTE 设置 PBMT=PMA（00），正常访问 | 正常访问 |
| PBMT-02 | NAPOT + PBMT NC 映射 | 64 KiB NAPOT PTE 设置 PBMT=NC（01），验证非缓存访问 | 正常访问，无缓存语义 |
| PBMT-03 | NAPOT + PBMT IO 映射 | 64 KiB NAPOT PTE 设置 PBMT=IO（10），验证 I/O 强排序访问 | 正常访问，强排序语义 |

```c
/* PBMT-01 示例：NAPOT + PBMT PMA */
TEST_REGISTER(test_napot_pbmt_pma);
bool test_napot_pbmt_pma(void) {
    TEST_BEGIN("PBMT-01: NAPOT PTE with PBMT=PMA (00)");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t code_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              code_flags, PT_LEVEL_1G);

    /* 64 KiB NAPOT 映射：PBMT=PMA (00, 默认) */
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t napot_pte = napot_make_pte(test_va, test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                                          NAPOT_64K);
    /* PBMT=00 (PMA) 是默认值，无需额外设置 */
    napot_install_pte(&ctx, test_va, napot_pte);

    uintptr_t result = vm_run_in_smode(&ctx, test_smode_read_write,
                                        test_va);
    TEST_ASSERT("NAPOT + PBMT=PMA works", result == 0);

    pt_pool_reset();
    TEST_END();
}
```

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1（基本映射）、Group 3（保留编码异常）、Group 4（权限控制） | NAPOT64-01~06、RSVD-01~09、PERM-01~07 | 核心功能：NAPOT 翻译、保留编码 fault、权限正确性 |
| P1（重要） | Group 2（PPN 替换）、Group 5（N=0 行为）、Group 6（SFENCE.VMA）、Group 12（RSW 位） | PPN-01~05、NZERO-01~03、SFENCE-01~05、RSW-01~02、AD-05~07 | PPN 替换语义正确性、N=0 兼容性、TLB 管理、RSW 位基本保障、A/D 别名独立性 |
| P2（建议） | Group 7（多模式兼容）、Group 8（A/D 位）、Group 9（TLB 缓存）、Group 11（bits 5-0 一致性）、Group 13（边界对齐） | MODE-01~05、AD-01~04、TLB-01~04、BITS50-01~03、ALIGN-01~02 | 跨模式验证、高级行为、别名一致性、对齐边界 |
| P3（可选） | Group 14（PBMT 交互） | PBMT-01~03 | 依赖 Svpbmt 扩展，条件性实现 |

---

## 测试实现说明

### NAPOT PTE 编码

Svnapot 扩展中，PTE bit 63 为 N 位。当 N=1 时，ppn[0] 的编码决定 NAPOT 区域大小。Version 1.0 仅标准化 64 KiB 支持：

```
N=1, i=0, ppn[0] = x xxxx 1000  →  64 KiB contiguous region, napot_bits = 4
```

64 KiB NAPOT PTE 的 ppn[0] 低 4 位编码为 `1000`（即 bit3=1, bits[2:0]=0），含义是：
- 该 PTE 覆盖 16 个连续的 4 KiB 页面（16 × 4 KiB = 64 KiB）
- 基地址必须 64 KiB 对齐
- 隐式读取时，ppn[0][3:0] 被替换为 vpn[0][3:0]

测试代码中应定义以下常量和辅助函数：

```c
/* NAPOT PTE N bit (bit 63) */
#define PTE_N           (1UL << 63)

/* NAPOT region size constants */
#define NAPOT_64K       16  /* 16 × 4 KiB pages */
#define NAPOT_64K_SIZE  (64 * 1024)  /* 64 KiB = 0x10000 */
#define NAPOT_64K_MASK  (NAPOT_64K_SIZE - 1)

/* NAPOT bits for 64 KiB region */
#define NAPOT_BITS_64K  4

/**
 * napot_make_pte - Construct a 64 KiB NAPOT PTE
 * @va:    Virtual address (must be 64 KiB aligned)
 * @pa:    Physical address (must be 64 KiB aligned)
 * @flags: PTE flags (PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D etc.)
 * @npages: Number of 4 KiB pages in NAPOT region (must be 16 for 64 KiB)
 *
 * Constructs a NAPOT PTE with N=1 and ppn[0] encoded as xxxx1000.
 *
 * PTE layout:
 *   bit 63:     N = 1
 *   bits 62-61: PBMT (0 if Svpbmt not used)
 *   bits 60-54: Reserved (must be 0)
 *   bits 53-10: PPN (with ppn[0] bits [3:0] = 1000)
 *   bits 9-8:   RSW
 *   bits 7-0:   D A G U X W R V
 */
static inline uintptr_t napot_make_pte(uintptr_t va, uintptr_t pa,
                                        uintptr_t flags, int npages) {
    /* pa must be 64 KiB aligned */
    uintptr_t ppn = pa >> PAGE_SHIFT;
    /* Set ppn[0] low 4 bits to 1000 for 64 KiB NAPOT encoding */
    ppn = (ppn & ~0xFUL) | 0x8UL;
    return PTE_N | (ppn << PTE_PPN_SHIFT) | (flags & PTE_FLAGS_MASK);
}

/**
 * napot_make_reserved_pte - Construct a NAPOT PTE with reserved encoding
 * @va:        Virtual address
 * @pa:        Physical address
 * @flags:     PTE flags
 * @ppn0_low4: Low 4 bits of ppn[0] (must be a reserved encoding)
 */
static inline uintptr_t napot_make_reserved_pte(uintptr_t va, uintptr_t pa,
                                                  uintptr_t flags,
                                                  unsigned ppn0_low4) {
    uintptr_t ppn = pa >> PAGE_SHIFT;
    ppn = (ppn & ~0xFUL) | (ppn0_low4 & 0xFUL);
    return PTE_N | (ppn << PTE_PPN_SHIFT) | (flags & PTE_FLAGS_MASK);
}

/**
 * napot_install_pte - Install a NAPOT PTE into the page table
 * @ctx:  Page table context
 * @va:   Virtual address (64 KiB aligned)
 * @pte:  Pre-constructed NAPOT PTE value
 *
 * Walks the page table to find the level-0 PTE slot for @va,
 * creating intermediate page table pages as needed, then writes
 * the provided PTE value directly.
 */
void napot_install_pte(pt_context_t *ctx, uintptr_t va, uintptr_t pte);
```

### S-mode 辅助函数

测试计划中使用的 S-mode 辅助函数：

| 函数名 | 功能 | 返回值 |
|--------|------|--------|
| `test_smode_load` | 执行 load 操作 | 0=成功 |
| `test_smode_store` | 执行 store 操作 | 0=成功 |
| `test_smode_read_write` | 写入 magic value 并读回验证 | 0=成功，非 0=失败 |
| `test_smode_load_expect_fault` | 执行 load，预期触发 fault | fault cause |
| `test_smode_store_expect_fault` | 执行 store，预期触发 fault | fault cause |
| `test_smode_exec_expect_fault` | 跳转执行，预期触发 fault | fault cause |
| `test_smode_ppn_subst_verify` | 在 64 KiB 区域内 16 个 4 KiB 页面写入不同 magic value 并读回验证 | 0=全部正确 |
| `test_smode_access_all_16_pages` | 遍历访问 64 KiB 区域内所有 16 个 4 KiB 页面 | 0=全部可访问 |

### 文件组织

Svnapot 测试依赖虚拟内存测试框架，建议的文件组织如下：

```
svnapot/
├── Makefile                # ENABLE_VM=1, TARGET=svnapot_test.elf
├── kernel.ld               # 链接脚本（复用 sv39 的模板）
├── main.c                  # Svnapot 测试用例入口
├── napot_helpers.c         # NAPOT PTE 构造和安装辅助函数
└── napot_reserved_tests.c  # 保留编码异常测试（Group 3），精细构造各种 PTE 编码

common/vm/                  # 已有，无需修改
├── vm_defs.h               # 常量定义
├── vm.h                    # API 声明
├── page_table.c            # 页表管理
└── satp.c                  # satp 控制和 vm_run_in_smode
```

### Makefile 配置

```makefile
# svnapot/Makefile
TARGET = svnapot_test.elf
ENABLE_VM = 1
EXT_OBJS = main.o napot_helpers.o
include ../common/Makefile.common
```

### 通用测试模式

Svnapot 测试用例遵循以下模式：

```c
#include "test_framework.h"
#include "vm/vm.h"
#include "napot_helpers.h"

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

    /* 2. 构造并安装 NAPOT PTE */
    uintptr_t test_va = TEST_REGION_BASE;  /* 必须 64 KiB 对齐 */
    uintptr_t napot_pte = napot_make_pte(test_va, test_va,
                                          PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
                                          NAPOT_64K);
    napot_install_pte(&ctx, test_va, napot_pte);

    /* 3. 在 S-mode + VM 下执行测试 */
    uintptr_t result = vm_run_in_smode(&ctx, test_smode_xxx, test_va);
    TEST_ASSERT("description", result == expected);

    /* 4. 清理 */
    pt_pool_reset();
    TEST_END();
}
```

### 关键注意事项

1. **64 KiB 对齐**：NAPOT 64 KiB 区域的基地址（VA 和 PA）必须 64 KiB（0x10000）对齐。`TEST_REGION_BASE` 应定义为满足此对齐要求的地址。

2. **PTE 手动构造**：由于现有 `pt_map_page()` API 不支持 NAPOT PTE，需要通过 `napot_install_pte()` 辅助函数直接写入页表。该函数需要遍历页表到 level 0，找到对应的 PTE slot 并写入预构造的 NAPOT PTE 值。

3. **PTE_A 和 PTE_D 位**：除 Group 8（A/D 位管理）的测试外，其他测试应始终设置 `PTE_A | PTE_D`，避免因 Svade 语义导致不必要的 page-fault。

4. **Page-fault cause 编码**：
   - `CAUSE_IPF` (12)：Instruction page-fault
   - `CAUSE_LPF` (13)：Load page-fault
   - `CAUSE_SPF` (15)：Store/AMO page-fault

5. **QEMU 支持**：QEMU 7.0+ 支持 Svnapot 扩展。启动时需在 `-cpu` 参数中启用 `svnapot=true`，例如：
   ```bash
   qemu-system-riscv64 -machine virt -cpu rv64,svnapot=true \
       -nographic -bios none -kernel svnapot/svnapot_test.elf
   ```

6. **NAPOT PTE 与普通 PTE 不冲突**：在同一页表中，NAPOT PTE 占用 level-0 的单个 PTE slot（对应 64 KiB 区域的首个 4 KiB 页面地址）。区域内其余 15 个 4 KiB 页面的 PTE slot 应置为无效（V=0）或保持一致的 NAPOT 编码。测试中需注意不要对这些 slot 设置冲突的映射。

7. **PMP 配置**：`vm_run_in_smode()` 自动配置 PMP entry 15 为全地址空间 NAPOT RWX。如需测试 PMP 与 Svnapot 的交互，需手动管理 PMP 配置。

8. **Hypervisor 扩展**：G-stage 相关测试（原 Group 10，GSTG-01~03）已迁移至 `DOCS/testplan/Hypervisor_cross_test_plan.md` Group 6。

---

## 规范覆盖矩阵

| 规范标签 | 覆盖的测试 ID |
|----------|--------------|
| `norm:Svnapot_pte_N` | NAPOT64-01~06、NZERO-01~03、PERM-01~07、BITS50-01~03 |
| `norm:Svnapot_range_napot` | NAPOT64-01~06、PPN-01~05、ALIGN-01~02 |
| `norm:Svnapot_depends_Sv39` | MODE-01、MODE-05 |
| `norm:Svnapot_valid_encoding` | NAPOT64-01~06、PPN-01~05、PERM-01~07 |
| `norm:Svnapot_implicit_read_ppn_subst` | PPN-01~05、ALIGN-01~02 |
| `norm:Svnapot_reserved_encoding_fault` | RSVD-01~09 |
| `norm:Svnapot_cache_entries` | TLB-01~04、SFENCE-01~05 |
| `norm:Svnapot_hyp_gstage` | GSTG-01~03（已迁移至 `Hypervisor_cross_test_plan.md` Group 6） |
| RSW 位（Note 引用） | RSW-01~02 |
| A/D 别名独立性（Note 引用） | AD-05~07 |
| PBMT 交互（Svpbmt 依赖） | PBMT-01~03（条件性，依赖 Svpbmt 扩展） |

---

## 参考

- RISC-V Privileged Specification — Svnapot Extension (`SPEC/svnapot.adoc`)
- `SPEC/supervisor.adoc` — Supervisor-Level ISA（SFENCE.VMA 定义、satp CSR 定义）
- `docs/vm_test_framework.md` — 虚拟内存测试框架文档
- `docs/vm_test_plan.md` — 虚拟内存测试计划（PTE 权限、SFENCE.VMA 参考）
- `common/vm/vm.h` — VM API 声明
- `common/vm/vm_defs.h` — 常量和宏定义
