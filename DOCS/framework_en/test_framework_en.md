**[中文](../framework/test_framework.md) | English**

## Test Common Framework Developer Guide

### Overview

This document describes the **common infrastructure** of the RISC-V privilege test framework, i.e., the core components under the `common/` directory that are extension-agnostic. All extensions (PMP, VM, Hypervisor, ZPM, etc.) are built on top of this common framework.

**Core Features:**

- Bare-metal execution, no OS dependency
- Supports both **RV32 / RV64** architectures
- Supports **M-mode / S-mode / U-mode** three-level privilege switching
- Exception-driven testing model: safely tests access control via trap arming mechanism
- `TEST_REGISTER` macro automatically registers test cases, collected at link time
- Deterministic trap handling: all memory operations use uncompressed instructions (`.option norvc`), ensuring reliable `mepc += 4` to skip faulting instructions
- Conditionally compiled common libraries: extensions link PMP / VM / HYP modules as needed

**Scope of this document:** Covers only the common test framework (`test_framework.h/c`, `privilege.c`, `trap.c`, `mem_ops.h`, etc.), excluding extension-specific frameworks such as PMP, VM, and Hypervisor. For extension frameworks, please refer to the corresponding documentation:

| Extension Framework | Documentation |
|---------------------|---------------|
| PMP | [`pmp_test_framework.md`](pmp_test_framework.md) |
| Smepmp | [`smepmp_test_framework.md`](smepmp_test_framework.md) |
| SPMP | [`spmp_test_framework.md`](spmp_test_framework.md) |
| Virtual Memory | [`vm_test_framework.md`](vm_test_framework.md) |
| Hypervisor | [`hypervisor_framework.md`](hypervisor_framework.md) |
| ZPM | [`zpm_framework.md`](zpm_framework.md) |

---

### Architecture Design

#### File Structure
```
common/
├── test_framework.h      # Core test framework header (macros, types, API declarations)
├── test_framework.c      # Test result tracking, reset_state, test_print_summary
├── trap.c                # Trap handling logic (arm/disarm, cause recording)
├── trap_asm.S            # Trap entry assembly (context save/restore)
├── privilege.c           # Privilege level switching (goto_priv, run_in_priv)
├── entry.S               # M-mode boot sequence, BSS initialization, _platform_init hook
├── platform_init.S       # Default weak symbol implementation of _platform_init
├── mem_ops.h             # Memory load/store/execute/AMO primitives (norvc)
├── csr_accessors.c       # Dynamic CSR read/write by index
├── types.h               # Basic type definitions (uintptr_t, bool, etc.)
├── encoding.h            # CSR numbers, exception cause codes, mstatus bitfield constants
├── uart.h / uart.c       # UART driver (printf implementation)
├── Makefile.common       # Shared build rules, toolchain configuration
└── config/               # Platform configuration directory
    ├── qemu-rv64-max/    # QEMU virt machine (rv64,max CPU)
    ├── sail-rv64-max/    # Sail simulator
    ├── spike-rv64-max/   # Spike simulator
    └── haps_*/           # FPGA/HAPS platforms
```

#### Execution Flow

```
_entry (entry.S)
  │
  ├── Clear BSS
  ├── Set stack pointer
  ├── Call _platform_init (weak symbol hook)
  ├── Set mtvec → m_trap_entry
  │
  └── Jump to main()
        │
        ├── Print test banner
        ├── Iterate through function pointers in .test_table section
        │     ├── test_fn_1() → TEST_BEGIN → test logic → TEST_END
        │     ├── test_fn_2() → ...
        │     └── ...
        └── test_print_summary() → print summary → wfi halt
```

#### Core Design Principles

1. **Zero coupling between common/ and extensions** — `common/` does not `#include` any extension headers; extensions inject behavior via weak symbols and link-time composition
2. **Each extension compiles to an independent bare-metal binary** — Fully self-contained ELF that can be directly loaded by QEMU / Sail / Spike
3. **Platform abstraction via `-include`** — Platform headers are injected at compile time via GCC `-include` flag
4. **Deterministic Trap Handling** — Uncompressed instructions ensure trap handler can reliably skip faulting instructions

---

### Test Lifecycle

| Macro | Description |
|-------|-------------|
| `TEST_REGISTER(fn)` | Register test function (automatically collected via `.test_table` linker section) |
| `TEST_BEGIN(name)` | Start test: print name, reset current test state |
| `TEST_END()` | End test: restore M-mode, reset state, print PASS/FAIL, return result |
| `TEST_SKIP(reason)` | Skip test: print reason, return success (skip is not failure) |
| `TEST_FATAL(reason)` | Terminate test: unreachable path, print reason, explicit failure |

#### TEST_REGISTER — Register Test Function

```c
#define TEST_REGISTER(test_fn) \
    bool test_fn(void); \
    static test_func_t test_fn##_ptr \
        __attribute__((section(".test_table"), used)) = test_fn
```

Places the test function pointer into the `.test_table` linker section, collected by the linker script. `main.c` iterates through this section to automatically execute all registered tests.

**Usage:**

```c
TEST_REGISTER(test_my_feature);
bool test_my_feature(void) {
    TEST_BEGIN("My feature description");
    // ... test logic ...
    TEST_END();
}
```

#### TEST_BEGIN — Start Test

```c
#define TEST_BEGIN(name) do { \
    test_results.current_test_name = (name); \
    test_results.current_test_failed = false; \
    printf("[TEST] %s\n", (name)); \
} while (0)
```

Initializes current test state and prints test name. Should be the first statement in each test function.

#### TEST_END — End Test

```c
#define TEST_END() do { \
    goto_priv(PRIV_M); \
    reset_state(); \
    if (test_results.current_test_failed) { ... printf("[FAIL]"); } \
    else { ... printf("[PASS]"); } \
    return !test_results.current_test_failed; \
} while (0)
```

Automatically restores M-mode, resets hardware state, tallies results, prints PASS/FAIL, and returns.

> **Important**: `TEST_END()` contains a `return` statement internally; do not write any code after it.

#### TEST_SKIP — Skip Test

```c
#define TEST_SKIP(reason)
```

Skips the current test (e.g., when hardware does not support a feature), prints `[SKIP]`, and returns true (skip is not failure).

**Usage:**

```c
bool test_svnapot(void) {
    TEST_BEGIN("Svnapot 64KB page");
    if (!detect_svnapot()) {
        TEST_SKIP("Svnapot not supported");
    }
    // ...
    TEST_END();
}
```

#### TEST_FATAL — Unconditional Failure

```c
#define TEST_FATAL(reason)
```

Unconditionally marks the current test as failed. Used for code paths that should not be reached, or scenarios requiring explicit failure marking.

```c
/* Should not reach here — fail if trap was not triggered */
if (!trap_was_triggered()) {
    TEST_FATAL("expected trap but none occurred");
}

/* Illegal value in switch */
default:
    TEST_FATAL("unexpected privilege level");
    break;
```

---

### Assertion Macros

All assertion macros can only be used in **M-mode** (because they require UART output access). When testing in S/U-mode, use the `PRIV_DO_*` + `CHECK_*` pattern instead.

#### Assertion Macro Behavior Summary

| Macro | Failure Output Format | Use Case |
|-------|----------------------|----------|
| `TEST_ASSERT(msg, cond)` | `ASSERT FAIL: msg (file:line)` | General boolean condition |
| `TEST_ASSERT_EQ(msg, actual, expected)` | `got 0xA, expected 0xB` | Value equality comparison |
| `TEST_ASSERT_NEQ(msg, actual, not_expected)` | `got 0xA, should NOT equal 0xA` | Value change verification |
| `TEST_ASSERT_BITS(msg, value, mask, expected)` | `value=0xF, mask=0x3, got field 0x1, expected field 0x3` | CSR bitfield check |


#### TEST_ASSERT — Boolean Condition Assertion

```c
#define TEST_ASSERT(msg, cond)
```

Checks a boolean condition. Prints message and source file location on failure.

```c
TEST_ASSERT("value is non-zero", result != 0);
TEST_ASSERT("bit 3 is set", (csr_val >> 3) & 1);
```

#### TEST_ASSERT_EQ — Equality Assertion

```c
#define TEST_ASSERT_EQ(msg, actual, expected)
```

Checks that two values are equal. Prints actual and expected values (in hexadecimal) on failure.

```c
TEST_ASSERT_EQ("trap cause", trap_get_cause(), CAUSE_LOAD_ACCESS);
TEST_ASSERT_EQ("return value", result, 0x42);
```

#### TEST_ASSERT_NEQ — Inequality Assertion

```c
#define TEST_ASSERT_NEQ(msg, actual, not_expected)
```

Checks that two values are not equal. Suitable for verifying that state has changed. Prints "got X, should NOT equal X" on failure.

```c
/* Verify mseccfg changed after RLB write */
uintptr_t before = CSRR(mseccfg);
CSRS(mseccfg, MSECCFG_RLB);
uintptr_t after = CSRR(mseccfg);
TEST_ASSERT_NEQ("mseccfg changed after RLB set", after, before);
```

#### TEST_ASSERT_BITS — Bitfield Assertion

```c
#define TEST_ASSERT_BITS(msg, value, mask, expected)
```

Extracts `(value & mask)` and compares with `(expected & mask)`. Suitable for verifying specific bitfields of CSRs. Prints full value, mask, and extracted bitfield on failure.

```c
/* Verify mseccfg.MML bit (bit 0) is set */
uintptr_t mseccfg = CSRR(mseccfg);
TEST_ASSERT_BITS("mseccfg.MML is set", mseccfg, 0x1, 0x1);

/* Verify hstatus.SPV (bit 7) is 0 */
uintptr_t hstatus = CSRR(hstatus);
TEST_ASSERT_BITS("hstatus.SPV cleared", hstatus, (1UL << 7), 0);

/* Verify mstatus.MPP field [12:11] is S-mode (01) */
uintptr_t mstatus = CSRR(mstatus);
TEST_ASSERT_BITS("MPP is S-mode", mstatus, (3UL << 11), (1UL << 11));
```


---

### Exception Testing Macros (M-mode)

The following macros are used to test exception behavior in **M-mode**. They internally call `trap_expect_begin()` / `trap_expect_end()` to complete trap arm/disarm.

| Macro | Description |
|-------|-------------|
| `EXPECT_NO_TRAP(stmt)` | Execute statement, assert no exception triggered |
| `EXPECT_TRAP(cause, stmt)` | Execute statement, assert exception with specified cause triggered |
| `EXPECT_EXEC_NO_TRAP(addr)` | Jump and execute, assert no exception triggered |
| `EXPECT_EXEC_TRAP(cause, addr)` | Jump and execute, assert exception with specified cause triggered |

#### EXPECT_NO_TRAP — Expect No Exception

```c
#define EXPECT_NO_TRAP(stmt)
```

Executes statement and asserts no exception is triggered.

```c
EXPECT_NO_TRAP(mem_load32(valid_addr));
EXPECT_NO_TRAP(CSRR(mstatus));
```

#### EXPECT_TRAP — Expect Specified Exception

```c
#define EXPECT_TRAP(expected_cause, stmt)
```

Executes statement and asserts an exception with the specified cause is triggered.

```c
EXPECT_TRAP(CAUSE_STORE_ACCESS, mem_store32(readonly_addr, 0));
EXPECT_TRAP(CAUSE_ILLEGAL_INST, CSRW(mstatus, 0));  /* from S-mode */
```

#### EXPECT_EXEC_TRAP — Expect Execution Exception

```c
#define EXPECT_EXEC_TRAP(expected_cause, addr)
```

Jumps to address and executes, asserting the specified exception is triggered. Implemented using `exec_at()`, supports trap recovery.

```c
EXPECT_EXEC_TRAP(CAUSE_INST_ACCESS, no_exec_region);
```

#### EXPECT_EXEC_NO_TRAP — Expect Execution Without Exception

```c
#define EXPECT_EXEC_NO_TRAP(addr)
```

Jumps to address and executes, asserting no exception is triggered.

```c
EXPECT_EXEC_NO_TRAP(executable_region);
```

#### M-mode Trap Macros (Smdbltrp Support)

On platforms supporting the Smdbltrp (Double Trap) extension, the `mstatus.MDT` bit must be cleared before triggering synchronous exceptions in M-mode; otherwise, a fatal double trap will occur. The framework provides the following M-mode specific variants:

| Macro | Description |
|-------|-------------|
| `M_TRAP_EXPECT_BEGIN()` | Clears MDT then calls `trap_expect_begin()` |
| `M_EXPECT_TRAP(cause, stmt)` | Clears MDT then executes `EXPECT_TRAP` |
| `M_EXPECT_NO_TRAP(stmt)` | Clears MDT then executes `EXPECT_NO_TRAP` |

**Use Cases:** Probing CSR existence in M-mode, testing PMP Lock bits, and other scenarios requiring expected exception triggering.

```c
/* Probe whether CSR exists */
M_TRAP_EXPECT_BEGIN();
CSRR(menvcfg);  /* Triggers Illegal Instruction if not present */
if (trap_was_triggered()) {
    /* CSR does not exist */
}
trap_expect_end();
```

> **Note:** These macros only clear MDT when `PLATFORM_DOUBLE_TRAP` is defined; otherwise they are no-ops.

#### Trap Query API

After a trap is triggered, detailed information can be queried:

| Function | Return Value | Description |
|----------|--------------|-------------|
| `trap_was_triggered()` | `bool` | Whether a trap was triggered during the most recent arm period |
| `trap_get_cause()` | `uintptr_t` | Exception cause code (mcause) |
| `trap_get_epc()` | `uintptr_t` | Exception PC (mepc) |
| `trap_get_tval()` | `uintptr_t` | Exception additional value (mtval) |
| `trap_get_htval()` | `uintptr_t` | Hypervisor: htval (GPA>>2, requires ENABLE_HYP) |
| `trap_get_htinst()` | `uintptr_t` | Hypervisor: htinst (transformed instruction, requires ENABLE_HYP) |
| `trap_get_gva()` | `bool` | Hypervisor: hstatus.GVA bit (requires ENABLE_HYP) |

---

### S/U-mode Safe Macros

#### Design Background

In RISC-V, UART is an MMIO device protected by PMP in M-mode. When test code runs in S-mode or U-mode, **UART cannot be accessed directly**, meaning `printf` cannot be called. Therefore, macros like `EXPECT_TRAP()` / `EXPECT_NO_TRAP()` that contain `TEST_ASSERT` (→ `printf`) cannot be used in S/U-mode.

The framework solves this problem through the **PRIV_DO + CHECK two-phase pattern**:

1. **Phase 1 (S/U-mode)**: Use `PRIV_DO` / `PRIV_DO_EXEC` to execute operations — only performs "arm trap → execute statement → disarm trap", without any printing or assertions
2. **Phase 2 (M-mode)**: After returning to M-mode, use `CHECK_NO_TRAP` / `CHECK_TRAP` to check trap records and print assertion results

#### PRIV_DO Series Summary

| Macro | Purpose | Execution Environment | Paired With |
|-------|---------|----------------------|-------------|
| `PRIV_DO(stmt)` | Execute load/store/CSR operations | S/U-mode | `CHECK_NO_TRAP` or `CHECK_TRAP` |
| `PRIV_DO_EXEC(addr)` | Test instruction execution permission | S/U-mode | `CHECK_NO_TRAP` or `CHECK_TRAP` |
| `CHECK_NO_TRAP(msg)` | Assert no exception occurred | M-mode | — |
| `CHECK_TRAP(msg, cause)` | Assert specified exception triggered | M-mode | — |

> **Compatibility**: Legacy macro names `PRIV_DO_NO_TRAP` / `PRIV_DO_TRAP` / `PRIV_DO_EXEC_NO_TRAP` / `PRIV_DO_EXEC_TRAP` remain usable (defined as aliases), but new code is recommended to uniformly use `PRIV_DO` / `PRIV_DO_EXEC`. Legacy names are retained to avoid breaking existing extensive test code.

#### Usage Pattern

```c
/* Standard two-phase testing pattern */
goto_priv(PRIV_S);
PRIV_DO(mem_load32(addr));           /* Phase 1: Execute load in S-mode */
PRIV_DO(mem_store32(addr, 0));       /* Phase 1: Execute store in S-mode */
goto_priv(PRIV_M);
CHECK_NO_TRAP("read should succeed");           /* Phase 2: Assert in M-mode */
CHECK_TRAP("write should fault", CAUSE_SAF);    /* Phase 2: Assert in M-mode */
```

**Design Intent**: Safely execute an operation that may trigger a trap in S/U-mode. The macro itself makes no judgment about whether a trap occurred — it only protectively records trap status, letting subsequent `CHECK_*` macros determine expectations.

**Use Cases**:

```c
/* Test load permission */
goto_priv(PRIV_S);
PRIV_DO(mem_load8(protected_addr));
goto_priv(PRIV_M);
CHECK_NO_TRAP("S-mode should be able to read");

/* Test store rejection */
goto_priv(PRIV_U);
PRIV_DO(mem_store32(readonly_addr, 0xBAD));
goto_priv(PRIV_M);
CHECK_TRAP("U-mode write denied", CAUSE_STORE_ACCESS_FAULT);

/* Test AMO operations */
goto_priv(PRIV_S);
PRIV_DO(mem_amo_swap_w(addr, 0x42));
goto_priv(PRIV_M);
CHECK_NO_TRAP("AMO should succeed");
```

**Design Intent**: Test **instruction execution permission** for a specific address in S/U-mode. Uses `exec_at()` to jump to target address (target address should contain `nop; ret` sequence), trap handler recovers via `_exec_return_addr`.

**Use Cases**:

```c
/* Test code region is executable */
goto_priv(PRIV_S);
PRIV_DO_EXEC(code_region);
goto_priv(PRIV_M);
CHECK_NO_TRAP("code region is executable");

/* Test data region is not executable */
goto_priv(PRIV_S);
PRIV_DO_EXEC(data_region);
goto_priv(PRIV_M);
CHECK_TRAP("data region not executable", CAUSE_INST_ACCESS_FAULT);
```

#### CHECK_NO_TRAP — Assert No Exception

```c
#define CHECK_NO_TRAP(msg) do { \
    TEST_ASSERT(msg, !trap_was_triggered()); \
} while (0)
```

**Design Intent**: Verify in M-mode that the previous `PRIV_DO` / `PRIV_DO_EXEC` operation did not trigger any trap. Internally calls `trap_was_triggered()` to check trap records and prints results via `TEST_ASSERT`.

**Notes**:
- Must be called after `goto_priv(PRIV_M)`
- Each `CHECK_*` call consumes one trap record, corresponding one-to-one with PRIV_DO call order

#### CHECK_TRAP — Assert Specified Exception

```c
#define CHECK_TRAP(msg, expected_cause) do { \
    TEST_ASSERT(msg ": trap triggered", trap_was_triggered()); \
    if (trap_was_triggered()) { \
        TEST_ASSERT_EQ(msg ": cause", trap_get_cause(), (uintptr_t)(expected_cause)); \
    } \
} while (0)
```

**Design Intent**: Verify in M-mode that the previous `PRIV_DO` / `PRIV_DO_EXEC` operation triggered a trap with a specific cause. First asserts that a trap indeed occurred, then asserts the cause code matches.

**Use Cases**:

```c
/* PMP denies read → load access fault */
CHECK_TRAP("PMP blocks read", CAUSE_LOAD_ACCESS_FAULT);

/* Page table lacks write permission → store page fault */
CHECK_TRAP("PTE denies write", CAUSE_STORE_PAGE_FAULT);

/* Execute non-X region → instruction access fault */
CHECK_TRAP("exec denied", CAUSE_INST_ACCESS_FAULT);
```



---

### Privilege Level Switching API

#### Privilege Level Constants

| Constant | Value | Meaning |
|----------|-------|---------|
| `PRIV_U` | 0 | User mode |
| `PRIV_S` | 1 | Supervisor mode |
| `PRIV_M` | 3 | Machine mode |
| `PRIV_VU` | 4 | Virtual User mode (Hypervisor extension) |
| `PRIV_VS` | 5 | Virtual Supervisor mode (Hypervisor extension) |

#### goto_priv — Switch Privilege Level

```c
void goto_priv(unsigned target);
```

Bidirectional switch to any privilege level:
- **Downward** (M→S, M→U, S→U): Implemented via `mret` / `sret`
- **Upward** (U→S, U→M, S→M): Handled by trap handler via `ecall`

```c
goto_priv(PRIV_S);   /* Switch to S-mode */
goto_priv(PRIV_U);   /* Switch to U-mode */
goto_priv(PRIV_M);   /* Switch back to M-mode */
```

#### get_current_priv — Get Current Privilege Level

```c
unsigned get_current_priv(void);
```

Returns the currently tracked privilege level (maintained internally by the framework).

#### run_in_priv — Execute Function at Specified Privilege Level

```c
uintptr_t run_in_priv(unsigned priv, uintptr_t (*fn)(uintptr_t), uintptr_t arg);
```

Switches to target privilege level, executes `fn(arg)`, then automatically returns to original privilege level. Returns the function's return value.

```c
static uintptr_t read_csr_in_smode(uintptr_t unused) {
    return CSRR(sstatus);
}
uintptr_t val = run_in_priv(PRIV_S, read_csr_in_smode, 0);
```

> **Note**: `run_in_priv` uses global variables to pass arguments and is not reentrant.

---

### State Management

#### reset_state — Reset Hardware State

```c
void reset_state(void);
```

Automatically called in each `TEST_END()`, restoring hardware to a clean baseline:

1. Ensure M-mode
2. Reset trap arm state
3. Rewrite `mtvec` / `stvec`
4. Clear `medeleg` / `mideleg` (M-mode handler takes over all exceptions)
5. Disable interrupts (clear MIE/SIE)
6. Clear MPRV, MXR bits
7. Clean up Pointer Masking state (trap-protected)

> Extension-specific resets (e.g., `pmp_clear_all()`) are called by each extension before `TEST_END` or in setup/teardown.

#### test_print_summary — Print Test Summary

```c
void test_print_summary(void);
```

Called after all tests have executed, prints result summary:

```
========================================
  Test Summary
========================================
  Total:   10
  Passed:  9
  Failed:  1
  Skipped: 0
----------------------------------------
  Assertions: 45 total, 44 passed, 1 failed
========================================
  RESULT: 1 FAILURES
----------------------------------------
  Failed tests:
    1) PMP TOR write protection
========================================
```

---

### Memory Operation Primitives

`mem_ops.h` provides a set of deterministic memory operation inline functions. All operations use `.option norvc` (uncompressed instructions), ensuring each instruction is exactly 4 bytes, allowing the trap handler to reliably skip faulting instructions via `mepc += 4`.

#### Load Operations

| Function | Description |
|----------|-------------|
| `mem_load8(addr)` | 8-bit load (lb) |
| `mem_load16(addr)` | 16-bit load (lh) |
| `mem_load32(addr)` | 32-bit load (lw) |
| `mem_load64(addr)` | 64-bit load (ld, RV64 only) |

#### Store Operations

| Function | Description |
|----------|-------------|
| `mem_store8(addr, val)` | 8-bit store (sb) |
| `mem_store16(addr, val)` | 16-bit store (sh) |
| `mem_store32(addr, val)` | 32-bit store (sw) |
| `mem_store64(addr, val)` | 64-bit store (sd, RV64 only) |

#### Execute Operation

```c
void exec_at(uintptr_t addr);
```

Jumps to `addr` to execute instructions. Target address should contain `nop; ret` sequence (populated by `entry.S`). Saves recovery address to `_exec_return_addr` before jumping, which trap handler can use for recovery.

#### AMO Operations

| Function | Description |
|----------|-------------|
| `mem_amo_swap_w(addr, val)` | Atomic swap (amoswap.w) |
| `mem_lr_w(addr)` | Load-Reserved (lr.w) |
| `mem_sc_w(addr, val)` | Store-Conditional (sc.w) |

---

### Logging System

The framework provides tiered logging macros, with output granularity controlled at compile time via `LOG_LEVEL`:

| Macro | Level | Purpose |
|-------|-------|---------|
| `LOG_E(fmt, ...)` | 1 - Error | Error messages |
| `LOG_W(fmt, ...)` | 2 - Warn | Warning messages |
| `LOG_I(fmt, ...)` | 3 - Info | General information (visible by default) |
| `LOG_D(fmt, ...)` | 4 - Debug | Debug information |
| `LOG_T(fmt, ...)` | 5 - Trace | Trace information |
| `LOG_V(fmt, ...)` | 6 - Verbose | Verbose information |

**Compile-time Configuration:**

```bash
make pmp LOG_LEVEL=5    # Enable up to Trace level
make pmp LOG_LEVEL=1    # Show errors only
```

---

### Build System Integration

#### Makefile.common

Each extension's `Makefile` includes common build rules via `include ../common/Makefile.common`.

**Typical Extension Makefile:**

```makefile
TARGET   = my_ext_test.elf

# Enable common libraries as needed
# ENABLE_PMP = 1    # Link common/pmp/ library
# ENABLE_VM  = 1    # Link common/vm/ library
# ENABLE_HYP = 1    # Link common/hyp/ library

EXT_OBJS = main.o tests/test_register.o

include ../common/Makefile.common
```

#### Conditional Compilation Switches

| Variable | Effect |
|----------|--------|
| `ENABLE_PMP=1` | Link `common/pmp/` (PMP configuration API) |
| `ENABLE_VM=1` | Link `common/vm/` (page table management API) |
| `ENABLE_HYP=1` | Link `common/hyp/` (Hypervisor support) |
| `ENABLE_PM=1` | Link `common/pm/` (Pointer Masking support) |
| `ENABLE_TWO_STAGE=1` | Enable two-stage translation support (Hypervisor G-stage) |

#### Linker Scripts

Base linker scripts are located in platform configuration directories `common/config/<platform>/link.ld`, defining common section layout (`.text.init`, `.data`, `.bss`, etc.). Each extension directory's `kernel.ld` inherits the base script via `INCLUDE` directive and adds extension-specific sections (e.g., `.test_table`, `.pmp_test_region`, `.page_tables`, etc.).

**Typical Extension kernel.ld Structure:**

```ld
INCLUDE "link.ld"

/* Extension-specific sections */
.test_table : { ... }
.pmp_test_region : { ... }
```

**Key Section Descriptions:**

| Section | Description |
|---------|-------------|
| `.text.init` | Boot code (entry.S) |
| `.text` | Code section |
| `.rodata` | Read-only data |
| `.test_table` | Test function pointer array (generated by `TEST_REGISTER`) |
| `.data` | Writable data |
| `.bss` | Zero-initialized data (cleared by entry.S) |
| `.stack` | Stack space (default 128KB) |

**`.test_table` Section Linker Symbols:**

```ld
.test_table : {
    . = ALIGN(8);
    _test_table = .;
    *(.test_table)
    _test_table_end = .;
}
_test_table_size = (_test_table_end - _test_table) / (__riscv_xlen / 8);
```

`main.c` iterates through all registered tests via `extern test_func_t _test_table[]` and `extern test_func_t _test_table_end[]`.

#### Build Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `XLEN` | `64` | Target architecture width: `32` or `64` |
| `CONFIG` | `qemu-rv64-max` | Platform configuration (options: `qemu-rv64-max`, `sail-rv64-max`, `spike-rv64-max` etc.) |
| `LOG_LEVEL` | `3` | Log level: 0-6 |
| `MEM_BASE` | Platform default | Memory base address |
| `CROSS_COMPILER` | Platform default | Cross-compiler prefix |

---

### Writing Test Cases

#### Complete Example

```c
#include "test_framework.h"

/* Declare external symbols (defined by linker script) */
extern uintptr_t __test_data;

/* Register test */
TEST_REGISTER(test_basic_access);

bool test_basic_access(void) {
    TEST_BEGIN("Basic memory access in S-mode");

    uintptr_t data_addr = (uintptr_t)&__test_data;

    /* 1. Configure hardware state in M-mode */
    // ... extension-specific configuration ...

    /* 2. Direct testing in M-mode (EXPECT_* macros available) */
    EXPECT_NO_TRAP(mem_load32(data_addr));

    /* 3. Switch to S-mode for testing */
    goto_priv(PRIV_S);
    PRIV_DO_NO_TRAP(mem_load32(data_addr));
    PRIV_DO_TRAP(mem_store32(data_addr, 0xDEAD));
    goto_priv(PRIV_M);

    /* 4. Check S-mode test results in M-mode */
    CHECK_NO_TRAP("S-mode read should succeed");
    CHECK_TRAP("S-mode write should fault", CAUSE_SAF);

    /* 5. Value assertions */
    uintptr_t val = mem_load32(data_addr);
    TEST_ASSERT_EQ("data unchanged", val, 0x12345678);

    /* 6. End test */
    TEST_END();
}
```

#### Test Function Signature

Each test function must:
- Return `bool` (true = pass, false = fail)
- Start with `TEST_BEGIN(name)`
- End with `TEST_END()`
- Complete all assertions before `TEST_END()`

#### Best Practices

1. **Do not call framework APIs in S/U-mode** — PMP configuration APIs use CSR instructions and can only be called in M-mode. Configure hardware state in M-mode first, then switch privilege levels.

2. **Use PRIV_DO + CHECK pattern** — printf is unavailable in S/U-mode; the two-phase pattern must be used.

3. **TEST_END() contains return** — Do not write any code after `TEST_END()`.

4. **Detect hardware capabilities before SKIP** — If a test depends on optional extensions, detect first then decide whether to skip:

   ```c
   if (!detect_feature()) {
       TEST_SKIP("feature not supported");
   }
   ```

5. **Use assertion macros appropriately** — Choose the most informative assertion based on what is being checked:
   - Value comparison → `TEST_ASSERT_EQ` / `TEST_ASSERT_NEQ`
   - Bitfield check → `TEST_ASSERT_BITS`
   - Compound condition → `TEST_ASSERT`

6. **EXPECT_* vs PRIV_DO+CHECK** — Use `EXPECT_*` directly in M-mode; only use `PRIV_DO_*` + `CHECK_*` two-phase pattern in S/U-mode.

---

### Common Exception Cause Codes

Common exception cause code constants defined in `encoding.h`:

| Constant | Value | Meaning | Short Alias |
|----------|-------|---------|-------------|
| `CAUSE_INST_ADDR_MISALIGN` | 0 | Instruction address misaligned | — |
| `CAUSE_INST_ACCESS_FAULT` | 1 | Instruction access fault | `CAUSE_IAF` |
| `CAUSE_ILLEGAL_INST` | 2 | Illegal instruction | `CAUSE_ILI` |
| `CAUSE_BREAKPOINT` | 3 | Breakpoint | — |
| `CAUSE_LOAD_ADDR_MISALIGN` | 4 | Load address misaligned | — |
| `CAUSE_LOAD_ACCESS_FAULT` | 5 | Load access fault | `CAUSE_LAF` |
| `CAUSE_STORE_ADDR_MISALIGN` | 6 | Store address misaligned | — |
| `CAUSE_STORE_ACCESS_FAULT` | 7 | Store access fault | `CAUSE_SAF` |
| `CAUSE_ECALL_FROM_U` | 8 | U-mode ecall | `CAUSE_ECU` |
| `CAUSE_ECALL_FROM_S` | 9 | S-mode ecall | `CAUSE_ECS` |
| `CAUSE_ECALL_FROM_VS` | 10 | VS-mode ecall | — |
| `CAUSE_ECALL_FROM_M` | 11 | M-mode ecall | `CAUSE_ECM` |
| `CAUSE_INST_PAGE_FAULT` | 12 | Instruction page fault | `CAUSE_IPF` |
| `CAUSE_LOAD_PAGE_FAULT` | 13 | Load page fault | `CAUSE_LPF` |
| `CAUSE_STORE_PAGE_FAULT` | 15 | Store page fault | `CAUSE_SPF` |
| `CAUSE_SOFTWARE_CHECK` | 18 | Software check exception (CFI) | — |
| `CAUSE_HARDWARE_ERROR` | 19 | Hardware error | — |
| `CAUSE_INST_GUEST_PAGE_FAULT` | 20 | Instruction guest page fault (H extension) | — |
| `CAUSE_LOAD_GUEST_PAGE_FAULT` | 21 | Load guest page fault (H extension) | — |
| `CAUSE_VIRTUAL_INSTRUCTION` | 22 | Virtual instruction exception (H extension) | — |
| `CAUSE_STORE_GUEST_PAGE_FAULT` | 23 | Store guest page fault (H extension) | — |

---

### Test Result Data Structure

```c
typedef struct {
    unsigned int total;              /* Total assertion count */
    unsigned int passed;             /* Passed assertion count */
    unsigned int failed;             /* Failed assertion count */
    unsigned int skipped;            /* Skipped test count */
    unsigned int tests_passed;       /* Passed test case count */
    unsigned int tests_failed;       /* Failed test case count */
    const char  *current_test_name;  /* Current test name */
    bool         current_test_failed;/* Whether current test has failed assertions */
    const char  *failed_names[64];   /* List of failed test names */
    unsigned int failed_count;       /* Number of recorded failed tests */
} test_result_t;

extern test_result_t test_results;
```

The global instance `test_results` is defined in `test_framework.c`; all macros track test progress through it.
