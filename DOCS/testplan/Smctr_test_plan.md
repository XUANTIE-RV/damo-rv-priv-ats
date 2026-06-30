# Smctr 扩展测试计划（Machine Mode）

> 本文档描述 Smctr（Control Transfer Records — Machine-level）扩展的测试计划。聚焦于 M-mode 下 `mctrctl` CSR、`mstateen0.CTR` 访问控制（非 Hypervisor 部分）、M-mode 录制使能、MTE 外部陷阱录制（非 Hypervisor 部分）、以及 Debug Mode 录制抑制。S-mode / VS-mode 层面的 Ssctr 行为由 `Ssctr_test_plan.md` 覆盖。Hypervisor 相关的 CTR 测试（`hstateen0.CTR` 控制、VS/VU-mode 外部陷阱）已迁移至 `Hypervisor_cross_test_plan.md` Group 13。
>
> 生成时间：2026-06-25

---

## 概述

Smctr 是 RISC-V Control Transfer Records (CTR) 扩展的 Machine-level 部分。CTR 提供在寄存器可访问的片上存储中录制有限的控制流转换历史的能力，旨在大幅降低采集转换历史的性能开销。Smctr 涵盖所有特权级新增的 CSR、指令和行为修改。

### 本文档覆盖的 SPEC 章节

- `mctrctl` CSR（Machine Control Transfer Records Control Register — 全部字段）
- `mstateen0.CTR` 访问控制（State Enable Access Control — 非 Hypervisor 部分）
- M-mode 录制使能（`mctrctl.M`）
- MTE（Machine Trap Enable）外部陷阱录制（S-mode / U-mode → M-mode）
- Debug Mode 录制抑制
- `mctrctl` 字段实现要求

### 由其他测试计划覆盖

- S-mode/VS-mode CTR 行为 → `Ssctr_test_plan.md`
- `sctrctl` / `vsctrctl` / `sctrdepth` / `sctrstatus` CSR → `Ssctr_test_plan.md`
- Entry Registers（ctrsource/ctrtarget/ctrdata）和 SCTRCLR 指令 → `Ssctr_test_plan.md`
- S-mode 层面的特权级转换、外部陷阱、转换类型过滤 → `Ssctr_test_plan.md`
- 周期计数、RAS 仿真、Freeze 行为 → `Ssctr_test_plan.md`
- **Hypervisor × Smctr 交叉测试**（`hstateen0.CTR` 控制、`vsctrctl` 访问、VS/VU-mode MTE 外部陷阱） → `Hypervisor_cross_test_plan.md` Group 13

---

## 覆盖的规范点

本章节列出本文档 Groups 1-5 所有测试组中引用的规范点（norm ID），已去重并按字母顺序排列。

> **注意**：Hypervisor 相关规范点（`norm:hstateen_ctr`、`norm:hstateen_vs`、`norm:hstateen0_CTR0-V1_op`、`norm:exttrap_vsm`、`norm:exttrap_vum`）已迁移至 `Hypervisor_cross_test_plan.md` Group 13。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:debug_recording_inhibited` | Recording in Debug Mode is always inhibited. Transfers into and out of Debug Mode are never recorded. | Debug Mode 下录制始终被抑制。进入和退出 Debug Mode 的转换永不录制。 |
| `norm:exttrap_def` | External traps are traps from a privilege mode enabled for CTR recording to a privilege mode that is not enabled for CTR recording. By default external traps are not recorded, but privileged software running in the target mode can opt-in. | 外部陷阱是从启用 CTR 录制的特权级到未启用录制的特权级的陷阱。默认不录制，但目标模式的特权软件可选择启用。 |
| `norm:exttrap_implreq` | If external trap recording is implemented, `mctrctl`.MTE and `sctrctl`.STE must be implemented, while `vsctrctl`.STE must be implemented if the H extension is implemented. | 若实现外部陷阱录制，MTE 和 STE 必须实现；若实现 H 扩展，vsSTE 也必须实现。 |
| `norm:exttrap_requirements` | External trap recording depends not only on the target mode, but on any intervening modes, which are modes that are more privileged than the source mode but less privileged than the target mode. Not only must the external trap enable bit for the target mode be set, but the external trap enable bit(s) for any intervening modes must also be set. | 外部陷阱录制不仅依赖目标模式，还依赖任何中间模式。不仅目标模式的外部陷阱使能位必须置位，中间模式的外部陷阱使能位也必须置位。 |
| `norm:exttrap_sm` | S-mode to M-mode: `mctrctl`.MTE | S-mode 到 M-mode 的外部陷阱需要 `mctrctl`.MTE 置位。 |
| `norm:exttrap_um` | U-mode to M-mode: `mctrctl`.MTE, `sctrctl`.STE | U-mode 到 M-mode 的外部陷阱需要 MTE 和 STE 都置位。 |
| `norm:mctrctl_impl` | All fields are optional except for M, S, U, and BPFRZ. All unimplemented fields are read-only 0, while all implemented fields are writable. If the Sscofpmf extension is implemented, LCOFIFRZ must be writable. | 除 M/S/U/BPFRZ 外所有字段可选。未实现字段只读零，已实现字段可写。若实现 Sscofpmf，LCOFIFRZ 必须可写。 |
| `norm:mctrctl_mode_op` | Enable transfer recording in the selected privileged mode(s). | 在选定的特权模式下启用转换录制（M/S/U 字段）。 |
| `norm:mctrctl_mte_op` | Enables recording of traps to M-mode when M=0. | 当 M=0 时启用陷阱到 M-mode 的录制（MTE 字段）。 |
| `norm:mstateen_ctr0` | When `mstateen0`.CTR=0 and the privilege mode is less privileged than M-mode, the following operations raise an illegal-instruction exception. | 当 `mstateen0`.CTR=0 且特权级低于 M-mode 时，以下操作触发 illegal-instruction 异常。 |
| `norm:mstateen_ctr0_execpt1` | Attempts to access `sctrctl`, `vsctrctl`, `sctrdepth`, or `sctrstatus` | 尝试访问 `sctrctl`/`vsctrctl`/`sctrdepth`/`sctrstatus`。 |
| `norm:mstateen_ctr0_execpt2` | Attempts to access `sireg*` when `siselect` is in 0x200..0x2FF, or `vsireg*` when `vsiselect` is in 0x200..0x2FF | 当 `siselect` 在 0x200..0x2FF 时访问 `sireg*`，或 `vsiselect` 在 0x200..0x2FF 时访问 `vsireg*`。 |
| `norm:mstateen_ctr0_execpt3` | Execution of the SCTRCLR instruction | 执行 SCTRCLR 指令。 |
| `norm:mstateen_ctr0_qualified_transfer` | When `mstateen0`.CTR=0, qualified control transfers executed in privilege modes less privileged than M-mode will continue to implicitly update entry registers and `sctrstatus`. | 当 `mstateen0`.CTR=0 时，低于 M-mode 的特权级执行的合格控制转换仍会隐式更新 entry 寄存器和 `sctrstatus`。 |
| `norm:mstateen_ctr1` | When `mstateen0`.CTR=1, accesses to CTR register state behave as described in CSRs and Entry Registers, while SCTRCLR behaves as described. | 当 `mstateen0`.CTR=1 时，CTR 寄存器状态访问按规范行为，SCTRCLR 按规范行为。 |
| `norm:Ssctr_mctrctl_sz_acc_op` | The `mctrctl` register is a 64-bit read/write register that enables and configures the CTR capability. | `mctrctl` 是 64 位读写寄存器，启用和配置 CTR 能力。 |
| `norm:Smctr_Ssctr_depend` | Smctr and Ssctr depend on both the implementation of S-mode and the Sscsrind extension. | Smctr/Ssctr 依赖 S-mode 实现和 Sscsrind 扩展。 |

---

## Group 1. mctrctl CSR 基础与字段验证

**规范依据**：
- `norm:Ssctr_mctrctl_sz_acc_op`：`mctrctl` 为 64-bit 读写寄存器
- `norm:mctrctl_mode_op`：M/S/U 字段启用对应特权级录制
- `norm:mctrctl_mte_op`：MTE 字段启用外部陷阱到 M-mode 的录制
- `norm:mctrctl_impl`：字段实现要求（M/S/U/BPFRZ 必须实现，其余可选）

**测试职责**：验证 `mctrctl` 寄存器的基本读写、各字段的可写/只读属性和实现要求。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SMCTR-CTL-01 | mctrctl 基本读写 | M-mode 写 mctrctl 各可写字段，读回验证 | 已实现字段读回一致，未实现字段只读零 |
| SMCTR-CTL-02 | mctrctl.M 字段必须可写 | M-mode 写 mctrctl.M=1 后读回；再写 M=0 读回 | M 字段必须实现且可写（规范要求） |
| SMCTR-CTL-03 | mctrctl.S 字段必须可写 | M-mode 写 mctrctl.S=1 后读回 | S 字段必须实现且可写（规范要求） |
| SMCTR-CTL-04 | mctrctl.U 字段必须可写 | M-mode 写 mctrctl.U=1 后读回 | U 字段必须实现且可写（规范要求） |
| SMCTR-CTL-05 | mctrctl.BPFRZ 字段必须可写 | M-mode 写 mctrctl.BPFRZ=1 后读回 | BPFRZ 字段必须实现且可写（规范要求） |
| SMCTR-CTL-06 | mctrctl.MTE 字段实现检测 | M-mode 写 mctrctl.MTE=1 后读回 | 若实现外部陷阱录制，MTE 可写；否则只读零 |
| SMCTR-CTL-07 | mctrctl.STE 字段实现检测 | M-mode 写 mctrctl.STE=1 后读回 | 若实现外部陷阱录制，STE 可写；否则只读零 |
| SMCTR-CTL-08 | mctrctl.RASEMU 字段实现检测 | M-mode 写 mctrctl.RASEMU=1 后读回 | 若实现 RAS 仿真，RASEMU 可写；否则只读零 |
| SMCTR-CTL-09 | mctrctl.LCOFIFRZ 可写性（Sscofpmf） | 若实现 Sscofpmf，写 mctrctl.LCOFIFRZ=1 后读回 | LCOFIFRZ 必须可写（规范要求） |
| SMCTR-CTL-10 | mctrctl.LCOFIFRZ 只读零（无 Sscofpmf） | 若未实现 Sscofpmf，写 mctrctl.LCOFIFRZ=1 后读回 | LCOFIFRZ 只读零 |
| SMCTR-CTL-11 | mctrctl 传输类型过滤位检测 | 逐位写 mctrctl[47:32]（EXCINH/INTRINH/TRETINH/NTBREN/TKBRINH/INDCALLINH/DIRCALLINH/INDJMPINH/DIRJMPINH/CORSWAPINH/RETINH/INDLJMPINH/DIRLJMPINH），读回确定实现 | 已实现位可写，未实现位只读零 |
| SMCTR-CTL-12 | mctrctl.Custom[3:0] 字段 | 写 mctrctl.Custom=非零值后读回 | 若实现 custom 扩展，Custom 可写；否则只读零 |

---

## Group 2. mstateen0 CTR 访问控制

**规范依据**：
- `norm:mstateen_ctr1` / `norm:mstateen_ctr0`：`mstateen0.CTR` 控制下级特权级对 CTR 状态的访问
- `norm:mstateen_ctr0_execpt1`：阻止访问 `sctrctl`/`vsctrctl`/`sctrdepth`/`sctrstatus`
- `norm:mstateen_ctr0_execpt2`：阻止访问 `sireg*`（siselect 0x200..0x2FF）/ `vsireg*`（vsiselect 0x200..0x2FF）
- `norm:mstateen_ctr0_execpt3`：阻止执行 SCTRCLR
- `norm:mstateen_ctr0_qualified_transfer`：CTR=0 时隐式更新继续

**测试职责**：验证 M-mode 通过 `mstateen0.CTR` 控制下级特权级对 CTR 状态的访问行为（非 Hypervisor 部分）。

> **注意**：Hypervisor 相关的 `hstateen0.CTR` 控制测试（原 STA-11~18）和 `vsctrctl` 访问测试（原 STA-05）已迁移至 `Hypervisor_cross_test_plan.md` Group 13。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SMCTR-STA-01 | mstateen0.CTR 读写验证 | M-mode 写 mstateen0.CTR=1 后读回；再写 CTR=0 读回 | 若实现 Smstateen，CTR 位可写且读回一致 |
| SMCTR-STA-02 | mstateen0.CTR=0 阻止 S-mode 访问 sctrctl | mstateen0.CTR=0，S-mode 尝试读 sctrctl | 触发 illegal-instruction 异常（cause=2） |
| SMCTR-STA-03 | mstateen0.CTR=0 阻止 S-mode 访问 sctrdepth | mstateen0.CTR=0，S-mode 尝试读 sctrdepth | 触发 illegal-instruction 异常（cause=2） |
| SMCTR-STA-04 | mstateen0.CTR=0 阻止 S-mode 访问 sctrstatus | mstateen0.CTR=0，S-mode 尝试读 sctrstatus | 触发 illegal-instruction 异常（cause=2） |
| SMCTR-STA-06 | mstateen0.CTR=0 阻止 S-mode 访问 sireg* | mstateen0.CTR=0，S-mode 设 siselect=0x200 后读 sireg | 触发 illegal-instruction 异常（cause=2） |
| SMCTR-STA-07 | mstateen0.CTR=0 阻止 S-mode 访问 sireg2/sireg3 | mstateen0.CTR=0，S-mode 设 siselect=0x200 后分别读 sireg2 和 sireg3 | 触发 illegal-instruction 异常（cause=2） |
| SMCTR-STA-08 | mstateen0.CTR=0 阻止 S-mode 执行 SCTRCLR | mstateen0.CTR=0，S-mode 执行 SCTRCLR | 触发 illegal-instruction 异常（cause=2） |
| SMCTR-STA-09 | mstateen0.CTR=1 允许 S-mode 完整访问 | mstateen0.CTR=1，S-mode 分别访问 sctrctl/sctrdepth/sctrstatus | 所有访问成功 |
| SMCTR-STA-10 | mstateen0.CTR=0 时隐式更新继续 | mstateen0.CTR=0，M-mode 设置 sctrctl.S=1（通过 mctrctl），S-mode 执行控制转换后，M-mode 检查 entry 寄存器 | entry 寄存器和 sctrstatus 仍被隐式更新 |
| SMCTR-STA-19 | mstateen0.CTR=0 不影响 M-mode 自身访问 | mstateen0.CTR=0，M-mode 访问 sctrctl/sctrdepth/sctrstatus | M-mode 访问不受影响，正常读写 |

---

## Group 3. M-mode 录制行为与 MTE 外部陷阱

**规范依据**：
- `norm:mctrctl_mode_op`：M 字段启用 M-mode 录制
- `norm:mctrctl_mte_op`：MTE 启用外部陷阱到 M-mode 的录制
- `norm:exttrap_def`：外部陷阱定义
- `norm:exttrap_requirements`：外部陷阱录制依赖目标模式和中间模式的 TE 位
- `norm:exttrap_sm`：S→M 需要 MTE
- `norm:exttrap_um`：U→M 需要 MTE + STE
- `norm:exttrap_implreq`：MTE 实现要求

**测试职责**：验证 M-mode 录制使能、MTE 外部陷阱录制到 M-mode 的行为（S-mode / U-mode → M-mode）。

> **注意**：Hypervisor 相关的 VS/VU-mode 外部陷阱测试（原 MODE-07~09）已迁移至 `Hypervisor_cross_test_plan.md` Group 13.3。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SMCTR-MODE-01 | mctrctl.M=1 启用 M-mode 录制 | mctrctl.M=1，M-mode 执行控制转换（如 JAL） | 转换被录制到 entry 0（logical entry 0） |
| SMCTR-MODE-02 | mctrctl.M=0 禁用 M-mode 录制 | mctrctl.M=0，M-mode 执行控制转换 | 转换不被录制 |
| SMCTR-MODE-03 | MTE=0 时外部陷阱不录制（S→M） | mctrctl.M=0，MTE=0，sctrctl.S=1，S-mode 触发陷阱到 M-mode | 外部陷阱不被录制 |
| SMCTR-MODE-04 | MTE=1 时外部陷阱录制（S→M） | mctrctl.M=0，MTE=1，sctrctl.S=1，S-mode 触发陷阱到 M-mode | 外部陷阱被录制，ctrtarget.PC=0 |
| SMCTR-MODE-05 | MTE=1 + STE=1 时外部陷阱录制（U→M） | mctrctl.M=0，MTE=1，sctrctl.S=0，STE=1，sctrctl.U=1，U-mode 触发陷阱到 M-mode | 外部陷阱被录制（需 MTE 和 STE 都置位） |
| SMCTR-MODE-06 | MTE=1 但 STE=0 时外部陷阱不录制（U→M） | mctrctl.M=0，MTE=1，STE=0，U-mode 触发陷阱到 M-mode | 外部陷阱不被录制（STE 未置位，中间模式要求不满足） |
| SMCTR-MODE-10 | 外部陷阱 ctrtarget.PC=0 | MTE=1，S-mode 触发外部陷阱到 M-mode，检查 entry 0 的 ctrtarget | ctrtarget.PC 字段为 0 |
| SMCTR-MODE-11 | M-mode 录制受 FROZEN 抑制 | mctrctl.M=1，sctrstatus.FROZEN=1，M-mode 执行控制转换 | 转换不被录制（FROZEN 抑制所有录制） |
| SMCTR-MODE-12 | 外部陷阱不受 EXCINH/INTRINH 影响 | MTE=1，sctrctl.EXCINH=1，S-mode 异常陷阱到 M-mode | 外部陷阱仍被录制（外部陷阱录制不依赖 EXCINH/INTRINH） |

---

## Group 4. Debug Mode 录制抑制

**规范依据**：
- `norm:debug_recording_inhibited`：Debug Mode 下录制始终被抑制，进入/退出 Debug Mode 的转换永不录制

**测试职责**：验证 Debug Mode 下 CTR 录制被抑制，以及进入/退出 Debug Mode 的转换不被录制。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SMCTR-DBG-01 | Debug Mode 下录制被抑制 | mctrctl.M=1，进入 Debug Mode，执行控制转换，退出 Debug Mode 后检查 entry | 转换不被录制 |
| SMCTR-DBG-02 | 进入 Debug Mode 不被录制 | mctrctl.M=1，M-mode 执行 EBREAK 进入 Debug Mode，退出后检查 entry | 进入 Debug Mode 的转换不被录制 |
| SMCTR-DBG-03 | 退出 Debug Mode 不被录制 | mctrctl.M=1，从 Debug Mode 执行 DRET，检查 entry | 退出 Debug Mode 的转换不被录制 |
| SMCTR-DBG-04 | Debug Mode 前后录制正常 | mctrctl.M=1，M-mode 先执行一次转换（A），然后进入 Debug Mode，退出后再执行一次转换（B），检查 entry | A 和 B 都被录制（Debug Mode 中的不录制，但不影响前后的录制） |

---

## Group 5. 扩展依赖与实现要求

**规范依据**：
- `norm:Smctr_Ssctr_depend`：Smctr/Ssctr 依赖 S-mode 和 Sscsrind
- `norm:mctrctl_impl`：M/S/U/BPFRZ 必须实现，其余可选；若实现 Sscofpmf 则 LCOFIFRZ 必须可写
- `norm:exttrap_implreq`：若实现外部陷阱录制，MTE 和 STE 必须实现

**测试职责**：验证 Smctr 的依赖关系和实现要求。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SMCTR-DEP-01 | S-mode 依赖验证 | 检测 S-mode 是否实现（misa.S bit） | 若实现 Smctr，必须实现 S-mode |
| SMCTR-DEP-02 | Sscsrind 依赖验证 | 检测 siselect CSR 是否可访问 | 若实现 Smctr，siselect 必须存在且可访问 |
| SMCTR-DEP-03 | M/S/U/BPFRZ 必须实现 | 写 mctrctl 的 M/S/U/BPFRZ 字段 | 这些字段必须可写（规范要求） |
| SMCTR-DEP-04 | MTE 实现要求（若实现外部陷阱） | 若实现外部陷阱录制功能，检测 MTE 字段 | MTE 必须可写（规范要求） |
| SMCTR-DEP-05 | LCOFIFRZ 实现要求（Sscofpmf） | 若实现 Sscofpmf，检测 LCOFIFRZ 字段 | LCOFIFRZ 必须可写（规范要求） |
| SMCTR-DEP-06 | Smctr/Ssctr 成对实现 | 检测 Smctr 是否隐含 Ssctr | 若实现 Smctr，S-mode 层面的 CTR 功能也应可用 |

---

## 测试实现说明

### 文件组织

```
damo-priv-test/
├── smctr/
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_mctrctl_csr.c        # Group 1: mctrctl CSR 基础与字段验证
│       ├── test_mstateen_ctr.c       # Group 2: mstateen0 CTR 访问控制
│       ├── test_mmode_recording.c    # Group 3: M-mode 录制行为与 MTE 外部陷阱
│       ├── test_debug_mode.c         # Group 4: Debug Mode 录制抑制
│       └── test_dependencies.c       # Group 5: 扩展依赖与实现要求
├── hypervisor_cross/                  # Hypervisor 交叉测试（包含 Smctr）
│   └── tests/
│       └── test_hcross_smctr.c       # Group 13: Hypervisor × Smctr
└── common/                            # 复用通用框架
```

> **注意**：原 `test_hstateen_ctr.c`（Group 2 Hypervisor 部分）和 `test_exttrap_hyp.c`（Group 3 VS/VU-mode 部分）已迁移至 `hypervisor_cross/` 项目。

### 运行时检测

```c
static bool check_smctr_extension(void) {
    /* Probe mctrctl writability */
    trap_expect_begin();
    uintptr_t old = CSRR(mctrctl);
    CSRW(mctrctl, old | MCTRCTL_M);  /* try to set M bit */
    uintptr_t new_val = CSRR(mctrctl);
    CSRW(mctrctl, old);  /* restore */
    trap_expect_end();
    return (new_val & MCTRCTL_M) != 0;  /* M bit was set => Smctr exists */
}

static bool check_mstateen_extension(void) {
    /* Probe mstateen0 CSR existence */
    trap_expect_begin();
    uintptr_t val = CSRR(mstateen0);
    (void)val;
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}
```

### 关键注意事项

1. **扩展检测**：所有测试必须在运行时检测 Smctr 的可用性（通过 `mctrctl` 可写性），不可用时 TEST_SKIP。

2. **Smstateen 依赖**：Group 2 测试需要 Smstateen 扩展。通过检测 `mstateen0` CSR 的存在性判断，不存在时 TEST_SKIP。

3. **MTE 字段检测**：MTE 是可选字段。测试前需通过写回检测确定是否实现，未实现时相关测试 TEST_SKIP。

4. **Hypervisor 测试**：需要 Hypervisor 扩展的测试（`hstateen0.CTR`、`vsctrctl`、VS/VU-mode 外部陷阱）已迁移至 `Hypervisor_cross_test_plan.md`。

5. **CTR 清理**：每次测试前应调用 `sctrclr()` 清空 entry 寄存器，避免前次测试干扰。

---

## 参考

- `SPEC/smctr.adoc` — Smctr (Control Transfer Records - Machine-level) Extension
- `SPEC/ssctr.adoc` — Ssctr (Control Transfer Records - Supervisor-level) Extension
- `SPEC/smstateen.adoc` — Smstateen Extension Specification
- `DOCS/testplan/Ssctr_test_plan.md` — Ssctr Supervisor Mode 测试计划
- `DOCS/testplan/Hypervisor_cross_test_plan.md` — Hypervisor 与其他扩展交叉测试计划（含 Hypervisor × Smctr，Group 13）
