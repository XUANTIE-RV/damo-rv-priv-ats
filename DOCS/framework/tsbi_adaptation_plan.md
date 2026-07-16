# T-SBI Adaptation Plan (Virtualization Tests First)

> **规范来源**: [ACT4 abstraction.adoc Test Supervisor Binary Interface](https://github.com/riscv/riscv-arch-test/blob/act4/docs/ctp/src/abstraction.adoc#test-supervisor-binary-interface-t-sbi)

> **实施决策（2026）**：采用 **目标 B（RVA23 认证兼容）**——对齐 custom M-mode + T-SBI 认证模型；GOTO 编码 **采用 0x1~0x5**（对齐 ACT4 参考实现 rvtest_trap_handler.h，但**不引入其 x3==0 legacy flag**，dispatch 纯由 a0 范围判定）；CSR_ACCESS 错误处理采用 **策略 A**。详见 §5 改造路径。

## 1. 背景与目标

RISC-V 社区在 ACT4（Architecture Compliance Test v4）框架中定义了 **T-SBI（Test Supervisor Binary Interface）** 规范——一种面向测试场景的轻量级 ecall 服务接口。

### 1.1 为什么不用标准 SBI（OpenSBI）

| 问题 | 说明 |
|------|------|
| 体积过大 | OpenSBI 编译产物 ~1MB，加载到每个测试不合理 |
| 寄存器约定冲突 | 标准 SBI 使用 a7/a6，RV32E 无此寄存器 |
| 功能冗余 | 测试场景仅需特权切换 + CSR 代理，不需要完整 SBI 服务 |

T-SBI 的核心设计：**用 a0/a1 传参，轻量级自定义调用约定，与标准 SBI 不兼容但满足测试需求**。

### 1.2 本方案目标

**纯自研实现** T-SBI 协议语义，不拉取 ACT4 仓库任何源文件，仅参考其公开规范定义，在本仓库 `damo-priv-test` 的 `common/` 框架中实现兼容接口，先从虚拟化测试用例入手验证。

> **注意**：RVMODEL_* / RVTEST_* 宏属于 Target 适配层（屏蔽底层硬件差异，每个 target 独立适配），与 T-SBI 协议本身无关，不在本方案范围内。

---

## 2. T-SBI 规范解构

### 2.1 调用约定

| 项目 | 规范定义 |
|------|----------|
| 触发方式 | `ecall` 指令 |
| 操作码传递 | `a0` 寄存器 |
| 参数传递 | `a1` 寄存器（部分操作无参数） |
| 返回值 | `a0` 寄存器（ECALL_TEST 返回 xEPC，CSR_READ 返回值） |
| 处理层 | 各级 trap handler（M / HS / VS），按能力处理或向上转发 |

### 2.2 服务接口定义

T-SBI 定义两类服务：**核心 T-SBI 调用**（所有测试套件必须实现）和 **ACT 扩展调用**（ACT4 trap handler 提供的便利接口）。

#### 2.2.1 核心 T-SBI 调用

##### ECALL_TEST — 测试 ecall 到执行环境

| 操作码 (a0) | 符号名 | 行为 |
|-------------|--------|------|
| `0x00000073` | ECALL_TEST | 测试 ecall 本身，返回 xEPC 到 a0（证明 trap handler 被进入） |

> **编码说明**：`0x73` 正是 `ecall` 指令的完整机器码。当 funct3=0（即 `a0[14:12]==0`）时，handler 识别为 ECALL_TEST 而非 CSR_ACCESS。

##### CSR 代理操作 — a0 即 CSR 指令编码

| 操作名 | a0 编码 | 固定低 20 位 | 等价指令 | a1 | 返回 a0 |
|--------|---------|-------------|----------|-----|---------|
| CSR_WRITE | `(csr<<20) \| 0x59073` | `0x59073` | `csrrw x0, csr, a1` | 写入值 | — |
| CSR_SET | `(csr<<20) \| 0x5A073` | `0x5A073` | `csrrs x0, csr, a1` | set mask | — |
| CSR_CLEAR | `(csr<<20) \| 0x5B073` | `0x5B073` | `csrrc x0, csr, a1` | clear mask | — |
| CSR_READ | `(csr<<20) \| 0x02573` | `0x02573` | `csrrs a0, csr, x0` | — | CSR 值 |

**关键设计**：a0 直接是 RISC-V CSR 指令的 32 位编码！Handler 可以直接将 a0 当作指令写入可执行区域并跳转执行。

**示例**：S-mode 设置 mstatus.TVM：
```
a0 = 0x3005A073  (即 csrrs x0, mstatus, a1 的编码)
a1 = (1 << 20)   (TVM bit)
ecall
```

**判定规则**：`(a0 & 0x7F) == 0x73 && (a0[14:12]) != 0` → CSR_ACCESS

#### 2.2.2 ACT 扩展调用 — 特权模式切换

| 操作码 (a0) | 符号名 | 目标特权级 |
|-------------|--------|-----------|
| `0x00000001` | GOTO_MMODE | M-mode（需 STANDARD_SM_SUPPORTED） |
| `0x00000002` | GOTO_SMODE | S/HS-mode |
| `0x00000003` | GOTO_UMODE | U-mode |
| `0x00000004` | GOTO_VSMODE | VS-mode (V=1) |
| `0x00000005` | GOTO_VUMODE | VU-mode (V=1) |

**行为语义**：
- Handler 接到 ecall 后，根据 a0 确定目标特权级
- 通过配置 `mstatus.MPP`（+ H 扩展下的 `mstatus.MPV`）+ `mret` 完成切换
- `mepc` 设为 ecall 的下一条指令（ecall+4）
- 切换完成后，执行流在目标特权级从 ecall 下一条指令继续

> **注意**：这些操作码为小整数 1~5，bits[6:0] ≠ 0x73，不会与 ECALL_TEST / CSR_ACCESS 冲突；且 a0=0 不属于合法 GOTO（走 armed-trap），我们不采用参考实现的 x3==0 legacy 机制。

#### 2.2.3 原始 ecall 测试（Raw Ecall as Test Stimulus）

当测试需要将 ecall 本身作为被测行为（如验证 ecall 在不同特权级的 trap 路径），需要一种机制告诉 handler："这次 ecall 不是 T-SBI 服务请求，请当作普通异常处理"。

**方案**：通过框架 API `tsbi_arm_raw_ecall(true)` 设置标志位，trap handler 检查该标志后跳过 T-SBI dispatch，将 ecall 视为普通 armed trap 记录到 `trap_record`。

### 2.3 Handler 链式转发机制

ACT4 规范明确要求：**S/HS-mode 和 VS-mode trap handler 也实现相同的 T-SBI 调用集**。

#### 转发规则

```
ecall from VS/VU-mode:
    → trap 到 HS-mode handler
    → HS handler 检查:
        ├─ 能处理（如 S-mode CSR 访问）→ 直接处理并返回
        └─ 不能处理（如 M-mode CSR / GOTO_MMODE）→ 再次 ecall 转发给 M-mode

ecall from S/U-mode:
    → trap 到 M-mode handler
    → M-mode handler 直接处理
```

#### 各层 Handler 职责

| Handler 层 | 能直接处理的请求 | 需要向上转发的请求 |
|------------|-----------------|-------------------|
| VS-mode handler | 无（VS ecall 总是 trap 到 HS） | 全部 |
| HS-mode handler | S-mode CSR、GOTO_SMODE/UMODE/VSMODE/VUMODE | M-mode CSR、GOTO_MMODE |
| M-mode handler | 所有请求 | — |

### 2.4 协议判定流程（完整）

```
ecall trapped to handler (M-mode / HS-mode):
│
├─ if (_skip_tsbi_dispatch == true)
│   └─ 走 armed-trap 路径（trap_record 记录）
│
├─ if (a0 >= 0x1 && a0 <= 0x5)
│   ├─ 本层能处理 → 配置 xPP/xPV + xret
│   └─ 本层不能处理 → ecall 转发上层
│
├─ if ((a0 & 0x7F) == 0x73)
│   ├─ funct3 == 0 → ECALL_TEST: 返回 xEPC 到 a0
│   └─ funct3 != 0 → CSR_ACCESS:
│       ├─ CSR 属于本层权限 → 动态指令注入执行
│       └─ CSR 属于更高特权级 → ecall 转发上层
│
└─ else
    └─ 未知操作 / 走 armed-trap 路径
```

---

## 3. 当前框架差异分析

### 3.1 现有实现 vs T-SBI 规范

| 维度 | 当前实现 | T-SBI 规范要求 |
|------|---------|---------------|
| 参数传递 | 全局变量 `ecall_args[2]` | 寄存器 a0/a1 |
| 操作码 | 单一 `ECALL_GOTO_PRIV=1`，a1 传 target | a0 直接编码目标（0x1-0x5） |
| ECALL_TEST | 无 | a0=0x73，返回 xEPC |
| VS/VU 编码 | `PRIV_VS=5, PRIV_VU=4`（bit2=V） | `GOTO_VSMODE=0x4, VUMODE=0x5` |
| CSR 代理 | 无（依赖 M-mode 直接 CSRR/CSRW） | CSR_ACCESS 动态指令注入 |
| 原始 ecall | `ecall_args[0]=0` 手动重置 | 框架级标志位 API |
| 汇编入口 | 不保存 a0/a1 到专用字段 | 需在 context save 阶段捕获 a0/a1 |
| Handler 层级 | 仅 M-mode 处理 ecall | M + HS 双层 dispatch，链式转发 |

### 3.2 影响范围

**框架核心文件**（需改造）：
- `common/trap.c` — M-mode handler ecall dispatch 逻辑（改为 T-SBI 协议）
- `common/trap.c` — HS-mode handler 新增 T-SBI dispatch + 转发逻辑
- `common/trap_asm.S` — 汇编入口需保存 a0/a1 到 trap frame
- `common/privilege.c` — `do_ecall()` 实现改为寄存器约定
- `common/hyp/hyp_priv.c` — `_v_trampoline` / `return_to_hs_mode` ecall 调用点

**测试代码**（需迁移，3 处虚拟化相关）：
- `Hypervisor/tests/test_helpers.c` — `ecall_args[0]=0`
- `Hypervisor/tests/test_exception_priority.c` — `ecall_args[0]=0`
- `Shtvala/tests/test_htval_clear.c` — `ecall_args[0]=0`

**不受影响**：
- 所有测试用例的断言逻辑（`trap_expect`、`CHECK_TRAP`、`run_in_vs_mode` 等 API 签名不变）
- 外部 API 接口（`run_in_vs_mode`、`run_in_vu_mode`、`goto_priv` 等函数签名保持不变）

---

## 4. 接口设计方案

### 4.1 新增头文件：`common/tsbi.h`

```c
#ifndef TSBI_H
#define TSBI_H

/* ===================================================================
 * T-SBI (Test Supervisor Binary Interface) Protocol Constants
 *
 * Compatible with ACT4 T-SBI specification.
 * Ref: riscv-arch-test/docs/ctp/src/abstraction.adoc
 * Pure self-implementation — no external source dependency.
 * =================================================================== */

#include <stdint.h>
#include <stdbool.h>

/* --- ECALL_TEST --- */
#define TSBI_ECALL_TEST   0x00000073  /* a0=ecall opcode, returns xEPC */

/* --- Privilege Mode Transition (ACT extension) --- */
#define TSBI_GOTO_MMODE   0x00000001  /* Switch to M-mode */
#define TSBI_GOTO_SMODE   0x00000002  /* Switch to S-mode / HS-mode */
#define TSBI_GOTO_UMODE   0x00000003  /* Switch to U-mode */
#define TSBI_GOTO_VSMODE  0x00000004  /* Switch to VS-mode (V=1) */
#define TSBI_GOTO_VUMODE  0x00000005  /* Switch to VU-mode (V=1) */

/* --- CSR_ACCESS encoding ---
 * a0 = complete RISC-V CSR instruction encoding
 * The handler can write this word to memory + fence.i + jump to execute.
 *
 * Encoding: csr[31:20] | rs1[19:15] | funct3[14:12] | rd[11:7] | 0x73[6:0]
 */
#define TSBI_CSR_WRITE_ENCODE(csr)  (((uint32_t)(csr) << 20) | 0x59073)  /* csrrw x0, csr, a1 */
#define TSBI_CSR_SET_ENCODE(csr)    (((uint32_t)(csr) << 20) | 0x5A073)  /* csrrs x0, csr, a1 */
#define TSBI_CSR_CLEAR_ENCODE(csr)  (((uint32_t)(csr) << 20) | 0x5B073)  /* csrrc x0, csr, a1 */
#define TSBI_CSR_READ_ENCODE(csr)   (((uint32_t)(csr) << 20) | 0x02573)  /* csrrs a0, csr, x0 */

/* --- Protocol detection helpers --- */
#define TSBI_IS_GOTO_MODE(a0)   ((a0) >= 0x1 && (a0) <= 0x5)
#define TSBI_IS_SYSTEM_OP(a0)   (((a0) & 0x7F) == 0x73)
#define TSBI_IS_ECALL_TEST(a0)  ((a0) == 0x73)
#define TSBI_IS_CSR_ACCESS(a0)  (TSBI_IS_SYSTEM_OP(a0) && (((a0) >> 12) & 7) != 0)

/* --- Raw ecall control --- */
void tsbi_arm_raw_ecall(bool skip_dispatch);

/* --- CSR proxy wrappers (callable from any priv level) --- */
uintptr_t tsbi_csr_read(uint16_t csr);
void      tsbi_csr_write(uint16_t csr, uintptr_t val);
uintptr_t tsbi_csr_set(uint16_t csr, uintptr_t mask);
uintptr_t tsbi_csr_clear(uint16_t csr, uintptr_t mask);

/* --- Privilege transition wrappers --- */
uintptr_t tsbi_ecall_test(void);      /* returns xEPC */
void      tsbi_goto_mmode(void);
void      tsbi_goto_smode(void);
void      tsbi_goto_umode(void);
void      tsbi_goto_vsmode(void);
void      tsbi_goto_vumode(void);

#endif /* TSBI_H */
```

### 4.2 ecall 发起接口：`tsbi_ecall(op, arg)`

替换现有 `do_ecall()`，改为标准寄存器约定：

```c
static inline uintptr_t tsbi_ecall(uintptr_t op, uintptr_t arg) {
    register uintptr_t a0 asm("a0") = op;
    register uintptr_t a1 asm("a1") = arg;
    asm volatile("ecall" : "+r"(a0) : "r"(a1) : "memory");
    return a0;  /* CSR_ACCESS 的返回值 */
}
```

### 4.3 M-mode Handler Dispatch

```c
unsigned m_trap_handler(void) {
    uintptr_t cause = CSRR(mcause);
    uintptr_t epc   = CSRR(mepc);
    uintptr_t a0    = saved_a0;  /* 从 trap frame 取 */
    uintptr_t a1    = saved_a1;

    if (is_ecall(cause)) {
        /* 1) 检查 raw-ecall 标志 */
        if (_skip_tsbi_dispatch) {
            _skip_tsbi_dispatch = false;
            goto armed_trap_path;
        }

        /* 2) 特权模式切换 (0x1-0x5) */
        if (TSBI_IS_GOTO_MODE(a0)) {
            unsigned target = tsbi_op_to_priv(a0);
            CSRW(mepc, epc + 4);
            configure_mpp_mpv(target);
            current_priv = target;
            return PRIV_M;
        }

        /* 3) SYSTEM opcode family (bits[6:0] == 0x73) */
        if (TSBI_IS_SYSTEM_OP(a0)) {
            if (TSBI_IS_ECALL_TEST(a0)) {
                /* ECALL_TEST: return xEPC to caller */
                set_trap_frame_a0(epc);
                CSRW(mepc, epc + 4);
                return PRIV_M;
            }
            /* CSR_ACCESS: funct3 != 0 */
            uintptr_t result = csr_access_dispatch((uint32_t)a0, a1);
            set_trap_frame_a0(result);
            CSRW(mepc, epc + 4);
            return PRIV_M;
        }

        /* 4) 未知操作 → armed trap */
    }

armed_trap_path:
    /* 原有 trap_record 逻辑不变 ... */
}
```

### 4.4 HS-mode Handler Dispatch（新增）

```c
unsigned hs_trap_handler(void) {
    uintptr_t cause = CSRR(scause);
    uintptr_t epc   = CSRR(sepc);
    uintptr_t a0    = saved_a0;
    uintptr_t a1    = saved_a1;

    if (is_ecall(cause)) {
        if (_skip_tsbi_dispatch) {
            _skip_tsbi_dispatch = false;
            goto armed_trap_path;
        }

        /* 特权模式切换 */
        if (TSBI_IS_GOTO_MODE(a0)) {
            if (a0 == TSBI_GOTO_MMODE) {
                /* HS 无法切到 M-mode，转发给 M */
                ecall_forward_to_m(a0, a1);
                return PRIV_S;
            }
            /* SMODE/UMODE/VSMODE/VUMODE 本层可处理 */
            unsigned target = tsbi_op_to_priv(a0);
            CSRW(sepc, epc + 4);
            configure_spp_spv(target);
            current_priv = target;
            return PRIV_S;
        }

        /* SYSTEM opcode */
        if (TSBI_IS_SYSTEM_OP(a0)) {
            uint32_t csr_addr = (a0 >> 20) & 0xFFF;
            if (TSBI_IS_ECALL_TEST(a0)) {
                set_trap_frame_a0(epc);
                CSRW(sepc, epc + 4);
                return PRIV_S;
            }
            /* CSR 权限判断：M-mode CSR (0x3xx, 0x7xx, 0xBxx, 0xFxx) 转发 */
            if (is_m_mode_csr(csr_addr)) {
                ecall_forward_to_m(a0, a1);
                return PRIV_S;
            }
            /* S-mode CSR 本层可处理 */
            uintptr_t result = csr_access_dispatch((uint32_t)a0, a1);
            set_trap_frame_a0(result);
            CSRW(sepc, epc + 4);
            return PRIV_S;
        }
    }

armed_trap_path:
    /* trap_record 逻辑 ... */
}
```

### 4.5 CSR_ACCESS 动态指令注入引擎

> **关键设计**：规范明确 a0 本身就是完整的 RISC-V CSR 指令编码（含正确的 rd/rs1/funct3），
> handler 应**直接将 a0 写入可执行区域并执行**，不需要拆解重组。
>
> 各操作的 rd/rs1 已由调用者正确编码：
> - CSR_WRITE: `csrrw x0, csr, a1` (rd=x0, rs1=a1 → 只写不读)
> - CSR_SET:   `csrrs x0, csr, a1` (rd=x0, rs1=a1 → 设置位)
> - CSR_CLEAR: `csrrc x0, csr, a1` (rd=x0, rs1=a1 → 清除位)
> - CSR_READ:  `csrrs a0, csr, x0` (rd=a0, rs1=x0 → 只读不改)

```c
/* 可执行 buffer（需 RWX 属性，通过 linker script 配置） */
static uint32_t __attribute__((aligned(16), section(".trampoline")))
    csr_trampoline[2];

uintptr_t csr_access_dispatch(uint32_t encoded_a0, uintptr_t rs1_val) {
    /*
     * encoded_a0 就是完整的 CSR 指令，直接写入 trampoline 执行。
     * 不做拆解重组 — 规范保证 rd/rs1/funct3 已由调用者正确编码。
     *
     * 参考 abstraction.adoc:
     *   "the trap handler to write this value to memory, perform fence.i
     *    to synchronize the instruction stream, execute the instruction"
     */
    csr_trampoline[0] = encoded_a0;
    csr_trampoline[1] = 0x00008067;  /* ret (jalr x0, x1, 0) */

    asm volatile("fence.i" ::: "memory");

    /* 调用 trampoline，a1 传入 rs1_val（供 CSR_WRITE/SET/CLEAR 使用），
     * a0 接收返回值（供 CSR_READ 使用）。
     * 对于 WRITE/SET/CLEAR，指令的 rd=x0，a0 不被修改，返回值无意义。
     * 对于 READ，指令的 rd=a0，结果直接写入 a0。
     */
    register uintptr_t _a1 asm("a1") = rs1_val;
    register uintptr_t _a0 asm("a0");
    asm volatile(
        "jalr ra, %2"
        : "=r"(_a0)
        : "r"(_a1), "r"(csr_trampoline)
        : "ra", "memory"
    );
    return _a0;
}
```

### 4.6 操作码到特权级映射

```c
/* T-SBI 操作码 → 本框架内部 PRIV 编码 */
static inline unsigned tsbi_op_to_priv(uintptr_t op) {
    switch (op) {
    case TSBI_GOTO_MMODE:  return PRIV_M;    /* 3 */
    case TSBI_GOTO_SMODE:  return PRIV_S;    /* 1 */
    case TSBI_GOTO_UMODE:  return PRIV_U;    /* 0 */
    case TSBI_GOTO_VSMODE: return PRIV_VS;   /* 5 */
    case TSBI_GOTO_VUMODE: return PRIV_VU;   /* 4 */
    default:               return PRIV_M;
    }
}

/* CSR 地址权限判断 — RISC-V 规范 CSR 地址 bits[9:8] = privilege level */
static inline bool is_m_mode_csr(uint32_t csr_addr) {
    unsigned priv = (csr_addr >> 8) & 0x3;
    return (priv == 3);  /* 0x3xx, 0x7xx, 0xBxx, 0xFxx 属于 M-mode */
}
```

### 4.7 CSR_ACCESS 错误处理（策略 A：错误标志位 + recovery）

> **决策**：采用策略 A。CSR_ACCESS 注入执行前 arm 一条 `csr_access_record`；若被注入的 CSR 指令自身触发嵌套 trap（如访问不存在的 CSR、权限不足），M-mode handler 捕获该嵌套异常，置错误标志并借助 `_exec_return_addr` 恢复到 trampoline 之后，最终把错误指示带回调用者，避免嵌套 trap 打断测试流。

```c
typedef volatile struct {
    bool      armed;    /* CSR 注入执行期间置 1 */
    bool      faulted;  /* 注入指令触发了嵌套 trap */
    uintptr_t cause;    /* 嵌套 trap 的 cause */
    uintptr_t recover;  /* trampoline 之后的恢复 PC */
} csr_access_record_t;

extern csr_access_record_t csr_access_record;

uintptr_t csr_access_dispatch(uint32_t encoded_a0, uintptr_t rs1_val) {
    csr_trampoline[0] = encoded_a0;
    csr_trampoline[1] = 0x00008067;  /* ret */
    asm volatile("fence.i" ::: "memory");

    csr_access_record.armed   = true;
    csr_access_record.faulted = false;
    csr_access_record.recover = (uintptr_t)&&csr_done;

    register uintptr_t _a1 asm("a1") = rs1_val;
    register uintptr_t _a0 asm("a0");
    asm volatile("jalr ra, %2" : "=r"(_a0) : "r"(_a1), "r"(csr_trampoline)
                 : "ra", "memory");
csr_done:
    csr_access_record.armed = false;
    if (csr_access_record.faulted)
        return (uintptr_t)-1;   /* 策略 A 约定的错误返回 */
    return _a0;
}
```

在 `m_trap_handler` 的异常路径中 **最优先** 判定 `csr_access_record.armed`：命中则说明是注入指令引发的嵌套 trap → 记录 cause、置 `faulted`、把 xEPC 设为 `recover` 后返回，不进入 `trap_record` 断言路径。

---

## 5. 改造路径（走 B：RVA23 认证兼容）

> **决策基线**：走 **目标 B**。GOTO 操作码 **采用 0x1~0x5**（对齐参考实现，不引入 x3 flag）；CSR_ACCESS 错误处理采用 **策略 A**（见 §4.7）。Phase 1 已完成，Phase 2~4 为走 B 的补齐工作。

### Phase 1 — 寄存器约定 + 基础 GOTO dispatch ✅ 已完成

| 改造项 | 文件 | 状态 |
|--------|------|------|
| 新增 `tsbi.h`（编码/映射/检测宏） | `common/tsbi.h` | ✅ |
| ecall 发起改寄存器约定 | `common/privilege.c`、`common/hyp/hyp_priv.c` | ✅ |
| 汇编入口保存 a0/a1 到 frame | `common/trap_asm.S` | ✅ |
| M/S handler GOTO dispatch（0x1~0x5） | `common/trap.c` | ✅ |
| 删除旧协议 `ecall_args[2]` | 多文件 | ✅ |

### Phase 2 — 核心 T-SBI 服务补齐（M-mode 内） ⬜ 待做

| 改造项 | 文件 | 说明 |
|--------|------|------|
| ECALL_TEST(0x73) | `common/trap.c` | 返回 xEPC 到 frame[a0]，mepc+=4 |
| CSR_ACCESS 注入引擎 | `common/csr_access_proxy.c`（新增） | 动态指令注入 + fence.i + jalr（见 §4.5） |
| `.trampoline` RWX 段 | linker script | 供注入指令执行 |
| 策略 A 错误处理 | `common/trap.c` | `csr_access_record` + recovery（见 §4.7） |
| M-mode dispatch 三分支 | `common/trap.c` | GOTO / ECALL_TEST / CSR_ACCESS |
| 测试侧 wrapper | `common/tsbi.h` | `tsbi_csr_read/write/set/clear`、`tsbi_ecall_test` |

### Phase 3 — HS/VS handler 链式转发 ⬜ 待做

| 改造项 | 文件 | 说明 |
|--------|------|------|
| S/HS handler 对称 dispatch | `common/trap.c` | 本级可处理（S-mode CSR、GOTO_S/U/VS/VU）→ 本地；否则转发 |
| 转发机制 | `common/trap.c` | `ecall_forward_to_m()`：参数不变，ecall 上传 M |
| CSR 权限判断 | `common/tsbi.h` | `is_m_mode_csr()`（0x3xx/0x7xx/0xBxx/0xFxx→M） |
| VS handler | `common/hyp/*` | GOTO_MMODE / M-mode CSR 转发至 HS→M |

### Phase 4 — 委托模型 + 委托/不委托双跑 ⬜ 待做

| 改造项 | 文件 | 说明 |
|--------|------|------|
| medeleg/mideleg 可配置 | `common/test_framework.c` | 由固定 0 改为按用例配置；**ecall-from-S 恒不委托** |
| M-mode handler 四步职责 | `common/trap.c` | ①T-SBI 判定 ②invisible trap ③undelegated trap-to-M 签名 ④填 sstatus/sepc/scause/stval 委托回 S |
| 委托双跑框架支持 | 测试框架 | 每个 cause 支持委托/不委托两种模式运行 |
| SBI 代理 M-mode CSR 覆盖 | 测试用例 | mstatus(TSR/TW/TVM)、mcounteren、mip、mie、menvcfg、mseccfg、mstateen 等 |

---

## 6. 验证策略

### 沙箱验证（先在独立目录跑通）

1. 新建 `_tsbi_sandbox/`，复制 `common/` + `Sv39x4_Sv39/`
2. 在副本上完成 Phase 1-3 改造
3. QEMU 验证：结果与基线 `Sv39x4_Sv39`（Total=149 Pass=148 Skip=0 Fail=1）完全一致

### 全量回归（patch 回主框架后）

20 个虚拟化目录：
```
Hypervisor、Sv{39,48,57}x4、
Sv39x4_Sv39、Sv39x4_Sv48、Sv39x4_Sv57、
Sv48x4_Sv39、Sv48x4_Sv48、Sv48x4_Sv57、
Sv57x4_Sv39、Sv57x4_Sv48、Sv57x4_Sv57、
Sha、Shgatpa、Shtvala、Shvsatpa、Shvstvala、Shvstvecd、Shlcofideleg、
aia_vs、aia_ipi_iommu
```

### 验收标准
- Pass/Skip/Fail 与改造前完全一致
- TS-HGATP-01 fail=1 为已知 QEMU WARL 差异，允许保留
- 无新增 Skip（否则说明 dispatch 时序有差异）

---

## 7. 已决策事项

1. ✅ **目标定位**：走 **目标 B**（RVA23 认证兼容）。
2. ✅ **GOTO 编码**：采用 **0x1~0x5**（对齐 ACT4 参考实现 rvtest_trap_handler.h）；**不引入 x3==0 legacy flag**，dispatch 纯由 a0 范围（1~5）判定；a0=0 非法走 armed-trap。
3. ✅ **S/HS/VS handler**：需实现对称 T-SBI dispatch + 链式转发（Phase 3）。
4. ✅ **CSR_ACCESS 错误处理**：采用 **策略 A**（错误标志位 + recovery，见 §4.7）。
5. ✅ **委托模型**：medeleg/mideleg 改为可配置，支持委托/不委托双跑；**ecall-from-S 恒不委托**（Phase 4）。
6. ✅ **CSRxI 变体**：a0 直接是完整指令编码，CSRRWI/SI/CI 自动支持，无需额外分支。
7. ⬜ **向后兼容**：旧 `ecall_args` 协议已移除，不保留双协议。
