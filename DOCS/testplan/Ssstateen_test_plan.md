**中文 | [English](../testplan_en/Ssstateen_test_plan_en.md)**

# Ssstateen 扩展测试方案

## 1. 概述

### 1.1 规范来源

- **规范文件**: `SPEC/smstateen.adoc`
- **扩展名称**: Ssstateen (Supervisor-level State Enable Extension)
- **关联扩展**: Smstateen (Machine-level superset)

### 1.2 被测特性

Ssstateen 扩展是 Smstateen 的 supervisor-level 子集，仅包含 sstateen\* 和 hstateen\* CSR 及其功能。该扩展使 supervisor-level 软件和 hypervisor 能够控制低特权级对扩展状态的访问，防止隐蔽通道。

> **注意**：Ssstateen 不包含 mstateen\* CSR。mstateen\* 的行为由 Smstateen 扩展定义和测试。本方案假设 mstateen\* 由更高特权级软件正确配置以允许 sstateen\*/hstateen\* 的访问。

### 1.3 测试范围

| 测试层级 | 涉及 CSR | 测试组 |
|---------|---------|--------|
| S-mode | sstateen0-3 | Group 1-6 |
| H-mode | hstateen0-3, hstateen0h-3h (RV32) | Group 7-12（**已迁移至 [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) Group 8**） |

### 1.4 前提条件

| 项目 | 要求 |
|------|------|
| 架构 | RV64 (RV32 部分测试可选) |
| 特权级支持 | S-mode（必需）、U-mode（必需） |
| 可选依赖 | H 扩展 (Group 7-12)、Ssaia、Sscsrind、Zcmt、Sdtrig |
| mstateen 配置 | 测试前需由 M-mode 将对应 mstateen 位设为 1 以放行访问 |

---

## 2. 规范要求分析

### 2.1 Ssstateen 相关 Normative Rules

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sstateen_rv64_csrs` | — | 实现 S-mode 时添加 sstateen0-3 |
| `norm:hstateen_rv64_csrs` | — | 实现 H 扩展时添加 hstateen0-3 |
| `norm:stateen_rv32_upper_bits_csrs` | — | RV32 额外提供 hstateen0h-3h |
| `norm:stateen_op` | The `stateen` registers at each level control access to state at all less-privileged levels, but not at its own level. | 各级 `stateen` 寄存器控制对所有较低特权级别的状态访问，但不控制其自身级别。 |
| `norm:stateen_illegal_state_access` | Just as with the `counteren` CSRs, when a `stateen` CSR prevents access to state by less-privileged levels, an attempt in one of those privilege modes to execute an instruction that would read or write the protected state raises an illegal-instruction exception, or, if executing in VS or VU mode and the circumstances for a virtual-instruction exception apply, raises a virtual-instruction exception instead of an illegal-instruction exception. | 与 `counteren` CSR 类似，当 `stateen` CSR 阻止较低特权级别访问状态时，在这些特权模式下尝试执行读取或写入受保护状态的指令会触发非法指令异常，或者如果在 VS 或 VU 模式下执行且满足虚拟指令异常的条件，则触发虚拟指令异常而非非法指令异常。 |
| `norm:stateen_implicit_state_update` | When a `stateen` CSR prevents access to state for a privilege mode, attempting to execute in that privilege mode an instruction that _implicitly_ updates the state without reading it may or may not raise an illegal-instruction or virtual-instruction exception. Such cases must be disambiguated by being explicitly specified one way or the other. | 当 `stateen` CSR 阻止某个特权模式访问状态时，在该特权模式下尝试执行隐式更新状态而不读取状态的指令可能会也可能不会触发非法指令或虚拟指令异常。此类情况必须通过明确指定来消除歧义。 |
| `norm:sstateen_user_access_control` | Each bit of a supervisor-level `sstateen` CSR controls user-level access (from U-mode or VU-mode) to an extension's state. | 监管级 `sstateen` CSR 的每一位控制用户级（来自 U 模式或 VU 模式）对扩展状态的访问。 |
| `norm:sstateen_bit_allocation` | The intention is to allocate the bits of `sstateen` CSRs starting at the least-significant end, bit 0, through to bit 31, and then on to the next-higher-numbered `sstateen` CSR. | 意图是从最低有效端（位 0）开始分配 `sstateen` CSR 的位，到位 31，然后继续到下一个更高编号的 `sstateen` CSR。 |
| `norm:sstateen_encroachment_bits_roz` | In that case, the bit positions of "encroaching" bits will remain forever read-only zeros in the matching `sstateen` CSRs. | 在这种情况下，"侵入"位的位位置在匹配的 `sstateen` CSR 中将永远保持为只读零。 |
| `norm:hstateen_encoding` | With the hypervisor extension, the `hstateen` CSRs have identical encodings to the `mstateen` CSRs, except controlling accesses for a virtual machine (from VS and VU modes). | 使用 hypervisor 扩展时，`hstateen` CSR 具有与 `mstateen` CSR 相同的编码，只是控制虚拟机（来自 VS 和 VU 模式）的访问。 |
| `norm:stateen_warl_access` | Each standard-defined bit of a `stateen` CSR is WARL and may be read-only zero or one, subject to the following conditions. | `stateen` CSR 的每个标准定义位都是 WARL，可以是只读零或一，需满足以下条件。 |
| `norm:stateen_unimplemented_state_roz` | Bits in any `stateen` CSR that are defined to control state that a hart doesn't implement are read-only zeros for that hart. | 任何 `stateen` CSR 中定义为控制 hart 未实现的状态的位对该 hart 而言是只读零。 |
| `norm:stateen_reserved_roz` | Likewise, all reserved bits not yet given a defined meaning are also read-only zeros. | 同样，所有尚未赋予定义含义的保留位也是只读零。 |
| `norm:mstateen_lower_priv_roz` | For every bit in an `mstateen` CSR that is zero (whether read-only zero or set to zero), the same bit appears as read-only zero in the matching `hstateen` and `sstateen` CSRs. | 对于 `mstateen` CSR 中为零的每一位（无论是只读零还是设置为零），相同的位在匹配的 `hstateen` 和 `sstateen` CSR 中显示为只读零。 |
| `norm:sstateen_vsmode_access_roz` | For every bit in an `hstateen` CSR that is zero (whether read-only zero or set to zero), the same bit appears as read-only zero in `sstateen` when accessed in VS-mode. | 对于 `hstateen` CSR 中为零的每一位（无论是只读零还是设置为零），在 VS 模式下访问时，相同的位在 `sstateen` 中显示为只读零。 |
| `norm:sstateen_ro1_bits` | A bit in a supervisor-level `sstateen` CSR cannot be read-only one unless the same bit is read-only one in the matching `mstateen` CSR and, if it exists, in the matching `hstateen` CSR. | 监管级 `sstateen` CSR 中的位不能是只读一，除非相同的位在匹配的 `mstateen` CSR 中以及（如果存在）匹配的 `hstateen` CSR 中是只读一。 |
| `norm:hstateen_ro1_bits` | A bit in an `hstateen` CSR cannot be read-only one unless the same bit is read-only one in the matching `mstateen` CSR. | `hstateen` CSR 中的位不能是只读一，除非相同的位在匹配的 `mstateen` CSR 中是只读一。 |
| `norm:hstateen_sstateen_zero_initialization` | If machine-level software changes these values, it is responsible for initializing the corresponding writable bits of the `hstateen` and `sstateen` CSRs to zeros too. | 如果机器级软件更改这些值，它负责将 `hstateen` 和 `sstateen` CSR 的相应可写位也初始化为零。 |
| `norm:hstateen_bit_63_op` | Likewise, bit 63 of each `hstateen` correspondingly controls access to the matching `sstateen` CSR. | 同样，每个 `hstateen` 的位 63 相应地控制对匹配的 `sstateen` CSR 的访问。 |
| `norm:hstateen_bit_63_writable` | Bit 63 of each `hstateen` CSR is always writable (not read-only). | 每个 `hstateen` CSR 的位 63 始终可写（非只读）。 |
| `norm:stateen0_c_op` | The C bit controls access to any and all custom state. | C 位控制对任何和所有自定义状态的访问。 |
| `norm:stateen0_fcsr_op` | The FCSR bit controls access to `fcsr` for the case when floating-point instructions operate on `x` registers instead of `f` registers as specified by the Zfinx and related extensions (Zdinx, etc.). | FCSR 位控制对 `fcsr` 的访问，适用于浮点指令在 `x` 寄存器而非 `f` 寄存器上操作的情况（如 Zfinx 及相关扩展所指定）。 |
| `norm:stateen0_jvt_op` | The JVT bit controls access to the `jvt` CSR provided by the Zcmt extension. | JVT 位控制对 Zcmt 扩展提供的 `jvt` CSR 的访问。 |
| `norm:hstateen0_SE0_op` | The SE0 bit in `hstateen0` controls access to the `sstateen0` CSR. | `hstateen0` 中的 SE0 位控制对 `sstateen0` CSR 的访问。 |
| `norm:hstateen0_envcfg_op` | The ENVCFG bit in `hstateen0` controls access to the `senvcfg` CSRs. | `hstateen0` 中的 ENVCFG 位控制对 `senvcfg` CSR 的访问。 |
| `norm:hstateen0_csrind_op` | The CSRIND bit in `hstateen0` controls access to the `siselect` and the `sireg*`, (really `vsiselect` and `vsireg*`) CSRs provided by the Sscsrind extensions. | `hstateen0` 中的 CSRIND 位控制对 Sscsrind 扩展提供的 `siselect` 和 `sireg*`（实际上是 `vsiselect` 和 `vsireg*`）CSR 的访问。 |
| `norm:hstateen0_imsic_op` | The IMSIC bit in `hstateen0` controls access to the guest IMSIC state, including CSRs `stopei` (really `vstopei`), provided by the Ssaia extension. | `hstateen0` 中的 IMSIC 位控制对 Ssaia 扩展提供的客户 IMSIC 状态（包括 CSR `stopei`，实际上是 `vstopei`）的访问。 |
| `norm:hstateen0_aia_op` | The AIA bit in `hstateen0` controls access to all state introduced by the Ssaia extension and not controlled by either the CSRIND or the IMSIC bits of `hstateen0`. | `hstateen0` 中的 AIA 位控制对 Ssaia 扩展引入的所有状态的访问，这些状态不受 `hstateen0` 的 CSRIND 或 IMSIC 位控制。 |
| `norm:hstateen0_context_op` | The CONTEXT bit in `hstateen0` controls access to the `scontext` CSR provided by the Sdtrig extension. | `hstateen0` 中的 CONTEXT 位控制对 Sdtrig 扩展提供的 `scontext` CSR 的访问。 |
| `norm:mstateen_bit_correspondence` | For every bit with a defined purpose in an `sstateen` CSR, the same bit is defined in the matching `mstateen` CSR to control access below machine level to the same state. | 对于 `sstateen` CSR 中具有定义用途的每一位，匹配的 `mstateen` CSR 中定义了相同的位，以控制机器级别以下对相同状态的访问。 |

---

## 3. S-mode 测试组

### Group 1：sstateen CSR 存在性与可访问性

**规范依据**：
- `norm:sstateen_rv64_csrs`：实现 S-mode 时添加 sstateen0-3
- `norm:sstateen_bit_allocation`：sstateen 位从 bit 0 向高位分配

**前提配置**：M-mode 需将对应 mstateen 的 bit 63 设为 1，允许 S-mode 访问 sstateen。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-EXIST-01 | sstateen0 在 S-mode 可读 | 配置 mstateen0.SE0=1，S-mode 读 sstateen0 | 无异常 | `norm:sstateen_rv64_csrs` |
| SS-EXIST-02 | sstateen0 在 S-mode 可写 | 配置 mstateen0.SE0=1，S-mode 写 sstateen0 后读回 | 无异常，可写位生效 | `norm:sstateen_rv64_csrs` |
| SS-EXIST-03 | sstateen1 在 S-mode 可读写 | 配置 mstateen1 bit63=1，S-mode 读写 sstateen1 | 无异常 | `norm:sstateen_rv64_csrs` |
| SS-EXIST-04 | sstateen2 在 S-mode 可读写 | 配置 mstateen2 bit63=1，S-mode 读写 sstateen2 | 无异常 | `norm:sstateen_rv64_csrs` |
| SS-EXIST-05 | sstateen3 在 S-mode 可读写 | 配置 mstateen3 bit63=1，S-mode 读写 sstateen3 | 无异常 | `norm:sstateen_rv64_csrs` |
| SS-EXIST-06 | sstateen0 为 32 位有效宽度 | S-mode 向 sstateen0 写入 64 位全 1 后读回 | 仅低 32 位有效，高位不存在 | `norm:sstateen_bit_allocation` |

**测试步骤示例 (SS-EXIST-01)**：
1. 在 M-mode 下将 mstateen0 bit 63 (SE0) 设为 1
2. 配置 PMP 允许 S-mode 执行
3. 切换到 S-mode
4. 读取 sstateen0 CSR
5. 验证读取操作无异常
6. 返回 M-mode

**预期结果**：
- S-mode 读取 sstateen0 正常，无 illegal-instruction 异常

**断言**：
```
CHECK_NO_TRAP("S-mode read sstateen0 should succeed");
uint32_t val = read_csr(sstateen0);
LOG_INFO("sstateen0 value = 0x%08x", val);
```

---

### Group 2：sstateen 控制 U-mode/VU-mode 访问

**规范依据**：
- `norm:sstateen_user_access_control`：sstateen 的每一位控制 U-mode 或 VU-mode 对扩展状态的访问
- `norm:stateen_op`：stateen 控制所有低特权级访问，不控制本级
- `norm:stateen_illegal_state_access`：被阻止的访问触发 illegal-instruction 异常；VS/VU 模式下若满足条件触发 virtual-instruction 异常

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-UCTL-01 | sstateen0.C=0 阻止 U-mode 自定义状态 | 设 sstateen0.C=0，U-mode 访问自定义 CSR | 触发 illegal-instruction 异常 | `norm:sstateen_user_access_control` |
| SS-UCTL-02 | sstateen0.C=1 允许 U-mode 自定义状态 | 设 sstateen0.C=1，U-mode 访问自定义 CSR | 访问正常（取决于扩展实现） | `norm:sstateen_user_access_control` |
| SS-UCTL-03 | sstateen0 不控制 S-mode 自身 | sstateen0.C=0，S-mode 访问自定义状态 | S-mode 访问正常，无异常 | `norm:stateen_op` |
| SS-UCTL-04 | sstateen0.JVT=0 阻止 U-mode jvt | 设 sstateen0.JVT=0，U-mode 读 jvt CSR | 触发 illegal-instruction 异常 | `norm:sstateen_user_access_control`, `norm:stateen0_jvt_op` |
| SS-UCTL-05 | sstateen0.JVT=1 允许 U-mode jvt | 设 sstateen0.JVT=1，U-mode 读 jvt CSR | 访问正常 | `norm:sstateen_user_access_control`, `norm:stateen0_jvt_op` |
| SS-UCTL-06 | sstateen0.FCSR=0 阻止 U-mode fcsr (Zfinx) | 确保 misa.F=0 且 sstateen0.FCSR=0，U-mode 执行浮点指令 | 所有浮点指令触发 illegal-instruction 异常 | `norm:sstateen_user_access_control`, `norm:stateen0_fcsr_op` |
| SS-UCTL-07 | U-mode 被阻止的读触发 illegal-instruction | sstateen0 某位=0，U-mode 读受控 CSR | 触发 illegal-instruction 异常 (cause=2) | `norm:stateen_illegal_state_access` |
| SS-UCTL-08 | U-mode 被阻止的写触发 illegal-instruction | sstateen0 某位=0，U-mode 写受控 CSR | 触发 illegal-instruction 异常 (cause=2) | `norm:stateen_illegal_state_access` |
| SS-UCTL-09 | VU-mode 被阻止的访问触发 virtual-instruction | sstateen0 某位=0，VU-mode 访问受控 CSR | 触发 virtual-instruction 异常 (cause=22) | `norm:stateen_illegal_state_access` |

> **[H 扩展依赖]** SS-UCTL-09 需要 Hypervisor 扩展支持（VU-mode 场景）。相关 Hypervisor 交叉测试见 [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) Group 8。

**测试步骤示例 (SS-UCTL-01)**：
1. 在 M-mode 下设置 mstateen0.C=1、mstateen0.SE0=1
2. 切换到 S-mode
3. 设置 sstateen0.C=0
4. 配置 PMP 允许 U-mode 执行
5. 安装 U-mode trap handler
6. 切换到 U-mode
7. 尝试访问自定义 CSR
8. 返回 S-mode 检查是否捕获 illegal-instruction 异常

**预期结果**：
- U-mode 访问自定义状态触发 illegal-instruction 异常

**断言**：
```
EXPECT_TRAP(CAUSE_ILLEGAL_INSTRUCTION, read_custom_csr());
CHECK_TRAP("U-mode custom CSR with sstateen0.C=0", CAUSE_ILLEGAL_INSTRUCTION);
```

---

### Group 3：sstateen0 功能位

**规范依据**：
- `norm:stateen0_c_op`：C 位 (bit 0) 控制自定义状态访问
- `norm:stateen0_fcsr_op`：FCSR 位 (bit 1) 控制 Zfinx 场景 fcsr
- `norm:stateen0_jvt_op`：JVT 位 (bit 2) 控制 Zcmt jvt CSR
- `norm:mstateen0_fcsr_roz`：misa.F=1 时 FCSR 位为只读零（传播到 sstateen0）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-FUNC-01 | sstateen0.C 位可写 | S-mode 写 sstateen0.C=1 后读回 | C 位为 1（前提：mstateen0.C=1） | `norm:stateen0_c_op` |
| SS-FUNC-02 | sstateen0.C=0 阻止 U-mode 自定义访问 | 设 sstateen0.C=0，U-mode 访问自定义 CSR | 触发 illegal-instruction 异常 | `norm:stateen0_c_op` |
| SS-FUNC-03 | sstateen0.C=1 允许 U-mode 自定义访问 | 设 sstateen0.C=1，U-mode 访问自定义 CSR | 访问正常 | `norm:stateen0_c_op` |
| SS-FUNC-04 | sstateen0.FCSR 位行为 (misa.F=1) | misa.F=1 时，sstateen0.FCSR 为只读零 | FCSR 位写 1 后读回仍为 0 | `norm:mstateen0_fcsr_roz` |
| SS-FUNC-05 | sstateen0.FCSR=0 浮点指令非法 (misa.F=0) | 确保 misa.F=0、sstateen0.FCSR=0，U-mode 执行浮点指令 | 所有浮点指令触发 illegal-instruction 异常 | `norm:stateen0_fcsr_op` |
| SS-FUNC-06 | sstateen0.JVT 位可写 | S-mode 写 sstateen0.JVT=1 后读回 | JVT 位为 1（前提：Zcmt 实现且 mstateen0.JVT=1） | `norm:stateen0_jvt_op` |
| SS-FUNC-07 | sstateen0.JVT=0 阻止 U-mode jvt | 设 sstateen0.JVT=0，U-mode 读 jvt | 触发 illegal-instruction 异常 | `norm:stateen0_jvt_op` |
| SS-FUNC-08 | sstateen0.JVT=1 允许 U-mode jvt | 设 sstateen0.JVT=1，U-mode 读 jvt | 访问正常 | `norm:stateen0_jvt_op` |
| SS-FUNC-09 | sstateen0 WPRI 位只读零 | S-mode 向 sstateen0 保留位写 1 后读回 | 保留位读回为 0 | `norm:stateen_reserved_roz` |

**测试步骤示例 (SS-FUNC-05)**：
1. 在 M-mode 确认 misa.F=0（Zfinx 场景）
2. 设置 mstateen0.FCSR=0、mstateen0.SE0=1
3. 切换到 S-mode
4. 确认 sstateen0.FCSR=0
5. 切换到 U-mode
6. 执行一条浮点指令（如 `fadd`）
7. 验证触发 illegal-instruction 异常

**预期结果**：
- 所有浮点指令均触发 illegal-instruction 异常，如同它们都访问 fcsr

**断言**：
```
EXPECT_TRAP(CAUSE_ILLEGAL_INSTRUCTION, asm("fadd fa0, fa0, fa1"));
CHECK_TRAP("FP instr illegal when misa.F=0 and FCSR=0", CAUSE_ILLEGAL_INSTRUCTION);
```

---

### Group 4：sstateen 位分配与只读约束

**规范依据**：
- `norm:sstateen_bit_allocation`：sstateen 位从 bit 0 向 bit 31 分配
- `norm:sstateen_ro1_bits`：sstateen 位不能为 RO1 除非 mstateen 和 hstateen 同位也为 RO1
- `norm:sstateen_encroachment_bits_roz`：mstateen 高位侵入的位在 sstateen 中永久只读零
- `norm:mstateen_bit_correspondence`：sstateen 有定义的位在 mstateen 中也有定义
- `norm:mstateen_lower_priv_roz`：mstateen 为零的位在 sstateen 中为只读零

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-ALLOC-01 | sstateen0 位分配从 bit 0 开始 | 检查 sstateen0 已定义位集中在低位 | C=bit0, FCSR=bit1, JVT=bit2 | `norm:sstateen_bit_allocation` |
| SS-ALLOC-02 | sstateen0 RO1 位约束 (无 H 扩展) | 识别 sstateen0 RO1 位，验证 mstateen0 同位也是 RO1 | mstateen0 同位也为 RO1 | `norm:sstateen_ro1_bits` |
| SS-ALLOC-03 | sstateen0 RO1 位约束 (含 H 扩展) | 识别 sstateen0 RO1 位，验证 mstateen0 和 hstateen0 同位也是 RO1 | 两者同位均为 RO1 | `norm:sstateen_ro1_bits` |

> **[H 扩展依赖]** SS-ALLOC-03 需要 Hypervisor 扩展（验证 hstateen0 的 RO1 约束）。相关 Hypervisor 交叉测试见 [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) Group 8（HCROSS-SSSTA-39~45 只读约束）。
| SS-ALLOC-04 | sstateen0 侵入位只读零 | 若 mstateen0 高位分配侵入低 32 位，检查 sstateen0 对应位 | 侵入位在 sstateen0 为只读零 | `norm:sstateen_encroachment_bits_roz` |
| SS-ALLOC-05 | sstateen 位与 mstateen 对应 | 对 sstateen0 每个有定义位，检查 mstateen0 对应位 | mstateen0 对应位有定义 | `norm:mstateen_bit_correspondence` |
| SS-ALLOC-06 | mstateen 零位传播到 sstateen | 将 mstateen0 某功能位设 0，S-mode 写 sstateen0 同位 | sstateen0 对应位为只读零 | `norm:mstateen_lower_priv_roz` |
| SS-ALLOC-07 | mstateen 一位解除 sstateen 传播 | 将 mstateen0 某功能位从 0 改为 1 | sstateen0 对应位变为可写 | `norm:mstateen_lower_priv_roz` |
| SS-ALLOC-08 | sstateen 未实现扩展位只读零 | 对于未实现的扩展，写 sstateen0 对应位 | 对应位读回为 0 | `norm:stateen_unimplemented_state_roz` |

**测试步骤示例 (SS-ALLOC-02)**：
1. 在 M-mode 下设置 mstateen0.SE0=1
2. 在 M-mode 下将 mstateen0 对应位写 0
3. 切换到 S-mode
4. 尝试将 sstateen0 对应位写 0
5. 读回 sstateen0
6. 如果该位读回为 1（RO1），返回 M-mode
7. 尝试清除 mstateen0 同位
8. 验证 mstateen0 同位也必须为 RO1

**预期结果**：
- 如果 sstateen0 某位为 RO1，则 mstateen0 同位也必须为 RO1

**断言**：
```
clear_csr(sstateen0, test_bit);
uint32_t ssta_val = read_csr(sstateen0);
if (ssta_val & test_bit) {
    /* sstateen0 该位为 RO1，回 M-mode 验证 mstateen0 */
    goto_priv(PRIV_M);
    clear_csr(mstateen0, test_bit);
    uint64_t msta_val = read_csr(mstateen0);
    TEST_ASSERT("mstateen0 same bit must also be RO1", msta_val & test_bit);
}
```

---

### Group 5：sstateen 异常行为

**规范依据**：
- `norm:stateen_illegal_state_access`：被阻止的访问触发 illegal-instruction 异常；VS/VU 模式下满足条件时触发 virtual-instruction 异常
- `norm:stateen_implicit_state_update`：隐式更新受控状态但不读取的指令，是否触发异常需明确规定
- `norm:stateen_op`：stateen 控制所有低特权级访问，不控制本级

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-EXC-01 | sstateen 不控制 S-mode 自身 | sstateen0 某位=0，S-mode 访问受控状态 | S-mode 访问正常 | `norm:stateen_op` |
| SS-EXC-02 | U-mode 被阻止的读触发 illegal-instruction | sstateen0 某位=0，U-mode 读受控 CSR | cause=2 (illegal-instruction) | `norm:stateen_illegal_state_access` |
| SS-EXC-03 | U-mode 被阻止的写触发 illegal-instruction | sstateen0 某位=0，U-mode 写受控 CSR | cause=2 (illegal-instruction) | `norm:stateen_illegal_state_access` |
| SS-EXC-04 | VU-mode 被阻止的访问触发 virtual-instruction | sstateen0 某位=0 且 VU-mode 访问 | cause=22 (virtual-instruction) | `norm:stateen_illegal_state_access` |

> **[H 扩展依赖]** SS-EXC-04 需要 Hypervisor 扩展（VU-mode 场景触发 virtual-instruction）。相关 Hypervisor 交叉测试见 [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) Group 8。
| SS-EXC-05 | 隐式状态更新指令行为 | 执行隐式更新受控状态但不读取的指令 | 行为取决于具体规范定义 | `norm:stateen_implicit_state_update` |

**测试步骤示例 (SS-EXC-02)**：
1. 在 M-mode 配置 mstateen0（对应位=1 放行 S-mode）、mstateen0.SE0=1
2. 切换到 S-mode
3. 设置 sstateen0.JVT=0
4. 安装 U-mode trap handler
5. 切换到 U-mode
6. 尝试读取 jvt CSR
7. 返回 S-mode，验证异常

**预期结果**：
- U-mode 读 jvt 触发 illegal-instruction 异常 (cause=2)

**断言**：
```
EXPECT_TRAP(CAUSE_ILLEGAL_INSTRUCTION, read_csr(jvt));
CHECK_TRAP("U-mode read jvt with sstateen0.JVT=0", CAUSE_ILLEGAL_INSTRUCTION);
```

---

### Group 6：sstateen 初始化要求

**规范依据**：
- `norm:hstateen_sstateen_zero_initialization`：M-mode 软件修改 mstateen 后，负责将对应 sstateen 初始化为零

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-INIT-01 | sstateen0 初始化为零 | M-mode 设置 mstateen0 后写零到 sstateen0，S-mode 读回 | 所有可写位为 0 | `norm:hstateen_sstateen_zero_initialization` |
| SS-INIT-02 | sstateen1 初始化为零 | M-mode 设置 mstateen1 后写零到 sstateen1，S-mode 读回 | 所有可写位为 0 | `norm:hstateen_sstateen_zero_initialization` |
| SS-INIT-03 | sstateen2 初始化为零 | M-mode 设置 mstateen2 后写零到 sstateen2，S-mode 读回 | 所有可写位为 0 | `norm:hstateen_sstateen_zero_initialization` |
| SS-INIT-04 | sstateen3 初始化为零 | M-mode 设置 mstateen3 后写零到 sstateen3，S-mode 读回 | 所有可写位为 0 | `norm:hstateen_sstateen_zero_initialization` |
| SS-INIT-05 | OS 进入时 sstateen 应为零 | 模拟 OS 首次进入 S-mode 时，验证 sstateen 已初始化 | 所有可写位为 0 | `norm:hstateen_sstateen_zero_initialization` |

**测试步骤示例 (SS-INIT-01)**：
1. 在 M-mode 下将 mstateen0 某些位设为 1（启用功能）
2. 向 sstateen0 写零
3. 设置 mstateen0.SE0=1
4. 切换到 S-mode
5. 读取 sstateen0
6. 验证所有可写位为零

**预期结果**：
- sstateen0 读回值中所有可写位均为 0

**断言**：
```
uint32_t val = read_csr(sstateen0);
TEST_ASSERT_EQ("sstateen0 should be initialized to 0", val, 0);
```

---

## 4. H-mode 测试组

> **前提条件**：以下测试需要 H（Hypervisor）扩展实现。M-mode 需预先将对应 mstateen 位设为 1 以放行 HS-mode 对 hstateen 的访问。

> **[迁移通知]** 以下 Group 7–12 的 H-mode 测试已迁移至 [`Hypervisor_cross_test_plan.md`](./Hypervisor_cross_test_plan.md) 的 **Group 8 (Hypervisor × Ssstateen 交叉测试)**，测试 ID 重新编号为 `HCROSS-SSSTA-01~50`。本文件保留原始描述供参考，实际实现和运行以交叉测试计划为准。

### Group 7：hstateen CSR 存在性与可访问性

> **[迁移]** 本组 6 个测试已迁移至 `Hypervisor_cross_test_plan.md` Group 8.1，ID 映射：SS-HEXIST-01~06 → HCROSS-SSSTA-01~06。

**规范依据**：
- `norm:hstateen_rv64_csrs`：实现 H 扩展时添加 hstateen0-3
- `norm:stateen_rv32_upper_bits_csrs`：RV32 额外提供 hstateen0h-3h
- `norm:hstateen_encoding`：hstateen 编码与 mstateen 一致（除控制对象为 VS/VU 模式外）

**前提配置**：M-mode 需将对应 mstateen 的 bit 63 设为 1。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-HEXIST-01 | hstateen0 在 HS-mode 可读 | 配置 mstateen0.SE0=1，HS-mode 读 hstateen0 | 无异常 | `norm:hstateen_rv64_csrs` |
| SS-HEXIST-02 | hstateen0 在 HS-mode 可写 | 配置 mstateen0.SE0=1，HS-mode 写 hstateen0 后读回 | 无异常，可写位生效 | `norm:hstateen_rv64_csrs` |
| SS-HEXIST-03 | hstateen1 在 HS-mode 可读写 | 配置 mstateen1 bit63=1，HS-mode 读写 hstateen1 | 无异常 | `norm:hstateen_rv64_csrs` |
| SS-HEXIST-04 | hstateen2 在 HS-mode 可读写 | 配置 mstateen2 bit63=1，HS-mode 读写 hstateen2 | 无异常 | `norm:hstateen_rv64_csrs` |
| SS-HEXIST-05 | hstateen3 在 HS-mode 可读写 | 配置 mstateen3 bit63=1，HS-mode 读写 hstateen3 | 无异常 | `norm:hstateen_rv64_csrs` |
| SS-HEXIST-06 | hstateen0h 在 HS-mode 可读写 (RV32) | RV32 下配置 mstateen0.SE0=1，HS-mode 读写 hstateen0h | 无异常 | `norm:stateen_rv32_upper_bits_csrs` |

**测试步骤示例 (SS-HEXIST-01)**：
1. 在 M-mode 下将 mstateen0 bit 63 (SE0) 设为 1
2. 切换到 HS-mode
3. 读取 hstateen0 CSR
4. 验证读取操作无异常
5. 返回 M-mode

**预期结果**：
- HS-mode 读取 hstateen0 正常，无 illegal-instruction 异常

**断言**：
```
CHECK_NO_TRAP("HS-mode read hstateen0 should succeed");
uint64_t val = read_csr(hstateen0);
LOG_INFO("hstateen0 value = 0x%016lx", val);
```

---

### Group 8：hstateen bit 63 控制 sstateen 访问

> **[迁移]** 本组 9 个测试已迁移至 `Hypervisor_cross_test_plan.md` Group 8.2，ID 映射：SS-HB63-01~09 → HCROSS-SSSTA-07~15。

**规范依据**：
- `norm:hstateen_bit_63_op`：hstateen 的 bit 63 控制对应 sstateen 的 VS-mode 访问
- `norm:hstateen_bit_63_writable`：hstateen bit 63 始终可写（不是只读）

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-HB63-01 | hstateen0 bit 63 可写（写 0） | HS-mode 将 hstateen0 bit 63 写 0 后读回 | bit 63 读回为 0 | `norm:hstateen_bit_63_writable` |
| SS-HB63-02 | hstateen0 bit 63 可写（写 1） | HS-mode 将 hstateen0 bit 63 写 1 后读回 | bit 63 读回为 1 | `norm:hstateen_bit_63_writable` |
| SS-HB63-03 | hstateen0.SE0=0 阻止 VS-mode 访问 sstateen0 | 设 hstateen0 bit63=0，VS-mode 读 sstateen0 | 触发 virtual-instruction 异常 (cause=22) | `norm:hstateen_bit_63_op` |
| SS-HB63-04 | hstateen0.SE0=1 允许 VS-mode 访问 sstateen0 | 设 hstateen0 bit63=1，VS-mode 读 sstateen0 | 访问正常，无异常 | `norm:hstateen_bit_63_op` |
| SS-HB63-05 | hstateen0.SE0=0 阻止 VS-mode 写 sstateen0 | 设 hstateen0 bit63=0，VS-mode 写 sstateen0 | 触发 virtual-instruction 异常 (cause=22) | `norm:hstateen_bit_63_op` |
| SS-HB63-06 | hstateen1 bit 63 可写 | HS-mode 写 hstateen1 bit 63 为 0 和 1 | 每次读回值与写入一致 | `norm:hstateen_bit_63_writable` |
| SS-HB63-07 | hstateen1 bit63=0 阻止 VS-mode 访问 sstateen1 | 设 hstateen1 bit63=0，VS-mode 读 sstateen1 | 触发 virtual-instruction 异常 | `norm:hstateen_bit_63_op` |
| SS-HB63-08 | hstateen2 bit 63 控制 sstateen2 | 设 hstateen2 bit63=0/1，VS-mode 访问 sstateen2 | bit63=0 异常，bit63=1 正常 | `norm:hstateen_bit_63_op` |
| SS-HB63-09 | hstateen3 bit 63 控制 sstateen3 | 设 hstateen3 bit63=0/1，VS-mode 访问 sstateen3 | bit63=0 异常，bit63=1 正常 | `norm:hstateen_bit_63_op` |

**测试步骤示例 (SS-HB63-03)**：
1. 在 M-mode 下设置 mstateen0.SE0=1（放行 HS-mode）
2. 切换到 HS-mode
3. 将 hstateen0 bit 63 设为 0
4. 配置 VS-mode 环境并安装 trap handler
5. 切换到 VS-mode
6. 尝试读取 sstateen0 CSR
7. 返回 HS-mode 验证异常

**预期结果**：
- VS-mode 读取 sstateen0 触发 virtual-instruction 异常 (cause=22)

**断言**：
```
EXPECT_TRAP(CAUSE_VIRTUAL_INSTRUCTION, read_csr(sstateen0));
CHECK_TRAP("VS-mode access sstateen0 with hstateen0.SE0=0", CAUSE_VIRTUAL_INSTRUCTION);
```

---

### Group 9：hstateen 对 VS-mode sstateen 的只读零传播

> **[迁移]** 本组 5 个测试已迁移至 `Hypervisor_cross_test_plan.md` Group 8.3，ID 映射：SS-VSPROP-01~05 → HCROSS-SSSTA-16~20。

**规范依据**：
- `norm:sstateen_vsmode_access_roz`：hstateen 中为零的位（无论是只读零还是写为零），在 VS-mode 访问 sstateen 时表现为只读零

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-VSPROP-01 | hstateen0.C=0 传播到 VS-mode sstateen0.C | 设 hstateen0.C=0 且 bit63=1，VS-mode 写 sstateen0.C=1 后读回 | sstateen0.C 在 VS-mode 读回 0 | `norm:sstateen_vsmode_access_roz` |
| SS-VSPROP-02 | hstateen0.C=1 解除 VS-mode 传播 | 设 hstateen0.C=1 且 bit63=1，VS-mode 写 sstateen0.C=1 后读回 | sstateen0.C 在 VS-mode 读回 1 | `norm:sstateen_vsmode_access_roz` |
| SS-VSPROP-03 | hstateen0.JVT=0 传播到 VS-mode sstateen0.JVT | 设 hstateen0.JVT=0 且 bit63=1，VS-mode 写 sstateen0.JVT=1 后读回 | sstateen0.JVT 在 VS-mode 读回 0 | `norm:sstateen_vsmode_access_roz` |
| SS-VSPROP-04 | hstateen0 多位同时传播 | 设 hstateen0 多个功能位为 0，VS-mode 逐一验证 sstateen0 对应位 | 所有对应位在 VS-mode 下为只读零 | `norm:sstateen_vsmode_access_roz` |
| SS-VSPROP-05 | hstateen0 位从 0 改 1 后传播解除 | 先设 hstateen0.C=0 验证传播，再改为 1 | sstateen0.C 在 VS-mode 变为可写 | `norm:sstateen_vsmode_access_roz` |

**测试步骤示例 (SS-VSPROP-01)**：
1. 在 M-mode 下设置 mstateen0.SE0=1、mstateen0.C=1
2. 切换到 HS-mode
3. 设置 hstateen0 bit63=1 且 hstateen0.C=0
4. 配置 VS-mode 环境
5. 切换到 VS-mode
6. 向 sstateen0 的 C 位写 1
7. 读回 sstateen0
8. 返回 HS-mode/M-mode 检查结果

**预期结果**：
- VS-mode 下 sstateen0.C 读回为 0（被 hstateen0 传播为只读零）

**断言**：
```
write_csr(sstateen0, STATEEN0_C_BIT);
uint32_t val = read_csr(sstateen0);
TEST_ASSERT_EQ("sstateen0.C in VS-mode should be RO0 when hstateen0.C=0",
               val & STATEEN0_C_BIT, 0);
```

---

### Group 10：hstateen0 各功能位控制

> **[迁移]** 本组 18 个测试已迁移至 `Hypervisor_cross_test_plan.md` Group 8.4，ID 映射：SS-HSE0-01~03 → HCROSS-SSSTA-21~23，SS-HENVCFG-01~03 → HCROSS-SSSTA-24~26，SS-HCSRIND-01~03 → HCROSS-SSSTA-27~29，SS-HIMSIC-01~03 → HCROSS-SSSTA-30~32，SS-HAIA-01~03 → HCROSS-SSSTA-33~35，SS-HCTX-01~03 → HCROSS-SSSTA-36~38。

**规范依据**：
- `norm:hstateen0_SE0_op`：hstateen0.SE0 (bit 63) 控制 VS-mode 对 sstateen0 的访问
- `norm:hstateen0_envcfg_op`：hstateen0.ENVCFG (bit 62) 控制 VS-mode 对 senvcfg 的访问
- `norm:hstateen0_csrind_op`：hstateen0.CSRIND (bit 60) 控制 VS-mode 对 siselect/sireg*（实为 vsiselect/vsireg*）的访问
- `norm:hstateen0_imsic_op`：hstateen0.IMSIC (bit 58) 控制 guest IMSIC 状态及 vstopei
- `norm:hstateen0_aia_op`：hstateen0.AIA (bit 59) 控制 Ssaia 非 CSRIND/IMSIC 的剩余状态
- `norm:hstateen0_context_op`：hstateen0.CONTEXT (bit 57) 控制 VS-mode 对 scontext 的访问

#### 10.1 SE0 位（bit 63）— sstateen0 VS-mode 访问

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-HSE0-01 | hstateen0.SE0=0 阻止 VS-mode 读 sstateen0 | 设 hstateen0.SE0=0，VS-mode 读 sstateen0 | 触发 virtual-instruction 异常 | `norm:hstateen0_SE0_op` |
| SS-HSE0-02 | hstateen0.SE0=0 阻止 VS-mode 写 sstateen0 | 设 hstateen0.SE0=0，VS-mode 写 sstateen0 | 触发 virtual-instruction 异常 | `norm:hstateen0_SE0_op` |
| SS-HSE0-03 | hstateen0.SE0=1 允许 VS-mode 访问 sstateen0 | 设 hstateen0.SE0=1，VS-mode 读写 sstateen0 | 访问正常 | `norm:hstateen0_SE0_op` |

#### 10.2 ENVCFG 位（bit 62）— senvcfg VS-mode 访问

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-HENVCFG-01 | hstateen0.ENVCFG=0 阻止 VS-mode 读 senvcfg | 设 ENVCFG=0，VS-mode 读 senvcfg | 触发 virtual-instruction 异常 | `norm:hstateen0_envcfg_op` |
| SS-HENVCFG-02 | hstateen0.ENVCFG=0 阻止 VS-mode 写 senvcfg | 设 ENVCFG=0，VS-mode 写 senvcfg | 触发 virtual-instruction 异常 | `norm:hstateen0_envcfg_op` |
| SS-HENVCFG-03 | hstateen0.ENVCFG=1 允许 VS-mode 访问 senvcfg | 设 ENVCFG=1，VS-mode 读写 senvcfg | 访问正常 | `norm:hstateen0_envcfg_op` |

#### 10.3 CSRIND 位（bit 60）— siselect/sireg VS-mode 访问

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-HCSRIND-01 | hstateen0.CSRIND=0 阻止 VS-mode 读 siselect | 设 CSRIND=0，VS-mode 读 siselect (实为 vsiselect) | 触发 virtual-instruction 异常 | `norm:hstateen0_csrind_op` |
| SS-HCSRIND-02 | hstateen0.CSRIND=0 阻止 VS-mode 读 sireg* | 设 CSRIND=0，VS-mode 读 sireg (实为 vsireg) | 触发 virtual-instruction 异常 | `norm:hstateen0_csrind_op` |
| SS-HCSRIND-03 | hstateen0.CSRIND=1 允许 VS-mode 访问 | 设 CSRIND=1，VS-mode 读 siselect/sireg* | 访问正常 | `norm:hstateen0_csrind_op` |

#### 10.4 IMSIC 位（bit 58）— Guest IMSIC 控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-HIMSIC-01 | hstateen0.IMSIC=0 阻止 VS-mode 访问 IMSIC | 设 IMSIC=0，VS-mode 读 stopei (实为 vstopei) | 触发 virtual-instruction 异常 | `norm:hstateen0_imsic_op` |
| SS-HIMSIC-02 | hstateen0.IMSIC=1 允许 VS-mode 访问 IMSIC | 设 IMSIC=1，VS-mode 读 stopei | 访问正常 | `norm:hstateen0_imsic_op` |
| SS-HIMSIC-03 | hstateen0.IMSIC=0 等效 VGEIN=0 | 设 IMSIC=0，验证 VS-mode 无法访问 IMSIC，等效于 hstatus.VGEIN=0 | VS-mode 无法访问 guest IMSIC | `norm:hstateen0_imsic_op` |

#### 10.5 AIA 位（bit 59）— Ssaia 剩余状态 VS-mode 控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-HAIA-01 | hstateen0.AIA=0 阻止 VS-mode Ssaia 状态 | 设 AIA=0，VS-mode 访问 Ssaia 非 CSRIND/IMSIC 状态 | 触发 virtual-instruction 异常 | `norm:hstateen0_aia_op` |
| SS-HAIA-02 | hstateen0.AIA=1 允许 VS-mode Ssaia 状态 | 设 AIA=1，VS-mode 访问 Ssaia 剩余状态 | 访问正常 | `norm:hstateen0_aia_op` |
| SS-HAIA-03 | hstateen0.AIA 不影响 CSRIND/IMSIC 控制 | 设 AIA=0 但 CSRIND=1、IMSIC=1，VS-mode 访问 siselect/stopei | 访问正常（AIA 不控制 CSRIND/IMSIC 管辖的状态） | `norm:hstateen0_aia_op` |

#### 10.6 CONTEXT 位（bit 57）— scontext VS-mode 控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-HCTX-01 | hstateen0.CONTEXT=0 阻止 VS-mode 读 scontext | 设 CONTEXT=0，VS-mode 读 scontext | 触发 virtual-instruction 异常 | `norm:hstateen0_context_op` |
| SS-HCTX-02 | hstateen0.CONTEXT=0 阻止 VS-mode 写 scontext | 设 CONTEXT=0，VS-mode 写 scontext | 触发 virtual-instruction 异常 | `norm:hstateen0_context_op` |
| SS-HCTX-03 | hstateen0.CONTEXT=1 允许 VS-mode 访问 scontext | 设 CONTEXT=1，VS-mode 读写 scontext | 访问正常 | `norm:hstateen0_context_op` |

---

### Group 11：hstateen 只读约束

> **[迁移]** 本组 7 个测试已迁移至 `Hypervisor_cross_test_plan.md` Group 8.5，ID 映射：SS-HRO-01~07 → HCROSS-SSSTA-39~45。

**规范依据**：
- `norm:hstateen_ro1_bits`：hstateen 中的位不能为 RO1，除非 mstateen 同位也为 RO1
- `norm:stateen_warl_access`：每个标准定义位都是 WARL
- `norm:stateen_unimplemented_state_roz`：控制未实现状态的位为只读零
- `norm:stateen_reserved_roz`：保留位为只读零

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-HRO-01 | hstateen0 RO1 位约束 | 识别 hstateen0 中任何 RO1 位，验证 mstateen0 同位也是 RO1 | mstateen0 同位也为 RO1 | `norm:hstateen_ro1_bits` |
| SS-HRO-02 | hstateen1 RO1 位约束 | 识别 hstateen1 中任何 RO1 位，验证 mstateen1 同位也是 RO1 | mstateen1 同位也为 RO1 | `norm:hstateen_ro1_bits` |
| SS-HRO-03 | hstateen2 RO1 位约束 | 识别 hstateen2 中任何 RO1 位，验证 mstateen2 同位也是 RO1 | mstateen2 同位也为 RO1 | `norm:hstateen_ro1_bits` |
| SS-HRO-04 | hstateen3 RO1 位约束 | 识别 hstateen3 中任何 RO1 位，验证 mstateen3 同位也是 RO1 | mstateen3 同位也为 RO1 | `norm:hstateen_ro1_bits` |
| SS-HRO-05 | hstateen0 保留位只读零 | 向 hstateen0 WPRI 域写 1 后读回 | 保留位读回 0 | `norm:stateen_reserved_roz` |
| SS-HRO-06 | hstateen0 未实现扩展位只读零 | 对于未实现的扩展对应位，写 1 后读回 | 对应位读回 0 | `norm:stateen_unimplemented_state_roz` |
| SS-HRO-07 | hstateen0 WARL 写入合法值 | 向 hstateen0 写入合法值后读回 | 读回值与写入值一致（限可写位） | `norm:stateen_warl_access` |

**测试步骤示例 (SS-HRO-01)**：
1. 在 M-mode 下设置 mstateen0.SE0=1
2. 切换到 HS-mode
3. 对 hstateen0 逐位尝试写 0
4. 读回 hstateen0
5. 如果某位读回为 1（RO1），返回 M-mode
6. 在 M-mode 尝试清除 mstateen0 同位
7. 读回 mstateen0，验证同位也是 RO1

**预期结果**：
- 如果 hstateen0 某位为 RO1，则 mstateen0 同位也必须为 RO1

**断言**：
```
clear_csr(hstateen0, test_bit);
uint64_t hsta_val = read_csr(hstateen0);
if (hsta_val & test_bit) {
    /* hstateen0 该位为 RO1，回 M-mode 检查 mstateen0 */
    goto_priv(PRIV_M);
    clear_csr(mstateen0, test_bit);
    uint64_t msta_val = read_csr(mstateen0);
    TEST_ASSERT("mstateen0 same bit must also be RO1", msta_val & test_bit);
}
```

---

### Group 12：hstateen 编码与 mstateen 一致性

> **[迁移]** 本组 5 个测试已迁移至 `Hypervisor_cross_test_plan.md` Group 8.6，ID 映射：SS-HENC-01~05 → HCROSS-SSSTA-46~50。

**规范依据**：
- `norm:hstateen_encoding`：hstateen CSR 的编码与 mstateen CSR 一致，区别仅在于 hstateen 控制 VS/VU 模式的访问

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| SS-HENC-01 | hstateen0 位域与 mstateen0 一致 | 对比 hstateen0 与 mstateen0 的可写位掩码 | 位域定义一致（C/FCSR/JVT/SE0/ENVCFG/CSRIND/IMSIC/AIA/CONTEXT 等位位置相同） | `norm:hstateen_encoding` |
| SS-HENC-02 | hstateen0 功能位与 mstateen0 对称 | 写 hstateen0 全 1，读回有效位；写 mstateen0 全 1，读回有效位；对比重叠部分 | hstateen0 有效位应是 mstateen0 有效位的子集（mstateen0 可能有 P1P13/SRMCFG 等 hstateen0 无对应的位） | `norm:hstateen_encoding` |
| SS-HENC-03 | hstateen1 编码与 mstateen1 一致 | 对比 hstateen1 与 mstateen1 的可写位掩码 | 位域定义一致 | `norm:hstateen_encoding` |
| SS-HENC-04 | hstateen2 编码与 mstateen2 一致 | 对比 hstateen2 与 mstateen2 的可写位掩码 | 位域定义一致 | `norm:hstateen_encoding` |
| SS-HENC-05 | hstateen3 编码与 mstateen3 一致 | 对比 hstateen3 与 mstateen3 的可写位掩码 | 位域定义一致 | `norm:hstateen_encoding` |

**测试步骤示例 (SS-HENC-01)**：
1. 在 M-mode 下向 mstateen0 写全 1，读回获取 mstateen0 可写掩码
2. 设置 mstateen0.SE0=1
3. 切换到 HS-mode
4. 向 hstateen0 写全 1，读回获取 hstateen0 可写掩码
5. 返回 M-mode
6. 对比两个掩码中重叠部分的位域位置

**预期结果**：
- hstateen0 的 C (bit 0)、FCSR (bit 1)、JVT (bit 2)、CONTEXT (bit 57)、IMSIC (bit 58)、AIA (bit 59)、CSRIND (bit 60)、ENVCFG (bit 62)、SE0 (bit 63) 位的位置与 mstateen0 完全一致

**断言**：
```
/* 已知功能位掩码 */
uint64_t known_bits = STATEEN0_C | STATEEN0_FCSR | STATEEN0_JVT |
                      STATEEN0_CONTEXT | STATEEN0_IMSIC | STATEEN0_AIA |
                      STATEEN0_CSRIND | STATEEN0_ENVCFG | STATEEN0_SE0;
/* hstateen0 中这些位应与 mstateen0 位置一致 */
TEST_ASSERT("hstateen0 known bits match mstateen0",
            (hstateen0_mask & known_bits) == (mstateen0_mask & known_bits));
```

---

## 5. 验证计划

### 5.1 自动化测试

```bash
# 编译 ssstateen 测试
make ssstateen CROSS_COMPILER=/path/to/riscv64-unknown-elf-

# 在 QEMU 上运行
qemu-system-riscv64 -M virt -cpu max -bios none \
    -kernel ssstateen/ssstateen_test.elf -m 256M -smp 1 -nographic

# 在 Spike 上运行
make spike-ssstateen CROSS_COMPILER=/path/to/riscv64-unknown-elf-

# 在 Sail 上运行
make sail-ssstateen CROSS_COMPILER=/path/to/riscv64-unknown-elf-
```

### 5.2 手动验证

- 验证所有 normative rules 是否被至少一个测试用例覆盖
- 检查 H 扩展相关测试（Group 7-12）在无 H 扩展的平台上被正确跳过
- 确认 virtual-instruction 异常 (cause=22) 在 VS/VU 模式正确触发
- 验证 RV32 特有的 hstateen0h 测试覆盖
- 确认 mstateen 预配置正确，不影响 Ssstateen 独立测试的准确性

---

## 6. 测试判定标准

### 6.1 通过标准

- 所有测试用例的所有断言均通过
- 测试框架输出 `RESULT: ALL PASSED`
- 无非预期异常发生

### 6.2 失败标准

- 任一断言失败即判定对应测试用例 FAIL
- 出现非预期的异常（trap）即判定 FAIL
- sstateen/hstateen 的 WARL 位行为不符合规范即判定 FAIL

---

## 7. 覆盖率矩阵

### 7.1 Normative Rules 覆盖追溯

| 规范引用 | 覆盖测试 ID |
|----------|------------|
| `norm:sstateen_rv64_csrs` | SS-EXIST-01~05 |
| `norm:hstateen_rv64_csrs` | SS-HEXIST-01~05 |
| `norm:stateen_rv32_upper_bits_csrs` | SS-HEXIST-06 |
| `norm:stateen_op` | SS-UCTL-03, SS-EXC-01 |
| `norm:stateen_illegal_state_access` | SS-UCTL-01, SS-UCTL-04, SS-UCTL-06~09, SS-EXC-02~04 |
| `norm:stateen_implicit_state_update` | SS-EXC-05 |
| `norm:sstateen_user_access_control` | SS-UCTL-01~09 |
| `norm:sstateen_bit_allocation` | SS-EXIST-06, SS-ALLOC-01 |
| `norm:sstateen_encroachment_bits_roz` | SS-ALLOC-04 |
| `norm:hstateen_encoding` | SS-HENC-01~05 |
| `norm:stateen_warl_access` | SS-HRO-07 |
| `norm:stateen_unimplemented_state_roz` | SS-ALLOC-08, SS-HRO-06 |
| `norm:stateen_reserved_roz` | SS-FUNC-09, SS-HRO-05 |
| `norm:mstateen_lower_priv_roz` | SS-ALLOC-06~07 |
| `norm:sstateen_vsmode_access_roz` | SS-VSPROP-01~05 |
| `norm:sstateen_ro1_bits` | SS-ALLOC-02~03 |
| `norm:hstateen_ro1_bits` | SS-HRO-01~04 |
| `norm:hstateen_sstateen_zero_initialization` | SS-INIT-01~05 |
| `norm:hstateen_bit_63_op` | SS-HB63-03~05, SS-HB63-07~09 |
| `norm:hstateen_bit_63_writable` | SS-HB63-01~02, SS-HB63-06 |
| `norm:stateen0_c_op` | SS-UCTL-01~02, SS-FUNC-01~03 |
| `norm:stateen0_fcsr_op` | SS-UCTL-06, SS-FUNC-04~05 |
| `norm:mstateen0_fcsr_roz` | SS-FUNC-04 |
| `norm:stateen0_jvt_op` | SS-UCTL-04~05, SS-FUNC-06~08 |
| `norm:hstateen0_SE0_op` | SS-HSE0-01~03 |
| `norm:hstateen0_envcfg_op` | SS-HENVCFG-01~03 |
| `norm:hstateen0_csrind_op` | SS-HCSRIND-01~03 |
| `norm:hstateen0_imsic_op` | SS-HIMSIC-01~03 |
| `norm:hstateen0_aia_op` | SS-HAIA-01~03 |
| `norm:hstateen0_context_op` | SS-HCTX-01~03 |
| `norm:mstateen_bit_correspondence` | SS-ALLOC-05 |
