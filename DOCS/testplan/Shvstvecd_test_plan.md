**中文 | [English](../testplan_en/Shvstvecd_test_plan_en.md)**

# Shvstvecd 扩展测试计划

本文档描述 Shvstvecd（Direct Trap Vectoring for VS-mode, Version 1.0）扩展的测试计划。Shvstvecd 是一个对 `vstvec` 寄存器行为的窄约束扩展，规范要求实现必须保证：(1) `vstvec.MODE` 能写入并保持 0（Direct）；(2) 在 `vstvec.MODE=Direct` 时，`vstvec.BASE` 必须能保持任意 4 字节对齐的地址。

---

## 测试范围

### 规范来源

- `SPEC/shvstvecd.adoc` — Shvstvecd Extension for Direct Trap Vectoring, Version 1.0
- `SPEC/hypervisor.adoc` 第 1300–1312 行（`vstvec` 寄存器定义） — 提供 `vstvec` 字段布局与 V=1 时替代 `stvec` 的行为
- `SPEC/supervisor.adoc` 第 318–365 行（`stvec` 寄存器与 MODE 字段编码） — 提供 BASE/MODE 字段布局与 Direct/Vectored 行为定义（vstvec 格式与 stvec 相同）

### 关键参考文件

| 路径 | 说明 |
|------|------|
| `SPEC/shvstvecd.adoc` | Shvstvecd 规范全文（共 10 行） |
| `SPEC/hypervisor.adoc:1300-1312` | `vstvec` 寄存器规范，定义为 VSXLEN-bit RW、V=1 时替代 `stvec` |
| `SPEC/supervisor.adoc:318-365` | `stvec` 寄存器规范，定义 BASE/MODE 字段 + Direct/Vectored 行为（vstvec 格式相同） |
| `common/encoding.h:289` | `CSR_VSTVEC = 0x205` |
| `common/csr_accessors.c:216,414` | 已存在 `_CSR_READ_CASE(0x205)` / `_CSR_WRITE_CASE(0x205)` 通用 CSR 读写入口 |
| `common/hyp/hyp_priv.h:21` | `run_in_vs_mode(fn, arg)` — 在 VS-mode (V=1) 执行测试函数 |
| `common/hyp/hyp_csr.h:124` | `hyp_delegate_to_vs(hedeleg_mask, hideleg_mask)` — 异常/中断委托到 VS-mode |
| `common/hyp/hyp_csr.h:135` | `hvip_set_vssi(pending)` — 注入 VS software interrupt |
| `common/hyp/hyp_test.h` | Hypervisor 测试宏（`HYP_TEST_END`、`EXPECT_GUEST_PAGE_FAULT` 等） |
| `common/hyp/hyp_reset.c:98` | `hyp_reset_state()` 中 `csrw 0x205, zero` 重置 vstvec |
| `DOCS/framework/hypervisor_framework.md` | Hypervisor 测试框架总览 |
| `DOCS/testplan/sstvecd_test_plan.md` | 同族 S-mode stvec 测试方案，结构参照基准 |

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:shvstvecd_vstvec_mode_direct` | If the Shvstvecd extension is implemented, then `vstvec.MODE` must be capable of holding the value 0 (Direct). | 若实现了 Shvstvecd 扩展，`vstvec.MODE` 必须能够保持值 0（Direct）。 |
| `norm:shvstvecd_vstvec_base_aligned_address` | Furthermore, when `vstvec.MODE`=Direct, `vstvec.BASE` must be capable of holding any valid four-byte-aligned address. | 此外，当 `vstvec.MODE`=Direct 时，`vstvec.BASE` 必须能够保持任何有效的四字节对齐地址。 |
| `norm:vstvec_sz_acc_op` | The `vstvec` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `stvec`. When V=1, `vstvec` substitutes for the usual `stvec`. When V=0, `vstvec` does not directly affect the behavior of the machine. | `vstvec` 是 VSXLEN 位读写寄存器，VS 模式版本的 `stvec`。V=1 时替代 `stvec`；V=0 时不直接影响机器行为。 |
| `norm:stvec_op` | The BASE field in `stvec` is a field that can hold any valid virtual or physical address, subject to the following alignment constraints: the address must be 4-byte aligned, and MODE settings other than Direct might impose additional alignment constraints on the value in the BASE field. | BASE 字段可保存任何有效虚拟或物理地址，但必须 4 字节对齐，非 Direct 模式可能施加更严格的对齐约束。 |
| `norm:stvec_sz_base` | The CSR contains only bits XLEN-1 through 2 of the address BASE. When used as an address, the lower two bits are filled with zeroes to obtain an XLEN-bit address that is always aligned on a 4-byte boundary. | CSR 仅保存 BASE 地址的 [XLEN-1:2] 位。用作地址时低两位填零，获得始终 4 字节对齐的 XLEN 位地址。 |

> [!IMPORTANT]
> Shvstvecd 规范本身只有两条强约束（Direct 可保持、BASE 4 字节对齐持有能力）。Group 3 的 trap 跳转测试是对"BASE 设置确实生效、Direct 模式行为正确"的端到端验证，规范依据来自 `SPEC/supervisor.adoc:355-364`（stvec MODE 编码定义，vstvec 格式相同）—— Shvstvecd 强约束 Direct 模式可用，自然要求该模式行为符合 supervisor 规范。Group 4 验证 V=1 时通过 `stvec` 指令名实际访问的是 `vstvec`，规范依据来自 `SPEC/hypervisor.adoc:1305-1307`。

### 不在测试范围内

- **Vectored 模式的功能性行为**：Shvstvecd 不要求实现 Vectored 模式；本计划只在 Group 1 中"探测"Vectored 是否被实现，不验证其正确性
- **S-mode 的 `stvec`**：Shvstvecd 仅约束 `vstvec`，不涉及 `stvec`；`stvec` 由 Sstvecd 覆盖
- **M-mode 的 `mtvec`**：不在 Shvstvecd 规范范畴
- **多 hart 场景**：项目为单核测试环境
- **Sv32 / Sv48 / Sv57 模式**：仅覆盖 RV64 + Sv39x4，与项目其它扩展计划保持一致
- **不同 VSXLEN 下的 BASE 范围**：仅覆盖 VSXLEN=64
- **G-stage 地址翻译验证**：本测试仅使用恒等映射建立基本执行环境，不验证翻译正确性

---

## 设计要点

### 1. vstvec 的访问路径

`vstvec`（CSR 0x205）可通过两种路径访问：

- **HS/M-mode 直接访问**：`csrr/csrw 0x205`（即使用 `vstvec` CSR 编号）—— Group 1 & 2 采用此路径，在 M-mode 直接操作 vstvec
- **VS-mode 间接访问**：当 V=1 时，VS-mode 执行 `csrr/csrw stvec`（CSR 0x105）实际访问的是 `vstvec` —— Group 3 & 4 采用此路径，在 VS-mode 通过 stvec 指令名设置 vstvec

### 2. trap entry 的安置（Group 3 专用）

Group 3 需要一个 VS-mode 可用的自定义 trap entry，与 sstvecd 类似但运行在 VS-mode 上下文中。要求：

- **4 字节对齐**：满足 `vstvec.BASE` 对齐约束（用 `.align 2` 保证）
- **位于恒等映射范围内**：确保 VS-mode 下 trap entry 地址有效（G-stage 恒等映射覆盖）
- **最小化处理路径**：记录命中 PC 到全局变量 `g_shvstvecd_trap_pc`、记录 `vscause` 到 `g_shvstvecd_trap_cause`，然后 `vsepc += 4` 后 `sret` 返回（V=1 时 `sepc/scause/sret` 实际操作 `vsepc/vscause`）

参考实现思路（伪代码）：

```asm
.section .text
.globl  shvstvecd_trap_entry
.align  2                          /* 4 byte aligned, 满足 BASE 对齐 */
shvstvecd_trap_entry:
    csrrw   t0, sscratch, t0       /* V=1 时实际操作 vsscratch */
    /* 记录命中 PC：entry 起点地址 */
    la      t1, shvstvecd_trap_entry
    la      t0, g_shvstvecd_trap_pc
    sd      t1, 0(t0)
    /* 记录 scause (V=1 时实际读 vscause) */
    csrr    t1, scause
    la      t0, g_shvstvecd_trap_cause
    sd      t1, 0(t0)
    /* sepc += 4，跳过触发 trap 的指令 (V=1 时实际操作 vsepc) */
    csrr    t1, sepc
    addi    t1, t1, 4
    csrw    sepc, t1
    csrrw   t0, sscratch, t0       /* 还原 t0 */
    sret
```

> [!NOTE]
> 实际实施阶段，可参考 `sstvecd` 测试方案中的 `sstvecd_strap.S`。VS-mode 中 `scause/sepc/sscratch/sret` 等指令在 V=1 时自动映射到 `vscause/vsepc/vsscratch` 等 VS CSR。

### 3. VS-mode 的 trap 委托

为使 trap 在 VS-mode 内部被处理（而非 escalate 到 HS-mode），需要通过 `hedeleg` 委托：

- **ecall from VU-mode (cause=8)**：`hedeleg` bit 8 设为 1（环境调用异常委托到 VS）
- **illegal instruction (cause=2)**：`hedeleg` bit 2 设为 1
- **VS software interrupt (VSSIP)**：`hideleg` bit 2 设为 1（VS-level software interrupt 委托到 VS）

使用 `hyp_delegate_to_vs(hedeleg_mask, hideleg_mask)` API 完成委托配置。

### 4. VS-mode 中断的注入（Group 3 VSTVEC-INT-01）

为验证"Direct 模式下中断也跳到 BASE 而非 BASE+4×cause"，使用 VSSIP（virtual supervisor software interrupt）：

1. M/HS-mode 通过 `hvip_set_vssi(true)` 注入 VSSIP pending
2. `hideleg` bit 2 = 1，确保 VSSIP 委托到 VS-mode
3. 进入 VS-mode 后：先 `csrw stvec, BASE`（实际写 vstvec），然后使能 `sie.SSIE`（实际操作 `vsie.SSIE`）+ `sstatus.SIE`（实际操作 `vsstatus.SIE`），中断立即触发
4. trap 命中后，断言：`g_shvstvecd_trap_pc == BASE`（Direct）；若实现是 Vectored，命中地址应为 `BASE + 4*2 = BASE+0x8`（VSSIP cause=2 的 offset），断言失败即可定位到 Direct 行为不正确

> [!NOTE]
> VSSIP 通过 `hvip` 注入是 hypervisor 测试中最简单的 VS-level 中断触发方式，无需 AIA/PLIC 依赖。

### 5. 与 hyp_reset_state 的隔离

`common/hyp/hyp_reset.c::hyp_reset_state()` 默认把 `vstvec` 设为 0（第 98 行）。本测试每个用例在 M-mode 层面先备份 vstvec，操作完成后还原。对于 VS-mode 内的操作（Group 3），利用 `run_in_vs_mode` 的 ecall 返回机制回到 M-mode 后进行断言与清理。

### 6. MODE 字段保留值行为

SPEC 没有将 `vstvec` 声明为 WARL 寄存器（`hypervisor.adoc:1302-1308` 仅描述为 "read/write"），因此对 MODE 字段写入保留值（2, 3）的行为是未定义的，不在测试范围内。

`vstvec.MODE` 字段：

- 写 0（Direct）必然成功（Shvstvecd 强约束）
- 写 1（Vectored）：若实现支持，回读为 1；若不支持，回读为 0。两种行为都不算违反 Shvstvecd
- 写 ≥2（Reserved）：行为未定义（SPEC 未声明 WARL），不做断言

`vstvec.BASE` 字段（[XLEN-1:2]）：

- Shvstvecd 要求在 MODE=Direct 时能保持"any valid 4-byte aligned address"
- 写入的高位 BASE 应原样回读

---

## 测试分组

> [!IMPORTANT]
> 共 5 个测试组、16 个测试用例。Group 1 & 2 运行于 M-mode 直接操作 `vstvec`（CSR 0x205）；Group 3 在 VS-mode (V=1) 内通过 `stvec` 指令名间接操作 `vstvec` 并验证 trap 行为；Group 4 验证 V=1 透传语义；Group 5 探测 Vectored 模式支持（非 normative）。每组提供：规范依据、测试职责、测试用例表（ID/名称/描述/预期结果）；每组提供 1 个关键 C 代码示例。

---

### Group 1：`vstvec.MODE` 可写性

**规范依据**：
- `norm:shvstvecd_vstvec_mode_direct`（`SPEC/shvstvecd.adoc:4-6`）：`vstvec.MODE` 必须能保持值 0（Direct）

**测试职责**：验证 `vstvec.MODE` 能稳定写入并保持 Direct 值（0）；探测 Vectored（1）是否被实现。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSTVEC-MODE-01 | MODE 写 0（Direct）回读 | M-mode `csrw 0x205, 0`（BASE=0, MODE=0），回读 `vstvec[1:0]` | `vstvec[1:0] == 0`（Direct 必然可保持） |
| VSTVEC-MODE-02 | MODE 0→1→0 切换 | 先写 MODE=0、回读确认；再写 MODE=1、回读记录；最后写 MODE=0、回读确认 | 第 1、3 次回读 `MODE==0`；第 2 次回读 ∈ {0, 1}（实现自由） |

#### 关键代码示例：VSTVEC-MODE-01

```c
/* tests/test_mode.c — VSTVEC-MODE-01 */

#include "test_framework.h"
#include "hyp/hyp_csr.h"

#define VSTVEC_MODE_MASK   0x3UL
#define VSTVEC_MODE_DIRECT 0x0UL

TEST_REGISTER(test_shvstvecd_mode_direct_writable);
bool test_shvstvecd_mode_direct_writable(void) {
    TEST_BEGIN("VSTVEC-MODE-01: vstvec.MODE writable to 0 (Direct)");

    uintptr_t saved_vstvec;
    asm volatile ("csrr %0, 0x205" : "=r"(saved_vstvec));

    /* 写入 BASE=0, MODE=0 */
    asm volatile ("csrw 0x205, %0" :: "r"((uintptr_t)0));

    uintptr_t readback;
    asm volatile ("csrr %0, 0x205" : "=r"(readback));
    TEST_ASSERT("vstvec.MODE reads back as 0 (Direct)",
                (readback & VSTVEC_MODE_MASK) == VSTVEC_MODE_DIRECT);

    /* 还原 vstvec */
    asm volatile ("csrw 0x205, %0" :: "r"(saved_vstvec));
    TEST_END();
}
```

---

### Group 2：`vstvec.BASE` 在 Direct 模式下的持有能力

**规范依据**：
- `norm:shvstvecd_vstvec_base_aligned_address`（`SPEC/shvstvecd.adoc:8-10`）：当 `vstvec.MODE`=Direct 时，`vstvec.BASE` 必须能保持任意有效的 4 字节对齐地址
- `norm:stvec_sz_base`（`SPEC/supervisor.adoc:335-338`）：CSR 仅存 BASE 的 [XLEN-1:2]，低 2 bit 写入时被强制为 0

**测试职责**：验证在 MODE=Direct 下，BASE 字段能保持各类 4 字节对齐地址（含跨 1 GiB / 512 GiB 边界、近 VSXLEN 上界）；验证 BASE 与 MODE 写入相互独立。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSTVEC-BASE-01 | BASE 写 0 回读 | `csrw 0x205, 0x0`（BASE=0, MODE=0），回读 | `vstvec == 0x0` |
| VSTVEC-BASE-02 | BASE 写 PLATFORM_MEM_BASE 回读 | `csrw 0x205, PLATFORM_MEM_BASE | 0`，回读 | 回读高位 == `PLATFORM_MEM_BASE`、低 2 bit == 0 |
| VSTVEC-BASE-03 | BASE 写跨 1 GiB 边界 | `csrw 0x205, 0x40000004`（跨 1 GiB），回读 | 回读 == `0x40000004` |
| VSTVEC-BASE-04 | BASE 写跨 512 GiB 边界（高位探测） | `csrw 0x205, 0x8000000000`（bit 39），回读 | 回读 == `0x8000000000`（若高位被截断，证明 Shvstvecd 未真正启用，失败） |
| VSTVEC-BASE-06 | BASE 多次改写不影响 MODE | 固定 MODE=0，依次写 BASE=0x1000、0x2000、0x4000、0x8000，每次回读 MODE | 每次 `MODE == 0` 且 BASE 与写入值一致 |
| VSTVEC-BASE-07 | BASE 高位扫描 | 依次写入 `1ULL << k | 0x4`（k 从 12 到 VSXLEN-1，只取 4 字节对齐位），每次回读 | 每次回读 BASE == 写入值（Shvstvecd 要求"any valid"） |
| VSTVEC-BASE-08 | BASE 最大合法地址（地址空间上界） | 写 `0x7FFFFFFFFFFFFFFC`（VSXLEN-1 位全 1 且 4 字节对齐），回读 | 回读 == `0x7FFFFFFFFFFFFFFC`（Shvstvecd 要求"任意合法 4 字节对齐地址"，应覆盖地址空间上界） |

> [!NOTE]
> **VSTVEC-BASE-04 的高位选择**：bit 39 对应 Sv39 的虚地址上界附近。在 VSXLEN=64 的环境下，Shvstvecd 规范要求 BASE 能保持"any valid"4 字节对齐地址，因此高位不应被截断。

> [!NOTE]
> **VSTVEC-BASE-07 的扫描范围**：按"`PLATFORM_MEM_BASE` 之上、不与 trap entry 冲突"的原则选取若干 k 值（如 k=12, 16, 20, 24, 28, 32, 36, 38），不必穷举到 63 位。

#### 关键代码示例：VSTVEC-BASE-06（多次改写不影响 MODE）

```c
/* tests/test_base.c — VSTVEC-BASE-06 */

#include "test_framework.h"

TEST_REGISTER(test_shvstvecd_base_06_independent_of_mode);
bool test_shvstvecd_base_06_independent_of_mode(void) {
    TEST_BEGIN("VSTVEC-BASE-06: BASE rewrites do not disturb MODE=Direct");

    VSTVEC_SAVE();

    /* Pin MODE=Direct and rewrite BASE several times. */
    static const uintptr_t bases[] = {
        0x1000UL, 0x2000UL, 0x4000UL, 0x8000UL,
    };
    const unsigned n = sizeof(bases) / sizeof(bases[0]);

    for (unsigned i = 0; i < n; i++) {
        uintptr_t target = bases[i] | VSTVEC_MODE_DIRECT;
        vstvec_write_raw(target);
        uintptr_t rb = vstvec_read_raw();
        TEST_ASSERT("BASE preserved across rewrite",
                    (rb & VSTVEC_BASE_MASK) == bases[i]);
        TEST_ASSERT("MODE remains Direct (=0)",
                    (rb & VSTVEC_MODE_MASK) == VSTVEC_MODE_DIRECT);
    }

    VSTVEC_RESTORE();
    TEST_END();
}
```

---

### Group 3：`MODE=Direct` 下 VS-mode trap 跳转到 BASE

**规范依据**：
- `SPEC/supervisor.adoc:355-364`：MODE=Direct 时，所有 trap（同步异常 + 异步中断）都把 `pc` 设为 BASE（vstvec 格式与 stvec 相同）
- `SPEC/hypervisor.adoc:1305-1307`：V=1 时 `vstvec` 替代 `stvec`
- `norm:H_trap_vs_csrwrites`（`SPEC/hypervisor.adoc:2549-2554`）：trap 进入 VS-mode 时写 `vsepc`、`vscause`、`vstval`，跳转目标为 `vstvec` 指定的地址
- Shvstvecd 强约束 Direct 必然可用（`norm:shvstvecd_vstvec_mode_direct`），因此 Direct 行为正确性是 Shvstvecd 的隐含承诺

**测试职责**：验证当 `vstvec.MODE=Direct` 时，在 VS-mode 中触发的同步异常与异步中断都跳转到 BASE（而不是 BASE+4×cause）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSTVEC-DIR-01 | 同步异常：VU-mode ecall | 在 VS-mode 中通过 `vstvec`（实际通过 stvec 指令）设为自定义 4 字节对齐 entry；触发 ecall from VU-mode（cause=8），trap 委托到 VS-mode | `g_shvstvecd_trap_pc == BASE`；`g_shvstvecd_trap_cause == 8` |
| VSTVEC-DIR-02 | 同步异常：illegal instruction | VS-mode 执行未知指令（cause=2），委托到 VS-mode | `g_shvstvecd_trap_pc == BASE`；`g_shvstvecd_trap_cause == 2` |
| VSTVEC-INT-01 | 异步中断：VSSIP | `hvip` 注入 VSSIP，`hideleg` bit 2 = 1 委托到 VS-mode；VS-mode 设 vstvec=entry 后使能中断 | `g_shvstvecd_trap_pc == BASE`（Direct 不加 4×cause） |
| VSTVEC-DIR-03 | 同步异常：load page-fault | VS-mode 访问未映射虚拟地址触发 load page-fault（cause=13），`hedeleg` bit 13 委托到 VS-mode | `g_shvstvecd_trap_pc == BASE`；`g_shvstvecd_trap_cause == 13` |

> [!NOTE]
> **VSTVEC-DIR-01 的触发方式**：VS-mode 先设置好 vstvec（通过 `csrw stvec, entry`，V=1 时实际操作 vstvec），然后切换到 VU-mode 执行 `ecall`（cause=8）。由于 `hedeleg` bit 8 已设为 1，ecall from VU 会被委托到 VS-mode，trap entry 跳转到 vstvec.BASE。

> [!NOTE]
> **VSTVEC-INT-01 的中断源**：使用 `hvip_set_vssi(true)` 从 HS/M-mode 注入 VSSIP pending，然后进入 VS-mode 并使能 `vsie.SSIE` + `vsstatus.SIE`。这是 H 扩展中触发 VS-level 中断最简洁的方式，无需外部中断控制器。

#### 关键代码示例：VSTVEC-DIR-01（VU-mode ecall 命中 BASE）

```c
/* tests/test_direct.c — VSTVEC-DIR-01 */

#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"

extern void shvstvecd_trap_entry(void);          /* 见"设计要点 2"：本地 4 byte 对齐 entry */
extern volatile uintptr_t g_shvstvecd_trap_pc;
extern volatile uintptr_t g_shvstvecd_trap_cause;

static uintptr_t vsmode_setup_and_trigger_ecall(uintptr_t arg) {
    uintptr_t entry = arg;
    /* V=1 时 csrw stvec 实际写 vstvec；MODE=0 (Direct) */
    asm volatile ("csrw stvec, %0" :: "r"(entry));
    /* 切换到 VU-mode 并触发 ecall (cause=8)；
     * ecall 被 hedeleg 委托到 VS-mode，命中 vstvec.BASE */
    asm volatile ("ecall");  /* 在 VU-mode 中执行；此处简化为 VS-mode 内 ecall 示意 */
    return 0;
}

TEST_REGISTER(test_shvstvecd_direct_ecall);
bool test_shvstvecd_direct_ecall(void) {
    TEST_BEGIN("VSTVEC-DIR-01: Direct mode, VU ecall lands at vstvec.BASE");

    /* 委托 ecall-from-VU (cause=8) 到 VS-mode */
    hyp_delegate_to_vs(1UL << 8, 0);

    uintptr_t entry = (uintptr_t)&shvstvecd_trap_entry;
    TEST_ASSERT("entry is 4-byte aligned", (entry & 0x3UL) == 0);

    g_shvstvecd_trap_pc    = 0;
    g_shvstvecd_trap_cause = 0;

    /* 在 VS-mode 中设置 vstvec 并触发 trap */
    run_in_vs_mode(vsmode_setup_and_trigger_ecall, entry);

    /* Direct 模式：trap 命中地址必须 == BASE */
    TEST_ASSERT("trap PC equals vstvec.BASE", g_shvstvecd_trap_pc == entry);
    TEST_ASSERT("vscause == 8 (ecall from VU-mode)",
                g_shvstvecd_trap_cause == 8);

    hyp_undelegate();
    HYP_TEST_END();
}
```

#### 关键代码示例：VSTVEC-INT-01（VSSIP 命中 BASE 而非 BASE+offset）

```c
/* tests/test_direct.c — VSTVEC-INT-01 */

static uintptr_t vsmode_enable_vssi_and_wait(uintptr_t arg) {
    uintptr_t entry = arg;
    /* V=1 时 csrw stvec 实际写 vstvec；MODE=0 (Direct) */
    asm volatile ("csrw stvec, %0" :: "r"(entry));
    /* 使能 SSIE（V=1 时实际操作 vsie.SSIE） */
    asm volatile ("csrs sie, %0" :: "r"(1UL << 1));
    /* 使能 SIE（V=1 时实际操作 vsstatus.SIE）— 中断立即触发 */
    asm volatile ("csrs sstatus, %0" :: "r"(1UL << 1));
    /* VSSIP 在使能瞬间即触发，handler 会处理并返回 */
    asm volatile ("nop");
    return 0;
}

TEST_REGISTER(test_shvstvecd_direct_vssip);
bool test_shvstvecd_direct_vssip(void) {
    TEST_BEGIN("VSTVEC-INT-01: Direct mode, VSSIP lands at BASE (not BASE+offset)");

    /* 委托 VSSIP 到 VS-mode */
    hyp_delegate_to_vs(0, 1UL << 2);

    uintptr_t entry = (uintptr_t)&shvstvecd_trap_entry;
    g_shvstvecd_trap_pc    = 0;
    g_shvstvecd_trap_cause = 0;

    /* 注入 VSSIP pending */
    hvip_set_vssi(true);

    /* 进入 VS-mode，设置 vstvec 并使能中断 */
    run_in_vs_mode(vsmode_enable_vssi_and_wait, entry);

    /* Direct 模式：中断也应跳到 BASE，而非 BASE + 4*cause */
    TEST_ASSERT("trap PC equals BASE (Direct, not Vectored)",
                g_shvstvecd_trap_pc == entry);
    /* vscause 高位（interrupt bit）= 1，低位 = 1 (SSI) */
    TEST_ASSERT("vscause is interrupt-1 (VSSI)",
                (g_shvstvecd_trap_cause >> (sizeof(uintptr_t)*8 - 1)) == 1 &&
                (g_shvstvecd_trap_cause & ~(1UL << (sizeof(uintptr_t)*8 - 1))) == 1);

    /* 清理 */
    hvip_set_vssi(false);
    hyp_undelegate();
    HYP_TEST_END();
}
```

---

### Group 4：V=1 透传验证（`stvec` 访问实际操作 `vstvec`）

**规范依据**：
- `norm:vstvec_sz_acc_op`（`SPEC/hypervisor.adoc:1302-1308`）：When V=1, `vstvec` substitutes for the usual `stvec`, so instructions that normally read or modify `stvec` actually access `vstvec` instead

**测试职责**：验证在 VS-mode (V=1) 通过 `stvec` 指令名写入的值能从 HS/M-mode 通过 `vstvec`（CSR 0x205）读回；反向亦然。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSTVEC-TRANS-01 | VS-mode 写 stvec → M-mode 读 vstvec | VS-mode 写 `stvec = 0xDEAD0000`，返回 M-mode 后读 `vstvec`（CSR 0x205） | `vstvec == 0xDEAD0000`（MODE=0, BASE=0xDEAD0000） |
| VSTVEC-TRANS-02 | M-mode 写 vstvec → VS-mode 读 stvec | M-mode 写 `vstvec = 0xBEEF0004`，进入 VS-mode 后读 `stvec` | VS-mode 读回 == `0xBEEF0004` |
| VSTVEC-TRANS-03 | VS-mode 写 stvec 不影响 HS-mode 的 stvec | VS-mode 写 `stvec = 0x12340000`，返回 HS-mode 后读真实 `stvec`（CSR 0x105 in V=0） | HS-mode 的 `stvec` 保持原值（未被 VS-mode 操作修改） |

#### 关键代码示例：VSTVEC-TRANS-01

```c
/* tests/test_transparent.c — VSTVEC-TRANS-01 */

#include "test_framework.h"
#include "hyp/hyp_priv.h"
#include "hyp/hyp_csr.h"
#include "hyp/hyp_test.h"

static uintptr_t vsmode_write_stvec(uintptr_t arg) {
    /* V=1：csrw stvec 实际写 vstvec */
    asm volatile ("csrw stvec, %0" :: "r"(arg));
    return 0;
}

TEST_REGISTER(test_shvstvecd_transparent_vs_write);
bool test_shvstvecd_transparent_vs_write(void) {
    TEST_BEGIN("VSTVEC-TRANS-01: VS writes stvec, M reads vstvec");

    uintptr_t saved_vstvec;
    asm volatile ("csrr %0, 0x205" : "=r"(saved_vstvec));

    uintptr_t test_val = 0xDEAD0000UL;  /* 4-byte aligned, MODE=0 */
    run_in_vs_mode(vsmode_write_stvec, test_val);

    uintptr_t readback;
    asm volatile ("csrr %0, 0x205" : "=r"(readback));
    TEST_ASSERT("vstvec == value written by VS-mode via stvec",
                readback == test_val);

    asm volatile ("csrw 0x205, %0" :: "r"(saved_vstvec));
    HYP_TEST_END();
}
```

---

### Group 5：Vectored 模式探测（非 normative，完善性验证）

**规范依据**：
- Shvstvecd 规范**不要求**实现 Vectored 模式（MODE=1），仅要求 Direct（MODE=0）可持有
- `SPEC/supervisor.adoc:355-364`：stvec MODE=1（Vectored）时，中断跳转到 `BASE + 4×cause`

**测试职责**：探测 vstvec.MODE 是否支持 Vectored（仅作信息收集）；若支持，验证中断跳转行为与 Direct 的差异。

> [!NOTE]
> 本组测试不属于 Shvstvecd 的 normative 覆盖范围。VSTVEC-VEC-01 仅探测并记录结果，不做 pass/fail 判断。VSTVEC-VEC-02 仅在 Vectored 可用时才执行功能验证。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| VSTVEC-VEC-01 | Vectored 模式可写性探测 | M-mode 写 `vstvec = BASE | 1`（MODE=Vectored），回读检查 MODE 字段 | 若回读 `MODE == 1`，记录"Vectored supported"；若回读 `MODE == 0`，记录"Vectored not supported, TEST_SKIP VEC-02" |
| VSTVEC-VEC-02 | Vectored 中断跳转到 BASE+4×cause | 前提：VEC-01 确认 Vectored 可用。VS-mode 设 `vstvec = entry | 1`；注入 VSSIP（cause=1） | `g_shvstvecd_trap_pc == BASE + 4*1`（Vectored 模式中断偏移） |

#### 关键代码示例：VSTVEC-VEC-01

```c
/* tests/test_vectored.c — VSTVEC-VEC-01 */

#include "test_framework.h"
#include "hyp/hyp_test.h"

TEST_REGISTER(test_shvstvecd_vectored_probe);
bool test_shvstvecd_vectored_probe(void) {
    TEST_BEGIN("VSTVEC-VEC-01: probe vstvec Vectored mode support");

    uintptr_t saved;
    asm volatile ("csrr %0, 0x205" : "=r"(saved));

    /* 写 MODE=1 (Vectored) + BASE=0x80001000 */
    uintptr_t write_val = 0x80001000UL | 1UL;
    asm volatile ("csrw 0x205, %0" :: "r"(write_val));

    uintptr_t readback;
    asm volatile ("csrr %0, 0x205" : "=r"(readback));
    unsigned int mode = readback & 0x3;

    if (mode == 1) {
        TEST_NOTE("vstvec Vectored mode (MODE=1) is SUPPORTED on this platform");
    } else {
        TEST_NOTE("vstvec Vectored mode (MODE=1) is NOT supported (readback MODE=%u)", mode);
    }

    /* 无论是否支持，此测试都 PASS（仅探测） */
    asm volatile ("csrw 0x205, %0" :: "r"(saved));
    HYP_TEST_END();
}
```

---

## 附录：相关常量与 API 参考

### CSR 与位定义

| 名称 | 值 | 说明 |
|------|-----|------|
| `CSR_VSTVEC` | `0x205` | Virtual supervisor trap vector base address，见 `common/encoding.h:289` |
| `CSR_STVEC` | `0x105` | Supervisor trap vector base address（V=1 时访问实际操作 vstvec） |
| `VSTVEC_MODE_MASK` | `0x3` | `vstvec[1:0]` 即 MODE 字段 |
| `VSTVEC_MODE_DIRECT` | `0x0` | Direct 模式 |
| `VSTVEC_MODE_VECTORED` | `0x1` | Vectored 模式（Shvstvecd 不要求实现） |
| `HIDELEG_VSSIP_BIT` | `1UL << 2` | 委托 VSSIP 到 VS-mode |
| `HVIP_VSSIP_BIT` | `1UL << 2` | hvip 中 VSSIP pending 位 |

### vscause 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| `CAUSE_ECALL_FROM_VU` | 8 | Environment call from VU-mode |
| `CAUSE_ILLEGAL_INSTRUCTION` | 2 | Illegal instruction |
| `CAUSE_VS_SOFTWARE_INTERRUPT`（中断号） | 1 | VS software interrupt（vscause 高位置 1）|

### 测试框架 API

- `run_in_vs_mode(fn, arg)`：在 VS-mode (V=1) 执行 fn(arg)，通过 ecall 返回 M-mode
- `run_in_vu_mode(fn, arg)`：在 VU-mode (V=1, U) 执行 fn(arg)
- `hyp_delegate_to_vs(hedeleg_mask, hideleg_mask)`：配置异常/中断委托到 VS-mode
- `hyp_undelegate()`：清除所有 hypervisor 委托
- `hvip_set_vssi(pending)`：注入/清除 VSSIP
- `hyp_reset_state()`：重置所有 hypervisor CSR（含 vstvec=0）
- `HYP_TEST_END()`：测试结束宏，含 hyp_reset_state + 结果记录
- `TEST_REGISTER` / `TEST_BEGIN` / `TEST_ASSERT` / `TEST_END`：测试用例注册与断言宏

### 全局变量（`shvstvecd/tests/shvstvecd_strap.S` 提供）

| 变量 | 类型 | 说明 |
|------|------|------|
| `g_shvstvecd_trap_pc` | `volatile uintptr_t` | VS-mode trap entry 命中时的 PC（应等于 `vstvec.BASE`） |
| `g_shvstvecd_trap_cause` | `volatile uintptr_t` | trap 命中时的 `vscause` |
| `shvstvecd_trap_entry` | `void(void)` | 4 字节对齐的 VS-mode trap entry 符号 |

---

## 测试执行说明

### 运行环境

- Group 1 & 2：M-mode 直接操作 CSR 0x205（vstvec），无需进入 VS-mode
- Group 3：通过 `run_in_vs_mode` 进入 VS-mode，使用 G-stage 恒等映射（Sv39x4）保证 trap entry 地址可访问
- Group 4：M-mode ↔ VS-mode 交替执行，验证 CSR 透传
- 单核环境，无需 IPI

### 失败定位指引

| 现象 | 可能原因 |
|------|----------|
| VSTVEC-MODE-01 失败 | 平台连基本的 vstvec 都未实现，或 `csrw 0x205` 路径异常 |
| VSTVEC-BASE-04 / 07 失败 | Shvstvecd 未真正启用，BASE 高位被 WARL 截断 |
| VSTVEC-DIR-01/02 失败（命中地址 ≠ BASE） | trap 委托路径异常，或 hedeleg 未正确配置 |
| VSTVEC-INT-01 失败（命中地址 = BASE+offset） | 实现错误地按 Vectored 处理中断；说明 MODE=0 写入未生效 |
| VSTVEC-INT-01 失败（无命中） | `hideleg` 未委托 VSSIP，或 `hvip` 未注入，或 `vsstatus.SIE` 未使能 |
| VSTVEC-TRANS-01/02 失败 | V=1 时 stvec 未正确映射到 vstvec，H 扩展实现异常 |
| VSTVEC-TRANS-03 失败 | VS-mode 写 stvec 错误地修改了 HS-mode 的 stvec |
