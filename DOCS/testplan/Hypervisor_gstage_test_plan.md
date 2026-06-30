**中文 | [English](../testplan_en/Hypervisor_gstage_test_plan_en.md)**

# RISC-V G-stage 地址翻译测试计划

本文档定义 RISC-V Hypervisor 扩展中 **G-stage**（第二阶段）地址翻译的测试计划，覆盖 Sv39x4/Sv48x4/Sv57x4 翻译算法、`hgatp` CSR、16KB 根页表、GPA 高位检查、U-bit 始终生效、guest-page-fault 报告等核心规范点。

> **范围说明**：本计划仅覆盖 **G-stage 独立翻译**（即 `vsatp.MODE = Bare`，`hgatp.MODE = Sv39x4 / Sv48x4 / Sv57x4`），目的是在 VS-stage 透传的简化场景下验证 Sv*x4 算法本身。涉及 VS-stage 与 G-stage 联合行为（完整两阶段链路、HFENCE、HLV/HSV、`mstatus.MPRV+MPV` 等）的测试位于配套文档 `docs/two_stage_translation_test_plan.md`。

> **注意**：当前仓库为 RV64，**Sv32x4 不在本计划范围内**。

---

## 适用范围

本计划在 V=1 且 `vsatp.MODE = Bare`（VS-stage 透传，GVA = GPA）的前提下，验证以下三种 G-stage 翻译模式：

- **Sv39x4**：3 级页表，41 位 GPA，支持 4KB / 2MB megapage / 1GB gigapage
- **Sv48x4**：4 级页表，50 位 GPA，支持 4KB / 2MB / 1GB / 512GB terapage
- **Sv57x4**：5 级页表，59 位 GPA，支持 4KB / 2MB / 1GB / 512GB / 256TB petapage

各模式相对于对应 Sv39/Sv48/Sv57 的关键差异：

- 根页表大小由 4KB 扩展为 **16KB**（4 个连续 4KB 页），且根页表地址必须 **16KB 对齐**
- 输入地址（GPA）比对应 Sv 模式宽 **2 bit**
- U-bit **始终作为 U-mode 检查**（`norm:H_vm_gpapriv`）
- 翻译失败触发 **guest-page-fault**（cause 20 / 21 / 23）而非普通 page-fault（12 / 13 / 15）
- ID 字段为 **VMID** 而非 ASID

---

## 规范引用

- `SPEC/hypervisor.adoc` — "H" Extension for Hypervisor Support, Version 1.0
  - Hypervisor Guest Address Translation and Protection (`hgatp`) Register
  - Two-Stage Address Translation
  - Guest Physical Address Translation
  - Guest-Page Faults
  - Hypervisor Trap Value (`htval`) Register
  - Hypervisor Trap Instruction (`htinst`) Register
  - Trap Cause Codes


## 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:hgatp_sz_acc_op` | The `hgatp` register is an HSXLEN-bit read/write register which controls G-stage address translation and protection, the second stage of two-stage translation for guest virtual addresses. | `hgatp` 是一个 HSXLEN 位读写寄存器，控制 G 阶段地址翻译和保护，即客户虚拟地址两阶段翻译的第二阶段。 |
| `norm:hgatp_mode_bare` | When MODE=Bare, guest physical addresses are equal to supervisor physical addresses, and there is no further memory protection for a guest virtual machine beyond the physical memory protection scheme. In this case, software must write zero to the remaining fields in `hgatp`. | 当 MODE=Bare 时，客户物理地址等于 supervisor 物理地址，无额外内存保护。此时软件必须将 `hgatp` 的其余字段写为零。 |
| `norm:hgatp_mode_sv` | When HSXLEN=32, the only other valid setting for MODE is Sv32x4. When HSXLEN=64, modes Sv39x4, Sv48x4, and Sv57x4 are defined. | 当 HSXLEN=32 时，MODE 的唯一其他有效设置为 Sv32x4。当 HSXLEN=64 时，定义了 Sv39x4、Sv48x4 和 Sv57x4 模式。 |
| `norm:hgatp_mode_warl` | A write to `hgatp` with an unsupported MODE value is not ignored as it is for `satp`. Instead, the fields of `hgatp` are WARL in the normal way, when so indicated. | 使用不支持的 MODE 值写入 `hgatp` 不会像 `satp` 那样被忽略。`hgatp` 的字段按常规 WARL 方式处理。 |
| `norm:hgatp_ppn_op` | For the paged virtual-memory schemes, the root page table is 16 KiB and must be aligned to a 16-KiB boundary. In these modes, the lowest two bits of the physical page number (PPN) in `hgatp` always read as zeros. | 对于分页虚拟内存方案，根页表为 16 KiB 且必须对齐到 16 KiB 边界。这些模式下 `hgatp` 中 PPN 的最低两位始终读为零。 |
| `norm:hgatp_vmid` | The number of VMID bits is UNSPECIFIED and may be zero. | VMID 位数未指定，可以为零。 |
| `norm:hgatp_vmid_lsbs` | The least-significant bits of VMID are implemented first: that is, if VMIDLEN > 0, VMID[VMIDLEN-1:0] is writable. The maximal value of VMIDLEN, termed VMIDMAX, is 7 for Sv32x4 or 14 for Sv39x4, Sv48x4, and Sv57x4. | VMID 的最低有效位先实现。VMIDMAX 对于 Sv32x4 为 7，对于 Sv39x4/Sv48x4/Sv57x4 为 14。 |
| `norm:hgatp_mode_sv39x4` | For Sv39x4, partitioning is identical to Sv39, except with 2 more bits at the high end in VPN[2]. Address bits 63:41 must all be zeros, or else a guest-page-fault exception occurs. | Sv39x4 的分区与 Sv39 相同，但 VPN[2] 高端多 2 位。地址位 63:41 必须全为零，否则发生客户页错误。 |
| `norm:hgatp_mode_sv48x4` | For Sv48x4, partitioning is identical to Sv48, except with 2 more bits at the high end in VPN[3]. Address bits 63:50 must all be zeros, or else a guest-page-fault exception occurs. | Sv48x4 的分区与 Sv48 相同，但 VPN[3] 高端多 2 位。地址位 63:50 必须全为零，否则发生客户页错误。 |
| `norm:hgatp_mode_sv57x4` | For Sv57x4, partitioning is identical to Sv57, except with 2 more bits at the high end in VPN[4]. Address bits 63:59 must all be zeros, or else a guest-page-fault exception occurs. | Sv57x4 的分区与 Sv57 相同，但 VPN[4] 高端多 2 位。地址位 63:59 必须全为零，否则发生客户页错误。 |
| `norm:hgatp_mode_x4` | When `hgatp`.MODE specifies a translation scheme of Sv32x4, Sv39x4, Sv48x4, or Sv57x4, G-stage address translation is a variation on the usual page-based scheme. In each case, the size of the incoming address is widened by 2 bits. The root page table is expanded by a factor of 4 to be 16 KiB and must be aligned to a 16 KiB boundary. | 当 `hgatp`.MODE 指定 Sv32x4/Sv39x4/Sv48x4/Sv57x4 时，G 阶段翻译是常规方案的变体。地址宽度增加 2 位，根页表扩大 4 倍为 16 KiB，必须对齐到 16 KiB 边界。 |
| `norm:H_vm_gpatrans` | The conversion of an Sv32x4, Sv39x4, Sv48x4, or Sv57x4 guest physical address uses the same algorithm as Sv32, Sv39, Sv48, or Sv57, except: `hgatp` substitutes for `satp`; the effective privilege mode must be VS-mode or VU-mode; the current privilege mode is always taken to be U-mode when checking the U bit; and guest-page-fault exceptions are raised instead of regular page-fault exceptions. | Sv32x4/Sv39x4/Sv48x4/Sv57x4 客户物理地址转换使用与 Sv32/Sv39/Sv48/Sv57 相同的算法，但有以下区别：`hgatp` 替代 `satp`；有效特权模式须为 VS/VU；检查 U 位时始终视为 U 模式；引发客户页错误而非普通页错误。 |
| `norm:H_vm_gpapriv` | For G-stage address translation, all memory accesses are considered to be user-level accesses. Access type permissions are checked during G-stage translation the same as for VS-stage. For memory accesses supporting VS-stage translation, permissions and A/D bit needs are checked as though for an implicit load or store, not for the original access type. However, any exception is always reported for the original access type. | G 阶段地址翻译中，所有内存访问视为用户级访问。访问权限检查方式与 VS 阶段相同。支持 VS 阶段翻译的内存访问按隐式 load/store 检查权限和 A/D 位需求，但异常始终按原始访问类型报告。 |
| `norm:H_vm_gpa_g` | The G bit in all G-stage PTEs is currently not used. It should be cleared by software for forward compatibility, and must be ignored by hardware. | G 阶段 PTE 中的 G 位当前未使用。软件应清零以保持前向兼容，硬件必须忽略。 |
| `norm:H_guest_page_fault` | Guest-page-fault traps may be delegated from M-mode to HS-mode under the control of `medeleg`, but cannot be delegated to other privilege modes. On a guest-page fault, `mtval` or `stval` is written with the faulting guest virtual address, and `mtval2` or `htval` is written either with zero or with the faulting guest physical address, shifted right by 2 bits. | 客户页错误可通过 `medeleg` 从 M 模式委托给 HS 模式，但不能委托给其他特权模式。客户页错误时 `mtval`/`stval` 写入故障客户虚拟地址，`mtval2`/`htval` 写入零或故障客户物理地址右移 2 位。 |
| `norm:htval_trapval` | When a guest-page-fault trap is taken into HS-mode, `htval` is written with either zero or the guest physical address that faulted, shifted right by 2 bits. For other traps, `htval` is set to zero. | 客户页错误陷阱进入 HS 模式时，`htval` 写入零或故障的客户物理地址右移 2 位。其他陷阱时 `htval` 设为零。 |
| `norm:mtval2_htval_virtaddr` | When a guest-page fault is not due to an implicit memory access for VS-stage address translation, a nonzero guest physical address written to `mtval2`/`htval` shall correspond to the exact virtual address written to `mtval`/`stval`. | 当客户页错误不是由 VS 阶段地址翻译的隐式内存访问引起时，写入 `mtval2`/`htval` 的非零客户物理地址必须对应写入 `mtval`/`stval` 的确切虚拟地址。 |
| `norm:H_trap_xtinst_guestpage` | For guest-page faults, the trap instruction register is written with a special pseudoinstruction value if: (a) the fault is caused by an implicit memory access for VS-stage address translation, and (b) a nonzero value is written to `mtval2` or `htval`. If both conditions are met, zero is not allowed. | 对于客户页错误，若 (a) 故障由 VS 阶段地址翻译的隐式内存访问引起，且 (b) `mtval2`/`htval` 写入非零值，则陷阱指令寄存器必须写入特殊伪指令值，不允许为零。 |
| `norm:H_trap_xtinst_guestpage_rw` | A write pseudoinstruction (0x00002020 or 0x00003020) is used for the case that the machine is attempting automatically to update bits A and/or D in VS-level page tables. All other implicit memory accesses for VS-stage address translation will be reads. | 写伪指令（0x00002020 或 0x00003020）用于机器自动更新 VS 级页表 A/D 位的情况。所有其他 VS 阶段翻译的隐式内存访问均为读取。 |
| `norm:hgatp_tvm_illegal` | When `mstatus`.TVM=1, attempts to read or write `hgatp` while executing in HS-mode will raise an illegal-instruction exception. | 当 `mstatus`.TVM=1 时，在 HS 模式下尝试读写 `hgatp` 将引发非法指令异常。 |
| `norm:hstatus_gva_op` | Field GVA (Guest Virtual Address) is written by the implementation whenever a trap is taken into HS-mode. For any trap that writes a guest virtual address to `stval`, GVA is set to 1. For any other trap into HS-mode, GVA is set to 0. | GVA 字段在进入 HS 模式的陷阱时由实现写入。写入客户虚拟地址到 `stval` 的陷阱设置 GVA=1，其他陷阱设置 GVA=0。 |

---

## 测试组定义

### Group 1：hgatp CSR 字段验证

**规范依据**：
- `norm:hgatp_sz_acc_op`：`hgatp` 是 HSXLEN-bit 读写寄存器，含 MODE / VMID / PPN 三个字段
- `norm:hgatp_mode_warl`：写入 `hgatp` 时**不支持的 MODE 不被忽略**（与 `satp` 不同），按 WARL 规则处理
- `norm:hgatp_ppn_op`：使用 Sv*x4 模式时 PPN[1:0] 始终读为 0；实现可使其只读为 0
- `norm:hgatp_vmid`、`norm:hgatp_vmid_lsbs`：VMIDLEN 由实现决定，最大 14（Sv39x4/48x4/57x4）
- `norm:hgatp_mode_bare`：MODE=Bare 时其他字段必须为零，软件如未清零行为 UNSPECIFIED
- `norm:hgatp_tvm_illegal`：`mstatus.TVM=1` 时 HS-mode 访问 `hgatp` 触发 illegal-instruction exception

**测试职责**：验证 `hgatp` CSR 的字段读写、WARL 行为、TVM 控制。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| GHCSR-01 | MODE=Bare 写读 | M-mode 写入 `hgatp` MODE=0、VMID=0、PPN=0 | 读回值匹配 |
| GHCSR-02 | MODE=Sv39x4 写读 | M-mode 写入 MODE=8（Sv39x4）+ 合法 PPN | 读回值匹配 |
| GHCSR-03 | MODE=Sv48x4 写读 | M-mode 写入 MODE=9（Sv48x4） | 读回值匹配 |
| GHCSR-04 | MODE=Sv57x4 写读 | M-mode 写入 MODE=10（Sv57x4） | 读回值匹配 |
| GHCSR-05 | 不支持的 MODE 按 WARL 规则处理 | 先写入合法 MODE=Sv39x4 并读回原始值；再写入保留 MODE=2，读回 hgatp 验证 MODE 字段未被静默接受为 2 | 与 `satp`（写无效 MODE 整体被忽略）不同：hgatp.MODE 字段按 WARL 单独决定（要么保留旧合法值，要么自动调整为某个合法值），写入操作本身不被忽略 |
| GHCSR-06 | PPN[1:0] 强制为 0 | 通过 `MAKE_HGATP(SV39X4, 0, ppn_with_low2_bits_set)` 写入 hgatp，读回提取 PPN 字段 | 读回 PPN[1:0]=0 |
| GHCSR-07 | VMIDLEN 探测与回写 | 向 VMID 字段写入全 1，读回判断 VMIDLEN；再用 0..VMIDMAX 范围内合法 VMID 写读 | VMIDLEN ≤ 14；合法 VMID 可写读 |
| GHCSR-08 | TVM=1 时 HS-mode 访问 hgatp | M-mode 设置 `mstatus.TVM=1`，切换到 HS-mode 后 `csrr` 读 `hgatp` | illegal-instruction exception (`scause`=2)；M-mode 访问不受影响 |
| GHCSR-09 | MODE=Bare 透传验证 | M-mode 写入 `hgatp` MODE=Bare，VS-mode 访问任意 GPA | GPA = SPA，无翻译无保护，访问成功 |

```c
/* GHCSR-06 示例：PPN[1:0] 强制为零 */
TEST_REGISTER(test_hgatp_ppn_low2_ro_zero);
bool test_hgatp_ppn_low2_ro_zero(void) {
    TEST_BEGIN("GHCSR-06: hgatp PPN[1:0] reads as zero in Sv*x4 modes");

    /* 构造一个 PPN 低 2 位非零的值，借助 MAKE_HGATP 写入。
     * MAKE_HGATP 由 docs/hypervisor_framework.md 定义（编码 mode/vmid/ppn 字段）。
     * CSRR/CSRW 由 common/encoding.h 提供（编译期 CSR 名）。 */
    uintptr_t test_ppn = (PLATFORM_MEM_BASE >> 12) | 0x3;
    uintptr_t hgatp_val = MAKE_HGATP(HGATP_MODE_SV39X4, /*vmid=*/0, test_ppn);
    CSRW(hgatp, hgatp_val);

    /* 读回并提取 PPN 字段（HGATP_PPN_MASK 由 hypervisor_framework.md 定义，
     * 对应 hgatp 寄存器 PPN 字段 bits 43:0 in HSXLEN=64） */
    uintptr_t read_back = CSRR(hgatp);
    uintptr_t ppn_field = read_back & HGATP_PPN_MASK;
    TEST_ASSERT("PPN[1:0] reads zero", (ppn_field & 0x3) == 0);

    /* 恢复 hgatp，避免影响后续测试 */
    CSRW(hgatp, 0);

    HYP_TEST_END();
}
```

---

### Group 2：根页表 16KB 对齐

**规范依据**：
- `norm:hgatp_mode_x4`：Sv32x4/39x4/48x4/57x4 的根页表为 **16KB**，必须 **16KB 对齐**
- `norm:hgatp_ppn_op`：低 2 位 PPN 在 Sv*x4 模式下被强制为 0，从而保证 16KB 对齐

**测试职责**：验证根页表的 16KB 大小与对齐要求，验证不对齐根页表的实现行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| GROOT-01 | Sv39x4 16KB 对齐根表正常工作 | 在 G-stage 页表池中分配 16KB 对齐根表（验证 `((uintptr_t)root_pt & 0x3FFF) == 0`），启用 Sv39x4 后访问已映射 GPA | 根表对齐符合要求；翻译成功 |
| GROOT-02 | Sv48x4 16KB 对齐根表正常工作 | 同 GROOT-01 但模式为 Sv48x4。Sv48x4 与 Sv39x4 共享 16KB 对齐要求（`norm:hgatp_mode_x4`），本用例覆盖该模式下框架确实分配了 16KB 对齐根表 | 翻译成功 |
| GROOT-03 | Sv57x4 16KB 对齐根表正常工作 | 同 GROOT-01 但模式为 Sv57x4 | 翻译成功 |
| GROOT-04 | hgatp.PPN[1:0] 永远读回 0 | 通过 `MAKE_HGATP` 写入 PPN bits 1:0 = 0b11 后读回 hgatp，再启用翻译访问已映射 GPA | hgatp.PPN[1:0] 读回 0；翻译使用对齐后的 PPN，访问成功 |

> [!NOTE]
> 不存在"4KB 对齐根表能否工作"的标准化测试用例 —— `hgatp_ppn_op` 已通过 PPN[1:0] 强制为 0 在硬件层面保证 16KB 对齐，软件无法构造一个真正的"低 2 位非零"的根页表地址写入 hgatp。

```c
/* GROOT-01 示例：Sv39x4 16KB 对齐根表 */
TEST_REGISTER(test_sv39x4_root_pt_16kb_aligned);
bool test_sv39x4_root_pt_16kb_aligned(void) {
    TEST_BEGIN("GROOT-01: Sv39x4 16KB-aligned root page table works");

    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, /*vs=Bare*/0, HGATP_MODE_SV39X4);

    /* gpt_init 内部分配 16KB 对齐的根表 */
    TEST_ASSERT("root PT is 16KB aligned",
                ((uintptr_t)ctx.g_ctx.root_pt & 0x3FFF) == 0);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D;
    int ret = two_stage_setup_identity(&ctx, base, PAGE_SIZE_1G,
                                       flags, PT_LEVEL_1G);
    TEST_ASSERT("identity mapping setup", ret == 0);

    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_read_write,
                                           (uintptr_t)test_data_area);
    TEST_ASSERT("translation through 16KB root works", result == 0);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

---

### Group 3：Sv39x4 基础映射

**规范依据**：
- `norm:hgatp_mode_sv39x4`：Sv39x4 是 Sv39 的 2-bit 扩展，41-bit GPA
- `norm:H_vm_gpatrans`：使用与 Sv39 相同的翻译算法，仅替换 satp → hgatp、effective privilege 限定 VS/VU、U-bit 视角始终为 U、fault 类型变为 guest-page-fault

**测试职责**：在 Bare VS-stage 的前提下，验证 Sv39x4 在三种页大小（1GB / 2MB / 4KB）下的 GPA → SPA 翻译。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| G39-MAP-01 | Sv39x4 1GB gigapage 映射 | 建立 1GB GPA→SPA 恒等映射，VS-mode 读写 | 读写成功，数据一致 |
| G39-MAP-02 | Sv39x4 2MB megapage 映射 | 建立 2MB 恒等映射，VS-mode 读写 | 读写成功 |
| G39-MAP-03 | Sv39x4 4KB page 映射 | 建立 4KB 恒等映射，VS-mode 读写 | 读写成功 |

```c
/* G39-MAP-03 示例：Sv39x4 4KB GPA→SPA 恒等映射 */
TEST_REGISTER(test_sv39x4_4k_identity);
bool test_sv39x4_4k_identity(void) {
    TEST_BEGIN("G39-MAP-03: Sv39x4 4KB GPA->SPA identity mapping");

    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, /*vs=Bare*/0, HGATP_MODE_SV39X4);

    /* G-stage 必须 U=1，因为 G-stage 视所有访问为 U-mode */
    uintptr_t base = PLATFORM_MEM_BASE;
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X
                    | PTE_U
                    | PTE_A | PTE_D;
    int ret = two_stage_setup_identity(&ctx, base, 4 * PAGE_SIZE_2M,
                                       flags, PT_LEVEL_4K);
    TEST_ASSERT("G-stage identity setup", ret == 0);

    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_read_write,
                                           (uintptr_t)test_data_area);
    TEST_ASSERT("VS-mode read/write via G-stage 4KB", result == 0);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

---

### Group 4：Sv48x4 基础映射

**规范依据**：
- `norm:hgatp_mode_sv48x4`：Sv48x4 是 Sv48 的 2-bit 扩展，50-bit GPA

**测试职责**：验证 Sv48x4 在四种页大小下的 GPA → SPA 翻译。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| G48-MAP-01 | Sv48x4 512GB terapage 映射 | 建立 512GB 恒等映射 | 读写成功（受平台物理内存限制） |
| G48-MAP-02 | Sv48x4 1GB gigapage 映射 | 建立 1GB 恒等映射 | 读写成功 |
| G48-MAP-03 | Sv48x4 2MB megapage 映射 | 建立 2MB 恒等映射 | 读写成功 |
| G48-MAP-04 | Sv48x4 4KB page 映射 | 建立 4KB 恒等映射 | 读写成功 |

> [!WARNING]
> G48-MAP-01（512GB terapage）在 QEMU virt 平台上可能因物理内存大小限制无法完整覆盖整个 512GB 区间，应仅访问位于实际物理内存范围内的子区间。

---

### Group 5：Sv57x4 基础映射

**规范依据**：
- `norm:hgatp_mode_sv57x4`：Sv57x4 是 Sv57 的 2-bit 扩展，59-bit GPA

**测试职责**：验证 Sv57x4 的基础翻译（256TB petapage 因物理内存限制不做完整覆盖）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| G57-MAP-01 | Sv57x4 1GB gigapage 映射 | 建立 1GB 恒等映射 | 读写成功 |
| G57-MAP-02 | Sv57x4 2MB megapage 映射 | 建立 2MB 恒等映射 | 读写成功 |
| G57-MAP-03 | Sv57x4 4KB page 映射 | 建立 4KB 恒等映射 | 读写成功 |

> [!NOTE]
> 256TB petapage 与 512GB terapage 测试受 QEMU 物理内存限制，建议在专用大内存配置下补充。

---

### Group 6：GPA 高位检查（必须为零）

**规范依据**：
- `norm:hgatp_mode_sv39x4`：Sv39x4 GPA 的 bits 63:41 必须全为零，否则触发 guest-page-fault
- `norm:hgatp_mode_sv48x4`：Sv48x4 GPA 的 bits 63:50 必须全为零
- `norm:hgatp_mode_sv57x4`：Sv57x4 GPA 的 bits 63:59 必须全为零

**测试职责**：验证 GPA 高位非零时正确触发 guest-page-fault。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| GHIGH-01 | Sv39x4 bit 41 非零 | 访问 GPA = `1UL << 41`（仅最低非法位置位，其余高位为 0） | load guest-page-fault (cause=21) |
| GHIGH-02 | Sv39x4 bit 63 非零 | 访问 GPA = `1UL << 63`（最高位置位，验证 bits 63:41 高端） | guest-page-fault |
| GHIGH-03 | Sv48x4 bit 50 非零 | 访问 GPA = `1UL << 50` | guest-page-fault |
| GHIGH-04 | Sv48x4 bit 63 非零 | 访问 GPA = `1UL << 63` | guest-page-fault |
| GHIGH-05 | Sv57x4 bit 59 非零 | 访问 GPA = `1UL << 59` | guest-page-fault |
| GHIGH-06 | Sv57x4 bit 63 非零 | 访问 GPA = `1UL << 63` | guest-page-fault |

```c
/* GHIGH-01 示例：Sv39x4 GPA bits 63:41 必须为零 */
TEST_REGISTER(test_sv39x4_gpa_high_bits_fault);
bool test_sv39x4_gpa_high_bits_fault(void) {
    TEST_BEGIN("GHIGH-01: Sv39x4 GPA bit 41 nonzero -> guest-page-fault");

    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, /*vs=Bare*/0, HGATP_MODE_SV39X4);

    /* 仅映射代码区用于 VS-mode 执行 */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    gpt_setup_identity_mapping(&ctx.g_ctx, base, PAGE_SIZE_1G,
                               PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D,
                               PT_LEVEL_1G);

    /* 构造非法 GPA：bit 41 为 1（41-bit GPA 范围之外）*/
    uintptr_t bad_gpa = 1UL << 41;
    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_load_expect_fault,
                                           bad_gpa);
    TEST_ASSERT("Sv39x4 GPA bit 41 triggers guest-page-fault",
                result == CAUSE_LOAD_GUEST_PAGE_FAULT);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

---

### Group 7：PTE 有效性检查

**规范依据**（沿用 Sv 算法步骤 3，但 fault 类型不同）：
- `pte.v = 0` → guest-page-fault
- `pte.r = 0 && pte.w = 1` → guest-page-fault（保留编码）
- `norm:H_vm_gpatrans`：失败时触发 guest-page-fault（cause 20/21/23）而非 page-fault

**测试职责**：验证 G-stage PTE 无效时的 guest-page-fault 行为，确认 cause code 与普通 page-fault 区分。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| GVALID-01 | V=0 PTE（load） | G-stage 数据测试页 V=0，VS-mode 执行 `ld` | load guest-page-fault (cause=21) |
| GVALID-02 | V=0 PTE（store） | 同 GVALID-01，VS-mode 执行 `sd` | store guest-page-fault (cause=23) |
| GVALID-03 | V=0 PTE（fetch） | 在另一个独立的"跳转目标"GPA 范围映射 V=0；VS-mode 跳转到该 GPA 执行 fetch（保留代码区合法映射用于触发指令本身的执行） | inst guest-page-fault (cause=20) |
| GVALID-04 | R=0,W=1 保留编码（load） | G-stage 数据测试页 V=1,R=0,W=1,X=0 | load guest-page-fault |
| GVALID-05 | R=0,W=1 保留编码（store） | 同上，VS-mode store | store guest-page-fault |

---

### Group 8：RWX 权限组合

**规范依据**：
- `norm:H_vm_gpatrans`：访问类型权限（R/W/X）按与 Sv 相同的方式检查
- `norm:H_vm_gpapriv`：所有 G-stage 访问视为 U-mode

**测试职责**：验证 G-stage PTE 的 RWX 权限组合在 VS-mode 访问下的行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| GRWX-01 | R=1,W=0,X=0 仅读 | load 成功，store/fetch 失败 | store/fetch guest-page-fault |
| GRWX-02 | R=1,W=1,X=0 RW | load/store 成功，fetch 失败 | fetch guest-page-fault |
| GRWX-03 | R=1,W=0,X=1 RX | load/fetch 成功，store 失败 | store guest-page-fault |
| GRWX-04 | R=1,W=1,X=1 RWX | 全部成功 | 无 fault |
| GRWX-05 | R=0,W=0,X=1 仅 X（execute-only） | fetch 成功，load 失败（HS-level `sstatus.MXR=0`） | load guest-page-fault |
| GRWX-06 | R=0,W=1,X=1 保留编码 | 任意访问失败 | guest-page-fault |
| GRWX-07 | R=W=X=0 出现在最低级 PTE | R=W=X=0 是非叶 PTE（指针）。在最低级 PTE 上出现该编码意味着算法尝试越级遍历，超出 LEVELS 范围 | guest-page-fault（依据 Sv 算法步骤 4：越级即触发 page-fault） |

---

### Group 9：U-bit 始终作为 U-mode 检查（G-stage 关键差异）

**规范依据**：
- `norm:H_vm_gpapriv`：「For G-stage address translation, all memory accesses (including those made to access data structures for VS-stage address translation) are considered to be user-level accesses, as though executed in U-mode.」

**测试职责**：验证 G-stage PTE 的 U-bit 必须为 1，否则即使 effective privilege 是 VS-mode 也会触发 guest-page-fault。这是 G-stage 与 VS-stage 最关键的差异之一。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| GUBIT-01 | VS-mode 访问 U=0 G-stage 页（load） | G-stage 映射 U=0，VS-mode（nominal S）load | load guest-page-fault |
| GUBIT-02 | VS-mode 访问 U=0 G-stage 页（store） | 同上 store | store guest-page-fault |
| GUBIT-03 | VS-mode 访问 U=0 G-stage 页（fetch） | 同上 fetch | inst guest-page-fault |
| GUBIT-04 | VU-mode 访问 U=1 G-stage 页 | G-stage U=1，通过 framework 的 `run_in_vu_mode` 切到 VU-mode 访问 | 成功 |
| GUBIT-05 | VS-mode 访问 U=1 G-stage 页 | G-stage U=1，VS-mode（`run_in_vs_mode` / `two_stage_run_in_vs`）访问 | 成功（G-stage 视角恒为 U-mode，U=1 总是允许） |

```c
/* GUBIT-01 示例：G-stage U=0 触发 guest-page-fault */
TEST_REGISTER(test_gstage_u0_faults);
bool test_gstage_u0_faults(void) {
    TEST_BEGIN("GUBIT-01: G-stage U=0 always faults (treated as U-mode)");

    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, /*vs=Bare*/0, HGATP_MODE_SV39X4);

    /* 代码区映射 U=1 用于执行 */
    uintptr_t code_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    gpt_setup_identity_mapping(&ctx.g_ctx, code_base, PAGE_SIZE_1G,
                               PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D,
                               PT_LEVEL_1G);

    /* 测试页 U=0 —— G-stage 视为 U-mode 访问，必触发 fault */
    uintptr_t test_gpa = TEST_REGION_BASE;
    gpt_map_page(&ctx.g_ctx, test_gpa, test_gpa,
                 PTE_V|PTE_R|PTE_W|PTE_A|PTE_D, /* U=0 */
                 PT_LEVEL_4K);

    /* 在 VS-mode (nominal S) 访问 U=0 的 G-stage 页 */
    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_load_expect_fault,
                                           test_gpa);
    TEST_ASSERT("U=0 in G-stage triggers guest-page-fault from VS",
                result == CAUSE_LOAD_GUEST_PAGE_FAULT);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

---

### Group 10：A/D 位管理

**规范依据**：
- `norm:H_vm_gpatrans`：「permissions and the need to set A and/or D bits at the G-stage level are checked as though for an implicit load or store, not for the original access type. However, any exception is always reported for the original access type.」
- 默认（未实现 Svadu）：A=0 或访问类型为 store 而 D=0 时触发 guest-page-fault

**测试职责**：验证 G-stage A/D 位的检查与 fault cause 的报告逻辑。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| GAD-01 | A=0 触发 load guest-page-fault | G-stage PTE A=0,D=1，VS-mode load | load guest-page-fault (cause=21) |
| GAD-02 | A=0 触发 store guest-page-fault | G-stage PTE A=0,D=1，VS-mode store | store guest-page-fault (cause=23) |
| GAD-03 | D=0 触发 store guest-page-fault | G-stage PTE A=1,D=0，VS-mode store | store guest-page-fault (cause=23) |
| GAD-04 | A=1,D=1 正常访问 | G-stage PTE A=1,D=1，VS-mode RW | 无 fault |

> [!NOTE]
> A/D 位的 fault cause 始终按"原始访问类型"报告，但权限检查按 implicit load/store 的逻辑。详见规范 `norm:H_vm_gpatrans`。

---

### Group 11：Superpage 对齐验证

**规范依据**：
- 沿用 Sv39/Sv48/Sv57 的 superpage 对齐规则（`pte.ppn[i-1:0]` 必须为零，否则 misaligned superpage）
- `norm:H_vm_gpatrans`：失败时触发 guest-page-fault

**测试职责**：验证 G-stage superpage PTE 的 ppn 低位必须按级别清零。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| GALIGN-01 | Sv39x4 1GB superpage misaligned | 1GB PTE 的 ppn[1:0] 非零 | guest-page-fault |
| GALIGN-02 | Sv39x4 2MB superpage misaligned | 2MB PTE 的 ppn[0] 非零 | guest-page-fault |
| GALIGN-03 | Sv48x4 512GB superpage misaligned | 512GB PTE 的 ppn[2:0] 非零 | guest-page-fault |
| GALIGN-04 | Sv57x4 256TB superpage misaligned | 256TB PTE 的 ppn[3:0] 非零 | guest-page-fault |
| GALIGN-05 | 对齐 superpage 正常工作 | 各级 superpage 正确对齐 | 无 fault |

---

### Group 12：G-bit 忽略验证

**规范依据**：
- `norm:H_vm_gpa_g`：「The G bit in all G-stage PTEs is currently not used. Until its use is defined by a standard extension, it should be cleared by software for forward compatibility, and must be ignored by hardware.」

**测试职责**：验证 G-stage PTE 的 G-bit 被硬件忽略。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| GGBIT-01 | G=1 不影响翻译 | G-stage PTE 设置 G=1，访问该页 | 翻译成功，无异常 |
| GGBIT-02 | G=0 与 G=1 行为相同 | 同一映射 G=0 和 G=1 的访问对比 | 行为一致 |

---

### Group 13：guest-page-fault 报告（htval / htinst / GVA）

**规范依据**：
- `norm:H_guest_page_fault`：guest-page-fault 时 `stval` 写入 faulting GVA、`htval` 写入 GPA>>2 或 0
- `norm:htval_trapval`、`norm:mtval2_htval_virtaddr`：非隐式访问时 `htval` 与 `stval` 对应同一访问；GPA 右移 2 bit 后存入
- `norm:H_trap_xtinst_guestpage`：guest-page-fault 时 `htinst` 可能为 0 或 transformed 指令；隐式 VS-stage 访问时使用 pseudoinstruction
- `norm:hstatus_gva_op`：`hstatus.GVA` 在 trap 入 HS-mode 时根据是否写入 GVA 至 stval 设置
- 在本组（G-stage 独立、VS-stage Bare）中 GVA = GPA，不存在 VS-stage 隐式访问

**测试职责**：验证 G-stage fault 时 htval / htinst / hstatus.GVA / stval 的写入。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| GFAULT-01 | load fault cause=21 | 触发 load guest-page-fault | scause=21 |
| GFAULT-02 | store fault cause=23 | 触发 store guest-page-fault | scause=23 |
| GFAULT-03 | inst fault cause=20 | 触发 inst guest-page-fault | scause=20 |
| GFAULT-04 | htval = GPA>>2 | 显式 load fault，检查 htval | htval == faulting GPA >> 2 |
| GFAULT-05 | stval = GVA | 显式 load fault，检查 stval | stval == faulting GVA（在 VS-stage Bare 下 GVA=GPA） |
| GFAULT-06 | hstatus.GVA = 1 | guest-page-fault 写 GVA 到 stval | hstatus.GVA == 1 |
| GFAULT-07 | htinst transformed for load | load fault 的 htinst 为 transformed load 或 0 | htinst 符合 `norm:H_trap_xtinst_exception_list` |
| GFAULT-08 | inst guest-page-fault 时 htinst | instruction guest-page-fault 触发后检查 htinst | 0（依据 tinst-values：inst guest-page-fault 不允许写 transformed standard instruction；本组无 VS-stage 隐式访问，故也不会是 pseudoinstruction，因此实现只能写 0 或 custom，对标准指令场景应为 0） |

> [!NOTE]
> `norm:H_trap_xtinst_guestpage_rw` 中的 **write pseudoinstruction**（0x00002020 / 0x00003020）场景仅在 VS-stage 隐式访问更新 A/D 位时触发 G-stage fault 才会出现，属于两阶段联合行为，由 `docs/two_stage_translation_test_plan.md` 覆盖。本组（VS-stage Bare）不存在 VS-stage 隐式访问，因此仅验证 read pseudoinstruction / transformed instruction / 0 的情况。

```c
/* GFAULT-04 示例：htval = GPA >> 2 */
TEST_REGISTER(test_gstage_htval_gpa_shifted);
bool test_gstage_htval_gpa_shifted(void) {
    TEST_BEGIN("GFAULT-04: htval reports faulting GPA shifted right by 2");

    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, /*vs=Bare*/0, HGATP_MODE_SV39X4);

    uintptr_t code_base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    gpt_setup_identity_mapping(&ctx.g_ctx, code_base, PAGE_SIZE_1G,
                               PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D,
                               PT_LEVEL_1G);

    /* 测试 GPA 未映射 */
    uintptr_t bad_gpa = 0x80000000UL + 0x10000000UL;  /* 未映射区域 */

    trap_expect_begin();
    two_stage_run_in_vs(&ctx, test_vs_load, bad_gpa);
    TEST_ASSERT("guest-page-fault triggered", trap_was_triggered());
    TEST_ASSERT_EQ("cause is load guest-page-fault",
                   trap_get_cause(), CAUSE_LOAD_GUEST_PAGE_FAULT);
    CHECK_HTVAL("htval = GPA >> 2", bad_gpa >> 2);
    trap_expect_end();

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 3、Group 7、Group 8、Group 9 | G39-MAP-01~03、GVALID-01~05、GRWX-01~07、GUBIT-01~05 | Sv39x4 核心算法 + PTE 有效性 + RWX + U-bit 关键差异 |
| P1（重要） | Group 1、Group 2、Group 6、Group 10、Group 13 | GHCSR-01~08、GROOT-01~04、GHIGH-01~06、GAD-01~04、GFAULT-01~08 | hgatp CSR、根表对齐、GPA 高位、A/D 位、fault 报告 |
| P2（建议） | Group 4、Group 5、Group 11 | G48-MAP-01~04、G57-MAP-01~03、GALIGN-01~05 | Sv48x4/Sv57x4 扩展模式、superpage 对齐 |
| P3（可选） | Group 12 | GGBIT-01~02 | G-bit 忽略验证 |

---

## 测试实现说明

### 文件组织

每个 G-stage 模式（Sv39x4 / Sv48x4 / Sv57x4）拥有独立的测试目录，与现有 `sv39/sv48/sv57` 命名风格一致。本计划与 `two_stage_translation_test_plan.md` 共享同一套目录与框架代码：

```
sv39x4/
├── Makefile
├── kernel.ld
└── main.c              # G-stage 独立 + 同位宽两阶段用例

sv48x4/
├── Makefile
├── kernel.ld
└── main.c              # 同上

sv57x4/
├── Makefile
├── kernel.ld
└── main.c              # 同上

common/hyp/             # Hypervisor 测试框架（依据 docs/hypervisor_framework.md）
├── hyp_defs.h
├── hyp_csr.c
├── hyp_priv.c
├── hyp_trap.c
├── hyp_trap_asm.S
├── hyp_fence.c
├── hyp_ldst.c
├── gstage_pt.c         # G-stage 页表管理 (gpt_*)
├── two_stage.c         # 两阶段管理 (two_stage_*)
├── hyp_reset.c
└── hyp_test.h
```

### 通用测试模式

每个 G-stage 测试用例遵循以下模式：

```c
#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_test.h"
#include "vm/vm.h"

/* VS-mode 下执行的测试函数 */
static uintptr_t test_vs_xxx(uintptr_t arg) {
    /* 在 VS-mode + G-stage 启用状态下执行 */
    return 0;
}

TEST_REGISTER(test_xxx);
bool test_xxx(void) {
    TEST_BEGIN("ID: description");

    /* 1. 初始化 G-stage 池与上下文 */
    two_stage_ctx_t ctx;
    gpt_pool_reset();
    two_stage_init(&ctx, /*vs=Bare*/0, HGATP_MODE_SV39X4);

    /* 2. 设置代码 + 数据区的 G-stage 恒等映射（U=1 必须） */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D;
    two_stage_setup_identity(&ctx, base, PAGE_SIZE_1G, flags, PT_LEVEL_1G);

    /* 3. 可选：映射特殊权限的测试页 */
    uintptr_t test_gpa = TEST_REGION_BASE;
    gpt_map_page(&ctx.g_ctx, test_gpa, test_gpa,
                 PTE_V|PTE_R|PTE_A|PTE_D|PTE_U,  /* 特定权限 */
                 PT_LEVEL_4K);

    /* 4. 在 VS-mode + G-stage 下执行 */
    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_xxx, arg);
    TEST_ASSERT("description", result == expected);

    /* 5. 清理 */
    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

### VS-mode 测试辅助函数

测试计划中引用的 VS-mode 辅助函数：

| 函数名 | 功能 | 返回值 |
|--------|------|--------|
| `test_vs_read_write` | VS-mode 写入 magic value 并读回验证 | 0=成功 |
| `test_vs_load` | VS-mode 执行 load | 0=成功 |
| `test_vs_store` | VS-mode 执行 store | 0=成功 |
| `test_vs_load_expect_fault` | VS-mode load，预期 fault | fault cause |
| `test_vs_store_expect_fault` | VS-mode store，预期 fault | fault cause |
| `test_vs_exec_expect_fault` | VS-mode 跳转执行，预期 fault | fault cause |

### 关键注意事项

1. **G-stage PTE 必须设置 U=1**：与 VS-stage 不同，G-stage 视所有访问为 U-mode（`norm:H_vm_gpapriv`）。即使从 VS-mode 触发的访问，若 G-stage PTE 的 U=0 也会触发 guest-page-fault。

2. **fault cause 区分**：G-stage fault 使用 cause 20/21/23（Instruction/Load/Store guest-page fault），与普通 page-fault 的 12/13/15 严格区分。测试断言必须使用正确的 cause 常量。

3. **htval 编码**：`htval` 始终写入 GPA 右移 2 bit 后的值（与 PMP / PTE PPN 编码一致）。低 2 bit 信息可从 `stval` 的低 2 bit 还原（非隐式访问场景）。

4. **hgatp 不支持 MODE 不被忽略**：与 `satp` 的 WARL 行为不同，写入 `hgatp` 的不支持 MODE 不会被静默丢弃，软件需要读回校验（`norm:hgatp_mode_warl`）。

5. **TVM 控制**：`mstatus.TVM=1` 时，HS-mode 访问 `hgatp` 或执行 HFENCE.GVMA 触发 illegal-instruction exception；M-mode 不受 TVM 影响。

6. **VMID 探测**：实现的 VMIDLEN 不固定，测试 VMID 字段时应先用全 1 写入读回探测 VMIDLEN，再使用合法 VMID 值。

7. **代码区 + 数据区映射要求**：恒等映射必须覆盖代码段、数据段、栈、页表池与 UART。对于 G-stage，所有这些段在 GPA 空间也必须映射且 U=1。

8. **QEMU 平台限制**：
   - QEMU virt 平台需要 `-cpu rv64,h=true`（或更新版本默认开启 H 扩展）
   - 512GB / 256TB 大 superpage 测试受物理内存限制
   - QEMU 默认实现 Svade（A=0 / D=0 触发 fault）

---

## 参考

- `SPEC/hypervisor.adoc` — RISC-V Hypervisor Extension, Version 1.0
- `docs/two_stage_translation_test_plan.md` — 两阶段联合行为测试计划（配套）
- `docs/vm_test_plan.md` — VS-stage / 普通 VM 测试计划（行为基线）
- `docs/hypervisor_framework.md` — Hypervisor 测试框架设计
- `common/hyp/gstage_pt.c` — G-stage 页表管理 API
- `common/hyp/two_stage.c` — 两阶段管理 API
- `common/hyp/hyp_defs.h` — Hypervisor CSR 与 cause code 定义
