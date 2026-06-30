**中文 | [English](../testplan_en/Shlcofideleg_test_plan_en.md)**

# Shlcofideleg 扩展测试计划

本文档描述 Shlcofideleg（LCOFI Interrupt Delegation to VS-mode）扩展的测试计划。Shlcofideleg 扩展支持将 LCOFI（Local Count Overflow Interrupt，本地计数溢出中断）委托到 VS-mode，使 hypervisor 能够将硬件性能计数器溢出中断直接注入 guest。

---

## 概述

RISC-V Sscofpmf 扩展定义了 LCOFI 中断（对应中断位 13），当硬件性能计数器溢出时产生中断请求。该中断通过 `mip`/`sip` 的 LCOFIP 位指示挂起状态，通过 `mie`/`sie` 的 LCOFIE 位控制使能。

在 H 扩展上下文中，`hideleg` 寄存器控制哪些中断从 HS-mode 进一步委托到 VS-mode。**Shlcofideleg 扩展的核心约束**：

> 1. 如果实现了 Shlcofideleg，`hideleg` 的 bit 13 是可写的；否则该位为 read-only zero。
> 2. 当 `hideleg` bit 13 = 0 时，`vsip`.LCOFIP 和 `vsie`.LCOFIE 是 read-only zero。
> 3. 当 `hideleg` bit 13 = 1 时，`vsip`.LCOFIP 和 `vsie`.LCOFIE 分别是 `sip`.LCOFIP 和 `sie`.LCOFIE 的别名。

---

## 测试范围

### 规范来源

- `SPEC/hypervisor.adoc:1276-1283` — `norm:vsip_vsie_lcofi`：Shlcofideleg 扩展对 hideleg[13]、vsip.LCOFIP、vsie.LCOFIE 的行为定义
- `SPEC/hypervisor.adoc:1248-1275` — vsip/vsie 寄存器基础定义（VSXLEN-bit R/W，V=1 时替代 sip/sie）
- `SPEC/sscofpmf.adoc` — Sscofpmf 扩展：LCOFI 中断定义（bit 13）、OF 位语义

### 关键参考文件

| 路径 | 说明 |
|------|------|
| `SPEC/hypervisor.adoc:1276-1283` | `norm:vsip_vsie_lcofi` — Shlcofideleg 核心规范 |
| `SPEC/hypervisor.adoc:1248-1258` | vsip/vsie 寄存器尺寸、访问属性与 V=1 替换语义 |
| `SPEC/hypervisor.adoc:1285-1300` | `norm:vsip_vsie_sei/sti/ssi` — SEI/STI/SSI 的类似委托模式（结构参考） |
| `SPEC/sscofpmf.adoc` | LCOFI 中断定义：bit 13、LCOFIP/LCOFIE 语义 |
| `common/encoding.h:271` | `CSR_HIDELEG = 0x603` |
| `common/encoding.h:290-291` | `CSR_VSIP = 0x244` / `CSR_VSIE = 0x204` |
| `common/encoding.h:144-145` | `CSR_SIP = 0x144` / `CSR_SIE = 0x104` |
| `Sscofpmf/sscofpmf_encoding.h` | `MIP_LCOFIP` / `IRQ_LCOFI = 13` 定义 |
| `common/encoding.h:317` | `STATEEN_BIT58` — Sscofpmf/LCOFI 状态控制位 |
| `common/hyp/hyp_csr.h:53-54` | `hideleg_write()` / `hideleg_read()` |
| `common/hyp/hyp_priv.h:21,24` | `run_in_vs_mode(fn, arg)` / `run_in_vu_mode(fn, arg)` |
| `common/hyp/hyp_test.h:95-100` | `VS_EXPECT_NO_TRAP(stmt)` — 期望无异常 |
| `DOCS/framework/hypervisor_framework.md` | Hypervisor 测试框架总览 |

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:vsip_vsie_lcofi` (R1) | Extension Shlcofideleg supports delegating LCOFI interrupts to VS-mode. If implemented, `hideleg` bit 13 is writable; otherwise read-only zero. | Shlcofideleg 扩展支持将 LCOFI 中断委托给 VS 模式。若实现，`hideleg` 第 13 位可写；否则只读零。 |
| `norm:vsip_vsie_lcofi` (R2) | When bit 13 of `hideleg` is zero, `vsip`.LCOFIP and `vsie`.LCOFIE are read-only zeros. | 第 13 位为零时，`vsip`.LCOFIP 和 `vsie`.LCOFIE 为只读零。 |
| `norm:vsip_vsie_lcofi` (R3) | Else, they are aliases of `sip`.LCOFIP and `sie`.LCOFIE. | 否则为 `sip`/`sie` 中对应位的别名。 |
| `norm:vsip_vsie_sz_acc_op` | The `vsip` and `vsie` registers are VSXLEN-bit read/write registers that are VS-mode's versions of supervisor CSRs `sip` and `sie`. When V=1, `vsip` and `vsie` substitute for the usual `sip` and `sie`. However, interrupts directed to HS-level continue to be indicated in the HS-level `sip` register, not in `vsip`, when V=1. | `vsip` 和 `vsie` 是 VSXLEN 位读写寄存器，是 VS 模式版本的 `sip` 和 `sie`。V=1 时替代 `sip`/`sie`。但 HS 级中断仍在 HS 级 `sip` 中指示。 |

### 不在测试范围内

- **LCOFI 中断的实际触发与处理**：计数器溢出产生 LCOFI 的功能由 Sscofpmf 测试覆盖
- **mideleg[13] 对 sip/sie 可见性的控制**：属于基础特权级规范，非 Shlcofideleg 特有
- **hideleg 其他位的委托行为**：SEI(bit 10)/STI(bit 6)/SSI(bit 2) 的委托由基础 H 扩展测试覆盖
- **Sscofpmf 扩展的 OF 位、计数器溢出机制**：由独立 Sscofpmf 测试计划覆盖
- **多 hart 场景**：项目为单核测试环境
- **RV32 场景**：仅覆盖 RV64

---

## 前提与约束

> [!IMPORTANT]
> 测试 Shlcofideleg 需要以下前置条件：
> 1. **Sscofpmf 已实现**：LCOFI 中断（bit 13）的存在是 Shlcofideleg 的前提
> 2. **mideleg[13] = 1**：LCOFI 中断必须先从 M-mode 委托到 S/HS-mode，sip/sie 的 LCOFIP/LCOFIE 位才可见
> 3. **使用 M-mode 操作 mip.LCOFIP**：LCOFIP 挂起位由硬件设置，测试中通过 M-mode 的 `CSRS(mip, MIP_LCOFIP)` 手动注入以验证别名关系

### 前置检查策略

```
1. 验证 Sscofpmf 已实现：检查 mip/mie 的 bit 13 是否可操作
2. 设置 mideleg[13] = 1：使 sip.LCOFIP / sie.LCOFIE 在 HS-mode 可见
3. 验证 hideleg[13] 可写（Shlcofideleg 存在性检查）：
   写 1 回读，若为 0 则 TEST_SKIP("Shlcofideleg not implemented")
```

---

## 设计要点

### 1. LCOFI 中断与中断委托层级

LCOFI 中断（bit 13）的委托链为 `mideleg → hideleg`：

- **mideleg[13]=1**：将 LCOFI 从 M-mode 委托到 HS-mode，`sip`.LCOFIP 和 `sie`.LCOFIE 变为 `mip`.LCOFIP 和 `mie`.LCOFIE 的可见映射
- **hideleg[13]=1**（Shlcofideleg）：将 LCOFI 从 HS-mode 进一步委托到 VS-mode，`vsip`.LCOFIP 和 `vsie`.LCOFIE 变为 `sip`.LCOFIP 和 `sie`.LCOFIE 的别名

### 2. 别名（alias）语义的验证策略

"别名"意味着两个 CSR 位指向同一个底层硬件位。验证方法：

- **正向别名**：修改 `sip`.LCOFIP → 读 `vsip`.LCOFIP，值应一致
- **反向别名**：修改 `vsip`.LCOFIP → 读 `sip`.LCOFIP，值应一致
- **双向一致**：任何时刻两者读出值相同

### 3. VS-mode 视角

当 V=1 时，VS-mode 执行 `csrr rd, sip` 实际访问的是 `vsip`：

- hideleg[13]=1 时：VS-mode 通过 `sip` 指令可看到 LCOFIP 的真实值
- hideleg[13]=0 时：VS-mode 通过 `sip` 指令看到的 LCOFIP 始终为 0

### 4. LCOFIP 的操作方式

`LCOFIP` 挂起位通常由硬件在计数器溢出时设置。测试中通过 M-mode 直接写 `mip` 的 bit 13 来模拟中断挂起，避免依赖实际计数器溢出。`LCOFIE` 使能位则可从 HS-mode 通过 `sie` 自由读写。

---

## 测试分组

> [!IMPORTANT]
> 共 5 个测试组、23 个测试用例。Group 1 验证 hideleg[13] 的可写性；Group 2 验证 hideleg[13]=0 时 vsip/vsie LCOFI 位为 read-only zero；Group 3 验证 hideleg[13]=1 时的别名关系；Group 4 从 VS-mode 视角验证端到端行为；Group 5 验证 hideleg 动态切换时的状态一致性。

---

### Group 1：hideleg[13] 可写性验证

**规范依据**：
- `norm:vsip_vsie_lcofi` (R1)：如果实现了 Shlcofideleg，hideleg bit 13 可写

**测试职责**：在 M-mode 下验证 hideleg bit 13 可以被写为 1 和写为 0。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| LCFIDLG-WR-01 | hideleg[13] 可设置为 1 | 写 hideleg bit 13 = 1，回读 | bit 13 = 1 |
| LCFIDLG-WR-02 | hideleg[13] 可清除为 0 | 写 hideleg bit 13 = 0，回读 | bit 13 = 0 |

#### 关键代码示例

```c
#include "test_framework.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"

TEST_REGISTER(test_shlcofideleg_hideleg_bit13_writable);
bool test_shlcofideleg_hideleg_bit13_writable(void) {
    TEST_BEGIN("LCFIDLG-WR: hideleg[13] writable");

    uintptr_t saved = hideleg_read();

    /* WR-01: 写 1 回读 */
    hideleg_write(saved | (1UL << 13));
    TEST_ASSERT("hideleg[13] can be set to 1",
                (hideleg_read() & (1UL << 13)) != 0);

    /* WR-02: 写 0 回读 */
    hideleg_write(hideleg_read() & ~(1UL << 13));
    TEST_ASSERT("hideleg[13] can be cleared to 0",
                (hideleg_read() & (1UL << 13)) == 0);

    hideleg_write(saved);
    HYP_TEST_END();
}
```

---

### Group 2：hideleg[13]=0 时 vsip/vsie LCOFI 位 read-only zero

**规范依据**：
- `norm:vsip_vsie_lcofi` (R2)：当 hideleg[13]=0，vsip.LCOFIP 和 vsie.LCOFIE 为 read-only zero

**测试职责**：hideleg[13]=0 时，无论 sip/sie 中 LCOFI 位如何，vsip/vsie 对应位始终读为 0 且不可写。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| LCFIDLG-RO-01 | vsip.LCOFIP 读为 0 | hideleg[13]=0，读 vsip bit 13 | vsip.LCOFIP = 0 |
| LCFIDLG-RO-02 | vsie.LCOFIE 读为 0 | hideleg[13]=0，读 vsie bit 13 | vsie.LCOFIE = 0 |
| LCFIDLG-RO-03 | vsip.LCOFIP 写入无效 | hideleg[13]=0，写 vsip bit 13 = 1，回读 | vsip.LCOFIP 仍为 0 |
| LCFIDLG-RO-04 | vsie.LCOFIE 写入无效 | hideleg[13]=0，写 vsie bit 13 = 1，回读 | vsie.LCOFIE 仍为 0 |
| LCFIDLG-RO-05 | sip.LCOFIP=1 不影响 vsip | hideleg[13]=0，M-mode 设 mip.LCOFIP=1，读 vsip bit 13 | vsip.LCOFIP = 0 |
| LCFIDLG-RO-06 | sie.LCOFIE=1 不影响 vsie | hideleg[13]=0，设 sie.LCOFIE=1，读 vsie bit 13 | vsie.LCOFIE = 0 |

#### 关键代码示例

```c
#include "test_framework.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"
#include "sscofpmf_encoding.h"

#define LCOFI_BIT  (1UL << 13)

TEST_REGISTER(test_shlcofideleg_ro_vsip_lcofip);
bool test_shlcofideleg_ro_vsip_lcofip(void) {
    TEST_BEGIN("LCFIDLG-RO: vsip/vsie LCOFI read-only zero when hideleg[13]=0");

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);
    uintptr_t saved_mip = CSRR(mip);

    /* 前置：mideleg[13]=1 使 sip.LCOFIP 可见 */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    /* 设置 hideleg[13]=0 */
    hideleg_write(saved_hideleg & ~LCOFI_BIT);

    /* RO-01: vsip.LCOFIP 读为 0 */
    TEST_ASSERT("vsip.LCOFIP reads 0",
                (csr_read(CSR_VSIP) & LCOFI_BIT) == 0);

    /* RO-03: 写 vsip bit 13 应无效 */
    csr_write(CSR_VSIP, csr_read(CSR_VSIP) | LCOFI_BIT);
    TEST_ASSERT("vsip.LCOFIP stays 0 after write",
                (csr_read(CSR_VSIP) & LCOFI_BIT) == 0);

    /* RO-05: 即使 mip.LCOFIP=1，vsip 仍为 0 */
    CSRS(mip, MIP_LCOFIP);
    TEST_ASSERT("vsip.LCOFIP still 0 even when sip.LCOFIP=1",
                (csr_read(CSR_VSIP) & LCOFI_BIT) == 0);

    /* 恢复 */
    CSRC(mip, MIP_LCOFIP);
    CSRW(mip, saved_mip);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}

TEST_REGISTER(test_shlcofideleg_ro_vsie_lcofie);
bool test_shlcofideleg_ro_vsie_lcofie(void) {
    TEST_BEGIN("LCFIDLG-RO: vsie.LCOFIE read-only zero when hideleg[13]=0");

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_sie = csr_read(CSR_SIE);

    hideleg_write(saved_hideleg & ~LCOFI_BIT);

    /* RO-02: vsie.LCOFIE 读为 0 */
    TEST_ASSERT("vsie.LCOFIE reads 0",
                (csr_read(CSR_VSIE) & LCOFI_BIT) == 0);

    /* RO-04: 写 vsie bit 13 应无效 */
    csr_write(CSR_VSIE, csr_read(CSR_VSIE) | LCOFI_BIT);
    TEST_ASSERT("vsie.LCOFIE stays 0 after write",
                (csr_read(CSR_VSIE) & LCOFI_BIT) == 0);

    /* RO-06: 即使 sie.LCOFIE=1，vsie 仍为 0 */
    csr_write(CSR_SIE, saved_sie | LCOFI_BIT);
    TEST_ASSERT("vsie.LCOFIE still 0 even when sie.LCOFIE=1",
                (csr_read(CSR_VSIE) & LCOFI_BIT) == 0);

    csr_write(CSR_SIE, saved_sie);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}
```

---

### Group 3：hideleg[13]=1 时 vsip/vsie 与 sip/sie 的别名验证

**规范依据**：
- `norm:vsip_vsie_lcofi` (R3)：hideleg[13]=1 时，vsip.LCOFIP 是 sip.LCOFIP 的别名，vsie.LCOFIE 是 sie.LCOFIE 的别名

**测试职责**：hideleg[13]=1 后，验证 vsip/vsie 的 LCOFI 位与 sip/sie 双向同步。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| LCFIDLG-ALIAS-01 | sip→vsip 正向别名 (LCOFIP) | 设 sip.LCOFIP=1，读 vsip.LCOFIP | vsip.LCOFIP = 1 |
| LCFIDLG-ALIAS-02 | sip→vsip 正向别名清除 | 清 sip.LCOFIP=0，读 vsip.LCOFIP | vsip.LCOFIP = 0 |
| LCFIDLG-ALIAS-03 | sie→vsie 正向别名 (LCOFIE) | 设 sie.LCOFIE=1，读 vsie.LCOFIE | vsie.LCOFIE = 1 |
| LCFIDLG-ALIAS-04 | sie→vsie 正向别名清除 | 清 sie.LCOFIE=0，读 vsie.LCOFIE | vsie.LCOFIE = 0 |
| LCFIDLG-ALIAS-05 | vsie→sie 反向别名 (LCOFIE) | 设 vsie.LCOFIE=1，读 sie.LCOFIE | sie.LCOFIE = 1 |
| LCFIDLG-ALIAS-06 | vsie→sie 反向别名清除 | 清 vsie.LCOFIE=0，读 sie.LCOFIE | sie.LCOFIE = 0 |
| LCFIDLG-ALIAS-07 | mip.LCOFIP→vsip 传递 | M-mode 设 mip.LCOFIP=1（mideleg[13]=1），读 vsip.LCOFIP | vsip.LCOFIP = 1 |

> [!NOTE]
> **LCOFIP 的写入限制**：`sip.LCOFIP` 在某些实现中可能是只读的（由硬件通过计数器溢出设置，通过 `mip` 清除）。测试通过 M-mode 的 `CSRS(mip, MIP_LCOFIP)` 注入 LCOFIP=1，再从 HS-mode 视角通过 `sip` 和 VS-mode 视角通过 `vsip` 验证别名。

> [!NOTE]
> **vsip.LCOFIP 反向写入**：LCOFIP 由硬件设置，spec 中 `sip.LCOFIP` 可能是只读位（写入需通过 mip）。因此 vsip→sip 的反向别名仅适用于 LCOFIE（使能位），不适用于 LCOFIP（挂起位）。

#### 关键代码示例

```c
#include "test_framework.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"
#include "sscofpmf_encoding.h"

#define LCOFI_BIT  (1UL << 13)

TEST_REGISTER(test_shlcofideleg_alias_lcofip_sip_to_vsip);
bool test_shlcofideleg_alias_lcofip_sip_to_vsip(void) {
    TEST_BEGIN("LCFIDLG-ALIAS: sip.LCOFIP → vsip.LCOFIP alias");

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);

    /* 前置：mideleg[13]=1, hideleg[13]=1 */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    hideleg_write(saved_hideleg | LCOFI_BIT);

    /* ALIAS-01: M-mode 设 LCOFIP=1, 验证 vsip 看到 */
    CSRS(mip, MIP_LCOFIP);
    TEST_ASSERT("vsip.LCOFIP reflects sip.LCOFIP=1",
                (csr_read(CSR_VSIP) & LCOFI_BIT) != 0);

    /* ALIAS-02: M-mode 清 LCOFIP=0, 验证 vsip 也清 */
    CSRC(mip, MIP_LCOFIP);
    TEST_ASSERT("vsip.LCOFIP reflects sip.LCOFIP=0",
                (csr_read(CSR_VSIP) & LCOFI_BIT) == 0);

    /* 恢复 */
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}

TEST_REGISTER(test_shlcofideleg_alias_lcofie_bidirectional);
bool test_shlcofideleg_alias_lcofie_bidirectional(void) {
    TEST_BEGIN("LCFIDLG-ALIAS: sie.LCOFIE ↔ vsie.LCOFIE bidirectional alias");

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_sie = csr_read(CSR_SIE);

    hideleg_write(saved_hideleg | LCOFI_BIT);

    /* ALIAS-03: sie→vsie 正向 */
    csr_write(CSR_SIE, csr_read(CSR_SIE) | LCOFI_BIT);
    TEST_ASSERT("vsie.LCOFIE=1 when sie.LCOFIE=1",
                (csr_read(CSR_VSIE) & LCOFI_BIT) != 0);

    /* ALIAS-04: sie→vsie 清除 */
    csr_write(CSR_SIE, csr_read(CSR_SIE) & ~LCOFI_BIT);
    TEST_ASSERT("vsie.LCOFIE=0 when sie.LCOFIE=0",
                (csr_read(CSR_VSIE) & LCOFI_BIT) == 0);

    /* ALIAS-05: vsie→sie 反向 */
    csr_write(CSR_VSIE, csr_read(CSR_VSIE) | LCOFI_BIT);
    TEST_ASSERT("sie.LCOFIE=1 when vsie.LCOFIE set",
                (csr_read(CSR_SIE) & LCOFI_BIT) != 0);

    /* ALIAS-06: vsie→sie 反向清除 */
    csr_write(CSR_VSIE, csr_read(CSR_VSIE) & ~LCOFI_BIT);
    TEST_ASSERT("sie.LCOFIE=0 when vsie.LCOFIE cleared",
                (csr_read(CSR_SIE) & LCOFI_BIT) == 0);

    csr_write(CSR_SIE, saved_sie);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}
```

---

### Group 4：VS-mode 端到端验证

**规范依据**：
- `norm:vsip_vsie_sz_acc_op`：V=1 时，`csrr sip` / `csrr sie` 实际访问 vsip / vsie
- `norm:vsip_vsie_lcofi` (R2)(R3)：hideleg[13] 控制 VS-mode 对 LCOFI 位的可见性

**测试职责**：在 VS-mode (V=1) 中通过 `csrr sip` / `csrr sie` 指令验证 LCOFI 位的实际可见性，端到端确认 vsip/vsie 的行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| LCFIDLG-VS-01 | VS-mode 读 sip 可见 LCOFIP (hideleg[13]=1) | hideleg[13]=1, M-mode 注入 LCOFIP=1, VS-mode csrr sip | sip（实为 vsip）bit 13 = 1 |
| LCFIDLG-VS-02 | VS-mode 读 sip 不可见 LCOFIP (hideleg[13]=0) | hideleg[13]=0, M-mode 注入 LCOFIP=1, VS-mode csrr sip | sip（实为 vsip）bit 13 = 0 |
| LCFIDLG-VS-03 | VS-mode 读 sie 可见 LCOFIE (hideleg[13]=1) | hideleg[13]=1, HS 设 sie.LCOFIE=1, VS-mode csrr sie | sie（实为 vsie）bit 13 = 1 |
| LCFIDLG-VS-04 | VS-mode 读 sie 不可见 LCOFIE (hideleg[13]=0) | hideleg[13]=0, HS 设 sie.LCOFIE=1, VS-mode csrr sie | sie（实为 vsie）bit 13 = 0 |
| LCFIDLG-VS-05 | VS-mode 写 sie.LCOFIE 生效 (hideleg[13]=1) | hideleg[13]=1, VS-mode csrs sie LCOFI_BIT, HS-mode 读 sie | sie.LCOFIE = 1（因别名） |
| LCFIDLG-VS-06 | VS-mode 写 sie.LCOFIE 无效 (hideleg[13]=0) | hideleg[13]=0, VS-mode csrs sie LCOFI_BIT, HS-mode 读 sie | sie.LCOFIE 不变（因 vsie 为 RO-zero） |

#### 关键代码示例

```c
#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"
#include "sscofpmf_encoding.h"

#define LCOFI_BIT  (1UL << 13)

/* VS-mode helper: read sip (actually vsip when V=1) */
static uintptr_t vsmode_read_sip(uintptr_t arg) {
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, sip" : "=r"(val));
    return val;
}

/* VS-mode helper: read sie (actually vsie when V=1) */
static uintptr_t vsmode_read_sie(uintptr_t arg) {
    (void)arg;
    uintptr_t val;
    asm volatile("csrr %0, sie" : "=r"(val));
    return val;
}

/* VS-mode helper: set sie LCOFI bit (actually vsie when V=1) */
static uintptr_t vsmode_set_sie_lcofi(uintptr_t arg) {
    (void)arg;
    asm volatile("csrs sie, %0" :: "r"(LCOFI_BIT));
    uintptr_t val;
    asm volatile("csrr %0, sie" : "=r"(val));
    return val;
}

TEST_REGISTER(test_shlcofideleg_vs_lcofip_visible);
bool test_shlcofideleg_vs_lcofip_visible(void) {
    TEST_BEGIN("LCFIDLG-VS-01: VS-mode sees LCOFIP when hideleg[13]=1");

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);

    /* 前置：mideleg[13]=1, hideleg[13]=1, 注入 LCOFIP */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    hideleg_write(saved_hideleg | LCOFI_BIT);
    CSRS(mip, MIP_LCOFIP);

    /* VS-mode 读 sip（实际是 vsip），应看到 bit 13 = 1 */
    uintptr_t vs_sip = run_in_vs_mode(vsmode_read_sip, 0);
    TEST_ASSERT("VS-mode sees LCOFIP=1 via sip",
                (vs_sip & LCOFI_BIT) != 0);

    CSRC(mip, MIP_LCOFIP);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}

TEST_REGISTER(test_shlcofideleg_vs_lcofip_hidden);
bool test_shlcofideleg_vs_lcofip_hidden(void) {
    TEST_BEGIN("LCFIDLG-VS-02: VS-mode cannot see LCOFIP when hideleg[13]=0");

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);

    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    hideleg_write(saved_hideleg & ~LCOFI_BIT);
    CSRS(mip, MIP_LCOFIP);

    /* VS-mode 读 sip（实际是 vsip），应看到 bit 13 = 0 */
    uintptr_t vs_sip = run_in_vs_mode(vsmode_read_sip, 0);
    TEST_ASSERT("VS-mode sees LCOFIP=0 via sip",
                (vs_sip & LCOFI_BIT) == 0);

    CSRC(mip, MIP_LCOFIP);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}

TEST_REGISTER(test_shlcofideleg_vs_write_sie_delegated);
bool test_shlcofideleg_vs_write_sie_delegated(void) {
    TEST_BEGIN("LCFIDLG-VS-05: VS-mode write sie.LCOFIE propagates when hideleg[13]=1");

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_sie = csr_read(CSR_SIE);

    /* 前置：hideleg[13]=1, 先清 sie.LCOFIE */
    hideleg_write(saved_hideleg | LCOFI_BIT);
    csr_write(CSR_SIE, saved_sie & ~LCOFI_BIT);

    /* VS-mode 写 sie（实际写 vsie，因别名写入 sie） */
    run_in_vs_mode(vsmode_set_sie_lcofi, 0);

    /* HS-mode 验证 sie.LCOFIE=1 */
    TEST_ASSERT("sie.LCOFIE=1 after VS-mode write",
                (csr_read(CSR_SIE) & LCOFI_BIT) != 0);

    csr_write(CSR_SIE, saved_sie);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}
```

---

### Group 5：hideleg 动态切换一致性

**规范依据**：
- `norm:vsip_vsie_lcofi` (R2)(R3)：hideleg[13] 的变化应立即反映在 vsip/vsie 的行为上

**测试职责**：验证 hideleg[13] 在 1 和 0 之间切换时，vsip/vsie 的 LCOFI 位行为立即且正确地跟随变化。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| LCFIDLG-DYN-01 | hideleg 1→0 切换后 vsip.LCOFIP 变为 0 | 先设 hideleg[13]=1 验证 vsip 可见，再清为 0 | 切换后 vsip.LCOFIP = 0 |
| LCFIDLG-DYN-02 | hideleg 0→1 切换后 vsip.LCOFIP 变为可见 | 先设 hideleg[13]=0，注入 LCOFIP，再设为 1 | 切换后 vsip.LCOFIP = 1 |
| LCFIDLG-DYN-03 | hideleg 反复切换 vsie.LCOFIE 一致性 | 反复切换 hideleg[13]，每次验证 vsie.LCOFIE 行为 | 每次切换后行为与当前 hideleg[13] 一致 |
| LCFIDLG-DYN-04 | hideleg 切换不影响 sip.LCOFIP | 切换 hideleg[13]，验证 sip.LCOFIP 不变 | sip.LCOFIP 始终反映真实挂起状态 |

> [!NOTE]
> **DYN-04 的意义**：hideleg 仅控制中断是否进一步委托到 VS-mode（即 vsip/vsie 是否为别名），不影响 sip/sie 本身的值。切换 hideleg[13] 后，sip.LCOFIP 应保持不变。

#### 关键代码示例

```c
#include "test_framework.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"
#include "sscofpmf_encoding.h"

#define LCOFI_BIT  (1UL << 13)

TEST_REGISTER(test_shlcofideleg_dynamic_toggle);
bool test_shlcofideleg_dynamic_toggle(void) {
    TEST_BEGIN("LCFIDLG-DYN: hideleg[13] dynamic toggle consistency");

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);
    uintptr_t saved_sie = csr_read(CSR_SIE);

    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    CSRS(mip, MIP_LCOFIP);
    csr_write(CSR_SIE, saved_sie | LCOFI_BIT);

    /* DYN-01: hideleg 1→0，vsip.LCOFIP 变为 0 */
    hideleg_write(saved_hideleg | LCOFI_BIT);
    TEST_ASSERT("vsip.LCOFIP=1 when hideleg[13]=1",
                (csr_read(CSR_VSIP) & LCOFI_BIT) != 0);

    hideleg_write(saved_hideleg & ~LCOFI_BIT);
    TEST_ASSERT("vsip.LCOFIP=0 after hideleg[13] cleared",
                (csr_read(CSR_VSIP) & LCOFI_BIT) == 0);

    /* DYN-02: hideleg 0→1，vsip.LCOFIP 变为可见 */
    hideleg_write(saved_hideleg | LCOFI_BIT);
    TEST_ASSERT("vsip.LCOFIP=1 after hideleg[13] re-set",
                (csr_read(CSR_VSIP) & LCOFI_BIT) != 0);

    /* DYN-03: vsie.LCOFIE 反复切换 */
    for (int i = 0; i < 4; i++) {
        bool delegated = (i % 2 == 0);

        if (delegated)
            hideleg_write(hideleg_read() | LCOFI_BIT);
        else
            hideleg_write(hideleg_read() & ~LCOFI_BIT);

        uintptr_t vsie_lcofie = csr_read(CSR_VSIE) & LCOFI_BIT;

        if (delegated) {
            TEST_ASSERT("vsie.LCOFIE visible after toggle",
                        vsie_lcofie != 0);
        } else {
            TEST_ASSERT("vsie.LCOFIE hidden after toggle",
                        vsie_lcofie == 0);
        }
    }

    /* DYN-04: sip.LCOFIP 不受 hideleg 切换影响 */
    hideleg_write(saved_hideleg | LCOFI_BIT);
    uintptr_t sip_before = csr_read(CSR_SIP) & LCOFI_BIT;
    hideleg_write(saved_hideleg & ~LCOFI_BIT);
    uintptr_t sip_after = csr_read(CSR_SIP) & LCOFI_BIT;
    TEST_ASSERT("sip.LCOFIP unchanged after hideleg toggle",
                sip_before == sip_after);

    /* 恢复 */
    CSRC(mip, MIP_LCOFIP);
    csr_write(CSR_SIE, saved_sie);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}
```

---

## 异常场景与边界条件

### 1. Shlcofideleg 未实现时的行为

当平台未实现 Shlcofideleg 时：
- hideleg[13] 应为 read-only zero
- 写入 hideleg[13]=1 并回读，结果为 0
- 此时所有 Group 2-5 的测试不适用，应 `TEST_SKIP`

### 2. mideleg[13] 未委托时的行为

若 mideleg[13]=0（LCOFI 未从 M-mode 委托到 HS-mode）：
- sip.LCOFIP 和 sie.LCOFIE 对 HS-mode 不可见
- hideleg[13]=1 的委托效果无意义
- 测试前置检查必须确保 mideleg[13]=1

### 3. Sscofpmf 未实现

若平台不支持 Sscofpmf：
- mip/sie 中 bit 13 可能不存在
- 测试前置检查应验证 bit 13 可操作，否则 `TEST_SKIP`

### 4. 与 Smstateen 的交互

若实现了 Smstateen/Ssstateen：
- `mstateen0` bit 58（STATEEN_BIT58）控制 Sscofpmf 相关状态的访问
- 当 `hstateen0` bit 58 = 0 时，VS-mode 访问 LCOFI 相关状态可能触发 virtual-instruction exception
- 本测试计划不覆盖此交互（由 Ssstateen 测试覆盖），但测试前置应确保 `mstateen0`/`hstateen0` bit 58 = 1

---

## 附录：相关常量与 API 参考

### CSR 与位定义

| 名称 | 值 | 说明 |
|------|-----|------|
| `CSR_HIDELEG` | `0x603` | Hypervisor 中断委托寄存器 |
| `CSR_MIDELEG` | `0x303` | Machine 中断委托寄存器 |
| `CSR_VSIP` | `0x244` | Virtual supervisor interrupt-pending 寄存器 |
| `CSR_VSIE` | `0x204` | Virtual supervisor interrupt-enable 寄存器 |
| `CSR_SIP` | `0x144` | Supervisor interrupt-pending 寄存器 |
| `CSR_SIE` | `0x104` | Supervisor interrupt-enable 寄存器 |
| `CSR_MIP` | `0x344` | Machine interrupt-pending 寄存器 |
| `CSR_MIE` | `0x304` | Machine interrupt-enable 寄存器 |
| `IRQ_LCOFI` | `13` | LCOFI 中断号 |
| `MIP_LCOFIP` | `1UL << 13` | mip/sip 中 LCOFIP 位掩码 |
| `MIE_LCOFIE` | `1UL << 13` | mie/sie 中 LCOFIE 位掩码 |
| `STATEEN_BIT58` | `1ULL << 58` | mstateen0/hstateen0 中 Sscofpmf/LCOFI 控制位 |

### hideleg bit 13 与 LCOFI 关系

| hideleg[13] | vsip.LCOFIP | vsie.LCOFIE | 行为 |
|-------------|-------------|-------------|------|
| 0 | read-only 0 | read-only 0 | LCOFI 不委托到 VS-mode |
| 1 | alias of sip.LCOFIP | alias of sie.LCOFIE | LCOFI 委托到 VS-mode |

### 类似委托模式对比（结构参考）

| hideleg bit | 中断类型 | vsip 别名 | vsie 别名 | 规范标签 |
|-------------|----------|-----------|-----------|----------|
| bit 2 | SSI | hip.VSSIP | hie.VSSIE | `norm:vsip_vsie_ssi` |
| bit 6 | STI | hip.VSTIP | hie.VSTIE | `norm:vsip_vsie_sti` |
| bit 10 | SEI | hip.VSEIP | hie.VSEIE | `norm:vsip_vsie_sei` |
| bit 13 | LCOFI | sip.LCOFIP | sie.LCOFIE | `norm:vsip_vsie_lcofi` |

> [!NOTE]
> **LCOFI 的别名目标与 SSI/STI/SEI 不同**：SSI/STI/SEI 委托后，vsip/vsie 别名到 `hip`/`hie` 的 VS* 位（例如 hip.VSEIP）。而 LCOFI 委托后，vsip/vsie 别名到 `sip`/`sie` 的 LCOFIP/LCOFIE 位，没有中间的 VS* 位。这是测试验证中的关键区别。

### 测试框架 API

- `hideleg_read()` / `hideleg_write(value)`：hideleg 读/写
- `run_in_vs_mode(fn, arg)`：在 VS-mode (V=1) 执行 fn(arg)
- `csr_read(addr)` / `csr_write(addr, val)`：动态 CSR 读写
- `CSRS(csr, mask)` / `CSRC(csr, mask)`：CSR 置位/清位（编译时 CSR 名）
- `CSRR(csr)`：CSR 读取（编译时 CSR 名）
- `trap_expect_begin()` / `trap_expect_end()`：trap 捕获窗口
- `trap_was_triggered()` / `trap_get_cause()`：trap 状态查询
- `HYP_TEST_END()`：测试结束宏（含 hyp_reset_state）
- `TEST_SKIP(msg)`：跳过测试（平台不支持时）

---

## 测试统计

| 分组 | 测试数量 | 说明 |
|------|----------|------|
| Group 1：hideleg[13] 可写性 | 2 | 基础读写回环 |
| Group 2：hideleg[13]=0 时 read-only zero | 6 | vsip/vsie LCOFI 位的只读零验证 |
| Group 3：hideleg[13]=1 时别名验证 | 7 | sip↔vsip / sie↔vsie 双向别名 |
| Group 4：VS-mode 端到端 | 6 | V=1 下 csrr sip/sie 实际行为 |
| Group 5：动态切换一致性 | 4 | hideleg 切换后即时一致性 |
| **合计** | **25** | |
