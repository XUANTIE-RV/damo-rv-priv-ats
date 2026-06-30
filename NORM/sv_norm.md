# Sv 扩展 Norm 规范条目表

> 来源：`SPEC/sv.adoc` — Supervisor Virtual-Memory Extensions 章节

## Svnapot - NAPOT 转换连续性

### 基本定义

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svnapot_pte_N` | N=1 | 当 PTE 的 N=1 时，该 PTE 表示一段连续虚拟到物理转换范围的一部分。 |
| `norm:Svnapot_range_napot` | Such ranges must be of a naturally aligned power-of-2 (NAPOT) granularity larger than the base page size. | 此类范围必须是大于基本页面大小的自然对齐的 2 的幂次方（NAPOT）粒度。 |
| `norm:Svnapot_depends_Sv39` | The Svnapot extension depends on the Sv39 extension. | Svnapot 扩展依赖于 Sv39 扩展。 |

### 地址转换行为

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svnapot_valid_encoding` | valid according to <<ptenapot>> | 如果 PTE 编码根据 <<ptenapot>> 有效。 |
| `norm:Svnapot_implicit_read_ppn_subst` | implicit reads of a NAPOT PTE return a copy of _pte_ in which __pte__.__ppn__[__i__][__pte__.__napot_bits__-1:0] is replaced by __vpn__[__i__][__pte__.__napot_bits__-1:0]. | NAPOT PTE 的隐式读取返回 PTE 的副本，其中 __pte__.__ppn__[__i__][__pte__.__napot_bits__-1:0] 被替换为 __vpn__[__i__][__pte__.__napot_bits__-1:0]。 |
| `norm:Svnapot_reserved_encoding_fault` | reserved according to <<ptenapot>>, then a page-fault exception must be raised. | 如果 PTE 编码根据 <<ptenapot>> 是保留的，则必须触发页错误异常。 |
| `norm:Svnapot_cache_entries` | Implicit reads of NAPOT page table entries may create address-translation cache entries mapping _a_ + _j_×PTESIZE to a copy of _pte_ in which _pte_._ppn_[_i_][_pte_.__napot_bits__-1:0] is replaced by _vpn[i][pte.napot_bits_-1:0], for any or all _j_ such that __j__ >> __napot_bits__ = __vpn__[__i__] >> __napot_bits__, all for the address space identified in _satp_ as loaded by step 1. | NAPOT 页表条目的隐式读取可以创建地址转换缓存条目，将 _a_ + _j_×PTESIZE 映射到 PTE 的副本（其中 PPN 的低位被 VPN 的对应位替换），适用于满足 __j__ >> __napot_bits__ = __vpn__[__i__] >> __napot_bits__ 的任意或所有 _j_。 |

### Hypervisor 支持

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svnapot_hyp_gstage` | Svnapot is also supported in G-stage translation. | Svnapot 在 G 阶段转换中也得到支持。 |

## Svpbmt - 基于页面的内存类型

### 基本定义

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svpbmt_depends_Sv39` | The Svpbmt extension depends on the Sv39 extension. | Svpbmt 扩展依赖于 Sv39 扩展。 |
| `norm:Svpbmt_impl_may_override_pmas` | Implementations may override additional PMAs not explicitly listed in <<pbmt>>. | 实现可以覆盖 <<pbmt>> 中未明确列出的额外 PMA。 |

### PTE 编码规则

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svpbmt_nonleaf_pte_pbmt_must_be_zero` | Until their use is defined by a standard extension, they must be cleared by software for forward compatibility, or else a page-fault exception is raised. | 在其用途被标准扩展定义之前，软件必须将其清零以保证向前兼容性，否则会触发页错误异常。 |
| `norm:Svpbmt_leaf_pte_pbmt_reserved_3_fault` | Until this value is defined by a standard extension, using this reserved value in a leaf PTE raises a page-fault exception. | 在此值被标准扩展定义之前，在叶 PTE 中使用此保留值会触发页错误异常。 |

### 内存排序规则

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svpbmt_obeys_mem_ordering` | memory accesses to such pages obey the memory ordering rules of the final effective attribute. | 对此类页面的内存访问遵循最终有效属性的内存排序规则。 |
| `norm:Svpbmt_io_pma_nc_pbmt_obey_rvwmo` | If the underlying physical memory attribute for a page is I/O, and the page has PBMT=NC, then accesses to that page obey RVWMO. | 如果页面的底层物理内存属性是 I/O，且页面具有 PBMT=NC，则对该页面的访问遵循 RVWMO。 |
| `norm:Svpbmt_io_pma_nc_pbmt_treated_as_io_and_memory` | accesses to such pages are considered to be _both_ I/O and main memory accesses for the purposes of FENCE, _.aq_, and _.rl_. | 出于 FENCE、_.aq_ 和 _.rl_ 的目的，对此类页面的访问被视为同时是 I/O 和主存访问。 |
| `norm:Svpbmt_memory_pma_io_pbmt_strong_io_ordering` | accesses to that page obey strong channel 0 I/O ordering rules. | 对该页面的访问遵循强通道 0 I/O 排序规则。 |
| `norm:Svpbmt_memory_pma_io_pbmt_treated_as_io_and_memory` | accesses to such pages are considered to be _both_ I/O and main memory accesses for the purposes of FENCE, _.aq_, and _.rl_. | 出于 FENCE、_.aq_ 和 _.rl_ 的目的，对此类页面的访问被视为同时是 I/O 和主存访问。 |

### 别名与一致性

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svpbmt_aliasing_attribute` | When Svpbmt is used with non-zero PBMT encodings, it is possible for multiple virtual aliases of the same physical page to exist simultaneously with different memory attributes. It is also possible for a U-mode or S-mode mapping through a PTE with Svpbmt enabled to observe different memory attributes for a given region of physical memory than a concurrent access to the same page performed by M-mode or when MODE=Bare. In such cases, the behaviors dictated by the attributes (including coherence, which is otherwise unaffected) may be violated. | 当 Svpbmt 与非零 PBMT 编码一起使用时，同一物理页面的多个虚拟别名可能同时存在并具有不同的内存属性。通过启用 Svpbmt 的 PTE 的 U 模式或 S 模式映射也可能观察到与 M 模式或 MODE=Bare 时并发访问同一页面不同的内存属性。在这种情况下，属性所规定的行为（包括一致性，否则不受影响）可能会被违反。 |
| `norm:Svpbmt_noncacheable_aliasing_no_coherence_loss` | Accessing the same location using different attributes that are both non-cacheable (e.g., NC and IO) does not cause loss of coherence. | 使用均为不可缓存的不同属性（如 NC 和 IO）访问同一位置不会导致一致性丢失。 |
| `norm:Svpbmt_noncacheable_aliasing_may_weaken_ordering` | might result in weaker memory ordering than the stricter attribute ordinarily guarantees. | 但可能导致比更严格的属性通常保证的更弱的内存排序。 |
| `norm:Svpbmt_noncacheable_aliasing_fence_prevents_ordering_loss` | `fence iorw, iorw` instruction between such accesses suffices to prevent loss of memory ordering. | 在此类访问之间执行 `fence iorw, iorw` 指令足以防止内存排序丢失。 |
| `norm:Svpbmt_cacheable_aliasing_may_cause_coherence_loss` | may cause loss of coherence. | 使用不同的可缓存性属性访问同一位置可能导致一致性丢失。 |
| `norm:Svpbmt_cacheable_aliasing_fence_flush_fence_required` | prevents both loss of coherence and loss of memory ordering: `fence iorw, iorw`, followed by `cbo.flush` to an address of that location, followed by a `fence iorw, iorw`. | 防止一致性和内存排序丢失的序列：`fence iorw, iorw`，后跟对该位置地址的 `cbo.flush`，再跟 `fence iorw, iorw`。 |

### 两阶段地址转换

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svpbmt_hgatp_stage_override_rule` | if `hgatp`.MODE is not equal to zero, non-zero G-stage PTE PBMT bits override the attributes in the PMA to produce an intermediate set of attributes. | 如果 `hgatp`.MODE 不等于零，非零的 G 阶段 PTE PBMT 位覆盖 PMA 中的属性以产生中间属性集。 |
| `norm:Svpbmt_vsatp_stage_override_rule` | if `vsatp`.MODE is not equal to zero, non-zero VS-stage PTE PBMT bits override the intermediate attributes to produce the final set of attributes used by accesses to the page in question. | 如果 `vsatp`.MODE 不等于零，非零的 VS 阶段 PTE PBMT 位覆盖中间属性以产生用于访问相关页面的最终属性集。 |

## Svadu - A/D 位硬件更新

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svadu_hw_update_a_d_bits` | If the Svadu extension is implemented, the `menvcfg`.ADUE field is writable. | 如果实现了 Svadu 扩展，则 `menvcfg`.ADUE 字段是可写的。 |
| `norm:Svadu_hypervisor_adue_writable` | If the hypervisor extension is additionally implemented, the `henvcfg`.ADUE field is also writable. | 如果还实现了 hypervisor 扩展，则 `henvcfg`.ADUE 字段也是可写的。 |
| `norm:Svadu_disabled_hw_update_falls_back_to_svade` | When hardware updating of A/D bits is disabled, the Svade extension, which mandates exceptions when A/D bits need be set, instead takes effect. | 当禁用 A/D 位的硬件更新时，Svade 扩展生效，该扩展要求在需要设置 A/D 位时触发异常。 |

## Svinval - 细粒度地址转换缓存失效

### 基本定义

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svinval_split_fine_grained` | that can be more efficiently batched or pipelined on certain classes of high-performance implementation. | 可以在某些高性能实现类别上更高效地批处理或流水线化。 |

### 指令语义

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svinval_sinval_vma_invalidates_same_as_sfence_vma` | However, unlike SFENCE.VMA, SINVAL.VMA instructions are only ordered with respect to SFENCE.VMA, SFENCE.W.INVAL, and SFENCE.INVAL.IR instructions as defined below. | 然而，与 SFENCE.VMA 不同，SINVAL.VMA 指令仅相对于 SFENCE.VMA、SFENCE.W.INVAL 和 SFENCE.INVAL.IR 指令有序。 |
| `norm:Svinval_sfence_w_inval_orders_before_sinval_vma` | The SFENCE.INVAL.IR instruction guarantees that any previous SINVAL.VMA instructions executed by the current hart are ordered before subsequent implicit references by that hart to the memory-management data structures. | SFENCE.INVAL.IR 指令保证当前 hart 执行的任何先前 SINVAL.VMA 指令在该 hart 后续对内存管理数据结构的隐式引用之前排序。 |

### 指令序列等价性

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svinval_sequence_rs1_rs2` | the values of _rs1_ and _rs2_ for the SFENCE.VMA are the same as those used in the SINVAL.VMA. | SFENCE.VMA 的 _rs1_ 和 _rs2_ 值与 SINVAL.VMA 中使用的相同。 |
| `norm:Svinval_sequence_reads_writes_before` | reads and writes prior to the SFENCE.W.INVAL are considered to be those prior to the SFENCE.VMA. | SFENCE.W.INVAL 之前的读写被视为 SFENCE.VMA 之前的读写。 |
| `norm:Svinval_sequence_reads_writes_after` | reads and writes following the SFENCE.INVAL.IR are considered to be those subsequent to the SFENCE.VMA. | SFENCE.INVAL.IR 之后的读写被视为 SFENCE.VMA 之后的读写。 |

### Hypervisor 扩展

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svinval_hinval_vvma_gvma` | These have the same semantics as SINVAL.VMA, except that they combine with SFENCE.W.INVAL and SFENCE.INVAL.IR to replace HFENCE.VVMA and HFENCE.GVMA, respectively, instead of SFENCE.VMA. | 这些指令与 SINVAL.VMA 具有相同的语义，只是它们与 SFENCE.W.INVAL 和 SFENCE.INVAL.IR 结合分别替换 HFENCE.VVMA 和 HFENCE.GVMA，而不是 SFENCE.VMA。 |
| `norm:Svinval_hinval_gvma_uses_vmid` | HINVAL.GVMA uses VMIDs instead of ASIDs. | HINVAL.GVMA 使用 VMID 而不是 ASID。 |

### 权限与异常

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svinval_illegal_instruction_u_mode` | In particular, an attempt to execute any of these instructions in U-mode always raises an illegal-instruction exception. | 特别是，在 U 模式下尝试执行这些指令中的任何一个总是触发非法指令异常。 |
| `norm:Svinval_illegal_instruction_tvm` | An attempt to execute SINVAL.VMA or HINVAL.GVMA in S-mode or HS-mode when `mstatus`.TVM=1 also raises an illegal-instruction exception. | 当 `mstatus`.TVM=1 时，在 S 模式或 HS 模式下尝试执行 SINVAL.VMA 或 HINVAL.GVMA 也会触发非法指令异常。 |
| `norm:Svinval_virtual_instruction_vu_vs` | An attempt to execute HINVAL.VVMA or HINVAL.GVMA in VS-mode or VU-mode, or to execute SINVAL.VMA in VU-mode, raises a virtual-instruction exception. | 在 VS 模式或 VU 模式下尝试执行 HINVAL.VVMA 或 HINVAL.GVMA，或在 VU 模式下执行 SINVAL.VMA，会触发虚拟指令异常。 |
| `norm:Svinval_virtual_instruction_vtvms` | When `hstatus`.VTVM=1, an attempt to execute SINVAL.VMA in VS-mode also raises a virtual-instruction exception. | 当 `hstatus`.VTVM=1 时，在 VS 模式下尝试执行 SINVAL.VMA 也会触发虚拟指令异常。 |
| `norm:Svinval_sfence_w_inval_inval_u_mode` | raises an illegal-instruction exception. | 在 U 模式下执行 SFENCE.W.INVAL 或 SFENCE.INVAL.IR 会触发非法指令异常。 |
| `norm:Svinval_sfence_w_inval_inval_vu_mode` | Doing so in VU-mode raises a virtual-instruction exception. | 在 VU 模式下执行会触发虚拟指令异常。 |
| `norm:Svinval_sfence_w_inval_inval_s_vs_mode` | SFENCE.W.INVAL and SFENCE.INVAL.IR are unaffected by the `mstatus`.TVM and `hstatus`.VTVM fields and hence are always permitted in S-mode and VS-mode. | SFENCE.W.INVAL 和 SFENCE.INVAL.IR 不受 `mstatus`.TVM 和 `hstatus`.VTVM 字段的影响，因此在 S 模式和 VS 模式下始终允许。 |

## Svvptc - 标记 PTE 有效后省略内存管理指令

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svvptc_explicit_stores_update_valid_bit` | When the Svvptc extension is implemented, explicit stores by a hart that update the Valid bit of leaf and/or non-leaf PTEs from 0 to 1 and are visible to a hart will eventually become visible within a bounded timeframe to subsequent implicit accesses by that hart to such PTEs. | 当实现 Svvptc 扩展时，hart 将叶和/或非叶 PTE 的 Valid 位从 0 更新到 1 的显式存储，如果对 hart 可见，最终将在有界时间范围内对该 hart 后续对此类 PTE 的隐式访问可见。 |

## Svrsw60t59b - PTE 软件保留位 60-59

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Svrsw60t59b_reserved_bits_60_59` | If the Svrsw60t59b extension is implemented, then bits 60-59 of the page table entries (PTEs) are reserved for use by supervisor software and are ignored by the implementation. | 如果实现了 Svrsw60t59b 扩展，则页表条目（PTE）的第 60-59 位保留供监管软件使用，并被实现忽略。 |
| `norm:Svrsw60t59b_h_g_stage_reserved_bits` | If the Hypervisor (H) extension is also implemented, then bits 60-59 of the G-stage PTEs are reserved for use by supervisor software and are ignored by the implementation. | 如果还实现了 Hypervisor (H) 扩展，则 G 阶段 PTE 的第 60-59 位保留供监管软件使用，并被实现忽略。 |
| `norm:Svrsw60t59b_depends_on_sv39` | The Svrsw60t59b extension depends on Sv39. | Svrsw60t59b 扩展依赖于 Sv39。 |
