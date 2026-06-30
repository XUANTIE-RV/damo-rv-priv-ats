**[中文](../framework/hypervisor_framework.md) | English**

# RISC-V Hypervisor Extension Test Framework Design

This document describes the interface design and feature planning for adding a RISC-V Hypervisor (H) extension test framework on top of the existing common test framework.

---

## Overview

Testing the Hypervisor extension is more complex than existing PMP / VM (Sv39/48/57) tests, primarily because it introduces:

- **Virtualization mode (V=1)**: Two new privilege levels — VS-mode and VU-mode
- **HS-mode**: Extends S-mode to Hypervisor-extended Supervisor Mode
- **Two-stage address translation**: VS-stage (`vsatp`) + G-stage (`hgatp`)
- **Many new CSRs**: Hypervisor CSRs such as hstatus, hedeleg, hideleg, hgatp, hvip, and VS CSRs such as vsstatus, vsatp
- **New trap types**: Virtual-instruction exception (cause=22), guest-page fault (cause=20/21/23)
- **New privileged instructions**: HLV/HLVX/HSV (virtual machine load/store), HFENCE.VVMA/HFENCE.GVMA

The existing framework only supports three-level switching (M → S → U). It needs to be extended to support four-level switching (M → HS → VS → VU) and manage G-stage page tables controlled by `hgatp`.

---

## Specification References

- `SPEC/hypervisor.adoc` — RISC-V Hypervisor Extension (H), Version 1.0
- `SPEC/supervisor.adoc` — Supervisor-Level ISA
- `SPEC/machine.adoc` — Machine-Level ISA

---

## Relationship with Existing Framework

### Existing Framework Modules

| Module | File | Function |
|--------|------|----------|
| Test Framework | `common/test_framework.h/c` | TEST_BEGIN/END, assertion macros, result statistics |
| Privilege Switching | `common/privilege.c` | Bidirectional M↔S↔U switching |
| Trap Handling | `common/trap.c` + `trap_asm.S` | M-mode/S-mode trap handlers |
| CSR Access | `common/csr_accessors.c` | Runtime dynamic CSR read/write |
| Memory Operations | `common/mem_ops.h` | Load/store/exec primitives |
| VM Management | `common/vm/` | Sv39/48/57 page table management, satp control |
| Encoding Definitions | `common/encoding.h` | CSR addresses, mstatus bit fields, cause codes |

### Reuse Strategy

- **Direct reuse**: test_framework (test macros, result statistics), mem_ops (memory primitives), uart (serial output), entry.S (boot code)
- **Extended reuse**: encoding.h (new CSR addresses and cause codes), csr_accessors.c (new CSR cases), trap.c/trap_asm.S (extend handlers to support VS-mode traps)
- **Reference design**: vm/ (G-stage page table management follows the pattern of VS-stage page table management)

---

## New Module Design

### File Organization Structure

```
common/hyp/
├── hyp_defs.h        # CSR addresses, bit fields, cause codes, hgatp encoding
├── hyp_csr.c         # CSR convenience operation APIs
├── hyp_priv.c        # Virtualization privilege level switching (HS→VS→VU)
├── hyp_trap.c        # HS-mode trap handler (handles VS/VU-mode traps)
├── hyp_trap_asm.S    # HS-mode trap entry (assembly)
├── hyp_fence.c       # HFENCE.VVMA / HFENCE.GVMA wrappers
├── hyp_ldst.c        # HLV / HLVX / HSV instruction wrappers
├── gstage_pt.c       # G-stage page table management (hgatp, Sv39x4/48x4/57x4)
├── two_stage.c       # Two-stage translation combined management
├── hyp_reset.c       # Hypervisor state reset
└── hyp_test.h        # Hypervisor test macros

hypervisor/              # Test case directory
├── Makefile
├── kernel.ld
└── main.c
```

---

### Module 1: Hypervisor CSR Encoding Definitions (`common/hyp/hyp_defs.h`)

**Function**: Define all hypervisor-related CSR addresses, bit field constants, and cause codes.

#### Hypervisor CSR Addresses

| CSR | Address | Description |
|-----|---------|-------------|
| `hstatus` | 0x600 | Hypervisor status register |
| `hedeleg` | 0x602 | Hypervisor exception delegation |
| `hideleg` | 0x603 | Hypervisor interrupt delegation |
| `hvip` | 0x645 | Hypervisor virtual interrupt pending |
| `hip` | 0x644 | Hypervisor interrupt pending |
| `hie` | 0x604 | Hypervisor interrupt enable |
| `hgeip` | 0xE12 | Guest external interrupt pending |
| `hgeie` | 0x607 | Guest external interrupt enable |
| `henvcfg` | 0x60A | Hypervisor environment configuration |
| `hcounteren` | 0x606 | Hypervisor counter enable |
| `htimedelta` | 0x605 | Hypervisor time delta |
| `htval` | 0x643 | Hypervisor trap value |
| `htinst` | 0x64A | Hypervisor trap instruction |
| `hgatp` | 0x680 | Guest address translation and protection |

#### Virtual Supervisor CSR Addresses

| CSR | Address | Description |
|-----|---------|-------------|
| `vsstatus` | 0x200 | VS-mode status |
| `vsie` | 0x204 | VS-mode interrupt enable |
| `vstvec` | 0x205 | VS-mode trap vector |
| `vsscratch` | 0x240 | VS-mode scratch |
| `vsepc` | 0x241 | VS-mode exception PC |
| `vscause` | 0x242 | VS-mode cause |
| `vstval` | 0x243 | VS-mode trap value |
| `vsip` | 0x244 | VS-mode interrupt pending |
| `vsatp` | 0x280 | VS-mode address translation |

#### M-mode Extended CSRs

| CSR | Address | Description |
|-----|---------|-------------|
| `mtval2` | 0x34B | Second trap value (guest physical address >> 2) |
| `mtinst` | 0x34A | Trap instruction |

#### hstatus Bit Field Definitions

| Field | Bit Position | Description |
|-------|--------------|-------------|
| VSBE | bit 5 | VS-mode byte order |
| GVA | bit 6 | Guest Virtual Address flag |
| SPV | bit 7 | Supervisor Previous Virtualization mode |
| SPVP | bit 8 | Supervisor Previous Virtual Privilege |
| HU | bit 9 | Hypervisor in U-mode (allows U-mode to execute HLV/HSV) |
| VGEIN | bits 17:12 | Virtual Guest External Interrupt Number |
| VTVM | bit 20 | Virtual TVM (VS-mode SFENCE/satp triggers trap) |
| VTW | bit 21 | Virtual TW (VS-mode WFI triggers trap) |
| VTSR | bit 22 | Virtual TSR (VS-mode SRET triggers trap) |
| VSXL | bits 33:32 | VS-mode XLEN control (RV64 only) |

#### hgatp MODE Encoding

| Value | Name | Description |
|-------|------|-------------|
| 0 | Bare | No translation |
| 8 | Sv39x4 | 41-bit GPA, 16KB root page table |
| 9 | Sv48x4 | 50-bit GPA, 16KB root page table |
| 10 | Sv57x4 | 59-bit GPA, 16KB root page table |

#### New Exception Cause Codes

| Value | Name | Description |
|-------|------|-------------|
| 10 | `CAUSE_ECALL_FROM_VS` | VS-mode ecall |
| 20 | `CAUSE_INST_GUEST_PAGE_FAULT` | Instruction guest-page fault |
| 21 | `CAUSE_LOAD_GUEST_PAGE_FAULT` | Load guest-page fault |
| 22 | `CAUSE_VIRTUAL_INSTRUCTION` | Virtual-instruction exception |
| 23 | `CAUSE_STORE_GUEST_PAGE_FAULT` | Store/AMO guest-page fault |

#### hgatp Construction Macro

```c
#define MAKE_HGATP(mode, vmid, ppn) \
    (((uintptr_t)(mode) << HGATP_MODE_SHIFT) | \
     ((uintptr_t)(vmid) << HGATP_VMID_SHIFT) | \
     ((uintptr_t)(ppn) & HGATP_PPN_MASK))
```

---

### Module 2: Virtualization Privilege Level Switching (`common/hyp/hyp_priv.c`)

**Function**: Extend the existing `privilege.c` privilege switching mechanism to support complete V=0/V=1 switching.

#### Privilege Mode Definitions

```
V=0: M-mode → HS-mode → U-mode       (reuse existing PRIV_M / PRIV_S / PRIV_U)
V=1:           VS-mode → VU-mode       (new PRIV_VS / PRIV_VU)
```

| Constant | Value | Meaning |
|----------|-------|---------|
| `PRIV_HS` | 1 | HS-mode (V=0, nominal S) — reuses PRIV_S |
| `PRIV_VS` | 5 | VS-mode (V=1, nominal S) — encoding: V=1, S=1 |
| `PRIV_VU` | 4 | VU-mode (V=1, nominal U) — encoding: V=1, U=0 |

#### Interface Definitions

```c
/**
 * goto_vs_mode - Switch from HS-mode to VS-mode
 *
 * Sets hstatus.SPV=1, sstatus.SPP=1, then executes sret.
 * On sret, V=SPV=1, entering VS-mode.
 */
void goto_vs_mode(void);

/**
 * goto_vu_mode - Switch from VS-mode to VU-mode
 *
 * Sets sstatus.SPP=0 (actually accesses vsstatus.SPP), then executes sret.
 */
void goto_vu_mode(void);

/**
 * return_to_hs_mode - Return from VS/VU-mode to HS-mode
 *
 * Triggers a trap to HS-mode via ecall (assuming ecall is not delegated to VS-mode).
 */
void return_to_hs_mode(void);

/**
 * get_virt_priv - Get current complete privilege state (including V bit)
 *
 * Returns: PRIV_M / PRIV_HS / PRIV_U / PRIV_VS / PRIV_VU
 */
unsigned get_virt_priv(void);

/**
 * run_in_vs_mode - Execute a function in VS-mode and return the result
 *
 * Similar to existing run_in_priv(), but targets VS-mode.
 * Called from M/HS-mode, automatically switches to VS-mode to execute fn(arg),
 * then returns to HS-mode via ecall, and finally back to M-mode.
 */
uintptr_t run_in_vs_mode(uintptr_t (*fn)(uintptr_t), uintptr_t arg);

/**
 * run_in_vu_mode - Execute a function in VU-mode and return the result
 */
uintptr_t run_in_vu_mode(uintptr_t (*fn)(uintptr_t), uintptr_t arg);
```

#### Switching Paths

```
M-mode → HS-mode:  Set mstatus.MPP=S, mret
HS-mode → VS-mode: Set hstatus.SPV=1, sstatus.SPP=1, sret
HS-mode → VU-mode: Set hstatus.SPV=1, sstatus.SPP=0, sret
VS-mode → VU-mode: Set vsstatus.SPP=0, sret (via csrw sstatus)
VS/VU → HS-mode:   ecall → HS-mode trap handler
HS-mode → M-mode:  ecall → M-mode trap handler
```

---

### Module 3: Hypervisor Trap Handler (`common/hyp/hyp_trap.c` + `hyp_trap_asm.S`)

**Function**: Add HS-mode trap handler to handle traps from VS/VU-mode.

#### Extended Trap Record

```c
typedef struct {
    /* Inherited base trap_record fields */
    bool      armed;
    bool      triggered;
    unsigned  priv_level;    /* Privilege level at trap entry */
    uintptr_t cause;         /* scause (HS-mode) or mcause (M-mode) */
    uintptr_t epc;           /* sepc / mepc */
    uintptr_t tval;          /* stval / mtval */
    uintptr_t return_addr;

    /* Hypervisor extension fields */
    uintptr_t htval;         /* guest physical address >> 2 */
    uintptr_t htinst;        /* transformed instruction / pseudoinstruction */
    bool      gva;           /* hstatus.GVA: whether stval is a guest virtual address */
    bool      spv;           /* hstatus.SPV: whether the trap originated from V=1 */
} hyp_trap_record_t;
```

#### Interface Definitions

```c
/**
 * hs_trap_handler - HS-mode trap handler
 *
 * Handles traps from VS/VU-mode to HS-mode:
 * - VS-mode ecall (cause=10): Used for privilege level switching
 * - Virtual-instruction exception (cause=22): Records trap information
 * - Guest-page fault (cause=20/21/23): Records htval/htinst
 * - Page faults delegated to VS-mode
 */
unsigned hs_trap_handler(void);

/* Get hypervisor trap extension information */
uintptr_t trap_get_htval(void);
uintptr_t trap_get_htinst(void);
bool      trap_get_gva(void);
bool      trap_get_spv(void);
```

#### Trap Path Description

```
When a trap occurs in VS/VU-mode:
  ├── medeleg not set → M-mode trap handler (m_trap_handler)
  ├── medeleg set + hedeleg not set → HS-mode trap handler (hs_trap_handler)
  └── medeleg set + hedeleg set → VS-mode trap handler (via vstvec)

When a trap occurs in HS-mode:
  ├── medeleg not set → M-mode trap handler
  └── medeleg set → HS-mode trap handler (S-mode trap, hstatus.SPV=0)

Determining trap origin:
  - hstatus.SPV=1: Trap originated from V=1 (VS/VU-mode)
  - hstatus.SPV=0: Trap originated from V=0 (HS-mode itself)
```

#### Areas Requiring Extension of Existing Trap Handlers

1. **`m_trap_handler`**: Needs to identify VS-mode ecall (cause=10) and distinguish it from HS-mode ecall (cause=9)
2. **`s_trap_handler`** (when acting as HS-mode trap handler): Needs to check `hstatus.SPV`, record `htval`/`htinst`/`GVA`, and handle guest-page faults

---

### Module 4: G-stage Page Table Management (`common/hyp/gstage_pt.c`)

**Function**: Manage G-stage (second-stage) address translation page tables controlled by `hgatp`.

#### Key Differences Between G-stage and VS-stage

| Feature | VS-stage (`satp`/`vsatp`) | G-stage (`hgatp`) |
|---------|---------------------------|-------------------|
| Root page table size | 4KB | **16KB** (x4) |
| Root page table alignment | 4KB | **16KB** |
| GPA width | Virtual address width | Virtual address width **+2 bits** |
| U-bit behavior | Normal S/U check | **Always checked as U-mode** |
| G-bit | Normal global page | **Ignored**, reserved for future use |
| Fault type | Page-fault (12/13/15) | **Guest-page-fault** (20/21/23) |
| ID field | ASID | **VMID** |
| Page table scheme | Sv39/Sv48/Sv57 | Sv39**x4**/Sv48**x4**/Sv57**x4** |

#### G-stage Page Table Context

```c
typedef struct {
    int         mode;       /* HGATP_MODE_SV39X4 / SV48X4 / SV57X4 */
    uintptr_t  *root_pt;   /* Root page table physical address (16KB aligned) */
    int         levels;     /* Number of page table levels (3/4/5) */
    uintptr_t   map_base;
    uintptr_t   map_size;
    int         map_level;
} gpt_context_t;
```

#### Interface Definitions

```c
/**
 * gpt_init - Initialize G-stage page table context
 *
 * Allocates a 16KB root page table (4 consecutive 4KB pages) from the G-stage page table pool.
 */
void gpt_init(gpt_context_t *ctx, int mode);

/**
 * gpt_map_page - Map a single guest physical address → supervisor physical address
 *
 * Similar to pt_map_page, but uses G-stage page table format.
 * In G-stage, all accesses are treated as U-mode, and the U-bit is always checked.
 */
int gpt_map_page(gpt_context_t *ctx, uintptr_t gpa, uintptr_t spa,
                 uintptr_t flags, int level);

/**
 * gpt_setup_identity_mapping - Create GPA = SPA identity mapping
 *
 * Maps the region [base, base+size) so that GPA == SPA.
 * Note: The U-bit in G-stage PTE must be set to 1,
 * because G-stage translation always treats accesses as U-mode.
 */
int gpt_setup_identity_mapping(gpt_context_t *ctx, uintptr_t base,
                                uintptr_t size, uintptr_t flags, int level);

/**
 * gpt_pool_reset - Reset G-stage page table pool
 *
 * G-stage uses a page table pool independent of VS-stage.
 */
void gpt_pool_reset(void);

/**
 * gpt_enable - Enable G-stage address translation
 *
 * Writes to hgatp CSR and executes hfence.gvma.
 */
void gpt_enable(gpt_context_t *ctx, unsigned vmid);

/**
 * gpt_disable - Disable G-stage address translation
 *
 * Sets hgatp.MODE=Bare.
 */
void gpt_disable(void);
```

#### Page Table Pool Design

The G-stage root page table is 16KB (4 × 4KB pages), requiring an independent page table pool with 16KB-aligned root page table allocation:

```
Linker script new section:
.gstage_page_tables (NOLOAD) : {
    PROVIDE(__gpt_pool_start = .);
    . = ALIGN(0x4000);         /* 16KB alignment */
    . += 128 * 4096;           /* 512KB pool */
    PROVIDE(__gpt_pool_end = .);
}
```

---

### Module 5: Two-Stage Address Translation Management (`common/hyp/two_stage.c`)

**Function**: Combined management of VS-stage + G-stage two-stage translation, providing high-level APIs.

#### Two-Stage Translation Context

```c
typedef struct {
    pt_context_t   vs_ctx;   /* VS-stage: controlled by vsatp */
    gpt_context_t  g_ctx;    /* G-stage: controlled by hgatp */
} two_stage_ctx_t;
```

#### Interface Definitions

```c
/**
 * two_stage_init - Initialize two-stage translation
 *
 * @vs_mode: SATP_MODE_SV39/48/57 (VS-stage translation mode)
 * @g_mode:  HGATP_MODE_SV39X4/48X4/57X4 (G-stage translation mode)
 */
void two_stage_init(two_stage_ctx_t *ctx, int vs_mode, int g_mode);

/**
 * two_stage_setup_identity - Set up two-stage identity mapping
 *
 * Creates VA=GPA=SPA identity mappings for both VS-stage and G-stage simultaneously.
 * VS-stage: VA → GPA (vsatp)
 * G-stage:  GPA → SPA (hgatp)
 */
int two_stage_setup_identity(two_stage_ctx_t *ctx, uintptr_t base,
                              uintptr_t size, uintptr_t flags, int level);

/**
 * two_stage_run_in_vs - Execute a function under VS-mode + two-stage translation
 *
 * Complete flow:
 * 1. Configure PMP to allow S/VS/VU-mode access
 * 2. Configure trap delegation (medeleg, hedeleg)
 * 3. Enable G-stage translation (write hgatp)
 * 4. Enable VS-stage translation (write vsatp)
 * 5. Switch to VS-mode to execute fn(arg)
 * 6. After return, disable translation and restore state
 */
uintptr_t two_stage_run_in_vs(two_stage_ctx_t *ctx,
                               uintptr_t (*fn)(uintptr_t),
                               uintptr_t arg);

/**
 * two_stage_cleanup - Clean up two-stage translation state
 */
void two_stage_cleanup(two_stage_ctx_t *ctx);
```

#### Address Translation Path

```
When V=1, the address translation process for memory access:

  Guest Virtual Address (VA)
         |
         | VS-stage translation (vsatp)
         | Intermediate physical addresses during page table walk also go through G-stage
         ↓
  Guest Physical Address (GPA)
         |
         | G-stage translation (hgatp)
         ↓
  Supervisor Physical Address (SPA)
         |
         | PMP check
         ↓
  Physical Memory Access
```

---

### Module 6: Hypervisor CSR Operation API (`common/hyp/hyp_csr.c`)

**Function**: Provide convenient read/write and configuration interfaces for hypervisor CSRs.

#### hstatus Field Operations

```c
void     hstatus_set_vtsr(bool enable);   /* VTSR: VS-mode SRET → virtual-inst trap */
void     hstatus_set_vtw(bool enable);    /* VTW: VS-mode WFI → virtual-inst trap */
void     hstatus_set_vtvm(bool enable);   /* VTVM: VS-mode SFENCE/satp → virtual-inst trap */
void     hstatus_set_hu(bool enable);     /* HU: Allow U-mode to execute HLV/HSV */
void     hstatus_set_spvp(unsigned priv); /* SPVP: Effective privilege for HLV/HSV */

unsigned hstatus_get_spv(void);           /* SPV: V bit of trap origin */
unsigned hstatus_get_gva(void);           /* GVA: Whether stval is a guest VA */
```

#### Trap Delegation Operations

```c
/**
 * hyp_delegate_to_vs - Configure exception/interrupt delegation to VS-mode
 *
 * First ensures medeleg/mideleg delegate the corresponding trap to HS-mode,
 * then further delegates to VS-mode via hedeleg/hideleg.
 */
void hyp_delegate_to_vs(uintptr_t hedeleg_mask, uintptr_t hideleg_mask);

/**
 * hyp_undelegate - Clear all hypervisor delegations
 */
void hyp_undelegate(void);
```

#### Virtual Interrupt Injection

```c
void hvip_set_vssi(bool pending);   /* VS software interrupt */
void hvip_set_vsti(bool pending);   /* VS timer interrupt */
void hvip_set_vsei(bool pending);   /* VS external interrupt */
```

#### Other CSR Controls

```c
/* hcounteren: Controls VS/VU-mode access to hardware counters */
void hcounteren_set(uint32_t mask);
void hcounteren_clear(uint32_t mask);

/* htimedelta: Sets the offset value for time CSR in VS-mode */
void htimedelta_set(uint64_t delta);

/* henvcfg: Controls execution environment features when V=1 */
void henvcfg_set_pbmte(bool enable);  /* Svpbmt for VS-stage */
void henvcfg_set_adue(bool enable);   /* Hardware A/D update for VS-stage */
void henvcfg_set_stce(bool enable);   /* vstimecmp enable */
```

---

### Module 7: HFENCE Instruction Wrappers (`common/hyp/hyp_fence.c`)

**Function**: Wrap HFENCE.VVMA and HFENCE.GVMA instructions.

```c
/**
 * hfence_vvma - Flush VS-stage TLB
 *
 * Similar to sfence.vma but acts on VS-stage translation controlled by vsatp.
 * Only valid in M-mode or HS-mode; triggers virtual-instruction exception in VS/VU-mode.
 *
 * @vaddr: Guest virtual address to flush (0=all)
 * @asid:  Guest ASID to flush (0=all)
 */
void hfence_vvma(uintptr_t vaddr, uintptr_t asid);

/**
 * hfence_vvma_all - Global VS-stage TLB flush
 */
void hfence_vvma_all(void);

/**
 * hfence_gvma - Flush G-stage TLB
 *
 * Acts on G-stage translation controlled by hgatp.
 * Only valid in M-mode or HS-mode (mstatus.TVM=0).
 *
 * @gpa_shifted: Guest physical address >> 2 (0=all)
 * @vmid:        Virtual Machine ID (0=all)
 */
void hfence_gvma(uintptr_t gpa_shifted, uintptr_t vmid);

/**
 * hfence_gvma_all - Global G-stage TLB flush
 */
void hfence_gvma_all(void);
```

---

### Module 8: HLV/HLVX/HSV Instruction Wrappers (`common/hyp/hyp_ldst.c`)

**Function**: Wrap hypervisor virtual machine load/store instructions.

These instructions are valid in M-mode or HS-mode (and U-mode when HU=1), performing accesses to guest virtual memory using two-stage address translation, with effective privilege controlled by `hstatus.SPVP`.

```c
/* HLV — Read from guest virtual memory */
uint8_t  hlv_b(uintptr_t addr);    /* Signed byte */
uint8_t  hlv_bu(uintptr_t addr);   /* Unsigned byte */
uint16_t hlv_h(uintptr_t addr);    /* Signed halfword */
uint16_t hlv_hu(uintptr_t addr);   /* Unsigned halfword */
uint32_t hlv_w(uintptr_t addr);    /* Signed word */
uint64_t hlv_d(uintptr_t addr);    /* Doubleword (RV64 only) */

/* HLVX — Read with execute permission (for emulating instruction fetch) */
uint16_t hlvx_hu(uintptr_t addr);  /* Unsigned halfword, execute perm */
uint32_t hlvx_wu(uintptr_t addr);  /* Unsigned word, execute perm */

/* HSV — Write to guest virtual memory */
void hsv_b(uintptr_t addr, uint8_t val);
void hsv_h(uintptr_t addr, uint16_t val);
void hsv_w(uintptr_t addr, uint32_t val);
void hsv_d(uintptr_t addr, uint64_t val);  /* RV64 only */
```

#### Usage Constraints

- Executing HLV/HLVX/HSV when V=1 triggers a **virtual-instruction exception**
- Executing in U-mode with `hstatus.HU=0` triggers an **illegal-instruction exception**
- Effective privilege: VU (SPVP=0) or VS (SPVP=1)
- Address translation: Always uses two-stage translation (vsatp + hgatp)
- HS-level `sstatus.SUM` is ignored
- HS-level `sstatus.MXR` affects both stages; `vsstatus.MXR` only affects VS-stage

---

### Module 9: Hypervisor Test State Management (`common/hyp/hyp_reset.c`)

**Function**: State reset for hypervisor testing.

```c
/**
 * hyp_reset_state - Reset all hypervisor state to a clean baseline
 *
 * Reset contents:
 * 1. Ensure return to M-mode, V=0
 * 2. Clear hstatus (VTSR/VTW/VTVM/HU/SPV/SPVP/GVA)
 * 3. Clear hedeleg, hideleg
 * 4. Clear hvip (all virtual interrupt pending)
 * 5. Clear hie
 * 6. Set hgatp = Bare (disable G-stage)
 * 7. Clear htval, htinst
 * 8. Clear VS CSRs (vsstatus/vstvec/vsepc/vscause/vstval/vsatp)
 * 9. Clear henvcfg, hcounteren, htimedelta
 * 10. Execute hfence.gvma global flush
 * 11. Call reset_state() for basic reset
 */
void hyp_reset_state(void);
```

---

### Module 10: Hypervisor Test Macros (`common/hyp/hyp_test.h`)

**Function**: Provide hypervisor-specific test assertions and helper macros.

```c
/**
 * EXPECT_VIRTUAL_INST - Execute stmt, expecting a virtual-instruction exception
 *
 * Used to verify the behavior of control fields such as VTSR/VTW/VTVM.
 */
#define EXPECT_VIRTUAL_INST(stmt) do { \
    trap_expect_begin(); \
    stmt; \
    TEST_ASSERT("virtual-inst trap triggered", trap_was_triggered()); \
    if (trap_was_triggered()) { \
        TEST_ASSERT_EQ("cause=22", trap_get_cause(), CAUSE_VIRTUAL_INSTRUCTION); \
    } \
    trap_expect_end(); \
} while (0)

/**
 * EXPECT_GUEST_PAGE_FAULT - Execute stmt, expecting a guest-page fault
 */
#define EXPECT_GUEST_PAGE_FAULT(expected_cause, stmt) do { \
    trap_expect_begin(); \
    stmt; \
    TEST_ASSERT("guest-page-fault triggered", trap_was_triggered()); \
    if (trap_was_triggered()) { \
        TEST_ASSERT_EQ("cause matches", trap_get_cause(), (expected_cause)); \
    } \
    trap_expect_end(); \
} while (0)

/**
 * VS_EXPECT_NO_TRAP - Execute stmt in VS-mode, expecting no trap
 */
#define VS_EXPECT_NO_TRAP(stmt) do { \
    trap_expect_begin(); \
    stmt; \
    TEST_ASSERT("no trap in VS-mode", !trap_was_triggered()); \
    trap_expect_end(); \
} while (0)

/**
 * CHECK_HTVAL - Check htval value (guest physical address >> 2)
 */
#define CHECK_HTVAL(msg, expected_gpa_shifted) do { \
    TEST_ASSERT_EQ(msg, trap_get_htval(), (uintptr_t)(expected_gpa_shifted)); \
} while (0)

/**
 * CHECK_HTINST - Check htinst pseudoinstruction value
 */
#define CHECK_HTINST(msg, expected_pseudoinst) do { \
    TEST_ASSERT_EQ(msg, trap_get_htinst(), (uintptr_t)(expected_pseudoinst)); \
} while (0)

/**
 * CHECK_GVA - Check hstatus.GVA
 */
#define CHECK_GVA(msg, expected_gva) do { \
    TEST_ASSERT_EQ(msg, trap_get_gva(), (bool)(expected_gva)); \
} while (0)

/**
 * HYP_TEST_END - Hypervisor version of TEST_END
 *
 * Uses hyp_reset_state instead of reset_state.
 */
#define HYP_TEST_END() do { \
    goto_priv(PRIV_M); \
    hyp_reset_state(); \
    if (test_results.current_test_failed) { \
        test_results.tests_failed++; \
        if (test_results.failed_count < MAX_FAILED_TESTS) { \
            test_results.failed_names[test_results.failed_count++] = \
                test_results.current_test_name; \
        } \
        printf("[FAIL] %s\n\n", test_results.current_test_name); \
    } else { \
        test_results.tests_passed++; \
        printf("[PASS] %s\n\n", test_results.current_test_name); \
    } \
    return !test_results.current_test_failed; \
} while (0)
```

---

## Modifications to Existing Common Modules

### encoding.h

Add hypervisor CSR addresses (or include `hyp_defs.h`):

```c
/* Hypervisor CSRs */
#define CSR_HSTATUS     0x600
#define CSR_HEDELEG     0x602
#define CSR_HIDELEG     0x603

#define CSR_HGATP       0x680
/* ... all hypervisor CSRs ... */

/* Hypervisor cause codes */
#define CAUSE_ECALL_FROM_VS             10
#define CAUSE_INST_GUEST_PAGE_FAULT     20
#define CAUSE_LOAD_GUEST_PAGE_FAULT     21
#define CAUSE_VIRTUAL_INSTRUCTION       22
#define CAUSE_STORE_GUEST_PAGE_FAULT    23
```

### csr_accessors.c

Add `csr_read`/`csr_write` switch cases for all hypervisor CSRs and VS CSRs:

```c
/* Added: Hypervisor CSRs */
_CSR_READ_CASE(0x600)  /* hstatus */
_CSR_READ_CASE(0x602)  /* hedeleg */
_CSR_READ_CASE(0x603)  /* hideleg */
_CSR_READ_CASE(0x680)  /* hgatp */
/* ... */

/* Added: VS CSRs */
_CSR_READ_CASE(0x200)  /* vsstatus */
_CSR_READ_CASE(0x280)  /* vsatp */
/* ... */
```

### trap.c

Extend `m_trap_handler` and `s_trap_handler`:

1. `m_trap_handler`: Identify `CAUSE_ECALL_FROM_VS` (10)
2. `s_trap_handler` (as HS-mode handler): Check `hstatus.SPV`, record `htval`/`htinst`/`GVA`

### trap_asm.S

May need to add VS-mode trap entry point (if trap delegation to VS-mode test scenarios are required).

### test_framework.h

Add virtualization privilege level definitions:

```c
#define PRIV_VS  5   /* Virtual Supervisor mode (V=1, S) */
#define PRIV_VU  4   /* Virtual User mode (V=1, U) */
```

### Makefile.common

Add `ENABLE_HYP` conditional compilation support:

```makefile
ifdef ENABLE_HYP
HYP_DIR = $(COMMON_DIR)/hyp
HYP_OBJS = $(HYP_DIR)/hyp_csr.o $(HYP_DIR)/hyp_priv.o \
            $(HYP_DIR)/hyp_trap.o $(HYP_DIR)/hyp_trap_asm.o \
            $(HYP_DIR)/hyp_fence.o $(HYP_DIR)/hyp_ldst.o \
            $(HYP_DIR)/gstage_pt.o $(HYP_DIR)/two_stage.o \
            $(HYP_DIR)/hyp_reset.o
COMMON_OBJS += $(HYP_OBJS)
CFLAGS += -DENABLE_HYP -I$(HYP_DIR)
MARCH = rv64imac_zicsr_zifencei_h   # Add H extension
endif
```

---

## Core Verification Areas for Test Coverage

| Area | Involved Modules | Key Test Points |
|------|------------------|-----------------|
| **CSR Access Control** | hyp_csr | Access permissions and virtual-instruction exceptions for hstatus/hedeleg/hideleg/hgatp and other CSRs in M/HS/VS/VU modes |
| **Privilege Level Switching** | hyp_priv | Bidirectional M→HS→VS→VU switching, correct V bit setting, correct SPV/SPP updates |
| **Virtual-instruction Exception** | hyp_trap | VTSR (VS-mode SRET trap), VTW (VS-mode WFI trap), VTVM (VS-mode SFENCE/satp trap), VS/VU-mode access to hypervisor CSR/VS CSR |
| **Trap Delegation** | hyp_csr, hyp_trap | Two-level delegation medeleg→hedeleg, distinguishing VS-mode ecall (cause=10) vs HS-mode ecall (cause=9) |
| **G-stage Page Table Translation** | gstage_pt | Sv39x4/48x4/57x4 translation correctness, 16KB root page table, guest-page fault (cause=20/21/23), U-bit always checked |
| **Two-Stage Address Translation** | two_stage | Full VA→GPA→SPA chain verification, VS-stage + G-stage permission interaction, G-stage fault during VS-stage page table walk |
| **HLV/HLVX/HSV** | hyp_ldst | Virtual machine load/store in HS-mode/U-mode(HU=1), effective privilege controlled by SPVP, virtual-instruction exception when V=1 |
| **HFENCE Instructions** | hyp_fence | TLB flush effects of HFENCE.VVMA/GVMA, virtual-instruction exception when executed in VS/VU-mode |
| **Virtual Interrupt Injection** | hyp_csr | hvip injection of VSSI/VSTI/VSEI, hideleg interrupt delegation to VS-mode, interrupt number translation (10→9, 6→5, 2→1) |
| **henvcfg Control** | hyp_csr | Effects of PBMTE (Svpbmt for VS-stage), ADUE (hardware A/D update), STCE (vstimecmp enable) on VS-mode |
| **hcounteren** | hyp_csr | Counter access control, virtual-instruction exception trigger conditions when V=1 |
| **htimedelta** | hyp_csr | time CSR read in VS/VU-mode = time + htimedelta |
| **htval/htinst** | hyp_trap | htval (GPA>>2) and htinst transformed instruction / pseudoinstruction during guest-page fault |
| **mstatus Extensions** | hyp_csr | MPV/GVA fields, MPRV + MPV interaction (two-stage translation as-if V=1) |

---

## Test Case Writing Patterns

### Basic Pattern (No VM)

```c
#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_test.h"

TEST_REGISTER(test_vtsr_traps_sret_in_vs);
bool test_vtsr_traps_sret_in_vs(void) {
    TEST_BEGIN("VTSR-01: SRET in VS-mode triggers virtual-inst when VTSR=1");

    /* Set VTSR=1 */
    hstatus_set_vtsr(true);

    /* Execute SRET in VS-mode, expecting virtual-instruction exception */
    goto_vs_mode();
    EXPECT_VIRTUAL_INST(asm volatile("sret"));
    goto_priv(PRIV_M);

    HYP_TEST_END();
}
```

### Two-Stage Translation Pattern

```c
#include "test_framework.h"
#include "hyp/hyp_defs.h"
#include "hyp/hyp_test.h"
#include "vm/vm.h"

TEST_REGISTER(test_two_stage_identity_mapping);
bool test_two_stage_identity_mapping(void) {
    TEST_BEGIN("2STAGE-01: Two-stage identity mapping read/write");

    two_stage_ctx_t ctx;
    two_stage_init(&ctx, SATP_MODE_SV39, HGATP_MODE_SV39X4);

    uintptr_t base = PLATFORM_MEM_BASE & ~(PAGE_SIZE_1G - 1);
    uintptr_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D;
    int ret = two_stage_setup_identity(&ctx, base, PAGE_SIZE_1G,
                                        flags, PT_LEVEL_1G);
    TEST_ASSERT("identity mapping setup", ret == 0);

    uintptr_t result = two_stage_run_in_vs(&ctx, test_vs_read_write,
                                            (uintptr_t)test_data);
    TEST_ASSERT("VS-mode read/write succeeds", result == 0);

    two_stage_cleanup(&ctx);
    HYP_TEST_END();
}
```

### HLV/HSV Test Pattern

```c
TEST_REGISTER(test_hlv_in_hs_mode);
bool test_hlv_in_hs_mode(void) {
    TEST_BEGIN("HLV-01: HLV.W reads guest memory from HS-mode");

    /* Set up G-stage mapping */
    gpt_context_t g_ctx;
    gpt_pool_reset();
    gpt_init(&g_ctx, HGATP_MODE_SV39X4);
    /* ... setup mapping ... */
    gpt_enable(&g_ctx, 0);

    /* Set SPVP=1 (VS-level access) */
    hstatus_set_spvp(1);

    /* Use HLV.W to read guest memory from HS-mode */
    goto_priv(PRIV_S);  /* HS-mode */
    uint32_t val = hlv_w(test_addr);
    goto_priv(PRIV_M);

    TEST_ASSERT_EQ("HLV read correct value", val, expected_val);

    gpt_disable();
    gpt_pool_reset();
    HYP_TEST_END();
}
```

---

## Key Design Decisions and Considerations

### 1. Privilege Level Encoding

The existing framework uses `PRIV_U=0, PRIV_S=1, PRIV_M=3` directly corresponding to RISC-V nominal privilege. The VS/VU modes added by the Hypervisor extension require additional encoding space to distinguish V=0 from V=1. It is recommended to use high bits to mark the V bit: `PRIV_VS = 5 (V=1|S=1)`, `PRIV_VU = 4 (V=1|U=0)`.

### 2. G-stage Page Table Pool

The G-stage root page table is 16KB (4 consecutive 4KB pages), requiring:
- A page table pool independent of VS-stage (both may be used simultaneously)
- Pool start address 16KB aligned
- Root page table allocation must return a 16KB aligned address

### 3. Trap Handler Layering

```
M-mode handler (highest priority, handles undelegated traps):
  └── HS-mode handler (handles traps delegated by medeleg):
       └── VS-mode handler (handles traps delegated by hedeleg)
```

The test framework needs to be able to configure delegation at each layer and capture trap information at the correct layer.

### 4. World Switch Order

According to the specification, when switching VMID, the following order must be followed to prevent speculative execution from polluting the TLB:

```
1. vsatp = 0          (Clear VS-stage translation first)
2. hgatp = new value  (Switch G-stage and VMID)
3. vsatp = new value  (Restore VS-stage translation)
```

### 5. QEMU Platform Support

The QEMU virt platform requires `-cpu rv64,h=true` (or newer versions enable H extension by default) to run hypervisor tests. The Makefile needs to adjust QEMU_OPTS accordingly.

---

## H Sub-extension Common Test Infrastructure (v2 Incremental)

### Design Motivation

The 8 H sub-extensions — `Sha / Shtvala / Shvstvala / Shvstvecd / Shvsatpa / Shgatpa / Shcounterenw / Shlcofideleg` — are almost all "tightening an implementation-defined choice on the H baseline to mandatory" from a specification perspective. Their demands on the test framework are highly isomorphic:
- Whether the platform implements the sub-extension → Determines whether the entire test suite is SKIP or RUN;
- Triggering a certain trap path → Reuses H baseline VS-mode triggering + delegation configuration;
- Checking CSR fields at trap entry instant (htval / vstval / vsepc / hstatus.GVA / htinst …) → Reuses trap_record;
- Implementation width / MODE subset → Obtained through WARL probing.

To avoid piling up extension-named APIs in `common/hyp/` every time a sub-extension is integrated, this section defines common infrastructure named by **"behavioral semantics" rather than "extension name"**.

> **Design Principle**: The common layer only contains capabilities that "the H baseline should already have"; the sub-extension layer only contains "normative tightening unique to that extension". The workload for integrating a new sub-extension is constrained to 5–30 lines of header per extension.

### Module 11: Platform Capability Matrix (`common/hyp/platform_caps.h` / `.c`)

```c
typedef struct {
    /* CSR implementation width (obtained via WARL probing, 0 means not probed) */
    uint8_t  htval_width_bits;
    uint8_t  vstval_width_bits;
    uint8_t  hgatp_vmid_width;
    uint8_t  vsatp_asid_width;

    /* MODE support bitmap (each bit represents whether a MODE is accepted by WARL) */
    uint16_t hgatp_modes;     /* bit BARE/SV39X4/SV48X4/SV57X4   */
    uint16_t vsatp_modes;     /* bit BARE/SV39/SV48/SV57         */
    uint16_t stvec_modes;     /* bit0=Direct, bit1=Vectored       */

    /* Sub-extension implementation bits (compile-time macro OR runtime probing) */
    bool has_sha,           has_shtvala,    has_shvstvala,
         has_shvstvecd,     has_shvsatpa,   has_shgatpa,
         has_shcounterenw,  has_shlcofideleg;
} platform_caps_t;

extern platform_caps_t g_caps;
void platform_caps_probe(void);          /* Called once at test suite main entry */

/**
 * platform_caps_init - Initialize platform capability matrix
 *
 * Probes CSR widths and MODE support via WARL, detects sub-extension implementation.
 */
void platform_caps_init(platform_caps_t *caps);

/**
 * platform_has_ext - Check if a sub-extension is implemented
 */
bool platform_has_ext(const platform_caps_t *caps, unsigned ext_id);
```

### Module 12: Sub-extension REQUIRE Macros (`common/hyp/ext_require.h`)

Unified conditional skip macros for test cases:

```c
/**
 * REQUIRE_EXT - Skip test if sub-extension is not implemented
 *
 * Usage: REQUIRE_EXT(has_shtvala);
 */
#define REQUIRE_EXT(field) do { \
    if (!platform_caps.field) { \
        TEST_SKIP("requires " #field); \
        return true; \
    } \
} while (0)

/**
 * REQUIRE_HGATP_MODE - Skip if specific hgatp MODE is not supported
 */
#define REQUIRE_HGATP_MODE(mode_bit) do { \
    if (!(platform_caps.hgatp_modes & (1U << (mode_bit)))) { \
        TEST_SKIP("hgatp mode " #mode_bit " not supported"); \
        return true; \
    } \
} while (0)

/**
 * REQUIRE_STVEC_MODE - Skip if specific stvec MODE is not supported
 */
#define REQUIRE_STVEC_MODE(mode_bit) do { \
    if (!(platform_caps.stvec_modes & (1U << (mode_bit)))) { \
        TEST_SKIP("stvec mode " #mode_bit " not supported"); \
        return true; \
    } \
} while (0)

/**
 * REQUIRE_VSATP_MODE - Skip if specific vsatp MODE is not supported
 */
#define REQUIRE_VSATP_MODE(mode_bit) do { \
    if (!(platform_caps.vsatp_modes & (1U << (mode_bit)))) { \
        TEST_SKIP("vsatp mode " #mode_bit " not supported"); \
        return true; \
    } \
} while (0)
```

### Module 13: Delegation Wrapper APIs (`common/hyp/hyp_csr.c` Extension)

Encapsulate two-level delegation configuration to avoid repetitive medeleg+hedeleg setup in test cases:

```c
/* Cause mask definitions */
#define MASK_GPF          ((1UL << 20) | (1UL << 21) | (1UL << 23))
#define MASK_VIRT_INST    (1UL << 22)
#define MASK_VS_PF        ((1UL << 12) | (1UL << 13) | (1UL << 15))
#define MASK_VS_ECALL     (1UL << 10)
#define MASK_LCOFI_INT    (1UL << 13)

/**
 * delegate_causes_to_hs - Delegate exception causes to HS-mode
 *
 * Sets corresponding bits in medeleg to delegate traps from M-mode to HS-mode.
 */
void delegate_causes_to_hs(uintptr_t cause_mask);

/**
 * delegate_causes_to_vs - Delegate exception causes to VS-mode
 *
 * First ensures medeleg delegates to HS-mode, then sets hedeleg to further delegate to VS-mode.
 */
void delegate_causes_to_vs(uintptr_t cause_mask);

/**
 * delegate_ints_to_vs - Delegate interrupt causes to VS-mode
 *
 * First ensures mideleg delegates to HS-mode, then sets hideleg to further delegate to VS-mode.
 */
void delegate_ints_to_vs(uintptr_t int_mask);
```

### Module 14: VS-mode Trigger Functions (`common/hyp/hyp_priv.c` Extension)

Provide standard VS-mode operation trigger functions for reuse across sub-extension tests:

```c
/* Memory access triggers — used with two_stage_build(FAULT_*) */
uintptr_t test_vs_load_expect_fault (uintptr_t addr);              /* Load access */
uintptr_t test_vs_store_expect_fault(uintptr_t addr);              /* Store access */
uintptr_t test_vs_exec_expect_fault (uintptr_t addr);              /* Instruction fetch */

/* Privileged instruction triggers — trigger virtual-inst */
uintptr_t test_vs_sret             (uintptr_t unused);              /* For VTSR */
uintptr_t test_vs_wfi              (uintptr_t unused);              /* For VTW  */
uintptr_t test_vs_sfence_vma       (uintptr_t va_asid);             /* For VTVM */
```

### Module 15: Two-Stage Translation "Fault Injection" Interface (`common/hyp/two_stage.*` Refactored Extension)

Abstract the original "identity mapping" + "set by victim flags" pattern into **fault type + mode parameters**:

```c
typedef enum {
    FAULT_NONE,
    FAULT_VS_STAGE_LOAD,        /* VS-stage page fault: Shvstvala / baseline */
    FAULT_VS_STAGE_STORE,
    FAULT_G_STAGE_EXPLICIT,     /* G-stage explicit access page fault: Shtvala main path */
    FAULT_G_STAGE_IMPLICIT,     /* Implicit VS-stage: Shtvala second segment */
    FAULT_GPA_HIGH_BITS,        /* Sv*x4 high-bit overflow: Shared by Shgatpa / Shtvala */
    FAULT_STRADDLE,             /* Cross-page: Shared by Shtvala / Shvstvala */
} fault_kind_t;

typedef struct {
    int           vs_mode;        /* SATP_MODE_BARE/SV39/SV48/SV57 */
    int           g_mode;         /* HGATP_MODE_BARE/SV39X4/...    */
    fault_kind_t  fault;
    uintptr_t     fault_gva;      /* Input or framework-calculated */
    uintptr_t     fault_gpa;      /* Framework write-back: test case directly uses CHECK_HTVAL_EQ_SHIFTED */
    size_t        access_bytes;   /* Used for misaligned/straddle */
} two_stage_scene_t;

int  two_stage_build   (two_stage_ctx_t *ctx, two_stage_scene_t *scene);
void two_stage_activate(two_stage_ctx_t *ctx);
```

> Test cases only declare "which fault I want", and the framework returns the expected GPA/GVA. Shared by Shtvala / Shgatpa / Shvstvala / baseline two-stage translation.
>
> **Current Status**: v1's `two_stage_init` only supports `vs_mode == SATP_MODE_BARE`, so `FAULT_VS_STAGE_*` / `FAULT_G_STAGE_IMPLICIT` require `TEST_SKIP("requires VS-stage")` on the test case side until the framework completes VS-stage support.

### Module 16: Common Trap Field Assertion Macros (`common/hyp/hyp_test.h` Extension)

Named by CSR field + variant, providing 4 variants per field: `_EQ / _NONZERO / _ZERO / _SHIFTED`:

```c
/* Already existing: CHECK_HTVAL / CHECK_HTINST / CHECK_GVA */

/* v2 additions — htval (GPA>>2) series */
#define CHECK_HTVAL_EQ(msg, exp_shifted)    CHECK_HTVAL(msg, exp_shifted)
#define CHECK_HTVAL_SHIFTED(msg, exp_gpa)   CHECK_HTVAL(msg, (uintptr_t)(exp_gpa) >> 2)
#define CHECK_HTVAL_NONZERO(msg)            TEST_ASSERT_NEQ(msg, trap_get_htval(), 0UL)
#define CHECK_HTVAL_ZERO(msg)               TEST_ASSERT_EQ (msg, trap_get_htval(), 0UL)

/* htinst */
#define CHECK_HTINST_NONZERO(msg)           TEST_ASSERT_NEQ(msg, trap_get_htinst(), 0UL)
#define CHECK_HTINST_ZERO(msg)              TEST_ASSERT_EQ (msg, trap_get_htinst(), 0UL)

/* hstatus.GVA */
#define CHECK_GVA_SET(msg)                  TEST_ASSERT(msg, trap_get_gva())
#define CHECK_GVA_CLEAR(msg)                TEST_ASSERT(msg, !trap_get_gva())

/* vstval / vsepc / vscause — For Shvstvala / Shvstvecd, enabled after trap_record adds these fields */
```

> Shtvala primarily uses the HTVAL series; Shvstvala uses the VSTVAL series; Shvstvecd uses VSEPC + vector; **completely symmetric**.

### Module 17: CSR WARL/MODE Common Probing (`common/hyp/hyp_csr.*` Extension)

```c
/* Write ~0, read back to get actual WARL-retained bits */
uintptr_t csr_warl_probe       (unsigned csr_num);
unsigned  csr_field_width      (unsigned csr_num, unsigned shift, uintptr_t mask);
unsigned  csr_mode_field_supported(unsigned csr_num,
                                   unsigned mode_shift,
                                   unsigned mode_max);  /* Returns MODE support bitmap */
```

### Module 18: Sub-extension Conditional Compilation Unified Form (`Makefile.common` Extension)

```makefile
# One independent switch per sub-extension, accumulated into HYP_EXT_FLAGS
HYP_EXT_FLAGS :=
ifdef ENABLE_SHA          ; HYP_EXT_FLAGS += -DENABLE_SHA          ; endif
ifdef ENABLE_SHTVALA      ; HYP_EXT_FLAGS += -DENABLE_SHTVALA      ; endif
ifdef ENABLE_SHVSTVALA    ; HYP_EXT_FLAGS += -DENABLE_SHVSTVALA    ; endif
ifdef ENABLE_SHVSTVECD    ; HYP_EXT_FLAGS += -DENABLE_SHVSTVECD    ; endif
ifdef ENABLE_SHVSATPA     ; HYP_EXT_FLAGS += -DENABLE_SHVSATPA     ; endif
ifdef ENABLE_SHGATPA      ; HYP_EXT_FLAGS += -DENABLE_SHGATPA      ; endif
ifdef ENABLE_SHCOUNTERENW ; HYP_EXT_FLAGS += -DENABLE_SHCOUNTERENW ; endif
ifdef ENABLE_SHLCOFIDELEG ; HYP_EXT_FLAGS += -DENABLE_SHLCOFIDELEG ; endif
CFLAGS  += $(HYP_EXT_FLAGS)
ASFLAGS += $(HYP_EXT_FLAGS)
```

### Sub-extension Registration Table

When adding a new sub-extension, only register one row in this table + one `common/hyp/ext/<name>.h` (5–30 lines):

| Sub-extension | REQUIRE Macro | Primary Common API Reuse | Primary Trap Fields | Primary Cause / Delegation Mask |
|---------------|---------------|--------------------------|---------------------|---------------------------------|
| Sha             | `SHA_REQUIRE`           | All                                        | —                        | — |
| Shtvala         | `SHTVALA_REQUIRE`       | `delegate_causes_to_hs(MASK_GPF)` / `two_stage_build(FAULT_G_STAGE_*)` / `CHECK_HTVAL_NONZERO` | `htval`, `htinst`, `GVA` | `MASK_GPF` |
| Shvstvala       | `SHVSTVALA_REQUIRE`     | `delegate_causes_to_vs(MASK_VS_PF\|MASK_VIRT_INST)` / `CHECK_VSTVAL_NONZERO` | `vstval`, `vscause` | `MASK_VS_PF \| MASK_VIRT_INST` |
| Shvstvecd       | `SHVSTVECD_REQUIRE`     | `REQUIRE_STVEC_MODE(0)` / `delegate_causes_to_vs(...)` | `vsepc`, `vscause` | `MASK_VS_PF` |
| Shvsatpa        | `SHVSATPA_REQUIRE`      | `csr_mode_field_supported(CSR_VSATP, ...)` | —                        | — |
| Shgatpa         | `SHGATPA_REQUIRE`       | `REQUIRE_HGATP_MODE(*)` / `two_stage_build(FAULT_GPA_HIGH_BITS)` | `htval` | `MASK_GPF` |
| Shcounterenw    | `SHCOUNTERENW_REQUIRE`  | `csr_warl_probe(CSR_HCOUNTEREN)`           | —                        | — |
| Shlcofideleg    | `SHLCOFIDELEG_REQUIRE`  | `delegate_ints_to_vs(MASK_LCOFI_INT)`      | —                        | `MASK_LCOFI_INT` |

### Comparison with v1 Naming (To Prevent Future Naming Drift)

| v1 Draft (Shtvala-specific) | v2 Generalized Naming |
|-----------------------------|----------------------|
| `delegate_gpf_to_hs()` | `delegate_causes_to_hs(MASK_GPF)` |
| `delegate_nongpf_traps_to_hs()` | `delegate_causes_to_hs(<other masks>)` |
| `two_stage_setup_bare_vsatp_*x4_hgatp()` × 3 | `two_stage_build(scene{ .fault=FAULT_G_STAGE_EXPLICIT, .g_mode=SV*X4 })` |
| `htval_warl_probe()` / `htval_impl_width_bits()` | `csr_warl_probe(CSR_HTVAL)` / `csr_field_width(CSR_HTVAL,...)` |
| `hgatp_supported_modes_mask()` | `csr_mode_field_supported(CSR_HGATP, HGATP_MODE_SHIFT, 15)` |
| `SHTVALA_REQUIRE` scattered in hyp_test.h | `REQUIRE_EXT(has_shtvala)` + sub-extension header |
| `CHECK_HTVAL_NONZERO/_ZERO/_RECONSTRUCT_GPA` | Same-named macros, but aligned with `CHECK_VSTVAL_*` / `CHECK_VSEPC_*` templates |
| Sub-extensions scattered directly in `hyp_csr.c` | `common/hyp/ext/<name>.h` isolated, one file per extension |
| Makefile single switch `ENABLE_SHTVALA` | 8 sub-extensions unified form `ENABLE_SH<NAME>` + `HYP_EXT_FLAGS` |

### Test Directory Convention

```
Shtvala/           # Implemented: Shtvala test suite
Shvstvala/         # Implemented: Shvstvala test suite
Shvstvecd/         # Implemented: Shvstvecd test suite
Shvsatpa/          # Implemented: Shvsatpa test suite
Shgatpa/           # Implemented: Shgatpa test suite
Shcounterenw/      # Implemented: Shcounterenw test suite
Shlcofideleg/      # Implemented: Shlcofideleg test suite
Sha/               # Implemented: Sha test suite
```

> **Note**: Directory names use capitalized first letter form (e.g., `Shtvala/`), consistent with Sv*/Ss*/Sm* extension directory naming conventions.

Each subdirectory has one `Makefile`, uniformly `include ../common/Makefile.common` and enabling its own `ENABLE_SH<NAME>`.

### Implementation Status (as of v2)

| Module | Status |
|--------|--------|
| Module 11 platform_caps_t  | TODO (minimum set: has_shtvala / hgatp_modes / htval_width_bits) |
| Module 12 REQUIRE macros       | TODO (first define SHTVALA_REQUIRE locally in Shtvala module, promote when framework converges) |
| Module 13 Delegation wrappers         | TODO (v1 already has hedeleg_write, Shtvala module directly uses medeleg+CSRR/W to set) |
| Module 14 VS-mode triggers   | Already have `test_vs_load_expect_fault/store_expect_fault/exec_expect_fault`; remaining are v2 additions |
| Module 15 Fault injection interface     | Already have `_setup_with_victim` pattern (Sv39x4), can serve as `FAULT_G_STAGE_EXPLICIT` implementation |
| Module 16 Trap field assertion macros  | Already have `CHECK_HTVAL/CHECK_HTINST/CHECK_GVA`; NONZERO/ZERO/SET/CLEAR completed in this update |
| Module 17 CSR WARL probing    | TODO |
| Module 18 Makefile sub-extension switches | Converged to `ENABLE_HYP`; sub-extension switches added as needed in v2 phase |

> When subsequently integrating extensions such as Shvstvala / Shgatpa, **prioritize implementing the corresponding TODO items in Modules 11/13/15/17**, then add the extension's own ext header; do not反过来 stuff specialized APIs into the sub-extension.

---

## Hypervisor Baseline Test Suite (`Hypervisor/`)

### Overview

The `Hypervisor/` directory contains a complete baseline test suite written according to `DOCS/testplan/Hypervisor_test_plan.md`, covering 19 test chapters and approximately 225 test cases. The test suite verifies core functionalities of the RISC-V Hypervisor Extension including CSR behavior, trap mechanisms, interrupt delegation, and address translation.

### Directory Structure

```
Hypervisor/
├── Makefile                    # ENABLE_HYP=1, ENABLE_VM=1
├── kernel.ld                   # .test_table / .vm_test_region / .gpt_page_tables
├── main.c                      # Iterates _test_table[] to execute all tests
└── tests/
    ├── test_helpers.h          # Common helper function declarations
    ├── test_helpers.c          # VS/VU trampolines, delegation configuration helpers
    ├── test_register.c         # #includes all test files, triggers TEST_REGISTER
    ├── test_csr_basics.c       # Ch.1  VCSR-01~17  VS CSR aliasing behavior
    ├── test_hstatus.c          # Ch.2  HSTAT-01~26 hstatus register
    ├── test_delegation.c       # Ch.3  DELEG-01~16 hedeleg/hideleg
    ├── test_interrupts.c       # Ch.4-5 HINT-01~14 + HGEI-01~05
    ├── test_henvcfg.c          # Ch.6  HENV-01~14  henvcfg
    ├── test_hcounteren.c       # Ch.7  HCNT-01~08  hcounteren
    ├── test_htimedelta.c       # Ch.8  HTDL-01~04  htimedelta
    ├── test_vsip_vsie.c        # Ch.9  VSIE-01~10  vsip/vsie alias
    ├── test_vstimecmp.c        # Ch.10 VSTC-01~07  vstimecmp
    ├── test_vs_scratch.c       # Ch.11 VSCR-01~06  vsscratch/vsepc/vscause/vstval
    ├── test_virtual_inst.c     # Ch.12 VINST-01~24 virtual-instruction exception
    ├── test_trap_entry.c       # Ch.13 TENT-01~15  trap entry CSR writes
    ├── test_trap_return.c      # Ch.14 TRET-01~15  MRET/SRET mode switching
    ├── test_htinst.c           # Ch.15 TINST-01~09 htinst transformed instruction
    ├── test_mstatus_hyp.c      # Ch.16 MSTAT-01~14 mstatus enhancements
    ├── test_mideleg_enhance.c  # Ch.17 MIDLG-01~07 mideleg/mip/mie enhancements
    ├── test_mtval2.c           # Ch.18 MTVAL-01~05 mtval2/mtinst
    └── test_exception_priority.c # Ch.19 PRIO-01~05 exception priority
```

### VS/VU-mode Trampoline Design

Since tests run in M-mode, trampoline functions are needed to execute specific operations in VS/VU-mode. `test_helpers.c` provides a set of trampoline functions, each with signature `uintptr_t fn(uintptr_t arg)`, called via `run_in_vs_mode(fn, arg)` or `run_in_vu_mode(fn, arg)`.

**Trampoline Function Categories**:

| Category | Functions | Purpose |
|----------|-----------|---------|
| S CSR Read | `vs_read_sstatus`, `vs_read_sie`, `vs_read_sip`, `vs_read_satp`, etc. | Access S CSRs when V=1 (actually mapped to VS CSRs) |
| S CSR Write | `vs_write_sstatus`, `vs_write_satp`, `vs_write_sie`, etc. | Write S CSRs when V=1 |
| H CSR Read | `vs_read_hstatus`, `vs_read_hedeleg`, `vs_read_hgatp` | VS-mode access to H CSRs (should trigger virtual-inst) |
| VS CSR Direct | `vs_read_vsstatus_direct` | VS-mode direct access to vsstatus (should trigger virtual-inst) |
| Privileged Instructions | `vs_exec_sret`, `vs_exec_wfi`, `vs_exec_sfence_vma`, `vs_exec_sinval_vma` | Test VTSR/VTW/VTVM controls |
| HLV/HSV | `vs_exec_hlv_w`, `vs_exec_hsv_w`, `vs_exec_hlvx_wu` | Execute virtual machine load/store in VS/VU-mode |
| HFENCE | `vs_exec_hfence_vvma`, `vs_exec_hfence_gvma` | Execute HFENCE in VS-mode (should trigger virtual-inst) |
| Ecall | `vs_exec_ecall`, `vs_exec_ebreak` | Trigger ecall/ebreak exceptions |
| Counter | `vs_read_cycle`, `vs_read_time`, `vs_read_instret` | Access counter CSRs |

### Trap Entry/Return Test Verification Strategy

Trap entry tests (TENT-01~15) verify automatic CSR writes through the following approach:

1. **trap_expect_begin()** marks the start of expected trap
2. **run_in_vs_mode/run_in_vu_mode** triggers the trap (e.g., ecall)
3. **trap_was_triggered()** confirms trap occurrence
4. **trap_get_cause/spv/gva/htval/htinst()** reads trap record fields
5. **hstatus_read()** checks SPV/SPVP/GVA bit fields

Trap return tests (TRET-01~15) leverage the underlying MRET/SRET mechanism of `run_in_vs_mode`/`run_in_vu_mode` to verify correct mode switching.

### Interrupt Injection Test Implementation Approach

Interrupt tests (HINT/HGEI/VSIE chapters) primarily verify CSR read/write behavior and alias relationships:

- **hvip injection**: Set pending bits via `hvip_set_vssi/vsti/vsei()`, verify alias by reading `hip` (0x644) via inline asm
- **hie/hip access**: Framework does not provide `hip_read/hie_write` functions; tests use inline asm to directly access CSRs
- **hideleg control**: Verify writability of delegation bits via `hideleg_write/read()`
- **hgeip/hgeie**: Access CSRs 0xE12/0x607 via inline asm

> [!NOTE]
> Some complex scenarios (such as interrupt priority arbitration, vstimecmp real timer interrupts, MPRV+MPV two-stage translation) are simplified to CSR read/write verification or TEST_SKIP in baseline tests, and can be supplemented in integration tests later.

### Compilation and Execution

```bash
cd Hypervisor
make PLATFORM=qemu_virt        # Compile
make qemu                      # Run on QEMU (requires rv64,h=true)
```

QEMU CPU configuration: `rv64,h=true,sv39=true,sv48=true,sv57=true`

---

## References

- RISC-V Privileged Specification — Hypervisor Extension (`SPEC/hypervisor.adoc`)
- `docs/vm_test_framework.md` — Existing VM test framework documentation
- `docs/vm_test_plan.md` — VM test plan (refer to its test organization approach)
- `common/test_framework.h` — Existing test framework API
- `common/vm/vm.h` — Existing VM management API
- `common/privilege.c` — Existing privilege level switching implementation
- `common/trap.c` — Existing trap handler implementation
