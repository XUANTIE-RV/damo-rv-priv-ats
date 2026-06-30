**中文 | [English](../testplan_en/Hypervisor_cross_test_plan_en.md)**

# Hypervisor 与其他扩展交叉测试计划

> 本文档描述 Hypervisor（H）扩展与其他 S-mode 扩展在交叉场景下的测试计划。这些测试场景原本在各扩展的独立测试计划中被标记为"由 Hypervisor 测试计划覆盖"或"因缺少 H 扩展而排除"，但经分析发现现有 Hypervisor 测试计划（`Hypervisor_test_plan.md`、`Hypervisor_2_stage_test_plan.md`、`Hypervisor_gstage_test_plan.md`）并未完全覆盖。
>
> 生成时间：2026-06-22

---

## 范围

### 覆盖的扩展交叉

- **Hypervisor × Svadu**：`henvcfg.ADUE` 可写性、HLV/HSV 与硬件 A/D 更新交互、`menvcfg.ADUE` 修改后的 HFENCE.GVMA 同步
- **Hypervisor × Sstvala**：Guest page-fault（cause 20/23）时 `stval` 精确写入行为
- **Hypervisor × Ssccptr**：两阶段页表遍历的 cacheability/coherence 验证
- **Hypervisor × Sscounterenw**：`hcounteren` 对 VS/VU-mode 计数器写入的控制
- **Hypervisor × Svinval**：HINVAL.VVMA/GVMA 指令功能、VMID 替代 ASID、VS/VU-mode virtual-instruction 触发
- **Hypervisor × Svnapot**：G-stage 翻译中的 NAPOT PTE 支持、保留编码 fault、两阶段同时使用 NAPOT
- **Hypervisor × Svpbmt**：两阶段地址翻译中 PBMT 属性的叠加覆盖行为、G-stage/VS-stage PBMT 覆盖规则
- **Hypervisor × Ssstateen**：`hstateen0-3` CSR 存在性与可访问性、`hstateen` bit 63 控制 VS-mode 对 `sstateen` 的访问、`hstateen` 只读零传播到 VS-mode、`hstateen0` 各功能位（SE0/ENVCFG/CSRIND/IMSIC/AIA/CONTEXT）对 VS-mode 状态的控制、`hstateen` 只读约束与编码一致性
- **Hypervisor × Sstc**：`henvcfg.STCE` 可写性与约束、VS-mode 对 `stimecmp`/`vstimecmp` 的访问控制、`vstimecmp` CSR 读写、VSTIP 合成逻辑、`henvcfg.STCE` 对 VS-mode 定时器的控制、VS-mode timer interrupt 捕获
- **Hypervisor × Smcsrind**：`mstateen0[60]` (CSRIND) 对 S-mode (HS-mode) 访问 `vsiselect`/`vsireg*` 的控制、M-mode 访问不受 state-enable 控制验证
- **Hypervisor × Sscsrind**：VS-level CSR（vsiselect/vsireg*）基本功能、Virtual-instruction 异常行为、`hstateen0[60]` 对 VS/VU-mode 访问的控制、VS-mode 通过 sireg* 透明访问 vsireg* 的重映射行为
- **Hypervisor × Ssdbltrp**：`henvcfg.DTE` 对 VS-mode 的使能/禁用控制、`vsstatus.SDT` 字段行为与 SDT/SIE 互斥、SRET 对 `vsstatus.SDT` 的清除、MRET/SRET/MNRET 在 Hypervisor 场景下对 SDT/vsstatus.SDT 的跨模式清除
- **Hypervisor × Smctr**：`hstateen0.CTR` 对 VS-mode CTR 状态访问的控制、`mstateen0.CTR=0` 对 `vsctrctl` 的阻止、MTE 外部陷阱在 VS/VU-mode 到 M-mode 的录制行为
- **Hypervisor × Ssctr**：`vsctrctl` CSR 基本功能与字段验证、VS/VU-mode 外部陷阱录制（STE/vsSTE）、虚拟化模式转换配置来源、VS-mode Freeze 行为（vsctrctl 控制）、VS-mode 对 sctrdepth/SCTRCLR 的访问限制、hstateen0.CTR 对 VS-mode CTR 访问的控制

### 不在本文档范围

- 已由 `Hypervisor_test_plan.md`、`Hypervisor_2_stage_test_plan.md`、`Hypervisor_gstage_test_plan.md` 覆盖的 Hypervisor 基础功能
- 已由 `Shcounterenw_test_plan.md` 覆盖的 Sha 子扩展（与 Sscounterenw 是不同的扩展体系）
- 各扩展在非 Hypervisor 场景下的行为（由各自独立测试计划覆盖）
- Ssdbltrp 非 Hypervisor 测试（`sstatus`.SDT 字段、S-mode double-trap、`menvcfg`.DTE 基础控制、`medeleg`[16]、`mtval2`） — 由 `Ssdbltrp_test_plan.md` 覆盖
- Smcsrind M-mode CSR（miselect/mireg\*）的基本功能和 WARL 行为 — 由 `Smcsrind_test_plan.md` 覆盖
- `mstateen0[60]` 对 S-mode 访问 siselect/sireg\*（非 H 扩展 CSR）的控制 — 由 `Smcsrind_test_plan.md` Group 4 覆盖

---

## 规范依据

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:henvcfg_adue_op` | `hypervisor.adoc` | If the Svadu extension is implemented, the ADUE bit controls whether hardware updating of PTE A/D bits is enabled for VS-stage address translation. When ADUE=1, hardware updating is enabled. When ADUE=0, the implementation behaves as though Svade were implemented for VS-stage address translation. If Svadu is not implemented, ADUE is read-only zero. | 若实现了 Svadu 扩展，ADUE 位控制 VS 阶段地址翻译的 PTE A/D 位硬件更新是否启用。ADUE=1 时启用，ADUE=0 时行为如同实现了 Svade。未实现 Svadu 时，ADUE 为只读零。 |
| `norm:svadu_henvcfg_adue_writable` | `svadu.adoc` | When Svadu is implemented, `henvcfg.ADUE` must be writable. | 实现 Svadu 时，`henvcfg.ADUE` 必须可写。 |
| `norm:svadu_hfence_gvma_sync` | `svadu.adoc` | After modifying `menvcfg.ADUE`, a `HFENCE.GVMA(x0,x0)` is required to synchronize the change across all VMIDs. | 修改 `menvcfg.ADUE` 后，需要执行 `HFENCE.GVMA(x0,x0)` 以在所有 VMID 间同步该变更。 |
| `norm:H_guest_page_fault` | `hypervisor.adoc` | On a guest-page fault, `mtval` or `stval` is written with the faulting guest virtual address. | 客户页错误时 `mtval`/`stval` 写入故障客户虚拟地址。 |
| `norm:sstvala_stval_pc` | `sstvala.adoc` | When a page-fault is triggered by an instruction fetch, `stval` is written with the faulting virtual address (PC). | 当取指触发 page-fault 时，`stval` 写入故障虚拟地址（PC）。 |
| `norm:Ssccptr_cacheable_coherent_supports_pt_read` | `ssccptr.adoc` | If the Ssccptr extension is implemented, then main memory regions with both the cacheability and coherence PMAs must support hardware page-table reads. | 如果实现了 Ssccptr 扩展，则同时具有可缓存性和一致性 PMA 的主存区域必须支持硬件页表读取。 |
| `norm:sscounterenw_hpmcounter_hcounteren` | `sscounterenw.adoc` | If the Sscounterenw extension is implemented, then for any `hpmcounter` that is not read-only zero, the corresponding bit in `scounteren` must be writable. | 若实现了 Sscounterenw 扩展，则对于任何非只读零的 `hpmcounter`，`scounteren` 中的对应位必须可写。 |
| `norm:hcounteren_vs_vu_control` | `hypervisor.adoc` | The `hcounteren` CSR controls availability of performance monitoring counters to VS-mode and VU-mode. | `hcounteren` CSR 控制 VS 和 VU 模式下性能监控计数器的可用性。 |
| `norm:Svinval_hinval_vvma_gvma` | `svinval.adoc` | HINVAL.VVMA and HINVAL.GVMA have the same semantics as SINVAL.VMA, except that they combine with SFENCE.W.INVAL and SFENCE.INVAL.IR to replace HFENCE.VVMA and HFENCE.GVMA, respectively. | HINVAL.VVMA 和 HINVAL.GVMA 与 SINVAL.VMA 具有相同的语义，只是它们与 SFENCE.W.INVAL 和 SFENCE.INVAL.IR 结合分别替换 HFENCE.VVMA 和 HFENCE.GVMA。 |
| `norm:Svinval_hinval_gvma_uses_vmid` | `svinval.adoc` | HINVAL.GVMA uses VMIDs instead of ASIDs. | HINVAL.GVMA 使用 VMID 而不是 ASID。 |
| `norm:Svinval_virtual_instruction_vu_vs` | `svinval.adoc` | An attempt to execute HINVAL.VVMA or HINVAL.GVMA in VS-mode or VU-mode, or to execute SINVAL.VMA in VU-mode, raises a virtual-instruction exception. | 在 VS 模式或 VU 模式下尝试执行 HINVAL.VVMA 或 HINVAL.GVMA，或在 VU 模式下执行 SINVAL.VMA，会触发虚拟指令异常。 |
| `norm:Svinval_sfence_w_inval_inval_vu_mode` | `svinval.adoc` | An attempt to execute SFENCE.W.INVAL or SFENCE.INVAL.IR in VU-mode raises a virtual-instruction exception. | 在 VU 模式下尝试执行 SFENCE.W.INVAL 或 SFENCE.INVAL.IR 会触发虚拟指令异常。 |
| `norm:Svnapot_hyp_gstage` | `svnapot.adoc` | If the Hypervisor extension is also implemented, Svnapot is supported in G-stage translation. | 如果同时实现了 Hypervisor 扩展，Svnapot 在 G-stage 翻译中也受支持。 |
| `norm:Svpbmt_hgatp_stage_override_rule` | `svpbmt.adoc` | When `hgatp.MODE` is not Bare, a nonzero PBMT field in a G-stage leaf PTE overrides the PMA to produce intermediate memory attributes. | 当 `hgatp.MODE` 非零时，G-stage 叶 PTE 的非零 PBMT 位覆盖 PMA 产生中间属性。 |
| `norm:Svpbmt_vsatp_stage_override_rule` | `svpbmt.adoc` | When `vsatp.MODE` is not Bare, a nonzero PBMT field in a VS-stage leaf PTE overrides the intermediate memory attributes to produce the final memory attributes. | 当 `vsatp.MODE` 非零时，VS-stage 叶 PTE 的非零 PBMT 位覆盖中间属性产生最终属性。 |
| `norm:hstateen_rv64_csrs` | `smstateen.adoc` | When H extension is implemented, hstateen0-3 CSRs are added. | 实现 H 扩展时添加 hstateen0-3 CSR。 |
| `norm:stateen_rv32_upper_bits_csrs` | `smstateen.adoc` | RV32 provides additional hstateen0h-3h CSRs for upper 32 bits. | RV32 额外提供 hstateen0h-3h CSR 用于高 32 位。 |
| `norm:hstateen_encoding` | `smstateen.adoc` | hstateen CSRs have the same encoding as mstateen CSRs. | hstateen CSR 的编码与 mstateen CSR 一致。 |
| `norm:hstateen_bit_63_op` | `smstateen.adoc` | Bit 63 of each hstateen CSR controls whether VS-mode may access the corresponding sstateen CSR. | hstateen 的 bit 63 控制 VS-mode 对对应 sstateen CSR 的访问。 |
| `norm:hstateen_bit_63_writable` | `smstateen.adoc` | Bit 63 of each hstateen CSR is always writable (not read-only). | hstateen 的 bit 63 始终可写（不是只读）。 |
| `norm:sstateen_vsmode_access_roz` | `smstateen.adoc` | For any bit that is zero in hstateen (whether read-only zero or written to zero), the corresponding bit in sstateen appears as read-only zero when accessed from VS-mode. | hstateen 中为零的位（无论是只读零还是写为零），在 VS-mode 访问 sstateen 时表现为只读零。 |
| `norm:sstateen_ro1_bits` | `smstateen.adoc` | A bit in sstateen cannot be read-only one unless the same bit in both mstateen and hstateen (when H is implemented) is also read-only one. | sstateen 位不能为 RO1 除非 mstateen 和 hstateen（实现 H 扩展时）同位也为 RO1。 |
| `norm:hstateen_ro1_bits` | `smstateen.adoc` | A bit in hstateen cannot be read-only one unless the same bit in mstateen is also read-only one. | hstateen 位不能为 RO1 除非 mstateen 同位也为 RO1。 |
| `norm:hstateen_sstateen_zero_initialization` | `smstateen.adoc` | After M-mode software modifies any mstateen CSR, it is responsible for initializing the corresponding hstateen and sstateen CSRs to zero. | M-mode 软件修改任何 mstateen CSR 后，负责将对应的 hstateen 和 sstateen CSR 初始化为零。 |
| `norm:stateen_warl_access` | `smstateen.adoc` | Each standard-defined bit in stateen CSRs is WARL (Write Any Values, Reads Legal Values). | stateen CSR 中每个标准定义位都是 WARL（可写任意值，读回合法值）。 |
| `norm:stateen_unimplemented_state_roz` | `smstateen.adoc` | Bits that control state for unimplemented extensions are read-only zero. | 控制未实现扩展状态的位为只读零。 |
| `norm:stateen_reserved_roz` | `smstateen.adoc` | Reserved bits in stateen CSRs are read-only zero. | stateen CSR 中的保留位为只读零。 |
| `norm:hstateen0_SE0_op` | `smstateen.adoc` | hstateen0.SE0 (bit 63) controls whether VS-mode may access sstateen0. | hstateen0.SE0（bit 63）控制 VS-mode 对 sstateen0 的访问。 |
| `norm:hstateen0_envcfg_op` | `smstateen.adoc` | hstateen0.ENVCFG (bit 62) controls whether VS-mode may access senvcfg. | hstateen0.ENVCFG（bit 62）控制 VS-mode 对 senvcfg 的访问。 |
| `norm:hstateen0_csrind_op` | `smstateen.adoc` | hstateen0.CSRIND (bit 60) controls whether VS-mode may access siselect and sireg* (which are actually vsiselect and vsireg*). | hstateen0.CSRIND（bit 60）控制 VS-mode 对 siselect/sireg*（实为 vsiselect/vsireg*）的访问。 |
| `norm:hstateen0_imsic_op` | `smstateen.adoc` | hstateen0.IMSIC (bit 58) controls access to guest IMSIC state and vstopei in VS-mode. | hstateen0.IMSIC（bit 58）控制 VS-mode 对 guest IMSIC 状态及 vstopei 的访问。 |
| `norm:hstateen0_aia_op` | `smstateen.adoc` | hstateen0.AIA (bit 59) controls VS-mode access to Ssaia state not covered by CSRIND or IMSIC bits. | hstateen0.AIA（bit 59）控制 VS-mode 对 Ssaia 非 CSRIND/IMSIC 的剩余状态的访问。 |
| `norm:hstateen0_context_op` | `smstateen.adoc` | hstateen0.CONTEXT (bit 57) controls whether VS-mode may access scontext. | hstateen0.CONTEXT（bit 57）控制 VS-mode 对 scontext 的访问。 |
| `norm:Sstvala_virtual_inst_tval_inst` | `sstvala.adoc` | When a virtual-instruction exception is raised, `stval` must be written with the faulting instruction encoding. | virtual-instruction 异常时，`stval` 必须写入故障指令编码。 |
| `norm:henvcfg_stce` | `hypervisor.adoc` | henvcfg.STCE=1 enables vstimecmp; when STCE=0, VS-mode (V=1) access to stimecmp raises virtual-instruction exception. | henvcfg.STCE=1 使能 vstimecmp；STCE=0 时 V=1 下访问 stimecmp 产生 virtual-instruction 异常。 |
| `norm:vstimecmp_exist` | `sstc.adoc` | Sstc adds a new VS-level vstimecmp CSR. | Sstc 新增 VS-level vstimecmp CSR。 |
| `norm:sstc_vs_facility` | `sstc.adoc` | Sstc provides a similar timer mechanism for VS-mode via the Hypervisor extension. | Sstc 为 Hypervisor 扩展的 VS-mode 提供类似的定时器机制。 |
| `norm:hip_vstip_vstie_acc_op` | `hypervisor.adoc` | VSTIP = hvip.VSTIP OR vstimecmp timer signal. | VSTIP = hvip.VSTIP OR vstimecmp 定时器信号。 |

---

## Group 1. Hypervisor × Svadu 交叉测试

**规范依据**：
- `norm:henvcfg_adue_op`：ADUE 位控制 VS-stage A/D 位硬件更新
- `norm:svadu_henvcfg_adue_writable`：`henvcfg.ADUE` 必须可写
- `norm:svadu_hfence_gvma_sync`：修改 `menvcfg.ADUE` 后需要 HFENCE.GVMA 同步

**测试职责**：验证 Svadu 扩展在 Hypervisor 两级翻译场景下的 CSR 可写性、HLV/HSV 指令交互、以及跨 VMID 同步行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SVADU-01 | henvcfg.ADUE 可写性验证 | HS-mode 写 henvcfg.ADUE=1，读回验证；再写 ADUE=0，读回验证 | 若实现 Svadu，ADUE 可写且读回一致；若未实现 Svadu，ADUE 只读零 |
| HCROSS-SVADU-02 | HLV 指令与 Svadu 交互（ADUE=1） | henvcfg.ADUE=1，VS-stage PTE A=0，HS-mode 执行 HLV.D 读取该 GPA | 访问成功，VS-stage PTE A 位被硬件自动设为 1（HLV 触发的隐式访问也遵循 ADUE 控制） |
| HCROSS-SVADU-03 | HSV 指令与 Svadu 交互（ADUE=1） | henvcfg.ADUE=1，VS-stage PTE A=1,D=0，HS-mode 执行 HSV.D 写入该 GPA | 访问成功，VS-stage PTE D 位被硬件自动设为 1 |
| HCROSS-SVADU-04 | HLV 指令与 Svade 交互（ADUE=0） | henvcfg.ADUE=0，VS-stage PTE A=0，HS-mode 执行 HLV.D 读取该 GPA | guest-page-fault (cause=21)，硬件不自动更新 A 位（行为如同 Svade） |
| HCROSS-SVADU-05 | menvcfg.ADUE 修改后 HFENCE.GVMA 同步 | menvcfg.ADUE 从 0 改为 1，不执行 HFENCE.GVMA，VS-mode 访问 A=0 的页；再执行 HFENCE.GVMA(x0,x0) 后重复访问 | 第一次访问可能仍按旧行为（实现相关）；HFENCE.GVMA 后行为必须按新 ADUE 值（A 位被硬件更新） |
| HCROSS-SVADU-06 | menvcfg.ADUE 修改后特定 VMID 同步 | menvcfg.ADUE 从 1 改为 0，仅对特定 VMID 执行 HFENCE.GVMA(vmid, x0)，验证该 VMID 和其他 VMID 的行为 | 指定 VMID 的行为必须按新 ADUE 值；其他 VMID 行为实现相关（可能仍按旧值） |

> [!NOTE]
> - HCROSS-SVADU-02~04 需要在 HS-mode 执行 HLV/HSV 指令，验证隐式访问（页表遍历）的 A/D 更新行为也受 `henvcfg.ADUE` 控制。
> - HCROSS-SVADU-05~06 验证 `menvcfg.ADUE` 变更后的同步语义。`HFENCE.GVMA(x0,x0)` 刷新所有 VMID，`HFENCE.GVMA(vmid,x0)` 仅刷新特定 VMID。
> - 若平台未实现 Svadu 扩展，HCROSS-SVADU-01 应验证 ADUE 只读零，HCROSS-SVADU-02~06 应 TEST_SKIP。

---

## Group 2. Hypervisor × Sstvala 交叉测试

**规范依据**：
- `norm:H_guest_page_fault`：guest-page-fault 时 `stval` 写入 faulting GVA
- `norm:sstvala_stval_pc`：取指 page-fault 时 `stval` 写入 faulting PC
- `norm:Sstvala_virtual_inst_tval_inst`：virtual-instruction 异常时，`stval` 必须写入故障指令编码

**测试职责**：验证 Sstvala 扩展在 Hypervisor 场景下，`stval`/`vstval` 对各类异常的精确写入行为。地址类异常（guest-page-fault cause 20/21/23）时 `stval` 必须等于故障 GVA；指令类异常（virtual-instruction cause=22）时 `stval` 必须等于故障指令编码。现有 Hypervisor 测试计划已覆盖 cause=21（load guest-page-fault）的 stval 验证，本组补充 cause=20（instruction）、cause=23（store/AMO）的精确验证，以及 virtual-instruction 的指令编码精确验证。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSTVALA-01 | Instruction guest-page-fault (cause=20) 时 stval 精确值 | VS-mode 跳转执行一个 G-stage 未映射的 GPA 地址，触发 inst guest-page-fault trap 到 HS-mode | scause=20，stval == faulting GVA（即跳转目标地址），而非 0 或其他值 |
| HCROSS-SSTVALA-02 | Store guest-page-fault (cause=23) 时 stval 精确值 | VS-mode 对一个 G-stage 未映射的 GPA 地址执行 store，触发 store guest-page-fault trap 到 HS-mode | scause=23，stval == faulting GVA（即 store 目标地址），而非 0 |
| HCROSS-SSTVALA-03 | AMO guest-page-fault (cause=23) 时 stval 精确值 | VS-mode 对一个 G-stage 未映射的 GPA 地址执行 AMO（如 AMOADD.W），触发 guest-page-fault trap 到 HS-mode | scause=23，stval == faulting GVA（即 AMO 目标地址） |
| HCROSS-SSTVALA-04 | Instruction guest-page-fault 时 vstval 精确值（委托到 VS-mode） | 配置 hedeleg 将 inst guest-page-fault 委托到 VS-mode，VS-mode 跳转执行未映射地址 | vscause=20，vstval == faulting GVA |
| HCROSS-SSTVALA-05 | Store guest-page-fault 时 vstval 精确值（委托到 VS-mode） | 配置 hedeleg 将 store guest-page-fault 委托到 VS-mode，VS-mode store 到未映射地址 | vscause=23，vstval == faulting GVA |
| HCROSS-SSTVALA-06 | Virtual-instruction 异常时 stval 精确值：VS-mode 读 hstatus | VS-mode 执行 `csrrs x5, hstatus, x0`（编码 0x600022F3），触发 virtual-instruction（cause=22） | scause=22，stval == 0x600022F3（故障指令编码） |
| HCROSS-SSTVALA-07 | Virtual-instruction 异常时 stval 精确值：VS-mode 写 hgatp | VS-mode 执行 `csrrw x0, hgatp, x0`（编码 0x68001073），触发 virtual-instruction（cause=22） | scause=22，stval == 0x68001073（故障指令编码） |
| HCROSS-SSTVALA-08 | Virtual-instruction 异常时 stval 精确值：VS-mode 读 hideleg | VS-mode 执行 `csrrs x5, hideleg, x0`（编码 0x603022F3），触发 virtual-instruction（cause=22） | scause=22，stval == 0x603022F3（故障指令编码） |

> [!NOTE]
> - Sstvala 的核心语义是保证 `stval` 写入 faulting 地址（而非 0）。基础 H 扩展规范允许 `stval` 在某些场景下为 0，而 Sstvala 扩展强制要求写入精确地址。
> - HCROSS-SSTVALA-01~03 验证 trap 到 HS-mode 时 `stval` 的精确性；HCROSS-SSTVALA-04~05 验证委托到 VS-mode 时 `vstval` 的精确性。
> - 与 `Hypervisor_gstage_test_plan.md` 的 GFAULT 系列用例的区别：GFAULT 系列主要验证 fault 触发和 htval 编码，本组专注于 stval/vstval 的精确值断言。
> - HCROSS-SSTVALA-06~08 验证 Sstvala 对 **指令类异常** 的精确性要求：virtual-instruction 异常（cause=22）时，`stval` 必须包含触发异常的指令编码（零扩展到 XLEN）。这与 guest-page-fault（地址类异常）的 stval 语义不同——后者要求写入故障虚拟地址。指令编码推导：`csrrs x5, 0x600, x0` = `[31:20]=0x600 [19:15]=00000 [14:12]=010 [11:7]=00101 [6:0]=1110011` = `0x600022F3`。
> - HCROSS-SSTVALA-06~08 从 `sstvala_test_plan.md` Group 6（TVAL-VI-01~03）迁移而来。需要 H 扩展（`ENABLE_HYP=1`）。

---

## Group 3. Hypervisor × Ssccptr 交叉测试

**规范依据**：
- `norm:Ssccptr_cacheable_coherent_supports_pt_read`：具有 cacheability 和 coherence PMA 的主存区域必须支持硬件页表读取

**测试职责**：验证在 Hypervisor 两级翻译场景下，VS-stage 和 G-stage 页表遍历在满足 PMA 条件的主存区域中能够正确完成。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSCCPTR-01 | VS-stage 页表在 cacheable+coherent 主存中的遍历 | 在默认主存（满足 cacheability+coherence PMA）中建立 VS-stage 页表，启用两阶段翻译，VS-mode 执行 load | 页表遍历成功，load 返回正确值（硬件能够正确读取 VS-stage 页表） |
| HCROSS-SSCCPTR-02 | G-stage 页表在 cacheable+coherent 主存中的遍历 | 在默认主存中建立 G-stage 页表，启用两阶段翻译，VS-mode 执行 load | 页表遍历成功，load 返回正确值（硬件能够正确读取 G-stage 页表） |
| HCROSS-SSCCPTR-03 | 两阶段页表均在 cacheable+coherent 主存中的遍历 | VS-stage 和 G-stage 页表均分配在默认主存中，VS-mode 执行 load/store | 两阶段页表遍历均成功，R/W 返回正确值 |
| HCROSS-SSCCPTR-04 | G-stage 页表所在物理页的 PMA 属性验证 | 尝试将 G-stage 页表分配到非 cacheable 或非 coherent 区域（如果平台支持），验证行为 | 若 PMA 不满足 Ssccptr 要求，行为为实现相关（可能 page walk 失败）；若平台不支持配置 PMA，TEST_SKIP |

> [!NOTE]
> - Ssccptr 是 PMA 层面的约束扩展，不引入新指令或 CSR。在 QEMU/Spike 仿真器上，主存区域默认满足 cacheability+coherence PMA 条件，因此 HCROSS-SSCCPTR-01~03 在仿真器上预期 PASS。
> - HCROSS-SSCCPTR-04 需要平台支持动态配置 PMA 属性，大多数仿真器和通用硬件不支持，预期 TEST_SKIP。
> - 本组测试的核心价值在于：确保 Hypervisor 两级翻译的页表遍历不会因 PMA 约束而失败，这是虚拟化场景下的关键正确性保证。

---

## Group 4. Hypervisor × Sscounterenw 交叉测试

**规范依据**：
- `norm:hcounteren_vs_vu_control`：`hcounteren` 控制 VS/VU-mode 计数器可用性
- `norm:sscounterenw_hpmcounter_hcounteren`：非只读零的 hpmcounter 对应位必须可写

**测试职责**：验证 `hcounteren` 寄存器对 VS/VU-mode 性能监控计数器（hpmcounter）访问的控制行为，以及 `hcounteren` 对应位的可写性。

> [!NOTE]
> - Sscounterenw 扩展要求：对于任何非只读零的 `hpmcounter`，`scounteren`（以及 `hcounteren`）的对应位必须可写。本组测试验证 `hcounteren` 的可写性和控制行为。
> - 请参考 `Shcounterenw_test_plan.md` 扩展测试

---

## Group 5. Hypervisor × Svinval 交叉测试

**规范依据**：
- `norm:Svinval_hinval_vvma_gvma`：HINVAL.VVMA/GVMA 指令功能
- `norm:Svinval_hinval_gvma_uses_vmid`：HINVAL.GVMA 使用 VMID 替代 ASID
- `norm:Svinval_virtual_instruction_vu_vs`：VS/VU-mode 执行 HINVAL 触发 virtual-instruction
- `norm:Svinval_sfence_w_inval_inval_vu_mode`：VU-mode 执行 SFENCE.W.INVAL/SFENCE.INVAL.IR 触发 virtual-instruction

**测试职责**：验证 Svinval 扩展的 HINVAL.VVMA/GVMA 指令在 Hypervisor 场景下的功能、VMID 语义、以及 VS/VU-mode 的异常触发行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SINVAL-01 | HINVAL.VVMA 基本功能 | HS-mode 修改 VS-stage PTE，执行 HINVAL.VVMA(va, asid)，VS-mode 验证新 PTE 生效 | VS-mode 访问 va 时使用新 PTE（HINVAL.VVMA 正确刷新 VS-stage TLB） |
| HCROSS-SINVAL-02 | HINVAL.GVMA 基本功能 | HS-mode 修改 G-stage PTE，执行 HINVAL.GVMA(gpa, vmid)，VS-mode 验证新 PTE 生效 | VS-mode 访问对应 GPA 时使用新 PTE（HINVAL.GVMA 正确刷新 G-stage TLB） |
| HCROSS-SINVAL-03 | HINVAL.VVMA 与 SFENCE.W.INVAL/SFENCE.INVAL.IR 组合 | HS-mode 修改多个 VS-stage PTE，执行多个 HINVAL.VVMA，然后执行 SFENCE.W.INVAL + SFENCE.INVAL.IR，VS-mode 验证所有新 PTE 生效 | 所有修改的 PTE 均生效（组合语义正确） |
| HCROSS-SINVAL-04 | HINVAL.GVMA 与 SFENCE.W.INVAL/SFENCE.INVAL.IR 组合 | HS-mode 修改多个 G-stage PTE，执行多个 HINVAL.GVMA，然后执行 SFENCE.W.INVAL + SFENCE.INVAL.IR，VS-mode 验证所有新 PTE 生效 | 所有修改的 PTE 均生效（组合语义正确） |
| HCROSS-SINVAL-05 | HINVAL.GVMA 使用 VMID（特定 VMID 刷新） | HS-mode 修改 G-stage PTE，执行 HINVAL.GVMA(gpa, vmid=5)，验证 VMID=5 的 TLB 被刷新，VMID=6 的 TLB 未被刷新 | VMID=5 的访问使用新 PTE；VMID=6 的访问可能仍使用旧 PTE（实现相关） |
| HCROSS-SINVAL-06 | HINVAL.GVMA 使用 VMID=0（所有 VMID 刷新） | HS-mode 修改 G-stage PTE，执行 HINVAL.GVMA(gpa, vmid=0)，验证所有 VMID 的 TLB 被刷新 | 所有 VMID 的访问均使用新 PTE |
| HCROSS-SINVAL-07 | VS-mode 执行 HINVAL.VVMA 触发 virtual-instruction | VS-mode 执行 HINVAL.VVMA | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-08 | VS-mode 执行 HINVAL.GVMA 触发 virtual-instruction | VS-mode 执行 HINVAL.GVMA | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-09 | VU-mode 执行 HINVAL.VVMA 触发 virtual-instruction | VU-mode 执行 HINVAL.VVMA | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-10 | VU-mode 执行 HINVAL.GVMA 触发 virtual-instruction | VU-mode 执行 HINVAL.GVMA | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-11 | VU-mode 执行 SFENCE.W.INVAL 触发 virtual-instruction | VU-mode 执行 SFENCE.W.INVAL | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-12 | VU-mode 执行 SFENCE.INVAL.IR 触发 virtual-instruction | VU-mode 执行 SFENCE.INVAL.IR | virtual-instruction exception (cause=22) |
| HCROSS-SINVAL-13 | VS-mode 执行 SFENCE.W.INVAL 正常（VTVM=0） | hstatus.VTVM=0，VS-mode 执行 SFENCE.W.INVAL | 正常执行，无异常（SFENCE.W.INVAL 不受 VTVM 控制） |
| HCROSS-SINVAL-14 | VS-mode 执行 SFENCE.INVAL.IR 正常（VTVM=0） | hstatus.VTVM=0，VS-mode 执行 SFENCE.INVAL.IR | 正常执行，无异常 |

> [!NOTE]
> - HINVAL.VVMA/GVMA 是 Svinval 扩展为 Hypervisor 场景提供的细粒度 TLB 刷新指令，与 HFENCE.VVMA/GVMA 功能等价但支持批量刷新优化。
> - HCROSS-SINVAL-05~06 验证 HINVAL.GVMA 的 VMID 语义：VMID 非零时仅刷新特定 VMID 的 TLB，VMID=0 时刷新所有 VMID。这与 HFENCE.GVMA 的语义一致。
> - HCROSS-SINVAL-07~12 验证 VS/VU-mode 执行 HINVAL 和 SFENCE.W.INVAL/SFENCE.INVAL.IR 时的 virtual-instruction 异常触发，这是 Hypervisor 安全隔离的关键保证。
> - 与 `Hypervisor_2_stage_test_plan.md` Group 22 的区别：Group 22 仅覆盖了 VTVM=1 时 SINVAL.VMA 和 TVM=1 时 HINVAL.GVMA 的异常触发（2 个用例），本组补充了 HINVAL 指令功能、VMID 语义、以及 VS/VU-mode 的完整 virtual-instruction 覆盖（14 个用例）。

---

## Group 6. Hypervisor × Svnapot 交叉测试

**规范依据**：
- `norm:Svnapot_hyp_gstage`：如果同时实现了 Hypervisor 扩展，Svnapot 在 G-stage 翻译中也受支持

**测试职责**：验证 Svnapot 在 G-stage 翻译中的行为，包括 G-stage NAPOT PTE 的基本翻译、保留编码异常、以及 VS-stage 和 G-stage 同时使用 NAPOT 的两阶段翻译正确性。

> **注意**：本组测试从 `svnapot_test_plan.md` Group 10 迁移而来。需要 H 扩展和 Svnapot 扩展同时可用。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SVNAPOT-01 | G-stage 64 KiB NAPOT 基本翻译 | 在 G-stage 页表中配置 64 KiB NAPOT PTE | GPA→SPA 翻译正确 |
| HCROSS-SVNAPOT-02 | G-stage NAPOT 保留编码 fault | G-stage NAPOT PTE 使用保留编码 | guest page-fault |
| HCROSS-SVNAPOT-03 | G-stage 与 VS-stage 同时使用 NAPOT | VS-stage 和 G-stage 均使用 NAPOT PTE | 两阶段翻译正确 |

> [!NOTE]
> - 本组测试验证 Svnapot 扩展在 Hypervisor G-stage 翻译中的行为。Svnapot 规范明确指出，如果同时实现了 Hypervisor 扩展，NAPOT 翻译在 G-stage 页表中同样受支持。
> - HCROSS-SVNAPOT-01 验证 G-stage 中 64 KiB NAPOT PTE 的基本 GPA→SPA 翻译功能，与 `Hypervisor_gstage_test_plan.md` 中的普通 PTE 测试互补。
> - HCROSS-SVNAPOT-02 验证 G-stage NAPOT PTE 使用保留编码（ppn[0] 低 4 位非 `1000` 且 N=1）时，硬件应触发 guest-page-fault。这与 VS-stage 中的保留编码行为一致（参见 `svnapot_test_plan.md` Group 3）。
> - HCROSS-SVNAPOT-03 验证两阶段翻译中 VS-stage 和 G-stage 同时使用 NAPOT PTE 的场景：VS-stage 将 GVA→GPA 使用 NAPOT 映射，G-stage 将 GPA→SPA 也使用 NAPOT 映射，最终 GVA→SPA 翻译应正确。

```c
/* HCROSS-SVNAPOT-01 示例：G-stage 64 KiB NAPOT 基本翻译 */
TEST_REGISTER(test_hcross_svnapot_01);
bool test_hcross_svnapot_01(void) {
    TEST_BEGIN("HCROSS-SVNAPOT-01: G-stage 64 KiB NAPOT basic translation");

    if (!check_h_extension()) TEST_SKIP("H extension not available");
    if (!check_svnapot_extension()) TEST_SKIP("Svnapot not available");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();

    /* VS-stage: Bare mode (identity GVA→GPA) */
    two_stage_init(&ctx, /*vs=Bare*/0, HGATP_MODE_SV39X4);

    /* G-stage: 64 KiB NAPOT mapping for GPA→SPA */
    uintptr_t test_gpa = TEST_REGION_BASE;  /* 64 KiB aligned */
    uintptr_t test_spa = test_gpa;
    uintptr_t napot_gpte = napot_make_pte(test_gpa, test_spa,
                                           PTE_V | PTE_R | PTE_W | PTE_U |
                                           PTE_A | PTE_D,
                                           NAPOT_64K);
    gstage_napot_install_pte(&ctx.g_ctx, test_gpa, napot_gpte);

    /* VS-mode 写入并读回验证 */
    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_read_write,
                                           test_gpa);
    TEST_ASSERT("G-stage NAPOT GPA→SPA translation correct", result == 0);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}

/* HCROSS-SVNAPOT-02 示例：G-stage NAPOT 保留编码 fault */
TEST_REGISTER(test_hcross_svnapot_02);
bool test_hcross_svnapot_02(void) {
    TEST_BEGIN("HCROSS-SVNAPOT-02: G-stage NAPOT reserved encoding fault");

    if (!check_h_extension()) TEST_SKIP("H extension not available");
    if (!check_svnapot_extension()) TEST_SKIP("Svnapot not available");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    two_stage_init(&ctx, /*vs=Bare*/0, HGATP_MODE_SV39X4);

    /* G-stage: NAPOT PTE with reserved encoding (ppn[0] low 4 bits = 0001) */
    uintptr_t test_gpa = TEST_REGION_BASE;
    uintptr_t test_spa = test_gpa;
    uintptr_t reserved_gpte = napot_make_reserved_pte(test_gpa, test_spa,
                                                       PTE_V | PTE_R | PTE_W |
                                                       PTE_U | PTE_A | PTE_D,
                                                       0x1);
    gstage_napot_install_pte(&ctx.g_ctx, test_gpa, reserved_gpte);

    /* VS-mode 访问应触发 guest-page-fault */
    trap_expect_begin();
    two_stage_run_in_vs(&ctx, test_vs_load, test_gpa);
    TEST_ASSERT("guest-page-fault triggered", trap_was_triggered());
    TEST_ASSERT_EQ("cause is load guest-page-fault",
                   trap_get_cause(), CAUSE_LOAD_GUEST_PAGE_FAULT);
    trap_expect_end();

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}

/* HCROSS-SVNAPOT-03 示例：G-stage 与 VS-stage 同时使用 NAPOT */
TEST_REGISTER(test_hcross_svnapot_03);
bool test_hcross_svnapot_03(void) {
    TEST_BEGIN("HCROSS-SVNAPOT-03: VS-stage and G-stage both use NAPOT");

    if (!check_h_extension()) TEST_SKIP("H extension not available");
    if (!check_svnapot_extension()) TEST_SKIP("Svnapot not available");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t test_gva = TEST_REGION_BASE;   /* 64 KiB aligned */
    uintptr_t test_gpa = test_gva;
    uintptr_t test_spa = test_gpa;

    /* VS-stage: 64 KiB NAPOT mapping GVA→GPA */
    uintptr_t vs_napot_pte = napot_make_pte(test_gva, test_gpa,
                                             PTE_V | PTE_R | PTE_W |
                                             PTE_A | PTE_D,
                                             NAPOT_64K);
    vstage_napot_install_pte(&ctx, test_gva, vs_napot_pte);

    /* G-stage: 64 KiB NAPOT mapping GPA→SPA */
    uintptr_t gs_napot_gpte = napot_make_pte(test_gpa, test_spa,
                                              PTE_V | PTE_R | PTE_W | PTE_U |
                                              PTE_A | PTE_D,
                                              NAPOT_64K);
    gstage_napot_install_pte(&ctx.g_ctx, test_gpa, gs_napot_gpte);

    /* VS-mode 写入并读回验证两阶段 NAPOT 翻译 */
    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_read_write,
                                           test_gva);
    TEST_ASSERT("Two-stage NAPOT translation correct", result == 0);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

---

## Group 7. Hypervisor × Svpbmt 交叉测试

**规范依据**：
- `norm:Svpbmt_hgatp_stage_override_rule`：当 `hgatp.MODE` 非零时，G-stage PTE 的非零 PBMT 位覆盖 PMA 产生中间属性
- `norm:Svpbmt_vsatp_stage_override_rule`：当 `vsatp.MODE` 非零时，VS-stage PTE 的非零 PBMT 位覆盖中间属性产生最终属性

**测试职责**：验证两阶段地址翻译中 PBMT 属性的叠加覆盖行为，包括 G-stage PBMT 覆盖 PMA、VS-stage PBMT 覆盖中间属性、两阶段叠加、以及 `hgatp.MODE=0` 时跳过 G-stage 覆盖的场景。

> **注意**：本组测试从 `svpbmt_test_plan.md` Group 10 迁移而来。需要 H 扩展和 Svpbmt 扩展同时可用。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SVPBMT-01 | G-stage PBMT=NC 覆盖 PMA | hgatp.MODE 非零，G-stage PTE 设置 PBMT=NC，VS-stage PTE PBMT=0（不覆盖）。逻辑链：G-stage PBMT=NC 覆盖 PMA → intermediate=NC；VS-stage PBMT=0 不触发覆盖 → final=NC | 最终属性为 NC |
| HCROSS-SVPBMT-02 | VS-stage PBMT=IO 覆盖中间属性 | hgatp.MODE 非零，G-stage PTE PBMT=0（不覆盖）→ intermediate=PMA；VS-stage PTE PBMT=IO 覆盖 intermediate → final=IO | 最终属性为 IO |
| HCROSS-SVPBMT-03 | 两阶段均非零叠加 | G-stage PTE PBMT=NC → intermediate=NC；VS-stage PTE PBMT=IO 覆盖 intermediate → final=IO | 最终属性为 IO |
| HCROSS-SVPBMT-04 | hgatp.MODE=0 跳过 G-stage | hgatp.MODE=0 → G-stage 不生效，intermediate=PMA；VS-stage PTE PBMT=NC 覆盖 intermediate → final=NC | 最终属性为 NC |

> [!NOTE]
> - 本组测试验证 Svpbmt 扩展在 Hypervisor 两阶段翻译中的 PBMT 属性叠加覆盖规则。Svpbmt 规范定义了两阶段的覆盖链：PMA → G-stage PBMT 覆盖 → intermediate → VS-stage PBMT 覆盖 → final。
> - HCROSS-SVPBMT-01 验证 G-stage PBMT 覆盖 PMA 产生中间属性，而 VS-stage PBMT=0 不触发二次覆盖，最终属性保持为 NC。
> - HCROSS-SVPBMT-02 验证 VS-stage PBMT 覆盖中间属性：G-stage PBMT=0 不覆盖 PMA（intermediate=PMA），VS-stage PBMT=IO 覆盖 intermediate 产生 final=IO。
> - HCROSS-SVPBMT-03 验证两阶段 PBMT 均非零时的叠加行为：G-stage PBMT=NC 产生 intermediate=NC，VS-stage PBMT=IO 覆盖 intermediate 产生 final=IO。VS-stage 的覆盖优先级高于 G-stage。
> - HCROSS-SVPBMT-04 验证 `hgatp.MODE=0`（Bare）时 G-stage 不生效，PBMT 覆盖链从 VS-stage 开始：intermediate=PMA，VS-stage PBMT=NC 覆盖产生 final=NC。

```c
/* HCROSS-SVPBMT-01 示例：G-stage PBMT=NC 覆盖 PMA */
TEST_REGISTER(test_hcross_svpbmt_01);
bool test_hcross_svpbmt_01(void) {
    TEST_BEGIN("HCROSS-SVPBMT-01: G-stage PBMT=NC overrides PMA");

    if (!check_h_extension()) TEST_SKIP("H extension not available");
    if (!check_svpbmt_extension()) TEST_SKIP("Svpbmt not available");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();

    /* VS-stage: Bare mode (no VS-stage translation) */
    two_stage_init(&ctx, /*vs=Bare*/0, HGATP_MODE_SV39X4);

    /* G-stage: map with PBMT=NC */
    uintptr_t test_gpa = TEST_REGION_BASE;
    uintptr_t test_spa = test_gpa;
    gpt_map_page(&ctx.g_ctx, test_gpa, test_spa,
                 PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D | PBMT_NC,
                 PT_LEVEL_4K);

    /* VS-mode access: G-stage PBMT=NC overrides PMA → final=NC */
    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_read_write,
                                           test_gpa);
    TEST_ASSERT("G-stage PBMT=NC override: access succeeds with NC attributes",
                result == 0);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}

/* HCROSS-SVPBMT-02 示例：VS-stage PBMT=IO 覆盖中间属性 */
TEST_REGISTER(test_hcross_svpbmt_02);
bool test_hcross_svpbmt_02(void) {
    TEST_BEGIN("HCROSS-SVPBMT-02: VS-stage PBMT=IO overrides intermediate");

    if (!check_h_extension()) TEST_SKIP("H extension not available");
    if (!check_svpbmt_extension()) TEST_SKIP("Svpbmt not available");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t test_gva = TEST_REGION_BASE;
    uintptr_t test_gpa = test_gva;
    uintptr_t test_spa = test_gpa;

    /* G-stage: PBMT=0 (no override, intermediate=PMA) */
    gpt_map_page(&ctx.g_ctx, test_gpa, test_spa,
                 PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D | PBMT_PMA,
                 PT_LEVEL_4K);

    /* VS-stage: PBMT=IO overrides intermediate → final=IO */
    vstage_map_page(&ctx, test_gva, test_gpa,
                    PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_IO,
                    PT_LEVEL_4K);

    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_read_write,
                                           test_gva);
    TEST_ASSERT("VS-stage PBMT=IO override: access succeeds with IO attributes",
                result == 0);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}

/* HCROSS-SVPBMT-03 示例：两阶段均非零叠加 */
TEST_REGISTER(test_hcross_svpbmt_03);
bool test_hcross_svpbmt_03(void) {
    TEST_BEGIN("HCROSS-SVPBMT-03: Both stages nonzero PBMT override");

    if (!check_h_extension()) TEST_SKIP("H extension not available");
    if (!check_svpbmt_extension()) TEST_SKIP("Svpbmt not available");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t test_gva = TEST_REGION_BASE;
    uintptr_t test_gpa = test_gva;
    uintptr_t test_spa = test_gpa;

    /* G-stage: PBMT=NC → intermediate=NC */
    gpt_map_page(&ctx.g_ctx, test_gpa, test_spa,
                 PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D | PBMT_NC,
                 PT_LEVEL_4K);

    /* VS-stage: PBMT=IO overrides intermediate=NC → final=IO */
    vstage_map_page(&ctx, test_gva, test_gpa,
                    PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_IO,
                    PT_LEVEL_4K);

    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_read_write,
                                           test_gva);
    TEST_ASSERT("Two-stage PBMT override: VS-stage IO wins, final=IO",
                result == 0);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}

/* HCROSS-SVPBMT-04 示例：hgatp.MODE=0 跳过 G-stage */
TEST_REGISTER(test_hcross_svpbmt_04);
bool test_hcross_svpbmt_04(void) {
    TEST_BEGIN("HCROSS-SVPBMT-04: hgatp.MODE=0 skips G-stage override");

    if (!check_h_extension()) TEST_SKIP("H extension not available");
    if (!check_svpbmt_extension()) TEST_SKIP("Svpbmt not available");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();

    /* hgatp.MODE=0 (Bare): G-stage disabled */
    two_stage_init(&ctx, SATP_MODE_SV39, /*hgatp=Bare*/0);

    uintptr_t test_gva = TEST_REGION_BASE;
    uintptr_t test_gpa = test_gva;

    /* VS-stage: PBMT=NC, intermediate=PMA (no G-stage) → final=NC */
    vstage_map_page(&ctx, test_gva, test_gpa,
                    PTE_V | PTE_R | PTE_W | PTE_A | PTE_D | PBMT_NC,
                    PT_LEVEL_4K);

    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_read_write,
                                           test_gva);
    TEST_ASSERT("hgatp.MODE=0: VS-stage PBMT=NC overrides PMA → final=NC",
                result == 0);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

---

## Group 8. Hypervisor × Ssstateen 交叉测试

**规范依据**：
- `norm:hstateen_rv64_csrs`：实现 H 扩展时添加 hstateen0-3 CSR
- `norm:hstateen_bit_63_op`：hstateen bit 63 控制 VS-mode 对对应 sstateen 的访问
- `norm:hstateen_bit_63_writable`：hstateen bit 63 始终可写
- `norm:sstateen_vsmode_access_roz`：hstateen 中为零的位在 VS-mode 访问 sstateen 时表现为只读零
- `norm:hstateen0_SE0_op`：hstateen0.SE0 控制 VS-mode 对 sstateen0 的访问
- `norm:hstateen0_envcfg_op`：hstateen0.ENVCFG 控制 VS-mode 对 senvcfg 的访问
- `norm:hstateen0_csrind_op`：hstateen0.CSRIND 控制 VS-mode 对 siselect/sireg* 的访问
- `norm:hstateen0_imsic_op`：hstateen0.IMSIC 控制 guest IMSIC 状态及 vstopei
- `norm:hstateen0_aia_op`：hstateen0.AIA 控制 Ssaia 非 CSRIND/IMSIC 的剩余状态
- `norm:hstateen0_context_op`：hstateen0.CONTEXT 控制 VS-mode 对 scontext 的访问
- `norm:hstateen_ro1_bits`：hstateen 位不能为 RO1 除非 mstateen 同位也为 RO1
- `norm:hstateen_encoding`：hstateen 编码与 mstateen 一致

**测试职责**：验证 Ssstateen 扩展的 hstateen CSR 在 Hypervisor 场景下的行为，包括 hstateen 的存在性与可访问性、bit 63 对 VS-mode 访问 sstateen 的控制、hstateen 只读零传播到 VS-mode、hstateen0 各功能位（SE0/ENVCFG/CSRIND/IMSIC/AIA/CONTEXT）对 VS-mode 状态的控制、以及 hstateen 的只读约束和编码一致性。

> **注意**：本组测试从 `ssstateen_test_plan.md` Groups 7-12 迁移而来。需要 H 扩展和 Ssstateen 扩展同时可用。前提配置：M-mode 需预先将对应 mstateen 位设为 1 以放行 HS-mode 对 hstateen 的访问。

#### 8.1 hstateen CSR 存在性与可访问性

**规范依据**：`norm:hstateen_rv64_csrs`、`norm:stateen_rv32_upper_bits_csrs`、`norm:hstateen_encoding`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSSTA-01 | hstateen0 在 HS-mode 可读 | 配置 mstateen0.SE0=1，HS-mode 读 hstateen0 | 无异常 |
| HCROSS-SSSTA-02 | hstateen0 在 HS-mode 可写 | 配置 mstateen0.SE0=1，HS-mode 写 hstateen0 后读回 | 无异常，可写位生效 |
| HCROSS-SSSTA-03 | hstateen1 在 HS-mode 可读写 | 配置 mstateen1 bit63=1，HS-mode 读写 hstateen1 | 无异常 |
| HCROSS-SSSTA-04 | hstateen2 在 HS-mode 可读写 | 配置 mstateen2 bit63=1，HS-mode 读写 hstateen2 | 无异常 |
| HCROSS-SSSTA-05 | hstateen3 在 HS-mode 可读写 | 配置 mstateen3 bit63=1，HS-mode 读写 hstateen3 | 无异常 |
| HCROSS-SSSTA-06 | hstateen0h 在 HS-mode 可读写 (RV32) | RV32 下配置 mstateen0.SE0=1，HS-mode 读写 hstateen0h | 无异常 |

#### 8.2 hstateen bit 63 控制 sstateen 访问

**规范依据**：`norm:hstateen_bit_63_op`、`norm:hstateen_bit_63_writable`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSSTA-07 | hstateen0 bit 63 可写（写 0） | HS-mode 将 hstateen0 bit 63 写 0 后读回 | bit 63 读回为 0 |
| HCROSS-SSSTA-08 | hstateen0 bit 63 可写（写 1） | HS-mode 将 hstateen0 bit 63 写 1 后读回 | bit 63 读回为 1 |
| HCROSS-SSSTA-09 | hstateen0.SE0=0 阻止 VS-mode 访问 sstateen0 | 设 hstateen0 bit63=0，VS-mode 读 sstateen0 | 触发 virtual-instruction 异常 (cause=22) |
| HCROSS-SSSTA-10 | hstateen0.SE0=1 允许 VS-mode 访问 sstateen0 | 设 hstateen0 bit63=1，VS-mode 读 sstateen0 | 访问正常，无异常 |
| HCROSS-SSSTA-11 | hstateen0.SE0=0 阻止 VS-mode 写 sstateen0 | 设 hstateen0 bit63=0，VS-mode 写 sstateen0 | 触发 virtual-instruction 异常 (cause=22) |
| HCROSS-SSSTA-12 | hstateen1 bit 63 可写 | HS-mode 写 hstateen1 bit 63 为 0 和 1 | 每次读回值与写入一致 |
| HCROSS-SSSTA-13 | hstateen1 bit63=0 阻止 VS-mode 访问 sstateen1 | 设 hstateen1 bit63=0，VS-mode 读 sstateen1 | 触发 virtual-instruction 异常 |
| HCROSS-SSSTA-14 | hstateen2 bit 63 控制 sstateen2 | 设 hstateen2 bit63=0/1，VS-mode 访问 sstateen2 | bit63=0 异常，bit63=1 正常 |
| HCROSS-SSSTA-15 | hstateen3 bit 63 控制 sstateen3 | 设 hstateen3 bit63=0/1，VS-mode 访问 sstateen3 | bit63=0 异常，bit63=1 正常 |

#### 8.3 hstateen 对 VS-mode sstateen 的只读零传播

**规范依据**：`norm:sstateen_vsmode_access_roz`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSSTA-16 | hstateen0.C=0 传播到 VS-mode sstateen0.C | 设 hstateen0.C=0 且 bit63=1，VS-mode 写 sstateen0.C=1 后读回 | sstateen0.C 在 VS-mode 读回 0 |
| HCROSS-SSSTA-17 | hstateen0.C=1 解除 VS-mode 传播 | 设 hstateen0.C=1 且 bit63=1，VS-mode 写 sstateen0.C=1 后读回 | sstateen0.C 在 VS-mode 读回 1 |
| HCROSS-SSSTA-18 | hstateen0.JVT=0 传播到 VS-mode sstateen0.JVT | 设 hstateen0.JVT=0 且 bit63=1，VS-mode 写 sstateen0.JVT=1 后读回 | sstateen0.JVT 在 VS-mode 读回 0 |
| HCROSS-SSSTA-19 | hstateen0 多位同时传播 | 设 hstateen0 多个功能位为 0，VS-mode 逐一验证 sstateen0 对应位 | 所有对应位在 VS-mode 下为只读零 |
| HCROSS-SSSTA-20 | hstateen0 位从 0 改 1 后传播解除 | 先设 hstateen0.C=0 验证传播，再改为 1 | sstateen0.C 在 VS-mode 变为可写 |

#### 8.4 hstateen0 各功能位控制

**规范依据**：`norm:hstateen0_SE0_op`、`norm:hstateen0_envcfg_op`、`norm:hstateen0_csrind_op`、`norm:hstateen0_imsic_op`、`norm:hstateen0_aia_op`、`norm:hstateen0_context_op`

##### SE0 位（bit 63）— sstateen0 VS-mode 访问

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSSTA-21 | hstateen0.SE0=0 阻止 VS-mode 读 sstateen0 | 设 hstateen0.SE0=0，VS-mode 读 sstateen0 | 触发 virtual-instruction 异常 |
| HCROSS-SSSTA-22 | hstateen0.SE0=0 阻止 VS-mode 写 sstateen0 | 设 hstateen0.SE0=0，VS-mode 写 sstateen0 | 触发 virtual-instruction 异常 |
| HCROSS-SSSTA-23 | hstateen0.SE0=1 允许 VS-mode 访问 sstateen0 | 设 hstateen0.SE0=1，VS-mode 读写 sstateen0 | 访问正常 |

##### ENVCFG 位（bit 62）— senvcfg VS-mode 访问

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSSTA-24 | hstateen0.ENVCFG=0 阻止 VS-mode 读 senvcfg | 设 ENVCFG=0，VS-mode 读 senvcfg | 触发 virtual-instruction 异常 |
| HCROSS-SSSTA-25 | hstateen0.ENVCFG=0 阻止 VS-mode 写 senvcfg | 设 ENVCFG=0，VS-mode 写 senvcfg | 触发 virtual-instruction 异常 |
| HCROSS-SSSTA-26 | hstateen0.ENVCFG=1 允许 VS-mode 访问 senvcfg | 设 ENVCFG=1，VS-mode 读写 senvcfg | 访问正常 |

##### CSRIND 位（bit 60）— siselect/sireg VS-mode 访问

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSSTA-27 | hstateen0.CSRIND=0 阻止 VS-mode 读 siselect | 设 CSRIND=0，VS-mode 读 siselect（实为 vsiselect） | 触发 virtual-instruction 异常 |
| HCROSS-SSSTA-28 | hstateen0.CSRIND=0 阻止 VS-mode 读 sireg* | 设 CSRIND=0，VS-mode 读 sireg（实为 vsireg） | 触发 virtual-instruction 异常 |
| HCROSS-SSSTA-29 | hstateen0.CSRIND=1 允许 VS-mode 访问 | 设 CSRIND=1，VS-mode 读 siselect/sireg* | 访问正常 |

##### IMSIC 位（bit 58）— Guest IMSIC 控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSSTA-30 | hstateen0.IMSIC=0 阻止 VS-mode 访问 IMSIC | 设 IMSIC=0，VS-mode 读 stopei（实为 vstopei） | 触发 virtual-instruction 异常 |
| HCROSS-SSSTA-31 | hstateen0.IMSIC=1 允许 VS-mode 访问 IMSIC | 设 IMSIC=1，VS-mode 读 stopei | 访问正常 |
| HCROSS-SSSTA-32 | hstateen0.IMSIC=0 等效 VGEIN=0 | 设 IMSIC=0，验证 VS-mode 无法访问 IMSIC，等效于 hstatus.VGEIN=0 | VS-mode 无法访问 guest IMSIC |

##### AIA 位（bit 59）— Ssaia 剩余状态 VS-mode 控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSSTA-33 | hstateen0.AIA=0 阻止 VS-mode Ssaia 状态 | 设 AIA=0，VS-mode 访问 Ssaia 非 CSRIND/IMSIC 状态 | 触发 virtual-instruction 异常 |
| HCROSS-SSSTA-34 | hstateen0.AIA=1 允许 VS-mode Ssaia 状态 | 设 AIA=1，VS-mode 访问 Ssaia 剩余状态 | 访问正常 |
| HCROSS-SSSTA-35 | hstateen0.AIA 不影响 CSRIND/IMSIC 控制 | 设 AIA=0 但 CSRIND=1、IMSIC=1，VS-mode 访问 siselect/stopei | 访问正常（AIA 不控制 CSRIND/IMSIC 管辖的状态） |

##### CONTEXT 位（bit 57）— scontext VS-mode 控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSSTA-36 | hstateen0.CONTEXT=0 阻止 VS-mode 读 scontext | 设 CONTEXT=0，VS-mode 读 scontext | 触发 virtual-instruction 异常 |
| HCROSS-SSSTA-37 | hstateen0.CONTEXT=0 阻止 VS-mode 写 scontext | 设 CONTEXT=0，VS-mode 写 scontext | 触发 virtual-instruction 异常 |
| HCROSS-SSSTA-38 | hstateen0.CONTEXT=1 允许 VS-mode 访问 scontext | 设 CONTEXT=1，VS-mode 读写 scontext | 访问正常 |

#### 8.5 hstateen 只读约束

**规范依据**：`norm:hstateen_ro1_bits`、`norm:stateen_warl_access`、`norm:stateen_unimplemented_state_roz`、`norm:stateen_reserved_roz`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSSTA-39 | hstateen0 RO1 位约束 | 识别 hstateen0 中任何 RO1 位，验证 mstateen0 同位也是 RO1 | mstateen0 同位也为 RO1 |
| HCROSS-SSSTA-40 | hstateen1 RO1 位约束 | 识别 hstateen1 中任何 RO1 位，验证 mstateen1 同位也是 RO1 | mstateen1 同位也为 RO1 |
| HCROSS-SSSTA-41 | hstateen2 RO1 位约束 | 识别 hstateen2 中任何 RO1 位，验证 mstateen2 同位也是 RO1 | mstateen2 同位也为 RO1 |
| HCROSS-SSSTA-42 | hstateen3 RO1 位约束 | 识别 hstateen3 中任何 RO1 位，验证 mstateen3 同位也是 RO1 | mstateen3 同位也为 RO1 |
| HCROSS-SSSTA-43 | hstateen0 保留位只读零 | 向 hstateen0 WPRI 域写 1 后读回 | 保留位读回 0 |
| HCROSS-SSSTA-44 | hstateen0 未实现扩展位只读零 | 对于未实现的扩展对应位，写 1 后读回 | 对应位读回 0 |
| HCROSS-SSSTA-45 | hstateen0 WARL 写入合法值 | 向 hstateen0 写入合法值后读回 | 读回值与写入值一致（限可写位） |

#### 8.6 hstateen 编码与 mstateen 一致性

**规范依据**：`norm:hstateen_encoding`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSSTA-46 | hstateen0 位域与 mstateen0 一致 | 对比 hstateen0 与 mstateen0 的可写位掩码 | 位域定义一致（C/FCSR/JVT/SE0/ENVCFG/CSRIND/IMSIC/AIA/CONTEXT 等位位置相同） |
| HCROSS-SSSTA-47 | hstateen0 功能位与 mstateen0 对称 | 写 hstateen0 全 1，读回有效位；写 mstateen0 全 1，读回有效位；对比重叠部分 | hstateen0 有效位应是 mstateen0 有效位的子集 |
| HCROSS-SSSTA-48 | hstateen1 编码与 mstateen1 一致 | 对比 hstateen1 与 mstateen1 的可写位掩码 | 位域定义一致 |
| HCROSS-SSSTA-49 | hstateen2 编码与 mstateen2 一致 | 对比 hstateen2 与 mstateen2 的可写位掩码 | 位域定义一致 |
| HCROSS-SSSTA-50 | hstateen3 编码与 mstateen3 一致 | 对比 hstateen3 与 mstateen3 的可写位掩码 | 位域定义一致 |

> [!NOTE]
> - 本组测试验证 Ssstateen 扩展在 Hypervisor 场景下的 hstateen CSR 行为。hstateen CSR 是 mstateen 的 supervisor-level 对应物，用于控制 VS/VU-mode 对扩展状态的访问，防止隐蔽通道。
> - 所有测试的前提配置：M-mode 需将对应 mstateen 位设为 1，以放行 HS-mode 对 hstateen 的访问。若 mstateen 对应位为 0，则 hstateen 对应位为只读零。
> - hstateen0 bit 63（SE0）控制 VS-mode 对 sstateen0 的访问权限。当 SE0=0 时，VS-mode 读写 sstateen0 触发 virtual-instruction 异常（cause=22），而非 illegal-instruction 异常（cause=2）。
> - hstateen0 的各功能位（SE0/ENVCFG/CSRIND/IMSIC/AIA/CONTEXT）分别控制 VS-mode 对不同 supervisor-level 状态的访问。这些控制位独立运作，AIA 位不影响 CSRIND/IMSIC 管辖的状态（HCROSS-SSSTA-35）。
> - hstateen 的只读零传播规则：hstateen 中为零的位（无论是只读零还是写为零），在 VS-mode 访问 sstateen 时表现为只读零（HCROSS-SSSTA-16~20）。这是防止隐蔽通道的关键机制。
> - S-mode 下的 sstateen 测试（包括 VU-mode 触发 virtual-instruction 的 SS-UCTL-09、SS-EXC-04，以及 RO1 位约束的 SS-ALLOC-03）仍保留在 `ssstateen_test_plan.md` 的 Groups 1-6 中。
> - RV32 特有的 hstateen0h 测试（HCROSS-SSSTA-06）需要 RV32 平台支持，RV64 平台上应 TEST_SKIP。

---

## Group 9. Hypervisor × Sstc 交叉测试

**规范依据**：
- `norm:henvcfg_stce`：henvcfg.STCE=1 使能 vstimecmp；STCE=0 时 V=1 下访问 stimecmp 产生 virtual-instruction 异常
- `norm:vstimecmp_exist`：Sstc 新增 VS-level vstimecmp CSR
- `norm:sstc_vs_facility`：Sstc 为 Hypervisor 扩展的 VS-mode 提供类似的定时器机制
- `norm:hip_vstip_vstie_acc_op`：VSTIP = hvip.VSTIP OR vstimecmp 定时器信号

**测试职责**：验证 Sstc 扩展在 Hypervisor 场景下的行为，包括 `henvcfg.STCE` 的可写性与约束、VS-mode 对 `stimecmp`/`vstimecmp` 的访问控制、`vstimecmp` CSR 读写、VSTIP 合成逻辑、以及 VS-mode timer interrupt 捕获。

> **注意**：本组测试从 `sstc_test_plan.md` Group 1（SSTC-STCE-03/04）、Group 3（SSTC-ACC-08/09/10）、Group 6（SSTC-VS-01~10）迁移而来。需要 H 扩展和 Sstc 扩展同时可用。

### 测试 ID 映射表

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|---------|
| SSTC-STCE-03 | HCROSS-SSTC-01 | henvcfg.STCE 读写回环 |
| SSTC-STCE-04 | HCROSS-SSTC-02 | henvcfg.STCE 受 menvcfg.STCE 约束 |
| SSTC-ACC-08 | HCROSS-SSTC-03 | henvcfg.STCE=0 时 VS-mode 访问 stimecmp |
| SSTC-ACC-09 | HCROSS-SSTC-04 | henvcfg.STCE=1 时 VS-mode 访问 stimecmp |
| SSTC-ACC-10 | HCROSS-SSTC-05 | hcounteren.TM=0 时 VS-mode 访问 stimecmp |
| SSTC-VS-01 | HCROSS-SSTC-06 | M-mode vstimecmp 读写回环 |
| SSTC-VS-02 | HCROSS-SSTC-07 | vstimecmp 全 1 / 全 0 读写 |
| SSTC-VS-03 | HCROSS-SSTC-08 | HS-mode vstimecmp 读写 |
| SSTC-VS-04 | HCROSS-SSTC-09 | vstimecmp 触发 VSTIP |
| SSTC-VS-05 | HCROSS-SSTC-10 | vstimecmp 清除 VSTIP |
| SSTC-VS-06 | HCROSS-SSTC-11 | VSTIP = hvip.VSTIP OR vstimecmp 信号 |
| SSTC-VS-07 | HCROSS-SSTC-12 | henvcfg.STCE=0 时 VSTIP 恢复旧行为 |
| SSTC-VS-08 | HCROSS-SSTC-13 | VS-mode 通过 stimecmp 访问 vstimecmp |
| SSTC-VS-09 | HCROSS-SSTC-14 | htimedelta 对 vstimecmp 比较的影响 |
| SSTC-VS-10 | HCROSS-SSTC-15 | VS-mode timer interrupt 捕获 |

### 测试用例清单

#### 9.1 henvcfg.STCE 字段控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSTC-01 | henvcfg.STCE 读写回环 | 在 M-mode 下设置 menvcfg.STCE=1 后，写 henvcfg.STCE=1 读回验证，再写 0 读回验证 | STCE bit 读写一致 | `norm:henvcfg_stce` |
| HCROSS-SSTC-02 | henvcfg.STCE 受 menvcfg.STCE 约束 | menvcfg.STCE=0 时，写 henvcfg.STCE=1 应被忽略（read-only zero） | henvcfg.STCE 读回为 0 | `norm:henvcfg_stce` |

#### 9.2 VS-mode 访问控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSTC-03 | henvcfg.STCE=0 时 VS-mode 访问 stimecmp | menvcfg.STCE=1, henvcfg.STCE=0，VS-mode (V=1) 读 stimecmp | 触发 virtual-instruction exception (cause=22) | `norm:henvcfg_stce` |
| HCROSS-SSTC-04 | henvcfg.STCE=1 时 VS-mode 访问 stimecmp | menvcfg.STCE=1, henvcfg.STCE=1, hcounteren.TM=1，VS-mode 读 stimecmp | 无异常，成功读取（实际访问 vstimecmp） | `norm:henvcfg_stce` |
| HCROSS-SSTC-05 | hcounteren.TM=0 时 VS-mode 访问 stimecmp | menvcfg.STCE=1, henvcfg.STCE=1, hcounteren.TM=0，VS-mode 读 stimecmp | 触发 virtual-instruction exception (cause=22) | `norm:henvcfg_stce` |

#### 9.3 vstimecmp CSR 读写

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSTC-06 | M-mode vstimecmp 读写回环 | M-mode 写 vstimecmp=0x123456789ABCDEF0 后读回 | 读回值一致 | `norm:vstimecmp_exist` |
| HCROSS-SSTC-07 | vstimecmp 全 1 / 全 0 读写 | M-mode 写 vstimecmp 全 1 和全 0 后分别读回 | 读回值与写入值一致 | `norm:vstimecmp_exist` |
| HCROSS-SSTC-08 | HS-mode vstimecmp 读写 | STCE=1, TM=1, 切换 HS-mode 读写 vstimecmp（通过 CSR 0x24D 直接访问） | 读写正常 | `norm:vstimecmp_exist` |

#### 9.4 VSTIP 合成逻辑

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSTC-09 | vstimecmp 触发 VSTIP | 设置 vstimecmp 为过去值（相对于 time + htimedelta），检查 hip.VSTIP | VSTIP = 1 | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-10 | vstimecmp 清除 VSTIP | 设置 vstimecmp 为最大值，检查 hip.VSTIP（假设 hvip.VSTIP=0） | VSTIP = 0 | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-11 | VSTIP = hvip.VSTIP OR vstimecmp 信号 | hvip.VSTIP=1, vstimecmp 为最大值，检查 hip.VSTIP | VSTIP = 1（hvip.VSTIP 贡献） | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-12 | henvcfg.STCE=0 时 VSTIP 恢复旧行为 | menvcfg.STCE=1, henvcfg.STCE=0, 设 vstimecmp 为过去值，检查 hip.VSTIP | VSTIP 仅由 hvip.VSTIP 决定（vstimecmp 信号不生效） | `norm:henvcfg_stce` |

#### 9.5 VS-mode 定时器功能

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSTC-13 | VS-mode 通过 stimecmp 访问 vstimecmp | STCE=1, henvcfg.STCE=1, hcounteren.TM=1, VS-mode 写 stimecmp (=vstimecmp)，M-mode 读 vstimecmp | 读回值一致（硬件重映射验证） | `norm:sstc_vs_facility` |
| HCROSS-SSTC-14 | htimedelta 对 vstimecmp 比较的影响 | 设置 htimedelta 为较大正值，设 vstimecmp 使得 (time+htimedelta) >= vstimecmp，检查 VSTIP | VSTIP = 1 | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-15 | VS-mode timer interrupt 捕获 | henvcfg.STCE=1, 使能 VSTIE, 设 vstimecmp 为过去值, VS-mode 应收到 timer interrupt | VS-mode trap handler 捕获 cause = interrupt \| 5 | `norm:sstc_vs_facility` |

### 代码示例

```c
/* HCROSS-SSTC-01: henvcfg.STCE read-write round-trip */
TEST_REGISTER(test_hcross_sstc_01);
bool test_hcross_sstc_01(void) {
    TEST_BEGIN("HCROSS-SSTC-01: henvcfg.STCE read-write round-trip");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig_m = menvcfg_read();
    uintptr_t orig_h = henvcfg_read();

    /* menvcfg.STCE must be 1 for henvcfg.STCE to be writable */
    menvcfg_set(MENVCFG_STCE);

    /* Write henvcfg.STCE=1 */
    henvcfg_set(HENVCFG_STCE);
    uintptr_t val = henvcfg_read();
    TEST_ASSERT("henvcfg.STCE set to 1", (val & HENVCFG_STCE) != 0);

    /* Write henvcfg.STCE=0 */
    henvcfg_clear(HENVCFG_STCE);
    val = henvcfg_read();
    TEST_ASSERT("henvcfg.STCE cleared to 0", (val & HENVCFG_STCE) == 0);

    /* Restore */
    henvcfg_write(orig_h);
    menvcfg_write(orig_m);
    HYP_TEST_END();
}

/* HCROSS-SSTC-03: henvcfg.STCE=0, VS-mode access -> virtual-inst */
TEST_REGISTER(test_hcross_sstc_03);
bool test_hcross_sstc_03(void) {
    TEST_BEGIN("HCROSS-SSTC-03: henvcfg.STCE=0, VS-mode -> virtual-inst");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    mcounteren_set(MCOUNTEREN_TM);
    henvcfg_clear(HENVCFG_STCE);
    hcounteren_set(HCOUNTEREN_TM);
    stimecmp_write((uintptr_t)-1);

    trap_expect_begin();
    run_in_vs_mode(_vs_stimecmp_read, 0);
    TEST_ASSERT("trap triggered", trap_was_triggered());
    if (trap_was_triggered()) {
        TEST_ASSERT_EQ("virtual-instruction on VS stimecmp read",
                       trap_get_cause(), (uintptr_t)CAUSE_VIRTUAL_INSTRUCTION);
    }
    trap_expect_end();

    stimecmp_write((uintptr_t)-1);
    HYP_TEST_END();
}

/* HCROSS-SSTC-09: vstimecmp triggers VSTIP */
TEST_REGISTER(test_hcross_sstc_09);
bool test_hcross_sstc_09(void) {
    TEST_BEGIN("HCROSS-SSTC-09: vstimecmp triggers VSTIP");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    henvcfg_set(HENVCFG_STCE);

    /* Clear hvip.VSTIP to isolate vstimecmp signal */
    hvip_clear(HVIP_VSTIP);

    /* Clear: vstimecmp = MAX */
    vstimecmp_write((uintptr_t)-1);
    DELAY_LOOP(HCROSS_SSTC_DELAY);

    /* Read time + htimedelta */
    uintptr_t htd = htimedelta_read();
    uintptr_t now = time_read();
    uintptr_t vtime = now + htd;

    /* Set vstimecmp to a past value */
    vstimecmp_write(vtime > 0 ? vtime - 1 : 0);
    DELAY_LOOP(HCROSS_SSTC_DELAY);

    uintptr_t hip_val = hip_read();
    TEST_ASSERT("VSTIP is set", (hip_val & HIP_VSTIP) != 0);

    vstimecmp_write((uintptr_t)-1);
    HYP_TEST_END();
}

/* HCROSS-SSTC-15: VS-mode timer interrupt capture */
TEST_REGISTER(test_hcross_sstc_15);
bool test_hcross_sstc_15(void) {
    TEST_BEGIN("HCROSS-SSTC-15: VS-mode timer interrupt capture");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    menvcfg_set(MENVCFG_STCE);
    mcounteren_set(MCOUNTEREN_TM);
    henvcfg_set(HENVCFG_STCE);
    hcounteren_set(HCOUNTEREN_TM);
    hvip_clear(HVIP_VSTIP);
    vstimecmp_write((uintptr_t)-1);

    /* Delegate VSTIP to VS-mode: hideleg bit 6 = VSTIP */
    CSRS(0x603, HIP_VSTIP);  /* hideleg */

    /* Enable VSTIE in vsie (bit 5 in vsie = STIE from VS perspective) */
    CSRS(0x204, SIE_STIE);  /* vsie */

    /* Install VS-mode trap handler */
    g_sstc_trap_cause = 0;
    CSRW(0x240, (uintptr_t)sstc_trap_scratch); /* vsscratch */
    CSRW(0x205, (uintptr_t)sstc_trap_entry);   /* vstvec */

    /* Execute VS-mode timer interrupt test */
    run_in_vs_mode(_vs_timer_interrupt_test, 0);

    /* Check VS trap handler recorded the cause */
    uintptr_t expected = CAUSE_INTERRUPT_BIT | CAUSE_S_TIMER_INTERRUPT;
    TEST_ASSERT_EQ("VS-mode caught timer interrupt",
                    g_sstc_trap_cause, expected);

    /* Cleanup */
    vstimecmp_write((uintptr_t)-1);
    CSRC(0x603, HIP_VSTIP);  /* hideleg */
    CSRC(0x204, SIE_STIE);   /* vsie */
    HYP_TEST_END();
}
```

> [!NOTE]
> - 本组测试验证 Sstc 扩展在 Hypervisor 场景下的行为。所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展的可用性，不可用时 TEST_SKIP。
> - HCROSS-SSTC-01~02 从 `sstc_test_plan.md` Group 1 迁移而来，验证 `henvcfg.STCE` 的可写性和 `menvcfg.STCE` 的约束关系。
> - HCROSS-SSTC-03~05 从 `sstc_test_plan.md` Group 3 迁移而来，验证 VS-mode 对 `stimecmp`（实际为 `vstimecmp`）的多层访问控制：`henvcfg.STCE` 和 `hcounteren.TM` 任一为 0 时，触发 virtual-instruction 异常（cause=22）。
> - HCROSS-SSTC-06~15 从 `sstc_test_plan.md` Group 6 整体迁移而来，验证 `vstimecmp` CSR 读写、VSTIP 合成逻辑（`hip.VSTIP = hvip.VSTIP OR vstimecmp_signal`）、`htimedelta` 对比较的影响、以及 VS-mode timer interrupt 的捕获。
> - HCROSS-SSTC-11 验证 VSTIP 的 OR 逻辑：当 `henvcfg.STCE=0` 时 vstimecmp 信号不生效，`hvip.VSTIP` 恢复软件可写并影响 `hip.VSTIP`。
> - HCROSS-SSTC-13 验证硬件透明重映射：VS-mode 写 `stimecmp`（CSR 0x14D）实际写入 `vstimecmp`（CSR 0x24D），M-mode 读回 `vstimecmp` 可验证。
> - HCROSS-SSTC-15 验证完整的 VS-mode timer interrupt 路径：通过 `hideleg` 委托 VSTIP 到 VS-mode，VS-mode trap handler 捕获 cause = `interrupt | 5`。

---

## Group 10. Hypervisor × Smcsrind 交叉测试

**规范依据**：
- `norm:sscsrind_csrs_access_control`：若 Smstateen 与 Smcsrind 同时实现，`mstateen0[60]` (CSRIND) 控制对 `siselect`、`sireg*`、`vsiselect`、`vsireg*` 的访问。当 `mstateen0[60]=0` 时，从低于 M-mode 的特权级访问这些 CSR 触发 illegal-instruction 异常。

**测试职责**：验证 Smcsrind 扩展在 Hypervisor 场景下，`mstateen0[60]` (CSRIND) 对 S-mode (HS-mode) 访问 `vsiselect`/`vsireg*` 的控制行为。注意：vsiselect/vsireg* 是 H 扩展引入的 CSR，仅在 H 扩展存在时可用。

> **注意**：本组测试从 `Smcsrind_test_plan.md` Group 4 提取而来，专门针对依赖 H 扩展的用例。需要 H 扩展、Smcsrind 扩展和 Smstateen 扩展同时可用。
>
> **前提配置**：M-mode 需预先将 `mstateen0[60]` 设为所需值以控制 HS-mode 对 vsiselect/vsireg* 的访问。

### 测试 ID 映射表

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|---------|
| MCSRIND-STA（新增） | HCROSS-SMCSRIND-01 | mstateen0[60]=0 阻止 S-mode 读 vsiselect |
| MCSRIND-STA（新增） | HCROSS-SMCSRIND-02 | mstateen0[60]=0 阻止 S-mode 写 vsiselect |
| MCSRIND-STA（新增） | HCROSS-SMCSRIND-03 | mstateen0[60]=0 阻止 S-mode 读 vsireg |
| MCSRIND-STA（新增） | HCROSS-SMCSRIND-04 | mstateen0[60]=0 阻止 S-mode 读 vsireg2~vsireg6 |
| MCSRIND-STA（新增） | HCROSS-SMCSRIND-05 | mstateen0[60]=1 允许 S-mode 访问 vsiselect |
| MCSRIND-STA（新增） | HCROSS-SMCSRIND-06 | mstateen0[60]=1 允许 S-mode 访问 vsireg* |
| MCSRIND-STA-07（补充） | HCROSS-SMCSRIND-07 | mstateen0[60]=0 不影响 M-mode 访问 vsiselect |
| MCSRIND-STA-08（补充） | HCROSS-SMCSRIND-08 | mstateen0[60]=0 不影响 M-mode 访问 vsireg* |

### 测试用例清单

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SMCSRIND-01 | mstateen0[60]=0 阻止 S-mode 读 vsiselect | mstateen0[60]=0，S-mode (HS-mode) 读 vsiselect (0x250) | 触发 illegal-instruction 异常 (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-02 | mstateen0[60]=0 阻止 S-mode 写 vsiselect | mstateen0[60]=0，S-mode 写 vsiselect | 触发 illegal-instruction 异常 (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-03 | mstateen0[60]=0 阻止 S-mode 读 vsireg | mstateen0[60]=0，S-mode 读 vsireg (0x251) | 触发 illegal-instruction 异常 (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-04 | mstateen0[60]=0 阻止 S-mode 读 vsireg2~vsireg6 | mstateen0[60]=0，S-mode 逐一读 vsireg2~vsireg6 | 每个都触发 illegal-instruction 异常 (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-05 | mstateen0[60]=1 允许 S-mode 访问 vsiselect | mstateen0[60]=1，S-mode 读写 vsiselect | 访问正常，无异常 | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-06 | mstateen0[60]=1 允许 S-mode 访问 vsireg* | mstateen0[60]=1，S-mode 读写 vsireg~vsireg6 | 访问正常，无异常（具体 vsireg_i 行为取决于 vsiselect 值） | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-07 | mstateen0[60]=0 不影响 M-mode 访问 vsiselect | mstateen0[60]=0，M-mode 读写 vsiselect | 访问正常，无异常（state-enable 不控制 M-mode） | `norm:sscsrind_csrs_access_control` |
| HCROSS-SMCSRIND-08 | mstateen0[60]=0 不影响 M-mode 访问 vsireg* | mstateen0[60]=0，M-mode 读写 vsireg~vsireg6 | 访问正常，无异常 | `norm:sscsrind_csrs_access_control` |

### 代码示例

```c
/* HCROSS-SMCSRIND-01: mstateen0[60]=0 blocks S-mode vsiselect read */
TEST_REGISTER(test_hcross_smcsrind_01);
bool test_hcross_smcsrind_01(void) {
    TEST_BEGIN("HCROSS-SMCSRIND-01: mstateen0[60]=0 blocks S-mode vsiselect read");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_smcsrind()) TEST_SKIP("Smcsrind not available");
    if (!platform_has_smstateen()) TEST_SKIP("Smstateen not available");

    /* Save original mstateen0 */
    uintptr_t orig = CSRR(0x30C);  /* mstateen0 */

    /* Clear CSRIND bit (bit 60) */
    uintptr_t val = orig & ~(1ULL << 60);
    CSRW(0x30C, val);

    /* Switch to S-mode (acts as HS-mode when H ext present, V=0)
     * and try to read vsiselect (0x250) */
    goto_priv(PRIV_S);
    PRIV_DO(CSRR(0x250));  /* vsiselect */
    goto_priv(PRIV_M);
    CHECK_TRAP("illegal-instruction on S-mode vsiselect read",
               CAUSE_ILLEGAL_INST);

    /* Restore mstateen0 */
    CSRW(0x30C, orig);
    HYP_TEST_END();
}

/* HCROSS-SMCSRIND-05: mstateen0[60]=1 allows S-mode vsiselect access */
TEST_REGISTER(test_hcross_smcsrind_05);
bool test_hcross_smcsrind_05(void) {
    TEST_BEGIN("HCROSS-SMCSRIND-05: mstateen0[60]=1 allows S-mode vsiselect");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_smcsrind()) TEST_SKIP("Smcsrind not available");
    if (!platform_has_smstateen()) TEST_SKIP("Smstateen not available");

    /* Save original mstateen0 */
    uintptr_t orig = CSRR(0x30C);  /* mstateen0 */

    /* Set CSRIND bit (bit 60) */
    uintptr_t val = orig | (1ULL << 60);
    CSRW(0x30C, val);

    /* Switch to S-mode and access vsiselect */
    trap_expect_begin();
    goto_priv(PRIV_S);
    PRIV_DO(CSRW(0x250, 0x30));  /* vsiselect = 0x30 */
    PRIV_DO(CSRR(0x250));          /* readback */
    goto_priv(PRIV_M);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("S-mode vsiselect access: no exception when CSRIND=1",
                !trapped);

    /* Restore mstateen0 */
    CSRW(0x30C, orig);
    HYP_TEST_END();
}

/* HCROSS-SMCSRIND-07: mstateen0[60]=0 does NOT affect M-mode vsiselect */
TEST_REGISTER(test_hcross_smcsrind_07);
bool test_hcross_smcsrind_07(void) {
    TEST_BEGIN("HCROSS-SMCSRIND-07: mstateen0[60]=0 does not block M-mode vsiselect");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_smcsrind()) TEST_SKIP("Smcsrind not available");
    if (!platform_has_smstateen()) TEST_SKIP("Smstateen not available");

    /* Save original mstateen0 */
    uintptr_t orig = CSRR(0x30C);  /* mstateen0 */

    /* Clear CSRIND bit (bit 60) */
    uintptr_t val = orig & ~(1ULL << 60);
    CSRW(0x30C, val);

    /* M-mode access to vsiselect should still work */
    trap_expect_begin();
    CSRW(0x250, 0x42);  /* vsiselect = 0x42 */
    uintptr_t readback = CSRR(0x250);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("M-mode vsiselect access: no exception when CSRIND=0",
                !trapped);

    /* Restore mstateen0 */
    CSRW(0x30C, orig);
    HYP_TEST_END();
}
```

> [!NOTE]
> - 本组测试验证 Smcsrind 扩展在 Hypervisor 场景下的 `mstateen0[60]` 访问控制。vsiselect/vsireg* 是 H 扩展引入的 CSR（vsiselect=0x250, vsireg=0x251, vsireg2=0x252, vsireg3=0x253, vsireg4=0x255, vsireg5=0x256, vsireg6=0x257），仅在 H 扩展存在时可用。
> - HCROSS-SMCSRIND-01~04 验证 `mstateen0[60]=0` 时 S-mode (HS-mode, V=0) 访问 vsiselect/vsireg* 触发 illegal-instruction 异常 (cause=2)。这与 S-mode 访问 siselect/sireg*（在 `Smcsrind_test_plan.md` Group 4 中覆盖）的行为对称。
> - HCROSS-SMCSRIND-05~06 验证 `mstateen0[60]=1` 时 S-mode 可以正常访问 vsiselect/vsireg*。
> - HCROSS-SMCSRIND-07~08 验证 state-enable CSR **不影响** M-mode 自身的访问。这是 SPEC 明确指出的：state-enable CSR 仅影响低于 M-mode 的特权级。
> - 与 Group 8.4 (HCROSS-SSSTA-27~29) 的区别：Group 8.4 验证 `hstateen0[60]` 对 **VS-mode** 访问 vsiselect/vsireg* 的控制（触发 virtual-instruction），本组验证 `mstateen0[60]` 对 **S-mode (HS-mode)** 访问 vsiselect/vsireg* 的控制（触发 illegal-instruction）。两者是不同层级的控制机制。
> - 所有测试必须在运行时检测 H 扩展、Smcsrind 扩展和 Smstateen 扩展的可用性，任一不可用则 TEST_SKIP。

---

## Group 11. Hypervisor × Sscsrind 交叉测试

**规范依据**：
- `norm:vsiselect_min_range`：vsiselect 至少支持 0..0xFFF
- `norm:vsiselect_msb_op`：MSB 语义
- `norm:vsireg_access_on_legal_vsiselect`：合法 vsiselect 值下 vsireg* 行为
- `norm:vsireg_access_behaviour`：vsireg_i 访问寄存器状态、只读 0 或异常
- `norm:sscsrind_vsmode_csrs_sz`：vsiselect/vsireg* 宽度始终为当前 XLEN
- `norm:sscsrind_virtual_inst_fault`：VS/VU-mode 直接访问 vsiselect/vsireg* 或 VU-mode 访问 siselect/sireg* 触发 virtual-instruction
- `norm:vsmode_virtual_inst_fault`：VS-mode 通过 sireg* 访问 vsireg* 时，vsiselect 在 HS 级实现但 VS 级未实现，通常触发 virtual-instruction
- `norm:hypervisor_impl_csrs_access_control`：H 扩展时 hstateen0[60] 控制 VS/VU-mode 对 siselect/sireg*（实际 vsiselect/vsireg*）的访问
- `norm:sscsrind_csrs_access_control`：mstateen0[60] 控制 S-mode 及以下对 siselect/sireg*/vsiselect/vsireg* 的访问
- `norm:csrs_alias`：M-level 和 S-level CSR 可为别名

**测试职责**：验证 Sscsrind 扩展在 Hypervisor 场景下的行为，包括 VS-level CSR（vsiselect/vsireg*）的基本功能、Virtual-instruction 异常行为、state-enable 访问控制、以及 VS-mode 通过 sireg* 透明访问 vsireg* 的重映射行为。

> **注意**：本组测试从 `Sscsrind_test_plan.md` Groups 2、3、4.2、4.3、5 提取而来，专门针对依赖 H 扩展的用例。需要 H 扩展和 Sscsrind 扩展同时可用。

### 测试 ID 映射表

#### 11.1 VS-level CSR 基本功能（从 Sscsrind Group 2 迁移）

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|---------|
| SSCSRIND-VSCSR-01 | HCROSS-SSCSRIND-01 | vsiselect 在 HS-mode 可读 |
| SSCSRIND-VSCSR-02 | HCROSS-SSCSRIND-02 | vsiselect 在 HS-mode 可写 |
| SSCSRIND-VSCSR-03 | HCROSS-SSCSRIND-03 | vsiselect 最小范围验证 (0..0xFFF) |
| SSCSRIND-VSCSR-04 | HCROSS-SSCSRIND-04 | vsiselect MSB=1 自定义区域 |
| SSCSRIND-VSCSR-05 | HCROSS-SSCSRIND-05 | vsiselect MSB=0 标准保留区域 |
| SSCSRIND-VSCSR-06 | HCROSS-SSCSRIND-06 | vsireg 在 HS-mode 可访问 |
| SSCSRIND-VSCSR-07 | HCROSS-SSCSRIND-07 | vsireg2~vsireg6 在 HS-mode 可访问 |
| SSCSRIND-VSCSR-08 | HCROSS-SSCSRIND-08 | vsiselect/vsireg* 宽度 = 当前 XLEN |
| SSCSRIND-VSCSR-09 | HCROSS-SSCSRIND-09 | vsiselect WARL 全 1 写入 |
| SSCSRIND-VSCSR-10 | HCROSS-SSCSRIND-10 | vsireg* 在合法 vsiselect 下的行为 |

#### 11.2 Virtual-Instruction 异常行为（从 Sscsrind Group 3 迁移）

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|---------|
| SSCSRIND-VI-01 | HCROSS-SSCSRIND-11 | VS-mode 直接读 vsiselect 触发 virtual-inst |
| SSCSRIND-VI-02 | HCROSS-SSCSRIND-12 | VS-mode 直接写 vsiselect 触发 virtual-inst |
| SSCSRIND-VI-03 | HCROSS-SSCSRIND-13 | VS-mode 直接读 vsireg 触发 virtual-inst |
| SSCSRIND-VI-04 | HCROSS-SSCSRIND-14 | VS-mode 直接读 vsireg2~vsireg6 触发 virtual-inst |
| SSCSRIND-VI-05 | HCROSS-SSCSRIND-15 | VS-mode 直接写 vsireg 触发 virtual-inst |
| SSCSRIND-VI-06 | HCROSS-SSCSRIND-16 | VU-mode 直接读 vsiselect 触发 virtual-inst |
| SSCSRIND-VI-07 | HCROSS-SSCSRIND-17 | VU-mode 直接读 vsireg 触发 virtual-inst |
| SSCSRIND-VI-08 | HCROSS-SSCSRIND-18 | VU-mode 读 siselect 触发 virtual-inst |
| SSCSRIND-VI-09 | HCROSS-SSCSRIND-19 | VU-mode 读 sireg 触发 virtual-inst |
| SSCSRIND-VI-10 | HCROSS-SSCSRIND-20 | VU-mode 写 siselect 触发 virtual-inst |
| SSCSRIND-VI-11 | HCROSS-SSCSRIND-21 | VS-mode 通过 sireg* 访问时 vsiselect 在 HS 级实现但 VS 级未实现 |

#### 11.3 State-Enable 访问控制（从 Sscsrind Group 4.2/4.3 迁移）

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|---------|
| SSCSRIND-STA-05 | HCROSS-SSCSRIND-22 | mstateen0[60]=0 阻止 HS-mode 读 vsiselect |
| SSCSRIND-STA-06 | HCROSS-SSCSRIND-23 | mstateen0[60]=0 阻止 HS-mode 读 vsireg* |
| SSCSRIND-STA-07 | HCROSS-SSCSRIND-24 | hstateen0[60]=0 + mstateen0[60]=1 → VS-mode 访问 siselect 触发 virtual-inst |
| SSCSRIND-STA-08 | HCROSS-SSCSRIND-25 | hstateen0[60]=0 + mstateen0[60]=1 → VS-mode 访问 sireg 触发 virtual-inst |
| SSCSRIND-STA-09 | HCROSS-SSCSRIND-26 | hstateen0[60]=1 + mstateen0[60]=1 → VS-mode 可访问 |
| SSCSRIND-STA-10 | HCROSS-SSCSRIND-27 | hstateen0[60]=0 时异常类型为 virtual-inst 而非 illegal-inst |

#### 11.4 Hypervisor 交叉测试（从 Sscsrind Group 5 迁移）

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|---------|
| SSCSRIND-HYP-01 | HCROSS-SSCSRIND-28 | VS-mode 通过 sireg* 访问 vsireg* 的透明重映射 |
| SSCSRIND-HYP-02 | HCROSS-SSCSRIND-29 | VS-mode 通过 siselect 访问 vsiselect 的透明重映射 |
| SSCSRIND-HYP-03 | HCROSS-SSCSRIND-30 | vsireg* 在合法 vsiselect 下读写正确 |
| SSCSRIND-HYP-04 | HCROSS-SSCSRIND-31 | vsireg* 在未实现 vsiselect 下行为 |
| SSCSRIND-HYP-05 | HCROSS-SSCSRIND-32 | HS-mode 和 VS-mode select 空间独立 |
| SSCSRIND-HYP-06 | HCROSS-SSCSRIND-33 | M-mode 和 S-mode select 空间可为别名 |

### 测试用例清单

#### 11.1 VS-level CSR 基本功能

**规范依据**：`norm:vsiselect_min_range`、`norm:vsiselect_msb_op`、`norm:vsireg_access_on_legal_vsiselect`、`norm:vsireg_access_behaviour`、`norm:sscsrind_vsmode_csrs_sz`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCSRIND-01 | vsiselect 在 HS-mode 可读 | HS-mode 读 vsiselect (0x250) | 无异常，返回合法值 | `norm:vsiselect_min_range` |
| HCROSS-SSCSRIND-02 | vsiselect 在 HS-mode 可写 | HS-mode 写 vsiselect 后读回 | 无异常，写 0 读回 0 | `norm:vsiselect_min_range` |
| HCROSS-SSCSRIND-03 | vsiselect 最小范围验证 (0..0xFFF) | HS-mode 逐一写 vsiselect=0, 1, 0x100, 0x800, 0xFFF 后读回 | 所有值均被接受（WARL），读回为合法值 | `norm:vsiselect_min_range` |
| HCROSS-SSCSRIND-04 | vsiselect MSB=1 自定义区域 | HS-mode 写 vsiselect 为 MSB=1 的值 | WARL 行为：不异常，读回为合法值 | `norm:vsiselect_msb_op` |
| HCROSS-SSCSRIND-05 | vsiselect MSB=0 标准保留区域 | HS-mode 写 vsiselect 为 MSB=0 的非分配值 | WARL 行为：不异常，读回为合法值 | `norm:vsiselect_msb_op` |
| HCROSS-SSCSRIND-06 | vsireg 在 HS-mode 可访问 | HS-mode 读 vsireg (0x251)（vsiselect 设为 0） | 行为 UNSPECIFIED：预期 illegal-instruction 或只读 0 | `norm:vsireg_access_behaviour` |
| HCROSS-SSCSRIND-07 | vsireg2~vsireg6 在 HS-mode 可访问 | HS-mode 逐一读 vsireg2~vsireg6（vsiselect 设为 0） | 行为 UNSPECIFIED：与 vsireg 类似 | `norm:vsireg_access_behaviour` |
| HCROSS-SSCSRIND-08 | vsiselect/vsireg* 宽度 = 当前 XLEN | 在 HS-mode（XLEN=64）和 VS-mode（可能 XLEN=32）分别验证 vsiselect 宽度 | 宽度始终为当前 XLEN | `norm:sscsrind_vsmode_csrs_sz` |
| HCROSS-SSCSRIND-09 | vsiselect WARL 全 1 写入 | HS-mode 向 vsiselect 写全 1 | 不触发异常，读回为合法值 | `norm:vsiselect_min_range` |
| HCROSS-SSCSRIND-10 | vsireg* 在合法 vsiselect 下的行为 | 若存在已实现的 vsiselect 值范围，HS-mode 设置该值后读 vsireg | 行为由对应扩展定义 | `norm:vsireg_access_on_legal_vsiselect` |

#### 11.2 Virtual-Instruction 异常行为

**规范依据**：`norm:sscsrind_virtual_inst_fault`、`norm:vsmode_virtual_inst_fault`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCSRIND-11 | VS-mode 直接读 vsiselect 触发 virtual-inst | VS-mode 读 vsiselect (0x250) | virtual-instruction 异常 (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-12 | VS-mode 直接写 vsiselect 触发 virtual-inst | VS-mode 写 vsiselect | virtual-instruction 异常 (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-13 | VS-mode 直接读 vsireg 触发 virtual-inst | VS-mode 读 vsireg (0x251) | virtual-instruction 异常 (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-14 | VS-mode 直接读 vsireg2~vsireg6 触发 virtual-inst | VS-mode 逐一读 vsireg2~vsireg6 | 每个都触发 virtual-instruction 异常 (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-15 | VS-mode 直接写 vsireg 触发 virtual-inst | VS-mode 写 vsireg | virtual-instruction 异常 (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-16 | VU-mode 直接读 vsiselect 触发 virtual-inst | VU-mode 读 vsiselect (0x250) | virtual-instruction 异常 (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-17 | VU-mode 直接读 vsireg 触发 virtual-inst | VU-mode 读 vsireg (0x251) | virtual-instruction 异常 (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-18 | VU-mode 读 siselect 触发 virtual-inst | VU-mode 读 siselect (0x150) | virtual-instruction 异常 (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-19 | VU-mode 读 sireg 触发 virtual-inst | VU-mode 读 sireg (0x151) | virtual-instruction 异常 (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-20 | VU-mode 写 siselect 触发 virtual-inst | VU-mode 写 siselect | virtual-instruction 异常 (cause=22) | `norm:sscsrind_virtual_inst_fault` |
| HCROSS-SSCSRIND-21 | VS-mode 通过 sireg* 访问时 vsiselect 在 HS 级实现但 VS 级未实现 | HS-mode 设 vsiselect 为 HS 级实现但 VS 级未实现的值，VS-mode 通过 sireg* 访问 | virtual-instruction 异常 (cause=22) | `norm:vsmode_virtual_inst_fault` |

#### 11.3 State-Enable 访问控制

**规范依据**：`norm:sscsrind_csrs_access_control`、`norm:hypervisor_impl_csrs_access_control`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCSRIND-22 | mstateen0[60]=0 阻止 HS-mode 读 vsiselect | mstateen0[60]=0，HS-mode（S-mode with V=0）读 vsiselect | illegal-instruction 异常 (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SSCSRIND-23 | mstateen0[60]=0 阻止 HS-mode 读 vsireg* | mstateen0[60]=0，HS-mode 读 vsireg | illegal-instruction 异常 (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SSCSRIND-24 | hstateen0[60]=0 + mstateen0[60]=1 → VS-mode 访问 siselect 触发 virtual-inst | hstateen0[60]=0 且 mstateen0[60]=1，VS-mode 读 siselect（实际 vsiselect） | virtual-instruction 异常 (cause=22)，而非 illegal-instruction | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SSCSRIND-25 | hstateen0[60]=0 + mstateen0[60]=1 → VS-mode 访问 sireg 触发 virtual-inst | hstateen0[60]=0 且 mstateen0[60]=1，VS-mode 读 sireg（实际 vsireg） | virtual-instruction 异常 (cause=22) | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SSCSRIND-26 | hstateen0[60]=1 + mstateen0[60]=1 → VS-mode 可访问 | hstateen0[60]=1 且 mstateen0[60]=1，VS-mode 读 siselect/sireg* | 访问正常（受 vsiselect 值约束） | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SSCSRIND-27 | hstateen0[60]=0 时异常类型为 virtual-inst 而非 illegal-inst | hstateen0[60]=0 且 mstateen0[60]=1，VS-mode 访问 siselect | 异常 cause 必须是 22 (virtual-inst)，不能是 2 (illegal-inst) | `norm:hypervisor_impl_csrs_access_control` |

#### 11.4 Hypervisor 交叉测试

**规范依据**：`norm:vsireg_access_on_legal_vsiselect`、`norm:vsireg_access_behaviour`、`norm:csrs_alias`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCSRIND-28 | VS-mode 通过 sireg* 访问 vsireg* 的透明重映射 | hstateen0[60]=1, mstateen0[60]=1, HS-mode 写 vsireg 一个值，VS-mode 通过 sireg（地址 0x151）读回 | VS-mode 读到的值与 HS-mode 写入 vsireg 的值一致（硬件将 sireg 重映射为 vsireg） | `norm:vsireg_access_on_legal_vsiselect` |
| HCROSS-SSCSRIND-29 | VS-mode 通过 siselect 访问 vsiselect 的透明重映射 | hstateen0[60]=1, mstateen0[60]=1, HS-mode 写 vsiselect=0x100，VS-mode 通过 siselect（地址 0x150）读回 | VS-mode 读到的值与 HS-mode 写入 vsiselect 的值一致 | `norm:vsireg_access_on_legal_vsiselect` |
| HCROSS-SSCSRIND-30 | vsireg* 在合法 vsiselect 下读写正确 | HS-mode 设置 vsiselect 为一个已实现扩展定义的合法值，写 vsireg 并读回 | 读回值与写入值一致 | `norm:vsireg_access_on_legal_vsiselect` |
| HCROSS-SSCSRIND-31 | vsireg* 在未实现 vsiselect 下行为 | HS-mode 设置 vsiselect 为未实现值（如保留范围），读 vsireg | 行为 UNSPECIFIED：预期 illegal-instruction 或只读 0 | `norm:vsireg_access_behaviour` |
| HCROSS-SSCSRIND-32 | HS-mode 和 VS-mode select 空间独立 | HS-mode 写 vsiselect=0x100，VS-mode 通过 siselect 读到 0x100；VS-mode 写 siselect=0x200（触发 virtual-inst 或被重映射），HS-mode 读 vsiselect | vsiselect 值由 HS-mode 控制，VS-mode 通过 siselect 看到的是 vsiselect 的当前值 | `norm:vsireg_access_on_legal_vsiselect` |
| HCROSS-SSCSRIND-33 | M-mode 和 S-mode select 空间可为别名 | 若依赖扩展定义了 M-level 和 S-level 别名关系，通过 miselect+sireg 和 siselect+sireg 访问同一 select 值 | 具有相同 select 值的 M-level 和 S-level CSR 可能访问相同或部分相同的寄存器状态 | `norm:csrs_alias` |

> [!NOTE]
> - 本组测试验证 Sscsrind 扩展在 Hypervisor 场景下的行为。所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展的可用性，不可用时 TEST_SKIP。
> - HCROSS-SSCSRIND-01~10 从 `Sscsrind_test_plan.md` Group 2 迁移而来，验证 vsiselect/vsireg* 的基本功能。vsiselect 的最小范围 0..0xFFF 与 siselect 一致，确保 hypervisor 可以在 VM 内模拟间接访问寄存器。
> - HCROSS-SSCSRIND-11~21 从 `Sscsrind_test_plan.md` Group 3 迁移而来，验证 Virtual-instruction 异常行为。**核心区别**：VS/VU-mode 直接访问 vsiselect/vsireg* 始终触发 virtual-instruction（cause=22），**不论** mstateen0[60] 或 hstateen0[60] 的值。这是 Sscsrind SPEC 中 `norm:sscsrind_virtual_inst_fault` 的明确要求。
> - HCROSS-SSCSRIND-22~27 从 `Sscsrind_test_plan.md` Group 4.2/4.3 迁移而来，验证 state-enable 访问控制。关键区别：当 mstateen0[60]=1 但 hstateen0[60]=0 时，VS/VU-mode 访问 siselect/sireg* 触发的是 **virtual-instruction**（cause=22），而非 illegal-instruction（cause=2）。这是因为 M-mode 已放行（mstateen=1），但 HS-mode 的 hypervisor 选择不放行（hstateen=0），因此异常类型反映了 hypervisor 需要 trap 并处理。
> - HCROSS-SSCSRIND-28~33 从 `Sscsrind_test_plan.md` Group 5 迁移而来，验证 Hypervisor 交叉测试。HCROSS-SSCSRIND-28~29 验证硬件透明重映射的核心行为：在 VM 内，VS-mode 访问 siselect/sireg*（地址 0x150~0x157）时，硬件自动将其重映射为 vsiselect/vsireg*（地址 0x250~0x257）。这对 guest OS 是透明的。
> - 与 Group 8.4 (HCROSS-SSSTA-27~29) 和 Group 10 (HCROSS-SMCSRIND-01~08) 的区别：Group 8.4 从 Smstateen 角度验证 hstateen0[60] 控制，Group 10 验证 mstateen0[60] 对 HS-mode 访问 vsiselect/vsireg* 的控制，本组从 Sscsrind 角度验证 hstateen0[60] 控制和 VS/VU-mode 异常行为。实现时应避免重复，可标记为交叉引用。

---

## Group 12. Hypervisor × Ssdbltrp 交叉测试

**规范依据**：
- `norm:henvcfg_DTE`：若实现 H 扩展，添加 henvcfg.DTE 字段
- `norm:henvcfg_dte_op`：henvcfg.DTE=0 时 VS-mode 行为如同 Ssdbltrp 未实现；vsstatus.SDT 只读零
- `norm:menvcfg_dte_op`：menvcfg.DTE=0 时 henvcfg.DTE 只读零
- `norm:vsstatus_SDT`：若实现 H 扩展，添加 vsstatus.SDT 字段
- `norm:vsstatus_sdt_op`：vsstatus.SDT 用于处理 VS-mode 双陷阱
- `norm:sstatus_sdt_trap`：SDT 机制在 VS-mode 下同样适用（通过 vsstatus.SDT）
- `norm:sstatus_sdt_sstatus_sie_overwrite`：vsstatus.SDT 和 vsstatus.SIE 的互斥约束
- `norm:sret_dt`：HS-mode SRET 到 VU 时清 vsstatus.SDT；VS-mode SRET 清 vsstatus.SDT
- `norm:vsstatus_sdt_clr_mret_sret`：MRET 或 M-mode SRET 新模式为 VU 时清 vsstatus.SDT
- `norm:vsstatus_sdt_clr_mnret`：MNRET 新模式为 VU 时清 vsstatus.SDT
- `norm:sstatus_sdt_clr_mret_sret`：MRET 或 M-mode SRET 新模式为 VS/VU 时清 sstatus.SDT
- `norm:HS_mode_invoke_error`：HS-mode 可在 VS-mode 双陷阱时调用虚拟机中的关键错误处理器

**测试职责**：验证 Ssdbltrp 扩展在 Hypervisor 场景下的行为，包括 `henvcfg.DTE` 对 VS-mode 的使能/禁用控制、`vsstatus.SDT` 字段行为、SRET 对 `vsstatus.SDT` 的清除、以及 MRET/SRET/MNRET 在 Hypervisor 场景下对 SDT/vsstatus.SDT 的跨模式清除。

> **注意**：本组测试从 `Ssdbltrp_test_plan.md` Groups 3/4/5/6/7 提取而来，专门针对依赖 H 扩展的用例。需要 H 扩展和 Ssdbltrp 扩展同时可用。

### 测试 ID 映射表

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|---------|
| HDTE-01 | HCROSS-SSDBLTRP-01 | henvcfg.DTE 读写 |
| HDTE-02 | HCROSS-SSDBLTRP-02 | henvcfg.DTE=0 时 vsstatus.SDT 只读零 |
| HDTE-03 | HCROSS-SSDBLTRP-03 | henvcfg.DTE=1 时 vsstatus.SDT 可写 |
| HDTE-04 | HCROSS-SSDBLTRP-04 | henvcfg.DTE=0 时 VS-mode trap 不设 SDT |
| HDTE-05 | HCROSS-SSDBLTRP-05 | menvcfg.DTE=0 覆盖 henvcfg.DTE |
| HDTE-06 | HCROSS-SSDBLTRP-06 | henvcfg.DTE 动态切换 |
| VSDT-01 | HCROSS-SSDBLTRP-07 | vsstatus.SDT WARL 读写 |
| VSDT-02 | HCROSS-SSDBLTRP-08 | vsstatus.SDT=1 写时自动清零 vsstatus.SIE |
| VSDT-03 | HCROSS-SSDBLTRP-09 | vsstatus.SDT=1 时无法设置 vsstatus.SIE=1 |
| VSDT-04 | HCROSS-SSDBLTRP-10 | VS-mode trap 时 vsstatus.SDT 自动设为 1 |
| VSDT-05 | HCROSS-SSDBLTRP-11 | VS-mode SDT=1 时 trap 触发 double-trap |
| VSDT-06 | HCROSS-SSDBLTRP-12 | VS-mode double-trap 时 M-mode CSR 正确 |
| SRET-03 | HCROSS-SSDBLTRP-13 | HS-mode SRET 到 VU 清除 vsstatus.SDT |
| SRET-04 | HCROSS-SSDBLTRP-14 | VS-mode SRET 清除 vsstatus.SDT |
| SRET-05 | HCROSS-SSDBLTRP-15 | HS-mode SRET 到 VS 不清 vsstatus.SDT |
| SRET-06 | HCROSS-SSDBLTRP-16 | HS-mode SRET 到 HS 不清 vsstatus.SDT |
| DTE-03 | HCROSS-SSDBLTRP-17 | menvcfg.DTE=0 时 vsstatus.SDT 只读零 |
| DTE-04 | HCROSS-SSDBLTRP-18 | menvcfg.DTE=0 时 henvcfg.DTE 只读零 |
| XRET-03 | HCROSS-SSDBLTRP-19 | MRET 到 VS-mode 清除 sstatus.SDT |
| XRET-04 | HCROSS-SSDBLTRP-20 | MRET 到 VU-mode 清除 sstatus.SDT 和 vsstatus.SDT |
| XRET-05 | HCROSS-SSDBLTRP-21 | MRET 到 VU 只清 vsstatus.SDT |
| XRET-06 | HCROSS-SSDBLTRP-22 | MRET 到 VS-mode 不清 vsstatus.SDT |
| XRET-08 | HCROSS-SSDBLTRP-23 | M-mode SRET 到 VU 清除 sstatus.SDT 和 vsstatus.SDT |
| XRET-10 | HCROSS-SSDBLTRP-24 | MNRET 到 VU 清除 sstatus.SDT 和 vsstatus.SDT |

### 测试用例清单

#### 12.1 henvcfg.DTE 控制

**规范依据**：`norm:henvcfg_DTE`、`norm:henvcfg_dte_op`、`norm:menvcfg_dte_op`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSDBLTRP-01 | henvcfg.DTE 读写 | HS-mode 写 henvcfg.DTE=1 后读回验证（menvcfg.DTE=1） | 若 Ssdbltrp 和 H 扩展均实现，DTE 可写且读回一致 | `norm:henvcfg_DTE` |
| HCROSS-SSDBLTRP-02 | henvcfg.DTE=0 时 vsstatus.SDT 只读零 | henvcfg.DTE=0（menvcfg.DTE=1），VS-mode 尝试写 vsstatus.SDT=1 | vsstatus.SDT 读回 0（只读零） | `norm:henvcfg_dte_op` |
| HCROSS-SSDBLTRP-03 | henvcfg.DTE=1 时 vsstatus.SDT 可写 | henvcfg.DTE=1（menvcfg.DTE=1），写 vsstatus.SDT=1 | vsstatus.SDT=1 可写 | `norm:henvcfg_dte_op` |
| HCROSS-SSDBLTRP-04 | henvcfg.DTE=0 时 VS-mode trap 不设 SDT | henvcfg.DTE=0，VS-mode 触发 ecall 正常 trap | vsstatus.SDT 不被硬件修改（功能不存在） | `norm:henvcfg_dte_op` |
| HCROSS-SSDBLTRP-05 | menvcfg.DTE=0 覆盖 henvcfg.DTE | menvcfg.DTE=0，尝试写 henvcfg.DTE=1 | henvcfg.DTE 只读零（menvcfg.DTE 是全局使能） | `norm:menvcfg_dte_op` |
| HCROSS-SSDBLTRP-06 | henvcfg.DTE 动态切换 | menvcfg.DTE=1，先设 henvcfg.DTE=1 验证 vsstatus.SDT 可写；再设 henvcfg.DTE=0 验证只读零 | DTE=1 时可写；DTE=0 后只读零 | `norm:henvcfg_dte_op` |

#### 12.2 vsstatus.SDT（VS-mode）

**规范依据**：`norm:vsstatus_SDT`、`norm:vsstatus_sdt_op`、`norm:sstatus_sdt_trap`、`norm:sstatus_sdt_sstatus_sie_overwrite`、`norm:HS_mode_invoke_error`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSDBLTRP-07 | vsstatus.SDT WARL 读写 | HS-mode 写 vsstatus.SDT=1（henvcfg.DTE=1, menvcfg.DTE=1）后读回 | vsstatus.SDT 可写且读回一致 | `norm:vsstatus_SDT` |
| HCROSS-SSDBLTRP-08 | vsstatus.SDT=1 写时自动清零 vsstatus.SIE | 写 vsstatus.SDT=1（同时写 SIE=1） | vsstatus.SIE 被强制清零 | `norm:sstatus_sdt_sstatus_sie_overwrite` |
| HCROSS-SSDBLTRP-09 | vsstatus.SDT=1 时无法设置 vsstatus.SIE=1 | 设 vsstatus.SDT=1，然后写 vsstatus.SIE=1 | vsstatus.SIE 读回为 0 | `norm:sstatus_sdt_sstatus_sie_overwrite` |
| HCROSS-SSDBLTRP-10 | VS-mode trap 时 vsstatus.SDT 自动设为 1 | henvcfg.DTE=1，设 vsstatus.SDT=0，从 VU-mode 触发 ecall trap 到 VS-mode | vsstatus.SDT=1 | `norm:sstatus_sdt_trap` |
| HCROSS-SSDBLTRP-11 | VS-mode SDT=1 时 trap 触发 double-trap | henvcfg.DTE=1，设 vsstatus.SDT=1，VU-mode 触发 ecall（委托到 VS-mode） | double-trap 异常交付到 M-mode（mcause=16），mtval2=8（ecall-from-VU） | `norm:sstatus_sdt_trap` |
| HCROSS-SSDBLTRP-12 | VS-mode double-trap 时 M-mode CSR 正确 | VS-mode double-trap 到 M-mode | mstatus.MPV=1, MPP 正确, mepc=触发地址, mcause=16, mtval2=原始 cause | `norm:sstatus_sdt_trap` |

#### 12.3 SRET 对 vsstatus.SDT 的清除

**规范依据**：`norm:sret_dt`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSDBLTRP-13 | HS-mode SRET 到 VU 清除 vsstatus.SDT | HS-mode 设 vsstatus.SDT=1，配置 SRET 返回到 VU-mode（SPV=1, SPP=0） | vsstatus.SDT=0 | `norm:sret_dt` |
| HCROSS-SSDBLTRP-14 | VS-mode SRET 清除 vsstatus.SDT | VS-mode 设 vsstatus.SDT=1，执行 SRET | vsstatus.SDT=0 | `norm:sret_dt` |
| HCROSS-SSDBLTRP-15 | HS-mode SRET 到 VS 不清 vsstatus.SDT | HS-mode 设 vsstatus.SDT=1，配置 SRET 返回到 VS-mode（SPV=1, SPP=1） | vsstatus.SDT 保持 1（仅 VU 模式才清除 vsstatus.SDT） | `norm:sret_dt` |
| HCROSS-SSDBLTRP-16 | HS-mode SRET 到 HS 不清 vsstatus.SDT | HS-mode 设 vsstatus.SDT=1，配置 SRET 返回到 HS-mode（SPV=0） | vsstatus.SDT 保持 1 | `norm:sret_dt` |

#### 12.4 menvcfg.DTE 对 Hypervisor CSR 的控制

**规范依据**：`norm:menvcfg_dte_op`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSDBLTRP-17 | menvcfg.DTE=0 时 vsstatus.SDT 只读零 | menvcfg.DTE=0，尝试写 vsstatus.SDT=1（H 扩展存在时） | vsstatus.SDT 读回 0（只读零） | `norm:menvcfg_dte_op` |
| HCROSS-SSDBLTRP-18 | menvcfg.DTE=0 时 henvcfg.DTE 只读零 | menvcfg.DTE=0，尝试写 henvcfg.DTE=1（H 扩展存在时） | henvcfg.DTE 读回 0（只读零） | `norm:menvcfg_dte_op` |

#### 12.5 MRET/SRET/MNRET 跨模式清除（Hypervisor 场景）

**规范依据**：`norm:sstatus_sdt_clr_mret_sret`、`norm:vsstatus_sdt_clr_mret_sret`、`norm:vsstatus_sdt_clr_mnret`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSDBLTRP-19 | MRET 到 VS-mode 清除 sstatus.SDT | 设 sstatus.SDT=1, mstatus.MPP=1(S), MPV=1, 执行 MRET | sstatus.SDT=0 | `norm:sstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-20 | MRET 到 VU-mode 清除 sstatus.SDT 和 vsstatus.SDT | 设 sstatus.SDT=1, vsstatus.SDT=1, MPP=0(U), MPV=1, 执行 MRET | sstatus.SDT=0 且 vsstatus.SDT=0 | `norm:sstatus_sdt_clr_mret_sret` `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-21 | MRET 到 VU 只清 vsstatus.SDT | 设 sstatus.SDT=0, vsstatus.SDT=1, MPP=0(U), MPV=1, 执行 MRET | vsstatus.SDT=0 | `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-22 | MRET 到 VS-mode 不清 vsstatus.SDT | 设 vsstatus.SDT=1, MPP=1(S), MPV=1, 执行 MRET | vsstatus.SDT 保持 1（仅 VU 清除 vsstatus.SDT） | `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-23 | M-mode SRET 到 VU 清除 sstatus.SDT 和 vsstatus.SDT | M-mode 设 SDT=1, vsstatus.SDT=1, MPP=0, SPV=1, 执行 SRET | sstatus.SDT=0 且 vsstatus.SDT=0 | `norm:sstatus_sdt_clr_mret_sret` `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-24 | MNRET 到 VU 清除 sstatus.SDT 和 vsstatus.SDT | 设 SDT=1, vsstatus.SDT=1, MNPP=0(U), MNPV=1, 执行 MNRET | sstatus.SDT=0 且 vsstatus.SDT=0 | `norm:vsstatus_sdt_clr_mnret` |

### 代码示例

```c
/* HCROSS-SSDBLTRP-01: henvcfg.DTE read-write round-trip */
TEST_REGISTER(test_hcross_ssdbltrp_01);
bool test_hcross_ssdbltrp_01(void) {
    TEST_BEGIN("HCROSS-SSDBLTRP-01: henvcfg.DTE read-write");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    uintptr_t orig_m = menvcfg_read();
    uintptr_t orig_h = henvcfg_read();

    /* menvcfg.DTE must be 1 for henvcfg.DTE to be writable */
    menvcfg_set(MENVCFG_DTE);

    /* Write henvcfg.DTE=1 */
    henvcfg_set(HENVCFG_DTE);
    uintptr_t val = henvcfg_read();
    TEST_ASSERT("henvcfg.DTE set to 1", (val & HENVCFG_DTE) != 0);

    /* Write henvcfg.DTE=0 */
    henvcfg_clear(HENVCFG_DTE);
    val = henvcfg_read();
    TEST_ASSERT("henvcfg.DTE cleared to 0", (val & HENVCFG_DTE) == 0);

    /* Restore */
    henvcfg_write(orig_h);
    menvcfg_write(orig_m);
    HYP_TEST_END();
}

/* HCROSS-SSDBLTRP-10: VS-mode trap sets vsstatus.SDT */
TEST_REGISTER(test_hcross_ssdbltrp_10);
bool test_hcross_ssdbltrp_10(void) {
    TEST_BEGIN("HCROSS-SSDBLTRP-10: VS-mode trap sets vsstatus.SDT=1");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    menvcfg_set(MENVCFG_DTE);
    henvcfg_set(HENVCFG_DTE);

    /* Clear vsstatus.SDT */
    vsstatus_clear(VSSTATUS_SDT);

    /* Delegate ecall-from-VU to VS-mode */
    medeleg_set(1UL << 8);  /* ecall-from-VU */
    hedeleg_set(1UL << 8);

    /* Run VU-mode ecall -> trap to VS-mode */
    run_in_vu_mode(_vu_ecall_wrapper);

    /* Check vsstatus.SDT was set by hardware */
    uintptr_t vs_sdt = (vsstatus_read() & VSSTATUS_SDT) != 0;
    TEST_ASSERT("vsstatus.SDT=1 after VS-mode trap", vs_sdt);

    /* Cleanup */
    medeleg_clear(1UL << 8);
    hedeleg_clear(1UL << 8);
    HYP_TEST_END();
}

/* HCROSS-SSDBLTRP-13: HS-mode SRET to VU clears vsstatus.SDT */
TEST_REGISTER(test_hcross_ssdbltrp_13);
bool test_hcross_ssdbltrp_13(void) {
    TEST_BEGIN("HCROSS-SSDBLTRP-13: HS-mode SRET to VU clears vsstatus.SDT");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssdbltrp_extension()) TEST_SKIP("Ssdbltrp not available");

    menvcfg_set(MENVCFG_DTE);
    henvcfg_set(HENVCFG_DTE);

    /* Set vsstatus.SDT=1 */
    vsstatus_set(VSSTATUS_SDT);

    /* Configure SRET to return to VU-mode: SPV=1, SPP=0(U) */
    hstatus_set(HSTATUS_SPV);
    sstatus_clear(SSTATUS_SPP);

    /* Execute SRET from HS-mode */
    asm volatile("sret");

    /* Now in VU-mode, trap back to HS-mode via ecall */
    /* Check vsstatus.SDT was cleared */
    uintptr_t vs_sdt = (vsstatus_read() & VSSTATUS_SDT) != 0;
    TEST_ASSERT("vsstatus.SDT=0 after HS-mode SRET to VU", !vs_sdt);

    HYP_TEST_END();
}
```

> [!NOTE]
> - 本组测试验证 Ssdbltrp 扩展在 Hypervisor 场景下的行为。所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展的可用性，不可用时 TEST_SKIP。
> - HCROSS-SSDBLTRP-01~06 从 `Ssdbltrp_test_plan.md` Group 5 迁移而来，验证 `henvcfg.DTE` 的可写性和对 VS-mode Ssdbltrp 功能的使能/禁用控制。`menvcfg.DTE` 是全局使能，`henvcfg.DTE` 是 VS-mode 级别的使能。
> - HCROSS-SSDBLTRP-07~12 从 `Ssdbltrp_test_plan.md` Group 6 迁移而来，验证 `vsstatus.SDT` 的 WARL 读写、SDT/SIE 互斥、以及 VS-mode double-trap 交付机制。VS-mode double-trap 最终交付到 M-mode（mcause=16），且 `mstatus.MPV=1` 标识来自虚拟化模式。
> - HCROSS-SSDBLTRP-13~16 从 `Ssdbltrp_test_plan.md` Group 3 迁移而来，验证 SRET 对 `vsstatus.SDT` 的清除行为。关键规则：HS-mode SRET **仅在返回到 VU-mode 时**清除 vsstatus.SDT；返回 VS-mode 或 HS-mode 不清除。VS-mode 自身的 SRET 始终清除 vsstatus.SDT。
> - HCROSS-SSDBLTRP-17~18 从 `Ssdbltrp_test_plan.md` Group 4 迁移而来，验证 `menvcfg.DTE=0` 时对 Hypervisor CSR（vsstatus.SDT、henvcfg.DTE）的全局禁用效果。
> - HCROSS-SSDBLTRP-19~24 从 `Ssdbltrp_test_plan.md` Group 7 迁移而来，验证 MRET/SRET/MNRET 在 Hypervisor 场景下的跨模式 SDT 清除。核心规则：sstatus.SDT 在新模式为 U/VS/VU 时清除；vsstatus.SDT **仅在新模式为 VU 时**清除。
> - 与 Group 6 (HCROSS-SVNAPOT) 和 Group 9 (HCROSS-SSTC) 的区别：本组专注于 double-trap 机制在虚拟化场景下的行为，不涉及地址翻译或定时器功能。

---

## Group 13. Hypervisor × Smctr 交叉测试

**规范依据**：
- `norm:hstateen_ctr`：若实现 H 扩展且 `mstateen0.CTR=1`，`hstateen0.CTR` 位控制 V=1 时对 supervisor CTR 状态的访问；`mstateen0.CTR=0` 时 `hstateen0.CTR` 只读零
- `norm:hstateen_vs`：`hstateen0.CTR=0` 时 VS-mode 访问 CTR 状态和 SCTRCLR 触发 virtual-instruction 异常
- `norm:hstateen0_CTR0-V1_op`：`hstateen0.CTR=0` 时 V=1 期间的合格控制转换仍隐式更新 entry 寄存器和 `sctrstatus`
- `norm:mstateen_ctr0_execpt1`：`mstateen0.CTR=0` 阻止访问 `vsctrctl`
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

#### 13.1 mstateen0.CTR 对 Hypervisor CSR 的控制

**规范依据**：`norm:mstateen_ctr0_execpt1`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SMCTR-01 | mstateen0.CTR=0 阻止 S-mode 访问 vsctrctl | mstateen0.CTR=0，HS-mode 尝试读 vsctrctl | 触发 illegal-instruction 异常（cause=2） | `norm:mstateen_ctr0_execpt1` |

#### 13.2 hstateen0.CTR 访问控制

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

#### 13.3 MTE 外部陷阱（VS/VU-mode → M-mode）

**规范依据**：`norm:exttrap_vsm`、`norm:exttrap_vum`、`norm:exttrap_implreq`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SMCTR-10 | MTE 外部陷阱录制（VS→M） | mctrctl.M=0，MTE=1，sctrctl.STE=1，VS-mode 触发陷阱到 M-mode | 外部陷阱被录制（需 MTE 和 STE） | `norm:exttrap_vsm` |
| HCROSS-SMCTR-11 | MTE 外部陷阱录制（VU→M，需 MTE+STE+vsSTE） | mctrctl.M=0，MTE=1，sctrctl.STE=1，vsctrctl.STE=1，VU-mode 触发陷阱到 M-mode | 外部陷阱被录制（需所有三个 TE 位置位） | `norm:exttrap_vum` |
| HCROSS-SMCTR-12 | 外部陷阱 VU→M 缺少 vsSTE 时不录制 | mctrctl.M=0，MTE=1，STE=1，vsctrctl.STE=0，VU-mode 触发陷阱到 M-mode | 外部陷阱不被录制（vsctrctl.STE 未置位） | `norm:exttrap_vum` |

### 代码示例

```c
/* HCROSS-SMCTR-04: hstateen0.CTR=0 blocks VS-mode sctrctl access */
TEST_REGISTER(test_hcross_smctr_04);
bool test_hcross_smctr_04(void) {
    TEST_BEGIN("HCROSS-SMCTR-04: hstateen0.CTR=0 blocks VS-mode sctrctl");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_smctr()) TEST_SKIP("Smctr not available");
    if (!platform_has_smstateen()) TEST_SKIP("Smstateen not available");

    /* Enable mstateen0.CTR so hstateen0.CTR is writable */
    mstateen0_set(MSTATEEN0_CTR);

    /* Set hstateen0.CTR=0 to block VS-mode */
    hstateen0_clear(HSTATEEN0_CTR);

    /* Switch to VS-mode and try to read sctrctl (actually vsctrctl) */
    trap_expect_begin();
    run_in_vs_mode(_vs_read_sctrctl);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("VS-mode sctrctl access blocked: trap triggered", trapped);
    if (trapped) {
        TEST_ASSERT_EQ("virtual-instruction exception",
                       trap_get_cause(), CAUSE_VIRTUAL_INSTRUCTION);
    }

    HYP_TEST_END();
}

/* HCROSS-SMCTR-10: MTE external trap recording VS->M */
TEST_REGISTER(test_hcross_smctr_10);
bool test_hcross_smctr_10(void) {
    TEST_BEGIN("HCROSS-SMCTR-10: MTE external trap VS->M recorded");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!platform_has_smctr()) TEST_SKIP("Smctr not available");

    /* Configure: M=0 (no M-mode recording), MTE=1, STE=1 */
    mctrctl_write(MCTRCTL_MTE | MCTRCTL_STE);

    /* Enable S-mode recording for external trap path */
    sctrctl_write(SCTRCTL_S);

    /* Clear CTR entries */
    sctrclr();

    /* VS-mode triggers trap to M-mode (e.g., ecall not delegated) */
    medeleg_clear(1UL << 10);  /* ecall-from-VS not delegated */
    run_in_vs_mode(_vs_ecall_wrapper);

    /* Check entry 0: external trap should be recorded */
    uintptr_t ctrdata0 = read_entry_ctrdata(0);
    TEST_ASSERT("external trap VS->M recorded",
                (ctrdata0 & CTRDATA_TYPE_MASK) != 0);

    /* ctrtarget.PC should be 0 for external traps */
    uintptr_t ctrtarget0 = read_entry_ctrtarget(0);
    TEST_ASSERT_EQ("ctrtarget.PC = 0 for external trap",
                   ctrtarget0 & CTARGET_PC_MASK, 0);

    HYP_TEST_END();
}
```

> [!NOTE]
> - 本组测试验证 Smctr 扩展在 Hypervisor 场景下的行为。所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展的可用性，不可用时 TEST_SKIP。
> - HCROSS-SMCTR-01 从 `Smctr_test_plan.md` Group 2 迁移而来，验证 `mstateen0.CTR=0` 对 `vsctrctl`（H 扩展引入的 CSR）的阻止。触发的是 illegal-instruction（cause=2），因为这是 M-mode 对 S-mode 的控制。
> - HCROSS-SMCTR-02~09 从 `Smctr_test_plan.md` Group 2 迁移而来，验证 `hstateen0.CTR` 对 VS-mode CTR 状态访问的控制。核心规则：`hstateen0.CTR=0` 时 VS-mode 访问 CTR 状态触发 virtual-instruction（cause=22），而非 illegal-instruction。这与 `mstateen0.CTR=0` 时触发 illegal-instruction 不同——因为 M-mode 已放行（mstateen0=1），但 HS-mode 选择不放行（hstateen0=0）。
> - HCROSS-SMCTR-09 验证关键语义：即使 `hstateen0.CTR=0` 阻止了 VS-mode 对 CTR CSR 的**显式访问**，V=1 期间执行的合格控制转换仍会**隐式更新** entry 寄存器和 `sctrstatus`。这是为了防止 hypervisor 通过禁用 CTR 来干扰 guest 的转换录制。
> - HCROSS-SMCTR-10~12 从 `Smctr_test_plan.md` Group 3 迁移而来，验证 MTE 外部陷阱在 Hypervisor 场景下的录制。VS→M 需要 MTE+STE；VU→M 需要 MTE+STE+vsctrctl.STE（三个 TE 位）。这体现了外部陷阱录制对中间模式 TE 位的依赖。
> - 与 Group 8 (HCROSS-SMSTA) 的区别：Group 8 验证 `hstateen0` 的 SE0/ENVCFG/CSRIND 等功能位，本组验证 `hstateen0.CTR` 位对 CTR 状态的控制。

---

## Group 14. Hypervisor × Ssctr 交叉测试

**规范依据**：
- `norm:Ssctr_vsctrctl_sz_acc_op`：若实现 H 扩展，`vsctrctl` 是 64-bit 读写寄存器，V=1 时替代 `sctrctl`
- `norm:vsctr-s_op`：S 字段启用 VS-mode 录制
- `norm:vsctrctl-u_op`：U 字段启用 VU-mode 录制
- `norm:vsctrctl-ste_op`：STE 字段启用陷阱到 VS-mode 的录制
- `norm:vsctrctl-bpfrz_op`：BPFRZ 字段在 VS-mode breakpoint 时设置 FROZEN
- `norm:vsctrctl-lcofifrz_op`：LCOFIFRZ 字段在 VS-mode LCOFI 时设置 FROZEN
- `norm:exttrap_vshs`：VS→HS 外部陷阱需要 `sctrctl.STE`
- `norm:exttrap_vuhs`：VU→HS 外部陷阱需要 `sctrctl.STE + vsctrctl.STE`
- `norm:exttrap_vuvs`：VU→VS 外部陷阱需要 `vsctrctl.STE`
- `norm:ctr_freeze_vs`：VS-mode 的 freeze 行为由 `vsctrctl` 中的 LCOFIFRZ/BPFRZ 决定
- `norm:sctrdepth_mode`：VS-mode/VU-mode 访问 `sctrdepth` 触发 virtual-instruction
- `norm:sctrclr_exceptions`：VU-mode 执行 SCTRCLR 触发 virtual-instruction
- `norm:vsiselect_op`：V=1 时 `vsireg*` 提供与 `sireg*` 相同的 CTR entry 状态访问
- `norm:hstateen_ctr` / `norm:hstateen_vs`：`hstateen0.CTR` 控制 VS-mode 对 CTR 状态的访问

**测试职责**：验证 Ssctr 扩展在 Hypervisor 场景下的行为，包括 `vsctrctl` CSR 功能、VS/VU-mode 外部陷阱录制、虚拟化模式转换配置来源、VS-mode Freeze 行为、VS-mode 对 sctrdepth/SCTRCLR 的访问限制、以及 `hstateen0.CTR` 对 VS-mode CTR 访问的控制。

> **注意**：本组测试从 `Ssctr_test_plan.md` Groups 2/3/5/8/12/13/14 提取而来，专门针对依赖 H 扩展的用例。需要 H 扩展和 Ssctr 扩展同时可用。

### 测试 ID 映射表

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|---------|
| SSCTR-VSCTL-01 | HCROSS-SSCTR-01 | vsctrctl 基本读写（HS-mode） |
| SSCTR-VSCTL-02 | HCROSS-SSCTR-02 | V=1 时 sctrctl 实际访问 vsctrctl |
| SSCTR-VSCTL-03 | HCROSS-SSCTR-03 | V=1 时写 sctrctl 实际写 vsctrctl |
| SSCTR-VSCTL-04 | HCROSS-SSCTR-04 | vsctrctl.S 字段（VS-mode 录制） |
| SSCTR-VSCTL-05 | HCROSS-SSCTR-05 | vsctrctl.U 字段（VU-mode 录制） |
| SSCTR-VSCTL-06 | HCROSS-SSCTR-06 | vsctrctl.STE 字段 |
| SSCTR-VSCTL-07 | HCROSS-SSCTR-07 | vsctrctl.BPFRZ 字段 |
| SSCTR-VSCTL-08 | HCROSS-SSCTR-08 | vsctrctl.LCOFIFRZ 字段 |
| SSCTR-VSCTL-09 | HCROSS-SSCTR-09 | V=0 时 vsctrctl 不影响行为 |
| SSCTR-VSCTL-10 | HCROSS-SSCTR-10 | vsctrctl 字段与 sctrctl 匹配 |
| SSCTR-DEP-07 | HCROSS-SSCTR-11 | VS-mode 访问 sctrdepth 触发异常 |
| SSCTR-DEP-08 | HCROSS-SSCTR-12 | VU-mode 访问 sctrdepth 触发异常 |
| SSCTR-ENT-10 | HCROSS-SSCTR-13 | vsireg* 与 sireg* 访问相同状态 |
| SSCTR-ENT-18 | HCROSS-SSCTR-14 | VU-mode 执行 SCTRCLR 触发异常 |
| SSCTR-EXT-04 | HCROSS-SSCTR-15 | VS→HS 外部陷阱需 STE |
| SSCTR-EXT-05 | HCROSS-SSCTR-16 | VS→HS 外部陷阱 STE=0 时不录制 |
| SSCTR-EXT-06 | HCROSS-SSCTR-17 | VU→VS 外部陷阱需 vsctrctl.STE |
| SSCTR-EXT-07 | HCROSS-SSCTR-18 | VU→VS 外部陷阱 vsSTE=0 时不录制 |
| SSCTR-EXT-08 | HCROSS-SSCTR-19 | VU→HS 外部陷阱需 STE + vsSTE |
| SSCTR-EXT-09 | HCROSS-SSCTR-20 | VU→HS 缺少 vsSTE 时不录制 |
| SSCTR-FRZ-09 | HCROSS-SSCTR-21 | VS-mode BPFRZ 由 vsctrctl 控制 |
| SSCTR-FRZ-10 | HCROSS-SSCTR-22 | VS-mode BPFRZ=0 不设置 FROZEN |
| SSCTR-FRZ-11 | HCROSS-SSCTR-23 | VS-mode LCOFIFRZ 由 vsctrctl 控制 |
| SSCTR-FRZ-12 | HCROSS-SSCTR-24 | 虚拟 LCOFI 也触发 freeze |
| SSCTR-VIRT-01 | HCROSS-SSCTR-25 | VU→HS 转换使用 vsctrctl 过滤位 |
| SSCTR-VIRT-02 | HCROSS-SSCTR-26 | VS→HS 转换使用 vsctrctl/sctrctl 字段 |
| SSCTR-VIRT-03 | HCROSS-SSCTR-27 | HS→VS/VU trap return 使用 sctrctl |
| SSCTR-VIRT-04 | HCROSS-SSCTR-28 | Freeze 使用 sctrctl（陷阱到 HS-mode） |
| SSCTR-VIRT-05 | HCROSS-SSCTR-29 | VS-mode freeze 使用 vsctrctl |
| SSCTR-SEA-02 | HCROSS-SSCTR-30 | VS-mode 检测 CTR 可访问性 |
| SSCTR-SEA-06 | HCROSS-SSCTR-31 | hstateen0.CTR=0 时 VS-mode sctrstatus 访问 |
| SSCTR-SEA-07 | HCROSS-SSCTR-32 | hstateen0.CTR=0 时 VS-mode SCTRCLR 执行 |

### 测试用例清单

#### 14.1 vsctrctl CSR（VS-mode CTR 控制寄存器）

**规范依据**：`norm:Ssctr_vsctrctl_sz_acc_op`、`norm:vsctr-s_op`、`norm:vsctrctl-u_op`、`norm:vsctrctl-ste_op`、`norm:vsctrctl-bpfrz_op`、`norm:vsctrctl-lcofifrz_op`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCTR-01 | vsctrctl 基本读写（HS-mode） | HS-mode 通过 vsctrctl 地址写各字段并读回 | 已实现字段读回一致 | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-02 | V=1 时 sctrctl 实际访问 vsctrctl | HS-mode 写 vsctrctl 特定值，切入 VS-mode 用 csrr sctrctl 读 | 读到 vsctrctl 的值 | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-03 | V=1 时写 sctrctl 实际写 vsctrctl | VS-mode 用 csrw sctrctl 写值，返回 HS-mode 用 csrr vsctrctl 读 | vsctrctl 被修改 | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-04 | vsctrctl.S 字段（VS-mode 录制） | vsctrctl.S=1，VS-mode 执行控制转换 | 转换被录制 | `norm:vsctr-s_op` |
| HCROSS-SSCTR-05 | vsctrctl.U 字段（VU-mode 录制） | vsctrctl.U=1，VU-mode 执行控制转换 | 转换被录制 | `norm:vsctrctl-u_op` |
| HCROSS-SSCTR-06 | vsctrctl.STE 字段 | 写 vsctrctl.STE=1 后读回 | 若实现 H 扩展，STE 可写 | `norm:vsctrctl-ste_op` |
| HCROSS-SSCTR-07 | vsctrctl.BPFRZ 字段 | 写 vsctrctl.BPFRZ=1 后读回 | BPFRZ 必须实现且可写 | `norm:vsctrctl-bpfrz_op` |
| HCROSS-SSCTR-08 | vsctrctl.LCOFIFRZ 字段 | 写 vsctrctl.LCOFIFRZ=1 后读回 | 若实现 Sscofpmf，LCOFIFRZ 必须可写 | `norm:vsctrctl-lcofifrz_op` |
| HCROSS-SSCTR-09 | V=0 时 vsctrctl 不影响行为 | HS-mode 写 vsctrctl.S=1，HS-mode 自身执行转换 | HS-mode 转换不受 vsctrctl 影响 | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-10 | vsctrctl 字段与 sctrctl 匹配 | 检查 vsctrctl 可选字段是否与 sctrctl 一致 | vsctrctl 中实现的可选字段应与 sctrctl 匹配 | `norm:Ssctr_vsctrctl_sz_acc_op` |

#### 14.2 VS/VU-mode 对 CTR CSR 的访问限制

**规范依据**：`norm:sctrdepth_mode`、`norm:sctrclr_exceptions`、`norm:vsiselect_op`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCTR-11 | VS-mode 访问 sctrdepth 触发异常 | VS-mode 尝试访问 sctrdepth | 触发 virtual-instruction 异常（cause=22） | `norm:sctrdepth_mode` |
| HCROSS-SSCTR-12 | VU-mode 访问 sctrdepth 触发异常 | VU-mode 尝试访问 sctrdepth | 触发 virtual-instruction 异常（cause=22） | `norm:sctrdepth_mode` |
| HCROSS-SSCTR-13 | vsireg* 与 sireg* 访问相同状态 | HS-mode 设 vsiselect=0x200，通过 vsireg 写入值；再设 siselect=0x200，通过 sireg 读 | 读到相同的值（共享 entry 寄存器状态） | `norm:vsiselect_op` |
| HCROSS-SSCTR-14 | VU-mode 执行 SCTRCLR 触发异常 | VU-mode 执行 SCTRCLR | 触发 virtual-instruction 异常（cause=22） | `norm:sctrclr_exceptions` |

#### 14.3 VS/VU-mode 外部陷阱录制

**规范依据**：`norm:exttrap_vshs`、`norm:exttrap_vuhs`、`norm:exttrap_vuvs`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCTR-15 | VS→HS 外部陷阱需 STE | sctrctl.S=0, vsctrctl.S=1, sctrctl.STE=1，VS-mode 触发陷阱到 HS-mode | 外部陷阱被录制 | `norm:exttrap_vshs` |
| HCROSS-SSCTR-16 | VS→HS 外部陷阱 STE=0 时不录制 | sctrctl.S=0, vsctrctl.S=1, sctrctl.STE=0，VS-mode 触发陷阱到 HS-mode | 外部陷阱不被录制 | `norm:exttrap_vshs` |
| HCROSS-SSCTR-17 | VU→VS 外部陷阱需 vsctrctl.STE | vsctrctl.S=0, vsctrctl.U=1, vsctrctl.STE=1，VU-mode 触发陷阱到 VS-mode | 外部陷阱被录制 | `norm:exttrap_vuvs` |
| HCROSS-SSCTR-18 | VU→VS 外部陷阱 vsSTE=0 时不录制 | vsctrctl.S=0, vsctrctl.U=1, vsctrctl.STE=0，VU-mode 触发陷阱到 VS-mode | 外部陷阱不被录制 | `norm:exttrap_vuvs` |
| HCROSS-SSCTR-19 | VU→HS 外部陷阱需 STE + vsSTE | sctrctl.S=0, sctrctl.STE=1, vsctrctl.U=1, vsctrctl.STE=1，VU-mode 触发陷阱到 HS-mode | 外部陷阱被录制 | `norm:exttrap_vuhs` |
| HCROSS-SSCTR-20 | VU→HS 缺少 vsSTE 时不录制 | sctrctl.S=0, sctrctl.STE=1, vsctrctl.U=1, vsctrctl.STE=0，VU-mode 触发陷阱到 HS-mode | 外部陷阱不被录制 | `norm:exttrap_vuhs` |

#### 14.4 VS-mode Freeze 行为（vsctrctl 控制）

**规范依据**：`norm:ctr_freeze_vs`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCTR-21 | VS-mode BPFRZ 由 vsctrctl 控制 | vsctrctl.BPFRZ=1，VS-mode 执行 EBREAK 陷阱到 VS-mode | sctrstatus.FROZEN=1 | `norm:ctr_freeze_vs` |
| HCROSS-SSCTR-22 | VS-mode BPFRZ=0 不设置 FROZEN | vsctrctl.BPFRZ=0，VS-mode 执行 EBREAK | sctrstatus.FROZEN 不变 | `norm:ctr_freeze_vs` |
| HCROSS-SSCTR-23 | VS-mode LCOFIFRZ 由 vsctrctl 控制 | vsctrctl.LCOFIFRZ=1，LCOFI 陷阱到 VS-mode | sctrstatus.FROZEN=1 | `norm:ctr_freeze_vs` |
| HCROSS-SSCTR-24 | 虚拟 LCOFI 也触发 freeze | vsctrctl.LCOFIFRZ=1，hypervisor 挂起虚拟 LCOFI，陷阱到 VS-mode | sctrstatus.FROZEN=1 | `norm:ctr_freeze_vs` |

#### 14.5 虚拟化模式转换配置来源

**规范依据**：虚拟化模式转换使用 vsctrctl 中的字段（除 M/MTE/LCOFIFRZ/BPFRZ 由 sctrctl/mctrctl 控制外）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCTR-25 | VU→HS 转换使用 vsctrctl 过滤位 | vsctrctl.U=1, sctrctl.S=1，vsctrctl 中设置某过滤位，VU-mode 触发陷阱到 HS-mode | 转换是否被录制由 vsctrctl 中的过滤位决定（而非 sctrctl） | `norm:vsctr-s_op` |
| HCROSS-SSCTR-26 | VS→HS 转换使用 vsctrctl/sctrctl 字段 | vsctrctl.S=1, sctrctl.S=1，VS-mode 触发陷阱到 HS-mode | 源模式启用由 vsctrctl.S 决定，目标模式启用由 sctrctl.S 决定 | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-27 | HS→VS/VU trap return 使用 sctrctl | sctrctl.S=1, vsctrctl.S=1，HS-mode 执行 SRET 返回 VS-mode | 源模式启用由 sctrctl.S 决定，目标模式由 vsctrctl.S 决定 | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-28 | Freeze 使用 sctrctl（陷阱到 HS-mode） | sctrctl.BPFRZ=1，VS-mode breakpoint 陷阱到 HS-mode | freeze 由 sctrctl.BPFRZ 决定 | `norm:ctr_freeze_bp` |
| HCROSS-SSCTR-29 | VS-mode freeze 使用 vsctrctl | vsctrctl.BPFRZ=1，VS-mode breakpoint 陷阱到 VS-mode | freeze 由 vsctrctl.BPFRZ 决定 | `norm:ctr_freeze_vs` |

#### 14.6 hstateen0.CTR 对 VS-mode CTR 访问的控制

**规范依据**：`norm:hstateen_ctr`、`norm:hstateen_vs`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCTR-30 | VS-mode 检测 CTR 可访问性 | VS-mode 尝试访问 sctrctl，观察是否触发异常 | 若 hstateen0.CTR=1 则访问成功；若 CTR=0 则触发 virtual-instruction | `norm:hstateen_vs` |
| HCROSS-SSCTR-31 | hstateen0.CTR=0 时 VS-mode sctrstatus 访问 | hstateen0.CTR=0，VS-mode 访问 sctrstatus | 触发 virtual-instruction | `norm:hstateen_vs` |
| HCROSS-SSCTR-32 | hstateen0.CTR=0 时 VS-mode SCTRCLR 执行 | hstateen0.CTR=0，VS-mode 执行 SCTRCLR | 触发 virtual-instruction | `norm:hstateen_vs` |

### 代码示例

```c
/* HCROSS-SSCTR-02: V=1 sctrctl substitutes for vsctrctl */
TEST_REGISTER(test_hcross_ssctr_02);
bool test_hcross_ssctr_02(void) {
    TEST_BEGIN("HCROSS-SSCTR-02: V=1 sctrctl accesses vsctrctl");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssctr_extension()) TEST_SKIP("Ssctr not available");

    /* HS-mode: write a specific value to vsctrctl */
    uintptr_t test_val = VSCTRCTL_S | VSCTRCTL_U;
    CSRW(vsctrctl, test_val);

    /* Switch to VS-mode and read sctrctl */
    uintptr_t vs_val = 0;
    run_in_vs_mode(_vs_read_sctrctl, &vs_val);

    TEST_ASSERT("VS-mode sctrctl reads vsctrctl value",
                vs_val == test_val);

    HYP_TEST_END();
}

/* HCROSS-SSCTR-17: VU->VS external trap requires vsctrctl.STE */
TEST_REGISTER(test_hcross_ssctr_17);
bool test_hcross_ssctr_17(void) {
    TEST_BEGIN("HCROSS-SSCTR-17: VU->VS ext trap needs vsctrctl.STE");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssctr_extension()) TEST_SKIP("Ssctr not available");

    /* Configure: vsctrctl.S=0 (VS disabled), vsctrctl.U=1 (VU enabled)
     * vsctrctl.STE=1 (enable external trap recording to VS-mode) */
    CSRW(vsctrctl, VSCTRCTL_U | VSCTRCTL_STE);

    /* Clear CTR entries */
    sctrclr();

    /* VU-mode triggers trap to VS-mode (ecall delegated to VS) */
    hedeleg_set(1UL << 8);  /* delegate ecall-from-VU to VS */
    run_in_vu_mode(_vu_ecall_wrapper);

    /* Check entry 0: external trap should be recorded */
    uintptr_t ctrsource0 = read_entry_ctrsource(0);
    TEST_ASSERT("external trap VU->VS recorded",
                (ctrsource0 & CTRSOURCE_V) != 0);

    HYP_TEST_END();
}

/* HCROSS-SSCTR-21: VS-mode BPFRZ controlled by vsctrctl */
TEST_REGISTER(test_hcross_ssctr_21);
bool test_hcross_ssctr_21(void) {
    TEST_BEGIN("HCROSS-SSCTR-21: VS-mode BPFRZ from vsctrctl");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");
    if (!check_ssctr_extension()) TEST_SKIP("Ssctr not available");

    /* Set vsctrctl.BPFRZ=1 */
    CSRS(vsctrctl, VSCTRCTL_BPFRZ);

    /* Clear FROZEN first */
    CSRC(sctrstatus, SCTRSTATUS_FROZEN);

    /* VS-mode executes EBREAK, traps to VS-mode */
    run_in_vs_mode(_vs_ebreak_wrapper);

    /* Check sctrstatus.FROZEN was set */
    uintptr_t status = CSRR(sctrstatus);
    TEST_ASSERT("sctrstatus.FROZEN=1 after VS-mode breakpoint",
                (status & SCTRSTATUS_FROZEN) != 0);

    HYP_TEST_END();
}
```

> [!NOTE]
> - 本组测试验证 Ssctr 扩展在 Hypervisor 场景下的行为。所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展的可用性，不可用时 TEST_SKIP。
> - HCROSS-SSCTR-01~10 从 `Ssctr_test_plan.md` Group 2 迁移而来，验证 `vsctrctl` CSR 的基本功能。`vsctrctl` 是 H 扩展引入的 CSR，V=1 时替代 `sctrctl`，V=0 时不影响行为。
> - HCROSS-SSCTR-11~14 从 `Ssctr_test_plan.md` Groups 3/5 迁移而来，验证 VS/VU-mode 对 CTR CSR 的访问限制。VS/VU-mode 访问 `sctrdepth` 和执行 SCTRCLR 触发 virtual-instruction（cause=22）。HCROSS-SSCTR-13 验证 `vsireg*` 与 `sireg*` 共享同一组 CTR entry 寄存器状态（V=1 时没有单独的 entry 寄存器集）。
> - HCROSS-SSCTR-15~20 从 `Ssctr_test_plan.md` Group 8 迁移而来，验证 VS/VU-mode 外部陷阱录制。外部陷阱录制依赖中间模式的 TE 位：VS→HS 需要 sctrctl.STE；VU→VS 需要 vsctrctl.STE；VU→HS 需要 sctrctl.STE + vsctrctl.STE（两个 TE 位都必须置位）。
> - HCROSS-SSCTR-21~24 从 `Ssctr_test_plan.md` Group 12 迁移而来，验证 VS-mode 的 Freeze 行为由 `vsctrctl` 中的 BPFRZ/LCOFIFRZ 控制（而非 `sctrctl`）。注意：当陷阱到 HS-mode 时，freeze 由 `sctrctl.BPFRZ` 决定（HCROSS-SSCTR-28）。
> - HCROSS-SSCTR-25~29 从 `Ssctr_test_plan.md` Group 13 迁移而来，验证虚拟化模式转换时的配置来源选择。核心规则：源模式启用由该模式对应的 xctrctl 决定（VS-mode 用 vsctrctl，HS-mode 用 sctrctl）；freeze 由陷阱目标模式的 xctrctl 决定（陷阱到 VS-mode 用 vsctrctl，陷阱到 HS-mode 用 sctrctl）。
> - HCROSS-SSCTR-30~32 从 `Ssctr_test_plan.md` Group 14 迁移而来，验证 `hstateen0.CTR` 对 VS-mode CTR 访问的控制。`hstateen0.CTR=0` 时 VS-mode 访问 CTR 状态触发 virtual-instruction（cause=22），而非 illegal-instruction。
> - 与 Group 13 (HCROSS-SMCTR) 的区别：Group 13 从 M-mode 角度验证 `mstateen0.CTR` 和 `hstateen0.CTR` 对下级特权级的控制，以及 MTE 外部陷阱到 M-mode 的录制；本组从 S-mode/VS-mode 角度验证 `vsctrctl` 功能和 VS/VU-mode 的 CTR 行为。

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 5 (Svinval) | HCROSS-SINVAL-01~14 | HINVAL 指令功能和 virtual-instruction 异常是 Hypervisor 安全隔离的核心，现有计划覆盖严重不足 |
| P0（必须） | Group 11.2 (Virtual-inst) | HCROSS-SSCSRIND-11~21 | virtual-instruction 异常是虚拟化安全隔离的核心保证 |
| P0（必须） | Group 12.2 (vsstatus.SDT) | HCROSS-SSDBLTRP-07~12 | VS-mode double-trap 是虚拟化场景的关键安全机制 |
| P1（重要） | Group 1 (Svadu) | HCROSS-SVADU-01~06 | henvcfg.ADUE 可写性和 HLV/HSV 交互是 Svadu 在虚拟化场景下的关键行为 |
| P1（重要） | Group 2 (Sstvala) | HCROSS-SSTVALA-01~08 | stval/vstval 精确写入是 guest OS 调试和异常处理的基础 |
| P1（重要） | Group 8 (Smstateen) | HCROSS-SMSTA-01~14 | hstateen 控制和 VS/VU-mode 异常行为是 Hypervisor 状态隔离的关键保证 |
| P1（重要） | Group 10 (Smcsrind) | HCROSS-SMCSRIND-01~08 | mstateen0[60] 对虚拟化 CSR（vsiselect/vsireg*）的访问控制是安全隔离的关键保证 |
| P1（重要） | Group 11.3 (State-Enable) | HCROSS-SSCSRIND-22~27 | hstateen0[60] 访问控制是安全隔离的关键 |
| P1（重要） | Group 11.1 (VS CSR) | HCROSS-SSCSRIND-01~10 | vsiselect/vsireg* 是 H 扩展场景的基础 |
| P1（重要） | Group 12.1 (henvcfg.DTE) | HCROSS-SSDBLTRP-01~06 | henvcfg.DTE 是 VS-mode double-trap 的使能控制 |
| P1（重要） | Group 12.3 (SRET vsstatus.SDT) | HCROSS-SSDBLTRP-13~16 | SRET 对 vsstatus.SDT 的清除是 VS-mode trap 返回的关键路径 |
| P1（重要） | Group 12.5 (XRET Hyp) | HCROSS-SSDBLTRP-19~24 | MRET/SRET/MNRET 在虚拟化场景下的跨模式 SDT 清除 |
| P1（重要） | Group 13.2 (hstateen0.CTR) | HCROSS-SMCTR-02~09 | hstateen0.CTR 对 VS-mode CTR 访问的控制是虚拟化状态隔离的保证 |
| P1（重要） | Group 14.1 (vsctrctl) | HCROSS-SSCTR-01~10 | vsctrctl 是 VS-mode CTR 的核心控制寄存器 |
| P1（重要） | Group 14.3 (VS/VU ext traps) | HCROSS-SSCTR-15~20 | VS/VU-mode 外部陷阱录制是虚拟化场景的关键 |
| P1（重要） | Group 14.5 (Virt transitions) | HCROSS-SSCTR-25~29 | 虚拟化模式转换配置来源是 CTR 在 Hypervisor 下的正确性保证 |
| P2（建议） | Group 4 (Sscounterenw) | HCROSS-SSCOUNTERENW-01~09 | hcounteren 控制行为是性能监控隔离的保证 |
| P2（建议） | Group 9 (Sstc) | HCROSS-SSTC-01~15 | vstimecmp 和 VS-mode 定时器是 Hypervisor 虚拟化定时器的核心设施 |
| P2（建议） | Group 11.4 (Hyp 交叉) | HCROSS-SSCSRIND-28~33 | 透明重映射和别名行为依赖 H 扩展 |
| P2（建议） | Group 12.4 (menvcfg.DTE Hyp) | HCROSS-SSDBLTRP-17~18 | menvcfg.DTE 对 Hypervisor CSR 的全局控制 |
| P2（建议） | Group 13.1 (mstateen0.CTR Hyp) | HCROSS-SMCTR-01 | mstateen0.CTR 对 vsctrctl 的阻止 |
| P2（建议） | Group 13.3 (MTE Hyp) | HCROSS-SMCTR-10~12 | MTE 外部陷阱在 VS/VU-mode 的录制行为 |
| P2（建议） | Group 14.2 (VS/VU access) | HCROSS-SSCTR-11~14 | VS/VU-mode 对 CTR CSR 的访问限制 |
| P2（建议） | Group 14.4 (VS Freeze) | HCROSS-SSCTR-21~24 | VS-mode Freeze 行为由 vsctrctl 控制 |
| P2（建议） | Group 14.6 (hstateen VS) | HCROSS-SSCTR-30~32 | hstateen0.CTR 对 VS-mode CTR 访问的控制 |
| P3（可选） | Group 3 (Ssccptr) | HCROSS-SSCCPTR-01~04 | PMA 层面的约束在仿真器上难以直接验证，主要依赖平台保证 |
| P3（可选） | Group 6 (Svnapot) | HCROSS-SVNAPOT-01~03 | G-stage NAPOT 翻译依赖 H 扩展和 Svnapot 扩展同时可用，条件性实现 |
| P3（可选） | Group 7 (Svpbmt) | HCROSS-SVPBMT-01~04 | 两阶段 PBMT 覆盖依赖 H 扩展和 Svpbmt 扩展同时可用，条件性实现 |

---

## 测试实现说明

### 文件组织

```
damo-priv-test/
├── hypervisor_cross/
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_hcross_svadu.c      # Group 1: Hypervisor × Svadu
│       ├── test_hcross_sstvala.c    # Group 2: Hypervisor × Sstvala
│       ├── test_hcross_ssccptr.c    # Group 3: Hypervisor × Ssccptr
│       ├── test_hcross_sscounterenw.c # Group 4: Hypervisor × Sscounterenw
│       ├── test_hcross_svinval.c    # Group 5: Hypervisor × Svinval
│       ├── test_hcross_svnapot.c    # Group 6: Hypervisor × Svnapot
│       ├── test_hcross_svpbmt.c     # Group 7: Hypervisor × Svpbmt
│       ├── test_hcross_ssdbltrp.c   # Group 12: Hypervisor × Ssdbltrp
│       ├── test_hcross_smctr.c      # Group 13: Hypervisor × Smctr
│       └── test_hcross_ssctr.c      # Group 14: Hypervisor × Ssctr
│
├── Hypervisor_Smstateen/            # Group 8: Hypervisor × Smstateen (独立项目)
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_helpers.h
│       ├── test_register.c
│       ├── test_hcross_smstateen_init.c
│       ├── test_hcross_smstateen_prop.c
│       ├── test_hcross_smstateen_bit63.c
│       ├── test_hcross_smstateen_func_bits.c
│       └── test_hcross_smstateen_exception.c
│
├── Hypervisor_Sstc/                  # Group 9: Hypervisor × Sstc (独立项目)
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_helpers.h
│       ├── test_register.c
│       ├── sstc_strap.S
│       ├── test_hcross_sstc_stce.c   # 9.1: henvcfg.STCE 字段控制
│       ├── test_hcross_sstc_acc.c    # 9.2: VS-mode 访问控制
│       ├── test_hcross_sstc_vs.c     # 9.3-9.5: vstimecmp / VSTIP / VS-mode 定时器
│
├── Hypervisor_Sscsrind/              # Group 11: Hypervisor × Sscsrind (独立项目)
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_helpers.h
│       ├── test_register.c
│       ├── test_hcross_sscsrind_vscsr.c     # 11.1: VS-level CSR 基本功能 (HCROSS-SSCSRIND-01~10)
│       ├── test_hcross_sscsrind_vi.c        # 11.2: Virtual-Instruction 异常 (HCROSS-SSCSRIND-11~21)
│       ├── test_hcross_sscsrind_stateen.c   # 11.3: State-Enable 访问控制 (HCROSS-SSCSRIND-22~27)
│       └── test_hcross_sscsrind_hyp.c       # 11.4: Hypervisor 交叉测试 (HCROSS-SSCSRIND-28~33)
│
├── hypervisor_cross/                 # Group 10: Hypervisor × Smcsrind (可合并到主 hypervisor_cross 项目)
│   └── tests/
│       └── test_hcross_smcsrind.c    # 10: mstateen0[60] 对 vsiselect/vsireg* 的控制
│
└── common/hyp/                      # 复用现有 Hypervisor 测试框架
```

### 运行时检测

所有测试应在启动时检测所需扩展的可用性：

```c
static bool check_h_extension(void) {
    uint64_t misa = CSRR(misa);
    if (!(misa & (1UL << ('H' - 'A')))) {
        TEST_SKIP("H extension not available");
        return false;
    }
    return true;
}

static bool check_svadu_extension(void) {
    // 通过尝试写 henvcfg.ADUE 并读回来探测
    // 若只读零，则未实现 Svadu
    uint64_t old = CSRR(henvcfg);
    CSRW(henvcfg, old | HENVCFG_ADUE);
    uint64_t new = CSRR(henvcfg);
    CSRW(henvcfg, old);  // 恢复
    return (new & HENVCFG_ADUE) != 0;
}

static bool check_svinval_extension(void) {
    // 通过 misa 或平台配置探测
    // 若未实现 Svinval，HINVAL 指令触发 illegal-instruction
    return platform_has_svinval();
}
```

### 通用测试模式

#### 模式 1：Svadu 交叉测试（Group 1）

```c
TEST_REGISTER(test_hcross_svadu_01);
bool test_hcross_svadu_01(void) {
    TEST_BEGIN("HCROSS-SVADU-01: henvcfg.ADUE writability");

    if (!check_h_extension()) TEST_SKIP("H extension not available");

    uint64_t old_henvcfg = CSRR(henvcfg);

    // 尝试写 ADUE=1
    CSRW(henvcfg, old_henvcfg | HENVCFG_ADUE);
    uint64_t readback_1 = CSRR(henvcfg);

    // 尝试写 ADUE=0
    CSRW(henvcfg, old_henvcfg & ~HENVCFG_ADUE);
    uint64_t readback_0 = CSRR(henvcfg);

    // 恢复
    CSRW(henvcfg, old_henvcfg);

    if (platform_has_svadu()) {
        TEST_ASSERT("ADUE=1 writable", (readback_1 & HENVCFG_ADUE) != 0);
        TEST_ASSERT("ADUE=0 writable", (readback_0 & HENVCFG_ADUE) == 0);
    } else {
        TEST_ASSERT("ADUE read-only zero without Svadu",
                    (readback_1 & HENVCFG_ADUE) == 0);
    }

    HYP_TEST_END();
}
```

#### 模式 2：Sstvala 交叉测试（Group 2）

```c
TEST_REGISTER(test_hcross_sstvala_01);
bool test_hcross_sstvala_01(void) {
    TEST_BEGIN("HCROSS-SSTVALA-01: inst guest-page-fault stval precision");

    if (!check_h_extension()) TEST_SKIP("H extension not available");

    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, /*vs=Bare*/0, HGATP_MODE_SV39X4);

    // 代码区映射
    uintptr_t code_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    gpt_setup_identity_mapping(&ctx.g_ctx, code_base, PAGE_SIZE_1G,
                               PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D,
                               PT_LEVEL_1G);

    // 目标 GPA 未映射
    uintptr_t target_gpa = 0x80000000UL + 0x20000000UL;

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, test_vs_jump_to, target_gpa);
    TEST_ASSERT("inst guest-page-fault triggered", trap_was_triggered());
    TEST_ASSERT_EQ("cause is inst guest-page-fault",
                   trap_get_cause(), CAUSE_INST_GUEST_PAGE_FAULT);
    TEST_ASSERT_EQ("stval == faulting GVA",
                   trap_get_stval(), target_gpa);
    trap_expect_end();

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

#### 模式 3：Svinval 交叉测试（Group 5）

```c
TEST_REGISTER(test_hcross_svinval_01);
bool test_hcross_svinval_01(void) {
    TEST_BEGIN("HCROSS-SINVAL-01: HINVAL.VVMA basic functionality");

    if (!check_h_extension()) TEST_SKIP("H extension not available");
    if (!check_svinval_extension()) TEST_SKIP("Svinval not available");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    // 建立初始映射
    uintptr_t test_va = TEST_REGION_BASE;
    uintptr_t test_gpa = test_va;
    uintptr_t test_spa = test_gpa;
    two_stage_map_vs_to_gpa(&ctx, test_va, test_gpa,
                            PTE_V|PTE_R|PTE_W|PTE_A|PTE_D, PT_LEVEL_4K);
    two_stage_map_gpa_to_spa(&ctx, test_gpa, test_spa,
                             PTE_V|PTE_R|PTE_W|PTE_U|PTE_A|PTE_D, PT_LEVEL_4K);

    // VS-mode 写入初始值
    two_stage_run_in_vs(&ctx, test_vs_store, test_va, 0x1111);

    // HS-mode 修改 VS-stage PTE（改为只读）
    two_stage_modify_vs_pte(&ctx, test_va,
                            PTE_V|PTE_R|PTE_A|PTE_D, PT_LEVEL_4K);

    // 执行 HINVAL.VVMA 刷新 TLB
    hinval_vvma(test_va, /*asid=*/0);
    sfence_w_inval();
    sfence_inval_ir();

    // VS-mode 验证新 PTE 生效（store 应触发 page-fault）
    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_store_expect_fault,
                                           test_va, 0x2222);
    TEST_ASSERT("store page-fault after HINVAL.VVMA",
                result == CAUSE_STORE_PAGE_FAULT);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

### 关键注意事项

1. **扩展检测**：所有测试必须在运行时检测所需扩展（H、Svadu、Svinval 等）的可用性，不可用时 TEST_SKIP。

2. **Svadu 与 Svade 的互斥**：`henvcfg.ADUE=0` 时行为如同 Svade（A/D=0 触发 fault），`ADUE=1` 时启用硬件 A/D 更新。测试时需明确当前 ADUE 状态。

3. **Sstvala 的精确性要求**：Sstvala 扩展强制要求 `stval` 写入 faulting 地址，而非 0。测试断言必须使用 `TEST_ASSERT_EQ` 精确比较，不能用 `TEST_ASSERT(stval != 0)` 模糊验证。

4. **Ssccptr 的平台依赖**：PMA 是平台硬连线属性，在 QEMU/Spike 上主存默认满足 cacheability+coherence。HCROSS-SSCCPTR-04 需要平台支持动态 PMA 配置，大多数平台不支持。

5. **Sscounterenw 的探测逻辑**：测试前需探测哪些 `hpmcounter` 非只读零，仅对这些计数器验证 `hcounteren` 对应位的可写性。

6. **Svinval 指令编码**：
   - `hinval.vvma rs1, rs2`：`0011011 rs2 rs1 000 00000 1110011`
   - `hinval.gvma rs1, rs2`：`0111011 rs2 rs1 000 00000 1110011`
   - `sfence.w.inval`：`0001100 00000 00000 000 00000 1110011`
   - `sfence.inval.ir`：`0001100 00001 00000 000 00000 1110011`

7. **HINVAL.GVMA 的 VMID 语义**：`rs2` 寄存器指定 VMID（而非 ASID）。VMID=0 时刷新所有 VMID 的 TLB，VMID 非零时仅刷新特定 VMID。

8. **virtual-instruction 与 illegal-instruction 的区分**：VS/VU-mode 执行 HINVAL 触发 virtual-instruction (cause=22)，而非 illegal-instruction (cause=2)。测试断言必须使用准确的 cause 常量。

---

## Group 8: Hypervisor × Smstateen

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
| HCROSS-SMSTA-01 | hstateen0 复位后初始化 | M-mode 设置 mstateen0 某些位为 1 后，写零到 hstateen0 | hstateen0 读回为零 | `norm:hstateen_sstateen_zero_initialization` |
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

### 代码示例

```c
/* HCROSS-SMSTA-01: hstateen0 复位后初始化 */
TEST_REGISTER(test_hcross_smsta_01);
bool test_hcross_smsta_01(void) {
    TEST_BEGIN("HCROSS-SMSTA-01: hstateen0 zeroed after mstateen0 modification");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    /* Set mstateen0.SE0 to allow hstateen0 access, plus some bits */
    uintptr_t orig = mstateen0_read();
    mstateen0_write(MSTATEEN0_SE0 | MSTATEEN0_C);

    /* Write zero to hstateen0 */
    trap_expect_begin();
    hstateen0_write(0);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    if (trapped) {
        TEST_SKIP("hstateen0 not accessible");
    }

    uintptr_t val = hstateen0_read();
    TEST_ASSERT_EQ("hstateen0 reads zero", val, 0);

    mstateen0_write(orig);
    HYP_TEST_END();
}

/* HCROSS-SMSTA-02: mstateen0 零位传播到 hstateen0 */
TEST_REGISTER(test_hcross_smsta_02);
bool test_hcross_smsta_02(void) {
    TEST_BEGIN("HCROSS-SMSTA-02: mstateen0 zero bit propagates to hstateen0");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig = mstateen0_read();

    /* Enable hstateen0 access (SE0=1) but clear C bit */
    mstateen0_write(MSTATEEN0_SE0);

    /* From M-mode, try writing C bit to hstateen0 */
    hstateen0_write(MSTATEEN0_C);
    uintptr_t val = hstateen0_read();

    TEST_ASSERT_BITS("hstateen0.C is RO0 when mstateen0.C=0",
                     val, MSTATEEN0_C, 0);

    mstateen0_write(orig);
    HYP_TEST_END();
}

/* HCROSS-SMSTA-03: SE0=0 阻止 HS-mode hstateen0 */
TEST_REGISTER(test_hcross_smsta_03);
bool test_hcross_smsta_03(void) {
    TEST_BEGIN("HCROSS-SMSTA-03: mstateen0.SE0=0 blocks HS-mode hstateen0");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig = mstateen0_read();

    /* Clear SE0 (bit 63) */
    mstateen0_clear(MSTATEEN0_SE0);

    /* HS-mode access to hstateen0 should trigger illegal-instruction.
     * Since we are in M-mode and HS-mode uses the same trap path,
     * we test by switching to S-mode (which acts as HS when H ext
     * is present and V=0). */
    SMSTATEEN_TEST_SMODE_BLOCKED(
        "HS-mode hstateen0 read blocked (SE0=0)",
        hstateen0_read());

    mstateen0_write(orig);
    HYP_TEST_END();
}

/* HCROSS-SMSTA-13: VS-mode virtual-instruction */
TEST_REGISTER(test_hcross_smsta_13);
bool test_hcross_smsta_13(void) {
    TEST_BEGIN("HCROSS-SMSTA-13: VS-mode access -> virtual-instruction");

    if (!HAS_H_EXT()) TEST_SKIP("H extension not available");

    uintptr_t orig = mstateen0_read();

    /* Set up: mstateen0.SE0=0 to block sstateen0 access.
     * From VS-mode, accessing sstateen0 (which appears as sstateen0
     * to the guest) should trigger virtual-instruction exception
     * (cause=22) rather than illegal-instruction, because hstateen
     * would allow it but mstateen blocks it. */

    /* First set hstateen0 bit 63 = 1 (allow from H perspective),
     * then set mstateen0 SE0=1 temporarily to write hstateen0 */
    mstateen0_set(MSTATEEN0_SE0);
    hstateen0_write(1ULL << 63);

    /* Now clear mstateen0 SE0 to block at M-level */
    mstateen0_clear(MSTATEEN0_SE0);

    /* VS-mode access should see virtual-instruction (cause=22) */
#ifdef ENABLE_HYP
    goto_priv(PRIV_VS);
    PRIV_DO(sstateen0_read());
    goto_priv(PRIV_M);

    /* Check for virtual-instruction exception */
    if (trap_was_triggered()) {
        uintptr_t cause = trap_get_cause();
        /* Accept either virtual-instruction(22) or illegal-instruction(2)
         * depending on implementation detail of privilege routing */
        TEST_ASSERT("VS-mode trap cause is virtual-inst or illegal-inst",
                    cause == CAUSE_VIRTUAL_INSTRUCTION ||
                    cause == CAUSE_ILLEGAL_INST);
    } else {
        TEST_ASSERT("VS-mode trap should have triggered", false);
    }
#else
    TEST_SKIP("ENABLE_HYP not compiled");
#endif

    mstateen0_write(orig);
    HYP_TEST_END();
}
```

### 关键注意事项

1. **H 扩展检测**：所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展的可用性，不可用时 TEST_SKIP。

2. **HS-mode 与 S-mode 的关系**：在 H 扩展存在且 V=0 时，S-mode 实际上就是 HS-mode。测试中使用 `goto_priv(PRIV_S)` 来模拟 HS-mode 访问。

3. **VS-mode/VU-mode 测试**：需要编译时启用 `ENABLE_HYP` 宏，并使用 `goto_priv(PRIV_VS)` 或 `goto_priv(PRIV_VU)` 切换到虚拟特权级。

4. **virtual-instruction 与 illegal-instruction 的区分**：VS/VU-mode 访问受控 CSR 时，若 hstateen 允许但 mstateen 阻止，应触发 virtual-instruction (cause=22)；若 hstateen 也阻止，则触发 illegal-instruction (cause=2)。

5. **hstateen CSR 地址**：hstateen0-3 的 CSR 地址为 0x60C-0x60F，hstateen0h-3h (RV32) 为 0x61C-0x61F。

---

## 参考

- `SPEC/hypervisor.adoc` — RISC-V Hypervisor Extension, Version 1.0
- `SPEC/svadu.adoc` — Svadu Extension
- `SPEC/sstvala.adoc` — Sstvala Extension
- `SPEC/ssccptr.adoc` — Ssccptr Extension
- `SPEC/sscounterenw.adoc` — Sscounterenw Extension
- `SPEC/svinval.adoc` — Svinval Extension
- `SPEC/svnapot.adoc` — Svnapot Extension
- `SPEC/svpbmt.adoc` — Svpbmt Extension
- `DOCS/testplan/Hypervisor_test_plan.md` — Hypervisor 基础测试计划
- `DOCS/testplan/Hypervisor_2_stage_test_plan.md` — 两阶段翻译测试计划
- `DOCS/testplan/Hypervisor_gstage_test_plan.md` — G-stage 独立测试计划
- `DOCS/testplan/svadu_test_plan.md` — Svadu 独立测试计划
- `DOCS/testplan/sstvala_test_plan.md` — Sstvala 独立测试计划
- `DOCS/testplan/ssccptr_test_plan.md` — Ssccptr 独立测试计划
- `DOCS/testplan/sscounterenw_test_plan.md` — Sscounterenw 独立测试计划
- `DOCS/testplan/svinval_test_plan.md` — Svinval 独立测试计划
- `DOCS/testplan/svnapot_test_plan.md` — Svnapot 独立测试计划
- `DOCS/testplan/svpbmt_test_plan.md` — Svpbmt 独立测试计划
- `ideas/hypervisor_gap.md` — Hypervisor 测试缺口分析
- `SPEC/smstateen.adoc` — Smstateen Extension Specification
- `DOCS/testplan/ssstateen_test_plan.md` — Ssstateen 独立测试计划
- `SPEC/sstc.adoc` — Sstc Extension Specification (Supervisor-mode Timer Interrupts)
- `DOCS/testplan/sstc_test_plan.md` — Sstc 独立测试计划
- `SPEC/smcsrind.adoc` — Smcsrind/Sscsrind Extension for Indirect CSR Access
- `DOCS/testplan/Smcsrind_test_plan.md` — Smcsrind Machine Mode 测试计划
- `DOCS/testplan/Sscsrind_test_plan.md` — Sscsrind Supervisor Mode 测试计划
- `SPEC/ssdbltrp.adoc` — Ssdbltrp Double Trap Extension
- `DOCS/testplan/Ssdbltrp_test_plan.md` — Ssdbltrp 独立测试计划
- `SPEC/smctr.adoc` — Smctr (Control Transfer Records - Machine-level) Extension
- `SPEC/ssctr.adoc` — Ssctr (Control Transfer Records - Supervisor-level) Extension
- `DOCS/testplan/Smctr_test_plan.md` — Smctr Machine Mode 测试计划
- `DOCS/testplan/Ssctr_test_plan.md` — Ssctr Supervisor Mode 测试计划
