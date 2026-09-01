**中文 | [English](../testplan_en/Hypervisor_Zi_test_plan_en.md)**

# Hypervisor 与 Z* 扩展交叉测试计划

> 本文档描述 Hypervisor（H）扩展与其他 Z* 系列扩展（含 Zk* 密码学扩展）在交叉场景下的测试计划。本方案从 `Hypervisor_cross_test_plan.md` 拆分而来，仅保留 Hypervisor 与 Z* 扩展交叉的内容。这些测试场景原本在各扩展的独立测试计划中被标记为"由 Hypervisor 测试计划覆盖"或"因缺少 H 扩展而排除"，但经分析发现现有 Hypervisor 测试计划并未完全覆盖。后续补充了 Hypervisor × Zawrs 交叉场景（Group 4）、Hypervisor × V 向量族交叉场景（Group 5）、Hypervisor × Zicntr 交叉场景（Group 6）与 Hypervisor × Zihpm 交叉场景（Group 7）。
>
> 生成时间：2026-06-22

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — Hypervisor（H）扩展：VS/VU-mode 访问受控 CSR 的 virtual-instruction 异常机制
- `SPEC/riscv-isa-manual/src/unpriv/zk.adoc` — Zkr 熵源扩展：`seed` CSR、`mseccfg.SSEED/USEED` 访问控制
- `SPEC/riscv-isa-manual/src/unpriv/zihintntl.adoc` — Zihintntl 扩展：NTL HINT 指令的无架构副作用语义与 trap 行为
- `SPEC/riscv-isa-manual/src/unpriv/zcmt.adoc` — Zcmt 扩展：cm.jt/cm.jalt 表跳转指令、jvt CSR 与两次隐式取指语义
- `SPEC/riscv-isa-manual/src/unpriv/zawrs.adoc` — Zawrs 扩展：wrs.nto/wrs.sto 等待保留集指令、hstatus.VTW 门控的 virtual-instruction 机制
- `SPEC/riscv-isa-manual/src/unpriv/vector-common.adoc` — V 向量族公共定义：vsstatus.vs 向量上下文状态字段、V=1 时向量指令/向量 CSR 的 Off 门控与双 Dirty 更新、向量浮点的 vsstatus.fs 交互
- `SPEC/riscv-isa-manual/src/unpriv/zicntr.adoc` — Zicntr 扩展：cycle/time/instret 基础计数器、`rdtime` 语义与 htimedelta 时间偏移的交集、`scounteren` 在 V=1 时对 VU-mode 的持续控制（无对应 VS CSR）
- `SPEC/riscv-isa-manual/src/unpriv/zihpm.adoc` — Zihpm 扩展：hpmcounter3–31 未实现计数器的访问行为（illegal-instruction 或常数值）在 V=1 场景下的门控交互

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内上述路径文件）

---

## 范围

### 覆盖的扩展交叉

- **Hypervisor × Zkr**：`mseccfg.SSEED` 对 VS/VU-mode 访问 `seed` CSR 的控制、VS/VU-mode 下 virtual-instruction 与 illegal-instruction 异常类型区分、HS-mode 访问 seed 的 SSEED 控制、只读访问异常优先于 virtual-instruction
- **Hypervisor × Zihintntl**：HS/VS/VU-mode 下 NTL HINT 的正常执行（不得误触发 virtual-instruction exception）、NTL 作用于 H 扩展虚拟机访存指令（HLV/HSV/HLVX）、VS-mode 下 NTL + CMO 的 virtual-instruction 报告、VS-mode 下 NTL + target 的 G-stage guest-page-fault 报告
- **Hypervisor × Zcmt**：HS/VS/VU-mode 下表跳转指令（cm.jt/cm.jalt）的正常执行、VS/VU-mode 下 jvt CSR 的访问与 Smstateen（JVT 位）门控、VS-mode 下 JVT 表项取指的两阶段翻译与 G-stage guest instruction page fault 报告
- **Hypervisor × Zawrs**：HS/VS/VU-mode 下 wrs.nto/wrs.sto 的正常执行（不得误触发 virtual-instruction exception）、VS/VU-mode 下 `hstatus.VTW` 对 `wrs.nto` 的 virtual-instruction 门控、`mstatus.TW` 优先于 `hstatus.VTW` 的异常类型判定、VTW 条款仅作用于 `wrs.nto` 而不作用于 `wrs.sto`
- **Hypervisor × V 向量族**：实现 H 扩展时 `vsstatus.vs` 字段（bits[10:9]）的存在性与读写、V=1 时 `vsstatus.vs`/`mstatus.vs` 任一为 Off 对向量指令与向量 CSR 的 illegal-instruction 门控、修改向量状态使两者同时置 Dirty、`vsstatus.sd` 与 `vsstatus.vs` 的联动、向量浮点指令的 `vsstatus.fs` 门控与双 Dirty 更新、`misa.v` 可写时 `vsstatus.vs` 的存在性（适用 V、Zve*、Zv* 全部共享 `vector-common.adoc` 的向量扩展）
- **Hypervisor × Zicntr**：`rdtime` 指令在 VS/VU-mode 下返回 `time + htimedelta`（指令级 delta 语义）、`hcounteren` CY/TM/IR 位对 `cycle`/`time`/`instret` 的门控（作为前置条件与异常类型区分）、`scounteren` 无对应 VS CSR 在 V=1 时继续控制 VU-mode 对基础计数器的可见性（含 `hcounteren`=0 时的被遮断分支）
- **Hypervisor × Zihpm**：未实现 `hpmcounter3–31` 在 V=1 下的访问行为（常数值或异常两种均合法）与 `hcounteren` HPMn 门控的交互、VU-mode 访问 `hpmcounter` 的 `mcounteren → hcounteren → scounteren` 三层门控链

### 不在本文档范围

- 已由 `Hypervisor_CSR_test_plan.md`、`Hypervisor_Interrupts_test_plan.md`、`Hypervisor_Exceptions_test_plan.md`、`Hypervisor_2_stage_test_plan.md`、`Hypervisor_gstage_test_plan.md` 覆盖的 Hypervisor 基础功能
- 各扩展在非 Hypervisor 场景下的行为（由各自独立测试计划覆盖）
- Hypervisor 与 Ss\*/Sv\*/Sm\* 扩展的交叉测试（分别由 `Hypervisor_Ss_test_plan.md`、`Hypervisor_Sv_test_plan.md`、`Hypervisor_Sm_test_plan.md` 覆盖）
- Zkr 非 Hypervisor 场景（M/S/U-mode 访问 seed 的基础控制）— 由 `Zkr_test_plan.md` 覆盖
- Zihintntl 非 Hypervisor 场景（M/S/U-mode 基础语义、编码、压缩变体、CMO 交互、LR/SC 前进保证等）— 由 `zihintntl_test_plan.md` 覆盖
- Zcmt 非 Hypervisor 场景（jvt WARL 行为、编码与操作语义、PMP/页表故障处理、表更新可见性与字节序等）— 由 `zcmt_test_plan.md` 覆盖
- Zawrs 非 Hypervisor 场景（指令编码与可用性、停顿与恢复语义、mstatus.TW 超时 illegal-instruction 行为等）— 由 `Zawrs_test_plan.md` 覆盖
- V 向量族非 Hypervisor 场景（vtype/vl 行为、向量指令基础语义、mstatus.vs 非虚拟化门控等）— 当前无独立测试方案，不在本文档范围；`hypervisor.adoc` 中与 `vector-common.adoc` 等价的 `norm:vsstatus_vs_op`/`norm:vsstatus_fs_op` 已由 `Hypervisor_CSR_test_plan.md`（VSST-03~09）覆盖，本方案不重复
- Zicntr 非 Hypervisor 场景（cycle/time/instret 计数器语义、M/S/U-mode counteren 控制矩阵）— 由 `Zicntr_test_plan.md`、`Sm_CSR_test_plan.md`、`Ss_CSR_test_plan.md` 覆盖
- Zihpm 非 Hypervisor 场景（hpmcounter 语义与事件配置、M/S/U-mode counteren 控制矩阵）— 由 `Zihpm_test_plan.md`、`Sm_CSR_test_plan.md`、`Ss_CSR_test_plan.md` 覆盖
- `hcounteren` 位级可写性与 VS/VU-mode 计数器访问的写回/门控矩阵 — 由 `Shcounterenw_test_plan.md` 与 `Hypervisor_Ss_test_plan.md` Group 3（Hypervisor × Sscounterenw）覆盖；本文档（Group 6/7）仅补充其未覆盖的指令级与未实现计数器行为用例，不重复验证门控矩阵本身

---

## 覆盖的规范点

下表列出本方案覆盖的规范点。带 `norm:` 前缀的为 SPEC 官方标签；不带前缀的为根据 SPEC 原文自行拆解的规范点。

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:mseccfg_sseed_VSorVU-mode_op` | `zk.adoc` | When the H extension is also implemented, access to the seed CSR from an HS-qualified instruction leads to a virtual-instruction exception in VS and VU modes; all other types of accesses raise an illegal-instruction exception. | 实现 H 扩展时，VS/VU 模式下 HS 限定指令访问 seed 引发虚拟指令异常；其他访问类型引发非法指令异常。 |
| `norm:mseccfg_sseed_SorHS-mode_op` | `zk.adoc` | When SSEED is 0, access to the seed CSR from S-/HS-mode raises an illegal-instruction exception. When SSEED is 1, read-write access to the seed CSR from S-/HS-mode is allowed; all other types of accesses raise an illegal-instruction exception. | SSEED=0 时 S/HS 模式访问 seed 引发非法指令异常；SSEED=1 时允许读写访问，其他访问类型仍引发非法指令异常。 |
| `norm:mseccfg_sseed_useed_op_tbl` | `zk.adoc` | Entropy Source Access Control table: M always available; U controlled by USEED; S/HS controlled by SSEED; VS/VU controlled by SSEED with virtual-instruction exception for HS-qualified read-write. | 熵源访问控制表：M 模式始终可用；U 模式由 USEED 控制；S/HS 由 SSEED 控制；VS/VU 由 SSEED 控制且 HS 限定的读写引发虚拟指令异常。 |
| `norm:seed_ro_illegal` | `zk.adoc` | Attempts to access the seed CSR using a read-only CSR-access instruction (csrrs/csrrc with rs1=x0 or csrrsi/csrrci with uimm=0) raise an illegal-instruction exception; any other CSR-access instruction may be used to access seed. | 使用只读 CSR 访问指令访问 seed 引发非法指令异常；其他 CSR 访问指令可用于访问 seed。 |
| `norm:NTL_target_definition` | `zihintntl.adoc` | The insn:ntl[] instructions do not change architectural state, nor do they alter the architecturally visible effects of the target instruction. | NTL 指令不改变架构状态，也不改变 target 指令的架构可见效果（虚拟化环境下同样适用）。 |
| `norm:NTL_range` | `zihintntl.adoc` | The insn:ntl[] instructions affect all memory-access instructions except the cache-management instructions in the ext:zicbom[] extension. | NTL 指令影响所有内存访问指令（含 H 扩展的 HLV/HSV/HLVX 虚拟机访存指令），Zicbom 的 cache-management 指令除外。 |
| `norm:cm-jt_op` | `zcmt.adoc` | cm.jt reads an entry from the jump vector table in memory and jumps to the address that was read. | cm.jt 从内存中的跳转向量表读取一个表项并跳转到读到的地址。 |
| `norm:cm-jalt_op` | `zcmt.adoc` | cm.jalt reads an entry from the jump vector table in memory and jumps to the address that was read, linking to _ra_. | cm.jalt 从内存中的跳转向量表读取一个表项并跳转到读到的地址，同时将返回地址链接到 ra。 |
| `norm:jvt_base_vm` | `zcmt.adoc` | jvt[base] is a virtual address, whenever virtual memory is enabled. | 虚拟内存启用时（含 VS-mode 的 vsatp 翻译），jvt.base 是虚拟地址。 |
| `norm:Zcmt_fetch` | `zcmt.adoc` | ... the execution of a table jump instruction involves two instruction fetches, the first to read the instruction (cm.jt/cm.jalt) and the second to read from the jump vector table (JVT). Both instruction fetches are _implicit_ reads, and both require execute permission; read permission is irrelevant. | 表跳转涉及两次指令取指：第一次取指令本身，第二次取 JVT 表项；两次均为隐式读且都要求执行权限，读权限无关。 |
| `norm:Zcmt_trap` | `zcmt.adoc` | If an exception occurs on either instruction fetch, xEPC is set to the PC of the table jump instruction, xCAUSE is set as expected for the type of fault and xTVAL (if not set to zero) contains the fetch address which caused the fault. | 任一次取指发生异常时，xEPC 设为表跳转指令的 PC，xCAUSE 按故障类型设置，xTVAL（若实现写非零）为引发故障的取指地址。 |
| `norm:Zawrs_exec_resume_rules` | `zawrs.adoc` | The wrs.nto and wrs.sto instructions follow the rules of the wfi instruction for resuming execution on a locally enabled pending interrupt. | wrs.nto/wrs.sto 遵循 wfi 指令关于本地使能的 pending 中断恢复执行的规则（VS/VU-mode 下存在本地使能中断时不停顿、不触发 VTW 异常）。 |
| `norm:Zawrs_virtual_instr_excp` | `zawrs.adoc` | When executing in VS- or VU-mode, if the vtw bit is set in hstatus, the tw bit in mstatus is clear, and the wrs.nto does not complete within an implementation-specific bounded time limit, the wrs.nto instruction will cause a virtual-instruction exception. | 在 VS/VU-mode 下，若 hstatus.VTW=1、mstatus.TW=0，且 wrs.nto 未在实现特定有限时间内完成，将引发虚拟指令异常。 |
| `norm:Zawrs_priv_illegal_instr_excp` | `zawrs.adoc` | When the tw (timeout wait) bit in mstatus is set and wrs.nto is executed in any privilege mode other than M-mode, and it does not complete within an implementation-specific bounded time limit, the wrs.nto instruction will cause an illegal-instruction exception. | mstatus.TW=1 时，非 M-mode（含 VS/VU-mode）执行 wrs.nto 若未在实现限定时间内完成，引发非法指令异常（TW 条款优先于 VTW）。 |
| `norm:Zawrs_stall_terminate` | `zawrs.adoc` | While stalled, an implementation is permitted to occasionally terminate the stall and complete execution for any reason. | 停顿期间，实现允许以任何原因终止停顿并完成执行（Group 4 时序失败处置的依据）。 |
| `norm:vsstatus_vs_sz_acc` | `vector-common.adoc` | When the hypervisor extension is present, a vector context status field, vs, is added to vsstatus[10:9]. It is defined analogously to the floating-point context status field, fs. | 实现 H 扩展时，vsstatus 增加向量上下文状态字段 vs（bits[10:9]），定义类比浮点上下文字段 fs。 |
| `norm:vsstatus_vs_mstatus_vs_op_off` | `vector-common.adoc` | When V=1, both vsstatus.vs and mstatus.vs are in effect: attempts to execute any vector instruction, or to access the vector CSRs, raise an illegal-instruction exception when either field is set to Off. | V=1 时 vsstatus.vs 与 mstatus.vs 同时生效；任一为 Off，执行任何向量指令或访问向量 CSR 均引发非法指令异常。 |
| `norm:vsstatus_vs_mstatus_vs_op_active` | `vector-common.adoc` | When V=1 and neither vsstatus.vs nor mstatus.vs is set to Off, executing any instruction that changes vector state, including the vector CSRs, will change both mstatus.vs and vsstatus.vs to Dirty. | V=1 且两者均非 Off，任何改变向量状态（含向量 CSR）的指令将 mstatus.vs 与 vsstatus.vs 同时置为 Dirty。 |
| `norm:hw_mstatus_vs_dirty_update` | `vector-common.adoc` | Implementations may also change mstatus.vs or vsstatus.vs from Initial or Clean to Dirty at any time, even when there is no change in vector state. | 实现允许在任何时刻将 mstatus.vs 或 vsstatus.vs 从 Initial/Clean 改为 Dirty（即使向量状态未变化）。 |
| `norm:vsstatus_sd_op_vs` | `vector-common.adoc` | If vsstatus.vs is Dirty, vsstatus.sd is 1; otherwise, vsstatus.sd is set in accordance with existing specifications. | vsstatus.vs 为 Dirty 时 vsstatus.sd=1；否则 sd 按既有规范（其他上下文字段）计算。 |
| `norm:vsstatus_vs_exists` | `vector-common.adoc` | For implementations with a writable misa.v field, the vsstatus.vs field may exist even if misa.v is clear. | misa.v 可写的实现，misa.v=0 时 vsstatus.vs 字段可以存在（"可以"存在，非强制）。 |
| `norm:vsstatus_mstatus_FS_off_hypervisor_V_fp_ill` | `vector-common.adoc` | If the hypervisor extension is implemented and V=1, the vsstatus.fs field is additionally in effect for vector floating-point instructions. If vsstatus.fs or mstatus.fs is Off then any attempt to execute a vector floating-point instruction will raise an illegal-instruction exception. | 实现 H 扩展且 V=1 时，vsstatus.fs 对向量浮点指令额外生效；vsstatus.fs 或 mstatus.fs 任一为 Off，执行向量浮点指令引发非法指令异常。 |
| `norm:vsstatus_mstatus_FS_dirty_hypervisor_V_fp` | `vector-common.adoc` | Any vector floating-point instruction that modifies any floating-point extension state (i.e., floating-point CSRs or f registers) must set both mstatus.fs and vsstatus.fs to Dirty. | 修改浮点状态的向量浮点指令必须将 mstatus.fs 与 vsstatus.fs 同时置为 Dirty。 |
| `norm:zicntr_rdtime_op` | `zicntr.adoc` | The rdtime pseudoinstruction reads the low XLEN bits of the time CSR, which counts wall-clock real time that has passed from an arbitrary start time in the past. | rdtime 读取 time CSR 低 XLEN 位；V=1 时该读回值为 time + htimedelta（结合 `norm:htimedelta_sz_acc_op`）。 |
| `norm:hpm_unimplemented_counter_access` | `zihpm.adoc` | Accessing an unimplemented counter may cause an illegal-instruction exception or may return a constant value. | 访问未实现计数器可引发非法指令异常，也可返回常数值（两种实现均合法，V=1 时同样适用）。 |
| `H_scsrs_nomatch_vu_counter` | `hypervisor.adoc`（`norm:H_scsrs_nomatch` 的计数器特化） | Some standard supervisor CSRs (senvcfg, scounteren, and scontext, possibly others) have no matching VS CSR. These supervisor CSRs continue to have their usual function and accessibility even when V=1, except with VS-mode and VU-mode substituting for HS-mode and U-mode. | `scounteren` 无对应 VS CSR，V=1 时继续以 VS 替代 HS、VU 替代 U 的方式生效，控制 VU-mode 对计数器的可见性。 |
| `hcounteren_gate_v1_counter` | `hypervisor.adoc`（`norm:hcounteren_op` 特化） | When the CY, TM, IR, or HPMn bit in hcounteren is clear, attempts to read the corresponding counter while V=1 will cause a virtual-instruction exception if the same bit in mcounteren is 1. | `hcounteren` 对应位清零且 `mcounteren` 同位为 1 时，V=1 下读对应计数器触发 virtual-instruction 异常；本文档（Group 6/7）仅将其作为未实现计数器/指令级用例的前置门控条件引用。 |
| `norm:stateen0_jvt_op` | `smstateen.adoc` | The JVT bit controls access to the `jvt` CSR provided by the Zcmt extension. | stateen0 的 JVT 位控制对 Zcmt 提供的 jvt CSR 的访问。 |
| `norm:H_virtinst_xtval` | `hypervisor.adoc` | On a virtual-instruction trap, `mtval` or `stval` is written the same as for an illegal-instruction trap. | virtual-instruction 异常的 mtval/stval 写入规则与 illegal-instruction 相同（指令编码或 0）。 |
| `norm:htval_trapval` | `hypervisor.adoc` | htval trap value reporting for guest-page faults (implementation may write zero or the faulting GPA>>2). | guest-page fault 时 htval 的故障值报告（实现允许写零或故障 GPA>>2）。 |

---

## Group 1. Hypervisor × Zkr 交叉测试

**规范依据**：
- `norm:mseccfg_sseed_VSorVU-mode_op`：实现 H 扩展时，VS/VU 模式下 HS 限定指令访问 seed 引发 virtual-instruction exception；其他访问引发 illegal-instruction exception
- `norm:mseccfg_sseed_useed_op_tbl`：VS/VU + SSEED=0 时任何访问引发 illegal-instruction exception；VS/VU + SSEED=1 时读写访问引发 virtual-instruction exception
- `norm:mseccfg_sseed_SorHS-mode_op`：SSEED=0 时 HS-mode 访问 seed 引发 illegal-instruction exception；SSEED=1 时允许读写访问
- `norm:seed_ro_illegal`：只读访问在任何模式下都引发 illegal-instruction exception

**测试职责**：验证 H 扩展下 VS/VU-mode 和 HS-mode 访问 seed CSR 的异常类型区分与访问控制。

### 1.1 HS-mode 访问控制（SSEED）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ZKR-HYP-01 | SSEED=1 HS-mode csrrw 访问 seed 正常 | 设 mseccfg.SSEED=1（H 扩展下 HS-mode），HS-mode 执行 csrrw rd, seed, x0 | 正常返回 seed 值 |
| ZKR-HYP-02 | SSEED=0 HS-mode csrrw 访问 seed 触发异常 | 设 mseccfg.SSEED=0，HS-mode 执行 csrrw rd, seed, x0 | illegal-instruction exception (cause=2) |
| ZKR-HYP-13 | SSEED=1 HS-mode 只读访问触发异常 | 设 mseccfg.SSEED=1，HS-mode 执行 csrrs rd, seed, x0 | illegal-instruction exception (cause=2) |

### 1.2 VS/VU-mode 访问控制（SSEED + H 扩展）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ZKR-HYP-03 | SSEED=0 VS-mode csrrw 访问 seed 触发 illegal | 设 mseccfg.SSEED=0，VS-mode 执行 csrrw rd, seed, x0 | illegal-instruction exception (cause=2) |
| ZKR-HYP-04 | SSEED=1 VS-mode csrrw 访问 seed 触发 virtual-instruction | 设 mseccfg.SSEED=1，VS-mode 执行 csrrw rd, seed, x0 | virtual-instruction exception (cause=22) |
| ZKR-HYP-05 | SSEED=0 VU-mode csrrw 访问 seed 触发 illegal | 设 mseccfg.SSEED=0，VU-mode 执行 csrrw rd, seed, x0 | illegal-instruction exception (cause=2) |
| ZKR-HYP-06 | SSEED=1 VU-mode csrrw 访问 seed 触发 virtual-instruction | 设 mseccfg.SSEED=1，VU-mode 执行 csrrw rd, seed, x0 | virtual-instruction exception (cause=22) |
| ZKR-HYP-07 | SSEED=1 VS-mode 只读访问触发 illegal（非 virtual） | 设 mseccfg.SSEED=1，VS-mode 执行 csrrs rd, seed, x0 | illegal-instruction exception (cause=2)（只读访问条件优先） |
| ZKR-HYP-08 | SSEED=1 VS-mode csrrsi uimm=0 触发 illegal | 设 mseccfg.SSEED=1，VS-mode 执行 csrrsi rd, seed, 0 | illegal-instruction exception (cause=2) |
| ZKR-HYP-09 | SSEED=1 VS-mode csrrs(rs1≠x0) 触发 virtual-instruction | 设 mseccfg.SSEED=1，VS-mode 执行 csrrs rd, seed, t0 (t0≠0) | virtual-instruction exception (cause=22)（HS 限定的读写） |
| ZKR-HYP-10 | SSEED=0 VS-mode csrrs(rs1≠x0) 触发 illegal | 设 mseccfg.SSEED=0，VS-mode 执行 csrrs rd, seed, t0 | illegal-instruction exception (cause=2) |
| ZKR-HYP-11 | SSEED 不影响 M-mode（VS/VU 场景下） | 设 mseccfg.SSEED=0，M-mode csrrw seed | 正常访问 |
| ZKR-HYP-14 | SSEED=1 VU-mode 只读访问触发 illegal（非 virtual） | 设 mseccfg.SSEED=1，VU-mode 执行 csrrs rd, seed, x0 | illegal-instruction exception (cause=2)（只读访问条件优先） |
| ZKR-HYP-15 | SSEED=1 VU-mode csrrsi uimm=0 触发 illegal | 设 mseccfg.SSEED=1，VU-mode 执行 csrrsi rd, seed, 0 | illegal-instruction exception (cause=2) |
| ZKR-HYP-16 | SSEED=1 VU-mode csrrs(rs1≠x0) 触发 virtual-instruction | 设 mseccfg.SSEED=1，VU-mode 执行 csrrs rd, seed, t0 (t0≠0) | virtual-instruction exception (cause=22)（HS 限定的读写） |
| ZKR-HYP-17 | SSEED=0 VU-mode csrrs(rs1≠x0) 触发 illegal | 设 mseccfg.SSEED=0，VU-mode 执行 csrrs rd, seed, t0 | illegal-instruction exception (cause=2) |
| ZKR-HYP-18 | SSEED=1 VS-mode csrrci uimm=0 触发 illegal | 设 mseccfg.SSEED=1，VS-mode 执行 csrrci rd, seed, 0 | illegal-instruction exception (cause=2) |

### 1.3 异常优先级与组合场景

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ZKR-HYP-12 | VU-mode 只读异常优先于 virtual-instruction | SSEED=1，VU-mode 执行 csrrs rd, seed, x0 | illegal-instruction exception (cause=2)（只读条件 → illegal，非 virtual-instruction） |

> [!NOTE]
> - 本组测试验证 Zkr 扩展在 Hypervisor 场景下的行为。所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展的可用性，不可用时 TEST_SKIP。
> - ZKR-HYP-03~18 从 `Zkr_test_plan.md` 迁移而来，专门针对依赖 H 扩展的用例。核心语义：VS/VU-mode 访问 seed 时，`mseccfg.SSEED` 决定是否放行，且 HS 限定的读写（SSEED=1）触发 **virtual-instruction**（cause=22），而 SSEED=0 或只读访问触发 **illegal-instruction**（cause=2）。
> - **virtual-instruction 与 illegal-instruction 的区分**：测试断言必须使用准确的 cause 常量，区分 SSEED=1 时的 virtual-instruction（cause=22）与只读访问/SSEED=0 时的 illegal-instruction（cause=2）。

---

## Group 2. Hypervisor × Zihintntl 交叉测试

**规范依据**：
- `norm:NTL_target_definition`：NTL 指令不改变架构状态，也不改变 target 指令的架构可见效果；虚拟化环境下 NTL 前缀序列的架构行为必须与非虚拟化时一致
- `norm:NTL_range`：NTL 影响所有内存访问指令，H 扩展的 HLV/HSV/HLVX 虚拟机访存指令亦在作用范围内

**测试职责**：验证 NTL HINT 在虚拟化环境（HS/VS/VU-mode）下的行为与非虚拟化场景一致：正常执行、不误触发 virtual-instruction exception，且 trap 报告信息（cause/epc/tval/GVA/htval）与不带前缀时完全一致。本组用例从 `zihintntl_test_plan.md` 迁移而来（原 NTL-12 虚拟化部分、NTL-RG-07、NTL-CMO-08、NTL-TRAP-08）。

### 2.1 HS/VS/VU-mode 下 NTL 执行

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| NTL-HYP-01 | HS-mode 执行 NTL + load | HS-mode 执行 ntl.all + ld，比对目标寄存器与内存 | 正常执行，无异常，结果与不带前缀一致 |
| NTL-HYP-02 | VS-mode 执行 NTL + load | 切入 VS-mode 执行 ntl.all + ld | 正常执行，无 virtual-instruction exception，结果与不带前缀一致 |
| NTL-HYP-03 | VU-mode 执行 NTL + load | 切入 VU-mode 执行 ntl.all + ld | 正常执行，无异常，结果与不带前缀一致 |

### 2.2 NTL 作用于 H 扩展虚拟机访存指令

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| NTL-HYP-04 | NTL 作用于 HLV/HSV/HLVX | HS-mode 以 ntl.all 前缀执行 HLV/HSV/HLVX，对照不带前缀的相同序列 | 正常执行，访存效果正确，两者架构行为一致 |

### 2.3 VS-mode 下 NTL + CMO 的 trap 报告

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| NTL-HYP-05 | ntl 前缀不改变 CMO 指令的 virtual-instruction 报告 | 若实现 Zicbom：VS-mode 下 henvcfg.CBIE=0 且 CBCFE=0 时执行「ntl.all + cbo.inval/cbo.clean」，对照不带前缀的相同序列 | 与不带前缀一样触发 virtual-instruction exception (cause=22)，cause/epc 指向 CMO 指令而非 NTL |

### 2.4 VS-mode 下 NTL + target 的 G-stage 缺页

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| NTL-HYP-06 | VS-mode 下 NTL + target 的 G-stage 缺页 | VS-mode 执行「ntl.p1 + ld」触发 G-stage guest-page-fault，对照不带前缀的相同序列 | 递送到 HS-mode，load guest-page fault (cause=21)，hstatus.GVA=1，stval/htval 正确，行为与不带前缀一致 |

> [!NOTE]
> - 本组所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展的可用性，不可用时 TEST_SKIP。
> - Zihintntl 无独立 misa/CSR 探测标志，按平台配置声明启用；NTL 指令以 raw encoding（.word 0x00200033/0x00300033/0x00400033/0x00500033）注入执行流，避免工具链别名干扰。
> - NTL-HYP-05 另需探测 Zicbom；NTL-HYP-06 需构造 VS-stage 有效、G-stage 无效的映射。核心断言策略沿用 `zihintntl_test_plan.md` 的「HINT 无副作用对照法」：带/不带 NTL 前缀的架构可见行为（含异常信息）必须完全一致。

---

## Group 3. Hypervisor × Zcmt 交叉测试

**规范依据**：
- `norm:cm-jt_op` / `norm:cm-jalt_op`：表跳转为普通指令，无特权级限制，HS/VS/VU-mode 下均应正常执行
- `norm:jvt_base_vm`：虚拟内存启用时 jvt.base 为虚拟地址，VS-mode 下经 vsatp 两阶段翻译
- `norm:Zcmt_fetch` / `norm:Zcmt_trap`：第二次取指（JVT 表项）同样经过翻译，故障时 xEPC 指向表跳转指令、xTVAL 为故障取指地址
- `norm:stateen0_jvt_op`：stateen0 的 JVT 位控制 jvt CSR 访问，仅门控 CSR 访问、不门控指令执行
- `norm:htval_trapval`：G-stage 故障时 htval 的故障值报告规则

**测试职责**：验证表跳转指令与 jvt CSR 在虚拟化环境下的行为：HS/VS/VU-mode 正常执行不误触发 virtual-instruction exception；VS-mode 下 JVT 表项取指经两阶段翻译，G-stage 故障按 guest instruction page fault 报告；hstateen0.JVT 门控 VS/VU 的 jvt 访问但不门控指令执行。本组用例从 `zcmt_test_plan.md` 迁移而来（原 ZCMT-27/28、ZACC-03/04/05 虚拟化部分、ZACC-06 虚拟化部分；HZCMT-08 为对照 ZCMT-25 补充的 VS-stage 故障用例）。

### 3.1 HS/VS/VU-mode 表跳转执行

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZCMT-01 | HS-mode 执行表跳转 | HS-mode 执行 cm.jt 与 cm.jalt（jvt 指向有效表） | 均正常跳转与链接，无异常 |
| HZCMT-02 | VS-mode 执行表跳转 | VS-mode 执行 cm.jt 与 cm.jalt | 均正常执行，无 virtual-instruction exception |
| HZCMT-03 | VU-mode 执行表跳转 | VU-mode 执行 cm.jt 与 cm.jalt | 均正常执行，无异常 |

### 3.2 VS/VU-mode jvt 访问与 stateen 门控

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZCMT-04 | VS/VU-mode 访问 jvt | stateen 使能状态下，VS/VU-mode csrr/csrw jvt | 访问正常（jvt 权限 URW + stateen 使能） |
| HZCMT-05 | hstateen0.JVT 门控 VS/VU 访问 | 实现 Smstateen 时按层级清零 hstateen0/sstateen0 的 JVT 位，VS/VU 访问 jvt | VS/VU 触发 virtual-instruction/illegal-instruction（详细用例见 `Smstateen_test_plan.md` / `Ssstateen_test_plan.md`） |
| HZCMT-06 | stateen 不门控表跳转指令执行 | 实现 Smstateen 时清零各级 stateen 的 JVT 位，VS-mode 执行 cm.jt/cm.jalt | 指令正常执行（state enable 仅门控 jvt CSR 访问，不门控指令本身） |

### 3.3 VS-mode 两阶段翻译下的表跳转

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZCMT-07 | VS-mode 翻译路径正常跳转 | VS-mode 启用 vsatp，VS-stage 映射表页 X=1，执行 cm.jt | 跳转成功（VS-stage 翻译生效） |
| HZCMT-08 | VS-mode 表页 VS-stage X=0 触发故障 | 表所在页 VS-stage 映射但 X=0，VS-mode 执行 cm.jt | instruction page fault 按委托路径递送，sepc=cm.jt PC，stval=表项虚拟地址 |
| HZCMT-09 | VS-mode 第二次取指 G-stage 故障 | G-stage 表页映射无效，VS-mode 执行 cm.jt | guest instruction page fault (cause=20) 递送至 HS-mode，hstatus.GVA=1，htval=故障表项 GPA>>2（norm:htval_trapval 允许为零） |

> [!NOTE]
> - 本组所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展的可用性，不可用时 TEST_SKIP；Zcmt 支持以平台配置 `ZCMT_SUPPORTED` 宏为准，jvt 可写性探测结果（只读实现，`norm:jvt_op` 允许）决定功能用例是否适用。
> - HZCMT-05 另需探测 Smstateen；HZCMT-07~09 需启用 vsatp（及 hgatp）构造两阶段翻译，VS-stage 有效 + G-stage 无效的映射组合用于隔离 G-stage 故障。
> - HZCMT-08/09 验证第二次取指（JVT 表项）的故障路径：vsepc 必须指向 cm.jt 指令本身而非表地址（`norm:Zcmt_trap`），vstval/htval 报告表项取指地址。

---

## Group 4. Hypervisor × Zawrs 交叉测试

**规范依据**：
- `norm:Zawrs_virtual_instr_excp`：VS/VU-mode 下 `hstatus.VTW`=1、`mstatus.TW`=0 且 `wrs.nto` 未在实现限定时间内完成 → virtual-instruction exception
- `norm:Zawrs_exec_resume_rules`：wrs 指令遵循 `wfi` 的本地使能中断恢复规则；存在本地使能的 pending 中断时不停顿，VTW 异常路径不触发（对照 `Hypervisor_CSR_test_plan.md` 的 `norm:hstatus_vtw_op`/`norm:vtw_virtinstr` WFI 语义；`norm:vtw_virtinstr` 同时允许实现在 VTW=1 时总是触发 virtual-instruction，即使存在被全局屏蔽的 pending 中断，故 HZWRS-06 为记录型用例）
- `zawrs.adoc`（`norm:Zawrs_priv_illegal_instr_excp`）：`mstatus.TW`=1 时非 M-mode 执行 `wrs.nto` 未完成引发 illegal-instruction——TW 条款优先于 VTW，VS/VU-mode 亦按 illegal 报告（与 HSTAT-06 的 WFI 语义一致）
- `zawrs.adoc`：VTW 条款仅点名 `wrs.nto`，`wrs.sto` 由其短超时限定、不受 VTW 门控；wrs 指令在所有特权模式可用，HS-mode 不受 VTW 约束（VTW 仅作用于 V=1）

**测试职责**：验证 Zawrs 指令在虚拟化环境下的行为：HS/VS/VU-mode 正常执行不误触发 virtual-instruction exception；VS/VU-mode 下 `hstatus.VTW` 对 `wrs.nto` 的 virtual-instruction 门控；`mstatus.TW` 与 `hstatus.VTW` 的优先级与异常类型区分；VTW 条款的指令范围（仅 `wrs.nto`）。指令以 raw encoding 注入：`wrs.nto` = 0x00D00073（SYSTEM opcode=0x73，funct3=0，rd=0，funct12=0x0d），`wrs.sto` = 0x01D00073（funct12=0x1d）。

### 4.1 HS/VS/VU-mode wrs 指令正常执行（VTW=0）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZWRS-01 | HS-mode 执行 wrs.nto/wrs.sto | `hstatus.VTW`=0，HS-mode 以 `lr` 建立保留集并前置本地使能的 pending 软中断，依次执行 `wrs.nto` 与 `wrs.sto`（raw encoding） | 均正常完成，无异常 |
| HZWRS-02 | VS-mode 执行 wrs.nto/wrs.sto | `hstatus.VTW`=0、`mstatus.TW`=0，VS-mode 同场景执行两条指令 | 均正常完成，不误触发 virtual-instruction exception |
| HZWRS-03 | VU-mode 执行 wrs.nto/wrs.sto | 同上配置，VU-mode 执行两条指令 | 均正常完成，无异常 |

### 4.2 hstatus.VTW 门控（VS/VU-mode wrs.nto）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZWRS-04 | VS-mode VTW=1 wrs.nto 触发 virtual-instruction | `hstatus.VTW`=1、`mstatus.TW`=0，屏蔽所有本地使能中断，VS-mode 执行 `wrs.nto`（trap-armed） | virtual-instruction exception (cause=22) |
| HZWRS-05 | VU-mode VTW=1 wrs.nto 触发 virtual-instruction | 同配置，VU-mode 执行 `wrs.nto` | virtual-instruction exception (cause=22) |
| HZWRS-06 | VTW=1 但本地使能中断已 pending（记录型） | `hstatus.VTW`=1，置位并本地使能一个中断后，VS-mode 执行 `wrs.nto` | 两种行为均合法：指令立即完成（不停顿，`norm:Zawrs_exec_resume_rules`）；或按 `norm:vtw_virtinstr` 的 VTW 拦截许可报 virtual-instruction (cause=22)。记录实现选择，若触发异常必须为 cause=22，不做强制判定 |
| HZWRS-07 | VTW 异常的 trap 报告 | 承接 HZWRS-04 场景，检查 HS-mode trap 现场 | cause=22，stval 按 `norm:H_virtinst_xtval` 规则写入（指令编码或 0），hstatus.SPV=1，handler 跳过后正常恢复 |

### 4.3 TW 优先级与指令范围（VS/VU-mode）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZWRS-08 | VTW=1 且 TW=1 时 VS-mode 报 illegal | `hstatus.VTW`=1、`mstatus.TW`=1，VS-mode 执行 `wrs.nto` | illegal-instruction exception (cause=2)（TW 条款优先，`norm:Zawrs_priv_illegal_instr_excp`） |
| HZWRS-09 | VTW=1 且 TW=1 时 VU-mode 报 illegal | 同配置，VU-mode 执行 `wrs.nto` | illegal-instruction exception (cause=2) |
| HZWRS-10 | VTW=1 仅作用于 wrs.nto | `hstatus.VTW`=1、`mstatus.TW`=0，VS-mode 执行 `wrs.sto` | 短超时后正常完成，无异常（VTW 条款仅点名 `wrs.nto`） |
| HZWRS-11 | VTW 不影响 HS-mode | `hstatus.VTW`=1，HS-mode 执行 `wrs.nto` | 正常完成，无异常（VTW 仅作用于 V=1） |
| HZWRS-12 | VS-mode 下 TW=1 单独生效 | `mstatus.TW`=1、`hstatus.VTW`=0，VS-mode 执行 `wrs.nto` | illegal-instruction exception (cause=2)（对照 `Hypervisor_CSR_test_plan.md` HSTAT-06 的 WFI 语义） |

> [!NOTE]
> - 本组所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展，并以 trap-armed raw encoding 探测 Zawrs 支持（未实现时全套 TEST_SKIP）；Zawrs 依赖 Zalrsc，用例须先以 `lr` 建立保留集。
> - 与 WFI/VTW 用例（HSTAT-04）同样存在时序依赖：VTW 异常要求 `wrs.nto` "未在实现限定时间内完成"。若实现按 `norm:Zawrs_stall_terminate` 以极短时长提前终止停顿导致异常不触发，应保持用例失败并记录至 `bugs/` 目录，不得放宽断言。
> - HZWRS-06 为记录型用例：`norm:vtw_virtinstr` 允许实现在 VTW=1 时总是触发 virtual-instruction（即使存在被全局屏蔽的 pending 中断），因此"立即完成"与"报 cause=22"均为合法实现；用例仅记录实现选择并约束异常类型（若触发必须为 cause=22），同时避免依赖实现停顿时长。
> - HZWRS-08/09 验证异常类型判定：`mstatus.TW`=1 时按 `norm:Zawrs_priv_illegal_instr_excp` 报 illegal-instruction (cause=2)，不得报 virtual-instruction；断言必须使用精确 cause 常量。
> - Zawrs 非 Hypervisor 场景（编码与可用性、停顿与恢复、TW 基础行为）由 `Zawrs_test_plan.md` 覆盖。

---

## Group 5. Hypervisor × V 向量族交叉测试

**规范依据**：
- `norm:vsstatus_vs_sz_acc`：实现 H 扩展时 vsstatus 增加向量上下文状态字段 vs（bits[10:9]），定义类比 fs
- `norm:vsstatus_vs_mstatus_vs_op_off`：V=1 时 vsstatus.vs 与 mstatus.vs 同时生效；任一为 Off，执行任何向量指令或访问向量 CSR → illegal-instruction
- `norm:vsstatus_vs_mstatus_vs_op_active`：V=1 且两者均非 Off，任何改变向量状态的指令将两者同时置 Dirty
- `norm:hw_mstatus_vs_dirty_update`：实现允许在任何时刻将 Initial/Clean 提升为 Dirty（许可行为，相关用例为记录型）
- `norm:vsstatus_sd_op_vs`：vsstatus.vs=Dirty → vsstatus.sd=1
- `norm:vsstatus_vs_exists`：misa.v 可写的实现，misa.v=0 时 vsstatus.vs 可以存在（条件用例）
- `norm:vsstatus_mstatus_FS_off_hypervisor_V_fp_ill` / `norm:vsstatus_mstatus_FS_dirty_hypervisor_V_fp`：V=1 时向量浮点指令的 vsstatus.fs 门控与双 Dirty 更新
- 与 `Hypervisor_CSR_test_plan.md` 的分工：`hypervisor.adoc` 的 `norm:vsstatus_vs_op`/`norm:vsstatus_fs_op`（等价表述）已由该方案 VSST-03~09 覆盖；本组仅覆盖 `vector-common.adoc` 特有的规范点（字段存在性、向量 CSR 访问门控、SD 联动、FP 侧双 Dirty 更新），不重复验证等价内容。本组适用所有共享 `vector-common.adoc` 的向量扩展（V、Zve32x/f、Zve64x/f/d、Zv*）

**测试职责**：验证向量上下文状态（vsstatus.vs）与向量浮点状态（vsstatus.fs）的 VS 级副本在 V=1 场景下的行为：字段存在性与读写、Off 门控（指令与向量 CSR）、双 Dirty 更新、SD 联动、向量浮点门控。向量/向量浮点指令以 raw encoding（.word）注入，避免构建 march 对 v 扩展的依赖。

### 5.1 vsstatus.vs 字段与向量指令/CSR 门控（VS/VU）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HVEC-01 | vsstatus.vs 字段读写 | 实现 H 扩展时，HS-mode 写 vsstatus.vs=0b11/0b01/0b00 并读回 | 字段存在且可写（bits[10:9]），读回一致 |
| HVEC-02 | vsstatus.vs=Off 门控向量指令 | `mstatus.vs`≠Off，设 `vsstatus.vs`=Off，VS-mode 执行向量指令（raw encoding） | illegal-instruction exception (cause=2) |
| HVEC-03 | mstatus.vs=Off 门控向量指令 | `vsstatus.vs`≠Off，设 `mstatus.vs`=Off，VS-mode 执行向量指令 | illegal-instruction exception (cause=2) |
| HVEC-04 | Off 门控向量 CSR 访问 | 设 `vsstatus.vs`=Off，VS-mode 访问向量 CSR（vstart/vl/vtype/vcsr） | illegal-instruction exception (cause=2)（Off 门控覆盖向量 CSR 访问） |
| HVEC-05 | 两者均非 Off 时 VS/VU 正常执行 | `vsstatus.vs` 与 `mstatus.vs` 均为 Initial/Dirty，VS-mode 与 VU-mode 分别执行向量指令 | 均正常执行，无异常 |
| HVEC-06 | 修改向量状态使两者同时置 Dirty | 两者均置 Initial (0b01)，VS-mode 执行改变向量状态的指令（含向量 CSR 写） | `mstatus.vs`=3 (Dirty) 且 `vsstatus.vs`=3 (Dirty) |
| HVEC-07 | vsstatus.sd 与 vs 联动 | HS-mode 写 `vsstatus.vs`=Dirty 读 `vsstatus.sd`；再写 `vsstatus.vs`=Initial（其余上下文字段非 Dirty）再读 | vs=Dirty → sd=1；vs=Initial → sd=0（`norm:vsstatus_sd_op_vs`） |
| HVEC-08 | （记录型）实现可随时提升 Clean 为 Dirty | 两者均置 Clean (0b02)，VS-mode 执行向量指令后回读两个字段 | 保持 Clean 与提升为 Dirty 均合法（`norm:hw_mstatus_vs_dirty_update`），记录实现行为，不做强制判定 |

### 5.2 向量浮点门控（vsstatus.fs，VS/VU）

**前置条件**：F 扩展（`misa.f`）与向量浮点指令支持（trap-armed raw encoding 探测），不满足则本小节全套 TEST_SKIP。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HVEC-09 | vsstatus.fs=Off 门控向量浮点指令 | `mstatus.fs`≠Off，设 `vsstatus.fs`=Off，VS-mode 执行向量浮点指令（raw encoding） | illegal-instruction exception (cause=2) |
| HVEC-10 | mstatus.fs=Off 门控向量浮点指令 | `vsstatus.fs`≠Off，设 `mstatus.fs`=Off，VS-mode 执行向量浮点指令 | illegal-instruction exception (cause=2) |
| HVEC-11 | VU-mode 向量浮点门控 | `vsstatus.fs`=Off，VU-mode 执行向量浮点指令 | illegal-instruction exception (cause=2) |
| HVEC-12 | 修改浮点状态使两者同时置 Dirty | 两者均置 Initial，VS-mode 执行修改浮点状态的向量浮点指令 | `mstatus.fs`=3 (Dirty) 且 `vsstatus.fs`=3 (Dirty) |

### 5.3 条件用例（misa.v 可写）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HVEC-13 | （条件）misa.v=0 时 vsstatus.vs 存在性 | 探测 `misa.v` 可写性：不可写则 TEST_SKIP；可写则写 `misa.v`=0，trap-armed 读写 `vsstatus.vs` | 字段"可以存在"也可不存在（`norm:vsstatus_vs_exists`），读写成功与非法指令均为合法实现，记录实现行为 |

> [!NOTE]
> - 扩展探测：V 扩展以 `misa.v` 判定；向量浮点另需 `misa.f` 与向量浮点指令的 trap-armed 探测，不满足时对应小节/用例 TEST_SKIP。
> - 向量/向量浮点指令以 raw encoding（.word）注入（如 vsetivli/vadd.vv/vfadd.vv 的固定编码），避免工具链依赖。
> - Off 门控的异常类型固定为 illegal-instruction (cause=2)：这是扩展上下文状态门控，不属于 Hypervisor 受控访问的 virtual-instruction (cause=22)，断言不得混淆。
> - HVEC-08/HVEC-13 为记录型用例：`norm:hw_mstatus_vs_dirty_update`/`norm:vsstatus_vs_exists` 均为实现许可行为，不做强制判定。
> - 中断环境：VS/VU 用例在零挂起中断环境下执行（与 Group 4 同原则，避免中断递送污染 trap 记录）。
> - V 向量族非虚拟化场景（vtype/vl、基础语义）无独立测试方案，不在本组范围。

---

## Group 6. Hypervisor × Zicntr 交叉测试

**与 Hypervisor 的交集点**：
1. **`time` 读取偏移**：V=1 时 VS/VU-mode 读取 `time`（含 `rdtime` 指令）返回 `time + htimedelta`（`norm:htimedelta_sz_acc_op`）；CSR 级语义已由 `Hypervisor_CSR_test_plan.md` HTDLT-01~05 覆盖，本组补充指令级（raw encoding）路径
2. **htimedelta 实现要求**：若实现 `time` CSR 则必须实现 `htimedelta`（`norm:time_htimedelta_req`，由成功访问隐式验证）
3. **hcounteren CY/TM/IR 门控**：对应位清零且 `mcounteren` 同位为 1 时，V=1 下读 `cycle`/`time`/`instret` 触发 virtual-instruction（`norm:hcounteren_op`）；门控矩阵由 `Shcounterenw_test_plan.md` 覆盖，本组仅将其作为前置条件并验证异常类型区分（HZCNT-05/06）
4. **hcounteren.TM 对 vstimecmp 的门控**：`norm:hcounteren_acc`，由 `Hypervisor_Ss_test_plan.md` HCROSS-SSTC-05 覆盖，本组不重复；vstimecmp/VSTIP 合成（`(time + htimedelta) >= vstimecmp`）由 `Hypervisor_CSR_test_plan.md` Group 7 覆盖
5. **scounteren 对 VU-mode 的持续控制**：`scounteren` 无对应 VS CSR（`norm:H_scsrs_nomatch`），V=1 时继续控制 VU-mode 对 `cycle`/`time`/`instret` 的可见性；`Hypervisor_CSR_test_plan.md` VCSR-17 仅覆盖 `hcounteren`=1 分支，本组补充 `hcounteren`=0 遮断分支与 `scounteren` 不影响 VS-mode 的反向验证（HZCNT-07~09）

**规范依据**：
- `norm:zicntr_rdtime_op`：`rdtime` 读取 `time` CSR 低 XLEN 位；V=1 时该读回值为 `time + htimedelta`，HS-mode 不受 delta 影响
- `H_scsrs_nomatch_vu_counter`：`scounteren` 无对应 VS CSR，V=1 时继续以 VS 替代 HS、VU 替代 U 的方式生效，控制 VU-mode 计数器可见性；VU-mode 计数器访问异常递送到 HS-mode（`norm:htval_trapval` 不适用）
- `hcounteren_gate_v1_counter`：`hcounteren`/`mcounteren` 门控作为本组用例的前置条件；门控矩阵本身由 `Shcounterenw_test_plan.md` 覆盖，不重复验证（例外：HZCNT-06 验证 `mcounteren`=0 时异常类型为 illegal 而非 virtual-instruction，属异常类型区分而非门控矩阵）

**测试职责**：验证 `rdtime` 指令级时间偏移语义与 `scounteren` 在 V=1 时对 VU-mode 的持续控制（`cycle`/`time`/`instret`）。计数器指令以 raw encoding 注入（`rdcycle`=0xC0002xx3、`rdtime`=0xC0102xx3、`rdinstret`=0xC0202xx3，funct3=2 SYSTEM/csrrs rd, csr, x0），避免工具链别名干扰。

### 6.1 rdtime 指令级 htimedelta 语义（VS/VU）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZCNT-01 | VS-mode rdtime 返回 time+delta | 设 `htimedelta`=N（N 为非零可辨识值），VS-mode 执行 `rdtime`（raw encoding），返回后在 HS-mode 读取真实 `time` | `rdtime` 返回值 ≈ 真实 `time` + N（容许时钟推进误差）；与 `csrr time` 路径（HTDLT-02）一致 |
| HZCNT-02 | VU-mode rdtime 返回 time+delta | 同配置，VU-mode 执行 `rdtime` | 返回值 ≈ 真实 `time` + N |
| HZCNT-03 | HS-mode rdtime 不含 delta | `htimedelta`=N，HS-mode 执行 `rdtime` | 返回真实 `time`，不含偏移（对照） |
| HZCNT-04 | 负偏移 rdtime | 设 `htimedelta` 为负值（如 0xFFFFFFFFFFFF0000），VS-mode 执行 `rdtime` | 返回值小于真实 `time`（无符号比较，对应截断语义） |
| HZCNT-05 | hcounteren.TM=0 时 VS-mode rdtime 触发 virtual-instruction | `mcounteren.TM`=1、`hcounteren.TM`=0，VS-mode 执行 `rdtime`（trap-armed） | virtual-instruction exception (cause=22)（`hcounteren_gate_v1_counter`） |
| HZCNT-06 | mcounteren.TM=0 时 VS-mode rdtime 报 illegal | `mcounteren.TM`=0、`hcounteren.TM`=0，VS-mode 执行 `rdtime` | illegal-instruction exception (cause=2)（`mcounteren` 层先决，不得报 cause=22） |

### 6.2 scounteren 在 V=1 时对 VU-mode 的持续控制（cycle）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZCNT-07 | hcounteren=1 时 scounteren 控制 VU 计数器（CY 位） | `mcounteren.CY`=1、`hcounteren.CY`=1，先设 `scounteren.CY`=1 验证 VU 读 `cycle` 成功，再设 `scounteren.CY`=0 重读 | 置 1 时无异常；置 0 时触发异常（`H_scsrs_nomatch_vu_counter`，对照 `Hypervisor_CSR_test_plan.md` VCSR-17） |
| HZCNT-08 | hcounteren=0 时遮断对 VU 生效 | `mcounteren.CY`=1、`hcounteren.CY`=0、`scounteren.CY`=1，VU-mode 读 `cycle` | 触发异常（`hcounteren` 层关闭优先遮断，验证 VCSR-17 未覆盖的分支） |
| HZCNT-09 | scounteren 对 VS-mode 不生效 | `scounteren.CY`=0、`hcounteren.CY`=1、`mcounteren.CY`=1，VS-mode 读 `cycle` | 正常读取（`scounteren` 仅约束 VU，VS-mode 不受其影响） |

> [!NOTE]
> - 本组所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展；Zicntr 支持以 `cycle`/`time` CSR 的 trap-armed 探测为准，不满足时全组 TEST_SKIP。
> - `rdtime`/`rdcycle` 等计数器访问以 raw encoding（csrrs rd, csr, x0 形式）注入，避免工具链对伪指令的展开差异；只读访问形式（rs1=x0）对计数器合法。
> - HZCNT-01/02 的时钟比较需容许真实 `time` 在测试窗口内的自然推进，断言采用差值区间（如 `N <= rdtime - time <= N + bound`），不得要求精确相等。
> - HZCNT-05/06 验证异常类型区分：门控异常必须为 virtual-instruction (cause=22)，`mcounteren` 层异常为 illegal-instruction (cause=2)，断言使用精确 cause 常量。
> - RV32 高半访问（`rdtimeh`/`htimedeltah`，`norm:zicntr_rdtimeh_op`）为条件规范点，RV64 平台不触发，本文档不设用例。
> - 门控矩阵（逐位写回 + VS/VU 组合）已由 `Shcounterenw_test_plan.md` 覆盖，本组仅引用门控作为前置条件，不重复验证。

---

## Group 7. Hypervisor × Zihpm 交叉测试

**与 Hypervisor 的交集点**：
1. **hcounteren HPMn 门控**：`hcounteren` 第 N 位清零且 `mcounteren` 同位为 1 时，V=1 下读 `hpmcounterN` 触发 virtual-instruction（`norm:hcounteren_op`）；门控矩阵由 `Shcounterenw_test_plan.md` 覆盖，本组仅将其作为前置条件（例外：HZHPM-03 验证门控关闭时异常由门控触发、与计数器是否实现无关）
2. **未实现计数器在 V=1 下的行为**：访问未实现 `hpmcounter` 可引发异常或返回常数值（`norm:hpm_unimplemented_counter_access`，两种均合法）；此前无任何测试计划覆盖，为本组新增用例（HZHPM-01/02/04）
3. **VU-mode 三层门控链**：VU-mode 访问 `hpmcounter` 需 `mcounteren[N]`/`hcounteren[N]`/`scounteren[N]` 全为 1，其中 `scounteren` 层的约束来自 `norm:H_scsrs_nomatch`（无对应 VS CSR，V=1 时继续生效）；此前仅 `Hypervisor_CSR_test_plan.md` VCSR-17 覆盖 `cycle` 的 `hcounteren`=1 分支，`hpmcounter` 链未覆盖（HZHPM-05）

**规范依据**：
- `norm:hpm_unimplemented_counter_access`：未实现 `hpmcounter` 的访问可引发非法指令异常或返回常数值（两种实现均合法）；V=1 时该行为仍受 `hcounteren`（及 VU-mode 下的 `scounteren`）门控，门控开启时不得对“未实现计数器本身”强行报 virtual-instruction
- `hcounteren_gate_v1_counter`：HPMn 位门控作为本组用例的前置条件；门控矩阵本身由 `Shcounterenw_test_plan.md` 覆盖，不重复验证（例外：HZHPM-03 验证门控关闭时异常由门控触发、与计数器是否实现无关；`mcounteren[N]` 本身为只读零时 `mcounteren` 层先决，V=1 访问报 illegal-instruction (cause=2) 而非 virtual-instruction，HZHPM-01~03 均验证该分支）
- `H_scsrs_nomatch_vu_counter`：VU-mode 访问 `hpmcounter` 额外受 `scounteren` 约束（无对应 VS CSR，V=1 时继续生效）
- 与既有方案的分工：`hcounteren` 位级可写性与已实现计数器的写回/门控矩阵由 `Shcounterenw_test_plan.md` 与 `Hypervisor_Ss_test_plan.md` Group 3 覆盖，本组不重复。

**测试职责**：验证未实现 `hpmcounter` 在 V=1 下的合法行为空间与 `hpmcounter` 的 VU 三层门控链。`hpmcounterN` 访问以 raw encoding 注入（CSR 0xC00+N，csrrs rd, csr, x0 形式），避免工具链别名干扰。

### 7.1 未实现 hpmcounter 在 V=1 下的行为

**前置条件**：以 M-mode 写 `mhpmcounterN` 非零并回读的方式探测（沿用 `Shcounterenw_test_plan.md` 的探测策略），选定一个未实现（只读零）的 `hpmcounterN`（N∈3..31）；平台全部实现时本子节全套 TEST_SKIP。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZHPM-01 | VS-mode 访问未实现计数器（尝试开门控） | 尝试置 `mcounteren[N]`/`hcounteren[N]`=1 并回读探测：两者均生效时（门控开启）VS-mode 执行 `csrr hpmcounterN`（raw encoding，trap-armed）；`mcounteren[N]` 只读零时按 `mcounteren` 层先决路径验证 | 门控开启：两种行为均合法——返回常数值（含 0）或触发 illegal-instruction (cause=2)，不得报 virtual-instruction；`mcounteren[N]` 只读零：必须触发异常且为 illegal-instruction (cause=2)。记录实现选择，均符合 `norm:hpm_unimplemented_counter_access` 与 `mcounteren` 先决规则 |
| HZHPM-02 | VU-mode 访问未实现计数器（尝试开三层门控） | 尝试置 `mcounteren[N]`/`hcounteren[N]`/`scounteren[N]`=1 并回读探测，VU-mode 执行 `csrr hpmcounterN` | 三层全生效：常数值或 illegal-instruction (cause=2) 均合法；`mcounteren[N]` 只读零：必须报 illegal-instruction (cause=2)；仅 `hcounteren[N]`/`scounteren[N]` 只读零：必须报 virtual-instruction (cause=22)。记录实现行为 |
| HZHPM-03 | hcounteren[N]=0 遮断未实现计数器 | 尝试置 `mcounteren[N]`=1；清零 `hcounteren[N]`，VS-mode 执行 `csrr hpmcounterN` | 门控关闭时异常由门控触发，与计数器是否实现无关：`mcounteren[N]`=1 时 virtual-instruction (cause=22)；`mcounteren[N]` 只读零时由 `mcounteren` 层先决报 illegal-instruction (cause=2) |
| HZHPM-04 | 记录型：回读值一致性 | 重复 HZHPM-01 的访问若干次 | 若实现返回常数值，多次读回一致；若实现报异常，每次异常 cause 一致。记录实现行为，不做强制判定 |

### 7.2 hpmcounter 的 VU 三层门控链（已实现计数器）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZHPM-05 | hpmcounter VU 三层门控 | 选定已实现 `hpmcounterN`，三层 `mcounteren[N]`/`hcounteren[N]`/`scounteren[N]` 全 1 后 VU 读取；再单独清 `scounteren[N]` 重读 | 全 1 时无异常；清 `scounteren[N]` 后触发异常（层级链 `mcounteren → hcounteren → scounteren`，`scounteren` 对 VU 的约束来自 `H_scsrs_nomatch_vu_counter`） |

> [!NOTE]
> - 本组所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展；`hpmcounterN` 的已实现/未实现状态以 M-mode 写 `mhpmcounterN` 非零回读探测（沿用 `Shcounterenw_test_plan.md` 策略），不满足时对应小节/用例 TEST_SKIP。
> - `hpmcounterN` 访问以 raw encoding（csrrs rd, csr, x0 形式）注入，避免工具链对伪指令的展开差异。
> - HZHPM-01/02/04 为记录型用例：`norm:hpm_unimplemented_counter_access` 允许常数值与异常两种实现，不得以任一结果为由判失败；但门控开启时不得报 virtual-instruction（强制断言），门控关闭时的异常类型为强制断言（HZHPM-03：`mcounteren[N]`=1 → cause=22；`mcounteren[N]` 只读零 → `mcounteren` 层先决报 cause=2）。
> - 门控矩阵（逐位写回 + VS/VU 组合）已由 `Shcounterenw_test_plan.md` 覆盖，本组仅引用门控作为前置条件，不重复验证。
> - RV32 高半访问（`hpmcounter3h`–`hpmcounter31h`，`norm:hpm_counter_op_sz_mode_xlen32`）为条件规范点，RV64 平台不触发，本文档不设用例。

---

## 关键注意事项

1. **扩展检测**：所有测试必须在运行时检测所需扩展（H、Zkr、Zihintntl、Zcmt 等）的可用性，不可用时 TEST_SKIP。Zkr 通过 `seed` CSR（0x015）的存在性探测；Zihintntl 无独立探测标志，按平台配置声明启用；Zcmt 以平台配置 `ZCMT_SUPPORTED` 宏与 jvt CSR（0x017）trap-armed 探测为准。

2. **SSEED 控制**：`mseccfg.SSEED` 控制 S/HS/VS/VU-mode 对 seed CSR 的访问。M-mode 访问不受 SSEED 影响（ZKR-HYP-11）。

3. **只读访问优先**：只读 CSR 访问指令（csrrs/csrrc with rs1=x0，或 csrrsi/csrrci with uimm=0）访问 seed 在任何模式下都触发 illegal-instruction（cause=2），该条件优先于 virtual-instruction 判定。

4. **virtual-instruction 与 illegal-instruction 的区分**：VS/VU-mode 访问受控 CSR 时，SSEED=1 的 HS 限定读写触发 virtual-instruction (cause=22)；SSEED=0 或只读访问触发 illegal-instruction (cause=2)。

5. **Zawrs 时序依赖**：Group 4 的 `wrs.nto` 用例依赖"未在实现限定时间内完成"这一时序条件（与 `Hypervisor_CSR_test_plan.md` HSTAT-04 的 WFI/VTW 用例同风格）。实现若按 `norm:Zawrs_stall_terminate` 提前终止停顿，应保持用例失败并记录 `bugs/`，不得放宽断言。

6. **向量指令注入与上下文门控**：Group 5 的向量/向量浮点指令以 raw encoding 注入；`vsstatus.vs`/`vsstatus.fs` 的 Off 门控报 illegal-instruction (cause=2)，不是 virtual-instruction；实现允许随时将 Initial/Clean 提升为 Dirty（`norm:hw_mstatus_vs_dirty_update`），相关用例为记录型，不得以"状态被提升"为由判失败。

7. **Group 5 实现与平台验证现状**（实现：`Hypervisor_Vector/` 套件，13 用例）：平台 V/F 支持以 `config/<platform>/rvtest_config.h` 的 `V_SUPPORTED`/`F_SUPPORTED` 宏为准（不做运行时探测）。Spike（`rv64imafdcvh_zicsr_zifencei`）13/13 全部通过；QEMU（qemu-rv64-max，`-cpu max`）10 PASS / 2 FAIL / 1 SKIP：HVEC-07（`vsstatus.sd` 未随 `vsstatus.vs`=Dirty 置 1，`norm:vsstatus_sd_op_vs`）与 HVEC-12（修改浮点状态的向量浮点指令未将两处 `fs` 字段置 Dirty，`norm:vsstatus_mstatus_FS_dirty_hypervisor_V_fp`）为 QEMU 实现缺陷（Spike 对照通过），已归档 `bugs/qemu_hypervisor_vector_bugs.md`，用例保持失败；HVEC-13 因 QEMU `misa.v` 不可写而条件 SKIP（Spike 上实际执行并通过）。

8. **Group 6/7 计数器交集要点**：`rdtime`/计数器访问以 raw encoding 注入；时钟比较采用差值区间而非精确相等；未实现 `hpmcounter` 的常数值/异常两种行为均为合法实现（记录型用例，不得放宽也不得误判）；门控异常的 cause 区分（cause=22 vs cause=2）为强制断言；门控矩阵本身不重复验证（由 `Shcounterenw_test_plan.md` 覆盖）。注意：负偏移 `rdtime` 的断言必须用有符号差值而非无符号比较——开机早期真实 `time` 小于偏移幅度时截断回绕会使无符号比较不成立（HZCNT-04 实现要点）。

9. **Group 6/7 实现与平台验证现状**（实现：`Hypervisor_Zicntr/` 套件 9 用例、`Hypervisor_Zihpm/` 套件 5 用例）：QEMU（qemu-rv64-max，`-cpu max`）：Zicntr 9/9 PASS；Zihpm 4 PASS / 1 SKIP（QEMU 实现 `hpmcounter3-18`，`mhpmcounter19-31` 只读零；探测选中 `hpmcounter19`，其 `mcounteren[19]` 只读零，HZHPM-01~03 按 `mcounteren` 层先决路径验证并报 cause=2；HZHPM-04 因门控不可开合规 SKIP；HZHPM-05 在已实现的 `hpmcounter3` 上验证三层门控链并通过）。Spike（`rv64imach_zicsr_zifencei_zicntr`/`_zihpm`）：Zicntr 9/9 PASS；Zihpm 4 PASS / 1 SKIP（Spike 的 `mhpmcounter3-31` 镜像为只读零，`norm:mhpmcounter_mhpmevent_rdonly0` 允许该实现，按本方案约定视同未实现；HZHPM-01~04 在门控开启分支执行，读回常数值 0 或门控关闭报 cause=22 均为合法路径；HZHPM-05 因无已实现计数器合规 SKIP）。两平台分别覆盖 `mcounteren` 先决分支与门控开启常数值分支，0 FAIL。计数器已实现/未实现探测为 M-mode 写 `mhpmcounterN` 非零并回读（不是 `mcounteren` 位回粘探测：后者仅测门控位可写性，按 `norm:mcounteren_flds_rdonly0` 与计数器存在性无等价关系）。

---

## 参考

- `SPEC/hypervisor.adoc` — RISC-V Hypervisor Extension, Version 1.0
- `SPEC/riscv-isa-manual/src/unpriv/zk.adoc` — Zkr Entropy Source Extension
- `SPEC/riscv-isa-manual/src/unpriv/zihintntl.adoc` — Zihintntl Extension for Non-Temporal Locality Hints
- `SPEC/riscv-isa-manual/src/unpriv/zcmt.adoc` — Zcmt Extension for Compressed Table Jumps
- `SPEC/riscv-isa-manual/src/unpriv/vector-common.adoc` — V Vector Extension common definitions（向量上下文状态与 Hypervisor 交互）
- `SPEC/riscv-isa-manual/src/unpriv/zicntr.adoc` — Zicntr Extension for Base Counters and Timers（cycle/time/instret 与 Hypervisor 交集）
- `SPEC/riscv-isa-manual/src/unpriv/zihpm.adoc` — Zihpm Extension for Hardware Performance Counters（hpmcounter 与 Hypervisor 交集）
- `DOCS/testplan/Zkr_test_plan.md` — Zkr 独立测试计划
- `DOCS/testplan/zihintntl_test_plan.md` — Zihintntl 独立测试计划
- `DOCS/testplan/zcmt_test_plan.md` — Zcmt 独立测试计划
- `DOCS/testplan/Zawrs_test_plan.md` — Zawrs 独立测试计划（非 Hypervisor 场景）
- `DOCS/testplan/Hypervisor_CSR_test_plan.md` — Hypervisor CSR 子集测试计划
- `DOCS/testplan/Hypervisor_Interrupts_test_plan.md` — Hypervisor 中断子集测试计划
- `DOCS/testplan/Hypervisor_Exceptions_test_plan.md` — Hypervisor 异常与 trap 子集测试计划
- `DOCS/testplan/Hypervisor_2_stage_test_plan.md` — 两阶段翻译测试计划
- `DOCS/testplan/Hypervisor_gstage_test_plan.md` — G-stage 独立测试计划
- `DOCS/testplan/Zicntr_test_plan.md` — Zicntr 独立测试计划（非 Hypervisor 场景）
- `DOCS/testplan/Zihpm_test_plan.md` — Zihpm 独立测试计划（非 Hypervisor 场景）
- `DOCS/testplan/Shcounterenw_test_plan.md` — Shcounterenw 测试计划（hcounteren 可写性与门控矩阵）
- `DOCS/testplan/Hypervisor_Ss_test_plan.md` — Hypervisor × Ss* 交叉测试计划（含 Hypervisor × Sscounterenw Group 3、hcounteren.TM 对 vstimecmp 门控）

---

## 附录 A：规范点覆盖矩阵

下表标明"覆盖的规范点"章节中每条规范点被哪些测试用例覆盖。

| Norm ID | 覆盖的测试 ID |
|---------|---------------|
| `norm:mseccfg_sseed_SorHS-mode_op` | ZKR-HYP-01、ZKR-HYP-02、ZKR-HYP-13 |
| `norm:mseccfg_sseed_VSorVU-mode_op` | ZKR-HYP-03~06、ZKR-HYP-09、ZKR-HYP-10、ZKR-HYP-16、ZKR-HYP-17 |
| `norm:mseccfg_sseed_useed_op_tbl` | ZKR-HYP-03~06、ZKR-HYP-11 |
| `norm:seed_ro_illegal` | ZKR-HYP-07、ZKR-HYP-08、ZKR-HYP-12、ZKR-HYP-13、ZKR-HYP-14、ZKR-HYP-15、ZKR-HYP-18 |
| `norm:NTL_target_definition` | NTL-HYP-01 ~ NTL-HYP-06 |
| `norm:NTL_range` | NTL-HYP-04 |
| `norm:cm-jt_op` | HZCMT-01 ~ HZCMT-03、HZCMT-06 ~ HZCMT-09 |
| `norm:cm-jalt_op` | HZCMT-01 ~ HZCMT-03、HZCMT-06 |
| `norm:jvt_base_vm` | HZCMT-07 ~ HZCMT-09 |
| `norm:Zcmt_fetch` | HZCMT-08、HZCMT-09 |
| `norm:Zcmt_trap` | HZCMT-08、HZCMT-09 |
| `norm:stateen0_jvt_op` | HZCMT-04 ~ HZCMT-06 |
| `norm:htval_trapval` | HZCMT-09 |
| `norm:Zawrs_exec_resume_rules` | HZWRS-01 ~ HZWRS-03、HZWRS-06（HZWRS-06 为记录型：两种合法行为均接受） |
| `norm:Zawrs_virtual_instr_excp` | HZWRS-04、HZWRS-05、HZWRS-06、HZWRS-07 |
| `norm:Zawrs_priv_illegal_instr_excp` | HZWRS-08、HZWRS-09、HZWRS-12 |
| `norm:Zawrs_stall_terminate` | —（实现允许的提前终止停顿行为，作为 Group 4 NOTE 与关键注意事项 5 的失败处置依据，无直接用例） |
| `norm:H_virtinst_xtval` | HZWRS-07 |
| `norm:vsstatus_vs_sz_acc` | HVEC-01 |
| `norm:vsstatus_vs_mstatus_vs_op_off` | HVEC-02、HVEC-03、HVEC-04 |
| `norm:vsstatus_vs_mstatus_vs_op_active` | HVEC-05、HVEC-06 |
| `norm:hw_mstatus_vs_dirty_update` | HVEC-08（记录型：提升与保持均合法） |
| `norm:vsstatus_sd_op_vs` | HVEC-07 |
| `norm:vsstatus_vs_exists` | HVEC-13（条件：misa.v 可写时探测；记录型） |
| `norm:vsstatus_mstatus_FS_off_hypervisor_V_fp_ill` | HVEC-09、HVEC-10、HVEC-11 |
| `norm:vsstatus_mstatus_FS_dirty_hypervisor_V_fp` | HVEC-12 |
| `norm:zicntr_rdtime_op` | HZCNT-01 ~ HZCNT-06 |
| `norm:hpm_unimplemented_counter_access` | HZHPM-01、HZHPM-02、HZHPM-04（HZHPM-03 为门控关闭分支，异常由 `hcounteren_gate_v1_counter` 触发） |
| `H_scsrs_nomatch_vu_counter` | HZCNT-07、HZCNT-08、HZCNT-09、HZHPM-05 |
| `hcounteren_gate_v1_counter` | HZCNT-05、HZCNT-06、HZCNT-08、HZHPM-03、HZHPM-05（仅作为前置门控条件引用；门控矩阵本身由 `Shcounterenw_test_plan.md` 覆盖） |
