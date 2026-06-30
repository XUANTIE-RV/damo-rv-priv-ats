# Machine-Level ISA Norm 规范条目表

> 来源：`SPEC/machine.adoc` — Machine-Level ISA, Version 1.13
> 共提取规范条目：475 条

---

## Machine-Level ISA, Version 1.13

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:M_highest_priv_mode` | machine-mode (M-mode), which is the highest privilege mode in a RISC-V hart. |  机器模式（M-mode）是 RISC-V hart 中的最高特权模式。 |
| `norm:M-mode_at_rst1` | M-mode is used for low-level access to a hardware platform and is the first mode entered at reset. |  M 模式用于对硬件平台的底层访问，并且是复位后进入的第一个模式。 |

### Machine-Level CSRs

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:M_access_all_lower_priv_CSRs` | M-mode code can access all CSRs at lower privilege levels. |  M 模式代码可以访问所有低特权级别的 CSR。 |

#### Machine ISA (misa) Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine ISA (misa) Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:misa_acc` | The `misa` CSR is a **WARL** read-write register |  `misa` CSR 是一个 **WARL** 读写寄存器，用于报告 hart 支持的 ISA。 |
| `norm:misa_always_rd` | This register must be readable in any implementation |  该寄存器在任何实现中都必须是可读的。 |
| `norm:misa_csr_implemented` | a value of zero can be returned to indicate this feature has not been implemented |  可以返回零值以表示此功能未实现，此时需要通过单独的非标准机制来确定 CPU 能力。 |
| `norm:misa_mxl_op_isa` | The MXL (Machine XLEN) field encodes the native base integer ISA width as shown in norm:misa_mxl_enc. |  MXL（Machine XLEN）字段编码原生基础整数 ISA 宽度。 |
| `norm:misa_mxl_acc` | The MXL field is read-only. |  MXL 字段是只读的。 |
| `norm:misa_mxl_op_nz` | If `misa` is nonzero, the MXL field indicates the effective XLEN in M-mode, a constant termed _MXLEN_. |  如果 `misa` 非零，则 MXL 字段表示 M 模式下的有效 XLEN，该常量称为 MXLEN。 |
| `norm:xlen_le_mxlen` | XLEN is never greater than MXLEN, but XLEN might be smaller than MXLEN in less-privileged modes. |  XLEN 永远不会大于 MXLEN，但在较低特权模式下 XLEN 可能小于 MXLEN。 |
| `norm:misa_sz` | The `misa` CSR is MXLEN bits wide. |  `misa` CSR 的宽度为 MXLEN 位。 |
| `norm:misa_extensions_enc_txt` | The Extensions field encodes the presence of the standard extensions, with a single bit per letter of the alphabet (bit 0 encodes presence of extension a , bit 1 encodes presence of extension b, through to bit 25 which encodes z). |  Extensions 字段编码标准扩展的存在情况，每个字母对应一位（第 0 位编码扩展 A，第 1 位编码扩展 B，直到第 25 位编码 Z）。 |
| `norm:misa_i_op` | The `i` bit will be set for the RV32I and RV64I base ISAs |  对于 RV32I 和 RV64I 基础 ISA，`i` 位将被置位。 |
| `norm:misa_e_op` | the `e` bit will be set for RV32E and RV64E. |  对于 RV32E 和 RV64E，`e` 位将被置位。 |
| `norm:misa_extensions_warl_op` | The Extensions field is a **WARL** field that can contain writable bits where the implementation allows the supported ISA to be modified. |  Extensions 字段是一个 **WARL** 字段，当实现允许修改支持的 ISA 时，可以包含可写位。 |
| `norm:misa_extensions_rst1` | At reset, the Extensions field shall contain the maximal set of supported extensions, and `i` shall be selected over `e` if both are available. |  复位时，Extensions 字段应包含最大支持扩展集，如果两者都可用，则优先选择 `i` 而非 `e`。 |
| `norm:misa_extensions_rsv_ret_0` | All bits that are reserved for future use must return zero when read. |  所有保留供未来使用的位在读取时必须返回零。 |
| `norm:misa_x_op` | The `x` bit will be set if there are any non-standard extensions. |  如果存在任何非标准扩展，则 `x` 位将被置位。 |
| `norm:misa_b_op` | When the `b` bit is 1, the implementation supports the instructions provided by the zba, zbb, and zbs extensions. |  当 `b` 位为 1 时，表示实现支持 Zba、Zbb 和 Zbs 扩展提供的指令。 |
| `norm:misa_m_op` | When the `m` bit is 1, the implementation supports all multiply and division instructions defined by the m extension. |  当 `m` 位为 1 时，表示实现支持 M 扩展定义的所有乘法和除法指令。 |
| `norm:Zmmul_misa_m` | if the zmmul extension is supported then the multiply instructions it specifies are supported irrespective of the value of the `m` bit. |  如果支持 Zmmul 扩展，则无论 `m` 位的值如何，该扩展指定的乘法指令都被支持。 |
| `norm:misa_s_op` | When the `s` bit is 1, the implementation supports supervisor mode. |  当 `s` 位为 1 时，表示实现支持监管者模式（S-mode）。 |
| `norm:misa_u_op` | When the `u` bit is 1, the implementation supports user mode. |  当 `u` 位为 1 时，表示实现支持用户模式（U-mode）。 |
| `norm:misa_e_acc` | The `e` bit is read-only. |  `e` 位是只读的。 |
| `norm:misa_e_not_i` | Unless csr:misa[] is all read-only zero, the `e` bit always reads as the complement of the `i` bit. |  除非 `misa` 全为只读零，否则 `e` 位始终读取为 `i` 位的反码。 |
| `norm:misa_extensions_dependencies` | If an ISA feature _x_ depends on an ISA feature _y_, then attempting to enable feature _x_ but disable feature _y_ results in both features being disabled. |  如果 ISA 特性 x 依赖于 ISA 特性 y，则尝试启用 x 但禁用 y 会导致两个特性都被禁用。 |
| `norm:misa_inc_ialign` | Writing `misa` may increase IALIGN, e.g., by disabling the c extension. If an instruction that would write `misa` increases IALIGN, and the subsequent instruction's address is not IALIGN-bit aligned, the write to `misa` is suppressed, leaving `misa` unchanged. |  写入 `misa` 可能会增加 IALIGN（例如通过禁用 C 扩展）。如果写入会增加 IALIGN，且后续指令地址未按新的 IALIGN 对齐，则写入被抑制，`misa` 保持不变。 |
| `norm:misa_c_set_line` | csr:misa[c] must be clear if any of the following holds: |  如果以下任何条件成立，`misa`[C] 必须被清零。 |

#### Machine Vendor ID (mvendorid) Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Vendor ID (mvendorid) Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mvendorid_sz_acc_op` | The `mvendorid` CSR is a 32-bit read-only register providing the JEDEC manufacturer ID of the provider of the core. |  `mvendorid` CSR 是一个 32 位只读寄存器，提供核心提供商的 JEDEC 制造商 ID。 |
| `norm:mvendorid_always_rd` | This register must be readable in any implementation, but a value of 0 can be returned to indicate the field is not implemented or that this is a non-commercial implementation. |  该寄存器在任何实现中都必须是可读的，但可以返回 0 以表示未实现或非商业实现。 |
| `norm:mvendorid_enc` | JEDEC manufacturer IDs are ordinarily encoded as a sequence of one-byte continuation codes `0x7f`, terminated by a one-byte ID not equal to `0x7f`, with an odd parity bit in the most-significant bit of each byte. `mvendorid` encodes the number of one-byte continuation codes in the Bank field, and encodes the final byte in the Offset field, discarding the parity bit. |  JEDEC 制造商 ID 通常编码为一系列单字节续传码 `0x7f`，后跟一个不等于 `0x7f` 的单字节 ID，每个字节的最高位为奇校验位。`mvendorid` 在 Bank 字段中编码续传码的数量，在 Offset 字段中编码最终字节（丢弃校验位）。 |

#### Machine Architecture ID (marchid) Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Architecture ID (marchid) Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:marchid_sz_acc_op` | The `marchid` CSR is an MXLEN-bit read-only register encoding the base microarchitecture of the hart. |  `marchid` CSR 是一个 MXLEN 位只读寄存器，编码 hart 的基础微架构。 |
| `norm:marchid_always_rd` | This register must be readable in any implementation, but a value of 0 can be returned to indicate the field is not implemented. |  该寄存器在任何实现中都必须是可读的，但可以返回 0 以表示未实现。 |

#### Machine Implementation ID (mimpid) Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Implementation ID (mimpid) Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mimpid_op` | The `mimpid` CSR provides a unique encoding of the version of the processor implementation. |  `mimpid` CSR 提供处理器实现版本的唯一编码。 |
| `norm:mimpid_always_rd` | This register must be readable in any implementation, but a value of 0 can be returned to indicate that the field is not implemented. |  该寄存器在任何实现中都必须是可读的，但可以返回 0 以表示未实现。 |

#### Hart ID (mhartid) Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Hart ID (mhartid) Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mhartid_sz_acc_op` | The `mhartid` CSR is an MXLEN-bit read-only register containing the integer ID of the hardware thread running the code. |  `mhartid` CSR 是一个 MXLEN 位只读寄存器，包含该 hart 的唯一标识符。 |
| `norm:mhartid_always_rd` | This register must be readable in any implementation. |  该寄存器在任何实现中都必须是可读的。 |
| `norm:mhartid_one_is_zero` | one hart must have a hart ID of zero. |  至少有一个 hart 的 hart ID 必须为零。 |
| `norm:mhartid_unique` | Hart IDs must be unique within the execution environment. |  Hart ID 在执行环境中必须是唯一的。 |

#### Machine Status (mstatus and mstatush) Registers

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Status (mstatus and mstatush) Registers*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mstatus_sz_acc` | The `mstatus` register is an MXLEN-bit read/write register formatted as shown in mstatusreg-rv32 for RV32 and mstatusreg for RV64. `mstatus` must be implemented. |  `mstatus` 寄存器是一个 MXLEN 位读写寄存器。 |
| `norm:mstatush_sz_acc` | For RV32 only, `mstatush` is a 32-bit read/write register formatted as shown in mstatushreg. |  仅对于 RV32，`mstatush` 是一个 32 位读写寄存器。 |
| `norm:mstatush_enc` | Bits 30:4 of `mstatush` generally contain the same fields found in bits 62:36 of `mstatus` for RV64. Fields SD, SXL, and UXL do not exist in `mstatush`. |  `mstatush` 的第 30:4 位通常包含与 RV64 的 `mstatus` 第 62:36 位相同的字段。SD、SXL 和 UXL 字段在 `mstatush` 中不存在。 |

#### Privilege and Global Interrupt-Enable Stack in mstatus register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Status (mstatus and mstatush) Registers > Privilege and Global Interrupt-Enable Stack in mstatus register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mstatus_mie_sie_op1` | Global interrupt-enable bits, MIE and SIE, are provided for M-mode and S-mode respectively. |  提供全局中断使能位 MIE 和 SIE，分别用于 M 模式和 S 模式。 |
| `norm:mstatus_mie_sie_op2` | When a hart is executing in privilege mode _x_, interrupts are globally enabled when *x*IE=1 and globally disabled when *x*IE=0. |  当 hart 在特权模式 x 下执行时，如果对应的全局中断使能位 xIE 被置位，则中断被全局使能。 |
| `norm:mstatus_xie_intr_en_dis` | Interrupts for lower-privilege modes, *w*<*x*, are always globally disabled regardless of the setting of any global *w*IE bit for the lower-privilege mode. Interrupts for higher-privilege modes, *y*>*x*, are always globally enabled regardless of the setting of the global *y*IE bit for the higher-privilege mode. |  低特权模式 w（w<x）的中断始终被全局使能，不受 xIE 位控制。 |
| `norm:mstatus_sie_spie_rdonly0` | MIE and MPIE must be writable. If supervisor mode is not implemented, then SIE and SPIE are read-only 0; otherwise, they must be writable. |  MIE 和 MPIE 必须是可写的。如果未实现 S 模式，则 SIE 和 SPIE 为只读零。 |
| `norm:mstatus_xpie_xpp_op` | each privilege mode _x_ that can respond to interrupts has a two-level stack of interrupt-enable bits and privilege modes. *x*PIE holds the value of the interrupt-enable bit active prior to the trap, and *x*PP holds the previous privilege mode. |  每个可以响应 trap 的特权模式 x 都有一个 xPIE 字段保存 trap 前的中断使能状态，以及一个 xPP 字段保存 trap 前的特权模式。 |
| `norm:mstatus_xpp_enc` | The *x*PP fields can only hold privilege modes up to _x_ |  xPP 字段只能保存不超过 x 的特权模式。 |
| `norm:mstatus_mpp_sz` | MPP is two bits wide |  MPP 字段宽度为两位。 |
| `norm:mstatus_spp_sz` | SPP is one bit wide. |  SPP 字段宽度为一位。 |
| `norm:mstatus_xpie_xie_xpp_trap_op` | When a trap is taken from privilege mode _y_ into privilege mode _x_, *x*PIE is set to the value of *x*IE; *x*IE is set to 0; and *x*PP is set to _y_. |  当从特权模式 y 进入特权模式 x 的 trap 时，xPIE 被设置为 yIE 的值，xIE 被清零，xPP 被设置为 y。 |
| `norm:mstatus_xret_op` | When executing an *x*RET instruction, supposing *x*PP holds the value _y_, *x*IE is set to *x*PIE; the privilege mode is changed to _y_; *x*PIE is set to 1; and *x*PP is set to the least-privileged supported mode (U if U-mode is implemented, else M). If *y*{ne}M, *x*RET also sets MPRV=0. |  xRET 指令将 xIE 设置为 xPIE 的值，将 xPIE 置 1，并将特权模式恢复为 xPP 中保存的模式。 |
| `norm:mstatus_xpp_warl` | *x*PP fields are **WARL** fields that can hold only privilege mode _x_ and any implemented privilege mode lower than _x_. |  xPP 字段是 WARL 字段。 |
| `norm:mstatus_xpp_rdonly0` | If privilege mode _x_ is not implemented, then *x*PP must be read-only 0. |  如果仅实现 M 模式，则 MPP 为只读（固定为 M 模式编码 11）。 |

#### Double Trap Control in mstatus Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Status (mstatus and mstatush) Registers > Double Trap Control in mstatus Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mstatus_mdt_sz_warl` | The M-mode-disable-trap (`MDT`) bit is a WARL field introduced by the Smdbltrp extension. |  MDT 字段为 1 位 WARL 字段，用于控制双重 trap 行为。 |
| `norm:mstatus_mdt_rst` | Upon reset, the `MDT` field is set to 1. |  复位时 MDT 被清零。 |
| `norm:mstatus_mie_clr_by_mdt` | When the `MDT` bit is set to 1 by an explicit CSR write, the `MIE` (Machine Interrupt Enable) bit is cleared to 0. |  当 MDT 为 1 时，在 RV32 下 MIE 被清零。 |
| `norm:mstatus_mie_clr_by_mdt_rv64` | For RV64, this clearing occurs regardless of the value written, if any, to the `MIE` bit by the same write. |  当 MDT 为 1 时，在 RV64 下 MIE 被清零。 |
| `norm:mstatus_mie_set_mdt_0` | The `MIE` bit can only be set to 1 by an explicit CSR write if the `MDT` bit is already 0 |  当 MIE 被显式写为 1 时（RV32），MDT 被清零。 |
| `norm:mstatus_mie_set_mdt_0_rv64` | or, for RV64, is being set to 0 by the same write |  当 MIE 被显式写为 1 时（RV64），MDT 被清零。 |
| `norm:trap_exp` | When a trap is to be taken into M-mode, if the `MDT` bit is currently 0, it is then set to 1, and the trap is delivered as expected. |  当 trap 进入 M 模式时，如果 MDT 已为 1，则发生双重 trap。 |
| `norm:trap_unexp_mdt_1` | However, if `MDT` is already set to 1, then this is an _unexpected trap_. |  如果 MDT 为 1 时发生 trap，则产生不可恢复的双重 trap 条件。 |
| `norm:trap_unexp_rnmi` | When the Smrnmi extension is implemented, a trap caused by an RNMI is not considered an _unexpected trap_ irrespective of the state of the `MDT` bit. |  如果实现了 Smrnmi 扩展，不可恢复的双重 trap 会导致跳转到 RNMI 处理程序。 |
| `norm:mstatus_mdt_not_set_rnmi` | A trap caused by an RNMI does not set the `MDT` bit. |  如果未实现 Smrnmi 扩展，不可恢复的双重 trap 会导致实现定义的行为。 |
| `norm:trap_unexp_mnstatus_nmie_0` | a trap that occurs when executing in M-mode with `mnstatus.NMIE` set to 0 is an _unexpected trap_. |  如果 mnstatus.NMIE 为 0 时发生 trap，则产生不可恢复的条件。 |
| `norm:trap_unexp_hndl_lead-in` | In the event of a _unexpected trap_, the handling is as follows: |  不可恢复的双重 trap 的处理方式取决于是否实现了 Smrnmi 扩展： |
| `norm:trap_unexp_hndl_rnmi` | When the Smrnmi extension is implemented and `mnstatus.NMIE` is 1, the hart traps to the RNMI handler. To deliver this trap, the `mnepc` and `mncause` registers are written with the values that the _unexpected trap_ would have written to the `mepc` and `mcause` registers respectively. The privilege mode information fields in the `mnstatus` register are written to indicate M-mode and its `NMIE` field is set to 0. |  如果实现了 Smrnmi，不可恢复的双重 trap 会跳转到 RNMI 处理程序。 |
| `norm:trap_unexp_hndl_no_rnmi` | When the Smrnmi extension is not implemented, or if the Smrnmi extension is implemented and `mnstatus.NMIE` is 0, the hart enters a critical-error state without updating any architectural state, including the `pc`. This state involves ceasing execution, disabling all interrupts (including NMIs), and asserting a `critical-error` signal to the platform. |  如果未实现 Smrnmi，不可恢复的双重 trap 会导致实现定义的行为（通常是 hart 停止）。 |
| `norm:critical_error` | The actions performed by the platform when a hart asserts a `critical-error` signal are platform-specific. The range of possible actions include restarting the affected hart or restarting the entire platform, among others. |  不可恢复的双重 trap 被视为关键错误。 |
| `norm:mstatus_mdt_clr_mret_sret` | The `MDT` bit is set to 0. |  MRET 和 SRET 指令会将 MDT 清零。 |
| `norm:sstatus_sdt_clr_mret_sret` | If the Ssdbltrp extension is also implemented, and the new privilege mode is U, VS, or VU, then `sstatus.SDT` is also set to 0. |  S-mode 的 SDT 字段在 MRET/SRET 返回到 S 或更低模式时被清零。 |
| `norm:vsstatus_sdt_clr_mret_sret` | Additionally, if it is VU, then `vsstatus.SDT` is also set to 0. |  VS-mode 的 SDT 字段在 MRET/SRET 返回到 VS 或更低模式时被清零。 |
| `norm:mstatus_mdt_clr_mnret` | When the Smdbltrp extension is implemented, the `MNRET` instruction, provided by the Smrnmi extension, sets the `MDT` bit to 0 if the new privilege mode is not M. |  MNRET 指令会将 MDT 清零。 |
| `norm:sstatus_sdt_clr_mnret` | If the Ssdbltrp extension is also implemented, and the new privilege mode is U, VS, or VU, then `sstatus.SDT` is also set to 0. |  S-mode 的 SDT 字段在 MNRET 返回到 S 或更低模式时被清零。 |
| `norm:vsstatus_sdt_clr_mnret` | Additionally, if it is VU, then `vsstatus.SDT` is also set to 0. |  VS-mode 的 SDT 字段在 MNRET 返回到 VS 或更低模式时被清零。 |

#### Base ISA Control in mstatus Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Status (mstatus and mstatush) Registers > Base ISA Control in mstatus Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mstatus_sxl_uxl_warl_op` | For RV64 harts, the SXL and UXL fields are **WARL** fields that control the value of XLEN for S-mode and U-mode, respectively. |  SXL 和 UXL 字段是 WARL 字段，编码 S 模式和 U 模式的有效 XLEN。 |
| `norm:mstatus_sxl_uxl_enc` | The encoding of these fields is the same as the MXL field of `misa`, shown in norm:misa_mxl_enc. |  SXL 和 UXL 字段的编码与 MXL 字段相同（1=32位，2=64位，3=128位）。 |
| `norm:sxlen_uxlen` | The effective XLEN in S-mode and U-mode are termed _SXLEN_ and _UXLEN_, respectively. |  当 SXL/UXL 设置为某个值时，在 S/U 模式下执行时 SXLEN/UXLEN 等于该值对应的宽度。 |
| `norm:mstatus_sxl_acc_mxlen64` | When MXLEN=64, if S-mode is not supported, then SXL is read-only zero. Otherwise, it is a **WARL** field that encodes the current value of SXLEN. |  仅当 MXLEN>=64 时，SXL 字段才存在。 |
| `norm:mstatus_sxl_rdonly_mxlen64` | an implementation may make SXL be a read-only field whose value always ensures that SXLEN=MXLEN. |  如果 MXLEN=32，则 SXL 字段不存在（隐含为 32 位）。 |
| `norm:mstatus_uxl_acc_mxlen64` | When MXLEN=64, if U-mode is not supported, then UXL is read-only zero. Otherwise, it is a **WARL** field that encodes the current value of UXLEN. |  仅当 MXLEN>=64 时，UXL 字段才存在。 |
| `norm:mstatus_uxl_rdonly_mxlen64` | an implementation may make UXL be a read-only field whose value always ensures that UXLEN=MXLEN or UXLEN=SXLEN. |  如果 MXLEN=32，则 UXL 字段不存在（隐含为 32 位）。 |

#### Memory Privilege in mstatus Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Status (mstatus and mstatush) Registers > Memory Privilege in mstatus Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mstatus_mprv_ldst_op` | When MPRV=0, explicit memory accesses behave as normal, using the translation and protection mechanisms of the current privilege mode. When MPRV=1, load and store memory addresses are translated and protected, and endianness is applied, as though the current privilege mode were set to MPP. |  当 MPRV=1 时，load 和 store 的内存访问权限检查使用 MPP 字段中的特权模式，而非当前特权模式。 |
| `norm:mstatus_mprv_inst_xlat_op` | Instruction address-translation and protection are unaffected by the setting of MPRV. |  MPRV 不影响指令获取的权限检查，指令获取始终使用当前特权模式。 |
| `norm:mstatus_mprv_rdonly0_no_umode` | MPRV is read-only 0 if U-mode is not supported; otherwise, it must be writable. |  如果未实现 U 模式，则 MPRV 为只读零。 |
| `norm:mstatus_mprv_clr_mret_sret_less_priv` | An MRET or SRET instruction that changes the privilege mode to a mode less privileged than M also sets MPRV=0. |  当 MRET/SRET 将特权模式切换到低于 M 的模式时，MPRV 被清零。 |
| `norm:mstatus_mxr_op` | When MXR=0, only loads from pages marked readable (R=1 in sv32pte) will succeed. When MXR=1, loads from pages marked either readable or executable (R=1 or X=1) will succeed. MXR has no effect when page-based virtual memory is not in effect. |  当 MXR=1 时，对仅具有执行权限的页面的 load 访问也被允许。 |
| `norm:mstatus_mxr_rdonly0_no_smode` | MXR is read-only 0 if S-mode is not supported or if `satp`.MODE is read-only 0; otherwise, it must be writable. |  如果未实现 S 模式，则 MXR 为只读零。 |
| `norm:mstatus_sum_op` | When SUM=0, S-mode memory accesses to pages that are accessible by U-mode (U=1 in sv32pte) will fault. When SUM=1, these accesses are permitted. |  当 SUM=1 时，S 模式可以访问 U 模式的页面。 |
| `norm:mstatus_sum_op_no-vm` | SUM has no effect when page-based virtual memory is not in effect. |  当未启用虚拟内存时，SUM 无效。 |
| `norm:mstatus_sum_op_mprv_mpp` | while SUM is ordinarily ignored when not executing in S-mode, it _is_ in effect when MPRV=1 and MPP=S. |  当 MPRV=1 且 MPP=S 时，SUM 也适用于 load/store 的权限检查。 |
| `norm:mstatus_sum_rdonly0` | SUM is read-only 0 if S-mode is not supported or if `satp`.MODE is read-only 0; otherwise, it must be writable. |  如果未实现 S 模式或未实现基于页面的虚拟内存，则 SUM 为只读零。 |

#### Endianness Control in mstatus and mstatush Registers

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Status (mstatus and mstatush) Registers > Endianness Control in mstatus and mstatush Registers*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mstatus_mstatush_xbe_warl` | The MBE, SBE, and UBE bits in `mstatus` and `mstatush` are **WARL** fields that control the endianness of memory accesses other than instruction fetches. Each may be read/write or read-only. |  MBE、SBE、UBE 字段是 WARL 字段，控制对应模式的数据访问字节序。0=小端，1=大端。 |
| `norm:endianness_inst_fetch_little` | Instruction fetches are always little-endian. |  指令获取始终为小端序，不受字节序控制位影响。 |
| `norm:mstatus_sbe_implicit` | For _implicit_ accesses to supervisor-level memory management data structures, such as page tables, endianness is always controlled by SBE. |  SBE 控制 S 模式下隐式页表访问的字节序。 |
| `norm:mstatus_sbe_change_fence` | Since changing SBE alters the implementation’s interpretation of these data structures, if any such data structures remain in use across a change to SBE, M-mode software must follow such a change to SBE by executing an SFENCE.VMA instruction with _rs1_=`x0` and _rs2_=`x0`. |  修改 SBE 后需要执行 FENCE 指令以确保一致性。 |
| `norm:mstatus_sbe_rocopy` | If S-mode is supported, an implementation may make SBE be a read-only copy of MBE. |  SBE 是 `mstatus` 中 SBE 字段的只读副本。 |
| `norm:mstatus_ube_rocopy` | If U-mode is supported, an implementation may make UBE be a read-only copy of either MBE or SBE. |  UBE 控制 U 模式下数据访问的字节序。 |

#### Virtualization Support in mstatus Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Status (mstatus and mstatush) Registers > Virtualization Support in mstatus Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mstatus_tw_warl` | The TW (Timeout Wait) bit is a **WARL** field that supports intercepting the WFI instruction (see wfi). |  TW 字段是 1 位 WARL 字段。 |
| `norm:mstatus_tw_op` | When TW=0, the WFI instruction may execute in modes less privileged than M when not prevented for some other reason. When TW=1, then if WFI is executed in any less-privileged mode, and it does not complete within an implementation-specific, bounded time limit, the WFI instruction causes an illegal-instruction exception. |  当 TW=1 时，S 模式下执行 WFI 指令会在有界时间内触发非法指令异常。 |
| `norm:mstatus_tw_always_illegal` | An implementation may have WFI always raise an illegal-instruction exception in modes less privileged than M when TW=1, even if there are pending globally-disabled interrupts when the instruction is executed. |  当 TW=1 时，无论是否有中断挂起，WFI 都可能触发非法指令异常。 |
| `norm:mstatus_tw_acc` | TW is read-only 0 when there are no modes less privileged than M; otherwise, it must be writable. |  TW 字段仅在实现 S 模式时存在。 |
| `norm:mstatus_tw_umode_op` | When S-mode is implemented, then executing WFI in U-mode causes an illegal-instruction exception, regardless of the value of the TW bit, unless the instruction completes within an implementation-specific, bounded time limit. |  在 U 模式下执行 WFI 始终触发非法指令异常（除非另有规定）。 |
| `norm:mstatus_tsr_warl` | The TSR (Trap SRET) bit is a **WARL** field that supports intercepting the supervisor exception return instruction, SRET. |  TSR 字段是 1 位 WARL 字段。 |
| `norm:mstatus_tsr_op` | When TSR=1, attempts to execute SRET while executing in S-mode will raise an illegal-instruction exception. When TSR=0, this operation is permitted in S-mode. |  当 TSR=1 时，S 模式下执行 SRET 指令会触发非法指令异常。 |
| `norm:mstatus_tsr_acc` | TSR is read-only 0 when S-mode is not supported; otherwise, it must be writable. |  TSR 字段仅在实现 S 模式时存在。 |

#### Extension Context Status in mstatus Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Status (mstatus and mstatush) Registers > Extension Context Status in mstatus Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mstatus_fs_vs_warl` | The FS[1:0] and VS[1:0] **WARL** fields |  FS 和 VS 字段是 2 位 WARL 字段，编码浮点和向量扩展的上下文状态。 |
| `norm:mstatus_fs_op` | The FS field encodes the status of the floating-point unit state, including the floating-point registers `f0`–`f31` and the CSRs `fcsr`, `frm`, and `fflags`. |  FS 字段编码浮点扩展的上下文状态：0=Off，1=Initial，2=Clean，3=Dirty。 |
| `norm:mstatus_vs_op` | The VS field encodes the status of the vector extension state, including the vector registers `v0`–`v31` and the CSRs `vcsr`, `vxrm`, `vxsat`, `vstart`, `vl`, `vtype`, and `vlenb`. |  VS 字段编码向量扩展的上下文状态：0=Off，1=Initial，2=Clean，3=Dirty。 |
| `norm:mstatus_xs_op1` | The XS field encodes the status of additional user-mode extensions and associated state. |  XS 字段编码用户自定义扩展的上下文状态。 |
| `norm:mstatus_fs_acc2` | If neither the F extension nor S-mode is implemented, then FS is read-only zero. |  FS 字段仅在实现 F/D/Q 扩展时存在。 |
| `norm:mstatus_fs_rdonly0_s_no_f` | If S-mode is implemented but the F extension is not, FS may optionally be read-only zero. |  如果未实现浮点扩展，则 FS 为只读零。 |
| `norm:mstatus_vs_acc2` | If neither the `v` registers nor S-mode is implemented, then VS is read-only zero. |  VS 字段仅在实现 V 扩展时存在。 |
| `norm:mstatus_vs_rdonly0_s_no_v` | If S-mode is implemented but the `v` registers are not, VS may optionally be read-only zero. |  如果未实现向量扩展，则 VS 为只读零。 |
| `norm:mstatus_xs_acc` | In harts without additional user extensions requiring new state, the XS field is read-only zero. |  XS 字段是 2 位只读字段。 |
| `norm:mstatus_xs_equiv` | Every additional extension with state provides a CSR field that encodes the equivalent of the XS states. |  XS 字段的状态编码与 FS/VS 相同（Off/Initial/Clean/Dirty）。 |
| `norm:mstatus_xs_op2` | The XS field represents a summary of all extensions' status as shown in norm:mstatus_fs_vs_xs_enc. |  XS 字段反映所有用户自定义扩展的上下文状态汇总。 |
| `norm:mstatus_sd_acc` | The SD bit is a read-only bit |  SD 位是只读位。 |
| `norm:mstatus_sd_op1` | summarizes whether either the FS, VS, or XS fields signal the presence of some dirty state that will require saving extended user context to memory. |  当 FS、VS 或 XS 中任何一个为 Dirty（3）时，SD 位被置 1。即 SD=(FS==Dirty OR VS==Dirty OR XS==Dirty)。 |
| `norm:mstatus_sd_rdonly0` | If FS, XS, and VS are all read-only zero, then SD is also always zero. |  如果未实现 FS、VS 和 XS 字段，则 SD 为只读零。 |
| `norm:mstatus_fs_vs_xs_off_op` | When an extension's status is set to Off, any instruction that attempts to read or write the corresponding state will cause an illegal-instruction exception. |  当 FS/VS/XS 为 Off（0）时，对应扩展的指令和 CSR 访问会触发非法指令异常。 |
| `norm:mstatus_fs_vs_xs_initial_op` | When the status is Initial, the corresponding state should have an initial constant value. |  当 FS/VS/XS 为 Initial（1）时，对应扩展可用但上下文处于初始状态。 |
| `norm:mstatus_fs_vs_xs_clean_op` | When the status is Clean, the corresponding state is potentially different from the initial value, but matches the last value stored on a context swap. |  当 FS/VS/XS 为 Clean（2）时，对应扩展可用且上下文自上次保存以来未被修改。 |
| `norm:mstatus_fs_vs_xs_dirty_op` | When the status is Dirty, the corresponding state has potentially been modified since the last context save. |  当 FS/VS/XS 为 Dirty（3）时，对应扩展可用且上下文可能已被修改，需要在上下文切换时保存。 |
| `norm:mstatus_fs_vs_xs_update_indep_priv` | The status fields will also be updated during execution of instructions, regardless of privilege mode. |  FS/VS/XS 字段的更新与当前特权模式无关。 |
| `norm:mstatus_fs_wr` | Changing the setting of FS has no effect on the contents of the floating-point register state. In particular, setting FS=Off does not destroy the state, nor does setting FS=Initial clear the contents. |  FS 字段可由软件直接写入以管理上下文状态。 |
| `norm:mstatus_vs_wr` | the setting of VS has no effect on the contents of the vector register state. |  VS 字段可由软件直接写入以管理上下文状态。 |
| `norm:mstatus_fs_imprecise` | Implementations may choose to track the dirtiness of the floating-point register state imprecisely by reporting the state to be dirty even when it has not been modified. On some implementations, some instructions that do not mutate the floating-point state may cause the state to transition from Initial or Clean to Dirty. |  FS 字段从 Clean 到 Dirty 的转变可能不精确（可能在浮点指令执行前就被标记为 Dirty）。 |
| `norm:mstatus_fs_no_dirty_track` | dirtiness might not be tracked at all, in which case the valid FS states are Off and Dirty, and an attempt to set FS to Initial or Clean causes it to be set to Dirty. |  某些实现可能不跟踪浮点上下文修改，始终将 FS 报告为 Dirty。 |
| `norm:mstatus_fs_no_change_dirty` | If an instruction explicitly or implicitly writes a floating-point register or the `fcsr` but does not alter its contents, and FS=Initial or FS=Clean, it is implementation-defined whether FS transitions to Dirty. |  如果 FS 已经是 Dirty，则执行浮点指令不会改变 FS 的值。 |
| `norm:mstatus_vs_imprecise` | Implementations may choose to track the dirtiness of the vector register state in an analogous imprecise fashion, including possibly setting VS to Dirty when software attempts to set VS=Initial or VS=Clean. |  VS 字段从 Clean 到 Dirty 的转变可能不精确。 |
| `norm:mstatus_vs_no_change_dirty` | When VS=Initial or VS=Clean, it is implementation-defined whether an instruction that writes a vector register or vector CSR but does not alter its contents causes VS to transition to Dirty. |  如果 VS 已经是 Dirty，则执行向量指令不会改变 VS 的值。 |

#### Previous Expected Landing Pad (ELP) State in mstatus Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Status (mstatus and mstatush) Registers > Previous Expected Landing Pad (ELP) State in mstatus Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mstatus_spelp_mpelp_op` | The Zicfilp extension adds the `SPELP` and `MPELP` fields that hold the previous `ELP`, and are updated as specified in ZICFILP_FORWARD_TRAPS. |  Zicfilp 扩展添加 SPELP 和 MPELP 字段，保存之前的 ELP 状态，按前向 trap 规则更新。 |
| `norm:mstatus_spelp_mpelp_enc_lead-in` | The ***x***`PELP` fields are encoded as follows: |  xPELP 字段编码如下：0=NO_LP_EXPECTED（不期望着陆垫指令），1=LP_EXPECTED（期望着陆垫指令）。 |

#### Machine Trap-Vector Base-Address (mtvec) Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Trap-Vector Base-Address (mtvec) Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mtvec_sz_warl_acc` | The `mtvec` register is an MXLEN-bit **WARL** read/write register that holds trap vector configuration, consisting of a vector base address (BASE) and a vector mode (MODE). |  `mtvec` 寄存器是一个 MXLEN 位 **WARL** 读写寄存器，包含 trap 向量配置，由向量基地址（BASE）和向量模式（MODE）组成。 |
| `norm:mtvec_mandatory` | The `mtvec` register must always be implemented |  `mtvec` 寄存器必须始终被实现。 |
| `norm:mtvec_rdonly` | can contain a read-only value. |  `mtvec` 可以包含只读值。 |
| `norm:mtvec_base_align_4B` | The value in the BASE field must always be aligned on a 4-byte boundary |  BASE 字段的值必须始终在 4 字节边界上对齐。 |
| `norm:mtvec_base_align_func_mode` | the MODE setting may impose additional alignment constraints on the value in the BASE field. |  MODE 设置可能会对 BASE 字段的值施加额外的对齐约束。 |
| `norm:mtvec_mode_direct_op` | When MODE=Direct, all traps into machine mode cause the `pc` to be set to the address in the BASE field. |  当 MODE=Direct 时，所有进入 M 模式的 trap 都将 pc 设置为 BASE 字段中的地址。 |
| `norm:mtvec_mode_vectored_op` | When MODE=Vectored, all synchronous exceptions into machine mode cause the `pc` to be set to the address in the BASE field, whereas interrupts cause the `pc` to be set to the address in the BASE field plus four times the interrupt cause number. |  当 MODE=Vectored 时，同步异常将 pc 设置为 BASE，而中断将 pc 设置为 BASE+4*cause。 |
| `norm:rst_and_nmi_addr` | Reset and NMI vector locations are given in a platform specification. |  复位和 NMI 向量位置由平台规范给出。 |

#### Machine Trap Delegation (medeleg and mideleg) Registers

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Trap Delegation (medeleg and mideleg) Registers*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:trap_def_M_mode` | By default, all traps at any privilege level are handled in machine mode |  默认情况下，任何特权级别的所有 trap 都在 M 模式中处理。 |
| `norm:medeleg_mideleg_op1` | implementations can provide individual read/write bits within `medeleg` and `mideleg` to indicate that certain exceptions and interrupts should be processed directly by a lower privilege level. |  实现可以在 `medeleg` 和 `mideleg` 中提供独立的读写位，以指示某些异常和中断应由较低特权级别直接处理。 |
| `norm:medeleg_sz_acc` | The machine exception delegation register (`medeleg`) is a 64-bit read/write register. |  机器异常委托寄存器（`medeleg`）是一个 64 位读写寄存器。 |
| `norm:mideleg_sz_acc` | The machine interrupt delegation (`mideleg`) register is an MXLEN-bit read/write register. |  机器中断委托寄存器（`mideleg`）是一个 MXLEN 位读写寄存器。 |
| `norm:medeleg_mideleg_mandatory_S_mode` | In harts with S-mode, the `medeleg` and `mideleg` registers must exist |  在具有 S 模式的 hart 中，`medeleg` 和 `mideleg` 寄存器必须存在。 |
| `norm:medeleg_mideleg_op2` | setting a bit in `medeleg` or `mideleg` will delegate the corresponding trap, when occurring in S-mode or U-mode, to the S-mode trap handler. |  设置 `medeleg` 或 `mideleg` 中的某一位，将把在 S 模式或 U 模式中发生的对应 trap 委托给 S 模式 trap 处理程序。 |
| `norm:medeleg_mideleg_omit_wo_S_mode` | In harts without S-mode, the `medeleg` and `mideleg` registers should not exist. |  在没有 S 模式的 hart 中，`medeleg` 和 `mideleg` 寄存器不应存在。 |
| `norm:trap_del_S-mode` | When a trap is delegated to S-mode |  当 trap 被委托给 S 模式时： |
| `norm:trap_del_S_mode_op` | the `scause` register is written with the trap cause; the `sepc` register is written with the virtual address of the instruction that took the trap; the `stval` register is written with an exception-specific datum; the SPP field of `mstatus` is written with the active privilege mode at the time of the trap; the SPIE field of `mstatus` is written with the value of the SIE field at the time of the trap; and the SIE field of `mstatus` is cleared. |  `scause` 被写入 trap 原因；`sepc` 被写入触发 trap 的指令的虚拟地址；`stval` 被写入异常特定数据；`mstatus` 的 SPP 被写入 trap 时的活动特权模式；SPIE 被写入 SIE 的值；SIE 被清零。 |
| `norm:trap_del_S_mode_no_M_mode` | The `mcause`, `mepc`, and `mtval` registers and the MPP and MPIE fields of `mstatus` are not written. |  `mcause`、`mepc`、`mtval` 寄存器以及 `mstatus` 的 MPP 和 MPIE 字段不会被写入。 |
| `norm:medeleg_mideleg_warl` | An implementation can choose to subset the delegatable traps, with the supported delegatable bits found by writing one to every bit location, then reading back the value in `medeleg` or `mideleg` to see which bit positions hold a one. |  实现可以选择可委托 trap 的子集，通过向每个位位置写入 1 然后读回来确定支持的位。 |
| `norm:medeleg_no_rd1` | An implementation shall not have any bits of `medeleg` be read-only one |  实现不得将 `medeleg` 的任何位设为只读 1，即任何可委托的同步 trap 必须支持不委托。 |
| `norm:mideleg_no_rd1` | an implementation shall not fix as read-only one any bits of `mideleg` corresponding to machine-level interrupts |  实现不得将 `mideleg` 中对应机器级中断的任何位固定为只读 1。 |
| `norm:mideleg_rd1_lower_level` | may do so for lower-level interrupts |  实现可以将 `mideleg` 中对应低级别中断的位设为只读 1。 |
| `norm:trap_never_trans_lower` | Traps never transition from a more-privileged mode to a less-privileged mode. For example, if M-mode has delegated illegal-instruction exceptions to S-mode, and M-mode software later executes an illegal instruction, the trap is taken in M-mode, rather than being delegated to S-mode. |  Trap 永远不会从高特权模式转移到低特权模式。例如，如果 M 模式已将非法指令异常委托给 S 模式，而 M 模式软件后来执行了非法指令，则该 trap 在 M 模式中处理。 |
| `norm:trap_horiz` | traps may be taken horizontally. Using the same example, if M-mode has delegated illegal-instruction exceptions to S-mode, and S-mode software later executes an illegal instruction, the trap is taken in S-mode. |  Trap 可以水平发生。例如，如果 M 模式已将非法指令异常委托给 S 模式，而 S 模式软件执行了非法指令，则该 trap 在 S 模式中处理。 |
| `norm:trap_del_intr_priv_lvl` | Delegated interrupts result in the interrupt being masked at the delegator privilege level. For example, if the supervisor timer interrupt (STI) is delegated to S-mode by setting `mideleg`[5], STIs will not be taken when executing in M-mode. By contrast, if `mideleg`[5] is clear, STIs can be taken in any mode and regardless of current mode will transfer control to M-mode. |  被委托的中断在委托者特权级别被屏蔽。例如，如果 STI 通过 mideleg[5] 委托给 S 模式，则在 M 模式下不会接收 STI。 |
| `norm:medeleg_enc_txt` | `medeleg` has a bit position allocated for every synchronous exception shown in norm:mcause_exccode_enc_img, with the index of the bit position equal to the value returned in the `mcause` register |  `medeleg` 为每个同步异常分配一个位位置，位位置索引等于 `mcause` 寄存器返回的值。 |
| `norm:medelegh_sz_acc_enc_xlen32` | When XLEN=32, `medelegh` is a 32-bit read/write register that aliases bits 63:32 of `medeleg`. |  当 XLEN=32 时，`medelegh` 是一个 32 位读写寄存器，映射 `medeleg` 的第 63:32 位。 |
| `norm:medelegh_omit_xlen64` | The `medelegh` register does not exist when XLEN=64. |  当 XLEN=64 时，`medelegh` 寄存器不存在。 |
| `norm:mideleg_enc_txt` | `mideleg` holds trap delegation bits for individual interrupts, with the layout of bits matching those in the `mip` register |  `mideleg` 保存各个中断的 trap 委托位，位布局与 `mip` 寄存器匹配。 |
| `norm:medeleg_when_rd0` | For exceptions that cannot occur in less privileged modes, the corresponding `medeleg` bits should be read-only zero. In particular, `medeleg`[11] is read-only zero. |  对于在低特权模式下不可能发生的异常，对应的 `medeleg` 位应为只读零。特别是 `medeleg`[11] 为只读零。 |
| `norm:medeleg_16_no_rd0` | The `medeleg`[16] is read-only zero as double trap is not delegatable. |  `medeleg`[16] 为只读零，因为双重 trap 不可委托。 |

#### Machine Interrupt (mip and mie) Registers

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Interrupt (mip and mie) Registers*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mip_sz_acc` | The `mip` register is an MXLEN-bit read/write register containing information on pending interrupts |  `mip` 寄存器是一个 MXLEN 位读写寄存器，包含挂起中断信息。 |
| `norm:mie_sz_acc` | `mie` is the corresponding MXLEN-bit read/write register containing interrupt enable bits. |  `mie` 是对应的 MXLEN 位读写寄存器，包含中断使能位。 |
| `norm:mip_mie_enc_txt` | Interrupt cause number _i_ (as reported in CSR `mcause`, mcause) corresponds with bit _i_ in both `mip` and `mie`. Bits 15:0 are allocated to standard interrupt causes only, while bits 16 and above are designated for platform use. |  中断原因号 i（如 `mcause` 中报告）对应 `mip` 和 `mie` 中的第 i 位。第 15:0 位分配给标准中断原因，第 16 位及以上用于平台使用。 |
| `norm:intr_mip_mie_op` | An interrupt _i_ will trap to M-mode (causing the privilege mode to change to M-mode) if all of the following are true: (a) either the current privilege mode is M and the MIE bit in the `mstatus` register is set, or the current privilege mode has less privilege than M-mode; (b) bit _i_ is set in both `mip` and `mie`; and (c) if register `mideleg` exists, bit _i_ is not set in `mideleg`. |  如果以下条件全部满足，中断 i 将 trap 到 M 模式：(a) 当前特权模式为 M 且 MIE=1，或当前特权模式低于 M；(b) `mip` 和 `mie` 中第 i 位都被置位；(c) 如果 `mideleg` 存在，第 i 位未被设置。 |
| `norm:intr_mip_mie_bounded_time` | These conditions for an interrupt trap to occur must be evaluated in a bounded amount of time from when an interrupt becomes, or ceases to be, pending in `mip`, |  中断 trap 的条件必须在 `mip` 中中断变为挂起或取消挂起后的有界时间内被评估。 |
| `norm:intr_mip_mie_xret_csrwr` | be evaluated immediately following the execution of an *x*RET instruction or an explicit write to a CSR on which these interrupt trap conditions expressly depend (including `mip`, `mie`, `mstatus`, and `mideleg`). |  中断条件还必须在执行 xRET 指令或显式写入相关 CSR（包括 `mip`、`mie`、`mstatus`、`mideleg`）后立即被评估。 |
| `norm:mip_bits_wr_or_rdonly` | The `mip` register must always be implemented, but can contain read-only zeros in any position, indicating that interrupt can never become pending. Each individual bit in register `mip` may be writable or may be read-only. |  `mip` 寄存器必须始终被实现，但可以在任何位置包含只读零。每个位可以是可写的或只读的。 |
| `norm:mip_bits_wr_op` | When bit _i_ in `mip` is writable, a pending interrupt _i_ can be cleared by writing 0 to this bit. |  当 `mip` 中第 i 位可写时，可以通过向该位写入 0 来清除挂起的中断 i。 |
| `norm:mip_bits_rdonly_op` | If interrupt _i_ can become pending but bit _i_ in `mip` is read-only, the implementation must provide some other mechanism for clearing the pending interrupt. |  如果中断 i 可以变为挂起但 `mip` 中第 i 位是只读的，则实现必须提供其他机制来清除挂起的中断。 |
| `norm:mie_bits_wr` | The `mie` register must always be implemented. A bit in `mie` must be writable if the corresponding interrupt can ever become pending. |  `mie` 寄存器必须始终被实现。如果对应中断可以变为挂起，则 `mie` 中的位必须是可写的。 |
| `norm:mie_bits_rdonly0` | Bits of `mie` that are not writable must be read-only zero. |  `mie` 中不可写的位必须为只读零。 |
| `norm:mip_mie_std_enc_txt` | The standard portions (bits 15:0) of the `mip` and `mie` registers are formatted as shown in norm:mip_std_enc_img and norm:mie_std_enc_img respectively. |  `mip` 和 `mie` 寄存器的标准部分（第 15:0 位）按照标准格式排列。 |
| `norm:mip_meip_mie_meie_op` | Bits `mip`.MEIP and `mie`.MEIE are the interrupt-pending and interrupt-enable bits for machine-level external interrupts. |  `mip`.MEIP 和 `mie`.MEIE 分别是机器级外部中断的挂起位和使能位。 |
| `norm:mip_meip_rdonly` | MEIP is read-only in `mip`, and is set and cleared by a platform-specific interrupt controller. |  MEIP 在 `mip` 中为只读，由平台特定的中断控制器设置和清除。 |
| `norm:mip_mtip_mie_mtie_op` | Bits `mip`.MTIP and `mie`.MTIE are the interrupt-pending and interrupt-enable bits for machine timer interrupts. |  `mip`.MTIP 和 `mie`.MTIE 分别是机器定时器中断的挂起位和使能位。 |
| `norm:mip_mtip_rdonly` | MTIP is read-only in the `mip` register, and is cleared by writing to the memory-mapped machine-mode timer compare register. |  MTIP 在 `mip` 中为只读，通过写入内存映射的机器模式定时器比较寄存器来清除。 |
| `norm:mip_msip_mie_msie_op` | Bits `mip`.MSIP and `mie`.MSIE are the interrupt-pending and interrupt-enable bits for machine-level software interrupts. |  `mip`.MSIP 和 `mie`.MSIE 分别是机器级软件中断的挂起位和使能位。 |
| `norm:mip_msip_rdonly` | MSIP is read-only in `mip`, and is written by accesses to memory-mapped control registers, which are used to provide machine-level interprocessor interrupts. |  MSIP 在 `mip` 中为只读，通过访问内存映射控制寄存器来写入，用于提供机器级处理器间中断。 |
| `norm:msip_sz_acc` | A hart's memory-mapped `msip` register is a 32-bit read/write register |  hart 的内存映射 `msip` 寄存器是一个 32 位读写寄存器。 |
| `norm:msip_enc` | bits 31--1 read as zero and bit 0 contains the MSIP bit. |  第 31-1 位读为零，第 0 位包含 MSIP 位。 |
| `norm:msip_update_max_time` | When the memory-mapped `msip` register changes, it is guaranteed to be reflected in `mip`.MSIP eventually, but not necessarily immediately. |  当内存映射的 `msip` 寄存器变化时，保证最终（但不一定立即）反映在 `mip`.MSIP 中。 |
| `norm:mip_msip_mie_msie_maybe_rdonly0` | If a system has only one hart, or if a platform standard supports the delivery of machine-level interprocessor interrupts through external interrupts (MEI) instead, then `mip`.MSIP and `mie`.MSIE may both be read-only zeros. |  如果系统只有一个 hart，或平台标准支持通过外部中断传递机器级处理器间中断，则 `mip`.MSIP 和 `mie`.MSIE 可以都为只读零。 |
| `norm:mip_sxip_mie_sxie_rdonly0` | If supervisor mode is not implemented, bits SEIP, STIP, and SSIP of `mip` and SEIE, STIE, and SSIE of `mie` are read-only zeros. |  如果未实现监管者模式，`mip` 的 SEIP、STIP、SSIP 和 `mie` 的 SEIE、STIE、SSIE 为只读零。 |
| `norm:mip_seip_mie_seie_op` | If supervisor mode is implemented, bits `mip`.SEIP and `mie`.SEIE are the interrupt-pending and interrupt-enable bits for supervisor-level external interrupts. |  如果实现了监管者模式，`mip`.SEIP 和 `mie`.SEIE 分别是监管者级外部中断的挂起位和使能位。 |
| `norm:mip_seip_acc` | SEIP is writable in `mip` |  SEIP 在 `mip` 中可写，M 模式软件可以写入以向 S 模式指示外部中断挂起。此外，平台级中断控制器也可以生成监管者级外部中断。 |
| `norm:intr_sei_op` | Supervisor-level external interrupts are made pending based on the logical-OR of the software-writable SEIP bit and the signal from the external interrupt controller. |  监管者级外部中断的挂起状态基于软件可写 SEIP 位和外部中断控制器信号的逻辑或。 |
| `norm:mip_seip_rdcsr` | When `mip` is read with a CSR instruction, the value of the SEIP bit returned in the `rd` destination register is the logical-OR of the software-writable bit and the interrupt signal from the interrupt controller |  当用 CSR 指令读取 `mip` 时，返回的 SEIP 位值是软件可写位和中断控制器信号的逻辑或。 |
| `norm:mip_seip_wrcsr` | the signal from the interrupt controller is not used to calculate the value written to SEIP. Only the software-writable SEIP bit participates in the read-modify-write sequence of a CSRRS or CSRRC instruction. |  但中断控制器的信号不参与 CSRRS 或 CSRRC 指令的读-修改-写序列，只有软件可写的 SEIP 位参与。 |
| `norm:mip_stip_mie_stie_op` | If supervisor mode is implemented, its `mip`.STIP and `mie`.STIE are the interrupt-pending and interrupt-enable bits for supervisor-level timer interrupts. |  如果实现了监管者模式，`mip`.STIP 和 `mie`.STIE 分别是监管者级定时器中断的挂起位和使能位。 |
| `norm:mip_stip_no_stimecmp_acc` | If the stimecmp register is not implemented, STIP is writable in mip |  如果未实现 `stimecmp` 寄存器，STIP 在 `mip` 中可写。 |
| `norm:mip_stip_no_stimecmp_op2` | may be written by M-mode software to deliver timer interrupts to S-mode. |  M 模式软件可以写入 STIP 以向 S 模式传递定时器中断。 |
| `norm:mip_stip_stimecmp_acc` | If the `stimecmp` (supervisor-mode timer compare) register is implemented, STIP is read-only in mip |  如果实现了 `stimecmp` 寄存器，STIP 在 `mip` 中为只读。 |
| `norm:mip_stip_stimecmp_op2` | reflects the supervisor-level timer interrupt signal resulting from stimecmp. |  STIP 反映由 `stimecmp` 产生的监管者级定时器中断信号。 |
| `norm:mip_stip_stimecmp_clr` | This timer interrupt signal is cleared by writing `stimecmp` with a value greater than the current time value. |  该定时器中断信号通过向 `stimecmp` 写入大于当前时间值的值来清除。 |
| `norm:mip_ssip_mie_ssie_op` | If supervisor mode is implemented, bits `mip`.SSIP and `mie`.SSIE are the interrupt-pending and interrupt-enable bits for supervisor-level software interrupts. |  如果实现了监管者模式，`mip`.SSIP 和 `mie`.SSIE 分别是监管者级软件中断的挂起位和使能位。 |
| `norm:mip_ssip_acc` | SSIP is writable in `mip` |  SSIP 在 `mip` 中可写。 |
| `norm:mip_ssip_intr_ctrl` | may also be set to 1 by a platform-specific interrupt controller. |  SSIP 也可以被平台特定的中断控制器设置为 1。 |
| `norm:mip_lcofip_mie_lcofie_op` | If the Sscofpmf extension is implemented, bits `mip`.LCOFIP and `mie`.LCOFIE are the interrupt-pending and interrupt-enable bits for local-counter-overflow interrupts. |  如果实现了 Sscofpmf 扩展，`mip`.LCOFIP 和 `mie`.LCOFIE 分别是本地计数器溢出中断的挂起位和使能位。 |
| `norm:mip_lcofip_acc` | LCOFIP is read-write in `mip` |  LCOFIP 在 `mip` 中为读写。 |
| `norm:mip_lcofip_op2` | reflects the occurrence of a local counter-overflow overflow interrupt request resulting from any of the `mhpmevent*n*`.OF bits being set. |  LCOFIP 反映由任何 `mhpmevent_n`.OF 位置位导致的本地计数器溢出中断请求。 |
| `norm:mip_lcofip_mie_lcofie_rdonly0` | If the Sscofpmf extension is not implemented, `mip`.LCOFIP and `mie`.LCOFIE are read-only zeros. |  如果未实现 Sscofpmf 扩展，`mip`.LCOFIP 和 `mie`.LCOFIE 为只读零。 |
| `norm:intr_M_mode_pri` | Multiple simultaneous interrupts destined for M-mode are handled in the following decreasing priority order: MEI, MSI, MTI, SEI, SSI, STI, LCOFI. |  多个同时发往 M 模式的中断按以下递减优先级顺序处理：MEI、MSI、MTI、SEI、SSI、STI、LCOFI。 |

#### Hardware Performance Monitor

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Hardware Performance Monitor*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:m_mode_perf_monitoring` | M-mode includes a basic hardware performance-monitoring facility. |  M 模式提供硬件性能监控功能，包括 `mcycle`、`minstret` 和 `mhpmcounter` 计数器。 |
| `norm:mcycle_op` | The `mcycle` CSR counts the number of clock cycles executed by the processor core on which the hart is running. |  `mcycle` CSR 计算 hart 的时钟周期数。 |
| `norm:minstret_op` | The `minstret` CSR counts the number of instructions the hart has retired. |  `minstret` CSR 计算 hart 已退休的指令数。 |
| `norm:mcycle_minstret_sz` | The `mcycle` and `minstret` registers have 64-bit precision on all RV32 and RV64 harts. |  `mcycle` 和 `minstret` 是 64 位宽的 CSR。 |
| `norm:mcycle_minstret_rst` | The counter registers have an arbitrary value after the hart is reset, and can be written with a given value. |  `mcycle` 和 `minstret` 在复位后没有规定的值。 |
| `norm:mcycle_minstret_wr` | Any CSR write takes effect after the writing instruction has otherwise completed. |  `mcycle` 和 `minstret` 是读写寄存器。 |
| `norm:mcycle_shared` | The `mcycle` CSR may be shared between harts on the same core, in which case writes to `mcycle` will be visible to those harts. |  `mcycle` 与 `time` CSR 共享底层计数器（在某些实现中）。 |
| `norm:mhpmcounter_num` | The hardware performance monitor includes 29 additional 64-bit event counters, `mhpmcounter3`-`mhpmcounter31`. |  最多支持 29 个 `mhpmcounter` 计数器（`mhpmcounter3`-`mhpmcounter31`）。 |
| `norm:mhpmevent_sz_warl_op` | The event selector CSRs, `mhpmevent3`-`mhpmevent31`, are 64-bit **WARL** registers that control which event causes the corresponding counter to increment. |  `mhpmevent` 寄存器是 MXLEN 位 WARL 读写寄存器，用于编程计数器的事件选择。 |
| `norm:mhpmevent_enc` | The meaning of these events is defined by the platform, but event 0 is defined to mean "`no event.`" |  `mhpmevent` 的事件编码是实现定义的。 |
| `norm:mhpmcounter_mandatory` | All counters should be implemented |  `mhpmcounter` 计数器必须被实现（至少作为只读零）。 |
| `norm:mhpmcounter_mhpmevent_rdonly0` | a legal implementation is to make both the counter and its corresponding event selector be read-only 0. |  未实现的 `mhpmcounter` 和 `mhpmevent` 寄存器为只读零。 |
| `norm:mhpmcounter_warl` | The `mhpmcounters` are **WARL** registers |  `mhpmcounter` 寄存器是 WARL 字段。 |
| `norm:mhpmcounter_sz` | support up to 64 bits of precision on RV32 and RV64. |  `mhpmcounter` 计数器为 64 位宽。 |
| `norm:mcycleh_minstreth_mhpmh_op` | reads of the `mcycleh`, `minstreth`, `mhpmcounter*n*h`, and `mhpmevent*n*h` CSRs return bits 63-32 of the corresponding register, and writes change only bits 63-32. |  对于 RV32，`mcycleh`、`minstreth` 和 `mhpmcounter_nh` 是 32 位读写寄存器，保存对应 64 位计数器的高 32 位。 |
| `norm:mhpmeventh_presence` | The `mhpmevent*n*h` CSRs are provided only if the Sscofpmf extension is implemented. |  对于 RV32，`mhpmeventh` 寄存器保存 `mhpmevent` 的高 32 位。 |

#### Machine Counter-Enable (mcounteren) Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Counter-Enable (mcounteren) Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mcounteren_sz` | The counter-enable `mcounteren` register is a 32-bit register |  `mcounteren` 是一个 32 位读写寄存器。 |
| `norm:mcounteren_op` | controls the availability of the hardware performance-monitoring counters to the next-lower privileged mode. |  `mcounteren` 控制低特权模式对性能计数器的访问权限。 |
| `norm:mcounteren_inc_inaccessible` | The settings in this register only control accessibility. The act of reading or writing this register does not affect the underlying counters, which continue to increment even when not accessible. |  当 `mcounteren` 中对应位为 0 时，低特权模式访问对应计数器会触发非法指令异常。 |
| `norm:mcounteren_clr_ill_inst_exc` | When the CY, TM, IR, or HPM*n* bit in the `mcounteren` register is clear, attempts to read the `cycle`, `time`, `instret`, or `hpmcountern` register while executing in S-mode or U-mode will cause an illegal-instruction exception. |  当 `mcounteren` 中对应位为 0 时，在 S 或 U 模式下读取对应计数器会触发非法指令异常。 |
| `norm:mcounteren_set_nxt_priv` | When one of these bits is set, access to the corresponding register is permitted in the next implemented privilege mode (S-mode if implemented, otherwise U-mode). |  当 `mcounteren` 中对应位为 1 时，下一低特权模式可以访问对应计数器。 |
| `norm:mcounteren_tm_clr` | when the TM bit in the `mcounteren` register is clear, attempts to access the `stimecmp` or `vstimecmp` register while executing in a mode less privileged than M will cause an illegal-instruction exception. |  当 TM 位为 0 时，低特权模式访问 `time` CSR 会触发非法指令异常。 |
| `norm:mcounteren_tm_set` | When this bit is set, access to the `stimecmp` or `vstimecmp` register is permitted in S-mode if implemented, and access to the `vstimecmp` register (via `stimecmp`) is permitted in VS-mode if implemented and not otherwise prevented by the TM bit in `hcounteren`. |  当 TM 位为 1 时，低特权模式可以读取 `time` CSR。 |
| `norm:cycle_instret_hpmcounter_op_rdonly` | The `cycle`, `instret`, and `hpmcountern` CSRs are read-only shadows of `mcycle`, `minstret`, and `mhpmcounter n`, respectively. |  `cycle`、`instret` 和 `hpmcountern` CSR 是只读寄存器。 |
| `norm:time_op_rdonly` | The `time` CSR is a read-only shadow of the memory-mapped `mtime` register. |  `time` CSR 是只读寄存器。 |
| `norm:cycleh_instreth_hpmcounternh_op_rdonly` | when XLEN=32, the `cycleh`, `instreth` and `hpmcounternh` CSRs are read-only shadows of `mcycleh`, `minstreth` and `mhpmcounternh`, respectively. |  对于 RV32，`cycleh`、`instreth` 和 `hpmcounternh` 是只读寄存器，保存对应 64 位计数器的高 32 位。 |
| `norm:timeh_op_rdonly` | When XLEN=32, the `timeh` CSR is a read-only shadow of the upper 32 bits of the memory-mapped `mtime` register |  对于 RV32，`timeh` 是只读寄存器，保存 `time` 的高 32 位。 |
| `norm:time_csr_architectural_availability` | Implementations can convert reads of the `time` and `timeh` CSRs into loads to the memory-mapped `mtime` register, or emulate this functionality on behalf of less-privileged modes in M-mode software. |  `time` CSR 必须在所有实现中可用。 |
| `norm:mcounteren_flds_mandatory_warl` | In harts with U-mode, the `mcounteren` must be implemented, but all fields are **WARL** |  `mcounteren` 的所有字段都是 WARL 字段。 |
| `norm:mcounteren_flds_rdonly0` | may be read-only zero, indicating reads to the corresponding counter will cause an illegal-instruction exception when executing in a less-privileged mode. |  未实现的计数器对应的 `mcounteren` 位为只读零。 |
| `norm:mcounteren_presence` | In harts without U-mode, the `mcounteren` register should not exist. |  `mcounteren` 仅在实现 S 模式时存在。 |

#### Machine Counter-Inhibit (mcountinhibit) Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Counter-Inhibit (mcountinhibit) Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mcountinhibit_sz_warl_op1` | The counter-inhibit register `mcountinhibit` is a 32-bit **WARL** register that controls which of the hardware performance-monitoring counters increment. |  `mcountinhibit` 是一个 32 位 WARL 读写寄存器，用于控制计数器递增的抑制。 |
| `norm:mcountinhibit_only_inc` | The settings in this register only control whether the counters increment; their accessibility is not affected by the setting of this register. |  `mcountinhibit` 仅控制计数器是否递增，不影响其他功能。 |
| `norm:mcountinhibit_op2` | When the CY, IR, or HPM*n* bit in the `mcountinhibit` register is clear, the `mcycle`, `minstret`, or `mhpmcountern` register increments as usual. When the CY, IR, or HPM*n* bit is set, the corresponding counter does not increment. |  当 `mcountinhibit` 中第 i 位为 1 时，对应计数器不递增。 |
| `norm:mcountinhibit_cy_shared` | The `mcycle` CSR may be shared between harts on the same core, in which case the `mcountinhibit.CY` field is also shared between those harts, and so writes to `mcountinhibit.CY` will be visible to those harts. |  `mcountinhibit` 的 CY 位（第 0 位）同时控制 `mcycle` 和 `time` 的抑制。 |
| `norm:mcountinhibit_not_impl` | If the `mcountinhibit` register is not implemented, the implementation behaves as though the register were set to zero. |  未实现的计数器对应的 `mcountinhibit` 位为只读零。 |

#### Machine Scratch (mscratch) Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Scratch (mscratch) Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mscratch_sz_acc` | The `mscratch` register is an MXLEN-bit read/write register dedicated for use by machine mode. The `mscratch` register must be implemented. |  `mscratch` 是一个 MXLEN 位读写寄存器，用于 M 模式 trap 处理程序的暂存。 |

#### Machine Exception Program Counter (mepc) Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Exception Program Counter (mepc) Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mepc_sz_acc` | `mepc` is an MXLEN-bit read/write register |  `mepc` 是一个 MXLEN 位读写寄存器。 |
| `norm:mepc_align` | The low bit of `mepc` (`mepc[0]`) is always zero. On implementations that support only IALIGN=32, the two low bits (`mepc[1:0]`) are always zero. |  `mepc` 的最低位（bit 0）始终为 0（IALIGN=32 时）。 |
| `norm:mepc_bit1_dyn_ialign_op` | If an implementation allows IALIGN to be either 16 or 32 (by changing CSR `misa`, for example), then, whenever IALIGN=32, bit `mepc[1]` is masked on reads so that it appears to be 0. This masking occurs also for the implicit read by the MRET instruction. Though masked, `mepc[1]` remains writable when IALIGN=32. |  当 IALIGN=16（C 扩展启用）时，`mepc` 的第 1:0 位可以保存 00 或 10。 |
| `norm:mepc_warl` | `mepc` is a **WARL** register |  `mepc` 是 WARL 寄存器。 |
| `norm:mepc_inv_addr_conv` | Prior to writing `mepc`, implementations may convert an invalid address into some other invalid address that `mepc` is capable of holding. |  `mepc` 不存储无效地址，写入时会将地址转换为合法值。 |
| `norm:mepc_op` | When a trap is taken into M-mode, `mepc` is written with the virtual address of the instruction that was interrupted or that encountered the exception. Otherwise, `mepc` is never written by the implementation, though it may be explicitly written by software. |  当 trap 进入 M 模式时，`mepc` 被写入触发 trap 的指令的虚拟地址。 |

#### Machine Cause (mcause) Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Cause (mcause) Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mcause_sz_acc` | The `mcause` register is an MXLEN-bit read-write register |  `mcause` 是一个 MXLEN 位读写寄存器。 |
| `norm:mcause_op` | When a trap is taken into M-mode, `mcause` is written with a code indicating the event that caused the trap. Otherwise, `mcause` is never written by the implementation, though it may be explicitly written by software. |  `mcause` 保存导致 trap 的原因。 |
| `norm:mcause_intr_op` | The Interrupt bit in the `mcause` register is set if the trap was caused by an interrupt. |  当最高位（Interrupt 位）为 1 时，表示 trap 由中断引起；为 0 时表示由异常引起。 |
| `norm:mcause_exccode_op` | The Exception Code field contains a code identifying the last exception or interrupt. |  异常代码（Exception Code）字段编码具体的异常类型。 |
| `norm:mcause_exccode_wlrl` | The Exception Code is a **WLRL** field, so is only guaranteed to hold supported exception codes. |  异常代码字段是 WLRL 字段，仅保存合法值。 |
| `norm:mcause_exccode_ld_ldrsv` | Note that load and load-reserved instructions generate load exceptions |  Load 和 Load-Reserved 访问错误使用相同的异常代码（5）。 |
| `norm:mcause_exccode_st_sc_amo` | store, store-conditional, and AMO instructions generate store/AMO exceptions. |  Store、Store-Conditional 和 AMO 访问错误使用相同的异常代码（7）。 |
| `norm:mcause_exccode_pri1` | If an instruction may raise multiple synchronous exceptions, the decreasing priority order of norm:exc_priority indicates which exception is taken and reported in `mcause`. |  M 模式中断的固定优先级为：MEI > MSI > MTI > SEI > SSI > STI > LCOFI。 |
| `norm:mcause_exccode_pri2` | Load/store/AMO address-misaligned exceptions may have either higher or lower priority than load/store/AMO page-fault and access-fault exceptions. |  中断优先级用于在多个同时挂起的中断中选择处理顺序。 |

#### Machine Trap Value (mtval) Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Trap Value (mtval) Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mtval_sz_acc` | The `mtval` register is an MXLEN-bit read-write register formatted as shown in norm:mtval_enc_img |  `mtval` 是一个 MXLEN 位读写寄存器。 |
| `norm:mtval_op` | When a trap is taken into M-mode, `mtval` is either set to zero or written with exception-specific information to assist software in handling the trap. Otherwise, `mtval` is never written by the implementation, though it may be explicitly written by software. |  当 trap 进入 M 模式时，`mtval` 被写入异常特定的信息。 |
| `norm:mtval_per_exc_behavior` | The hardware platform will specify which exceptions must set `mtval` informatively, which may unconditionally set it to zero, and which may exhibit either behavior, depending on the underlying event that caused the exception. |  `mtval` 的内容取决于异常类型：对于地址相关异常，保存故障地址；对于非法指令异常，保存指令编码或零。 |
| `norm:mtval_rdonly0` | The `mtval` register must always be implemented. If the hardware platform specifies that no exceptions set `mtval` to a nonzero value, then `mtval` is read-only zero. |  `mtval` 可以被硬连线为零。 |
| `norm:mtval_vaddr_wr1` | If `mtval` is written with a nonzero value when a breakpoint, address-misaligned, access-fault, page-fault, or hardware-error exception occurs on an instruction fetch, load, or store, then `mtval` will contain the faulting virtual address. |  对于指令地址未对齐、指令访问错误、加载/存储地址未对齐、加载/存储访问错误异常，`mtval` 被写入故障的有效地址。 |
| `norm:mtval_vaddr_not_paddr` | When page-based virtual memory is enabled, `mtval` is written with the faulting virtual address, even for physical-memory access-fault exceptions. |  `mtval` 保存的是虚拟地址，而非物理地址。 |
| `norm:mtval_vaddr_wr2` | If `mtval` is written with a nonzero value when a misaligned load or store causes an access-fault, page-fault, or hardware-error exception, then `mtval` will contain the virtual address of the portion of the access that caused the fault. |  对于页面错误异常，`mtval` 被写入故障的虚拟地址。 |
| `norm:mtval_varlen_wr` | If `mtval` is written with a nonzero value when an instruction access-fault, page-fault, or hardware-error exception occurs on a hart with variable-length instructions, then `mtval` will contain the virtual address of the portion of the instruction that caused the fault |  当 MXLEN 小于 XLEN 时，`mtval` 保存地址的低位部分。 |
| `norm:mepc_varlen_wr` | `mepc` will point to the beginning of the instruction. |  当 MXLEN 小于 XLEN 时，`mepc` 保存地址的低位部分。 |
| `norm:mtval_instr_bits_lead-in` | The `mtval` register can optionally also be used to return the faulting instruction bits on an illegal-instruction exception |  对于非法指令异常，`mtval` 可以保存故障指令的编码： |
| `norm:mtval_ill_instr_exc_in_low_bits` | The value loaded into `mtval` on an illegal-instruction exception is right-justified and all unused upper bits are cleared to zero. |  非法指令编码保存在 `mtval` 的低位中，高位零扩展。 |
| `norm:mtval_swchk_lead-in` | On a trap caused by a software-check exception, the `mtval` register holds the cause for the exception. The following encodings are defined: |  对于软件检查异常： |
| `norm:mtval_other_traps_zero` | For other traps, `mtval` is set to zero |  对于其他类型的 trap，`mtval` 被设置为零。 |
| `norm:mtval_warl` | If `mtval` is not read-only zero, it is a **WARL** register |  `mtval` 是 WARL 寄存器，可以硬连线为零。 |
| `norm:mtval_vaddr_and_0_sz` | must be able to hold all valid virtual addresses and the value zero. |  `mtval` 的宽度为 MXLEN 位，保存虚拟地址或零。 |
| `norm:mtval_inv_addr_conv` | Prior to writing `mtval`, implementations may convert an invalid address into some other invalid address that `mtval` is capable of holding. |  `mtval` 不存储无效地址。 |
| `norm:mtval_instr_bits_sz` | If the feature to return the faulting instruction bits is implemented, `mtval` must also be able to hold all values less than 2^*N*^, where _N_ is the smaller of MXLEN and ILEN. |  当保存指令编码时，`mtval` 保存指令的低位（最多 MXLEN 位）。 |

#### Machine Configuration Pointer (mconfigptr) Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Configuration Pointer (mconfigptr) Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mconfigptr_sz_acc` | The `mconfigptr` register is an MXLEN-bit read-only CSR formatted as shown in norm:mconfigptr_enc_img |  `mconfigptr` 是一个 MXLEN 位只读寄存器。 |
| `norm:mconfigptr_op` | holds the physical address of a configuration data structure. |  `mconfigptr` 保存指向配置数据结构的物理地址。 |
| `norm:mconfigptr_align` | The pointer alignment in bits must be no smaller than MXLEN: i.e., if MXLEN is 8×*n*, then `mconfigptr`[log~2n~-1:0] must be zero. |  `mconfigptr` 的值必须自然对齐。 |
| `norm:mconfigptr_mandatory` | The `mconfigptr` register must be implemented |  `mconfigptr` 必须始终被实现。 |
| `norm:mconfigptr_val` | it may be zero to indicate the configuration data structure does not exist or that an alternative mechanism must be used to locate it. |  如果未提供配置数据结构，`mconfigptr` 读取为零。 |

#### Machine Environment Configuration (menvcfg) Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Environment Configuration (menvcfg) Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:menvcfg_sz_acc` | The `menvcfg` CSR is a 64-bit read/write register, formatted as shown in norm:menvcfg_enc_img |  `menvcfg` 是一个 64 位读写寄存器（RV32 下为 `menvcfg` + `menvcfgh`）。 |
| `norm:menvcfg_op` | controls certain characteristics of the execution environment for modes less privileged than M. |  `menvcfg` 控制低特权模式下的环境配置。 |
| `norm:menvcfg_fiom_fence_op` | If bit FIOM (Fence of I/O implies Memory) is set to one in `menvcfg`, FENCE instructions executed in modes less privileged than M are modified so the requirement to order accesses to device I/O implies also the requirement to order main memory accesses. norm:menvcfg_fiom_fence_presuc_op details the modified interpretation of FENCE instruction bits PI, PO, SI, and SO for modes less privileged than M when FIOM=1. |  当 FIOM=1 时，低特权模式的 FENCE.I 指令行为受限制。 |
| `norm:menvcfg_fiom_atomic_op` | for modes less privileged than M when FIOM=1, if an atomic instruction that accesses a region ordered as device I/O has its _aq_ and/or _rl_ bit set, then that instruction is ordered as though it accesses both device I/O and memory. |  当 FIOM=1 时，低特权模式的原子指令的 FENCE 语义受限制。 |
| `norm:menvcfg_fiom_rdonly0_ok` | If S-mode is not supported, or if `satp`.MODE is read-only zero (always Bare), the implementation may make FIOM read-only zero. |  如果不需要 FIOM 功能，FIOM 可以为只读零。 |
| `norm:menvcfg_pbmte_op` | The PBMTE bit controls whether the Svpbmt extension is available for use in S-mode and G-stage address translation (i.e., for page tables pointed to by `satp` or `hgatp`). When PBMTE=1, Svpbmt is available for S-mode and G-stage address translation. When PBMTE=0, the implementation behaves as though Svpbmt were not implemented. |  PBMTE 位控制低特权模式是否可以使用页面级内存类型（Page-Based Memory Types）。 |
| `norm:menvcfg_pbmte_rdonly0` | If Svpbmt is not implemented, PBMTE is read-only zero. |  如果未实现 Svnapot 或相关扩展，PBMTE 为只读零。 |
| `norm:menvcfg_pbmte_henvcfg_pbmte_rdonly0` | for implementations with the hypervisor extension, `henvcfg`.PBMTE is read-only zero if `menvcfg`.PBMTE is zero. |  当 `menvcfg`.PBMTE=0 时，`henvcfg`.PBMTE 为只读零。 |
| `norm:menvcfg_pbmte_fence` | After changing `menvcfg`.PBMTE, executing an SFENCE.VMA instruction with _rs1_=`x0` and _rs2_=`x0` suffices to synchronize address-translation caches with respect to the altered interpretation of page-table entries' PBMT fields. |  修改 PBMTE 后需要执行 SFENCE.VMA 以确保一致性。 |
| `norm:menvcfg_adue_op` | If the Svadu extension is implemented, the ADUE bit controls whether hardware updating of PTE A/D bits is enabled for S-mode and G-stage address translations. When ADUE=1, hardware updating of PTE A/D bits is enabled during S-mode address translation, and the implementation behaves as though the Svade extension were not implemented for S-mode address translation. When the hypervisor extension is implemented, if ADUE=1, hardware updating of PTE A/D bits is enabled during G-stage address translation, and the implementation behaves as though the Svade extension were not implemented for G-stage address translation. When ADUE=0, the implementation behaves as though Svade were implemented for S-mode and G-stage address translation. |  ADUE 位控制低特权模式是否可以使用硬件 A/D 位更新。 |
| `norm:menvcfg_adue_rdonly0` | If Svadu is not implemented, ADUE is read-only zero. |  如果未实现 Svade 或 Ssade 扩展，ADUE 为只读零。 |
| `norm:menvcfg_adue_henvcfg_adue_rdonly0` | for implementations with the hypervisor extension, `henvcfg`.ADUE is read-only zero if `menvcfg`.ADUE is zero. |  当 `menvcfg`.ADUE=0 时，`henvcfg`.ADUE 为只读零。 |
| `norm:menvcfg_adue_fence` | After changing `menvcfg`.ADUE, executing an SFENCE.VMA instruction with _rs1_=`x0` and _rs2_=`x0` suffices to synchronize address-translation caches with respect to the altered interpretation of page-table entries' A/D bits. |  修改 ADUE 后需要执行 SFENCE.VMA 以确保一致性。 |
| `norm:menvcfg_cde_op` | If the Smcdeleg extension is implemented, the CDE (Counter Delegation Enable) bit controls whether Zicntr and Zihpm counters can be delegated to S-mode. When CDE=1, the Smcdeleg extension is enabled, see smcdeleg. When CDE=0, the Smcdeleg and Ssccfg extensions appear to be not implemented. |  CDE 位控制低特权模式的缓存目录操作。 |
| `norm:menvcfg_cde_rdonly0` | If Smcdeleg is not implemented, CDE is read-only zero. |  如果未实现相关扩展，CDE 为只读零。 |
| `norm:menvcfg_stce_op1` | The Sstc extension adds the `STCE` (STimecmp Enable) bit to `menvcfg` CSR. |  STCE 位控制 S 模式是否可以使用 `stimecmp` 寄存器。 |
| `norm:menvcfg_stce_rdonly0` | When the Sstc extension is not implemented, `STCE` is read-only zero. |  如果未实现 Sstc 扩展，STCE 为只读零。 |
| `norm:menvcfg_stce_op2` | The `STCE` bit enables `stimecmp` for S-mode when set to one. When this extension is implemented and `STCE` in `menvcfg` is zero, an attempt to access `stimecmp` in a mode other than M-mode raises an illegal-instruction exception, `STCE` in `henvcfg` is read-only zero, and `STIP` in `mip` and `sip` reverts to its defined behavior as if this extension is not implemented. Further, if the H extension is implemented, then `hip`.VSTIP also reverts its defined behavior as if this extension is not implemented. |  当 STCE=0 时，S 模式对 `stimecmp` 的访问会触发非法指令异常。 |
| `norm:menvcfg_cbze_op` | The Zicboz extension adds the `CBZE` (Cache Block Zero instruction enable) field to `menvcfg`. When the `CBZE` field is set to 1, it enables execution of the cache block zero instruction, `CBO.ZERO`, in modes less privileged than M. Otherwise, the instruction raises an illegal-instruction exception in modes less privileged than M. |  CBZE 位控制低特权模式是否可以使用 CBO.ZERO 指令。 |
| `norm:menvcfg_cbze_rdonly0` | When the Zicboz extension is not implemented, `CBZE` is read-only zero. |  如果未实现 Zicboz 扩展，CBZE 为只读零。 |
| `norm:menvcfg_cbcfe_op` | The Zicbom extension adds the `CBCFE` (Cache Block Clean and Flush instruction Enable) field to `menvcfg`. When the `CBCFE` field is set to 1, it enables execution of the cache block clean instruction (`CBO.CLEAN`) and the cache block flush instruction (`CBO.FLUSH`) in modes less privileged than M. Otherwise, these instructions raise an illegal-instruction exception in modes less privileged than M. |  CBCFE 位控制低特权模式是否可以使用 CBO.CLEAN 和 CBO.FLUSH 指令。 |
| `norm:menvcfg_cbcfe_rdonly0` | When the Zicbom extension is not implemented, `CBCFE` is read-only zero. |  如果未实现 Zicbom 扩展，CBCFE 为只读零。 |
| `norm:menvcfg_cbie_warl_op` | The Zicbom extension adds the `CBIE` (Cache Block Invalidate instruction Enable) WARL field to `menvcfg` to control execution of the cache block invalidate instruction (`CBO.INVAL`) in modes less privileged than M. When `CBIE` is set to `00b`, the instruction raises an illegal-instruction exception in modes less privileged than M. |  CBIE 字段是 WARL 字段，控制 CBO.INVAL 指令在低特权模式下的行为。 |
| `norm:menvcfg_cbie_rdonly0` | When the Zicbom extension is not implemented, `CBIE` is read-only zero. |  如果未实现 Zicbom 扩展，CBIE 为只读零。 |
| `norm:menvcfg_cbie_cbo-inval_op_lead-in` | When `CBIE` is set to `01b` or `11b`, and when enabled for execution in modes less privileged than M, it behaves as follows: |  CBIE 字段编码 CBO.INVAL 在低特权模式下的行为：0=保留（触发异常），1=CBO.INVAL 执行刷新操作，2=CBO.INVAL 正常执行，3=保留。 |
| `norm:menvcfg_pmm_op` | If the Smnpm extension is implemented, the `PMM` field enables or disables pointer masking (see Zpm) for the next-lower privilege mode (S-/HS-mode if S-mode is implemented, or U-mode otherwise), according to the values in norm:menvcfg_pmm_enc. |  PMM 字段控制指针掩码（Pointer Masking）功能在低特权模式下的可用性。 |
| `norm:menvcfg_pmm_rdonly0` | If Smnpm is not implemented, `PMM` is read-only zero. The `PMM` field is read-only zero for RV32. |  如果未实现 Smnpm 扩展，PMM 为只读零。 |
| `norm:menvcfg_lpe_op_lead-in` | The Zicfilp extension adds the `LPE` field in `menvcfg`. When the `LPE` field is set to 1 and S-mode is implemented, the Zicfilp extension is enabled in S-mode. If `LPE` field is set to 1 and S-mode is not implemented, the Zicfilp extension is enabled in U-mode. When the `LPE` field is 0, the Zicfilp extension is not enabled in S-mode, and the following rules apply to S-mode. If the `LPE` field is 0 and S-mode is not implemented, then the same rules apply to U-mode. |  LPE 位控制 Zicfilp 扩展在低特权模式下是否启用。 |
| `norm:menvcfg_sse_op_lead-in` | The Zicfiss extension adds the `SSE` field to `menvcfg`. When the `SSE` field is set to 1 the Zicfiss extension is activated in S-mode. When `SSE` field is 0, the following rules apply to privilege modes that are less than M: |  SSE 位控制 Zicfiss 扩展在低特权模式下是否启用。 |
| `norm:menvcfg_sse_rdonly0` | When `menvcfg.SSE` is 0, the `henvcfg.SSE` and `senvcfg.SSE` fields are read-only zero. |  如果未实现 Zicfiss 扩展，SSE 为只读零。 |
| `norm:menvcfg_dte_op` | The Ssdbltrp extension adds the double-trap-enable (`DTE`) field in `menvcfg`. When `menvcfg.DTE` is zero, the implementation behaves as though Ssdbltrp is not implemented. When Ssdbltrp is not implemented `sstatus.SDT`, `vsstatus.SDT`, and `henvcfg.DTE` bits are read-only zero. |  DTE 位控制双 trap 使能（Double Trap Enable）在 S 模式下是否可用。 |
| `norm:menvcfgh_sz_acc_op` | When XLEN=32, `menvcfgh` is a 32-bit read/write register that aliases bits 63:32 of `menvcfg`. The `menvcfgh` register does not exist when XLEN=64. |  当 XLEN=32 时，`menvcfgh` 是一个 32 位读写寄存器，保存 `menvcfg` 的高 32 位。 |
| `norm:menvcfg_menvcfgh_no_U_mode` | In harts with U-mode, `menvcfg` must be implemented, but all `WARL` fields may be read-only zero. For harts with S-mode and XLEN=32, `menvcfgh` must be similarly implemented. If U-mode is not supported, then registers `menvcfg` and `menvcfgh` do not exist. |  如果没有 U 模式，`menvcfg` 和 `menvcfgh` 不存在。 |

#### Machine Security Configuration (mseccfg) Register

*Path: Machine-Level ISA, Version 1.13 > Machine-Level CSRs > Machine Security Configuration (mseccfg) Register*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mseccfg_sz_acc` | `mseccfg` is a 64-bit read/write register, formatted as shown in norm:mseccfg_enc_img, that controls security features. |  `mseccfg` 是一个 64 位读写寄存器（RV32 下为 `mseccfg` + `mseccfgh`）。 |
| `norm:mseccfg_presence` | It exists if any extension that adds a field to `mseccfg` is implemented. Otherwise, it is reserved. |  `mseccfg` 仅在实现特定扩展时存在。 |
| `norm:mseccfg_useed_U-mode_op` | When `USEED` is 0, access to the `seed` CSR in U-mode raises an illegal-instruction exception. When `USEED` is 1, read-write access to the `seed` CSR from U-mode is allowed; all other types of accesses raise an illegal-instruction exception. |  USEED 位控制 U 模式是否可以使用种子寄存器（用于哈希功能）。 |
| `norm:mseccfg_useed_rdonly0` | If Zkr or U-mode is not implemented, `USEED` is read-only zero. |  如果未实现 Zkr 扩展，USEED 为只读零。 |
| `norm:mseccfg_sseed_SorHS-mode_op` | When `SSEED` is 0, access to the `seed` CSR from S-/HS-mode raises an illegal-instruction exception. When `SSEED` is 1, read-write access to the `seed` CSR from S-/HS-mode is allowed; all other types of accesses raise an illegal-instruction exception. |  SSEED 位控制 S/HS 模式是否可以使用种子寄存器。 |
| `norm:mseccfg_sseed_rdonly0` | If Zkr or S-mode is not implemented, `SSEED` is read-only zero. |  如果未实现 Zkr 扩展，SSEED 为只读零。 |
| `norm:mseccfg_sseed_VSorVU-mode_op` | When the H extension is also implemented, access to the `seed` CSR from an HS-qualified instruction leads to a virtual-instruction exception in VS and VU modes; all other types of accesses raise an illegal-instruction exception. |  SSEED 位还控制 VS/VU 模式是否可以使用种子寄存器（通过 `henvcfg`）。 |
| `norm:mseccfg_mml_warl` | The `mseccfg.MML` (Machine Mode Lockdown) is a WARL field. |  MML（Machine Mode Lockdown）位是 WARL 字段。 |
| `norm:mseccfg_mml_sticky` | This is a sticky bit, meaning that once set it cannot be unset until a **PMP reset**. |  MML 位一旦置位，就不能被清零（sticky bit），直到 hart 复位。 |
| `norm:mseccfg_mml_set` | When `mseccfg.MML` is set the system's behavior changes in the following way: |  当 MML=1 时，PMP 的权限检查规则发生变化，支持更细粒度的 M 模式锁定。 |
| `norm:mseccfg_mml_pmpcfg_L_op` | The meaning of `pmpcfg.L` changes: Instead of marking a rule as **locked** and **enforced** in all modes, it now marks a rule as **M-mode-only** when set and **S/U-mode-only** when unset. The formerly reserved encoding of `pmpcfg.RW=01`, and the encoding `pmpcfg.LRWX=1111`, now encode a **Shared-Region**. |  在 MML=1 时，PMP 条目的 L、R、W、X 位的组合具有不同的语义。 |
| `norm:mseccfg_mml_M_rule_op` | An _M-mode-only_ rule is **enforced** on Machine mode and **denied** in Supervisor or User mode. It also remains **locked** so that any further modifications to its associated configuration or address registers are ignored until a **PMP reset**, unless `mseccfg.RLB` is set. |  M-mode-only 规则在 M 模式下**强制执行**，在 S/U 模式下**拒绝**。 |
| `norm:mseccfg_mml_SorU_rule_op` | An _S/U-mode-only_ rule is **enforced** on Supervisor and User modes and **denied** on Machine mode. |  S/U-mode-only 规则在 S 和 U 模式下**强制执行**，在 M 模式下**拒绝**。 |
| `norm:mseccfg_mml_shared_rule_op` | A _Shared-Region_ rule is **enforced** on all modes, with restrictions depending on the `pmpcfg.L` and `pmpcfg.X` bits: |  共享区域规则在所有模式下**强制执行**，但有限制。 |
| `norm:mseccfg_mml_shared_L0_op` | A _Shared-Region_ rule where `pmpcfg.L` is not set can be used for sharing data between M-mode and S/U-mode, so is not executable. M-mode has read/write access to that region, and S/U-mode has read access if `pmpcfg.X` is not set, or read/write access if `pmpcfg.X` is set. |  L=0 的共享区域规则可用于 M 模式和 S/U 模式之间的数据共享。 |
| `norm:mseccfg_mml_shared_L1_op` | A _Shared-Region_ rule where `pmpcfg.L` is set can be used for sharing code between M-mode and S/U-mode, so is not writable. Both M-mode and S/U-mode have execute access on the region, and M-mode also has read access if `pmpcfg.X` is set. The rule remains **locked** so that any further modifications to its associated configuration or address registers are ignored until a **PMP reset**, unless `mseccfg.RLB` is set. |  L=1 的共享区域规则可用于 M 模式和 S/U 模式之间的代码共享。 |
| `norm:mseccfg_mml_shared_LRWX_1111_op` | The encoding `pmpcfg.LRWX=1111` can be used for sharing data between M-mode and S/U mode, where both modes only have read-only access to the region. The rule remains **locked** so that any further modifications to its associated configuration or address registers are ignored until a **PMP reset**, unless `mseccfg.RLB` is set. |  编码 pmpcfg.LRWX=1111 可用于 M 模式和 S/U 模式之间的数据共享。 |
| `norm:mseccfg_mml_X_restrict` | Adding a rule with executable privileges that either is **M-mode-only** or a **locked** **Shared-Region** is not possible and such `pmpcfg` writes are ignored, leaving `pmpcfg` unchanged. This restriction can be temporarily lifted by setting `mseccfg.RLB` |  在 MML=1 时，添加具有执行权限的 M-mode-only 或共享区域规则受到限制。 |
| `norm:mseccfg_mml_exec_code` | Executing code with Machine mode privileges is only possible from memory regions with a matching **M-mode-only** rule or a **locked** **Shared-Region** rule with executable privileges. Executing code from a region without a matching rule or with a matching _S/U-mode-only_ rule is **denied**. |  具有 M 模式特权的代码执行只能来自 M 模式可访问的内存区域。 |
| `norm:mseccfg_pmm_presence_op` | If the Smmpm extension is implemented, the `PMM` field enables or disables pointer masking (see Zpm) for M-mode according to the values in norm:mseccfg_pmm_enc. |  如果实现了 Smmpm 扩展，PMM 字段控制 M 模式下的指针掩码功能。 |
| `norm:mseccfg_pmm_rdonly0` | If Smmpm is not implemented, `PMM` is read-only zero. The `PMM` field is read-only zero for RV32. |  如果未实现 Smmpm 扩展，PMM 为只读零。 |
| `norm:mseccfg_mlpe_presence` | The Zicfilp extension adds the `MLPE` field in `mseccfg`. |  Zicfilp 扩展在 `mseccfg` 中添加 MLPE 字段。 |
| `norm:mseccfg_mlpe_set_op` | When `MLPE` field is 1, Zicfilp extension is enabled in M-mode. |  当 MLPE=1 时，Zicfilp 扩展在 M 模式下启用。 |
| `norm:mseccfg_mlpe_clr_op_lead-in` | When the `MLPE` field is 0, the Zicfilp extension is not enabled in M-mode and the following rules apply to M-mode. |  当 MLPE=0 时，Zicfilp 扩展在 M 模式下不启用。 |
| `norm:mseccfgh_sz_acc_op` | When XLEN=32 only, `mseccfgh` is a 32-bit read/write register that aliases bits 63:32 of `mseccfg`. |  当 XLEN=32 时，`mseccfgh` 是一个 32 位读写寄存器，保存 `mseccfg` 的高 32 位。 |
| `norm:mseccfgh_presence` | Register `mseccfgh` exists when XLEN=32 and `mseccfg` is implemented; it does not exist when XLEN=64. |  `mseccfgh` 在 XLEN=32 且 `mseccfg` 被实现时存在；XLEN=64 时不存在。 |

#### Machine Timer (mtime and mtimecmp) Registers

*Path: Machine-Level ISA, Version 1.13 > Machine-Level Memory-Mapped Registers > Machine Timer (mtime and mtimecmp) Registers*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mtime_acc` | Platforms provide a real-time counter, exposed as a memory-mapped machine-mode read-write register, `mtime`. |  平台提供一个实时计数器，以内存映射的 `mtime` 寄存器形式暴露。 |
| `norm:mtime_op` | `mtime` must increment at constant frequency |  `mtime` 必须以恒定频率递增。 |
| `norm:mtime_tick_period` | the platform must provide a mechanism for determining the period of an `mtime` tick. |  平台必须提供机制来确定 `mtime` 的递增频率。 |
| `norm:mtime_wrap` | The `mtime` register will wrap around if the count overflows. |  `mtime` 寄存器在计数溢出时会回绕。 |
| `norm:mtime_sz` | The `mtime` register has a 64-bit precision on all RV32 and RV64 systems. |  `mtime` 寄存器在所有 RV32 和 RV64 系统上具有 64 位精度。 |
| `norm:mtimecmp_sz` | Platforms provide a 64-bit memory-mapped machine-mode timer compare register (`mtimecmp`). |  平台提供 64 位内存映射的机器模式定时器比较寄存器 `mtimecmp`。 |
| `norm:mtime_intr_pending` | A machine timer interrupt becomes pending whenever `mtime` contains a value greater than or equal to `mtimecmp`, treating the values as unsigned integers. The interrupt remains posted until `mtimecmp` becomes greater than `mtime` |  当 `mtime` >= `mtimecmp` 时，机器定时器中断变为挂起。 |
| `norm:mtime_intr_taken` | The interrupt will only be taken if interrupts are enabled and the MTIE bit is set in the `mie` register. |  仅当中断被使能时，中断才会被接收。 |
| `norm:mtime_intr_mtip_visibility` | If the result of the comparison between `mtime` and `mtimecmp` changes, it is guaranteed to be reflected in MTIP eventually, but not necessarily immediately. |  如果 `mtime` 和 `mtimecmp` 之间的比较结果发生变化，MTIP 位最终会反映该变化。 |
| `norm:mtimecmp_rv32_wr` | In RV32, memory-mapped writes to `mtimecmp` modify only one 32-bit part of the register. |  在 RV32 中，对 `mtimecmp` 的内存映射写入仅修改 32 位的一部分。 |
| `norm:mtime_mtimecmp_rv64_wr` | For RV64, naturally aligned 64-bit memory accesses to the `mtime` and `mtimecmp` registers are additionally supported and are atomic. |  对于 RV64，对 `mtime` 和 `mtimecmp` 的自然对齐 64 位内存访问是原子的。 |
| `norm:time_timeh_visibility_mtime` | When `mtime` changes, it is guaranteed to be reflected in `time` and `timeh` eventually, but not necessarily immediately. |  当 `mtime` 变化时，保证最终（但不一定立即）反映在 `time` 和 `timeh` CSR 中。 |

#### Environment Call and Breakpoint

*Path: Machine-Level ISA, Version 1.13 > Machine-Mode Privileged Instructions > Environment Call and Breakpoint*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:ecall_ebreak_epc_value` | ECALL and EBREAK cause the receiving privilege mode’s `epc` register to be set to the address of the ECALL or EBREAK instruction itself, _not_ the address of the following instruction. |  ECALL 和 EBREAK 使接收特权模式的 `epc` 寄存器被设置为 ECALL/EBREAK 指令的地址。 |
| `norm:ecall_ebreak_no_minstret_inc` | As ECALL and EBREAK cause synchronous exceptions, they are not considered to retire, and should not increment the `minstret` CSR. |  由于 ECALL 和 EBREAK 导致 trap，`minstret` 不递增。 |

#### Trap-Return Instructions

*Path: Machine-Level ISA, Version 1.13 > Machine-Mode Privileged Instructions > Trap-Return Instructions*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mret_presence` | MRET is always provided. |  MRET 始终被提供。 |
| `norm:sret_presence` | SRET must be provided if supervisor mode is supported, and should raise an illegal-instruction exception otherwise. |  如果支持监管者模式，则必须提供 SRET。 |
| `norm:sret_ill_inst_exc_rst` | SRET should also raise an illegal-instruction exception when TSR=1 in `mstatus`, as described in virt-control. |  当 TSR=1 时，SRET 在 S 模式下也应触发非法指令异常。 |
| `norm:xret_in_higher_mode` | An *x*RET instruction can be executed in privilege mode _x_ or higher, where executing a lower-privilege *x*RET instruction will pop the relevant lower-privilege interrupt enable and privilege mode stack. |  xRET 指令可以在特权模式 x 或更高模式下执行。 |
| `norm:xret_in_lower_mode` | Attempting to execute an *x*RET instruction in a mode less privileged than _x_ will raise an illegal-instruction exception. |  尝试在低于 x 的特权模式下执行 xRET 会触发非法指令异常。 |
| `norm:xret_op` | In addition to manipulating the privilege stack as described in privstack, *x*RET sets the `pc` to the value stored in the `*x*epc` register. |  除了操作特权栈外，xRET 还将 pc 设置为 `xepc` 中保存的地址。 |
| `norm:xret_clr_lr_resv` | If the Zalrsc extension is supported, the *x*RET instruction is allowed to clear any outstanding LR address reservation but is not required to. |  如果支持 Zalrsc 扩展，xRET 指令允许清除 load-reserved 保留。 |

#### Wait for Interrupt

*Path: Machine-Level ISA, Version 1.13 > Machine-Mode Privileged Instructions > Wait for Interrupt*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:wfi_op` | The Wait for Interrupt instruction (WFI) informs the implementation that the current hart can be stalled until an interrupt might need servicing. Execution of the WFI instruction can also be used to inform the hardware platform that suitable interrupts should preferentially be routed to this hart. |  WFI（Wait for Interrupt）指令通知实现当前 hart 可以进入低功耗状态，直到中断或其他事件唤醒。 |
| `norm:wfi_all_privileged_modes` | WFI is available in all privileged modes |  WFI 在所有特权模式下可用。 |
| `norm:wfi_opt_U_mode` | optionally available to U-mode. |  WFI 可选地在 U 模式下可用。 |
| `norm:wfi_ill_exc` | This instruction may raise an illegal-instruction exception when TW=1 in `mstatus`, as described in virt-control. |  当 `mstatus`.TW=1 时，WFI 在 S 模式下可能触发非法指令异常。 |
| `norm:wfi_mepc_val` | If an enabled interrupt is present or later becomes present while the hart is stalled, the interrupt trap will be taken on the following instruction, i.e., execution resumes in the trap handler and `mepc` = `pc` + 4. |  如果在 WFI 执行期间有使能的中断挂起或稍后变为挂起，WFI 将在有界时间内完成。 |
| `norm:wfi_resume_reason` | Implementations are permitted to resume execution for any reason, even if an enabled interrupt has not become pending. Hence, a legal implementation is to simply implement the WFI instruction as a NOP. |  实现允许因任何原因恢复执行，即使没有中断发生。 |
| `norm:wfi_intr_dis` | The WFI instruction can also be executed when interrupts are disabled. |  WFI 指令也可以在中断被禁用时执行。 |
| `norm:wfi_unaffected_conditions` | The operation of WFI must be unaffected by the global interrupt bits in `mstatus` (MIE and SIE) and the delegation register `mideleg` (i.e., the hart must resume if a locally enabled interrupt becomes pending, even if it has been delegated to a less-privileged mode), but should honor the individual interrupt enables (e.g, MTIE) (i.e., implementations should avoid resuming the hart if the interrupt is pending but not locally enabled). WFI is also required to resume execution for locally enabled interrupts pending at any privilege level, regardless of the global interrupt enable at each privilege level. |  WFI 的操作不受 `mstatus` 中全局中断位或 `mie` 中中断使能位的影响。 |
| `norm:wfi_no_intr_pc` | If the event that causes the hart to resume execution does not cause an interrupt to be taken, execution will resume at `pc` + 4 |  如果导致 hart 恢复执行的事件不是中断，则 pc 被设置为 WFI 之后的下一条指令。 |

### Reset

*Path: Machine-Level ISA, Version 1.13 > Reset*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:M-mode_at_rst2` | Upon reset, a hart’s privilege mode is set to M. |  复位时，hart 的特权模式被设置为 M。 |
| `norm:mstatus_mie_mprv_rst` | The `mstatus` fields MIE and MPRV are reset to 0. |  `mstatus` 的 MIE 和 MPRV 字段被复位为 0。 |
| `norm:mstatus_mstatush_mbe_rst` | If little-endian memory accesses are supported, the `mstatus`/`mstatush` field MBE is reset to 0. |  如果支持小端内存访问，`mstatus`/`mstatush` 的 MBE 字段被复位为 0。 |
| `norm:misa_extensions_rst2` | The `misa` register is reset to enable the maximal set of supported extensions, as described in misa. |  `misa` 寄存器被复位为使能最大支持扩展集。 |
| `norm:ld_rsv_rst` | For implementations with the Zalrsc standard extension, there is no valid load reservation. |  对于实现 Zalrsc 标准扩展的实现，复位后没有有效的 load 保留。 |
| `norm:pc_rst` | The `pc` is set to an implementation-defined reset vector. |  `pc` 被设置为实现定义的复位向量。 |
| `norm:mcause_rst1_val` | The `mcause` register is set to a value indicating the cause of the reset. |  `mcause` 寄存器被设置为指示复位原因的值。 |
| `norm:pmp_A_L_rst` | Writable PMP registers’ A and L fields are set to 0, unless the platform mandates a different reset value for some PMP registers’ A and L fields. |  可写的 PMP 寄存器的 A 和 L 字段被置为 0，除非平台要求不同的复位值。 |
| `norm:hgatp_vsatp_mode_rst` | If the hypervisor extension is implemented, the `hgatp`.MODE and `vsatp`.MODE fields are reset to 0. |  如果实现了 hypervisor 扩展，`hgatp` 和 `vsatp` 的 MODE 字段被复位。 |
| `norm:mnstatus_nmie_rst` | If the Smrnmi extension is implemented, the `mnstatus`.NMIE field is reset to 0. |  如果实现了 Smrnmi 扩展，`mnstatus`.NMIE 字段被复位为 0。 |
| `norm:warl_rst` | No **WARL** field contains an illegal value. |  复位后没有 **WARL** 字段包含非法值。 |
| `norm:mseccfg_mlpe_rst` | If the Zicfilp extension is implemented, the `mseccfg`.MLPE field is reset to 0. |  如果实现了 Zicfilp 扩展，`mseccfg`.MLPE 字段被复位为 0。 |
| `norm:mseccfg_mml_mmwp_rlb_rst` | The `MML`, `MMWP`, and `RLB` fields of the `mseccfg` register are set to 0, unless the platform mandates a different reset value. |  `mseccfg` 寄存器的 MML、MMWP 和 RLB 字段在复位时被清零。 |
| `norm:mcause_rst2_val` | The `mcause` values after reset have implementation-specific interpretation |  复位后的 `mcause` 值具有实现特定的编码。 |
| `norm:mcause_rst_zero` | the value 0 should be returned on implementations that do not distinguish different reset conditions. Implementations that distinguish different reset conditions should only use 0 to indicate the most complete reset. |  在不需要区分复位原因的实现上，应返回值 0。 |
| `norm:mseccfg_useed_sseed_rst` | The `USEED` and `SSEED` fields of the `mseccfg` CSR must have defined reset values. The system must not allow them to be in an undefined state after reset. |  `mseccfg` CSR 的 USEED 和 SSEED 字段在复位时被清零。 |
| `norm:mcause_rst_alias_ok` | `mcause` reset values may alias `mcause` values following synchronous exceptions |  `mcause` 复位值可以与 `mcause` 的标准异常/中断编码别名。 |

### Non-Maskable Interrupts

*Path: Machine-Level ISA, Version 1.13 > Non-Maskable Interrupts*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:nmi_op` | cause an immediate jump to an implementation-defined NMI vector running in M-mode regardless of the state of a hart’s interrupt enable bits. The `mepc` register is written with the virtual address of the instruction that was interrupted, |  NMI 导致立即跳转到实现定义的 NMI 处理程序地址。 |
| `norm:nmi_mcause_val1` | `mcause` is set to a value indicating the source of the NMI. |  `mcause` 被设置为指示 NMI 原因的值。 |
| `norm:nmi_mcause_val2` | The values written to `mcause` on an NMI are implementation-defined. |  NMI 时写入 `mcause` 的值是实现定义的。 |
| `norm:nmi_mcause_restrictions` | The high Interrupt bit of `mcause` should be set to indicate that this was an interrupt. An Exception Code of 0 is reserved to mean "`unknown cause`" and implementations that do not distinguish sources of NMIs via the `mcause` register should return 0 in the Exception Code. |  `mcause` 的高位（Interrupt 位）应被置位以指示这是一个中断。 |
| `norm:nmi_no_rst` | NMIs do not reset processor state |  NMI 不复位处理器状态。 |

### Physical Memory Attributes

*Path: Machine-Level ISA, Version 1.13 > Physical Memory Attributes*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pma_indep_exec_context` | PMAs do not vary by execution context. |  PMA 不随执行上下文（如特权模式）变化。 |
| `norm:pma_rtcfg_mmr` | Where the attributes are run-time configurable, platform-specific memory-mapped control registers can be provided |  当属性可运行时配置时，通过内存映射寄存器进行配置。 |
| `norm:pma_chk_paddr` | PMAs are checked for any access to physical memory, including accesses that have undergone virtual to physical memory translation. |  PMA 对任何物理内存访问进行检查，包括隐式访问（如页表遍历）。 |
| `norm:pma_precise_recom` | we strongly recommend that, where possible, RISC-V processors precisely trap physical memory accesses that fail PMA checks. Precisely trapped PMA violations manifest as instruction, load, or store access-fault exceptions, distinct from virtual-memory page-fault exceptions. |  强烈建议在可能的情况下，RISC-V 实现应使 PMA 违规产生精确异常。 |
| `norm:pma_imprecise_ok` | Precise PMA traps might not always be possible, for example, when probing a legacy bus architecture that uses access failures as part of the discovery mechanism. In this case, error responses from peripheral devices will be reported as imprecise bus-error interrupts. |  精确的 PMA trap 可能并不总是可行，例如对于总线错误。 |

#### Supported Access Type PMAs

*Path: Machine-Level ISA, Version 1.13 > Physical Memory Attributes > Supported Access Type PMAs*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pma_mm_rw` | Main memory regions always support read and write of all access widths required by the attached devices |  主内存区域始终支持所有访问宽度的读写操作。 |
| `norm:pma_mm_ifetch` | can specify whether instruction fetch is supported. |  主内存区域可以指定是否支持指令获取。 |
| `norm:pma_mm_mandatory_sz` | the design of a processor or device accessing main memory might support other widths, but must be able to function with the types supported by the main memory. |  访问主内存的处理器或设备的设计必须支持所需的访问宽度。 |
| `norm:pma_io_rwx_per_sz` | I/O regions can specify which combinations of read, write, or execute accesses to which data widths are supported. |  I/O 区域可以指定读、写或执行的哪些组合以及哪些访问宽度是合法的。 |
| `norm:pma_vm_hw_tbl_acc` | For systems with page-based virtual memory, I/O and memory regions can specify which combinations of hardware page-table reads and hardware page-table writes are supported. |  对于具有基于页面的虚拟内存的系统，I/O 和内存区域可以指定是否支持硬件页表遍历访问。 |

#### Atomicity PMAs

*Path: Machine-Level ISA, Version 1.13 > Physical Memory Attributes > Atomicity PMAs*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pma_cache_mm_all_atomics` | Some platforms might mandate that all of cacheable main memory support all atomic operations |  某些平台可能要求所有可缓存主内存支持完整的原子操作。 |

#### AMO PMA

*Path: Machine-Level ISA, Version 1.13 > Physical Memory Attributes > Atomicity PMAs > AMO PMA*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pma_amo_levels` | Within AMOs, there are four levels of support: _AMONone_, _AMOSwap_, _AMOLogical_, and _AMOArithmetic_. AMONone indicates that no AMO operations are supported. AMOSwap indicates that only `amoswap` instructions are supported in this address range. AMOLogical indicates that swap instructions plus all the logical AMOs (`amoand`, `amoor`, `amoxor`) are supported. AMOArithmetic indicates that all RISC-V AMOs defined by the A extension are supported. |  在 AMO 中，有四个支持级别：AMONone、AMOSwap、AMOLogical 和 AMOArithmetic。 |
| `norm:pma_amo_sz_support` | For each level of support, naturally aligned AMOs of a given width are supported if the underlying memory region supports reads and writes of that width. |  对于每个支持级别，给定宽度的自然对齐 AMO 被支持。 |
| `norm:pma_amo_far_subset_proc` | Main memory and I/O regions may only support a subset or none of the processor-supported atomic operations. |  主内存和 I/O 区域可能仅支持处理器支持的原子操作的子集或不支持。 |
| `norm:pma_amo_zacas_levels1` | The Zacas extension defines three additional levels of support: AMOCASW, AMOCASD, and AMOCASQ. |  Zacas 扩展定义了三个额外的支持级别：AMOCASW、AMOCASD 和 AMOCASQ。 |
| `norm:pma_amo_zacas_levels2` | AMOCASW indicates that in addition to instructions indicated by AMOArithmetic-level support, the `AMOCAS.W` instruction is supported. AMOCASD indicates that in addition to instructions indicated by AMOCASW-level support, the `AMOCAS.D` instruction is supported. AMOCASQ indicates that in addition to instructions indicated by AMOCASD-level support, the `AMOCAS.Q` instruction is supported. |  AMOCASW 表示除 AMOArithmetic 级别外还支持 `AMOCAS.W`；AMOCASD 表示还支持 `AMOCAS.D`。 |
| `norm:pma_amo_zacas_req_arith` | `AMOCASW/D/Q` require AMOArithmetic-level support as the `AMOCAS.W/D/Q` instructions require ability to perform an arithmetic comparison and a swap operation. |  `AMOCASW/D/Q` 需要 AMOArithmetic 级别支持，因为 `AMOCAS.W/D/Q` 使用 AMOArithmetic 级别的指令。 |
| `norm:pma_amo_zabha_req` | The AMOs specified by the Zabha extension require the same level of support as the corresponding instructions in the Zaamo standard extension or the Zacas extension. |  Zabha 扩展指定的 AMO 需要与标准 AMO 相同级别的支持。 |
| `norm:ziccamoa_pma_amo_req` | The ziccamoa extension requires AMOArithmetic-level support to main memory regions with both the coherence and cacheability PMAs. |  Ziccamoa 扩展要求 AMOArithmetic 级别支持。 |
| `norm:ziccamoc_pma_amo_req` | The ziccamoc extension requires AMOCASQ-level support to main memory regions with both the coherence and cacheability PMAs. |  Ziccamoc 扩展要求 AMOCASQ 级别支持。 |

#### Reservability PMA

*Path: Machine-Level ISA, Version 1.13 > Physical Memory Attributes > Atomicity PMAs > Reservability PMA*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pma_rsrv_levels` | For _LR/SC_, there are three levels of support indicating combinations of the reservability and eventuality properties: _RsrvNone_, _RsrvNonEventual_, and _RsrvEventual_. RsrvNone indicates that no LR/SC operations are supported (the location is non-reservable). RsrvNonEventual indicates that the operations are supported (the location is reservable), but without the eventual success guarantee described in the unprivileged ISA specification. RsrvEventual indicates that the operations are supported and provide the eventual success guarantee. |  对于 LR/SC，有三个支持级别，指示支持的组合。 |
| `norm:ziccrse_pma_rsrv_req` | The ziccrse extension requires RsrvEventual support to main memory regions with both the coherence and cacheability PMAs. |  Ziccrse 扩展要求 RsrvEventual 级别支持。 |

#### Misaligned Atomicity Granule PMA

*Path: Machine-Level ISA, Version 1.13 > Physical Memory Attributes > Misaligned Atomicity Granule PMA*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pma_mag_insts` | The misaligned atomicity granule PMA applies only to AMOs, loads and stores defined in the base ISAs, and loads and stores of no more than XLEN bits defined in the F, D, and Q extensions, and compressed encodings thereof. |  未对齐原子粒度 PMA 仅适用于 AMO、load 和 store 指令。 |
| `norm:pma_mag_op_within` | if all accessed bytes lie within the same misaligned atomicity granule, the instruction will not raise an exception for reasons of address alignment, and the instruction will give rise to only one memory operation for the purposes of RVWMO--i.e., it will execute atomically. |  如果所有访问的字节都在同一个自然对齐的 2^G 字节区域内，则操作是原子的。 |
| `norm:pma_mag_op_amo` | If a misaligned AMO accesses a region that does not specify a misaligned atomicity granule PMA, or if not all accessed bytes lie within the same misaligned atomicity granule, then an exception is raised. |  如果未对齐的 AMO 访问未指定未对齐原子粒度的区域，则触发访问错误异常。 |
| `norm:pma_mag_op_ldst` | For regular loads and stores that access such a region or for which not all accessed bytes lie within the same atomicity granule, then either an exception is raised, or the access proceeds but is not guaranteed to be atomic. |  对于访问此类区域的常规 load/store，或不是所有字节都在同一区域内的操作，行为是实现定义的。 |
| `norm:pma_mag_exc` | Implementations may raise access-fault exceptions instead of address-misaligned exceptions for some misaligned accesses, indicating the instruction should not be emulated by a trap handler. |  实现可以选择触发访问错误异常而非地址未对齐异常。 |
| `norm:pma_mag_op_rsrv` | LR/SC instructions are unaffected by this PMA and so always raise an exception when misaligned. |  LR/SC 指令不受此 PMA 影响，因此始终对未对齐地址触发地址未对齐异常。 |
| `norm:pma_mag_op_vec` | Vector memory accesses are also unaffected, so might execute non-atomically even when contained within a misaligned atomicity granule. |  向量内存访问也不受影响，因此即使区域内也可能非原子执行。 |
| `norm:pma_mag_op_implicit` | Implicit accesses are similarly unaffected by this PMA. |  隐式访问同样不受此 PMA 影响。 |
| `norm:zama16b_pma_mag_req` | The Zama16b extension requires MAG16 support to main memory regions. |  Zama16b 扩展要求对主内存支持 MAG16（16 字节未对齐原子粒度）。 |

#### Memory-Ordering PMAs

*Path: Machine-Level ISA, Version 1.13 > Physical Memory Attributes > Memory-Ordering PMAs*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pma_mo_io_chan_def` | Each strongly ordered I/O region specifies a numbered ordering channel, which is a mechanism by which ordering guarantees can be provided between different I/O regions. |  每个强序 I/O 区域指定一个编号的排序通道。 |
| `norm:pma_mo_io_chan_0` | Channel 0 is used to indicate point-to-point strong ordering only, where only accesses by the hart to the single associated I/O region are strongly ordered. |  通道 0 用于表示该区域与其他所有内存全局排序。 |

#### Coherence and Cacheability PMAs

*Path: Machine-Level ISA, Version 1.13 > Physical Memory Attributes > Coherence and Cacheability PMAs*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pma_non_cacheable` | If a PMA indicates non-cacheability, then accesses to that region must be satisfied by the memory itself, not by any caches. |  如果 PMA 指示不可缓存性，则对该区域的访问必须由内存本身满足，而非任何缓存。 |
| `norm:uncached_ignores_cached` | For implementations with a cacheability-control mechanism, the situation may arise that a program uncacheably accesses a memory location that is currently cache-resident. In this situation, the cached copy must be ignored. This constraint is necessary to prevent more-privileged modes’ speculative cache refills from affecting the behavior of less-privileged modes’ uncacheable accesses. |  对于具有缓存控制机制的实现，当程序不可缓存地访问当前驻留在缓存中的内存位置时，必须忽略缓存副本。 |

#### Idempotency PMAs

*Path: Machine-Level ISA, Version 1.13 > Physical Memory Attributes > Idempotency PMAs*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pma_idp_mm` | Main memory regions are assumed to be idempotent. |  主内存区域被假定为幂等的。 |
| `norm:pma_idp_no_spec` | hardware should always be designed to avoid speculative or redundant accesses to memory regions marked as non-idempotent |  硬件应始终被设计为避免对标记为非幂等区域的推测性或冗余访问。 |
| `norm:pma_idp_misaligned_exc` | Non-idempotent regions might not support misaligned accesses. Misaligned accesses to such regions should raise access-fault exceptions rather than address-misaligned exceptions |  非幂等区域可能不支持未对齐访问。对此类区域的未对齐访问应触发访问错误异常而非地址未对齐异常。 |
| `norm:pma_idp_implicit_ok` | For non-idempotent regions, implicit reads and writes must not be performed early or speculatively, with the following exceptions. When a non-speculative implicit read is performed, an implementation is permitted to additionally read any of the bytes within a naturally aligned power-of-2 region containing the address of the non-speculative implicit read. Furthermore, when a non-speculative instruction fetch is performed, an implementation is permitted to additionally read any of the bytes within the _next_ naturally aligned power-of-2 region of the same size (with the address of the region taken modulo 2^XLEN^). |  对于非幂等区域，隐式读写不得提前或推测性地执行，但有例外情况。 |
| `norm:pma_idp_implicit_sz` | The size of these naturally aligned power-of-2 regions is implementation-defined, but, for systems with page-based virtual memory, must not exceed the smallest supported page size. |  这些自然对齐的 2 的幂次区域的大小是实现定义的，但对于具有基于页面的虚拟内存的系统，不得超过最小支持的页面大小。 |

### Physical Memory Protection

*Path: Machine-Level ISA, Version 1.13 > Physical Memory Protection*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pmp_granularity` | The granularity of PMP access control settings are platform-specific, but the standard PMP encoding supports regions as small as four bytes. |  PMP 访问控制设置的粒度由平台决定，但标准 PMP 编码支持最小 4 字节的区域。 |
| `norm:pmp_mmode_only_regions` | Certain regions’ privileges can be hardwired—for example, some regions might only ever be visible in machine mode but in no lower-privilege layers. |  某些区域的权限可以被硬连线——例如，某些区域可能仅在 M 模式下可见。 |
| `norm:pmp_check_priv_modes` | PMP checks are applied to all accesses whose effective privilege mode is S or U, including instruction fetches and data accesses in S and U mode, and data accesses in M-mode when the MPRV bit in `mstatus` is set and the MPP field in `mstatus` contains S or U. |  PMP 检查适用于所有有效特权模式为 S 或 U 的访问，包括 S/U 模式下的取指和数据访问，以及 M 模式下 MPRV=1 且 MPP 为 S 或 U 时的数据访问。 |
| `norm:pmp_check_pagetable_access` | PMP checks are also applied to page-table accesses for virtual-address translation, for which the effective privilege mode is S. |  PMP 检查也适用于虚拟地址翻译的页表访问，此时有效特权模式为 S。 |
| `norm:pmp_mmode_locking` | Optionally, PMP checks may additionally apply to M-mode accesses, in which case the PMP registers themselves are locked, so that even M-mode software cannot change them until the hart is reset. |  可选地，PMP 检查也可以应用于 M 模式访问，此时 PMP 寄存器被锁定，M 模式软件也无法修改，直到 hart 复位。 |
| `norm:pmp_violation_precise_trap` | PMP violations are always trapped precisely at the processor. |  PMP 违规总是在处理器上被精确捕获（精确异常）。 |

#### Physical Memory Protection CSRs

*Path: Machine-Level ISA, Version 1.13 > Physical Memory Protection > Physical Memory Protection CSRs*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pmp_entry_structure` | PMP entries are described by an 8-bit configuration register and one MXLEN-bit address register. Some PMP settings additionally use the address register associated with the preceding PMP entry. |  PMP 条目由一个 8 位配置寄存器和一个 MXLEN 位地址寄存器描述。某些 PMP 设置还会使用前一个 PMP 条目的地址寄存器。 |
| `norm:pmp_entry_count` | Up to 64 PMP entries are supported. Implementations may implement zero, 16, or 64 PMP entries; the lowest-numbered PMP entries must be implemented first. |  最多支持 64 个 PMP 条目。实现可以实现 0、16 或 64 个条目；必须优先实现编号最小的条目。 |
| `norm:pmp_csrs_warl_access` | All PMP CSR fields are **WARL** and may be read-only zero. |  所有 PMP CSR 字段都是 WARL（写任意值，读合法值），可以是只读零。 |
| `norm:pmp_csr_mode` | PMP CSRs are only accessible to M-mode. |  PMP CSR 仅 M 模式可访问。 |
| `norm:pmp_cfg_rv32_layout` | For RV32, sixteen CSRs, `pmpcfg0`–`pmpcfg15`, hold the configurations `pmp0cfg`–`pmp63cfg` for the 64 PMP entries, as shown in pmpcfg-rv32. |  RV32 下，16 个 CSR（`pmpcfg0`-`pmpcfg15`）保存 64 个 PMP 条目的配置。 |
| `norm:pmp_cfg_rv64_layout` | For RV64, eight even-numbered CSRs, `pmpcfg0`, `pmpcfg2`, …, `pmpcfg14`, hold the configurations for the 64 PMP entries, as shown in pmpcfg-rv64. |  RV64 下，8 个偶数编号的 CSR（`pmpcfg0`、`pmpcfg2`、…、`pmpcfg14`）保存 64 个 PMP 条目的配置。 |
| `norm:pmp_cfg_rv64_illegal` | For RV64, the odd-numbered configuration registers, `pmpcfg1`, `pmpcfg3`, …, `pmpcfg15`, are illegal. |  RV64 下，奇数编号的配置寄存器（`pmpcfg1`、`pmpcfg3`、…、`pmpcfg15`）是非法的。 |
| `norm:pmp_addr_registers` | The PMP address registers are CSRs named `pmpaddr0`-`pmpaddr63`. |  PMP 地址寄存器为 `pmpaddr0`-`pmpaddr63`。 |
| `norm:pmp_addr_encoding` | Each PMP address register encodes bits 33-2 of a 34-bit physical address for RV32, as shown in pmpaddr-rv32. For RV64, each PMP address register encodes bits 55-2 of a 56-bit physical address, as shown in pmpaddr-rv64. |  RV32 下每个 PMP 地址寄存器编码 34 位物理地址的 [33:2] 位；RV64 下编码 56 位物理地址的 [55:2] 位。 |
| `norm:pmp_addr_warl` | Not all physical address bits may be implemented, and so the `pmpaddr` registers are **WARL**. |  并非所有物理地址位都必须在 PMP 地址寄存器中编码；未实现的位为只读零。 |
| `norm:pmp_cfg_permissions` | pmpcfg shows the layout of a PMP configuration register. The R, W, and X bits, when set, indicate that the PMP entry permits read, write, and instruction execution, respectively. When one of these bits is clear, the corresponding access type is denied. |  配置寄存器的 R、W、X 位分别编码读、写、执行权限。 |
| `norm:pmp_rwx_warl` | The R, W, and X fields form a collective **WARL** field for which the combinations with R=0 and W=1 are reserved. |  R、W、X 字段是 WARL 字段。 |
| `norm:pmp_exec_fault` | Attempting to fetch an instruction from a PMP region that does not have execute permissions raises an instruction access-fault exception. |  如果 PMP 条目匹配但 X=0，则触发指令访问错误异常。 |
| `norm:pmp_load_fault` | Attempting to execute a load, load-reserved, or cache-block management instruction which accesses a physical address within a PMP region without read permissions raises a load access-fault exception. |  如果 PMP 条目匹配但 R=0，则触发加载访问错误异常。 |
| `norm:pmp_store_fault` | Attempting to execute a store, store-conditional, AMO, or cache-block zero instruction which accesses a physical address within a PMP region without write permissions raises a store access-fault exception. |  如果 PMP 条目匹配但 W=0，则触发存储/AMO 访问错误异常。 |

#### Address Matching

*Path: Machine-Level ISA, Version 1.13 > Physical Memory Protection > Physical Memory Protection CSRs > Address Matching*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pmp_a_field_encoding` | The A field in a PMP entry's configuration register encodes the address-matching mode of the associated PMP address register. The encoding of this field is shown in pmpcfg-a. |  A 字段编码地址匹配模式：0=OFF（禁用），1=TOR（顶部范围），2=NA4（自然对齐 4 字节），3=NAPOT（自然对齐 2 的幂次）。 |
| `norm:pmp_a_field_off` | When A=0, this PMP entry is disabled and matches no addresses. |  当 A=OFF 时，该 PMP 条目被禁用，不匹配任何地址。 |
| `norm:pmp_a_field_napot_na4` | naturally aligned power-of-2 regions (NAPOT), including the special case of naturally aligned four-byte regions (NA4); |  NA4 和 NAPOT 模式使用地址寄存器中的低位来编码区域大小。 |
| `norm:pmp_napot_encoding_low_bits` | NAPOT ranges make use of the low-order bits of the associated address register to encode the size of the range, as shown in pmpcfg-napot. |  对于 NAPOT 模式，地址寄存器的低位编码区域大小和对齐。 |
| `norm:pmp_a_field_tor` | If TOR is selected, the associated address register forms the top of the address range, and the preceding PMP address register forms the bottom of the address range. If PMP entry *i*'s A field is set to TOR, the entry matches any address _y_ such that `pmpaddr~i-1~`<=*y*<``pmpaddr~i~`` (irrespective of the value of `pmpcfg~i-1~`). If PMP entry 0's A field is set to TOR, zero is used for the lower bound, and so it matches any address `y<pmpaddr~0~`. |  TOR（Top of Range）模式使用前一个条目的地址作为区域起始地址，当前条目的地址作为区域结束地址。 |
| `norm:pmp_region_granularity_g` | Although the PMP mechanism supports regions as small as four bytes, platforms may specify coarser PMP regions. In general, the PMP grain is 2^G+2^ bytes and must be the same across all PMP regions. |  PMP 区域的最小粒度 G 是实现定义的，但至少为 4 字节。 |
| `norm:pmp_na4_select_restriction` | When G >= 1, the NA4 mode is not selectable. |  如果 G>2，则 NA4 模式不可选择，A 字段编码 2 为保留值。 |
| `norm:pmp_napot_addr_read_mask` | When G >= 2 and pmpcfg~i~.A[1] is set, i.e. the mode is NAPOT, then bits pmpaddr~i~[G-2:0] read as all ones. |  对于 NAPOT 区域，读取地址寄存器时，低位被屏蔽为零。 |
| `norm:pmp_tor_off_addr_read_mask` | When G >= 1 and pmpcfg~i~.A[1] is clear, i.e. the mode is OFF or TOR, then bits pmpaddr~i~[G-1:0] read as all zeros. Bits pmpaddr~i~[G-1:0] do not affect the TOR address-matching logic. |  对于 TOR 或 OFF 区域，读取地址寄存器时返回实际值。 |
| `norm:pmp_addr_retention_on_mode_change` | Although changing pmpcfg~i~.A[1] affects the value read from pmpaddr~i~, it does not affect the underlying value stored in that register—in particular, pmpaddr~i~[G-1] retains its original value when pmpcfg~i~.A is changed from NAPOT to TOR/OFF then back to NAPOT. |  当 PMP 条目的 A 字段从 TOR 或 OFF 更改为 NAPOT 时，地址寄存器的值被保留。 |

#### Locking and Privilege Mode

*Path: Machine-Level ISA, Version 1.13 > Physical Memory Protection > Physical Memory Protection CSRs > Locking and Privilege Mode*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pmp_l_bit_function` | The L bit indicates that the PMP entry is locked, i.e., writes to the configuration register and associated address registers are ignored. |  L 位控制 PMP 条目是否被锁定。 |
| `norm:pmp_l_bit_write_protection` | If PMP entry _i_ is locked, writes to ``pmp``*i*``cfg`` and ``pmpaddr``*i* are ignored. Additionally, if PMP entry _i_ is locked and ``pmp``*i*``cfg.A`` is set to TOR, writes to ``pmpaddr``*i*-1 are ignored. |  当 L=1 时，该 PMP 条目及其配置寄存器被锁定，M 模式软件也无法修改，直到 hart 复位。 |
| `norm:pmp_l_bit_m_mode_enforcement` | In addition to locking the PMP entry, the L bit indicates whether the R/W/X permissions are additionally enforced on M-mode accesses. When the L bit is set, these permissions are enforced for all privilege modes. When the L bit is clear, any M-mode access matching the PMP entry will succeed; the R/W/X permissions apply only to S and U modes. |  当 L=1 时，该 PMP 条目也适用于 M 模式访问。 |

#### Priority and Matching Logic

*Path: Machine-Level ISA, Version 1.13 > Physical Memory Protection > Physical Memory Protection CSRs > Priority and Matching Logic*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pmp_misaligned_access_behavior` | On some implementations, misaligned loads, stores, and instruction fetches may be decomposed into multiple memory operations, some of which may succeed before an access-fault exception occurs, as described in the RVWMO specification. PMP checking is performed on each memory operation independently. |  对于未对齐的内存访问，如果访问的任何字节不在允许的区域内，则触发访问错误异常。 |
| `norm:pmp_entry_priority` | PMP entries are statically prioritized. The lowest-numbered PMP entry that matches any byte of a memory operation determines whether that operation succeeds or fails. |  PMP 条目按编号顺序检查，第一个匹配的条目决定访问权限。 |
| `norm:pmp_full_match_required` | The matching PMP entry must match all bytes of a memory operation, or the operation fails, irrespective of the L, R, W, and X bits. |  对于 TOR 模式，整个访问范围必须在 [pmpaddr_{i-1}, pmpaddr_i) 范围内。 |
| `norm:pmp_rwx_check` | If a PMP entry matches all bytes of a memory operation, then the L, R, W, and X bits determine whether the operation succeeds or fails. If the L bit is clear and the privilege mode of the access is M, the operation succeeds. Otherwise, if the L bit is set or the privilege mode of the access is S or U, then the operation succeeds only if the R, W, or X bit corresponding to the access type is set. |  PMP 检查验证访问类型（读/写/执行）是否被允许。 |
| `norm:pmp_no_entry_match` | If no PMP entry matches an M-mode memory operation, the operation succeeds. If no PMP entry matches an S-mode or U-mode memory operation, but at least one PMP entry is implemented, the operation fails. |  如果没有 PMP 条目匹配，则对于 S/U 模式访问触发访问错误异常。 |
| `norm:pmp_access_fault_exception` | Failed memory operations generate an instruction, load, or store access-fault exception. |  PMP 违规触发访问错误异常（而非页面错误）。 |

#### Physical Memory Protection and Paging

*Path: Machine-Level ISA, Version 1.13 > Physical Memory Protection > Physical Memory Protection and Paging*

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pmp_with_paging` | When paging is enabled, instructions that access virtual memory may result in multiple physical-memory accesses, including implicit references to the page tables. The PMP checks apply to all of these accesses. |  当启用基于页面的虚拟内存时，PMP 检查在地址翻译之后进行。 |
| `norm:pmp_speculative_translations` | Implementations with virtual memory are permitted to perform address translations speculatively and earlier than required by an explicit memory access, and are permitted to cache them in address translation cache structures—including possibly caching the identity mappings from effective address to physical address used in Bare translation modes and M-mode. The PMP settings for the resulting physical address may be checked (and possibly cached) at any point between the address translation and the explicit memory access. |  PMP 检查适用于所有物理地址访问，包括推测性访问。 |
| `norm:pmp_sfence_required` | when the PMP settings are modified, M-mode software must synchronize the PMP settings with the virtual memory system and any PMP or address-translation caches. This is accomplished by executing an SFENCE.VMA instruction with _rs1_=`x0` and _rs2_=`x0`, after the PMP CSRs are written. |  修改 PMP 配置后，需要执行 SFENCE.VMA 以确保一致性。 |
