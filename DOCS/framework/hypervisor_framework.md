**中文 | [English](../framework_en/hypervisor_framework_en.md)**

# RISC-V Hypervisor Extension 测试框架设计

本文档描述在当前 common 测试框架基础上，新增 RISC-V Hypervisor (H) 扩展测试框架的接口设计和功能规划。

---

## 概述

Hypervisor 扩展的测试比现有的 PMP / VM (Sv39/48/57) 测试复杂度更高，核心原因在于它引入了：

- **虚拟化模式 (V=1)**：新增 VS-mode 和 VU-mode 两个特权级
- **HS-mode**：将 S-mode 扩展为 Hypervisor-extended Supervisor Mode
- **两阶段地址翻译**：VS-stage (`vsatp`) + G-stage (`hgatp`)
- **大量新增 CSR**：hstatus、hedeleg、hideleg、hgatp、hvip 等 hypervisor CSR，以及 vsstatus、vsatp 等 VS CSR
- **新的 trap 类型**：virtual-instruction exception (cause=22)、guest-page fault (cause=20/21/23)
- **新的特权指令**：HLV/HLVX/HSV（虚拟机 load/store）、HFENCE.VVMA/HFENCE.GVMA

现有框架只支持 M → S → U 三级切换，需要扩展到支持 M → HS → VS → VU 四级切换，并且需要管理 `hgatp` 控制的 G-stage 页表。

---

## 规范参考

- `SPEC/hypervisor.adoc` — RISC-V Hypervisor Extension (H), Version 1.0
- `SPEC/supervisor.adoc` — Supervisor-Level ISA
- `SPEC/machine.adoc` — Machine-Level ISA

---

## 与现有框架的关系

### 现有框架模块

| 模块 | 文件 | 功能 |
|------|------|------|
| 测试框架 | `common/test_framework.h/c` | TEST_BEGIN/END、断言宏、结果统计 |
| 特权级切换 | `common/privilege.c` | M↔S↔U 双向切换 |
| Trap 处理 | `common/trap.c` + `trap_asm.S` | M-mode/S-mode trap handler |
| CSR 访问 | `common/csr_accessors.c` | 运行时动态 CSR 读写 |
| 内存操作 | `common/mem_ops.h` | load/store/exec 原语 |
| VM 管理 | `common/vm/` | Sv39/48/57 页表管理、satp 控制 |
| 编码定义 | `common/encoding.h` | CSR 地址、mstatus 位域、cause code |

### 复用策略

- **直接复用**：test_framework（测试宏、结果统计）、mem_ops（内存原语）、uart（串口输出）、entry.S（启动代码）
- **扩展复用**：encoding.h（新增 CSR 地址和 cause code）、csr_accessors.c（新增 CSR case）、trap.c/trap_asm.S（扩展 handler 支持 VS-mode trap）
- **参考设计**：vm/（G-stage 页表管理参考 VS-stage 页表管理的模式）

---

## 新增模块设计

### 文件组织结构

```
common/hyp/
├── hyp_defs.h             # CSR 地址、位域、cause code、hgatp/vsatp 编码
├── hyp_csr.c/.h           # CSR 操作 API（hstatus/hedeleg/hgatp/henvcfg、委托、WARL 探测）
├── hyp_priv.c/.h          # 虚拟化特权级切换 (M↔HS↔VS↔VU)
├── hyp_trap.c/.h          # HS/M-mode trap handler（捕获 VS/VU-mode trap）
├── hyp_trap_asm.S         # HS/M-mode trap entry（汇编）
├── hyp_vs_trap.c/.h       # VS-mode trap handler（委托到 VS 后由 guest 处理）
├── hyp_fence.c/.h         # HFENCE.VVMA / HFENCE.GVMA 封装
├── hyp_ldst.c/.h          # HLV / HLVX / HSV 指令封装
├── gstage_pt.c/.h         # G-stage 页表管理 (hgatp, Sv39x4/48x4/57x4)
├── two_stage.c/.h         # 两阶段翻译核心 (two_stage_init/enable/run_in_vs/vu)
├── two_stage_helpers.c/.h # 两阶段场景助手 (ts2_setup_* / ts2_run_check_*)
├── test_vs_helpers.c/.h   # VS-mode 触发器 (test_vs_load/store/exec_*)
├── hyp_test_helpers.c/.h  # htinst golden 值、implicit-walk victim 构造
├── hyp_reset.c/.h         # Hypervisor 状态重置 (hyp_reset_state)
└── hyp_test.h             # 测试宏（REQUIRE_*_MODE / CHECK_* / EXPECT_* / HYP_TEST_END）

<套件目录>/                 # 如 Hypervisor_Zaamo/、Shtvala/、Sv39x4_Sv39/
├── Makefile               # include ../common/Makefile.common；ENABLE_HYP/ENABLE_TWO_STAGE；SUITE_* 声明
├── kernel.ld
├── main.c
└── tests/                 # 测试文件（#include 进 test_register.c 单一编译单元）
```

---

### 模块 1：Hypervisor CSR 编码定义 (`common/hyp/hyp_defs.h`)

**功能**：定义所有 hypervisor 相关的 CSR 地址、位域常量和 cause code。

#### Hypervisor CSR 地址

| CSR | 地址 | 说明 |
|-----|------|------|
| `hstatus` | 0x600 | Hypervisor 状态寄存器 |
| `hedeleg` | 0x602 | Hypervisor 异常委托 |
| `hideleg` | 0x603 | Hypervisor 中断委托 |
| `hvip` | 0x645 | Hypervisor 虚拟中断 pending |
| `hip` | 0x644 | Hypervisor 中断 pending |
| `hie` | 0x604 | Hypervisor 中断使能 |
| `hgeip` | 0xE12 | Guest 外部中断 pending |
| `hgeie` | 0x607 | Guest 外部中断使能 |
| `henvcfg` | 0x60A | Hypervisor 环境配置 |
| `hcounteren` | 0x606 | Hypervisor 计数器使能 |
| `htimedelta` | 0x605 | Hypervisor 时间偏移 |
| `htval` | 0x643 | Hypervisor trap value |
| `htinst` | 0x64A | Hypervisor trap instruction |
| `hgatp` | 0x680 | Guest 地址翻译和保护 |

#### Virtual Supervisor CSR 地址

| CSR | 地址 | 说明 |
|-----|------|------|
| `vsstatus` | 0x200 | VS-mode 状态 |
| `vsie` | 0x204 | VS-mode 中断使能 |
| `vstvec` | 0x205 | VS-mode trap 向量 |
| `vsscratch` | 0x240 | VS-mode scratch |
| `vsepc` | 0x241 | VS-mode exception PC |
| `vscause` | 0x242 | VS-mode cause |
| `vstval` | 0x243 | VS-mode trap value |
| `vsip` | 0x244 | VS-mode 中断 pending |
| `vsatp` | 0x280 | VS-mode 地址翻译 |

#### M-mode 扩展 CSR

| CSR | 地址 | 说明 |
|-----|------|------|
| `mtval2` | 0x34B | 第二 trap value (guest physical address >> 2) |
| `mtinst` | 0x34A | Trap instruction |

#### hstatus 位域定义

| 字段 | 位位置 | 说明 |
|------|--------|------|
| VSBE | bit 5 | VS-mode 字节序 |
| GVA | bit 6 | Guest Virtual Address 标志 |
| SPV | bit 7 | Supervisor Previous Virtualization mode |
| SPVP | bit 8 | Supervisor Previous Virtual Privilege |
| HU | bit 9 | Hypervisor in U-mode (允许 U-mode 执行 HLV/HSV) |
| VGEIN | bits 17:12 | Virtual Guest External Interrupt Number |
| VTVM | bit 20 | Virtual TVM (VS-mode SFENCE/satp 触发 trap) |
| VTW | bit 21 | Virtual TW (VS-mode WFI 触发 trap) |
| VTSR | bit 22 | Virtual TSR (VS-mode SRET 触发 trap) |
| VSXL | bits 33:32 | VS-mode XLEN 控制 (RV64 only) |

#### hgatp MODE 编码

| 值 | 名称 | 说明 |
|----|------|------|
| 0 | Bare | 无翻译 |
| 8 | Sv39x4 | 41-bit GPA, 16KB 根页表 |
| 9 | Sv48x4 | 50-bit GPA, 16KB 根页表 |
| 10 | Sv57x4 | 59-bit GPA, 16KB 根页表 |

#### 新增异常 Cause Code

| 值 | 名称 | 说明 |
|----|------|------|
| 10 | `CAUSE_ECALL_FROM_VS` | VS-mode ecall |
| 20 | `CAUSE_INST_GUEST_PAGE_FAULT` | 指令 guest-page fault |
| 21 | `CAUSE_LOAD_GUEST_PAGE_FAULT` | Load guest-page fault |
| 22 | `CAUSE_VIRTUAL_INSTRUCTION` | Virtual-instruction exception |
| 23 | `CAUSE_STORE_GUEST_PAGE_FAULT` | Store/AMO guest-page fault |

#### hgatp 构造宏

```c
#define MAKE_HGATP(mode, vmid, ppn) \
    (((uintptr_t)(mode) << HGATP_MODE_SHIFT) | \
     ((uintptr_t)(vmid) << HGATP_VMID_SHIFT) | \
     ((uintptr_t)(ppn) & HGATP_PPN_MASK))
```

---

### 模块 2：虚拟化特权级切换 (`common/hyp/hyp_priv.c`)

**功能**：扩展现有的 `privilege.c` 特权级切换机制，支持 V=0/V=1 的完整切换。

#### 特权模式定义

```
V=0: M-mode → HS-mode → U-mode       (复用现有 PRIV_M / PRIV_S / PRIV_U)
V=1:           VS-mode → VU-mode       (新增 PRIV_VS / PRIV_VU)
```

| 常量 | 值 | 含义 |
|------|----|------|
| `PRIV_HS` | 1 | HS-mode (V=0, nominal S) — 复用 PRIV_S |
| `PRIV_VS` | 5 | VS-mode (V=1, nominal S) — 编码: V=1, S=1 |
| `PRIV_VU` | 4 | VU-mode (V=1, nominal U) — 编码: V=1, U=0 |

#### 接口定义

```c
/**
 * goto_vs_mode - 从 HS-mode 切换到 VS-mode
 *
 * 设置 hstatus.SPV=1, sstatus.SPP=1, 然后执行 sret。
 * sret 时 V=SPV=1, 进入 VS-mode。
 */
void goto_vs_mode(void);

/**
 * goto_vu_mode - 从 VS-mode 切换到 VU-mode
 *
 * 设置 sstatus.SPP=0 (实际访问 vsstatus.SPP), 然后执行 sret。
 */
void goto_vu_mode(void);

/**
 * return_to_hs_mode - 从 VS/VU-mode 返回 HS-mode
 *
 * 通过 ecall 触发 trap 到 HS-mode (假设 ecall 未委托到 VS-mode)。
 */
void return_to_hs_mode(void);

/**
 * get_virt_priv - 获取当前完整特权状态 (包含 V bit)
 *
 * Returns: PRIV_M / PRIV_HS / PRIV_U / PRIV_VS / PRIV_VU
 */
unsigned get_virt_priv(void);

/**
 * run_in_vs_mode - 在 VS-mode 下执行函数并返回结果
 *
 * 类似现有 run_in_priv(), 但目标为 VS-mode。
 * 从 M/HS-mode 调用，自动切换到 VS-mode 执行 fn(arg),
 * 然后通过 ecall 返回 HS-mode, 再返回 M-mode。
 */
uintptr_t run_in_vs_mode(uintptr_t (*fn)(uintptr_t), uintptr_t arg);

/**
 * run_in_vu_mode - 在 VU-mode 下执行函数并返回结果
 */
uintptr_t run_in_vu_mode(uintptr_t (*fn)(uintptr_t), uintptr_t arg);
```

#### 切换路径

```
M-mode → HS-mode:  设置 mstatus.MPP=S, mret
HS-mode → VS-mode: 设置 hstatus.SPV=1, sstatus.SPP=1, sret
HS-mode → VU-mode: 设置 hstatus.SPV=1, sstatus.SPP=0, sret
VS-mode → VU-mode: 设置 vsstatus.SPP=0, sret (通过 csrw sstatus)
VS/VU → HS-mode:   ecall → HS-mode trap handler
HS-mode → M-mode:  ecall → M-mode trap handler
```

---

### 模块 3：Hypervisor Trap Handler (`common/hyp/hyp_trap.c` + `hyp_trap_asm.S`)

**功能**：新增 HS-mode trap handler，处理来自 VS/VU-mode 的 trap。

#### 扩展的 trap 记录

```c
typedef struct {
    /* 继承基础 trap_record 字段 */
    bool      armed;
    bool      triggered;
    unsigned  priv_level;    /* trap 入口的特权级 */
    uintptr_t cause;         /* scause (HS-mode) 或 mcause (M-mode) */
    uintptr_t epc;           /* sepc / mepc */
    uintptr_t tval;          /* stval / mtval */
    uintptr_t return_addr;

    /* Hypervisor 扩展字段 */
    uintptr_t htval;         /* guest physical address >> 2 */
    uintptr_t htinst;        /* transformed instruction / pseudoinstruction */
    bool      gva;           /* hstatus.GVA: stval 是否为 guest virtual address */
    bool      spv;           /* hstatus.SPV: trap 来源是否为 V=1 */
} hyp_trap_record_t;
```

#### 接口定义

```c
/**
 * hs_trap_handler - HS-mode trap handler
 *
 * 处理从 VS/VU-mode trap 到 HS-mode 的情况：
 * - VS-mode ecall (cause=10): 用于特权级切换
 * - Virtual-instruction exception (cause=22): 记录 trap 信息
 * - Guest-page fault (cause=20/21/23): 记录 htval/htinst
 * - Page fault 委托到 VS-mode 的情况
 */
unsigned hs_trap_handler(void);

/* 获取 hypervisor trap 扩展信息 */
uintptr_t trap_get_htval(void);
uintptr_t trap_get_htinst(void);
bool      trap_get_gva(void);
bool      trap_get_spv(void);
```

#### Trap 路径说明

```
VS/VU-mode 发生 trap:
  ├── medeleg 未设置 → M-mode trap handler (m_trap_handler)
  ├── medeleg 设置 + hedeleg 未设置 → HS-mode trap handler (hs_trap_handler)
  └── medeleg 设置 + hedeleg 设置 → VS-mode trap handler (通过 vstvec)

HS-mode 发生 trap:
  ├── medeleg 未设置 → M-mode trap handler
  └── medeleg 设置 → HS-mode trap handler (S-mode trap, hstatus.SPV=0)

判断 trap 来源:
  - hstatus.SPV=1: trap 来自 V=1 (VS/VU-mode)
  - hstatus.SPV=0: trap 来自 V=0 (HS-mode 自身)
```

#### 需要扩展现有 trap handler 的地方

1. **`m_trap_handler`**：需要识别 VS-mode ecall (cause=10)，与 HS-mode ecall (cause=9) 区分
2. **`s_trap_handler`**（作为 HS-mode trap handler 时）：需要检查 `hstatus.SPV`，记录 `htval`/`htinst`/`GVA`，处理 guest-page fault

---

### 模块 4：G-stage 页表管理 (`common/hyp/gstage_pt.c`)

**功能**：管理 `hgatp` 控制的 G-stage（第二阶段）地址翻译页表。

#### G-stage vs VS-stage 关键差异

| 特性 | VS-stage (`satp`/`vsatp`) | G-stage (`hgatp`) |
|------|--------------------------|-------------------|
| 根页表大小 | 4KB | **16KB** (x4) |
| 根页表对齐 | 4KB | **16KB** |
| GPA 宽度 | 虚拟地址宽度 | 虚拟地址宽度 **+2 bit** |
| U-bit 行为 | 正常 S/U 检查 | **始终作为 U-mode 检查** |
| G-bit | 正常全局页 | **忽略**, 留作未来使用 |
| Fault 类型 | page-fault (12/13/15) | **guest-page-fault** (20/21/23) |
| ID 字段 | ASID | **VMID** |
| 页表 scheme | Sv39/Sv48/Sv57 | Sv39**x4**/Sv48**x4**/Sv57**x4** |

#### G-stage 页表上下文

```c
typedef struct {
    int         mode;       /* HGATP_MODE_SV39X4 / SV48X4 / SV57X4 */
    uintptr_t  *root_pt;   /* 根页表物理地址 (16KB 对齐) */
    int         levels;     /* 页表级数 (3/4/5) */
    uintptr_t   map_base;
    uintptr_t   map_size;
    int         map_level;
} gpt_context_t;
```

#### 接口定义

```c
/**
 * gpt_init - 初始化 G-stage 页表上下文
 *
 * 从 G-stage 页表池分配 16KB 根页表（4 个连续 4KB 页）。
 */
void gpt_init(gpt_context_t *ctx, int mode);

/**
 * gpt_map_page - 映射单个 guest physical address → supervisor physical address
 *
 * 类似 pt_map_page, 但使用 G-stage 页表格式。
 * G-stage 中所有访问视为 U-mode, U-bit 总是被检查。
 */
int gpt_map_page(gpt_context_t *ctx, uintptr_t gpa, uintptr_t spa,
                 uintptr_t flags, int level);

/**
 * gpt_setup_identity_mapping - 创建 GPA = SPA 恒等映射
 *
 * 映射区域 [base, base+size) 使 GPA == SPA。
 * 注意：G-stage PTE 的 U-bit 必须设置为 1,
 * 因为 G-stage 翻译始终视为 U-mode 访问。
 */
int gpt_setup_identity_mapping(gpt_context_t *ctx, uintptr_t base,
                                uintptr_t size, uintptr_t flags, int level);

/**
 * gpt_pool_reset - 重置 G-stage 页表池
 *
 * G-stage 使用独立于 VS-stage 的页表池。
 */
void gpt_pool_reset(void);

/**
 * gpt_enable - 启用 G-stage 地址翻译
 *
 * 写入 hgatp CSR 并执行 hfence.gvma。
 */
void gpt_enable(gpt_context_t *ctx, unsigned vmid);

/**
 * gpt_disable - 禁用 G-stage 地址翻译
 *
 * 设置 hgatp.MODE=Bare。
 */
void gpt_disable(void);
```

#### 页表池设计

G-stage 根页表为 16KB（4 个 4KB 页），因此需要独立的页表池，且池中的根页表分配需要保证 16KB 对齐：

```
链接脚本新增段:
.gstage_page_tables (NOLOAD) : {
    PROVIDE(__gpt_pool_start = .);
    . = ALIGN(0x4000);         /* 16KB 对齐 */
    . += 128 * 4096;           /* 512KB pool */
    PROVIDE(__gpt_pool_end = .);
}
```

---

### 模块 5：两阶段地址翻译管理 (`common/hyp/two_stage.c`)

**功能**：组合管理 VS-stage + G-stage 两阶段翻译，提供高级 API。

#### 两阶段翻译上下文

```c
typedef struct {
    pt_context_t   vs_ctx;   /* VS-stage: vsatp 控制 */
    gpt_context_t  g_ctx;    /* G-stage: hgatp 控制 */
} two_stage_ctx_t;
```

#### 接口定义

```c
/**
 * two_stage_init - 初始化两阶段翻译
 *
 * @vs_mode: SATP_MODE_SV39/48/57 (VS-stage 翻译模式)
 * @g_mode:  HGATP_MODE_SV39X4/48X4/57X4 (G-stage 翻译模式)
 */
void two_stage_init(two_stage_ctx_t *ctx, int vs_mode, int g_mode);

/**
 * two_stage_setup_identity - 设置两阶段恒等映射
 *
 * 同时为 VS-stage 和 G-stage 创建 VA=GPA=SPA 的恒等映射。
 * VS-stage: VA → GPA (vsatp)
 * G-stage:  GPA → SPA (hgatp)
 */
int two_stage_setup_identity(two_stage_ctx_t *ctx, uintptr_t base,
                              uintptr_t size, uintptr_t flags, int level);

/**
 * two_stage_run_in_vs - 在 VS-mode + 两阶段翻译下执行函数
 *
 * 完整流程:
 * 1. 配置 PMP 允许 S/VS/VU-mode 访问
 * 2. 配置 trap 委托 (medeleg, hedeleg)
 * 3. 启用 G-stage 翻译 (写 hgatp)
 * 4. 启用 VS-stage 翻译 (写 vsatp)
 * 5. 切换到 VS-mode 执行 fn(arg)
 * 6. 返回后禁用翻译并恢复状态
 */
uintptr_t two_stage_run_in_vs(two_stage_ctx_t *ctx,
                               uintptr_t (*fn)(uintptr_t),
                               uintptr_t arg);

/**
 * two_stage_cleanup - 清理两阶段翻译状态
 */
void two_stage_cleanup(two_stage_ctx_t *ctx);
```

#### 地址翻译路径

```
V=1 时，内存访问的地址翻译过程：

  Guest Virtual Address (VA)
         |
         | VS-stage 翻译 (vsatp)
         | 页表遍历的中间物理地址也经过 G-stage
         ↓
  Guest Physical Address (GPA)
         |
         | G-stage 翻译 (hgatp)
         ↓
  Supervisor Physical Address (SPA)
         |
         | PMP 检查
         ↓
  Physical Memory Access
```

---

### 模块 6：Hypervisor CSR 操作 API (`common/hyp/hyp_csr.c`)

**功能**：提供 hypervisor CSR 的便捷读写和配置接口。

#### hstatus 字段操作

```c
void     hstatus_set_vtsr(bool enable);   /* VTSR: VS-mode SRET → virtual-inst trap */
void     hstatus_set_vtw(bool enable);    /* VTW: VS-mode WFI → virtual-inst trap */
void     hstatus_set_vtvm(bool enable);   /* VTVM: VS-mode SFENCE/satp → virtual-inst trap */
void     hstatus_set_hu(bool enable);     /* HU: 允许 U-mode 执行 HLV/HSV */
void     hstatus_set_spvp(unsigned priv); /* SPVP: HLV/HSV 的 effective privilege */
unsigned hstatus_get_spv(void);           /* SPV: trap 来源的 V bit */
unsigned hstatus_get_gva(void);           /* GVA: stval 是否为 guest VA */
```

#### Trap 委托操作

```c
/**
 * hyp_delegate_to_vs - 配置异常/中断委托到 VS-mode
 *
 * 先确保 medeleg/mideleg 将对应 trap 委托到 HS-mode,
 * 再通过 hedeleg/hideleg 进一步委托到 VS-mode。
 */
void hyp_delegate_to_vs(uintptr_t hedeleg_mask, uintptr_t hideleg_mask);

/**
 * hyp_undelegate - 清除所有 hypervisor 委托
 */
void hyp_undelegate(void);
```

#### 虚拟中断注入

```c
void hvip_set_vssi(bool pending);   /* VS software interrupt */
void hvip_set_vsti(bool pending);   /* VS timer interrupt */
void hvip_set_vsei(bool pending);   /* VS external interrupt */
```

#### 其他 CSR 控制

```c
/* hcounteren: 控制 VS/VU-mode 对硬件计数器的访问 */
void hcounteren_set(uint32_t mask);
void hcounteren_clear(uint32_t mask);

/* htimedelta: 设置 VS-mode 下 time CSR 的偏移值 */
void htimedelta_set(uint64_t delta);

/* henvcfg: 控制 V=1 时的执行环境特性 */
void henvcfg_set_pbmte(bool enable);  /* Svpbmt for VS-stage */
void henvcfg_set_adue(bool enable);   /* Hardware A/D update for VS-stage */
void henvcfg_set_stce(bool enable);   /* vstimecmp enable */
```

---

### 模块 7：HFENCE 指令封装 (`common/hyp/hyp_fence.c`)

**功能**：封装 HFENCE.VVMA 和 HFENCE.GVMA 指令。

```c
/**
 * hfence_vvma - 刷新 VS-stage TLB
 *
 * 类似 sfence.vma 但作用于 vsatp 控制的 VS-stage 翻译。
 * 仅在 M-mode 或 HS-mode 有效，在 VS/VU-mode 触发 virtual-instruction exception。
 *
 * @vaddr: 要刷新的 guest virtual address (0=全部)
 * @asid:  要刷新的 guest ASID (0=全部)
 */
void hfence_vvma(uintptr_t vaddr, uintptr_t asid);

/**
 * hfence_vvma_all - 全局 VS-stage TLB 刷新
 */
void hfence_vvma_all(void);

/**
 * hfence_gvma - 刷新 G-stage TLB
 *
 * 作用于 hgatp 控制的 G-stage 翻译。
 * 仅在 M-mode 或 HS-mode (mstatus.TVM=0) 有效。
 *
 * @gpa_shifted: guest physical address >> 2 (0=全部)
 * @vmid:        Virtual Machine ID (0=全部)
 */
void hfence_gvma(uintptr_t gpa_shifted, uintptr_t vmid);

/**
 * hfence_gvma_all - 全局 G-stage TLB 刷新
 */
void hfence_gvma_all(void);
```

---

### 模块 8：HLV/HLVX/HSV 指令封装 (`common/hyp/hyp_ldst.c`)

**功能**：封装 hypervisor 虚拟机 load/store 指令。

这些指令在 M-mode 或 HS-mode（以及 HU=1 时的 U-mode）有效，执行对 guest virtual memory 的访问，使用两阶段地址翻译，effective privilege 由 `hstatus.SPVP` 控制。

```c
/* HLV — 从 guest virtual memory 读取 */
uint8_t  hlv_b(uintptr_t addr);    /* 有符号 byte */
uint8_t  hlv_bu(uintptr_t addr);   /* 无符号 byte */
uint16_t hlv_h(uintptr_t addr);    /* 有符号 halfword */
uint16_t hlv_hu(uintptr_t addr);   /* 无符号 halfword */
uint32_t hlv_w(uintptr_t addr);    /* 有符号 word */
uint64_t hlv_d(uintptr_t addr);    /* doubleword (RV64 only) */

/* HLVX — 按 execute 权限读取 (用于模拟指令获取) */
uint16_t hlvx_hu(uintptr_t addr);  /* 无符号 halfword, execute perm */
uint32_t hlvx_wu(uintptr_t addr);  /* 无符号 word, execute perm */

/* HSV — 向 guest virtual memory 写入 */
void hsv_b(uintptr_t addr, uint8_t val);
void hsv_h(uintptr_t addr, uint16_t val);
void hsv_w(uintptr_t addr, uint32_t val);
void hsv_d(uintptr_t addr, uint64_t val);  /* RV64 only */
```

#### 使用约束

- 在 V=1 时执行 HLV/HLVX/HSV 触发 **virtual-instruction exception**
- 在 U-mode 且 `hstatus.HU=0` 时触发 **illegal-instruction exception**
- effective privilege: VU (SPVP=0) 或 VS (SPVP=1)
- 地址翻译: 始终使用两阶段翻译 (vsatp + hgatp)
- HS-level `sstatus.SUM` 被忽略
- HS-level `sstatus.MXR` 影响两个阶段; `vsstatus.MXR` 仅影响 VS-stage

---

### 模块 9：Hypervisor 测试状态管理 (`common/hyp/hyp_reset.c`)

**功能**：hypervisor 测试的状态重置。

```c
/**
 * hyp_reset_state - 重置所有 hypervisor 状态到干净基线
 *
 * 重置内容:
 * 1. 确保回到 M-mode, V=0
 * 2. 清除 hstatus (VTSR/VTW/VTVM/HU/SPV/SPVP/GVA)
 * 3. 清除 hedeleg, hideleg
 * 4. 清除 hvip (所有虚拟中断 pending)
 * 5. 清除 hie
 * 6. 设置 hgatp = Bare (禁用 G-stage)
 * 7. 清除 htval, htinst
 * 8. 清除 VS CSR (vsstatus/vstvec/vsepc/vscause/vstval/vsatp)
 * 9. 清除 henvcfg, hcounteren, htimedelta
 * 10. 执行 hfence.gvma 全局刷新
 * 11. 调用 reset_state() 处理基础重置
 */
void hyp_reset_state(void);
```

---

### 模块 10：Hypervisor 测试宏 (`common/hyp/hyp_test.h`)

**功能**：提供 hypervisor 特有的测试断言和辅助宏。

```c
/**
 * EXPECT_VIRTUAL_INST - 执行 stmt, 预期触发 virtual-instruction exception
 *
 * 用于验证 VTSR/VTW/VTVM 等控制字段的行为。
 */
#define EXPECT_VIRTUAL_INST(stmt) do { \
    trap_expect_begin(); \
    stmt; \
    TEST_ASSERT("virtual-inst trap triggered", trap_was_triggered()); \
    if (trap_was_triggered()) { \
        TEST_ASSERT_EQ("cause=22", trap_get_cause(), CAUSE_VIRTUAL_INSTRUCTION); \
    } \
    trap_expect_end(); \
} while (0)

/**
 * EXPECT_GUEST_PAGE_FAULT - 执行 stmt, 预期触发 guest-page fault
 */
#define EXPECT_GUEST_PAGE_FAULT(expected_cause, stmt) do { \
    trap_expect_begin(); \
    stmt; \
    TEST_ASSERT("guest-page-fault triggered", trap_was_triggered()); \
    if (trap_was_triggered()) { \
        TEST_ASSERT_EQ("cause matches", trap_get_cause(), (expected_cause)); \
    } \
    trap_expect_end(); \
} while (0)

/**
 * VS_EXPECT_NO_TRAP - 在 VS-mode 执行 stmt, 预期无 trap
 */
#define VS_EXPECT_NO_TRAP(stmt) do { \
    trap_expect_begin(); \
    stmt; \
    TEST_ASSERT("no trap in VS-mode", !trap_was_triggered()); \
    trap_expect_end(); \
} while (0)

/**
 * CHECK_HTVAL - 检查 htval 值 (guest physical address >> 2)
 */
#define CHECK_HTVAL(msg, expected_gpa_shifted) do { \
    TEST_ASSERT_EQ(msg, trap_get_htval(), (uintptr_t)(expected_gpa_shifted)); \
} while (0)

/**
 * CHECK_HTINST - 检查 htinst 伪指令值
 */
#define CHECK_HTINST(msg, expected_pseudoinst) do { \
    TEST_ASSERT_EQ(msg, trap_get_htinst(), (uintptr_t)(expected_pseudoinst)); \
} while (0)

/**
 * CHECK_GVA - 检查 hstatus.GVA
 */
#define CHECK_GVA(msg, expected_gva) do { \
    TEST_ASSERT_EQ(msg, trap_get_gva(), (bool)(expected_gva)); \
} while (0)

/**
 * HYP_TEST_END - TEST_END 的 hypervisor 版本
 *
 * 使用 hyp_reset_state 代替 reset_state。
 */
#define HYP_TEST_END() do { \
    goto_priv(PRIV_M); \
    hyp_reset_state(); \
    if (test_results.current_test_failed) { \
        test_results.tests_failed++; \
        if (test_results.failed_count < MAX_FAILED_TESTS) { \
            test_results.failed_names[test_results.failed_count++] = \
                test_results.current_test_name; \
        } \
        printf("[FAIL] %s\n\n", test_results.current_test_name); \
    } else { \
        test_results.tests_passed++; \
        printf("[PASS] %s\n\n", test_results.current_test_name); \
    } \
    return !test_results.current_test_failed; \
} while (0)
```

---

## 对现有 common 模块的修改

### encoding.h

新增 hypervisor CSR 地址（或 include `hyp_defs.h`）：

```c
/* Hypervisor CSRs */
#define CSR_HSTATUS     0x600
#define CSR_HEDELEG     0x602
#define CSR_HIDELEG     0x603
#define CSR_HGATP       0x680
/* ... 全部 hypervisor CSR ... */

/* Hypervisor cause codes */
#define CAUSE_ECALL_FROM_VS             10
#define CAUSE_INST_GUEST_PAGE_FAULT     20
#define CAUSE_LOAD_GUEST_PAGE_FAULT     21
#define CAUSE_VIRTUAL_INSTRUCTION       22
#define CAUSE_STORE_GUEST_PAGE_FAULT    23
```

### csr_accessors.c

新增所有 hypervisor CSR 和 VS CSR 的 `csr_read`/`csr_write` switch case：

```c
/* 新增: Hypervisor CSRs */
_CSR_READ_CASE(0x600)  /* hstatus */
_CSR_READ_CASE(0x602)  /* hedeleg */
_CSR_READ_CASE(0x603)  /* hideleg */
_CSR_READ_CASE(0x680)  /* hgatp */
/* ... */

/* 新增: VS CSRs */
_CSR_READ_CASE(0x200)  /* vsstatus */
_CSR_READ_CASE(0x280)  /* vsatp */
/* ... */
```

### trap.c

扩展 `m_trap_handler` 和 `s_trap_handler`：

1. `m_trap_handler`: 识别 `CAUSE_ECALL_FROM_VS` (10)
2. `s_trap_handler`（作为 HS-mode handler）: 检查 `hstatus.SPV`, 记录 `htval`/`htinst`/`GVA`

### trap_asm.S

可能需要新增 VS-mode trap entry point（如果 trap 委托到 VS-mode 测试场景需要）。

### test_framework.h

新增虚拟化特权级定义：

```c
#define PRIV_VS  5   /* Virtual Supervisor mode (V=1, S) */
#define PRIV_VU  4   /* Virtual User mode (V=1, U) */
```

### Makefile.common

套件 Makefile 设 `ENABLE_HYP=1` 启用 H 扩展：链接 `common/hyp/` 目标、向 CFLAGS 与 ASFLAGS 注入 `-DENABLE_HYP -I$(HYP_DIR)`、并把 MARCH 重建为 `rvXXimach<扩展后缀>`（加入 `h`，保留套件已声明的扩展后缀）。`ENABLE_TWO_STAGE=1` 额外链接两阶段助手 `two_stage_helpers.o`。

```makefile
ifdef ENABLE_HYP
HYP_DIR  = $(COMMON_DIR)/hyp
HYP_OBJS = \
    $(HYP_DIR)/hyp_csr.o       $(HYP_DIR)/hyp_priv.o \
    $(HYP_DIR)/hyp_trap.o      $(HYP_DIR)/hyp_trap_asm.o \
    $(HYP_DIR)/hyp_fence.o     $(HYP_DIR)/hyp_ldst.o \
    $(HYP_DIR)/hyp_reset.o     $(HYP_DIR)/gstage_pt.o \
    $(HYP_DIR)/two_stage.o     $(HYP_DIR)/test_vs_helpers.o \
    $(HYP_DIR)/hyp_vs_trap.o   $(HYP_DIR)/hyp_test_helpers.o
ifdef ENABLE_TWO_STAGE
HYP_OBJS += $(HYP_DIR)/two_stage_helpers.o
endif
COMMON_OBJS += $(HYP_OBJS)
endif
```

分页模式相关的 `SUITE_SATP_MODE` / `SUITE_VSATP_MODE` / `SUITE_HGATP_MODE` 注入与命令行旋钮（`SATP_MODE` / `HGATP_MODE` / `VSATP_MODE`）见「分页模式配置机制」一节。

---

## 测试覆盖的核心验证领域

| 领域 | 涉及模块 | 关键测试点 |
|------|---------|-----------|
| **CSR 访问控制** | hyp_csr | hstatus/hedeleg/hideleg/hgatp 等 CSR 在 M/HS/VS/VU 各模式下的访问权限和 virtual-instruction exception |
| **特权级切换** | hyp_priv | M→HS→VS→VU 双向切换, V bit 正确设置, SPV/SPP 正确更新 |
| **Virtual-instruction exception** | hyp_trap | VTSR (VS-mode SRET trap), VTW (VS-mode WFI trap), VTVM (VS-mode SFENCE/satp trap), VS/VU-mode 访问 hypervisor CSR/VS CSR |
| **Trap 委托** | hyp_csr, hyp_trap | medeleg→hedeleg 两级委托, VS-mode ecall (cause=10) vs HS-mode ecall (cause=9) 区分 |
| **G-stage 页表翻译** | gstage_pt | Sv39x4/48x4/57x4 翻译正确性, 16KB 根页表, guest-page fault (cause=20/21/23), U-bit 始终检查 |
| **两阶段地址翻译** | two_stage | VA→GPA→SPA 全链路验证, VS-stage + G-stage 权限交互, VS-stage 页表遍历中的 G-stage fault |
| **HLV/HLVX/HSV** | hyp_ldst | HS-mode/U-mode(HU=1) 下的虚拟机 load/store, SPVP 控制的 effective privilege, V=1 时触发 virtual-instruction exception |
| **HFENCE 指令** | hyp_fence | HFENCE.VVMA/GVMA 的 TLB 刷新效果, VS/VU-mode 执行时的 virtual-instruction exception |
| **虚拟中断注入** | hyp_csr | hvip 注入 VSSI/VSTI/VSEI, hideleg 中断委托到 VS-mode, 中断号翻译 (10→9, 6→5, 2→1) |
| **henvcfg 控制** | hyp_csr | PBMTE (Svpbmt for VS-stage), ADUE (hardware A/D update), STCE (vstimecmp enable) 对 VS-mode 的影响 |
| **hcounteren** | hyp_csr | 计数器访问控制, V=1 时 virtual-instruction exception 触发条件 |
| **htimedelta** | hyp_csr | VS/VU-mode 下 time CSR 读取 = time + htimedelta |
| **htval/htinst** | hyp_trap | guest-page fault 时 htval (GPA>>2), htinst transformed instruction / pseudoinstruction |
| **mstatus 扩展** | hyp_csr | MPV/GVA 字段, MPRV + MPV 交互 (两阶段翻译 as-if V=1) |

---

## 测试用例编写模式

### 基本模式 (无 VM)

```c
#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_test.h"

TEST_REGISTER(test_vtsr_traps_sret_in_vs);
bool test_vtsr_traps_sret_in_vs(void) {
    TEST_BEGIN("VTSR-01: SRET in VS-mode triggers virtual-inst when VTSR=1");

    /* 设置 VTSR=1 */
    hstatus_set_vtsr(true);

    /* 在 VS-mode 执行 SRET, 预期 virtual-instruction exception */
    goto_vs_mode();
    EXPECT_VIRTUAL_INST(asm volatile("sret"));
    goto_priv(PRIV_M);

    HYP_TEST_END();
}
```

### 两阶段翻译模式

```c
#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_test.h"
#include "vm/vm.h"

TEST_REGISTER(test_two_stage_identity_mapping);
bool test_two_stage_identity_mapping(void) {
    TEST_BEGIN("2STAGE-01: Two-stage identity mapping read/write");

    REQUIRE_VSATP_MODE(SUITE_VSATP_MODE);
    REQUIRE_HGATP_MODE(SUITE_HGATP_MODE);

    two_stage_ctx_t ctx;
    two_stage_init(&ctx, SUITE_VSATP_MODE, SUITE_HGATP_MODE);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D;
    int ret = two_stage_setup_identity(&ctx, base, PAGE_SIZE_1G,
                                        flags, PT_LEVEL_1G);
    TEST_ASSERT("identity mapping setup", ret == 0);

    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_read_write,
                                            (uintptr_t)test_data);
    TEST_ASSERT("VS-mode read/write succeeds", result == 0);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

### HLV/HSV 测试模式

```c
TEST_REGISTER(test_hlv_in_hs_mode);
bool test_hlv_in_hs_mode(void) {
    TEST_BEGIN("HLV-01: HLV.W reads guest memory from HS-mode");

    /* 设置 G-stage 映射 */
    gpt_context_t g_ctx;
    gpt_pool_reset();
    gpt_init(&g_ctx, SUITE_HGATP_MODE);
    /* ... setup mapping ... */
    gpt_enable(&g_ctx, 0);

    /* 设置 SPVP=1 (VS-level access) */
    hstatus_set_spvp(1);

    /* 从 HS-mode 使用 HLV.W 读取 guest memory */
    goto_priv(PRIV_S);  /* HS-mode */
    uint32_t val = hlv_w(test_addr);
    goto_priv(PRIV_M);

    TEST_ASSERT_EQ("HLV read correct value", val, expected_val);

    gpt_disable();
    gpt_pool_reset();
    HYP_TEST_END();
}
```

---

## 关键设计决策和注意事项

### 1. 特权级编码

现有框架使用 `PRIV_U=0, PRIV_S=1, PRIV_M=3` 直接对应 RISC-V 的 nominal privilege。Hypervisor 扩展新增的 VS/VU 模式需要额外的编码空间来区分 V=0 和 V=1。建议使用高位标记 V bit: `PRIV_VS = 5 (V=1|S=1)`, `PRIV_VU = 4 (V=1|U=0)`。

### 2. G-stage 页表池

G-stage 根页表为 16KB（4 个连续 4KB 页），需要：
- 独立于 VS-stage 的页表池（两者可能同时使用）
- 池起始地址 16KB 对齐
- 根页表分配必须返回 16KB 对齐地址

### 3. Trap Handler 分层

```
M-mode handler (最高优先级, 处理未委托的 trap):
  └── HS-mode handler (处理 medeleg 委托的 trap):
       └── VS-mode handler (处理 hedeleg 委托的 trap)
```

测试框架需要能配置每一层的委托，并在正确的层捕获 trap 信息。

### 4. 世界切换 (World Switch) 顺序

根据规范，切换 VMID 时必须遵循以下顺序防止推测执行污染 TLB：

```
1. vsatp = 0          (先清零 VS-stage 翻译)
2. hgatp = new value  (切换 G-stage 和 VMID)
3. vsatp = new value  (恢复 VS-stage 翻译)
```

### 5. QEMU 平台支持

QEMU virt 平台需要 `-cpu rv64,h=true`（或更新版本默认启用 H 扩展）才能运行 hypervisor 测试。Makefile 需要相应调整 QEMU_OPTS。

---

## 分页模式配置机制

虚拟化测试的 VS 阶段与 G 阶段分页模式在编译期由三层协作确定。

### 平台能力：PLATFORM_SATP_MODE / PLATFORM_HGATP_MODE

`common/capabilities.h` 从平台 config（`config/<platform>/rvtest_config.h` 的 `SV39/48/57_SUPPORTED`）派生 `SV39/48/57_AVAILABLE`，再按“最小优先”（Sv39>Sv48>Sv57）选出平台可用的默认分页模式：

- `PLATFORM_SATP_MODE`：S/VS 阶段模式（Bare=0 / Sv39=8 / Sv48=9 / Sv57=10；rv32 固定 Sv32=1）。
- `PLATFORM_HGATP_MODE`：与之配对的 G 阶段模式（Sv39x4=8 / Sv48x4=9 / Sv57x4=10）。

`PLATFORM_*` 纯由平台 config 派生、**永不被强制覆盖**，代表“该平台能用的模式”。

### 套件选择：SUITE_SATP_MODE / SUITE_VSATP_MODE / SUITE_HGATP_MODE

每个套件实际运行的模式用 `SUITE_*` 表示，与平台能力解耦：

- 单阶段（非 Hypervisor）套件：`SUITE_SATP_MODE`（S 阶段）。
- 两阶段（Hypervisor / Sv\*x4）套件：`SUITE_VSATP_MODE`（VS 阶段）+ `SUITE_HGATP_MODE`（G 阶段）。

约定：
- **模式无关套件**：Makefile 里写 `SUITE_* ?= PLATFORM_*`，跟随平台。
- **模式内禀套件**（如 `Sv39x4_Sv48` 专测某模式对）：Makefile 里写 `-DSUITE_HGATP_MODE=HGATP_MODE_SV39X4 -DSUITE_VSATP_MODE=SATP_MODE_SV48` 钉死具体模式。

调用点（`REQUIRE_*_MODE` 门控、`two_stage_init` / `ts2_setup_*` 初始化）一律使用 `SUITE_*`，不直接写 `PLATFORM_*` 或具体模式字面量。

### 命令行覆盖与优先级

`common/Makefile.common` 提供三个诊断旋钮，构建时临时改写套件模式：

```
make SATP_MODE=sv39|sv48|sv57     # 单阶段套件   -> SUITE_SATP_MODE
make HGATP_MODE=sv39|sv48|sv57    # 两阶段 G 阶段 -> SUITE_HGATP_MODE
make VSATP_MODE=sv39|sv48|sv57    # 两阶段 VS 阶段 -> SUITE_VSATP_MODE
```

优先级：`命令行旋钮 > 套件 Makefile 的 SUITE_* ?= 默认 > 头文件 #ifndef 兜底`。两阶段兜底在 `common/hyp/two_stage_helpers.h`（`#ifndef SUITE_HGATP_MODE → PLATFORM_HGATP_MODE`、`#ifndef SUITE_VSATP_MODE → PLATFORM_SATP_MODE`）。

> 兜底只在 include 了该头的编译单元内生效；含独立编译单元的套件（如单独的 `tests/test_helpers.o`）必须靠 Makefile 的全局 `-DSUITE_*`，故 `SUITE_* ?= PLATFORM_*` 不可省。

### 分页模式门控宏（REQUIRE_*_MODE）

`common/hyp/hyp_test.h` 提供编译期门控：`mode` 为架构 MODE 编码，是否支持取自 config 声明的 `SV39/48/57_AVAILABLE`，不做运行时 WARL 探测。

```c
#define PAGING_MODE_UNAVAILABLE_(mode) ( \
    ((mode) == 0)                     || \
    ((mode) == 8  && !SV39_AVAILABLE) || \
    ((mode) == 9  && !SV48_AVAILABLE) || \
    ((mode) == 10 && !SV57_AVAILABLE))

#define REQUIRE_HGATP_MODE(mode)  do { if (PAGING_MODE_UNAVAILABLE_(mode)) TEST_SKIP("hgatp mode not declared by platform config"); } while (0)
#define REQUIRE_SATP_MODE(mode)   do { if (PAGING_MODE_UNAVAILABLE_(mode)) TEST_SKIP("satp mode not declared by platform config"); } while (0)
#define REQUIRE_VSATP_MODE(mode)  do { if (PAGING_MODE_UNAVAILABLE_(mode)) TEST_SKIP("vsatp mode not declared by platform config"); } while (0)
```

以特定模式 WARL 行为为测试对象的用例（如 `Shvsatpa` / `Shgatpa` / `Shtvala/test_htval_modes.c`）直接调用 `hgatp_supports_mode()` / `satp_supports_mode()` / `vsatp_supports_mode()`（`common/hyp/hyp_csr.h`）。

## 两阶段翻译测试 API

### 核心（`common/hyp/two_stage.h`）

```c
void      two_stage_init(two_stage_ctx_t *ctx, int vs_mode, int g_mode);
void      two_stage_enable(two_stage_ctx_t *ctx, unsigned vmid);
uintptr_t two_stage_run_in_vs(two_stage_ctx_t *ctx, uintptr_t (*fn)(uintptr_t), uintptr_t arg);
uintptr_t two_stage_run_in_vu(two_stage_ctx_t *ctx, uintptr_t (*fn)(uintptr_t), uintptr_t arg);
void      two_stage_cleanup(two_stage_ctx_t *ctx);
int       two_stage_vs_map(...);   int two_stage_vs_identity(...);
uintptr_t two_stage_vs_pt_page_addr(...);
```

### 场景搭建助手（`common/hyp/two_stage_helpers.h`）

```c
void ts2_setup_full(ctx, vs_mode, g_mode);                          /* VS+G 全恒等映射 */
void ts2_setup_full_u(ctx, vs_mode, g_mode);                        /* VS 叶子带 U=1（VU 访问）*/
void ts2_setup_with_g_victim(ctx, vs_mode, g_mode, gva, g_flags);   /* G 阶段 victim 页 */
void ts2_setup_with_vs_victim(ctx, vs_mode, g_mode, va, vs_flags);  /* VS 阶段 victim 页 */
void ts2_setup_with_dual_victim(ctx, vs_mode, g_mode, va, vs_flags, g_flags);
void ts2_setup_non_identity(ctx, vs_mode, g_mode, va, gpa, spa, vs_flags, g_flags);
void ts2_setup_granular(...);
uintptr_t ts2_invalidate_vs_pt_in_g(ctx, va, level);   /* 制造 implicit VS-walk fault */
bool      ts2_run_check_fault(ctx, fn, arg, exp_cause);
uintptr_t ts2_run_check_no_fault(ctx, fn, arg);
void      ts2_finish(ctx);
void      ts2_enable_adue / ts2_disable_adue / ts2_enable_pbmte / ts2_disable_pbmte(void);
```

`vs_mode` / `g_mode` 实参统一传 `SUITE_VSATP_MODE` / `SUITE_HGATP_MODE`；模式内禀用例传具体 `SATP_MODE_*` / `HGATP_MODE_*`，Bare 场景传 `SATP_MODE_BARE` / `HGATP_MODE_BARE`。

### 委托与 VS-mode 触发器

- 委托（`common/hyp/hyp_csr.h`）：`hedeleg_write()` / `hedeleg_read()` / `hideleg_write()`、`hyp_delegate_to_vs(cause_mask, ...)`（medeleg+hedeleg 双层委托到 VS）。
- VS-mode 触发器（`common/hyp/test_vs_helpers.h`）：`test_vs_read_write`、`test_vs_load` / `test_vs_store`、`test_vs_load_expect_fault`（cause 21）/ `test_vs_store_expect_fault`（23）/ `test_vs_exec_expect_fault`（20）。作为 `two_stage_run_in_vs/vu` 或 `ts2_run_check_*` 的回调使用。

### trap 字段断言宏（`common/hyp/hyp_test.h`）

- htval / GVA：`CHECK_HTVAL(msg, gpa>>2)`、`CHECK_GVA(msg, expected)`。
- VS-mode trap 记录（Shvstvala / Shvstvecd 用）：`CHECK_VSTVAL` / `CHECK_VSTVAL_NONZERO` / `CHECK_VSTVAL_ZERO`、`CHECK_VS_TRAP_CAUSE` / `CHECK_VS_TRAP_TVAL` / `CHECK_VS_TRAP_VECTORED_ENTRY`、`CHECK_IMPLICIT_FAULT_REPORT`。
- 异常预期：`EXPECT_VIRTUAL_INST`（cause 22）、`EXPECT_ILLEGAL_INST`（2）、`EXPECT_ILLEGAL_OR_VIRTUAL_INST`、`EXPECT_GUEST_PAGE_FAULT(cause, stmt)`、`EXPECT_COUNTER_TRAP`、`VS_EXPECT_NO_TRAP`。
- 收尾：`HYP_TEST_END()`（以 `hyp_reset_state()` 代替 `reset_state()`）。

### CSR WARL 探测（`common/hyp/hyp_csr.h`）

```c
uintptr_t csr_warl_probe(unsigned csr_num, uintptr_t value);  /* 写 value、读回实际保留位 */
```

分页模式支持探测用 `hgatp_supports_mode()` / `satp_supports_mode()` / `vsatp_supports_mode()`；VMID/ASID 宽度用 `hgatp_vmid_width()` / `satp_asid_width()` / `vsatp_asid_width()`。

## H 子扩展与交叉测试套件

- **H 子扩展套件**（`Sha/`、`Shtvala/`、`Shvstvala/`、`Shvstvecd/`、`Shvsatpa/`、`Shgatpa/`、`Shcounterenw/`、`Shlcofideleg/`）：验证各 H 子扩展对 H 基线的 normative 收紧。`Shvsatpa` / `Shgatpa` 为模式内禀（逐模式探测 vsatp/hgatp 支持）；其余模式无关（`SUITE_* ?= PLATFORM_*`）。
- **Hypervisor × 其他扩展交叉套件**（`Hypervisor_Zaamo/`、`Zabha/`、`Zacas/`、`Zalasr/`、`Zalrsc/`、`Zca/`、`Zicbom/`、`Zicbop/`、`Zicboz/`、`Zicfilp/`、`Zicfiss/`、`Zihintntl/`、`Ssnpm/`、`Smmpm/`、`Smnpm/`、`Ssccptr/`、`Svadu/`、`Svinval/`、`Svnapot/`、`Svpbmt/`、`Sstvala/`、`CSR/`、`Exceptions/`、`PMP/` 等）：验证 H 与原子 / 压缩 / CMO / CFI / PM / 分页等扩展的交互，均为模式无关。
- **两阶段模式对套件**（`Sv39x4/`、`Sv48x4/`、`Sv57x4/` 及 `Sv*x4_Sv*` 9 个组合）：模式内禀，Makefile 钉死 `SUITE_HGATP_MODE` / `SUITE_VSATP_MODE`。9 个 `Sv*x4_Sv*` 组合套件通过 symlink 共享 `Sv39x4_Sv39/tests/` 源码，仅 Makefile 的 `-DSUITE_*` 与 `main.c` banner 不同。

每个套件一个 `Makefile`，`include ../common/Makefile.common`，打开 `ENABLE_HYP=1`（两阶段再加 `ENABLE_TWO_STAGE=1`，按需 `ENABLE_VM` / `ENABLE_PMP`），并声明 `SUITE_*`。目录名用首字母大写形式（如 `Shtvala/`），与 `Sv*` / `Ss*` / `Sm*` 一致。

---

## Hypervisor 基线测试套件（`Hypervisor_CSR/`、`Hypervisor_Interrupts/`、`Hypervisor_Exceptions/`）

### 概述

原 `Hypervisor/` 综合基线套件已拆分为三个独立子集（原目录与 `Hypervisor_test_plan.md` 已删除），分别对应 `Hypervisor_CSR_test_plan.md`（9 组 / 128 用例）、`Hypervisor_Interrupts_test_plan.md`（4 组 / 37 用例）、`Hypervisor_Exceptions_test_plan.md`（8 组 / 115 用例），合计 20 组 / 280 用例，用例编号与原方案保持一致。共用辅助代码位于 `common/hyp/hyp_test_helpers.{c,h}`；其中依赖 `__vm_test_region` 链接区域的 G-stage fault 辅助位于 `common/hyp/hyp_test_helpers_region.c`，由各子集通过 EXT_OBJS 链接。

### 目录结构

```
Hypervisor_CSR/                 # Hypervisor_Interrupts/、Hypervisor_Exceptions/ 结构相同
├── Makefile                    # ENABLE_HYP=1, ENABLE_VM=1
├── kernel.ld                   # .test_table / .vm_test_region / .gpt_page_tables
├── main.c                      # 遍历 _test_table[] 执行所有测试
└── tests/
    ├── test_register.c         # #include 所有测试文件，触发 TEST_REGISTER
    └── test_xxx.c              # 按 Group 划分的测试文件
```

### VS/VU-mode Trampoline 设计

由于测试在 M-mode 下运行，需要通过 trampoline 函数在 VS/VU-mode 下执行特定操作。`common/hyp/hyp_test_helpers.c` 提供了一组 trampoline 函数，每个函数签名为 `uintptr_t fn(uintptr_t arg)`，通过 `run_in_vs_mode(fn, arg)` 或 `run_in_vu_mode(fn, arg)` 调用。

**Trampoline 函数分类**：

| 类别 | 函数 | 用途 |
|------|------|------|
| S CSR 读取 | `vs_read_sstatus`, `vs_read_sie`, `vs_read_sip`, `vs_read_satp` 等 | V=1 时访问 S CSR（实际映射到 VS CSR） |
| S CSR 写入 | `vs_write_sstatus`, `vs_write_satp`, `vs_write_sie` 等 | V=1 时写入 S CSR |
| H CSR 读取 | `vs_read_hstatus`, `vs_read_hedeleg`, `vs_read_hgatp` | VS-mode 访问 H CSR（应触发 virtual-inst） |
| VS CSR 直接 | `vs_read_vsstatus_direct` | VS-mode 直接访问 vsstatus（应触发 virtual-inst） |
| 特权指令 | `vs_exec_sret`, `vs_exec_wfi`, `vs_exec_sfence_vma`, `vs_exec_sinval_vma` | 测试 VTSR/VTW/VTVM 控制 |
| HLV/HSV | `vs_exec_hlv_w`, `vs_exec_hsv_w`, `vs_exec_hlvx_wu` | VS/VU-mode 执行虚拟机 load/store |
| HFENCE | `vs_exec_hfence_vvma`, `vs_exec_hfence_gvma` | VS-mode 执行 HFENCE（应触发 virtual-inst） |
| Ecall | `vs_exec_ecall`, `vs_exec_ebreak` | 触发 ecall/ebreak 异常 |
| Counter | `vs_read_cycle`, `vs_read_time`, `vs_read_instret` | 访问 counter CSR |

### Trap Entry/Return 测试验证策略

Trap entry 测试（TENT-01~15）通过以下方式验证 CSR 自动写入：

1. **trap_expect_begin()** 标记开始期望 trap
2. **run_in_vs_mode/run_in_vu_mode** 触发 trap（如 ecall）
3. **trap_was_triggered()** 确认 trap 发生
4. **trap_get_cause/spv/gva/htval/htinst()** 读取 trap 记录字段
5. **hstatus_read()** 检查 SPV/SPVP/GVA 等位域

Trap return 测试（TRET-01~15）利用 `run_in_vs_mode`/`run_in_vu_mode` 底层的 MRET/SRET 机制，验证模式切换的正确性。

### 中断注入测试实现方案

中断测试（HINT/HGEI/VSIE 章节）主要验证 CSR 读写行为和 alias 关系：

- **hvip 注入**：通过 `hvip_set_vssi/vsti/vsei()` 设置 pending 位，通过内联 asm 读取 `hip`（0x644）验证 alias
- **hie/hip 访问**：框架未提供 `hip_read/hie_write` 等函数，测试中使用内联 asm 直接访问 CSR
- **hideleg 控制**：通过 `hideleg_write/read()` 验证委托位的可写性
- **hgeip/hgeie**：通过内联 asm 访问 CSR 0xE12/0x607

> [!NOTE]
> 部分复杂场景（如中断优先级仲裁、vstimecmp 真实定时器中断、MPRV+MPV 两阶段翻译）在基线测试中简化为 CSR 读写验证或 TEST_SKIP，后续可在集成测试中补充。

### 编译与运行

```bash
cd <套件目录>                 # 如 Hypervisor_CSR/、Shtvala/、Sv39x4_Sv39/
make clean                    # 切换 CONFIG 或模式旋钮前必须先 clean（common/*.o 跨套件共享）
make                          # 编译（默认 CONFIG=qemu-rv64-max）
make qemu                     # 在 QEMU 上运行
make spike                    # 在 Spike 上运行
make sail                     # 在 Sail 上运行

make CONFIG=ngf_c9502 qemu    # 指定平台配置

# 诊断旋钮：临时改写套件分页模式（详见「分页模式配置机制」）
make HGATP_MODE=sv48 VSATP_MODE=sv48 qemu    # 两阶段套件的 G / VS 阶段
make SATP_MODE=sv57 qemu                      # 单阶段套件的 S 阶段
```

QEMU 以 `-cpu max` 启动（rv64 + H + Sv39/48/57）。

---

## 参考

- RISC-V Privileged Specification — Hypervisor Extension (`SPEC/hypervisor.adoc`)
- `docs/vm_test_framework.md` — 现有 VM 测试框架文档
- `docs/vm_test_plan.md` — VM 测试计划（可参考其测试组织方式）
- `common/test_framework.h` — 现有测试框架 API
- `common/vm/vm.h` — 现有 VM 管理 API
- `common/privilege.c` — 现有特权级切换实现
- `common/trap.c` — 现有 trap handler 实现
