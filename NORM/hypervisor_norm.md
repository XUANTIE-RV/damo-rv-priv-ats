# Hypervisor Extension Norm 规范条目表

> 来源：`SPEC/hypervisor.adoc` — Extension for Hypervisor Support 章节

## 扩展依赖与基本要求

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:H_mtval_nrz` | CSR `mtval` must not be read-only zero. | CSR `mtval` 不得为只读零。 |
| `norm:H_vm_supported` | Standard page-based address translation must be supported, either Sv32 for RV32, or a minimum of Sv39 for RV64. | 必须支持标准的基于页的地址翻译，RV32 使用 Sv32，RV64 至少支持 Sv39。 |
| `norm:misa_h_op` | The hypervisor extension is enabled by setting bit 7 in the `misa` CSR. | 通过设置 `misa` CSR 的第 7 位来启用 hypervisor 扩展。 |

## Hypervisor 和 Virtual Supervisor CSR

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:H_csrs_hs_not_vs` | Additional CSRs are provided to HS-mode, but not to VS-mode, to manage two-stage address translation and to control the behavior of a VS-mode guest: `hstatus`, `hedeleg`, `hideleg`, `hvip`, `hip`, `hie`, `hgeip`, `hgeie`, `henvcfg`, `henvcfgh`, `hcounteren`, `htimedelta`, `htimedeltah`, `htval`, `htinst`, and `hgatp`. | 额外的 CSR 仅提供给 HS 模式，不提供给 VS 模式，用于管理两阶段地址翻译和控制 VS 模式客户机的行为。 |
| `norm:H_vscsrs_sub` | When V=1, the VS CSRs substitute for the corresponding supervisor CSRs, taking over all functions of the usual supervisor CSRs except as specified otherwise. Instructions that normally read or modify a supervisor CSR shall instead access the corresponding VS CSR. | 当 V=1 时，VS CSR 替代对应的 supervisor CSR，接管其所有功能（除非另有规定）。通常读写 supervisor CSR 的指令将改为访问对应的 VS CSR。 |
| `norm:H_vscsrs_acc_vs` | When V=1, an attempt to read or write a VS CSR directly by its own separate CSR address causes a virtual-instruction exception. | 当 V=1 时，尝试通过 VS CSR 自身的独立 CSR 地址直接读写它会引发虚拟指令异常。 |
| `norm:H_vscsrs_acc_u` | Attempts from U-mode cause an illegal-instruction exception as usual. | 来自 U 模式的访问尝试照常引发非法指令异常。 |
| `norm:H_vscsrs_acc_m_hs` | The VS CSRs can be accessed as themselves only from M-mode or HS-mode. | VS CSR 只能从 M 模式或 HS 模式以其自身地址访问。 |
| `norm:H_vscsrs_v1` | While V=1, the normal HS-level supervisor CSRs that are replaced by VS CSRs retain their values but do not affect the behavior of the machine unless specifically documented to do so. | 当 V=1 时，被 VS CSR 替代的普通 HS 级 supervisor CSR 保留其值，但不影响机器行为（除非特别说明）。 |
| `norm:H_vscsrs_v0` | When V=0, the VS CSRs do not ordinarily affect the behavior of the machine other than being readable and writable by CSR instructions. | 当 V=0 时，VS CSR 通常不影响机器行为，仅可被 CSR 指令读写。 |
| `norm:H_scsrs_nomatch` | Some standard supervisor CSRs (`senvcfg`, `scounteren`, and `scontext`, possibly others) have no matching VS CSR. These supervisor CSRs continue to have their usual function and accessibility even when V=1, except with VS-mode and VU-mode substituting for HS-mode and U-mode. | 某些标准 supervisor CSR（如 `senvcfg`、`scounteren`、`scontext` 等）没有对应的 VS CSR。这些 CSR 即使在 V=1 时也保持原有功能和可访问性，只是 VS 模式和 VU 模式分别替代 HS 模式和 U 模式。 |
| `norm:hsxlen` | We use the term HSXLEN to refer to the effective XLEN when executing in HS-mode. | HSXLEN 指在 HS 模式下执行时的有效 XLEN。 |
| `norm:vsxlen` | VSXLEN to refer to the effective XLEN when executing in VS-mode. | VSXLEN 指在 VS 模式下执行时的有效 XLEN。 |

## hstatus 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:hstatus_sz_acc_op` | The `hstatus` register is an HSXLEN-bit read/write register... provides facilities analogous to the `mstatus` register for tracking and controlling the exception behavior of a VS-mode guest. | `hstatus` 是一个 HSXLEN 位的读写寄存器，提供类似 `mstatus` 的功能，用于跟踪和控制 VS 模式客户机的异常行为。 |
| `norm:hstatus_vsxl_op` | The VSXL field controls the effective XLEN for VS-mode (known as VSXLEN), which may differ from the XLEN for HS-mode (HSXLEN). | VSXL 字段控制 VS 模式的有效 XLEN（即 VSXLEN），可以与 HS 模式的 XLEN（HSXLEN）不同。 |
| `norm:hstatus_vsxl_32` | When HSXLEN=32, the VSXL field does not exist, and VSXLEN=32. | 当 HSXLEN=32 时，VSXL 字段不存在，VSXLEN=32。 |
| `norm:hstatus_vsxl_64` | When HSXLEN=64, VSXL is a WARL field that is encoded the same as the MXL field of `misa`. | 当 HSXLEN=64 时，VSXL 是一个 WARL 字段，编码方式与 `misa` 的 MXL 字段相同。 |
| `norm:vsxl_ro` | In particular, an implementation may make VSXL be a read-only field whose value always ensures that VSXLEN=HSXLEN. | 实现可以使 VSXL 为只读字段，其值始终确保 VSXLEN=HSXLEN。 |
| `norm:hstatus_vsxl_change` | If HSXLEN is changed from 32 to a wider width, and if field VSXL is not restricted to a single value, it gets the value corresponding to the widest supported width not wider than the new HSXLEN. | 如果 HSXLEN 从 32 变为更宽的宽度，且 VSXL 字段未限制为单一值，则它取不超过新 HSXLEN 的最宽支持宽度对应的值。 |
| `norm:hstatus_vtsr_op` | When VTSR=1, an attempt in VS-mode to execute SRET raises a virtual-instruction exception. | 当 VTSR=1 时，在 VS 模式下尝试执行 SRET 将引发虚拟指令异常。 |
| `norm:hstatus_vtw_op` | When VTW=1 (and assuming `mstatus`.TW=0), an attempt in VS-mode to execute WFI raises a virtual-instruction exception if the WFI does not complete within an implementation-specific, bounded time limit. | 当 VTW=1（且 `mstatus`.TW=0）时，在 VS 模式下执行 WFI 若未在实现特定的有限时间内完成，将引发虚拟指令异常。 |
| `norm:vtw_virtinstr` | An implementation may have WFI always raise a virtual-instruction exception in VS-mode when VTW=1 (and `mstatus`.TW=0), even if there are pending globally-disabled interrupts when the instruction is executed. | 当 VTW=1（且 `mstatus`.TW=0）时，实现可以使 WFI 在 VS 模式下始终引发虚拟指令异常，即使执行时存在全局禁用的待处理中断。 |
| `norm:hstatus_vtvm_op` | When VTVM=1, an attempt in VS-mode to execute SFENCE.VMA or SINVAL.VMA or to access CSR `satp` raises a virtual-instruction exception. | 当 VTVM=1 时，在 VS 模式下尝试执行 SFENCE.VMA 或 SINVAL.VMA 或访问 CSR `satp` 将引发虚拟指令异常。 |
| `norm:hstatus_vgein_op` | The VGEIN (Virtual Guest External Interrupt Number) field selects a guest external interrupt source for VS-level external interrupts. VGEIN is a WLRL field that must be able to hold values between zero and the maximum guest external interrupt number (known as GEILEN), inclusive. | VGEIN 字段选择 VS 级外部中断的客户外部中断源。VGEIN 是一个 WLRL 字段，必须能保持 0 到 GEILEN（含）之间的值。 |
| `norm:hstatus_hu_op` | Field HU (Hypervisor in U-mode) controls whether the virtual-machine load/store instructions, HLV, HLVX, and HSV, can be used also in U-mode. When HU=1, these instructions can be executed in U-mode the same as in HS-mode. When HU=0, all hypervisor instructions cause an illegal-instruction exception in U-mode. | HU 字段控制虚拟机 load/store 指令是否也可在 U 模式下使用。HU=1 时可在 U 模式下执行；HU=0 时所有 hypervisor 指令在 U 模式下引发非法指令异常。 |
| `norm:hstatus_spv_op` | The SPV bit (Supervisor Previous Virtualization mode) is written by the implementation whenever a trap is taken into HS-mode. Just as the SPP bit in `sstatus` is set to the (nominal) privilege mode at the time of the trap, the SPV bit in `hstatus` is set to the value of the virtualization mode V at the time of the trap. | SPV 位在每次进入 HS 模式的陷阱时由实现写入，设置为陷阱发生时虚拟化模式 V 的值。 |
| `norm:hstatus_spv_sret` | When an SRET instruction is executed when V=0, V is set to SPV. | 当 V=0 时执行 SRET 指令，V 被设置为 SPV。 |
| `norm:hstatus_spvp_op` | When V=1 and a trap is taken into HS-mode, bit SPVP (Supervisor Previous Virtual Privilege) is set to the nominal privilege mode at the time of the trap, the same as `sstatus`.SPP. But if V=0 before a trap, SPVP is left unchanged on trap entry. | 当 V=1 且陷阱进入 HS 模式时，SPVP 位设置为陷阱时的名义特权模式。若陷阱前 V=0，SPVP 在陷阱入口不变。 |
| `norm:hstatus_gva_op` | Field GVA (Guest Virtual Address) is written by the implementation whenever a trap is taken into HS-mode. For any trap that writes a guest virtual address to `stval`, GVA is set to 1. For any other trap into HS-mode, GVA is set to 0. | GVA 字段在进入 HS 模式的陷阱时由实现写入。写入客户虚拟地址到 `stval` 的陷阱设置 GVA=1，其他陷阱设置 GVA=0。 |
| `norm:hstatus_vsbe_op` | The VSBE bit is a WARL field that controls the endianness of explicit memory accesses made from VS-mode. | VSBE 位是一个 WARL 字段，控制 VS 模式下显式内存访问的字节序。 |

## hedeleg 和 hideleg 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:hedeleg_sz_acc` | Register `hedeleg` is a 64-bit read/write register. | `hedeleg` 是一个 64 位读写寄存器。 |
| `norm:hideleg_sz_acc` | Register `hideleg` is an HSXLEN-bit read/write register. | `hideleg` 是一个 HSXLEN 位读写寄存器。 |
| `norm:hedeleg_img` | Hypervisor exception delegation register (`hedeleg`) is formatted as shown in the specification figure. | `hedeleg` 寄存器格式如规范图所示。 |
| `norm:hideleg_img` | Hypervisor interrupt delegation register (`hideleg`) is formatted as shown in the specification figure. | `hideleg` 寄存器格式如规范图所示。 |
| `norm:hedeleg_op` | A synchronous trap that has been delegated to HS-mode (using `medeleg`) is further delegated to VS-mode if V=1 before the trap and the corresponding `hedeleg` bit is set. | 已通过 `medeleg` 委托给 HS 模式的同步陷阱，若陷阱前 V=1 且对应的 `hedeleg` 位已设置，则进一步委托给 VS 模式。 |
| `norm:hedeleg_acc` | Each bit of `hedeleg` shall be either writable or read-only zero. Many bits of `hedeleg` are required specifically to be writable or zero, as enumerated in the table. Bit 0, corresponding to instruction address-misaligned exceptions, must be writable if IALIGN=32. | `hedeleg` 的每一位要么可写要么为只读零。第 0 位（指令地址未对齐异常）在 IALIGN=32 时必须可写。 |
| `norm:hedelegh_sz_acc_op` | When XLEN=32, `hedelegh` is a 32-bit read/write register that aliases bits 63:32 of `hedeleg`. Register `hedelegh` does not exist when XLEN=64. | 当 XLEN=32 时，`hedelegh` 是 32 位读写寄存器，别名 `hedeleg` 的 63:32 位。XLEN=64 时不存在。 |
| `norm:hideleg_op` | An interrupt that has been delegated to HS-mode (using `mideleg`) is further delegated to VS-mode if the corresponding `hideleg` bit is set. | 已通过 `mideleg` 委托给 HS 模式的中断，若对应 `hideleg` 位已设置，则进一步委托给 VS 模式。 |
| `norm:hideleg_acc` | Among bits 15:0 of `hideleg`, bits 10, 6, and 2 (corresponding to the standard VS-level interrupts) are writable, and bits 12, 9, 5, and 1 (corresponding to the standard S-level interrupts) are read-only zeros. | `hideleg` 的 15:0 位中，第 10、6、2 位（标准 VS 级中断）可写，第 12、9、5、1 位（标准 S 级中断）为只读零。 |
| `norm:hideleg_trans` | When a virtual supervisor external interrupt (code 10) is delegated to VS-mode, it is automatically translated by the machine into a supervisor external interrupt (code 9) for VS-mode. Likewise, virtual supervisor timer interrupt (6) is translated into supervisor timer interrupt (5), and virtual supervisor software interrupt (2) into supervisor software interrupt (1). | 当虚拟 supervisor 外部中断（代码 10）被委托给 VS 模式时，自动翻译为 supervisor 外部中断（代码 9）。类似地，虚拟定时器中断(6)→定时器中断(5)，虚拟软件中断(2)→软件中断(1)。 |

## hvip、hip 和 hie 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:hvip_sz_op` | Register `hvip` is an HSXLEN-bit read/write register that a hypervisor can write to indicate virtual interrupts intended for VS-mode. Bits of `hvip` that are not writable are read-only zeros. | `hvip` 是一个 HSXLEN 位读写寄存器，hypervisor 可写入以指示用于 VS 模式的虚拟中断。不可写的位为只读零。 |
| `norm:hvip_img` | Hypervisor virtual-interrupt-pending register (`hvip`) is formatted as shown in the specification figure. | `hvip` 寄存器格式如规范图所示。 |
| `norm:hvip_acc` | The standard portion (bits 15:0) of `hvip` is formatted as shown. Bits VSEIP, VSTIP, and VSSIP of `hvip` are writable. | `hvip` 的标准部分（15:0 位）中，VSEIP、VSTIP 和 VSSIP 位可写。 |
| `norm:hip_hie_sz_acc` | Registers `hip` and `hie` are HSXLEN-bit read/write registers that supplement HS-level's `sip` and `sie` respectively. | `hip` 和 `hie` 是 HSXLEN 位读写寄存器，分别补充 HS 级的 `sip` 和 `sie`。 |
| `norm:hip_img` | Hypervisor interrupt-pending register (`hip`) is formatted as shown in the specification figure. | `hip` 寄存器格式如规范图所示。 |
| `norm:hie_img` | Hypervisor interrupt-enable register (`hie`) is formatted as shown in the specification figure. | `hie` 寄存器格式如规范图所示。 |
| `norm:hip_op` | The `hip` register indicates pending VS-level and hypervisor-specific interrupts. | `hip` 寄存器指示待处理的 VS 级和 hypervisor 特定中断。 |
| `norm:hie_op` | `hie` contains enable bits for the same interrupts. | `hie` 包含相同中断的使能位。 |
| `norm:sie_hip_hie_mutex` | For each writable bit in `sie`, the corresponding bit shall be read-only zero in both `hip` and `hie`. Hence, the nonzero bits in `sie` and `hie` are always mutually exclusive, and likewise for `sip` and `hip`. | 对于 `sie` 中的每个可写位，`hip` 和 `hie` 中的对应位必须为只读零。`sie` 和 `hie` 的非零位始终互斥，`sip` 和 `hip` 同理。 |
| `norm:hideleg_hs` | An interrupt _i_ will trap to HS-mode whenever all of the following are true: (a) either the current operating mode is HS-mode and the SIE bit in the `sstatus` register is set, or the current operating mode has less privilege than HS-mode; (b) bit _i_ is set in both `sip` and `sie`, or in both `hip` and `hie`; and (c) bit _i_ is not set in `hideleg`. | 中断 _i_ 在以下条件全部满足时陷入 HS 模式：(a) 当前为 HS 模式且 `sstatus`.SIE=1，或当前特权低于 HS 模式；(b) 位 _i_ 在 `sip`/`sie` 或 `hip`/`hie` 中都已设置；(c) 位 _i_ 在 `hideleg` 中未设置。 |
| `norm:hip_acc` | If bit _i_ of `sie` is read-only zero, the same bit in register `hip` may be writable or may be read-only. When bit _i_ in `hip` is writable, a pending interrupt _i_ can be cleared by writing 0 to this bit. | 若 `sie` 的位 _i_ 为只读零，`hip` 中的同一位可以是可写的或只读的。当 `hip` 中位 _i_ 可写时，可通过写 0 清除待处理中断 _i_。 |
| `norm:hie_acc` | A bit in `hie` shall be writable if the corresponding interrupt can ever become pending in `hip`. Bits of `hie` that are not writable shall be read-only zero. | 若对应中断可在 `hip` 中变为待处理，则 `hie` 中对应位必须可写。`hie` 中不可写的位必须为只读零。 |
| `norm:hip_sgeip_sgeie_acc_op` | Bits `hip`.SGEIP and `hie`.SGEIE are the interrupt-pending and interrupt-enable bits for guest external interrupts at supervisor level (HS-level). SGEIP is read-only in `hip`, and is 1 if and only if the bitwise logical-AND of CSRs `hgeip` and `hgeie` is nonzero in any bit. | `hip`.SGEIP 和 `hie`.SGEIE 是 HS 级客户外部中断的中断待处理和使能位。SGEIP 在 `hip` 中为只读，当且仅当 `hgeip` 与 `hgeie` 按位与的结果在任何位上非零时为 1。 |
| `norm:hip_vseip_vseie_op` | Bits `hip`.VSEIP and `hie`.VSEIE are the interrupt-pending and interrupt-enable bits for VS-level external interrupts. VSEIP is read-only in `hip`, and is the logical-OR of: bit VSEIP of `hvip`; the bit of `hgeip` selected by `hstatus`.VGEIN; and any other platform-specific external interrupt signal directed to VS-level. | `hip`.VSEIP 和 `hie`.VSEIE 是 VS 级外部中断的中断待处理和使能位。VSEIP 在 `hip` 中为只读，是 `hvip`.VSEIP、`hgeip` 中由 `hstatus`.VGEIN 选择的位、以及其他平台特定的 VS 级外部中断信号的逻辑或。 |
| `norm:hip_vstip_vstie_acc_op` | Bits `hip`.VSTIP and `hie`.VSTIE are the interrupt-pending and interrupt-enable bits for VS-level timer interrupts. VSTIP is read-only in `hip`, and is the logical-OR of `hvip`.VSTIP and, when the Sstc extension is implemented, the timer interrupt signal resulting from `vstimecmp`. | `hip`.VSTIP 和 `hie`.VSTIE 是 VS 级定时器中断的中断待处理和使能位。VSTIP 在 `hip` 中为只读，是 `hvip`.VSTIP 与（当实现 Sstc 扩展时）`vstimecmp` 产生的定时器中断信号的逻辑或。 |
| `norm:hip_vssip_vssie_op` | Bits `hip`.VSSIP and `hie`.VSSIE are the interrupt-pending and interrupt-enable bits for VS-level software interrupts. VSSIP in `hip` is an alias (writable) of the same bit in `hvip`. | `hip`.VSSIP 和 `hie`.VSSIE 是 VS 级软件中断的中断待处理和使能位。`hip` 中的 VSSIP 是 `hvip` 中相同位的别名（可写）。 |
| `norm:hsint_priority` | Multiple simultaneous interrupts destined for HS-mode are handled in the following decreasing priority order: SEI, SSI, STI, SGEI, VSEI, VSSI, VSTI, LCOFI. | 多个同时到达 HS 模式的中断按以下优先级递减顺序处理：SEI、SSI、STI、SGEI、VSEI、VSSI、VSTI、LCOFI。 |

## hgeip 和 hgeie 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:hgeip_sz_acc_op` | The `hgeip` register is an HSXLEN-bit read-only register that indicates pending guest external interrupts for this hart. | `hgeip` 是一个 HSXLEN 位只读寄存器，指示此 hart 的待处理客户外部中断。 |
| `norm:hgeie_sz_acc_op` | The `hgeie` register is an HSXLEN-bit read/write register that contains enable bits for the guest external interrupts at this hart. | `hgeie` 是一个 HSXLEN 位读写寄存器，包含此 hart 的客户外部中断使能位。 |
| `norm:hgeie_img` | Hypervisor guest external interrupt-enable register (`hgeie`) is formatted as shown in the specification figure. | `hgeie` 寄存器格式如规范图所示。 |
| `norm:hgeip_hgeie_fields` | Guest external interrupt number _i_ corresponds with bit _i_ in both `hgeip` and `hgeie`. | 客户外部中断号 _i_ 对应 `hgeip` 和 `hgeie` 中的位 _i_。 |
| `norm:geilen` | The number of bits implemented in `hgeip` and `hgeie` for guest external interrupts is UNSPECIFIED and may be zero. This number is known as GEILEN. The least-significant bits are implemented first, apart from bit 0. Hence, if GEILEN is nonzero, bits GEILEN:1 shall be writable in `hgeie`, and all other bit positions shall be read-only zeros in both `hgeip` and `hgeie`. | `hgeip` 和 `hgeie` 中客户外部中断实现的位数未指定，可为零。该数目称为 GEILEN。除第 0 位外，最低有效位先实现。若 GEILEN 非零，`hgeie` 的 GEILEN:1 位可写，其余位在两个寄存器中均为只读零。 |
| `norm:hgeie_op` | Register `hgeie` selects the subset of guest external interrupts that cause a supervisor-level (HS-level) guest external interrupt. The enable bits in `hgeie` do not affect the VS-level external interrupt signal selected from `hgeip` by `hstatus`.VGEIN. | `hgeie` 选择引起 HS 级客户外部中断的客户外部中断子集。`hgeie` 中的使能位不影响由 `hstatus`.VGEIN 从 `hgeip` 中选择的 VS 级外部中断信号。 |

## henvcfg 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:henvcfg_sz_acc_op` | The `henvcfg` CSR is a 64-bit read/write register that controls certain characteristics of the execution environment when virtualization mode V=1. | `henvcfg` 是一个 64 位读写寄存器，控制虚拟化模式 V=1 时执行环境的某些特性。 |
| `norm:henvcfg_fiom_op` | If bit FIOM is set to one in `henvcfg`, FENCE instructions executed when V=1 are modified so the requirement to order accesses to device I/O implies also the requirement to order main memory accesses. | 若 `henvcfg` 的 FIOM 位设为 1，V=1 时执行的 FENCE 指令被修改，对设备 I/O 的排序要求也隐含对主存访问的排序要求。 |
| `norm:henvcfg_fiom_order` | Similarly, when FIOM=1 and V=1, if an atomic instruction that accesses a region ordered as device I/O has its _aq_ and/or _rl_ bit set, then that instruction is ordered as though it accesses both device I/O and memory. | 当 FIOM=1 且 V=1 时，若一条原子指令访问设备 I/O 有序区域并设置了 aq/rl 位，该指令的排序如同同时访问设备 I/O 和内存。 |
| `norm:henvcfg_pbmte_op` | The PBMTE bit controls whether the Svpbmt extension is available for use in VS-stage address translation. When PBMTE=1, Svpbmt is available for VS-stage address translation. When PBMTE=0, the implementation behaves as though Svpbmt were not implemented for VS-stage address translation. If Svpbmt is not implemented, PBMTE is read-only zero. | PBMTE 位控制 Svpbmt 扩展是否可用于 VS 阶段地址翻译。PBMTE=1 时可用，PBMTE=0 时行为如同未实现。若未实现 Svpbmt，PBMTE 为只读零。 |
| `norm:henvcfg_adue_op` | If the Svadu extension is implemented, the ADUE bit controls whether hardware updating of PTE A/D bits is enabled for VS-stage address translation. When ADUE=1, hardware updating is enabled. When ADUE=0, the implementation behaves as though Svade were implemented for VS-stage address translation. If Svadu is not implemented, ADUE is read-only zero. | 若实现了 Svadu 扩展，ADUE 位控制 VS 阶段地址翻译的 PTE A/D 位硬件更新是否启用。ADUE=1 时启用，ADUE=0 时行为如同实现了 Svade。未实现 Svadu 时，ADUE 为只读零。 |
| `norm:henvcfg_stce` | The Sstc extension adds the STCE (STimecmp Enable) bit to `henvcfg` CSR. When the Sstc extension is not implemented, STCE is read-only zero. The STCE bit enables `vstimecmp` for VS-mode when set to one. When STCE is zero, an attempt to access `stimecmp` (really `vstimecmp`) when V=1 raises a virtual-instruction exception, and VSTIP in `hip` reverts to its defined behavior as if this extension is not implemented. | Sstc 扩展向 `henvcfg` 添加 STCE 位。未实现时为只读零。STCE=1 时为 VS 模式启用 `vstimecmp`。STCE=0 时 V=1 下访问 `stimecmp`（实际为 `vstimecmp`）引发虚拟指令异常。 |
| `norm:henvcfg_cbze` | The Zicboz extension adds the CBZE field to `henvcfg`. When CBZE=1, CBO.ZERO is enabled for execution in VS/VU mode; when CBZE=0, it raises a virtual-instruction exception. When the Zicboz extension is not implemented, CBZE is read-only zero. | Zicboz 扩展向 `henvcfg` 添加 CBZE 字段。CBZE=1 时在 VS/VU 模式下启用 CBO.ZERO；CBZE=0 时引发虚拟指令异常。未实现时为只读零。 |
| `norm:henvcfg_cbcfe` | The Zicbom extension adds the CBCFE field to `henvcfg`. When V=1, if CBO.CLEAN and CBO.FLUSH are HS-qualified and CBCFE=1, they are enabled; if CBCFE=0, they raise a virtual-instruction exception. When Zicbom is not implemented, CBCFE is read-only zero. | Zicbom 扩展向 `henvcfg` 添加 CBCFE 字段。V=1 时若 HS 限定且 CBCFE=1，则 CBO.CLEAN 和 CBO.FLUSH 启用；CBCFE=0 时引发虚拟指令异常。未实现时为只读零。 |
| `norm:henvcfg_cbie` | The Zicbom extension adds the CBIE WARL field to `henvcfg`. The CBIE field controls execution of CBO.INVAL in privilege modes VS and VU. The encoding `10b` is reserved. When Zicbom is not implemented, CBIE is read-only zero. | Zicbom 扩展向 `henvcfg` 添加 CBIE WARL 字段，控制 VS 和 VU 模式下 CBO.INVAL 的执行。编码 `10b` 保留。未实现时为只读零。 |
| `norm:cbo-inval_h-mode_veq1_op` | When V=1, if the CBO.INVAL instruction is not HS-qualified, it raises an illegal-instruction exception. If HS-qualified and CBIE is `01b` or `11b`, the instruction is enabled; otherwise, it raises a virtual-instruction exception. | 当 V=1 时，若 CBO.INVAL 非 HS 限定则引发非法指令异常。若 HS 限定且 CBIE 为 01b 或 11b 则启用；否则引发虚拟指令异常。 |
| `norm:cbo-inval_h-mode_op0` | If CBO.INVAL is enabled in HS-mode to perform a flush operation, then when the instruction is enabled in VS- or VU-mode it performs a flush operation, even if CBIE is set to `11b`. Otherwise, when the instruction is enabled for execution, its behavior depends on the CBIE encoding. | 若 CBO.INVAL 在 HS 模式下启用执行 flush 操作，则在 VS/VU 模式下启用时也执行 flush，即使 CBIE=11b。否则行为取决于 CBIE 编码。 |
| `norm:cbo-inval_h-mode_op1` | `01b` -- The instruction is executed and performs a flush operation, even if configured by VS-mode to perform an invalidate operation. | 01b——指令执行 flush 操作，即使 VS 模式配置为 invalidate 操作。 |
| `norm:cbo-inval_h-mode_op2` | `11b` -- The instruction is executed and performs an invalidate operation, unless configured by VS-mode to perform a flush operation. | 11b——指令执行 invalidate 操作，除非 VS 模式配置为 flush 操作。 |
| `norm:henvcfg_pmm_op` | If the Ssnpm extension is implemented, the PMM field enables or disables pointer masking for VS-mode. When not implemented, PMM is read-only zero. PMM is read-only zero for RV32. | 若实现了 Ssnpm 扩展，PMM 字段启用或禁用 VS 模式的指针屏蔽。未实现时为只读零。RV32 下也为只读零。 |
| `norm:henvcfg_lpe_op` | The Zicfilp extension adds the LPE field in `henvcfg`. When LPE=1, the Zicfilp extension is enabled in VS-mode. When LPE=0, the hart does not update the ELP state and LPAD operates as a no-op. | Zicfilp 扩展向 `henvcfg` 添加 LPE 字段。LPE=1 时在 VS 模式下启用 Zicfilp。LPE=0 时 hart 不更新 ELP 状态，LPAD 作为 no-op 运行。 |
| `norm:henvcfg_sse_op` | The Zicfiss extension adds the SSE field in `henvcfg`. If SSE=1, the Zicfiss extension is activated in VS-mode. When SSE=0, the extension remains inactive in VS-mode with specific behavior changes. | Zicfiss 扩展向 `henvcfg` 添加 SSE 字段。SSE=1 时在 VS 模式下激活 Zicfiss。SSE=0 时保持不活跃，具有特定行为变化。 |
| `norm:henvcfg_dte_op` | The Ssdbltrp extension adds the double-trap-enable (DTE) field in `henvcfg`. When `henvcfg`.DTE is zero, the implementation behaves as though Ssdbltrp is not implemented for VS-mode and the `vsstatus`.SDT bit is read-only zero. | Ssdbltrp 扩展向 `henvcfg` 添加 DTE 字段。`henvcfg`.DTE=0 时，行为如同未为 VS 模式实现 Ssdbltrp，`vsstatus`.SDT 为只读零。 |
| `norm:henvcfgh_sz_acc_op` | When XLEN=32, `henvcfgh` is a 32-bit read/write register that aliases bits 63:32 of `henvcfg`. Register `henvcfgh` does not exist when XLEN=64. | 当 XLEN=32 时，`henvcfgh` 是 32 位读写寄存器，别名 `henvcfg` 的 63:32 位。XLEN=64 时不存在。 |

## hcounteren 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:hcounteren_sz` | The counter-enable register `hcounteren` is a 32-bit register that controls the availability of the hardware performance monitoring counters to the guest virtual machine. | `hcounteren` 是一个 32 位寄存器，控制硬件性能监控计数器对客户虚拟机的可用性。 |
| `norm:hcounteren_op` | When the CY, TM, IR, or HPMn bit in the `hcounteren` register is clear, attempts to read the corresponding counter while V=1 will cause a virtual-instruction exception if the same bit in `mcounteren` is 1. | 当 `hcounteren` 中的 CY、TM、IR 或 HPMn 位清零时，V=1 下读取对应计数器若 `mcounteren` 中同一位为 1 则引发虚拟指令异常。 |
| `norm:hcounteren_acc` | When the TM bit in `hcounteren` is clear, attempts to access the `vstimecmp` register (via `stimecmp`) while executing in VS-mode will cause a virtual-instruction exception if the same bit in `mcounteren` is set. | 当 `hcounteren` 的 TM 位清零时，VS 模式下访问 `vstimecmp`（通过 `stimecmp`）若 `mcounteren` 中同一位已设置则引发虚拟指令异常。 |
| `norm:hcounteren_warl` | `hcounteren` must be implemented. However, any of the bits may be read-only zero, indicating reads to the corresponding counter will cause an exception when V=1. Hence, they are effectively WARL fields. | `hcounteren` 必须实现。但任何位可以为只读零，表示 V=1 时读取对应计数器会引发异常。因此它们实际上是 WARL 字段。 |

## htimedelta 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:htimedelta_sz_acc_op` | The `htimedelta` CSR is a 64-bit read/write register that contains the delta between the value of the `time` CSR and the value returned in VS-mode or VU-mode. Reading the `time` CSR in VS or VU mode returns the sum of `htimedelta` and the actual value of `time`. | `htimedelta` 是一个 64 位读写寄存器，包含 `time` CSR 的值与 VS/VU 模式下返回值之间的差值。VS/VU 模式下读取 `time` 返回 `htimedelta` 与实际 `time` 值之和。 |
| `norm:htimedeltah_sz_acc_op` | When XLEN=32, `htimedeltah` is a 32-bit read/write register that aliases bits 63:32 of `htimedelta`. Register `htimedeltah` does not exist when XLEN=64. | 当 XLEN=32 时，`htimedeltah` 是 32 位读写寄存器，别名 `htimedelta` 的 63:32 位。XLEN=64 时不存在。 |
| `norm:time_htimedelta_req` | If the `time` CSR is implemented, `htimedelta` (and `htimedeltah` for XLEN=32) must be implemented. | 若实现了 `time` CSR，则 `htimedelta`（XLEN=32 时还有 `htimedeltah`）必须实现。 |

## htval 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:htval_sz_acc_op` | The `htval` register is an HSXLEN-bit read/write register. When a trap is taken into HS-mode, `htval` is written with additional exception-specific information, alongside `stval`, to assist software in handling the trap. | `htval` 是一个 HSXLEN 位读写寄存器。陷阱进入 HS 模式时写入额外的异常特定信息以辅助软件处理。 |
| `norm:htval_trapval` | When a guest-page-fault trap is taken into HS-mode, `htval` is written with either zero or the guest physical address that faulted, shifted right by 2 bits. For other traps, `htval` is set to zero. | 客户页错误陷阱进入 HS 模式时，`htval` 写入零或故障的客户物理地址右移 2 位。其他陷阱设为零。 |
| `norm:htval_val` | `htval` is a WARL register that must be able to hold zero and may be capable of holding only an arbitrary subset of other 2-bit-shifted guest physical addresses, if any. | `htval` 是一个 WARL 寄存器，必须能保持零，可能仅能保持其他 2 位右移客户物理地址的任意子集。 |

## htinst 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:htinst_sz_acc_op` | The `htinst` register is an HSXLEN-bit read/write register. When a trap is taken into HS-mode, `htinst` is written with a value that, if nonzero, provides information about the instruction that trapped. | `htinst` 是一个 HSXLEN 位读写寄存器。陷阱进入 HS 模式时写入提供关于陷阱指令信息的值。 |
| `norm:htinst_val` | `htinst` is a WARL register that need only be able to hold the values that the implementation may automatically write to it on a trap. | `htinst` 是一个 WARL 寄存器，仅需能保持实现在陷阱时可能自动写入的值。 |

## hgatp 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:hgatp_sz_acc_op` | The `hgatp` register is an HSXLEN-bit read/write register which controls G-stage address translation and protection, the second stage of two-stage translation for guest virtual addresses. | `hgatp` 是一个 HSXLEN 位读写寄存器，控制 G 阶段地址翻译和保护，即客户虚拟地址两阶段翻译的第二阶段。 |
| `norm:hgatp_tvm_illegal` | When `mstatus`.TVM=1, attempts to read or write `hgatp` while executing in HS-mode will raise an illegal-instruction exception. | 当 `mstatus`.TVM=1 时，在 HS 模式下尝试读写 `hgatp` 将引发非法指令异常。 |
| `norm:hgatp_mode_bare` | When MODE=Bare, guest physical addresses are equal to supervisor physical addresses, and there is no further memory protection for a guest virtual machine beyond the physical memory protection scheme. In this case, software must write zero to the remaining fields in `hgatp`. | 当 MODE=Bare 时，客户物理地址等于 supervisor 物理地址，无额外内存保护。此时软件必须将 `hgatp` 的其余字段写为零。 |
| `norm:hgatp_mode_sv` | When HSXLEN=32, the only other valid setting for MODE is Sv32x4. When HSXLEN=64, modes Sv39x4, Sv48x4, and Sv57x4 are defined. | 当 HSXLEN=32 时，MODE 的唯一其他有效设置为 Sv32x4。当 HSXLEN=64 时，定义了 Sv39x4、Sv48x4 和 Sv57x4 模式。 |
| `norm:hgatp_mode_warl` | A write to `hgatp` with an unsupported MODE value is not ignored as it is for `satp`. Instead, the fields of `hgatp` are WARL in the normal way, when so indicated. | 使用不支持的 MODE 值写入 `hgatp` 不会像 `satp` 那样被忽略。`hgatp` 的字段按常规 WARL 方式处理。 |
| `norm:hgatp_ppn_op` | For the paged virtual-memory schemes, the root page table is 16 KiB and must be aligned to a 16-KiB boundary. In these modes, the lowest two bits of the physical page number (PPN) in `hgatp` always read as zeros. | 对于分页虚拟内存方案，根页表为 16 KiB 且必须对齐到 16 KiB 边界。这些模式下 `hgatp` 中 PPN 的最低两位始终读为零。 |
| `norm:hgatp_vmid` | The number of VMID bits is UNSPECIFIED and may be zero. | VMID 位数未指定，可以为零。 |
| `norm:hgatp_vmid_lsbs` | The least-significant bits of VMID are implemented first: that is, if VMIDLEN > 0, VMID[VMIDLEN-1:0] is writable. The maximal value of VMIDLEN, termed VMIDMAX, is 7 for Sv32x4 or 14 for Sv39x4, Sv48x4, and Sv57x4. | VMID 的最低有效位先实现。VMIDMAX 对于 Sv32x4 为 7，对于 Sv39x4/Sv48x4/Sv57x4 为 14。 |

## vsstatus 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:vsstatus_sz_acc_op` | The `vsstatus` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `sstatus`. When V=1, `vsstatus` substitutes for the usual `sstatus`, so instructions that normally read or modify `sstatus` actually access `vsstatus` instead. | `vsstatus` 是一个 VSXLEN 位读写寄存器，是 VS 模式版本的 `sstatus`。V=1 时替代 `sstatus`。 |
| `norm:vsstatus_uxl_op` | The UXL field controls the effective XLEN for VU-mode. When VSXLEN=32, the UXL field does not exist, and VU-mode XLEN=32. When VSXLEN=64, UXL is a WARL field. An implementation may make UXL be a read-only copy of field VSXL of `hstatus`, forcing VU-mode XLEN=VSXLEN. | UXL 字段控制 VU 模式的有效 XLEN。VSXLEN=32 时不存在，VU 模式 XLEN=32。VSXLEN=64 时为 WARL 字段。实现可使 UXL 为 `hstatus`.VSXL 的只读副本。 |
| `norm:vsstatus_uxl_change` | If VSXLEN is changed from 32 to a wider width, and if field UXL is not restricted to a single value, it gets the value corresponding to the widest supported width not wider than the new VSXLEN. | 若 VSXLEN 从 32 变为更宽宽度且 UXL 未限制为单一值，则取不超过新 VSXLEN 的最宽支持宽度对应值。 |
| `norm:vsstatus_fs_op` | When V=1, both `vsstatus`.FS and the HS-level `sstatus`.FS are in effect. Attempts to execute a floating-point instruction when either field is 0 (Off) raise an illegal-instruction exception. Modifying the floating-point state when V=1 causes both fields to be set to 3 (Dirty). | V=1 时，`vsstatus`.FS 和 HS 级 `sstatus`.FS 同时生效。任一为 0 时执行浮点指令引发非法指令异常。V=1 时修改浮点状态使两者都设为 3(Dirty)。 |
| `norm:vsstatus_vs_op` | Similarly, when V=1, both `vsstatus`.VS and the HS-level `sstatus`.VS are in effect. Attempts to execute a vector instruction when either field is 0 (Off) raise an illegal-instruction exception. Modifying the vector state when V=1 causes both fields to be set to 3 (Dirty). | 类似地，V=1 时 `vsstatus`.VS 和 HS 级 `sstatus`.VS 同时生效。任一为 0 时执行向量指令引发非法指令异常。V=1 时修改向量状态使两者都设为 3(Dirty)。 |
| `norm:vsstatus_sd_xs_op` | Read-only fields SD and XS summarize the extension context status as it is visible to VS-mode only. For example, the value of the HS-level `sstatus`.FS does not affect `vsstatus`.SD. | 只读字段 SD 和 XS 仅总结对 VS 模式可见的扩展上下文状态。例如 HS 级 `sstatus`.FS 不影响 `vsstatus`.SD。 |
| `norm:vsstatus_ube` | An implementation may make field UBE be a read-only copy of `hstatus`.VSBE. | 实现可使 UBE 字段为 `hstatus`.VSBE 的只读副本。 |
| `norm:vsstatus_v0` | When V=0, `vsstatus` does not directly affect the behavior of the machine, unless a virtual-machine load/store (HLV, HLVX, or HSV) or the MPRV feature in the `mstatus` register is used to execute a load or store as though V=1. | V=0 时 `vsstatus` 不直接影响机器行为，除非使用虚拟机 load/store 或 `mstatus` 的 MPRV 功能以 V=1 方式执行访问。 |
| `norm:vsstatus_spelp_op` | The Zicfilp extension adds the SPELP field that holds the previous ELP. Encoded as: 0 - NO_LP_EXPECTED, 1 - LP_EXPECTED. | Zicfilp 扩展添加 SPELP 字段保存前一个 ELP。编码：0=NO_LP_EXPECTED，1=LP_EXPECTED。 |
| `norm:vsstatus_sdt_op` | The Ssdbltrp adds an S-mode-disable-trap (SDT) field extension to address double trap in VS-mode. | Ssdbltrp 添加 SDT 字段以处理 VS 模式下的双陷阱。 |

## vsip 和 vsie 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:vsip_vsie_sz_acc_op` | The `vsip` and `vsie` registers are VSXLEN-bit read/write registers that are VS-mode's versions of supervisor CSRs `sip` and `sie`. When V=1, `vsip` and `vsie` substitute for the usual `sip` and `sie`. However, interrupts directed to HS-level continue to be indicated in the HS-level `sip` register, not in `vsip`, when V=1. | `vsip` 和 `vsie` 是 VSXLEN 位读写寄存器，是 VS 模式版本的 `sip` 和 `sie`。V=1 时替代 `sip`/`sie`。但 HS 级中断仍在 HS 级 `sip` 中指示。 |
| `norm:vsip_img` | Virtual supervisor interrupt-pending register (`vsip`) is formatted as shown in the specification figure. | `vsip` 寄存器格式如规范图所示。 |
| `norm:vsie_img` | Virtual supervisor interrupt-enable register (`vsie`) is formatted as shown in the specification figure. | `vsie` 寄存器格式如规范图所示。 |
| `norm:vsip_vsie_lcofi` | Extension Shlcofideleg supports delegating LCOFI interrupts to VS-mode. If implemented, `hideleg` bit 13 is writable; otherwise read-only zero. When bit 13 of `hideleg` is zero, `vsip`.LCOFIP and `vsie`.LCOFIE are read-only zeros. Else, they are aliases of `sip`.LCOFIP and `sie`.LCOFIE. | Shlcofideleg 扩展支持将 LCOFI 中断委托给 VS 模式。若实现，`hideleg` 第 13 位可写；否则只读零。第 13 位为零时，`vsip`.LCOFIP 和 `vsie`.LCOFIE 为只读零；否则为 `sip`/`sie` 中对应位的别名。 |
| `norm:vsip_vsie_sei` | When bit 10 of `hideleg` is zero, `vsip`.SEIP and `vsie`.SEIE are read-only zeros. Else, they are aliases of `hip`.VSEIP and `hie`.VSEIE. | 当 `hideleg` 第 10 位为零时，`vsip`.SEIP 和 `vsie`.SEIE 为只读零。否则为 `hip`.VSEIP 和 `hie`.VSEIE 的别名。 |
| `norm:vsip_vsie_sti` | When bit 6 of `hideleg` is zero, `vsip`.STIP and `vsie`.STIE are read-only zeros. Else, they are aliases of `hip`.VSTIP and `hie`.VSTIE. | 当 `hideleg` 第 6 位为零时，`vsip`.STIP 和 `vsie`.STIE 为只读零。否则为 `hip`.VSTIP 和 `hie`.VSTIE 的别名。 |
| `norm:vsip_vsie_ssi` | When bit 2 of `hideleg` is zero, `vsip`.SSIP and `vsie`.SSIE are read-only zeros. Else, they are aliases of `hip`.VSSIP and `hie`.VSSIE. | 当 `hideleg` 第 2 位为零时，`vsip`.SSIP 和 `vsie`.SSIE 为只读零。否则为 `hip`.VSSIP 和 `hie`.VSSIE 的别名。 |

## 其他 VS CSR 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:vstvec_sz_acc_op` | The `vstvec` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `stvec`. When V=1, `vstvec` substitutes for the usual `stvec`. When V=0, `vstvec` does not directly affect the behavior of the machine. | `vstvec` 是 VSXLEN 位读写寄存器，VS 模式版本的 `stvec`。V=1 时替代 `stvec`；V=0 时不直接影响机器行为。 |
| `norm:vstvec_img` | Virtual supervisor trap vector base address register `vstvec` is formatted as shown in the specification figure. | `vstvec` 寄存器格式如规范图所示。 |
| `norm:vsscratch_sz_acc_op` | The `vsscratch` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `sscratch`. When V=1, `vsscratch` substitutes for the usual `sscratch`. The contents of `vsscratch` never directly affect the behavior of the machine. | `vsscratch` 是 VSXLEN 位读写寄存器，VS 模式版本的 `sscratch`。V=1 时替代 `sscratch`。其内容从不直接影响机器行为。 |
| `norm:vspec_sz_acc_op` | The `vsepc` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `sepc`. When V=1, `vsepc` substitutes for the usual `sepc`. When V=0, `vsepc` does not directly affect the behavior of the machine. | `vsepc` 是 VSXLEN 位读写寄存器，VS 模式版本的 `sepc`。V=1 时替代 `sepc`；V=0 时不直接影响机器行为。 |
| `norm:vsepc_warl` | `vsepc` is a WARL register that must be able to hold the same set of values that `sepc` can hold. | `vsepc` 是一个 WARL 寄存器，必须能保持与 `sepc` 相同的值集合。 |
| `norm:vscause_sz_acc_op` | The `vscause` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `scause`. When V=1, `vscause` substitutes for the usual `scause`. When V=0, `vscause` does not directly affect the behavior of the machine. | `vscause` 是 VSXLEN 位读写寄存器，VS 模式版本的 `scause`。V=1 时替代 `scause`；V=0 时不直接影响机器行为。 |
| `norm:vscause_wlrl` | `vscause` is a WLRL register that must be able to hold the same set of values that `scause` can hold. | `vscause` 是一个 WLRL 寄存器，必须能保持与 `scause` 相同的值集合。 |
| `norm:vstval_sz_acc_op` | The `vstval` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `stval`. When V=1, `vstval` substitutes for the usual `stval`. When V=0, `vstval` does not directly affect the behavior of the machine. | `vstval` 是 VSXLEN 位读写寄存器，VS 模式版本的 `stval`。V=1 时替代 `stval`；V=0 时不直接影响机器行为。 |
| `norm:vstval_warl` | `vstval` is a WARL register that must be able to hold the same set of values that `stval` can hold. | `vstval` 是一个 WARL 寄存器，必须能保持与 `stval` 相同的值集合。 |
| `norm:vsatp_sz_acc_op` | The `vsatp` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `satp`. When V=1, `vsatp` substitutes for the usual `satp`. `vsatp` controls VS-stage address translation, the first stage of two-stage translation for guest virtual addresses. | `vsatp` 是 VSXLEN 位读写寄存器，VS 模式版本的 `satp`。V=1 时替代 `satp`。`vsatp` 控制 VS 阶段地址翻译。 |
| `norm:vs_stage_speculative_a_bit` | When `vsatp` is active, VS-stage page-table entries' A bits must not be set as a result of speculative execution, unless the effective privilege mode is VS or VU. | 当 `vsatp` 活跃时，VS 阶段页表项的 A 位不得因推测执行而被设置，除非有效特权模式为 VS 或 VU。 |
| `norm:vsatp_mode_unsupported_v0` | When V=0, a write to `vsatp` with an unsupported MODE value is either ignored as it is for `satp`, or the fields of `vsatp` are treated as WARL in the normal way. | V=0 时，使用不支持的 MODE 值写入 `vsatp`，要么像 `satp` 一样被忽略，要么按常规 WARL 方式处理。 |
| `norm:vsatp_mode_unsupported_v1` | However, when V=1, a write to `satp` with an unsupported MODE value is ignored and no write to `vsatp` is effected. | V=1 时，使用不支持的 MODE 值写入 `satp` 被忽略，不会对 `vsatp` 生效。 |
| `norm:vsatp_v0` | When V=0, `vsatp` does not directly affect the behavior of the machine, unless a virtual-machine load/store (HLV, HLVX, or HSV) or the MPRV feature in the `mstatus` register is used to execute a load or store as though V=1. | V=0 时 `vsatp` 不直接影响机器行为，除非使用虚拟机 load/store 或 MPRV 功能以 V=1 方式执行。 |

## vstimecmp 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:vstimecmp_sz` | The `vstimecmp` CSR is a 64-bit register and has 64-bit precision on all RV32 and RV64 systems. | `vstimecmp` 是一个 64 位寄存器，在所有 RV32 和 RV64 系统上都有 64 位精度。 |
| `norm:vstimecmp_acc` | In RV32 only, accesses to the `vstimecmp` CSR access the low 32 bits, while accesses to the `vstimecmph` CSR access the high 32 bits of `vstimecmp`. | 仅在 RV32 中，访问 `vstimecmp` CSR 访问低 32 位，访问 `vstimecmph` CSR 访问高 32 位。 |
| `norm:hip_vstip_op` | A virtual supervisor timer interrupt becomes pending, as reflected in the VSTIP bit in the `hip` register, whenever (`time` + `htimedelta`), truncated to 64 bits, contains a value greater than or equal to `vstimecmp`, treating the values as unsigned integers. | 当 (`time` + `htimedelta`) 截断为 64 位后的值大于或等于 `vstimecmp`（无符号比较）时，虚拟 supervisor 定时器中断变为待处理，反映在 `hip` 的 VSTIP 位中。 |
| `norm:hip_vstip_clear` | If the result of this comparison changes, it is guaranteed to be reflected in VSTIP eventually, but not necessarily immediately. The interrupt remains posted until `vstimecmp` becomes greater than (`time` + `htimedelta`), typically as a result of writing `vstimecmp`. | 比较结果变化保证最终反映在 VSTIP 中，但不一定立即。中断保持到 `vstimecmp` 大于 (`time` + `htimedelta`)，通常通过写入 `vstimecmp` 实现。 |
| `norm:hip_vstip_enable` | The interrupt will be taken based on the standard interrupt enable and delegation rules while V=1. | 中断将在 V=1 时根据标准中断使能和委托规则被接收。 |

## Hypervisor 指令

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:hlsv_mode` | The hypervisor virtual-machine load and store instructions are valid only in M-mode or HS-mode, or in U-mode when `hstatus`.HU=1. | hypervisor 虚拟机 load/store 指令仅在 M 模式、HS 模式或 `hstatus`.HU=1 的 U 模式下有效。 |
| `norm:hlsv_priv` | Each instruction performs an explicit memory access with an effective privilege mode of VS or VU. The effective privilege mode is VU when `hstatus`.SPVP=0, and VS when `hstatus`.SPVP=1. | 每条指令以 VS 或 VU 有效特权模式执行显式内存访问。SPVP=0 时为 VU，SPVP=1 时为 VS。 |
| `norm:hlsv_trans` | As usual for VS-mode and VU-mode, two-stage address translation is applied, and the HS-level `sstatus`.SUM is ignored. | 与 VS/VU 模式一样，应用两阶段地址翻译，HS 级 `sstatus`.SUM 被忽略。 |
| `norm:hlsv_sstatus_mxr` | HS-level `sstatus`.MXR makes execute-only pages readable by explicit loads for both stages of address translation (VS-stage and G-stage). | HS 级 `sstatus`.MXR 使仅执行页对两个阶段的地址翻译（VS 阶段和 G 阶段）的显式加载可读。 |
| `norm:hlsv_vsstatus_mxr` | `vsstatus`.MXR affects only the first translation stage (VS-stage). | `vsstatus`.MXR 仅影响第一翻译阶段（VS 阶段）。 |
| `norm:hlsv_op` | For every RV32I or RV64I load instruction, there is a corresponding virtual-machine load instruction: HLV.B, HLV.BU, HLV.H, HLV.HU, HLV.W, HLV.WU, and HLV.D. For every store instruction, there is: HSV.B, HSV.H, HSV.W, and HSV.D. Instructions HLV.WU, HLV.D, and HSV.D are not valid for RV32. | 每条 RV32I/RV64I load 指令有对应的虚拟机 load 指令；每条 store 指令有对应的虚拟机 store 指令。HLV.WU、HLV.D、HSV.D 对 RV32 无效。 |
| `norm:hlsv_u_op` | Instructions HLVX.HU and HLVX.WU are the same as HLV.HU and HLV.WU, except that execute permission takes the place of read permission during address translation. The supervisor physical memory attributes must grant both execute and read permissions. | HLVX.HU 和 HLVX.WU 与 HLV.HU 和 HLV.WU 相同，但地址翻译时执行权限替代读权限。supervisor 物理内存属性必须同时授予执行和读权限。 |
| `norm:hlvx-wu_valid32` | HLVX.WU is valid for RV32, even though LWU and HLV.WU are not. | HLVX.WU 对 RV32 有效，即使 LWU 和 HLV.WU 无效。 |
| `norm:hlsv_virtinst` | Attempts to execute a virtual-machine load/store instruction (HLV, HLVX, or HSV) when V=1 cause a virtual-instruction exception. | V=1 时尝试执行虚拟机 load/store 指令引发虚拟指令异常。 |
| `norm:hlsv_illegalinst` | Attempts to execute one of these same instructions from U-mode when `hstatus`.HU=0 cause an illegal-instruction exception. | 当 `hstatus`.HU=0 时从 U 模式执行这些指令引发非法指令异常。 |

## Hypervisor 内存管理 Fence 指令

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:hfence-vvma_hfence-gvma_op` | HFENCE.VVMA and HFENCE.GVMA perform a function similar to SFENCE.VMA, except applying to the VS-level memory-management data structures controlled by CSR `vsatp` (HFENCE.VVMA) or the guest-physical memory-management data structures controlled by CSR `hgatp` (HFENCE.GVMA). | HFENCE.VVMA 和 HFENCE.GVMA 类似 SFENCE.VMA，但分别应用于 `vsatp` 控制的 VS 级内存管理数据结构和 `hgatp` 控制的客户物理内存管理数据结构。 |
| `norm:hfence-vvma_mode` | HFENCE.VVMA is valid only in M-mode or HS-mode. Executing an HFENCE.VVMA guarantees that any previous stores already visible to the current hart are ordered before all implicit reads by that hart done for VS-stage address translation for subsequent instructions when `hgatp`.VMID has the same setting. | HFENCE.VVMA 仅在 M 模式或 HS 模式有效。执行后保证先前可见的存储在 `hgatp`.VMID 相同时排在后续 VS 阶段地址翻译的隐式读取之前。 |
| `norm:hfence-vvma_limits` | Implicit reads need not be ordered when `hgatp`.VMID is different than at the time HFENCE.VVMA executed. If rs1≠x0, it specifies a single guest virtual address, and if rs2≠x0, it specifies a single guest address-space identifier (ASID). | `hgatp`.VMID 不同时隐式读取无需排序。rs1≠x0 指定单个客户虚拟地址，rs2≠x0 指定单个客户 ASID。 |
| `norm:hfence-vvma_asid` | When rs2≠x0, bits XLEN-1:ASIDMAX of the value held in rs2 are reserved for future standard use. If ASIDLEN < ASIDMAX, the implementation shall ignore bits ASIDMAX-1:ASIDLEN of the value held in rs2. | rs2≠x0 时，rs2 中 XLEN-1:ASIDMAX 位保留。若 ASIDLEN < ASIDMAX，实现应忽略 rs2 中 ASIDMAX-1:ASIDLEN 位。 |
| `norm:hfence-vvma_tvm` | Neither `mstatus`.TVM nor `hstatus`.VTVM causes HFENCE.VVMA to trap. | `mstatus`.TVM 和 `hstatus`.VTVM 都不会导致 HFENCE.VVMA 陷入。 |
| `norm:hfence-gvma_op` | HFENCE.GVMA is valid only in HS-mode when `mstatus`.TVM=0, or in M-mode (irrespective of `mstatus`.TVM). Executing an HFENCE.GVMA guarantees that any previous stores already visible to the current hart are ordered before all implicit reads done for G-stage address translation for subsequent instructions. If rs1≠x0, it specifies a single guest physical address, shifted right by 2 bits, and if rs2≠x0, it specifies a single VMID. | HFENCE.GVMA 仅在 `mstatus`.TVM=0 的 HS 模式或 M 模式下有效。执行后保证先前可见的存储排在后续 G 阶段地址翻译的隐式读取之前。rs1≠x0 指定右移 2 位的客户物理地址，rs2≠x0 指定 VMID。 |
| `norm:hfence-gvma_vmid` | When rs2≠x0, bits XLEN-1:VMIDMAX of the value held in rs2 are reserved. If VMIDLEN < VMIDMAX, the implementation shall ignore bits VMIDMAX-1:VMIDLEN. | rs2≠x0 时，rs2 中 XLEN-1:VMIDMAX 位保留。若 VMIDLEN < VMIDMAX，实现应忽略对应位。 |
| `norm:hfence-gvma_mode` | If `hgatp`.MODE is changed for a given VMID, an HFENCE.GVMA with rs1=x0 (and rs2 set to either x0 or the VMID) must be executed to order subsequent guest translations with the MODE change—even if the old MODE or new MODE is Bare. | 若给定 VMID 的 `hgatp`.MODE 改变，必须执行 rs1=x0 的 HFENCE.GVMA 以排序后续客户翻译——即使旧/新 MODE 为 Bare。 |
| `norm:hfence-vvma_hfence-gvma_exceptions` | Attempts to execute HFENCE.VVMA or HFENCE.GVMA when V=1 cause a virtual-instruction exception, while attempts in U-mode cause an illegal-instruction exception. Attempting HFENCE.GVMA in HS-mode when `mstatus`.TVM=1 also causes an illegal-instruction exception. | V=1 时执行 HFENCE.VVMA 或 HFENCE.GVMA 引发虚拟指令异常；U 模式下引发非法指令异常。HS 模式下 `mstatus`.TVM=1 时执行 HFENCE.GVMA 也引发非法指令异常。 |

## 机器级 CSR 修改

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mstatus_mpv_op` | The MPV bit is written by the implementation whenever a trap is taken into M-mode, set to the value of V at the time of the trap. When an MRET instruction is executed, V is set to MPV, unless MPP=3, in which case V remains 0. | MPV 位在陷阱进入 M 模式时由实现写入，设为陷阱时 V 的值。执行 MRET 时 V 设为 MPV，除非 MPP=3 时 V 保持 0。 |
| `norm:mstatus_gva_op` | Field GVA is written by the implementation whenever a trap is taken into M-mode. For any trap that writes a guest virtual address to `mtval`, GVA is set to 1. For any other trap into M-mode, GVA is set to 0. | GVA 字段在陷阱进入 M 模式时由实现写入。写入客户虚拟地址到 `mtval` 的陷阱设 GVA=1，其他设 GVA=0。 |
| `norm:mstatus_modes` | The TSR and TVM fields of `mstatus` affect execution only in HS-mode, not in VS-mode. The TW field affects execution in all modes except M-mode. | `mstatus` 的 TSR 和 TVM 字段仅影响 HS 模式执行，不影响 VS 模式。TW 字段影响除 M 模式外的所有模式。 |
| `norm:mstatus_tvm_hs` | Setting TVM=1 prevents HS-mode from accessing `hgatp` or executing HFENCE.GVMA or HINVAL.GVMA, but has no effect on accesses to `vsatp` or instructions HFENCE.VVMA or HINVAL.VVMA. | TVM=1 阻止 HS 模式访问 `hgatp` 或执行 HFENCE.GVMA/HINVAL.GVMA，但不影响 `vsatp` 访问或 HFENCE.VVMA/HINVAL.VVMA 指令。 |
| `norm:mstatus_mprv_hypervisor` | The hypervisor extension changes the behavior of MPRV. When MPRV=0, normal translation. When MPRV=1, explicit memory accesses are translated and protected as though the current virtualization mode were set to MPV and the current nominal privilege mode were set to MPP. | hypervisor 扩展改变了 MPRV 的行为。MPRV=0 时正常翻译。MPRV=1 时显式内存访问如同当前虚拟化模式为 MPV、名义特权模式为 MPP 般翻译和保护。 |
| `norm:mstatus_mprv_hlsv` | MPRV does not affect the virtual-machine load/store instructions, HLV, HLVX, and HSV. The explicit loads and stores of these instructions always act as though V=1 and the nominal privilege mode were `hstatus`.SPVP, overriding MPRV. | MPRV 不影响虚拟机 load/store 指令。这些指令的显式访问始终如同 V=1 且名义特权模式为 `hstatus`.SPVP，覆盖 MPRV。 |
| `norm:mideleg_acc_h` | When the hypervisor extension is implemented, bits 10, 6, and 2 of `mideleg` are each read-only one. Furthermore, if GEILEN is nonzero, bit 12 of `mideleg` is also read-only one. VS-level interrupts and guest external interrupts are always delegated past M-mode to HS-mode. | 实现 hypervisor 扩展时，`mideleg` 的第 10、6、2 位为只读 1。若 GEILEN 非零，第 12 位也为只读 1。VS 级中断和客户外部中断始终委托到 HS 模式。 |
| `norm:mideleg_hroz` | For bits of `mideleg` that are zero, the corresponding bits in `hideleg`, `hip`, and `hie` are read-only zeros. | `mideleg` 中为零的位，`hideleg`、`hip`、`hie` 中对应位为只读零。 |
| `norm:mip_mie_vs` | The hypervisor extension gives registers `mip` and `mie` additional active bits for the hypervisor-added interrupts. | hypervisor 扩展为 `mip` 和 `mie` 添加 hypervisor 新增中断的额外活跃位。 |
| `norm:mip_mie_alias` | Bits SGEIP, VSEIP, VSTIP, and VSSIP in `mip` are aliases for the same bits in hypervisor CSR `hip`, while SGEIE, VSEIE, VSTIE, and VSSIE in `mie` are aliases for the same bits in `hie`. | `mip` 中的 SGEIP、VSEIP、VSTIP、VSSIP 是 `hip` 中相同位的别名；`mie` 中的 SGEIE、VSEIE、VSTIE、VSSIE 是 `hie` 中相同位的别名。 |

## mtval2 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mtval2_sz_acc_op` | The `mtval2` register is an MXLEN-bit read/write register. When a trap is taken into M-mode, `mtval2` is written with additional exception-specific information, alongside `mtval`. | `mtval2` 是一个 MXLEN 位读写寄存器。陷阱进入 M 模式时写入额外异常特定信息。 |
| `norm:mtval2_trapval` | When a guest-page-fault trap is taken into M-mode, `mtval2` is written with either zero or the guest physical address that faulted, shifted right by 2 bits. For other traps, `mtval2` is set to zero. | 客户页错误陷阱进入 M 模式时，`mtval2` 写入零或故障客户物理地址右移 2 位。其他陷阱设为零。 |
| `norm:mtval2_trapval_vstrans` | If a guest-page fault is due to an implicit memory access during first-stage (VS-stage) address translation, a guest physical address written to `mtval2` is that of the implicit memory access that faulted. | 若客户页错误由 VS 阶段地址翻译的隐式内存访问引起，写入 `mtval2` 的地址是故障的隐式内存访问地址。 |
| `norm:mtval2_trapval_other` | Otherwise, for misaligned loads and stores that cause guest-page faults, a nonzero guest physical address in `mtval2` corresponds to the faulting portion of the access. For instruction guest-page faults on systems with variable-length instructions, a nonzero `mtval2` corresponds to the faulting portion of the instruction. | 对于导致客户页错误的未对齐 load/store，`mtval2` 中非零地址对应访问的故障部分。变长指令系统的指令客户页错误同理。 |
| `norm:mtval2_val` | `mtval2` is a WARL register that must be able to hold zero and may be capable of holding only an arbitrary subset of other 2-bit-shifted guest physical addresses. | `mtval2` 是 WARL 寄存器，必须能保持零，可能仅能保持 2 位右移客户物理地址的任意子集。 |
| `norm:mtval2_Ssdbltrap` | The Ssdbltrap extension requires the implementation of the `mtval2` CSR. | Ssdbltrap 扩展要求实现 `mtval2` CSR。 |

## mtinst 寄存器

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mtinst_sz_acc_op` | The `mtinst` register is an MXLEN-bit read/write register. When a trap is taken into M-mode, `mtinst` is written with a value that, if nonzero, provides information about the instruction that trapped. | `mtinst` 是一个 MXLEN 位读写寄存器。陷阱进入 M 模式时写入关于陷阱指令的信息。 |
| `norm:mtinst_val` | `mtinst` is a WARL register that need only be able to hold the values that the implementation may automatically write to it on a trap. | `mtinst` 是 WARL 寄存器，仅需能保持实现在陷阱时可能自动写入的值。 |

## 两阶段地址翻译

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:H_vm_twostage` | Whenever the current virtualization mode V is 1, two-stage address translation and protection is in effect. For any virtual memory access, the original virtual address is converted in the first stage by VS-level address translation, as controlled by `vsatp`, into a guest physical address. The guest physical address is then converted in the second stage by guest physical address translation, as controlled by `hgatp`, into a supervisor physical address. | 当 V=1 时，两阶段地址翻译和保护生效。虚拟地址先由 `vsatp` 控制的 VS 级翻译转为客户物理地址，再由 `hgatp` 控制的 G 阶段翻译转为 supervisor 物理地址。 |
| `norm:vsstatus_mxr_vm` | The `vsstatus` field MXR, which makes execute-only pages readable by explicit loads, only overrides VS-stage page protection. Setting MXR at VS-level does not override guest-physical page protections. | `vsstatus` 的 MXR 字段仅覆盖 VS 阶段页保护。VS 级设置 MXR 不覆盖客户物理页保护。 |
| `norm:sstatus_mxr_vm` | Setting MXR at HS-level, however, overrides both VS-stage and G-stage execute-only permissions. | HS 级设置 MXR 覆盖 VS 阶段和 G 阶段的仅执行权限。 |
| `norm:H_vm_gstagetrans` | When V=1, memory accesses that would normally bypass address translation are subject to G-stage address translation alone. This includes memory accesses made in support of VS-stage address translation, such as reads and writes of VS-level page tables. | V=1 时，通常绕过地址翻译的内存访问仅受 G 阶段地址翻译约束，包括支持 VS 阶段地址翻译的访问（如读写 VS 级页表）。 |
| `norm:H_pmp` | Machine-level physical memory protection applies to supervisor physical addresses and is in effect regardless of virtualization mode. | 机器级物理内存保护适用于 supervisor 物理地址，与虚拟化模式无关。 |

## 客户物理地址翻译

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:hgatp_mode_bare_trans` | When the address translation scheme selected by the MODE field of `hgatp` is Bare, guest physical addresses are equal to supervisor physical addresses without modification, and no memory protection applies in the trivial translation. | 当 `hgatp` 的 MODE 为 Bare 时，客户物理地址等于 supervisor 物理地址，无内存保护。 |
| `norm:hgatp_mode_x4` | When `hgatp`.MODE specifies a translation scheme of Sv32x4, Sv39x4, Sv48x4, or Sv57x4, G-stage address translation is a variation on the usual page-based scheme. In each case, the size of the incoming address is widened by 2 bits. The root page table is expanded by a factor of 4 to be 16 KiB and must be aligned to a 16 KiB boundary. | 当 `hgatp`.MODE 指定 Sv32x4/Sv39x4/Sv48x4/Sv57x4 时，G 阶段翻译是常规方案的变体。地址宽度增加 2 位，根页表扩大 4 倍为 16 KiB，必须对齐到 16 KiB 边界。 |
| `norm:hgatp_mode_sv32x4` | For Sv32x4, an incoming guest physical address is partitioned into a virtual page number (VPN) and page offset, identical to Sv32 except with two more bits at the high end in VPN[1]. | Sv32x4 的客户物理地址分为 VPN 和页偏移，与 Sv32 相同，但 VPN[1] 高端多 2 位。 |
| `norm:hgatp_mode_sv39x4` | For Sv39x4, partitioning is identical to Sv39, except with 2 more bits at the high end in VPN[2]. Address bits 63:41 must all be zeros, or else a guest-page-fault exception occurs. | Sv39x4 的分区与 Sv39 相同，但 VPN[2] 高端多 2 位。地址位 63:41 必须全为零，否则发生客户页错误。 |
| `norm:hgatp_mode_sv48x4` | For Sv48x4, partitioning is identical to Sv48, except with 2 more bits at the high end in VPN[3]. Address bits 63:50 must all be zeros, or else a guest-page-fault exception occurs. | Sv48x4 的分区与 Sv48 相同，但 VPN[3] 高端多 2 位。地址位 63:50 必须全为零，否则发生客户页错误。 |
| `norm:hgatp_mode_sv57x4` | For Sv57x4, partitioning is identical to Sv57, except with 2 more bits at the high end in VPN[4]. Address bits 63:59 must all be zeros, or else a guest-page-fault exception occurs. | Sv57x4 的分区与 Sv57 相同，但 VPN[4] 高端多 2 位。地址位 63:59 必须全为零，否则发生客户页错误。 |
| `norm:H_vm_gpatrans` | The conversion of an Sv32x4, Sv39x4, Sv48x4, or Sv57x4 guest physical address uses the same algorithm as Sv32, Sv39, Sv48, or Sv57, except: `hgatp` substitutes for `satp`; the effective privilege mode must be VS-mode or VU-mode; the current privilege mode is always taken to be U-mode when checking the U bit; and guest-page-fault exceptions are raised instead of regular page-fault exceptions. | Sv32x4/Sv39x4/Sv48x4/Sv57x4 客户物理地址转换使用相同算法，但 `hgatp` 替代 `satp`；有效特权模式须为 VS/VU；检查 U 位时始终视为 U 模式；引发客户页错误而非普通页错误。 |
| `norm:H_vm_gpapriv` | For G-stage address translation, all memory accesses are considered to be user-level accesses. Access type permissions are checked during G-stage translation the same as for VS-stage. For memory accesses supporting VS-stage translation, permissions and A/D bit needs are checked as though for an implicit load or store, not for the original access type. However, any exception is always reported for the original access type. | G 阶段地址翻译中，所有内存访问视为用户级访问。访问权限检查方式与 VS 阶段相同。支持 VS 阶段翻译的访问按隐式 load/store 检查权限，但异常始终报告为原始访问类型。 |
| `norm:H_vm_gpa_g` | The G bit in all G-stage PTEs is currently not used. It should be cleared by software for forward compatibility, and must be ignored by hardware. | G 阶段 PTE 中的 G 位当前未使用。软件应清零以保持前向兼容，硬件必须忽略。 |

## 客户页错误

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:H_guest_page_fault` | Guest-page-fault traps may be delegated from M-mode to HS-mode under the control of `medeleg`, but cannot be delegated to other privilege modes. On a guest-page fault, `mtval` or `stval` is written with the faulting guest virtual address, and `mtval2` or `htval` is written either with zero or with the faulting guest physical address, shifted right by 2 bits. | 客户页错误可通过 `medeleg` 从 M 模式委托给 HS 模式，但不能委托给其他特权模式。客户页错误时 `mtval`/`stval` 写入故障客户虚拟地址，`mtval2`/`htval` 写入零或故障客户物理地址右移 2 位。 |
| `norm:H_straddle` | When an instruction fetch or a misaligned memory access straddles a page boundary, two different address translations are involved. When a guest-page fault occurs, the faulting virtual address may be a page-boundary address that is higher than the instruction's original virtual address. | 当取指或未对齐内存访问跨越页边界时涉及两个地址翻译。客户页错误时故障虚拟地址可能是高于指令原始虚拟地址的页边界地址。 |
| `norm:mtval2_htval_virtaddr` | When a guest-page fault is not due to an implicit memory access for VS-stage address translation, a nonzero guest physical address written to `mtval2`/`htval` shall correspond to the exact virtual address written to `mtval`/`stval`. | 当客户页错误不是由 VS 阶段地址翻译的隐式内存访问引起时，写入 `mtval2`/`htval` 的非零客户物理地址必须对应写入 `mtval`/`stval` 的确切虚拟地址。 |

## 内存管理 Fence

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sfence_vma_v0` | When V=0, the virtual-address argument to SFENCE.VMA is an HS-level virtual address, and the ASID argument is an HS-level ASID. The instruction orders stores only to HS-level address-translation structures with subsequent HS-level address translations. | V=0 时，SFENCE.VMA 的虚拟地址参数为 HS 级虚拟地址，ASID 参数为 HS 级 ASID。指令仅对 HS 级地址翻译结构排序。 |
| `norm:sfence_vma_v1` | When V=1, the virtual-address argument is a guest virtual address within the current virtual machine, and the ASID argument is a VS-level ASID. The instruction orders stores only to the VS-level address-translation structures with subsequent VS-stage address translations for the same virtual machine. | V=1 时，虚拟地址参数为当前虚拟机内的客户虚拟地址，ASID 参数为 VS 级 ASID。指令仅对同一虚拟机的 VS 级地址翻译结构排序。 |

## 陷阱原因码

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:H_cause` | The hypervisor extension augments the trap cause encoding. Codes are added for VS-level interrupts (2, 6, 10), for supervisor-level guest external interrupts (12), for virtual-instruction exceptions (22), and for guest-page faults (20, 21, 23). Environment calls from VS-mode are assigned cause 10. | hypervisor 扩展增加了陷阱原因编码。添加了 VS 级中断（2、6、10）、HS 级客户外部中断（12）、虚拟指令异常（22）和客户页错误（20、21、23）的代码。VS 模式环境调用使用原因码 10。 |
| `norm:H_cause_ecall` | HS-mode and VS-mode ECALLs use different cause values so they can be delegated separately. | HS 模式和 VS 模式的 ECALL 使用不同原因值以便分别委托。 |
| `norm:H_cause_virtual_instruction` | When V=1, a virtual-instruction exception (code 22) is normally raised instead of an illegal-instruction exception if the attempted instruction is HS-qualified but is prevented from executing when V=1. An instruction is HS-qualified if it would be valid to execute in HS-mode, assuming TSR and TVM of `mstatus` are both zero. | V=1 时，若尝试的 HS 限定指令被阻止执行，通常引发虚拟指令异常（代码 22）而非非法指令异常。指令在假设 `mstatus`.TSR 和 TVM 均为零时能在 HS 模式有效执行则为 HS 限定。 |
| `norm:H_cause_virtual_instruction_high` | When V=1 and XLEN=32, an invalid attempt to access a high-half CSR raises a virtual-instruction exception instead of an illegal-instruction exception if the same CSR instruction for the corresponding low-half CSR is HS-qualified. | V=1 且 XLEN=32 时，无效的高半 CSR 访问若对应低半 CSR 指令为 HS 限定，则引发虚拟指令异常而非非法指令异常。 |
| `norm:H_illegal_high_half` | When XLEN>32, an attempt to access a high-half CSR always raises an illegal-instruction exception. | XLEN>32 时，访问高半 CSR 始终引发非法指令异常。 |

## 虚拟指令异常的具体情形

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:H_virtinst_vs_nonhighctr_h0_m1` | In VS-mode, attempts to access a non-high-half counter CSR when the corresponding bit in `hcounteren` is 0 and the same bit in `mcounteren` is 1. | VS 模式下，访问非高半计数器 CSR 时 `hcounteren` 对应位为 0 且 `mcounteren` 同一位为 1。 |
| `norm:H_virtinst_vs32_highctr_h0_m1` | In VS-mode, if XLEN=32, attempts to access a high-half counter CSR when the corresponding bit in `hcounteren` is 0 and the same bit in `mcounteren` is 1. | VS 模式 XLEN=32 下，访问高半计数器 CSR 时 `hcounteren` 对应位为 0 且 `mcounteren` 同一位为 1。 |
| `norm:H_virtinst_vu_nonhighctr_h0_s0_m1` | In VU-mode, attempts to access a non-high-half counter CSR when the corresponding bit in either `hcounteren` or `scounteren` is 0 and the same bit in `mcounteren` is 1. | VU 模式下，访问非高半计数器 CSR 时 `hcounteren` 或 `scounteren` 对应位为 0 且 `mcounteren` 同一位为 1。 |
| `norm:H_virtinst_vu32_highctr_h0_s0_m1` | In VU-mode, if XLEN=32, attempts to access a high-half counter CSR when the corresponding bit in either `hcounteren` or `scounteren` is 0 and the same bit in `mcounteren` is 1. | VU 模式 XLEN=32 下，访问高半计数器 CSR 时 `hcounteren` 或 `scounteren` 对应位为 0 且 `mcounteren` 同一位为 1。 |
| `norm:H_virtinst_vu_vs_hinst` | In VS-mode or VU-mode, attempts to execute a hypervisor instruction (HLV, HLVX, HSV, or HFENCE). | VS 或 VU 模式下，尝试执行 hypervisor 指令（HLV、HLVX、HSV 或 HFENCE）。 |
| `norm:H_virtinst_vu_vs_nonhigh_allowedhs_tvm0` | In VS-mode or VU-mode, attempts to access an implemented non-high-half hypervisor CSR or VS CSR when the same access would be allowed in HS-mode, assuming `mstatus`.TVM=0. | VS 或 VU 模式下，访问已实现的非高半 hypervisor/VS CSR，且假设 `mstatus`.TVM=0 时该访问在 HS 模式下允许。 |
| `norm:H_virtinst_vu_vs32_high_allowedhs_tvm0` | In VS-mode or VU-mode, if XLEN=32, attempts to access an implemented high-half hypervisor CSR or high-half VS CSR when the same access to the CSR's low-half partner would be allowed in HS-mode, assuming `mstatus`.TVM=0. | VS 或 VU 模式 XLEN=32 下，访问已实现的高半 hypervisor/VS CSR，且假设 TVM=0 时对应低半 CSR 在 HS 模式下允许访问。 |
| `norm:H_virtinst_vu_wfi_tw0` | In VU-mode, attempts to execute WFI when `mstatus`.TW=0. | VU 模式下，`mstatus`.TW=0 时尝试执行 WFI。 |
| `norm:H_virtinst_vu_sret_sfence` | In VU-mode, attempts to execute a supervisor instruction (SRET or SFENCE). | VU 模式下，尝试执行 supervisor 指令（SRET 或 SFENCE）。 |
| `norm:H_virtinst_vu_nonhigh_supervisor_allowedhs_tvm0` | In VU-mode, attempts to access an implemented non-high-half supervisor CSR when the same access would be allowed in HS-mode, assuming `mstatus`.TVM=0. | VU 模式下，访问已实现的非高半 supervisor CSR，且假设 TVM=0 时该访问在 HS 模式下允许。 |
| `norm:H_virtinst_vu32_high_supervisor_allowedhs_tvm0` | In VU-mode, if XLEN=32, attempts to access an implemented high-half supervisor CSR when the same access to the CSR's low-half partner would be allowed in HS-mode, assuming `mstatus`.TVM=0. | VU 模式 XLEN=32 下，访问已实现的高半 supervisor CSR，且假设 TVM=0 时对应低半 CSR 在 HS 模式下允许访问。 |
| `norm:H_virtinst_wfi_vtw1_tw0` | In VS-mode, attempts to execute WFI when `hstatus`.VTW=1 and `mstatus`.TW=0, unless the instruction completes within an implementation-specific, bounded time. | VS 模式下，`hstatus`.VTW=1 且 `mstatus`.TW=0 时尝试执行 WFI，除非指令在实现特定的有限时间内完成。 |
| `norm:H_virtinst_vs_sret_vtsr1` | In VS-mode, attempts to execute SRET when `hstatus`.VTSR=1. | VS 模式下，`hstatus`.VTSR=1 时尝试执行 SRET。 |
| `norm:H_virtinst_vs_sfence_sinval_satp_vtvm1` | In VS-mode, attempts to execute an SFENCE.VMA or SINVAL.VMA instruction or to access `satp`, when `hstatus`.VTVM=1. | VS 模式下，`hstatus`.VTVM=1 时尝试执行 SFENCE.VMA、SINVAL.VMA 或访问 `satp`。 |
| `norm:H_virtinst_xtval` | On a virtual-instruction trap, `mtval` or `stval` is written the same as for an illegal-instruction trap. | 虚拟指令陷阱时，`mtval` 或 `stval` 的写入方式与非法指令陷阱相同。 |
| `norm:H_illegalinst_xstatus_fs_vs` | Fields FS and VS in registers `sstatus` and `vsstatus` deviate from the usual HS-qualified rule. If an instruction is prevented from executing because FS or VS is zero in either `sstatus` or `vsstatus`, the exception raised is always an illegal-instruction exception, never a virtual-instruction exception. | `sstatus` 和 `vsstatus` 中的 FS 和 VS 字段偏离通常的 HS 限定规则。若指令因 FS 或 VS 为零而被阻止，始终引发非法指令异常，不引发虚拟指令异常。 |
| `norm:H_exception_priority` | If an instruction may raise multiple synchronous exceptions, the decreasing priority order indicates which exception is taken and reported in `mcause` or `scause`. | 若指令可能引发多个同步异常，按优先级递减顺序决定哪个异常被接收并报告在 `mcause` 或 `scause` 中。 |

## 陷阱入口

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:H_trap_deleg` | When a trap occurs in HS-mode or U-mode, it goes to M-mode, unless delegated by `medeleg` or `mideleg`, in which case it goes to HS-mode. When a trap occurs in VS-mode or VU-mode, it goes to M-mode, unless delegated by `medeleg`/`mideleg` to HS-mode, unless further delegated by `hedeleg`/`hideleg` to VS-mode. | HS/U 模式陷阱进入 M 模式，除非通过 `medeleg`/`mideleg` 委托给 HS 模式。VS/VU 模式陷阱进入 M 模式，除非委托给 HS 模式，除非进一步委托给 VS 模式。 |
| `norm:H_trap_m_csrwrites` | When a trap is taken into M-mode, V gets set to 0, and fields MPV and MPP in `mstatus` are set accordingly. A trap into M-mode also writes fields GVA, MPIE, and MIE in `mstatus` and writes CSRs `mepc`, `mcause`, `mtval`, `mtval2`, and `mtinst`. | 陷阱进入 M 模式时，V 设为 0，`mstatus` 的 MPV 和 MPP 相应设置。同时写入 GVA、MPIE、MIE 及 CSR `mepc`、`mcause`、`mtval`、`mtval2`、`mtinst`。 |
| `norm:H_trap_hs_csrwrites` | When a trap is taken into HS-mode, V is set to 0, and `hstatus`.SPV and `sstatus`.SPP are set accordingly. If V was 1 before the trap, SPVP is set the same as `sstatus`.SPP; otherwise, SPVP is left unchanged. A trap into HS-mode also writes GVA in `hstatus`, SPIE and SIE in `sstatus`, and CSRs `sepc`, `scause`, `stval`, `htval`, and `htinst`. | 陷阱进入 HS 模式时，V 设为 0，`hstatus`.SPV 和 `sstatus`.SPP 相应设置。陷阱前 V=1 时 SPVP 与 SPP 相同；否则不变。同时写入相关字段和 CSR。 |
| `norm:H_trap_vs_csrwrites` | When a trap is taken into VS-mode, `vsstatus`.SPP is set accordingly. Register `hstatus` and the HS-level `sstatus` are not modified, and V remains 1. A trap into VS-mode also writes SPIE and SIE in `vsstatus` and writes CSRs `vsepc`, `vscause`, and `vstval`. | 陷阱进入 VS 模式时，`vsstatus`.SPP 相应设置。`hstatus` 和 HS 级 `sstatus` 不修改，V 保持 1。同时写入 `vsstatus` 的 SPIE/SIE 及 CSR `vsepc`、`vscause`、`vstval`。 |

## 陷阱指令值（mtinst/htinst）

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:H_trap_xtinst` | On any trap into M-mode or HS-mode, one of these values is written to `mtinst` or `htinst`: zero; a transformation of the trapping instruction; a custom value (only if the trapping instruction is non-standard); or a special pseudoinstruction. | 任何陷阱进入 M/HS 模式时，`mtinst`/`htinst` 写入以下之一：零；陷阱指令的转换；自定义值（仅限非标准指令）；或特殊伪指令。 |
| `norm:H_trap_xtinst_interrupt` | On an interrupt, the value written to the trap instruction register is always zero. | 中断时，陷阱指令寄存器写入值始终为零。 |
| `norm:H_trap_xtinst_exception_lead-in` | On a synchronous exception, if a nonzero value is written, one of the following shall be true about the value. | 同步异常时，若写入非零值，该值必须满足以下条件之一。 |
| `norm:H_trap_xtinst_exception_list` | Bit 0 is 1 and replacing bit 1 with 1 makes a valid standard instruction encoding; OR bit 0 is 1 and replacing bit 1 with 1 makes a custom instruction encoding (custom value); OR the value is a special pseudoinstruction with bits 1:0 equal to 00. | 位 0 为 1 且位 1 替换为 1 后构成有效标准指令编码；或位 0 为 1 且位 1 替换为 1 后构成自定义指令编码；或值为位 1:0 等于 00 的特殊伪指令。 |
| `norm:H_trap_xtinst_val` | The values that may be automatically written to the trap instruction register for each standard exception cause are enumerated in the specification table. | 每种标准异常原因可自动写入陷阱指令寄存器的值在规范表中列举。 |
| `norm:H_trap_xtinst_guestpage` | For guest-page faults, the trap instruction register is written with a special pseudoinstruction value if: (a) the fault is caused by an implicit memory access for VS-stage address translation, and (b) a nonzero value is written to `mtval2` or `htval`. If both conditions are met, zero is not allowed. | 对于客户页错误，若 (a) 故障由 VS 阶段地址翻译的隐式内存访问引起，且 (b) `mtval2`/`htval` 写入非零值，则陷阱指令寄存器必须写入特殊伪指令值，不允许零。 |
| `norm:H_trap_xtinst_guestpage_rw` | A write pseudoinstruction (0x00002020 or 0x00003020) is used for the case that the machine is attempting automatically to update bits A and/or D in VS-level page tables. All other implicit memory accesses for VS-stage address translation will be reads. | 写伪指令（0x00002020 或 0x00003020）用于机器自动更新 VS 级页表 A/D 位的情况。所有其他 VS 阶段翻译的隐式内存访问为读取。 |

## 陷阱返回

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mret_h` | MRET first determines the new privilege mode according to MPP and MPV in `mstatus`. MRET then sets MPV=0, MPP=0, MIE=MPIE, and MPIE=1. Lastly, MRET sets the privilege mode as previously determined, and sets pc=mepc. | MRET 先根据 `mstatus` 中 MPP 和 MPV 确定新特权模式，然后设 MPV=0、MPP=0、MIE=MPIE、MPIE=1，最后设置特权模式并 pc=mepc。 |
| `norm:sret_h` | The SRET instruction is used to return from a trap taken into HS-mode or VS-mode. Its behavior depends on the current virtualization mode. | SRET 指令用于从 HS 模式或 VS 模式的陷阱返回，行为取决于当前虚拟化模式。 |
| `norm:sret_v0` | When executed in M-mode or HS-mode (V=0), SRET first determines the new privilege mode according to `hstatus`.SPV and `sstatus`.SPP. SRET then sets `hstatus`.SPV=0, and in `sstatus` sets SPP=0, SIE=SPIE, and SPIE=1. Lastly, SRET sets the privilege mode and sets pc=sepc. | 在 M/HS 模式（V=0）执行时，SRET 根据 `hstatus`.SPV 和 `sstatus`.SPP 确定新特权模式，然后设 SPV=0、SPP=0、SIE=SPIE、SPIE=1，最后设置特权模式并 pc=sepc。 |
| `norm:sret_v1` | When executed in VS-mode (V=1), SRET sets the privilege mode accordingly, in `vsstatus` sets SPP=0, SIE=SPIE, and SPIE=1, and lastly sets pc=vsepc. | 在 VS 模式（V=1）执行时，SRET 相应设置特权模式，在 `vsstatus` 中设 SPP=0、SIE=SPIE、SPIE=1，最后 pc=vsepc。 |
| `norm:sret_dt` | If the Ssdbltrp extension is implemented, when SRET is executed in HS-mode, if the new privilege mode is VU, the SRET instruction sets `vsstatus`.SDT to 0. When executed in VS-mode, `vsstatus`.SDT is set to 0. | 若实现了 Ssdbltrp 扩展，HS 模式执行 SRET 且新特权模式为 VU 时，设 `vsstatus`.SDT=0。VS 模式执行时也设 `vsstatus`.SDT=0。 |

## Shcounterenw 扩展（Counter-Enable Writability, v1.0）

> 来源：`SPEC/shcounterenw.adoc`

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:shcounterenw_hpmcounter_hcounteren` | If the Shcounterenw extension is implemented, then for any `hpmcounter` that is not read-only zero, the corresponding bit in `hcounteren` must be writable. | 若实现了 Shcounterenw 扩展，则对于任何非只读零的 `hpmcounter`，`hcounteren` 中的对应位必须可写。 |

## Shgatpa 扩展（Translation Mode Support, v1.0）

> 来源：`SPEC/shgatpa.adoc`

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:shgatpa_satp_hgatp_mode_support` | If the Shgatpa extension is implemented, then for each supported virtual memory scheme Sv_NN_ supported in `satp`, the corresponding hgatp Sv_NN_x4 mode must be supported. | 若实现了 Shgatpa 扩展，则 `satp` 中支持的每种虚拟内存方案 Sv_NN_，`hgatp` 中对应的 Sv_NN_x4 模式也必须支持。 |
| `norm:shgatpa_hgatp_bare_mode` | Furthermore, the `hgatp` mode Bare must also be supported. | 此外，`hgatp` 的 Bare 模式也必须支持。 |

## Shtvala 扩展（Trap Value Reporting, v1.0）

> 来源：`SPEC/shtvala.adoc`

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:shtvala_htval_faulting_gpa` | If the Shtvala extension is implemented, `htval` must be written with the faulting guest physical address in all circumstances permitted by the ISA. | 若实现了 Shtvala 扩展，在 ISA 允许的所有情况下，`htval` 必须写入故障的客户物理地址。 |

## Shvsatpa 扩展（Translation Mode Support, v1.0）

> 来源：`SPEC/shvsatpa.adoc`

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:shvsatpa_satp_vsatp_modes` | If the Shvsatpa extension is implemented, all translation modes supported in `satp` must be supported in `vsatp`. | 若实现了 Shvsatpa 扩展，`satp` 中支持的所有翻译模式在 `vsatp` 中也必须支持。 |

## Shvstvala 扩展（Trap Value Reporting, v1.0）

> 来源：`SPEC/shvstvala.adoc`

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:shvstvala_vstval_written` | If the Shvstvala extension is implemented, `vstval` must be written in all cases described for `stval`. | 若实现了 Shvstvala 扩展，`vstval` 必须在所有为 `stval` 描述的情况下被写入。 |

## Shvstvecd 扩展（Direct Trap Vectoring, v1.0）

> 来源：`SPEC/shvstvecd.adoc`

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:shvstvecd_vstvec_mode_direct` | If the Shvstvecd extension is implemented, then `vstvec.MODE` must be capable of holding the value 0 (Direct). | 若实现了 Shvstvecd 扩展，`vstvec.MODE` 必须能够保持值 0（Direct）。 |
| `norm:shvstvecd_vstvec_base_aligned_address` | Furthermore, when `vstvec.MODE`=Direct, `vstvec.BASE` must be capable of holding any valid four-byte-aligned address. | 此外，当 `vstvec.MODE`=Direct 时，`vstvec.BASE` 必须能够保持任何有效的四字节对齐地址。 |
