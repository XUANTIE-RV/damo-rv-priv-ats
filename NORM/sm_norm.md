# Sm 扩展 Norm 规范条目表

> 来源：`SPEC/sm.adoc` — Machine Extensions 章节

## Smstateen - 状态使能控制

### 基本操作

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:stateen_op` | The `stateen` registers at each level control access to state at all less-privileged levels, but not at its own level. | 各级 `stateen` 寄存器控制对所有较低特权级别的状态访问，但不控制其自身级别。 |
| `norm:stateen_illegal_state_access` | Just as with the `counteren` CSRs, when a `stateen` CSR prevents access to state by less-privileged levels, an attempt in one of those privilege modes to execute an instruction that would read or write the protected state raises an illegal-instruction exception, or, if executing in VS or VU mode and the circumstances for a virtual-instruction exception apply, raises a virtual-instruction exception instead of an illegal-instruction exception. | 与 `counteren` CSR 类似，当 `stateen` CSR 阻止较低特权级别访问状态时，在这些特权模式下尝试执行读取或写入受保护状态的指令会触发非法指令异常，或者如果在 VS 或 VU 模式下执行且满足虚拟指令异常的条件，则触发虚拟指令异常而非非法指令异常。 |
| `norm:stateen_implicit_state_update` | When a `stateen` CSR prevents access to state for a privilege mode, attempting to execute in that privilege mode an instruction that _implicitly_ updates the state without reading it may or may not raise an illegal-instruction or virtual-instruction exception. Such cases must be disambiguated by being explicitly specified one way or the other. | 当 `stateen` CSR 阻止某个特权模式访问状态时，在该特权模式下尝试执行隐式更新状态而不读取状态的指令可能会也可能不会触发非法指令或虚拟指令异常。此类情况必须通过明确指定来消除歧义。 |

### Supervisor 级别控制

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sstateen_user_access_control` | Each bit of a supervisor-level `sstateen` CSR controls user-level access (from U-mode or VU-mode) to an extension's state. | 监管级 `sstateen` CSR 的每一位控制用户级（来自 U 模式或 VU 模式）对扩展状态的访问。 |
| `norm:sstateen_bit_allocation` | The intention is to allocate the bits of `sstateen` CSRs starting at the least-significant end, bit 0, through to bit 31, and then on to the next-higher-numbered `sstateen` CSR. | 意图是从最低有效端（位 0）开始分配 `sstateen` CSR 的位，到位 31，然后继续到下一个更高编号的 `sstateen` CSR。 |

### Machine 级别控制

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mstateen_bit_correspondence` | For every bit with a defined purpose in an `sstateen` CSR, the same bit is defined in the matching `mstateen` CSR to control access below machine level to the same state. | 对于 `sstateen` CSR 中具有定义用途的每一位，匹配的 `mstateen` CSR 中定义了相同的位，以控制机器级别以下对相同状态的访问。 |
| `norm:mstateen_bit_allocation` | The intention is to allocate bits for this purpose starting at the most-significant end, bit 63, through to bit 32, and then on to the next-higher `mstateen` CSR. | 意图是从最高有效端（位 63）开始为此目的分配位，到位 32，然后继续到下一个更高的 `mstateen` CSR。 |
| `norm:mstateen_bit_encroachment` | If the rate that bits are being allocated from the least-significant end for `sstateen` CSRs is sufficiently low, allocation from the most-significant end of `mstateen` CSRs may be allowed to encroach on the lower 32 bits before jumping to the next-higher `mstateen` CSR. | 如果从 `sstateen` CSR 的最低有效端分配位的速率足够低，则允许从 `mstateen` CSR 的最高有效端分配侵入低 32 位，然后再跳转到下一个更高的 `mstateen` CSR。 |
| `norm:sstateen_encroachment_bits_roz` | In that case, the bit positions of "encroaching" bits will remain forever read-only zeros in the matching `sstateen` CSRs. | 在这种情况下，"侵入"位的位位置在匹配的 `sstateen` CSR 中将永远保持为只读零。 |

### Hypervisor 级别控制

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:hstateen_encoding` | With the hypervisor extension, the `hstateen` CSRs have identical encodings to the `mstateen` CSRs, except controlling accesses for a virtual machine (from VS and VU modes). | 使用 hypervisor 扩展时，`hstateen` CSR 具有与 `mstateen` CSR 相同的编码，只是控制虚拟机（来自 VS 和 VU 模式）的访问。 |

### WARL 与只读位

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:stateen_warl_access` | Each standard-defined bit of a `stateen` CSR is WARL and may be read-only zero or one, subject to the following conditions. | `stateen` CSR 的每个标准定义位都是 WARL，可以是只读零或一，需满足以下条件。 |
| `norm:stateen_unimplemented_state_roz` | Bits in any `stateen` CSR that are defined to control state that a hart doesn't implement are read-only zeros for that hart. | 任何 `stateen` CSR 中定义为控制 hart 未实现的状态的位对该 hart 而言是只读零。 |
| `norm:stateen_reserved_roz` | Likewise, all reserved bits not yet given a defined meaning are also read-only zeros. | 同样，所有尚未赋予定义含义的保留位也是只读零。 |
| `norm:mstateen_lower_priv_roz` | For every bit in an `mstateen` CSR that is zero (whether read-only zero or set to zero), the same bit appears as read-only zero in the matching `hstateen` and `sstateen` CSRs. | 对于 `mstateen` CSR 中为零的每一位（无论是只读零还是设置为零），相同的位在匹配的 `hstateen` 和 `sstateen` CSR 中显示为只读零。 |
| `norm:sstateen_vsmode_access_roz` | For every bit in an `hstateen` CSR that is zero (whether read-only zero or set to zero), the same bit appears as read-only zero in `sstateen` when accessed in VS-mode. | 对于 `hstateen` CSR 中为零的每一位（无论是只读零还是设置为零），在 VS 模式下访问时，相同的位在 `sstateen` 中显示为只读零。 |
| `norm:sstateen_ro1_bits` | A bit in a supervisor-level `sstateen` CSR cannot be read-only one unless the same bit is read-only one in the matching `mstateen` CSR and, if it exists, in the matching `hstateen` CSR. | 监管级 `sstateen` CSR 中的位不能是只读一，除非相同的位在匹配的 `mstateen` CSR 中以及（如果存在）匹配的 `hstateen` CSR 中是只读一。 |
| `norm:hstateen_ro1_bits` | A bit in an `hstateen` CSR cannot be read-only one unless the same bit is read-only one in the matching `mstateen` CSR. | `hstateen` CSR 中的位不能是只读一，除非相同的位在匹配的 `mstateen` CSR 中是只读一。 |

### 初始化

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mstateen_zero_initialization` | On reset, all writable `mstateen` bits are initialized by the hardware to zeros. | 复位时，所有可写的 `mstateen` 位由硬件初始化为零。 |
| `norm:hstateen_sstateen_zero_initialization` | If machine-level software changes these values, it is responsible for initializing the corresponding writable bits of the `hstateen` and `sstateen` CSRs to zeros too. | 如果机器级软件更改这些值，它负责将 `hstateen` 和 `sstateen` CSR 的相应可写位也初始化为零。 |

### 位 63 控制

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mstateen_bit_63_op` | For each `mstateen` CSR, bit 63 is defined to control access to the matching `sstateen` and `hstateen` CSRs. | 对于每个 `mstateen` CSR，位 63 定义为控制对匹配的 `sstateen` 和 `hstateen` CSR 的访问。 |
| `norm:hstateen_bit_63_op` | Likewise, bit 63 of each `hstateen` correspondingly controls access to the matching `sstateen` CSR. | 同样，每个 `hstateen` 的位 63 相应地控制对匹配的 `sstateen` CSR 的访问。 |
| `norm:mstateen_bit_63_roz` | Bit 63 of each `mstateen` CSR may be read-only zero only if the hypervisor extension is not implemented and the matching supervisor-level `sstateen` CSR is all read-only zeros. | 仅当未实现 hypervisor 扩展且匹配的监管级 `sstateen` CSR 全为只读零时，每个 `mstateen` CSR 的位 63 才可以是只读零。 |
| `norm:hstateen_bit_63_writable` | Bit 63 of each `hstateen` CSR is always writable (not read-only). | 每个 `hstateen` CSR 的位 63 始终可写（非只读）。 |

### State Enable 0 寄存器位定义

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:stateen0_c_op` | The C bit controls access to any and all custom state. | C 位控制对任何和所有自定义状态的访问。 |
| `norm:stateen0_fcsr_op` | The FCSR bit controls access to `fcsr` for the case when floating-point instructions operate on `x` registers instead of `f` registers as specified by the Zfinx and related extensions (Zdinx, etc.). | FCSR 位控制对 `fcsr` 的访问，适用于浮点指令在 `x` 寄存器而非 `f` 寄存器上操作的情况（如 Zfinx 及相关扩展所指定）。 |
| `norm:stateen0_fcsr0_misa_f0_illegal_fpu_instr` | For convenience, when the `stateen` CSRs are implemented and `misa.F` = 0, then if the FCSR bit of a controlling `stateen0` CSR is zero, all floating-point instructions cause an illegal-instruction exception (or virtual-instruction exception, if relevant), as though they all access `fcsr`, regardless of whether they really do. | 为方便起见，当实现 `stateen` CSR 且 `misa.F` = 0 时，如果控制 `stateen0` CSR 的 FCSR 位为零，所有浮点指令都会导致非法指令异常（或虚拟指令异常，如果相关），就好像它们都访问 `fcsr` 一样，无论它们是否真的访问。 |
| `norm:stateen0_jvt_op` | The JVT bit controls access to the `jvt` CSR provided by the Zcmt extension. | JVT 位控制对 Zcmt 扩展提供的 `jvt` CSR 的访问。 |
| `norm:mstateen0_se0_op` | The SE0 bit in `mstateen0` controls access to the `hstateen0`, `hstateen0h`, and the `sstateen0` CSRs. | `mstateen0` 中的 SE0 位控制对 `hstateen0`、`hstateen0h` 和 `sstateen0` CSR 的访问。 |
| `norm:hstateen0_SE0_op` | The SE0 bit in `hstateen0` controls access to the `sstateen0` CSR. | `hstateen0` 中的 SE0 位控制对 `sstateen0` CSR 的访问。 |
| `norm:mstateen0_envcfg_op` | The ENVCFG bit in `mstateen0` controls access to the `henvcfg`, `henvcfgh`, and the `senvcfg` CSRs. | `mstateen0` 中的 ENVCFG 位控制对 `henvcfg`、`henvcfgh` 和 `senvcfg` CSR 的访问。 |
| `norm:hstateen0_envcfg_op` | The ENVCFG bit in `hstateen0` controls access to the `senvcfg` CSRs. | `hstateen0` 中的 ENVCFG 位控制对 `senvcfg` CSR 的访问。 |
| `norm:mstateen0_csrind_op` | The CSRIND bit in `mstateen0` controls access to the `siselect`, `sireg*`, `vsiselect`, and the `vsireg*` CSRs provided by the Sscsrind extensions. | `mstateen0` 中的 CSRIND 位控制对 Sscsrind 扩展提供的 `siselect`、`sireg*`、`vsiselect` 和 `vsireg*` CSR 的访问。 |
| `norm:hstateen0_csrind_op` | The CSRIND bit in `hstateen0` controls access to the `siselect` and the `sireg*`, (really `vsiselect` and `vsireg*`) CSRs provided by the Sscsrind extensions. | `hstateen0` 中的 CSRIND 位控制对 Sscsrind 扩展提供的 `siselect` 和 `sireg*`（实际上是 `vsiselect` 和 `vsireg*`）CSR 的访问。 |
| `norm:mstateen0_imsic_op` | The IMSIC bit in `mstateen0` controls access to the IMSIC state, including CSRs `stopei` and `vstopei`, provided by the Ssaia extension. | `mstateen0` 中的 IMSIC 位控制对 Ssaia 扩展提供的 IMSIC 状态（包括 CSR `stopei` 和 `vstopei`）的访问。 |
| `norm:hstateen0_imsic_op` | The IMSIC bit in `hstateen0` controls access to the guest IMSIC state, including CSRs `stopei` (really `vstopei`), provided by the Ssaia extension. | `hstateen0` 中的 IMSIC 位控制对 Ssaia 扩展提供的客户 IMSIC 状态（包括 CSR `stopei`，实际上是 `vstopei`）的访问。 |
| `norm:mstateen0_aia_op` | The AIA bit in `mstateen0` controls access to all state introduced by the Ssaia extension and not controlled by either the CSRIND or the IMSIC bits. | `mstateen0` 中的 AIA 位控制对 Ssaia 扩展引入的所有状态的访问，这些状态不受 CSRIND 或 IMSIC 位控制。 |
| `norm:hstateen0_aia_op` | The AIA bit in `hstateen0` controls access to all state introduced by the Ssaia extension and not controlled by either the CSRIND or the IMSIC bits of `hstateen0`. | `hstateen0` 中的 AIA 位控制对 Ssaia 扩展引入的所有状态的访问，这些状态不受 `hstateen0` 的 CSRIND 或 IMSIC 位控制。 |
| `norm:mstateen0_context_op` | The CONTEXT bit in `mstateen0` controls access to the `scontext` and `hcontext` CSRs provided by the Sdtrig extension. | `mstateen0` 中的 CONTEXT 位控制对 Sdtrig 扩展提供的 `scontext` 和 `hcontext` CSR 的访问。 |
| `norm:hstateen0_context_op` | The CONTEXT bit in `hstateen0` controls access to the `scontext` CSR provided by the Sdtrig extension. | `hstateen0` 中的 CONTEXT 位控制对 Sdtrig 扩展提供的 `scontext` CSR 的访问。 |
| `norm:mstateen0_p1p13_op` | The P1P13 bit in `mstateen0` controls access to the `hedelegh` introduced by Privileged Specification Version 1.13. | `mstateen0` 中的 P1P13 位控制对特权规范版本 1.13 引入的 `hedelegh` 的访问。 |
| `norm:mstateen0_srmcfg_op` | The SRMCFG bit in `mstateen0` controls access to the `srmcfg` CSR introduced by the Ssqosid extension. | `mstateen0` 中的 SRMCFG 位控制对 Ssqosid 扩展引入的 `srmcfg` CSR 的访问。 |

## Smcsrind/Sscsrind - 间接 CSR 访问

### 基本概念

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:csr_access` | A CSR accessible via one method may or may not be accessible via the other method. | 通过一种方法可访问的 CSR 可能通过另一种方法可访问，也可能不可访问。 |
| `norm:select_value_separate_address_space` | Select values are a separate address space from CSR numbers, and from tselect values in the Sdtrig extension. | 选择值是与 CSR 编号以及 Sdtrig 扩展中的 tselect 值不同的地址空间。 |
| `norm:select_value_unrelated` | If a CSR is both directly and indirectly accessible, the CSR's select value is unrelated to its CSR number. | 如果 CSR 既可直接访问又可间接访问，则 CSR 的选择值与其 CSR 编号无关。 |
| `norm:csrs_alias` | Machine-level and Supervisor-level CSRs with the same select value may be defined by an extension as partial or full aliases with respect to each other. | 具有相同选择值的机器级和监管级 CSR 可由扩展定义为彼此的部分或完全别名。 |

### Machine 级别 CSR

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:miph_csr_number` | The `mireg*` CSR numbers are not consecutive because miph is CSR number 0x354. | `mireg*` CSR 编号不连续，因为 miph 是 CSR 编号 0x354。 |
| `norm:miselect_op` | The value of `miselect` determines which register is accessed upon read or write of each of the machine indirect alias CSRs (`mireg*`). | `miselect` 的值决定在读取或写入每个机器间接别名 CSR（`mireg*`）时访问哪个寄存器。 |
| `norm:miselect_value_range` | `miselect` value ranges are allocated to dependent extensions, which specify the register state accessible via each `mireg_i` register, for each `miselect` value. | `miselect` 值范围分配给依赖扩展，这些扩展指定每个 `miselect` 值可通过每个 `mireg_i` 寄存器访问的寄存器状态。 |
| `norm:miselect_WARL` | `miselect` is a WARL register. | `miselect` 是 WARL 寄存器。 |
| `norm:miselect_min_sz` | The `miselect` register implements at least enough bits to support all implemented `miselect` values (corresponding to the implemented extensions that utilize `miselect`/`mireg*` to indirectly access register state). | `miselect` 寄存器至少实现足够的位以支持所有已实现的 `miselect` 值（对应于利用 `miselect`/`mireg*` 间接访问寄存器状态的已实现扩展）。 |
| `norm:miselect_msb_op` | Values of `miselect` with the most-significant bit set (bit XLEN - 1 = 1) are designated only for custom use, presumably for accessing custom registers through the alias CSRs. Values of `miselect` with the most-significant bit clear are designated only for standard use and are reserved until allocated to a standard architecture extension. If XLEN is changed, the most-significant bit of `miselect` moves to the new position, retaining its value from before. | 最高有效位置位（位 XLEN - 1 = 1）的 `miselect` 值仅指定用于自定义用途，可能是通过别名 CSR 访问自定义寄存器。最高有效位清零的 `miselect` 值仅指定用于标准用途，并在分配给标准架构扩展之前保留。如果 XLEN 更改，`miselect` 的最高有效位移动到新位置，保留其之前的值。 |
| `norm:mireg_access_on_legal_miselect` | Attempts to access `mireg*` while `miselect` holds a number in an allocated and implemented range results in a specific behavior that, for each combination of `miselect` and `mireg_i`, is defined by the extension to which the `miselect` value is allocated. | 当 `miselect` 持有已分配和实现范围内的数字时，尝试访问 `mireg*` 会导致特定行为，对于 `miselect` 和 `mireg_i` 的每种组合，由分配 `miselect` 值的扩展定义。 |
| `norm:mireg_access_behaviour` | Ordinarily, each `mireg*_i_* will access register state, access read-only 0 state, or raise an illegal-instruction exception. | 通常，每个 `mireg*_i_* 将访问寄存器状态、访问只读 0 状态或触发非法指令异常。 |

### Supervisor 级别 CSR

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:siselect_min_range` | The `siselect` register will support the value range 0..0xFFF at a minimum. | `siselect` 寄存器至少支持值范围 0..0xFFF。 |
| `norm:siselect_msb_op` | Values of `siselect` with the most-significant bit set (bit XLEN - 1 = 1) are designated only for custom use, presumably for accessing custom registers through the alias CSRs. Values of `siselect` with the most-significant bit clear are designated only for standard use and are reserved until allocated to a standard architecture extension. If XLEN is changed, the most-significant bit of `siselect` moves to the new position, retaining its value from before. | 最高有效位置位（位 XLEN - 1 = 1）的 `siselect` 值仅指定用于自定义用途，可能是通过别名 CSR 访问自定义寄存器。最高有效位清零的 `siselect` 值仅指定用于标准用途，并在分配给标准架构扩展之前保留。如果 XLEN 更改，`siselect` 的最高有效位移动到新位置，保留其之前的值。 |
| `norm:sireg_access_on_illegal_siselect` | The behavior upon accessing `sireg*` from M-mode or S-mode, while `siselect` holds a value that is not implemented at supervisor level, is UNSPECIFIED. | 当 `siselect` 持有在监管级未实现的值时，从 M 模式或 S 模式访问 `sireg*` 的行为是未指定的。 |
| `norm:sireg_access_on_legal_siselect` | Otherwise, attempts to access `sireg*` from M-mode or S-mode while `siselect` holds a number in a standard-defined and implemented range result in specific behavior that, for each combination of `siselect` and `sireg_i`, is defined by the extension to which the `siselect` value is allocated. | 否则，当 `siselect` 持有标准定义和实现范围内的数字时，从 M 模式或 S 模式尝试访问 `sireg*` 会导致特定行为，对于 `siselect` 和 `sireg_i` 的每种组合，由分配 `siselect` 值的扩展定义。 |
| `norm:sireg_access_behaviour` | Ordinarily, each `sireg*_i_* will access register state, access read-only 0 state, or, unless executing in a virtual machine (covered in the next section), raise an illegal-instruction exception. | 通常，每个 `sireg*_i_* 将访问寄存器状态、访问只读 0 状态，或者（除非在虚拟机中执行，见下一节）触发非法指令异常。 |
| `norm:sscsrind_smode_csrs_sz` | Note that the widths of `siselect` and `sireg*` are always the current XLEN rather than SXLEN. Hence, for example, if MXLEN = 64 and SXLEN = 32, then these registers are 64 bits when the current privilege mode is M (running RV64 code) but 32 bits when the privilege mode is S (RV32 code). | 注意 `siselect` 和 `sireg*` 的宽度始终是当前 XLEN 而非 SXLEN。因此，例如，如果 MXLEN = 64 且 SXLEN = 32，则当当前特权模式为 M（运行 RV64 代码）时这些寄存器为 64 位，但当特权模式为 S（RV32 代码）时为 32 位。 |

### Virtual Supervisor 级别 CSR

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:vsiselect_min_range` | The `vsiselect` register will support the value range 0..0xFFF at a minimum. | `vsiselect` 寄存器至少支持值范围 0..0xFFF。 |
| `norm:vsiselect_msb_op` | Values of `vsiselect` with the most-significant bit set (bit XLEN - 1 = 1) are designated only for custom use, presumably for accessing custom registers through the alias CSRs. Values of `vsiselect` with the most-significant bit clear are designated only for standard use and are reserved until allocated to a standard architecture extension. If XLEN is changed, the most-significant bit of `vsiselect` moves to the new position, retaining its value from before. | 最高有效位置位（位 XLEN - 1 = 1）的 `vsiselect` 值仅指定用于自定义用途，可能是通过别名 CSR 访问自定义寄存器。最高有效位清零的 `vsiselect` 值仅指定用于标准用途，并在分配给标准架构扩展之前保留。如果 XLEN 更改，`vsiselect` 的最高有效位移动到新位置，保留其之前的值。 |
| `norm:sscsrind_virtual_inst_fault` | A virtual-instruction exception is raised for attempts from VS-mode or VU-mode to directly access `vsiselect` or `vsireg*`, or attempts from VU-mode to access `siselect` or `sireg*`. | 从 VS 模式或 VU 模式尝试直接访问 `vsiselect` 或 `vsireg*`，或从 VU 模式尝试访问 `siselect` 或 `sireg*` 会触发虚拟指令异常。 |
| `norm:vsireg_access_on_legal_vsiselect` | Otherwise, while `vsiselect` holds a number in a standard-defined and implemented range, attempts to access `vsireg*` from a sufficiently privileged mode, or to access `sireg*` (really `vsireg*`) from VS-mode, result in specific behavior that, for each combination of `vsiselect` and `vsireg_i`, is defined by the extension to which the `vsiselect` value is allocated. | 否则，当 `vsiselect` 持有标准定义和实现范围内的数字时，从足够特权的模式尝试访问 `vsireg*`，或从 VS 模式尝试访问 `sireg*`（实际上是 `vsireg*`）会导致特定行为，对于 `vsiselect` 和 `vsireg_i` 的每种组合，由分配 `vsiselect` 值的扩展定义。 |
| `norm:vsireg_access_behaviour` | Ordinarily, each `vsireg*_i_* will access register state, access read-only 0 state, or raise an exception (either an illegal-instruction exception or, for select accesses from VS-mode, a virtual-instruction exception). | 通常，每个 `vsireg*_i_* 将访问寄存器状态、访问只读 0 状态或触发异常（非法指令异常或，对于从 VS 模式的选择访问，虚拟指令异常）。 |
| `norm:vsmode_virtual_inst_fault` | When `vsiselect` holds a value that is implemented at HS level but not at VS level, attempts to access `sireg*` (really `vsireg*`) from VS-mode will typically raise a virtual-instruction exception. | 当 `vsiselect` 持有在 HS 级实现但在 VS 级未实现的值时，从 VS 模式尝试访问 `sireg*`（实际上是 `vsireg*`）通常会触发虚拟指令异常。 |
| `norm:sscsrind_vsmode_csrs_sz` | Like `siselect` and `sireg*`, the widths of `vsiselect` and `vsireg*` are always the current XLEN rather than VSXLEN. Hence, for example, if HSXLEN = 64 and VSXLEN = 32, then these registers are 64 bits when accessed by a hypervisor in HS-mode (running RV64 code) but 32 bits for a guest OS in VS-mode (RV32 code). | 与 `siselect` 和 `sireg*` 类似，`vsiselect` 和 `vsireg*` 的宽度始终是当前 XLEN 而非 VSXLEN。因此，例如，如果 HSXLEN = 64 且 VSXLEN = 32，则当由 HS 模式（运行 RV64 代码）的 hypervisor 访问时这些寄存器为 64 位，但对于 VS 模式（RV32 代码）的客户操作系统为 32 位。 |

### 访问控制

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sscsrind_csrs_access_control` | If extension Smstateen is implemented together with Smcsrind, bit 60 of state-enable register `mstateen0` controls access to `siselect`, `sireg*`, `vsiselect`, and `vsireg*`. When `mstateen0`[60]=0, an attempt to access one of these CSRs from a privilege mode less privileged than M-mode results in an illegal-instruction exception. | 如果 Smstateen 扩展与 Smcsrind 一起实现，状态使能寄存器 `mstateen0` 的位 60 控制对 `siselect`、`sireg*`、`vsiselect` 和 `vsireg*` 的访问。当 `mstateen0`[60]=0 时，从低于 M 模式的特权模式尝试访问这些 CSR 之一会导致非法指令异常。 |
| `norm:hypervisor_impl_csrs_access_control` | If the hypervisor extension is implemented, the same bit is defined also in hypervisor CSR `hstateen0`, but controls access to only `siselect` and `sireg*` (really `vsiselect` and `vsireg*`), which is the state potentially accessible to a virtual machine executing in VS or VU-mode. When `hstateen0`[60]=0 and `mstateen0`[60]=1, all attempts from VS or VU-mode to access `siselect` or `sireg*` raise a virtual-instruction exception, not an illegal-instruction exception, regardless of the value of `vsiselect` or any other mstateen bit. | 如果实现了 hypervisor 扩展，相同的位也在 hypervisor CSR `hstateen0` 中定义，但仅控制对 `siselect` 和 `sireg*`（实际上是 `vsiselect` 和 `vsireg*`）的访问，这是虚拟机在 VS 或 VU 模式下执行时可能访问的状态。当 `hstateen0`[60]=0 且 `mstateen0`[60]=1 时，从 VS 或 VU 模式尝试访问 `siselect` 或 `sireg*` 会触发虚拟指令异常，而非非法指令异常，无论 `vsiselect` 的值或任何其他 mstateen 位如何。 |

## Smepmp - PMP 增强

### 威胁模型

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:smepmp_no_mml_limit` | Without the Smepmp extension, it is not possible for a PMP rule to be *enforced* only on non-Machine modes and *denied* on Machine mode, in order to allow access to a memory region solely by less-privileged modes. | 如果没有 Smepmp 扩展，PMP 规则不可能仅在非机器模式下*强制执行*而在机器模式下*拒绝*，以允许仅由较低特权模式访问内存区域。 |
| `norm:smepmp_no_mml_behavior` | It is only possible to have a *locked* rule that will be *enforced* on all modes, or a rule that will be *enforced* on non-Machine modes and be *ignored* by Machine mode. | 只能有一个在所有模式下*强制执行*的*锁定*规则，或者一个在非机器模式下*强制执行*并被机器模式*忽略*的规则。 |
| `norm:smepmp_machine_unlimited` | So for any physical memory region which is not protected with a Locked rule, Machine mode has unlimited access, including the ability to execute it. | 因此，对于任何未受锁定规则保护的物理内存区域，机器模式具有无限访问权限，包括执行它的能力。 |
| `norm:smepmp_attack_surface` | Without being able to protect less-privileged modes from Machine mode, it is not possible to prevent the mentioned attack vector. This becomes even more important for RISC-V than on other architectures, since implementations are allowed where a hart only has Machine and User modes available, so the whole OS will run on Machine mode instead of the non-existent Supervisor mode. In such implementations the attack surface is greatly increased, and the same kind of attacks performed on Supervisor mode and mitigated through the virtual-memory system, can be performed on Machine mode without any available mitigations. Even on implementations with Supervisor mode present attacks are still possible against the Firmware and/or the Secure Monitor running on Machine mode. | 如果无法保护较低特权模式免受机器模式的攻击，就不可能防止上述攻击向量。这对于 RISC-V 比其他架构更重要，因为允许实现中 hart 仅具有机器和用户模式可用，因此整个操作系统将在机器模式下运行，而不是不存在的监管模式。在此类实现中，攻击面大大增加，在监管模式上执行并通过虚拟内存系统缓解的同类攻击可以在机器模式上执行而没有任何可用的缓解措施。即使在存在监管模式的实现中，针对在机器模式下运行的固件和/或安全监控器的攻击仍然是可能的。 |

### Smepmp 物理内存保护规则

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mseccfg_fields_exist` | this extension introduces the `RLB`, `MMWP`, and `MML` fields in the `mseccfg` CSR and their associated rules. | 此扩展在 `mseccfg` CSR 中引入 `RLB`、`MMWP` 和 `MML` 字段及其关联规则。 |
| `norm:mml_truth_table` | The physical memory protection rules when `mseccfg.MML` is set to 1 are summarized in the truth table below. | 当 `mseccfg.MML` 设置为 1 时的物理内存保护规则总结在下面的真值表中。 |

### Smepmp 软件发现

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mseccfg_locking` | Since all fields defined in `mseccfg` as part of this extension are locked when set (`MMWP`/`MML`) or locked when cleared (`RLB`), software can't poll them for determining the presence of Smepmp. | 由于作为此扩展一部分在 `mseccfg` 中定义的所有字段在设置时（`MMWP`/`MML`）或清除时（`RLB`）被锁定，软件无法轮询它们以确定 Smepmp 的存在。 |
| `norm:bootrom_discovery` | It is expected that BootROM will set `mseccfg.MMWP` and/or `mseccfg.MML` during early boot, before jumping to the firmware, so that the firmware will be able to determine the presence of Smepmp by reading `mseccfg` and checking the state of `mseccfg.MMWP` and `mseccfg.MML`. | 预期 BootROM 将在早期启动期间（在跳转到固件之前）设置 `mseccfg.MMWP` 和/或 `mseccfg.MML`，以便固件能够通过读取 `mseccfg` 并检查 `mseccfg.MMWP` 和 `mseccfg.MML` 的状态来确定 Smepmp 的存在。 |

## Smcntrpmf - 周期/指令计数模式过滤

### CSR 定义

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mcyclecfg_sz` | mcyclecfg and minstretcfg are 64-bit registers | mcyclecfg 和 minstretcfg 是 64 位寄存器 |
| `norm:mcyclecfg_op` | configure privilege mode filtering for the cycle and instret counters, respectively. | 分别为 cycle 和 instret 计数器配置特权模式过滤。 |
| `norm:all_xinh_zero` | When all __x__INH bits are zero, event counting is enabled in all modes. | 当所有 __x__INH 位为零时，在所有模式下启用事件计数。 |
| `norm:unimplemented_mode_bits` | For each bit in 61:58, if the associated privilege mode is not implemented, the bit is read-only zero. | 对于 61:58 中的每一位，如果关联的特权模式未实现，则该位为只读零。 |
| `norm:rv32_high_access` | For RV32, bits 63:32 of mcyclecfg can be accessed via the mcyclecfgh CSR, and bits 63:32 of minstretcfg can be accessed via the minstretcfgh CSR. | 对于 RV32，mcyclecfg 的 63:32 位可通过 mcyclecfgh CSR 访问，minstretcfg 的 63:32 位可通过 minstretcfgh CSR 访问。 |
| `norm:csr_supervisor_access` | The content of these registers may be accessible from Supervisor level if the Smcdeleg/Ssccfg extensions are implemented. | 如果实现了 Smcdeleg/Ssccfg 扩展，这些寄存器的内容可能从监管级可访问。 |

### 计数器行为

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:counter_inhibited_behavior` | The fundamental behavior of cycle and instret is modified in that counting does not occur while executing in an inhibited privilege mode. | cycle 和 instret 的基本行为被修改为在抑制的特权模式下执行时不进行计数。 |
| `norm:transition_counting_defined` | Further, the following defines how transitions between a non-inhibited privilege mode and an inhibited privilege mode are counted. | 此外，以下定义了非抑制特权模式和抑制特权模式之间的转换如何计数。 |
| `norm:cycle_counting` | The cycle counter will simply count CPU cycles while the CPU is in a non-inhibited privilege mode. | cycle 计数器仅在 CPU 处于非抑制特权模式时计算 CPU 周期。 |
| `norm:instret_non_inhibited` | instructions that retire in a non-inhibited mode increment instret, and instructions that retire in an inhibited mode do not. | 在非抑制模式下退休的指令递增 instret，在抑制模式下退休的指令不递增。 |
| `norm:instret_exception` | The former are not considered to retire, and hence do not increment instret. | 前者不被认为退休，因此不递增 instret。 |
| `norm:instret_xret` | The latter do retire, and should increment instret only if the originating privilege mode is not inhibited. | 后者确实退休，并且仅当源特权模式未被抑制时才应递增 instret。 |

## Smrnmi - 可恢复不可屏蔽中断

### RNMI 信号与地址

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Smrnmi_csrs` | The extension adds four new CSRs (`mnepc`, `mncause`, `mnstatus`, and `mnscratch`) to hold the interrupted state | 此扩展添加四个新 CSR（`mnepc`、`mncause`、`mnstatus` 和 `mnscratch`）以保存中断状态 |
| `norm:mnret_exist` | one new instruction, MNRET, to resume from the RNMI handler. | 一条新指令 MNRET，用于从 RNMI 处理程序恢复。 |
| `norm:rnmi_priority` | These interrupts have higher priority than any other interrupt or exception on the hart | 这些中断的优先级高于 hart 上的任何其他中断或异常 |
| `norm:rnmi_not_disabled` | cannot be disabled by software. Specifically, they are not disabled by clearing the `mstatus`.MIE register. | 无法被软件禁用。具体来说，它们不会通过清除 `mstatus`.MIE 寄存器而被禁用。 |
| `norm:rnmi_trap_addr` | The RNMI interrupt trap handler address is implementation-defined. | RNMI 中断陷阱处理程序地址是实现定义的。 |
| `norm:rnmi_exc_trap_addr` | RNMI also has an associated exception trap handler address, which is implementation defined. | RNMI 还有一个关联的异常陷阱处理程序地址，这是实现定义的。 |

### RNMI CSR

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mnscratch_sz` | The `mnscratch` CSR holds an MXLEN-bit | `mnscratch` CSR 保存一个 MXLEN 位 |
| `norm:mnscratch_acc` | read-write register | 读写寄存器 |
| `norm:mnscratch_op` | enables the RNMI trap handler to save and restore the context that was interrupted. | 使 RNMI 陷阱处理程序能够保存和恢复被中断的上下文。 |
| `norm:mnepc_sz` | The `mnepc` CSR is an MXLEN-bit | `mnepc` CSR 是一个 MXLEN 位 |
| `norm:mnepc_acc` | read-write register | 读写寄存器 |
| `norm:mnepc_op` | RNMI trap handler holds the PC of the instruction that took the interrupt. | RNMI 陷阱处理程序保存接受中断的指令的 PC。 |
| `norm:mnepc_bit0` | The low bit of `mnepc` (`mnepc[0]`) is always zero. | `mnepc` 的低位（`mnepc[0]`）始终为零。 |
| `norm:mnepc_ialign32` | On implementations that support only IALIGN=32, the two low bits (`mnepc[1:0]`) are always zero. | 在仅支持 IALIGN=32 的实现上，两个低位（`mnepc[1:0]`）始终为零。 |
| `norm:mnepc_ialign_mask` | If an implementation allows IALIGN to be either 16 or 32 (by changing CSR `misa`, for example), then, whenever IALIGN=32, bit `mnepc[1]` is masked on reads so that it appears to be 0. | 如果实现允许 IALIGN 为 16 或 32（例如通过更改 CSR `misa`），则每当 IALIGN=32 时，位 `mnepc[1]` 在读取时被屏蔽，使其显示为 0。 |
| `norm:mnepc_bit1_writable` | Though masked, `mnepc[1]` remains writable when IALIGN=32. | 虽然被屏蔽，但当 IALIGN=32 时 `mnepc[1]` 仍然可写。 |
| `norm:mnepc_warl` | `mnepc` is a *WARL* register that must be able to hold all valid virtual addresses. | `mnepc` 是一个 *WARL* 寄存器，必须能够保存所有有效虚拟地址。 |
| `norm:mnepc_invalid_convert` | Prior to writing `mnepc`, implementations may convert an invalid address into some other invalid address that `mnepc` is capable of holding. | 在写入 `mnepc` 之前，实现可以将无效地址转换为 `mnepc` 能够保存的其他无效地址。 |
| `norm:mncause_op` | The `mncause` CSR holds the reason for the RNMI. | `mncause` CSR 保存 RNMI 的原因。 |
| `norm:mncause_interrupt` | If the reason is an interrupt, bit MXLEN-1 is set to 1, | 如果原因是中断，则位 MXLEN-1 设置为 1， |
| `norm:mncause_interrupt_code` | the RNMI cause is encoded in the least-significant bits. | RNMI 原因编码在最低有效位中。 |
| `norm:mncause_interrupt_zero` | If the reason is an interrupt and RNMI causes are not supported, bit MXLEN-1 is set to 1, and zero is written to the least-significant bits. | 如果原因是中断且不支持 RNMI 原因，则位 MXLEN-1 设置为 1，并将零写入最低有效位。 |
| `norm:mncause_doubletrap` | If the reason is an exception within M-mode that results in a double trap as specified in the Smdbltrp extension, bit MXLEN-1 is set to 0 and the least-significant bits are set to the cause code corresponding to the exception that precipitated the double trap. | 如果原因是 M 模式内导致双重陷阱的异常（如 Smdbltrp 扩展所指定），则位 MXLEN-1 设置为 0，最低有效位设置为对应于引发双重陷阱的异常的原因代码。 |
| `norm:mnstatus_mnpp_op` | The `mnstatus` CSR holds a two-bit field, MNPP, which on entry to the RNMI trap handler holds the privilege mode of the interrupted context, encoded in the same manner as `mstatus`.MPP. | `mnstatus` CSR 保存一个两位字段 MNPP，在进入 RNMI 陷阱处理程序时保存被中断上下文的特权模式，编码方式与 `mstatus`.MPP 相同。 |
| `norm:mnstatus_mnpv_op` | If the H extension is also implemented, `mnstatus` also holds a one-bit field, MNPV, which on entry to the RNMI trap handler holds the virtualization mode of the interrupted context, encoded in the same manner as `mstatus`.MPV. | 如果还实现了 H 扩展，`mnstatus` 还保存一个一位字段 MNPV，在进入 RNMI 陷阱处理程序时保存被中断上下文的虚拟化模式，编码方式与 `mstatus`.MPV 相同。 |
| `norm:mnstatus_mnpelp_op` | If the Zicfilp extension is implemented, `mnstatus` also holds the MNPELP field, which on entry to the RNMI trap handler holds the previous `ELP` state. | 如果实现了 Zicfilp 扩展，`mnstatus` 还保存 MNPELP 字段，在进入 RNMI 陷阱处理程序时保存先前的 `ELP` 状态。 |
| `norm:mnstatus_mnpelp_update` | When an RNMI trap is taken, MNPELP is set to `ELP` and `ELP` is set to 0. | 当接受 RNMI 陷阱时，MNPELP 设置为 `ELP`，`ELP` 设置为 0。 |
| `norm:mnstatus_nmie_enable` | When NMIE=1, non-maskable interrupts are enabled. | 当 NMIE=1 时，启用不可屏蔽中断。 |
| `norm:mnstatus_nmie_disable` | When NMIE=0, _all_ interrupts are disabled. | 当 NMIE=0 时，_所有_ 中断被禁用。 |
| `norm:mnstatus_mprv_clear` | When NMIE=0, the hart behaves as though `mstatus`.MPRV were clear, regardless of the current setting of `mstatus`.MPRV. | 当 NMIE=0 时，hart 的行为就像 `mstatus`.MPRV 被清除一样，无论 `mstatus`.MPRV 的当前设置如何。 |
| `norm:mnstatus_nmie_reset` | Upon reset, NMIE contains the value 0. | 复位时，NMIE 包含值 0。 |
| `norm:mnstatus_nmie_set_clear` | Software can set NMIE to 1, but attempts to clear NMIE have no effect. | 软件可以将 NMIE 设置为 1，但尝试清除 NMIE 无效。 |
| `norm:mnstatus_wfi` | For the purposes of the WFI instruction, NMIE is a global interrupt enable, meaning that the setting of NMIE does not affect the operation of the WFI instruction. | 出于 WFI 指令的目的，NMIE 是全局中断使能，意味着 NMIE 的设置不影响 WFI 指令的操作。 |
| `norm:mnstatus_reserved` | The other bits in `mnstatus` are _reserved_; software should write zeros and hardware implementations should return zeros. | `mnstatus` 中的其他位是 _保留的_；软件应写入零，硬件实现应返回零。 |

### MNRET 指令

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mnret_mode` | MNRET is an M-mode-only instruction | MNRET 是仅限 M 模式的指令 |
| `norm:mnret_restore` | This instruction also sets `mnstatus`.NMIE. | 此指令还设置 `mnstatus`.NMIE。 |
| `norm:mnret_mprv` | If MNRET changes the privilege mode to a mode less privileged than M, it also sets `mstatus`.MPRV to 0. | 如果 MNRET 将特权模式更改为低于 M 的模式，它还将 `mstatus`.MPRV 设置为 0。 |
| `norm:mnret_zicfilp` | If the Zicfilp extension is implemented, then if the new privileged mode is __y__, MNRET sets `ELP` to the logical AND of __y__LPE (see <<FCFIACT>>) and `mnstatus`.MNPELP. | 如果实现了 Zicfilp 扩展，则如果新特权模式为 __y__，MNRET 将 `ELP` 设置为 __y__LPE（参见 <<FCFIACT>>）和 `mnstatus`.MNPELP 的逻辑与。 |

### RNMI 操作

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:rnmi_entry` | When an RNMI interrupt is detected, the interrupted PC is written to the `mnepc` CSR, | 当检测到 RNMI 中断时，被中断的 PC 被写入 `mnepc` CSR， |
| `norm:rnmi_entry_cause` | the type of RNMI to the `mncause` CSR, | RNMI 类型被写入 `mncause` CSR， |
| `norm:rnmi_entry_priv` | the privilege mode of the interrupted context to the `mnstatus` CSR. | 被中断上下文的特权模式被写入 `mnstatus` CSR。 |
| `norm:rnmi_entry_nmie_clear` | The `mnstatus`.NMIE bit is cleared, masking all interrupts. | `mnstatus`.NMIE 位被清除，屏蔽所有中断。 |
| `norm:rnmi_enter_mmode` | The hart then enters machine-mode and jumps to the RNMI trap handler address. | 然后 hart 进入机器模式并跳转到 RNMI 陷阱处理程序地址。 |
| `norm:rnmi_resume` | The RNMI handler can resume original execution using the new MNRET instruction, which restores the PC from `mnepc`, the privilege mode from `mnstatus`, and also sets `mnstatus`.NMIE, which re-enables interrupts. | RNMI 处理程序可以使用新的 MNRET 指令恢复原始执行，该指令从 `mnepc` 恢复 PC，从 `mnstatus` 恢复特权模式，并设置 `mnstatus`.NMIE，从而重新启用中断。 |
| `norm:rnmi_exception_nmie0` | If the hart encounters an exception while executing in M-mode with the `mnstatus`.NMIE bit clear, the actions taken are the same as if the exception had occurred while `mnstatus`.NMIE were set, except that the program counter is set to the RNMI exception trap handler address. | 如果 hart 在 M 模式下执行且 `mnstatus`.NMIE 位被清除时遇到异常，则采取的操作与 `mnstatus`.NMIE 被设置时发生异常时相同，只是程序计数器设置为 RNMI 异常陷阱处理程序地址。 |

## Smcdeleg/Ssccfg - 计数器委托

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:smcdeleg_ssccfg_tandem` | For a RISC-V hardware platform, Smcdeleg and Ssccfg must always be implemented in tandem. | 对于 RISC-V 硬件平台，Smcdeleg 和 Ssccfg 必须始终一起实现。 |
| `norm:smcdeleg_cde_en` | When the Smcdeleg/Ssccfg extensions are enabled (`menvcfg`.CDE=1), it further allows M-mode to delegate select counters to S-mode. | 当启用 Smcdeleg/Ssccfg 扩展（`menvcfg`.CDE=1）时，它进一步允许 M 模式将选定的计数器委托给 S 模式。 |
| `norm:ssccfg_illegal_sireg_cde0` | attempts to access any `sireg*` when `menvcfg`.CDE = 0; | 当 `menvcfg`.CDE = 0 时尝试访问任何 `sireg*`； |
| `norm:ssccfg_illegal_sireg3_6` | attempts to access `sireg3` or `sireg6`; | 尝试访问 `sireg3` 或 `sireg6`； |
| `norm:ssccfg_illegal_sireg4_5_xlen64` | attempts to access `sireg4` or `sireg5` when XLEN = 64; | 当 XLEN = 64 时尝试访问 `sireg4` 或 `sireg5`； |
| `norm:ssccfg_illegal_sireg_not_delegated` | attempts to access `sireg*` when `siselect` = 0x41, or when the counter selected by `siselect` is not delegated to S-mode (the corresponding bit in `mcounteren` = 0). | 当 `siselect` = 0x41 时，或当 `siselect` 选择的计数器未委托给 S 模式（`mcounteren` 中对应位 = 0）时，尝试访问 `sireg*`。 |
| `norm:ssccfg_missing_extension_illegal` | If any extension upon which the underlying state depends is not implemented, an attempt from M or S mode to access the given state through `sireg*` raises an illegal-instruction exception. | 如果底层状态依赖的任何扩展未实现，从 M 或 S 模式尝试通过 `sireg*` 访问给定状态会触发非法指令异常。 |
| `norm:ssccfg_hyp_vs_or_vu_access_vsireg_illegal` | If the hypervisor (H) extension is also implemented, then as specified by extensions Smcsrind/Sscsrind, a virtual-instruction exception is raised for attempts from VS-mode or VU-mode to directly access `vsiselect` or `vsireg*`, or attempts from VU-mode to access `siselect` or `sireg*`. | 如果还实现了 hypervisor (H) 扩展，则如 Smcsrind/Sscsrind 扩展所指定，从 VS 模式或 VU 模式尝试直接访问 `vsiselect` 或 `vsireg*`，或从 VU 模式尝试访问 `siselect` 或 `sireg*` 会触发虚拟指令异常。 |
| `norm:ssccfg_hyp_m_s_vsireg_illegal` | An attempt to access any `vsireg*` from M or S mode raises an illegal-instruction exception. | 从 M 或 S 模式尝试访问任何 `vsireg*` 会触发非法指令异常。 |
| `norm:ssccfg_hyp_vs_access_sireg_conditional` | An attempt from VS-mode to access any `sireg*` (really `vsireg*`) raises an illegal-instruction exception if `menvcfg`.CDE = 0, or a virtual-instruction exception if `menvcfg`.CDE = 1. | 如果 `menvcfg`.CDE = 0，从 VS 模式尝试访问任何 `sireg*`（实际上是 `vsireg*`）会触发非法指令异常；如果 `menvcfg`.CDE = 1，则触发虚拟指令异常。 |
| `norm:ssccfg_scountinhibit_exists` | Smcdeleg/Ssccfg defines a new `scountinhibit` register, a masked alias of `mcountinhibit`. | Smcdeleg/Ssccfg 定义了一个新的 `scountinhibit` 寄存器，它是 `mcountinhibit` 的掩码别名。 |
| `norm:ssccfg_scountinhibit_delegated_rw` | For counters delegated to S-mode, the associated `mcountinhibit` bits can be accessed via `scountinhibit`. | 对于委托给 S 模式的计数器，关联的 `mcountinhibit` 位可通过 `scountinhibit` 访问。 |
| `norm:ssccfg_scountinhibit_nondelegated_ro` | For counters not delegated to S-mode, the associated bits in `scountinhibit` are read-only zero. | 对于未委托给 S 模式的计数器，`scountinhibit` 中的关联位为只读零。 |
| `norm:ssccfg_illegal_scountinhibit_cde0` | When `menvcfg`.CDE=0, attempts to access `scountinhibit` raise an illegal-instruction exception. | 当 `menvcfg`.CDE=0 时，尝试访问 `scountinhibit` 会触发非法指令异常。 |
| `norm:ssccfg_illegal_scountinhibit_vs_vu` | When Supervisor Counter Delegation is enabled, attempts to access `scountinhibit` from VS-mode or VU-mode raise a virtual-instruction exception. | 当启用监管计数器委托时，从 VS 模式或 VU 模式尝试访问 `scountinhibit` 会触发虚拟指令异常。 |
| `norm:ssccfg_virtual_scountovf_vs_vu` | For implementations that support Smcdeleg/Ssccfg, Sscofpmf, and the H extension, when `menvcfg`.CDE=1, attempts to read `scountovf` from VS-mode or VU-mode raise a virtual-instruction exception. | 对于支持 Smcdeleg/Ssccfg、Sscofpmf 和 H 扩展的实现，当 `menvcfg`.CDE=1 时，从 VS 模式或 VU 模式尝试读取 `scountovf` 会触发虚拟指令异常。 |
| `norm:ssccfg_lcofi_mvip_mvien` | For implementations that support Smcdeleg, Sscofpmf, and Smaia, the local-counter-overflow interrupt (LCOFI) bit (bit 13) in each of CSRs `mvip` and `mvien` is implemented and writable. | 对于支持 Smcdeleg、Sscofpmf 和 Smaia 的实现，CSR `mvip` 和 `mvien` 中的每一个的本地计数器溢出中断（LCOFI）位（位 13）已实现且可写。 |
| `norm:ssccfg_lcofi_hvip_hvien` | For implementations that support Smcdeleg/Ssccfg, Sscofpmf, Smaia/Ssaia, and the H extension, the LCOFI bit (bit 13) in each of `hvip` and `hvien` is implemented and writable. | 对于支持 Smcdeleg/Ssccfg、Sscofpmf、Smaia/Ssaia 和 H 扩展的实现，`hvip` 和 `hvien` 中的每一个的 LCOFI 位（位 13）已实现且可写。 |

## Smdbltrp - 双重陷阱

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Smdbltrp_with_Smrnmi_op` | When the Smrnmi extension is implemented, it enables invocation of the RNMI handler on a double trap in M-mode to handle the critical error. | 当实现 Smrnmi 扩展时，它允许在 M 模式下发生双重陷阱时调用 RNMI 处理程序来处理关键错误。 |

## Smctr - 控制传输记录

### 基本操作

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sctrctl_op` | The `sctrctl` CSR enables/disables CTR and configures its behavior. | `sctrctl` CSR 启用/禁用 CTR 并配置其行为。 |
| `norm:sctrstatus_op` | The `sctrstatus` CSR provides status information about the CTR buffer. | `sctrstatus` CSR 提供 CTR 缓冲区的状态信息。 |
| `norm:sctrdepth_op` | The `sctrdepth` CSR indicates the depth of the CTR buffer. | `sctrdepth` CSR 指示 CTR 缓冲区的深度。 |

### 特权模式过滤

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:ctrctl_mode_filter` | The `__x__ctrctl`.__x__ bits enable recording of control transfers from privilege mode __x__. | `__x__ctrctl`.__x__ 位启用从特权模式 __x__ 的控制传输记录。 |
| `norm:ctrctl_excinh` | When `__x__ctrctl`.EXCINH is set, exceptions that trap to a more privileged mode are not recorded. | 当 `__x__ctrctl`.EXCINH 置位时，陷入更高特权模式的异常不被记录。 |
| `norm:ctrctl_intrinh` | When `__x__ctrctl`.INTRINH is set, interrupts that trap to a more privileged mode are not recorded. | 当 `__x__ctrctl`.INTRINH 置位时，陷入更高特权模式的中断不被记录。 |

### 外部陷阱

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:exttrap_def` | External traps are traps from a privilege mode enabled for CTR recording to a privilege mode that is not enabled for CTR recording. By default external traps are not recorded, but privileged software running in the target mode of the trap can opt-in to allowing CTR to record external traps into that mode. The `__x__ctrctl`.__x__TE bits allow M-mode, S-mode, and VS-mode to opt-in separately. | 外部陷阱是从启用 CTR 记录的特权模式到未启用 CTR 记录的特权模式的陷阱。默认情况下外部陷阱不被记录，但在陷阱目标模式运行的特权软件可以选择允许 CTR 记录到该模式的外部陷阱。`__x__ctrctl`.__x__TE 位允许 M 模式、S 模式和 VS 模式分别选择。 |
| `norm:exttrap_requirements` | External trap recording depends not only on the target mode, but on any intervening modes, which are modes that are more privileged than the source mode but less privileged than the target mode. Not only must the external trap enable bit for the target mode be set, but the external trap enable bit(s) for any intervening modes must also be set. | 外部陷阱记录不仅取决于目标模式，还取决于任何中间模式（比源模式特权更高但比目标模式特权更低的模式）。不仅必须设置目标模式的外部陷阱使能位，还必须设置任何中间模式的外部陷阱使能位。 |
| `norm:exttrap_us` | `sctrctl`.STE | `sctrctl`.STE |
| `norm:exttrap_um` | `mctrctl`.MTE, `sctrctl`.STE | `mctrctl`.MTE, `sctrctl`.STE |
| `norm:exttrap_sm` | `mctrctl`.MTE | `mctrctl`.MTE |
| `norm:exttrap_vuvs` | `vsctrctl`.STE | `vsctrctl`.STE |
| `norm:exttrap_vuhs` | `sctrctl`.STE, `vsctrctl`.STE | `sctrctl`.STE, `vsctrctl`.STE |
| `norm:exttrap_vum` | `mctrctl`.MTE, `sctrctl`.STE, `vsctrctl`.STE | `mctrctl`.MTE, `sctrctl`.STE, `vsctrctl`.STE |
| `norm:exttrap_vshs` | `sctrctl`.STE | `sctrctl`.STE |
| `norm:exttrap_vsm` | `mctrctl`.MTE, `sctrctl`.STE | `mctrctl`.MTE, `sctrctl`.STE |
| `norm:exttrap_ctrtarget0` | In records for external traps, the `ctrtarget`.PC is 0. | 在外部陷阱的记录中，`ctrtarget`.PC 为 0。 |
| `norm:exttrap_implreq` | If external trap recording is implemented, `mctrctl`.MTE and `sctrctl`.STE must be implemented, while `vsctrctl`.STE must be implemented if the H extension is implemented. | 如果实现了外部陷阱记录，则必须实现 `mctrctl`.MTE 和 `sctrctl`.STE，而如果实现了 H 扩展则必须实现 `vsctrctl`.STE。 |

### 传输类型过滤

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:ctr_ttf_default` | Default CTR behavior, when all transfer type filter bits (`__x__ctrctl`[47:32]) are unimplemented or 0, is to record all control transfers within enabled privileged modes. By setting transfer type filter bits, software can opt out of recording select transfer types, or opt into recording non-default operations. All transfer type filter bits are optional. | 默认 CTR 行为（当所有传输类型过滤位未实现或为 0 时）是记录启用的特权模式内的所有控制传输。通过设置传输类型过滤位，软件可以选择不记录选定的传输类型，或选择记录非默认操作。所有传输类型过滤位都是可选的。 |
| `norm:ctr_ttype0` | _Not used by CTR_ | _CTR 不使用_ |
| `norm:ctr_ttype1` | Exception | 异常 |
| `norm:ctr_ttype2` | Interrupt | 中断 |
| `norm:ctr_ttype3` | Trap return | 陷阱返回 |
| `norm:ctr_ttype4` | Not-taken branch | 未采取的分支 |
| `norm:ctr_ttype5` | Taken branch | 采取的分支 |
| `norm:ctr_ttype8` | Indirect call | 间接调用 |
| `norm:ctr_ttype9` | Direct call | 直接调用 |
| `norm:ctr_ttype10` | Indirect jump (without linkage) | 间接跳转（无链接） |
| `norm:ctr_ttype11` | Direct jump (without linkage) | 直接跳转（无链接） |
| `norm:ctr_ttype12` | Co-routine swap | 协程交换 |
| `norm:ctr_ttype13` | Function return | 函数返回 |
| `norm:ctr_ttype14` | Other indirect jump (with linkage) | 其他间接跳转（有链接） |
| `norm:ctr_ttype15` | Other direct jump (with linkage) | 其他直接跳转（有链接） |
| `norm:ctr_various_jump_enc` | Encodings 8 through 15 refer to various encodings of jump instructions. The types are distinguished as described below. | 编码 8 到 15 指的是跳转指令的各种编码。类型按如下所述区分。 |

### 周期计数

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:ctrdata_cc_supported` | The `ctrdata` register may optionally include a count of CPU cycles elapsed since the prior CTR record. The elapsed cycle count value is represented by the CC field, which has a 12-bit mantissa component (Cycle Count Mantissa, or CCM) and a 4-bit exponent component (Cycle Count Exponent, or CCE). | `ctrdata` 寄存器可选地包括自上一个 CTR 记录以来经过的 CPU 周期计数。经过的周期计数值由 CC 字段表示，该字段具有 12 位尾数分量（CCM）和 4 位指数分量（CCE）。 |
| `norm:ctr_ccounter_inc` | The elapsed cycle counter (CtrCycleCounter) increments at the same rate as the `mcycle` counter. Only cycles while CTR is active are counted, where active implies that the current privilege mode is enabled for recording and CTR is not frozen. | 经过的周期计数器（CtrCycleCounter）以与 `mcycle` 计数器相同的速率递增。仅计算 CTR 活动时的周期，其中活动意味着当前特权模式已启用记录且 CTR 未冻结。 |
| `norm:ctr_ccounter_reset` | The CtrCycleCounter is reset on writes to `__x__ctrctl`, and on execution of SCTRCLR, to ensure that any accumulated cycle counts do not persist across a context switch. | CtrCycleCounter 在写入 `__x__ctrctl` 和执行 SCTRCLR 时重置，以确保任何累积的周期计数不会在上下文切换中持续存在。 |
| `norm:ctr_ccounter_impl` | An implementation that supports cycle counting must implement CCV and all CCM bits, but may implement 0..4 exponent bits in CCE. Unimplemented CCE bits are read-only 0. | 支持周期计数的实现必须实现 CCV 和所有 CCM 位，但可以在 CCE 中实现 0..4 个指数位。未实现的 CCE 位为只读 0。 |
| `norm:ccsize0` | 4095 | 4095 |
| `norm:ccsize1` | 8191 | 8191 |
| `norm:ccsize2` | 32764 | 32764 |
| `norm:ccsize3` | 524224 | 524224 |
| `norm:ccsize4` | 134201344 | 134201344 |
| `norm:ctr_ccounter_sat` | The CC value saturates when all implemented bits in CCM and CCE are 1. | 当 CCM 和 CCE 中所有已实现的位都为 1 时，CC 值饱和。 |
| `norm:ctr_ccounter_ccv` | The CC value is valid only when the Cycle Count Valid (CCV) bit is set. If CCV=0, the CC value might not hold the correct count of elapsed active cycles since the last recorded transfer. | CC 值仅在周期计数有效（CCV）位置位时有效。如果 CCV=0，CC 值可能不保存自上次记录传输以来经过的活动周期的正确计数。 |

### RAS 模拟模式

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:ctrctl_rasemu_op` | When the optional `__x__ctrctl`.RASEMU bit is implemented and set to 1, transfer recording behavior is altered to emulate the behavior of a return-address stack (RAS). | 当可选的 `__x__ctrctl`.RASEMU 位已实现并设置为 1 时，传输记录行为被更改以模拟返回地址栈（RAS）的行为。 |

### 冻结

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sctrstatus_frozen_set` | When `sctrctl`.LCOFIFRZ=1 and a local-counter-overflow interrupt (LCOFI) traps (as a result of an HPM counter overflow) to M-mode or to S-mode, `sctrstatus`.FROZEN is set by hardware. This inhibits CTR recording until software clears FROZEN. The LCOFI trap itself is not recorded. | 当 `sctrctl`.LCOFIFRZ=1 且本地计数器溢出中断（LCOFI）陷入（由于 HPM 计数器溢出）到 M 模式或 S 模式时，`sctrstatus`.FROZEN 由硬件设置。这会抑制 CTR 记录直到软件清除 FROZEN。LCOFI 陷阱本身不被记录。 |
| `norm:ctr_freeze_bp` | Similarly, on a breakpoint exception that traps to M-mode or S-mode with `sctrctl`.BPFRZ=1, FROZEN is set by hardware. The breakpoint exception itself is not recorded. | 类似地，当 `sctrctl`.BPFRZ=1 时，陷入 M 模式或 S 模式的断点异常会由硬件设置 FROZEN。断点异常本身不被记录。 |
| `norm:ctr_freeze_vs` | If the H extension is implemented, freeze behavior for LCOFIs and breakpoint exceptions that trap to VS-mode is determined by the LCOFIFRZ and BPFRZ values, respectively, in `vsctrctl`. This includes virtual LCOFIs pended by a hypervisor. | 如果实现了 H 扩展，则陷入 VS 模式的 LCOFI 和断点异常的冻结行为分别由 `vsctrctl` 中的 LCOFIFRZ 和 BPFRZ 值决定。这包括由 hypervisor 挂起的虚拟 LCOFI。 |

### 自定义扩展

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:ctr_custom_bits` | Any custom CTR extension must be associated with a non-zero value within the designated custom bits in `__x__ctrctl`. When the custom bits hold a non-zero value that enables a custom extension, the extension may alter standard CTR behavior, and may define new custom status fields within `sctrstatus` or the CTR Entry Registers. All custom status fields, and standard status fields whose behavior is altered by the custom extension, must revert to standard behavior when the custom bits hold zero. | 任何自定义 CTR 扩展必须与 `__x__ctrctl` 中指定的自定义位内的非零值关联。当自定义位持有启用自定义扩展的非零值时，该扩展可以更改标准 CTR 行为，并可以在 `sctrstatus` 或 CTR 条目寄存器中定义新的自定义状态字段。所有自定义状态字段以及行为被自定义扩展更改的标准状态字段必须在自定义位持有零时恢复为标准行为。 |

## Zicfilp - 前向边缘控制流完整性

### 陷阱时的 ELP 状态保存

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:Zicfilp_forward_traps` | A trap may need to be delivered to the same or to a higher privilege mode upon completion of jalr/c.jalr/c.jr, but before the instruction at the target of indirect call/jump was decoded | 可能需要在 jalr/c.jalr/c.jr 完成后但在间接调用/跳转目标处的指令被解码之前，将陷阱传递到相同或更高特权模式 |
| `norm:Zicfilp_forward_trap_async_interrupt` | Asynchronous interrupts. | 异步中断。 |
| `norm:Zicfilp_forward_trap_async_exception` | Synchronous exceptions with priority higher than that of a software-check exception with `__x__tval` set to "landing pad fault (code=2)". | 优先级高于 `__x__tval` 设置为"着陆垫故障（代码=2）"的软件检查异常的同步异常。 |
| `norm:Zicfilp_exception_priority` | The software-check exception caused by ext:zicfilp[] has higher priority than an illegal-instruction exception but lower priority than instruction access-fault. | 由 zicfilp 引起的软件检查异常的优先级高于非法指令异常但低于指令访问错误。 |
| `norm:lpad_sw_exception` | The software-check exception due to the instruction not being an lpad instruction when `ELP` is `LP_EXPECTED` or a software-check exception caused by the lpad instruction itself leads to a trap being delivered to the same or to a higher privilege mode. | 当 `ELP` 为 `LP_EXPECTED` 时由于指令不是 lpad 指令导致的软件检查异常，或由 lpad 指令本身引起的软件检查异常，会导致陷阱被传递到相同或更高特权模式。 |
| `norm:cfi_mstatus_mpelp_op` | To store the previous `ELP` state on trap delivery to M-mode, an mpelp bit is provided in the mstatus CSR. | 为了在陷阱传递到 M 模式时存储先前的 `ELP` 状态，在 mstatus CSR 中提供了 mpelp 位。 |
| `norm:cfi_mstatus_spelp_op` | To store the previous `ELP` state on trap delivery to S/HS-mode, an spelp bit is provided in the mstatus CSR. | 为了在陷阱传递到 S/HS 模式时存储先前的 `ELP` 状态，在 mstatus CSR 中提供了 spelp 位。 |
| `norm:cfi_sstatus_spelp_op` | The spelp bit in `mstatus` can be accessed through the sstatus CSR. | `mstatus` 中的 spelp 位可通过 sstatus CSR 访问。 |
| `norm:vsstatus_spelp_op2` | To store the previous `ELP` state on traps to VS-mode, a spelp bit is defined in the vsstatus (VS-modes version of sstatus). | 为了在陷阱到 VS 模式时存储先前的 `ELP` 状态，在 vsstatus（sstatus 的 VS 模式版本）中定义了 spelp 位。 |
| `norm:dcsr_pelp_op` | To store the previous `ELP` state on transition to Debug Mode, a pelp bit is defined in the dcsr register. | 为了在转换到调试模式时存储先前的 `ELP` 状态，在 dcsr 寄存器中定义了 pelp 位。 |
| `norm:Zicfilp_pelp_trap` | When a trap is taken into privilege mode `x`, the xpelp is set to `ELP` and `ELP` is set to `NO_LP_EXPECTED`. | 当陷阱进入特权模式 `x` 时，xpelp 设置为 `ELP`，`ELP` 设置为 `NO_LP_EXPECTED`。 |
| `norm:Zicfilp_pelp_trap_return` | An mret or sret instruction is used to return from a trap in M-mode or S-mode, respectively. When executing an xret instruction, if the new privilege mode is `y`, then `ELP` is set to the value of xpelp if `__y__LPE` is 1; otherwise, it is set to `NO_LP_EXPECTED`; xpelp is set to `NO_LP_EXPECTED`. | mret 或 sret 指令分别用于从 M 模式或 S 模式的陷阱返回。当执行 xret 指令时，如果新特权模式为 `y`，则如果 `__y__LPE` 为 1，`ELP` 设置为 xpelp 的值；否则设置为 `NO_LP_EXPECTED`；xpelp 设置为 `NO_LP_EXPECTED`。 |
| `norm:Zicfilp_pelp_debug_mode` | Upon entry into Debug Mode, the pelp bit in dcsr is updated with the `ELP` at the privilege level the hart was previously in, and the `ELP` is set to `NO_LP_EXPECTED`. When a hart resumes from Debug Mode, if the new privilege mode is `y`, then `ELP` is set to the value of pelp if `__y__LPE` is 1; otherwise, it is set to `NO_LP_EXPECTED`. | 进入调试模式时，dcsr 中的 pelp 位更新为 hart 之前所在特权级别的 `ELP`，`ELP` 设置为 `NO_LP_EXPECTED`。当 hart 从调试模式恢复时，如果新特权模式为 `y`，则如果 `__y__LPE` 为 1，`ELP` 设置为 pelp 的值；否则设置为 `NO_LP_EXPECTED`。 |

## Zicfiss - 后向边缘控制流完整性（影子栈）

### SSP CSR 访问控制

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:zicfiss_ssp_csr` | Attempts to access the ssp CSR may result in either an illegal-instruction exception or a virtual-instruction exception, contingent upon the state of the envcfg[sse] fields. | 尝试访问 ssp CSR 可能导致非法指令异常或虚拟指令异常，取决于 envcfg[sse] 字段的状态。 |
| `norm:zicfiss_m_menvcfg_sse` | If the privilege mode is less than M and menvcfg[sse] is 0, an illegal-instruction exception is raised. | 如果特权模式低于 M 且 menvcfg[sse] 为 0，则触发非法指令异常。 |
| `norm:zicfiss_u_senvcfg_sse` | Otherwise, if in U-mode and senvcfg[sse] is 0, an illegal-instruction exception is raised. | 否则，如果在 U 模式且 senvcfg[sse] 为 0，则触发非法指令异常。 |
| `norm:zicfiss_vs_henvcfg_sse` | Otherwise, if in VS-mode and henvcfg[sse] is 0, a virtual-instruction exception is raised. | 否则，如果在 VS 模式且 henvcfg[sse] 为 0，则触发虚拟指令异常。 |
| `norm:zicfiss_vu_henvcfg_senvcfg_sse` | Otherwise, if in VU-mode and either henvcfg[sse] or senvcfg[sse] is 0, a virtual-instruction exception is raised. | 否则，如果在 VU 模式且 henvcfg[sse] 或 senvcfg[sse] 为 0，则触发虚拟指令异常。 |
| `norm:zicfiss_sse_access` | Otherwise, the access is allowed. | 否则，允许访问。 |
| `norm:zicfiss_smode_xsse` | When S-mode is not implemented, then `xSSE` is 0 at both M and U privilege modes. | 当未实现 S 模式时，M 和 U 特权模式的 `xSSE` 均为 0。 |

### 影子栈内存保护

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:ss_page_enc` | The encoding `R=0`, `W=1`, and `X=0`, is defined to represent an SS page. | 编码 `R=0`、`W=1` 和 `X=0` 被定义为表示 SS 页面。 |
| `norm:ssmp_menvcfg_sse` | When menvcfg[sse]=0, this encoding remains reserved. | 当 menvcfg[sse]=0 时，此编码保持保留。 |
| `norm:ssmp_henvcfg_sse` | Similarly, when `V=1` and henvcfg[sse]=0, this encoding remains reserved at `VS` and `VU` levels. | 类似地，当 `V=1` 且 henvcfg[sse]=0 时，此编码在 `VS` 和 `VU` 级别保持保留。 |
| `norm:satp_mode_bare` | If satp[mode] (or vsatp[mode] when `V=1`) is set to `Bare` and the effective privilege mode is less than M, shadow stack instructions raise a store/AMO access-fault exception. | 如果 satp[mode]（或当 `V=1` 时的 vsatp[mode]）设置为 `Bare` 且有效特权模式低于 M，影子栈指令会触发 store/AMO 访问错误异常。 |
| `norm:ssmp_ssamoswap` | When the effective privilege mode is M, memory access by an ssamoswap.w or ssamoswap.d instruction results in a store/AMO access-fault exception. | 当有效特权模式为 M 时，ssamoswap.w 或 ssamoswap.d 指令的内存访问会导致 store/AMO 访问错误异常。 |
| `norm:ssmp_ss_page_access_fault` | Memory mapped as an SS page cannot be written to by instructions other than ssamoswap.w, ssamoswap.d, sspush, and c.sspush. Attempts will raise a store/AMO access-fault exception. | 映射为 SS 页面的内存不能由 ssamoswap.w、ssamoswap.d、sspush 和 c.sspush 之外的指令写入。尝试会触发 store/AMO 访问错误异常。 |
| `norm:ssmp_ss_cache_block_access_fault` | Access to a SS page using _cache-block operation_ (cbo.*) instructions is not permitted. Such accesses will raise a store/AMO access-fault exception. | 不允许使用 _缓存块操作_（cbo.*）指令访问 SS 页面。此类访问会触发 store/AMO 访问错误异常。 |
| `norm:ssmp_ss_implicit_access_fault` | Implicit accesses, including instruction fetches to an SS page, are not permitted. Such accesses will raise an access-fault exception appropriate to the access type. | 不允许隐式访问，包括对 SS 页面的指令获取。此类访问会触发适合访问类型的访问错误异常。 |
| `norm:ssmp_ss_load` | However, the shadow stack is readable by all instructions that only load from memory. | 然而，影子栈可被所有仅从内存加载的指令读取。 |
| `norm:ss_fault_exception_code` | If a shadow stack (SS) instruction raises an access-fault, page-fault, or guest-page-fault exception that is supposed to indicate the original instruction type (load or store/AMO), then the reported exception cause is respectively a store/AMO access fault (code 7), a store/AMO page fault (code 15), or a store/AMO guest-page fault (code 23). | 如果影子栈（SS）指令触发应指示原始指令类型（加载或存储/AMO）的访问错误、页错误或客户页错误异常，则报告的异常原因分别是 store/AMO 访问错误（代码 7）、store/AMO 页错误（代码 15）或 store/AMO 客户页错误（代码 23）。 |
| `norm:ssmp_ss_page_illegeal_access` | Should a shadow stack instruction access a page that is not designated as a shadow stack page and is not marked as read-only (`pte.xwr=001`), a store/AMO access-fault exception will be invoked. | 如果影子栈指令访问未指定为影子栈页面且未标记为只读（`pte.xwr=001`）的页面，将调用 store/AMO 访问错误异常。 |
| `norm:ssmp_ss_read_only_page` | Conversely, if the page being accessed by a shadow stack instruction is a read-only page, a store/AMO page-fault exception will be triggered. | 相反，如果影子栈指令访问的页面是只读页面，将触发 store/AMO 页错误异常。 |
| `norm:ssp_xlen_aligned` | If the virtual address in ssp is not `XLEN` aligned, then the sspush, c.sspush, sspopchk and c.sspopchk instructions cause a store/AMO access-fault exception. | 如果 ssp 中的虚拟地址未按 `XLEN` 对齐，则 sspush、c.sspush、sspopchk 和 c.sspopchk 指令会导致 store/AMO 访问错误异常。 |
| `norm:ssmp_ss_idempotent_memory` | If the memory referenced by sspush, c.sspush, sspopchk, c.sspopchk, ssamoswap.w and ssamoswap.d instructions is not idempotent, then the instructions cause a store/AMO access-fault exception. | 如果 sspush、c.sspush、sspopchk、c.sspopchk、ssamoswap.w 和 ssamoswap.d 指令引用的内存不是幂等的，则这些指令会导致 store/AMO 访问错误异常。 |
| `norm:active_g_stage_pte` | When G-stage page tables are active, the shadow stack instructions that access memory require the G-stage page table to have read-write permission for the accessed memory; else a store/AMO guest-page-fault exception is raised. | 当 G 阶段页表处于活动状态时，访问内存的影子栈指令要求 G 阶段页表对访问的内存具有读写权限；否则会触发 store/AMO 客户页错误异常。 |

## Zpm - 指针掩码扩展

### 基本概念

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pm_ignore_upper_bits` | RISC-V Pointer Masking (PM) is a feature that, when enabled, causes the CPU to ignore the upper bits of the effective address. | RISC-V 指针掩码（PM）是一项功能，启用后会导致 CPU 忽略有效地址的高位。 |
| `norm:pm_all_priv_modes` | Such tools can be applied in all privilege modes (U, S, and M). | 此类工具可应用于所有特权模式（U、S 和 M）。 |
| `norm:pm_tag_check_impl` | The tag checks themselves can be implemented in software or hardware. | 标签检查本身可以在软件或硬件中实现。 |

### Ignore 转换

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pm_ignore_va` | The ignore transformation differs depending on whether it applies to a virtual or physical address. For virtual addresses, it replaces the upper PMLEN bits with the sign extension of the PMLEN+1st bit. | ignore 转换根据应用于虚拟地址还是物理地址而有所不同。对于虚拟地址，它将高 PMLEN 位替换为 PMLEN+1 位的符号扩展。 |
| `norm:pm_ignore_pa` | When applied to a physical address, including guest-physical addresses (i.e., all cases except when the active satp register's MODE field != Bare), the ignore transformation replaces the upper PMLEN bits with 0. | 当应用于物理地址（包括客户物理地址，即除活动 satp 寄存器的 MODE 字段 != Bare 之外的所有情况）时，ignore 转换将高 PMLEN 位替换为 0。 |
| `norm:pm_apply_explicit` | When pointer masking is enabled, the ignore transformation will be applied to every explicit memory access (e.g., loads/stores, atomics operations, and floating point loads/stores). | 当启用指针掩码时，ignore 转换将应用于每个显式内存访问（例如加载/存储、原子操作和浮点加载/存储）。 |
| `norm:pm_not_apply_implicit` | The transformation *does not* apply to implicit accesses such as page-table walks or instruction fetches. | 该转换 *不* 适用于隐式访问，如页表遍历或指令获取。 |
| `norm:pm_deterministic_effect` | Pointer masking with the same value of PMLEN always has the same effect for the same type of address (virtual or physical). | 具有相同 PMLEN 值的指针掩码对相同类型的地址（虚拟或物理）始终具有相同的效果。 |

### PMLEN 值

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pmlen_supported_values` | The current standard only supports PMLEN=XLEN-48 (i.e., PMLEN=16 in RV64) and PMLEN=XLEN-57 (i.e., PMLEN=7 in RV64). | 当前标准仅支持 PMLEN=XLEN-48（即 RV64 中 PMLEN=16）和 PMLEN=XLEN-57（即 RV64 中 PMLEN=7）。 |
| `norm:pmlen_future_reserved` | A setting has been reserved to potentially support other values of PMLEN in future standards. | 已保留一个设置以在未来标准中潜在支持其他 PMLEN 值。 |

### 特权模式控制

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pm_per_mode_control` | Pointer masking is controlled separately for different privilege modes. | 指针掩码针对不同特权模式分别控制。 |
| `norm:pm_auto_apply_active_mode` | Different privilege modes may have different pointer masking settings active simultaneously and the hardware will automatically apply the pointer masking settings of the currently active privilege mode. | 不同特权模式可以同时激活不同的指针掩码设置，硬件将自动应用当前活动特权模式的指针掩码设置。 |
| `norm:pm_config_next_higher` | A privilege mode's pointer masking setting is configured by bits in configuration registers of the next-higher privilege mode. | 特权模式的指针掩码设置由下一个更高特权模式的配置寄存器中的位配置。 |
| `norm:pm_mode_only_dependency` | the pointer masking setting that is applied only depends on the active privilege mode, not on the address that is being masked. | 应用的指针掩码设置仅取决于活动特权模式，而不取决于被掩码的地址。 |

### 内存访问

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pm_mprv_spvp` | MPRV and SPVP affect pointer masking as well, causing the pointer masking settings of the effective privilege mode to be applied. | MPRV 和 SPVP 也影响指针掩码，导致应用有效特权模式的指针掩码设置。 |
| `norm:pm_mxr_exception` | When MXR is in effect at the effective privilege mode where explicit memory access is performed, pointer masking does not apply. | 当 MXR 在执行显式内存访问的有效特权模式下生效时，指针掩码不适用。 |
| `norm:pm_cpu_only` | Pointer masking only applies to accesses generated by instructions on the CPU (including CPU extensions such as an FPU). E.g., it does not apply to accesses generated by page-table walks, the IOMMU, or devices. | 指针掩码仅适用于 CPU 上指令生成的访问（包括 CPU 扩展如 FPU）。例如，它不适用于页表遍历、IOMMU 或设备生成的访问。 |
| `norm:pm_misaligned_equivalence` | Misaligned accesses are supported, subject to the same limitations as in the absence of pointer masking. The behavior is identical to applying the pointer masking transformation to every constituent aligned memory access. | 支持非对齐访问，受与没有指针掩码时相同的限制。行为等同于将指针掩码转换应用于每个组成的对齐内存访问。 |
| `norm:pm_no_csr_sw` | No pointer masking operations are applied when software reads/writes to CSRs, including those meant to hold addresses. | 当软件读取/写入 CSR（包括用于保存地址的 CSR）时，不应用指针掩码操作。 |
| `norm:pm_warl_unaffected` | The implemented WARL width of CSRs is unaffected by pointer masking. | CSR 的已实现 WARL 宽度不受指针掩码影响。 |
| `norm:pm_csr_hw_apply` | pointer masking, when applicable, **is applied** for hardware writes to a CSR (e.g., when the hardware writes the transformed address to `stval` when taking an exception). | 指针掩码在适用时 **应用于** 硬件对 CSR 的写入（例如，当硬件在接受异常时将转换后的地址写入 `stval`）。 |
| `norm:pm_debug_trigger` | Pointer masking is also applied, when applicable, to the memory access address when matching address triggers in debug. | 指针掩码在适用时也应用于调试中匹配地址触发器时的内存访问地址。 |
| `norm:pm_no_trap_vector_mask` | on trap delivery (e.g., due to an exception or interrupt), pointer masking **will not be applied** to the address of the trap handler. | 在陷阱传递时（例如由于异常或中断），指针掩码 **不会应用于** 陷阱处理程序的地址。 |

### 扩展族

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:pm_family_extensions` | Pointer masking refers to a number of separate extensions, all of which are privileged. | 指针掩码指的是多个单独的扩展，所有这些都是特权的。 |
| `norm:ssnpm_definition` | A supervisor-level extension that provides pointer masking for the next lower privilege mode (U-mode), and for VS- and VU-modes if the H extension is present. | 一个监管级扩展，为下一个更低特权模式（U 模式）提供指针掩码，如果存在 H 扩展则为 VS 和 VU 模式提供。 |
| `norm:smnpm_definition` | A machine-level extension that provides pointer masking for the next lower privilege mode (S/HS if S-mode is implemented, or U-mode otherwise). | 一个机器级扩展，为下一个更低特权模式（如果实现了 S 模式则为 S/HS，否则为 U 模式）提供指针掩码。 |
| `norm:smmpm_definition` | A machine-level extension that provides pointer masking for M-mode. | 一个为 M 模式提供指针掩码的机器级扩展。 |
| `norm:sspm_definition` | An extension that indicates that there is pointer-masking support available in supervisor mode, with some facility provided in the supervisor execution environment to control pointer masking. | 一个指示监管模式下有指针掩码支持可用的扩展，监管执行环境中提供了某些设施来控制指针掩码。 |
| `norm:supm_definition` | An extension that indicates that there is pointer-masking support available in user mode, with some facility provided in the application execution environment to control pointer masking. | 一个指示用户模式下有指针掩码支持可用的扩展，应用程序执行环境中提供了某些设施来控制指针掩码。 |
| `norm:pm_rv64_only` | Pointer masking only applies to RV64. | 指针掩码仅适用于 RV64。 |
| `norm:pm_rv32_illegal` | In RV32, trying to enable pointer masking will result in an illegal WARL write and not update the pointer masking configuration bits. | 在 RV32 中，尝试启用指针掩码会导致非法 WARL 写入且不更新指针掩码配置位。 |
| `norm:pm_uxl_clear` | Setting UXL/SXL/MXL to 1 will clear the corresponding pointer masking configuration bits. | 将 UXL/SXL/MXL 设置为 1 将清除相应的指针掩码配置位。 |
| `norm:pmlen_mode_depend` | the supported values of PMLEN may depend on the effective privilege mode. The current standard only defines PMLEN=XLEN-48 and PMLEN=XLEN-57. | PMLEN 的支持值可能取决于有效特权模式。当前标准仅定义 PMLEN=XLEN-48 和 PMLEN=XLEN-57。 |
| `norm:pmlen_illegal_warl` | Trying to enable pointer masking in an unsupported scenario represents an illegal write to the corresponding pointer masking enable bit and follows WARL semantics. | 在不支持的场景中尝试启用指针掩码表示对相应指针掩码使能位的非法写入，并遵循 WARL 语义。 |
