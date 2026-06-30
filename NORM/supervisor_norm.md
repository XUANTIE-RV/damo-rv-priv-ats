# Supervisor Norm 规范条目表

> 来源：`SPEC/supervisor.adoc` — Supervisor-Level ISA 章节

## sstatus 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sstatus` | The `sstatus` register is an SXLEN-bit read/write register. The `sstatus` register keeps track of the processor's current operating state. | `sstatus` 是 SXLEN 位读写寄存器，用于跟踪处理器当前运行状态。 |
| `norm:sstatus_spp` | The SPP bit indicates the privilege level at which a hart was executing before entering supervisor mode. When a trap is taken, SPP is set to 0 if the trap originated from user mode, or 1 otherwise. When an SRET instruction is executed to return from the trap handler, the privilege level is set to user mode if the SPP bit is 0, or supervisor mode if the SPP bit is 1; SPP is then set to 0. | SPP 位指示进入 S 模式前 hart 的特权级。陷入时：来自 U 模式则 SPP=0，否则 SPP=1。SRET 返回时：SPP=0 返回 U 模式，SPP=1 返回 S 模式，然后 SPP 清零。 |
| `norm:sstatus_sie` | The SIE bit enables or disables all interrupts in supervisor mode. When SIE is clear, interrupts are not taken while in supervisor mode. When the hart is running in user-mode, the value in SIE is ignored, and supervisor-level interrupts are enabled. | SIE 位控制 S 模式下所有中断的使能。SIE=0 时 S 模式不接收中断。U 模式下 SIE 被忽略，S 级中断始终使能。 |
| `norm:sstatus_spie` | The SPIE bit indicates whether supervisor interrupts were enabled prior to trapping into supervisor mode. When a trap is taken into supervisor mode, SPIE is set to SIE, and SIE is set to 0. When an SRET instruction is executed, SIE is set to SPIE, then SPIE is set to 1. | SPIE 位保存陷入 S 模式前中断是否使能。陷入时：SPIE←SIE，SIE←0。SRET 时：SIE←SPIE，SPIE←1。 |
| `norm:sstatus_uxl` | The UXL field controls the value of XLEN for U-mode, termed UXLEN, which may differ from the value of XLEN for S-mode, termed SXLEN. The encoding of UXL is the same as that of the MXL field of `misa`. | UXL 字段控制 U 模式的 XLEN（称为 UXLEN），可与 S 模式的 SXLEN 不同。编码方式与 `misa` 的 MXL 字段相同。 |
| `norm:sstatus_uxl_sz` | When SXLEN=32, the UXL field does not exist, and UXLEN=32. When SXLEN=64, it is a WARL field that encodes the current value of UXLEN. An implementation may make UXL be a read-only field whose value always ensures that UXLEN=SXLEN. | SXLEN=32 时 UXL 不存在，UXLEN=32。SXLEN=64 时 UXL 是 WARL 字段。实现可将 UXL 设为只读，始终保证 UXLEN=SXLEN。 |
| `norm:sstatus_uxl_behavior` | If UXLEN≠SXLEN, instructions executed in the narrower mode must ignore source register operand bits above the configured XLEN, and must sign-extend results to fill the widest supported XLEN in the destination register. If UXLEN < SXLEN, user-mode instruction-fetch addresses and load and store effective addresses are taken modulo 2^UXLEN. | UXLEN≠SXLEN 时，窄模式指令必须忽略源寄存器中超出 XLEN 的高位，结果符号扩展至最宽 XLEN。UXLEN<SXLEN 时，U 模式地址取模 2^UXLEN。 |
| `norm:hint_sxlen` | Some HINT instructions are encoded as integer computational instructions that overwrite their destination register with its current value. When such a HINT is executed with XLEN < SXLEN and bits SXLEN..XLEN of the destination register not all equal to bit XLEN-1, it is implementation-defined whether bits SXLEN..XLEN of the destination register are unchanged or are overwritten with copies of bit XLEN-1. | 某些 HINT 指令编码为覆盖目标寄存器自身值的整数运算指令。XLEN<SXLEN 时，目标寄存器高位是保持不变还是被 XLEN-1 位的副本覆盖，由实现决定。 |
| `norm:sstatus_mxr` | The MXR (Make eXecutable Readable) bit modifies the privilege with which loads access virtual memory. When MXR=0, only loads from pages marked readable (R=1) will succeed. When MXR=1, loads from pages marked either readable or executable (R=1 or X=1) will succeed. MXR has no effect when page-based virtual memory is not in effect. | MXR 位修改 load 访问虚拟内存的权限。MXR=0 时仅从可读页面加载成功；MXR=1 时从可读或可执行页面加载均成功。未启用分页时无效。 |
| `norm:sstatus_sum` | The SUM (permit Supervisor User Memory access) bit modifies the privilege with which S-mode loads and stores access virtual memory. When SUM=0, S-mode memory accesses to pages that are accessible by U-mode (U=1) will fault. When SUM=1, these accesses are permitted. SUM has no effect when page-based virtual memory is not in effect, nor when executing in U-mode. Note that S-mode can never execute instructions from user pages, regardless of the state of SUM. | SUM 位控制 S 模式对 U 模式页面的访问。SUM=0 时 S 模式访问 U=1 页面会出错；SUM=1 时允许。未启用分页或 U 模式下无效。S 模式永远不能执行用户页面的指令。 |
| `norm:sstatus_sum_satp_mode` | SUM is read-only 0 if `satp`.MODE is read-only 0. | 如果 `satp`.MODE 为只读 0，则 SUM 为只读 0。 |
| `norm:sstatus_ube` | The UBE bit is a WARL field that controls the endianness of explicit memory accesses made from U-mode, which may differ from the endianness of memory accesses in S-mode. An implementation may make UBE be a read-only field that always specifies the same endianness as for S-mode. UBE=0 means little-endian, UBE=1 means big-endian. | UBE 是 WARL 字段，控制 U 模式显式内存访问的字节序，可与 S 模式不同。UBE=0 小端，UBE=1 大端。实现可将其设为只读，始终与 S 模式字节序相同。 |
| `norm:sstatus_ube_implicit` | UBE has no effect on instruction fetches, which are implicit memory accesses that are always little-endian. For implicit accesses to supervisor-level memory management data structures, such as page tables, S-mode endianness always applies and UBE is ignored. | UBE 不影响取指（始终小端）。对页表等 S 级内存管理数据结构的隐式访问始终使用 S 模式字节序，UBE 被忽略。 |
| `norm:sstatus_spelp` | Access to the `SPELP` field, added by Zicfilp, accesses the homonymous fields of `mstatus` when `V=0`, and the homonymous fields of `vsstatus` when `V=1`. | Zicfilp 添加的 SPELP 字段：V=0 时访问 `mstatus` 的同名字段，V=1 时访问 `vsstatus` 的同名字段。 |
| `norm:sstatus_sdt` | The S-mode-disable-trap (`SDT`) bit is a WARL field introduced by the Ssdbltrp extension to address double trap at privilege modes lower than M. | SDT 位是 Ssdbltrp 扩展引入的 WARL 字段，用于处理 M 模式以下特权级的双重陷阱。 |
| `norm:sstatus_sdt_sstatus_sie_overwrite` | When the `SDT` bit is set to 1 by an explicit CSR write, the `SIE` bit is cleared to 0. This clearing occurs regardless of the value written, if any, to the `SIE` bit by the same write. The `SIE` bit can only be set to 1 by an explicit CSR write if the `SDT` bit is being set to 0 by the same write or is already 0. | 显式 CSR 写入将 SDT 置 1 时，SIE 被清零（无论同次写入对 SIE 写了什么值）。SIE 只能在同次写入将 SDT 清零或 SDT 已为 0 时才能被写为 1。 |
| `norm:sstatus_sdt_trap` | When a trap is to be taken into S-mode, if the `SDT` bit is currently 0, it is then set to 1, and the trap is delivered as expected. However, if `SDT` is already set to 1, then this is an unexpected trap. In the event of an unexpected trap, a double-trap exception trap is delivered into M-mode. | 陷入 S 模式时：若 SDT=0，则 SDT 置 1 并正常传递陷阱；若 SDT 已为 1，则为意外陷阱，双重陷阱异常被传递到 M 模式。 |
| `norm:sstatus_sdt_sret` | An `SRET` instruction sets the `SDT` bit to 0. | SRET 指令将 SDT 位清零。 |

## stvec 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:stvec` | The `stvec` register is an SXLEN-bit read/write register that holds trap vector configuration, consisting of a vector base address (BASE) and a vector mode (MODE). | `stvec` 是 SXLEN 位读写寄存器，保存陷阱向量配置，包含向量基地址（BASE）和向量模式（MODE）。 |
| `norm:stvec_op` | The BASE field in `stvec` is a field that can hold any valid virtual or physical address, subject to the following alignment constraints: the address must be 4-byte aligned, and MODE settings other than Direct might impose additional alignment constraints on the value in the BASE field. | BASE 字段可保存任何有效虚拟或物理地址，但必须 4 字节对齐，非 Direct 模式可能施加更严格的对齐约束。 |
| `norm:stvec_sz_base` | The CSR contains only bits XLEN-1 through 2 of the address BASE. When used as an address, the lower two bits are filled with zeroes to obtain an XLEN-bit address that is always aligned on a 4-byte boundary. | CSR 仅保存 BASE 地址的 [XLEN-1:2] 位。用作地址时低两位填零，获得始终 4 字节对齐的 XLEN 位地址。 |

## 中断 (sip/sie)

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sip_sie` | The `sip` register is an SXLEN-bit read/write register containing information on pending interrupts, while `sie` is the corresponding SXLEN-bit read/write register containing interrupt enable bits. Interrupt cause number _i_ corresponds with bit _i_ in both `sip` and `sie`. Bits 15:0 are allocated to standard interrupt causes only, while bits 16 and above are designated for platform use. | `sip` 保存待处理中断信息，`sie` 保存中断使能位。中断原因号 i 对应两个寄存器中的第 i 位。位 15:0 分配给标准中断，位 16 及以上用于平台。 |
| `norm:sie_sip_supervisor_strap` | An interrupt _i_ will trap to S-mode if both of the following are true: (a) either the current privilege mode is S and the SIE bit in the `sstatus` register is set, or the current privilege mode has less privilege than S-mode; and (b) bit _i_ is set in both `sip` and `sie`. | 中断 i 陷入 S 模式的条件：(a) 当前为 S 模式且 SIE=1，或当前特权级低于 S；且 (b) `sip` 和 `sie` 的第 i 位均置位。 |
| `norm:sie_sip_strap_time_constraint` | These conditions for an interrupt trap to occur must be evaluated in a bounded amount of time from when an interrupt becomes, or ceases to be, pending in `sip`, and must also be evaluated immediately following the execution of an SRET instruction or an explicit write to a CSR on which these interrupt trap conditions expressly depend (including `sip`, `sie` and `sstatus`). | 中断陷阱条件必须在 `sip` 中中断变为待处理/取消待处理后的有界时间内评估，且必须在 SRET 执行后或对相关 CSR（`sip`、`sie`、`sstatus`）显式写入后立即评估。 |
| `norm:sip_acc` | Each individual bit in register `sip` may be writable or may be read-only. | `sip` 中每个位可以是可写的或只读的。 |
| `norm:sip_op` | When bit _i_ in `sip` is writable, a pending interrupt _i_ can be cleared by writing 0 to this bit. If interrupt _i_ can become pending but bit _i_ in `sip` is read-only, the implementation must provide some other mechanism for clearing the pending interrupt (which may involve a call to the execution environment). | `sip` 位 i 可写时，写 0 可清除待处理中断 i。若中断 i 可变为待处理但 `sip` 位 i 只读，实现必须提供其他清除机制（可能涉及调用执行环境）。 |
| `norm:sie_acc` | A bit in `sie` must be writable if the corresponding interrupt can ever become pending. Bits of `sie` that are not writable are read-only zero. | 若对应中断可变为待处理，`sie` 中该位必须可写。不可写的位为只读零。 |
| `norm:s_interrupt_priority` | Interrupts to S-mode take priority over any interrupts to lower privilege modes. | S 模式中断优先于低特权级中断。 |
| `norm:sip_sie_bits_sz` | The standard portions (bits 15:0) of registers `sip` and `sie` are formatted as shown in the standard figures. | `sip` 和 `sie` 的标准部分（位 15:0）按标准格式定义。 |
| `norm:sip_seip_sie_seie` | Bits `sip`.SEIP and `sie`.SEIE are the interrupt-pending and interrupt-enable bits for supervisor-level external interrupts. If implemented, SEIP is read-only in `sip`, and is set and cleared by the execution environment. | SEIP/SEIE 是 S 级外部中断的待处理/使能位。若实现，SEIP 在 `sip` 中为只读，由执行环境设置和清除。 |
| `norm:sip_stip_sie_stie` | Bits `sip`.STIP and `sie`.STIE are the interrupt-pending and interrupt-enable bits for supervisor-level timer interrupts. If implemented, STIP is read-only in `sip`. When Sstc is not implemented, STIP is set and cleared by the execution environment. When Sstc is implemented, STIP reflects the timer interrupt signal resulting from `stimecmp`. | STIP/STIE 是 S 级计时器中断的待处理/使能位。STIP 只读。未实现 Sstc 时由执行环境控制；实现 Sstc 时 STIP 反映 `stimecmp` 产生的计时器中断信号。 |
| `norm:sip_ssip_sie_ssie` | Bits `sip`.SSIP and `sie`.SSIE are the interrupt-pending and interrupt-enable bits for supervisor-level software interrupts. If implemented, SSIP is writable in `sip` and may also be set to 1 by a platform-specific interrupt controller. | SSIP/SSIE 是 S 级软件中断的待处理/使能位。若实现，SSIP 在 `sip` 中可写，也可由平台中断控制器置 1。 |
| `norm:sip_sie_Sscofpmf` | If the Sscofpmf extension is implemented, bits `sip`.LCOFIP and `sie`.LCOFIE are the interrupt-pending and interrupt-enable bits for local-counter-overflow interrupts. LCOFIP is read-write in `sip`. If Sscofpmf is not implemented, LCOFIP and LCOFIE are read-only zeros. | 若实现 Sscofpmf，LCOFIP/LCOFIE 是本地计数器溢出中断的待处理/使能位，LCOFIP 可读写。未实现时为只读零。 |
| `norm:std_super_intr_impl` | Each standard interrupt type (SEI, STI, SSI, or LCOFI) may not be implemented, in which case the corresponding interrupt-pending and interrupt-enable bits are read-only zeros. | 每种标准中断类型（SEI、STI、SSI、LCOFI）可能未实现，此时对应的待处理和使能位为只读零。 |
| `norm:sip_sie_warl` | All bits in `sip` and `sie` are WARL fields. The implemented interrupts may be found by writing one to every bit location in `sie`, then reading back to see which bit positions hold a one. | `sip` 和 `sie` 所有位均为 WARL 字段。可通过向 `sie` 每位写 1 后读回确定已实现的中断。 |
| `norm:sip_sie_priority_bit_order` | Multiple simultaneous interrupts destined for supervisor mode are handled in the following decreasing priority order: SEI, SSI, STI, LCOFI. | 多个同时的 S 模式中断按降序优先级处理：SEI、SSI、STI、LCOFI。 |

## Supervisor 计时器与性能计数器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:supervisor_timer_scheduling` | The implementation must provide a facility for scheduling timer interrupts in terms of the real-time counter, `time`. | 实现必须提供基于实时计数器 `time` 调度计时器中断的机制。 |

## scounteren 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:scounteren` | The counter-enable (`scounteren`) CSR is a 32-bit register that controls the availability of the hardware performance monitoring counters to U-mode. | `scounteren` 是 32 位 CSR，控制 U 模式下硬件性能监控计数器的可用性。 |
| `norm:scounteren_op` | When the CY, TM, IR, or HPMn bit in the `scounteren` register is clear, attempts to read the `cycle`, `time`, `instret`, or `hpmcountern` register while executing in U-mode will cause an illegal-instruction exception. When one of these bits is set, access to the corresponding register is permitted. | `scounteren` 中 CY/TM/IR/HPMn 位清零时，U 模式读取对应计数器将导致非法指令异常。置位时允许访问。 |
| `norm:scounteren_acc` | `scounteren` must be implemented. However, any of the bits may be read-only zero, indicating reads to the corresponding counter will cause an exception when executing in U-mode. Hence, they are effectively WARL fields. | `scounteren` 必须实现。但任何位可以是只读零（U 模式读对应计数器将异常），因此实际为 WARL 字段。 |

## sscratch 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sscratch` | The `sscratch` CSR is an SXLEN-bit read/write register, dedicated for use by the supervisor. Typically, `sscratch` is used to hold a pointer to the hart-local supervisor context while the hart is executing user code. | `sscratch` 是 SXLEN 位读写寄存器，专供 S 模式使用。通常用于在 hart 执行 U 模式代码时保存指向 hart 本地 S 模式上下文的指针。 |

## sepc 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sepc` | `sepc` is an SXLEN-bit read/write CSR. The low bit of `sepc` (`sepc[0]`) is always zero. On implementations that support only IALIGN=32, the two low bits (`sepc[1:0]`) are always zero. | `sepc` 是 SXLEN 位读写 CSR。最低位始终为零。仅支持 IALIGN=32 的实现中，低两位始终为零。 |
| `norm:sepc_op_mask_ialign32` | If an implementation allows IALIGN to be either 16 or 32, then, whenever IALIGN=32, bit `sepc[1]` is masked on reads so that it appears to be 0. This masking occurs also for the implicit read by the SRET instruction. Though masked, `sepc[1]` remains writable when IALIGN=32. | 若实现允许 IALIGN 为 16 或 32，当 IALIGN=32 时，`sepc[1]` 读时被屏蔽为 0（包括 SRET 隐式读取），但该位仍可写。 |
| `norm:sepc_acc_invalid_addr` | `sepc` is a WARL register that must be able to hold all valid virtual addresses. It need not be capable of holding all possible invalid addresses. Prior to writing `sepc`, implementations may convert an invalid address into some other invalid address that `sepc` is capable of holding. | `sepc` 是 WARL 寄存器，必须能保存所有有效虚拟地址，不必能保存所有无效地址。写入前实现可将无效地址转换为 `sepc` 能保存的其他无效地址。 |
| `norm:sepc_op_trap_write` | When a trap is taken into S-mode, `sepc` is written with the virtual address of the instruction that was interrupted or that encountered the exception. Otherwise, `sepc` is never written by the implementation, though it may be explicitly written by software. | 陷入 S 模式时，`sepc` 被写入被中断或遇到异常的指令的虚拟地址。否则实现不写入 `sepc`，但软件可显式写入。 |

## scause 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:scause` | The `scause` CSR is an SXLEN-bit read-write register. When a trap is taken into S-mode, `scause` is written with a code indicating the event that caused the trap. Otherwise, `scause` is never written by the implementation, though it may be explicitly written by software. | `scause` 是 SXLEN 位读写寄存器。陷入 S 模式时写入导致陷阱的事件代码。否则实现不写入，但软件可显式写入。 |
| `norm:scause_interrupt` | The Interrupt bit in the `scause` register is set if the trap was caused by an interrupt. The Exception Code field contains a code identifying the last exception or interrupt. | `scause` 的中断位在陷阱由中断引起时置位。异常代码字段包含标识最近异常或中断的代码。 |
| `norm:scause_exception_code_acc` | The Exception Code is a WLRL field. | 异常代码是 WLRL 字段。 |
| `norm:scause_exception_code_sz` | It is required to hold the values 0–31 (i.e., bits 4–0 must be implemented), but otherwise it is only guaranteed to hold supported exception codes. | 必须能保存 0-31（即位 4-0 必须实现），但除此之外仅保证保存支持的异常代码。 |

## senvcfg 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:senvcfg_cbcfe` | The Zicbom extension adds the `CBCFE` field to `senvcfg` to control execution of the `CBO.CLEAN` and `CBO.FLUSH` instructions in U-mode. Execution is enabled only if enabled in S-mode and `CBCFE`=1; otherwise, an illegal-instruction exception is raised. When Zicbom is not implemented, `CBCFE` is read-only zero. | Zicbom 扩展在 `senvcfg` 中添加 CBCFE 字段，控制 U 模式下 CBO.CLEAN/CBO.FLUSH 的执行。仅当 S 模式启用且 CBCFE=1 时 U 模式才可执行，否则非法指令异常。未实现 Zicbom 时为只读零。 |
| `norm:senvcfg_cbie` | The Zicbom extension adds the `CBIE` WARL field to `senvcfg` to control execution of the `CBO.INVAL` instruction in U-mode. The encoding `10b` is reserved. When Zicbom is not implemented, `CBIE` is read-only zero. Execution is enabled only if enabled in S-mode and `CBIE` is `01b` or `11b`. | Zicbom 扩展添加 CBIE WARL 字段控制 U 模式下 CBO.INVAL 的执行。编码 `10b` 保留。未实现 Zicbom 时为只读零。仅当 S 模式启用且 CBIE=`01b` 或 `11b` 时 U 模式才可执行。 |
| `norm:cbo-inval_s-mode_op0` | If `CBO.INVAL` is enabled in S-mode to perform a flush operation, then when the instruction is enabled in U-mode it performs a flush operation, even if `CBIE` is set to `11b`. Otherwise: `01b` — performs a flush operation; `11b` — performs an invalidate operation. | 若 S 模式下 CBO.INVAL 执行刷新操作，则 U 模式下也执行刷新（即使 CBIE=`11b`）。否则按 CBIE 编码：`01b` 执行刷新，`11b` 执行无效化。 |
| `norm:senvcfg_pmm_Ssnpm` | If the Ssnpm extension is implemented, the `PMM` field enables or disables pointer masking for the next-lower privilege mode (U/VU). If Ssnpm is not implemented, `PMM` is read-only zero. The `PMM` field is read-only zero for RV32. | 若实现 Ssnpm，PMM 字段为下一低特权级（U/VU）启用或禁用指针掩码。未实现 Ssnpm 时为只读零。RV32 下 PMM 为只读零。 |
| `norm:senvcfg_lpe_Zicfilp` | The Zicfilp extension adds the `LPE` field in `senvcfg`. When `LPE`=1, Zicfilp is enabled in VU/U-mode. When `LPE`=0: the hart does not update the `ELP` state (remains `NO_LP_EXPECTED`); the `LPAD` instruction operates as a no-op. | Zicfilp 扩展在 `senvcfg` 中添加 LPE 字段。LPE=1 时 VU/U 模式启用 Zicfilp。LPE=0 时：hart 不更新 ELP 状态（保持 NO_LP_EXPECTED），LPAD 指令为空操作。 |
| `norm:senvcfg_sse_Zicfilp` | The Zicfiss extension adds the `SSE` field in `senvcfg`. When `SSE`=1, Zicfiss is activated in VU/U-mode. When `SSE`=0: 32-bit Zicfiss instructions revert to Zimop behavior; 16-bit Zicfiss instructions revert to Zcmop behavior; when `menvcfg.SSE` is one, `SSAMOSWAP.W/D` raises an illegal-instruction exception in U-mode and a virtual-instruction exception in VU-mode. | Zicfiss 扩展在 `senvcfg` 中添加 SSE 字段。SSE=1 时 VU/U 模式激活 Zicfiss。SSE=0 时：32 位 Zicfiss 指令回退为 Zimop 行为，16 位回退为 Zcmop 行为；menvcfg.SSE=1 时 SSAMOSWAP.W/D 在 U 模式引发非法指令异常，VU 模式引发虚拟指令异常。 |

## satp 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:satp` | The `satp` CSR is an SXLEN-bit read/write register which controls supervisor-mode address translation and protection. This register holds the physical page number (PPN) of the root page table, an address space identifier (ASID), and the MODE field which selects the current address-translation scheme. | `satp` 是 SXLEN 位读写寄存器，控制 S 模式地址转换和保护。保存根页表的物理页号（PPN）、地址空间标识符（ASID）和选择当前地址转换方案的 MODE 字段。 |
| `norm:satp_mode` | When MODE=Bare, supervisor virtual addresses are equal to supervisor physical addresses, and there is no additional memory protection. To select MODE=Bare, software must write zero to the remaining fields of `satp`. Attempting to select MODE=Bare with a nonzero pattern in the remaining fields has an UNSPECIFIED effect. | MODE=Bare 时，S 虚拟地址等于 S 物理地址，无额外内存保护。选择 Bare 模式时软件必须将 `satp` 其余字段写零。非零模式时效果未指定。 |
| `norm:svbare_satp_mode_bare` | If an implementation supports the Svbare extension, then the `satp` register's MODE field must be capable of holding the value Bare. | 若实现支持 Svbare 扩展，`satp` 的 MODE 字段必须能保存 Bare 值。 |
| `norm:satp_mode_sxlen32` | When SXLEN=32, the only other valid setting for MODE is Sv32. | SXLEN=32 时，MODE 唯一其他有效设置为 Sv32。 |
| `norm:satp_mode_sxlen64` | When SXLEN=64, three paged virtual-memory schemes are defined: Sv39, Sv48, and Sv57. One additional scheme, Sv64, will be defined in a later version. The remaining MODE settings are reserved for future use. | SXLEN=64 时定义了三种分页方案：Sv39、Sv48、Sv57。Sv64 将在后续版本定义。其余 MODE 设置保留。 |
| `norm:satp_op_sfence_vma` | Writing `satp` does not imply any ordering constraints between page-table updates and subsequent address translations, nor does it imply any invalidation of address-translation caches. If the new address space's page tables have been modified, or if an ASID is reused, it may be necessary to execute an SFENCE.VMA instruction after, or in some cases before, writing `satp`. | 写入 `satp` 不隐含页表更新与后续地址转换之间的顺序约束，也不隐含地址转换缓存的无效化。若页表被修改或 ASID 被重用，可能需要在写入 `satp` 后（或之前）执行 SFENCE.VMA。 |

## stimecmp 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:stimecmp_stimecmph_sz_acc` | The `stimecmp` CSR is a 64-bit register and has 64-bit precision on all RV32 and RV64 systems. In RV32 only, accesses to the `stimecmp` CSR access the low 32 bits, while accesses to the `stimecmph` CSR access the high 32 bits of `stimecmp`. | `stimecmp` 是 64 位寄存器，在 RV32 和 RV64 上均具有 64 位精度。仅 RV32 下：访问 `stimecmp` 访问低 32 位，访问 `stimecmph` 访问高 32 位。 |
| `norm:mip_sip_stip_op` | A supervisor timer interrupt becomes pending whenever `time` contains a value greater than or equal to `stimecmp`, treating the values as unsigned integers. If the result of this comparison changes, it is guaranteed to be reflected in STIP eventually, but not necessarily immediately. The interrupt remains posted until `stimecmp` becomes greater than `time`. | 当 `time` ≥ `stimecmp`（无符号比较）时 S 模式计时器中断变为待处理。比较结果变化保证最终反映在 STIP 中（不一定立即）。中断保持待处理直到 `stimecmp` > `time`。 |

## SFENCE.VMA 指令

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sfence_vma_op` | The supervisor memory-management fence instruction SFENCE.VMA is used to synchronize updates to in-memory memory-management data structures with current execution. | SFENCE.VMA 指令用于同步内存中内存管理数据结构的更新与当前执行。 |
| `norm:sfence_vma_ordering` | Executing an SFENCE.VMA instruction guarantees that any previous stores already visible to the current RISC-V hart are ordered before certain implicit references by subsequent instructions in that hart to the memory-management data structures. The specific set of operations ordered by SFENCE.VMA is determined by _rs1_ and _rs2_. | 执行 SFENCE.VMA 保证当前 hart 已可见的先前存储，排在后续对内存管理数据结构的隐式引用之前。具体排序的操作集由 rs1 和 rs2 决定。 |
| `norm:sfence_vma_invalidation` | SFENCE.VMA is also used to invalidate entries in the address-translation cache associated with a hart. | SFENCE.VMA 也用于无效化与 hart 关联的地址转换缓存条目。 |
| `norm:sfence_vma_all_asid_va` | If _rs1_=`x0` and _rs2_=`x0`, the fence orders all reads and writes made to any level of the page tables, for all address spaces. The fence also invalidates all address-translation cache entries, for all address spaces. | rs1=x0 且 rs2=x0 时：对所有地址空间的所有级别页表的读写进行排序，并无效化所有地址空间的所有地址转换缓存条目。 |
| `norm:sfence_vma_asid_only` | If _rs1_=`x0` and _rs2_≠`x0`, the fence orders all reads and writes made to any level of the page tables, but only for the address space identified by integer register _rs2_. Accesses to global mappings are not ordered. The fence also invalidates all address-translation cache entries matching the address space identified by _rs2_, except for entries containing global mappings. | rs1=x0 且 rs2≠x0 时：仅对 rs2 指定地址空间的所有级别页表读写排序，全局映射不排序。无效化匹配该地址空间的缓存条目（全局映射除外）。 |
| `norm:sfence_vma_va_all_asid` | If _rs1_≠`x0` and _rs2_=`x0`, the fence orders only reads and writes made to leaf page table entries corresponding to the virtual address in _rs1_, for all address spaces. The fence also invalidates all address-translation cache entries that contain leaf page table entries corresponding to the virtual address in _rs1_, for all address spaces. | rs1≠x0 且 rs2=x0 时：仅对 rs1 中虚拟地址对应的叶页表条目读写排序（所有地址空间）。无效化所有地址空间中包含该虚拟地址叶页表条目的缓存条目。 |
| `norm:sfence_vma_va_asid` | If _rs1_≠`x0` and _rs2_≠`x0`, the fence orders only reads and writes made to leaf page table entries corresponding to the virtual address in _rs1_, for the address space identified by _rs2_. Accesses to global mappings are not ordered. The fence also invalidates matching cache entries except for global mappings. | rs1≠x0 且 rs2≠x0 时：仅对 rs1 中虚拟地址在 rs2 指定地址空间中的叶页表条目读写排序，全局映射不排序。无效化匹配的缓存条目（全局映射除外）。 |
| `norm:sfence_vma_invalid_va` | If the value held in _rs1_ is not a valid virtual address, then the SFENCE.VMA instruction has no effect. No exception is raised in this case. | 若 rs1 中的值不是有效虚拟地址，SFENCE.VMA 无效，不引发异常。 |
| `norm:sfence_vma_rs2_bits` | When _rs2_≠`x0`, bits SXLEN-1:ASIDMAX of the value held in _rs2_ are reserved for future standard use. Until their use is defined, they should be zeroed by software and ignored by current implementations. If ASIDLEN<ASIDMAX, the implementation shall ignore bits ASIDMAX-1:ASIDLEN of _rs2_. | rs2≠x0 时，rs2 的 [SXLEN-1:ASIDMAX] 位保留。软件应将其清零，当前实现应忽略。若 ASIDLEN<ASIDMAX，实现应忽略 [ASIDMAX-1:ASIDLEN] 位。 |
| `norm:sfence_vma_ordering_semantics` | An implicit read of the memory-management data structures may return any translation for an address that was valid at any time since the most recent SFENCE.VMA that subsumes that address. | 对内存管理数据结构的隐式读取可返回自最近一次涵盖该地址的 SFENCE.VMA 以来任何时刻有效的地址转换。 |
| `norm:sfence_vma_implicit_access` | Implementations must only perform implicit reads of the translation data structures pointed to by the current contents of the `satp` register or a subsequent valid (V=1) translation data structure entry, and must only raise exceptions for implicit accesses that are generated as a result of instruction execution, not those that are performed speculatively. | 实现只能对 `satp` 当前内容指向的转换数据结构或后续有效（V=1）条目执行隐式读取，且仅对指令执行产生的隐式访问（非推测性访问）引发异常。 |
| `norm:sfence_vma_sum_mxr_effect` | Changes to the `sstatus` fields SUM and MXR take effect immediately, without the need to execute an SFENCE.VMA instruction. | `sstatus` 的 SUM 和 MXR 字段变更立即生效，无需执行 SFENCE.VMA。 |
| `norm:sfence_vma_mode_effect` | Changing `satp`.MODE from Bare to other modes and vice versa also takes effect immediately, without the need to execute an SFENCE.VMA instruction. | `satp`.MODE 在 Bare 与其他模式之间切换立即生效，无需执行 SFENCE.VMA。 |
| `norm:sfence_vma_asid_effect` | Likewise, changes to `satp`.ASID take effect immediately. | `satp`.ASID 的变更也立即生效。 |

## Sv32 地址转换与保护

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:satp_ppn_sv32_sz` | The 20-bit VPN is translated into a 22-bit physical page number (PPN), while the 12-bit page offset is untranslated. | Sv32 下 20 位 VPN 转换为 22 位 PPN，12 位页内偏移不转换。 |
| `norm:fetch_page_fault_no_x` | Attempting to fetch an instruction from a page that does not have execute permissions raises a fetch page-fault exception. | 从没有执行权限的页面取指会触发取指页错误异常。 |
| `norm:load_page_fault_no_r` | Attempting to execute a load, load-reserved, or cache-block management instruction whose effective address lies within a page without read permissions raises a load page-fault exception. | 对没有读权限的页面执行 load/load-reserved/缓存块管理指令会触发加载页错误异常。 |
| `norm:store_page_fault_no_w` | Attempting to execute a store, store-conditional, AMO, or cache-block zero instruction whose effective address lies within a page without write permissions raises a store page-fault exception. | 对没有写权限的页面执行 store/store-conditional/AMO/缓存块清零指令会触发存储页错误异常。 |
| `norm:svade_access_ad_bit_clear` | The Svade extension: when a virtual page is accessed and the A bit is clear, or is written and the D bit is clear, a page-fault exception is raised. | Svade 扩展：当虚拟页面被访问且 A 位为 0，或被写入且 D 位为 0 时，触发页错误异常。 |

## Sv39 虚拟内存系统

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Sv39_sxlen` | Sv39 is a simple paged virtual-memory system for SXLEN=64. | Sv39 是面向 SXLEN=64 的分页虚拟内存系统。 |
| `norm:Sv39_va_sz` | Sv39 supports 39-bit virtual address spaces. | Sv39 支持 39 位虚拟地址空间。 |
| `norm:Sv39_va_signext` | Instruction fetch addresses and load and store effective addresses, which are 64 bits, must have bits 63–39 all equal to bit 38, or else a page-fault exception will occur. | 64 位地址的位 63-39 必须全部等于位 38（符号扩展），否则触发页错误异常。 |
| `norm:Sv39_vpn_sz` | The VPN is 27 bits. | Sv39 的 VPN 为 27 位。 |
| `norm:satp_ppn_sv39_sz` | The PPN is 44 bits. | Sv39 的 PPN 为 44 位。 |
| `norm:Sv39_levels` | Sv39 uses a three-level page table. | Sv39 使用三级页表。 |
| `norm:Sv39_page_offset_sz` | The 12-bit page offset is untranslated. | Sv39 的 12 位页内偏移不转换。 |
| `norm:Sv39_pte_count` | Sv39 page tables contain 2^9 page table entries (PTEs). | Sv39 页表包含 2^9（512）个页表项。 |
| `norm:Sv39_pte_sz` | Each PTE is eight bytes. | Sv39 每个 PTE 为 8 字节。 |
| `norm:Sv39_pt_sz` | A page table is exactly the size of a page. | Sv39 页表大小恰好为一页。 |
| `norm:Sv39_pt_align` | A page table must always be aligned to a page boundary. | Sv39 页表必须始终按页边界对齐。 |
| `norm:Sv39_pte_svnapot_rsv` | If Svnapot is not implemented, bit 63 remains reserved and must be zeroed by software for forward compatibility, or else a page-fault exception is raised. | 若未实现 Svnapot，位 63 保留，软件必须清零以保证前向兼容，否则触发页错误异常。 |
| `norm:Sv39_pte_svpbmt_rsv` | If Svpbmt is not implemented, bits 62-61 remain reserved and must be zeroed by software for forward compatibility, or else a page-fault exception is raised. | 若未实现 Svpbmt，位 62-61 保留，软件必须清零以保证前向兼容，否则触发页错误异常。 |
| `norm:Sv39_pte_future_rsv` | Bits 60-54 are reserved for future standard use and must be zeroed by software for forward compatibility. If any of these bits are set, a page-fault exception is raised. | 位 60-54 保留供未来标准使用，软件必须清零以保证前向兼容。若任何位置位，触发页错误异常。 |
| `norm:Sv39_leaf_any_level` | Any level of PTE may be a leaf PTE. | Sv39 中任何级别的 PTE 都可以是叶 PTE。 |
| `norm:Sv39_page_sizes` | Sv39 supports 4 KiB pages, 2 MiB megapages, and 1 GiB gigapages. | Sv39 支持 4 KiB 页、2 MiB 大页和 1 GiB 超大页。 |
| `norm:Sv39_superpage_align` | A superpage must be virtually and physically aligned to a boundary equal to its size. | 超大页必须按与其大小相等的边界进行虚拟和物理对齐。 |
| `norm:Sv39_superpage_align_fault` | A page-fault exception is raised if the physical address is insufficiently aligned. | 若物理地址对齐不足，触发页错误异常。 |
| `norm:Sv39_LEVELS` | For Sv39, LEVELS equals 3. | Sv39 的 LEVELS 等于 3。 |
| `norm:Sv39_PTESIZE` | For Sv39, PTESIZE equals 8. | Sv39 的 PTESIZE 等于 8。 |

## Sv48 虚拟内存系统

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Sv48_sxlen` | Sv48 is a simple paged virtual-memory system for SXLEN=64. | Sv48 是面向 SXLEN=64 的分页虚拟内存系统。 |
| `norm:Sv48_va_sz` | Sv48 supports 48-bit virtual address spaces. | Sv48 支持 48 位虚拟地址空间。 |
| `norm:Sv48_requires_Sv39` | Implementations that support Sv48 must also support Sv39. | 支持 Sv48 的实现必须同时支持 Sv39。 |
| `norm:Sv48_va_signext` | Instruction fetch addresses and load and store effective addresses, which are 64 bits, must have bits 63–48 all equal to bit 47, or else a page-fault exception will occur. | 64 位地址的位 63-48 必须全部等于位 47（符号扩展），否则触发页错误异常。 |
| `norm:Sv48_vpn_sz` | The VPN is 36 bits. | Sv48 的 VPN 为 36 位。 |
| `norm:satp_ppn_sv48_sz` | The PPN is 44 bits. | Sv48 的 PPN 为 44 位。 |
| `norm:Sv48_levels` | Sv48 uses a four-level page table. | Sv48 使用四级页表。 |
| `norm:Sv48_page_offset_sz` | The 12-bit page offset is untranslated. | Sv48 的 12 位页内偏移不转换。 |
| `norm:Sv48_leaf_any_level` | Any level of PTE may be a leaf PTE. | Sv48 中任何级别的 PTE 都可以是叶 PTE。 |
| `norm:Sv48_page_sizes` | Sv48 supports 4 KiB pages, 2 MiB megapages, 1 GiB gigapages, and 512 GiB terapages. | Sv48 支持 4 KiB 页、2 MiB 大页、1 GiB 超大页和 512 GiB 特大页。 |
| `norm:Sv48_superpage_align` | A superpage must be virtually and physically aligned to a boundary equal to its size. | 超大页必须按与其大小相等的边界进行虚拟和物理对齐。 |
| `norm:Sv48_superpage_align_fault` | A page-fault exception is raised if the physical address is insufficiently aligned. | 若物理地址对齐不足，触发页错误异常。 |
| `norm:Sv48_LEVELS` | For Sv48, LEVELS equals 4. | Sv48 的 LEVELS 等于 4。 |
| `norm:Sv48_PTESIZE` | For Sv48, PTESIZE equals 8. | Sv48 的 PTESIZE 等于 8。 |

## Sv57 虚拟内存系统

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Sv57_sxlen` | Sv57 is a simple paged virtual-memory system designed for RV64 systems. | Sv57 是面向 RV64 系统的分页虚拟内存系统。 |
| `norm:Sv57_va_sz` | Sv57 supports 57-bit virtual address spaces. | Sv57 支持 57 位虚拟地址空间。 |
| `norm:Sv57_requires_Sv48` | Implementations that support Sv57 must also support Sv48. | 支持 Sv57 的实现必须同时支持 Sv48。 |
| `norm:Sv57_va_signext` | Instruction fetch addresses and load and store effective addresses, which are 64 bits, must have bits 63–57 all equal to bit 56, or else a page-fault exception will occur. | 64 位地址的位 63-57 必须全部等于位 56（符号扩展），否则触发页错误异常。 |
| `norm:Sv57_vpn_sz` | The VPN is 45 bits. | Sv57 的 VPN 为 45 位。 |
| `norm:satp_ppn_sv57_sz` | The PPN is 44 bits. | Sv57 的 PPN 为 44 位。 |
| `norm:Sv57_levels` | Sv57 uses a five-level page table. | Sv57 使用五级页表。 |
| `norm:Sv57_page_offset_sz` | The 12-bit page offset is untranslated. | Sv57 的 12 位页内偏移不转换。 |
| `norm:Sv57_leaf_any_level` | Any level of PTE may be a leaf PTE. | Sv57 中任何级别的 PTE 都可以是叶 PTE。 |
| `norm:Sv57_page_sizes` | Sv57 supports 4 KiB pages, 2 MiB megapages, 1 GiB gigapages, 512 GiB terapages, and 256 TiB petapages. | Sv57 支持 4 KiB 页、2 MiB 大页、1 GiB 超大页、512 GiB 特大页和 256 TiB 巨大页。 |
| `norm:Sv57_superpage_align` | A superpage must be virtually and physically aligned to a boundary equal to its size. | 超大页必须按与其大小相等的边界进行虚拟和物理对齐。 |
| `norm:Sv57_superpage_align_fault` | A page-fault exception is raised if the physical address is insufficiently aligned. | 若物理地址对齐不足，触发页错误异常。 |
| `norm:Sv57_LEVELS` | For Sv57, LEVELS equals 5. | Sv57 的 LEVELS 等于 5。 |
| `norm:Sv57_PTESIZE` | For Sv57, PTESIZE equals 8. | Sv57 的 PTESIZE 等于 8。 |
