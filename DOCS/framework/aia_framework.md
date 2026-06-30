**中文 | [English](../framework_en/aia_framework_en.md)**

## AIA 测试框架公共文档

**适用范围**：RISC-V AIA (Advanced Interrupt Architecture) 全部测试计划

**规范依据**：RISC-V Advanced Interrupt Architecture Specification, Version 1.0 (Ratified June 2023, Revised 20250312)

---

### 测试计划依赖关系与执行顺序

```
aia_smaia_test_plan.md (CSR + 优先级)
        │
        ▼
aia_imsic_test_plan.md (IMSIC)
        │
        ├──────────────────┐
        ▼                  ▼
aia_aplic_test_plan.md   aia_vs_test_plan.md (VS 虚拟化)
        │                  │
        └──────┬───────────┘
               ▼
aia_ipi_iommu_test_plan.md (IPI + IOMMU)
```

| 执行顺序 | 测试计划 | 前置依赖 |
|---------|----------|---------|
| 1 | `aia_smaia_test_plan.md` | 无 |
| 2 | `aia_imsic_test_plan.md` | CSR 基础功能通过 |
| 3 | `aia_aplic_test_plan.md` | IMSIC 功能通过（MSI 转发模式依赖 IMSIC） |
| 3 | `aia_vs_test_plan.md` | CSR + IMSIC 功能通过 |
| 4 | `aia_ipi_iommu_test_plan.md` | IMSIC 功能通过 |

---

### 目录结构

AIA 测试框架包含 5 个子模块目录，每个子模块独立编译和运行：

```
damo-priv-test/
├── aia_aplic/           # APLIC (Advanced Platform-Level Interrupt Controller) 测试
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   ├── aia_encoding.h   # AIA CSR 编码常量（共享）
│   ├── aia_helpers.h    # AIA 辅助函数（共享，内联汇编实现）
│   ├── aia_platform.h   # 平台配置（共享）
│   └── tests/
│       ├── test_register.c
│       └── ...
├── aia_imsic/           # IMSIC (Incoming MSI Controller) 测试
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   ├── aia_encoding.h
│   ├── aia_helpers.h
│   ├── aia_platform.h
│   └── tests/
├── aia_smaia/           # Smaia (Supervisor-mode AIA) CSR 测试
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   ├── aia_encoding.h
│   ├── aia_helpers.h
│   ├── aia_platform.h
│   └── tests/
├── aia_hypervisor/              # VS-mode AIA 虚拟化测试
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   ├── aia_encoding.h
│   ├── aia_helpers.h
│   ├── aia_platform.h
│   └── tests/
└── aia_ipi_iommu/       # IPI + IOMMU 集成测试
    ├── Makefile
    ├── kernel.ld
    ├── main.c
    ├── aia_encoding.h
    ├── aia_helpers.h
    ├── aia_platform.h
    └── tests/
```

**共享头文件说明**：

- **`aia_encoding.h`**：定义 AIA 相关 CSR 地址常量、位域掩码、中断优先级等编码
- **`aia_helpers.h`**：提供 `static inline` 辅助函数，使用内联汇编封装 CSR 读写（如 `aia_write_miselect`、`aia_read_mireg` 等）
- **`aia_platform.h`**：平台相关配置（如 IMSIC 基地址、APLIC 域数量等）

每个子模块目录都包含这三个头文件的副本，确保独立编译。

---

### 通用辅助函数接口

> **实现说明**：以下 API 在 `aia_helpers.h` 中以 `static inline` 函数 + 内联汇编形式实现，直接封装 CSR 读写指令。每个 AIA 子模块目录（`aia_aplic/`、`aia_imsic/` 等）都包含共享的 `aia_helpers.h`、`aia_encoding.h`、`aia_platform.h` 三个头文件。

#### CSR 操作

```c
/* CSR 读写基础接口（内联汇编实现） */
uint64_t csr_read(unsigned int csr_num);
void csr_write(unsigned int csr_num, uint64_t val);
void csr_set(unsigned int csr_num, uint64_t mask);
void csr_clear(unsigned int csr_num, uint64_t mask);
uint64_t csr_swap(unsigned int csr_num, uint64_t val);  /* CSRRW 语义 */
```

**示例（aia_helpers.h 中的实际实现）**：

```c
static inline void aia_write_miselect(uintptr_t val) {
    asm volatile("csrw " CSR_STR(0x350) ", %0" :: "r"(val) : "memory");
}

static inline uintptr_t aia_read_miselect(void) {
    uintptr_t val;
    asm volatile("csrr %0, " CSR_STR(0x350) : "=r"(val) :: "memory");
    return val;
}
```

#### 特权级切换

```c
/* 切换到指定特权级执行 */
void goto_priv(unsigned int priv_level);

/* 特权级常量 */
#define PRIV_M   3
#define PRIV_HS  2   /* HS-mode (Hypervisor) */
#define PRIV_S   1
#define PRIV_U   0
#define PRIV_VS  5   /* VS-mode (Virtual Supervisor) */
#define PRIV_VU  4   /* VU-mode (Virtual User) */
```

#### 异常期望宏

```c
/*
 * EXPECT_TRAP(cause, statement)
 * 执行 statement，期望触发 cause 指定的异常。
 * 若未触发异常或触发了不同的异常，测试失败。
 */
#define EXPECT_TRAP(cause, statement)  do { \
    arm_trap(cause);                        \
    statement;                              \
    TEST_ASSERT("expected trap occurred", trap_triggered(cause)); \
    disarm_trap();                           \
} while (0)

/* 异常原因码 */
#define CAUSE_ILLEGAL_INSTRUCTION    2
#define CAUSE_VIRTUAL_INSTRUCTION    22
```

#### 轮询等待

```c
/*
 * poll_with_timeout(timeout_us, condition)
 * 轮询直到 condition 为真或超时。
 * 超时后 condition 仍不为真则继续（由调用者检查）。
 */
#define poll_with_timeout(timeout_us, condition)  do { \
    unsigned long _deadline = read_time() + us_to_ticks(timeout_us); \
    while (!(condition) && read_time() < _deadline) { \
        asm volatile("nop");                           \
    }                                                  \
} while (0)
```

#### 内存映射 I/O

```c
/* 32 位小端读写 */
uint32_t readl(volatile void *addr);
void writel(uint32_t val, volatile void *addr);
```

---

### IMSIC 辅助函数接口

```c
/* 平台配置查询 */
uintptr_t platform_imsic_m_base(void);         /* M 级 IMSIC 基地址 A */
uintptr_t platform_imsic_s_base(void);         /* S 级 IMSIC 基地址 B */
unsigned int platform_imsic_m_stride_log2(void);  /* M 级步进 C */
unsigned int platform_imsic_s_stride_log2(void);  /* S 级步进 D */
unsigned int platform_hart_count(void);
unsigned int platform_max_identity(void);      /* 最大实现的中断 identity N */
unsigned int platform_geilen(void);            /* guest 中断文件数量 */

/* 中断文件地址计算 */
uintptr_t imsic_m_file_addr(unsigned int hart);
uintptr_t imsic_s_file_addr(unsigned int hart);
uintptr_t imsic_guest_file_addr(unsigned int hart, unsigned int guest);

/* 中断文件寄存器操作（通过 *iselect/*ireg 间接访问） */
void imsic_set_eidelivery(uint32_t val);
uint32_t imsic_get_eidelivery(void);

void imsic_set_eithreshold(uint32_t val);
uint32_t imsic_get_eithreshold(void);

void imsic_set_eip(unsigned int identity, int val);  /* 设置/清除 pending */
int imsic_get_eip(unsigned int identity);             /* 查询 pending */

void imsic_set_eie(unsigned int identity, int val);  /* 设置/清除 enable */
int imsic_get_eie(unsigned int identity);             /* 查询 enable */

/* Guest 中断文件操作（需指定 hart 和 guest 编号） */
void guest_imsic_set_eie(unsigned int hart, unsigned int guest,
                         unsigned int identity, int val);
void guest_imsic_set_eip(unsigned int hart, unsigned int guest,
                         unsigned int identity, int val);
int guest_imsic_get_eip(unsigned int hart, unsigned int guest,
                        unsigned int identity);
```

---

### APLIC 辅助函数接口

```c
/* APLIC 域基地址查询 */
volatile void *aplic_domain_base(unsigned int domain_id);

/* IDC（直接投递）结构基地址 */
volatile void *aplic_idc_base(unsigned int domain_id, unsigned int hart);

/* domaincfg 操作 */
void aplic_set_domaincfg(unsigned int domain_id, uint32_t flags);
void aplic_set_domaincfg_ie(unsigned int domain_id, int enable);

/* Source 配置 */
void aplic_set_sourcecfg(unsigned int domain_id, unsigned int source,
                         uint32_t mode);

/* Source mode 常量 */
#define APLIC_SM_INACTIVE  0
#define APLIC_SM_DETACHED  1
#define APLIC_SM_EDGE1     4
#define APLIC_SM_EDGE0     5
#define APLIC_SM_LEVEL1    6
#define APLIC_SM_LEVEL0    7

/* domaincfg 标志位 */
#define APLIC_IE       (1 << 8)
#define APLIC_DM_MSI   (1 << 2)
#define APLIC_BE       (1 << 0)

/* Pending/Enable 操作 */
void aplic_set_pending(unsigned int domain_id, unsigned int source);
void aplic_clear_pending(unsigned int domain_id, unsigned int source);
int aplic_get_pending(unsigned int domain_id, unsigned int source);
void aplic_set_ie(unsigned int domain_id, unsigned int source, int enable);

/* Target 配置 */
void aplic_set_target(unsigned int domain_id, unsigned int source,
                      unsigned int hart_index, unsigned int guest_index,
                      unsigned int eiid);

/* MSI 地址配置 */
void aplic_set_mmsiaddrcfg(unsigned int domain_id, uint64_t base_ppn,
                           unsigned int lhxw, unsigned int hhxw,
                           unsigned int lhxs, unsigned int hhxs);
void aplic_set_smsiaddrcfg(unsigned int domain_id, uint64_t base_ppn,
                           unsigned int lhxw, unsigned int hhxw,
                           unsigned int lhxs, unsigned int hhxs);

/* genmsi 操作 */
void aplic_write_genmsi(unsigned int domain_id, unsigned int hart_index,
                        unsigned int eiid);
int aplic_genmsi_busy(unsigned int domain_id);
```

---

### IOMMU 辅助函数接口

```c
/* 设备上下文 MSI 配置 */
void iommu_set_msi_addr_mask(void *dev_ctx, uint32_t mask);
void iommu_set_msi_addr_pattern(void *dev_ctx, uint32_t pattern);

/* MSI 页表操作 */
typedef struct {
    uint64_t dw0;
    uint64_t dw1;
} msi_pte_t;

/* 设备 MSI 写入模拟 */
void device_write_msi(unsigned int dev_id, uintptr_t addr, uint32_t data);

/* 虚拟中断文件地址计算 */
uintptr_t virtual_imsic_addr(unsigned int file_num);
```

---

### 多 Hart 操作接口

```c
/* 获取当前 hart ID */
unsigned int hart_id(void);

/* 在指定 hart 上执行代码块 */
#define on_hart(hart, block)  do { \
    send_ipi_to(hart);             \
    /* hart 执行 block 并同步返回 */ \
    block;                          \
} while (0)

/* 触发/清除中断辅助 */
void trigger_m_timer_interrupt(void);
void clear_m_timer_interrupt(void);
void trigger_m_software_interrupt(void);
void clear_m_software_interrupt(void);
```

---

### 测试执行通用注意事项

#### WARL 字段处理

AIA 中大量寄存器字段为 WARL（Write Any, Read Legal）。测试中：
- 写入任意值后读回，验证读回值属于合法值集合
- 不能假设写入值与读回值完全一致
- 合法值集合由实现决定，测试需列出规范允许的所有合法值进行断言

#### 中断投递延迟

IMSIC 中断文件状态变化到 `mip`/`hgeip` 的反映、APLIC MSI 到达 IMSIC，均可能存在延迟。涉及中断信号检查的测试必须使用 `poll_with_timeout` 机制，不能假设写入后立即可见。

#### Hart 间通信与同步

IPI 和多 hart 测试需要 hart 间同步机制：
- 使用 spinlock + 共享内存实现 hart 间同步
- MSI 发送前需执行 `fence ow, ow` 确保之前的内存写入对目标 hart 可见
- 目标 hart 收到 MSI 后需执行 `fence ir, ir` 确保观察到源 hart 的写入

#### 虚拟化测试环境

VS 级（Group 9）测试需要 hypervisor 环境：
- 需配置 VS-mode guest 执行上下文（`hstatus`、`vsstatus`、`vstvec` 等）
- `hstatus`.VGEIN 选择 guest 中断文件
- 通过 `sret` 进入 VS-mode，通过 trap 返回 HS-mode

#### 特权级切换

多数测试涉及跨特权级操作（M→S→VS）：
- 通过 trap handler 和 `mret`/`sret` 实现特权级切换
- 所有跨特权级测试代码使用 `.option norvc` 确保指令定长（`mepc += 4` 可靠跳过故障指令）

#### IOMMU 设备模拟

IOMMU 相关测试可能需要可控 DMA 设备或设备模拟器来产生指定地址的 MSI 写入。平台需提供 `device_write_msi()` 的具体实现。

---

### CSR 编号参考

| CSR 名称 | 编号 | 说明 |
|---------|------|------|
| `miselect` | 0x350 | M 级间接寄存器选择 |
| `mireg` | 0x351 | M 级间接寄存器别名 |
| `mtopei` | 0x35C | M 级 top 外部中断（IMSIC） |
| `mtopi` | 0xFB0 | M 级 top 中断 |
| `mvien` | 0x308 | M 级虚拟中断使能 |
| `mvip` | 0x309 | M 级虚拟中断 pending |
| `siselect` | 0x150 | S 级间接寄存器选择 |
| `sireg` | 0x151 | S 级间接寄存器别名 |
| `stopei` | 0x15C | S 级 top 外部中断（IMSIC） |
| `stopi` | 0xDB0 | S 级 top 中断 |
| `vsiselect` | 0x250 | VS 级间接寄存器选择 |
| `vsireg` | 0x251 | VS 级间接寄存器别名 |
| `vstopei` | 0x25C | VS 级 top 外部中断（IMSIC） |
| `vstopi` | 0xEB0 | VS 级 top 中断 |
| `hvien` | 0x608 | Hypervisor 虚拟中断使能 |
| `hvictl` | 0x609 | Hypervisor 虚拟中断控制 |
| `hviprio1` | 0x646 | Hypervisor VS 级优先级 1 |
| `hviprio2` | 0x647 | Hypervisor VS 级优先级 2 |
| `hvip` | 0x645 | Hypervisor 虚拟中断 pending |
