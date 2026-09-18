# Ssccfg 扩展测试计划（Supervisor Mode）

本文档描述 Ssccfg（Counter Configuration — Supervisor-level）扩展的测试计划。聚焦于 S-mode 通过 `siselect`/`sireg*` 间接访问委托计数器、`scountinhibit` 寄存器、以及 MINH 位行为。M-mode 层面的 Smcdeleg 行为由 `Smcdeleg_test_plan.md` 覆盖；Hypervisor 场景的虚拟化行为（scountovf/scountinhibit 虚拟化、LCOFI 虚拟中断位、vsiselect/vsireg* 访问规则、hstateen0 bit 60 控制）已迁移至 `Hypervisor_Ss_test_plan.md` Group 11。

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/smcdeleg.adoc` — Smcdeleg/Ssccfg：Counter Delegation、scountinhibit、间接 HPM 映射、MINH 只读零
- `SPEC/riscv-isa-manual/src/priv/smcsrind.adoc` — Smcsrind/Sscsrind：siselect/sireg* 间接 CSR 访问机制
- `SPEC/riscv-isa-manual/src/priv/smstateen.adoc` — Smstateen：mstateen0[60] 对间接 CSR 的门控
- `SPEC/riscv-isa-manual/src/priv/sscofpmf.adoc` — Sscofpmf：hpmevent MINH 位（bit 62）语义
- `SPEC/riscv-isa-manual/src/priv/smcntrpmf.adoc` — Smcntrpmf：cyclecfg/instretcfg MINH 位语义

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内 src/priv/smcdeleg.adoc、src/priv/smcsrind.adoc、src/priv/smstateen.adoc、src/priv/sscofpmf.adoc、src/priv/smcntrpmf.adoc）

---

## 范围

### 覆盖的 Ssccfg Supervisor-level 行为

- **间接 HPM 映射**：`siselect` 0x40-0x5F 范围通过 `sireg*` 访问委托计数器的状态
- **sireg* 访问规则**：非法访问条件（CDE=0、sireg3/6、sireg4/5 in RV64、未委托计数器、缺少依赖扩展）
- **MINH 位行为**：hpmevent/cyclecfg/instretcfg 的 bit 62 经 sireg* 间接访问时只读零（Group 7）
- **scountinhibit 寄存器**：委托计数器的计数抑制控制、非委托位的只读零行为（Group 3）

### 不在本文档范围

- M-mode 的 `menvcfg.CDE` 使能控制和 `mcounteren` 委托位设置（由 `Smcdeleg_test_plan.md` 覆盖）
- `mstateen0` bit 60 的 M-mode 控制（由 `Smcdeleg_test_plan.md` 覆盖）
- `mvip`/`mvien` LCOFI 位的 M-mode 验证（由 `Smcdeleg_test_plan.md` 覆盖）
- 非委托场景下的 `scounteren` 基本行为（由其他扩展测试计划覆盖）
- Hypervisor 场景的虚拟化行为（VS/VU-mode 读 `scountovf`、访问 `scountinhibit`、`hvip`/`hvien` LCOFI 位、`vsiselect`/`vsireg*` 访问规则、hstateen0 bit 60 VS-mode 控制）— 已迁移至 `Hypervisor_Ss_test_plan.md` Group 11

---

## 覆盖的规范点

本方案引用的规范点主要来自 `smcdeleg.adoc`。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:ssccfg_illegal_sireg_cde0` | Attempts to access any `sireg*` when `menvcfg`.CDE = 0 raise illegal-instruction exceptions. | 当 `menvcfg`.CDE = 0 时，访问任何 `sireg*` 触发 illegal-instruction 异常。 |
| `norm:ssccfg_illegal_sireg3_6` | Attempts to access `sireg3` or `sireg6` raise illegal-instruction exceptions. | 访问 `sireg3` 或 `sireg6` 触发 illegal-instruction 异常。 |
| `norm:ssccfg_illegal_sireg4_5_xlen64` | Attempts to access `sireg4` or `sireg5` when XLEN = 64 raise illegal-instruction exceptions. | 当 XLEN = 64 时，访问 `sireg4` 或 `sireg5` 触发 illegal-instruction 异常。 |
| `norm:ssccfg_illegal_sireg_not_delegated` | Attempts to access `sireg*` when `siselect` = 0x41, or when the counter selected by `siselect` is not delegated to S-mode (mcounteren bit = 0), raise illegal-instruction exceptions. | 当 `siselect` = 0x41 或所选计数器未委托给 S-mode（mcounteren 对应位 = 0）时，访问 `sireg*` 触发 illegal-instruction 异常。 |
| `norm:ssccfg_missing_extension_illegal` | If any extension upon which the underlying state depends is not implemented, an attempt from M or S mode to access the given state through `sireg*` raises an illegal-instruction exception. | 如果底层状态依赖的任何扩展未实现，从 M 或 S 模式通过 `sireg*` 访问该状态会触发 illegal-instruction 异常。 |
| `norm:ssccfg_scountinhibit_exists` | Smcdeleg/Ssccfg defines a new `scountinhibit` register, a masked alias of `mcountinhibit`. | Smcdeleg/Ssccfg 定义了新的 `scountinhibit` 寄存器，是 `mcountinhibit` 的掩码别名。 |
| `norm:ssccfg_scountinhibit_delegated_rw` | For counters delegated to S-mode, the associated `mcountinhibit` bits can be accessed via `scountinhibit`. | 对于委托给 S-mode 的计数器，关联的 `mcountinhibit` 位可通过 `scountinhibit` 访问。 |
| `norm:ssccfg_scountinhibit_nondelegated_ro` | For counters not delegated to S-mode, the associated bits in `scountinhibit` are read-only zero. | 对于未委托给 S-mode 的计数器，`scountinhibit` 中关联的位为只读零。 |
| `norm:ssccfg_illegal_scountinhibit_cde0` | When `menvcfg`.CDE=0, attempts to access `scountinhibit` raise an illegal-instruction exception. | 当 `menvcfg`.CDE=0 时，访问 `scountinhibit` 触发 illegal-instruction 异常。 |
| `norm:sscsrind_csrs_access_control` | If extension Smstateen is implemented together with Smcsrind, bit 60 of state-enable register `mstateen0` controls access to `siselect`, `sireg*`, `vsiselect`, and `vsireg*`. When `mstateen0`[60]=0, an attempt to access one of these CSRs from a privilege mode less privileged than M-mode results in an illegal-instruction exception. | Smstateen 与 Smcsrind 同时实现时，`mstateen0` bit 60 控制对 siselect/sireg*/vsiselect/vsireg* 的访问。为 0 时低于 M-mode 的访问触发 illegal-instruction。 |

> **已迁移的规范点**：`norm:ssccfg_illegal_scountinhibit_vs_vu`、`norm:ssccfg_virtual_scountovf_vs_vu`、`norm:ssccfg_lcofi_hvip_hvien`、`norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal`、`norm:ssccfg_hyp_m_s_vsireg_illegal`、`norm:ssccfg_hyp_vs_access_sireg_conditional` 全部依赖 Hypervisor 扩展，已迁移至 `Hypervisor_Ss_test_plan.md` Group 11。

---

## Group 1. siselect 范围与间接 HPM 映射

**规范依据**：
- 间接 HPM 状态映射表（siselect 0x40-0x5F）
- `norm:ssccfg_missing_extension_illegal`：缺少依赖扩展时触发 illegal-instruction

**测试职责**：验证 S-mode 通过 `siselect`/`sireg*` 间接访问委托计数器的映射正确性，包括 cycle/instret/hpmcounter 的计数器值、高位、事件选择器、配置寄存器的映射关系。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCFG-MAP-01 | siselect=0x40 → cycle 映射 | CDE=1，mcounteren[0]=1，S-mode 设 siselect=0x40，读 sireg | sireg 返回 cycle 值（非零，与直接读 cycle 一致） |
| SSCFG-MAP-02 | siselect=0x40 → cycleh 映射 (RV32) | RV32 下，siselect=0x40，读 sireg4 | sireg4 返回 cycleh 值（cycle 高 32 位） |
| SSCFG-MAP-03 | siselect=0x42 → instret 映射 | CDE=1，mcounteren[2]=1，S-mode 设 siselect=0x42，读 sireg | sireg 返回 instret 值 |
| SSCFG-MAP-04 | siselect=0x42 → instreth 映射 (RV32) | RV32 下，siselect=0x42，读 sireg4 | sireg4 返回 instreth 值 |
| SSCFG-MAP-05 | siselect=0x43 → hpmcounter3 映射 | CDE=1，mcounteren[3]=1，S-mode 设 siselect=0x43，读 sireg | sireg 返回 hpmcounter3 值 |
| SSCFG-MAP-06 | siselect=0x43 → hpmcounter3h 映射 (RV32) | RV32 下，siselect=0x43，读 sireg4 | sireg4 返回 hpmcounter3h 值 |
| SSCFG-MAP-07 | siselect=0x43 → hpmevent3 映射 | CDE=1，mcounteren[3]=1，S-mode 设 siselect=0x43，写 sireg2 后读回 | sireg2 对应 hpmevent3，写入值与读回值一致（受 MINH 位约束） |
| SSCFG-MAP-08 | siselect=0x5F → hpmcounter31 映射 | CDE=1，mcounteren[31]=1，S-mode 设 siselect=0x5F，读 sireg | sireg 返回 hpmcounter31 值 |
| SSCFG-MAP-09 | siselect=0x40 → cyclecfg 映射（Smcntrpmf） | 若实现 Smcntrpmf，siselect=0x40，写 sireg2 后读回 | sireg2 对应 cyclecfg，可写位与读回一致；bit 62 (MINH) 为只读零 |
| SSCFG-MAP-10 | siselect=0x42 → instretcfg 映射（Smcntrpmf） | 若实现 Smcntrpmf，siselect=0x42，写 sireg2 后读回 | sireg2 对应 instretcfg，可写位与读回一致；bit 62 (MINH) 为只读零 |
| SSCFG-MAP-11 | siselect=0x41 始终非法 | CDE=1，mcounteren[1]=1，S-mode 设 siselect=0x41 访问 sireg | 触发 illegal-instruction 异常（mtime 不可委托） |
| SSCFG-MAP-12 | 缺少 Zicntr → cycle 访问非法 | CDE=1，mcounteren[0]=1，若未实现 Zicntr，S-mode 设 siselect=0x40 访问 sireg | 触发 illegal-instruction 异常 |
| SSCFG-MAP-13 | 缺少 Zihpm → hpmcounter 访问非法 | CDE=1，mcounteren[3]=1，若未实现 Zihpm，S-mode 设 siselect=0x43 访问 sireg | 触发 illegal-instruction 异常 |

> [!NOTE]
> - 间接 HPM 映射表定义了 siselect 0x40-0x5F 与计数器状态的对应关系：
>   - `sireg`：计数器值（cycle/instret/hpmcounterN）
>   - `sireg4`：计数器高位（cycleh/instreth/hpmcounterNh，仅 RV32 有效）
>   - `sireg2`：事件选择器或配置寄存器（cyclecfg/instretcfg/hpmeventN）
>   - `sireg5`：事件选择器高位（cyclecfgh/instretcfgh/hpmeventNh，仅 RV32 有效）
> - RV64 下访问 sireg4/sireg5 触发 illegal-instruction（由 Group 2 覆盖）。
> - `hpmevent` bit 62（MINH）通过 sireg* 访问时为只读零（若实现 Sscofpmf）。
> - `cyclecfg`/`instretcfg` bit 62（MINH）通过 sireg* 访问时为只读零（若实现 Smcntrpmf）。
> - siselect=0x41 对应 mtime，始终不可通过委托机制访问，即使 mcounteren[1]=1 也触发 illegal-instruction。

---

## Group 2. sireg* 访问规则与非法条件

**规范依据**：
- `norm:ssccfg_illegal_sireg_cde0`：CDE=0 时访问 sireg* 非法
- `norm:ssccfg_illegal_sireg3_6`：sireg3/sireg6 始终非法
- `norm:ssccfg_illegal_sireg4_5_xlen64`：RV64 下 sireg4/sireg5 非法
- `norm:ssccfg_illegal_sireg_not_delegated`：未委托计数器或 siselect=0x41 非法

**测试职责**：验证 S-mode 在 M 或 S 模式下，sireg* 访问触发 illegal-instruction 异常的所有条件。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCFG-ILL-01 | CDE=0 时访问 sireg 非法 | menvcfg.CDE=0，S-mode 设 siselect=0x40 读 sireg | 触发 illegal-instruction 异常 |
| SSCFG-ILL-02 | CDE=0 时访问 sireg2 非法 | menvcfg.CDE=0，S-mode 设 siselect=0x43 读写 sireg2 | 触发 illegal-instruction 异常 |
| SSCFG-ILL-03 | CDE=0 时访问 sireg4 非法 | menvcfg.CDE=0，S-mode 设 siselect=0x40 读 sireg4 | 触发 illegal-instruction 异常 |
| SSCFG-ILL-04 | CDE=0 时访问 sireg5 非法 | menvcfg.CDE=0，S-mode 设 siselect=0x43 写 sireg5 | 触发 illegal-instruction 异常 |
| SSCFG-ILL-05 | sireg3 始终非法（CDE=1） | menvcfg.CDE=1，mcounteren[0]=1，S-mode 设 siselect=0x40 读 sireg3 | 触发 illegal-instruction 异常 |
| SSCFG-ILL-06 | sireg6 始终非法（CDE=1） | menvcfg.CDE=1，mcounteren[0]=1，S-mode 设 siselect=0x40 读 sireg6 | 触发 illegal-instruction 异常 |
| SSCFG-ILL-07 | sireg3 始终非法（CDE=0） | menvcfg.CDE=0，S-mode 设 siselect=0x40 读 sireg3 | 触发 illegal-instruction 异常 |
| SSCFG-ILL-08 | RV64 下 sireg4 非法 | RV64 平台，menvcfg.CDE=1，mcounteren[0]=1，S-mode 设 siselect=0x40 读 sireg4 | 触发 illegal-instruction 异常 |
| SSCFG-ILL-09 | RV64 下 sireg5 非法 | RV64 平台，menvcfg.CDE=1，mcounteren[3]=1，S-mode 设 siselect=0x43 写 sireg5 | 触发 illegal-instruction 异常 |
| SSCFG-ILL-10 | 未委托计数器访问非法 | menvcfg.CDE=1，mcounteren[5]=0，S-mode 设 siselect=0x45 读 sireg | 触发 illegal-instruction 异常 |
| SSCFG-ILL-11 | siselect=0x41 非法（mtime） | menvcfg.CDE=1，mcounteren[1]=1，S-mode 设 siselect=0x41 读 sireg | 触发 illegal-instruction 异常 |
| SSCFG-ILL-12 | siselect=0x41 非法（mcounteren[1]=0） | menvcfg.CDE=1，mcounteren[1]=0，S-mode 设 siselect=0x41 读 sireg | 触发 illegal-instruction 异常 |
| SSCFG-ILL-13 | siselect 超出范围 | CDE=1，S-mode 设 siselect=0x60（超出 0x40-0x5F 范围），读 sireg | 行为取决于其他扩展的 siselect 映射，不受 Smcdeleg/Ssccfg 规范约束 |
| SSCFG-ILL-14 | M-mode 访问 siselect 0x40-0x5F 时 sireg3 非法 | M-mode 设 siselect=0x40 读 sireg3 | 触发 illegal-instruction 异常 |
| SSCFG-ILL-15 | M-mode 访问 siselect 0x40-0x5F 时 sireg6 非法 | M-mode 设 siselect=0x40 读 sireg6 | 触发 illegal-instruction 异常 |

> [!NOTE]
> - 规范明确指出，当特权模式为 M 或 S 且 `siselect` 在 0x40-0x5F 范围内时，以下访问触发 illegal-instruction：
>   1. `menvcfg.CDE = 0` 时访问任何 sireg*
>   2. 访问 sireg3 或 sireg6（无论 CDE 值如何）
>   3. RV64 下访问 sireg4 或 sireg5（这些寄存器仅在 RV32 下有定义）
>   4. siselect=0x41 时访问任何 sireg*（mtime 不可委托）
>   5. 所选计数器未委托时访问任何 sireg*
> - SSCFG-ILL-05~07 验证 sireg3/sireg6 始终非法，与 CDE 状态无关。
> - SSCFG-ILL-14~15 验证 M-mode 也受同样规则约束。
> - RV32 平台下，SSCFG-ILL-08~09 应改为验证 sireg4/sireg5 的正常访问（由 Group 1 覆盖），而非触发异常。

---

## Group 3. scountinhibit 寄存器

**规范依据**：
- `norm:ssccfg_scountinhibit_exists`：scountinhibit 是 mcountinhibit 的掩码别名
- `norm:ssccfg_scountinhibit_delegated_rw`：委托计数器的位可通过 scountinhibit 读写
- `norm:ssccfg_scountinhibit_nondelegated_ro`：未委托计数器的位为只读零
- `norm:ssccfg_illegal_scountinhibit_cde0`：CDE=0 时访问 scountinhibit 非法

**测试职责**：验证 `scountinhibit` 寄存器的存在性、委托位的读写行为、非委托位的只读零行为、以及 CDE=0 时的非法访问。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCFG-SINH-01 | scountinhibit 存在性验证 | CDE=1，S-mode 读 scountinhibit | 访问成功，无异常 |
| SSCFG-SINH-02 | 委托位可写（CY） | CDE=1，mcounteren[0]=1，S-mode 写 scountinhibit bit 0 = 1 后读回 | bit 0 读回为 1 |
| SSCFG-SINH-03 | 委托位可写清零（CY） | CDE=1，mcounteren[0]=1，S-mode 写 scountinhibit bit 0 = 0 后读回 | bit 0 读回为 0 |
| SSCFG-SINH-04 | 委托位可写（IR） | CDE=1，mcounteren[2]=1，S-mode 写 scountinhibit bit 2 = 1 后读回 | bit 2 读回为 1 |
| SSCFG-SINH-05 | 委托位可写（HPM3） | CDE=1，mcounteren[3]=1，S-mode 写 scountinhibit bit 3 = 1 后读回 | bit 3 读回为 1 |
| SSCFG-SINH-06 | 委托位可写（HPM31） | CDE=1，mcounteren[31]=1，S-mode 写 scountinhibit bit 31 = 1 后读回 | bit 31 读回为 1 |
| SSCFG-SINH-07 | 非委托位只读零（CY 未委托） | CDE=1，mcounteren[0]=0，S-mode 写 scountinhibit bit 0 = 1 后读回 | bit 0 读回为 0（只读零） |
| SSCFG-SINH-08 | 非委托位只读零（IR 未委托） | CDE=1，mcounteren[2]=0，S-mode 写 scountinhibit bit 2 = 1 后读回 | bit 2 读回为 0 |
| SSCFG-SINH-09 | 非委托位只读零（HPM 未委托） | CDE=1，mcounteren[3]=0，S-mode 写 scountinhibit bit 3 = 1 后读回 | bit 3 读回为 0 |
| SSCFG-SINH-10 | TM 位（bit 1）只读零 | CDE=1，mcounteren[1]=1，S-mode 写 scountinhibit bit 1 = 1 后读回 | bit 1 读回为 0（mtime 不可委托，TM 位始终只读零） |
| SSCFG-SINH-11 | 多位委托/非委托混合 | CDE=1，mcounteren = 0x05（仅 CY 和 HPM3 委托），S-mode 写 scountinhibit 全 1 后读回 | 仅 bit 0 和 bit 3 读回为 1，其余位读回为 0 |
| SSCFG-SINH-12 | CDE=0 时访问非法 | menvcfg.CDE=0，S-mode 读 scountinhibit | 触发 illegal-instruction 异常 |
| SSCFG-SINH-13 | CDE=0 时写访问非法 | menvcfg.CDE=0，S-mode 写 scountinhibit | 触发 illegal-instruction 异常 |
| SSCFG-SINH-14 | scountinhibit 与 mcountinhibit 同步 | CDE=1，mcounteren[0]=1，S-mode 写 scountinhibit bit 0 = 1，M-mode 读 mcountinhibit bit 0 | mcountinhibit bit 0 读回为 1（scountinhibit 是 mcountinhibit 的别名） |
| SSCFG-SINH-15 | mcountinhibit 修改反映到 scountinhibit | CDE=1，mcounteren[0]=1，M-mode 写 mcountinhibit bit 0 = 1，S-mode 读 scountinhibit bit 0 | scountinhibit bit 0 读回为 1 |
| SSCFG-SINH-16 | scountinhibit 功能验证（抑制计数） | CDE=1，mcounteren[0]=1，S-mode 设 scountinhibit bit 0 = 1 后验证 cycle 停止递增 | 在 scountinhibit CY=1 期间，通过 siselect=0x40 读到的 cycle 值不变（或增长极慢） |

> [!NOTE]
> - `scountinhibit` 是 `mcountinhibit` 的掩码别名。委托计数器的位在 `scountinhibit` 中可读写，且读写操作实际影响 `mcountinhibit` 对应位。未委托计数器的位在 `scountinhibit` 中为只读零。
> - bit 1（TM，对应 time/mtime）在 `scountinhibit` 中始终为只读零，因为 mtime 是内存映射寄存器，不是性能监控计数器，不可通过此机制委托。
> - SSCFG-SINH-14~15 验证 `scountinhibit` 与 `mcountinhibit` 的同步关系 —— 它们共享同一底层状态，只是通过 mcounteren 掩码控制可见性。
> - SSCFG-SINH-16 验证功能正确性：设置 `scountinhibit` CY=1 后，cycle 计数器应被抑制。
> - `scountinhibit` 的 CSR 地址为 0x120。

---

## Group 4. scountovf 虚拟化（Hypervisor 场景）

> [!NOTE]
> 本组用例（`norm:ssccfg_virtual_scountovf_vs_vu`）依赖 Hypervisor 扩展，已迁移至 `Hypervisor_Ss_test_plan.md` Group 11（HCROSS-SSCCFG-01~04）。

---

## Group 5. LCOFI 虚拟化（Hypervisor 场景）

> [!NOTE]
> 本组用例（`norm:ssccfg_lcofi_hvip_hvien`）依赖 Hypervisor 扩展，已迁移至 `Hypervisor_Ss_test_plan.md` Group 11（HCROSS-SSCCFG-07~11）。

---

## Group 6. Hypervisor 交互：vsiselect/vsireg* 访问规则

> [!NOTE]
> 本组用例（`norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal`、`norm:ssccfg_hyp_m_s_vsireg_illegal`、`norm:ssccfg_hyp_vs_access_sireg_conditional`）依赖 Hypervisor 扩展，已迁移至 `Hypervisor_Ss_test_plan.md` Group 11（HCROSS-SSCCFG-12~21）。

---

## Group 7. hpmevent MINH 位与 cyclecfg/instretcfg MINH 位

**规范依据**：
- Sscofpmf 下 `hpmevent` bit 62（MINH）通过 sireg* 访问时为只读零
- Smcntrpmf 下 `cyclecfg`/`instretcfg` bit 62（MINH）通过 sireg* 访问时为只读零

**测试职责**：验证通过间接访问机制时，MINH 位的只读零行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCFG-MINH-01 | hpmevent3 MINH (bit 62) 通过 sireg2 只读零 | CDE=1，mcounteren[3]=1，实现 Sscofpmf，S-mode 设 siselect=0x43，通过 sireg2 写 hpmevent3 bit 62 = 1 后读回 | bit 62 读回为 0（MINH 通过 sireg* 访问时只读零） |
| SSCFG-MINH-02 | hpmevent3 其他位通过 sireg2 可写 | CDE=1，mcounteren[3]=1，S-mode 设 siselect=0x43，通过 sireg2 写 hpmevent3 低 32 位 | 低 32 位可写且读回一致 |
| SSCFG-MINH-03 | hpmevent31 MINH 通过 sireg2 只读零 | CDE=1，mcounteren[31]=1，实现 Sscofpmf，S-mode 设 siselect=0x5F，通过 sireg2 写 hpmevent31 bit 62 = 1 | bit 62 读回为 0 |
| SSCFG-MINH-04 | cyclecfg MINH (bit 62) 通过 sireg2 只读零 | CDE=1，mcounteren[0]=1，实现 Smcntrpmf，S-mode 设 siselect=0x40，通过 sireg2 写 cyclecfg bit 62 = 1 | bit 62 读回为 0 |
| SSCFG-MINH-05 | instretcfg MINH (bit 62) 通过 sireg2 只读零 | CDE=1，mcounteren[2]=1，实现 Smcntrpmf，S-mode 设 siselect=0x42，通过 sireg2 写 instretcfg bit 62 = 1 | bit 62 读回为 0 |
| SSCFG-MINH-06 | hpmevent3h (RV32) 通过 sireg5 访问 | RV32 下，CDE=1，mcounteren[3]=1，S-mode 设 siselect=0x43，通过 sireg5 读写 hpmevent3h | 可写位读写一致（受限于实现） |

> [!NOTE]
> - MINH 位（bit 62）控制计数器是否在 M-mode 下计数。通过 sireg* 间接访问时，S-mode 无法控制 M-mode 的计数行为，因此 MINH 被强制为只读零。
> - SSCFG-MINH-01~03 需要 Sscofpmf 扩展实现；SSCFG-MINH-04~05 需要 Smcntrpmf 扩展实现。若对应扩展未实现，应 TEST_SKIP。
> - 通过 M-mode 直接访问 `mhpmevent3`（CSR 0x323）时，MINH 位仍可正常读写。只读零仅适用于通过 sireg* 间接访问。

---

## Group 8. Smstateen 与 Ssccfg 交互（S-mode 视角）

**规范依据**：
- `norm:sscsrind_csrs_access_control`：`mstateen0` bit 60 = 0 阻止 S-mode 访问 siselect/sireg*

**测试职责**：验证 Smstateen 控制位对 S-mode 间接访问 CSR 的影响。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCFG-STA-01 | mstateen0 bit 60 = 0 阻止 S-mode 写 siselect | mstateen0 bit 60 = 0，S-mode 写 siselect | 触发 illegal-instruction 异常 |
| SSCFG-STA-02 | mstateen0 bit 60 = 0 阻止 S-mode 读 sireg | mstateen0 bit 60 = 0，S-mode 读 sireg | 触发 illegal-instruction 异常 |
| SSCFG-STA-03 | mstateen0 bit 60 = 1 允许 S-mode 访问 | mstateen0 bit 60 = 1，CDE=1，mcounteren[0]=1，S-mode 设 siselect=0x40 读 sireg | 访问成功 |

> [!NOTE]
> - 本组测试需要 Smstateen 扩展实现。若未实现，所有测试应 TEST_SKIP。
> - `mstateen0` bit 60（CSRIND）对 S-mode 的控制优先于 `menvcfg.CDE`。即使 CDE=1，若 bit 60 = 0，S-mode 仍无法使用间接访问机制。
> - hstateen0 bit 60 对 VS-mode 的控制用例依赖 H 扩展，已迁移至 `Hypervisor_Ss_test_plan.md` Group 11（HCROSS-SSCCFG-22~24）。

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 2 (非法条件) | SSCFG-ILL-01~15 | sireg* 非法访问条件是 Ssccfg 安全保证的核心 |
| P0（必须） | Group 3 (scountinhibit) | SSCFG-SINH-01~16 | scountinhibit 是委托计数器的核心控制寄存器 |
| P1（重要） | Group 1 (映射) | SSCFG-MAP-01~13 | 间接 HPM 映射的正确性是计数器委托功能的基础 |
| P1（重要） | Group 7 (MINH) | SSCFG-MINH-01~06 | MINH 位只读零是 M-mode 计数控制的安全隔离保证 |
| P3（可选） | Group 8 (Smstateen) | SSCFG-STA-01~03 | Smstateen 交互是安全隔离的补充保证 |

> 注：原 Group 4（scountovf 虚拟化）、Group 5（LCOFI 虚拟化）、Group 6（Hypervisor 交互）及 Group 8 的 hstateen0 相关用例均依赖 H 扩展，已迁移至 `Hypervisor_Ss_test_plan.md` Group 11（HCROSS-SSCCFG-01~24）。

---

## 关键注意事项

1. **扩展检测**：所有测试必须在运行时检测 Ssccfg（通过 `menvcfg.CDE` 可写性 + `siselect` CSR 存在性）的可用性。不可用时 TEST_SKIP。

2. **sireg* 编号映射**：
   - `sireg` (CSR 0x145)：计数器值
   - `sireg2` (CSR 0x155)：事件选择器/配置寄存器
   - `sireg3` (CSR 0x165)：始终非法
   - `sireg4` (CSR 0x175)：计数器高位（仅 RV32），RV64 下非法
   - `sireg5` (CSR 0x185)：事件选择器高位（仅 RV32），RV64 下非法
   - `sireg6` (CSR 0x195)：始终非法

3. **siselect 范围**：0x40-0x5F 对应委托计数器访问。0x41 始终非法（mtime）。0x40 = cycle，0x42 = instret，0x43-0x5F = hpmcounter3-31。

4. **scountinhibit 与 mcountinhibit**：scountinhibit 是 mcountinhibit 的掩码别名。委托计数器的位在两者之间同步。M-mode 修改 mcountinhibit 会反映到 scountinhibit（对委托位），S-mode 修改 scountinhibit 也会反映到 mcountinhibit。

5. **Hypervisor 场景异常类型**：VS/VU-mode 对 vsiselect/vsireg*/scountovf/scountinhibit 的访问规则与异常类型已迁移至 `Hypervisor_Ss_test_plan.md` Group 11，不在本方案实现。

6. **MINH 位**：bit 62 在通过 sireg* 间接访问 hpmeventN/cyclecfg/instretcfg 时为只读零。这是为了防止 S-mode 控制 M-mode 的计数行为。需要 Sscofpmf 或 Smcntrpmf 扩展才能验证。

7. **RV32 vs RV64**：
   - RV32 下 sireg4/sireg5 用于访问计数器/事件选择器的高 32 位
   - RV64 下 sireg4/sireg5 访问触发 illegal-instruction
   - 测试实现需要根据 XLEN 选择正确的测试路径

任一平台违反 SPEC 时用例保持 FAIL，实现缺陷记录至 `bugs/` 目录。

---

## 附录 A：规范点覆盖矩阵

| Norm ID | 覆盖的测试 ID | 覆盖状态 | 备注 |
|---------|--------------|----------|------|
| `norm:ssccfg_illegal_sireg_cde0` | SSCFG-ILL-01、SSCFG-ILL-02、SSCFG-ILL-03、SSCFG-ILL-04、SSCFG-ILL-07 | 已覆盖 | CDE=0 时 sireg* 全部非法 |
| `norm:ssccfg_illegal_sireg3_6` | SSCFG-ILL-05、SSCFG-ILL-06、SSCFG-ILL-07、SSCFG-ILL-14、SSCFG-ILL-15 | 已覆盖 | sireg3/sireg6 始终非法 |
| `norm:ssccfg_illegal_sireg4_5_xlen64` | SSCFG-ILL-08、SSCFG-ILL-09 | 已覆盖 | RV64 下 sireg4/sireg5 非法 |
| `norm:ssccfg_illegal_sireg_not_delegated` | SSCFG-ILL-10、SSCFG-ILL-11、SSCFG-ILL-12、SSCFG-MAP-11 | 已覆盖 | 未委托或 siselect=0x41 非法 |
| `norm:ssccfg_missing_extension_illegal` | SSCFG-MAP-12、SSCFG-MAP-13 | 已覆盖 | 缺少依赖扩展时非法 |
| `norm:ssccfg_scountinhibit_exists` | SSCFG-SINH-01、SSCFG-SINH-14、SSCFG-SINH-15 | 已覆盖 | scountinhibit 存在性与别名 |
| `norm:ssccfg_scountinhibit_delegated_rw` | SSCFG-SINH-02 ~ SSCFG-SINH-06、SSCFG-SINH-11、SSCFG-SINH-14 ~ SSCFG-SINH-16 | 已覆盖 | 委托位可读写 |
| `norm:ssccfg_scountinhibit_nondelegated_ro` | SSCFG-SINH-07 ~ SSCFG-SINH-11 | 已覆盖 | 非委托位只读零 |
| `norm:ssccfg_illegal_scountinhibit_cde0` | SSCFG-SINH-12、SSCFG-SINH-13 | 已覆盖 | CDE=0 时 scountinhibit 非法 |
| `norm:sscsrind_csrs_access_control` | SSCFG-STA-01、SSCFG-STA-02、SSCFG-STA-03 | 已覆盖 | mstateen0[60] 门控 S-mode 访问 |

**未覆盖规范点说明**：
- 间接 HPM 状态映射表本身（siselect 0x40-0x5F 与计数器状态的对应关系）为 SPEC 表格数据，无独立 norm 标签，通过 Group 1 SSCFG-MAP-01 ~ SSCFG-MAP-10 端到端验证。
- MINH 位（bit 62）通过 sireg* 只读零行为源自 `smcdeleg.adoc` 表格说明与 Sscofpmf/Smcntrpmf 交叉约束，无独立 norm 标签，通过 Group 7 SSCFG-MINH-01 ~ SSCFG-MINH-06 覆盖。
- Hypervisor 相关规范点（`norm:ssccfg_illegal_scountinhibit_vs_vu`、`norm:ssccfg_virtual_scountovf_vs_vu`、`norm:ssccfg_lcofi_hvip_hvien`、`norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal`、`norm:ssccfg_hyp_m_s_vsireg_illegal`、`norm:ssccfg_hyp_vs_access_sireg_conditional`）已迁移至 `Hypervisor_Ss_test_plan.md` Group 11，本方案不重复覆盖。

---

## 参考

- `smcdeleg.adoc` — Smcdeleg and Ssccfg Counter Delegation Extensions
- `smcsrind.adoc` — Smcsrind/Sscsrind Extension（indirect CSR access）
- `smstateen.adoc` — Smstateen Extension
- `sscofpmf.adoc` — Sscofpmf Extension（Counter Overflow and Mode Filtering）
- `smcntrpmf.adoc` — Smcntrpmf Extension（Cycle and Instret Mode Filtering）
- `DOCS/testplan/Hypervisor_Ss_test_plan.md` — Hypervisor 与 Ss* 扩展交叉测试计划（本方案 Hypervisor 用例的迁移目标，Group 11）
- `DOCS/testplan/Smcdeleg_test_plan.md` — Smcdeleg 扩展测试计划（Machine Mode）
