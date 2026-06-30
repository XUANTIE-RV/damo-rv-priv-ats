**[中文](../framework/build_framework.md) | English**

# Build Framework: Multi-Target Support

## Overview

The build system supports two modes:

1. **Single-target mode** (legacy) — Each extension directory compiles to one ELF file
2. **Multi-target mode** (new) — The same extension directory compiles to multiple independent ELF files

Both modes share identical compilation parameters, linker scripts, and simulator integration. Existing single-target extensions **require no modifications** to continue working.

---

## Single-Target Mode (Legacy)

Define `TARGET` and `EXT_OBJS`:

```makefile
TARGET = pmp_test.elf
ENABLE_PMP = 1
EXT_OBJS = pmp_reset.o main.o tests/test_register.o
include ../common/Makefile.common
```

---

## Multi-Target Mode

Define `TARGETS` (plural) and a `<base>_OBJS` variable for each target:

```makefile
ENABLE_PMP = 1

TARGETS = pmp_cvpt.elf pmp_basic.elf pmp_lock.elf

pmp_cvpt_OBJS  = pmp_reset.o main_cvpt.o tests/register_cvpt.o
pmp_basic_OBJS = pmp_reset.o main_basic.o tests/register_basic.o
pmp_lock_OBJS  = pmp_reset.o main_lock.o tests/register_lock.o

include ../common/Makefile.common
```

### Variable Naming Convention

The `_OBJS` variable name is derived from the ELF filename:
1. Remove the `.elf` suffix
2. Replace `-` and `.` in the filename with `_`

Examples:

| Target Filename      | Corresponding Variable Name |
|----------------------|----------------------------|
| `pmp_cvpt.elf`       | `pmp_cvpt_OBJS`           |
| `pmp-basic.elf`      | `pmp_basic_OBJS`          |
| `sv39_walk.test.elf` | `sv39_walk_test_OBJS`     |

---

## Directory Structure (Multi-Target Example)

Using the `pmp` extension as an example, the restructured directory layout:

```
pmp/
├── Makefile                 # Defines TARGETS and _OBJS for each target
├── kernel.ld                # Shared linker script (no modification needed)
├── pmp_regions.ld           # Shared PMP region definitions (no modification needed)
├── pmp_reset.c              # Shared initialization code (shared by all ELFs)
│
├── main_cvpt.c              # Entry file for pmp_cvpt.elf
├── main_basic.c             # Entry file for pmp_basic.elf
├── main_lock.c              # Entry file for pmp_lock.elf
│
└── tests/
    ├── test_helpers.h       # Shared test helper header
    │
    ├── register_cvpt.c      # Test registration file for pmp_cvpt.elf
    ├── register_basic.c     # Test registration file for pmp_basic.elf
    ├── register_lock.c      # Test registration file for pmp_lock.elf
    │
    ├── test_cvpt_napot.c    # Test implementation files (included by register files via #include)
    ├── test_cap.c
    ├── test_napot.c
    ├── test_tor.c
    ├── test_lock.c
    └── ...
```

### Build Artifacts (All in the Same Directory)

```
pmp/
├── pmp_cvpt.elf  / .asm / .sym     # Coverpoint tests
├── pmp_basic.elf / .asm / .sym     # Basic functionality tests
├── pmp_lock.elf  / .asm / .sym     # Lock-specific tests
├── pmp_reset.o                      # Compiled once, linked into all ELFs
├── main_cvpt.o
├── main_basic.o
├── main_lock.o
└── tests/
    ├── register_cvpt.o
    ├── register_basic.o
    └── register_lock.o
```

---

## Test Registration Mechanism and Multi-Target Integration

### Principle

Test registration relies on the following chain:

1. The `TEST_REGISTER(fn)` macro places function pointers into the `.test_table` section
2. `kernel.ld` collects all `.test_table` sections and defines `_test_table` / `_test_table_end` symbols
3. The `main` function iterates through the `_test_table[]` array to execute all registered tests

**Key Point**: The linker only collects `.test_table` sections from **.o files participating in the current link**. Therefore, test sets for different ELFs are naturally isolated, and `kernel.ld` requires no modifications.

### Data Flow

```
register_cvpt.c ──compile──► register_cvpt.o ──┐
                                                ├──link──► pmp_cvpt.elf
main_cvpt.o ────────────────────────────────────┘
  └── .test_table contains only cvpt test function pointers

register_basic.c ──compile──► register_basic.o ──┐
                                                  ├──link──► pmp_basic.elf
main_basic.o ─────────────────────────────────────┘
  └── .test_table contains only basic test function pointers
```

### Step-by-Step Details

1. **Write test implementations** (`tests/test_xxx.c`): Use `TEST_REGISTER(fn)` within each file to declare test functions.

2. **Create a registration file for each ELF** (e.g., `tests/register_cvpt.c`): Selectively include required test files via `#include`.

3. **Create an entry file for each ELF** (e.g., `main_cvpt.c`): Iterate through `_test_table[]` to execute tests; each main file only needs to modify the banner title.

4. **No modification to `kernel.ld` required**: The linker script automatically collects section contents based on the .o files participating in the link.

### Registration File Example

```c
// tests/register_cvpt.c
#include "test_framework.h"
#include "pmp_cfg.h"

/* This ELF includes only coverpoint-related tests */
#include "test_cvpt_napot.c"
```

```c
// tests/register_basic.c
#include "test_framework.h"
#include "pmp_cfg.h"

/* This ELF includes basic functionality tests */
#include "test_cap.c"
#include "test_napot.c"
#include "test_tor.c"
#include "test_na4.c"
#include "test_rwx.c"
#include "test_priority.c"
#include "test_mmode.c"
#include "test_multi.c"
#include "test_granularity.c"
```

### Entry File Example

```c
// main_cvpt.c
#include "test_framework.h"
#include "pmp_cfg.h"

extern void pmp_reset(void);
extern test_func_t _test_table[];
extern test_func_t _test_table_end[];

void main(void) {
    uart_init();
    reset_state();
    pmp_reset();

    printf("\n");
    printf("==============================================\n");
    printf("  PMP Coverpoint NAPOT Tests\n");
    printf("==============================================\n");
    printf("  Platform: %s | XLEN: %d\n", CONFIG_NAME, __riscv_xlen);

    unsigned int test_count = (unsigned int)(
        (uintptr_t)_test_table_end - (uintptr_t)_test_table
    ) / sizeof(test_func_t);
    printf("  Test count: %u\n", test_count);
    printf("==============================================\n\n");

    for (unsigned int i = 0; i < test_count; i++) {
        _test_table[i]();
    }

    test_print_summary();
}
```

---

## Top-Level Build System

The root `Makefile` provides a unified build entry point, supporting batch building and running by extension groups.

### Extension Groups

The project organizes 60+ extensions into 6 functional groups:

| Group Name   | Included Extensions                                                                                                                              | Build Command      |
|--------------|--------------------------------------------------------------------------------------------------------------------------------------------------|--------------------|
| `PMP_GROUP`  | pmp, smepmp, spmp, pmp_sv39, pmp_sv48, pmp_sv57                                                                                                 | `make pmp-group`   |
| `SV_GROUP`   | Sv39, Sv48, Sv57, Svbare, Svade, Svadu, Svnapot, Svinval, Svpbmt, Svvptc, Svrsw60t59b                                                           | `make sv-group`    |
| `SS_GROUP`   | Ssccptr, Sscofpmf, Sscounterenw, Ssstateen, Sstc, Sstvala, Sstvecd, Ssu64xl                                                                      | `make ss-group`    |
| `SM_GROUP`   | Smstateen, smrnmi                                                                                                                                 | `make sm-group`    |
| `HYP_GROUP`  | Sv39x4, Sv48x4, Sv57x4, Sv39x4_Sv39, Sv39x4_Sv48, Sv39x4_Sv57, Sv48x4_Sv48, Sv57x4_Sv57, Hypervisor, Sha, Shcounterenw, Shgatpa, Shlcofideleg, Shtvala, Shvsatpa, Shvstvala, Shvstvecd | `make hyp-group`   |
| `INT_GROUP`  | aia_aplic, aia_imsic, aia_smaia, aia_ipi_iommu, aia_vs                                                                                            | `make int-group`   |

**Common Build Commands:**

```bash
make all              # Build all extensions
make pmp              # Build a single extension
make pmp-group        # Build all extensions in the PMP group
make sv-group         # Build all extensions in the SV group
make clean            # Clean all build artifacts
```

### Simulator Run Targets

The root Makefile automatically generates three simulator run targets for each extension:

| Command Format     | Description                              | Example              |
|--------------------|------------------------------------------|----------------------|
| `make sail-<ext>`  | Run specified extension on Sail simulator | `make sail-pmp`      |
| `make spike-<ext>` | Run specified extension on Spike simulator| `make spike-sv39`    |
| `make qemu-<ext>`  | Run specified extension on QEMU          | `make qemu-Hypervisor`|

**Batch Running:**

```bash
make sail-all         # Run all extensions sequentially on Sail
make spike-all        # Run all extensions sequentially on Spike
make qemu-all         # Run all extensions sequentially on QEMU
```

Each extension runs independently and sequentially. If any extension fails (returns non-zero), the sequence stops immediately.

---

## Simulator Targets (Within Extension)

Inside an extension directory, under multi-target mode, simulator commands automatically iterate through all ELFs:

| Command        | Behavior                                              |
|----------------|-------------------------------------------------------|
| `make`         | Compile all TARGETS                                   |
| `make spike`   | Compile then run each ELF sequentially on Spike       |
| `make sail`    | Compile then run each ELF sequentially on Sail        |
| `make trace`   | Compile then trace each ELF and convert to RVVI format|
| `make clean`   | Remove build artifacts for all targets                |

Each ELF runs independently and sequentially. If any ELF fails (returns non-zero), the sequence stops immediately.

---

## Shared Object Files

Object files appearing in multiple `_OBJS` variables (e.g., `pmp_reset.o`) are compiled only once by Make, then linked into all ELFs that reference them. This is safe because:

- All ELFs share identical `CFLAGS` / `MARCH` / `ABI`
- Shared .o files do not contain `.test_table` sections (they do not invoke `TEST_REGISTER`)
- Linker scripts use `mcmodel=medany`, making them position-independent

---

## Migration Guide

Converting an existing single-target extension to multi-target:

1. **Rename** `main.c` → `main_<group>.c` (or keep the original file as one of the groups)
2. **Rename** `tests/test_register.c` → `tests/register_<group>.c`
3. **Split** the registration file into multiple files, each `#include`-ing the test files for its corresponding group
4. **Create** additional `main_<group>.c` files (copy and modify the banner title)
5. **Modify** the `Makefile`:
   - Replace `TARGET = ...` / `EXT_OBJS = ...` with
   - `TARGETS = ...` and a `_OBJS` variable for each target
6. **Done** — No modifications needed to `kernel.ld`, `test_framework.h`, or any code in the common directory

---

## Design Decisions

| Decision Point         | Choice                                          | Rationale                                      |
|------------------------|-------------------------------------------------|------------------------------------------------|
| Backward Compatibility | `ifdef TARGETS` conditional branching           | Existing extensions work without any changes   |
| Output Location        | All ELFs placed flat in the extension directory | Meets the "keep in same folder" requirement    |
| Compilation Parameters | All ELFs inherit identical CFLAGS/LDFLAGS/MARCH | Ensures consistency, avoids ABI mismatches     |
| Linker Script          | Share a single kernel.ld                        | `.test_table` collection naturally scopes to linked objects |
| Rule Generation Method | `$(eval $(call ...))` template expansion        | Standard GNU Make practice, good maintainability |
| Simulator Iteration    | Shell for loop + `|| exit 1`                    | Simple and reliable, stops on failure          |
