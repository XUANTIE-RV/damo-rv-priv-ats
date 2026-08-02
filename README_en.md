**[中文](README_cn.md) | English**

# Damo RV Priv ATS

Damo RV Priv ATS is a compliance test framework for validating RISC-V privilege extensions. It runs directly on hardware DUTs or simulators (QEMU / Sail / Spike) as bare-metal programs with no operating system dependency. The framework covers 90+ privilege extensions defined in the RISC-V Privileged Specification, with each extension compiled into a fully independent ELF binary. Platform abstraction enables the same test code to target QEMU, Sail, Spike, or FPGA/hardware by simply switching the build configuration, with zero coupling between the shared framework and extension-specific logic.

---

## Project Directory Structure

```
damo-priv-test/
├── Makefile               # Top-level: builds all extensions
├── README_en.md / README_cn.md
│
├── common/                # Shared infrastructure (extension-independent)
│   ├── pmp/               # PMP common library
│   ├── vm/                # Virtual memory common library
│   ├── hyp/               # Hypervisor extension framework
│   ├── pm/                # Pointer Masking framework
│   ├── config/            # Platform configurations (qemu, sail, spike, haps, ...)
│   └── ...                # entry.S, trap.c, privilege.c, test_framework.h/c, uart.c, etc.
│
├── DOCS/                  # Project documentation
│   ├── framework/         # Framework design documents (architecture, API, usage)
│   ├── testplan/          # Compliance test plans (one per extension)
│   ├── develop_guide/     # Developer guides (test writing, API reference, adding extensions)
│   ├── framework_en/      # English framework documents
│   └── testplan_en/       # English test plan documents
│
├── SPEC/                  # RISC-V specification excerpts (.adoc) and full spec repos
├── NORM/                  # Structured spec requirement IDs (Norm ID → spec text)
│
├── Sha/                   # ── Extension test directories ──
├── Sstc/                  #    (each compiles to an independent bare-metal binary)
├── Sv39/                  #
├── Hypervisor/            #
├── aia_aplic/             #
└── ...                    #    (90+ extension directories total)
```

Standard structure of each extension directory:

```
<extension>/
├── Makefile          # Includes ../common/Makefile.common
├── kernel.ld         # Linker script
├── main.c            # Extension test entry point
└── tests/
    ├── test_register.c   # Test case registration
    └── test_xxx.c        # Individual test cases
```

---

## Prerequisites

- **RISC-V cross-compiler** (either one):
  - GCC: `riscv64-unknown-elf-gcc` (or `riscv32-unknown-elf-gcc` for RV32)
  - LLVM/Clang: `clang` + `ld.lld` + `llvm-objcopy` + `llvm-objdump`
- **QEMU** (optional): `qemu-system-riscv64` / `qemu-system-riscv32`
- **Sail** (optional): `sail_riscv_sim` for reference model verification
- **Spike** (optional): `spike` for ISA simulation

---

## Quick Start

```bash
# Build a specific extension (default: QEMU RV64)
make pmp CROSS_COMPILER=/path/to/riscv64-unknown-elf-

# Build for a different platform
make pmp CONFIG=sail-rv64-max CROSS_COMPILER=/path/to/riscv64-unknown-elf-

# Build all extensions
make all CROSS_COMPILER=/path/to/riscv64-unknown-elf-

# Run on simulators
make qemu-pmp CROSS_COMPILER=/path/to/riscv64-unknown-elf-
make sail-pmp CROSS_COMPILER=/path/to/riscv64-unknown-elf-
make spike-pmp CROSS_COMPILER=/path/to/riscv64-unknown-elf-

# Build for RV32
make pmp XLEN=32 CROSS_COMPILER=/path/to/riscv32-unknown-elf-
```

Run specific tests using `TEST_FILTER`:

```bash
make qemu-pmp EXTRA_CFLAGS='-DTEST_FILTER="PMP"' CROSS_COMPILER=/path/to/riscv64-unknown-elf-
```

---

## Toolchain Selection

The framework supports both **GCC** and **LLVM/Clang** toolchains, switched via the `TOOLCHAIN` variable. GCC is the default for full backward compatibility.

### GCC (default)

```bash
# Default behavior, no extra parameters needed
make pmp

# Specify toolchain prefix manually
make pmp CROSS_COMPILER=/path/to/riscv64-unknown-elf-

# Build for RV32
make pmp XLEN=32 CROSS_COMPILER=/path/to/riscv32-unknown-elf-
```

### LLVM/Clang

When using the LLVM toolchain, the framework invokes the compiler via `$(CROSS_COMPILER)clang` (e.g., `riscv64-unknown-elf-clang`) and uses `ld.lld` as the linker. The `CROSS_COMPILER` prefix works the same way as in GCC mode.

```bash
# Switch to LLVM toolchain (default prefix: riscv64-unknown-elf-)
make pmp TOOLCHAIN=clang

# Run on simulators
make qemu-pmp TOOLCHAIN=clang

# Build all extensions
make all TOOLCHAIN=clang

# Build for RV32 (clang handles 32/64-bit via -march)
make pmp TOOLCHAIN=clang XLEN=32

# Specify toolchain prefix manually
make pmp TOOLCHAIN=clang CROSS_COMPILER=/path/to/riscv64-unknown-elf-
```

Optional parameters:

| Variable | Default | Description |
|----------|---------|-------------|
| `TOOLCHAIN` | `gcc` | Compiler backend: `gcc` or `clang` |
| `CROSS_COMPILER` | `riscv64-unknown-elf-` | Toolchain prefix (shared by GCC and Clang modes) |

### Installing LLVM Toolchain

```bash
# Ubuntu/Debian
sudo apt install llvm lld clang

# Or build RISC-V LLVM from source
# See: https://github.com/llvm/llvm-project
```

---

## DUT Adaptation

The framework supports multiple DUT (Device Under Test) targets through the `CONFIG` variable. Each configuration defines platform-specific memory layout, UART settings, and build options.

### Available Configurations

| CONFIG | Target | XLEN | MEM_BASE | Description |
|--------|--------|------|----------|-------------|
| `qemu-rv64-max` | QEMU virt | 64 | 0x80000000 | Default QEMU platform |
| `qemu-rv32-max` | QEMU virt | 32 | 0x80000000 | QEMU RV32 variant |
| `sail-rv64-max` | Sail | 64 | 0x80000000 | Reference model |
| `spike-rv64-max` | Spike | 64 | 0x80000000 | ISA simulator |


### How It Works

Each config lives in `common/config/<CONFIG>/` and contains:
- **platform.mk** — Build settings (cross-compiler, memory base, simulator options)
- **platform_config.h** — Platform-level configuration (UART base address, AIA BASE address, TRACE base address, etc.)
- **rvtest_config.h** — Core-supported extension definitions and ISA parameters (auto-generated by riscv-unified-db)
- **rvmodel_macros.h** — Model parameters

Platform headers and extension configuration are injected at compile time via GCC `-include`, so test source code never directly includes platform-specific headers.

### Running on HAPS Hardware

```bash
cd <test_folder>
make clean
make CONFIG=haps_xiaohui CROSS_COMPILER=/path/to/riscv64-unknown-elf-
../scripts/remote_debug.py <ip_addr> <test_elf>
```

---

## Important Notes

1. **Always `make clean` before switching CONFIG** — Platform headers are baked into all `.o` files via `-include`. Switching platforms without cleaning will cause link errors or incorrect behavior due to stale object files with the wrong platform definitions.

2. **Different platforms have different memory layouts** — QEMU uses `MEM_BASE=0x80000000`, while HAPS platforms use `MEM_BASE=0x60000000`. Binaries compiled for one platform will not work on another.

3. **Platform-specific test exclusions** — Some platforms define flags like `SKIP_BREAKPOINT_TESTS` or `PLATFORM_CLEAR_MAEE` that alter test behavior. Check `platform_config.h` for platform-specific constraints.

4. **RV32 vs RV64** — Use `CONFIG=qemu-rv32-max` with `XLEN=32` for RV32 builds. Not all extensions support RV32.

5. **S/U-mode needs PMP coverage** — Without any matching PMP entry, all S-mode and U-mode accesses are denied. Always configure at least one PMP entry covering firmware code before switching privilege levels.

6. **`TEST_END()` contains `return`** — Do not write code after `TEST_END()`.

For test-writing guidelines and core API reference, see [`DOCS/develop_guide/`](DOCS/develop_guide/).

---

## Implemented Extensions

| Category | Extension | Description | Open-sourced |
|----------|-----------|-------------|:------------:|
| **Memory Protection** | `pmp` | PMP Physical Memory Protection | |
| | `smepmp` | Smepmp (PMP M-mode enhancements) | |
| | `spmp` | SPMP (S-level Physical Memory Protection) | |
| **Virtual Memory** | `Sv39` | Sv39 (3-level page table) | ✓ |
| | `Sv48` | Sv48 (4-level page table) | ✓ |
| | `Sv57` | Sv57 (5-level page table) | ✓ |
| | `Svbare` | Svbare (satp.MODE=Bare) | |
| | `Svnapot` | NAPOT translation contiguity | ✓ |
| | `Svpbmt` | Page-based memory types | ✓ |
| | `Svinval` | Fine-grained TLB invalidation | ✓ |
| | `Svade` | Hardware A/D bit exceptions | |
| | `Svadu` | Hardware A/D bit auto-update | ✓ |
| | `Svvptc` | Obviating MM instructions after PTE valid | |
| | `Svrsw60t59b` | PTE reserved bit extension | |
| **PMP+VM Interaction** | `pmp_sv39` | PMP + Sv39 interaction | |
| | `pmp_sv48` | PMP + Sv48 interaction | |
| | `pmp_sv57` | PMP + Sv57 interaction | |
| **Hypervisor** | `Hypervisor` | Hypervisor base extension | ✓ |
| | `Sv39x4` | Sv39x4 G-stage translation | ✓ |
| | `Sv48x4` | Sv48x4 G-stage translation | ✓ |
| | `Sv57x4` | Sv57x4 G-stage translation | ✓ |
| | `Sv39x4_Sv39` | Two-stage: Sv39x4 + Sv39 | ✓ |
| | `Sv39x4_Sv48` | Two-stage: Sv39x4 + Sv48 | ✓ |
| | `Sv39x4_Sv57` | Two-stage: Sv39x4 + Sv57 | ✓ |
| | `Sv48x4_Sv39` | Two-stage: Sv48x4 + Sv39 | ✓ |
| | `Sv48x4_Sv48` | Two-stage: Sv48x4 + Sv48 | ✓ |
| | `Sv48x4_Sv57` | Two-stage: Sv48x4 + Sv57 | ✓ |
| | `Sv57x4_Sv39` | Two-stage: Sv57x4 + Sv39 | ✓ |
| | `Sv57x4_Sv48` | Two-stage: Sv57x4 + Sv48 | ✓ |
| | `Sv57x4_Sv57` | Two-stage: Sv57x4 + Sv57 | ✓ |
| **Hypervisor CSR** | `Sha` | Augmented Hypervisor extension | ✓ |
| | `Shgatpa` | Translation mode support | ✓ |
| | `Shcounterenw` | Counter-enable writability | ✓ |
| | `Shlcofideleg` | Counter overflow delegation | ✓ |
| | `Shtvala` | Hypervisor trap value reporting | ✓ |
| | `Shvsatpa` | VS-stage translation mode support | ✓ |
| | `Shvstvala` | VS-stage trap value reporting | ✓ |
| | `Shvstvecd` | VS-stage direct trap vectoring | ✓ |
| **Hypervisor Combos** | `Hypervisor_Smcsrind` | Hyp + Smcsrind | ✓ |
| | `Hypervisor_Smmpm` | Hyp + Smmpm (M-mode Pointer Masking) | ✓ |
| | `Hypervisor_Smnpm` | Hyp + Smnpm (Next-level Pointer Masking) | ✓ |
| | `Hypervisor_Smstateen` | Hyp + Smstateen | ✓ |
| | `Hypervisor_Ssccptr` | Hyp + Ssccptr | ✓ |
| | `Hypervisor_Sscsrind` | Hyp + Sscsrind | ✓ |
| | `Hypervisor_Ssdbltrp` | Hyp + Ssdbltrp | ✓ |
| | `Hypervisor_Ssnpm` | Hyp + Ssnpm (S-mode Pointer Masking) | ✓ |
| | `Hypervisor_Ssstateen` | Hyp + Ssstateen | ✓ |
| | `Hypervisor_Sstc` | Hyp + Sstc | ✓ |
| | `Hypervisor_Sstvala` | Hyp + Sstvala | ✓ |
| | `Hypervisor_Svadu` | Hyp + Svadu | ✓ |
| | `Hypervisor_Svinval` | Hyp + Svinval | ✓ |
| | `Hypervisor_Svnapot` | Hyp + Svnapot | ✓ |
| | `Hypervisor_Svpbmt` | Hyp + Svpbmt | ✓ |
| | `Hypervisor_Zicbom` | Hyp + Zicbom (Cache Block Management) | ✓ |
| | `Hypervisor_Zicbop` | Hyp + Zicbop (Cache Block Prefetch) | ✓ |
| | `Hypervisor_Zicboz` | Hyp + Zicboz (Cache Block Zero) | ✓ |
| | `Hypervisor_Zicfilp` | Hyp + Zicfilp (CFI Landing Pad) | ✓ |
| | `Hypervisor_Zicfiss` | Hyp + Zicfiss (CFI Shadow Stack) | ✓ |
| | `Hypervisor_Smcntrpmf` | Hyp + Smcntrpmf | |
| | `Hypervisor_Ssqosid` | Hyp + Ssqosid | |
| | `Hypervisor_Zkr` | Hyp + Zkr | |
| **Machine-mode (Sm*)** | `Smstateen` | State enable | ✓ |
| | `smrnmi` | Resumable NMI | |
| | `Smcdeleg` | Counter delegation | |
| | `Smcntrpmf` | Cycle/Instret privilege mode filtering | |
| | `Smcsrind` | Indirect CSR access | ✓ |
| | `Smctr` | Control Transfer Records | |
| | `Smdbltrp` | Double trap | |
| **Supervisor (Ss*)** | `Ssccptr` | Main memory page-table reads | ✓ |
| | `Sscofpmf` | Counter overflow / mode filtering | |
| | `Sscounterenw` | Counter enable writability | |
| | `Ssstateen` | State enable | ✓ |
| | `Sstc` | Supervisor-mode timer interrupts | ✓ |
| | `Sstvala` | Trap value reporting | ✓ |
| | `Sstvecd` | Direct trap vectoring | ✓ |
| | `Ssu64xl` | UXLEN=64 support | |
| | `Ssccfg` | S-mode counter delegation | |
| | `Sscsrind` | Indirect CSR access | ✓ |
| | `Ssctr` | Control Transfer Records | |
| | `Ssdbltrp` | Double trap | ✓ |
| **AIA (Interrupts)** | `aia_aplic` | APLIC | |
| | `aia_imsic` | AIA IMSIC | |
| | `aia_smaia` | M-mode AIA | |
| | `aia_iommu` | AIA + IOMMU | |
| | `aia_hypervisor` | Hypervisor AIA | |
| **CFI (Control Flow Integrity)** | `cfi.Zicfilp` | CFI Landing Pad | ✓ |
| | `cfi.Zicfiss` | CFI Shadow Stack | ✓ |
| **CMO (Cache Management)** | `cmo.base` | CMO base | |
| | `cmo.Zicbom` | Cache Block Management | ✓ |
| | `cmo.Zicbop` | Cache Block Prefetch | ✓ |
| | `cmo.Zicboz` | Cache Block Zero | ✓ |
| **Pointer Masking** | `zpm.Smmpm` | M-mode Pointer Masking | ✓ |
| | `zpm.Smnpm` | Next-level Pointer Masking | ✓ |
| | `zpm.Ssnpm` | S-mode Pointer Masking | ✓ |
| **CLIC (Interrupts)** | `clic` | CLIC | |
| | `aia_clic` | AIA + CLIC | |
| **Other** | `aclint` | ACLINT | |
| | `iopmp` | IOPMP | |
| | `qos.cbqri` | QoS CBQRI | |
| | `qos.Ssqosid` | QoS Ssqosid | |
| | `sbi` | SBI interface | |
| | `ntrace` | Ntrace | |
| | `raseri` | Raseri | |
| | `Zkr` | Entropy source (Key Seed) | |

---

## Design Philosophy

The framework separates **shared infrastructure** from **extension-specific logic**. `common/` provides boot, trap handling, privilege switching, and the test framework, while each extension directory compiles to an independent bare-metal ELF binary.

Key design principles:

1. **Zero coupling** — `common/` has no `#include` of any extension header. Extensions inject behavior through weak symbols and link-time composition.
2. **Independent bare-metal binaries** — Each extension is a fully self-contained ELF, loadable directly by QEMU, Sail, or Spike.
3. **Weak symbol hooks** — `entry.S` calls `_platform_init` (weak, defaults to no-op). Extensions can provide a strong definition for custom initialization.
4. **Platform abstraction via `-include`** — Platform headers (`platform_config.h` and `rvtest_config.h`) are injected at compile time, avoiding hardcoded `#include "platform_config.h"` in source files.
5. **RV32/RV64 dual support** — All assembly uses conditional macros derived from `__riscv_xlen`.
6. **Deterministic trap handling** — All memory operations use non-compressed instructions (`.option norvc`) so the trap handler can reliably skip faulting instructions with `mepc += 4`.
7. **Conditionally-linked common libraries** — Extensions opt-in via `ENABLE_PMP=1`, `ENABLE_VM=1`, `ENABLE_HYP=1`, `ENABLE_PM=1`, `ENABLE_TWO_STAGE=1`, `ENABLE_IOPMP=1`, `ENABLE_NTRACE=1`, or `ENABLE_CMO=1` in their Makefile.

---

## Further Reading

- [RISC-V Privileged Specification](https://github.com/riscv/riscv-isa-manual)
- [`DOCS/framework/`](DOCS/framework/) — Subsystem framework documents
- [`DOCS/testplan/`](DOCS/testplan/) — Extension test plans
- [`DOCS/develop_guide/`](DOCS/develop_guide/) — Developer guides
- [`SPEC/`](SPEC/) — Specification excerpts
- [`COVERPOINTS/`](COVERPOINTS/) — Coverage coverpoint definitions
