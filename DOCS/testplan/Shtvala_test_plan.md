**中文 | [English](../testplan_en/Shtvala_test_plan_en.md)**

# Shtvala 扩展测试计划

本文档描述 Shtvala（Trap Value Reporting for Hypervisor, Version 1.0）扩展的测试计划。Shtvala 在 H 扩展基础之上**仅约束 `htval` CSR 的写入行为**：把 H 扩展中允许实现"或写零或写 GPA>>2"的弱化写法，**强制收紧为必须写入故障 guest physical address（GPA）右移 2 位的值**。`mtval2`、`stval`、`vsatp`、`hgatp` 等寄存器的行为均**不在 Shtvala 范围内**，由 H 扩展基线测试方案覆盖。

---

## 测试范围

### 规范来源

- `SPEC/shtvala.adoc` — Shtvala Extension for Trap Value Reporting, Version 1.0（共 6 行，单 normative rule）
- `SPEC/hypervisor.adoc` — "H" Extension for Hypervisor Support, Version 1.0
  - Hypervisor Trap Value (`htval`) Register（L904-967）
  - Two-Stage Address Translation（L1869-1899）
  - Guest Physical Address Translation（L1901-1976）
  - Guest-Page Faults（L2041-2067）
  - hstatus 相关字段（L321-326）

### 关键参考文件

| 路径 | 说明 |
|------|------|
| `SPEC/shtvala.adoc` | Shtvala v1.0 全文（6 行） |
| `SPEC/hypervisor.adoc` | H 扩展规范（含 htval/Sv*x4/GPF 描述） |
| `common/trap.c:31` | `trap_record.htval` 字段，统一存 GPA>>2 |
| `common/trap.c:189` | HS-mode trap handler 读 `htval` (CSR 0x643) 写入 `trap_record.htval` |
| `common/trap.c:502` | `trap_get_htval()` 函数声明，返回最近一次 trap 的 htval（GPA>>2） |
| `common/hyp/hyp_test.h:21-25` | `HYP_TEST_END()` 宏，复位 hypervisor 状态 |
| `common/hyp/hyp_test.h:28-37` | `EXPECT_GUEST_PAGE_FAULT(cause, stmt)` 宏 |
| `common/hyp/hyp_test.h:39-42` | `CHECK_HTVAL(msg, expected_gpa_shifted)` 宏，断言 htval == 预期 GPA>>2 |
| `common/hyp/hyp_test.h:44-46` | `CHECK_HTINST(msg, expected)` 宏 |
| `common/hyp/hyp_test.h:48-50` | `CHECK_GVA(msg, expected)` 宏 |
| `common/hyp/hyp_priv.h` | `run_in_vs_mode()`、`run_in_vu_mode()` |
| `common/hyp/gstage_pt.h` | G-stage 页表 API（`gstage_pt_init`、`gstage_pt_map_page`） |
| `common/hyp/two_stage.h` | vsatp+hgatp 两阶段联合配置 |
| `common/encoding.h` | `CAUSE_INST_GUEST_PAGE_FAULT (20)` / `CAUSE_LOAD_GUEST_PAGE_FAULT (21)` / `CAUSE_STORE_GUEST_PAGE_FAULT (23)` |
| `DOCS/framework/hypervisor_framework.md` | Hypervisor 测试框架总览 |
| `DOCS/testplan/sstvala_test_plan.md` | 同族 S-mode stval 测试方案，结构参照基准 |
| `DOCS/testplan/hyp_2_stage_translation_test_plan.md` | 两阶段翻译基线（含隐式访问场景） |
| `DOCS/testplan/hyp_gstage_translation_test_plan.md` | G-stage 翻译基线 |

### 覆盖的规范点（norm ID 级锚定）

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:shtvala_htval_faulting_gpa` | If the Shtvala extension is implemented, `htval` must be written with the faulting guest physical address in all circumstances permitted by the ISA. | 若实现了 Shtvala 扩展，在 ISA 允许的所有情况下，`htval` 必须写入故障的客户物理地址。 |
| `norm:htval_sz_acc_op` | The `htval` register is an HSXLEN-bit read/write register. When a trap is taken into HS-mode, `htval` is written with additional exception-specific information, alongside `stval`, to assist software in handling the trap. | `htval` 是一个 HSXLEN 位读写寄存器。陷阱进入 HS 模式时写入额外的异常特定信息以辅助软件处理。 |
| `norm:htval_trapval` | When a guest-page-fault trap is taken into HS-mode, `htval` is written with either zero or the guest physical address that faulted, shifted right by 2 bits. For other traps, `htval` is set to zero. | 客户页错误陷阱进入 HS 模式时，`htval` 写入零或故障的客户物理地址右移 2 位。其他陷阱设为零。 |
| `norm:htval_val` | `htval` is a WARL register that must be able to hold zero and may be capable of holding only an arbitrary subset of other 2-bit-shifted guest physical addresses, if any. | `htval` 是一个 WARL 寄存器，必须能保持零，可能仅能保持其他 2 位右移客户物理地址的任意子集。 |
| `norm:H_guest_page_fault` | Guest-page-fault traps may be delegated from M-mode to HS-mode under the control of `medeleg`, but cannot be delegated to other privilege modes. On a guest-page fault, `mtval` or `stval` is written with the faulting guest virtual address, and `mtval2` or `htval` is written either with zero or with the faulting guest physical address, shifted right by 2 bits. | 客户页错误可通过 `medeleg` 从 M 模式委托给 HS 模式，但不能委托给其他特权模式。客户页错误时 `mtval`/`stval` 写入故障客户虚拟地址，`mtval2`/`htval` 写入零或故障客户物理地址右移 2 位。 |
| `norm:H_straddle` | When an instruction fetch or a misaligned memory access straddles a page boundary, two different address translations are involved. When a guest-page fault occurs, the faulting virtual address may be a page-boundary address that is higher than the instruction's original virtual address. | 当取指或未对齐内存访问跨越页边界时涉及两个地址翻译。客户页错误时故障虚拟地址可能是高于指令原始虚拟地址的页边界地址。 |
| `norm:mtval2_htval_virtaddr` | When a guest-page fault is not due to an implicit memory access for VS-stage address translation, a nonzero guest physical address written to `mtval2`/`htval` shall correspond to the exact virtual address written to `mtval`/`stval`. | 当客户页错误不是由 VS 阶段地址翻译的隐式内存访问引起时，写入 `mtval2`/`htval` 的非零客户物理地址必须对应写入 `mtval`/`stval` 的确切虚拟地址。 |
| `norm:hgatp_mode_x4` | When `hgatp`.MODE specifies a translation scheme of Sv32x4, Sv39x4, Sv48x4, or Sv57x4, G-stage address translation is a variation on the usual page-based scheme. In each case, the size of the incoming address is widened by 2 bits. The root page table is expanded by a factor of 4 to be 16 KiB and must be aligned to a 16 KiB boundary. | 当 `hgatp`.MODE 指定 Sv32x4/Sv39x4/Sv48x4/Sv57x4 时，G 阶段翻译是常规方案的变体。地址宽度增加 2 位，根页表扩大 4 倍为 16 KiB，必须对齐到 16 KiB 边界。 |
| `norm:hgatp_mode_sv39x4` | For Sv39x4, partitioning is identical to Sv39, except with 2 more bits at the high end in VPN[2]. Address bits 63:41 must all be zeros, or else a guest-page-fault exception occurs. | Sv39x4 的分区与 Sv39 相同，但 VPN[2] 高端多 2 位。地址位 63:41 必须全为零，否则发生客户页错误。 |
| `norm:hgatp_mode_sv48x4` | For Sv48x4, partitioning is identical to Sv48, except with 2 more bits at the high end in VPN[3]. Address bits 63:50 must all be zeros, or else a guest-page-fault exception occurs. | Sv48x4 的分区与 Sv48 相同，但 VPN[3] 高端多 2 位。地址位 63:50 必须全为零，否则发生客户页错误。 |
| `norm:hgatp_mode_sv57x4` | For Sv57x4, partitioning is identical to Sv57, except with 2 more bits at the high end in VPN[4]. Address bits 63:59 must all be zeros, or else a guest-page-fault exception occurs. | Sv57x4 的分区与 Sv57 相同，但 VPN[4] 高端多 2 位。地址位 63:59 必须全为零，否则发生客户页错误。 |
| `norm:hstatus_gva_op` | Field GVA (Guest Virtual Address) is written by the implementation whenever a trap is taken into HS-mode. For any trap that writes a guest virtual address to `stval`, GVA is set to 1. For any other trap into HS-mode, GVA is set to 0. | GVA 字段在进入 HS 模式的陷阱时由实现写入。写入客户虚拟地址到 `stval` 的陷阱设置 GVA=1，其他陷阱设置 GVA=0。 |

> [!IMPORTANT]
> Shtvala 规范的核心是**单条收紧规则**：把 H 扩展原文 "either zero or the guest physical address" 改为 "must be the guest physical address"。所有测试用例围绕"触发 GPF → 验证 htval 必须等于 GPA>>2 且 != 0"展开，并在 Shtvala 不直接约束但与 htval 行为联动的场景（如 Sv*x4 高位越界、跨页访问、非 GPF trap 清零等）借助 H 扩展的相邻 norm 完成完整覆盖。

### 不在测试范围内

- **`mtval2` 行为**：Shtvala SPEC 原文不涉及 `mtval2`。`mtval2` 由 H 扩展基线 `norm:mtval2_trapval` 系列约束（含 medeleg=0 不委托、`mstatus.MPRV+MPV` 触发 M-mode 两阶段访问），归 H 扩展基线测试方案。但 Group 5 中 HLV/HLVX/HSV 指令的 GPF 路径（trap 到 M-mode、写 mtval2）作为 Shtvala 约束的对称覆盖纳入本方案
- **`stval` 是否写 GVA 的强约束**：归 Sstvala 扩展（`SPEC/sstvala.adoc`）
- **`htinst` 编码**：归 H 扩展基线（`norm:htinst_val`）
- **Sv*x4 G-stage 翻译路径正确性**：归 `hyp_gstage_translation_test_plan.md`
- **VS-stage 翻译路径正确性**：归 `hyp_2_stage_translation_test_plan.md`
- **`medeleg` / `hedeleg` 委托位 WARL**：归 M/H 扩展基线
- **VMID、ASID 行为**：归 H 扩展基线
- **多 hart 场景**：项目为单核测试环境
- **RV32 / Sv32x4**：仅覆盖 RV64 + Sv39x4/Sv48x4/Sv57x4，与项目其它扩展计划保持一致

---

## 设计要点

### 1. htval 验证的通用模式

所有测试用例遵循统一模式：

```
1. 设置 medeleg + hedeleg 委托 cause 20/21/23 到 HS-mode（避免落到 M-mode 写 mtval2）
2. 配置 vsatp + hgatp（two_stage.h），构造目标 GPF 场景
3. EXPECT_GUEST_PAGE_FAULT(cause, { 进 VS-mode 触发故障的代码 })
4. CHECK_HTVAL("htval == faulting GPA >> 2", expected_gpa >> 2)
5. 额外断言：trap_get_htval() != 0（Shtvala 增量核心 —— 排除写 0 实现）
6. HYP_TEST_END()
```

框架中 `trap_record.htval` 在 HS-mode handler（`common/trap.c:189`）和 M-mode handler（`common/trap.c:180`）中分别从 `htval` 和 `mtval2` 读取，统一存 GPA>>2。本测试方案 **只用 HS-mode 路径**（因 mtval2 不在范围内）。

### 2. faulting GPA 的来源分类

测试中 faulting GPA 有三类来源，必须显式标注：

| 来源类型 | GPA 计算方式 | 锚定 norm |
|---------|-------------|-----------|
| 显式 G-stage 访问 | GVA 经 vsatp 翻译后得到 GPA | `norm:htval_trapval` 主段 |
| 隐式 VS-stage 翻译 | PTE 自身的 GPA（不是原始 GVA 对应的 GPA） | `norm:htval_trapval` 第二段 |
| 跨页 / misaligned | faulting portion（page-boundary 之后的字节）的 GPA | `norm:htval_trapval` 第三段 + `norm:H_straddle` |

### 3. vsatp=Bare 简化技巧

为简化期望 GPA 的计算，**部分用例可设置 `vsatp.MODE=Bare`**，使 GVA 直接等于 GPA。此时 G-stage GPF 的期望 htval 等于 stval >> 2，便于断言。借鉴自 qinghao 方案。

### 4. WARL 验证策略

`norm:htval_val` 允许 htval 仅装下 GPA 子集。验证策略：
- 写入 0 必须能保持 0（强制要求）
- 写入实现宽度（IMPL_HTVAL_WIDTH）内的 faulting GPA>>2 必须能完整保持
- 写入显然超过 IMPL_HTVAL_WIDTH 的值，回读为 WARL 子集（不抛异常）
- Shtvala 联合约束：实现的 WARL 子集必须能装下实现宽度内**所有可能的 faulting GPA**，否则 Shtvala 无法满足

### 5. 委托配置（hedeleg/medeleg）

所有 GPF 用例的前置：
- `medeleg[20] = medeleg[21] = medeleg[23] = 1`（M-mode 委托给 HS）
- `hedeleg[20] = hedeleg[21] = hedeleg[23] = 0`（HS 不委托给 VS，否则 GPF 走 VS handler 不写 htval）

### 6. 平台能力探测

- 用例入口探测 Shtvala 是否实现（具体方式由平台 ABI 决定，如 ISA 字符串 `"_shtvala"` 解析、特性寄存器位、或编译期宏 `ENABLE_SHTVALA`）
- 未实现 Shtvala 时整个文件 `TEST_SKIP`（基线 H 扩展可能写 0，断言 `htval != 0` 会失败）

---

## 测试分组

> [!IMPORTANT]
> 共 9 个测试组、30 个测试用例。
> Group 1 验证 htval 寄存器属性（基础前提）；Group 2-4 是 Shtvala 核心约束（GPF 时 htval 必须写 GPA）；Group 5 覆盖 HLV/HLVX/HSV 指令路径（V=0 trap 到 M-mode，验证 mtval2）；Group 6 覆盖 G-stage 三种 MODE 与 GPA 高位越界；Group 7 验证非 GPF trap 不污染 htval；Group 8 验证 htval/stval 的 GPA 完整重建；Group 9 验证 hstatus.GVA 联动。

---

### Group 1：htval 寄存器属性（基础 RW 与 WARL）

**规范依据**：
- `norm:htval_sz_acc_op`（`SPEC/hypervisor.adoc:906-910`）：htval 是 HSXLEN-bit RW 寄存器
- `norm:htval_val`（`SPEC/hypervisor.adoc:956-959`）：htval 是 WARL，必须能装 0

**测试职责**：验证 htval 寄存器在 M-mode/HS-mode 下可读可写、WARL 行为合法、VS/VU-mode 访问触发 illegal-instruction。这是 Shtvala 后续断言的前提。

**前置条件**：
- M-mode 启动后切换到 HS-mode，确认 misa.H = 1
- 不需要委托或两阶段翻译

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HTVAL-REG-01 | M-mode 写读 htval | M-mode 显式 `csrw htval, 0xDEADBEEF` 后 `csrr` 回读，确认 htval 可读写 | 回读值落在 WARL 子集内（实现允许的 GPA>>2 子集） |
| HTVAL-REG-02 | HS-mode 写读 htval | HS-mode 显式写 `0` 后回读必须为 `0`；写 `MAX_VALID_GPA >> 2` 后回读完整保持 | 写 0 回读 = 0；写合法 GPA>>2 回读完整保持 |
| HTVAL-REG-03 | WARL 子集合法性 | HS-mode 写入显然非法的全 1 值 `~0UL`，回读为 WARL 子集（不抛异常） | 不触发异常；回读值 ⊆ 写入值（按位） |
| HTVAL-REG-04 | VS/VU-mode 访问 htval | VS-mode 执行 `csrr x5, htval` → cause 22 (virtual-instruction)；VU-mode 执行同样 → cause 2 (illegal-instruction) | VS：cause=22；VU：cause=2 |

#### 关键代码示例：HTVAL-REG-02

```c
/* tests/hyp/test_shtvala_reg.c — HTVAL-REG-02 */

#include "hyp/hyp_test.h"

TEST_REGISTER(test_shtvala_htval_rw_hs);
bool test_shtvala_htval_rw_hs(void) {
    TEST_BEGIN("HTVAL-REG-02: HS-mode htval RW preserves GPA>>2");

    SHTVALA_REQUIRE();  /* 平台未实现 Shtvala 则 TEST_SKIP */

    /* 写 0 必须能保持 */
    CSRW(CSR_HTVAL, 0);
    TEST_ASSERT_EQ("htval write 0 preserved",
                   CSRR(CSR_HTVAL), 0UL);

    /* 写实现宽度内的 GPA>>2 必须能完整保持 */
    uintptr_t test_gpa = 0x80100000UL;     /* 41-bit 内合法 GPA */
    uintptr_t shifted  = test_gpa >> 2;    /* 实际写入值 */
    CSRW(CSR_HTVAL, shifted);
    TEST_ASSERT_EQ("htval write GPA>>2 preserved",
                   CSRR(CSR_HTVAL), shifted);

    HYP_TEST_END();
}
```

---

### Group 2：显式 G-stage 访问 GPF（Shtvala 核心：htval 必须非零）

**规范依据**：
- `norm:shtvala_htval_faulting_gpa`（`SPEC/shtvala.adoc:4-6`）：htval 必须写入 faulting GPA
- `norm:htval_trapval`（`SPEC/hypervisor.adoc:916-921`）：GPF 时 htval 写 GPA>>2
- `norm:H_guest_page_fault`（`SPEC/hypervisor.adoc:2043-2051`）：GPF 总规则
- `norm:mtval2_htval_virtaddr`（`SPEC/hypervisor.adoc:2063-2067`）：非 implicit 时 GPA 对应 stval

**测试职责**：当 VS-mode 显式访问触发 G-stage GPF（cause 20/21/23）时，验证 `htval == faulting GPA >> 2` 且 `htval != 0`。这是 Shtvala 与基线 H 扩展的核心区别。

**前置条件**：
- vsatp = Sv39 或 Bare（部分用例用 Bare 简化期望值）
- hgatp = Sv39x4，构造目标 GPA 在 G-stage 的 PTE 为 Invalid / 缺权限
- medeleg[20]=medeleg[21]=medeleg[23]=1，hedeleg[20]=hedeleg[21]=hedeleg[23]=0

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HTVAL-LGP-01 | Load GPF：G-stage Invalid PTE | vsatp=Bare，VS-mode load 至 GVA=GPA=0x80100000，G-stage PTE V=0 → cause 21 | `cause == 21`；`htval == 0x80100000 >> 2 != 0` |
| HTVAL-LGP-02 | Load GPF：G-stage 缺 R 权限 | vsatp=Bare，G-stage PTE V=1, R=0, X=1，VS-mode load → cause 21 | `cause == 21`；`htval == GPA >> 2 != 0` |
| HTVAL-SGP-01 | Store GPF：G-stage 缺 W 权限 | vsatp=Bare，G-stage PTE V=1, R=1, W=0，VS-mode store → cause 23 | `cause == 23`；`htval == GPA >> 2 != 0` |
| HTVAL-SGP-02 | Store GPF：G-stage A=1 D=0 写访问 | G-stage PTE V=1, R=1, W=1, A=1, D=0，VS-mode store → cause 23（无 Svadu） | `cause == 23`；`htval == GPA >> 2 != 0` |
| HTVAL-IGP-01 | Inst GPF：G-stage 缺 X 权限 | vsatp=Bare，G-stage PTE V=1, R=1, X=0，VS-mode 跳转执行 → cause 20 | `cause == 20`；`htval == fault PC GPA >> 2 != 0` |
| HTVAL-LGP-03 | Load GPF：vsatp=Sv39 + G-stage Invalid | vsatp=Sv39 设置 GVA→GPA 翻译，G-stage 该 GPA 为 Invalid → cause 21 | `cause == 21`；`htval == 翻译后 GPA >> 2 != 0`，与 stval（GVA）不同 |
| HTVAL-AMO-01 | AMO 指令 Store GPF | vsatp=Bare，VS-mode 对 G-stage 缺 W 权限的 GPA 执行 `amoadd.d` → cause 23 | `cause == 23`；`htval == GPA >> 2 != 0`（AMO 与 Store 共享 cause 23 但触发路径不同：AMO 需 R+W 权限） |

#### 关键代码示例：HTVAL-LGP-01（vsatp=Bare 简化）

```c
/* tests/hyp/test_shtvala_explicit.c — HTVAL-LGP-01 */

#include "hyp/hyp_test.h"
#include "hyp/two_stage.h"
#include "hyp/gstage_pt.h"

#define TEST_GVA_GPA  0x80100000UL    /* Bare 模式下 GVA == GPA */

static void vs_load_unmapped_gpa(void) {
    volatile uintptr_t *p = (volatile uintptr_t *)TEST_GVA_GPA;
    (void)*p;   /* 触发 load GPF (cause 21) */
}

TEST_REGISTER(test_shtvala_load_gpf_invalid_pte);
bool test_shtvala_load_gpf_invalid_pte(void) {
    TEST_BEGIN("HTVAL-LGP-01: load GPF on G-stage Invalid PTE, htval == GPA>>2");

    SHTVALA_REQUIRE();

    /* 委托 GPF 到 HS-mode */
    delegate_gpf_to_hs();

    /* vsatp=Bare 让 GVA == GPA */
    /* hgatp=Sv39x4，目标 GPA 不在 G-stage 页表中（PTE.V=0） */
    two_stage_setup_bare_vsatp_sv39x4_hgatp();
    /* 不映射 TEST_GVA_GPA，使其在 G-stage 翻译时缺失 */

    EXPECT_GUEST_PAGE_FAULT(CAUSE_LOAD_GUEST_PAGE_FAULT, {
        run_in_vs_mode((vs_func_t)vs_load_unmapped_gpa, 0);
    });

    CHECK_HTVAL("htval == faulting GPA >> 2", TEST_GVA_GPA >> 2);
    TEST_ASSERT("htval != 0 (Shtvala enforcement)",
                trap_get_htval() != 0);

    HYP_TEST_END();
}
```

#### 关键代码示例：HTVAL-LGP-03（vsatp=Sv39 双阶段）

```c
/* tests/hyp/test_shtvala_explicit.c — HTVAL-LGP-03 */

#define TEST_GVA   0x40000000UL
#define TEST_GPA   0x80200000UL

TEST_REGISTER(test_shtvala_load_gpf_two_stage);
bool test_shtvala_load_gpf_two_stage(void) {
    TEST_BEGIN("HTVAL-LGP-03: load GPF after VS-stage translates, htval == GPA>>2");

    SHTVALA_REQUIRE();
    delegate_gpf_to_hs();

    /* vsatp=Sv39: TEST_GVA → TEST_GPA */
    /* hgatp=Sv39x4: TEST_GPA 缺 PTE（Invalid） */
    two_stage_t ts;
    two_stage_init(&ts, SV39, SV39X4);
    two_stage_map_vs(&ts, TEST_GVA, TEST_GPA, PTE_V|PTE_R|PTE_W|PTE_X|PTE_U);
    /* 不在 G-stage 中映射 TEST_GPA → 触发 cause 21 */
    two_stage_activate(&ts);

    EXPECT_GUEST_PAGE_FAULT(CAUSE_LOAD_GUEST_PAGE_FAULT, {
        run_in_vs_mode_load(TEST_GVA);
    });

    /* htval 必须为 G-stage faulting GPA，不是 GVA */
    CHECK_HTVAL("htval == G-stage faulting GPA >> 2", TEST_GPA >> 2);
    TEST_ASSERT("htval != 0", trap_get_htval() != 0);
    /* stval 应为 GVA */
    TEST_ASSERT_EQ("stval == GVA", trap_get_stval(), TEST_GVA);

    HYP_TEST_END();
}
```

---

### Group 3：隐式 VS-stage 翻译 GPF（htval = PTE 自身 GPA）

**规范依据**：
- `norm:shtvala_htval_faulting_gpa`（`SPEC/shtvala.adoc:4-6`）
- `norm:htval_trapval`（`SPEC/hypervisor.adoc:922-929`）："a guest physical address written to `htval` is that of the implicit memory access that faulted—for example, the address of a VS-level page table entry that could not be read"

**测试职责**：验证当 VS-stage 翻译过程中隐式读取 PTE 触发 G-stage GPF 时，`htval` 等于该 **PTE 自身**的 GPA（不是原始 GVA 经 VS-stage 翻译后的 GPA，因为 VS-stage 翻译失败时该值未知）。

**前置条件**：
- vsatp = Sv39（必须，Bare 不会触发隐式访问）
- VS-stage 各级页表的根/中级/叶子 PTE 各占一个 GPA
- hgatp 中故意缺失某一级 PTE 所在的 GPA 映射

> [!NOTE]
> Shtvala 强制了 H 扩展原文中已经描述但允许"写零"的 implicit-VS-stage 路径。隐式访问场景 htval 的低 2 bit 为 0（PTE 物理上 8-byte 对齐）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HTVAL-IMP-01 | Implicit L0：VS 根表 PTE 缺映射 | vsatp Sv39 根页表 GPA 在 hgatp 中未映射，VS-mode load 触发隐式读 → cause 21 | `cause == 21`；`htval == VS 根表 PTE 所在 GPA >> 2 != 0`；与原 GVA 无关 |
| HTVAL-IMP-02 | Implicit L1：VS 中级 PTE 缺映射 | vsatp Sv39 中级页表 GPA 在 hgatp 中未映射，VS-mode load → cause 21 | `cause == 21`；`htval == 中级 PTE 所在 GPA >> 2 != 0` |
| HTVAL-IMP-03 | Implicit L2：VS 叶子 PTE 缺映射 | vsatp Sv39 叶子页表 GPA 在 hgatp 中未映射，VS-mode load → cause 21 | `cause == 21`；`htval == 叶子 PTE 所在 GPA >> 2 != 0` |
| HTVAL-IMP-04 | Implicit + store/inst | 同 HTVAL-IMP-01 但分别用 store/取指 → cause 23/20 | `cause == 23/20`；`htval == 根表 PTE GPA >> 2 != 0` |
| HTVAL-IMP-05 | Implicit read GPF 时 htinst == pseudoinstruction（read） | 复用 HTVAL-IMP-01 场景（隐式读 PTE 触发 GPF），额外验证 `htinst` | `htinst == 0x00003000`（RV64 read pseudoinstruction，`norm:H_trap_xtinst_guestpage`） |
| HTVAL-IMP-06 | Implicit write GPF 时 htinst == pseudoinstruction（write） | vsatp Sv39 构造 VS-stage PTE A=0（需硬件自动置 A），隐式写触发 GPF → cause 23 | `htinst == 0x00003020`（RV64 write pseudoinstruction）；若平台不自动更新 A/D 则 `TEST_SKIP` |
| HTVAL-IMP-07 | Sv48x4 隐式 GPF 路径 | vsatp=Sv48，构造 L2 PTE 所在 GPA 在 G-stage（Sv48x4）未映射 → cause 21 | `htval == L2 PTE GPA >> 2 != 0`；若平台不支持 Sv48 则 `TEST_SKIP` |

#### 关键代码示例：HTVAL-IMP-03

```c
/* tests/hyp/test_shtvala_implicit.c — HTVAL-IMP-03 */

#define TEST_GVA       0x40000000UL
#define LEAF_PT_GPA    0x80300000UL    /* VS-stage 叶子页表所在的 GPA */

TEST_REGISTER(test_shtvala_implicit_leaf);
bool test_shtvala_implicit_leaf(void) {
    TEST_BEGIN("HTVAL-IMP-03: implicit VS-stage leaf PT GPF, htval == leaf PTE GPA>>2");

    SHTVALA_REQUIRE();
    delegate_gpf_to_hs();

    two_stage_t ts;
    two_stage_init(&ts, SV39, SV39X4);
    /* VS-stage：构造 TEST_GVA 的叶子 PTE 放在 LEAF_PT_GPA */
    two_stage_set_vs_leaf_pt_at(&ts, TEST_GVA, LEAF_PT_GPA);
    /* hgatp：根表/中级表的 GPA 都映射，但 LEAF_PT_GPA 不映射 */
    two_stage_map_gs_except(&ts, LEAF_PT_GPA);
    two_stage_activate(&ts);

    EXPECT_GUEST_PAGE_FAULT(CAUSE_LOAD_GUEST_PAGE_FAULT, {
        run_in_vs_mode_load(TEST_GVA);
    });

    /* htval 是叶子 PTE 自身的 GPA，与原 GVA 翻译后的目标 GPA 无关 */
    CHECK_HTVAL("htval == leaf PT GPA >> 2", LEAF_PT_GPA >> 2);
    TEST_ASSERT("htval != 0", trap_get_htval() != 0);
    /* htval 低 2 bit 必为 0（PTE 8-byte 对齐） */
    TEST_ASSERT_EQ("htval low 2 bits = 0 for implicit",
                   (trap_get_htval() << 2) & 0x3, 0UL);

    HYP_TEST_END();
}
```

---

### Group 4：跨页 / Misaligned 访问（htval = faulting portion）

**规范依据**：
- `norm:htval_trapval`（`SPEC/hypervisor.adoc:931-936`）："for misaligned loads and stores that cause guest-page faults, a nonzero guest physical address in `htval` corresponds to the faulting portion of the access"
- `norm:H_straddle`（`SPEC/hypervisor.adoc:2053-2061`）：跨页访问 stval 写 page-boundary GVA
- `norm:mtval2_htval_virtaddr`（`SPEC/hypervisor.adoc:2063-2067`）：非 implicit 时 GPA 对应 stval 中的精确 GVA

**测试职责**：当 load/store/取指访问跨越页边界，前半部分访问成功后半部分触发 G-stage GPF 时，验证 `htval` 等于 **faulting portion** 的 GPA（即页边界后那个字节起始的 GPA），而非原始访问起始地址。

**前置条件**：
- vsatp=Bare 简化（GVA=GPA）
- hgatp Sv39x4：前半页（faulting page 之前那一页）正常映射，目标页 Invalid
- 平台不支持硬件透明非对齐访问（否则跳过部分用例）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HTVAL-STR-01 | 跨 4KB load GPF | 8 字节 load 起始于 `0x80000FFC`，跨 4KB 边界进入未映射的 `0x80001000` 页 → cause 21 | `cause == 21`；`htval == 0x80001000 >> 2 != 0`；`stval == 0x80001000` |
| HTVAL-STR-02 | 跨 2MB superpage store GPF | 8 字节 store 起始于 `0x801FFFFC`，跨 2MB 边界进入未映射的 `0x80200000` superpage → cause 23 | `cause == 23`；`htval == 0x80200000 >> 2 != 0` |
| HTVAL-STR-03 | 跨 4KB instruction GPF（变长） | 4 字节非压缩指令位于 `0x80000FFE`，跨 4KB 边界进入未映射页执行 → cause 20 | `cause == 20`；`htval == 0x80001000 >> 2 != 0` |

> [!WARNING]
> 大多数 RISC-V 实现支持硬件透明非对齐访问，HTVAL-STR-01 / STR-02 在该平台上需先探测：若不触发 misaligned 异常但触发 GPF，则继续；若硬件忽略非对齐导致单页访问，则用例 `TEST_SKIP`。

#### 关键代码示例：HTVAL-STR-01

```c
/* tests/hyp/test_shtvala_straddle.c — HTVAL-STR-01 */

#define PAGE_OK       0x80000000UL  /* 已映射 */
#define PAGE_FAULT    0x80001000UL  /* G-stage 未映射 */
#define LOAD_START    0x80000FFCUL  /* 跨页 load 起始 */

TEST_REGISTER(test_shtvala_straddle_load);
bool test_shtvala_straddle_load(void) {
    TEST_BEGIN("HTVAL-STR-01: cross-page load GPF, htval == faulting half GPA>>2");

    SHTVALA_REQUIRE();
    delegate_gpf_to_hs();

    two_stage_t ts;
    two_stage_init(&ts, BARE, SV39X4);
    two_stage_map_gs_4k(&ts, PAGE_OK, PAGE_OK,
                        PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D);
    /* PAGE_FAULT 不映射 */
    two_stage_activate(&ts);

    EXPECT_GUEST_PAGE_FAULT(CAUSE_LOAD_GUEST_PAGE_FAULT, {
        run_in_vs_mode_load_8byte(LOAD_START);
    });

    /* htval 报告 faulting portion（后半页起始） */
    CHECK_HTVAL("htval == fault page GPA >> 2", PAGE_FAULT >> 2);
    TEST_ASSERT("htval != 0", trap_get_htval() != 0);
    TEST_ASSERT_EQ("stval == fault page boundary", trap_get_stval(), PAGE_FAULT);

    HYP_TEST_END();
}
```

---

### Group 5：HLV / HLVX / HSV 指令触发的 G-stage GPF（mtval2 = GPA >> 2）

**规范依据**：
- `norm:shtvala_htval_faulting_gpa`（`SPEC/shtvala.adoc:4-6`）：htval / mtval2 必须写入 faulting GPA
- `norm:htval_trapval` / `norm:mtval2_trapval`（`SPEC/hypervisor.adoc:916-921`）：GPF 时写 GPA>>2
- `norm:hlsv_trans`（`SPEC/hypervisor.adoc:1492`）：HLV/HLVX/HSV 始终执行两阶段翻译
- `norm:hlsv_mode`（`SPEC/hypervisor.adoc:1488`）：HLV/HLVX/HSV 仅在 M-mode、HS-mode 或 U-mode（hstatus.HU=1）下有效
- `SPEC/hypervisor.adoc:1505-1507`（NOTE）：HLVX 的异常类型与 load 相同（cause=21），**不是**取指异常（cause=20）

**测试职责**：验证在 HS-mode（V=0）使用 HLV/HLVX/HSV 指令执行两阶段翻译时，若 G-stage 映射缺失导致 GPF，mtval2 正确写入 GPA >> 2。HLV/HSV 在 V=0 执行时 GPF trap 到 M-mode，因此验证目标是 `mcause` 和 `mtval2`（与 `htval` 语义对称）。

**前置条件**：
- 在 HS-mode（V=0）下执行 HLV/HLVX/HSV 指令
- hgatp=Sv39x4，目标 GPA 在 G-stage 未映射（PTE.V=0）
- HLV/HSV 使用 vsatp=Bare（地址即 GPA）
- HLVX 使用 vsatp=Sv39（identity-mapped VS-stage），使 G-stage 失败能确定性地产生 cause=21
- HLVX 的有效访问特权由 `hstatus.SPVP` 控制：SPVP=0 为 VU（PTE.U=1），SPVP=1 为 VS（PTE.U=0）
- 额外覆盖 vsatp=BARE 路径，验证 HLVX 在无 VS-stage 翻译时的异常分类

> [!NOTE]
> HLV/HSV 在 HS-mode（V=0）执行时触发的 GPF trap 到 M-mode，因此验证的是 `mcause` 和 `mtval2`。框架的 `trap_get_cause()` / `trap_get_htval()` 在 M-mode trap 时分别返回 `mcause` / `mtval2` 值。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HTVAL-HLV-01a | HLVX.WU @ VU (SPVP=0) | HS-mode 执行 HLVX.WU，SPVP=0（VU 身份），VS PTE U=1，目标 GPA 未映射 | `mcause == 21`（load GPF）；`mtval2 == GPA>>2` |
| HTVAL-HLV-01b | HLVX.WU @ VS (SPVP=1) | HS-mode 执行 HLVX.WU，SPVP=1（VS 身份），VS PTE U=0，目标 GPA 未映射 | `mcause == 21`（load GPF）；`mtval2 == GPA>>2` |
| HTVAL-HLV-02 | HLV.D G-stage load GPF | HS-mode 执行 HLV.D，目标 GPA 未映射 | `mcause == 21`（load GPF）；`mtval2 == GPA>>2` |
| HTVAL-HLV-03 | HSV.D G-stage store GPF | HS-mode 执行 HSV.D，目标 GPA 未映射 | `mcause == 23`（store GPF）；`mtval2 == GPA>>2` |
| HTVAL-HLV-04 | HLVX.WU (vsatp=BARE) | HS-mode 执行 HLVX.WU，vsatp=BARE（VA 直接作为 GPA），目标 GPA 未映射 | `mcause == 21`（load GPF）；`mtval2 == GPA>>2` |

#### 关键代码示例：HTVAL-HLV-01a

```c
/* shtvala/tests/test_htval_hlv.c — HTVAL-HLV-01a */

TEST_REGISTER(test_htval_hlv_01a_hlvx_vu);
bool test_htval_hlv_01a_hlvx_vu(void) {
    TEST_BEGIN("HTVAL-HLV-01a: HLVX.WU @ VU (SPVP=0) gpf reports mtval2 = GPA>>2");
    REQUIRE_HGATP_MODE(HGATP_MODE_SV39X4);

    uintptr_t target = (uintptr_t)test_fault_page;
    /* V=0, HLVX.WU through invalid G-stage PTE, effective priv = VU.
     * Per hypervisor.adoc:1505-1507, HLVX raises the same exceptions
     * as other load instructions -> CAUSE_LOAD_GUEST_PAGE_FAULT (21). */
    bool fired = _fire_hlvx_fault_priv(target, /*flags=*/0, PRIV_U);
    TEST_ASSERT("HLVX load-gpf fired (VU)", fired);
    if (fired) {
        TEST_ASSERT_EQ("mcause = 21 (load-gpf)",
                       trap_get_cause(), CAUSE_LOAD_GUEST_PAGE_FAULT);
        TEST_ASSERT_EQ("mtval2 = GPA>>2",
                       trap_get_htval(), target >> 2);
    }
    hyp_reset_state();
    HYP_TEST_END();
}
```

---

### Group 6：G-stage MODE 覆盖与 GPA 高位越界

**规范依据**：
- `norm:hgatp_mode_x4`（`SPEC/hypervisor.adoc:1914-1924`）：Sv*x4 入参 GPA 比 Sv* 多 2 bit
- `norm:hgatp_mode_sv39x4`（`SPEC/hypervisor.adoc:1943-1947`）：bits 63:41 必须为 0，否则 GPF
- `norm:hgatp_mode_sv48x4`（`SPEC/hypervisor.adoc:1953-1959`）：bits 63:50 必须为 0，否则 GPF
- `norm:hgatp_mode_sv57x4`（`SPEC/hypervisor.adoc:1965-1971`）：bits 63:59 必须为 0，否则 GPF
- `norm:shtvala_htval_faulting_gpa`：联动 —— 即使越界场景，htval 也必须写 GPA

**测试职责**：验证三种 G-stage MODE（Sv39x4/Sv48x4/Sv57x4）下，无论合法范围内 GPA 还是高位越界 GPA 的 GPF，htval 都必须写入完整 faulting GPA>>2。

**前置条件**：
- 用例分别配置 hgatp.MODE = Sv39x4 / Sv48x4 / Sv57x4
- 越界用例下，平台必须实现对应宽度的 GPA 总线（否则越界 bit 在硬件上被截断，无法测试）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HTVAL-MOD-01 | Sv39x4 合法范围 GPF | hgatp Sv39x4，访问 GPA = `0x100_0000_0000`（41-bit 内合法）但未映射 → cause 21 | `cause == 21`；`htval == GPA >> 2 != 0` |
| HTVAL-MOD-02 | Sv39x4 高位越界 GPF | hgatp Sv39x4，访问 GPA bit 41=1（如 `0x200_0000_0000`），越界 → cause 21 | `cause == 21`；`htval == 越界 GPA >> 2 != 0`（实现宽度内完整保留） |
| HTVAL-MOD-03 | Sv48x4 高位越界 GPF | hgatp Sv48x4，访问 GPA bit 50=1，越界 → cause 21 | `cause == 21`；`htval == 越界 GPA >> 2 != 0` |
| HTVAL-MOD-04 | Sv57x4 高位越界 GPF | hgatp Sv57x4，访问 GPA bit 59=1，越界 → cause 21 | `cause == 21`；`htval == 越界 GPA >> 2 != 0` |

#### 关键代码示例：HTVAL-MOD-02（Sv39x4 高位越界）

```c
/* tests/hyp/test_shtvala_mode.c — HTVAL-MOD-02 */

#define OVERFLOW_GPA   (1UL << 41)   /* bit 41=1, Sv39x4 越界 */

TEST_REGISTER(test_shtvala_sv39x4_overflow);
bool test_shtvala_sv39x4_overflow(void) {
    TEST_BEGIN("HTVAL-MOD-02: Sv39x4 GPA bit 41=1 → GPF, htval records full GPA");

    SHTVALA_REQUIRE();
    REQUIRE_HGATP_MODE(SV39X4);
    delegate_gpf_to_hs();

    two_stage_t ts;
    two_stage_init(&ts, BARE, SV39X4);
    two_stage_activate(&ts);

    EXPECT_GUEST_PAGE_FAULT(CAUSE_LOAD_GUEST_PAGE_FAULT, {
        run_in_vs_mode_load(OVERFLOW_GPA);
    });

    /* WARL 子集允许情况下，htval 应完整记录越界 GPA */
    CHECK_HTVAL("htval == overflow GPA >> 2", OVERFLOW_GPA >> 2);
    TEST_ASSERT("htval != 0", trap_get_htval() != 0);

    HYP_TEST_END();
}
```

---

### Group 7：非 GPF Trap 不污染 htval（必须写 0）

**规范依据**：
- `norm:htval_trapval`（`SPEC/hypervisor.adoc:919`）："For other traps, `htval` is set to zero"

**测试职责**：Shtvala 强化 GPF 时 htval 必须非零，反向也意味着非 GPF 进入 HS-mode 的 trap 必须**主动**清零 htval（防止读到上一次 GPF 的残留值）。

**前置条件**：
- 先触发一次 GPF 让 htval 非零作为"污染"准备
- 紧接着触发非 GPF trap（ecall / illegal / page-fault / virtual-instruction），且该 trap 也进 HS-mode

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HTVAL-CLR-01 | 先 GPF 后 ecall | 触发 cause 21（htval 写非零）；从 VS-mode/VU-mode ecall（cause 8/9/10）进 HS | 第二次 trap：`cause==8/9/10`；`htval == 0` |
| HTVAL-CLR-02 | 先 GPF 后 illegal-instruction | 触发 cause 21；HS-mode 执行 illegal 指令（cause 2） | 第二次 trap：`cause==2`；`htval == 0` |
| HTVAL-CLR-03 | 先 GPF 后普通 page-fault | 触发 cause 21；HS-mode 触发 cause 12/13/15（普通 page-fault，非 guest） | 第二次 trap：`cause==12/13/15`；`htval == 0` |
| HTVAL-CLR-04 | 先 GPF 后 virtual-instruction | 触发 cause 21；VS-mode 执行 HS-level CSR（cause 22） | 第二次 trap：`cause==22`；`htval == 0` |

#### 关键代码示例：HTVAL-CLR-01

```c
/* tests/hyp/test_shtvala_clear.c — HTVAL-CLR-01 */

TEST_REGISTER(test_shtvala_clear_after_ecall);
bool test_shtvala_clear_after_ecall(void) {
    TEST_BEGIN("HTVAL-CLR-01: htval cleared on ecall after GPF");

    SHTVALA_REQUIRE();
    delegate_gpf_to_hs();
    two_stage_setup_bare_vsatp_sv39x4_hgatp();

    /* Step 1: 触发 GPF 把 htval 写非零 */
    EXPECT_GUEST_PAGE_FAULT(CAUSE_LOAD_GUEST_PAGE_FAULT, {
        run_in_vs_mode_load(0x80100000UL);
    });
    TEST_ASSERT("htval != 0 after GPF", trap_get_htval() != 0);

    /* Step 2: VS-mode ecall → cause 10 进 HS-mode */
    EXPECT_TRAP_HS(CAUSE_VS_ECALL, {
        run_in_vs_mode_ecall();
    });

    /* htval 必须被清零 */
    TEST_ASSERT_EQ("htval cleared after non-GPF trap",
                   trap_get_htval(), 0UL);

    HYP_TEST_END();
}
```

---

### Group 8：htval / stval 一致性 + 低 2 bit 通过 stval 恢复

**规范依据**：
- `norm:mtval2_htval_virtaddr`（`SPEC/hypervisor.adoc:2063-2067`）：非 implicit 时 GPA 必须对应 stval
- `norm:htval_trapval` NOTE 段（`SPEC/hypervisor.adoc:948-953`）："the least-significant two bits are ordinarily the same as the least-significant two bits of the faulting virtual address in `stval`. For faults due to implicit memory accesses for VS-stage address translation, the least-significant two bits are instead zeros"

**测试职责**：因为 htval 存的是 GPA>>2，完整 GPA 重建需要：
- **显式访问**：`(htval << 2) | (stval & 0x3) == faulting GPA`
- **隐式 VS-stage 访问**：低 2 bit 强制为 0（PTE 8-byte 对齐），与 stval 低 2 bit 无关

**前置条件**：
- 显式用例：vsatp=Bare，构造 GVA=GPA 低 2 bit ≠ 00（典型如 `0x???????3`）
- 隐式用例：复用 Group 3 的设置

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HTVAL-CON-01 | 显式 GPF 低 2 bit 通过 stval 恢复 | vsatp=Bare，访问 GVA=`0x80100003`（低 2 bit=0b11）→ cause 21 | `(htval << 2) \| (stval & 0x3) == 0x80100003`；`stval & 0x3 == 0x3` |
| HTVAL-CON-02 | 显式 GPF 不同低 2 bit 组合 | 用 `0x...01`、`0x...02`、`0x...03` 三种低 2 bit 组合各跑一次 | 每次 `(htval << 2) \| (stval & 0x3) == GVA` |
| HTVAL-CON-03 | 隐式 GPF 低 2 bit 必为 0 | 复用 HTVAL-IMP-03 设置，验证 `(htval << 2) & 0x3 == 0`，与 stval 低 2 bit 无关 | `htval << 2` 低 2 bit 为 0；与 stval 低 2 bit 不一定相等 |

#### 关键代码示例：HTVAL-CON-01

```c
/* tests/hyp/test_shtvala_consistency.c — HTVAL-CON-01 */

#define TEST_GVA_GPA  0x80100003UL    /* 低 2 bit = 0b11 */

TEST_REGISTER(test_shtvala_consistency_low_bits);
bool test_shtvala_consistency_low_bits(void) {
    TEST_BEGIN("HTVAL-CON-01: (htval<<2) | (stval&3) == faulting GPA");

    SHTVALA_REQUIRE();
    delegate_gpf_to_hs();
    two_stage_setup_bare_vsatp_sv39x4_hgatp();
    /* GPA 0x80100003 所在 4KB 页未映射 */

    EXPECT_GUEST_PAGE_FAULT(CAUSE_LOAD_GUEST_PAGE_FAULT, {
        run_in_vs_mode_load(TEST_GVA_GPA);
    });

    uintptr_t reconstructed = (trap_get_htval() << 2) |
                              (trap_get_stval() & 0x3);

    TEST_ASSERT_EQ("reconstructed GPA == TEST_GVA_GPA",
                   reconstructed, TEST_GVA_GPA);
    TEST_ASSERT_EQ("stval low 2 bits preserved",
                   trap_get_stval() & 0x3, 0x3UL);

    HYP_TEST_END();
}
```

---

### Group 9：hstatus.GVA 联动

**规范依据**：
- `norm:hstatus_gva_op`（`SPEC/hypervisor.adoc:321-326`）：进 HS-mode 时若写 GVA 到 stval，则 hstatus.GVA=1；其他情况 GVA=0

**测试职责**：Shtvala 不直接管 hstatus.GVA，但 GPF 时 stval 写 GVA 与 htval 写 GPA 是一对联动信息，验证两者状态一致：GPF → GVA=1 且 htval≠0；非 GPF → GVA=0 且 htval=0。

**前置条件**：
- 复用 Group 2 / Group 7 的设置

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HTVAL-GVA-01 | GPF 时 GVA=1 && htval≠0 | 触发 cause 21 | `hstatus.GVA == 1`；`stval != 0`；`htval != 0` |
| HTVAL-GVA-02 | 非 GPF 时 GVA=0 && htval=0 | VS/VU ecall 进 HS（cause 8/9/10） | `hstatus.GVA == 0`；`htval == 0` |

#### 关键代码示例：HTVAL-GVA-01

```c
/* tests/hyp/test_shtvala_gva.c — HTVAL-GVA-01 */

TEST_REGISTER(test_shtvala_gva_set_on_gpf);
bool test_shtvala_gva_set_on_gpf(void) {
    TEST_BEGIN("HTVAL-GVA-01: hstatus.GVA == 1 && htval != 0 on GPF");

    SHTVALA_REQUIRE();
    delegate_gpf_to_hs();
    two_stage_setup_bare_vsatp_sv39x4_hgatp();

    EXPECT_GUEST_PAGE_FAULT(CAUSE_LOAD_GUEST_PAGE_FAULT, {
        run_in_vs_mode_load(0x80100000UL);
    });

    CHECK_GVA("hstatus.GVA == 1", 1);
    TEST_ASSERT("htval != 0", trap_get_htval() != 0);
    TEST_ASSERT("stval != 0", trap_get_stval() != 0);

    HYP_TEST_END();
}
```

---

## 附录：相关常量与 API 参考

### 异常 Cause 值（与 Shtvala 相关）

| 名称 | 值 | 说明 | htval 含义（Shtvala） |
|------|-----|------|------|
| `CAUSE_INST_GUEST_PAGE_FAULT` | 20 | 指令 guest-page-fault | **必须**写 faulting GPA >> 2 |
| `CAUSE_LOAD_GUEST_PAGE_FAULT` | 21 | Load guest-page-fault | **必须**写 faulting GPA >> 2 |
| `CAUSE_STORE_GUEST_PAGE_FAULT` | 23 | Store/AMO guest-page-fault | **必须**写 faulting GPA >> 2 |
| `CAUSE_VIRTUAL_INSTRUCTION` | 22 | 虚拟指令异常 | 写 0（非 GPF） |
| `CAUSE_S_ECALL` / `VS_ECALL` / `U_ECALL` | 9 / 10 / 8 | ecall | 写 0（非 GPF） |
| `CAUSE_ILLEGAL_INST` | 2 | 非法指令 | 写 0（非 GPF） |
| `CAUSE_LOAD_PAGE_FAULT` | 13 | 普通 load page-fault | 写 0（非 GPF） |
| `CAUSE_STORE_PAGE_FAULT` | 15 | 普通 store page-fault | 写 0（非 GPF） |

### 框架 API 参考

| 函数/宏 | 路径 | 说明 |
|---------|------|------|
| `trap_get_htval()` | `common/trap.c:502` | 返回最近一次 trap 的 htval/mtval2 值（GPA>>2） |
| `trap_get_stval()` | `common/trap.c` | 返回最近一次 trap 的 stval（GVA） |
| `trap_get_cause()` | `common/trap.c` | 返回最近一次 trap 的 cause |
| `EXPECT_GUEST_PAGE_FAULT(cause, stmt)` | `common/hyp/hyp_test.h:28-37` | 期望 stmt 触发指定 GPF cause |
| `CHECK_HTVAL(msg, expected)` | `common/hyp/hyp_test.h:39-42` | 断言 htval == expected（GPA>>2） |
| `CHECK_GVA(msg, expected)` | `common/hyp/hyp_test.h:48-50` | 断言 hstatus.GVA == expected |
| `HYP_TEST_END()` | `common/hyp/hyp_test.h:21-25` | 复位 hypervisor 状态并结束测试 |
| `run_in_vs_mode(fn, arg)` | `common/hyp/hyp_priv.h` | 进入 VS-mode 执行 fn，trap 后返回 |
| `run_in_vu_mode(fn, arg)` | `common/hyp/hyp_priv.h` | 进入 VU-mode 执行 fn |
| `two_stage_init(ts, vs_mode, gs_mode)` | `common/hyp/two_stage.h` | 初始化两阶段翻译上下文 |
| `two_stage_map_vs(ts, gva, gpa, flags)` | `common/hyp/two_stage.h` | 在 VS-stage 增加映射 |
| `two_stage_map_gs_4k(ts, gpa, hpa, flags)` | `common/hyp/two_stage.h` | 在 G-stage 增加 4K 映射 |
| `two_stage_activate(ts)` | `common/hyp/two_stage.h` | 写入 vsatp + hgatp 激活 |
| `delegate_gpf_to_hs()` | 测试辅助 | 设置 medeleg/hedeleg 委托 cause 20/21/23 至 HS |
| `SHTVALA_REQUIRE()` | 测试辅助 | 平台未实现 Shtvala 时 `TEST_SKIP` |
| `REQUIRE_HGATP_MODE(mode)` | 测试辅助 | hgatp 不支持指定 MODE 时 `TEST_SKIP` |

### CSR 定义

| CSR 名称 | 编号 | 说明 |
|----------|------|------|
| `htval` | `0x643` | Hypervisor trap value（GPA>>2） |
| `htinst` | `0x64A` | Hypervisor trap instruction |
| `hstatus` | `0x600` | Hypervisor status（含 GVA、SPV、SPVP） |
| `hgatp` | `0x680` | Hypervisor guest address translation and protection |
| `hedeleg` | `0x602` | Hypervisor exception delegation |
| `vsatp` | `0x280` | Virtual supervisor address translation and protection |
| `stval` | `0x143` | Supervisor trap value（HS-mode 中存 GVA） |
| `medeleg` | `0x302` | Machine exception delegation |
| `mtval2` | `0x34B` | Machine second trap value（**out-of-scope**） |

### 测试用例总览

| Group | 测试数量 | 主要异常类型 | htval 语义 | 主锚 norm |
|-------|---------|------------|-----------|-----------|
| Group 1 | 4 | 寄存器属性（无异常） | 0 / GPA>>2 (RW) | `htval_sz_acc_op` / `htval_val` |
| Group 2 | 7 | GPF cause 20/21/23 显式访问 + AMO | 显式 GPA >> 2 (≠0) | `shtvala_htval_faulting_gpa` |
| Group 3 | 7 | GPF 隐式 VS-stage + htinst 联动 | PTE GPA >> 2 (≠0) | `shtvala_htval_faulting_gpa` / `H_trap_xtinst_guestpage` |
| Group 4 | 3 | GPF 跨页 / misaligned | faulting portion GPA >> 2 (≠0) | `htval_trapval` (misaligned) / `H_straddle` |
| Group 5 | 3 | HLV/HLVX/HSV 指令 GPF (V=0→M-mode) | mtval2 == GPA >> 2 | `hlsv_trans` / `mtval2_trapval` / `hypervisor.adoc:1505-1507` |
| Group 6 | 4 | Sv*x4 模式 / 高位越界 GPF | 完整 GPA >> 2 (≠0) | `hgatp_mode_sv39x4/sv48x4/sv57x4` |
| Group 7 | 4 | 非 GPF trap | 0 | `htval_trapval`（其他 trap 段） |
| Group 8 | 3 | 显式 / 隐式 GPF 低位重建 | 重建 GPA | `mtval2_htval_virtaddr` / `htval_trapval` (NOTE) |
| Group 9 | 2 | GPF / 非 GPF GVA 联动 | ≠0 / 0 | `hstatus_gva_op` |
| **总计** | **37** | | | |

### Shtvala 与基线 H 扩展的差异

| 维度 | H 扩展基线（无 Shtvala） | 本方案（启用 Shtvala） |
|------|-------------------------|------------------------|
| GPF 时 htval | 允许实现写 0 | **必须**写 faulting GPA>>2，且 `!= 0` |
| 隐式 VS-stage 翻译 GPF | 允许实现写 0 | **必须**写 PTE 自身 GPA>>2 |
| 跨页 / misaligned GPF | 允许实现写 0 | **必须**写 faulting portion GPA>>2 |
| Sv*x4 高位越界 GPF | 允许实现写 0 | **必须**写完整越界 GPA>>2（受 WARL 子集限制） |
| WARL 子集 | 允许极小子集（甚至只 0） | 必须能装下实现宽度内的所有 faulting GPA |
| 非 GPF trap 时 htval | 写 0（基线已要求） | 同（Shtvala 不收紧） |
| `mtval2` 行为 | 由 `mtval2_trapval` 系列约束 | **不受 Shtvala 约束**（out-of-scope） |
