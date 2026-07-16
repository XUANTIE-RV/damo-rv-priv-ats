# RISC-V AIA（Advanced Interrupt Architecture）扩展说明

## 1. AIA 是什么

AIA（Advanced Interrupt Architecture，高级中断架构）是 RISC-V 国际协会于 2023 年 6 月批准的一项标准扩展，旨在对基础 RISC-V 特权架构的中断处理功能进行全面增强。

AIA 由以下三部分组成：

1. **ISA 扩展**：为 RISC-V hart 新增一组 CSR（控制状态寄存器）并修改中断响应行为，分别对应：
   - **Smaia**：Machine 级别的 ISA 扩展，涵盖 AIA 在所有特权级别新增的全部 CSR 和行为变更。
   - **Ssaia**：Supervisor 级别的 ISA 扩展，与 Smaia 基本相同，但不包含对 Supervisor 级别不可见的 Machine 级别 CSR 和行为。
2. **两个标准中断控制器**：
   - **APLIC（Advanced Platform-Level Interrupt Controller）**：高级平台级中断控制器，用于处理传统的有线中断（wired interrupts）。
   - **IMSIC（Incoming Message-Signaled Interrupt Controller）**：入站消息信号中断控制器，用于接收和处理 MSI（Message-Signaled Interrupts）。
3. **对系统其他组件的中断相关要求**：例如 IOMMU 对 MSI 的地址转换支持。

---

## 2. AIA 要解决什么问题

### 2.1 基础中断架构的局限

RISC-V 基础特权架构的中断机制相对简单，存在以下局限：

- **不支持消息信号中断（MSI）**：基础架构仅支持有线中断，无法直接对接 PCI Express 等现代总线标准所要求的 MSI 机制。
- **中断优先级能力有限**：只能由外部中断控制器（如传统 PLIC）对外部中断进行优先级排序，无法将本地中断（如定时器中断、软件中断）与外部中断的优先级统一混合排序。
- **本地中断框架薄弱**：hart 本地中断（如定时器、软件中断）种类有限，缺乏对更多异步事件（如硬件错误）的标准化支持。
- **虚拟化支持不足**：当使用 H 扩展（Hypervisor 扩展）时，虚拟机（Guest OS）无法直接控制物理设备的中断，必须依赖 Hypervisor 频繁介入进行中断注入，导致严重的性能开销。
- **中断数量受限**：基础架构中 `mip`、`mie`、`mideleg` 等 CSR 仅有有限的中断位，无法满足大型系统对大量中断源的需求。

### 2.2 AIA 的设计目标

针对上述问题，AIA 设定了以下核心目标：

1. **支持 MSI**：使 RISC-V 系统能够直接处理消息信号中断，兼容 PCI Express 等标准。
2. **增强有线中断处理**：定义新的 APLIC 替代传统 PLIC，为每个特权级别提供独立的控制接口，并可将有线中断转换为 MSI。
3. **扩展本地中断**：大幅增加 hart 本地中断的种类和数量。
4. **统一优先级排序**：允许软件配置所有中断源（包括定时器、软件、外部中断）的相对优先级。
5. **虚拟化中断支持**：当 hart 实现 H 扩展时，为虚拟机提供充足的中断虚拟化辅助，使 Guest OS 能够直接控制设备中断，最大限度减少 Hypervisor 介入。
6. **避免硬件成为虚拟化的瓶颈**：确保中断硬件不会限制可运行的虚拟机数量。

---

## 3. AIA 的核心组件与作用

### 3.1 IMSIC（Incoming MSI Controller）

IMSIC 是与每个 hart 紧密耦合的可选硬件组件，每个 hart 一个 IMSIC。

**核心功能：**
- 接收并记录来自设备或其他 hart 的消息信号中断（MSI）。
- 当有待处理且已使能的中断时，通知 hart 进行中断处理。
- 通过内存映射寄存器接收 MSI 写入，MSI 本质上是一次特定地址的内存写操作。

**中断文件（Interrupt File）：**
- 每个 IMSIC 包含多个中断文件，分别对应不同的特权级别：
  - **Machine 级中断文件**：接收 M-mode 的 MSI。
  - **Supervisor 级中断文件**：接收 S-mode 的 MSI。
  - **Guest 中断文件（可选）**：当实现 H 扩展时，用于向虚拟机（VS-mode）直接传递中断。
- 每个中断文件包含：
  - **中断挂起位数组（interrupt-pending bits）**：记录已到达但尚未处理的中断。
  - **中断使能位数组（interrupt-enable bits）**：指定 hart 当前愿意接收哪些中断。
- 每个中断文件最多支持 2047 个独立的中断标识号。

**对 IPI 的支持：**
- 当 hart 拥有 IMSIC 时，处理器间中断（IPI）可通过向目标 hart 的 IMSIC 写入 MSI 来发送，替代基础的软件中断机制。

### 3.2 APLIC（Advanced Platform-Level Interrupt Controller）

APLIC 是系统级的中断控制器，处理通过物理线路信号传输的中断。

**两种工作模式：**

| 模式 | 适用场景 | 行为 |
|------|----------|------|
| **直接传递模式** | 系统无 IMSIC | APLIC 作为传统中断控制器，通过专用线路直接将中断路由到 hart 的对应特权级别 |
| **MSI 转发模式** | 系统有 IMSIC | APLIC 将有线中断转换为 MSI 写操作，发送到目标 hart 的 IMSIC |

**关键特性：**
- 支持最多 1023 个中断源。
- 为每个特权级别提供独立的控制接口（Domain），M-mode 和 S-mode 可独立配置。
- 每个中断源可独立配置为电平触发或边沿触发。
- 软件可为每个中断源指定目标 hart 和中断标识号。

### 3.3 新增 CSR

AIA 为每个特权级别新增了大量 CSR：

**Machine 级别：**
| CSR | 功能 |
|-----|------|
| `miselect` / `mireg` | 间接寄存器访问窗口，用于访问中断优先级和 IMSIC 寄存器 |
| `mtopei` | Machine 级最高优先级外部中断（仅 IMSIC） |
| `mtopi` | Machine 级最高优先级中断 |
| `mvien` | Machine 虚拟中断使能，用于向 S-mode 注入虚拟中断 |
| `mvip` | Machine 虚拟中断挂起位 |

**Supervisor 级别：**
| CSR | 功能 |
|-----|------|
| `siselect` / `sireg` | Supervisor 级间接寄存器访问窗口 |
| `stopei` | Supervisor 级最高优先级外部中断（仅 IMSIC） |
| `stopi` | Supervisor 级最高优先级中断 |

**Hypervisor / VS 级别（需 H 扩展）：**
| CSR | 功能 |
|-----|------|
| `hvien` | Hypervisor 虚拟中断使能 |
| `hvictl` | Hypervisor 虚拟中断控制 |
| `hviprio1` / `hviprio2` | VS 级中断优先级 |
| `vsiselect` / `vsireg` | VS 级间接寄存器访问窗口 |
| `vstopei` | VS 级最高优先级外部中断 |
| `vstopi` | VS 级最高优先级中断 |

**RV32 高半部 CSR：**
对于 RV32 架构，AIA 新增了 `midelegh`、`mieh`、`mvienh`、`mviph`、`miph`、`sieh`、`siph` 等高半部寄存器，用于访问 64 位 CSR 的高 32 位。

### 3.4 中断标识体系

AIA 对中断标识进行了系统化扩展：

| 主标识号（Major Identity） | 用途 |
|---------------------------|------|
| 0-15 | 基础特权架构保留 |
| 16-23 | AIA 保留的标准本地中断 |
| 24-31 | 自定义使用 |
| 32-47 | AIA 保留的标准本地中断（含 RAS 事件中断） |
| ≥48 | 自定义使用 |

外部中断共享主标识号（M-level=11, S-level=9, VS-level=10），通过**次标识号（minor identity）**区分不同的外部中断源。

---

## 4. 为什么需要 AIA 的两个子扩展（Smaia 和 Ssaia）

AIA 定义了两个 ISA 扩展名称，原因在于：

1. **Smaia（Machine 级）**：面向 Machine 级执行环境（如固件、M-mode 软件），涵盖 AIA 定义的所有 CSR 和行为变更，包括 M-mode 独有的 `miselect`、`mireg`、`mtopei`、`mtopi`、`mvien`、`mvip` 等寄存器。

2. **Ssaia（Supervisor 级）**：面向 Supervisor 级执行环境（如操作系统），与 Smaia 基本等价，但排除了对 S-mode 不可见的 M-mode CSR 和行为。S-mode 软件能看到的是 `siselect`、`sireg`、`stopei`、`stopi` 等 Supervisor 级 CSR。

这种分离的意义在于：
- **编译器/工具链适配**：工具链可根据目标环境是 M-mode 还是 S-mode，选择性地支持对应的 CSR 访问。
- **虚拟化兼容**：在虚拟化场景中，VS-mode 的 Guest OS 看到的是 Ssaia 定义的 Supervisor 级视图，Hypervisor 负责将 Ssaia 的行为映射到物理硬件上。
- **模块化实现**：小型嵌入式系统可能只实现 Smaia（仅 M-mode），而服务器系统可同时实现 Smaia + Ssaia + H 扩展的完整 AIA 功能。

---

## 5. 何时使用 AIA

### 5.1 典型使用场景

| 场景 | 说明 |
|------|------|
| **高性能服务器/数据中心** | 需要支持大量 MSI 设备（如 PCI Express 设备），AIA 提供完整的 MSI 支持和统一的中断优先级管理 |
| **虚拟化环境** | Guest OS 需要直接控制物理设备中断（设备直通），IMSIC 的 Guest 中断文件 + IOMMU 使这成为可能，大幅减少 VM-Exit |
| **多核/众核系统** | AIA 支持最多 16,384 个物理 hart，每个 hart 可支持最多 63 个活跃虚拟 hart，适合大规模 SMP 系统 |
| **需要精细中断优先级的系统** | AIA 允许所有中断源（定时器、软件、外部中断）的优先级混合排序，而非仅外部中断可排序 |
| **现代总线互联** | 系统使用 PCI Express、CXL 等要求 MSI/MSI-X 的总线标准时，AIA 是必要的 |

### 5.2 不需要 AIA 的场景

- **简单嵌入式系统**：中断源少、无 MSI 需求的小型系统可继续使用传统 PLIC。
- **实时系统**：AIA 初始版本面向高性能系统，不支持每个中断独立 trap 入口地址、自动寄存器压栈、自动中断嵌套等实时特性（这些可能由后续的 CLIC 等扩展覆盖）。

---

## 6. AIA 与其他扩展的交集

### 6.1 与 H 扩展（Hypervisor）的交互

AIA 与 H 扩展有深度集成：
- **Guest 中断文件**：IMSIC 可为每个物理 hart 提供多个 Guest 中断文件，每个文件对应一个虚拟 hart，使 Guest OS 拥有"真实的"中断控制器体验。
- **VS 级 CSR**：AIA 新增的 `vsiselect`、`vsireg`、`vstopei`、`vstopi` 等 VS CSR 在虚拟机中替代对应的 S-mode CSR。
- **中断注入**：Hypervisor 通过 `hvien`、`hvictl`、`hviprio1/2` 等 CSR 向虚拟机注入和管理虚拟中断。
- **VGEIN 字段**：`hstatus.VGEIN` 字段指示当前激活的 Guest 中断文件编号，是 VS-mode 访问 IMSIC Guest 中断文件的关键门控。

### 6.2 与 Smcsrind/Sscsrind（间接 CSR 访问）的交互

- AIA 首先引入了 `*iselect` / `*ireg` 间接寄存器访问机制。
- Smcsrind/Sscsrind 对此机制进行了泛化，增加了 `*ireg2` 到 `*ireg6` 等额外别名寄存器。
- 当两者同时实现时，Smcsrind/Sscsrind **优先于** AIA（即 AIA 的 `*iselect`/`*ireg` 范围内，访问 `*ireg2`~`*ireg6` 会触发非法指令异常）。

### 6.3 与 Smstateen/Ssstateen（状态使能控制）的交互

当 Smstateen 与 AIA 同时实现时，`mstateen0` 和 `hstateen0` 中的三个位控制 AIA 状态的访问：
- **bit 60（CSRIND）**：控制 `siselect`、`sireg`、`vsiselect`、`vsireg` 的访问。
- **bit 59（AIA）**：控制 AIA 新增的其他所有状态（如 `stopi`、`hvictl` 等）。
- **bit 58（IMSIC）**：控制 IMSIC 相关状态（如 `stopei`、`vstopei`、Guest 中断文件）。

这些位为 0 时，低特权级别访问对应状态会触发非法指令异常（从 M-mode）或虚拟指令异常（从 VS/VU-mode）。

### 6.4 与 CLIC（Core-Local Interrupt Controller）的关系

- CLIC 面向嵌入式/实时系统，提供每个中断独立入口地址、自动寄存器压栈、自动中断嵌套等低延迟特性。
- AIA 面向高性能/服务器系统，侧重 MSI 支持、虚拟化、大规模中断管理。
- 两者定位不同，面向不同的市场领域，但在中断标识号和部分 CSR 编号空间上需要协调。

### 6.5 与 Sstc（stimecmp/vstimecmp）的交互

- Sstc 新增的 `stimecmp`/`vstimecmp` CSR 产生的定时器中断仍通过标准的中断优先级机制处理。
- AIA 的统一优先级排序可将定时器中断与外部中断进行混合优先级比较。

### 6.6 与 Smcntrpmf / Sscofpmf（计数器溢出中断）的交互

- 计数器溢出中断（Major Identity = 13）是 AIA 中断标识体系中定义的标准中断之一。
- Smcntrpmf/Sscofpmf 产生的溢出事件通过 AIA 的中断优先级框架参与排序和处理。

### 6.7 与 Smrnmi（Resumable NMI）的交互

- Smrnmi 定义的可恢复不可屏蔽中断机制与 AIA 的中断优先级体系共存。
- NMI 的优先级始终高于所有可屏蔽中断，不受 AIA 优先级配置的影响。

### 6.8 与 AIA 相关的 IOMMU 支持

- 当系统包含 IOMMU 时，IOMMU 可对设备发出的 MSI 进行地址转换，将 Guest 物理地址转换为 Host 物理地址。
- 这使得设备可以直接向虚拟机的 Guest 中断文件发送 MSI，无需 Hypervisor 介入每次中断传递。
- IOMMU 还支持 **MRIF（Memory-Resident Interrupt Files）**，允许将不活跃的虚拟 hart 的中断文件换出到内存，从而支持远超物理 Guest 中断文件数量的虚拟机。

### 6.9 与 RAS（riscv-ras-eri）的交互

- AIA 在中断标识体系中为 RAS 事件预留了两个主标识号：
  - **Major Identity 35**：低优先级 RAS 事件中断。
  - **Major Identity 43**：高优先级 RAS 事件中断。
- 这使得硬件错误事件能够通过标准化的中断路径被处理。

---

## 7. 系统架构总览

```
┌─────────────────────────────────────────────────────────────┐
│                      RISC-V 系统                             │
│                                                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                  │
│  │ Hart 0   │  │ Hart 1   │  │ Hart N   │                  │
│  │ ┌──────┐ │  │ ┌──────┐ │  │ ┌──────┐ │                  │
│  │ │IMSIC │ │  │ │IMSIC │ │  │ │IMSIC │ │                  │
│  │ │(M/S/ │ │  │ │(M/S/ │ │  │ │(M/S/ │ │                  │
│  │ │Guest)│ │  │ │Guest)│ │  │ │Guest)│ │                  │
│  │ └──┬───┘ │  │ └──┬───┘ │  │ └──┬───┘ │                  │
│  └────┼─────┘  └────┼─────┘  └────┼─────┘                  │
│       │              │              │                        │
│       │    MSI Write（内存写）       │                        │
│       │              │              │                        │
│  ┌────┴──────────────┴──────────────┴────┐                  │
│  │          系统互连 / 总线               │                  │
│  └────┬──────────────┬──────────────┬────┘                  │
│       │              │              │                        │
│  ┌────┴────┐   ┌─────┴─────┐  ┌────┴────┐                  │
│  │  APLIC  │   │   IOMMU   │  │PCIe Dev │                  │
│  │(有线→MSI│   │(MSI地址转换)│  │(MSI源)  │                  │
│  │  转换)  │   │           │  │         │                  │
│  └────┬────┘   └───────────┘  └─────────┘                  │
│       │                                                     │
│  ┌────┴────┐                                                │
│  │ 有线中断 │                                                │
│  │  设备   │                                                │
│  └─────────┘                                                │
└─────────────────────────────────────────────────────────────┘
```

---

## 8. 关键数据汇总

| 指标 | 最大值 |
|------|--------|
| 物理 hart 数量 | 16,384 |
| 每个物理 hart 的活跃虚拟 hart（有设备直通） | RV32: 31, RV64: 63 |
| 每个物理 hart 的空闲（换出）虚拟 hart | 可达数千个（需 IOMMU + MRIF） |
| 单个 APLIC 的有线中断源 | 1,023 |
| 每个中断文件的 MSI 标识数 | 最多 2,047 |

---

## 9. 参考文档

- **AIA 规范正文**：`SPEC/riscv-aia/src/riscv-interrupts.adoc`
- **IMSIC 规范**：`SPEC/riscv-aia/src/IMSIC.adoc`
- **APLIC 规范**：`SPEC/riscv-aia/src/AdvPLIC.adoc`
- **VS 级中断**：`SPEC/riscv-aia/src/VSLevel.adoc`
- **IPI 规范**：`SPEC/riscv-aia/src/IPIs.adoc`
- **IOMMU 支持**：`SPEC/riscv-aia/src/IOMMU.adoc`
- **H 扩展（Hypervisor）**：`SPEC/hypervisor.adoc`
- **Smcsrind/Sscsrind**：`SPEC/smcsrind.adoc`
- **Smstateen/Ssstateen**：`SPEC/smstateen.adoc`
