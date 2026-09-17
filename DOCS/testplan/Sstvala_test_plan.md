**中文 | [English](../testplan_en/Sstvala_test_plan_en.md)**

# Sstvala 扩展测试计划

本文档描述 Sstvala（Trap Value Reporting, Version 1.0）扩展的测试计划。Sstvala 扩展规定了 `stval` CSR 在不同异常类型下必须写入的值：对于地址类异常（page-fault、access-fault、misaligned、非 EBREAK 的 breakpoint），`stval` 必须写入故障虚拟地址；对于指令类异常（illegal-instruction、virtual-instruction），`stval` 必须写入故障指令编码。

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/sstvala.adoc` — Sstvala Extension for Trap Value Reporting, Version 1.0
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — `stval` CSR 定义与写入行为、各类异常 cause 编码
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — `mtval` 与 `stval` 对称行为、medeleg 委托
- `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — virtual-instruction 异常（cause=22）触发条件（Group 6 已迁移）

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内 src/priv/sstvala.adoc、src/priv/supervisor.adoc、src/priv/machine.adoc、src/priv/hypervisor.adoc）

---

## 测试范围

### 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sstvala_stval_faulting_vaddr` | If the Sstvala extension is implemented, then `stval` must be written with the faulting virtual address for load, store, and instruction page-fault, access-fault, and misaligned exceptions, and for breakpoint exceptions that are defined to write an address to stval, other than those caused by execution of the `EBREAK` or `C.EBREAK` instructions. | 如果实现了 Sstvala 扩展，则对于加载、存储和指令页错误、访问错误及非对齐异常，以及定义为向 stval 写入地址的断点异常（由 `EBREAK` 或 `C.EBREAK` 指令引起的除外），`stval` 必须写入故障虚拟地址。 |
| `norm:sstvala_stval_faulting_instruction` | For virtual-instruction and illegal-instruction exceptions, `stval` must be written with the faulting instruction. | 对于虚拟指令和非法指令异常，`stval` 必须写入故障指令。 |

**自行拆解的规范点**（从上述两个官方 norm 拆解出的按异常类型分组的可测断言，SPEC 无独立 norm 标签）：

| Norm ID | 中文说明 |
|---------|----------|
| `Sstvala_pagefault_tval_addr` | load/store/instruction page-fault 时，`stval` 必须写入故障虚拟地址 |
| `Sstvala_accessfault_tval_addr` | load/store/instruction access-fault 时，`stval` 必须写入故障虚拟地址 |
| `Sstvala_misaligned_tval_addr` | load/store/instruction misaligned 异常时，`stval` 必须写入故障虚拟地址 |
| `Sstvala_breakpoint_tval_addr` | 对于 breakpoint 异常（cause=3），若非由 EBREAK/C.EBREAK 指令触发，且规范定义应写地址到 stval，则 `stval` 必须写入故障地址 |
| `Sstvala_illegal_inst_tval_inst` | illegal-instruction 异常时，`stval` 必须写入故障指令编码 |
| `Sstvala_virtual_inst_tval_inst` | virtual-instruction 异常时，`stval` 必须写入故障指令编码 |

> [!IMPORTANT]
> Sstvala 规范的核心分为两大类：
> 1. **地址类异常**（page-fault、access-fault、misaligned、breakpoint）→ `stval` = 故障虚拟地址
> 2. **指令类异常**（illegal-instruction、virtual-instruction）→ `stval` = 故障指令编码
>
> 所有测试用例围绕"触发特定异常 → 验证 `stval` 值"展开。

### 不在测试范围内

- **`mtval` 的行为**：Sstvala 仅约束 `stval`（S-mode trap value），不涉及 `mtval`（M-mode）。但由于测试框架在 M-mode 捕获 trap 时使用 `mtval`，而规范中 `mtval` 的行为与 `stval` 对称，因此 M-mode trap 的 `mtval` 验证可作为等效证据
- **EBREAK / C.EBREAK 导致的 breakpoint**：规范明确排除，`stval` 行为由其他规范定义
- **多 hart 场景**：项目为单核测试环境
- **Sv32 / Sv48 / Sv57 模式**：仅覆盖 RV64 + Sv39，与项目其它扩展计划保持一致
- **Guest page-fault（cause 20/21/23）**：涉及 H 扩展的两级翻译，由 Hypervisor 测试计划独立覆盖

---

## 设计要点

### 1. stval 验证的通用模式

所有测试用例遵循统一模式：

1. 设置触发条件（页表配置 / PMP 配置 / 特殊指令）
2. arm trap → 执行触发指令 → disarm trap
3. 断言 trap 已触发
4. 断言 trap cause 等于预期值
5. 断言 trap tval 等于预期 stval 值（地址或指令编码）

框架中 M-mode 与 S-mode trap handler 都会捕获 `mtval`/`stval` 并保存到 trap 状态记录中，测试用例通过统一接口读取该值。

### 2. 地址类异常的 stval 验证

对于 page-fault、access-fault、misaligned 异常，`stval` 应等于触发异常的虚拟地址。验证时需要：

- **已知目标地址**：测试代码显式构造访问目标地址，然后断言捕获的 tval 等于该地址
- **M-mode vs S-mode**：page-fault 需要 VM 启用（S-mode），access-fault 可在 M-mode（PMP）或 S-mode 触发，misaligned 在任何模式下都可触发

### 3. 指令类异常的 stval 验证

对于 illegal-instruction 异常，`stval` 应等于故障指令的编码值。验证时：

- **已知指令编码**：在内存中预置已知的非法指令字节序列，然后执行或在 trap 后读取捕获的 tval 与预期编码比较
- **32 位指令**：`stval` 应为完整的 32 位指令编码（零扩展到 XLEN）
- **16 位压缩指令**：`stval` 应为 16 位指令编码（零扩展到 XLEN）

### 4. VM 配置（Page-Fault 场景）

Page-fault 测试需要启用 Sv39 页表：

- **代码/数据区域**：identity mapping，`PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D`，确保测试代码和 trap handler 可正常运行
- **测试区域**：故意缺少映射（unmapped VA）或权限不足（只读页写入），触发 page-fault
- 使用框架的 S-mode 执行辅助进入 S-mode，异常被 S-mode trap handler 捕获

### 5. PMP 配置（Access-Fault 场景）

Access-fault 通过 PMP 限制触发：

- 配置 PMP 条目，使特定地址区域对 S/U-mode 不可读/不可写/不可执行
- 在 S-mode 或 U-mode 下访问该区域，触发 access-fault
- 验证捕获的 tval 等于被拒绝访问的地址

### 6. EBREAK 排除说明

规范明确排除 EBREAK/C.EBREAK 导致的 breakpoint。EBREAK 的 `stval` 行为由基础特权级规范定义（通常为 0 或 PC），不属于 Sstvala 测试范围。Group 4 仅测试由 trigger 模块（如果实现）或其他机制触发的 breakpoint 异常。

> [!NOTE]
> 若平台未实现 trigger 模块（Sdtrig 扩展），Group 4 的用例将被 `TEST_SKIP` 跳过，不影响整体测试结论。

### 7. Virtual Instruction 异常说明

Virtual-instruction 异常（cause=22）需要 H 扩展（Hypervisor）支持，在 VS-mode 下执行某些 HS-level CSR 访问指令时触发。Group 6 的 3 个测试已迁移至 [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) Group 2（ID 为 HCROSS-SSTVALA-06~08）。本文件保留 Group 6 的描述供参考。

### 8. Misaligned 平台能力探测

平台是否支持硬件非对齐访问决定了 misaligned 异常是否会被触发。测试用例在运行时通过尝试非对齐访问并观察是否触发异常来探测平台能力：

- 若平台不支持硬件非对齐访问，触发 misaligned 异常，正常验证 tval 值
- 若平台支持硬件非对齐访问，非对齐访问不产生异常，相关用例 `TEST_SKIP`
- Instruction misaligned（cause=0）通常在跳转到非 2 字节对齐地址时总能触发，不受平台非对齐 load/store 能力影响

任一平台违反 SPEC 时用例保持 FAIL，实现缺陷记录至 `bugs/` 目录。

---

## 测试分组

> [!IMPORTANT]
> 共 6 个测试组、25 个测试用例。Group 1–3 为地址类异常核心测试，Group 4 为 breakpoint（条件性），Group 5 为指令类异常核心测试，Group 6 为可选（需 H 扩展，已迁移）。

---

### Group 1：Page-Fault 地址类异常（stval = 故障虚拟地址）

**规范依据**：
- `Sstvala_pagefault_tval_addr`：load/store/instruction page-fault 时，`stval` 必须写入故障虚拟地址

**测试职责**：验证在 Sv39 虚拟内存模式下，当 S-mode 访问未映射或权限不足的虚拟地址触发 page-fault 时，`stval`（通过 M-mode `mtval` 等效捕获）等于触发异常的虚拟地址。

**前置条件**：
- 启用 Sv39 页表，代码/栈区域 identity mapping（`PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D`）
- 测试目标虚拟地址未映射或权限受限
- M-mode `medeleg` 委托 page-fault 到 S-mode（或由 M-mode handler 直接捕获）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TVAL-LPF-01 | load page-fault：未映射 VA | 在 Sv39 下，S-mode 对未映射的 VA（如 `0x40000000`）执行 `ld`，触发 load page-fault（cause=13） | cause == 13；tval == `0x40000000` |
| TVAL-LPF-02 | load page-fault：写权限页读取（反向验证） | 4 KiB PTE：V=1、W=1、R=1、X=0、A=1、D=1，S-mode load 应成功 | 无异常（反向验证，确认正确映射不触发 fault） |
| TVAL-SPF-01 | store page-fault：只读页写入 | 4 KiB PTE：V=1、R=1、W=0、A=1、D=1，S-mode 对该 VA 执行 `sd`，触发 store page-fault（cause=15） | cause == 15；tval == 测试 VA |
| TVAL-SPF-02 | store page-fault：未映射 VA | S-mode 对未映射 VA 执行 `sd`，触发 store page-fault（cause=15） | cause == 15；tval == 未映射 VA |
| TVAL-IPF-01 | instruction page-fault：未映射 VA fetch | S-mode 跳转到未映射的 VA 执行取指，触发 instruction page-fault（cause=12） | cause == 12；tval == 目标 PC |
| TVAL-IPF-02 | instruction page-fault：不可执行页 fetch | 4 KiB PTE：V=1、R=1、W=0、X=0、A=1、D=1，S-mode 跳转执行，触发 instruction page-fault（cause=12） | cause == 12；tval == 目标 PC |
| TVAL-LPF-03 | load page-fault：非规范地址 | S-mode 对非规范 VA（如 Sv39 下 bit[63:39] 不一致的地址 `0x4000000000`）执行 load | cause == 13；tval == `0x4000000000` |

---

### Group 2：Access-Fault 地址类异常（stval = 故障虚拟地址）

**规范依据**：
- `Sstvala_accessfault_tval_addr`：load/store/instruction access-fault 时，`stval` 必须写入故障虚拟地址

**测试职责**：验证当 PMP 限制导致 access-fault 时，`stval` 等于被拒绝访问的虚拟地址。Access-fault 通过 PMP 配置在 M-mode 下限制 S/U-mode 对特定区域的访问权限来触发。

**前置条件**：
- PMP 配置：至少一个条目限制特定地址区域对 S-mode 不可读/不可写/不可执行
- `medeleg` 委托 access-fault 到 S-mode（或由 M-mode handler 直接捕获）
- 可选择启用或不启用 VM（access-fault 在物理地址层面检查）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TVAL-LAF-01 | load access-fault：PMP 禁读区域 | 配置 PMP 使特定区域对 S-mode 不可读，S-mode 对该区域执行 `ld` | cause == 5（LAF）；tval == 目标地址 |
| TVAL-LAF-02 | load access-fault：不同地址验证 | 同 LAF-01 但使用不同的目标地址，确认 stval 跟随实际访问地址变化 | cause == 5；tval == 另一地址 |
| TVAL-SAF-01 | store access-fault：PMP 禁写区域 | 配置 PMP 使特定区域对 S-mode 不可写，S-mode 对该区域执行 `sd` | cause == 7（SAF）；tval == 目标地址 |
| TVAL-IAF-01 | instruction access-fault：PMP 禁执行区域 | 配置 PMP 使特定区域对 S-mode 不可执行，S-mode 跳转到该区域取指 | cause == 1（IAF）；tval == 目标 PC |

---

### Group 3：Misaligned 地址类异常（stval = 故障虚拟地址）

**规范依据**：
- `Sstvala_misaligned_tval_addr`：load/store/instruction misaligned 异常时，`stval` 必须写入故障虚拟地址

**测试职责**：验证当 load/store 执行非自然对齐的内存访问触发 misaligned 异常时，`stval` 等于未对齐的访问地址。

**前置条件**：
- 平台必须不支持硬件非对齐访问（即非对齐访问会触发异常而非硬件透明处理）。若平台支持硬件非对齐访问，相关用例应被 `TEST_SKIP` 跳过
- Instruction misaligned（cause=0）仅在跳转到非 2 字节对齐地址时触发（若支持 C 扩展则需非 2 字节对齐，否则需非 4 字节对齐）

> [!WARNING]
> 许多现代 RISC-V 实现在默认配置下**支持硬件非对齐 load/store 访问**，不会触发 misaligned 异常。因此 TVAL-LMA-01 和 TVAL-SMA-01 在这些平台上会被跳过。TVAL-IMA-01（instruction misaligned）通常可以在任何平台上测试，因为非 2 字节对齐的 PC 总是触发异常。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TVAL-LMA-01 | load misaligned：非对齐 load | M-mode 对非 8 字节对齐地址执行 `ld`（如 addr+3），若平台不支持非对齐访问则触发 cause=4 | cause == 4；tval == (addr+3)；若平台支持非对齐访问则 `TEST_SKIP` |
| TVAL-SMA-01 | store misaligned：非对齐 store | M-mode 对非 8 字节对齐地址执行 `sd`（如 addr+5），若平台不支持非对齐访问则触发 cause=6 | cause == 6；tval == (addr+5)；若平台支持非对齐访问则 `TEST_SKIP` |
| TVAL-IMA-01 | instruction misaligned：跳转到奇数地址 | 使用 `jalr` 跳转到奇数地址（如 `target \| 1`），触发 instruction address misaligned（cause=0） | cause == 0；tval == (target \| 1) |

---

### Group 4：Breakpoint 异常（stval = 故障地址，非 EBREAK）

**规范依据**：
- `Sstvala_breakpoint_tval_addr`：对于 breakpoint 异常（cause=3），若非由 EBREAK/C.EBREAK 指令触发，且规范定义应写地址到 stval，则 `stval` 必须写入故障地址

**测试职责**：验证由硬件 trigger（Sdtrig 扩展）触发的 breakpoint 异常中，`stval` 写入断点命中的地址。

**前置条件**：
- 平台必须实现 Sdtrig 扩展（Debug Trigger 模块），提供 `tselect`、`tdata1`、`tdata2` 等 CSR
- 若平台未实现 trigger 模块，所有用例 `TEST_SKIP`

> [!NOTE]
> Sdtrig 扩展不在所有平台上可用。若平台不支持，本组所有用例自动跳过，不影响整体测试结论。EBREAK/C.EBREAK 触发的 breakpoint 被规范明确排除，不在本组测试范围内。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TVAL-BKP-01 | trigger breakpoint：地址匹配 load | 配置 trigger 在特定地址触发 load breakpoint（type=6 mcontrol6，load=1），M-mode 执行 load 到该地址 | cause == 3；tval == trigger 地址 |
| TVAL-BKP-02 | trigger breakpoint：地址匹配 instruction | 配置 trigger 在特定 PC 触发 execute breakpoint（execute=1），执行到该 PC | cause == 3；tval == trigger PC |
| TVAL-BKP-03 | EBREAK 排除验证（反向） | 执行 EBREAK 指令，验证 breakpoint 被触发但**不断言** stval 为特定值（Sstvala 不约束 EBREAK 的 stval） | cause == 3；stval 值不做断言（仅记录） |

---

### Group 5：Illegal Instruction 指令类异常（stval = 故障指令编码）

**规范依据**：
- `Sstvala_illegal_inst_tval_inst`：illegal-instruction 异常时，`stval` 必须写入故障指令编码

**测试职责**：验证当执行非法指令触发 illegal-instruction 异常（cause=2）时，`stval` 等于触发异常的指令编码值（32 位或 16 位指令，零扩展到 XLEN）。

**前置条件**：
- 在内存中预置已知编码的非法指令
- 使用框架的执行辅助跳转到该地址执行，或直接用内联汇编嵌入非法指令

> [!IMPORTANT]
> 指令编码验证是 Sstvala 测试的另一核心部分。对于 32 位指令，`stval` 应为完整的 32 位编码（零扩展到 XLEN 宽度）；对于 16 位压缩非法指令，`stval` 应为 16 位编码（零扩展到 XLEN 宽度）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TVAL-ILL-01 | 32 位非法指令：custom-0 操作码 | 在内存预置 `0x0000000B`（bits[1:0]=11 表示 32 位指令，opcode[6:0]=0001011 即 custom-0，通常未实现），跳转执行 | cause == 2；tval == `0x0000000B` |
| TVAL-ILL-02 | 32 位非法指令：写只读 CSR | 在内存预置 `0xC0001073`（`csrrw x0, 0xC00, x0`，写入只读 CSR `cycle`），跳转执行 | cause == 2；tval == `0xC0001073` |
| TVAL-ILL-03 | 32 位非法指令：访问不存在的 CSR | 在内存预置 `0xFFF022F3`（`csrrs x5, 0xFFF, x0`，访问不存在的 CSR 0xFFF），跳转执行 | cause == 2；tval == `0xFFF022F3` |
| TVAL-ILL-04 | 16 位压缩非法指令 | 在内存预置 16 位全零编码 `0x0000`（C 扩展中全零为非法压缩指令，bits[1:0]=00 表示 16 位指令），跳转执行 | cause == 2；tval == `0x0000`（零扩展到 XLEN） |
| TVAL-ILL-05 | 连续两次非法指令 stval 不同 | 依次执行 `0x0000000B`（custom-0）和 `0xC0001073`（写只读 CSR）两条不同非法指令，验证 stval 每次对应实际指令 | 第 1 次 tval == `0x0000000B`；第 2 次 tval == `0xC0001073` |

---

### Group 6：Virtual Instruction 指令类异常（stval = 故障指令编码，可选）

> **[迁移]** 本组 3 个测试已迁移至 [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) **Group 2 (Hypervisor × Sstvala 交叉测试)**，ID 映射：TVAL-VI-01~03 → HCROSS-SSTVALA-06~08。本文件保留原始描述供参考，实际实现和运行以交叉测试计划为准。

**规范依据**：
- `Sstvala_virtual_inst_tval_inst`：virtual-instruction 异常时，`stval` 必须写入故障指令编码

**测试职责**：验证当 VS-mode 执行需要 HS 权限的 CSR 指令触发 virtual-instruction 异常（cause=22）时，`stval` 等于触发异常的指令编码。

**前置条件**：
- 平台必须实现 H 扩展
- 测试运行在 VS-mode 下，执行 HS-level CSR 访问（如 `hgatp`、`hstatus`）

> [!WARNING]
> 本组为**可选测试组**，仅在启用 H 扩展的构建配置下编译和执行。若未启用 H 扩展，整组 `TEST_SKIP`。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TVAL-VI-01 | virtual-instruction：VS-mode 读 hstatus | VS-mode 执行 `csrrs x5, hstatus, x0`（CSR 0x600），触发 virtual-instruction（cause=22） | cause == 22；tval == `0x600022F3` |
| TVAL-VI-02 | virtual-instruction：VS-mode 写 hgatp | VS-mode 执行 `csrrw x0, hgatp, x0`（CSR 0x680），触发 virtual-instruction（cause=22） | cause == 22；tval == `0x68001073` |
| TVAL-VI-03 | virtual-instruction：VS-mode 读 hideleg | VS-mode 执行 `csrrs x5, hideleg, x0`（CSR 0x603），触发 virtual-instruction（cause=22） | cause == 22；tval == `0x603022F3` |

> [!NOTE]
> **TVAL-VI-03 CSR 选择说明**：原始设计曾考虑使用 `vsstatus`（CSR 0x200），但 VS-mode 下 `vsstatus` 实际是 `sstatus` 的透明别名，VS-mode 可正常访问，不会触发 virtual-instruction 异常。因此改用 `hideleg`（CSR 0x603），这是 HS-level CSR，VS-mode 访问时必定触发 virtual-instruction。

> [!NOTE]
> **指令编码推导**：
> - `csrrs x5, 0x600, x0`：`[31:20]=0x600 [19:15]=00000 [14:12]=010 [11:7]=00101 [6:0]=1110011` = `0x600022F3`
> - `csrrw x0, 0x680, x0`：`[31:20]=0x680 [19:15]=00000 [14:12]=001 [11:7]=00000 [6:0]=1110011` = `0x68001073`
> - `csrrs x5, 0x603, x0`：`[31:20]=0x603 [19:15]=00000 [14:12]=010 [11:7]=00101 [6:0]=1110011` = `0x603022F3`

---

## 异常 Cause 值参考

| 名称 | 值 | 说明 | stval 含义（Sstvala） |
|------|-----|------|------|
| Instruction address misaligned | 0 | 指令地址未对齐 | 故障虚拟地址 |
| Instruction access fault | 1 | 指令访问故障（PMP/PMA） | 故障虚拟地址 |
| Illegal instruction | 2 | 非法指令 | 故障指令编码 |
| Breakpoint | 3 | 断点（非 EBREAK 时） | 故障地址 |
| Load address misaligned | 4 | Load 地址未对齐 | 故障虚拟地址 |
| Load access fault | 5 | Load 访问故障（PMP/PMA） | 故障虚拟地址 |
| Store address misaligned | 6 | Store 地址未对齐 | 故障虚拟地址 |
| Store access fault | 7 | Store 访问故障（PMP/PMA） | 故障虚拟地址 |
| Instruction page fault | 12 | 指令页面故障 | 故障虚拟地址 |
| Load page fault | 13 | Load 页面故障 | 故障虚拟地址 |
| Store page fault | 15 | Store 页面故障 | 故障虚拟地址 |
| Virtual instruction | 22 | 虚拟指令异常（H 扩展） | 故障指令编码 |

---

## 测试用例总览

| Group | 测试数量 | 异常类型 | stval 语义 | 依赖 |
|-------|---------|----------|------------|------|
| Group 1 | 7 | Page-Fault（cause 12/13/15） | 故障虚拟地址 | VM（Sv39） |
| Group 2 | 4 | Access-Fault（cause 1/5/7） | 故障虚拟地址 | PMP |
| Group 3 | 3 | Misaligned（cause 0/4/6） | 故障虚拟地址 | 平台相关 |
| Group 4 | 3 | Breakpoint（cause 3） | 故障地址 | Sdtrig（可选） |
| Group 5 | 5 | Illegal Instruction（cause 2） | 故障指令编码 | 无 |
| Group 6 | 3 | Virtual Instruction（cause 22） | 故障指令编码 | H 扩展（可选，**已迁移**） |
| **总计** | **25** | | | |

---

## 附录 A：规范点覆盖矩阵

| Norm ID | 覆盖的测试 ID | 覆盖状态 | 备注 |
|---------|--------------|----------|------|
| `norm:sstvala_stval_faulting_vaddr` | TVAL-LPF-01 ~ TVAL-LPF-03、TVAL-SPF-01 ~ TVAL-SPF-02、TVAL-IPF-01 ~ TVAL-IPF-02、TVAL-LAF-01 ~ TVAL-LAF-02、TVAL-SAF-01、TVAL-IAF-01、TVAL-LMA-01、TVAL-SMA-01、TVAL-IMA-01、TVAL-BKP-01、TVAL-BKP-02 | 已覆盖 | 地址类异常 stval = 故障虚拟地址 |
| `norm:sstvala_stval_faulting_instruction` | TVAL-ILL-01 ~ TVAL-ILL-05、TVAL-VI-01 ~ TVAL-VI-03 | 已覆盖 | 指令类异常 stval = 故障指令编码 |
| `Sstvala_pagefault_tval_addr` | TVAL-LPF-01 ~ TVAL-LPF-03、TVAL-SPF-01 ~ TVAL-SPF-02、TVAL-IPF-01 ~ TVAL-IPF-02 | 已覆盖 | page-fault 分组 |
| `Sstvala_accessfault_tval_addr` | TVAL-LAF-01 ~ TVAL-LAF-02、TVAL-SAF-01、TVAL-IAF-01 | 已覆盖 | access-fault 分组 |
| `Sstvala_misaligned_tval_addr` | TVAL-LMA-01、TVAL-SMA-01、TVAL-IMA-01 | 已覆盖 | misaligned 分组 |
| `Sstvala_breakpoint_tval_addr` | TVAL-BKP-01、TVAL-BKP-02、TVAL-BKP-03 | 已覆盖 | breakpoint 分组（非 EBREAK） |
| `Sstvala_illegal_inst_tval_inst` | TVAL-ILL-01 ~ TVAL-ILL-05 | 已覆盖 | illegal-instruction 分组 |
| `Sstvala_virtual_inst_tval_inst` | TVAL-VI-01 ~ TVAL-VI-03 | 已覆盖（已迁移） | virtual-instruction 分组，实际实现见 Hypervisor_cross_test_plan.md Group 2 |
