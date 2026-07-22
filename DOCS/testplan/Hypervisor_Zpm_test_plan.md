**中文 | [English](../testplan_en/Hypervisor_Zpm_test_plan_en.md)**

# Hypervisor × Zpm (Pointer Masking) 交叉测试计划

> 本文档描述 Hypervisor（H）扩展与 Pointer Masking（Zpm）扩展族在交叉场景下的测试计划。
> Zpm 基础功能（U/S/M-mode 的 ignore 变换、CSR WARL 行为、MPRV/MXR 基础交互等）已由 `zpm_test_plan.md` 覆盖，本文档仅覆盖引入 H 扩展后新增的交叉行为。

---

## 概述

### H 扩展与 Zpm 扩展族的交集分析

Zpm 是一族扩展（`SPEC/zpm.adoc` sec:pm-exts），与 H 扩展的交集按子扩展分别分析如下：

#### H × Ssnpm（核心交集，交集最大）

Ssnpm 定义（`norm:ssnpm_definition`）："A supervisor-level extension that provides pointer masking for the next lower privilege mode (U-mode), **and for VS- and VU-modes if the H extension is present**"。实现 H 扩展后，Ssnpm 新增以下硬件行为：

1. **`henvcfg.PMM`（bits [33:32]）**：控制 VS-mode 的 PM（`norm:henvcfg_pmm_op`）。Ssnpm 未实现时只读零；RV32 只读零。
2. **`hstatus.HUPMM`（bits [49:48]，RV64）**：在 U-mode 执行 `HLV.*`/`HSV.*`（`hstatus.HU=1`）且显式访问按 VU-mode 执行时，控制这些指令的 PM（`hypervisor.adoc` sec:hstatus HUPMM 段）。Ssnpm 未实现时只读零；RV32 只读零。
3. **`senvcfg.PMM` 控制对象扩展为 U/VU**（`norm:senvcfg_pmm_Ssnpm`："next-lower privilege mode (U/VU)"）。`senvcfg` 没有对应的 VS CSR（`norm:H_scsrs_nomatch`），V=1 时 VS-mode 可直接读写 `senvcfg.PMM` 来控制 VU-mode 的 PM。
4. **HLV/HSV 指令的 PM 选择**（sec:hstatus HUPMM 段 + `norm:mstatus_mprv_hlsv`）：HLV/HSV 的显式访问始终按 V=1、名义特权级=`hstatus.SPVP` 执行，PM 设置按有效特权级选择：
   - SPVP=1（按 VS-mode 访问）→ 由 `henvcfg.PMM` 控制（HS/M/U 任何执行模式均如此）；
   - SPVP=0（按 VU-mode 访问）且执行模式为 HS/M → 由 `senvcfg.PMM` 控制；
   - SPVP=0（按 VU-mode 访问）且执行模式为 U（HU=1）→ 由 `hstatus.HUPMM` 控制。
5. **两阶段地址翻译交互**（`hypervisor.adoc` [[pm-two-stage]]）：GPA 比对应 VA 翻译模式宽 2 bit（Sv39x4/Sv48x4/Sv57x4）。当 VS/VU-mode 以 `vsatp.MODE=Bare` 运行时，有效地址是 GPA，这额外的 2 bit 可能被 PM 屏蔽（取决于 `hgatp.MODE` 与 `senvcfg.PMM`/`henvcfg.PMM`）；`vsatp.MODE!=Bare` 时该问题不适用。当 `henvcfg.PMM` 在 "(XLEN-PMLEN) 小于 hgatp 支持的 GPA 宽度" 的取值间切换时（PMLEN=7+sv57x4、PMLEN=16+sv57x4、PMLEN=16+sv48x4），hypervisor 应执行 `HFENCE.GVMA(rs1=x0)`。
6. **GPA 的 ignore 变换为零扩展**（`norm:pm_ignore_pa`）："including guest-physical addresses (i.e., all cases except when the active satp register's MODE field != Bare)"。V=1 时活跃 satp 为 vsatp，故 `vsatp.MODE=Bare` 时 VS/VU 的有效地址按物理地址做 **zero-extend** 变换（而非 VA 的 sign-extend）——这是 H 扩展引入的独特变换路径。

#### H × Smnpm（交集较小）

Smnpm 定义（`norm:smnpm_definition`）："provides pointer masking for the next lower privilege mode (**S/HS** if S-mode is implemented, or U-mode otherwise)"。`norm:menvcfg_pmm_op` 明确 `menvcfg.PMM` 控制 S-/**HS**-mode 的 PM。交集点：

1. HS-mode（hypervisor 自身）的显式访问 PM 由 `menvcfg.PMM` 控制；
2. `menvcfg.PMM` **不**控制 VS/VU-mode（VS 由 `henvcfg.PMM`、VU 由 `senvcfg.PMM` 控制），需验证三级设置相互独立（`norm:pm_per_mode_control`、`norm:pm_mode_only_dependency`）；
3. MPRV 交互（`norm:pm_mprv_spvp` + `norm:mstatus_mprv_hypervisor`）：M-mode 下 `MPRV=1, MPV=0, MPP=S` 时有效特权级为 HS-mode，应用 `menvcfg.PMM`。

#### H × Smmpm（交集最小）

Smmpm 定义（`norm:smmpm_definition`）：`mseccfg.PMM` 仅控制 M-mode 自身的 PM。交集点仅在于虚拟化场景下的"负向"验证：

1. M-mode 下 `MPRV=1, MPV=1` 时有效特权级为 VS/VU（`norm:mstatus_mprv_hypervisor`），此时应用 `henvcfg.PMM`/`senvcfg.PMM`，**不**应用 `mseccfg.PMM`（`norm:pm_mprv_spvp`）；
2. M-mode 执行 HLV/HSV 时 PM 由 `senvcfg.PMM`/`henvcfg.PMM` 控制（按 SPVP），与 `mseccfg.PMM` 无关（`norm:mstatus_mprv_hlsv` + sec:hstatus HUPMM 段）。

#### Sspm / Supm（与 H 扩展无交集）

`norm:sspm_definition`/`norm:supm_definition` 明确二者是**纯 profile 描述扩展**："describe an execution environment but **have no bearing on hardware implementations**"，具体设施由各自执行环境定义。它们不引入任何 CSR 字段或硬件行为，因此与 H 扩展**没有可测试的交集**，本计划不为其编写测试用例（与 `zpm_test_plan.md` 的处理一致）。

### 本文档覆盖范围

- `henvcfg.PMM`、`hstatus.HUPMM` 的 CSR 行为（WARL、只读零条件）
- VS-mode PM（`henvcfg.PMM` 控制，VA sign-extend / GPA zero-extend 两种变换路径）
- VU-mode PM（`senvcfg.PMM` 控制，VS-mode 对 `senvcfg` 的直接访问）
- HLV/HSV 指令的 PM 选择逻辑（SPVP × 执行模式 × HUPMM/senvcfg.PMM/henvcfg.PMM）
- 两阶段地址翻译与 PM 的交互（pm-two-stage，GPA 额外 2 bit，HFENCE.GVMA 同步）
- HS-mode PM（`menvcfg.PMM`）与 VS/VU PM 的独立性
- MPRV+MPV 下的有效特权级 PM 选择
- 虚拟化场景下 trap 相关 PM 行为（vstval/stval 硬件变换、trap vector 不变换、MXR 抑制）

### 由其他测试计划覆盖

- Zpm 基础功能（U/S/M-mode 变换、三 CSR PMM 基础 WARL、MPRV/MXR 基础交互、非虚拟化 stval/mtval）→ `zpm_test_plan.md`
- H 扩展基础功能（CSR、trap、HLV/HSV 异常场景）→ `Hypervisor_test_plan.md`
- 两阶段地址翻译本身 → `Hypervisor_2_stage_test_plan.md`、`Hypervisor_gstage_test_plan.md`

### 不在本文档范围

- **Sspm / Supm**：纯 profile 描述扩展，无硬件行为
- **RV32 场景**：PM 仅适用于 RV64（`norm:pm_rv64_only`），RV32 下 `henvcfg.PMM`/`hstatus.HUPMM` 只读零
- **Debug trigger 地址匹配**（`norm:pm_debug_trigger`）：涉及 Debug 扩展
- **IOMMU/设备访问**（`norm:pm_cpu_only` 的设备侧）：当前框架无设备模型

---

## 规范依据

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:ssnpm_definition` | `zpm.adoc` | A supervisor-level extension that provides pointer masking for the next lower privilege mode (U-mode), and for VS- and VU-modes if the H extension is present. | Ssnpm 为下一级特权模式（U-mode）提供 PM；若实现 H 扩展，还包括 VS/VU-mode。 |
| `norm:smnpm_definition` | `zpm.adoc` | A machine-level extension that provides pointer masking for the next lower privilege mode (S/HS if S-mode is implemented, or U-mode otherwise). | Smnpm 为下一级特权模式（实现 S-mode 时为 S/HS-mode）提供 PM。 |
| `norm:smmpm_definition` | `zpm.adoc` | A machine-level extension that provides pointer masking for M-mode. | Smmpm 为 M-mode 提供 PM。 |
| `norm:sspm_definition` / `norm:supm_definition` | `zpm.adoc` | Extensions that describe an execution environment but have no bearing on hardware implementations. | Sspm/Supm 是纯执行环境描述扩展，无硬件行为（本计划不覆盖）。 |
| `norm:henvcfg_pmm_op` | `hypervisor.adoc` | If the Ssnpm extension is implemented, the PMM field enables or disables pointer masking for VS-mode. When not implemented, PMM is read-only zero. PMM is read-only zero for RV32. | 实现 Ssnpm 时 `henvcfg.PMM` 控制 VS-mode PM；未实现时只读零；RV32 只读零。 |
| (sec:hstatus HUPMM 段) | `hypervisor.adoc` | When Ssnpm is implemented, HUPMM enables/disables PM for HLV.\*/HSV.\* in U-mode when their access is performed as though in VU-mode. In HS- and M-modes, PM for these instructions as though in VU-mode is controlled by senvcfg.PMM; as though in VS-mode by henvcfg.PMM. HUPMM is read-only zero without Ssnpm and for RV32. | HUPMM 仅控制 U-mode 下按 VU 访问的 HLV/HSV；HS/M-mode 下按 VU 访问用 senvcfg.PMM，按 VS 访问用 henvcfg.PMM。 |
| `norm:senvcfg_pmm_Ssnpm` | `supervisor.adoc` | If Ssnpm is implemented, the PMM field enables or disables pointer masking for the next-lower privilege mode (U/VU). If not implemented, PMM is read-only zero. | `senvcfg.PMM` 控制 U/VU-mode PM；未实现 Ssnpm 时只读零。 |
| `norm:menvcfg_pmm_op` | `machine.adoc` | If Smnpm is implemented, the PMM field enables or disables pointer masking for the next-lower privilege mode (S-/HS-mode if S-mode is implemented, or U-mode otherwise). | `menvcfg.PMM` 控制 S/HS-mode PM。 |
| `norm:mseccfg_pmm_presence_op` | `machine.adoc` | If Smmpm is implemented, the PMM field enables or disables pointer masking for M-mode. | `mseccfg.PMM` 控制 M-mode PM。 |
| `norm:pm_ignore_va` | `zpm.adoc` | For virtual addresses, the ignore transformation replaces the upper PMLEN bits with the sign extension of the PMLEN+1st bit. | VA 变换：上 PMLEN 位替换为 bit(XLEN-PMLEN-1) 的符号扩展。 |
| `norm:pm_ignore_pa` | `zpm.adoc` | When applied to a physical address, including guest-physical addresses (all cases except when the active satp's MODE != Bare), the ignore transformation replaces the upper PMLEN bits with 0. | PA（含 GPA，即活跃 satp MODE=Bare 的所有情形）变换：上 PMLEN 位清零。 |
| `norm:pm_apply_explicit` | `zpm.adoc` | When pointer masking is enabled, the ignore transformation will be applied to every explicit memory access. | PM 应用于所有显式内存访问。 |
| `norm:pm_not_apply_implicit` | `zpm.adoc` | The transformation does not apply to implicit accesses such as page-table walks or instruction fetches. | PM 不应用于隐式访问（页表遍历、取指）。 |
| `norm:pm_per_mode_control` | `zpm.adoc` | Pointer masking is controlled separately for different privilege modes. | PM 按特权模式独立控制。 |
| `norm:pm_mode_only_dependency` | `zpm.adoc` | The pointer masking setting that is applied only depends on the active privilege mode, not on the address that is being masked. | 应用的 PM 设置仅取决于当前特权模式，与被屏蔽的地址无关。 |
| `norm:pm_mprv_spvp` | `zpm.adoc` | MPRV and SPVP affect pointer masking as well, causing the pointer masking settings of the effective privilege mode to be applied. | MPRV/SPVP 影响 PM，应用有效特权模式的 PM 设置。 |
| `norm:pm_mxr_exception` | `zpm.adoc` | When MXR is in effect at the effective privilege mode where explicit memory access is performed, pointer masking does not apply. | 有效特权模式下 MXR 生效时 PM 不应用。 |
| `norm:pm_csr_hw_apply` | `zpm.adoc` | Pointer masking, when applicable, is applied for hardware writes to a CSR (e.g., the transformed address to stval when taking an exception). | 硬件写 CSR（如异常时写 stval/vstval）时应用 PM 变换。 |
| `norm:pm_no_trap_vector_mask` | `zpm.adoc` | On trap delivery, pointer masking will not be applied to the address of the trap handler. | trap 投递时 handler 地址不应用 PM。 |
| `norm:pm_no_csr_sw` | `zpm.adoc` | No pointer masking operations are applied when software reads/writes to CSRs. | 软件读写 CSR 不应用 PM。 |
| `norm:pmlen_supported_values` | `zpm.adoc` | The current standard only supports PMLEN=XLEN-48 and PMLEN=XLEN-57. | 仅支持 PMLEN=16（RV64）和 PMLEN=7（RV64）。 |
| `norm:pmlen_illegal_warl` | `zpm.adoc` | Trying to enable pointer masking in an unsupported scenario represents an illegal write and follows WARL semantics. | 不支持的 PMM 写入遵循 WARL 语义。 |
| `norm:H_scsrs_nomatch` | `hypervisor.adoc` | Some standard supervisor CSRs (senvcfg, ...) have no matching VS CSR. These CSRs continue to have their usual function and accessibility even when V=1, with VS/VU-mode substituting for HS/U-mode. | `senvcfg` 无对应 VS CSR，V=1 时保持原有功能与可访问性（VS/VU 替代 HS/U）。 |
| `norm:mstatus_mprv_hlsv` | `hypervisor.adoc` | MPRV does not affect HLV/HLVX/HSV. Their explicit loads/stores always act as though V=1 and the nominal privilege mode were hstatus.SPVP, overriding MPRV. | MPRV 不影响 HLV/HSV；其访问始终按 V=1、特权级=SPVP 执行。 |
| `norm:mstatus_mprv_hypervisor` | `hypervisor.adoc` | When MPRV=1, explicit memory accesses are translated and protected as though the current virtualization mode were set to MPV and the nominal privilege mode were set to MPP. | MPRV=1 时显式访问按 V=MPV、特权级=MPP 翻译与保护。 |
| ([[pm-two-stage]]) | `hypervisor.adoc` | GPAs are 2-bit wider than the corresponding VA translation modes. With vsatp[mode]=Bare in VS/VU mode, those 2 bits may be subject to pointer masking depending on hgatp[mode] and senvcfg/henvcfg[pmm]. If vsatp[mode]!=Bare, this issue does not apply. Hypervisors should execute HFENCE.GVMA(rs1=x0) when henvcfg.PMM changes from/to a value where (XLEN-PMLEN) < GPA width of hgatp.MODE. | GPA 比 VA 模式宽 2 bit；vsatp=Bare 时这 2 bit 可能被 PM 屏蔽；PMM 跨越特定边界变更时需 HFENCE.GVMA 同步。 |

---

## 测试分组

> [!IMPORTANT]
> 共 8 个测试组。VS/VU-mode 测试使用 `common/hyp/hyp_priv.h` 的 `run_in_vs_mode()`/`run_in_vu_mode()`；两阶段场景使用 `common/hyp/two_stage.c` 的 `two_stage_run_in_vs()`；HLV/HSV 使用 `common/hyp/hyp_ldst.h` 的 `hlv_d()`/`hsv_d()`。PM 配置与 tagged 地址构造复用 `common/pm/pm_cfg.h` 与 `common/pm/pm_addr.h`。
>
> 所有用例执行前需探测扩展实现情况（`detect_ssnpm()`/`detect_smnpm()`/`detect_smmpm()` + H 扩展可用性），未实现则 TEST_SKIP，不得为通过测试而降低 SPEC 要求。

---

## Group 1：能力探测与 H 级 PM CSR 字段控制

**规范依据**：
- `norm:henvcfg_pmm_op`：`henvcfg.PMM` 控制 VS-mode PM；Ssnpm 未实现时只读零
- sec:hstatus HUPMM 段：`hstatus.HUPMM` 控制 U-mode 下 HLV/HSV 的 PM；Ssnpm 未实现时只读零
- `norm:pmlen_supported_values` / `norm:pmlen_illegal_warl`：PMM 合法值为 00/10/11，01 保留，WARL 语义
- `norm:H_virtinst_vu_vs_nonhigh_allowedhs_tvm0`：V=1 访问 hypervisor CSR 引发 virtual-instruction 异常

**测试职责**：在 HS-mode 下验证 `henvcfg.PMM`（bits [33:32]）与 `hstatus.HUPMM`（bits [49:48]）的可写性、WARL 约束、字段隔离性，以及 guest（VS-mode）无法修改自身 VS 级 PM 设置。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZPM-CAP-01 | 探测 henvcfg.PMM 实现 | HS-mode 写 henvcfg.PMM=0b11，读回 | Ssnpm 实现时读回非零；未实现时只读零（记录并用于后续 SKIP） |
| HZPM-CAP-02 | 探测 hstatus.HUPMM 实现 | HS-mode 写 hstatus.HUPMM=0b11，读回 | Ssnpm 实现时读回非零；未实现时只读零 |
| HZPM-CAP-03 | henvcfg.PMM 支持的 PMLEN | 分别写 0b10/0b11 到 henvcfg.PMM，读回 | 记录 PMLEN=7/16 支持情况，至少支持其一 |
| HZPM-CAP-04 | hstatus.HUPMM 支持的 PMLEN | 分别写 0b10/0b11 到 hstatus.HUPMM，读回 | 记录 PMLEN=7/16 支持情况 |
| HZPM-CAP-05 | henvcfg.PMM reserved 编码 | 写 PMM=0b01（reserved），读回 | PMM 保持原值（WARL 忽略非法写） |
| HZPM-CAP-06 | hstatus.HUPMM reserved 编码 | 写 HUPMM=0b01，读回 | HUPMM 保持原值 |
| HZPM-CAP-07 | henvcfg.PMM 字段隔离 | 切换 PMM 前后读 FIOM/CBIE/CBCFE/CBZE/PBMTE/ADUE/STCE | 其他字段值不变 |
| HZPM-CAP-08 | hstatus.HUPMM 字段隔离 | 切换 HUPMM 前后读 VSBE/GVA/SPV/SPVP/HU/VGEIN/VTVM/VTW/VTSR/VSXL | 其他字段值不变 |
| HZPM-CAP-09 | Ssnpm 未实现时只读零 | 若 HZPM-CAP-01 探测为未实现，写非零值到 henvcfg.PMM 与 hstatus.HUPMM | 两字段均只读零 |
| HZPM-CAP-10 | VS-mode 不可写 henvcfg.PMM | VS-mode 执行 `csrrw`/`csrrs` 访问 henvcfg | virtual-instruction 异常（cause=22），guest 无法修改自身 VS 级 PM 设置 |

> [!NOTE]
> HZPM-CAP-10 是安全关键用例：VS 级 PM 配置权必须保留在 HS 级（`norm:pm_config_next_higher`）。若 guest 能修改 `henvcfg.PMM`，可绕过 hypervisor 的 PM 策略。

---

## Group 2：H × Ssnpm — VS-mode PM（henvcfg.PMM）

**规范依据**：
- `norm:henvcfg_pmm_op`：`henvcfg.PMM` 控制 VS-mode PM
- `norm:pm_ignore_va`：vsatp.MODE!=Bare 时 VA sign-extend 变换
- `norm:pm_ignore_pa`：vsatp.MODE=Bare 时有效地址为 GPA，zero-extend 变换
- `norm:pm_apply_explicit` / `norm:pm_mode_only_dependency` / `norm:pm_per_mode_control`

**测试职责**：验证 VS-mode 下 tagged load/store/AMO 的 ignore 变换正确性。分两条路径：(a) `vsatp=Sv39`（VA → sign-extend）；(b) `vsatp=Bare` + `hgatp=Sv39x4`（GPA → zero-extend）。PM 配置只能由 HS-mode 通过 `henvcfg.PMM` 完成。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZPM-VS-01 | PMLEN7 VS-mode tagged load（VA 路径） | vsatp=Sv39，henvcfg.PMM=PMLEN7，VS-mode load tagged VA（tag=全1） | load 成功，值与 untagged 地址一致（sign-extend 变换） |
| HZPM-VS-02 | PMLEN16 VS-mode tagged load（VA 路径） | 同上，PMM=PMLEN16 | load 成功，值正确 |
| HZPM-VS-03 | PMLEN7 VS-mode tagged store | vsatp=Sv39，PMM=PMLEN7，VS-mode store via tagged VA | store 成功，untagged 读回值正确 |
| HZPM-VS-04 | PMLEN7 VS-mode AMO | vsatp=Sv39，PMM=PMLEN7，VS-mode amoadd.d via tagged VA | 旧值正确，内存更新正确 |
| HZPM-VS-05 | 不同 tag 访问同一位置 | 同一 base 嵌入 tag=0x55 与 tag=0x7F，VS-mode 分别 load | 两次 load 读到相同值 |
| HZPM-VS-06 | PM 禁用时 tagged VA | henvcfg.PMM=DISABLED，VS-mode load tagged VA | page-fault（tagged VA 无有效映射） |
| HZPM-VS-07 | sign-extend 正确性 | PMLEN7，bit 56=1 的 base 嵌入 tag 后 VS-mode load | 变换后 bits[63:57] 全 1，访问正确地址 |
| HZPM-VS-08 | VS PM 独立于 HS PM | menvcfg.PMM=DISABLED（HS PM 关），henvcfg.PMM=PMLEN7 | VS-mode tagged load 仍成功（VS PM 仅取决于 henvcfg.PMM） |
| HZPM-VS-09 | GPA 路径 zero-extend 变换 | vsatp=Bare + hgatp=Sv39x4，henvcfg.PMM=PMLEN7，VS-mode load tagged GPA | 按 PA 规则 zero-extend（非 sign-extend），访问正确 SPA |
| HZPM-VS-10 | GPA 路径 PM 禁用 | vsatp=Bare + hgatp=Sv39x4，henvcfg.PMM=DISABLED，VS-mode load tagged GPA | guest-page-fault（cause 20/21/23，tagged GPA 超出 hgatp 映射） |

```c
/* HZPM-VS-01 示例：PMLEN7 tagged load in VS-mode (VA path) */
TEST_REGISTER(test_hzpm_vs_pmlen7_load);
bool test_hzpm_vs_pmlen7_load(void) {
    TEST_BEGIN("HZPM-VS-01: PMLEN7 tagged load in VS-mode");
    if (!detect_ssnpm()) TEST_SKIP("Ssnpm not implemented");

    /* HS-mode configures VS-mode PM via henvcfg.PMM */
    write_csr(henvcfg, (read_csr(henvcfg) & ~HENVCFG_PMM_MASK)
                       | (PMM_PMLEN7 << HENVCFG_PMM_OFF));
    if (((read_csr(henvcfg) & HENVCFG_PMM_MASK) >> HENVCFG_PMM_OFF)
        != PMM_PMLEN7)
        TEST_SKIP("PMLEN=7 not supported for VS-mode");

    /* Setup VS-stage Sv39 identity mapping (single-stage, hgatp=Bare) */
    /* ... pt_init(&ctx, SATP_MODE_SV39); pt_setup_identity_mapping(...); ... */

    uintptr_t base = (uintptr_t)&test_data_vs;
    uintptr_t tagged = pm_tag_address(base, pm_max_tag(7), 7);

    uintptr_t result = run_in_vs_mode(vsmode_tagged_load, tagged);
    TEST_ASSERT("correct value via tagged VA", result == TEST_PATTERN_VS);

    write_csr(henvcfg, read_csr(henvcfg) & ~HENVCFG_PMM_MASK);
    TEST_END();
}
```

> [!IMPORTANT]
> HZPM-VS-09/10 是 H 扩展特有的变换路径：`vsatp.MODE=Bare` 时 VS-mode 有效地址是 GPA，按 `norm:pm_ignore_pa` 必须 **zero-extend**（与 VA 的 sign-extend 语义不同）。若实现错误地对 GPA 做 sign-extend，tag 高位为 1 时将访问错误的物理地址。构造用例时需保证 zero-extend 后的 GPA 落在 hgatp 映射范围内。

---

## Group 3：H × Ssnpm — VU-mode PM（senvcfg.PMM）

**规范依据**：
- `norm:senvcfg_pmm_Ssnpm`：`senvcfg.PMM` 控制 U/VU-mode PM
- `norm:H_scsrs_nomatch`：`senvcfg` 无 VS CSR，V=1 时 VS-mode 可直接访问，功能以 VS/VU 替代 HS/U
- `norm:pm_ignore_va` / `norm:pm_ignore_pa` / `norm:pm_per_mode_control`

**测试职责**：验证 VU-mode 下 PM 由 `senvcfg.PMM` 控制，且 VS-mode（guest OS）可直接写 `senvcfg.PMM` 管理其 VU 用户态的 PM；验证 VU 与 VS 的 PM 设置相互独立。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZPM-VU-01 | PMLEN7 VU-mode tagged load | vsatp=Sv39（PTE_U），senvcfg.PMM=PMLEN7，VU-mode load tagged VA | load 成功，值正确 |
| HZPM-VU-02 | PMLEN16 VU-mode tagged load | 同上，PMM=PMLEN16 | load 成功，值正确 |
| HZPM-VU-03 | PMLEN7 VU-mode tagged store | senvcfg.PMM=PMLEN7，VU-mode store via tagged VA | store 成功，值正确 |
| HZPM-VU-04 | VU PM 禁用时 tagged VA | senvcfg.PMM=DISABLED，VU-mode load tagged VA | page-fault |
| HZPM-VU-05 | VS-mode 直接写 senvcfg.PMM | VS-mode 写 senvcfg.PMM=PMLEN7（不经过 HS），再进入 VU-mode tagged load | VS-mode 写成功（无 virtual-instruction），VU tagged load 成功 |
| HZPM-VU-06 | VU PM 独立于 VS PM | henvcfg.PMM=DISABLED（VS PM 关），senvcfg.PMM=PMLEN7（VU PM 开） | VU tagged load 成功；同配置下 VS tagged load fault |
| HZPM-VU-07 | VU-mode GPA 路径 zero-extend | vsatp=Bare + hgatp=Sv39x4，senvcfg.PMM=PMLEN7，VU-mode load tagged GPA | zero-extend 变换，访问正确 SPA |

> [!NOTE]
> HZPM-VU-05 验证 `norm:H_scsrs_nomatch` 在 PM 场景的应用：`senvcfg` 在 V=1 时保持原有可访问性，guest OS（VS-mode）可直接管理其用户态（VU）的 PM 设置，无需 hypervisor 介入。这也是 Linux guest 自行启用 HWASAN 类机制的必要条件。

---

## Group 4：H × Ssnpm — HLV/HSV 指令的 PM 选择

**规范依据**：
- sec:hstatus HUPMM 段：HLV/HSV 的 PM 按有效特权级选择——按 VS 访问用 `henvcfg.PMM`；HS/M-mode 按 VU 访问用 `senvcfg.PMM`；U-mode 按 VU 访问用 `hstatus.HUPMM`
- `norm:mstatus_mprv_hlsv`：HLV/HSV 始终按 V=1、特权级=SPVP 执行，MPRV 不影响
- `norm:pm_mprv_spvp`：SPVP 影响 PM，应用有效特权模式的 PM 设置

**测试职责**：验证 HLV.D/HSV.D 在三种执行模式（HS/M/U）× 两种 SPVP 取值下的 PM 选择逻辑。这是 H × Zpm 交集中最容易实现错误的部分（三个不同 CSR 字段按场景选择）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZPM-HLV-01 | HS-mode SPVP=1 用 henvcfg.PMM | hstatus.SPVP=1，henvcfg.PMM=PMLEN7，HS-mode 执行 hlv_d(tagged_gva) | 按 VS PM 变换，读到正确 guest 数据 |
| HZPM-HLV-02 | HS-mode SPVP=1 PM 禁用 | SPVP=1，henvcfg.PMM=DISABLED，hlv_d(tagged_gva) | fault（tagged 地址未变换，无映射） |
| HZPM-HLV-03 | HS-mode SPVP=0 用 senvcfg.PMM | SPVP=0，senvcfg.PMM=PMLEN7，HS-mode hlv_d(tagged_gva) | 按 VU PM 变换，读到正确数据 |
| HZPM-HLV-04 | HS-mode SPVP=0 时 HUPMM 无效 | SPVP=0，senvcfg.PMM=DISABLED，hstatus.HUPMM=PMLEN7，HS-mode hlv_d(tagged_gva) | fault——HS-mode 下 HUPMM 不生效，仅 senvcfg.PMM 控制 |
| HZPM-HLV-05 | M-mode SPVP=0 用 senvcfg.PMM | M-mode，SPVP=0，senvcfg.PMM=PMLEN7，hlv_d(tagged_gva) | 成功（M-mode 同样用 senvcfg.PMM，非 mseccfg.PMM） |
| HZPM-HLV-06 | M-mode SPVP=1 用 henvcfg.PMM | M-mode，SPVP=1，henvcfg.PMM=PMLEN7，hlv_d(tagged_gva) | 成功 |
| HZPM-HLV-07 | U-mode SPVP=0 用 HUPMM | hstatus.HU=1，SPVP=0，hstatus.HUPMM=PMLEN7，U-mode hlv_d(tagged_gva) | 成功（U-mode 下 HUPMM 生效） |
| HZPM-HLV-08 | U-mode SPVP=0 时 senvcfg.PMM 无效 | HU=1，SPVP=0，HUPMM=DISABLED，senvcfg.PMM=PMLEN7，U-mode hlv_d(tagged_gva) | fault——U-mode 下仅 HUPMM 控制 HLV/HSV 的 PM |
| HZPM-HLV-09 | HSV.D tagged store | SPVP=1，henvcfg.PMM=PMLEN7，HS-mode hsv_d(tagged_gva, val) | store 成功，guest 侧 untagged 读回正确 |
| HZPM-HLV-10 | MPRV 不改变 HLV 的 PM 选择 | M-mode，MPRV=1 且 MPP=M，SPVP=1，henvcfg.PMM=PMLEN7，hlv_d(tagged_gva) | 仍按 SPVP=1（VS）应用 henvcfg.PMM，成功 |

```c
/* HZPM-HLV-04 示例：HS-mode 下 HUPMM 不生效（关键非对称性） */
TEST_REGISTER(test_hzpm_hlv_hupmm_ineffective_in_hs);
bool test_hzpm_hlv_hupmm_ineffective_in_hs(void) {
    TEST_BEGIN("HZPM-HLV-04: HUPMM ineffective for HLV in HS-mode");
    if (!detect_ssnpm()) TEST_SKIP("Ssnpm not implemented");

    /* SPVP=0 (access as VU), senvcfg.PMM off, HUPMM on */
    write_csr(hstatus, (read_csr(hstatus) & ~HSTATUS_SPVP)
                       | HSTATUS_HUPMM_PMLEN7);
    write_csr(senvcfg, read_csr(senvcfg) & ~SENVCFG_PMM_MASK);

    /* Setup guest mapping for the untagged address only ... */
    uintptr_t tagged = pm_tag_address(guest_data_gva, pm_max_tag(7), 7);

    /* Expect fault: in HS-mode, HLV-as-VU uses senvcfg.PMM (=off),
     * so the tagged address is NOT transformed and has no mapping. */
    trap_expect_begin();
    (void)hlv_d(tagged);
    trap_expect_end();
    TEST_ASSERT("fault expected (load/guest-page-fault)",
                trap_get_cause() == CAUSE_LOAD_PAGE_FAULT ||
                trap_get_cause() == CAUSE_LOAD_GUEST_PAGE_FAULT);

    write_csr(hstatus, read_csr(hstatus) & ~HSTATUS_HUPMM_MASK);
    TEST_END();
}
```

> [!WARNING]
> HZPM-HLV-04 与 HZPM-HLV-08 验证 SPEC 的非对称规则：`hstatus.HUPMM` **仅**在 U-mode 执行 HLV/HSV 时生效；HS/M-mode 下按 VU 访问时使用 `senvcfg.PMM`。SPEC 注释要求 hypervisor 在 U-mode 调用 HLV/HSV 前将 guest 写入 `senvcfg.PMM` 的值复制到 `hstatus.HUPMM`。若实现混淆这两个字段的选择条件，将导致 PM 被错误启用或禁用。

---

## Group 5：H × Ssnpm — 两阶段地址翻译与 PM（pm-two-stage）

**规范依据**：
- [[pm-two-stage]]（hypervisor.adoc:2127-2161）：GPA 比 VA 模式宽 2 bit；vsatp=Bare 时额外 2 bit 可能被 PM 屏蔽；`henvcfg.PMM` 跨越 "(XLEN-PMLEN) < GPA 宽度" 边界变更时需 `HFENCE.GVMA(rs1=x0)`；地址特定 HFENCE.GVMA 应忽略地址参数或忽略被屏蔽的 GPA 高位
- `norm:pm_ignore_pa`：GPA zero-extend 变换
- `norm:pm_not_apply_implicit`：页表遍历（含 G-stage walk）不应用 PM

**测试职责**：验证两阶段翻译（hgatp + vsatp）下 PM 与 GPA 额外 2 bit 的交互，以及 PMM 变更后的 TLB 同步要求。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZPM-2STG-01 | GPA 额外 2 bit 被 PM 屏蔽 | hgatp=Sv39x4，vsatp=Bare，henvcfg.PMM=PMLEN7，VS-mode 访问 tag 覆盖 GPA bits[40:39] 的地址 | zero-extend 后访问正确 SPA（额外 2 bit 不影响寻址） |
| HZPM-2STG-02 | vsatp!=Bare 时额外 2 bit 问题不适用 | hgatp=Sv39x4，vsatp=Sv39，henvcfg.PMM=PMLEN7，VS-mode tagged VA load | VA sign-extend 变换后经两阶段翻译访问正确 SPA |
| HZPM-2STG-03 | PMLEN=16 + hgatp=sv48x4 变更同步 | hgatp=Sv48x4（若支持），henvcfg.PMM 从 0 改为 PMLEN16，执行 HFENCE.GVMA(x0,x0) 后 VS-mode tagged GPA 访问 | fence 后按新 PMLEN 正确变换，无陈旧 TLB 项导致的错误 |
| HZPM-2STG-04 | PMLEN=7 + hgatp=sv57x4 变更同步 | hgatp=Sv57x4（若支持），PMM 从 PMLEN16 改为 PMLEN7，HFENCE.GVMA(x0,x0) 后访问 | fence 后行为按新 PMLEN |
| HZPM-2STG-05 | G-stage 页表遍历不受 PM | hgatp 次級页表指针所在 GPA 的高位与 PM 屏蔽位重叠，henvcfg.PMM=PMLEN7 | G-stage walk 使用原始 PTE 地址（不变换），翻译正常 |

> [!NOTE]
> - HZPM-2STG-03/04 依赖平台支持的 hgatp 模式（sv48x4/sv57x4），不支持时 TEST_SKIP。SPEC 要求同步的场景为：PMLEN=7+sv57x4、PMLEN=16+sv57x4、PMLEN=16+sv48x4。
> - HZPM-2STG-05 是负向安全用例：若 PM 错误应用于 G-stage 页表遍历（隐式访问），tag 位重叠的页表指针将被错误变换，导致翻译到错误的页表。

---

## Group 6：H × Smnpm — HS-mode PM（menvcfg.PMM）与独立性

**规范依据**：
- `norm:menvcfg_pmm_op` / `norm:smnpm_definition`：`menvcfg.PMM` 控制 S/HS-mode PM
- `norm:pm_per_mode_control` / `norm:pm_mode_only_dependency`：各特权模式 PM 独立，仅取决于当前活跃模式

**测试职责**：验证 HS-mode（hypervisor 自身）的 PM 由 `menvcfg.PMM` 控制，且与 VS/VU 的 PM 设置（`henvcfg.PMM`/`senvcfg.PMM`）完全独立。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZPM-HS-01 | PMLEN7 HS-mode tagged load（VA） | satp=Sv39，menvcfg.PMM=PMLEN7，HS-mode load tagged VA | sign-extend 变换，load 成功 |
| HZPM-HS-02 | HS-mode Bare tagged load（PA） | satp=Bare，menvcfg.PMM=PMLEN7，HS-mode load tagged PA | zero-extend 变换，load 成功 |
| HZPM-HS-03 | HS PM 独立于 VS PM | menvcfg.PMM=DISABLED，henvcfg.PMM=PMLEN7 | HS-mode tagged load fault；进入 VS-mode 后 tagged load 成功 |
| HZPM-HS-04 | menvcfg.PMM 不影响 VS | menvcfg.PMM=PMLEN7，henvcfg.PMM=DISABLED | VS-mode tagged load fault（menvcfg.PMM 不作用于 VS） |
| HZPM-HS-05 | menvcfg.PMM 不影响 VU | menvcfg.PMM=PMLEN7，senvcfg.PMM=DISABLED | VU-mode tagged load fault（menvcfg.PMM 不作用于 VU） |
| HZPM-HS-06 | PMLEN7 HS-mode AMO | satp=Sv39，menvcfg.PMM=PMLEN7，HS-mode amoadd.d via tagged VA | 旧值正确，内存更新正确 |

---

## Group 7：H × Smmpm — MPRV+MPV 下的有效特权级 PM 选择

**规范依据**：
- `norm:pm_mprv_spvp`：MPRV/SPVP 使有效特权模式的 PM 设置被应用
- `norm:mstatus_mprv_hypervisor`：MPRV=1 时按 V=MPV、特权级=MPP 翻译与保护
- `norm:mseccfg_pmm_presence_op` / `norm:smmpm_definition`：`mseccfg.PMM` 仅控制 M-mode

**测试职责**：验证 M-mode 设置 `mstatus.MPRV=1` 后，load/store 应用由 MPV/MPP 决定的有效特权级的 PM 设置，而非 `mseccfg.PMM`。这是 Smmpm 与 H 扩展的唯一交集路径。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZPM-MPRV-01 | MPRV=1 MPV=1 MPP=S 用 henvcfg.PMM | mseccfg.PMM=DISABLED，henvcfg.PMM=PMLEN7，M-mode MPRV=1/MPV=1/MPP=S tagged load | 按有效 VS-mode 应用 henvcfg.PMM，load 成功 |
| HZPM-MPRV-02 | MPRV=1 MPV=1 MPP=U 用 senvcfg.PMM | mseccfg.PMM=DISABLED，senvcfg.PMM=PMLEN7，MPRV=1/MPV=1/MPP=U tagged load | 按有效 VU-mode 应用 senvcfg.PMM，load 成功 |
| HZPM-MPRV-03 | mseccfg.PMM 不作用于有效 VS | mseccfg.PMM=PMLEN7，henvcfg.PMM=DISABLED，MPRV=1/MPV=1/MPP=S tagged load | fault——M-mode 自身 PM 不应用于有效 VS 访问 |
| HZPM-MPRV-04 | MPRV=1 MPV=0 MPP=S 用 menvcfg.PMM | mseccfg.PMM=DISABLED，menvcfg.PMM=PMLEN7，MPRV=1/MPV=0/MPP=S tagged load | 按有效 HS-mode 应用 menvcfg.PMM，load 成功 |
| HZPM-MPRV-05 | MPRV=0 基线 | MPRV=0，mseccfg.PMM=PMLEN7，M-mode tagged load | 应用 M-mode 自身 PM（与 zpm_test_plan Group 8 一致） |

> [!WARNING]
> MPRV 测试在 M-mode 直接操作 mstatus，修改 MPRV/MPV/MPP 后须立即执行 tagged load 并立即恢复。MPRV=1 期间若触发异常，trap handler 也受 MPRV 影响，可能导致不可预期行为。

---

## Group 8：虚拟化场景 trap 与 PM 交互

**规范依据**：
- `norm:pm_csr_hw_apply`：硬件写 CSR（异常时 vstval/stval）应用 PM 变换
- `norm:pm_no_trap_vector_mask`：trap 投递时 handler 地址（vstvec/stvec）不应用 PM
- `norm:pm_no_csr_sw`：软件读写 CSR 不应用 PM
- `norm:pm_not_apply_implicit`：取指不应用 PM
- `norm:pm_mxr_exception`：有效特权模式 MXR 生效时 PM 不应用

**测试职责**：验证 V=1 场景下异常地址的硬件变换写入、trap vector 不变换、以及 VS-mode MXR 对 PM 的抑制。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HZPM-TRAP-01 | vstval 写入变换后地址 | medeleg+hedeleg 委托 load page-fault 到 VS，henvcfg.PMM=PMLEN7，VS-mode tagged VA 触发 fault | vstval == PM 变换后（sign-extended）的地址 |
| HZPM-TRAP-02 | stval 写入变换后 GVA | hgatp 未映射 tagged GPA 对应区域，vsatp=Bare，henvcfg.PMM=PMLEN7，VS-mode tagged GPA load 触发 guest-page-fault 到 HS | stval == zero-extend 变换后的 GVA，hstatus.GVA=1 |
| HZPM-TRAP-03 | trap 投递不受 PM 影响 | henvcfg.PMM=PMLEN7，vstvec 为正常 handler 地址，VS-mode 触发 ecall | trap 正常投递到 vstvec 原始地址，handler 正确执行 |
| HZPM-TRAP-04 | tagged vstvec 不变换 | 软件将 tagged handler 地址写入 vstvec（`norm:pm_no_csr_sw` 允许），VS-mode 触发 trap | 投递到原始 tagged 地址，取指不变换 → instruction page-fault（证明 PM 未应用于 trap vector） |
| HZPM-TRAP-05 | VS-mode MXR 抑制 PM | henvcfg.PMM=PMLEN7，vsstatus.MXR=1，VS-mode tagged load | PM 不应用，tagged 地址直接访问 → fault |
| HZPM-TRAP-06 | VS-mode 取指不受 PM | henvcfg.PMM=PMLEN7，VS-mode 跳转到 tagged 函数地址 | 取指不变换 → instruction page-fault |

> [!IMPORTANT]
> HZPM-TRAP-01/02 验证 `norm:pm_csr_hw_apply` 在虚拟化两条 trap 路径上的差异：委托到 VS 的异常写 `vstval`（VA，sign-extend）；进入 HS 的 guest-page-fault 写 `stval`（GVA，zero-extend）。两个寄存器的期望变换方式不同，构造期望值时须分别使用 `pm_transform_va()` 与 `pm_transform_pa()`。

---

## 测试用例汇总

| Group | 测试 ID 范围 | 用例数 | 覆盖交集 |
|-------|-------------|--------|---------|
| 1 | HZPM-CAP-01~10 | 10 | 共享（CSR 基础） |
| 2 | HZPM-VS-01~10 | 10 | H × Ssnpm（VS-mode） |
| 3 | HZPM-VU-01~07 | 7 | H × Ssnpm（VU-mode） |
| 4 | HZPM-HLV-01~10 | 10 | H × Ssnpm（HLV/HSV） |
| 5 | HZPM-2STG-01~05 | 5 | H × Ssnpm（两阶段） |
| 6 | HZPM-HS-01~06 | 6 | H × Smnpm（HS-mode） |
| 7 | HZPM-MPRV-01~05 | 5 | H × Smmpm（MPRV+MPV） |
| 8 | HZPM-TRAP-01~06 | 6 | 共享（trap 交互） |
| **合计** | | **59** | |

---

## 依赖的框架组件

| 组件 | 文件 | 用途 |
|------|------|------|
| PM 控制 API | `common/pm/pm_cfg.h` + `pm_cfg.c` | `pm_set_umode()`/`detect_ssnpm()` 等（senvcfg/menvcfg/mseccfg） |
| Tagged Address 工具 | `common/pm/pm_addr.h` | `pm_tag_address()`/`pm_transform_va()`/`pm_transform_pa()` |
| VS/VU-mode 执行 | `common/hyp/hyp_priv.h` + `hyp_priv.c` | `run_in_vs_mode()`/`run_in_vu_mode()` |
| 两阶段翻译 | `common/hyp/two_stage.c` + `two_stage_helpers.h` | `two_stage_run_in_vs()`、hgatp/vsatp 配置 |
| HLV/HSV 指令 | `common/hyp/hyp_ldst.h` + `hyp_ldst.c` | `hlv_d()`/`hsv_d()` 等 |
| VS trap 处理 | `common/hyp/hyp_vs_trap.h` | VS-mode trap 预期与捕获 |
| 页表构造 | `common/vm/satp.c` | `pt_init()`/`pt_setup_identity_mapping()` |
| CSR 定义 | `common/encoding.h` | `HENVCFG_PMM_MASK`/`HSTATUS_HUPMM_MASK`/`PMM_PMLEN7` 等（需补充 HUPMM 相关定义） |

> [!NOTE]
> 实现时需检查 `common/encoding.h` 是否已定义 `hstatus.HUPMM`（bits [49:48]）与 `henvcfg.PMM`（bits [33:32]）的掩码/偏移宏；若缺失需按现有命名规范补充。`pm_cfg.h` 目前仅封装 senvcfg/menvcfg/mseccfg 三级 API，henvcfg.PMM 与 hstatus.HUPMM 的读写可先用 `read_csr`/`write_csr` 直接操作，或按 `pm_cfg` 风格扩展 `pm_set_vsmode()`/`pm_set_hupmm()` 辅助函数。
