**[中文](../framework/vm_framework.md) | English**

# RISC-V Virtual Memory Framework

## Overview

This document describes the virtual memory management framework for RISC-V privilege testing, including page table management, satp CSR control, and test execution APIs.

The framework supports three virtual memory modes:
- **Sv39**: 3-level page table, 39-bit virtual address
- **Sv48**: 4-level page table, 48-bit virtual address
- **Sv57**: 5-level page table, 57-bit virtual address

---

## Architecture

### Page Table Structure

RISC-V uses a multi-level radix tree page table structure. Each Page Table Entry (PTE) is 64 bits, containing physical page number (PPN), permission flags, and extension fields.

```
Sv39: VPN[2] | VPN[1] | VPN[0] | offset
       9bit     9bit     9bit    12bit   = 39-bit VA

Sv48: VPN[3] | VPN[2] | VPN[1] | VPN[0] | offset
       9bit     9bit     9bit     9bit    12bit   = 48-bit VA

Sv57: VPN[4] | VPN[3] | VPN[2] | VPN[1] | VPN[0] | offset
       9bit     9bit     9bit     9bit     9bit    12bit   = 57-bit VA
```

### Supported Page Sizes

| Mode | Level | Page Size | PPN Bits Used |
|------|-------|-----------|---------------|
| Sv39/Sv48/Sv57 | L0 | 4KB | PPN[0] |
| Sv39/Sv48/Sv57 | L1 | 2MB (megapage) | PPN[1:0] |
| Sv39/Sv48/Sv57 | L2 | 1GB (gigapage) | PPN[2:0] |
| Sv48/Sv57 | L3 | 512GB | PPN[3:0] |
| Sv57 | L4 | 256TB | PPN[4:0] |

> **Note**: The current framework primarily uses 4KB, 2MB, and 1GB page sizes. Larger pages are reserved for future extensions.

### satp CSR

The `satp` (Supervisor Address Translation and Protection) CSR controls virtual memory mode and root page table location:

```
satp (64-bit):
+--------+------+------------------+
| MODE   | ASID | PPN              |
+--------+------+------------------+
  4bit     16bit    44bit

MODE values:
  0  = Bare (no translation)
  8  = Sv39
  9  = Sv48
  10 = Sv57
```

After writing to `satp`, a `sfence.vma` instruction must be executed to flush the TLB.

---

## Core Data Structures

### pt_context_t

Page table context, maintaining per-test mapping state:

```c
typedef struct {
    int         mode;      /* satp mode (SV39/SV48/SV57)           */
    uintptr_t   root_ppn;  /* Root page table physical page number */
    int         levels;    /* Number of page table levels (3/4/5)  */
    uintptr_t   map_base;  /* Identity mapping base address        */
    uintptr_t   map_size;  /* Identity mapping size                */
    int         map_level; /* Page level used for identity mapping */
} pt_context_t;
```

---

## API Reference

### Page Table Management

#### pt_init

Initialize page table context and allocate root page table from the page table pool.

```c
void pt_init(pt_context_t *ctx, int mode);
```

- **ctx**: Page table context to initialize
- **mode**: satp mode, valid values:
  - `SATP_MODE_SV39` (8) — 3-level page table, 39-bit virtual address
  - `SATP_MODE_SV48` (9) — 4-level page table, 48-bit virtual address
  - `SATP_MODE_SV57` (10) — 5-level page table, 57-bit virtual address

#### pt_map_page

Map a single page (4KB / 2MB / 1GB). Automatically creates intermediate page table pages.

```c
int pt_map_page(pt_context_t *ctx, uintptr_t va, uintptr_t pa,
                uintptr_t flags, int level);
```

- **ctx**: Page table context
- **va**: Virtual address (must be aligned to target page size)
- **pa**: Physical address (must be aligned to target page size)
- **flags**: PTE flag combination (`PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D`, etc.)
- **level**: Page level
  - `PT_LEVEL_4K` (0) — 4KB page
  - `PT_LEVEL_2M` (1) — 2MB megapage
  - `PT_LEVEL_1G` (2) — 1GB gigapage
- **Return value**: 0 on success, -1 on failure

#### pt_setup_identity_mapping

Create identity mapping (VA = PA). Automatically maps UART I/O region to support printf in S-mode.

```c
int pt_setup_identity_mapping(pt_context_t *ctx, uintptr_t base,
                              uintptr_t size, uintptr_t flags, int level);
```

- **ctx**: Page table context
- **base**: Base physical address
- **size**: Mapping region size
- **flags**: PTE flags
- **level**: Page level (`PT_LEVEL_4K` / `PT_LEVEL_2M` / `PT_LEVEL_1G`)
- **Return value**: 0 on success, -1 on failure

#### pt_pool_reset

Reset page table pool allocator. All allocated page table pages become invalid.

```c
void pt_pool_reset(void);
```

#### pt_destroy

Release page table context resources (does not reset page table pool).

```c
void pt_destroy(pt_context_t *ctx);
```

#### pt_dump

Print valid PTE entries in root page table (for debugging).

```c
void pt_dump(pt_context_t *ctx);
```

#### pt_get_pte

Get pointer to PTE at specified virtual address and page table level. Used for manual modification of PTE fields (e.g., PBMT).

```c
uintptr_t *pt_get_pte(pt_context_t *ctx, uintptr_t va, int level);
```

- **ctx**: Page table context
- **va**: Virtual address
- **level**: Target page table level (`PT_LEVEL_4K` / `PT_LEVEL_2M` / `PT_LEVEL_1G`)
- **Return value**: PTE pointer, or NULL if PTE at target level does not exist

#### get_pt_page_addr

Get physical address of page table page containing PTE for specified virtual address.

```c
uintptr_t get_pt_page_addr(pt_context_t *ctx, uintptr_t va, int target_level);
```

- **ctx**: Page table context
- **va**: Virtual address
- **target_level**: Page table level (0=L0 page table page, 1=L1 page table page, 2=root page table)
- **Return value**: Physical address of page table page, 0 on error

---

### satp Control

#### vm_enable

Enable virtual memory (write satp CSR and execute sfence.vma).

```c
void vm_enable(pt_context_t *ctx, unsigned asid);
```

- **ctx**: Page table context with configured mappings
- **asid**: Address Space Identifier (0 means no ASID)

#### vm_disable

Disable virtual memory (set satp to Bare mode and flush TLB).

```c
void vm_disable(void);
```

#### vm_sfence_vma

Execute sfence.vma instruction to flush TLB.

```c
void vm_sfence_vma(uintptr_t vaddr, uintptr_t asid);
```

- **vaddr**: Virtual address to flush (0 means all)
- **asid**: ASID to flush (0 means all)

#### vm_switch_mode

Switch to different Sv mode. Disables VM, reinitializes page table, and rebuilds identity mapping.

```c
void vm_switch_mode(pt_context_t *ctx, int new_mode);
```

---

### Advanced Execution APIs

#### vm_run_in_smode

Execute function in S-mode with virtual memory enabled. This is the most commonly used test entry point.

```c
uintptr_t vm_run_in_smode(pt_context_t *ctx,
                           uintptr_t (*fn)(uintptr_t),
                           uintptr_t arg);
```

- **ctx**: Page table context (must have identity mapping configured)
- **fn**: Function to execute in S-mode
- **arg**: Argument passed to fn
- **Return value**: Return value of fn

**Internal execution flow**:

1. Configure PMP entry 15 as full address space NAPOT RWX (allow S-mode access)
2. Configure trap delegation (page fault → S-mode)
3. Set stvec to point to S-mode trap handler
4. Enable virtual memory (write satp + sfence.vma)
5. Switch to S-mode via `run_in_priv(PRIV_S, fn, arg)` to execute fn
6. After fn returns, disable VM, restore medeleg, clear PMP entry

---

#### vm_run_in_umode

Execute function in U-mode with virtual memory enabled. Used for tests requiring U-mode execution such as Ssnpm (Pointer Masking).

```c
uintptr_t vm_run_in_umode(pt_context_t *ctx,
                           uintptr_t (*fn)(uintptr_t),
                           uintptr_t arg);
```

- **ctx**: Page table context (must have identity mapping configured)
- **fn**: Function to execute in U-mode
- **arg**: Argument passed to fn
- **Return value**: Return value of fn

**Differences from `vm_run_in_smode`:**
- Uses `mret` to jump directly from M-mode to U-mode (MPP=0)
- Caller must set `PTE_U` flag in page table mapping
- Function returns to M-mode via ecall (CAUSE_ECALL_FROM_U, cause 8)
- Does not delegate page fault to S-mode (captured by M-mode trap_record)

---

## PTE Flags

| Flag | Value | Description |
|------|-------|-------------|
| `PTE_V` | bit 0 | Valid |
| `PTE_R` | bit 1 | Read permission |
| `PTE_W` | bit 2 | Write permission |
| `PTE_X` | bit 3 | Execute permission |
| `PTE_U` | bit 4 | User-accessible (U-mode) |
| `PTE_G` | bit 5 | Global mapping |
| `PTE_A` | bit 6 | Accessed |
| `PTE_D` | bit 7 | Dirty |

**Svpbmt Extension Fields (PTE bits 62-61):**

| Constant | Value | Description |
|----------|-------|-------------|
| `PBMT_PMA` | 00 | Use underlying PMA (Physical Memory Attributes) |
| `PBMT_NC` | 01 | Non-cacheable, idempotent, RVWMO |
| `PBMT_IO` | 10 | Non-cacheable, non-idempotent, I/O ordering |
| `PBMT_RSVD` | 11 | Reserved (triggers page fault) |

Extraction macro: `PTE_PBMT(pte)` returns PBMT field value.

**Common Combinations:**

| Macro | Value | Description |
|-------|-------|-------------|
| `PTE_RWX` | R\|W\|X | Read-Write-Execute |
| `PTE_RWXU` | R\|W\|X\|U | Read-Write-Execute + U-mode |
| `PTE_RWXAD` | R\|W\|X\|A\|D | Read-Write-Execute + Accessed + Dirty |

> **Note**: Setting `PTE_A` and `PTE_D` avoids hardware-triggered page faults to set these bits. It is recommended to always set `PTE_A | PTE_D` in tests.

---

## Page Table Pool

Page table pages are allocated from the `.page_tables` section statically reserved in the linker script, using a bump allocator:

- **Capacity**: 64 pages × 4KB = 256KB
- **Allocation**: `pt_alloc_page()` returns a zeroed 4KB-aligned page
- **Reset**: `pt_pool_reset()` resets allocation pointer to start position

Relevant section definitions in linker script:

```ld
/* Page table pool */
. = ALIGN(0x1000);
.page_tables (NOLOAD) : {
    PROVIDE(__page_tables_start = .);
    . += 64 * 4096;    /* 256KB = 64 pages */
    PROVIDE(__page_tables_end = .);
}

/* S-mode stack */
. = ALIGN(16);
.smode_stack (NOLOAD) : {
    PROVIDE(__smode_stack_start = .);
    . += 16 * 1024;    /* 16KB S-mode stack */
    . = ALIGN(16);
    PROVIDE(__smode_stack_end = .);
}
```

---

## Build Configuration

### Makefile Configuration

Set `ENABLE_VM = 1` in extended Makefile to enable VM support:

```makefile
# Sv39/Makefile (mode-intrinsic: pinned to Sv39)
TARGET = sv39_test.elf
ENABLE_VM = 1
EXT_OBJS = \
    main.o \
    tests/test_register.o
include ../common/Makefile.common

# S-stage paging mode: Sv39 (appended after include, so the per-dir -D wins)
CFLAGS += -DSUITE_SATP_MODE=SATP_MODE_SV39
```

Mode-agnostic Sv\* extension suites instead follow the platform via `?=`:

```makefile
# e.g. Svadu/Makefile (mode-agnostic)
TARGET = svadu_test.elf
ENABLE_VM = 1
include ../common/Makefile.common
SUITE_SATP_MODE ?= PLATFORM_SATP_MODE
```

`ENABLE_VM = 1` automatically:
- Links `common/vm/page_table.o` and `common/vm/satp.o`
- Adds `-DENABLE_VM` compile macro
- Adds `-I$(VM_DIR)` header search path

> The full semantics, priority and per-mode derived macros of `SUITE_SATP_MODE` are described in "Paging Mode Configuration" below.

### Compile and Run

```bash
# Compile single test
make Sv39          # or make Sv48 / make Sv57

# Compile all tests (including pmp, smepmp, Sv39, Sv48, Sv57)
make

# Run on QEMU
cd Sv39 && make qemu

# Or use qemu-system-riscv64 directly
qemu-system-riscv64 -machine virt -nographic -bios none -kernel Sv39/sv39_test.elf

# Clean
make clean
```

---

## Paging Mode Configuration

For single-stage (non-Hypervisor) VM suites, the S-stage paging mode is determined at compile time by two cooperating layers: the platform capability `PLATFORM_SATP_MODE` and the suite selection `SUITE_SATP_MODE`.

### Suite Organization: Shared Sources for Sv39/Sv48/Sv57

`Sv39/`, `Sv48/` and `Sv57/` are a single shared code base; only the `-DSUITE_SATP_MODE` in each directory's Makefile selects the paging mode (mirroring the two-stage `Sv39x4/`, `Sv48x4/`, `Sv57x4/` suites). `Sv39/` is the master holding the real files; `Sv48/` and `Sv57/` reuse `tests/*`, `main.c` and `kernel.ld` via symlinks (shown in git as file mode `100644 -> 120000`), keeping only their own Makefile.

Other **mode-agnostic** Sv\* extension suites that use VM as their working environment (`Svade/`, `Svadu/`, `Svinval/`, `Svnapot/`, `Svpbmt/`, `Svvptc/`, `Svrsw60t59b/`, etc.) also build on this framework, but each keeps its own sources and follows the platform via `SUITE_SATP_MODE ?= PLATFORM_SATP_MODE`.

### Platform Capability: PLATFORM_SATP_MODE

`common/capabilities.h` (force-included into every translation unit; preprocessor-only, asm-safe) derives `SV39/48/57_AVAILABLE` from the platform config (`SV39/48/57_SUPPORTED` in `config/<platform>/rvtest_config.h`), then selects the platform's default usable paging mode with a "smallest-first" policy (Sv39 > Sv48 > Sv57):

- `PLATFORM_SATP_MODE`: the S-stage mode, using SPEC encoding literals (Bare=0 / Sv39=8 / Sv48=9 / Sv57=10; rv32 fixed to Sv32=1).
- `PLATFORM_SATP_MODE` is purely platform-derived and **never force-overridden**; it represents "the default mode this platform can use".

> `capabilities.h` **deliberately provides no** `#ifndef SUITE_SATP_MODE` fallback: it is force-included before every `test_helpers.h`, so a fallback there would preempt the mode-intrinsic suites' (Sv39/48/57) mode definition and silently collapse Sv48/Sv57 to Sv39.

### Suite Selection: SUITE_SATP_MODE

The S-stage mode a suite actually runs is expressed by `SUITE_SATP_MODE`, decoupled from the platform capability. Two conventions:

| Kind | Makefile form | Examples |
|------|---------------|----------|
| **Mode-agnostic** | `SUITE_SATP_MODE ?= PLATFORM_SATP_MODE` (follow platform) | Svade / Svadu / Svinval / Svnapot / Svpbmt / Svvptc / Ssccptr / Sstvala / Sstvecd / Svbare / Ss_CSR / Ss_Exceptions / Svrsw60t59b |
| **Mode-intrinsic** | `CFLAGS += -DSUITE_SATP_MODE=SATP_MODE_SV39/48/57` (pin a specific mode) | Sv39 / Sv48 / Sv57 |

All working-mode call sites — `pt_init()`, `MAKE_SATP()`, `REQUIRE_SATP_MODE()` gates — pass `SUITE_SATP_MODE`, never `PLATFORM_SATP_MODE` or a literal mode. Only cases whose *subject* is a specific mode's WARL behavior (e.g. the satp reserved-MODE enumeration, or the target mode of a cross-mode switch) keep explicit constants.

### Command-line Override and Priority

`common/Makefile.common` provides a diagnostic knob to temporarily rewrite the suite mode at build time:

```
make SATP_MODE=sv39|sv48|sv57     # -> -DSUITE_SATP_MODE=SATP_MODE_SV*
```

Priority: `command-line knob > suite Makefile's SUITE_SATP_MODE ?= default > test_helpers.h #ifndef fallback`.

> Note: for the mode-intrinsic suites (Sv39/48/57) the `-DSUITE_SATP_MODE=...` is a **bare `CFLAGS +=`** appended *after* `include ../common/Makefile.common`, so it comes after the `-D` emitted by the command-line knob; for a duplicate macro the later one wins, hence `make SATP_MODE=` does not change these three suites' mode (consistent with the two-stage Sv\*x4 suites). Mode-agnostic suites use a `?=` variable, which the command-line `:=` overrides normally.

### Per-mode Derived Macros (Sv39/48/57 shared sources)

Because Sv39/48/57 share the same `Sv39/tests/` sources, every mode-varying element is derived from `SUITE_SATP_MODE` in `Sv39/tests/test_helpers.h`, keeping the test bodies byte-identical:

| Macro | Sv39 | Sv48 | Sv57 | Purpose |
|-------|------|------|------|---------|
| `SUITE_MODE_NAME` | "Sv39" | "Sv48" | "Sv57" | mode name in banner / TEST_BEGIN |
| `SV_FN_BASE` | `test_sv39_` | `test_sv48_` | `test_sv57_` | test function name prefix |
| `ID_MAP_1G/2M/4K` | MAP-01/02/03 | MAP-05/06/07 | MAP-08/09/10 | unified `vm_test_plan.md` ID space |
| `ID_SIGN_NONCANON` | SIGN-03 | SIGN-05 | SIGN-07 | non-canonical VA case ID |
| `ID_WALK_FULL` | WALK-01 | WALK-02 | WALK-03 | full-walk case ID |
| `ID_SATP_ENABLE` | SATP-02 | SATP-03 | SATP-04 | enable-VM case ID |
| `ID_SATP_SWITCH` | SATP-05 | SATP-06 | (none) | mode-switch case ID |
| `SUITE_NONCANON_VA` | bit39 boundary | bit48 boundary | bit57 boundary | non-canonical VA constant |
| `SUITE_ROOT_VPN` / `SUITE_ROOT_LEVEL_STR` | VA_VPN2 / "L2" | VA_VPN3 / "L3" | VA_VPN4 / "L4" | root-level PTE check |
| `SUITE_WALK_DEPTH` | "three-level" | "four-level" | "five-level" | walk-depth wording |
| `SUITE_NEXT_MODE(_NAME)` | Sv48 | Sv57 | (none) | mode-switch target |

Two companion macro utilities:
- `SVFN(name)`: token-pastes `test_svXX_##name`, preserving a distinct symbol name per mode.
- `SV_REGISTER(name)`: a local equivalent of `TEST_REGISTER`. Required because `TEST_REGISTER(fn)`'s internal `fn##_ptr` paste suppresses expansion of a function-like `SVFN(x)` argument, so passing it directly would fail to compile.

Sv57 is the highest mode with no higher mode to switch to, so the mode-switch case is compiled out with `#if SUITE_SATP_MODE != SATP_MODE_SV57` (Sv57 has one fewer case than Sv39/Sv48: 49 vs 50).

### Paging Mode Gate Macro (REQUIRE_SATP_MODE)

`common/hyp/hyp_test.h` provides the compile-time gate `REQUIRE_SATP_MODE(mode)`: whether `mode` is supported comes from the config-declared `SV39/48/57_AVAILABLE`; if unsupported it does `TEST_SKIP`, with **no runtime WARL probe**.

The single-stage Sv39/48/57 suites do not include the Hypervisor headers, so an equivalent local `REQUIRE_SATP_MODE` is defined in `Sv39/tests/test_helpers.h` (guarded by `#ifndef`), depending only on the force-included `SV39/48/57_AVAILABLE` and `TEST_SKIP` from `test_framework.h`. For example, the "enable VM" case uses `REQUIRE_SATP_MODE(SUITE_SATP_MODE)` to decide whether to run, replacing the former Sv48-only runtime `satp.MODE` probe.

---

## Writing Test Cases

### Basic Pattern

Each test case follows this pattern:

```c
#include "test_framework.h"
#include "vm/vm.h"

/* Test function executed in S-mode */
static uintptr_t my_smode_test(uintptr_t arg) {
    /* Executed with S-mode + VM enabled */
    volatile uintptr_t *ptr = (volatile uintptr_t *)arg;
    ptr[0] = 0xDEADBEEF;
    if (ptr[0] != 0xDEADBEEF)
        return 1;  /* Failure */
    return 0;      /* Success */
}

TEST_REGISTER(test_my_vm_test);
bool test_my_vm_test(void) {
    TEST_BEGIN("MY-01: description");

    /* 1. Reset page table pool and initialize context */
    pt_context_t ctx;
    pt_pool_reset();
    pt_init(&ctx, SUITE_SATP_MODE);  /* suite working mode; or pass SATP_MODE_SV39/48/57 explicitly */

    /* 2. Setup identity mapping */
    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
    int ret = pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                                        flags, PT_LEVEL_1G);
    TEST_ASSERT("identity mapping setup", ret == 0);

    /* 3. Execute test in S-mode + VM */
    uintptr_t result = vm_run_in_smode(&ctx, my_smode_test,
                                        (uintptr_t)test_data);
    TEST_ASSERT("S-mode test passed", result == 0);

    /* 4. Cleanup */
    pt_pool_reset();
    TEST_END();
}
```

### Mapping Examples for Different Page Sizes

#### 1GB gigapage

```c
uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
pt_setup_identity_mapping(&ctx, base, PAGE_SIZE_1G,
                          flags, PT_LEVEL_1G);
```

#### 2MB megapage

```c
uintptr_t base = PLATFORM_MEM_BASE;
uintptr_t size = 16 * PAGE_SIZE_2M;  /* 32MB, covering code+data+stack+page tables */
pt_setup_identity_mapping(&ctx, base, size,
                          flags, PT_LEVEL_2M);
```

#### 4KB page

```c
uintptr_t base = PLATFORM_MEM_BASE;
uintptr_t size = 2 * PAGE_SIZE_2M;   /* 4MB */
pt_setup_identity_mapping(&ctx, base, size,
                          flags, PT_LEVEL_4K);
```

> **Note**: When using 4KB pages, larger mapping regions consume more page table pool pages. A 4MB region requires approximately 3 page table pages (1 L2 + 1 L1 + 1024 L0 PTEs distributed across 2 L0 pages).

### Manual Single Page Mapping

```c
/* Map single 4KB page */
pt_map_page(&ctx, 0x80100000, 0x80100000,
            PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
            PT_LEVEL_4K);

/* Map MMIO region (no execute permission) */
pt_map_page(&ctx, 0x10000000, 0x10000000,
            PTE_V | PTE_R | PTE_W | PTE_A | PTE_D,
            PT_LEVEL_4K);
```

### Mode Switching

```c
pt_context_t ctx;
pt_pool_reset();
pt_init(&ctx, SATP_MODE_SV39);

/* Setup Sv39 identity mapping */
pt_setup_identity_mapping(&ctx, base, size, flags, PT_LEVEL_1G);

/* Run Sv39 test */
vm_run_in_smode(&ctx, test_fn, arg);

/* Switch to Sv48 (automatically rebuilds identity mapping) */
vm_switch_mode(&ctx, SATP_MODE_SV48);

/* Run Sv48 test */
vm_run_in_smode(&ctx, test_fn, arg);
```

---

## Existing Test Cases

Sv39/Sv48/Sv57 share the same set of cases under `Sv39/tests/`, organized by Group; the full list and pass criteria are in the test plan `DOCS/testplan/vm_test_plan.md`. Each mode's test IDs come from the plan's unified ID space, selected by the per-mode derived macros in `test_helpers.h`:

| Group | File | Cases | Sv39 ID | Sv48 ID | Sv57 ID |
|-------|------|-------|---------|---------|---------|
| 1 Basic mapping | test_mapping.c | 1G/2M/4K identity mapping | MAP-01/02/03 | MAP-05/06/07 | MAP-08/09/10 |
| 2 Sign extension | test_mapping.c | non-canonical VA -> page fault | SIGN-03 | SIGN-05 | SIGN-07 |
| 3 PTE validity | test_pte_valid.c | V=0 / R=0&W=1 | VALID-01~05 | same | same |
| 4 Permission bits | test_pte_perms.c | RWX combinations | RWX-01~05 | same | same |
| 5 U-bit | test_upriv.c | U/S-mode access | UPRIV-03~07 | same | same |
| 6 SUM | test_upriv.c | SUM control | SUM-01~05 | same | same |
| 7 MXR | test_mxr.c | MXR control | MXR-01~05 | same | same |
| 8 Alignment | test_superpage.c | superpage misalign | ALIGN-01~02 | same | same |
| 9 Walk | test_superpage.c | full walk + non-leaf/intermediate leaf | WALK-01/04/05 | WALK-02/04/05 | WALK-03/04/05 |
| 10 A/D bits | test_ad_bits.c | A/D management | AD-01~05 | same | same |
| 11 satp | test_satp.c | Bare/enable/switch/ASID/reserved MODE | SATP-01/02/05/07/09 | SATP-01/03/06/07/09 | SATP-01/04/07/09 |
| 12 SFENCE | test_sfence_rsvd.c | sfence.vma takes effect | SFENCE-01/04/05 | same | same |
| 13 Reserved bits | test_sfence_rsvd.c | PTE reserved bits | RSVD-01~02 | same | same |
| 14 Non-leaf PTE | test_sfence_rsvd.c | non-leaf D=A=U=0 | NLPTE-04 | same | same |

Total cases: **Sv39 = 50, Sv48 = 50, Sv57 = 49** (Sv57 has no mode-switch case SATP-05/06 since it is the highest mode).

Typical verification method per case:
1. `pt_pool_reset()` + `pt_init(&ctx, SUITE_SATP_MODE)` to initialize the page table context
2. `pt_setup_identity_mapping()` / `pt_map_page()` to build mappings
3. Execute read/write/exec tests in S/U-mode via `vm_run_in_smode()` / `vm_run_in_umode()`
4. Use `TEST_ASSERT` to check the return value (success / expected page-fault cause)

---

## Key Considerations

### Identity Mapping Coverage

When using `pt_setup_identity_mapping()`, the mapping region must cover:
- **Code segment** (.text) — for instruction fetch
- **Data segment** (.data / .bss) — for global variable access
- **Stack** (.stack) — for function calls
- **Page table pool** (.page_tables) — for page table lookup
- **Test data region** — for test read/write operations

`pt_setup_identity_mapping()` automatically maps additional UART I/O region (`PLATFORM_UART0_BASE`) to support printf output in S-mode.

### PTE_A and PTE_D Bits

RISC-V specification allows hardware to automatically set A (Accessed) and D (Dirty) bits on first access, and also allows hardware to trigger page fault when A/D bits are not set. To avoid unnecessary page faults, it is recommended to always set `PTE_A | PTE_D` during mapping.

### PMP and Virtual Memory

`vm_run_in_smode()` automatically configures PMP entry 15 as full address space NAPOT RWX to allow S-mode access to all physical memory. This PMP entry is automatically cleared after function returns.

If tests need to verify interaction behavior between PMP and VM simultaneously, manual PMP configuration management is required without using `vm_run_in_smode()`.

### Page Table Pool Capacity

Page table pool has 64 pages (256KB) total. Page table consumption for different mapping methods:

| Mapping Method | Consumption Per Mapping | Description |
|----------------|------------------------|-------------|
| 1GB gigapage | 1 page (root page table) | Install leaf PTE directly in root page table |
| 2MB megapage | 1-2 pages | Root page table + 1 L1 page table page |
| 4KB page | 2-3+ pages | Root page table + L1 page table page + L0 page table pages |

> **Note**: `pt_init()` allocates 1 page as root page table. UART mapping consumes additional 1-2 pages (depending on mode levels).

---

## References

- RISC-V Privileged Specification 4.3-4.5 (Sv39/Sv48/Sv57 Virtual Memory Systems)
- `SPEC/supervisor.adoc` — Supervisor-Level Specification
- `SPEC/machine.adoc` — Machine-Level Specification (satp CSR definition)
- `common/vm/vm.h` — API declarations
- `common/vm/vm_defs.h` — Constants and macro definitions
