# CBQRI (Capacity and Bandwidth QoS Register Interface) 测试计划

## 概述

本测试计划覆盖 RISC-V Capacity and Bandwidth Controller QoS Register Interface (CBQRI) 规范 v1.0 中的核心功能点。

本测试计划依据 `SPEC/riscv-cbqri` 中的规范内容编写。CBQRI 规范定义了内存映射寄存器接口，用于共享资源的容量分配、带宽分配及使用监控。

### 本文档覆盖的 SPEC 章节
- QoS Identifiers（RCID/MCID 关联机制、Access Type 编码、srmcfg CSR）
- Capacity-controller QoS Register Interface（cc_capabilities、cc_mon_ctl、cc_mon_ctr_val、cc_alloc_ctl、cc_block_mask、cc_cunits）
- Bandwidth-controller QoS Register Interface（bc_capabilities、bc_mon_ctl、bc_mon_ctr_val、bc_alloc_ctl、bc_bw_alloc）
- General Register Interface Requirements（对齐、字节序、访问宽度）

---

## 覆盖的规范点

本章节列出本文档所有测试组中引用的规范点（norm ID），已去重并按字母顺序排列。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:cbqri_access_width` | The memory-mapped registers can be accessed by using naturally aligned 4-byte or 8-byte memory accesses. | 内存映射寄存器可使用自然对齐的 4 字节或 8 字节访问。 |
| `norm:cbqri_align_8byte` | Each controller that supports CBQRI provides a set of registers that are memory-mapped, starting at an 8-byte aligned physical address. | 每个支持 CBQRI 的控制器提供一组内存映射寄存器，起始于 8 字节对齐的物理地址。 |
| `norm:cbqri_endian` | The controller registers use little-endian byte order (even if all harts are big-endian-only). | 控制器寄存器使用小端字节序（即使所有 hart 仅支持大端）。 |
| `norm:cbqri_unsupported_cap_zero` | When a capability is not supported, the registers and/or fields used to configure and/or control such capabilities are hardwired to 0. | 当能力不被支持时，用于配置/控制该能力的寄存器和/或字段硬连线为 0。 |
| `norm:cbqri_4byte_atomic` | A 4-byte access to a register must be single-copy atomic. | 4 字节寄存器访问必须是单拷贝原子的。 |
| `norm:cbqri_at_unsupported` | For unsupported AT values the resource controller behaves as if AT was 0. | 对于不支持的 AT 值，资源控制器的行为如同 AT 为 0。 |
| `norm:cbqri_srmcfg` | The Ssqosid extension introduces a read/write S/HS-mode register (srmcfg) to configure QoS Identifiers. | Ssqosid 扩展引入 S/HS 模式读写寄存器 srmcfg 用于配置 QoS 标识符。 |
| `norm:cbqri_srmcfg_stateen` | If Smstateen is implemented then bit 55 of mstateen0 controls access to srmcfg from privilege modes less than M. | 若实现 Smstateen，mstateen0 的 bit 55 控制低于 M 模式对 srmcfg 的访问。 |
| `norm:cc_capabilities_ro` | The cc_capabilities register is a read-only register that holds the capacity-controller capabilities. | cc_capabilities 是只读寄存器，保存容量控制器能力。 |
| `norm:cc_reset_alloc` | The capacity controllers at reset must allocate all available capacity to RCID value of 0. | 复位时容量控制器必须将所有可用容量分配给 RCID=0。 |
| `norm:cc_reset_busy` | The reset value is 0 for cc_mon_ctl.BUSY and cc_alloc_ctl.BUSY fields. | cc_mon_ctl.BUSY 和 cc_alloc_ctl.BUSY 复位值为 0。 |
| `norm:cc_mon_ctl_ro_zero` | When the controller does not support capacity usage monitoring the cc_mon_ctl register is read-only zero. | 不支持容量监控时 cc_mon_ctl 为只读零。 |
| `norm:cc_mon_ctl_warl` | The OP, AT, ATV, MCID, and EVT_ID fields of the register are WARL fields. | cc_mon_ctl 的 OP/AT/ATV/MCID/EVT_ID 字段为 WARL。 |
| `norm:cc_mon_busy_behavior` | A write to the cc_mon_ctl sets the read-only BUSY bit to 1. When the BUSY bit reads 0, the operation is complete. The state of the BUSY bit, when not hardwired to 0, shall only change in response to a write to the register. | 写 cc_mon_ctl 设 BUSY=1。BUSY=0 时操作完成。BUSY 仅在写寄存器时改变。 |
| `norm:cc_mon_status_valid` | The STATUS field remains valid until a subsequent write to the cc_mon_ctl register. | STATUS 字段在下次写 cc_mon_ctl 前保持有效。 |
| `norm:cc_mon_config_reset` | When the EVT_ID for a MCID is programmed with a non-zero and legal value by using the CONFIG_EVENT operation, the counter is reset to 0 and starts counting matching events. If EVT_ID is programmed to 0, the counter stops counting. | CONFIG_EVENT 配置非零合法 EVT_ID 时计数器清零并开始计数。EVT_ID=0 时停止计数。 |
| `norm:cc_mon_atv_zero` | A controller that does not support monitoring by access-type identifier can hardwire the ATV and the AT fields to 0. | 不支持按 AT 监控的控制器可将 ATV 和 AT 硬连线为 0。 |
| `norm:cc_mon_ctr_val_ro` | The cc_mon_ctr_val is a read-only register. When the controller does not support capacity usage monitoring, the cc_mon_ctr_val register always reads as zero. | cc_mon_ctr_val 为只读。不支持监控时始终读零。 |
| `norm:cc_ctr_no_neg` | The counter shall not decrement below zero. | 计数器不得递减至零以下。 |
| `norm:cc_alloc_ctl_ro_zero` | If a controller does not support capacity allocation, then this register is read-only zero. If the controller does not support capacity allocation per access type, then the AT field is read-only zero. | 不支持容量分配时 cc_alloc_ctl 只读零。不支持按 AT 分配时 AT 字段只读零。 |
| `norm:cc_alloc_warl` | The OP, RCID and AT fields in this register are WARL. | cc_alloc_ctl 的 OP/RCID/AT 字段为 WARL。 |
| `norm:cc_alloc_busy_behavior` | A write to the cc_alloc_ctl sets the read-only BUSY bit to 1. The state of the BUSY bit, when not hardwired to 0, shall change only in response to a write to the register. The STATUS field remains valid until a subsequent write to the cc_alloc_ctl register. | 写 cc_alloc_ctl 设 BUSY=1。BUSY 仅在写寄存器时改变。STATUS 在下次写前有效。 |
| `norm:cc_block_mask_warl` | The cc_block_mask is a WARL register. If the controller does not support capacity allocation (NCBLKS is 0), then this register is read-only 0. | cc_block_mask 为 WARL。NCBLKS=0 时只读零。 |
| `norm:cc_block_mask_bits` | Bits NCBLKS-1:0 are read-write, and the bits BMW-1:NCBLKS are read-only zero. | NCBLKS-1:0 位可读写，BMW-1:NCBLKS 位只读零。 |
| `norm:cc_cunits_ro_zero` | If the controller does not support capacity allocation (NCBLKS=0), this register shall be read-only zero. If cc_capabilities.CUNITS=0, then this register shall be read-only zero. | NCBLKS=0 或 CUNITS=0 时 cc_cunits 只读零。 |
| `norm:cc_cunits_zero_nolimit` | A value of zero programmed into the cc_cunits register indicates that no limits shall be enforced on capacity unit allocation. | cc_cunits 写入 0 表示不限制容量单元分配。 |
| `norm:bc_capabilities_ro` | The bc_capabilities register is a read-only register that holds the bandwidth-controller capabilities. | bc_capabilities 是只读寄存器，保存带宽控制器能力。 |
| `norm:bc_reset_alloc` | The bandwidth controllers at reset must allocate all available bandwidth to RCID value of 0. | 复位时带宽控制器必须将所有可用带宽分配给 RCID=0。 |
| `norm:bc_reset_busy` | The reset value is 0 for bc_mon_ctl.BUSY and bc_alloc_ctl.BUSY fields. | bc_mon_ctl.BUSY 和 bc_alloc_ctl.BUSY 复位值为 0。 |
| `norm:bc_mon_ctl_ro_zero` | When the controller does not support bandwidth usage monitoring, the bc_mon_ctl register is read-only zero. | 不支持带宽监控时 bc_mon_ctl 只读零。 |
| `norm:bc_mon_ctl_warl` | The OP, AT, MCID, and EVT_ID fields of the register are WARL fields. | bc_mon_ctl 的 OP/AT/MCID/EVT_ID 字段为 WARL。 |
| `norm:bc_mon_busy_behavior` | A write to the bc_mon_ctl register sets the read-only BUSY bit to 1. The state of the BUSY bit, when not hardwired to 0, shall only change in response to a write to the register. The STATUS field remains valid until a subsequent write to the bc_mon_ctl register. | 写 bc_mon_ctl 设 BUSY=1。BUSY 仅在写寄存器时改变。STATUS 在下次写前有效。 |
| `norm:bc_mon_config_reset` | When the EVT_ID for a MCID is programmed with a non-zero and legal value, the counter is reset to 0 and starts counting matching events. If EVT_ID is configured as 0, the counter stops counting. | 配置非零合法 EVT_ID 时计数器清零并开始计数。EVT_ID=0 时停止计数。 |
| `norm:bc_mon_ctr_val_ro` | The bc_mon_ctr_val is a read-only register. When the controller does not support bandwidth usage monitoring, the bc_mon_ctr_val register always reads as zero. | bc_mon_ctr_val 为只读。不支持监控时始终读零。 |
| `norm:bc_alloc_ctl_ro_zero` | If a controller does not support bandwidth allocation, then the register is read-only zero. If the controller does not support bandwidth allocation per access-type, then the AT field is read-only zero. | 不支持带宽分配时 bc_alloc_ctl 只读零。不支持按 AT 分配时 AT 字段只读零。 |
| `norm:bc_alloc_busy_behavior` | A write to the bc_alloc_ctl sets the read-only BUSY bit to 1. The state of the BUSY bit, when not hardwired to 0, shall only change in response to a write to the register. The STATUS field remains valid until a subsequent write to the bc_alloc_ctl register. | 写 bc_alloc_ctl 设 BUSY=1。BUSY 仅在写寄存器时改变。STATUS 在下次写前有效。 |
| `norm:bc_bw_alloc_ro_zero` | If a controller does not support bandwidth allocation, then the bc_bw_alloc register is read-only zero. | 不支持带宽分配时 bc_bw_alloc 只读零。 |
| `norm:bc_bw_alloc_warl` | The Rbwb, Mweight, sharedAT, and useShared are all WARL fields. | bc_bw_alloc 的 Rbwb/Mweight/sharedAT/useShared 均为 WARL。 |
| `norm:bc_rbwb_range` | The value in Rbwb must be at least 1 and must not exceed MRBWB value. Otherwise, the CONFIG_LIMIT operation fails with STATUS=5. | Rbwb 必须 >=1 且 <=MRBWB，否则 CONFIG_LIMIT 失败 STATUS=5。 |
| `norm:bc_rbwb_sum` | The sum of Rbwb allocated across all RCIDs must not exceed MRBWB value, or the CONFIG_LIMIT operation fails with STATUS=5. | 所有 RCID 的 Rbwb 之和不得超过 MRBWB，否则 CONFIG_LIMIT 失败 STATUS=5。 |
| `norm:bc_useshared_ro_zero` | The useShared and sharedAT fields are read-only zero if the bandwidth controller does not support bandwidth allocation per access-type identifier. | 不支持按 AT 分配时 useShared 和 sharedAT 只读零。 |

---

## Group 1. 通用寄存器接口要求

**规范依据**：
- `norm:cbqri_align_8byte`：寄存器起始于 8 字节对齐物理地址
- `norm:cbqri_access_width`：使用自然对齐的 4 字节或 8 字节访问
- `norm:cbqri_4byte_atomic`：4 字节访问必须单拷贝原子
- `norm:cbqri_endian`：寄存器使用小端字节序
- `norm:cbqri_unsupported_cap_zero`：不支持的能力对应寄存器/字段硬连线为 0

**测试职责**：验证 CBQRI 内存映射寄存器接口的通用访问规则。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| GEN-01 | 寄存器基址 8 字节对齐 | 读取设备树/ACPI 获取 CBQRI 控制器基址，验证对齐 | 基址 & 0x7 == 0 |
| GEN-02 | 4 字节自然对齐访问 | 对 CBQRI 寄存器执行 4 字节对齐的 load/store | 正常完成，无异常 |
| GEN-03 | 8 字节自然对齐访问 | 对 CBQRI 寄存器执行 8 字节对齐的 load/store | 正常完成，无异常 |
| GEN-04 | 小端字节序验证 | 向寄存器写入已知值（如 0x12345678），按字节读取内存 | 低地址存放低字节（0x78），高地址存放高字节（0x12） |
| GEN-05 | 不支持的能力寄存器只读零 | 读取不支持能力的控制器对应寄存器 | 读回全零 |

---

## Group 2. QoS 标识符与 srmcfg CSR

**规范依据**：
- `norm:cbqri_srmcfg`：srmcfg 为 S/HS-mode 读写寄存器，配置 RCID 和 MCID
- `norm:cbqri_srmcfg_stateen`：Smstateen 实现时 mstateen0 bit 55 控制低特权级访问
- `norm:cbqri_at_unsupported`：不支持的 AT 值行为如同 AT=0

**测试职责**：验证 srmcfg CSR 的访问控制及 AT 编码行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| QOSID-01 | srmcfg 基本读写 | HS-mode 写 srmcfg 设置 RCID 和 MCID，读回 | 读回值与写入一致（WARL 约束内） |
| QOSID-02 | S-mode 访问 srmcfg | S-mode 读写 srmcfg | 正常访问 |
| QOSID-03 | U-mode 访问 srmcfg 触发异常 | U-mode 尝试访问 srmcfg | illegal-instruction exception (cause=2) |
| QOSID-04 | mstateen0 bit 55 控制 S-mode 访问 | 设 mstateen0[55]=0，S-mode 访问 srmcfg | illegal-instruction exception (cause=2) |
| QOSID-05 | mstateen0 bit 55=1 允许 S-mode 访问 | 设 mstateen0[55]=1，S-mode 访问 srmcfg | 正常访问 |
| QOSID-06 | M-mode 访问 srmcfg 不受 mstateen0 限制 | 设 mstateen0[55]=0，M-mode 访问 srmcfg | 正常访问 |
| QOSID-07 | AT 编码验证 | 读取控制器支持的 AT 值，配置不支持的 AT 值 | 不支持的 AT 值行为如同 AT=0 |

---

## Group 3. 容量控制器能力寄存器 (cc_capabilities)

**规范依据**：
- `norm:cc_capabilities_ro`：cc_capabilities 为只读寄存器
- `norm:cc_reset_alloc`：复位时所有容量分配给 RCID=0
- `norm:cc_reset_busy`：BUSY 字段复位值为 0

**测试职责**：验证容量控制器能力寄存器的只读属性和复位行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| CCAP-01 | cc_capabilities 只读验证 | 尝试写 cc_capabilities 寄存器，读回 | 写入被忽略，读回值不变 |
| CCAP-02 | VER 字段格式验证 | 读取 cc_capabilities.VER | 高 4 位为主版本，低 4 位为次版本（如 0x10 表示 v1.0） |
| CCAP-03 | NCBLKS 字段有效性 | 读取 NCBLKS 字段 | 值为控制器支持的可分配容量块总数 |
| CCAP-04 | FRCID/CUNITS/RPFX 字段一致性 | 读取 FRCID、CUNITS、RPFX 位 | 若 RPFX=0 则 P=0；各字段值与控制器实际能力一致 |
| CCAP-05 | 复位后 BUSY 为 0 | 复位后读取 cc_mon_ctl.BUSY 和 cc_alloc_ctl.BUSY | 两者均为 0 |
| CCAP-06 | 复位后 RCID=0 拥有全部容量 | 复位后对 RCID=0 执行 READ_LIMIT | 返回的 cc_block_mask 包含所有容量块 |

---

## Group 4. 容量使用监控 (cc_mon_ctl / cc_mon_ctr_val)

**规范依据**：
- `norm:cc_mon_ctl_ro_zero`：不支持监控时 cc_mon_ctl 只读零
- `norm:cc_mon_ctl_warl`：OP/AT/ATV/MCID/EVT_ID 为 WARL
- `norm:cc_mon_busy_behavior`：写设 BUSY=1，BUSY 仅在写时改变
- `norm:cc_mon_status_valid`：STATUS 在下次写前有效
- `norm:cc_mon_config_reset`：CONFIG_EVENT 非零 EVT_ID 清零计数器并开始计数；EVT_ID=0 停止计数
- `norm:cc_mon_atv_zero`：不支持按 AT 监控时 ATV/AT 硬连线为 0
- `norm:cc_mon_ctr_val_ro`：cc_mon_ctr_val 只读；不支持监控时读零
- `norm:cc_ctr_no_neg`：计数器不递减至零以下

**测试职责**：验证容量使用监控的配置、操作和计数器行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| CCMON-01 | 不支持监控时 cc_mon_ctl 只读零 | 若控制器不支持监控，读 cc_mon_ctl | 全零 |
| CCMON-02 | 不支持监控时 cc_mon_ctr_val 只读零 | 若控制器不支持监控，读 cc_mon_ctr_val | 全零 |
| CCMON-03 | CONFIG_EVENT 操作成功 | 写 cc_mon_ctl：OP=1, MCID=合法值, EVT_ID=1(Occupancy), ATV=0 | BUSY 先置 1 后清 0，STATUS=1（成功） |
| CCMON-04 | CONFIG_EVENT 清零计数器 | 先配置 EVT_ID=1 并产生事件使计数器非零，再重新 CONFIG_EVENT EVT_ID=1 | 计数器被清零，重新开始计数 |
| CCMON-05 | CONFIG_EVENT EVT_ID=0 停止计数 | 配置 EVT_ID=0 | 计数器停止计数 |
| CCMON-06 | READ_COUNTER 操作 | 配置 EVT_ID=1 后产生容量事件，执行 OP=2 (READ_COUNTER) | cc_mon_ctr_val.CTR 反映当前占用量，INV=0 |
| CCMON-07 | 无效 OP 返回 STATUS=2 | 写 cc_mon_ctl：OP=0（保留值） | STATUS=2（无效操作） |
| CCMON-08 | 无效 MCID 返回 STATUS=3 | 写 cc_mon_ctl：OP=1, MCID=超出支持范围的值 | STATUS=3（无效 MCID） |
| CCMON-09 | 无效 EVT_ID 返回 STATUS=4 | 写 cc_mon_ctl：OP=1, EVT_ID=保留值（如 2-127） | STATUS=4（无效 EVT_ID） |
| CCMON-10 | 无效 AT 返回 STATUS=5 | 若支持按 AT 监控，写 ATV=1, AT=保留值（2-5） | STATUS=5（无效 AT） |
| CCMON-11 | BUSY 期间写入行为 | 写 cc_mon_ctl 触发操作，BUSY=1 时再次写入 | 行为 UNSPECIFIED（验证不崩溃） |
| CCMON-12 | BUSY 仅在写时改变 | 写 cc_mon_ctl 后轮询 BUSY | BUSY 从 1 变 0，无其他自发变化 |
| CCMON-13 | STATUS 在下次写前保持有效 | 执行操作获取 STATUS，多次读取 STATUS | STATUS 值保持不变直到下次写 cc_mon_ctl |
| CCMON-14 | 计数器不递减至零以下 | 配置 EVT_ID=1(Occupancy)，MCID 重分配后产生 deallocation | 计数器保持 0，不出现负值（INV 可能置位） |
| CCMON-15 | ATV=0 时计数所有 AT | 配置 ATV=0, EVT_ID=1，产生不同 AT 的请求 | 计数器统计所有 AT 的事件 |
| CCMON-16 | ATV=1 时仅计数指定 AT | 配置 ATV=1, AT=0(Data), EVT_ID=1，产生 AT=0 和 AT=1 的请求 | 仅 AT=0 的事件被计数 |
| CCMON-17 | 不支持按 AT 监控时 ATV/AT 只读零 | 若控制器不支持按 AT 监控，写 ATV=1, AT=1 后读回 | ATV=0, AT=0 |
| CCMON-18 | cc_mon_ctr_val INV 字段 | 读取 cc_mon_ctr_val.INV | INV=0 表示有效，INV=1 表示暂时无效 |
| CCMON-19 | BUSY/STATUS 写入被忽略 | 尝试写 cc_mon_ctl 的 BUSY 和 STATUS 字段 | 写入被忽略（只读字段） |

---

## Group 5. 容量分配 (cc_alloc_ctl / cc_block_mask / cc_cunits)

**规范依据**：
- `norm:cc_alloc_ctl_ro_zero`：不支持分配时 cc_alloc_ctl 只读零；不支持按 AT 分配时 AT 只读零
- `norm:cc_alloc_warl`：OP/RCID/AT 为 WARL
- `norm:cc_alloc_busy_behavior`：写设 BUSY=1，仅在写时改变；STATUS 在下次写前有效
- `norm:cc_block_mask_warl`：cc_block_mask 为 WARL；NCBLKS=0 时只读零
- `norm:cc_block_mask_bits`：NCBLKS-1:0 可读写，BMW-1:NCBLKS 只读零
- `norm:cc_cunits_ro_zero`：NCBLKS=0 或 CUNITS=0 时 cc_cunits 只读零
- `norm:cc_cunits_zero_nolimit`：cc_cunits=0 表示不限制

**测试职责**：验证容量分配控制的操作、block mask 约束和 cunits 限制。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| CCALLOC-01 | 不支持分配时 cc_alloc_ctl 只读零 | 若控制器不支持容量分配，读 cc_alloc_ctl | 全零 |
| CCALLOC-02 | 不支持分配时 cc_block_mask 只读零 | 若 NCBLKS=0，读 cc_block_mask | 全零 |
| CCALLOC-03 | 不支持分配时 cc_cunits 只读零 | 若 NCBLKS=0 或 CUNITS=0，读 cc_cunits | 全零 |
| CCALLOC-04 | CONFIG_LIMIT 操作成功 | 写 cc_block_mask 选择块，写 cc_alloc_ctl：OP=1, RCID=1, AT=0 | BUSY 清 0 后 STATUS=1（成功） |
| CCALLOC-05 | READ_LIMIT 读回配置 | 先 CONFIG_LIMIT 配置 RCID=1，再 OP=2 (READ_LIMIT), RCID=1 | cc_block_mask 和 cc_cunits 反映之前配置 |
| CCALLOC-06 | FLUSH_RCID 操作（FRCID=1） | 若 FRCID=1，写 cc_alloc_ctl：OP=3, RCID=已配置值 | STATUS=1（成功），已分配容量块配置不变 |
| CCALLOC-07 | FLUSH_RCID 不支持（FRCID=0） | 若 FRCID=0，写 OP=3 | STATUS=2（无效/不支持操作） |
| CCALLOC-08 | 无效 OP 返回 STATUS=2 | 写 cc_alloc_ctl：OP=0（保留） | STATUS=2 |
| CCALLOC-09 | 无效 RCID 返回 STATUS=3 | 写 cc_alloc_ctl：OP=1, RCID=超出支持范围 | STATUS=3 |
| CCALLOC-10 | 无效 AT 返回 STATUS=4 | 若支持按 AT，写 AT=保留值 | STATUS=4 |
| CCALLOC-11 | 无效 block mask 返回 STATUS=5 | 写 cc_block_mask 全零（某些实现要求至少一个块），执行 CONFIG_LIMIT | STATUS=5 |
| CCALLOC-12 | cc_block_mask 高位只读零 | 写 cc_block_mask 全 1，读回 | bits NCBLKS-1:0 可写，bits BMW-1:NCBLKS 为 0 |
| CCALLOC-13 | cc_block_mask BMW 计算验证 | 根据 NCBLKS 计算 BMW=floor((NCBLKS+63)/64)*64，验证寄存器宽度 | 寄存器宽度 = BMW/8 字节 |
| CCALLOC-14 | cc_cunits=0 表示无限制 | CONFIG_LIMIT 时 cc_cunits=0，READ_LIMIT 读回 | cc_cunits=0，表示不限制容量单元 |
| CCALLOC-15 | cc_cunits 非零限制 | 若 CUNITS=1，CONFIG_LIMIT 设 cc_cunits=N，READ_LIMIT 读回 | cc_cunits=N |
| CCALLOC-16 | 不支持按 AT 分配时 AT 只读零 | 若不支持按 AT，写 AT=1 后读回 | AT=0 |
| CCALLOC-17 | BUSY 期间写 cc_block_mask/cc_cunits | BUSY=1 时写 cc_block_mask 或 cc_cunits | 行为 UNSPECIFIED（验证不崩溃） |
| CCALLOC-18 | BUSY/STATUS 写入被忽略 | 尝试写 cc_alloc_ctl 的 BUSY 和 STATUS 字段 | 写入被忽略 |
| CCALLOC-19 | 重叠 block mask 配置 | 为 RCID=1 和 RCID=2 配置重叠的 cc_block_mask | 两者均配置成功（允许重叠） |

---

## Group 6. 带宽控制器能力寄存器 (bc_capabilities)

**规范依据**：
- `norm:bc_capabilities_ro`：bc_capabilities 为只读寄存器
- `norm:bc_reset_alloc`：复位时所有带宽分配给 RCID=0
- `norm:bc_reset_busy`：BUSY 字段复位值为 0

**测试职责**：验证带宽控制器能力寄存器的只读属性和复位行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| BCAP-01 | bc_capabilities 只读验证 | 尝试写 bc_capabilities 寄存器，读回 | 写入被忽略，读回值不变 |
| BCAP-02 | VER 字段格式验证 | 读取 bc_capabilities.VER | 高 4 位主版本，低 4 位次版本 |
| BCAP-03 | NBWBLKS 字段有效性 | 读取 NBWBLKS 字段 | 值为控制器可用带宽块总数 |
| BCAP-04 | MRBWB 字段有效性 | 读取 MRBWB 字段 | MRBWB <= NBWBLKS（最大可预留带宽块数） |
| BCAP-05 | RPFX/P 字段一致性 | 读取 RPFX 和 P 字段 | 若 RPFX=0 则 P=0；P 范围 0-12 |
| BCAP-06 | 复位后 BUSY 为 0 | 复位后读取 bc_mon_ctl.BUSY 和 bc_alloc_ctl.BUSY | 两者均为 0 |
| BCAP-07 | 复位后 RCID=0 拥有全部带宽 | 复位后对 RCID=0, AT=0 执行 READ_LIMIT | 返回的 Rbwb 为全部可用带宽 |

---

## Group 7. 带宽使用监控 (bc_mon_ctl / bc_mon_ctr_val)

**规范依据**：
- `norm:bc_mon_ctl_ro_zero`：不支持监控时 bc_mon_ctl 只读零
- `norm:bc_mon_ctl_warl`：OP/AT/MCID/EVT_ID 为 WARL
- `norm:bc_mon_busy_behavior`：写设 BUSY=1，仅在写时改变；STATUS 在下次写前有效
- `norm:bc_mon_config_reset`：CONFIG_EVENT 非零 EVT_ID 清零并开始计数；EVT_ID=0 停止
- `norm:bc_mon_ctr_val_ro`：bc_mon_ctr_val 只读；不支持监控时读零

**测试职责**：验证带宽使用监控的配置、操作和计数器行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| BCMON-01 | 不支持监控时 bc_mon_ctl 只读零 | 若控制器不支持带宽监控，读 bc_mon_ctl | 全零 |
| BCMON-02 | 不支持监控时 bc_mon_ctr_val 只读零 | 若控制器不支持带宽监控，读 bc_mon_ctr_val | 全零 |
| BCMON-03 | CONFIG_EVENT 操作成功 | 写 bc_mon_ctl：OP=1, MCID=合法值, EVT_ID=1(Total RW byte count) | BUSY 清 0 后 STATUS=1 |
| BCMON-04 | CONFIG_EVENT 清零计数器 | 先配置 EVT_ID=1 并产生流量，再重新 CONFIG_EVENT | 计数器清零，重新开始计数 |
| BCMON-05 | CONFIG_EVENT EVT_ID=0 停止计数 | 配置 EVT_ID=0 | 计数器停止计数 |
| BCMON-06 | READ_COUNTER 操作 | 配置 EVT_ID=1 后产生内存读写流量，执行 OP=2 | bc_mon_ctr_val.CTR 反映传输字节数 |
| BCMON-07 | EVT_ID=2 仅计读字节 | 配置 EVT_ID=2(Total Read byte count)，产生读写流量 | 仅统计读请求字节数 |
| BCMON-08 | EVT_ID=3 仅计写字节 | 配置 EVT_ID=3(Total Write byte count)，产生读写流量 | 仅统计写请求字节数 |
| BCMON-09 | 无效 OP 返回 STATUS=2 | 写 bc_mon_ctl：OP=0（保留） | STATUS=2 |
| BCMON-10 | 无效 MCID 返回 STATUS=3 | 写 OP=1, MCID=超出范围 | STATUS=3 |
| BCMON-11 | 无效 EVT_ID 返回 STATUS=4 | 写 OP=1, EVT_ID=保留值（4-127） | STATUS=4 |
| BCMON-12 | 无效 AT 返回 STATUS=5 | 若支持按 AT，写 ATV=1, AT=保留值 | STATUS=5 |
| BCMON-13 | BUSY 仅在写时改变 | 写 bc_mon_ctl 后轮询 BUSY | BUSY 从 1 变 0，无自发变化 |
| BCMON-14 | STATUS 在下次写前保持有效 | 执行操作后多次读 STATUS | STATUS 保持不变直到下次写 |
| BCMON-15 | OVF 溢出标志 | 产生大量流量使计数器溢出 | bc_mon_ctr_val.OVF=1 |
| BCMON-16 | 重新 CONFIG_EVENT 清除 OVF | OVF=1 后重新执行 CONFIG_EVENT | 计数器清零，OVF 清除 |
| BCMON-17 | BUSY/STATUS 写入被忽略 | 尝试写 BUSY 和 STATUS 字段 | 写入被忽略 |
| BCMON-18 | ATV=0 时计数所有 AT | 配置 ATV=0，产生不同 AT 流量 | 计数器统计所有 AT |

---

## Group 8. 带宽分配 (bc_alloc_ctl / bc_bw_alloc)

**规范依据**：
- `norm:bc_alloc_ctl_ro_zero`：不支持分配时 bc_alloc_ctl 只读零；不支持按 AT 时 AT 只读零
- `norm:bc_alloc_busy_behavior`：写设 BUSY=1，仅在写时改变；STATUS 在下次写前有效
- `norm:bc_bw_alloc_ro_zero`：不支持分配时 bc_bw_alloc 只读零
- `norm:bc_bw_alloc_warl`：Rbwb/Mweight/sharedAT/useShared 为 WARL
- `norm:bc_rbwb_range`：Rbwb >= 1 且 <= MRBWB，否则 STATUS=5
- `norm:bc_rbwb_sum`：所有 RCID 的 Rbwb 之和 <= MRBWB，否则 STATUS=5
- `norm:bc_useshared_ro_zero`：不支持按 AT 时 useShared/sharedAT 只读零

**测试职责**：验证带宽分配控制的操作、范围约束和共享机制。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| BCALLOC-01 | 不支持分配时 bc_alloc_ctl 只读零 | 若控制器不支持带宽分配，读 bc_alloc_ctl | 全零 |
| BCALLOC-02 | 不支持分配时 bc_bw_alloc 只读零 | 若控制器不支持带宽分配，读 bc_bw_alloc | 全零 |
| BCALLOC-03 | CONFIG_LIMIT 操作成功 | 写 bc_bw_alloc.Rbwb=合法值，写 bc_alloc_ctl：OP=1, RCID=1, AT=0 | BUSY 清 0 后 STATUS=1 |
| BCALLOC-04 | READ_LIMIT 读回配置 | 先 CONFIG_LIMIT，再 OP=2 (READ_LIMIT), RCID=1 | bc_bw_alloc 反映之前配置的 Rbwb/Mweight |
| BCALLOC-05 | Rbwb=0 失败 STATUS=5 | 写 bc_bw_alloc.Rbwb=0，执行 CONFIG_LIMIT | STATUS=5（无效带宽块） |
| BCALLOC-06 | Rbwb>MRBWB 失败 STATUS=5 | 写 bc_bw_alloc.Rbwb=MRBWB+1，执行 CONFIG_LIMIT | STATUS=5 |
| BCALLOC-07 | Rbwb 总和超 MRBWB 失败 STATUS=5 | 为多个 RCID 配置 Rbwb 使总和 > MRBWB | 最后一次 CONFIG_LIMIT 返回 STATUS=5 |
| BCALLOC-08 | 无效 OP 返回 STATUS=2 | 写 bc_alloc_ctl：OP=0（保留） | STATUS=2 |
| BCALLOC-09 | 无效 RCID 返回 STATUS=3 | 写 OP=1, RCID=超出范围 | STATUS=3 |
| BCALLOC-10 | 无效 AT 返回 STATUS=4 | 若支持按 AT，写 AT=保留值 | STATUS=4 |
| BCALLOC-11 | Mweight=0 为硬限制 | 配置 Mweight=0，验证 RCID 不使用非预留带宽 | 该 RCID 不使用超出 Rbwb 的带宽 |
| BCALLOC-12 | Mweight 非零比例分配 | 为两个 RCID 配置不同 Mweight，产生竞争流量 | 非预留带宽按 Mweight 比例分配 |
| BCALLOC-13 | useShared=1 共享 AT 配置 | 配置 AT=0 为独立分配，AT=1 设 useShared=1, sharedAT=0 | AT=1 共享 AT=0 的带宽配置 |
| BCALLOC-14 | useShared=1 时 Rbwb/Mweight 被忽略 | useShared=1 时写 Rbwb/Mweight 为非零值 | Rbwb/Mweight 被忽略，使用 sharedAT 的配置 |
| BCALLOC-15 | 不支持按 AT 时 useShared/sharedAT 只读零 | 若不支持按 AT 分配，写 useShared=1, sharedAT=1 后读回 | useShared=0, sharedAT=0 |
| BCALLOC-16 | 不支持按 AT 时 AT 只读零 | 若不支持按 AT 分配，写 AT=1 后读回 | AT=0 |
| BCALLOC-17 | BUSY/STATUS 写入被忽略 | 尝试写 bc_alloc_ctl 的 BUSY 和 STATUS 字段 | 写入被忽略 |
| BCALLOC-18 | BUSY 仅在写时改变 | 写 bc_alloc_ctl 后轮询 BUSY | BUSY 从 1 变 0，无自发变化 |
| BCALLOC-19 | STATUS 在下次写前保持有效 | 执行操作后多次读 STATUS | STATUS 保持不变直到下次写 |

---

## Group 9. RCID 前缀模式 (RPFX) 与有效 MCID

**规范依据**：
- cc_capabilities.RPFX 和 P 字段定义 RCID 前缀模式
- bc_capabilities.RPFX 和 P 字段定义 RCID 前缀模式
- 有效 MCID 计算公式：Effective-MCID = (RCID << P) | (MCID & ((1 << P) - 1))
- RPFX=0 时 P=0，有效 MCID 等于请求中的 MCID

**测试职责**：验证 RCID 前缀模式下有效 MCID 的计算和监控计数器选择。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| RPFX-01 | RPFX=0 时有效 MCID=MCID | 若 RPFX=0，配置 MCID=M 的监控事件，产生 MCID=M 的请求 | 计数器 M 被触发 |
| RPFX-02 | RPFX=1 时有效 MCID 计算 | 若 RPFX=1, P=2, RCID=3, MCID=5，计算有效 MCID=(3<<2)|(5&3)=13 | 计数器 13 被触发 |
| RPFX-03 | P 值范围验证 | 读取 P 字段 | P 范围 0-12 |
| RPFX-04 | RPFX=0 时 P=0 验证 | 若 RPFX=0，读取 P 字段 | P=0 |
| RPFX-05 | 软件使用有效 MCID 操作计数器 | 以计算的有效 MCID 作为 READ_COUNTER 的 MCID 操作数 | 正确读取对应计数器值 |

---
