**中文 | [English](../framework_en/build_framework_en.md)**

# 构建框架：多目标（Multi-Target）支持

## 概述

构建系统支持两种模式：

1. **单目标模式**（legacy）— 每个扩展目录编译出一个 ELF 文件
2. **多目标模式**（新增）— 同一个扩展目录编译出多个独立的 ELF 文件

两种模式共享完全相同的编译参数、链接脚本和模拟器集成。已有的单目标扩展**无需任何修改**即可继续工作。

---

## 单目标模式（Legacy）

定义 `TARGET` 和 `EXT_OBJS`：

```makefile
TARGET = pmp_test.elf
ENABLE_PMP = 1
EXT_OBJS = pmp_reset.o main.o tests/test_register.o
include ../common/Makefile.common
```

---

## 多目标模式

定义 `TARGETS`（复数）和每个目标对应的 `<base>_OBJS` 变量：

```makefile
ENABLE_PMP = 1

TARGETS = pmp_cvpt.elf pmp_basic.elf pmp_lock.elf

pmp_cvpt_OBJS  = pmp_reset.o main_cvpt.o tests/register_cvpt.o
pmp_basic_OBJS = pmp_reset.o main_basic.o tests/register_basic.o
pmp_lock_OBJS  = pmp_reset.o main_lock.o tests/register_lock.o

include ../common/Makefile.common
```

### 变量命名规则

`_OBJS` 变量名由 ELF 文件名推导而来：
1. 去掉 `.elf` 后缀
2. 将文件名中的 `-` 和 `.` 替换为 `_`

示例：

| 目标文件名            | 对应变量名             |
|----------------------|----------------------|
| `pmp_cvpt.elf`       | `pmp_cvpt_OBJS`     |
| `pmp-basic.elf`      | `pmp_basic_OBJS`    |
| `sv39_walk.test.elf` | `sv39_walk_test_OBJS`|

---

## 目录结构（多目标示例）

以 `pmp` 扩展为例，改造后的目录结构：

```
pmp/
├── Makefile                 # 定义 TARGETS 和每个目标的 _OBJS
├── kernel.ld                # 共用链接脚本（无需修改）
├── pmp_regions.ld           # 共用 PMP 区域定义（无需修改）
├── pmp_reset.c              # 共用初始化代码（所有 ELF 共享）
│
├── main_cvpt.c              # pmp_cvpt.elf 的入口文件
├── main_basic.c             # pmp_basic.elf 的入口文件
├── main_lock.c              # pmp_lock.elf 的入口文件
│
└── tests/
    ├── test_helpers.h       # 共用测试辅助头文件
    │
    ├── register_cvpt.c      # pmp_cvpt.elf 的测试注册文件
    ├── register_basic.c     # pmp_basic.elf 的测试注册文件
    ├── register_lock.c      # pmp_lock.elf 的测试注册文件
    │
    ├── test_cvpt_napot.c    # 测试实现文件（被 register 文件 #include）
    ├── test_cap.c
    ├── test_napot.c
    ├── test_tor.c
    ├── test_lock.c
    └── ...
```

### 编译产物（全部在同一目录下）

```
pmp/
├── pmp_cvpt.elf  / .asm / .sym     # 覆盖点测试
├── pmp_basic.elf / .asm / .sym     # 基础功能测试
├── pmp_lock.elf  / .asm / .sym     # Lock 专项测试
├── pmp_reset.o                      # 编译一次，链接到所有 ELF
├── main_cvpt.o
├── main_basic.o
├── main_lock.o
└── tests/
    ├── register_cvpt.o
    ├── register_basic.o
    └── register_lock.o
```

---

## 测试注册机制与多目标的配合

### 原理

测试注册依赖以下链路：

1. `TEST_REGISTER(fn)` 宏将函数指针放入 `.test_table` section
2. `kernel.ld` 收集所有 `.test_table` 段，定义 `_test_table` / `_test_table_end` 符号
3. `main` 函数遍历 `_test_table[]` 数组执行所有注册的测试

**关键点**：链接器只收集**参与当前链接的 .o 文件**中的 `.test_table` 段。因此不同 ELF 的测试集天然互不干扰，`kernel.ld` 无需任何修改。

### 数据流

```
register_cvpt.c ──编译──► register_cvpt.o ──┐
                                            ├──链接──► pmp_cvpt.elf
main_cvpt.o ────────────────────────────────┘
  └── .test_table 只包含 cvpt 测试的函数指针

register_basic.c ──编译──► register_basic.o ──┐
                                              ├──链接──► pmp_basic.elf
main_basic.o ─────────────────────────────────┘
  └── .test_table 只包含 basic 测试的函数指针
```

### 步骤详解

1. **编写测试实现**（`tests/test_xxx.c`）：每个文件内使用 `TEST_REGISTER(fn)` 声明测试函数。

2. **为每个 ELF 创建注册文件**（如 `tests/register_cvpt.c`）：通过 `#include` 选择性地包含所需的测试文件。

3. **为每个 ELF 创建入口文件**（如 `main_cvpt.c`）：遍历 `_test_table[]` 执行测试，每个 main 文件只需修改 banner 标题即可。

4. **`kernel.ld` 无需修改**：链接脚本按链接参与的 .o 文件自动收集 section 内容。

### 注册文件示例

```c
// tests/register_cvpt.c
#include "test_framework.h"
#include "pmp_cfg.h"

/* 本 ELF 只包含覆盖点相关测试 */
#include "test_cvpt_napot.c"
```

```c
// tests/register_basic.c
#include "test_framework.h"
#include "pmp_cfg.h"

/* 本 ELF 包含基础功能测试 */
#include "test_cap.c"
#include "test_napot.c"
#include "test_tor.c"
#include "test_na4.c"
#include "test_rwx.c"
#include "test_priority.c"
#include "test_mmode.c"
#include "test_multi.c"
#include "test_granularity.c"
```

### 入口文件示例

```c
// main_cvpt.c
#include "test_framework.h"
#include "pmp_cfg.h"

extern void pmp_reset(void);
extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

void main(void) {
    uart_init();
    reset_state();
    pmp_reset();

    printf("\n");
    printf("==============================================\n");
    printf("  PMP Coverpoint NAPOT Tests\n");
    printf("==============================================\n");
    printf("  Platform: %s | XLEN: %d\n", CONFIG_NAME, __riscv_xlen);

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);
    printf("  Test count: %u\n", test_count);
    printf("==============================================\n\n");

    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    test_print_summary();
}
```

---

## 顶层构建系统

根目录 `Makefile` 提供了统一的构建入口，支持按扩展组批量构建和运行。

### 扩展组（Extension Groups）

项目将 60+ 个扩展按功能分为 6 个组：

| 组名 | 包含的扩展 | 构建命令 |
|------|-----------|---------|
| `PMP_GROUP` | pmp, smepmp, spmp, pmp_sv39, pmp_sv48, pmp_sv57 | `make pmp-group` |
| `SV_GROUP` | Sv39, Sv48, Sv57, Svbare, Svade, Svadu, Svnapot, Svinval, Svpbmt, Svvptc, Svrsw60t59b | `make sv-group` |
| `SS_GROUP` | Ssccptr, Sscofpmf, Sscounterenw, Ssstateen, Sstc, Sstvala, Sstvecd, Ssu64xl | `make ss-group` |
| `SM_GROUP` | Smstateen, smrnmi | `make sm-group` |
| `HYP_GROUP` | Sv39x4, Sv48x4, Sv57x4, Sv39x4_Sv39, Sv39x4_Sv48, Sv39x4_Sv57, Sv48x4_Sv48, Sv57x4_Sv57, Hypervisor, Sha, Shcounterenw, Shgatpa, Shlcofideleg, Shtvala, Shvsatpa, Shvstvala, Shvstvecd | `make hyp-group` |
| `INT_GROUP` | aia_aplic, aia_imsic, aia_smaia, aia_ipi_iommu, aia_vs | `make int-group` |

**常用构建命令：**

```bash
make all              # 构建所有扩展
make pmp              # 构建单个扩展
make pmp-group        # 构建 PMP 组所有扩展
make sv-group         # 构建 SV 组所有扩展
make clean            # 清理所有编译产物
```

### 模拟器运行目标

根 Makefile 为每个扩展自动生成三种模拟器的运行目标：

| 命令格式 | 说明 | 示例 |
|---------|------|------|
| `make sail-<ext>` | 在 Sail 模拟器上运行指定扩展 | `make sail-pmp` |
| `make spike-<ext>` | 在 Spike 模拟器上运行指定扩展 | `make spike-sv39` |
| `make qemu-<ext>` | 在 QEMU 上运行指定扩展 | `make qemu-Hypervisor` |

**批量运行：**

```bash
make sail-all         # 在 Sail 上依次运行所有扩展
make spike-all        # 在 Spike 上依次运行所有扩展
make qemu-all         # 在 QEMU 上依次运行所有扩展
```

每个扩展独立运行、顺序执行。如果某个扩展运行失败（返回非零），序列立即停止。

---

## 模拟器目标（扩展内部）

在扩展目录内部，多目标模式下模拟器命令会自动遍历执行所有 ELF：

| 命令            | 行为                                       |
|----------------|-------------------------------------------|
| `make`         | 编译所有 TARGETS                            |
| `make spike`   | 编译后依次在 Spike 上运行每个 ELF            |
| `make sail`    | 编译后依次在 Sail 上运行每个 ELF             |
| `make trace`   | 编译后依次 trace 每个 ELF 并转换为 RVVI 格式 |
| `make clean`   | 清除所有目标的编译产物                        |

每个 ELF 独立运行、顺序执行。如果某个 ELF 运行失败（返回非零），序列立即停止。

---

## 共享目标文件

出现在多个 `_OBJS` 变量中的目标文件（如 `pmp_reset.o`）只会被 Make 编译一次，然后链接到所有引用它的 ELF 中。这是安全的，因为：

- 所有 ELF 共享完全相同的 `CFLAGS` / `MARCH` / `ABI`
- 共享的 .o 文件中不包含 `.test_table` 段（不调用 `TEST_REGISTER`）
- 链接脚本使用 `mcmodel=medany`，位置无关

---

## 迁移指南

将已有的单目标扩展转换为多目标：

1. **重命名** `main.c` → `main_<group>.c`（或保留原文件作为其中一个分组）
2. **重命名** `tests/test_register.c` → `tests/register_<group>.c`
3. **拆分**注册文件为多个，每个 `#include` 对应分组的测试文件
4. **创建**额外的 `main_<group>.c`（复制后修改 banner 标题）
5. **修改** `Makefile`：
   - 将 `TARGET = ...` / `EXT_OBJS = ...` 替换为
   - `TARGETS = ...` 以及每个目标的 `_OBJS` 变量
6. **完成** — 无需修改 `kernel.ld`、`test_framework.h` 或 common 目录下的任何代码

---

## 设计决策

| 决策点               | 选择                                        | 原因                                 |
|---------------------|---------------------------------------------|--------------------------------------|
| 向后兼容             | `ifdef TARGETS` 条件分支                     | 已有扩展零改动即可继续使用              |
| 输出位置             | 所有 ELF 平铺在扩展目录下                     | 满足"保留在同一文件夹"需求             |
| 编译参数             | 所有 ELF 继承相同 CFLAGS/LDFLAGS/MARCH       | 一致性保证，避免 ABI 不匹配            |
| 链接脚本             | 共用同一个 kernel.ld                         | `.test_table` 收集天然按链接对象范围    |
| 规则生成方式          | `$(eval $(call ...))` 模板展开               | GNU Make 标准做法，可维护性好           |
| 模拟器遍历           | shell for 循环 + `|| exit 1`                | 简单可靠，失败即停                     |
