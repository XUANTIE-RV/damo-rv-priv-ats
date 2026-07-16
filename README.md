**中文 | [English](README_en.md)**

# Damo RV Priv ATS

Damo RV Priv ATS 是一个用于验证 RISC-V 特权态扩展的的兼容性测试框架。可以直接运行在硬件DUT或模拟器（QEMU / Sail / Spike）上运行，基于Baremetal程序，不依赖任何操作系统。该框架覆盖了 RISC-V 特权态规范中定义的 70+ 个特权扩展，每个扩展编译为完全独立的 ELF 二进制文件。通过平台抽象使得——同一套测试代码只需切换编译配置即可适配 QEMU、Sail、Spike 或 FPGA/硬件目标，共享框架与扩展逻辑之间零耦合。

---

## 项目目录结构

```
damo-priv-test/
├── Makefile               # 顶层：构建所有扩展
├── README.md / README_zh.md
│
├── common/                # 共享基础设施（与扩展无关）
│   ├── pmp/               # PMP 公共库
│   ├── vm/                # 虚拟内存公共库
│   ├── hyp/               # Hypervisor 扩展框架
│   ├── pm/                # Pointer Masking 框架
│   ├── config/            # 平台配置（qemu, sail, spike, haps, ...）
│   └── ...                # entry.S, trap.c, privilege.c, test_framework.h/c, uart.c 等
│
├── DOCS/                  # 项目文档
│   ├── framework/         # 框架设计文档（架构、API、用法）
│   ├── testplan/          # 合规性测试方案（每个扩展一份）
│   ├── develop_guide/     # 开发者指南（编写测试、API 参考、添加扩展）
│   ├── framework_en/      # 英文版框架文档
│   └── testplan_en/       # 英文版测试方案
│
├── SPEC/                  # RISC-V 规范摘录（.adoc）及完整规范仓库
├── NORM/                  # 结构化规范条目（Norm ID → 规范原文）
│
├── Sha/                   # ── 扩展测试目录 ──
├── Sstc/                  #    （每个目录编译为独立的裸机二进制）
├── Sv39/                  #
├── Hypervisor/            #
├── aia_aplic/             #
└── ...                    #    （共 70+ 个扩展目录）
```

每个扩展目录的标准结构：

```
<extension>/
├── Makefile          # 包含 ../common/Makefile.common
├── kernel.ld         # 链接脚本
├── main.c            # 扩展测试入口
└── tests/
    ├── test_register.c   # 测试用例注册
    └── test_xxx.c        # 具体测试用例
```

---

## 前置要求

- **RISC-V 交叉编译器**：`riscv64-unknown-elf-gcc`（RV32 使用 `riscv32-unknown-elf-gcc`）
- **QEMU**（可选）：`qemu-system-riscv64` / `qemu-system-riscv32`
- **Sail**（可选）：`sail_riscv_sim`，用于参考模型验证
- **Spike**（可选）：`spike`，用于 ISA 模拟

---

## 快速开始

```bash
# 构建单个扩展（默认：QEMU RV64）
make pmp CROSS_COMPILER=/path/to/riscv64-unknown-elf-

# 切换到其他平台构建
make pmp CONFIG=sail-rv64-max CROSS_COMPILER=/path/to/riscv64-unknown-elf-

# 构建所有扩展
make all CROSS_COMPILER=/path/to/riscv64-unknown-elf-

# 在模拟器上运行
make qemu-pmp CROSS_COMPILER=/path/to/riscv64-unknown-elf-
make sail-pmp CROSS_COMPILER=/path/to/riscv64-unknown-elf-
make spike-pmp CROSS_COMPILER=/path/to/riscv64-unknown-elf-

# 构建 RV32 版本
make XLEN=32 CROSS_COMPILER=/path/to/riscv32-unknown-elf-
```

使用 `TEST_FILTER` 运行特定测试：

```bash
make qemu-pmp EXTRA_CFLAGS='-DTEST_FILTER="PMP"' CROSS_COMPILER=/path/to/riscv64-unknown-elf-
```

---

## DUT 适配

框架通过 `CONFIG` 变量支持多种 DUT（被测设备）目标。每种配置定义了平台特定的内存布局、UART 设置和构建选项。

### 可用配置

| CONFIG | 目标 | XLEN | MEM_BASE | 说明 |
|--------|------|------|----------|------|
| `qemu-rv64-max` | QEMU virt | 64 | 0x80000000 | 默认 QEMU 平台 |
| `qemu-rv32-max` | QEMU virt | 32 | 0x80000000 | QEMU RV32 变体 |
| `sail-rv64-max` | Sail | 64 | 0x80000000 | 参考模型 |
| `spike-rv64-max` | Spike | 64 | 0x80000000 | ISA 模拟器 |


### 工作原理

每个配置位于 `common/config/<CONFIG>/`，包含：
- **platform.mk** — 构建设置（交叉编译器、内存基址、模拟器选项）
- **platform_config.h** — 平台层配置（UART 基地址、AIA BASE 地址、TRACE base 地址等硬件定义）
- **rvtest_config.h** — Core 支持的扩展定义及 ISA parameters（由 riscv-unified-db 自动生成）
- **rvmodel_macros.h** — 模型参数

平台头文件和扩展配置在编译时通过 GCC `-include` 注入，因此测试源代码无需直接包含平台特定的头文件。

### 在 HAPS 硬件上运行

```bash
cd <test_folder>
make clean
make CONFIG=haps_xiaohui CROSS_COMPILER=/path/to/riscv64-unknown-elf-
../scripts/remote_debug.py <ip_addr> <test_elf>
```

---

## 重要注意事项

1. **切换 CONFIG 前必须 `make clean`** — 平台头文件通过 `-include` 烧入所有 `.o` 文件。切换平台时不清理会因旧的目标文件包含错误的平台定义而导致链接错误或行为异常。

2. **不同平台有不同的内存布局** — QEMU 使用 `MEM_BASE=0x80000000`，HAPS 平台使用 `MEM_BASE=0x60000000`。为一个平台编译的二进制不能在另一个平台上运行。

3. **平台特定的测试排除** — 部分平台定义了 `SKIP_BREAKPOINT_TESTS` 或 `PLATFORM_CLEAR_MAEE` 等标志来改变测试行为。请查阅 `platform_config.h` 了解平台特定约束。

4. **RV32 与 RV64** — 使用 `CONFIG=qemu-rv32-max` 配合 `XLEN=32` 进行 RV32 构建。并非所有扩展都支持 RV32。

5. **S/U-mode 需要 PMP 覆盖** — 如果没有任何匹配的 PMP entry，S-mode 和 U-mode 的所有访问都会被拒绝。切换特权级前务必至少配置一个覆盖固件代码区域的 PMP entry。

6. **`TEST_END()` 包含 `return`** — 不要在 `TEST_END()` 之后编写代码。

测试编写指南和核心 API 参考请参阅 [`DOCS/develop_guide/`](DOCS/develop_guide/)。

---

## 已实现的扩展

| 分类 | 扩展 | 说明 |
|------|------|------|
| **内存保护** | `pmp` | PMP 物理内存保护 |
| | `smepmp` | Smepmp（PMP M-mode 增强） |
| | `spmp` | SPMP（S-level 物理内存保护） |
| **虚拟内存** | `Sv39` | Sv39（3 级页表） |
| | `Sv48` | Sv48（4 级页表） |
| | `Sv57` | Sv57（5 级页表） |
| | `Svbare` | Svbare（satp.MODE=Bare） |
| | `Svnapot` | NAPOT 翻译连续性 |
| | `Svpbmt` | 页级内存类型 |
| | `Svinval` | 细粒度 TLB 无效化 |
| | `Svade` | 硬件 A/D 位异常 |
| | `Svadu` | 硬件 A/D 位自动更新 |
| | `Svvptc` | 虚拟化页表缓存 |
| | `Svrsw60t59b` | PTE reserved 位扩展 |
| **PMP+VM 交互** | `pmp_sv39` | PMP + Sv39 交互 |
| | `pmp_sv48` | PMP + Sv48 交互 |
| | `pmp_sv57` | PMP + Sv57 交互 |
| **Hypervisor** | `Hypervisor` | Hypervisor 基础扩展 |
| | `Sv39x4` | Sv39x4 G-stage 翻译 |
| | `Sv48x4` | Sv48x4 G-stage 翻译 |
| | `Sv57x4` | Sv57x4 G-stage 翻译 |
| | `Sv39x4_Sv39` | 两阶段：Sv39x4 + Sv39 |
| | `Sv39x4_Sv48` | 两阶段：Sv39x4 + Sv48 |
| | `Sv39x4_Sv57` | 两阶段：Sv39x4 + Sv57 |
| | `Sv48x4_Sv39` | 两阶段：Sv48x4 + Sv39 |
| | `Sv48x4_Sv48` | 两阶段：Sv48x4 + Sv48 |
| | `Sv48x4_Sv57` | 两阶段：Sv48x4 + Sv57 |
| | `Sv57x4_Sv39` | 两阶段：Sv57x4 + Sv39 |
| | `Sv57x4_Sv48` | 两阶段：Sv57x4 + Sv48 |
| | `Sv57x4_Sv57` | 两阶段：Sv57x4 + Sv57 |
| **Hypervisor CSR** | `Sha` | Hypervisor 地址翻译 |
| | `Shgatpa` | G-stage 页表 |
| | `Shcounterenw` | Hypervisor 计数器使能 |
| | `Shlcofideleg` | 计数器溢出委托 |
| | `Shtvala` | Hypervisor trap 值 |
| | `Shvsatpa` | VS-stage 地址翻译 |
| | `Shvstvala` | VS-stage trap 值 |
| | `Shvstvecd` | VS-stage trap 向量 |
| **Hypervisor 组合** | `Hypervisor_Smcsrind` | Hyp + Smcsrind |
| | `Hypervisor_Smstateen` | Hyp + Smstateen |
| | `Hypervisor_Ssccptr` | Hyp + Ssccptr |
| | `Hypervisor_Sscsrind` | Hyp + Sscsrind |
| | `Hypervisor_Ssdbltrp` | Hyp + Ssdbltrp |
| | `Hypervisor_Ssstateen` | Hyp + Ssstateen |
| | `Hypervisor_Sstc` | Hyp + Sstc |
| | `Hypervisor_Sstvala` | Hyp + Sstvala |
| | `Hypervisor_Svadu` | Hyp + Svadu |
| | `Hypervisor_Svinval` | Hyp + Svinval |
| | `Hypervisor_Svnapot` | Hyp + Svnapot |
| | `Hypervisor_Svpbmt` | Hyp + Svpbmt |
| **Machine-mode (Sm*)** | `Smstateen` | 状态使能 |
| | `Smcdeleg` | 计数器委托 |
| | `Smcsrind` | 间接 CSR 访问 |
| | `Smdbltrp` | 双重 trap |
| **Supervisor (Ss*)** | `Ssccptr` | CBO 缓存操作 |
| | `Sscofpmf` | 计数器溢出/模式过滤 |
| | `Sscounterenw` | 计数器使能可写性 |
| | `Ssstateen` | 状态使能 |
| | `Sstc` | S-mode 定时器比较 |
| | `Sstvala` | Trap 值 |
| | `Sstvecd` | Trap 向量 |
| | `Ssu64xl` | UXL 字段控制 |
| | `Ssccfg` | 计数器配置 |
| | `Sscsrind` | 间接 CSR 访问 |
| | `ssctr` | 计数器触发 |
| | `Ssdbltrp` | 双重 trap |
| **AIA（中断）** | `aia_aplic` | APLIC |
| | `aia_imsic` | IMSIC |
| | `aia_smaia` | S-mode AIA |
| | `aia_iommu` | IOMMU |
| | `aia_hypervisor` | Hypervisor AIA |
| **其他** | `zpm` | Pointer Masking |

---

## 设计理念

框架将**共享基础设施**与**扩展特定逻辑**分离。`common/` 提供启动引导、trap 处理、特权切换和测试框架，每个扩展目录编译为独立的裸机 ELF 二进制文件。

核心设计原则：

1. **零耦合** — `common/` 不包含任何扩展头文件的 `#include`。扩展通过弱符号和链接时组合注入行为。
2. **独立裸机二进制** — 每个扩展是完全自包含的 ELF，可直接被 QEMU、Sail 或 Spike 加载。
3. **弱符号钩子** — `entry.S` 调用 `_platform_init`（弱符号，默认为空操作）。扩展可提供强定义来执行自定义初始化。
4. **通过 `-include` 实现平台抽象** — 平台头文件（`platform_config.h` 和 `rvtest_config.h`）在编译时注入，避免在源文件中硬编码 `#include "platform_config.h"`。
5. **RV32/RV64 双架构支持** — 所有汇编使用从 `__riscv_xlen` 派生的条件编译宏。
6. **确定性 Trap 处理** — 所有内存操作使用非压缩指令（`.option norvc`），使 trap handler 可以可靠地通过 `mepc += 4` 跳过故障指令。
7. **条件编译公共库** — 扩展通过在 Makefile 中设置 `ENABLE_PMP=1`、`ENABLE_VM=1`、`ENABLE_HYP=1` 或 `ENABLE_PM=1` 按需链接。

---

## 延伸阅读

- [RISC-V 特权态规范](https://github.com/riscv/riscv-isa-manual)
- [`DOCS/framework/`](DOCS/framework/) — 子系统框架文档
- [`DOCS/testplan/`](DOCS/testplan/) — 扩展测试方案
- [`DOCS/develop_guide/`](DOCS/develop_guide/) — 开发者指南
- [`SPEC/`](SPEC/) — 规范摘录
- [`COVERPOINTS/`](COVERPOINTS/) — 覆盖率 coverpoint 定义
