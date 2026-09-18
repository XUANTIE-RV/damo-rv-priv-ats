**中文 | [English](../testplan_en/Hypervisor_Sv_test_plan_en.md)**

# Hypervisor 与 Sv* 扩展交叉测试计划

> 本文档描述 Hypervisor（H）扩展与其他 Sv* 系列（Supervisor 虚拟地址翻译相关）扩展在交叉场景下的测试计划。

---

## 本文档覆盖的 SPEC 章节

本方案依据以下 RISC-V 官方规范（本地路径）：

- `SPEC/riscv-isa-manual/src/priv/hypervisor.adoc` — Hypervisor（H）扩展：henvcfg.ADUE、HLV/HSV、HFENCE.GVMA、VS/VU-mode virtual-instruction 机制
- `SPEC/riscv-isa-manual/src/priv/svadu.adoc` — Svadu：硬件 A/D 位更新与 henvcfg.ADUE
- `SPEC/riscv-isa-manual/src/priv/svinval.adoc` — Svinval：HINVAL.VVMA/GVMA、SFENCE.W.INVAL/SFENCE.INVAL.IR
- `SPEC/riscv-isa-manual/src/priv/svnapot.adoc` — Svnapot：NAPOT PTE 与 G-stage 支持
- `SPEC/riscv-isa-manual/src/priv/svpbmt.adoc` — Svpbmt：两阶段翻译 PBMT 覆盖规则

官方仓库：

- https://github.com/riscv/riscv-isa-manual （对应仓库内上述路径文件）

---

## 范围

### 覆盖的扩展交叉

- **Hypervisor × Svadu**：`henvcfg.ADUE` 可写性、HLV/HSV 与硬件 A/D 更新交互、`menvcfg.ADUE` 修改后的 HFENCE.GVMA 同步
- **Hypervisor × Svinval**：HINVAL.VVMA/GVMA 指令功能、VMID 替代 ASID、VS/VU-mode virtual-instruction 触发
- **Hypervisor × Svnapot**：G-stage 翻译中的 NAPOT PTE 支持、保留编码 fault、两阶段同时使用 NAPOT
- **Hypervisor × Svpbmt**：两阶段地址翻译中 PBMT 属性的叠加覆盖行为、G-stage/VS-stage PBMT 覆盖规则

### 不在本文档范围

- 已由 `Hypervisor_CSR_test_plan.md`、`Hypervisor_Interrupts_test_plan.md`、`Hypervisor_Exceptions_test_plan.md`、`Hypervisor_2_stage_test_plan.md`、`Hypervisor_gstage_test_plan.md` 覆盖的 Hypervisor 基础功能
- 各扩展在非 Hypervisor 场景下的行为（由各自独立测试计划覆盖）
- Hypervisor 与 Ss\*/Sm\*/Z\* 扩展的交叉测试（分别由 `Hypervisor_Ss_test_plan.md`、`Hypervisor_Sm_test_plan.md`、`Hypervisor_Zi_test_plan.md`（非原子 Z 系列）与 `Hypervisor_Za_test_plan.md`（Za 原子/保留集系列：Zalrsc、Zawrs）覆盖）

---

## 覆盖的规范点

下表列出本方案覆盖的规范点。带 `norm:` 前缀的为 SPEC 官方 normative rule 标签；不带前缀的为本方案依据 SPEC 原文归纳的规范点。

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:henvcfg_adue_op` | `hypervisor.adoc` | If the Svadu extension is implemented, the ADUE bit controls whether hardware updating of PTE A/D bits is enabled for VS-stage address translation. When ADUE=1, hardware updating is enabled. When ADUE=0, the implementation behaves as though Svade were implemented for VS-stage address translation. If Svadu is not implemented, ADUE is read-only zero. | 若实现了 Svadu 扩展，ADUE 位控制 VS 阶段地址翻译的 PTE A/D 位硬件更新是否启用。ADUE=1 时启用，ADUE=0 时行为如同实现了 Svade。未实现 Svadu 时，ADUE 为只读零。 |
| `norm:Svadu_hypervisor_adue_writable` | `svadu.adoc` | When Svadu is implemented, `henvcfg.ADUE` must be writable. | 实现 Svadu 时，`henvcfg.ADUE` 必须可写。 |
| `svadu_hfence_gvma_sync` | `hypervisor.adoc` | After modifying `menvcfg.ADUE`, a `HFENCE.GVMA(x0,x0)` is required to synchronize the change across all VMIDs. | 修改 `menvcfg.ADUE` 后，需要执行 `HFENCE.GVMA(x0,x0)` 以在所有 VMID 间同步该变更。 |
| `norm:Svinval_hinval_vvma_gvma` | `svinval.adoc` | HINVAL.VVMA and HINVAL.GVMA have the same semantics as SINVAL.VMA, except that they combine with SFENCE.W.INVAL and SFENCE.INVAL.IR to replace HFENCE.VVMA and HFENCE.GVMA, respectively. | HINVAL.VVMA 和 HINVAL.GVMA 与 SINVAL.VMA 具有相同的语义，只是它们与 SFENCE.W.INVAL 和 SFENCE.INVAL.IR 结合分别替换 HFENCE.VVMA 和 HFENCE.GVMA。 |
| `norm:Svinval_hinval_gvma_uses_vmid` | `svinval.adoc` | HINVAL.GVMA uses VMIDs instead of ASIDs. | HINVAL.GVMA 使用 VMID 而不是 ASID。 |
| `norm:Svinval_virtual_instruction_vu_vs` | `svinval.adoc` | An attempt to execute HINVAL.VVMA or HINVAL.GVMA in VS-mode or VU-mode, or to execute SINVAL.VMA in VU-mode, raises a virtual-instruction exception. | 在 VS 模式或 VU 模式下尝试执行 HINVAL.VVMA 或 HINVAL.GVMA，或在 VU 模式下执行 SINVAL.VMA，会触发虚拟指令异常。 |
| `norm:Svinval_sfence_w_inval_inval_vu_mode` | `svinval.adoc` | An attempt to execute SFENCE.W.INVAL or SFENCE.INVAL.IR in VU-mode raises a virtual-instruction exception. | 在 VU 模式下尝试执行 SFENCE.W.INVAL 或 SFENCE.INVAL.IR 会触发虚拟指令异常。 |
| `norm:Svnapot_hyp_gstage` | `svnapot.adoc` | If the Hypervisor extension is also implemented, Svnapot is supported in G-stage translation. | 如果同时实现了 Hypervisor 扩展，Svnapot 在 G-stage 翻译中也受支持。 |
| `norm:Svpbmt_hgatp_stage_override_rule` | `svpbmt.adoc` | When `hgatp.MODE` is not Bare, a nonzero PBMT field in a G-stage leaf PTE overrides the PMA to produce intermediate memory attributes. | 当 `hgatp.MODE` 非零时，G-stage 叶 PTE 的非零 PBMT 位覆盖 PMA 产生中间属性。 |
| `norm:Svpbmt_vsatp_stage_override_rule` | `svpbmt.adoc` | When `vsatp.MODE` is not Bare, a nonzero PBMT field in a VS-stage leaf PTE overrides the intermediate memory attributes to produce the final memory attributes. | 当 `vsatp.MODE` 非零时，VS-stage 叶 PTE 的非零 PBMT 位覆盖中间属性产生最终属性。 |

---

## Group 1. Hypervisor × Svadu 交叉测试

**规范依据**：
- `norm:henvcfg_adue_op`：ADUE 位控制 VS-stage A/D 位硬件更新
- `norm:Svadu_hypervisor_adue_writable`：`henvcfg.ADUE` 必须可写
- `svadu_hfence_gvma_sync`：修改 `menvcfg.ADUE` 后需要 HFENCE.GVMA 同步

**测试职责**：验证 Svadu 扩展在 Hypervisor 两级翻译场景下的 CSR 可写性、HLV/HSV 指令交互、以及跨 VMID 同步行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SVADU-01 | henvcfg.ADUE 可写性验证 | HS-mode 写 henvcfg.ADUE=1，读回验证；再写 ADUE=0，读回验证 | 若实现 Svadu，ADUE 可写且读回一致；若未实现 Svadu，ADUE 只读零 |
| HCROSS-SVADU-02 | HLV 指令与 Svadu 交互（ADUE=1） | henvcfg.ADUE=1，VS-stage PTE A=0，HS-mode 执行 HLV.D 读取该 GPA | 访问成功，VS-stage PTE A 位被硬件自动设为 1（HLV 触发的隐式访问也遵循 ADUE 控制） |
| HCROSS-SVADU-03 | HSV 指令与 Svadu 交互（ADUE=1） | henvcfg.ADUE=1，VS-stage PTE A=1,D=0，HS-mode 执行 HSV.D 写入该 GPA | 访问成功，VS-stage PTE D 位被硬件自动设为 1 |
| HCROSS-SVADU-04 | HLV 指令与 Svade 交互（ADUE=0） | henvcfg.ADUE=0，VS-stage PTE A=0，HS-mode 执行 HLV.D 读取该 GPA | page-fault (cause=13)，硬件不自动更新 A 位（行为如同 Svade）。注意：VS-stage 翻译异常产生 page-fault (cause=13)，非 guest-page-fault (cause=21)。guest-page-fault 仅用于 G-stage 翻译异常（norm:H_vm_gpatrans） |
| HCROSS-SVADU-05 | menvcfg.ADUE 修改后 HFENCE.GVMA 同步 | menvcfg.ADUE 从 0 改为 1，不执行 HFENCE.GVMA，VS-mode 访问 A=0 的页；再执行 HFENCE.GVMA(x0,x0) 后重复访问 | 第一次访问可能仍按旧行为（实现相关）；HFENCE.GVMA 后行为必须按新 ADUE 值（A 位被硬件更新） |
| HCROSS-SVADU-06 | menvcfg.ADUE 修改后特定 VMID 同步 | menvcfg.ADUE 从 1 改为 0，仅对特定 VMID 执行 HFENCE.GVMA(vmid, x0)，验证该 VMID 和其他 VMID 的行为 | 指定 VMID 的行为必须按新 ADUE 值；其他 VMID 行为实现相关（可能仍按旧值） |

> [!NOTE]
> - HCROSS-SVADU-02~04 需要在 HS-mode 执行 HLV/HSV 指令，验证隐式访问（页表遍历）的 A/D 更新行为也受 `henvcfg.ADUE` 控制。
> - HCROSS-SVADU-04~07 中 VS-stage 翻译异常（A=0 + Svade）产生 **page-fault (cause=13)**，而非 guest-page-fault (cause=21)。根据 norm:H_vm_gpatrans，guest-page-fault 仅用于 **G-stage** 翻译异常，VS-stage 翻译异常使用常规 page-fault cause 码。
> - HCROSS-SVADU-05~06 验证 `menvcfg.ADUE` 变更后的同步语义。`HFENCE.GVMA(x0,x0)` 刷新所有 VMID，`HFENCE.GVMA(vmid,x0)` 仅刷新特定 VMID。
> - 若平台未实现 Svadu 扩展，HCROSS-SVADU-01 应验证 ADUE 只读零，HCROSS-SVADU-02~07 应 TEST_SKIP。

---

## Group 2. Hypervisor × Svinval 交叉测试

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
| HCROSS-SINVAL-15 | VU-mode 执行 SINVAL.VMA 触发 virtual-instruction | VU-mode 执行 SINVAL.VMA | virtual-instruction exception (cause=22) |

> [!NOTE]
> - HINVAL.VVMA/GVMA 是 Svinval 扩展为 Hypervisor 场景提供的细粒度 TLB 刷新指令，与 HFENCE.VVMA/GVMA 功能等价但支持批量刷新优化。
> - HCROSS-SINVAL-05~06 验证 HINVAL.GVMA 的 VMID 语义：VMID 非零时仅刷新特定 VMID 的 TLB，VMID=0 时刷新所有 VMID。这与 HFENCE.GVMA 的语义一致。
> - HCROSS-SINVAL-07~12 验证 VS/VU-mode 执行 HINVAL 和 SFENCE.W.INVAL/SFENCE.INVAL.IR 时的 virtual-instruction 异常触发，这是 Hypervisor 安全隔离的关键保证。
> - HCROSS-SINVAL-15 验证 VU-mode 执行 SINVAL.VMA 时触发 virtual-instruction 异常，与 HINVAL.VVMA/GVMA 在 VU-mode 的异常行为保持一致，完整覆盖 `norm:Svinval_virtual_instruction_vu_vs` 规范。
> - 与 `Hypervisor_2_stage_test_plan.md` Group 22 的区别：Group 22 仅覆盖了 VTVM=1 时 SINVAL.VMA 和 TVM=1 时 HINVAL.GVMA 的异常触发（2 个用例），本组补充了 HINVAL 指令功能、VMID 语义、以及 VS/VU-mode 的完整 virtual-instruction 覆盖（15 个用例）。

---

## Group 3. Hypervisor × Svnapot 交叉测试

**规范依据**：
- `norm:Svnapot_hyp_gstage`：如果同时实现了 Hypervisor 扩展，Svnapot 在 G-stage 翻译中也受支持

**测试职责**：验证 Svnapot 在 G-stage 翻译中的行为，包括 G-stage NAPOT PTE 的基本翻译、保留编码异常、以及 VS-stage 和 G-stage 同时使用 NAPOT 的两阶段翻译正确性。

> **注意**：本组测试从 `Svnapot_test_plan.md` Group 10 迁移而来。需要 H 扩展和 Svnapot 扩展同时可用。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCROSS-SVNAPOT-01 | G-stage 64 KiB NAPOT 基本翻译 | 在 G-stage 页表中配置 64 KiB NAPOT PTE | GPA→SPA 翻译正确 |
| HCROSS-SVNAPOT-02 | G-stage NAPOT 保留编码 fault | G-stage NAPOT PTE 使用保留编码 | guest page-fault |
| HCROSS-SVNAPOT-03 | G-stage 与 VS-stage 同时使用 NAPOT | VS-stage 和 G-stage 均使用 NAPOT PTE | 两阶段翻译正确 |

> [!NOTE]
> - 本组测试验证 Svnapot 扩展在 Hypervisor G-stage 翻译中的行为。Svnapot 规范明确指出，如果同时实现了 Hypervisor 扩展，NAPOT 翻译在 G-stage 页表中同样受支持。
> - HCROSS-SVNAPOT-01 验证 G-stage 中 64 KiB NAPOT PTE 的基本 GPA→SPA 翻译功能，与 `Hypervisor_gstage_test_plan.md` 中的普通 PTE 测试互补。
> - HCROSS-SVNAPOT-02 验证 G-stage NAPOT PTE 使用保留编码（ppn[0] 低 4 位非 `1000` 且 N=1）时，硬件应触发 guest-page-fault。这与 VS-stage 中的保留编码行为一致（参见 `Svnapot_test_plan.md` Group 3）。
> - HCROSS-SVNAPOT-03 验证两阶段翻译中 VS-stage 和 G-stage 同时使用 NAPOT PTE 的场景：VS-stage 将 GVA→GPA 使用 NAPOT 映射，G-stage 将 GPA→SPA 也使用 NAPOT 映射，最终 GVA→SPA 翻译应正确。

---

## Group 4. Hypervisor × Svpbmt 交叉测试

**规范依据**：
- `norm:Svpbmt_hgatp_stage_override_rule`：当 `hgatp.MODE` 非零时，G-stage PTE 的非零 PBMT 位覆盖 PMA 产生中间属性
- `norm:Svpbmt_vsatp_stage_override_rule`：当 `vsatp.MODE` 非零时，VS-stage PTE 的非零 PBMT 位覆盖中间属性产生最终属性

**测试职责**：验证两阶段地址翻译中 PBMT 属性的叠加覆盖行为，包括 G-stage PBMT 覆盖 PMA、VS-stage PBMT 覆盖中间属性、两阶段叠加、以及 `hgatp.MODE=0` 时跳过 G-stage 覆盖的场景。

> **注意**：本组测试从 `Svpbmt_test_plan.md` Group 10 迁移而来。需要 H 扩展和 Svpbmt 扩展同时可用。

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

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 2 (Svinval) | HCROSS-SINVAL-01~15 | HINVAL 指令功能和 virtual-instruction 异常是 Hypervisor 安全隔离的核心，现有计划覆盖严重不足 |
| P1（重要） | Group 1 (Svadu) | HCROSS-SVADU-01~06 | henvcfg.ADUE 可写性和 HLV/HSV 交互是 Svadu 在虚拟化场景下的关键行为 |
| P3（可选） | Group 3 (Svnapot) | HCROSS-SVNAPOT-01~03 | G-stage NAPOT 翻译依赖 H 扩展和 Svnapot 扩展同时可用，条件性实现 |
| P3（可选） | Group 4 (Svpbmt) | HCROSS-SVPBMT-01~04 | 两阶段 PBMT 覆盖依赖 H 扩展和 Svpbmt 扩展同时可用，条件性实现 |

---

## 关键注意事项

1. **扩展检测**：所有测试必须在运行时检测所需扩展（H、Svadu、Svinval、Svnapot、Svpbmt 等）的可用性，不可用时 TEST_SKIP。

2. **Svadu 与 Svade 的互斥**：`henvcfg.ADUE=0` 时行为如同 Svade（A/D=0 触发 fault），`ADUE=1` 时启用硬件 A/D 更新。测试时需明确当前 ADUE 状态。

3. **Svinval 指令编码**：
   - `hinval.vvma rs1, rs2`：`0011011 rs2 rs1 000 00000 1110011`
   - `hinval.gvma rs1, rs2`：`0111011 rs2 rs1 000 00000 1110011`
   - `sfence.w.inval`：`0001100 00000 00000 000 00000 1110011`
   - `sfence.inval.ir`：`0001100 00001 00000 000 00000 1110011`

4. **HINVAL.GVMA 的 VMID 语义**：`rs2` 寄存器指定 VMID（而非 ASID）。VMID=0 时刷新所有 VMID 的 TLB，VMID 非零时仅刷新特定 VMID。

5. **virtual-instruction 与 illegal-instruction 的区分**：VS/VU-mode 执行 HINVAL 触发 virtual-instruction (cause=22)，而非 illegal-instruction (cause=2)。测试断言必须使用准确的 cause 常量。

---

## 参考

- `hypervisor.adoc` — RISC-V Hypervisor Extension, Version 1.0
- `svadu.adoc` — Svadu Extension
- `svinval.adoc` — Svinval Extension
- `svnapot.adoc` — Svnapot Extension
- `svpbmt.adoc` — Svpbmt Extension
- `DOCS/testplan/Hypervisor_CSR_test_plan.md` — Hypervisor CSR 子集测试计划
- `DOCS/testplan/Hypervisor_Interrupts_test_plan.md` — Hypervisor 中断子集测试计划
- `DOCS/testplan/Hypervisor_Exceptions_test_plan.md` — Hypervisor 异常与 trap 子集测试计划
- `DOCS/testplan/Hypervisor_2_stage_test_plan.md` — 两阶段翻译测试计划
- `DOCS/testplan/Hypervisor_gstage_test_plan.md` — G-stage 独立测试计划
- `DOCS/testplan/Svadu_test_plan.md` — Svadu 独立测试计划
- `DOCS/testplan/Svinval_test_plan.md` — Svinval 独立测试计划
- `DOCS/testplan/Svnapot_test_plan.md` — Svnapot 独立测试计划
- `DOCS/testplan/Svpbmt_test_plan.md` — Svpbmt 独立测试计划
- `ideas/hypervisor_gap.md` — Hypervisor 测试缺口分析

---

## 附录 A：规范点覆盖矩阵

下表标明"覆盖的规范点"章节中每条规范点被哪些测试用例覆盖。

| Norm ID | 覆盖的测试 ID |
|---------|---------------|
| `norm:henvcfg_adue_op` | HCROSS-SVADU-01~04 |
| `norm:Svadu_hypervisor_adue_writable` | HCROSS-SVADU-01 |
| `svadu_hfence_gvma_sync` | HCROSS-SVADU-05、HCROSS-SVADU-06 |
| `norm:Svinval_hinval_vvma_gvma` | HCROSS-SINVAL-01~04 |
| `norm:Svinval_hinval_gvma_uses_vmid` | HCROSS-SINVAL-05、HCROSS-SINVAL-06 |
| `norm:Svinval_virtual_instruction_vu_vs` | HCROSS-SINVAL-07~10、HCROSS-SINVAL-15 |
| `norm:Svinval_sfence_w_inval_inval_vu_mode` | HCROSS-SINVAL-11、HCROSS-SINVAL-12（VS-mode 正常执行对照：HCROSS-SINVAL-13、HCROSS-SINVAL-14） |
| `norm:Svnapot_hyp_gstage` | HCROSS-SVNAPOT-01~03 |
| `norm:Svpbmt_hgatp_stage_override_rule` | HCROSS-SVPBMT-01、HCROSS-SVPBMT-03、HCROSS-SVPBMT-04 |
| `norm:Svpbmt_vsatp_stage_override_rule` | HCROSS-SVPBMT-02、HCROSS-SVPBMT-03、HCROSS-SVPBMT-04 |
