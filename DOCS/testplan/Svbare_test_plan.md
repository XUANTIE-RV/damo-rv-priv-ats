**中文 | [English](../testplan_en/Svbare_test_plan_en.md)**

# Svbare 扩展测试计划

本文档描述 Svbare（Supervisor Bare Mode Support）扩展的测试计划。Svbare 扩展规定：实现必须支持 `satp` 寄存器的 MODE 字段能够保持 Bare 值（MODE=0）。当 MODE=Bare 时，supervisor 虚拟地址等于 supervisor 物理地址，不进行页表翻译，除 PMP 外无额外内存保护。

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — `satp` 寄存器 MODE 字段语义、Bare 模式定义、Svbare 扩展定义、`sstatus`.SUM/MXR 位语义

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内上述路径文件）

---

## 概述

RISC-V 特权级规范中，`satp`（Supervisor Address Translation and Protection）寄存器控制地址翻译模式。其 MODE 字段决定了地址翻译方案：

1. **MODE=Bare（值 0）**：不进行地址翻译和保护。supervisor 虚拟地址直接等于物理地址，唯一的内存保护机制是 PMP（Physical Memory Protection）。
2. **MODE=Sv39/Sv48/Sv57**：启用对应级别的页表虚拟内存系统。

Svbare 扩展的核心要求是：**实现必须支持 `satp.MODE` 字段能保持 Bare 值**。这意味着软件可以通过写 `satp=0` 来禁用虚拟内存。

选择 MODE=Bare 时，软件必须将 `satp` 的剩余字段（ASID、PPN）全部写零。若以非零模式写入剩余字段，其效果为 UNSPECIFIED。

本测试计划聚焦以下方面：
- `satp.MODE=Bare` 的 CSR 可写性与回读一致性
- Bare 模式下 VA=PA 直通内存访问行为
- Bare 模式下 SUM/MXR 位无效性验证
- Bare 模式与 PMP 的交互
- satp MODE 在 Bare 与 Sv39 之间的切换
- 边界条件与特殊场景

---

## 覆盖的规范点

下表列出本方案覆盖的规范点。带 `norm:` 前缀的为 SPEC 官方 normative rule 标签。

| Norm ID | 原文（要点） | 中文说明 |
|---------|------|----------|
| `norm:svbare_satp_mode_bare` | If an implementation supports the Svbare extension, then the `satp` register's MODE field must be capable of holding the value Bare. | 若实现支持 Svbare 扩展，`satp` 的 MODE 字段必须能保存 Bare 值。 |
| `norm:satp_mode` | When MODE=Bare, supervisor virtual addresses are equal to supervisor physical addresses, and there is no additional memory protection. To select MODE=Bare, software must write zero to the remaining fields of `satp`. Attempting to select MODE=Bare with a nonzero pattern in the remaining fields has an UNSPECIFIED effect. | MODE=Bare 时，S 虚拟地址等于 S 物理地址，无额外内存保护。选择 Bare 模式时软件必须将 `satp` 其余字段写零。非零模式时效果未指定。 |
| `norm:satp_mode_op_unsupported` | Implementations are not required to support all MODE settings, and if `satp` is written with an unsupported MODE, the entire write has no effect; no fields in `satp` are modified. | 写入不支持的 MODE 时整个写无效，`satp` 不被修改。 |
| `norm:satp_op_active` | The `satp` CSR is considered active when the effective privilege mode is S-mode or U-mode. | `satp` 仅在有效特权模式为 S-mode 或 U-mode 时激活。 |
| `norm:sstatus_sum` | The SUM bit modifies the privilege with which S-mode loads and stores access virtual memory. SUM has no effect when page-based virtual memory is not in effect, nor when executing in U-mode. | SUM 位控制 S 模式对 U 模式页面的访问；未启用分页或 U 模式下无效。 |
| `norm:sstatus_mxr` | The MXR bit modifies the privilege with which loads access virtual memory. MXR has no effect when page-based virtual memory is not in effect. | MXR 位修改 load 访问虚拟内存的权限；未启用分页时无效。 |

---

## 不在测试范围内

- **Hypervisor 两级翻译场景**：VS-stage 与 G-stage 下的 Bare 模式行为由 `Hypervisor_Sv_test_plan.md` 及相关 Hypervisor 方案覆盖。
- **Sv32（RV32）**：本计划仅覆盖 RV64（SXLEN=64），RV32 下 Bare 的 MODE 编码与 ASID 字段布局不同。
- **多 hart 一致性**：本方案聚焦单 hart 行为。
- **Sv48 / Sv57 模式切换**：本计划仅以 Sv39 作为 Bare 的对比切换模式，其他 Sv 模式行为一致。

---

## 测试分组

### Group 1：satp.MODE=Bare 可写性验证

**规范依据**：
- `norm:svbare_satp_mode_bare`：`satp.MODE` 必须能保持 Bare 值
- `norm:satp_mode`：选择 Bare 须写零到剩余字段；非零剩余字段效果 UNSPECIFIED

**测试职责**：验证 M-mode 下写入 `satp` MODE=Bare 的能力，包括回读一致性、剩余字段清零约束、与其他模式的切换。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVBARE-CSR-01 | satp 写入 MODE=Bare 后回读 | M-mode 写 satp=0（MODE=Bare, 其余全零），回读 | 回读 MODE 字段为 0（Bare） |
| SVBARE-CSR-02 | Bare 模式下 ASID/PPN 字段为零 | 写 satp=0 后回读完整值 | satp 全部 64 位均为 0 |
| SVBARE-CSR-03 | 从 Sv39 切换到 Bare | 先写 satp MODE=Sv39（有效配置），再写 satp=0 | 回读 MODE=Bare，satp=0 |
| SVBARE-CSR-04 | 从 Bare 切换到 Sv39 再回 Bare | Bare→Sv39→Bare 多次切换 | 每次 Bare 回读均为 0 |
| SVBARE-CSR-05 | MODE=Bare + 非零剩余字段 | 写 satp 时 MODE=0 但 ASID/PPN 非零 | UNSPECIFIED：记录实际行为（可能忽略、可能清零、可能拒绝写入） |

---

### Group 2：Bare 模式 VA=PA 直通访问

**规范依据**：
- `norm:satp_mode`：MODE=Bare 时 supervisor 虚拟地址等于 supervisor 物理地址
- `norm:satp_op_active`：`satp` 在 S/U 模式有效

**测试职责**：验证 MODE=Bare 下 S-mode 和 U-mode 的内存访问直接使用物理地址，无翻译。需要配合 PMP 允许 S/U-mode 访问。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVBARE-VA-01 | S-mode Bare load | satp=0, S-mode 从已知物理地址 load | 读取成功，值正确 |
| SVBARE-VA-02 | S-mode Bare store | satp=0, S-mode 向已知物理地址 store | 写入成功，回读验证 |
| SVBARE-VA-03 | S-mode Bare fetch | satp=0, S-mode 从物理地址取指执行 | 执行成功 |
| SVBARE-VA-04 | U-mode Bare load | satp=0, U-mode 从已知物理地址 load | 读取成功（需 PMP 允许） |
| SVBARE-VA-05 | U-mode Bare store | satp=0, U-mode 向已知物理地址 store | 写入成功（需 PMP 允许） |
| SVBARE-VA-06 | Bare 模式多地址访问 | S-mode 依次访问不同物理地址区域 | 每次均直通成功 |

---

### Group 3：Bare 模式下无页表翻译验证

**规范依据**：
- `norm:satp_mode`：MODE=Bare 时无额外内存保护（除 PMP 外）
- `norm:sstatus_sum`：SUM 在无页表虚拟内存时无效（Bare 模式下不产生作用）
- `norm:sstatus_mxr`：MXR 在无页表虚拟内存时无效（Bare 模式下不产生作用）

**测试职责**：验证 Bare 模式下不会触发 page-fault，SUM/MXR 位对 Bare 模式无影响，SFENCE.VMA 无副作用。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVBARE-NOPT-01 | Bare 模式不触发 page-fault | satp=0, S-mode 访问任意 PMP 允许的物理地址 | 无 page-fault（scause != 12/13/15） |
| SVBARE-NOPT-02 | SUM=0 不影响 Bare S-mode 访问 | satp=0, sstatus.SUM=0, S-mode load | 访问成功（SUM 在 Bare 模式无效） |
| SVBARE-NOPT-03 | SUM=1 不影响 Bare S-mode 访问 | satp=0, sstatus.SUM=1, S-mode load | 访问成功（SUM 在 Bare 模式无效） |
| SVBARE-NOPT-04 | MXR 不影响 Bare 模式行为 | satp=0, sstatus.MXR=1 或 0, S-mode load | 访问行为一致（MXR 在 Bare 模式无效） |
| SVBARE-NOPT-05 | Bare 模式下 SFENCE.VMA 无副作用 | satp=0, 执行 SFENCE.VMA 后正常访问 | 访问成功，无异常 |

---

### Group 4：Bare 模式与 PMP 交互

**规范依据**：
- `norm:satp_mode`：MODE=Bare 时无额外内存保护，仅 PMP 有效

**测试职责**：验证 Bare 模式下 PMP 仍然是唯一的内存保护机制。PMP 允许时访问成功，PMP 拒绝时触发 access-fault（非 page-fault）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVBARE-PMP-01 | PMP 允许 + Bare S-mode load | satp=0, PMP RWX 全开, S-mode load | 读取成功 |
| SVBARE-PMP-02 | PMP 拒绝 + Bare S-mode load | satp=0, PMP 不含目标地址, S-mode load | load access-fault（scause=5） |
| SVBARE-PMP-03 | PMP 只读 + Bare S-mode store | satp=0, PMP 只有 R, S-mode store | store access-fault（scause=7） |
| SVBARE-PMP-04 | PMP 无执行 + Bare S-mode fetch | satp=0, PMP 有 RW 无 X, S-mode fetch | instruction access-fault（scause=1） |
| SVBARE-PMP-05 | PMP 允许 + Bare U-mode load | satp=0, PMP RWX 全开, U-mode load | 读取成功 |
| SVBARE-PMP-06 | PMP 拒绝 + Bare U-mode store | satp=0, PMP 不含目标地址, U-mode store | store access-fault（scause=7） |

---

### Group 5：satp MODE 切换与 Bare 模式转换

**规范依据**：
- `norm:svbare_satp_mode_bare`：MODE 字段必须能保持 Bare 值
- `norm:satp_mode_op_unsupported`：写入不支持的 MODE 时整个写无效，`satp` 不被修改

**测试职责**：验证 satp 在不同 MODE 之间切换时 Bare 模式的正确性，以及写入无效 MODE 后 Bare 的保留行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVBARE-SW-01 | Bare→Sv39→Bare 正常切换 | 切换三次，每次验证回读 | MODE 字段正确反映切换结果 |
| SVBARE-SW-02 | 写入 reserved MODE 后 satp 不变 | satp=0(Bare), 写入 MODE=7(reserved), 回读 | satp 保持 Bare 不变（整个写无效） |
| SVBARE-SW-03 | 写入 reserved MODE 后 S-mode 仍为 Bare | 续 SW-02, S-mode load | 直通访问成功（仍为 Bare） |
| SVBARE-SW-04 | Sv39→Bare 切换后 VA=PA | 从 Sv39 模式切换回 Bare, S-mode 访问 | 直通物理地址访问成功 |
| SVBARE-SW-05 | 多次 Bare↔Sv39 切换稳定性 | 10 次循环切换 Bare/Sv39 | 每次 Bare 回读均正确，每次切回后 VA=PA |

---

### Group 6：Bare 模式特殊场景

**规范依据**：
- `norm:satp_mode`：Bare + 非零剩余字段效果 UNSPECIFIED
- `norm:satp_op_active`：`satp` 仅在 S/U 模式有效，M-mode 不受 `satp` 影响

**测试职责**：覆盖边界条件和特殊场景，包括 M-mode 下 satp 无影响、TVM 位对 S-mode satp 写入的控制等。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SVBARE-EDGE-01 | M-mode 下 Bare 无影响 | satp=0, M-mode 访问 | M-mode 不受 satp 影响，正常访问 |
| SVBARE-EDGE-02 | satp 全 1 写入（MODE 非法） | 写入 satp 全位为 1 | 实现定义：satp 可能不变（整个写无效）或 MODE 部分被截断 |
| SVBARE-EDGE-03 | Bare 模式连续读写 satp 一致性 | 写 satp=0 后连续 N 次回读 | 每次回读均为 0 |
| SVBARE-EDGE-04 | Bare 后 S-mode 写 satp（TVM=0） | mstatus.TVM=0, S-mode 尝试写 satp=0 | 写入成功，satp 保持 Bare |
| SVBARE-EDGE-05 | TVM=1 时 S-mode 写 satp 触发异常 | mstatus.TVM=1, S-mode 写 satp | 触发 illegal instruction exception（scause=2） |

> [!NOTE]
> SVBARE-EDGE-02 涉及实现定义行为，测试用例应记录实际行为而非做强断言。

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1（可写性）、Group 2（VA=PA 直通） | SVBARE-CSR-01~05、SVBARE-VA-01~06 | Svbare 核心：MODE 可保持 Bare 且 Bare 下 VA=PA |
| P1（重要） | Group 3（无页表翻译）、Group 4（PMP 交互）、Group 5（MODE 切换） | SVBARE-NOPT-01~05、SVBARE-PMP-01~06、SVBARE-SW-01~05 | Bare 语义边界、唯一保护机制、切换稳定性 |
| P2（建议） | Group 6（特殊场景） | SVBARE-EDGE-01~05 | 实现定义与边界条件 |

---

## 结果判定原则

- 平台行为偏离 SPEC（如 satp.MODE 无法保持 Bare、Bare 下产生 page-fault、写入不支持 MODE 却修改了 satp 等）：保持用例失败，与 SPEC 比对后将问题记录至 `bugs/` 目录，禁止修改用例或加 workaround 适配错误实现。
- UNSPECIFIED / 实现定义场景（SVBARE-CSR-05、SVBARE-EDGE-02）：记录实际观测行为，不做强断言。

---

## 附录：规范性引用

### satp 寄存器 RV64 布局

```
 63    60 59    44 43                 0
+--------+--------+-------------------+
|  MODE  |  ASID  |       PPN         |
| (4 bit)|(16 bit)|     (44 bit)      |
+--------+--------+-------------------+
```

### satp MODE 编码表

| 值 | 名称 | 说明 |
|-----|------|------|
| 0 | Bare | 不进行地址翻译和保护 |
| 1-7 | - | Reserved for standard use |
| 8 | Sv39 | 39-bit 页表虚拟地址 |
| 9 | Sv48 | 48-bit 页表虚拟地址 |
| 10 | Sv57 | 57-bit 页表虚拟地址 |
| 11 | Sv64 | Reserved for 64-bit virtual addressing |
| 12-13 | - | Reserved for standard use |
| 14-15 | - | Designated for custom use |

### 相关 scause 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| Instruction access fault | 1 | 取指访问错误（PMP） |
| Illegal instruction | 2 | 非法指令 |
| Load access fault | 5 | load 访问错误（PMP） |
| Store/AMO access fault | 7 | store/AMO 访问错误（PMP） |
| Instruction page fault | 12 | 取指页错误 |
| Load page fault | 13 | load 页错误 |
| Store/AMO page fault | 15 | store/AMO 页错误 |

### 相关 mstatus/sstatus 位

| 位 | 名称 | 说明 |
|-----|------|------|
| bit 18 | SUM | Permit Supervisor User Memory access |
| bit 19 | MXR | Make eXecutable Readable |
| bit 20 | TVM | Trap Virtual Memory（S-mode 访问 satp 时触发异常） |

---

## 参考

- `supervisor.adoc` — `satp` 寄存器、Bare 模式、Svbare 扩展、`sstatus`.SUM/MXR 定义
- `Svadu_test_plan.md`、`Svnapot_test_plan.md` — 其他 Sv* 扩展测试计划
- `Hypervisor_Sv_test_plan.md` — Hypervisor × Sv* 交叉测试计划

---

## 附录 A：规范点覆盖矩阵

下表标明"覆盖的规范点"章节中每条规范点被哪些测试用例覆盖。

| Norm ID | 覆盖的测试 ID |
|---------|---------------|
| `norm:svbare_satp_mode_bare` | SVBARE-CSR-01~05、SVBARE-SW-01~05 |
| `norm:satp_mode` | SVBARE-CSR-02、SVBARE-CSR-05、SVBARE-VA-01~06、SVBARE-NOPT-01~05、SVBARE-PMP-01~06 |
| `norm:satp_mode_op_unsupported` | SVBARE-SW-02、SVBARE-SW-03、SVBARE-EDGE-02 |
| `norm:satp_op_active` | SVBARE-VA-01~06、SVBARE-EDGE-01、SVBARE-EDGE-04、SVBARE-EDGE-05 |
| `norm:sstatus_sum` | SVBARE-NOPT-02、SVBARE-NOPT-03 |
| `norm:sstatus_mxr` | SVBARE-NOPT-04 |
