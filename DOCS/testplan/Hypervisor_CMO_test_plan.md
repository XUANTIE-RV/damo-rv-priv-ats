# Hypervisor × CMO 交叉测试计划

## 概述

本测试计划覆盖 RISC-V Hypervisor (H) 扩展与 CMO (Cache Management Operations) 扩展的交叉功能点。

依据规范：
- `SPEC/hypervisor.adoc`：henvcfg CBIE/CBCFE/CBZE 字段定义
- `SPEC/riscv-isa-manual/src/unpriv/cmo.adoc`：CMO 指令在 V=1 时的行为、htinst/mtinst 标准转换

CMO 扩展包含三个子扩展：
- **Zicbom**：Cache-Block Management 指令（cbo.inval, cbo.clean, cbo.flush）
- **Zicboz**：Cache-Block Zero 指令（cbo.zero）
- **Zicbop**：Cache-Block Prefetch 指令（prefetch.r, prefetch.w, prefetch.i）

### 本文档覆盖的 SPEC 章节
- henvcfg CBIE/CBCFE/CBZE 字段对 VS/VU-mode CBO 指令的控制
- cbo.inval 在 V=1 时的 flush 覆盖语义（HS-mode CBIE=01 时强制 flush）
- CBO 指令的 htinst/mtinst 标准转换格式
- CMO 指令在 G-stage 地址翻译下的 guest-page-fault 行为
- prefetch 指令在 VS/VU-mode 下不触发 virtual-instruction

### 由其他测试计划覆盖
- CMO 基础功能（M/HS/U-mode CSR 控制、功能语义、编码）→ `CMO_test_plan.md`
- Hypervisor 核心功能（hstatus/hedeleg/hideleg/VS CSR）→ `Hypervisor_test_plan.md`

---

## 覆盖的规范点

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:henvcfg_cbze` | The Zicboz extension adds the CBZE field to henvcfg. The CBZE field applies to execution of CBO.ZERO in privilege modes VS and VU, and only when the instruction is HS-qualified. If not HS-qualified, raises illegal-instruction. If HS-qualified and CBZE=1, enabled; if CBZE=0, raises virtual-instruction. When Zicboz not implemented, CBZE is read-only zero. | henvcfg.CBZE 控制 VS/VU 模式下 CBO.ZERO 的执行。非 HS 限定引发 illegal-instruction；HS 限定且 CBZE=0 引发 virtual-instruction。未实现 Zicboz 时只读零。 |
| `norm:henvcfg_cbcfe` | The Zicbom extension adds the CBCFE field to henvcfg. When V=1, if CBO.CLEAN and CBO.FLUSH are not HS-qualified, they raise illegal-instruction. If HS-qualified and CBCFE=1, enabled; if CBCFE=0, raises virtual-instruction. When Zicbom not implemented, CBCFE is read-only zero. | henvcfg.CBCFE 控制 VS/VU 模式下 CBO.CLEAN/CBO.FLUSH 的执行。未实现 Zicbom 时只读零。 |
| `norm:henvcfg_cbie` | The Zicbom extension adds the CBIE WARL field to henvcfg. The CBIE field controls execution of CBO.INVAL in privilege modes VS and VU. The encoding 10b is reserved. When Zicbom not implemented, CBIE is read-only zero. | henvcfg.CBIE WARL 字段控制 VS/VU 模式下 CBO.INVAL 的执行。编码 10b 保留。未实现时只读零。 |
| `norm:cbo-inval_h-mode_veq1_op` | When V=1, if CBO.INVAL is not HS-qualified, raises illegal-instruction. If HS-qualified and CBIE=01b or 11b, enabled; otherwise raises virtual-instruction. | V=1 时 CBO.INVAL 非 HS 限定引发 illegal-instruction；HS 限定且 CBIE≠00 时使能。 |
| `norm:cbo-inval_h-mode_op0` | If CBO.INVAL is enabled in HS-mode to perform a flush operation, then when enabled in VS/VU-mode it performs a flush operation, even if CBIE=11b. Otherwise, behavior depends on CBIE encoding. | 若 HS-mode 配置 cbo.inval 执行 flush（menvcfg.CBIE=01），则 VS/VU-mode 也执行 flush，即使 henvcfg.CBIE=11。 |
| `norm:cbo-inval_h-mode_op1` | CBIE=01b: The instruction is executed and performs a flush operation, even if configured by VS-mode to perform an invalidate operation. | henvcfg.CBIE=01 时执行 flush，即使 VS-mode 配置为 invalidate。 |
| `norm:cbo-inval_h-mode_op2` | CBIE=11b: The instruction is executed and performs an invalidate operation, unless configured by VS-mode to perform a flush operation. | henvcfg.CBIE=11 时执行 invalidate，除非 VS-mode 配置为 flush。 |
| `norm:h_trans_cache` | For writing mtinst or htinst on a trap, the standard transformation for cache-block management and zero instructions: {operation[11:0], 0x0, funct3, 0x0, opcode}. The operation field is the 12 most significant bits of the trapping instruction. | CBO 指令的 htinst/mtinst 标准转换格式。 |
| `norm:cbm_unperm_fault` | If access not permitted, a cache-block management instruction raises store page-fault or store guest-page-fault if address translation does not permit any access. | management 指令在 G-stage 翻译不允许时引发 store guest-page-fault。 |
| `norm:cbz_unperm_fault` | If access not permitted, a cache-block zero instruction raises store page-fault or store guest-page-fault if address translation does not permit write access. | cbo.zero 在 G-stage 无写权限时引发 store guest-page-fault。 |
| `norm:cbp_unperm_noexcep` | If access not permitted, a cache-block prefetch instruction does not raise any exceptions. | prefetch 在任何模式下访问不允许时不引发异常。 |
| `norm:fault_excep_csr` | When a page-fault, guest-page-fault, or access-fault exception is taken, the relevant *tval CSR is written with the faulting effective address (i.e. the value of rs1). | 异常时 *tval 写入 rs1 值。 |
| `norm:cbxe_unaffected` | The CBIE/CBCFE/CBZE fields in each envcfg register do not affect the read and write behavior of the same fields in the other envcfg registers. | 各 envcfg 的 CMO 字段互不影响。 |

---

## Group 1. henvcfg CBIE 控制 — VS/VU-mode cbo.inval

**规范依据**：
- `norm:henvcfg_cbie`：henvcfg.CBIE WARL 字段，编码 10b 保留，未实现时只读零
- `norm:cbo-inval_h-mode_veq1_op`：V=1 时 CBO.INVAL 的异常/执行判定
- `norm:cbo-inval_h-mode_op0`：HS-mode flush 配置覆盖 VS/VU 行为
- `norm:cbo-inval_h-mode_op1`：henvcfg.CBIE=01 执行 flush
- `norm:cbo-inval_h-mode_op2`：henvcfg.CBIE=11 执行 invalidate（除非 VS-mode 配置 flush）

**测试职责**：验证 henvcfg.CBIE 对 VS/VU-mode cbo.inval 的控制，包括 flush 覆盖语义。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCBIE-01 | VS-mode henvcfg.CBIE=00 触发 virtual-instruction | menvcfg.CBIE=11, henvcfg.CBIE=00，VS-mode 执行 cbo.inval | virtual-instruction exception (cause=22) |
| HCBIE-02 | VS-mode henvcfg.CBIE=01 执行 flush | menvcfg.CBIE=11, henvcfg.CBIE=01，VS-mode 执行 cbo.inval | 正常执行（flush 操作） |
| HCBIE-03 | VS-mode henvcfg.CBIE=11 执行 invalidate | menvcfg.CBIE=11, henvcfg.CBIE=11，VS-mode 执行 cbo.inval | 正常执行 invalidate |
| HCBIE-04 | VU-mode henvcfg.CBIE=00 触发 virtual-instruction | menvcfg.CBIE=11, henvcfg.CBIE=00，VU-mode 执行 cbo.inval | virtual-instruction exception (cause=22) |
| HCBIE-05 | VU-mode senvcfg.CBIE=00 触发 virtual-instruction | menvcfg.CBIE=11, henvcfg.CBIE=11, senvcfg.CBIE=00，VU-mode 执行 cbo.inval | virtual-instruction exception (cause=22) |
| HCBIE-06 | VU-mode 两级 CBIE 均非零执行 | menvcfg.CBIE=11, henvcfg.CBIE=11, senvcfg.CBIE=11，VU-mode 执行 cbo.inval | 正常执行 invalidate |
| HCBIE-07 | VU-mode henvcfg.CBIE=01 执行 flush | menvcfg.CBIE=11, henvcfg.CBIE=01, senvcfg.CBIE=11，VU-mode 执行 cbo.inval | 正常执行（flush 操作） |
| HCBIE-08 | VU-mode senvcfg.CBIE=01 执行 flush | menvcfg.CBIE=11, henvcfg.CBIE=11, senvcfg.CBIE=01，VU-mode 执行 cbo.inval | 正常执行（flush 操作） |
| HCBIE-09 | henvcfg.CBIE WARL 保留编码 10 | 写 henvcfg.CBIE=10，读回 | WARL：不保留编码 10b |
| HCBIE-10 | henvcfg.CBIE 未实现 Zicbom 时只读零 | 若 Zicbom 未实现，读 henvcfg.CBIE | 只读零 |
| HCBIE-11 | menvcfg.CBIE=01 时 VS-mode 强制 flush（覆盖 henvcfg.CBIE=11） | menvcfg.CBIE=01, henvcfg.CBIE=11，VS-mode 执行 cbo.inval | 执行 flush（HS-mode flush 配置覆盖） |
| HCBIE-12 | menvcfg.CBIE=01 时 VU-mode 强制 flush | menvcfg.CBIE=01, henvcfg.CBIE=11, senvcfg.CBIE=11，VU-mode 执行 cbo.inval | 执行 flush（HS-mode flush 配置覆盖） |
| HCBIE-13 | henvcfg.CBIE 不影响 menvcfg/senvcfg.CBIE | 写 henvcfg.CBIE=00，读 menvcfg.CBIE/senvcfg.CBIE | 其他 envcfg 的 CBIE 不受影响 |

---

## Group 2. henvcfg CBCFE 控制 — VS/VU-mode cbo.clean / cbo.flush

**规范依据**：
- `norm:henvcfg_cbcfe`：henvcfg.CBCFE 控制 VS/VU-mode CBO.CLEAN/CBO.FLUSH，未实现时只读零
- `norm:cbxe_unaffected`：各 envcfg 的 CBCFE 字段互不影响

**测试职责**：验证 henvcfg.CBCFE 对 VS/VU-mode cbo.clean 和 cbo.flush 的控制。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCBCFE-01 | VS-mode henvcfg.CBCFE=0 cbo.clean 触发 virtual-instruction | menvcfg.CBCFE=1, henvcfg.CBCFE=0，VS-mode 执行 cbo.clean | virtual-instruction exception (cause=22) |
| HCBCFE-02 | VS-mode henvcfg.CBCFE=0 cbo.flush 触发 virtual-instruction | menvcfg.CBCFE=1, henvcfg.CBCFE=0，VS-mode 执行 cbo.flush | virtual-instruction exception (cause=22) |
| HCBCFE-03 | VS-mode henvcfg.CBCFE=1 cbo.clean 正常执行 | menvcfg.CBCFE=1, henvcfg.CBCFE=1，VS-mode 执行 cbo.clean | 正常执行 |
| HCBCFE-04 | VS-mode henvcfg.CBCFE=1 cbo.flush 正常执行 | menvcfg.CBCFE=1, henvcfg.CBCFE=1，VS-mode 执行 cbo.flush | 正常执行 |
| HCBCFE-05 | VU-mode henvcfg.CBCFE=0 触发 virtual-instruction | menvcfg.CBCFE=1, henvcfg.CBCFE=0，VU-mode 执行 cbo.clean | virtual-instruction exception (cause=22) |
| HCBCFE-06 | VU-mode senvcfg.CBCFE=0 触发 virtual-instruction | menvcfg.CBCFE=1, henvcfg.CBCFE=1, senvcfg.CBCFE=0，VU-mode 执行 cbo.clean | virtual-instruction exception (cause=22) |
| HCBCFE-07 | VU-mode 两级 CBCFE=1 cbo.clean 正常执行 | menvcfg.CBCFE=1, henvcfg.CBCFE=1, senvcfg.CBCFE=1，VU-mode 执行 cbo.clean | 正常执行 |
| HCBCFE-08 | VU-mode 两级 CBCFE=1 cbo.flush 正常执行 | menvcfg.CBCFE=1, henvcfg.CBCFE=1, senvcfg.CBCFE=1，VU-mode 执行 cbo.flush | 正常执行 |
| HCBCFE-09 | henvcfg.CBCFE 未实现 Zicbom 时只读零 | 若 Zicbom 未实现，读 henvcfg.CBCFE | 只读零 |
| HCBCFE-10 | henvcfg.CBCFE 不影响 menvcfg/senvcfg.CBCFE | 写 henvcfg.CBCFE=0，读 menvcfg.CBCFE/senvcfg.CBCFE | 其他 envcfg 的 CBCFE 不受影响 |

---

## Group 3. henvcfg CBZE 控制 — VS/VU-mode cbo.zero

**规范依据**：
- `norm:henvcfg_cbze`：henvcfg.CBZE 控制 VS/VU-mode CBO.ZERO，未实现时只读零
- `norm:cbxe_unaffected`：各 envcfg 的 CBZE 字段互不影响

**测试职责**：验证 henvcfg.CBZE 对 VS/VU-mode cbo.zero 的控制。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCBZE-01 | VS-mode henvcfg.CBZE=0 触发 virtual-instruction | menvcfg.CBZE=1, henvcfg.CBZE=0，VS-mode 执行 cbo.zero | virtual-instruction exception (cause=22) |
| HCBZE-02 | VS-mode henvcfg.CBZE=1 正常执行 | menvcfg.CBZE=1, henvcfg.CBZE=1，VS-mode 执行 cbo.zero | 正常执行 |
| HCBZE-03 | VU-mode henvcfg.CBZE=0 触发 virtual-instruction | menvcfg.CBZE=1, henvcfg.CBZE=0，VU-mode 执行 cbo.zero | virtual-instruction exception (cause=22) |
| HCBZE-04 | VU-mode senvcfg.CBZE=0 触发 virtual-instruction | menvcfg.CBZE=1, henvcfg.CBZE=1, senvcfg.CBZE=0，VU-mode 执行 cbo.zero | virtual-instruction exception (cause=22) |
| HCBZE-05 | VU-mode 两级 CBZE=1 正常执行 | menvcfg.CBZE=1, henvcfg.CBZE=1, senvcfg.CBZE=1，VU-mode 执行 cbo.zero | 正常执行 |
| HCBZE-06 | henvcfg.CBZE 未实现 Zicboz 时只读零 | 若 Zicboz 未实现，读 henvcfg.CBZE | 只读零 |
| HCBZE-07 | henvcfg.CBZE 不影响 menvcfg/senvcfg.CBZE | 写 henvcfg.CBZE=0，读 menvcfg.CBZE/senvcfg.CBZE | 其他 envcfg 的 CBZE 不受影响 |

---

## Group 4. CMO htinst/mtinst 标准转换

**规范依据**：
- `norm:h_trans_cache`：CBO 指令的 htinst/mtinst 标准转换格式为 {operation[11:0], 0x0, funct3, 0x0, opcode}
- Hypervisor SPEC：htinst 可写入零替代标准转换

**标准转换格式**：
```
[31:20] operation  (指令最高 12 位)
[19:15] 0x0
[14:12] funct3 (=2 for CBO)
[11:7]  0x0
[6:0]   opcode (=0x0F for MISC-MEM)
```

转换值：
- cbo.inval → 0x0000200F
- cbo.clean → 0x0010200F
- cbo.flush → 0x0020200F
- cbo.zero  → 0x0040200F

**测试职责**：验证 CBO 指令触发 trap 时 htinst/mtinst 的正确写入。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCXINST-01 | cbo.clean page-fault 时 htinst 标准转换 | VS-mode 执行 cbo.clean 触发 page-fault 到 HS-mode，读 htinst | htinst = 0x0010200F 或 0 |
| HCXINST-02 | cbo.flush page-fault 时 htinst 标准转换 | VS-mode 执行 cbo.flush 触发 page-fault，读 htinst | htinst = 0x0020200F 或 0 |
| HCXINST-03 | cbo.inval page-fault 时 htinst 标准转换 | VS-mode 执行 cbo.inval 触发 page-fault，读 htinst | htinst = 0x0000200F 或 0 |
| HCXINST-04 | cbo.zero page-fault 时 htinst 标准转换 | VS-mode 执行 cbo.zero 触发 page-fault，读 htinst | htinst = 0x0040200F 或 0 |
| HCXINST-05 | cbo.clean trap 到 M-mode 时 mtinst | VS-mode 执行 cbo.clean 触发 trap 到 M-mode，读 mtinst | mtinst = 0x0010200F 或 0 |
| HCXINST-06 | cbo.zero trap 到 M-mode 时 mtinst | VS-mode 执行 cbo.zero 触发 trap 到 M-mode，读 mtinst | mtinst = 0x0040200F 或 0 |
| HCXINST-07 | cbo.inval virtual-instruction 时 htinst | henvcfg.CBIE=00，VS-mode 执行 cbo.inval 触发 virtual-instruction，读 htinst | htinst = 0x0000200F 或 0 |
| HCXINST-08 | cbo.clean virtual-instruction 时 htinst | henvcfg.CBCFE=0，VS-mode 执行 cbo.clean 触发 virtual-instruction，读 htinst | htinst = 0x0010200F 或 0 |
| HCXINST-09 | cbo.zero virtual-instruction 时 htinst | henvcfg.CBZE=0，VS-mode 执行 cbo.zero 触发 virtual-instruction，读 htinst | htinst = 0x0040200F 或 0 |
| HCXINST-10 | VU-mode cbo.inval virtual-instruction htinst | henvcfg.CBIE=00，VU-mode 执行 cbo.inval，读 htinst | htinst = 0x0000200F 或 0 |
| HCXINST-11 | VU-mode cbo.zero virtual-instruction htinst | henvcfg.CBZE=0，VU-mode 执行 cbo.zero，读 htinst | htinst = 0x0040200F 或 0 |

---

## Group 5. CMO G-stage 地址翻译异常

**规范依据**：
- `norm:cbm_unperm_fault`：management 指令在 G-stage 翻译不允许时引发 store guest-page-fault
- `norm:cbz_unperm_fault`：cbo.zero 在 G-stage 无写权限时引发 store guest-page-fault
- `norm:fault_excep_csr`：*tval 写入 rs1 值
- `norm:cbp_unperm_noexcep`：prefetch 不引发异常

**测试职责**：验证 CMO 指令在 V=1 时 G-stage 地址翻译下的异常行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HCGSTAGE-01 | cbo.clean G-stage 无权限触发 guest-page-fault | VS-mode，G-stage 页表无权限，执行 cbo.clean | store guest-page-fault (cause=21) |
| HCGSTAGE-02 | cbo.flush G-stage 无权限触发 guest-page-fault | VS-mode，G-stage 页表无权限，执行 cbo.flush | store guest-page-fault (cause=21) |
| HCGSTAGE-03 | cbo.inval G-stage 无权限触发 guest-page-fault | VS-mode，G-stage 页表无权限，执行 cbo.inval | store guest-page-fault (cause=21) |
| HCGSTAGE-04 | cbo.zero G-stage 无写权限触发 guest-page-fault | VS-mode，G-stage 页表只读，执行 cbo.zero | store guest-page-fault (cause=21) |
| HCGSTAGE-05 | cbo.clean G-stage guest-page-fault stval=rs1 | 触发 guest-page-fault，检查 stval | stval = rs1 的值 |
| HCGSTAGE-06 | cbo.zero G-stage guest-page-fault stval=rs1 | 触发 guest-page-fault，检查 stval | stval = rs1 的值 |
| HCGSTAGE-07 | cbo.zero G-stage guest-page-fault htval | 触发 guest-page-fault，检查 htval | htval = 故障 GPA >> 2 |
| HCGSTAGE-08 | cbo.clean 有读权限无写权限可执行（G-stage） | G-stage 页表只读，执行 cbo.clean | 正常执行（management 只需 load 或 store 任一允许） |
| HCGSTAGE-09 | cbo.inval 有读权限无写权限可执行（G-stage） | G-stage 页表只读，执行 cbo.inval | 正常执行 |
| HCGSTAGE-10 | cbo.zero 仅有读权限触发 guest-page-fault | G-stage 页表只读，执行 cbo.zero | store guest-page-fault（cbo.zero 需要写权限） |
| HCGSTAGE-11 | prefetch G-stage 无权限不触发异常 | VS-mode，G-stage 页表无权限，执行 prefetch.r | 不触发任何异常 |
| HCGSTAGE-12 | cbo.clean VS-stage page-fault（非 G-stage） | VS-mode，VS-stage 页表无权限（G-stage 有权限），执行 cbo.clean | store page-fault (cause=15)，非 guest-page-fault |

---

## Group 6. CMO virtual-instruction 异常 stval 行为

**规范依据**：
- Hypervisor SPEC：virtual-instruction exception 的 stval 写入规则与 illegal-instruction 相同
- `norm:fault_excep_csr`：异常时 *tval 写入故障地址

**测试职责**：验证 CMO 指令触发 virtual-instruction 异常时 stval 的写入行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HVINST-01 | cbo.inval virtual-instruction stval | henvcfg.CBIE=00，VS-mode 执行 cbo.inval | stval 按 illegal-instruction 规则写入（指令编码或 0） |
| HVINST-02 | cbo.clean virtual-instruction stval | henvcfg.CBCFE=0，VS-mode 执行 cbo.clean | stval 按 illegal-instruction 规则写入 |
| HVINST-03 | cbo.flush virtual-instruction stval | henvcfg.CBCFE=0，VS-mode 执行 cbo.flush | stval 按 illegal-instruction 规则写入 |
| HVINST-04 | cbo.zero virtual-instruction stval | henvcfg.CBZE=0，VS-mode 执行 cbo.zero | stval 按 illegal-instruction 规则写入 |
| HVINST-05 | VU-mode cbo.inval virtual-instruction stval | henvcfg.CBIE=00，VU-mode 执行 cbo.inval | stval 按 illegal-instruction 规则写入 |

---

## Group 7. Zicbop Prefetch VS/VU-mode 行为

**规范依据**：
- `norm:cbp_unperm_noexcep`：prefetch 不引发任何异常
- CMO SPEC：prefetch 不引发 illegal-instruction 或 virtual-instruction

**测试职责**：验证 prefetch 指令在 VS/VU-mode 下不受 henvcfg CMO 字段控制，不触发 virtual-instruction。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HPREFETCH-01 | VS-mode prefetch.r 不触发 virtual-instruction | henvcfg.CBIE=00, CBCFE=0, CBZE=0，VS-mode 执行 prefetch.r | 正常执行，无异常 |
| HPREFETCH-02 | VS-mode prefetch.w 不触发 virtual-instruction | 同上配置，VS-mode 执行 prefetch.w | 正常执行，无异常 |
| HPREFETCH-03 | VS-mode prefetch.i 不触发 virtual-instruction | 同上配置，VS-mode 执行 prefetch.i | 正常执行，无异常 |
| HPREFETCH-04 | VU-mode prefetch.r 不触发 virtual-instruction | henvcfg 所有 CBO 字段=0, senvcfg 所有 CBO 字段=0，VU-mode 执行 prefetch.r | 正常执行，无异常 |
| HPREFETCH-05 | VU-mode prefetch.w 不触发 virtual-instruction | 同上配置，VU-mode 执行 prefetch.w | 正常执行，无异常 |
| HPREFETCH-06 | VU-mode prefetch.i 不触发 virtual-instruction | 同上配置，VU-mode 执行 prefetch.i | 正常执行，无异常 |
| HPREFETCH-07 | VS-mode prefetch G-stage 无权限不触发异常 | G-stage 页表无权限，VS-mode 执行 prefetch.r | 不触发任何异常 |
| HPREFETCH-08 | VS-mode prefetch 不检查 A/D 位 | VS-stage 页表 A=0 D=0，VS-mode 执行 prefetch.w | 不触发异常，A/D 位不被设置 |
