# T-SBI (Test Supervisor Binary Interface) 详解

## 1. T-SBI 的定义与背景

### 1.1 什么是 T-SBI

T-SBI (Test Supervisor Binary Interface) 是 RISC-V 架构兼容性测试 (ACT) 框架中定义的一套**轻量级测试专用接口**，用于 S-mode/U-mode 测试程序向执行环境（通常是 M-mode trap handler）请求服务。

T-SBI 定义在 ACT 框架的以下文件中：
- 规范描述：`riscv-arch-test/docs/ctp/src/abstraction.adoc`
- 认证要求：`riscv-arch-test/docs/crd/src/rva23_crd.adoc`
- 代码实现：`riscv-arch-test/tests/env/rvtest_trap_handler.h`

### 1.2 为什么需要 T-SBI（而非直接使用官方 SBI）

| 问题 | 官方 SBI (OpenSBI) | T-SBI |
|------|-------------------|-------|
| 代码体积 | ~1MB 固件二进制 | 几百行汇编（嵌入 trap handler） |
| RV32E 兼容 | 使用 a6/a7 寄存器（RV32E 无此寄存器） | 仅使用 a0/a1 |
| M-mode 假设 | 要求标准 M-mode | 支持自定义 M-mode（Custom M-mode） |
| 加载方式 | 独立固件文件 | 与测试 ELF 一体编译 |
| 目的 | 生产环境 OS 服务 | 测试环境 CSR 访问和特权切换 |

**核心设计动机**（来自 CRD 文档）：

> RVA23S64 profile 不要求标准 M-mode。认证测试不测试 RISC-V SBI，而是提供一个更简单的 T-SBI 来访问执行环境。

---

## 2. T-SBI 与官方 SBI SPEC 的关系与区别

### 2.1 对比总结

| 维度 | 官方 SBI (riscv-sbi SPEC) | T-SBI (ACT 框架) |
|------|--------------------------|------------------|
| **定位** | 生产环境 OS↔固件接口 | 测试环境 test↔trap handler 接口 |
| **规范状态** | Ratified 正式规范 | 测试框架内部定义 |
| **调用触发** | ECALL | ECALL |
| **EID/FID 寻址** | a7=EID, a6=FID（二级寻址） | a0=操作码（一级寻址） |
| **参数传递** | a0-a5（6 个参数） | a0-a1（最多 2 个） |
| **返回值** | a0=error, a1=value | a0=结果 |
| **寄存器保留** | 除 a0/a1 外全部保留 | 除 a0/a1 外全部保留 |
| **扩展性** | 模块化扩展（Base/Timer/IPI/HSM/PMU...） | 固定操作集（GOTO/CSR/ECALL_TEST） |
| **错误码** | 14 种标准错误码 | 仅 -1 (TSBI_RESERVED_RET) |
| **实现者** | OpenSBI/RustSBI/KVM 等独立固件 | 测试框架内嵌 trap handler |
| **版本协商** | sbi_get_spec_version() | 无 |
| **扩展探测** | sbi_probe_extension() | 无 |

### 2.2 调用约定对比

**官方 SBI**：
```asm
li   a7, 0x54494D45    # EID = TIME extension
li   a6, 0             # FID = set_timer
li   a0, <stime_lo>    # 参数：定时器值低位
li   a1, <stime_hi>    # 参数：定时器值高位（RV32）
ecall
# a0 = error, a1 = value
```

**T-SBI**：
```asm
li   a0, 0x3005a073    # 操作码 = csrrs x0, mstatus, a1 的机器码
li   a1, (1<<20)       # 参数：mstatus.TVM bit
ecall
# a0 = 结果（CSR_READ 时返回 CSR 值）
```

### 2.3 设计哲学差异

**官方 SBI**：
- 面向**操作系统**：提供定时器、IPI、电源管理、性能监控等完整服务
- 面向**可移植性**：OS 不关心底层是 OpenSBI 还是 KVM
- 面向**安全性**：SBI 实现验证所有请求，可以拒绝

**T-SBI**：
- 面向**测试可控性**：测试需要直接操控 M-mode CSR 来构造测试场景
- 面向**最小假设**：不假设标准 M-mode 存在
- 面向**透明性**：CSR 操作直接执行，无策略判断

### 2.4 共同点

1. 都使用 ECALL 指令触发
2. 都遵循"除返回值寄存器外全部保留"的约定
3. 都是从低特权级向高特权级请求服务的机制
4. 都体现了 RISC-V "执行环境" 的架构思想

---

## 3. T-SBI 接口定义

### 3.1 调用约定

```
┌──────────────────────────────────────────────────────────┐
│                  T-SBI 调用约定                            │
├──────────┬───────────────────────────────────────────────┤
│ 寄存器    │ 用途                                         │
├──────────┼───────────────────────────────────────────────┤
│ a0       │ 输入：操作码；输出：返回值                      │
│ a1       │ 输入：可选参数（如 CSR 写入值）                 │
│ 其他      │ 全部保留（handler 使用 T1-T6 作为内部临时）     │
└──────────┴───────────────────────────────────────────────┘
```

**关键约束**：
- handler 永远不使用 a0/a1 作为内部临时寄存器
- handler 的临时寄存器是 T1..T6 (x6..x9, x14, x15)，通过 save area 保存/恢复
- 未识别的操作码返回 -1 (TSBI_RESERVED_RET)

### 3.2 操作码定义

#### 特权级切换操作

| 操作码 | 名称 | 值 | 说明 |
|--------|------|-----|------|
| TSBI_GOTO_MMODE | 切换到 M-mode | 0x00000001 | 从任意模式切换到 M-mode |
| TSBI_GOTO_SMODE | 切换到 S/HS-mode | 0x00000002 | 从任意模式切换到 S-mode |
| TSBI_GOTO_UMODE | 切换到 U-mode | 0x00000003 | 从任意模式切换到 U-mode |
| TSBI_GOTO_VSMODE | 切换到 VS-mode | 0x00000004 | 需要 H 扩展 |
| TSBI_GOTO_VUMODE | 切换到 VU-mode | 0x00000005 | 需要 H 扩展 |

#### ECALL 测试操作

| 操作码 | 名称 | 值 | 说明 |
|--------|------|-----|------|
| TSBI_ECALL_TEST | 测试 ECALL 路径 | 0x00000073 | 返回 xEPC 到 a0（证明 trap handler 被进入） |

#### CSR 访问操作

CSR 访问不使用固定操作码，而是**直接传递 CSR 指令的机器码**作为操作码：

| 操作类型 | a0 编码格式 | 说明 |
|----------|-------------|------|
| CSRR (读) | `0x<CSR>02573` | 读 CSR 到 a0 |
| CSRW (写) | `0x<CSR>59073` | 写 a1 到 CSR |
| CSRS (置位) | `0x<CSR>5a073` | 将 a1 中的位设置到 CSR |
| CSRC (清位) | `0x<CSR>5b073` | 将 a1 中的位从 CSR 清除 |

**编码解析**：
```
a0 = CSR 指令的完整 32-bit 机器码
     [31:20] = CSR 地址 (12-bit)
     [19:15] = rs1 寄存器编号（通常为 a1=x11=01011 或 x0=00000）
     [14:12] = funct3 (001=CSRRW, 010=CSRRS, 011=CSRRC)
     [11:7]  = rd 寄存器编号（必须为 a0=x10=01010 或 x0=00000）
     [6:0]   = 1110011 (SYSTEM opcode = 0x73)
```

#### 内存访问操作（规划中）

| 操作码 | 名称 | 说明 |
|--------|------|------|
| 0x0005a503 | TSBI_LW | LW a0, 0(a1) - M-mode 权限加载字 |
| 0x0045a503 | TSBI_LW+4 | LW a0, 4(a1) - 加载高 32 位 |
| 0x0005b503 | TSBI_LD | LD a0, 0(a1) - M-mode 权限加载双字（RV64） |
| 0x00a5a023 | TSBI_SW | SW a0, 0(a1) - M-mode 权限存储字 |
| 0x00a5a223 | TSBI_SW+4 | SW a0, 4(a1) - 存储高 32 位 |
| 0x00a5b023 | TSBI_SD | SD a0, 0(a1) - M-mode 权限存储双字（RV64） |

### 3.3 Handler 分派逻辑

```
ecall 进入 trap handler 后：

1. if (a0-1) < 5       → GOTO_xMODE      (a0 在 [1..5] 范围)
2. if a0 == 0x73       → ECALL_TEST      (精确匹配 ecall 编码)
3. if a0[6:0]==0x73 && a0[14:12]!=0
                       → CSR_ACCESS      (SYSTEM opcode + funct3 非零)
4. otherwise           → RESERVED        (返回 -1)
```

### 3.4 分派层级

```
M-mode handler:  直接处理所有操作
S-mode handler:  本地处理 ECALL_TEST、GOTO_S/U、S-mode CSR_ACCESS
                 转发 GOTO_M/VS/VU 和 M-mode CSR_ACCESS 到 M-mode（通过 ecall）
```

---

## 4. T-SBI 使用示例

### 4.1 汇编宏接口

```asm
# 切换到 M-mode
RVTEST_TSBI_GOTO_MMODE

# 切换到 U-mode
RVTEST_TSBI_GOTO_UMODE

# 测试 ecall 路径（a0 返回 ecall 指令地址）
RVTEST_TSBI_ECALL_TEST

# 读取 mstatus 到 a0
# 编码: csrrs a0, mstatus, x0 = 0x30002573
RVTEST_TSBI_CSR_ACCESS 0x30002573

# 设置 mstatus.TVM (bit 20)
# 编码: csrrs x0, mstatus, a1 = 0x3005A073
li   a1, (1 << 20)
RVTEST_TSBI_CSR_ACCESS 0x3005A073, a1

# 清除 mstatus.TVM
# 编码: csrc x0, mstatus, a1 = 0x3005B073
li   a1, (1 << 20)
RVTEST_TSBI_CSR_ACCESS 0x3005B073, a1
```

### 4.2 CSR 指令编码计算

以 `csrrs a0, mstatus, x0`（读 mstatus）为例：

```
CSR 地址: mstatus = 0x300
rs1:      x0     = 00000 (读操作不需要 rs1)
funct3:   CSRRS  = 010
rd:       a0     = 01010 (x10)
opcode:   SYSTEM = 1110011

拼接: 0011_0000_0000 | 00000 | 010 | 01010 | 1110011
     = 0x300         | 0x00  | 0x2 | 0x5   | 0x73
     = 0x30002573
```

以 `csrrs x0, mstatus, a1`（置位 mstatus）为例：

```
CSR 地址: mstatus = 0x300
rs1:      a1     = 01011 (x11)
funct3:   CSRRS  = 010
rd:       x0     = 00000 (不保存旧值)
opcode:   SYSTEM = 1110011

拼接: 0x300 | 01011 | 010 | 00000 | 1110011
     = 0x3005A073
```

### 4.3 典型测试场景

```asm
# 场景：测试 S-mode 下 mstatus.TVM=1 时访问 satp 触发异常

# 1. 从 S-mode 设置 mstatus.TVM（通过 T-SBI 请求 M-mode 操作）
li   a1, (1 << 20)                    # TVM = bit 20
RVTEST_TSBI_CSR_ACCESS 0x3005A073, a1 # csrrs x0, mstatus, a1

# 2. 在 S-mode 尝试访问 satp（应触发 illegal-instruction）
csrr t0, satp                         # 此指令应触发异常

# 3. trap handler 记录异常信息到 signature

# 4. 测试结束后清除 TVM
li   a1, (1 << 20)
RVTEST_TSBI_CSR_ACCESS 0x3005B073, a1 # csrc x0, mstatus, a1
```

---

## 5. T-SBI 在 RVA23 认证中的角色

### 5.1 认证架构

```
┌─────────────────────────────────────────────────────────────┐
│ 认证测试 (S-mode)                                            │
│   • 测试 S/U-mode 可见行为                                    │
│   • 通过 T-SBI 操控 M-mode CSR 构造测试场景                    │
│   • 通过 T-SBI 切换特权级                                     │
├─────────────────────────────────────────────────────────────┤
│ T-SBI (M-mode trap handler)                                  │
│   • 处理 ecall from S-mode（medeleg[9]=0，不委托）             │
│   • 执行 CSR 读写（直接执行指令编码）                           │
│   • 执行特权切换（修改 mstatus.MPP + mret）                    │
│   • 转发不可委托的 trap 到 S-mode                              │
├─────────────────────────────────────────────────────────────┤
│ 硬件 DUT                                                     │
│   • 可以是标准 M-mode 或自定义 M-mode                          │
│   • 自定义 M-mode 必须模拟标准行为对 S-mode 透明                │
└─────────────────────────────────────────────────────────────┘
```

### 5.2 M-mode CSR 控制要求

RVA23 认证要求 T-SBI 能控制以下影响低特权级行为的 M-mode CSR：

| CSR | 需控制的字段 |
|-----|-------------|
| mstatus | TSR, TW, TVM |
| mcounteren | 所有非 hardwired-0 字段 |
| mcountinhibit | 所有非 hardwired-0 字段 |
| mip | SEIP, STIP, SSIP |
| menvcfg | STCE, PBMTE, ADUE, PMM, CBZE, CBCFE, CBIE, SSE, LPE |
| mseccfg | SSEED, USEED |
| mstateen | SE0, ENVCFG, CONTEXT |
| medeleg | 除 S-mode ecall 外的所有异常 |
| mideleg | SEI, STI, SSI, LCOFI |

### 5.3 委托配置

```
medeleg = 0xFCB5FF
  → 委托所有异常到 S-mode，除了：
    - ecall from M-mode (cause 11)
    - ecall from S-mode (cause 9)  ← T-SBI 必须处理
    - double trap (cause 16)

mideleg = 0x366
  → 委托所有 S/U 中断到 S-mode（SEI, STI, SSI, LCOFI）
```

---

## 6. T-SBI 实现机制

### 6.1 CSR_ACCESS 的执行方式

T-SBI handler 处理 CSR_ACCESS 的核心技巧是**动态指令执行**：

```
1. 将 a0 中的指令编码写入内存（save area 的 scratch 区域）
2. 在该内存后写入 ret 指令
3. 执行 fence.i 同步指令流
4. 跳转到该内存地址执行
5. CSR 指令执行后，通过 ret 返回 handler
6. handler 恢复寄存器，mret 返回调用者
```

这种方式的优势：
- 无需为每个 CSR 编写单独的处理代码
- 自动支持所有 CSR 指令变体（CSRRW/CSRRS/CSRRC）
- 指令编码自描述，handler 无需查表

### 6.2 特权切换的实现

```
GOTO_MMODE:
  1. 设置 mstatus.MPP = M-mode (11)
  2. 执行 mret → 进入 M-mode

GOTO_SMODE:
  1. 设置 mstatus.MPP = S-mode (01)
  2. 执行 mret → 进入 S-mode

GOTO_UMODE:
  1. 设置 mstatus.MPP = U-mode (00)
  2. 执行 mret → 进入 U-mode

GOTO_VSMODE:
  1. 设置 mstatus.MPP = S-mode, hstatus.SPV = 1
  2. 执行 mret → 进入 VS-mode
```

---

## 7. 总结对比表

| 特性 | 官方 SBI | T-SBI |
|------|----------|-------|
| 规范文档 | riscv-sbi SPEC (Ratified) | ACT CTP/CRD (测试框架文档) |
| 实现代码 | OpenSBI (~1MB) | rvtest_trap_handler.h (~3000 行) |
| 调用方式 | a7=EID, a6=FID, a0-a5=args | a0=opcode, a1=arg |
| 返回方式 | a0=error, a1=value | a0=result |
| 错误处理 | 14 种错误码 | -1 (reserved) |
| 功能范围 | 完整平台服务 | CSR 访问 + 特权切换 + ecall 测试 |
| 扩展发现 | sbi_probe_extension() | 无（固定操作集） |
| 适用场景 | 生产 OS 运行 | 架构兼容性认证测试 |
| M-mode 要求 | 标准 M-mode 或 hypervisor | 支持自定义 M-mode |
| RV32E 支持 | 不支持（使用 a6/a7） | 支持（仅用 a0/a1） |
