# Zkr Entropy Source Extension 测试计划

## 概述

本测试计划覆盖 RISC-V Zkr（Entropy Source）扩展的所有核心功能点，包括 `seed` CSR 的格式与状态机行为、访问控制机制、以及各特权级下的异常行为。

本测试计划依据 `SPEC/riscv-isa-manual/src/unpriv/zk.adoc` 中 Entropy Source 章节和 `SPEC/riscv-isa-manual/src/priv/machine.adoc` 中 `mseccfg` 相关章节的规范点编写。

### 本文档覆盖的 SPEC 章节
- Zkr Entropy Source Extension（seed CSR 定义、地址、格式）
- The seed CSR（OPST 状态位、entropy 字段、只读访问异常、写值忽略、wipe-on-read）
- Entropy Source Requirements（最低安全强度要求）
- Access Control to seed（mseccfg.SSEED/USEED 字段、各特权级访问控制表）

---

## 覆盖的规范点

本章节列出本文档所有测试组中引用的规范点（norm ID），已去重并按字母顺序排列。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:mseccfg_sseed_SorHS-mode_op` | When SSEED is 0, access to the seed CSR from S-/HS-mode raises an illegal-instruction exception. When SSEED is 1, read-write access to the seed CSR from S-/HS-mode is allowed; all other types of accesses raise an illegal-instruction exception. | SSEED=0 时 S/HS 模式访问 seed 引发非法指令异常；SSEED=1 时允许读写访问，其他访问类型仍引发非法指令异常。 |
| `norm:mseccfg_sseed_rdonly0` | If Zkr or S-mode is not implemented, SSEED is read-only zero. | 若未实现 Zkr 或 S 模式，SSEED 为只读零。 |
| `norm:mseccfg_sseed_useed_op_tbl` | Entropy Source Access Control table: M always available; U controlled by USEED; S/HS controlled by SSEED; VS/VU controlled by SSEED with virtual-instruction exception for HS-qualified read-write. | 熵源访问控制表：M 模式始终可用；U 模式由 USEED 控制；S/HS 由 SSEED 控制；VS/VU 由 SSEED 控制且 HS 限定的读写引发虚拟指令异常。 |
| `norm:mseccfg_sseed_useed_presence` | The Zkr extension adds the SSEED and USEED fields to the mseccfg CSR to control access to the seed CSR from modes less privileged than M. | Zkr 扩展向 mseccfg 添加 SSEED 和 USEED 字段，控制低特权级对 seed CSR 的访问。 |
| `norm:mseccfg_useed_rdonly0` | If Zkr or U-mode is not implemented, USEED is read-only zero. | 若未实现 Zkr 或 U 模式，USEED 为只读零。 |
| `norm:mseccfg_useed_U-mode_op` | When USEED is 0, access to the seed CSR in U-mode raises an illegal-instruction exception. When USEED is 1, read-write access to the seed CSR from U-mode is allowed; all other types of accesses raise an illegal-instruction exception. | USEED=0 时 U 模式访问 seed 引发非法指令异常；USEED=1 时允许读写访问，其他访问类型仍引发非法指令异常。 |
| `norm:seed_bist_latch` | Such a BIST alarm must be latched until polled at least once to enable software to record its occurrence. | BIST 告警必须被锁存直到至少被轮询一次，以便软件记录其发生。 |
| `norm:seed_csr_unpriv` | seed is an unprivileged CSR located at address 0x015. | seed 是一个非特权 CSR，位于地址 0x015。 |
| `norm:seed_entropy_unique` | Each returned seed[15:0] = entropy value represents unique randomness when OPST=ES16, even if its numerical value is the same as that of a previously polled entropy value. | OPST=ES16 时每次返回的 entropy 值代表唯一随机性，即使数值与先前轮询值相同。 |
| `norm:seed_entropy_zero_non_es16` | When OPST is not ES16, entropy must be set to 0. | OPST 非 ES16 时，entropy 必须为 0。 |
| `norm:seed_exec_mode_control` | The seed CSR is also access controlled by execution mode, and attempted read or write access will raise an illegal-instruction exception outside M-mode unless access is explicitly granted. | seed CSR 受执行模式访问控制，M 模式外的读写访问在未明确授权时引发非法指令异常。 |
| `norm:seed_min_security_256` | Any implementation of the seed CSR that limits the security strength shall not reduce it to less than 256 bits. | seed CSR 实现若限制安全强度，不得低于 256 位。 |
| `norm:seed_disable_if_weak` | If the security level is under 256 bits, then the interface must not be available. | 若安全级别低于 256 位，接口必须不可用。 |
| `norm:seed_ro_illegal` | Attempts to access the seed CSR using a read-only CSR-access instruction (csrrs/csrrc with rs1=x0 or csrrsi/csrrci with uimm=0) raise an illegal-instruction exception; any other CSR-access instruction may be used to access seed. | 使用只读 CSR 访问指令（csrrs/csrrc 且 rs1=x0，或 csrrsi/csrrci 且 uimm=0）访问 seed 引发非法指令异常；其他 CSR 访问指令可用于访问 seed。 |
| `norm:seed_wipe_on_read` | Polling (reading) must have the side effect of clearing (wipe-on-read) the entropy contents and changing the state to WAIT (unless there is entropy immediately available for ES16). Other states (BIST, WAIT, and DEAD) may be unaffected by polling. | 轮询（读取）必须有清除 entropy 内容并将状态变为 WAIT 的副作用（除非立即可用 ES16）。BIST、WAIT、DEAD 状态可能不受轮询影响。 |
| `norm:seed_write_ignore` | The write value (in rs1 or uimm) must be ignored by implementations. | 写入值（rs1 或 uimm 中的值）必须被实现忽略。 |
| `norm:zkr_seed_addr` | The entropy source extension defines the seed CSR at address 0x015. This CSR provides up to 16 physical entropy bits that can be used to seed cryptographic random bit generators. | 熵源扩展定义 seed CSR 地址为 0x015，提供最多 16 位物理熵用于种子密码学随机位生成器。 |

---

## Group 1. seed CSR 基本格式与地址

**规范依据**：
- `norm:zkr_seed_addr`：seed CSR 地址为 0x015，提供最多 16 位物理熵
- `norm:seed_csr_unpriv`：seed 是非特权 CSR，位于地址 0x015

**测试职责**：验证 seed CSR 的存在性、地址编码和基本格式。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SEED-01 | M-mode 使用 csrrw 访问 seed CSR | M-mode 执行 csrrw rd, 0x015, x0 | 正常返回 32 位值，不触发异常 |
| SEED-02 | seed CSR 地址编码验证 | 使用 CSR 地址 0x015 访问 | 正确访问 seed CSR（非其他 CSR） |
| SEED-03 | seed CSR 返回值为 32 位 | M-mode 执行 csrrw，检查返回值的位宽 | 返回 32 位值（RV64 下高 32 位为零扩展） |
| SEED-04 | seed CSR OPST 字段编码合法 | M-mode 多次读取 seed，检查 bits[31:30] | OPST 值仅为 00(BIST)/01(WAIT)/10(ES16)/11(DEAD) 之一 |

---

## Group 2. seed CSR OPST 状态与 entropy 字段

**规范依据**：
- `norm:seed_entropy_unique`：OPST=ES16 时每次返回的 entropy 代表唯一随机性
- `norm:seed_entropy_zero_non_es16`：OPST 非 ES16 时 entropy 必须为 0
- `norm:seed_wipe_on_read`：读取有 wipe-on-read 副作用，清除 entropy 并将状态变为 WAIT
- `norm:seed_bist_latch`：BIST 告警必须锁存直到被轮询

**测试职责**：验证 seed CSR 的状态机行为、entropy 字段约束和 wipe-on-read 语义。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| OPST-01 | ES16 状态时 entropy 非零可用 | M-mode 轮询 seed 直到 OPST=ES16 | bits[15:0] 包含 16 位熵值 |
| OPST-02 | 非 ES16 状态时 entropy 为零 | M-mode 轮询 seed，当 OPST=WAIT(01) 时检查 bits[15:0] | bits[15:0] = 0 |
| OPST-03 | BIST 状态时 entropy 为零 | M-mode 轮询 seed，若 OPST=BIST(00) 时检查 bits[15:0] | bits[15:0] = 0 |
| OPST-04 | DEAD 状态时 entropy 为零 | M-mode 轮询 seed，若 OPST=DEAD(11) 时检查 bits[15:0] | bits[15:0] = 0 |
| OPST-05 | wipe-on-read：ES16 读取后状态变化 | M-mode 轮询到 ES16 后立即再次读取 | 第二次读取 OPST 为 WAIT(01) 或新的 ES16（若立即有新熵），不会返回相同 entropy |
| OPST-06 | WAIT 状态轮询不改变状态 | M-mode 在 OPST=WAIT 时读取 seed | OPST 仍为 WAIT 或变为 ES16（若熵已就绪） |
| OPST-07 | 连续 ES16 值唯一性 | M-mode 连续轮询多次 ES16 成功，收集 entropy 值 | 连续值不完全相同（统计检查：连续 N 次不全相等） |
| OPST-08 | reserved 和 custom 位检查 | M-mode 读取 seed，检查 bits[29:24] | reserved 位为零（实现可安全设为零） |
| OPST-09 | BIST 告警锁存验证 | 若可触发 BIST 告警，验证告警在首次轮询前保持 | BIST 状态在首次轮询前持续可见 |

---

## Group 3. seed CSR 只读访问异常

**规范依据**：
- `norm:seed_ro_illegal`：使用只读 CSR 访问指令（csrrs/csrrc 且 rs1=x0，或 csrrsi/csrrci 且 uimm=0）访问 seed 引发 illegal-instruction exception
- `norm:seed_write_ignore`：写入值必须被实现忽略

**测试职责**：验证只读访问异常和写值忽略行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ROACC-01 | csrrs rd, seed, x0 触发异常 | M-mode 执行 csrrs rd, 0x015, x0 | illegal-instruction exception (cause=2) |
| ROACC-02 | csrrc rd, seed, x0 触发异常 | M-mode 执行 csrrc rd, 0x015, x0 | illegal-instruction exception (cause=2) |
| ROACC-03 | csrrsi rd, seed, 0 触发异常 | M-mode 执行 csrrsi rd, 0x015, 0 | illegal-instruction exception (cause=2) |
| ROACC-04 | csrrci rd, seed, 0 触发异常 | M-mode 执行 csrrci rd, 0x015, 0 | illegal-instruction exception (cause=2) |
| ROACC-05 | csrrw rd, seed, x0 正常访问 | M-mode 执行 csrrw rd, 0x015, x0 | 正常返回 seed 值，无异常 |
| ROACC-06 | csrrs rd, seed, rs1(rs1≠x0) 正常访问 | M-mode 执行 csrrs rd, 0x015, t0 (t0≠0) | 正常返回 seed 值，无异常 |
| ROACC-07 | csrrsi rd, seed, uimm(uimm≠0) 正常访问 | M-mode 执行 csrrsi rd, 0x015, 1 | 正常返回 seed 值，无异常 |
| ROACC-08 | 写值被忽略验证 | M-mode 使用 csrrw 写入非零值到 seed，连续两次读取比较 | 写入值不影响返回的 entropy/OPST（返回由硬件决定） |
| ROACC-09 | csrrw 写非零 rs1 不影响 entropy | M-mode 执行 csrrw rd, 0x015, t0 (t0=0xFFFFFFFF) | 返回正常 seed 值，写入值被忽略 |

---

## Group 4. M-mode 访问控制

**规范依据**：
- `norm:mseccfg_sseed_useed_op_tbl`：M 模式下 seed CSR 始终可用（使用读写指令）
- `norm:seed_exec_mode_control`：seed CSR 受执行模式访问控制

**测试职责**：验证 M-mode 下 seed CSR 的无条件可用性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MACC-01 | M-mode csrrw 始终可用（SSEED=0, USEED=0） | 设 mseccfg.SSEED=0, USEED=0，M-mode csrrw seed | 正常访问，无异常 |
| MACC-02 | M-mode csrrw 始终可用（SSEED=1, USEED=1） | 设 mseccfg.SSEED=1, USEED=1，M-mode csrrw seed | 正常访问，无异常 |
| MACC-03 | M-mode 只读访问仍触发异常 | M-mode 执行 csrrs rd, seed, x0 | illegal-instruction exception (cause=2)（只读访问限制与模式无关） |

---

## Group 5. U-mode 访问控制（USEED）

**规范依据**：
- `norm:mseccfg_useed_U-mode_op`：USEED=0 时 U-mode 访问 seed 引发 illegal-instruction exception；USEED=1 时允许读写访问
- `norm:mseccfg_useed_rdonly0`：未实现 Zkr 或 U-mode 时 USEED 为只读零
- `norm:mseccfg_sseed_useed_presence`：Zkr 扩展向 mseccfg 添加 SSEED/USEED 字段
- `norm:seed_ro_illegal`：只读访问在任何模式下都引发异常

**测试职责**：验证 USEED 字段对 U-mode 访问 seed CSR 的控制效果。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| UACC-01 | USEED=0 U-mode csrrw 访问 seed 触发异常 | 设 mseccfg.USEED=0，U-mode 执行 csrrw rd, seed, x0 | illegal-instruction exception (cause=2) |
| UACC-02 | USEED=1 U-mode csrrw 访问 seed 正常 | 设 mseccfg.USEED=1，U-mode 执行 csrrw rd, seed, x0 | 正常返回 seed 值 |
| UACC-03 | USEED=1 U-mode 只读访问仍触发异常 | 设 mseccfg.USEED=1，U-mode 执行 csrrs rd, seed, x0 | illegal-instruction exception (cause=2) |
| UACC-04 | USEED=1 U-mode csrrsi uimm=0 触发异常 | 设 mseccfg.USEED=1，U-mode 执行 csrrsi rd, seed, 0 | illegal-instruction exception (cause=2) |
| UACC-05 | USEED 字段可写性探测 | M-mode 写 mseccfg.USEED=1 并读回 | 若 Zkr 和 U-mode 已实现，读回 1；否则只读零 |
| UACC-06 | USEED=0 U-mode csrrs(rs1≠x0) 访问触发异常 | 设 mseccfg.USEED=0，U-mode 执行 csrrs rd, seed, t0 | illegal-instruction exception (cause=2)（任何访问均异常） |
| UACC-07 | USEED 不影响 M-mode 访问 | 设 mseccfg.USEED=0，M-mode csrrw seed | 正常访问（M-mode 不受 USEED 控制） |

---

## Group 6. S-mode 访问控制（SSEED）

**规范依据**：
- `norm:mseccfg_sseed_SorHS-mode_op`：SSEED=0 时 S-mode 访问 seed 引发 illegal-instruction exception；SSEED=1 时允许读写访问
- `norm:mseccfg_sseed_rdonly0`：未实现 Zkr 或 S-mode 时 SSEED 为只读零
- `norm:seed_ro_illegal`：只读访问在任何模式下都引发异常

**测试职责**：验证 SSEED 字段对 S-mode 访问 seed CSR 的控制效果。

> 注：HS-mode 及 VS/VU-mode 的访问控制测试已移至 `DOCS/testplan/Hypervisor_cross_test_plan.md` Group 17。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SACC-01 | SSEED=0 S-mode csrrw 访问 seed 触发异常 | 设 mseccfg.SSEED=0，S-mode 执行 csrrw rd, seed, x0 | illegal-instruction exception (cause=2) |
| SACC-02 | SSEED=1 S-mode csrrw 访问 seed 正常 | 设 mseccfg.SSEED=1，S-mode 执行 csrrw rd, seed, x0 | 正常返回 seed 值 |
| SACC-03 | SSEED=1 S-mode 只读访问仍触发异常 | 设 mseccfg.SSEED=1，S-mode 执行 csrrs rd, seed, x0 | illegal-instruction exception (cause=2) |
| SACC-06 | SSEED 字段可写性探测 | M-mode 写 mseccfg.SSEED=1 并读回 | 若 Zkr 和 S-mode 已实现，读回 1；否则只读零 |
| SACC-07 | SSEED=1 S-mode csrrci uimm=0 触发异常 | 设 mseccfg.SSEED=1，S-mode 执行 csrrci rd, seed, 0 | illegal-instruction exception (cause=2) |
| SACC-08 | SSEED=0 S-mode 任何 CSR 指令访问均异常 | 设 mseccfg.SSEED=0，S-mode 执行 csrrw/csrrs/csrrsi 等 | 所有访问均触发 illegal-instruction exception |
| SACC-09 | SSEED 不影响 M-mode 访问 | 设 mseccfg.SSEED=0，M-mode csrrw seed | 正常访问（M-mode 不受 SSEED 控制） |

---

## Group 7. mseccfg SSEED/USEED 字段属性

**规范依据**：
- `norm:mseccfg_sseed_useed_presence`：Zkr 扩展向 mseccfg 添加 SSEED/USEED 字段
- `norm:mseccfg_useed_rdonly0`：未实现 Zkr 或 U-mode 时 USEED 为只读零
- `norm:mseccfg_sseed_rdonly0`：未实现 Zkr 或 S-mode 时 SSEED 为只读零

**测试职责**：验证 mseccfg 中 SSEED/USEED 字段的 WARL 属性和实现检测。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MSECFG-01 | SSEED 字段写入和读回 | M-mode 写 mseccfg.SSEED=1，读回 | 若 Zkr+S-mode 已实现则读回 1；否则读回 0（只读零） |
| MSECFG-02 | USEED 字段写入和读回 | M-mode 写 mseccfg.USEED=1，读回 | 若 Zkr+U-mode 已实现则读回 1；否则读回 0（只读零） |
| MSECFG-03 | SSEED/USEED 初始值检测 | 复位后 M-mode 读 mseccfg | 记录 SSEED/USEED 初始值（实现可能为 0 或 1） |
| MSECFG-04 | SSEED=0 后 S-mode 不可访问 | 写 mseccfg.SSEED=0，S-mode 尝试 csrrw seed | illegal-instruction exception |
| MSECFG-05 | USEED=0 后 U-mode 不可访问 | 写 mseccfg.USEED=0，U-mode 尝试 csrrw seed | illegal-instruction exception |
| MSECFG-06 | SSEED/USEED 不影响 mseccfg 其他字段 | 修改 SSEED/USEED，检查 mseccfg 的 RLB/MMWP/MML 等字段 | 其他字段不受影响 |

---

## Group 8. 安全强度要求

**规范依据**：
- `norm:seed_min_security_256`：seed CSR 实现的安全强度不得低于 256 位
- `norm:seed_disable_if_weak`：若安全级别低于 256 位，接口必须不可用

**测试职责**：验证安全强度约束（架构层面可验证的部分）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SEC-01 | seed CSR 可用时安全强度不低于 256 位 | 若 seed CSR 可访问（M-mode csrrw 不触发异常） | 接口可用即表示满足 ≥256 位安全强度（设计保证，非运行时检测） |
| SEC-02 | DEAD 状态表示不可恢复错误 | 若 OPST=DEAD(11)，持续轮询 | DEAD 状态持续，不恢复到 ES16/WAIT（不可恢复） |

---

## Group 9. 综合场景与边界条件

**规范依据**：
- `norm:seed_ro_illegal`：只读访问异常规则在所有模式下适用
- `norm:seed_exec_mode_control`：执行模式访问控制
- `norm:seed_wipe_on_read`：wipe-on-read 语义
- `norm:mseccfg_sseed_useed_op_tbl`：完整访问控制表

**测试职责**：验证跨模式切换、异常优先级和组合场景。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| MISC-01 | 模式切换后访问控制生效 | M-mode 设 SSEED=1，切入 S-mode 访问 seed（成功），返回 M-mode 设 SSEED=0，再切入 S-mode 访问 | 第二次 S-mode 访问触发 illegal-instruction exception |
| MISC-02 | USEED 和 SSEED 独立控制 | 设 SSEED=1, USEED=0，分别 S-mode 和 U-mode 访问 | S-mode 正常，U-mode 异常 |
| MISC-03 | USEED=1 SSEED=0 独立控制 | 设 SSEED=0, USEED=1，分别 S-mode 和 U-mode 访问 | S-mode 异常，U-mode 正常 |
| MISC-04 | trap handler 中访问 seed | M-mode trap handler 中执行 csrrw seed | 正常访问（M-mode 始终可用） |
| MISC-05 | 只读访问异常优先于模式访问控制 | SSEED=0，S-mode 执行 csrrs rd, seed, x0（同时满足只读异常和模式异常） | illegal-instruction exception (cause=2)（两种条件都导致 illegal，无歧义） |
| MISC-07 | 连续快速轮询 seed | M-mode 紧密循环轮询 seed 100 次 | 每次返回合法 OPST 值；ES16 时 entropy 有效；WAIT 时 entropy=0 |
| MISC-08 | csrrc rd, seed, rs1(rs1≠x0) 正常 | M-mode 执行 csrrc rd, 0x015, t0 (t0≠0) | 正常返回 seed 值（非只读访问） |
| MISC-09 | csrrw rd, seed, x0 写值为零仍正常 | M-mode 执行 csrrw rd, 0x015, x0（写值=0） | 正常访问（csrrw 非只读指令，写值被忽略） |

---
