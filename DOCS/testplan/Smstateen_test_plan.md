# Smstateen 扩展测试方案

## 1. 概述

### 1.1 规范来源

- **规范文件**: `SPEC/smstateen.adoc`
- **扩展名称**: Smstateen (Machine-level State Enable Extension)
- **关联扩展**: Ssstateen (Supervisor-level subset)

### 1.2 被测特性

Smstateen 扩展提供了一组 CSR，用于控制低特权级对各种扩展状态的访问，防止隐蔽通道（covert channel）。该扩展包含完整的 mstateen\*、sstateen\*、hstateen\* CSR 及其功能。

### 1.3 测试范围

本测试方案覆盖 Smstateen 扩展中 M-mode 相关功能：

| 测试层级 | 涉及 CSR | 测试组 |
|---------|---------|--------|
| M-mode | mstateen0-3, mstateen0h-3h (RV32) | Group 1-7 |

### 1.4 前提条件

| 项目 | 要求 |
|------|------|
| 架构 | RV64 (RV32 部分测试可选) |
| 特权级支持 | M-mode, S-mode |
| 可选依赖 | H 扩展 (Group 12-16)、Ssaia、Sscsrind、Zcmt、Sdtrig、Ssqosid |

---

## 2. 规范要求分析

### 2.1 Normative Rules 清单

| 编号 | 规范引用 | 简述 |
|------|---------|------|
| NR-01 | `norm:mstateen_rv64_csrs` | RV64 添加 mstateen0-3 四个 64 位 CSR |
| NR-02 | `norm:stateen_rv32_upper_bits_csrs` | RV32 上半部分 CSR 地址 |
| NR-03 | `norm:stateen_op` | stateen 控制所有低特权级访问，不控制本级 |
| NR-04 | `norm:stateen_illegal_state_access` | 被阻止的访问引发 illegal-instruction 或 virtual-instruction 异常 |
| NR-05 | `norm:stateen_implicit_state_update` | 隐式更新状态的指令是否触发异常需明确规定 |
| NR-06 | `norm:mstateen_bit_allocation` | mstateen 上 32 位从 bit 63 向低位分配 |
| NR-07 | `norm:mstateen_bit_encroachment` | mstateen 高位分配可侵入低 32 位 |
| NR-08 | `norm:stateen_warl_access` | 每个标准定义位都是 WARL |
| NR-09 | `norm:stateen_unimplemented_state_roz` | 未实现状态的控制位为只读零 |
| NR-10 | `norm:stateen_reserved_roz` | 保留位为只读零 |
| NR-11 | `norm:mstateen_lower_priv_roz` | mstateen 中为零的位在 hstateen/sstateen 中也为只读零 |
| NR-12 | `norm:mstateen_zero_initialization` | 复位时所有可写 mstateen 位初始化为零 |
| NR-13 | `norm:hstateen_sstateen_zero_initialization` | M-mode 软件修改后负责初始化 hstateen/sstateen 为零 |
| NR-14 | `norm:mstateen_bit_63_op` | mstateen 的 bit 63 控制对应 sstateen 和 hstateen 的访问 |
| NR-15 | `norm:mstateen_bit_63_roz` | mstateen bit 63 仅在无 H 扩展且 sstateen 全只读零时可为 RO0 |
| NR-16 | `norm:stateen0_c_op` | C 位控制自定义状态访问 |
| NR-17 | `norm:stateen0_fcsr_op` | FCSR 位控制 Zfinx 场景下的 fcsr 访问 |
| NR-18 | `norm:mstateen0_fcsr_roz` | misa.F=1 时 FCSR 位为只读零 |
| NR-19 | `norm:stateen0_fcsr0_misa_f0_illegal_fpu_instr` | stateen 实现且 misa.F=0 时 FCSR=0 则所有浮点指令非法 |
| NR-20 | `norm:stateen0_jvt_op` | JVT 位控制 Zcmt 的 jvt CSR |
| NR-21 | `norm:mstateen0_se0_op` | mstateen0.SE0 控制 hstateen0/sstateen0 的访问 |
| NR-22 | `norm:mstateen0_envcfg_op` | mstateen0.ENVCFG 控制 henvcfg/senvcfg 访问 |
| NR-23 | `norm:mstateen0_csrind_op` | mstateen0.CSRIND 控制 siselect/sireg/vsiselect/vsireg |
| NR-24 | `norm:mstateen0_imsic_op` | mstateen0.IMSIC 控制 IMSIC 状态及 stopei/vstopei |
| NR-25 | `norm:mstateen0_aia_op` | mstateen0.AIA 控制 Ssaia 剩余状态 |
| NR-26 | `norm:mstateen0_context_op` | mstateen0.CONTEXT 控制 scontext/hcontext |
| NR-27 | `norm:mstateen0_p1p13_op` | mstateen0.P1P13 控制 hedelegh |
| NR-28 | `norm:mstateen0_srmcfg_op` | mstateen0.SRMCFG 控制 srmcfg |

---

## 3. M-mode 测试组

### Group 1：mstateen CSR 存在性与可访问性

**规范依据**：
- `norm:mstateen_rv64_csrs`：RV64 添加 mstateen0、mstateen1、mstateen2、mstateen3 四个 64 位 CSR
- `norm:stateen_rv32_upper_bits_csrs`：RV32 额外提供 mstateen0h-3h

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-EXIST-01 | mstateen0 可读写 | 在 M-mode 读写 mstateen0 | 读写不触发异常 | `norm:mstateen_rv64_csrs` |
| MSTA-EXIST-02 | mstateen1 可读写 | 在 M-mode 读写 mstateen1 | 读写不触发异常 | `norm:mstateen_rv64_csrs` |
| MSTA-EXIST-03 | mstateen2 可读写 | 在 M-mode 读写 mstateen2 | 读写不触发异常 | `norm:mstateen_rv64_csrs` |
| MSTA-EXIST-04 | mstateen3 可读写 | 在 M-mode 读写 mstateen3 | 读写不触发异常 | `norm:mstateen_rv64_csrs` |
| MSTA-EXIST-05 | mstateen0h 可读写 (RV32) | 在 RV32 M-mode 读写 mstateen0h | 读写不触发异常 | `norm:stateen_rv32_upper_bits_csrs` |

---

### Group 2：mstateen 复位初始化行为

**规范依据**：
- `norm:mstateen_zero_initialization`：复位时所有可写 mstateen 位初始化为零
- `norm:hstateen_sstateen_zero_initialization`：M-mode 软件修改 mstateen 后，需负责将 hstateen/sstateen 初始化为零

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-INIT-01 | mstateen0 复位值为零 | 复位后立即读取 mstateen0，检查所有可写位 | 所有可写位为 0 | `norm:mstateen_zero_initialization` |
| MSTA-INIT-02 | mstateen1 复位值为零 | 复位后立即读取 mstateen1 | 所有可写位为 0 | `norm:mstateen_zero_initialization` |
| MSTA-INIT-03 | mstateen2 复位值为零 | 复位后立即读取 mstateen2 | 所有可写位为 0 | `norm:mstateen_zero_initialization` |
| MSTA-INIT-04 | mstateen3 复位值为零 | 复位后立即读取 mstateen3 | 所有可写位为 0 | `norm:mstateen_zero_initialization` |
| MSTA-INIT-05 | 修改 mstateen 后初始化 hstateen **[已迁移至 Hypervisor_cross_test_plan.md]** | M-mode 设置 mstateen0 某些位为 1 后，写零到 hstateen0 | hstateen0 读回为零 | `norm:hstateen_sstateen_zero_initialization` |
| MSTA-INIT-06 | 修改 mstateen 后初始化 sstateen | M-mode 设置 mstateen0 某些位为 1 后，写零到 sstateen0 | sstateen0 读回为零 | `norm:hstateen_sstateen_zero_initialization` |

**测试步骤示例 (MSTA-INIT-01)**：
1. 在测试最早期（复位后第一次运行），读取 mstateen0
2. 检查所有可写位是否为零

**预期结果**：
- mstateen0 读回值为 0（所有可写位均为 0）

**断言**：
```
TEST_ASSERT_EQ("mstateen0 reset value should be 0", read_csr(mstateen0), 0)
```

---

### Group 3：WARL 与只读零行为

**规范依据**：
- `norm:stateen_warl_access`：每个标准定义位都是 WARL，可能为只读零或只读一
- `norm:stateen_unimplemented_state_roz`：控制未实现状态的位为只读零
- `norm:stateen_reserved_roz`：保留位为只读零

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-WARL-01 | mstateen0 保留位只读零 | 向 mstateen0 保留位 (WPRI 域) 写 1 后读回 | 保留位读回为 0 | `norm:stateen_reserved_roz` |
| MSTA-WARL-02 | mstateen0 未实现扩展位只读零 | 对于未实现的扩展 (如 Zcmt 未实现时 JVT 位)，尝试写 1 | 该位读回为 0 | `norm:stateen_unimplemented_state_roz` |
| MSTA-WARL-03 | mstateen0 WARL 写入合法值 | 向 mstateen0 写入合法值后读回 | 读回值与写入值一致（限可写位） | `norm:stateen_warl_access` |
| MSTA-WARL-04 | mstateen1 保留位只读零 | 向 mstateen1 全部写 1 后读回 | 所有保留位读回为 0 | `norm:stateen_reserved_roz` |
| MSTA-WARL-05 | mstateen2 保留位只读零 | 向 mstateen2 全部写 1 后读回 | 所有保留位读回为 0 | `norm:stateen_reserved_roz` |
| MSTA-WARL-06 | mstateen3 保留位只读零 | 向 mstateen3 全部写 1 后读回 | 所有保留位读回为 0 | `norm:stateen_reserved_roz` |

**测试步骤示例 (MSTA-WARL-01)**：
1. 在 M-mode 下保存 mstateen0 原始值
2. 向 mstateen0 写入 0xFFFFFFFF_FFFFFFFF（全 1）
3. 读回 mstateen0
4. 检查 WPRI 域（bits [53:3]）是否均为 0
5. 恢复原始值

**预期结果**：
- WPRI 域读回值全部为 0，仅已定义的功能位可能保持写入值

**断言**：
```
uint64_t val = read_csr(mstateen0);
uint64_t wpri_mask = 0x003FFFFFFFFFFFF8UL;  /* bits [53:3] */
TEST_ASSERT_EQ("mstateen0 WPRI bits should be 0", val & wpri_mask, 0)
```

---

### Group 4：mstateen 对低特权级的只读零传播

**规范依据**：
- `norm:mstateen_lower_priv_roz`：mstateen 中为零的位在 hstateen 和 sstateen 中表现为只读零

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-PROP-01 | mstateen0 零位传播到 sstateen0 | 将 mstateen0 某功能位设为 0，在 S-mode 尝试写该位到 sstateen0 | sstateen0 对应位读回为 0 | `norm:mstateen_lower_priv_roz` |
| MSTA-PROP-02 | mstateen0 零位传播到 hstateen0 **[已迁移至 Hypervisor_cross_test_plan.md]** | 将 mstateen0 某功能位设为 0，在 HS-mode 尝试写该位到 hstateen0 | hstateen0 对应位读回为 0 | `norm:mstateen_lower_priv_roz` |
| MSTA-PROP-03 | mstateen0 一位解除传播 | 先将 mstateen0 某功能位设为 0 验证传播，再设为 1 | sstateen0/hstateen0 对应位变为可写 | `norm:mstateen_lower_priv_roz` |
| MSTA-PROP-04 | mstateen1-3 零位传播 | 对 mstateen1-3 重复上述传播验证 | 对应 sstateen/hstateen 位为只读零 | `norm:mstateen_lower_priv_roz` |

**测试步骤示例 (MSTA-PROP-01)**：
1. 在 M-mode 下将 mstateen0 的 C 位 (bit 0) 设为 0
2. 确保 mstateen0.SE0 (bit 63) 为 1 以允许 S-mode 访问 sstateen0
3. 切换到 S-mode
4. 尝试向 sstateen0 的 C 位写 1
5. 读回 sstateen0
6. 返回 M-mode 检查结果

**预期结果**：
- sstateen0 的 C 位读回为 0（被 mstateen0 传播为只读零）

**断言**：
```
write_csr(sstateen0, STATEEN0_C_BIT);
uint32_t val = read_csr(sstateen0);
TEST_ASSERT_EQ("sstateen0.C should be RO0 when mstateen0.C=0", val & STATEEN0_C_BIT, 0)
```

---

### Group 5：mstateen bit 63 控制行为

**规范依据**：
- `norm:mstateen_bit_63_op`：每个 mstateen 的 bit 63 控制对应 sstateen 和 hstateen 的访问
- `norm:mstateen_bit_63_roz`：仅在无 H 扩展且对应 sstateen 全只读零时，mstateen bit 63 可为 RO0
- `norm:hstateen_bit_63_writable`：hstateen bit 63 始终可写

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-B63-01 | mstateen0.SE0=0 阻止 S-mode 访问 sstateen0 | 设置 mstateen0 bit 63 为 0，在 S-mode 访问 sstateen0 | 触发 illegal-instruction 异常 | `norm:mstateen_bit_63_op` |
| MSTA-B63-02 | mstateen0.SE0=1 允许 S-mode 访问 sstateen0 | 设置 mstateen0 bit 63 为 1，在 S-mode 访问 sstateen0 | 访问正常，无异常 | `norm:mstateen_bit_63_op` |
| MSTA-B63-03 | mstateen0.SE0=0 阻止 HS-mode 访问 hstateen0 **[已迁移至 Hypervisor_cross_test_plan.md]** | 设置 mstateen0 bit 63 为 0，在 HS-mode 访问 hstateen0 | 触发 illegal-instruction 异常 | `norm:mstateen_bit_63_op` |
| MSTA-B63-04 | mstateen1 bit 63 控制 sstateen1 | 设置 mstateen1 bit 63 为 0/1，在 S-mode 访问 sstateen1 | bit63=0 触发异常，bit63=1 正常 | `norm:mstateen_bit_63_op` |
| MSTA-B63-05 | mstateen2 bit 63 控制 sstateen2 | 设置 mstateen2 bit 63 为 0/1，在 S-mode 访问 sstateen2 | bit63=0 触发异常，bit63=1 正常 | `norm:mstateen_bit_63_op` |
| MSTA-B63-06 | mstateen3 bit 63 控制 sstateen3 | 设置 mstateen3 bit 63 为 0/1，在 S-mode 访问 sstateen3 | bit63=0 触发异常，bit63=1 正常 | `norm:mstateen_bit_63_op` |
| MSTA-B63-07 | mstateen bit 63 可写性条件 **[已迁移至 Hypervisor_cross_test_plan.md]** | 检查 mstateen0 bit 63 是否可写（有 H 扩展或 sstateen 非全只读零） | 满足条件时可写，否则 RO0 | `norm:mstateen_bit_63_roz` |

**测试步骤示例 (MSTA-B63-01)**：
1. 在 M-mode 下将 mstateen0 bit 63 (SE0) 清零
2. 配置 PMP 允许 S-mode 执行
3. 在 S-mode 安装 trap handler
4. 切换到 S-mode
5. 尝试读取 sstateen0 CSR
6. 返回 M-mode 检查是否捕获到 illegal-instruction 异常

**预期结果**：
- S-mode 读取 sstateen0 触发 illegal-instruction 异常（cause = 2）

**断言**：
```
EXPECT_TRAP(CAUSE_ILLEGAL_INSTRUCTION, read_csr(sstateen0));
CHECK_TRAP("S-mode access sstateen0 with mstateen0.SE0=0", CAUSE_ILLEGAL_INSTRUCTION);
```

---

### Group 6：mstateen0 各功能位控制

**规范依据**：
- `norm:stateen0_c_op`：C 位控制自定义状态
- `norm:stateen0_fcsr_op`：FCSR 位控制 Zfinx 场景 fcsr
- `norm:mstateen0_fcsr_roz`：misa.F=1 时 FCSR 为只读零
- `norm:stateen0_fcsr0_misa_f0_illegal_fpu_instr`：stateen 实现且 misa.F=0、FCSR=0 时所有浮点指令非法
- `norm:stateen0_jvt_op`：JVT 位控制 Zcmt 的 jvt CSR
- `norm:mstateen0_se0_op`：SE0 控制 hstateen0/hstateen0h/sstateen0
- `norm:mstateen0_envcfg_op`：ENVCFG 控制 henvcfg/henvcfgh/senvcfg
- `norm:mstateen0_csrind_op`：CSRIND 控制 siselect/sireg*/vsiselect/vsireg*
- `norm:mstateen0_imsic_op`：IMSIC 控制 stopei/vstopei
- `norm:mstateen0_aia_op`：AIA 控制 Ssaia 剩余状态
- `norm:mstateen0_context_op`：CONTEXT 控制 scontext/hcontext
- `norm:mstateen0_p1p13_op`：P1P13 控制 hedelegh
- `norm:mstateen0_srmcfg_op`：SRMCFG 控制 srmcfg

#### 6.1 C 位（bit 0）— 自定义状态控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-C-01 | mstateen0.C=0 阻止 S-mode 自定义状态 | 设 mstateen0.C=0，S-mode 访问自定义 CSR | 触发 illegal-instruction 异常 | `norm:stateen0_c_op` |
| MSTA-C-02 | mstateen0.C=1 允许 S-mode 自定义状态 | 设 mstateen0.C=1，S-mode 访问自定义 CSR | 访问正常（取决于自定义扩展实现） | `norm:stateen0_c_op` |

#### 6.2 FCSR 位（bit 1）— Zfinx 场景 fcsr 控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-FCSR-01 | misa.F=1 时 FCSR 位只读零 | 检查 misa.F，若为 1 则写 mstateen0.FCSR=1 后读回 | FCSR 位读回为 0 | `norm:mstateen0_fcsr_roz` |
| MSTA-FCSR-02 | misa.F=0 且 FCSR=0 时浮点指令非法 | 确保 misa.F=0 且 mstateen0.FCSR=0，S-mode 执行浮点指令 | 触发 illegal-instruction 异常 | `norm:stateen0_fcsr0_misa_f0_illegal_fpu_instr` |
| MSTA-FCSR-03 | misa.F=0 且 FCSR=1 时浮点 fcsr 可访问 | 确保 misa.F=0 且 mstateen0.FCSR=1，S-mode 访问 fcsr | 访问正常，无异常 | `norm:stateen0_fcsr_op` |

#### 6.3 JVT 位（bit 2）— Zcmt jvt CSR 控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-JVT-01 | mstateen0.JVT=0 阻止 S-mode 访问 jvt | 设 mstateen0.JVT=0，S-mode 读 jvt CSR | 触发 illegal-instruction 异常 | `norm:stateen0_jvt_op` |
| MSTA-JVT-02 | mstateen0.JVT=1 允许 S-mode 访问 jvt | 设 mstateen0.JVT=1，S-mode 读 jvt CSR | 访问正常 | `norm:stateen0_jvt_op` |

#### 6.4 SE0 位（bit 63）— sstateen0/hstateen0 访问控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-SE0-01 | mstateen0.SE0=0 阻止 S-mode 访问 sstateen0 | 设 mstateen0.SE0=0，S-mode 读 sstateen0 | 触发 illegal-instruction 异常 | `norm:mstateen0_se0_op` |
| MSTA-SE0-02 | mstateen0.SE0=0 阻止访问 hstateen0 **[已迁移至 Hypervisor_cross_test_plan.md]** | 设 mstateen0.SE0=0，HS-mode 读 hstateen0 | 触发 illegal-instruction 异常 | `norm:mstateen0_se0_op` |
| MSTA-SE0-03 | mstateen0.SE0=0 阻止访问 hstateen0h (RV32) **[已迁移至 Hypervisor_cross_test_plan.md]** | 设 mstateen0.SE0=0，HS-mode 读 hstateen0h | 触发 illegal-instruction 异常 | `norm:mstateen0_se0_op` |
| MSTA-SE0-04 | mstateen0.SE0=1 允许访问 | 设 mstateen0.SE0=1，S/HS-mode 读 sstateen0/hstateen0 | 访问正常 | `norm:mstateen0_se0_op` |

#### 6.5 ENVCFG 位（bit 62）— senvcfg/henvcfg 访问控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-ENVCFG-01 | mstateen0.ENVCFG=0 阻止 S-mode 访问 senvcfg | 设 ENVCFG=0，S-mode 读 senvcfg | 触发 illegal-instruction 异常 | `norm:mstateen0_envcfg_op` |
| MSTA-ENVCFG-02 | mstateen0.ENVCFG=0 阻止访问 henvcfg **[已迁移至 Hypervisor_cross_test_plan.md]** | 设 ENVCFG=0，HS-mode 读 henvcfg | 触发 illegal-instruction 异常 | `norm:mstateen0_envcfg_op` |
| MSTA-ENVCFG-03 | mstateen0.ENVCFG=1 允许访问 | 设 ENVCFG=1，S/HS-mode 读 senvcfg/henvcfg | 访问正常 | `norm:mstateen0_envcfg_op` |

#### 6.6 CSRIND 位（bit 60）— 间接 CSR 访问控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-CSRIND-01 | mstateen0.CSRIND=0 阻止访问 siselect | 设 CSRIND=0，S-mode 读 siselect | 触发 illegal-instruction 异常 | `norm:mstateen0_csrind_op` |
| MSTA-CSRIND-02 | mstateen0.CSRIND=0 阻止访问 sireg* | 设 CSRIND=0，S-mode 读 sireg | 触发 illegal-instruction 异常 | `norm:mstateen0_csrind_op` |
| MSTA-CSRIND-03 | mstateen0.CSRIND=0 阻止访问 vsiselect **[已迁移至 Hypervisor_cross_test_plan.md]** | 设 CSRIND=0，HS-mode 读 vsiselect | 触发 illegal-instruction 异常 | `norm:mstateen0_csrind_op` |
| MSTA-CSRIND-04 | mstateen0.CSRIND=1 允许访问 | 设 CSRIND=1，S/HS-mode 读 siselect/sireg* | 访问正常 | `norm:mstateen0_csrind_op` |

#### 6.7 IMSIC 位（bit 58）— IMSIC 状态控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-IMSIC-01 | mstateen0.IMSIC=0 阻止访问 stopei | 设 IMSIC=0，S-mode 读 stopei | 触发 illegal-instruction 异常 | `norm:mstateen0_imsic_op` |
| MSTA-IMSIC-02 | mstateen0.IMSIC=0 阻止访问 vstopei **[已迁移至 Hypervisor_cross_test_plan.md]** | 设 IMSIC=0，HS-mode 读 vstopei | 触发 illegal-instruction 异常 | `norm:mstateen0_imsic_op` |
| MSTA-IMSIC-03 | mstateen0.IMSIC=1 允许访问 | 设 IMSIC=1，S/HS-mode 读 stopei/vstopei | 访问正常 | `norm:mstateen0_imsic_op` |

#### 6.8 AIA 位（bit 59）— Ssaia 剩余状态控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-AIA-01 | mstateen0.AIA=0 阻止访问 Ssaia 状态 | 设 AIA=0，S-mode 访问 Ssaia 引入的非 CSRIND/IMSIC 状态 | 触发 illegal-instruction 异常 | `norm:mstateen0_aia_op` |
| MSTA-AIA-02 | mstateen0.AIA=1 允许访问 | 设 AIA=1，S-mode 访问 Ssaia 剩余状态 | 访问正常 | `norm:mstateen0_aia_op` |

#### 6.9 CONTEXT 位（bit 57）— Sdtrig scontext/hcontext 控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-CTX-01 | mstateen0.CONTEXT=0 阻止访问 scontext | 设 CONTEXT=0，S-mode 读 scontext | 触发 illegal-instruction 异常 | `norm:mstateen0_context_op` |
| MSTA-CTX-02 | mstateen0.CONTEXT=0 阻止访问 hcontext **[已迁移至 Hypervisor_cross_test_plan.md]** | 设 CONTEXT=0，HS-mode 读 hcontext | 触发 illegal-instruction 异常 | `norm:mstateen0_context_op` |
| MSTA-CTX-03 | mstateen0.CONTEXT=1 允许访问 | 设 CONTEXT=1，S/HS-mode 读 scontext/hcontext | 访问正常 | `norm:mstateen0_context_op` |

#### 6.10 P1P13 位（bit 56）— hedelegh 控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-P1P13-01 | mstateen0.P1P13=0 阻止访问 hedelegh **[已迁移至 Hypervisor_cross_test_plan.md]** | 设 P1P13=0，HS-mode 读 hedelegh | 触发 illegal-instruction 异常 | `norm:mstateen0_p1p13_op` |
| MSTA-P1P13-02 | mstateen0.P1P13=1 允许访问 hedelegh **[已迁移至 Hypervisor_cross_test_plan.md]** | 设 P1P13=1，HS-mode 读 hedelegh | 访问正常 | `norm:mstateen0_p1p13_op` |

#### 6.11 SRMCFG 位（bit 55）— srmcfg 控制

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-SRMCFG-01 | mstateen0.SRMCFG=0 阻止访问 srmcfg | 设 SRMCFG=0，S-mode 读 srmcfg | 触发 illegal-instruction 异常 | `norm:mstateen0_srmcfg_op` |
| MSTA-SRMCFG-02 | mstateen0.SRMCFG=1 允许访问 srmcfg | 设 SRMCFG=1，S-mode 读 srmcfg | 访问正常 | `norm:mstateen0_srmcfg_op` |

---

### Group 7：mstateen 访问控制异常行为

**规范依据**：
- `norm:stateen_op`：stateen 控制所有低特权级访问，不控制本级
- `norm:stateen_illegal_state_access`：被阻止的访问触发 illegal-instruction 异常；VS/VU 模式下若满足条件则触发 virtual-instruction 异常
- `norm:stateen_implicit_state_update`：隐式更新状态的指令是否触发异常需明确规定

| 测试 ID | 测试名称 | 测试描述 | 预期结果 | 规范引用 |
|---------|----------|----------|----------|----------|
| MSTA-EXC-01 | mstateen 不控制 M-mode 自身 | M-mode 下 mstateen0 某位为 0 时，M-mode 访问受控状态 | M-mode 访问正常，无异常 | `norm:stateen_op` |
| MSTA-EXC-02 | 被阻止的 S-mode 读触发 illegal-instruction | mstateen0 某位=0，S-mode 读受控 CSR | 触发 illegal-instruction 异常 (cause=2) | `norm:stateen_illegal_state_access` |
| MSTA-EXC-03 | 被阻止的 S-mode 写触发 illegal-instruction | mstateen0 某位=0，S-mode 写受控 CSR | 触发 illegal-instruction 异常 (cause=2) | `norm:stateen_illegal_state_access` |
| MSTA-EXC-04 | VS-mode 访问触发 virtual-instruction **[已迁移至 Hypervisor_cross_test_plan.md]** | mstateen0 某位=0 且从 VS-mode 访问，满足虚拟指令异常条件 | 触发 virtual-instruction 异常 (cause=22) | `norm:stateen_illegal_state_access` |
| MSTA-EXC-05 | VU-mode 访问触发 virtual-instruction **[已迁移至 Hypervisor_cross_test_plan.md]** | mstateen0 某位=0 且从 VU-mode 访问，满足虚拟指令异常条件 | 触发 virtual-instruction 异常 (cause=22) | `norm:stateen_illegal_state_access` |
| MSTA-EXC-06 | 隐式状态更新指令行为 | 执行隐式更新受控状态但不读取的指令（mstateen 阻止访问时） | 行为取决于具体规范定义（可能触发也可能不触发异常） | `norm:stateen_implicit_state_update` |

**测试步骤示例 (MSTA-EXC-01)**：
1. 在 M-mode 下将 mstateen0 的 ENVCFG 位设为 0
2. 在 M-mode 下直接读取 senvcfg CSR
3. 验证读取正常，无异常

**预期结果**：
- M-mode 不受 mstateen 控制，访问正常

**断言**：
```
clear_csr(mstateen0, MSTATEEN0_ENVCFG);
CHECK_NO_TRAP("M-mode read senvcfg with mstateen0.ENVCFG=0");
uint64_t val = read_csr(senvcfg);  /* should succeed */
```

---

## 4. 验证计划

### 4.1 自动化测试

```bash
# 编译 smstateen 测试
make smstateen CROSS_COMPILER=/path/to/riscv64-unknown-elf-

# 在 QEMU 上运行
qemu-system-riscv64 -M virt -cpu max -bios none \
    -kernel smstateen/smstateen_test.elf -m 256M -smp 1 -nographic

# 在 Spike 上运行
make spike-smstateen CROSS_COMPILER=/path/to/riscv64-unknown-elf-

# 在 Sail 上运行
make sail-smstateen CROSS_COMPILER=/path/to/riscv64-unknown-elf-
```

### 4.2 手动验证

- 验证所有 M-mode 相关 normative rules 是否被至少一个测试用例覆盖
- 确认 mstateen 对低特权级的访问控制行为（异常触发）正确

---

## 5. 测试判定标准

### 5.1 通过标准

- 所有测试用例的所有断言均通过
- 测试框架输出 `RESULT: ALL PASSED`

### 5.2 失败标准

- 任一断言失败即判定对应测试用例 FAIL
- 出现非预期的异常（trap）即判定 FAIL

---

## 6. 覆盖率矩阵

### 6.1 Normative Rules 覆盖追溯

| 规范引用 | 覆盖测试 ID |
|----------|------------|
| `norm:mstateen_rv64_csrs` | MSTA-EXIST-01~04 |
| `norm:stateen_rv32_upper_bits_csrs` | MSTA-EXIST-05 |
| `norm:stateen_op` | MSTA-EXC-01 |
| `norm:stateen_illegal_state_access` | MSTA-EXC-02~05 |
| `norm:stateen_implicit_state_update` | MSTA-EXC-06 |
| `norm:stateen_warl_access` | MSTA-WARL-03 |
| `norm:stateen_unimplemented_state_roz` | MSTA-WARL-02 |
| `norm:stateen_reserved_roz` | MSTA-WARL-01, MSTA-WARL-04~06 |
| `norm:mstateen_lower_priv_roz` | MSTA-PROP-01~04 |
| `norm:mstateen_zero_initialization` | MSTA-INIT-01~04 |
| `norm:hstateen_sstateen_zero_initialization` | MSTA-INIT-05~06 |
| `norm:mstateen_bit_63_op` | MSTA-B63-01~06 |
| `norm:mstateen_bit_63_roz` | MSTA-B63-07 |
| `norm:mstateen_bit_allocation` | MSTA-WARL-01 (隐式) |
| `norm:stateen0_c_op` | MSTA-C-01~02 |
| `norm:stateen0_fcsr_op` | MSTA-FCSR-03 |
| `norm:mstateen0_fcsr_roz` | MSTA-FCSR-01 |
| `norm:stateen0_fcsr0_misa_f0_illegal_fpu_instr` | MSTA-FCSR-02 |
| `norm:stateen0_jvt_op` | MSTA-JVT-01~02 |
| `norm:mstateen0_se0_op` | MSTA-SE0-01~04 |
| `norm:mstateen0_envcfg_op` | MSTA-ENVCFG-01~03 |
| `norm:mstateen0_csrind_op` | MSTA-CSRIND-01~04 |
| `norm:mstateen0_imsic_op` | MSTA-IMSIC-01~03 |
| `norm:mstateen0_aia_op` | MSTA-AIA-01~02 |
| `norm:mstateen0_context_op` | MSTA-CTX-01~03 |
| `norm:mstateen0_p1p13_op` | MSTA-P1P13-01~02 |
| `norm:mstateen0_srmcfg_op` | MSTA-SRMCFG-01~02 |
