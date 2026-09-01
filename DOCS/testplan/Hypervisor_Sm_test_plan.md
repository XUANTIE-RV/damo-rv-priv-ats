**中文 | [English](../testplan_en/Hypervisor_Sm_test_plan_en.md)**

# Hypervisor 与 Sm* 扩展交叉测试计划

> 本文档描述 Hypervisor（H）扩展与其他 Sm* 系列（Machine-level）扩展在交叉场景下的测试计划。本方案从 `Hypervisor_cross_test_plan.md` 拆分而来，仅保留 Hypervisor 与 Sm* 扩展交叉的内容。这些测试场景原本在各扩展的独立测试计划中被标记为"由 Hypervisor 测试计划覆盖"或"因缺少 H 扩展而排除"，但经分析发现现有 Hypervisor 测试计划并未完全覆盖。
>
> 生成时间：2026-06-22

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — Hypervisor（H）扩展：hstateen/hcounteren 对 VS/VU-mode 的控制、virtual-instruction 机制
- `SPEC/riscv-isa-manual/src/priv/smstateen.adoc` — Smstateen：mstateen 对 hstateen 及 Hypervisor CSR 的访问控制
- `SPEC/riscv-isa-manual/src/priv/smcsrind.adoc` — Smcsrind：mstateen0.CSRIND 对 vsiselect/vsireg* 的访问控制
- `SPEC/riscv-isa-manual/src/priv/smctr.adoc` — Smctr：mstateen0.CTR/hstateen0.CTR、MTE 外部陷阱录制
- `SPEC/riscv-isa-manual/src/priv/smcntrpmf.adoc` — Smcntrpmf：mcyclecfg/minstretcfg 的 VSINH/VUINH 计数抑制
- `SPEC/riscv-isa-manual/src/priv/smcdeleg.adoc` — Smcdeleg：Smstateen 与 Smcdeleg/Ssccfg 同时实现时，`mstateen0` bit 60 对 vsiselect/vsireg* 的访问控制
- `SPEC/riscv-isa-manual/src/priv/machine.adoc` — PMP（Physical Memory Protection）：PMP 与分页的交互（pmp-vmem）、隐式页表访问的 PMP 约束、PMP 修改后的同步要求

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内上述路径文件）

---

## 范围

### 覆盖的扩展交叉

- **Hypervisor × Smcsrind**：`mstateen0[60]` (CSRIND) 对 S-mode (HS-mode) 访问 `vsiselect`/`vsireg*` 的控制、M-mode 访问不受 state-enable 控制验证；含 Smcdeleg 视角的同源规范（`norm:smcdeleg_mstateen0_bit60`，从 `Smcdeleg_test_plan.md` 迁入）
- **Hypervisor × Smctr**：`hstateen0.CTR` 对 VS-mode CTR 状态访问的控制、`mstateen0.CTR=0` 对 `vsctrctl` 的阻止、MTE 外部陷阱在 VS/VU-mode 到 M-mode 的录制行为
- **Hypervisor × Smcntrpmf**：`mcyclecfg`/`minstretcfg` 的 VSINH/VUINH 位对 VS/VU-mode cycle/instret 计数的抑制、未实现 H 扩展时 VSINH/VUINH 只读零、`hcounteren` 与计数抑制的正交性
- **Hypervisor × Smstateen**：`mstateen0` 对 hstateen CSR 访问的控制、`mstateen0` 零位传播到 hstateen、各功能位（SE0/ENVCFG/CSRIND/IMSIC/CONTEXT/P1P13）对 Hypervisor CSR 的阻止、VS/VU-mode virtual-instruction
- **Hypervisor × PMP**：两阶段翻译后最终 SPA 仍受机器级 PMP 约束（`norm:H_pmp`）、PMP 对隐式页表遍历的约束（`norm:pmp_with_paging`）、PMP 修改与 G-stage/VS-stage 翻译缓存的同步（HFENCE.GVMA）、hgatp=Bare 时 PMP 为唯一保护机制、HLVX 不能绕过 PMP、PMP 违规陷入时的 htval/GVA/SPV 报告语义

### 不在本文档范围

- 已由 `Hypervisor_CSR_test_plan.md`、`Hypervisor_Interrupts_test_plan.md`、`Hypervisor_Exceptions_test_plan.md`、`Hypervisor_2_stage_test_plan.md`、`Hypervisor_gstage_test_plan.md` 覆盖的 Hypervisor 基础功能
- 各扩展在非 Hypervisor 场景下的行为（由各自独立测试计划覆盖）
- Hypervisor 与 Ss\*/Sv\*/Z\* 扩展的交叉测试（分别由 `Hypervisor_Ss_test_plan.md`、`Hypervisor_Sv_test_plan.md`、`Hypervisor_Zi_test_plan.md` 覆盖）
- Smcsrind M-mode CSR（miselect/mireg\*）的基本功能和 WARL 行为 — 由 `Smcsrind_test_plan.md` 覆盖
- `mstateen0[60]` 对 S-mode 访问 siselect/sireg\*（非 H 扩展 CSR）的控制 — 由 `Smcsrind_test_plan.md` Group 4 覆盖
- PMP/Smepmp/Spmp 在非 Hypervisor 场景下的基础功能 — 分别由 `pmp_test_plan.md`、`Smepmp_test_plan.md`、`spmp_test_plan.md` 覆盖
- 双 Bare 下 PMP 兜底基础场景（GBARE-04）与两阶段翻译后 SPA 被 PMP 拒绝的基础场景（TS-PMP-01）— 已由 `Hypervisor_gstage_test_plan.md`、`Hypervisor_2_stage_test_plan.md` 覆盖，本方案 Group 5 仅补充其未覆盖的增量交集点

---

## 覆盖的规范点

下表列出本方案覆盖的规范点。带 `norm:` 前缀的为 SPEC 官方标签；不带前缀的为从 SPEC 原文拆解出的规范点。各 Group 规范依据中直接引用的规范点（mstateen/hstateen 功能位、CTR、外部陷阱录制等）亦属本方案覆盖范围，统一列入文末附录 A 覆盖矩阵。

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `hstateen_sstateen_zero_initialization` | `smstateen.adoc` | After M-mode software modifies any mstateen CSR, it is responsible for initializing the corresponding hstateen and sstateen CSRs to zero. | M-mode 软件修改任何 mstateen CSR 后，负责将对应的 hstateen 和 sstateen CSR 初始化为零。 |
| `norm:unimplemented_mode_bits` | `smcntrpmf.adoc` | For each bit in 61:58, if the associated privilege mode is not implemented, the bit is read-only zero. | `mcyclecfg`/`minstretcfg` 的 61:58 位中，若对应特权模式未实现，该位为只读零。 |
| `norm:counter_inhibited_behavior` | `smcntrpmf.adoc` | The fundamental behavior of cycle and instret is modified in that counting does not occur while executing in an inhibited privilege mode. | cycle 和 instret 的基本行为被修改：在被抑制的特权模式下执行时不发生计数。 |
| `hcounteren_vs_vu_control` | `hypervisor.adoc` | The `hcounteren` CSR controls availability of performance monitoring counters to VS-mode and VU-mode. | `hcounteren` CSR 控制 VS 和 VU 模式下性能监控计数器的可用性。 |
| `norm:smcdeleg_mstateen0_bit60` | `smcdeleg.adoc` | If extension Smstateen is implemented, setting bit 60 of CSR `mstateen0` to zero prevents access to registers `siselect`, `sireg*`, `vsiselect`, and `vsireg*` from privileged modes less privileged than M-mode. | 若实现了 Smstateen 扩展，将 `mstateen0` 的 bit 60 设为 0 会阻止低于 M-mode 的特权级访问 `siselect`、`sireg*`、`vsiselect` 和 `vsireg*`。 |
| `norm:H_pmp` | `hypervisor.adoc` | Machine-level physical memory protection applies to supervisor physical addresses and is in effect regardless of virtualization mode. | 机器级 PMP 作用于 supervisor 物理地址，与虚拟化模式无关（V=1 时依然生效）。 |
| `norm:pmp_with_paging` | `machine.adoc` | When paging is enabled, instructions that access virtual memory may result in multiple physical-memory accesses, including implicit references to the page tables. The PMP checks apply to all of these accesses. The effective privilege mode for implicit page-table accesses is S. | 启用分页后，指令访问虚拟内存会产生多次物理访问（含对页表的隐式引用），PMP 检查适用于所有这些访问；隐式页表访问的有效特权级为 S。 |
| `norm:pmp_sfence_required` | `machine.adoc` | When the PMP settings are modified, M-mode software must synchronize the PMP settings with the virtual memory system and any PMP or address-translation caches (SFENCE.VMA with rs1=x0 and rs2=x0 after writing PMP CSRs). | 修改 PMP 设置后，M-mode 软件必须将 PMP 设置与虚拟内存系统及翻译缓存同步（写 PMP CSR 后执行 SFENCE.VMA，rs1=x0、rs2=x0）。 |
| `hyp_hfence_gvma_pmp_sync` | `hypervisor.adoc` | Executing an HFENCE.GVMA instruction with rs1=x0 and rs2=x0 suffices to flush all G-stage or VS-stage address-translation cache entries that have cached PMP settings corresponding to the final translated supervisor physical address. An HFENCE.VVMA instruction is not required. | 执行 HFENCE.GVMA（rs1=x0、rs2=x0）足以刷新所有缓存了最终 SPA 对应 PMP 设置的 G-stage/VS-stage 翻译缓存条目；无需 HFENCE.VVMA。 |
| `norm:hgatp_mode_bare` | `hypervisor.adoc` | When MODE=Bare, guest physical addresses are equal to supervisor physical addresses, and there is no further memory protection for a guest virtual machine beyond the physical memory protection scheme described in PMP. | hgatp.MODE=Bare 时 GPA==SPA，guest 虚拟机除 PMP 外无其他内存保护机制。 |
| `hlvx_pmp_override` | `hypervisor.adoc` | HLVX cannot override machine-level physical memory protection (PMP), so attempting to read memory that PMP designates as execute-only still results in an access-fault exception. | HLVX 不能绕过机器级 PMP：读取 PMP 标记为 execute-only 的内存仍产生 access-fault 异常。 |
| `norm:hlsv_priv` | `hypervisor.adoc` | Each HLV/HLVX/HSV instruction performs an explicit memory access with an effective privilege mode of VS or VU: VU when hstatus.SPVP=0, VS when hstatus.SPVP=1. | HLV/HLVX/HSV 的显式内存访问有效特权级由 `hstatus.SPVP` 控制：SPVP=0 为 VU，SPVP=1 为 VS。 |
| `norm:hstatus_spv_op` | `hypervisor.adoc` | The SPV bit is written by the implementation whenever a trap is taken into HS-mode, set to the value of virtualization mode V at the time of the trap. | SPV 位仅在陷入 HS-mode 时由硬件写入，置为陷入时的虚拟化模式 V 值（陷入 M-mode 时以 `mstatus.MPV` 记录）。 |

---

## Group 1. Hypervisor × Smcsrind 交叉测试

**规范依据**：
- `norm:sscsrind_csrs_access_control`：若 Smstateen 与 Smcsrind 同时实现，`mstateen0[60]` (CSRIND) 控制对 `siselect`、`sireg*`、`vsiselect`、`vsireg*` 的访问。当 `mstateen0[60]=0` 时，从低于 M-mode 的特权级访问这些 CSR 触发 illegal-instruction 异常。
- `norm:hypervisor_impl_csrs_access_control`：若 Hypervisor 扩展已实现，`hstateen0[60]` 同样定义，但仅控制 VS/VU-mode 对 `siselect`/`sireg*`（实为 `vsiselect`/`vsireg*`）的访问。当 `hstateen0[60]=0` 且 `mstateen0[60]=1` 时，VS/VU-mode 访问 `siselect`/`sireg*` 触发 virtual-instruction 异常（非 illegal-instruction）。
- `norm:smcdeleg_mstateen0_bit60`：若实现了 Smstateen 扩展，将 `mstateen0` 的 bit 60 设为 0 会阻止低于 M-mode 的特权级访问 `siselect`、`sireg*`、`vsiselect` 和 `vsireg*`。此为 `smcdeleg.adoc` 对同一控制规则的表述（与 `norm:sscsrind_csrs_access_control` 同源）；本方案覆盖其中 `vsiselect`/`vsireg*`（Hypervisor CSR）部分。对 `siselect`/`sireg*` 部分的验证见 `Smcdeleg_test_plan.md` Group 4 与 `Smcsrind_test_plan.md` Group 4。

**测试职责**：验证 Smcsrind 扩展在 Hypervisor 场景下的 CSRIND 访问控制：
- Group 1.1 (01-08)：`mstateen0[60]` 对 S-mode (HS-mode) 访问 `vsiselect`/`vsireg*` 的控制。M-mode 访问不受 state-enable 影响。
- Group 1.2 (09-11)：`hstateen0[60]` 对 VS-mode 访问 `siselect`/`sireg*`（实为 `vsiselect`/`vsireg*`）的控制。当 `hstateen0[60]=0` 且 `mstateen0[60]=1` 时触发 virtual-instruction 异常。

注意：vsiselect/vsireg* 是 H 扩展引入的 CSR，仅在 H 扩展存在时可用。

> **注意**：本组测试从 `Smcsrind_test_plan.md` Group 4 提取而来，专门针对依赖 H 扩展的用例。需要 H 扩展、Smcsrind 扩展和 Smstateen 扩展同时可用。
>
> **前提配置**：
> - Group 1.1：M-mode 需预先将 `mstateen0[60]` 设为所需值以控制 HS-mode 对 vsiselect/vsireg* 的访问。
> - Group 1.2：M-mode 需预先将 `mstateen0[60]` 和 `mstateen0[63]` (SE0) 设为 1，以放行 HS-mode 对 hstateen0 的访问和 CSRIND 控制的状态。

### 测试 ID 映射表

#### Group 1.1: mstateen0[60] 控制 S-mode (HS-mode) 访问

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|--------|
| MCSRIND-STA（新增） | HCROSS-SMCSRIND-01 | mstateen0[60]=0 阻止 S-mode 读 vsiselect |
| MCSRIND-STA（新增） | HCROSS-SMCSRIND-02 | mstateen0[60]=0 阻止 S-mode 写 vsiselect |
| MCSRIND-STA（新增） | HCROSS-SMCSRIND-03 | mstateen0[60]=0 阻止 S-mode 读 vsireg |
| MCSRIND-STA（新增） | HCROSS-SMCSRIND-04 | mstateen0[60]=0 阻止 S-mode 读写 vsireg2~vsireg6 |
| MCSRIND-STA（新增） | HCROSS-SMCSRIND-05 | mstateen0[60]=1 允许 S-mode 访问 vsiselect |
| MCSRIND-STA（新增） | HCROSS-SMCSRIND-06 | mstateen0[60]=1 允许 S-mode 访问 vsireg* |
| MCSRIND-STA-07（补充） | HCROSS-SMCSRIND-07 | mstateen0[60]=0 不影响 M-mode 访问 vsiselect |
| MCSRIND-STA-08（补充） | HCROSS-SMCSRIND-08 | mstateen0[60]=0 不影响 M-mode 访问 vsireg* |

#### Group 1.2: hstateen0[60] 控制 VS-mode 访问

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|--------|
| HCROSS-SMCSRIND-09（补充） | HCROSS-SMCSRIND-09 | hstateen0[60]=0 阻止 VS-mode 读 siselect |
| HCROSS-SMCSRIND-10（补充） | HCROSS-SMCSRIND-10 | hstateen0[60]=0 阻止 VS-mode 读 sireg |
| HCROSS-SMCSRIND-11（补充） | HCROSS-SMCSRIND-11 | hstateen0[60]=1 允许 VS-mode 访问 siselect/sireg |

### 测试用例清单

#### Group 1.1: mstateen0[60] 控制 S-mode (HS-mode) 访问

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SMCSRIND-01 | mstateen0[60]=0 阻止 S-mode 读 vsiselect | mstateen0[60]=0，S-mode (HS-mode) 读 vsiselect (0x250) | 触发 illegal-instruction 异常 (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-02 | mstateen0[60]=0 阻止 S-mode 写 vsiselect | mstateen0[60]=0，S-mode 写 vsiselect | 触发 illegal-instruction 异常 (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-03 | mstateen0[60]=0 阻止 S-mode 读 vsireg | mstateen0[60]=0，S-mode 读 vsireg (0x251) | 触发 illegal-instruction 异常 (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-04 | mstateen0[60]=0 阻止 S-mode 读写 vsireg2~vsireg6 | mstateen0[60]=0，S-mode 逐一读写 vsireg2~vsireg6 | 每个都触发 illegal-instruction 异常 (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-05 | mstateen0[60]=1 允许 S-mode 访问 vsiselect | mstateen0[60]=1，S-mode 读写 vsiselect | 访问正常，无异常 | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-06 | mstateen0[60]=1 允许 S-mode 访问 vsireg* | mstateen0[60]=1，S-mode 读写 vsireg~vsireg6 | 访问不因 mstateen0 被阻止（vsireg2-6 可能因实现可选性触发 illegal-instruction） | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-07 | mstateen0[60]=0 不影响 M-mode 访问 vsiselect | mstateen0[60]=0，M-mode 读写 vsiselect | 访问正常，无异常（state-enable 不控制 M-mode） | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-08 | mstateen0[60]=0 不影响 M-mode 访问 vsireg* | mstateen0[60]=0，M-mode 读写 vsireg~vsireg6 | 访问正常，无异常 | `norm:sscsrind_csrs_access_control` |

#### Group 1.2: hstateen0[60] 控制 VS-mode 访问

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SMCSRIND-09 | hstateen0[60]=0 阻止 VS-mode 读 siselect | mstateen0[60]=1, hstateen0[60]=0，VS-mode 读 siselect (0x150, 实为 vsiselect) | 触发 virtual-instruction 异常 (cause=22) | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SMCSRIND-10 | hstateen0[60]=0 阻止 VS-mode 读 sireg | mstateen0[60]=1, hstateen0[60]=0，VS-mode 读 sireg (0x151, 实为 vsireg) | 触发 virtual-instruction 异常 (cause=22) | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SMCSRIND-11 | hstateen0[60]=1 允许 VS-mode 访问 siselect/sireg | mstateen0[60]=1, hstateen0[60]=1，VS-mode 写 siselect、读 sireg | siselect 访问正常；sireg 不因 hstateen0 被阻止 | `norm:hypervisor_impl_csrs_access_control` |


> [!NOTE]
> - **Group 1.1** 验证 Smcsrind 扩展在 Hypervisor 场景下的 `mstateen0[60]` 访问控制。vsiselect/vsireg* 是 H 扩展引入的 CSR（vsiselect=0x250, vsireg=0x251, vsireg2=0x252, vsireg3=0x253, vsireg4=0x255, vsireg5=0x256, vsireg6=0x257），仅在 H 扩展存在时可用。
> - HCROSS-SMCSRIND-01~04 验证 `mstateen0[60]=0` 时 S-mode (HS-mode, V=0) 访问 vsiselect/vsireg* 触发 illegal-instruction 异常 (cause=2)。04 同时覆盖读和写操作。这与 S-mode 访问 siselect/sireg*（在 `Smcsrind_test_plan.md` Group 4 中覆盖）的行为对称。
> - HCROSS-SMCSRIND-05~06 验证 `mstateen0[60]=1` 时 S-mode 可以正常访问 vsiselect/vsireg*。注意：vsireg2-6 是可选实现的 CSR，当实现不支持时可能触发 illegal-instruction（SPEC 允许），测试以诊断方式输出 trap cause。
> - HCROSS-SMCSRIND-07~08 验证 state-enable CSR **不影响** M-mode 自身的访问。这是 SPEC 明确指出的：state-enable CSR 仅影响低于 M-mode 的特权级。
> - **Group 1.2** 验证 `hstateen0[60]` 对 VS-mode 访问 siselect/sireg*（实为 vsiselect/vsireg*）的控制。与 Group 1.1 的区别：Group 1.1 中 `mstateen0[60]` 控制 HS-mode (V=0) 访问，触发 illegal-instruction (cause=2)；Group 1.2 中 `hstateen0[60]` 控制 VS-mode (V=1) 访问，触发 virtual-instruction (cause=22)。两者是不同层级的控制机制，规范依据分别为 `norm:sscsrind_csrs_access_control` 和 `norm:hypervisor_impl_csrs_access_control`。
> - HCROSS-SMCSRIND-09~10 验证 `hstateen0[60]=0` 且 `mstateen0[60]=1` 时，VS-mode 访问 siselect/sireg 触发 virtual-instruction 异常。注意：异常类型是 virtual-instruction 而非 illegal-instruction，因为 M-mode 已放行（mstateen0=1），但 HS-mode 的 hypervisor 选择不放行（hstateen0=0）。
> - HCROSS-SMCSRIND-11 验证 `hstateen0[60]=1` 时 VS-mode 可以访问 siselect/sireg。sireg 访问可能因 vsiselect 值触发其他异常，但不应触发 virtual-instruction。
> - Group 1.2 前提配置：M-mode 需将 `mstateen0[63]` (SE0) 设为 1 以放行 HS-mode 对 hstateen0 的访问，并将 `mstateen0[60]` (CSRIND) 设为 1 以放行 CSRIND 控制的状态。
> - 与 `Hypervisor_Ss_test_plan.md` 中 Ssstateen Group 4.4 (HCROSS-SSSTA-27~29) 的关系：两者验证相同的 `hstateen0[60]` 控制行为，但 Ssstateen 组从 Ssstateen 角度验证，本组 Group 1.2 从 Smcsrind 角度验证。实现时可交叉引用。
> - 所有测试必须在运行时检测 H 扩展、Smcsrind 扩展和 Smstateen 扩展的可用性，任一不可用则 TEST_SKIP。Group 1.2 还需检测 `hstateen0.CSRIND` 可写性。

---

## Group 2. Hypervisor × Smctr 交叉测试

**规范依据**：
- `norm:hstateen_ctr`：若实现 H 扩展且 `mstateen0.CTR=1`，`hstateen0.CTR` 位控制 V=1 时对 supervisor CTR 状态的访问；`mstateen0.CTR=0` 时 `hstateen0.CTR` 只读零
- `norm:hstateen_vs`：`hstateen0.CTR=0` 时 VS-mode 访问 CTR 状态和 SCTRCLR 触发 virtual-instruction 异常
- `norm:hstateen0_CTR0-V1_op`：`hstateen0.CTR=0` 时 V=1 期间的合格控制转换仍隐式更新 entry 寄存器和 `sctrstatus`
- `norm:mstateen_ctr0_except1`：`mstateen0.CTR=0` 阻止访问 `vsctrctl`
- `norm:exttrap_vsm`：VS-mode 到 M-mode 的外部陷阱需要 MTE + STE
- `norm:exttrap_vum`：VU-mode 到 M-mode 的外部陷阱需要 MTE + STE + vsctrctl.STE
- `norm:exttrap_implreq`：若实现 H 扩展，`vsctrctl.STE` 必须实现

**测试职责**：验证 Smctr 扩展在 Hypervisor 场景下的行为，包括 `hstateen0.CTR` 对 VS-mode CTR 状态访问的控制、`mstateen0.CTR=0` 对 `vsctrctl` 的阻止、以及 MTE 外部陷阱在 VS/VU-mode 到 M-mode 的录制行为。

> **注意**：本组测试从 `Smctr_test_plan.md` Groups 2/3 提取而来，专门针对依赖 H 扩展的用例。需要 H 扩展和 Smctr 扩展同时可用。

### 测试 ID 映射表

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|---------|
| SMCTR-STA-05 | HCROSS-SMCTR-01 | mstateen0.CTR=0 阻止 S-mode 访问 vsctrctl |
| SMCTR-STA-11 | HCROSS-SMCTR-02 | hstateen0.CTR 读写验证 |
| SMCTR-STA-12 | HCROSS-SMCTR-03 | hstateen0.CTR 只读零（mstateen0.CTR=0） |
| SMCTR-STA-13 | HCROSS-SMCTR-04 | hstateen0.CTR=0 阻止 VS-mode 访问 sctrctl |
| SMCTR-STA-14 | HCROSS-SMCTR-05 | hstateen0.CTR=0 阻止 VS-mode 访问 sctrstatus |
| SMCTR-STA-15 | HCROSS-SMCTR-06 | hstateen0.CTR=0 阻止 VS-mode 访问 sireg* |
| SMCTR-STA-16 | HCROSS-SMCTR-07 | hstateen0.CTR=0 阻止 VS-mode 执行 SCTRCLR |
| SMCTR-STA-17 | HCROSS-SMCTR-08 | hstateen0.CTR=1 允许 VS-mode 完整访问 |
| SMCTR-STA-18 | HCROSS-SMCTR-09 | hstateen0.CTR=0 时隐式更新继续 |
| SMCTR-MODE-07 | HCROSS-SMCTR-10 | MTE 外部陷阱录制（VS→M） |
| SMCTR-MODE-08 | HCROSS-SMCTR-11 | MTE 外部陷阱录制（VU→M，需 MTE+STE+vsSTE） |
| SMCTR-MODE-09 | HCROSS-SMCTR-12 | 外部陷阱 VU→M 缺少 vsSTE 时不录制 |

### 测试用例清单

#### 2.1 mstateen0.CTR 对 Hypervisor CSR 的控制

**规范依据**：`norm:mstateen_ctr0_except1`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SMCTR-01 | mstateen0.CTR=0 阻止 S-mode 访问 vsctrctl | mstateen0.CTR=0，HS-mode 尝试读 vsctrctl | 触发 illegal-instruction 异常（cause=2） | `norm:mstateen_ctr0_except1` |

#### 2.2 hstateen0.CTR 访问控制

**规范依据**：`norm:hstateen_ctr`、`norm:hstateen_vs`、`norm:hstateen0_CTR0-V1_op`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SMCTR-02 | hstateen0.CTR 读写验证 | mstateen0.CTR=1，M-mode 写 hstateen0.CTR=1 后读回 | 若实现 H 扩展且 mstateen0.CTR=1，hstateen0.CTR 可写 | `norm:hstateen_ctr` |
| HCROSS-SMCTR-03 | hstateen0.CTR 只读零（mstateen0.CTR=0） | mstateen0.CTR=0，尝试写 hstateen0.CTR=1 | hstateen0.CTR 只读零 | `norm:hstateen_ctr` |
| HCROSS-SMCTR-04 | hstateen0.CTR=0 阻止 VS-mode 访问 sctrctl | mstateen0.CTR=1，hstateen0.CTR=0，VS-mode 访问 sctrctl（实际 vsctrctl） | 触发 virtual-instruction 异常（cause=22） | `norm:hstateen_vs` |
| HCROSS-SMCTR-05 | hstateen0.CTR=0 阻止 VS-mode 访问 sctrstatus | mstateen0.CTR=1，hstateen0.CTR=0，VS-mode 访问 sctrstatus | 触发 virtual-instruction 异常（cause=22） | `norm:hstateen_vs` |
| HCROSS-SMCTR-06 | hstateen0.CTR=0 阻止 VS-mode 访问 sireg* | mstateen0.CTR=1，hstateen0.CTR=0，VS-mode 设 siselect=0x200 后读 sireg | 触发 virtual-instruction 异常（cause=22） | `norm:hstateen_vs` |
| HCROSS-SMCTR-07 | hstateen0.CTR=0 阻止 VS-mode 执行 SCTRCLR | mstateen0.CTR=1，hstateen0.CTR=0，VS-mode 执行 SCTRCLR | 触发 virtual-instruction 异常（cause=22） | `norm:hstateen_vs` |
| HCROSS-SMCTR-08 | hstateen0.CTR=1 允许 VS-mode 完整访问 | mstateen0.CTR=1，hstateen0.CTR=1，VS-mode 分别访问 sctrctl/sctrstatus | 所有访问成功 | `norm:hstateen_vs` |
| HCROSS-SMCTR-09 | hstateen0.CTR=0 时隐式更新继续 | mstateen0.CTR=1，hstateen0.CTR=0，VS-mode 启用录制，执行控制转换，M-mode 检查 entry 寄存器 | entry 寄存器和 sctrstatus 仍被隐式更新 | `norm:hstateen0_CTR0-V1_op` |

#### 2.3 MTE 外部陷阱（VS/VU-mode → M-mode）

**规范依据**：`norm:exttrap_vsm`、`norm:exttrap_vum`、`norm:exttrap_implreq`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SMCTR-10 | MTE 外部陷阱录制（VS→M） | mctrctl.M=0，MTE=1，sctrctl.STE=1，VS-mode 触发陷阱到 M-mode | 外部陷阱被录制（需 MTE 和 STE） | `norm:exttrap_vsm` |
| HCROSS-SMCTR-11 | MTE 外部陷阱录制（VU→M，需 MTE+STE+vsSTE） | mctrctl.M=0，MTE=1，sctrctl.STE=1，vsctrctl.STE=1，VU-mode 触发陷阱到 M-mode | 外部陷阱被录制（需所有三个 TE 位置位） | `norm:exttrap_vum` |
| HCROSS-SMCTR-12 | 外部陷阱 VU→M 缺少 vsSTE 时不录制 | mctrctl.M=0，MTE=1，STE=1，vsctrctl.STE=0，VU-mode 触发陷阱到 M-mode | 外部陷阱不被录制（vsctrctl.STE 未置位） | `norm:exttrap_vum` |


> [!NOTE]
> - 本组测试验证 Smctr 扩展在 Hypervisor 场景下的行为。所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展的可用性，不可用时 TEST_SKIP。
> - HCROSS-SMCTR-01 从 `Smctr_test_plan.md` Group 2 迁移而来，验证 `mstateen0.CTR=0` 对 `vsctrctl`（H 扩展引入的 CSR）的阻止。触发的是 illegal-instruction（cause=2），因为这是 M-mode 对 S-mode 的控制。
> - HCROSS-SMCTR-02~09 从 `Smctr_test_plan.md` Group 2 迁移而来，验证 `hstateen0.CTR` 对 VS-mode CTR 状态访问的控制。核心规则：`hstateen0.CTR=0` 时 VS-mode 访问 CTR 状态触发 virtual-instruction（cause=22），而非 illegal-instruction。这与 `mstateen0.CTR=0` 时触发 illegal-instruction 不同——因为 M-mode 已放行（mstateen0=1），但 HS-mode 选择不放行（hstateen0=0）。
> - HCROSS-SMCTR-09 验证关键语义：即使 `hstateen0.CTR=0` 阻止了 VS-mode 对 CTR CSR 的**显式访问**，V=1 期间执行的合格控制转换仍会**隐式更新** entry 寄存器和 `sctrstatus`。这是为了防止 hypervisor 通过禁用 CTR 来干扰 guest 的转换录制。
> - HCROSS-SMCTR-10~12 从 `Smctr_test_plan.md` Group 3 迁移而来，验证 MTE 外部陷阱在 Hypervisor 场景下的录制。VS→M 需要 MTE+STE；VU→M 需要 MTE+STE+vsctrctl.STE（三个 TE 位）。这体现了外部陷阱录制对中间模式 TE 位的依赖。
> - 与 Group 4 (HCROSS-SMSTA) 的区别：Group 4 验证 `hstateen0` 的 SE0/ENVCFG/CSRIND 等功能位，本组验证 `hstateen0.CTR` 位对 CTR 状态的控制。

---

## Group 3. Hypervisor × Smcntrpmf 交叉测试

本组测试验证 Smcntrpmf（Cycle and Instret Privilege Mode Filtering）扩展在 Hypervisor 场景下的行为，即 `mcyclecfg`/`minstretcfg` 的 VSINH/VUINH 位对 VS/VU-mode 计数的抑制效果，以及与 `hcounteren` 的交互。这些测试从 `Smcntrpmf_test_plan.md` 迁移而来，专门针对依赖 H 扩展的用例。

**规范依据**：
- `norm:unimplemented_mode_bits`：`mcyclecfg`/`minstretcfg` 的 61:58 位中，若对应特权模式未实现，该位为只读零
- `norm:counter_inhibited_behavior`：在被抑制的特权模式下执行时不发生计数
- `hcounteren_vs_vu_control`：`hcounteren` 控制 VS/VU-mode 下性能监控计数器的可用性

**测试职责**：验证 VSINH/VUINH 位对 VS/VU-mode 下 cycle/instret 计数的抑制效果、未实现 H 扩展时的只读零行为、以及 `hcounteren` 访问控制与计数抑制的正交性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PMF-CSR-05 | 未实现 H 扩展时 VSINH/VUINH 只读零 | 若 H 扩展未实现，写 `mcyclecfg`/`minstretcfg` 的 VSINH/VUINH=1，读回 | VSINH/VUINH 为只读零 |
| PMF-CYC-08 | VSINH=1 抑制 VS-mode cycle 计数 | 设 `mcyclecfg.VSINH=1`，VS-mode 执行固定循环，读 cycle 差值 | VS-mode 下 cycle 不递增 |
| PMF-CYC-09 | VUINH=1 抑制 VU-mode cycle 计数 | 设 `mcyclecfg.VUINH=1`，VU-mode 执行固定循环，读 cycle 差值 | VU-mode 下 cycle 不递增 |
| PMF-INS-06 | VSINH=1 抑制 VS-mode instret 计数 | 设 `minstretcfg.VSINH=1`，VS-mode 执行 N 条指令，读 instret 差值 | VS-mode 下 instret 不递增 |
| PMF-INS-07 | VUINH=1 抑制 VU-mode instret 计数 | 设 `minstretcfg.VUINH=1`，VU-mode 执行 N 条指令，读 instret 差值 | VU-mode 下 instret 不递增 |
| PMF-CTR-04 | hcounteren.CY=0 时 VS-mode 不可读 cycle | `hcounteren.CY=0`，VS-mode 读 cycle | virtual-instruction exception (cause=22) |
| HCROSS-PMF-01 | VSINH 抑制与 hcounteren 正交 | `mcyclecfg.VSINH=1`，`hcounteren.CY=1`，VS-mode 执行循环后读 cycle | cycle 可读但值不递增（访问允许但计数被抑制） |

> [!NOTE]
> - PMF-CSR-05 验证 H 扩展**未实现**时 VSINH/VUINH 的只读零行为（`norm:unimplemented_mode_bits`）；在实现 H 扩展的平台上该用例应 TEST_SKIP，由 PMF-CYC-08/09、PMF-INS-06/07 验证 VSINH/VUINH 的可写性与功能。
> - PMF-CYC-08/09、PMF-INS-06/07 需要在 VS/VU-mode 执行计数循环，验证 VSINH/VUINH 的抑制效果。VSINH/VUINH 位与 Sscofpmf 的 `mhpmevent` 字段共用相同的位编码（bit 59/58）。
> - PMF-CTR-04 验证 `hcounteren` 对 VS-mode 计数器访问的控制：`hcounteren.CY=0` 时 VS-mode 读 cycle 触发 virtual-instruction exception（cause=22），而非 illegal-instruction。
> - HCROSS-PMF-01 验证计数抑制（VSINH）与访问控制（`hcounteren`/`mcounteren`）的正交性：两者独立生效，访问允许但模式被抑制时计数器可读但不递增。

### 测试 ID 映射表

| 原始 ID | 新位置 | 测试名称 |
|---------|--------|----------|
| PMF-CSR-05 | Group 3 | 未实现 H 扩展时 VSINH/VUINH 只读零 |
| PMF-CYC-08 | Group 3 | VSINH=1 抑制 VS-mode cycle 计数 |
| PMF-CYC-09 | Group 3 | VUINH=1 抑制 VU-mode cycle 计数 |
| PMF-INS-06 | Group 3 | VSINH=1 抑制 VS-mode instret 计数 |
| PMF-INS-07 | Group 3 | VUINH=1 抑制 VU-mode instret 计数 |
| PMF-CTR-04 | Group 3 | hcounteren.CY=0 时 VS-mode 不可读 cycle |
| —（新增） | Group 3 | HCROSS-PMF-01 VSINH 抑制与 hcounteren 正交 |

### 实现注意事项

1. **H 扩展检测**：测试前需通过 `HAS_H_EXT()`（misa.H）运行时检测 H 扩展可用性。PMF-CYC-08/09、PMF-INS-06/07、PMF-CTR-04、HCROSS-PMF-01 在 H 扩展不可用时 TEST_SKIP；PMF-CSR-05 仅在 H 扩展**未实现**时执行（验证只读零），实现 H 扩展时 TEST_SKIP。

2. **Smcntrpmf 检测**：需先探测 `mcyclecfg`（CSR 0x321）/`minstretcfg`（CSR 0x322）是否实现（trap-protected 写读 MINH 位），未实现时整组 TEST_SKIP。

3. **VS/VU-mode 切换**：需编译时启用 `ENABLE_HYP` 宏，使用 `goto_priv(PRIV_VS)`/`goto_priv(PRIV_VU)` 切换虚拟特权级，并配置两阶段翻译（`hgatp`/`vsatp`）使 VS/VU-mode 可执行计数循环。

4. **计数器读取**：VS/VU-mode 读取 `cycle`/`instret`（CSR 0xC00/0xC02）需 `mcounteren` 与 `hcounteren` 对应位均使能；验证抑制效果时建议在 M-mode 切换前后读取 `mcycle`/`minstret` 取差值，避免在 VS/VU-mode 调用 C 函数。

5. **计数抑制与访问控制正交**：VSINH/VUINH 抑制计数递增，`hcounteren`/`mcounteren` 控制计数器可读性，两者独立。HCROSS-PMF-01 需确保 `mcounteren.CY=1` 且 `hcounteren.CY=1`，再验证 VSINH=1 时计数不递增。

---

## Group 4. Hypervisor × Smstateen

本组测试验证 Smstateen 扩展在 Hypervisor 场景下的行为，包括 hstateen CSR 访问控制、HS-mode/VS-mode/VU-mode 特权级交互等。这些测试从 `smstateen_test_plan.md` 迁移而来，专门针对依赖 H 扩展的用例。

### 测试 ID 映射表

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|---------|
| MSTA-INIT-05 | HCROSS-SMSTA-01 | hstateen0 复位后初始化 |
| MSTA-PROP-02 | HCROSS-SMSTA-02 | mstateen0 零位传播到 hstateen0 |
| MSTA-B63-03 | HCROSS-SMSTA-03 | SE0=0 阻止 HS-mode hstateen0 |
| MSTA-B63-07 | HCROSS-SMSTA-04 | bit 63 可写性条件 |
| MSTA-SE0-02 | HCROSS-SMSTA-05 | SE0=0 阻止 hstateen0 |
| MSTA-SE0-03 | HCROSS-SMSTA-06 | SE0=0 阻止 hstateen0h (RV32) |
| MSTA-ENVCFG-02 | HCROSS-SMSTA-07 | ENVCFG=0 阻止 henvcfg |
| MSTA-CSRIND-03 | HCROSS-SMSTA-08 | CSRIND=0 阻止 vsiselect |
| MSTA-IMSIC-02 | HCROSS-SMSTA-09 | IMSIC=0 阻止 vstopei |
| MSTA-CTX-02 | HCROSS-SMSTA-10 | CONTEXT=0 阻止 hcontext |
| MSTA-P1P13-01 | HCROSS-SMSTA-11 | P1P13=0 阻止 hedelegh |
| MSTA-P1P13-02 | HCROSS-SMSTA-12 | P1P13=1 允许 hedelegh |
| MSTA-EXC-04 | HCROSS-SMSTA-13 | VS-mode virtual-instruction |
| MSTA-EXC-05 | HCROSS-SMSTA-14 | VU-mode virtual-instruction |

### 测试用例清单

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SMSTA-01 | hstateen0 复位后初始化 | M-mode 设置 mstateen0 某些位为 1 后，写零到 hstateen0 | hstateen0 读回为零 | `hstateen_sstateen_zero_initialization` |
| HCROSS-SMSTA-02 | mstateen0 零位传播到 hstateen0 | 将 mstateen0 某功能位设为 0，在 HS-mode 尝试写该位到 hstateen0 | hstateen0 对应位读回为 0 | `norm:mstateen_lower_priv_roz` |
| HCROSS-SMSTA-03 | SE0=0 阻止 HS-mode hstateen0 | 设置 mstateen0 bit 63 为 0，在 HS-mode 访问 hstateen0 | 触发 illegal-instruction 异常 | `norm:mstateen_bit_63_op` |
| HCROSS-SMSTA-04 | bit 63 可写性条件 | 检查 mstateen0 bit 63 是否可写（有 H 扩展或 sstateen 非全只读零） | 满足条件时可写，否则 RO0 | `norm:mstateen_bit_63_roz` |
| HCROSS-SMSTA-05 | SE0=0 阻止 hstateen0 | 设 mstateen0.SE0=0，HS-mode 读 hstateen0 | 触发 illegal-instruction 异常 | `norm:mstateen0_se0_op` |
| HCROSS-SMSTA-06 | SE0=0 阻止 hstateen0h (RV32) | 设 mstateen0.SE0=0，HS-mode 读 hstateen0h | 触发 illegal-instruction 异常 | `norm:mstateen0_se0_op` |
| HCROSS-SMSTA-07 | ENVCFG=0 阻止 henvcfg | 设 ENVCFG=0，HS-mode 读 henvcfg | 触发 illegal-instruction 异常 | `norm:mstateen0_envcfg_op` |
| HCROSS-SMSTA-08 | CSRIND=0 阻止 vsiselect | 设 CSRIND=0，HS-mode 读 vsiselect | 触发 illegal-instruction 异常 | `norm:mstateen0_csrind_op` |
| HCROSS-SMSTA-09 | IMSIC=0 阻止 vstopei | 设 IMSIC=0，HS-mode 读 vstopei | 触发 illegal-instruction 异常 | `norm:mstateen0_imsic_op` |
| HCROSS-SMSTA-10 | CONTEXT=0 阻止 hcontext | 设 CONTEXT=0，HS-mode 读 hcontext | 触发 illegal-instruction 异常 | `norm:mstateen0_context_op` |
| HCROSS-SMSTA-11 | P1P13=0 阻止 hedelegh | 设 P1P13=0，HS-mode 读 hedelegh | 触发 illegal-instruction 异常 | `norm:mstateen0_p1p13_op` |
| HCROSS-SMSTA-12 | P1P13=1 允许 hedelegh | 设 P1P13=1，HS-mode 读 hedelegh | 访问正常 | `norm:mstateen0_p1p13_op` |
| HCROSS-SMSTA-13 | VS-mode virtual-instruction | mstateen0 某位=0 且从 VS-mode 访问，满足虚拟指令异常条件 | 触发 virtual-instruction 异常 (cause=22) | `norm:stateen_illegal_state_access` |
| HCROSS-SMSTA-14 | VU-mode virtual-instruction | mstateen0 某位=0 且从 VU-mode 访问，满足虚拟指令异常条件 | 触发 virtual-instruction 异常 (cause=22) | `norm:stateen_illegal_state_access` |


### 关键注意事项

1. **H 扩展检测**：所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展的可用性，不可用时 TEST_SKIP。

2. **HS-mode 与 S-mode 的关系**：在 H 扩展存在且 V=0 时，S-mode 实际上就是 HS-mode。测试中使用 `goto_priv(PRIV_S)` 来模拟 HS-mode 访问。

3. **VS-mode/VU-mode 测试**：需要编译时启用 `ENABLE_HYP` 宏，并使用 `goto_priv(PRIV_VS)` 或 `goto_priv(PRIV_VU)` 切换到虚拟特权级。

4. **virtual-instruction 与 illegal-instruction 的区分**：VS/VU-mode 访问受控 CSR 时，若 hstateen 允许但 mstateen 阻止，应触发 virtual-instruction (cause=22)；若 hstateen 也阻止，则触发 illegal-instruction (cause=2)。

5. **hstateen CSR 地址**：hstateen0-3 的 CSR 地址为 0x60C-0x60F，hstateen0h-3h (RV32) 为 0x61C-0x61F。

---

## Group 5. Hypervisor × PMP 交叉测试

**规范依据**：
- `norm:H_pmp`：机器级物理内存保护作用于 supervisor 物理地址，与虚拟化模式无关（V=1 时依然生效）。即 VS/VU-mode 访问经两阶段翻译得到最终 SPA 后，仍必须通过机器级 PMP 检查。
- `norm:pmp_with_paging`：启用分页后，指令访问虚拟内存会产生多次物理访问，包括对页表的隐式引用，PMP 检查适用于所有这些访问；隐式页表访问的有效特权级为 S。因此在 H 扩展下，VS-stage 与 G-stage 页表遍历均受 PMP 约束。
- `norm:pmp_sfence_required`：修改 PMP 设置后，M-mode 软件必须将 PMP 设置与虚拟内存系统及翻译缓存同步（写 PMP CSR 后执行 SFENCE.VMA，rs1=x0、rs2=x0）。
- `hyp_hfence_gvma_pmp_sync`：实现 H 扩展时，与 G-stage/VS-stage 数据结构的同步同样必要；执行 HFENCE.GVMA（rs1=x0、rs2=x0）足以刷新所有缓存了最终 SPA 对应 PMP 设置的 G-stage/VS-stage 翻译缓存条目，无需 HFENCE.VVMA。
- `norm:hgatp_mode_bare`：hgatp.MODE=Bare 时 GPA==SPA，guest 虚拟机除 PMP 外无其他内存保护机制。
- `hlvx_pmp_override`：HLVX 不能绕过机器级 PMP，读取 PMP 标记为 execute-only 的内存仍产生 access-fault 异常。

**测试职责**：验证机器级 PMP 在 Hypervisor 场景下的交集行为：
- Group 5.1（01~04）：两阶段翻译成功后，最终 SPA 被 PMP 拒绝时，VS/VU-mode 显式访问（load/store/fetch）产生 access fault 而非 page fault/guest page fault。
- Group 5.2（05~06）：PMP 对隐式页表遍历（VS-stage、G-stage walk）的约束；PMP 违规始终产生 access fault（`norm:pmp_access_fault_exception`），`norm:H_vm_gpatrans` 仅将 page fault 转换为 guest page fault，不覆盖 access fault。
- Group 5.3（07）：修改 PMP 设置后，HFENCE.GVMA 对缓存了 PMP 属性的 G-stage/VS-stage 翻译缓存的同步必要性。
- Group 5.4（08）：HLVX 不能绕过 PMP（execute-only 区域仍 access fault）。

> **注意**：本组仅覆盖既有方案未覆盖的增量交集点。双 Bare 下 PMP 兜底基础场景（GBARE-04）由 `Hypervisor_gstage_test_plan.md` 覆盖；两阶段翻译后 SPA 被 PMP 拒绝的 load/store 基础场景（TS-PMP-01）由 `Hypervisor_2_stage_test_plan.md` Group 19 覆盖，本组在其基础上补充 fetch、VU-mode、隐式遍历、栅栏同步、HLVX 等增量维度。
>
> **前提配置**：
> - 需要 H 扩展与 PMP（至少一个 entry 可用）同时实现；运行时探测，任一不可用则 TEST_SKIP。
> - 默认配置两阶段翻译（`vsatp` 与 `hgatp` 均非 Bare），测试页的 VS-stage 与 G-stage PTE 权限均放行，仅由 PMP 控制目标区域。
> - PMP 检查针对的是最终 SPA，测试前需明确目标 GPA→SPA 映射，用 SPA 配置 PMP 条目。

### 测试用例清单

#### Group 5.1：两阶段翻译后 SPA 的 PMP 约束（显式访问）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-PMP-01 | VS-mode load 被 PMP 拒绝 | 两阶段翻译均放行，目标最终 SPA 被 PMP 配置为不可访问（无匹配条目或 R=0），VS-mode load | load access fault (cause=5)，且不属于 {13, 21}（非 page fault/guest page fault） | `norm:H_pmp` |
| HCROSS-PMP-02 | VS-mode store 被 PMP 拒绝 | 同上配置，VS-mode store | store access fault (cause=7)，且不属于 {15, 23} | `norm:H_pmp` |
| HCROSS-PMP-03 | VS-mode fetch 被 PMP 拒绝 | 两阶段翻译均放行，目标代码页最终 SPA 被 PMP 配置为 X=0，VS-mode 跳转到该页执行 | instruction access fault (cause=1)，且不属于 {12, 20}；陷阱未委托时递送到 M-mode，报告 `mstatus.MPV==1` | `norm:H_pmp` |
| HCROSS-PMP-04 | VU-mode 访问被 PMP 拒绝 | 同 01 配置，VU-mode load | load access fault (cause=5)；若该 cause 已委托给 HS（`hedeleg` 相应位置位），陷阱递送到 HS-mode 且 `hstatus.SPV==1` | `norm:H_pmp` |

#### Group 5.2：PMP 对隐式页表遍历的约束

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-PMP-05 | VS-stage 页表遍历被 PMP 拒绝 | 两阶段翻译页表权限均放行，将承载 VS-stage 页表（经 G-stage 翻译后）的物理页配置为 PMP 不可访问，VS-mode 访问触发 walk | VS-mode 访问报 load access fault (cause=5)：隐式读取经 G-stage 翻译成功后，PMP 检查失败属物理保护违规（非翻译失败），不产生 guest page fault；恢复 PMP 后访问成功 | `norm:pmp_with_paging`、`norm:H_pmp` |
| HCROSS-PMP-06 | G-stage 页表遍历被 PMP 拒绝 | 将承载 G-stage 页表（hgatp 根/中间级页表）的 SPA 配置为 PMP 不可访问，以 `hgatp` 激活状态在 HS-mode 执行 HLV 触发 G-stage walk | 访问报 load access fault (cause=5)（失败发生在翻译过程中的物理保护检查）；接受实现因 walk 失败点在翻译内部而报 load guest page fault (cause=21) 的情形并诊断；恢复 PMP 后访问成功 | `norm:pmp_with_paging`、`norm:H_pmp` |

#### Group 5.3：PMP 修改与翻译缓存同步（HFENCE.GVMA）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-PMP-07 | 收紧 PMP 后 HFENCE.GVMA 同步 | 初始两阶段翻译与 PMP 均放行，VS-mode 访问成功（使翻译缓存生效）；M-mode 收紧 PMP 拒绝目标 SPA，依次执行 SFENCE.VMA (x0,x0) 与 HFENCE.GVMA (x0,x0)，再次进入 VS-mode 访问 | 第二次访问报 access fault（load 为 cause=5），证明缓存了旧 PMP 属性的翻译缓存条目已被刷新 | `norm:pmp_sfence_required`、`hyp_hfence_gvma_pmp_sync` |

#### Group 5.4：HLVX 与 PMP

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-PMP-08 | HLVX 不能绕过 PMP execute-only | 目标页在 VS-stage 与 G-stage 页表中均标记 X=1、U=1，HLVX 以 VU 有效特权（`hstatus.SPVP`=0）执行；PMP 将该页最终 SPA 配置为仅 X（R=0，MML=0 非锁定条目对 S/U 级生效）；先验证 PMP 全放行时 HLVX 可成功（sanity），再验证 X-only 下 HLVX 失败，恢复 PMP 后同访问再次成功（隔离归因） | X-only PMP 下 HLVX 报 load access fault (cause=5)；sanity 与隔离步骤均成功 | `hlvx_pmp_override` |

> [!NOTE]
> - HCROSS-PMP-01~04 的关键断言是异常**类型**：PMP 违规产生 access fault（1/5/7）而非 page fault（12/13/15）或 guest page fault（20/21/23），因为两阶段翻译已成功，失败发生在翻译后的物理地址保护检查阶段。
> - HCROSS-PMP-05/06 与显式访问的关键差异在于失败**位置**（翻译过程中的隐式页表读取）而非异常类型：PMP 违规始终产生 access fault（`norm:pmp_access_fault_exception`）；`norm:H_vm_gpatrans` 仅将 page fault 转换为 guest page fault，不覆盖 access fault。若实现报出 guest page fault 则违反 SPEC（06 中 walk 失败点位于 G-stage 翻译机构内部，规范未明确约束其报告形式，允许 cause=21 但须诊断）。
> - HCROSS-PMP-05 的 PMP 拒绝对象是 VS-stage 页表经 G-stage 翻译后的**最终物理地址**（隐式访问本身也先经过 G-stage 翻译）；配置时须用页表实际落地的 SPA 而非 GPA。
> - HCROSS-PMP-06 采用 HS-mode HLV（hgatp 激活、V=0）触发 G-stage walk 而非 VS-mode 显式访问：G-stage 中间级页表与内核映像通常位于同一 1GB 窗口，在 VS-mode 下拒绝该页会使进入 VS 的 trampoline 取指即故障且无法恢复；HLV 触发同一条 G-stage 遍历路径，且陷阱在 HS/M 上下文可正常恢复。
> - HCROSS-PMP-07 验证翻译缓存可能缓存 PMP 属性（`norm:pmp_speculative_translations` 允许）这一前提下的同步义务：缺少 HFENCE.GVMA（或任何等价同步）时，实现允许继续使用旧的已缓存翻译（含旧 PMP 属性），因此本用例必须以“完成同步后必须失败”为断言，不能以“未同步时失败”为断言。
> - HCROSS-PMP-08 采用 VU 有效特权（SPVP=0）+ 两级 U=1 页表，使页表权限检查全部放行、PMP 成为唯一可能的拒绝源（`norm:hlsv_priv`）；隔离归因采用“PMP 全放行时 HLVX 成功（sanity）→ X-only 拒绝 → 恢复后再次成功”三步对照，而非 VS-mode 取指对照（U=1 页表禁止 S 级取指，不可用）。
> - 所有 PMP 违规类用例需断言陷入上下文的虚拟化来源：未委托时陷阱递送到 M-mode，`hstatus.SPV` 仅在陷入 HS-mode 时由硬件写入（`norm:hstatus_spv_op`），M-mode 递送下应以 `mstatus.MPV==1` 断言；委托给 HS 时方以 `hstatus.SPV==1` 断言。对 htval/GVA 的断言限于 SPEC 明确的情形（guest page fault 时报告 guest 物理地址信息），不对 access fault 下 GVA/htval 的实现选择作强制断言。
> - 修改任何 PMP CSR 后，测试自身也须执行 SFENCE.VMA (x0,x0)（及需要时 HFENCE.GVMA）以同步本测试环境自身的翻译缓存。
> - 运行时探测项：H 扩展（`HAS_H_EXT()`）、PMP entry 数量与粒度（pmpcfg/pmpaddr trap-protected 探测）、两阶段翻译可用性；任一不满足则 TEST_SKIP。若平台启用 Smepmp（mseccfg.MML=1）且无法恢复默认规则，PMP 权限语义改变，本组用例应 TEST_SKIP 并诊断。
> - PMP 与 Smepmp/Spmp 的交集分析：Smepmp（mseccfg.MML/MMWP/RLB）重定义的是 S/U 级访问规则，随 `norm:H_pmp` 传递性适用于 VS/VU 访问与隐式页表遍历，但 SPEC 无显式 H×Smepmp 规范条文，不在本组覆盖；Spmp 非本仓库 SPEC 收录的标准扩展，无规范依据，不立项。
> - 实现建议归入 `Sv39x4_Sv39`（或对应两阶段翻译测试目录）下的独立 `test_hyp_pmp.c`，复用 `pmp/` 模块的 PMP 配置 API 与框架 trap 断言机制；用例需显性失败（ASSERT）并继续执行，失败时与 SPEC 比对报告而非适配实现。

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P1（重要） | Group 4 (Smstateen) | HCROSS-SMSTA-01~14 | hstateen 控制和 VS/VU-mode 异常行为是 Hypervisor 状态隔离的关键保证 |
| P1（重要） | Group 1 (Smcsrind) | HCROSS-SMCSRIND-01~08 | mstateen0[60] 对虚拟化 CSR（vsiselect/vsireg*）的访问控制是安全隔离的关键保证 |
| P1（重要） | Group 2.2 (hstateen0.CTR) | HCROSS-SMCTR-02~09 | hstateen0.CTR 对 VS-mode CTR 访问的控制是虚拟化状态隔离的保证 |
| P2（建议） | Group 2.1 (mstateen0.CTR Hyp) | HCROSS-SMCTR-01 | mstateen0.CTR 对 vsctrctl 的阻止 |
| P2（建议） | Group 2.3 (MTE Hyp) | HCROSS-SMCTR-10~12 | MTE 外部陷阱在 VS/VU-mode 的录制行为 |
| P1（重要） | Group 5.1/5.2 (PMP 交集) | HCROSS-PMP-01~06 | `norm:H_pmp` 与 `norm:pmp_with_paging` 是 guest 内存隔离的最后一道防线；显式访问与隐式页表遍历的异常类型区分是高频实现错误点 |
| P2（建议） | Group 5.3/5.4 (PMP 同步与 HLVX) | HCROSS-PMP-07~08 | 翻译缓存 PMP 属性同步与 HLVX 边界行为，依赖实现缓存结构，平台相关性较强 |

> 注：Smcntrpmf（Group 3）的测试用例（PMF-CSR-05、PMF-CYC-08/09、PMF-INS-06/07、PMF-CTR-04、HCROSS-PMF-01）在原始合并方案中未单独标注优先级，建议参照 `Smcntrpmf_test_plan.md` 的优先级执行。

---

## 关键注意事项

1. **扩展检测**：所有测试必须在运行时检测所需扩展（H、Smcsrind、Smctr、Smcntrpmf、Smstateen 等）的可用性，不可用时 TEST_SKIP。

2. **state-enable 层级控制**：`mstateen` 控制 HS-mode 及以下对扩展状态的访问（触发 illegal-instruction），`hstateen` 控制 VS/VU-mode 的访问（触发 virtual-instruction）。两者独立运作，M-mode 放行（mstateen=1）但 HS-mode 不放行（hstateen=0）时，VS/VU-mode 触发 virtual-instruction（cause=22）。

3. **M-mode 不受 state-enable 控制**：state-enable CSR 仅影响低于 M-mode 的特权级，M-mode 自身的访问不受影响。

4. **virtual-instruction 与 illegal-instruction 的区分**：测试断言必须使用准确的 cause 常量。

5. **PMP 交集的异常类型区分**：无论显式访问还是隐式页表遍历，PMP 违规均报 access fault（1/5/7）；guest page fault（20/21/23）仅产生于翻译本身失败（页表项无效/权限不足），`norm:H_vm_gpatrans` 的 page fault→guest page fault 转换不覆盖 access fault。修改 PMP 后须按规范执行同步栅栏（SFENCE.VMA 及 HFENCE.GVMA）。

---

## 参考

- `SPEC/hypervisor.adoc` — RISC-V Hypervisor Extension, Version 1.0
- `SPEC/smstateen.adoc` — Smstateen Extension Specification
- `SPEC/smcsrind.adoc` — Smcsrind/Sscsrind Extension for Indirect CSR Access
- `SPEC/smctr.adoc` — Smctr (Control Transfer Records - Machine-level) Extension
- `SPEC/smcntrpmf.adoc` — Smcntrpmf (Cycle and Instret Privilege Mode Filtering) Extension
- `SPEC/smcdeleg.adoc` — Smcdeleg and Ssccfg Counter Delegation Extensions
- `SPEC/machine.adoc` — Machine-Level ISA（含 PMP 章节与 pmp-vmem：PMP 与分页的交互）
- `SPEC/pmp.adoc` — Physical Memory Protection（machine.adoc 内章节）
- `DOCS/testplan/Smcsrind_test_plan.md` — Smcsrind Machine Mode 测试计划
- `DOCS/testplan/Smctr_test_plan.md` — Smctr Machine Mode 测试计划
- `DOCS/testplan/Smcntrpmf_test_plan.md` — Smcntrpmf 独立测试计划
- `DOCS/testplan/smstateen_test_plan.md` — Smstateen 独立测试计划
- `DOCS/testplan/Smcdeleg_test_plan.md` — Smcdeleg 扩展测试计划（Machine Mode）
- `DOCS/testplan/pmp_test_plan.md` — PMP 独立测试计划（非 Hypervisor 场景）
- `DOCS/testplan/Smepmp_test_plan.md` — Smepmp 独立测试计划（非 Hypervisor 场景）
- `DOCS/testplan/Hypervisor_CSR_test_plan.md` — Hypervisor CSR 子集测试计划
- `DOCS/testplan/Hypervisor_Interrupts_test_plan.md` — Hypervisor 中断子集测试计划
- `DOCS/testplan/Hypervisor_Exceptions_test_plan.md` — Hypervisor 异常与 trap 子集测试计划
- `DOCS/testplan/Hypervisor_2_stage_test_plan.md` — 两阶段翻译测试计划
- `DOCS/testplan/Hypervisor_gstage_test_plan.md` — G-stage 独立测试计划
- `ideas/hypervisor_gap.md` — Hypervisor 测试缺口分析

---

## 附录 A：规范点覆盖矩阵

下表标明"覆盖的规范点"章节中每条规范点被哪些测试用例覆盖。仅列入本方案各 Group 直接引用的规范点；主表所列部分规范点（如 `hstateen_sstateen_zero_initialization`、`hcounteren_vs_vu_control`）为多个 Group 共用的基础规范点。

| Norm ID | 覆盖的测试 ID |
|---------|---------------|
| `hstateen_sstateen_zero_initialization` | HCROSS-SMSTA-01 |
| `norm:mstateen_lower_priv_roz` | HCROSS-SMSTA-02 |
| `norm:mstateen_bit_63_op` | HCROSS-SMSTA-03 |
| `norm:mstateen_bit_63_roz` | HCROSS-SMSTA-04 |
| `norm:mstateen0_se0_op` | HCROSS-SMSTA-05、HCROSS-SMSTA-06 |
| `norm:mstateen0_envcfg_op` | HCROSS-SMSTA-07 |
| `norm:mstateen0_csrind_op` | HCROSS-SMSTA-08 |
| `norm:mstateen0_imsic_op` | HCROSS-SMSTA-09 |
| `norm:mstateen0_context_op` | HCROSS-SMSTA-10 |
| `norm:mstateen0_p1p13_op` | HCROSS-SMSTA-11、HCROSS-SMSTA-12 |
| `norm:stateen_illegal_state_access` | HCROSS-SMSTA-13、HCROSS-SMSTA-14 |
| `norm:sscsrind_csrs_access_control` | HCROSS-SMCSRIND-01~08（VS-mode 角度的同源验证见 `Hypervisor_Ss_test_plan.md` Sscsrind Group） |
| `norm:smcdeleg_mstateen0_bit60` | HCROSS-SMCSRIND-01~08（与 `norm:sscsrind_csrs_access_control` 同源；从 `Smcdeleg_test_plan.md` 迁入，本方案覆盖其 `vsiselect`/`vsireg*` 部分） |
| `norm:hypervisor_impl_csrs_access_control` | HCROSS-SMCSRIND-09~11（VS-mode 角度的同源验证见 `Hypervisor_Ss_test_plan.md` Sscsrind Group） |
| `norm:mstateen_ctr0_except1` | HCROSS-SMCTR-01 |
| `norm:hstateen_ctr` | HCROSS-SMCTR-02、HCROSS-SMCTR-03、HCROSS-SMCTR-04~09 |
| `norm:hstateen_vs` | HCROSS-SMCTR-04~08（VS-mode 角度的同源验证见 `Hypervisor_Ss_test_plan.md` Ssctr Group） |
| `norm:hstateen0_CTR0-V1_op` | HCROSS-SMCTR-09 |
| `norm:exttrap_vsm` | HCROSS-SMCTR-10 |
| `norm:exttrap_vum` | HCROSS-SMCTR-11、HCROSS-SMCTR-12 |
| `norm:exttrap_implreq` | HCROSS-SMCTR-10~12（vsctrctl.STE 实现前提） |
| `norm:unimplemented_mode_bits` | PMF-CSR-05 |
| `norm:counter_inhibited_behavior` | PMF-CYC-08、PMF-CYC-09、PMF-INS-06、PMF-INS-07、HCROSS-PMF-01 |
| `hcounteren_vs_vu_control` | PMF-CTR-04、HCROSS-PMF-01 |
| `norm:H_pmp` | HCROSS-PMP-01~06（基础场景另见 `Hypervisor_gstage_test_plan.md` GBARE-04、`Hypervisor_2_stage_test_plan.md` TS-PMP-01） |
| `norm:pmp_with_paging` | HCROSS-PMP-05、HCROSS-PMP-06 |
| `norm:pmp_sfence_required` | HCROSS-PMP-07 |
| `hyp_hfence_gvma_pmp_sync` | HCROSS-PMP-07 |
| `norm:hgatp_mode_bare` | —（基础场景由 `Hypervisor_gstage_test_plan.md` GBARE-04/05 覆盖，本方案不重复立项） |
| `hlvx_pmp_override` | HCROSS-PMP-08 |
| `norm:hlsv_priv` | HCROSS-PMP-08（VU 有效特权前提） |
| `norm:hstatus_spv_op` | Group 5.1/5.2 全部 PMP 违规类用例的 SPV/MPV 断言前提 |
