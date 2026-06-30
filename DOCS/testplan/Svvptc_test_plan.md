**中文 | [English](../testplan_en/Svvptc_test_plan_en.md)**

# Svvptc 扩展测试计划

本文档描述 Svvptc（Obviating Memory-Management Instructions after Marking PTEs Valid）扩展的测试计划。Svvptc 扩展规定：当 hart 通过显式 store 把叶/非叶 PTE 的 Valid 位**从 0 置为 1**后，操作系统**不再需要**执行 `SFENCE.VMA` / `SINVAL.VMA` 等地址翻译缓存同步指令；该 PTE 更新会在**有界时间**内对该 hart 后续的隐式访问（地址翻译）变得可见。

---

## 概述

RISC-V 特权级规范要求：当软件修改了内存中的 PTE 后，必须执行 `SFENCE.VMA` 或 `SINVAL.VMA` 来同步 hart 的地址翻译缓存（TLB / Page Walk Cache），否则后续访问可能仍然使用过时的翻译。

Svvptc 扩展放宽了**一种特定方向**的同步要求：

- **覆盖场景（Svvptc 保证）**：PTE 的 Valid 位 `V: 0 → 1`，包括叶 PTE 和非叶 PTE。`SFENCE.VMA` 等指令变为**冗余**；硬件保证更新在有界时间内对后续隐式访问可见，代价是**可能偶尔出现一次额外的虚假 page-fault**（spec NOTE: "occasional gratuitous additional page fault"）。
- **不覆盖场景（仍需 sfence）**：PTE 的任何**其他**形式的更新，包括 `V: 1 → 0`、权限降级（如 RW → R）、PA 重映射（PPN 变更）、A/D 位的软件清除等。这些场景仍需调用 `SFENCE.VMA` / `SINVAL.VMA`，由 svinval 测试计划独立覆盖。

实现 Svvptc 的可能微架构方式包括（参见 `SPEC/svvptc.adoc:21-25`）：
- 不实现地址翻译缓存；
- 不缓存 Invalid PTE；
- 用有界定时器自动驱逐 Invalid PTE；
- 让翻译缓存与修改 PTE 的 store 保持一致。

本测试计划聚焦该规范在**单 hart 单线程**场景下的行为：在不调用 sfence.vma 的前提下，验证修改 V=0→1 后**最终**能命中（在 bounded retry 上限内），并以"加 sfence 立即命中"为基线对照。

---

## 测试范围

### 规范来源

| 文件 | 行/章节 | 内容 |
|------|---------|------|
| `SPEC/svvptc.adoc` | 第 1–28 行 | Svvptc 扩展定义；唯一规范点 `norm:Svvptc_explicit_stores_update_valid_bit`；NOTE 说明 OS 可省略 sfence、虚假 page-fault 的容忍 |
| `SPEC/svvptc.adoc` | 第 4–7 行 | 核心规范：`When the Svvptc extension is implemented, explicit stores by a hart that update the Valid bit of leaf and/or non-leaf PTEs from 0 to 1 ... will eventually become visible within a bounded timeframe to subsequent implicit accesses by that hart to such PTEs.` |
| `SPEC/supervisor.adoc` | 翻译算法（步骤 1–10） | sv39 翻译流程；任何中间 PTE 或叶 PTE V=0 时停止并抛出对应类型 page-fault |
| `SPEC/svinval.adoc` | 全文 | SFENCE.VMA / SINVAL.VMA 语义（用于"反向场景必须 sfence"的边界对照，但不在本计划测试范围） |
| `common/vm/vm.h` | 全文 | `pt_init` / `pt_map_page` / `pt_get_pte` / `vm_run_in_smode` / `vm_sfence_vma` 等 VM API |
| `common/vm/page_table.c` | 第 324–339 行 | `pt_get_pte(ctx, va, level)` 实际语义：`level` 是 page table 层级（Sv39 中 `PT_LEVEL_4K=0`/`PT_LEVEL_2M=1`/`PT_LEVEL_1G=2`），返回该层 PT page 中 VPN 索引对应的 PTE 槽位指针 |
| `common/encoding.h` | 全文 | `PTE_V` / `PTE_R` / `PTE_W` / `PTE_X` / `PTE_A` / `PTE_D` / `CAUSE_*` 等宏 |
| `common/platform/{qemu_virt,spike,sail}/platform.h` | — | `PLATFORM_MEM_BASE = 0x80000000`、`PLATFORM_MEM_SIZE = 0x10000000`（256 MiB），三个平台均一致 |
| `svadu/main.c`、`svadu/Makefile`、`svadu/kernel.ld` | — | 子目录组织模板（main.c 入口、TARGET、SPIKE_ISA_EXT、Makefile.common 包含方式、`__vm_test_region_start` / `__vm_test_region_2m_start` 由各子目录 `kernel.ld` 自行 PROVIDE） |
| `svpbmt/tests/test_nonleaf_pbmt.c` | 第 31、190 行 | 现成范例：`pt_get_pte(&ctx, va, PT_LEVEL_2M)` 取 L1 非叶 PTE（指向 L0 的指针）、`pt_get_pte(&ctx, va, PT_LEVEL_1G)` 取 L2 root 非叶 PTE（指向 L1 的指针） |

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svvptc_explicit_stores_update_valid_bit` | When the Svvptc extension is implemented, explicit stores by a hart that update the Valid bit of leaf and/or non-leaf PTEs from 0 to 1 and are visible to a hart will eventually become visible within a bounded timeframe to subsequent implicit accesses by that hart to such PTEs. | 当实现 Svvptc 扩展时，hart 将叶和/或非叶 PTE 的 Valid 位从 0 更新到 1 的显式存储，如果对 hart 可见，最终将在有界时间范围内对该 hart 后续对此类 PTE 的隐式访问可见。 |
| `"bounded time"语义` | — | 有界时间通过 trap-handler 重试 + 最大重试上限 `SVVPTC_MAX_RETRY=1024` 来量化验证：超过上限仍失败则判定不符合 Svvptc |
| `反向边界（基线对照）` | — | V: 0→1 + 显式 sfence.vma → 立即命中（与 Svvptc 路径形成对照，确保用例本身的 PTE 设置正确） |

### 不在测试范围内

- **反向场景**（V: 1→0、权限降级、PPN 重映射、A/D 软件清除）：Svvptc **不**保证此类更新无需 sfence；这些场景由 `docs/svinval_test_plan.md` 等覆盖。
- **Hypervisor 两级翻译场景**：VS-stage / G-stage 下的 Svvptc 行为（涉及 H 扩展，本测试平台不包含）。
- **Sv32 / Sv48 / Sv57 模式**：仅覆盖 Sv39，与现有 svade/svadu/svnapot/svpbmt 测试计划保持一致。
- **多 hart 一致性**：Svvptc 规范明确仅约束**同一 hart**的显式 store 与隐式访问可见性；跨 hart 可见性需通过 IPI + sfence 等机制保障，超出 Svvptc 范畴。本测试框架为单核环境。
- **与 svinval / svadu 的复合交互**：例如"用 SINVAL.VMA 替代 SFENCE.VMA 时 Svvptc 是否仍生效"等组合场景，由各扩展独立计划覆盖，不在此处展开。
- **PMP / Smepmp 与 Svvptc 的交叉**：相关交互由 PMP 系列测试计划独立覆盖。

---

## 设计要点

### 1. Trap-handler 重试策略（验证"bounded time"）

Svvptc 允许"偶尔的虚假 page-fault"，因此修改 V=0→1 后的**首次**隐式访问可能仍触发 page-fault。测试通过以下方式验证"有界时间内最终可见"：

1. 测试代码在 S-mode 下访问目标 va，预期"最终"成功。
2. 自定义 S-mode trap handler 捕获 page-fault 后**不递增 sepc**，而是直接 `sret` 重试同一指令；同时通过 `sscratch` 累加重试计数。
3. 当重试计数超过 `SVVPTC_MAX_RETRY = 1024` 时，trap handler 把 sepc 跳转到一个**逃逸标签**并回写一个失败标记，由测试代码读取并断言失败。
4. 用例最终断言：`访问成功 && 重试次数 ≤ SVVPTC_MAX_RETRY`。

> [!NOTE]
> `SVVPTC_MAX_RETRY = 1024` 为初始保守值，依据：Spike / Sail 等模拟器实现 Svvptc 时通常不缓存 Invalid PTE，重试 0 次即命中；真实硬件（HAPS / RTL）需要数十至数百次。1024 提供两个数量级的余量，且 1024 次 trap 往返仍能在毫秒级完成测试。该上限可在执行阶段根据平台实测分布上调（例如改为可由 Makefile 通过 `-DSVVPTC_MAX_RETRY=N` 注入），但**不可下调到 0**——下调到 0 等价于强制要求"首次必命中"，违反 spec NOTE 关于"occasional gratuitous additional page fault"的容忍。

### 2. 不主动调用 sfence.vma

所有 Group 1–4 用例**故意省略** `sfence.vma`，依赖 Svvptc 的"有界时间最终可见"保证。这是测试的核心立意——若实现错误地缓存 Invalid PTE 且无淘汰机制，该用例将无限重试直至超出上限并失败。

### 3. 直接 store 修改 PTE

测试通过 `pt_get_pte(&ctx, va, level)` 拿到 PTE 指针后用普通 C 赋值（`*pte = new_value;`）来修改 V 位，对应规范中"explicit stores by a hart"。`pt_get_pte` 的 `level` 参数语义见 `common/vm/page_table.c:329`：

| `level` 取值 | Sv39 含义 | 返回值 |
|---|---|---|
| `PT_LEVEL_4K` (0) | 叶层 PT page（L0） | 4K 叶 PTE 槽位指针 |
| `PT_LEVEL_2M` (1) | L1 中间层 PT page | 若 va 用 4K 映射 → L1 非叶 PTE（指向 L0）；若 va 用 2M 大页 → L1 叶 PTE（megapage） |
| `PT_LEVEL_1G` (2) | L2 root PT page | 若 va 用 4K/2M → L2 非叶 PTE（指向 L1）；若 va 用 1G 大页 → L2 叶 PTE（gigapage） |

`svpbmt/tests/test_nonleaf_pbmt.c` 中已有现成范例（第 31 行用 `PT_LEVEL_2M` 取 L1 非叶 PTE、第 190 行用 `PT_LEVEL_1G` 取 L2 root 非叶 PTE）。

**不**使用 `pt_map_page` 重新映射（后者隐含太多副作用）。

> [!NOTE]
> **不需要** `fence rw, rw`：同一 hart 内对同一地址的 store→后续指令读，由 RVWMO 的 program order + 同地址依赖自动保证可见性。`fence rw, rw` 既不替代 `sfence.vma`（不刷 TLB），也不为 PTE store 提供额外语义，反而会让读者误以为它"参与了 Svvptc 路径"。本计划的 PTE store 后只做：`*pte = new_value;`，不加任何 fence。

### 4. fetch 用例的指令预置流程

X 权限叶页在初始 V=0 时**无法**被任何 store 写入指令字节（V=0 既不可读也不可写）。因此所有 fetch 类用例（SVVPTC-4K-03 / SVVPTC-2M-03 / SVVPTC-NL-03）必须按以下三步流程预置：

1. **预置阶段**：先用 `pt_map_page(va, pa, PTE_V|PTE_R|PTE_W|PTE_A|PTE_D, level)` 临时把目标页映射为 RW 权限并 V=1，调用 `init_exec_page(va)`（参考 `svadu/test_helpers.h::init_exec_page`，写 1023 条 `nop` + 1 条 `ret = 0x00008067`），随后 `vm_sfence_vma(0, 0)` 同步。
2. **改装阶段**：用 `pt_get_pte` 拿到 PTE 指针，直接 store 把权限改为 V=0、X=1、A=1（去掉 R/W、清 V），并 `vm_sfence_vma(0, 0)` 同步。**这一步必须 sfence**——V:1→0 与权限降级**不在** Svvptc 保证范围内。
3. **测试阶段**：再次直接 store 把 V 位改为 1（**不** sfence），进入 S-mode 通过函数指针 `((void(*)(void))va)()` 调用，用 trap-handler 重试机制吸收虚假 instruction page-fault；最终断言 `g_svvptc_retry_count <= SVVPTC_MAX_RETRY` 且 `!g_svvptc_escape`。

### 5. 基线对照（Sanity check）

每组核心用例在 Group 5 中都有一条对照：相同的 PTE 修改流程**加上**显式 `sfence.vma`，验证"不依赖 Svvptc 的传统路径"必然首次命中。这能排除"用例本身 PTE 设置错误"等假阳性，提高测试可信度。

### 6. SVVPTC-4K-04 多轮切换中 sfence 的必要性

SVVPTC-4K-04 在每轮**结尾**用 `sfence.vma` 把 V 位**清回 0**。这里 sfence 是**必须的**，因为：

- V: 1→0 属于 Svvptc 保证范围**之外**的 PTE 更新；若不 sfence，TLB 仍可能命中旧 V=1 翻译，导致下一轮的"V=0 起点"实际未生效，整个测试退化为"V 始终 = 1"，无法验证 Svvptc 的正向语义。
- 仅在每轮**起点**的 V=0→1 切换故意省略 sfence，对应 Svvptc 测试目标。

### 7. Trap-handler 重试钩子的实现方案

`common/trap.c` 现有架构是"M-mode 统一处理 + `trap_record.armed` 单次捕获"，无法直接复用为"S-mode 多次重试"（`trap_record.armed` 在首次触发后即 clear）。本测试采用**唯一确定**的子目录本地方案，避免侵入 common：

1. 在 `svvptc/tests/svvptc_strap.S` 提供一个本地 S-mode trap entry `svvptc_smode_trap_entry`（4 字节对齐的 asm stub）。
2. 在 `svvptc/tests/svvptc_strap.S` 同时定义一个 `.globl svvptc_escape_label`（`ret` 单指令的全局符号）作为 escape 跳转目标。
3. 测试主体在 M-mode 设置 `medeleg` 把 page-fault（cause 12 / 13 / 15）委托到 S-mode（`vm_run_in_smode` 已自动做了一部分委托），然后调用 `vm_run_in_smode` 进入 S-mode；S-mode 入口函数的**第一条** C 语句是 `asm volatile("csrw stvec, %0" :: "r"(svvptc_smode_trap_entry));`，把 stvec 委托给本地 entry。
4. S-mode 测试函数返回前还原 stvec：`asm volatile("csrw stvec, %0" :: "r"(saved_stvec));`（`saved_stvec` 通过 `csrr` 在第 3 步前保存）。
5. 退出 S-mode 后回到 M-mode，`medeleg` 由 `vm_run_in_smode` 的对应清理路径还原。

> [!IMPORTANT]
> **不**采用"在 common/trap.c 增加 weak 钩子"的替代方案，原因：(a) 侵入公共代码且其他测试无此需求；(b) common 的 M-mode trap 路径与 S-mode page-fault 处理在概念上是两套机制，混在一起会增加 common 的认知负担；(c) 子目录本地方案完全够用且零侵入。

### 8. 子目录与构建集成

> [!IMPORTANT]
> 本计划阶段**仅产出 `docs/svvptc_test_plan.md`**，不创建 `svvptc/` 子目录、不修改顶层 `Makefile`、不写 C 测试代码。后续实施阶段按本文档创建 `svvptc/main.c`、`svvptc/Makefile`（参考 `svadu/Makefile`，`SPIKE_ISA_EXT = _svvptc`）、`svvptc/kernel.ld`（参考 `svadu/kernel.ld`，需自行 PROVIDE `__vm_test_region_start`、`__vm_test_region_2m_start` 与 `__page_tables_start/_end`）、`svvptc/tests/*.c`，并把 `svvptc` 加入顶层 `Makefile` 的 `EXTENSIONS` 列表。

---

## 测试分组

> [!IMPORTANT]
> 共 5 个测试组、15 个测试用例。所有测试均运行于 Sv39 模式、S-mode 下。每组提供：规范依据、测试职责、测试用例表（ID/名称/描述/预期结果）；Group 1 提供完整 C 代码示例。

---

### Group 1：4 KiB 叶 PTE V: 0→1 后无 sfence 的最终可见性

**规范依据**：
- `norm:Svvptc_explicit_stores_update_valid_bit`：叶 PTE V: 0→1 在有界时间内对后续隐式访问可见。

**测试职责**：在 4 KiB 叶 PTE 上验证三种隐式访问类型（load / store / instruction fetch）在 V=0→1 后均能在 `SVVPTC_MAX_RETRY` 内命中；并通过"多轮 V 位切换 + sfence 清回 0"用例验证可重复性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVVPTC-4K-01 | 4K V=0→1 load 最终可见 | 4 KiB PTE 初始 V=0, R=1, A=1, D=1；store 改 PTE.V=1（无 sfence）；S-mode 反复 load 直至成功 | load 成功；重试次数 ≤ MAX_RETRY |
| SVVPTC-4K-02 | 4K V=0→1 store 最终可见 | 4 KiB PTE 初始 V=0, R=1, W=1, A=1, D=1；store 改 PTE.V=1（无 sfence）；S-mode 反复 store 直至成功 | store 成功；重试次数 ≤ MAX_RETRY |
| SVVPTC-4K-03 | 4K V=0→1 fetch 最终可见 | 4 KiB PTE 初始 V=0, X=1, A=1；store 改 PTE.V=1（无 sfence）；S-mode 跳转执行 | fetch 成功；重试次数 ≤ MAX_RETRY |
| SVVPTC-4K-04 | 4K V 位多轮切换可见性 | 重复 8 轮：`sfence.vma` 把 V 清 0（V:1→0 不在 Svvptc 范围，**必须 sfence**）→ 不带 sfence 把 V 置 1（V:0→1 受 Svvptc 保护）→ load 直至成功；每轮重试计数独立累加 | 每轮均成功；每轮 `retry_count ≤ MAX_RETRY` |

#### 关键代码示例：SVVPTC-4K-01

```c
/* tests/test_helpers.h（节选） */

#ifndef SVVPTC_TEST_HELPERS_H
#define SVVPTC_TEST_HELPERS_H

#include "test_framework.h"
#include "vm/vm.h"

#define SVVPTC_MAX_RETRY  1024

/* 跨用例共享：由 svvptc/tests/svvptc_strap.S 中的本地 S-mode trap entry
 * 直接读写（详见"设计要点 7：Trap-handler 重试钩子的可落地实现方案"）。
 * 进入 S-mode 前测试代码 csrw stvec 委托给 svvptc_smode_trap_entry，
 * 退出 S-mode 后还原 stvec。 */
extern volatile unsigned long g_svvptc_retry_count;
extern volatile bool          g_svvptc_escape;
extern void svvptc_smode_trap_entry(void);   /* 本地 S-mode trap entry */

/* 直接 store 把 PTE 的 V 位置 0 / 1（不调用 sfence）
 * pte 指向页表中的 PTE 槽位（来自 pt_get_pte）。
 * fence rw,rw 仅保证对**该 hart 后续指令**的 program-order 可见性，
 * 不替代 sfence.vma；TLB 同步交给 Svvptc 的"有界时间"保证。 */
static inline void pte_set_valid(uintptr_t *pte, int valid) {
    uintptr_t v = *pte;
    if (valid) v |=  PTE_V;
    else       v &= ~PTE_V;
    *pte = v;
    asm volatile ("fence rw, rw" ::: "memory");
}

#endif /* SVVPTC_TEST_HELPERS_H */
```

```c
/* tests/test_4k_leaf.c — SVVPTC-4K-01 */

#include "test_helpers.h"

extern uintptr_t __vm_test_region_start;
#define test_page ((uintptr_t)&__vm_test_region_start)

/* S-mode 端：循环 load 直至成功或被 trap-handler 标记 escape。
 * Svvptc 的"虚假 page-fault"由 trap-handler 内重试（不增 sepc）吸收，
 * 因此函数体本身只读取一次。 */
static uintptr_t smode_load_with_retry(uintptr_t addr) {
    g_svvptc_retry_count = 0;
    g_svvptc_escape      = false;

    /* 单条 load —— 若触发 page-fault, S-mode trap-handler 不增 sepc
     * 直接 sret 重试；retry_count 在 handler 内自增；
     * 超过 MAX_RETRY 时 handler 把 sepc 设为 escape 标签并置 escape=true。 */
    volatile uintptr_t v = *(volatile uintptr_t *)addr;
    (void)v;
    return 0;
}

TEST_REGISTER(test_svvptc_4k_load);
bool test_svvptc_4k_load(void) {
    TEST_BEGIN("SVVPTC-4K-01: 4K V=0->1 load eventually visible without sfence");

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);

    /* 代码 + UART 恒等映射（参考 svadu/test_helpers.h::setup_code_mapping） */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D,
                              PT_LEVEL_1G);

    /* 在测试区域单独挂一个 4K 叶 PTE，初始 V=0（其余权限位齐全） */
    pt_map_page(&ctx, test_page, test_page,
                PTE_R | PTE_A | PTE_D,    /* 注意：故意不带 PTE_V */
                PT_LEVEL_4K);

    /* 拿到该 PTE 的指针，准备稍后 store 把 V 置 1 */
    uintptr_t *pte = pt_get_pte(&ctx, test_page, PT_LEVEL_4K);
    TEST_ASSERT("pt_get_pte returned a valid pointer", pte != NULL);
    TEST_ASSERT("PTE.V initially 0", (*pte & PTE_V) == 0);

    /* —— 关键步骤 —— 显式 store 把 V: 0->1，故意不调用 sfence.vma */
    pte_set_valid(pte, 1);

    /* S-mode 进入并 load。Svvptc 保证有界时间内成功；
     * 若实现错误地缓存 V=0 且无淘汰，trap-handler 将在 MAX_RETRY 后 escape。 */
    (void)vm_run_in_smode(&ctx, smode_load_with_retry, test_page);

    TEST_ASSERT("load eventually succeeded (no escape)", !g_svvptc_escape);
    TEST_ASSERT("retry count within bound",
                g_svvptc_retry_count <= SVVPTC_MAX_RETRY);
    printf("  retry_count = %lu\n", g_svvptc_retry_count);

    pt_pool_reset();
    TEST_END();
}
```

#### S-mode trap-handler 重试钩子（asm 入口 + C 等价语义）

按"设计要点 7"，本测试**不**修改 `common/trap.c`，而是在 `svvptc/tests/svvptc_strap.S` 提供一个本地 S-mode trap entry，进入 S-mode 前用 `csrw stvec, svvptc_smode_trap_entry` 委托。下面是该 entry 的 asm 骨架与等价 C 语义说明。

```asm
/* svvptc/tests/svvptc_strap.S — 本地 S-mode trap entry（RV64 / Sv39）
 *
 * 寄存器约定：
 *   - 进入时 sscratch 存放一段 16 字节临时栈（在 svvptc/main.c 启动时
 *     由 csrw sscratch, &svvptc_trap_scratch 初始化），用于保存 t0/t1。
 *   - 仅 t0、t1 两个临时寄存器在 handler 中被使用；通过 sscratch 指向
 *     的 scratch 区域保存 + 恢复后，对被中断指令的状态完全透明。
 *   - 其他 GPR 不动；sret 后回到原指令重试或回到 svvptc_escape_label。
 *
 * 全局符号：
 *   g_svvptc_retry_count : volatile uint64_t（svvptc/tests/test_register.c）
 *   g_svvptc_escape      : volatile uint8_t（同上）
 *   svvptc_escape_label  : 本文件内 .globl 的全局逃逸入口（见下方）
 */
    .section .text
    .globl  svvptc_smode_trap_entry
    .align  2
svvptc_smode_trap_entry:
    /* 保存 t0/t1 到 sscratch 指向的 scratch 区 */
    csrrw   t0, sscratch, t0          /* t0 <-> sscratch（t0 现为 scratch 指针） */
    sd      t1,  0(t0)                /* spill 旧 t1 */
    csrr    t1, sscratch              /* t1 = 用户原 t0（仍在 sscratch 中） */
    sd      t1,  8(t0)                /* spill 旧 t0 */
    csrw    sscratch, t0              /* sscratch 重新指向 scratch 区 */

    /* 读取 scause，判断是否为三种 page-fault 之一 */
    csrr    t0, scause
    li      t1, 12                    /* CAUSE_INST_PAGE_FAULT */
    beq     t0, t1, .Lretry
    li      t1, 13                    /* CAUSE_LOAD_PAGE_FAULT */
    beq     t0, t1, .Lretry
    li      t1, 15                    /* CAUSE_STORE_PAGE_FAULT */
    beq     t0, t1, .Lretry
    j       .Lescape                  /* 非 page-fault: 异常 escape */

.Lretry:
    /* 比较 g_svvptc_retry_count 与 SVVPTC_MAX_RETRY (=1024) */
    la      t0, g_svvptc_retry_count
    ld      t1, 0(t0)
    li      t0, 1024                  /* SVVPTC_MAX_RETRY，与 test_helpers.h 同步 */
    bgeu    t1, t0, .Lescape

    /* retry_count += 1 */
    addi    t1, t1, 1
    la      t0, g_svvptc_retry_count
    sd      t1, 0(t0)

    /* 不修改 sepc -> sret 后重新执行同一条 load/store/fetch */
    j       .Lrestore_and_sret

.Lescape:
    /* g_svvptc_escape = 1; sepc = svvptc_escape_label */
    la      t0, g_svvptc_escape
    li      t1, 1
    sb      t1, 0(t0)
    la      t0, svvptc_escape_label
    csrw    sepc, t0
    /* fallthrough */

.Lrestore_and_sret:
    /* 从 scratch 区恢复 t0/t1 */
    csrr    t0, sscratch              /* t0 = scratch 指针 */
    ld      t1,  0(t0)                /* 恢复旧 t1 */
    ld      t0,  8(t0)                /* 恢复旧 t0（最后写，保留 t0 寄存器） */
    sret

/* ===================================================================
 * svvptc_escape_label —— 全局逃逸入口
 *
 * Escape 时 sepc 被改写到此处。该函数不依赖任何调用栈：sret 后处于
 * S-mode、被中断的访问指令的"目标寄存器"已被硬件标记为未写入（因
 * 异常发生在 retire 之前），但 ra/sp 等调用约定寄存器保持不变（因
 * sret 不动 GPR）。这里直接 ret 即从被中断的 S-mode 函数返回到其
 * 调用者（即 vm_run_in_smode 的内部 trampoline）。
 * =================================================================== */
    .globl  svvptc_escape_label
    .align  2
svvptc_escape_label:
    li      a0, 0                     /* 返回值 = 0（与正常返回一致） */
    ret
```

> [!IMPORTANT]
> `svvptc_escape_label` 是一个**真正的全局函数符号**（用 `.globl` 导出、汇编里 `ret` 收尾），不是测试函数体内的内联标号。这样 `la svvptc_escape_label` 在另一个翻译单元里能正确解析。escape 后 `sret` 直接跳到该 stub，`ret` 则按 S-mode 调用约定返回到 `vm_run_in_smode` 的 S-mode trampoline，最终把控制权干净交还给 M-mode 的测试主体。

等价 C 语义（仅供阅读，不参与编译）：

```c
/* 等价 C 语义。实际实现见 svvptc/tests/svvptc_strap.S。
 * 该 handler 由 stvec 直接委托，不经过 common/trap.c 的 m_trap_handler 路径。 */
void svvptc_smode_trap_handler_equiv(void) {
    uintptr_t scause = read_csr(scause);

    if (scause != CAUSE_INST_PAGE_FAULT  &&
        scause != CAUSE_LOAD_PAGE_FAULT  &&
        scause != CAUSE_STORE_PAGE_FAULT) {
        /* 非 page-fault: 不应出现，escape 让测试失败 */
        g_svvptc_escape = 1;
        write_csr(sepc, (uintptr_t)svvptc_escape_label);
        return;   /* sret */
    }

    if (g_svvptc_retry_count >= SVVPTC_MAX_RETRY) {
        g_svvptc_escape = 1;
        write_csr(sepc, (uintptr_t)svvptc_escape_label);
        return;   /* sret */
    }

    g_svvptc_retry_count++;
    /* 不修改 sepc => sret 后重新执行同一条 load/store/fetch；
     * Svvptc 保证 PTE 更新在有界时间内可见，下一次重试有概率命中。 */
}
```

---

### Group 2：2 MiB megapage 叶 PTE V: 0→1 最终可见性

**规范依据**：
- `norm:Svvptc_explicit_stores_update_valid_bit`：规范不区分 PTE 层级，2 MiB megapage 叶 PTE 同样适用。

**测试职责**：在 2 MiB megapage 叶 PTE 上验证 load / store / fetch 三种访问类型在 V=0→1 后均能在 `SVVPTC_MAX_RETRY` 内命中。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVVPTC-2M-01 | 2M V=0→1 load 最终可见 | 2 MiB megapage 初始 V=0, R=1, A=1, D=1；store V=1（无 sfence）；S-mode load | load 成功；重试 ≤ MAX_RETRY |
| SVVPTC-2M-02 | 2M V=0→1 store 最终可见 | 2 MiB megapage 初始 V=0, R=1, W=1, A=1, D=1；store V=1（无 sfence）；S-mode store | store 成功；重试 ≤ MAX_RETRY |
| SVVPTC-2M-03 | 2M V=0→1 fetch 最终可见 | 2 MiB megapage 初始 V=0, X=1, A=1；store V=1（无 sfence）；S-mode 跳转执行 | fetch 成功；重试 ≤ MAX_RETRY |

> [!NOTE]
> 2 MiB 测试区域使用 `__vm_test_region_2m_start`（参考 `svadu/test_helpers.h`）。`pt_get_pte(&ctx, va, PT_LEVEL_2M)` 返回 L1 层 PTE 指针，用 `pte_set_valid` 直接 store。

---

### Group 3：1 GiB gigapage 叶 PTE V: 0→1 最终可见性

**规范依据**：
- `norm:Svvptc_explicit_stores_update_valid_bit`：1 GiB gigapage 叶 PTE 同样适用。

**测试职责**：在 1 GiB gigapage 叶 PTE 上验证 load / store 在 V=0→1 后均能在 `SVVPTC_MAX_RETRY` 内命中。fetch 不在 1G 用例中单独覆盖（与 2M 等效，无额外覆盖价值）。

> [!IMPORTANT]
> **不可行性分析**：要在 1 GiB gigapage 叶 PTE 上验证 Svvptc，必须把 `0x80000000` 起的整段 1 GiB 虚拟地址用一条**单独的 1G 叶 PTE**覆盖、再把 V 位清 0，使翻译走该叶 PTE 的 V=0 路径。但是：
>
> 1. 测试运行所需的代码段、stack、`__page_tables_*` 池、`__smode_stack_*`、UART MMIO 都通过 `kernel.ld`（参考 `svadu/kernel.ld`）落在 `[0x80000000, 0x80000000 + 256 MiB)` 范围内（`PLATFORM_MEM_SIZE = 0x10000000`）。
> 2. 一旦该 1G 叶 PTE 覆盖整段 `[0x80000000, 0x80000000 + 1 GiB)` 虚拟地址，**所有**上述必须可访问的区域都会被同一条 PTE 控制，无法在 V=0 期间继续取指 / 访问栈 / 访问页表。
> 3. Sv39 翻译过程中 leaf-at-L2 PTE 的存在会让任何 4K / 2M PTE 在该 1G 子空间内被**遮蔽**（叶 PTE 一旦命中即结束翻译），因此无法用 4K / 2M PTE "桥接"代码段。
> 4. 唯一干净的解决方案是把代码/栈/页表/MMIO 全部搬迁到 `[0x80000000 + 1 GiB, ...)` 之外的物理内存，而当前三个平台的物理内存上限就是 `0x80000000 + 256 MiB`，没有可搬迁的目标地址。
>
> 结论：**SVVPTC-1G-* 在当前 `PLATFORM_MEM_SIZE = 256 MiB` 的三个平台（qemu_virt / spike / sail）上无法用纯软件方法干净落地。**

> [!IMPORTANT]
> 因此本计划**确定**：SVVPTC-1G-* 两个用例统一采用 `TEST_SKIP` 实现，并打印明确的跳过原因（`"1G gigapage test skipped: needs PLATFORM_MEM_SIZE > 1 GiB to host code/stack outside the gigapage region; current = 256 MiB"`）。规范覆盖维度由 Group 1（4K）+ Group 2（2M）+ Group 4（非叶）共同完成；1G gigapage 与 2M megapage 在 Svvptc 语义上**完全等价**（都是 leaf-PTE V:0→1，规范文本 `leaf and/or non-leaf PTEs from 0 to 1` 不区分 leaf 的具体层级），不构成规范覆盖空缺。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVVPTC-1G-01 | 1G V=0→1 load 占位（跳过） | 函数体仅一条 `TEST_SKIP("1G gigapage test skipped: needs PLATFORM_MEM_SIZE > 1 GiB ...")` | `[SKIP]` 输出，不计入失败 |
| SVVPTC-1G-02 | 1G V=0→1 store 占位（跳过） | 同 SVVPTC-1G-01 | `[SKIP]` 输出，不计入失败 |

> [!NOTE]
> 保留 SVVPTC-1G-01 / -02 两个 `TEST_SKIP` 用例的目的：在测试报告中显式记录"1G 维度被有意识地跳过及其原因"，避免后续维护者误以为是遗漏。这种"`TEST_SKIP` + 明确原因"是 `common/test_framework.h:99` 中 `TEST_SKIP` 宏的标准用法，与 `svnapot/tests/test_sv_modes.c:49`（"Sv48 not supported on this platform"）、`svadu/tests/test_csr_adue.c:32`（"Platform does not implement Svadu"）等已有跳过用例风格完全一致——**不是未实现的占位，而是确定的、有依据的工程决策**。

---

### Group 4：非叶 PTE V: 0→1 最终可见性

**规范依据**：
- `norm:Svvptc_explicit_stores_update_valid_bit`：明确包含 **non-leaf PTEs**。即使叶 PTE 已完全有效，只要路径上任意一个非叶 PTE 的 V=0，翻译就会停在该层并触发 page-fault；对该非叶 PTE 做 V=0→1 显式 store 同样受 Svvptc 保护。

**测试职责**：构造"非叶 PTE V=0、叶 PTE V=1 且权限齐全"的页表布局，再 store 把非叶 PTE V 位置 1，验证最终访问能命中。覆盖中间层（L1）与次顶层（L2）两个非叶层级。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVVPTC-NL-01 | L1 非叶 V=0→1 后访问叶页 | 4K 叶 PTE 完整有效（V=1, R=1, A=1, D=1），但其上层 L1 非叶 PTE V=0 → store L1 PTE V=1（无 sfence）→ S-mode load 4K 叶页 | load 成功；重试 ≤ MAX_RETRY |
| SVVPTC-NL-02 | L2 非叶 V=0→1 后访问叶页 | 4K 叶 PTE 完整有效，L1 非叶 V=1，但 L2 非叶 PTE V=0 → store L2 PTE V=1（无 sfence）→ S-mode load | load 成功；重试 ≤ MAX_RETRY |
| SVVPTC-NL-03 | L1 非叶 V=0→1 后 fetch | 4K 叶 PTE V=1, X=1, A=1；其上层 L1 非叶 V=0 → store L1 V=1（无 sfence）→ S-mode 跳转执行 | fetch 成功；重试 ≤ MAX_RETRY |

> [!NOTE]
> 构造方法：先用 `pt_map_page(va, pa, PTE_V|PTE_R|PTE_A|PTE_D, PT_LEVEL_4K)` 建立叶页与全部上层非叶 PTE，然后用 `pt_get_pte(&ctx, va, PT_LEVEL_2M)`（拿 L1 非叶 PTE）或 `pt_get_pte(&ctx, va, PT_LEVEL_1G)`（拿 L2 非叶 PTE）取得非叶 PTE 指针，用 `pte_set_valid(pte, 0)` 清 V，sfence 后再 `pte_set_valid(pte, 1)`（不 sfence），进入 S-mode 验证。

---

### Group 5：基线对照 / Sanity check

**规范依据**：
- 反向边界：Svvptc 之外的传统路径（V: 0→1 + 显式 sfence）必然首次命中，用于排除"用例本身 PTE 设置错误"等假阳性。

**测试职责**：与 Group 1 / 2 / 4 形成 1:1 对照——相同的 PTE 修改流程**加上** `sfence.vma`，验证不依赖 Svvptc 时第 1 次访问就能成功（重试次数 = 0）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVVPTC-BASE-01 | 4K V=0→1 + sfence 立即可见 | 与 SVVPTC-4K-01 相同设置，但在 store V=1 后立即 `sfence.vma zero, zero`；S-mode load | 第 1 次 load 成功；`retry_count == 0` |
| SVVPTC-BASE-02 | 2M V=0→1 + sfence 立即可见 | 与 SVVPTC-2M-01 相同设置，附 sfence；S-mode load | 第 1 次 load 成功；`retry_count == 0` |
| SVVPTC-BASE-03 | 非叶 V=0→1 + sfence 立即可见 | 与 SVVPTC-NL-01 相同设置，附 `sfence.vma zero, zero`（rs1=x0 全局刷新覆盖非叶失效）；S-mode load | 第 1 次 load 成功；`retry_count == 0` |

> [!NOTE]
> SVVPTC-BASE-* 的"`retry_count == 0`"是规范无关的工程基线，用于检测"用例本身 PTE 设置错误"或"trap-handler 钩子统计错误"。Svvptc 路径用例（Group 1–4）的"`retry_count <= MAX_RETRY`"则是 Svvptc 的实际合规验证。

---

## 测试用例汇总

| Group | 用例总数 | 实测用例 | TEST_SKIP 用例 | ID 前缀 | 关注点 |
|-------|---------|---------|---------------|---------|--------|
| Group 1 | 4 | 4 | 0 | `SVVPTC-4K-*` | 4 KiB 叶 PTE，三种访问类型 + 多轮切换 |
| Group 2 | 3 | 3 | 0 | `SVVPTC-2M-*` | 2 MiB megapage 叶 PTE |
| Group 3 | 2 | 0 | 2 | `SVVPTC-1G-*` | 1 GiB gigapage 叶 PTE（受 256 MiB 物理内存约束统一跳过） |
| Group 4 | 3 | 3 | 0 | `SVVPTC-NL-*` | 非叶 PTE（L1 / L2） |
| Group 5 | 3 | 3 | 0 | `SVVPTC-BASE-*` | 基线对照（加 sfence） |
| **合计** | **15** | **13** | **2** | — | — |

---

## 实施阶段建议（非本计划范围）

> [!IMPORTANT]
> 以下内容仅供后续实施阶段参考，**不在本计划文档生成阶段执行**。本文档仅产出 `docs/svvptc_test_plan.md`，不创建 `svvptc/` 子目录、不修改顶层 `Makefile`、不写 C 测试代码。

1. 创建 `svvptc/` 子目录，参考 `svadu/` 结构：
   - `main.c`：测试入口（参考 `svadu/main.c`，但**不需要**保存 `g_menvcfg_reset_value`，Svvptc 无 CSR 控制位）；
   - `kernel.ld`：linker script（参考 `svadu/kernel.ld`，需 PROVIDE `__vm_test_region_start`、`__vm_test_region_2m_start`、`__page_tables_start`、`__page_tables_end`、`__smode_stack_start`、`__smode_stack_end` 等所有 common 模块依赖的符号）；
   - `Makefile`：`TARGET = svvptc_test.elf`、`ENABLE_VM = 1`、`SPIKE_ISA_EXT = _svvptc`、`include ../common/Makefile.common`；
   - `tests/`：`test_helpers.h`、`test_register.c`、`test_4k_leaf.c`、`test_2m_megapage.c`、`test_1g_gigapage.c`、`test_non_leaf.c`、`test_baseline.c`，以及一个本地 S-mode trap entry 的 asm 文件（如 `tests/svvptc_strap.S`）。
2. **Trap-handler 重试钩子**：按"设计要点 7"采用子目录本地方案：在 `svvptc/tests/svvptc_strap.S` 实现一个 4 字节对齐的 S-mode trap entry，进入 S-mode 前 `csrw stvec, svvptc_smode_trap_entry` 委托 page-fault；不依赖修改 `common/trap.c`。
3. 在顶层 `Makefile` 的 `EXTENSIONS` 列表（当前为 `pmp smepmp spmp pmp_sv39 pmp_sv48 pmp_sv57 sv39 sv48 sv57 sv39x4 svnapot svinval aia svadu`）末尾追加 `svvptc`。
4. 验证：
   - `make spike-svvptc`：在 `Makefile` 中通过 `SPIKE_ISA_EXT = _svvptc` 把 `_svvptc` 追加到 `--isa=` 字符串。无论 Spike 当前版本是否将 `_svvptc` 作为已识别的 ISA 字符串，本测试的语义不依赖 Spike 显式实现 Svvptc——Spike 不缓存 Invalid PTE 的行为天然满足 Svvptc 规范，所有 Group 1 / 2 / 4 用例的 `retry_count` 在 Spike 上预期为 0，Group 5 基线用例同样为 0，两者输出一致即为合规。若 Spike 因不识别 `_svvptc` 字符串而启动失败，则改为 `SPIKE_ISA_EXT =` 留空，并通过 main.c banner 打印一行 "`Svvptc: relying on Spike's intrinsic 'no Invalid-PTE caching' behavior`"。
   - `make sail-svvptc`：与 spike 相同的处理逻辑。
   - 真实硬件 / HAPS：观察 `retry_count` 分布，若所有用例的 `retry_count < SVVPTC_MAX_RETRY` 且 `g_svvptc_escape == false` 则视为合规；若任一用例触达 escape（`retry_count == SVVPTC_MAX_RETRY` 且 `g_svvptc_escape == true`），则提交硬件缺陷单。
