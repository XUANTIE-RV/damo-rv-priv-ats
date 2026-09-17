**中文 | [English](../testplan_en/Hypervisor_Za_test_plan_en.md)**

# Hypervisor 与 Za 原子扩展交叉测试计划

> 本文档描述 Hypervisor（H）扩展与 Za 系列原子/保留集扩展（Zalrsc、Zaamo、Zacas、Zabha、Zalasr、Zawrs）在交叉场景下的测试计划。

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/unpriv/zalrsc.adoc` — Zalrsc 扩展：`lr.w`/`lr.d`/`sc.w`/`sc.d` 指令语义、SC 成功/失败与退休权限检查、失败 SC 的副作用与按 store 保护、自然对齐约束、aq/rl 排序注解软件规则、受约束 LR/SC 循环的前向进展保证
- `SPEC/riscv-isa-manual/src/unpriv/zaamo.adoc` — Zaamo 扩展：`amoswap`/`amoadd`/`amoand`/`amoor`/`amoxor`/`amomin`/`amomax`/`amominu`/`amomaxu` 的 `.w`/`.d` AMO 指令语义、操作数宽度与 RV64 `.w` 符号扩展、自然对齐约束与 misaligned atomicity granule（MAG）放宽、aq/rl release consistency 语义
- `SPEC/riscv-isa-manual/src/unpriv/zacas.adoc` — Zacas 扩展：`amocas.w/d/q` 比较交换语义、RV32/RV64 寄存器对编码约束、`rs1` 对齐（跨引用 Zaamo 异常选项与 MAG）、`amocas` 恒需写权限
- `SPEC/riscv-isa-manual/src/unpriv/zabha.adoc` — Zabha 扩展：字节/半字 AMO 与 `amocas.b/h` 语义、rd 符号扩展与忽略 rs2 高位、rs1 对齐（字节 AMO 恒对齐）、无字节/半字 `lr`/`sc`（保留编码）
- `SPEC/riscv-isa-manual/src/unpriv/zalasr.adoc` — Zalasr 扩展：load-acquire/store-release 纯加载/纯存储语义、rd 符号扩展与忽略 rs2 高位、恒带 aq/rl 与保留编码、自然对齐与 MAG 放宽、`ld.aq`/`sd.rl` 仅 RV64、可独立实现
- `SPEC/riscv-isa-manual/src/unpriv/zawrs.adoc` — Zawrs 扩展：`wrs.nto`/`wrs.sto` 等待保留集指令、`hstatus.VTW` 门控的 virtual-instruction 机制、`mstatus.TW` 优先级、停顿终止许可
- `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — Hypervisor（H）扩展：`hedeleg` 委托位属性、两阶段翻译与 guest-page fault、`htval`/`htinst` 陷阱值与 transformed/伪指令规则、`hstatus.GVA`/`SPV`/`SPVP`、HLV/HLVX/HSV 虚拟机访存指令、`henvcfg.FIOM`/`ADUE`
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — `mcause` 异常编码归类：store、store-conditional 与 AMO 指令均生成 store/AMO 异常类（cause 6/7/15/23），load 与 load-reserved 生成 load 类（cause 4/5/13/21）
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — 页级权限与异常归类：load-reserved 需读权限（→ load page-fault）、store-conditional 与 AMO 需写权限（→ store page-fault，AMO/SC 永不报 load page-fault）；硬件 A/D 更新的原子性要求、aq/rl 对 PTE 更新排序的约束、页表内存须具备 RsrvEventual PMA
- `SPEC/riscv-isa-manual/src/unpriv/rvwmo.adoc` — RVWMO 内存模型：Atomicity Axiom、acquire-RCsc/release-RCsc 排序注解与 Preserved Program Order 规则（作为原子性与 aq/rl 语义的可测展开）

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内上述路径文件）

---

## 范围

### 覆盖的扩展交叉

- **Hypervisor × Zalrsc**：LR/SC 在 HS/VS/VU-mode 的正常执行与 virtual-instruction (cause=22) 排除、LR 归 load 类而 SC 归 store/AMO 类的异常二分、VS-stage 故障可委托与 G-stage 故障强制陷入 HS-mode、`htinst` transformed/伪指令消歧、`hstatus.GVA`/`SPV` 组合与 `htval` 取值差异、失败 SC 仍受写权限检查、未对齐 LR/SC 与 Addr. Offset 恒 0、`henvcfg.FIOM`/`ADUE` 交互、受约束循环前向进展、H 扩展无 LR/SC 虚拟机等价指令
- **Hypervisor × Zaamo**：AMO（9 条 × `.w`/`.d`）正常执行与 virtual-instruction 排除、AMO 异常统一归 store/AMO 类、AMO 需 R+W 权限、VS-stage/G-stage 故障委托路径二分、隐式遍历故障报 cause=23、`htinst` transformed/伪指令消歧、`hstatus.GVA`/`SPV` 与 `htval`、未对齐 AMO 的 MAG 放宽（Zaamo 独有）、ADUE=0 时 D 位缺失报 store page-fault、`henvcfg.FIOM` 排序、H 扩展无 AMO 虚拟机等价指令
- **Hypervisor × Zacas**：`amocas.w/d/q` 正常执行与 virtual-instruction 排除、amocas 异常归 store/AMO 类、`norm:Zacas_amocas_w_permission` 无条件写权限检查（比较失败仍报 store 类）、VS-stage/G-stage 委托路径二分、`htinst` 保留 funct5=00101 的 transformed 格式、未对齐 amocas 的 MAG 放宽、ADUE=0 时成功 amocas 报 store page-fault、ADUE=1 时失败 amocas 的 D 位副作用（SPEC 空白点，记录型）、`henvcfg.FIOM` 排序、清零 `hstateen0` 后 VS/VU-mode 仍执行 amocas（HZACAS-36）
- **Hypervisor × Zabha**：字节/半字 AMO（funct3=000/001）正常执行与 virtual-instruction 排除、异常归 store/AMO 类、需 R+W 权限、VS-stage/G-stage 委托路径二分、`htinst` 须保留 funct3 宽度编码（Zabha 特有）、字节 AMO 恒对齐（无 MAG 分支）而未对齐仅半字 AMO 可测、rd 符号扩展与忽略 rs2 高位、`amocas.b/h`（Zabha × Zacas 交集）、保留字节/半字 lr/sc 编码报 illegal-instruction (cause=2)、HLV.B/H 等非原子字节/半字 guest 访存的架构边界
- **Hypervisor × Zalasr**：load-acquire/store-release 正常执行与 virtual-instruction 排除、异常二分（load-acquire→load 类仅需读权限、store-release→store/AMO 类需写权限）、load-acquire 到 R=1/W=0 页必正常执行、VS-stage/G-stage 委托路径二分、`htinst` 统一走 opcode 0x2F transformed atomic 格式、未对齐的 MAG 放宽、store-release 恒写使 ADUE=0 报 store page-fault、`henvcfg.FIOM` 对恒带 aq/rl 指令的排序、保留编码（load 无 aq/store 无 rl）报 illegal-instruction (cause=2)、H 扩展无原子有序 load/store 等价指令
- **Hypervisor × Zawrs**：HS/VS/VU-mode 下 `wrs.nto`/`wrs.sto` 的正常执行（不得误触发 virtual-instruction exception）、VS/VU-mode 下 `hstatus.VTW` 对 `wrs.nto` 的 virtual-instruction 门控、`mstatus.TW` 优先于 `hstatus.VTW` 的异常类型判定、VTW 条款仅作用于 `wrs.nto` 而不作用于 `wrs.sto`、VTW 异常的 trap 报告（stval/SPV）

### 不在本文档范围

- Zalrsc 非 Hypervisor 场景（LR/SC 指令编码、reservation set 登记与形态、SC 成功/失败基础语义与失败码、配对与必须失败情形、非虚拟化下的对齐与权限异常、aq/rl 数据语义、受约束循环构造自检、M/S/U 各特权级基础可执行性）— 由 `Zalrsc_test_plan.md` 覆盖
- Zawrs 非 Hypervisor 场景（指令编码与可用性、停顿与恢复语义、`mstatus.TW` 超时 illegal-instruction 行为等）— 由 `Zawrs_test_plan.md` 覆盖
- 主存区域 RsrvEventual PMA 与受约束 LR/SC 循环前向进展的主存保证 — 由 `Ziccrse_test_plan.md` 覆盖；本文档 HZLRSC-35 仅验证 VS-mode 下的前向进展路径，页表内存的 RsrvEventual 要求作为前置条件引用
- `hstatus.VTW`/`mstatus.TW` 对 WFI 的基础门控语义（HSTAT-04/06）— 由 `Hypervisor_CSR_test_plan.md` 覆盖；本文档 Group 6 仅将其作为 Zawrs 的对照引用
- Hypervisor 与 Zkr/Zihintntl/Zcmt/V 向量族/Zicntr/Zihpm 等非原子扩展的交叉 — 由 `Hypervisor_Zi_test_plan.md` 覆盖
- 已由 `Hypervisor_CSR_test_plan.md`、`Hypervisor_Interrupts_test_plan.md`、`Hypervisor_Exceptions_test_plan.md`、`Hypervisor_2_stage_test_plan.md`、`Hypervisor_gstage_test_plan.md` 覆盖的 Hypervisor 基础功能
- Zaamo/Zacas/Zabha/Zalasr 的非虚拟化语义 — 分别由 `Zaamo_test_plan.md`、`Zacas_test_plan.md`、`Zabha_test_plan.md`、`Zalasr_test_plan.md` 覆盖（其与 Hypervisor 的交叉已分别由本文档 Group 2/3/4/5 覆盖）
- 主存 AMO 支持等级 PMA（Ziccamoa/Ziccamoc）— 由 `Ziccamoa_test_plan.md`、`Ziccamoc_test_plan.md` 覆盖

---

## 覆盖的规范点

下表列出本方案覆盖的规范点。带 `norm:` 前缀的为 SPEC 官方标签；不带前缀的为根据 SPEC 原文拆解的规范点。

### Zalrsc 相关（`zalrsc.adoc`）

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:lr_w_op` | `zalrsc.adoc` | lr.w loads a word from the address in rs1, places the sign-extended value in rd, and registers a reservation set: a set of bytes that subsumes the bytes in the addressed word. | `lr.w` 从 rs1 加载一个字、符号扩展写入 rd 并登记至少覆盖被寻址字的 reservation set；虚拟化下该语义在 VS/VU-mode 不变。 |
| `norm:sc_w_success` | `zalrsc.adoc` | sc.w succeeds only if the reservation is still valid and the reservation set contains the bytes being written; on success it writes rs2 to memory and zero to rd. | SC 仅在 reservation 有效且覆盖被写字节时成功，成功时写内存并向 rd 写零；VS-mode 下语义不变。 |
| `norm:sc_w_failure` | `zalrsc.adoc` | If the sc.w fails, the instruction does not write to memory, and it writes a nonzero value to rd. | SC 失败时不写内存、向 rd 写非零值。 |
| `norm:sc_retire_permission` | `zalrsc.adoc` | No sc.w instruction shall retire unless it passes memory permission checks. | SC 未通过内存权限检查不得退休；V=1 时须同时通过 VS-stage 与 G-stage 权限检查，不足时抛异常而非静默失败。 |
| `norm:sc_failed_side_effects` | `zalrsc.adoc` | It is UNSPECIFIED whether any side effects of implicit address translation and protection memory accesses (such as setting a page-table entry D bit) occur on a failed sc.w. | 失败 SC 是否产生隐式翻译/保护访问副作用（如置 PTE D 位）为 UNSPECIFIED；与 `henvcfg.ADUE` 叠加构成双重不确定性（记录型用例）。 |
| `norm:sc_failed_as_store` | `zalrsc.adoc` | For the purposes of memory protection, a failed sc.w may be treated like a store. | 就内存保护而言失败 SC 可按 store 处理；结合 `norm:sc_retire_permission` 与 `norm:store_page_fault_no_w`，无 reservation 的 SC 指向无写权限页仍须报 store 类异常。 |
| `norm:sc_reservation_invalidate` | `zalrsc.adoc` | Regardless of success or failure, executing an sc.w instruction invalidates any reservation held by this hart. | 无论成功失败，执行 SC 均作废本 hart 的 reservation；虚拟化下 trap 至 HS-mode 后 guest reservation 的存续由实现决定。 |
| `norm:lr_sc_rv64` | `zalrsc.adoc` | lr.d and sc.d act analogously on doublewords and are only available on RV64. For RV64, lr.w and sc.w sign-extend the value placed in rd. | `lr.d`/`sc.d` 仅 RV64 可用；RV64 下 `lr.w`/`sc.w` 写入 rd 的值符号扩展。VS/VU-mode 下 `.w`/`.d` 双宽度均须正常执行。 |
| `norm:lr_sc_alignment` | `zalrsc.adoc` | For lr and sc, the address held in rs1 must be naturally aligned to the size of the operand; if not, an address-misaligned exception or an access-fault exception will be generated. | LR/SC 要求 rs1 自然对齐，未对齐必产生地址未对齐或访问错误异常（**无 MAG 放宽**，与 Zalasr 不同）；故 LR/SC 不做未对齐拆分访问，`htinst` 的 Addr. Offset 恒为 0。 |
| `norm:lr_sc_aq_rl_software_rule` | `zalrsc.adoc` | Software should not set rl on lr unless aq is also set, nor aq on sc unless rl is also set; lr.rl and sc.aq are not guaranteed stronger ordering. | LR/SC 的 aq/rl 位软件规则；aq/rl 位在 `htinst` transformed 值中必须保留，且受 `henvcfg.FIOM` 在 V=1 时的排序修改。 |
| `norm:lrsc_eventuality_region` | `zalrsc.adoc` | The lr and sc addresses must lie within a memory region with the lr/sc eventuality property. | LR/SC 地址须位于具备 eventuality 属性的区域，是前向进展保证的前提。 |
| `norm:constrained_lrsc_forward_progress_intro` | `zalrsc.adoc` | If a hart H enters a constrained lr/sc loop, the execution environment must guarantee that one of the following events eventually occurs. | 受约束循环的前向进展保证；VS-mode 下该保证同样成立。 |
| `norm:constrained_lrsc_forward_progress_trap` | `zalrsc.adoc` | H traps. | 前向进展事件之一为“H 发生 trap”；这使 guest 受约束循环被 hypervisor 抢占打断时保证即已满足，是 Zalrsc 与虚拟化兼容的关键。 |
| `norm:unconstrained_lrsc_no_progress` | `zalrsc.adoc` | Unconstrained lr/sc sequences might succeed on some attempts on some implementations, but might never succeed on other implementations. | 不受约束序列允许永不成功，故不设成功断言（记录型）。 |

### Zaamo 相关（`zaamo.adoc`）

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `amo_rmw_semantics` | `zaamo.adoc`（AMO 通用读改写语义，SPEC 未标注 norm） | The AMO instructions atomically load a data value from the address in rs1, place the value into register rd, apply a binary operator to the loaded value and the original value in rs2, then store the result back to the original address in rs1. | AMO 原子地读旧值入 rd、对旧值与 rs2 运算、写回同一地址；9 条操作（swap/add/and/or/xor/min/max/minu/maxu）×（.w/.d）。原子性在 V=1 与两阶段翻译下不变。 |
| `norm:amo_operand_size` | `zaamo.adoc` | AMOs can either operate on doublewords (RV64 only) or words in memory. For RV64, 32-bit AMOs always sign-extend the value placed in rd, and ignore the upper 32 bits of the original value of rs2. | AMO 操作字或（仅 RV64）双字；RV64 下 `.w` AMO 写入 rd 的旧值符号扩展、忽略 rs2 高 32 位。该数据语义在 VS/VU-mode 下不变。 |
| `norm:amo_alignment` | `zaamo.adoc` | For AMOs, the Zaamo extension requires that the address held in rs1 be naturally aligned to the size of the operand; if not, an address-misaligned exception or an access-fault exception will be generated. | AMO 要求 rs1 自然对齐，未对齐产生地址未对齐或访问错误异常；因 AMO 归 store/AMO 类，未对齐报 cause=6/7（**非** load 类 4/5）。 |
| `norm:misaligned_atomicity_granule_size` | `zaamo.adoc` | If all accessed bytes lie within the same misaligned atomicity granule, the instruction will not raise an exception for reasons of address alignment, and will give rise to only one memory operation--i.e., it will execute atomically. | MAG 粒度内的未对齐 AMO 无对齐异常且作为**单一内存操作**原子执行（Zaamo 独有放宽，Zalrsc 无）；单一内存操作 ⇒ htinst 的 Addr. Offset 恒为 0。 |
| `norm:amo_release_consistency` | `zaamo.adoc` | If the aq bit is set, no later memory operations in this hart can be observed to take place before the AMO; if the rl bit is set, other harts will not observe the AMO before preceding accesses; setting both makes the sequence sequentially consistent. | AMO 的 aq/rl 提供 release consistency 语义；aq/rl 位在 htinst transformed 值中保留，且受 `henvcfg.FIOM` 在 V=1 时的排序修改。 |

### Zacas 相关（`zacas.adoc`）

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:Zacas_rv64_amocas-d_op`（代表核心 CAS 语义，其余宽度/寄存器对变体见 `Zacas_test_plan.md`） | `zacas.adoc` | For RV64, amocas.d atomically loads 64-bits of a data value from address in rs1, compares the loaded value to a 64-bit value held in rd, and if the comparison is bitwise equal, then stores the 64-bit value held in rs2 to the original address in rs1. The value loaded from memory is placed into register rd. | `amocas.d`（RV64）原子加载 64 位数据与 rd 比较，逐位相等则写入 rs2；内存加载值放入 rd。`amocas.w`/`amocas.q` 及 RV32 寄存器对形态的完整指令级语义（编码约束、x0 配对规则）由 `Zacas_test_plan.md` 覆盖，本组仅以本条为代表验证 CAS 核心语义在 VS/VU-mode 下不变。 |
| `norm:Zacas_amocas_w_permission` | `zacas.adoc` | An amocas.w/d/q instruction always requires write permissions. | `amocas.w/d/q` **始终**需要写权限（无条件语言，不区分比较成功/失败）——与 `norm:store_page_fault_no_w`（AMO 需写权限 → store page-fault）结合，构成 Group 3 "失败 CAS 仍需写权限检查"用例（HZACAS-07/12）的强制判定依据，本质与 Group 1 中 `norm:sc_retire_permission`/`norm:sc_failed_as_store`（失败 SC 仍需权限检查）同构。 |
| `norm:Zacas_amocas_rs1_addr_alignment` | `zacas.adoc` | amocas.w/d/q requires that the address held in rs1 be naturally aligned to the size of the operand... And the same exception options apply if the address is not naturally aligned. | `amocas` 要求 rs1 自然对齐（4/8/16 字节）；"the same exception options apply" 明确跨引用 `norm:amo_alignment`/`norm:misaligned_atomicity_granule_size`（Zaamo），故 amocas 同样适用 address-misaligned/access-fault 异常选项与 MAG 放宽分支。 |
| `norm:Zacas_amocas_mem_op_success_aq_rl` | `zacas.adoc` | The memory operation performed by an amocas.w/d/q, when successful, has acquire semantics if aq bit is 1 and has release semantics if rl bit is 1. | 比较**成功**时，`amocas` 的内存操作在 aq=1 时具 acquire 语义、rl=1 时具 release 语义；aq/rl 位在 htinst transformed 值中保留（同 `htinst_transformed_atomic`），且受 `henvcfg.FIOM` 在 V=1 时的排序修改。 |
| `norm:Zacas_amocas_mem_op_fail_aq_rl` | `zacas.adoc` | The memory operation performed by an amocas.w/d/q, when not successful, has acquire semantics if aq bit is 1 but does not have release semantics, regardless of rl. | 比较**失败**时，`amocas` 的内存操作在 aq=1 时仍具 acquire 语义，但**无论 rl 为何值都不具 release 语义**——这是 Zacas 区别于 Zaamo（`norm:amo_release_consistency` 不区分成败）的特有排序规则；排序公理本身需多 hart 压力测试支撑（见"关键注意事项"多 hart 限制），本组仅验证 aq/rl 编码变体在 V=1 下可执行且数据语义不变。 |
| `zacas_failed_cas_write_note` | `zacas.adoc`（NOTE，SPEC 未标注 norm） | An unsuccessful amocas.w/d/q may either not perform a memory write or may write back the old value loaded from memory. The memory write, if produced, does not have release semantics, regardless of rl. Irrespective of whether a write is actually performed, the instruction is treated as an AMO for the purposes of the RVWMO PPO rules. | 失败 CAS 可以不写内存，也可以写回旧值（实现自选）；无论是否实际写入，该指令在 RVWMO PPO 规则中均视为一次 AMO。**Zacas SPEC 未像 Zalrsc 的 `norm:sc_failed_side_effects` 那样明文规定"失败 CAS 是否产生隐式翻译/保护访问副作用（如置 PTE D 位）"为 UNSPECIFIED**——此为 SPEC 空白点，Group 3 据此将"ADUE=1 时失败 CAS 的 D 位副作用"（HZACAS-32/33）按记录型处理，并显式标注该差异（区别于 HZLRSC-32/33 有明文 UNSPECIFIED 依据）。 |

### Zabha 相关（`zabha.adoc`）

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:Zabha_rd_sign_extension` | `zabha.adoc` | Byte and halfword AMOs always sign-extend the value placed in rd, and ignore the XLEN-1:2^(width+3) bits of the original value in rs2. | 字节/半字 AMO 始终将写入 rd 的内存旧值符号扩展至 XLEN（字节 width=0 忽略 rs2[XLEN-1:8]、半字 width=1 忽略 rs2[XLEN-1:16]），仅 rs2 低 8/16 位参与运算。该数据语义在 VS/VU-mode 下不变（非虚拟化语义由 `Zabha_test_plan.md` 覆盖）。 |
| `norm:Zabha_amocas-BH_ignore_bits` | `zabha.adoc` | The amocas.b/h instructions similarly ignore the XLEN-1:2^(width+3) bits of the original value in rd. | `amocas.b/h` 忽略 rd 原值的 XLEN-1:2^(width+3) 位（仅 rd[7:0]/rd[15:0] 参与比较）；与 Zacas 的 `norm:Zacas_amocas_w_permission`（恒需写权限）叠加，构成 Group 4 中 `amocas.b/h`（Zabha × Zacas 交集）的比较高位忽略与失败 CAS 权限检查用例。 |
| `norm:Zabha_rs1_align_addr` | `zabha.adoc` | Similar to the AMOs specified in the Zaamo extension, the Zabha extension mandates that the address contained in the rs1 register must be naturally aligned to the size of the operand. The same exception options as specified in the Zaamo extension are applicable in cases where the address is not naturally aligned. | 字节/半字 AMO 要求 rs1 按操作数大小自然对齐；"the same exception options as Zaamo" 跨引用 `norm:amo_alignment`（address-misaligned/access-fault）与 `norm:misaligned_atomicity_granule_size`（MAG 放宽）。**字节 AMO 1 字节对齐恒成立、永不未对齐**，故未对齐/MAG 分支仅半字 AMO（奇地址）可测；因归 store/AMO 类，未对齐报 cause=6/7。 |
| `zabha_no_byte_halfword_lrsc`（Zabha NOTE，SPEC 未标注 norm） | `zabha.adoc` | Zabha omits byte and halfword support for lr and sc due to low utility. | Zabha 不提供字节/半字 `lr`/`sc`，对应 funct3=000/001 的 LR/SC 编码为保留编码；结合 `norm:H_cause_virtual_instruction`（保留编码非 HS-qualified），在 V=1 时执行报 illegal-instruction (cause=2) 而非 virtual-instruction (cause=22)。 |

### Zalasr 相关（`zalasr.adoc`）

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:zalasr_atomic_ordered` | `zalasr.adoc` | The Zalasr instructions are atomic loads and stores that support ordering annotations. | Zalasr 指令是支持排序注解的**原子加载与原子存储**（非读改写 AMO）——这是 load-acquire 归 load 类、store-release 归 store/AMO 类的功能语义基础；虚拟化下原子性与数据语义在 VS/VU-mode 不变。 |
| `norm:ldaq_atomic_load_op` | `zalasr.adoc` | This instruction loads 2^width bytes of memory from rs1 atomically and writes the result into rd. | load-acquire 仅从 rs1 原子加载 2^width 字节写入 rd（**纯读、不写内存**）；故仅需读权限（到 R=1/W=0 页必正常执行）、数据访问不置 D 位——与 LR（`norm:lr_w_op`）同为 load 语义，区别于 AMO/SC 的读改写。 |
| `norm:sdrl_atomic_store_op` | `zalasr.adoc` | This instruction stores 2^width bytes of memory from rs1 atomically.（Synopsis：atomically stores the 2^width-byte value from the low bits of rs2 to the address in rs1） | store-release 将 rs2 低位 2^width 字节原子存储到 rs1（**纯写、恒写入、无“失败”分支**）；故需写权限（W=0 页报 store 类异常）且 D 位要求为**强制**（同 Zaamo AMO，区别于失败 SC/CAS）。 |
| `norm:ldaq_atomic_load_enc` | `zalasr.adoc` | Load Acquire encoding: opcode=AMO(0x2F), funct3=width, rs2=0, rl, aq=1, funct5=00110. | load-acquire 编码：opcode=AMO(0x2F)、funct5(bits31:27)=00110、aq(bit26)=1、rl(bit25) 可选、rs2 字段固定 0、funct3=宽度；因位于 opcode 0x2F，其 `htinst` transformed 值走 `transformedatomicinst`（保留除 bits19:15 外全部字段，含 funct5=00110/aq/rl/funct3/rd/rs2=0）而非 transformedloadinst。 |
| `norm:sdrl_atomic_store_enc` | `zalasr.adoc` | Store Release encoding: opcode=AMO(0x2F), rd=0, funct3=width, rs2=src, rl=1, aq, funct5=00111. | store-release 编码：opcode=AMO(0x2F)、funct5(bits31:27)=00111、rl(bit25)=1、aq(bit26) 可选、rd 字段固定 0、funct3=宽度；其 `htinst` transformed 值同样走 `transformedatomicinst`（保留 funct5=00111/rl/aq/funct3/rd=0/rs2）而非 transformedstoreinst，HS-mode 可由 funct5 区分 load-acquire(00110)/store-release(00111)。 |
| `norm:zalasr_signext_rd` / `norm:ldaq_signext_rule` | `zalasr.adoc` | The Zalasr instructions always sign-extend the value placed in rd; if the size (2^(width+3) bits) is less than XLEN, it is sign-extended to fill the destination register. | load-acquire 对写入 rd 的加载值符号扩展（小于 XLEN 的宽度符号扩展填满 rd）；该数据语义在 VS/VU-mode 下与 HS-mode 一致（非虚拟化语义由 `Zalasr_test_plan.md` 覆盖）。 |
| `norm:zalasr_ignore_rs2_upper` | `zalasr.adoc` | ... ignore the upper bits of the value of rs2. | store-release 仅取 rs2 低 2^width 位写入内存、忽略高位；该数据语义在 VS/VU-mode 下不变。 |
| `norm:ldaq_aq_required` / `norm:sdrl_rl_required` | `zalasr.adoc` | This load must have the ordering annotation aq; this store must have ordering annotation rl. | load-acquire 恒 aq=1（bit26）、store-release 恒 rl=1（bit25）——故 Zalasr 指令**恒为带排序注解的原子指令**，`norm:henvcfg_fiom_order` 的 FIOM 排序修改对其**恒适用**（区别于 LR/SC/AMO 的 aq/rl 可选）。 |
| `norm:ldaq_no_aq_reserved` / `norm:sdrl_no_rl_reserved` | `zalasr.adoc` | The versions without the aq bit set (load) / without the rl bit set (store) are RESERVED. | 无 aq 的 load-acquire（含 load-release）与无 rl 的 store-release（含 store-acquire）编码为 RESERVED；结合 `norm:H_cause_virtual_instruction`（保留编码非 HS-qualified），在 V=1 时执行报 illegal-instruction (cause=2) 而非 virtual-instruction (cause=22)——同 Zabha 保留字节/半字 lr/sc（HZABHA-37）。 |
| `norm:zalasr_natural_align` / `norm:zalasr_misaligned_exception` | `zalasr.adoc` | The instructions require that the address held in rs1 be naturally aligned to the size in bytes (2^width); if not, an address-misaligned exception or an access-fault exception will be generated. | Zalasr 指令要求 rs1 自然对齐，未对齐产生地址未对齐或访问错误异常（**zalasr.adoc 未明文指定 load/store 子类**）；按功能语义 load-acquire 未对齐归 load 类（cause 4/5）、store-release 归 store/AMO 类（cause 6/7），load-acquire 的具体 cause 按观测记录型处理。 |
| `norm:zalasr_misaligned_pma_relax` / `norm:zalasr_misaligned_single_op` | `zalasr.adoc` | The misaligned atomicity granule PMA optionally relaxes this alignment requirement; if all accessed bytes lie within the same misaligned atomicity granule, the instruction will not raise an exception and will give rise to only one memory operation (execute atomically). | MAG 粒度内的未对齐 load-acquire/store-release 无对齐异常且作为**单一内存操作**原子执行（同 Zaamo/Zacas 的 MAG 放宽，区别于 Zalrsc 无 MAG）；单一内存操作 ⇒ htinst 的 Addr. Offset 恒为 0。 |
| `norm:ldaq_rv64_only` / `norm:sdrl_rv64_only` | `zalasr.adoc` | ld.{aq,aqrl} is RV64-only; sd.{rl,aqrl} is RV64-only. | `ld.aq`/`sd.rl`（funct3=011，8 字节=XLEN）仅 RV64 可用；RV64 下 `.b`/`.h`/`.w`/`.d` 四宽度均须在 VS/VU-mode 正常执行，RV32 平台不适用 `.d` 分支。 |
| `norm:ldaq_rcsc_semantics` / `norm:sdrl_rcsc_semantics` | `zalasr.adoc` | Load-acquire always has an acquire-RCsc annotation (and release-RCsc if rl set); store-release always has a release-RCsc annotation (and acquire-RCsc if aq set). | load-acquire 总带 acquire-RCsc（rl 置位另带 release-RCsc）、store-release 总带 release-RCsc（aq 置位另带 acquire-RCsc）；aq/rl 位在 htinst transformed 值中保留，排序公理需多 hart 支撑（见“关键注意事项”多 hart 限制），本组仅验证 aq/rl 编码变体在 V=1 下可执行且数据语义不变。 |
| `norm:zalasr_builds_on_amo` | `zalasr.adoc` | The Zalasr extension builds on the atomic support provided by the Zaamo, Zalrsc, and Zabha extensions ..., although it can be implemented independently of them. | Zalasr 可独立于 Zaamo/Zalrsc/Zabha 实现；故本组门控以 `ZALASR_SUPPORTED` 为准，**不以 A 扩展宏为前置**（区别于 Group 2/3/4 依赖 Zaamo）。 |

### 异常归类与页级权限（`machine.adoc` / `supervisor.adoc`）

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:mcause_exccode_ld_ldrsv` | `machine.adoc` | Note that load and load-reserved instructions generate load exceptions. | **load 与 load-reserved 生成 load 类异常**（cause 4/5/13/21）——load-acquire 作为纯加载（`norm:ldaq_atomic_load_op`）功能上归此类的权威依据（与 Zalrsc 的 LR 同）；注：zalasr.adoc 未明文将 load-acquire 归入本条，故其具体 cause 按观测记录型处理。 |
| `norm:mcause_exccode_st_sc_amo` | `machine.adoc` | store, store-conditional, and AMO instructions generate store/AMO exceptions. | **store、store-conditional 与 AMO 明文归入 store/AMO 异常类**（cause 6/7/15/23），而 load-reserved 归 load 类（cause 4/5/13/21）——LR/SC 异常类别二分的权威依据；store-release 作为纯存储（`norm:sdrl_atomic_store_op`）无论视为 store 还是 AMO 均归本类，故其 cause 6/7/15/23 **无歧义**。 |
| `norm:load_page_fault_no_r` | `supervisor.adoc` | Attempting to execute a load, load-reserved, or cache-block management instruction whose effective address lies within a page without read permissions raises a load page-fault exception. | **load-reserved 需读权限**，无读权限 → load page-fault (cause 13)；故 LR 到 R=1/W=0 页应正常执行。 |
| `norm:store_page_fault_no_w` | `supervisor.adoc` | Attempting to execute a store, store-conditional, AMO, or cache-block zero instruction whose effective address lies within a page without write permissions raises a store page-fault exception. NOTE: AMOs never raise load page-fault exceptions; since any unreadable page is also unwritable, an AMO on an unreadable page always raises a store page-fault. | **store-conditional 需写权限** → store page-fault (cause 15)；配套 NOTE 说明不可读页必不可写，故 SC 到不可读页亦报 cause=15 而非 13。 |
| `ptmem_rsrv_eventual` | `supervisor.adoc`（硬件 A/D 更新方案条款，SPEC 未标注 norm） | The page tables must be located in memory with hardware page-table write access and RsrvEventual PMA. | 页表须位于具备硬件页表写访问与 RsrvEventual PMA 的内存；虚拟化下 VS-stage（`vsatp`）与 G-stage（`hgatp`）页表所在的 guest 物理内存均须满足（因硬件 A/D 更新须对 PTE 做原子 RMW），作为前向进展用例的前置条件。 |

### Hypervisor 侧（`hypervisor.adoc`）

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:hlsv_op` | `hypervisor.adoc` | For every RV32I or RV64I load instruction (LB, LBU, LH, LHU, LW, LWU, LD) there is a corresponding HLV; for every RV32I or RV64I store instruction (SB, SH, SW, SD) there is a corresponding HSV. | 虚拟机 load/store 指令**仅映射 RV32I/RV64I 的基础 load/store**；LR/SC 属 Zalrsc（AMO opcode），**无虚拟机等价指令**（无 HLR/HSC）→ HS-mode 无法以 VS/VU 有效特权对 guest 内存做原子 RMW。 |
| `norm:hlsv_priv` | `hypervisor.adoc` | Each HLV/HLVX/HSV performs an explicit memory access with an effective privilege mode of VS or VU, selected by hstatus.SPVP. | HLV/HLVX/HSV 的有效特权由 `hstatus.SPVP` 选择（VS 或 VU）；该机制不适用于任何原子指令，构成架构边界用例的对照基准。 |
| `norm:hlsv_virtinst` | `hypervisor.adoc` | Attempts to execute a virtual-machine load/store instruction (HLV, HLVX, or HSV) when V=1 cause a virtual-instruction exception. | HLV/HLVX/HSV 在 V=1 时报 virtual-instruction (cause=22)；**LR/SC 无此类条款**，故 guest LR/SC 绝不报 cause=22（负向断言依据）。 |
| `norm:hstatus_gva_op` | `hypervisor.adoc` | GVA is written whenever a trap is taken into HS-mode; for any trap that writes a guest virtual address to stval, GVA=1. NOTE: GVA is redundant with SPV except when an HLV/HLVX/HSV explicit access faults (SPV=0 but GVA=1). | guest LR/SC 内存访问 trap 时 GVA=1；因 LR/SC 非虚拟机访存指令，SPV 亦为 1（与 HLV/HSV 的 SPV=0/GVA=1 形成对比）。 |
| `norm:hstatus_spv_op` | `hypervisor.adoc` | The SPV bit is written whenever a trap is taken into HS-mode, set to the value of the virtualization mode V at the time of the trap. | guest LR/SC（V=1）trap 至 HS-mode 时 SPV=1；HZWRS-07 的 VTW 异常现场同样要求 SPV=1。 |
| `norm:hedeleg_acc` | `hypervisor.adoc` | Each bit of hedeleg shall be either writable or read-only zero, as enumerated in the hedeleg-bits table. | `hedeleg` 位属性表：bit 4/5/6/7/13/15（LR/SC 的 VS-stage 故障）为 **Writable**，bit 21/23（LR/SC 的 G-stage 故障）为 **Read-only 0** → 委托路径二分的权威依据。 |
| `norm:hedeleg_op` | `hypervisor.adoc` | A synchronous trap delegated to HS-mode (using medeleg) is further delegated to VS-mode if V=1 before the trap and the corresponding hedeleg bit is set. | V=1 且对应 `hedeleg` 位置位时同步 trap 进一步委托至 VS-mode；决定 LR/SC 故障的递送目标。 |
| `norm:H_vm_gpatrans` | `hypervisor.adoc` | G-stage translation uses hgatp in place of satp; for the translation to begin, the effective privilege mode must be VS-mode or VU-mode; guest-page-fault exceptions are raised instead of regular page-fault exceptions. | G-stage 翻译仅在有效特权为 VS/VU-mode 时进行，故障报 guest-page-fault（cause 20/21/23）而非常规 page-fault → LR/SC 的 G-stage 故障必为 cause 21/23。 |
| `norm:H_vm_gpapriv` | `hypervisor.adoc` | For a memory access made to support VS-stage address translation, permissions and the need to set A/D bits at the G-stage level are checked as though for an implicit load or store, not for the original access type. However, any exception is always reported for the original access type (instruction, load, or store/AMO). | 支持 VS-stage 翻译的隐式访问按 implicit load/store 检查 G-stage 权限与 A/D，但异常**始终按原始访问类型报告**——故 AMO 的隐式遍历 GPF 报 store/AMO guest-page fault (cause=23)，而非 LR 的 load guest-page fault (cause=21)。 |
| `norm:H_trap_xtinst_val` | `hypervisor.adoc` | tinst-values table: for Load/Store-AMO address misaligned, access fault, page fault, and guest-page fault, a Transformed Standard Instruction may be written. | `tinst-values` 表确认 cause 4/5/6/7/13/15/21/23 均允许写入 Transformed Standard Instruction；guest-page fault 另允许伪指令值。 |
| `htinst_transformed_atomic` | `hypervisor.adoc`（`transformedatomicinst` 格式图，SPEC 未标注 norm） | For a standard atomic instruction (load-reserved, store-conditional, or AMO instruction), the transformed instruction has the format: all fields are the same as the trapping instruction except bits 19:15, Addr. Offset. | `htinst`/`mtinst` 对 LR/SC 的 transformed 格式：**保留原指令除 bits19:15 外的全部字段**（含 funct5、aq、rl、funct3、rd、rs2、opcode），bits19:15 ← Addr. Offset。 |
| `norm:H_trap_xtinst_guestpage` | `hypervisor.adoc` | For guest-page faults, the trap instruction register is written with a special pseudoinstruction value if (a) the fault is caused by an implicit memory access for VS-stage address translation, and (b) a nonzero faulting GPA is written to mtval2/htval. If both conditions are met, the value must be taken from the pseudoinstruction table; zero is not allowed. | LR/SC 的 VS-stage 隐式页表遍历在 G-stage 故障时，若 htval 非零则 `htinst` **必须**为伪指令、**不允许为 0**。 |
| `norm:H_trap_xtinst_guestpage_rw` | `hypervisor.adoc` | A write pseudoinstruction (0x00002020 or 0x00003020) is used when the machine is attempting automatically to update bits A and/or D in VS-level page tables. The fact that such a page table update must actually be atomic is ignored for the pseudoinstruction. | VS-level 页表 A/D 自动更新故障用**写伪指令**（RV64=0x00003020）；SPEC 明确该更新“实际必须是原子的”但伪指令忽略原子性——与 Zalrsc 原子语义的深层交汇。 |
| `norm:htval_trapval` | `hypervisor.adoc` | htval trap value reporting for guest-page faults (implementation may write zero or the faulting GPA>>2). | guest-page fault 时 htval 的故障值报告（实现允许写零或故障 GPA>>2）；非 guest-page fault 的 trap 写零。 |
| `norm:henvcfg_fiom_order` | `hypervisor.adoc` | When FIOM=1 and V=1, if an atomic instruction that accesses a region ordered as device I/O has its aq and/or rl bit set, then that instruction is ordered as though it accesses both device I/O and memory. | V=1 且 FIOM=1 时，带 aq/rl 的原子指令（含 LR/SC）访问 device-I/O 有序区域时排序被修改为同时约束 I/O 与内存——hypervisor 可配置修改 Zalrsc 指令语义的唯一直接途径。 |
| `norm:henvcfg_adue_op` | `hypervisor.adoc` | If Svadu is implemented, ADUE controls whether hardware updating of PTE A/D bits is enabled for VS-stage translation; when ADUE=0 the implementation behaves as though Svade were implemented for VS-stage. | ADUE 控制 VS-stage 的 PTE A/D 硬件更新；ADUE=0 时 LR/SC 走 Svade 页错误路径（cause 13/15），ADUE=1 时才有硬件更新与写伪指令场景。 |
| `norm:H_virtinst_xtval` | `hypervisor.adoc` | On a virtual-instruction trap, `mtval` or `stval` is written the same as for an illegal-instruction trap. | virtual-instruction 异常的 mtval/stval 写入规则与 illegal-instruction 相同（指令编码或 0）；HZWRS-07 据此验证 VTW 异常的 stval。 |
| `norm:H_cause_virtual_instruction` | `hypervisor.adoc` | When V=1, a virtual-instruction exception (code 22) is normally raised instead of an illegal-instruction exception if the attempted instruction is HS-qualified but is prevented from executing when V=1 either due to insufficient privilege or because the instruction is expressly disabled by a supervisor or hypervisor CSR. | V=1 时 virtual-instruction (cause=22) **仅**替代 HS-qualified（HS-mode 下合法可执行）但因特权不足或被 CSR 显式禁用而受阻的指令；真正保留/非法编码（如 Zabha 未定义的字节/半字 `lr`/`sc`）在所有模式均非法（非 HS-qualified），故 V=1 时仍报 illegal-instruction (cause=2) 而非 cause=22——Group 4 中 HZABHA-37 的权威依据，亦印证原子指令族无 virtual-instruction 门控。 |

### Smstateen 相关（`smstateen.adoc`）

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:stateen_op` | `smstateen.adoc` | The `stateen` registers at each level control access to state at all less-privileged levels, but not at its own level. | 各级的 `stateen` 寄存器控制对所有更低特权级状态的访问，但不控制本级——故 `hstateen0` 门控 VS/VU-mode，不影响 HS-mode 自身。 |
| `norm:stateen_illegal_state_access` | `smstateen.adoc` | Just as with the `counteren` CSRs, when a `stateen` CSR prevents access to state by less-privileged levels, an attempt in one of those privilege modes to execute an instruction that would read or write the protected state raises an illegal-instruction exception, or, if executing in VS or VU mode and the circumstances for a virtual-instruction exception apply, raises a virtual-instruction exception instead of an illegal-instruction exception. | `stateen` 阻止低特权级访问某状态时，在该特权级执行**会读写受保护状态**的指令引发 illegal-instruction；若处于 VS/VU-mode 且满足 virtual-instruction 条件，则改报 virtual-instruction (cause=22)。门控触发条件是"读写受保护状态"——Za 系列原子扩展（Zalrsc/Zaamo/Zacas/Zabha/Zalasr）均无 CSR、不引入架构状态，无 stateen 位可分配，故其指令不落在该门控范围内，清零 `hstateen0` 后 VS/VU-mode 仍必须正常执行且**绝不**报 cause=22（HZACAS-36 的判定依据）。 |
| `norm:stateen_unimplemented_state_roz` | `smstateen.adoc` | Bits in any `stateen` CSR that are defined to control state that a hart doesn't implement are read-only zeros for that hart. | 控制本 hart 未实现状态的 `stateen` 位为只读零——据此可反向确认平台未为 Za 系列原子指令分配任何 stateen 位（HZACAS-36 的辅助探测依据）。 |
| `norm:mstateen0_se0_op` | `smstateen.adoc` | The SE0 bit in `mstateen0` controls access to the `hstateen0`, `hstateen0h`, and the `sstateen0` CSRs. | `mstateen0`.SE0（bit 63）控制对 `hstateen0`/`hstateen0h`/`sstateen0` 的访问；构造 HZACAS-36 时须保持 SE0=1，否则 HS-mode 对 `hstateen0` 的读写自身会被门控，使前置条件无法建立。 |
| `norm:hstateen0_SE0_op` | `smstateen.adoc` | The SE0 bit in `hstateen0` controls access to the `vsstateen0` and the `sstateen0` CSRs. | `hstateen0`.SE0 控制对 `vsstateen0`/`sstateen0` 的访问；清零 `hstateen0` 会连带门控 VS-mode 对 `vsstateen0` 的访问，但不影响 `amocas`（无状态可门控）。 |

### Zawrs 相关（`zawrs.adoc`）

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:Zawrs_exec_resume_rules` | `zawrs.adoc` | The wrs.nto and wrs.sto instructions follow the rules of the wfi instruction for resuming execution on a locally enabled pending interrupt. | wrs.nto/wrs.sto 遵循 wfi 指令关于本地使能的 pending 中断恢复执行的规则（VS/VU-mode 下存在本地使能中断时不停顿、不触发 VTW 异常）。 |
| `norm:Zawrs_virtual_instr_excp` | `zawrs.adoc` | When executing in VS- or VU-mode, if the vtw bit is set in hstatus, the tw bit in mstatus is clear, and the wrs.nto does not complete within an implementation-specific bounded time limit, the wrs.nto instruction will cause a virtual-instruction exception. | 在 VS/VU-mode 下，若 hstatus.VTW=1、mstatus.TW=0，且 wrs.nto 未在实现特定有限时间内完成，将引发虚拟指令异常。 |
| `norm:Zawrs_priv_illegal_instr_excp` | `zawrs.adoc` | When the tw (timeout wait) bit in mstatus is set and wrs.nto is executed in any privilege mode other than M-mode, and it does not complete within an implementation-specific bounded time limit, the wrs.nto instruction will cause an illegal-instruction exception. | mstatus.TW=1 时，非 M-mode（含 VS/VU-mode）执行 wrs.nto 若未在实现限定时间内完成，引发非法指令异常（TW 条款优先于 VTW）。 |
| `norm:Zawrs_stall_terminate` | `zawrs.adoc` | While stalled, an implementation is permitted to occasionally terminate the stall and complete execution for any reason. | 停顿期间，实现允许以任何原因终止停顿并完成执行（Group 6 时序失败处置的依据）。 |

> **对照引用（不在本方案覆盖范围）**：`norm:hstatus_vtw_op`（VTW=1 时 VS-mode 执行 WFI 触发 virtual-instruction）与 `norm:vtw_virtinstr`（实现可在 VTW=1 时总是触发 virtual-instruction）由 `Hypervisor_CSR_test_plan.md`（HSTAT-04/06）覆盖，本方案 Group 6 仅作为 WFI 语义对照引用；`norm:vtw_virtinstr` 的许可行为是 HZWRS-06 判为记录型的依据。

---

## Group 1. Hypervisor × Zalrsc 交叉测试

**与 Hypervisor 的交集点**：
1. **异常类别二分**：LR 为 load 访问 → load 类 cause 4/5/13/21；SC 为 store 访问 → store/AMO 类 cause 6/7/15/23。权威依据：`norm:mcause_exccode_st_sc_amo`（store-conditional 生成 store/AMO 异常）、`norm:load_page_fault_no_r`（load-reserved 需读权限）、`norm:store_page_fault_no_w`（store-conditional 需写权限，且不可读页必不可写故报 store 类）
2. **委托路径二分**：VS-stage 故障（cause 4/5/6/7/13/15）的 `hedeleg` 位为 Writable，可下放 VS-mode；G-stage 故障（cause 21/23）的 `hedeleg` 位为 Read-only 0，**架构强制**陷入 HS-mode（`norm:hedeleg_acc`、`norm:H_vm_gpatrans`）
3. **htinst 双路径消歧**：显式访问故障 → transformed atomic instruction（保留 funct5/aq/rl/funct3/rd/rs2，仅 bits19:15 ← Addr. Offset）或 0；隐式 VS-stage 页表遍历故障 → **必须**为伪指令（读 0x00003000 / A-D 写 0x00003020，RV64）且不允许 0。htinst 是 HS-mode 区分两者的**唯一手段**（htval 分别指向目标数据 GPA 与 PTE GPA）
4. **Addr. Offset 恒为 0**：Addr. Offset = faulting VA − original VA，仅未对齐拆分访问可非零；但 `norm:lr_sc_alignment` 要求 LR/SC 必须自然对齐且**无 MAG 放宽**（与 Zalasr 不同），未对齐直接异常而不拆分，故 LR/SC 的 Addr. Offset 恒为 0
5. **GVA/SPV 组合**：guest LR/SC trap → GVA=1 且 SPV=1；而 HLV/HLVX/HSV 显式访问故障 → GVA=1 但 SPV=0。两者对比可证明 LR/SC 不属虚拟机访存指令
6. **失败 SC 仍受写权限检查**：`norm:sc_retire_permission`（shall，强制）+ `norm:sc_failed_as_store`（may）→ 一条因无 reservation 而必然失败、逻辑上不写内存的 SC，指向无写权限页时**仍须**报 store 类异常（含 G-stage 的 cause=23）
7. **无 HLR/HSC 架构边界**：`norm:hlsv_op` 仅映射 RV32I/RV64I 基础 load/store，LR/SC（AMO opcode）无虚拟机等价指令 → HS-mode 无法以 VS/VU 有效特权（`norm:hlsv_priv`）对 guest 内存做原子 RMW
8. **cause=22 排除**：`hypervisor.adoc` 全部 virtual-instruction 条款（VTVM/VTW/VTSR、HLV/HSV/HLVX、CBO 门控、counter CSR 等）**无一涵盖 LR/SC** → guest LR/SC 绝不报 cause=22
9. **henvcfg.FIOM × aq/rl**：V=1 且 FIOM=1 时，带 aq/rl 的 LR/SC 访问 device-I/O 有序区域时排序被修改（`norm:henvcfg_fiom_order`）——hypervisor 可配置修改 Zalrsc 语义的唯一直接途径
10. **henvcfg.ADUE × 失败 SC 副作用**：`norm:sc_failed_side_effects`（UNSPECIFIED）与 `norm:henvcfg_adue_op` 叠加，且 VS-stage/G-stage 两级 PTE 的 D 位**各自独立** UNSPECIFIED（双重不确定性）
11. **前向进展 × 虚拟化 trap**：`norm:constrained_lrsc_forward_progress_trap`（“H traps”）使 guest 受约束循环被 hypervisor 抢占打断时保证即已满足——Zalrsc 活锁自由保证与虚拟化天然兼容
12. **页表内存 RsrvEventual**：硬件 A/D 更新须对 PTE 做原子 RMW，故 VS-stage（`vsatp`）与 G-stage（`hgatp`）页表所在的 guest 物理内存均须具备 RsrvEventual PMA（`ptmem_rsrv_eventual`）

**规范依据**：
- `norm:lr_w_op` / `norm:sc_w_success` / `norm:sc_w_failure` / `norm:lr_sc_rv64`：LR/SC 基础语义在 VS/VU-mode 下不变（非虚拟化语义由 `Zalrsc_test_plan.md` 覆盖，本组仅验证虚拟化不改变语义）
- `norm:mcause_exccode_st_sc_amo` / `norm:load_page_fault_no_r` / `norm:store_page_fault_no_w`：LR→load 类、SC→store/AMO 类的异常归类与权限要求
- `norm:hedeleg_acc` / `norm:hedeleg_op` / `norm:H_vm_gpatrans`：委托路径二分与 G-stage 故障必报 guest-page-fault
- `norm:H_trap_xtinst_val` / `htinst_transformed_atomic` / `norm:H_trap_xtinst_guestpage` / `norm:H_trap_xtinst_guestpage_rw`：htinst 的 transformed 与伪指令两类取值规则
- `norm:hstatus_gva_op` / `norm:hstatus_spv_op` / `norm:htval_trapval`：trap 现场 GVA/SPV/htval 组合
- `norm:sc_retire_permission` / `norm:sc_failed_as_store` / `norm:sc_failed_side_effects` / `norm:sc_reservation_invalidate`：SC 退休权限、失败 SC 的 store 保护与副作用
- `norm:lr_sc_alignment`：未对齐 LR/SC 必产生异常（无 MAG 放宽），推导 Addr. Offset 恒为 0
- `norm:lr_sc_aq_rl_software_rule` / `norm:henvcfg_fiom_order` / `norm:henvcfg_adue_op`：aq/rl 位在 htinst 中的保留、FIOM 排序修改、ADUE 与 A/D 更新路径
- `norm:constrained_lrsc_forward_progress_intro` / `norm:constrained_lrsc_forward_progress_trap` / `norm:unconstrained_lrsc_no_progress` / `norm:lrsc_eventuality_region` / `ptmem_rsrv_eventual`：VS-mode 下受约束循环前向进展与虚拟化 trap 的兼容性
- `norm:hlsv_op` / `norm:hlsv_priv` / `norm:hlsv_virtinst`：无 LR/SC 虚拟机等价指令的架构边界，以及 cause=22 的排除对照

**测试职责**：验证 LR/SC 在 V=1 / 两阶段翻译 / HS-mode 视角下的交叉行为：异常类别与 cause 精确区分、委托路径二分、htinst 的 transformed/伪指令双路径消歧、GVA/SPV/htval trap 现场、SC 的退休权限与失败 SC 的 store 类异常、未对齐路径与 Addr. Offset、FIOM/ADUE 交互、受约束循环前向进展与虚拟化 trap 兼容性，以及无 HLR/HSC 的架构边界。LR/SC 指令若工具链不支持助记符则以 raw encoding 注入（AMO opcode=0x2F，funct5=00010(LR)/00011(SC)，funct3=010(.w)/011(.d)，bit26=aq、bit25=rl，LR 的 rs2 字段置 0），统一 `.option norvc` 保证 4 字节指令长度。本组 1.1 的各特权级可执行性用例（HZLRSC-01~04）从 `Zalrsc_test_plan.md` 迁移而来（原 ZLRSC-56：HS/VS/VU-mode 执行 LR/SC），并强化为含 `.d` 宽度、SC 成功语义与 virtual-instruction (cause=22) 负向断言；1.2~1.9 为虚拟化交叉分析新增用例。迁移后 `Zalrsc_test_plan.md` 仅保留 M/S/U 非虚拟化场景，Zalrsc 在虚拟化下的全部交叉行为以本组为唯一权威来源。

### 1.1 HS/VS/VU-mode 正常执行与 cause=22 排除

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLRSC-01 | HS-mode 执行 LR/SC 正常 | HS-mode 对主存执行 `lr.w` → `sc.w`（同地址同尺寸、无干扰）及 `lr.d` → `sc.d` | SC 成功（rd=0）、内存更新正确，无异常 |
| HZLRSC-02 | VS-mode 执行 LR/SC 不报 cause=22 | V=1，VS-mode 执行 `lr.w` → `sc.w` 与 `lr.d` → `sc.d`（目标为 VS-stage/G-stage 均有效的映射） | 正常执行、SC 成功；**绝不**触发 virtual-instruction exception (cause=22)（对比 `norm:hlsv_virtinst` 仅适用 HLV/HLVX/HSV） |
| HZLRSC-03 | VU-mode 执行 LR/SC 不报 cause=22 | 同配置，VU-mode 执行 `lr.w` → `sc.w` | 正常执行，无异常，绝不报 cause=22 |
| HZLRSC-04 | VS-mode LR/SC 语义与非虚拟化一致 | 同一 LR/SC 序列分别在 HS-mode 与 VS-mode 执行，比对加载值、符号扩展结果、SC 返回值与内存终值 | 架构可见语义完全一致（虚拟化不改变 Zalrsc 指令语义，`norm:lr_w_op`/`norm:sc_w_success`） |

### 1.2 异常类别二分与 VS-stage 故障的委托路径

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLRSC-05 | LR/SC 相关 hedeleg 位属性探测 | 逐位写 `hedeleg` bit 4/5/6/7/13/15 与 bit 21/23 并读回 | bit 4/5/6/7/13/15 可写（Writable）；bit 21/23 只读零（Read-only 0，`norm:hedeleg_acc`）——确立“VS-stage 可委托、G-stage 强制 HS-mode”二分前提 |
| HZLRSC-06 | LR 的 VS-stage 读权限故障报 load 类 | `hedeleg[13]`=1，VS-stage 映射目标页 R=0，VS-mode 执行 `lr.w` | load page fault (cause=13) 递送至 VS-mode，`vsepc`=`lr.w` PC，`vstval`=故障 GVA；**不得**报 cause=15（`norm:load_page_fault_no_r`） |
| HZLRSC-07 | SC 的 VS-stage 写权限故障报 store/AMO 类 | `hedeleg[15]`=1，VS-stage 映射目标页 R=1/W=0，VS-mode 执行 `lr.w` → `sc.w` | store/AMO page fault (cause=15) 递送至 VS-mode；**不得**报 cause=13（`norm:mcause_exccode_st_sc_amo`/`norm:store_page_fault_no_w`） |
| HZLRSC-08 | hedeleg=0 时 LR/SC 故障陷入 HS-mode | `hedeleg[13]`=0、`hedeleg[15]`=0，VS-mode 分别触发 LR/SC 的 VS-stage 页故障 | 均递送至 HS-mode（`norm:hedeleg_op`），cause 仍为 13/15（委托与否不改变 cause，仅改变递送目标） |

### 1.3 G-stage 故障强制 HS-mode 与 trap 现场（GVA/SPV/htval）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLRSC-09 | LR 的 G-stage 故障强制陷入 HS-mode | G-stage 中目标 GPA 映射无效，VS-mode 执行 `lr.w`；同时探测 `hedeleg[21]` | load guest-page fault (cause=21) 陷入 HS-mode；`hedeleg[21]` 为只读零，无法委托至 VS-mode（`norm:H_vm_gpatrans`/`norm:hedeleg_acc`） |
| HZLRSC-10 | SC 的 G-stage 故障强制陷入 HS-mode | G-stage 中目标 GPA 无写权限，VS-mode 执行 `lr.w` → `sc.w`；探测 `hedeleg[23]` | store/AMO guest-page fault (cause=23) 陷入 HS-mode；`hedeleg[23]` 只读零 |
| HZLRSC-11 | guest LR/SC trap 的 GVA=1 且 SPV=1 | 承接 HZLRSC-09/10，检查 HS-mode trap 现场的 `hstatus` | GVA=1 **且** SPV=1（`norm:hstatus_gva_op`/`norm:hstatus_spv_op`），`stval`=故障 GVA |
| HZLRSC-12 | guest LR/SC 故障时 htval=GPA>>2 | 承接 HZLRSC-09/10，检查 `htval` | `htval`=故障 GPA>>2（`norm:htval_trapval` 允许写零；非零时必须为 GPA>>2） |
| HZLRSC-13 | VS-stage 故障时 htval=0 | 承接 HZLRSC-08（cause=13/15 陷入 HS-mode），检查 `htval` | `htval`=0（`norm:htval_trapval`：仅 guest-page fault 写 GPA，其余 trap 写零）——与 HZLRSC-12 形成对比 |
| HZLRSC-14 | 对比 HLV/HSV 的 SPV/GVA 组合 | HS-mode（V=0）设 `hstatus.SPVP`=1 执行 `HLV.W` 触发 guest-page fault，检查 `hstatus` | SPV=0 但 GVA=1（`norm:hstatus_gva_op` NOTE 的唯一例外情形）；与 HZLRSC-11 的 SPV=1/GVA=1 对比，证明 LR/SC 不属虚拟机访存指令 |

### 1.4 htinst：transformed atomic instruction 与伪指令的消歧

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLRSC-15 | guest LR 显式访问故障的 htinst | VS-mode 执行 `lr.w` 触发 G-stage **显式**访问故障（cause=21），读 `htinst` 与 `vsepc` 处指令字比对 | `htinst`=0 或 transformed atomic instruction（`norm:H_trap_xtinst_val`）；非零时必须等于“原指令字保留全部字段、仅 bits19:15 替换为 Addr. Offset”（`htinst_transformed_atomic`） |
| HZLRSC-16 | guest SC 显式访问故障的 htinst | VS-mode 执行 `sc.w` 触发 cause=23，读 `htinst` 与 golden 值比对 | 同上；golden 值由 `hyp_transform_mem_inst()`（opcode 0x2F 分支）计算，必须保留 funct5=00011、funct3、rd、rs2 |
| HZLRSC-17 | htinst 保留 aq/rl 位 | VS-mode 分别执行 `lr.w.aq`、`sc.w.rl`、`lr.w.aqrl`、`sc.w.aqrl` 触发 G-stage 故障，比对 `htinst` 的 bit26/bit25 | transformed 值的 aq(bit26)/rl(bit25) 与陷入指令逐位一致（`htinst_transformed_atomic`：保留除 bits19:15 外全部字段） |
| HZLRSC-18 | LR/SC 的 htinst Addr. Offset 恒为 0 | 承接 HZLRSC-15/16，检查 `htinst` bits19:15 | Addr. Offset=0（`norm:lr_sc_alignment` 要求自然对齐且无 MAG 放宽，未对齐直接异常而不拆分访问，故 faulting VA 等于 original VA） |
| HZLRSC-19 | 隐式 VS-stage 遍历故障时 htinst 必须为伪指令 | 构造 VS-stage 叶页表页在 G-stage 无效，VS-mode 执行 `lr.w`，使其翻译在**隐式读 VS-stage PTE** 时故障 | cause=21 陷入 HS-mode；当 `htval` 非零（=该 PTE 的 GPA>>2）时，`htinst` **必须**为伪指令（RV64 读=0x00003000），**不允许为 0**（`norm:H_trap_xtinst_guestpage`） |
| HZLRSC-20 | A/D 自动更新故障的写伪指令（ADUE=1） | `henvcfg.ADUE`=1，构造 VS-stage PTE 的 A=0（或 D=0 且为写访问）且该 PTE 页在 G-stage 无写权限，VS-mode 执行 `sc.w` | `htinst`=写伪指令 0x00003020（`norm:H_trap_xtinst_guestpage_rw`：A/D 自动更新用 write 伪指令）；`htval`=该 PTE 的 GPA>>2；cause=23 |
| HZLRSC-21 | 显式与隐式故障的 htinst 消歧 | 对**同一 cause=21**，分别构造“LR 自身数据访问在 G-stage 失败”与“LR 的 VS-stage PTE 读在 G-stage 失败”两种场景，比对 `htinst` 与 `htval` | 前者 `htinst`=0 或 transformed atomic、`htval`=目标数据 GPA>>2；后者 `htinst`=伪指令（非零强制）、`htval`=PTE GPA>>2。htinst 为唯一消歧手段 |

### 1.5 SC 退休权限检查（失败 SC 仍报 store 类异常）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLRSC-22 | 有效 reservation 的 SC 到无写权限页 | VS-stage 页 R=1/W=0，VS-mode 执行 `lr.w`（成功建立 reservation）→ `sc.w` | store/AMO page fault (cause=15)（`norm:store_page_fault_no_w` + `norm:sc_retire_permission`：未过权限检查不得退休） |
| HZLRSC-23 | **失败 SC** 仍报 store 类异常 | VS-stage 页 R=1/W=0，VS-mode **不执行 LR** 直接执行 `sc.w`（该 SC 因无 reservation 必然失败、逻辑上不写内存） | **仍**触发 store/AMO page fault (cause=15)（`norm:sc_failed_as_store`：就内存保护而言失败 SC 可按 store 处理；`norm:sc_retire_permission` 为强制）——“不写内存的指令产生写类异常” |
| HZLRSC-24 | 失败 SC 的 G-stage 写权限检查 | G-stage 目标 GPA 无写权限，VS-mode 不执行 LR 直接执行 `sc.w` | store/AMO guest-page fault (cause=23) 陷入 HS-mode，GVA=1、SPV=1，`htval`=GPA>>2 |
| HZLRSC-25 | LR 仅需读权限（R=1/W=0 页正常） | VS-stage 页 R=1/W=0，VS-mode 执行 `lr.w` | **正常执行，无异常**（`norm:load_page_fault_no_r`：LR 仅需读权限）——与 HZLRSC-22 的 SC 形成关键对比 |
| HZLRSC-26 | SC 到不可读页报 store 类而非 load 类 | VS-stage 页 R=0/W=0，VS-mode 不执行 LR 直接执行 `sc.w` | store/AMO page fault (cause=15)，**不得**报 load page fault (cause=13)（`norm:store_page_fault_no_w` 配套 NOTE：不可读页必不可写，AMO/SC 恒报 store page-fault） |

### 1.6 未对齐 LR/SC 的异常路径

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLRSC-27 | VS-mode 未对齐 LR 的异常与委托 | VS-stage/G-stage 均有效，VS-mode 对 2 字节对齐（非 4 字节）地址执行 `lr.w`（trap-armed），`hedeleg[4]`/`[5]` 分别置 0 与 1 | 触发 load address misaligned (cause=4) 或 load access fault (cause=5)（二者之一均合规，`norm:lr_sc_alignment`）；`hedeleg` 置 1 时递送 VS-mode、置 0 时陷入 HS-mode |
| HZLRSC-28 | VS-mode 未对齐 SC 的异常与委托 | 同上，对 2 字节对齐地址执行 `sc.w`，`hedeleg[6]`/`[7]` 分别置 0 与 1 | store/AMO address misaligned (cause=6) 或 store/AMO access fault (cause=7)；委托路径按 `hedeleg[6]`/`[7]` |
| HZLRSC-29 | 未对齐 LR/SC 陷入 HS-mode 时的现场 | `hedeleg[4]`/`[6]`=0，承接 HZLRSC-27/28，检查 HS-mode trap 现场 | cause=4/5（LR）或 6/7（SC）；`stval`=**未对齐地址本身**（等于 original VA）；GVA=1、SPV=1；`htval`=0（非 guest-page fault）；`htinst`=0 或 transformed atomic 且 **Addr. Offset=0**（LR/SC 不拆分未对齐访问，与普通 load/store 的 Addr. Offset 可非零相区别） |

### 1.7 henvcfg.FIOM 与 ADUE 交互

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLRSC-30 | FIOM=1 时 VS-mode 带 aq/rl 的 LR/SC | `henvcfg.FIOM`=1，VS-mode 对平台声明为 device-I/O 有序的区域执行 `lr.w.aq` / `sc.w.rl`（trap-armed） | 指令可执行且数据语义不变；排序被修改为同时约束 device I/O 与内存（`norm:henvcfg_fiom_order`）——排序效果需多 hart 观测，单 hart 仅验证可执行性与数据语义，排序断言待多 hart 就绪 |
| HZLRSC-31 | FIOM=0 对照 | `henvcfg.FIOM`=0，同场景执行相同指令 | 排序不被修改（对照），指令可执行且数据语义与 HZLRSC-30 一致 |
| HZLRSC-32 | （记录型）ADUE=1 时失败 SC 的 VS-stage D 位副作用 | `henvcfg.ADUE`=1，VS-stage PTE D=0 且 W=1，构造必然失败的 SC（无 reservation）指向该页，回读 PTE 的 D 位 | D 位置位或保持**均合法**（`norm:sc_failed_side_effects` 为 UNSPECIFIED），记录实现行为，不做强制判定 |
| HZLRSC-33 | （记录型）ADUE=1 时失败 SC 的 G-stage D 位副作用 | 同 HZLRSC-32，另回读对应 G-stage PTE 的 D 位 | 同为 UNSPECIFIED（两级 PTE **各自独立**不确定）；且 `supervisor.adoc` 允许 G-stage D 位由 VS-stage PTE 的隐式访问更新。记录实现行为，不做强制判定 |
| HZLRSC-34 | ADUE=0 时 VS-mode LR/SC 走 Svade 页错误路径 | `henvcfg.ADUE`=0，VS-stage PTE A=0，VS-mode 执行 `lr.w` → `sc.w` | VS-stage 按 Svade 行为报 page fault（LR → cause=13、SC → cause=15）而非硬件更新 A 位（`norm:henvcfg_adue_op`）；递送目标按 `hedeleg[13]`/`[15]`。注：不得将 VS-stage 的 A/D 违规误判为 guest-page fault（cause 21/23） |

### 1.8 受约束循环前向进展与虚拟化 trap 兼容性

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLRSC-35 | VS-mode 受约束 LR/SC 循环前向进展 | VS-mode 执行严格受约束的 LR.W/SC.W 循环（≤16 条指令、LR-SC 间仅基础整数指令），目标为具备 eventuality 属性的主存，看门狗保护 | 循环在看门狗时限内完成（`norm:constrained_lrsc_forward_progress_intro`、`norm:lrsc_eventuality_region`）；前置：VS-stage/G-stage 页表内存满足 `ptmem_rsrv_eventual` |
| HZLRSC-36 | VS-mode 受约束循环被 trap 打断仍合规 | 在 VS-mode 受约束循环执行期间周期性触发 trap 至 HS-mode（如 HS-mode 抢占），观察循环最终结果 | **合规**：`norm:constrained_lrsc_forward_progress_trap`（“H traps”）本身即前向进展事件之一。trap 打断后 SC 失败并重试最终成功、或经退出分支结束，均不得判 FAIL；仅看门狗超时且无法经退出路径结束（挂死）才 FAIL |
| HZLRSC-37 | （记录型）VS-mode 不受约束序列对照 | VS-mode 构造不受约束 LR/SC 序列（LR-SC 间插入 load/store 或超 16 条指令）带看门狗执行 | 不设成功断言（`norm:unconstrained_lrsc_no_progress` 允许永不成功）；仅挂死时 FAIL |
| HZLRSC-38 | （记录型）trap 往返后 guest reservation 存续 | VS-mode 执行 `lr.w` 建立 reservation → 主动 ecall 陷入 HS-mode → HS-mode 执行自身 `lr`/`sc` 或 store → 返回 VS-mode 执行 `sc.w` | SC 成功或失败**均合规**：同 hart 介入访问使序列不再受约束，且 SPEC 允许 SC 因任何原因偶发失败（前提是不违反前向进展保证）。记录实现行为；仅异常类型错误或挂死时 FAIL。注：hypervisor 应在 VM 切换时以 scratch SC 主动作废 guest 遗留 reservation |

### 1.9 架构边界：H 扩展无 LR/SC 虚拟机等价指令

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLRSC-39 | （记录型）无 HLR/HSC 等价指令 | 以 trap-armed 探测 HS-mode 下是否存在按 `norm:hlsv_priv` 语义（有效特权由 `hstatus.SPVP` 决定）执行原子 RMW 的指令编码；对照 HLV/HSV 的可用性 | 记录性：`norm:hlsv_op` 仅映射 RV32I/RV64I 的 LB..LD / SB..SD，无 LR/SC 对应指令。HS-mode 对 guest 内存的原子 RMW 只能经 HLV+HSV（非原子）或将 GPA 映射入 HS 地址空间后用普通 LR/SC。探测到的任何“虚拟机原子指令”编码须与 SPEC 比对后报告，不得自行假定 |
| HZLRSC-40 | （记录型）HLV+HSV 无法替代 LR/SC 原子性 | HS-mode 以 `HLV.W` + `HSV.W` 序列对 guest 内存做读改写，与 VS-mode 内的 LR/SC 循环并发（需多 hart） | 记录性：HLV+HSV 为两次独立非原子访问、无 reservation 保护，并发下可观测到丢失更新；多 hart 未就绪时 SKIP |

> [!NOTE]
> - 本组所有测试必须以平台配置 `H_SUPPORTED` 宏为准检测 H 扩展；Zalrsc 支持以平台配置 `ZALRSC_SUPPORTED` 宏为准，并以 trap-armed raw encoding 探测指令可用性，不满足时全组 TEST_SKIP。
> - **异常类别与 cause 断言必须精确**：LR 归 load 类（4/5/13/21）、SC 归 store/AMO 类（6/7/15/23），依据 `norm:mcause_exccode_st_sc_amo`、`norm:load_page_fault_no_r`、`norm:store_page_fault_no_w`。断言不得将两者混淆，也不得将 VS-stage 故障（cause 13/15）误判为 G-stage 故障（cause 21/23）。
> - **cause=22 为强制负向断言**：HZLRSC-02/03 中 guest LR/SC 若报 virtual-instruction exception 即为违反 SPEC（`hypervisor.adoc` 无任何条款对 LR/SC 施加 virtual-instruction 门控），应保持失败并记录至 `bugs/` 目录，不得放宽断言。
> - **htinst golden 值计算已就绪**：框架 `hyp_transform_mem_inst()` 对 opcode 0x2F 已实现“Atomic (LR/SC/AMO)：保留除 bits19:15 外全部字段”，与 `htinst_transformed_atomic` 逐字一致；HZLRSC-15~18 可直接复用。HZLRSC-19/20 的隐式遍历场景可复用 `setup_implicit_walk_victim()`。
> - **htinst 伪指令的强制性**：HZLRSC-19 中当 `htval` 非零且故障源于 VS-stage 隐式访问时，`htinst` 写 0 属违反 `norm:H_trap_xtinst_guestpage`（“zero is not allowed”），应保持失败。而 HZLRSC-15/16 的显式访问场景下 `htinst` 写 0 是允许的（实现可减少努力）。
> - **未对齐异常宽容度**：HZLRSC-27/28 允许 address-misaligned 与 access-fault 两类合规结果（`norm:lr_sc_alignment`），用例验证 cause 属预期集合并记录实际值，不得因平台选择其中一类而判 FAIL。注意 Zalrsc **无** MAG 放宽（区别于 Zalasr），故未对齐 LR/SC 必须产生异常，无“无异常原子完成”分支。
> - **记录型用例不得强制判定**：HZLRSC-32/33（失败 SC 的 PTE D 位副作用，双重 UNSPECIFIED）、HZLRSC-37（不受约束序列）、HZLRSC-38（trap 往返后 reservation 存续）、HZLRSC-39/40（架构边界）均为实现自定义或 SPEC 许可行为，仅记录实现选择；以任一合法结果为由判失败即构成不当抬高或降低标准。
> - **看门狗与受约束形态**：HZLRSC-35~37 以循环内迭代计数器硬上限作看门狗（阈值远大于正常完成尝试次数），迭代判断置于重试分支处（SC 之后）而不放入 LR-SC 之间，以保持受约束形态；循环体须用内联汇编显式构造并静态断言指令条数 ≤16。
> - **多 hart 限制**：HZLRSC-30/31 的排序效果断言与 HZLRSC-40 需 secondary hart 支持；当前公共框架仅 hart 0 运行，相关断言在框架就绪前 SKIP（非 FAIL），单 hart 平台同样 SKIP 并注明。
> - **权限用例依赖**：1.2/1.3/1.5 需 VS-stage（`vsatp`）与 G-stage（`hgatp`）页表构造能力，包括 R=0、R=1/W=0、G-stage 无写权限等映射组合；权限检查机制本身由 `pmp_test_plan.md`/`vm_test_plan.md` 覆盖，本组仅验证 LR/SC 的权限语义结论。
> - Zalrsc 非虚拟化语义（编码、reservation set 形态、失败码、配对与必须失败情形、受约束循环构造自检）由 `Zalrsc_test_plan.md` 覆盖；主存 RsrvEventual 前向进展保证由 `Ziccrse_test_plan.md` 覆盖，本组不重复。
> - **与 `Zalrsc_test_plan.md` Group 5 的职责划分（不构成重复）**：该方案 Group 5 的 SC 权限与失败 SC 的 PTE A/D 副作用用例（ZLRSC-25~28）仅涉 PMP 与单层 S-mode 页表（satp），不涉 V=1；其虚拟化对应版本由本组 HZLRSC-22~26（VS-stage/G-stage 权限与失败 SC 的 store 类异常）与 HZLRSC-32/33（`henvcfg.ADUE`=1 时失败 SC 的 VS-stage/G-stage PTE D 位副作用）**独立覆盖**。两者验证对象不同（单层页表/PMP vs 两阶段翻译），结论互为佐证，不得以“已覆盖”为由删减任一侧用例。原 ZLRSC-56（HS/VS/VU-mode 执行 LR/SC）已整体迁入本组 HZLRSC-01~04，`Zalrsc_test_plan.md` 已不再保留任何 Hypervisor 相关内容，该编号废弃不复用。

---

## Group 2. Hypervisor × Zaamo 交叉测试

**与 Hypervisor 的交集点**：
1. **异常统一归 store/AMO 类**：AMO 即使含读阶段，其全部异常归 store/AMO 类（misaligned=6、access fault=7、page fault=15、guest-page fault=23），**绝不**报 load 类（4/5/13/21）。权威依据 `norm:mcause_exccode_st_sc_amo`（AMO 生成 store/AMO 异常）+ `norm:store_page_fault_no_w` 配套 NOTE（“AMOs never raise load page-fault exceptions”，不可读页必不可写故 AMO 到不可读页恒报 store page-fault）。这是与 Group 1 中 LR（load 类）的根本区别
2. **AMO 需 R+W 权限**：AMO 对 R=1/W=0 或 R=0 页均报 store page-fault (15)（VS-stage）或 store guest-page-fault (23)（G-stage）；对比 LR 仅需读权限（HZLRSC-25）
3. **委托路径二分**：VS-stage AMO 故障（cause 6/7/15）的 `hedeleg` 位为 Writable，可下放 VS-mode；G-stage AMO 故障（cause 23）的 `hedeleg[23]` 为 Read-only 0，**架构强制**陷入 HS-mode（`norm:hedeleg_acc`、`norm:H_vm_gpatrans`）
4. **隐式 VS-stage 遍历故障按 AMO 原始类型报 cause=23**：`norm:H_vm_gpapriv` — 隐式 PTE 访问按 implicit load/store 检查权限，但异常“always reported for the original access type”，故 AMO 的隐式遍历 GPF 报 store/AMO guest-page fault (cause=23)，**而非** LR 的 load guest-page fault (cause=21，HZLRSC-19)
5. **htinst transformed atomic instruction**：AMO 显式访问故障 → `htinst`/`mtinst` = 0 或 transformed atomic（保留 funct5/aq/rl/funct3/rd/rs2，仅 bits19:15 ← Addr. Offset），`transformedatomicinst` 格式图明文涵盖 AMO；框架 `hyp_transform_mem_inst()` 的 opcode 0x2F 分支已就绪
6. **Addr. Offset 恒为 0（理由区别于 Zalrsc）**：AMO 原子性要求单一内存操作——MAG 放宽时不跨页拆分、未放宽时直接异常，故 Addr. Offset 恒为 0（Zalrsc 因**无** MAG 放宽、未对齐直接异常，理由不同但结论一致）
7. **MAG 放宽（Zaamo 独有）**：`norm:misaligned_atomicity_granule_size` — 未对齐 AMO 若全部字节位于同一 MAG 粒度内则无对齐异常且原子执行；Zalrsc 无此分支
8. **GVA/SPV 组合**：guest AMO trap → GVA=1 且 SPV=1（VS 来源 SPVP=1、VU 来源 SPVP=0）；对比 HLV/HLVX/HSV 显式访问故障 → GVA=1 但 SPV=0
9. **AMO 必写 ⇒ D 位要求确定**：AMO 恒执行写回，故 `henvcfg.ADUE`=0 时对 A=0 或 D=0 的 VS-stage 页报 store page-fault（Svade 路径），**非 UNSPECIFIED**（区别于 Group 1 中失败 SC 的 PTE D 位副作用双重 UNSPECIFIED，HZLRSC-32/33）
10. **henvcfg.FIOM × aq/rl**：`norm:henvcfg_fiom_order` 明文“an atomic instruction that accesses a region ordered as device I/O has its aq and/or rl bit set”，V=1 且 FIOM=1 时带 aq/rl 的 AMO 访问 device-I/O 有序区域时排序被修改为同时约束 I/O 与内存
11. **cause=22 排除**：`hypervisor.adoc` 全部 virtual-instruction 条款无一涵盖 AMO → guest AMO 绝不报 cause=22（与 LR/SC 同）
12. **无 AMO 虚拟机等价指令**：`norm:hlsv_op` 仅映射 RV32I/RV64I 基础 load/store，AMO（opcode 0x2F）无 HL*/HS* 等价指令 → HS-mode 无法以 VS/VU 有效特权对 guest 内存做原子 RMW（与 Group 1 的 HZLRSC-39/40 共享架构边界）

**规范依据**：
- `amo_rmw_semantics` / `norm:amo_operand_size`：AMO 原子读改写与宽度/符号扩展语义在 VS/VU-mode 下不变（非虚拟化语义由 `Zaamo_test_plan.md` 覆盖，本组仅验证虚拟化不改变语义）
- `norm:mcause_exccode_st_sc_amo` / `norm:store_page_fault_no_w` / `norm:load_page_fault_no_r`：AMO 异常统一归 store/AMO 类、需 R+W 权限、绝不报 load page-fault
- `norm:hedeleg_acc` / `norm:hedeleg_op` / `norm:H_vm_gpatrans`：委托路径二分与 G-stage 故障必报 store/AMO guest-page-fault
- `norm:H_vm_gpapriv`：隐式 VS-stage 遍历故障按 AMO 原始类型（store/AMO）报告 cause=23
- `norm:H_trap_xtinst_val` / `htinst_transformed_atomic` / `norm:H_trap_xtinst_guestpage` / `norm:H_trap_xtinst_guestpage_rw`：htinst 的 transformed 与伪指令两类取值规则
- `norm:hstatus_gva_op` / `norm:hstatus_spv_op` / `norm:htval_trapval`：trap 现场 GVA/SPV/htval 组合
- `norm:amo_alignment` / `norm:misaligned_atomicity_granule_size`：未对齐 AMO 的异常路径与 MAG 放宽分支
- `norm:amo_release_consistency` / `norm:henvcfg_fiom_order` / `norm:henvcfg_adue_op`：aq/rl 位在 htinst 中的保留、FIOM 排序修改、ADUE 与 A/D 更新路径
- `norm:hlsv_op` / `norm:hlsv_priv` / `norm:hlsv_virtinst`：无 AMO 虚拟机等价指令的架构边界与 cause=22 排除对照

**测试职责**：验证 AMO 在 V=1 / 两阶段翻译 / HS-mode 视角下的交叉行为：异常统一归 store/AMO 类（绝不报 load 类）、R+W 权限要求、委托路径二分、隐式遍历故障按原始类型报 cause=23、htinst 的 transformed/伪指令双路径消歧、GVA/SPV/htval trap 现场、未对齐 AMO 的 MAG 放宽分支、FIOM/ADUE 交互，以及无 AMO 虚拟机等价指令的架构边界。AMO 基础原语（9 条 × .w/.d）已由 `common/mem_ops.h` 提供；aq/rl 变体与 funct5 位保留验证以 raw encoding 注入（AMO opcode=0x2F，funct5：amoadd=00000/amoswap=00001/amoxor=00100/amoor=00110/amoand=00111/amomin=01000/amomax=01001/amominu=01100/amomaxu=01101，funct3=010(.w)/011(.d)，bit26=aq、bit25=rl），统一 `.option norvc` 保证 4 字节指令长度；框架缺 aq/rl 原语时需在公共框架补充。本组 2.1 的各特权级可执行性用例与 Ziccamoa 的 ZCAMOA-22（主存 AMOArithmetic PMA 保证视角）互补——本组为 Hypervisor 交叉视角的权威来源，聚焦 cause=22 排除、异常归类与 trap 现场。

### 2.1 HS/VS/VU-mode 正常执行与 cause=22 排除

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZAMO-01 | HS-mode 执行全部 AMO 正常 | HS-mode 对主存执行 9 条 AMO 的 `.w`（RV64 另含 `.d`）变体 | 全部正常执行、rd=旧值、内存按运算更新，无异常 |
| HZAMO-02 | VS-mode 执行 AMO 不报 cause=22 | V=1，VS-mode 执行 AMO 集合（目标为 VS-stage/G-stage 均有效的可读写映射） | 正常执行、语义正确；**绝不**触发 virtual-instruction exception (cause=22)（`hypervisor.adoc` 无任何条款对 AMO 施加 virtual-instruction 门控） |
| HZAMO-03 | VU-mode 执行 AMO 不报 cause=22 | 同配置，VU-mode 执行 AMO 集合（目标页 U=1） | 正常执行，无异常，绝不报 cause=22 |
| HZAMO-04 | VS-mode AMO 语义与非虚拟化一致 | 同一 AMO 序列分别在 HS-mode 与 VS-mode 执行，比对 rd 旧值、`.w` 符号扩展、rs2 高 32 位忽略与内存终值 | 架构可见语义完全一致（虚拟化不改变 Zaamo 指令语义，`amo_rmw_semantics`/`norm:amo_operand_size`） |

### 2.2 AMO 异常统一归 store/AMO 类与 VS-stage 委托路径

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZAMO-05 | AMO 相关 hedeleg 位属性探测 | 逐位写 `hedeleg` bit 6/7/15 与 bit 23 并读回 | bit 6/7/15 可写（Writable）；bit 23 只读零（Read-only 0，`norm:hedeleg_acc`）——确立“VS-stage 可委托、G-stage 强制 HS-mode”二分前提 |
| HZAMO-06 | AMO 到 R=1/W=0 VS-stage 页报 store page-fault | `hedeleg[15]`=1，VS-stage 映射目标页 R=1/W=0，VS-mode 执行 `amoadd.w`/`amoadd.d` | store/AMO page fault (cause=15) 递送至 VS-mode，`vsepc`=AMO PC，`vstval`=故障 GVA（`norm:store_page_fault_no_w`：AMO 需写权限） |
| HZAMO-07 | AMO 到不可读页报 store 类而非 load 类 | VS-stage 页 R=0/W=0，VS-mode 执行 `amoadd.w` | store/AMO page fault (cause=15)，**绝不**报 load page fault (cause=13)（`norm:store_page_fault_no_w` 配套 NOTE：AMO 恒报 store page-fault） |
| HZAMO-08 | hedeleg=0 时 AMO VS-stage 故障陷入 HS-mode | `hedeleg[15]`=0，VS-mode 触发 AMO 的 VS-stage 写权限故障 | 递送至 HS-mode（`norm:hedeleg_op`），cause 仍为 15（委托与否不改变 cause，仅改变递送目标） |
| HZAMO-09 | LR 与 AMO 权限对比（R=1/W=0） | 同一 R=1/W=0 VS-stage 页，VS-mode 先执行 `lr.w`（对照 HZLRSC-25）再执行 `amoadd.w` | `lr.w` **正常执行**（仅需读权限），`amoadd.w` 报 store page-fault (cause=15)（需 R+W）——证明 AMO 权限要求严于 LR |

### 2.3 G-stage 故障强制 HS-mode 与 trap 现场（GVA/SPV/htval）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZAMO-10 | AMO 的 G-stage 故障强制陷入 HS-mode | G-stage 中目标 GPA 无写权限，VS-mode 执行 `amoadd.d`；探测 `hedeleg[23]` | store/AMO guest-page fault (cause=23) 陷入 HS-mode；`hedeleg[23]` 只读零，无法委托至 VS-mode（`norm:H_vm_gpatrans`/`norm:hedeleg_acc`） |
| HZAMO-11 | guest AMO trap 的 GVA=1 且 SPV=1 | 承接 HZAMO-10，检查 HS-mode trap 现场 `hstatus`；另以 VU-mode 触发同故障 | VS 来源：GVA=1、SPV=1、SPVP=1；VU 来源：GVA=1、SPV=1、SPVP=0（`norm:hstatus_gva_op`/`norm:hstatus_spv_op`），`stval`=故障 GVA |
| HZAMO-12 | guest AMO G-stage 故障时 htval=GPA>>2 或 0 | 承接 HZAMO-10，检查 `htval` | `htval`=故障 GPA>>2 或 0（基线 H `norm:htval_trapval` 允许写零；非零时必须为 GPA>>2）。注：Shtvala 实现下收紧为必须非零，由 `Shtvala_test_plan.md` HTVAL-AMO-01 覆盖 |
| HZAMO-13 | VS-stage AMO 故障时 htval=0 | 承接 HZAMO-08（cause=15 陷入 HS-mode），检查 `htval` | `htval`=0（`norm:htval_trapval`：仅 guest-page fault 写 GPA，其余 trap 写零）——与 HZAMO-12 形成对比 |
| HZAMO-14 | 对比 HLV/HSV 的 SPV/GVA 组合 | HS-mode（V=0）设 `hstatus.SPVP`=1 执行 `HSV.D` 触发 guest-page fault，检查 `hstatus` | SPV=0 但 GVA=1（`norm:hstatus_gva_op` NOTE 的唯一例外）；与 HZAMO-11 的 SPV=1/GVA=1 对比，证明 AMO 不属虚拟机访存指令 |

### 2.4 htinst：transformed atomic instruction 与伪指令消歧

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZAMO-15 | guest AMO 显式访问故障的 htinst | VS-mode 执行 `amoadd.d` 触发 G-stage **显式**访问故障（cause=23），读 `htinst` 与 `vsepc` 处指令字比对 | `htinst`=0 或 transformed atomic instruction（`norm:H_trap_xtinst_val`）；非零时必须等于“原指令字保留全部字段、仅 bits19:15 替换为 Addr. Offset”（`htinst_transformed_atomic`，golden 由 `hyp_transform_mem_inst()` opcode 0x2F 分支计算，须保留 funct5/funct3/rd/rs2） |
| HZAMO-16 | htinst 保留 aq/rl 位 | VS-mode 分别执行 `amoadd.w.aq`、`amoadd.w.rl`、`amoadd.w.aqrl`（raw encoding）触发 G-stage 故障，比对 `htinst` 的 bit26/bit25 | transformed 值的 aq(bit26)/rl(bit25) 与陷入指令逐位一致（`htinst_transformed_atomic`：保留除 bits19:15 外全部字段） |
| HZAMO-17 | AMO 的 htinst Addr. Offset 恒为 0 | 承接 HZAMO-15，检查 `htinst` bits19:15 | Addr. Offset=0——AMO 原子性要求单一内存操作、不跨页拆分（MAG 放宽时为单内存操作、未放宽时直接异常），故 faulting VA 等于 original VA |
| HZAMO-18 | 隐式 VS-stage 遍历故障按 AMO 原始类型报 cause=23 | 构造 VS-stage 叶页表页在 G-stage 无效，VS-mode 执行 `amoadd.w`，使其翻译在**隐式读 VS-stage PTE** 时故障 | cause=**23**（store/AMO guest-page fault，按 AMO 原始访问类型报告，**非** LR 的 cause=21，`norm:H_vm_gpapriv`）陷入 HS-mode；当 `htval` 非零（=该 PTE 的 GPA>>2）时 `htinst` **必须**为读伪指令（RV64=0x00003000）、**不允许为 0**（`norm:H_trap_xtinst_guestpage`） |
| HZAMO-19 | A/D 自动更新故障的写伪指令（ADUE=1） | `henvcfg.ADUE`=1，构造 VS-stage PTE 的 D=0（AMO 为写访问）且该 PTE 页在 G-stage 无写权限，VS-mode 执行 `amoadd.d` | `htinst`=写伪指令 0x00003020（`norm:H_trap_xtinst_guestpage_rw`：A/D 自动更新用 write 伪指令）；`htval`=该 PTE 的 GPA>>2；cause=23 |
| HZAMO-20 | 显式与隐式故障的 htinst/htval 消歧 | 对**同一 cause=23**，分别构造“AMO 自身数据访问在 G-stage 失败”与“AMO 的 VS-stage PTE 读在 G-stage 失败”两种场景，比对 `htinst` 与 `htval` | 前者 `htinst`=0 或 transformed atomic、`htval`=目标数据 GPA>>2；后者 `htinst`=伪指令（非零强制）、`htval`=PTE GPA>>2。htinst 为唯一消歧手段 |

### 2.5 未对齐 AMO 与 MAG 放宽（Zaamo 区别于 Zalrsc）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZAMO-21 | VS-mode 未对齐 AMO 无 MAG 覆盖的异常与委托 | VS-stage/G-stage 均有效，VS-mode 对 2 字节对齐（非 4 字节）地址执行 `amoadd.w`（trap-armed），`hedeleg[6]`/`[7]` 分别置 0 与 1 | 触发 store/AMO address misaligned (cause=6) 或 store/AMO access fault (cause=7)（二者之一均合规，`norm:amo_alignment`；**非** load 类 4/5）；`hedeleg` 置 1 时递送 VS-mode、置 0 时陷入 HS-mode |
| HZAMO-22 | MAG 粒度内未对齐 AMO 无异常且原子执行 | 平台声明 MAG，VS-mode 对全部字节位于同一 MAG 粒度内的未对齐地址执行 `amoadd.w` | **无**对齐异常、正常执行且语义正确（`norm:misaligned_atomicity_granule_size`，Zalrsc 无此放宽分支）；平台未声明 MAG 时本用例 SKIP |
| HZAMO-23 | MAG 内未对齐 AMO 的 G-stage 故障 | 承接 HZAMO-22 场景，令目标 GPA 在 G-stage 无写权限 | cause=23 陷入 HS-mode，`htval`=GPA>>2 或 0，`htinst` Addr. Offset=0（单一内存操作，不拆分） |
| HZAMO-24 | 未对齐 AMO 陷入 HS-mode 时的现场 | `hedeleg[6]`=0，承接 HZAMO-21，检查 HS-mode trap 现场 | cause=6 或 7；`stval`=**未对齐地址本身**（等于 original VA）；GVA=1、SPV=1；`htval`=0（非 guest-page fault）；`htinst`=0 或 transformed atomic 且 Addr. Offset=0 |

### 2.6 henvcfg.FIOM 与 ADUE 交互

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZAMO-25 | FIOM=1 时 VS-mode 带 aq/rl 的 AMO | `henvcfg.FIOM`=1，VS-mode 对平台声明为 device-I/O 有序的区域执行 `amoadd.w.aq`/`amoswap.d.rl`（trap-armed） | 指令可执行且数据语义不变；排序被修改为同时约束 device I/O 与内存（`norm:henvcfg_fiom_order`）——排序效果需多 hart 观测，单 hart 仅验证可执行性与数据语义 |
| HZAMO-26 | FIOM=0 对照 | `henvcfg.FIOM`=0，同场景执行相同指令 | 排序不被修改（对照），指令可执行且数据语义与 HZAMO-25 一致 |
| HZAMO-27 | ADUE=0 时 AMO 到 A=0 VS-stage 页 | `henvcfg.ADUE`=0，VS-stage PTE A=0，VS-mode 执行 `amoadd.w` | VS-stage 按 Svade 行为报 store page-fault (cause=15) 而非硬件更新 A 位（`norm:henvcfg_adue_op`）；递送目标按 `hedeleg[15]`；不得误判为 guest-page fault (cause 21/23) |
| HZAMO-28 | ADUE=0 时 AMO 到 A=1/D=0 页 | `henvcfg.ADUE`=0，VS-stage PTE A=1/D=0，VS-mode 执行 `amoadd.w` | store page-fault (cause=15)——AMO **恒执行写回**，D 位要求为**强制**（非 UNSPECIFIED，区别于失败 SC 的 HZLRSC-32/33）；`norm:henvcfg_adue_op`：ADUE=0 时按 Svade 报页错误 |
| HZAMO-29 | ADUE=1 时 AMO 硬件更新 A/D 位后正常完成 | `henvcfg.ADUE`=1（且平台实现 Svadu），VS-stage PTE A=0/D=0，VS-mode 执行 `amoadd.w` | 硬件自动置 A/D 位，AMO 正常完成、无页错误；回读 PTE A=1/D=1（`norm:henvcfg_adue_op`）；平台未实现 Svadu 时 SKIP |

### 2.7 架构边界：H 扩展无 AMO 虚拟机等价指令

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZAMO-30 | （记录型）无 HL*/HS* AMO 等价指令 | 以 trap-armed 探测 HS-mode 下是否存在按 `norm:hlsv_priv` 语义（有效特权由 `hstatus.SPVP` 决定）执行原子 RMW 的指令编码 | 记录性：`norm:hlsv_op` 仅映射 RV32I/RV64I 的 LB..LD / SB..SD，AMO（opcode 0x2F）无对应虚拟机指令。与 HZLRSC-39 共享同一架构边界结论；探测到的任何“虚拟机原子指令”编码须与 SPEC 比对后报告 |
| HZAMO-31 | （记录型）HLV+HSV 无法替代 AMO 原子性 | HS-mode 以 `HLV.W`+`HSV.W` 序列对 guest 内存做读改写，与 VS-mode 内的 `amoadd.w` 并发（需多 hart） | 记录性：HLV+HSV 为两次独立非原子访问，并发下可观测到丢失更新，而 VS-mode AMO 保持原子；多 hart 未就绪时 SKIP（与 HZLRSC-40 同理） |

> [!NOTE]
> - 本组所有测试必须以平台配置 `H_SUPPORTED` 宏为准检测 H 扩展；Zaamo/A 支持以平台配置 `ZAAMO_SUPPORTED`（或 `A_SUPPORTED`）宏为准，并以 trap-armed 执行 AMO 二次探测，不满足时全组 TEST_SKIP。
> - **异常类别断言必须精确且区别于 LR**：AMO 的**全部**异常归 store/AMO 类（misaligned=6、access fault=7、page fault=15、guest-page fault=23），依据 `norm:mcause_exccode_st_sc_amo` 与 `norm:store_page_fault_no_w` 配套 NOTE。断言**绝不**得接受 load 类（4/5/13/21）——AMO 到不可读页亦报 cause=15 而非 13；AMO 的隐式 VS-stage 遍历 GPF 报 cause=23 而非 21（`norm:H_vm_gpapriv`）。这是与 Group 1（LR 报 load 类）的根本分野。
> - **cause=22 为强制负向断言**：HZAMO-02/03 中 guest AMO 若报 virtual-instruction exception 即违反 SPEC（`hypervisor.adoc` 无任何条款对 AMO 施加 virtual-instruction 门控），应保持失败并记录至 `bugs/` 目录，不得放宽断言。
> - **htinst golden 值计算已就绪**：框架 `hyp_transform_mem_inst()` 对 opcode 0x2F 已实现“Atomic (LR/SC/AMO)：保留除 bits19:15 外全部字段”，与 `htinst_transformed_atomic` 逐字一致，HZAMO-15~17 可直接复用；HZAMO-18/19 的隐式遍历场景可复用 `setup_implicit_walk_victim()`。aq/rl 变体（HZAMO-16/25）若框架无对应原语须以 raw encoding 注入或补充原语。
> - **htinst 伪指令的强制性**：HZAMO-18 中当 `htval` 非零且故障源于 VS-stage 隐式访问时，`htinst` 写 0 属违反 `norm:H_trap_xtinst_guestpage`（“zero is not allowed”），应保持失败；HZAMO-15 的显式访问场景下 `htinst` 写 0 是允许的。
> - **MAG 放宽为 Zaamo 独有分支**：HZAMO-22/23 依赖平台声明 misaligned atomicity granule（`norm:misaligned_atomicity_granule_size`）；未声明 MAG 时未对齐 AMO 必产生 cause=6/7（HZAMO-21），HZAMO-22/23 SKIP。这与 Zalrsc（HZLRSC-27/28，无 MAG 放宽、未对齐必异常）形成对照。未对齐异常允许 address-misaligned 与 access-fault 两类合规结果，用例验证 cause 属预期集合并记录实际值，不得因平台选择其中一类而判 FAIL。
> - **AMO 必写 ⇒ D 位强制**：HZAMO-28 中 AMO 恒执行写回，故 ADUE=0 时对 D=0 页报 store page-fault 为**强制**结果（非记录型）；这与 Group 1 中失败 SC 的 PTE D 位副作用（HZLRSC-32/33，UNSPECIFIED 记录型）本质不同，不得混淆。
> - **记录型用例不得强制判定**：HZAMO-30/31（无 AMO 虚拟机等价指令的架构边界）为架构许可/实现自定义行为，仅记录实现选择。
> - **多 hart 限制**：HZAMO-25/26 的 FIOM 排序效果断言与 HZAMO-31 需 secondary hart 支持；当前公共框架仅 hart 0 运行，相关断言在框架就绪前 SKIP（非 FAIL），单 hart 平台同样 SKIP 并注明。
> - **权限用例依赖**：2.2/2.3/2.6 需 VS-stage（`vsatp`）与 G-stage（`hgatp`）页表构造能力（R=0、R=1/W=0、A=0、D=0、G-stage 无写权限等映射组合）；权限检查机制本身由 `pmp_test_plan.md`/`vm_test_plan.md` 覆盖，本组仅验证 AMO 的权限语义结论。
> - Zaamo 非虚拟化语义（AMO 编码、9 条操作的数据语义、rd 符号扩展、rs2 高 32 位忽略、aq/rl 数据语义、多 hart 原子性、M/S/U 各特权级基础可执行性）由 `Zaamo_test_plan.md` 覆盖；主存 AMOArithmetic 级 PMA 支持保证由 `Ziccamoa_test_plan.md` 覆盖，本组不重复。

---

## Group 3. Hypervisor × Zacas 交叉测试

**与 Hypervisor 的交集点**：
1. **异常统一归 store/AMO 类**：`amocas.w/d/q` 与 Zaamo 的 AMO 同属 opcode 0x2F 指令族（funct5=00101），`norm:mcause_exccode_st_sc_amo` 明文“AMO instructions generate store/AMO exceptions”涵盖 amocas，故其全部异常（misaligned=6、access fault=7、page fault=15、guest-page fault=23）**绝不**报 load 类（4/5/13/21），与 Group 2 中 Zaamo 的结论一致，本组不重复推导通用分类逻辑，仅以 amocas 具体指令字（funct5=00101）复验相同的 cause 归类
2. **无条件写权限检查（Zacas 特有，与 Group 1 失败 SC 同构）**：`norm:Zacas_amocas_w_permission`（“always requires write permissions”）为无条件强制语言，即使 CAS 比较失败（`zacas_failed_cas_write_note`：实现可选择不写内存），权限检查仍必须执行并报 store 类异常——这与 Group 1 中 `norm:sc_retire_permission`/`norm:sc_failed_as_store`（失败 SC 仍受写权限检查，HZLRSC-22~26）本质同构，区别于 Group 2 中 Zaamo 的 AMO（恒无条件写回，不存在“失败”分支，故无需专门验证“失败仍需权限检查”这一维度）
3. **委托路径二分**：VS-stage amocas 故障（cause 6/7/15）的 `hedeleg` 位为 Writable，可下放 VS-mode；G-stage amocas 故障（cause 23）的 `hedeleg[23]` 为 Read-only 0，**架构强制**陷入 HS-mode（`norm:hedeleg_acc`、`norm:H_vm_gpatrans`）——与 Group 1/Group 2 共享同一委托机制，本组独立探测以确立前提（保持每组自洽，不跨组引用探测结果）
4. **隐式 VS-stage 遍历故障按 amocas 原始类型报 cause=23**：`norm:H_vm_gpapriv` — 隐式 PTE 访问按 implicit load/store 检查权限，但异常“always reported for the original access type”，故 amocas 的隐式遍历 GPF 报 store/AMO guest-page fault (cause=23)，与 Group 2 中 Zaamo 的结论一致（HZAMO-18）
5. **htinst transformed atomic instruction**：amocas 显式访问故障 → `htinst`/`mtinst` = 0 或 transformed atomic（保留 funct5=00101/funct3/rd/rs2/aq/rl，仅 bits19:15 ← Addr. Offset），`transformedatomicinst` 格式图明文涵盖“AMO instruction”（含 amocas）；需验证框架 `hyp_transform_mem_inst()` 的 opcode 0x2F 分支对 funct5=00101 的具体处理（虽与 amoadd 等共享同一分支，仍须以 amocas 实际指令字实测确认 golden 值计算正确，不排除框架按 funct5 细化处理的可能性）
6. **Addr. Offset 恒为 0**：amocas 的自然对齐要求（`norm:Zacas_amocas_rs1_addr_alignment`，4/8/16 字节）与原子性要求单一内存操作，故 htinst 的 Addr. Offset 恒为 0（与 Group 1/Group 2 同理）
7. **MAG 放宽（跨引用 Zaamo）**：`norm:Zacas_amocas_rs1_addr_alignment` 的“the same exception options apply”明确指向 Zaamo 的对齐异常选项，故 amocas 同样适用 `norm:misaligned_atomicity_granule_size` 的 MAG 放宽分支（区别于 Group 1 Zalrsc 无 MAG 放宽）
8. **GVA/SPV 组合**：guest amocas trap → GVA=1 且 SPV=1（VS 来源 SPVP=1、VU 来源 SPVP=0）；对比 HLV/HLVX/HSV 显式访问故障 → GVA=1 但 SPV=0（与 Group 1/Group 2 共享同一结论，本组独立验证以确认 amocas 不属虚拟机访存指令）
9. **成功 CAS 必写 ⇒ D 位要求确定；失败 CAS 的 D 位副作用为 SPEC 空白**：CAS 比较成功时恒执行写回，故 `henvcfg.ADUE`=0 时对 A=0 或 D=0 的 VS-stage 页报 store page-fault（Svade 路径），**强制**（同 Group 2 的 HZAMO-27/28）；但 CAS 比较**失败**时，`zacas_failed_cas_write_note` 允许实现选择不写内存，而 Zacas SPEC **未**像 Zalrsc 的 `norm:sc_failed_side_effects` 那样明文规定此场景下 PTE D 位副作用为 UNSPECIFIED——此为 SPEC 空白点，本组按记录型处理（HZACAS-32/33）并显式标注该差异，不做强制判定
10. **henvcfg.FIOM × aq/rl**：`norm:henvcfg_fiom_order` 明文“an atomic instruction that accesses a region ordered as device I/O has its aq and/or rl bit set”涵盖 amocas（属 atomic instruction），V=1 且 FIOM=1 时带 aq/rl 的 amocas 访问 device-I/O 有序区域时排序被修改；此外 `norm:Zacas_amocas_mem_op_fail_aq_rl`（失败路径无 release 语义，无论 rl）为 Zacas 特有规则，排序公理本身需多 hart 压力测试支撑，本组仅验证可执行性与数据语义
11. **cause=22 排除**：`hypervisor.adoc` 全部 virtual-instruction 条款无一涵盖 amocas（与 LR/SC/AMO 同）→ guest amocas 绝不报 cause=22
12. **无 amocas 虚拟机等价指令**：`norm:hlsv_op` 仅映射 RV32I/RV64I 基础 load/store，amocas（opcode 0x2F，funct5=00101）无 HL*/HS* 等价指令 → HS-mode 无法以 VS/VU 有效特权对 guest 内存做原子 CAS（与 Group 1 的 HZLRSC-39/40、Group 2 的 HZAMO-30/31 共享同一架构边界结论）
13. **`hstateen0` 不门控 amocas（V=1 侧 Smstateen 非门控）**：`norm:stateen_illegal_state_access` 的门控触发条件是"执行会**读写受保护状态**的指令"，而 `norm:stateen_op` 规定 `hstateen0` 仅控制 VS/VU 对状态的访问。Zacas 无 CSR、不引入任何架构状态（仅使用通用寄存器与内存操作数），无 stateen 位可分配（`norm:stateen_unimplemented_state_roz` 可反向印证），故清零 `hstateen0` 全部位后 VS/VU-mode 执行 `amocas.w/d/q` 仍必须正常完成，且**绝不**得报 virtual-instruction (cause=22) 或 illegal-instruction (cause=2)。这是 cause=22 排除（交集点 11）的一个独立成因分支：HZACAS-02/03 验证的是"`hypervisor.adoc` 无条款对 amocas 施加 virtual-instruction 门控"，HZACAS-36 验证的是"即使 hypervisor 主动清零 `hstateen0` 试图关闭状态访问，amocas 也不因此被门控"，二者依据不同、不可相互替代

**规范依据**：
- `norm:Zacas_rv64_amocas-d_op`（代表核心 CAS 语义）：CAS 比较/交换语义在 VS/VU-mode 下不变（非虚拟化语义，含 `amocas.w`/`amocas.q` 及 RV32 寄存器对形态、编码约束、x0 配对规则，由 `Zacas_test_plan.md` 覆盖，本组仅验证虚拟化不改变核心语义）
- `norm:Zacas_amocas_w_permission` / `zacas_failed_cas_write_note`：amocas 无条件需写权限（强制），失败 CAS 的实际内存写入为实现自选（SPEC 空白点，记录型）
- `norm:mcause_exccode_st_sc_amo` / `norm:store_page_fault_no_w` / `norm:load_page_fault_no_r`：amocas 异常统一归 store/AMO 类、需 R+W 权限、绝不报 load page-fault（与 Group 2 中 Zaamo 共享同一依据，本组以 amocas 具体指令字复验）
- `norm:hedeleg_acc` / `norm:hedeleg_op` / `norm:H_vm_gpatrans`：委托路径二分与 G-stage 故障必报 store/AMO guest-page-fault
- `norm:H_vm_gpapriv`：隐式 VS-stage 遍历故障按 amocas 原始类型（store/AMO）报告 cause=23
- `norm:H_trap_xtinst_val` / `htinst_transformed_atomic` / `norm:H_trap_xtinst_guestpage` / `norm:H_trap_xtinst_guestpage_rw`：htinst 的 transformed 与伪指令两类取值规则（funct5=00101 的具体 golden 值验证）
- `norm:hstatus_gva_op` / `norm:hstatus_spv_op` / `norm:htval_trapval`：trap 现场 GVA/SPV/htval 组合
- `norm:Zacas_amocas_rs1_addr_alignment` / `norm:amo_alignment` / `norm:misaligned_atomicity_granule_size`：未对齐 amocas 的异常路径与 MAG 放宽分支（跨引用 Zaamo）
- `norm:Zacas_amocas_mem_op_success_aq_rl` / `norm:Zacas_amocas_mem_op_fail_aq_rl` / `norm:henvcfg_fiom_order` / `norm:henvcfg_adue_op`：aq/rl 位在 htinst 中的保留、FIOM 排序修改、ADUE 与 A/D 更新路径（成功 CAS 强制、失败 CAS 记录型）
- `norm:hlsv_op` / `norm:hlsv_priv` / `norm:hlsv_virtinst`：无 amocas 虚拟机等价指令的架构边界与 cause=22 排除对照
- `norm:stateen_op` / `norm:stateen_illegal_state_access` / `norm:stateen_unimplemented_state_roz`：`hstateen0` 仅门控 VS/VU 对**受保护状态**的读写，而 Zacas 无架构状态、无 stateen 位可分配，故清零 `hstateen0` 后 VS/VU-mode 的 amocas 仍必须正常执行（HZACAS-36）
- `norm:mstateen0_se0_op` / `norm:hstateen0_SE0_op`：SE0 位对 `hstateen0`/`sstateen0`/`vsstateen0` 自身访问的门控——构造 HZACAS-36 前置条件时必须保持 `mstateen0`.SE0=1，否则 HS-mode 对 `hstateen0` 的读写自身被门控

**测试职责**：验证 `amocas.w/d/q` 在 V=1 / 两阶段翻译 / HS-mode 视角下的交叉行为：异常统一归 store/AMO 类（绝不报 load 类）、**无条件写权限检查**（含 Zacas 特有的“失败 CAS 仍需权限检查”维度，与 Group 1 失败 SC 同构）、委托路径二分、隐式遍历故障按原始类型报 cause=23、htinst 的 transformed/伪指令双路径消歧（含 funct5=00101 的具体 golden 值验证）、GVA/SPV/htval trap 现场、未对齐 amocas 的 MAG 放宽分支、FIOM/ADUE 交互（含 Zacas 特有的“失败 CAS × D 位副作用”SPEC 空白点记录型用例），以及无 amocas 虚拟机等价指令的架构边界。amocas 基础原语若工具链（`-march` 含 `zacas`）支持助记符则直接使用；否则以 raw encoding 注入（AMO opcode=0x2F，funct5=00101，funct3=010(.w)/011(.d)/100(.q)，bit26=aq、bit25=rl），统一 `.option norvc` 保证 4 字节指令长度；RV32 平台的 `amocas.d` 与 RV64 平台的 `amocas.q` 须使用偶数寄存器对（rd/rd+1、rs2/rs2+1），避开 trap handler 保存的寄存器（参照 `Zacas_test_plan.md` 测试实现说明 3/4）。本组 3.1 的各特权级可执行性用例与 `Zacas_test_plan.md` Group 11（ZACAS-47~49 与 ZACAS-53，非虚拟化视角）互补——本组为 Hypervisor 交叉视角的权威来源，聚焦 cause=22 排除、异常归类与 trap 现场；3.2/3.3 中“失败 CAS 仍需写权限检查”用例（HZACAS-07/12）为 Zacas 区别于 Zaamo 的核心新增维度，与 Group 1 的 HZLRSC-22~26（失败 SC 权限检查）形成跨组呼应。**分工边界**：原 `Zacas_test_plan.md` 的 ZACAS-50/51/52（HS/VS/VU-mode 执行 amocas）已整体迁入本组 3.1（编号废弃且不再复用），原 ZACAS-53 中属 V=1 场景的 `hstateen0` 与 VS/VU 维度已迁入本组 3.8（HZACAS-36）；`Zacas_test_plan.md` 仅保留 M/S/U 非虚拟化视角，其 ZACAS-53 只清零 `mstateen0`/`sstateen0` 并只在 S/U-mode 验证。

### 3.1 HS/VS/VU-mode 正常执行与 cause=22 排除

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZACAS-01 | HS-mode 执行 amocas.w/d/q 正常 | HS-mode 对主存执行 `amocas.w`/`amocas.d`/`amocas.q`（成功路径与失败路径各一次） | 全部正常执行、rd（或寄存器对）=内存旧值、成功路径内存按 CAS 语义更新、失败路径内存不变或写回旧值（均合规），无异常 |
| HZACAS-02 | VS-mode 执行 amocas 不报 cause=22 | V=1，VS-mode 执行 `amocas.w`/`amocas.d`/`amocas.q`（目标为 VS-stage/G-stage 均有效的可读写映射） | 正常执行、语义正确；**绝不**触发 virtual-instruction exception (cause=22)（`hypervisor.adoc` 无任何条款对 amocas 施加 virtual-instruction 门控） |
| HZACAS-03 | VU-mode 执行 amocas 不报 cause=22 | 同配置，VU-mode 执行 `amocas.w`/`amocas.d`（目标页 U=1） | 正常执行，无异常，绝不报 cause=22 |
| HZACAS-04 | VS-mode amocas 语义与非虚拟化一致 | 同一 amocas 序列（含成功/失败路径、RV64 `amocas.w` 符号扩展、`amocas.q` 寄存器对语义）分别在 HS-mode 与 VS-mode 执行，比对 rd/寄存器对与内存终值 | 架构可见语义完全一致（虚拟化不改变 Zacas 指令语义，`norm:Zacas_rv64_amocas-d_op` 代表） |

### 3.2 amocas 异常统一归 store/AMO 类与 VS-stage 委托路径（含 Zacas 特有的失败 CAS 权限检查）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZACAS-05 | amocas 相关 hedeleg 位属性探测 | 逐位写 `hedeleg` bit 6/7/15 与 bit 23 并读回 | bit 6/7/15 可写（Writable）；bit 23 只读零（Read-only 0，`norm:hedeleg_acc`）——确立“VS-stage 可委托、G-stage 强制 HS-mode”二分前提（与 HZLRSC-05/HZAMO-05 结论一致，本组独立复验以保持自洽） |
| HZACAS-06 | **成功** amocas 到 R=1/W=0 VS-stage 页报 store page-fault | `hedeleg[15]`=1，VS-stage 映射目标页 R=1/W=0，VS-mode 执行 `amocas.d`（比较值预置为匹配，即成功路径） | store/AMO page fault (cause=15) 递送至 VS-mode，`vsepc`=amocas PC，`vstval`=故障 GVA（`norm:Zacas_amocas_w_permission`+`norm:store_page_fault_no_w`） |
| HZACAS-07 | **失败 CAS**（比较不匹配）到 R=1/W=0 VS-stage 页仍报 store page-fault | 同 HZACAS-06 的页表配置，但 rd 预置为与内存**不匹配**的比较值（构造必然失败的 CAS，逻辑上可能不写内存，见 `zacas_failed_cas_write_note`），VS-mode 执行 `amocas.d` | **仍**触发 store/AMO page fault (cause=15)（`norm:Zacas_amocas_w_permission`：“always requires write permissions” 为无条件语言，不因比较失败而豁免权限检查）——与 HZLRSC-23（失败 SC 仍报 store 类异常）同构，区别于 Group 2 中 Zaamo 无“失败”分支 |
| HZACAS-08 | amocas 到不可读页报 store 类而非 load 类 | VS-stage 页 R=0/W=0，VS-mode 执行 `amocas.w` | store/AMO page fault (cause=15)，**绝不**报 load page fault (cause=13)（`norm:store_page_fault_no_w` 配套 NOTE：AMO 恒报 store page-fault，含 amocas） |
| HZACAS-09 | hedeleg=0 时 amocas VS-stage 故障陷入 HS-mode | `hedeleg[15]`=0，VS-mode 触发 amocas 的 VS-stage 写权限故障（成功路径与失败路径各一次） | 均递送至 HS-mode（`norm:hedeleg_op`），cause 仍为 15（委托与否不改变 cause，仅改变递送目标；成功/失败路径的 cause 一致，验证权限检查与比较结果无关） |
| HZACAS-10 | 成功 CAS 与失败 CAS 权限要求一致（对照） | 同一 R=1/W=0 VS-stage 页，VS-mode 分别执行“比较匹配”与“比较不匹配”的 `amocas.d` | 两者**均**报 store page-fault (cause=15)，cause 与递送路径完全一致——证明 `norm:Zacas_amocas_w_permission` 的权限检查独立于 CAS 比较结果（与 HZACAS-06/07 互为印证） |

### 3.3 G-stage 故障强制 HS-mode 与 trap 现场（GVA/SPV/htval）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZACAS-11 | amocas 的 G-stage 故障强制陷入 HS-mode（成功路径） | G-stage 中目标 GPA 无写权限，VS-mode 执行 `amocas.d`（比较匹配，成功路径）；探测 `hedeleg[23]` | store/AMO guest-page fault (cause=23) 陷入 HS-mode；`hedeleg[23]` 只读零，无法委托至 VS-mode（`norm:H_vm_gpatrans`/`norm:hedeleg_acc`） |
| HZACAS-12 | **失败 CAS** 的 G-stage 写权限检查 | 同 HZACAS-11 的 G-stage 配置，但构造必然失败的比较（rd 与内存不匹配），VS-mode 执行 `amocas.d` | **仍**触发 store/AMO guest-page fault (cause=23) 陷入 HS-mode（`norm:Zacas_amocas_w_permission` 无条件适用于 G-stage 权限检查）——与 HZLRSC-24（失败 SC 的 G-stage 写权限检查）同构 |
| HZACAS-13 | guest amocas trap 的 GVA=1 且 SPV=1 | 承接 HZACAS-11/12，检查 HS-mode trap 现场 `hstatus`；另以 VU-mode 触发同故障 | VS 来源：GVA=1、SPV=1、SPVP=1；VU 来源：GVA=1、SPV=1、SPVP=0（`norm:hstatus_gva_op`/`norm:hstatus_spv_op`），`stval`=故障 GVA |
| HZACAS-14 | guest amocas G-stage 故障时 htval=GPA>>2 或 0 | 承接 HZACAS-11，检查 `htval` | `htval`=故障 GPA>>2 或 0（基线 H `norm:htval_trapval` 允许写零；非零时必须为 GPA>>2）。注：Shtvala 实现下收紧为必须非零，由 `Shtvala_test_plan.md` 覆盖（本组不重复该强化断言） |
| HZACAS-15 | VS-stage amocas 故障时 htval=0 | 承接 HZACAS-09（cause=15 陷入 HS-mode），检查 `htval` | `htval`=0（`norm:htval_trapval`：仅 guest-page fault 写 GPA，其余 trap 写零）——与 HZACAS-14 形成对比 |
| HZACAS-16 | 对比 HLV/HSV 的 SPV/GVA 组合 | HS-mode（V=0）设 `hstatus.SPVP`=1 执行 `HSV.D` 触发 guest-page fault，检查 `hstatus` | SPV=0 但 GVA=1（`norm:hstatus_gva_op` NOTE 的唯一例外）；与 HZACAS-13 的 SPV=1/GVA=1 对比，证明 amocas 不属虚拟机访存指令（与 HZLRSC-14/HZAMO-14 共享同一结论） |

### 3.4 htinst：transformed atomic instruction 与伪指令消歧

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZACAS-17 | guest amocas 显式访问故障的 htinst | VS-mode 执行 `amocas.d` 触发 G-stage **显式**访问故障（cause=23），读 `htinst` 与 `vsepc` 处指令字比对 | `htinst`=0 或 transformed atomic instruction（`norm:H_trap_xtinst_val`）；非零时必须等于“原指令字保留全部字段、仅 bits19:15 替换为 Addr. Offset”（`htinst_transformed_atomic`，golden 由 `hyp_transform_mem_inst()` opcode 0x2F 分支计算，须保留 **funct5=00101**、funct3、rd、rs2） |
| HZACAS-18 | htinst 保留 aq/rl 位 | VS-mode 分别执行 `amocas.w.aq`、`amocas.w.rl`、`amocas.w.aqrl`（raw encoding 或助记符）触发 G-stage 故障，比对 `htinst` 的 bit26/bit25 | transformed 值的 aq(bit26)/rl(bit25) 与陷入指令逐位一致（`htinst_transformed_atomic`：保留除 bits19:15 外全部字段） |
| HZACAS-19 | amocas 的 htinst Addr. Offset 恒为 0 | 承接 HZACAS-17，检查 `htinst` bits19:15 | Addr. Offset=0——amocas 原子性要求单一内存操作、不跨页拆分（MAG 放宽时为单内存操作、未放宽时直接异常），故 faulting VA 等于 original VA |
| HZACAS-20 | 隐式 VS-stage 遍历故障按 amocas 原始类型报 cause=23 | 构造 VS-stage 叶页表页在 G-stage 无效，VS-mode 执行 `amocas.w`，使其翻译在**隐式读 VS-stage PTE** 时故障 | cause=**23**（store/AMO guest-page fault，按 amocas 原始访问类型报告，`norm:H_vm_gpapriv`）陷入 HS-mode；当 `htval` 非零（=该 PTE 的 GPA>>2）时 `htinst` **必须**为读伪指令（RV64=0x00003000）、**不允许为 0**（`norm:H_trap_xtinst_guestpage`） |
| HZACAS-21 | A/D 自动更新故障的写伪指令（ADUE=1，成功 CAS） | `henvcfg.ADUE`=1，构造 VS-stage PTE 的 D=0（CAS 比较匹配，为写访问）且该 PTE 页在 G-stage 无写权限，VS-mode 执行 `amocas.d` | `htinst`=写伪指令 0x00003020（`norm:H_trap_xtinst_guestpage_rw`：A/D 自动更新用 write 伪指令）；`htval`=该 PTE 的 GPA>>2；cause=23 |
| HZACAS-22 | 显式与隐式故障的 htinst/htval 消歧 | 对**同一 cause=23**，分别构造“amocas 自身数据访问在 G-stage 失败”与“amocas 的 VS-stage PTE 读在 G-stage 失败”两种场景，比对 `htinst` 与 `htval` | 前者 `htinst`=0 或 transformed atomic、`htval`=目标数据 GPA>>2；后者 `htinst`=伪指令（非零强制）、`htval`=PTE GPA>>2。htinst 为唯一消歧手段（与 HZAMO-20 同构，本组以 amocas 指令字复验） |

### 3.5 未对齐 amocas 与 MAG 放宽

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZACAS-23 | VS-mode 未对齐 amocas 无 MAG 覆盖的异常与委托 | VS-stage/G-stage 均有效，VS-mode 对 2 字节对齐（非 4 字节）地址执行 `amocas.w`（trap-armed），`hedeleg[6]`/`[7]` 分别置 0 与 1 | 触发 store/AMO address misaligned (cause=6) 或 store/AMO access fault (cause=7)（二者之一均合规，`norm:Zacas_amocas_rs1_addr_alignment` 跨引用 `norm:amo_alignment`；**非** load 类 4/5）；`hedeleg` 置 1 时递送 VS-mode、置 0 时陷入 HS-mode |
| HZACAS-24 | MAG 粒度内未对齐 amocas 无异常且原子执行 | 平台声明 MAG，VS-mode 对全部字节位于同一 MAG 粒度内的未对齐地址执行 `amocas.w` | **无**对齐异常、正常执行且语义正确（`norm:misaligned_atomicity_granule_size`，经 `norm:Zacas_amocas_rs1_addr_alignment` 的“the same exception options apply”跨引用适用于 amocas）；平台未声明 MAG 时本用例 SKIP |
| HZACAS-25 | MAG 内未对齐 amocas 的 G-stage 故障 | 承接 HZACAS-24 场景，令目标 GPA 在 G-stage 无写权限 | cause=23 陷入 HS-mode，`htval`=GPA>>2 或 0，`htinst` Addr. Offset=0（单一内存操作，不拆分） |
| HZACAS-26 | 未对齐 amocas 陷入 HS-mode 时的现场 | `hedeleg[6]`=0，承接 HZACAS-23，检查 HS-mode trap 现场 | cause=6 或 7；`stval`=**未对齐地址本身**（等于 original VA）；GVA=1、SPV=1；`htval`=0（非 guest-page fault）；`htinst`=0 或 transformed atomic 且 Addr. Offset=0 |

### 3.6 henvcfg.FIOM 与 ADUE 交互（含 Zacas 特有的失败 CAS × D 位记录型用例）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZACAS-27 | FIOM=1 时 VS-mode 带 aq/rl 的 amocas | `henvcfg.FIOM`=1，VS-mode 对平台声明为 device-I/O 有序的区域执行 `amocas.w.aq`/`amocas.d.rl`（trap-armed） | 指令可执行且数据语义不变；排序被修改为同时约束 device I/O 与内存（`norm:henvcfg_fiom_order`）——排序效果需多 hart 观测，单 hart 仅验证可执行性与数据语义 |
| HZACAS-28 | FIOM=0 对照 | `henvcfg.FIOM`=0，同场景执行相同指令 | 排序不被修改（对照），指令可执行且数据语义与 HZACAS-27 一致 |
| HZACAS-29 | ADUE=0 时 amocas 到 A=0 VS-stage 页（成功路径） | `henvcfg.ADUE`=0，VS-stage PTE A=0，VS-mode 执行 `amocas.w`（比较匹配） | VS-stage 按 Svade 行为报 store page-fault (cause=15) 而非硬件更新 A 位（`norm:henvcfg_adue_op`）；递送目标按 `hedeleg[15]`；不得误判为 guest-page fault (cause 21/23) |
| HZACAS-30 | ADUE=0 时**成功** amocas 到 A=1/D=0 页 | `henvcfg.ADUE`=0，VS-stage PTE A=1/D=0，VS-mode 执行 `amocas.d`（比较匹配，必然写回） | store page-fault (cause=15)——成功 CAS **恒执行写回**，D 位要求为**强制**（同 HZAMO-28，非 UNSPECIFIED）；`norm:henvcfg_adue_op`：ADUE=0 时按 Svade 报页错误 |
| HZACAS-31 | ADUE=1 时 amocas 硬件更新 A/D 位后正常完成（成功路径） | `henvcfg.ADUE`=1（且平台实现 Svadu），VS-stage PTE A=0/D=0，VS-mode 执行 `amocas.w`（比较匹配） | 硬件自动置 A/D 位，amocas 正常完成、无页错误；回读 PTE A=1/D=1（`norm:henvcfg_adue_op`）；平台未实现 Svadu 时 SKIP |
| HZACAS-32 | **（记录型）ADUE=1 时失败 CAS（比较不匹配）的 VS-stage D 位副作用** | `henvcfg.ADUE`=1，VS-stage PTE D=0 且 W=1，构造必然失败的 CAS（rd 与内存不匹配）指向该页，VS-mode 执行 `amocas.d`，回读 PTE 的 D 位 | D 位置位或保持**均记录为合规**（`zacas_failed_cas_write_note` 允许失败 CAS 不写内存，但 **Zacas SPEC 未像 Zalrsc 的 `norm:sc_failed_side_effects` 那样明文规定此场景为 UNSPECIFIED**——此为 SPEC 空白点，不做强制判定，仅记录实现行为并在报告中显式标注该规范差异） |
| HZACAS-33 | **（记录型）ADUE=1 时失败 CAS 的 G-stage D 位副作用** | 同 HZACAS-32，另回读对应 G-stage PTE 的 D 位 | 同为 SPEC 空白点（Zacas 无明文 UNSPECIFIED 声明，区别于 HZLRSC-33 有 `norm:sc_failed_side_effects` 依据）；记录实现行为，不做强制判定 |

### 3.7 架构边界：H 扩展无 amocas 虚拟机等价指令

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZACAS-34 | （记录型）无 HL*/HS* amocas 等价指令 | 以 trap-armed 探测 HS-mode 下是否存在按 `norm:hlsv_priv` 语义（有效特权由 `hstatus.SPVP` 决定）执行原子 CAS 的指令编码 | 记录性：`norm:hlsv_op` 仅映射 RV32I/RV64I 的 LB..LD / SB..SD，amocas（opcode 0x2F，funct5=00101）无对应虚拟机指令。与 HZLRSC-39/HZAMO-30 共享同一架构边界结论；探测到的任何“虚拟机原子指令”编码须与 SPEC 比对后报告 |
| HZACAS-35 | （记录型）HLV+HSV 无法替代 amocas 原子性 | HS-mode 以 `HLV.D`+`HSV.D` 序列对 guest 内存做比较交换的软件模拟，与 VS-mode 内的 `amocas.d` 并发（需多 hart） | 记录性：HLV+HSV 为两次独立非原子访问、且软件模拟的“比较-交换”序列在并发下存在 TOCTOU 竞争窗口（无硬件原子性保证），可观测到丢失更新或比较结果被绕过；多 hart 未就绪时 SKIP（与 HZLRSC-40/HZAMO-31 同理） |

### 3.8 `hstateen0` 不门控 amocas（V=1 侧 Smstateen 非门控）

> **编号说明**：本子节与 HZACAS-36 为后补用例，承接自 `Zacas_test_plan.md` 原 ZACAS-53中属 V=1 场景的两个维度（清零 `hstateen0`、VS/VU-mode 执行）。因 HZACAS-01~35 编号已分配完毕且**不得重排或复用**（维持历史用例编号与测试报告的稳定对应），新用例取 ID 末尾的 HZACAS-36 并置于本组最后一个子节；其主题（可执行性与门控排除）与 3.1 相近，阅读时可与 3.1 对照。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZACAS-36 | 清零 `hstateen0` 后 VS/VU-mode 仍正常执行 amocas | 先确认 Smstateen 已实现且 `mstateen0`/`hstateen0` 可读写，**保持 `mstateen0`.SE0=1**（`norm:mstateen0_se0_op`：SE0=0 时 HS-mode 对 `hstateen0` 的访问自身被门控，前置条件无法建立）；保存并清零 `hstateen0` 全部位，回读确认为 0（证明清零已生效、门控已处于最严状态）；VS-stage/G-stage 均映射为可读写（VU 目标页 U=1），分别在 VS-mode 与 VU-mode 执行 `amocas.w`/`amocas.d`/`amocas.q`（成功路径与失败路径各一次），完毕后恢复 `hstateen0` | 全部正常执行，rd（或寄存器对）=内存旧值、成功路径内存按 CAS 语义更新、失败路径内存不变或写回旧值（均合规），与非虚拟化语义一致；**绝不**报 virtual-instruction (cause=22) 或 illegal-instruction (cause=2)——依据 `norm:stateen_op` + `norm:stateen_illegal_state_access`：门控仅适用于"读写受保护状态"的指令，Zacas 无 CSR、不引入架构状态，无 stateen 位可分配，故门控条件不成立（与 HZACAS-02/03 的区别见交集点 13）。若 `hstateen0` 回读非 0（平台将其硬接为只读零以外的值但写入未生效），则前置条件不成立，用例 SKIP 并注明原因，不得当作 PASS |

> [!NOTE]
> - 本组所有测试必须以平台配置 `H_SUPPORTED` 宏为准检测 H 扩展；Zacas 支持以平台配置 `ZACAS_SUPPORTED` 宏为准，并以 trap-armed 执行 amocas 二次探测，不满足时全组 TEST_SKIP；Zacas 依赖 Zaamo（进而依赖 A 扩展），故本组以 Group 2 的 Zaamo 探测结论为隐式前提（`ZAAMO_SUPPORTED`/`A_SUPPORTED` 亦须满足）。
> - **异常类别断言必须精确**：amocas 的**全部**异常归 store/AMO 类（misaligned=6、access fault=7、page fault=15、guest-page fault=23），依据 `norm:mcause_exccode_st_sc_amo`（amocas 属 AMO 指令族）与 `norm:store_page_fault_no_w` 配套 NOTE。断言**绝不**得接受 load 类（4/5/13/21）。
> - **无条件写权限检查为 Zacas 特有强制断言**：HZACAS-07/12（失败 CAS 仍报 store 类异常）依据 `norm:Zacas_amocas_w_permission` 的“always”无条件语言，为**强制**判定（非记录型）——这与 HZACAS-32/33（失败 CAS 的 D 位副作用，SPEC 空白点，记录型）性质不同，不得混淆：前者是权限**检查**（W 位/G-stage 写权限），SPEC 明文强制；后者是权限检查通过后 D 位**自动更新**的副作用，Zacas SPEC 未明文规定（区别于 Zalrsc 的 `norm:sc_failed_side_effects` 明文 UNSPECIFIED）。
> - **cause=22 为强制负向断言**：HZACAS-02/03 中 guest amocas 若报 virtual-instruction exception 即违反 SPEC（`hypervisor.adoc` 无任何条款对 amocas 施加 virtual-instruction 门控），应保持失败并记录至 `bugs/` 目录，不得放宽断言。
> - **htinst golden 值计算**：框架 `hyp_transform_mem_inst()` 对 opcode 0x2F 已实现“Atomic (LR/SC/AMO)：保留除 bits19:15 外全部字段”，理论上涵盖 funct5=00101（amocas）；HZACAS-17~19 须以 amocas 实际指令字实测确认该分支对 funct5=00101 的 golden 值计算正确（不排除框架实现按 funct5 分支细化处理的可能性，须实测验证而非假定复用即可）；HZACAS-20/21 的隐式遍历场景可复用 `setup_implicit_walk_victim()`。aq/rl 变体（HZACAS-18/27）若框架无对应原语须以 raw encoding 注入或补充原语。
> - **htinst 伪指令的强制性**：HZACAS-20 中当 `htval` 非零且故障源于 VS-stage 隐式访问时，`htinst` 写 0 属违反 `norm:H_trap_xtinst_guestpage`（“zero is not allowed”），应保持失败；HZACAS-17 的显式访问场景下 `htinst` 写 0 是允许的。
> - **MAG 放宽经跨引用适用于 amocas**：HZACAS-24/25 依赖平台声明 misaligned atomicity granule（`norm:misaligned_atomicity_granule_size`，经 `norm:Zacas_amocas_rs1_addr_alignment` 的“the same exception options apply”跨引用）；未声明 MAG 时未对齐 amocas 必产生 cause=6/7（HZACAS-23），HZACAS-24/25 SKIP。未对齐异常允许 address-misaligned 与 access-fault 两类合规结果，用例验证 cause 属预期集合并记录实际值，不得因平台选择其中一类而判 FAIL。
> - **成功 CAS 必写 ⇒ D 位强制；失败 CAS 的 D 位为 SPEC 空白**：HZACAS-30（ADUE=0 + 成功 CAS + D=0 → cause=15）为**强制**结果，同 HZAMO-28；HZACAS-32/33（ADUE=1 + 失败 CAS + D 位副作用）为**记录型**，因 Zacas SPEC 未明文规定（区别于 HZLRSC-32/33 有 `norm:sc_failed_side_effects` 明文 UNSPECIFIED 依据），不得对 HZACAS-32/33 做强制判定，亦不得因缺乏明文依据而将其误判为“必然置位”或“必然不置位”。
> - **记录型用例不得强制判定**：HZACAS-32/33（失败 CAS 的 PTE D 位副作用，SPEC 空白点）、HZACAS-34/35（无 amocas 虚拟机等价指令的架构边界）仅记录实现选择。
> - **多 hart 限制**：HZACAS-27/28 的 FIOM 排序效果断言与 HZACAS-35 需 secondary hart 支持；当前公共框架仅 hart 0 运行，相关断言在框架就绪前 SKIP（非 FAIL），单 hart 平台同样 SKIP 并注明。`norm:Zacas_amocas_mem_op_fail_aq_rl`（失败路径无 release 语义）的跨 hart 排序验证同样受此限制，本组仅做单 hart 数据语义检查。
> - **权限用例依赖**：3.2/3.3/3.6 需 VS-stage（`vsatp`）与 G-stage（`hgatp`）页表构造能力（R=0、R=1/W=0、A=0、D=0、G-stage 无写权限等映射组合）；权限检查机制本身由 `pmp_test_plan.md`/`vm_test_plan.md` 覆盖，本组仅验证 amocas 的权限语义结论。
> - **寄存器对与 RV32/RV64 适配**：`amocas.d`(RV32)/`amocas.q`(RV64) 要求 rd/rs2 为偶数起始寄存器对，实现时须避开 trap handler 保存/使用的寄存器（参照 `Zacas_test_plan.md` 测试实现说明 3/4）；RV32 平台不适用 `amocas.q` 相关用例（HZACAS-01~04 中的 `.q` 分支），RV64 平台不适用 RV32 `amocas.d` 寄存器对形态（本组以 RV64 为主要目标，与 `Zacas_test_plan.md` 一致）。
> - Zacas 非虚拟化语义（`amocas.w/d/q` 编码、寄存器对偶数约束、x0 配对读写、RV32/RV64 宽度差异、比较/交换基础语义、aq/rl 数据语义、M/S/U 各特权级基础可执行性、以及清零 `mstateen0`/`sstateen0` 后 S/U-mode 的 Smstateen 非门控验证即 ZACAS-53）由 `Zacas_test_plan.md` 覆盖；主存 AMOCASQ 级 PMA 支持保证由 `Ziccamoc_test_plan.md` 覆盖，本组不重复。**本组为该扩展 Hypervisor 交叉视角的唯一权威来源**：`Zacas_test_plan.md` 已不含任何虚拟化用例（原 ZACAS-50/51/52 编号废弃，原 ZACAS-53 的 V=1 维度迁为 HZACAS-36）。
> - **HZACAS-36 的前置条件与对照**：清零 `hstateen0` 前必须保持 `mstateen0`.SE0=1（否则 HS-mode 读写 `hstateen0` 自身就被 `norm:mstateen0_se0_op` 门控），且必须**回读确认 `hstateen0` 已为 0**——缺少这一步则“amocas 仍正常执行”可能仅因清零未生效而平凡通过，不构成对 `norm:stateen_illegal_state_access` 的有效验证。平台未实现 Smstateen（`mstateen0`/`hstateen0` 不存在）或 `hstateen0` 写入不生效时，本用例 SKIP 并注明原因，不得当作 PASS；用例结束后必须恢复 `hstateen0` 原值，避免污染后续用例。

---

## Group 4. Hypervisor × Zabha 交叉测试

**与 Hypervisor 的交集点**：
1. **异常统一归 store/AMO 类**：字节/半字 AMO（`amoadd.b/h` 等）与 Zaamo 的 AMO 同属 opcode 0x2F 指令族，仅 funct3=000(.b)/001(.h) 区别于 word/dword 的 010/011，`norm:mcause_exccode_st_sc_amo` 明文“AMO instructions generate store/AMO exceptions”涵盖全部宽度，故其全部异常（misaligned=6、access fault=7、page fault=15、guest-page fault=23）**绝不**报 load 类（4/5/13/21），与 Group 2 中 Zaamo 结论一致，本组以字节/半字具体指令字复验
2. **AMO 需 R+W 权限（宽度无关）**：字节与半字 AMO 对 R=1/W=0 或 R=0 页均报 store page-fault (15)（VS-stage）或 store guest-page-fault (23)（G-stage）；权限要求与访问宽度（.b/.h/.w/.d）无关
3. **委托路径二分**：VS-stage byte/halfword AMO 故障（cause 6/7/15）的 `hedeleg` 位为 Writable，可下放 VS-mode；G-stage 故障（cause 23）的 `hedeleg[23]` 为 Read-only 0，**架构强制**陷入 HS-mode（`norm:hedeleg_acc`、`norm:H_vm_gpatrans`）——与 Group 1/2/3 共享同一委托机制，本组独立探测以确立前提（保持每组自洽）
4. **隐式 VS-stage 遍历故障按 AMO 原始类型报 cause=23**：`norm:H_vm_gpapriv` — 隐式 PTE 访问按 implicit load/store 检查权限，但异常“always reported for the original access type”，故字节/半字 AMO 的隐式遍历 GPF 报 store/AMO guest-page fault (cause=23)，与 Group 2/3 一致
5. **htinst transformed atomic instruction 须保留 funct3 宽度编码（Zabha 特有可测维度）**：byte/halfword AMO 显式访问故障 → `htinst`/`mtinst` = 0 或 transformed atomic（保留 funct5/aq/rl/**funct3=000(.b)/001(.h)**/rd/rs2，仅 bits19:15 ← Addr. Offset）；funct3 位于 bits14:12，被 `htinst_transformed_atomic`（“all fields ... except bits 19:15”）保留，使 HS-mode 能从 htinst 判定陷入访问的**字节/半字宽度**——这是 word/dword AMO（Group 2）不具备的、Zabha 独有的 htinst 信息维度，须实测确认框架 `hyp_transform_mem_inst()` 的 opcode 0x2F 分支正确保留 funct3=000/001
6. **Addr. Offset 恒为 0（Zabha 特有的对齐理由）**：字节 AMO 因 1 字节对齐**恒成立**、永不未对齐、永不拆分；半字 AMO 未对齐时直接异常（不拆分）或 MAG 放宽（单一内存操作），两种情形 faulting VA 均等于 original VA，故 htinst 的 Addr. Offset 恒为 0
7. **MAG 放宽仅半字 AMO 可测（Zabha 特有）**：`norm:Zabha_rs1_align_addr` 的“the same exception options as Zaamo”跨引用 `norm:amo_alignment` 与 `norm:misaligned_atomicity_granule_size`；但**字节 AMO 无未对齐场景**（1 字节对齐恒成立），故 misaligned 异常与 MAG 放宽分支**仅对半字 AMO（奇地址）可测**——这是 Zabha 区别于 Zaamo（word/dword 均可未对齐）的关键收窄
8. **rd 符号扩展至 8/16 位与忽略 rs2 高位在虚拟化下不变（Zabha 特有 norm）**：`norm:Zabha_rd_sign_extension` — 字节 AMO 将 8 位旧值符号扩展入 rd、忽略 rs2[XLEN-1:8]；半字 AMO 将 16 位旧值符号扩展入 rd、忽略 rs2[XLEN-1:16]；该数据语义在 VS/VU-mode 下必须与 HS-mode 一致
9. **GVA/SPV 组合**：guest byte/halfword AMO trap → GVA=1 且 SPV=1（VS 来源 SPVP=1、VU 来源 SPVP=0）；对比 HLV.B/HLV.H/HSV.B/HSV.H 显式访问故障 → GVA=1 但 SPV=0
10. **AMO 必写 ⇒ D 位要求确定**：字节/半字 AMO 恒执行写回，故 `henvcfg.ADUE`=0 时对 A=0 或 D=0 的 VS-stage 页报 store page-fault（Svade 路径），**强制**（同 Group 2 的 HZAMO-27/28，非 UNSPECIFIED）
11. **henvcfg.FIOM × aq/rl**：`norm:henvcfg_fiom_order` 明文“an atomic instruction ... has its aq and/or rl bit set”涵盖字节/半字 AMO，V=1 且 FIOM=1 时带 aq/rl 的 byte/halfword AMO 访问 device-I/O 有序区域时排序被修改
12. **cause=22 排除**：`hypervisor.adoc` 全部 virtual-instruction 条款无一涵盖 byte/halfword AMO → guest 字节/半字 AMO 绝不报 cause=22（与 LR/SC/AMO/CAS 同）
13. **amocas.b/h（Zabha × Zacas 交集）**：若实现 Zacas，`amocas.b/h`（funct5=00101、funct3=000/001）兼具 Zabha 的宽度语义与 Zacas 的 CAS 语义——`norm:Zabha_amocas-BH_ignore_bits`（比较仅用 rd[7:0]/rd[15:0]、忽略高位）在 VS/VU-mode 下不变；`norm:Zacas_amocas_w_permission`（恒需写权限）使**失败 CAS**（比较不匹配）指向无写权限页时仍报 store 类异常（与 Group 3 的 HZACAS-07/12 同构，本组以字节/半字宽度复验）
14. **保留字节/半字 lr/sc 编码在 V=1 报 cause=2 而非 cause=22（Zabha 特有）**：Zabha NOTE 明确“omits byte and halfword support for lr and sc”，对应 funct3=000/001 的 LR/SC 编码为保留编码；`norm:H_cause_virtual_instruction` 规定 virtual-instruction (cause=22) **仅**替代 HS-qualified 但 V=1 受阻的指令，而保留编码在所有模式（含 HS/M）均非法（非 HS-qualified），故 V=1 时执行报 illegal-instruction (cause=2) 而非 cause=22
15. **无字节/半字原子虚拟机等价指令，但 HLV.B/HLV.H/HSV.B/HSV.H 提供非原子访存（Zabha 特有的更丰富架构边界）**：`norm:hlsv_op` 明文映射 LB/LBU/LH/LHU → HLV.B/HLV.BU/HLV.H/HLV.HU、SB/SH → HSV.B/HSV.H，故 HS-mode **可**以 VS/VU 有效特权（`norm:hlsv_priv`）对 guest 内存做字节/半字访问，但这些是**非原子**的单次 load/store，无法替代字节/半字 AMO 的原子 RMW（对比 Group 2/3 仅有 word/dword 的 HLV.W/HLV.D 对照，Zabha 的字节/半字对照更直接）

**规范依据**：
- `norm:Zabha_rd_sign_extension` / `norm:Zabha_amocas-BH_ignore_bits`：字节/半字 AMO 与 `amocas.b/h` 的 rd 符号扩展（8/16 位）与 rs2/rd 高位忽略在 VS/VU-mode 下不变（非虚拟化语义由 `Zabha_test_plan.md` 覆盖，本组仅验证虚拟化不改变语义）
- `norm:Zabha_rs1_align_addr` / `norm:amo_alignment` / `norm:misaligned_atomicity_granule_size`：字节/半字 AMO 自然对齐要求与跨引用 Zaamo 的异常选项、MAG 放宽（字节无未对齐场景、仅半字可测）
- `zabha_no_byte_halfword_lrsc` / `norm:H_cause_virtual_instruction`：保留字节/半字 lr/sc 编码在 V=1 报 illegal-instruction (cause=2) 而非 virtual-instruction (cause=22)
- `norm:mcause_exccode_st_sc_amo` / `norm:store_page_fault_no_w` / `norm:load_page_fault_no_r`：byte/halfword AMO 异常统一归 store/AMO 类、需 R+W 权限、绝不报 load page-fault（与 Group 2 共享依据，本组以字节/半字指令字复验）
- `norm:hedeleg_acc` / `norm:hedeleg_op` / `norm:H_vm_gpatrans`：委托路径二分与 G-stage 故障必报 store/AMO guest-page-fault
- `norm:H_vm_gpapriv`：隐式 VS-stage 遍历故障按 AMO 原始类型（store/AMO）报告 cause=23
- `norm:H_trap_xtinst_val` / `htinst_transformed_atomic` / `norm:H_trap_xtinst_guestpage` / `norm:H_trap_xtinst_guestpage_rw`：htinst 的 transformed（funct3=000/001 保留）与伪指令两类取值规则
- `norm:hstatus_gva_op` / `norm:hstatus_spv_op` / `norm:htval_trapval`：trap 现场 GVA/SPV/htval 组合
- `norm:amo_release_consistency` / `norm:henvcfg_fiom_order` / `norm:henvcfg_adue_op`：aq/rl 位在 htinst 中的保留、FIOM 排序修改、ADUE 与 A/D 更新路径
- `norm:Zacas_amocas_w_permission`（amocas.b/h 条件）：`amocas.b/h` 无条件需写权限，失败 CAS 仍报 store 类异常
- `norm:hlsv_op` / `norm:hlsv_priv` / `norm:hlsv_virtinst`：HLV.B/HLV.H/HSV.B/HSV.H 非原子字节/半字 guest 访存的架构边界与 cause=22 排除对照

**测试职责**：验证字节/半字 AMO（及 Zacas 条件下的 `amocas.b/h`）在 V=1 / 两阶段翻译 / HS-mode 视角下的交叉行为：异常统一归 store/AMO 类（绝不报 load 类）、R+W 权限要求（宽度无关）、委托路径二分、隐式遍历故障按原始类型报 cause=23、htinst 的 transformed/伪指令双路径消歧（**含 funct3=000/001 宽度编码保留**这一 Zabha 特有维度）、GVA/SPV/htval trap 现场、rd 符号扩展与 rs2 高位忽略在虚拟化下不变、**字节 AMO 永不未对齐而半字 AMO 独有 misaligned/MAG 分支**、FIOM/ADUE 交互、`amocas.b/h` 的比较高位忽略与失败 CAS 权限检查、**保留字节/半字 lr/sc 编码报 cause=2 而非 cause=22**，以及 HLV.B/HLV.H/HSV.B/HSV.H 非原子访存的架构边界。字节/半字 AMO 若工具链（`-march` 含 `zabha`）支持助记符则直接使用；否则以 raw encoding 注入（AMO opcode=0x2F，funct5：amoadd=00000/amoswap=00001/amoxor=00100/amoor=00110/amoand=00111/amomin=01000/amomax=01001/amominu=01100/amomaxu=01101/amocas=00101，**funct3=000(.b)/001(.h)**，bit26=aq、bit25=rl）；保留字节/半字 lr/sc 以 raw encoding 注入（opcode=0x2F，funct5=00010(lr)/00011(sc)，funct3=000/001）；统一 `.option norvc` 保证 4 字节指令长度。`common/mem_ops.h` 若仅提供 `.w`/`.d` 原语，则字节/半字原语须补充或以 raw encoding 注入。本组 4.1 的各特权级可执行性用例与 `Zabha_test_plan.md`（非虚拟化视角）互补——本组为 Hypervisor 交叉视角的权威来源，聚焦 cause=22 排除、异常归类、funct3 宽度保留与 trap 现场；4.7 的 `amocas.b/h` 为 Zabha × Zacas 交集，与 Group 3 的 `amocas.w/d/q` 形成宽度互补。

### 4.1 HS/VS/VU-mode 正常执行与 cause=22 排除

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZABHA-01 | HS-mode 执行全部字节/半字 AMO 正常 | HS-mode 对主存执行 9 条 AMO 的 `.b` 与 `.h` 变体（共 18 条，自然对齐） | 全部正常执行、rd=符号扩展后的旧值、目标字节/半字按运算更新、相邻内存不被破坏，无异常 |
| HZABHA-02 | VS-mode 执行字节/半字 AMO 不报 cause=22 | V=1，VS-mode 执行 `.b`/`.h` AMO 集合（目标为 VS-stage/G-stage 均有效的可读写映射） | 正常执行、语义正确；**绝不**触发 virtual-instruction exception (cause=22)（`hypervisor.adoc` 无任何条款对 byte/halfword AMO 施加 virtual-instruction 门控） |
| HZABHA-03 | VU-mode 执行字节/半字 AMO 不报 cause=22 | 同配置，VU-mode 执行 `.b`/`.h` AMO 集合（目标页 U=1） | 正常执行，无异常，绝不报 cause=22 |
| HZABHA-04 | VS-mode 字节/半字 AMO 语义与非虚拟化一致 | 同一 byte/halfword AMO 序列分别在 HS-mode 与 VS-mode 执行，比对 rd 的 8/16 位符号扩展、rs2 高位忽略（`norm:Zabha_rd_sign_extension`）与内存终值 | 架构可见语义完全一致（虚拟化不改变 Zabha 指令语义）；rd 高位符号扩展正确、rs2[XLEN-1:8]（.b）/rs2[XLEN-1:16]（.h）被忽略 |

### 4.2 byte/halfword AMO 异常统一归 store/AMO 类与 VS-stage 委托路径

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZABHA-05 | byte/halfword AMO 相关 hedeleg 位属性探测 | 逐位写 `hedeleg` bit 6/7/15 与 bit 23 并读回 | bit 6/7/15 可写（Writable）；bit 23 只读零（Read-only 0，`norm:hedeleg_acc`）——确立“VS-stage 可委托、G-stage 强制 HS-mode”二分前提（与 HZLRSC-05/HZAMO-05/HZACAS-05 结论一致，本组独立复验以保持自洽） |
| HZABHA-06 | byte/halfword AMO 到 R=1/W=0 VS-stage 页报 store page-fault | `hedeleg[15]`=1，VS-stage 映射目标页 R=1/W=0，VS-mode 执行 `amoadd.b` 与 `amoadd.h` | store/AMO page fault (cause=15) 递送至 VS-mode，`vsepc`=AMO PC，`vstval`=故障 GVA（`norm:store_page_fault_no_w`：AMO 需写权限）；字节与半字宽度结果一致 |
| HZABHA-07 | byte/halfword AMO 到不可读页报 store 类而非 load 类 | VS-stage 页 R=0/W=0，VS-mode 执行 `amoadd.b`/`amoadd.h` | store/AMO page fault (cause=15)，**绝不**报 load page fault (cause=13)（`norm:store_page_fault_no_w` 配套 NOTE：AMO 恒报 store page-fault，含字节/半字宽度） |
| HZABHA-08 | hedeleg=0 时 byte/halfword AMO VS-stage 故障陷入 HS-mode | `hedeleg[15]`=0，VS-mode 触发字节与半字 AMO 的 VS-stage 写权限故障 | 均递送至 HS-mode（`norm:hedeleg_op`），cause 仍为 15（委托与否不改变 cause，仅改变递送目标） |
| HZABHA-09 | 字节与半字 AMO 权限要求一致（宽度无关，对照 word AMO） | 同一 R=1/W=0 VS-stage 页，VS-mode 分别执行 `amoadd.b`、`amoadd.h` 与（对照）`amoadd.w` | 三者**均**报 store page-fault (cause=15)——证明 store/AMO 权限要求与访问宽度（.b/.h/.w）无关（与 HZAMO-06 的 word 结论一致） |

### 4.3 G-stage 故障强制 HS-mode 与 trap 现场（GVA/SPV/htval）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZABHA-10 | byte/halfword AMO 的 G-stage 故障强制陷入 HS-mode | G-stage 中目标 GPA 无写权限，VS-mode 执行 `amoadd.b` 与 `amoadd.h`；探测 `hedeleg[23]` | store/AMO guest-page fault (cause=23) 陷入 HS-mode；`hedeleg[23]` 只读零，无法委托至 VS-mode（`norm:H_vm_gpatrans`/`norm:hedeleg_acc`） |
| HZABHA-11 | guest byte/halfword AMO trap 的 GVA=1 且 SPV=1 | 承接 HZABHA-10，检查 HS-mode trap 现场 `hstatus`；另以 VU-mode 触发同故障 | VS 来源：GVA=1、SPV=1、SPVP=1；VU 来源：GVA=1、SPV=1、SPVP=0（`norm:hstatus_gva_op`/`norm:hstatus_spv_op`），`stval`=故障 GVA |
| HZABHA-12 | guest byte/halfword AMO G-stage 故障时 htval=GPA>>2 或 0 | 承接 HZABHA-10，检查 `htval` | `htval`=故障 GPA>>2 或 0（基线 H `norm:htval_trapval` 允许写零；非零时必须为 GPA>>2）。注：Shtvala 实现下收紧为必须非零，由 `Shtvala_test_plan.md` 覆盖（本组不重复该强化断言） |
| HZABHA-13 | VS-stage byte/halfword AMO 故障时 htval=0 | 承接 HZABHA-08（cause=15 陷入 HS-mode），检查 `htval` | `htval`=0（`norm:htval_trapval`：仅 guest-page fault 写 GPA，其余 trap 写零）——与 HZABHA-12 形成对比 |
| HZABHA-14 | 对比 HLV.B/HSV.B 的 SPV/GVA 组合 | HS-mode（V=0）设 `hstatus.SPVP`=1 执行 `HSV.B`（字节宽度虚拟机 store）触发 guest-page fault，检查 `hstatus` | SPV=0 但 GVA=1（`norm:hstatus_gva_op` NOTE 的唯一例外）；与 HZABHA-11 的 SPV=1/GVA=1 对比，证明 byte/halfword AMO 不属虚拟机访存指令（本组以字节宽度 HSV.B 对照，区别于 HZAMO-14 的 HSV.D） |

### 4.4 htinst：transformed atomic instruction 与伪指令消歧（funct3 宽度保留）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZABHA-15 | guest byte/halfword AMO 显式访问故障的 htinst | VS-mode 执行 `amoadd.b` 与 `amoadd.h` 触发 G-stage **显式**访问故障（cause=23），读 `htinst` 与 `vsepc` 处指令字比对 | `htinst`=0 或 transformed atomic instruction（`norm:H_trap_xtinst_val`）；非零时必须等于“原指令字保留全部字段、仅 bits19:15 替换为 Addr. Offset”（`htinst_transformed_atomic`，golden 由 `hyp_transform_mem_inst()` opcode 0x2F 分支计算，须保留 funct5/funct3/rd/rs2） |
| HZABHA-16 | **htinst 保留 funct3 宽度编码（Zabha 特有）** | 分别以 `amoadd.b`（funct3=000）与 `amoadd.h`（funct3=001）触发同类 G-stage 显式故障，比对两者 `htinst` 的 bits14:12 | transformed 值的 funct3 **逐位保留**陷入指令的宽度编码（.b→000、.h→001），使 HS-mode 能从 htinst 判定访问宽度；两者 htinst 仅 funct3 不同、其余字段按各自指令字保留（word/dword AMO 无此宽度区分需求，为 Zabha 独有验证维度） |
| HZABHA-17 | htinst 保留 aq/rl 位 | VS-mode 分别执行 `amoadd.b.aq`、`amoadd.h.rl`、`amoadd.b.aqrl`（raw encoding）触发 G-stage 故障，比对 `htinst` 的 bit26/bit25 | transformed 值的 aq(bit26)/rl(bit25) 与陷入指令逐位一致（`htinst_transformed_atomic`：保留除 bits19:15 外全部字段） |
| HZABHA-18 | byte/halfword AMO 的 htinst Addr. Offset 恒为 0 | 承接 HZABHA-15，检查 `htinst` bits19:15 | Addr. Offset=0——字节 AMO 1 字节对齐恒成立、永不拆分；半字 AMO 原子性要求单一内存操作、不跨页拆分，故 faulting VA 等于 original VA |
| HZABHA-19 | 隐式 VS-stage 遍历故障按 byte/halfword AMO 原始类型报 cause=23 | 构造 VS-stage 叶页表页在 G-stage 无效，VS-mode 执行 `amoadd.h`，使其翻译在**隐式读 VS-stage PTE** 时故障 | cause=**23**（store/AMO guest-page fault，按 AMO 原始访问类型报告，`norm:H_vm_gpapriv`）陷入 HS-mode；当 `htval` 非零（=该 PTE 的 GPA>>2）时 `htinst` **必须**为读伪指令（RV64=0x00003000）、**不允许为 0**（`norm:H_trap_xtinst_guestpage`） |
| HZABHA-20 | A/D 自动更新故障的写伪指令（ADUE=1） | `henvcfg.ADUE`=1，构造 VS-stage PTE 的 D=0（AMO 为写访问）且该 PTE 页在 G-stage 无写权限，VS-mode 执行 `amoadd.b` | `htinst`=写伪指令 0x00003020（`norm:H_trap_xtinst_guestpage_rw`：A/D 自动更新用 write 伪指令）；`htval`=该 PTE 的 GPA>>2；cause=23 |
| HZABHA-21 | 显式与隐式故障的 htinst/htval 消歧 | 对**同一 cause=23**，分别构造“字节 AMO 自身数据访问在 G-stage 失败”与“字节 AMO 的 VS-stage PTE 读在 G-stage 失败”两种场景，比对 `htinst` 与 `htval` | 前者 `htinst`=0 或 transformed atomic（funct3=000 保留）、`htval`=目标数据 GPA>>2；后者 `htinst`=伪指令（非零强制）、`htval`=PTE GPA>>2。htinst 为唯一消歧手段（与 HZAMO-20 同构，本组以字节宽度指令字复验） |

### 4.5 未对齐 byte/halfword AMO 与 MAG 放宽（Zabha 特有：仅半字可测未对齐）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZABHA-22 | **字节 AMO 任意地址均自然对齐（无未对齐场景，Zabha 特有）** | VS-stage/G-stage 均有效，VS-mode 对奇地址（如 base+1）执行 `amoadd.b`（trap-armed） | **无**对齐相关异常、正常执行且结果正确——字节 AMO 的 1 字节对齐恒成立（`norm:Zabha_rs1_align_addr`：按操作数大小自然对齐，字节=1 字节），绝不报 cause=6/7；与半字 AMO（HZABHA-23）形成关键对比 |
| HZABHA-23 | VS-mode 未对齐半字 AMO 无 MAG 覆盖的异常与委托 | VS-stage/G-stage 均有效，VS-mode 对奇地址（非 2 字节对齐）执行 `amoadd.h`（trap-armed），`hedeleg[6]`/`[7]` 分别置 0 与 1 | 触发 store/AMO address misaligned (cause=6) 或 store/AMO access fault (cause=7)（二者之一均合规，`norm:Zabha_rs1_align_addr` 跨引用 `norm:amo_alignment`；**非** load 类 4/5）；`hedeleg` 置 1 时递送 VS-mode、置 0 时陷入 HS-mode |
| HZABHA-24 | MAG 粒度内未对齐半字 AMO 无异常且原子执行 | 平台声明 MAG，VS-mode 对全部字节位于同一 MAG 粒度内的奇地址执行 `amoadd.h` | **无**对齐异常、正常执行且语义正确（`norm:misaligned_atomicity_granule_size`，经 `norm:Zabha_rs1_align_addr` 的“the same exception options”跨引用适用于半字 AMO）；平台未声明 MAG 时本用例 SKIP |
| HZABHA-25 | MAG 内未对齐半字 AMO 的 G-stage 故障 | 承接 HZABHA-24 场景，令目标 GPA 在 G-stage 无写权限 | cause=23 陷入 HS-mode，`htval`=GPA>>2 或 0，`htinst` Addr. Offset=0（单一内存操作，不拆分） |
| HZABHA-26 | 未对齐半字 AMO 陷入 HS-mode 时的现场 | `hedeleg[6]`=0，承接 HZABHA-23，检查 HS-mode trap 现场 | cause=6 或 7；`stval`=**未对齐地址本身**（等于 original VA）；GVA=1、SPV=1；`htval`=0（非 guest-page fault）；`htinst`=0 或 transformed atomic 且 Addr. Offset=0 |

### 4.6 henvcfg.FIOM 与 ADUE 交互

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZABHA-27 | FIOM=1 时 VS-mode 带 aq/rl 的 byte/halfword AMO | `henvcfg.FIOM`=1，VS-mode 对平台声明为 device-I/O 有序的区域执行 `amoadd.b.aq`/`amoadd.h.rl`（trap-armed） | 指令可执行且数据语义不变；排序被修改为同时约束 device I/O 与内存（`norm:henvcfg_fiom_order`）——排序效果需多 hart 观测，单 hart 仅验证可执行性与数据语义 |
| HZABHA-28 | FIOM=0 对照 | `henvcfg.FIOM`=0，同场景执行相同指令 | 排序不被修改（对照），指令可执行且数据语义与 HZABHA-27 一致 |
| HZABHA-29 | ADUE=0 时 byte/halfword AMO 到 A=0 VS-stage 页 | `henvcfg.ADUE`=0，VS-stage PTE A=0，VS-mode 执行 `amoadd.b`/`amoadd.h` | VS-stage 按 Svade 行为报 store page-fault (cause=15) 而非硬件更新 A 位（`norm:henvcfg_adue_op`）；递送目标按 `hedeleg[15]`；不得误判为 guest-page fault (cause 21/23) |
| HZABHA-30 | ADUE=0 时 byte/halfword AMO 到 A=1/D=0 页 | `henvcfg.ADUE`=0，VS-stage PTE A=1/D=0，VS-mode 执行 `amoadd.h` | store page-fault (cause=15)——AMO **恒执行写回**，D 位要求为**强制**（非 UNSPECIFIED，同 HZAMO-28）；`norm:henvcfg_adue_op`：ADUE=0 时按 Svade 报页错误 |
| HZABHA-31 | ADUE=1 时 byte/halfword AMO 硬件更新 A/D 位后正常完成 | `henvcfg.ADUE`=1（且平台实现 Svadu），VS-stage PTE A=0/D=0，VS-mode 执行 `amoadd.b` | 硬件自动置 A/D 位，AMO 正常完成、无页错误；回读 PTE A=1/D=1（`norm:henvcfg_adue_op`）；平台未实现 Svadu 时 SKIP |

### 4.7 amocas.b/h 交叉（Zabha × Zacas，条件用例）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZABHA-32 | VS/VU-mode 执行 amocas.b/h 不报 cause=22 且语义一致 | V=1，VS-mode 执行 `amocas.b`/`amocas.h`（成功路径与失败路径各一次，目标为可读写映射），比对 rd 的 8/16 位符号扩展与 rd 高位忽略 | 正常执行、语义正确（`norm:Zabha_amocas-BH_ignore_bits`：比较仅用 rd[7:0]/rd[15:0]、忽略高位）；**绝不**报 cause=22；未实现 Zacas 时本用例 SKIP |
| HZABHA-33 | amocas.b/h 异常归 store/AMO 类（无条件写权限检查） | `hedeleg[15]`=1，VS-stage 页 R=1/W=0，VS-mode 执行 `amocas.b`（成功路径与失败路径各一次） | 两条路径**均**报 store/AMO page fault (cause=15)（`norm:Zacas_amocas_w_permission`：“always requires write permissions” 不因比较失败而豁免）——与 HZACAS-06/07 同构，本组以字节宽度复验 |
| HZABHA-34 | amocas.h 的 G-stage 故障强制陷入 HS-mode | G-stage 目标 GPA 无写权限，VS-mode 执行 `amocas.h`（比较匹配，成功路径） | store/AMO guest-page fault (cause=23) 陷入 HS-mode；`hedeleg[23]` 只读零；GVA=1、SPV=1，`htval`=GPA>>2 或 0 |
| HZABHA-35 | amocas.b/h 的 htinst transformed 保留 funct5=00101 与 funct3=000/001 | VS-mode 执行 `amocas.b` 与 `amocas.h` 触发 G-stage 显式故障，比对 `htinst` 的 funct5（bits31:27）与 funct3（bits14:12） | transformed 值保留 **funct5=00101** 与 **funct3=000(.b)/001(.h)**、rd、rs2，仅 bits19:15 ← Addr. Offset；两者 htinst 仅 funct3 不同（与 HZABHA-16 同理，验证 CAS 宽度编码保留） |
| HZABHA-36 | 未对齐 amocas.h 的异常与 MAG 放宽（仅半字可测） | VS-mode 对奇地址执行 `amocas.h`（trap-armed）；另在平台声明 MAG 时对同一 MAG 粒度内奇地址执行 | 无 MAG 时报 store/AMO address misaligned (cause=6) 或 access fault (cause=7)；MAG 粒度内无异常且原子执行（`norm:Zabha_rs1_align_addr` 跨引用，`amocas.b` 因字节对齐恒成立无未对齐场景）；未声明 MAG 时 MAG 分支 SKIP |

### 4.8 保留字节/半字 lr/sc 编码的异常类型（Zabha 特有）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZABHA-37 | VS/VU-mode 保留字节/半字 lr/sc 编码报 illegal-instruction (cause=2) 而非 cause=22 | V=1，VS-mode 与 VU-mode 分别以 raw encoding 注入 funct3=000/001 的 `lr`（funct5=00010）与 `sc`（funct5=00011）保留编码并执行（trap-armed） | illegal-instruction exception (cause=2)——Zabha 未定义字节/半字 lr/sc，保留编码在所有模式均非法（非 HS-qualified），故 V=1 时按 `norm:H_cause_virtual_instruction` **仍报 cause=2 而非 virtual-instruction (cause=22)**；若报 cause=22 即违反 SPEC，保持失败并记录至 `bugs/` |

### 4.9 架构边界：H 扩展无字节/半字原子虚拟机等价指令

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZABHA-38 | （记录型）无字节/半字原子虚拟机等价指令，但 HLV.B/HLV.H/HSV.B/HSV.H 存在（非原子） | 以 trap-armed 探测 HS-mode 下是否存在按 `norm:hlsv_priv` 语义执行字节/半字原子 RMW 的指令编码；对照 HLV.B/HLV.BU/HLV.H/HLV.HU/HSV.B/HSV.H 的可用性 | 记录性：`norm:hlsv_op` 仅映射 RV32I/RV64I 的 LB..LD / SB..SD（含字节/半字），**无字节/半字原子（AMO）虚拟机等价指令**；HS-mode 可对 guest 字节/半字做非原子 HLV/HSV，但无法做原子 RMW。探测到的任何“虚拟机原子指令”编码须与 SPEC 比对后报告 |
| HZABHA-39 | （记录型）HLV.B+HSV.B / HLV.H+HSV.H 无法替代字节/半字 AMO 原子性 | HS-mode 以 `HLV.B`+`HSV.B`（或 `HLV.H`+`HSV.H`）序列对 guest 内存做字节/半字读改写，与 VS-mode 内的 `amoadd.b`/`amoadd.h` 并发（需多 hart） | 记录性：HLV+HSV 为两次独立非原子访问、无原子性保证，并发下可观测到丢失更新，而 VS-mode 字节/半字 AMO 保持原子；多 hart 未就绪时 SKIP（与 HZLRSC-40/HZAMO-31/HZACAS-35 同理） |

> [!NOTE]
> - 本组所有测试必须以平台配置 `H_SUPPORTED` 宏为准检测 H 扩展；Zabha 支持以平台配置 `ZABHA_SUPPORTED` 宏为准，并以 trap-armed 执行字节/半字 AMO 二次探测，不满足时全组 TEST_SKIP；Zabha 依赖 Zaamo（进而依赖 A 扩展），故本组以 `ZAAMO_SUPPORTED`/`A_SUPPORTED` 亦满足为隐式前提；4.7 的 `amocas.b/h` 另依赖 Zacas（`ZACAS_SUPPORTED`），未实现时相应用例（HZABHA-32~36）TEST_SKIP。
> - **异常类别断言必须精确**：字节/半字 AMO 与 `amocas.b/h` 的**全部**异常归 store/AMO 类（misaligned=6、access fault=7、page fault=15、guest-page fault=23），依据 `norm:mcause_exccode_st_sc_amo`（byte/halfword AMO 属 AMO 指令族，与宽度无关）与 `norm:store_page_fault_no_w` 配套 NOTE。断言**绝不**得接受 load 类（4/5/13/21）；隐式 VS-stage 遍历 GPF 报 cause=23（`norm:H_vm_gpapriv`）。
> - **cause=22 为强制负向断言（有效指令）**：HZABHA-02/03/32 中 guest 字节/半字 AMO 与 amocas.b/h 若报 virtual-instruction exception 即违反 SPEC，应保持失败并记录至 `bugs/` 目录，不得放宽断言。
> - **htinst funct3 宽度保留为 Zabha 特有强制维度**：HZABHA-16/35 须实测确认框架 `hyp_transform_mem_inst()` 的 opcode 0x2F 分支正确保留 funct3=000(.b)/001(.h)（bits14:12），使 HS-mode 能判定陷入访问宽度；不得假定与 word/dword AMO 共享分支即宽度编码自动正确，须以字节/半字实际指令字实测 golden 值。
> - **htinst 伪指令的强制性**：HZABHA-19 中当 `htval` 非零且故障源于 VS-stage 隐式访问时，`htinst` 写 0 属违反 `norm:H_trap_xtinst_guestpage`（“zero is not allowed”），应保持失败；HZABHA-15 的显式访问场景下 `htinst` 写 0 是允许的。
> - **字节 AMO 无未对齐场景（Zabha 区别于 Zaamo 的关键）**：HZABHA-22 验证字节 AMO 在任意地址（含奇地址）均自然对齐、正常完成，**绝不**报 cause=6/7；未对齐异常与 MAG 放宽分支（HZABHA-23~26、HZABHA-36）**仅对半字 AMO（奇地址）**可测。HZABHA-24/25/36 的 MAG 分支依赖平台声明 misaligned atomicity granule，未声明时 SKIP；未对齐异常允许 address-misaligned 与 access-fault 两类合规结果，用例验证 cause 属预期集合并记录实际值，不得因平台选择其中一类而判 FAIL。
> - **AMO 必写 ⇒ D 位强制**：HZABHA-30 中字节/半字 AMO 恒执行写回，故 ADUE=0 时对 D=0 页报 store page-fault 为**强制**结果（非记录型，同 HZAMO-28）。
> - **保留 lr/sc 编码报 cause=2 而非 cause=22**：HZABHA-37 依据 `norm:H_cause_virtual_instruction`——virtual-instruction 仅替代 HS-qualified 但 V=1 受阻的指令，保留字节/半字 lr/sc 在所有模式均非法（非 HS-qualified），故报 illegal-instruction (cause=2)。若平台报 cause=22 即违反 SPEC，保持失败。
> - **记录型用例不得强制判定**：HZABHA-38/39（无字节/半字原子虚拟机等价指令的架构边界）为架构许可/实现自定义行为，仅记录实现选择。
> - **多 hart 限制**：HZABHA-27/28 的 FIOM 排序效果断言与 HZABHA-39 需 secondary hart 支持；当前公共框架仅 hart 0 运行，相关断言在框架就绪前 SKIP（非 FAIL），单 hart 平台同样 SKIP 并注明。
> - **权限用例依赖**：4.2/4.3/4.6/4.7 需 VS-stage（`vsatp`）与 G-stage（`hgatp`）页表构造能力（R=0、R=1/W=0、A=0、D=0、G-stage 无写权限等映射组合）；权限检查机制本身由 `pmp_test_plan.md`/`vm_test_plan.md` 覆盖，本组仅验证字节/半字 AMO 的权限语义结论。
> - Zabha 非虚拟化语义（字节/半字 AMO 编码、18 条指令的数据语义、rd 符号扩展、rs2 高位忽略、`amocas.b/h` 比较高位忽略、保留 lr/sc 编码、aq/rl 数据语义、M/S/U 各特权级基础可执行性）由 `Zabha_test_plan.md` 覆盖；`amocas.w/d/q` 的完整指令级语义由 `Zacas_test_plan.md` 覆盖，本组不重复。

---

## Group 5. Hypervisor × Zalasr 交叉测试

**与 Hypervisor 的交集点**：
1. **异常归类二分（load-acquire → load 类，store-release → store/AMO 类）**：Zalasr 提供**独立的原子有序纯加载与纯存储**（`norm:zalasr_atomic_ordered`），load-acquire（funct5=00110）仅读内存写 rd（`norm:ldaq_atomic_load_op`）、store-release（funct5=00111）仅写内存（`norm:sdrl_atomic_store_op`）。按功能语义，load-acquire 归 load 类（cause 4/5/13/21，同 Group 1 的 LR）、store-release 归 store/AMO 类（cause 6/7/15/23）。这与 Group 1 Zalrsc 的 LR/SC 二分**同构但机制不同**（Zalrsc 是成对读改写拆成的两半，Zalasr 是两条独立指令），区别于 Group 2/3/4（全为读改写 AMO，异常统一归 store/AMO 类）。**SPEC 歧义标注**：`machine.adoc` 的 `norm:mcause_exccode_ld_ldrsv`/`norm:mcause_exccode_st_sc_amo` 分类文本早于 Zalasr，未明文点名 load-acquire/store-release；store-release 无论视为 store 还是 AMO 均归 store/AMO 类（**无歧义，可强制断言**），而 load-acquire 归 load 类还是 store/AMO 类存在歧义（若按 AMO 处理则需写权限、报 cause 15），故 load-acquire 的具体 cause 类按**观测记录型**处理——记录实际 cause 并与功能分类（load 类）比对，偏差报告至 `bugs/` 供 SPEC 澄清，不做武断强制判定
2. **load-acquire 仅需读权限（R=1/W=0 页必正常执行）**：load-acquire 为纯加载，到 R=1/W=0 VS-stage 页**必须正常执行**（对比 store-release/AMO 需写权限报 cause 15）——这是 load-acquire 归 load 语义的架构必然（独立原子加载必须能读只读内存，否则 `norm:zalasr_atomic_ordered` 的“原子加载”失去意义）；与 Group 1 的 HZLRSC-25（LR 仅需读权限）、Group 2 的 HZAMO-09（LR vs AMO 权限对比）同构
3. **store-release 需写权限且恒写（D 位强制）**：store-release 为纯存储、**恒写入、无“失败”分支**（区别于 SC 可失败、CAS 可比较失败），故需写权限（W=0 页报 store page-fault cause 15 / G-stage cause 23），且 `henvcfg.ADUE`=0 时对 D=0 页报 store page-fault 为**强制**（同 Group 2 的 HZAMO-28，非 UNSPECIFIED）
4. **委托路径二分**：VS-stage 故障（load-acquire cause 4/5/13、store-release cause 6/7/15）的 `hedeleg` 位为 Writable，可下放 VS-mode；G-stage 故障（load-acquire cause 21、store-release cause 23）的 `hedeleg` 位为 Read-only 0，**架构强制**陷入 HS-mode（`norm:hedeleg_acc`、`norm:H_vm_gpatrans`）——委托路径的 load/store 二分镜像异常类别二分
5. **隐式 VS-stage 遍历故障按原始类型报告（load-acquire → cause 21，store-release → cause 23）**：`norm:H_vm_gpapriv` — 隐式 PTE 访问按 implicit load/store 检查 G-stage 权限，但异常“always reported for the original access type”，故 load-acquire 的隐式遍历 GPF 报 load guest-page fault (cause 21)、store-release 报 store/AMO guest-page fault (cause 23)。这一隐式遍历 cause 的二分与 Group 1（LR→21、SC→23）一致，区别于 Group 2/3/4（全 AMO→23）
6. **htinst transformed atomic instruction（统一 opcode 0x2F 格式，非 load/store 格式）**：load-acquire/store-release 虽为纯 load/store，但因位于 AMO opcode (0x2F) 空间且**不在** `transformedloadinst`（LB..LD，opcode 0x03）/`transformedstoreinst`（SB..SD，opcode 0x23）的助记符列表中，其 `htinst` transformed 值走 `transformedatomicinst` 格式（保留除 bits19:15 外全部字段，含 funct5=00110/00111、aq/rl、funct3、rd/rs2）——这是**反直觉但可测**的点：load-acquire 是“load”却用 atomic transformed 格式。框架 `hyp_transform_mem_inst()` 的 opcode 0x2F 分支已正确处理，HS-mode 可由 funct5 区分 load-acquire(00110)/store-release(00111)
7. **Addr. Offset 恒为 0**：Zalasr 原子性要求单一内存操作——MAG 放宽时不跨页拆分、未放宽时直接异常，故 htinst 的 Addr. Offset 恒为 0（同 Group 2/3/4）
8. **GVA/SPV 组合**：guest load-acquire/store-release trap → GVA=1 且 SPV=1（VS 来源 SPVP=1、VU 来源 SPVP=0）；对比 HLV/HLVX/HSV 显式访问故障 → GVA=1 但 SPV=0（`norm:hstatus_gva_op`/`norm:hstatus_spv_op`）
9. **henvcfg.FIOM × aq/rl（Zalasr 指令恒带 aq/rl）**：`norm:henvcfg_fiom_order` 明文“an atomic instruction that accesses a region ordered as device I/O has its aq and/or rl bit set”。load-acquire 恒 aq=1（`norm:ldaq_aq_required`）、store-release 恒 rl=1（`norm:sdrl_rl_required`），故 V=1 且 FIOM=1 时 Zalasr 指令访问 device-I/O 有序区域时排序**恒被修改**为同时约束 I/O 与内存——比 Group 1/2/3/4（aq/rl 可选）更普适，无需构造特定 aq/rl 变体
10. **henvcfg.ADUE × A/D 更新（load-acquire 只涉 A，store-release 涉 A+D）**：load-acquire 纯读，仅触发 A 位更新、**永不置数据页 D 位**；ADUE=0 时到 A=0 页报 load page-fault (cause 13，Svade 路径）。store-release 纯写，触发 A+D 位更新；ADUE=0 时到 A=0 或 D=0 页报 store page-fault (cause 15)。隐式遍历 A/D 更新故障：load-acquire → 写伪指令 0x00003020 + cause 21；store-release → 写伪指令 0x00003020 + cause 23（`norm:H_trap_xtinst_guestpage_rw`）
11. **cause=22 排除**：`hypervisor.adoc` 全部 virtual-instruction 条款无一涵盖 load-acquire/store-release → guest Zalasr 绝不报 cause=22（与 LR/SC/AMO/CAS 同）
12. **保留编码在 V=1 报 cause=2 而非 cause=22（同 Zabha）**：无 aq 的 load-acquire（含 load-release，`norm:ldaq_no_aq_reserved`）与无 rl 的 store-release（含 store-acquire，`norm:sdrl_no_rl_reserved`）为 RESERVED；`norm:H_cause_virtual_instruction` 规定 virtual-instruction (cause=22) 仅替代 HS-qualified 但 V=1 受阻的指令，而保留编码在所有模式均非法（非 HS-qualified），故 V=1 时执行报 illegal-instruction (cause=2) 而非 cause=22——与 Group 4 的 HZABHA-37 同构
13. **无原子有序 load/store 虚拟机等价指令（架构边界，比 AMO 更微妙）**：`norm:hlsv_op` 仅映射 RV32I/RV64I 基础 load/store（opcode 0x03/0x23），load-acquire/store-release（opcode 0x2F）无 HL*/HS* 等价指令。但**区别于 AMO/CAS**：load-acquire/store-release 本身即纯 load/store，故 HLV.W 可复制 load-acquire 的**数据传输**、HSV.W 可复制 store-release 的数据传输——但丢失 (a) 单拷贝原子性保证与 (b) acquire/release RCsc 排序注解（HLV/HSV 无 aq/rl 变体）。这一“数据可复制、原子性与排序不可复制”的边界比 Group 2/3（AMO 的读改写连数据都无法用 HLV+HSV 原子复制）更微妙

**规范依据**：
- `norm:zalasr_atomic_ordered` / `norm:ldaq_atomic_load_op` / `norm:sdrl_atomic_store_op`：Zalasr 为独立原子加载/存储，load-acquire 纯读、store-release 纯写，语义在 VS/VU-mode 下不变（非虚拟化语义由 `Zalasr_test_plan.md` 覆盖，本组仅验证虚拟化不改变语义）
- `norm:mcause_exccode_ld_ldrsv` / `norm:mcause_exccode_st_sc_amo` / `norm:load_page_fault_no_r` / `norm:store_page_fault_no_w`：load-acquire → load 类（仅需读权限）、store-release → store/AMO 类（需写权限）的功能分类依据（load-acquire 的具体 cause 类因 zalasr.adoc 未明文而按观测记录型处理）
- `norm:ldaq_signext_rule` / `norm:zalasr_signext_rd` / `norm:zalasr_ignore_rs2_upper`：rd 符号扩展与 rs2 高位忽略在虚拟化下不变
- `norm:hedeleg_acc` / `norm:hedeleg_op` / `norm:H_vm_gpatrans`：委托路径二分与 G-stage 故障必报 guest-page-fault
- `norm:H_vm_gpapriv`：隐式 VS-stage 遍历故障按原始类型（load-acquire→cause 21、store-release→cause 23）报告
- `norm:H_trap_xtinst_val` / `htinst_transformed_atomic` / `norm:H_trap_xtinst_guestpage` / `norm:H_trap_xtinst_guestpage_rw`：htinst 的 transformed（统一 opcode 0x2F 格式）与伪指令两类取值规则
- `norm:hstatus_gva_op` / `norm:hstatus_spv_op` / `norm:htval_trapval`：trap 现场 GVA/SPV/htval 组合
- `norm:zalasr_natural_align` / `norm:zalasr_misaligned_exception` / `norm:zalasr_misaligned_pma_relax` / `norm:zalasr_misaligned_single_op`：未对齐异常路径与 MAG 放宽分支
- `norm:ldaq_aq_required` / `norm:sdrl_rl_required` / `norm:henvcfg_fiom_order` / `norm:henvcfg_adue_op`：Zalasr 恒带 aq/rl 使 FIOM 排序修改恒适用、ADUE 与 A/D 更新路径
- `norm:ldaq_no_aq_reserved` / `norm:sdrl_no_rl_reserved` / `norm:H_cause_virtual_instruction`：保留编码在 V=1 报 illegal-instruction (cause=2) 而非 virtual-instruction (cause=22)
- `norm:hlsv_op` / `norm:hlsv_priv` / `norm:hlsv_virtinst`：无原子有序 load/store 虚拟机等价指令的架构边界与 cause=22 排除对照
- `norm:zalasr_builds_on_amo`：Zalasr 可独立实现，门控不以 A 扩展宏为前置

**测试职责**：验证 load-acquire/store-release 在 V=1 / 两阶段翻译 / HS-mode 视角下的交叉行为：异常归类二分（load-acquire→load 类、store-release→store/AMO 类）、load-acquire 仅需读权限（R=1/W=0 页正常执行）与 store-release 需写权限且恒写（D 位强制）、委托路径二分、隐式遍历故障按原始类型报告（cause 21/23）、htinst 的统一 opcode 0x2F transformed atomic 格式（保留 funct5=00110/00111 区分 load/store）、GVA/SPV/htval trap 现场、未对齐 load-acquire/store-release 的 MAG 放宽分支、FIOM 对恒带 aq/rl 指令的排序修改、ADUE 与 A/D 更新路径（load-acquire 只涉 A、store-release 涉 A+D）、保留编码报 cause=2 而非 cause=22，以及无原子有序 load/store 虚拟机等价指令（但 HLV/HSV 可复制数据传输）的架构边界。load-acquire/store-release 若工具链（`-march` 含 `zalasr`）支持助记符则直接使用；否则以 raw encoding 注入（AMO opcode=0x2F，load-acquire funct5(bits31:27)=00110 且 rs2 字段(bits24:20)置 0、aq(bit26)=1，store-release funct5=00111 且 rd 字段(bits11:7)置 0、rl(bit25)=1，funct3=000(.b)/001(.h)/010(.w)/011(.d，RV64)）；保留编码以 raw encoding 注入（load funct5=00110+aq=0、store funct5=00111+rl=0）；统一 `.option norvc` 保证 4 字节指令长度。`common/mem_ops.h` 若无 load-acquire/store-release 原语须补充或以 raw encoding 注入（参照 `Zalasr_test_plan.md` 测试实现说明 1）。**因 load-acquire 的 cause 归类存在 SPEC 歧义（zalasr.adoc 未明文、编码位于 AMO opcode 空间），本组对 load-acquire 的 cause 类采用“记录实际观测值 + 与功能分类比对 + 偏差报告 bugs/”策略，store-release 因 store/AMO 同归一类可强制断言**。本组 5.1 的 HS/VS/VU-mode 可执行性用例（HZLASR-01~03）为 Hypervisor 交叉视角的**唯一权威来源**——原 `Zalasr_test_plan.md` Group 7 的 HS/VS/VU-mode 执行用例（旧 ZALASR-52~54）已整体迁移至本组，`Zalasr_test_plan.md` Group 7 现仅保留 M/S/U 非虚拟化特权级用例（ZALASR-49~51）与扩展独立性/门控用例（ZALASR-55/56）。本组聚焦 cause=22 排除、异常归类二分与 trap 现场。

### 5.1 HS/VS/VU-mode 正常执行与 cause=22 排除

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLASR-01 | HS-mode 执行 load-acquire/store-release 正常 | HS-mode 对主存执行 `lb.aq`/`lh.aq`/`lw.aq`/`ld.aq`（RV64）与 `sb.rl`/`sh.rl`/`sw.rl`/`sd.rl`（RV64，各宽度抽样） | 全部正常执行、load-acquire 的 rd=符号扩展后的加载值、store-release 的内存=rs2 低对应宽度位，无异常 |
| HZLASR-02 | VS-mode 执行 load-acquire/store-release 不报 cause=22 | V=1，VS-mode 执行 load-acquire/store-release 集合（目标为 VS-stage/G-stage 均有效的可读写映射） | 正常执行、语义正确；**绝不**触发 virtual-instruction exception (cause=22)（`hypervisor.adoc` 无任何条款对 load-acquire/store-release 施加 virtual-instruction 门控） |
| HZLASR-03 | VU-mode 执行 load-acquire/store-release 不报 cause=22 | 同配置，VU-mode 执行 load-acquire/store-release 集合（目标页 U=1） | 正常执行，无异常，绝不报 cause=22 |
| HZLASR-04 | VS-mode load-acquire/store-release 语义与非虚拟化一致 | 同一 load-acquire/store-release 序列分别在 HS-mode 与 VS-mode 执行，比对 rd 符号扩展（`norm:ldaq_signext_rule`）、rs2 高位忽略（`norm:zalasr_ignore_rs2_upper`）、load 不改内存/store 写入正确 | 架构可见语义完全一致（虚拟化不改变 Zalasr 指令语义，`norm:zalasr_atomic_ordered`） |

### 5.2 异常归类二分与权限要求（load-acquire→load 类仅需读；store-release→store/AMO 类需写）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLASR-05 | Zalasr 相关 hedeleg 位属性探测 | 逐位写 `hedeleg` bit 4/5/13（load 侧）与 bit 6/7/15（store 侧）及 bit 21/23（G-stage）并读回 | bit 4/5/6/7/13/15 可写（Writable）；bit 21/23 只读零（Read-only 0，`norm:hedeleg_acc`）——确立“VS-stage 可委托、G-stage 强制 HS-mode”二分前提（load-acquire 用 load 侧位、store-release 用 store 侧位） |
| HZLASR-06 | load-acquire 到 R=1/W=0 VS-stage 页正常执行（仅需读权限，强制正向断言） | `hedeleg[13]`=1，VS-stage 映射目标页 R=1/W=0，VS-mode 执行 `lw.aq`/`ld.aq`（trap-armed） | **正常执行、无异常**（load-acquire 为纯加载，仅需读权限，`norm:ldaq_atomic_load_op`）——与 store-release（HZLASR-08）形成关键对比；若平台报 store page-fault (cause 15)（按 AMO 语义要求写权限），记录实际 cause 并作为疑似缺陷报告至 `bugs/`（独立原子加载必须能读只读内存） |
| HZLASR-07 | load-acquire 到 R=0 VS-stage 页的故障归类（观测记录型） | `hedeleg[13]`=1，VS-stage 页 R=0/W=0，VS-mode 执行 `lw.aq`（trap-armed） | **期望** load page-fault (cause 13) 递送至 VS-mode（load-acquire 纯读，功能上归 load 类）；**记录实际观测 cause**——若为 cause 13 则与功能分类一致；若为 cause 15（store 类）则记录并报告（zalasr.adoc 未明文规定归类，位于 AMO opcode 空间，按观测记录型处理，不武断强制判定） |
| HZLASR-08 | store-release 到 R=1/W=0 VS-stage 页报 store page-fault | `hedeleg[15]`=1，VS-stage 映射目标页 R=1/W=0，VS-mode 执行 `sw.rl`/`sd.rl` | store/AMO page fault (cause=15) 递送至 VS-mode，`vsepc`=store-release PC，`vstval`=故障 GVA（`norm:store_page_fault_no_w`：store-release 需写权限）——**强制断言**（store 与 AMO 同归 store/AMO 类，无歧义） |
| HZLASR-09 | store-release 到不可读页报 store 类而非 load 类 | VS-stage 页 R=0/W=0，VS-mode 执行 `sw.rl` | store/AMO page fault (cause=15)，**绝不**报 load page fault (cause=13)（`norm:store_page_fault_no_w` 配套 NOTE：不可读页必不可写） |
| HZLASR-10 | load-acquire 与 store-release 权限对比（R=1/W=0） | 同一 R=1/W=0 VS-stage 页，VS-mode 先执行 `lw.aq`（对照 HZLASR-06）再执行 `sw.rl` | `lw.aq` **正常执行**（仅需读权限），`sw.rl` 报 store page-fault (cause=15)（需写权限）——证明 load-acquire 与 store-release 的权限要求二分（同 Group 2 HZAMO-09 的 LR vs AMO 对比逻辑） |
| HZLASR-11 | hedeleg=0 时 load-acquire/store-release VS-stage 故障陷入 HS-mode | `hedeleg[13]`=0、`hedeleg[15]`=0，VS-mode 分别触发 load-acquire 的 VS-stage 读权限故障（R=0）与 store-release 的写权限故障（R=1/W=0） | 均递送至 HS-mode（`norm:hedeleg_op`）；load-acquire cause 记录实际观测（期望 13）、store-release cause=15（委托与否不改变 cause，仅改变递送目标） |

### 5.3 G-stage 故障强制 HS-mode 与 trap 现场（GVA/SPV/htval）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLASR-12 | load-acquire 的 G-stage 故障强制陷入 HS-mode（观测记录型） | G-stage 中目标 GPA 映射无效（R=0），VS-mode 执行 `lw.aq`；探测 `hedeleg[21]` | **期望** load guest-page fault (cause=21) 陷入 HS-mode（load-acquire 纯读，功能上归 load 类）；`hedeleg[21]` 只读零，无法委托至 VS-mode（`norm:H_vm_gpatrans`/`norm:hedeleg_acc`）；**记录实际观测 cause**（若为 cause 23 则报告，同 HZLASR-07 的歧义处理） |
| HZLASR-13 | store-release 的 G-stage 故障强制陷入 HS-mode | G-stage 中目标 GPA 无写权限，VS-mode 执行 `sd.rl`；探测 `hedeleg[23]` | store/AMO guest-page fault (cause=23) 陷入 HS-mode；`hedeleg[23]` 只读零（`norm:H_vm_gpatrans`/`norm:hedeleg_acc`）——**强制断言**（store-release 归 store/AMO 类无歧义） |
| HZLASR-14 | guest load-acquire/store-release trap 的 GVA=1 且 SPV=1 | 承接 HZLASR-12/13，检查 HS-mode trap 现场 `hstatus`；另以 VU-mode 触发同故障 | VS 来源：GVA=1、SPV=1、SPVP=1；VU 来源：GVA=1、SPV=1、SPVP=0（`norm:hstatus_gva_op`/`norm:hstatus_spv_op`），`stval`=故障 GVA |
| HZLASR-15 | guest Zalasr G-stage 故障时 htval=GPA>>2 或 0 | 承接 HZLASR-12/13，检查 `htval` | `htval`=故障 GPA>>2 或 0（基线 H `norm:htval_trapval` 允许写零；非零时必须为 GPA>>2）。注：Shtvala 实现下收紧为必须非零，由 `Shtvala_test_plan.md` 覆盖（本组不重复该强化断言） |
| HZLASR-16 | VS-stage 故障时 htval=0 | 承接 HZLASR-11（cause 13/15 陷入 HS-mode），检查 `htval` | `htval`=0（`norm:htval_trapval`：仅 guest-page fault 写 GPA，其余 trap 写零）——与 HZLASR-15 形成对比 |
| HZLASR-17 | 对比 HLV/HSV 的 SPV/GVA 组合 | HS-mode（V=0）设 `hstatus.SPVP`=1 执行 `HLV.W`（对照 load-acquire）与 `HSV.W`（对照 store-release）触发 guest-page fault，检查 `hstatus` | SPV=0 但 GVA=1（`norm:hstatus_gva_op` NOTE 的唯一例外）；与 HZLASR-14 的 SPV=1/GVA=1 对比，证明 load-acquire/store-release 不属虚拟机访存指令 |

### 5.4 htinst：transformed atomic instruction（统一 opcode 0x2F 格式）与伪指令消歧

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLASR-18 | guest load-acquire 显式访问故障的 htinst | VS-mode 执行 `lw.aq` 触发 G-stage **显式**访问故障，读 `htinst` 与 `vsepc` 处指令字比对 | `htinst`=0 或 transformed atomic instruction（`norm:H_trap_xtinst_val`）；非零时必须等于“原指令字保留全部字段、仅 bits19:15 替换为 Addr. Offset”（`htinst_transformed_atomic`，走 opcode 0x2F 分支，golden 由 `hyp_transform_mem_inst()` 计算，须保留 **funct5=00110**、aq=1、funct3、rd、rs2=0）——load-acquire 虽为 load 却用 atomic transformed 格式 |
| HZLASR-19 | guest store-release 显式访问故障的 htinst | VS-mode 执行 `sd.rl` 触发 G-stage **显式**访问故障（cause=23），读 `htinst` 与指令字比对 | `htinst`=0 或 transformed atomic instruction；非零时保留 **funct5=00111**、rl=1、funct3、rd=0、rs2，仅 bits19:15 ← Addr. Offset（走 opcode 0x2F 的 `transformedatomicinst` 而非 transformedstoreinst） |
| HZLASR-20 | htinst 保留 funct5 load/store 区分与 aq/rl 位 | VS-mode 分别执行 `lw.aq`（funct5=00110,aq=1,rl=0）、`lw.aqrl`（aq=1,rl=1）、`sw.rl`（funct5=00111,rl=1,aq=0）、`sw.aqrl`（rl=1,aq=1）触发 G-stage 故障，比对 `htinst` 的 funct5(bits31:27)/bit26/bit25 | transformed 值的 funct5 逐位保留（load=00110、store=00111，使 HS-mode 区分 load-acquire/store-release），aq(bit26)/rl(bit25) 与陷入指令逐位一致（`htinst_transformed_atomic`：保留除 bits19:15 外全部字段） |
| HZLASR-21 | load-acquire/store-release 的 htinst Addr. Offset 恒为 0 | 承接 HZLASR-18/19，检查 `htinst` bits19:15 | Addr. Offset=0——Zalasr 原子性要求单一内存操作、不跨页拆分（MAG 放宽时为单内存操作、未放宽时直接异常），故 faulting VA 等于 original VA |
| HZLASR-22 | 隐式 VS-stage 遍历故障按原始类型报告（load-acquire→21，store-release→23） | 构造 VS-stage 叶页表页在 G-stage 无效，VS-mode 分别执行 `lw.aq` 与 `sw.rl`，使其翻译在**隐式读 VS-stage PTE** 时故障 | load-acquire → **期望** cause=21（观测记录型，同 HZLASR-12）、store-release → cause=**23**（强制，`norm:H_vm_gpapriv`：按原始访问类型报告）；两者当 `htval` 非零（=该 PTE 的 GPA>>2）时 `htinst` **必须**为读伪指令（RV64=0x00003000）、**不允许为 0**（`norm:H_trap_xtinst_guestpage`） |
| HZLASR-23 | A/D 自动更新故障的写伪指令（ADUE=1） | `henvcfg.ADUE`=1，(a) 构造 VS-stage PTE A=0 且该 PTE 页在 G-stage 无写权限，VS-mode 执行 `lw.aq`（仅触发 A 位更新）；(b) 构造 D=0，执行 `sd.rl`（触发 A+D 位更新） | (a) load-acquire：`htinst`=写伪指令 0x00003020、cause=**21**（观测记录型）；(b) store-release：`htinst`=写伪指令 0x00003020、cause=**23**（强制）；两者 `htval`=该 PTE 的 GPA>>2（`norm:H_trap_xtinst_guestpage_rw`）——load-acquire 仅更新 A 位、store-release 更新 A+D 位 |
| HZLASR-24 | 显式与隐式故障的 htinst/htval 消歧 | 对 store-release 的**同一 cause=23**，分别构造“store-release 自身数据访问在 G-stage 失败”与“store-release 的 VS-stage PTE 读在 G-stage 失败”两种场景，比对 `htinst` 与 `htval` | 前者 `htinst`=0 或 transformed atomic（funct5=00111 保留）、`htval`=目标数据 GPA>>2；后者 `htinst`=伪指令（非零强制）、`htval`=PTE GPA>>2。htinst 为唯一消歧手段（与 HZAMO-20 同构，本组以 store-release 指令字复验） |

### 5.5 未对齐 load-acquire/store-release 与 MAG 放宽

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLASR-25 | VS-mode 未对齐 load-acquire 无 MAG 覆盖的异常与委托（观测记录型） | VS-stage/G-stage 均有效，VS-mode 对 2 字节对齐（非 4 字节）地址执行 `lw.aq`（trap-armed），`hedeleg[4]`/`[5]` 分别置 0 与 1 | **期望** load address misaligned (cause=4) 或 load access fault (cause=5)（二者之一均合规，`norm:zalasr_misaligned_exception`；load-acquire 功能上归 load 类）；**记录实际观测 cause**（若为 store 类 6/7 则报告，同 HZLASR-07 歧义处理）；`hedeleg` 置 1 时递送 VS-mode、置 0 时陷入 HS-mode |
| HZLASR-26 | VS-mode 未对齐 store-release 无 MAG 覆盖的异常与委托 | VS-stage/G-stage 均有效，VS-mode 对 2 字节对齐（非 4 字节）地址执行 `sw.rl`（trap-armed），`hedeleg[6]`/`[7]` 分别置 0 与 1 | store/AMO address misaligned (cause=6) 或 store/AMO access fault (cause=7)（二者之一均合规，`norm:zalasr_misaligned_exception`；**非** load 类 4/5）——**强制断言**（store-release 归 store/AMO 类无歧义）；`hedeleg` 置 1 时递送 VS-mode、置 0 时陷入 HS-mode |
| HZLASR-27 | MAG 粒度内未对齐 load-acquire/store-release 无异常且原子执行 | 平台声明 MAG，VS-mode 对全部字节位于同一 MAG 粒度内的未对齐地址执行 `lw.aq` 与 `sw.rl` | **无**对齐异常、正常执行且语义正确（`norm:zalasr_misaligned_single_op`，同 Zaamo/Zacas 的 MAG 放宽，区别于 Zalrsc 无 MAG）；平台未声明 MAG 时本用例 SKIP |
| HZLASR-28 | 未对齐 load-acquire/store-release 陷入 HS-mode 时的现场 | `hedeleg[4]`/`[6]`=0，承接 HZLASR-25/26，检查 HS-mode trap 现场 | load-acquire cause=4/5（观测记录）、store-release cause=6/7（强制）；`stval`=**未对齐地址本身**（等于 original VA）；GVA=1、SPV=1；`htval`=0（非 guest-page fault）；`htinst`=0 或 transformed atomic 且 Addr. Offset=0（单一内存操作，不拆分） |

### 5.6 henvcfg.FIOM 与 ADUE 交互（Zalasr 指令恒带 aq/rl）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLASR-29 | FIOM=1 时 VS-mode load-acquire/store-release（恒带 aq/rl） | `henvcfg.FIOM`=1，VS-mode 对平台声明为 device-I/O 有序的区域执行 `lw.aq`（恒 aq=1）/`sw.rl`（恒 rl=1）（trap-armed） | 指令可执行且数据语义不变；排序被修改为同时约束 device I/O 与内存（`norm:henvcfg_fiom_order`）——Zalasr 恒带 aq/rl，故无需构造特定变体即恒适用；排序效果需多 hart 观测，单 hart 仅验证可执行性与数据语义 |
| HZLASR-30 | FIOM=0 对照 | `henvcfg.FIOM`=0，同场景执行相同指令 | 排序不被修改（对照），指令可执行且数据语义与 HZLASR-29 一致 |
| HZLASR-31 | ADUE=0 时 load-acquire 到 A=0 VS-stage 页（仅 A 位，观测记录型） | `henvcfg.ADUE`=0，VS-stage PTE A=0（R=1），VS-mode 执行 `lw.aq` | **期望** VS-stage 按 Svade 行为报 load page-fault (cause=13) 而非硬件更新 A 位（`norm:henvcfg_adue_op`）；递送目标按 `hedeleg[13]`；load-acquire 纯读**不涉 D 位**；记录实际观测 cause（若为 store 类则报告，同 HZLASR-07）；不得误判为 guest-page fault (cause 21/23) |
| HZLASR-32 | ADUE=0 时 store-release 到 A=1/D=0 页（D 位强制） | `henvcfg.ADUE`=0，VS-stage PTE A=1/D=0，VS-mode 执行 `sw.rl` | store page-fault (cause=15)——store-release **恒执行写入**，D 位要求为**强制**（非 UNSPECIFIED，同 HZAMO-28）；`norm:henvcfg_adue_op`：ADUE=0 时按 Svade 报页错误 |
| HZLASR-33 | ADUE=1 时 load-acquire/store-release 硬件更新 A/D 位后正常完成 | `henvcfg.ADUE`=1（且平台实现 Svadu），VS-stage PTE A=0/D=0，VS-mode 分别执行 `lw.aq` 与 `sw.rl` | `lw.aq`：硬件置 A 位（**不置 D**，纯读），加载正常完成，回读 PTE A=1/D=0；`sw.rl`：硬件置 A+D 位，存储正常完成，回读 PTE A=1/D=1（`norm:henvcfg_adue_op`）；平台未实现 Svadu 时 SKIP |

### 5.7 保留编码（load 无 aq / store 无 rl）的异常类型（Zalasr 特有）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLASR-34 | VS/VU-mode 保留 load-acquire/store-release 编码报 illegal-instruction (cause=2) 而非 cause=22 | V=1，VS-mode 与 VU-mode 分别以 raw encoding 注入 funct5=00110+aq=0（含 rl=0 的无注解 load 与 rl=1 的 load-release）与 funct5=00111+rl=0（含 aq=0 的无注解 store 与 aq=1 的 store-acquire）保留编码并执行（trap-armed） | illegal-instruction exception (cause=2)——`norm:ldaq_no_aq_reserved`/`norm:sdrl_no_rl_reserved` 规定这些编码为 RESERVED，在所有模式均非法（非 HS-qualified），故 V=1 时按 `norm:H_cause_virtual_instruction` **仍报 cause=2 而非 virtual-instruction (cause=22)**；若报 cause=22 即违反 SPEC，保持失败并记录至 `bugs/`（与 HZABHA-37 同构） |

### 5.8 架构边界：H 扩展无原子有序 load/store 虚拟机等价指令

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZLASR-35 | （记录型）无原子有序 load/store 虚拟机等价指令，但 HLV/HSV 可复制数据传输 | 以 trap-armed 探测 HS-mode 下是否存在按 `norm:hlsv_priv` 语义执行**带 acquire/release 排序注解的原子** load/store 的指令编码；对照 HLV.W/HSV.W 的可用性 | 记录性：`norm:hlsv_op` 仅映射 RV32I/RV64I 的 LB..LD / SB..SD（opcode 0x03/0x23），load-acquire/store-release（opcode 0x2F）无对应虚拟机指令。与 HZLRSC-39/HZAMO-30 共享“无原子虚拟机等价指令”结论，但**区别于 AMO**：HLV.W/HSV.W 可复制 load-acquire/store-release 的**数据传输**（因二者本即纯 load/store），只是无原子性与 aq/rl 排序注解。探测到的任何“虚拟机原子有序指令”编码须与 SPEC 比对后报告 |
| HZLASR-36 | （记录型）HLV/HSV 无法复制 load-acquire/store-release 的原子性与 RCsc 排序 | HS-mode 以 `HLV.W`+`HSV.W` 序列对 guest 内存做读改写，与 VS-mode 内的 `lw.aq`/`sw.rl` 并发（需多 hart），观测单拷贝原子性与 acquire/release 排序 | 记录性：HLV/HSV 为**非原子、无排序注解**的独立访问，并发下可观测到丢失更新与排序违例，而 VS-mode load-acquire/store-release 保持单拷贝原子性与 RCsc 排序；HLV/HSV 无 aq/rl 变体，故无法复制排序注解。多 hart 未就绪时 SKIP（与 HZLRSC-40/HZAMO-31/HZACAS-35/HZABHA-39 同理） |

> [!NOTE]
> - 本组所有测试必须以平台配置 `H_SUPPORTED` 宏为准检测 H 扩展；Zalasr 支持以平台配置 `ZALASR_SUPPORTED`（或 `ZALASR1P0P0_SUPPORTED`）宏为准，并以 trap-armed 执行 load-acquire/store-release 二次探测，不满足时全组 TEST_SKIP；**Zalasr 可独立于 Zaamo/Zalrsc/Zabha 实现（`norm:zalasr_builds_on_amo`），故门控不以 A 扩展宏（`ZAAMO_SUPPORTED`/`A_SUPPORTED`）为前置**（区别于 Group 2/3/4）。
> - **load-acquire 的 cause 归类为观测记录型（SPEC 歧义）**：zalasr.adoc 未明文规定 load-acquire/store-release 的异常 cause 归类（编码位于 AMO opcode 0x2F 空间），本组按功能语义（load-acquire 为纯加载→load 类、store-release 为纯存储→store/AMO 类）设计期望值，但**对 load-acquire 的具体 cause（HZLASR-07/12/22/23a/25/31）采用“记录实际观测值 + 与功能分类比对 + 偏差报告 bugs/”策略**，不武断强制判定；**store-release 的 cause（HZLASR-08/09/13/22/23b/26/32）因 store 与 AMO 同归 store/AMO 类而无歧义，为强制断言**。
> - **load-acquire 到 R=1/W=0 页正常执行为架构必然（HZLASR-06/10 强制正向断言）**：独立原子加载必须能读只读内存，若平台按 AMO 语义要求写权限而报 cause 15，则 load-acquire 无法读取只读变量、违背 `norm:zalasr_atomic_ordered` 的“原子加载”语义，记录实际 cause 并作为疑似缺陷报告至 `bugs/`。
> - **cause=22 为强制负向断言（有效指令）**：HZLASR-02/03 中 guest load-acquire/store-release 若报 virtual-instruction exception 即违反 SPEC（`hypervisor.adoc` 无任何条款对其施加 virtual-instruction 门控），应保持失败并记录至 `bugs/` 目录，不得放宽断言。
> - **保留编码报 cause=2 而非 cause=22（HZLASR-34 强制）**：依据 `norm:H_cause_virtual_instruction`——virtual-instruction 仅替代 HS-qualified 但 V=1 受阻的指令，保留编码（load 无 aq、store 无 rl，含 load-release/store-acquire）在所有模式均非法（非 HS-qualified），故报 illegal-instruction (cause=2)。若平台报 cause=22 即违反 SPEC，保持失败。
> - **htinst 统一走 opcode 0x2F transformed atomic 格式**：框架 `hyp_transform_mem_inst()` 对 opcode 0x2F 已实现“Atomic：保留除 bits19:15 外全部字段”，理论上涵盖 funct5=00110(load-acquire)/00111(store-release)；HZLASR-18~21 须以 load-acquire/store-release 实际指令字实测确认该分支对 funct5=00110/00111 的 golden 值计算正确（不排除框架实现按 funct5 分支细化处理的可能性，须实测验证而非假定复用即可，同 Group 3 HZACAS-17 对 funct5=00101 的处理原则）；HZLASR-22/23 的隐式遍历场景可复用 `setup_implicit_walk_victim()`。
> - **htinst 伪指令的强制性**：HZLASR-22 中当 `htval` 非零且故障源于 VS-stage 隐式访问时，`htinst` 写 0 属违反 `norm:H_trap_xtinst_guestpage`（“zero is not allowed”），应保持失败；HZLASR-18/19 的显式访问场景下 `htinst` 写 0 是允许的。
> - **MAG 放宽分支**：HZLASR-27 依赖平台声明 misaligned atomicity granule（`norm:zalasr_misaligned_pma_relax`/`norm:zalasr_misaligned_single_op`）；未声明 MAG 时未对齐 load-acquire/store-release 必产生异常（HZLASR-25/26），HZLASR-27 SKIP。未对齐异常允许 address-misaligned 与 access-fault 两类合规结果，用例验证 cause 属预期集合并记录实际值，不得因平台选择其中一类而判 FAIL。
> - **store-release 恒写 ⇒ D 位强制**：HZLASR-32 中 store-release 恒执行写入，故 ADUE=0 时对 D=0 页报 store page-fault 为**强制**结果（非记录型，同 HZAMO-28）；load-acquire 纯读**不涉 D 位**（HZLASR-31/33 验证 load-acquire 仅置 A 位、不置 D 位）。
> - **记录型用例不得强制判定**：HZLASR-35/36（无原子有序 load/store 虚拟机等价指令的架构边界）为架构许可/实现自定义行为，仅记录实现选择。
> - **多 hart 限制**：HZLASR-29/30 的 FIOM 排序效果断言与 HZLASR-36 需 secondary hart 支持；当前公共框架仅 hart 0 运行，相关断言在框架就绪前 SKIP（非 FAIL），单 hart 平台同样 SKIP 并注明。`norm:ldaq_rcsc_semantics`/`norm:sdrl_rcsc_semantics` 的 acquire/release RCsc 排序公理属多 hart 内存序属性，本组仅做单 hart 数据语义与 htinst 位保留验证，排序公理由 `Zalasr_test_plan.md` Group 5 覆盖。
> - **权限用例依赖**：5.2/5.3/5.6 需 VS-stage（`vsatp`）与 G-stage（`hgatp`）页表构造能力（R=0、R=1/W=0、A=0、D=0、G-stage 无写权限/无效映射等组合）；权限检查机制本身由 `pmp_test_plan.md`/`vm_test_plan.md` 覆盖，本组仅验证 load-acquire/store-release 的权限语义结论。
> - **RV32/RV64 适配**：`ld.aq`/`sd.rl`（funct3=011）为 RV64-only（`norm:ldaq_rv64_only`/`norm:sdrl_rv64_only`），RV32 平台不适用 `.d` 分支（HZLASR-01/04 中的 `.d` 抽样），复用 byte/half/word 用例；本组以 RV64 为主要目标，与 `Zalasr_test_plan.md` 一致。
> - Zalasr 非虚拟化语义（指令编码、原子加载/存储与符号扩展、aq/rl 约束与保留编码基础行为、RCsc 内存序与单拷贝原子性 litmus、对齐与 MAG 基础、M/S/U 各特权级基础可执行性、扩展独立性）由 `Zalasr_test_plan.md` 覆盖；主存 AMO 支持等级 PMA 由 `Ziccamoa_test_plan.md`/`Ziccamoc_test_plan.md` 覆盖，本组不重复。

---

## Group 6. Hypervisor × Zawrs 交叉测试

**规范依据**：
- `norm:Zawrs_virtual_instr_excp`：VS/VU-mode 下 `hstatus.VTW`=1、`mstatus.TW`=0 且 `wrs.nto` 未在实现限定时间内完成 → virtual-instruction exception
- `norm:Zawrs_exec_resume_rules`：wrs 指令遵循 `wfi` 的本地使能中断恢复规则；存在本地使能的 pending 中断时不停顿，VTW 异常路径不触发（对照 `Hypervisor_CSR_test_plan.md` 的 `norm:hstatus_vtw_op`/`norm:vtw_virtinstr` WFI 语义；`norm:vtw_virtinstr` 同时允许实现在 VTW=1 时总是触发 virtual-instruction，即使存在被全局屏蔽的 pending 中断，故 HZWRS-06 为记录型用例）
- `zawrs.adoc`（`norm:Zawrs_priv_illegal_instr_excp`）：`mstatus.TW`=1 时非 M-mode 执行 `wrs.nto` 未完成引发 illegal-instruction——TW 条款优先于 VTW，VS/VU-mode 亦按 illegal 报告（与 HSTAT-06 的 WFI 语义一致）
- `zawrs.adoc`：VTW 条款仅点名 `wrs.nto`，`wrs.sto` 由其短超时限定、不受 VTW 门控；wrs 指令在所有特权模式可用，HS-mode 不受 VTW 约束（VTW 仅作用于 V=1）
- `norm:H_virtinst_xtval`：virtual-instruction 异常的 stval 写入规则与 illegal-instruction 相同
- `norm:hstatus_spv_op`：VTW 异常自 VS/VU-mode 陷入 HS-mode 时 SPV=1

**测试职责**：验证 Zawrs 指令在虚拟化环境下的行为：HS/VS/VU-mode 正常执行不误触发 virtual-instruction exception；VS/VU-mode 下 `hstatus.VTW` 对 `wrs.nto` 的 virtual-instruction 门控；`mstatus.TW` 与 `hstatus.VTW` 的优先级与异常类型区分；VTW 条款的指令范围（仅 `wrs.nto`）。指令以 raw encoding 注入：`wrs.nto` = 0x00D00073（SYSTEM opcode=0x73，funct3=0，rd=0，funct12=0x0d），`wrs.sto` = 0x01D00073（funct12=0x1d）。**Zawrs 语义建立在 Zalrsc 的 reservation set 之上**（`wrs.nto` 等待保留集被写），故本组用例须先以 `lr` 建立保留集，与 Group 1 构成依赖关系。

### 6.1 HS/VS/VU-mode wrs 指令正常执行（VTW=0）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZWRS-01 | HS-mode 执行 wrs.nto/wrs.sto | `hstatus.VTW`=0，HS-mode 以 `lr` 建立保留集并前置**委托至 S 级的**（`mideleg.SS`=1、`sstatus.SIE`=0）本地使能 pending 软中断，依次执行 `wrs.nto` 与 `wrs.sto`（raw encoding） | 均正常完成，无异常 |
| HZWRS-02 | VS-mode 执行 wrs.nto/wrs.sto | `hstatus.VTW`=0、`mstatus.TW`=0，前置 VS 级 pending 软中断（`hvip.VSSIP` 经 `hideleg` 路由至 VS、`vsie.SSIE`=1、`vsstatus.SIE`=0），VS-mode 依次执行两条指令 | 均正常完成，不误触发 virtual-instruction exception |
| HZWRS-03 | VU-mode 执行 wrs.nto/wrs.sto | 同上配置，但 VU-mode 无法维持 pending 唤醒源（任何本地使能中断的目标特权级均高于 U，进入 VU-mode 即被取走），改为以看门狗 M-timer（MTIE+MIE）兑底：`wrs` 可合法停顿，M-timer 到期打断停顿并恢复执行 | 两条指令均在有限时间内完成，无同步异常（trap 记录为空或仅含 M-timer 中断） |

### 6.2 hstatus.VTW 门控（VS/VU-mode wrs.nto）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZWRS-04 | VS-mode VTW=1 wrs.nto 触发 virtual-instruction | `hstatus.VTW`=1、`mstatus.TW`=0，屏蔽所有本地使能中断，VS-mode 执行 `wrs.nto`（trap-armed） | virtual-instruction exception (cause=22) |
| HZWRS-05 | VU-mode VTW=1 wrs.nto 触发 virtual-instruction | 同配置，VU-mode 执行 `wrs.nto` | virtual-instruction exception (cause=22) |
| HZWRS-06 | VTW=1 但本地使能中断已 pending（记录型） | `hstatus.VTW`=1，置位并本地使能一个中断后，VS-mode 执行 `wrs.nto` | 两种行为均合法：指令立即完成（不停顿，`norm:Zawrs_exec_resume_rules`）；或按 `norm:vtw_virtinstr` 的 VTW 拦截许可报 virtual-instruction (cause=22)。记录实现选择，若触发异常必须为 cause=22，不做强制判定 |
| HZWRS-07 | VTW 异常的 trap 报告 | 承接 HZWRS-04 场景，检查 HS-mode trap 现场 | cause=22，stval 按 `norm:H_virtinst_xtval` 规则写入（指令编码或 0），hstatus.SPV=1，handler 跳过后正常恢复 |

### 6.3 TW 优先级与指令范围（VS/VU-mode）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZWRS-08 | VTW=1 且 TW=1 时 VS-mode 报 illegal | `hstatus.VTW`=1、`mstatus.TW`=1，VS-mode 执行 `wrs.nto` | illegal-instruction exception (cause=2)（TW 条款优先，`norm:Zawrs_priv_illegal_instr_excp`） |
| HZWRS-09 | VTW=1 且 TW=1 时 VU-mode 报 illegal | 同配置，VU-mode 执行 `wrs.nto` | illegal-instruction exception (cause=2) |
| HZWRS-10 | VTW=1 仅作用于 wrs.nto | `hstatus.VTW`=1、`mstatus.TW`=0，VS-mode 执行 `wrs.sto` | 短超时后正常完成，无异常（VTW 条款仅点名 `wrs.nto`） |
| HZWRS-11 | VTW 不影响 HS-mode | `hstatus.VTW`=1，HS-mode 执行 `wrs.nto` | 正常完成，无异常（VTW 仅作用于 V=1） |
| HZWRS-12 | VS-mode 下 TW=1 单独生效 | `mstatus.TW`=1、`hstatus.VTW`=0，VS-mode 执行 `wrs.nto` | illegal-instruction exception (cause=2)（对照 `Hypervisor_CSR_test_plan.md` HSTAT-06 的 WFI 语义） |

> [!NOTE]
> - 本组所有测试必须以平台配置 `H_SUPPORTED` 宏为准检测 H 扩展，并以 trap-armed raw encoding 探测 Zawrs 支持（未实现时全套 TEST_SKIP）；Zawrs 依赖 Zalrsc，用例须先以 `lr` 建立保留集（该 `lr` 的语义验证归 Group 1，本组仅将其作为前置手段）。
> - 与 WFI/VTW 用例（HSTAT-04）同样存在时序依赖：VTW 异常要求 `wrs.nto` "未在实现限定时间内完成"。若实现按 `norm:Zawrs_stall_terminate` 以极短时长提前终止停顿导致异常不触发，应保持用例失败并记录至 `bugs/` 目录，不得放宽断言。
> - HZWRS-06 为记录型用例：`norm:vtw_virtinstr` 允许实现在 VTW=1 时总是触发 virtual-instruction（即使存在被全局屏蔽的 pending 中断），因此"立即完成"与"报 cause=22"均为合法实现；用例仅记录实现选择并约束异常类型（若触发必须为 cause=22），同时避免依赖实现停顿时长。
> - HZWRS-08/09 验证异常类型判定：`mstatus.TW`=1 时按 `norm:Zawrs_priv_illegal_instr_excp` 报 illegal-instruction (cause=2)，不得报 virtual-instruction；断言必须使用精确 cause 常量。
> - 中断环境：本组 VS/VU 用例需在受控中断环境下执行（HZWRS-01~02、HZWRS-06 需前置本地使能的 pending 中断；HZWRS-04/05 需屏蔽所有本地使能中断），避免中断递送污染 trap 记录。**pending 唤醒源的特权级生存性**：未委托的 SSIP 目标为 M-mode，一旦 hart 降至 M 以下即被合法取走（`machine.adoc` `norm:intr_mip_mie_op` 条件 (a)：当前特权级低于目标模式即取中断），故 HS-mode 用例必须先将 SSIP 委托至 S 级（`mideleg.SS`=1，配合 `sstatus.SIE`=0 使其在 HS-mode 下保持 pending）；VS-mode 用例使用 `hvip.VSSIP`（VS 级目标，`vsstatus.SIE`=0 下保持 pending）；VU-mode 下任何 pending 使能中断都会在进入 VU-mode 时被取走，改用看门狗兜底验证“不永久挂死”。
> - Zawrs 非 Hypervisor 场景（编码与可用性、停顿与恢复、TW 基础行为）由 `Zawrs_test_plan.md` 覆盖；`hstatus.VTW`/`mstatus.TW` 对 WFI 的基础门控由 `Hypervisor_CSR_test_plan.md`（HSTAT-04/06）覆盖，本组不重复。

---

## 关键注意事项

1. **扩展检测**：所有测试必须以平台配置 `H_SUPPORTED` 宏为准检测 H 扩展，不可用时 TEST_SKIP。Zalrsc 以平台配置 `ZALRSC_SUPPORTED` 宏为准并以 trap-armed raw encoding 二次探测；Zaamo 以平台配置 `ZAAMO_SUPPORTED`（或 `A_SUPPORTED`）宏为准并以 trap-armed 执行 AMO 二次探测；Zacas 以平台配置 `ZACAS_SUPPORTED` 宏为准并以 trap-armed 执行 amocas 二次探测（依赖 Zaamo，故 `ZAAMO_SUPPORTED`/`A_SUPPORTED` 亦须满足）；Zabha 以平台配置 `ZABHA_SUPPORTED` 宏为准并以 trap-armed 执行字节/半字 AMO 二次探测（依赖 Zaamo，故 `ZAAMO_SUPPORTED`/`A_SUPPORTED` 亦须满足；其 `amocas.b/h` 用例另需 `ZACAS_SUPPORTED`）；Zalasr 以平台配置 `ZALASR_SUPPORTED`（或 `ZALASR1P0P0_SUPPORTED`）宏为准并以 trap-armed 执行 load-acquire/store-release 二次探测（**可独立于 Zaamo/Zalrsc/Zabha 实现，不以 A 扩展宏为前置**，`norm:zalasr_builds_on_amo`）；Zawrs 无独立探测标志，以 trap-armed raw encoding（`wrs.nto`=0x00D00073、`wrs.sto`=0x01D00073）探测，未实现时全套 TEST_SKIP。

2. **指令注入约定**：LR/SC 若工具链不支持助记符则以 raw encoding 注入（AMO opcode=0x2F，funct5=00010(LR)/00011(SC)，funct3=010(.w)/011(.d)，bit26=aq、bit25=rl，LR 的 rs2 字段置 0）；AMO 基础原语（9 条 × .w/.d）已由 `common/mem_ops.h` 提供，aq/rl 变体若缺原语则以 raw encoding 注入（opcode=0x2F，funct5：amoadd=00000/amoswap=00001/amoxor=00100/amoor=00110/amoand=00111/amomin=01000/amomax=01001/amominu=01100/amomaxu=01101）；amocas 若工具链（`-march` 含 `zacas`）不支持助记符则以 raw encoding 注入（opcode=0x2F，funct5=00101，funct3=010(.w)/011(.d)/100(.q)，bit26=aq、bit25=rl），RV32 的 `amocas.d`/RV64 的 `amocas.q` 须使用偶数寄存器对（rd/rd+1、rs2/rs2+1）；字节/半字 AMO 若工具链（`-march` 含 `zabha`）不支持助记符则以 raw encoding 注入（opcode=0x2F，funct5 同 word AMO，**funct3=000(.b)/001(.h)**，bit26=aq、bit25=rl），`amocas.b/h` 为 funct5=00101 + funct3=000/001，保留字节/半字 lr/sc 为 funct5=00010(lr)/00011(sc) + funct3=000/001；load-acquire/store-release 若工具链（`-march` 含 `zalasr`）不支持助记符则以 raw encoding 注入（opcode=0x2F，load-acquire funct5=00110 且 rs2 字段置 0、aq(bit26)=1，store-release funct5=00111 且 rd 字段置 0、rl(bit25)=1，funct3=000(.b)/001(.h)/010(.w)/011(.d，RV64)），保留编码为 load funct5=00110+aq=0 / store funct5=00111+rl=0；wrs 指令一律以 raw encoding 注入（SYSTEM opcode=0x73）。上述指令统一 `.option norvc` 保证 4 字节指令长度，便于 trap handler 按 sepc+4 跳过故障指令。

3. **Group 1 Zalrsc 交叉要点**：
    - **异常类别与 cause 二分**：LR 为 load 访问（cause 4/5/13/21）、SC 为 store 访问（cause 6/7/15/23），依据 `norm:mcause_exccode_st_sc_amo`、`norm:load_page_fault_no_r`、`norm:store_page_fault_no_w`。断言必须使用精确 cause 常量，不得混淆两类，也不得将 VS-stage 故障（13/15）误判为 G-stage 故障（21/23）。
    - **委托路径二分**：`hedeleg` bit 4/5/6/7/13/15 可写（VS-stage 故障可下放 VS-mode），bit 21/23 只读零（G-stage 故障架构强制陷入 HS-mode）。HZLRSC-05 先探测位属性以确立后续用例前提。
    - **htinst 双路径消歧**：显式访问故障→ transformed atomic（保留 funct5/aq/rl/funct3/rd/rs2，仅 bits19:15 ← Addr. Offset）或 0；隐式 VS-stage 页表遍历故障→ **必须**为伪指令（读 0x00003000 / A-D 写 0x00003020，RV64）且不允许 0。后者写 0 属违反 `norm:H_trap_xtinst_guestpage`，应保持失败。
    - **失败 SC 仍报 store 类异常**（最易被实现忽略的点）：`norm:sc_retire_permission` 为强制（shall），故无 reservation、逻辑上不写内存的 SC 指向无写权限页时仍须报 cause=15（VS-stage）或 cause=23（G-stage）（HZLRSC-23/24）。
    - **cause=22 负向断言**：`hypervisor.adoc` 无任何条款对 LR/SC 施加 virtual-instruction 门控，guest LR/SC 报 cause=22 即为违反 SPEC，不得放宽断言。
    - **未对齐无 MAG 放宽**：Zalrsc 区别于 Zalasr——未对齐 LR/SC 必产生异常，无“同一 MAG 内无异常原子完成”分支；且不拆分访问，故 `stval`=未对齐地址本身、htinst 的 Addr. Offset 恒为 0。address-misaligned 与 access-fault 两类结果均合规，不得因平台选择其中一类而判 FAIL。
    - **记录型用例清单**：HZLRSC-32/33（失败 SC 的 PTE D 位副作用，VS-stage/G-stage 双重 UNSPECIFIED）、HZLRSC-37（不受约束序列）、HZLRSC-38（trap 往返后 reservation 存续）、HZLRSC-39/40（无 HLR/HSC 架构边界）仅记录实现选择，不得强制判定。
    - **框架就绪度**：`hyp_transform_mem_inst()`（opcode 0x2F 分支）已提供 LR/SC 的 transformed golden 值计算，`setup_implicit_walk_victim()` 已提供隐式遍历故障构造能力，HZLRSC-15~21 无需新增基础设施。

4. **Group 2 Zaamo 交叉要点**：
    - **异常统一归 store/AMO 类**（与 LR 的根本区别）：AMO 即使含读阶段，全部异常归 store/AMO 类（misaligned=6、access fault=7、page fault=15、guest-page fault=23），绝不报 load 类（4/5/13/21），依据 `norm:mcause_exccode_st_sc_amo` 与 `norm:store_page_fault_no_w` 配套 NOTE（AMO 到不可读页亦报 cause=15 而非 13）。
    - **AMO 需 R+W 权限**：AMO 对 R=1/W=0 或 R=0 页报 cause=15（VS-stage）/ cause=23（G-stage）；HZAMO-09 以 LR（仅需 R、正常执行）与 AMO（报 15）的对比确立该分野。
    - **隐式遍历故障报 cause=23**：`norm:H_vm_gpapriv` 规定隐式 VS-stage 访问按原始类型报告，故 AMO 的隐式 PTE 遍历 GPF 报 cause=23（HZAMO-18），区别于 LR 的 cause=21（HZLRSC-19）。
    - **MAG 放宽为 Zaamo 独有**：HZAMO-22/23 依赖平台声明 MAG，未声明时未对齐 AMO 必报 cause=6/7（HZAMO-21）、HZAMO-22/23 SKIP；对照 Zalrsc 无 MAG 放宽（HZLRSC-27/28）。
    - **AMO 必写 ⇒ D 位强制**：HZAMO-28（ADUE=0 + D=0 → cause=15）为强制结果，区别于失败 SC 的 D 位副作用 UNSPECIFIED（HZLRSC-32/33 记录型）。
    - **框架就绪度**：`hyp_transform_mem_inst()`（opcode 0x2F 分支）已提供 AMO 的 transformed golden 值计算，`common/mem_ops.h` 已提供 9 条 AMO 的 `.w`/`.d` 原语；aq/rl 变体（HZAMO-16/25）与隐式遍历构造（HZAMO-18/19，复用 `setup_implicit_walk_victim()`）若缺原语须补充。
    - **记录型用例清单**：HZAMO-30/31（无 AMO 虚拟机等价指令的架构边界）仅记录实现选择，不得强制判定。

5. **Group 3 Zacas 交叉要点**：
    - **异常统一归 store/AMO 类**（与 Zaamo 一致）：amocas 属 AMO 指令族（opcode 0x2F，funct5=00101），全部异常归 store/AMO 类（misaligned=6、access fault=7、page fault=15、guest-page fault=23），绝不报 load 类（4/5/13/21），依据同 Group 2（`norm:mcause_exccode_st_sc_amo`）。
    - **无条件写权限检查为 Zacas 特有强制维度**：`norm:Zacas_amocas_w_permission`（“always requires write permissions”）不区分比较成功/失败，故 HZACAS-07/12（失败 CAS 仍报 cause=15/23）为**强制**判定，与 Group 1 的 HZLRSC-22~26（失败 SC 权限检查）同构，区别于 Group 2 中 Zaamo 无“失败”分支（AMO 恒无条件写回）。
    - **失败 CAS 的 D 位副作用为 SPEC 空白点（记录型）**：HZACAS-32/33 因 Zacas SPEC 未像 Zalrsc 的 `norm:sc_failed_side_effects` 那样明文规定 UNSPECIFIED，按记录型处理，不做强制判定，须显式标注该规范差异（区别于 HZLRSC-32/33 有明文依据、HZAMO-28 为强制结果）。
    - **隐式遍历故障报 cause=23**：同 Group 2，`norm:H_vm_gpapriv` 规定隐式 VS-stage 访问按原始类型报告，故 amocas 的隐式 PTE 遍历 GPF 报 cause=23（HZACAS-20）。
    - **MAG 放宽经跨引用适用**：HZACAS-24/25 依赖平台声明 MAG（经 `norm:Zacas_amocas_rs1_addr_alignment` 的“the same exception options apply”跨引用 `norm:misaligned_atomicity_granule_size`），未声明时未对齐 amocas 必报 cause=6/7（HZACAS-23）、HZACAS-24/25 SKIP。
    - **框架就绪度**：`hyp_transform_mem_inst()`（opcode 0x2F 分支）理论上涵盖 funct5=00101（amocas），但须以 HZACAS-17~19 实测确认 golden 值计算正确，不得假定复用即可；aq/rl 变体（HZACAS-18/27）与隐式遍历构造（HZACAS-20/21，复用 `setup_implicit_walk_victim()`）若缺原语须补充。
    - **记录型用例清单**：HZACAS-32/33（失败 CAS 的 PTE D 位副作用，SPEC 空白点）、HZACAS-34/35（无 amocas 虚拟机等价指令的架构边界）仅记录实现选择，不得强制判定。

6. **Group 4 Zabha 交叉要点**：
    - **异常统一归 store/AMO 类（宽度无关）**：字节/半字 AMO 与 `amocas.b/h` 属 AMO 指令族（opcode 0x2F，funct3=000/001），全部异常归 store/AMO 类（misaligned=6、access fault=7、page fault=15、guest-page fault=23），绝不报 load 类（4/5/13/21），依据同 Group 2/3（`norm:mcause_exccode_st_sc_amo`）；权限要求与访问宽度无关（HZABHA-09 以 .b/.h/.w 三者均报 cause=15 确立）。
    - **htinst funct3 宽度保留为 Zabha 特有强制维度**：HZABHA-16/35 须实测确认框架 `hyp_transform_mem_inst()` 的 opcode 0x2F 分支正确保留 funct3=000(.b)/001(.h)（bits14:12），使 HS-mode 能从 htinst 判定陷入访问宽度；不得假定与 word/dword AMO 共享分支即宽度编码自动正确。
    - **字节 AMO 无未对齐场景（区别于 Zaamo）**：HZABHA-22 验证字节 AMO 1 字节对齐恒成立、任意地址正常完成、绝不报 cause=6/7；未对齐异常与 MAG 放宽分支（HZABHA-23~26、HZABHA-36）仅对半字 AMO（奇地址）可测。
    - **rd 符号扩展与 rs2/rd 高位忽略在虚拟化下不变**：HZABHA-04/32 验证 `norm:Zabha_rd_sign_extension`（rd 符号扩展 8/16 位、忽略 rs2 高位）与 `norm:Zabha_amocas-BH_ignore_bits`（amocas.b/h 忽略 rd 高位比较）在 VS/VU-mode 下与 HS-mode 一致。
    - **失败 CAS 仍需写权限检查（amocas.b/h）**：HZABHA-33 依据 `norm:Zacas_amocas_w_permission` 的“always”无条件语言，成功/失败 CAS 到 R=1/W=0 页均报 cause=15，与 Group 3 的 HZACAS-07/12 同构（本组以字节/半字宽度复验）。
    - **保留字节/半字 lr/sc 报 cause=2 而非 cause=22**：HZABHA-37 依据 `norm:H_cause_virtual_instruction`——保留编码非 HS-qualified，V=1 时仍报 illegal-instruction (cause=2)；若报 cause=22 即违反 SPEC。
    - **AMO 必写 ⇒ D 位强制**：HZABHA-30（ADUE=0 + D=0 → cause=15）为强制结果，同 HZAMO-28。
    - **框架就绪度**：`hyp_transform_mem_inst()`（opcode 0x2F 分支）须以 HZABHA-16/35 实测确认对 funct3=000/001 的 golden 值计算正确；`common/mem_ops.h` 若仅提供 `.w`/`.d` 原语，字节/半字原语与 aq/rl 变体（HZABHA-17/27）须补充或以 raw encoding 注入；隐式遍历构造（HZABHA-19/20）复用 `setup_implicit_walk_victim()`。
    - **记录型用例清单**：HZABHA-38/39（无字节/半字原子虚拟机等价指令的架构边界）仅记录实现选择，不得强制判定。

7. **Group 5 Zalasr 交叉要点**：
    - **异常归类二分（load-acquire→load 类，store-release→store/AMO 类）**：load-acquire 为纯加载（`norm:ldaq_atomic_load_op`）功能上归 load 类（cause 4/5/13/21，仅需读权限），store-release 为纯存储（`norm:sdrl_atomic_store_op`）归 store/AMO 类（cause 6/7/15/23，需写权限）；与 Group 1 的 LR/SC 二分同构但机制不同（Zalrsc 为成对读改写的两半，Zalasr 为两条独立指令），区别于 Group 2/3/4（全 AMO 统一归 store/AMO 类）。
    - **load-acquire 的 cause 归类为观测记录型（SPEC 歧义）**：zalasr.adoc 未明文规定 load-acquire/store-release 的 cause 归类（编码位于 AMO opcode 空间），故 **load-acquire 的具体 cause（HZLASR-07/12/22/23a/25/31）采用“记录实际观测值 + 与功能分类（load 类）比对 + 偏差报告 bugs/”策略，不武断强制判定**；**store-release 的 cause（HZLASR-08/09/13/22/23b/26/32）因 store 与 AMO 同归 store/AMO 类无歧义，为强制断言**。
    - **load-acquire 到 R=1/W=0 页正常执行为架构必然**：HZLASR-06/10 强制正向断言——独立原子加载必须能读只读内存，若平台按 AMO 语义报 cause 15 则违背 `norm:zalasr_atomic_ordered`，记录并报告 `bugs/`。
    - **隐式遍历故障按原始类型报告（load-acquire→21、store-release→23）**：`norm:H_vm_gpapriv`，与 Group 1（LR→21、SC→23）一致，区别于 Group 2/3/4（全 AMO→23）。
    - **htinst 统一走 opcode 0x2F transformed atomic 格式**：load-acquire/store-release 虽为纯 load/store 却用 `transformedatomicinst`（非 transformedload/storeinst），HZLASR-18~21 须实测确认框架 `hyp_transform_mem_inst()` 对 funct5=00110/00111 的 golden 值计算正确（同 Group 3 对 funct5=00101 的实测原则）；HS-mode 可由 funct5 区分 load/store。
    - **FIOM 恒适用（Zalasr 恒带 aq/rl）**：load-acquire 恒 aq=1、store-release 恒 rl=1，故 `norm:henvcfg_fiom_order` 的排序修改对 Zalasr 恒适用（HZLASR-29/30），无需构造特定 aq/rl 变体（区别于 Group 1/2/3/4）。
    - **store-release 恒写 ⇒ D 位强制；load-acquire 纯读不涉 D**：HZLASR-32（ADUE=0 + D=0 → cause 15）为强制（同 HZAMO-28）；HZLASR-31/33 验证 load-acquire 仅置 A 位、不置 D 位。
    - **保留编码报 cause=2 而非 cause=22**：HZLASR-34 依据 `norm:H_cause_virtual_instruction`（保留 load 无 aq/store 无 rl 编码非 HS-qualified），同 HZABHA-37。
    - **门控独立于 A 扩展**：Zalasr 可独立实现（`norm:zalasr_builds_on_amo`），门控仅依赖 `ZALASR_SUPPORTED`，不以 `ZAAMO_SUPPORTED`/`A_SUPPORTED` 为前置（区别于 Group 2/3/4）。
    - **框架就绪度**：`hyp_transform_mem_inst()`（opcode 0x2F 分支）须以 HZLASR-18~21 实测确认对 funct5=00110/00111 的 golden 值；`common/mem_ops.h` 若无 load-acquire/store-release 原语须补充或以 raw encoding 注入；隐式遍历构造（HZLASR-22/23）复用 `setup_implicit_walk_victim()`。
    - **记录型用例清单**：HZLASR-35/36（无原子有序 load/store 虚拟机等价指令的架构边界）仅记录实现选择，不得强制判定。

8. **Group 6 Zawrs 时序依赖**：`wrs.nto` 用例依赖“未在实现限定时间内完成”这一时序条件（与 `Hypervisor_CSR_test_plan.md` HSTAT-04 的 WFI/VTW 用例同风格）。实现若按 `norm:Zawrs_stall_terminate` 提前终止停顿，应保持用例失败并记录 `bugs/`，不得放宽断言。HZWRS-06 为记录型（`norm:vtw_virtinstr` 允许两种合法行为）。

9. **virtual-instruction 与 illegal-instruction 的区分**（多组共通）：Group 1 中 LR/SC、Group 2 中 AMO、Group 3 中 amocas、Group 4 中字节/半字 AMO 与 amocas.b/h、Group 5 中 load-acquire/store-release 均**绝不**应报 cause=22（负向断言，`hypervisor.adoc` 无任何条款对原子指令施加 virtual-instruction 门控）；Group 4 中保留字节/半字 lr/sc 编码、Group 5 中保留 load-acquire（无 aq）/store-release（无 rl）编码因非 HS-qualified 而**必须**报 illegal-instruction (cause=2) 而非 cause=22（`norm:H_cause_virtual_instruction`）；Group 6 中 `wrs.nto` 在 VTW=1/TW=0 时**必须**报 cause=22、在 TW=1 时**必须**报 cause=2（正向断言）。断言必须使用精确 cause 常量，不得混淆。

10. **多 hart 限制**：HZLRSC-30/31、HZAMO-25/26、HZACAS-27/28、HZABHA-27/28 与 HZLASR-29/30 的 FIOM 排序效果断言、HZLRSC-40、HZAMO-31、HZACAS-35、HZABHA-39 与 HZLASR-36 需 secondary hart 支持；当前公共框架仅 hart 0 运行，相关断言在框架就绪前 SKIP（非 FAIL），单 hart 平台同样 SKIP 并注明。

11. **失败处置**：任一平台出现 LR/SC 异常类别错误（LR 报 store 类或 SC 报 load 类）、AMO/amocas/字节半字 AMO 异常类别错误（报 load 类 4/5/13/21，或隐式遍历 GPF 报 cause=21 而非 23）、**store-release 异常类别错误（报 load 类 4/5/13/21，或隐式遍历 GPF 报 cause=21 而非 23）、load-acquire 到 R=1/W=0 页报 store page-fault（违背纯加载语义，HZLASR-06/10；load-acquire 其余 cause 归类因 SPEC 歧义按观测记录报告而非直接判失败）**、guest-page fault 未强制陷入 HS-mode、htinst 伪指令写 0 或 funct3 宽度编码未保留（HZABHA-16/35）、失败 SC 或失败 CAS（含 amocas.b/h）未做写权限检查（HZLRSC-23/24、HZACAS-07/12、HZABHA-33 对应的强制断言失败）、guest LR/SC/AMO/amocas/字节半字 AMO/load-acquire/store-release 报 cause=22、保留字节/半字 lr/sc 编码或保留 load-acquire/store-release 编码报 cause=22 而非 cause=2（HZABHA-37、HZLASR-34）、受约束循环活锁、或 Zawrs 的 VTW/TW 异常类型判定错误，均属违反 SPEC 的实现缺陷，用例保持 FAIL 并记录至 `bugs/` 目录（含平台、指令、地址、期望/实际 cause 与 CSR 现场），严禁以跳过或 workaround 方式规避。

---

## 参考

- `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — RISC-V Hypervisor Extension, Version 1.0
- `SPEC/riscv-isa-manual/src/unpriv/zalrsc.adoc` — Zalrsc Extension for Load-Reserved/Store-Conditional Instructions
- `SPEC/riscv-isa-manual/src/unpriv/zaamo.adoc` — Zaamo Extension for Atomic Memory Operations
- `SPEC/riscv-isa-manual/src/unpriv/zacas.adoc` — Zacas Extension for Atomic Compare-and-Swap (CAS) Instructions
- `SPEC/riscv-isa-manual/src/unpriv/zabha.adoc` — Zabha Extension for Byte and Halfword Atomic Memory Operations
- `SPEC/riscv-isa-manual/src/unpriv/zalasr.adoc` — Zalasr Extension for Atomic Load-Acquire and Store-Release Instructions
- `SPEC/riscv-isa-manual/src/unpriv/zawrs.adoc` — Zawrs Extension for Wait-on-Reservation-Set
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — `mcause` 异常编码归类（load/load-reserved → load 类；store、store-conditional、AMO → store/AMO 异常类）
- `SPEC/riscv-isa-manual/src/priv/supervisor.adoc` — 页级权限与异常归类、硬件 A/D 更新原子性与页表 RsrvEventual PMA 要求
- `SPEC/riscv-isa-manual/src/unpriv/rvwmo.adoc` — RVWMO 内存模型（Atomicity Axiom、RCsc 排序注解与 PPO 规则）
- `DOCS/testplan/Zalrsc_test_plan.md` — Zalrsc 独立测试计划（非 Hypervisor 场景）
- `DOCS/testplan/Zawrs_test_plan.md` — Zawrs 独立测试计划（非 Hypervisor 场景）
- `DOCS/testplan/Ziccrse_test_plan.md` — Ziccrse 测试计划（主存 RsrvEventual PMA 与受约束 LR/SC 循环前向进展保证）
- `DOCS/testplan/Hypervisor_Zi_test_plan.md` — Hypervisor 与 Z* 扩展交叉测试计划（本方案的拆分来源，保留 Zkr/Zihintntl/Zcmt/V/Zicntr/Zihpm 交叉）
- `DOCS/testplan/Hypervisor_CSR_test_plan.md` — Hypervisor CSR 子集测试计划（`hstatus.VTW`/`mstatus.TW` 对 WFI 的门控，HSTAT-04/06）
- `DOCS/testplan/Hypervisor_Exceptions_test_plan.md` — Hypervisor 异常与 trap 子集测试计划
- `DOCS/testplan/Hypervisor_2_stage_test_plan.md` — 两阶段翻译测试计划
- `DOCS/testplan/Hypervisor_gstage_test_plan.md` — G-stage 独立测试计划
- `DOCS/testplan/Zaamo_test_plan.md` — Zaamo 独立测试计划（非 Hypervisor 场景；其 Hypervisor 交叉由本文档 Group 2 覆盖）
- `DOCS/testplan/Zacas_test_plan.md` — Zacas 独立测试计划（非 Hypervisor 场景；其 Hypervisor 交叉由本文档 Group 3 覆盖）
- `DOCS/testplan/Ziccamoa_test_plan.md` — Ziccamoa 测试计划（主存 AMOArithmetic 级 PMA 支持保证，与 Group 2 的 Hypervisor 交叉视角互补）
- `DOCS/testplan/Ziccamoc_test_plan.md` — Ziccamoc 测试计划（主存 AMOCASQ 级 PMA 支持保证，与 Group 3 的 Hypervisor 交叉视角互补）
- `DOCS/testplan/Shtvala_test_plan.md` — Shtvala 测试计划（HTVAL-AMO-01 强化 AMO GPF 的 htval 必须非零，与 HZAMO-12 的基线 H 行为互补）
- `DOCS/testplan/Zabha_test_plan.md` — Zabha 独立测试计划（非 Hypervisor 场景；其 Hypervisor 交叉由本文档 Group 4 覆盖）
- `DOCS/testplan/Zalasr_test_plan.md` — Zalasr 独立测试计划（非 Hypervisor 场景；其 Hypervisor 交叉由本文档 Group 5 覆盖）

---

## 附录 A：规范点覆盖矩阵

下表标明"覆盖的规范点"章节中每条规范点被哪些测试用例覆盖。

| Norm ID | 覆盖的测试 ID |
|---------|---------------|
| `norm:lr_w_op` | HZLRSC-01、HZLRSC-02、HZLRSC-04、HZLRSC-06、HZLRSC-25 |
| `norm:sc_w_success` | HZLRSC-01、HZLRSC-02、HZLRSC-04、HZLRSC-07 |
| `norm:sc_w_failure` | HZLRSC-04、HZLRSC-38 |
| `norm:sc_retire_permission` | HZLRSC-22、HZLRSC-23、HZLRSC-24、HZLRSC-26 |
| `norm:sc_failed_side_effects` | HZLRSC-32、HZLRSC-33（记录型：UNSPECIFIED，置位与保持均合法） |
| `norm:sc_failed_as_store` | HZLRSC-23、HZLRSC-24、HZLRSC-26 |
| `norm:sc_reservation_invalidate` | HZLRSC-38（记录型：trap 往返后 reservation 存续由实现决定）；HZWRS-01~03（以 `lr` 建立保留集作为前置手段） |
| `norm:lr_sc_rv64` | HZLRSC-01、HZLRSC-02（`.d` 宽度在 HS/VS-mode 执行） |
| `norm:lr_sc_alignment` | HZLRSC-18、HZLRSC-27、HZLRSC-28、HZLRSC-29 |
| `norm:lr_sc_aq_rl_software_rule` | HZLRSC-17、HZLRSC-30 |
| `norm:lrsc_eventuality_region` | HZLRSC-35 |
| `norm:constrained_lrsc_forward_progress_intro` | HZLRSC-35、HZLRSC-36 |
| `norm:constrained_lrsc_forward_progress_trap` | HZLRSC-36（“H traps”事件使虚拟化抢占下保证即已满足） |
| `norm:unconstrained_lrsc_no_progress` | HZLRSC-37（记录型：不设成功断言） |
| `norm:mcause_exccode_ld_ldrsv` | HZLRSC-06、HZLRSC-09、HZLRSC-27（LR 归 load 类）；HZLASR-06、HZLASR-07、HZLASR-12、HZLASR-22、HZLASR-25、HZLASR-31（load-acquire 功能上归 load 类，观测记录型） |
| `norm:mcause_exccode_st_sc_amo` | HZLRSC-07、HZLRSC-10、HZLRSC-22、HZLRSC-23、HZLRSC-24、HZLRSC-26、HZLRSC-28、HZAMO-06、HZAMO-07、HZAMO-10、HZAMO-18、HZAMO-21、HZACAS-06、HZACAS-07、HZACAS-08、HZACAS-11、HZACAS-12、HZACAS-20、HZACAS-23、HZABHA-06、HZABHA-07、HZABHA-10、HZABHA-19、HZABHA-23、HZABHA-33、HZABHA-34、HZLASR-08、HZLASR-09、HZLASR-13、HZLASR-22、HZLASR-26、HZLASR-32（store-release 归 store/AMO 类，强制） |
| `norm:load_page_fault_no_r` | HZLRSC-06、HZLRSC-25、HZAMO-09（LR 仅需读权限，与 AMO 需 R+W 的对比）、HZLASR-06、HZLASR-07、HZLASR-10（load-acquire 仅需读权限，与 store-release 需写的对比） |
| `norm:store_page_fault_no_w` | HZLRSC-07、HZLRSC-22、HZLRSC-23、HZLRSC-26、HZAMO-06、HZAMO-07、HZAMO-09、HZAMO-27、HZAMO-28、HZACAS-06、HZACAS-07、HZACAS-08、HZACAS-09、HZACAS-10、HZACAS-29、HZACAS-30、HZABHA-06、HZABHA-07、HZABHA-09、HZABHA-29、HZABHA-30、HZABHA-33、HZLASR-08、HZLASR-09、HZLASR-10、HZLASR-32（store-release 需写权限） |
| `ptmem_rsrv_eventual` | HZLRSC-35（作为前置条件引用；主存 RsrvEventual 保证本身由 `Ziccrse_test_plan.md` 覆盖） |
| `amo_rmw_semantics` | HZAMO-01、HZAMO-02、HZAMO-04 |
| `norm:amo_operand_size` | HZAMO-04 |
| `norm:amo_alignment` | HZAMO-21、HZAMO-24、HZABHA-23、HZABHA-26、HZABHA-36（经 `norm:Zabha_rs1_align_addr` 跨引用，仅半字 AMO） |
| `norm:misaligned_atomicity_granule_size` | HZAMO-22、HZAMO-23、HZABHA-24、HZABHA-25、HZABHA-36（仅半字 AMO 可测未对齐） |
| `norm:amo_release_consistency` | HZAMO-16、HZAMO-25、HZABHA-17、HZABHA-27 |
| `norm:Zacas_rv64_amocas-d_op` | HZACAS-01、HZACAS-02、HZACAS-04（代表核心 CAS 语义在 VS/VU-mode 下不变；`amocas.w`/`amocas.q` 及 RV32 寄存器对形态的完整指令级语义由 `Zacas_test_plan.md` 覆盖） |
| `norm:Zacas_amocas_w_permission` | HZACAS-06、HZACAS-07、HZACAS-09、HZACAS-10、HZACAS-11、HZACAS-12、HZABHA-33、HZABHA-34（amocas.b/h，条件依赖 Zacas） |
| `zacas_failed_cas_write_note` | HZACAS-07、HZACAS-12、HZACAS-32、HZACAS-33（SPEC 未标注 norm；HZACAS-32/33 为记录型，标注 Zacas SPEC 未明文规定 UNSPECIFIED 的空白点） |
| `norm:Zacas_amocas_rs1_addr_alignment` | HZACAS-23、HZACAS-24、HZACAS-25、HZACAS-26 |
| `norm:Zacas_amocas_mem_op_success_aq_rl` | HZACAS-18、HZACAS-27（aq/rl 位在 htinst 中的保留与 FIOM 交互层面的数据语义验证；acquire/release 排序公理本身待多 hart 就绪，见 Group 3 NOTE） |
| `norm:Zacas_amocas_mem_op_fail_aq_rl` | HZACAS-18、HZACAS-27（同上；“失败路径无 release 语义，无论 rl”的跨 hart 排序验证待多 hart 就绪） |
| `norm:Zabha_rd_sign_extension` | HZABHA-01、HZABHA-04（rd 符号扩展 8/16 位与 rs2 高位忽略在 HS/VS-mode 一致） |
| `norm:Zabha_amocas-BH_ignore_bits` | HZABHA-32、HZABHA-35（amocas.b/h 忽略 rd 高位比较，条件依赖 Zacas） |
| `norm:Zabha_rs1_align_addr` | HZABHA-18、HZABHA-22、HZABHA-23、HZABHA-24、HZABHA-25、HZABHA-26、HZABHA-36（字节恒对齐、半字未对齐与 MAG 放宽） |
| `zabha_no_byte_halfword_lrsc` | HZABHA-37（保留字节/半字 lr/sc 编码；SPEC 未标注 norm） |
| `norm:zalasr_atomic_ordered` | HZLASR-01、HZLASR-02、HZLASR-04、HZLASR-06（Zalasr 为独立原子加载/存储，是 load/store 异常归类二分的功能基础） |
| `norm:ldaq_atomic_load_op` | HZLASR-01、HZLASR-04、HZLASR-06、HZLASR-10、HZLASR-31、HZLASR-33（load-acquire 纯读、仅需读权限、不置数据页 D 位） |
| `norm:sdrl_atomic_store_op` | HZLASR-01、HZLASR-04、HZLASR-08、HZLASR-10、HZLASR-32、HZLASR-33（store-release 纯写、需写权限、恒写使 D 位强制） |
| `norm:ldaq_atomic_load_enc` | HZLASR-18、HZLASR-20（htinst transformed 保留 funct5=00110/aq=1/rs2=0，走 opcode 0x2F atomic 格式） |
| `norm:sdrl_atomic_store_enc` | HZLASR-19、HZLASR-20（htinst transformed 保留 funct5=00111/rl=1/rd=0，走 opcode 0x2F atomic 格式） |
| `norm:zalasr_signext_rd` / `norm:ldaq_signext_rule` | HZLASR-04（rd 符号扩展在 VS/VU-mode 与 HS-mode 一致） |
| `norm:zalasr_ignore_rs2_upper` | HZLASR-04（store-release 忽略 rs2 高位在虚拟化下不变） |
| `norm:ldaq_aq_required` / `norm:sdrl_rl_required` | HZLASR-20、HZLASR-29（load 恒 aq=1、store 恒 rl=1，使 FIOM 排序修改恒适用） |
| `norm:ldaq_no_aq_reserved` / `norm:sdrl_no_rl_reserved` | HZLASR-34（保留编码在 V=1 报 cause=2 而非 cause=22） |
| `norm:zalasr_natural_align` / `norm:zalasr_misaligned_exception` | HZLASR-25、HZLASR-26、HZLASR-28（未对齐异常路径；load-acquire 期望 load 类、store-release 期望 store/AMO 类） |
| `norm:zalasr_misaligned_pma_relax` / `norm:zalasr_misaligned_single_op` | HZLASR-21、HZLASR-27（MAG 放宽为单一内存操作，Addr. Offset 恒为 0） |
| `norm:ldaq_rv64_only` / `norm:sdrl_rv64_only` | HZLASR-01、HZLASR-04（`ld.aq`/`sd.rl` RV64-only 在 VS/VU-mode 执行；RV32 为条件不触发分支） |
| `norm:ldaq_rcsc_semantics` / `norm:sdrl_rcsc_semantics` | HZLASR-20、HZLASR-29（aq/rl 位在 htinst 中的保留与 FIOM 交互的数据语义验证；RCsc 排序公理属多 hart，由 `Zalasr_test_plan.md` Group 5 覆盖） |
| `norm:zalasr_builds_on_amo` | HZLASR-01~36（门控仅依赖 `ZALASR_SUPPORTED`，不以 A 扩展宏为前置；见 Group 5 NOTE 与关键注意事项 7） |
| `norm:H_cause_virtual_instruction` | HZABHA-37、HZLASR-34（保留编码非 HS-qualified，V=1 报 cause=2 而非 cause=22） |
| `norm:stateen_op` | HZACAS-36（`hstateen0` 仅门控 VS/VU 对状态的访问，不影响 HS-mode 自身） |
| `norm:stateen_illegal_state_access` | HZACAS-36（门控触发条件为"读写受保护状态"；Zacas 无架构状态、无 stateen 位，故清零 `hstateen0` 后 VS/VU-mode 的 amocas 仍必正常执行且绝不报 cause=22/cause=2） |
| `norm:stateen_unimplemented_state_roz` | HZACAS-36（辅助探测：控制未实现状态的位为只读零，反向印证平台未为 Za 系列原子指令分配 stateen 位） |
| `norm:mstateen0_se0_op` | HZACAS-36（前置条件：SE0=1 才能从 HS-mode 读写 `hstateen0`） |
| `norm:hstateen0_SE0_op` | HZACAS-36（清零 `hstateen0` 连带门控 VS-mode 对 `vsstateen0` 的访问，但不影响 amocas） |
| `norm:hlsv_op` | HZLRSC-14、HZLRSC-39、HZLRSC-40、HZAMO-14、HZAMO-30、HZAMO-31、HZACAS-16、HZACAS-34、HZACAS-35、HZABHA-14、HZABHA-38、HZABHA-39、HZLASR-17、HZLASR-35、HZLASR-36 |
| `norm:hlsv_priv` | HZLRSC-14、HZLRSC-39、HZAMO-14、HZAMO-30、HZACAS-16、HZACAS-34、HZABHA-14、HZABHA-38、HZLASR-17、HZLASR-35 |
| `norm:hlsv_virtinst` | HZLRSC-02、HZLRSC-03、HZLRSC-14、HZAMO-02、HZAMO-03、HZAMO-14、HZACAS-02、HZACAS-03、HZACAS-16、HZABHA-02、HZABHA-03、HZABHA-14、HZABHA-32、HZLASR-02、HZLASR-03、HZLASR-17（作为 cause=22 的排除对照） |
| `norm:hstatus_gva_op` | HZLRSC-11、HZLRSC-14、HZLRSC-24、HZLRSC-29、HZAMO-11、HZAMO-14、HZAMO-24、HZACAS-13、HZACAS-16、HZACAS-26、HZABHA-11、HZABHA-14、HZABHA-26、HZABHA-34、HZLASR-14、HZLASR-17 |
| `norm:hstatus_spv_op` | HZLRSC-11、HZLRSC-14、HZLRSC-24、HZLRSC-29、HZWRS-07、HZAMO-11、HZAMO-14、HZAMO-24、HZACAS-13、HZACAS-16、HZACAS-26、HZABHA-11、HZABHA-14、HZABHA-26、HZABHA-34、HZLASR-14、HZLASR-17 |
| `norm:hedeleg_acc` | HZLRSC-05、HZLRSC-09、HZLRSC-10、HZAMO-05、HZAMO-10、HZACAS-05、HZACAS-11、HZABHA-05、HZABHA-10、HZLASR-05、HZLASR-12、HZLASR-13 |
| `norm:hedeleg_op` | HZLRSC-06、HZLRSC-07、HZLRSC-08、HZLRSC-27、HZLRSC-28、HZLRSC-34、HZAMO-06、HZAMO-08、HZAMO-21、HZAMO-27、HZACAS-06、HZACAS-09、HZACAS-23、HZACAS-29、HZABHA-06、HZABHA-08、HZABHA-23、HZABHA-29、HZLASR-06、HZLASR-08、HZLASR-11、HZLASR-25、HZLASR-26、HZLASR-31 |
| `norm:H_vm_gpatrans` | HZLRSC-09、HZLRSC-10、HZLRSC-19、HZLRSC-20、HZLRSC-24、HZAMO-10、HZAMO-18、HZAMO-19、HZAMO-23、HZACAS-11、HZACAS-12、HZACAS-20、HZACAS-21、HZACAS-25、HZABHA-10、HZABHA-19、HZABHA-20、HZABHA-25、HZABHA-34、HZLASR-12、HZLASR-13、HZLASR-22、HZLASR-23 |
| `norm:H_vm_gpapriv` | HZAMO-18、HZAMO-19、HZACAS-20、HZACAS-21、HZABHA-19、HZABHA-20、HZLASR-22、HZLASR-23（load-acquire→cause 21、store-release→cause 23） |
| `norm:H_trap_xtinst_val` | HZLRSC-15、HZLRSC-16、HZLRSC-17、HZLRSC-29、HZAMO-15、HZAMO-16、HZAMO-24、HZACAS-17、HZACAS-18、HZACAS-26、HZABHA-15、HZABHA-16、HZABHA-17、HZABHA-26、HZABHA-35、HZLASR-18、HZLASR-19、HZLASR-20、HZLASR-28 |
| `htinst_transformed_atomic` | HZLRSC-15、HZLRSC-16、HZLRSC-17、HZLRSC-18、HZLRSC-21、HZLRSC-29、HZAMO-15、HZAMO-16、HZAMO-17、HZAMO-20、HZAMO-24、HZACAS-17、HZACAS-18、HZACAS-19、HZACAS-22、HZACAS-26、HZABHA-15、HZABHA-16、HZABHA-17、HZABHA-18、HZABHA-21、HZABHA-26、HZABHA-35、HZLASR-18、HZLASR-19、HZLASR-20、HZLASR-21、HZLASR-24、HZLASR-28 |
| `norm:H_trap_xtinst_guestpage` | HZLRSC-19、HZLRSC-21、HZAMO-18、HZAMO-20、HZACAS-20、HZACAS-22、HZABHA-19、HZABHA-21、HZLASR-22、HZLASR-24 |
| `norm:H_trap_xtinst_guestpage_rw` | HZLRSC-20、HZAMO-19、HZACAS-21、HZABHA-20、HZLASR-23 |
| `norm:htval_trapval` | HZLRSC-12、HZLRSC-13、HZLRSC-24、HZAMO-12、HZAMO-13、HZAMO-23、HZAMO-24、HZACAS-14、HZACAS-15、HZACAS-25、HZACAS-26、HZABHA-12、HZABHA-13、HZABHA-25、HZABHA-26、HZABHA-34、HZLASR-15、HZLASR-16、HZLASR-28 |
| `norm:henvcfg_fiom_order` | HZLRSC-30、HZLRSC-31、HZAMO-25、HZAMO-26、HZACAS-27、HZACAS-28、HZABHA-27、HZABHA-28、HZLASR-29、HZLASR-30（排序效果断言待多 hart 就绪；Zalasr 恒带 aq/rl 故恒适用） |
| `norm:henvcfg_adue_op` | HZLRSC-20、HZLRSC-32、HZLRSC-33、HZLRSC-34、HZAMO-19、HZAMO-27、HZAMO-28、HZAMO-29、HZACAS-21、HZACAS-29、HZACAS-30、HZACAS-31、HZACAS-32、HZACAS-33、HZABHA-20、HZABHA-29、HZABHA-30、HZABHA-31、HZLASR-23、HZLASR-31、HZLASR-32、HZLASR-33 |
| `norm:H_virtinst_xtval` | HZWRS-07 |
| `norm:Zawrs_exec_resume_rules` | HZWRS-01 ~ HZWRS-03、HZWRS-06（HZWRS-06 为记录型：两种合法行为均接受） |
| `norm:Zawrs_virtual_instr_excp` | HZWRS-04、HZWRS-05、HZWRS-06、HZWRS-07 |
| `norm:Zawrs_priv_illegal_instr_excp` | HZWRS-08、HZWRS-09、HZWRS-12 |
| `norm:Zawrs_stall_terminate` | —（实现允许的提前终止停顿行为，作为 Group 6 NOTE 与关键注意事项 8 的失败处置依据，无直接用例） |

未被覆盖/不可测规范点说明：本方案声明的规范点均有对应用例，唯 `norm:Zawrs_stall_terminate` 为实现许可行为、无直接用例（作为时序失败处置依据）。其中 HZLRSC-30/31 的 `norm:henvcfg_fiom_order` 排序效果与 HZLRSC-40 属多 hart 内存序属性，需公共框架补充 secondary hart 支持后方可断言，就绪前相关断言 SKIP（非 FAIL），单 hart 仅验证指令可执行性与数据语义；HZLRSC-32/33（`norm:sc_failed_side_effects` × `norm:henvcfg_adue_op`）、HZLRSC-37（`norm:unconstrained_lrsc_no_progress`）、HZLRSC-38（`norm:sc_reservation_invalidate` 在 trap 往返后的存续）、HZLRSC-39/40（`norm:hlsv_op`/`norm:hlsv_priv` 的架构边界）、HZWRS-06（`norm:vtw_virtinstr` 许可的两种行为）为记录型用例，对应 SPEC 明文 UNSPECIFIED、实现自定义或许可行为，不做强制判定；HZLRSC-27/28 的 address-misaligned 与 access-fault 为两类均合规的结果，用例验证 cause 属预期集合并记录实际值；HZLRSC-19/20 的伪指令取值依赖平台实际发生 VS-stage 隐式遍历故障与 A/D 自动更新（`norm:henvcfg_adue_op` 以 Svadu 实现为前提），不具备该映射构造能力时相应用例 SKIP；HZLRSC-35 依赖 VS-stage/G-stage 页表内存满足 `ptmem_rsrv_eventual`，该前置由平台配置声明；`norm:hstatus_vtw_op`/`norm:vtw_virtinstr` 为对照引用，其用例覆盖归 `Hypervisor_CSR_test_plan.md`（HSTAT-04/06），本方案不设独立用例。Group 2（Zaamo）中：HZAMO-22/23 依赖平台声明 MAG（`norm:misaligned_atomicity_granule_size`），未声明时 SKIP；HZAMO-29 依赖平台实现 Svadu（`henvcfg.ADUE` 硬件 A/D 更新），未实现时 SKIP；HZAMO-25/26 的 FIOM 排序效果与 HZAMO-31 属多 hart 内存序属性，就绪前 SKIP；HZAMO-30/31（`norm:hlsv_op`/`norm:hlsv_priv` 的架构边界）为记录型用例，不做强制判定；HZAMO-21/24 的 address-misaligned 与 access-fault 为两类均合规的结果，用例验证 cause 属预期集合并记录实际值。Group 3（Zacas）中：HZACAS-24/25 依赖平台声明 MAG（经 `norm:Zacas_amocas_rs1_addr_alignment` 跨引用 `norm:misaligned_atomicity_granule_size`），未声明时 SKIP；HZACAS-31 依赖平台实现 Svadu（`henvcfg.ADUE` 硬件 A/D 更新），未实现时 SKIP；HZACAS-27/28 的 FIOM 排序效果与 HZACAS-35 属多 hart 内存序属性，就绪前 SKIP；HZACAS-32/33（`zacas_failed_cas_write_note` × `norm:henvcfg_adue_op`）为记录型用例，且与 HZLRSC-32/33 不同——Zacas SPEC 未明文规定失败 CAS 的 PTE D 位副作用为 UNSPECIFIED，此为 SPEC 空白点，仅记录实现行为并显式标注该规范差异，不做强制判定；HZACAS-34/35（`norm:hlsv_op`/`norm:hlsv_priv` 的架构边界）为记录型用例，不做强制判定；HZACAS-23/26 的 address-misaligned 与 access-fault 为两类均合规的结果，用例验证 cause 属预期集合并记录实际值；HZACAS-17~19 须实测确认框架 `hyp_transform_mem_inst()` 对 funct5=00101（amocas）的 golden 值计算正确，不得假定与 amoadd 等共享分支即可直接复用。`norm:Zacas_amocas_mem_op_success_aq_rl`/`norm:Zacas_amocas_mem_op_fail_aq_rl` 的 acquire/release 排序公理（尤其“失败路径无 release 语义，无论 rl”）属多 hart 内存序属性，本方案仅做单 hart 数据语义与 htinst 位保留验证，排序公理待公共框架补充 secondary hart 支持后由未来内存模型测试范围覆盖。Group 4（Zabha）中：HZABHA-24/25/36 依赖平台声明 MAG（经 `norm:Zabha_rs1_align_addr` 跨引用 `norm:misaligned_atomicity_granule_size`），未声明时未对齐半字 AMO 必报 cause=6/7（HZABHA-23）、MAG 分支 SKIP；**字节 AMO 因 1 字节对齐恒成立无未对齐场景（HZABHA-22 为强制正向断言：任意地址正常完成、绝不报 cause=6/7），未对齐/MAG 分支仅半字 AMO 可测**；HZABHA-31 依赖平台实现 Svadu（`henvcfg.ADUE` 硬件 A/D 更新），未实现时 SKIP；HZABHA-27/28 的 FIOM 排序效果与 HZABHA-39 属多 hart 内存序属性，就绪前 SKIP；HZABHA-32~36 的 `amocas.b/h` 为条件用例，依赖 Zacas（`ZACAS_SUPPORTED`），未实现时 SKIP；HZABHA-16/35 须实测确认框架 `hyp_transform_mem_inst()` 对 funct3=000(.b)/001(.h) 的 golden 值计算正确（Zabha 特有的宽度编码保留维度），不得假定与 word/dword AMO 共享分支即宽度自动正确；HZABHA-23/26/36 的 address-misaligned 与 access-fault 为两类均合规的结果，用例验证 cause 属预期集合并记录实际值；HZABHA-38/39（`norm:hlsv_op`/`norm:hlsv_priv` 的架构边界）为记录型用例，不做强制判定；HZABHA-37（保留字节/半字 lr/sc 编码报 cause=2 而非 cause=22，依据 `norm:H_cause_virtual_instruction`）为强制负向断言，平台报 cause=22 即违反 SPEC。Group 5（Zalasr）中：**load-acquire 的 cause 归类为观测记录型**——zalasr.adoc 未明文规定 load-acquire/store-release 的异常 cause 归类（编码位于 AMO opcode 0x2F 空间），故对 load-acquire 的具体 cause（HZLASR-07/12/22/23a/25/31）采用“记录实际观测值 + 与功能分类（load 类）比对 + 偏差报告 bugs/”策略，不武断强制判定；**store-release 的 cause（HZLASR-08/09/13/22/23b/26/32）因 store 与 AMO 同归 store/AMO 类无歧义，为强制断言**；HZLASR-06/10（load-acquire 到 R=1/W=0 页正常执行）为强制正向断言（独立原子加载必须能读只读内存，违背则报告 bugs/）；HZLASR-27 依赖平台声明 MAG（`norm:zalasr_misaligned_pma_relax`/`norm:zalasr_misaligned_single_op`），未声明时未对齐 load-acquire/store-release 必产生异常（HZLASR-25/26）、HZLASR-27 SKIP；HZLASR-33 依赖平台实现 Svadu（`henvcfg.ADUE` 硬件 A/D 更新），未实现时 SKIP；HZLASR-29/30 的 FIOM 排序效果与 HZLASR-36 属多 hart 内存序属性，就绪前 SKIP；`norm:ldaq_rcsc_semantics`/`norm:sdrl_rcsc_semantics` 的 acquire/release RCsc 排序公理属多 hart 内存序属性，本方案仅做单 hart 数据语义与 htinst 位保留验证，排序公理由 `Zalasr_test_plan.md` Group 5 覆盖；HZLASR-18~21 须实测确认框架 `hyp_transform_mem_inst()` 对 funct5=00110(load-acquire)/00111(store-release) 的 golden 值计算正确（统一走 opcode 0x2F transformed atomic 格式，非 transformedload/storeinst），不得假定复用即可；HZLASR-25/28 的 address-misaligned 与 access-fault 为两类均合规的结果，用例验证 cause 属预期集合并记录实际值；HZLASR-35/36（`norm:hlsv_op`/`norm:hlsv_priv` 的架构边界，HLV/HSV 可复制数据传输但丢失原子性与 RCsc 排序）为记录型用例，不做强制判定；HZLASR-34（保留 load-acquire/store-release 编码报 cause=2 而非 cause=22，依据 `norm:H_cause_virtual_instruction`）为强制负向断言，平台报 cause=22 即违反 SPEC；`ld.aq`/`sd.rl`（funct3=011）为 RV64-only（`norm:ldaq_rv64_only`/`norm:sdrl_rv64_only`），RV32 平台不适用 `.d` 分支；Zalasr 门控仅依赖 `ZALASR_SUPPORTED`，不以 A 扩展宏为前置（`norm:zalasr_builds_on_amo`）。
