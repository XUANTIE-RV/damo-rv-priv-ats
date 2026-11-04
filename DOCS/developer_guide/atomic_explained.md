# RISC-V 原子指令扩展详解（Zaamo / Zalrsc / Zars / Zawrs / Zama / Zabha / Zacas / Zalasr）

> 本文基于 SPEC 原文编写，解释 RISC-V 原子指令家族各扩展**定义了哪些内容**、**设计目的**、**使用上的异同**、**典型场景**以及**背景与注意点**。
>
> 依据的 SPEC 文件（均在 `SPEC/riscv-isa-manual/src/unpriv/` 下）：
>
> | 扩展 | SPEC 文件 | 行数 | 性质 |
> |------|-----------|------|------|
> | A（家族总述） | [za.adoc](../../SPEC/riscv-isa-manual/src/unpriv/za.adoc) | 72 | 章首总述 + include 入口 |
> | A（=Zalrsc+Zaamo） | [a-st-ext.adoc](../../SPEC/riscv-isa-manual/src/unpriv/a-st-ext.adoc) | 26 | 扩展组合说明 |
> | Zaamo | [zaamo.adoc](../../SPEC/riscv-isa-manual/src/unpriv/zaamo.adoc) | 83 | 指令扩展 |
> | Zalrsc | [zalrsc.adoc](../../SPEC/riscv-isa-manual/src/unpriv/zalrsc.adoc) | 281 | 指令扩展 |
> | Za64rs / Za128rs | [zars.adoc](../../SPEC/riscv-isa-manual/src/unpriv/zars.adoc) | 20 | 属性约束扩展 |
> | Zawrs | [zawrs.adoc](../../SPEC/riscv-isa-manual/src/unpriv/zawrs.adoc) | 106 | 指令扩展 |
> | Zama16b | [zama.adoc](../../SPEC/riscv-isa-manual/src/unpriv/zama.adoc) | 8 | 属性约束扩展 |
> | Zabha | [zabha.adoc](../../SPEC/riscv-isa-manual/src/unpriv/zabha.adoc) | 71 | 指令扩展 |
> | Zacas | [zacas.adoc](../../SPEC/riscv-isa-manual/src/unpriv/zacas.adoc) | 168 | 指令扩展 |
> | Zalasr | [zalasr.adoc](../../SPEC/riscv-isa-manual/src/unpriv/zalasr.adoc) | 138 | 指令扩展 |
>
> 相关章节：[rvwmo.adoc](../../SPEC/riscv-isa-manual/src/unpriv/rvwmo.adoc)（Atomicity Axiom）、[machine.adoc](../../SPEC/riscv-isa-manual/src/priv/machine.adoc)（AMO PMA / Reservability PMA / Misaligned Atomicity Granule PMA）、[atomics-examples.adoc](../../SPEC/riscv-isa-manual/src/unpriv/atomics-examples.adoc)（非规范性汇编示例）。

---

## 目录

1. [全景：Za\* 原子扩展家族](#1-全景za-原子扩展家族)
2. [共同的设计基础](#2-共同的设计基础)
3. [Zaamo —— 原子读-改-写（AMO）](#3-zaamo--原子读-改-写amo)
4. [Zalrsc —— LR/SC 保留加载与条件存储](#4-zalrsc--lrsc-保留加载与条件存储)
5. [Zars（Za64rs / Za128rs）—— 保留集大小约束](#5-zarsza64rs--za128rs-保留集大小约束)
6. [Zama（Zama16b）—— 16 字节非对齐原子性](#6-zamazama16b-16-字节非对齐原子性)
7. [Zawrs —— 等待保留集（低功耗轮询）](#7-zawrs--等待保留集低功耗轮询)
8. [Zabha —— 字节/半字 AMO](#8-zabha--字节半字-amo)
9. [Zacas —— 原子比较并交换（amocas）](#9-zacas--原子比较并交换amocas)
10. [Zalasr —— 原子 Load-Acquire / Store-Release](#10-zalasr--原子-load-acquire--store-release)
11. [横向对比：异同总表](#11-横向对比异同总表)
12. [场景选型指南](#12-场景选型指南)
13. [背景知识与常见陷阱](#13-背景知识与常见陷阱)
14. [本项目相关资产](#14-本项目相关资产)
15. [速查小结](#15-速查小结)

---

## 1. 全景：Za\* 原子扩展家族

### 1.1 A 扩展与 Za\* 命名体系

RISC-V 的原子能力**不再是一个单体的 "A 扩展"**，而是被拆分成一组可独立声明的 `Za*` 子扩展：

- **A 扩展 = Zalrsc + Zaamo**（`a-st-ext.adoc` 明确规定：*"The A extension comprises the Zalrsc and Zaamo extensions"*）。
- `Za*` 前缀表示 "Atomic 类别下的标准非特权扩展"；`Zic*` 前缀则表示与**内存系统属性（PMA）/一致性**相关的约束类扩展。
- 除 `A` 之外的每个 `Za*` 扩展都可以在 `misa.A` 之外单独出现在 ISA 字符串中（如 `rv64gc_zabha_zacas_zawrs`），因此软件与验证环境**必须逐个探测**，不能用"有 A 扩展"推断"有 amocas"。

SPEC 在 `za.adoc` 章首给出了整个家族的统一定位：

> RISC-V 提供若干扩展，对内存执行**原子读-改-写**，以支持在**同一内存空间**中运行的多个 RISC-V hart 之间的同步。提供两种形式的原子指令：**load-reserved/store-conditional** 与**原子 fetch-and-op**。两类原子指令都支持多种内存一致性排序，包括 unordered、acquire、release 与 sequentially consistent。这些指令使 RISC-V 得以支持 **RCsc（release consistency with sequentially consistent synchronization operations）** 内存一致性模型。

### 1.2 八份文档的分类速览

按"是否引入新指令"可以把这 8 份文档清晰地分成三类：

| 类别 | 扩展 | 一句话职责 | 引入的指令 |
|------|------|-----------|-----------|
| **基础指令扩展** | **Zaamo** | 原子 fetch-and-op（读-改-写） | `amoswap/amoadd/amoand/amoor/amoxor/amomin[u]/amomax[u].w/.d` |
| | **Zalrsc** | 保留加载 / 条件存储，用于构造任意复杂原子操作 | `lr.w/.d`、`sc.w/.d` |
| **能力补齐指令扩展** | **Zabha** | 把 AMO 扩展到**字节/半字**宽度 | `amo*.b`、`amo*.h`（含 `amocas.b/h`，需 Zacas） |
| | **Zacas** | 提供硬件 **CAS**，含 128 位 | `amocas.w/.d/.q` |
| | **Zalasr** | 提供**独立的**原子有序纯加载/纯存储 | `lb/lh/lw/ld.{aq,aqrl}`、`sb/sh/sw/sd.{rl,aqrl}` |
| **效率/辅助指令扩展** | **Zawrs** | 让 hart 低功耗等待某个内存位置被写 | `wrs.nto`、`wrs.sto` |
| **属性约束扩展（无新指令）** | **Zars**（`Za64rs`/`Za128rs`） | 约束 LR/SC **保留集**的最大尺寸与形状 | 无 |
| | **Zama**（`Zama16b`） | 约束**非对齐原子性粒度（MAG）**为 16 字节 | 无 |

关键区别：**Zars 与 Zama 不引入任何指令**，它们只是把"实现自由度"收窄成一个**软件可依赖的架构承诺**。这类扩展存在的意义在于：原子指令的很多行为（SC 何时失败、非对齐访问是否原子）在基础 SPEC 里是"实现可选"的，导致软件无法编写可移植的无锁代码；Zars/Zama 把这些可选行为标准化。

### 1.3 SPEC 章节的组织顺序

`za.adoc` 的 include 顺序本身就反映了扩展之间的依赖与层次：

```
za.adoc  (chapter intro + unified aq/rl rules)
 ├── a-st-ext.adoc   (A = Zalrsc + Zaamo)
 ├── zalrsc.adoc     (LR/SC)
 ├── zars.adoc       (reservation-set size constraint, attached to Zalrsc)
 ├── zawrs.adoc      (wait-on-reservation-set, attached to lr of Zalrsc)
 ├── zaamo.adoc      (AMO)
 ├── zalasr.adoc     (standalone atomic ordered load/store, builds on Zaamo/Zalrsc/Zabha)
 ├── zabha.adoc      (byte/halfword AMO, depends on Zaamo)
 ├── zacas.adoc      (CAS, depends on Zaamo)
 └── zama.adoc       (MAG=16B, constrains alignment rules of Zaamo/Zalasr)
```

### 1.4 与 Zic\* PMA 家族的分工

`Za*` 定义**指令**，`Zic*` 定义**内存区域必须具备的属性（PMA）**。二者配合才能保证"这条原子指令在这块内存上真的能用、真的原子"：

| Zic\* 扩展 | SPEC 文件 | 对内存区域的强制要求 | 服务于 |
|------------|-----------|---------------------|--------|
| **Ziccamoa** | `ziccamoa.adoc` | coherent + cacheable 主存必须提供 **AMOArithmetic** 级 PMA 支持 | Zaamo / Zabha 的全部算术 AMO |
| **Ziccamoc** | `ziccamoc.adoc` | coherent + cacheable 主存必须提供 **AMOCASQ** 级 PMA 支持 | Zacas 的 `amocas.q` |
| **Ziccrse** | `ziccrse.adoc` | coherent + cacheable 主存必须支持 **RsrvEventual** PMA | Zalrsc 的前向进展保证 |
| **Zicclsm** | `zicclsm.adoc` | coherent + cacheable 主存必须支持**非对齐 load/store**（SPEC 明确注明：**不包含任何 Za\* 扩展的指令**） | 普通访存，与 Za\* 正交 |
| **Zic64b** | `zic64b.adoc` | cache block 必须为 **64 字节**且自然对齐 | 与 Za64rs 配合推断保留集粒度 |

> **要点**：`Zicclsm` 的 NOTE 明确写着 *"It does not include any instructions in the various Z\*a\* extensions."* —— 也就是说，实现了 Zicclsm 只保证普通 `lw/sw` 等非对齐访问可用，**绝不意味着非对齐 AMO 可用**。非对齐 AMO 的可用性由 **MAG PMA / Zama16b** 决定。这是很容易搞混的一点。

---

## 2. 共同的设计基础

这一节的内容是**所有** `Za*` 指令扩展共享的规则，理解它可以避免在每个扩展里重复阅读相同的条款。

### 2.1 为什么需要原子指令

多 hart 共享内存时，"读-改-写"（Read-Modify-Write）序列如果由三条独立指令构成，就会在中途被其他 hart 插入，造成**丢失更新（lost update）**：

```asm
# Non-atomic counter++ (WRONG: demonstrates the lost-update race)
lw   t0, (a0)     # read
addi t0, t0, 1    # modify
sw   t0, (a0)     # write  <-- two harts doing this concurrently lose one increment
```

原子指令家族提供了两条解决路径：

1. **fetch-and-op（Zaamo/Zabha/Zacas）**：一条指令内完成"取旧值 → 运算 → 写回"，硬件保证不可分割；
2. **乐观并发（Zalrsc）**：先 `lr` 建立**保留（reservation）**，中间做任意计算，最后 `sc` 只在"期间没人动过这块内存"时才写成功，否则重试。

### 2.2 Release Consistency 与 aq / rl 两位

`za.adoc` 的 `norm:a_aq_rl_bits` 规定：**每条原子指令都有 `aq` 和 `rl` 两位**，用于指定"从其他 RISC-V hart 视角看"的额外内存排序约束。四种组合的语义（`norm:a_no_aq_rl` ~ `norm:a_seq_cst_semantics`）：

| aq | rl | 助记符后缀 | 语义 | 对应 C11/C++11 |
|----|----|-----------|------|----------------|
| 0 | 0 | （无） | **不施加任何额外排序约束** | `memory_order_relaxed` |
| 1 | 0 | `.aq` | **acquire**：本 hart 后续任何内存操作都不能被观测到早于该原子操作 | `memory_order_acquire` |
| 0 | 1 | `.rl` | **release**：该原子操作不能被观测到早于本 hart 先前的任何内存操作 | `memory_order_release` |
| 1 | 1 | `.aqrl` | **sequentially consistent**：既不能早于先前操作，也不能晚于后续操作 | `memory_order_seq_cst` |

`aq`/`rl` 位在编码中的位置是固定的（bit 26 = `aq`，bit 25 = `rl`），对 AMO/LR/SC/amocas/Zalasr 全部适用。

### 2.3 地址域限定：aq/rl 只约束"当前访问所在的那个域"

这是最容易被忽略的一条规范（`norm:a_domain_specific_ordering`）：

> `aq`/`rl` 位只对**两个地址域之一（memory 或 I/O）**排序，具体取决于该原子指令访问的是哪个域。对另一个域**不隐含任何排序约束**；跨域排序必须使用 `fence`。

实践含义：对一个 MMIO 设备寄存器做 `amoor.w.aq` **不能**用来排序此前对普通内存的写入。跨"内存 ↔ I/O"的发布顺序必须显式 `fence rw, rw`。

### 2.4 aq/rl 优于 fence 的原因

`zaamo.adoc` 的 NOTE 明确指出：虽然 `fence r,rw` 足以实现 acquire、`fence rw,w` 足以实现 release，但**二者都隐含了比"带对应 aq/rl 位的 AMO"更多的、不必要的排序**。

原因在于 fence 是**双向屏障**且作用于"所有前后访问"，而 `aq` 只禁止"后面的操作跑到前面来"（单向栅栏），`rl` 只禁止"前面的操作被推到后面去"。用 fence 实现 acquire 会额外阻止"前面的 store 与后面的 load 重排"这类本可允许的优化，压制硬件并行度。

`a-st-ext.adoc` 的 NOTE 进一步说明：用 A 扩展凑出 C++ 的 seq_cst load/store 是有代价的——

- **seq_cst load** 可用 `lr` + `aq` 实现，但 LR/SC 的**前向进展保证**会拖慢对同一有效地址的并发加载；
- **seq_cst store** 可用 `amoswap`（旧值写到 `x0`）+ `rl` 实现，但那次**多余的 load** 会强加本不需要的排序约束；
- **Zalasr 正是为了消除这两个缺陷而设计的**。

### 2.5 统一的操作数宽度与符号扩展规则

所有 `Za*` 指令扩展共享同一套宽度/扩展约定：

| 宽度后缀 | funct3 | 操作数大小 | 可用平台 | 引入扩展 |
|---------|--------|-----------|---------|---------|
| `.b` | `000` | 1 字节 | RV32/RV64 | Zabha（仅 AMO/CAS，**无 lr/sc**） |
| `.h` | `001` | 2 字节 | RV32/RV64 | Zabha（仅 AMO/CAS，**无 lr/sc**） |
| `.w` | `010` | 4 字节 | RV32/RV64 | Zaamo / Zalrsc / Zacas / Zalasr |
| `.d` | `011` | 8 字节 | **RV64 only** | Zaamo / Zalrsc / Zacas / Zalasr |
| `.q` | `100` | 16 字节 | **RV64 only** | Zacas（`amocas.q`）、Ziccamoc（PMA） |

**符号扩展的统一规则**：凡是从内存加载到 `rd` 的值，**若宽度小于 XLEN，一律符号扩展**（不是零扩展）。涉及：

- `norm:amo_operand_size`（Zaamo）：RV64 上 32 位 AMO 总是符号扩展写入 `rd`，并**忽略 `rs2` 原值的高 32 位**；
- `norm:Zabha_rd_sign_extension`（Zabha）：字节/半字 AMO 总是符号扩展写入 `rd`，并**忽略 `rs2` 原值的 `[XLEN-1 : 2^(width+3)]` 位**；
- `norm:Zacas_rv64_amocas-w_op`（Zacas）：RV64 上 `amocas.w` 加载的 32 位值**符号扩展**后写入 `rd`；
- `norm:zalasr_signext_rd` / `norm:ldaq_signext_rule`（Zalasr）：总是符号扩展写入 `rd`。

> **验证提示**：这意味着 `amomin.w` 与 `amominu.w` 的比较语义（有符号/无符号）与 `rd` 的写回形式（总是符号扩展）是**两件独立的事**。用 `0xFFFFFFFF` 做 `amoadd.w` 后读 `rd`，在 RV64 上应看到 `0xFFFFFFFF_FFFFFFFF`（若结果为 -1），而不是 `0x00000000_FFFFFFFF`。

### 2.6 统一的自然对齐要求

**所有** `Za*` 指令扩展都写着同一段话：`rs1` 中的地址**必须按操作数大小自然对齐**；否则产生 **address-misaligned 异常**或 **access-fault 异常**（后者用于"本可完成但因未对齐而不宜由软件模拟"的情形）。

- Zaamo：`norm:amo_alignment`（**并额外声明 MAG 可选放宽**）
- Zabha：`norm:Zabha_rs1_align_addr`（"与 Zaamo 相同的异常选项适用"）
- Zacas：`norm:Zacas_amocas_rs1_addr_alignment`（4 / 8 / 16 字节，"适用相同的异常选项"）
- Zalasr：`norm:zalasr_natural_align` + `norm:zalasr_misaligned_exception` + `norm:zalasr_misaligned_pma_relax`
- Zalrsc：`norm:lr_sc_alignment` —— **唯一不受 MAG 放宽的**（参见 §4.3）

真正的差异不在"是否要求对齐"，而在**"MAG 能否放宽"**：Zaamo/Zabha/Zacas/Zalasr 可以，**Zalrsc 永远不可以**。

### 2.7 异常归类：`lr` 走 Load 类，其余原子指令走 Store/AMO 类

原子指令中，只有 `lr` 是"纯读"，其余都涉及写。特权规范据此把它们**分成两类**（`machine.adoc`）：

| norm ID | 规定 |
|---------|------|
| `norm:mcause_exccode_ld_ldrsv` | **load 与 load-reserved 指令产生 load 类异常** |
| `norm:mcause_exccode_st_sc_amo` | **store、store-conditional 与 AMO 指令产生 store/AMO 类异常** |

由此得到完整的 cause 映射：

| cause | 名称 | 触发指令 |
|-------|------|---------|
| 4 | Load address misaligned | 普通 load、**`lr`** |
| **6** | **Store/AMO address misaligned** | 普通 store、**`sc`、所有 AMO（Zaamo/Zabha）、`amocas`（Zacas）、store-release（Zalasr）** |
| 5 | Load access fault | 普通 load、**`lr`** |
| **7** | **Store/AMO access fault** | 普通 store、**`sc`、AMO、`amocas`、store-release** |
| 13 | Load page fault | 普通 load、**`lr`** |
| **15** | **Store/AMO page fault** | 普通 store、**`sc`、AMO、`amocas`、store-release** |

PMP 与页表两条权限规范把这一点写得很具体：

| 规范 | 内容 |
|------|------|
| `norm:pmp_load_fault`（machine.adoc） | 执行 **load、load-reserved** 或 cache-block 管理指令，访问无读权限的 PMP 区域 → **load access-fault** |
| `norm:pmp_store_fault`（machine.adoc） | 执行 **store、store-conditional、AMO** 或 cache-block zero 指令，访问无写权限的 PMP 区域 → **store access-fault** |
| `norm:load_page_fault_no_r`（supervisor.adoc） | 执行 **load、load-reserved** 或 cache-block 管理指令，有效地址落在无读权限的页 → **load page-fault** |
| `norm:store_page_fault_no_w`（supervisor.adoc） | 执行 **store、store-conditional、AMO** 或 cache-block zero 指令，有效地址落在无写权限的页 → **store page-fault** |

`Zacas` 把"总是需要写权限"写成了显式规范：**`norm:Zacas_amocas_w_permission` —— `amocas.w/d/q` 总是需要写权限**。因此对只读页执行 `amocas`，即使比较必然失败（不会真的写），也必须报 **cause 15（Store/AMO page fault）**，报 cause 13 即为违规。

`Zalrsc` 对 `sc` 也给出了对应条款：

- `norm:sc_retire_permission`：**任何 `sc.w` 都不得在通过内存权限检查之前退休**；
- `norm:sc_failed_as_store`：**就内存保护而言，失败的 `sc.w` 可以被当作一次 store 处理**（即报 Store/AMO 类故障是合规的）；
- `norm:sc_failed_side_effects`：失败的 `sc` 是否产生隐式地址翻译/保护访问的副作用（如设置页表项 D 位）是 **UNSPECIFIED**。

> **Zalasr 的特殊性**：`zalasr.adoc` 未对 load-acquire / store-release 单独规定异常归类。按其"原子 load"/"原子 store"的本质，load-acquire 宜归 Load 类、store-release 宜归 Store/AMO 类；但由于二者编码位于 **AMO 操作码空间**，实际归类需以实现声明为准。验证时应**分别记录观测到的 cause** 并与上表比对，发现不一致时按项目原则报问题而非放宽判定。

---

## 3. Zaamo —— 原子读-改-写（AMO）

### 3.1 定义了什么

`zaamo.adoc` 定义了 9 条 AMO 指令（R-type 编码，opcode = `0101111`）：

| 指令 | funct5 | 运算 | 说明 |
|------|--------|------|------|
| `amoswap.w/.d` | `00001` | `mem = rs2` | 交换；旧值送 `rd` |
| `amoadd.w/.d` | `00000` | `mem = mem + rs2` | 加法 |
| `amoxor.w/.d` | `00100` | `mem = mem ^ rs2` | 按位异或 |
| `amoand.w/.d` | `01100` | `mem = mem & rs2` | 按位与（清位） |
| `amoor.w/.d` | `01000` | `mem = mem \| rs2` | 按位或（置位） |
| `amomin.w/.d` | `10000` | `mem = min(mem, rs2)` | **有符号**最小值 |
| `amomax.w/.d` | `10100` | `mem = max(mem, rs2)` | **有符号**最大值 |
| `amominu.w/.d` | `11000` | `mem = minu(mem, rs2)` | **无符号**最小值 |
| `amomaxu.w/.d` | `11100` | `mem = maxu(mem, rs2)` | **无符号**最大值 |

统一语义（`zaamo.adoc` 开头段）：

> AMO 指令**原子地**从 `rs1` 所指地址加载一个数据值放入 `rd`，对"加载的值"与"`rs2` 的原值"施加一个二元运算符，然后把结果存回 `rs1` 的原地址。

即一条指令完成三件事：`rd ← mem`、`mem ← op(mem, rs2)`、且整体不可分割。

### 3.2 设计目的（SPEC NOTE 的四个论点）

1. **可扩展性优于 LR/SC 与 CAS**：*"We provided fetch-and-op style atomic primitives as they scale to highly parallel systems better than LR/SC or CAS."* —— fetch-and-op 不需要"重试"，在高争用下不会像 LR/SC 那样反复失败重跑，也不像 CAS 那样需要软件循环。
2. **实现弹性大**：简单微架构可以**用 LR/SC 原语来实现 AMO**（前提是能保证 AMO 最终完成）；复杂实现可以把 AMO **下推到内存控制器**执行（near-memory / far atomic），减少缓存行迁移。
3. **可优化掉无用的返回**：当 `rd = x0` 时，实现可以**不取回原值**；配合 silent-store 检测还能在"运算结果与内存原值相同"时减少写流量。
4. **覆盖微控制器场景**：*"The Zaamo extension enables microcontroller class implementations to utilize atomic primitives ... Typically such implementations do not have caches and thus may not be able to naturally support the LR/SC instructions."* —— 无 cache 的 MCU 很难自然支持 LR/SC（保留跟踪通常建在私有 cache 上），但很容易支持 AMO。这是把 A 拆成 Zaamo/Zalrsc 两个独立扩展的**核心动机**。

AMO 操作集合的选择目标：**高效支持 C11/C++11 的 atomic memory operations**，以及**支持内存中的并行归约（parallel reduction）**；另一个用途是**对 I/O 空间中的内存映射设备寄存器做原子位更新**（置位/清位/翻转）。

### 3.3 MAG：对对齐要求的可选放宽

`norm:misaligned_atomicity_granule_size` 规定：**Misaligned Atomicity Granule（MAG）PMA** 可以**可选地放宽**上述自然对齐要求。

MAG 的完整定义在 `machine.adoc` 的 `sec:misaligned-atomicity-granule`：

- MAG 是一个**自然对齐的、2 的幂字节数**；用 `MAGnn` 表示（如 MAG16 表示粒度至少 16 字节）。
- **适用范围**（`norm:pma_mag_insts`）：AMO、基础 ISA 定义的 load/store、F/D/Q 扩展中**不超过 XLEN 位**的 load/store，以及它们的压缩编码。
- **效果**（`norm:pma_mag_op_within`）：若某指令访问的**全部字节都落在同一个 MAG 块内**，则①不因对齐原因产生异常，②就 RVWMO 而言**只产生一个内存操作**——即**原子执行**。
- **不满足时**：`norm:pma_mag_op_amo` —— 若区域未声明 MAG，或访问字节跨越了 MAG 块边界，则**产生异常**（AMO 没有"非原子执行"这个选项）。
- **明确不受 MAG 影响的三类访问**：
  - `norm:pma_mag_op_rsrv`：**LR/SC 不受该 PMA 影响，未对齐时总是产生异常**；
  - `norm:pma_mag_op_vec`：**向量访存不受影响**，即使落在 MAG 块内也可能非原子执行；
  - `norm:pma_mag_op_implicit`：**隐式访问**（如页表遍历、A/D 位更新）同样不受影响。
- `norm:pma_mag_exc`：实现可以对某些非对齐访问报 **access-fault** 而非 address-misaligned，表示"不该由 trap handler 模拟"。

> **验证提示（本项目已踩过的坑）**：`qemu-rv64-max` 声明 MAG=4096，因此非对齐 AMO 在粒度块内**不产生异常且原子执行是完全合规的**。任何针对"非对齐 AMO 必须报异常"的用例都必须写成**双分支验证**：
> - 分支 A：无异常 + 数据语义正确（MAG 存在且未跨界）；
> - 分支 B：有异常且 cause ∈ {4, 6, 5, 7}（MAG 不存在或跨界）。
>
> 参见 `Ziccamoa/tests/test_width_align.c` 的实现方式（改用**字节级读取**避免对齐单元干扰）。

### 3.4 使用场景

| 场景 | 推荐指令 | 说明 |
|------|---------|------|
| 全局计数器 / 统计量 | `amoadd.w/d`（`rd=x0`） | 返回值无用即写 `x0`，硬件可省掉回传 |
| 并行归约（多 hart 累加/求最值） | `amoadd` / `amomin[u]` / `amomax[u]`（`rd=x0`，不带 aq/rl） | SPEC 明示："Without ordering constraints, these AMOs can be used to implement parallel reduction operations" |
| 位标志置位/清位/翻转 | `amoor` / `amoand` / `amoxor` | 对设备寄存器尤其有用（AMOLogical 级 PMA 即可） |
| TTAS 自旋锁 | `amoswap.w.aq` 获取 + `amoswap.w.rl` 释放 | 见 §12.2 SPEC 示例 |
| seq_cst store（无 Zalasr 时的退路） | `amoswap.w.aqrl x0, rs2, (a0)` | 有多余 load 的排序代价 |
| 指针/句柄发布 | `amoswap.d.rl` | release 语义保证之前的初始化对获取方可见 |

### 3.5 注意点

1. **AMO 总是需要写权限**（即使 `rd=x0` 且运算结果与原值相同）。
2. **AMO 到 I/O 区域的支持是分级的**：可能只有 AMOSwap 或 AMOLogical 级（见 §13.1）。软件不能假设对某个 MMIO 寄存器做 `amoadd` 一定可用。
3. **AMO 不能用 LR/SC 序列里的"重试"心智模型理解**——AMO 保证一次完成，不存在失败重试。反过来，用 LR/SC 实现 AMO 的实现必须保证 AMO 最终完成。
4. `rd = x0` 的 AMO 依然是一次完整的原子 RMW（只是不回传旧值），**不能被降级成普通 store**（除非实现检测到 silent store 并优化写流量，这对软件不可见）。

---

## 4. Zalrsc —— LR/SC 保留加载与条件存储

`zalrsc.adoc` 是整个家族中**规范性内容最重（281 行）**的一份，因为它要定义一整套"乐观并发"的正确性条件与**前向进展（forward progress）保证**。

### 4.1 指令与语义

| 指令 | funct5 | rs2 | 语义 |
|------|--------|-----|------|
| `lr.w` / `lr.d` | `00010` | `00000` | 从 `rs1` 加载一个 word/doubleword，**符号扩展**后放入 `rd`，并**注册一个保留集（reservation set）**：一个**包含被寻址数据字/双字全部字节**的字节集合 |
| `sc.w` / `sc.d` | `00011` | 源数据 | **有条件地**把 `rs2` 写入 `rs1` 地址：仅当**保留仍然有效**且**保留集包含被写入的字节**时才成功 |

- `norm:sc_w_success`：`sc` 成功 → 写入内存，且**向 `rd` 写 0**；
- `norm:sc_w_failure`：`sc` 失败 → **不写内存**，且**向 `rd` 写非零值**；
- `norm:sc_failure_code`：**失败码 1 表示"未指明的失败"，其他失败码目前保留。可移植软件只应假设失败码非零。**（NOTE 解释：保留 1 是为了让简单实现直接复用 `slt/sltu` 已有的多路选择器）
- `norm:lr_sc_rv64`：`lr.d`/`sc.d` 仅在 RV64 可用；RV64 上 `lr.w`/`sc.w` 符号扩展 `rd`。
- `norm:lr_sc_atomicity_axiom`：成功的 LR/SC 序列的精确原子性要求由 `rvwmo.adoc` 的 **Atomicity Axiom** 定义：

> 若 `r` 和 `w` 是 hart `h` 中由**对齐的** `lr` 与 `sc` 配对产生的 load 与 store 操作，`s` 是对字节 `x` 的一次 store，且 `r` 返回了 `s` 写入的值，则 `s` 必须在全局内存序中先于 `w`，且**在全局内存序中不存在来自 `h` 以外 hart 的、对字节 `x` 的 store 位于 `s` 之后 `w` 之前**。

（Atomicity Axiom 的 NOTE 补充：理论上它支持宽度不同、地址不匹配的 LR/SC 配对，因为实现**允许**让这种 SC 成功；但实践中这种模式罕见且**被明确不鼓励**。）

### 4.2 保留集的规则

| norm ID | 规则 |
|---------|------|
| `norm:lr_reservation_set_size` | 实现可以在每次 `lr` 时注册**任意大**的保留集，只要它包含被寻址数据字/双字的全部字节 |
| `norm:sc_pairs_latest_lr` | **一条 `sc` 只能与程序序中最近的一条 `lr` 配对** |
| `norm:sc_reservation_invalidate` | **无论成功还是失败，执行 `sc` 都会使本 hart 持有的任何保留失效** |
| `norm:sc_not_observable_before_lr` | `sc` **绝不可能**被其他 hart 观测到早于建立该保留的 `lr` |
| `norm:sc_addr_not_in_reservation_fail` | 若地址不在最近一条 `lr` 的保留集内，`sc` **必须失败** |
| `norm:sc_other_hart_store_fail` | 若在 `lr` 与 `sc` 之间**可观测到**来自其他 hart 的对保留集的 store，`sc` **必须失败** |
| `norm:sc_other_device_write_fail` | 若在 `lr` 与 `sc` 之间可观测到**其他设备**对"`lr` 所访问的那些字节"的写，`sc` **必须失败**（若设备写的是保留集里**其他**字节，则 `sc` 可能成功也可能失败） |
| `norm:sc_intervening_sc_fail` | 若 `lr` 与该 `sc` 之间（程序序）存在**另一条 `sc`（对任意地址）**，该 `sc` **必须失败** |
| `norm:sc_success_conditions` | `sc` **只可能**在满足"无其他 hart 对保留集的 store"+"其间没有其他 `sc`"时成功 |
| `norm:sc_device_write_failure` | `sc` **只可能**在"无其他设备对 `lr` 访问字节的写"时成功 |

由 `sc_reservation_invalidate` + `sc_pairs_latest_lr` 可推出一个重要结论（SPEC NOTE 明示）：**一个 hart 同一时刻只能持有一个保留**；`sc` 只能与最近的 `lr` 配对，`lr` 只能与其后紧随的 `sc` 配对。这是对 Atomicity Axiom 的**额外限制**，目的是保证软件能在"预期的常见实现"上正确运行。

**虚拟地址别名**（NOTE）：在有内存翻译的系统中，若先前的 `lr` 通过**不同虚拟地址的别名**保留了同一物理位置，`sc` **允许成功，也允许失败**。

**遗留设备/总线**（NOTE）：非 hart 设备的写**只在重叠 `lr` 所访问的字节时**才被要求使保留失效；写保留集内的其他字节时不要求失效。

### 4.3 对齐：不受 MAG 放宽

`norm:lr_sc_alignment`：`lr`/`sc` 要求 `rs1` 按操作数大小自然对齐，否则产生 address-misaligned 或 access-fault 异常。

NOTE 给出理由：**在大多数系统中模拟非对齐 LR/SC 序列是不切实际的**；而且非对齐 LR/SC 会带来"一次访问多个保留集"的可能性，现有定义并未为此提供规定。配合 `machine.adoc` 的 `norm:pma_mag_op_rsrv`（LR/SC 不受 MAG PMA 影响，**未对齐时总是产生异常**），结论非常明确：

> **即使平台实现了 Zama16b（MAG=16），非对齐的 `lr`/`sc` 依然必须报异常。** 这是 Zaamo/Zalasr（受 MAG 放宽）与 Zalrsc（不受放宽）之间最关键的行为差异。

### 4.4 aq / rl 的放置规则

`zalrsc.adoc` 的 NOTE 给出了 LR/SC 序列的排序映射：

- 在 **`lr` 上置 `aq`** → 给整个 LR/SC 序列 **acquire** 语义；
- 在 **`sc` 上置 `rl`** → 给整个 LR/SC 序列 **release** 语义；
- 两者同时 → 序列在 C++ `memory_order_seq_cst` 意义上是**顺序一致**的。

两条重要限定：

1. **这样的序列并不充当"序列前后普通 load/store"的栅栏**（*"Such a sequence does not act as a fence for ordering ordinary load and store instructions before and after the sequence."*）；其他 C++ 原子操作的映射、或更强的顺序一致概念，**可能要求在 `lr` 或 `sc` 上同时置两位**。
2. **若两位都不置**，LR/SC 序列可被观测到发生在同 hart 周围内存操作之前或之后 —— 这**正适合用于实现并行归约**。

`norm:lr_sc_aq_rl_software_rule` 是一条**软件规则**（不是硬件行为规定）：

> 软件**不应**在 `lr` 上置 `rl`（除非同时也置了 `aq`），也**不应**在 `sc` 上置 `aq`（除非同时也置了 `rl`）。`lr.rl` 与 `sc.aq` **不保证提供比两位全清零更强的排序**，但**可能导致更低的性能**。

### 4.5 constrained LR/SC loop 与前向进展保证

这是 Zalrsc 最独特、也最难验证的部分。SPEC 定义了**受约束的 LR/SC 循环（constrained lr/sc loops）**，只有满足全部约束的循环才享有**活锁自由（livelock-freedom）**的架构保证。

**约束条件（5 条）：**

| norm ID | 约束 |
|---------|------|
| `norm:constrained_lrsc_loop_size` | 循环**只包含**一个 LR/SC 序列 + 失败重试代码，且**最多 16 条指令**、在内存中**顺序放置** |
| `norm:constrained_lrsc_instruction_set` | 序列以 `lr` 开始、以 `sc` 结束；`lr` 与 `sc` 之间**动态执行的代码只能包含 I 或 E 基础指令集**的指令，且**排除 load、store、backward jump、taken backward branch、`jalr`、`fence`、`system`** |
| `norm:constrained_lrsc_compressed_allowed` | 上述 I/E 指令在 **Zca（因而 Zca）与 Zcb** 中的**压缩形式也被允许** |
| `norm:constrained_lrsc_retry_code` | 重试代码**可以包含向后跳转/分支**以重复该序列，但除此之外受与"lr 与 sc 之间代码"相同的约束 |
| `norm:lrsc_eventuality_region` | `lr` 与 `sc` 的地址必须位于具有 **lr/sc eventuality（必然性）属性**的内存区域内；**执行环境负责告知哪些区域具备该属性**（对应 PMA 的 RsrvEventual，见 §13.1 / Ziccrse） |
| `norm:lrsc_same_address_and_size` | `sc` 必须与同 hart 最近执行的 `lr` **具有相同的有效地址与相同的数据大小** |

NOTE 解释了每条约束的动机：

- **16 条指令 / 64 连续指令字节**：避免对 I-cache 与 TLB 的尺寸和组相联度造成不当限制；
- **禁止循环内其他 load/store**：避免对"把保留跟踪在私有 cache 中"的简单实现造成数据 cache 组相联度限制；
- **限制分支/跳转**：限制序列内可花费的时间；
- **禁止浮点与整数乘除**：简化操作系统在缺乏相应硬件支持的实现上对这些指令的模拟（因为只允许 I/E 基础指令集，天然排除了 M/F/D）。

**前向进展保证**（`norm:constrained_lrsc_forward_progress_intro`）：若 hart `H` 进入一个 constrained LR/SC 循环，**执行环境必须保证下列事件之一最终发生**：

1. `norm:constrained_lrsc_forward_progress_sc`：`H` 或其他 hart 对 `H` 循环中 `lr` 的保留集**执行了一次成功的 `sc`**；
2. `norm:constrained_lrsc_forward_progress_store_amo`：其他 hart 对该保留集执行了**无条件 store 或 AMO**，或系统中**其他设备写**了该保留集；
3. `norm:constrained_lrsc_forward_progress_branch`：`H` 执行了**退出该循环的分支或跳转**；
4. `norm:constrained_lrsc_forward_progress_trap`：`H` **发生 trap**。

推论（NOTE）：若若干 hart 都在跑 constrained 循环，且没有其他 hart/设备对该保留集做无条件 store 或 AMO，则**至少有一个 hart 最终会退出循环**；反之，若其他 hart/设备持续写该保留集，则**不保证任何 hart 能退出**。

**不受约束的序列**（`norm:unconstrained_lrsc_no_progress`）：不满足上述约束的 LR/SC 序列称为 **unconstrained**，它们**在某些实现上可能有时成功，但在另一些实现上可能永远不成功**。NOTE 补充：**实现被允许无条件地让任何 unconstrained 序列失败**；软件并未被禁止使用它们，但**可移植软件必须检测序列反复失败的情形，并回退到不依赖 unconstrained 序列的替代代码**。

**其他 NOTE 要点：**

- 实现**允许因任何原因偶发地让 `sc` 失败**（spurious failure），只要不违反上述保证；
- **load 与 load-reserved 本身不会阻碍其他 hart 的 LR/SC 进展**：其他 hart（可能在同一核内）执行的 load/`lr` 不能无限期阻碍进展；例如**共享 cache 的另一个 hart 引起的 cache 逐出不能无限期阻碍进展**——这通常意味着**保留的跟踪要独立于共享 cache 的逐出**；同理，hart 内投机执行引起的 cache miss 也不能无限期阻碍进展；
- **早期版本规定的是更强的"无饥饿（starvation-freedom）"保证**，现改为更弱的**活锁自由**，因为它足以实现 C11/C++11，且在某些微架构风格下容易得多。

### 4.6 为什么 RISC-V 选 LR/SC 而不是（只选）CAS

`zalrsc.adoc` 的长 NOTE 记录了架构决策的完整理由：

**选择 LR/SC 的四点理由：**

1. **CAS 有 ABA 问题**，而 LR/SC **监视对该地址的所有写**（而非只检查数据值是否变化），因此天然避免 ABA；
2. CAS 需要**支持三个源操作数**（地址、比较值、交换值）的**新整数指令格式**，以及**不同的内存系统消息格式**，会复杂化微架构；
3. 其他系统为避免 ABA 提供 **double-wide CAS（DW-CAS）**（允许同时测试并递增一个计数器与一个数据字），这需要**一条指令读 5 个寄存器、写 2 个**，还需要**更大的新内存系统消息类型**，进一步复杂化实现；
4. **LR/SC 对许多原语的实现更高效**：只需**一次 load**，而 CAS 需要两次（一次在 CAS 之前 load 以获得用于投机计算的值，再一次作为 CAS 指令的一部分 load 以检查值是否未变）。

**LR/SC 的主要缺点与缓解：**

- 主要缺点是**活锁（livelock）**，RISC-V 通过**架构化的最终前向进展保证**（§4.5）在特定条件下规避；
- 另一个顾虑是 x86 的 DW-CAS 会影响同步库/软件的移植（这类软件假设 DW-CAS 是基本机器原语）；可能的缓解因素是 x86 新近加入的事务内存指令可能促使业界远离 DW-CAS；
- 更通用地说，**多字原子原语是人们期望的**，但其形式仍有相当争论，且保证前向进展会给系统增加复杂度。

> 值得注意的是：**Zacas 后来仍然引入了 CAS**（包括 128 位的 `amocas.q`，正是当年被否决的 DW-CAS 形态）。这说明上述"复杂度"顾虑在硬件生态成熟后被重新权衡了——CAS 的**无重试、确定性前进**特性对无锁算法与高争用场景有实质优势，而 `amocas.q` 用**寄存器对**而非"5 读 2 写"的形式规避了当年 DW-CAS 的格式难题。

### 4.7 软件必须注意的保留管理

SPEC NOTE 明确建议：**应当用一条指向"临时字（scratch word）"的 `sc` 来强制失效任何已存在的 load 保留**，在以下两种时机：

- **抢占式上下文切换时**；
- **改变虚实地址映射时**（例如迁移可能包含活跃保留的页面）。

平台层面：NOTE 指出**平台应当提供确定保留集大小与形状的手段**，且**平台规范可以约束保留集的大小与形状**（这正是 Zars 的作用，见 §5）。

### 4.8 使用场景

| 场景 | 用法 |
|------|------|
| 复杂原子操作（CAS、fetch-and-multiply、原子结构体字段更新） | `lr` → 任意 I/E 计算 → `sc` → 失败重试 |
| 无锁数据结构（链表/栈/队列的指针更新） | LR/SC 天然免疫 ABA |
| 需要"读到旧值再决定要不要写"的条件更新 | LR/SC 只需一次 load |
| 为 Zawrs 建立保留 | `lr` 是 `wrs.nto`/`wrs.sto` 的**前置必需指令** |
| 并行归约（不关心顺序） | `lr`/`sc` 两位全清零 |

### 4.9 注意点小结

1. **`sc` 失败不是 bug**：伪失败是被明确允许的，软件必须写成重试循环。
2. **不要在 LR/SC 之间插入 `pause`**：会使前向进展保证失效（详见 [zihintpause_explained.md](zihintpause_explained.md) §2.3）。同理不要插入 load/store/fence/system/向后跳转。
3. **保留集可能远大于访问的那个字**：两条位于同一 64 字节块但不同字的原子操作可能互相干扰。用 Za64rs/Za128rs 可以收窄这种不确定性。
4. **虚拟地址别名下 `sc` 可成功可失败**：不要依赖别名地址的 LR/SC 配对。
5. **`sc` 必须与最近的 `lr` 同有效地址、同数据大小**才在受约束循环之内。
6. **`sc` 退休前必须通过权限检查**，但失败 `sc` 是否设置页表 D 位是 UNSPECIFIED——验证时不能对 D 位做确定性断言。

---

## 5. Zars（Za64rs / Za128rs）—— 保留集大小约束

### 5.1 定义了什么

`zars.adoc` 只有 20 行、3 条规范，是家族中最短的一份：

| norm ID | 内容 |
|---------|------|
| `norm:za128rs_res_set_req` | **Za128rs** 要求 Zalrsc 指令使用的保留集是**连续的（contiguous）、自然对齐的（naturally aligned）、且最多 128 字节** |
| `norm:za64rs_res_set_req` | **Za64rs** 要求 Zalrsc 指令使用的保留集是**连续的、自然对齐的、且最多 64 字节** |
| `norm:za64rs_implies_za128rs` | **Za64rs 蕴含 Za128rs** |

三个约束维度缺一不可：

- **连续**：保留集不能是分散的多个块；
- **自然对齐**：块的起始地址必须是其大小的整数倍（例如 64 字节保留集必须起始于 64 字节对齐地址）；
- **尺寸上限**：`lr` 注册的保留集**不得超过** 64（或 128）字节。注意这是**上限**——`zalrsc.adoc` 的 `norm:lr_reservation_set_size` 允许实现注册任意大的保留集，Zars 正是把这个自由度封顶。

### 5.2 设计目的（解读）

> 以下为基于 SPEC 条款的设计意图分析，`zars.adoc` 本身未附 NOTE 说明。

`zalrsc.adoc` 给了实现极大的自由：保留集可以任意大、形状任意。这对硬件是友好的（直接按 cache line 甚至更大粒度跟踪），但对软件是灾难性的：

1. **伪冲突（false conflict）不可预测**：若保留集是 512 字节，那么对同一 512 字节块内**任何**字节的其他 hart 写都会让本 hart 的 `sc` 失败。软件无法推断"我把两个无关的原子变量放得多远才互不干扰"。
2. **无锁数据结构布局无法可移植**：链表节点、队列槽位若落在同一保留集内，多 hart 并发操作会互相踩踏，且踩踏程度随实现而异。
3. **性能不可预测**：高争用下 `sc` 失败率直接由保留集大小决定，跨平台性能差异可达数量级。

Zars 通过**封顶 + 强制自然对齐 + 强制连续**把这件事变成软件可依赖的架构承诺：

- 声明 **Za64rs** 的平台，软件可以确信"相距 64 字节以上、且各自 64 字节对齐的两个原子变量，其 LR/SC 序列不会因保留集重叠而互相失效"；
- 与 **Zic64b**（cache block = 64 字节）配合时，保留集粒度与 cache line 粒度对齐，实现与软件的心智模型完全一致——这也是为什么 64 字节是最常用的取值。

### 5.3 与其他扩展的关系

| 关系 | 说明 |
|------|------|
| **依附 Zalrsc** | Zars 约束的对象是"Zalrsc 扩展中指令使用的保留集"，无 Zalrsc 则 Zars 无意义 |
| **不影响 Zawrs 语义** | Zawrs 的停顿终止条件之一是"保留集被写"；保留集更小意味着**更少的伪唤醒**，但 Zawrs 本来就允许任意原因的伪唤醒，故不改变正确性 |
| **与 Zic64b 互补** | Zic64b 约束 cache block 尺寸，Zars 约束保留集尺寸；两者对齐时无锁代码布局最可预测 |
| **与 Ziccrse 正交** | Ziccrse 约束"主存区域必须支持 RsrvEventual（前向进展保证）"，Zars 约束"保留集多大"；前者管**能否前进**，后者管**冲突粒度** |
| **与页式虚存的既有约束叠加** | 见下 |

### 5.3.1 页式虚存下早已存在的保留集约束

Zars 并非第一个约束保留集的规范。`supervisor.adoc` 早已规定：

> **对于同时实现页式虚存与 A 扩展的实现，LR/SC 保留集必须完整地位于单个基物理页（base physical page）之内，即一个自然对齐的 4 KiB 物理内存区域。**

`priv/preface.adoc` 的变更记录也印证了这一点：*"Constrained the LR/SC reservation set size and shape when using page-based virtual memory."*

三者的层次关系：

| 规范来源 | 约束内容 | 适用条件 |
|---------|---------|---------|
| `supervisor.adoc`（基线） | 保留集**不得跨越 4 KiB 基页边界** | 实现了页式虚存 + A 扩展时**自动生效** |
| **Za128rs** | 保留集**连续、自然对齐、≤128 字节** | 需显式声明该扩展 |
| **Za64rs** | 保留集**连续、自然对齐、≤64 字节**（蕴含 Za128rs） | 需显式声明该扩展 |

因此 4 KiB 是"开了虚存就有的粗粒度上限"，Zars 是"必须显式声明的细粒度上限"。软件若只依赖基线规则，就必须按 4 KiB 间距布局无锁数据结构——这在实际中显然过于保守，这正是 Zars 的价值。

### 5.4 验证注意点

1. **上限无法直接观测**：架构上没有 CSR 报告保留集大小（`zalrsc.adoc` NOTE 只说"平台**应当**提供确定保留集大小与形状的手段"，属 SHOULD 而非 MUST）。可行的验证思路是**行为反证**：构造两个相距 N 字节的原子变量，一个 hart 对 A 做 `lr`，另一个 hart 写 B，再让第一个 hart 做 `sc`；若声明 Za64rs 却在 B 距 A 64 字节以上（且各自 64 字节对齐）时 `sc` 仍失败，即为不符合。
2. **必须区分"必须失败"与"允许失败"**：Zars 只限制保留集**尺寸**，并**不**保证"保留集外的写一定不让 `sc` 失败"之外的东西——`sc` 仍被允许因实现原因**伪失败**。因此单次 `sc` 失败不能作为违规证据，必须用**统计/重复**方式判定。
3. **蕴含关系可测**：声明 Za64rs 的平台必然满足 Za128rs 的要求，ISA 字符串中出现 `za64rs` 而无 `za128rs` 不构成违规（蕴含关系），但反之（只有 `za128rs` 却满足 64 字节）需要按声明的较弱者验证。

---

## 6. Zama（Zama16b）—— 16 字节非对齐原子性

### 6.1 定义了什么

`zama.adoc` 只有 8 行、2 条规范：

| norm ID | 内容 |
|---------|------|
| `norm:zama16b_mag` | 若实现了 **Zama16b**，则**同时具备 coherence 与 cacheability 两个 PMA 的主存区域**中，**非对齐原子性粒度（MAG）为 16 字节** |
| `norm:zama16b_atomic_ops` | 对这些区域、**不跨越自然对齐的 16 字节边界**的**非对齐 load、store 与 AMO** 是**原子的** |

对应 `machine.adoc` 的 `norm:zama16b_pma_mag_req`：*"The Zama16b extension requires MAG16 support to main memory regions."*

### 6.2 设计目的

单拷贝原子性（single-copy atomicity）是**无锁编程的基石**：一个 8 字节的指针发布，如果读方看到"一半旧值一半新值"，整个无锁算法就崩了。基础 SPEC 只保证**自然对齐**的访问是原子的，这让编译器在以下场景陷入困境：

- **打包结构体**（如 `{ uint32_t tag; uint32_t ptr_lo; }` 中 4 字节偏移处的 8 字节字段）；
- **JIT / 运行时代码生成**中动态计算的数据布局；
- **网络协议/文件格式**中按字节流排列的字段（天然可能非对齐）；
- **`std::atomic<T>` 且 `T` 的对齐要求小于其大小**的情形。

没有 MAG 承诺时，编译器只能：①插入 `fence` + 分裂访问（丢失原子性，改用锁）；②生成对齐检查与慢路径分支（代码膨胀）。**Zama16b 给出了一个明确的、可移植的承诺：只要不跨 16 字节边界，非对齐的 load/store/AMO 依然原子。** 16 字节这个数字选得很实用——它恰好覆盖"任何不超过 8 字节的原子对象在任意 8 字节对齐起点上的非对齐访问"，也覆盖 `amocas.q` 的 16 字节操作数。

### 6.3 关键边界：Zama16b 管什么、不管什么

| 访问类型 | 是否受 MAG / Zama16b 保护 | 依据 |
|---------|--------------------------|------|
| 基础 ISA 的非对齐 `lb/lh/lw/ld/sb/sh/sw/sd` | ✅ 在同一 16 字节块内则**原子且不报对齐异常** | `norm:pma_mag_insts`、`norm:zama16b_atomic_ops` |
| F/D/Q 扩展中 **≤ XLEN 位**的 load/store | ✅ | `norm:pma_mag_insts` |
| 上述指令的**压缩编码** | ✅ | `norm:pma_mag_insts` |
| **AMO**（Zaamo / Zabha） | ✅ 在同一块内则原子；**跨块或未声明 MAG 则必须异常**（`norm:pma_mag_op_amo`） | `norm:misaligned_atomicity_granule_size` |
| **Zalasr 的 load-acquire / store-release** | ✅ 显式规定（`norm:zalasr_misaligned_pma_relax` / `norm:zalasr_misaligned_single_op`） | `zalasr.adoc` |
| **LR/SC** | ❌ **完全不受影响，非对齐时总是产生异常** | `norm:pma_mag_op_rsrv` |
| **向量访存** | ❌ 不受影响，**即使在同一 MAG 块内也可能非原子执行** | `norm:pma_mag_op_vec` |
| **隐式访问**（页表遍历、A/D 位更新等） | ❌ 不受影响 | `norm:pma_mag_op_implicit` |
| **不具备 coherence + cacheability 双 PMA 的区域**（如 I/O、非一致内存） | ❌ Zama16b 的承诺**只覆盖 coherent + cacheable 主存** | `norm:zama16b_mag` |
| **跨越 16 字节自然对齐边界**的访问 | ❌ 不在承诺范围内 | `norm:zama16b_atomic_ops` |

### 6.4 与 Zicclsm 的区别（极易混淆）

| 维度 | **Zama16b** | **Zicclsm** |
|------|-------------|-------------|
| 约束对象 | 非对齐访问的**原子性**（MAG = 16B） | 非对齐 load/store 的**可用性（不报异常）** |
| 是否覆盖 AMO | ✅ 覆盖 AMO | ❌ **明确排除**所有 Za\* 扩展的指令 |
| 是否覆盖向量 | ❌ | ✅ NOTE 明确"包含向量访存" |
| 是否承诺原子 | ✅ | ❌ 只承诺"支持"，可能是多次内存操作、非原子 |
| 区域范围 | coherent + cacheable 主存 | coherent + cacheable 主存 |

一句话：**Zicclsm 保证"能跑"，Zama16b 保证"原子"**。Zicclsm 的 NOTE 还提醒：即使被强制支持，非对齐 load/store 也**可能极慢**；标准软件发行版**只应为了正确性而假设其存在，不应为了性能**。

### 6.5 验证注意点

1. **必须双分支验证**：平台是否声明 MAG 决定了"非对齐 AMO 无异常且原子"与"非对齐 AMO 报 cause 4/6/5/7"两种结果都合规。参见 §3.3 与 `Ziccamoa/tests/test_width_align.c`。
2. **边界内外要分别构造**：起点 `base+12`、长度 8 的访问**跨越** 16 字节边界（12..19 vs 16 边界）→ 不在承诺内；起点 `base+4`、长度 8（4..11）→ 在同一块内 → 必须原子。
3. **LR/SC 必须单独验证"总是异常"**：即使 Zama16b 存在，非对齐 `lr/sc` 报异常才是合规行为。
4. **向量访存不原子是合规的**：不能因为 MAG 存在就断言向量非对齐访问原子。
5. **QEMU 的 MAG 值可能远大于 16**（如 4096）：这是"实现提供更强能力"，合规；不能反过来用 QEMU 的行为去要求硬件。

---

## 7. Zawrs —— 等待保留集（低功耗轮询）

### 7.1 要解决什么问题

`zawrs.adoc` 开篇列出了三类"等待内存位置被更新"的常见模式：

1. **锁竞争者**等待锁变量被更新；
2. **消费者**等待空队列的尾部被生产者放入工作/数据——生产者可能是另一个 RISC-V hart、加速器设备、或外部 I/O agent；
3. **等待内存中的标志位被置起**以指示某事件发生，例如 hart 上的软件等待加速器设备在内存中设置 "done" 标志以表明先前提交的作业已完成。

这些场景都涉及**对内存位置的轮询**，而这种忙等循环是**能量的浪费**。

### 7.2 定义了什么

两条指令，编码在 **SYSTEM 操作码空间（opcode = `0x73`）**，`rd`/`funct3`/`rs1` 全为 0：

| 指令 | funct12 | norm ID | 行为 |
|------|---------|---------|------|
| `wrs.nto`（WRS-with-no-timeout） | `0x0d` | `norm:Zawrs_wrs-nto_stall_exec` | 使 hart **临时停顿在低功耗状态**，直到**对保留集发生 store** 或**观测到中断** |
| `wrs.sto`（WRS-with-short-timeout） | `0x1d` | `norm:Zawrs_wrs-sto_stall_duration` | 与 `wrs.nto` 相同，但**把停顿时长限定在一个实现定义的短超时**内；若无其他条件终止停顿，则在超时时终止 |

**使用前提**：软件先用 **`lr`** 注册一个包含目标内存位置全部字节的保留集，再执行 `wrs`。SPEC NOTE 明示：*"The instructions in the Zawrs extension are only useful in conjunction with the `lr` instruction, which is provided by the Zalrsc component of the A extension."* —— **Zawrs 事实上依附于 Zalrsc**。

### 7.3 停顿的维持条件

hart 可以在**下列条件全部满足**时保持停顿：

- (a) **保留集有效**；
- (b) 若是 `wrs.sto`，从停顿开始起的"短"时长**尚未过去**；
- (c) **任何特权级上本地使能的中断都没有 pending**（*regardless of the global interrupt enable at each privilege level* —— **无视各特权级的全局中断使能**）。

两条重要规范：

- `norm:Zawrs_stall_terminate`：**停顿期间，实现被允许因任何原因偶尔终止停顿并完成执行**（即**伪唤醒是合规的**）；
- `norm:Zawrs_exec_resume_rules`：`wrs.nto`/`wrs.sto` **遵循 `wfi` 指令关于"在本地使能的 pending 中断上恢复执行"的规则**。

因此 NOTE 提醒：**由于 `wrs.sto`/`wrs.nto` 可能因"对保留集的 store"以外的原因完成执行，软件很可能需要循环直到所需的 store 真正发生。**

`wrs.sto` 的超时量级（NOTE）：**在不同实现之间甚至同一实现内部差异可能很大；典型实现中该时长应大致在片上 cache miss 延迟、或无 cache 访问主存延迟的 10 到 100 倍范围内。** 软件据此判断自己的 deadline 是否已到。

指令**在所有特权级都可用**（*"These instructions are available in all privilege modes."*）。

### 7.4 特权级陷阱：TW 与 VTW

这是 Zawrs 与特权规范交互最紧密的部分，也是本项目 Hypervisor 交叉测试的重点：

| norm ID | 条件 | 结果 |
|---------|------|------|
| `norm:Zawrs_priv_illegal_instr_excp` | `mstatus.TW`（timeout wait）= 1，且 **`wrs.nto` 在 M-mode 以外的任何特权级执行**，且**未在实现定义的有界时间限制内完成** | **illegal-instruction 异常** |
| `norm:Zawrs_virtual_instr_excp` | 在 **VS- 或 VU-mode** 执行，`hstatus.VTW` = 1，**`mstatus.TW` = 0**，且 `wrs.nto` 未在实现定义的有界时间限制内完成 | **virtual-instruction 异常** |

要点：

1. **只有 `wrs.nto` 受 TW/VTW 管辖**，`wrs.sto` 不受（因为它自带短超时，不会无限期占用 hart）。
2. **触发异常的前提是"未在有界时间内完成"**——如果 `wrs.nto` 很快就因保留集被写或中断而完成，即使 TW=1 也不应报异常。这使得该用例在"没有第二个写者"的环境下才容易触发。
3. **TW=1 时非 M-mode 报 illegal-instruction；VTW=1 且 TW=0 时 VS/VU 报 virtual-instruction**——这是 RISC-V 中标准的"两级截获"模式（与 `wfi` 一致）。

### 7.5 与 wfi 的关键差异

SPEC NOTE 专门澄清：

> `wrs.nto` 与 `wfi` 不同，**并未规定在 U-mode 执行且管辖它的 `TW` 位为 0 时产生 illegal-instruction 异常**。`wfi` 通常不被期望在 U-mode 使用，且在许多系统上若在 U-mode 使用可能立即产生 illegal-instruction 异常。与 `wfi` 不同，**`wrs.nto` 被期望由 U-mode 软件在"等待内存但对该等待没有 deadline"时使用**。

设计意图很清楚：`wfi` 是"等中断"（系统级、通常属 OS/firmware），`wrs.nto` 是"等内存"（应用级、可下放给用户态自旋锁/无锁队列）。这使 Zawrs 成为**用户态节能同步**的正规手段。

### 7.6 与 Zihintpause 的对比

两者都用于自旋等待，但机制完全不同：

| 维度 | **Zihintpause (`pause`)** | **Zawrs (`wrs.nto`/`wrs.sto`)** |
|------|--------------------------|--------------------------------|
| 指令性质 | **HINT**，编码为 `fence w,0`；不改变架构状态 | **真实指令**，SYSTEM 操作码；使 hart 进入低功耗停顿 |
| 唤醒条件 | 无（只是"临时降低退休速率"，时长可为 0） | 保留集被 store / 中断 pending / 短超时（sto）/ 实现任意原因 |
| 是否需要 `lr` 前置 | 不需要 | **需要**（`wrs` 依赖 `lr` 建立的保留集） |
| 特权级截获 | 无 | `mstatus.TW` → illegal-instruction；`hstatus.VTW` → virtual-instruction |
| 节能力度 | 弱（节流/让出 SMT 资源） | 强（进入低功耗停顿状态） |
| 用于 LR/SC 序列内部 | **禁止**（会失去前向进展保证） | 不适用（`wrs` 本身在 `lr` 之后、不在 constrained 循环内） |
| 可移植性 | 极高（老实现当作近 NOP 的 fence） | 需探测扩展存在性 |

### 7.7 使用场景与注意点

**典型用法（等待标志位）：**

```asm
wait_flag:
    lr.w    t0, (a0)        # register a reservation set covering the flag word
    bnez    t0, done        # already set, exit
    wrs.nto                 # stall in low-power state: reservation-set store / interrupt
    j       wait_flag       # must loop and re-check (spurious wakeup is allowed)
done:
```

**注意点：**

1. **必须写成循环**：`wrs` 可能因任何原因返回，返回≠条件满足。
2. **`lr` 之后到 `wrs` 之间不要放会被 constrained 循环规则禁止的指令**——虽然 `wrs` 本身不属于 constrained LR/SC 循环的一部分（该循环要求 `lr` 紧跟 `sc`），但若同一保留集还要用于后续 `sc`，需注意 `wrs` 属 `system` 类编码，插入 `lr`/`sc` 之间会使序列变为 unconstrained，从而**失去前向进展保证**。
3. **中断使能状态不影响唤醒**：条件 (c) 明确"无视全局中断使能"，因此即使 `mstatus.MIE=0`，只要有本地使能的 pending 中断，`wrs` 也会结束停顿。这一点与 `wfi` 的规则一致，但与直觉不同——**关闭全局中断并不能让 `wrs` 睡得更久**。
4. **`wrs.nto` 在 U-mode 是预期用法**，不要按 `wfi` 的习惯假设它会立即报异常。
5. **虚拟化环境下**：Hypervisor 可通过 `hstatus.VTW` 截获 guest 的 `wrs.nto`（前提是 `mstatus.TW=0`），用于避免 guest 长时间占用物理 hart。

---

## 8. Zabha —— 字节/半字 AMO

### 8.1 要解决什么问题：模拟窄原子操作的四宗罪

`zabha.adoc` 开篇指出：Zaamo 与 Zacas 只提供 word/doubleword/（amocas 还有）quadword 的原子操作，**缺少子字（subword）数据类型的原子操作**迫使软件采用模拟策略：

- **位运算类**：可用 word 宽度的 `amo*` 位运算指令模拟（读-改-写整个 word，只影响目标字节）；
- **非位运算类**（add/min/max/swap/cas）：只能用 word 宽度的 `lr`/`sc` 模拟。

SPEC 列出这种模拟方式的**四个局限**：

1. **可扩展性与公平性**：在大规模或 **NUMA** 配置中，基于 `lr`/`sc` 的模拟在**高争用**条件下引入可扩展性与公平性问题；
2. **非幂等 I/O 区域的副作用**：在 non-idempotent I/O 内存区域上，用更宽的 `amo*` 模拟更窄的 AMO **可能产生意外副作用**（因为对 I/O 寄存器做了本不该发生的 word 宽度读/写）；
3. **误触发调试资源**：用更宽的 `amo*` 模拟更窄的 AMO，**有激活多余断点（breakpoint）或观察点（watchpoint）的风险**（宽访问覆盖了相邻字节，命中了针对相邻变量设置的 watchpoint）；
4. **代码膨胀**：缺乏原生子字原子支持时，编译器常常**内联代码序列**来提供模拟，这**增加代码体积**，进而影响系统性能与内存占用。

### 8.2 定义了什么

- **依赖关系**：`norm` 明示 **Zabha 依赖 Zaamo** 标准扩展。
- **提供的指令**（funct3 = `000` 字节 / `001` 半字，其余编码布局与 Zaamo 相同）：
  `amoadd.b/h`、`amoand.b/h`、`amoor.b/h`、`amoxor.b/h`、`amoswap.b/h`、`amomin.b/h`、`amominu.b/h`、`amomax.b/h`、`amomaxu.b/h`。
- **条件提供**：若**同时实现了 Zacas**，Zabha 进一步提供 **`amocas.b/h`**（funct5 = `00101`）。

三条核心规范：

| norm ID | 内容 |
|---------|------|
| `norm:Zabha_rd_sign_extension` | 字节/半字 AMO **总是符号扩展**放入 `rd` 的值，并**忽略 `rs2` 原值的 `[XLEN-1 : 2^(width+3)]` 位** |
| `norm:Zabha_amocas-BH_ignore_bits` | `amocas.b/h` 同样**忽略 `rd` 原值的 `[XLEN-1 : 2^(width+3)]` 位** |
| `norm:Zabha_rs1_align_addr` | 与 Zaamo 的 AMO 类似，**要求 `rs1` 中的地址按操作数大小自然对齐**；未对齐时**适用与 Zaamo 相同的异常选项** |

其中 `width` 即 funct3 的值：字节 width=0 → 忽略 `[XLEN-1:1]`；半字 width=1 → 忽略 `[XLEN-1:2]`。

`aq`/`rl`：与 Zaamo 和 Zacas 的 AMO 类似，Zabha 的 AMO **可选地提供 release consistency 语义**（用 `aq`/`rl` 位）以帮助实现多处理器同步。

### 8.3 明确不提供的东西

NOTE 只有一句，但非常重要：

> **Zabha 省略了对 `lr` 和 `sc` 的字节与半字支持，理由是实用性低（low utility）。**

含义：**`lr.b`/`sc.b`/`lr.h`/`sc.h` 是保留编码**，执行时应产生 illegal-instruction 异常。设计理由（解读）：LR/SC 的价值在于"中间可以插入任意计算"，而 1 字节/2 字节的原子变量通常只需要 swap/add/位运算/CAS 这些已有直接对应指令的操作；且保留集机制天然是按 cache line 粒度工作的，为子字宽度单独定义保留语义收益极小。

### 8.4 使用场景

| 场景 | 指令 | 说明 |
|------|------|------|
| `std::atomic<uint8_t>` / `atomic<bool>` 标志位 | `amoswap.b`、`amoor.b`、`amoand.b` | 避免内联 LR/SC 序列，代码体积大幅减小 |
| 8 位/16 位引用计数 | `amoadd.h` | 避免 word 宽度 LR/SC 的争用重试 |
| 8 位计数器数组（bitmap 计数、直方图） | `amoadd.b` | 相邻字节互不干扰，无伪冲突 |
| **非幂等 I/O 寄存器的窄原子更新** | `amoor.h` | 直接解决模拟方案的第 2 宗罪 |
| 与调试器共存的窄原子变量 | `amoadd.b` | 直接解决模拟方案的第 3 宗罪（不误触相邻 watchpoint） |
| 窄类型 CAS | `amocas.b/h`（需 Zacas） | 无锁窄类型指针/标签更新 |
| NUMA 高争用下的窄原子操作 | 任意 Zabha 指令 | 直接解决模拟方案的第 1 宗罪 |

### 8.5 注意点

1. **PMA 支持级别与对应宽度指令相同**：`machine.adoc` 的 `norm:pma_amo_zabha_req` 规定 *"The AMOs specified by the Zabha extension require the same level of support as the corresponding instructions in the Zaamo standard extension or the Zacas extension."* —— 即字节/半字 AMO 的可用性遵循同一套 AMONone/AMOSwap/AMOLogical/AMOArithmetic/AMOCAS\* 分级。
2. **符号扩展易错**：`amoadd.b` 对 `0xFF` 加 1，内存中是 `0x00`，但 `rd` 中写入的是旧值 `0xFF` 的**符号扩展**，即 RV64 上为 `0xFFFFFFFF_FFFFFFFF`。
3. **忽略 `rs2` 高位的含义**：向 `amoand.b` 的 `rs2` 传 `0xDEADBEEF`，只有 bit 0 参与运算（字节情形），其余位被忽略——不会污染内存的相邻字节。
4. **`amocas.b/h` 的存在性依赖两个扩展**：只有 Zabha 而无 Zacas 时，`amocas.b/h` 不可用；测试必须先探测 Zacas。
5. **`lr.b`/`sc.b`/`lr.h`/`sc.h` 必须报 illegal-instruction**：这是保留编码，是可直接验证的规范点（本项目 Zabha 测试方案即以此方式验证）。
6. **Zabha 无 CSR、无 `misa` 位**：探测只能靠"平台配置声明 + trap-armed 运行时探测"（执行一条 `amoadd.b` 并 armed 捕获 illegal-instruction）。

---

## 9. Zacas —— 原子比较并交换（amocas）

### 9.1 定义了什么

`zacas.adoc` 开篇：

> CAS 在作为硬件指令被支持时，为线程同步操作提供了**简单且通常更快**的方式。CAS 通常被**无锁（lock-free）与免等待（wait-free）算法**使用。本扩展定义了对 **32 位、64 位与 128 位（仅 RV64）**数据值操作的 CAS 指令。**Zacas 依赖 Zaamo 标准扩展。**

指令：`amocas.w`、`amocas.d`、`amocas.q`，funct5 = `00101`，funct3 分别为 `010`/`011`/`100`。

### 9.2 分平台的操作语义

**RV32：**

| norm ID | 指令 | 语义 |
|---------|------|------|
| `norm:Zacas_rv32_amocas-w_op` | `amocas.w` | 原子加载 `rs1` 地址处的 **32 位**值，与 `rd` 中的 **32 位**值比较；**按位相等**则把 `rs2` 中的 32 位值存入 `rs1` 原地址。**从内存加载的值放入 `rd`** |
| `norm:Zacas_rv32_amocas-d_op` | `amocas.d` | 原子加载 **64 位**值，与**寄存器对 `rd`/`rd+1`** 中的 64 位值比较；相等则把**寄存器对 `rs2`/`rs2+1`** 的 64 位值存入。加载值放入寄存器对 `rd`/`rd+1` |

RV32 `amocas.w` 伪代码（SPEC 原文）：

```
    temp = mem[X(rs1)]
    if ( temp == X(rd) )
        mem[X(rs1)] = X(rs2)
    X(rd) = temp
```

RV32 `amocas.d` 伪代码（SPEC 原文）：

```
    temp0 = mem[X(rs1)+0]
    temp1 = mem[X(rs1)+4]
    comp0 = (rd == x0)  ? 0 : X(rd)
    comp1 = (rd == x0)  ? 0 : X(rd+1)
    swap0 = (rs2 == x0) ? 0 : X(rs2)
    swap1 = (rs2 == x0) ? 0 : X(rs2+1)
    if ( temp0 == comp0 ) && ( temp1 == comp1 )
        mem[X(rs1)+0] = swap0
        mem[X(rs1)+4] = swap1
    endif
    if ( rd != x0 )
        X(rd)   = temp0
        X(rd+1) = temp1
    endif
```

**RV64：**

| norm ID | 指令 | 语义 |
|---------|------|------|
| `norm:Zacas_rv64_amocas-w_op` | `amocas.w` | 原子加载 **32 位**值，与 `rd` 值的**低 32 位**比较；相等则把 `rs2` 值的**低 32 位**存入。加载的 32 位值**符号扩展**后放入 `rd` |
| `norm:Zacas_rv64_amocas-d_op` | `amocas.d` | 原子加载 **64 位**值，与 `rd` 中的 64 位值比较；相等则把 `rs2` 的 64 位值存入。加载值放入 `rd`（单寄存器） |
| `norm:Zacas_rv64_amocas-q_op` | `amocas.q` | **仅 RV64**：原子加载 **128 位**值，与寄存器对 `rd`/`rd+1` 的 128 位值比较；相等则把寄存器对 `rs2`/`rs2+1` 的 128 位值存入。加载值放入 `rd`/`rd+1` |

RV64 `amocas.q` 伪代码与 RV32 `amocas.d` 结构相同，只是偏移为 `+0`/`+8`。

### 9.3 寄存器对的三条硬性规则

`amocas.d`（RV32）与 `amocas.q`（RV64）使用寄存器对，SPEC 给出了三条必须严格遵守的规则（对 d/q 各有一套同名 norm）：

| 规则 | RV32 `amocas.d` norm | RV64 `amocas.q` norm | 内容 |
|------|---------------------|---------------------|------|
| **偶数号约束** | `norm:Zacas_rv32_amocas-d_frst_pair_entry_reg_even` | `norm:Zacas_rv64_amocas-q_frst_pair_entry_reg_even` | 寄存器对的**第一个寄存器必须是偶数号**；`rs2` 与 `rd` 指定**奇数号寄存器的编码是保留的** |
| **源对为 x0** | `norm:Zacas_rv32_amocas-d_rs2_frst_reg_x0` | `norm:Zacas_rv64_amocas-q_rs2_frst_reg_x0` | 当源寄存器对的第一个寄存器是 `x0` 时，**该对的两半都读作 0** |
| **目的对为 x0** | `norm:Zacas_rv32_amocas-d_rd_frst_reg_x0` | `norm:Zacas_rv64_amocas-q_rd_frst_reg_x0` | 当目的寄存器对的第一个寄存器是 `x0` 时，**整个寄存器结果被丢弃，两个目的寄存器都不写** |

注意"目的对为 x0"的语义：`x0`/`x1` 作为目的对时，**连 `x1` 也不写**——这不是普通的"`x0` 吸收写入"，而是**整个结果被丢弃**。这是很容易在验证中搞错的一点。

### 9.4 对齐与权限

| norm ID | 内容 |
|---------|------|
| `norm:Zacas_amocas_rs1_addr_alignment` | 与 Zaamo 的 AMO 一样，`amocas.w/d/q` 要求 `rs1` 中的地址**按操作数大小自然对齐**（quadword 16 字节、doubleword 8 字节、word 4 字节）；未对齐时适用**相同的异常选项** |
| `norm:Zacas_amocas_w_permission` | **`amocas.w/d/q` 总是需要写权限** |

"总是需要写权限"是 Zacas 最重要的可验证规范点之一：**即使比较失败、内存不会被更新，也必须在执行前通过写权限检查**。对只读页执行 `amocas` 必须报 **Store/AMO page fault（cause 15）**，报 Load page fault（cause 13）即为违规。

### 9.5 aq / rl 语义：成功与失败不对称

这是 Zacas 独有、且与其他所有 `Za*` 扩展不同的一条规则：

| norm ID | 情形 | acquire | release |
|---------|------|---------|---------|
| `norm:Zacas_amocas_mem_op_success_aq_rl` | **成功**（比较相等） | `aq`=1 时有 acquire 语义 | `rl`=1 时有 release 语义 |
| `norm:Zacas_amocas_mem_op_fail_aq_rl` | **不成功**（比较不等） | `aq`=1 时**仍有** acquire 语义 | **没有 release 语义，无论 `rl` 为何** |

NOTE 进一步说明：

> 不成功的 `amocas.w/d/q` **可能不执行内存写，也可能把从内存加载的旧值写回**。该内存写（若产生）**不具有 release 语义，无论 `rl` 为何**。**不论实际是否执行了写，出于 RVWMO PPO（Preserved Program Order）规则的目的，该指令都被当作一个 AMO 对待。**

设计意图（解读）：release 语义的含义是"我之前的所有写都对看到这次写的人可见"。如果 CAS **失败**了，就没有"发布"任何东西——此时赋予 release 语义会让硬件强加不必要的排序，压制性能。而 acquire 语义在失败时仍然保留，因为**加载确实发生了**，软件接下来会基于加载到的值做决策，需要保证后续访问不会跑到这次加载之前。

同时，`fence` 指令**可以**用于对 `amocas.w/d/q` 的内存读访问、以及（若产生的）内存写访问进行排序。

### 9.6 关于"比较值来源"的重要澄清

NOTE：

> 某些算法可能把某个内存位置的**先前数据值**加载到"用作 Zacas 指令比较数据源"的寄存器中。当使用**以寄存器对**提供比较值的 Zacas 指令时，这两个寄存器**可以用两次独立的 load 加载**。这两次独立 load **可能读到不一致的一对值，但这不是问题**，因为 `amocas` 操作本身使用**来自内存的原子 load-pair** 来获取用于比较的数据值。

这条澄清消除了一个常见误解：软件**不需要**用原子方式加载 128 位的比较值（例如不需要先用 LR/SC 读一对），因为 CAS 内部会原子地重新读取内存值进行比较。这正是 `atomics-examples.adoc` 中 RV32 64 位计数器自增示例用两次 `lw` 加载 `a2`/`a3` 的原因。

### 9.7 PMA 支持级别

`machine.adoc` 为 Zacas 定义了三个额外的 AMO PMA 级别：

| norm ID | 内容 |
|---------|------|
| `norm:pma_amo_zacas_levels1` | Zacas 定义三个额外支持级别：**AMOCASW、AMOCASD、AMOCASQ** |
| `norm:pma_amo_zacas_levels2` | AMOCASW 表示在 AMOArithmetic 级别所指示的指令之外还支持 `AMOCAS.W`；AMOCASD 表示在 AMOCASW 之外还支持 `AMOCAS.D`；AMOCASQ 表示在 AMOCASD 之外还支持 `AMOCAS.Q` |
| `norm:pma_amo_zacas_req_arith` | **`AMOCASW/D/Q` 要求 AMOArithmetic 级别的支持**，因为 `AMOCAS.W/D/Q` 指令需要执行算术比较与交换操作的能力 |
| `norm:ziccamoc_pma_amo_req` | **Ziccamoc** 要求 coherent + cacheable 主存区域提供 **AMOCASQ 级**支持 |

即 PMA 级别是**累进的**：`AMONone ⊂ AMOSwap ⊂ AMOLogical ⊂ AMOArithmetic ⊂ AMOCASW ⊂ AMOCASD ⊂ AMOCASQ`。

### 9.8 使用场景

| 场景 | 指令 | 说明 |
|------|------|------|
| **RV32 上的 64 位原子计数器** | `amocas.d`（寄存器对） | 见 §12.2 SPEC 示例；无需 LR/SC 重试循环 |
| **无锁队列的 enqueue（避免 ABA）** | `amocas.q`（指针 + 修改计数器） | 见 §12.2 SPEC 示例；128 位 = 64 位指针 + 64 位 counter |
| 无锁栈/链表的指针更新 | `amocas.d`（RV64） | 确定性前进，不受 SC 伪失败影响 |
| 版本号 / 序列号保护的状态更新 | `amocas.w` | 典型 seqlock 变体 |
| 高争用下的锁获取 | `amocas.w` | 相比 LR/SC，失败即返回、不占用保留资源，NUMA 下更公平 |
| `atomic<T>::compare_exchange_weak/strong` | `amocas.w/d/q` | 直接单指令映射，无需内联 LR/SC 循环 |

### 9.9 注意点

1. **`amocas.q` 是 RV64-only**，且需要 **16 字节自然对齐**。
2. **奇数号寄存器对编码是保留编码** → 必须报 illegal-instruction，这是直接可验证的规范点。
3. **失败时的 release 语义缺失**是刻意设计，不要写成 bug。
4. **失败时可能写回旧值**：因此不能通过"观察内存是否被写"来判断 CAS 是否成功——必须看 `rd` 中返回的旧值是否等于比较值。
5. **总是需要写权限**：即使是"只读探测"式的 CAS 也会因缺写权限而故障。
6. **`amocas.b/h` 属于 Zabha 范畴**：只有 Zabha + Zacas 同时实现时才存在。

---

## 10. Zalasr —— 原子 Load-Acquire / Store-Release

### 10.1 定义了什么，以及填补了什么空白

`norm:zalasr_def`：

> **Zalasr 扩展在 RISC-V 中提供 load-acquire 与 store-release 指令。** 它们对高性能设计很重要，因为通过提供**单向栅栏（unidirectional fence）**，可以实现比仅用 fence 更细粒度的同步。Load-acquire 与 store-release 被**语言级内存模型广泛使用**：Java 与 C++ 内存模型都使用 acquire-release 语义，且 C++ 的 `atomic` 提供了意在**直接映射到 load-acquire 与 store-release 指令**的原语。

`norm:zalasr_builds_on_amo` 精确定位了空白所在：

> **Zalasr 扩展构建于 Zaamo、Zalrsc 与 Zabha 提供的原子支持之上以提供额外的原子操作，但它可以独立于它们实现。** Zaamo 与 Zabha 中所有的 AMO 操作都是**既加载又存储**的读-改-写操作。Zalrsc 提供了"只加载"或"只存储"的操作，但由于它被设计为对单个内存 word 或 doubleword 执行原子操作，Zalrsc 提供的 load 与 store **被设计为成对使用**：load-reserved 意味着后续会有一条 store-conditional，而 store-conditional 要求之前有一条 load-reserved 且其间没有其他 load 或 store。**因此 Zalrsc 并不提供通用的、原子且有序的 load 或 store。**

于是：

> **Zalasr 通过提供真正独立（standalone）的原子且有序的 load 与 store 来填补这一空白。**（`norm:zalasr_atomic_ordered`：Zalasr 指令是**支持排序注解的原子 load 与 store**。）
>
> **借助 Zaamo、Zabha、Zacas 与 Zalasr 的组合，所有 C++ 原子操作都可以用单条指令支持。**

### 10.2 指令与编码

编码在 **AMO 操作码空间（opcode = `0101111`）**，funct3 = width：

**Load Acquire**（`norm:ldaq_atomic_load_enc`，funct5 = `00110`，**`rs2` 字段固定为 0**）：

| 助记符 | width(funct3) | 平台 |
|--------|--------------|------|
| `lb.aq` / `lb.aqrl` | `000` | RV32/RV64 |
| `lh.aq` / `lh.aqrl` | `001` | RV32/RV64 |
| `lw.aq` / `lw.aqrl` | `010` | RV32/RV64 |
| `ld.aq` / `ld.aqrl` | `011` | **RV64 only**（`norm:ldaq_rv64_only`） |

**Store Release**（`norm:sdrl_atomic_store_enc`，funct5 = `00111`，**`rd` 字段固定为 0**）：

| 助记符 | width(funct3) | 平台 |
|--------|--------------|------|
| `sb.rl` / `sb.aqrl` | `000` | RV32/RV64 |
| `sh.rl` / `sh.aqrl` | `001` | RV32/RV64 |
| `sw.rl` / `sw.aqrl` | `010` | RV32/RV64 |
| `sd.rl` / `sd.aqrl` | `011` | **RV64 only**（`norm:sdrl_rv64_only`） |

### 10.3 语义规范点全表

**Load Acquire：**

| norm ID | 内容 |
|---------|------|
| `norm:ldaq_atomic_load_op` | 从 `rs1` **原子地**加载 2^width 字节内存，结果写入 `rd` |
| `norm:ldaq_signext_rule` | 若大小（2^width+3 位）**小于 XLEN，则符号扩展**以填满目的寄存器 |
| `norm:ldaq_aq_required` | **该 load 必须带 `aq` 排序注解** |
| `norm:ldaq_rl_optional` | **可以带 `rl` 排序注解** |
| `norm:ldaq_rcsc_semantics` | 该指令**总是带 acquire-RCsc 注解**；若 `rl` 置位则**另带 release-RCsc 注解** |
| `norm:ldaq_no_aq_reserved` | **不带 `aq` 位的版本是 RESERVED** |

**Store Release：**

| norm ID | 内容 |
|---------|------|
| `norm:sdrl_atomic_store_op` | **原子地**存储 2^width 字节 |
| `norm:sdrl_rl_required` | **该 store 必须带 `rl` 排序注解** |
| `norm:sdrl_aq_optional` | **可以带 `aq` 排序注解** |
| `norm:sdrl_rcsc_semantics` | 该指令**总是带 release-RCsc 注解**；若 `aq` 置位则**另带 acquire-RCsc 注解** |
| `norm:sdrl_no_rl_reserved` | **不带 `rl` 位的版本是 RESERVED** |

**通用属性：**

| norm ID | 内容 |
|---------|------|
| `norm:zalasr_signext_rd` | Zalasr 指令**总是符号扩展**放入 `rd` 的值 |
| `norm:zalasr_ignore_rs2_upper` | **忽略 `rs2` 值的高位**（store-release 只取低 2^width 位） |
| `norm:zalasr_natural_align` | 要求 `rs1` 中的地址**按操作数字节大小（2^width）自然对齐** |
| `norm:zalasr_misaligned_exception` | 未自然对齐则产生 **address-misaligned 或 access-fault 异常** |
| `norm:zalasr_misaligned_pma_relax` | **MAG PMA 可选地放宽该对齐要求** |
| `norm:zalasr_misaligned_single_op` | 若所有被访问字节位于同一 MAG 块内，则**不因对齐原因产生异常**，且就 RVWMO 而言**只产生一个内存操作**——即原子执行 |

### 10.4 为什么 `aq`（load）与 `rl`（store）是强制的

SPEC 的两条 NOTE 给出了完整理由，二者对称：

**Load Acquire 的 NOTE：**

> `aq` 位是强制的，因为由此会产生的两种编码**目前被认为没有用处**。既不带 `aq` 也不带 `rl` 的版本对应"一个不带排序注解、但保证原子执行的 load"——**这可以通过恰当对齐指针用普通 load 指令实现**。只带 `rl` 的版本对应 **load-release**：load-release 在 **seqlock** 中有理论应用，但**语言级内存模型不支持它**，故未纳入。

**Store Release 的 NOTE：** 完全对称——只带 `aq` 的版本对应 **store-acquire**，同样在 seqlock 中有理论应用但语言内存模型不支持。

因此：`lb/lh/lw/ld` 不带 `aq` 的 4 个编码，以及 `sb/sh/sw/sd` 不带 `rl` 的 4 个编码，共 **8 个编码是 RESERVED**，执行应报 illegal-instruction。

### 10.5 使用场景

| 场景 | 用法 | 相比 A 扩展的优势 |
|------|------|------------------|
| **单写多读的发布/订阅**（如 seqlock 读侧、无锁环形缓冲的 head/tail） | 写方 `sd.rl`，读方 `ld.aq` | 单指令，无需 `fence`，且不引入双向屏障 |
| **C++ `atomic<T>::load(memory_order_acquire)`** | `l{b,h,w,d}.aq` | 直接单指令映射（此前需 `lr.aq` 或 `lw + fence r,rw`） |
| **C++ `atomic<T>::store(memory_order_release)`** | `s{b,h,w,d}.rl` | 直接单指令映射（此前需 `amoswap.rl` 到 x0，带多余 load） |
| **seq_cst load/store** | `l*.aqrl` / `s*.aqrl` | 避免 `lr.aq` 触发 LR/SC 前向进展保证而拖慢同地址并发读 |
| **窄类型（byte/halfword）的有序原子访问** | `lb.aq` / `sh.rl` | Zabha 只提供 AMO，无窄宽度的有序纯 load/store |
| **指针发布 + 数据初始化的配对** | 初始化数据 → `sd.rl` 发布指针；消费方 `ld.aq` 读指针 → 读数据 | release/acquire 配对建立 happens-before |

### 10.6 注意点

1. **可独立实现**：`norm:zalasr_builds_on_amo` 明确"can be implemented independently of them"，因此**不能因为平台没有 A 扩展就跳过 Zalasr 探测**，反之亦然。本项目 `Zalasr_test_plan.md` 据此规定用例门控不以 A 扩展宏为前置。
2. **RCsc vs RCpc**：SPEC 用词是 **acquire-RCsc / release-RCsc**（RC**sc** = release consistency with **sequentially consistent** synchronization operations），这与 Ztso 等扩展涉及的 RCpc（processor consistency）不同。RCsc 注解之间的排序约束更强。完整的 PPO（Preserved Program Order）规则展开在 `rvwmo.adoc`。
3. **权限检查**：`zalasr.adoc` 未单独规定权限检查规则；store-release 本质是 store（必然需要写权限），load-acquire 本质是 load。由于编码位于 AMO 操作码空间，实际故障归类需以实现声明为准，验证时应分别记录观测到的 cause 并与 SPEC 的 Load / Store-AMO 分类比对。
4. **MAG 放宽适用**：与 Zaamo 相同，非对齐时"无异常且原子"与"报 cause 4/6/5/7"两种结果都可能合规，需双分支验证。
5. **`ld.aq`/`sd.rl` 在 RV32 上是保留编码**（RV64-only）。
6. **符号扩展**：`lb.aq` 加载 `0x80` 时，`rd` 得到 `0xFFFFFFFF_FFFFFF80`（RV64），不是 `0x80`。

---

## 11. 横向对比：异同总表

### 11.1 能力矩阵（宽度 × 操作类型）

| 操作类型 \ 宽度 | `.b`(1B) | `.h`(2B) | `.w`(4B) | `.d`(8B) | `.q`(16B) |
|----------------|----------|----------|----------|----------|-----------|
| **fetch-and-op AMO**（swap/add/and/or/xor/min[u]/max[u]） | **Zabha** | **Zabha** | **Zaamo** | **Zaamo**（RV64） | — |
| **CAS**（amocas） | **Zabha + Zacas** | **Zabha + Zacas** | **Zacas** | **Zacas**（RV32 用寄存器对 / RV64 用单寄存器） | **Zacas**（RV64，寄存器对） |
| **LR / SC** | ❌ **保留编码** | ❌ **保留编码** | **Zalrsc** | **Zalrsc**（RV64） | ❌ |
| **原子有序纯 load**（load-acquire） | **Zalasr** | **Zalasr** | **Zalasr** | **Zalasr**（RV64） | ❌ |
| **原子有序纯 store**（store-release） | **Zalasr** | **Zalasr** | **Zalasr** | **Zalasr**（RV64） | ❌ |
| **低功耗等待该地址被写** | **Zawrs**（依赖 `lr` 建立保留集） | 同左 | 同左 | 同左 | — |

**观察**：

- **Zalrsc 是唯一停留在 word/doubleword 的操作类型**（Zabha 明确因"实用性低"省略了 `.b`/`.h`）；
- **Zalasr 是唯一覆盖 byte/halfword 的"有序纯 load/store"**，这是 Zabha 无法提供的；
- **`.q`（128 位）只有 CAS 一条路**（`amocas.q`），不存在 128 位 AMO 或 128 位 LR/SC。

### 11.2 依赖关系

| 扩展 | 依赖 | 被依赖 / 关联 |
|------|------|--------------|
| **Zaamo** | — | 构成 A；被 Zabha、Zacas 依赖；Zalasr "构建于其上但可独立实现" |
| **Zalrsc** | — | 构成 A；**Zawrs 只在有 `lr` 时有用**；被 Zars 约束保留集；被 Ziccrse 保证前向进展 |
| **Zars**（Za64rs/Za128rs） | 事实上依附 Zalrsc | Za64rs **蕴含** Za128rs |
| **Zama**（Zama16b） | — | 放宽 Zaamo/Zabha/Zacas/Zalasr 的对齐要求；**不影响 Zalrsc、向量、隐式访问** |
| **Zawrs** | 需要 `lr`（Zalrsc） | 受 `mstatus.TW` / `hstatus.VTW` 管辖 |
| **Zabha** | **Zaamo**（规范明示） | `amocas.b/h` 还需 **Zacas** |
| **Zacas** | **Zaamo**（规范明示） | `amocas.q` 的 PMA 由 **Ziccamoc** 保证 |
| **Zalasr** | **可独立实现**（规范明示"although it can be implemented independently of them"） | 与 Zaamo+Zabha+Zacas 组合可单指令支持全部 C++ 原子操作 |

```
                  +----------- A -----------+
                  |                         |
               Zaamo                     Zalrsc
                  |                       |    |
          +-------+-------+        Zars --+    +-- Zawrs (requires lr)
          |               |      (Za64rs implies Za128rs)
        Zabha           Zacas
     (byte/half AMO)  (amocas.w/d/q)
          +-------+-------+
                  |  amocas.b/h needs both

     Zalasr  -- can be implemented independently, but semantically
                "builds on Zaamo/Zalrsc/Zabha"
     Zama16b -- relaxes alignment rules of Zaamo/Zabha/Zacas/Zalasr
                (does NOT relax Zalrsc)
```

### 11.3 aq / rl 语义对比

| 扩展 | aq/rl 位置 | 特殊规则 |
|------|-----------|---------|
| **Zaamo** | 每条 AMO 指令内 | 两位全置 = sequentially consistent |
| **Zabha** | 每条 AMO 指令内 | 与 Zaamo 相同（"similarly ... optionally provide release consistency semantics"） |
| **Zacas** | 每条 amocas 内 | **成功时**：aq→acquire、rl→release；**失败时**：aq→acquire、**rl 无效（永不 release）**；无论是否写，RVWMO PPO 中都按 AMO 对待 |
| **Zalrsc** | **`aq` 放在 `lr`、`rl` 放在 `sc`** | 序列不构成"前后普通访存"的栅栏；**软件不应单独使用 `lr.rl` 或 `sc.aq`**（不保证更强排序，可能更慢）；两位全清适合并行归约 |
| **Zalasr** | 指令内，但**有强制位** | load **必须** `aq`（否则 RESERVED）；store **必须** `rl`（否则 RESERVED）；注解为 **RCsc** 级别 |
| **Zawrs** | 无 aq/rl 位 | `wrs` 不是内存操作指令，不参与排序注解 |
| **Zars / Zama** | 不适用 | 属性扩展，无指令 |

### 11.4 对齐要求与 MAG 影响对比

| 扩展 | 自然对齐要求 | MAG 是否放宽 | 未对齐时的异常 |
|------|-------------|-------------|---------------|
| **Zaamo** | ✅ 按操作数大小 | ✅（`norm:misaligned_atomicity_granule_size`） | address-misaligned **或** access-fault；无 MAG 或跨界则必须异常 |
| **Zabha** | ✅ | ✅（沿用 Zaamo 的"相同异常选项"） | 同上 |
| **Zacas** | ✅（w=4B, d=8B, **q=16B**） | ✅（"the same exception options apply"） | 同上 |
| **Zalasr** | ✅ | ✅（`norm:zalasr_misaligned_pma_relax`） | 同上 |
| **Zalrsc** | ✅ | ❌ **完全不放宽**（`norm:pma_mag_op_rsrv`） | **总是**产生异常 |
| **Zama16b** | — | **它本身就把 MAG 定义为 16 字节** | — |

### 11.5 保留（RESERVED）编码汇总

这些编码都是**可直接验证的规范点**（执行应产生 illegal-instruction 异常）：

| 扩展 | 保留编码 | norm ID |
|------|---------|---------|
| **Zabha** | `lr.b`/`sc.b`/`lr.h`/`sc.h`（字节/半字 LR/SC，因"实用性低"被省略） | NOTE（非 norm 标注，但属规范性省略） |
| **Zacas** | RV32 `amocas.d` 的 `rd`/`rs2` 为**奇数号寄存器**；RV64 `amocas.q` 的 `rd`/`rs2` 为**奇数号寄存器** | `norm:Zacas_rv32_amocas-d_frst_pair_entry_reg_even`、`norm:Zacas_rv64_amocas-q_frst_pair_entry_reg_even` |
| **Zacas** | `amocas.q` 在 RV32 上不存在（RV64-only） | `norm:Zacas_rv64_amocas-q_op` |
| **Zalasr** | load-acquire **不带 `aq`** 的版本（4 个宽度） | `norm:ldaq_no_aq_reserved` |
| **Zalasr** | store-release **不带 `rl`** 的版本（4 个宽度） | `norm:sdrl_no_rl_reserved` |
| **Zalasr** | `ld.aq`/`ld.aqrl`/`sd.rl`/`sd.aqrl` 在 RV32 上不存在 | `norm:ldaq_rv64_only`、`norm:sdrl_rv64_only` |
| **Zaamo/Zalrsc** | `lr.d`/`sc.d`/`amo*.d` 在 RV32 上不存在（RV64-only） | `norm:lr_sc_rv64`、`norm:amo_operand_size` |
| **Zawrs** | `wrs.nto`/`wrs.sto` 之外的 funct12 组合（rd/funct3/rs1 必须为 0） | 编码图 |

### 11.6 权限与异常归类对比

| 扩展/指令 | 需要的权限 | 故障归类 |
|-----------|-----------|---------|
| **Zaamo AMO** | 读 + 写 | Store/AMO 类（cause 6/7/15） |
| **Zabha AMO** | 读 + 写；PMA 级别与对应 Zaamo/Zacas 指令相同 | Store/AMO 类 |
| **Zacas amocas** | **总是需要写权限**（`norm:Zacas_amocas_w_permission`），即使比较失败 | Store/AMO 类；只读页必须 cause 15 |
| **Zalrsc lr** | 读 | **Load 类**（`norm:mcause_exccode_ld_ldrsv`、`norm:pmp_load_fault`、`norm:load_page_fault_no_r`）：cause 4/5/13 |
| **Zalrsc sc** | **退休前必须通过内存权限检查**（`norm:sc_retire_permission`）；**失败的 sc 可被当作 store 处理**（`norm:sc_failed_as_store`） | **Store/AMO 类**（`norm:mcause_exccode_st_sc_amo`、`norm:pmp_store_fault`、`norm:store_page_fault_no_w`）：cause 6/7/15；失败 sc 是否设置页表 D 位为 **UNSPECIFIED** |
| **Zalasr load-acquire** | 读 | SPEC 未单独规定归类；按本质宜归 **Load 类**，但编码在 AMO 空间，以实现声明为准（见 §2.7 末尾说明） |
| **Zalasr store-release** | 写 | SPEC 未单独规定归类；按本质宜归 **Store/AMO 类** |
| **Zawrs wrs.\*** | 不涉及内存访问权限（依赖此前 `lr` 的保留） | `mstatus.TW`=1 且非 M-mode 且未在有限时间内完成 → **illegal-instruction**；`hstatus.VTW`=1 且 `mstatus.TW`=0 且 VS/VU-mode 且未完成 → **virtual-instruction** |

---

## 12. 场景选型指南

### 12.1 场景 → 指令选择

| 需求 | 首选 | 备选（无该扩展时） | 关键理由 |
|------|------|-------------------|---------|
| 全局计数器累加 | `amoadd.w/d`（`rd=x0`） | `lr`/`sc` 循环 | 无重试、可下推到内存控制器 |
| 位标志置位/清位 | `amoor` / `amoand` | `lr`/`sc` 或 word 宽度位运算 AMO | I/O 区域只需 AMOLogical 级 PMA |
| 8/16 位原子变量 | `amo*.b/h`（**Zabha**） | word 宽度 `lr`/`sc`（内联，代码膨胀） | 避免 NUMA 争用、I/O 副作用、误触 watchpoint |
| 自旋锁获取/释放 | `amoswap.w.aq` / `amoswap.w.rl`（TTAS：先 `lw` 本地自旋） | `lr.aq`/`sc.rl` 循环 | SPEC 推荐此 AMO swap 惯用法（便于投机锁消除） |
| 自旋锁**等待**节能 | `lr` + `wrs.nto`（**Zawrs**） | `pause`（Zihintpause） | wrs 真正进入低功耗停顿；pause 只是节流 HINT |
| 有 deadline 的等待 | `lr` + `wrs.sto`（**Zawrs**） | `pause` + 计数器 | sto 的短超时（约 10~100× cache miss 延迟）便于检查 deadline |
| CAS 语义（比较并交换） | `amocas.w/d/q`（**Zacas**） | `lr`/`sc` 循环（SPEC 示例仅 4 条指令） | CAS 确定性前进，无伪失败 |
| 避免 ABA 的指针更新 | `amocas.q`（指针 + 计数器） | `lr`/`sc`（天然免疫 ABA） | LR/SC 监视所有写，本就无 ABA 问题 |
| RV32 上的 64 位原子量 | `amocas.d`（寄存器对） | `lr.d` 不存在 → 只能用 `amocas.d` 或软件锁 | RV32 无 `.d` 的 LR/SC 与 AMO |
| acquire load | `l{b,h,w,d}.aq`（**Zalasr**） | `lr.w.aq`（受前向进展保证拖累）或 `lw` + `fence r,rw` | Zalasr 单指令、单向栅栏、无多余语义 |
| release store | `s{b,h,w,d}.rl`（**Zalasr**） | `amoswap.w.rl x0, rs2`（多余 load 强加排序）或 `sw` + `fence rw,w` | 同上 |
| seq_cst load/store | `l*.aqrl` / `s*.aqrl`（**Zalasr**） | `lr.aq` / `amoswap.rl` 到 x0 | `a-st-ext.adoc` NOTE 明确后者有额外代价 |
| 并行归约（多 hart 求和/最值） | `amoadd`/`amomin[u]`/`amomax[u]`，`rd=x0`，**aq/rl 全清** | `lr`/`sc`（两位全清） | 无排序约束时性能最佳 |
| 复杂条件原子更新（无对应 AMO 运算） | `lr` → 计算 → `sc` | `amocas` 循环 | LR/SC 允许中间任意 I/E 计算 |
| 非对齐的原子访问 | 需 **Zama16b / MAG** 且访问在同一粒度块内 | 对齐化数据结构或改用锁 | LR/SC 无论如何都不支持非对齐 |
| 推断两个原子变量的最小安全间距 | **Za64rs / Za128rs** + Zic64b | 保守假设（如 128B/256B 间距） | 保留集封顶使伪冲突可预测 |

### 12.2 SPEC 官方汇编示例（非规范性，摘自 `atomics-examples.adoc`）

**① 用 LR/SC 实现 CAS**（内联时仅需 4 条指令）：

```asm
    # a0 holds address of memory location
    # a1 holds expected value
    # a2 holds desired value
    # a0 holds return value, 0 if successful, !0 otherwise
cas:
    lr.w t0, (a0)        # Load original value.
    bne t0, a1, fail     # Doesn't match, so fail.
    sc.w t0, a2, (a0)    # Try to update.
    bnez t0, cas         # Retry if store-conditional failed.
    li a0, 0             # Set return to success.
    jr ra                # Return.
fail:
    li a0, 1             # Set return to failure.
    jr ra                # Return.
```

**② Test-and-Test-and-Set 自旋锁**（SPEC 推荐此 AMO swap 惯用法用于锁获取与释放，以简化投机锁消除的实现）：

```asm
    # a0 contains the address of the lock
    li           t0, 1        # Initialize swap value.
again:
    lw           t1, (a0)     # Check if lock is held.
    bnez         t1, again    # Retry if held.
    amoswap.w.aq t1, t0, (a0) # Attempt to acquire lock.
    bnez         t1, again    # Retry if held.
    # ...
    # Critical section.
    # ...
    amoswap.w.rl x0, x0, (a0) # Release lock by storing 0.
```

注意：第一个 AMO 标 `aq`（把锁获取排在临界区之前），第二个 AMO 标 `rl`（把临界区排在锁释放之前）。外层先用普通 `lw` 本地自旋，避免无谓的原子操作打爆缓存行——这就是 **test-and-test-and-set** 相对纯 test-and-set 的价值。

**③ RV32 上用 `amocas.d` 原子递增 64 位计数器**：

```asm
# a0 - address of the counter.
increment:
    lw   a2, (a0)      # Load current counter value using
    lw   a3, 4(a0)     # two individual loads.
retry:
    mv   a6, a2        # Save the low 32 bits of the current value.
    mv   a7, a3        # Save the high 32 bits of the current value.
    addi a4, a2, 1     # Increment the low 32 bits.
    sltu a1, a4, a2    # Determine if there is a carry out.
    add  a5, a3, a1    # Add the carry if any to high 32 bits.
    amocas.d.aqrl a2, a4, (a0)
    bne  a2, a6, retry # If amocas.d failed then retry
    bne  a3, a7, retry # using current values loaded by amocas.d.
    ret
```

这个例子同时印证了 §9.6 的规范澄清：**两次独立的 `lw` 可能读到不一致的一对值，但没有关系**——`amocas.d` 内部使用原子 load-pair 获取比较值。

**④ 用 `amocas.q` 实现无锁并发队列的 enqueue**（通过"指针 + 修改计数器"避免 ABA 问题）：

```asm
# Data structures used by the queue:
#   structure pointer_t {ptr:   node_t *, count: uint64_t}
#   structure node_t    {next: pointer_t, value: data type}
#   structure queue_t   {Head: pointer_t, Tail:  pointer_t}
# Inputs to the procedure:
#   a0 - address of Tail variable
#   a4 - address of a new node to insert at tail
enqueue:
    ld   a6, (a0)          # a6 = Tail.ptr
    ld   a7, 8(a0)         # a7 = Tail.count
    ld   a2, (a6)          # a2 = Tail.ptr->next.ptr
    ld   a3, 8(a6)         # a3 = Tail.ptr->next.count
    ld   t1, (a0)
    ld   t2, 8(a0)
    bne  a6, t1, enqueue   # Retry if Tail & next are not consistent
    bne  a7, t2, enqueue
    bne  a2, x0, move_tail # Was tail pointing to the last node?
    mv   t1, a2            # Save Tail.ptr->next.ptr
    mv   t2, a3
    addi a5, a3, 1         # Link the node at the end of the list
    amocas.q.aqrl a2, a4, (a6)
    bne  a2, t1, enqueue   # Retry if CAS failed
    bne  a3, t2, enqueue
    addi a5, a7, 1         # Update Tail to the inserted node
    amocas.q.aqrl a6, a4, (a0)
    ret
move_tail:                 # Tail was not pointing to the last node
    addi a3, a7, 1         # Try to swing Tail to the next node
    amocas.q.aqrl a6, a2, (a0)
    j    enqueue           # Retry
```

**⑤ Zawrs 节能等待（本文档综合示例，非 SPEC 原文）**：

```asm
# a0 - address of the flag word; wait until the flag becomes non-zero
wait_flag:
    lr.w    t0, (a0)       # register a reservation set covering the flag word
    bnez    t0, done       # condition already met, exit
    wrs.nto                # low-power stall: reservation-set store / pending interrupt
                           # / any implementation-defined reason
    j       wait_flag      # must loop and re-check (spurious wakeup is legal)
done:
```

---

## 13. 背景知识与常见陷阱

### 13.1 PMA 分级：原子能力是"分区域"的

`machine.adoc` 的 `sec:amo-pma` 规定，AMO 支持**按内存区域**分为四个基础级别：

| 级别 | 支持的操作 |
|------|-----------|
| **AMONone** | 不支持任何 AMO |
| **AMOSwap** | 仅 `amoswap` |
| **AMOLogical** | `amoswap` + `amoand`/`amoor`/`amoxor` |
| **AMOArithmetic** | A 扩展定义的**全部** AMO（再加 `amoadd`/`amomin`/`amomax`/`amominu`/`amomaxu`） |

Zacas 追加三个级别（累进）：**AMOCASW ⊃ AMOCASD ⊃ AMOCASQ**，且都要求 AMOArithmetic 级别。

LR/SC 的支持分为三个级别（`sec` Reservability PMA）：

| 级别 | 含义 |
|------|------|
| **RsrvNone** | 不支持 LR/SC（该位置**不可保留**） |
| **RsrvNonEventual** | 支持（可保留），但**没有**前向进展（最终成功）保证 |
| **RsrvEventual** | 支持**且**提供最终成功保证 |

关键规范：

- `norm:pma_amo_sz_support`：对每个支持级别，**若底层内存区域支持该宽度的读写，则支持该宽度的自然对齐 AMO**；
- `norm:pma_amo_far_subset_proc`：**主存与 I/O 区域可能只支持处理器所支持原子操作的一个子集，或完全不支持**；
- NOTE：**建议为 I/O 区域提供至少 AMOLogical 级支持**；**建议尽可能为主存区域提供 RsrvEventual 支持**；大多数 I/O 区域不会支持 LR/SC（因为 LR/SC 最方便地构建在缓存一致性之上），但有些可能支持 RsrvNonEventual 或 RsrvEventual；
- NOTE：**当 LR/SC 用于标记为 RsrvNonEventual 的内存位置时，软件应提供替代的回退机制，用于在检测到无法前进时使用。**

> **陷阱**：一个 hart 声称实现了 Zaamo，**不等于**对某个 MMIO 寄存器做 `amoadd` 不会报 access-fault。必须先确认该区域的 AMO PMA 级别。同理，在 RsrvNonEventual 区域跑 LR/SC 循环可能**永远失败**，这不是 bug。

### 13.2 Profile 层面的约束

各 `Za*`/`Zic*` 扩展在某一代应用处理器 profile（RVA20/RVA22/RVA23 等）中是**必需、可选还是排除**，由 **riscv-profiles** 规范规定。该仓库**不在本项目的 SPEC 子模块列表中**（见 `.gitmodules`），因此涉及 profile 合规性的结论必须以对应的 profile 文档为准，不能从本仓库的 ISA 手册推断。

本项目以 **RVA23S64** 认证体系为重要对标（参见 [tsbi_design_brief.md](../framework/tsbi_design_brief.md)），因此 Zaamo/Zalrsc 及其相关 Zic\* 属性扩展的验证优先级较高。

### 13.3 虚拟化：htinst 中的"transformed atomic instruction"

Hypervisor 扩展规定（`hypervisor.adoc`）：当 guest（VS/VU-mode）的**原子指令**引发需要模拟的 trap 时，写入 `htinst` 的不是原指令的简单拷贝，而是一个**变换后的指令（transformation）**：

> 对于标准原子指令（**load-reserved、store-conditional 或 AMO 指令**），transformed instruction 的格式为：**除 bits 19:15（Addr. Offset）外，所有字段都与触发 trap 的指令相同**。

- bits 19:15 原本是 `rs1` 字段，被替换为 **Addr. Offset**：即"写入 `mtval`/`stval` 的故障虚拟地址"与"`rs1` 中的地址"之间的**正差值**；
- 这样做的目的是**最小化硬件负担**，同时仍向 trap handler 提供模拟该指令所需的信息；
- **实现可以在任何时候用 0 替代 transformed instruction 来减少工作量**（即 `htinst = 0` 总是合规的）。

> **陷阱**：验证 `htinst` 时不能要求硬件一定给出 transformed 值；`htinst = 0` 与"正确的 transformed atomic instruction"都是合规结果，用例必须**双分支**接受。此外，`lr`/`sc`/AMO 全部归为同一个 transformed 格式，而普通 load/store 用的是另外两种格式（transformed load / transformed store，其 rs1 同样被替换为 Addr. Offset）。

### 13.4 多 hart 验证的现实约束

原子扩展的**核心价值只在多 hart 并发时才显现**，但验证成本极高：

| 验证维度 | 单 hart 可验证 | 必须多 hart |
|---------|--------------|------------|
| 指令数据语义（load/op/store 结果、rd 值、符号扩展） | ✅ | — |
| 编码/保留编码 → illegal-instruction | ✅ | — |
| 对齐要求与异常 cause | ✅ | — |
| 权限检查（PMP/PMA/页表） | ✅ | — |
| **原子性本身**（不可分割） | ❌（只能验证"结果正确"） | ✅ |
| **aq/rl 排序公理（RCsc）** | ❌（单 hart 下排序注解不改变数据语义） | ✅ litmus 测试 |
| **SC 失败条件**（其他 hart store 到保留集） | ❌ | ✅ |
| **前向进展保证**（livelock freedom） | ❌ | ✅ |
| **Zars 保留集尺寸约束** | ❌ | ✅（且需统计判定） |
| **Zawrs 被 store 唤醒** | ⚠️（可用中断/超时唤醒） | ✅ |

**本项目现状**：`common/entry.S` 目前**只支持 hart 0 运行**，因此：

- aq/rl 跨 hart 排序用例**只记录不断言**（见 Zabha/Zalasr 测试方案的范围说明）；
- 多 hart 用例整体标记为 **SKIP（非 FAIL）**，并注明"框架补充 secondary hart 启动与 hart 间同步原语前不覆盖"；
- **单 hart 平台不能声称完成了原子性或排序覆盖**——这是 `Zaamo_test_plan.md` 范围说明中的明确表述。

### 13.5 常见陷阱清单

| # | 陷阱 | 正确认知 |
|---|------|---------|
| 1 | 认为"实现了 A 扩展就有 amocas" | A = Zaamo + Zalrsc；amocas 属 **Zacas**，必须单独探测 |
| 2 | 认为 `sc` 失败是实现 bug | **伪失败被明确允许**；只有违反前向进展保证才是 bug |
| 3 | 认为非对齐 AMO 必须报异常 | 若 MAG 存在且访问在同一粒度块内，**无异常且原子执行是合规的**（必须双分支验证） |
| 4 | 认为 Zama16b 也放宽了 LR/SC 的对齐要求 | `norm:pma_mag_op_rsrv`：**LR/SC 不受 MAG 影响，未对齐总是异常** |
| 5 | 认为 Zicclsm 覆盖了非对齐 AMO | Zicclsm NOTE：**不包含任何 Za\* 扩展的指令** |
| 6 | 认为向量非对齐访问在 MAG 内也原子 | `norm:pma_mag_op_vec`：**向量访存不受影响，可能非原子执行** |
| 7 | 认为 `amocas` 失败时不需要写权限 | `norm:Zacas_amocas_w_permission`：**总是需要写权限**；只读页报 cause 15 |
| 8 | 认为 `amocas` 失败时 `rl` 仍提供 release | `norm:Zacas_amocas_mem_op_fail_aq_rl`：**失败时无 release，无论 `rl`** |
| 9 | 认为可以通过"内存是否被写"判断 CAS 成功与否 | 失败的 `amocas` **可能把旧值写回**；必须比较 `rd` 返回的旧值与比较值 |
| 10 | 认为 `amocas.d`(RV32)/`amocas.q`(RV64) 的 `rd=x0` 时 `rd+1` 仍被写 | `norm:..._rd_frst_reg_x0`：**整个结果被丢弃，两个寄存器都不写** |
| 11 | 认为奇数寄存器对的 `amocas.d/q` 会执行 | 这些编码是 **RESERVED** → illegal-instruction |
| 12 | 认为 `lr.b`/`sc.b`/`lr.h`/`sc.h` 属 Zabha | Zabha NOTE：**明确省略**，这些是保留编码 |
| 13 | 认为 Zalasr 依赖 A 扩展 | `norm:zalasr_builds_on_amo`：**可独立实现**，门控不能以 A 扩展为前置 |
| 14 | 认为 `lb.aq` 之外的 `lb.rl`（load-release）存在 | `norm:ldaq_no_aq_reserved`：**不带 `aq` 的版本是 RESERVED** |
| 15 | 认为 `lr.rl` / `sc.aq` 提供更强排序 | `norm:lr_sc_aq_rl_software_rule`：**不保证更强排序，可能更慢**；软件不应这样用 |
| 16 | 在 LR/SC 之间插入 `pause`、load/store、fence、system、向后跳转 | 使序列变为 **unconstrained**，**失去前向进展保证**；实现被允许无条件让其永远失败 |
| 17 | LR/SC 循环体超过 16 条指令 / 64 字节 | 超出即 **unconstrained** |
| 18 | 认为 `wrs.nto` 返回就说明标志已置位 | **允许因任何原因伪唤醒**；必须回环重新检查 |
| 19 | 认为关闭全局中断能让 `wrs` 睡得更久 | 停顿条件 (c)：**无视各特权级的全局中断使能**，只看"本地使能的中断是否 pending" |
| 20 | 认为 `wrs.nto` 在 U-mode 会像 `wfi` 一样立即报异常 | SPEC NOTE：**未规定** TW=0 时 U-mode 报 illegal-instruction；U-mode 使用是**预期用法** |
| 21 | 认为 `wrs.sto` 也受 TW/VTW 管辖 | 规范只针对 **`wrs.nto`** |
| 22 | 上下文切换 / 页迁移后未失效残留保留 | SPEC 建议：**用一条指向 scratch word 的 `sc` 强制失效任何已存在的保留** |
| 23 | 依赖不同虚拟地址别名的 LR/SC 配对 | 别名情形下 `sc` **允许成功也允许失败** |
| 24 | 认为 `htinst` 一定给出 transformed atomic instruction | 实现**可以在任何时候用 0 替代**；两者都合规 |
| 25 | 认为 `misa.A` 能反映所有子扩展 | 子扩展可独立声明；**探测未实现指令必须 trap-armed**（各模拟器对未实现指令的行为不一致） |
| 26 | 认为**所有**原子指令的故障都归 Store/AMO 类 | `norm:mcause_exccode_ld_ldrsv`：**`lr` 产生 Load 类异常**（cause 4/5/13）；只有 `sc`/AMO/`amocas`/store-release 归 Store/AMO 类（cause 6/7/15） |
| 27 | 开了虚存后认为保留集可以跨页 | `supervisor.adoc`：**LR/SC 保留集必须完整位于单个自然对齐的 4 KiB 基物理页内** |
| 28 | 假设模拟器/硬件行为正确 | 本项目原则：**以 SPEC 为准则**；若平台违反 SPEC，用例保持 FAIL 并记录至 `bugs/`，绝不为通过测试而降低标准或做 workaround |

### 13.6 未解/演进中的问题

- **多字原子原语**：`zalrsc.adoc` NOTE 指出"更通用地说，多字原子原语是人们期望的，但其形式仍有相当争论，且保证前向进展会给系统增加复杂度"。Zacas 的 `amocas.q` 是对这一问题的一种回答（128 位 CAS），但**不存在 128 位 AMO 或 128 位 LR/SC**。
- **更具体的 SC 失败码**：目前只定义了失败码 1 = "unspecified"，其他保留；NOTE 提到"未来版本或扩展可能定义更具体的失败码"。
- **`za.adoc` 章首自带提示**：*"This chapter is currently being restructured. Its contents are normative, but the presentation might appear disjoint."* —— 该章**仍在重构中**，内容是规范性的但组织形式可能显得零散。引用具体条款时应以 `norm:` 标记为准。
- **load-release / store-acquire**：Zalasr 明确不纳入（语言级内存模型不支持），但 NOTE 承认其在 **seqlock** 中有理论应用——未来可能被重新讨论。

---

## 14. 本项目相关资产

### 14.1 测试方案（`DOCS/testplan/`）

| 文档 | 覆盖对象 |
|------|---------|
| [Zaamo_test_plan.md](../testplan/Zaamo_test_plan.md) | 9 条 AMO 的 `.w`/`.d` 语义、宽度与符号扩展、对齐与 MAG、aq/rl |
| [Zalrsc_test_plan.md](../testplan/Zalrsc_test_plan.md) | LR/SC 保留集、SC 成功/失败条件、constrained 循环与前向进展 |
| [Za128rs_test_plan.md](../testplan/Za128rs_test_plan.md) | 保留集 ≤128 字节上界、自然对齐与连续性、假冲突自由、4 KiB 基页基线叠加 |
| [Za64rs_test_plan.md](../testplan/Za64rs_test_plan.md) | 保留集 ≤64 字节上界（更严）、Za64rs ⇒ Za128rs 蕴含关系与跳计划口径一致性 |
| [Zabha_test_plan.md](../testplan/Zabha_test_plan.md) | 字节/半字 AMO、`rd` 符号扩展、`rs2`/`rd` 高位忽略、`lr.b/h` 保留编码 |
| [Zacas_test_plan.md](../testplan/Zacas_test_plan.md) | `amocas.w/d/q` 语义、寄存器对与 `x0` 规则、成功/失败的 aq/rl 不对称、写权限 |
| [Zalasr_test_plan.md](../testplan/Zalasr_test_plan.md) | load-acquire / store-release 编码、强制位与 RESERVED、RCsc 注解、MAG |
| [Zawrs_test_plan.md](../testplan/Zawrs_test_plan.md) | `wrs.nto`/`wrs.sto` 停顿条件、TW/VTW 截获、伪唤醒 |
| [Hypervisor_Za_test_plan.md](../testplan/Hypervisor_Za_test_plan.md) | **Za 系列原子扩展与 Hypervisor 的虚拟化交集中心文档**（含 Zalrsc、Zawrs） |
| [Ziccamoa_test_plan.md](../testplan/Ziccamoa_test_plan.md) | 主存 AMOArithmetic 级 PMA 要求 |
| [Ziccamoc_test_plan.md](../testplan/Ziccamoc_test_plan.md) | 主存 AMOCASQ 级 PMA 要求 |
| [Ziccrse_test_plan.md](../testplan/Ziccrse_test_plan.md) | 主存 RsrvEventual（LR/SC 前向进展）要求 |
| [Zicclsm_test_plan.md](../testplan/Zicclsm_test_plan.md) | 主存非对齐 load/store 支持（**不含 Za\***） |

### 14.2 已实现的测试目录

| 目录 | 说明 |
|------|------|
| `Zawrs/` | Zawrs 测试套件 |
| `Ziccamoa/` | 主存 AMO PMA 测试套件（含**非对齐 AMO 双分支验证**的参考实现 `tests/test_width_align.c`） |
| `Ziccamoc/` | 主存 CASQ PMA 测试套件 |
| `Ziccrse/` | 主存可保留性测试套件 |
| `Zicclsm/` | 主存非对齐访存测试套件 |
| `Za64rs/` | 保留集大小约束测试套件**主目录**（持有全部共享源码，`-DZARS_BOUND=64`） |
| `Za128rs/` | 保留集大小约束测试套件（仅 `Makefile` 为实体文件，其余 symlink 到 `Za64rs/`，`-DZARS_BOUND=128`；沿用 `Sv39x4` → `Sv48x4`/`Sv57x4` 模式） |

### 14.3 功能覆盖点（`COVERPOINTS/`）

| 文件族 | 覆盖对象 |
|--------|---------|
| `EndianZaamo_coverage*.svh` / `EndianZalrsc_coverage*.svh` | AMO / LR-SC 指令的编码与大小端功能覆盖 |
| `ExceptionsZaamo_coverage*.svh` / `ExceptionsZalrsc_coverage*.svh` | AMO / LR-SC 触发的异常覆盖 |
| `ExceptionsSvZaamo_coverage*.svh` / `ExceptionsSvZalrsc_coverage*.svh` | 分页（Sv\*）环境下 AMO / LR-SC 的异常覆盖 |
| `PMPZaamo_coverage*.svh` / `PMPZalrsc_coverage*.svh` | PMP 保护下 AMO / LR-SC 的访问覆盖 |

### 14.4 相关文档

- [zihintpause_explained.md](zihintpause_explained.md) —— 自旋等待的另一条路径（`pause` HINT），含与 LR/SC 的交互约束
- [create_test.md](create_test.md) —— 测试用例开发流程
- `common/mem_ops.h` —— AMO 原语封装（`mem_amo_add_d`、`mem_amo_and_d`、`mem_amo_xor_d` 等）

---

## 15. 速查小结

| 扩展 | 全称 | 指令 | 依赖 | CSR | 核心价值 | 最需注意的一点 |
|------|------|------|------|-----|---------|--------------|
| **Zaamo** | Atomic Memory Operations | `amoswap/amoadd/amoand/amoor/amoxor/amomin[u]/amomax[u].w/.d` | — | 无 | fetch-and-op，高并行下可扩展性优于 LR/SC 与 CAS；无 cache 的 MCU 也能实现 | MAG 可放宽对齐；`.d` 仅 RV64；RV64 上 `.w` 符号扩展且忽略 `rs2` 高 32 位 |
| **Zalrsc** | Load-Reserved / Store-Conditional | `lr.w/.d`、`sc.w/.d` | — | 无 | 支持任意复杂原子操作；天然免疫 ABA；只需一次 load | **constrained 循环（≤16 指令 / 64 字节 / 仅 I-E 基础指令）才享有前向进展保证**；未对齐**总是**异常（不受 MAG 放宽）；`sc` 伪失败合规；**`lr` 归 Load 类异常、`sc` 归 Store/AMO 类**；开虚存时保留集**不得跨 4 KiB 基页** |
| **Za64rs / Za128rs**（Zars） | Reservation-Set Size | 无 | 依附 Zalrsc | 无 | 保留集**连续、自然对齐、≤64/≤128 字节**，使伪冲突可预测 | 是**上限**约束；Za64rs **蕴含** Za128rs；架构上无 CSR 报告尺寸；与页式虚存的"不得跨 4 KiB 基页"基线约束**叠加生效** |
| **Zama16b**（Zama） | 16-byte Misaligned Atomicity | 无 | — | 无 | coherent+cacheable 主存的 **MAG = 16 字节**；不跨 16 字节边界的非对齐 load/store/AMO **原子** | **不影响 LR/SC、向量访存、隐式访问**；与 Zicclsm（只管"能用"不管"原子"）区分 |
| **Zawrs** | Wait-on-Reservation-Set | `wrs.nto`(funct12=0x0d)、`wrs.sto`(0x1d) | 需要 `lr`（Zalrsc） | 受 `mstatus.TW`、`hstatus.VTW` 管辖 | 低功耗等待内存被写（锁等待、队列消费、设备 done 标志） | **允许任意原因伪唤醒 → 必须循环**；停顿条件 (c) **无视全局中断使能**；TW→illegal-instruction、VTW→virtual-instruction **仅针对 `wrs.nto`**；U-mode 使用是预期用法 |
| **Zabha** | Byte and Halfword AMOs | `amo*.b`、`amo*.h`；`amocas.b/h`（需 Zacas） | **Zaamo** | 无、无 `misa` 位 | 消除窄原子操作模拟的四宗罪：NUMA 争用、非幂等 I/O 副作用、误触 watchpoint、代码膨胀 | **明确省略 `lr.b/h`、`sc.b/h`（保留编码）**；`rd` 符号扩展；忽略 `rs2`（amocas 还有 `rd`）的 `[XLEN-1:2^(width+3)]` 位 |
| **Zacas** | Atomic Compare-and-Swap | `amocas.w/.d/.q`（funct5=00101） | **Zaamo** | 无 | 硬件 CAS，无锁/免等待算法；`amocas.q`（128 位）解决 ABA；RV32 上唯一的 64 位原子手段 | 寄存器对**必须偶数号**（奇数保留）；目的对首寄存器为 `x0` 时**两个寄存器都不写**；**总是需要写权限**；**失败时无 release 语义**；失败时**可能写回旧值** |
| **Zalasr** | Atomic Load-Acquire / Store-Release | `lb/lh/lw/ld.{aq,aqrl}`（funct5=00110）、`sb/sh/sw/sd.{rl,aqrl}`（00111） | **可独立实现** | 无 | 真正独立的原子有序纯 load/store；单向栅栏比 fence 更细粒度；直接映射 C++/Java acquire-release | load **必须带 `aq`**、store **必须带 `rl`**（否则 **RESERVED**）；`ld`/`sd` **仅 RV64**；注解为 **RCsc**；不提供 load-release / store-acquire |

**一句话总览**：

> **Zaamo** 给"运算型"原子操作，**Zalrsc** 给"通用型"原子操作，**Zacas** 给"比较型"原子操作，**Zabha** 把前三者变窄到字节/半字，**Zalasr** 把"有序"从"读-改-写"里剥离成独立的纯 load/store，**Zawrs** 让"等待被写"变得省电，**Zars** 和 **Zama** 则不增加任何指令——它们只是把原本"实现自由"的两件事（保留集多大、非对齐访问多小仍原子）变成软件可以依赖的架构承诺。

---

> **文档维护提示**：`za.adoc` 章首注明该章"currently being restructured"，SPEC 条款可能随上游更新而调整。本文所有规范性结论均标注了对应的 `norm:` ID，复核时应以 `SPEC/riscv-isa-manual` 子模块中的原文为准；发现平台行为与 SPEC 不符时，按项目原则**保持用例 FAIL 并记录至 `bugs/`**，不得为通过测试而降低标准。
