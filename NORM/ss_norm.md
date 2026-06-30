# Ss 扩展 Norm 规范条目表

> 来源：`SPEC/ss.adoc` — Supervisor Extensions 章节

## Ssqosid - QoS 标识符扩展

### 背景与目的

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Ssqosid_shared_resource_need_management` | When multiple workloads execute concurrently on modern processors—equipped with large core counts, multiple cache hierarchies, and multiple memory controllers—the performance of any given workload becomes less deterministic, or even non-deterministic, due to shared resource contention. | 在现代处理器上多个工作负载并发执行时（配备大量核心、多级缓存和多个内存控制器），由于共享资源争用，任何给定工作负载的性能变得不确定甚至非确定。 |
| `norm:Ssqosid_hw_monitoring_required` | To manage performance variability, system software needs resource allocation and monitoring capabilities. These capabilities allow for the reservation of resources like cache and bandwidth, thus meeting individual performance targets while minimizing interference. For resource management, hardware should provide monitoring features that allow system software to profile workload resource consumption and allocate resources accordingly. | 为管理性能变化，系统软件需要资源分配和监控能力，以预留缓存和带宽等资源，满足各自性能目标并最小化干扰。硬件应提供监控功能，使系统软件能分析工作负载资源消耗并据此分配资源。 |
| `norm:Ssqosid_srmcfg_introduced` | To facilitate this, the QoS Identifiers extension (Ssqosid) introduces the `srmcfg` register, which configures a hart with two identifiers: a Resource Control ID (`RCID`) and a Monitoring Counter ID (`MCID`). These identifiers accompany each request issued by the hart to shared resource controllers. | 为此，QoS 标识符扩展（Ssqosid）引入了 `srmcfg` 寄存器，为 hart 配置两个标识符：资源控制 ID（`RCID`）和监控计数器 ID（`MCID`）。这些标识符随 hart 发往共享资源控制器的每个请求一起传递。 |
| `norm:Ssqosid_metadata_used_by_controllers` | Additional metadata, like the nature of the memory access and the ID of the originating supervisor domain, can accompany `RCID` and `MCID`. Resource controllers may use this metadata for differentiated service such as a different capacity allocation for code storage vs. data storage. Resource controllers can use this data for security policies such as not exposing statistics of one security domain to another. | 额外元数据（如内存访问性质和源监管域 ID）可随 `RCID` 和 `MCID` 一起传递。资源控制器可使用这些元数据进行差异化服务（如代码存储与数据存储的不同容量分配），也可用于安全策略（如不将一个安全域的统计信息暴露给另一个）。 |
| `norm:Ssqosid_cbqri_spec` | These identifiers are crucial for the RISC-V Capacity and Bandwidth Controller QoS Register Interface (CBQRI) specification, which provides methods for setting resource usage limits and monitoring resource consumption. The `RCID` controls resource allocations, while the `MCID` is used for tracking resource usage. | 这些标识符对 RISC-V 容量和带宽控制器 QoS 寄存器接口（CBQRI）规范至关重要，该规范提供了设置资源使用限制和监控资源消耗的方法。`RCID` 控制资源分配，`MCID` 用于跟踪资源使用。 |

### srmcfg 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Ssqosid_srmcfg_register_size` | The `srmcfg` register is an SXLEN-bit read/write register used to configure a Resource Control ID (`RCID`) and a Monitoring Counter ID (`MCID`). Both `RCID` and `MCID` are WARL fields. | `srmcfg` 寄存器是一个 SXLEN 位读写寄存器，用于配置资源控制 ID（`RCID`）和监控计数器 ID（`MCID`）。`RCID` 和 `MCID` 均为 WARL 字段。 |
| `norm:Ssqosid_rcid_mcid_accompany_requests` | The `RCID` and `MCID` accompany each request made by the hart to shared resource controllers. The `RCID` is used to determine the resource allocations (e.g., cache occupancy limits, memory bandwidth limits, etc.) to enforce. The `MCID` is used to identify a counter to monitor resource usage. | `RCID` 和 `MCID` 随 hart 向共享资源控制器发出的每个请求一起传递。`RCID` 用于确定要强制执行资源分配（如缓存占用限制、内存带宽限制等），`MCID` 用于标识监控资源使用的计数器。 |
| `norm:Ssqosid_srmcfg_csr_applies_to_all_modes` | The `RCID` and `MCID` configured in the `srmcfg` CSR apply to all privilege modes of software execution on that hart by default, but this behavior may be overridden by future extensions. | `srmcfg` CSR 中配置的 `RCID` 和 `MCID` 默认适用于该 hart 上软件执行的所有特权模式，但此行为可能被未来扩展覆盖。 |

### 访问控制

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Ssqosid_smstateen_srmcfg_requires_mstateen0` | If extension Smstateen is implemented together with Ssqosid, then Ssqosid also requires the SRMCFG bit in `mstateen0` to be implemented. | 如果 Smstateen 扩展与 Ssqosid 一起实现，则 Ssqosid 还要求实现 `mstateen0` 中的 SRMCFG 位。 |
| `norm:Ssqosid_srmcfg_access_illegal_instruction` | If `mstateen0`.SRMCFG is 0, attempts to access `srmcfg` in privilege modes less privileged than M-mode raise an illegal-instruction exception. | 如果 `mstateen0`.SRMCFG 为 0，在低于 M 模式的特权模式下尝试访问 `srmcfg` 会触发非法指令异常。 |
| `norm:Ssqosid_srmcfg_access_virtual_instruction` | If `mstateen0`.SRMCFG is 1 or if extension Smstateen is not implemented, attempts to access `srmcfg` when `V=1` raise a virtual-instruction exception. | 如果 `mstateen0`.SRMCFG 为 1 或未实现 Smstateen 扩展，当 `V=1` 时尝试访问 `srmcfg` 会触发虚拟指令异常。 |

## Ssu64xl - UXLEN=64 支持

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:ssu64xl_uxl_sstatus` | If the Ssu64xl extension is implemented, then `sstatus.UXL` must be capable of holding the value 2 (i.e., UXLEN=64 must be supported). | 如果实现了 Ssu64xl 扩展，则 `sstatus.UXL` 必须能够保存值 2（即必须支持 UXLEN=64）。 |

## Ssccptr - 主存页表读取

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:ssccptr_memory_pte_reads` | If the Ssccptr extension is implemented, then main memory regions with both the cacheability and coherence PMAs must support hardware page-table reads. | 如果实现了 Ssccptr 扩展，则同时具有可缓存性和一致性 PMA 的主存区域必须支持硬件页表读取。 |

## Sstvecd - 直接陷阱向量

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sstvecd_stvec_mode_direct` | If the Sstvecd extension is implemented, then `stvec.MODE` must be capable of holding the value 0 (Direct). | 如果实现了 Sstvecd 扩展，则 `stvec.MODE` 必须能够保存值 0（直接模式）。 |
| `norm:sstvecd_stvec_base_aligned_address` | Furthermore, when `stvec.MODE=Direct`, `stvec.BASE` must be capable of holding any valid four-byte-aligned address. | 此外，当 `stvec.MODE=Direct` 时，`stvec.BASE` 必须能够保存任何有效的四字节对齐地址。 |

## Sstvala - 陷阱值报告

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sstvala_stval_faulting_vaddr` | If the Sstvala extension is implemented, then `stval` must be written with the faulting virtual address for load, store, and instruction page-fault, access-fault, and misaligned exceptions, and for breakpoint exceptions that are defined to write an address to stval, other than those caused by execution of the `EBREAK` or `C.EBREAK` instructions. | 如果实现了 Sstvala 扩展，则对于加载、存储和指令页错误、访问错误及非对齐异常，以及定义为向 stval 写入地址的断点异常（由 `EBREAK` 或 `C.EBREAK` 指令引起的除外），`stval` 必须写入故障虚拟地址。 |
| `norm:sstvala_stval_faulting_instruction` | For virtual-instruction and illegal-instruction exceptions, `stval` must be written with the faulting instruction. | 对于虚拟指令和非法指令异常，`stval` 必须写入故障指令。 |

## Sscounterenw - 计数器使能可写性

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sscounterenw_hpmcounter_scounteren` | If the Sscounterenw extension is implemented, then for any `hpmcounter` that is not read-only zero, the corresponding bit in `scounteren` must be writable. | 如果实现了 Sscounterenw 扩展，则对于任何非只读零的 `hpmcounter`，`scounteren` 中对应的位必须是可写的。 |

## Ssstrict - 扩展一致性

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:ssstrict_no_nonconforming_extensions` | If the Ssstrict extension is implemented, then no non-conforming extensions are present. | 如果实现了 Ssstrict 扩展，则不存在不一致的扩展。 |
| `norm:ssstrict_unimplemented_opcodes_csr_exception` | Furthermore, attempts to execute unimplemented opcodes or access unimplemented CSRs in the standard or reserved encoding spaces raises an illegal instruction exception that results in a contained trap to the supervisor-mode trap handler. | 此外，尝试执行未实现的操作码或访问标准或保留编码空间中未实现的 CSR 会触发非法指令异常，导致陷入监管模式陷阱处理程序。 |

## Sstc - 监管模式定时器中断

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sstc_purpose` | This extension serves to provide supervisor mode with its own CSR-based timer interrupt facility that it can directly manage to provide its own timer service (in the form of having its own `stimecmp` register) - thus eliminating the large overheads for emulating S/HS-mode timers and timer interrupt generation up in M-mode. | 此扩展旨在为监管模式提供其自己的基于 CSR 的定时器中断设施，使其能直接管理以提供自己的定时器服务（拥有自己的 `stimecmp` 寄存器），从而消除在 M 模式下模拟 S/HS 模式定时器和定时器中断生成的巨大开销。 |
| `norm:sstc_vs_facility` | Further, this extension adds a similar facility to the Hypervisor extension for VS-mode. | 此外，此扩展为 Hypervisor 扩展的 VS 模式添加了类似设施。 |
| `norm:stimecmp_exist` | This extension adds the S-level `stimecmp` CSR. | 此扩展添加了 S 级 `stimecmp` CSR。 |
| `norm:vstimecmp_exist` | This extension adds the VS-level `vstimecmp` CSR. | 此扩展添加了 VS 级 `vstimecmp` CSR。 |
| `norm:stce_bit_exist` | This extension adds the `STCE` bit to the `menvcfg` and `henvcfg` CSRs. | 此扩展向 `menvcfg` 和 `henvcfg` CSR 添加了 `STCE` 位。 |

## Sscofpmf - 计数溢出与模式过滤

### 计数溢出控制

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mhpmevent_inh_op` | Each of the five `x`INH bits, when set, inhibit counting of events while in privilege mode `x`. All-zeroes for these bits results in counting of events in all modes. | 五个 `x`INH 位中的每一个，当置位时，禁止在特权模式 `x` 下计数事件。这些位全零时在所有模式下计数事件。 |
| `norm:mhpmevent_of_op` | The OF bit is set when the corresponding hpmcounter overflows, and remains set until written by software. | 当对应的 hpmcounter 溢出时 OF 位置位，并保持置位直到被软件写入。 |
| `norm:hpmcounter_overflow` | Since hpmcounter values are unsigned values, overflow is defined as unsigned overflow of the implemented counter bits. Note that there is no loss of information after an overflow since the counter wraps around and keeps counting while the sticky OF bit remains set. | 由于 hpmcounter 值是无符号值，溢出定义为已实现计数器位的无符号溢出。注意溢出后没有信息丢失，因为计数器会回绕并继续计数，同时粘性 OF 位保持置位。 |
| `norm:count_overflow_interrupt` | If an hpmcounter overflows while the associated OF bit is zero, then a "count overflow interrupt request" is generated. If the OF bit is one, then no interrupt request is generated. Consequently the OF bit also functions as a count overflow interrupt disable for the associated hpmcounter. | 如果 hpmcounter 在关联 OF 位为零时溢出，则生成"计数溢出中断请求"。如果 OF 位为 1，则不生成中断请求。因此 OF 位也充当关联 hpmcounter 的计数溢出中断禁用位。 |
| `norm:count_overflow_trigger` | Count overflow never results from writes to the mhpmcounter_n or mhpmevent_n registers, only from hardware increments of counter registers. | 计数溢出永远不会由对 mhpmcounter_n 或 mhpmevent_n 寄存器的写入引起，仅由计数器寄存器的硬件递增引起。 |
| `norm:mhpmevent_of_bit_set` | Generation of a count-overflow-interrupt request by an `hpmcounter` sets the associated OF bit. | `hpmcounter` 生成计数溢出中断请求会设置关联的 OF 位。 |
| `norm:LCOFIP_op` | When an OF bit is set, it eventually, but not necessarily immediately, sets the LCOFIP bit in the `mip`/`sip` registers. | 当 OF 位置位时，它最终（但不一定立即）设置 `mip`/`sip` 寄存器中的 LCOFIP 位。 |

### Supervisor Count Overflow (scountovf) 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:scountovf_op` | This extension adds the `scountovf` CSR, a 32-bit read-only register that contains shadow copies of the OF bits in the 29 mhpmevent CSRs (mhpmevent_3 - mhpmevent_31) - where scountovf bit X corresponds to mhpmevent_X. | 此扩展添加了 `scountovf` CSR，这是一个 32 位只读寄存器，包含 29 个 mhpmevent CSR（mhpmevent_3 - mhpmevent_31）中 OF 位的影子副本——其中 scountovf 位 X 对应 mhpmevent_X。 |
| `norm:scountovf_smode_read_access_control` | Read access to bit X is subject to the same mcounteren (or mcounteren and hcounteren) CSRs that mediate access to the hpmcounter CSRs by S-mode (or VS-mode). | 对位 X 的读取访问受与 S 模式（或 VS 模式）访问 hpmcounter CSR 相同的 mcounteren（或 mcounteren 和 hcounteren）CSR 控制。 |
| `norm:scountovf_mmode_read_access` | In M-mode, scountovf bit X is always readable. | 在 M 模式下，scountovf 位 X 始终可读。 |
| `norm:scountovf_smode_read_access` | In S/HS-mode, scountovf bit X is readable when mcounteren bit X is set, and otherwise reads as zero. | 在 S/HS 模式下，当 mcounteren 位 X 置位时 scountovf 位 X 可读，否则读为零。 |
| `norm:scountovf_vsmode_read_access` | Similarly, in VS mode, scountovf bit X is readable when mcounteren bit X and hcounteren bit X are both set, and otherwise reads as zero. | 类似地，在 VS 模式下，当 mcounteren 位 X 和 hcounteren 位 X 都置位时 scountovf 位 X 可读，否则读为零。 |

## Ssdbltrp - 双重陷阱扩展

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:HS_mode_invoke_error` | It enables HS-mode to invoke a critical error handler in a virtual machine on a double trap in VS-mode. | 它使 HS 模式能够在 VS 模式发生双重陷阱时调用虚拟机中的关键错误处理程序。 |
| `norm:M_mode_invoke_error` | It also allows M-mode to invoke a critical error handler in the OS/Hypervisor on a double trap in S/HS-mode. | 它还允许 M 模式在 S/HS 模式发生双重陷阱时调用 OS/Hypervisor 中的关键错误处理程序。 |
| `norm:menvcfg_DTE` | The Ssdbltrp extension adds the `menvcfg`.DTE. | Ssdbltrp 扩展添加了 `menvcfg`.DTE。 |
| `norm:sstatus_SDT` | The Ssdbltrp extension adds the `sstatus`.SDT fields. | Ssdbltrp 扩展添加了 `sstatus`.SDT 字段。 |
| `norm:henvcfg_DTE` | If the hypervisor extension is additionally implemented, then the extension adds the `henvcfg`.DTE. | 如果还实现了 hypervisor 扩展，则该扩展添加 `henvcfg`.DTE。 |
| `norm:vsstatus_SDT` | If the hypervisor extension is additionally implemented, then the extension adds the `vsstatus`.SDT fields. | 如果还实现了 hypervisor 扩展，则该扩展添加 `vsstatus`.SDT 字段。 |
