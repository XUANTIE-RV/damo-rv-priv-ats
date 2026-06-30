# Ssccfg 扩展测试计划（Supervisor Mode）

> 本文档描述 Ssccfg（Counter Configuration — Supervisor-level）扩展的测试计划。聚焦于 S-mode 通过 `siselect`/`sireg*` 间接访问委托计数器、`scountinhibit` 寄存器、`scountovf` 虚拟化、以及 Hypervisor 场景下的虚拟化行为。M-mode 层面的 Smcdeleg 行为由 `Smcdeleg_test_plan.md` 覆盖。
>
> 生成时间：2026-06-25

---

## 范围

### 覆盖的 Ssccfg Supervisor-level 行为

- **间接 HPM 映射**：`siselect` 0x40-0x5F 范围通过 `sireg*` 访问委托计数器的状态
- **sireg* 访问规则**：非法访问条件（CDE=0、sireg3/6、sireg4/5 in RV64、未委托计数器、缺少依赖扩展）
- **scountinhibit 寄存器**：委托计数器的计数抑制控制、非委托位的只读零行为
- **scountovf 虚拟化**：Hypervisor 场景下 VS/VU-mode 读 `scountovf` 触发 virtual-instruction
- **LCOFI 虚拟化**：`hvip`/`hvien` bit 13 的实现（Hypervisor 场景）
- **Hypervisor 交互**：`vsiselect`/`vsireg*` 的访问规则、VS-mode 条件访问

### 不在本文档范围

- M-mode 的 `menvcfg.CDE` 使能控制和 `mcounteren` 委托位设置（由 `Smcdeleg_test_plan.md` 覆盖）
- `mstateen0` bit 60 的 M-mode 控制（由 `Smcdeleg_test_plan.md` 覆盖）
- `mvip`/`mvien` LCOFI 位的 M-mode 验证（由 `Smcdeleg_test_plan.md` 覆盖）
- 非委托场景下的 `scounteren` 基本行为（由其他扩展测试计划覆盖）

---

## 规范依据

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:ssccfg_illegal_sireg_cde0` | `smcdeleg.adoc` | Attempts to access any `sireg*` when `menvcfg`.CDE = 0 raise illegal-instruction exceptions. | 当 `menvcfg`.CDE = 0 时，访问任何 `sireg*` 触发 illegal-instruction 异常。 |
| `norm:ssccfg_illegal_sireg3_6` | `smcdeleg.adoc` | Attempts to access `sireg3` or `sireg6` raise illegal-instruction exceptions. | 访问 `sireg3` 或 `sireg6` 触发 illegal-instruction 异常。 |
| `norm:ssccfg_illegal_sireg4_5_xlen64` | `smcdeleg.adoc` | Attempts to access `sireg4` or `sireg5` when XLEN = 64 raise illegal-instruction exceptions. | 当 XLEN = 64 时，访问 `sireg4` 或 `sireg5` 触发 illegal-instruction 异常。 |
| `norm:ssccfg_illegal_sireg_not_delegated` | `smcdeleg.adoc` | Attempts to access `sireg*` when `siselect` = 0x41, or when the counter selected by `siselect` is not delegated to S-mode (mcounteren bit = 0), raise illegal-instruction exceptions. | 当 `siselect` = 0x41 或所选计数器未委托给 S-mode（mcounteren 对应位 = 0）时，访问 `sireg*` 触发 illegal-instruction 异常。 |
| `norm:ssccfg_missing_extension_illegal` | `smcdeleg.adoc` | If any extension upon which the underlying state depends is not implemented, an attempt from M or S mode to access the given state through `sireg*` raises an illegal-instruction exception. | 如果底层状态依赖的任何扩展未实现，从 M 或 S 模式通过 `sireg*` 访问该状态会触发 illegal-instruction 异常。 |
| `norm:ssccfg_scountinhibit_exists` | `smcdeleg.adoc` | Smcdeleg/Ssccfg defines a new `scountinhibit` register, a masked alias of `mcountinhibit`. | Smcdeleg/Ssccfg 定义了新的 `scountinhibit` 寄存器，是 `mcountinhibit` 的掩码别名。 |
| `norm:ssccfg_scountinhibit_delegated_rw` | `smcdeleg.adoc` | For counters delegated to S-mode, the associated `mcountinhibit` bits can be accessed via `scountinhibit`. | 对于委托给 S-mode 的计数器，关联的 `mcountinhibit` 位可通过 `scountinhibit` 访问。 |
| `norm:ssccfg_scountinhibit_nondelegated_ro` | `smcdeleg.adoc` | For counters not delegated to S-mode, the associated bits in `scountinhibit` are read-only zero. | 对于未委托给 S-mode 的计数器，`scountinhibit` 中关联的位为只读零。 |
| `norm:ssccfg_illegal_scountinhibit_cde0` | `smcdeleg.adoc` | When `menvcfg`.CDE=0, attempts to access `scountinhibit` raise an illegal-instruction exception. | 当 `menvcfg`.CDE=0 时，访问 `scountinhibit` 触发 illegal-instruction 异常。 |
| `norm:ssccfg_illegal_scountinhibit_vs_vu` | `smcdeleg.adoc` | When Supervisor Counter Delegation is enabled, attempts to access `scountinhibit` from VS-mode or VU-mode raise a virtual-instruction exception. | 当 Supervisor 计数器委托启用时，从 VS-mode 或 VU-mode 访问 `scountinhibit` 触发 virtual-instruction 异常。 |
| `norm:ssccfg_virtual_scountovf_vs_vu` | `smcdeleg.adoc` | For implementations that support Smcdeleg/Ssccfg, Sscofpmf, and the H extension, when `menvcfg`.CDE=1, attempts to read `scountovf` from VS-mode or VU-mode raise a virtual-instruction exception. | 对于支持 Smcdeleg/Ssccfg、Sscofpmf 和 H 扩展的实现，当 `menvcfg`.CDE=1 时，从 VS-mode 或 VU-mode 读 `scountovf` 触发 virtual-instruction 异常。 |
| `norm:ssccfg_lcofi_hvip_hvien` | `smcdeleg.adoc` | For implementations that support Smcdeleg/Ssccfg, Sscofpmf, Smaia/Ssaia, and the H extension, the LCOFI bit (bit 13) in each of `hvip` and `hvien` is implemented and writable. | 对于支持 Smcdeleg/Ssccfg、Sscofpmf、Smaia/Ssaia 和 H 扩展的实现，`hvip` 和 `hvien` 中的 LCOFI 位（bit 13）已实现且可写。 |
| `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` | `smcdeleg.adoc` | If the hypervisor (H) extension is also implemented, a virtual-instruction exception is raised for attempts from VS-mode or VU-mode to directly access `vsiselect` or `vsireg*`, or attempts from VU-mode to access `siselect` or `sireg*`. | 若实现了 Hypervisor 扩展，从 VS-mode 或 VU-mode 直接访问 `vsiselect` 或 `vsireg*`，或从 VU-mode 访问 `siselect` 或 `sireg*`，触发 virtual-instruction 异常。 |
| `norm:ssccfg_hyp_m_s_vsireg_illegal` | `smcdeleg.adoc` | An attempt to access any `vsireg*` from M or S mode raises an illegal-instruction exception while `vsiselect` holds a value in the range 0x40-0x5F. | 当 `vsiselect` 持有 0x40-0x5F 范围的值时，从 M 或 S 模式访问任何 `vsireg*` 触发 illegal-instruction 异常。 |
| `norm:ssccfg_hyp_vs_access_sireg_conditional` | `smcdeleg.adoc` | An attempt from VS-mode to access any `sireg*` (really `vsireg*`) raises an illegal-instruction exception if `menvcfg`.CDE = 0, or a virtual-instruction exception if `menvcfg`.CDE = 1. | 从 VS-mode 访问任何 `sireg*`（实际为 `vsireg*`），若 `menvcfg`.CDE = 0 触发 illegal-instruction 异常，若 CDE = 1 触发 virtual-instruction 异常。 |

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
> - SSCFG-SINH-14~15 验证 `scountinhibit` 与 `mcountinhibit` 的同步关系——它们共享同一底层状态，只是通过 mcounteren 掩码控制可见性。
> - SSCFG-SINH-16 验证功能正确性：设置 `scountinhibit` CY=1 后，cycle 计数器应被抑制。
> - `scountinhibit` 的 CSR 地址为 0x120。

---

## Group 4. scountovf 虚拟化（Hypervisor 场景）

**规范依据**：
- `norm:ssccfg_virtual_scountovf_vs_vu`：Smcdeleg/Ssccfg + Sscofpmf + H 扩展下，CDE=1 时 VS/VU-mode 读 scountovf 触发 virtual-instruction

**测试职责**：验证在 Hypervisor 场景下，当 Smcdeleg/Ssccfg + Sscofpmf + H 扩展同时实现且 CDE=1 时，VS-mode 和 VU-mode 读 `scountovf` 触发 virtual-instruction 异常。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCFG-OVF-01 | VS-mode 读 scountovf（CDE=1）触发 virtual-instruction | CDE=1，VS-mode 读 scountovf | 触发 virtual-instruction 异常 (cause=22) |
| SSCFG-OVF-02 | VU-mode 读 scountovf（CDE=1）触发 virtual-instruction | CDE=1，VU-mode 读 scountovf | 触发 virtual-instruction 异常 (cause=22) |
| SSCFG-OVF-03 | HS-mode 读 scountovf（CDE=1）正常 | CDE=1，HS-mode（V=0 的 S-mode）读 scountovf | 访问成功，无异常 |
| SSCFG-OVF-04 | VS-mode 读 scountovf（CDE=0）行为 | CDE=0，VS-mode 读 scountovf | 行为取决于 Sscofpmf 的基本规则（不受本虚拟化条款约束） |

> [!NOTE]
> - 本组测试需要 Smcdeleg/Ssccfg、Sscofpmf 和 H 扩展三者同时实现。若任一未实现，应 TEST_SKIP。
> - `scountovf` 的 CSR 地址为 0xDA0（由 Sscofpmf 定义）。在 Hypervisor 场景下，CDE=1 时 VS/VU-mode 的读取会被虚拟化——触发 virtual-instruction 异常让 hypervisor 介入处理。
> - HS-mode（V=0 的 S-mode）读 `scountovf` 不受此虚拟化约束。

---

## Group 5. LCOFI 虚拟化（Hypervisor 场景）

**规范依据**：
- `norm:ssccfg_lcofi_hvip_hvien`：Smcdeleg/Ssccfg + Sscofpmf + Smaia/Ssaia + H 扩展下，`hvip`/`hvien` bit 13 已实现且可写

**测试职责**：验证在 Hypervisor 场景下，当所需扩展全部实现时，`hvip` 和 `hvien` 寄存器中 LCOFI 位（bit 13）的实现与可写性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCFG-HLCOFI-01 | hvip bit 13（LCOFI）可写性 | HS-mode 写 hvip bit 13 = 1 后读回；再写 0 读回 | bit 13 可写且读回一致 |
| SSCFG-HLCOFI-02 | hvien bit 13（LCOFI）可写性 | HS-mode 写 hvien bit 13 = 1 后读回；再写 0 读回 | bit 13 可写且读回一致 |
| SSCFG-HLCOFI-03 | hvip.LCOFI 独立验证 | 写 hvip 全 1 后读回，检查 bit 13 | bit 13 读回为 1 |
| SSCFG-HLCOFI-04 | hvien.LCOFI 独立验证 | 写 hvien 全 1 后读回，检查 bit 13 | bit 13 读回为 1 |
| SSCFG-HLCOFI-05 | vsie/vsip LCOFI 位隐含实现 | 验证 vsie bit 13 和 vsip bit 13 的存在性 | vsie bit 13 和 vsip bit 13 应存在（hvip.LCOFI 的实现隐含了这些位的实现） |

> [!NOTE]
> - 本组测试需要 Smcdeleg/Ssccfg、Sscofpmf、Smaia/Ssaia 和 H 扩展四者同时实现。若任一未实现，应 TEST_SKIP。
> - `hvip`（Hypervisor Virtual Interrupt Pending）和 `hvien`（Hypervisor Virtual Interrupt Enable）由 H 扩展和 Smaia/Ssaia 定义。
> - 规范要求：实现 `hvip`.LCOFI 意味着 `vsie` bit 13 和 `vsip` bit 13 也隐含实现。这确保虚拟 LCOFI 中断可以递送到 VS-mode 的 guest OS。
> - `hvip`/`hvien` 的 CSR 地址分别为 0x645 和 0x648。

---

## Group 6. Hypervisor 交互：vsiselect/vsireg* 访问规则

**规范依据**：
- `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal`：VS/VU-mode 直接访问 vsiselect/vsireg* 触发 virtual-instruction
- `norm:ssccfg_hyp_m_s_vsireg_illegal`：M/S-mode 在 vsiselect 0x40-0x5F 时访问 vsireg* 触发 illegal-instruction
- `norm:ssccfg_hyp_vs_access_sireg_conditional`：VS-mode 访问 sireg*（实际为 vsireg*）的条件异常

**测试职责**：验证 Hypervisor 场景下 vsiselect/vsireg* 的多特权级访问规则。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCFG-HYP-01 | VS-mode 直接访问 vsiselect 触发 virtual-instruction | VS-mode 尝试直接读写 vsiselect CSR | 触发 virtual-instruction 异常 (cause=22) |
| SSCFG-HYP-02 | VS-mode 直接访问 vsireg 触发 virtual-instruction | VS-mode 尝试直接读写 vsireg CSR | 触发 virtual-instruction 异常 (cause=22) |
| SSCFG-HYP-03 | VU-mode 直接访问 vsiselect 触发 virtual-instruction | VU-mode 尝试直接读写 vsiselect CSR | 触发 virtual-instruction 异常 (cause=22) |
| SSCFG-HYP-04 | VU-mode 直接访问 vsireg 触发 virtual-instruction | VU-mode 尝试直接读写 vsireg CSR | 触发 virtual-instruction 异常 (cause=22) |
| SSCFG-HYP-05 | VU-mode 访问 siselect 触发 virtual-instruction | VU-mode 尝试读写 siselect CSR | 触发 virtual-instruction 异常 (cause=22) |
| SSCFG-HYP-06 | VU-mode 访问 sireg 触发 virtual-instruction | VU-mode 尝试读写 sireg CSR | 触发 virtual-instruction 异常 (cause=22) |
| SSCFG-HYP-07 | M-mode 在 vsiselect 0x40-0x5F 时访问 vsireg 非法 | M-mode 设 vsiselect=0x40，访问 vsireg* | 触发 illegal-instruction 异常 |
| SSCFG-HYP-08 | S-mode (HS) 在 vsiselect 0x40-0x5F 时访问 vsireg 非法 | HS-mode 设 vsiselect=0x40，访问 vsireg* | 触发 illegal-instruction 异常 |
| SSCFG-HYP-09 | VS-mode 通过 sireg* 访问（CDE=0）→ illegal-instruction | menvcfg.CDE=0，VS-mode 通过 siselect=0x40 访问 sireg（实际为 vsireg） | 触发 illegal-instruction 异常 |
| SSCFG-HYP-10 | VS-mode 通过 sireg* 访问（CDE=1）→ virtual-instruction | menvcfg.CDE=1，VS-mode 通过 siselect=0x40 访问 sireg（实际为 vsireg） | 触发 virtual-instruction 异常 |

> [!NOTE]
> - 本组测试需要 H 扩展实现。若未实现 H 扩展，所有测试应 TEST_SKIP。
> - VS-mode 和 VU-mode **直接**访问 `vsiselect`/`vsireg*`（使用 CSR 编码）始终触发 virtual-instruction 异常。这是因为这些 CSR 是 hypervisor 管理的，guest 不应直接使用。
> - VS-mode 通过 `siselect`/`sireg*`（guest 视角的 CSR 名称）访问时，硬件实际映射到 `vsiselect`/`vsireg*`。此时的异常类型取决于 `menvcfg.CDE`：
>   - CDE=0 → illegal-instruction（计数器委托未启用）
>   - CDE=1 → virtual-instruction（委托启用但 hypervisor 需要介入管理）
> - M-mode 和 HS-mode 不能直接操作 `vsireg*`（当 `vsiselect` 在 0x40-0x5F 范围内时），应通过修改 VS-mode 的 guest 状态间接管理。
> - `vsiselect` 的 CSR 地址为 0x240，`vsireg` 的 CSR 地址为 0x245。

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
- `norm:smcdeleg_mstateen0_bit60`：`mstateen0` bit 60 = 0 阻止 S-mode 访问 siselect/sireg*
- `hstateen0` bit 60 = 0 阻止 VS-mode 访问 vsiselect/vsireg*

**测试职责**：验证 Smstateen 控制位对 S-mode 间接访问 CSR 的影响。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCFG-STA-01 | mstateen0 bit 60 = 0 阻止 S-mode 写 siselect | mstateen0 bit 60 = 0，S-mode 写 siselect | 触发 illegal-instruction 异常 |
| SSCFG-STA-02 | mstateen0 bit 60 = 0 阻止 S-mode 读 sireg | mstateen0 bit 60 = 0，S-mode 读 sireg | 触发 illegal-instruction 异常 |
| SSCFG-STA-03 | mstateen0 bit 60 = 1 允许 S-mode 访问 | mstateen0 bit 60 = 1，CDE=1，mcounteren[0]=1，S-mode 设 siselect=0x40 读 sireg | 访问成功 |
| SSCFG-STA-04 | hstateen0 bit 60 = 0 阻止 VS-mode 访问 siselect（实际为 vsiselect） | hstateen0 bit 60 = 0，VS-mode 写 siselect | 触发 virtual-instruction 异常 |
| SSCFG-STA-05 | hstateen0 bit 60 = 0 阻止 VS-mode 读 sireg（实际为 vsireg） | hstateen0 bit 60 = 0，VS-mode 读 sireg | 触发 virtual-instruction 异常 |
| SSCFG-STA-06 | hstateen0 bit 60 = 1 允许 VS-mode 访问 | hstateen0 bit 60 = 1，VS-mode 访问 siselect/sireg* | 访问成功（但可能触发 virtual-instruction 如果 CDE=1，由 hypervisor 处理） |

> [!NOTE]
> - 本组测试需要 Smstateen 扩展实现。若未实现，所有测试应 TEST_SKIP。
> - `mstateen0` bit 60（CSRIND）对 S-mode 的控制优先于 `menvcfg.CDE`。即使 CDE=1，若 bit 60 = 0，S-mode 仍无法使用间接访问机制。
> - Hypervisor 场景下，`hstateen0` bit 60 控制 VS-mode 对 `vsiselect`/`vsireg*`（在 guest 视角显示为 `siselect`/`sireg*`）的访问。
> - SSCFG-STA-04~06 需要 H 扩展实现。

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 2 (非法条件) | SSCFG-ILL-01~15 | sireg* 非法访问条件是 Ssccfg 安全保证的核心 |
| P0（必须） | Group 3 (scountinhibit) | SSCFG-SINH-01~16 | scountinhibit 是委托计数器的核心控制寄存器 |
| P1（重要） | Group 1 (映射) | SSCFG-MAP-01~13 | 间接 HPM 映射的正确性是计数器委托功能的基础 |
| P1（重要） | Group 7 (MINH) | SSCFG-MINH-01~06 | MINH 位只读零是 M-mode 计数控制的安全隔离保证 |
| P2（建议） | Group 4 (scountovf) | SSCFG-OVF-01~04 | Hypervisor 场景下 scountovf 虚拟化行为 |
| P2（建议） | Group 5 (LCOFI 虚拟化) | SSCFG-HLCOFI-01~05 | Hypervisor 场景下 LCOFI 中断位虚拟化 |
| P2（建议） | Group 6 (Hypervisor 交互) | SSCFG-HYP-01~10 | vsiselect/vsireg* 多特权级访问规则 |
| P3（可选） | Group 8 (Smstateen) | SSCFG-STA-01~06 | Smstateen 交互是安全隔离的补充保证 |

---

## 测试实现说明

### 文件组织

```
damo-priv-test/
├── ssccfg/
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_ssccfg_mapping.c      # Group 1: siselect 映射
│       ├── test_ssccfg_illegal.c      # Group 2: sireg* 非法条件
│       ├── test_ssccfg_scountinhibit.c # Group 3: scountinhibit
│       ├── test_ssccfg_scountovf.c    # Group 4: scountovf 虚拟化
│       ├── test_ssccfg_lcofi_hyp.c    # Group 5: LCOFI 虚拟化
│       ├── test_ssccfg_hyp_vsireg.c   # Group 6: Hypervisor 交互
│       ├── test_ssccfg_minh.c         # Group 7: MINH 位
│       └── test_ssccfg_stateen.c      # Group 8: Smstateen 交互
└── common/                             # 复用通用框架
```

### 运行时检测

```c
static bool check_ssccfg_extension(void) {
    /* Probe via siselect CSR existence and menvcfg.CDE */
    if (!check_sscsrind_extension()) return false;
    return check_smcdeleg_extension();  /* Smcdeleg/Ssccfg tandem */
}

static bool check_h_extension(void) {
    uint64_t misa = CSRR(misa);
    return (misa & (1UL << ('H' - 'A'))) != 0;
}

static bool check_sscofpmf_extension(void) {
    /* Probe via scountovf CSR existence */
    trap_expect_begin();
    CSRR(scountovf);
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

static bool check_smcntrpmf_extension(void) {
    return platform_has_smcntrpmf();
}
```

### 通用测试模式

#### 模式 1：sireg* 非法条件（Group 2）

```c
/* SSCFG-ILL-01: CDE=0, sireg access illegal */
TEST_REGISTER(test_sscfg_ill_01);
bool test_sscfg_ill_01(void) {
    TEST_BEGIN("SSCFG-ILL-01: CDE=0, sireg access illegal");

    if (!check_ssccfg_extension()) TEST_SKIP("Ssccfg not available");

    uintptr_t orig_cde = CSRR(menvcfg);

    /* Clear CDE */
    CSRW(menvcfg, orig_cde & ~MENVCFG_CDE);

    /* S-mode: siselect=0x40, read sireg -> should trap */
    trap_expect_begin();
    run_in_smode(_s_siselect_sireg_read, 0x40);
    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("illegal-instruction exception",
                       trap_get_cause(), CAUSE_ILLEGAL_INSTRUCTION);
    }
    trap_expect_end();

    /* Restore */
    CSRW(menvcfg, orig_cde);
    TEST_END();
}

/* SSCFG-ILL-05: sireg3 always illegal */
TEST_REGISTER(test_sscfg_ill_05);
bool test_sscfg_ill_05(void) {
    TEST_BEGIN("SSCFG-ILL-05: sireg3 always illegal (CDE=1)");

    if (!check_ssccfg_extension()) TEST_SKIP("Ssccfg not available");

    uintptr_t orig_cde = CSRR(menvcfg);
    uintptr_t orig_ctren = CSRR(mcounteren);

    /* CDE=1, delegate cycle */
    CSRW(menvcfg, orig_cde | MENVCFG_CDE);
    CSRW(mcounteren, orig_ctren | MCOUNTEREN_CY);

    /* S-mode: siselect=0x40, read sireg3 -> should trap */
    trap_expect_begin();
    run_in_smode(_s_siselect_sireg3_read, 0x40);
    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("illegal-instruction for sireg3",
                       trap_get_cause(), CAUSE_ILLEGAL_INSTRUCTION);
    }
    trap_expect_end();

    /* Restore */
    CSRW(menvcfg, orig_cde);
    CSRW(mcounteren, orig_ctren);
    TEST_END();
}
```

#### 模式 2：scountinhibit 寄存器（Group 3）

```c
/* SSCFG-SINH-02: Delegated CY bit writable */
TEST_REGISTER(test_sscfg_sinh_02);
bool test_sscfg_sinh_02(void) {
    TEST_BEGIN("SSCFG-SINH-02: scountinhibit CY bit writable");

    if (!check_ssccfg_extension()) TEST_SKIP("Ssccfg not available");

    uintptr_t orig_cde = CSRR(menvcfg);
    uintptr_t orig_ctren = CSRR(mcounteren);
    uintptr_t orig_sinh = CSRR(scountinhibit);

    /* CDE=1, delegate cycle */
    CSRW(menvcfg, orig_cde | MENVCFG_CDE);
    CSRW(mcounteren, orig_ctren | MCOUNTEREN_CY);

    /* S-mode: write scountinhibit bit 0 = 1 */
    trap_expect_begin();
    run_in_smode(_s_scountinhibit_set_bit, 0);
    TEST_ASSERT("no trap on scountinhibit write", !trap_was_triggered());
    trap_expect_end();

    /* Verify from M-mode */
    uintptr_t sinh_val = CSRR(scountinhibit);
    TEST_ASSERT("scountinhibit bit 0 is 1", (sinh_val & 0x1) != 0);

    /* Clear and verify */
    run_in_smode(_s_scountinhibit_clear_bit, 0);
    sinh_val = CSRR(scountinhibit);
    TEST_ASSERT("scountinhibit bit 0 is 0", (sinh_val & 0x1) == 0);

    /* Restore */
    CSRW(scountinhibit, orig_sinh);
    CSRW(menvcfg, orig_cde);
    CSRW(mcounteren, orig_ctren);
    TEST_END();
}

/* SSCFG-SINH-07: Non-delegated bit read-only zero */
TEST_REGISTER(test_sscfg_sinh_07);
bool test_sscfg_sinh_07(void) {
    TEST_BEGIN("SSCFG-SINH-07: non-delegated CY bit read-only zero");

    if (!check_ssccfg_extension()) TEST_SKIP("Ssccfg not available");

    uintptr_t orig_cde = CSRR(menvcfg);
    uintptr_t orig_ctren = CSRR(mcounteren);
    uintptr_t orig_sinh = CSRR(scountinhibit);

    /* CDE=1, do NOT delegate cycle */
    CSRW(menvcfg, orig_cde | MENVCFG_CDE);
    CSRW(mcounteren, orig_ctren & ~MCOUNTEREN_CY);

    /* S-mode: try to write scountinhibit bit 0 = 1 */
    trap_expect_begin();
    run_in_smode(_s_scountinhibit_set_bit, 0);
    TEST_ASSERT("no trap on scountinhibit write", !trap_was_triggered());
    trap_expect_end();

    /* Verify bit is still 0 (read-only zero) */
    uintptr_t sinh_val = CSRR(scountinhibit);
    TEST_ASSERT("scountinhibit bit 0 is RO0 when not delegated",
                (sinh_val & 0x1) == 0);

    /* Restore */
    CSRW(scountinhibit, orig_sinh);
    CSRW(menvcfg, orig_cde);
    CSRW(mcounteren, orig_ctren);
    TEST_END();
}
```

#### 模式 3：Hypervisor 场景下的 VS-mode 访问（Group 6）

```c
/* SSCFG-HYP-09: VS-mode sireg access (CDE=0) -> illegal */
TEST_REGISTER(test_sscfg_hyp_09);
bool test_sscfg_hyp_09(void) {
    TEST_BEGIN("SSCFG-HYP-09: VS-mode sireg* access (CDE=0) -> illegal");

    if (!check_ssccfg_extension()) TEST_SKIP("Ssccfg not available");
    if (!check_h_extension()) TEST_SKIP("H extension not available");

    uintptr_t orig_cde = CSRR(menvcfg);

    /* CDE=0 */
    CSRW(menvcfg, orig_cde & ~MENVCFG_CDE);

    /* VS-mode: siselect=0x40, read sireg (actually vsireg) */
    trap_expect_begin();
    run_in_vs_mode(_vs_siselect_sireg_read, 0x40);
    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("illegal-instruction when CDE=0",
                       trap_get_cause(), CAUSE_ILLEGAL_INSTRUCTION);
    }
    trap_expect_end();

    /* Restore */
    CSRW(menvcfg, orig_cde);
    TEST_END();
}

/* SSCFG-HYP-10: VS-mode sireg access (CDE=1) -> virtual-inst */
TEST_REGISTER(test_sscfg_hyp_10);
bool test_sscfg_hyp_10(void) {
    TEST_BEGIN("SSCFG-HYP-10: VS-mode sireg* access (CDE=1) -> virtual-inst");

    if (!check_ssccfg_extension()) TEST_SKIP("Ssccfg not available");
    if (!check_h_extension()) TEST_SKIP("H extension not available");

    uintptr_t orig_cde = CSRR(menvcfg);

    /* CDE=1 */
    CSRW(menvcfg, orig_cde | MENVCFG_CDE);

    /* VS-mode: siselect=0x40, read sireg (actually vsireg) */
    trap_expect_begin();
    run_in_vs_mode(_vs_siselect_sireg_read, 0x40);
    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("virtual-instruction when CDE=1",
                       trap_get_cause(), CAUSE_VIRTUAL_INSTRUCTION);
    }
    trap_expect_end();

    /* Restore */
    CSRW(menvcfg, orig_cde);
    TEST_END();
}
```

### 关键注意事项

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

5. **Hypervisor 场景异常类型**：
   - VS/VU-mode **直接**访问 vsiselect/vsireg* → virtual-instruction
   - VS-mode 通过 siselect/sireg* 访问（映射到 vsiselect/vsireg*）：CDE=0 → illegal-instruction，CDE=1 → virtual-instruction
   - M/S-mode 在 vsiselect 0x40-0x5F 时访问 vsireg* → illegal-instruction

6. **MINH 位**：bit 62 在通过 sireg* 间接访问 hpmeventN/cyclecfg/instretcfg 时为只读零。这是为了防止 S-mode 控制 M-mode 的计数行为。需要 Sscofpmf 或 Smcntrpmf 扩展才能验证。

7. **RV32 vs RV64**：
   - RV32 下 sireg4/sireg5 用于访问计数器/事件选择器的高 32 位
   - RV64 下 sireg4/sireg5 访问触发 illegal-instruction
   - 测试实现需要根据 XLEN 选择正确的测试路径

---

## 参考

- `SPEC/smcdeleg.adoc` — Smcdeleg and Ssccfg Counter Delegation Extensions
- `SPEC/sscsrind.adoc` — Sscsrind Extension (indirect CSR access)
- `SPEC/smstateen.adoc` — Smstateen Extension
- `SPEC/sscofpmf.adoc` — Sscofpmf Extension (Counter Overflow and Mode Filtering)
- `SPEC/smcntrpmf.adoc` — Smcntrpmf Extension (Cycle and Instret Mode Filtering)
- `SPEC/smaia.adoc` — Smaia Extension (Advanced Interrupt Architecture)
- `SPEC/hypervisor.adoc` — Hypervisor Extension
- `DOCS/testplan/Smcdeleg_test_plan.md` — Smcdeleg 扩展测试计划（Machine Mode）
- `DOCS/testplan/Hypervisor_cross_test_plan.md` — Hypervisor 交叉测试计划
