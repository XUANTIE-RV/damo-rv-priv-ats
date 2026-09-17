**中文 | [English](../testplan_en/Svpbmt_test_plan_en.md)**

# Svpbmt 扩展测试计划

本文档描述 Svpbmt（Page-Based Memory Types）扩展的测试计划。Svpbmt 扩展在 Sv39/Sv48/Sv57 的叶 PTE 中使用 bits 62-61（PBMT 字段）覆盖页面的物理内存属性（PMA），覆盖 PBMT 编码验证、保留值异常、非叶 PTE 检查、内存排序语义、别名一致性等规范要求。

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/svpbmt.adoc` — Svpbmt 扩展：PBMT 编码表（pbmt）、叶/非叶 PTE PBMT 规则、保留值 page-fault、内存排序与别名一致性、两阶段翻译 PBMT 覆盖规则
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — PTE 格式定义、SFENCE.VMA 定义（PBMT 变更后的 TLB 同步）

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内上述路径文件）

---

## 概述

Svpbmt 扩展在 Sv39、Sv48 和 Sv57 的叶页表条目（leaf PTE）中使用 bits 62-61（PBMT 字段）来覆盖页面的物理内存属性（PMA）。PBMT 字段的编码如下（规范性引用，源自 `svpbmt.adoc` 的 pbmt 编码表）：

| Mode | Value | 请求的内存属性 |
|------|-------|---------------|
| PMA  | 0     | 无覆盖，使用底层 PMA |
| NC   | 1     | 非缓存、幂等、弱排序（RVWMO）、主内存 |
| IO   | 2     | 非缓存、非幂等、强排序（I/O ordering）、I/O |
| —    | 3     | 保留（未来标准用途），在叶 PTE 中使用触发 page-fault |

**依赖关系**：Svpbmt 扩展依赖 Sv39 扩展（`norm:Svpbmt_depends_Sv39`）。

---

## 覆盖的规范点

下表列出本方案覆盖的规范点。带 `norm:` 前缀的为 SPEC 官方 normative rule 标签；不带前缀的为本方案依据 SPEC 原文归纳的规范点。

| Norm ID | 原文（要点） | 中文说明 |
|---------|------|----------|
| `norm:Svpbmt_depends_Sv39` | The Svpbmt extension depends on the Sv39 extension. | Svpbmt 扩展依赖于 Sv39 扩展。 |
| `norm:Svpbmt_impl_may_override_pmas` | Implementations may override additional PMAs not explicitly listed in <<pbmt>>. | 实现可以覆盖 pbmt 表中未明确列出的额外 PMA（如 PBMT=IO 页面的非对齐访问可能触发异常）。 |
| `norm:Svpbmt_nonleaf_pte_pbmt_must_be_zero` | Until their use is defined by a standard extension, they must be cleared by software for forward compatibility, or else a page-fault exception is raised. | 非叶 PTE 的 bits 62-61 在用途被标准扩展定义前必须由软件清零，否则触发页错误异常。 |
| `norm:Svpbmt_leaf_pte_pbmt_reserved_3_fault` | Until this value is defined by a standard extension, using this reserved value in a leaf PTE raises a page-fault exception. | 在叶 PTE 中使用保留值 3 会触发页错误异常。 |
| `norm:Svpbmt_obeys_mem_ordering` | memory accesses to such pages obey the memory ordering rules of the final effective attribute. | 对此类页面的内存访问遵循最终有效属性的内存排序规则。 |
| `norm:Svpbmt_io_pma_nc_pbmt_obey_rvwmo` | If the underlying physical memory attribute for a page is I/O, and the page has PBMT=NC, then accesses to that page obey RVWMO. | 底层 PMA 为 I/O 且 PBMT=NC 时，对该页面的访问遵循 RVWMO。 |
| `norm:Svpbmt_io_pma_nc_pbmt_treated_as_io_and_memory` | accesses to such pages are considered to be both I/O and main memory accesses for the purposes of FENCE, .aq, and .rl. | 出于 FENCE、.aq、.rl 目的，此类页面访问被视为同时是 I/O 和主存访问。 |
| `norm:Svpbmt_memory_pma_io_pbmt_strong_io_ordering` | accesses to that page obey strong channel 0 I/O ordering rules. | 底层 PMA 为主内存且 PBMT=IO 时，对该页面的访问遵循强通道 0 I/O 排序规则。 |
| `norm:Svpbmt_memory_pma_io_pbmt_treated_as_io_and_memory` | accesses to such pages are considered to be both I/O and main memory accesses for the purposes of FENCE, .aq, and .rl. | 出于 FENCE、.aq、.rl 目的，此类页面访问被视为同时是 I/O 和主存访问。 |
| `norm:Svpbmt_aliasing_attribute` | When Svpbmt is used with non-zero PBMT encodings, it is possible for multiple virtual aliases of the same physical page to exist simultaneously with different memory attributes ... the behaviors dictated by the attributes (including coherence) may be violated. | 使用非零 PBMT 编码时，同一物理页的多个虚拟别名可能同时具有不同内存属性，属性所规定的行为（含一致性）可能被违反。 |
| `norm:Svpbmt_noncacheable_aliasing_no_coherence_loss` | Accessing the same location using different attributes that are both non-cacheable (e.g., NC and IO) does not cause loss of coherence. | 使用均为不可缓存的不同属性（如 NC 和 IO）访问同一位置不会导致一致性丢失。 |
| `norm:Svpbmt_noncacheable_aliasing_may_weaken_ordering` | might result in weaker memory ordering than the stricter attribute ordinarily guarantees. | 但可能导致比更严格属性通常保证的更弱内存排序。 |
| `norm:Svpbmt_noncacheable_aliasing_fence_prevents_ordering_loss` | `fence iorw, iorw` instruction between such accesses suffices to prevent loss of memory ordering. | 在此类访问之间执行 `fence iorw, iorw` 足以防止内存排序丢失。 |
| `norm:Svpbmt_cacheable_aliasing_may_cause_coherence_loss` | may cause loss of coherence. | 使用不同可缓存性属性访问同一位置可能导致一致性丢失。 |
| `norm:Svpbmt_cacheable_aliasing_fence_flush_fence_required` | prevents both loss of coherence and loss of memory ordering: `fence iorw, iorw`, followed by `cbo.flush` to an address of that location, followed by a `fence iorw, iorw`. | 防止一致性和内存排序丢失的序列：`fence iorw, iorw` + 对该位置的 `cbo.flush` + `fence iorw, iorw`。 |
| `norm:Svpbmt_hgatp_stage_override_rule` | if `hgatp`.MODE is not equal to zero, non-zero G-stage PTE PBMT bits override the attributes in the PMA to produce an intermediate set of attributes. | `hgatp`.MODE 非零时，G-stage 叶 PTE 的非零 PBMT 位覆盖 PMA 产生中间属性集。 |
| `norm:Svpbmt_vsatp_stage_override_rule` | if `vsatp`.MODE is not equal to zero, non-zero VS-stage PTE PBMT bits override the intermediate attributes to produce the final set of attributes. | `vsatp`.MODE 非零时，VS-stage 叶 PTE 的非零 PBMT 位覆盖中间属性产生最终属性集。 |
| `Svpbmt_leaf_pte_pbmt_overrides_pma` | bits 62-61 of a leaf PTE indicate the use of page-based memory types that override the PMA(s) for the associated memory pages (per <<pbmt>>). | 叶 PTE 的 bits 62-61 按 pbmt 编码表覆盖关联内存页的 PMA；权限（R/W/X）检查独立进行，PBMT 变更后需 SFENCE.VMA 同步。 |

---

## 不在测试范围内

- **Hypervisor 两阶段翻译场景**：G-stage / VS-stage PBMT 叠加覆盖规则由 `Hypervisor_Sv_test_plan.md`（Hypervisor × Svpbmt 交叉测试）覆盖。
- **多核场景下的一致性**：本方案聚焦单 hart 行为，别名一致性用例主要验证 fence/flush 序列的正确执行与基本数据可见性。

---

## 测试分组

### Group 1：PBMT 基本编码验证

**规范依据**：
- `Svpbmt_leaf_pte_pbmt_overrides_pma`：叶 PTE bits 62-61 编码 PBMT 字段，有效值为 0（PMA）、1（NC）、2（IO）
- `norm:Svpbmt_depends_Sv39`：Svpbmt 依赖 Sv39 扩展

**测试职责**：验证 PBMT 三种有效编码（PMA、NC、IO）在叶 PTE 中的基本功能，确认设置这些值的页面可以正常访问。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PBMT-01 | PBMT=PMA load/store | 叶 PTE 设置 PBMT=0（PMA），执行 load/store | 正常访问，使用底层 PMA |
| PBMT-02 | PBMT=NC load/store | 叶 PTE 设置 PBMT=1（NC），对主内存区域执行 load/store | 正常访问，非缓存语义 |
| PBMT-03 | PBMT=IO load/store | 叶 PTE 设置 PBMT=2（IO），对主内存区域执行 load/store | 正常访问，强排序语义 |
| PBMT-04 | PBMT=PMA exec | 叶 PTE 设置 PBMT=0（PMA），执行指令获取 | 正常执行 |
| PBMT-05 | PBMT=NC exec | 叶 PTE 设置 PBMT=1（NC），执行指令获取 | 正常执行（非缓存语义） |
| PBMT-06 | PBMT=IO exec | 叶 PTE 设置 PBMT=2（IO），执行指令获取 | 实现定义（I/O 区域可能不支持指令获取） |

---

### Group 2：PBMT 保留值异常

**规范依据**：
- `norm:Svpbmt_leaf_pte_pbmt_reserved_3_fault`：叶 PTE 中 PBMT=3（bits 62-61 = 11）为保留值，触发 page-fault

**测试职责**：验证叶 PTE 中 PBMT 设置为保留值 3 时，任何类型的访问均触发 page-fault。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| RSVD-01 | PBMT=3 load fault | 叶 PTE 设置 PBMT=3，执行 load | load page-fault |
| RSVD-02 | PBMT=3 store fault | 叶 PTE 设置 PBMT=3，执行 store | store page-fault |
| RSVD-03 | PBMT=3 exec fault | 叶 PTE 设置 PBMT=3，执行指令获取 | instruction page-fault |
| RSVD-04 | PBMT=3 superpage fault | 2MB superpage 叶 PTE 设置 PBMT=3，执行 load | load page-fault |

---

### Group 3：非叶 PTE PBMT 位检查

**规范依据**：
- `norm:Svpbmt_nonleaf_pte_pbmt_must_be_zero`：非叶 PTE 的 bits 62-61 保留，必须清零，否则触发 page-fault

**测试职责**：验证非叶 PTE（中间页表指针）中 PBMT 位非零时触发 page-fault。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| NLPTE-01 | 非叶 PTE PBMT=1 | 中间 PTE（非叶）设置 PBMT=NC（01），通过该 PTE 执行 load | load page-fault |
| NLPTE-02 | 非叶 PTE PBMT=2 | 中间 PTE（非叶）设置 PBMT=IO（10），通过该 PTE 执行 load | load page-fault |
| NLPTE-03 | 非叶 PTE PBMT=3 | 中间 PTE（非叶）设置 PBMT=3（11），通过该 PTE 执行 load | load page-fault |
| NLPTE-04 | 非叶 PTE PBMT=0 正常 | 中间 PTE（非叶）PBMT=0（合规），通过该 PTE 执行 load | 正常访问 |
| NLPTE-05 | 多级非叶 PTE PBMT 检查 | 不同层级的非叶 PTE 分别设置 PBMT 非零 | 每次均触发 page-fault |

---

### Group 4：PBMT 与页面权限交互

**规范依据**：
- `Svpbmt_leaf_pte_pbmt_overrides_pma`：PBMT 仅覆盖内存属性，不影响 PTE 的 R/W/X 权限检查（权限检查按标准页表遍历算法独立进行）
- `norm:Svpbmt_impl_may_override_pmas`：实现可能基于 PBMT 覆盖额外的 PMA 约束（如对 PBMT=IO 页面的非对齐访问可能触发异常）

**测试职责**：验证 PBMT 属性不影响 RWX 权限语义，以及 PBMT 与 U 位、非对齐访问的交互。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PERM-01 | PBMT=NC + R-only | 叶 PTE 设置 PBMT=NC，仅 R 权限，执行 store | store page-fault |
| PERM-02 | PBMT=IO + R-only | 叶 PTE 设置 PBMT=IO，仅 R 权限，执行 store | store page-fault |
| PERM-03 | PBMT=NC + no-exec | 叶 PTE 设置 PBMT=NC，无 X 权限，执行指令获取 | instruction page-fault |
| PERM-04 | PBMT=IO + no-exec | 叶 PTE 设置 PBMT=IO，无 X 权限，执行指令获取 | instruction page-fault |
| PERM-05 | PBMT=NC + U-bit S-mode | 叶 PTE 设置 PBMT=NC 和 U=1，S-mode（SUM=0）访问 | page-fault |
| PERM-06 | PBMT=NC + U-bit SUM=1 | 叶 PTE 设置 PBMT=NC 和 U=1，S-mode（SUM=1）访问 | 正常访问 |
| ALIGN-01 | PBMT=IO 非对齐 load | 对 PBMT=IO 页面执行非对齐 load（如从 addr+1 读取 8 字节） | 实现定义：可能触发 load access-fault 或正常访问 |
| ALIGN-02 | PBMT=IO 非对齐 store | 对 PBMT=IO 页面执行非对齐 store（如向 addr+1 写入 8 字节） | 实现定义：可能触发 store access-fault 或正常访问 |
| ALIGN-03 | PBMT=PMA 非对齐 load（对照） | 对 PBMT=PMA 页面执行相同的非对齐 load | 正常访问（对照基线） |

---

### Group 5：PBMT 与不同页大小

**规范依据**：
- `Svpbmt_leaf_pte_pbmt_overrides_pma`：PBMT 定义在叶 PTE 的 bits 62-61，适用于所有叶 PTE 级别
- `norm:Svpbmt_depends_Sv39`：Svpbmt 依赖 Sv39 扩展

**测试职责**：验证 PBMT 属性在 4KB 页、2MB megapage 和 1GB gigapage 上均正常工作。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PGSZ-01 | 4KB 页 + PBMT=NC | 4KB 叶 PTE 设置 PBMT=NC，load/store | 正常访问 |
| PGSZ-02 | 4KB 页 + PBMT=IO | 4KB 叶 PTE 设置 PBMT=IO，load/store | 正常访问 |
| PGSZ-03 | 2MB megapage + PBMT=NC | 2MB superpage 叶 PTE 设置 PBMT=NC，load/store | 正常访问 |
| PGSZ-04 | 2MB megapage + PBMT=IO | 2MB superpage 叶 PTE 设置 PBMT=IO，load/store | 正常访问 |
| PGSZ-05 | 1GB gigapage + PBMT=NC | 1GB superpage 叶 PTE 设置 PBMT=NC，load/store | 正常访问 |
| PGSZ-06 | 1GB gigapage + PBMT=IO | 1GB superpage 叶 PTE 设置 PBMT=IO，load/store | 正常访问 |

---

### Group 6：内存排序语义

**规范依据**：
- `norm:Svpbmt_obeys_mem_ordering`：当 PBMT 覆盖内存属性时，访问遵循最终有效属性的排序规则
- `norm:Svpbmt_io_pma_nc_pbmt_obey_rvwmo`：I/O PMA + PBMT=NC 时遵循 RVWMO
- `norm:Svpbmt_io_pma_nc_pbmt_treated_as_io_and_memory`：I/O PMA + PBMT=NC 时被视为同时是 I/O 和主内存访问
- `norm:Svpbmt_memory_pma_io_pbmt_strong_io_ordering`：主内存 PMA + PBMT=IO 时遵循强 I/O 排序
- `norm:Svpbmt_memory_pma_io_pbmt_treated_as_io_and_memory`：主内存 PMA + PBMT=IO 时被视为同时是 I/O 和主内存访问

**测试职责**：验证 PBMT 覆盖内存属性后的排序语义。

> [!NOTE]
> 内存排序测试在单 hart 环境下难以完全验证排序强度差异。以下测试主要验证 PBMT 覆盖后的访问不会导致异常或不正确的数据，以及 FENCE 指令对 PBMT 覆盖页面的作用。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ORDER-01 | 主内存 + PBMT=IO 访问正确 | 主内存区域设置 PBMT=IO，执行多次顺序 load/store | 数据正确（强 I/O 排序） |
| ORDER-02 | 主内存 + PBMT=IO + FENCE | 主内存区域设置 PBMT=IO，在 load/store 之间插入 `fence iorw, iorw` | 数据正确 |
| ORDER-03 | 主内存 + PBMT=NC 访问正确 | 主内存区域设置 PBMT=NC，执行多次顺序 load/store | 数据正确（RVWMO） |
| ORDER-04 | 主内存 + PBMT=NC + FENCE | 主内存区域设置 PBMT=NC，在 load/store 之间插入 `fence rw, rw` | 数据正确 |
| ORDER-05 | I/O 区域 + PBMT=NC 访问（discouraged） | I/O 区域（如 UART）设置 PBMT=NC，验证 RVWMO 排序生效。规范不鼓励此配置：I/O 设备驱动依赖强排序规则时将无法正确工作 | 正常访问，但排序弱于 PBMT=IO |
| ORDER-06 | PBMT=IO 页 FENCE 覆盖性 | 主内存 + PBMT=IO 页面上执行 `fence iorw, iorw`，验证该页面访问对 FENCE 有效 | FENCE 对 I/O 和主内存访问均有效 |

---

### Group 7：别名与一致性

**规范依据**：
- `norm:Svpbmt_aliasing_attribute`：不同虚拟别名可以有不同的内存属性，此时属性行为（包括一致性）可能被违反
- `norm:Svpbmt_noncacheable_aliasing_no_coherence_loss`：两个非缓存属性的别名不会导致一致性丧失
- `norm:Svpbmt_noncacheable_aliasing_may_weaken_ordering`：两个非缓存属性的别名可能削弱排序保证
- `norm:Svpbmt_noncacheable_aliasing_fence_prevents_ordering_loss`：`fence iorw, iorw` 可防止非缓存别名间的排序丧失
- `norm:Svpbmt_cacheable_aliasing_may_cause_coherence_loss`：不同缓存属性的别名可能导致一致性丧失
- `norm:Svpbmt_cacheable_aliasing_fence_flush_fence_required`：`fence iorw, iorw` + `cbo.flush` + `fence iorw, iorw` 序列可防止缓存别名的一致性和排序丧失

**测试职责**：验证不同 PBMT 属性的虚拟别名行为，以及规范要求的 fence/flush 序列的有效性。

> [!NOTE]
> 别名测试需要将同一物理页面映射到两个不同虚拟地址，分别设置不同的 PBMT 属性。在单 hart 测试环境下，一致性问题可能不会暴露，以下测试主要验证 fence/flush 序列的正确执行（不触发异常）以及基本数据可见性。ALIAS-04~06 使用来自 Zicbom 扩展的 `cbo.flush` 指令；若目标平台未实现 Zicbom，这些用例应跳过。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ALIAS-01 | NC + IO 非缓存别名 | 同一物理页映射到 VA1（PBMT=NC）和 VA2（PBMT=IO），通过 VA1 写入后通过 VA2 读取 | 数据一致（非缓存别名不丢失一致性） |
| ALIAS-02 | NC + IO 别名 + fence | 同一物理页 NC/IO 别名，在写入和读取之间插入 `fence iorw, iorw` | 数据一致，排序保证 |
| ALIAS-03 | PMA + NC 缓存属性别名 | 同一物理页映射到 VA1（PBMT=PMA）和 VA2（PBMT=NC），通过 VA1 写入后通过 VA2 读取 | 行为不确定（可能一致性丧失） |
| ALIAS-04 | PMA + NC 别名 + fence+flush+fence | 同上，但在写入和读取之间插入 `fence iorw,iorw` + `cbo.flush` + `fence iorw,iorw` | 数据一致 |
| ALIAS-05 | PMA + IO 缓存属性别名 | 同一物理页映射到 VA1（PBMT=PMA）和 VA2（PBMT=IO），通过 VA1 写入后通过 VA2 读取 | 行为不确定（可能一致性丧失） |
| ALIAS-06 | PMA + IO 别名 + fence+flush+fence | 同上，但在写入和读取之间插入完整的 fence+flush+fence 序列 | 数据一致 |
| ALIAS-07 | M-mode PMA vs S-mode PBMT=NC | M-mode 以 PMA 属性直接写入物理页，S-mode 以 PBMT=NC 映射同一页并读取（需手动管理特权级切换） | 行为不确定（规范允许属性行为违反，包括一致性） |

---

### Group 8：SFENCE.VMA 与 PBMT

**规范依据**：
- `Svpbmt_leaf_pte_pbmt_overrides_pma`：修改 PTE 的 PBMT 字段后，必须执行 SFENCE.VMA 刷新 TLB 缓存的翻译条目，否则 TLB 可能仍缓存旧的 PBMT 属性

**测试职责**：验证 PBMT 属性变更后 SFENCE.VMA 的刷新效果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SFENCE-01 | PBMT 变更后全局刷新 | 将 PBMT 从 PMA 改为 NC，执行全局 sfence.vma，验证新属性生效 | 新属性生效，正常访问 |
| SFENCE-02 | PBMT 变更后按地址刷新 | 将 PBMT 从 PMA 改为 IO，按地址执行 sfence.vma，验证新属性生效 | 新属性生效，正常访问 |
| SFENCE-03 | PBMT 改为保留值后刷新 | 将 PBMT 从 NC 改为 3（保留），执行 sfence.vma 后访问。验证 TLB 刷新后正确反映 PTE 的非法状态（保留编码） | page-fault |
| SFENCE-04 | PBMT 从保留值恢复后刷新 | 将 PBMT 从 3（保留）改回 PMA，执行 sfence.vma 后访问。验证 TLB 刷新后正确反映 PTE 从非法状态恢复为合法状态 | 正常访问 |

---

### Group 9：多模式兼容性

**规范依据**：
- `norm:Svpbmt_depends_Sv39`：Svpbmt 依赖 Sv39 扩展
- `Svpbmt_leaf_pte_pbmt_overrides_pma`：PBMT 字段（bits 62-61）在 Sv39、Sv48、Sv57 PTE 格式中定义相同

**测试职责**：验证 PBMT 属性在 Sv39、Sv48 和 Sv57 模式下均正常工作。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MODE-01 | Sv39 + PBMT=NC | Sv39 模式下叶 PTE 设置 PBMT=NC，load/store | 正常访问 |
| MODE-02 | Sv48 + PBMT=NC | Sv48 模式下叶 PTE 设置 PBMT=NC，load/store | 正常访问 |
| MODE-03 | Sv57 + PBMT=NC | Sv57 模式下叶 PTE 设置 PBMT=NC，load/store | 正常访问 |

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1（基本编码）、Group 2（保留值异常）、Group 3（非叶 PTE 检查） | PBMT-01~06、RSVD-01~04、NLPTE-01~05 | 核心功能：PBMT 编码识别、保留值 fault、非叶 PTE 合规性 |
| P1（重要） | Group 4（权限交互+非对齐）、Group 5（页大小）、Group 6 基础排序（ORDER-01~04）、Group 8（SFENCE.VMA） | PERM-01~06、ALIGN-01~03、PGSZ-01~06、ORDER-01~04、SFENCE-01~04 | PBMT 与权限/页大小的正交性、非对齐访问约束、基础排序语义、TLB 刷新正确性 |
| P2（建议） | Group 6 高级排序（ORDER-05~06）、Group 7（别名一致性 ALIAS-01~06）、Group 9（多模式兼容） | ORDER-05~06、ALIAS-01~06、MODE-01~03 | 高级排序与 discouraged 配置、缓存一致性、跨模式验证 |
| P3（可选） | ALIAS-07（跨特权级别名） | ALIAS-07 | 依赖手动特权级管理，条件性实现 |

---

## 结果判定原则

- 平台实现 Svpbmt 但行为偏离 SPEC（如保留值 3 不触发 page-fault、非叶 PTE PBMT 非零不触发 page-fault、PBMT 变更后 TLB 未刷新等）：保持用例失败，与 SPEC 比对后将问题记录至 `bugs/` 目录，禁止修改用例或加 workaround 适配错误实现。
- "实现定义/行为不确定"类用例（PBMT-06、ALIGN-01~02、ALIAS-03/05/07）：记录实际观测行为，不做强断言。

---

## 参考

- `svpbmt.adoc` — Svpbmt 扩展定义
- `supervisor.adoc` — PTE 格式、SFENCE.VMA 定义
- `Svnapot_test_plan.md` — Svnapot 测试计划（NAPOT × PBMT 交互参考）
- `Hypervisor_Sv_test_plan.md` — Hypervisor × Svpbmt 交叉测试计划（两阶段 PBMT 覆盖）

---

## 附录 A：规范点覆盖矩阵

下表标明"覆盖的规范点"章节中每条规范点被哪些测试用例覆盖。两阶段翻译相关规范点由 `Hypervisor_Sv_test_plan.md` 覆盖。

| Norm ID | 覆盖的测试 ID |
|---------|---------------|
| `norm:Svpbmt_depends_Sv39` | MODE-01~03、PBMT-01~06 |
| `norm:Svpbmt_impl_may_override_pmas` | ALIGN-01~03 |
| `norm:Svpbmt_nonleaf_pte_pbmt_must_be_zero` | NLPTE-01~05 |
| `norm:Svpbmt_leaf_pte_pbmt_reserved_3_fault` | RSVD-01~04、SFENCE-03、SFENCE-04 |
| `norm:Svpbmt_obeys_mem_ordering` | ORDER-01~06 |
| `norm:Svpbmt_io_pma_nc_pbmt_obey_rvwmo` | ORDER-05 |
| `norm:Svpbmt_io_pma_nc_pbmt_treated_as_io_and_memory` | ORDER-05、ORDER-06 |
| `norm:Svpbmt_memory_pma_io_pbmt_strong_io_ordering` | ORDER-01、ORDER-02 |
| `norm:Svpbmt_memory_pma_io_pbmt_treated_as_io_and_memory` | ORDER-02、ORDER-06 |
| `norm:Svpbmt_aliasing_attribute` | ALIAS-01~07 |
| `norm:Svpbmt_noncacheable_aliasing_no_coherence_loss` | ALIAS-01 |
| `norm:Svpbmt_noncacheable_aliasing_may_weaken_ordering` | ALIAS-01、ALIAS-02 |
| `norm:Svpbmt_noncacheable_aliasing_fence_prevents_ordering_loss` | ALIAS-02 |
| `norm:Svpbmt_cacheable_aliasing_may_cause_coherence_loss` | ALIAS-03、ALIAS-05 |
| `norm:Svpbmt_cacheable_aliasing_fence_flush_fence_required` | ALIAS-04、ALIAS-06 |
| `norm:Svpbmt_hgatp_stage_override_rule` | 由 `Hypervisor_Sv_test_plan.md`（HCROSS-SVPBMT-01、HCROSS-SVPBMT-03、HCROSS-SVPBMT-04）覆盖 |
| `norm:Svpbmt_vsatp_stage_override_rule` | 由 `Hypervisor_Sv_test_plan.md`（HCROSS-SVPBMT-02、HCROSS-SVPBMT-03、HCROSS-SVPBMT-04）覆盖 |
| `Svpbmt_leaf_pte_pbmt_overrides_pma` | PBMT-01~06、PERM-01~06、PGSZ-01~06、SFENCE-01~04、MODE-01~03 |
