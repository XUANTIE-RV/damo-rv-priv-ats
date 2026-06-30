# Sscsrind 扩展 Supervisor Mode 测试计划

> 本文档描述 Sscsrind (Supervisor-level Indirect CSR Access) 扩展中 Supervisor Mode 相关功能的测试计划。Sscsrind 扩展为 S-mode 提供间接 CSR 访问机制（siselect/sireg\*），并在 H 扩展存在时提供 VS-level 间接 CSR 访问（vsiselect/vsireg\*）。
>
> 生成时间：2026-06-25

---

## 范围

### 覆盖的功能

- **S-mode CSR 基本功能**：`siselect`、`sireg`~`sireg6` 的存在性、可访问性、最小范围、宽度
- **State-Enable 访问控制**：`mstateen0[60]` 对 S-mode 访问 siselect/sireg\* 的控制

### 不在本文档范围

- Machine-level CSR（miselect/mireg\*）的测试 — 由 `Smcsrind_test_plan.md` 覆盖
- `mstateen0[60]` 对 M-mode 自身访问的影响验证 — 由 `Smcsrind_test_plan.md` Group 4 覆盖
- 依赖 Sscsrind 的具体扩展（如 Ssaia、Ssccfg）的间接寄存器内容测试 — 由各自扩展的测试计划覆盖
- VS-level CSR（vsiselect/vsireg\*）基本功能 — 已迁移到 `Hypervisor_cross_test_plan.md` Group 11.1 (HCROSS-SSCSRIND-01~10)
- Virtual-instruction 异常行为 — 已迁移到 `Hypervisor_cross_test_plan.md` Group 11.2 (HCROSS-SSCSRIND-11~21)
- State-Enable H 扩展场景（hstateen0[60] 控制、mstateen0[60] 对 HS-mode 控制） — 已迁移到 `Hypervisor_cross_test_plan.md` Group 11.3 (HCROSS-SSCSRIND-22~27)
- VS-mode 通过 sireg\* 透明访问 vsireg\* 的重映射行为 — 已迁移到 `Hypervisor_cross_test_plan.md` Group 11.4 (HCROSS-SSCSRIND-28~33)
- `hstateen0[60]` 在 Smstateen 测试体系中的全面验证 — 由 `Hypervisor_cross_test_plan.md` Group 8 覆盖

---

## 规范依据

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:siselect_min_range` | `smcsrind.adoc` | The `siselect` register will support the value range 0..0xFFF at a minimum. A future extension may define a value range outside of this minimum range. | `siselect` 寄存器至少支持值范围 0..0xFFF。未来扩展可能定义此最小范围之外的值。 |
| `norm:siselect_msb_op` | `smcsrind.adoc` | Values of `siselect` with the most-significant bit set (bit XLEN - 1 = 1) are designated only for custom use. Values with MSB clear are designated only for standard use and are reserved. If XLEN is changed, the MSB moves to the new position, retaining its value. | MSB=1 的 `siselect` 值仅用于自定义用途。MSB=0 用于标准用途并保留。XLEN 变化时 MSB 移动并保留值。 |
| `norm:sireg_access_on_illegal_siselect` | `smcsrind.adoc` | The behavior upon accessing `sireg*` from M-mode or S-mode, while `siselect` holds a value that is not implemented at supervisor level, is UNSPECIFIED. | 当 `siselect` 持有在 S-level 未实现的值时，从 M-mode 或 S-mode 访问 `sireg*` 的行为是 UNSPECIFIED。 |
| `norm:sireg_access_on_legal_siselect` | `smcsrind.adoc` | Attempts to access `sireg*` from M-mode or S-mode while `siselect` holds a number in a standard-defined and implemented range result in specific behavior defined by the extension. | 当 `siselect` 持有标准定义和实现范围内的值时，从 M-mode 或 S-mode 访问 `sireg*` 的行为由对应扩展定义。 |
| `norm:sireg_access_behaviour` | `smcsrind.adoc` | Ordinarily, each `sireg*_i_* will access register state, access read-only 0 state, or, unless executing in a virtual machine, raise an illegal-instruction exception. | 通常，每个 `sireg_i` 访问寄存器状态、只读 0 状态或（除非在虚拟机中）触发 illegal-instruction 异常。 |
| `norm:sscsrind_smode_csrs_sz` | `smcsrind.adoc` | The widths of `siselect` and `sireg*` are always the current XLEN rather than SXLEN. | `siselect` 和 `sireg*` 的宽度始终是当前 XLEN 而非 SXLEN。 |
| `norm:vsiselect_min_range` | `smcsrind.adoc` | The `vsiselect` register will support the value range 0..0xFFF at a minimum. | `vsiselect` 寄存器至少支持值范围 0..0xFFF。 |
| `norm:vsiselect_msb_op` | `smcsrind.adoc` | Values of `vsiselect` with the most-significant bit set (bit XLEN - 1 = 1) are designated only for custom use. Values with MSB clear are for standard use and reserved. | MSB=1 的 `vsiselect` 值仅用于自定义用途。MSB=0 用于标准用途并保留。 |
| `norm:sscsrind_virtual_inst_fault` | `smcsrind.adoc` | A virtual-instruction exception is raised for attempts from VS-mode or VU-mode to directly access `vsiselect` or `vsireg*`, or attempts from VU-mode to access `siselect` or `sireg*`. | VS-mode 或 VU-mode 直接访问 `vsiselect`/`vsireg*`，或 VU-mode 访问 `siselect`/`sireg*` 触发 virtual-instruction 异常。 |
| `norm:vsireg_access_on_legal_vsiselect` | `smcsrind.adoc` | While `vsiselect` holds a number in a standard-defined and implemented range, attempts to access `vsireg*` from a sufficiently privileged mode, or `sireg*` (really `vsireg*`) from VS-mode, result in specific behavior defined by the extension. | 当 `vsiselect` 持有标准定义和实现范围内的值时，访问 `vsireg*` 或 VS-mode 访问 `sireg*`（实际为 `vsireg*`）的行为由对应扩展定义。 |
| `norm:vsireg_access_behaviour` | `smcsrind.adoc` | Ordinarily, each `vsireg*_i_* will access register state, access read-only 0 state, or raise an exception (either illegal-instruction or, for select accesses from VS-mode, virtual-instruction). | 通常，每个 `vsireg_i` 访问寄存器状态、只读 0 状态或触发异常（illegal-instruction 或 VS-mode 下的 virtual-instruction）。 |
| `norm:vsmode_virtual_inst_fault` | `smcsrind.adoc` | When `vsiselect` holds a value that is implemented at HS level but not at VS level, attempts to access `sireg*` (really `vsireg*`) from VS-mode will typically raise a virtual-instruction exception. | 当 `vsiselect` 持有在 HS-level 实现但 VS-level 未实现的值时，VS-mode 访问 `sireg*`（实际为 `vsireg*`）通常触发 virtual-instruction 异常。 |
| `norm:sscsrind_vsmode_csrs_sz` | `smcsrind.adoc` | The widths of `vsiselect` and `vsireg*` are always the current XLEN rather than VSXLEN. | `vsiselect` 和 `vsireg*` 的宽度始终是当前 XLEN 而非 VSXLEN。 |
| `norm:sscsrind_csrs_access_control` | `smcsrind.adoc` | If Smstateen is implemented together with Smcsrind, bit 60 of `mstateen0` controls access to `siselect`, `sireg*`, `vsiselect`, and `vsireg*`. When `mstateen0`[60]=0, access from below M-mode raises illegal-instruction. | `mstateen0[60]` 控制对 siselect/sireg\*/vsiselect/vsireg\* 的访问。为 0 时低于 M-mode 的访问触发 illegal-instruction。 |
| `norm:hypervisor_impl_csrs_access_control` | `smcsrind.adoc` | If hypervisor extension is implemented, bit 60 is also defined in `hstateen0`, controlling access to `siselect` and `sireg*` (really `vsiselect`/`vsireg*`). When `hstateen0`[60]=0 and `mstateen0`[60]=1, VS/VU-mode access raises virtual-instruction, not illegal-instruction. | H 扩展存在时 `hstateen0[60]` 控制 VS/VU-mode 对 siselect/sireg\*（实际为 vsiselect/vsireg\*）的访问。hstateen0[60]=0 且 mstateen0[60]=1 时触发 virtual-instruction。 |
| `norm:csrs_alias` | `smcsrind.adoc` | Machine-level and Supervisor-level CSRs with the same select value may be defined as partial or full aliases. | 具有相同选择值的 M-level 和 S-level CSR 可定义为别名。 |

---

## S-mode CSR 表

| 编号 | 特权级 | 宽度 | 名称 | 描述 |
|------|--------|------|------|------|
| 0x150 | SRW | XLEN | `siselect` | Supervisor indirect register select |
| 0x151 | SRW | XLEN | `sireg` | Supervisor indirect register alias |
| 0x152 | SRW | XLEN | `sireg2` | Supervisor indirect register alias 2 |
| 0x153 | SRW | XLEN | `sireg3` | Supervisor indirect register alias 3 |
| 0x155 | SRW | XLEN | `sireg4` | Supervisor indirect register alias 4 |
| 0x156 | SRW | XLEN | `sireg5` | Supervisor indirect register alias 5 |
| 0x157 | SRW | XLEN | `sireg6` | Supervisor indirect register alias 6 |

## VS-mode CSR 表（需 H 扩展）

| 编号 | 特权级 | 宽度 | 名称 | 描述 |
|------|--------|------|------|------|
| 0x250 | HRW | XLEN | `vsiselect` | Virtual supervisor indirect register select |
| 0x251 | HRW | XLEN | `vsireg` | Virtual supervisor indirect register alias |
| 0x252 | HRW | XLEN | `vsireg2` | Virtual supervisor indirect register alias 2 |
| 0x253 | HRW | XLEN | `vsireg3` | Virtual supervisor indirect register alias 3 |
| 0x255 | HRW | XLEN | `vsireg4` | Virtual supervisor indirect register alias 4 |
| 0x256 | HRW | XLEN | `vsireg5` | Virtual supervisor indirect register alias 5 |
| 0x257 | HRW | XLEN | `vsireg6` | Virtual supervisor indirect register alias 6 |

> [!NOTE]
> VS CSR 编号 0x254 被 `vsiph` 占用（与 M-mode 的 miph 对应），因此 vsireg* 编号不连续。

---

## Group 1. S-mode CSR 基本功能

**规范依据**：
- `norm:siselect_min_range`：siselect 至少支持 0..0xFFF
- `norm:siselect_msb_op`：MSB 语义
- `norm:sireg_access_on_illegal_siselect`：未实现 siselect 值时 sireg* 行为 UNSPECIFIED
- `norm:sireg_access_on_legal_siselect`：合法 siselect 值下 sireg* 行为由扩展定义
- `norm:sireg_access_behaviour`：sireg_i 访问寄存器状态、只读 0 或 illegal-instruction
- `norm:sscsrind_smode_csrs_sz`：宽度始终为当前 XLEN

**测试职责**：验证 siselect/sireg\* 的存在性、可访问性、最小支持范围、宽度特性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCSRIND-SCSR-01 | siselect 在 S-mode 可读 | S-mode 读 siselect (0x150) | 无异常，返回合法值 |
| SSCSRIND-SCSR-02 | siselect 在 S-mode 可写 | S-mode 写 siselect 后读回 | 无异常，写 0 读回 0（WARL） |
| SSCSRIND-SCSR-03 | siselect 最小范围验证 (0..0xFFF) | S-mode 逐一写 siselect=0, 1, 0x100, 0x800, 0xFFF 后读回 | 所有值均被接受（写入不触发异常），读回为合法值 |
| SSCSRIND-SCSR-04 | siselect MSB=1 自定义区域 | S-mode 写 siselect 为 MSB=1 的值（如 0x8000000000000000） | WARL 行为：写入不异常，读回为合法值 |
| SSCSRIND-SCSR-05 | siselect MSB=0 标准保留区域 | S-mode 写 siselect 为 MSB=0 的非标准分配值 | WARL 行为：写入不异常，读回为合法值 |
| SSCSRIND-SCSR-06 | sireg 在 S-mode 可访问 | S-mode 读 sireg (0x151)（siselect 设为 0） | 行为取决于 siselect 值：可能返回只读 0 或触发 illegal-instruction |
| SSCSRIND-SCSR-07 | sireg2~sireg6 在 S-mode 可访问 | S-mode 逐一读 sireg2~sireg6（siselect 设为 0） | 行为取决于 siselect 值：可能返回只读 0 或触发 illegal-instruction |
| SSCSRIND-SCSR-08 | siselect/sireg* 宽度 = 当前 XLEN | 在 M-mode（XLEN=64）和 S-mode 分别读 siselect，对比宽度 | 宽度始终为当前 XLEN（M-mode 下为 MXLEN，S-mode 下为 SXLEN） |
| SSCSRIND-SCSR-09 | siselect WARL 全 1 写入 | S-mode 向 siselect 写全 1 (0xFFFFFFFFFFFFFFFF) | 不触发异常，读回为合法值 |
| SSCSRIND-SCSR-10 | sireg* 在合法 siselect 下的行为 | 若存在已实现的 siselect 值范围（如 Ssaia 定义的 0x30~0x3F），S-mode 设置该值后读 sireg | 行为由对应扩展定义：访问寄存器状态、只读 0 或 illegal-instruction |

> [!NOTE]
> - S-mode CSR（siselect/sireg*）要求 S-mode 实现时必须有。S-mode 是否实现可通过 misa.S 位判断。
> - SSCSRIND-SCSR-03 验证 siselect 的最小范围 0..0xFFF。SPEC 要求 siselect 至少支持这个范围，即使大部分空间可能保留或不可访问。这确保了 M-mode 可以在此范围内模拟间接访问寄存器。
> - SSCSRIND-SCSR-06~07 的行为取决于当前 siselect 值。siselect=0 时，由于 0 值范围是保留的（未被任何标准扩展分配），访问 sireg* 行为 UNSPECIFIED，预期触发 illegal-instruction 或返回只读 0。
> - SSCSRIND-SCSR-08 验证宽度特性：SPEC 明确指出 siselect/sireg* 宽度始终为当前 XLEN 而非 SXLEN。在 MXLEN=64 且 SXLEN=32 的配置下，M-mode 访问 siselect 为 64 位，S-mode 访问为 32 位。
> - SSCSRIND-SCSR-10 依赖已实现的扩展。例如 Ssaia 定义了 siselect=0x30~0x3F 范围，用于通过 sireg/sireg2 访问 sieh/siph 等。若无此类扩展，应 TEST_SKIP。

---

## Group 2. VS-level CSR 基本功能（已迁移）

> **本组测试已迁移到 `Hypervisor_cross_test_plan.md` Group 11.1（HCROSS-SSCSRIND-01~10）。**
>
> 本组所有测试需要 H 扩展，属于 Hypervisor 交叉测试范畴。详见：
> - **测试用例**：`Hypervisor_cross_test_plan.md` Group 11.1
> - **规范依据**：`norm:vsiselect_min_range`、`norm:vsiselect_msb_op`、`norm:vsireg_access_on_legal_vsiselect`、`norm:vsireg_access_behaviour`、`norm:sscsrind_vsmode_csrs_sz`
> - **测试职责**：验证 vsiselect/vsireg\* 的存在性、可访问性、最小支持范围（需 H 扩展）

---

## Group 3. Virtual-Instruction 异常行为（已迁移）

> **本组测试已迁移到 `Hypervisor_cross_test_plan.md` Group 11.2（HCROSS-SSCSRIND-11~21）。**
>
> 本组所有测试需要 H 扩展，属于 Hypervisor 交叉测试范畴。详见：
> - **测试用例**：`Hypervisor_cross_test_plan.md` Group 11.2
> - **规范依据**：`norm:sscsrind_virtual_inst_fault`、`norm:vsmode_virtual_inst_fault`
> - **测试职责**：验证 VS-mode/VU-mode 对间接 CSR 的访问正确触发 virtual-instruction 异常（cause=22），而非 illegal-instruction（cause=2）
> - **ID 映射**：SSCSRIND-VI-01~11 → HCROSS-SSCSRIND-11~21

---

## Group 4. State-Enable 访问控制

**规范依据**：
- `norm:sscsrind_csrs_access_control`：mstateen0[60] 控制 S-mode 及以下对 siselect/sireg\*/vsiselect/vsireg\* 的访问
- `norm:hypervisor_impl_csrs_access_control`：H 扩展时 hstateen0[60] 控制 VS/VU-mode 对 siselect/sireg\*（实际 vsiselect/vsireg\*）的访问

**测试职责**：验证 mstateen0[60] 和 hstateen0[60] 对各特权级的访问控制行为。

### 4.1 mstateen0[60] 对 S-mode 的控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSCSRIND-STA-01 | mstateen0[60]=0 阻止 S-mode 读 siselect | mstateen0[60]=0，S-mode 读 siselect | illegal-instruction 异常 (cause=2) |
| SSCSRIND-STA-02 | mstateen0[60]=0 阻止 S-mode 读 sireg | mstateen0[60]=0，S-mode 读 sireg | illegal-instruction 异常 (cause=2) |
| SSCSRIND-STA-03 | mstateen0[60]=0 阻止 S-mode 写 siselect | mstateen0[60]=0，S-mode 写 siselect | illegal-instruction 异常 (cause=2) |
| SSCSRIND-STA-04 | mstateen0[60]=1 允许 S-mode 访问 siselect/sireg* | mstateen0[60]=1，S-mode 读写 siselect 和 sireg | 访问正常，无异常 |

### 4.2 mstateen0[60] 对 VS/VU-mode 的控制（已迁移）

> **本小节测试已迁移到 `Hypervisor_cross_test_plan.md` Group 11.3（HCROSS-SSCSRIND-22~23）。**
>
> 本小节所有测试需要 H 扩展，属于 Hypervisor 交叉测试范畴。详见：
> - **测试用例**：`Hypervisor_cross_test_plan.md` Group 11.3
> - **规范依据**：`norm:sscsrind_csrs_access_control`
> - **ID 映射**：SSCSRIND-STA-05~06 → HCROSS-SSCSRIND-22~23

### 4.3 hstateen0[60] 对 VS/VU-mode 的控制（已迁移）

> **本小节测试已迁移到 `Hypervisor_cross_test_plan.md` Group 11.3（HCROSS-SSCSRIND-24~27）。**
>
> 本小节所有测试需要 H 扩展，属于 Hypervisor 交叉测试范畴。详见：
> - **测试用例**：`Hypervisor_cross_test_plan.md` Group 11.3
> - **规范依据**：`norm:hypervisor_impl_csrs_access_control`
> - **ID 映射**：SSCSRIND-STA-07~10 → HCROSS-SSCSRIND-24~27
> - **交叉引用**：与 Group 8.4 (HCROSS-SSSTA-27~29) 覆盖相同规范点

> [!NOTE]
> - SSCSRIND-STA-01~04 与 `Smcsrind_test_plan.md` Group 4 (MCSRIND-STA-01~06) 覆盖相同规范点。从 Sscsrind 角度的测试聚焦于 S-mode CSR 的被控行为，从 Smcsrind 角度的测试聚焦于 mstateen0 的控制机制。实现时选择一处实现并交叉引用。
> - SSCSRIND-STA-05~10 已迁移到 `Hypervisor_cross_test_plan.md` Group 11.3。

---

## Group 5. Hypervisor 交叉测试（已迁移）

> **本组测试已迁移到 `Hypervisor_cross_test_plan.md` Group 11.4（HCROSS-SSCSRIND-28~33）。**
>
> 本组所有测试需要 H 扩展，属于 Hypervisor 交叉测试范畴。详见：
> - **测试用例**：`Hypervisor_cross_test_plan.md` Group 11.4
> - **规范依据**：`norm:vsireg_access_on_legal_vsiselect`、`norm:vsireg_access_behaviour`、`norm:csrs_alias`
> - **测试职责**：验证 VS-mode 通过 sireg* 的硬件重映射访问 vsireg* 的行为，以及 HS-mode/VS-mode 的间接 CSR 交互
> - **ID 映射**：SSCSRIND-HYP-01~06 → HCROSS-SSCSRIND-28~33

### 代码示例

> **注意**：以下代码示例仅保留非 H 扩展场景的测试（Group 1 和 Group 4.1）。H 扩展相关测试的代码示例请参见 `Hypervisor_cross_test_plan.md` Group 11。

```c
/* SSCSRIND-SCSR-03: siselect minimum range 0..0xFFF */
TEST_REGISTER(test_sscsrind_scsr_03);
bool test_sscsrind_scsr_03(void) {
    TEST_BEGIN("SSCSRIND-SCSR-03: siselect minimum range 0..0xFFF");

    if (!(CSRR(misa) & (1UL << ('S' - 'A')))) {
        TEST_SKIP("S-mode not implemented");
    }

    /* Ensure Smstateen allows S-mode access */
    if (platform_has_smstateen()) {
        CSRS(0x30C, (1ULL << 60));  /* mstateen0[60] = 1 */
    }

    goto_priv(PRIV_S);

    /* Test several values within the 0..0xFFF range */
    uintptr_t test_vals[] = {0, 1, 0x100, 0x800, 0xFFF};
    for (int i = 0; i < 5; i++) {
        PRIV_DO(CSRW(0x150, test_vals[i]));  /* siselect */
        /* WARL: writing should not cause exception.
         * Readback should be a legal value. */
    }

    goto_priv(PRIV_M);
    TEST_ASSERT("siselect accepts 0..0xFFF range", 1);

    TEST_END();
}

/* SSCSRIND-STA-01: mstateen0[60]=0 blocks S-mode siselect */
TEST_REGISTER(test_sscsrind_sta_01);
bool test_sscsrind_sta_01(void) {
    TEST_BEGIN("SSCSRIND-STA-01: mstateen0[60]=0 blocks S-mode siselect");

    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    uintptr_t orig = CSRR(0x30C);  /* mstateen0 */

    /* Clear CSRIND bit (bit 60) */
    CSRC(0x30C, (1ULL << 60));

    /* S-mode tries to read siselect */
    goto_priv(PRIV_S);
    PRIV_DO(CSRR(0x150));  /* siselect */
    goto_priv(PRIV_M);
    CHECK_TRAP("illegal-instruction on S-mode siselect read",
               CAUSE_ILLEGAL_INST);

    /* Restore */
    CSRW(0x30C, orig);
    TEST_END();
}

/* NOTE: H-extension code examples (test_sscsrind_vi_01, test_sscsrind_sta_10,
 * test_sscsrind_hyp_01) have been moved to Hypervisor_cross_test_plan.md Group 11.
 * See HCROSS-SSCSRIND-11~33 for H-dependent test implementations. */
```

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1 (S-mode CSR) | SSCSRIND-SCSR-01~10 | siselect/sireg* 是 Sscsrind 的核心 CSR |
| P1（重要） | Group 4.1 (State-Enable S-mode) | SSCSRIND-STA-01~04 | 访问控制是安全隔离的关键 |

> **注意**：Group 2、Group 3、Group 4.2/4.3、Group 5 已迁移到 `Hypervisor_cross_test_plan.md` Group 11（HCROSS-SSCSRIND-01~33）。优先级参见该文档。

---

## 测试实现说明

### 文件组织

```
damo-priv-test/
├── Sscsrind/
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_sscsrind_smode.c      # Group 1: S-mode CSR 基本功能
│       └── test_sscsrind_stateen.c    # Group 4.1: State-Enable 控制 (S-mode only)
│
│   注意：以下 Group 已迁移到 Hypervisor_cross_test_plan.md Group 11：
│   - Group 2 (VS-level CSR) → HCROSS-SSCSRIND-01~10
│   - Group 3 (Virtual-Instruction) → HCROSS-SSCSRIND-11~21
│   - Group 4.2/4.3 (State-Enable H-ext) → HCROSS-SSCSRIND-22~27
│   - Group 5 (Hypervisor 交叉) → HCROSS-SSCSRIND-28~33
```

### 运行时检测

```c
static bool platform_has_sscsrind(void) {
    /* Check if siselect CSR is accessible */
    trap_expect_begin();
    CSRR(0x150);  /* siselect */
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

static bool platform_has_h_ext(void) {
    /* Check H extension via misa */
    uintptr_t misa = CSRR(misa);
    return (misa & (1UL << ('H' - 'A'))) != 0;
}

static bool platform_has_smstateen(void) {
    trap_expect_begin();
    CSRR(0x30C);  /* mstateen0 */
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}
```

### 关键注意事项

1. **CSR 地址**：
   - S-mode: siselect=0x150, sireg=0x151, sireg2=0x152, sireg3=0x153, sireg4=0x155, sireg5=0x156, sireg6=0x157
   - VS-mode: vsiselect=0x250, vsireg=0x251, vsireg2=0x252, vsireg3=0x253, vsireg4=0x255, vsireg5=0x256, vsireg6=0x257

2. **Virtual-Instruction vs Illegal-Instruction**：
   - VS/VU-mode 直接访问 vsiselect/vsireg*（0x250~0x257）→ **始终** virtual-instruction (cause=22)
   - VU-mode 访问 siselect/sireg*（0x150~0x157）→ **始终** virtual-instruction (cause=22)
   - S-mode 访问 siselect/sireg* 且 mstateen0[60]=0 → illegal-instruction (cause=2)
   - VS-mode 访问 siselect/sireg* 且 hstateen0[60]=0, mstateen0[60]=1 → virtual-instruction (cause=22)

3. **硬件重映射**：在 VM 内（V=1），VS-mode 访问 siselect/sireg*（0x150~0x157）时，硬件自动将其重映射为 vsiselect/vsireg*（0x250~0x257）。这不是"直接访问" vsiselect/vsireg*，因此不会无条件触发 virtual-instruction。异常行为取决于 vsiselect 值和 stateen 控制位。

4. **宽度特性**：siselect/sireg* 和 vsiselect/vsireg* 的宽度始终为当前 XLEN，而非 SXLEN/VSXLEN。在 MXLEN=64、SXLEN=32 的配置下，M-mode 访问 siselect 为 64 位，S-mode 访问为 32 位。

5. **框架修改**：需在 `common/encoding.h` 中添加 CSR 地址定义：
   ```c
   #define CSR_SISELECT   0x150
   #define CSR_SIREG      0x151
   #define CSR_SIREG2     0x152
   #define CSR_SIREG3     0x153
   #define CSR_SIREG4     0x155
   #define CSR_SIREG5     0x156
   #define CSR_SIREG6     0x157
   #define CSR_VSISELECT  0x250
   #define CSR_VSIREG     0x251
   #define CSR_VSIREG2    0x252
   #define CSR_VSIREG3    0x253
   #define CSR_VSIREG4    0x255
   #define CSR_VSIREG5    0x256
   #define CSR_VSIREG6    0x257
   ```

6. **与其他测试计划的交叉引用**：
   - Smstateen MSTA-CSRIND-01~04：从 Smstateen 角度验证 bit 60 控制。SSCSRIND-STA-01~04 从 Sscsrind 角度验证同一规范点。实现时选择一处，另一处交叉引用。
   - Hypervisor_cross HCROSS-SSSTA-27~29：从 Hypervisor 角度验证 hstateen0[60] 控制。SSCSRIND-STA-07~10 从 Sscsrind 角度验证同一规范点。实现时选择一处，另一处交叉引用。
   - Smcsrind MCSRIND-STA-01~10：从 Smcsrind 角度验证 mstateen0[60] 控制。SSCSRIND-STA-01~06 从 Sscsrind 角度验证。实现时应合并或交叉引用。

---

## 测试判定标准

| 判定 | 条件 |
|------|------|
| PASS | 所有测试断言通过 |
| FAIL | 任何测试断言失败，且与 SPEC 规范一致（即实现不符合 SPEC） |
| SKIP | 所需扩展未实现（Smstateen 未实现时 Group 4.1 SKIP） |
| UNSPECIFIED | SPEC 明确标注行为未指定的情况（如未实现 siselect 值的 sireg* 访问），记录实际行为 |

> **注意**：H 扩展相关测试（Group 2/3/4.2/4.3/5）已迁移到 `Hypervisor_cross_test_plan.md` Group 11。判定标准参见该文档。

---

## 覆盖率矩阵

| 规范 ID | 覆盖的测试 ID（本文档） | 已迁移到 Hypervisor_cross |
|---------|------------------------|--------------------------|
| `norm:siselect_min_range` | SSCSRIND-SCSR-03 | — |
| `norm:siselect_msb_op` | SSCSRIND-SCSR-04, SSCSRIND-SCSR-05 | — |
| `norm:sireg_access_on_illegal_siselect` | SSCSRIND-SCSR-06, SSCSRIND-SCSR-07 | — |
| `norm:sireg_access_on_legal_siselect` | SSCSRIND-SCSR-10 | — |
| `norm:sireg_access_behaviour` | SSCSRIND-SCSR-06, SSCSRIND-SCSR-10 | — |
| `norm:sscsrind_smode_csrs_sz` | SSCSRIND-SCSR-08 | — |
| `norm:vsiselect_min_range` | — | HCROSS-SSCSRIND-03 |
| `norm:vsiselect_msb_op` | — | HCROSS-SSCSRIND-04, HCROSS-SSCSRIND-05 |
| `norm:vsireg_access_on_legal_vsiselect` | — | HCROSS-SSCSRIND-10, HCROSS-SSCSRIND-28~32 |
| `norm:vsireg_access_behaviour` | — | HCROSS-SSCSRIND-06, HCROSS-SSCSRIND-10, HCROSS-SSCSRIND-31 |
| `norm:sscsrind_virtual_inst_fault` | — | HCROSS-SSCSRIND-11~20 |
| `norm:vsmode_virtual_inst_fault` | — | HCROSS-SSCSRIND-21 |
| `norm:sscsrind_vsmode_csrs_sz` | — | HCROSS-SSCSRIND-08 |
| `norm:sscsrind_csrs_access_control` | SSCSRIND-STA-01~04 | HCROSS-SSCSRIND-22~23 |
| `norm:hypervisor_impl_csrs_access_control` | — | HCROSS-SSCSRIND-24~27 |
| `norm:csrs_alias` | — | HCROSS-SSCSRIND-33 |

---

## 参考

- `SPEC/smcsrind.adoc` — Smcsrind/Sscsrind Extension for Indirect CSR Access
- `SPEC/smstateen.adoc` — Smstateen Extension (State Enable)
- `SPEC/hypervisor.adoc` — Hypervisor Extension
- `NORM/sm_norm.md` — 规范点汇总
- `DOCS/testplan/Smcsrind_test_plan.md` — Smcsrind Machine Mode 测试计划
- `DOCS/testplan/smstateen_test_plan.md` — Smstateen 独立测试计划
- `DOCS/testplan/Hypervisor_cross_test_plan.md` — Hypervisor 交叉测试计划（Group 8: Smstateen）
