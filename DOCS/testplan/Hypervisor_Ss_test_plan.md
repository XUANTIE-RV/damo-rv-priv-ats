**中文 | [English](../testplan_en/Hypervisor_Ss_test_plan_en.md)**

# Hypervisor 与 Ss* 扩展交叉测试计划

> 本文档描述 Hypervisor（H）扩展与其他 Ss* 系列（Supervisor-level）扩展在交叉场景下的测试计划。本方案从 `Hypervisor_cross_test_plan.md` 拆分而来，仅保留 Hypervisor 与 Ss* 扩展交叉的内容。这些测试场景原本在各扩展的独立测试计划中被标记为"由 Hypervisor 测试计划覆盖"或"因缺少 H 扩展而排除"，但经分析发现现有 Hypervisor 测试计划（`Hypervisor_CSR_test_plan.md`、`Hypervisor_Interrupts_test_plan.md`、`Hypervisor_Exceptions_test_plan.md`、`Hypervisor_2_stage_test_plan.md`、`Hypervisor_gstage_test_plan.md`）并未完全覆盖。
>
> 生成时间：2026-06-22

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — Hypervisor（H）扩展：hstateen/henvcfg/hcounteren 对 VS/VU-mode 的控制、virtual-instruction 机制、VSTIP 合成
- `SPEC/riscv-isa-manual/src/priv/sstvala.adoc` — Sstvala：trap 时 stval 精确写入
- `SPEC/riscv-isa-manual/src/priv/ssccptr.adoc` — Ssccptr：页表读取的 cacheability/coherence PMA 要求
- `SPEC/riscv-isa-manual/src/priv/sscounterenw.adoc` — Sscounterenw：计数器使能位可写性
- `SPEC/riscv-isa-manual/src/priv/smstateen.adoc` — Smstateen：hstateen CSR 行为（Ssstateen 交叉部分）
- `SPEC/riscv-isa-manual/src/priv/sstc.adoc` — Sstc：vstimecmp 与 VS-mode 定时器
- `SPEC/riscv-isa-manual/src/priv/smcsrind.adoc` — Smcsrind/Sscsrind：vsiselect/vsireg* 间接 CSR 访问
- `SPEC/riscv-isa-manual/src/priv/ssdbltrp.adoc` — Ssdbltrp：henvcfg.DTE、vsstatus.SDT、VS-mode double-trap
- `SPEC/riscv-isa-manual/src/priv/smctr.adoc` — Ssctr：vsctrctl 与 VS/VU-mode 控制转换录制（Ssctr 定义与 Smctr 合卷于本文件）
- `SPEC/riscv-isa-manual/src/priv/sscofpmf.adoc` — Sscofpmf：mhpmevent VSINH/VUINH 计数抑制、VS-mode `scountovf` 双重门控
- `SPEC/riscv-isa-manual/src/priv/smcdeleg.adoc` — Smcdeleg/Ssccfg：scountovf/scountinhibit 虚拟化、hvip/hvien LCOFI 位、vsiselect/vsireg* 访问规则
- `SPEC/riscv-ssqosid/sqosid.adoc` — Ssqosid：srmcfg 与 mstateen0[55] 门控

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内 src/priv 下上述路径文件）
- https://github.com/riscv/riscv-ssqosid （对应仓库内 sqosid.adoc）

---

## 范围

### 覆盖的扩展交叉

- **Hypervisor × Sstvala**：Guest page-fault（cause 20/23）时 `stval` 精确写入行为
- **Hypervisor × Ssccptr**：两阶段页表遍历的 cacheability/coherence 验证
- **Hypervisor × Sscounterenw**：`hcounteren` 对 VS/VU-mode 计数器写入的控制
- **Hypervisor × Ssstateen**：`hstateen0-3` CSR 存在性与可访问性、`hstateen` bit 63 控制 VS-mode 对 `sstateen` 的访问、`hstateen` 只读零传播到 VS-mode、`hstateen0` 各功能位（SE0/ENVCFG/CSRIND/IMSIC/AIA/CONTEXT）对 VS-mode 状态的控制、`hstateen` 只读约束与编码一致性
- **Hypervisor × Sstc**：`henvcfg.STCE` 可写性与约束、VS-mode 对 `stimecmp`/`vstimecmp` 的访问控制、`vstimecmp` CSR 读写、VSTIP 合成逻辑、`henvcfg.STCE` 对 VS-mode 定时器的控制、VS-mode timer interrupt 捕获
- **Hypervisor × Sscsrind**：VS-level CSR（vsiselect/vsireg*）基本功能、Virtual-instruction 异常行为、`hstateen0[60]` 对 VS/VU-mode 访问的控制、VS-mode 通过 sireg* 透明访问 vsireg* 的重映射行为
- **Hypervisor × Ssdbltrp**：`henvcfg.DTE` 对 VS-mode 的使能/禁用控制、`vsstatus.SDT` 字段行为与 SDT/SIE 互斥、SRET 对 `vsstatus.SDT` 的清除、MRET/SRET/MNRET 在 Hypervisor 场景下对 SDT/vsstatus.SDT 的跨模式清除
- **Hypervisor × Ssctr**：`vsctrctl` CSR 基本功能与字段验证、VS/VU-mode 外部陷阱录制（STE/vsSTE）、虚拟化模式转换配置来源、VS-mode Freeze 行为（vsctrctl 控制）、VS-mode 对 sctrdepth/SCTRCLR 的访问限制、hstateen0.CTR 对 VS-mode CTR 访问的控制
- **Hypervisor × Ssqosid**：V=1 时 VS/VU-mode 访问 `srmcfg` 触发 virtual-instruction exception、mstateen0[55] 门控与 V=1 规则的优先级、virtual-instruction trap 时 stval/htinst 值
- **Hypervisor × Sscofpmf**：`mhpmevent` VSINH/VUINH 对 VS/VU-mode 计数的抑制、VS-mode `scountovf` 的 `mcounteren`+`hcounteren` 双重门控
- **Hypervisor × Smcdeleg/Ssccfg**：CDE=1 时 VS/VU-mode 读 `scountovf`、访问 `scountinhibit` 触发 virtual-instruction、`hvip`/`hvien` LCOFI 位（bit 13）实现与可写性、`vsiselect`/`vsireg*` 在 0x40-0x5F 范围的多特权级访问规则、hstateen0 bit 60 对 VS-mode 的控制

### 不在本文档范围

- 已由 `Hypervisor_CSR_test_plan.md`、`Hypervisor_Interrupts_test_plan.md`、`Hypervisor_Exceptions_test_plan.md`、`Hypervisor_2_stage_test_plan.md`、`Hypervisor_gstage_test_plan.md` 覆盖的 Hypervisor 基础功能
- 已由 `Shcounterenw_test_plan.md` 覆盖的 Sha 子扩展（与 Sscounterenw 是不同的扩展体系）
- 各扩展在非 Hypervisor 场景下的行为（由各自独立测试计划覆盖）
- Hypervisor 与 Sv\*/Sm\*/Z\* 扩展的交叉测试（分别由 `Hypervisor_Sv_test_plan.md`、`Hypervisor_Sm_test_plan.md`、`Hypervisor_Zi_test_plan.md` 覆盖）
- Ssdbltrp 非 Hypervisor 测试（`sstatus`.SDT 字段、S-mode double-trap、`menvcfg`.DTE 基础控制、`medeleg`[16]、`mtval2`） — 由 `Ssdbltrp_test_plan.md` 覆盖

---

## 覆盖的规范点

下表列出本方案覆盖的核心规范点。带 `norm:` 前缀的为 SPEC 官方标签；不带前缀的为根据 SPEC 原文自行拆解的规范点。各 Group 规范依据中直接引用的其余规范点（Sscsrind/Ssdbltrp/Ssctr/Ssqosid 等）亦属本方案覆盖范围，统一列入文末附录 A 覆盖矩阵。

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:H_guest_page_fault` | `hypervisor.adoc` | On a guest-page fault, `mtval` or `stval` is written with the faulting guest virtual address. | 客户页错误时 `mtval`/`stval` 写入故障客户虚拟地址。 |
| `norm:sstvala_stval_faulting_vaddr` | `sstvala.adoc` | When a page-fault is triggered by an instruction fetch, `stval` is written with the faulting virtual address (PC). | 当取指触发 page-fault 时，`stval` 写入故障虚拟地址（PC）。 |
| `norm:sstvala_stval_faulting_instruction` | `sstvala.adoc` | When a virtual-instruction exception is raised, `stval` must be written with the faulting instruction encoding. | virtual-instruction 异常时，`stval` 必须写入故障指令编码。 |
| `norm:ssccptr_memory_pte_reads` | `ssccptr.adoc` | If the Ssccptr extension is implemented, then main memory regions with both the cacheability and coherence PMAs must support hardware page-table reads. | 如果实现了 Ssccptr 扩展，则同时具有可缓存性和一致性 PMA 的主存区域必须支持硬件页表读取。 |
| `norm:sscounterenw_hpmcounter_scounteren` | `sscounterenw.adoc` | If the Sscounterenw extension is implemented, then for any `hpmcounter` that is not read-only zero, the corresponding bit in `scounteren` must be writable. | 若实现了 Sscounterenw 扩展，则对于任何非只读零的 `hpmcounter`，`scounteren` 中的对应位必须可写。 |
| `hcounteren_vs_vu_control` | `hypervisor.adoc` | The `hcounteren` CSR controls availability of performance monitoring counters to VS-mode and VU-mode. | `hcounteren` CSR 控制 VS 和 VU 模式下性能监控计数器的可用性。 |
| `norm:hstateen_rv64_csrs` | `smstateen.adoc` | When H extension is implemented, hstateen0-3 CSRs are added. | 实现 H 扩展时添加 hstateen0-3 CSR。 |
| `norm:stateen_rv32_upper_bits_csrs` | `smstateen.adoc` | RV32 provides additional hstateen0h-3h CSRs for upper 32 bits. | RV32 额外提供 hstateen0h-3h CSR 用于高 32 位。 |
| `norm:hstateen_encoding` | `smstateen.adoc` | hstateen CSRs have the same encoding as mstateen CSRs. | hstateen CSR 的编码与 mstateen CSR 一致。 |
| `norm:hstateen_bit_63_op` | `smstateen.adoc` | Bit 63 of each hstateen CSR controls whether VS-mode may access the corresponding sstateen CSR. | hstateen 的 bit 63 控制 VS-mode 对对应 sstateen CSR 的访问。 |
| `norm:hstateen_bit_63_writable` | `smstateen.adoc` | Bit 63 of each hstateen CSR is always writable (not read-only). | hstateen 的 bit 63 始终可写（不是只读）。 |
| `norm:sstateen_vsmode_access_roz` | `smstateen.adoc` | For any bit that is zero in hstateen (whether read-only zero or written to zero), the corresponding bit in sstateen appears as read-only zero when accessed from VS-mode. | hstateen 中为零的位（无论是只读零还是写为零），在 VS-mode 访问 sstateen 时表现为只读零。 |
| `norm:sstateen_ro1_bits` | `smstateen.adoc` | A bit in sstateen cannot be read-only one unless the same bit in both mstateen and hstateen (when H is implemented) is also read-only one. | sstateen 位不能为 RO1 除非 mstateen 和 hstateen（实现 H 扩展时）同位也为 RO1。 |
| `norm:hstateen_ro1_bits` | `smstateen.adoc` | A bit in hstateen cannot be read-only one unless the same bit in mstateen is also read-only one. | hstateen 位不能为 RO1 除非 mstateen 同位也为 RO1。 |
| `norm:stateen_warl_access` | `smstateen.adoc` | Each standard-defined bit in stateen CSRs is WARL (Write Any Values, Reads Legal Values). | stateen CSR 中每个标准定义位都是 WARL（可写任意值，读回合法值）。 |
| `norm:stateen_unimplemented_state_roz` | `smstateen.adoc` | Bits that control state for unimplemented extensions are read-only zero. | 控制未实现扩展状态的位为只读零。 |
| `norm:stateen_reserved_roz` | `smstateen.adoc` | Reserved bits in stateen CSRs are read-only zero. | stateen CSR 中的保留位为只读零。 |
| `norm:hstateen0_SE0_op` | `smstateen.adoc` | hstateen0.SE0 (bit 63) controls whether VS-mode may access sstateen0. | hstateen0.SE0（bit 63）控制 VS-mode 对 sstateen0 的访问。 |
| `norm:hstateen0_envcfg_op` | `smstateen.adoc` | hstateen0.ENVCFG (bit 62) controls whether VS-mode may access senvcfg. | hstateen0.ENVCFG（bit 62）控制 VS-mode 对 senvcfg 的访问。 |
| `norm:hstateen0_csrind_op` | `smstateen.adoc` | hstateen0.CSRIND (bit 60) controls whether VS-mode may access siselect and sireg* (which are actually vsiselect and vsireg*). | hstateen0.CSRIND（bit 60）控制 VS-mode 对 siselect/sireg*（实为 vsiselect/vsireg*）的访问。 |
| `norm:hstateen0_imsic_op` | `smstateen.adoc` | hstateen0.IMSIC (bit 58) controls access to guest IMSIC state and vstopei in VS-mode. | hstateen0.IMSIC（bit 58）控制 VS-mode 对 guest IMSIC 状态及 vstopei 的访问。 |
| `norm:hstateen0_aia_op` | `smstateen.adoc` | hstateen0.AIA (bit 59) controls VS-mode access to Ssaia state not covered by CSRIND or IMSIC bits. | hstateen0.AIA（bit 59）控制 VS-mode 对 Ssaia 非 CSRIND/IMSIC 的剩余状态的访问。 |
| `norm:hstateen0_context_op` | `smstateen.adoc` | hstateen0.CONTEXT (bit 57) controls whether VS-mode may access scontext. | hstateen0.CONTEXT（bit 57）控制 VS-mode 对 scontext 的访问。 |
| `norm:hcounteren_acc` | `hypervisor.adoc` | When the TM bit in `hcounteren` is clear, attempts to access the `vstimecmp` register (via `stimecmp`) while executing in VS-mode will cause a virtual-instruction exception if the same bit in `mcounteren` is set. | `hcounteren`.TM=0 时，VS-mode 经 stimecmp 访问 vstimecmp，若 `mcounteren`.TM=1 则触发 virtual-instruction 异常（HCROSS-SSTC-05）。 |
| `norm:henvcfg_stce` | `hypervisor.adoc` | henvcfg.STCE=1 enables vstimecmp; when STCE=0, VS-mode (V=1) access to stimecmp raises virtual-instruction exception. | henvcfg.STCE=1 使能 vstimecmp；STCE=0 时 V=1 下访问 stimecmp 产生 virtual-instruction 异常。 |
| `norm:vstimecmp_exist` | `sstc.adoc` | Sstc adds a new VS-level vstimecmp CSR. | Sstc 新增 VS-level vstimecmp CSR。 |
| `norm:sstc_vs_facility` | `sstc.adoc` | Sstc provides a similar timer mechanism for VS-mode via the Hypervisor extension. | Sstc 为 Hypervisor 扩展的 VS-mode 提供类似的定时器机制。 |
| `norm:hip_vstip_vstie_acc_op` | `hypervisor.adoc` | VSTIP = hvip.VSTIP OR vstimecmp timer signal. | VSTIP = hvip.VSTIP OR vstimecmp 定时器信号。 |

---

## Group 1. Hypervisor × Sstvala 交叉测试

**规范依据**：
- `norm:H_guest_page_fault`：guest-page-fault 时 `stval` 写入 faulting GVA
- `norm:sstvala_stval_faulting_vaddr`：取指 page-fault 时 `stval` 写入 faulting PC
- `norm:sstvala_stval_faulting_instruction`：virtual-instruction 异常时，`stval` 必须写入故障指令编码

**测试职责**：验证 Sstvala 扩展在 Hypervisor 场景下，`stval`/`vstval` 对各类异常的精确写入行为。地址类异常（guest-page-fault cause 20/21/23）时 `stval` 必须等于故障 GVA；指令类异常（virtual-instruction cause=22）时 `stval` 必须等于故障指令编码。现有 Hypervisor 测试计划已覆盖 cause=21（load guest-page-fault）的 stval 验证，本组补充 cause=20（instruction）、cause=23（store/AMO）的精确验证，以及 virtual-instruction 的指令编码精确验证。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSTVALA-01 | Instruction guest-page-fault (cause=20) 时 stval 精确值 | VS-mode 跳转执行一个 G-stage 未映射的 GPA 地址，触发 inst guest-page-fault trap 到 HS-mode | scause=20，stval == faulting GVA（即跳转目标地址），而非 0 或其他值 |
| HCROSS-SSTVALA-02 | Store guest-page-fault (cause=23) 时 stval 精确值 | VS-mode 对一个 G-stage 未映射的 GPA 地址执行 store，触发 store guest-page-fault trap 到 HS-mode | scause=23，stval == faulting GVA（即 store 目标地址），而非 0 |
| HCROSS-SSTVALA-03 | AMO guest-page-fault (cause=23) 时 stval 精确值 | VS-mode 对一个 G-stage 未映射的 GPA 地址执行 AMO（如 AMOADD.W），触发 guest-page-fault trap 到 HS-mode | scause=23，stval == faulting GVA（即 AMO 目标地址） |
| HCROSS-SSTVALA-04 | VS-stage inst page-fault (cause=12) 委托到 VS-mode 时 vstval 精确值 | 启用 VS-stage 翻译（SV39），配置 medeleg+hedeleg 将 inst page-fault (cause=12) 委托到 VS-mode。VS-mode 跳转执行一个 VS-stage 未映射的地址，触发 inst page-fault 委托到 VS-mode handler | vscause=12，vstval == faulting VA（跳转目标虚拟地址） |
| HCROSS-SSTVALA-05 | VS-stage load page-fault (cause=13) 委托到 VS-mode 时 vstval 精确值 | 启用 VS-stage 翻译（SV39），配置 medeleg+hedeleg 将 load page-fault (cause=13) 委托到 VS-mode。VS-mode load 一个 VS-stage 未映射的地址，触发 load page-fault 委托到 VS-mode handler | vscause=13，vstval == faulting VA（load 目标虚拟地址） |
| HCROSS-SSTVALA-06 | Virtual-instruction 异常时 stval 精确值：VS-mode 读 hstatus | VS-mode 执行 `csrrs x5, hstatus, x0`（编码 0x600022F3），触发 virtual-instruction（cause=22） | scause=22，stval == 0x600022F3（故障指令编码） |
| HCROSS-SSTVALA-07 | Virtual-instruction 异常时 stval 精确值：VS-mode 写 hgatp | VS-mode 执行 `csrrw x0, hgatp, x0`（编码 0x68001073），触发 virtual-instruction（cause=22） | scause=22，stval == 0x68001073（故障指令编码） |
| HCROSS-SSTVALA-08 | Virtual-instruction 异常时 stval 精确值：VS-mode 读 hideleg | VS-mode 执行 `csrrs x5, hideleg, x0`（编码 0x603022F3），触发 virtual-instruction（cause=22） | scause=22，stval == 0x603022F3（故障指令编码） |

> [!NOTE]
> - Sstvala 的核心语义是保证 `stval` 写入 faulting 地址（而非 0）。基础 H 扩展规范允许 `stval` 在某些场景下为 0，而 Sstvala 扩展强制要求写入精确地址。
> - 规范点对应：地址类异常（page-fault/guest-page-fault）的 stval 精确性对应 `norm:sstvala_stval_faulting_vaddr`；指令类异常（virtual-instruction）的 stval 精确性对应 `norm:sstvala_stval_faulting_instruction`。
> - HCROSS-SSTVALA-01~03 验证 guest-page-fault（G-stage fault）trap 到 HS-mode 时 `stval` 的精确性；HCROSS-SSTVALA-04~05 验证 VS-stage page-fault 通过 medeleg+hedeleg 委托到 VS-mode 时 `vstval` 的精确性。04/05 使用 VS-stage page-fault（cause 12/13）而非 guest-page-fault（cause 20/23），因为 RISC-V SPEC 规定 guest-page-fault 不能通过 hedeleg 委托到 VS-mode（hedeleg bits 20/21/23 为 read-only zero，详见 `Hypervisor_Exceptions_test_plan.md` DELEG-15/16）。
> - 与 `Hypervisor_gstage_test_plan.md` 的 GFAULT 系列用例的区别：GFAULT 系列主要验证 fault 触发和 htval 编码，本组专注于 stval/vstval 的精确值断言。
> - HCROSS-SSTVALA-06~08 验证 Sstvala 对 **指令类异常** 的精确性要求：virtual-instruction 异常（cause=22）时，`stval` 必须包含触发异常的指令编码（零扩展到 XLEN）。这与 guest-page-fault（地址类异常）的 stval 语义不同——后者要求写入故障虚拟地址。指令编码推导：`csrrs x5, 0x600, x0` = `[31:20]=0x600 [19:15]=00000 [14:12]=010 [11:7]=00101 [6:0]=1110011` = `0x600022F3`。
> - HCROSS-SSTVALA-06~08 从 `sstvala_test_plan.md` Group 6（TVAL-VI-01~03）迁移而来。需要 H 扩展（`ENABLE_HYP=1`）。

---

## Group 2. Hypervisor × Ssccptr 交叉测试

**规范依据**：
- `norm:ssccptr_memory_pte_reads`：具有 cacheability 和 coherence PMA 的主存区域必须支持硬件页表读取

**测试职责**：验证在 Hypervisor 两级翻译场景下，VS-stage 和 G-stage 页表遍历在满足 PMA 条件的主存区域中能够正确完成。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSCCPTR-01 | VS-stage 页表在 cacheable+coherent 主存中的遍历 | 在默认主存（满足 cacheability+coherence PMA）中建立 VS-stage 页表，启用两阶段翻译，VS-mode 执行 load | 页表遍历成功，load 返回正确值（硬件能够正确读取 VS-stage 页表） |
| HCROSS-SSCCPTR-02 | G-stage 页表在 cacheable+coherent 主存中的遍历 | 在默认主存中建立 G-stage 页表，启用两阶段翻译，VS-mode 执行 load | 页表遍历成功，load 返回正确值（硬件能够正确读取 G-stage 页表） |
| HCROSS-SSCCPTR-03 | 两阶段页表均在 cacheable+coherent 主存中的遍历 | VS-stage 和 G-stage 页表均分配在默认主存中，VS-mode 执行 load/store | 两阶段页表遍历均成功，R/W 返回正确值 |
| HCROSS-SSCCPTR-04 | G-stage 页表所在物理页的 PMA 属性验证 | 尝试将 G-stage 页表分配到非 cacheable 或非 coherent 区域（如果平台支持），验证行为 | 若 PMA 不满足 Ssccptr 要求，行为为实现相关（可能 page walk 失败）；若平台不支持配置 PMA，TEST_SKIP |

> [!NOTE]
> - Ssccptr 是 PMA 层面的约束扩展，不引入新指令或 CSR。主存区域默认满足 cacheability+coherence PMA 条件的平台上，HCROSS-SSCCPTR-01~03 预期 PASS。
> - HCROSS-SSCCPTR-04 需要平台支持动态配置 PMA 属性；不支持时该用例 TEST_SKIP。
> - 本组测试的核心价值在于：确保 Hypervisor 两级翻译的页表遍历不会因 PMA 约束而失败，这是虚拟化场景下的关键正确性保证。

---

## Group 3. Hypervisor × Sscounterenw 交叉测试

**规范依据**：
- `hcounteren_vs_vu_control`：`hcounteren` 控制 VS/VU-mode 计数器可用性
- `norm:sscounterenw_hpmcounter_scounteren`：非只读零的 hpmcounter 对应位必须可写

**测试职责**：验证 `hcounteren` 寄存器对 VS/VU-mode 性能监控计数器（hpmcounter）访问的控制行为，以及 `hcounteren` 对应位的可写性。

> [!NOTE]
> - Sscounterenw 扩展要求：对于任何非只读零的 `hpmcounter`，`scounteren`（以及 `hcounteren`）的对应位必须可写。本组测试验证 `hcounteren` 的可写性和控制行为。
> - 请参考 `Shcounterenw_test_plan.md` 扩展测试

---

## Group 4. Hypervisor × Ssstateen 交叉测试

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

#### 4.1 hstateen CSR 存在性与可访问性

**规范依据**：`norm:hstateen_rv64_csrs`、`norm:stateen_rv32_upper_bits_csrs`、`norm:hstateen_encoding`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSSTA-01 | hstateen0 在 HS-mode 可读 | 配置 mstateen0.SE0=1，HS-mode 读 hstateen0 | 无异常 |
| HCROSS-SSSTA-02 | hstateen0 在 HS-mode 可写 | 配置 mstateen0.SE0=1，HS-mode 写 hstateen0 后读回 | 无异常，可写位生效 |
| HCROSS-SSSTA-03 | hstateen1 在 HS-mode 可读写 | 配置 mstateen1 bit63=1，HS-mode 读写 hstateen1 | 无异常 |
| HCROSS-SSSTA-04 | hstateen2 在 HS-mode 可读写 | 配置 mstateen2 bit63=1，HS-mode 读写 hstateen2 | 无异常 |
| HCROSS-SSSTA-05 | hstateen3 在 HS-mode 可读写 | 配置 mstateen3 bit63=1，HS-mode 读写 hstateen3 | 无异常 |
| HCROSS-SSSTA-06 | hstateen0h 在 HS-mode 可读写 (RV32) | RV32 下配置 mstateen0.SE0=1，HS-mode 读写 hstateen0h | 无异常 |

#### 4.2 hstateen bit 63 控制 sstateen 访问

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

#### 4.3 hstateen 对 VS-mode sstateen 的只读零传播

**规范依据**：`norm:sstateen_vsmode_access_roz`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SSSTA-16 | hstateen0.C=0 传播到 VS-mode sstateen0.C | 设 hstateen0.C=0 且 bit63=1，VS-mode 写 sstateen0.C=1 后读回 | sstateen0.C 在 VS-mode 读回 0 |
| HCROSS-SSSTA-17 | hstateen0.C=1 解除 VS-mode 传播 | 设 hstateen0.C=1 且 bit63=1，VS-mode 写 sstateen0.C=1 后读回 | sstateen0.C 在 VS-mode 读回 1 |
| HCROSS-SSSTA-18 | hstateen0.JVT=0 传播到 VS-mode sstateen0.JVT | 设 hstateen0.JVT=0 且 bit63=1，VS-mode 写 sstateen0.JVT=1 后读回 | sstateen0.JVT 在 VS-mode 读回 0 |
| HCROSS-SSSTA-19 | hstateen0 多位同时传播 | 设 hstateen0 多个功能位为 0，VS-mode 逐一验证 sstateen0 对应位 | 所有对应位在 VS-mode 下为只读零 |
| HCROSS-SSSTA-20 | hstateen0 位从 0 改 1 后传播解除 | 先设 hstateen0.C=0 验证传播，再改为 1 | sstateen0.C 在 VS-mode 变为可写 |

#### 4.4 hstateen0 各功能位控制

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

#### 4.5 hstateen 只读约束

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

#### 4.6 hstateen 编码与 mstateen 一致性

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

## Group 5. Hypervisor × Sstc 交叉测试

**规范依据**：
- `norm:henvcfg_stce`：henvcfg.STCE=1 使能 vstimecmp；STCE=0 时 V=1 下访问 stimecmp 产生 virtual-instruction 异常
- `norm:hcounteren_acc`：hcounteren.TM=0 且 mcounteren.TM=1 时，VS-mode 访问 stimecmp（vstimecmp）产生 virtual-instruction 异常（HCROSS-SSTC-05）
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

#### 5.1 henvcfg.STCE 字段控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSTC-01 | henvcfg.STCE 读写回环 | 在 M-mode 下设置 menvcfg.STCE=1 后，写 henvcfg.STCE=1 读回验证，再写 0 读回验证 | STCE bit 读写一致 | `norm:henvcfg_stce` |
| HCROSS-SSTC-02 | henvcfg.STCE 受 menvcfg.STCE 约束 | menvcfg.STCE=0 时，写 henvcfg.STCE=1 应被忽略（read-only zero） | henvcfg.STCE 读回为 0 | `norm:henvcfg_stce` |

#### 5.2 VS-mode 访问控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSTC-03 | henvcfg.STCE=0 时 VS-mode 访问 stimecmp | menvcfg.STCE=1, henvcfg.STCE=0，VS-mode (V=1) 读 stimecmp | 触发 virtual-instruction exception (cause=22) | `norm:henvcfg_stce` |
| HCROSS-SSTC-04 | henvcfg.STCE=1 时 VS-mode 访问 stimecmp | menvcfg.STCE=1, henvcfg.STCE=1, hcounteren.TM=1，VS-mode 读 stimecmp | 无异常，成功读取（实际访问 vstimecmp） | `norm:henvcfg_stce` |
| HCROSS-SSTC-05 | hcounteren.TM=0 时 VS-mode 访问 stimecmp | menvcfg.STCE=1, henvcfg.STCE=1, hcounteren.TM=0，VS-mode 读 stimecmp | 触发 virtual-instruction exception (cause=22) | `norm:hcounteren_acc`、`norm:henvcfg_stce` |

#### 5.3 vstimecmp CSR 读写

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSTC-06 | M-mode vstimecmp 读写回环 | M-mode 写 vstimecmp=0x123456789ABCDEF0 后读回 | 读回值一致 | `norm:vstimecmp_exist` |
| HCROSS-SSTC-07 | vstimecmp 全 1 / 全 0 读写 | M-mode 写 vstimecmp 全 1 和全 0 后分别读回 | 读回值与写入值一致 | `norm:vstimecmp_exist` |
| HCROSS-SSTC-08 | HS-mode vstimecmp 读写 | STCE=1, TM=1, 切换 HS-mode 读写 vstimecmp（通过 CSR 0x24D 直接访问） | 读写正常 | `norm:vstimecmp_exist` |

#### 5.4 VSTIP 合成逻辑

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSTC-09 | vstimecmp 触发 VSTIP | 设置 vstimecmp 为过去值（相对于 time + htimedelta），检查 hip.VSTIP | VSTIP = 1 | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-10 | vstimecmp 清除 VSTIP | 设置 vstimecmp 为最大值，检查 hip.VSTIP（假设 hvip.VSTIP=0） | VSTIP = 0 | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-11 | VSTIP = hvip.VSTIP OR vstimecmp 信号 | STCE=1 时验证 vstimecmp 信号三态跳变：vstimecmp=MAX→VSTIP=0，vstimecmp=expired→VSTIP=1，vstimecmp=MAX→VSTIP=0 | 各态 hip.VSTIP 符合预期 | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-12 | henvcfg.STCE=0 时 VSTIP 恢复旧行为 | menvcfg.STCE=1, henvcfg.STCE=0, 设 vstimecmp 为过去值，检查 hip.VSTIP | VSTIP 仅由 hvip.VSTIP 决定（vstimecmp 信号不生效） | `norm:henvcfg_stce` |

#### 5.5 VS-mode 定时器功能

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSTC-13 | VS-mode 通过 stimecmp 访问 vstimecmp | STCE=1, henvcfg.STCE=1, hcounteren.TM=1, VS-mode 写 stimecmp (=vstimecmp)，M-mode 读 vstimecmp | 读回值一致（硬件重映射验证） | `norm:sstc_vs_facility` |
| HCROSS-SSTC-14 | htimedelta 对 vstimecmp 比较的影响 | 设置 htimedelta 为较大正值，设 vstimecmp 使得 (time+htimedelta) >= vstimecmp，检查 VSTIP | VSTIP = 1 | `norm:hip_vstip_vstie_acc_op` |
| HCROSS-SSTC-15 | VS-mode timer interrupt 捕获 | henvcfg.STCE=1, 使能 VSTIE, 设 vstimecmp 为过去值, VS-mode 应收到 timer interrupt | VS-mode trap handler 捕获 cause = interrupt \| 5 | `norm:sstc_vs_facility` |


> [!NOTE]
> - 本组测试验证 Sstc 扩展在 Hypervisor 场景下的行为。所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展的可用性，不可用时 TEST_SKIP。
> - HCROSS-SSTC-01~02 从 `sstc_test_plan.md` Group 1 迁移而来，验证 `henvcfg.STCE` 的可写性和 `menvcfg.STCE` 的约束关系。
> - HCROSS-SSTC-03~05 从 `sstc_test_plan.md` Group 3 迁移而来，验证 VS-mode 对 `stimecmp`（实际为 `vstimecmp`）的多层访问控制：`henvcfg.STCE` 和 `hcounteren.TM` 任一为 0 时，触发 virtual-instruction 异常（cause=22）。
> - HCROSS-SSTC-06~15 从 `sstc_test_plan.md` Group 6 整体迁移而来，验证 `vstimecmp` CSR 读写、VSTIP 合成逻辑（`hip.VSTIP = hvip.VSTIP OR vstimecmp_signal`）、`htimedelta` 对比较的影响、以及 VS-mode timer interrupt 的捕获。
> - HCROSS-SSTC-11 验证 VSTIP 的 OR 逻辑：当 `henvcfg.STCE=0` 时 vstimecmp 信号不生效，`hvip.VSTIP` 恢复软件可写并影响 `hip.VSTIP`。
> - HCROSS-SSTC-13 验证硬件透明重映射：VS-mode 写 `stimecmp`（CSR 0x14D）实际写入 `vstimecmp`（CSR 0x24D），M-mode 读回 `vstimecmp` 可验证。
> - HCROSS-SSTC-15 验证完整的 VS-mode timer interrupt 路径：通过 `hideleg` 委托 VSTIP 到 VS-mode，VS-mode trap handler 捕获 cause = `interrupt | 5`。

---

## Group 6. Hypervisor × Sscsrind 交叉测试

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

#### 6.1 VS-level CSR 基本功能（从 Sscsrind Group 2 迁移）

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

#### 6.2 Virtual-Instruction 异常行为（从 Sscsrind Group 3 迁移）

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

#### 6.3 State-Enable 访问控制（从 Sscsrind Group 4.2/4.3 迁移）

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|---------|
| SSCSRIND-STA-05 | HCROSS-SSCSRIND-22 | mstateen0[60]=0 阻止 HS-mode 读 vsiselect |
| SSCSRIND-STA-06 | HCROSS-SSCSRIND-23 | mstateen0[60]=0 阻止 HS-mode 读 vsireg* |
| SSCSRIND-STA-07 | HCROSS-SSCSRIND-24 | hstateen0[60]=0 + mstateen0[60]=1 → VS-mode 访问 siselect 触发 virtual-inst |
| SSCSRIND-STA-08 | HCROSS-SSCSRIND-25 | hstateen0[60]=0 + mstateen0[60]=1 → VS-mode 访问 sireg 触发 virtual-inst |
| SSCSRIND-STA-09 | HCROSS-SSCSRIND-26 | hstateen0[60]=1 + mstateen0[60]=1 → VS-mode 可访问 |
| SSCSRIND-STA-10 | HCROSS-SSCSRIND-27 | hstateen0[60]=0 时异常类型为 virtual-inst 而非 illegal-inst |

#### 6.4 Hypervisor 交叉测试（从 Sscsrind Group 5 迁移）

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|---------|
| SSCSRIND-HYP-01 | HCROSS-SSCSRIND-28 | VS-mode 通过 sireg* 访问 vsireg* 的透明重映射 |
| SSCSRIND-HYP-02 | HCROSS-SSCSRIND-29 | VS-mode 通过 siselect 访问 vsiselect 的透明重映射 |
| SSCSRIND-HYP-03 | HCROSS-SSCSRIND-30 | vsireg* 在合法 vsiselect 下读写正确 |
| SSCSRIND-HYP-04 | HCROSS-SSCSRIND-31 | vsireg* 在未实现 vsiselect 下行为 |
| SSCSRIND-HYP-05 | HCROSS-SSCSRIND-32 | HS-mode 和 VS-mode select 空间独立 |
| SSCSRIND-HYP-06 | HCROSS-SSCSRIND-33 | M-mode 和 S-mode select 空间可为别名 |

### 测试用例清单

#### 6.1 VS-level CSR 基本功能

**规范依据**：`norm:vsiselect_min_range`、`norm:vsiselect_msb_op`、`norm:vsireg_access_on_legal_vsiselect`、`norm:vsireg_access_behaviour`、`norm:sscsrind_vsmode_csrs_sz`、`norm:sscsrind_csrs_access_control`、`norm:mstateen_zero_initialization`

**前置条件**：若实现 Smstateen，复位时所有可写 `mstateen` 位初始化为 0（`norm:mstateen_zero_initialization`），且 `mstateen0[60]`=0 时低于 M-mode 的特权级访问 `siselect`/`sireg*`/`vsiselect`/`vsireg*` 触发 illegal-instruction（`norm:sscsrind_csrs_access_control`）。因此本组用例执行前，M-mode 必须先置 `mstateen0[60]`=1（`mstateen0.CSRIND`），再进入 HS-mode 测试；`vsiselect`/`vsireg*` 本身的 WARL/范围行为与 stateen 无关。

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

#### 6.2 Virtual-Instruction 异常行为

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

#### 6.3 State-Enable 访问控制

**规范依据**：`norm:sscsrind_csrs_access_control`、`norm:hypervisor_impl_csrs_access_control`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCSRIND-22 | mstateen0[60]=0 阻止 HS-mode 读 vsiselect | mstateen0[60]=0，HS-mode（S-mode with V=0）读 vsiselect | illegal-instruction 异常 (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SSCSRIND-23 | mstateen0[60]=0 阻止 HS-mode 读 vsireg* | mstateen0[60]=0，HS-mode 读 vsireg | illegal-instruction 异常 (cause=2) | `norm:sscsrind_csrs_access_control` |
| HCROSS-SSCSRIND-24 | hstateen0[60]=0 + mstateen0[60]=1 → VS-mode 访问 siselect 触发 virtual-inst | hstateen0[60]=0 且 mstateen0[60]=1，VS-mode 读 siselect（实际 vsiselect） | virtual-instruction 异常 (cause=22)，而非 illegal-instruction | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SSCSRIND-25 | hstateen0[60]=0 + mstateen0[60]=1 → VS-mode 访问 sireg 触发 virtual-inst | hstateen0[60]=0 且 mstateen0[60]=1，VS-mode 读 sireg（实际 vsireg） | virtual-instruction 异常 (cause=22) | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SSCSRIND-26 | hstateen0[60]=1 + mstateen0[60]=1 → VS-mode 可访问 | hstateen0[60]=1 且 mstateen0[60]=1，VS-mode 读 siselect/sireg* | 访问正常（受 vsiselect 值约束） | `norm:hypervisor_impl_csrs_access_control` |
| HCROSS-SSCSRIND-27 | hstateen0[60]=0 时异常类型为 virtual-inst 而非 illegal-inst | hstateen0[60]=0 且 mstateen0[60]=1，VS-mode 访问 siselect | 异常 cause 必须是 22 (virtual-inst)，不能是 2 (illegal-inst) | `norm:hypervisor_impl_csrs_access_control` |

#### 6.4 Hypervisor 交叉测试

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
> - HCROSS-SSCSRIND-01~10 从 `Sscsrind_test_plan.md` Group 2 迁移而来，验证 vsiselect/vsireg* 的基本功能。vsiselect 的最小范围 0..0xFFF 与 siselect 一致，确保 hypervisor 可以在 VM 内模拟间接访问寄存器。**注意**：若实现 Smstateen，用例须先在 M-mode 置 `mstateen0[60]`=1（复位默认 0 会阻止 HS-mode 访问 vsiselect/vsireg*）。
> - HCROSS-SSCSRIND-11~21 从 `Sscsrind_test_plan.md` Group 3 迁移而来，验证 Virtual-instruction 异常行为。**核心区别**：VS/VU-mode 直接访问 vsiselect/vsireg* 始终触发 virtual-instruction（cause=22），**不论** mstateen0[60] 或 hstateen0[60] 的值。这是 Sscsrind SPEC 中 `norm:sscsrind_virtual_inst_fault` 的明确要求。
> - HCROSS-SSCSRIND-22~27 从 `Sscsrind_test_plan.md` Group 4.2/4.3 迁移而来，验证 state-enable 访问控制。关键区别：当 mstateen0[60]=1 但 hstateen0[60]=0 时，VS/VU-mode 访问 siselect/sireg* 触发的是 **virtual-instruction**（cause=22），而非 illegal-instruction（cause=2）。这是因为 M-mode 已放行（mstateen=1），但 HS-mode 的 hypervisor 选择不放行（hstateen=0），因此异常类型反映了 hypervisor 需要 trap 并处理。
> - HCROSS-SSCSRIND-28~33 从 `Sscsrind_test_plan.md` Group 5 迁移而来，验证 Hypervisor 交叉测试。HCROSS-SSCSRIND-28~29 验证硬件透明重映射的核心行为：在 VM 内，VS-mode 访问 siselect/sireg*（地址 0x150~0x157）时，硬件自动将其重映射为 vsiselect/vsireg*（地址 0x250~0x257）。这对 guest OS 是透明的。
> - 与 Group 4.4 (HCROSS-SSSTA-27~29) 和 `Hypervisor_Sm_test_plan.md` 中 Smcsrind Group 1 (HCROSS-SMCSRIND-01~08) 的区别：Group 4.4 从 Ssstateen 角度验证 hstateen0[60] 控制，Smcsrind 组验证 mstateen0[60] 对 HS-mode 访问 vsiselect/vsireg* 的控制，本组从 Sscsrind 角度验证 hstateen0[60] 控制和 VS/VU-mode 异常行为。实现时应避免重复，可标记为交叉引用。

---

## Group 7. Hypervisor × Ssdbltrp 交叉测试

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

#### 7.1 henvcfg.DTE 控制

**规范依据**：`norm:henvcfg_DTE`、`norm:henvcfg_dte_op`、`norm:menvcfg_dte_op`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSDBLTRP-01 | henvcfg.DTE 读写 | HS-mode 写 henvcfg.DTE=1 后读回验证（menvcfg.DTE=1） | 若 Ssdbltrp 和 H 扩展均实现，DTE 可写且读回一致 | `norm:henvcfg_DTE` |
| HCROSS-SSDBLTRP-02 | henvcfg.DTE=0 时 vsstatus.SDT 只读零 | henvcfg.DTE=0（menvcfg.DTE=1），VS-mode 尝试写 vsstatus.SDT=1 | vsstatus.SDT 读回 0（只读零） | `norm:henvcfg_dte_op` |
| HCROSS-SSDBLTRP-03 | henvcfg.DTE=1 时 vsstatus.SDT 可写 | henvcfg.DTE=1（menvcfg.DTE=1），写 vsstatus.SDT=1 | vsstatus.SDT=1 可写 | `norm:henvcfg_dte_op` |
| HCROSS-SSDBLTRP-04 | henvcfg.DTE=0 时 VS-mode trap 不设 SDT | henvcfg.DTE=0，VS-mode 触发 ecall 正常 trap | vsstatus.SDT 不被硬件修改（功能不存在） | `norm:henvcfg_dte_op` |
| HCROSS-SSDBLTRP-05 | menvcfg.DTE=0 覆盖 henvcfg.DTE | menvcfg.DTE=0，尝试写 henvcfg.DTE=1 | henvcfg.DTE 只读零（menvcfg.DTE 是全局使能） | `norm:menvcfg_dte_op` |
| HCROSS-SSDBLTRP-06 | henvcfg.DTE 动态切换 | menvcfg.DTE=1，先设 henvcfg.DTE=1 验证 vsstatus.SDT 可写；再设 henvcfg.DTE=0 验证只读零 | DTE=1 时可写；DTE=0 后只读零 | `norm:henvcfg_dte_op` |

#### 7.2 vsstatus.SDT（VS-mode）

**规范依据**：`norm:vsstatus_SDT`、`norm:vsstatus_sdt_op`、`norm:sstatus_sdt_trap`、`norm:sstatus_sdt_sstatus_sie_overwrite`、`norm:HS_mode_invoke_error`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSDBLTRP-07 | vsstatus.SDT WARL 读写 | HS-mode 写 vsstatus.SDT=1（henvcfg.DTE=1, menvcfg.DTE=1）后读回 | vsstatus.SDT 可写且读回一致 | `norm:vsstatus_SDT` |
| HCROSS-SSDBLTRP-08 | vsstatus.SDT=1 写时自动清零 vsstatus.SIE | 写 vsstatus.SDT=1（同时写 SIE=1） | vsstatus.SIE 被强制清零 | `norm:sstatus_sdt_sstatus_sie_overwrite` |
| HCROSS-SSDBLTRP-09 | vsstatus.SDT=1 时无法设置 vsstatus.SIE=1 | 设 vsstatus.SDT=1，然后写 vsstatus.SIE=1 | vsstatus.SIE 读回为 0 | `norm:sstatus_sdt_sstatus_sie_overwrite` |
| HCROSS-SSDBLTRP-10 | VS-mode trap 时 vsstatus.SDT 自动设为 1 | henvcfg.DTE=1，设 vsstatus.SDT=0，从 VU-mode 触发 ecall trap 到 VS-mode | vsstatus.SDT=1 | `norm:sstatus_sdt_trap` |
| HCROSS-SSDBLTRP-11 | VS-mode SDT=1 时 trap 触发 double-trap | henvcfg.DTE=1，设 vsstatus.SDT=1，VU-mode 触发 ecall（委托到 VS-mode） | double-trap 异常交付到 M-mode（mcause=16），mtval2=8（ecall-from-VU） | `norm:sstatus_sdt_trap` |
| HCROSS-SSDBLTRP-12 | VS-mode double-trap 时 M-mode CSR 正确 | VS-mode double-trap 到 M-mode | mstatus.MPV=1, MPP 正确, mepc=触发地址, mcause=16, mtval2=原始 cause | `norm:sstatus_sdt_trap` |

#### 7.3 SRET 对 vsstatus.SDT 的清除

**规范依据**：`norm:sret_dt`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSDBLTRP-13 | HS-mode SRET 到 VU 清除 vsstatus.SDT | HS-mode 设 vsstatus.SDT=1，配置 SRET 返回到 VU-mode（SPV=1, SPP=0） | vsstatus.SDT=0 | `norm:sret_dt` |
| HCROSS-SSDBLTRP-14 | VS-mode SRET 清除 vsstatus.SDT | VS-mode 设 vsstatus.SDT=1，执行 SRET | vsstatus.SDT=0 | `norm:sret_dt` |
| HCROSS-SSDBLTRP-15 | HS-mode SRET 到 VS 不清 vsstatus.SDT | HS-mode 设 vsstatus.SDT=1，配置 SRET 返回到 VS-mode（SPV=1, SPP=1） | vsstatus.SDT 保持 1（仅 VU 模式才清除 vsstatus.SDT） | `norm:sret_dt` |
| HCROSS-SSDBLTRP-16 | HS-mode SRET 到 HS 不清 vsstatus.SDT | HS-mode 设 vsstatus.SDT=1，配置 SRET 返回到 HS-mode（SPV=0） | vsstatus.SDT 保持 1 | `norm:sret_dt` |

#### 7.4 menvcfg.DTE 对 Hypervisor CSR 的控制

**规范依据**：`norm:menvcfg_dte_op`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSDBLTRP-17 | menvcfg.DTE=0 时 vsstatus.SDT 只读零 | menvcfg.DTE=0，尝试写 vsstatus.SDT=1（H 扩展存在时） | vsstatus.SDT 读回 0（只读零） | `norm:menvcfg_dte_op` |
| HCROSS-SSDBLTRP-18 | menvcfg.DTE=0 时 henvcfg.DTE 只读零 | menvcfg.DTE=0，尝试写 henvcfg.DTE=1（H 扩展存在时） | henvcfg.DTE 读回 0（只读零） | `norm:menvcfg_dte_op` |

#### 7.5 MRET/SRET/MNRET 跨模式清除（Hypervisor 场景）

**规范依据**：`norm:sstatus_sdt_clr_mret_sret`、`norm:vsstatus_sdt_clr_mret_sret`、`norm:vsstatus_sdt_clr_mnret`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSDBLTRP-19 | MRET 到 VS-mode 清除 sstatus.SDT | 设 sstatus.SDT=1, mstatus.MPP=1(S), MPV=1, 执行 MRET | sstatus.SDT=0 | `norm:sstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-20 | MRET 到 VU-mode 清除 sstatus.SDT 和 vsstatus.SDT | 设 sstatus.SDT=1, vsstatus.SDT=1, MPP=0(U), MPV=1, 执行 MRET | sstatus.SDT=0 且 vsstatus.SDT=0 | `norm:sstatus_sdt_clr_mret_sret` `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-21 | MRET 到 VU 只清 vsstatus.SDT | 设 sstatus.SDT=0, vsstatus.SDT=1, MPP=0(U), MPV=1, 执行 MRET | vsstatus.SDT=0 | `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-22 | MRET 到 VS-mode 不清 vsstatus.SDT | 设 vsstatus.SDT=1, MPP=1(S), MPV=1, 执行 MRET | vsstatus.SDT 保持 1（仅 VU 清除 vsstatus.SDT） | `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-23 | M-mode SRET 到 VU 清除 sstatus.SDT 和 vsstatus.SDT | M-mode 设 SDT=1, vsstatus.SDT=1, MPP=0, SPV=1, 执行 SRET | sstatus.SDT=0 且 vsstatus.SDT=0 | `norm:sstatus_sdt_clr_mret_sret` `norm:vsstatus_sdt_clr_mret_sret` |
| HCROSS-SSDBLTRP-24 | MNRET 到 VU 清除 sstatus.SDT 和 vsstatus.SDT | 设 SDT=1, vsstatus.SDT=1, MNPP=0(U), MNPV=1, 执行 MNRET | sstatus.SDT=0 且 vsstatus.SDT=0 | `norm:vsstatus_sdt_clr_mnret` |


> [!NOTE]
> - 本组测试验证 Ssdbltrp 扩展在 Hypervisor 场景下的行为。所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展的可用性，不可用时 TEST_SKIP。
> - HCROSS-SSDBLTRP-01~06 从 `Ssdbltrp_test_plan.md` Group 5 迁移而来，验证 `henvcfg.DTE` 的可写性和对 VS-mode Ssdbltrp 功能的使能/禁用控制。`menvcfg.DTE` 是全局使能，`henvcfg.DTE` 是 VS-mode 级别的使能。
> - HCROSS-SSDBLTRP-07~12 从 `Ssdbltrp_test_plan.md` Group 6 迁移而来，验证 `vsstatus.SDT` 的 WARL 读写、SDT/SIE 互斥、以及 VS-mode double-trap 交付机制。VS-mode double-trap 最终交付到 M-mode（mcause=16），且 `mstatus.MPV=1` 标识来自虚拟化模式。
> - HCROSS-SSDBLTRP-13~16 从 `Ssdbltrp_test_plan.md` Group 3 迁移而来，验证 SRET 对 `vsstatus.SDT` 的清除行为。关键规则：HS-mode SRET **仅在返回到 VU-mode 时**清除 vsstatus.SDT；返回 VS-mode 或 HS-mode 不清除。VS-mode 自身的 SRET 始终清除 vsstatus.SDT。
> - HCROSS-SSDBLTRP-17~18 从 `Ssdbltrp_test_plan.md` Group 4 迁移而来，验证 `menvcfg.DTE=0` 时对 Hypervisor CSR（vsstatus.SDT、henvcfg.DTE）的全局禁用效果。
> - HCROSS-SSDBLTRP-19~24 从 `Ssdbltrp_test_plan.md` Group 7 迁移而来，验证 MRET/SRET/MNRET 在 Hypervisor 场景下的跨模式 SDT 清除。核心规则：sstatus.SDT 在新模式为 U/VS/VU 时清除；vsstatus.SDT **仅在新模式为 VU 时**清除。
> - 与 `Hypervisor_Sv_test_plan.md` 中 Svnapot Group 3 (HCROSS-SVNAPOT) 和 Group 5 (HCROSS-SSTC) 的区别：本组专注于 double-trap 机制在虚拟化场景下的行为，不涉及地址翻译或定时器功能。

---

## Group 8. Hypervisor × Ssctr 交叉测试

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

#### 8.1 vsctrctl CSR（VS-mode CTR 控制寄存器）

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

#### 8.2 VS/VU-mode 对 CTR CSR 的访问限制

**规范依据**：`norm:sctrdepth_mode`、`norm:sctrclr_exceptions`、`norm:vsiselect_op`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCTR-11 | VS-mode 访问 sctrdepth 触发异常 | VS-mode 尝试访问 sctrdepth | 触发 virtual-instruction 异常（cause=22） | `norm:sctrdepth_mode` |
| HCROSS-SSCTR-12 | VU-mode 访问 sctrdepth 触发异常 | VU-mode 尝试访问 sctrdepth | 触发 virtual-instruction 异常（cause=22） | `norm:sctrdepth_mode` |
| HCROSS-SSCTR-13 | vsireg* 与 sireg* 访问相同状态 | HS-mode 设 vsiselect=0x200，通过 vsireg 写入值；再设 siselect=0x200，通过 sireg 读 | 读到相同的值（共享 entry 寄存器状态） | `norm:vsiselect_op` |
| HCROSS-SSCTR-14 | VU-mode 执行 SCTRCLR 触发异常 | VU-mode 执行 SCTRCLR | 触发 virtual-instruction 异常（cause=22） | `norm:sctrclr_exceptions` |

#### 8.3 VS/VU-mode 外部陷阱录制

**规范依据**：`norm:exttrap_vshs`、`norm:exttrap_vuhs`、`norm:exttrap_vuvs`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCTR-15 | VS→HS 外部陷阱需 STE | sctrctl.S=0, vsctrctl.S=1, sctrctl.STE=1，VS-mode 触发陷阱到 HS-mode | 外部陷阱被录制 | `norm:exttrap_vshs` |
| HCROSS-SSCTR-16 | VS→HS 外部陷阱 STE=0 时不录制 | sctrctl.S=0, vsctrctl.S=1, sctrctl.STE=0，VS-mode 触发陷阱到 HS-mode | 外部陷阱不被录制 | `norm:exttrap_vshs` |
| HCROSS-SSCTR-17 | VU→VS 外部陷阱需 vsctrctl.STE | vsctrctl.S=0, vsctrctl.U=1, vsctrctl.STE=1，VU-mode 触发陷阱到 VS-mode | 外部陷阱被录制 | `norm:exttrap_vuvs` |
| HCROSS-SSCTR-18 | VU→VS 外部陷阱 vsSTE=0 时不录制 | vsctrctl.S=0, vsctrctl.U=1, vsctrctl.STE=0，VU-mode 触发陷阱到 VS-mode | 外部陷阱不被录制 | `norm:exttrap_vuvs` |
| HCROSS-SSCTR-19 | VU→HS 外部陷阱需 STE + vsSTE | sctrctl.S=0, sctrctl.STE=1, vsctrctl.U=1, vsctrctl.STE=1，VU-mode 触发陷阱到 HS-mode | 外部陷阱被录制 | `norm:exttrap_vuhs` |
| HCROSS-SSCTR-20 | VU→HS 缺少 vsSTE 时不录制 | sctrctl.S=0, sctrctl.STE=1, vsctrctl.U=1, vsctrctl.STE=0，VU-mode 触发陷阱到 HS-mode | 外部陷阱不被录制 | `norm:exttrap_vuhs` |

#### 8.4 VS-mode Freeze 行为（vsctrctl 控制）

**规范依据**：`norm:ctr_freeze_vs`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCTR-21 | VS-mode BPFRZ 由 vsctrctl 控制 | vsctrctl.BPFRZ=1，VS-mode 执行 EBREAK 陷阱到 VS-mode | sctrstatus.FROZEN=1 | `norm:ctr_freeze_vs` |
| HCROSS-SSCTR-22 | VS-mode BPFRZ=0 不设置 FROZEN | vsctrctl.BPFRZ=0，VS-mode 执行 EBREAK | sctrstatus.FROZEN 不变 | `norm:ctr_freeze_vs` |
| HCROSS-SSCTR-23 | VS-mode LCOFIFRZ 由 vsctrctl 控制 | vsctrctl.LCOFIFRZ=1，LCOFI 陷阱到 VS-mode | sctrstatus.FROZEN=1 | `norm:ctr_freeze_vs` |
| HCROSS-SSCTR-24 | 虚拟 LCOFI 也触发 freeze | vsctrctl.LCOFIFRZ=1，hypervisor 挂起虚拟 LCOFI，陷阱到 VS-mode | sctrstatus.FROZEN=1 | `norm:ctr_freeze_vs` |

#### 8.5 虚拟化模式转换配置来源

**规范依据**：虚拟化模式转换使用 vsctrctl 中的字段（除 M/MTE/LCOFIFRZ/BPFRZ 由 sctrctl/mctrctl 控制外）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCTR-25 | VU→HS 转换使用 vsctrctl 过滤位 | vsctrctl.U=1, sctrctl.S=1，vsctrctl 中设置某过滤位，VU-mode 触发陷阱到 HS-mode | 转换是否被录制由 vsctrctl 中的过滤位决定（而非 sctrctl） | `norm:vsctr-s_op` |
| HCROSS-SSCTR-26 | VS→HS 转换使用 vsctrctl/sctrctl 字段 | vsctrctl.S=1, sctrctl.S=1，VS-mode 触发陷阱到 HS-mode | 源模式启用由 vsctrctl.S 决定，目标模式启用由 sctrctl.S 决定 | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-27 | HS→VS/VU trap return 使用 sctrctl | sctrctl.S=1, vsctrctl.S=1，HS-mode 执行 SRET 返回 VS-mode | 源模式启用由 sctrctl.S 决定，目标模式由 vsctrctl.S 决定 | `norm:Ssctr_vsctrctl_sz_acc_op` |
| HCROSS-SSCTR-28 | Freeze 使用 sctrctl（陷阱到 HS-mode） | sctrctl.BPFRZ=1，VS-mode breakpoint 陷阱到 HS-mode | freeze 由 sctrctl.BPFRZ 决定 | `norm:ctr_freeze_bp` |
| HCROSS-SSCTR-29 | VS-mode freeze 使用 vsctrctl | vsctrctl.BPFRZ=1，VS-mode breakpoint 陷阱到 VS-mode | freeze 由 vsctrctl.BPFRZ 决定 | `norm:ctr_freeze_vs` |

#### 8.6 hstateen0.CTR 对 VS-mode CTR 访问的控制

**规范依据**：`norm:hstateen_ctr`、`norm:hstateen_vs`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCTR-30 | VS-mode 检测 CTR 可访问性 | VS-mode 尝试访问 sctrctl，观察是否触发异常 | 若 hstateen0.CTR=1 则访问成功；若 CTR=0 则触发 virtual-instruction | `norm:hstateen_vs` |
| HCROSS-SSCTR-31 | hstateen0.CTR=0 时 VS-mode sctrstatus 访问 | hstateen0.CTR=0，VS-mode 访问 sctrstatus | 触发 virtual-instruction | `norm:hstateen_vs` |
| HCROSS-SSCTR-32 | hstateen0.CTR=0 时 VS-mode SCTRCLR 执行 | hstateen0.CTR=0，VS-mode 执行 SCTRCLR | 触发 virtual-instruction | `norm:hstateen_vs` |

> [!NOTE]
> - 本组测试验证 Ssctr 扩展在 Hypervisor 场景下的行为。所有测试必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展的可用性，不可用时 TEST_SKIP。
> - HCROSS-SSCTR-01~10 从 `Ssctr_test_plan.md` Group 2 迁移而来，验证 `vsctrctl` CSR 的基本功能。`vsctrctl` 是 H 扩展引入的 CSR，V=1 时替代 `sctrctl`，V=0 时不影响行为。
> - HCROSS-SSCTR-11~14 从 `Ssctr_test_plan.md` Groups 3/5 迁移而来，验证 VS/VU-mode 对 CTR CSR 的访问限制。VS/VU-mode 访问 `sctrdepth` 和执行 SCTRCLR 触发 virtual-instruction（cause=22）。HCROSS-SSCTR-13 验证 `vsireg*` 与 `sireg*` 共享同一组 CTR entry 寄存器状态（V=1 时没有单独的 entry 寄存器集）。
> - HCROSS-SSCTR-15~20 从 `Ssctr_test_plan.md` Group 8 迁移而来，验证 VS/VU-mode 外部陷阱录制。外部陷阱录制依赖中间模式的 TE 位：VS→HS 需要 sctrctl.STE；VU→VS 需要 vsctrctl.STE；VU→HS 需要 sctrctl.STE + vsctrctl.STE（两个 TE 位都必须置位）。
> - HCROSS-SSCTR-21~24 从 `Ssctr_test_plan.md` Group 12 迁移而来，验证 VS-mode 的 Freeze 行为由 `vsctrctl` 中的 BPFRZ/LCOFIFRZ 控制（而非 `sctrctl`）。注意：当陷阱到 HS-mode 时，freeze 由 `sctrctl.BPFRZ` 决定（HCROSS-SSCTR-28）。
> - HCROSS-SSCTR-25~29 从 `Ssctr_test_plan.md` Group 13 迁移而来，验证虚拟化模式转换时的配置来源选择。核心规则：源模式启用由该模式对应的 xctrctl 决定（VS-mode 用 vsctrctl，HS-mode 用 sctrctl）；freeze 由陷阱目标模式的 xctrctl 决定（陷阱到 VS-mode 用 vsctrctl，陷阱到 HS-mode 用 sctrctl）。
> - HCROSS-SSCTR-30~32 从 `Ssctr_test_plan.md` Group 14 迁移而来，验证 `hstateen0.CTR` 对 VS-mode CTR 访问的控制。`hstateen0.CTR=0` 时 VS-mode 访问 CTR 状态触发 virtual-instruction（cause=22），而非 illegal-instruction。
> - 与 `Hypervisor_Sm_test_plan.md` 中 Smctr Group 2 (HCROSS-SMCTR) 的区别：Smctr 组从 M-mode 角度验证 `mstateen0.CTR` 和 `hstateen0.CTR` 对下级特权级的控制，以及 MTE 外部陷阱到 M-mode 的录制；本组从 S-mode/VS-mode 角度验证 `vsctrctl` 功能和 VS/VU-mode 的 CTR 行为。

---

## Group 9. Hypervisor × Ssqosid 交叉测试

本组测试验证 Ssqosid 扩展在 Hypervisor 场景下的行为，即 V=1 时 VS/VU-mode 访问 `srmcfg` CSR 的异常行为。这些测试从 `Ssqosid_test_plan.md` 迁移而来，专门针对依赖 H 扩展的用例。

**规范依据**：
- `ssqosid_virtinst`：若 mstateen0[55]=1 或未实现 Smstateen，V=1 时尝试访问 srmcfg 引发 virtual-instruction exception
- `ssqosid_smstateen_bit55_0`：若 mstateen0[55]=0，低于 M 模式的特权级访问 srmcfg 引发 illegal-instruction exception

**测试职责**：验证虚拟化模式下 srmcfg 的访问异常行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SRMCFG-19 | V=1 VS-mode 读 srmcfg 触发 virtual-instruction exception | 未实现 Smstateen 或 mstateen0[55]=1，VS-mode 执行 csrr srmcfg | virtual-instruction exception (cause=22) |
| SRMCFG-20 | V=1 VS-mode 写 srmcfg 触发 virtual-instruction exception | 未实现 Smstateen 或 mstateen0[55]=1，VS-mode 执行 csrw srmcfg | virtual-instruction exception (cause=22) |
| SRMCFG-21 | V=1 VU-mode 访问 srmcfg 触发异常 | VU-mode 尝试访问 srmcfg（CSR 0x181 为 S 级 CSR） | illegal-instruction (cause=2) 或 virtual-instruction (cause=22)（规范优先级存在歧义） |
| SRMCFG-22 | V=0 HS-mode 正常访问 srmcfg | V=0（HS-mode），csrr/csrw srmcfg | 正常读写成功，无异常 |
| SRMCFG-23 | Smstateen 实现且 mstateen0[55]=0 时 V=1 访问 | mstateen0[55]=0，VS-mode 尝试 csrr srmcfg | illegal-instruction exception (cause=2)（mstateen0 门控优先于 V=1 规则） |
| SRMCFG-24 | virtual-instruction trap 时 stval/htinst 值 | VS-mode 访问 srmcfg 触发 virtual-instruction exception，检查 stval 和 htinst | stval 为 0 或故障指令编码（SYSTEM opcode），htinst 为 0 或转换值 |

### 测试 ID 映射表

| 原始 ID | 新位置 | 测试名称 |
|---------|--------|----------|
| SRMCFG-19 | Group 9 | V=1 VS-mode 读 srmcfg |
| SRMCFG-20 | Group 9 | V=1 VS-mode 写 srmcfg |
| SRMCFG-21 | Group 9 | V=1 VU-mode 访问 srmcfg |
| SRMCFG-22 | Group 9 | V=0 HS-mode 正常访问 |
| SRMCFG-23 | Group 9 | mstateen0[55]=0 V=1 访问 |
| SRMCFG-24 | Group 9 | stval/htinst 值验证 |

### 实现注意事项

1. **扩展检测**：测试前需检测 Ssqosid（srmcfg CSR 0x181 存在性）和 H 扩展（misa.H）的可用性，不可用时 TEST_SKIP。

2. **Smstateen 交互**：若 Smstateen 已实现，测试 SRMCFG-19/20/21/22/24 前需设置 mstateen0[55]=1，以确保测试的是 V=1 规则而非 mstateen 门控。

3. **SRMCFG-21 规范歧义**：VU-mode（有效特权级=U）访问 S 级 CSR 时，标准 CSR 访问规则给出 cause=2（illegal-instruction），而 Ssqosid SPEC 的 "when V=1" 措辞暗示 cause=22（virtual-instruction）。测试接受两种 cause 值。

4. **SRMCFG-23 优先级**：mstateen0[55]=0 的门控规则优先于 V=1 的 virtual-instruction 规则，因此 VS-mode 访问应触发 illegal-instruction (cause=2)。

---

## Group 10. Hypervisor × Sscofpmf 交叉测试

**规范依据**：
- `norm:mhpmevent_inh_op`：五个 xINH 位中的每一个置位时抑制对应特权模式下的事件计数；VSINH/VUINH 分别抑制 VS/VU-mode 计数；对应特权模式未实现时该位为只读零
- `norm:scountovf_vsmode_read_access`：VS-mode 下 `scountovf` bit X 可读当且仅当 `mcounteren` bit X 与 `hcounteren` bit X 均置位，否则读为零
- `norm:scountovf_smode_read_access_control`：`scountovf` bit X 的读取访问受与 hpmcounter 访问相同的 `mcounteren`/`hcounteren` 规则控制

**测试职责**：验证 Sscofpmf 扩展在 Hypervisor 场景下的行为，包括 VS-mode 读取 `scountovf` 的 `mcounteren`+`hcounteren` 双重门控、以及 `mhpmevent` VSINH/VUINH 对 VS/VU-mode 事件计数的抑制。

> **注意**：本组用例中 01~03 从 `Sscofpmf_test_plan.md` Group 4（COFPMF-SOV-08~10）迁移而来，专门针对依赖 H 扩展的用例；04~06 补齐原方案 Group 2（特权模式过滤）缺失的 VSINH/VUINH 功能用例。需要 H 扩展和 Sscofpmf 扩展同时可用（06 除外，见下）。

### 测试 ID 映射表

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|---------|
| COFPMF-SOV-08 | HCROSS-SSCOFPMF-01 | VS-mode scountovf 双重 gate（均允许） |
| COFPMF-SOV-09 | HCROSS-SSCOFPMF-02 | VS-mode mcounteren=0 读为零 |
| COFPMF-SOV-10 | HCROSS-SSCOFPMF-03 | VS-mode hcounteren=0 读为零 |
| —（新增） | HCROSS-SSCOFPMF-04 | VSINH=1 抑制 VS-mode 计数 |
| —（新增） | HCROSS-SSCOFPMF-05 | VUINH=1 抑制 VU-mode 计数 |
| —（新增） | HCROSS-SSCOFPMF-06 | 未实现 H 扩展时 VSINH/VUINH 只读零 |

### 测试用例清单

#### 10.1 VS-mode scountovf 双重门控（从 Sscofpmf Group 4 迁移）

**规范依据**：`norm:scountovf_vsmode_read_access`、`norm:scountovf_smode_read_access_control`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCOFPMF-01 | VS-mode scountovf 双重 gate（均允许） | mcounteren bit 3 = 1, hcounteren bit 3 = 1，M-mode 设 mhpmevent3 OF=1，VS-mode 读 scountovf | scountovf bit 3 = 1（读到真实 OF 值） | `norm:scountovf_vsmode_read_access` |
| HCROSS-SSCOFPMF-02 | VS-mode mcounteren=0 读为零 | mcounteren bit 3 = 0（不论 hcounteren），OF=1，VS-mode 读 scountovf | scountovf bit 3 = 0 | `norm:scountovf_vsmode_read_access` |
| HCROSS-SSCOFPMF-03 | VS-mode hcounteren=0 读为零 | mcounteren bit 3 = 1, hcounteren bit 3 = 0，OF=1，VS-mode 读 scountovf | scountovf bit 3 = 0 | `norm:scountovf_vsmode_read_access` |

#### 10.2 VSINH/VUINH 计数抑制（补齐原方案 Group 2 缺失）

**规范依据**：`norm:mhpmevent_inh_op`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCOFPMF-04 | VSINH=1 抑制 VS-mode 计数 | 设 mhpmevent VSINH=1 并配置已退休指令事件，VS-mode 执行固定指令循环，M-mode 读取计数器差值 | VS-mode 执行期间计数器不递增 | `norm:mhpmevent_inh_op` |
| HCROSS-SSCOFPMF-05 | VUINH=1 抑制 VU-mode 计数 | 设 mhpmevent VUINH=1，VU-mode 执行固定指令循环，M-mode 读取计数器差值 | VU-mode 执行期间计数器不递增 | `norm:mhpmevent_inh_op` |
| HCROSS-SSCOFPMF-06 | 未实现 H 扩展时 VSINH/VUINH 只读零 | 若 H 扩展未实现，写 mhpmevent VSINH/VUINH=1 后读回 | VSINH/VUINH 为只读零（实现 H 扩展的平台 TEST_SKIP） | `norm:mhpmevent_inh_op` |

> [!NOTE]
> - 本组测试验证 Sscofpmf 扩展在 Hypervisor 场景下的行为。HCROSS-SSCOFPMF-01~05 必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展可用性，不可用时 TEST_SKIP；HCROSS-SSCOFPMF-06 仅在 H 扩展**未实现**时执行，实现 H 扩展时 TEST_SKIP。
> - 所有用例需先探测 Sscofpmf 是否实现（trap-protected 写 `mhpmevent` OF 位并读回，写触发 illegal-instruction 则未实现），未实现时整组 TEST_SKIP；并采用动态发现法探测目标 `mhpmcounter`（写非零值读回为零则未实现，改用其他计数器或 TEST_SKIP）。
> - HCROSS-SSCOFPMF-01~03 的访问控制语义与 `hpmcounter` 一致：VS-mode 读 `scountovf` bit X 需 `mcounteren[X]` 与 `hcounteren[X]` **同时**为 1 才读到真实 OF 值，否则读为零（注意是读零而非触发 trap）。
> - HCROSS-SSCOFPMF-04~05 需使用 `goto_priv(PRIV_VS)`/`goto_priv(PRIV_VU)` 进入虚拟特权级执行计数循环，回到 M-mode 读取 `mhpmcounter` 差值，不做精确计数断言。模式切换本身产生指令计数，可设 MINH=1 抑制 M-mode 计数以排除干扰（与 `Sscofpmf_test_plan.md` Group 2 NOTE 的处理方式一致）；需编译时启用 `ENABLE_HYP` 宏并配置两阶段翻译使 VS/VU-mode 可执行。
> - 与 `Sscofpmf_test_plan.md` COFPMF-RW-05/06 的关系：原用例验证 VSINH/VUINH 的 WARL 读写正分支（不依赖 H 扩展），保留在独立方案；本组 06 承接"未实现 H 扩展时只读零"负分支，04~05 承接依赖 H 扩展的计数抑制功能验证。VSINH/VUINH 与 Smcntrpmf 的 `mcyclecfg`/`minstretcfg` VSINH/VUINH 共用相同位编码（bit 59/58），但属不同寄存器，与 `Hypervisor_Sm_test_plan.md` Group 3 无重叠。
> - 与 Group 11（Smcdeleg/Ssccfg）的关系：本组验证**未启用计数器委托**（CDE 不参与）时 VS-mode 读 `scountovf` 的 `mcounteren`+`hcounteren` 门控（读零语义）；Group 11 验证 **CDE=1** 时 VS/VU-mode 读 `scountovf` 被虚拟化（virtual-instruction 语义，`norm:ssccfg_virtual_scountovf_vs_vu`）。两者互补，实现时需注意 `menvcfg.CDE` 的前置状态。

---

## Group 11. Hypervisor × Smcdeleg/Ssccfg 交叉测试

**规范依据**：
- `norm:ssccfg_virtual_scountovf_vs_vu`：支持 Smcdeleg/Ssccfg、Sscofpmf 与 H 扩展的实现，当 `menvcfg.CDE=1` 时，VS/VU-mode 读 `scountovf` 触发 virtual-instruction 异常
- `norm:ssccfg_illegal_scountinhibit_vs_vu`：计数器委托启用（CDE=1）时，VS/VU-mode 访问 `scountinhibit` 触发 virtual-instruction 异常
- `norm:ssccfg_lcofi_hvip_hvien`：支持 Smcdeleg/Ssccfg、Sscofpmf、Smaia/Ssaia 与 H 扩展的实现，`hvip` 和 `hvien` 的 LCOFI 位（bit 13）已实现且可写；隐含 `vsie`/`vsip` bit 13 也实现
- `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal`：实现 H 扩展时，VS/VU-mode 直接访问 `vsiselect`/`vsireg*`，或 VU-mode 访问 `siselect`/`sireg*`，触发 virtual-instruction
- `norm:ssccfg_hyp_m_s_vsireg_illegal`：`vsiselect` 在 0x40-0x5F 范围时，M 或 S 模式访问任何 `vsireg*` 触发 illegal-instruction
- `norm:ssccfg_hyp_vs_access_sireg_conditional`：VS-mode 访问 `sireg*`（实际为 `vsireg*`）时，`menvcfg.CDE=0` 触发 illegal-instruction，CDE=1 触发 virtual-instruction
- `norm:hstateen0_csrind_op`：hstateen0 bit 60 控制 VS-mode 对 siselect/sireg*（实为 vsiselect/vsireg*）的访问

**测试职责**：验证 Smcdeleg/Ssccfg 计数器委托扩展在 Hypervisor 场景下的虚拟化行为，包括 `scountovf`/`scountinhibit` 的 VS/VU-mode 虚拟化、`hvip`/`hvien` LCOFI 虚拟中断位、`vsiselect`/`vsireg*` 的多特权级访问规则、以及 hstateen0 bit 60 的交叉控制。

> **注意**：本组用例从 `Ssccfg_test_plan.md` Groups 4/5/6/8（SSCFG-OVF-01~04、SSCFG-HLCOFI-01~05、SSCFG-HYP-01~10、SSCFG-STA-04~06）迁移而来，专门针对依赖 H 扩展的用例；05~06 补齐原方案中 `norm:ssccfg_illegal_scountinhibit_vs_vu` 无对应用例的缺口。需要 H 扩展与 Smcdeleg/Ssccfg（含 `menvcfg.CDE`）同时可用，部分子组需额外扩展（见各子组说明）。
>
> **前提配置**：M-mode 需预先将 `menvcfg.CDE` 设为所需值，并委托目标计数器（`mcounteren` 对应位）。

### 测试 ID 映射表

| 原始 ID | 新 ID | 测试名称 |
|---------|-------|---------|
| SSCFG-OVF-01 | HCROSS-SSCCFG-01 | VS-mode 读 scountovf（CDE=1）触发 virtual-instruction |
| SSCFG-OVF-02 | HCROSS-SSCCFG-02 | VU-mode 读 scountovf（CDE=1）触发 virtual-instruction |
| SSCFG-OVF-03 | HCROSS-SSCCFG-03 | HS-mode 读 scountovf（CDE=1）正常 |
| SSCFG-OVF-04 | HCROSS-SSCCFG-04 | VS-mode 读 scountovf（CDE=0）行为 |
| —（新增） | HCROSS-SSCCFG-05 | VS-mode 访问 scountinhibit（CDE=1）触发 virtual-instruction |
| —（新增） | HCROSS-SSCCFG-06 | VU-mode 访问 scountinhibit（CDE=1）触发 virtual-instruction |
| SSCFG-HLCOFI-01 | HCROSS-SSCCFG-07 | hvip bit 13（LCOFI）可写性 |
| SSCFG-HLCOFI-02 | HCROSS-SSCCFG-08 | hvien bit 13（LCOFI）可写性 |
| SSCFG-HLCOFI-03 | HCROSS-SSCCFG-09 | hvip.LCOFI 独立验证 |
| SSCFG-HLCOFI-04 | HCROSS-SSCCFG-10 | hvien.LCOFI 独立验证 |
| SSCFG-HLCOFI-05 | HCROSS-SSCCFG-11 | vsie/vsip LCOFI 位隐含实现 |
| SSCFG-HYP-01 | HCROSS-SSCCFG-12 | VS-mode 直接访问 vsiselect 触发 virtual-instruction |
| SSCFG-HYP-02 | HCROSS-SSCCFG-13 | VS-mode 直接访问 vsireg 触发 virtual-instruction |
| SSCFG-HYP-03 | HCROSS-SSCCFG-14 | VU-mode 直接访问 vsiselect 触发 virtual-instruction |
| SSCFG-HYP-04 | HCROSS-SSCCFG-15 | VU-mode 直接访问 vsireg 触发 virtual-instruction |
| SSCFG-HYP-05 | HCROSS-SSCCFG-16 | VU-mode 访问 siselect 触发 virtual-instruction |
| SSCFG-HYP-06 | HCROSS-SSCCFG-17 | VU-mode 访问 sireg 触发 virtual-instruction |
| SSCFG-HYP-07 | HCROSS-SSCCFG-18 | M-mode 在 vsiselect 0x40-0x5F 时访问 vsireg 非法 |
| SSCFG-HYP-08 | HCROSS-SSCCFG-19 | HS-mode 在 vsiselect 0x40-0x5F 时访问 vsireg 非法 |
| SSCFG-HYP-09 | HCROSS-SSCCFG-20 | VS-mode 经 sireg* 访问（CDE=0）→ illegal-instruction |
| SSCFG-HYP-10 | HCROSS-SSCCFG-21 | VS-mode 经 sireg* 访问（CDE=1）→ virtual-instruction |
| SSCFG-STA-04 | HCROSS-SSCCFG-22 | hstateen0 bit 60=0 阻止 VS-mode 写 siselect |
| SSCFG-STA-05 | HCROSS-SSCCFG-23 | hstateen0 bit 60=0 阻止 VS-mode 读 sireg |
| SSCFG-STA-06 | HCROSS-SSCCFG-24 | hstateen0 bit 60=1 允许 VS-mode 访问 |

### 测试用例清单

#### 11.1 scountovf 虚拟化（从 Ssccfg Group 4 迁移）

**规范依据**：`norm:ssccfg_virtual_scountovf_vs_vu`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCCFG-01 | VS-mode 读 scountovf（CDE=1）触发 virtual-instruction | CDE=1，VS-mode 读 scountovf (0xDA0) | 触发 virtual-instruction 异常 (cause=22) | `norm:ssccfg_virtual_scountovf_vs_vu` |
| HCROSS-SSCCFG-02 | VU-mode 读 scountovf（CDE=1）触发 virtual-instruction | CDE=1，VU-mode 读 scountovf | 触发 virtual-instruction 异常 (cause=22) | `norm:ssccfg_virtual_scountovf_vs_vu` |
| HCROSS-SSCCFG-03 | HS-mode 读 scountovf（CDE=1）正常 | CDE=1，HS-mode（V=0 的 S-mode）读 scountovf | 访问成功，无异常 | `norm:ssccfg_virtual_scountovf_vs_vu` |
| HCROSS-SSCCFG-04 | VS-mode 读 scountovf（CDE=0）行为 | CDE=0，VS-mode 读 scountovf | 不受本虚拟化条款约束，按 Sscofpmf 基本规则（见 Group 10 HCROSS-SSCOFPMF-01~03 的门控语义） | `norm:ssccfg_virtual_scountovf_vs_vu`（负向） |

#### 11.2 scountinhibit VS/VU 虚拟化（补齐缺口）

**规范依据**：`norm:ssccfg_illegal_scountinhibit_vs_vu`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCCFG-05 | VS-mode 访问 scountinhibit（CDE=1）触发 virtual-instruction | CDE=1，VS-mode 读写 scountinhibit (0x120) | 触发 virtual-instruction 异常 (cause=22) | `norm:ssccfg_illegal_scountinhibit_vs_vu` |
| HCROSS-SSCCFG-06 | VU-mode 访问 scountinhibit（CDE=1）触发 virtual-instruction | CDE=1，VU-mode 读 scountinhibit | 触发 virtual-instruction 异常 (cause=22) | `norm:ssccfg_illegal_scountinhibit_vs_vu` |

#### 11.3 LCOFI 虚拟化：hvip/hvien bit 13（从 Ssccfg Group 5 迁移）

**规范依据**：`norm:ssccfg_lcofi_hvip_hvien`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCCFG-07 | hvip bit 13（LCOFI）可写性 | HS-mode 写 hvip bit 13 = 1 后读回，再写 0 读回 | bit 13 可写且读回一致 | `norm:ssccfg_lcofi_hvip_hvien` |
| HCROSS-SSCCFG-08 | hvien bit 13（LCOFI）可写性 | HS-mode 写 hvien bit 13 = 1 后读回，再写 0 读回 | bit 13 可写且读回一致 | `norm:ssccfg_lcofi_hvip_hvien` |
| HCROSS-SSCCFG-09 | hvip.LCOFI 独立验证 | 写 hvip 全 1 后读回，检查 bit 13 | bit 13 读回为 1 | `norm:ssccfg_lcofi_hvip_hvien` |
| HCROSS-SSCCFG-10 | hvien.LCOFI 独立验证 | 写 hvien 全 1 后读回，检查 bit 13 | bit 13 读回为 1 | `norm:ssccfg_lcofi_hvip_hvien` |
| HCROSS-SSCCFG-11 | vsie/vsip LCOFI 位隐含实现 | 验证 vsie bit 13 与 vsip bit 13 的存在性（读写不触发异常） | vsie/vsip bit 13 存在（hvip.LCOFI 的实现隐含这些位的实现） | `norm:ssccfg_lcofi_hvip_hvien` |

#### 11.4 vsiselect/vsireg* 多特权级访问规则（从 Ssccfg Group 6 迁移）

**规范依据**：`norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal`、`norm:ssccfg_hyp_m_s_vsireg_illegal`、`norm:ssccfg_hyp_vs_access_sireg_conditional`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCCFG-12 | VS-mode 直接访问 vsiselect 触发 virtual-instruction | VS-mode 直接读写 vsiselect (0x240)（vsiselect 在 0x40-0x5F） | 触发 virtual-instruction 异常 (cause=22) | `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` |
| HCROSS-SSCCFG-13 | VS-mode 直接访问 vsireg 触发 virtual-instruction | VS-mode 直接读写 vsireg (0x245) | 触发 virtual-instruction 异常 (cause=22) | `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` |
| HCROSS-SSCCFG-14 | VU-mode 直接访问 vsiselect 触发 virtual-instruction | VU-mode 直接读写 vsiselect | 触发 virtual-instruction 异常 (cause=22) | `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` |
| HCROSS-SSCCFG-15 | VU-mode 直接访问 vsireg 触发 virtual-instruction | VU-mode 直接读写 vsireg | 触发 virtual-instruction 异常 (cause=22) | `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` |
| HCROSS-SSCCFG-16 | VU-mode 访问 siselect 触发 virtual-instruction | VU-mode 读写 siselect | 触发 virtual-instruction 异常 (cause=22) | `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` |
| HCROSS-SSCCFG-17 | VU-mode 访问 sireg 触发 virtual-instruction | VU-mode 读写 sireg | 触发 virtual-instruction 异常 (cause=22) | `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` |
| HCROSS-SSCCFG-18 | M-mode 在 vsiselect 0x40-0x5F 时访问 vsireg 非法 | M-mode 设 vsiselect=0x40，访问 vsireg* | 触发 illegal-instruction 异常 | `norm:ssccfg_hyp_m_s_vsireg_illegal` |
| HCROSS-SSCCFG-19 | HS-mode 在 vsiselect 0x40-0x5F 时访问 vsireg 非法 | HS-mode 设 vsiselect=0x40，访问 vsireg* | 触发 illegal-instruction 异常 | `norm:ssccfg_hyp_m_s_vsireg_illegal` |
| HCROSS-SSCCFG-20 | VS-mode 经 sireg* 访问（CDE=0）→ illegal-instruction | menvcfg.CDE=0，VS-mode 经 siselect=0x40 访问 sireg（实际为 vsireg） | 触发 illegal-instruction 异常 | `norm:ssccfg_hyp_vs_access_sireg_conditional` |
| HCROSS-SSCCFG-21 | VS-mode 经 sireg* 访问（CDE=1）→ virtual-instruction | menvcfg.CDE=1，VS-mode 经 siselect=0x40 访问 sireg（实际为 vsireg） | 触发 virtual-instruction 异常 | `norm:ssccfg_hyp_vs_access_sireg_conditional` |

#### 11.5 hstateen0 bit 60 交叉控制（从 Ssccfg Group 8 迁移）

**规范依据**：`norm:hstateen0_csrind_op`

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| HCROSS-SSCCFG-22 | hstateen0 bit 60=0 阻止 VS-mode 写 siselect | mstateen0 bit 60=1，hstateen0 bit 60=0，VS-mode 写 siselect | 触发 virtual-instruction 异常 | `norm:hstateen0_csrind_op` |
| HCROSS-SSCCFG-23 | hstateen0 bit 60=0 阻止 VS-mode 读 sireg | mstateen0 bit 60=1，hstateen0 bit 60=0，VS-mode 读 sireg | 触发 virtual-instruction 异常 | `norm:hstateen0_csrind_op` |
| HCROSS-SSCCFG-24 | hstateen0 bit 60=1 允许 VS-mode 访问 | mstateen0 bit 60=1，hstateen0 bit 60=1，VS-mode 访问 siselect/sireg* | 访问不因 hstateen0 被阻止（CDE=1 时可能触发 virtual-instruction，由 HCROSS-SSCCFG-21 覆盖） | `norm:hstateen0_csrind_op` |

> [!NOTE]
> - 本组所有用例必须在运行时通过 `HAS_H_EXT()` 检测 H 扩展、并探测 Smcdeleg/Ssccfg（`menvcfg.CDE` 可写性 + `siselect` 存在性，依赖 Sscsrind），任一不可用时整组 TEST_SKIP。
> - 11.1/11.2 需额外实现 Sscofpmf（`scountovf` 存在）；11.3 需额外实现 Sscofpmf + Smaia/Ssaia（`hvien` 存在），缺失时对应子组 TEST_SKIP。
> - HCROSS-SSCCFG-01~02 与 Group 10（HCROSS-SSCOFPMF-01~03）的区分关键在 `menvcfg.CDE`：CDE=0 时 VS-mode 读 `scountovf` 按 mcounteren+hcounteren 门控读零；CDE=1 时被虚拟化，读访问直接触发 virtual-instruction，hypervisor 介入。实现用例前必须显式设置 CDE 状态。
> - HCROSS-SSCCFG-05~06 补齐 `Ssccfg_test_plan.md` 中 `norm:ssccfg_illegal_scountinhibit_vs_vu` 无对应用例的缺口：`scountinhibit` 的 CDE=0 非法访问（illegal-instruction）由原方案 SSCFG-SINH-12/13 覆盖，本组覆盖 CDE=1 的 VS/VU 虚拟化分支。
> - HCROSS-SSCCFG-12~17 与 Group 6 Sscsrind（HCROSS-SSCSRIND-11~20）的关系：VS/VU 直接访问 vsiselect/vsireg* 触发 virtual-instruction 的基础行为属 `norm:sscsrind_virtual_inst_fault`，本组从 Ssccfg 角度验证 vsiselect 处于委托计数器区（0x40-0x5F）时的同源行为，实现时可交叉引用避免重复。
> - HCROSS-SSCCFG-18~19 是 Ssccfg 特有的新增规则：`vsiselect` 在 0x40-0x5F 时，M/S-mode 也**不得**直接访问 `vsireg*`（illegal-instruction），因为该区间状态属于 VS-mode 委托计数器，应通过修改 guest 状态间接管理。
> - HCROSS-SSCCFG-22~24 与 Group 4.4（HCROSS-SSSTA-27~29）、Group 6.3（HCROSS-SSCSRIND-24~27）验证相同的 `hstateen0[60]` 控制，本组从 Ssccfg 委托计数器角度补充，实现时可交叉引用。
> - M-mode 层面的 `menvcfg.CDE` 使能、`mcounteren` 委托位设置、`mvip`/`mvien` LCOFI 验证由 `Smcdeleg_test_plan.md` 覆盖，不在本组范围。
> - `vsiselect` CSR 地址为 0x240，`vsireg` 为 0x245；`scountinhibit` 为 0x120；`scountovf` 为 0xDA0；`hvip`/`hvien` 分别为 0x645/0x648。

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 6.2 (Virtual-inst) | HCROSS-SSCSRIND-11~21 | virtual-instruction 异常是虚拟化安全隔离的核心保证 |
| P0（必须） | Group 7.2 (vsstatus.SDT) | HCROSS-SSDBLTRP-07~12 | VS-mode double-trap 是虚拟化场景的关键安全机制 |
| P1（重要） | Group 1 (Sstvala) | HCROSS-SSTVALA-01~08 | stval/vstval 精确写入是 guest OS 调试和异常处理的基础 |
| P1（重要） | Group 6.3 (State-Enable) | HCROSS-SSCSRIND-22~27 | hstateen0[60] 访问控制是安全隔离的关键 |
| P1（重要） | Group 6.1 (VS CSR) | HCROSS-SSCSRIND-01~10 | vsiselect/vsireg* 是 H 扩展场景的基础 |
| P1（重要） | Group 7.1 (henvcfg.DTE) | HCROSS-SSDBLTRP-01~06 | henvcfg.DTE 是 VS-mode double-trap 的使能控制 |
| P1（重要） | Group 7.3 (SRET vsstatus.SDT) | HCROSS-SSDBLTRP-13~16 | SRET 对 vsstatus.SDT 的清除是 VS-mode trap 返回的关键路径 |
| P1（重要） | Group 7.5 (XRET Hyp) | HCROSS-SSDBLTRP-19~24 | MRET/SRET/MNRET 在虚拟化场景下的跨模式 SDT 清除 |
| P1（重要） | Group 8.1 (vsctrctl) | HCROSS-SSCTR-01~10 | vsctrctl 是 VS-mode CTR 的核心控制寄存器 |
| P1（重要） | Group 8.3 (VS/VU ext traps) | HCROSS-SSCTR-15~20 | VS/VU-mode 外部陷阱录制是虚拟化场景的关键 |
| P1（重要） | Group 8.5 (Virt transitions) | HCROSS-SSCTR-25~29 | 虚拟化模式转换配置来源是 CTR 在 Hypervisor 下的正确性保证 |
| P2（建议） | Group 3 (Sscounterenw) | HCROSS-SSCOUNTERENW-01~09 | hcounteren 控制行为是性能监控隔离的保证 |
| P2（建议） | Group 5 (Sstc) | HCROSS-SSTC-01~15 | vstimecmp 和 VS-mode 定时器是 Hypervisor 虚拟化定时器的核心设施 |
| P2（建议） | Group 6.4 (Hyp 交叉) | HCROSS-SSCSRIND-28~33 | 透明重映射和别名行为依赖 H 扩展 |
| P2（建议） | Group 7.4 (menvcfg.DTE Hyp) | HCROSS-SSDBLTRP-17~18 | menvcfg.DTE 对 Hypervisor CSR 的全局控制 |
| P2（建议） | Group 8.2 (VS/VU access) | HCROSS-SSCTR-11~14 | VS/VU-mode 对 CTR CSR 的访问限制 |
| P2（建议） | Group 8.4 (VS Freeze) | HCROSS-SSCTR-21~24 | VS-mode Freeze 行为由 vsctrctl 控制 |
| P2（建议） | Group 8.6 (hstateen VS) | HCROSS-SSCTR-30~32 | hstateen0.CTR 对 VS-mode CTR 访问的控制 |
| P2（建议） | Group 10 (Sscofpmf) | HCROSS-SSCOFPMF-01~06 | VS-mode scountovf 双重门控与 VSINH/VUINH 计数抑制是性能监控隔离的保证 |
| P2（建议） | Group 11 (Smcdeleg/Ssccfg) | HCROSS-SSCCFG-01~24 | scountovf/scountinhibit 虚拟化、LCOFI 虚拟中断位与 vsireg* 访问规则是计数器委托隔离的保证 |
| P3（可选） | Group 2 (Ssccptr) | HCROSS-SSCCPTR-01~04 | PMA 层面的约束依赖平台保证，动态 PMA 配置能力受限的用例按平台能力 TEST_SKIP |

> 注：Ssqosid（Group 9）的测试用例（SRMCFG-19~24）在原始合并方案中未单独标注优先级，建议参照 `Ssqosid_test_plan.md` 的优先级执行。Group 4 (Ssstateen) 的 hstateen 控制用例（HCROSS-SSSTA-01~50）优先级参照原合并方案 Group 8 的 P1 定级。

---

## 关键注意事项

1. **扩展检测**：所有测试必须在运行时检测所需扩展（H、Sstvala、Ssccptr、Sscounterenw、Ssstateen、Sstc、Sscsrind、Ssdbltrp、Ssctr、Ssqosid、Sscofpmf、Smcdeleg/Ssccfg 等）的可用性，不可用时 TEST_SKIP。

2. **Sstvala 的精确性要求**：Sstvala 扩展强制要求 `stval` 写入 faulting 地址，而非 0。测试断言必须使用 `TEST_ASSERT_EQ` 精确比较，不能用 `TEST_ASSERT(stval != 0)` 模糊验证。

3. **Ssccptr 的平台依赖**：PMA 是平台硬连线属性，主存默认满足 cacheability+coherence。HCROSS-SSCCPTR-04 需要平台支持动态 PMA 配置，不支持时 TEST_SKIP。

4. **Sscounterenw 的探测逻辑**：测试前需探测哪些 `hpmcounter` 非只读零，仅对这些计数器验证 `hcounteren` 对应位的可写性。

5. **virtual-instruction 与 illegal-instruction 的区分**：VS/VU-mode 访问受控 CSR 时，若 mstateen 放行但 hstateen 阻止，触发 virtual-instruction (cause=22)；若 mstateen 也阻止，则触发 illegal-instruction (cause=2)。测试断言必须使用准确的 cause 常量。

---

## 参考

- `SPEC/hypervisor.adoc` — RISC-V Hypervisor Extension, Version 1.0
- `SPEC/sstvala.adoc` — Sstvala Extension
- `SPEC/ssccptr.adoc` — Ssccptr Extension
- `SPEC/sscounterenw.adoc` — Sscounterenw Extension
- `SPEC/smstateen.adoc` — Smstateen Extension Specification
- `SPEC/sstc.adoc` — Sstc Extension Specification (Supervisor-mode Timer Interrupts)
- `SPEC/smcsrind.adoc` — Smcsrind/Sscsrind Extension for Indirect CSR Access
- `SPEC/ssdbltrp.adoc` — Ssdbltrp Double Trap Extension
- `SPEC/ssctr.adoc` — Ssctr (Control Transfer Records - Supervisor-level) Extension
- `SPEC/riscv-ssqosid/sqosid.adoc` — Ssqosid (QoS Identifiers) Extension Specification
- `SPEC/sscofpmf.adoc` — Sscofpmf Extension Specification (Count Overflow and Mode-Based Filtering)
- `SPEC/smcdeleg.adoc` — Smcdeleg and Ssccfg Counter Delegation Extensions
- `DOCS/testplan/Hypervisor_CSR_test_plan.md` — Hypervisor CSR 子集测试计划
- `DOCS/testplan/Hypervisor_Interrupts_test_plan.md` — Hypervisor 中断子集测试计划
- `DOCS/testplan/Hypervisor_Exceptions_test_plan.md` — Hypervisor 异常与 trap 子集测试计划
- `DOCS/testplan/Hypervisor_2_stage_test_plan.md` — 两阶段翻译测试计划
- `DOCS/testplan/Hypervisor_gstage_test_plan.md` — G-stage 独立测试计划
- `DOCS/testplan/sstvala_test_plan.md` — Sstvala 独立测试计划
- `DOCS/testplan/ssccptr_test_plan.md` — Ssccptr 独立测试计划
- `DOCS/testplan/sscounterenw_test_plan.md` — Sscounterenw 独立测试计划
- `DOCS/testplan/ssstateen_test_plan.md` — Ssstateen 独立测试计划
- `DOCS/testplan/sstc_test_plan.md` — Sstc 独立测试计划
- `DOCS/testplan/Sscsrind_test_plan.md` — Sscsrind Supervisor Mode 测试计划
- `DOCS/testplan/Ssdbltrp_test_plan.md` — Ssdbltrp 独立测试计划
- `DOCS/testplan/Ssctr_test_plan.md` — Ssctr Supervisor Mode 测试计划
- `DOCS/testplan/Ssqosid_test_plan.md` — Ssqosid 独立测试计划
- `DOCS/testplan/Sscofpmf_test_plan.md` — Sscofpmf 独立测试计划
- `DOCS/testplan/Ssccfg_test_plan.md` — Ssccfg 独立测试计划
- `ideas/hypervisor_gap.md` — Hypervisor 测试缺口分析

---

## 附录 A：规范点覆盖矩阵

下表标明"覆盖的规范点"章节及各 Group 规范依据中每条规范点被哪些测试用例覆盖。带（自行拆解）标注的为非官方标签条目。

| Norm ID | 覆盖的测试 ID |
|---------|---------------|
| `norm:H_guest_page_fault` | HCROSS-SSTVALA-01~03 |
| `norm:sstvala_stval_faulting_vaddr` | HCROSS-SSTVALA-01~05 |
| `norm:sstvala_stval_faulting_instruction` | HCROSS-SSTVALA-06~08 |
| `norm:ssccptr_memory_pte_reads` | HCROSS-SSCCPTR-01~04 |
| `norm:sscounterenw_hpmcounter_scounteren` | Group 3（Sscounterenw，用例待按 `Shcounterenw_test_plan.md` 模式细化） |
| `hcounteren_vs_vu_control`（自行拆解） | Group 3（Sscounterenw） |
| `norm:hstateen_rv64_csrs` | HCROSS-SSSTA-01~05 |
| `norm:stateen_rv32_upper_bits_csrs` | HCROSS-SSSTA-06（RV32 平台；RV64 TEST_SKIP） |
| `norm:hstateen_encoding` | HCROSS-SSSTA-46~50 |
| `norm:hstateen_bit_63_op` | HCROSS-SSSTA-09~15、HCROSS-SSSTA-21~23 |
| `norm:hstateen_bit_63_writable` | HCROSS-SSSTA-07、HCROSS-SSSTA-08、HCROSS-SSSTA-12 |
| `norm:sstateen_vsmode_access_roz` | HCROSS-SSSTA-16~20 |
| `norm:sstateen_ro1_bits` | HCROSS-SSSTA-39~42（结合 hstateen RO1 约束联合验证） |
| `norm:hstateen_ro1_bits` | HCROSS-SSSTA-39~42 |
| `norm:stateen_warl_access` | HCROSS-SSSTA-45 |
| `norm:stateen_unimplemented_state_roz` | HCROSS-SSSTA-44 |
| `norm:stateen_reserved_roz` | HCROSS-SSSTA-43 |
| `norm:hstateen0_SE0_op` | HCROSS-SSSTA-21~23 |
| `norm:hstateen0_envcfg_op` | HCROSS-SSSTA-24~26 |
| `norm:hstateen0_csrind_op` | HCROSS-SSSTA-27~29 |
| `norm:hstateen0_imsic_op` | HCROSS-SSSTA-30~32 |
| `norm:hstateen0_aia_op` | HCROSS-SSSTA-33~35 |
| `norm:hstateen0_context_op` | HCROSS-SSSTA-36~38 |
| `norm:hcounteren_acc` | HCROSS-SSTC-05 |
| `norm:henvcfg_stce` | HCROSS-SSTC-01~04、HCROSS-SSTC-12 |
| `norm:vstimecmp_exist` | HCROSS-SSTC-06~08 |
| `norm:sstc_vs_facility` | HCROSS-SSTC-13、HCROSS-SSTC-15 |
| `norm:hip_vstip_vstie_acc_op` | HCROSS-SSTC-09~11、HCROSS-SSTC-14 |
| `norm:vsiselect_min_range` | HCROSS-SSCSRIND-01~03、HCROSS-SSCSRIND-09 |
| `norm:vsiselect_msb_op` | HCROSS-SSCSRIND-04、HCROSS-SSCSRIND-05 |
| `norm:vsireg_access_on_legal_vsiselect` | HCROSS-SSCSRIND-10、HCROSS-SSCSRIND-28~30、HCROSS-SSCSRIND-32 |
| `norm:vsireg_access_behaviour` | HCROSS-SSCSRIND-06、HCROSS-SSCSRIND-07、HCROSS-SSCSRIND-31 |
| `norm:sscsrind_vsmode_csrs_sz` | HCROSS-SSCSRIND-08 |
| `norm:sscsrind_virtual_inst_fault` | HCROSS-SSCSRIND-11~20 |
| `norm:vsmode_virtual_inst_fault` | HCROSS-SSCSRIND-21 |
| `norm:hypervisor_impl_csrs_access_control` | HCROSS-SSCSRIND-24~27 |
| `norm:sscsrind_csrs_access_control` | HCROSS-SSCSRIND-22、HCROSS-SSCSRIND-23 |
| `norm:csrs_alias` | HCROSS-SSCSRIND-33 |
| `norm:henvcfg_DTE` | HCROSS-SSDBLTRP-01 |
| `norm:henvcfg_dte_op` | HCROSS-SSDBLTRP-02~04、HCROSS-SSDBLTRP-06 |
| `norm:menvcfg_dte_op` | HCROSS-SSDBLTRP-05、HCROSS-SSDBLTRP-17、HCROSS-SSDBLTRP-18 |
| `norm:vsstatus_SDT` | HCROSS-SSDBLTRP-07 |
| `norm:vsstatus_sdt_op` | HCROSS-SSDBLTRP-07~12 |
| `norm:sstatus_sdt_trap` | HCROSS-SSDBLTRP-10~12 |
| `norm:sstatus_sdt_sstatus_sie_overwrite` | HCROSS-SSDBLTRP-08、HCROSS-SSDBLTRP-09 |
| `norm:sret_dt` | HCROSS-SSDBLTRP-13~16 |
| `norm:vsstatus_sdt_clr_mret_sret` | HCROSS-SSDBLTRP-20~23 |
| `norm:vsstatus_sdt_clr_mnret` | HCROSS-SSDBLTRP-24 |
| `norm:sstatus_sdt_clr_mret_sret` | HCROSS-SSDBLTRP-19~21、HCROSS-SSDBLTRP-23、HCROSS-SSDBLTRP-24 |
| `norm:HS_mode_invoke_error` | HCROSS-SSDBLTRP-11、HCROSS-SSDBLTRP-12（double-trap 交付路径） |
| `norm:Ssctr_vsctrctl_sz_acc_op` | HCROSS-SSCTR-01~03、HCROSS-SSCTR-09、HCROSS-SSCTR-10、HCROSS-SSCTR-26、HCROSS-SSCTR-27 |
| `norm:vsctr-s_op` | HCROSS-SSCTR-04、HCROSS-SSCTR-25 |
| `norm:vsctrctl-u_op` | HCROSS-SSCTR-05 |
| `norm:vsctrctl-ste_op` | HCROSS-SSCTR-06 |
| `norm:vsctrctl-bpfrz_op` | HCROSS-SSCTR-07 |
| `norm:vsctrctl-lcofifrz_op` | HCROSS-SSCTR-08 |
| `norm:exttrap_vshs` | HCROSS-SSCTR-15、HCROSS-SSCTR-16 |
| `norm:exttrap_vuhs` | HCROSS-SSCTR-19、HCROSS-SSCTR-20 |
| `norm:exttrap_vuvs` | HCROSS-SSCTR-17、HCROSS-SSCTR-18 |
| `norm:ctr_freeze_vs` | HCROSS-SSCTR-21~24、HCROSS-SSCTR-29 |
| `norm:ctr_freeze_bp` | HCROSS-SSCTR-28 |
| `norm:sctrdepth_mode` | HCROSS-SSCTR-11、HCROSS-SSCTR-12 |
| `norm:sctrclr_exceptions` | HCROSS-SSCTR-14 |
| `norm:vsiselect_op` | HCROSS-SSCTR-13 |
| `norm:hstateen_ctr` | HCROSS-SSCTR-30~32 |
| `norm:hstateen_vs` | HCROSS-SSCTR-30~32 |
| `norm:mhpmevent_inh_op` | HCROSS-SSCOFPMF-04、HCROSS-SSCOFPMF-05、HCROSS-SSCOFPMF-06（VSINH/VUINH 部分；M/S/U 模式过滤由 `Sscofpmf_test_plan.md` Group 2 覆盖） |
| `norm:scountovf_vsmode_read_access` | HCROSS-SSCOFPMF-01~03 |
| `norm:scountovf_smode_read_access_control` | HCROSS-SSCOFPMF-01~03（VS-mode 侧；S/HS-mode 侧由 `Sscofpmf_test_plan.md` Group 4 覆盖） |
| `norm:ssccfg_virtual_scountovf_vs_vu` | HCROSS-SSCCFG-01~04 |
| `norm:ssccfg_illegal_scountinhibit_vs_vu` | HCROSS-SSCCFG-05、HCROSS-SSCCFG-06（CDE=1 虚拟化分支；CDE=0 分支由 `Ssccfg_test_plan.md` SSCFG-SINH-12/13 覆盖） |
| `norm:ssccfg_lcofi_hvip_hvien` | HCROSS-SSCCFG-07~11 |
| `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` | HCROSS-SSCCFG-12~17 |
| `norm:ssccfg_hyp_m_s_vsireg_illegal` | HCROSS-SSCCFG-18、HCROSS-SSCCFG-19 |
| `norm:ssccfg_hyp_vs_access_sireg_conditional` | HCROSS-SSCCFG-20、HCROSS-SSCCFG-21 |
| `norm:hstateen0_csrind_op` | HCROSS-SSCCFG-22~24（Ssccfg 角度；同源验证见 HCROSS-SSSTA-27~29、HCROSS-SSCSRIND-24~27） |
| `ssqosid_virtinst`（自行拆解） | SRMCFG-19、SRMCFG-20、SRMCFG-21、SRMCFG-24 |
| `ssqosid_smstateen_bit55_0`（自行拆解） | SRMCFG-23 |
