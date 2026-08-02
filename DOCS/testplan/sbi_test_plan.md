# SBI (Supervisor Binary Interface) 测试计划

## 概述

本测试计划覆盖 RISC-V SBI 规范中的核心功能点，验证 SBI 实现（SEE）对 supervisor-mode 软件提供的标准接口是否符合规范。

本测试计划依据 `SPEC/riscv-sbi` 中的规范内容编写。

**注意**：本测试计划不包含依赖 Hypervisor (H) 扩展的内容，包括：
- Nested Acceleration Extension (NACL, EID #0x4E41434C)
- RFENCE 扩展中的 HFENCE 相关函数（FID #3-#6）

### 本文档覆盖的 SPEC 章节
- Binary Encoding（调用约定、错误码、hart mask 参数、共享内存参数）
- Base Extension (EID #0x10)
- Timer Extension (EID #0x54494D45)
- IPI Extension (EID #0x735049)
- RFENCE Extension (EID #0x52464E43)（仅 FID #0-#2，不含 HFENCE 函数）
- Hart State Management Extension (EID #0x48534D)
- System Reset Extension (EID #0x53525354)
- Debug Console Extension (EID #0x4442434E)
- System Suspend Extension (EID #0x53555350)
- Performance Monitoring Unit Extension (EID #0x504D55)
- CPPC Extension (EID #0x43505043)
- Steal-time Accounting Extension (EID #0x535441)
- Firmware Features Extension (EID #0x46574654)
- Supervisor Software Events Extension (EID #0x535345)
- Legacy Extensions (EIDs #0x00-#0x08)

### 不在本文档范围
- Nested Acceleration Extension → 依赖 Hypervisor
- RFENCE HFENCE 函数 (FID #3-#6) → 依赖 Hypervisor
- Debug Triggers Extension → 待补充
- Message Proxy Extension (MPXY) → 待补充

---

## 覆盖的规范点

本章节列出本文档所有测试组中引用的规范点，已去重并按字母顺序排列。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:sbi_ecall_convention` | An ECALL is used as the control transfer instruction between the supervisor and the SEE. a7 encodes the EID, a6 encodes the FID, a0-a5 contain arguments. | ECALL 用于 supervisor 和 SEE 之间的控制转移。a7 编码 EID，a6 编码 FID，a0-a5 传递参数。 |
| `norm:sbi_register_preserve` | All registers except a0 & a1 must be preserved across an SBI call by the callee. | 除 a0 和 a1 外，所有寄存器必须在 SBI 调用中被被调用者保留。 |
| `norm:sbi_return_convention` | SBI functions must return a pair of values in a0 and a1, with a0 returning an error code. | SBI 函数必须在 a0 和 a1 中返回一对值，a0 返回错误码。 |
| `norm:sbi_unsupported_eid_fid` | An ECALL with an unsupported SBI extension ID (EID) or an unsupported SBI function ID (FID) must return the error code SBI_ERR_NOT_SUPPORTED. | 不支持的 EID 或 FID 的 ECALL 必须返回 SBI_ERR_NOT_SUPPORTED。 |
| `norm:sbi_error_a1_unspecified` | If an SBI function call returns an error code other than SBI_SUCCESS, the value returned in a1 is unspecified unless explicitly defined for that SBI function. | 若 SBI 调用返回非 SBI_SUCCESS 错误码，a1 中的值未指定（除非该函数明确定义）。 |
| `norm:sbi_hart_mask` | hart_mask_base can be set to -1 to indicate that hart_mask shall be ignored and all available harts must be considered. | hart_mask_base 设为 -1 时表示忽略 hart_mask，考虑所有可用 hart。 |
| `norm:sbi_hart_mask_error` | Any SBI function taking hart mask arguments may return SBI_ERR_INVALID_PARAM if at least one hartid is not valid. | 使用 hart mask 参数的 SBI 函数在 hartid 无效时可返回 SBI_ERR_INVALID_PARAM。 |
| `norm:sbi_shmem_accessible` | The SBI implementation MUST check that the specified physical memory range is composed of accessible physical addresses and return SBI_ERR_INVALID_ADDRESS when any address in the range is not accessible. | SBI 实现必须检查指定物理内存范围由可访问物理地址组成，否则返回 SBI_ERR_INVALID_ADDRESS。 |
| `norm:sbi_shmem_allowed` | The SBI implementation MUST check that the supervisor-mode software is allowed to access the specified physical memory range with the access type requested. | SBI 实现必须检查 supervisor-mode 软件是否被允许以请求的访问类型访问指定物理内存范围。 |
| `norm:sbi_shmem_pma` | The SBI implementation MUST access the specified physical memory range using the PMA attributes. | SBI 实现必须使用 PMA 属性访问指定物理内存范围。 |
| `norm:sbi_shmem_endian` | The data in the shared memory MUST follow little-endian byte ordering. | 共享内存中的数据必须遵循小端字节序。 |
| `norm:sbi_base_must_succeed` | All functions in the base extension must be supported by all SBI implementations, so there are no error returns defined. | Base 扩展的所有函数必须被所有 SBI 实现支持，无错误返回。 |
| `norm:sbi_base_spec_version_encoding` | The minor number is encoded in the low 24 bits, with the major number encoded in the next 7 bits. Bit 31 must be 0. When XLEN > 32, bits 32 and above must also be 0. | 版本号低 24 位为 minor，接下来 7 位为 major。Bit 31 必须为 0。XLEN>32 时 bit 32 及以上也必须为 0。 |
| `norm:sbi_base_probe` | Returns 0 if the given SBI extension ID (EID) is not available, or 1 if it is available unless defined as any other non-zero value by the implementation. | 若 EID 不可用返回 0，可用返回 1（除非实现定义为其他非零值）。 |
| `norm:sbi_time_clear_pending` | This function must clear the pending timer interrupt bit when stime_value is set to some time in the future, regardless of whether timer interrupts are masked or not. | 当 stime_value 设为未来时间时，无论定时器中断是否被屏蔽，都必须清除 pending 定时器中断位。 |
| `norm:sbi_time_always_success` | This function always returns SBI_SUCCESS in sbiret.error. | 此函数始终在 sbiret.error 中返回 SBI_SUCCESS。 |
| `norm:sbi_ipi_manifest` | Interprocessor interrupts manifest at the receiving harts as the supervisor software interrupts. | 处理器间中断在接收 hart 上表现为 supervisor 软件中断。 |
| `norm:sbi_rfence_range_all` | The remote fence operation applies to the entire address space if either: start_addr and size are both 0, or size is equal to 2^XLEN-1. | 若 start_addr 和 size 均为 0，或 size 等于 2^XLEN-1，则远程 fence 操作应用于整个地址空间。 |
| `norm:sbi_hsm_start_regs` | Hart start: satp=0, sstatus.SIE=0, a0=hartid, a1=opaque. All other registers remain in an undefined state. | Hart start 时：satp=0, sstatus.SIE=0, a0=hartid, a1=opaque。其他寄存器未定义。 |
| `norm:sbi_hsm_stop_no_return` | This call is not expected to return under normal conditions. The sbi_hart_stop() must be called with supervisor-mode interrupts disabled. | 正常情况下此调用不会返回。必须在 supervisor-mode 中断禁用时调用。 |
| `norm:sbi_hsm_suspend_retentive` | Resuming from a retentive suspend state is straight forward and the supervisor-mode software will see SBI suspend call return without any failures. The resume_addr parameter is unused during retentive suspend. | 从保持型挂起恢复时 SBI 调用正常返回。resume_addr 在保持型挂起时未使用。 |
| `norm:sbi_hsm_suspend_nonretentive_regs` | Upon resuming from non-retentive suspend state: satp=0, sstatus.SIE=0, a0=hartid, a1=opaque. All other registers remain in an undefined state. | 从非保持型挂起恢复时：satp=0, sstatus.SIE=0, a0=hartid, a1=opaque。其他寄存器未定义。 |
| `norm:sbi_srst_synchronous` | This is a synchronous call and does not return if it succeeds. | 这是同步调用，成功时不返回。 |
| `norm:sbi_dbcn_write_nonblocking` | This is a non-blocking SBI call and it may do partial/no writes if the debug console is not able to accept more bytes. | 这是非阻塞调用，若控制台无法接受更多字节可能部分写入或不写入。 |
| `norm:sbi_dbcn_write_byte_blocking` | This is a blocking SBI call and it will only return after writing the specified byte to the debug console. | 这是阻塞调用，仅在写入指定字节后返回。 |
| `norm:sbi_dbcn_read_nonblocking` | This is a non-blocking SBI call and it will not write anything into the output memory if there are no bytes to be read in the debug console. | 这是非阻塞调用，若无可读字节则不写入输出内存。 |
| `norm:sbi_susp_no_return_on_success` | A return from a sbi_system_suspend() call implies an error. A successful suspend and wake up results in the hart resuming from the STOPPED state. | sbi_system_suspend() 返回意味着错误。成功挂起和唤醒后 hart 从 STOPPED 状态恢复。 |
| `norm:sbi_susp_resume_regs` | System suspend resume: satp=0, sstatus.SIE=0, a0=hartid, a1=opaque. All other registers remain in an undefined state. | 系统挂起恢复时：satp=0, sstatus.SIE=0, a0=hartid, a1=opaque。其他寄存器未定义。 |
| `norm:sbi_pmu_num_counters_success` | Returns the number of counters (both hardware and firmware) in sbiret.value and always returns SBI_SUCCESS in sbiret.error. | 返回硬件和固件计数器总数，始终返回 SBI_SUCCESS。 |
| `norm:sbi_cppc_probe_width` | If the register is implemented, sbiret.value will contain the register width. If not implemented, sbiret.value will be set to 0. | 若寄存器已实现，sbiret.value 包含寄存器宽度；未实现则为 0。 |
| `norm:sbi_cppc_read_hi_rv64` | This function always returns zero in sbiret.value when supervisor mode XLEN is 64 or higher. | 当 supervisor mode XLEN 为 64 或更高时，此函数始终在 sbiret.value 中返回零。 |
| `norm:sbi_sta_shmem_zero` | The SBI implementation MUST zero the first 64 bytes of the shared memory before returning from the SBI call. | SBI 实现必须在返回前将共享内存前 64 字节清零。 |
| `norm:sbi_sta_shmem_align` | shmem_phys_lo MUST be 64-byte aligned. The size of the shared memory must be at least 64 bytes. | shmem_phys_lo 必须 64 字节对齐。共享内存大小至少 64 字节。 |
| `norm:sbi_sta_preempted_zero` | A zero value MUST be written before the virtual hart starts to run again. | 虚拟 hart 重新运行前必须写入零值。 |
| `norm:sbi_fwft_lock` | If LOCK flag is provided, once set, the feature value can no longer be modified until hart reset (local) or system reset (global). | 若提供 LOCK 标志，一旦设置，特性值不可修改直到 hart 复位（local）或系统复位（global）。 |
| `norm:sbi_legacy_convention` | Legacy extensions: a6 register is ignored, nothing is returned in a1, all registers except a0 must be preserved. | Legacy 扩展：忽略 a6，a1 不返回，除 a0 外所有寄存器必须保留。 |
| `norm:sbi_legacy_fault_redirect` | The page and access faults taken by the SBI implementation while accessing memory on behalf of the supervisor are redirected back to the supervisor with sepc CSR pointing to the faulting ECALL instruction. | SBI 实现代替 supervisor 访问内存时的页/访问故障被重定向回 supervisor，sepc 指向故障 ECALL 指令。 |

---

## Group 1. Binary Encoding 与调用约定

**规范依据**：
- `norm:sbi_ecall_convention`：ECALL 调用约定，a7=EID, a6=FID, a0-a5=参数
- `norm:sbi_register_preserve`：除 a0/a1 外所有寄存器必须保留
- `norm:sbi_return_convention`：a0=error, a1=value
- `norm:sbi_unsupported_eid_fid`：不支持的 EID/FID 返回 SBI_ERR_NOT_SUPPORTED
- `norm:sbi_error_a1_unspecified`：错误时 a1 未指定

**测试职责**：验证 SBI 调用约定的基本正确性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| ENC-01 | 不支持的 EID 返回 NOT_SUPPORTED | ECALL 使用无效 EID（如 0x7FFFFFFF） | a0 = SBI_ERR_NOT_SUPPORTED (-2) |
| ENC-02 | 不支持的 FID 返回 NOT_SUPPORTED | 对已支持的 EID 使用无效 FID | a0 = SBI_ERR_NOT_SUPPORTED (-2) |
| ENC-03 | 寄存器保留验证 | SBI 调用前设置 a2-a7, t0-t6, s0-s11 为已知值，调用后检查 | 除 a0/a1 外所有寄存器保持不变 |
| ENC-04 | 返回值在 a0/a1 中 | 调用 sbi_get_spec_version | a0=0 (SBI_SUCCESS), a1=版本号 |
| ENC-05 | 错误时 a1 未指定 | 调用不支持的函数，检查 a1 | a0 为错误码，a1 值不作断言 |

---

## Group 2. Hart Mask 参数

**规范依据**：
- `norm:sbi_hart_mask`：hart_mask_base=-1 时忽略 hart_mask，考虑所有可用 hart
- `norm:sbi_hart_mask_error`：无效 hartid 返回 SBI_ERR_INVALID_PARAM

**测试职责**：验证 hart mask 参数的通用行为。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HMASK-01 | hart_mask_base=-1 广播所有 hart | 使用 sbi_send_ipi，hart_mask=任意, hart_mask_base=-1 | SBI_SUCCESS，所有可用 hart 收到 IPI |
| HMASK-02 | 无效 hartid 返回 INVALID_PARAM | 使用 sbi_send_ipi，hart_mask 包含不存在的 hartid | SBI_ERR_INVALID_PARAM (-3) |
| HMASK-03 | 单 hart mask 正确 | sbi_send_ipi 仅目标为当前 hart (hart_mask=1, hart_mask_base=当前hartid) | SBI_SUCCESS，当前 hart 收到 IPI |

---

## Group 3. 共享内存参数

**规范依据**：
- `norm:sbi_shmem_accessible`：物理地址必须可访问，否则返回 SBI_ERR_INVALID_ADDRESS
- `norm:sbi_shmem_allowed`：supervisor-mode 必须被允许访问
- `norm:sbi_shmem_pma`：使用 PMA 属性访问
- `norm:sbi_shmem_endian`：小端字节序

**测试职责**：验证共享内存参数的通用约束。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SHMEM-01 | 无效物理地址返回 INVALID_ADDRESS | 使用 sbi_debug_console_write 传入不存在的物理地址 | SBI_ERR_INVALID_ADDRESS (-5) |
| SHMEM-02 | 有效共享内存读写正常 | 使用 sbi_debug_console_write 传入有效物理地址 | SBI_SUCCESS |

---

## Group 4. Base Extension (EID #0x10)

**规范依据**：
- `norm:sbi_base_must_succeed`：所有 Base 函数必须成功
- `norm:sbi_base_spec_version_encoding`：版本号编码规则
- `norm:sbi_base_probe`：probe 返回值语义

**测试职责**：验证 Base 扩展所有函数的正确性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| BASE-01 | sbi_get_spec_version 返回值合法 | 调用 FID #0 | a0=0, a1 的 bit31=0, bits[30:24]=major, bits[23:0]=minor |
| BASE-02 | sbi_get_spec_version XLEN>32 高位为零 | RV64 调用 FID #0 | a1 的 bits[63:32] 全为零 |
| BASE-03 | sbi_get_impl_id 返回合法 ID | 调用 FID #1 | a0=0, a1 为已知实现 ID 之一 |
| BASE-04 | sbi_get_impl_version 返回 | 调用 FID #2 | a0=0 |
| BASE-05 | sbi_probe_extension 已实现扩展返回非零 | 调用 FID #3, extension_id=0x10 (Base) | a0=0, a1≠0 |
| BASE-06 | sbi_probe_extension 未实现扩展返回零 | 调用 FID #3, extension_id=无效 EID | a0=0, a1=0 |
| BASE-07 | sbi_get_mvendorid 返回合法值 | 调用 FID #4 | a0=0, a1 为 mvendorid 合法值 |
| BASE-08 | sbi_get_marchid 返回合法值 | 调用 FID #5 | a0=0, a1 为 marchid 合法值 |
| BASE-09 | sbi_get_mimpid 返回合法值 | 调用 FID #6 | a0=0, a1 为 mimpid 合法值 |

---

## Group 5. Timer Extension (EID #0x54494D45)

**规范依据**：
- `norm:sbi_time_clear_pending`：设未来时间时必须清除 pending timer interrupt
- `norm:sbi_time_always_success`：始终返回 SBI_SUCCESS

**测试职责**：验证定时器设置功能。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| TIME-01 | sbi_set_timer 返回 SBI_SUCCESS | 调用 sbi_set_timer(当前 time + 大值) | a0=0 (SBI_SUCCESS) |
| TIME-02 | sbi_set_timer 清除 pending 中断 | 先使 timer 中断 pending，再调用 sbi_set_timer(远未来值) | sip.STIP 被清除 |
| TIME-03 | sbi_set_timer 触发定时器中断 | 设 sie.STIE=1, sstatus.SIE=1，调用 sbi_set_timer(当前 time + 小值) | 收到 timer interrupt (cause=5) |
| TIME-04 | sbi_set_timer(-1) 清除中断不触发 | 调用 sbi_set_timer((uint64_t)-1) | 不触发定时器中断，pending 清除 |
| TIME-05 | 中断屏蔽时仍清除 pending | 清 sie.STIE，先使 STIP pending，调用 sbi_set_timer(远未来值) | STIP 被清除（即使中断被屏蔽） |

---

## Group 6. IPI Extension (EID #0x735049)

**规范依据**：
- `norm:sbi_ipi_manifest`：IPI 在接收 hart 上表现为 supervisor software interrupt

**测试职责**：验证处理器间中断发送功能。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| IPI-01 | sbi_send_ipi 到自身 | hart_mask 指向当前 hart | SBI_SUCCESS，当前 hart 收到 supervisor software interrupt |
| IPI-02 | IPI 表现为 SSIP | 发送 IPI 到自身，检查 sip.SSIP | sip.SSIP=1 |
| IPI-03 | IPI 中断递送 | 使能 sie.SSIE 和 sstatus.SIE，发送 IPI 到自身 | 收到 software interrupt (cause=1) |
| IPI-04 | 无效 hart_mask 返回错误 | 使用包含无效 hartid 的 hart_mask | SBI_ERR_INVALID_PARAM |

---

## Group 7. RFENCE Extension (EID #0x52464E43, FID #0-#2)

**规范依据**：
- `norm:sbi_rfence_range_all`：start_addr=0 且 size=0 或 size=2^XLEN-1 时应用于整个地址空间

**测试职责**：验证远程 fence 功能（不含 HFENCE 函数）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| RFNC-01 | sbi_remote_fence_i 到自身 | hart_mask 指向当前 hart | SBI_SUCCESS |
| RFNC-02 | sbi_remote_fence_i 无效 hart | 使用无效 hartid | SBI_ERR_INVALID_PARAM |
| RFNC-03 | sbi_remote_sfence_vma 全地址空间 | start_addr=0, size=0, hart_mask 指向当前 hart | SBI_SUCCESS |
| RFNC-04 | sbi_remote_sfence_vma 指定范围 | start_addr=有效地址, size=4096 | SBI_SUCCESS |
| RFNC-05 | sbi_remote_sfence_vma size=2^XLEN-1 | start_addr=任意, size=2^XLEN-1 | SBI_SUCCESS（全地址空间） |
| RFNC-06 | sbi_remote_sfence_vma_asid 指定 ASID | 传入有效 ASID | SBI_SUCCESS |
| RFNC-07 | sbi_remote_sfence_vma_asid 无效 ASID | 传入超出范围的 ASID | SBI_ERR_INVALID_PARAM |
| RFNC-08 | sbi_remote_sfence_vma 无效地址 | start_addr 为无效地址 | SBI_ERR_INVALID_ADDRESS |

---

## Group 8. HSM Extension (EID #0x48534D)

**规范依据**：
- `norm:sbi_hsm_start_regs`：hart start 时寄存器初始状态
- `norm:sbi_hsm_stop_no_return`：hart stop 正常不返回，需禁用中断
- `norm:sbi_hsm_suspend_retentive`：保持型挂起正常返回
- `norm:sbi_hsm_suspend_nonretentive_regs`：非保持型挂起恢复时寄存器状态

**测试职责**：验证 Hart 状态管理功能。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| HSM-01 | sbi_hart_get_status 当前 hart STARTED | 查询当前 hart 状态 | sbiret.value=0 (STARTED) |
| HSM-02 | sbi_hart_get_status 无效 hartid | 查询不存在的 hartid | SBI_ERR_INVALID_PARAM |
| HSM-03 | sbi_hart_start 已启动 hart | 对当前已 STARTED 的 hart 调用 start | SBI_ERR_ALREADY_AVAILABLE (-6) |
| HSM-04 | sbi_hart_start 无效 hartid | 使用无效 hartid | SBI_ERR_INVALID_PARAM |
| HSM-05 | sbi_hart_start 无效地址 | 使用无效 start_addr | SBI_ERR_INVALID_ADDRESS |
| HSM-06 | sbi_hart_start 成功启动停止的 hart | 先 stop 另一 hart，再 start 它，验证寄存器状态 | 目标 hart 在 start_addr 处执行，satp=0, SIE=0, a0=hartid, a1=opaque |
| HSM-07 | sbi_hart_suspend 保持型挂起 | suspend_type=0x00000000 (default retentive) | SBI_SUCCESS，调用正常返回 |
| HSM-08 | sbi_hart_suspend 保留类型 | suspend_type=0x00000002 (reserved) | SBI_ERR_INVALID_PARAM |
| HSM-09 | sbi_hart_stop 禁用中断检查 | 在 sstatus.SIE=1 时调用 sbi_hart_stop | SBI_ERR_FAILED（规范要求禁用中断） |

---

## Group 9. System Reset Extension (EID #0x53525354)

**规范依据**：
- `norm:sbi_srst_synchronous`：成功时不返回

**测试职责**：验证系统复位功能的参数校验。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SRST-01 | 保留 reset_type 返回错误 | reset_type=0x00000003 (reserved) | SBI_ERR_INVALID_PARAM |
| SRST-02 | 保留 reset_reason 返回错误 | reset_type=0 (shutdown), reset_reason=0x00000002 (reserved) | SBI_ERR_INVALID_PARAM |
| SRST-03 | 合法参数组合验证 | reset_type=0, reset_reason=0（不实际执行 shutdown，仅验证参数接受） | 不返回 SBI_ERR_INVALID_PARAM |

---

## Group 10. Debug Console Extension (EID #0x4442434E)

**规范依据**：
- `norm:sbi_dbcn_write_nonblocking`：Console Write 非阻塞
- `norm:sbi_dbcn_write_byte_blocking`：Console Write Byte 阻塞
- `norm:sbi_dbcn_read_nonblocking`：Console Read 非阻塞

**测试职责**：验证调试控制台功能。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| DBCN-01 | sbi_debug_console_write_byte 成功 | 写入一个 ASCII 字符 | SBI_SUCCESS, sbiret.uvalue=0 |
| DBCN-02 | sbi_debug_console_write 有效内存 | 传入有效物理地址和 num_bytes | SBI_SUCCESS, sbiret.uvalue >= 0 |
| DBCN-03 | sbi_debug_console_write 无效地址 | 传入无效物理地址 | SBI_ERR_INVALID_PARAM |
| DBCN-04 | sbi_debug_console_read 无数据 | 无输入数据时调用 read | SBI_SUCCESS, sbiret.uvalue=0（非阻塞） |
| DBCN-05 | sbi_debug_console_read 无效地址 | 传入无效输出地址 | SBI_ERR_INVALID_PARAM |
| DBCN-06 | sbi_debug_console_write num_bytes=0 | 传入 num_bytes=0 | SBI_SUCCESS, sbiret.uvalue=0 |

---

## Group 11. System Suspend Extension (EID #0x53555350)

**规范依据**：
- `norm:sbi_susp_no_return_on_success`：返回意味着错误
- `norm:sbi_susp_resume_regs`：恢复时寄存器状态

**测试职责**：验证系统挂起功能的参数校验。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SUSP-01 | 保留 sleep_type 返回错误 | sleep_type=0x00000001 (reserved) | SBI_ERR_INVALID_PARAM |
| SUSP-02 | 无效 resume_addr 返回错误 | sleep_type=0, resume_addr=无效地址 | SBI_ERR_INVALID_ADDRESS |
| SUSP-03 | SUSPEND_TO_RAM 入口条件不满足 | 有其他 hart 处于 STARTED 状态时调用 sleep_type=0 | SBI_ERR_DENIED |

---

## Group 12. PMU Extension (EID #0x504D55)

**规范依据**：
- `norm:sbi_pmu_num_counters_success`：始终返回 SBI_SUCCESS

**测试职责**：验证性能监控单元接口。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| PMU-01 | sbi_pmu_num_counters 返回计数器数 | 调用 FID #0 | SBI_SUCCESS, sbiret.value > 0 |
| PMU-02 | sbi_pmu_counter_get_info 有效 counter | 调用 FID #1, counter_idx=0 | SBI_SUCCESS, 返回 counter_info |
| PMU-03 | sbi_pmu_counter_get_info 无效 counter | counter_idx 超出范围 | SBI_ERR_INVALID_PARAM |
| PMU-04 | sbi_pmu_counter_config_matching 硬件事件 | event_idx=0x00001 (CPU_CYCLES), config_flags=0 | SBI_SUCCESS, 返回 counter_idx |
| PMU-05 | sbi_pmu_counter_config_matching 不支持事件 | event_idx=无效值 | SBI_ERR_NOT_SUPPORTED |
| PMU-06 | sbi_pmu_counter_config_matching 保留 flag | config_flags 含保留位 | SBI_ERR_INVALID_PARAM |
| PMU-07 | sbi_pmu_counter_start 正常启动 | 配置后启动计数器 | SBI_SUCCESS |
| PMU-08 | sbi_pmu_counter_start 已启动计数器 | 对已启动的计数器再次 start | SBI_ERR_ALREADY_STARTED |
| PMU-09 | sbi_pmu_counter_stop 正常停止 | 停止已启动的计数器 | SBI_SUCCESS |
| PMU-10 | sbi_pmu_counter_stop 已停止计数器 | 对已停止的计数器再次 stop | SBI_ERR_ALREADY_STOPPED |
| PMU-11 | sbi_pmu_counter_fw_read 固件计数器 | 读取固件类型计数器 | SBI_SUCCESS |
| PMU-12 | sbi_pmu_counter_fw_read 硬件计数器 | 对硬件计数器调用 fw_read | SBI_ERR_INVALID_PARAM |
| PMU-13 | counter_start 设初始值 | start_flags=SET_INIT_VALUE, initial_value=100 | SBI_SUCCESS, 计数器从 100 开始 |
| PMU-14 | counter_stop RESET flag | stop_flags=SBI_PMU_STOP_FLAG_RESET | SBI_SUCCESS, 计数器事件映射被重置 |

---

## Group 13. CPPC Extension (EID #0x43505043)

**规范依据**：
- `norm:sbi_cppc_probe_width`：probe 返回寄存器宽度或 0
- `norm:sbi_cppc_read_hi_rv64`：RV64 时 read_hi 始终返回 0

**测试职责**：验证 CPPC 寄存器访问接口。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| CPPC-01 | sbi_cppc_probe 已实现寄存器 | probe HighestPerformance (reg_id=0) | SBI_SUCCESS, sbiret.value=32（宽度） |
| CPPC-02 | sbi_cppc_probe 保留 reg_id | probe reg_id=0x00000015 (reserved) | SBI_ERR_INVALID_PARAM |
| CPPC-03 | sbi_cppc_read 只读寄存器 | read HighestPerformance (reg_id=0) | SBI_SUCCESS |
| CPPC-04 | sbi_cppc_read 未实现寄存器 | read 未实现的 reg_id | SBI_ERR_NOT_SUPPORTED |
| CPPC-05 | sbi_cppc_write 只读寄存器返回 DENIED | write HighestPerformance (reg_id=0) | SBI_ERR_DENIED |
| CPPC-06 | sbi_cppc_write 读写寄存器 | write DesiredPerformance (reg_id=5) | SBI_SUCCESS |
| CPPC-07 | sbi_cppc_read_hi RV64 返回零 | RV64 下调用 read_hi | SBI_SUCCESS, sbiret.value=0 |
| CPPC-08 | sbi_cppc_read 保留 reg_id | read reg_id=0x80000001 (reserved) | SBI_ERR_INVALID_PARAM |

---

## Group 14. Steal-time Accounting Extension (EID #0x535441)

**规范依据**：
- `norm:sbi_sta_shmem_zero`：设置时前 64 字节清零
- `norm:sbi_sta_shmem_align`：64 字节对齐
- `norm:sbi_sta_preempted_zero`：hart 运行前 preempted 字段必须为零

**测试职责**：验证 steal-time 共享内存设置。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| STA-01 | sbi_steal_time_set_shmem 有效地址 | 传入 64 字节对齐有效物理地址, flags=0 | SBI_SUCCESS, 共享内存前 64 字节被清零 |
| STA-02 | sbi_steal_time_set_shmem 非对齐地址 | shmem_phys_lo 非 64 字节对齐 | SBI_ERR_INVALID_PARAM |
| STA-03 | sbi_steal_time_set_shmem flags 非零 | flags≠0 | SBI_ERR_INVALID_PARAM |
| STA-04 | sbi_steal_time_set_shmem 无效地址 | 传入不可访问的物理地址 | SBI_ERR_INVALID_ADDRESS |
| STA-05 | sbi_steal_time_set_shmem 清除 | shmem_phys_lo=-1, shmem_phys_hi=-1 | SBI_SUCCESS, 停止报告 |
| STA-06 | 共享内存 preempted 字段初始为零 | 设置 shmem 后检查 offset 16 处 | preempted=0 |

---

## Group 15. Firmware Features Extension (EID #0x46574654)

**规范依据**：
- `norm:sbi_fwft_lock`：LOCK 标志锁定特性值

**测试职责**：验证固件特性管理接口。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| FWFT-01 | sbi_fwft_get 有效特性 | get MISALIGNED_EXC_DELEG (feature=0) | SBI_SUCCESS, sbiret.value 为 0 或 1 |
| FWFT-02 | sbi_fwft_get 保留特性 | get feature=0x00000006 (reserved) | SBI_ERR_DENIED |
| FWFT-03 | sbi_fwft_set 有效值 | set DOUBLE_TRAP (feature=3), value=1, flags=0 | SBI_SUCCESS |
| FWFT-04 | sbi_fwft_set 无效值 | set DOUBLE_TRAP, value=2 (invalid) | SBI_ERR_INVALID_PARAM |
| FWFT-05 | sbi_fwft_set 保留 flags | flags 含保留位 | SBI_ERR_INVALID_PARAM |
| FWFT-06 | sbi_fwft_set LOCK 后不可修改 | set feature, flags=LOCK; 再次 set 不同 value | 第二次返回 SBI_ERR_DENIED_LOCKED |
| FWFT-07 | sbi_fwft_get LOCK 后仍可读取 | LOCK 后 get | SBI_SUCCESS, 值为 LOCK 时设定的值 |
| FWFT-08 | sbi_fwft_set 不支持的平台特性 | set platform-specific 未实现特性 | SBI_ERR_NOT_SUPPORTED 或 SBI_ERR_DENIED |

---

## Group 16. SSE Extension (EID #0x535345)

**规范依据**：
- SSE 扩展定义软件事件状态机：UNUSED → REGISTERED → ENABLED → RUNNING

**测试职责**：验证 Supervisor Software Events 接口基本功能。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SSE-01 | sbi_sse_register 有效事件 | 注册 local event (event_id=0x00000000) | SBI_SUCCESS |
| SSE-02 | sbi_sse_register 无效事件 | 注册 reserved event_id | SBI_ERR_INVALID_PARAM |
| SSE-03 | sbi_sse_enable 未注册事件 | 对未注册的事件调用 enable | SBI_ERR_INVALID_STATE 或 SBI_ERR_DENIED |
| SSE-04 | sbi_sse_enable 已注册事件 | 注册后 enable | SBI_SUCCESS |
| SSE-05 | sbi_sse_disable 已启用事件 | enable 后 disable | SBI_SUCCESS |
| SSE-06 | sbi_sse_get_attrs 有效事件 | 获取已注册事件的属性 | SBI_SUCCESS |
| SSE-07 | sbi_sse_inject 软件注入事件 | 注入 software injected local event (0xffff0000) | SBI_SUCCESS（若已注册并启用） |

---

## Group 17. Legacy Extensions (EIDs #0x00-#0x08)

**规范依据**：
- `norm:sbi_legacy_convention`：Legacy 调用约定（a6 忽略，a1 不返回）
- `norm:sbi_legacy_fault_redirect`：故障重定向到 supervisor，sepc 指向 ECALL

**测试职责**：验证 Legacy 扩展的基本兼容性。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| LEG-01 | Legacy set_timer (EID #0x00) | 使用 legacy 调用约定设置定时器 | 返回 0（成功） |
| LEG-02 | Legacy console_putchar (EID #0x01) | 使用 legacy 调用输出字符 | 返回 0（成功） |
| LEG-03 | Legacy console_getchar (EID #0x02) | 使用 legacy 调用读取字符 | 返回字节或 -1 |
| LEG-04 | Legacy send_ipi (EID #0x04) | 使用 legacy 调用发送 IPI | 返回 0（成功） |
| LEG-05 | Legacy remote_fence_i (EID #0x05) | 使用 legacy 调用远程 fence.i | 返回 0（成功） |
| LEG-06 | Legacy remote_sfence_vma (EID #0x06) | 使用 legacy 调用远程 sfence.vma | 返回 0（成功） |
| LEG-07 | Legacy 寄存器保留 | 调用前设置 a1-a7, t0-t6, s0-s11，调用后检查 | 除 a0 外所有寄存器保持不变 |
| LEG-08 | Legacy 故障重定向 | 使用 legacy sbi_send_ipi 传入无效虚拟地址 | 触发 page/access fault，sepc 指向 ECALL 指令 |

---
