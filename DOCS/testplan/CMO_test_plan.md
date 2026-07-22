# CMO (Cache Management Operations) 综合测试计划

## 概述

本测试计划覆盖 RISC-V Cache Management Operations (CMO) 扩展的所有核心功能点，依据 `SPEC/riscv-isa-manual/src/unpriv/cmo.adoc` 中的规范点（norm 标记）编写。

CMO 扩展包含三个子扩展：
- **Zicbom**：Cache-Block Management 指令（cbo.inval, cbo.clean, cbo.flush）
- **Zicboz**：Cache-Block Zero 指令（cbo.zero）
- **Zicbop**：Cache-Block Prefetch 指令（prefetch.r, prefetch.w, prefetch.i）

### 本文档覆盖的 SPEC 章节
- CSR controls for CMO instructions（menvcfg/senvcfg 中 CBIE/CBCFE/CBZE 字段，M/HS/U-mode）
- Traps（illegal-instruction, page-fault, access-fault）
- Zicbom Extension（cbo.inval/cbo.clean/cbo.flush 功能语义）
- Zicboz Extension（cbo.zero 功能语义）
- Zicbop Extension（prefetch.r/prefetch.w/prefetch.i 功能语义）
- Memory Ordering（PPO rules）

### 由其他测试计划覆盖
- CMO × Hypervisor 交互（henvcfg CBIE/CBCFE/CBZE 对 VS/VU-mode 控制、htinst/mtinst 标准转换、G-stage guest-page-fault、virtual-instruction）→ `Hypervisor_CMO_test_plan.md`

---

## 覆盖的规范点

本章节列出本文档所有测试组中引用的规范点（norm ID），已去重并按字母顺序排列。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:cache_block_size` | The capacity and organization of a cache and the size of a cache block are both implementation-specific... the size of a cache block shall be uniform throughout the system. | Cache block 大小为实现特定，但在整个系统中必须统一。 |
| `norm:cbm_access` | A cache-block management instruction is permitted to access the specified cache block whenever a load instruction or store instruction is permitted to access the corresponding physical addresses. | 当 load 或 store 指令被允许访问对应物理地址时，cache-block management 指令即被允许访问。 |
| `norm:cbm_unperm_fault` | If access to the cache block is not permitted, a cache-block management instruction raises a store page-fault or store guest-page-fault exception if address translation does not permit any access or raises a store access-fault exception otherwise. | 访问不被允许时，management 指令引发 store page-fault/store guest-page-fault 或 store access-fault。 |
| `norm:cbm_unperm_translate` | During address translation, the instruction also checks the accessed bit and may either raise an exception or set the bit as required. | 地址翻译时检查 accessed 位，可引发异常或设置该位。 |
| `norm:cbo-clean_cbo-flush` | A cbo.clean or cbo.flush instruction executes or raises an illegal-instruction or virtual-instruction exception based on the state of the envcfg CBCFE bits. | cbo.clean/cbo.flush 根据 envcfg CBCFE 位执行或引发异常。 |
| `norm:cbo-clean_offset` | The offset operand may be omitted; otherwise, any expression that computes the offset shall evaluate to zero. | offset 操作数可省略；否则计算 offset 的表达式必须求值为零。 |
| `norm:cbo-clean_op` | A cbo.clean instruction performs a clean operation on the cache block whose effective address is the base address specified in rs1. | cbo.clean 对 rs1 指定的基地址所在的 cache block 执行 clean 操作。 |
| `norm:cbo-flush_op` | A cbo.flush instruction performs a flush operation on the cache block that contains the address specified in rs1. | cbo.flush 对包含 rs1 地址的 cache block 执行 flush 操作。 |
| `norm:cbo-flush_unaligned` | It is not required that rs1 is aligned to the size of a cache block. On faults, the faulting virtual address is considered to be the value in rs1. | 不要求 rs1 对齐到 cache block 大小。fault 时报告 rs1 的值。 |
| `norm:cbo-inval` | A cbo.inval instruction executes or raises either an illegal-instruction exception or a virtual-instruction exception based on the state of the envcfg CBIE fields. | cbo.inval 根据 envcfg CBIE 字段执行或引发异常。 |
| `norm:cbo-inval_op` | A cbo.inval instruction performs an invalidate operation on the cache block that contains the address specified in rs1. | cbo.inval 对包含 rs1 地址的 cache block 执行 invalidate 操作。 |
| `norm:cbo-inval_unaligned` | It is not required that rs1 is aligned to the size of a cache block. On faults, the faulting virtual address is considered to be the value in rs1. | 不要求 rs1 对齐到 cache block 大小。fault 时报告 rs1 的值。 |
| `norm:cbo-zero_basedon_xenvcfg-CBZE` | A cbo.zero instruction executes or raises an illegal-instruction or virtual-instruction exception based on the state of the envcfg CBZE bits. | cbo.zero 根据 envcfg CBZE 位执行或引发异常。 |
| `norm:cbo-zero_offset` | The assembly offset operand may be omitted. If it isn't then any expression that computes the offset shall evaluate to zero. | offset 操作数可省略；否则必须求值为零。 |
| `norm:cbo-zero_op` | A cbo.zero instruction performs stores of zeros to the full set of bytes corresponding to the cache block that contains the address specified in rs1. | cbo.zero 对包含 rs1 地址的 cache block 全部字节写零。 |
| `norm:cbo-zero_specified_block` | These instructions operate on the cache block, or the memory locations corresponding to the cache block, whose effective address is specified in rs1. | cbo.zero 操作 rs1 指定的有效地址对应的 cache block。 |
| `norm:cbo-zero_unaligned` | It is not required that rs1 is aligned to the size of a cache block. On faults, the faulting virtual address is considered to be the value in rs1. | 不要求 rs1 对齐。fault 时报告 rs1 的值。 |
| `norm:cbp_access` | A cache-block prefetch instruction is permitted to access the specified cache block whenever a load instruction, store instruction, or instruction fetch is permitted to access the corresponding physical addresses. | 当 load/store/instruction fetch 被允许时，prefetch 指令即被允许访问。 |
| `norm:cbp_unperm_noexcep` | If access to the cache block is not permitted, a cache-block prefetch instruction does not raise any exceptions and shall not access any caches or memory. | 访问不被允许时，prefetch 指令不引发任何异常，也不访问 cache 或内存。 |
| `norm:cbp_unperm_translate` | During address translation, the instruction does not check the accessed and dirty bits and neither raises an exception nor sets the bits. | 地址翻译时不检查 accessed/dirty 位，不引发异常也不设置这些位。 |
| `norm:cbxe_unaffected` | The CBIE/CBCFE/CBZE fields in each envcfg register do not affect the read and write behavior of the same fields in the other envcfg registers. | 各 envcfg 寄存器中的 CBIE/CBCFE/CBZE 字段互不影响读写行为。 |
| `norm:cbz_access` | A cache-block zero instruction is permitted to access the specified cache block whenever a store instruction is permitted to access the corresponding physical addresses and when the PMAs indicate that cache-block zero instructions are a supported access type. | 当 store 被允许且 PMA 支持 cache-block zero 访问类型时，cbo.zero 被允许。 |
| `norm:cbz_unperm_fault` | If access to the cache block is not permitted, a cache-block zero instruction raises a store page-fault or store guest-page-fault exception if address translation does not permit write access or raises a store access-fault exception otherwise. | 访问不被允许时，cbo.zero 引发 store page-fault/store guest-page-fault 或 store access-fault。 |
| `norm:cbz_unperm_translate` | During address translation, the instruction also checks the accessed and dirty bits and may either raise an exception or set the bits as required. | 地址翻译时检查 accessed 和 dirty 位。 |
| `norm:fault_excep_csr` | When a page-fault, guest-page-fault, or access-fault exception is taken, the relevant *tval CSR is written with the faulting effective address (i.e. the value of rs1). | 异常发生时，*tval CSR 写入故障有效地址（即 rs1 的值）。 |
| `norm:no_addr_misaligned_excep` | CMO instructions do not generate address-misaligned exceptions. | CMO 指令不产生地址未对齐异常。 |
| `norm:PMA_same` | The PMAs shall be the same for all physical addresses in the cache block, and if write permission is granted by the supported access type PMAs, read permission shall also be granted. | cache block 内所有物理地址的 PMA 必须相同。 |
| `norm:PMP_same` | The PMP access control bits shall be the same for all physical addresses in the cache block, and if write permission is granted by the PMP access control bits, read permission shall also be granted. | cache block 内所有物理地址的 PMP 访问控制位必须相同。 |
| `norm:PPO_overlap_1` | If a precedes b in program order, then a will precede b in the global memory order if: a is an invalidate, clean, or flush, b is a load, and a and b access overlapping memory addresses. | invalidate/clean/flush 在程序序中先于重叠地址的 load，则在全局内存序中也先于该 load。 |
| `norm:prefetch-i_op` | A prefetch.i instruction indicates to hardware that the cache block whose effective address is the sum of the base address specified in rs1 and the sign-extended offset is likely to be accessed by an instruction fetch in the near future. | prefetch.i 提示硬件 rs1+offset 对应的 cache block 可能即将被指令取指访问。 |
| `norm:prefetch-r_op` | A prefetch.r instruction indicates to hardware that the cache block is likely to be accessed by a data read (i.e. load) in the near future. | prefetch.r 提示硬件该 cache block 可能即将被数据读取访问。 |
| `norm:prefetch-w_op` | A prefetch.w instruction indicates to hardware that the cache block is likely to be accessed by a data write (i.e. store) in the near future. | prefetch.w 提示硬件该 cache block 可能即将被数据写入访问。 |
| `norm:prefetch_operating_block` | These instructions operate on the cache block whose effective address is the sum of the base address specified in rs1 and the sign-extended offset encoded in imm[11:0], where imm[4:0] shall equal 0b00000. | prefetch 指令操作的有效地址为 rs1 + sign-extended offset，其中 imm[4:0] 必须为 0。 |
| `norm:cbo_rsv` | If the PMP/PMA constraints are not met, the behavior of a CBO instruction is UNSPECIFIED. | 若 PMP/PMA 约束不满足，CBO 指令行为未指定。 |

---

## Group 1. Zicbom CSR 控制 — cbo.inval (CBIE)

**规范依据**：
- `norm:cbo-inval`：cbo.inval 根据 envcfg CBIE 字段执行或引发 illegal-instruction / virtual-instruction exception
- `norm:cbxe_unaffected`：各 envcfg 的 CBIE 字段互不影响

**测试职责**：验证 M/HS/U-mode 下 menvcfg.CBIE / senvcfg.CBIE 对 cbo.inval 执行行为的控制。VS/VU-mode 的 henvcfg.CBIE 控制见 `Hypervisor_CMO_test_plan.md`。

**CBIE 编码**：
- `00`：禁止执行（引发异常）
- `01`：执行 flush 操作（替代 invalidate）
- `10`：保留
- `11`：执行 invalidate 操作

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| CBIE-01 | M-mode cbo.inval 不受 CBIE 限制 | menvcfg.CBIE=00，M-mode 执行 cbo.inval | 正常执行 invalidate（M-mode 不受限制） |
| CBIE-02 | HS-mode CBIE=00 触发 illegal-instruction | menvcfg.CBIE=00，HS-mode 执行 cbo.inval | illegal-instruction exception (cause=2) |
| CBIE-03 | HS-mode CBIE=01 执行 flush | menvcfg.CBIE=01，HS-mode 执行 cbo.inval | 正常执行（执行 flush 操作） |
| CBIE-04 | HS-mode CBIE=11 执行 invalidate | menvcfg.CBIE=11，HS-mode 执行 cbo.inval | 正常执行 invalidate 操作 |
| CBIE-05 | U-mode menvcfg.CBIE=00 触发 illegal-instruction | menvcfg.CBIE=00，U-mode 执行 cbo.inval | illegal-instruction exception (cause=2) |
| CBIE-06 | U-mode menvcfg.CBIE≠00 但 senvcfg.CBIE=00 | menvcfg.CBIE=11, senvcfg.CBIE=00，U-mode 执行 cbo.inval | illegal-instruction exception (cause=2) |
| CBIE-07 | U-mode 两级 CBIE 均非零执行 | menvcfg.CBIE=11, senvcfg.CBIE=11，U-mode 执行 cbo.inval | 正常执行 invalidate |
| CBIE-08 | U-mode senvcfg.CBIE=01 执行 flush | menvcfg.CBIE=11, senvcfg.CBIE=01，U-mode 执行 cbo.inval | 正常执行（flush 操作） |
| CBIE-09 | CBIE WARL 保留编码 10 行为 | 写 menvcfg.CBIE=10，读回 | WARL：不保留保留编码 10b（读回非 10） |
| CBIE-10 | 各 envcfg CBIE 字段互不影响 | 写 menvcfg.CBIE=00，检查 senvcfg.CBIE | 其他 envcfg 的 CBIE 不受影响 |

---

## Group 2. Zicbom CSR 控制 — cbo.clean / cbo.flush (CBCFE)

**规范依据**：
- `norm:cbo-clean_cbo-flush`：cbo.clean/cbo.flush 根据 envcfg CBCFE 位执行或引发异常
- `norm:cbxe_unaffected`：各 envcfg 的 CBCFE 字段互不影响

**测试职责**：验证 M/HS/U-mode 下 menvcfg.CBCFE / senvcfg.CBCFE 对 cbo.clean 和 cbo.flush 执行行为的控制。VS/VU-mode 的 henvcfg.CBCFE 控制见 `Hypervisor_CMO_test_plan.md`。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| CBCFE-01 | M-mode cbo.clean 不受 CBCFE 限制 | menvcfg.CBCFE=0，M-mode 执行 cbo.clean | 正常执行 |
| CBCFE-02 | M-mode cbo.flush 不受 CBCFE 限制 | menvcfg.CBCFE=0，M-mode 执行 cbo.flush | 正常执行 |
| CBCFE-03 | HS-mode CBCFE=0 cbo.clean 触发 illegal-instruction | menvcfg.CBCFE=0，HS-mode 执行 cbo.clean | illegal-instruction exception (cause=2) |
| CBCFE-04 | HS-mode CBCFE=0 cbo.flush 触发 illegal-instruction | menvcfg.CBCFE=0，HS-mode 执行 cbo.flush | illegal-instruction exception (cause=2) |
| CBCFE-05 | HS-mode CBCFE=1 cbo.clean 正常执行 | menvcfg.CBCFE=1，HS-mode 执行 cbo.clean | 正常执行 |
| CBCFE-06 | HS-mode CBCFE=1 cbo.flush 正常执行 | menvcfg.CBCFE=1，HS-mode 执行 cbo.flush | 正常执行 |
| CBCFE-07 | U-mode menvcfg.CBCFE=0 触发 illegal-instruction | menvcfg.CBCFE=0，U-mode 执行 cbo.clean | illegal-instruction exception (cause=2) |
| CBCFE-08 | U-mode menvcfg.CBCFE=1 但 senvcfg.CBCFE=0 | menvcfg.CBCFE=1, senvcfg.CBCFE=0，U-mode 执行 cbo.clean | illegal-instruction exception (cause=2) |
| CBCFE-09 | U-mode 两级 CBCFE 均为 1 正常执行 | menvcfg.CBCFE=1, senvcfg.CBCFE=1，U-mode 执行 cbo.clean | 正常执行 |
| CBCFE-10 | U-mode 两级 CBCFE 均为 1 cbo.flush 正常 | menvcfg.CBCFE=1, senvcfg.CBCFE=1，U-mode 执行 cbo.flush | 正常执行 |
| CBCFE-11 | 各 envcfg CBCFE 字段互不影响 | 写 menvcfg.CBCFE=0，检查 senvcfg.CBCFE | 其他 envcfg 的 CBCFE 不受影响 |

---

## Group 3. Zicboz CSR 控制 — cbo.zero (CBZE)

**规范依据**：
- `norm:cbo-zero_basedon_xenvcfg-CBZE`：cbo.zero 根据 envcfg CBZE 位执行或引发异常
- `norm:cbxe_unaffected`：各 envcfg 的 CBZE 字段互不影响

**测试职责**：验证 M/HS/U-mode 下 menvcfg.CBZE / senvcfg.CBZE 对 cbo.zero 执行行为的控制。VS/VU-mode 的 henvcfg.CBZE 控制见 `Hypervisor_CMO_test_plan.md`。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| CBZE-01 | M-mode cbo.zero 不受 CBZE 限制 | menvcfg.CBZE=0，M-mode 执行 cbo.zero | 正常执行 |
| CBZE-02 | HS-mode CBZE=0 触发 illegal-instruction | menvcfg.CBZE=0，HS-mode 执行 cbo.zero | illegal-instruction exception (cause=2) |
| CBZE-03 | HS-mode CBZE=1 正常执行 | menvcfg.CBZE=1，HS-mode 执行 cbo.zero | 正常执行 |
| CBZE-04 | U-mode menvcfg.CBZE=0 触发 illegal-instruction | menvcfg.CBZE=0，U-mode 执行 cbo.zero | illegal-instruction exception (cause=2) |
| CBZE-05 | U-mode menvcfg.CBZE=1 但 senvcfg.CBZE=0 | menvcfg.CBZE=1, senvcfg.CBZE=0，U-mode 执行 cbo.zero | illegal-instruction exception (cause=2) |
| CBZE-06 | U-mode 两级 CBZE 均为 1 正常执行 | menvcfg.CBZE=1, senvcfg.CBZE=1，U-mode 执行 cbo.zero | 正常执行 |
| CBZE-07 | 各 envcfg CBZE 字段互不影响 | 写 menvcfg.CBZE=0，检查 senvcfg.CBZE | 其他 envcfg 的 CBZE 不受影响 |

---

## Group 4. Zicbom 功能测试 — cbo.inval / cbo.clean / cbo.flush

**规范依据**：
- `norm:cbo-inval_op`：cbo.inval 对 rs1 地址所在 cache block 执行 invalidate
- `norm:cbo-inval_unaligned`：rs1 不要求对齐，fault 时报告 rs1 值
- `norm:cbo-clean_op`：cbo.clean 对 rs1 基地址所在 cache block 执行 clean
- `norm:cbo-clean_offset`：offset 必须为零
- `norm:cbo-flush_op`：cbo.flush 对包含 rs1 地址的 cache block 执行 flush
- `norm:cbo-flush_unaligned`：rs1 不要求对齐，fault 时报告 rs1 值
- `norm:cache_block_size`：cache block 大小整个系统统一

**测试职责**：验证 Zicbom 三条指令的基本功能语义、对齐行为和 cache block 操作效果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| CBM-FUNC-01 | cbo.clean 基本功能 | 写 dirty 数据到 cache block，执行 cbo.clean，通过非缓存路径验证内存已更新 | 内存中数据与 cache 中一致（clean 写回） |
| CBM-FUNC-02 | cbo.flush 基本功能 | 写 dirty 数据到 cache block，执行 cbo.flush，验证数据写回且 cache 无效化 | 内存数据正确，后续 load 从内存获取 |
| CBM-FUNC-03 | cbo.inval 基本功能（CBIE=11） | 设 CBIE=11，写数据到 cache block，外部修改内存，执行 cbo.inval 后 load | load 获取到外部修改后的值（cache 已无效化） |
| CBM-FUNC-04 | cbo.inval 执行 flush（CBIE=01） | 设 CBIE=01，写 dirty 数据，执行 cbo.inval | 数据被写回内存（flush 语义） |
| CBM-FUNC-05 | cbo.clean rs1 未对齐 | rs1 = cache_block_base + 1，执行 cbo.clean | 正常执行（不产生 misaligned 异常），操作包含 rs1 的 cache block |
| CBM-FUNC-06 | cbo.flush rs1 未对齐 | rs1 = cache_block_base + 3，执行 cbo.flush | 正常执行，操作包含 rs1 的 cache block |
| CBM-FUNC-07 | cbo.inval rs1 未对齐 | rs1 = cache_block_base + 7，执行 cbo.inval | 正常执行，操作包含 rs1 的 cache block |
| CBM-FUNC-08 | cbo.clean offset 必须为零 | 使用非零 offset 编码执行 cbo.clean | 行为：offset 字段被忽略或指令正常（编码约束） |
| CBM-FUNC-09 | cbo.flush offset 必须为零 | 使用非零 offset 编码执行 cbo.flush | 行为：offset 字段被忽略或指令正常（编码约束） |
| CBM-FUNC-10 | cbo.inval offset 必须为零 | 使用非零 offset 编码执行 cbo.inval | 行为：offset 字段被忽略或指令正常（编码约束） |
| CBM-FUNC-11 | cbo.clean 对 clean block 无副作用 | 对未修改的 cache block 执行 cbo.clean | 不产生 write transfer（数据未 dirty） |
| CBM-FUNC-12 | cbo.flush 后 load 重新从内存获取 | 写数据，cbo.flush，外部修改内存，再 load | load 获取外部修改后的值 |
| CBM-FUNC-13 | cbo.inval 后 load 重新从内存获取 | CBIE=11，写数据，cbo.inval，外部修改内存，再 load | load 获取外部修改后的值 |
| CBM-FUNC-14 | cache block 大小发现 | 通过执行环境发现机制获取 cache block 大小 | 返回合理的 power-of-2 值 |

---

## Group 5. Zicboz 功能测试 — cbo.zero

**规范依据**：
- `norm:cbo-zero_op`：cbo.zero 对包含 rs1 地址的 cache block 全部字节写零
- `norm:cbo-zero_specified_block`：操作 rs1 有效地址对应的 cache block
- `norm:cbo-zero_unaligned`：rs1 不要求对齐，fault 时报告 rs1 值
- `norm:cbo-zero_offset`：offset 必须为零
- `norm:cache_block_size`：cache block 大小整个系统统一

**测试职责**：验证 cbo.zero 指令的写零功能、对齐行为和 cache block 范围。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| CBZ-FUNC-01 | cbo.zero 基本写零功能 | 先写入非零 pattern 到 cache block，执行 cbo.zero，读回验证 | cache block 全部字节为零 |
| CBZ-FUNC-02 | cbo.zero 覆盖整个 cache block | 写满整个 cache block 非零数据，cbo.zero，逐字节检查 | 所有字节均为 0 |
| CBZ-FUNC-03 | cbo.zero rs1 未对齐 | rs1 = cache_block_base + 5，执行 cbo.zero | 正常执行，整个 cache block 被写零（不产生 misaligned 异常） |
| CBZ-FUNC-04 | cbo.zero 仅操作一个 cache block | 相邻两个 cache block 写入非零，cbo.zero 其中一个 | 目标 block 全零，相邻 block 数据不变 |
| CBZ-FUNC-05 | cbo.zero offset 必须为零 | 使用非零 offset 编码执行 cbo.zero | 行为：offset 字段被忽略或指令正常（编码约束） |
| CBZ-FUNC-06 | cbo.zero 对已为零的 block 执行 | cache block 已全零，执行 cbo.zero | 正常执行，无异常 |
| CBZ-FUNC-07 | cbo.zero 后 load 返回零 | 写非零数据，cbo.zero，load 同一地址 | load 返回 0 |
| CBZ-FUNC-08 | cbo.zero cache block 大小验证 | 确定 cache block 大小 N，写 N 字节非零，cbo.zero | 恰好 N 字节被清零 |

---

## Group 6. Zicbop 功能测试 — prefetch.r / prefetch.w / prefetch.i

**规范依据**：
- `norm:prefetch_operating_block`：有效地址 = rs1 + sign-extended offset，imm[4:0] 必须为 0
- `norm:prefetch-i_op`：prefetch.i 提示即将进行指令取指
- `norm:prefetch-r_op`：prefetch.r 提示即将进行数据读取
- `norm:prefetch-w_op`：prefetch.w 提示即将进行数据写入
- `norm:cbp_access`：load/store/fetch 任一允许时 prefetch 即允许
- `norm:cbp_unperm_noexcep`：访问不允许时不引发异常，不访问 cache/memory
- `norm:cbp_unperm_translate`：不检查 accessed/dirty 位

**测试职责**：验证 prefetch 指令的 HINT 语义、有效地址计算、无异常特性和翻译行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| CBP-FUNC-01 | prefetch.r 基本执行不触发异常 | 对合法地址执行 prefetch.r | 正常执行，无异常 |
| CBP-FUNC-02 | prefetch.w 基本执行不触发异常 | 对合法地址执行 prefetch.w | 正常执行，无异常 |
| CBP-FUNC-03 | prefetch.i 基本执行不触发异常 | 对合法地址执行 prefetch.i | 正常执行，无异常 |
| CBP-FUNC-04 | prefetch 有效地址 = rs1 + offset | 设 rs1=base, offset=64，执行 prefetch.r | 操作 base+64 对应的 cache block（HINT 语义） |
| CBP-FUNC-05 | prefetch offset 为负值 | 设 offset=-64（sign-extended），执行 prefetch.r | 正常执行，操作 rs1-64 对应的 cache block |
| CBP-FUNC-06 | prefetch 对无效地址不触发异常 | 对未映射地址执行 prefetch.r | 不触发任何异常（HINT 语义） |
| CBP-FUNC-07 | prefetch 对无权限地址不触发异常 | 对 PMP 禁止的地址执行 prefetch.w | 不触发任何异常，不访问 cache/memory |
| CBP-FUNC-08 | prefetch 不检查 accessed 位 | 页表 A=0 的地址，执行 prefetch.r | 不触发异常，A 位不被设置 |
| CBP-FUNC-09 | prefetch 不检查 dirty 位 | 页表 D=0 的地址，执行 prefetch.w | 不触发异常，D 位不被设置 |
| CBP-FUNC-10 | prefetch 不引发 illegal-instruction | 在任何特权级执行 prefetch.r/w/i | 均不触发 illegal-instruction exception |
| CBP-FUNC-11 | prefetch 编码为 ORI rd=0 格式 | 验证 prefetch 指令编码（opcode=0x13, funct3=6, rd=0） | 编码正确，作为 HINT 执行 |
| CBP-FUNC-12 | prefetch imm[4:0] 必须为零 | 验证编码中 imm[4:0] 字段（即 rd 字段）为 0 | 编码约束满足 |
| CBP-FUNC-13 | prefetch 不受 CBIE/CBCFE/CBZE 控制 | 设所有 envcfg CBO 字段为 0，执行 prefetch.r/w/i | 正常执行，不受 CBO 控制位影响 |

---

## Group 7. CMO 公共 Trap 行为

**规范依据**：
- `norm:cbm_access`：management 指令在 load/store 允许时允许访问
- `norm:cbm_unperm_fault`：不允许时引发 store page-fault / store guest-page-fault / store access-fault
- `norm:cbm_unperm_translate`：检查 accessed 位
- `norm:cbz_access`：cbo.zero 在 store 允许且 PMA 支持时允许
- `norm:cbz_unperm_fault`：不允许时引发 store page-fault / store guest-page-fault / store access-fault
- `norm:cbz_unperm_translate`：检查 accessed 和 dirty 位
- `norm:fault_excep_csr`：*tval 写入故障有效地址（rs1 值）
- `norm:no_addr_misaligned_excep`：CMO 指令不产生地址未对齐异常
- `norm:PMP_same`：cache block 内 PMP 位必须一致
- `norm:PMA_same`：cache block 内 PMA 必须一致
- `norm:cbo_rsv`：约束不满足时行为 UNSPECIFIED

**测试职责**：验证 CMO 指令在地址翻译、PMP、PMA 等保护机制下的异常行为和 tval 写入。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| CMO-TRAP-01 | cbo.clean store page-fault | 页表无读写权限，HS-mode 执行 cbo.clean | store page-fault exception (cause=15) |
| CMO-TRAP-02 | cbo.flush store page-fault | 页表无读写权限，HS-mode 执行 cbo.flush | store page-fault exception (cause=15) |
| CMO-TRAP-03 | cbo.inval store page-fault | 页表无读写权限，HS-mode 执行 cbo.inval | store page-fault exception (cause=15) |
| CMO-TRAP-04 | cbo.zero store page-fault（无写权限） | 页表只读（无写权限），执行 cbo.zero | store page-fault exception (cause=15) |
| CMO-TRAP-05 | cbo.clean stval 写入 rs1 值 | 触发 page-fault，检查 stval | stval = rs1 的值（故障有效地址） |
| CMO-TRAP-06 | cbo.flush stval 写入 rs1 值 | 触发 page-fault，检查 stval | stval = rs1 的值 |
| CMO-TRAP-07 | cbo.inval stval 写入 rs1 值 | 触发 page-fault，检查 stval | stval = rs1 的值 |
| CMO-TRAP-08 | cbo.zero stval 写入 rs1 值 | 触发 page-fault，检查 stval | stval = rs1 的值 |
| CMO-TRAP-09 | cbo.clean 未对齐地址不触发 misaligned | rs1 未对齐，页表有权限，执行 cbo.clean | 不触发 address-misaligned exception |
| CMO-TRAP-10 | cbo.zero 未对齐地址不触发 misaligned | rs1 未对齐，页表有写权限，执行 cbo.zero | 不触发 address-misaligned exception |
| CMO-TRAP-11 | cbo.flush 未对齐地址不触发 misaligned | rs1 未对齐，页表有权限，执行 cbo.flush | 不触发 address-misaligned exception |
| CMO-TRAP-12 | cbo.inval 未对齐地址不触发 misaligned | rs1 未对齐，页表有权限，执行 cbo.inval | 不触发 address-misaligned exception |
| CMO-TRAP-13 | cbo.clean 检查 accessed 位 | 页表 A=0，执行 cbo.clean | 引发 page-fault 或硬件设置 A 位 |
| CMO-TRAP-14 | cbo.zero 检查 accessed 和 dirty 位 | 页表 A=0 D=0，执行 cbo.zero | 引发 page-fault 或硬件设置 A/D 位 |
| CMO-TRAP-15 | cbo.clean 不检查 dirty 位 | 页表 A=1 D=0，执行 cbo.clean | 不因 D=0 触发异常（management 不检查 dirty） |
| CMO-TRAP-16 | cbo.zero store access-fault | PMP 禁止写访问，执行 cbo.zero | store access-fault exception (cause=7) |
| CMO-TRAP-17 | cbo.clean store access-fault | PMP 禁止所有访问，执行 cbo.clean | store access-fault exception (cause=7) |
| CMO-TRAP-18 | cbo.flush store access-fault | PMP 禁止所有访问，执行 cbo.flush | store access-fault exception (cause=7) |
| CMO-TRAP-19 | cbo.inval store access-fault | PMP 禁止所有访问，执行 cbo.inval | store access-fault exception (cause=7) |
| CMO-TRAP-20 | cbo.flush mtval/stval 为 rs1 值（M-mode trap） | M-mode 接收 trap，检查 mtval | mtval = rs1 值 |
| CMO-TRAP-21 | cbo.clean 有读权限但无写权限可执行 | 页表只读（有读无写），执行 cbo.clean | 正常执行（management 只需 load 或 store 任一允许） |
| CMO-TRAP-22 | cbo.inval 有读权限但无写权限可执行 | 页表只读（有读无写），执行 cbo.inval | 正常执行 |
| CMO-TRAP-23 | cbo.zero 仅有读权限触发 page-fault | 页表只读（有读无写），执行 cbo.zero | store page-fault（cbo.zero 需要 store 权限） |

---

## Group 8. CMO 内存序（Memory Ordering）

**规范依据**：
- `norm:PPO_overlap_1`：若 invalidate/clean/flush 在程序序中先于重叠地址的 load，则在全局内存序中也先于该 load
- Prefetch 指令不受 PPO 规则排序，也不受任何排序指令约束
- cbo.zero 产生的 store 操作按 PPO 中 store 规则排序

**测试职责**：验证 CMO 指令在 RVWMO 内存模型下的排序行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| CMO-MO-01 | cbo.inval 先于重叠地址 load 保序 | 程序序：store → cbo.inval → load 同地址 | load 不会在全局序中先于 cbo.inval（PPO 规则） |
| CMO-MO-02 | cbo.clean 先于重叠地址 load 保序 | 程序序：store → cbo.clean → load 同地址 | load 不会在全局序中先于 cbo.clean |
| CMO-MO-03 | cbo.flush 先于重叠地址 load 保序 | 程序序：store → cbo.flush → load 同地址 | load 不会在全局序中先于 cbo.flush |
| CMO-MO-04 | cbo.zero 按 store 排序规则 | 程序序：cbo.zero → fence w,w → store 同地址 | cbo.zero 的 store 操作在全局序中先于后续 store |
| CMO-MO-05 | prefetch 不受 PPO 排序 | 程序序：prefetch.r → store 同地址 | prefetch 操作在全局序中无排序约束 |
| CMO-MO-06 | management 操作按 store 分类参与 fence | fence 指令的 predecessor/successor set 包含 management 操作 | management 操作被 fence 正确排序（分类为 W 或 O） |
| CMO-MO-07 | management 操作不被 fence.i 排序 | 验证 fence.i 不对 cbo.clean/flush/inval 产生排序 | fence.i 不影响 management 操作顺序 |
| CMO-MO-08 | management 操作不被 sfence.vma 排序 | 验证 sfence.vma 不对 management 操作产生排序 | sfence.vma 不影响 management 操作顺序 |

---

## Group 9. CMO 指令编码验证

**规范依据**：
- Zicbom 编码：opcode=0x0F (MISC-MEM), funct3=2, rs1=base, operation 字段区分指令
  - cbo.inval: operation=0x000
  - cbo.clean: operation=0x001
  - cbo.flush: operation=0x002
- Zicboz 编码：opcode=0x0F (MISC-MEM), funct3=2, rs1=base
  - cbo.zero: operation=0x004
- Zicbop 编码：opcode=0x13 (OP-IMM), funct3=6 (ORI), rd=0 (imm[4:0]=0)
  - prefetch.i: imm[11:5] 中 funct5=0x0
  - prefetch.r: imm[11:5] 中 funct5=0x1
  - prefetch.w: imm[11:5] 中 funct5=0x3

**测试职责**：验证 CMO 指令编码的正确性和非法编码的异常行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| CMO-ENC-01 | cbo.inval 编码正确性 | 构造 cbo.inval 机器码 0x0000200F（rs1=x0）执行 | 指令被正确识别为 cbo.inval |
| CMO-ENC-02 | cbo.clean 编码正确性 | 构造 cbo.clean 机器码 0x0010200F（rs1=x0）执行 | 指令被正确识别为 cbo.clean |
| CMO-ENC-03 | cbo.flush 编码正确性 | 构造 cbo.flush 机器码 0x0020200F（rs1=x0）执行 | 指令被正确识别为 cbo.flush |
| CMO-ENC-04 | cbo.zero 编码正确性 | 构造 cbo.zero 机器码 0x0040200F（rs1=x0）执行 | 指令被正确识别为 cbo.zero |
| CMO-ENC-05 | prefetch.i 编码正确性 | 构造 prefetch.i 机器码（ORI rd=0, funct5=0x0）执行 | 指令被正确识别为 prefetch.i |
| CMO-ENC-06 | prefetch.r 编码正确性 | 构造 prefetch.r 机器码（ORI rd=0, funct5=0x1）执行 | 指令被正确识别为 prefetch.r |
| CMO-ENC-07 | prefetch.w 编码正确性 | 构造 prefetch.w 机器码（ORI rd=0, funct5=0x3）执行 | 指令被正确识别为 prefetch.w |
| CMO-ENC-08 | Zicbom 未实现时触发 illegal-instruction | 若 Zicbom 未实现，执行 cbo.clean | illegal-instruction exception (cause=2) |
| CMO-ENC-09 | Zicboz 未实现时触发 illegal-instruction | 若 Zicboz 未实现，执行 cbo.zero | illegal-instruction exception (cause=2) |
| CMO-ENC-10 | 非法 operation 编码触发异常 | opcode=0x0F, funct3=2, operation=0x003（未定义） | illegal-instruction exception |
| CMO-ENC-11 | 非法 operation 编码 0x005-0xFFF | opcode=0x0F, funct3=2, operation=0x005 | illegal-instruction exception |
