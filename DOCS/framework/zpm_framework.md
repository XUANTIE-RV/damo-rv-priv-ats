**中文 | [English](../framework_en/zpm_framework_en.md)**

## ZPM 框架能力扩充文档

**目标**：为现有测试框架补充 RISC-V Pointer Masking (ZPM) 扩展所需的基础设施。

**规范依据**：`SPEC/zpm.adoc`（Pointer Masking Extensions, Version 1.0.0）

---

### 扩充概览

| 组件 | 修改文件 | 说明 |
|------|---------|------|
| CSR 定义扩展 | `common/encoding.h` | 新增 PMM 字段掩码、`CSR_SENVCFG`、PMM 编码常量 |
| 动态 CSR 访问 | `common/csr_accessors.c` | 新增 `senvcfg (0x10A)` / `menvcfg (0x30A)` 的 switch case |
| PM 控制 API | `common/pm/pm_cfg.h` + `pm_cfg.c` | 各特权模式 PM 使能/禁用/检测 |
| Tagged Address 工具 | `common/pm/pm_addr.h` | 地址打标/提取/ignore 变换（纯头文件） |
| VM + U-mode 执行 | `common/vm/vm.h` + `satp.c` | `vm_run_in_umode()` 支持 Ssnpm 测试 |

---

### Component 1：CSR 定义扩展

**文件**：[encoding.h](file://../..//common/encoding.h)

新增内容：

```c
/* menvcfg 区域新增 */
#define MENVCFG_PMM_OFF  32             /* PMM field offset */
#define MENVCFG_PMM_MASK (3ULL << 32)   /* PMM field mask [33:32] (Smnpm) */

/* mseccfg 区域新增 */
#define MSECCFG_PMM_OFF  32
#define MSECCFG_PMM_MASK (3ULL << 32)   /* (Smmpm) */

/* Supervisor 区域新增 */
#define CSR_SENVCFG     0x10A
#define CSR_SENVCFGH    0x11A           /* RV32 only */
#define SENVCFG_PMM_OFF  32
#define SENVCFG_PMM_MASK (3ULL << 32)   /* (Ssnpm) */

/* 通用 PMM 编码常量 */
#define PMM_DISABLED    0   /* PMLEN=0 */
#define PMM_RESERVED    1
#define PMM_PMLEN7      2   /* PMLEN=7 on RV64 */
#define PMM_PMLEN16     3   /* PMLEN=16 on RV64 */
```

---

### Component 2：动态 CSR 访问扩展

**文件**：[csr_accessors.c](file://../..//common/csr_accessors.c)

在 `csr_read()` 和 `csr_write()` 中各新增两个 switch case：

- `0x10A` — senvcfg（Ssnpm 需读写）
- `0x30A` — menvcfg（Smnpm 需读写）

> [!NOTE]
> `mseccfg (0x747)` 和 `henvcfg (0x60A)` 已在现有代码中支持，无需额外添加。

---

### Component 3：PM 控制 API

**文件**：[pm_cfg.h](file://../..//common/pm/pm_cfg.h) + [pm_cfg.c](file://../..//common/pm/pm_cfg.c)

提供的 API：

| 函数 | 说明 |
|------|------|
| `pm_set_umode(pmm)` / `pm_get_umode()` | 控制 U-mode PM（写 `senvcfg.PMM`） |
| `pm_set_smode(pmm)` / `pm_get_smode()` | 控制 S-mode PM（写 `menvcfg.PMM`） |
| `pm_set_mmode(pmm)` / `pm_get_mmode()` | 控制 M-mode PM（写 `mseccfg.PMM`） |
| `detect_ssnpm()` | 检测 Ssnpm 是否实现 |
| `detect_smnpm()` | 检测 Smnpm 是否实现 |
| `detect_smmpm()` | 检测 Smmpm 是否实现 |
| `pmm_to_pmlen(pmm)` | PMM 编码转 PMLEN 值（内联） |
| `pm_detect_supported_pmlen(...)` | 探测支持的 PMLEN 值组合 |

**检测策略**：使用 `trap_expect_begin/end` 保护，尝试写 PMM=0b11 并回读，非零即表示扩展已实现。

---

### Component 4：Tagged Address 工具

**文件**：[pm_addr.h](file://../..//common/pm/pm_addr.h)（纯头文件，内联函数）

| 函数 | 说明 |
|------|------|
| `pm_tag_address(addr, tag, pmlen)` | 在地址上 PMLEN 位嵌入 tag |
| `pm_extract_tag(addr, pmlen)` | 从地址提取 tag |
| `pm_transform_va(addr, pmlen)` | 虚拟地址 ignore 变换（sign-extend） |
| `pm_transform_pa(addr, pmlen)` | 物理地址 ignore 变换（zero-extend） |
| `pm_addrs_equivalent_va(a, b, pmlen)` | 判断两地址在 VA PM 下是否等价 |
| `pm_addrs_equivalent_pa(a, b, pmlen)` | 判断两地址在 PA PM 下是否等价 |
| `pm_max_tag(pmlen)` / `pm_alt_tag(pmlen)` | 常用测试 tag 模式 |

---

### Component 5：VM + U-mode 执行支持

**文件**：[vm.h](file://../..//common/vm/vm.h) + [satp.c](file://../..//common/vm/satp.c)

新增 `vm_run_in_umode(ctx, fn, arg)`：

- **用途**：Ssnpm 测试需在 VM 开启后以 U-mode 执行带 tag 的 load/store
- **实现**：参考 `vm_run_in_smode()`，改用 `run_in_priv(PRIV_U)`
- **差异**：
  - 不委托 page fault 到 S-mode（由 M-mode trap_record 捕获）
  - 调用方必须在页表映射中设置 `PTE_U` 标志
  - ecall 返回走 M-mode handler（CAUSE_ECALL_FROM_U, cause 8）

---

### 使用方式

ZPM 测试子目录的 Makefile 通过 `ENABLE_PM = 1` 开关引入 PM 模块支持（与 `ENABLE_PMP`、`ENABLE_VM`、`ENABLE_HYP` 一致）：

```makefile
# zpm/Makefile 示例
TARGET = zpm_test.elf
ENABLE_VM = 1
ENABLE_PMP = 1
ENABLE_PM = 1
SPIKE_ISA_EXT = _ssnpm_smnpm_smmpm

EXT_OBJS = main.o tests/test_register.o

include ../common/Makefile.common
```

设置 `ENABLE_PM = 1` 后，`Makefile.common` 会自动：
- 将 `common/pm/pm_cfg.o` 加入 `COMMON_OBJS` 参与编译链接
- 在 `CFLAGS` 中添加 `-DENABLE_PM` 预处理宏和 `-I` 包含路径

`common/pm/` 下的文件不会被未设置 `ENABLE_PM` 的测试子目录编译，对现有测试零侵入。

---

### 测试文件列表

`zpm/tests/` 目录包含以下 10 个测试文件：

| 测试文件 | 测试内容 |
|---------|---------|
| `test_cap.c` | Ssnpm 实现探测（ZPM-CAP-01: Detect Ssnpm implementation） |
| `test_csr.c` | senvcfg.PMM CSR 可写性测试（ZPM-CSR-01: senvcfg.PMM writable 0→PMLEN7） |
| `test_mpa.c` | M-mode 带标签地址加载测试（ZPM-MPA-01: PMLEN7 tagged load in M-mode） |
| `test_mprv.c` | MPRV=1 MPP=S 使用 S-mode PM 测试（ZPM-MPRV-01: MPRV=1 MPP=S uses S-mode PM） |
| `test_mxr.c` | MXR=1 禁用 PM 测试（ZPM-MXR-01: MXR=1 disables PM） |
| `test_neg.c` | 指令取指不受 PM 影响测试（ZPM-NEG-01: Instruction fetch not affected by PM） |
| `test_sva.c` | S-mode 带标签地址加载测试（ZPM-SVA-01: PMLEN7 tagged load in S-mode） |
| `test_trap.c` | stval 包含转换后地址测试（ZPM-TRAP-01: stval contains transformed address） |
| `test_uamo.c` | U-mode 带标签地址 AMO 测试（ZPM-UAMO-01: PMLEN7 amoadd.d via tagged addr） |
| `test_uva.c` | U-mode 带标签地址加载测试（ZPM-UVA-01: PMLEN7 tagged load in U-mode） |

---

### 回归验证结果

以下测试在框架修改后编译通过，无错误、无新增警告：

| 测试 | 状态 |
|------|------|
| `pmp` | 通过 |
| `svadu` | 通过 |
| `smepmp` | 通过 |
| `svvptc` | 通过 |
