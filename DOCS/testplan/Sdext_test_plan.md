# Sdext 扩展测试计划（Core Debug）

> 本文档描述 Sdext（ISA Extension for External Debug）扩展的测试计划。聚焦于 Debug Mode 行为、Core Debug CSR（`dcsr`/`dpc`/`dscratch0`/`dscratch1`）访问限制、ebreak 默认行为、Sdtrig 依赖、Halt/Resume 状态管理、单步执行（Single Step）、Debug Mode 下指令行为、以及计数器/定时器控制。
>
> 生成时间：2026-07-01

---

## 概述

Sdext 是 RISC-V 外部调试 ISA 扩展。它为外部调试器（Debug Module, DM）提供必要的处理器端支持，包括特殊的 Debug Mode 执行模式、少量调试 CSR 以及与 DM 配合的 halt/resume/single-step 机制。Sdext 的实现是使外部调试正常工作的必要条件。

### 测试可行性说明

Sdext 的大部分功能需要 hart 处于 Debug Mode，而进入 Debug Mode 需要外部调试器（DM）配合。本框架为裸机测试环境，无法直接控制 Debug Mode 的进入和退出。因此测试分为两类：

- **裸机可测试**（Group 1-3）：验证 Debug CSR 访问限制、ebreak 默认行为、Sdtrig 依赖关系
- **需外部调试器配合**（Group 4-7）：验证 Halt 进入状态、Resume 行为、单步执行、Debug Mode 下指令行为。此类测试在裸机环境下 TEST_SKIP，需配合 OpenOCD/GDB 等外部调试工具验证

### 本文档覆盖的 SPEC 章节

- Debug Mode 定义与行为规则（Debug Mode 下指令行为、中断屏蔽、陷阱处理、触发器行为）
- Core Debug CSR（`dcsr`/`dpc`/`dscratch0`/`dscratch1`）访问限制
- `ebreak` 默认行为（`dcsr.ebreakm`/`ebreaks`/`ebreaku` 默认值 = 0）
- Sdtrig 依赖关系
- Halt 进入（`dcsr.cause`/`dcsr.prv`/`dcsr.v`/`dpc` 记录）
- Resume 行为（PC/特权级恢复、MPRV/MDT/SDT 清除）
- 单步执行（`dcsr.step`）
- Debug Mode 下指令行为（wfi/wrs/ecall/mret/控制转移/auipc 等）
- 计数器/定时器控制（`dcsr.stopcount`/`dcsr.stoptime`）

### 由其他测试计划覆盖

- Trigger Module（Sdtrig）功能测试 → `Sdtrig_test_plan.md`（若存在）
- Smctr Debug Mode 录制抑制 → `Smctr_test_plan.md` Group 4
- Hypervisor × Debug 交叉测试（VS/VU-mode ebreak、`dcsr.ebreakvs`/`ebreakvu`） → `Hypervisor_cross_test_plan.md`
- Zicfilp 完整测试 → `cfi_test_plan.md`

---

## 覆盖的规范点

本章节列出本文档 Groups 1-7 所有测试组中引用的规范点（norm ID），已去重并按字母顺序排列。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:debug_mode_behavior` | All implemented instructions operate just as they do in M-mode, unless an exception is mentioned. All operations are executed with machine mode privilege. | Debug Mode 下所有已实现指令按 M-mode 方式执行，除非规范中另有说明。所有操作以 M-mode 特权级执行。 |
| `norm:debug_mode_interrupts` | All interrupts (including NMI) are masked. | Debug Mode 下所有中断（包括 NMI）被屏蔽。 |
| `norm:debug_mode_traps` | Traps don't take place. Instead, they end execution of the program buffer and the hart remains in Debug Mode. They do not update registers such as `mepc`, `mcause`, `mtval`, `mtval2`, and `mtinst`. | Debug Mode 下陷阱不发生。陷阱结束 program buffer 执行，hart 保持在 Debug Mode。不更新 `mepc`/`mcause`/`mtval` 等寄存器。 |
| `norm:debug_mode_triggers` | Triggers don't match or fire. | Debug Mode 下触发器不匹配也不触发。 |
| `norm:debug_mode_wfi_wrs` | Instructions that place the hart into a stalled state act as a `nop`. This includes `wfi`, `wrs.sto`, and `wrs.nto`. | 使 hart 进入停滞状态的指令作为 `nop` 执行。包括 `wfi`、`wrs.sto` 和 `wrs.nto`。 |
| `norm:debug_mode_priv_change` | Almost all instructions that change the privilege mode have UNSPECIFIED behavior. This includes `ecall`, `mret`, `sret`, and `uret`. The only exception is `ebreak`, which ends execution of the Program Buffer when executed. | 几乎所有改变特权级的指令行为为 UNSPECIFIED。包括 `ecall`、`mret`、`sret` 和 `uret`。唯一例外是 `ebreak`，执行时结束 Program Buffer。 |
| `norm:debug_mode_control_transfer` | All control transfer instructions may act as illegal instructions if their destination is in the Program Buffer. All control transfer instructions may act as illegal instructions if their destination is outside the Program Buffer. | 所有控制转移指令的目的地在 Program Buffer 内时可能作为非法指令。目的地在 Program Buffer 外时也可能作为非法指令。 |
| `norm:debug_mode_auipc` | Instructions that depend on the value of the PC (e.g. `auipc`) may act as illegal instructions. | 依赖 PC 值的指令（如 `auipc`）可能作为非法指令。 |
| `norm:debug_mode_zicfilp` | When the Zicfilp extension is implemented, the `ELP` state is `NO_LP_EXPECTED` and is not updated by any instructions. LPAD instruction executes as a no-op. | 实现 Zicfilp 扩展时，`ELP` 状态为 `NO_LP_EXPECTED` 且不被任何指令更新。LPAD 指令作为 no-op 执行。 |
| `norm:debug_mode_dxlen` | Effective XLEN is DXLEN. | Debug Mode 下有效 XLEN 为 DXLEN。 |
| `norm:core_debug_csr_access` | The supported Core Debug Registers must be implemented for each hart that can be debugged. Attempts to access an unimplemented Core Debug Register raise an illegal instruction exception. These registers are only accessible from Debug Mode. | 每个可调试的 hart 必须实现所支持的 Core Debug 寄存器。尝试访问未实现的 Core Debug 寄存器触发 illegal instruction 异常。这些寄存器仅在 Debug Mode 下可访问。 |
| `norm:single_step` | An external debugger can cause a halted hart to execute a single instruction or trap and then re-enter Debug Mode by setting `dcsr.step` before resuming. | 外部调试器可在 resume 前设置 `dcsr.step`，使暂停的 hart 执行单条指令或陷阱后重新进入 Debug Mode。 |
| `norm:single_step_trap` | If control is transferred to a trap handler while executing the instruction, then Debug Mode is re-entered immediately after the PC is changed to the trap handler, and the appropriate `tval` and `cause` registers are updated. | 若执行指令时发生陷阱，Debug Mode 在 PC 更改为陷阱处理程序后立即重新进入，并更新相应的 `tval` 和 `cause` 寄存器。 |
| `norm:single_step_trigger` | If executing or fetching the instruction causes a trigger to fire with action=1, Debug Mode is re-entered immediately after that trigger has fired. `dcsr.cause` is set to 2 (trigger) instead of 4 (single step). | 若执行指令触发 action=1 的触发器，Debug Mode 在触发器触发后立即重新进入。`dcsr.cause` 设为 2（trigger）而非 4（step）。 |
| `norm:single_step_wfi` | If the instruction being stepped over would normally stall the hart, then instead the instruction is treated as a `nop`. This includes `wfi`, `wrs.sto`, and `wrs.nto`. | 若单步执行的指令通常会使 hart 停滞，则该指令作为 `nop` 处理。包括 `wfi`、`wrs.sto` 和 `wrs.nto`。 |
| `norm:single_step_stepie` | `dcsr.stepie`: 0=interrupts disabled during single stepping. 1=interrupts enabled during single stepping. May be hardwired to 0. | `dcsr.stepie`：0=单步执行时中断被屏蔽。1=单步执行时中断被使能。可硬连线为 0。 |
| `norm:halt_entry` | When a hart halts: `dcsr.cause` is updated; `dcsr.prv` and `dcsr.v` are set to reflect current privilege mode; `dcsr.pelp` is set to current ELP state (Zicfilp); `dpc` is set to the next instruction that should be executed; the hart enters Debug Mode. | 当 hart 暂停时：更新 `dcsr.cause`；设置 `dcsr.prv` 和 `dcsr.v` 反映当前特权级；设置 `dcsr.pelp` 为当前 ELP 状态；设置 `dpc` 为下一条应执行的指令；进入 Debug Mode。 |
| `norm:halt_vector_partial` | If the current instruction can be partially executed and should be restarted to complete, then the relevant state for that is updated. E.g. if a halt occurs during a partially executed vector instruction, then `vstart` is updated, and `dpc` is updated to the address of the partially executed instruction. | 若当前指令可部分执行并需要重新启动完成，则更新相关状态。如向量指令部分执行时暂停，更新 `vstart`，`dpc` 更新为部分执行指令的地址。 |
| `norm:resume_behavior` | When a hart resumes: `pc` changes to `dpc`; privilege mode changed to `dcsr.prv`; `MPRV` in `mstatus` is cleared if new privilege mode is less privileged than M-mode; the hart is no longer in debug mode. | 当 hart 恢复时：`pc` 变为 `dpc`；特权级变为 `dcsr.prv`；若新特权级低于 M-mode 则清除 `MPRV`；hart 不再处于 debug mode。 |
| `norm:resume_smdbltrp` | If the Smdbltrp extension is implemented and the new privilege mode is not M, then the `MDT` bit is set to 0. | 若实现 Smdbltrp 扩展且新特权级非 M，则 `MDT` 位设为 0。 |
| `norm:resume_ssdbltrp` | If the Ssdbltrp extension is implemented and the new privilege mode is U, VS, or VU, then `sstatus.SDT` is set to 0. Additionally, if it is VU, then `vsstatus.SDT` is also set to 0. | 若实现 Ssdbltrp 扩展且新特权级为 U/VS/VU，则 `sstatus.SDT` 设为 0。若为 VU，`vsstatus.SDT` 也设为 0。 |
| `norm:halt_resume_zicfilp` | Halt: `dcsr.pelp` set to current ELP state, ELP set to `NO_LP_EXPECTED`. Resume: ELP changed to `dcsr.pelp` if Zicfilp enabled at new privilege mode, else `NO_LP_EXPECTED`. `dcsr.pelp` set to `NO_LP_EXPECTED`. | 暂停时：`dcsr.pelp` 保存当前 ELP 状态，ELP 设为 `NO_LP_EXPECTED`。恢复时：若新特权级启用 Zicfilp，ELP 变为 `dcsr.pelp`，否则 `NO_LP_EXPECTED`。`dcsr.pelp` 设为 `NO_LP_EXPECTED`。 |
| `norm:reset_halt` | If the halt signal or `dmstatus.hasresethaltreq` are asserted when a hart comes out of reset, the hart must enter Debug Mode before executing any instructions, but after performing any initialization. | 若 hart 退出复位时 halt 信号或 `dmstatus.hasresethaltreq` 被置位，hart 必须在执行任何指令前（但完成初始化后）进入 Debug Mode。 |
| `norm:lr_sc_reservation` | The reservation registered by an `lr` instruction on a memory address may be lost when entering Debug Mode or while in Debug Mode. | 进入 Debug Mode 时或 Debug Mode 期间，`lr` 指令注册的内存地址预留可能丢失。 |
| `norm:wfi_halt_completion` | If halt is requested while `wfi` is executing, then the hart must leave the stalled state, completing this instruction's execution, and then enter Debug Mode. | 若 `wfi` 执行期间收到 halt 请求，hart 必须退出停滞状态、完成该指令执行，然后进入 Debug Mode。 |
| `norm:wrs_halt_completion` | If halt is requested while `wrs.sto` or `wrs.nto` is executing, then the hart must leave the stalled state, completing this instruction's execution, and then enter Debug Mode. | 若 `wrs.sto` 或 `wrs.nto` 执行期间收到 halt 请求，hart 必须退出停滞状态、完成该指令执行，然后进入 Debug Mode。 |
| `norm:dcsr_cause_priority` | Priority of reasons for entering Debug Mode (highest to lowest): resethaltreq(5) > halt group(6) > haltreq(3) > trigger(2) > ebreak(1) > step(4). | 进入 Debug Mode 原因的优先级（从高到低）：resethaltreq(5) > halt group(6) > haltreq(3) > trigger(2) > ebreak(1) > step(4)。 |
| `norm:dpc_writability` | The writability of `dpc` follows the same rules as `mepc` from the Privileged Spec. | `dpc` 的可写性遵循特权级规范中 `mepc` 的相同规则。 |
| `norm:dscratch_optional` | `dscratch0` and `dscratch1` are optional scratch registers. | `dscratch0` 和 `dscratch1` 是可选的暂存寄存器。 |
| `norm:dcsr_ebreakm` | `dcsr.ebreakm`: 0=ebreak in M-mode behaves per Privileged Spec. 1=ebreak in M-mode enters Debug Mode. | `dcsr.ebreakm`：0=M-mode 下 ebreak 按特权级规范行为。1=M-mode 下 ebreak 进入 Debug Mode。 |
| `norm:dcsr_ebreaks` | `dcsr.ebreaks`: 0=ebreak in S-mode behaves per Privileged Spec. 1=ebreak in S-mode enters Debug Mode. Hardwired to 0 if no S-mode. | `dcsr.ebreaks`：0=S-mode 下 ebreak 按特权级规范行为。1=S-mode 下 ebreak 进入 Debug Mode。无 S-mode 时硬连线为 0。 |
| `norm:dcsr_ebreaku` | `dcsr.ebreaku`: 0=ebreak in U-mode behaves per Privileged Spec. 1=ebreak in U-mode enters Debug Mode. Hardwired to 0 if no U-mode. | `dcsr.ebreaku`：0=U-mode 下 ebreak 按特权级规范行为。1=U-mode 下 ebreak 进入 Debug Mode。无 U-mode 时硬连线为 0。 |
| `norm:dcsr_stopcount` | `dcsr.stopcount`: 0=increment counters as usual. 1=freeze all hart-local counters while in Debug Mode. May be hardwired to 0 or 1. | `dcsr.stopcount`：0=正常递增计数器。1=Debug Mode 下冻结所有 hart 本地计数器。可硬连线为 0 或 1。 |
| `norm:dcsr_stoptime` | `dcsr.stoptime`: 0=time reflects mtime. 1=time frozen at Debug Mode entry. May be hardwired to 0 or 1. | `dcsr.stoptime`：0=time 反映 mtime。1=Debug Mode 入口时冻结 time。可硬连线为 0 或 1。 |
| `norm:dcsr_mprven` | `dcsr.mprven`: 0=mprv in mstatus ignored in Debug Mode. 1=mprv takes effect in Debug Mode. May be tied to either 0 or 1. | `dcsr.mprven`：0=Debug Mode 下忽略 mstatus 中的 mprv。1=mprv 在 Debug Mode 下生效。可连线为 0 或 1。 |
| `norm:sdtrig_dependency` | If Sdext is implemented and Sdtrig is not implemented, then accessing any of the Sdtrig CSRs must raise an illegal instruction exception. | 若实现 Sdext 但未实现 Sdtrig，则访问任何 Sdtrig CSR 必须触发 illegal instruction 异常。 |

---

## CSR 表

| 编号 | 特权级 | 宽度 | 名称 | 描述 |
|------|--------|------|------|------|
| 0x7b0 | Debug Mode only | DXLEN | `dcsr` | Debug Control and Status Register — 调试控制与状态寄存器 |
| 0x7b1 | Debug Mode only | DXLEN | `dpc` | Debug PC — 调试程序计数器 |
| 0x7b2 | Debug Mode only | DXLEN | `dscratch0` | Debug Scratch Register 0（可选） |
| 0x7b3 | Debug Mode only | DXLEN | `dscratch1` | Debug Scratch Register 1（可选） |

> **注意**：以上 CSR 仅在 Debug Mode 下可访问。从 M-mode / S-mode / U-mode 访问均触发 illegal instruction 异常（cause=2）。

---

## Group 1. Debug CSR 访问限制

**规范依据**：
- `norm:core_debug_csr_access`：Core Debug 寄存器仅在 Debug Mode 下可访问

**测试职责**：验证 `dcsr`/`dpc`/`dscratch0`/`dscratch1` 在非 Debug Mode（M-mode / S-mode / U-mode）下访问时触发 illegal instruction 异常（cause=2）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SDEX-CSR-01 | M-mode 读 dcsr | M-mode 执行 `csrr t0, dcsr` | 触发 illegal instruction 异常（cause=2） |
| SDEX-CSR-02 | M-mode 写 dcsr | M-mode 执行 `csrw dcsr, t0` | 触发 illegal instruction 异常（cause=2） |
| SDEX-CSR-03 | S-mode 读 dcsr | S-mode 执行 `csrr t0, dcsr` | 触发 illegal instruction 异常（cause=2） |
| SDEX-CSR-04 | S-mode 写 dcsr | S-mode 执行 `csrw dcsr, t0` | 触发 illegal instruction 异常（cause=2） |
| SDEX-CSR-05 | U-mode 读 dcsr | U-mode 执行 `csrr t0, dcsr` | 触发 illegal instruction 异常（cause=2） |
| SDEX-CSR-06 | U-mode 写 dcsr | U-mode 执行 `csrw dcsr, t0` | 触发 illegal instruction 异常（cause=2） |
| SDEX-CSR-07 | M-mode 读 dpc | M-mode 执行 `csrr t0, dpc` | 触发 illegal instruction 异常（cause=2） |
| SDEX-CSR-08 | M-mode 写 dpc | M-mode 执行 `csrw dpc, t0` | 触发 illegal instruction 异常（cause=2） |
| SDEX-CSR-09 | S-mode 读写 dpc | S-mode 读写 dpc | 触发 illegal instruction 异常（cause=2） |
| SDEX-CSR-10 | U-mode 读写 dpc | U-mode 读写 dpc | 触发 illegal instruction 异常（cause=2） |
| SDEX-CSR-11 | M-mode 读 dscratch0 | M-mode 执行 `csrr t0, dscratch0` | 触发 illegal instruction 异常（cause=2） |
| SDEX-CSR-12 | M-mode 写 dscratch0 | M-mode 执行 `csrw dscratch0, t0` | 触发 illegal instruction 异常（cause=2） |
| SDEX-CSR-13 | M-mode 读 dscratch1 | M-mode 执行 `csrr t0, dscratch1` | 触发 illegal instruction 异常（cause=2） |
| SDEX-CSR-14 | M-mode 写 dscratch1 | M-mode 执行 `csrw dscratch1, t0` | 触发 illegal instruction 异常（cause=2） |

> [!NOTE]
> - 所有测试均可在裸机环境下执行，无需外部调试器。
> - S-mode 和 U-mode 测试使用 `goto_priv()` 切换特权级，通过 `PRIV_DO` 或 `EXPECT_TRAP` 宏执行 CSR 访问指令。
> - 即使 Sdext 未实现，访问这些 CSR 地址（0x7b0-0x7b3）也应触发 illegal instruction 异常（因为这些 CSR 地址不存在于标准 M-mode CSR 空间中）。因此此组测试同时验证了 Sdext 实现的存在性和访问限制的正确性。
> - 若平台将 debug CSR 地址映射为其他用途（不符合规范），此组测试将 FAIL，应报告实现问题。

```c
TEST_REGISTER(test_sdex_csr_01);
bool test_sdex_csr_01(void)
{
    TEST_BEGIN("SDEX-CSR-01: M-mode read dcsr triggers illegal instruction");

    EXPECT_TRAP(CAUSE_ILLEGAL_INSTRUCTION, {
        CSRR(0x7b0);  /* dcsr */
    });

    TEST_END();
}
```

---

## Group 2. ebreak 默认行为

**规范依据**：
- `norm:dcsr_ebreakm`：`dcsr.ebreakm` 默认为 0，ebreak 在 M-mode 按特权级规范行为（触发 breakpoint 异常）
- `norm:dcsr_ebreaks`：`dcsr.ebreaks` 默认为 0，ebreak 在 S-mode 按特权级规范行为
- `norm:dcsr_ebreaku`：`dcsr.ebreaku` 默认为 0，ebreak 在 U-mode 按特权级规范行为
- `norm:debug_mode_priv_change`：`ebreak` 是唯一在 Debug Mode 下有明确定义行为的特权级变更指令

**测试职责**：验证 `dcsr.ebreakm`/`ebreaks`/`ebreaku` 均为默认值 0 时，ebreak 指令在各特权级下触发 breakpoint 异常（cause=3），而非进入 Debug Mode。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SDEX-EBRK-01 | M-mode ebreak 默认行为 | M-mode 执行 ebreak 指令 | 触发 breakpoint 异常（cause=3），mepc 指向 ebreak 地址 |
| SDEX-EBRK-02 | S-mode ebreak 默认行为 | S-mode 执行 ebreak 指令 | 触发 breakpoint 异常（cause=3），sepc 指向 ebreak 地址 |
| SDEX-EBRK-03 | U-mode ebreak 默认行为 | U-mode 执行 ebreak 指令 | 触发 breakpoint 异常（cause=3），异常正确路由 |
| SDEX-EBRK-04 | c.ebreak 压缩指令行为 | M-mode 执行 c.ebreak 指令 | 触发 breakpoint 异常（cause=3），行为与 ebreak 一致 |
| SDEX-EBRK-05 | mepc 指向 ebreak 地址 | M-mode ebreak 后检查 mepc | mepc == ebreak 指令的虚拟地址 |
| SDEX-EBRK-06 | mcause 包含 breakpoint | M-mode ebreak 后检查 mcause | mcause == 3（CAUSE_BREAKPOINT） |
| SDEX-EBRK-07 | ebreak 进入 Debug Mode（DM） | 外部调试器设 dcsr.ebreakm=1 后，M-mode 执行 ebreak | hart 进入 Debug Mode，dcsr.cause=1(ebreak)（需外部调试器） |
| SDEX-EBRK-08 | S-mode ebreak 进入 Debug Mode（DM） | 外部调试器设 dcsr.ebreaks=1 后，S-mode 执行 ebreak | hart 进入 Debug Mode，dcsr.cause=1(ebreak)（需外部调试器） |

> [!NOTE]
> - SDEX-EBRK-01 至 SDEX-EBRK-06 可在裸机环境下执行。`dcsr.ebreakm`/`ebreaks`/`ebreaku` 复位后均为 0，因此 ebreak 不会进入 Debug Mode，而是按特权级规范触发 breakpoint 异常。
> - SDEX-EBRK-07/08 需外部调试器设置 `dcsr.ebreakm`/`ebreaks` 为 1，在裸机环境下 TEST_SKIP。
> - S-mode ebreak 的路由取决于 `medeleg` 中的 breakpoint 位。若 `medeleg[3]=0`，S-mode ebreak 陷阱到 M-mode；若 `medeleg[3]=1`，陷阱到 S-mode。测试前需确认 `medeleg` 配置。

```c
TEST_REGISTER(test_sdex_ebrk_01);
bool test_sdex_ebrk_01(void)
{
    TEST_BEGIN("SDEX-EBRK-01: M-mode ebreak default behavior");

    EXPECT_TRAP(CAUSE_BREAKPOINT, {
        asm volatile("ebreak");
    });

    uintptr_t cause = trap_get_cause();
    TEST_ASSERT_EQ("ebreak cause", cause, CAUSE_BREAKPOINT);

    TEST_END();
}
```

---

## Group 3. Sdtrig 依赖关系

**规范依据**：
- `norm:sdtrig_dependency`：若实现 Sdext 但未实现 Sdtrig，访问 Sdtrig CSR 触发 illegal instruction 异常

**测试职责**：验证 Sdext 实现但 Sdtrig 未实现时，访问 trigger module CSR（`tselect`/`tdata1`/`tdata2`/`tdata3`/`tinfo`）触发 illegal instruction 异常。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SDEX-SDTRIG-01 | Sdtrig 实现检测 | M-mode 尝试读写 tselect，检测 Sdtrig 是否实现 | 确定 Sdtrig 实现状态（trapped → 未实现，成功 → 已实现） |
| SDEX-SDTRIG-02 | 无 Sdtrig 时 tselect 访问 | Sdext 实现但 Sdtrig 未实现时，M-mode 读 tselect(0x7a0) | 触发 illegal instruction 异常（cause=2） |
| SDEX-SDTRIG-03 | 无 Sdtrig 时 tdata1 访问 | Sdext 实现但 Sdtrig 未实现时，M-mode 读 tdata1(0x7a1) | 触发 illegal instruction 异常（cause=2） |
| SDEX-SDTRIG-04 | 无 Sdtrig 时 tdata2 访问 | Sdext 实现但 Sdtrig 未实现时，M-mode 读 tdata2(0x7a2) | 触发 illegal instruction 异常（cause=2） |
| SDEX-SDTRIG-05 | 无 Sdtrig 时 tdata3 访问 | Sdext 实现但 Sdtrig 未实现时，M-mode 读 tdata3(0x7a3) | 触发 illegal instruction 异常（cause=2） |
| SDEX-SDTRIG-06 | 无 Sdtrig 时 tinfo 访问 | Sdext 实现但 Sdtrig 未实现时，M-mode 读 tinfo(0x7a4) | 触发 illegal instruction 异常（cause=2） |

> [!NOTE]
> - 本组测试的前提条件：Sdext 已实现。若 Sdext 也未实现，所有测试 TEST_SKIP。
> - 若 Sdtrig 已实现（SDEX-SDTRIG-01 检测通过），SDEX-SDTRIG-02 至 SDEX-SDTRIG-06 应 TEST_SKIP（规范仅要求 Sdext+Sdtrig 不存在时触发异常）。
> - 若 Sdtrig 未实现，trigger CSR 地址（0x7a0-0x7a4）的访问必须触发 illegal instruction，这是 Sdext 规范的强制要求。
> - S-mode 和 U-mode 下的 trigger CSR 访问也应触发 illegal instruction（与 M-mode 一致），但此处仅测试 M-mode 以简化。

---

## Group 4. Halt 进入与 dcsr 状态记录

**规范依据**：
- `norm:halt_entry`：暂停时更新 `dcsr.cause`/`dcsr.prv`/`dcsr.v`/`dcsr.pelp`/`dpc`
- `norm:halt_vector_partial`：向量指令部分执行时的状态更新
- `norm:dcsr_cause_priority`：进入 Debug Mode 原因的优先级
- `norm:dcsr_ebreakm` / `norm:dcsr_ebreaks` / `norm:dcsr_ebreaku`：ebreak 进入 Debug Mode
- `norm:reset_halt`：复位暂停行为

**测试职责**：验证通过各种方式进入 Debug Mode 时，`dcsr` 各字段和 `dpc` 被正确记录。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SDEX-HALT-01 | dcsr.cause=haltreq | 外部调试器发送 halt 请求，hart 暂停后读 dcsr.cause | dcsr.cause == 3（haltreq）或 5（resethaltreq） |
| SDEX-HALT-02 | dcsr.cause=ebreak | 设 dcsr.ebreakm=1，执行 ebreak 进入 Debug Mode 后读 dcsr.cause | dcsr.cause == 1（ebreak） |
| SDEX-HALT-03 | dcsr.cause=trigger | 配置 trigger action=1，触发后进入 Debug Mode 读 dcsr.cause | dcsr.cause == 2（trigger） |
| SDEX-HALT-04 | dcsr.cause=step | 设 dcsr.step=1，单步执行后读 dcsr.cause | dcsr.cause == 4（step） |
| SDEX-HALT-05 | dcsr.cause=resethaltreq | Halt-on-reset 使能后复位，读 dcsr.cause | dcsr.cause == 5（resethaltreq）或 3（haltreq） |
| SDEX-HALT-06 | dcsr.cause 优先级 | 同时触发多种 halt 原因（如 haltreq + trigger），验证 cause 值 | cause 反映最高优先级原因（按规范优先级表） |
| SDEX-HALT-07 | dcsr.prv 记录 M-mode | M-mode 下进入 Debug Mode，读 dcsr.prv | dcsr.prv == 3（M-mode） |
| SDEX-HALT-08 | dcsr.prv 记录 S-mode | S-mode 下进入 Debug Mode，读 dcsr.prv | dcsr.prv == 1（S-mode） |
| SDEX-HALT-09 | dcsr.prv 记录 U-mode | U-mode 下进入 Debug Mode，读 dcsr.prv | dcsr.prv == 0（U-mode） |
| SDEX-HALT-10 | dpc 指向下一条指令 | haltreq 暂停后检查 dpc | dpc 指向暂停时应执行的下一条指令的虚拟地址 |
| SDEX-HALT-11 | dpc ebreak 地址 | ebreak 进入 Debug Mode 后检查 dpc | dpc 指向 ebreak 指令本身的地址 |
| SDEX-HALT-12 | dpc 单步地址 | dcsr.step=1 单步执行后检查 dpc | dpc 指向单步执行后下一条应执行的指令地址 |
| SDEX-HALT-13 | dcsr.debugver 值 | Debug Mode 下读 dcsr.debugver[31:28] | debugver == 4（v1.0）或 15（custom）或 0（无调试支持） |
| SDEX-HALT-14 | dcsr.v 无虚拟化 | 无 H 扩展时，读 dcsr.v | dcsr.v == 0（硬连线） |
| SDEX-HALT-15 | 向量指令部分执行时 dpc/vstart | 向量指令部分执行期间 halt，检查 dpc 和 vstart | dpc 指向部分执行的向量指令地址，vstart 反映已处理元素 |

> [!NOTE]
> - **所有测试均需外部调试器配合**。在裸机环境下 TEST_SKIP。
> - SDEX-HALT-05（resethaltreq）需要 DM 在复位前设置 halt-on-reset，复位后 hart 自动进入 Debug Mode。规范要求 cause=5 或 3 均可接受。
> - SDEX-HALT-06 需要能够同时触发多个 halt 原因的复杂调试环境。
> - SDEX-HALT-14 的 dcsr.v 仅在实现 H 扩展时可测试非零值。
> - SDEX-HALT-15 需要向量扩展（V 扩展）支持。

---

## Group 5. Resume 行为

**规范依据**：
- `norm:resume_behavior`：恢复时 PC=dpc，特权级=dcsr.prv，MPRV 条件清除
- `norm:resume_smdbltrp`：恢复至非 M-mode 时清除 MDT
- `norm:resume_ssdbltrp`：恢复至 U/VS/VU-mode 时清除 SDT
- `norm:halt_resume_zicfilp`：ELP 状态保存与恢复
- `norm:dpc_writability`：dpc 可写性与 mepc 相同

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SDEX-RSM-01 | Resume PC = dpc | Debug Mode 下写 dpc 为目标地址，resume 后检查 PC | PC == 写入的 dpc 值 |
| SDEX-RSM-02 | Resume 到 M-mode | dcsr.prv=3，resume 后检查特权级 | 当前特权级为 M-mode |
| SDEX-RSM-03 | Resume 到 S-mode | dcsr.prv=1，resume 后检查特权级 | 当前特权级为 S-mode |
| SDEX-RSM-04 | Resume 到 U-mode | dcsr.prv=0，resume 后检查特权级 | 当前特权级为 U-mode |
| SDEX-RSM-05 | Resume 到低特权级清除 MPRV | dcsr.prv=1（S-mode），mstatus.MPRV=1，resume 后检查 MPRV | MPRV == 0（被清除） |
| SDEX-RSM-06 | Resume 到 M-mode 不清除 MPRV | dcsr.prv=3（M-mode），mstatus.MPRV=1，resume 后检查 MPRV | MPRV == 1（保留不变） |
| SDEX-RSM-07 | Resume 到非 M 清除 MDT（Smdbltrp） | 实现 Smdbltrp，dcsr.prv=1，mstatus.MDT=1，resume 后检查 MDT | MDT == 0（被清除） |
| SDEX-RSM-08 | Resume 到 M 不清除 MDT（Smdbltrp） | 实现 Smdbltrp，dcsr.prv=3，mstatus.MDT=1，resume 后检查 MDT | MDT == 1（保留不变） |
| SDEX-RSM-09 | Resume 到 U 清除 SDT（Ssdbltrp） | 实现 Ssdbltrp，dcsr.prv=0，sstatus.SDT=1，resume 后检查 SDT | SDT == 0（被清除） |
| SDEX-RSM-10 | Zicfilp ELP 暂停保存与恢复 | 实现 Zicfilp，ELP=LP_EXPECTED 时 halt，检查 dcsr.pelp；resume 后检查 ELP | halt 时 dcsr.pelp=1(LP_EXPECTED)，ELP=NO_LP_EXPECTED；resume 后 ELP 恢复为 LP_EXPECTED（若新特权级启用 Zicfilp） |
| SDEX-RSM-11 | dpc 可写性验证 | Debug Mode 下写 dpc 各种地址值，读回验证 | dpc 读写一致，遵循 mepc 相同的可写性规则（如低位对齐 IALIGN） |

> [!NOTE]
> - **所有测试均需外部调试器配合**。在裸机环境下 TEST_SKIP。
> - SDEX-RSM-05/06 验证 MPRV 条件清除逻辑：仅当 resume 到比 M-mode 低的特权级时才清除。
> - SDEX-RSM-07/08 需要 Smdbltrp 扩展支持。若平台未实现 Smdbltrp，TEST_SKIP。
> - SDEX-RSM-09 需要 Ssdbltrp 扩展支持。若平台未实现 Ssdbltrp，TEST_SKIP。
> - SDEX-RSM-10 需要 Zicfilp 扩展支持。若平台未实现 Zicfilp，TEST_SKIP。
> - SDEX-RSM-11 验证 dpc 的可写性规则与 mepc 一致（如 RV64 上低 2 位应为 0 当 IALIGN=32）。

---

## Group 6. 单步执行（Single Step）

**规范依据**：
- `norm:single_step`：dcsr.step 使 hart 执行单条指令后重新进入 Debug Mode
- `norm:single_step_trap`：单步执行时发生陷阱，Debug Mode 在 PC 更改为陷阱处理程序后重新进入
- `norm:single_step_trigger`：单步执行时触发 trigger，cause 为 trigger(2) 而非 step(4)
- `norm:single_step_wfi`：单步执行停滞指令作为 nop 处理
- `norm:single_step_stepie`：stepie 控制单步执行时的中断屏蔽

**测试职责**：验证 `dcsr.step` 单步执行机制及各种边界情况。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SDEX-STEP-01 | 单步执行单条指令 | 设 dcsr.step=1，resume 执行一条普通指令 | hart 执行一条指令后重新进入 Debug Mode，dcsr.cause=4(step) |
| SDEX-STEP-02 | 单步执行时中断屏蔽（stepie=0） | dcsr.step=1, dcsr.stepie=0，有中断挂起 | 中断在单步执行期间被屏蔽，单步完成后中断可被传递 |
| SDEX-STEP-03 | 单步执行时中断使能（stepie=1） | dcsr.step=1, dcsr.stepie=1，有中断挂起 | 中断可在单步执行期间传递。若中断触发，Debug Mode 在 trap handler 入口处重新进入 |
| SDEX-STEP-04 | 单步执行陷阱处理 | dcsr.step=1，执行的指令触发异常（如非法指令） | Debug Mode 在 trap handler 入口 PC 处重新进入，tval/cause 寄存器已更新 |
| SDEX-STEP-05 | 单步执行时 trigger 优先级 | dcsr.step=1，同时配置 trigger action=1 | dcsr.cause=2(trigger) 而非 4(step)，trigger 优先于 step |
| SDEX-STEP-06 | 单步执行 wfi | dcsr.step=1，执行 wfi | wfi 作为 nop 处理，hart 立即重新进入 Debug Mode |
| SDEX-STEP-07 | 单步执行 wrs.sto | dcsr.step=1，执行 wrs.sto | wrs.sto 作为 nop 处理，hart 立即重新进入 Debug Mode |
| SDEX-STEP-08 | 单步执行 wrs.nto | dcsr.step=1，执行 wrs.nto | wrs.nto 作为 nop 处理，hart 立即重新进入 Debug Mode |
| SDEX-STEP-09 | 单步执行后 PC 正确 | dcsr.step=1，执行一条 32-bit 顺序指令，检查 dpc | dpc == 原 PC + 4（下一条指令地址） |
| SDEX-STEP-10 | 单步执行跳转指令 | dcsr.step=1，执行一条 JAL 指令 | dpc == JAL 的目标地址 |

> [!NOTE]
> - **所有测试均需外部调试器配合**。在裸机环境下 TEST_SKIP。
> - SDEX-STEP-02/03：`dcsr.stepie` 可能被硬连线为 0。若 stepie 不可写，SDEX-STEP-03 应 TEST_SKIP。
> - SDEX-STEP-05：根据规范，trigger（cause=2）优先级高于 step（cause=4），因此同时触发时 cause 应为 2。
> - SDEX-STEP-04：陷阱触发时，hart 在 trap handler 入口处重新进入 Debug Mode，不执行任何 trap handler 代码。
> - SDEX-STEP-09/10 验证单步执行后 dpc 指向正确的下一条指令。

---

## Group 7. Debug Mode 指令行为与计数器控制

**规范依据**：
- `norm:debug_mode_wfi_wrs`：wfi/wrs.sto/wrs.nto 在 Debug Mode 下作为 nop
- `norm:debug_mode_priv_change`：ecall/mret/sret/uret 行为 UNSPECIFIED（ebreak 除外）
- `norm:debug_mode_control_transfer`：控制转移指令可能作为非法指令
- `norm:debug_mode_auipc`：auipc 可能作为非法指令
- `norm:debug_mode_zicfilp`：ELP=NO_LP_EXPECTED，LPAD 作为 no-op
- `norm:debug_mode_dxlen`：有效 XLEN 为 DXLEN
- `norm:debug_mode_interrupts`：中断被屏蔽
- `norm:debug_mode_triggers`：触发器不匹配/不触发
- `norm:debug_mode_traps`：陷阱结束 program buffer 执行
- `norm:dcsr_stopcount`：stopcount 控制计数器冻结
- `norm:dcsr_stoptime`：stoptime 控制 time 冻结
- `norm:dcsr_mprven`：mprven 控制 mprv 在 Debug Mode 下是否生效
- `norm:lr_sc_reservation`：LR/SC 预留可能丢失
- `norm:wfi_halt_completion` / `norm:wrs_halt_completion`：wfi/wrs 执行期间 halt 完成后再进入 Debug Mode

**测试职责**：验证 Debug Mode 下各类指令的行为、计数器/定时器控制、以及 wfi/wrs 执行期间的 halt 行为。

### 7.1 Debug Mode 下指令行为

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SDEX-DBG-01 | wfi 作为 nop | Debug Mode 下执行 wfi | 指令立即返回（作为 nop），不等待中断 |
| SDEX-DBG-02 | wrs.sto 作为 nop | Debug Mode 下执行 wrs.sto | 指令立即返回（作为 nop） |
| SDEX-DBG-03 | wrs.nto 作为 nop | Debug Mode 下执行 wrs.nto | 指令立即返回（作为 nop） |
| SDEX-DBG-04 | ebreak 结束 Program Buffer | Debug Mode 下执行 ebreak | 结束 Program Buffer 执行，hart 保持在 Debug Mode |
| SDEX-DBG-05 | 控制转移到 PB 内 | Debug Mode 下执行跳转到 Program Buffer 内的跳转指令 | 可能作为非法指令（实现相关）。若一个作为非法指令，所有都必须 |
| SDEX-DBG-06 | 控制转移到 PB 外 | Debug Mode 下执行跳转到 Program Buffer 外的跳转指令 | 可能作为非法指令（实现相关）。若一个作为非法指令，所有都必须 |
| SDEX-DBG-07 | auipc 行为 | Debug Mode 下执行 auipc | 可能作为非法指令（实现相关） |
| SDEX-DBG-08 | ecall UNSPECIFIED | Debug Mode 下执行 ecall | 行为 UNSPECIFIED — 记录实际行为 |
| SDEX-DBG-09 | mret UNSPECIFIED | Debug Mode 下执行 mret | 行为 UNSPECIFIED — 记录实际行为 |
| SDEX-DBG-10 | sret UNSPECIFIED | Debug Mode 下执行 sret | 行为 UNSPECIFIED — 记录实际行为 |
| SDEX-DBG-11 | uret UNSPECIFIED | Debug Mode 下执行 uret | 行为 UNSPECIFIED — 记录实际行为（若实现 uret） |
| SDEX-DBG-12 | Zicfilp LPAD no-op | Debug Mode 下执行 LPAD 指令（实现 Zicfilp） | LPAD 作为 no-op 执行 |
| SDEX-DBG-13 | Zicfilp ELP 状态 | Debug Mode 下检查 ELP 状态 | ELP == NO_LP_EXPECTED，且不被任何指令更新 |
| SDEX-DBG-14 | DXLEN 验证 | Debug Mode 下验证有效 XLEN | 有效 XLEN == DXLEN |
| SDEX-DBG-15 | M-mode 权限内存访问 | Debug Mode 下执行 load/store | 以 M-mode 权限访问内存 |
| SDEX-DBG-16 | 中断屏蔽验证 | Debug Mode 下触发中断（设 mie 和 mip） | 中断不触发陷阱，hart 保持在 Debug Mode |
| SDEX-DBG-17 | 触发器不匹配 | Debug Mode 下配置 trigger 条件 | 触发器不匹配也不触发 |
| SDEX-DBG-18 | 陷阱结束 PB 执行 | Debug Mode Program Buffer 中执行导致异常的指令 | Program Buffer 执行结束，hart 保持 Debug Mode，不更新 mepc/mcause/mtval |

### 7.2 计数器/定时器控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SDEX-DBG-19 | stopcount=0 计数器正常递增 | dcsr.stopcount=0，进入 Debug Mode 前后读取 instret | instret 在 Debug Mode 期间继续递增 |
| SDEX-DBG-20 | stopcount=1 计数器冻结 | dcsr.stopcount=1，进入 Debug Mode 前后读取 instret | instret 在 Debug Mode 期间不递增（被冻结） |
| SDEX-DBG-21 | stoptime=0 time 正常更新 | dcsr.stoptime=0，进入 Debug Mode 前后读取 time CSR | time 继续反映 mtime |
| SDEX-DBG-22 | stoptime=1 time 冻结 | dcsr.stoptime=1，进入 Debug Mode 前后读取 time CSR | time 在 Debug Mode 入口时冻结，退出后重新同步 mtime |

### 7.3 LR/SC 与 WFI/WRS Halt 行为

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SDEX-DBG-23 | LR 预留可能丢失 | 执行 LR 后进入 Debug Mode，退出后执行 SC | SC 可能失败（预留丢失），或可能成功（实现相关） |
| SDEX-DBG-24 | wfi 执行期间 halt 完成 | hart 执行 wfi 进入停滞状态，此时发送 halt 请求 | wfi 完成执行后进入 Debug Mode（非中断 wfi） |
| SDEX-DBG-25 | wrs.sto 执行期间 halt 完成 | hart 执行 wrs.sto 进入停滞状态，此时发送 halt 请求 | wrs.sto 完成执行后进入 Debug Mode |
| SDEX-DBG-26 | wrs.nto 执行期间 halt 完成 | hart 执行 wrs.nto 进入停滞状态，此时发送 halt 请求 | wrs.nto 完成执行后进入 Debug Mode |

### 7.4 Debug CSR 字段验证

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SDEX-DBG-27 | dscratch0 读写 | Debug Mode 下写 dscratch0 并读回 | 读写一致（若实现） |
| SDEX-DBG-28 | dscratch1 读写 | Debug Mode 下写 dscratch1 并读回 | 读写一致（若实现） |
| SDEX-DBG-29 | dcsr.mprven 验证 | Debug Mode 下设 dcsr.mprven=1，mstatus.MPRV=1，执行内存访问 | mprv 生效（内存访问按 MPP 指定的特权级进行） |
| SDEX-DBG-30 | dcsr.cetrig（Smdbltrp） | 实现 Smdbltrp，设 dcsr.cetrig=1，触发 critical error | hart 进入 Debug Mode，dcsr.cause=7, dcsr.extcause=0 |

> [!NOTE]
> - **所有测试均需外部调试器配合**。在裸机环境下 TEST_SKIP。
> - SDEX-DBG-05/06：规范要求若控制转移指令在 PB 内（或 PB 外）作为非法指令，则所有此类指令都必须作为非法指令。这是一个全有或全无的要求。
> - SDEX-DBG-08 至 SDEX-DBG-11：ecall/mret/sret/uret 在 Debug Mode 下行为为 UNSPECIFIED。测试应记录实际行为而非断言特定结果。若实现导致 hart 状态不可恢复，测试应标记为 UNSPECIFIED。
> - SDEX-DBG-19 至 SDEX-DBG-22：`dcsr.stopcount` 和 `dcsr.stoptime` 可能被硬连线为 0 或 1。测试前应先检测字段可写性。
> - SDEX-DBG-23：LR/SC 预留丢失是"可能"（may），不是"必须"。测试应记录行为而非断言。
> - SDEX-DBG-24 至 SDEX-DBG-26：需要在 wfi/wrs 执行期间（hart 处于停滞状态）发送 halt 请求，验证指令完成后再进入 Debug Mode。
> - SDEX-DBG-27/28：dscratch0/dscratch1 是可选寄存器。若未实现，读写可能返回 0 或触发异常。
> - SDEX-DBG-29：`dcsr.mprven` 可能硬连线为 0 或 1。若硬连线为 0，mprv 在 Debug Mode 下无效。
> - SDEX-DBG-30：需要 Smdbltrp 扩展支持。

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0 | Group 1: Debug CSR 访问限制 | SDEX-CSR-01 ~ SDEX-CSR-14 | 核心安全要求：Debug CSR 在非 Debug Mode 下不可访问 |
| P0 | Group 2: ebreak 默认行为 | SDEX-EBRK-01 ~ SDEX-EBRK-06 | 基础行为验证：ebreak 默认不进入 Debug Mode |
| P0 | Group 3: Sdtrig 依赖 | SDEX-SDTRIG-01 ~ SDEX-SDTRIG-06 | 规范要求验证：Sdext 无 Sdtrig 时的访问限制 |
| P1 | Group 4: Halt 进入 | SDEX-HALT-01 ~ SDEX-HALT-15 | 核心调试功能：halt 状态记录正确性 |
| P1 | Group 5: Resume 行为 | SDEX-RSM-01 ~ SDEX-RSM-11 | 核心调试功能：resume 状态恢复正确性 |
| P1 | Group 6: 单步执行 | SDEX-STEP-01 ~ SDEX-STEP-10 | 核心调试功能：单步执行正确性 |
| P2 | Group 7: Debug Mode 指令行为 | SDEX-DBG-01 ~ SDEX-DBG-30 | 全面行为验证：指令、计数器、边界情况 |

---

## 测试实现说明

### 文件组织

```
damo-priv-test/
├── Sdext/
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_helpers.h           # CSR 地址定义、扩展检测、辅助宏
│       ├── test_register.c          # #include 所有测试文件
│       ├── test_csr_access.c        # Group 1: Debug CSR 访问限制
│       ├── test_ebreak_default.c    # Group 2: ebreak 默认行为
│       ├── test_sdtrig_dep.c        # Group 3: Sdtrig 依赖关系
│       ├── test_halt_entry.c        # Group 4: Halt 进入与 dcsr 状态记录
│       ├── test_resume.c            # Group 5: Resume 行为
│       ├── test_single_step.c       # Group 6: 单步执行
│       └── test_debug_mode.c        # Group 7: Debug Mode 指令行为与计数器控制
└── common/                          # 复用通用框架
```

### 运行时检测

```c
/* Detect Sdext by checking if debug CSR addresses are recognized */
static bool check_sdext_extension(void)
{
    /*
     * Accessing dcsr (0x7b0) from non-Debug Mode should raise
     * illegal instruction. If no trap occurs, the address is not
     * recognized as a debug CSR (or we are already in Debug Mode,
     * which is impossible from bare-metal).
     *
     * Note: This detection is imperfect - both "Sdext implemented"
     * and "Sdext not implemented" result in illegal instruction
     * for debug CSR access. Platform-specific detection (mimpid,
     * device tree) should be used when available.
     */
    trap_expect_begin();
    asm volatile("csrr t0, 0x7b0" ::: "t0");  /* try read dcsr */
    bool trapped = trap_was_triggered();
    uintptr_t cause = trap_get_cause();
    trap_expect_end();

    /*
     * If trapped with illegal instruction, debug CSR address is
     * recognized as restricted => Sdext likely implemented.
     * If no trap, address is unused => Sdext not implemented.
     */
    return trapped && (cause == CAUSE_ILLEGAL_INSTRUCTION);
}

/* Detect Sdtrig by checking tselect accessibility */
static bool check_sdtrig_extension(void)
{
    trap_expect_begin();
    asm volatile("csrr t0, 0x7a0" ::: "t0");  /* try read tselect */
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;  /* No trap => Sdtrig implemented */
}
```

### 关键注意事项

1. **Sdext 检测的局限性**：从裸机环境无法可靠区分"Sdext 已实现"和"Sdext 未实现"，因为两种情况下访问 debug CSR 都会触发 illegal instruction。建议结合平台特性（mimpid、device tree、平台头文件宏）进行判断。

2. **DM 依赖测试的处理**：Group 4-7 的测试在裸机环境下无法执行，应以 `TEST_SKIP("Requires external debugger (DM)")` 处理。测试代码应完整实现验证逻辑，以便在配合 OpenOCD/GDB 时可直接使用。

3. **ebreak 默认值假设**：`dcsr.ebreakm`/`ebreaks`/`ebreaku` 复位后均为 0。因此 ebreak 在默认情况下不进入 Debug Mode，而是触发 breakpoint 异常。这使得 Group 2 可在裸机环境下测试。

4. **UNSPECIFIED 行为的处理**：Debug Mode 下 ecall/mret/sret/uret 的行为为 UNSPECIFIED。测试应记录实际行为，不应断言特定结果。若实现导致 hart 状态不可恢复（如改变了特权级），测试应标记为 UNSPECIFIED 而非 PASS/FAIL。

5. **控制转移指令的一致性**：规范要求若控制转移指令在 PB 内（或外）作为非法指令，则所有此类指令都必须作为非法指令。测试应验证这个全有或全无的一致性要求。

6. **stopcount/stoptime/stepie/mprven 的可写性**：这些字段可能被硬连线为 0 或 1。测试前应先检测可写性，不可写时相关测试 TEST_SKIP。

---

## 测试判定标准

| 判定 | 条件 |
|------|------|
| PASS | 所有断言通过：CSR 访问触发正确的异常类型、ebreak 触发 breakpoint 异常、dcsr 字段记录正确的值、resume 后状态正确 |
| FAIL | CSR 访问未触发异常或触发错误的异常类型、ebreak 行为与规范不符、dcsr 字段值错误、resume 后状态不正确 |
| SKIP | Sdext 未实现、Sdtrig 未实现（Group 3 相关测试）、需要外部调试器（Group 4-7）、可选字段硬连线、依赖扩展未实现（Smdbltrp/Ssdbltrp/Zicfilp） |
| UNSPECIFIED | Debug Mode 下 ecall/mret/sret/uret 行为、LR/SC 预留丢失、控制转移/auipc 在 Debug Mode 下的行为 |

---

## 覆盖率矩阵

| 规范 ID | 覆盖的测试 ID |
|---------|--------------|
| `norm:core_debug_csr_access` | SDEX-CSR-01 ~ SDEX-CSR-14 |
| `norm:dcsr_ebreakm` | SDEX-EBRK-01, SDEX-EBRK-07 |
| `norm:dcsr_ebreaks` | SDEX-EBRK-02, SDEX-EBRK-08 |
| `norm:dcsr_ebreaku` | SDEX-EBRK-03 |
| `norm:debug_mode_priv_change` | SDEX-EBRK-04, SDEX-DBG-04, SDEX-DBG-08 ~ SDEX-DBG-11 |
| `norm:sdtrig_dependency` | SDEX-SDTRIG-01 ~ SDEX-SDTRIG-06 |
| `norm:halt_entry` | SDEX-HALT-01 ~ SDEX-HALT-14 |
| `norm:halt_vector_partial` | SDEX-HALT-15 |
| `norm:dcsr_cause_priority` | SDEX-HALT-06 |
| `norm:resume_behavior` | SDEX-RSM-01 ~ SDEX-RSM-06, SDEX-RSM-11 |
| `norm:resume_smdbltrp` | SDEX-RSM-07, SDEX-RSM-08 |
| `norm:resume_ssdbltrp` | SDEX-RSM-09 |
| `norm:halt_resume_zicfilp` | SDEX-RSM-10, SDEX-DBG-12, SDEX-DBG-13 |
| `norm:dpc_writability` | SDEX-RSM-11 |
| `norm:dscratch_optional` | SDEX-DBG-27, SDEX-DBG-28 |
| `norm:single_step` | SDEX-STEP-01, SDEX-STEP-09, SDEX-STEP-10 |
| `norm:single_step_trap` | SDEX-STEP-04 |
| `norm:single_step_trigger` | SDEX-STEP-05 |
| `norm:single_step_wfi` | SDEX-STEP-06, SDEX-STEP-07, SDEX-STEP-08 |
| `norm:single_step_stepie` | SDEX-STEP-02, SDEX-STEP-03 |
| `norm:debug_mode_wfi_wrs` | SDEX-DBG-01, SDEX-DBG-02, SDEX-DBG-03 |
| `norm:debug_mode_control_transfer` | SDEX-DBG-05, SDEX-DBG-06 |
| `norm:debug_mode_auipc` | SDEX-DBG-07 |
| `norm:debug_mode_zicfilp` | SDEX-DBG-12, SDEX-DBG-13 |
| `norm:debug_mode_dxlen` | SDEX-DBG-14 |
| `norm:debug_mode_behavior` | SDEX-DBG-15 |
| `norm:debug_mode_interrupts` | SDEX-DBG-16 |
| `norm:debug_mode_triggers` | SDEX-DBG-17 |
| `norm:debug_mode_traps` | SDEX-DBG-18 |
| `norm:dcsr_stopcount` | SDEX-DBG-19, SDEX-DBG-20 |
| `norm:dcsr_stoptime` | SDEX-DBG-21, SDEX-DBG-22 |
| `norm:dcsr_mprven` | SDEX-DBG-29 |
| `norm:lr_sc_reservation` | SDEX-DBG-23 |
| `norm:wfi_halt_completion` | SDEX-DBG-24 |
| `norm:wrs_halt_completion` | SDEX-DBG-25, SDEX-DBG-26 |
| `norm:reset_halt` | SDEX-HALT-05 |
| `norm:debug_mode_zicfilp` | SDEX-DBG-12, SDEX-DBG-13, SDEX-RSM-10 |

---

## 参考

- `SPEC/riscv-debug/Sdext.adoc` — Sdext (ISA Extension for External Debug)
- `SPEC/riscv-debug/xml/core_registers.xml` — Core Debug CSR 定义（dcsr/dpc/dscratch0/dscratch1）
- `SPEC/riscv-debug/debug_module.adoc` — Debug Module (DM) 规范
- `SPEC/riscv-debug/Sdtrig.adoc` — Sdtrig (Trigger Module) 规范
- `DOCS/testplan/Smctr_test_plan.md` — Smctr 测试计划（含 Debug Mode 录制抑制）
- `DOCS/testplan/cfi_test_plan.md` — CFI (Zicfilp/Zicfiss) 测试计划
