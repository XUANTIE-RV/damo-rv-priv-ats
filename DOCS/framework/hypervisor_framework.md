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
├── hyp_defs.h        # CSR 地址、位域、cause code、hgatp 编码
├── hyp_csr.c         # CSR 便捷操作 API
├── hyp_priv.c        # 虚拟化特权级切换 (HS→VS→VU)
├── hyp_trap.c        # HS-mode trap handler (处理 VS/VU-mode trap)
├── hyp_trap_asm.S    # HS-mode trap entry (汇编)
├── hyp_fence.c       # HFENCE.VVMA / HFENCE.GVMA 封装
├── hyp_ldst.c        # HLV / HLVX / HSV 指令封装
├── gstage_pt.c       # G-stage 页表管理 (hgatp, Sv39x4/48x4/57x4)
├── two_stage.c       # 两阶段翻译组合管理
├── hyp_reset.c       # Hypervisor 状态重置
└── hyp_test.h        # Hypervisor 测试宏

hypervisor/              # 测试用例目录
├── Makefile
├── kernel.ld
└── main.c
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

新增 `ENABLE_HYP` 条件编译支持：

```makefile
ifdef ENABLE_HYP
HYP_DIR = $(COMMON_DIR)/hyp
HYP_OBJS = $(HYP_DIR)/hyp_csr.o $(HYP_DIR)/hyp_priv.o \
            $(HYP_DIR)/hyp_trap.o $(HYP_DIR)/hyp_trap_asm.o \
            $(HYP_DIR)/hyp_fence.o $(HYP_DIR)/hyp_ldst.o \
            $(HYP_DIR)/gstage_pt.o $(HYP_DIR)/two_stage.o \
            $(HYP_DIR)/hyp_reset.o
COMMON_OBJS += $(HYP_OBJS)
CFLAGS += -DENABLE_HYP -I$(HYP_DIR)
MARCH = rv64imac_zicsr_zifencei_h   # 添加 H 扩展
endif
```

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

    two_stage_ctx_t ctx;
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

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
    gpt_init(&g_ctx, HGATP_MODE_SV39X4);
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

## H 子扩展通用测试基建（v2 增量）

### 设计动机

`Sha / Shtvala / Shvstvala / Shvstvecd / Shvsatpa / Shgatpa / Shcounterenw / Shlcofideleg` 这 8 个 H 子扩展从规范上看几乎全部都是"在 H 基线上，把某个原本可以由实现自由选择的写法收紧为强制"，对测试框架的诉求高度同构：
- 平台是否实现该子扩展 → 决定整套用例是 SKIP 还是 RUN；
- 触发某种 trap 路径 → 复用 H 基线的 VS-mode 触发 + 委托配置；
- 检查 trap 入口瞬间的 CSR 字段（htval / vstval / vsepc / hstatus.GVA / htinst …） → 复用 trap_record；
- 实现宽度 / MODE 子集 → 通过 WARL 探测获取。

为避免每接入一个子扩展就在 `common/hyp/` 里堆一批以扩展命名的 API，本节定义**按"行为语义"而非"扩展名"**命名的通用基建。

> **设计原则**：通用层只放"H 基线本就该有"的能力；子扩展层只放"该扩展独有的 normative 收紧"。新接入子扩展的工作量约束在每扩展 5–30 行 header。

### 模块 11：平台能力矩阵（`common/hyp/platform_caps.h` / `.c`）

```c
typedef struct {
    /* CSR 实现宽度（WARL 探测得到，0 表示未探测）*/
    uint8_t  htval_width_bits;
    uint8_t  vstval_width_bits;
    uint8_t  hgatp_vmid_width;
    uint8_t  vsatp_asid_width;

    /* MODE 支持位图（每位代表一个 MODE 是否被 WARL 接受）*/
    uint16_t hgatp_modes;     /* bit BARE/SV39X4/SV48X4/SV57X4   */
    uint16_t vsatp_modes;     /* bit BARE/SV39/SV48/SV57         */
    uint16_t stvec_modes;     /* bit0=Direct, bit1=Vectored       */

    /* 子扩展实现位（编译期宏 OR 运行期探测）*/
    bool has_sha,           has_shtvala,    has_shvstvala,
         has_shvstvecd,     has_shvsatpa,   has_shgatpa,
         has_shcounterenw,  has_shlcofideleg;
} platform_caps_t;

extern platform_caps_t g_caps;
void platform_caps_probe(void);          /* 测试套件 main 入口调用一次 */
```

### 模块 12：通用门控宏（`common/hyp/hyp_test.h` 扩展）

```c
#define REQUIRE_EXT(field)         do { if (!g_caps.field) TEST_SKIP(#field); } while(0)
#define REQUIRE_HGATP_MODE(m)      do { if (!(g_caps.hgatp_modes & (1u<<(m)))) TEST_SKIP("hgatp mode"); } while(0)
#define REQUIRE_VSATP_MODE(m)      do { if (!(g_caps.vsatp_modes & (1u<<(m)))) TEST_SKIP("vsatp mode"); } while(0)
#define REQUIRE_STVEC_MODE(m)      do { if (!(g_caps.stvec_modes & (1u<<(m)))) TEST_SKIP("stvec mode"); } while(0)
```

子扩展层（每扩展 1 行）：
```c
/* common/hyp/ext/shtvala.h    */ #define SHTVALA_REQUIRE()      REQUIRE_EXT(has_shtvala)
/* common/hyp/ext/shvstvala.h  */ #define SHVSTVALA_REQUIRE()    REQUIRE_EXT(has_shvstvala)
/* common/hyp/ext/shvstvecd.h  */ #define SHVSTVECD_REQUIRE()    REQUIRE_EXT(has_shvstvecd)
/* common/hyp/ext/shvsatpa.h   */ #define SHVSATPA_REQUIRE()     REQUIRE_EXT(has_shvsatpa)
/* common/hyp/ext/shgatpa.h    */ #define SHGATPA_REQUIRE()      REQUIRE_EXT(has_shgatpa)
/* common/hyp/ext/sha.h        */ #define SHA_REQUIRE()          REQUIRE_EXT(has_sha)
/* common/hyp/ext/shcounterenw.h     */ #define SHCOUNTERENW_REQUIRE()    REQUIRE_EXT(has_shcounterenw)
/* common/hyp/ext/shlcofideleg.h     */ #define SHLCOFIDELEG_REQUIRE()    REQUIRE_EXT(has_shlcofideleg)
```

### 模块 13：通用委托封装（`common/hyp/hyp_csr.c` 扩展）

不再为每种 trap 单独写 `delegate_xxx_to_hs`，统一改为掩码驱动：

```c
void delegate_causes_to_hs(uintptr_t cause_mask);   /* 仅 medeleg, hedeleg 清零 */
void delegate_causes_to_vs(uintptr_t cause_mask);   /* medeleg + hedeleg 双层 */
void delegate_ints_to_hs  (uintptr_t int_mask);
void delegate_ints_to_vs  (uintptr_t int_mask);

/* 预定义 mask 常量 */
#define MASK_GPF       ((1ul<<20)|(1ul<<21)|(1ul<<23))
#define MASK_VS_PF     ((1ul<<12)|(1ul<<13)|(1ul<<15))
#define MASK_VS_ECALL  (1ul<<10)
#define MASK_VIRT_INST (1ul<<22)
#define MASK_LCOFI_INT (1ul<<13)   /* Shlcofideleg 用 */
```

> Shtvala 用 `delegate_causes_to_hs(MASK_GPF)`；Shvstvala 用 `delegate_causes_to_vs(MASK_VS_PF | MASK_VIRT_INST)`；Shlcofideleg 用 `delegate_ints_to_vs(MASK_LCOFI_INT)`。**同一组 API 全覆盖。**

### 模块 14：通用 VS-mode 触发器（`common/hyp/test_vs_helpers.*` 扩展）

按内存访问语义而非按扩展命名：

```c
/* 已存在 */
uintptr_t test_vs_load_expect_fault (uintptr_t gva);   /* cause 21 */
uintptr_t test_vs_store_expect_fault(uintptr_t gva);   /* cause 23 */
uintptr_t test_vs_exec_expect_fault (uintptr_t gva);   /* cause 20 */

/* v2 新增 */
uintptr_t test_vs_load_n           (uintptr_t gva, size_t bytes);   /* misaligned/straddle */
uintptr_t test_vs_fetch_straddle   (uintptr_t gva);                 /* 末 2B 落入下一页 */
uintptr_t test_vs_csr_rw           (uintptr_t csr_and_val);         /* 触发 virtual-inst */
uintptr_t test_vs_sret             (uintptr_t unused);              /* 配合 VTSR */
uintptr_t test_vs_wfi              (uintptr_t unused);              /* 配合 VTW  */
uintptr_t test_vs_sfence_vma       (uintptr_t va_asid);             /* 配合 VTVM */
```

### 模块 15：两阶段翻译"故障注入"接口（`common/hyp/two_stage.*` 重构扩展）

把原来的"恒等映射"+"按 victim flags 设置"模式抽象为**故障类型 + 模式参数**：

```c
typedef enum {
    FAULT_NONE,
    FAULT_VS_STAGE_LOAD,        /* VS-stage 缺页：Shvstvala / 基线 */
    FAULT_VS_STAGE_STORE,
    FAULT_G_STAGE_EXPLICIT,     /* G-stage 显式访问缺页：Shtvala 主路径 */
    FAULT_G_STAGE_IMPLICIT,     /* implicit VS-stage：Shtvala 第二段 */
    FAULT_GPA_HIGH_BITS,        /* Sv*x4 高位越界：Shgatpa / Shtvala 共用 */
    FAULT_STRADDLE,             /* 跨页：Shtvala / Shvstvala 共用 */
} fault_kind_t;

typedef struct {
    int           vs_mode;        /* SATP_MODE_BARE/SV39/SV48/SV57 */
    int           g_mode;         /* HGATP_MODE_BARE/SV39X4/...    */
    fault_kind_t  fault;
    uintptr_t     fault_gva;      /* 输入或框架推算 */
    uintptr_t     fault_gpa;      /* 框架回写：用例直接 CHECK_HTVAL_EQ_SHIFTED */
    size_t        access_bytes;   /* misaligned/straddle 时使用 */
} two_stage_scene_t;

int  two_stage_build   (two_stage_ctx_t *ctx, two_stage_scene_t *scene);
void two_stage_activate(two_stage_ctx_t *ctx);
```

> 用例只声明"我要哪种故障"，框架返回期望 GPA/GVA。Shtvala / Shgatpa / Shvstvala / 基线两阶段翻译共用。
>
> **现状**：v1 的 `two_stage_init` 仅支持 `vs_mode == SATP_MODE_BARE`，因此 `FAULT_VS_STAGE_*` / `FAULT_G_STAGE_IMPLICIT` 在框架补全 VS-stage 之前需在用例侧 `TEST_SKIP("requires VS-stage")`。

### 模块 16：通用 trap 字段断言宏（`common/hyp/hyp_test.h` 扩展）

按 CSR 字段 + 变体命名，每字段提供 `_EQ / _NONZERO / _ZERO / _SHIFTED` 共 4 个：

```c
/* 已存在：CHECK_HTVAL / CHECK_HTINST / CHECK_GVA */

/* v2 新增 —— htval（GPA>>2）系列 */
#define CHECK_HTVAL_EQ(msg, exp_shifted)    CHECK_HTVAL(msg, exp_shifted)
#define CHECK_HTVAL_SHIFTED(msg, exp_gpa)   CHECK_HTVAL(msg, (uintptr_t)(exp_gpa) >> 2)
#define CHECK_HTVAL_NONZERO(msg)            TEST_ASSERT_NEQ(msg, trap_get_htval(), 0UL)
#define CHECK_HTVAL_ZERO(msg)               TEST_ASSERT_EQ (msg, trap_get_htval(), 0UL)

/* htinst */
#define CHECK_HTINST_NONZERO(msg)           TEST_ASSERT_NEQ(msg, trap_get_htinst(), 0UL)
#define CHECK_HTINST_ZERO(msg)              TEST_ASSERT_EQ (msg, trap_get_htinst(), 0UL)

/* hstatus.GVA */
#define CHECK_GVA_SET(msg)                  TEST_ASSERT(msg, trap_get_gva())
#define CHECK_GVA_CLEAR(msg)                TEST_ASSERT(msg, !trap_get_gva())

/* vstval / vsepc / vscause —— Shvstvala / Shvstvecd 用，待 trap_record 增加这些字段后启用 */
```

> Shtvala 主要用 HTVAL 系列；Shvstvala 用 VSTVAL 系列；Shvstvecd 用 VSEPC + 向量；**完全对称**。

### 模块 17：CSR WARL/MODE 通用探测（`common/hyp/hyp_csr.*` 扩展）

```c
/* 写 ~0、读回，得到 WARL 实际可保留位 */
uintptr_t csr_warl_probe       (unsigned csr_num);
unsigned  csr_field_width      (unsigned csr_num, unsigned shift, uintptr_t mask);
unsigned  csr_mode_field_supported(unsigned csr_num,
                                   unsigned mode_shift,
                                   unsigned mode_max);  /* 返回 MODE 支持位图 */
```

### 模块 18：子扩展条件编译统一形式（`Makefile.common` 扩展）

```makefile
# 每个子扩展一个独立开关，叠加到 HYP_EXT_FLAGS
HYP_EXT_FLAGS :=
ifdef ENABLE_SHA          ; HYP_EXT_FLAGS += -DENABLE_SHA          ; endif
ifdef ENABLE_SHTVALA      ; HYP_EXT_FLAGS += -DENABLE_SHTVALA      ; endif
ifdef ENABLE_SHVSTVALA    ; HYP_EXT_FLAGS += -DENABLE_SHVSTVALA    ; endif
ifdef ENABLE_SHVSTVECD    ; HYP_EXT_FLAGS += -DENABLE_SHVSTVECD    ; endif
ifdef ENABLE_SHVSATPA     ; HYP_EXT_FLAGS += -DENABLE_SHVSATPA     ; endif
ifdef ENABLE_SHGATPA      ; HYP_EXT_FLAGS += -DENABLE_SHGATPA      ; endif
ifdef ENABLE_SHCOUNTERENW ; HYP_EXT_FLAGS += -DENABLE_SHCOUNTERENW ; endif
ifdef ENABLE_SHLCOFIDELEG ; HYP_EXT_FLAGS += -DENABLE_SHLCOFIDELEG ; endif
CFLAGS  += $(HYP_EXT_FLAGS)
ASFLAGS += $(HYP_EXT_FLAGS)
```

### 子扩展登记表

新增子扩展时只需在此表登记一行 + 一个 `common/hyp/ext/<name>.h`（5–30 行）：

| 子扩展 | REQUIRE 宏 | 主要复用通用 API | 主要 trap 字段 | 主要 cause / 委托 mask |
|---|---|---|---|---|
| Sha             | `SHA_REQUIRE`           | 全部                                       | —                        | — |
| Shtvala         | `SHTVALA_REQUIRE`       | `delegate_causes_to_hs(MASK_GPF)` / `two_stage_build(FAULT_G_STAGE_*)` / `CHECK_HTVAL_NONZERO` | `htval`, `htinst`, `GVA` | `MASK_GPF` |
| Shvstvala       | `SHVSTVALA_REQUIRE`     | `delegate_causes_to_vs(MASK_VS_PF\|MASK_VIRT_INST)` / `CHECK_VSTVAL_NONZERO` | `vstval`, `vscause` | `MASK_VS_PF \| MASK_VIRT_INST` |
| Shvstvecd       | `SHVSTVECD_REQUIRE`     | `REQUIRE_STVEC_MODE(0)` / `delegate_causes_to_vs(...)` | `vsepc`, `vscause` | `MASK_VS_PF` |
| Shvsatpa        | `SHVSATPA_REQUIRE`      | `csr_mode_field_supported(CSR_VSATP, ...)` | —                        | — |
| Shgatpa         | `SHGATPA_REQUIRE`       | `REQUIRE_HGATP_MODE(*)` / `two_stage_build(FAULT_GPA_HIGH_BITS)` | `htval` | `MASK_GPF` |
| Shcounterenw    | `SHCOUNTERENW_REQUIRE`  | `csr_warl_probe(CSR_HCOUNTEREN)`           | —                        | — |
| Shlcofideleg    | `SHLCOFIDELEG_REQUIRE`  | `delegate_ints_to_vs(MASK_LCOFI_INT)`      | —                        | `MASK_LCOFI_INT` |

### 与 v1 命名的对照（避免日后命名漂移）

| v1 草案（Shtvala 专用） | v2 通用化命名 |
|---|---|
| `delegate_gpf_to_hs()` | `delegate_causes_to_hs(MASK_GPF)` |
| `delegate_nongpf_traps_to_hs()` | `delegate_causes_to_hs(<other masks>)` |
| `two_stage_setup_bare_vsatp_*x4_hgatp()` × 3 | `two_stage_build(scene{ .fault=FAULT_G_STAGE_EXPLICIT, .g_mode=SV*X4 })` |
| `htval_warl_probe()` / `htval_impl_width_bits()` | `csr_warl_probe(CSR_HTVAL)` / `csr_field_width(CSR_HTVAL,...)` |
| `hgatp_supported_modes_mask()` | `csr_mode_field_supported(CSR_HGATP, HGATP_MODE_SHIFT, 15)` |
| `SHTVALA_REQUIRE` 散在 hyp_test.h | `REQUIRE_EXT(has_shtvala)` + 子扩展 header |
| `CHECK_HTVAL_NONZERO/_ZERO/_RECONSTRUCT_GPA` | 同名宏，但与 `CHECK_VSTVAL_*` / `CHECK_VSEPC_*` 模板对齐 |
| 子扩展直接散在 `hyp_csr.c` | `common/hyp/ext/<name>.h` 隔离，每扩展一个文件 |
| Makefile 单开关 `ENABLE_SHTVALA` | 8 个子扩展统一形式 `ENABLE_SH<NAME>` + `HYP_EXT_FLAGS` |

### 测试目录约定

```
Shtvala/           # 已实现：Shtvala 测试套件
Shvstvala/         # 已实现：Shvstvala 测试套件
Shvstvecd/         # 已实现：Shvstvecd 测试套件
Shvsatpa/          # 已实现：Shvsatpa 测试套件
Shgatpa/           # 已实现：Shgatpa 测试套件
Shcounterenw/      # 已实现：Shcounterenw 测试套件
Shlcofideleg/      # 已实现：Shlcofideleg 测试套件
Sha/               # 已实现：Sha 测试套件
```

> **注意**：目录名使用首字母大写形式（如 `Shtvala/`），与 Sv*/Ss*/Sm* 等扩展目录命名风格一致。

每个子目录一个 `Makefile`，统一 `include ../common/Makefile.common` 并打开自己的 `ENABLE_SH<NAME>`。

### 落地状态（截至 v2）

| 模块 | 状态 |
|---|---|
| 模块 11 platform_caps_t  | TODO（最低集合：has_shtvala / hgatp_modes / htval_width_bits） |
| 模块 12 REQUIRE 宏       | TODO（先在 Shtvala 模块本地定义 SHTVALA_REQUIRE，框架收敛时上提） |
| 模块 13 委托封装         | TODO（v1 已有 hedeleg_write，Shtvala 模块直接用 medeleg+CSRR/W 设置） |
| 模块 14 VS-mode 触发器   | 已有 `test_vs_load_expect_fault/store_expect_fault/exec_expect_fault`；其余 v2 新增 |
| 模块 15 故障注入接口     | 已有 `_setup_with_victim` 模式（Sv39x4），可作为 `FAULT_G_STAGE_EXPLICIT` 实现 |
| 模块 16 trap 字段断言宏  | 已有 `CHECK_HTVAL/CHECK_HTINST/CHECK_GVA`；NONZERO/ZERO/SET/CLEAR 已在本次补齐 |
| 模块 17 CSR WARL 探测    | TODO |
| 模块 18 Makefile 子扩展开关 | 已收敛到 `ENABLE_HYP`；子扩展开关在 v2 阶段按需添加 |

> 后续接入 Shvstvala / Shgatpa 等扩展时，**优先把对应模块 11/13/15/17 的 TODO 项落地**，再补该子扩展自身的 ext header；不要反过来在子扩展里塞专用 API。

---

## Hypervisor 基线测试套件 (`Hypervisor/`)

### 概述

`Hypervisor/` 目录下包含根据 `DOCS/testplan/Hypervisor_test_plan.md` 编写的完整基线测试套件，覆盖 19 个测试章节、约 225 个测试用例。测试套件验证 RISC-V Hypervisor Extension 的 CSR 行为、trap 机制、中断委托、地址翻译等核心功能。

### 目录结构

```
Hypervisor/
├── Makefile                    # ENABLE_HYP=1, ENABLE_VM=1
├── kernel.ld                   # .test_table / .vm_test_region / .gpt_page_tables
├── main.c                      # 遍历 _test_table[] 执行所有测试
└── tests/
    ├── test_helpers.h          # 通用辅助函数声明
    ├── test_helpers.c          # VS/VU trampoline、委托配置辅助
    ├── test_register.c         # #include 所有测试文件，触发 TEST_REGISTER
    ├── test_csr_basics.c       # Ch.1  VCSR-01~17  VS CSR 替代行为
    ├── test_hstatus.c          # Ch.2  HSTAT-01~26 hstatus 寄存器
    ├── test_delegation.c       # Ch.3  DELEG-01~16 hedeleg/hideleg
    ├── test_interrupts.c       # Ch.4-5 HINT-01~14 + HGEI-01~05
    ├── test_henvcfg.c          # Ch.6  HENV-01~14  henvcfg
    ├── test_hcounteren.c       # Ch.7  HCNT-01~08  hcounteren
    ├── test_htimedelta.c       # Ch.8  HTDL-01~04  htimedelta
    ├── test_vsip_vsie.c        # Ch.9  VSIE-01~10  vsip/vsie alias
    ├── test_vstimecmp.c        # Ch.10 VSTC-01~07  vstimecmp
    ├── test_vs_scratch.c       # Ch.11 VSCR-01~06  vsscratch/vsepc/vscause/vstval
    ├── test_virtual_inst.c     # Ch.12 VINST-01~24 virtual-instruction exception
    ├── test_trap_entry.c       # Ch.13 TENT-01~15  trap entry CSR 写入
    ├── test_trap_return.c      # Ch.14 TRET-01~15  MRET/SRET 模式切换
    ├── test_htinst.c           # Ch.15 TINST-01~09 htinst 转换指令
    ├── test_mstatus_hyp.c      # Ch.16 MSTAT-01~14 mstatus 增强
    ├── test_mideleg_enhance.c  # Ch.17 MIDLG-01~07 mideleg/mip/mie 增强
    ├── test_mtval2.c           # Ch.18 MTVAL-01~05 mtval2/mtinst
    └── test_exception_priority.c # Ch.19 PRIO-01~05 异常优先级
```

### VS/VU-mode Trampoline 设计

由于测试在 M-mode 下运行，需要通过 trampoline 函数在 VS/VU-mode 下执行特定操作。`test_helpers.c` 提供了一组 trampoline 函数，每个函数签名为 `uintptr_t fn(uintptr_t arg)`，通过 `run_in_vs_mode(fn, arg)` 或 `run_in_vu_mode(fn, arg)` 调用。

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
cd Hypervisor
make PLATFORM=qemu_virt        # 编译
make qemu                      # 在 QEMU 上运行（需要 rv64,h=true）
```

QEMU CPU 配置: `rv64,h=true,sv39=true,sv48=true,sv57=true`

---

## 参考

- RISC-V Privileged Specification — Hypervisor Extension (`SPEC/hypervisor.adoc`)
- `docs/vm_test_framework.md` — 现有 VM 测试框架文档
- `docs/vm_test_plan.md` — VM 测试计划（可参考其测试组织方式）
- `common/test_framework.h` — 现有测试框架 API
- `common/vm/vm.h` — 现有 VM 管理 API
- `common/privilege.c` — 现有特权级切换实现
- `common/trap.c` — 现有 trap handler 实现
