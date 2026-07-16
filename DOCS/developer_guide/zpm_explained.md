# RISC-V Zpm（Pointer Masking）扩展详解

## 1. 背景：Zpm 要解决什么问题？

### 1.1 内存安全检测的核心挑战

在现代软件开发中，**内存安全错误**（如 use-after-free、buffer overflow、out-of-bounds 访问）是最常见且最危险的漏洞类型之一。为了在运行时检测这些错误，业界开发了多种动态检测工具：

- **AddressSanitizer (ASan)**：通过在每次内存访问前后插入检查代码来检测内存错误，但性能开销巨大（通常 2-3 倍减速）。
- **HWASAN（Hardware-assisted AddressSanitizer）**：利用地址的高位存储"指针标签"（pointer tag），与存储在旁路表（side table）中的"内存标签"进行比对，从而在硬件层面高效检测内存错误。

### 1.2 核心问题：高位 Tag 带来的地址处理开销

HWASAN 的工作原理是将一个随机标签（tag）存储在指针的高位（即地址中未用于寻址的 NVBITS 位）。在每次内存访问时，需要：
1. 从指针中提取 tag（读取高位）
2. 将高位清零或符号扩展，还原出真实地址
3. 用真实地址执行内存访问
4. 将指针 tag 与目标内存的 tag 进行比对

**问题在于**：步骤 2（还原真实地址）在每次内存访问时都必须执行，这在软件中实现会带来巨大的性能开销。

### 1.3 Zpm 的解决方案：硬件级指针掩码

RISC-V **Pointer Masking (PM)** 扩展通过**硬件自动忽略地址的高位**来解决这个问题。当 PM 启用时，CPU 在执行显式内存访问（load/store/atomic）时，会自动对地址执行"忽略变换"（ignore transformation），将高位 tag 屏蔽掉，无需软件干预。

这样：
- **指针 tag 仍然存储在地址的高位**，可供软件（如 HWASAN）读取并用于校验
- **CPU 自动忽略这些 tag 位**，直接用真实的低位地址执行内存访问
- **性能大幅提升**：省去了每次内存访问前的地址清理开销

---

## 2. Zpm 的五个扩展

Zpm 实际上是一个**扩展家族**，包含 5 个独立的扩展，分为两类：

### 2.1 硬件实现扩展（3个）

| 扩展名 | 全称 | 层级 | 作用 |
|--------|------|------|------|
| **Ssnpm** | Supervisor-level pointer masking for Next-lower Privilege mode | Supervisor 级 | 为**下一个低特权级**（U-mode，或 H 扩展下的 VS/VU-mode）提供指针掩码 |
| **Smnpm** | Machine-level pointer masking for Next-lower Privilege mode | Machine 级 | 为**下一个低特权级**（S/HS-mode，若无 S-mode 则为 U-mode）提供指针掩码 |
| **Smmpm** | Machine-level pointer masking for M-mode | Machine 级 | 为 **M-mode 自身**提供指针掩码 |

### 2.2 执行环境描述扩展（2个，不涉及硬件实现）

| 扩展名 | 全称 | 层级 | 作用 |
|--------|------|------|------|
| **Sspm** | Supervisor-mode Pointer Masking | Supervisor 级 | 表明 S-mode 下存在指针掩码支持，由**监督执行环境（SEE）**提供控制设施 |
| **Supm** | User-mode Pointer Masking | User 级 | 表明 U-mode 下存在指针掩码支持，由**应用执行环境（AEE）**提供控制设施 |

> Sspm 和 Supm 是**执行环境层面的声明**，用于 Profile 规范中描述环境能力，不直接对应硬件实现。例如：RVA23 Profile 中要求实现 Sspm。

---

## 3. 五个扩展的详细说明

### 3.1 Ssnpm（Supervisor → 下一级）

**目的**：让 S-mode 操作系统能够为 U-mode 进程（或虚拟化场景下的 VS/VU-mode）启用指针掩码。

**控制机制**：
- 通过 `senvcfg[PMM]` 字段控制 U-mode（以及 VU-mode）的指针掩码
- 通过 `henvcfg[PMM]` 字段控制 VS-mode 的指针掩码（若 H 扩展存在）

**PMM 字段编码**：

| PMM 值 | 含义 |
|--------|------|
| `00` | 禁用指针掩码（PMLEN=0） |
| `10` | PMLEN=XLEN-57（RV64 下为 7） |
| `11` | PMLEN=XLEN-48（RV64 下为 16） |

**典型使用场景**：
- Linux 内核为某个用户进程启用 HWASAN，设置 `senvcfg.PMM=11`（PMLEN=16）
- 该进程的指针高位 16 位可以存储 tag，CPU 自动忽略这些位

### 3.2 Smnpm（Machine → 下一级）

**目的**：让 M-mode 固件能够为 S-mode（或 U-mode，若无 S-mode）启用指针掩码。

**控制机制**：
- 通过 `menvcfg[PMM]` 字段控制 S/HS-mode 的指针掩码（若 S-mode 已实现）
- 若无 S-mode，则控制 U-mode 的指针掩码

**典型使用场景**：
- M-mode 固件为 S-mode 操作系统启用指针掩码，使内核态代码也能利用 tag 进行内存安全检查
- 在仅 M+U 双特权级的嵌入式系统中，为 U-mode 启用指针掩码

### 3.3 Smmpm（Machine → M-mode 自身）

**目的**：为 M-mode 自身提供指针掩码支持。

**控制机制**：
- 通过 `mseccfg[PMM]` 字段控制 M-mode 自身的指针掩码

**典型使用场景**：
- M-mode 固件本身需要使用带 tag 的指针（如 M-mode 下的安全监控代码使用 HWASAN）
- 嵌入式系统中 M-mode 运行复杂代码时的内存安全检测

### 3.4 Sspm（Supervisor 执行环境声明）

**目的**：在 Profile 规范中声明 S-mode 具有指针掩码能力。

**本质**：这不是一个硬件扩展，而是一个**环境能力声明**。它表明：
- 该执行环境中 S-mode 可以使用指针掩码
- 监督执行环境（SEE，如 hypervisor 或固件）提供了控制指针掩码的接口
- S-mode 代码可以通过 SEE 提供的机制来启用/禁用自身的指针掩码

**典型使用场景**：
- RVA23/RVB23 Profile 要求实现 Sspm，确保符合该 Profile 的平台都支持 S-mode 指针掩码

### 3.5 Supm（User 执行环境声明）

**目的**：在 Profile 规范中声明 U-mode 具有指针掩码能力。

**本质**：这也是一个**环境能力声明**。它表明：
- 该执行环境中 U-mode 可以使用指针掩码
- 应用执行环境（AEE，如操作系统）提供了控制指针掩码的接口
- 用户态程序可以通过系统调用（如 `prctl`）请求操作系统启用指针掩码

**典型使用场景**：
- 应用程序向操作系统请求启用 HWASAN 支持时，操作系统通过 Supm 声明的能力来配置 `senvcfg.PMM`

---

## 4. "Ignore" 变换：Zpm 的核心机制

### 4.1 虚拟地址的变换

当使用虚拟地址（`satp.MODE != Bare`）时，ignore 变换将高位 PMLEN 位替换为**第 PMLEN+1 位的符号扩展**：

```
transformed_address = {PMLEN{address[XLEN-PMLEN-1]}}, address[XLEN-PMLEN-1:0]}
```

**示例（RV64 + Sv57, PMLEN=7）**：
- 原始地址：`0xABFFFFFF12345678`（高位 tag = `0xAB`）
- 第 56 位（PMLEN+1st bit）= 1
- 变换后地址：`0xFFFFFFFF12345678`（高位被符号扩展填充）

### 4.2 物理地址的变换

当使用物理地址（`satp.MODE == Bare` 或 M-mode）时，ignore 变换将高位 PMLEN 位**清零**：

```
transformed_address = {PMLEN{0}}, address[XLEN-PMLEN-1:0]}
```

这与 RISC-V 已有的物理地址约定一致（物理地址高位为 0）。

### 4.3 PMLEN 的合法值

当前标准仅支持两种 PMLEN：
- **PMLEN=XLEN-48**（RV64 下为 16）：适用于 Sv39、Sv48
- **PMLEN=XLEN-57**（RV64 下为 7）：适用于 Sv57

---

## 5. 何时会用到 Zpm？

### 5.1 主要使用场景：HWASAN（硬件辅助地址消毒器）

这是 Zpm 的**首要设计目标**：

1. 编译器在每个内存分配时，为其关联一个随机 tag，存储在指针的高位
2. 同时在一个旁路表（shadow memory）中记录该内存区域的 tag
3. 每次内存访问时：
   - CPU 自动忽略指针高位（由 Zpm 硬件完成）
   - 软件读取指针 tag 并与旁路表中的 tag 比对
   - 若不匹配，说明存在内存错误（use-after-free、buffer overflow 等）

### 5.2 其他潜在场景

- **沙箱隔离（Sandboxing）**：利用高位 tag 标识地址所属的沙箱域
- **对象类型检查（Object Type Checks）**：tag 编码对象的类型信息
- **垃圾回收（GC）标记**：运行时系统在指针高位存储 GC 相关标记位

### 5.3 典型使用流程

```
[用户程序] --prctl(PR_SET_TAGGED_ADDR_CTRL)--> [Linux内核]
    |                                              |
    |                                        设置 senvcfg.PMM
    |                                              |
    v                                              v
[带 tag 指针的内存访问] ---硬件自动掩码---> [真实地址内存访问]
```

---

## 6. 为什么需要 5 个独立的扩展？

### 6.1 特权级分层控制

RISC-V 的多特权级模型要求**每个特权级的指针掩码独立控制**，由上一级为下一级配置：

```
M-mode (Smmpm: 控制自身, Smnpm: 控制 S-mode)
  └── S/HS-mode (Ssnpm: 控制 U-mode)
       └── U-mode (Supm: 声明能力)
```

这确保了：
- 高特权级可以**独立决定**是否为低特权级启用指针掩码
- 不同进程/虚拟机可以有**不同的 PMLEN 配置**
- M-mode 可以保护自身不受 S-mode 指针掩码设置的影响

### 6.2 硬件实现与执行环境分离

- **Ssnpm/Smnpm/Smmpm** 是硬件扩展，定义了 CSR 中的控制位
- **Sspm/Supm** 是环境声明，用于 Profile 规范
- 这种分离使得：
  - 硬件实现者可以选择实现哪些扩展
  - Profile 规范可以独立声明环境能力要求
  - 虚拟化环境下 hypervisor 可以精细控制每个 guest 的指针掩码

---

## 7. Zpm 与其他扩展的交集

### 7.1 与虚拟内存（Sv39/Sv48/Sv57）

- PMLEN 的值与地址翻译模式紧密相关
- Sv57 下 PMLEN=7（掩码 7 位，剩余 57 位用于寻址）
- Sv48/Sv39 下 PMLEN=16（掩码 16 位，剩余 48 位用于寻址）
- 启用/禁用指针掩码**不需要**执行 `SFENCE.VMA`，因为 PM 只影响有效地址，不影响页表结构

### 7.2 与 Hypervisor 扩展（H-extension）

- Ssnpm 通过 `henvcfg[PMM]` 为 VS-mode 提供独立的指针掩码控制
- `hstatus` 中也包含相关字段影响 VU-mode 的指针掩码行为
- Hypervisor 可以为每个虚拟机配置不同的 PMLEN
- Guest-physical address 使用物理地址的 ignore 变换（高位清零）

### 7.3 与 PMP（Physical Memory Protection）

- 物理地址的 ignore 变换（高位清零）与 PMP 地址匹配一致
- PMP 检查基于变换后的地址进行

### 7.4 与 MPRV/SPVP（mstatus 字段）

- `mstatus.MPRV`（Modify PRiVilege）和 `mstatus.SPVP` 会影响指针掩码
- 当 MPRV=1 时，使用有效特权级的指针掩码设置，而非 M-mode 的设置

### 7.5 与 MXR（Make eXecutable Readable）

- 当 MXR 生效时（允许从可执行页读取数据），**指针掩码不应用**
- 这是一个特殊的安全设计：MXR 绕过了页表的正常权限检查，因此也不应绕过掩码

### 7.6 与 CMO（Cache Management Operations，Zicbom/Zicbop/Zicboz）

- **CMO 指令必须遵守指针掩码**
- 否则可能导致：
  - `CBO.ZERO` 绕过掩码写入任意内存
  - 侧信道攻击：U-mode 通过 `CBO.FLUSH` 刷出不该访问的物理地址

### 7.7 与 Svinval（细粒度地址翻译失效）

- `SFENCE.*`、`HFENCE.*`、`SINVAL.*`、`HINVAL.*` 指令**不应用**指针掩码
- 软件在执行这些指令时需要提供正确的（未掩码的）地址

### 7.8 与 CFI（Zicfiss/Zicfilp）

- Shadow stack 指令（`sspush`、`sspopchk`、`ssamoswap`）作为显式内存访问，**受指针掩码影响**
- 这意味着如果启用了 PM 和 CFI，shadow stack 的地址也需要正确处理 tag

### 7.9 与 Debug（调试扩展）

- 调试地址触发器（address trigger）在匹配地址时**应用指针掩码**
- 确保调试工具在有/无 PM 的环境下行为一致

### 7.10 与 CSR 读写

- 软件读写 CSR 时**不应用**指针掩码（包括地址类 CSR 如 `stvec`、`stval` 等）
- 但**硬件写入 CSR 时应用**指针掩码（如 trap 时硬件将变换后的地址写入 `stval`）
- CSR 的 WARL 位宽不受 PM 影响

### 7.11 RV32 限制

- 指针掩码**仅适用于 RV64**
- 在 RV32 下，PMM 字段为只读零，尝试启用 PM 会按 WARL 语义失败
- 当 `UXL/SXL/MXL` 被设为 1（即该特权级使用 RV32）时，对应 PMM 位被清零

---

## 8. 总结对比表

| 扩展 | 类型 | 控制者 | 被控制者 | 控制 CSR | 适用场景 |
|------|------|--------|----------|----------|----------|
| Ssnpm | 硬件 | S-mode | U/VU-mode（`senvcfg.PMM`）<br>VS-mode（`henvcfg.PMM`） | `senvcfg`、`henvcfg` | 操作系统为用户进程启用 HWASAN |
| Smnpm | 硬件 | M-mode | S/HS-mode（`menvcfg.PMM`）<br>或 U-mode | `menvcfg` | 固件为操作系统启用 PM |
| Smmpm | 硬件 | M-mode | M-mode（`mseccfg.PMM`） | `mseccfg` | M-mode 固件自身的内存安全检查 |
| Sspm | 环境声明 | SEE | S-mode | N/A（由环境提供） | Profile 规范声明 S-mode PM 能力 |
| Supm | 环境声明 | AEE | U-mode | N/A（由环境提供） | Profile 规范声明 U-mode PM 能力 |

五个扩展协同工作，为 RISC-V 多级特权架构提供完整的指针掩码支持，使 HWASAN 等动态内存安全检测工具能够以最小的性能开销在所有特权级下运行。
