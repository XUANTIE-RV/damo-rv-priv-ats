**中文 | [English](../testplan_en/Svinval_test_plan_en.md)**

# Svinval 扩展测试计划

本文档描述 Svinval（Fine-Grained Address-Translation Cache Invalidation）扩展的测试计划。Svinval 扩展将 SFENCE.VMA 指令拆分为更细粒度的失效和排序操作（SINVAL.VMA、SFENCE.W.INVAL、SFENCE.INVAL.IR），以便在高性能实现中更高效地进行批量或流水线化的 TLB 失效操作。

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/svinval.adoc` — Svinval 扩展：SINVAL.VMA / SFENCE.W.INVAL / SFENCE.INVAL.IR 语义、三指令序列与 SFENCE.VMA 的等价性、各特权级下的 illegal-instruction / virtual-instruction 触发规则
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — SFENCE.VMA 定义（用于序列等价性对照）

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内上述路径文件）

---

## 覆盖的规范点

下表列出本方案覆盖的规范点。带 `norm:` 前缀的为 SPEC 官方 normative rule 标签。

| Norm ID | 原文（要点） | 中文说明 |
|---------|------|----------|
| `norm:Svinval_split_fine_grained` | ...that can be more efficiently batched or pipelined on certain classes of high-performance implementation. | 可以在某些高性能实现类别上更高效地批处理或流水线化。 |
| `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma` | However, unlike SFENCE.VMA, SINVAL.VMA instructions are only ordered with respect to SFENCE.VMA, SFENCE.W.INVAL, and SFENCE.INVAL.IR instructions as defined below. | 与 SFENCE.VMA 不同，SINVAL.VMA 指令仅相对于 SFENCE.VMA、SFENCE.W.INVAL 和 SFENCE.INVAL.IR 指令有序。 |
| `norm:Svinval_sfence_w_inval_orders_before_sinval_vma` | The SFENCE.INVAL.IR instruction guarantees that any previous SINVAL.VMA instructions executed by the current hart are ordered before subsequent implicit references by that hart to the memory-management data structures. | SFENCE.INVAL.IR 保证当前 hart 执行的任何先前 SINVAL.VMA 指令在该 hart 后续对内存管理数据结构的隐式引用之前排序。 |
| `norm:Svinval_sequence_rs1_rs2` | the values of rs1 and rs2 for the SFENCE.VMA are the same as those used in the SINVAL.VMA. | SFENCE.VMA 的 rs1 和 rs2 值与 SINVAL.VMA 中使用的相同。 |
| `norm:Svinval_sequence_reads_writes_before` | reads and writes prior to the SFENCE.W.INVAL are considered to be those prior to the SFENCE.VMA. | SFENCE.W.INVAL 之前的读写被视为 SFENCE.VMA 之前的读写。 |
| `norm:Svinval_sequence_reads_writes_after` | reads and writes following the SFENCE.INVAL.IR are considered to be those subsequent to the SFENCE.VMA. | SFENCE.INVAL.IR 之后的读写被视为 SFENCE.VMA 之后的读写。 |
| `norm:Svinval_hinval_vvma_gvma` | These have the same semantics as SINVAL.VMA, except that they combine with SFENCE.W.INVAL and SFENCE.INVAL.IR to replace HFENCE.VVMA and HFENCE.GVMA, respectively. | HINVAL.VVMA/GVMA 与 SINVAL.VMA 语义相同，分别与 SFENCE.W.INVAL/SFENCE.INVAL.IR 结合替换 HFENCE.VVMA/HFENCE.GVMA。 |
| `norm:Svinval_hinval_gvma_uses_vmid` | HINVAL.GVMA uses VMIDs instead of ASIDs. | HINVAL.GVMA 使用 VMID 而不是 ASID。 |
| `norm:Svinval_illegal_instruction_u_mode` | In particular, an attempt to execute any of these instructions in U-mode always raises an illegal-instruction exception. | 在 U 模式下尝试执行这些指令中的任何一个总是触发非法指令异常。 |
| `norm:Svinval_illegal_instruction_tvm` | An attempt to execute SINVAL.VMA or HINVAL.GVMA in S-mode or HS-mode when `mstatus`.TVM=1 also raises an illegal-instruction exception. | 当 `mstatus`.TVM=1 时，在 S 模式或 HS 模式下尝试执行 SINVAL.VMA 或 HINVAL.GVMA 也会触发非法指令异常。 |
| `norm:Svinval_virtual_instruction_vu_vs` | An attempt to execute HINVAL.VVMA or HINVAL.GVMA in VS-mode or VU-mode, or to execute SINVAL.VMA in VU-mode, raises a virtual-instruction exception. | 在 VS/VU 模式下执行 HINVAL.VVMA/GVMA，或在 VU 模式下执行 SINVAL.VMA，触发虚拟指令异常。 |
| `norm:Svinval_virtual_instruction_vtvms` | When `hstatus`.VTVM=1, an attempt to execute SINVAL.VMA in VS-mode also raises a virtual-instruction exception. | 当 `hstatus`.VTVM=1 时，在 VS 模式下执行 SINVAL.VMA 也触发虚拟指令异常。 |
| `norm:Svinval_sfence_w_inval_inval_u_mode` | Attempting to execute SFENCE.W.INVAL or SFENCE.INVAL.IR in U-mode raises an illegal-instruction exception. | 在 U 模式下执行 SFENCE.W.INVAL 或 SFENCE.INVAL.IR 触发非法指令异常。 |
| `norm:Svinval_sfence_w_inval_inval_vu_mode` | Doing so in VU-mode raises a virtual-instruction exception. | 在 VU 模式下执行 SFENCE.W.INVAL 或 SFENCE.INVAL.IR 触发虚拟指令异常。 |
| `norm:Svinval_sfence_w_inval_inval_s_vs_mode` | SFENCE.W.INVAL and SFENCE.INVAL.IR are unaffected by the `mstatus`.TVM and `hstatus`.VTVM fields and hence are always permitted in S-mode and VS-mode. | SFENCE.W.INVAL 和 SFENCE.INVAL.IR 不受 `mstatus`.TVM 和 `hstatus`.VTVM 影响，在 S 模式和 VS 模式下始终允许。 |

---

## 不在测试范围内

- **HINVAL.VVMA / HINVAL.GVMA 指令**：需要 Hypervisor 扩展支持，其功能、VMID 语义与 VS/VU-mode virtual-instruction 触发由 `Hypervisor_Sv_test_plan.md`（Hypervisor × Svinval 交叉测试）覆盖。本方案仅覆盖非虚拟化场景下的 SINVAL.VMA / SFENCE.W.INVAL / SFENCE.INVAL.IR。
- **多核场景下的 TLB 一致性**：本方案聚焦单 hart 行为。

---

## 测试分组

### Group 1：SINVAL.VMA 基本功能

**规范依据**：
- `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma`：SINVAL.VMA 失效与 SFENCE.VMA 相同的地址翻译缓存条目
- `norm:Svinval_sequence_rs1_rs2`：三指令序列中 SINVAL.VMA 的 rs1/rs2 等同于假设的 SFENCE.VMA 的 rs1/rs2

**测试职责**：验证 SFENCE.W.INVAL + SINVAL.VMA + SFENCE.INVAL.IR 序列在功能上等同于 SFENCE.VMA，即修改页表后通过该序列刷新 TLB，后续访问使用新的翻译。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SINVAL-01 | 权限升级后序列刷新 | 将 R-only 页面升级为 RW，执行 SFENCE.W.INVAL + SINVAL.VMA(va, asid=0) + SFENCE.INVAL.IR 后写入 | 写入成功 |
| SINVAL-02 | 映射失效后序列刷新 | 将有效映射改为无效（V=0），执行三指令序列后访问 | page-fault |
| SINVAL-03 | 权限降级后序列刷新 | 将 RWX 页面降级为 R-only，执行三指令序列后写入 | store page-fault |
| SINVAL-04 | 物理地址重映射后序列刷新 | 修改 PTE 指向不同的物理页，执行三指令序列后读取 | 读到新物理页的数据 |
| SINVAL-05 | rs1=x0 全地址刷新 | 修改多个页面的 PTE，执行 SINVAL.VMA(rs1=x0, rs2=x0) 全局刷新 | 所有修改的页面新权限生效 |
| SINVAL-06 | 2M 大页面 SINVAL.VMA 失效 | 修改 2M megapage 的 PTE 权限后执行三指令序列 | 整个 2M 区域新翻译生效 |
| SINVAL-07 | 1G 大页面 SINVAL.VMA 失效 | 修改 1G gigapage 的 PTE 权限后执行三指令序列 | 整个 1G 区域新翻译生效 |

---

### Group 2：排序语义验证

**规范依据**：
- `norm:Svinval_sfence_w_inval_orders_before_sinval_vma`：SFENCE.W.INVAL 保证先前的 store 在后续 SINVAL.VMA 之前排序；SFENCE.INVAL.IR 保证先前的 SINVAL.VMA 在后续隐式内存管理数据结构引用之前排序
- `norm:Svinval_sequence_reads_writes_before`：SFENCE.W.INVAL 之前的读写等同于 SFENCE.VMA 之前的读写
- `norm:Svinval_sequence_reads_writes_after`：SFENCE.INVAL.IR 之后的读写等同于 SFENCE.VMA 之后的读写

**测试职责**：验证三指令序列的排序保证——SFENCE.W.INVAL 之前的 store（页表修改）对 SINVAL.VMA 可见，SFENCE.INVAL.IR 之后的访问使用更新后的翻译。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ORDER-01 | store → W.INVAL → SINVAL → INVAL.IR → load | 修改 PTE（store），执行完整三指令序列，然后 load 验证新翻译生效 | load 使用新翻译 |
| ORDER-02 | 缺少 SFENCE.W.INVAL 的序列 | 修改 PTE 后仅执行 SINVAL.VMA + SFENCE.INVAL.IR（省略 SFENCE.W.INVAL） | 行为不确定（可能使用旧翻译） |
| ORDER-03 | 缺少 SFENCE.INVAL.IR 的序列 | 修改 PTE 后执行 SFENCE.W.INVAL + SINVAL.VMA（省略 SFENCE.INVAL.IR） | 行为不确定（可能使用旧翻译） |
| ORDER-04 | 完整序列与 SFENCE.VMA 等价性 | 对同一页面分别用 SFENCE.VMA 和三指令序列刷新，验证结果一致 | 两种方式结果相同 |
| ORDER-05 | SFENCE.VMA 作为 SINVAL.VMA 的排序 fence | 修改 PTE 后执行 SINVAL.VMA，再执行 SFENCE.VMA（替代 SFENCE.INVAL.IR），然后访问页面 | SFENCE.VMA 保证 SINVAL.VMA 生效，新翻译生效 |
| ORDER-06 | 多个 SINVAL.VMA 后跟 SFENCE.VMA | 修改多个页面 PTE，批量执行多个 SINVAL.VMA，最后用 SFENCE.VMA 替代 SFENCE.INVAL.IR | 所有页面新翻译生效 |

> [!NOTE]
> ORDER-02 和 ORDER-03 测试的是不完整序列的行为。规范不保证不完整序列的正确性，但规范明确允许较简单的实现将 SINVAL.VMA 实现为与 SFENCE.VMA 完全相同、将 SFENCE.W.INVAL 和 SFENCE.INVAL.IR 实现为 NOP，因此不完整序列在这类实现上可能仍然正确工作。这两个用例仅用于探测实现行为，不作为合规性判定依据。

---

### Group 3：批量失效操作

**规范依据**：
- `norm:Svinval_split_fine_grained`：Svinval 的设计目标是支持批量或流水线化的 TLB 失效操作
- `norm:Svinval_sfence_w_inval_orders_before_sinval_vma`：多个 SINVAL.VMA 可以在 SFENCE.W.INVAL 和 SFENCE.INVAL.IR 之间批量执行

**测试职责**：验证在 SFENCE.W.INVAL 和 SFENCE.INVAL.IR 之间执行多个 SINVAL.VMA 指令的正确性，这是 Svinval 扩展的核心使用场景。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| BATCH-01 | 批量失效多个页面 | 修改 3 个页面的 PTE，在 W.INVAL 和 INVAL.IR 之间执行 3 个 SINVAL.VMA | 所有 3 个页面新权限生效 |
| BATCH-02 | 批量失效不同权限变更 | 页面 A 升级 R→RW，页面 B 降级 RW→R，页面 C 失效 V→0，批量刷新 | A 可写，B 写触发 fault，C 访问触发 fault |
| BATCH-03 | 批量失效大量页面 | 修改 16 个连续页面的 PTE，批量执行 16 个 SINVAL.VMA | 所有 16 个页面新权限生效 |
| BATCH-04 | 批量失效混合 rs1 参数 | 部分 SINVAL.VMA 指定具体地址，部分使用 rs1=x0 | 所有指定的页面被正确刷新 |

---

### Group 4：rs1/rs2 参数组合

**规范依据**：
- `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma`：SINVAL.VMA 使用与 SFENCE.VMA 相同的 rs1/rs2 语义
- `norm:Svinval_sequence_rs1_rs2`：三指令序列中 SINVAL.VMA 的 rs1/rs2 等同于假设的 SFENCE.VMA 的 rs1/rs2

**测试职责**：验证 SINVAL.VMA 的 rs1（虚拟地址）和 rs2（ASID）参数的不同组合，确保与 SFENCE.VMA 的参数语义一致。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PARAM-01 | rs1=addr, rs2=x0 | 指定地址，ASID=0（所有 ASID），修改该地址 PTE 后刷新 | 该地址新翻译生效 |
| PARAM-02 | rs1=x0, rs2=x0 | 全局刷新（所有地址、所有 ASID），修改多个 PTE 后刷新 | 所有修改的页面新翻译生效 |
| PARAM-03 | rs1=addr, rs2=asid | 指定地址和 ASID，修改该地址 PTE 后刷新 | 该地址新翻译生效 |
| PARAM-04 | rs1=x0, rs2=asid | 指定 ASID 的所有地址，修改多个 PTE 后刷新 | 该 ASID 下所有修改的页面新翻译生效 |
| PARAM-05 | 刷新不匹配的地址 | 修改页面 A 的 PTE，但 SINVAL.VMA 指定页面 B 的地址 | 页面 A 可能仍使用旧翻译（行为不确定） |

---

### Group 5：SINVAL.VMA 特权级异常检查

**规范依据**：
- `norm:Svinval_illegal_instruction_u_mode`：U-mode 执行 SINVAL.VMA 始终触发 illegal-instruction 异常
- `norm:Svinval_illegal_instruction_tvm`：mstatus.TVM=1 时，S-mode 执行 SINVAL.VMA 触发 illegal-instruction 异常

**测试职责**：验证 SINVAL.VMA 指令在不同特权级和 CSR 配置下的异常行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PRIV-01 | U-mode 执行 SINVAL.VMA | 在 U-mode 下执行 SINVAL.VMA 指令 | illegal-instruction 异常（cause=2） |
| PRIV-02 | S-mode TVM=0 执行 SINVAL.VMA | mstatus.TVM=0 时，S-mode 执行 SINVAL.VMA | 正常执行，无异常 |
| PRIV-03 | S-mode TVM=1 执行 SINVAL.VMA | mstatus.TVM=1 时，S-mode 执行 SINVAL.VMA | illegal-instruction 异常（cause=2） |
| PRIV-04 | M-mode 执行 SINVAL.VMA | 在 M-mode 下执行 SINVAL.VMA 指令 | 正常执行，无异常 |

---

### Group 6：SFENCE.W.INVAL / SFENCE.INVAL.IR 特权级行为

**规范依据**：
- `norm:Svinval_sfence_w_inval_inval_u_mode`：U-mode 执行 SFENCE.W.INVAL 或 SFENCE.INVAL.IR 触发 illegal-instruction 异常
- `norm:Svinval_sfence_w_inval_inval_s_vs_mode`：SFENCE.W.INVAL 和 SFENCE.INVAL.IR 不受 mstatus.TVM 和 hstatus.VTVM 影响，S-mode 和 VS-mode 始终允许执行

**测试职责**：验证 SFENCE.W.INVAL 和 SFENCE.INVAL.IR 指令的特权级访问控制，特别是它们不受 TVM 位影响的特性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FENCE-01 | U-mode 执行 SFENCE.W.INVAL | 在 U-mode 下执行 SFENCE.W.INVAL | illegal-instruction 异常（cause=2） |
| FENCE-02 | U-mode 执行 SFENCE.INVAL.IR | 在 U-mode 下执行 SFENCE.INVAL.IR | illegal-instruction 异常（cause=2） |
| FENCE-03 | S-mode TVM=0 执行 SFENCE.W.INVAL | mstatus.TVM=0 时，S-mode 执行 SFENCE.W.INVAL | 正常执行，无异常 |
| FENCE-04 | S-mode TVM=0 执行 SFENCE.INVAL.IR | mstatus.TVM=0 时，S-mode 执行 SFENCE.INVAL.IR | 正常执行，无异常 |
| FENCE-05 | S-mode TVM=1 执行 SFENCE.W.INVAL | mstatus.TVM=1 时，S-mode 执行 SFENCE.W.INVAL | 正常执行，无异常（不受 TVM 影响） |
| FENCE-06 | S-mode TVM=1 执行 SFENCE.INVAL.IR | mstatus.TVM=1 时，S-mode 执行 SFENCE.INVAL.IR | 正常执行，无异常（不受 TVM 影响） |
| FENCE-07 | M-mode 执行 SFENCE.W.INVAL | 在 M-mode 下执行 SFENCE.W.INVAL | 正常执行，无异常 |
| FENCE-08 | M-mode 执行 SFENCE.INVAL.IR | 在 M-mode 下执行 SFENCE.INVAL.IR | 正常执行，无异常 |

---

### Group 7：指令编码验证

**规范依据**：
- Svinval 扩展引入了 3 条新指令（SINVAL.VMA、SFENCE.W.INVAL、SFENCE.INVAL.IR），需要验证指令编码被硬件正确识别

**测试职责**：验证 Svinval 指令在 M-mode 下可以正常执行而不触发 illegal-instruction 异常。这是所有其他功能测试的前提——如果指令编码本身有误或平台不支持 Svinval 扩展，所有功能测试都将无法正确运行。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ENC-01 | SINVAL.VMA 指令编码验证 | 在 M-mode 下执行 SINVAL.VMA，确认不触发 illegal-instruction | 正常执行，无异常 |
| ENC-02 | SFENCE.W.INVAL 指令编码验证 | 在 M-mode 下执行 SFENCE.W.INVAL，确认不触发 illegal-instruction | 正常执行，无异常 |
| ENC-03 | SFENCE.INVAL.IR 指令编码验证 | 在 M-mode 下执行 SFENCE.INVAL.IR，确认不触发 illegal-instruction | 正常执行，无异常 |

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 7（编码验证）、Group 1（SINVAL.VMA 基本功能）、Group 5（特权级异常） | ENC-01~03、SINVAL-01~07、PRIV-01~04 | 指令编码基础验证、核心功能验证和安全性保证 |
| P1（重要） | Group 2（排序语义）、Group 6（SFENCE.W.INVAL/INVAL.IR 特权级） | ORDER-01~06、FENCE-01~08 | 排序正确性和特权级访问控制 |
| P2（建议） | Group 3（批量失效）、Group 4（rs1/rs2 参数组合） | BATCH-01~04、PARAM-01~05 | 批量操作和参数覆盖 |

---

## 结果判定原则

- 平台实现 Svinval 但行为偏离 SPEC（如三指令序列不等价于 SFENCE.VMA、U-mode/TVM=1 下未触发 illegal-instruction、SFENCE.W.INVAL/SFENCE.INVAL.IR 受 TVM 影响等）：保持用例失败，与 SPEC 比对后将问题记录至 `bugs/` 目录，禁止修改用例或加 workaround 适配错误实现。
- ORDER-02/03、PARAM-05 等"行为不确定"用例仅探测实现行为，不作为合规性判定依据。

---

## 附录：规范性引用

### Svinval 指令编码

下表为 Svinval 扩展引入指令的编码（R-type，opcode=SYSTEM=0x73，funct3=000），属于规范性引用：

| 指令 | funct7 (31..25) | rs2 (24..20) | rs1 (19..15) | rd (11..7) | opcode (6..0) |
|------|-----------------|--------------|--------------|------------|---------------|
| `sinval.vma rs1, rs2` | 0001011 | rs2 | rs1 | 00000 | 1110011 |
| `sfence.w.inval` | 0001100 | 00000 | 00000 | 00000 | 1110011 |
| `sfence.inval.ir` | 0001100 | 00001 | 00000 | 00000 | 1110011 |
| `hinval.vvma rs1, rs2` | 0011011 | rs2 | rs1 | 00000 | 1110011 |
| `hinval.gvma rs1, rs2` | 0111011 | rs2 | rs1 | 00000 | 1110011 |

> HINVAL.VVMA / HINVAL.GVMA 的功能与异常验证由 `Hypervisor_Sv_test_plan.md` 覆盖。

### 相关 scause 常量

| 常量 | 值 | 说明 |
|------|-----|------|
| Illegal instruction | 2 | 非法指令 |
| Instruction page fault | 12 | 取指页错误 |
| Load page fault | 13 | load 页错误 |
| Store/AMO page fault | 15 | store/AMO 页错误 |
| Virtual instruction | 22 | 虚拟指令（Hypervisor 场景，由 `Hypervisor_Sv_test_plan.md` 覆盖） |

---

## 参考

- `svinval.adoc` — Svinval 扩展定义
- `supervisor.adoc` — SFENCE.VMA 定义
- `Hypervisor_Sv_test_plan.md` — Hypervisor × Svinval 交叉测试计划（HINVAL.VVMA/GVMA）

---

## 附录 A：规范点覆盖矩阵

下表标明"覆盖的规范点"章节中每条规范点被哪些测试用例覆盖。Hypervisor 相关规范点由 `Hypervisor_Sv_test_plan.md` 覆盖。

| Norm ID | 覆盖的测试 ID |
|---------|---------------|
| `norm:Svinval_split_fine_grained` | BATCH-01~04 |
| `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma` | SINVAL-01~07、PARAM-01~05、ORDER-05~06 |
| `norm:Svinval_sfence_w_inval_orders_before_sinval_vma` | ORDER-01~06 |
| `norm:Svinval_sequence_rs1_rs2` | SINVAL-01~07、PARAM-01~05 |
| `norm:Svinval_sequence_reads_writes_before` | ORDER-01~06 |
| `norm:Svinval_sequence_reads_writes_after` | ORDER-01~06 |
| `norm:Svinval_hinval_vvma_gvma` | 由 `Hypervisor_Sv_test_plan.md`（HCROSS-SINVAL-01~04）覆盖 |
| `norm:Svinval_hinval_gvma_uses_vmid` | 由 `Hypervisor_Sv_test_plan.md`（HCROSS-SINVAL-05~06）覆盖 |
| `norm:Svinval_illegal_instruction_u_mode` | PRIV-01 |
| `norm:Svinval_illegal_instruction_tvm` | PRIV-03 |
| `norm:Svinval_virtual_instruction_vu_vs` | 由 `Hypervisor_Sv_test_plan.md`（HCROSS-SINVAL-07~10、HCROSS-SINVAL-15）覆盖 |
| `norm:Svinval_virtual_instruction_vtvms` | 由 `Hypervisor_Sv_test_plan.md`（VS-mode VTVM 场景）覆盖 |
| `norm:Svinval_sfence_w_inval_inval_u_mode` | FENCE-01、FENCE-02 |
| `norm:Svinval_sfence_w_inval_inval_vu_mode` | 由 `Hypervisor_Sv_test_plan.md`（HCROSS-SINVAL-11~12）覆盖 |
| `norm:Svinval_sfence_w_inval_inval_s_vs_mode` | FENCE-03~06 |
