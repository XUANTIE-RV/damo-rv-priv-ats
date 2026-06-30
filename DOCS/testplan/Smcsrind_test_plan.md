# Smcsrind 扩展 Machine Mode 测试计划

> 本文档描述 Smcsrind (Machine-level Indirect CSR Access) 扩展中 Machine Mode 相关功能的测试计划。Smcsrind 扩展提供了间接 CSR 访问机制，允许通过 select 值间接访问一组寄存器，而无需占用大量 CSR 地址空间。
>
> 生成时间：2026-06-25

---

## 范围

### 覆盖的功能

- **M-mode CSR 存在性**：`miselect`、`mireg`~`mireg6` 的存在性与可访问性
- **miselect 寄存器行为**：WARL 属性、最小位宽、MSB 语义
- **mireg\* 间接访问机制**：miselect 选择值决定访问哪个寄存器、合法/非法 select 值的行为
- **Smstateen 访问控制**：`mstateen0[60]` (CSRIND) 对 S-mode 访问 siselect/sireg\*/vsiselect/vsireg\* 的控制

### 不在本文档范围

- Supervisor-level CSR（siselect/sireg\*）的基本功能测试 — 由 `Sscsrind_test_plan.md` 覆盖
- Virtual Supervisor-level CSR（vsiselect/vsireg\*）的基本功能测试 — 由 `Sscsrind_test_plan.md` 覆盖
- Virtual-instruction 异常行为 — 由 `Sscsrind_test_plan.md` 覆盖
- H 扩展场景下 hstateen0[60] 对 VS/VU-mode 的控制 — 由 `Hypervisor_cross_test_plan.md` Group 8.4 (HCROSS-SSSTA-27~29) 覆盖
- 依赖 Smcsrind 的具体扩展（如 Smaia、Smcdeleg）的间接寄存器内容测试 — 由各自扩展的测试计划覆盖

---

## 规范依据

| 规范 ID | 来源 | 描述（英文） | 描述（中文） |
|---------|------|-------------|-------------|
| `norm:miph_csr_number` | `smcsrind.adoc` | The `mireg*` CSR numbers are not consecutive because miph is CSR number 0x354. | `mireg*` CSR 编号不连续，因为 miph 是 CSR 编号 0x354。 |
| `norm:miselect_op` | `smcsrind.adoc` | The value of `miselect` determines which register is accessed upon read or write of each of the machine indirect alias CSRs (`mireg*`). | `miselect` 的值决定在读取或写入每个机器间接别名 CSR（`mireg*`）时访问哪个寄存器。 |
| `norm:miselect_value_range` | `smcsrind.adoc` | `miselect` value ranges are allocated to dependent extensions, which specify the register state accessible via each `mireg_i` register, for each `miselect` value. | `miselect` 值范围分配给依赖扩展，这些扩展指定每个 `miselect` 值可通过每个 `mireg_i` 寄存器访问的寄存器状态。 |
| `norm:miselect_WARL` | `smcsrind.adoc` | `miselect` is a WARL register. | `miselect` 是 WARL 寄存器。 |
| `norm:miselect_min_sz` | `smcsrind.adoc` | The `miselect` register implements at least enough bits to support all implemented `miselect` values (corresponding to the implemented extensions that utilize `miselect`/`mireg*` to indirectly access register state). The `miselect` register may be read-only zero if there are no extensions implemented that utilize it. | `miselect` 寄存器至少实现足够的位以支持所有已实现的 `miselect` 值。如果没有利用它的扩展实现，则 `miselect` 寄存器可以为只读零。 |
| `norm:miselect_msb_op` | `smcsrind.adoc` | Values of `miselect` with the most-significant bit set (bit XLEN - 1 = 1) are designated only for custom use, presumably for accessing custom registers through the alias CSRs. Values of `miselect` with the most-significant bit clear are designated only for standard use and are reserved until allocated to a standard architecture extension. If XLEN is changed, the most-significant bit of `miselect` moves to the new position, retaining its value from before. | MSB=1 的 `miselect` 值仅用于自定义用途。MSB=0 的值仅用于标准用途并保留。XLEN 变化时 MSB 移动到新位置并保留值。 |
| `norm:mireg_access_on_legal_miselect` | `smcsrind.adoc` | Attempts to access `mireg*` while `miselect` holds a number in an allocated and implemented range results in a specific behavior that, for each combination of `miselect` and `mireg_i`, is defined by the extension to which the `miselect` value is allocated. | 当 `miselect` 持有已分配和实现范围内的值时，访问 `mireg*` 的行为由分配该 `miselect` 值的扩展定义。 |
| `norm:mireg_access_behaviour` | `smcsrind.adoc` | Ordinarily, each `mireg*_i_* will access register state, access read-only 0 state, or raise an illegal-instruction exception. | 通常，每个 `mireg_i` 将访问寄存器状态、访问只读 0 状态或触发 illegal-instruction 异常。 |
| `norm:csr_access` | `smcsrind.adoc` | A CSR accessible via one method may or may not be accessible via the other method. | 通过一种方法可访问的 CSR 可能通过另一种方法可访问，也可能不可访问。 |
| `norm:select_value_separate_address_space` | `smcsrind.adoc` | Select values are a separate address space from CSR numbers, and from tselect values in the Sdtrig extension. | 选择值是与 CSR 编号以及 Sdtrig 扩展中的 tselect 值不同的地址空间。 |
| `norm:select_value_unrelated` | `smcsrind.adoc` | If a CSR is both directly and indirectly accessible, the CSR's select value is unrelated to its CSR number. | 如果 CSR 既可直接访问又可间接访问，则 CSR 的选择值与其 CSR 编号无关。 |
| `norm:csrs_alias` | `smcsrind.adoc` | Machine-level and Supervisor-level CSRs with the same select value may be defined by an extension as partial or full aliases with respect to each other. | 具有相同选择值的机器级和监管级 CSR 可由扩展定义为彼此的部分或完全别名。 |
| `norm:sscsrind_csrs_access_control` | `smcsrind.adoc` | If extension Smstateen is implemented together with Smcsrind, bit 60 of state-enable register `mstateen0` controls access to `siselect`, `sireg*`, `vsiselect`, and `vsireg*`. When `mstateen0`[60]=0, an attempt to access one of these CSRs from a privilege mode less privileged than M-mode results in an illegal-instruction exception. | 如果 Smstateen 与 Smcsrind 一起实现，`mstateen0[60]` 控制对 `siselect`/`sireg*`/`vsiselect`/`vsireg*` 的访问。`mstateen0[60]=0` 时，从低于 M-mode 的特权级访问这些 CSR 触发 illegal-instruction 异常。 |

---

## M-mode CSR 表

| 编号 | 特权级 | 宽度 | 名称 | 描述 |
|------|--------|------|------|------|
| 0x350 | MRW | XLEN | `miselect` | Machine indirect register select |
| 0x351 | MRW | XLEN | `mireg` | Machine indirect register alias |
| 0x352 | MRW | XLEN | `mireg2` | Machine indirect register alias 2 |
| 0x353 | MRW | XLEN | `mireg3` | Machine indirect register alias 3 |
| 0x355 | MRW | XLEN | `mireg4` | Machine indirect register alias 4 |
| 0x356 | MRW | XLEN | `mireg5` | Machine indirect register alias 5 |
| 0x357 | MRW | XLEN | `mireg6` | Machine indirect register alias 6 |

> [!NOTE]
> CSR 编号 0x354 被 `miph` 占用，因此 `mireg*` 编号不连续。

---

## Group 1. M-mode CSR 存在性与可访问性

**规范依据**：
- `norm:miph_csr_number`：`mireg*` CSR 编号不连续，0x354 为 miph

**测试职责**：验证 miselect 和 mireg~mireg6 共 7 个 M-mode CSR 的存在性与基本可读写性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MCSRIND-EXIST-01 | miselect 可读写 | M-mode 写 miselect 一个值后读回验证 | 无异常，读回值与写入值一致（受 WARL 约束） |
| MCSRIND-EXIST-02 | mireg 可读写 | M-mode 设置一个合法的 miselect 值后，写 mireg 并读回 | 无异常，读回值一致 |
| MCSRIND-EXIST-03 | mireg2 可读写 | M-mode 设置一个合法的 miselect 值后，写 mireg2 并读回 | 无异常，读回值一致 |
| MCSRIND-EXIST-04 | mireg3 可读写 | M-mode 设置一个合法的 miselect 值后，写 mireg3 并读回 | 无异常，读回值一致 |
| MCSRIND-EXIST-05 | mireg4 可读写 | M-mode 设置一个合法的 miselect 值后，写 mireg4 并读回 | 无异常，读回值一致 |
| MCSRIND-EXIST-06 | mireg5 可读写 | M-mode 设置一个合法的 miselect 值后，写 mireg5 并读回 | 无异常，读回值一致 |
| MCSRIND-EXIST-07 | mireg6 可读写 | M-mode 设置一个合法的 miselect 值后，写 mireg6 并读回 | 无异常，读回值一致 |

> [!NOTE]
> - mireg~mireg6 的读写行为取决于当前 miselect 的值。若 miselect 指向一个未实现的范围，访问 mireg* 可能触发 illegal-instruction（行为 UNSPECIFIED）。因此测试时 miselect 应设为一个已实现扩展定义的有效 select 值。
> - 若平台上没有实现任何利用 miselect/mireg* 的扩展（如 Smaia），miselect 可能为只读零，此时所有 mireg* 的访问行为 UNSPECIFIED。测试应首先检测 miselect 是否可写。
> - M-mode CSR 的可访问性不受 mstateen 控制（state-enable CSR 仅影响低于 M-mode 的特权级）。

---

## Group 2. miselect 寄存器行为

**规范依据**：
- `norm:miselect_WARL`：miselect 是 WARL 寄存器
- `norm:miselect_min_sz`：至少实现足够的位以支持所有已实现的 miselect 值
- `norm:miselect_msb_op`：MSB=1 为自定义用途，MSB=0 为标准保留用途
- `norm:miselect_value_range`：miselect 值范围分配给依赖扩展

**测试职责**：验证 miselect 寄存器的 WARL 属性、位宽要求、MSB 语义。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MCSRIND-MSEL-01 | miselect WARL 行为 | M-mode 向 miselect 写入全 1（0xFFFFFFFFFFFFFFFF），读回验证 | 读回值为合法值（WARL：写入任意值，读回合法值），不应触发异常 |
| MCSRIND-MSEL-02 | miselect WARL 写入零 | M-mode 向 miselect 写入 0，读回验证 | 读回值为 0（0 始终是合法的保留值） |
| MCSRIND-MSEL-03 | miselect 最小位宽验证 | 若存在已实现的 miselect 值（如 Smaia 定义的范围），写入该值并读回 | 读回值与写入值一致 |
| MCSRIND-MSEL-04 | miselect MSB=1 自定义区域 | 写入 MSB=1 的值（如 0x8000000000000000），读回验证 | 读回值为合法值（可能保留 MSB=1，也可能被 WARL 映射为其他值） |
| MCSRIND-MSEL-05 | miselect MSB=0 标准区域 | 写入 MSB=0 的值（如 0x0000000000000001），读回验证 | 读回值为合法值 |
| MCSRIND-MSEL-06 | miselect 无扩展时可读零 | 在没有实现任何利用 miselect 的扩展的平台上，读取 miselect | miselect 可能为只读零（SPEC 允许此情况） |
| MCSRIND-MSEL-07 | miselect 连续写入不同值 | 连续写入 0、最大值、0x100、0x200 等值，每次写入后读回 | 每次读回均为合法值，WARL 行为一致 |

> [!NOTE]
> - WARL (Write Any values, Reads Legal Values) 意味着写入任意值不会触发异常，但读回值可能被映射为某个合法值。测试验证的是"不异常"和"读回合法"两个性质。
> - MCSRIND-MSEL-03 依赖平台上已实现的扩展。例如 Smaia 扩展定义了 miselect 值范围 0x30~0x3F（用于 miselect/mireg* 间接访问 mireg 等）。若无此类扩展，此用例应 TEST_SKIP。
> - MCSRIND-MSEL-06 与 MSCRIND-MSEL-03 互补：若没有依赖扩展，miselect 可能完全只读零。
> - MSB 的位位置取决于 XLEN：RV64 时为 bit 63，RV32 时为 bit 31。

---

## Group 3. mireg\* 间接访问机制

**规范依据**：
- `norm:miselect_op`：miselect 值决定 mireg* 访问哪个寄存器
- `norm:mireg_access_on_legal_miselect`：合法 miselect 值下 mireg* 行为由依赖扩展定义
- `norm:mireg_access_behaviour`：每个 mireg_i 访问寄存器状态、只读 0 或触发 illegal-instruction
- `norm:select_value_separate_address_space`：select 值是与 CSR 编号不同的地址空间
- `norm:select_value_unrelated`：select 值与 CSR 编号无关

**测试职责**：验证 miselect 选择值对 mireg* 间接访问的控制行为，包括合法/非法 select 值的访问结果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MCSRIND-MIREG-01 | miselect 决定 mireg 访问目标 | 设置 miselect 为两个不同的合法值，分别读 mireg | 不同 miselect 值时 mireg 返回不同结果（对应不同的间接寄存器） |
| MCSRIND-MIREG-02 | 未实现 miselect 值时访问 mireg | 设置 miselect 为一个未实现的值（保留范围内的值），尝试读 mireg | 行为 UNSPECIFIED：预期触发 illegal-instruction 异常，或返回只读零（取决于实现） |
| MCSRIND-MIREG-03 | 未实现 miselect 值时访问 mireg2~mireg6 | 设置 miselect 为一个未实现的值，逐一尝试读 mireg2~mireg6 | 行为 UNSPECIFIED：与 MCSRIND-MIREG-02 类似 |
| MCSRIND-MIREG-04 | 合法 miselect 下 mireg 写入并读回 | 设置 miselect 为一个已实现扩展定义的可写范围，写 mireg 后读回 | 读回值与写入值一致（若该 mireg 为可写寄存器状态） |
| MCSRIND-MIREG-05 | 合法 miselect 下 mireg 只读 0 验证 | 设置 miselect 为一个已实现扩展定义的只读 0 范围，写 mireg 后读回 | 读回值为 0（该 mireg 访问只读 0 状态） |
| MCSRIND-MIREG-06 | select 值与 CSR 编号独立性 | 验证 miselect 的选择值空间与 CSR 编号空间是独立的 | 通过 miselect 值 N 访问的寄存器不一定是 CSR 编号为 N 的寄存器（地址空间独立） |
| MCSRIND-MIREG-07 | 同一 miselect 下多个 mireg_i 独立性 | 设置一个合法 miselect，分别读写 mireg、mireg2、mireg3 | 各 mireg_i 访问不同的寄存器状态（互不影响） |

> [!NOTE]
> - MCSRIND-MIREG-02~03 验证 UNSPECIFIED 行为。SPEC 明确指出：从 M-mode 访问 mireg* 而 miselect 持有未实现值时，行为是 UNSPECIFIED。SPEC 预期实现通常触发 illegal-instruction 异常，但不强制要求。测试应记录实际行为并与 SPEC 预期比对。
> - MCSRIND-MIREG-01 和 MCSRIND-MIREG-04 依赖已实现的依赖扩展。例如 Smaia 定义了 miselect=0x30 时 mireg 对应 mireg CSR。若平台上没有此类扩展，应 TEST_SKIP。
> - MCSRIND-MIREG-06 是概念验证：间接访问机制使用的 select 值是一个独立的地址空间，与 CSR 编号无关。这可以通过尝试 miselect=0x350（等于 miselect 自身的 CSR 编号）来验证——它不应访问 miselect 本身。
> - MCSRIND-MIREG-07 验证多个 mireg_i 的独立性：对于同一个 miselect 值，mireg、mireg2、mireg3 等分别访问不同的寄存器。

---

## Group 4. Smstateen × Smcsrind 访问控制

**规范依据**：
- `norm:sscsrind_csrs_access_control`：`mstateen0[60]` 控制 S-mode 及以下对 siselect/sireg\*/vsiselect/vsireg\* 的访问

**测试职责**：验证 `mstateen0[60]` (CSRIND bit) 对 S-mode 访问 supervisor-level 间接 CSR 的控制行为。注意：M-mode 自身对 miselect/mireg* 的访问**不受** mstateen0 控制。

> **前提配置**：需要 Smstateen 扩展与 Smcsrind 同时实现。
>
> **注意**：本组仅覆盖非 H 扩展场景（siselect/sireg*）。H 扩展场景下 `mstateen0[60]` 对 vsiselect/vsireg* 的控制已提取到 `Hypervisor_cross_test_plan.md` Group 10（HCROSS-SMCSRIND-01~08）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MCSRIND-STA-01 | mstateen0[60]=0 阻止 S-mode 读 siselect | mstateen0[60]=0，S-mode 读 siselect (0x150) | 触发 illegal-instruction 异常 (cause=2) |
| MCSRIND-STA-02 | mstateen0[60]=0 阻止 S-mode 写 siselect | mstateen0[60]=0，S-mode 写 siselect | 触发 illegal-instruction 异常 (cause=2) |
| MCSRIND-STA-03 | mstateen0[60]=0 阻止 S-mode 读 sireg | mstateen0[60]=0，S-mode 读 sireg (0x151) | 触发 illegal-instruction 异常 (cause=2) |
| MCSRIND-STA-04 | mstateen0[60]=0 阻止 S-mode 读 sireg2~sireg6 | mstateen0[60]=0，S-mode 逐一读 sireg2~sireg6 | 每个都触发 illegal-instruction 异常 (cause=2) |
| MCSRIND-STA-05 | mstateen0[60]=1 允许 S-mode 访问 siselect | mstateen0[60]=1，S-mode 读写 siselect | 访问正常，无异常 |
| MCSRIND-STA-06 | mstateen0[60]=1 允许 S-mode 访问 sireg* | mstateen0[60]=1，S-mode 读写 sireg~sireg6 | 访问正常，无异常（具体 mireg_i 行为取决于 siselect 值） |
| MCSRIND-STA-07 | mstateen0[60]=0 不影响 M-mode 访问 miselect | mstateen0[60]=0，M-mode 读写 miselect | 访问正常，无异常（state-enable 不控制 M-mode） |
| MCSRIND-STA-08 | mstateen0[60]=0 不影响 M-mode 访问 mireg* | mstateen0[60]=0，M-mode 读写 mireg~mireg6 | 访问正常，无异常 |
| MCSRIND-STA-09 | mstateen0[60]=0 阻止 S-mode 写 sireg | mstateen0[60]=0，S-mode 写 sireg | 触发 illegal-instruction 异常 (cause=2) |
| MCSRIND-STA-10 | mstateen0 未实现时行为 | 若 Smstateen 扩展未实现，S-mode 访问 siselect/sireg* | 访问正常（无 state-enable 控制机制） |

> [!NOTE]
> - 本组测试验证 Smstateen 对 Smcsrind/Sscsrind 的访问控制。mstateen0[60] (CSRIND) 为 0 时，**所有低于 M-mode 的特权级**访问 siselect/sireg\*/vsiselect/vsireg\* 都触发 illegal-instruction 异常。
> - MCSRIND-STA-01~04 与 `smstateen_test_plan.md` 中的 MSTA-CSRIND-01~04 覆盖相同规范点但角度不同：MSTA-CSRIND 系列从 Smstateen 扩展的角度验证 bit 60 的控制行为，本组从 Smcsrind 扩展的角度验证 CSR 的受控行为。实现时应避免重复，可标记为交叉引用。
> - MCSRIND-STA-07~08 验证 state-enable CSR **不影响** M-mode 自身的访问。这是 SPEC 明确指出的：state-enable CSR 仅影响低于 M-mode 的特权级。
> - MCSRIND-STA-10 验证 Smstateen 未实现时的行为：没有 Smstateen 扩展，mstateen0 CSR 不存在，因此不存在访问控制。
> - **H 扩展场景**：`mstateen0[60]=0` 还会阻止 S-mode 访问 vsiselect/vsireg*。此场景需要 H 扩展，已从本组提取到 `Hypervisor_cross_test_plan.md` Group 10（HCROSS-SMCSRIND-01~08）。

### 代码示例

```c
/* MCSRIND-MSEL-01: miselect WARL behavior */
TEST_REGISTER(test_mcsrind_msel_01);
bool test_mcsrind_msel_01(void) {
    TEST_BEGIN("MCSRIND-MSEL-01: miselect WARL behavior");

    /* Write all-ones to miselect */
    uintptr_t all_ones = (uintptr_t)-1;
    CSRW(0x350, all_ones);  /* miselect */
    uintptr_t readback = CSRR(0x350);

    /* WARL: writing any value should not cause an exception.
     * Readback value is implementation-defined legal value. */
    TEST_ASSERT("miselect WARL: no exception on all-ones write",
                1);  /* If we got here, no exception occurred */

    /* Write zero */
    CSRW(0x350, 0);
    readback = CSRR(0x350);
    TEST_ASSERT_EQ("miselect WARL: zero reads back as zero",
                   readback, 0);

    TEST_END();
}

/* MCSRIND-STA-01: mstateen0[60]=0 blocks S-mode siselect read */
TEST_REGISTER(test_mcsrind_sta_01);
bool test_mcsrind_sta_01(void) {
    TEST_BEGIN("MCSRIND-STA-01: mstateen0[60]=0 blocks S-mode siselect");

    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    /* Save original mstateen0 */
    uintptr_t orig = CSRR(0x30C);  /* mstateen0 */

    /* Clear CSRIND bit (bit 60) */
    uintptr_t val = orig & ~(1ULL << 60);
    CSRW(0x30C, val);

    /* Switch to S-mode and try to read siselect (0x150) */
    goto_priv(PRIV_S);
    PRIV_DO(CSRR(0x150));  /* siselect */
    goto_priv(PRIV_M);
    CHECK_TRAP("illegal-instruction on S-mode siselect read",
               CAUSE_ILLEGAL_INST);

    /* Restore mstateen0 */
    CSRW(0x30C, orig);
    TEST_END();
}

/* MCSRIND-STA-07: mstateen0[60]=0 does NOT affect M-mode miselect */
TEST_REGISTER(test_mcsrind_sta_07);
bool test_mcsrind_sta_07(void) {
    TEST_BEGIN("MCSRIND-STA-07: mstateen0[60]=0 does not block M-mode miselect");

    if (!platform_has_smstateen()) {
        TEST_SKIP("Smstateen not implemented");
    }

    /* Save original mstateen0 */
    uintptr_t orig = CSRR(0x30C);  /* mstateen0 */

    /* Clear CSRIND bit (bit 60) */
    uintptr_t val = orig & ~(1ULL << 60);
    CSRW(0x30C, val);

    /* M-mode access to miselect should still work */
    trap_expect_begin();
    CSRW(0x350, 0x42);  /* miselect = 0x42 */
    uintptr_t readback = CSRR(0x350);
    bool trapped = trap_was_triggered();
    trap_expect_end();

    TEST_ASSERT("M-mode miselect access: no exception when CSRIND=0",
                !trapped);

    /* Restore mstateen0 */
    CSRW(0x30C, orig);
    TEST_END();
}

/* NOTE: H-extension scenarios (vsiselect/vsireg*) have been extracted to
 * Hypervisor_cross_test_plan.md Group 10 (HCROSS-SMCSRIND-01~08).
 * Those tests require H extension and verify mstateen0[60] control
 * over S-mode (HS-mode) access to vsiselect/vsireg*. */
```

---

## 测试优先级

| 优先级 | 测试组 | 覆盖的测试 ID | 理由 |
|--------|--------|--------------|------|
| P0（必须） | Group 1 (存在性) | MCSRIND-EXIST-01~07 | CSR 存在性是所有后续测试的基础 |
| P0（必须） | Group 2 (miselect 行为) | MCSRIND-MSEL-01~07 | WARL 和 MSB 语义是间接访问机制的核心 |
| P1（重要） | Group 3 (mireg 间接访问) | MCSRIND-MIREG-01~07 | 间接访问的正确性依赖于依赖扩展的实现 |
| P1（重要） | Group 4 (stateen 控制) | MCSRIND-STA-01~10 | 访问控制是安全隔离的关键保证 |

---

## 测试实现说明

### 文件组织

```
damo-priv-test/
├── Smcsrind/
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   └── tests/
│       ├── test_mcsrind_exist.c       # Group 1: CSR 存在性
│       ├── test_mcsrind_miselect.c    # Group 2: miselect 行为
│       ├── test_mcsrind_mireg.c       # Group 3: mireg 间接访问
│       └── test_mcsrind_stateen.c     # Group 4: Smstateen 控制
```

### 运行时检测

```c
static bool platform_has_smcsrind(void) {
    /* Check if miselect CSR is accessible.
     * If Smcsrind is not implemented, accessing miselect
     * triggers illegal-instruction. */
    trap_expect_begin();
    CSRR(0x350);  /* miselect */
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}

static bool platform_has_smstateen(void) {
    /* Check if mstateen0 CSR is accessible */
    trap_expect_begin();
    CSRR(0x30C);  /* mstateen0 */
    bool trapped = trap_was_triggered();
    trap_expect_end();
    return !trapped;
}
```

### 关键注意事项

1. **CSR 地址**：miselect=0x350, mireg=0x351, mireg2=0x352, mireg3=0x353, mireg4=0x355, mireg5=0x356, mireg6=0x357。注意 0x354 为 miph，不是 mireg4。

2. **WARL 语义**：miselect 是 WARL 寄存器，写入任意值不应触发异常。测试应验证"不异常"性质，同时验证读回值为合法值。

3. **UNSPECIFIED 行为**：SPEC 明确说明从 M-mode 访问 mireg* 而 miselect 持有未实现值时，行为是 UNSPECIFIED。虽然 SPEC 预期实现通常触发 illegal-instruction，但测试不应将其作为 PASS/FAIL 的硬性标准，而应记录行为并与 SPEC 比对。

4. **依赖扩展**：mireg* 的实际内容（寄存器状态 vs 只读 0 vs illegal-instruction）取决于 miselect 指向的依赖扩展。测试 mireg* 功能时，需要先确定平台上实现了哪些利用 miselect/mireg* 的扩展（如 Smaia）。

5. **框架修改**：需在 `common/encoding.h` 中添加 CSR 地址定义：
   ```c
   #define CSR_MISELECT   0x350
   #define CSR_MIREG      0x351
   #define CSR_MIREG2     0x352
   #define CSR_MIREG3     0x353
   #define CSR_MIREG4     0x355
   #define CSR_MIREG5     0x356
   #define CSR_MIREG6     0x357
   ```

---

## 测试判定标准

| 判定 | 条件 |
|------|------|
| PASS | 所有测试断言通过 |
| FAIL | 任何测试断言失败，且与 SPEC 规范一致（即实现不符合 SPEC） |
| SKIP | 所需扩展未实现（如 Smstateen 未实现时 Group 4 SKIP） |
| UNSPECIFIED | SPEC 明确标注行为未指定的情况（如未实现 miselect 值的 mireg* 访问），记录实际行为 |

---

## 覆盖率矩阵

| 规范 ID | 覆盖的测试 ID |
|---------|--------------|
| `norm:miph_csr_number` | MCSRIND-EXIST-01~07 |
| `norm:miselect_WARL` | MCSRIND-MSEL-01, MCSRIND-MSEL-02, MCSRIND-MSEL-07 |
| `norm:miselect_min_sz` | MCSRIND-MSEL-03, MCSRIND-MSEL-06 |
| `norm:miselect_msb_op` | MCSRIND-MSEL-04, MCSRIND-MSEL-05 |
| `norm:miselect_value_range` | MCSRIND-MSEL-03 |
| `norm:miselect_op` | MCSRIND-MIREG-01 |
| `norm:mireg_access_on_legal_miselect` | MCSRIND-MIREG-04, MCSRIND-MIREG-05 |
| `norm:mireg_access_behaviour` | MCSRIND-MIREG-02, MCSRIND-MIREG-03, MCSRIND-MIREG-05, MCSRIND-MIREG-07 |
| `norm:select_value_separate_address_space` | MCSRIND-MIREG-06 |
| `norm:select_value_unrelated` | MCSRIND-MIREG-06 |
| `norm:csrs_alias` | — （由依赖扩展测试覆盖） |
| `norm:sscsrind_csrs_access_control` | MCSRIND-STA-01~10 |

---

## 参考

- `SPEC/smcsrind.adoc` — Smcsrind/Sscsrind Extension for Indirect CSR Access
- `SPEC/smstateen.adoc` — Smstateen Extension (State Enable)
- `NORM/sm_norm.md` — 规范点汇总
- `DOCS/testplan/smstateen_test_plan.md` — Smstateen 独立测试计划
- `DOCS/testplan/Hypervisor_cross_test_plan.md` — Hypervisor 交叉测试计划（Group 8: Smstateen）
- `DOCS/testplan/Sscsrind_test_plan.md` — Sscsrind Supervisor Mode 测试计划
