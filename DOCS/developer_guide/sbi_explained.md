# RISC-V SBI (Supervisor Binary Interface) 详解

## 1. SBI 的定义与目的

### 1.1 什么是 SBI

SBI (Supervisor Binary Interface) 是 RISC-V 架构中定义的一套** supervisor-mode 软件与更高特权级固件之间的标准接口**。它允许 supervisor-mode（S-mode 或 VS-mode）软件在所有 RISC-V 实现上具有可移植性，通过为平台（或 hypervisor）特定功能定义统一的抽象层。

### 1.2 为什么需要 SBI

RISC-V 特权架构将系统分为多个特权级别：

```
┌─────────────────────────────────────────┐
│  M-mode (Machine)                       │  ← 最高特权级，固件运行于此
│  ┌───────────────────────────────────┐  │
│  │  S-mode (Supervisor)              │  │  ← 操作系统内核运行于此
│  │  ┌───────────────────────────┐    │  │
│  │  │  U-mode (User)            │    │  │  ← 应用程序运行于此
│  │  └───────────────────────────┘    │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

**核心问题**：S-mode 软件（如 OS 内核）无法直接访问 M-mode CSR 和执行某些硬件操作（如配置 PMP、管理 hart 电源状态、访问平台特定设备等）。

**SBI 的解决方案**：

1. **硬件抽象**：将平台特定操作封装为标准 API，OS 无需关心底层硬件差异
2. **特权级桥接**：提供从 S-mode 到 M-mode 的安全服务调用通道
3. **可移植性**：同一个 OS 内核可以在不同 RISC-V 平台上运行，只需平台提供符合 SBI 规范的固件
4. **虚拟化支持**：在有 H 扩展的系统上，hypervisor 可以作为 SBI 实现向 Guest OS 提供服务

### 1.3 SBI 实现 (SEE)

提供 SBI 接口的高特权级软件称为 **SBI 实现** 或 **SEE (Supervisor Execution Environment)**：

- **无 H 扩展**：SEE 是运行在 M-mode 的平台运行时固件（如 OpenSBI）
- **有 H 扩展**：SEE 可以是运行在 HS-mode 的 hypervisor（如 KVM）

常见的 SBI 实现：

| ID | 名称 | 说明 |
|----|------|------|
| 0 | BBL | Berkeley Boot Loader（早期） |
| 1 | OpenSBI | 开源 SBI 实现（最常用） |
| 2 | Xvisor | 虚拟化 hypervisor |
| 3 | KVM | Linux KVM |
| 4 | RustSBI | Rust 语言实现 |
| 5 | Diosix | 轻量级 hypervisor |

---

## 2. SBI 接口定义

### 2.1 调用约定 (Binary Encoding)

所有 SBI 函数共享统一的二进制编码，通过 `ECALL` 指令实现调用：

```
┌──────────────────────────────────────────────────────────┐
│                    SBI 调用约定                            │
├──────────┬───────────────────────────────────────────────┤
│ 寄存器    │ 用途                                         │
├──────────┼───────────────────────────────────────────────┤
│ a7       │ Extension ID (EID) - 扩展标识                  │
│ a6       │ Function ID (FID) - 函数标识（v0.2+）          │
│ a0-a5    │ 输入参数                                      │
│ a0       │ 返回值：错误码 (error)                         │
│ a1       │ 返回值：数据 (value)                           │
└──────────┴───────────────────────────────────────────────┘
```

**C 语言伪代码表示**：

```c
struct sbiret {
    long error;   // a0: 错误码
    union {
        long value;           // a1: 有符号返回值
        unsigned long uvalue; // a1: 无符号返回值
    };
};
```

### 2.2 标准错误码

| 错误码 | 值 | 含义 |
|--------|-----|------|
| SBI_SUCCESS | 0 | 成功 |
| SBI_ERR_FAILED | -1 | 通用失败 |
| SBI_ERR_NOT_SUPPORTED | -2 | 不支持 |
| SBI_ERR_INVALID_PARAM | -3 | 参数无效 |
| SBI_ERR_DENIED | -4 | 被拒绝 |
| SBI_ERR_INVALID_ADDRESS | -5 | 地址无效 |
| SBI_ERR_ALREADY_AVAILABLE | -6 | 已可用 |
| SBI_ERR_ALREADY_STARTED | -7 | 已启动 |
| SBI_ERR_ALREADY_STOPPED | -8 | 已停止 |
| SBI_ERR_NO_SHMEM | -9 | 共享内存不可用 |
| SBI_ERR_INVALID_STATE | -10 | 状态无效（v3.0 新增） |
| SBI_ERR_BAD_RANGE | -11 | 范围无效（v3.0 新增） |
| SBI_ERR_TIMEOUT | -12 | 超时（v3.0 新增） |
| SBI_ERR_IO | -13 | I/O 错误（v3.0 新增） |
| SBI_ERR_DENIED_LOCKED | -14 | 因锁定被拒绝（v3.0 新增） |

### 2.3 调用示例

```c
// 底层 ECALL 封装
struct sbiret sbi_ecall(long eid, long fid,
                         long a0, long a1, long a2,
                         long a3, long a4, long a5)
{
    register long r_a0 asm("a0") = a0;
    register long r_a1 asm("a1") = a1;
    register long r_a6 asm("a6") = fid;
    register long r_a7 asm("a7") = eid;
    // ... 设置其他参数 ...

    asm volatile("ecall"
        : "+r"(r_a0), "+r"(r_a1)
        : "r"(r_a6), "r"(r_a7)
        : "memory");

    return (struct sbiret){r_a0, r_a1};
}

// 使用示例：获取 SBI 版本
struct sbiret ret = sbi_ecall(0x10, 0, 0, 0, 0, 0, 0, 0);
// ret.error = 0 (成功)
// ret.value = 版本号 (major << 24 | minor)
```

---

## 3. SBI 扩展体系

### 3.1 扩展分类

SBI 采用模块化设计，功能按扩展 (Extension) 组织：

```
SBI Extensions
├── Base (0x10)              - 必须实现，版本查询/扩展探测
├── Timer (0x54494D45)       - 定时器管理
├── IPI (0x735049)           - 处理器间中断
├── RFENCE (0x52464E43)      - 远程 fence 操作
├── HSM (0x48534D)           - Hart 状态管理
├── SRST (0x53525354)        - 系统复位
├── PMU (0x504D55)           - 性能监控
├── DBCN (0x4442434E)        - 调试控制台
├── SUSP (0x53555350)        - 系统挂起
├── CPPC (0x43505043)        - 处理器性能控制
├── NACL (0x4E41434C)        - 嵌套虚拟化加速
├── STA (0x535441)           - Steal-time 统计
├── SSE (0x535345)           - 软件事件 [v3.0]
├── FWFT (0x46574654)        - 固件特性 [v3.0]
├── DBTR (0x44425452)        - 调试触发器 [v3.0]
├── MPXY (0x4D505859)        - 消息代理 [v3.0]
└── Legacy (0x00-0x08)       - 旧版兼容（已废弃）
```

### 3.2 Base 扩展（必须实现）

Base 扩展是唯一必须被所有 SBI 实现支持的扩展：

| FID | 函数 | 说明 |
|-----|------|------|
| 0 | sbi_get_spec_version | 获取 SBI 规范版本 |
| 1 | sbi_get_impl_id | 获取实现 ID |
| 2 | sbi_get_impl_version | 获取实现版本 |
| 3 | sbi_probe_extension | 探测扩展是否可用 |
| 4 | sbi_get_mvendorid | 获取 mvendorid |
| 5 | sbi_get_marchid | 获取 marchid |
| 6 | sbi_get_mimpid | 获取 mimpid |

### 3.3 Hart Mask 参数

需要指定多个 hart 的函数使用 hart mask 参数：

```c
// hart_mask: 位向量，bit[i]=1 表示 hart_mask_base+i
// hart_mask_base: 起始 hartid
// hart_mask_base = -1: 忽略 mask，表示所有可用 hart
struct sbiret sbi_send_ipi(unsigned long hart_mask,
                            unsigned long hart_mask_base);
```

### 3.4 共享内存参数

需要传递大数据的函数使用共享内存：

- 物理地址必须可访问，否则返回 `SBI_ERR_INVALID_ADDRESS`
- SBI 实现必须检查 S-mode 是否有权访问该内存
- 数据必须使用小端字节序
- 使用 PMA 属性访问

---

## 4. SBI 设计原则

### 4.1 核心原则

1. **最小化核心 + 模块化扩展**
   - 遵循 RISC-V 哲学：小核心 + 可选扩展
   - Base 扩展极简，仅包含探测和版本查询
   - 功能按需实现，不强制所有扩展

2. **向后兼容**
   - 旧版接口保留为 Legacy 扩展
   - 新版函数使用 EID+FID 二级寻址
   - 错误码只增不改

3. **可发现性**
   - `sbi_probe_extension()` 运行时探测扩展可用性
   - 扩展不可部分实现（除非定义了函数级探测机制）
   - 版本协商通过 `sbi_get_spec_version()`

4. **平台无关**
   - 不定义硬件发现方法（由 Device Tree/ACPI 负责）
   - 不假设特定平台布局
   - 共享内存使用物理地址

5. **安全性**
   - SBI 实现必须验证所有输入参数
   - 共享内存必须检查访问权限
   - 不暴露 M-mode 内部状态

### 4.2 调用约定原则

- **寄存器保留**：除 a0/a1 外，所有寄存器由被调用者保留
- **错误码优先**：a0 始终返回错误码，成功时 a1 返回数据
- **错误时 a1 未定义**：除非函数明确定义
- **不支持返回 NOT_SUPPORTED**：无效 EID/FID 必须返回 -2

### 4.3 扩展实现原则

- 扩展整体可选，但不可部分实现（除非有探测机制）
- 若 `sbi_probe_extension()` 返回可用，则该版本的所有函数必须符合规范
- 保留参数/标志位必须为零，否则返回 `SBI_ERR_INVALID_PARAM`

---

## 5. SBI 版本演进

### 5.1 版本历史

| 版本 | 状态 | 关键变化 |
|------|------|----------|
| v0.1 | 废弃 | 初始版本，Legacy 接口 |
| v0.2 | 废弃 | 引入 EID+FID 调用约定，Legacy 变为可选 |
| v0.3 | 废弃 | 新增 SRST、HSM suspend、PMU 扩展 |
| v1.0 | Ratified | 首个正式批准版本 |
| v2.0 | Ratified | 新增 DBCN、SUSP、CPPC、NACL、STA、PMU snapshot |
| **v3.0** | **Ratified** | **新增 SSE、FWFT、DBTR、MPXY；新增错误码** |

### 5.2 SBI 3.0 vs 2.0 主要变化

#### 新增扩展

| 扩展 | EID | 说明 |
|------|-----|------|
| **SSE** (Supervisor Software Events) | 0x535345 | 软件事件注入机制，支持 RAS 事件、double trap 通知、PMU 溢出事件 |
| **FWFT** (Firmware Features) | 0x46574654 | 固件特性管理：misaligned 委托、landing pad、shadow stack、double trap、PTE A/D 硬件更新、pointer masking |
| **DBTR** (Debug Triggers) | 0x44425452 | S-mode 调试触发器抽象，允许 OS 使用硬件断点 |
| **MPXY** (Message Proxy) | 0x4D505859 | 消息代理通道，支持 RPMI 等消息协议 |

#### 新增错误码

| 错误码 | 值 | 用途 |
|--------|-----|------|
| SBI_ERR_INVALID_STATE | -10 | SSE 事件状态无效 |
| SBI_ERR_BAD_RANGE | -11 | 参数范围无效 |
| SBI_ERR_TIMEOUT | -12 | 操作超时 |
| SBI_ERR_IO | -13 | I/O 错误 |
| SBI_ERR_DENIED_LOCKED | -14 | FWFT 特性被锁定后拒绝修改 |

#### PMU 扩展增强

- 新增 `sbi_pmu_event_get_info()` 函数（FID #8）：通过共享内存批量查询事件信息
- 新增 Hardware raw events v2 (Type #3)：支持 56-bit mhpmevent 值（v1 仅 48-bit）

#### SSE 扩展详解（v3.0 核心新特性）

SSE 提供了从 SBI 实现向 supervisor 软件注入软件事件的机制：

```
事件状态机：UNUSED → REGISTERED → ENABLED → RUNNING
                                              ↓
                                         (事件触发)
```

- **Local 事件**：绑定到特定 hart
- **Global 事件**：系统级，任意 hart 可处理
- **标准事件**：High Priority RAS、Double Trap、PMU Overflow、Low Priority RAS

#### FWFT 扩展详解（v3.0 核心新特性）

允许 S-mode 软件控制固件特性：

| Feature | 说明 |
|---------|------|
| MISALIGNED_EXC_DELEG | 控制非对齐访问异常委托到 S-mode |
| LANDING_PAD | 控制 Zicfilp landing pad |
| SHADOW_STACK | 控制 Zicfiss shadow stack |
| DOUBLE_TRAP | 控制 Ssdbltrp double trap |
| PTE_AD_HW_UPDATING | 控制 PTE A/D 位硬件更新 |
| POINTER_MASKING_PMLEN | 控制指针屏蔽长度 |

支持 **LOCK 标志**：一旦锁定，特性值不可修改直到 hart reset（local）或 system reset（global）。

### 5.3 从 v2.0 迁移到 v3.0 的注意事项

1. **向后兼容**：所有 v2.0 接口在 v3.0 中保持不变
2. **新错误码**：实现应正确处理新增错误码
3. **扩展探测**：使用 `sbi_probe_extension()` 探测新扩展
4. **SSE 依赖**：SSE double trap 事件需要先通过 FWFT 启用 DOUBLE_TRAP 特性
5. **MPXY 互斥**：对于同一消息协议，实现只能提供 MPXY 或专用 SBI 扩展之一，不可同时提供

---

## 6. 典型使用场景

### 6.1 OS 启动流程

```
1. OpenSBI (M-mode) 初始化
2. OpenSBI 跳转到 OS 内核 (S-mode)
3. OS 调用 sbi_get_spec_version() 确认 SBI 版本
4. OS 调用 sbi_probe_extension() 探测可用扩展
5. OS 根据可用扩展初始化各子系统
```

### 6.2 定时器管理

```c
// 设置下一次定时器中断
struct sbiret sbi_set_timer(uint64_t stime_value);

// 清除 pending：设置远未来值
sbi_set_timer((uint64_t)-1);
```

### 6.3 多核管理

```c
// 启动另一个 hart
sbi_hart_start(hartid, start_addr, opaque);

// 查询 hart 状态
sbi_hart_get_status(hartid);

// 发送 IPI
sbi_send_ipi(hart_mask, hart_mask_base);
```

### 6.4 系统关机

```c
// 关机（成功时不返回）
sbi_system_reset(SBI_SRST_RESET_TYPE_SHUTDOWN,
                  SBI_SRST_RESET_REASON_NONE);
```

---

## 7. 参考文档

- 规范源码：`SPEC/riscv-sbi/`
- 测试计划：`DOCS/testplan/sbi_test_plan.md`
- 测试实现：`SBI/`
- 公共框架 SBI 接口：`common/sbi/`
