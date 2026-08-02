# Ssqosid (QoS Identifiers) 扩展测试计划

## 概述

本测试计划覆盖 RISC-V Ssqosid (Quality-of-Service Identifiers) 扩展的所有核心功能点。Ssqosid 扩展引入 `srmcfg` 寄存器，用于配置 hart 的 Resource Control ID (`RCID`) 和 Monitoring Counter ID (`MCID`)，这两个标识符伴随 hart 向共享资源控制器发出的每个请求。

本测试计划依据 `SPEC/riscv-ssqosid/sqosid.adoc` 中的规范内容编写。

### 本文档覆盖的 SPEC 章节
- Quality-of-Service (QoS) Identifiers（srmcfg CSR 格式、字段行为、访问控制）
- Smstateen 与 Ssqosid 的交互（mstateen0 bit 55 门控）

### 由其他测试计划覆盖
- 虚拟化模式 (V=1) 下的访问异常行为 → `Hypervisor_cross_test_plan.md` Group 15

---

## 覆盖的规范点

本章节列出本文档所有测试组中引用的规范点（norm ID），已去重并按字母顺序排列。

| Norm ID | 原文 | 中文说明 |
|---------|------|----------|
| `norm:ssqosid_srmcfg_sz_acc` | The `srmcfg` register is an SXLEN-bit read/write register used to configure a Resource Control ID (`RCID`) and a Monitoring Counter ID (`MCID`). Both `RCID` and `MCID` are WARL fields. The CSR number is 0x181. | `srmcfg` 是一个 SXLEN 位读写寄存器，用于配置 RCID 和 MCID。两者均为 WARL 字段。CSR 编号为 0x181。 |
| `norm:ssqosid_srmcfg_format64` | When SXLEN=64, srmcfg format: bits [11:0] RCID, bits [15:12] WPRI, bits [27:16] MCID, bits [63:28] WPRI. | SXLEN=64 时 srmcfg 格式：[11:0] RCID，[15:12] WPRI，[27:16] MCID，[63:28] WPRI。 |
| `norm:ssqosid_srmcfg_format32` | When SXLEN=32, srmcfg format: bits [11:0] RCID, bits [15:12] WPRI, bits [27:16] MCID, bits [31:28] WPRI. | SXLEN=32 时 srmcfg 格式：[11:0] RCID，[15:12] WPRI，[27:16] MCID，[31:28] WPRI。 |
| `norm:ssqosid_rcid_mcid_accompany` | The `RCID` and `MCID` accompany each request made by the hart to shared resource controllers. The `RCID` is used to determine the resource allocations (e.g., cache occupancy limits, memory bandwidth limits, etc.) to enforce. The `MCID` is used to identify a counter to monitor resource usage. | RCID 和 MCID 伴随 hart 向共享资源控制器发出的每个请求。RCID 用于确定资源分配，MCID 用于标识监控计数器。 |
| `norm:ssqosid_all_priv_modes` | The `RCID` and `MCID` configured in the `srmcfg` CSR apply to all privilege modes of software execution on that hart by default, but this behavior may be overridden by future extensions. | `srmcfg` 中配置的 RCID 和 MCID 默认适用于该 hart 上所有特权模式的软件执行，但未来扩展可覆盖此行为。 |
| `norm:ssqosid_smstateen_bit55` | If extension Smstateen is implemented together with Ssqosid, then Ssqosid also requires the bit 55 in `mstateen0` introduced by Priv 1.14 to be implemented. | 若 Smstateen 与 Ssqosid 同时实现，则 Ssqosid 还要求实现 Priv 1.14 引入的 `mstateen0` 第 55 位。 |
| `norm:ssqosid_smstateen_bit55_0` | If bit 55 of `mstateen0` is 0, attempts to access `srmcfg` in privilege modes less privileged than M-mode raise an illegal-instruction exception. | 若 `mstateen0` 第 55 位为 0，在低于 M 模式的特权级尝试访问 `srmcfg` 将引发非法指令异常。 |

---

## Group 1. srmcfg 寄存器基本读写与字段行为

**规范依据**：
- `norm:ssqosid_srmcfg_sz_acc`：SXLEN-bit 读写寄存器，CSR 编号 0x181，RCID 和 MCID 为 WARL 字段
- `norm:ssqosid_srmcfg_format64`：SXLEN=64 时寄存器布局
- `norm:ssqosid_srmcfg_format32`：SXLEN=32 时寄存器布局

**测试职责**：验证 srmcfg 寄存器的基本可访问性、WARL 字段行为及 WPRI 字段为零。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SRMCFG-01 | M-mode 读写 srmcfg | M-mode 通过 CSR 指令读写 srmcfg（CSR 0x181），写入 RCID 和 MCID 字段 | 正常读写成功，WARL 字段读回值合法 |
| SRMCFG-02 | HS-mode 读写 srmcfg | HS-mode 通过 CSR 指令读写 srmcfg | 正常读写成功 |
| SRMCFG-03 | RCID 字段 WARL 行为 | 写入 RCID 全 1（0xFFF），读回；写入 0，读回；写入 walking-1 模式 | WARL 行为：读回值为实现支持的合法值，写入非法值时不触发异常 |
| SRMCFG-04 | MCID 字段 WARL 行为 | 写入 MCID 全 1（0xFFF），读回；写入 0，读回；写入 walking-1 模式 | WARL 行为：读回值为实现支持的合法值 |
| SRMCFG-05 | WPRI 字段读为零（SXLEN=64） | 写入 srmcfg 全 1，读回检查 bits[15:12] 和 bits[63:28] | WPRI 字段读回为零 |
| SRMCFG-06 | WPRI 字段读为零（SXLEN=32） | 写入 srmcfg 全 1，读回检查 bits[15:12] 和 bits[31:28] | WPRI 字段读回为零 |
| SRMCFG-07 | RCID 和 MCID 独立性 | 仅写 RCID 字段（MCID 保持），读回验证 MCID 不变；反之亦然 | 两个字段互不影响 |

---

## Group 2. srmcfg 跨特权模式适用性

**规范依据**：
- `norm:ssqosid_all_priv_modes`：srmcfg 中配置的 RCID 和 MCID 默认适用于该 hart 上所有特权模式
- `norm:ssqosid_rcid_mcid_accompany`：RCID 和 MCID 伴随 hart 向共享资源控制器发出的每个请求

**测试职责**：验证 srmcfg 配置在所有特权模式下生效（通过 CSR 可读性验证，资源控制器行为依赖平台实现不做强制验证）。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SRMCFG-08 | HS-mode 配置后 S-mode 可见 | HS-mode 写 srmcfg 特定 RCID/MCID 值，切换到 S-mode（U-mode 不可直接访问）后切回 HS-mode 读 | 值保持不变，说明配置跨模式持续生效 |
| SRMCFG-09 | M-mode 配置后 HS-mode 可见 | M-mode 写 srmcfg 特定值，切换到 HS-mode 读 srmcfg | 读到 M-mode 写入的值 |
| SRMCFG-10 | 上下文切换场景 | HS-mode 写 srmcfg 值 A，模拟切换到另一上下文写值 B，再切回读 | 值反映最后一次写入（软件负责上下文切换时保存/恢复） |

---

## Group 3. Smstateen 门控访问控制

**规范依据**：
- `norm:ssqosid_smstateen_bit55`：Smstateen 与 Ssqosid 同时实现时，要求实现 mstateen0 bit 55
- `norm:ssqosid_smstateen_bit55_0`：mstateen0[55]=0 时，低于 M 模式的特权级访问 srmcfg 触发 illegal-instruction exception

**测试职责**：验证 mstateen0 bit 55 对 srmcfg 访问的门控行为。本组测试仅在 Smstateen 扩展已实现时执行。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SRMCFG-11 | mstateen0[55]=0 HS-mode 访问 srmcfg 触发异常 | M-mode 设置 mstateen0[55]=0，HS-mode 尝试 csrr srmcfg | illegal-instruction exception (cause=2) |
| SRMCFG-12 | mstateen0[55]=0 HS-mode 写 srmcfg 触发异常 | M-mode 设置 mstateen0[55]=0，HS-mode 尝试 csrw srmcfg | illegal-instruction exception (cause=2) |
| SRMCFG-13 | mstateen0[55]=0 S-mode 访问 srmcfg 触发异常 | M-mode 设置 mstateen0[55]=0，S-mode 尝试 csrr srmcfg | illegal-instruction exception (cause=2) |
| SRMCFG-14 | mstateen0[55]=0 U-mode 访问 srmcfg 触发异常 | M-mode 设置 mstateen0[55]=0，U-mode 尝试 csrr srmcfg | illegal-instruction exception (cause=2) |
| SRMCFG-15 | mstateen0[55]=1 HS-mode 正常访问 srmcfg | M-mode 设置 mstateen0[55]=1，HS-mode csrr/csrw srmcfg | 正常读写成功 |
| SRMCFG-16 | mstateen0[55]=1 S-mode 正常访问 srmcfg | M-mode 设置 mstateen0[55]=1，S-mode csrr/csrw srmcfg | 正常读写成功 |
| SRMCFG-17 | M-mode 不受 mstateen0[55] 限制 | M-mode 设置 mstateen0[55]=0，M-mode 自身 csrr/csrw srmcfg | 正常读写成功（M-mode 不受门控） |
| SRMCFG-18 | mstateen0[55] 位可写性验证 | M-mode 写 mstateen0[55]=1 读回，写 0 读回 | bit 55 可写（Smstateen+Ssqosid 同时实现时必须可写） |

---

## Group 4. U-mode 访问控制

**规范依据**：
- `norm:ssqosid_srmcfg_sz_acc`：srmcfg 为 S 级 CSR（CSR 编号 0x181，地址范围属 S 级）

**测试职责**：验证 U-mode（非虚拟化环境下）对 srmcfg 的访问限制。

| 测试 ID | 测试名称 | 测试描述 | 预期结果 |
|---------|----------|----------|----------|
| SRMCFG-25 | U-mode 访问 srmcfg 触发 illegal-instruction exception | 无 Smstateen 门控（或 mstateen0[55]=1），U-mode 尝试 csrr srmcfg | illegal-instruction exception (cause=2)（S 级 CSR 在 U-mode 不可访问） |
| SRMCFG-26 | S-mode 正常访问 srmcfg | S-mode csrr/csrw srmcfg | 正常读写成功 |

---
