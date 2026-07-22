# Hypervisor × CFI 交叉测试计划

## 概述

本测试计划覆盖 RISC-V Hypervisor (H) 扩展与 CFI (Control-Flow Integrity) 扩展的交叉功能点。CFI 扩展包含两个子扩展：

- **Zicfilp**（前向控制流保护 / Landing Pad）：确保间接跳转/调用目标合法
- **Zicfiss**（后向控制流保护 / Shadow Stack）：确保函数返回地址未被篡改

在 Hypervisor 环境下，CFI 的使能、状态保存与恢复、异常委托、页表交互等均受 H 扩展影响。本测试计划聚焦于 H 扩展分别与 Zicfilp 和 Zicfiss 两个子扩展的交集行为。

本测试计划依据 `SPEC/hypervisor.adoc` 和 `SPEC/cfi.adoc` 中的规范点（norm 标记）编写。

### 本文档覆盖的 SPEC 章节

**来自 cfi.adoc：**
- Landing-Pad-Enabled (LPE) State（xLPE 确定表，VS-mode 由 henvcfg.LPE 控制）
- Preserving Expected Landing Pad State on Traps（ELP/xPELP 保存恢复，vsstatus.SPELP）
- Shadow Stack Pointer (ssp) CSR access control（VS/VU-mode 访问控制）
- Shadow-Stack-Enabled (SSE) State（xSSE 确定表，VS-mode 由 henvcfg.SSE 控制）
- Shadow Stack Memory Protection（SS 页类型、G-stage 交互、satp Bare 模式）

**来自 hypervisor.adoc：**
- henvcfg 寄存器（LPE/SSE 字段控制 VS-mode CFI 使能）
- vsstatus 寄存器（SPELP 字段保存 VS-mode 的 previous ELP）
- hedeleg 委托寄存器（software-check exception 委托）
- Trap Entry/Return 行为（VS-mode trap 时 ELP 保存恢复）

### 由其他测试计划覆盖
- Hypervisor 核心功能 → `Hypervisor_test_plan.md`
- CFI 基础功能（非虚拟化场景）→ `cfi_test_plan.md`
- G-stage / VS-stage 地址翻译 → `hyp_gstage_translation_test_plan.md` / `hyp_2_stage_translation_test_plan.md`

---

## 覆盖的规范点

本章节列出本文档所有测试组中引用的规范点（norm ID），已去重并按来源排列。

| Norm ID | 来源 | 原文 | 中文说明 |
|---------|------|------|----------|
| `norm:henvcfg_lpe_op` | hypervisor.adoc | The Zicfilp extension adds the LPE field in henvcfg. When LPE=1, the Zicfilp extension is enabled in VS-mode. When LPE=0, the hart does not update the ELP state and LPAD operates as a no-op. | Zicfilp 扩展向 henvcfg 添加 LPE 字段。LPE=1 时在 VS 模式启用 Zicfilp。LPE=0 时不更新 ELP 状态，LPAD 作为 no-op。 |
| `norm:henvcfg_sse_op` | hypervisor.adoc | The Zicfiss extension adds the SSE field in henvcfg. If SSE=1, the Zicfiss extension is activated in VS-mode. When SSE=0, 32-bit Zicfiss instructions revert to Zimop behavior, 16-bit to Zcmop, pte.xwr=010b becomes reserved, senvcfg.SSE reads zero, and SSAMOSWAP.W/D raises virtual-instruction exception when menvcfg.SSE=1. | Zicfiss 扩展向 henvcfg 添加 SSE 字段。SSE=1 时在 VS 模式激活 Zicfiss。SSE=0 时指令回退、页表编码保留、senvcfg.SSE 只读零、SSAMOSWAP 触发虚拟指令异常。 |
| `norm:vsstatus_spelp_op` | hypervisor.adoc | The Zicfilp extension adds the SPELP field that holds the previous ELP, and is updated as specified in ZICFILP_FORWARD_TRAPS. SPELP encoded as: 0=NO_LP_EXPECTED, 1=LP_EXPECTED. | Zicfilp 扩展向 vsstatus 添加 SPELP 字段保存 previous ELP。编码：0=NO_LP_EXPECTED，1=LP_EXPECTED。 |
| `norm:vsstatus_spelp_op2` | cfi.adoc | To store the previous ELP state on traps to VS-mode, a spelp bit is defined in the vsstatus (VS-modes version of sstatus). | 在 vsstatus 中定义 spelp 位以在陷阱到 VS 模式时保存 previous ELP 状态。 |
| `norm:cfi_mstatus_mpelp_op` | cfi.adoc | To store the previous ELP state on trap delivery to M-mode, an mpelp bit is provided in the mstatus CSR. | 在 mstatus CSR 中提供 mpelp 位以在陷阱到 M 模式时保存 previous ELP。 |
| `norm:cfi_mstatus_spelp_op` | cfi.adoc | To store the previous ELP state on trap delivery to S/HS-mode, an spelp bit is provided in the mstatus CSR. | 在 mstatus CSR 中提供 spelp 位以在陷阱到 S/HS 模式时保存 previous ELP。 |
| `norm:cfi_sstatus_spelp_op` | cfi.adoc | The spelp bit in mstatus can be accessed through the sstatus CSR. | mstatus 中的 spelp 位可通过 sstatus CSR 访问。 |
| `norm:Zicfilp_pelp_trap` | cfi.adoc | When a trap is taken into privilege mode x, the xpelp is set to ELP and ELP is set to NO_LP_EXPECTED. | 陷阱进入特权模式 x 时，xpelp 设为 ELP，ELP 设为 NO_LP_EXPECTED。 |
| `norm:Zicfilp_pelp_trap_return` | cfi.adoc | When executing an xret instruction, if the new privilege mode is y, then ELP is set to the value of xpelp if yLPE is 1; otherwise, it is set to NO_LP_EXPECTED; xpelp is set to NO_LP_EXPECTED. | 执行 xret 时，若新特权模式为 y，则 yLPE=1 时 ELP←xpelp，否则 ELP←NO_LP_EXPECTED；xpelp←NO_LP_EXPECTED。 |
| `norm:Zicfilp_forward_traps` | cfi.adoc | A trap may need to be delivered upon completion of jalr/c.jalr/c.jr but before the target instruction was decoded. The ELP prior to the trap must be preserved. | 陷阱可能需要在 jalr/c.jalr/c.jr 完成后但目标指令解码前交付。陷阱前的 ELP 必须被保存。 |
| `norm:lpad_sw_exception` | cfi.adoc | The software-check exception due to the instruction not being an lpad instruction when ELP is LP_EXPECTED leads to a trap. | ELP 为 LP_EXPECTED 时目标指令非 LPAD 导致的 software-check 异常引发陷阱。 |
| `norm:Zicfilp_exception_priority` | cfi.adoc | The software-check exception caused by Zicfilp has higher priority than an illegal-instruction exception but lower priority than instruction access-fault. | Zicfilp 的 software-check 异常优先级高于非法指令异常但低于指令访问故障。 |
| `norm:zicfiss_ssp_csr` | cfi.adoc | Attempts to access the ssp CSR may result in either an illegal-instruction exception or a virtual-instruction exception, contingent upon the state of the envcfg sse fields. | 访问 ssp CSR 可能引发非法指令异常或虚拟指令异常，取决于 envcfg sse 字段状态。 |
| `norm:zicfiss_m_menvcfg_sse` | cfi.adoc | If the privilege mode is less than M and menvcfg.sse is 0, an illegal-instruction exception is raised. | 特权级低于 M 且 menvcfg.sse=0 时引发非法指令异常。 |
| `norm:zicfiss_vs_henvcfg_sse` | cfi.adoc | Otherwise, if in VS-mode and henvcfg.sse is 0, a virtual-instruction exception is raised. | VS 模式且 henvcfg.sse=0 时引发虚拟指令异常。 |
| `norm:zicfiss_vu_henvcfg_senvcfg_sse` | cfi.adoc | Otherwise, if in VU-mode and either henvcfg.sse or senvcfg.sse is 0, a virtual-instruction exception is raised. | VU 模式且 henvcfg.sse 或 senvcfg.sse 为 0 时引发虚拟指令异常。 |
| `norm:zicfiss_sse_access` | cfi.adoc | Otherwise, the access is allowed. | 其他情况下访问被允许。 |
| `norm:ss_page_enc` | cfi.adoc | The encoding R=0, W=1, and X=0, is defined to represent an SS page. | 编码 R=0、W=1、X=0 表示 Shadow Stack 页。 |
| `norm:ssmp_henvcfg_sse` | cfi.adoc | When V=1 and henvcfg.sse=0, this encoding remains reserved at VS and VU levels. | V=1 且 henvcfg.sse=0 时，该编码在 VS 和 VU 级别保持保留。 |
| `norm:ssmp_menvcfg_sse` | cfi.adoc | When menvcfg.sse=0, this encoding remains reserved. | menvcfg.sse=0 时该编码保持保留。 |
| `norm:satp_mode_bare` | cfi.adoc | If satp.mode (or vsatp.mode when V=1) is set to Bare and the effective privilege mode is less than M, shadow stack instructions raise a store/AMO access-fault exception. | satp.mode（V=1 时为 vsatp.mode）为 Bare 且有效特权级低于 M 时，shadow stack 指令引发 store/AMO access-fault。 |
| `norm:ssmp_ssamoswap` | cfi.adoc | When the effective privilege mode is M, memory access by an ssamoswap.w or ssamoswap.d instruction results in a store/AMO access-fault exception. | 有效特权模式为 M 时，ssamoswap.w/d 的内存访问引发 store/AMO access-fault。 |
| `norm:ssmp_ss_page_access_fault` | cfi.adoc | Memory mapped as an SS page cannot be written to by instructions other than ssamoswap, sspush, and c.sspush. Attempts will raise a store/AMO access-fault exception. | SS 页不能被 ssamoswap/sspush/c.sspush 以外的指令写入。尝试写入引发 store/AMO access-fault。 |
| `norm:ssmp_ss_page_illegeal_access` | cfi.adoc | Should a shadow stack instruction access a page that is not designated as a shadow stack page and is not marked as read-only, a store/AMO access-fault exception will be invoked. | shadow stack 指令访问非 SS 页且非只读页时引发 store/AMO access-fault。 |
| `norm:ssmp_ss_read_only_page` | cfi.adoc | If the page being accessed by a shadow stack instruction is a read-only page, a store/AMO page-fault exception will be triggered. | shadow stack 指令访问只读页时引发 store/AMO page-fault。 |
| `norm:ss_fault_exception_code` | cfi.adoc | If a shadow stack instruction raises an access-fault, page-fault, or guest-page-fault exception, the reported exception cause is respectively store/AMO access fault (7), store/AMO page fault (15), or store/AMO guest-page fault (23). | shadow stack 指令引发异常时，报告的 cause 分别为 7/15/23。 |
| `norm:ssp_xlen_aligned` | cfi.adoc | If the virtual address in ssp is not XLEN aligned, then sspush/c.sspush/sspopchk/c.sspopchk cause a store/AMO access-fault exception. | ssp 虚拟地址未 XLEN 对齐时，相关指令引发 store/AMO access-fault。 |
| `norm:ssmp_ss_idempotent_memory` | cfi.adoc | If the memory referenced by sspush/c.sspush/sspopchk/c.sspopchk/ssamoswap.w/ssamoswap.d is not idempotent, then the instructions cause a store/AMO access-fault exception. | shadow stack 指令引用的内存非幂等时引发 store/AMO access-fault。 |
| `norm:active_g_stage_pte` | cfi.adoc | When G-stage page tables are active, the shadow stack instructions that access memory require the G-stage page table to have read-write permission; else a store/AMO guest-page-fault exception is raised. | G-stage 页表活跃时，shadow stack 指令需要 G-stage 读写权限，否则引发 store/AMO guest-page-fault。 |
| `norm:hedeleg_op` | hypervisor.adoc | A synchronous trap delegated to HS-mode is further delegated to VS-mode if V=1 before the trap and the corresponding hedeleg bit is set. | 已委托到 HS 模式的同步陷阱，若 V=1 且 hedeleg 对应位置位则进一步委托到 VS 模式。 |
| `norm:hedeleg_acc` | hypervisor.adoc | Each bit of hedeleg shall be either writable or read-only zero. | hedeleg 每位要么可写要么只读零。 |
| `norm:H_trap_vs_csrwrites` | hypervisor.adoc | When a trap is taken into VS-mode, vsstatus.SPP is set accordingly. hstatus and HS-level sstatus are not modified, V remains 1. | 陷阱进入 VS 模式时 vsstatus.SPP 相应设置，hstatus 和 HS 级 sstatus 不修改，V 保持 1。 |
| `norm:H_trap_hs_csrwrites` | hypervisor.adoc | When a trap is taken into HS-mode, V is set to 0, hstatus.SPV and sstatus.SPP are set accordingly. | 陷阱进入 HS 模式时 V 设为 0，SPV 和 SPP 相应设置。 |
| `norm:H_trap_m_csrwrites` | hypervisor.adoc | When a trap is taken into M-mode, V gets set to 0, MPV and MPP in mstatus are set accordingly. | 陷阱进入 M 模式时 V 设为 0，MPV 和 MPP 相应设置。 |
| `norm:sret_v1` | hypervisor.adoc | When executed in VS-mode (V=1), SRET sets the privilege mode accordingly, in vsstatus sets SPP=0, SIE=SPIE, and SPIE=1, and sets pc=vsepc. | VS 模式执行 SRET 时相应设置特权模式，vsstatus 中 SPP=0/SIE=SPIE/SPIE=1，pc=vsepc。 |
| `norm:sret_v0` | hypervisor.adoc | When executed in M-mode or HS-mode (V=0), SRET determines new mode according to hstatus.SPV and sstatus.SPP, sets SPV=0, SPP=0, SIE=SPIE, SPIE=1, pc=sepc. | M/HS 模式执行 SRET 时根据 SPV/SPP 确定新模式，设置相应字段，pc=sepc。 |

---

# Part A: Hypervisor × Zicfilp（前向控制流 / Landing Pad）

---

## Group A1. henvcfg.LPE 字段与 VS-mode Zicfilp 使能控制

**规范依据**：
- `norm:henvcfg_lpe_op`：LPE=1 时 VS-mode 启用 Zicfilp；LPE=0 时 ELP 保持 NO_LP_EXPECTED，LPAD 为 no-op
- cfi.adoc xLPE 确定表：VS-mode xLPE = henvcfg.LPE；VU-mode xLPE = senvcfg.LPE

**测试职责**：验证 henvcfg.LPE 字段对 VS-mode Zicfilp 扩展的使能控制，包括 LPE=0 时 ELP 不更新和 LPAD 退化为 no-op 的行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCFI-LP-01 | henvcfg.LPE 基本读写 | HS-mode 写 henvcfg.LPE=1 并读回 | LPE=1（若 Zicfilp 已实现）；未实现时只读零 |
| HCFI-LP-02 | henvcfg.LPE=0 时 VS-mode ELP 不更新 | 设 henvcfg.LPE=0，menvcfg.LPE=1，VS-mode 执行间接跳转（JALR）到 LPAD 目标 | ELP 保持 NO_LP_EXPECTED，LPAD 作为 no-op 执行，无异常 |
| HCFI-LP-03 | henvcfg.LPE=1 时 VS-mode ELP 更新 | 设 henvcfg.LPE=1, menvcfg.LPE=1, mseccfg.MLPE=1，VS-mode 执行间接跳转（JALR）到 LPAD 目标 | ELP 从 LP_EXPECTED 被 LPAD 清除为 NO_LP_EXPECTED，正常执行无异常 |
| HCFI-LP-04 | henvcfg.LPE=0 时 VS-mode LPAD 为 no-op | 设 henvcfg.LPE=0，VS-mode 执行 LPAD 指令 | LPAD 执行为 no-op，不触发异常，ELP 不变 |
| HCFI-LP-05 | henvcfg.LPE=1 时 VS-mode 非法间接跳转触发 LP Fault | 设 henvcfg.LPE=1，VS-mode 间接跳转到非 LPAD 目标 | software-check exception (cause=18, stval/vstval=2) |
| HCFI-LP-06 | henvcfg.LPE=0 时 VS-mode 非法间接跳转不触发 LP Fault | 设 henvcfg.LPE=0，VS-mode 间接跳转到非 LPAD 目标 | 无 software-check exception，ELP 保持 NO_LP_EXPECTED |
| HCFI-LP-07 | VU-mode xLPE 由 senvcfg.LPE 控制 | V=1, henvcfg.LPE=1, senvcfg.LPE=0，VU-mode 间接跳转到非 LPAD 目标 | 无 software-check exception（VU-mode xLPE=0） |
| HCFI-LP-08 | VU-mode xLPE=1 时触发 LP Fault | V=1, henvcfg.LPE=1, senvcfg.LPE=1，VU-mode 间接跳转到非 LPAD 目标 | software-check exception (cause=18, stval/vstval=2) |
| HCFI-LP-09 | henvcfg.LPE=1 但 menvcfg.LPE=0 | 设 menvcfg.LPE=0, henvcfg.LPE=1，VS-mode 执行间接跳转 | ELP 不更新（menvcfg.LPE=0 使 HS-level xLPE=0，但 VS-level xLPE=henvcfg.LPE 应独立生效）；验证 VS-mode Zicfilp 是否正常工作 |
| HCFI-LP-10 | henvcfg.LPE 未实现时只读零 | 若 Zicfilp 未实现，读 henvcfg.LPE | 只读零 |

---

## Group A2. vsstatus.SPELP 字段与 VS-mode trap 保存恢复

**规范依据**：
- `norm:vsstatus_spelp_op`：vsstatus.SPELP 保存 VS-mode 的 previous ELP，编码 0=NO_LP_EXPECTED, 1=LP_EXPECTED
- `norm:vsstatus_spelp_op2`：vsstatus 中定义 spelp 位用于陷阱到 VS-mode 时保存 previous ELP
- `norm:Zicfilp_pelp_trap`：陷阱进入模式 x 时 xpelp←ELP, ELP←NO_LP_EXPECTED
- `norm:Zicfilp_pelp_trap_return`：xRET 时若新模式 y 的 yLPE=1 则 ELP←xpelp，否则 ELP←NO_LP_EXPECTED

**测试职责**：验证 vsstatus.SPELP 在 VS-mode trap entry/return 时的保存恢复行为，以及跨特权级（VS↔HS, VS↔M）的 ELP 状态传递。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCFI-LP-11 | vsstatus.SPELP 基本读写 | HS-mode 写 vsstatus.SPELP=1 并读回 | SPELP=1 |
| HCFI-LP-12 | VS-mode trap 到 VS-mode 时 SPELP 保存 ELP | 设 henvcfg.LPE=1，VS-mode 中 ELP=LP_EXPECTED 时触发已委托异常进入 VS-mode | vsstatus.SPELP=1（保存 LP_EXPECTED），ELP 设为 NO_LP_EXPECTED |
| HCFI-LP-13 | VS-mode SRET 恢复 ELP（VSLPE=1） | 承接 HCFI-LP-12，VS-mode trap handler 中执行 SRET 返回 | ELP 恢复为 vsstatus.SPELP 的值（LP_EXPECTED），SPELP 设为 NO_LP_EXPECTED |
| HCFI-LP-14 | VS-mode SRET 不恢复 ELP（VSLPE=0） | 设 henvcfg.LPE=0，VS-mode trap 时 SPELP 保存了 ELP，执行 SRET | ELP 设为 NO_LP_EXPECTED（VSLPE=0），SPELP 设为 NO_LP_EXPECTED |
| HCFI-LP-15 | VS-mode trap 到 HS-mode 时 mstatus.SPELP 保存 ELP | 设 henvcfg.LPE=1，VS-mode 中 ELP=LP_EXPECTED 时触发未委托异常进入 HS-mode | mstatus.SPELP=1（保存 LP_EXPECTED），ELP 设为 NO_LP_EXPECTED |
| HCFI-LP-16 | HS-mode SRET 返回 VS-mode 恢复 ELP（VSLPE=1） | 承接 HCFI-LP-15，HS-mode 执行 SRET 返回 VS-mode | ELP 恢复为 mstatus.SPELP（LP_EXPECTED），mstatus.SPELP 设为 NO_LP_EXPECTED |
| HCFI-LP-17 | HS-mode SRET 返回 VS-mode 不恢复 ELP（VSLPE=0） | 设 henvcfg.LPE=0，HS-mode SRET 返回 VS-mode | ELP 设为 NO_LP_EXPECTED（VSLPE=0） |
| HCFI-LP-18 | VS-mode trap 到 M-mode 时 mstatus.MPELP 保存 ELP | 设 henvcfg.LPE=1，VS-mode 中 ELP=LP_EXPECTED 时触发异常进入 M-mode | mstatus.MPELP=1（保存 LP_EXPECTED），ELP 设为 NO_LP_EXPECTED |
| HCFI-LP-19 | M-mode MRET 返回 VS-mode 恢复 ELP（VSLPE=1） | 承接 HCFI-LP-18，M-mode 执行 MRET 返回 VS-mode | ELP 恢复为 mstatus.MPELP（LP_EXPECTED），mstatus.MPELP 设为 NO_LP_EXPECTED |
| HCFI-LP-20 | M-mode MRET 返回 VS-mode 不恢复 ELP（VSLPE=0） | 设 henvcfg.LPE=0，M-mode MRET 返回 VS-mode | ELP 设为 NO_LP_EXPECTED（VSLPE=0） |
| HCFI-LP-21 | ELP=NO_LP_EXPECTED 时 trap 保存 SPELP=0 | 设 henvcfg.LPE=1，VS-mode 中 ELP=NO_LP_EXPECTED 时触发 trap 到 VS-mode | vsstatus.SPELP=0（保存 NO_LP_EXPECTED） |
| HCFI-LP-22 | VS-mode SRET 返回 VU-mode 时 ELP 恢复 | 设 henvcfg.LPE=1, senvcfg.LPE=1，VS-mode 中 vsstatus.SPELP=1，SRET 返回 VU-mode（SPP=0） | ELP 恢复为 vsstatus.SPELP（VULPE=1 时），SPELP 设为 NO_LP_EXPECTED |
| HCFI-LP-23 | VS-mode SRET 返回 VU-mode 时 VULPE=0 | 设 senvcfg.LPE=0，VS-mode SRET 返回 VU-mode | ELP 设为 NO_LP_EXPECTED（VULPE=0） |
| HCFI-LP-24 | V=0 时 vsstatus.SPELP 不影响行为 | V=0 时设 vsstatus.SPELP=1，HS-mode 执行间接跳转 | ELP 不受 vsstatus.SPELP 影响 |

---

## Group A3. VS-mode Landing Pad 功能与 software-check exception

**规范依据**：
- `norm:lpad_sw_exception`：ELP=LP_EXPECTED 时目标指令非 LPAD → software-check exception
- `norm:Zicfilp_exception_priority`：software-check 优先级高于 illegal-instruction，低于 instruction access-fault
- `norm:Zicfilp_forward_traps`：陷阱可能在 JALR 完成后但目标解码前交付，ELP 需被保存

**测试职责**：验证 VS-mode 下 Zicfilp 的 landing pad 检查功能、software-check exception 的触发条件以及异常优先级。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCFI-LP-25 | VS-mode 合法间接调用到 LPAD 目标 | henvcfg.LPE=1，VS-mode JALR 到以 LPAD 开头的函数 | 正常执行，ELP 从 LP_EXPECTED 清除为 NO_LP_EXPECTED |
| HCFI-LP-26 | VS-mode 非法间接调用触发 LP Fault | henvcfg.LPE=1，VS-mode JALR 到非 LPAD 目标 | software-check exception (cause=18, vstval=2) |
| HCFI-LP-27 | VS-mode 合法间接跳转到 LPAD 目标 | henvcfg.LPE=1，VS-mode JALR（非 call）到 LPAD 目标 | 正常执行 |
| HCFI-LP-28 | VS-mode 非法间接跳转触发 LP Fault | henvcfg.LPE=1，VS-mode JALR（非 call）到非 LPAD 目标 | software-check exception (cause=18, vstval=2) |
| HCFI-LP-29 | VS-mode LP Fault 与 instruction access-fault 优先级 | VS-mode 间接跳转到不可访问地址（同时触发 access-fault 和 LP check） | instruction access-fault 优先于 software-check exception |
| HCFI-LP-30 | VS-mode LP Fault 与 illegal-instruction 优先级 | VS-mode 间接跳转目标为非法指令（同时触发 illegal 和 LP check） | software-check exception 优先于 illegal-instruction exception |
| HCFI-LP-31 | VS-mode 中断打断间接跳转时 ELP 保存 | henvcfg.LPE=1，VS-mode JALR 执行后、LPAD 解码前收到中断 | ELP=LP_EXPECTED 被保存到 vsstatus.SPELP（trap 到 VS）或 mstatus.SPELP（trap 到 HS） |
| HCFI-LP-32 | VS-mode C.JALR 非法跳转触发 LP Fault | henvcfg.LPE=1，VS-mode C.JALR 到非 LPAD 目标 | software-check exception (cause=18, vstval=2) |
| HCFI-LP-33 | VS-mode C.JR 非法跳转触发 LP Fault | henvcfg.LPE=1，VS-mode C.JR 到非 LPAD 目标 | software-check exception (cause=18, vstval=2) |

---

## Group A4. VS-mode software-check exception 委托

**规范依据**：
- `norm:hedeleg_op`：V=1 时已委托到 HS 的同步异常，若 hedeleg 对应位置位则进一步委托到 VS
- `norm:hedeleg_acc`：hedeleg bit 18（software-check）可写
- `norm:H_trap_vs_csrwrites`：trap 到 VS-mode 时 vsepc/vscause/vstval 写入
- `norm:H_trap_hs_csrwrites`：trap 到 HS-mode 时 sepc/scause/stval 写入

**测试职责**：验证 software-check exception（cause=18）在 Hypervisor 环境下的三级委托链路（M→HS→VS），以及 vstval/stval 的正确写入。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCFI-LP-34 | hedeleg[18] 可写性验证 | HS-mode 写 hedeleg bit 18=1 并读回 | bit 18 可写（若 Zicfilp 已实现）；否则只读零 |
| HCFI-LP-35 | LP Fault 委托到 VS-mode | medeleg[18]=1, hedeleg[18]=1, henvcfg.LPE=1，VS-mode 触发 LP Fault | trap 进入 VS-mode，vscause=18, vstval=2 |
| HCFI-LP-36 | LP Fault 委托到 HS-mode（hedeleg=0） | medeleg[18]=1, hedeleg[18]=0，VS-mode 触发 LP Fault | trap 进入 HS-mode，scause=18, stval=2 |
| HCFI-LP-37 | LP Fault 委托到 M-mode（medeleg=0） | medeleg[18]=0，VS-mode 触发 LP Fault | trap 进入 M-mode，mcause=18, mtval=2 |
| HCFI-LP-38 | LP Fault 委托到 VS-mode 时 vsepc 正确 | medeleg[18]=1, hedeleg[18]=1，VS-mode 触发 LP Fault | vsepc = 目标指令（非 LPAD 指令）的地址 |
| HCFI-LP-39 | LP Fault 委托到 VS-mode 时 GVA=0 | medeleg[18]=1, hedeleg[18]=1，VS-mode 触发 LP Fault | hstatus.GVA=0（非地址翻译相关异常） |
| HCFI-LP-40 | LP Fault 委托到 HS-mode 时 sepc 正确 | medeleg[18]=1, hedeleg[18]=0，VS-mode 触发 LP Fault | sepc = 目标指令（非 LPAD 指令）的地址, hstatus.SPV=1 |
| HCFI-LP-41 | LP Fault trap 到 M-mode 时 MPV=1 | medeleg[18]=0，VS-mode 触发 LP Fault | mstatus.MPV=1, mstatus.MPP=1 |

---

## Group A5. VS-mode Zicfilp trap 中断与异步事件交互

**规范依据**：
- `norm:Zicfilp_forward_traps`：异步中断和高于 software-check 优先级的同步异常可在 JALR 后目标解码前交付陷阱
- `norm:Zicfilp_forward_trap_async_interrupt`：异步中断可在间接跳转后、目标解码前交付
- `norm:Zicfilp_forward_trap_async_exception`：高于 software-check 优先级的同步异常可先交付

**测试职责**：验证 VS-mode 下间接跳转过程中被异步事件打断时 ELP 的正确保存与恢复。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCFI-LP-42 | VS-mode JALR 后异步中断保存 ELP | henvcfg.LPE=1，VS-mode JALR 执行后、LPAD 解码前注入异步中断 | ELP=LP_EXPECTED 被保存到 vsstatus.SPELP（委托到 VS）或 mstatus.SPELP（到 HS） |
| HCFI-LP-43 | 中断返回后 ELP 恢复并继续 LP 检查 | 承接 HCFI-LP-42，中断 handler 返回后 | ELP 恢复为 LP_EXPECTED，继续对目标指令进行 LP 检查 |
| HCFI-LP-44 | VS-mode JALR 后高优先级异常保存 ELP | henvcfg.LPE=1，VS-mode JALR 到不可执行页面（instruction access-fault 优先级高于 software-check） | ELP 被保存，trap 报告 instruction access-fault 而非 software-check |

---

# Part B: Hypervisor × Zicfiss（后向控制流 / Shadow Stack）

---

## Group B1. henvcfg.SSE 字段与 VS-mode Zicfiss 使能控制

**规范依据**：
- `norm:henvcfg_sse_op`：SSE=1 激活 VS-mode Zicfiss；SSE=0 时指令回退、页表编码保留、senvcfg.SSE 只读零、SSAMOSWAP 触发虚拟指令异常
- cfi.adoc xSSE 确定表：VS-mode xSSE = henvcfg.SSE；VU-mode xSSE = senvcfg.SSE

**测试职责**：验证 henvcfg.SSE 字段对 VS-mode Zicfiss 扩展的使能控制，包括 SSE=0 时的指令回退、页表编码保留等行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCFI-SS-01 | henvcfg.SSE 基本读写 | HS-mode 写 henvcfg.SSE=1 并读回 | SSE=1（若 Zicfiss 已实现）；未实现时只读零 |
| HCFI-SS-02 | henvcfg.SSE=1 时 VS-mode Zicfiss 指令正常执行 | 设 henvcfg.SSE=1, menvcfg.SSE=1，VS-mode 执行 SSPUSH | SSPUSH 正常执行，无异常 |
| HCFI-SS-03 | henvcfg.SSE=0 时 32-bit Zicfiss 指令回退为 Zimop | 设 henvcfg.SSE=0，VS-mode 执行 32-bit SSPUSH 指令 | 指令作为 Zimop no-op 执行，不触发异常 |
| HCFI-SS-04 | henvcfg.SSE=0 时 16-bit Zicfiss 指令回退为 Zcmop | 设 henvcfg.SSE=0，VS-mode 执行 C.SSPUSH 指令 | 指令作为 Zcmop no-op 执行，不触发异常 |
| HCFI-SS-05 | henvcfg.SSE=0 时 VS-stage pte.xwr=010 保留 | 设 henvcfg.SSE=0，VS-stage 页表使用 pte.xwr=010 编码 | 该编码保留，触发 page-fault |
| HCFI-SS-06 | henvcfg.SSE=0 时 senvcfg.SSE 只读零 | 设 henvcfg.SSE=0，VS-mode 读 senvcfg.SSE | senvcfg.SSE=0 且不可写 |
| HCFI-SS-07 | henvcfg.SSE=0 时 senvcfg.SSE 写入无效 | 设 henvcfg.SSE=0，VS-mode 写 senvcfg.SSE=1 并读回 | senvcfg.SSE 保持 0 |
| HCFI-SS-08 | henvcfg.SSE=1 时 senvcfg.SSE 可写 | 设 henvcfg.SSE=1，VS-mode 写 senvcfg.SSE=1 并读回 | senvcfg.SSE=1 |
| HCFI-SS-09 | henvcfg.SSE=0 + menvcfg.SSE=1 时 VS-mode SSAMOSWAP 触发异常 | 设 henvcfg.SSE=0, menvcfg.SSE=1，VS-mode 执行 SSAMOSWAP.W | virtual-instruction exception (cause=22) |
| HCFI-SS-10 | henvcfg.SSE=1 时 VS-mode SSAMOSWAP 正常执行 | 设 henvcfg.SSE=1, menvcfg.SSE=1，VS-mode 执行 SSAMOSWAP.W（目标为 SS 页） | 正常执行，无异常 |
| HCFI-SS-11 | henvcfg.SSE=0 + menvcfg.SSE=0 时 VS-mode SSAMOSWAP 行为 | 设 henvcfg.SSE=0, menvcfg.SSE=0，VS-mode 执行 SSAMOSWAP.W | illegal-instruction exception (cause=2)（SSAMOSWAP 为 AMO 编码，非 Zimop，特权级 < M 且 menvcfg.SSE=0 时按 cfi.adoc 操作伪码触发非法指令异常） |
| HCFI-SS-12 | VU-mode xSSE 由 senvcfg.SSE 控制 | V=1, henvcfg.SSE=1, senvcfg.SSE=0，VU-mode 执行 SSPUSH | 32-bit 指令回退为 Zimop no-op（VU-mode xSSE=0） |
| HCFI-SS-13 | henvcfg.SSE 未实现时只读零 | 若 Zicfiss 未实现，读 henvcfg.SSE | 只读零 |
| HCFI-SS-14 | henvcfg.SSE=0 时 pte.xwr=010 触发异常类型 | 设 henvcfg.SSE=0，VS-stage 页表 pte.xwr=010，VS-mode 访问该页 | 触发 page-fault（编码保留），非 access-fault |

---

## Group B2. ssp CSR 在 VS/VU-mode 的访问控制

**规范依据**：
- `norm:zicfiss_ssp_csr`：ssp CSR 访问受 envcfg sse 字段控制
- `norm:zicfiss_m_menvcfg_sse`：特权级 < M 且 menvcfg.SSE=0 → illegal-instruction exception
- `norm:zicfiss_vs_henvcfg_sse`：VS-mode 且 henvcfg.SSE=0 → virtual-instruction exception
- `norm:zicfiss_vu_henvcfg_senvcfg_sse`：VU-mode 且 henvcfg.SSE=0 或 senvcfg.SSE=0 → virtual-instruction exception
- `norm:zicfiss_sse_access`：其他情况允许访问

**测试职责**：验证 ssp CSR 在 VS/VU-mode 下的访问控制，包括异常类型区分（illegal vs virtual）和多级使能门控。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCFI-SS-15 | VS-mode + henvcfg.SSE=0 访问 ssp 触发异常 | 设 henvcfg.SSE=0, menvcfg.SSE=1，VS-mode csrr ssp | virtual-instruction exception (cause=22) |
| HCFI-SS-16 | VS-mode + henvcfg.SSE=1 + menvcfg.SSE=1 访问 ssp 正常 | 设 henvcfg.SSE=1, menvcfg.SSE=1，VS-mode csrr/csrw ssp | 正常读写 |
| HCFI-SS-17 | VS-mode + menvcfg.SSE=0 访问 ssp 触发异常 | 设 menvcfg.SSE=0（henvcfg.SSE 任意），VS-mode csrr ssp | illegal-instruction exception (cause=2)（menvcfg.SSE=0 优先级最高） |
| HCFI-SS-18 | VU-mode + henvcfg.SSE=0 访问 ssp 触发异常 | 设 henvcfg.SSE=0, senvcfg.SSE=1, menvcfg.SSE=1，VU-mode csrr ssp | virtual-instruction exception (cause=22) |
| HCFI-SS-19 | VU-mode + senvcfg.SSE=0 访问 ssp 触发异常 | 设 henvcfg.SSE=1, senvcfg.SSE=0, menvcfg.SSE=1，VU-mode csrr ssp | virtual-instruction exception (cause=22) |
| HCFI-SS-20 | VU-mode + henvcfg.SSE=1 + senvcfg.SSE=1 + menvcfg.SSE=1 访问 ssp 正常 | 全部使能，VU-mode csrr/csrw ssp | 正常读写 |
| HCFI-SS-21 | VU-mode + menvcfg.SSE=0 访问 ssp 触发异常 | 设 menvcfg.SSE=0，VU-mode csrr ssp | illegal-instruction exception (cause=2) |
| HCFI-SS-22 | VS-mode 写 ssp 后 HS-mode 可读 | 设 henvcfg.SSE=1, menvcfg.SSE=1，VS-mode csrw ssp=VAL，HS-mode csrr ssp | 读到 VAL |
| HCFI-SS-23 | VS-mode ssp 对齐检查 | 设 henvcfg.SSE=1，VS-mode 写 ssp 为非 XLEN 对齐值后执行 SSPUSH | store/AMO access-fault exception (`norm:ssp_xlen_aligned`) |

---

## Group B3. VS-stage 页表 Shadow Stack 页类型

**规范依据**：
- `norm:ss_page_enc`：R=0, W=1, X=0 表示 SS 页
- `norm:ssmp_henvcfg_sse`：V=1 且 henvcfg.SSE=0 时该编码保留
- `norm:ssmp_ss_page_access_fault`：非 SS 指令写入 SS 页 → store/AMO access-fault
- `norm:ssmp_ss_page_illegeal_access`：SS 指令访问非 SS 且非只读页 → store/AMO access-fault
- `norm:ssmp_ss_read_only_page`：SS 指令访问只读页 → store/AMO page-fault
- `norm:ss_fault_exception_code`：SS 指令异常 cause 为 7/15/23
- `norm:ssmp_ss_idempotent_memory`：非幂等内存 → store/AMO access-fault

**测试职责**：验证 VS-stage 页表中 Shadow Stack 页类型的编码、保护机制和异常行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCFI-SS-24 | henvcfg.SSE=1 时 pte.xwr=010 为 SS 页 | 设 henvcfg.SSE=1，VS-stage 页表设 pte.xwr=010，VS-mode SSPUSH 到该页 | 正常执行（SS 页有效） |
| HCFI-SS-25 | henvcfg.SSE=0 时 pte.xwr=010 保留 | 设 henvcfg.SSE=0，VS-stage 页表设 pte.xwr=010，VS-mode 访问该页 | page-fault（编码保留） |
| HCFI-SS-26 | 普通 store 写入 SS 页触发 access-fault | 设 henvcfg.SSE=1，VS-stage 页表设 pte.xwr=010，VS-mode 执行普通 SW | store/AMO access-fault (cause=7) |
| HCFI-SS-27 | 普通 load 读取 SS 页正常 | 设 henvcfg.SSE=1，VS-stage 页表设 pte.xwr=010，VS-mode 执行 LW | 正常读取（SS 页可被所有 load 指令读取） |
| HCFI-SS-28 | SS 指令访问非 SS 非只读页触发 access-fault | 设 henvcfg.SSE=1，VS-stage 页表设 pte.xwr=011（RW），VS-mode SSPUSH 到该页 | store/AMO access-fault (cause=7) |
| HCFI-SS-29 | SS 指令访问只读页触发 page-fault | 设 henvcfg.SSE=1，VS-stage 页表设 pte.xwr=001（R），VS-mode SSPUSH 到该页 | store/AMO page-fault (cause=15) |
| HCFI-SS-30 | CBO 指令访问 SS 页触发 access-fault | 设 henvcfg.SSE=1，VS-stage 页表设 pte.xwr=010，VS-mode 执行 CBO.INVAL | store/AMO access-fault (cause=7) |
| HCFI-SS-31 | 指令取指从 SS 页触发 access-fault | 设 henvcfg.SSE=1，VS-stage 页表设 pte.xwr=010，VS-mode 执行跳转到该页 | instruction access-fault（SS 页不允许指令取指） |
| HCFI-SS-32 | SSPOPCHK 读 SS 页正常 | 设 henvcfg.SSE=1，VS-stage 页表设 pte.xwr=010，VS-mode 先 SSPUSH 再 SSPOPCHK | 正常执行（SSPOPCHK 读取 SS 页） |
| HCFI-SS-33 | SSPOPCHK 读 SS 页异常报告为 store 类型 | 设 henvcfg.SSE=1，VS-stage SS 页不存在映射，VS-mode SSPOPCHK | store/AMO page-fault (cause=15)，非 load page-fault |
| HCFI-SS-34 | SS 页 COW（copy-on-write）场景 | 设 henvcfg.SSE=1，VS-stage 页表设 pte.xwr=001（只读，COW 标记），VS-mode SSPUSH | store/AMO page-fault (cause=15)，允许 OS 处理 COW |
| HCFI-SS-35 | U/SUM 位对 SS 页访问的影响 | 设 henvcfg.SSE=1, vsstatus.SUM=1，VS-mode 访问 VU-mode 的 SS 页 | U/SUM 位对 SS 指令访问正常生效 |
| HCFI-SS-36 | MXR 位不影响 SS 页读取 | 设 henvcfg.SSE=1, vsstatus.MXR=1，验证 MXR 不影响 SS 页的 load 行为 | SS 页始终可被 load 指令读取，MXR 无额外影响 |

---

## Group B4. G-stage 翻译与 Shadow Stack 交互

**规范依据**：
- cfi.adoc：G-stage 地址翻译和保护不受 Zicfiss 扩展影响，G-stage pte.xwr=010 保持保留
- `norm:active_g_stage_pte`：G-stage 页表活跃时，shadow stack 指令需要 G-stage 读写权限，否则 store/AMO guest-page-fault

**测试职责**：验证 G-stage 地址翻译与 VS-mode shadow stack 指令的交互行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCFI-SS-37 | G-stage 页表 pte.xwr=010 保持保留 | 设 G-stage 页表 pte.xwr=010，VS-mode SSPUSH | guest-page-fault（G-stage 不支持 SS 页编码） |
| HCFI-SS-38 | G-stage RW 权限下 SS 指令正常执行 | 设 henvcfg.SSE=1，VS-stage SS 页有效，G-stage 页表 pte.xwr=011（RW），VS-mode SSPUSH | 正常执行 |
| HCFI-SS-39 | G-stage 只读权限下 SS 指令触发 guest-page-fault | 设 VS-stage SS 页有效，G-stage 页表 pte.xwr=001（只读），VS-mode SSPUSH | store/AMO guest-page-fault (cause=23) |
| HCFI-SS-40 | G-stage 无权限下 SS 指令触发 guest-page-fault | 设 VS-stage SS 页有效，G-stage 页表 pte.xwr=000（无权限），VS-mode SSPUSH | store/AMO guest-page-fault (cause=23) |
| HCFI-SS-41 | G-stage guest-page-fault 异常委托 | VS-mode SSPUSH 触发 G-stage guest-page-fault | trap 到 HS-mode（guest-page-fault 不可委托到 VS），hstatus.GVA=1 |
| HCFI-SS-42 | SSPOPCHK 触发 G-stage guest-page-fault | VS-mode SSPOPCHK，G-stage 页表无写权限 | store/AMO guest-page-fault (cause=23)（非 load guest-page-fault） |

---

## Group B5. satp/vsatp Bare 模式下 Shadow Stack 行为

**规范依据**：
- `norm:satp_mode_bare`：satp.mode（V=1 时为 vsatp.mode）为 Bare 且有效特权级 < M 时，shadow stack 指令引发 store/AMO access-fault

**测试职责**：验证 vsatp.mode=Bare 时 VS/VU-mode 下 shadow stack 指令的异常行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCFI-SS-43 | vsatp.mode=Bare 时 VS-mode SSPUSH 触发 access-fault | 设 henvcfg.SSE=1, vsatp.mode=Bare, V=1，VS-mode 执行 SSPUSH | store/AMO access-fault (cause=7) |
| HCFI-SS-44 | vsatp.mode=Bare 时 VS-mode SSPOPCHK 触发 access-fault | 设 henvcfg.SSE=1, vsatp.mode=Bare, V=1，VS-mode 执行 SSPOPCHK | store/AMO access-fault (cause=7) |
| HCFI-SS-45 | vsatp.mode=Bare 时 VS-mode SSAMOSWAP 触发 access-fault | 设 henvcfg.SSE=1, vsatp.mode=Bare, V=1，VS-mode 执行 SSAMOSWAP.W | store/AMO access-fault (cause=7) |
| HCFI-SS-46 | vsatp.mode=Bare 时 VU-mode SS 指令触发 access-fault | 设 henvcfg.SSE=1, senvcfg.SSE=1, vsatp.mode=Bare, V=1，VU-mode 执行 SSPUSH | store/AMO access-fault (cause=7) |
| HCFI-SS-47 | vsatp.mode 非 Bare 时 SS 指令正常 | 设 henvcfg.SSE=1, vsatp.mode=Sv39, VS-stage SS 页有效，VS-mode SSPUSH | 正常执行 |
| HCFI-SS-48 | vsatp.mode=Bare 时 ssp CSR 仍可访问 | 设 henvcfg.SSE=1, vsatp.mode=Bare, V=1，VS-mode csrr ssp | 正常读取（CSR 访问不受翻译模式影响） |

---

## Group B6. VS-mode Zicfiss 异常行为与委托

**规范依据**：
- `norm:hedeleg_op`：V=1 时已委托到 HS 的同步异常，若 hedeleg 对应位置位则进一步委托到 VS
- `norm:hedeleg_acc`：hedeleg bit 18 可写
- `norm:ss_fault_exception_code`：SS 指令异常 cause 为 7/15/23
- cfi.adoc：software-check exception (cause=18, mtval/stval=3) for shadow stack fault

**测试职责**：验证 VS-mode 下 Zicfiss 引发的各类异常（software-check exception、access-fault、page-fault、guest-page-fault）的委托链路和 CSR 写入。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCFI-SS-49 | VS-mode SSPOPCHK 不匹配触发 SS Fault | henvcfg.SSE=1，VS-mode SSPUSH 后篡改 shadow stack，再 SSPOPCHK | software-check exception (cause=18, vstval=3) |
| HCFI-SS-50 | SS Fault 委托到 VS-mode | medeleg[18]=1, hedeleg[18]=1，VS-mode 触发 SS Fault | trap 进入 VS-mode，vscause=18, vstval=3 |
| HCFI-SS-51 | SS Fault 委托到 HS-mode | medeleg[18]=1, hedeleg[18]=0，VS-mode 触发 SS Fault | trap 进入 HS-mode，scause=18, stval=3 |
| HCFI-SS-52 | SS Fault 委托到 M-mode | medeleg[18]=0，VS-mode 触发 SS Fault | trap 进入 M-mode，mcause=18, mtval=3 |
| HCFI-SS-53 | VS-mode SS access-fault 委托到 VS | medeleg[7]=1, hedeleg[7]=1，VS-mode 普通 store 写 SS 页 | trap 进入 VS-mode，vscause=7 |
| HCFI-SS-54 | VS-mode SS access-fault 委托到 HS | medeleg[7]=1, hedeleg[7]=0，VS-mode 普通 store 写 SS 页 | trap 进入 HS-mode，scause=7 |
| HCFI-SS-55 | VS-mode SS page-fault 委托到 VS | medeleg[15]=1, hedeleg[15]=1，VS-mode SS 指令访问只读页 | trap 进入 VS-mode，vscause=15 |
| HCFI-SS-56 | VS-mode SS guest-page-fault 不可委托到 VS | VS-mode SS 指令触发 G-stage guest-page-fault | trap 进入 HS-mode（hedeleg[20/21/23] 只读零），hstatus.GVA=1 |
| HCFI-SS-57 | SS Fault trap 到 VS 时 vsepc 正确 | medeleg[18]=1, hedeleg[18]=1，VS-mode 触发 SS Fault | vsepc = 触发 SSPOPCHK 指令的地址 |
| HCFI-SS-58 | SS access-fault trap 到 HS 时 sepc/scause 正确 | medeleg[7]=1, hedeleg[7]=0，VS-mode 普通 store 写 SS 页 | sepc = store 指令地址, scause=7, hstatus.SPV=1 |

---

## Group B7. VS-mode Zicfiss 功能完整性

**规范依据**：
- `norm:henvcfg_sse_op`：SSE=1 时 VS-mode Zicfiss 完全激活
- cfi.adoc：SSPUSH/SSPOPCHK/SSAMOSWAP 指令行为
- `norm:ssmp_ss_idempotent_memory`：shadow stack 内存需幂等

**测试职责**：验证 VS-mode 下 Zicfiss 的完整功能链路，包括 shadow stack 的压栈、弹栈、原子交换等操作。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCFI-SS-59 | VS-mode SSPUSH/SSPOPCHK 基本流程 | henvcfg.SSE=1，VS-mode 设置 ssp 指向 SS 页，SSPUSH 压栈后 SSPOPCHK 弹栈检查 | 匹配时无异常，正常返回 |
| HCFI-SS-60 | VS-mode C.SSPUSH/C.SSPOPCHK 基本流程 | henvcfg.SSE=1，VS-mode 使用压缩指令版本 | 正常执行 |
| HCFI-SS-61 | VS-mode SSAMOSWAP.W 原子交换 | henvcfg.SSE=1, menvcfg.SSE=1，VS-mode 在 SS 页上执行 SSAMOSWAP.W | 原子交换成功 |
| HCFI-SS-62 | VS-mode SSAMOSWAP.D 原子交换 | henvcfg.SSE=1, menvcfg.SSE=1，VS-mode 在 SS 页上执行 SSAMOSWAP.D | 原子交换成功 |
| HCFI-SS-63 | VS-mode SSPOPCHK 不匹配触发异常后 ssp 状态 | henvcfg.SSE=1，VS-mode SSPUSH 后修改 ssp 使 SSPOPCHK 不匹配 | software-check exception (cause=18, vstval=3) |
| HCFI-SS-64 | VS-mode Shadow Stack guard page 检测 | henvcfg.SSE=1，VS-mode SSPUSH 越界到非 SS 页（guard page） | store/AMO access-fault (cause=7) |
| HCFI-SS-65 | VS-mode 非幂等内存上 SS 指令触发异常 | henvcfg.SSE=1，VS-mode SSPUSH 到设备内存（非幂等） | store/AMO access-fault (cause=7) |
| HCFI-SS-66 | VS-mode SSAMOSWAP 需要 AMOSwap 级 PMA 支持 | henvcfg.SSE=1，VS-mode SSAMOSWAP.W 到不支持 AMOSwap 的内存区域 | store/AMO access-fault (cause=7) |
| HCFI-SS-67 | VU-mode SSPUSH/SSPOPCHK 基本流程 | henvcfg.SSE=1, senvcfg.SSE=1, menvcfg.SSE=1，VU-mode SSPUSH/SSPOPCHK | 正常执行 |
| HCFI-SS-68 | VS-mode ssp 在 trap 间保持 | henvcfg.SSE=1，VS-mode 设 ssp=VAL，触发 trap 到 HS-mode 后返回 | ssp 保持 VAL（ssp 是常规 CSR，不受 trap 自动保存恢复） |

---

## Group B8. henvcfg.SSE=0 时 VS-mode Zicfiss 回退行为综合

**规范依据**：
- `norm:henvcfg_sse_op`：SSE=0 时的完整回退规则列表
- cfi.adoc：Zicfiss 指令在扩展未启用时回退为 Zimop/Zcmop 行为

**测试职责**：综合验证 henvcfg.SSE=0 时 VS-mode 下所有 Zicfiss 相关行为的一致性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCFI-SS-69 | SSE=0 时 32-bit SS 指令行为 | henvcfg.SSE=0, menvcfg.SSE=1，VS-mode 逐一执行 SSPUSH/SSPOPCHK/SSAMOSWAP.W/SSAMOSWAP.D | SSPUSH/SSPOPCHK 回退为 Zimop no-op；SSAMOSWAP.W/D 触发 virtual-instruction exception (cause=22)（AMO 编码不回退，见 norm:henvcfg_sse_op） |
| HCFI-SS-70 | SSE=0 时所有 16-bit SS 指令回退 | henvcfg.SSE=0，VS-mode 逐一执行 C.SSPUSH/C.SSPOPCHK | 全部作为 Zcmop no-op 执行 |
| HCFI-SS-71 | SSE=0 时 ssp CSR 访问仍触发异常 | henvcfg.SSE=0, menvcfg.SSE=1，VS-mode csrr ssp | virtual-instruction exception (cause=22)（ssp 访问受控，非指令回退） |
| HCFI-SS-72 | SSE=0 时 SS 页编码保留（VS-stage 和 VU-stage） | henvcfg.SSE=0，VS/VU-stage 页表 pte.xwr=010 | 编码保留，访问触发 page-fault |
| HCFI-SS-73 | SSE=0→1 切换后 SS 功能立即可用 | 先设 henvcfg.SSE=0 执行 SS 指令（no-op），再设 SSE=1 执行相同指令 | SSE=1 后指令正常执行为 Zicfiss 行为 |
| HCFI-SS-74 | SSE=1→0 切换后 SS 指令回退 | 先设 henvcfg.SSE=1 执行 SS 指令正常，再设 SSE=0 执行相同指令 | SSE=0 后指令回退为 no-op |
| HCFI-SS-75 | SSE 切换不需要地址翻译缓存同步 | henvcfg.SSE 从 0 切换到 1 后不执行 SFENCE.VMA/HFENCE.VVMA，立即执行 SSPUSH | 正常执行（SSE 变化立即生效，无需缓存同步） |

---

## 测试前置依赖

### 框架层面需要补充的内容

> [!IMPORTANT]
> 以下修改是 Hypervisor × CFI 交叉测试的前置依赖。

1. **henvcfg LPE/SSE 字段宏定义**：在 `common/encoding.h` 中添加 `HENVCFG_LPE` 和 `HENVCFG_SSE` 宏。
2. **vsstatus.SPELP 字段宏定义**：在 `common/encoding.h` 中添加 `VSSTATUS_SPELP_BIT` 宏。
3. **software-check exception cause**：在 `common/encoding.h` 中添加 `CAUSE_SOFTWARE_CHECK` (18) 宏。
4. **mtval/stval/vstval CFI 编码**：添加 `SWCHECK_LANDING_PAD_FAULT` (2) 和 `SWCHECK_SHADOW_STACK_FAULT` (3) 宏。
5. **CSR_SSP 宏定义**：在 `common/encoding.h` 中添加 `CSR_SSP` (0x011) 宏。
6. **trap handler 增强**：识别 software-check exception (cause=18)，在 armed trap 场景下正确捕获并记录 `mtval`/`stval`/`vstval` 值。
7. **VS-mode CFI 测试基础设施**：在 Hypervisor 测试框架中增加对 VS-mode 下 Zicfilp/Zicfiss 指令的编译和执行支持。

### 测试文件结构

实际实现拆分为两个独立测试目录（Zicfilp 与 Zicfiss 分别构建）：

```
Hypervisor_Zicfilp/
├── Makefile
├── kernel.ld
├── main.c
└── tests/
    ├── test_helpers.h
    ├── test_hyp_zicfilp_envcfg.c     # Group A1: henvcfg.LPE 使能控制
    ├── test_hyp_zicfilp_spelp.c      # Group A2: vsstatus.SPELP 保存恢复
    ├── test_hyp_zicfilp_lpad.c       # Group A3: VS-mode Landing Pad 功能
    ├── test_hyp_zicfilp_deleg.c      # Group A4: software-check exception 委托
    └── test_hyp_zicfilp_async.c      # Group A5: 异步事件与 ELP 交互
Hypervisor_Zicfiss/
├── Makefile
├── kernel.ld
├── main.c
└── tests/
    ├── test_helpers.h
    ├── test_hyp_zicfiss_envcfg.c     # Group B1: henvcfg.SSE 使能控制
    ├── test_hyp_zicfiss_ssp_csr.c    # Group B2: ssp CSR 访问控制
    ├── test_hyp_zicfiss_ss_page.c    # Group B3: VS-stage SS 页类型
    ├── test_hyp_zicfiss_gstage.c     # Group B4: G-stage 翻译交互
    ├── test_hyp_zicfiss_bare.c       # Group B5: satp/vsatp Bare 模式
    ├── test_hyp_zicfiss_deleg.c      # Group B6: Zicfiss 异常委托
    ├── test_hyp_zicfiss_func.c       # Group B7: Zicfiss 功能完整性
    └── test_hyp_zicfiss_revert.c     # Group B8: SSE=0 回退行为
```

---

## Verification Plan

### Automated Tests
- `cd Hypervisor_Zicfilp && make clean && make qemu` — QEMU 模拟器（需支持 H 扩展 + Zicfilp）
- `cd Hypervisor_Zicfiss && make clean && make qemu` — QEMU 模拟器（需支持 H 扩展 + Zicfiss）
- `cd Hypervisor_Zicfilp && make clean && make spike` / `cd Hypervisor_Zicfiss && make clean && make spike` — Spike 模拟器
- `cd Hypervisor_Zicfilp && make clean && make sail` / `cd Hypervisor_Zicfiss && make clean && make sail` — Sail 模拟器

### Manual Verification
- 在硬件平台上通过 `make PLATFORM=haps_xiaohui` 编译后使用 `remote_debug.py` 部署测试

### 注意事项
1. 测试前需确认目标平台已实现 H 扩展和 Zicfilp/Zicfiss 扩展。
2. 若平台未实现 CFI 扩展，henvcfg.LPE/SSE 应为只读零，相关测试应报告"扩展未实现"而非失败。
3. software-check exception (cause=18) 的委托测试需确认 hedeleg bit 18 在平台上的可写性。
4. Shadow Stack 页类型测试需要 VS-stage 页表的精细控制，确保正确设置 pte.xwr 编码。
5. 不允许为了通过测试而降低 SPEC 标准。如果 QEMU/Sail/硬件的行为与 SPEC 不一致，应报告问题而非修改测试。
6. 当前已知实现状态（known gap）：
   - Group A5（HCFI-LP-31/42/43）的异步中断注入窗口（JALR 后、LPAD 解码前）非确定，当前以同步 software-check 异常作为代理验证 ELP 保存/恢复机制。
   - HCFI-SS-04/60/70（16-bit 压缩指令回退/流程）、HCFI-SS-30（真实 CBO 指令）、HCFI-SS-65/66（非幂等内存 / AMOSwap PMA）暂未直接实现，以注释标注为 known gap。
7. 当前 QEMU（cskysim）已确认的 SPEC 偏差（测试保持 FAIL 以报告问题，不做 workaround）：
   - VS→HS trap 时 ELP 被保存到 vsstatus.SPELP 而非 mstatus.SPELP（影响 HCFI-LP-15/16/17/31/42）。
   - VS-mode xLPE 被 menvcfg.LPE 门控（SPEC 规定 VS xLPE = henvcfg.LPE 独立生效，影响 HCFI-LP-09）。
