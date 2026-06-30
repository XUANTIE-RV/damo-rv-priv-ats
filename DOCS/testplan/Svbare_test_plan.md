**中文 | [English](../testplan_en/Svbare_test_plan_en.md)**

# Svbare 扩展测试计划

本文档描述 Svbare（Supervisor Bare Mode Support）扩展的测试计划。Svbare 扩展规定：实现必须支持 `satp` 寄存器的 MODE 字段能够保持 Bare 值（MODE=0）。当 MODE=Bare 时，supervisor 虚拟地址等于 supervisor 物理地址，不进行页表翻译，除 PMP 外无额外内存保护。

---

## 概述

RISC-V 特权级规范中，`satp`（Supervisor Address Translation and Protection）寄存器控制地址翻译模式。其 MODE 字段决定了地址翻译方案：

1. **MODE=Bare（值 0）**：不进行地址翻译和保护。supervisor 虚拟地址直接等于物理地址，唯一的内存保护机制是 PMP（Physical Memory Protection）。
2. **MODE=Sv39/Sv48/Sv57**：启用对应级别的页表虚拟内存系统。

Svbare 扩展的核心要求是：**实现必须支持 `satp.MODE` 字段能保持 Bare 值**。这意味着软件可以通过写 `satp=0` 来禁用虚拟内存。

选择 MODE=Bare 时，软件必须将 `satp` 的剩余字段（ASID、PPN）全部写零。若以非零模式写入剩余字段，其效果为 UNSPECIFIED。

本测试计划聚焦以下方面：
- `satp.MODE=Bare` 的 CSR 可写性与回读一致性
- Bare 模式下 VA=PA 直通内存访问行为
- Bare 模式下 SUM/MXR 位无效性验证
- Bare 模式与 PMP 的交互
- satp MODE 在 Bare 与 Sv39 之间的切换
- 边界条件与特殊场景

---

## 测试范围

### 规范来源

- `SPEC/supervisor.adoc`，第 1005–1015 行（`satp` MODE 字段语义，MODE=Bare 定义）
- `SPEC/supervisor.adoc`，第 1036–1037 行（`[[svbare]]` 扩展定义）
- `SPEC/supervisor.adoc`，第 1055–1057 行（写入不支持 MODE 时整个写无效）
- `SPEC/supervisor.adoc`，第 1100–1103 行（`satp` 仅在 S/U 模式有效）

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:svbare_satp_mode_bare` | If an implementation supports the Svbare extension, then the `satp` register's MODE field must be capable of holding the value Bare. | 若实现支持 Svbare 扩展，`satp` 的 MODE 字段必须能保存 Bare 值。 |
| `norm:satp_mode` | When MODE=Bare, supervisor virtual addresses are equal to supervisor physical addresses, and there is no additional memory protection. To select MODE=Bare, software must write zero to the remaining fields of `satp`. Attempting to select MODE=Bare with a nonzero pattern in the remaining fields has an UNSPECIFIED effect. | MODE=Bare 时，S 虚拟地址等于 S 物理地址，无额外内存保护。选择 Bare 模式时软件必须将 `satp` 其余字段写零。非零模式时效果未指定。 |
| `norm:satp_mode_op_unsupported` | — | 写入不支持的 MODE 时整个写无效，`satp` 不被修改 |
| `norm:satp_op_active` | — | `satp` 仅在有效特权模式为 S-mode 或 U-mode 时激活 |
| `norm:sstatus_sum` | The SUM (permit Supervisor User Memory access) bit modifies the privilege with which S-mode loads and stores access virtual memory. When SUM=0, S-mode memory accesses to pages that are accessible by U-mode (U=1) will fault. When SUM=1, these accesses are permitted. SUM has no effect when page-based virtual memory is not in effect, nor when executing in U-mode. Note that S-mode can never execute instructions from user pages, regardless of the state of SUM. | SUM 位控制 S 模式对 U 模式页面的访问。SUM=0 时 S 模式访问 U=1 页面会出错；SUM=1 时允许。未启用分页或 U 模式下无效。S 模式永远不能执行用户页面的指令。 |
| `norm:sstatus_mxr` | The MXR (Make eXecutable Readable) bit modifies the privilege with which loads access virtual memory. When MXR=0, only loads from pages marked readable (R=1) will succeed. When MXR=1, loads from pages marked either readable or executable (R=1 or X=1) will succeed. MXR has no effect when page-based virtual memory is not in effect. | MXR 位修改 load 访问虚拟内存的权限。MXR=0 时仅从可读页面加载成功；MXR=1 时从可读或可执行页面加载均成功。未启用分页时无效。 |

### 不在测试范围内

- **Hypervisor 两级翻译场景**：VS-stage 与 G-stage 下的 Bare 模式行为（涉及 H 扩展）。
- **Sv32（RV32）**：本计划仅覆盖 RV64（SXLEN=64），RV32 下 Bare 的 MODE 编码与 ASID 字段布局不同。
- **多 hart 一致性**：当前测试框架为单核环境。
- **Sv48 / Sv57 模式切换**：本计划仅以 Sv39 作为 Bare 的对比切换模式，其他 Sv 模式行为一致。

---

## 测试分组

### Group 1：satp.MODE=Bare 可写性验证

**规范依据**：
- `svbare`：`satp.MODE` 必须能保持 Bare 值
- `norm:satp_mode`：选择 Bare 须写零到剩余字段；非零剩余字段效果 UNSPECIFIED

**测试职责**：验证 M-mode 下写入 `satp` MODE=Bare 的能力，包括回读一致性、剩余字段清零约束、与其他模式的切换。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVBARE-CSR-01 | satp 写入 MODE=Bare 后回读 | M-mode 写 satp=0（MODE=Bare, 其余全零），回读 | 回读 MODE 字段为 0（Bare） |
| SVBARE-CSR-02 | Bare 模式下 ASID/PPN 字段为零 | 写 satp=0 后回读完整值 | satp 全部 64 位均为 0 |
| SVBARE-CSR-03 | 从 Sv39 切换到 Bare | 先写 satp MODE=Sv39（有效配置），再写 satp=0 | 回读 MODE=Bare，satp=0 |
| SVBARE-CSR-04 | 从 Bare 切换到 Sv39 再回 Bare | Bare→Sv39→Bare 多次切换 | 每次 Bare 回读均为 0 |
| SVBARE-CSR-05 | MODE=Bare + 非零剩余字段 | 写 satp 时 MODE=0 但 ASID/PPN 非零 | UNSPECIFIED：记录实际行为（可能忽略、可能清零、可能拒绝写入） |

```c
/* SVBARE-CSR-01 示例：satp 写入 MODE=Bare 后回读 */
TEST_REGISTER(test_svbare_csr_write_bare);
bool test_svbare_csr_write_bare(void) {
    TEST_BEGIN("SVBARE-CSR-01: satp write MODE=Bare readback");

    /* 写入 satp=0 (MODE=Bare, ASID=0, PPN=0) */
    CSRW(satp, 0);

    /* 回读 satp */
    uintptr_t satp_val = CSRR(satp);
    uintptr_t mode = satp_val >> SATP_MODE_SHIFT;

    TEST_ASSERT_EQ("MODE field is Bare (0)", mode, SATP_MODE_BARE);

    TEST_END();
}

/* SVBARE-CSR-03 示例：从 Sv39 切换到 Bare */
TEST_REGISTER(test_svbare_csr_sv39_to_bare);
bool test_svbare_csr_sv39_to_bare(void) {
    TEST_BEGIN("SVBARE-CSR-03: Switch from Sv39 to Bare");

    /* 先设置为 Sv39 模式（需要有效的 PPN） */
    extern uintptr_t __page_table_pool;
    uintptr_t root_ppn = (uintptr_t)&__page_table_pool >> PAGE_SHIFT;
    uintptr_t satp_sv39 = MAKE_SATP(SATP_MODE_SV39, 0, root_ppn);
    CSRW(satp, satp_sv39);

    uintptr_t satp_val = CSRR(satp);
    uintptr_t mode = satp_val >> SATP_MODE_SHIFT;
    TEST_ASSERT_EQ("MODE is Sv39 after write", mode, SATP_MODE_SV39);

    /* 切换回 Bare */
    CSRW(satp, 0);
    satp_val = CSRR(satp);
    TEST_ASSERT_EQ("satp is 0 after switching to Bare", satp_val, 0);

    TEST_END();
}
```

---

### Group 2：Bare 模式 VA=PA 直通访问

**规范依据**：
- `norm:satp_mode`：MODE=Bare 时 supervisor 虚拟地址等于 supervisor 物理地址
- `norm:satp_op_active`：`satp` 在 S/U 模式有效

**测试职责**：验证 MODE=Bare 下 S-mode 和 U-mode 的内存访问直接使用物理地址，无翻译。需要配合 PMP 允许 S/U-mode 访问。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVBARE-VA-01 | S-mode Bare load | satp=0, S-mode 从已知物理地址 load | 读取成功，值正确 |
| SVBARE-VA-02 | S-mode Bare store | satp=0, S-mode 向已知物理地址 store | 写入成功，回读验证 |
| SVBARE-VA-03 | S-mode Bare fetch | satp=0, S-mode 从物理地址取指执行 | 执行成功 |
| SVBARE-VA-04 | U-mode Bare load | satp=0, U-mode 从已知物理地址 load | 读取成功（需 PMP 允许） |
| SVBARE-VA-05 | U-mode Bare store | satp=0, U-mode 向已知物理地址 store | 写入成功（需 PMP 允许） |
| SVBARE-VA-06 | Bare 模式多地址访问 | S-mode 依次访问不同物理地址区域 | 每次均直通成功 |

```c
/* SVBARE-VA-01 示例：S-mode Bare mode load */

static uintptr_t smode_load_fn(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    return *ptr;
}

TEST_REGISTER(test_svbare_smode_load);
bool test_svbare_smode_load(void) {
    TEST_BEGIN("SVBARE-VA-01: S-mode Bare mode load");

    /* 确保 satp=0 (Bare mode) */
    CSRW(satp, 0);
    sfence_vma();

    /* PMP 允许 S-mode 全地址空间访问 */
    pmp_allow_all();

    /* 在已知物理地址写入测试值 */
    volatile uintptr_t *test_addr = (volatile uintptr_t *)TEST_DATA_ADDR;
    *test_addr = 0xCAFEBABE;

    /* S-mode load — 直接使用物理地址 */
    uintptr_t result = run_in_priv(PRIV_S, smode_load_fn,
                                    (uintptr_t)test_addr);
    TEST_ASSERT_EQ("S-mode Bare load value", result, 0xCAFEBABE);

    reset_state();
    TEST_END();
}

/* SVBARE-VA-02 示例：S-mode Bare mode store */

static uintptr_t smode_store_fn(uintptr_t addr) {
    volatile uintptr_t *ptr = (volatile uintptr_t *)addr;
    *ptr = 0xDEADBEEF;
    return 0;
}

TEST_REGISTER(test_svbare_smode_store);
bool test_svbare_smode_store(void) {
    TEST_BEGIN("SVBARE-VA-02: S-mode Bare mode store");

    CSRW(satp, 0);
    sfence_vma();
    pmp_allow_all();

    volatile uintptr_t *test_addr = (volatile uintptr_t *)TEST_DATA_ADDR;
    *test_addr = 0;

    /* S-mode store */
    run_in_priv(PRIV_S, smode_store_fn, (uintptr_t)test_addr);

    /* M-mode 回读验证 */
    TEST_ASSERT_EQ("S-mode Bare store value", *test_addr, 0xDEADBEEF);

    reset_state();
    TEST_END();
}
```

---

### Group 3：Bare 模式下无页表翻译验证

**规范依据**：
- `norm:satp_mode`：MODE=Bare 时无额外内存保护（除 PMP 外）
- `norm:sstatus_sum`：SUM 在无页表虚拟内存时无效（Bare 模式下不产生作用）
- `norm:sstatus_mxr`：MXR 在无页表虚拟内存时无效（Bare 模式下不产生作用）

**测试职责**：验证 Bare 模式下不会触发 page-fault，SUM/MXR 位对 Bare 模式无影响，SFENCE.VMA 无副作用。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVBARE-NOPT-01 | Bare 模式不触发 page-fault | satp=0, S-mode 访问任意 PMP 允许的物理地址 | 无 page-fault（scause != 12/13/15） |
| SVBARE-NOPT-02 | SUM=0 不影响 Bare S-mode 访问 | satp=0, sstatus.SUM=0, S-mode load | 访问成功（SUM 在 Bare 模式无效） |
| SVBARE-NOPT-03 | SUM=1 不影响 Bare S-mode 访问 | satp=0, sstatus.SUM=1, S-mode load | 访问成功（SUM 在 Bare 模式无效） |
| SVBARE-NOPT-04 | MXR 不影响 Bare 模式行为 | satp=0, sstatus.MXR=1 或 0, S-mode load | 访问行为一致（MXR 在 Bare 模式无效） |
| SVBARE-NOPT-05 | Bare 模式下 SFENCE.VMA 无副作用 | satp=0, 执行 SFENCE.VMA 后正常访问 | 访问成功，无异常 |

```c
/* SVBARE-NOPT-02 示例：SUM=0 不影响 Bare S-mode 访问 */
TEST_REGISTER(test_svbare_sum0_no_effect);
bool test_svbare_sum0_no_effect(void) {
    TEST_BEGIN("SVBARE-NOPT-02: SUM=0 does not affect Bare S-mode access");

    CSRW(satp, 0);
    sfence_vma();
    pmp_allow_all();

    /* 显式清除 SUM 位 */
    CSRC(sstatus, MSTATUS_SUM_BIT);

    volatile uintptr_t *test_addr = (volatile uintptr_t *)TEST_DATA_ADDR;
    *test_addr = 0x12345678;

    /* S-mode load — SUM=0 在 Bare 模式下无效，访问应成功 */
    uintptr_t result = run_in_priv(PRIV_S, smode_load_fn,
                                    (uintptr_t)test_addr);
    TEST_ASSERT_EQ("SUM=0 Bare load succeeds", result, 0x12345678);

    reset_state();
    TEST_END();
}

/* SVBARE-NOPT-03 示例：SUM=1 不影响 Bare S-mode 访问 */
TEST_REGISTER(test_svbare_sum1_no_effect);
bool test_svbare_sum1_no_effect(void) {
    TEST_BEGIN("SVBARE-NOPT-03: SUM=1 does not affect Bare S-mode access");

    CSRW(satp, 0);
    sfence_vma();
    pmp_allow_all();

    /* 显式设置 SUM 位 */
    CSRS(sstatus, MSTATUS_SUM_BIT);

    volatile uintptr_t *test_addr = (volatile uintptr_t *)TEST_DATA_ADDR;
    *test_addr = 0xABCDABCD;

    /* S-mode load — SUM=1 在 Bare 模式下同样无效 */
    uintptr_t result = run_in_priv(PRIV_S, smode_load_fn,
                                    (uintptr_t)test_addr);
    TEST_ASSERT_EQ("SUM=1 Bare load succeeds", result, 0xABCDABCD);

    reset_state();
    TEST_END();
}

/* SVBARE-NOPT-05 示例：Bare 模式下 SFENCE.VMA 无副作用 */
TEST_REGISTER(test_svbare_sfence_vma_no_effect);
bool test_svbare_sfence_vma_no_effect(void) {
    TEST_BEGIN("SVBARE-NOPT-05: SFENCE.VMA has no side effect in Bare mode");

    CSRW(satp, 0);
    pmp_allow_all();

    volatile uintptr_t *test_addr = (volatile uintptr_t *)TEST_DATA_ADDR;
    *test_addr = 0x55AA55AA;

    /* 执行 SFENCE.VMA */
    sfence_vma();

    /* S-mode load — 应正常成功 */
    uintptr_t result = run_in_priv(PRIV_S, smode_load_fn,
                                    (uintptr_t)test_addr);
    TEST_ASSERT_EQ("load after sfence.vma succeeds", result, 0x55AA55AA);

    reset_state();
    TEST_END();
}
```

---

### Group 4：Bare 模式与 PMP 交互

**规范依据**：
- `norm:satp_mode`：MODE=Bare 时无额外内存保护，仅 PMP 有效

**测试职责**：验证 Bare 模式下 PMP 仍然是唯一的内存保护机制。PMP 允许时访问成功，PMP 拒绝时触发 access-fault（非 page-fault）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVBARE-PMP-01 | PMP 允许 + Bare S-mode load | satp=0, PMP RWX 全开, S-mode load | 读取成功 |
| SVBARE-PMP-02 | PMP 拒绝 + Bare S-mode load | satp=0, PMP 不含目标地址, S-mode load | load access-fault（scause=5） |
| SVBARE-PMP-03 | PMP 只读 + Bare S-mode store | satp=0, PMP 只有 R, S-mode store | store access-fault（scause=7） |
| SVBARE-PMP-04 | PMP 无执行 + Bare S-mode fetch | satp=0, PMP 有 RW 无 X, S-mode fetch | instruction access-fault（scause=1） |
| SVBARE-PMP-05 | PMP 允许 + Bare U-mode load | satp=0, PMP RWX 全开, U-mode load | 读取成功 |
| SVBARE-PMP-06 | PMP 拒绝 + Bare U-mode store | satp=0, PMP 不含目标地址, U-mode store | store access-fault（scause=7） |

```c
/* SVBARE-PMP-01 示例：PMP 允许 + Bare S-mode load */
TEST_REGISTER(test_svbare_pmp_allow_smode_load);
bool test_svbare_pmp_allow_smode_load(void) {
    TEST_BEGIN("SVBARE-PMP-01: PMP allow + Bare S-mode load");

    CSRW(satp, 0);
    sfence_vma();

    /* 配置 PMP：NAPOT 覆盖全地址空间，RWX */
    pmp_allow_all();

    volatile uintptr_t *test_addr = (volatile uintptr_t *)TEST_DATA_ADDR;
    *test_addr = 0xBEEFBEEF;

    uintptr_t result = run_in_priv(PRIV_S, smode_load_fn,
                                    (uintptr_t)test_addr);
    TEST_ASSERT_EQ("PMP allow Bare S-mode load", result, 0xBEEFBEEF);

    reset_state();
    TEST_END();
}

/* SVBARE-PMP-02 示例：PMP 拒绝 + Bare S-mode load */
TEST_REGISTER(test_svbare_pmp_deny_smode_load);
bool test_svbare_pmp_deny_smode_load(void) {
    TEST_BEGIN("SVBARE-PMP-02: PMP deny + Bare S-mode load");

    CSRW(satp, 0);
    sfence_vma();

    /* 配置 PMP：仅允许代码段和栈区域，不覆盖 TEST_DATA_ADDR */
    pmp_setup_code_and_stack_only();

    trap_expect_begin();
    run_in_priv(PRIV_S, smode_load_fn, (uintptr_t)TEST_DATA_ADDR);

    /* 回到 M-mode 检查 trap 结果 */
    TEST_ASSERT("trap triggered", trap_was_triggered());
    TEST_ASSERT_EQ("load access-fault (cause=5)",
                   trap_get_cause(), CAUSE_LOAD_ACCESS_FAULT);
    trap_expect_end();

    reset_state();
    TEST_END();
}

/* SVBARE-PMP-03 示例：PMP 只读 + Bare S-mode store */
TEST_REGISTER(test_svbare_pmp_ro_smode_store);
bool test_svbare_pmp_ro_smode_store(void) {
    TEST_BEGIN("SVBARE-PMP-03: PMP read-only + Bare S-mode store");

    CSRW(satp, 0);
    sfence_vma();

    /* 配置 PMP：目标地址区域仅允许 R，不允许 W/X */
    pmp_setup_readonly(TEST_DATA_ADDR, PAGE_SIZE_4K);

    trap_expect_begin();
    run_in_priv(PRIV_S, smode_store_fn, (uintptr_t)TEST_DATA_ADDR);

    TEST_ASSERT("trap triggered", trap_was_triggered());
    TEST_ASSERT_EQ("store access-fault (cause=7)",
                   trap_get_cause(), CAUSE_STORE_ACCESS_FAULT);
    trap_expect_end();

    reset_state();
    TEST_END();
}
```

---

### Group 5：satp MODE 切换与 Bare 模式转换

**规范依据**：
- `svbare`：MODE 字段必须能保持 Bare 值
- `norm:satp_mode_op_unsupported`：写入不支持的 MODE 时整个写无效，`satp` 不被修改

**测试职责**：验证 satp 在不同 MODE 之间切换时 Bare 模式的正确性，以及写入无效 MODE 后 Bare 的保留行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVBARE-SW-01 | Bare→Sv39→Bare 正常切换 | 切换三次，每次验证回读 | MODE 字段正确反映切换结果 |
| SVBARE-SW-02 | 写入 reserved MODE 后 satp 不变 | satp=0(Bare), 写入 MODE=7(reserved), 回读 | satp 保持 Bare 不变（整个写无效） |
| SVBARE-SW-03 | 写入 reserved MODE 后 S-mode 仍为 Bare | 续 SW-02, S-mode load | 直通访问成功（仍为 Bare） |
| SVBARE-SW-04 | Sv39→Bare 切换后 VA=PA | 从 Sv39 模式切换回 Bare, S-mode 访问 | 直通物理地址访问成功 |
| SVBARE-SW-05 | 多次 Bare↔Sv39 切换稳定性 | 10 次循环切换 Bare/Sv39 | 每次 Bare 回读均正确，每次切回后 VA=PA |

```c
/* SVBARE-SW-02 示例：写入 reserved MODE 后 satp 不变 */
TEST_REGISTER(test_svbare_reserved_mode_no_change);
bool test_svbare_reserved_mode_no_change(void) {
    TEST_BEGIN("SVBARE-SW-02: Writing reserved MODE leaves satp unchanged");

    /* 先设置为 Bare */
    CSRW(satp, 0);
    uintptr_t satp_before = CSRR(satp);
    TEST_ASSERT_EQ("Initial satp is 0 (Bare)", satp_before, 0);

    /* 尝试写入 reserved MODE=7 */
    uintptr_t reserved_satp = (uintptr_t)7 << SATP_MODE_SHIFT;
    CSRW(satp, reserved_satp);

    /* 回读：整个写应无效，satp 保持不变 */
    uintptr_t satp_after = CSRR(satp);
    TEST_ASSERT_EQ("satp unchanged after reserved MODE write",
                   satp_after, satp_before);

    TEST_END();
}

/* SVBARE-SW-05 示例：多次 Bare↔Sv39 切换稳定性 */
TEST_REGISTER(test_svbare_mode_switch_stability);
bool test_svbare_mode_switch_stability(void) {
    TEST_BEGIN("SVBARE-SW-05: Multiple Bare/Sv39 mode switch stability");

    extern uintptr_t __page_table_pool;
    uintptr_t root_ppn = (uintptr_t)&__page_table_pool >> PAGE_SHIFT;
    uintptr_t satp_sv39 = MAKE_SATP(SATP_MODE_SV39, 0, root_ppn);

    for (int i = 0; i < 10; i++) {
        /* Switch to Sv39 */
        CSRW(satp, satp_sv39);
        uintptr_t val = CSRR(satp);
        uintptr_t mode = val >> SATP_MODE_SHIFT;
        TEST_ASSERT_EQ("MODE is Sv39", mode, SATP_MODE_SV39);

        /* Switch back to Bare */
        CSRW(satp, 0);
        val = CSRR(satp);
        TEST_ASSERT_EQ("satp is 0 (Bare)", val, 0);
    }

    TEST_END();
}
```


---

### Group 6：Bare 模式特殊场景

**规范依据**：
- `norm:satp_mode`：Bare + 非零剩余字段效果 UNSPECIFIED
- `norm:satp_op_active`：`satp` 仅在 S/U 模式有效，M-mode 不受 `satp` 影响

**测试职责**：覆盖边界条件和特殊场景，包括 M-mode 下 satp 无影响、TVM 位对 S-mode satp 写入的控制等。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVBARE-EDGE-01 | M-mode 下 Bare 无影响 | satp=0, M-mode 访问 | M-mode 不受 satp 影响，正常访问 |
| SVBARE-EDGE-02 | satp 全 1 写入（MODE 非法） | 写入 satp 全位为 1 | 实现定义：satp 可能不变（整个写无效）或 MODE 部分被截断 |
| SVBARE-EDGE-03 | Bare 模式连续读写 satp 一致性 | 写 satp=0 后连续 N 次回读 | 每次回读均为 0 |
| SVBARE-EDGE-04 | Bare 后 S-mode 写 satp（TVM=0） | mstatus.TVM=0, S-mode 尝试写 satp=0 | 写入成功，satp 保持 Bare |
| SVBARE-EDGE-05 | TVM=1 时 S-mode 写 satp 触发异常 | mstatus.TVM=1, S-mode 写 satp | 触发 illegal instruction exception（scause=2） |

> **注意**：SVBARE-EDGE-02 涉及实现定义行为，测试用例应记录实际行为而非做强断言。

```c
/* SVBARE-EDGE-01 示例：M-mode 下 satp Bare 无影响 */
TEST_REGISTER(test_svbare_mmode_unaffected);
bool test_svbare_mmode_unaffected(void) {
    TEST_BEGIN("SVBARE-EDGE-01: M-mode unaffected by satp Bare");

    CSRW(satp, 0);

    /* M-mode 直接访问物理内存 - satp 对 M-mode 无影响 */
    volatile uintptr_t *test_addr = (volatile uintptr_t *)TEST_DATA_ADDR;
    *test_addr = 0xFACEFACE;
    uintptr_t val = *test_addr;
    TEST_ASSERT_EQ("M-mode load unaffected", val, 0xFACEFACE);

    TEST_END();
}

/* SVBARE-EDGE-03 示例：连续回读一致性 */
TEST_REGISTER(test_svbare_consecutive_readback);
bool test_svbare_consecutive_readback(void) {
    TEST_BEGIN("SVBARE-EDGE-03: Consecutive satp readback consistency");

    CSRW(satp, 0);

    for (int i = 0; i < 100; i++) {
        uintptr_t val = CSRR(satp);
        if (val != 0) {
            TEST_ASSERT_EQ("satp readback is 0", val, 0);
            break;
        }
    }
    TEST_ASSERT("All 100 readbacks are 0", true);

    TEST_END();
}

/* SVBARE-EDGE-05 示例：TVM=1 时 S-mode 写 satp 触发异常 */

static uintptr_t smode_write_satp_fn(uintptr_t val) {
    asm volatile("csrw satp, %0" :: "r"(val) : "memory");
    return 0;
}

TEST_REGISTER(test_svbare_tvm_smode_write_satp);
bool test_svbare_tvm_smode_write_satp(void) {
    TEST_BEGIN("SVBARE-EDGE-05: TVM=1 S-mode write satp triggers exception");

    CSRW(satp, 0);
    pmp_allow_all();

    /* 设置 mstatus.TVM=1 */
    CSRS(mstatus, MSTATUS_TVM_BIT);

    trap_expect_begin();
    run_in_priv(PRIV_S, smode_write_satp_fn, 0);

    TEST_ASSERT("trap triggered", trap_was_triggered());
    TEST_ASSERT_EQ("illegal instruction (cause=2)",
                   trap_get_cause(), CAUSE_ILLEGAL_INST);
    trap_expect_end();

    /* 清除 TVM */
    CSRC(mstatus, MSTATUS_TVM_BIT);
    reset_state();
    TEST_END();
}
```


---

## 附录：相关常量与 API 参考

### satp 寄存器 RV64 布局

```
 63    60 59    44 43                 0
+--------+--------+-------------------+
|  MODE  |  ASID  |       PPN         |
| (4 bit)|(16 bit)|     (44 bit)      |
+--------+--------+-------------------+
```

### MODE 编码表

| 值 | 名称 | 说明 |
|-----|------|------|
| 0 | Bare | 不进行地址翻译和保护 |
| 1-7 | - | Reserved for standard use |
| 8 | Sv39 | 39-bit 页表虚拟地址 |
| 9 | Sv48 | 48-bit 页表虚拟地址 |
| 10 | Sv57 | 57-bit 页表虚拟地址 |
| 11 | Sv64 | Reserved for 64-bit virtual addressing |
| 12-13 | - | Reserved for standard use |
| 14-15 | - | Designated for custom use |

### CSR 访问宏

- `CSRR(satp)`：读取 satp 寄存器
- `CSRW(satp, val)`：写入 satp 寄存器
- `CSRS(sstatus, bit)`：设置 sstatus 指定位
- `CSRC(sstatus, bit)`：清除 sstatus 指定位
- `sfence_vma()`：执行 SFENCE.VMA 指令

### satp 构造宏

```c
#define SATP_MODE_BARE  0
#define SATP_MODE_SV39  8
#define SATP_MODE_SV48  9
#define SATP_MODE_SV57  10

#define SATP_MODE_SHIFT     60
#define SATP_ASID_SHIFT     44
#define SATP_PPN_MASK       ((1UL << 44) - 1)

#define MAKE_SATP(mode, asid, ppn)     (((uintptr_t)(mode) << SATP_MODE_SHIFT) |      ((uintptr_t)(asid) << SATP_ASID_SHIFT) |      ((uintptr_t)(ppn) & SATP_PPN_MASK))
```

### PMP 辅助函数

- `pmp_allow_all()`：配置 PMP NAPOT 覆盖全地址空间，RWX 权限
- `pmp_clear()`：清除 PMP 配置
- `pmp_setup_code_and_stack_only()`：仅允许代码段和栈区域
- `pmp_setup_readonly(addr, size)`：配置指定区域为只读

### scause 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| `CAUSE_INST_ACCESS_FAULT` | 1 | Instruction access fault (PMP) |
| `CAUSE_ILLEGAL_INST` | 2 | Illegal instruction |
| `CAUSE_LOAD_ACCESS_FAULT` | 5 | Load access fault (PMP) |
| `CAUSE_STORE_ACCESS_FAULT` | 7 | Store/AMO access fault (PMP) |
| `CAUSE_INST_PAGE_FAULT` | 12 | Instruction page fault |
| `CAUSE_LOAD_PAGE_FAULT` | 13 | Load page fault |
| `CAUSE_STORE_PAGE_FAULT` | 15 | Store/AMO page fault |

### mstatus 相关位

| 位 | 名称 | 说明 |
|-----|------|------|
| bit 18 | SUM | Permit Supervisor User Memory access |
| bit 19 | MXR | Make eXecutable Readable |
| bit 20 | TVM | Trap Virtual Memory（S-mode 访问 satp 时触发异常） |

### 测试框架 API

- `run_in_priv(priv, fn, arg)`：在指定特权级执行 fn(arg)，返回函数返回值
- `goto_priv(priv)`：切换到指定特权级
- `reset_state()`：重置 PMP 和测试状态，返回 M-mode
- `trap_expect_begin()` / `trap_expect_end()`：trap 记录开始/结束
- `trap_was_triggered()`：检查是否发生了 trap
- `trap_get_cause()`：获取 trap 的 scause 值
- `TEST_REGISTER(fn)`：注册测试函数
- `TEST_BEGIN(name)` / `TEST_END()`：测试用例开始/结束
- `TEST_ASSERT(msg, cond)`：条件断言
- `TEST_ASSERT_EQ(msg, actual, expected)`：相等断言
- `TEST_SKIP(reason)`：跳过测试

---

## 测试执行说明

### 目录结构

测试代码计划放在新建的 `svbare/` 目录，结构如下：

```
svbare/
├── Makefile             # ENABLE_VM = 0, 无 SPIKE_ISA_EXT
├── kernel.ld            # 链接脚本（复用 common/kernel_common.ld）
├── main.c               # 测试入口
└── tests/
    ├── test_register.c       # 测试注册（#include 各测试文件）
    ├── test_csr_bare.c       # Group 1：satp.MODE=Bare 可写性验证
    ├── test_va_pa.c          # Group 2：Bare 模式 VA=PA 直通访问
    ├── test_no_paging.c      # Group 3：Bare 模式下无页表翻译验证
    ├── test_pmp_bare.c       # Group 4：Bare 模式与 PMP 交互
    ├── test_mode_switch.c    # Group 5：satp MODE 切换与 Bare 模式转换
    └── test_edge_cases.c     # Group 6：Bare 模式特殊场景
```

### Makefile 配置

```makefile
# Svbare Extension Verification Test
TARGET = svbare_test.elf
ENABLE_VM = 0

EXT_OBJS =     main.o     tests/test_register.o

include ../common/Makefile.common
```

> **注意**：Svbare 测试**无需启用 VM**（`ENABLE_VM = 0`），因为 Bare 模式不涉及页表翻译。测试在 M-mode 操作 `satp` CSR，并通过 `run_in_priv()` 切换到 S/U-mode 验证内存访问行为。

### 模拟器要求

- **Spike**：默认支持 Bare 模式，无需额外 ISA 扩展参数。直接运行：
  ```
  spike svbare_test.elf
  ```
- **QEMU**：默认支持 Bare 模式，无需额外参数。
- **Sail**：默认支持 Bare 模式。

### 测试运行命令

```bash
cd svbare/
make clean && make          # 编译
make spike                  # 在 Spike 上运行
make qemu                   # 在 QEMU 上运行
make sail                   # 在 Sail 上运行
```
