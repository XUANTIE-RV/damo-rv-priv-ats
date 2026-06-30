# Smcdeleg 扩展测试计划（Machine Mode）

> 本文档描述 Smcdeleg（Counter Delegation — Machine-level）扩展的测试计划。聚焦于 M-mode 下 `menvcfg.CDE` 使能控制、`mcounteren` 委托交互、以及与依赖扩展的协同行为。S-mode 层面的 Ssccfg 行为由 `Ssccfg_test_plan.md` 覆盖。
>
> 生成时间：2026-06-25

---

## 范围

### 覆盖的 Smcdeleg Machine-level 行为

- **`menvcfg.CDE` 使能控制**：M-mode 对 `menvcfg.CDE` 位的读写、对下级特权级的控制效果
- **`mcounteren` 委托交互**：`mcounteren` 位与 `menvcfg.CDE=1` 配合实现计数器委托
- **扩展依赖**：Smcdeleg/Ssccfg 必须成对实现；均依赖 Sscsrind
- **Smstateen 交互**：`mstateen0` bit 60 对 `siselect`/`sireg*`/`vsiselect`/`vsireg*` 的控制
- **LCOFI（M-mode 部分）**：Smcdeleg + Sscofpmf + Smaia 下 `mvip`/`mvien` bit 13 的实现

### 不在本文档范围

- S-mode 通过 `siselect`/`sireg*` 间接访问委托计数器的行为（由 `Ssccfg_test_plan.md` 覆盖）
- `scountinhibit` 寄存器的 S-mode 访问行为（由 `Ssccfg_test_plan.md` 覆盖）
- Hypervisor 场景下 `vsiselect`/`vsireg*` 的虚拟化行为（由 `Ssccfg_test_plan.md` 覆盖）
- 非委托场景下的 `mcounteren` 基本行为（由其他扩展测试计划覆盖）

---

## 规范依据

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:smcdeleg_ssccfg_tandem` | `smcdeleg.adoc` | For a RISC-V hardware platform, Smcdeleg and Ssccfg must always be implemented in tandem. | 在 RISC-V 硬件平台上，Smcdeleg 和 Ssccfg 必须成对实现。 |
| `norm:smcdeleg_cde_en` | `smcdeleg.adoc` | When the Smcdeleg/Ssccfg extensions are enabled (`menvcfg`.CDE=1), `mcounteren` further allows M-mode to delegate select counters to S-mode. | 当 Smcdeleg/Ssccfg 扩展启用（`menvcfg`.CDE=1）时，`mcounteren` 进一步允许 M-mode 将选定的计数器委托给 S-mode。 |
| `norm:smcdeleg_depends_sscsrind` | `smcdeleg.adoc` | The Smcdeleg and Ssccfg extensions both depend on the Sscsrind extension. | Smcdeleg 和 Ssccfg 扩展都依赖 Sscsrind 扩展。 |
| `norm:smcdeleg_mstateen0_bit60` | `smcdeleg.adoc` | If extension Smstateen is implemented, setting bit 60 of CSR `mstateen0` to zero prevents access to registers `siselect`, `sireg*`, `vsiselect`, and `vsireg*` from privileged modes less privileged than M-mode. | 若实现了 Smstateen 扩展，将 `mstateen0` 的 bit 60 设为 0 会阻止低于 M-mode 的特权级访问 `siselect`、`sireg*`、`vsiselect` 和 `vsireg*`。 |
| `norm:ssccfg_lcofi_mvip_mvien` | `smcdeleg.adoc` | For implementations that support Smcdeleg, Sscofpmf, and Smaia, the local-counter-overflow interrupt (LCOFI) bit (bit 13) in each of CSRs `mvip` and `mvien` is implemented and writable. | 对于支持 Smcdeleg、Sscofpmf 和 Smaia 的实现，`mvip` 和 `mvien` CSR 中的 LCOFI 位（bit 13）必须已实现且可写。 |

---

## Group 1. menvcfg.CDE 使能控制

**规范依据**：
- `norm:smcdeleg_cde_en`：`menvcfg.CDE=1` 启用计数器委托

**测试职责**：验证 M-mode 对 `menvcfg.CDE` 位的读写行为，以及 CDE 位对计数器委托功能的使能/禁用控制。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SMCD-MENV-01 | menvcfg.CDE 读写回环 | M-mode 写 menvcfg.CDE=1 后读回验证；再写 CDE=0 读回验证 | 若实现 Smcdeleg，CDE 可写且读回一致；若未实现 Smcdeleg，CDE 为只读零 |
| SMCD-MENV-02 | menvcfg.CDE=0 禁止委托 | menvcfg.CDE=0，mcounteren[i]=1，S-mode 通过 siselect=0x40+i 访问 sireg* | 触发 illegal-instruction 异常（CDE=0 时不允许通过 siselect 0x40-0x5F 范围访问 sireg*） |
| SMCD-MENV-03 | menvcfg.CDE=1 启用委托 | menvcfg.CDE=1，mcounteren[0]=1（cycle 委托），S-mode 设 siselect=0x40 后读 sireg | 访问成功，sireg 返回 cycle 计数器值 |
| SMCD-MENV-04 | menvcfg.CDE 动态切换 | 先设 CDE=1 并验证 S-mode 可访问委托计数器；再设 CDE=0 并验证 S-mode 访问被阻止 | CDE=1 时访问正常；CDE=0 后访问触发 illegal-instruction 异常 |

> [!NOTE]
> - `menvcfg.CDE` 是计数器委托的总使能开关。即使 `mcounteren[i]=1`，若 CDE=0，S-mode 仍无法通过间接访问机制操作委托计数器。
> - SMCD-MENV-01 需要在运行时检测 Smcdeleg 扩展的可用性。若平台未实现 Smcdeleg，CDE 位应为只读零，SMCD-MENV-02~04 应 TEST_SKIP。
> - `menvcfg` 的 CSR 地址为 0x30A（RV64）或 0x30A/0x31A（RV32，menvcfgh）。

---

## Group 2. mcounteren 委托交互

**规范依据**：
- `norm:smcdeleg_cde_en`：`mcounteren` 与 `menvcfg.CDE=1` 配合实现委托

**测试职责**：验证 M-mode 通过 `mcounteren` 位选择性地委托计数器给 S-mode 的行为，以及委托位与 CDE 的组合效果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SMCD-MCTR-01 | mcounteren 委托 cycle（bit 0） | menvcfg.CDE=1，mcounteren[0]=1，S-mode 设 siselect=0x40 访问 sireg | 访问成功（cycle 已委托） |
| SMCD-MCTR-02 | mcounteren 未委托 cycle | menvcfg.CDE=1，mcounteren[0]=0，S-mode 设 siselect=0x40 访问 sireg | 触发 illegal-instruction 异常（计数器未委托） |
| SMCD-MCTR-03 | mcounteren 委托 instret（bit 2） | menvcfg.CDE=1，mcounteren[2]=1，S-mode 设 siselect=0x42 访问 sireg | 访问成功（instret 已委托） |
| SMCD-MCTR-04 | mcounteren 未委托 instret | menvcfg.CDE=1，mcounteren[2]=0，S-mode 设 siselect=0x42 访问 sireg | 触发 illegal-instruction 异常 |
| SMCD-MCTR-05 | mcounteren 委托 hpmcounter3（bit 3） | menvcfg.CDE=1，mcounteren[3]=1，S-mode 设 siselect=0x43 访问 sireg | 访问成功（hpmcounter3 已委托） |
| SMCD-MCTR-06 | mcounteren 未委托 hpmcounter3 | menvcfg.CDE=1，mcounteren[3]=0，S-mode 设 siselect=0x43 访问 sireg | 触发 illegal-instruction 异常 |
| SMCD-MCTR-07 | mcounteren 委托 hpmcounter31（bit 31） | menvcfg.CDE=1，mcounteren[31]=1，S-mode 设 siselect=0x5F 访问 sireg | 访问成功（hpmcounter31 已委托） |
| SMCD-MCTR-08 | mcounteren 动态切换委托位 | menvcfg.CDE=1，先设 mcounteren[3]=1 验证 S-mode 可访问；再清 mcounteren[3]=0 验证 S-mode 访问被阻止 | mcounteren[3]=1 时访问正常；mcounteren[3]=0 后访问触发 illegal-instruction |
| SMCD-MCTR-09 | mcounteren 多位同时委托 | menvcfg.CDE=1，mcounteren = 0xFFFFFFFF（委托所有计数器），S-mode 逐一访问 siselect 0x40-0x5F（跳过 0x41） | 除 siselect=0x41 外，所有委托计数器的 sireg 访问均成功 |
| SMCD-MCTR-10 | mcounteren TM 位（bit 1）与 siselect=0x41 | menvcfg.CDE=1，mcounteren[1]=1，S-mode 设 siselect=0x41 访问 sireg | 触发 illegal-instruction 异常（mtime 不可委托，siselect=0x41 始终非法） |

> [!NOTE]
> - `mcounteren` bit 0 = CY（cycle），bit 1 = TM（time），bit 2 = IR（instret），bits 3-31 = HPM3-HPM31。
> - siselect=0x41 对应 mtime，规范明确指出 mtime 不是性能监控计数器，不应通过委托机制管理，因此 siselect=0x41 始终触发 illegal-instruction 异常，与 mcounteren[1] 无关。
> - SMCD-MCTR-09 需跳过 siselect=0x41 并检查依赖扩展（Zicntr、Zihpm）的实现情况。
> - M-mode 对 `mcounteren` 的写入本身不受 CDE 限制——CDE 仅影响 S-mode 能否通过间接访问机制使用委托的计数器。

---

## Group 3. 扩展依赖与成对实现

**规范依据**：
- `norm:smcdeleg_ssccfg_tandem`：Smcdeleg 和 Ssccfg 必须成对实现
- `norm:smcdeleg_depends_sscsrind`：两个扩展都依赖 Sscsrind

**测试职责**：验证 Smcdeleg/Ssccfg 的成对实现要求，以及对 Sscsrind 的依赖关系。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SMCD-DEP-01 | Smcdeleg 与 Ssccfg 成对检测 | 检测平台是否同时实现 Smcdeleg 和 Ssccfg | 若 menvcfg.CDE 可写（非 RO0），则 S-mode 的 siselect/sireg* 间接访问机制应可用（Ssccfg 已实现） |
| SMCD-DEP-02 | Sscsrind 依赖验证 | 检测 Sscsrind 是否实现（siselect CSR 是否存在） | 若 Sscsrind 已实现，siselect CSR 应可读写；若未实现，Smcdeleg/Ssccfg 不应可用 |
| SMCD-DEP-03 | siselect 存在性验证 | M-mode 尝试读写 siselect CSR | siselect 应可读写（Smcdeleg/Ssccfg 依赖 Sscsrind，Sscsrind 定义 siselect） |

> [!NOTE]
> - Smcdeleg 和 Ssccfg 规范要求必须成对实现。测试应验证两者同时存在或同时不存在，不应出现只实现其中一个的情况。
> - Sscsrind 是 Smcdeleg/Ssccfg 的前提依赖。若 Sscsrind 未实现，则 siselect/sireg* CSR 不存在，Smcdeleg/Ssccfg 无法工作。
> - 运行时可通过尝试写 menvcfg.CDE 并读回来探测 Smcdeleg 是否实现；通过尝试访问 siselect 来探测 Sscsrind 是否实现。

---

## Group 4. Smstateen 与 Smcdeleg 交互（M-mode 控制）

**规范依据**：
- `norm:smcdeleg_mstateen0_bit60`：`mstateen0` bit 60 控制下级对 siselect/sireg*/vsiselect/vsireg* 的访问

**测试职责**：验证 M-mode 通过 `mstateen0` bit 60 控制 S-mode 对间接访问 CSR 的访问权限。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SMCD-STA-01 | mstateen0 bit 60 读写验证 | M-mode 写 mstateen0 bit 60 = 1 后读回；再写 0 读回 | 若实现 Smstateen，bit 60 可写且读回一致；若未实现 Smstateen，bit 60 为只读零 |
| SMCD-STA-02 | mstateen0 bit 60 = 0 阻止 S-mode 访问 siselect | mstateen0 bit 60 = 0，menvcfg.CDE=1，S-mode 尝试写 siselect | 触发 illegal-instruction 异常 |
| SMCD-STA-03 | mstateen0 bit 60 = 0 阻止 S-mode 访问 sireg* | mstateen0 bit 60 = 0，menvcfg.CDE=1，S-mode 尝试读 sireg | 触发 illegal-instruction 异常 |
| SMCD-STA-04 | mstateen0 bit 60 = 1 允许 S-mode 访问 | mstateen0 bit 60 = 1，menvcfg.CDE=1，mcounteren[0]=1，S-mode 设 siselect=0x40 读 sireg | 访问成功 |
| SMCD-STA-05 | mstateen0 bit 60 优先于 CDE | mstateen0 bit 60 = 0，menvcfg.CDE=1，S-mode 访问 siselect | 触发 illegal-instruction 异常（mstateen0 阻止优先级高于 CDE 使能） |

> [!NOTE]
> - 本组测试仅在平台实现 Smstateen 扩展时有效。若未实现 Smstateen，mstateen0 CSR 不存在，所有测试应 TEST_SKIP。
> - `mstateen0` bit 60 是 CSRIND 控制位，控制所有间接访问相关 CSR（siselect/sireg*/vsiselect/vsireg*）对低于 M-mode 特权级的访问。
> - bit 60 = 0 时的阻止效果优先于 `menvcfg.CDE` 的使能效果：即使 CDE=1，若 bit 60 = 0，S-mode 仍无法访问 siselect/sireg*。

---

## Group 5. LCOFI 中断位（M-mode 部分）

**规范依据**：
- `norm:ssccfg_lcofi_mvip_mvien`：Smcdeleg + Sscofpmf + Smaia 下，`mvip`/`mvien` bit 13 必须已实现且可写

**测试职责**：验证在 Smcdeleg + Sscofpmf + Smaia 同时实现时，M-mode 的 `mvip` 和 `mvien` 寄存器中 LCOFI 位（bit 13）的实现与可写性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SMCD-LCOFI-01 | mvip bit 13（LCOFI）可写性 | M-mode 写 mvip bit 13 = 1 后读回；再写 0 读回 | 若 Smcdeleg+Sscofpmf+Smaia 均已实现，bit 13 可写且读回一致 |
| SMCD-LCOFI-02 | mvien bit 13（LCOFI）可写性 | M-mode 写 mvien bit 13 = 1 后读回；再写 0 读回 | 若 Smcdeleg+Sscofpmf+Smaia 均已实现，bit 13 可写且读回一致 |
| SMCD-LCOFI-03 | mvip.LCOFI 独立于其他位 | 写 mvip 全 1 后读回，检查 bit 13 | bit 13 读回为 1（不受其他位影响） |
| SMCD-LCOFI-04 | mvien.LCOFI 独立于其他位 | 写 mvien 全 1 后读回，检查 bit 13 | bit 13 读回为 1 |

> [!NOTE]
> - 本组测试需要 Smcdeleg、Sscofpmf 和 Smaia 三个扩展同时实现。若任一扩展未实现，应 TEST_SKIP。
> - `mvip`（Machine Virtual Interrupt Pending）和 `mvien`（Machine Virtual Interrupt Enable）由 Smaia 扩展定义。LCOFI 位（bit 13）的可写性在 Smcdeleg + Sscofpmf 存在时被强制要求。
> - LCOFI 位的实现确保虚拟 LCOFI 中断可以递送到 S-mode 操作系统。
> - `mvip`/`mvien` 的 CSR 地址分别为 0x310 和 0x308（由 Smaia 定义）。

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1 (menvcfg.CDE) | SMCD-MENV-01~04 | CDE 是计数器委托的总使能开关，是 Smcdeleg 的核心功能 |
| P0（必须） | Group 2 (mcounteren) | SMCD-MCTR-01~10 | mcounteren 委托位选择是计数器委托的基础机制 |
| P1（重要） | Group 3 (依赖) | SMCD-DEP-01~03 | 成对实现和依赖关系是正确性保证的前提 |
| P1（重要） | Group 5 (LCOFI) | SMCD-LCOFI-01~04 | LCOFI 中断位是性能监控溢出中断的关键设施 |
| P2（建议） | Group 4 (Smstateen) | SMCD-STA-01~05 | Smstateen 交互是安全隔离的补充保证 |

---

## 测试实现说明

### 文件组织

```
damo-priv-test/
├── smcdeleg/
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_smcdeleg_menvcfg.c    # Group 1: menvcfg.CDE 使能控制
│       ├── test_smcdeleg_mcounteren.c # Group 2: mcounteren 委托交互
│       ├── test_smcdeleg_dep.c        # Group 3: 扩展依赖
│       ├── test_smcdeleg_stateen.c    # Group 4: Smstateen 交互
│       └── test_smcdeleg_lcofi.c     # Group 5: LCOFI 中断位
└── common/                            # 复用通用框架
```

### 运行时检测

```c
static bool check_smcdeleg_extension(void) {
    /* Probe menvcfg.CDE writability */
    uintptr_t old = CSRR(menvcfg);
    CSRW(menvcfg, old | MENVCFG_CDE);
    uintptr_t new_val = CSRR(menvcfg);
    CSRW(menvcfg, old);  /* restore */
    return (new_val & MENVCFG_CDE) != 0;
}

static bool check_sscsrind_extension(void) {
    /* Probe siselect CSR existence via trap */
    trap_expect_begin();
    CSRW(siselect, 0x30);
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

static bool check_smaia_extension(void) {
    /* Probe mvip CSR existence */
    trap_expect_begin();
    CSRR(mvip);
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

static bool check_sscofpmf_extension(void) {
    /* Probe via menvcfg.CDE or platform config */
    return platform_has_sscofpmf();
}
```

### 通用测试模式

#### 模式 1：menvcfg.CDE 使能控制（Group 1）

```c
/* SMCD-MENV-01: menvcfg.CDE read-write round-trip */
TEST_REGISTER(test_smcd_menv_01);
bool test_smcd_menv_01(void) {
    TEST_BEGIN("SMCD-MENV-01: menvcfg.CDE read-write round-trip");

    uintptr_t orig = CSRR(menvcfg);

    /* Write CDE=1 */
    CSRW(menvcfg, orig | MENVCFG_CDE);
    uintptr_t val1 = CSRR(menvcfg);

    /* Write CDE=0 */
    CSRW(menvcfg, orig & ~MENVCFG_CDE);
    uintptr_t val0 = CSRR(menvcfg);

    /* Restore */
    CSRW(menvcfg, orig);

    if (check_smcdeleg_extension()) {
        TEST_ASSERT("CDE=1 writable", (val1 & MENVCFG_CDE) != 0);
        TEST_ASSERT("CDE=0 writable", (val0 & MENVCFG_CDE) == 0);
    } else {
        TEST_ASSERT("CDE read-only zero without Smcdeleg",
                    (val1 & MENVCFG_CDE) == 0);
    }

    TEST_END();
}

/* SMCD-MENV-02: menvcfg.CDE=0 disables delegation */
TEST_REGISTER(test_smcd_menv_02);
bool test_smcd_menv_02(void) {
    TEST_BEGIN("SMCD-MENV-02: menvcfg.CDE=0 disables delegation");

    if (!check_smcdeleg_extension()) TEST_SKIP("Smcdeleg not available");
    if (!check_sscsrind_extension()) TEST_SKIP("Sscsrind not available");

    uintptr_t orig_cde = CSRR(menvcfg);
    uintptr_t orig_ctren = CSRR(mcounteren);

    /* CDE=0, mcounteren[0]=1 */
    CSRW(menvcfg, orig_cde & ~MENVCFG_CDE);
    CSRW(mcounteren, orig_ctren | MCOUNTEREN_CY);

    /* S-mode: siselect=0x40, read sireg -> should trap */
    trap_expect_begin();
    run_in_smode(_s_siselect_sireg_read, 0x40);
    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("illegal-instruction exception",
                       trap_get_cause(), CAUSE_ILLEGAL_INSTRUCTION);
    }
    trap_expect_end();

    /* Restore */
    CSRW(menvcfg, orig_cde);
    CSRW(mcounteren, orig_ctren);
    TEST_END();
}

/* SMCD-MENV-03: menvcfg.CDE=1 enables delegation */
TEST_REGISTER(test_smcd_menv_03);
bool test_smcd_menv_03(void) {
    TEST_BEGIN("SMCD-MENV-03: menvcfg.CDE=1 enables delegation");

    if (!check_smcdeleg_extension()) TEST_SKIP("Smcdeleg not available");
    if (!check_sscsrind_extension()) TEST_SKIP("Sscsrind not available");

    uintptr_t orig_cde = CSRR(menvcfg);
    uintptr_t orig_ctren = CSRR(mcounteren);

    /* CDE=1, mcounteren[0]=1 (delegate cycle) */
    CSRW(menvcfg, orig_cde | MENVCFG_CDE);
    CSRW(mcounteren, orig_ctren | MCOUNTEREN_CY);

    /* S-mode: siselect=0x40, read sireg -> should succeed */
    trap_expect_begin();
    uintptr_t cycle_val = run_in_smode(_s_siselect_sireg_read, 0x40);
    TEST_ASSERT("no trap on delegated cycle access",
                !trap_was_triggered());
    TEST_ASSERT("cycle value is nonzero", cycle_val != 0);
    trap_expect_end();

    /* Restore */
    CSRW(menvcfg, orig_cde);
    CSRW(mcounteren, orig_ctren);
    TEST_END();
}
```

#### 模式 2：mcounteren 委托交互（Group 2）

```c
/* SMCD-MCTR-10: mcounteren TM bit and siselect=0x41 */
TEST_REGISTER(test_smcd_mctr_10);
bool test_smcd_mctr_10(void) {
    TEST_BEGIN("SMCD-MCTR-10: siselect=0x41 always illegal (mtime)");

    if (!check_smcdeleg_extension()) TEST_SKIP("Smcdeleg not available");
    if (!check_sscsrind_extension()) TEST_SKIP("Sscsrind not available");

    uintptr_t orig_cde = CSRR(menvcfg);
    uintptr_t orig_ctren = CSRR(mcounteren);

    /* CDE=1, mcounteren[1]=1 (TM bit set) */
    CSRW(menvcfg, orig_cde | MENVCFG_CDE);
    CSRW(mcounteren, orig_ctren | MCOUNTEREN_TM);

    /* S-mode: siselect=0x41, read sireg -> should always trap
     * mtime is NOT a delegatable counter */
    trap_expect_begin();
    run_in_smode(_s_siselect_sireg_read, 0x41);
    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("illegal-instruction for siselect=0x41",
                       trap_get_cause(), CAUSE_ILLEGAL_INSTRUCTION);
    }
    trap_expect_end();

    /* Restore */
    CSRW(menvcfg, orig_cde);
    CSRW(mcounteren, orig_ctren);
    TEST_END();
}
```

### 关键注意事项

1. **扩展检测**：所有测试必须在运行时检测 Smcdeleg/Ssccfg 的可用性（通过 `menvcfg.CDE` 可写性），不可用时 TEST_SKIP。

2. **Sscsrind 依赖**：Smcdeleg/Ssccfg 依赖 Sscsrind。若 Sscsrind 未实现，`siselect`/`sireg*` CSR 不存在，所有涉及间接访问的测试应 TEST_SKIP。

3. **成对实现要求**：Smcdeleg 和 Ssccfg 必须成对实现。若检测到 `menvcfg.CDE` 可写，则 Ssccfg 的 S-mode 行为（siselect/sireg* 间接访问）也应可用。

4. **mcounteren 位映射**：bit 0 = CY（cycle），bit 1 = TM（time），bit 2 = IR（instret），bits 3-31 = HPM3-HPM31。siselect=0x41（mtime）始终不可委托。

5. **LCOFI 依赖链**：`mvip`/`mvien` bit 13 的可写性需要 Smcdeleg + Sscofpmf + Smaia 三者同时实现。若任一未实现，bit 13 可能为只读零。

6. **Smstateen 交互**：`mstateen0` bit 60（CSRIND）阻止优先级高于 `menvcfg.CDE`。即使 CDE=1，若 bit 60 = 0，S-mode 仍无法访问间接访问 CSR。

---

## 参考

- `SPEC/smcdeleg.adoc` — Smcdeleg and Ssccfg Counter Delegation Extensions
- `SPEC/sscsrind.adoc` — Sscsrind Extension (indirect CSR access)
- `SPEC/smstateen.adoc` — Smstateen Extension
- `SPEC/smaia.adoc` — Smaia Extension (Advanced Interrupt Architecture)
- `SPEC/sscofpmf.adoc` — Sscofpmf Extension (Counter Overflow and Mode Filtering)
- `DOCS/testplan/Ssccfg_test_plan.md` — Ssccfg 扩展测试计划（Supervisor Mode）
