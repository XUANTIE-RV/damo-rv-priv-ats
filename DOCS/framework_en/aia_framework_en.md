**[中文](../framework/aia_framework.md) | English**

## AIA Test Framework Common Documentation

**Scope**: All RISC-V AIA (Advanced Interrupt Architecture) test plans

**Specification Reference**: RISC-V Advanced Interrupt Architecture Specification, Version 1.0 (Ratified June 2023, Revised 20250312)

---

### Test Plan Dependencies and Execution Order

```
aia_smaia_test_plan.md (CSR + Priority)
        │
        ▼
aia_imsic_test_plan.md (IMSIC)
        │
        ├──────────────────┐
        ▼                  ▼
aia_aplic_test_plan.md   aia_vs_test_plan.md (VS Virtualization)
        │                  │
        └──────┬───────────┘
               ▼
aia_ipi_iommu_test_plan.md (IPI + IOMMU)
```

| Execution Order | Test Plan | Prerequisites |
|-----------------|-----------|---------------|
| 1 | `aia_smaia_test_plan.md` | None |
| 2 | `aia_imsic_test_plan.md` | CSR basic functionality passed |
| 3 | `aia_aplic_test_plan.md` | IMSIC functionality passed (MSI forwarding mode depends on IMSIC) |
| 3 | `aia_vs_test_plan.md` | CSR + IMSIC functionality passed |
| 4 | `aia_ipi_iommu_test_plan.md` | IMSIC functionality passed |

---

### Directory Structure

The AIA test framework contains 5 submodule directories, each independently compiled and executed:

```
damo-priv-test/
├── aia_aplic/           # APLIC (Advanced Platform-Level Interrupt Controller) tests
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   ├── aia_encoding.h   # AIA CSR encoding constants (shared)
│   ├── aia_helpers.h    # AIA helper functions (shared, inline assembly implementation)
│   ├── aia_platform.h   # Platform configuration (shared)
│   └── tests/
│       ├── test_register.c
│       └── ...
├── aia_imsic/           # IMSIC (Incoming MSI Controller) tests
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   ├── aia_encoding.h
│   ├── aia_helpers.h
│   ├── aia_platform.h
│   └── tests/
├── aia_smaia/           # Smaia (Supervisor-mode AIA) CSR tests
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   ├── aia_encoding.h
│   ├── aia_helpers.h
│   ├── aia_platform.h
│   └── tests/
├── aia_vs/              # VS-mode AIA virtualization tests
│   ├── Makefile
│   ├── kernel.ld
│   ├── main.c
│   ├── aia_encoding.h
│   ├── aia_helpers.h
│   ├── aia_platform.h
│   └── tests/
└── aia_ipi_iommu/       # IPI + IOMMU integration tests
    ├── Makefile
    ├── kernel.ld
    ├── main.c
    ├── aia_encoding.h
    ├── aia_helpers.h
    ├── aia_platform.h
    └── tests/
```

**Shared Header Files Description**:

- **`aia_encoding.h`**: Defines AIA-related CSR address constants, bit field masks, interrupt priority encodings, etc.
- **`aia_helpers.h`**: Provides `static inline` helper functions that wrap CSR read/write operations using inline assembly (e.g., `aia_write_miselect`, `aia_read_mireg`, etc.)
- **`aia_platform.h`**: Platform-specific configurations (e.g., IMSIC base addresses, number of APLIC domains, etc.)

Each submodule directory contains copies of these three header files to ensure independent compilation.

---

### Common Helper Function Interfaces

> **Implementation Note**: The following APIs are implemented in `aia_helpers.h` as `static inline` functions with inline assembly, directly wrapping CSR read/write instructions. Each AIA submodule directory (`aia_aplic/`, `aia_imsic/`, etc.) contains the shared `aia_helpers.h`, `aia_encoding.h`, and `aia_platform.h` header files.

#### CSR Operations

```c
/* Basic CSR read/write interfaces (inline assembly implementation) */
uint64_t csr_read(unsigned int csr_num);
void csr_write(unsigned int csr_num, uint64_t val);
void csr_set(unsigned int csr_num, uint64_t mask);
void csr_clear(unsigned int csr_num, uint64_t mask);
uint64_t csr_swap(unsigned int csr_num, uint64_t val);  /* CSRRW semantics */
```

**Example (actual implementation in aia_helpers.h)**:

```c
static inline void aia_write_miselect(uintptr_t val) {
    asm volatile("csrw " CSR_STR(0x350) ", %0" :: "r"(val) : "memory");
}

static inline uintptr_t aia_read_miselect(void) {
    uintptr_t val;
    asm volatile("csrr %0, " CSR_STR(0x350) : "=r"(val) :: "memory");
    return val;
}
```

#### Privilege Level Switching

```c
/* Switch to specified privilege level for execution */
void goto_priv(unsigned int priv_level);

/* Privilege level constants */
#define PRIV_M   3
#define PRIV_HS  2   /* HS-mode (Hypervisor) */
#define PRIV_S   1
#define PRIV_U   0
#define PRIV_VS  5   /* VS-mode (Virtual Supervisor) */
#define PRIV_VU  4   /* VU-mode (Virtual User) */
```

#### Exception Expectation Macros

```c
/*
 * EXPECT_TRAP(cause, statement)
 * Execute statement, expecting to trigger the exception specified by cause.
 * If no exception is triggered or a different exception occurs, the test fails.
 */
#define EXPECT_TRAP(cause, statement)  do { \
    arm_trap(cause);                        \
    statement;                              \
    TEST_ASSERT("expected trap occurred", trap_triggered(cause)); \
    disarm_trap();                           \
} while (0)

/* Exception cause codes */
#define CAUSE_ILLEGAL_INSTRUCTION    2
#define CAUSE_VIRTUAL_INSTRUCTION    22
```

#### Polling Wait

```c
/*
 * poll_with_timeout(timeout_us, condition)
 * Poll until condition becomes true or timeout occurs.
 * If condition is still not true after timeout, continue (caller should check).
 */
#define poll_with_timeout(timeout_us, condition)  do { \
    unsigned long _deadline = read_time() + us_to_ticks(timeout_us); \
    while (!(condition) && read_time() < _deadline) { \
        asm volatile("nop");                           \
    }                                                  \
} while (0)
```

#### Memory-Mapped I/O

```c
/* 32-bit little-endian read/write */
uint32_t readl(volatile void *addr);
void writel(uint32_t val, volatile void *addr);
```

---

### IMSIC Helper Function Interfaces

```c
/* Platform configuration queries */
uintptr_t platform_imsic_m_base(void);         /* M-level IMSIC base address A */
uintptr_t platform_imsic_s_base(void);         /* S-level IMSIC base address B */
unsigned int platform_imsic_m_stride_log2(void);  /* M-level stride C */
unsigned int platform_imsic_s_stride_log2(void);  /* S-level stride D */
unsigned int platform_hart_count(void);
unsigned int platform_max_identity(void);      /* Maximum implemented interrupt identity N */
unsigned int platform_geilen(void);            /* Number of guest interrupt files */

/* Interrupt file address calculation */
uintptr_t imsic_m_file_addr(unsigned int hart);
uintptr_t imsic_s_file_addr(unsigned int hart);
uintptr_t imsic_guest_file_addr(unsigned int hart, unsigned int guest);

/* Interrupt file register operations (indirect access via *iselect/*ireg) */
void imsic_set_eidelivery(uint32_t val);
uint32_t imsic_get_eidelivery(void);

void imsic_set_eithreshold(uint32_t val);
uint32_t imsic_get_eithreshold(void);

void imsic_set_eip(unsigned int identity, int val);  /* Set/clear pending */
int imsic_get_eip(unsigned int identity);             /* Query pending */

void imsic_set_eie(unsigned int identity, int val);  /* Set/clear enable */
int imsic_get_eie(unsigned int identity);             /* Query enable */

/* Guest interrupt file operations (requires specifying hart and guest numbers) */
void guest_imsic_set_eie(unsigned int hart, unsigned int guest,
                         unsigned int identity, int val);
void guest_imsic_set_eip(unsigned int hart, unsigned int guest,
                         unsigned int identity, int val);
int guest_imsic_get_eip(unsigned int hart, unsigned int guest,
                        unsigned int identity);
```

---

### APLIC Helper Function Interfaces

```c
/* APLIC domain base address query */
volatile void *aplic_domain_base(unsigned int domain_id);

/* IDC (Direct Delivery) structure base address */
volatile void *aplic_idc_base(unsigned int domain_id, unsigned int hart);

/* domaincfg operations */
void aplic_set_domaincfg(unsigned int domain_id, uint32_t flags);
void aplic_set_domaincfg_ie(unsigned int domain_id, int enable);

/* Source configuration */
void aplic_set_sourcecfg(unsigned int domain_id, unsigned int source,
                         uint32_t mode);

/* Source mode constants */
#define APLIC_SM_INACTIVE  0
#define APLIC_SM_DETACHED  1
#define APLIC_SM_EDGE1     4
#define APLIC_SM_EDGE0     5
#define APLIC_SM_LEVEL1    6
#define APLIC_SM_LEVEL0    7

/* domaincfg flag bits */
#define APLIC_IE       (1 << 8)
#define APLIC_DM_MSI   (1 << 2)
#define APLIC_BE       (1 << 0)

/* Pending/Enable operations */
void aplic_set_pending(unsigned int domain_id, unsigned int source);
void aplic_clear_pending(unsigned int domain_id, unsigned int source);
int aplic_get_pending(unsigned int domain_id, unsigned int source);
void aplic_set_ie(unsigned int domain_id, unsigned int source, int enable);

/* Target configuration */
void aplic_set_target(unsigned int domain_id, unsigned int source,
                      unsigned int hart_index, unsigned int guest_index,
                      unsigned int eiid);

/* MSI address configuration */
void aplic_set_mmsiaddrcfg(unsigned int domain_id, uint64_t base_ppn,
                           unsigned int lhxw, unsigned int hhxw,
                           unsigned int lhxs, unsigned int hhxs);
void aplic_set_smsiaddrcfg(unsigned int domain_id, uint64_t base_ppn,
                           unsigned int lhxw, unsigned int hhxw,
                           unsigned int lhxs, unsigned int hhxs);

/* genmsi operations */
void aplic_write_genmsi(unsigned int domain_id, unsigned int hart_index,
                        unsigned int eiid);
int aplic_genmsi_busy(unsigned int domain_id);
```

---

### IOMMU Helper Function Interfaces

```c
/* Device context MSI configuration */
void iommu_set_msi_addr_mask(void *dev_ctx, uint32_t mask);
void iommu_set_msi_addr_pattern(void *dev_ctx, uint32_t pattern);

/* MSI page table operations */
typedef struct {
    uint64_t dw0;
    uint64_t dw1;
} msi_pte_t;

/* Device MSI write emulation */
void device_write_msi(unsigned int dev_id, uintptr_t addr, uint32_t data);

/* Virtual interrupt file address calculation */
uintptr_t virtual_imsic_addr(unsigned int file_num);
```

---

### Multi-Hart Operation Interfaces

```c
/* Get current hart ID */
unsigned int hart_id(void);

/* Execute code block on specified hart */
#define on_hart(hart, block)  do { \
    send_ipi_to(hart);             \
    /* hart executes block and returns synchronously */ \
    block;                          \
} while (0)

/* Trigger/clear interrupt helpers */
void trigger_m_timer_interrupt(void);
void clear_m_timer_interrupt(void);
void trigger_m_software_interrupt(void);
void clear_m_software_interrupt(void);
```

---

### General Notes for Test Execution

#### WARL Field Handling

Many register fields in AIA are WARL (Write Any, Read Legal). In testing:
- After writing any value and reading back, verify that the read-back value belongs to the set of legal values
- Cannot assume that the written value matches the read-back value exactly
- The set of legal values is implementation-defined; tests must enumerate all legal values allowed by the specification for assertions

#### Interrupt Delivery Latency

There may be latency between IMSIC interrupt file state changes and their reflection in `mip`/`hgeip`, as well as between APLIC MSI arrival at IMSIC. Tests involving interrupt signal checking must use the `poll_with_timeout` mechanism and cannot assume immediate visibility after writes.

#### Inter-Hart Communication and Synchronization

IPI and multi-hart tests require inter-hart synchronization mechanisms:
- Use spinlock + shared memory to implement inter-hart synchronization
- Execute `fence ow, ow` before sending MSI to ensure previous memory writes are visible to the target hart
- Target hart must execute `fence ir, ir` after receiving MSI to ensure observation of source hart's writes

#### Virtualization Test Environment

VS-level (Group 9) tests require a hypervisor environment:
- Must configure VS-mode guest execution context (`hstatus`, `vsstatus`, `vstvec`, etc.)
- `hstatus`.VGEIN selects the guest interrupt file
- Enter VS-mode via `sret`, return to HS-mode via trap

#### Privilege Level Switching

Most tests involve cross-privilege-level operations (M→S→VS):
- Implement privilege level switching through trap handlers and `mret`/`sret`
- All cross-privilege-level test code uses `.option norvc` to ensure fixed-length instructions (`mepc += 4` reliably skips faulting instructions)

#### IOMMU Device Emulation

IOMMU-related tests may require controllable DMA devices or device emulators to generate MSI writes at specified addresses. The platform must provide a concrete implementation of `device_write_msi()`.

---

### CSR Number Reference

| CSR Name | Number | Description |
|----------|--------|-------------|
| `miselect` | 0x350 | M-level indirect register select |
| `mireg` | 0x351 | M-level indirect register alias |
| `mtopei` | 0x35C | M-level top external interrupt (IMSIC) |
| `mtopi` | 0xFB0 | M-level top interrupt |
| `mvien` | 0x308 | M-level virtual interrupt enable |
| `mvip` | 0x309 | M-level virtual interrupt pending |
| `siselect` | 0x150 | S-level indirect register select |
| `sireg` | 0x151 | S-level indirect register alias |
| `stopei` | 0x15C | S-level top external interrupt (IMSIC) |
| `stopi` | 0xDB0 | S-level top interrupt |
| `vsiselect` | 0x250 | VS-level indirect register select |
| `vsireg` | 0x251 | VS-level indirect register alias |
| `vstopei` | 0x25C | VS-level top external interrupt (IMSIC) |
| `vstopi` | 0xEB0 | VS-level top interrupt |
| `hvien` | 0x608 | Hypervisor virtual interrupt enable |
| `hvictl` | 0x609 | Hypervisor virtual interrupt control |
| `hviprio1` | 0x646 | Hypervisor VS-level priority 1 |
| `hviprio2` | 0x647 | Hypervisor VS-level priority 2 |
| `hvip` | 0x645 | Hypervisor virtual interrupt pending |
