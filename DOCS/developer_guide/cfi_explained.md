# RISC-V CFI（Control-Flow Integrity）扩展详解

## 1. 背景：CFI 要解决什么问题？

在现代软件安全领域，**控制流劫持攻击**是最常见且最危险的攻击手段之一。攻击者通过利用缓冲区溢出、格式化字符串漏洞等内存安全问题，篡改程序的控制流（即程序执行的指令顺序），从而执行恶意代码或窃取敏感信息。

主要的控制流攻击类型有两类：

1. **ROP（Return-Oriented Programming，面向返回的编程）**：攻击者篡改栈上的返回地址，使函数返回时跳转到攻击者预设的"gadget"代码片段，再通过这些 gadget 链完成恶意操作。这类攻击针对的是**后向边（backward-edge）**控制流——即函数返回路径。

2. **COP/JOP（Call/Jump-Oriented Programming，面向调用/跳转的编程）**：攻击者篡改函数指针、虚函数表、跳转表等，使间接调用（`jalr`）或间接跳转指令跳转到攻击者指定的代码位置。这类攻击针对的是**前向边（forward-edge）**控制流——即间接调用/跳转路径。

软件层面的防御手段（如 stack canary、ASLR、DEP/NX）在一定程度上可以增大攻击难度，但本质上仍无法彻底阻止控制流被篡改。因此，业界需要**硬件级别的控制流完整性（CFI）**保护机制，从根本上验证控制流转移的合法性。

---

## 2. CFI 的两个扩展：Zicfilp 与 Zicfiss

RISC-V 通过两个独立但互补的扩展来实现完整的 CFI 保护：

| 扩展名 | 全称 | 保护方向 | 核心机制 |
|--------|------|----------|----------|
| **Zicfilp** | Control-Flow Integrity Landing Pad | 前向边（forward-edge） | Landing Pad（着陆垫） |
| **Zicfiss** | Control-Flow Integrity Shadow Stack | 后向边（backward-edge） | Shadow Stack（影子栈） |

两者分别保护控制流的两个维度，共同构建完整的控制流完整性防线。

---

## 3. Zicfilp（Landing Pad）：前向边保护

### 3.1 设计目的

Zicfilp 的目的是**验证间接调用和间接跳转的目标是否合法**。在正常的程序中，间接调用（如通过函数指针调用）应该跳转到已知的、合法的函数入口。攻击者如果篡改了函数指针，目标很可能不是一个合法的函数入口。

### 3.2 核心机制

Zicfilp 引入了一条新的指令：**`lpad`（Landing Pad，着陆垫）**。

- 编译器在每个合法函数（或可被间接跳转到的目标）的入口处插入一条 `lpad` 指令作为"着陆标记"。
- 当 CPU 执行一条间接调用/跳转指令（`jalr`、`c.jalr`、`c.jr`）时，硬件进入 `ELP=LP_EXPECTED`（Expected Landing Pad）状态，表示"期望下一条执行的指令是 `lpad`"。
- 如果目标地址处的第一条指令确实是 `lpad`，则 `ELP` 被重置为 `NO_LP_EXPECTED`，正常继续执行。
- 如果目标地址处的第一条指令**不是** `lpad`，则触发**software-check exception**（`x tval` 设为 "landing pad fault, code=2"），即前向边控制流违规。

### 3.3 使能控制（LPE）

Zicfilp 需要在每个特权级上独立启用，通过以下 CSR 字段控制：

| 特权级 | 控制字段 |
|--------|----------|
| M-mode | `mseccfg[MLPE]` |
| S/HS-mode | `menvcfg[LPE]` |
| VS-mode | `henvcfg[LPE]` |
| U/VU-mode | `senvcfg[LPE]` |

未启用时，`lpad` 指令会被当作 NOP（空操作），程序正常运行但没有前向边保护。

### 3.4 Trap 时的状态保存

当发生 trap（中断或异常）时，当前的 `ELP` 状态需要保存：
- Trap 进入 M-mode 时，`ELP` 保存到 `mstatus[MPELP]`
- Trap 进入 S/HS-mode 时，`ELP` 保存到 `mstatus[SPELP]`
- Trap 进入 VS-mode 时，`ELP` 保存到 `vsstatus[SPELP]`
- 进入 Debug Mode 时，`ELP` 保存到 `dcsr[PELP]`
- Trap 返回时（`mret`/`sret`），通过 `xret` 指令恢复目标特权级的 `ELP` 状态

---

## 4. Zicfiss（Shadow Stack）：后向边保护

### 4.1 设计目的

Zicfiss 的目的是**保护函数返回地址不被篡改**。在传统攻击中，攻击者通过栈溢出覆盖函数的返回地址，使 `ret` 指令跳转到恶意代码。Shadow stack 通过在独立的、受硬件保护的栈空间中维护一份返回地址的"影子副本"，在函数返回时与常规栈上的返回地址进行比对，从而检测返回地址是否被篡改。

### 4.2 核心机制

Zicfiss 引入了一个新的 CSR（`ssp`，Shadow Stack Pointer）和一组专用指令：

- **`sspush`** / **`c.sspush`**：将当前链接寄存器（link register，`x1` 或 `x5`）的值压入影子栈（`ssp` 指向的位置），并更新 `ssp`。
- **`sspopchk`** / **`c.sspopchk`**：从影子栈弹出一个值，与链接寄存器进行比较。如果不匹配，说明返回地址被篡改，触发异常。
- **`ssamoswap.w`** / **`ssamoswap.d`**：原子交换操作，用于操作系统在特权级切换时安全地切换影子栈指针。

典型的工作流程：
1. **函数调用前**：调用者执行 `sspush`，将返回地址压入影子栈。
2. **函数返回前**：被调用者执行 `sspopchk`，从影子栈弹出值并与链接寄存器比对。
3. 如果影子栈中的值与链接寄存器不匹配，触发异常，阻止恶意跳转。

### 4.3 使能控制（SSE）

Zicfiss 也需要在每个特权级上独立启用：

| 特权级 | 控制字段 |
|--------|----------|
| M-mode | **不支持**（当前规范中 `xSSE` 固定为 0） |
| S/HS-mode | `menvcfg[SSE]` |
| VS-mode | `henvcfg[SSE]` |
| U/VU-mode | `senvcfg[SSE]` |

注意：当 S-mode 未实现时，M-mode 和 U-mode 均不支持 Zicfiss。

### 4.4 Shadow Stack 页（SS Page）—— 内存保护

Zicfiss 在页表（PTE）中定义了一种新的页类型：**Shadow Stack 页**，编码为 `R=0, W=1, X=0`（即 `xwr=010b`）。

这种页的访问规则极为严格：

| 访问类型 | 是否允许 |
|----------|----------|
| 通过 `sspush`/`sspopchk`/`ssamoswap` 等指令访问 | **允许** |
| 通过普通 store 指令写入 | **拒绝**（触发 store/AMO access-fault，致命错误） |
| 通过普通 load 指令读取 | **允许**（便于调试和性能分析） |
| 隐式访问（如取指令） | **拒绝**（触发 access-fault） |
| 通过 `cbo.*` 缓存操作指令访问 | **拒绝**（触发 access-fault） |

**关键设计决策**：非影子栈指令写入 SS 页会触发 **access-fault**（而非 page-fault）。这是一个故意的安全设计——access-fault 表示不可恢复的致命错误，而 page-fault 暗示操作系统可以尝试修复，从而避免攻击者利用 page-fault handler 绕过保护。

### 4.5 ssp CSR 的访问控制

对 `ssp` CSR 的访问受 `envcfg` 中的 `SSE` 字段保护，不同特权级有不同的控制逻辑：
- 低于 M-mode 且 `menvcfg[SSE]=0`：触发 illegal-instruction exception
- U-mode 且 `senvcfg[SSE]=0`：触发 illegal-instruction exception
- VS-mode 且 `henvcfg[SSE]=0`：触发 virtual-instruction exception
- VU-mode 且 `henvcfg[SSE]=0` 或 `senvcfg[SSE]=0`：触发 virtual-instruction exception

---

## 5. 何时会用到这两个扩展？

### 5.1 Zicfilp（Landing Pad）的使用场景

- **防御 COP/JOP 攻击**：任何涉及函数指针、虚函数调用、间接跳转的场景。
- **操作系统内核保护**：内核中的间接调用（如系统调用表、驱动回调函数）是高价值攻击目标。
- **高安全等级系统**：金融、军事、关键基础设施等对控制流完整性有严格要求的场景。
- **编译器配合**：需要编译器（如 LLVM/GCC）开启 CFI 支持，在函数入口自动插入 `lpad` 指令。

### 5.2 Zicfiss（Shadow Stack）的使用场景

- **防御 ROP 攻击**：这是 shadow stack 最主要的应用，防止返回地址被篡改。
- **用户态程序保护**：操作系统的用户态进程启用 shadow stack 后，即使存在栈溢出漏洞，攻击者也无法劫持返回地址。
- **与 CFI 编译器配合**：编译器在函数序言（prologue）中插入 `sspush`，在函数尾声（epilogue）中插入 `sspopchk`。
- **上下文切换**：操作系统在进程/线程切换时使用 `ssamoswap` 安全地切换影子栈指针。

### 5.3 两者结合使用

当同时启用 Zicfilp 和 Zicfiss 时，可以实现**完整的控制流保护**：
- Zicfilp 保护所有间接调用/跳转的目标合法性（前向边）
- Zicfiss 保护所有函数返回地址不被篡改（后向边）

---

## 6. 为什么需要这两个扩展（而非纯软件方案）？

| 维度 | 纯软件 CFI | 硬件 CFI（Zicfilp + Zicfiss） |
|------|------------|-------------------------------|
| **性能开销** | 高（需要插入大量检查代码，运行时开销 10-50%） | 低（检查由硬件自动完成） |
| **安全性** | 依赖软件正确性，可能被绕过 | 硬件强制，攻击者无法绕过 |
| **兼容性** | 需要重新编译所有代码 | `lpad` 在未启用时是 NOP；`zicfiss` 指令退化为 `zimop`/`zcmop` |
| **影子栈保护** | 需软件模拟，开销极大且可能被攻击 | 硬件级 SS 页保护，非授权写入直接触发 access-fault |
| **粒度控制** | 粗粒度（通常全局启用或禁用） | 可按特权级独立启用/禁用 |

---

## 7. 与其他扩展的交集

### 7.1 与 Hypervisor 扩展（H-extension）

- Zicfilp 在 VS-mode 和 VU-mode 下通过 `henvcfg[LPE]` 控制启用
- Zicfiss 在 VS-mode 和 VU-mode 下通过 `henvcfg[SSE]` 控制启用
- Hypervisor 可以为每个虚拟机（guest）独立配置 CFI 策略
- Shadow stack 指令在 VS-stage 翻译中，如果 `henvcfg[SSE]=0`，SS 页编码仍被视为保留
- G-stage 页表中 `xwr=010b` 编码目前保留，未来扩展可能定义 G-stage shadow stack 支持
- G-stage 要求对 shadow stack 访问提供读写权限，否则触发 guest-page-fault

### 7.2 与 Svinval（细粒度地址翻译失效）

- `xSSE` 的变更立即生效，**不需要**执行 `sfence.vma`、`hfence.gvma` 或 `hfence.vvma` 指令来同步地址翻译缓存

### 7.3 与 Svpbmt（基于页的内存类型）

- Shadow stack 页支持 Svpbmt 扩展定义的内存类型属性

### 7.4 与 Svnapot（自然对齐 2 的幂次页）

- Shadow stack 页支持 Svnapot 扩展定义的大页（NAPOT）映射

### 7.5 与 PMP（Physical Memory Protection）

- Shadow stack 指令访问的内存必须在 PMP 中具有读写权限
- 如果 PMP 不提供读写权限或内存不是幂等的（idempotent），触发 store/AMO access-fault

### 7.6 与 Debug（调试扩展）

- 进入 Debug Mode 时，`ELP` 保存到 `dcsr[PELP]`
- 退出 Debug Mode 时根据目标特权级的 `xLPE` 恢复 `ELP`

### 7.7 与 Smrnmi（Resumable NMI）

- Zicfilp 为 RNMI trap 和 `mnret` 指令增加了额外的语义（保存/恢复 `ELP` 状态）

### 7.8 与 Zimop/Zcmop（May-Be-Operations）

- Zicfiss 的指令编码复用了 `Zimop` 和 `Zcmop` 的代码空间
- 当 Zicfiss 未实现或未启用时，这些指令退化为 `Zimop`/`Zcmop` 定义的行为（通常为 NOP）
- 这保证了向后兼容性：带有 Zicfiss 指令的程序在不支持 Zicfiss 的处理器上仍可运行（只是没有保护）

### 7.9 与 Zicboz/Zicbom/Zicbop（Cache-Block Operations）

- Shadow stack 页**不允许**通过 `cbo.*` 指令访问，触发 store/AMO access-fault

### 7.10 与 AIA（Advanced Interrupt Architecture）

- 在虚拟化环境下，AIA 的中断机制与 CFI 的 trap 处理机制需要协同工作

---

## 8. 兼容性设计

RISC-V CFI 扩展在兼容性上做了精心设计：

1. **向前兼容（Forward Compatibility）**：
   - 未启用 Zicfilp 时，`lpad` 指令对程序无影响（NOP 语义）
   - 未启用 Zicfiss 时，shadow stack 指令退化为 `zimop`/`zcmop` 行为

2. **按特权级独立控制**：
   - 可以为每个特权级独立启用或禁用 CFI
   - 允许操作系统为不同进程配置不同的保护策略

3. **共享库兼容**：
   - 未启用 Zicfiss 的进程可以调用启用了 Zicfiss 的共享库
   - 共享库中的 shadow stack 指令在未启用时自动退化为 NOP

---

## 9. 总结

| 特性 | Zicfilp（Landing Pad） | Zicfiss（Shadow Stack） |
|------|------------------------|-------------------------|
| 保护方向 | 前向边（间接调用/跳转） | 后向边（函数返回） |
| 防御攻击 | COP/JOP | ROP |
| 核心机制 | 目标地址处验证 `lpad` 指令 | 影子栈维护返回地址副本 |
| 关键 CSR | `mstatus[MPELP/SPELP]`、`vsstatus[SPELP]`、`dcsr[PELP]` | `ssp`（Shadow Stack Pointer） |
| 页表新类型 | 无 | SS 页（`xwr=010b`） |
| 新增指令 | `lpad` | `sspush`、`sspopchk`、`ssamoswap` |
| M-mode 支持 | 是（`mseccfg[MLPE]`） | 否（当前规范不支持） |
| 虚拟化支持 | 通过 `henvcfg[LPE]` 控制 | 通过 `henvcfg[SSE]` 控制 |
| 向后兼容 | 未启用时 `lpad` 为 NOP | 未启用时退化为 `zimop`/`zcmop` |

两个扩展协同工作，为 RISC-V 处理器提供从硬件层面的完整控制流完整性保护，从根本上抵御 ROP/COP/JOP 类攻击。
