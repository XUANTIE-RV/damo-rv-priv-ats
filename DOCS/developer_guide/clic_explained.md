# RISC-V CLIC (Core-Local Interrupt Controller) 扩展详解

> 本文档基于 SPEC/clic_0.1、SPEC/riscv-aia、SPEC/riscv-aclint 等规范内容编写，旨在系统性地说明 CLIC 扩展的设计目的、架构组织、与其他中断扩展的关系，以及不同场景下的中断控制器组合选型建议。

---

## 目录

1. [CLIC 扩展的背景与动机](#1-clic-扩展的背景与动机)
2. [CLIC 要解决的核心问题](#2-clic-要解决的核心问题)
3. [CLIC 的扩展结构与模块组织](#3-clic-的扩展结构与模块组织)
4. [为什么需要 CLIC](#4-为什么需要-clic)
5. [何时使用 CLIC](#5-何时使用-clic)
6. [CLIC 与其他中断扩展的关系](#6-clic-与其他中断扩展的关系)
7. [不同场景下的中断控制器组合选型](#7-不同场景下的中断控制器组合选型)
8. [附录：关键 CSR 与术语速查](#8-附录关键-csr-与术语速查)

---

## 1. CLIC 扩展的背景与动机

### 1.1 RISC-V 原始中断机制（CLINT mode）的局限

RISC-V 特权架构最初定义了一套基础本地中断机制，通过 `mip`/`mie`/`mtvec` 等 CSR 来管理中断。这种基础机制通常与 SiFive CLINT（Core-Local Interruptor）设备配合使用，因此规范中将其称为 **CLINT mode**（即 `mtvec.mode` = `00` 或 `01`）。

CLINT mode 存在以下关键局限：

| 局限点 | 具体说明 |
|--------|----------|
| **优先级固定** | 本地中断的优先级是固定的，无法为每个中断源单独配置优先级 |
| **无同级抢占** | 仅支持基于特权模式（privilege mode）的抢占，不支持同一特权模式内的中断抢占（horizontal preemption） |
| **向量表受限** | CLINT vectored mode 的向量表中存放的是跳转指令（jump instructions），受 ±1MiB 跳转范围限制 |
| **中断委托方式单一** | 仅通过 `mideleg` CSR 进行中断委托，无法灵活地为每个中断源单独指定目标特权模式 |
| **上下文切换开销大** | 处理背靠背（back-to-back）中断时，需要完整的上下文保存/恢复流程，延迟较高 |
| **外部中断信号单一** | PLIC 向每个特权模式仅发送单一外部中断信号（meip/seip），中断控制器侧需要 notification/claim/response/completion 多步操作 |

### 1.2 CLIC 的设计目标

CLIC（Core-Local Interrupt Controller）的规范标题为 **"Core-Local Interrupt Controller (CLIC) RISC-V Privileged Architecture Extensions"**，其核心设计目标是：

> **提供低延迟、向量化、可抢占的中断处理能力**（low-latency, vectored, pre-emptive interrupts）

具体而言：

- **低延迟**：通过硬件向量化（hardware vectoring）和 `mnxti` 等机制减少中断响应路径上的指令数
- **向量化**：每个中断源可以有独立的 trap 入口地址，且向量表存放的是函数指针而非跳转指令，无地址范围限制
- **可抢占**：支持同一特权模式内基于中断级别（interrupt level）的抢占，最多 256 级
- **最小硬件需求**：基础设计只需要最少的硬件，但可通过可选扩展提供硬件加速
- **灵活的软件 ABI**：支持多种软件中断模型（inline handler、C-ABI trampoline、中断驱动模型等）

---

## 2. CLIC 要解决的核心问题

### 2.1 问题一：缺少细粒度的中断优先级与抢占控制

在 CLINT mode 下，中断只能按特权模式进行抢占（M-mode 可以抢占 S-mode），同一特权模式内的中断无法互相抢占。这意味着：

- 一个低优先级的中断处理函数会阻塞同特权模式下的高优先级中断
- 无法实现实时系统中常见的"中断嵌套"需求

**CLIC 的解决方案**：
- 引入 **256 级中断级别**（interrupt level, 0-255），level 0 为正常执行，1-255 为中断处理级别
- 高级别的中断可以抢占同特权模式下低级别的中断（horizontal preemption）
- 每个中断源的 level 和 priority 可通过 `clicintctl[i]` 独立配置
- 通过 `mintthresh`（中断级别阈值）实现临界区（critical section）

### 2.2 问题二：中断向量化的局限性

CLINT mode 的 vectored mode 向量表中存放的是跳转指令，存在 ±1MiB 的地址范围限制，且所有中断共享一个通用的入口点。

**CLIC 的解决方案**：
- CLIC mode 的向量表存放的是 **XLEN 位函数指针**（地址而非指令），无跳转范围限制
- 引入 **选择性硬件向量化**（Selective Hardware Vectoring, smclicshv）扩展
- 每个中断可通过 `clicintattr[i].shv` 位独立选择：
  - **硬件向量化**（shv=1）：硬件自动从 `mtvt` 指向的向量表中读取函数指针并跳转，最低中断延迟
  - **软件向量化**（shv=0）：所有中断跳转到 `mtvec` 基地址的公共代码，由软件通过 `mnxti` CSR 查询并分发，代码体积更小

### 2.3 问题三：中断处理上下文切换开销大

在 CLINT mode 下，每次中断处理都需要完整的寄存器保存/恢复流程，对于频繁的背靠背中断，开销显著。

**CLIC 的解决方案**：
- `mnxti` CSR：用于在处理完一个中断后，无需完整的上下文保存/恢复即可处理下一个同级或更高级别的中断（interrupt chaining）
- `mcause` 扩展：将 `mpp`、`mpie`、`mpil` 镜像到 `mcause` 中，减少 context save/restore 的 CSR 访问次数
- `mscratchcsw` / `mscratchcswl`：条件性 scratch 寄存器交换，加速跨特权模式或跨中断级别的栈指针切换

### 2.4 问题四：中断委托不够灵活

CLINT mode 通过 `mideleg` 进行中断委托，粒度粗（仅按中断类型），且无法为每个中断源单独指定目标特权模式。

**CLIC 的解决方案**：
- 在 CLIC mode 下，`mideleg` 不再生效
- 每个中断源通过 `clicintattr[i].mode` 字段独立指定目标特权模式（M/S/U）
- 中断委托不再依赖 `mideleg` 的位映射，而是逐中断配置

---

## 3. CLIC 的扩展结构与模块组织

### 3.1 扩展概览

CLIC 不是单一扩展，而是由多个子扩展组成的扩展集合。规范定义了以下 4 个子扩展：

| 扩展名 | 描述 | 依赖关系 |
|--------|------|----------|
| **smclic** | CLIC 的 M-mode 支持，是 CLIC 的基础扩展 | 无 |
| **ssclic** | CLIC 的 S-mode 支持 | 依赖 smclic |
| **smclicshv** | M-mode 选择性硬件向量化（Selective Hardware Vectoring） | 依赖 smclic |
| **smclicconfig** | 允许实现支持不同的 CLIC 参数化配置 | 依赖 smclic |

> **注意**：规范中曾定义过 `suclic`（U-mode CLIC），但在后续修订中已移除 U-mode 中断的提及。当前 CLIC 支持 M-mode 和 S-mode 两个特权级别。

### 3.2 CLIC 的中断输入模型

CLIC 支持每个 hart 最多 **4096 个中断输入**。每个中断输入 `i` 有四个控制寄存器：

```
clicintip[i]    - 中断挂起位（Interrupt Pending）
clicintie[i]    - 中断使能位（Interrupt Enable）
clicintattr[i]  - 中断属性（特权模式、触发类型、SHV 标志）
clicintctl[i]   - 中断级别与优先级控制（Level + Priority）
```

**中断 ID 分配**（与 CLINT mode 兼容的方案）：

| ID | 中断 | 说明 |
|----|------|------|
| 0 | reserved | 保留 |
| 1 | ssip | Supervisor 软件中断 |
| 3 | msip | Machine 软件中断 |
| 5 | stip | Supervisor 定时器中断 |
| 7 | mtip | Machine 定时器中断 |
| 9 | seip | Supervisor 外部中断（来自 PLIC/APLIC） |
| 11 | meip | Machine 外部中断（来自 PLIC/APLIC） |
| 16 | csip | CLIC 软件中断（用于本地后台线程） |
| 17+ | CLIC local inputs | 额外本地中断输入 |

前 16 个中断 ID 保留给 CLINT mode 中断，以保持向后兼容。最多可添加 4080 个额外本地中断。

### 3.3 寄存器访问方式

CLIC 的中断控制寄存器（`clicintip`、`clicintie`、`clicintattr`、`clicintctl`、`clicinttrig`）通过 **间接 CSR 访问**（Indirect CSR Access）来访问，利用 Smcsrind/Sscsrind 扩展提供的 `miselect`/`mireg`/`mireg2`（M-mode）或 `siselect`/`sireg`/`sireg2`（S-mode）机制。

间接访问的映射关系：

| miselect 范围 | mireg 内容 | mireg2 内容 |
|---------------|-----------|-------------|
| 0x1000+i | clicintctl[i*4+0..3]（4 个中断的 level/priority，每中断 8 bit） | clicintattr[i*4+0..3]（4 个中断的属性，每中断 8 bit） |
| 0x1400-0x147F | clicintip[31:0] 到 clicintip[4095:4064]（每 32 个中断一组） | clicintie[31:0] 到 clicintie[4095:4064] |
| 0x1480-0x149F | clicinttrig[0..31]（中断触发器） | - |
| 0x14A0 | mcliccfg（全局配置） | - |

### 3.4 CLIC 新增/修改的 CSR

| CSR 地址 | 名称 | 说明 |
|----------|------|------|
| 0x305 | mtvec | 修改：新增 mode=11 + submode=0000 为 CLIC mode |
| 0x307 | mtvt | 新增：trap 向量表基地址（用于硬件向量化和 mnxti） |
| 0x342 | mcause | 修改：扩展 mpp/mpie/mpil/minhv 字段 |
| 0x345 | mnxti | 新增：下一个中断处理地址与使能修改器 |
| 0xFB1 | mintstatus | 新增：当前中断级别（mil/sil） |
| 0x347 | mintthresh | 新增：中断级别阈值 |
| 0x348 | mscratchcsw | 新增：跨特权模式条件性 scratch 交换 |
| 0x349 | mscratchcswl | 新增：跨中断级别条件性 scratch 交换 |
| 0x303 | mideleg | 修改：CLIC mode 下失效 |
| 0x304 | mie | 修改：CLIC mode 下读为 0 |
| 0x344 | mip | 修改：CLIC mode 下读为 0 |

S-mode 对应的 CSR（`stvt`、`snxti`、`sintstatus`、`sintthresh`、`sscratchcsw`、`sscratchcswl`）地址在 M-mode 对应地址的基础上减去 0x200。

### 3.5 CLIC mode 与 CLINT mode 的切换

CLIC mode 和 CLINT mode 通过 `mtvec.mode` 字段进行切换：

| mtvec.mode | mtvec.submode | 模式 | PC on Interrupt |
|------------|---------------|------|-----------------|
| 00 | xxxx | CLINT direct mode | OBASE |
| 01 | xxxx | CLINT vectored mode | OBASE + 4*exccode |
| 11 | 0000 | **CLIC mode** | NBASE（或 SHV 向量化地址） |
| 10 | 0000 | Reserved | - |

关键约束：**当前版本要求所有特权模式必须同时运行在 CLIC mode 或同时运行在 CLINT mode**，不允许混合使用。

### 3.6 中断抢占模型

CLIC 的中断抢占分为两类：

**垂直抢占（Vertical Preemption）**：
- 高特权模式的中断抢占低特权模式的执行
- 与 CLINT mode 行为一致，不依赖全局中断使能

**水平抢占（Horizontal Preemption）**：
- 同一特权模式内，高级别中断抢占低级别中断
- 需要当前特权模式的全局中断使能（`mstatus.mie`）已置位
- 中断级别存储在 `mintstatus.mil`（当前级别）和 `mcause.mpil`（前一级别）中

抢占条件总结：

```
当前状态 P/ie/L，CLIC 选中中断 priv/level/id：
- nP < P              → 中断被忽略
- nP = P, ie = 0      → 中断被禁用
- nP = P, level <= L  → 中断被忽略
- nP = P, level > L   → 水平抢占（horizontal interrupt taken）
- nP > P, level > 0   → 垂直抢占（vertical interrupt taken）
```

### 3.7 smclicconfig 扩展——参数化配置

smclicconfig 扩展允许硬件实现支持可配置的 CLIC 参数化，主要通过 `mcliccfg` 寄存器实现：

| 字段 | 说明 |
|------|------|
| `nmbits` | 指定 `clicintattr.mode` 中物理实现的位数（0=M-only, 1=M/S） |
| `mnlbits` | M-mode 下 `clicintctl` 中分配给中断级别的位数 |
| `snlbits` | S-mode 下 `clicintctl` 中分配给中断级别的位数 |

CLIC 的关键参数：

| 参数名 | 范围 | 说明 |
|--------|------|------|
| NUM_INTERRUPT | 2-4096 | 最大中断输入数 |
| CLICINTCTLBITS | 0-8 | clicintctl 实际实现的位数 |
| CLICLEVELS | 2-256 | 中断级别数（含 0） |
| CLICPRIVMODES | 1-3 | 支持的特权模式数 |
| CLICANDBASIC | 0-1 | 是否同时支持 CLINT mode |
| INTTHRESHBITS | 1-8 | mintthresh 实现的位数 |
| NVBITS | 0-1 | 是否支持 smclicshv |

---

## 4. 为什么需要 CLIC

### 4.1 实时性需求

嵌入式和实时系统对中断响应延迟有严格要求。CLINT mode 的中断处理路径需要：
1. 跳转到 trap 入口
2. 保存上下文（多个寄存器）
3. 查询中断原因
4. 分发到具体处理函数
5. 处理完毕后恢复上下文
6. 返回

CLIC 通过硬件向量化直接跳转到特定中断的处理函数，配合 inline handler 可将中断响应路径缩短到 **约 13 条指令**（含清中断源和计数器递增）。对于背靠背中断，`mnxti` 机制使得服务循环仅需 **约 7 个时钟周期**的额外开销。

### 4.2 细粒度优先级控制

实时系统需要为每个中断源设置独立的优先级和抢占级别。CLINT mode 无法做到这一点，而 CLIC 提供了：
- 每中断独立的 level（决定抢占）和 priority（同级排序）
- 最多 256 级中断级别
- 每中断独立的目标特权模式配置

### 4.3 软件灵活性

CLIC 不强制使用单一的软件中断模型，而是支持多种模型：

| 模型 | 特点 | 适用场景 |
|------|------|----------|
| Inline handler | 函数内直接处理，callee-save，最小开销 | 简单快速中断处理 |
| C-ABI trampoline | 通用 trampoline + C 函数调用 | 需要完整 C ABI 的复杂处理 |
| 中断驱动模型 | 所有代码由中断驱动，级别 0 睡眠 | 低功耗嵌入式系统 |
| gp trampoline | 3 条指令的极简 trampoline | 单特权模式系统 |

### 4.4 与现有生态的兼容性

CLIC 设计为与 CLINT mode 向后兼容：
- 前 16 个中断 ID 保持与 CLINT mode 一致
- 支持同时实现 CLINT 和 CLIC 两种模式
- CLIC CSR 在 CLINT mode 下行为与无 CLIC 时一致
- `mie`/`mip` 的状态在模式切换时保留

---

## 5. 何时使用 CLIC

### 5.1 适合使用 CLIC 的场景

1. **实时嵌入式系统**：需要确定性的低中断延迟和可配置的优先级抢占
2. **单核或少量核心的系统**：CLIC 是 per-hart（每核）的中断控制器，适合每核独立管理本地中断
3. **需要细粒度中断优先级的系统**：同一特权模式内需要多级中断抢占
4. **需要独立中断入口地址的系统**：不同中断需要不同的 trap 入口以减少分发延迟
5. **中断密集型应用**：频繁的背靠背中断场景，`mnxti` 机制可显著降低开销
6. **微控制器类应用**：M-mode only 或 M/S mode 的简洁系统

### 5.2 不适合（或不需要）使用 CLIC 的场景

1. **大型 SMP 服务器系统**：此类系统更适合使用 AIA（MSI + IMSIC + APLIC），AIA 专为高性能多核系统设计
2. **需要虚拟化中断支持的系统**：AIA 提供了更完善的虚拟化中断支持（guest interrupt files）
3. **不需要细粒度优先级的简单系统**：如果 CLINT mode 的固定优先级已满足需求，无需引入 CLIC 的复杂性
4. **需要消息信号中断（MSI）的系统**：CLIC 不支持 MSI，需使用 AIA

### 5.3 CLIC 与 AIA 的功能对比

根据 CLIC 规范和 AIA 规范的对比：

| 特性 | CLIC | AIA |
|------|------|-----|
| **设计目标** | 低延迟、向量化、可抢占 | MSI 支持、多核、虚拟化 |
| **目标系统** | 嵌入式/实时系统 | 大型高性能系统 |
| **中断源类型** | 本地有线中断 | MSI + 有线中断 |
| **每核独立中断入口** | 支持（SHV） | 不支持 |
| **同级中断抢占** | 支持（256 级） | 不支持（AIA 明确指出不支持） |
| **自动寄存器堆叠** | 不支持 | 不支持 |
| **中断优先级配置** | 每中断可配置 level + priority | 可选的 iprio 机制 |
| **向量表内容** | 函数指针（地址） | 无独立向量表 |
| **外部中断控制器** | 与 PLIC/APLIC 互补 | APLIC + IMSIC |
| **虚拟化支持** | 无 | 完善（guest interrupt files） |
| **MSI 支持** | 无 | 有（IMSIC） |
| **多核 IPI** | 无内置 IPI 机制 | 有（通过 IMSIC 写入） |
| **寄存器访问** | 间接 CSR（miselect/mireg） | CSR + 内存映射 |

> **关键区别**：AIA 在其引言中明确指出，当前版本**不支持**以下实时系统特性：
> - 每个中断源独立的 trap 入口地址
> - 中断进入时自动堆叠寄存器
> - 基于优先级的自动中断抢占
>
> 而 CLIC 正是为了提供这些特性而设计的。两者在设计目标上是**互补**的。

---

## 6. CLIC 与其他中断扩展的关系

### 6.1 RISC-V 中断控制器全景图

```
                        ┌──────────────────────────────────────────┐
                        │           外部中断源（I/O 设备等）          │
                        └──────┬───────────────┬───────────────────┘
                               │               │
                    ┌──────────▼──────┐ ┌──────▼──────────────┐
                    │   PLIC (原始)    │ │   APLIC (AIA)        │
                    │  集中式有线路由   │ │  有线路由 + MSI 转换  │
                    └──────────┬──────┘ └──────┬──────────────┘
                               │               │
                    ┌──────────▼──────┐ ┌──────▼──────────────┐
                    │   IMSIC (AIA)   │ │   直接有线信号        │
                    │  MSI 接收/分发   │ │                      │
                    └──────────┬──────┘ └─────────────────────┘
                               │
          ┌────────────────────┼────────────────────┐
          │                    │                    │
   ┌──────▼──────┐    ┌───────▼───────┐   ┌───────▼───────┐
   │   CLINT      │    │   ACLINT       │   │    CLIC       │
   │ (SiFive原始)  │    │ (模块化CLINT)  │   │ (高级本地中断) │
   │ MSWI+MTIMER  │    │ MSWI+MTIMER   │   │ per-hart本地   │
   │ 统一地址空间  │    │ +SSWI(分离)    │   │ 中断控制器     │
   └──────────────┘    └───────────────┘   └───────────────┘
```

### 6.2 CLIC 与 CLINT 的关系

| 方面 | 说明 |
|------|------|
| **替代关系** | CLIC 在激活时**取代**（subsume）CLINT mode 的本地中断方案 |
| **兼容性** | CLIC 可与 CLINT mode 共存，通过 `mtvec.mode` 切换 |
| **中断 ID 兼容** | CLINT mode 的中断 ID（0-15）在 CLIC mode 中保持不变 |
| **功能扩展** | CLINT 的 `mip`/`mie` 在 CLIC mode 下读为 0，由 `clicintip`/`clicintie` 替代 |
| **中断委托** | CLINT 用 `mideleg`，CLIC 用 `clicintattr.mode` |

CLIC 规范明确指出：
> "When activated the CLIC subsumes and replaces the original RISC-V basic local interrupt scheme (known as the CLINT in this document)."

### 6.3 CLIC 与 ACLINT 的关系

ACLINT（Advanced Core Local Interruptor）是 CLINT 的模块化升级版本，定义了三个独立的内存映射设备：

| ACLINT 设备 | 功能 | CLIC 中的对应 |
|-------------|------|--------------|
| MTIMER | M-mode 定时器（MTIME + MTIMECMP） | CLIC 将 mtip 作为中断输入 7 处理 |
| MSWI | M-mode 软件中断（MSIP） | CLIC 将 msip 作为中断输入 3 处 |
| SSWI | S-mode 软件中断（SETSSIP） | CLIC 将 ssip 作为中断输入 1 处 |

**关键关系**：
- ACLINT 提供的是**定时器和 IPI 的物理信号源**，而 CLIC 是**中断控制器**
- ACLINT 产生的中断信号（mtip/msip/ssip）作为 CLIC 的中断输入
- 两者是**互补关系**：ACLINT 提供信号源，CLIC 负责优先级仲裁、向量化、抢占控制
- 在一个系统中可以同时存在 ACLINT 和 CLIC：ACLINT 提供定时器和 IPI 信号，CLIC 负责中断管理

### 6.4 CLIC 与 PLIC/APLIC 的关系

CLIC 规范明确指出 CLIC 与 PLIC 是**互补关系**：

> "The CLIC complements the PLIC. Smaller single-core systems might have only a CLIC, while multicore systems might have a CLIC per-core and a single shared PLIC."

| 方面 | CLIC | PLIC/APLIC |
|------|------|------------|
| **作用范围** | per-hart（每核本地） | 系统级（多核共享） |
| **中断类型** | 本地中断（含 timer/software/external） | 外部中断（来自 I/O 设备） |
| **外部中断处理** | 将 PLIC/APLIC 的 meip/seip 信号作为本地中断输入 | 集中管理外部设备中断，路由到各 hart |
| **共存方式** | PLIC/APLIC 的 xeip 信号作为 CLIC 的中断输入（ID 9/11） | PLIC/APLIC 不感知 CLIC 的存在 |

工作流程：
1. 外部设备产生中断 → PLIC/APLIC 仲裁优先级
2. PLIC/APLIC 向目标 hart 发送 meip/seip 信号
3. CLIC 将 meip/seip 作为本地中断输入（ID 11/9）处理
4. CLIC 根据 `clicintattr[11/9]` 的配置决定在哪个特权模式处理
5. CLIC 根据 `clicintctl[11/9]` 的配置决定中断级别和优先级

### 6.5 CLIC 与 AIA 的关系

AIA（Advanced Interrupt Architecture）是一个更全面的中断架构，包含：

| AIA 组件 | 功能 | 与 CLIC 的关系 |
|----------|------|--------------|
| Smaia/Ssaia (CSR) | 扩展 hart 的中断 CSR | 与 CLIC 在 CSR 层面有交集（都使用 `miselect`/`mireg` 间接访问机制） |
| IMSIC | MSI 接收控制器 | 功能不重叠：CLIC 不处理 MSI，IMSIC 不处理本地中断 |
| APLIC | 高级 PLIC | 与 PLIC 同理，与 CLIC 互补 |
| iprio 机制 | 可选的中断优先级配置 | 功能部分重叠：都允许配置中断优先级，但机制不同 |

**关键交集**：
1. **间接 CSR 访问**：CLIC 和 AIA 都使用 Smcsrind/Sscsrind 提供的 `miselect`/`mireg`/`mireg2` 机制。AIA 规范明确指出 Smcsrind/Sscsrind 扩展了 AIA 的间接 CSR 访问设施。
2. **中断优先级**：AIA 的 `iprio` 机制允许配置主要中断的优先级，CLIC 的 `clicintctl` 也提供优先级配置，但 CLIC 额外提供了 level-based 抢占机制。
3. **设计定位不同**：AIA 明确表示不支持独立 trap 入口、自动寄存器堆叠和基于优先级的自动抢占，而这些正是 CLIC 的核心功能。

### 6.6 CLIC 与 CLINT mode 的共存约束

当系统同时支持 CLINT 和 CLIC 两种模式时：

- 所有特权模式必须**同时**运行在 CLIC mode 或 CLINT mode
- 模式切换时，`mip`/`mie` 的状态位保留，但 CLIC CSR 的值变为 undefined
- `mcause` 中的 CLIC 扩展字段（mpil/minhv）在切换到 CLINT mode 时被清零
- `mintstatus` 在 CLINT mode 下仍可访问（用于支持两种模式的系统）

---

## 7. 不同场景下的中断控制器组合选型

### 7.1 场景一：简单微控制器（M-mode only，无外部中断）

```
┌─────────────────────────────────┐
│  Hart (M-mode only)             │
│  ┌───────────────────────────┐  │
│  │  CLIC                     │  │
│  │  (本地中断管理)            │  │
│  │  + timer/software 输入    │  │
│  └───────────────────────────┘  │
│  无 PLIC/APLIC/IMSIC            │
└─────────────────────────────────┘
```

**适用场景**：低成本微控制器，仅需定时器和少量本地中断
**组合**：CLIC only（nmbits=0，所有中断为 M-mode）
**优势**：最低硬件成本，CLIC 提供低延迟向量化中断

### 7.2 场景二：嵌入式实时系统（M/S mode，有外部设备）

```
┌─────────────────────────────────┐
│  Hart (M/S mode)                │
│  ┌───────────────────────────┐  │
│  │  CLIC                     │  │
│  │  + ACLINT (timer + IPI)   │  │
│  └───────────┬───────────────┘  │
│              │ meip/seip        │
│  ┌───────────▼───────────────┐  │
│  │  PLIC 或 APLIC             │  │
│  │  (外部设备中断路由)         │  │
│  └───────────────────────────┘  │
└─────────────────────────────────┘
```

**适用场景**：需要实时中断响应的嵌入式系统，有外部 I/O 设备
**组合**：CLIC + ACLINT + PLIC（或 APLIC without IMSIC）
**优势**：CLIC 提供低延迟本地中断和抢占，PLIC/APLIC 处理外部设备中断
**注意**：此场景下不建议使用 AIA 的 iprio 机制与 CLIC 的优先级机制混用

### 7.3 场景三：多核实时系统

```
┌──────────────────────────────────────┐
│  Hart 0          Hart 1    ... Hart N│
│  ┌─────────┐    ┌─────────┐         │
│  │  CLIC   │    │  CLIC   │  ...    │
│  │+ACLINT  │    │+ACLINT  │         │
│  └────┬────┘    └────┬────┘         │
│       │              │               │
│  ┌────▼──────────────▼────┐         │
│  │  PLIC 或 APLIC (共享)   │         │
│  └────────────────────────┘         │
└──────────────────────────────────────┘
```

**适用场景**：多核实时系统，每核需要独立的中断优先级管理
**组合**：每核 CLIC + 共享 ACLINT（MTIMER + MSWI + SSWI）+ 共享 PLIC/APLIC
**优势**：每核独立的中断管理，ACLINT 提供跨核 IPI 和共享定时器

### 7.4 场景四：大型 SMP 服务器系统（非实时）

```
┌──────────────────────────────────────┐
│  Hart 0          Hart 1    ... Hart N│
│  ┌─────────┐    ┌─────────┐         │
│  │  IMSIC  │    │  IMSIC  │  ...    │
│  │ (AIA)   │    │ (AIA)   │         │
│  └────┬────┘    └────┬────┘         │
│       │              │               │
│  ┌────▼──────────────▼────┐         │
│  │  APLIC (MSI 转换模式)    │         │
│  └────────────────────────┘         │
│  + ACLINT (timer + IPI)             │
│  无 CLIC                            │
└──────────────────────────────────────┘
```

**适用场景**：Linux/服务器级 SMP 系统，有 PCI 设备，需要虚拟化
**组合**：AIA（IMSIC + APLIC）+ ACLINT，不使用 CLIC
**优势**：MSI 支持、完善的虚拟化中断支持、大规模扩展能力
**说明**：此类系统不需要 CLIC 的低延迟向量化功能，AIA 的优先级配置已满足需求

### 7.5 场景五：带虚拟化的嵌入式系统

```
┌──────────────────────────────────────┐
│  Hart (M/S mode + H extension)       │
│  ┌───────────────────────────┐       │
│  │  CLIC (M/S mode)          │       │
│  │  + ACLINT                 │       │
│  └───────────┬───────────────┘       │
│              │                       │
│  ┌───────────▼───────────────┐       │
│  │  APLIC (有线路由模式)       │       │
│  │  无 IMSIC                 │       │
│  └───────────────────────────┘       │
│  注意：CLIC 不支持虚拟化中断           │
│  虚拟机中断需通过 hypervisor 注入      │
└──────────────────────────────────────┘
```

**适用场景**：需要 Hypervisor 但又要求实时中断响应的系统
**组合**：CLIC + ACLINT + APLIC（有线路由模式，无 IMSIC）
**限制**：CLIC 不提供虚拟化中断支持，虚拟机的外部中断需由 hypervisor 软件注入
**权衡**：如果虚拟化中断支持是刚需，应考虑使用 AIA 代替 CLIC

### 7.6 选型决策流程

```
是否需要 MSI（消息信号中断）？
├── 是 → 使用 AIA（IMSIC + APLIC）
│        └── 是否同时需要实时低延迟中断？
│             ├── 是 → 当前 AIA 不支持，需等待未来扩展
│             └── 否 → AIA + ACLINT，不使用 CLIC
│
└── 否 → 是否需要同级中断抢占/独立中断入口/细粒度优先级？
         ├── 是 → 使用 CLIC
         │        └── 是否有多核 IPI/定时器需求？
         │             ├── 是 → CLIC + ACLINT
         │             └── 否 → CLIC only
         │        └── 是否有外部 I/O 设备？
         │             ├── 是 → CLIC + PLIC 或 APLIC（有线路由）
         │             └── 否 → 无需 PLIC/APLIC
         │
         └── 否 → 使用 CLINT mode（基础中断机制）
                  └── 是否需要 S-mode IPI？
                       ├── 是 → ACLINT（含 SSWI）
                       └── 否 → SiFive CLINT 或 ACLINT（MTIMER + MSWI）
```

---

## 8. 附录：关键 CSR 与术语速查

### 8.1 CLIC 关键 CSR 一览

| CSR | M-mode 地址 | S-mode 地址 | 说明 |
|-----|-------------|-------------|------|
| xtvec | 0x305 | 0x105 | trap 向量基址/模式（mode=11 为 CLIC） |
| xtvt | 0x307 | 0x107 | trap 向量表基址（SHV 使用） |
| xnxti | 0x345 | 0x145 | 下一个中断处理地址 |
| xintstatus | 0xFB1 | 0xDB1 | 当前中断级别 |
| xintthresh | 0x347 | 0x147 | 中断级别阈值 |
| xscratchcsw | 0x348 | 0x148 | 跨特权模式 scratch 交换 |
| xscratchcswl | 0x349 | 0x149 | 跨中断级别 scratch 交换 |
| xiselect | 0x350 | 0x150 | 间接寄存器选择 |
| xireg | 0x351 | 0x151 | 间接寄存器别名 |
| xireg2 | 0x352 | 0x152 | 间接寄存器别名2 |

### 8.2 术语表

| 术语 | 全称 | 说明 |
|------|------|------|
| CLIC | Core-Local Interrupt Controller | 核心本地中断控制器 |
| CLINT | Core-Local Interruptor | SiFive 定义的原始本地中断器 |
| ACLINT | Advanced Core Local Interruptor | 模块化的 CLINT 升级版 |
| PLIC | Platform-Level Interrupt Controller | 平台级中断控制器 |
| APLIC | Advanced PLIC | AIA 定义的高级 PLIC |
| AIA | Advanced Interrupt Architecture | 高级中断架构 |
| IMSIC | Incoming MSI Controller | MSI 接收控制器 |
| SHV | Selective Hardware Vectoring | 选择性硬件向量化 |
| WARL | Write Any, Read Legal | RISC-V CSR 可写任意值读合法值的约定 |
| WPRI | Writes Preserve, Reads Ignore | 保留字段的读写约定 |
| mnxti | M-mode Next Interrupt | M-mode 下一个中断处理 CSR |
| mpil | M-mode Previous Interrupt Level | M-mode 前一中断级别 |
| inhv | In Hardware Vectoring | 硬件向量化进行中标志位 |
| IPI | Inter-Processor Interrupt | 核间中断 |
| MSI | Message-Signaled Interrupt | 消息信号中断 |

### 8.3 中断控制器对比总结

| 特性 | CLINT | ACLINT | CLIC | PLIC | APLIC | IMSIC |
|------|-------|--------|------|------|-------|-------|
| 类型 | 设备 | 设备 | ISA扩展 | 设备 | 设备 | ISA扩展+设备 |
| 作用域 | per-hart | per-hart | per-hart | 系统级 | 系统级 | per-hart |
| Timer | 是 | 是 | 否 | 否 | 否 | 否 |
| IPI | M-only | M+S | 否 | 否 | 否 | 是(MSI) |
| 优先级 | 固定 | 固定 | 可配置 | 可配置 | 可配置 | 可配置 |
| 向量化 | 有限 | 无 | 高级SHV | 无 | 无 | 无 |
| 抢占 | 仅特权级 | 仅特权级 | 256级 | N/A | N/A | N/A |
| MSI | 否 | 否 | 否 | 否 | 转换 | 接收 |
| 虚拟化 | 否 | 否 | 否 | 否 | 否 | 是 |
| 外部中断 | 否 | 否 | 否 | 是 | 是 | 是(MSI) |

---

> **参考资料**：
> - SPEC/clic_0.1/src/clic.adoc — CLIC 规范正文
> - SPEC/riscv-aia/src/intro.adoc — AIA 介绍
> - SPEC/riscv-aia/src/MSLevel.adoc — AIA 机器级和超级级中断
> - SPEC/riscv-aclint/riscv-aclint.adoc — ACLINT 规范
> - SPEC/clic_0.1/test-plan-clic.adoc — CLIC 测试计划
