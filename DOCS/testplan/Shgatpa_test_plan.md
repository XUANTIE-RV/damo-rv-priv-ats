**中文 | [English](../testplan_en/Shgatpa_test_plan_en.md)**

# Shgatpa 扩展测试计划

本文档描述 Shgatpa（Translation Mode Support for hgatp, Version 1.0）扩展的测试计划。Shgatpa 要求：(1) 对于 `satp` 中支持的每一种虚拟内存方案 SvNN，`hgatp` 中对应的 SvNNx4 模式也必须被支持；(2) `hgatp` 的 Bare 模式必须被支持。

---

## 测试范围

### 规范来源

- `SPEC/shgatpa.adoc` — Shgatpa Extension for Translation Mode Support, Version 1.0
- `SPEC/hypervisor.adoc` 第 986–1089 行（`hgatp` 寄存器定义） — 提供 `hgatp` 字段布局、MODE 编码表、WARL 写入语义
- `SPEC/supervisor.adoc` 第 997–1065 行（`satp` 寄存器与 MODE 字段编码） — 提供 satp MODE 编码表，是 Shgatpa 映射关系的源头

### 关键参考文件

| 路径 | 说明 |
|------|------|
| `SPEC/shgatpa.adoc` | Shgatpa 规范全文（共 11 行，2 条 norm） |
| `SPEC/hypervisor.adoc:986-1089` | `hgatp` 寄存器规范：HSXLEN-bit RW、MODE 编码（Bare/Sv39x4/Sv48x4/Sv57x4）、WARL 行为 |
| `SPEC/supervisor.adoc:997-1065` | `satp` 寄存器规范：MODE 编码表（Bare=0, Sv39=8, Sv48=9, Sv57=10）、WARL 行为 |
| `common/encoding.h:284` | `CSR_HGATP = 0x680` |
| `common/encoding.h:145` | `CSR_SATP = 0x180` |
| `common/encoding.h:332-340` | `HGATP64_MODE_SHIFT=60`、`HGATP_MODE_BARE=0`、`HGATP_MODE_SV39X4=8`、`HGATP_MODE_SV48X4=9`、`HGATP_MODE_SV57X4=10` |
| `common/hyp/hyp_defs.h:28-40` | `MAKE_HGATP(mode, vmid, ppn)` 宏、`HGATP_GET_MODE()` 等字段提取宏 |
| `common/hyp/hyp_priv.h:21` | `run_in_vs_mode(fn, arg)` — 在 VS-mode (V=1) 执行测试函数 |
| `common/hyp/hyp_test.h` | Hypervisor 测试宏（`HYP_TEST_END` 等） |
| `common/hyp/hyp_reset.c` | `hyp_reset_state()` 重置 hypervisor CSR 状态 |
| `DOCS/framework/hypervisor_framework.md` | Hypervisor 测试框架总览 |
| `DOCS/testplan/shvsatpa_test_plan.md` | 同族 vsatp 模式支持测试方案，结构参照基准 |

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:shgatpa_satp_hgatp_mode_support` | If the Shgatpa extension is implemented, then for each supported virtual memory scheme Sv_NN_ supported in `satp`, the corresponding hgatp Sv_NN_x4 mode must be supported. | 若实现了 Shgatpa 扩展，则 `satp` 中支持的每种虚拟内存方案 Sv_NN_，`hgatp` 中对应的 Sv_NN_x4 模式也必须支持。 |
| `norm:shgatpa_hgatp_bare_mode` | Furthermore, the `hgatp` mode Bare must also be supported. | 此外，`hgatp` 的 Bare 模式也必须支持。 |
| `norm:hgatp_sz_acc_op` | The `hgatp` register is an HSXLEN-bit read/write register which controls G-stage address translation and protection, the second stage of two-stage translation for guest virtual addresses. | `hgatp` 是一个 HSXLEN 位读写寄存器，控制 G 阶段地址翻译和保护，即客户虚拟地址两阶段翻译的第二阶段。 |
| `norm:hgatp_mode_bare` | When MODE=Bare, guest physical addresses are equal to supervisor physical addresses, and there is no further memory protection for a guest virtual machine beyond the physical memory protection scheme. In this case, software must write zero to the remaining fields in `hgatp`. | 当 MODE=Bare 时，客户物理地址等于 supervisor 物理地址，无额外内存保护。此时软件必须将 `hgatp` 的其余字段写为零。 |
| `norm:hgatp_mode_sv` | When HSXLEN=32, the only other valid setting for MODE is Sv32x4. When HSXLEN=64, modes Sv39x4, Sv48x4, and Sv57x4 are defined. | 当 HSXLEN=32 时，MODE 的唯一其他有效设置为 Sv32x4。当 HSXLEN=64 时，定义了 Sv39x4、Sv48x4 和 Sv57x4 模式。 |
| `norm:hgatp_mode_warl` | A write to `hgatp` with an unsupported MODE value is not ignored as it is for `satp`. Instead, the fields of `hgatp` are WARL in the normal way, when so indicated. | 使用不支持的 MODE 值写入 `hgatp` 不会像 `satp` 那样被忽略。`hgatp` 的字段按常规 WARL 方式处理。 |
| `norm:hgatp_ppn_op` | For the paged virtual-memory schemes, the root page table is 16 KiB and must be aligned to a 16-KiB boundary. In these modes, the lowest two bits of the physical page number (PPN) in `hgatp` always read as zeros. | 对于分页虚拟内存方案，根页表为 16 KiB 且必须对齐到 16 KiB 边界。这些模式下 `hgatp` 中 PPN 的最低两位始终读为零。 |
| `norm:satp_mode_sxlen64` | When SXLEN=64, three paged virtual-memory schemes are defined: Sv39, Sv48, and Sv57. One additional scheme, Sv64, will be defined in a later version. The remaining MODE settings are reserved for future use. | SXLEN=64 时定义了三种分页方案：Sv39、Sv48、Sv57。Sv64 将在后续版本定义。其余 MODE 设置保留。 |
| `norm:satp_mode_op_unsupported` | If a write to `satp` specifies a MODE value that is not supported, the entire write has no effect; no fields in `satp` are modified. | 如果写入 `satp` 的 MODE 值不被支持，整个写入无效；`satp` 中没有任何字段被修改。 |

> [!IMPORTANT]
> Shgatpa 的核心约束有两条：(1) satp 支持 SvNN → hgatp 必须支持 SvNNx4；(2) hgatp.MODE=Bare 必须可用。注意 hgatp 与 satp 的 WARL 行为**不同**：satp 写不支持 MODE 整个写被忽略，而 hgatp 写不支持 MODE 时字段按 WARL 正常处理（`norm:hgatp_mode_warl`）。

### 模式映射关系

| satp MODE | 值 | 对应 hgatp MODE | 值 | 关系 |
|-----------|----|-----------------|----|------|
| Bare | 0 | Bare | 0 | hgatp Bare 独立要求（`norm:shgatpa_hgatp_bare_mode`） |
| Sv39 | 8 | Sv39x4 | 8 | satp 支持 Sv39 → hgatp 必须支持 Sv39x4 |
| Sv48 | 9 | Sv48x4 | 9 | satp 支持 Sv48 → hgatp 必须支持 Sv48x4 |
| Sv57 | 10 | Sv57x4 | 10 | satp 支持 Sv57 → hgatp 必须支持 Sv57x4 |

> [!NOTE]
> 在 HSXLEN=64 时，satp MODE 编码值与对应的 hgatp MODE 编码值**相同**（Bare=0, Sv39/Sv39x4=8, Sv48/Sv48x4=9, Sv57/Sv57x4=10），但它们代表不同的翻译方案（hgatp 的 SvNNx4 多出 2 位 GPA 支持，根页表为 16KB）。

### 不在测试范围内

- **G-stage 地址翻译的查表正确性**：本计划只验证 MODE 字段是否可持有，不验证翻译查表行为
- **vsatp 的 MODE 一致性**：由 Shvsatpa 扩展（`shvsatpa_test_plan.md`）覆盖
- **PPN 字段完整的地址范围验证**：仅做基本写回读验证，不穷举
- **VMID 字段宽度测试**：不在本计划核心范围
- **mstatus.TVM=1 下的 illegal-instruction 行为**：`norm:hgatp_tvm_illegal` 属于 TVM 功能测试
- **多 hart 场景**：项目为单核测试环境
- **HSXLEN=32 场景**：仅覆盖 HSXLEN=64（RV64 环境）
- **HLV/HLVX/HSV/MPRV 场景下的 G-stage 生效行为**：不在本计划范围

---

## 设计要点

### 1. MODE 探测方法

**satp 探测**（与 shvsatpa 相同）：
- satp 的 MODE 是 WARL：写入不支持的值则**整个写被忽略**（`norm:satp_mode_op_unsupported`）
- 策略：先写 Bare（0）建立基线，再写目标 MODE，回读判断

**hgatp 探测**（与 satp 不同！）：
- hgatp 的 MODE 是 WARL 但行为不同：写入不支持的 MODE 值**不会**被忽略，而是字段按 WARL 正常处理（`norm:hgatp_mode_warl`）
- 策略：写入目标 MODE，回读 MODE 字段。如果回读 == 写入值 → 支持；如果回读 != 写入值（被 WARL 强制为其他合法值）→ 不支持

> [!IMPORTANT]
> 这是 hgatp 与 satp/vsatp WARL 行为的关键区别。satp 写不支持 MODE 时整个写被忽略（包括 ASID/PPN 字段），而 hgatp 的各字段独立按 WARL 处理（MODE 被强制为合法值，但 PPN/VMID 可能仍被写入）。

### 2. hgatp 的访问路径

`hgatp`（CSR 0x680）只能在 M-mode 或 HS-mode (V=0) 下直接访问。VS-mode (V=1) 无法直接访问 hgatp（会触发 virtual-instruction exception）。

| 权限模式 | 访问方式 | 说明 |
|----------|----------|------|
| M-mode | `csrr/csrw 0x680` | 直接读写 hgatp |
| HS-mode (V=0) | `csrr/csrw hgatp` | 直接读写（除非 mstatus.TVM=1） |
| VS-mode (V=1) | 不可访问 | 访问 hgatp 触发 virtual-instruction exception |

### 3. MODE 编码（HSXLEN=64）

| 值 | 名称 | 描述 |
|----|------|------|
| 0 | Bare | 无翻译保护（GPA = SPA） |
| 1-7 | — | 保留 |
| 8 | Sv39x4 | 41 位 GPA（Sv39 的 2 位扩展） |
| 9 | Sv48x4 | 50 位 GPA（Sv48 的 2 位扩展） |
| 10 | Sv57x4 | 59 位 GPA（Sv57 的 2 位扩展） |
| 11-15 | — | 保留 |

### 4. PPN 字段的低 2 位约束

在 Sv*x4 模式下，hgatp.PPN[1:0] 始终读零（`norm:hgatp_ppn_op`），因为根页表必须 16KB 对齐。测试需注意：
- 写入 PPN 低 2 位为非零值时，回读应为零
- 仅当 MODE=Bare 时此约束不一定适用

### 5. 与 hyp_reset_state 的隔离

`common/hyp/hyp_reset.c::hyp_reset_state()` 重置 hypervisor CSR 状态。本测试每个用例操作前先备份 `hgatp`，操作完成后还原。

### 6. 探测结果传递

Group 1 探测 `satp` 支持的 MODE 集合后，将结果存入全局变量（如 `g_satp_supports_sv39`），供 Group 2 使用来确定应验证哪些 hgatp 模式。

---

## 测试分组

> [!IMPORTANT]
> 共 6 个测试组、21 个测试用例。Group 1 在 M/HS-mode 探测 satp 支持的模式集；Group 2 验证 hgatp 对应 SvNNx4 模式的支持一致性（核心验证组）；Group 3 验证 hgatp Bare 模式的强制支持；Group 4 验证 hgatp MODE WARL 行为；Group 5 验证 hgatp PPN 字段的低 2 位行为；Group 6 验证 VMID 字段宽度。每组提供：规范依据、测试职责、测试用例表（ID/名称/描述/预期结果）；每组提供 1 个关键 C 代码示例。

---

### Group 1：`satp` MODE 支持探测（基线建立）

**规范依据**：
- `norm:satp_mode_sxlen64`（`SPEC/supervisor.adoc:1044-1050`）：SXLEN=64 时定义 Sv39/Sv48/Sv57
- `norm:satp_mode_op_unsupported`（`SPEC/supervisor.adoc:1052-1055`）：写入不支持的 MODE 整个写无效

**测试职责**：在 M/HS-mode 探测 `satp` 支持的翻译模式集合，建立 Group 2 的比较基线。对每个标准 MODE 值（Bare=0, Sv39=8, Sv48=9, Sv57=10）进行写入-回读测试。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HGATP-PROBE-01 | satp MODE=Bare 探测 | 写 `satp = 0`（MODE=0, ASID=0, PPN=0），回读 MODE 字段 | 回读 MODE=0（Bare 通常被支持） |
| HGATP-PROBE-02 | satp MODE=Sv39 探测 | 先写 Bare 基线，再写 `satp = 8 << 60`（MODE=8），回读 MODE | 回读 MODE ∈ {0, 8}；记录支持状态 |
| HGATP-PROBE-03 | satp MODE=Sv48 探测 | 先写 Bare 基线，再写 `satp = 9 << 60`（MODE=9），回读 MODE | 回读 MODE ∈ {之前有效值, 9}；记录支持状态 |
| HGATP-PROBE-04 | satp MODE=Sv57 探测 | 先写 Bare 基线，再写 `satp = 10 << 60`（MODE=10），回读 MODE | 回读 MODE ∈ {之前有效值, 10}；记录支持状态 |

> [!NOTE]
> 探测逻辑与 shvsatpa 测试计划中的 Group 1 相同。至少一个非 Bare 模式应被支持（否则 H 扩展几乎无法运行 guest OS）。

#### 关键代码示例：HGATP-PROBE-02（Sv39 探测）

```c
/* tests/test_probe.c — HGATP-PROBE-02 */

#include "test_framework.h"

#define SATP_MODE_SHIFT  60
#define SATP_MODE_MASK   (0xFUL << SATP_MODE_SHIFT)
#define SATP_MODE_BARE   (0UL  << SATP_MODE_SHIFT)
#define SATP_MODE_SV39   (8UL  << SATP_MODE_SHIFT)

volatile bool g_satp_supports_sv39 = false;

TEST_REGISTER(test_shgatpa_probe_satp_sv39);
bool test_shgatpa_probe_satp_sv39(void) {
    TEST_BEGIN("HGATP-PROBE-02: probe satp MODE=Sv39 support");

    uintptr_t saved_satp;
    asm volatile ("csrr %0, satp" : "=r"(saved_satp));

    /* 先写 Bare 建立基线 */
    asm volatile ("csrw satp, %0" :: "r"(SATP_MODE_BARE));

    /* 写 Sv39 */
    asm volatile ("csrw satp, %0" :: "r"(SATP_MODE_SV39));

    uintptr_t readback;
    asm volatile ("csrr %0, satp" : "=r"(readback));
    uintptr_t mode = (readback & SATP_MODE_MASK) >> SATP_MODE_SHIFT;

    if (mode == 8) {
        g_satp_supports_sv39 = true;
        TEST_LOG("satp supports Sv39");
    } else {
        g_satp_supports_sv39 = false;
        TEST_LOG("satp does NOT support Sv39 (MODE read back as %lu)", mode);
    }

    TEST_ASSERT("satp MODE readback is legal (0 or 8)",
                mode == 0 || mode == 8);

    asm volatile ("csrw satp, %0" :: "r"(saved_satp));
    TEST_END();
}
```

---

### Group 2：`hgatp` SvNNx4 MODE 对 `satp` 支持模式的一致性验证

**规范依据**：
- `norm:shgatpa_satp_hgatp_mode_support`（`SPEC/shgatpa.adoc:4-7`）：satp 支持 SvNN → hgatp 必须支持 SvNNx4
- `norm:hgatp_sz_acc_op`（`SPEC/hypervisor.adoc:989-994`）：`hgatp` 是 HSXLEN-bit RW 寄存器
- `norm:hgatp_mode_sv`（`SPEC/hypervisor.adoc:1020-1027`）：HSXLEN=64 时定义 Sv39x4/Sv48x4/Sv57x4

**测试职责**：对 Group 1 中探测到的 satp 支持的每个 SvNN MODE，在 M/HS-mode 直接写入 `hgatp`（CSR 0x680）对应的 SvNNx4 值并验证回读 MODE 一致，确认 Shgatpa 映射关系成立。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HGATP-MODE-01 | hgatp MODE=Sv39x4 支持 | 仅当 `g_satp_supports_sv39 == true` 时执行；写 `hgatp = MAKE_HGATP(8, 0, 0)`，回读 MODE | `MODE == 8`（Shgatpa 要求 Sv39→Sv39x4 一致） |
| HGATP-MODE-02 | hgatp MODE=Sv48x4 支持 | 仅当 `g_satp_supports_sv48 == true` 时执行；写 `hgatp = MAKE_HGATP(9, 0, 0)`，回读 MODE | `MODE == 9`（Shgatpa 要求 Sv48→Sv48x4 一致） |
| HGATP-MODE-03 | hgatp MODE=Sv57x4 支持 | 仅当 `g_satp_supports_sv57 == true` 时执行；写 `hgatp = MAKE_HGATP(10, 0, 0)`，回读 MODE | `MODE == 10`（Shgatpa 要求 Sv57→Sv57x4 一致） |
| HGATP-MODE-04 | hgatp 模式交叉切换 | 依次写入所有 satp 支持对应的 hgatp 模式（如 Bare→Sv39x4→Sv48x4→Bare），每次回读验证 | 每次回读 MODE == 写入值 |

> [!IMPORTANT]
> 这是 Shgatpa 扩展的核心验证组。如果 satp 支持 Sv39 但 hgatp 不支持 Sv39x4，则 Shgatpa 约束被违反。断言条件为：**对于 satp 支持的每个 SvNN，写入 hgatp 对应 SvNNx4 后回读的 MODE 必须等于写入值**。

#### 关键代码示例：HGATP-MODE-01（hgatp Sv39x4 一致性验证）

```c
/* tests/test_mode.c — HGATP-MODE-01 */

#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_test.h"

extern volatile bool g_satp_supports_sv39;

TEST_REGISTER(test_shgatpa_hgatp_mode_sv39x4);
bool test_shgatpa_hgatp_mode_sv39x4(void) {
    TEST_BEGIN("HGATP-MODE-01: hgatp supports Sv39x4 if satp supports Sv39");

    if (!g_satp_supports_sv39) {
        TEST_SKIP("satp does not support Sv39, skipping hgatp Sv39x4 check");
    }

    uintptr_t saved_hgatp;
    asm volatile ("csrr %0, 0x680" : "=r"(saved_hgatp));

    /* 先写 Bare 建立基线 */
    asm volatile ("csrw 0x680, %0" :: "r"(0UL));

    /* 写 Sv39x4 (MODE=8, VMID=0, PPN=0) */
    uintptr_t write_val = MAKE_HGATP(HGATP_MODE_SV39X4, 0, 0);
    asm volatile ("csrw 0x680, %0" :: "r"(write_val));

    uintptr_t readback;
    asm volatile ("csrr %0, 0x680" : "=r"(readback));
    uintptr_t mode = HGATP_GET_MODE(readback);

    TEST_ASSERT("hgatp.MODE == Sv39x4 (Shgatpa requirement: satp Sv39 -> hgatp Sv39x4)",
                mode == HGATP_MODE_SV39X4);

    asm volatile ("csrw 0x680, %0" :: "r"(saved_hgatp));
    HYP_TEST_END();
}
```

---

### Group 3：`hgatp` Bare 模式强制支持验证

**规范依据**：
- `norm:shgatpa_hgatp_bare_mode`（`SPEC/shgatpa.adoc:9-10`）：`hgatp` 的 Bare 模式必须被支持
- `norm:hgatp_mode_bare`（`SPEC/hypervisor.adoc:1010-1015`）：MODE=Bare 时无翻译保护；选择 Bare 时其余字段写零

**测试职责**：验证 `hgatp` 的 Bare 模式（MODE=0）始终可写入且回读正确，且选择 Bare 后其余字段（VMID/PPN）为零。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HGATP-BARE-01 | hgatp MODE=Bare 写入回读 | 写 `hgatp = 0`（MODE=0, VMID=0, PPN=0），回读 | `hgatp == 0`（全零） |
| HGATP-BARE-02 | hgatp 从 SvNNx4 切回 Bare | 先写支持的 SvNNx4 模式，再写 Bare（全零），回读 | `MODE == 0`，整个寄存器为 0 |
| HGATP-BARE-03 | hgatp Bare 模式 PPN/VMID 归零 | 写 `hgatp = 0`（Bare），回读验证 PPN=0, VMID=0 | `HGATP_GET_PPN(readback) == 0` 且 `HGATP_GET_VMID(readback) == 0` |

> [!NOTE]
> `norm:hgatp_mode_bare` 要求选择 Bare 时"software must write zero to the remaining fields"。如果软件写入 Bare 但 PPN/VMID 非零，行为 UNSPECIFIED。因此 HGATP-BARE-01/03 确保合规写入后状态正确。

#### 关键代码示例：HGATP-BARE-02（从 SvNNx4 切回 Bare）

```c
/* tests/test_bare.c — HGATP-BARE-02 */

#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_test.h"

extern volatile bool g_satp_supports_sv39;

TEST_REGISTER(test_shgatpa_hgatp_bare_switch);
bool test_shgatpa_hgatp_bare_switch(void) {
    TEST_BEGIN("HGATP-BARE-02: hgatp switches from SvNNx4 back to Bare");

    uintptr_t saved_hgatp;
    asm volatile ("csrr %0, 0x680" : "=r"(saved_hgatp));

    /* 先设置为 SvNNx4 模式（使用已知支持的模式） */
    if (g_satp_supports_sv39) {
        uintptr_t sv39x4_val = MAKE_HGATP(HGATP_MODE_SV39X4, 0, 0x1000);
        asm volatile ("csrw 0x680, %0" :: "r"(sv39x4_val));

        uintptr_t mid_readback;
        asm volatile ("csrr %0, 0x680" : "=r"(mid_readback));
        TEST_ASSERT("hgatp in Sv39x4 mode", HGATP_GET_MODE(mid_readback) == HGATP_MODE_SV39X4);
    }

    /* 切回 Bare（全零） */
    asm volatile ("csrw 0x680, %0" :: "r"(0UL));

    uintptr_t readback;
    asm volatile ("csrr %0, 0x680" : "=r"(readback));

    TEST_ASSERT("hgatp.MODE == Bare after switch", HGATP_GET_MODE(readback) == HGATP_MODE_BARE);
    TEST_ASSERT("hgatp == 0 (all fields zero in Bare)", readback == 0);

    asm volatile ("csrw 0x680, %0" :: "r"(saved_hgatp));
    HYP_TEST_END();
}
```

---

### Group 4：`hgatp` MODE WARL 行为验证

**规范依据**：
- `norm:hgatp_mode_warl`（`SPEC/hypervisor.adoc:1073-1076`）：写 hgatp 不支持的 MODE 值**不会**被忽略（与 satp 不同），字段按 WARL 正常处理

**测试职责**：验证写入不支持的 MODE 值时，hgatp 的 WARL 行为——MODE 被强制为合法值，但写入不是被完全忽略（其他字段可能被更新）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HGATP-WARL-01 | 写保留 MODE=7 | 先设 hgatp=Bare；写 `hgatp = MAKE_HGATP(7, 0, 0x2000)`，回读 MODE | MODE != 7（MODE 被 WARL 强制为合法值） |
| HGATP-WARL-02 | 写保留 MODE=15 | 先设 hgatp=Bare；写 `hgatp = MAKE_HGATP(15, 0, 0x3000)`，回读 MODE | MODE != 15（合法值应为 0/8/9/10 中已实现的某值） |
| HGATP-WARL-03 | 写保留 MODE=1 | 先设 hgatp=Bare；写 `hgatp = MAKE_HGATP(1, 0, 0x4000)`，回读 MODE | MODE != 1（1 是保留值，不应被持有） |
| HGATP-WARL-04 | WARL 行为对比：hgatp 不完全忽略写入 | 先设 hgatp=Bare(PPN=0)；写不支持 MODE + PPN=0x5000；回读 PPN | 回读 PPN 可能为 0x5000（WARL 独立处理，不像 satp 那样整个写被忽略）或为 0（实现自由） |
| HGATP-WARL-05 | MODE 回读值始终合法 | 依次写 MODE=1,2,...,15 中所有值，每次回读 MODE | 每次回读 MODE ∈ {0, 8, 9, 10}（仅合法/已实现的值） |

> [!IMPORTANT]
> **HGATP-WARL-04 的特殊性**：这条用例验证 hgatp 与 satp 的 WARL 行为差异。`norm:hgatp_mode_warl` 明确说 hgatp 写不支持 MODE 时"不是被忽略"，而是各字段按 WARL 正常处理。这意味着即使 MODE 被强制为合法值，PPN/VMID 字段仍可能被写入。但具体行为是实现定义的（WARL 允许两种选择），因此此用例采用"观察并记录"而非硬断言 PPN 必须被写入。

#### 关键代码示例：HGATP-WARL-01（保留 MODE 被 WARL 强制）

```c
/* tests/test_warl.c — HGATP-WARL-01 */

#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_test.h"

TEST_REGISTER(test_shgatpa_hgatp_warl_reserved_mode7);
bool test_shgatpa_hgatp_warl_reserved_mode7(void) {
    TEST_BEGIN("HGATP-WARL-01: write reserved MODE=7, WARL forces legal value");

    uintptr_t saved_hgatp;
    asm volatile ("csrr %0, 0x680" : "=r"(saved_hgatp));

    /* 先设 Bare 基线 */
    asm volatile ("csrw 0x680, %0" :: "r"(0UL));

    /* 写 MODE=7（保留值）+ PPN=0x2000 */
    uintptr_t write_val = MAKE_HGATP(7, 0, 0x2000);
    asm volatile ("csrw 0x680, %0" :: "r"(write_val));

    uintptr_t readback;
    asm volatile ("csrr %0, 0x680" : "=r"(readback));
    uintptr_t mode = HGATP_GET_MODE(readback);

    /* MODE=7 是保留值，WARL 应强制为合法值 */
    TEST_ASSERT("hgatp.MODE != 7 (reserved value not held)",
                mode != 7);
    TEST_ASSERT("hgatp.MODE is a legal value (0, 8, 9, or 10)",
                mode == 0 || mode == 8 || mode == 9 || mode == 10);

    asm volatile ("csrw 0x680, %0" :: "r"(saved_hgatp));
    HYP_TEST_END();
}
```

---

### Group 5：`hgatp` PPN 字段低 2 位行为验证

**规范依据**：
- `norm:hgatp_ppn_op`（`SPEC/hypervisor.adoc:1078-1085`）：Sv*x4 模式下 PPN 的低 2 位始终读零（根页表 16KB 对齐约束）

**测试职责**：验证在 Sv*x4 模式下写入 hgatp.PPN 低 2 位为非零值时，回读这些位始终为零；在 Bare 模式下记录行为（规范不强制约束）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HGATP-PPN-01 | Sv39x4 模式 PPN[1:0]=0b11 写入 | 写 `hgatp = MAKE_HGATP(8, 0, 0x2003)`（PPN 低 2 位 = 0b11），回读 PPN | `PPN & 0x3 == 0`（低 2 位强制为零） |
| HGATP-PPN-02 | Sv39x4 模式 PPN 高位保留 | 写 `hgatp = MAKE_HGATP(8, 0, 0x12340)`（PPN 低 2 位 = 0），回读 PPN | `PPN == 0x12340`（高位保留，低 2 位为零） |
| HGATP-PPN-03 | Sv48x4 模式 PPN[1:0] 强制清零 | 仅当支持 Sv48x4；写 PPN=0x5001（低位=01），回读 PPN | `PPN & 0x3 == 0`（低 2 位强制为零） |

#### 关键代码示例：HGATP-PPN-01（PPN 低 2 位强制清零）

```c
/* tests/test_ppn.c — HGATP-PPN-01 */

#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_test.h"

extern volatile bool g_satp_supports_sv39;

TEST_REGISTER(test_shgatpa_hgatp_ppn_low2_forced_zero);
bool test_shgatpa_hgatp_ppn_low2_forced_zero(void) {
    TEST_BEGIN("HGATP-PPN-01: Sv39x4 mode PPN[1:0] forced to zero");

    if (!g_satp_supports_sv39) {
        TEST_SKIP("satp does not support Sv39, Sv39x4 unavailable");
    }

    uintptr_t saved_hgatp;
    asm volatile ("csrr %0, 0x680" : "=r"(saved_hgatp));

    /* 写 Sv39x4 MODE + PPN 低 2 位为 0b11 */
    uintptr_t write_val = MAKE_HGATP(HGATP_MODE_SV39X4, 0, 0x2003);
    asm volatile ("csrw 0x680, %0" :: "r"(write_val));

    uintptr_t readback;
    asm volatile ("csrr %0, 0x680" : "=r"(readback));
    uintptr_t ppn = HGATP_GET_PPN(readback);

    /* PPN[1:0] 在 Sv*x4 模式下必须读零 */
    TEST_ASSERT("hgatp.PPN[1:0] == 0 in Sv39x4 mode",
                (ppn & 0x3UL) == 0);
    /* PPN 高位应保留写入值 */
    TEST_ASSERT("hgatp.PPN high bits preserved (0x2000)",
                (ppn & ~0x3UL) == 0x2000UL);

    asm volatile ("csrw 0x680, %0" :: "r"(saved_hgatp));
    HYP_TEST_END();
}
```

---

### Group 6：VMID 字段宽度验证

**规范依据**：
- `norm:hgatp_vmid_op`（`SPEC/hypervisor.adoc:1000-1010`）：hgatp 的 VMID 字段为 WARL，实现可支持少于 14 位（RV64）的 VMID 宽度
- Shgatpa 隐含了"hgatp 支持的 MODE 也意味着 VMID 字段可用"

**测试职责**：验证 VMID 字段的实际支持宽度（通过写全 1 回读确认）；验证切换 MODE 后 VMID 字段是否保留。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HGATP-VMID-01 | VMID 宽度探测 | 写 hgatp 的 VMID 字段为全 1（14 位最大值 0x3FFF），回读确认实际支持的宽度 | 回读 VMID 为连续低位 1（如 0x3FFF 表示 14 位全支持，0x003F 表示仅支持 6 位） |
| HGATP-VMID-02 | MODE 切换后 VMID 保留 | 写 hgatp MODE=Sv39x4 + VMID=0x1234；切换 MODE 为 Bare 再切回 Sv39x4；回读 VMID | VMID 值可能被清零（WARL 允许），记录行为但不做 pass/fail 判断 |

#### 关键代码示例：HGATP-VMID-01

```c
/* tests/hyp/test_shgatpa_vmid.c — HGATP-VMID-01 */

#include "hyp/hyp_test.h"
#include "hyp/hyp_defs.h"

#define HGATP_VMID_SHIFT  44
#define HGATP_VMID_MASK   (0x3FFFUL << HGATP_VMID_SHIFT)

TEST_REGISTER(test_shgatpa_vmid_width);
bool test_shgatpa_vmid_width(void) {
    TEST_BEGIN("HGATP-VMID-01: VMID field width probe");

    uintptr_t saved;
    asm volatile ("csrr %0, 0x680" : "=r"(saved));

    /* 写 MODE=Sv39x4 + VMID=全1 + PPN=0 */
    uintptr_t write_val = MAKE_HGATP(HGATP_MODE_SV39X4, 0x3FFF, 0);
    asm volatile ("csrw 0x680, %0" :: "r"(write_val));

    uintptr_t readback;
    asm volatile ("csrr %0, 0x680" : "=r"(readback));
    uintptr_t vmid_rb = (readback & HGATP_VMID_MASK) >> HGATP_VMID_SHIFT;

    /* VMID 应为连续低位1（WARL，实现可截断高位） */
    TEST_ASSERT("VMID field supports at least 1 bit", vmid_rb > 0);
    /* 记录实际宽度供调试 */
    unsigned int width = 0;
    for (uintptr_t v = vmid_rb; v & 1; v >>= 1) width++;
    TEST_NOTE("VMID supported width: %u bits (readback=0x%lx)", width, vmid_rb);

    asm volatile ("csrw 0x680, %0" :: "r"(saved));
    HYP_TEST_END();
}
```

---

## 附录：相关常量与 API 参考

### CSR 与位定义

| 名称 | 值 | 说明 |
|------|-----|------|
| `CSR_HGATP` | `0x680` | Hypervisor guest address translation and protection，见 `common/encoding.h:284` |
| `CSR_SATP` | `0x180` | Supervisor address translation and protection，见 `common/encoding.h:145` |
| `HGATP64_MODE_SHIFT` | 60 | MODE 字段在 HSXLEN=64 时的起始 bit 位置 |
| `HGATP64_VMID_SHIFT` | 44 | VMID 字段起始 bit |
| `HGATP64_PPN_MASK` | `(1UL << 44) - 1` | PPN 字段掩码（44 bit） |
| `HGATP64_VMID_MASK` | `(1UL << 14) - 1` | VMID 字段掩码（最多 14 bit） |

### MODE 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| `HGATP_MODE_BARE` | 0 | 无翻译保护（GPA=SPA） |
| `HGATP_MODE_SV39X4` | 8 | 41-bit GPA 分页 |
| `HGATP_MODE_SV48X4` | 9 | 50-bit GPA 分页 |
| `HGATP_MODE_SV57X4` | 10 | 59-bit GPA 分页 |

### satp → hgatp 模式映射

| satp MODE 常量 | satp 值 | hgatp MODE 常量 | hgatp 值 | 映射依据 |
|---------------|---------|-----------------|---------|---------|
| `MODE_BARE` | 0 | `HGATP_MODE_BARE` | 0 | `norm:shgatpa_hgatp_bare_mode`（独立要求） |
| `MODE_SV39` | 8 | `HGATP_MODE_SV39X4` | 8 | `norm:shgatpa_satp_hgatp_mode_support` |
| `MODE_SV48` | 9 | `HGATP_MODE_SV48X4` | 9 | `norm:shgatpa_satp_hgatp_mode_support` |
| `MODE_SV57` | 10 | `HGATP_MODE_SV57X4` | 10 | `norm:shgatpa_satp_hgatp_mode_support` |

### 全局变量

| 变量 | 类型 | 说明 |
|------|------|------|
| `g_satp_supports_bare` | `volatile bool` | satp 是否支持 Bare 模式 |
| `g_satp_supports_sv39` | `volatile bool` | satp 是否支持 Sv39 模式 |
| `g_satp_supports_sv48` | `volatile bool` | satp 是否支持 Sv48 模式 |
| `g_satp_supports_sv57` | `volatile bool` | satp 是否支持 Sv57 模式 |

### 测试框架 API

- `MAKE_HGATP(mode, vmid, ppn)`：构造 hgatp 寄存器值
- `HGATP_GET_MODE(v)` / `HGATP_GET_PPN(v)` / `HGATP_GET_VMID(v)`：提取 hgatp 字段
- `hyp_reset_state()`：重置所有 hypervisor CSR
- `HYP_TEST_END()`：测试结束宏，含 hyp_reset_state + 结果记录
- `TEST_REGISTER` / `TEST_BEGIN` / `TEST_ASSERT` / `TEST_SKIP` / `TEST_END`：测试用例注册与断言宏
- `TEST_LOG(fmt, ...)`：测试日志输出

---

## 测试执行说明

### 运行环境

- Group 1：M/HS-mode 直接操作 CSR 0x180（satp），探测支持模式
- Group 2：M/HS-mode 直接操作 CSR 0x680（hgatp），验证 SvNNx4 模式一致性
- Group 3：M/HS-mode 直接操作 CSR 0x680，验证 Bare 模式支持
- Group 4：M/HS-mode 直接操作 CSR 0x680，验证 WARL 行为
- Group 5：M/HS-mode 直接操作 CSR 0x680，验证 PPN 低 2 位行为
- 单核环境，无需 IPI
- 所有测试在 M-mode 或 HS-mode (V=0) 执行（hgatp 只能从 V=0 访问）

### 失败定位指引

| 现象 | 可能原因 |
|------|----------|
| HGATP-PROBE-02/03/04 全部显示不支持 | 平台未实现任何分页模式（仅 Bare），需确认硬件能力 |
| HGATP-MODE-01 失败（satp 支持 Sv39 但 hgatp 不支持 Sv39x4） | **Shgatpa 约束违反**：hgatp.MODE 未实现 Sv39x4 |
| HGATP-MODE-02/03 失败 | 同上，hgatp 未实现 satp 支持的高级翻译模式对应的 x4 版本 |
| HGATP-BARE-01/02 失败（MODE 写 0 回读非 0） | hgatp Bare 模式未实现，**违反 `norm:shgatpa_hgatp_bare_mode`** |
| HGATP-WARL-01/02/03 回读 MODE 等于保留值 | hgatp.MODE 的 WARL 实现异常（不应保持非法值） |
| HGATP-WARL-05 失败 | MODE 回读为 1-7 或 11-15 中的保留值，WARL 未正确工作 |
| HGATP-PPN-01/03 失败（PPN[1:0] != 0） | hgatp.PPN 低 2 位未正确实现为只读零，违反 `norm:hgatp_ppn_op` |
