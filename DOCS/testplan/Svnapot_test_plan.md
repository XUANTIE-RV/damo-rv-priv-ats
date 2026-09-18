**中文 | [English](../testplan_en/Svnapot_test_plan_en.md)**

# Svnapot 扩展测试计划

本文档描述 Svnapot（NAPOT Translation Contiguity）扩展的测试计划。Svnapot 扩展允许一个 PTE 表示一段连续虚拟到物理翻译范围（NAPOT，自然对齐的 2 的幂次方粒度），Version 1.0 标准化 64 KiB 连续区域支持。

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/svnapot.adoc` — Svnapot 扩展：PTE.N 位、NAPOT 编码表（ptenapot）、PPN 替换语义、保留编码 page-fault、地址翻译缓存条目、Sv39 依赖、G-stage 支持
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — SFENCE.VMA 定义、satp CSR、PTE 格式（用于 NAPOT 与翻译算法的交互）

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内上述路径文件）

---

## 覆盖的规范点

下表列出本方案覆盖的规范点。带 `norm:` 前缀的为 SPEC 官方 normative rule 标签；不带前缀的为本方案依据 SPEC 原文（含 NOTE）归纳的规范点。

| Norm ID | 原文（要点） | 中文说明 |
|---------|------|----------|
| `norm:Svnapot_pte_N` | N=1 ... the PTE represents a translation that is part of a range of contiguous virtual-to-physical translations with the same values for PTE bits 5-0. | 当 PTE 的 N=1 时，该 PTE 表示一段连续虚拟到物理翻译范围的一部分，范围内 PTE bits 5-0 值相同。 |
| `norm:Svnapot_range_napot` | Such ranges must be of a naturally aligned power-of-2 (NAPOT) granularity larger than the base page size. | 此类范围必须是大于基本页面大小的自然对齐的 2 的幂次方（NAPOT）粒度。 |
| `norm:Svnapot_depends_Sv39` | The Svnapot extension depends on the Sv39 extension. | Svnapot 扩展依赖于 Sv39 扩展。 |
| `norm:Svnapot_valid_encoding` | valid according to <<ptenapot>> | 如果 PTE 编码根据 ptenapot 表有效。 |
| `norm:Svnapot_implicit_read_ppn_subst` | implicit reads of a NAPOT PTE return a copy of pte in which pte.ppn[i][pte.napot_bits-1:0] is replaced by vpn[i][pte.napot_bits-1:0]. | NAPOT PTE 的隐式读取返回 PTE 的副本，其中 ppn[i] 低 napot_bits 位被替换为 vpn[i] 对应位。 |
| `norm:Svnapot_reserved_encoding_fault` | reserved according to <<ptenapot>>, then a page-fault exception must be raised. | 如果 PTE 编码根据 ptenapot 表是保留的，则必须触发页错误异常。 |
| `norm:Svnapot_cache_entries` | Implicit reads of NAPOT page table entries may create address-translation cache entries mapping a + j×PTESIZE to a copy of pte ... for any or all j such that j >> napot_bits = vpn[i] >> napot_bits. | NAPOT 页表条目的隐式读取可创建地址翻译缓存条目，将范围内地址映射到 PPN 低位被 VPN 对应位替换后的 PTE 副本。 |
| `norm:Svnapot_hyp_gstage` | If the hypervisor extension is also implemented, Svnapot is also supported in G-stage translation. | 如果同时实现了 Hypervisor 扩展，Svnapot 在 G-stage 翻译中也受支持。 |
| `Svnapot_ad_alias_independence` | The D and A bits may not be identical across all mappings of the same address range; the OS must query all NAPOT aliases of a page. | 同一地址范围的多个 NAPOT 别名间 D/A 位可能不一致；OS 必须查询所有别名以确定访问/脏状态。 |
| `Svnapot_rsw_reserved` | RSW remains reserved for supervisor software control. | RSW（PTE bits 9-8）保留供 supervisor 软件控制，硬件忽略，不影响翻译。 |
| `Svnapot_pbmt_interaction` | bits 62-61 of a leaf PTE (PBMT, defined by Svpbmt) also apply to NAPOT leaf PTEs when Svpbmt is implemented. | 若实现 Svpbmt，NAPOT 叶 PTE 的 bits 62-61（PBMT）同样适用；未实现时应为 0。 |

---

## 测试目标

验证 RISC-V 处理器的 Svnapot 扩展实现是否符合规范，重点覆盖：

1. PTE N 位的基本功能：N=1 时启用 NAPOT 翻译连续性
2. 64 KiB NAPOT 页面的有效编码和地址翻译正确性
3. 保留编码（Reserved encoding）触发 page-fault 异常
4. NAPOT PTE 的 PPN 替换语义（ppn[i] 低位由 vpn[i] 替换）
5. NAPOT 页面的 RWX 权限控制
6. NAPOT 页面与 SFENCE.VMA 的交互
7. Svnapot 对 Sv39 扩展的依赖关系与多模式（Sv39/Sv48/Sv57）一致性
8. NAPOT 页面的 A/D 位行为及 NAPOT 别名 A/D 位独立性
9. PTE bits 5-0 一致性与 RSW 位保留语义
10. NAPOT 区域边界对齐验证与 PBMT 交互（条件性）

---

## 不在测试范围内

- **Hypervisor G-stage NAPOT**：G-stage 翻译中的 NAPOT PTE 支持、保留编码 guest-page-fault、两阶段同时使用 NAPOT 由 `Hypervisor_Sv_test_plan.md`（Hypervisor × Svnapot 交叉测试）覆盖。
- **多核场景**：本方案聚焦单 hart 行为。

---

## 测试分组

### Group 1：64 KiB NAPOT 页面基本映射

**规范依据**：
- `norm:Svnapot_pte_N`：N=1 时，PTE 表示一个连续虚拟到物理翻译范围的一部分
- `norm:Svnapot_range_napot`：连续范围必须是自然对齐的 2 的幂次方粒度，且大于基本页大小
- `norm:Svnapot_valid_encoding`：当 i=0 且 pte.ppn[0] 编码为 `x xxxx 1000` 时，表示 64 KiB 连续区域，napot_bits=4
- `norm:Svnapot_implicit_read_ppn_subst`：隐式读取 NAPOT PTE 时，pte.ppn[i][napot_bits-1:0] 被替换为 vpn[i][napot_bits-1:0]

**测试职责**：验证 64 KiB NAPOT 页面的基本地址翻译功能。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| NAPOT64-01 | 64 KiB NAPOT 页面读访问 | 配置 N=1、ppn[0]=`xxxx1000` 的 NAPOT PTE，映射 64 KiB 连续区域，S-mode 读取区域首地址 | 读成功，地址翻译正确 |
| NAPOT64-02 | 64 KiB NAPOT 页面写访问 | 同上配置，S-mode 写入区域首地址 | 写成功 |
| NAPOT64-03 | 64 KiB NAPOT 页面末地址访问 | S-mode 读取 64 KiB 区域最后一个字节 | 读成功 |
| NAPOT64-04 | 64 KiB NAPOT 区域内多偏移访问 | 分别访问 64 KiB 区域内偏移 0x0000、0x1000、0x8000、0xF000 处 | 所有访问成功，翻译到对应物理地址 |
| NAPOT64-05 | 64 KiB NAPOT 区域外访问 | 访问 NAPOT 区域外第一个字节（base + 64 KiB） | page-fault（无映射） |
| NAPOT64-06 | 多个 NAPOT 64 KiB 区域 | 配置两个相邻的 64 KiB NAPOT 区域，分别访问 | 两个区域均正确翻译 |

---

### Group 2：PPN 替换语义验证

**规范依据**：
- `norm:Svnapot_implicit_read_ppn_subst`：隐式读取 NAPOT PTE 返回一个副本，其中 pte.ppn[i][napot_bits-1:0] 被替换为 vpn[i][napot_bits-1:0]

**测试职责**：验证 NAPOT PTE 的物理地址翻译正确地将 ppn 低位替换为 vpn 低位。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PPN-01 | PPN 替换基本验证 | 64 KiB NAPOT 区域内，写入不同 4 KiB 页面的 magic value，读回验证物理地址正确 | 每个 4 KiB 页面写入独立的物理位置 |
| PPN-02 | 区域内偏移 0x0 翻译验证 | 访问 NAPOT 区域 VA 偏移 0x0，验证翻译到 PA 偏移 0x0 | PA = base_pa + 0x0 |
| PPN-03 | 区域内偏移 0x4000 翻译验证 | 访问 NAPOT 区域 VA 偏移 0x4000，验证翻译到 PA 偏移 0x4000 | PA = base_pa + 0x4000 |
| PPN-04 | 区域内偏移 0xF000 翻译验证 | 访问 NAPOT 区域 VA 偏移 0xF000，验证翻译到 PA 偏移 0xF000 | PA = base_pa + 0xF000 |
| PPN-05 | 非恒等映射 PPN 替换 | VA 和 PA 不同的 64 KiB NAPOT 映射，验证区域内各偏移翻译到正确 PA | 各偏移的 PA 地址正确 |

---

### Group 3：保留编码异常

**规范依据**：
- `norm:Svnapot_reserved_encoding_fault`：当 PTE 的编码在 ptenapot 表中标记为 Reserved 时，必须触发 page-fault 异常

根据规范中 ptenapot 表（i=0 级别），以下 ppn[0] 编码在 N=1 时为保留：`x xxxx xxx1`、`x xxxx xx1x`、`x xxxx x1xx`、`x xxxx 0xxx`（仅 `x xxxx 1000` 为 64 KiB 有效编码）；i≥1 级别的所有 ppn 编码在 N=1 时均为保留。

**测试职责**：验证所有保留编码触发 page-fault。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| RSVD-01 | ppn[0] = xxx1（bit 0 = 1） | N=1，ppn[0] 低 4 位为 0001 | page-fault |
| RSVD-02 | ppn[0] = xx1x（bit 1 = 1） | N=1，ppn[0] 低 4 位为 0010 | page-fault |
| RSVD-03 | ppn[0] = x1xx（bit 2 = 1） | N=1，ppn[0] 低 4 位为 0100 | page-fault |
| RSVD-04 | ppn[0] = 0000（bit 3 = 0） | N=1，ppn[0] 低 4 位为 0000（Reserved: `x xxxx 0xxx`） | page-fault |
| RSVD-05 | ppn[0] = 0101（多保留位） | N=1，ppn[0] 低 4 位为 0101 | page-fault |
| RSVD-06 | ppn[0] = 0111（多保留位） | N=1，ppn[0] 低 4 位为 0111 | page-fault |
| RSVD-07 | ppn[0] = 1001（bit 0 和 bit 3 同时置位） | N=1，ppn[0] 低 4 位为 1001 | page-fault |
| RSVD-08 | i≥1 级 N=1（Reserved，level 1） | i≥1 级别的 NAPOT PTE（任意 ppn 编码均保留），在 level 1（2 MB superpage）设置 N=1 | page-fault |
| RSVD-09 | i≥1 级 N=1（Reserved，level 2） | 在 level 2（1 GB gigapage 级别）设置 N=1，所有 ppn 编码均保留 | page-fault |

---

### Group 4：NAPOT PTE 权限控制

**规范依据**：
- `norm:Svnapot_pte_N`：N=1 时，PTE 表示连续翻译范围，PTE bits 5-0（包含 V、R、W、X、U、G）在范围内所有翻译中相同
- NAPOT PTE 在地址翻译算法中的行为与非 NAPOT PTE 相同（除 PPN 替换和编码检查外）

**测试职责**：验证 NAPOT 页面的 R、W、X、U 权限位正确生效。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PERM-01 | NAPOT 只读页面（R only） | N=1 NAPOT 页面仅设置 R 权限，S-mode 写入 | store page-fault |
| PERM-02 | NAPOT 读写页面（RW） | N=1 NAPOT 页面设置 RW 权限，S-mode 读写 | 读写成功 |
| PERM-03 | NAPOT 可执行页面（RX） | N=1 NAPOT 页面设置 RX 权限，S-mode 跳转执行 | 执行成功 |
| PERM-04 | NAPOT 不可执行页面写执行 | N=1 NAPOT 页面仅设置 RW（无 X），S-mode 跳转执行 | instruction page-fault |
| PERM-05 | NAPOT U 位访问控制（S-mode） | N=1 NAPOT 页面设置 U=1，S-mode（sstatus.SUM=0）访问 | page-fault |
| PERM-06 | NAPOT U 位访问控制（SUM=1） | N=1 NAPOT 页面设置 U=1，S-mode（sstatus.SUM=1）读写 | 读写成功 |
| PERM-07 | NAPOT 权限在区域内一致 | 64 KiB NAPOT 区域内不同偏移处验证权限一致 | 所有偏移权限行为一致 |

---

### Group 5：N=0 行为验证

**规范依据**：
- `norm:Svnapot_pte_N`：N=1 时启用 NAPOT 语义；当 N=0 时，PTE 行为与标准非 NAPOT PTE 完全相同（即标准 4 KiB 页面翻译）

**测试职责**：验证 N=0 时 PTE 行为与普通 PTE 完全一致。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| NZERO-01 | N=0 标准 4 KiB 页面 | 设置 N=0 的普通 4 KiB PTE，S-mode 读写 | 正常 4 KiB 页面翻译 |
| NZERO-02 | N=0 ppn[0]=1000 无 NAPOT 效果 | N=0 但 ppn[0] 低位编码恰好是 `1000`，验证不触发 NAPOT 语义 | 标准 4 KiB 翻译（ppn 不替换） |
| NZERO-03 | N=0 时 ppn[0] 低位不参与 NAPOT 编码检查 | N=0 时 ppn[0] 低位为 `0001` 是标准 ppn 编码，不触发任何 NAPOT 相关检查 | 正常访问（N=0 时无 NAPOT 编码检查） |

---

### Group 6：SFENCE.VMA 与 NAPOT 页面交互

**规范依据**：
- `norm:Svnapot_cache_entries`：NAPOT PTE 的隐式读取可能创建地址翻译缓存条目，映射 a + j×PTESIZE 到替换后的 pte 副本
- 规范 NOTE：更新 NAPOT PTE 时，OS 一般应先使所有 PTE 无效，然后对范围内所有 4 KiB 区域执行 SFENCE.VMA（单条 rs1=x0 全局刷新，或多条 rs1≠x0 逐页刷新），再更新 PTE

**测试职责**：验证 SFENCE.VMA 对 NAPOT 区域 TLB 缓存的刷新效果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SFENCE-01 | 全局 SFENCE.VMA 刷新 NAPOT 缓存 | 修改 NAPOT PTE 权限后执行 sfence.vma(0,0)，验证新权限生效 | 新权限生效 |
| SFENCE-02a | 单条全局 SFENCE.VMA 刷新 NAPOT 区域 | 修改 NAPOT PTE 后，执行单条 sfence.vma rs1=x0（全局刷新），验证 64 KiB 区域内新权限生效 | 新权限生效 |
| SFENCE-02b | 多条按地址 SFENCE.VMA 逐页刷新 NAPOT 区域 | 修改 NAPOT PTE 后，对区域内每个 4 KiB 页分别执行 sfence.vma rs1≠x0（共 16 条），验证新权限生效 | 新权限生效 |
| SFENCE-03 | NAPOT 映射移除后刷新 | 将 NAPOT PTE 设为无效（V=0），执行 sfence.vma 后访问 | page-fault |
| SFENCE-04 | NAPOT 权限升级后刷新 | 将 NAPOT R-only 升级为 RW，执行 sfence.vma 后写入 | 写入成功 |
| SFENCE-05 | 单地址 SFENCE.VMA 仅刷新对应 4 KiB | 对 64 KiB NAPOT 区域内单个 4 KiB 地址刷新，验证该地址新权限生效 | 至少该 4 KiB 地址新权限生效 |

---

### Group 7：Sv 模式兼容性

**规范依据**：
- `norm:Svnapot_depends_Sv39`：Svnapot 扩展依赖 Sv39 扩展
- 规范描述：Svnapot 在 Sv39、Sv48 和 Sv57 中均适用

**测试职责**：验证 NAPOT 页面在不同 Sv 模式下的行为一致性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MODE-01 | Sv39 下 64 KiB NAPOT | Sv39 模式下配置 64 KiB NAPOT 区域并访问 | 翻译正确 |
| MODE-02 | Sv48 下 64 KiB NAPOT | Sv48 模式下配置 64 KiB NAPOT 区域并访问 | 翻译正确 |
| MODE-03 | Sv57 下 64 KiB NAPOT | Sv57 模式下配置 64 KiB NAPOT 区域并访问 | 翻译正确 |
| MODE-04 | Sv39→Sv48 切换 NAPOT 保持 | 从 Sv39 切换到 Sv48，重建含 NAPOT 映射的页表后验证 | 切换后 NAPOT 翻译正确 |
| MODE-05 | Sv39 依赖验证 | 验证 Svnapot 实现必须支持 Sv39 | Sv39 模式可用 |

---

### Group 8：A/D 位与 NAPOT 页面

**规范依据**：
- `Svnapot_ad_alias_independence`：实现可能不直接查询算法指定的 PTE，因此 NAPOT 区域内不同映射的 D 和 A 位可能不一致；OS 必须查询所有 NAPOT 别名以确定页面是否被访问或脏

**测试职责**：验证 NAPOT 页面 A/D 位的基本行为及别名独立性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| AD-01 | NAPOT A=0 触发 page-fault（Svade） | 64 KiB NAPOT PTE A=0，S-mode 读取 | load page-fault（Svade 语义下） |
| AD-02 | NAPOT D=0 写触发 page-fault（Svade） | 64 KiB NAPOT PTE D=0（A=1），S-mode 写入 | store page-fault（Svade 语义下） |
| AD-03 | NAPOT A=1,D=1 正常访问 | 64 KiB NAPOT PTE A=1,D=1，S-mode 读写 | 正常访问 |
| AD-04 | NAPOT 区域内 A/D 位一致性 | 设置 NAPOT PTE A=1,D=1，访问区域内多个 4 KiB 偏移 | 所有偏移正常访问 |
| AD-05 | NAPOT 别名 A 位独立性 | 配置两个 NAPOT PTE 别名覆盖同一 64 KiB 区域，一个 A=1，另一个 A=0，访问区域 | 实现可能使用任一别名——访问行为取决于实现选择 |
| AD-06 | NAPOT 别名 D 位独立性 | 类似 AD-05，但测试 D 位：一个 D=1，另一个 D=0，写入区域 | 实现可能使用任一别名——写入行为取决于实现选择 |
| AD-07 | OS 手动设置 A 位避免 trap | 手动设置一个 NAPOT 别名的 A=1，但其他别名 A=0，访问后验证 trap 行为 | 若实现选择 A=0 的别名则 trap，若选择 A=1 的别名则不 trap |

---

### Group 9：NAPOT PTE 与 TLB 缓存行为

**规范依据**：
- `norm:Svnapot_cache_entries`：NAPOT PTE 的隐式读取可创建地址翻译缓存条目，映射范围内所有满足 j >> napot_bits = vpn[i] >> napot_bits 的 j
- 规范 NOTE：TLB 允许缓存 V=0 的 NAPOT PTE；64 KiB NAPOT PTE 可能触发创建 16 个标准 4 KiB TLB 条目

**测试职责**：验证 NAPOT PTE 的 TLB 缓存行为和多别名访问。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TLB-01 | 单次 PTE walk 后全区域可访问 | 访问 64 KiB NAPOT 区域内首地址后，验证其他偏移也可访问 | 所有 16 个 4 KiB 页面均可访问 |
| TLB-02 | NAPOT 区域替换为普通 PTE 后刷新 | 将 NAPOT PTE 替换为普通 4 KiB PTE，刷新 TLB 后验证 | 仅替换的 4 KiB 页面可访问 |
| TLB-03 | NAPOT V=0 缓存允许 | 设置 NAPOT PTE V=0，访问应触发 page-fault | page-fault |
| TLB-04 | V=0 NAPOT PTE TLB 缓存一致性 | 设置 NAPOT PTE V=0 并访问触发 fault 后，修改 PTE 为 V=1 但不执行 SFENCE.VMA 直接访问 | 实现可能使用缓存的 V=0 翻译（仍 fault）或重新查询页表（成功）——验证不出现不可预测行为 |

---

### Group 10：PTE bits 5-0 一致性与 NAPOT 别名

**规范依据**：
- `norm:Svnapot_pte_N`：NAPOT PTE "represents a translation that is part of a range of contiguous virtual-to-physical translations with the same values for PTE bits 5-0"
- 规范 NOTE：同一 NAPOT 区域内的所有 NAPOT PTE 应具有相同属性、相同 PPN 和相同 bits 5-0 值；如果存在不一致，效果与 SFENCE.VMA 使用不正确时相同——将选择其中一个翻译，但选择不可预测

**测试职责**：验证同一 NAPOT 区域内多个别名 PTE 的 bits 5-0 一致性行为，以及 G 位的全局翻译语义。

> [!NOTE]
> BITS50-01 和 BITS50-02 属于"OS 责任"类测试。规范指出 "if any inconsistencies do exist, one of the translations will be chosen, but the choice is unpredictable"，此类测试主要验证实现不会产生不可恢复的错误。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| BITS50-01 | NAPOT 别名 R 位不一致 | 在 64 KiB 区域配置两个 NAPOT PTE 别名，一个 R=1，另一个 R=0 | 实现行为确定（选择其中一个翻译，不可预测但不崩溃） |
| BITS50-02 | NAPOT 别名 V 位不一致 | 一个 NAPOT PTE V=1，同区域另一个别名 V=0 | 实现行为确定（不可恢复错误不应发生） |
| BITS50-03 | NAPOT G 位基本功能 | 64 KiB NAPOT PTE 设置 G=1，验证 global 翻译语义 | 翻译不受 ASID 影响（按 ASID 刷新的 sfence.vma 不影响 G=1 条目） |

---

### Group 11：RSW 位验证

**规范依据**：
- `Svnapot_rsw_reserved`：RSW（PTE bits 9-8）保留供 supervisor 软件控制，硬件应忽略此字段，不影响地址翻译行为

**测试职责**：验证 NAPOT PTE 的 RSW 位可被自由写入且不影响翻译。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| RSW-01 | NAPOT PTE RSW 位可写入 | 在 NAPOT PTE 的 RSW 位写入非零值（01、10、11），验证不影响地址翻译 | 地址翻译正常，不触发 fault |
| RSW-02 | NAPOT PTE RSW 位读取回 | 写入 RSW 位后，通过页表 walk 读取 PTE，验证 RSW 值保持 | RSW 值与写入值一致 |

---

### Group 12：NAPOT 区域边界对齐验证

**规范依据**：
- `norm:Svnapot_range_napot`：连续范围必须是自然对齐的 2 的幂次方粒度，且大于基本页大小
- `norm:Svnapot_implicit_read_ppn_subst`：ppn[i][napot_bits-1:0] 被替换为 vpn[i][napot_bits-1:0]

**测试职责**：验证非 64 KiB 对齐场景下 NAPOT PTE 的翻译行为。

> [!NOTE]
> 由于 NAPOT 的 PPN 替换语义是 ppn[0][3:0] 被替换为 vpn[0][3:0]，如果 VA 不是 64 KiB 对齐，翻译结果中 PA 的低位会被 VA 的低位覆盖。这不是 fault 条件，但翻译结果需要验证。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ALIGN-01 | VA 非 64 KiB 对齐 NAPOT 翻译验证 | NAPOT PTE 的 ppn[0] 编码为 1000，但 VA 不是 64 KiB 对齐（如 VA 偏移 0x1000），验证翻译结果 | 翻译结果中 PA 低位被 VA 低位覆盖（PPN 替换语义） |
| ALIGN-02 | PA 非 64 KiB 对齐 NAPOT 翻译验证 | NAPOT PTE 的 PA 字段不是 64 KiB 对齐，ppn[0] 低 4 位仍编码为 1000，验证翻译结果 | 翻译结果中 PA 低位由 vpn[0] 低位决定（ppn 低位被替换） |

---

### Group 13：PBMT 与 NAPOT 交互测试（条件性）

**规范依据**：
- `Svnapot_pbmt_interaction`：PTE bits 62-61 为 PBMT（由 Svpbmt 扩展定义）；如果实现了 Svpbmt，NAPOT 叶 PTE 也应支持 PBMT 属性；未实现时 PBMT 位应保持为 0

**测试职责**：验证 NAPOT PTE 与 Svpbmt 扩展的交互（仅当 Svpbmt 已实现时适用）。

> [!NOTE]
> 以下测试仅在目标平台实现了 Svpbmt 扩展时执行；如果 Svpbmt 未实现，这些测试应跳过。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PBMT-01 | NAPOT + PBMT PMA 映射 | 64 KiB NAPOT PTE 设置 PBMT=PMA（00），正常访问 | 正常访问 |
| PBMT-02 | NAPOT + PBMT NC 映射 | 64 KiB NAPOT PTE 设置 PBMT=NC（01），验证非缓存访问 | 正常访问，无缓存语义 |
| PBMT-03 | NAPOT + PBMT IO 映射 | 64 KiB NAPOT PTE 设置 PBMT=IO（10），验证 I/O 强排序访问 | 正常访问，强排序语义 |

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1（基本映射）、Group 3（保留编码异常）、Group 4（权限控制） | NAPOT64-01~06、RSVD-01~09、PERM-01~07 | 核心功能：NAPOT 翻译、保留编码 fault、权限正确性 |
| P1（重要） | Group 2（PPN 替换）、Group 5（N=0 行为）、Group 6（SFENCE.VMA）、Group 11（RSW 位） | PPN-01~05、NZERO-01~03、SFENCE-01~05、RSW-01~02、AD-05~07 | PPN 替换语义正确性、N=0 兼容性、TLB 管理、RSW 位保障、A/D 别名独立性 |
| P2（建议） | Group 7（多模式兼容）、Group 8（A/D 位）、Group 9（TLB 缓存）、Group 10（bits 5-0 一致性）、Group 12（边界对齐） | MODE-01~05、AD-01~04、TLB-01~04、BITS50-01~03、ALIGN-01~02 | 跨模式验证、高级行为、别名一致性、对齐边界 |
| P3（可选） | Group 13（PBMT 交互） | PBMT-01~03 | 依赖 Svpbmt 扩展，条件性实现 |

---

## 结果判定原则

- 平台实现 Svnapot 但行为偏离 SPEC（如有效编码翻译错误、保留编码不触发 page-fault、PPN 替换语义错误等）：保持用例失败，与 SPEC 比对后将问题记录至 `bugs/` 目录，禁止修改用例或加 workaround 适配错误实现。
- "实现选择/不可预测"类用例（AD-05~07、TLB-04、BITS50-01~02）：验证实现不产生不可恢复错误，不对具体选择做强断言。

---

## 附录：规范性引用

### NAPOT PTE 编码（64 KiB，Version 1.0）

Svnapot 扩展中，PTE bit 63 为 N 位。当 N=1 且 i=0 时，ppn[0] 低 4 位编码决定 NAPOT 区域：仅 `x xxxx 1000` 为有效（64 KiB 连续区域，napot_bits=4，覆盖 16 个 4 KiB 页面，基地址须 64 KiB 对齐），其余编码均为 Reserved。i≥1 级别在 N=1 时所有编码均为 Reserved。

### 相关 scause 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| Instruction page fault | 12 | 取指页错误 |
| Load page fault | 13 | load 页错误 |
| Store/AMO page fault | 15 | store/AMO 页错误 |

---

## 参考

- `svnapot.adoc` — Svnapot 扩展定义
- `supervisor.adoc` — SFENCE.VMA、satp、PTE 格式定义
- `Hypervisor_Sv_test_plan.md` — Hypervisor × Svnapot 交叉测试计划（G-stage NAPOT）
- `Svpbmt_test_plan.md` — Svpbmt 测试计划（PBMT 交互参考）

---

## 附录 A：规范点覆盖矩阵

下表标明"覆盖的规范点"章节中每条规范点被哪些测试用例覆盖。G-stage 相关规范点由 `Hypervisor_Sv_test_plan.md` 覆盖。

| Norm ID | 覆盖的测试 ID |
|---------|---------------|
| `norm:Svnapot_pte_N` | NAPOT64-01~06、NZERO-01~03、PERM-01~07、BITS50-01~03 |
| `norm:Svnapot_range_napot` | NAPOT64-01~06、PPN-01~05、ALIGN-01~02 |
| `norm:Svnapot_depends_Sv39` | MODE-01~05 |
| `norm:Svnapot_valid_encoding` | NAPOT64-01~06、PPN-01~05、PERM-01~07、MODE-01~03 |
| `norm:Svnapot_implicit_read_ppn_subst` | PPN-01~05、ALIGN-01~02 |
| `norm:Svnapot_reserved_encoding_fault` | RSVD-01~09 |
| `norm:Svnapot_cache_entries` | TLB-01~04、SFENCE-01~05 |
| `norm:Svnapot_hyp_gstage` | 由 `Hypervisor_Sv_test_plan.md`（HCROSS-SVNAPOT-01~03）覆盖 |
| `Svnapot_ad_alias_independence` | AD-01~07 |
| `Svnapot_rsw_reserved` | RSW-01~02 |
| `Svnapot_pbmt_interaction` | PBMT-01~03（条件性，依赖 Svpbmt 扩展） |
