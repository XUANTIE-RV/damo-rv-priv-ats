**中文 | [English](../testplan_en/Sstvecd_test_plan_en.md)**

# Sstvecd 扩展测试计划

本文档描述 Sstvecd（Direct Trap Vectoring, Version 1.0）扩展的测试计划。Sstvecd 是一个对 `stvec` 寄存器行为的窄约束扩展，规范要求实现必须保证：(1) `stvec.MODE` 能写入并保持 0（Direct）；(2) 在 `stvec.MODE=Direct` 时，`stvec.BASE` 必须能保持任意 4 字节对齐的地址。

---

## 测试范围

### 规范来源

- `SPEC/sstvecd.adoc` — Sstvecd Extension for Direct Trap Vectoring, Version 1.0
- `SPEC/supervisor.adoc` 第 318–365 行（`stvec` 寄存器与 MODE 字段编码） — 提供 `stvec` 字段布局与 Direct/Vectored 行为定义

### 关键参考文件

| 路径 | 说明 |
|------|------|
| `SPEC/sstvecd.adoc` | Sstvecd 规范全文（共 6 行） |
| `SPEC/supervisor.adoc:318-365` | `stvec` 寄存器规范，定义 BASE/MODE 字段 + Direct/Vectored 行为 |
| `common/test_framework.c:36-38` | 默认 `stvec` 设置为 `s_trap_entry`，每个用例进入/退出时需保存还原 |
| `common/encoding.h:129` | `CSR_STVEC = 0x105` |
| `common/csr_accessors.c:281` | 已存在 `_CSR_WRITE_CASE(0x105)` 通用 CSR 写入入口 |
| `common/vm/satp.c::vm_run_in_smode` | 进入 S-mode 执行测试函数，捕获异常并返回 scause |
| `svadu/Makefile` | 子目录 Makefile 模板（`SPIKE_ISA_EXT = _svadu` 启用扩展），后续实施阶段参考 |
| `svvptc/tests/...` | 子目录本地 trap-handler 实现范例（参考"设计要点 2 trap entry 安置"） |

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sstvecd_stvec_mode_direct` | If the Sstvecd extension is implemented, then `stvec.MODE` must be capable of holding the value 0 (Direct). | 如果实现了 Sstvecd 扩展，则 `stvec.MODE` 必须能够保存值 0（直接模式）。 |
| `norm:sstvecd_stvec_base_aligned_address` | Furthermore, when `stvec.MODE=Direct`, `stvec.BASE` must be capable of holding any valid four-byte-aligned address. | 此外，当 `stvec.MODE=Direct` 时，`stvec.BASE` 必须能够保存任何有效的四字节对齐地址。 |
| `norm:stvec_op` | The BASE field in `stvec` is a field that can hold any valid virtual or physical address, subject to the following alignment constraints: the address must be 4-byte aligned, and MODE settings other than Direct might impose additional alignment constraints on the value in the BASE field. | BASE 字段可保存任何有效虚拟或物理地址，但必须 4 字节对齐，非 Direct 模式可能施加更严格的对齐约束。 |
| `norm:stvec_sz_base` | The CSR contains only bits XLEN-1 through 2 of the address BASE. When used as an address, the lower two bits are filled with zeroes to obtain an XLEN-bit address that is always aligned on a 4-byte boundary. | CSR 仅保存 BASE 地址的 [XLEN-1:2] 位。用作地址时低两位填零，获得始终 4 字节对齐的 XLEN 位地址。 |

> [!IMPORTANT]
> Sstvecd 规范本身只有两条强约束（Direct 可保持、BASE 4 字节对齐持有能力）。Group 3 的 trap 跳转测试是对"BASE 设置确实生效、Direct 模式行为正确"的端到端验证，规范依据来自 `SPEC/supervisor.adoc:341-365`（stvec MODE 编码定义）—— Sstvecd 强约束 Direct 模式可用，自然要求该模式行为符合 supervisor 规范。

### 不在测试范围内

- **Vectored 模式的功能性行为**：Sstvecd 不要求实现 Vectored 模式；本计划只在 Group 1 中"探测"Vectored 是否被实现，不验证其正确性
- **VS-mode 的 `vstvec`**：Sstvecd 仅约束 `stvec`，不涉及 `vstvec`
- **M-mode 的 `mtvec`**：不在 Sstvecd 规范范畴
- **多 hart 场景**：项目为单核测试环境
- **Sv32 / Sv48 / Sv57 模式**：仅覆盖 RV64 + Sv39，与项目其它扩展计划保持一致
- **不同 SXLEN 下的 BASE 范围**：仅覆盖 SXLEN=64

---

## 设计要点

### 1. 实现存在性的处理

平台必须通过 `SPIKE_ISA_EXT = _sstvecd` 启用 Sstvecd（参考 `svadu/Makefile` 的 `SPIKE_ISA_EXT = _svadu`）。如果未启用，Group 1 的"MODE=0 写后回读=0"在大多数实现下仍然能通过（任何合规 RISC-V 实现都至少支持 Direct=0），因此该用例实际上无法独立证明 Sstvecd 已启用。

为强化"Sstvecd 必须启用"的前提，计划在 Group 2 BASE-04 用例中**故意写入一个高位（如 bit 38 / bit 47 / 接近 SXLEN 上界）的 4 字节对齐地址**——若实现未声明 Sstvecd，可能将 BASE 的高位视为 WARL 截断（read-only 0），从而暴露实现差异。该用例使用 hard-assert，失败即说明平台未真正实现 Sstvecd。

### 2. trap entry 的安置（Group 3 专用）

Group 3 需要一个用户可控的 S-mode trap entry，要求：

- **4 字节对齐**：满足 `stvec.BASE` 对齐约束（用 `__attribute__((aligned(4)))` 或汇编 `.align 2` 保证）
- **位于 `.text` 段**：链接脚本无需特殊处理
- **最小化处理路径**：保存 `sscratch`，记录命中地址到全局变量 `g_sstvecd_trap_pc`、记录 `scause` 到 `g_sstvecd_trap_cause`，然后 `sepc += 4` 后 `sret` 返回

参考实现思路（伪代码）：

```asm
.section .text
.globl  sstvecd_trap_entry
.align  2                          /* 4 byte aligned, 满足 BASE 对齐 */
sstvecd_trap_entry:
    csrrw   t0, sscratch, t0       /* swap t0 <-> sscratch（取暂存槽） */
    /* 记录命中 PC：把当前 PC（= stvec.BASE）写入全局 */
    auipc   t1, 0
    addi    t1, t1, -4             /* 当前指令的 PC ≈ entry 起点 */
    la      t0, g_sstvecd_trap_pc
    sd      t1, 0(t0)
    /* 记录 scause */
    csrr    t1, scause
    la      t0, g_sstvecd_trap_cause
    sd      t1, 0(t0)
    /* sepc += 4，跳过触发 trap 的指令 */
    csrr    t1, sepc
    addi    t1, t1, 4
    csrw    sepc, t1
    csrrw   t0, sscratch, t0       /* 还原 t0 */
    sret
```

> [!NOTE]
> 实际实施阶段，可参考 `svvptc/tests/svvptc_strap.S`（svvptc 计划中提到的本地 S-mode trap entry 范例）的写法。`g_sstvecd_trap_pc` 用于断言"trap 命中地址 = stvec.BASE"，是 Direct 行为验证的核心证据。

### 3. 与默认框架的隔离

`common/test_framework.c::reset_state()` 默认把 `stvec` 设为 `s_trap_entry`（第 37 行）。本测试每个用例的标准模板：

```c
TEST_REGISTER(test_sstvecd_xxx);
bool test_sstvecd_xxx(void) {
    TEST_BEGIN("...");
    uintptr_t saved_stvec;
    asm volatile ("csrr %0, stvec" : "=r"(saved_stvec));

    /* ... 测试体 ... */

    /* 还原 stvec，避免污染后续测试 */
    asm volatile ("csrw stvec, %0" :: "r"(saved_stvec));
    TEST_END();
}
```

不依赖 `medeleg`：sstvecd 不涉及异常委托语义。Group 3 的同步异常通过 `vm_run_in_smode` 在 S-mode 内主动触发；触发时 trap 直接进入 S-mode（而非被 M-mode 委托），命中我们设置的 `stvec`。

### 4. 异步中断的触发（Group 3 STVEC-INT-01）

为验证"Direct 模式下中断也跳到 BASE 而非 BASE+4×cause"，使用 SSIP（supervisor software interrupt）：

1. M-mode 设置 `mideleg.SSIE`（bit 1）= 1，把 supervisor software interrupt 委托到 S-mode
2. M-mode 准备好 `g_sstvecd_trap_pc = 0`、`g_sstvecd_trap_cause = 0`
3. 进入 S-mode 后：先 `csrw stvec, BASE`，然后 `csrs sie, 1<<1`（使能 SSIE）、`csrs sstatus, 1<<1`（SIE=1）、最后 `csrs sip, 1<<1` 自触发 SSIP
4. trap 命中后，断言：`g_sstvecd_trap_pc == BASE`（Direct）；若实现是 Vectored，命中地址应为 `BASE + 4*1 = BASE+0x4`，断言失败即可定位到 Direct 行为不正确

> [!NOTE]
> SSIP 是 supervisor 内部可写的中断标志（`SPEC/supervisor.adoc:norm:sip_ssip_sie_ssie`），最适合在单核环境下自激发。STIP（timer）需要 Sstc 或 SBI，SEIP（external）需要 PLIC/AIA，相对复杂。

### 5. WARL 写入回读约定

`stvec.MODE` 字段是 WARL：

- 写 0（Direct）必然成功（Sstvecd 强约束）
- 写 1（Vectored）：若实现支持，回读为 1；若不支持，回读为某个合法值（最自然的选择是 0，因为 Direct 必然支持）。两种行为都不算违反 Sstvecd
- 写 ≥2（Reserved）：必须回读为合法值（0 或 1），不应被锁死

`stvec.BASE` 字段（[XLEN-1:2]）是 WARL：

- 写入低 2 bit 非 0 的值，硬件应把低 2 bit 强制为 0（`SPEC/supervisor.adoc:norm:stvec_sz_base`）
- 写入的高位 BASE 应原样回读（Sstvecd 要求"any valid 4-byte aligned address"）

---

## 测试分组

> [!IMPORTANT]
> 共 3 个测试组、12 个测试用例。所有测试运行于 RV64 + M-mode 设置 + 部分用例进入 S-mode（Group 3）。每组提供：规范依据、测试职责、测试用例表（ID/名称/描述/预期结果）；每组提供 1 个关键 C 代码示例。

---

### Group 1：`stvec.MODE` 可写性

**规范依据**：
- `norm:Sstvecd_mode_direct_writable`（`SPEC/sstvecd.adoc:4-5`）：`stvec.MODE` 必须能保持值 0（Direct）

**测试职责**：验证 `stvec.MODE` 能稳定写入并保持 Direct 值（0）；探测 Vectored（1）是否被实现；验证写入保留值（≥2）后 WARL 行为合理。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| STVEC-MODE-01 | MODE 写 0（Direct）回读 | `csrw stvec, 0`（BASE=0, MODE=0），回读 `stvec[1:0]` | `stvec[1:0] == 0`（Direct 必然可保持） |
| STVEC-MODE-02 | MODE 0→1→0 切换 | 先写 MODE=0、回读确认；再写 MODE=1、回读记录；最后写 MODE=0、回读确认 | 第 1、3 次回读 `MODE==0`；第 2 次回读 ∈ {0, 1}（实现自由） |
| STVEC-MODE-03 | MODE 写保留值（=2） | `csrw stvec, 0x2`（BASE=0, MODE=2），回读 `stvec[1:0]` | 回读 ∈ {0, 1}，不应为 2（WARL，保留值不应被锁死） |
| STVEC-MODE-04 | MODE 写保留值（=3） | `csrw stvec, 0x3`（BASE=0, MODE=3），回读 `stvec[1:0]` | 回读 ∈ {0, 1}，不应为 3 |

#### 关键代码示例：STVEC-MODE-01

```c
/* tests/test_mode.c — STVEC-MODE-01 */

#include "test_framework.h"

#define STVEC_MODE_MASK   0x3UL
#define STVEC_MODE_DIRECT 0x0UL

TEST_REGISTER(test_sstvecd_mode_direct_writable);
bool test_sstvecd_mode_direct_writable(void) {
    TEST_BEGIN("STVEC-MODE-01: stvec.MODE writable to 0 (Direct)");

    uintptr_t saved_stvec;
    asm volatile ("csrr %0, stvec" : "=r"(saved_stvec));

    /* 写入 BASE=0, MODE=0 */
    asm volatile ("csrw stvec, %0" :: "r"((uintptr_t)0));

    uintptr_t readback;
    asm volatile ("csrr %0, stvec" : "=r"(readback));
    TEST_ASSERT("stvec.MODE reads back as 0 (Direct)",
                (readback & STVEC_MODE_MASK) == STVEC_MODE_DIRECT);

    /* 还原 stvec */
    asm volatile ("csrw stvec, %0" :: "r"(saved_stvec));
    TEST_END();
}
```

---

### Group 2：`stvec.BASE` 在 Direct 模式下的持有能力

**规范依据**：
- `norm:Sstvecd_base_holds_any_4byte_aligned`（`SPEC/sstvecd.adoc:5-6`）：当 `stvec.MODE=Direct` 时，`stvec.BASE` 必须能保持任意有效的 4 字节对齐地址
- `norm:stvec_sz_base`（`SPEC/supervisor.adoc:335-338`）：CSR 仅存 BASE 的 [XLEN-1:2]，低 2 bit 写入时被强制为 0

**测试职责**：验证在 MODE=Direct 下，BASE 字段能保持各类 4 字节对齐地址（含跨 1 GiB / 512 GiB 边界、近 SXLEN 上界）；验证非 4 字节对齐地址写入时低 2 bit 被强制清零；验证 BASE 与 MODE 写入相互独立。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| STVEC-BASE-01 | BASE 写 0 回读 | `csrw stvec, 0x0`（BASE=0, MODE=0），回读 | `stvec == 0x0` |
| STVEC-BASE-02 | BASE 写 PLATFORM_MEM_BASE 回读 | `csrw stvec, PLATFORM_MEM_BASE | 0`，回读 | 回读高位 == `PLATFORM_MEM_BASE`、低 2 bit == 0 |
| STVEC-BASE-03 | BASE 写跨 1 GiB 边界 | `csrw stvec, 0x40000004`（跨 1 GiB），回读 | 回读 == `0x40000004` |
| STVEC-BASE-04 | BASE 写跨 512 GiB 边界（高位探测） | `csrw stvec, 0x8000000000`（bit 39），回读 | 回读 == `0x8000000000`（若高位被 WARL 截断，证明 Sstvecd 未真正启用，失败） |
| STVEC-BASE-05 | BASE 写非 4 字节对齐地址 | `csrw stvec, 0x40000003`（低 2 bit = 0b11, MODE 字段），回读 | 回读 BASE 部分 == `0x40000000`、低 2 bit 表现见下方说明 |
| STVEC-BASE-06 | BASE 多次改写不影响 MODE | 固定 MODE=0，依次写 BASE=0x1000、0x2000、0x4000、0x8000，每次回读 MODE | 每次 `MODE == 0` 且 BASE 与写入值一致 |
| STVEC-BASE-07 | BASE 高位扫描 | 依次写入 `1ULL << k | 0x4`（k 从 12 到 SXLEN-1，只取 4 字节对齐位），每次回读 | 每次回读 BASE == 写入值（Sstvecd 要求"any valid"） |

> [!IMPORTANT]
> **STVEC-BASE-05 的低 2 bit 语义说明**：写入 `0x40000003` 时，[1:0]=0b11 实际落在 MODE 字段（不是 BASE 的 bit[1:0]）。规范要求 BASE 是 [XLEN-1:2]，所以这条用例**实际验证的是**：BASE 字段保留 = `0x40000000 >> 2 << 2 = 0x40000000`，MODE 字段 = 0b11。MODE=3 是保留值，按 STVEC-MODE-04 的规则回读应为合法值（0 或 1）。因此 BASE-05 的精确断言为：`(readback & ~0x3) == 0x40000000` 且 `(readback & 0x3) ∈ {0, 1}`。

> [!NOTE]
> **STVEC-BASE-04 的高位选择**：bit 39 对应 Sv39 的虚地址上界附近，是探测高位 WARL 截断的典型选择。若实现把 BASE 限制在 [38:2]（如某些 RV64 实现可能默认按 Sv39 上界截断），该用例会失败。Sstvecd 规范要求 BASE 能保持"any valid"4 字节对齐地址，因此该限制应被 Sstvecd 实现解除。

> [!NOTE]
> **STVEC-BASE-07 的扫描范围**：按"`PLATFORM_MEM_BASE` 之上、不与 trap entry 冲突"的原则选取若干 k 值（如 k=12, 16, 20, 24, 28, 32, 36, 38），不必穷举到 63 位。若 SXLEN=64 但虚地址有效位仅 39/48/57，写入超过有效位的高位地址行为由实现定义，本用例只覆盖到 bit 38（Sv39 范围内）。

#### 关键代码示例：STVEC-BASE-05（非对齐写入）

```c
/* tests/test_base.c — STVEC-BASE-05 */

TEST_REGISTER(test_sstvecd_base_unaligned_write);
bool test_sstvecd_base_unaligned_write(void) {
    TEST_BEGIN("STVEC-BASE-05: unaligned BASE write masks low 2 bits");

    uintptr_t saved_stvec;
    asm volatile ("csrr %0, stvec" : "=r"(saved_stvec));

    /* 写入 0x40000003：BASE 字段实际是 0x40000000，[1:0]=0b11 落在 MODE */
    uintptr_t write_val = 0x40000003UL;
    asm volatile ("csrw stvec, %0" :: "r"(write_val));

    uintptr_t readback;
    asm volatile ("csrr %0, stvec" : "=r"(readback));

    /* BASE 部分（高位）保留 */
    TEST_ASSERT("BASE field preserved (high bits == 0x40000000)",
                (readback & ~0x3UL) == 0x40000000UL);

    /* MODE 字段为保留值 3，按 WARL 应回读为合法值（0 或 1） */
    uintptr_t mode = readback & 0x3UL;
    TEST_ASSERT("MODE field maps reserved value to {0,1}",
                mode == 0 || mode == 1);

    asm volatile ("csrw stvec, %0" :: "r"(saved_stvec));
    TEST_END();
}
```

---

### Group 3：`MODE=Direct` 下 trap 跳转到 BASE

**规范依据**：
- `SPEC/supervisor.adoc:355-365`：MODE=Direct 时，所有 trap（同步异常 + 异步中断）都把 `pc` 设为 BASE
- Sstvecd 强约束 Direct 必然可用（`norm:Sstvecd_mode_direct_writable`），因此 Direct 行为正确性是 Sstvecd 的隐含承诺

**测试职责**：验证当 `stvec.MODE=Direct` 时，同步异常与异步中断都跳转到 BASE（而不是 BASE+4×cause）；验证多次 trap 后 BASE 仍保持原值。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| STVEC-DIR-01 | 同步异常：S-mode ecall | 在 S-mode 执行 `ecall`（cause=9），`stvec` 设为自定义 4 字节对齐 entry，BASE=entry 地址 | `g_sstvecd_trap_pc == BASE`（命中 entry 起点）；`g_sstvecd_trap_cause == 9` |
| STVEC-DIR-02 | 同步异常：illegal instruction | S-mode 执行未知指令（cause=2），同上 entry | `g_sstvecd_trap_pc == BASE`；`g_sstvecd_trap_cause == 2` |
| STVEC-DIR-03 | 同步异常：load page-fault | 启用 Sv39 + 不映射的 va，S-mode `lw` 触发 cause=13 | `g_sstvecd_trap_pc == BASE`；`g_sstvecd_trap_cause == 13` |
| STVEC-INT-01 | 异步中断：SSIP | M-mode 设置 `mideleg.SSIE=1`，S-mode 自激发 SSIP（cause=1，中断位置位） | `g_sstvecd_trap_pc == BASE`（Direct 不加 4×1=4） |
| STVEC-MULTI-01 | 多次 trap BASE 不变 | 连续触发 5 次 ecall，每次 trap 后 M-mode 回读 `stvec` 检查 BASE 与 MODE | 5 次后 `BASE == 原值` 且 `MODE == 0` |

#### 关键代码示例：STVEC-DIR-01（同步异常 ecall 命中 BASE）

```c
/* tests/test_direct.c — STVEC-DIR-01 */

#include "test_framework.h"
#include "vm/vm.h"

extern void sstvecd_trap_entry(void);          /* 见"设计要点 2"：本地 4 byte 对齐 entry */
extern volatile uintptr_t g_sstvecd_trap_pc;
extern volatile uintptr_t g_sstvecd_trap_cause;

static uintptr_t smode_do_ecall(uintptr_t arg) {
    (void)arg;
    asm volatile ("ecall");                     /* cause = 9 (ecall from S-mode) */
    return 0;
}

TEST_REGISTER(test_sstvecd_direct_ecall);
bool test_sstvecd_direct_ecall(void) {
    TEST_BEGIN("STVEC-DIR-01: Direct mode, S-mode ecall lands at BASE");

    uintptr_t saved_stvec;
    asm volatile ("csrr %0, stvec" : "=r"(saved_stvec));

    /* 4 byte 对齐由 sstvecd_trap_entry 的 .align 2 保证 */
    uintptr_t entry = (uintptr_t)&sstvecd_trap_entry;
    TEST_ASSERT("entry is 4-byte aligned", (entry & 0x3UL) == 0);

    /* 委托 ecall(9) 到 S-mode（用 vm_run_in_smode 进 S-mode 时已委托一部分；
     * 此处显式委托 ecall-from-S-mode 由 vm_run_in_smode 内部处理） */
    asm volatile ("csrw stvec, %0" :: "r"(entry));   /* MODE=0 (Direct) */
    g_sstvecd_trap_pc    = 0;
    g_sstvecd_trap_cause = 0;

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D,
                              PT_LEVEL_1G);

    (void)vm_run_in_smode(&ctx, smode_do_ecall, 0);

    /* Direct 模式：trap 命中地址必须 == BASE */
    TEST_ASSERT("trap PC equals stvec.BASE", g_sstvecd_trap_pc == entry);
    TEST_ASSERT("scause == 9 (S-mode ecall)", g_sstvecd_trap_cause == 9);

    asm volatile ("csrw stvec, %0" :: "r"(saved_stvec));
    pt_pool_reset();
    TEST_END();
}
```

#### 关键代码示例：STVEC-INT-01（异步中断 SSIP 命中 BASE 而非 BASE+4）

```c
/* tests/test_direct.c — STVEC-INT-01 */

static uintptr_t smode_self_trigger_ssip(uintptr_t arg) {
    (void)arg;
    /* 使能 supervisor software interrupt */
    asm volatile ("csrs sie, %0"     :: "r"(1UL << 1));   /* SSIE */
    asm volatile ("csrs sstatus, %0" :: "r"(1UL << 1));   /* SIE */
    /* 自激发 SSIP；下一指令窗口内中断应被处理 */
    asm volatile ("csrs sip, %0"     :: "r"(1UL << 1));   /* SSIP */
    /* 等待中断命中（实际上立即触发，handler 会把 sepc 推进过 nop） */
    asm volatile ("nop");
    return 0;
}

TEST_REGISTER(test_sstvecd_direct_ssip);
bool test_sstvecd_direct_ssip(void) {
    TEST_BEGIN("STVEC-INT-01: Direct mode, SSIP lands at BASE (not BASE+4)");

    uintptr_t saved_stvec, saved_mideleg;
    asm volatile ("csrr %0, stvec"   : "=r"(saved_stvec));
    asm volatile ("csrr %0, mideleg" : "=r"(saved_mideleg));

    /* 委托 SSIP 到 S-mode */
    asm volatile ("csrs mideleg, %0" :: "r"(1UL << 1));

    uintptr_t entry = (uintptr_t)&sstvecd_trap_entry;
    asm volatile ("csrw stvec, %0" :: "r"(entry));   /* MODE=0 */
    g_sstvecd_trap_pc    = 0;
    g_sstvecd_trap_cause = 0;

    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                              PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D,
                              PT_LEVEL_1G);

    (void)vm_run_in_smode(&ctx, smode_self_trigger_ssip, 0);

    /* Direct 模式：中断 cause=1 也应跳到 BASE，而非 BASE + 4*1 */
    TEST_ASSERT("trap PC equals BASE (Direct, not Vectored)",
                g_sstvecd_trap_pc == entry);
    /* scause 高位（interrupt bit）= 1，低位 = 1 (SSI) */
    TEST_ASSERT("scause is interrupt-1 (SSI)",
                (g_sstvecd_trap_cause >> (sizeof(uintptr_t)*8 - 1)) == 1 &&
                (g_sstvecd_trap_cause & ~(1UL << (sizeof(uintptr_t)*8 - 1))) == 1);

    /* 清理：清 SSIP / 关 SIE / 还原 mideleg、stvec */
    asm volatile ("csrc sip, %0"     :: "r"(1UL << 1));
    asm volatile ("csrw mideleg, %0" :: "r"(saved_mideleg));
    asm volatile ("csrw stvec, %0"   :: "r"(saved_stvec));
    pt_pool_reset();
    TEST_END();
}
```

---

## 附录：相关常量与 API 参考

### CSR 与位定义

| 名称 | 值 | 说明 |
|------|-----|------|
| `CSR_STVEC` | `0x105` | Supervisor trap vector base address，见 `common/encoding.h:129` |
| `STVEC_MODE_MASK` | `0x3` | `stvec[1:0]` 即 MODE 字段 |
| `STVEC_MODE_DIRECT` | `0x0` | Direct 模式 |
| `STVEC_MODE_VECTORED` | `0x1` | Vectored 模式（Sstvecd 不要求实现） |
| `MIDELEG_SSIE_BIT` | `1UL << 1` | 把 SSIP 委托到 S-mode |
| `SIP_SSIP_BIT` / `SIE_SSIE_BIT` / `SSTATUS_SIE_BIT` | `1UL << 1` | SSIP 自激发与 S-mode 中断使能 |

### scause 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| `CAUSE_S_ECALL` | 9 | Environment call from S-mode |
| `CAUSE_ILLEGAL_INSTRUCTION` | 2 | Illegal instruction |
| `CAUSE_LOAD_PAGE_FAULT` | 13 | Load page fault |
| `CAUSE_S_SOFTWARE_INTERRUPT`（中断号） | 1 | Supervisor software interrupt（scause 高位置 1） |

### 测试框架 API

- `pt_context_t` / `pt_init` / `pt_pool_reset`：页表上下文管理
- `pt_setup_identity_mapping(ctx, base, size, flags, level)`：批量恒等映射
- `vm_run_in_smode(ctx, fn, arg)`：以指定页表上下文进入 S-mode 执行 fn(arg)，发生异常时返回 scause（trap entry 由本地 `stvec` 设置接管）
- `TEST_REGISTER` / `TEST_BEGIN` / `TEST_ASSERT` / `TEST_END`：测试用例注册与断言宏
- `CSRR(csr)` / `CSRW(csr, val)` / `CSRS(csr, mask)` / `CSRC(csr, mask)`：CSR 读 / 写 / 置位 / 清位

### 全局变量（`sstvecd/tests/sstvecd_strap.S` 提供）

| 变量 | 类型 | 说明 |
|------|------|------|
| `g_sstvecd_trap_pc` | `volatile uintptr_t` | trap entry 命中时的 PC（应等于 `stvec.BASE`） |
| `g_sstvecd_trap_cause` | `volatile uintptr_t` | trap 命中时的 `scause` |
| `sstvecd_trap_entry` | `void(void)` | 4 字节对齐的 S-mode trap entry 符号 |

---

## 测试执行说明

### 子目录与构建集成（实施阶段参考，本计划不产出）

> [!IMPORTANT]
> 本计划阶段**仅产出 `docs/sstvecd_test_plan.md`**，不创建 `sstvecd/` 子目录、不修改顶层 `Makefile`、不写 C 测试代码。后续实施阶段按本文档创建：
> - `sstvecd/main.c`（参考 `svadu/main.c`）
> - `sstvecd/Makefile`（参考 `svadu/Makefile`，`SPIKE_ISA_EXT = _sstvecd`）
> - `sstvecd/kernel.ld`（参考 `svadu/kernel.ld`）
> - `sstvecd/tests/test_mode.c`、`test_base.c`、`test_direct.c`、`sstvecd_strap.S`、`test_helpers.h`
> 并把 `sstvecd` 加入顶层 `Makefile` 的 `EXTENSIONS` 列表。

### 运行环境

- 所有测试运行于 RV64 + Sv39（Group 3）/ M-mode 直接 CSR 操作（Group 1, 2）
- M-mode 通过 `SPIKE_ISA_EXT = _sstvecd` 启用扩展；不启用时 STVEC-BASE-04 / 07 用例预期失败
- 单核环境，无需 IPI

### 失败定位指引

| 现象 | 可能原因 |
|------|----------|
| STVEC-MODE-01 失败 | 平台连基本的 stvec 都未实现，或 `csrw stvec` 路径异常 |
| STVEC-BASE-04 / 07 失败 | Sstvecd 未真正启用，BASE 高位被 WARL 截断 |
| STVEC-DIR-01/02/03 失败（命中地址 ≠ BASE） | trap 路径异常，或 `vm_run_in_smode` 内部覆盖了 `stvec` |
| STVEC-INT-01 失败（命中地址 = BASE+4） | 实现错误地按 Vectored 处理中断；说明 MODE=0 写入未生效 |
| STVEC-INT-01 失败（无命中） | `mideleg` 未委托 SSIP，或 `sstatus.SIE` 未使能 |
