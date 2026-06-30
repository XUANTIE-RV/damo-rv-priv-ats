**[中文](../framework/zpm_framework.md) | English**

## ZPM Framework Capability Extension Document

**Objective**: Supplement the existing test framework with infrastructure required for RISC-V Pointer Masking (ZPM) extensions.

**Specification Reference**: `SPEC/zpm.adoc` (Pointer Masking Extensions, Version 1.0.0)

---

### Extension Overview

| Component | Modified Files | Description |
|------|---------|------|
| CSR Definition Extension | `common/encoding.h` | Add PMM field masks, `CSR_SENVCFG`, and PMM encoding constants |
| Dynamic CSR Access | `common/csr_accessors.c` | Add switch cases for `senvcfg (0x10A)` / `menvcfg (0x30A)` |
| PM Control API | `common/pm/pm_cfg.h` + `pm_cfg.c` | PM enable/disable/detection for each privilege mode |
| Tagged Address Utilities | `common/pm/pm_addr.h` | Address tagging/extraction/ignore transformation (header-only) |
| VM + U-mode Execution | `common/vm/vm.h` + `satp.c` | `vm_run_in_umode()` supports Ssnpm testing |

---

### Component 1: CSR Definition Extension

**File**: [encoding.h](file://../..//common/encoding.h)

New additions:

```c
/* Added to menvcfg section */
#define MENVCFG_PMM_OFF  32             /* PMM field offset */
#define MENVCFG_PMM_MASK (3ULL << 32)   /* PMM field mask [33:32] (Smnpm) */

/* Added to mseccfg section */
#define MSECCFG_PMM_OFF  32
#define MSECCFG_PMM_MASK (3ULL << 32)   /* (Smmpm) */

/* Added to Supervisor section */
#define CSR_SENVCFG     0x10A
#define CSR_SENVCFGH    0x11A           /* RV32 only */
#define SENVCFG_PMM_OFF  32
#define SENVCFG_PMM_MASK (3ULL << 32)   /* (Ssnpm) */

/* Generic PMM encoding constants */
#define PMM_DISABLED    0   /* PMLEN=0 */
#define PMM_RESERVED    1
#define PMM_PMLEN7      2   /* PMLEN=7 on RV64 */
#define PMM_PMLEN16     3   /* PMLEN=16 on RV64 */
```

---

### Component 2: Dynamic CSR Access Extension

**File**: [csr_accessors.c](file://../..//common/csr_accessors.c)

Add two new switch cases in both `csr_read()` and `csr_write()`:

- `0x10A` — senvcfg (required for Ssnpm read/write)
- `0x30A` — menvcfg (required for Smnpm read/write)

> [!NOTE]
> `mseccfg (0x747)` and `henvcfg (0x60A)` are already supported in existing code; no additional changes needed.

---

### Component 3: PM Control API

**Files**: [pm_cfg.h](file://../..//common/pm/pm_cfg.h) + [pm_cfg.c](file://../..//common/pm/pm_cfg.c)

Provided APIs:

| Function | Description |
|------|------|
| `pm_set_umode(pmm)` / `pm_get_umode()` | Control U-mode PM (write `senvcfg.PMM`) |
| `pm_set_smode(pmm)` / `pm_get_smode()` | Control S-mode PM (write `menvcfg.PMM`) |
| `pm_set_mmode(pmm)` / `pm_get_mmode()` | Control M-mode PM (write `mseccfg.PMM`) |
| `detect_ssnpm()` | Detect whether Ssnpm is implemented |
| `detect_smnpm()` | Detect whether Smnpm is implemented |
| `detect_smmpm()` | Detect whether Smmpm is implemented |
| `pmm_to_pmlen(pmm)` | Convert PMM encoding to PMLEN value (inline) |
| `pm_detect_supported_pmlen(...)` | Probe supported PMLEN value combinations |

**Detection Strategy**: Use `trap_expect_begin/end` protection, attempt to write PMM=0b11 and read back; a non-zero result indicates the extension is implemented.

---

### Component 4: Tagged Address Utilities

**File**: [pm_addr.h](file://../..//common/pm/pm_addr.h) (header-only, inline functions)

| Function | Description |
|------|------|
| `pm_tag_address(addr, tag, pmlen)` | Embed tag into address at PMLEN bits |
| `pm_extract_tag(addr, pmlen)` | Extract tag from address |
| `pm_transform_va(addr, pmlen)` | Virtual address ignore transformation (sign-extend) |
| `pm_transform_pa(addr, pmlen)` | Physical address ignore transformation (zero-extend) |
| `pm_addrs_equivalent_va(a, b, pmlen)` | Determine if two addresses are equivalent under VA PM |
| `pm_addrs_equivalent_pa(a, b, pmlen)` | Determine if two addresses are equivalent under PA PM |
| `pm_max_tag(pmlen)` / `pm_alt_tag(pmlen)` | Common test tag patterns |

---

### Component 5: VM + U-mode Execution Support

**Files**: [vm.h](file://../..//common/vm/vm.h) + [satp.c](file://../..//common/vm/satp.c)

New addition: `vm_run_in_umode(ctx, fn, arg)`:

- **Purpose**: Ssnpm tests need to execute tagged load/store operations in U-mode after VM is enabled
- **Implementation**: Based on `vm_run_in_smode()`, but uses `run_in_priv(PRIV_U)` instead
- **Differences**:
  - Does not delegate page faults to S-mode (captured by M-mode trap_record)
  - Caller must set `PTE_U` flag in page table mappings
  - ecall returns via M-mode handler (CAUSE_ECALL_FROM_U, cause 8)

---

### Usage

The Makefile in the ZPM test subdirectory enables PM module support via the `ENABLE_PM = 1` switch (consistent with `ENABLE_PMP`, `ENABLE_VM`, `ENABLE_HYP`):

```makefile
# zpm/Makefile example
TARGET = zpm_test.elf
ENABLE_VM = 1
ENABLE_PMP = 1
ENABLE_PM = 1
SPIKE_ISA_EXT = _ssnpm_smnpm_smmpm

EXT_OBJS = main.o tests/test_register.o

include ../common/Makefile.common
```

After setting `ENABLE_PM = 1`, `Makefile.common` will automatically:
- Add `common/pm/pm_cfg.o` to `COMMON_OBJS` for compilation and linking
- Add `-DENABLE_PM` preprocessor macro and `-I` include path to `CFLAGS`

Files under `common/pm/` will not be compiled by test subdirectories that do not set `ENABLE_PM`, ensuring zero intrusion on existing tests.

---

### Test File List

The `zpm/tests/` directory contains the following 10 test files:

| Test File | Test Content |
|---------|---------|
| `test_cap.c` | Ssnpm implementation detection (ZPM-CAP-01: Detect Ssnpm implementation) |
| `test_csr.c` | senvcfg.PMM CSR writability test (ZPM-CSR-01: senvcfg.PMM writable 0→PMLEN7) |
| `test_mpa.c` | M-mode tagged address load test (ZPM-MPA-01: PMLEN7 tagged load in M-mode) |
| `test_mprv.c` | MPRV=1 MPP=S uses S-mode PM test (ZPM-MPRV-01: MPRV=1 MPP=S uses S-mode PM) |
| `test_mxr.c` | MXR=1 disables PM test (ZPM-MXR-01: MXR=1 disables PM) |
| `test_neg.c` | Instruction fetch not affected by PM test (ZPM-NEG-01: Instruction fetch not affected by PM) |
| `test_sva.c` | S-mode tagged address load test (ZPM-SVA-01: PMLEN7 tagged load in S-mode) |
| `test_trap.c` | stval contains transformed address test (ZPM-TRAP-01: stval contains transformed address) |
| `test_uamo.c` | U-mode tagged address AMO test (ZPM-UAMO-01: PMLEN7 amoadd.d via tagged addr) |
| `test_uva.c` | U-mode tagged address load test (ZPM-UVA-01: PMLEN7 tagged load in U-mode) |

---

### Regression Verification Results

The following tests compiled successfully after framework modifications, with no errors or new warnings:

| Test | Status |
|------|------|
| `pmp` | Passed |
| `svadu` | Passed |
| `smepmp` | Passed |
| `svvptc` | Passed |
