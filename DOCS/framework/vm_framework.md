**中文 | [English](../framework_en/vm_framework_en.md)**

# RISC-V 虚拟内存测试框架

本文档描述虚拟内存（Sv39/Sv48/Sv57）测试框架的架构、API 和使用方法。

---

## 概述

VM 测试框架提供了一套统一的接口，用于在 RISC-V 平台上验证 Sv39（3 级页表）、Sv48（4 级页表）和 Sv57（5 级页表）虚拟内存配置的正确性。框架支持：

- **恒等映射**（VA = PA），基地址可配置
- **三种页大小**：4KB、2MB（megapage）、1GB（gigapage）
- **S-mode 执行**：在启用虚拟内存的 S-mode 下运行测试函数
- **自动 PMP 配置**：自动设置 PMP 规则以允许 S-mode 访问
- **自动 trap 委托**：自动将 page fault 委托给 S-mode 处理

---

## 文件结构

```
common/vm/
├── vm_defs.h       # 常量定义：satp MODE、PTE 位域、页大小、宏
├── vm.h            # API 声明和 pt_context_t 结构体
├── page_table.c    # 页表池分配器、页表映射、恒等映射
└── satp.c          # satp 控制、sfence.vma、PMP 辅助、vm_run_in_smode

Sv39/
├── Makefile        # ENABLE_VM=1, TARGET=sv39_test.elf
├── kernel.ld       # 链接脚本（含 .page_tables 和 .smode_stack 段）
└── main.c          # Sv39 测试用例（1GB/2MB/4KB 页恒等映射验证）

Sv48/                # 与 Sv39 结构相同，测试 Sv48 模式
├── Makefile
├── kernel.ld
└── main.c

Sv57/                # 与 Sv39 结构相同，测试 Sv57 模式
├── Makefile
├── kernel.ld
└── main.c
```

---

## 核心数据结构

### pt_context_t

页表上下文，持有管理一组页表所需的全部状态。每个上下文代表一个地址空间配置。

```c
typedef struct {
    int         mode;       /* SATP_MODE_SV39 / SV48 / SV57          */
    uintptr_t  *root_pt;   /* 根页表物理地址                          */
    int         levels;     /* 页表级数（3/4/5）                       */
    uintptr_t   map_base;  /* 恒等映射基地址                          */
    uintptr_t   map_size;  /* 恒等映射大小                            */
    int         map_level;  /* 恒等映射使用的页级别                     */
} pt_context_t;
```

---

## API 参考

### 页表管理

#### pt_init

初始化页表上下文，从页表池分配根页表。

```c
void pt_init(pt_context_t *ctx, int mode);
```

- **ctx**: 待初始化的页表上下文
- **mode**: satp 模式，可选值：
  - `SATP_MODE_SV39` (8) — 3 级页表，39 位虚拟地址
  - `SATP_MODE_SV48` (9) — 4 级页表，48 位虚拟地址
  - `SATP_MODE_SV57` (10) — 5 级页表，57 位虚拟地址

#### pt_map_page

映射单个页面（4KB / 2MB / 1GB）。自动创建中间页表页。

```c
int pt_map_page(pt_context_t *ctx, uintptr_t va, uintptr_t pa,
                uintptr_t flags, int level);
```

- **ctx**: 页表上下文
- **va**: 虚拟地址（必须按目标页大小对齐）
- **pa**: 物理地址（必须按目标页大小对齐）
- **flags**: PTE 标志位组合（`PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D` 等）
- **level**: 页级别
  - `PT_LEVEL_4K` (0) — 4KB 页
  - `PT_LEVEL_2M` (1) — 2MB megapage
  - `PT_LEVEL_1G` (2) — 1GB gigapage
- **返回值**: 0 成功，-1 失败

#### pt_setup_identity_mapping

创建恒等映射（VA = PA）。自动映射 UART I/O 区域以支持 S-mode 下的 printf。

```c
int pt_setup_identity_mapping(pt_context_t *ctx, uintptr_t base,
                              uintptr_t size, uintptr_t flags, int level);
```

- **ctx**: 页表上下文
- **base**: 基物理地址
- **size**: 映射区域大小
- **flags**: PTE 标志位
- **level**: 页级别（`PT_LEVEL_4K` / `PT_LEVEL_2M` / `PT_LEVEL_1G`）
- **返回值**: 0 成功，-1 失败

#### pt_pool_reset

重置页表池分配器。所有已分配的页表页变为无效。

```c
void pt_pool_reset(void);
```

#### pt_destroy

释放页表上下文资源（不重置页表池）。

```c
void pt_destroy(pt_context_t *ctx);
```

#### pt_dump

打印根页表中的有效 PTE 条目（调试用）。

```c
void pt_dump(pt_context_t *ctx);
```

#### pt_get_pte

获取指定虚拟地址在特定页表级别的 PTE 指针。用于手动修改 PTE 字段（如 PBMT）。

```c
uintptr_t *pt_get_pte(pt_context_t *ctx, uintptr_t va, int level);
```

- **ctx**: 页表上下文
- **va**: 虚拟地址
- **level**: 目标页表级别（`PT_LEVEL_4K` / `PT_LEVEL_2M` / `PT_LEVEL_1G`）
- **返回值**: PTE 指针，如果目标级别的 PTE 不存在则返回 NULL

#### get_pt_page_addr

获取包含指定虚拟地址 PTE 的页表页的物理地址。

```c
uintptr_t get_pt_page_addr(pt_context_t *ctx, uintptr_t va, int target_level);
```

- **ctx**: 页表上下文
- **va**: 虚拟地址
- **target_level**: 页表级别（0=L0 页表页, 1=L1 页表页, 2=根页表）
- **返回值**: 页表页的物理地址，出错返回 0

---

### satp 控制

#### vm_enable

启用虚拟内存（写 satp CSR 并执行 sfence.vma）。

```c
void vm_enable(pt_context_t *ctx, unsigned asid);
```

- **ctx**: 已配置映射的页表上下文
- **asid**: 地址空间标识符（0 表示不使用 ASID）

#### vm_disable

禁用虚拟内存（设置 satp 为 Bare 模式并刷新 TLB）。

```c
void vm_disable(void);
```

#### vm_sfence_vma

执行 sfence.vma 指令刷新 TLB。

```c
void vm_sfence_vma(uintptr_t vaddr, uintptr_t asid);
```

- **vaddr**: 要刷新的虚拟地址（0 表示全部）
- **asid**: 要刷新的 ASID（0 表示全部）

#### vm_switch_mode

切换到不同的 Sv 模式。禁用 VM，重新初始化页表，并重建恒等映射。

```c
void vm_switch_mode(pt_context_t *ctx, int new_mode);
```

---

### 高级执行 API

#### vm_run_in_smode

在启用虚拟内存的 S-mode 下执行函数。这是最常用的测试入口。

```c
uintptr_t vm_run_in_smode(pt_context_t *ctx,
                           uintptr_t (*fn)(uintptr_t),
                           uintptr_t arg);
```

- **ctx**: 页表上下文（必须已设置恒等映射）
- **fn**: 在 S-mode 下执行的函数
- **arg**: 传递给 fn 的参数
- **返回值**: fn 的返回值

**内部执行流程**：

1. 配置 PMP entry 15 为全地址空间 NAPOT RWX（允许 S-mode 访问）
2. 配置 trap 委托（page fault → S-mode）
3. 设置 stvec 指向 S-mode trap handler
4. 启用虚拟内存（写 satp + sfence.vma）
5. 通过 `run_in_priv(PRIV_S, fn, arg)` 切换到 S-mode 执行 fn
6. fn 返回后，禁用 VM、恢复 medeleg、清除 PMP entry

---

#### vm_run_in_umode

在启用虚拟内存的 U-mode 下执行函数。用于 Ssnpm（Pointer Masking）等需要 U-mode 执行的测试。

```c
uintptr_t vm_run_in_umode(pt_context_t *ctx,
                           uintptr_t (*fn)(uintptr_t),
                           uintptr_t arg);
```

- **ctx**: 页表上下文（必须已设置恒等映射）
- **fn**: 在 U-mode 下执行的函数
- **arg**: 传递给 fn 的参数
- **返回值**: fn 的返回值

**与 `vm_run_in_smode` 的差异：**
- 使用 `mret` 直接从 M-mode 跳转到 U-mode（MPP=0）
- 调用方必须在页表映射中设置 `PTE_U` 标志
- 函数通过 ecall 返回 M-mode（CAUSE_ECALL_FROM_U, cause 8）
- 不委托 page fault 到 S-mode（由 M-mode trap_record 捕获）

---

## PTE 标志位

| 标志 | 值 | 说明 |
|------|-----|------|
| `PTE_V` | bit 0 | Valid — 有效位 |
| `PTE_R` | bit 1 | Read — 读权限 |
| `PTE_W` | bit 2 | Write — 写权限 |
| `PTE_X` | bit 3 | Execute — 执行权限 |
| `PTE_U` | bit 4 | User — U-mode 可访问 |
| `PTE_G` | bit 5 | Global — 全局映射 |
| `PTE_A` | bit 6 | Accessed — 已访问 |
| `PTE_D` | bit 7 | Dirty — 已修改 |

**Svpbmt 扩展字段（PTE bits 62-61）：**

| 常量 | 值 | 说明 |
|------|-----|------|
| `PBMT_PMA` | 00 | 使用底层 PMA（Physical Memory Attributes） |
| `PBMT_NC` | 01 | Non-cacheable, idempotent, RVWMO |
| `PBMT_IO` | 10 | Non-cacheable, non-idempotent, I/O ordering |
| `PBMT_RSVD` | 11 | 保留（触发 page fault） |

提取宏：`PTE_PBMT(pte)` 返回 PBMT 字段值。

**常用组合**：

| 宏 | 值 | 说明 |
|----|-----|------|
| `PTE_RWX` | R\|W\|X | 读写执行 |
| `PTE_RWXU` | R\|W\|X\|U | 读写执行 + U-mode |
| `PTE_RWXAD` | R\|W\|X\|A\|D | 读写执行 + 已访问已修改 |

> **注意**：使用 `PTE_A` 和 `PTE_D` 可以避免硬件触发 page fault 来设置这些位。在测试中建议始终设置 `PTE_A | PTE_D`。

---

## 页表池

页表页从链接脚本中静态预留的 `.page_tables` 段分配，使用 bump allocator：

- **容量**：64 页 × 4KB = 256KB
- **分配**：`pt_alloc_page()` 返回一个已清零的 4KB 对齐页
- **重置**：`pt_pool_reset()` 将分配指针重置到起始位置

链接脚本中的相关段定义：

```ld
/* 页表池 */
. = ALIGN(0x1000);
.page_tables (NOLOAD) : {
    PROVIDE(__page_tables_start = .);
    . += 64 * 4096;    /* 256KB = 64 pages */
    PROVIDE(__page_tables_end = .);
}

/* S-mode 栈 */
. = ALIGN(16);
.smode_stack (NOLOAD) : {
    PROVIDE(__smode_stack_start = .);
    . += 16 * 1024;    /* 16KB S-mode stack */
    . = ALIGN(16);
    PROVIDE(__smode_stack_end = .);
}
```

---

## 构建配置

### Makefile 配置

在扩展的 Makefile 中设置 `ENABLE_VM = 1` 即可启用 VM 支持：

```makefile
# Sv39/Makefile
TARGET = sv39_test.elf
ENABLE_VM = 1
EXT_OBJS = main.o
include ../common/Makefile.common
```

`ENABLE_VM = 1` 会自动：
- 将 `common/vm/page_table.o` 和 `common/vm/satp.o` 加入链接
- 添加 `-DENABLE_VM` 编译宏
- 添加 `-I$(VM_DIR)` 头文件搜索路径

### 编译和运行

```bash
# 编译单个测试
make Sv39          # 或 make Sv48 / make Sv57

# 编译所有测试（包括 pmp、smepmp、Sv39、Sv48、Sv57）
make

# 在 QEMU 上运行
cd Sv39 && make qemu

# 或直接使用 qemu-system-riscv64
qemu-system-riscv64 -machine virt -nographic -bios none -kernel Sv39/sv39_test.elf

# 清理
make clean
```

---

## 编写测试用例

### 基本模式

每个测试用例遵循以下模式：

```c
#include "test_framework.h"
#include "vm/vm.h"

/* S-mode 下执行的测试函数 */
static uintptr_t my_smode_test(uintptr_t arg) {
    /* 在 S-mode + VM 启用状态下执行 */
    volatile uintptr_t *ptr = (volatile uintptr_t *)arg;
    ptr[0] = 0xDEADBEEF;
    if (ptr[0] != 0xDEADBEEF)
        return 1;  /* 失败 */
    return 0;      /* 成功 */
}

TEST_REGISTER(test_my_vm_test);
bool test_my_vm_test(void) {
    TEST_BEGIN("MY-01: description");

    /* 1. 重置页表池并初始化上下文 */
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SATP_MODE_SV39);  /* 或 SV48 / SV57 */

    /* 2. 设置恒等映射 */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                                        flags, PT_LEVEL_1G);
    TEST_ASSERT("identity mapping setup", ret == 0);

    /* 3. 在 S-mode + VM 下执行测试 */
    uintptr_t result = vm_run_in_smode(&ctx, my_smode_test,
                                        (uintptr_t)test_data);
    TEST_ASSERT("S-mode test passed", result == 0);

    /* 4. 清理 */
    pt_pool_reset();
    TEST_END();
}
```

### 不同页大小的映射示例

#### 1GB gigapage

```c
uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                          flags, PT_LEVEL_1G);
```

#### 2MB megapage

```c
uintptr_t base = PLATFORM_MEM_BASE;
uintptr_t size = 16 * PAGE_SIZE_2M;  /* 32MB，覆盖代码+数据+栈+页表 */
pt_setup_identity_mapping(&ctx, base, size,
                          flags, PT_LEVEL_2M);
```

#### 4KB page

```c
uintptr_t base = PLATFORM_MEM_BASE;
uintptr_t size = 2 * PAGE_SIZE_2M;   /* 4MB */
pt_setup_identity_mapping(&ctx, base, size,
                          flags, PT_LEVEL_4K);
```

> **注意**：使用 4KB 页时，映射区域越大，消耗的页表池页越多。4MB 区域需要约 3 个页表页（1 个 L2 + 1 个 L1 + 1024 个 L0 PTE 分布在 2 个 L0 页中）。

### 手动映射单个页面

```c
/* 映射单个 4KB 页 */
pt_map_page(&ctx, 0x80100000, 0x80100000,
            PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
            PT_LEVEL_4K);

/* 映射 MMIO 区域（无执行权限） */
pt_map_page(&ctx, 0x10000000, 0x10000000,
            PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
            PT_LEVEL_4K);
```

### 模式切换

```c
pt_context_t ctx;
pt_pool_reset();
pt_init(&ctx, SATP_MODE_SV39);

/* 设置 Sv39 恒等映射 */
pt_setup_identity_mapping(&ctx, base, size, flags, PT_LEVEL_1G);

/* 运行 Sv39 测试 */
vm_run_in_smode(&ctx, test_fn, arg);

/* 切换到 Sv48（自动重建恒等映射） */
vm_switch_mode(&ctx, SATP_MODE_SV48);

/* 运行 Sv48 测试 */
vm_run_in_smode(&ctx, test_fn, arg);
```

---

## 已有测试用例

### Sv39 测试 (Sv39/main.c)

| 测试 ID | 测试名称 | 页大小 | 映射区域 |
|---------|----------|--------|----------|
| SV39-01 | 1GB gigapage identity mapping | 1GB | 0x80000000 起 1GB |
| SV39-02 | 2MB megapage identity mapping | 2MB | 0x80000000 起 32MB |
| SV39-03 | 4KB page identity mapping | 4KB | 0x80000000 起 4MB |

### Sv48 测试 (Sv48/main.c)

| 测试 ID | 测试名称 | 页大小 | 映射区域 |
|---------|----------|--------|----------|
| SV48-01 | 1GB gigapage identity mapping | 1GB | 0x80000000 起 1GB |
| SV48-02 | 2MB megapage identity mapping | 2MB | 0x80000000 起 32MB |
| SV48-03 | 4KB page identity mapping | 4KB | 0x80000000 起 4MB |

### Sv57 测试 (Sv57/main.c)

| 测试 ID | 测试名称 | 页大小 | 映射区域 |
|---------|----------|--------|----------|
| SV57-01 | 1GB gigapage identity mapping | 1GB | 0x80000000 起 1GB |
| SV57-02 | 2MB megapage identity mapping | 2MB | 0x80000000 起 32MB |
| SV57-03 | 4KB page identity mapping | 4KB | 0x80000000 起 4MB |

每个测试用例的验证方法：
1. 初始化页表上下文
2. 设置恒等映射
3. 通过 `vm_run_in_smode()` 在 S-mode 下执行读写测试
4. 验证写入的 magic value 能正确读回

---

## 关键注意事项

### 恒等映射覆盖范围

使用 `pt_setup_identity_mapping()` 时，映射区域必须覆盖：
- **代码段**（.text）— 用于指令获取
- **数据段**（.data / .bss）— 用于全局变量访问
- **栈**（.stack）— 用于函数调用
- **页表池**（.page_tables）— 用于页表查找
- **测试数据区域** — 用于测试读写

`pt_setup_identity_mapping()` 会自动额外映射 UART I/O 区域（`PLATFORM_UART0_BASE`），以支持 S-mode 下的 printf 输出。

### PTE_A 和 PTE_D 位

RISC-V 规范允许硬件在首次访问时自动设置 A（Accessed）和 D（Dirty）位，也允许硬件在 A/D 未设置时触发 page fault。为避免不必要的 page fault，建议在映射时始终设置 `PTE_A | PTE_D`。

### PMP 与虚拟内存

`vm_run_in_smode()` 自动配置 PMP entry 15 为全地址空间 NAPOT RWX，以允许 S-mode 访问所有物理内存。函数返回后自动清除该 PMP entry。

如果测试需要同时验证 PMP 和 VM 的交互行为，需要手动管理 PMP 配置，不使用 `vm_run_in_smode()`。

### 页表池容量

页表池共 64 页（256KB）。不同映射方式的页表消耗：

| 映射方式 | 每次映射消耗 | 说明 |
|----------|-------------|------|
| 1GB gigapage | 1 页（根页表） | 直接在根页表中安装 leaf PTE |
| 2MB megapage | 1-2 页 | 根页表 + 1 个 L1 页表页 |
| 4KB page | 2-3+ 页 | 根页表 + L1 页表页 + L0 页表页 |

> **注意**：`pt_init()` 会分配 1 页作为根页表。UART 映射额外消耗 1-2 页（取决于模式级数）。

---

## 参考

- RISC-V Privileged Specification 4.3-4.5（Sv39/Sv48/Sv57 虚拟内存系统）
- `SPEC/supervisor.adoc` — Supervisor 级规范
- `SPEC/machine.adoc` — Machine 级规范（satp CSR 定义）
- `common/vm/vm.h` — API 声明
- `common/vm/vm_defs.h` — 常量和宏定义
