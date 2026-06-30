**中文 | [English](../testplan_en/Hypervisor_2_stage_test_plan_en.md)**

# RISC-V 两阶段地址翻译测试计划

本文档定义 RISC-V Hypervisor 扩展中 **两阶段地址翻译**（VS-stage + G-stage）的测试计划。覆盖 V=1 时 VS-stage（`vsatp` 控制）与 G-stage（`hgatp` 控制）的联合行为：完整 VA → GPA → SPA 链路、隐式访问引发的 G-stage fault、权限交叉、TLB fence、HLV/HLVX/HSV 指令以及 M-mode 下 `mstatus.MPRV+MPV` 触发的两阶段访问。

> **范围说明**：本计划覆盖以下五类场景：
> 1. V=1 且 hgatp=Bare：仅 VS-stage（验证 vsatp 与 satp 行为一致）
> 2. V=1 且 vsatp=Bare（GVA=GPA）：仅 G-stage（已在配套文档 `docs/gstage_translation_test_plan.md` 中详尽覆盖，本文档仅作交叉引用）
> 3. V=1 且两阶段同时启用：VS-stage + G-stage 完整链路
> 4. HLV/HLVX/HSV：HS-mode（或 U-mode + HU=1）下显式触发两阶段
> 5. `mstatus.MPRV=1 + MPV=1`：M-mode 显式触发两阶段
>
> 当前仓库为 RV64，仅覆盖 Sv39/Sv48/Sv57 与 Sv39x4/Sv48x4/Sv57x4。

> **配套文档**：本计划与 `docs/gstage_translation_test_plan.md` 配套，共享同一套 hypervisor 测试框架（`docs/hypervisor_framework.md`）以及 `sv39x4/`、`sv48x4/`、`sv57x4/` 目录结构。

---

## 适用范围与组合矩阵

两阶段翻译涉及 VS-stage（4 种 MODE）× G-stage（4 种 MODE）= 16 种组合。本计划按下表分配测试职责：

| VS-stage \ G-stage | Bare | Sv39x4 | Sv48x4 | Sv57x4 |
|-------------------|------|--------|--------|--------|
| **Bare** | ❌ V=0 等价（不属于本计划） | (Group A) → 见 `gstage_translation_test_plan.md` | (Group A) → 见同文档 | (Group A) → 见同文档 |
| **Sv39** | Group B | **Group C 主组合** | Group C′ | Group C′ |
| **Sv48** | Group B | Group C′ | **Group C 主组合** | Group C′ |
| **Sv57** | Group B | Group C′ | Group C′ | **Group C 主组合** |

- **Group A**：仅 G-stage（VS-stage Bare），由 `gstage_translation_test_plan.md` 独立覆盖
- **Group B**：仅 VS-stage（hgatp=Bare），验证 V=1 下 vsatp 与普通 satp 的行为对等
- **Group C 主组合**：3 个同位宽匹配组合（Sv39+Sv39x4 / Sv48+Sv48x4 / Sv57+Sv57x4），覆盖核心两阶段行为
- **Group C′ 异位宽组合**：6 个异位宽组合，每对至少 1~2 个 sanity 用例

---

## 规范引用

- `SPEC/hypervisor.adoc` — "H" Extension for Hypervisor Support, Version 1.0
  - Two-Stage Address Translation
  - Guest Physical Address Translation
  - Virtual Supervisor Address Translation and Protection (`vsatp`) Register
  - Hypervisor Memory-Management Fence Instructions (HFENCE.VVMA / HFENCE.GVMA)
  - Hypervisor Virtual-Machine Load and Store Instructions (HLV / HLVX / HSV)
  - Memory-Management Fences (with V=0/V=1 SFENCE.VMA semantics)
  - Machine Status (`mstatus` and `mstatush`) Registers — MPV / MPRV 表
  - Trap Cause Codes
  - Hypervisor Trap Value (`htval`) Register / Hypervisor Trap Instruction (`htinst`) Register
  - Transformed Instruction or Pseudoinstruction for `mtinst` or `htinst`

## 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:H_vm_twostage` | Whenever the current virtualization mode V is 1, two-stage address translation and protection is in effect. For any virtual memory access, the original virtual address is converted in the first stage by VS-level address translation, as controlled by `vsatp`, into a guest physical address. The guest physical address is then converted in the second stage by guest physical address translation, as controlled by `hgatp`, into a supervisor physical address. | 当 V=1 时，两阶段地址翻译和保护生效。虚拟地址先由 `vsatp` 控制的 VS 级翻译转为客户物理地址，再由 `hgatp` 控制的 G 阶段翻译转为 supervisor 物理地址。 |
| `norm:H_vm_gstagetrans` | When V=1, memory accesses that would normally bypass address translation are subject to G-stage address translation alone. This includes memory accesses made in support of VS-stage address translation, such as reads and writes of VS-level page tables. | V=1 时，通常绕过地址翻译的内存访问仅受 G 阶段地址翻译约束，包括支持 VS 阶段地址翻译的访问（如读写 VS 级页表）。 |
| `norm:H_vm_gpatrans` | The conversion of an Sv32x4, Sv39x4, Sv48x4, or Sv57x4 guest physical address uses the same algorithm as Sv32, Sv39, Sv48, or Sv57, except: `hgatp` substitutes for `satp`; the effective privilege mode must be VS-mode or VU-mode; the current privilege mode is always taken to be U-mode when checking the U bit; and guest-page-fault exceptions are raised instead of regular page-fault exceptions. | Sv32x4/Sv39x4/Sv48x4/Sv57x4 客户物理地址转换使用相同算法，但 `hgatp` 替代 `satp`；有效特权模式须为 VS/VU；检查 U 位时始终视为 U 模式；引发客户页错误而非普通页错误。 |
| `norm:H_vm_gpapriv` | For G-stage address translation, all memory accesses are considered to be user-level accesses. Access type permissions are checked during G-stage translation the same as for VS-stage. For memory accesses supporting VS-stage translation, permissions and A/D bit needs are checked as though for an implicit load or store, not for the original access type. However, any exception is always reported for the original access type. | G 阶段地址翻译中，所有内存访问视为用户级访问。访问权限检查方式与 VS 阶段相同。支持 VS 阶段翻译的访问按隐式 load/store 检查权限，但异常始终报告为原始访问类型。 |
| `norm:vsstatus_mxr_vm` | The `vsstatus` field MXR, which makes execute-only pages readable by explicit loads, only overrides VS-stage page protection. Setting MXR at VS-level does not override guest-physical page protections. | `vsstatus` 的 MXR 字段仅覆盖 VS 阶段页保护。VS 级设置 MXR 不覆盖客户物理页保护。 |
| `norm:sstatus_mxr_vm` | Setting MXR at HS-level, however, overrides both VS-stage and G-stage execute-only permissions. | HS 级设置 MXR 覆盖 VS 阶段和 G 阶段的仅执行权限。 |
| `norm:vsatp_sz_acc_op` | The `vsatp` register is a VSXLEN-bit read/write register that is VS-mode's version of supervisor register `satp`. When V=1, `vsatp` substitutes for the usual `satp`. `vsatp` controls VS-stage address translation, the first stage of two-stage translation for guest virtual addresses. | `vsatp` 是 VSXLEN 位读写寄存器，VS 模式版本的 `satp`。V=1 时替代 `satp`。`vsatp` 控制 VS 阶段地址翻译。 |
| `norm:vsatp_v0` | When V=0, `vsatp` does not directly affect the behavior of the machine, unless a virtual-machine load/store (HLV, HLVX, or HSV) or the MPRV feature in the `mstatus` register is used to execute a load or store as though V=1. | V=0 时 `vsatp` 不直接影响机器行为，除非使用虚拟机 load/store 或 MPRV 功能以 V=1 方式执行。 |
| `norm:vsatp_mode_unsupported_v0` | When V=0, a write to `vsatp` with an unsupported MODE value is either ignored as it is for `satp`, or the fields of `vsatp` are treated as WARL in the normal way. | V=0 时，使用不支持的 MODE 值写入 `vsatp`，要么像 `satp` 一样被忽略，要么按常规 WARL 方式处理。 |
| `norm:vsatp_mode_unsupported_v1` | However, when V=1, a write to `satp` with an unsupported MODE value is ignored and no write to `vsatp` is effected. | V=1 时，使用不支持的 MODE 值写入 `satp` 被忽略，不会对 `vsatp` 生效。 |
| `norm:vs_stage_speculative_a_bit` | When `vsatp` is active, VS-stage page-table entries' A bits must not be set as a result of speculative execution, unless the effective privilege mode is VS or VU. | 当 `vsatp` 活跃时，VS 阶段页表项的 A 位不得因推测执行而被设置，除非有效特权模式为 VS 或 VU。 |
| `norm:hlsv_mode` | The hypervisor virtual-machine load and store instructions are valid only in M-mode or HS-mode, or in U-mode when `hstatus`.HU=1. | hypervisor 虚拟机 load/store 指令仅在 M 模式、HS 模式或 `hstatus`.HU=1 的 U 模式下有效。 |
| `norm:hlsv_priv` | Each instruction performs an explicit memory access with an effective privilege mode of VS or VU. The effective privilege mode is VU when `hstatus`.SPVP=0, and VS when `hstatus`.SPVP=1. | 每条指令以 VS 或 VU 有效特权模式执行显式内存访问。SPVP=0 时为 VU，SPVP=1 时为 VS。 |
| `norm:hlsv_trans` | As usual for VS-mode and VU-mode, two-stage address translation is applied, and the HS-level `sstatus`.SUM is ignored. | 与 VS/VU 模式一样，应用两阶段地址翻译，HS 级 `sstatus`.SUM 被忽略。 |
| `norm:hlsv_sstatus_mxr` | HS-level `sstatus`.MXR makes execute-only pages readable by explicit loads for both stages of address translation (VS-stage and G-stage). | HS 级 `sstatus`.MXR 使仅执行页对两个阶段的地址翻译（VS 阶段和 G 阶段）的显式加载可读。 |
| `norm:hlsv_vsstatus_mxr` | `vsstatus`.MXR affects only the first translation stage (VS-stage). | `vsstatus`.MXR 仅影响第一翻译阶段（VS 阶段）。 |
| `norm:hlsv_u_op` | Instructions HLVX.HU and HLVX.WU are the same as HLV.HU and HLV.WU, except that execute permission takes the place of read permission during address translation. The supervisor physical memory attributes must grant both execute and read permissions. | HLVX.HU 和 HLVX.WU 与 HLV.HU 和 HLV.WU 相同，但地址翻译时执行权限替代读权限。supervisor 物理内存属性必须同时授予执行和读权限。 |
| `norm:hlsv_virtinst` | Attempts to execute a virtual-machine load/store instruction (HLV, HLVX, or HSV) when V=1 cause a virtual-instruction exception. | V=1 时尝试执行虚拟机 load/store 指令引发虚拟指令异常。 |
| `norm:hlsv_illegalinst` | Attempts to execute one of these same instructions from U-mode when `hstatus`.HU=0 cause an illegal-instruction exception. | 当 `hstatus`.HU=0 时从 U 模式执行这些指令引发非法指令异常。 |
| `norm:hfence-vvma_hfence-gvma_op` | HFENCE.VVMA and HFENCE.GVMA perform a function similar to SFENCE.VMA, except applying to the VS-level memory-management data structures controlled by CSR `vsatp` (HFENCE.VVMA) or the guest-physical memory-management data structures controlled by CSR `hgatp` (HFENCE.GVMA). | HFENCE.VVMA 和 HFENCE.GVMA 类似 SFENCE.VMA，但分别应用于 `vsatp` 控制的 VS 级内存管理数据结构和 `hgatp` 控制的客户物理内存管理数据结构。 |
| `norm:hfence-vvma_mode` | HFENCE.VVMA is valid only in M-mode or HS-mode. Executing an HFENCE.VVMA guarantees that any previous stores already visible to the current hart are ordered before all implicit reads by that hart done for VS-stage address translation for subsequent instructions when `hgatp`.VMID has the same setting. | HFENCE.VVMA 仅在 M 模式或 HS 模式有效。执行后保证先前可见的存储在 `hgatp`.VMID 相同时排在后续 VS 阶段地址翻译的隐式读取之前。 |
| `norm:hfence-vvma_limits` | Implicit reads need not be ordered when `hgatp`.VMID is different than at the time HFENCE.VVMA executed. If rs1≠x0, it specifies a single guest virtual address, and if rs2≠x0, it specifies a single guest address-space identifier (ASID). | `hgatp`.VMID 不同时隐式读取无需排序。rs1≠x0 指定单个客户虚拟地址，rs2≠x0 指定单个客户 ASID。 |
| `norm:hfence-vvma_asid` | When rs2≠x0, bits XLEN-1:ASIDMAX of the value held in rs2 are reserved for future standard use. If ASIDLEN < ASIDMAX, the implementation shall ignore bits ASIDMAX-1:ASIDLEN of the value held in rs2. | rs2≠x0 时，rs2 中 XLEN-1:ASIDMAX 位保留。若 ASIDLEN < ASIDMAX，实现应忽略 rs2 中 ASIDMAX-1:ASIDLEN 位。 |
| `norm:hfence-vvma_tvm` | Neither `mstatus`.TVM nor `hstatus`.VTVM causes HFENCE.VVMA to trap. | `mstatus`.TVM 和 `hstatus`.VTVM 都不会导致 HFENCE.VVMA 陷入。 |
| `norm:hfence-gvma_op` | HFENCE.GVMA is valid only in HS-mode when `mstatus`.TVM=0, or in M-mode (irrespective of `mstatus`.TVM). Executing an HFENCE.GVMA guarantees that any previous stores already visible to the current hart are ordered before all implicit reads done for G-stage address translation for subsequent instructions. If rs1≠x0, it specifies a single guest physical address, shifted right by 2 bits, and if rs2≠x0, it specifies a single VMID. | HFENCE.GVMA 仅在 `mstatus`.TVM=0 的 HS 模式或 M 模式下有效。执行后保证先前可见的存储排在后续 G 阶段地址翻译的隐式读取之前。rs1≠x0 指定右移 2 位的客户物理地址，rs2≠x0 指定 VMID。 |
| `norm:hfence-gvma_mode` | If `hgatp`.MODE is changed for a given VMID, an HFENCE.GVMA with rs1=x0 (and rs2 set to either x0 or the VMID) must be executed to order subsequent guest translations with the MODE change—even if the old MODE or new MODE is Bare. | 若给定 VMID 的 `hgatp`.MODE 改变，必须执行 rs1=x0 的 HFENCE.GVMA 以排序后续客户翻译——即使旧/新 MODE 为 Bare。 |
| `norm:hfence-gvma_vmid` | When rs2≠x0, bits XLEN-1:VMIDMAX of the value held in rs2 are reserved. If VMIDLEN < VMIDMAX, the implementation shall ignore bits VMIDMAX-1:VMIDLEN. | rs2≠x0 时，rs2 中 XLEN-1:VMIDMAX 位保留。若 VMIDLEN < VMIDMAX，实现应忽略对应位。 |
| `norm:hfence-vvma_hfence-gvma_exceptions` | Attempts to execute HFENCE.VVMA or HFENCE.GVMA when V=1 cause a virtual-instruction exception, while attempts in U-mode cause an illegal-instruction exception. Attempting HFENCE.GVMA in HS-mode when `mstatus`.TVM=1 also causes an illegal-instruction exception. | V=1 时执行 HFENCE.VVMA 或 HFENCE.GVMA 引发虚拟指令异常；U 模式下引发非法指令异常。HS 模式下 `mstatus`.TVM=1 时执行 HFENCE.GVMA 也引发非法指令异常。 |
| `norm:sfence_vma_v0` | When V=0, the virtual-address argument to SFENCE.VMA is an HS-level virtual address, and the ASID argument is an HS-level ASID. The instruction orders stores only to HS-level address-translation structures with subsequent HS-level address translations. | V=0 时，SFENCE.VMA 的虚拟地址参数为 HS 级虚拟地址，ASID 参数为 HS 级 ASID。指令仅对 HS 级地址翻译结构排序。 |
| `norm:sfence_vma_v1` | When V=1, the virtual-address argument is a guest virtual address within the current virtual machine, and the ASID argument is a VS-level ASID. The instruction orders stores only to the VS-level address-translation structures with subsequent VS-stage address translations for the same virtual machine. | V=1 时，虚拟地址参数为当前虚拟机内的客户虚拟地址，ASID 参数为 VS 级 ASID。指令仅对同一虚拟机的 VS 级地址翻译结构排序。 |
| `norm:mstatus_mprv_hypervisor` | The hypervisor extension changes the behavior of MPRV. When MPRV=0, normal translation. When MPRV=1, explicit memory accesses are translated and protected as though the current virtualization mode were set to MPV and the current nominal privilege mode were set to MPP. | hypervisor 扩展改变了 MPRV 的行为。MPRV=0 时正常翻译。MPRV=1 时显式内存访问如同当前虚拟化模式为 MPV、名义特权模式为 MPP 般翻译和保护。 |
| `norm:mstatus_mprv_hlsv` | MPRV does not affect the virtual-machine load/store instructions, HLV, HLVX, and HSV. The explicit loads and stores of these instructions always act as though V=1 and the nominal privilege mode were `hstatus`.SPVP, overriding MPRV. | MPRV 不影响虚拟机 load/store 指令。这些指令的显式访问始终如同 V=1 且名义特权模式为 `hstatus`.SPVP，覆盖 MPRV。 |
| `norm:H_guest_page_fault` | Guest-page-fault traps may be delegated from M-mode to HS-mode under the control of `medeleg`, but cannot be delegated to other privilege modes. On a guest-page fault, `mtval` or `stval` is written with the faulting guest virtual address, and `mtval2` or `htval` is written either with zero or with the faulting guest physical address, shifted right by 2 bits. | 客户页错误可通过 `medeleg` 从 M 模式委托给 HS 模式，但不能委托给其他特权模式。客户页错误时 `mtval`/`stval` 写入故障客户虚拟地址，`mtval2`/`htval` 写入零或故障客户物理地址右移 2 位。 |
| `norm:htval_trapval` | When a guest-page-fault trap is taken into HS-mode, `htval` is written with either zero or the guest physical address that faulted, shifted right by 2 bits. For other traps, `htval` is set to zero. | 客户页错误陷阱进入 HS 模式时，`htval` 写入零或故障的客户物理地址右移 2 位。其他陷阱设为零。 |
| `norm:H_trap_xtinst_guestpage` | For guest-page faults, the trap instruction register is written with a special pseudoinstruction value if: (a) the fault is caused by an implicit memory access for VS-stage address translation, and (b) a nonzero value is written to `mtval2` or `htval`. If both conditions are met, zero is not allowed. | 对于客户页错误，若 (a) 故障由 VS 阶段地址翻译的隐式内存访问引起，且 (b) `mtval2`/`htval` 写入非零值，则陷阱指令寄存器必须写入特殊伪指令值，不允许零。 |
| `norm:H_trap_xtinst_guestpage_rw` | A write pseudoinstruction (0x00002020 or 0x00003020) is used for the case that the machine is attempting automatically to update bits A and/or D in VS-level page tables. All other implicit memory accesses for VS-stage address translation will be reads. | 写伪指令（0x00002020 或 0x00003020）用于机器自动更新 VS 级页表 A/D 位的情况。所有其他 VS 阶段翻译的隐式内存访问为读取。 |
| `norm:henvcfg_adue_op` | If the Svadu extension is implemented, the ADUE bit controls whether hardware updating of PTE A/D bits is enabled for VS-stage address translation. When ADUE=1, hardware updating is enabled. When ADUE=0, the implementation behaves as though Svade were implemented for VS-stage address translation. If Svadu is not implemented, ADUE is read-only zero. | 若实现了 Svadu 扩展，ADUE 位控制 VS 阶段地址翻译的 PTE A/D 位硬件更新是否启用。ADUE=1 时启用，ADUE=0 时行为如同实现了 Svade。未实现 Svadu 时，ADUE 为只读零。 |
| `norm:henvcfg_pbmte_op` | The PBMTE bit controls whether the Svpbmt extension is available for use in VS-stage address translation. When PBMTE=1, Svpbmt is available for VS-stage address translation. When PBMTE=0, the implementation behaves as though Svpbmt were not implemented for VS-stage address translation. If Svpbmt is not implemented, PBMTE is read-only zero. | PBMTE 位控制 Svpbmt 扩展是否可用于 VS 阶段地址翻译。PBMTE=1 时可用，PBMTE=0 时行为如同未实现。若未实现 Svpbmt，PBMTE 为只读零。 |
| `norm:H_straddle` | When an instruction fetch or a misaligned memory access straddles a page boundary, two different address translations are involved. When a guest-page fault occurs, the faulting virtual address may be a page-boundary address that is higher than the instruction's original virtual address. | 当取指或未对齐内存访问跨越页边界时涉及两个地址翻译。客户页错误时故障虚拟地址可能是高于指令原始虚拟地址的页边界地址。 |
| `norm:mtval2_htval_virtaddr` | When a guest-page fault is not due to an implicit memory access for VS-stage address translation, a nonzero guest physical address written to `mtval2`/`htval` shall correspond to the exact virtual address written to `mtval`/`stval`. | 当客户页错误不是由 VS 阶段地址翻译的隐式内存访问引起时，写入 `mtval2`/`htval` 的非零客户物理地址必须对应写入 `mtval`/`stval` 的确切虚拟地址。 |
| `norm:H_vm_gpa_g` | The G bit in all G-stage PTEs is currently not used. It should be cleared by software for forward compatibility, and must be ignored by hardware. | G 阶段 PTE 中的 G 位当前未使用。软件应清零以保持前向兼容，硬件必须忽略。 |
| `norm:H_pmp` | Machine-level physical memory protection applies to supervisor physical addresses and is in effect regardless of virtualization mode. | 机器级物理内存保护适用于 supervisor 物理地址，与虚拟化模式无关。 |
| `norm:H_exception_priority` | If an instruction may raise multiple synchronous exceptions, the decreasing priority order indicates which exception is taken and reported in `mcause` or `scause`. | 若指令可能引发多个同步异常，按优先级递减顺序决定哪个异常被接收并报告在 `mcause` 或 `scause` 中。 |
| `norm:hgatp_ppn_op` | For the paged virtual-memory schemes, the root page table is 16 KiB and must be aligned to a 16-KiB boundary. In these modes, the lowest two bits of the physical page number (PPN) in `hgatp` always read as zeros. | 对于分页虚拟内存方案，根页表为 16 KiB 且必须对齐到 16 KiB 边界。这些模式下 `hgatp` 中 PPN 的最低两位始终读为零。 |
| `norm:hgatp_mode_warl` | A write to `hgatp` with an unsupported MODE value is not ignored as it is for `satp`. Instead, the fields of `hgatp` are WARL in the normal way, when so indicated. | 使用不支持的 MODE 值写入 `hgatp` 不会像 `satp` 那样被忽略。`hgatp` 的字段按常规 WARL 方式处理。 |

---

## 测试组定义

### Group 1：V=1 时 VS-stage 单阶段（hgatp = Bare）

**规范依据**：
- `norm:H_vm_twostage`：V=1 时强制启用两阶段，但任一阶段可通过对应 ATP 设为 Bare 而"有效禁用"
- `norm:vsatp_sz_acc_op`：`vsatp` 是 VS-mode 的 `satp`，使用相同的算法与 PTE 格式
- 当 hgatp=Bare 时，GPA 直接等于 SPA，两阶段退化为单 VS-stage 翻译

**测试职责**：当 hgatp=Bare 时，VS-stage 的行为应与普通 S-mode 下 satp 控制的翻译完全一致。本组以 `docs/vm_test_plan.md` 中的核心 Group 为基线，验证 V=1 下 vsatp 的对等行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-VS-01 | Sv39 vsatp 1GB gigapage | hgatp=Bare，vsatp=Sv39，1GB 恒等映射，VS-mode 读写 | 读写成功 |
| TS-VS-02 | Sv39 vsatp 2MB megapage | 同上 2MB | 读写成功 |
| TS-VS-03 | Sv39 vsatp 4KB page | 同上 4KB | 读写成功 |
| TS-VS-04 | Sv48 vsatp 4KB | hgatp=Bare，vsatp=Sv48，4KB 映射 | 读写成功 |
| TS-VS-05 | Sv57 vsatp 4KB | hgatp=Bare，vsatp=Sv57，4KB 映射 | 读写成功 |
| TS-VS-06 | VS-stage U-bit：VS 访问 U=0 | vsatp=Sv39，PTE U=0，VS-mode（nominal S）访问，`vsstatus.SUM=0` | 成功（VS-mode 是 S 级，U=0 PTE 拒绝 U-mode 访问，但允许 VS-mode） |
| TS-VS-07 | VS-stage U-bit：VS 访问 U=1 + SUM=0 | vsatp=Sv39，PTE U=1，VS-mode 访问，`vsstatus.SUM=0` | store/load page-fault（cause 13/15，与 vm_test_plan SUM-02 对等） |
| TS-VS-08 | VS-stage U-bit：VS 访问 U=1 + SUM=1 | 同上但 `vsstatus.SUM=1` | 成功 |
| TS-VS-09 | VS-stage MXR：vsstatus.MXR=1 读 X-only | PTE R=0,X=1，`vsstatus.MXR=1`，VS-mode load | 成功 |
| TS-VS-10 | VS-stage PTE V=0/RW=01 | vsatp=Sv39，PTE V=0 或 R=0,W=1 | page-fault（cause 12/13/15，**非** guest-page-fault） |

> [!NOTE]
> Group 1 全部用例以 `vm_test_plan.md` 的对应 Group 为基线，但 trap 上下文从 `s_trap_handler` 切换为 `hs_trap_handler`，且 trap 来源 `hstatus.SPV=1`。fault cause 仍是 12/13/15（普通 page-fault），因为 G-stage Bare 不会触发 guest-page-fault。

```c
/* TS-VS-03 示例：Sv39 vsatp 4KB 映射（hgatp=Bare） */
TEST_REGISTER(test_vsatp_sv39_4k_only);
bool test_vsatp_sv39_4k_only(void) {
    TEST_BEGIN("TS-VS-03: vsatp=Sv39 + hgatp=Bare, 4KB identity mapping");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, /*g=Bare*/0);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    /* hgatp=Bare 下 G-stage 不参与，VS-stage PTE 不必设置 U=1 */
    int ret = two_stage_setup_identity(&ctx, base, PAGE_SIZE_1G,
                                       flags, PT_LEVEL_4K);
    TEST_ASSERT("VS-stage identity setup", ret == 0);

    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_read_write,
                                           (uintptr_t)test_data_area);
    TEST_ASSERT("VS-mode 4KB R/W via vsatp only", result == 0);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

---

### Group 2：vsatp CSR 行为

**规范依据**：
- `norm:vsatp_sz_acc_op`：V=1 时 `satp` 实际访问 `vsatp`
- `norm:vsatp_mode_unsupported_v0`：V=0 时写 `vsatp` 不支持 MODE 的行为同 `satp`（被忽略或 WARL）
- `norm:vsatp_mode_unsupported_v1`：V=1 时写 `satp` 的不支持 MODE **被忽略**，且不写入 `vsatp`
- `norm:vs_stage_speculative_a_bit`：VS-stage A 位不得因推测执行而被设置（除非真正在 VS/VU-mode）
- `norm:vsatp_v0`：V=0 时 `vsatp` 不直接影响机器行为，但 HLV/HSV/MPRV 可触发 as-if V=1

**测试职责**：验证 vsatp 的访问、MODE WARL/忽略行为、ASID 字段、V=0/V=1 时的差异。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-VSATP-01 | V=1 时 satp 访问 vsatp | HS-mode 写 vsatp 一个值，切到 VS-mode 用 `csrr satp` 读 | 读到 vsatp 的值 |
| TS-VSATP-02 | V=1 时 satp 写入 vsatp | VS-mode 用 `csrw satp` 写一个合法值，切回 HS-mode 用 `csrr vsatp` 读 | vsatp 已被写入 |
| TS-VSATP-03 | V=1 写不支持 MODE 被忽略 | VS-mode 通过 `csrw satp` 写 MODE=2（保留） | vsatp 未被写入（与 V=0 行为不同） |
| TS-VSATP-04 | V=0 写 vsatp 保留 MODE | HS-mode 直接 `csrw vsatp` 写 MODE=7（保留编码，永远不支持） | (a) 整个写入被忽略（satp 语义），或 (b) MODE 被 WARL 调整为合法值（0/8/9/10）；保留编码不得残留（norm:vsatp_mode_unsupported_v0） |
| TS-VSATP-05 | vsatp ASID 字段读写 | HS-mode 写 vsatp ASID=0xFF，读回 | ASID 保留实现支持的位 |
| TS-VSATP-06 | vsatp PPN 字段读写 | HS-mode 写合法 PPN，读回 | PPN 字段读回正确（无类似 hgatp 的 PPN[1:0] 强制为零） |
| TS-VSATP-07 | hstatus.VTVM=1 时 VS 访问 satp | 设 `hstatus.VTVM=1`，VS-mode `csrr satp` | virtual-instruction exception (cause=22) |

---

### Group 3：完整两阶段恒等映射（同位宽）

**规范依据**：
- `norm:H_vm_twostage`：V=1 时两阶段同时生效
- `norm:H_vm_gstagetrans`：当 V=1 时，连支持 VS-stage 翻译的隐式访问（读/写 VS-level 页表）也经过 G-stage 翻译

**测试职责**：在 VA = GPA = SPA 的恒等映射下，验证三种同位宽两阶段组合的核心翻译路径。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-MAP-01 | Sv39+Sv39x4 1GB 两阶段恒等 | 两阶段 1GB 恒等映射，VS-mode R/W | 成功 |
| TS-MAP-02 | Sv39+Sv39x4 2MB | 两阶段 2MB | 成功 |
| TS-MAP-03 | Sv39+Sv39x4 4KB | 两阶段 4KB | 成功 |
| TS-MAP-04 | Sv39+Sv39x4 superpage 混合 | VS-stage 用 1GB，G-stage 用 4KB；以及反向 | 成功（实际页大小取最小的） |
| TS-MAP-05 | Sv48+Sv48x4 1GB | 同模式 1GB 两阶段 | 成功 |
| TS-MAP-06 | Sv48+Sv48x4 2MB | 同模式 2MB | 成功 |
| TS-MAP-07 | Sv48+Sv48x4 4KB | 同模式 4KB | 成功 |
| TS-MAP-08 | Sv48+Sv48x4 512GB terapage | 大 superpage（受平台内存限制） | 成功（在合法子区间内） |
| TS-MAP-09 | Sv57+Sv57x4 1GB | 同模式 1GB | 成功 |
| TS-MAP-10 | Sv57+Sv57x4 2MB | 同模式 2MB | 成功 |
| TS-MAP-11 | Sv57+Sv57x4 4KB | 同模式 4KB | 成功 |
| TS-MAP-12 | VU-mode 两阶段访问 | Sv39+Sv39x4，VS-stage 与 G-stage 均 U=1，进入 VU-mode 访问 | 成功 |

```c
/* TS-MAP-01：Sv39 + Sv39x4 完整两阶段恒等映射 */
TEST_REGISTER(test_2s_sv39_sv39x4_identity);
bool test_2s_sv39_sv39x4_identity(void) {
    TEST_BEGIN("TS-MAP-01: Sv39+Sv39x4 two-stage identity VA=GPA=SPA");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    /* G-stage PTE 必须 U=1（G-stage 视所有访问为 U-mode）；
     * VS-stage PTE 不必 U=1（VS-mode 是 S 级），但本用例为简化 setup
     * 在 two_stage_setup_identity 中统一传入相同 flags */
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X
                    | PTE_U | PTE_A | PTE_D;
    int ret = two_stage_setup_identity(&ctx, base, PAGE_SIZE_1G,
                                       flags, PT_LEVEL_1G);
    TEST_ASSERT("two-stage identity setup", ret == 0);

    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_read_write,
                                           (uintptr_t)test_data_area);
    TEST_ASSERT("VA->GPA->SPA succeeds", result == 0);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

---

### Group 4：异位宽两阶段组合

**规范依据**：
- VS-stage 与 G-stage 的 MODE 选择相互独立，规范未要求位宽一致（典型用法是 hypervisor 在 hgatp 选择宽于或窄于 vsatp 的方案）
- `norm:hgatp_mode_sv39x4`、`norm:hgatp_mode_sv48x4`、`norm:hgatp_mode_sv57x4`：定义了三种 G-stage 模式

**测试职责**：覆盖全部 9 种异位宽组合（含 3 种 VS≤G 和 3 种 VS>G 的正常流，以及 3 种 VS>G 的 GPA 越界故障流），每对以多页面粒度子变体（4K/2M/1G）验证组合可用且翻译正确。

#### VS≤G 正常流（VS 地址空间不宽于 G）

| 测试 ID | 测试名称 | VS-stage | G-stage | 测试描述 | 预期结果 |
|---------|----------|---------|--------|----------|----------|
| TS-XMODE-01.a | Sv39+Sv48x4 VS=4K G=4K | Sv39 | Sv48x4 | 4KB 恒等映射 R/W | 成功 |
| TS-XMODE-01.b | Sv39+Sv48x4 VS=4K G=2M | Sv39 | Sv48x4 | G-stage 2MB superpage | 成功 |
| TS-XMODE-01.c | Sv39+Sv48x4 VS=4K G=1G | Sv39 | Sv48x4 | G-stage 1GB superpage | 成功 |
| TS-XMODE-02.a | Sv39+Sv57x4 VS=4K G=4K | Sv39 | Sv57x4 | 4KB 恒等映射 R/W | 成功 |
| TS-XMODE-02.b | Sv39+Sv57x4 VS=4K G=2M | Sv39 | Sv57x4 | G-stage 2MB superpage | 成功 |
| TS-XMODE-02.c | Sv39+Sv57x4 VS=4K G=1G | Sv39 | Sv57x4 | G-stage 1GB superpage | 成功 |
| TS-XMODE-04.a | Sv48+Sv57x4 VS=4K G=4K | Sv48 | Sv57x4 | 4KB 恒等映射 R/W | 成功 |
| TS-XMODE-04.b | Sv48+Sv57x4 VS=4K G=2M | Sv48 | Sv57x4 | G-stage 2MB superpage | 成功 |
| TS-XMODE-04.c | Sv48+Sv57x4 VS=4K G=1G | Sv48 | Sv57x4 | G-stage 1GB superpage | 成功 |

#### VS>G 正常流（VS 地址空间宽于 G，但 GPA 落在 G-stage 可寻址范围内）

| 测试 ID | 测试名称 | VS-stage | G-stage | 测试描述 | 预期结果 |
|---------|----------|---------|--------|----------|----------|
| TS-XMODE-03.a | Sv48+Sv39x4 VS=4K G=4K | Sv48 | Sv39x4 | 4KB 恒等映射 R/W；GPA 落在 [0, 2^41) 内 | 成功 |
| TS-XMODE-03.b | Sv48+Sv39x4 VS=4K G=2M | Sv48 | Sv39x4 | G-stage 2MB superpage；GPA < 2^41 | 成功 |
| TS-XMODE-03.c | Sv48+Sv39x4 VS=4K G=1G | Sv48 | Sv39x4 | G-stage 1GB superpage；GPA < 2^41 | 成功 |
| TS-XMODE-05.a | Sv57+Sv39x4 VS=4K G=4K | Sv57 | Sv39x4 | 4KB 恒等映射 R/W；GPA < 2^41 | 成功 |
| TS-XMODE-05.b | Sv57+Sv39x4 VS=4K G=2M | Sv57 | Sv39x4 | G-stage 2MB superpage；GPA < 2^41 | 成功 |
| TS-XMODE-05.c | Sv57+Sv39x4 VS=4K G=1G | Sv57 | Sv39x4 | G-stage 1GB superpage；GPA < 2^41 | 成功 |
| TS-XMODE-06.a | Sv57+Sv48x4 VS=4K G=4K | Sv57 | Sv48x4 | 4KB 恒等映射 R/W；GPA < 2^50 | 成功 |
| TS-XMODE-06.b | Sv57+Sv48x4 VS=4K G=2M | Sv57 | Sv48x4 | G-stage 2MB superpage；GPA < 2^50 | 成功 |
| TS-XMODE-06.c | Sv57+Sv48x4 VS=4K G=1G | Sv57 | Sv48x4 | G-stage 1GB superpage；GPA < 2^50 | 成功 |

#### VS>G 越界故障流（VS 翻译产生的 GPA 超出 G-stage 可寻址范围）

| 测试 ID | 测试名称 | VS-stage | G-stage | 测试描述 | 预期结果 |
|---------|----------|---------|--------|----------|----------|
| TS-XMODE-07 | Sv48+Sv39x4 GPA 越界 | Sv48 | Sv39x4 | VS-stage PTE 的 PPN 指向 GPA ≥ 2^41 | guest-page-fault (cause=21) |
| TS-XMODE-08 | Sv57+Sv39x4 GPA 越界 | Sv57 | Sv39x4 | VS-stage PTE 的 PPN 指向 GPA ≥ 2^41 | guest-page-fault (cause=21) |
| TS-XMODE-09 | Sv57+Sv48x4 GPA 越界 | Sv57 | Sv48x4 | VS-stage PTE 的 PPN 指向 GPA ≥ 2^50 | guest-page-fault (cause=21) |

> [!NOTE]
> - TS-XMODE-03/05/06 的关键场景：虽然 VS-stage 地址空间宽于 G-stage，但只要 VS 翻译产出的 GPA 落在 G-stage 可寻址范围内（低地址区域），两阶段翻译应正常完成。测试使用低内存区域的 identity 映射（test_data_area < 4GB）来验证此场景。
> - TS-XMODE-07/08/09 的关键场景：当 VS-stage 翻译产生的 GPA 超出 G-stage 模式可处理的范围时（例如 Sv48 输出 GPA ≥ 2^41 但 hgatp=Sv39x4 仅支持 41-bit GPA），应触发 guest-page-fault (cause=21)。实现方式为篡改 VS leaf PTE 的 PPN 使其指向越界地址。

---

### Group 5：非恒等两阶段映射

**规范依据**：
- `norm:H_vm_twostage`：两阶段独立翻译，最终物理地址 = 两阶段映射的复合
- 规范未限制 VA、GPA、SPA 必须相等

**测试职责**：在非恒等映射下验证两阶段链路的端到端正确性，并验证 VS-stage 输出的 GPA 与 G-stage 输入的 GPA 严格一致。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-NID-01 | VS-stage 非恒等 | VS-stage 映射 VA1 → GPA_X，G-stage 映射 GPA_X → SPA_X（GPA_X = SPA_X 恒等）；VS-mode 写 VA1，物理 SPA_X 应被修改 | 物理读 SPA_X 看到写入值 |
| TS-NID-02 | G-stage 非恒等 | VS-stage 映射 VA1 → GPA_X 恒等；G-stage 映射 GPA_X → SPA_Y（不同地址）；VS-mode 写 VA1，物理 SPA_Y 应被修改 | 物理读 SPA_Y 看到写入值；物理读 GPA_X 不变 |
| TS-NID-03 | 两阶段都非恒等 | VS-stage VA1 → GPA_X，G-stage GPA_X → SPA_Y | VS-mode 访问 VA1 影响物理 SPA_Y |
| TS-NID-04 | 多页映射连续性 | VS-stage 与 G-stage 各自把连续 4 页映射到不同基地址，验证 4 个页的映射均生效 | 每页 R/W 均成功且数据隔离 |

---

### Group 6：隐式访问引发的 G-stage fault

**规范依据**：
- `norm:H_vm_gstagetrans`：V=1 时支持 VS-stage 翻译的隐式访问（读 VS-level 页表）也受 G-stage 翻译
- `norm:H_vm_gpapriv`：隐式访问按 implicit load/store 检查权限和 A/D 位
- `norm:H_guest_page_fault`：fault 时 stval=GVA，htval 可能写入隐式访问的 GPA
- `norm:htval_trapval`：当 fault 由隐式 VS-stage 访问触发时，htval 写入 **PTE 的 GPA**（不是原始 VA 对应的 GPA）
- `norm:H_trap_xtinst_guestpage`：此种场景下 htinst 必须写 pseudoinstruction（不允许 0）
- `norm:H_trap_xtinst_guestpage_rw`：read 用 `0x00002000`/`0x00003000`，write（A/D 自动更新）用 `0x00002020`/`0x00003020`

**测试职责**：把 VS-stage 页表本身放在一个 G-stage 不映射或无权限的 GPA，触发隐式访问 fault，验证 htval/htinst/cause 的正确性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-IMPL-01 | VS-stage PT 在 G-stage 未映射 GPA | VS-stage 根页表分配在 GPA_PT，G-stage 不映射 GPA_PT；VS-mode load VA | cause=21 (load guest-page-fault), stval=VA, htval=GPA_PT>>2, htinst=`0x00003000` (RV64 read) |
| TS-IMPL-03 | VS-stage PT 在 G-stage 只读 + D=0 store | VS-stage PTE 需要硬件更新 D 位（store 访问），但 G-stage 把对应 GPA 映射为只读 | cause=23（HW A/D 实现）或 cause=21（无 HW A/D 实现），两种均合规通过 |
| TS-IMPL-04 | VS-stage PT G-stage U=0 | VS-stage 页表 GPA 在 G-stage 映射 U=0 | 隐式访问失败，guest-page-fault |
| TS-IMPL-06 | inst guest-page-fault from implicit | VS-stage 翻译 fetch 时 PT 隐式访问失败 | cause=20, htinst 必须为 pseudoinst（read：`0x00003000`） |

```c
/* TS-IMPL-01：VS-level 页表自身位于无 G-stage 映射的 GPA */
TEST_REGISTER(test_2s_implicit_gstage_fault);
bool test_2s_implicit_gstage_fault(void) {
    TEST_BEGIN("TS-IMPL-01: VS-stage PT walk implicit access -> G-stage fault");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* 1. VS-stage：建立 VA → GPA 映射（VS-stage 页表本身位于 ctx.vs_ctx.pool） */
    uintptr_t va = TEST_REGION_BASE;
    uintptr_t gpa_target = TEST_REGION_BASE;
    pt_map_page(&ctx.vs_ctx, va, gpa_target,
                PTE_V|PTE_R|PTE_W|PTE_A|PTE_D, PT_LEVEL_4K);

    /* 2. 通过 ctx.vs_ctx.root_pt 显式拿到 VS-level 根页表的物理地址
     *    (common/vm/page_table.c 的 pt_context 已暴露 root_pt 字段，
     *     与 hypervisor_framework.md 中 gpt_context 的 root_pt 字段对齐) */
    uintptr_t vs_root_spa = (uintptr_t)ctx.vs_ctx.root_pt;
    uintptr_t vs_root_gpa = vs_root_spa;  /* 在恒等映射下 GPA = SPA */

    /* 3. G-stage：先把测试目标页（GPA=va）显式映射，再独立把
     *    "VS-stage 根页表所在的 GPA" 标记为无效。
     *    避免使用大 superpage 覆盖整个内存导致根页表也被恒等映射。
     *    这样保证：
     *      - VS-mode 取指、栈、目标数据访问都走 G-stage 已映射的页
     *      - 而对 VS-stage PT 的隐式 walk 被 G-stage 拦截 */
    /* 3.1 代码段 + 栈 + 目标数据：4KB 颗粒精细映射（仅举关键段，
     *     具体覆盖范围由 kernel.ld 中 .text/.data/.stack/test_data_area
     *     等符号决定，此处用伪代码示意） */
    extern char __text_start[], __text_end[];
    extern char __data_start[], __data_end[];
    extern char __stack_start[], __stack_end[];
    map_range_4k(&ctx.g_ctx, (uintptr_t)__text_start,
                 (uintptr_t)__text_end - (uintptr_t)__text_start);
    map_range_4k(&ctx.g_ctx, (uintptr_t)__data_start,
                 (uintptr_t)__data_end - (uintptr_t)__data_start);
    map_range_4k(&ctx.g_ctx, (uintptr_t)__stack_start,
                 (uintptr_t)__stack_end - (uintptr_t)__stack_start);
    /* 测试目标数据页 */
    gpt_map_page(&ctx.g_ctx, gpa_target, gpa_target,
                 PTE_V|PTE_R|PTE_W|PTE_U|PTE_A|PTE_D, PT_LEVEL_4K);

    /* 3.2 显式不映射 VS-stage 根页表所在的 GPA（4KB 粒度，V=0） */
    gpt_map_page(&ctx.g_ctx, vs_root_gpa, vs_root_gpa,
                 0, /* V=0：无效 PTE */
                 PT_LEVEL_4K);

    /* 4. 触发：VS-mode load VA，硬件遍历 VS-stage PT 时
     *    根页表的隐式 read 被 G-stage 拦截 */
    EXPECT_GUEST_PAGE_FAULT(CAUSE_LOAD_GUEST_PAGE_FAULT,
        two_stage_run_in_vs(&ctx, test_vs_load, va));

    /* htval 指向 VS-level PTE 的 GPA（不是原始 VA 对应的 GPA） */
    CHECK_HTVAL("htval points to VS-level root PT GPA",
                vs_root_gpa >> 2);
    CHECK_HTINST("htinst is RV64 read pseudoinst", 0x00003000);
    CHECK_GVA("hstatus.GVA = 1", true);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}

/* 辅助：按 4KB 粒度恒等映射一段地址范围到 G-stage（U=1, RWX, A/D=1） */
static inline void map_range_4k(gpt_context_t *g, uintptr_t base,
                                uintptr_t size) {
    uintptr_t end = (base + size + PAGE_SIZE_4K - 1) & ~(PAGE_SIZE_4K - 1);
    base &= ~(PAGE_SIZE_4K - 1);
    for (uintptr_t p = base; p < end; p += PAGE_SIZE_4K) {
        gpt_map_page(g, p, p,
                     PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D,
                     PT_LEVEL_4K);
    }
}
```

> [!NOTE]
> TS-IMPL-01 通过 `ctx.vs_ctx.root_pt` 字段（仓库 `common/vm/page_table.c` 的 `pt_context` 已提供）拿到 VS-level 根页表物理地址，**不依赖任何未实现的接口**。`__text_start/__text_end` 等 linker symbol 由现有 `kernel.ld` 提供（参见 `sv39/kernel.ld`），新建测试目录时需保持同样的符号导出。`map_range_4k` 是测试文件内的本地辅助函数。

---

### Group 7：权限交叉（VS-stage RWX × G-stage RWX）

**规范依据**：
- 两阶段权限取交集：任一阶段拒绝即触发 fault
- fault cause 区分：VS-stage 失败 → page-fault (12/13/15)；G-stage 失败 → guest-page-fault (20/21/23)

**测试职责**：覆盖 VS-stage 与 G-stage 各自 RWX 权限的关键交叉，验证 fault cause 与 stage 来源的对应关系。

| 测试 ID | 测试名称 | VS-stage PTE | G-stage PTE | 访问类型 | 预期结果 |
|---------|----------|-------------|------------|---------|----------|
| TS-PERM-01 | 双阶段 RWX | RWX | RWXU | R/W/X | 全部成功 |
| TS-PERM-02 | VS-stage R-only，G-stage RWX | R | RWXU | store | store page-fault (cause=15, **VS-stage 来源**) |
| TS-PERM-03 | VS-stage RWX，G-stage R-only | RWX | RU | store | store guest-page-fault (cause=23, **G-stage 来源**) |
| TS-PERM-04 | VS-stage X-only，G-stage RWX | X | RWXU | load（无 MXR） | load page-fault (cause=13) |
| TS-PERM-05 | VS-stage RWX，G-stage X-only | RWX | XU | load（无 MXR） | load guest-page-fault (cause=21) |
| TS-PERM-06 | VS-stage RX，G-stage RX | RX | RXU | fetch | 成功 |
| TS-PERM-07 | VS-stage V=0 | V=0 | RWXU | load | page-fault (cause=13) |
| TS-PERM-08 | G-stage V=0 | RWX | V=0 | load | guest-page-fault (cause=21) |
| TS-PERM-09 | G-stage U=0 | RWX | RWX (U=0) | load | guest-page-fault (cause=21) |
| TS-PERM-10 | VS-stage U=0 (VU 访问) | RWX (U=0) | RWXU | VU-mode load | page-fault (cause=13, U-mode 不能访问 U=0) |
| TS-PERM-11 | VS-stage U=1 (VS 访问，SUM=0) | RWX (U=1) | RWXU | VS-mode load with vsstatus.SUM=0 | page-fault (cause=13) |
| TS-PERM-12 | VS-stage U=1 (VS 访问，SUM=1) | RWX (U=1) | RWXU | VS-mode load with vsstatus.SUM=1 | 成功 |

---

### Group 8：MXR 在两阶段的作用差异

**规范依据**：
- `norm:vsstatus_mxr_vm`：「The vsstatus field MXR ... only overrides VS-stage page protection. Setting MXR at VS-level does not override guest-physical page protections.」
- `norm:sstatus_mxr_vm`：「Setting MXR at HS-level, however, overrides both VS-stage and G-stage execute-only permissions.」

**测试职责**：验证 MXR 在两阶段的双重语义。

| 测试 ID | 测试名称 | VS-stage | G-stage | sstatus.MXR | vsstatus.MXR | 访问 | 预期结果 |
|---------|----------|---------|--------|------------|-------------|------|----------|
| TS-MXR-01 | 无 MXR，X-only 不可读 | X-only | RWXU | 0 | 0 | load | load page-fault |
| TS-MXR-02 | vsstatus.MXR=1，VS X-only 可读 | X-only | RWXU | 0 | 1 | load | 成功 |
| TS-MXR-03 | vsstatus.MXR=1 不能 override G-stage X-only | RWX | X-only U | 0 | 1 | load | guest-page-fault（vsstatus.MXR 不影响 G-stage） |
| TS-MXR-04 | sstatus.MXR=1 同时 override 两阶段 | X-only | X-only U | 1 | 0 | load | 成功 |
| TS-MXR-05 | sstatus.MXR=1 + vsstatus.MXR=1 | X-only | X-only U | 1 | 1 | load | 成功（与 TS-MXR-04 等价） |

```c
/* TS-MXR-03 示例：vsstatus.MXR=1 不影响 G-stage */
TEST_REGISTER(test_vsstatus_mxr_does_not_override_gstage);
bool test_vsstatus_mxr_does_not_override_gstage(void) {
    TEST_BEGIN("TS-MXR-03: vsstatus.MXR cannot override G-stage execute-only");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* 代码区两阶段都 RWX */
    uintptr_t code_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    pt_setup_identity_mapping(&ctx.vs_ctx, code_base, PAGE_SIZE_1G,
                              PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D, PT_LEVEL_1G);
    gpt_setup_identity_mapping(&ctx.g_ctx, code_base, PAGE_SIZE_1G,
                               PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D,
                               PT_LEVEL_1G);

    /* 测试页：VS-stage RWX，G-stage X-only */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx.vs_ctx, test_va, test_va,
                PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D, PT_LEVEL_4K);
    gpt_map_page(&ctx.g_ctx, test_va, test_va,
                 PTE_V|PTE_X|PTE_U|PTE_A|PTE_D, PT_LEVEL_4K);

    /* 设 vsstatus.MXR=1，sstatus.MXR=0
     * 使用仓库 common/encoding.h 提供的 CSRS / CSRC 宏（编译期 CSR 名）。
     * SSTATUS_MXR 与 vsstatus 对应位的常量来自 encoding.h；vsstatus 与 sstatus
     * 共享相同字段布局，因此沿用 SSTATUS_MXR 即可。 */
    CSRS(vsstatus, SSTATUS_MXR);
    CSRC(sstatus,  SSTATUS_MXR);

    /* VS-mode load 应触发 G-stage load guest-page-fault */
    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_load_expect_fault,
                                           test_va);
    TEST_ASSERT_EQ("vsstatus.MXR does NOT cover G-stage X-only",
                   result, CAUSE_LOAD_GUEST_PAGE_FAULT);

    CSRC(vsstatus, SSTATUS_MXR);
    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

---

### Group 9：SUM 在两阶段的作用

**规范依据**：
- `vsstatus.SUM` 控制 VS-stage 的 U-bit 检查（VS-mode 是否能访问 U=1 PTE）
- G-stage 始终视为 U-mode，不存在 SUM 概念
- `norm:hlsv_trans`：HLV/HSV 时 HS-level `sstatus.SUM` 被忽略（详见 Group 13）

**测试职责**：验证 vsstatus.SUM 仅作用于 VS-stage。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-SUM-01 | vsstatus.SUM=0，VS 访问 U=1 | VS-stage PTE U=1，VS-mode load with SUM=0 | page-fault (cause=13) |
| TS-SUM-02 | vsstatus.SUM=1，VS 访问 U=1 | 同上 SUM=1 | 成功 |
| TS-SUM-03 | SUM 不影响 G-stage U-bit | VS-stage U=1（VS 访问 OK with SUM=1），G-stage U=0 | guest-page-fault（G-stage 不受 SUM 影响） |

---

### Group 10：HFENCE.VVMA 语义

**规范依据**：
- `norm:hfence-vvma_hfence-gvma_op`：HFENCE.VVMA 类似在 VS-mode 执行 SFENCE.VMA，但保证后续 VS-stage 翻译可见之前的 VS-level 页表写入
- `norm:hfence-vvma_mode`：仅 M-mode / HS-mode 有效
- `norm:hfence-vvma_limits`：仅作用于当前 hgatp.VMID 对应的虚拟机；rs1/rs2 选择 VA/ASID
- `norm:hfence-vvma_tvm`：mstatus.TVM 与 hstatus.VTVM 都不影响 HFENCE.VVMA
- `norm:hfence-vvma_hfence-gvma_exceptions`：V=1 执行 → virtual-instruction exception (cause=22)；U-mode 执行 → illegal-instruction exception

**测试职责**：验证 HFENCE.VVMA 的刷新效果与异常行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-HV-01 | 全局刷新 (rs1=0, rs2=0) | VS-stage 修改 PTE 后 HS-mode 执行 `hfence.vvma`，再 VS-mode 访问 | 新 PTE 生效 |
| TS-HV-02 | 按 VA 刷新 (rs1≠0) | 修改 VS-stage PTE 后 `hfence.vvma vaddr, x0` | 该 VA 新 PTE 生效 |
| TS-HV-03 | 按 ASID 刷新 (rs2≠0) | `hfence.vvma x0, asid` | 指定 ASID 翻译被刷新 |
| TS-HV-04 | mstatus.TVM=1 不影响 | HS-mode 设 TVM=1 后执行 `hfence.vvma` | 正常执行（不报异常） |
| TS-HV-05 | hstatus.VTVM=1 不影响 | HS-mode 设 VTVM=1 后执行 `hfence.vvma` | 正常执行 |
| TS-HV-06 | V=1 执行 → virtual-inst exception | VS-mode 执行 `hfence.vvma` | virtual-instruction exception (cause=22) |

---

### Group 11：HFENCE.GVMA 语义

**规范依据**：
- `norm:hfence-gvma_op`：HFENCE.GVMA 刷新 G-stage 翻译缓存
- 仅 HS-mode (mstatus.TVM=0) 或 M-mode 有效
- `norm:hfence-gvma_mode`：修改 hgatp.MODE 后必须执行 HFENCE.GVMA（rs1=0）
- `norm:hfence-gvma_vmid`：rs2 选择 VMID
- rs1 是 GPA>>2
- `norm:hfence-vvma_hfence-gvma_exceptions`：V=1 → virtual-inst；mstatus.TVM=1 在 HS-mode → illegal-inst；U-mode → illegal-inst

**测试职责**：验证 HFENCE.GVMA 的刷新效果与异常行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-HG-01 | 全局刷新 (rs1=0, rs2=0) | HS-mode 修改 G-stage PTE 后 `hfence.gvma`，VS-mode 访问 | 新 G-stage PTE 生效 |
| TS-HG-02 | 按 GPA 刷新 (rs1=GPA>>2) | `hfence.gvma gpa>>2, x0` | 指定 GPA 翻译被刷新 |
| TS-HG-03 | 按 VMID 刷新 (rs2=vmid) | `hfence.gvma x0, vmid` | 指定 VMID 翻译被刷新 |
| TS-HG-04 | hgatp.MODE 切换后必须 fence | 切换 hgatp.MODE Sv39x4↔Sv48x4 | 切换后执行 `hfence.gvma` 才正常工作 |
| TS-HG-05 | mstatus.TVM=1 触发 illegal | HS-mode 设 TVM=1 后 `hfence.gvma` | illegal-instruction (cause=2) |
| TS-HG-06 | V=1 执行 → virtual-inst | VS-mode 执行 `hfence.gvma` | virtual-instruction (cause=22) |

---

### Group 12：SFENCE.VMA 在 V=1 下行为

**规范依据**：
- `norm:sfence_vma_v0`：V=0 时 SFENCE.VMA 操作 HS-level 页表
- `norm:sfence_vma_v1`：V=1 时 SFENCE.VMA 的 VA 是 GVA，ASID 是 VS-level ASID，仅作用于 hgatp.VMID 对应的虚拟机
- `norm:hstatus_vtvm_op`：hstatus.VTVM=1 时 VS-mode 执行 SFENCE.VMA → virtual-instruction exception

**测试职责**：验证 SFENCE.VMA 在 V=1 下的语义切换。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-SF-01 | V=1 SFENCE.VMA 刷新 VS-stage | VS-mode 修改 vsatp 控制的 PTE 后执行 `sfence.vma` | 新 PTE 生效 |
| TS-SF-02 | V=1 SFENCE.VMA 不刷新 G-stage | VS-mode `sfence.vma` 不影响 G-stage 翻译缓存 | G-stage 修改不立即生效（需 HFENCE.GVMA） |
| TS-SF-03 | hstatus.VTVM=1 → virtual-inst | 设 VTVM=1，VS-mode 执行 `sfence.vma` | virtual-instruction (cause=22) |
| TS-SF-04 | V=0 SFENCE.VMA 不影响 VS-stage | HS-mode 执行 `sfence.vma` | 仅刷新 HS-level 翻译缓存 |

---

### Group 13：HLV/HLVX/HSV 两阶段翻译

**规范依据**：
- `norm:hlsv_mode`：HLV/HLVX/HSV 仅在 M-mode 或 HS-mode 有效；U-mode 仅当 `hstatus.HU=1` 时可用
- `norm:hlsv_priv`：effective privilege 由 `hstatus.SPVP` 决定（0=VU，1=VS）
- `norm:hlsv_trans`：始终使用两阶段翻译（vsatp + hgatp），HS-level `sstatus.SUM` 被忽略
- `norm:hlsv_sstatus_mxr`：HS-level `sstatus.MXR` 同时影响两阶段
- `norm:hlsv_vsstatus_mxr`：`vsstatus.MXR` 仅影响 VS-stage
- `norm:hlsv_u_op`：HLVX.HU/WU 用 execute 权限替代 read 权限
- `norm:hlvx-wu_valid32`：HLVX.WU 在 RV32 也有效
- `norm:hlsv_virtinst`：V=1 时执行 HLV/HLVX/HSV → virtual-instruction exception
- `norm:hlsv_illegalinst`：U-mode 且 hstatus.HU=0 时 → illegal-instruction exception

**测试职责**：验证 HLV/HLVX/HSV 在两阶段翻译下的行为、effective privilege 控制、MXR/SUM 差异、异常触发。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-HLV-01 | HS-mode HLV.D 读 guest 内存 | 两阶段映射建立，HS-mode 执行 `hlv.d` 读 guest VA | 读到正确值 |
| TS-HLV-02 | HS-mode HSV.D 写 guest 内存 | HS-mode 执行 `hsv.d` 写 guest VA | guest 内存被写入 |
| TS-HLV-03 | SPVP=0 (VU) 访问 U=0 → fault | VS-stage PTE U=0，SPVP=0，HS-mode `hlv.d` | load page-fault（U-mode 不能访问 U=0） |
| TS-HLV-04 | SPVP=1 (VS) 访问 U=0 → 成功 | VS-stage PTE U=0，SPVP=1，HS-mode `hlv.d` | 成功（VS-mode 是 S 级） |
| TS-HLV-05 | sstatus.SUM 被忽略 | VS-stage PTE U=1，SPVP=1，sstatus.SUM=0 但 vsstatus.SUM=1，HS-mode `hlv.d` | 成功（HS sstatus.SUM 不影响，由 vsstatus.SUM 控制） |
| TS-HLV-06 | sstatus.MXR=1 影响 VS-stage X-only | VS-stage X-only，G-stage RWX，sstatus.MXR=1，HS-mode `hlv.d` | 成功 |
| TS-HLV-07 | sstatus.MXR=1 影响 G-stage X-only | VS-stage RWX，G-stage X-only U，sstatus.MXR=1，HS-mode `hlv.d` | 成功（HS-MXR 影响两阶段） |
| TS-HLV-08 | vsstatus.MXR=1 不影响 G-stage X-only | VS-stage RWX，G-stage X-only U，vsstatus.MXR=1，sstatus.MXR=0，HS-mode `hlv.d` | guest-page-fault |
| TS-HLV-09 | HLVX.WU 用 X 权限替代 R | VS-stage X-only（R=0,X=1），G-stage X-only U，HS-mode `hlvx.wu` | 成功（X 权限即可） |
| TS-HLV-10 | V=1 执行 HLV → virtual-inst | VS-mode 执行 `hlv.d` | virtual-instruction exception (cause=22) |
| TS-HLV-11 | U-mode + HU=0 → illegal | hstatus.HU=0，U-mode 执行 `hlv.d` | illegal-instruction exception (cause=2) |
| TS-HLV-12 | U-mode + HU=1 → 正常 | hstatus.HU=1，U-mode 执行 `hlv.d`，正确两阶段映射 | 成功 |

```c
/* TS-HLV-07：sstatus.MXR=1 使 G-stage X-only 页面可读 */
TEST_REGISTER(test_sstatus_mxr_overrides_gstage);
bool test_sstatus_mxr_overrides_gstage(void) {
    TEST_BEGIN("TS-HLV-07: sstatus.MXR makes G-stage X-only readable via HLV");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* 代码区映射：两阶段 RWX */
    uintptr_t code_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    pt_setup_identity_mapping(&ctx.vs_ctx, code_base, PAGE_SIZE_1G,
                              PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D, PT_LEVEL_1G);
    gpt_setup_identity_mapping(&ctx.g_ctx, code_base, PAGE_SIZE_1G,
                               PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D,
                               PT_LEVEL_1G);

    /* 测试页：VS-stage RWX，G-stage X-only */
    uintptr_t test_va = TEST_REGION_BASE;
    pt_map_page(&ctx.vs_ctx, test_va, test_va,
                PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D, PT_LEVEL_4K);
    gpt_map_page(&ctx.g_ctx, test_va, test_va,
                 PTE_V|PTE_X|PTE_U|PTE_A|PTE_D, PT_LEVEL_4K);

    /* 预填 magic value 到测试页（恒等映射下 SPA=test_va） */
    const uint64_t MAGIC = 0xDEADBEEFCAFEBABEULL;
    *(volatile uint64_t *)test_va = MAGIC;

    /* 启用两阶段（不进入 VS-mode，HS-mode 直接 HLV） */
    gpt_enable(&ctx.g_ctx, /*vmid=*/0);
    /* CSR_VSATP 与 MAKE_SATP 由 common/vm/vm_defs.h 与 common/encoding.h 提供 */
    CSRW(vsatp, MAKE_SATP(SATP_MODE_SV39, /*asid=*/0,
                          ((uintptr_t)ctx.vs_ctx.root_pt) >> 12));

    /* sstatus.MXR=1, vsstatus.MXR=0；SPVP=1（VS-level） */
    CSRS(sstatus,  SSTATUS_MXR);
    CSRC(vsstatus, SSTATUS_MXR);
    hstatus_set_spvp(1);

    goto_priv(PRIV_S);  /* HS-mode */
    uint64_t val = hlv_d(test_va);
    goto_priv(PRIV_M);

    TEST_ASSERT_EQ("HLV.D returned magic via two-stage with sstatus.MXR",
                   val, MAGIC);

    CSRC(sstatus, SSTATUS_MXR);
    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

> [!NOTE]
> TS-HLV-09（HLVX）测试要求测试页面在 VS-stage 与 G-stage 两阶段都至少 X=1（read 权限可缺失）。同时 SPA 对应物理内存属性必须 X+R 两种权限都满足（`norm:hlsv_u_op`），实际测试中通过 PMP 配置全 RWX 来满足。

---

### Group 14：mstatus.MPRV+MPV 触发的两阶段

**规范依据**：
- `norm:mstatus_mprv_hypervisor`：M-mode 下 `mstatus.MPRV=1` 时，显式内存访问按 `mstatus.MPV` 与 `mstatus.MPP` 决定的虚拟化模式与特权级翻译
- h-mprv 表：MPRV=1 + MPV=1 + MPP=0 → VU-level 两阶段；MPRV=1 + MPV=1 + MPP=1 → VS-level 两阶段；MPP=3 (M) → 无翻译
- `norm:mstatus_mprv_hlsv`：MPRV 不影响 HLV/HLVX/HSV（这些指令始终按 V=1 + SPVP 行事）

**测试职责**：验证 M-mode 下 MPRV+MPV 组合触发的两阶段翻译，覆盖 h-mprv 表的关键行。

| 测试 ID | 测试名称 | MPRV | MPV | MPP | 测试描述 | 预期结果 |
|---------|----------|------|-----|-----|----------|----------|
| TS-MPRV-01 | MPRV=0 → 无翻译 | 0 | 1 | 1 | M-mode 普通 ld 访问，MPRV=0 | 直接物理访问，不触发两阶段 |
| TS-MPRV-02 | MPRV=1 + MPV=1 + MPP=S → VS-level 两阶段 | 1 | 1 | S | 两阶段映射就绪，M-mode 普通 ld 访问 guest VA | 经两阶段翻译，访问到 SPA |
| TS-MPRV-03 | MPRV=1 + MPV=1 + MPP=U → VU-level 两阶段 | 1 | 1 | U | 两阶段映射 U=1，M-mode 普通 ld | 经两阶段以 VU 视角翻译；U=0 PTE 触发 fault |
| TS-MPRV-04 | MPRV=1 + MPP=M → 无翻译 | 1 | x | M | M-mode 普通 ld，MPP=M | 直接物理访问（与 MPV 无关） |
| TS-MPRV-05 | MPRV 不影响 HLV | 1 | 0 | M | M-mode 设 MPRV=1 + MPV=0，但执行 HLV.D | HLV 仍按 V=1 + SPVP 翻译，与 MPRV/MPV/MPP 无关 |

```c
/* TS-MPRV-02：M-mode MPRV=1+MPV=1+MPP=S 触发 VS-level 两阶段 */
TEST_REGISTER(test_mprv_mpv_vs_level);
bool test_mprv_mpv_vs_level(void) {
    TEST_BEGIN("TS-MPRV-02: MPRV=1+MPV=1+MPP=S -> VS-level two-stage access");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    /* 两阶段恒等映射，目标页使用非恒等以验证翻译 */
    uintptr_t code_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    pt_setup_identity_mapping(&ctx.vs_ctx, code_base, PAGE_SIZE_1G,
                              PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D, PT_LEVEL_1G);
    gpt_setup_identity_mapping(&ctx.g_ctx, code_base, PAGE_SIZE_1G,
                               PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D,
                               PT_LEVEL_1G);

    /* 测试 VA：VS-stage 把 VA → GPA_X，G-stage 把 GPA_X → SPA_Y */
    uintptr_t va  = TEST_REGION_BASE;
    uintptr_t gpa = TEST_REGION_BASE + PAGE_SIZE_4K;
    uintptr_t spa = TEST_REGION_BASE + 2 * PAGE_SIZE_4K;
    pt_map_page(&ctx.vs_ctx, va, gpa,
                PTE_V|PTE_R|PTE_W|PTE_A|PTE_D, PT_LEVEL_4K);
    gpt_map_page(&ctx.g_ctx, gpa, spa,
                 PTE_V|PTE_R|PTE_W|PTE_U|PTE_A|PTE_D, PT_LEVEL_4K);

    /* 物理预填 SPA */
    *(volatile uint64_t *)spa = 0xCAFEBABE12345678ULL;

    /* 启用两阶段 CSR */
    gpt_enable(&ctx.g_ctx, /*vmid=*/0);
    CSRW(vsatp, MAKE_SATP(SATP_MODE_SV39, /*asid=*/0,
                          ((uintptr_t)ctx.vs_ctx.root_pt) >> 12));

    /* M-mode 设置 MPRV=1, MPV=1, MPP=S
     *
     * 字段常量来源：
     *   MSTATUS_MPRV_BIT - common/encoding.h:149（已存在）
     *   MSTATUS_MPP_OFF/MASK - common/encoding.h:147-148（已存在）
     *   MSTATUS_MPV_BIT - 由 hyp_csr 模块新增（hypervisor_framework.md 837
     *                      已规划 mstatus 扩展字段：MPV/GVA）
     * 写入 MPP=S（值 1）：先清 MASK 再 OR (1 << OFF) */
    CSRS(mstatus, MSTATUS_MPRV_BIT | MSTATUS_MPV_BIT);
    CSRC(mstatus, MSTATUS_MPP_MASK);
    CSRS(mstatus, (1ULL << MSTATUS_MPP_OFF));  /* MPP = S = 1 */

    /* M-mode 执行普通 ld va，应被两阶段翻译到 SPA */
    uint64_t val = *(volatile uint64_t *)va;

    /* 恢复 MPRV/MPV，避免影响后续访问 */
    CSRC(mstatus, MSTATUS_MPRV_BIT | MSTATUS_MPV_BIT);

    TEST_ASSERT_EQ("Two-stage translation took effect",
                   val, 0xCAFEBABE12345678ULL);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

> [!WARNING]
> TS-MPRV 系列测试需特别注意：M-mode 设置 MPRV=1 后，**M-mode 自身**的所有显式 load/store 都会经过翻译，包括栈访问、trap handler 内的 CSR 读写。务必在最小代码块内启用 MPRV 并在执行完目标访问后立即清除，避免误伤栈 / 局部变量访问。建议使用 inline asm 严格控制 MPRV 启用窗口。

### Group 15：A/D 位两阶段处理

**规范依据**：
- `norm:henvcfg_adue_op`：ADUE=0 时 VS-stage 行为如同 Svade（A/D=0 触发 page-fault 而非硬件更新）；ADUE=1 时硬件可自动更新 VS-stage PTE A/D
- `norm:H_vm_gpapriv`：支持 VS-stage 翻译的隐式内存访问（包括硬件更新 A/D 的隐式 store），按 implicit load/store 检查 G-stage 权限和 A/D 位

**测试职责**：验证 henvcfg.ADUE 对 VS-stage A/D 位硬件更新的控制效果，以及 G-stage 对隐式写 A/D 的权限拦截行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-AD-01 | ADUE=0 时 VS-stage A=0 | henvcfg.ADUE=0，VS-stage PTE A=0，VS-mode load | page-fault (cause=13)，硬件不自动更新 A 位 |
| TS-AD-02 | ADUE=1 时 VS-stage A=0 正常 | henvcfg.ADUE=1，VS-stage PTE A=0，G-stage 对 PTE GPA 映射 RWU | 访问成功，VS-stage PTE A 位被硬件自动设为 1 |
| TS-AD-03 | ADUE=1 时 A 位更新被 G-stage 只读拦截 | henvcfg.ADUE=1，VS-stage PTE A=0，G-stage 对 PTE GPA 映射只读 U | cause=23（HW A/D 实现）或 cause=21（无 HW A/D 实现），两种均合规通过 |
| TS-AD-04 | ADUE=1 时 D 位更新被 G-stage 只读拦截 | henvcfg.ADUE=1，VS-stage PTE A=1,D=0，VS-mode store，G-stage 对 PTE GPA 映射只读 U | guest-page-fault (cause=23)，htinst=`0x00003020`（write pseudoinst） |
| TS-AD-05 | G-stage PTE A=0 | G-stage PTE A=0（其余权限正常 RWXU），VS-mode load | guest-page-fault (cause=21) |
| TS-AD-06 | G-stage PTE D=0 + store | G-stage PTE D=0（R=1,W=1,U=1,A=1），VS-mode store | guest-page-fault (cause=23) |

---

### Group 16：页边界跨越（Straddle）

**规范依据**：
- `norm:H_straddle`：取指或未对齐内存访问跨越页边界时涉及两个地址翻译；guest-page-fault 时 stval 可能是页边界地址
- `norm:mtval2_htval_virtaddr`：非隐式访问的 guest-page-fault 时，htval 中的非零 GPA 必须对应 stval 指向的确切虚拟地址

**测试职责**：验证跨页边界访问在两阶段翻译下的 fault 行为和 htval/stval 精度。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-STRD-01 | Load 跨页，第二页 G-stage 未映射 | 4 字节 load 起始于 page_end-2，第一页两阶段正常，第二页 G-stage 无效 | guest-page-fault (cause=21)，stval=第二页起始 GVA，htval=第二页 GPA>>2 |
| TS-STRD-02 | Fetch 跨页，第二页 G-stage 无 X | 2 字节压缩指令起始于页尾倒数第 1 字节，第二页 G-stage X=0 | inst guest-page-fault (cause=20)，stval=第二页起始 GVA |
| TS-STRD-03 | Store 跨页，第二页 VS-stage 无 W | 4 字节 store 跨页，第二页 VS-stage PTE W=0 | store page-fault (cause=15)，stval=原始 VA |

---

### Group 17：G-stage PTE G 位忽略

**规范依据**：
- `norm:H_vm_gpa_g`：G-stage PTE 中的 G 位当前未使用，软件应清零以保持前向兼容，硬件必须忽略

**测试职责**：验证 G-stage PTE 中 G 位被设置时不影响翻译行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-GBIT-01 | G-stage PTE G=1 不影响翻译 | G-stage PTE 设置 G=1，其余权限正常（V=1,R=1,W=1,X=1,U=1,A=1,D=1） | 两阶段翻译正常成功，G 位被硬件忽略 |

---

### Group 18：henvcfg.PBMTE 对 VS-stage 的控制

**规范依据**：
- `norm:henvcfg_pbmte_op`：PBMTE=0 时行为如同 Svpbmt 未实现（VS-stage PTE 的 PBMT 字段 bit[62:61] 为 reserved，非零触发异常）；PBMTE=1 时 Svpbmt 可用于 VS-stage 地址翻译

**测试职责**：验证 PBMTE 开关对 VS-stage PTE PBMT 字段的影响。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-PBMT-01 | PBMTE=0 时 PBMT 非零触发 fault | henvcfg.PBMTE=0，VS-stage PTE bit[62:61]=01 (NC) | page-fault（reserved bits 非法） |
| TS-PBMT-02 | PBMTE=1 时 PBMT=NC 正常翻译 | henvcfg.PBMTE=1，VS-stage PTE bit[62:61]=01 (NC)，其余权限正常 | 翻译成功 |

---

### Group 19：PMP 与两阶段的交互

**规范依据**：
- `norm:H_pmp`：Machine-level physical memory protection applies to supervisor physical addresses and is in effect regardless of virtualization mode.

**测试职责**：验证两阶段翻译成功后，最终 SPA 仍受 PMP 约束。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-PMP-01 | SPA 落在 PMP 拒绝区域 | 两阶段翻译均成功，最终 SPA 被 PMP 配置为不可访问 | access-fault (cause=5 load / 7 store)，非 page-fault 也非 guest-page-fault |

---

### Group 20：异常优先级

**规范依据**：
- `norm:H_exception_priority`：多个同步异常可能同时触发时，按规范优先级递减顺序决定报告哪个

**测试职责**：验证两阶段翻译场景下异常优先级的正确性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-PRIO-01 | 未对齐 + VS-stage fault 优先级 | VS-mode 对未对齐地址 load，同时 VS-stage PTE V=0 | page-fault (cause=13) 优先于 misaligned（按规范优先级表） |
| TS-PRIO-02 | G-stage 隐式访问拦截优先于 VS-stage PTE 检查 | VS-stage 根页表 GPA 被 G-stage 标记无效 + VS-stage PTE 本身也是非法编码 | guest-page-fault（G-stage 隐式读根表先被拦截，VS-stage PTE 内容未及检查） |

---

### Group 21：hgatp 根页表对齐与 WARL 行为

**规范依据**：
- `norm:hgatp_ppn_op`：G-stage 根页表为 16 KiB 且必须对齐到 16 KiB 边界；PPN 最低两位始终读为零
- `norm:hgatp_mode_warl`：写入不支持的 MODE 值不会像 satp 那样被忽略，而是按 WARL 方式处理

**测试职责**：验证 hgatp CSR 的对齐约束和 WARL 语义。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-HGATP-01 | hgatp.PPN[1:0] 强制为零 | 写 hgatp.PPN 最低 2 位为 1，读回 | PPN[1:0] 始终读为 0（16 KiB 对齐 WARL 强制约束，spec 明确要求；不合规则 FAIL） |
| TS-HGATP-02 | hgatp MODE WARL | 写 hgatp.MODE 为不支持的值（如保留编码） | WARL 行为（字段被调整，不报异常；与 vsatp 的"忽略"语义不同） |

---

### Group 22：Svinval 指令异常行为

**规范依据**：
- `norm:hstatus_vtvm_op`（原文）：「When VTVM=1, an attempt in VS-mode to execute SFENCE.VMA or **SINVAL.VMA** or to access CSR `satp` raises a virtual-instruction exception.」
- `norm:mstatus_tvm_hs`（原文）：「Setting TVM=1 prevents HS-mode from accessing `hgatp` or executing HFENCE.GVMA or **HINVAL.GVMA**...」

**测试职责**：验证 Svinval 扩展指令在两阶段场景下的异常触发行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TS-SINV-01 | VTVM=1 时 VS 执行 SINVAL.VMA | hstatus.VTVM=1，VS-mode 执行 `sinval.vma` | virtual-instruction exception (cause=22) |
| TS-SINV-02 | TVM=1 时 HS 执行 HINVAL.GVMA | mstatus.TVM=1，HS-mode 执行 `hinval.gvma` | illegal-instruction exception (cause=2) |

---

### Group 23：大页面粒度两阶段组合

**规范依据**：
- `norm:H_vm_twostage`：两阶段翻译时，VS-stage 和 G-stage 的叶节点可以是任意受支持的 superpage 粒度
- VS-stage 支持的最大叶节点粒度由 vsatp MODE 决定：Sv39 最大 1G (level 2)，Sv48 最大 512G (level 3)，Sv57 最大 256T (level 4)
- G-stage 支持的最大叶节点粒度由 hgatp MODE 决定：Sv39x4 最大 1G (level 2)，Sv48x4 最大 512G (level 3)，Sv57x4 最大 256T (level 4)
- 两阶段翻译的有效页面大小取 VS-stage 和 G-stage 叶节点粒度中较小的那个

**测试职责**：验证 VS-stage 和 G-stage 使用不同大 superpage 粒度（1G/512G/256T）时两阶段翻译的正确性，补充 Group 3 中仅覆盖到 4K/2M（以及部分 1G）的不足。

| 测试 ID | 测试名称 | VS 粒度 | G 粒度 | VS-stage | G-stage | 测试描述 | 预期结果 |
|---------|----------|---------|--------|---------|--------|----------|----------|
| TS-LP-01 | VS=1G G=4K 恒等映射 | 1G | 4K | Sv39+ | Sv39x4+ | VS-stage 1GB superpage，G-stage 4KB 逐页映射 | R/W 成功 |
| TS-LP-02 | VS=1G G=2M 恒等映射 | 1G | 2M | Sv39+ | Sv39x4+ | VS-stage 1GB superpage，G-stage 2MB superpage | R/W 成功 |
| TS-LP-03 | VS=1G G=1G 恒等映射 | 1G | 1G | Sv39+ | Sv39x4+ | 两阶段均 1GB superpage | R/W 成功 |
| TS-LP-04 | VS=4K G=512G 恒等映射 | 4K | 512G | Sv39+ | Sv48x4+ | G-stage 单个 512GB superpage 覆盖全部物理内存 | R/W 成功 |
| TS-LP-05 | VS=2M G=512G 恒等映射 | 2M | 512G | Sv39+ | Sv48x4+ | VS-stage 2MB + G-stage 512GB superpage | R/W 成功 |
| TS-LP-06 | VS=1G G=512G 恒等映射 | 1G | 512G | Sv39+ | Sv48x4+ | VS-stage 1GB + G-stage 512GB superpage | R/W 成功 |
| TS-LP-07 | VS=4K G=256T 恒等映射 | 4K | 256T | Sv39+ | Sv57x4 | G-stage 单个 256TB superpage 覆盖全部物理内存 | R/W 成功 |
| TS-LP-08 | VS=2M G=256T 恒等映射 | 2M | 256T | Sv39+ | Sv57x4 | VS-stage 2MB + G-stage 256TB superpage | R/W 成功 |
| TS-LP-09 | VS=512G G=4K 恒等映射 | 512G | 4K | Sv48+ | Sv39x4+ | VS-stage 512GB superpage，G-stage 4KB 逐页映射 | R/W 成功 |
| TS-LP-10 | VS=256T G=4K 恒等映射 | 256T | 4K | Sv57 | Sv39x4+ | VS-stage 256TB superpage，G-stage 4KB 逐页映射 | R/W 成功 |

> [!NOTE]
> - TS-LP-04~08（G=512G/256T）：G-stage 使用 `gpt_map_page(gpa=0, spa=0, level=PT_LEVEL_512G/256T)` 建立单个 superpage 恒等映射覆盖 [0, 512G) 或 [0, 256T)。因 PLATFORM_MEM_BASE（QEMU 0x80000000 / HAPS 0x60000000）均在此范围内，映射可行。
> - TS-LP-09/10（VS=512G/256T）：VS-stage 使用 `pt_map_page(va=0, gpa=0, level=PT_LEVEL_512G/256T)` 建立单个 superpage 恒等映射。需要 `page_table.c` 的 `page_size_for_level()` 扩展以支持 level 3/4。
> - 所有用例采用 identity 映射（VA=GPA=SPA），成功标准为 VS-mode 对 `test_data_area` 的读写返回正确值。
> - 512G/256T superpage 需要对应的 hgatp/vsatp MODE 支持：512G 需要 Sv48x4+ 或 Sv48+ vsatp；256T 需要 Sv57x4 或 Sv57 vsatp。不支持时通过 `REQUIRE_HGATP_MODE` / `REQUIRE_VSATP_MODE` 宏自动 SKIP。

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1、Group 3、Group 6、Group 7、Group 15 | TS-VS-01~10、TS-MAP-01~12、TS-IMPL-01~06、TS-PERM-01~12、TS-AD-01~06 | VS-stage 基线 + 同位宽两阶段 + 隐式 G-stage fault + 权限交叉 + A/D 位处理 |
| P1（重要） | Group 8、Group 9、Group 10、Group 11、Group 13、Group 16、Group 19 | TS-MXR-01~05、TS-SUM-01~03、TS-HV-01~06、TS-HG-01~06、TS-HLV-01~12、TS-STRD-01~03、TS-PMP-01~02 | MXR/SUM 双阶段语义、HFENCE 刷新、HLV/HSV、页边界跨越、PMP 交互 |
| P2（建议） | Group 2、Group 5、Group 12、Group 14、Group 17、Group 18、Group 20、Group 21、Group 22 | TS-VSATP-01~07、TS-NID-01~04、TS-SF-01~04、TS-MPRV-01~05、TS-GBIT-01、TS-PBMT-01~02、TS-PRIO-01~02、TS-HGATP-01~02、TS-SINV-01~02 | vsatp CSR、非恒等映射、SFENCE V=1、MPRV、G bit、PBMTE、异常优先级、hgatp WARL、Svinval 异常 |
| P3（可选） | Group 4、Group 23 | TS-XMODE-01~09、TS-LP-01~10 | 异位宽组合 + 大页面粒度组合（兼容性验证） |

---

## 测试实现说明

### 文件组织（9 目录拆分布局）

两阶段（VS+G）测试用例从原 `Sv39x4/`、`Sv48x4/`、`Sv57x4/` 三个 G-stage 目录中剥离出来，按 (VS-mode, G-mode) 笛卡尔积单独建立 **9 个目录**，每个目录独立测试一种 (VS, G) 组合，并完整遍历该组合下支持的所有页面粒度。

采用「主目录持有 + 其他借用」模式：

- **主目录** `Sv39_Sv39x4/`：物理持有全部 22 个 group test `.c` 文件 + 1 个新增 `test_granular_matrix.c` 笛卡尔积驱动文件。
- **8 个借用目录**：`Sv39_Sv48x4/`、`Sv39_Sv57x4/`、`Sv48_Sv39x4/`、`Sv48_Sv48x4/`、`Sv48_Sv57x4/`、`Sv57_Sv39x4/`、`Sv57_Sv48x4/`、`Sv57_Sv57x4/`。每个目录通过 Makefile `CFLAGS += -I../Sv39_Sv39x4/tests` 借用源码，仅在自身 `tests/test_register.c` 中定制 `SUITE_VSATP_MODE` / `SUITE_HGATP_MODE` 宏。
- 原三个 G-stage 目录（`Sv39x4/`、`Sv48x4/`、`Sv57x4/`）退化为**纯 G-stage 测试目录**（保留 11 个 G-stage group：HCSR/ROOT/MAP/HIGH/VALID/RWX/UBIT/AD/ALIGN/GBIT/FAULT），不再承担两阶段测试。

```
damo-priv-test/
├── Sv39x4/  Sv48x4/  Sv57x4/    # 纯 G-stage 测试（清理 two_stage 后）
│
├── Sv39_Sv39x4/                 # 【主目录】持有 22 个 two_stage/*.c
│   ├── Makefile                 #   ENABLE_TWO_STAGE=1, ENABLE_HYP=1
│   ├── kernel.ld                #   16KB 对齐根表段（同 Sv39x4）
│   ├── main.c
│   └── tests/
│       ├── test_helpers.h       # forward header（仅转发框架头）
│       ├── test_register.c      # 22 个 two_stage/*.c #include + SUITE 宏
│       └── two_stage/           # 22 个原始 group + test_granular_matrix.c
│
├── Sv39_Sv48x4/  ...  Sv57_Sv57x4/   # 8 个借用目录
│   ├── Makefile                 #   CFLAGS += -I../Sv39_Sv39x4/tests
│   ├── kernel.ld                #   与主目录完全一致
│   ├── main.c                   #   仅 banner 不同
│   └── tests/
│       └── test_register.c      # 22 个 #include + 定制 SUITE 宏
│
└── common/hyp/                  # 框架层（仅含通用 helper，禁测试逻辑）
```

#### 9 目录映射表

| 目录 | SUITE_VSATP_MODE | SUITE_HGATP_MODE | 笛卡尔积粒度数 |
|---|---|---|---|
| `Sv39_Sv39x4` *(主)* | `SATP_MODE_SV39` | `HGATP_MODE_SV39X4` | 3 × 3 = **9** |
| `Sv39_Sv48x4` | `SATP_MODE_SV39` | `HGATP_MODE_SV48X4` | 3 × 4 = **12** |
| `Sv39_Sv57x4` | `SATP_MODE_SV39` | `HGATP_MODE_SV57X4` | 3 × 5 = **15** |
| `Sv48_Sv39x4` | `SATP_MODE_SV48` | `HGATP_MODE_SV39X4` | 4 × 3 = **12** |
| `Sv48_Sv48x4` | `SATP_MODE_SV48` | `HGATP_MODE_SV48X4` | 4 × 4 = **16** |
| `Sv48_Sv57x4` | `SATP_MODE_SV48` | `HGATP_MODE_SV57X4` | 4 × 5 = **20** |
| `Sv57_Sv39x4` | `SATP_MODE_SV57` | `HGATP_MODE_SV39X4` | 5 × 3 = **15** |
| `Sv57_Sv48x4` | `SATP_MODE_SV57` | `HGATP_MODE_SV48X4` | 5 × 4 = **20** |
| `Sv57_Sv57x4` | `SATP_MODE_SV57` | `HGATP_MODE_SV57X4` | 5 × 5 = **25** |
| **合计粒度组合** | | | **144** |

#### VS×G 粒度矩阵（每目录完整遍历）

每个目录均跑 `test_granular_matrix.c` 中的 25 行 `MATRIX_CASE`（覆盖最大集 5×5），每条受 `vs_max_level(SUITE_VSATP_MODE)` / `g_max_level(SUITE_HGATP_MODE)` 边界自动 SKIP——目录内合法子集 PASS、超出边界 SKIP。

- **VS levels**：Sv39 → {4K, 2M, 1G}；Sv48 → {4K, 2M, 1G, 512G}；Sv57 → {4K, 2M, 1G, 512G, 256T}
- **G levels**：Sv39x4 → {4K, 2M, 1G}；Sv48x4 → {4K, 2M, 1G, 512G}；Sv57x4 → {4K, 2M, 1G, 512G, 256T}
- **矩阵驱动**：基于框架 helper `vs_max_level()` / `g_max_level()` / `PAGE_SIZE_AT_LEVEL()`（`common/hyp/two_stage_helpers.h`）。
- **底层 API**：每个 case 调用 `ts2_setup_granular(ctx, vs_mode, g_mode, vs_level, g_level)` 后 `ts2_run_check_no_fault(ctx, test_vs_read_write, va)` 验证 R/W。

#### 22 Group 在 9 目录的分布

所有 9 个目录均包含全部 22 个 group test 文件（共 ≈139 个 `TEST_REGISTER`）+ 1 个 `test_granular_matrix.c` 文件（25 个 `TEST_REGISTER`）。每条用例内部通过 `REQUIRE_VSATP_MODE` / `REQUIRE_HGATP_MODE` 在不匹配的目录中自动 SKIP，因此用例集合**对所有目录是同源的**，目录间差异仅由 `SUITE_*MODE` 宏控制运行时分支。

| Group | 文件 | TEST 数 | 主测 (VS, G) | 在其他目录的行为 |
|---|---|---|---|---|
| Group 1  (VS-only) | `test_vs_only.c` | 10 | 由 SUITE 决定 | 全跑 |
| Group 2  (VSATP CSR) | `test_vsatp_csr.c` | 7 | 全 mode | 全跑 |
| Group 3  (Same width) | `test_same_width.c` | 12 | VS=G 对角 | 非对角 SKIP |
| Group 4  (Cross width) | `test_cross_width.c` | 21 | 异位宽组合 | 不匹配的 SKIP |
| Group 5  (Non-identity) | `test_non_identity.c` | 4 | 由 SUITE | 全跑 |
| Group 6  (Implicit fault) | `test_implicit_gstage_fault.c` | 4 | 由 SUITE | 全跑 |
| Group 7  (Perm cross) | `test_perm_cross.c` | 12 | 由 SUITE | 全跑 |
| Group 8  (MXR) | `test_mxr.c` | 5 | 由 SUITE | 全跑 |
| Group 9  (SUM) | `test_sum.c` | 3 | 由 SUITE | 全跑 |
| Group 10 (HFENCE.VVMA) | `test_hfence_vvma.c` | 6 | 由 SUITE | 全跑 |
| Group 11 (HFENCE.GVMA) | `test_hfence_gvma.c` | 6 | 由 SUITE | 全跑 |
| Group 12 (SFENCE.VMA) | `test_sfence_vma.c` | 4 | 由 SUITE | 全跑 |
| Group 13 (HLV/HSV) | `test_hlv_hsv.c` | 12 | 由 SUITE | 全跑 |
| Group 14 (MPRV+MPV) | `test_mprv_mpv.c` | 5 | 由 SUITE | 全跑 |
| Group 15 (A/D bits) | `test_ad_two_stage.c` | 6 | 由 SUITE | 全跑 |
| Group 16 (Page straddle) | `test_page_straddle.c` | 3 | 由 SUITE | 全跑 |
| Group 17 (G bit) | `test_g_bit.c` | 1 | 由 SUITE | 全跑 |
| Group 18 (PBMTE) | `test_pbmte.c` | 2 | 由 SUITE | 全跑 |
| Group 19 (PMP) | `test_pmp.c` | 1 | 由 SUITE | 全跑 |
| Group 20 (Priority) | `test_priority.c` | 2 | 由 SUITE | 全跑 |
| Group 21 (HGATP WARL) | `test_hgatp_warl.c` | 2 | 全 G-mode | 全跑 |
| Group 22 (Svinval) | `test_svinval.c` | 2 | 由 SUITE | 全跑 |
| Group 23 (Large page) | `test_large_page.c` | 10 | 大页粒度 | 不匹配 SKIP |
| **新增 (Granular matrix)** | `test_granular_matrix.c` | 25 | VS×G 全笛卡尔积 | 超出 (vs_max, g_max) SKIP |
| **合计** | 23 个文件 | **≈164** | | |

#### 框架层最小增量

`common/hyp/two_stage_helpers.h` 新增（**仅 helper，无任何 `TEST_REGISTER` 或断言**）：

- `vs_max_level(int vs_mode)` — Sv39→1G / Sv48→512G / Sv57→256T
- `g_max_level(int g_mode)` — Sv39x4→1G / Sv48x4→512G / Sv57x4→256T
- `PAGE_SIZE_AT_LEVEL(level)` 宏 — 4K/2M/1G/512G/256T → 字节大小

已有但本次重构关键依赖：`ts2_setup_granular()`、`ts2_map_region_g()`、`ts2_map_region_vs()`、`ts2_run_check_no_fault()`、`ts2_finish()`，均已 mode-agnostic（接受 `vs_mode`/`g_mode` 参数），无需改造。


### 通用测试模式

#### 模式 1：VS-mode 下两阶段翻译（Group 1/3/4/5/6/7/8/9）

```c
TEST_REGISTER(test_xxx);
bool test_xxx(void) {
    TEST_BEGIN("ID: description");

    two_stage_ctx_t ctx;
    pt_pool_reset();
    gpt_pool_reset();
    two_stage_init(&ctx, SATP_MODE_SV*, HGATP_MODE_SV*X4);

    /* 1. 代码 + 数据区两阶段恒等映射 */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D;
    two_stage_setup_identity(&ctx, base, PAGE_SIZE_1G, flags, PT_LEVEL_1G);

    /* 2. 可选：测试页特殊权限映射 */
    /* pt_map_page(&ctx.vs_ctx, ...) 与 gpt_map_page(&ctx.g_ctx, ...) */

    /* 3. 进入 VS-mode 执行测试函数 */
    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_xxx, arg);
    TEST_ASSERT("description", result == expected);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

#### 模式 2：HFENCE / SFENCE 测试（Group 10/11/12）

```c
TEST_REGISTER(test_fence_xxx);
bool test_fence_xxx(void) {
    TEST_BEGIN("ID: fence semantics");

    /* setup ctx 并启用两阶段 */
    /* HS-mode 修改 PTE */
    /* 执行 HFENCE.VVMA / HFENCE.GVMA / SFENCE.VMA */
    hfence_vvma(0, 0);  /* 或对应的 fence 调用 */
    /* VS-mode 验证新 PTE 生效 */
    HYP_TEST_END();
}
```

#### 模式 3：HLV/HSV 测试（Group 13）

```c
TEST_REGISTER(test_hlv_xxx);
bool test_hlv_xxx(void) {
    TEST_BEGIN("ID: HLV/HSV behavior");

    /* setup 两阶段映射 */
    gpt_enable(&ctx.g_ctx, vmid);
    CSRW(vsatp, MAKE_SATP(SATP_MODE_SV39, /*asid=*/0,
                          ((uintptr_t)ctx.vs_ctx.root_pt) >> 12));
    hstatus_set_spvp(1);  /* 或 0 */

    goto_priv(PRIV_S);    /* HS-mode */
    uint64_t val = hlv_d(guest_va);
    goto_priv(PRIV_M);

    TEST_ASSERT_EQ("HLV.D returned expected value", val, expected);
    HYP_TEST_END();
}
```

#### 模式 4：MPRV+MPV 测试（Group 14）

```c
TEST_REGISTER(test_mprv_xxx);
bool test_mprv_xxx(void) {
    TEST_BEGIN("ID: MPRV+MPV two-stage");

    /* setup 两阶段映射 + 启用 vsatp/hgatp */
    /* 严格控制 MPRV 启用窗口（最小化 M-mode 自身被翻译的代码段） */
    CSRS(mstatus, MSTATUS_MPRV_BIT | MSTATUS_MPV_BIT);
    /* ... 执行最小化的目标访问 ... */
    CSRC(mstatus, MSTATUS_MPRV_BIT | MSTATUS_MPV_BIT);

    HYP_TEST_END();
}
```

### VS-mode 测试辅助函数

| 函数名 | 功能 | 返回值 |
|--------|------|--------|
| `test_vs_read_write` | VS-mode 写入 magic value 并读回验证 | 0=成功 |
| `test_vs_load` | VS-mode 执行 load | 0=成功 |
| `test_vs_store` | VS-mode 执行 store | 0=成功 |
| `test_vs_load_expect_fault` | VS-mode load，预期 fault | fault cause |
| `test_vs_store_expect_fault` | VS-mode store，预期 fault | fault cause |
| `test_vs_exec_expect_fault` | VS-mode 跳转执行，预期 fault | fault cause |

### 关键注意事项

1. **fault cause 区分阶段来源**：
   - VS-stage 失败 → cause 12/13/15（普通 page-fault）
   - G-stage 失败 → cause 20/21/23（guest-page-fault）
   - 测试断言必须使用准确的 cause 常量，不能模糊处理

2. **htval 的双重含义**（`norm:htval_trapval`）：
   - 显式访问 fault：htval = 原始 GPA >> 2，与 stval 对应同一访问
   - 隐式 VS-stage 访问 fault：htval = VS-level PTE 的 GPA >> 2，与 stval 不对应同一地址
   - 通过 htinst 判断是否为隐式访问

3. **htinst 伪指令编码**（`norm:H_trap_xtinst_guestpage_rw`）：
   - RV64 隐式 read：`0x00003000`
   - RV64 隐式 write（A/D 自动更新）：`0x00003020`
   - 当 mtval2/htval 非零且为隐式 VS-stage 访问时**必须**写 pseudoinst（不允许 0）

4. **MXR 双重语义**（`norm:vsstatus_mxr_vm`、`norm:sstatus_mxr_vm`）：
   - HS-level `sstatus.MXR` 同时影响 VS-stage 与 G-stage
   - `vsstatus.MXR` 仅影响 VS-stage
   - 测试时需要分别设置/清除两个 MXR 字段以验证差异

5. **SUM 仅作用于 VS-stage**：
   - `vsstatus.SUM` 控制 VS-stage 的 U-bit 检查
   - G-stage 始终 U-mode 视角，无 SUM 概念
   - HLV/HSV 时 HS-level `sstatus.SUM` 被忽略（`norm:hlsv_trans`）

6. **VS-stage A 位推测限制**（`norm:vs_stage_speculative_a_bit`）：
   - VS-stage A 位不得因 mispredict 而被设置（除非真正在 VS/VU-mode）
   - 这是难以直接测试的微架构行为，在 QEMU 上一般无法可观察验证。本计划不包含直接验证用例，仅在文档中说明该约束

7. **世界切换顺序**（参考 `docs/hypervisor_framework.md`）：
   - 切换 VMID 时严格顺序：vsatp=0 → 写 hgatp → 写 vsatp
   - 避免推测执行污染 TLB 标记

8. **HLV/HSV 在 V=1 时触发 virtual-instruction exception**（cause=22），不是 illegal-instruction

9. **MPRV 不影响 HLV/HSV**（`norm:mstatus_mprv_hlsv`）：HLV/HSV 始终按 V=1 + SPVP 行事

10. **MPRV+MPV 测试的栈安全**：M-mode 设置 MPRV=1 后所有 load/store 都被翻译，包括栈访问。务必在最小窗口启用 MPRV 并使用 inline asm 严格控制（详见 Group 14 注意事项）

11. **QEMU 平台限制**：
    - 需要 `-cpu rv64,h=true`（或更新版本默认开启 H 扩展）
    - 512GB / 256TB 大 superpage 受物理内存限制
    - 部分实现可能不支持 Sv57/Sv57x4，应在测试前探测

---

## 参考

- `SPEC/hypervisor.adoc` — RISC-V Hypervisor Extension, Version 1.0
- `docs/gstage_translation_test_plan.md` — G-stage 独立翻译测试计划（配套）
- `docs/vm_test_plan.md` — VS-stage / 普通 VM 测试计划（行为基线）
- `docs/hypervisor_framework.md` — Hypervisor 测试框架设计
- `common/hyp/two_stage.c` — 两阶段管理 API
- `common/hyp/gstage_pt.c` — G-stage 页表管理 API
- `common/hyp/hyp_ldst.c` — HLV/HLVX/HSV 指令封装
- `common/hyp/hyp_fence.c` — HFENCE.VVMA/GVMA 指令封装
- `common/hyp/hyp_csr.c` — Hypervisor CSR 操作 API
