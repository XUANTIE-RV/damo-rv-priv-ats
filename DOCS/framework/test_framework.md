**中文 | [English](../framework_en/test_framework_en.md)**

## 测试公共框架开发者指南

### 概述

本文档描述 RISC-V 特权态测试框架的**公共基础设施**，即 `common/` 目录下与扩展无关的核心组件。所有扩展（PMP、VM、Hypervisor、ZPM 等）均构建在此公共框架之上。

**核心特性：**

- 裸机运行，不依赖任何操作系统
- 支持 **RV32 / RV64** 双架构
- 支持 **M-mode / S-mode / U-mode** 三态切换
- 异常驱动测试模型：通过 trap arming 机制安全地测试访问控制
- `TEST_REGISTER` 宏自动注册测试用例，链接时自动收集
- 确定性 trap 处理：所有内存操作使用非压缩指令（`.option norvc`），保证 `mepc += 4` 可靠跳过故障指令
- 条件编译的公共库：扩展按需链接 PMP / VM / HYP 等模块

**本文档范围：** 仅覆盖公共测试框架（`test_framework.h/c`、`privilege.c`、`trap.c`、`mem_ops.h` 等），不包含 PMP、VM、Hypervisor 等扩展特定框架。扩展框架请参阅对应文档：

| 扩展框架 | 文档 |
|---------|------|
| PMP | [`pmp_test_framework.md`](pmp_test_framework.md) |
| Smepmp | [`smepmp_test_framework.md`](smepmp_test_framework.md) |
| SPMP | [`spmp_test_framework.md`](spmp_test_framework.md) |
| 虚拟内存 | [`vm_test_framework.md`](vm_test_framework.md) |
| Hypervisor | [`hypervisor_framework.md`](hypervisor_framework.md) |
| ZPM | [`zpm_framework.md`](zpm_framework.md) |

---

### 架构设计

#### 文件组成
```
common/
├── test_framework.h      # 测试框架核心头文件（宏、类型、API 声明）
├── test_framework.c      # 测试结果跟踪、reset_state、test_print_summary
├── trap.c                # Trap 处理逻辑（arm/disarm、cause 记录）
├── trap_asm.S            # Trap 入口汇编（上下文保存/恢复）
├── privilege.c           # 特权级切换（goto_priv、run_in_priv）
├── entry.S               # M-mode 启动引导、BSS 初始化、_platform_init 钩子
├── platform_init.S       # 默认 _platform_init 弱符号实现
├── mem_ops.h             # 内存加载/存储/执行/AMO 原语（norvc）
├── csr_accessors.c       # 按索引动态读写 CSR
├── types.h               # 基础类型定义（uintptr_t、bool 等）
├── encoding.h            # CSR 编号、异常原因码、mstatus 位域常量
├── uart.h / uart.c       # UART 驱动（printf 实现）
├── Makefile.common       # 共享构建规则、工具链配置
└── config/               # 平台配置目录
    ├── qemu-rv64-max/    # QEMU virt 机器（rv64,max CPU）
    ├── sail-rv64-max/    # Sail 模拟器
    ├── spike-rv64-max/   # Spike 模拟器
    └── haps_*/           # FPGA/HAPS 平台
```
#### 执行流程

```
_entry (entry.S)
  │
  ├── BSS 清零
  ├── 设置栈指针
  ├── 调用 _platform_init（弱符号钩子）
  ├── 设置 mtvec → m_trap_entry
  │
  └── 跳转到 main()
        │
        ├── 打印测试 banner
        ├── 遍历 .test_table 段中的函数指针
        │     ├── test_fn_1() → TEST_BEGIN → 测试逻辑 → TEST_END
        │     ├── test_fn_2() → ...
        │     └── ...
        └── test_print_summary() → 打印汇总 → wfi 停机
```

#### 核心设计原则

1. **common/ 与扩展之间零耦合** — `common/` 不 `#include` 任何扩展头文件，扩展通过弱符号和链接时组合注入行为
2. **每个扩展编译为独立裸机二进制** — 完全自包含的 ELF，可直接被 QEMU / Sail / Spike 加载
3. **通过 `-include` 实现平台抽象** — 平台头文件在编译时通过 GCC `-include` 标志注入
4. **确定性 Trap 处理** — 非压缩指令保证 trap handler 可靠跳过故障指令

---

### 测试生命周期

| 宏 | 说明 |
|----|------|
| `TEST_REGISTER(fn)` | 注册测试函数（通过 `.test_table` 链接段自动收集） |
| `TEST_BEGIN(name)` | 开始测试：打印名称、重置当前测试状态 |
| `TEST_END()` | 结束测试：恢复 M-mode、重置状态、打印 PASS/FAIL、返回结果 |
| `TEST_SKIP(reason)` | 跳过测试：打印原因、返回成功（跳过不是失败） |
| `TEST_FATAL(reason)` | 终止测试：不可达路径、打印原因，显式失败 |

#### TEST_REGISTER — 注册测试函数

```c
#define TEST_REGISTER(test_fn) \
    bool test_fn(void); \
    static test_func_t test_fn##_ptr \
        __attribute__((section(".test_table"), used)) = test_fn
```

将测试函数指针放入 `.test_table` 链接段，由链接脚本收集。`main.c` 中遍历该段自动执行所有注册的测试。

**用法：**

```c
TEST_REGISTER(test_my_feature);
bool test_my_feature(void) {
    TEST_BEGIN("My feature description");
    // ... 测试逻辑 ...
    TEST_END();
}
```

#### TEST_BEGIN — 开始测试

```c
#define TEST_BEGIN(name) do { \
    test_results.current_test_name = (name); \
    test_results.current_test_failed = false; \
    printf("[TEST] %s\n", (name)); \
} while (0)
```

初始化当前测试状态，打印测试名称。每个测试函数的第一条语句。

#### TEST_END — 结束测试

```c
#define TEST_END() do { \
    goto_priv(PRIV_M); \
    reset_state(); \
    if (test_results.current_test_failed) { ... printf("[FAIL]"); } \
    else { ... printf("[PASS]"); } \
    return !test_results.current_test_failed; \
} while (0)
```

自动恢复 M-mode、重置硬件状态、统计结果、打印 PASS/FAIL 并返回。

> **重要**：`TEST_END()` 内部包含 `return` 语句，不要在其后编写任何代码。

#### TEST_SKIP — 跳过测试

```c
#define TEST_SKIP(reason)
```

跳过当前测试（如硬件不支持某特性），打印 `[SKIP]` 并返回 true（跳过不是失败）。

**用法：**

```c
bool test_svnapot(void) {
    TEST_BEGIN("Svnapot 64KB page");
    if (!detect_svnapot()) {
        TEST_SKIP("Svnapot not supported");
    }
    // ...
    TEST_END();
}
```

#### TEST_FATAL — 无条件失败

```c
#define TEST_FATAL(reason)
```

无条件标记当前测试为失败。用于不应到达的代码路径，或需要显式标记失败的场景。

```c
/* 不应到达此处 — 如果 trap 未触发则失败 */
if (!trap_was_triggered()) {
    TEST_FATAL("expected trap but none occurred");
}

/* switch 中的非法值 */
default:
    TEST_FATAL("unexpected privilege level");
    break;
```

---

### 断言宏

所有断言宏仅可在 **M-mode** 中使用（因为需要访问 UART 输出）。在 S/U-mode 中测试时，请使用 `PRIV_DO_*` + `CHECK_*` 模式。

#### 断言宏行为汇总

| 宏 | 失败输出格式 | 适用场景 |
|----|-------------|---------|
| `TEST_ASSERT(msg, cond)` | `ASSERT FAIL: msg (file:line)` | 通用布尔条件 |
| `TEST_ASSERT_EQ(msg, actual, expected)` | `got 0xA, expected 0xB` | 值相等比较 |
| `TEST_ASSERT_NEQ(msg, actual, not_expected)` | `got 0xA, should NOT equal 0xA` | 值已变更验证 |
| `TEST_ASSERT_BITS(msg, value, mask, expected)` | `value=0xF, mask=0x3, got field 0x1, expected field 0x3` | CSR 位域检查 |


#### TEST_ASSERT — 布尔条件断言

```c
#define TEST_ASSERT(msg, cond)
```

检查布尔条件。失败时打印消息和源文件位置。

```c
TEST_ASSERT("value is non-zero", result != 0);
TEST_ASSERT("bit 3 is set", (csr_val >> 3) & 1);
```

#### TEST_ASSERT_EQ — 相等断言

```c
#define TEST_ASSERT_EQ(msg, actual, expected)
```

检查两个值相等。失败时打印实际值和期望值（十六进制）。

```c
TEST_ASSERT_EQ("trap cause", trap_get_cause(), CAUSE_LOAD_ACCESS);
TEST_ASSERT_EQ("return value", result, 0x42);
```

#### TEST_ASSERT_NEQ — 不等断言

```c
#define TEST_ASSERT_NEQ(msg, actual, not_expected)
```

检查两个值不相等。适用于验证状态已变更的场景。失败时打印 "got X, should NOT equal X"。

```c
/* 验证 RLB 写入后 mseccfg 发生了变化 */
uintptr_t before = CSRR(mseccfg);
CSRS(mseccfg, MSECCFG_RLB);
uintptr_t after = CSRR(mseccfg);
TEST_ASSERT_NEQ("mseccfg changed after RLB set", after, before);
```

#### TEST_ASSERT_BITS — 位域断言

```c
#define TEST_ASSERT_BITS(msg, value, mask, expected)
```

提取 `(value & mask)` 并与 `(expected & mask)` 比较。适用于验证 CSR 的特定位域。失败时打印完整值、掩码和提取的位域。

```c
/* 验证 mseccfg.MML 位 (bit 0) 已设置 */
uintptr_t mseccfg = CSRR(mseccfg);
TEST_ASSERT_BITS("mseccfg.MML is set", mseccfg, 0x1, 0x1);

/* 验证 hstatus.SPV (bit 7) 为 0 */
uintptr_t hstatus = CSRR(hstatus);
TEST_ASSERT_BITS("hstatus.SPV cleared", hstatus, (1UL << 7), 0);

/* 验证 mstatus.MPP 字段 [12:11] 为 S-mode (01) */
uintptr_t mstatus = CSRR(mstatus);
TEST_ASSERT_BITS("MPP is S-mode", mstatus, (3UL << 11), (1UL << 11));
```


---

### 异常测试宏（M-mode）

以下宏用于在 **M-mode** 中测试异常行为。它们内部调用 `trap_expect_begin()` / `trap_expect_end()` 完成 trap 的 arm/disarm。

| 宏 | 说明 |
|----|------|
| `EXPECT_NO_TRAP(stmt)` | 执行语句，断言不触发异常 |
| `EXPECT_TRAP(cause, stmt)` | 执行语句，断言触发指定原因的异常 |
| `EXPECT_EXEC_NO_TRAP(addr)` | 跳转执行，断言不触发异常 |
| `EXPECT_EXEC_TRAP(cause, addr)` | 跳转执行，断言触发指定原因的异常 |

#### EXPECT_NO_TRAP — 期望无异常

```c
#define EXPECT_NO_TRAP(stmt)
```

执行语句并断言不触发异常。

```c
EXPECT_NO_TRAP(mem_load32(valid_addr));
EXPECT_NO_TRAP(CSRR(mstatus));
```

#### EXPECT_TRAP — 期望指定异常

```c
#define EXPECT_TRAP(expected_cause, stmt)
```

执行语句并断言触发指定原因的异常。

```c
EXPECT_TRAP(CAUSE_STORE_ACCESS, mem_store32(readonly_addr, 0));
EXPECT_TRAP(CAUSE_ILLEGAL_INST, CSRW(mstatus, 0));  /* from S-mode */
```

#### EXPECT_EXEC_TRAP — 期望执行异常

```c
#define EXPECT_EXEC_TRAP(expected_cause, addr)
```

跳转到地址执行并断言触发指定异常。使用 `exec_at()` 实现，支持 trap recovery。

```c
EXPECT_EXEC_TRAP(CAUSE_INST_ACCESS, no_exec_region);
```

#### EXPECT_EXEC_NO_TRAP — 期望执行无异常

```c
#define EXPECT_EXEC_NO_TRAP(addr)
```

跳转到地址执行并断言不触发异常。

```c
EXPECT_EXEC_NO_TRAP(executable_region);
```

#### M-mode Trap 宏（Smdbltrp 支持）

在支持 Smdbltrp（Double Trap）扩展的平台上，M-mode 下触发同步异常前必须清除 `mstatus.MDT` 位，否则会触发致命的双重陷阱。框架提供以下 M-mode 专用变体：

| 宏 | 说明 |
|----|------|
| `M_TRAP_EXPECT_BEGIN()` | 清除 MDT 后调用 `trap_expect_begin()` |
| `M_EXPECT_TRAP(cause, stmt)` | 清除 MDT 后执行 `EXPECT_TRAP` |
| `M_EXPECT_NO_TRAP(stmt)` | 清除 MDT 后执行 `EXPECT_NO_TRAP` |

**使用场景：** 在 M-mode 下探测 CSR 是否存在、测试 PMP Lock 位等需要触发预期异常的场景。

```c
/* 探测 CSR 是否存在 */
M_TRAP_EXPECT_BEGIN();
CSRR(menvcfg);  /* 如果不存在会触发 Illegal Instruction */
if (trap_was_triggered()) {
    /* CSR 不存在 */
}
trap_expect_end();
```

> **注意：** 这些宏仅在定义了 `PLATFORM_DOUBLE_TRAP` 时才会清除 MDT，否则为 no-op。

#### Trap 查询 API

在 trap 触发后可查询详细信息：

| 函数 | 返回值 | 说明 |
|------|--------|------|
| `trap_was_triggered()` | `bool` | 最近一次 arm 期间是否触发了 trap |
| `trap_get_cause()` | `uintptr_t` | 异常原因码（mcause） |
| `trap_get_epc()` | `uintptr_t` | 异常 PC（mepc） |
| `trap_get_tval()` | `uintptr_t` | 异常附加值（mtval） |
| `trap_get_htval()` | `uintptr_t` | Hypervisor: htval（GPA>>2，需 ENABLE_HYP） |
| `trap_get_htinst()` | `uintptr_t` | Hypervisor: htinst（转换指令，需 ENABLE_HYP） |
| `trap_get_gva()` | `bool` | Hypervisor: hstatus.GVA 位（需 ENABLE_HYP） |

---

### S/U-mode 安全宏

#### 设计背景

在 RISC-V 中，UART 是 M-mode 通过 PMP 保护的 MMIO 设备。当测试代码运行在 S-mode 或 U-mode 时，**无法直接访问 UART**，也就无法调用 `printf`。因此，`EXPECT_TRAP()` / `EXPECT_NO_TRAP()` 这类内含 `TEST_ASSERT`（→ `printf`）的宏不能在 S/U-mode 中使用。

框架通过 **PRIV_DO + CHECK 两阶段模式** 解决此问题：

1. **阶段一（S/U-mode）**：使用 `PRIV_DO` / `PRIV_DO_EXEC` 执行操作——仅做"arm trap → 执行语句 → disarm trap"，不进行任何打印或断言
2. **阶段二（M-mode）**：返回 M-mode 后，使用 `CHECK_NO_TRAP` / `CHECK_TRAP` 检查 trap 记录并打印断言结果

#### PRIV_DO 系列汇总

| 宏 | 用途 | 执行环境 | 配合使用 |
|----|------|----------|---------|
| `PRIV_DO(stmt)` | 执行 load/store/CSR 操作 | S/U-mode | `CHECK_NO_TRAP` 或 `CHECK_TRAP` |
| `PRIV_DO_EXEC(addr)` | 测试指令执行权限 | S/U-mode | `CHECK_NO_TRAP` 或 `CHECK_TRAP` |
| `CHECK_NO_TRAP(msg)` | 断言无异常发生 | M-mode | — |
| `CHECK_TRAP(msg, cause)` | 断言触发指定异常 | M-mode | — |

> **兼容性**：旧宏名 `PRIV_DO_NO_TRAP` / `PRIV_DO_TRAP` / `PRIV_DO_EXEC_NO_TRAP` / `PRIV_DO_EXEC_TRAP` 仍可使用（作为别名定义），但新代码推荐统一使用 `PRIV_DO` / `PRIV_DO_EXEC`。旧名称被保留是为了避免破坏已有的大量测试代码。

#### 使用模式

```c
/* 标准两阶段测试模式 */
goto_priv(PRIV_S);
PRIV_DO(mem_load32(addr));           /* 阶段一：S-mode 下执行 load */
PRIV_DO(mem_store32(addr, 0));       /* 阶段一：S-mode 下执行 store */
goto_priv(PRIV_M);
CHECK_NO_TRAP("read should succeed");           /* 阶段二：M-mode 下断言 */
CHECK_TRAP("write should fault", CAUSE_SAF);    /* 阶段二：M-mode 下断言 */
```

**设计意图**：在 S/U-mode 中安全地执行一条可能触发 trap 的操作。宏本身不对 trap 是否发生做任何判断——它只是保护性地记录 trap 状态，让后续的 `CHECK_*` 宏来决定期望。

**使用场景**：

```c
/* 测试 load 权限 */
goto_priv(PRIV_S);
PRIV_DO(mem_load8(protected_addr));
goto_priv(PRIV_M);
CHECK_NO_TRAP("S-mode should be able to read");

/* 测试 store 被拒绝 */
goto_priv(PRIV_U);
PRIV_DO(mem_store32(readonly_addr, 0xBAD));
goto_priv(PRIV_M);
CHECK_TRAP("U-mode write denied", CAUSE_STORE_ACCESS_FAULT);

/* 测试 AMO 操作 */
goto_priv(PRIV_S);
PRIV_DO(mem_amo_swap_w(addr, 0x42));
goto_priv(PRIV_M);
CHECK_NO_TRAP("AMO should succeed");
```

**设计意图**：在 S/U-mode 中测试对特定地址的**指令执行权限**。使用 `exec_at()` 跳转到目标地址（目标地址应包含 `nop; ret` 序列），trap handler 通过 `_exec_return_addr` 恢复。

**使用场景**：

```c
/* 测试代码区可执行 */
goto_priv(PRIV_S);
PRIV_DO_EXEC(code_region);
goto_priv(PRIV_M);
CHECK_NO_TRAP("code region is executable");

/* 测试数据区不可执行 */
goto_priv(PRIV_S);
PRIV_DO_EXEC(data_region);
goto_priv(PRIV_M);
CHECK_TRAP("data region not executable", CAUSE_INST_ACCESS_FAULT);
```

#### CHECK_NO_TRAP — 断言无异常

```c
#define CHECK_NO_TRAP(msg) do { \
    TEST_ASSERT(msg, !trap_was_triggered()); \
} while (0)
```

**设计意图**：在 M-mode 中验证上一次 `PRIV_DO` / `PRIV_DO_EXEC` 操作未触发任何 trap。内部调用 `trap_was_triggered()` 检查 trap 记录，并通过 `TEST_ASSERT` 打印结果。

**注意事项**：
- 必须在 `goto_priv(PRIV_M)` 之后调用
- 每次 `CHECK_*` 调用消耗一次 trap 记录，按 PRIV_DO 的调用顺序一一对应

#### CHECK_TRAP — 断言指定异常

```c
#define CHECK_TRAP(msg, expected_cause) do { \
    TEST_ASSERT(msg ": trap triggered", trap_was_triggered()); \
    if (trap_was_triggered()) { \
        TEST_ASSERT_EQ(msg ": cause", trap_get_cause(), (uintptr_t)(expected_cause)); \
    } \
} while (0)
```

**设计意图**：在 M-mode 中验证上一次 `PRIV_DO` / `PRIV_DO_EXEC` 操作触发了特定原因的 trap。先断言 trap 确实发生，再断言 cause 码匹配。

**使用场景**：

```c
/* PMP 拒绝读取 → load access fault */
CHECK_TRAP("PMP blocks read", CAUSE_LOAD_ACCESS_FAULT);

/* 页表缺少写权限 → store page fault */
CHECK_TRAP("PTE denies write", CAUSE_STORE_PAGE_FAULT);

/* 执行非 X 区域 → instruction access fault */
CHECK_TRAP("exec denied", CAUSE_INST_ACCESS_FAULT);
```



---

### 特权级切换 API

#### 特权级常量

| 常量 | 值 | 含义 |
|------|----|------|
| `PRIV_U` | 0 | User mode |
| `PRIV_S` | 1 | Supervisor mode |
| `PRIV_M` | 3 | Machine mode |
| `PRIV_VU` | 4 | Virtual User mode（Hypervisor 扩展） |
| `PRIV_VS` | 5 | Virtual Supervisor mode（Hypervisor 扩展） |

#### goto_priv — 切换特权级

```c
void goto_priv(unsigned target);
```

双向切换到任意特权级：
- **向下**（M→S、M→U、S→U）：通过 `mret` / `sret` 实现
- **向上**（U→S、U→M、S→M）：通过 `ecall` 由 trap handler 处理

```c
goto_priv(PRIV_S);   /* 切换到 S-mode */
goto_priv(PRIV_U);   /* 切换到 U-mode */
goto_priv(PRIV_M);   /* 切换回 M-mode */
```

#### get_current_priv — 获取当前特权级

```c
unsigned get_current_priv(void);
```

返回当前跟踪的特权级（由框架内部维护）。

#### run_in_priv — 在指定特权级执行函数

```c
uintptr_t run_in_priv(unsigned priv, uintptr_t (*fn)(uintptr_t), uintptr_t arg);
```

切换到目标特权级，执行 `fn(arg)`，然后自动返回原特权级。返回函数的返回值。

```c
static uintptr_t read_csr_in_smode(uintptr_t unused) {
    return CSRR(sstatus);
}
uintptr_t val = run_in_priv(PRIV_S, read_csr_in_smode, 0);
```

> **注意**：`run_in_priv` 使用全局变量传递参数，不可重入。

---

### 状态管理

#### reset_state — 重置硬件状态

```c
void reset_state(void);
```

在每个 `TEST_END()` 中自动调用，将硬件恢复到干净基线：

1. 确保处于 M-mode
2. 重置 trap arm 状态
3. 重写 `mtvec` / `stvec`
4. 清零 `medeleg` / `mideleg`（M-mode handler 接管所有异常）
5. 禁用中断（清除 MIE/SIE）
6. 清除 MPRV、MXR 位
7. 清理 Pointer Masking 状态（trap-protected）

> 扩展特定的 reset（如 `pmp_clear_all()`）由各扩展自行在 `TEST_END` 前或 setup/teardown 中调用。

#### test_print_summary — 打印测试汇总

```c
void test_print_summary(void);
```

在所有测试执行完毕后调用，打印结果汇总：

```
========================================
  Test Summary
========================================
  Total:   10
  Passed:  9
  Failed:  1
  Skipped: 0
----------------------------------------
  Assertions: 45 total, 44 passed, 1 failed
========================================
  RESULT: 1 FAILURES
----------------------------------------
  Failed tests:
    1) PMP TOR write protection
========================================
```

---

### 内存操作原语

`mem_ops.h` 提供了一套确定性的内存操作内联函数，所有操作使用 `.option norvc`（非压缩指令），保证每条指令恰好 4 字节，使 trap handler 能可靠地通过 `mepc += 4` 跳过故障指令。

#### Load 操作

| 函数 | 说明 |
|------|------|
| `mem_load8(addr)` | 8-bit 加载（lb） |
| `mem_load16(addr)` | 16-bit 加载（lh） |
| `mem_load32(addr)` | 32-bit 加载（lw） |
| `mem_load64(addr)` | 64-bit 加载（ld，仅 RV64） |

#### Store 操作

| 函数 | 说明 |
|------|------|
| `mem_store8(addr, val)` | 8-bit 存储（sb） |
| `mem_store16(addr, val)` | 16-bit 存储（sh） |
| `mem_store32(addr, val)` | 32-bit 存储（sw） |
| `mem_store64(addr, val)` | 64-bit 存储（sd，仅 RV64） |

#### 执行操作

```c
void exec_at(uintptr_t addr);
```

跳转到 `addr` 执行指令。目标地址应包含 `nop; ret` 序列（由 `entry.S` 填充）。跳转前保存 recovery 地址到 `_exec_return_addr`，trap handler 可据此恢复。

#### AMO 操作

| 函数 | 说明 |
|------|------|
| `mem_amo_swap_w(addr, val)` | 原子交换（amoswap.w） |
| `mem_lr_w(addr)` | Load-Reserved（lr.w） |
| `mem_sc_w(addr, val)` | Store-Conditional（sc.w） |

---

### 日志系统

框架提供分级日志宏，编译时通过 `LOG_LEVEL` 控制输出粒度：

| 宏 | 级别 | 用途 |
|----|------|------|
| `LOG_E(fmt, ...)` | 1 - Error | 错误信息 |
| `LOG_W(fmt, ...)` | 2 - Warn | 警告信息 |
| `LOG_I(fmt, ...)` | 3 - Info | 一般信息（默认可见） |
| `LOG_D(fmt, ...)` | 4 - Debug | 调试信息 |
| `LOG_T(fmt, ...)` | 5 - Trace | 追踪信息 |
| `LOG_V(fmt, ...)` | 6 - Verbose | 详细信息 |

**编译时设置：**

```bash
make pmp LOG_LEVEL=5    # 启用到 Trace 级别
make pmp LOG_LEVEL=1    # 仅显示错误
```

---

### 构建系统集成

#### Makefile.common

每个扩展的 `Makefile` 通过 `include ../common/Makefile.common` 引入公共构建规则。

**典型扩展 Makefile：**

```makefile
TARGET   = my_ext_test.elf

# 按需启用公共库
# ENABLE_PMP = 1    # 链接 common/pmp/ 公共库
# ENABLE_VM  = 1    # 链接 common/vm/ 公共库
# ENABLE_HYP = 1    # 链接 common/hyp/ 公共库

EXT_OBJS = main.o tests/test_register.o

include ../common/Makefile.common
```

#### 条件编译开关

| 变量 | 效果 |
|------|------|
| `ENABLE_PMP=1` | 链接 `common/pmp/`（PMP 配置 API） |
| `ENABLE_VM=1` | 链接 `common/vm/`（页表管理 API） |
| `ENABLE_HYP=1` | 链接 `common/hyp/`（Hypervisor 支持） |
| `ENABLE_PM=1` | 链接 `common/pm/`（Pointer Masking 支持） |
| `ENABLE_TWO_STAGE=1` | 启用两阶段翻译支持（Hypervisor G-stage） |

#### 链接脚本

基础链接脚本位于各平台配置目录 `common/config/<platform>/link.ld`，定义了通用的段布局（`.text.init`、`.data`、`.bss` 等）。各扩展目录下的 `kernel.ld` 通过 `INCLUDE` 指令继承基础脚本，并添加扩展特定的段（如 `.test_table`、`.pmp_test_region`、`.page_tables` 等）。

**典型扩展 kernel.ld 结构：**

```ld
INCLUDE "link.ld"

/* 扩展特定段 */
.test_table : { ... }
.pmp_test_region : { ... }
```

**关键段说明：**

| 段 | 说明 |
|----|------|
| `.text.init` | 启动代码（entry.S） |
| `.text` | 代码段 |
| `.rodata` | 只读数据 |
| `.test_table` | 测试函数指针数组（`TEST_REGISTER` 产生） |
| `.data` | 可写数据 |
| `.bss` | 零初始化数据（由 entry.S 清零） |
| `.stack` | 栈空间（默认 128KB） |

**`.test_table` 段的链接符号：**

```ld
.test_table : {
    . = ALIGN(8);
    _test_table = .;
    *(.test_table)
    _test_table_end = .;
}
_test_table_size = (_test_table_end - _test_table) / (__riscv_xlen / 8);
```

`main.c` 中通过 `extern test_func_t _test_table[]` 和 `extern test_func_t _test_table_end[]` 遍历所有注册的测试。

#### 构建变量

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `XLEN` | `64` | 目标架构位宽：`32` 或 `64` |
| `CONFIG` | `qemu-rv64-max` | 平台配置（可选：`qemu-rv64-max`、`sail-rv64-max`、`spike-rv64-max`等） |
| `LOG_LEVEL` | `3` | 日志级别：0-6 |
| `MEM_BASE` | 平台默认 | 内存基地址 |
| `CROSS_COMPILER` | 平台默认 | 交叉编译器前缀 |

---

### 编写测试用例

#### 完整示例

```c
#include "test_framework.h"

/* 声明外部符号（由链接脚本定义） */
extern uintptr_t __test_data;

/* 注册测试 */
TEST_REGISTER(test_basic_access);

bool test_basic_access(void) {
    TEST_BEGIN("Basic memory access in S-mode");

    uintptr_t data_addr = (uintptr_t)&__test_data;

    /* 1. 在 M-mode 配置硬件状态 */
    // ... 扩展特定配置 ...

    /* 2. M-mode 直接测试（可用 EXPECT_* 宏） */
    EXPECT_NO_TRAP(mem_load32(data_addr));

    /* 3. 切换到 S-mode 进行测试 */
    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(mem_load32(data_addr));
    PRIV_DO_TRAP(mem_store32(data_addr, 0xDEAD));
    goto_priv(PRIV_M);

    /* 4. 在 M-mode 检查 S-mode 的测试结果 */
    CHECK_NO_TRAP("S-mode read should succeed");
    CHECK_TRAP("S-mode write should fault", CAUSE_SAF);

    /* 5. 值断言 */
    uintptr_t val = mem_load32(data_addr);
    TEST_ASSERT_EQ("data unchanged", val, 0x12345678);

    /* 6. 结束测试 */
    TEST_END();
}
```

#### 测试函数签名

每个测试函数必须：
- 返回 `bool`（true = 通过，false = 失败）
- 以 `TEST_BEGIN(name)` 开始
- 以 `TEST_END()` 结束
- 在 `TEST_END()` 之前完成所有断言

#### 最佳实践

1. **不要在 S/U-mode 中调用框架 API** — PMP 配置 API 使用 CSR 指令，只能在 M-mode 下调用。先在 M-mode 配置硬件状态，然后再切换特权级。

2. **使用 PRIV_DO + CHECK 模式** — S/U-mode 中无法 printf，必须使用两阶段模式。

3. **TEST_END() 包含 return** — 不要在 `TEST_END()` 之后编写任何代码。

4. **检测硬件能力后 SKIP** — 如果测试依赖可选扩展，先检测再决定是否跳过：

   ```c
   if (!detect_feature()) {
       TEST_SKIP("feature not supported");
   }
   ```

5. **合理使用断言宏** — 根据检查内容选择最具信息量的断言：
   - 值比较 → `TEST_ASSERT_EQ` / `TEST_ASSERT_NEQ`
   - 位域检查 → `TEST_ASSERT_BITS`
   - 复合条件 → `TEST_ASSERT`

6. **EXPECT_* vs PRIV_DO+CHECK** — 在 M-mode 中直接使用 `EXPECT_*`，仅在 S/U-mode 中才需要 `PRIV_DO_*` + `CHECK_*` 两阶段模式。

---

### 常用异常原因码

`encoding.h` 中定义的常用异常原因码常量：

| 常量 | 值 | 含义 | 短别名 |
|------|----|------|--------|
| `CAUSE_INST_ADDR_MISALIGN` | 0 | 取指未对齐 | — |
| `CAUSE_INST_ACCESS_FAULT` | 1 | 取指访问错误 | `CAUSE_IAF` |
| `CAUSE_ILLEGAL_INST` | 2 | 非法指令 | `CAUSE_ILI` |
| `CAUSE_BREAKPOINT` | 3 | 断点 | — |
| `CAUSE_LOAD_ADDR_MISALIGN` | 4 | 加载未对齐 | — |
| `CAUSE_LOAD_ACCESS_FAULT` | 5 | 加载访问错误 | `CAUSE_LAF` |
| `CAUSE_STORE_ADDR_MISALIGN` | 6 | 存储未对齐 | — |
| `CAUSE_STORE_ACCESS_FAULT` | 7 | 存储访问错误 | `CAUSE_SAF` |
| `CAUSE_ECALL_FROM_U` | 8 | U-mode ecall | `CAUSE_ECU` |
| `CAUSE_ECALL_FROM_S` | 9 | S-mode ecall | `CAUSE_ECS` |
| `CAUSE_ECALL_FROM_VS` | 10 | VS-mode ecall | — |
| `CAUSE_ECALL_FROM_M` | 11 | M-mode ecall | `CAUSE_ECM` |
| `CAUSE_INST_PAGE_FAULT` | 12 | 取指页错误 | `CAUSE_IPF` |
| `CAUSE_LOAD_PAGE_FAULT` | 13 | 加载页错误 | `CAUSE_LPF` |
| `CAUSE_STORE_PAGE_FAULT` | 15 | 存储页错误 | `CAUSE_SPF` |
| `CAUSE_SOFTWARE_CHECK` | 18 | 软件检查异常（CFI） | — |
| `CAUSE_HARDWARE_ERROR` | 19 | 硬件错误 | — |
| `CAUSE_INST_GUEST_PAGE_FAULT` | 20 | 取指客户页错误（H 扩展） | — |
| `CAUSE_LOAD_GUEST_PAGE_FAULT` | 21 | 加载客户页错误（H 扩展） | — |
| `CAUSE_VIRTUAL_INSTRUCTION` | 22 | 虚拟指令异常（H 扩展） | — |
| `CAUSE_STORE_GUEST_PAGE_FAULT` | 23 | 存储客户页错误（H 扩展） | — |

---

### 测试结果数据结构

```c
typedef struct {
    unsigned int total;              /* 总断言数 */
    unsigned int passed;             /* 通过的断言数 */
    unsigned int failed;             /* 失败的断言数 */
    unsigned int skipped;            /* 跳过的测试数 */
    unsigned int tests_passed;       /* 通过的测试用例数 */
    unsigned int tests_failed;       /* 失败的测试用例数 */
    const char  *current_test_name;  /* 当前测试名称 */
    bool         current_test_failed;/* 当前测试是否有失败断言 */
    const char  *failed_names[64];   /* 失败测试名称列表 */
    unsigned int failed_count;       /* 已记录的失败测试数 */
} test_result_t;

extern test_result_t test_results;
```

全局实例 `test_results` 在 `test_framework.c` 中定义，所有宏通过它跟踪测试进度。
