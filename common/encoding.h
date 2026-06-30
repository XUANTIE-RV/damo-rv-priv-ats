/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PMP_ENCODING_H
#define PMP_ENCODING_H

/* ===================================================================
 * RISC-V CSR Encoding Constants (PMP-focused subset)
 * =================================================================== */

/* ----- Machine-level CSRs ----- */
#define CSR_MSTATUS     0x300
#define CSR_MISA        0x301
#define CSR_MEDELEG     0x302
#define CSR_MIDELEG     0x303
#define CSR_MIE         0x304
#define CSR_MTVEC       0x305
#define CSR_MCOUNTEREN  0x306
#define CSR_MSCRATCH    0x340
#define CSR_MEPC        0x341
#define CSR_MCAUSE      0x342
#define CSR_MTVAL       0x343
#define CSR_MIP         0x344
#define CSR_MHARTID     0xF14
#define CSR_MVENDORID   0xF11

/* ----- Machine Environment Configuration ----- */
#define CSR_MENVCFG     0x30A
#define CSR_MENVCFGH    0x31A   /* RV32 only: upper 32 bits of menvcfg */

/* menvcfg field bits */
#define MENVCFG_PBMTE   (1ULL << 62)    /* Page-Based Memory Types Enable (Svpbmt) */
#define MENVCFG_ADUE    (1ULL << 61)    /* A/D Update Enable (Svadu) */
#define MENVCFG_LPE     (1ULL << 2)    /* Landing Pad Enable for S-mode (Zicfilp) */
#define MENVCFG_SSE     (1ULL << 3)    /* Shadow Stack Enable for S-mode (Zicfiss) */
#define MENVCFG_PMM_OFF  32             /* PMM field offset in menvcfg */
#define MENVCFG_PMM_MASK (3ULL << 32)   /* PMM field mask [33:32] (Smnpm) */

/* ----- Machine Security Configuration ----- */
#define CSR_MSECCFG     0x747
#define CSR_MSECCFGH    0x757   /* RV32 only: upper 32 bits of mseccfg */

/* mseccfg field bits */
#define MSECCFG_MML     (1ULL << 0)     /* Machine Mode Lockdown */
#define MSECCFG_MMWP    (1ULL << 1)     /* Machine Mode Allowlist Policy */
#define MSECCFG_RLB     (1ULL << 2)     /* Rule Locking Bypass */
#define MSECCFG_MLPE    (1ULL << 10)    /* M-mode Landing Pad Enable (Zicfilp) */
#define MSECCFG_PMM_OFF  32             /* PMM field offset in mseccfg */
#define MSECCFG_PMM_MASK (3ULL << 32)   /* PMM field mask [33:32] (Smmpm) */

/* ----- PMP Configuration CSRs ----- */
#define CSR_PMPCFG0     0x3A0
#define CSR_PMPCFG1     0x3A1   /* RV32 only */
#define CSR_PMPCFG2     0x3A2
#define CSR_PMPCFG3     0x3A3   /* RV32 only */
#define CSR_PMPCFG4     0x3A4
#define CSR_PMPCFG5     0x3A5   /* RV32 only */
#define CSR_PMPCFG6     0x3A6
#define CSR_PMPCFG7     0x3A7   /* RV32 only */
#define CSR_PMPCFG8     0x3A8
#define CSR_PMPCFG9     0x3A9   /* RV32 only */
#define CSR_PMPCFG10    0x3AA
#define CSR_PMPCFG11    0x3AB   /* RV32 only */
#define CSR_PMPCFG12    0x3AC
#define CSR_PMPCFG13    0x3AD   /* RV32 only */
#define CSR_PMPCFG14    0x3AE
#define CSR_PMPCFG15    0x3AF   /* RV32 only */

/* ----- PMP Address CSRs ----- */
#define CSR_PMPADDR0    0x3B0
#define CSR_PMPADDR1    0x3B1
#define CSR_PMPADDR2    0x3B2
#define CSR_PMPADDR3    0x3B3
#define CSR_PMPADDR4    0x3B4
#define CSR_PMPADDR5    0x3B5
#define CSR_PMPADDR6    0x3B6
#define CSR_PMPADDR7    0x3B7
#define CSR_PMPADDR8    0x3B8
#define CSR_PMPADDR9    0x3B9
#define CSR_PMPADDR10   0x3BA
#define CSR_PMPADDR11   0x3BB
#define CSR_PMPADDR12   0x3BC
#define CSR_PMPADDR13   0x3BD
#define CSR_PMPADDR14   0x3BE
#define CSR_PMPADDR15   0x3BF
#define CSR_PMPADDR16   0x3C0
#define CSR_PMPADDR17   0x3C1
#define CSR_PMPADDR18   0x3C2
#define CSR_PMPADDR19   0x3C3
#define CSR_PMPADDR20   0x3C4
#define CSR_PMPADDR21   0x3C5
#define CSR_PMPADDR22   0x3C6
#define CSR_PMPADDR23   0x3C7
#define CSR_PMPADDR24   0x3C8
#define CSR_PMPADDR25   0x3C9
#define CSR_PMPADDR26   0x3CA
#define CSR_PMPADDR27   0x3CB
#define CSR_PMPADDR28   0x3CC
#define CSR_PMPADDR29   0x3CD
#define CSR_PMPADDR30   0x3CE
#define CSR_PMPADDR31   0x3CF
#define CSR_PMPADDR32   0x3D0
#define CSR_PMPADDR33   0x3D1
#define CSR_PMPADDR34   0x3D2
#define CSR_PMPADDR35   0x3D3
#define CSR_PMPADDR36   0x3D4
#define CSR_PMPADDR37   0x3D5
#define CSR_PMPADDR38   0x3D6
#define CSR_PMPADDR39   0x3D7
#define CSR_PMPADDR40   0x3D8
#define CSR_PMPADDR41   0x3D9
#define CSR_PMPADDR42   0x3DA
#define CSR_PMPADDR43   0x3DB
#define CSR_PMPADDR44   0x3DC
#define CSR_PMPADDR45   0x3DD
#define CSR_PMPADDR46   0x3DE
#define CSR_PMPADDR47   0x3DF
#define CSR_PMPADDR48   0x3E0
#define CSR_PMPADDR49   0x3E1
#define CSR_PMPADDR50   0x3E2
#define CSR_PMPADDR51   0x3E3
#define CSR_PMPADDR52   0x3E4
#define CSR_PMPADDR53   0x3E5
#define CSR_PMPADDR54   0x3E6
#define CSR_PMPADDR55   0x3E7
#define CSR_PMPADDR56   0x3E8
#define CSR_PMPADDR57   0x3E9
#define CSR_PMPADDR58   0x3EA
#define CSR_PMPADDR59   0x3EB
#define CSR_PMPADDR60   0x3EC
#define CSR_PMPADDR61   0x3ED
#define CSR_PMPADDR62   0x3EE
#define CSR_PMPADDR63   0x3EF

/* ----- Unprivileged CSRs (Zicfiss) ----- */
#define CSR_SSP         0x011   /* Shadow Stack Pointer (Zicfiss) */

/* ----- Supervisor-level CSRs ----- */
#define CSR_SSTATUS     0x100
#define CSR_SIE         0x104
#define CSR_STVEC       0x105
#define CSR_SCOUNTEREN  0x106
#define CSR_SSCRATCH    0x140
#define CSR_SEPC        0x141
#define CSR_SCAUSE      0x142
#define CSR_STVAL       0x143
#define CSR_SIP         0x144
#define CSR_SATP        0x180

/* ----- Supervisor Environment Configuration ----- */
#define CSR_SENVCFG     0x10A
#define CSR_SENVCFGH    0x11A   /* RV32 only: upper 32 bits of senvcfg */

/* senvcfg field bits */
#define SENVCFG_LPE     (1ULL << 2)    /* Landing Pad Enable for U-mode (Zicfilp) */
#define SENVCFG_SSE     (1ULL << 3)    /* Shadow Stack Enable for U-mode (Zicfiss) */
#define SENVCFG_PMM_OFF  32             /* PMM field offset in senvcfg */
#define SENVCFG_PMM_MASK (3ULL << 32)   /* PMM field mask [33:32] (Ssnpm) */

/* ===================================================================
 * Pointer Masking (PM) common constants
 * =================================================================== */
#define PMM_DISABLED    0   /* Pointer masking disabled (PMLEN=0) */
#define PMM_RESERVED    1   /* Reserved */
#define PMM_PMLEN7      2   /* PMLEN = XLEN-57 = 7 on RV64 */
#define PMM_PMLEN16     3   /* PMLEN = XLEN-48 = 16 on RV64 */

/* ===================================================================
 * mstatus field offsets and masks
 * =================================================================== */
#define MSTATUS_SIE_BIT     BIT(1)
#define MSTATUS_MIE_BIT     BIT(3)
#define MSTATUS_SPIE_BIT    BIT(5)
#define MSTATUS_MPIE_BIT    BIT(7)
#define MSTATUS_SPP_OFF     8
#define MSTATUS_SPP_BIT     BIT(8)
#define MSTATUS_MPP_OFF     11
#define MSTATUS_MPP_MASK    BIT_MASK(11, 2)
#define MSTATUS_MPRV_BIT    BIT(17)
#define MSTATUS_SUM_BIT     BIT(18)
#define MSTATUS_MXR_BIT     BIT(19)
#define MSTATUS_TVM_BIT     BIT(20)
#define MSTATUS_TW_BIT      BIT(21)
#define MSTATUS_TSR_BIT     BIT(22)
#define MSTATUS_SPELP_BIT   BIT(33)    /* S-mode Previous ELP (Zicfilp) */
#define MSTATUS_MPELP_BIT   BIT(41)    /* M-mode Previous ELP (Zicfilp) */

/* ===================================================================
 * Exception cause codes (synchronous)
 * =================================================================== */
#define CAUSE_INST_ADDR_MISALIGN    0
#define CAUSE_INST_ACCESS_FAULT     1   /* Instruction access fault (PMP/PMA) */
#define CAUSE_ILLEGAL_INST          2
#define CAUSE_BREAKPOINT            3
#define CAUSE_LOAD_ADDR_MISALIGN    4
#define CAUSE_LOAD_ACCESS_FAULT     5   /* Load access fault (PMP/PMA) */
#define CAUSE_STORE_ADDR_MISALIGN   6
#define CAUSE_STORE_ACCESS_FAULT    7   /* Store/AMO access fault (PMP/PMA) */
#define CAUSE_ECALL_FROM_U          8
#define CAUSE_ECALL_FROM_S          9
#define CAUSE_ECALL_FROM_M          11
#define CAUSE_INST_PAGE_FAULT       12
#define CAUSE_LOAD_PAGE_FAULT       13
#define CAUSE_STORE_PAGE_FAULT      15
#define CAUSE_SOFTWARE_CHECK        18  /* Software-check exception (CFI) */
#define CAUSE_HARDWARE_ERROR        19  /* Hardware-error exception */

/* mtval encodings for software-check exception (cause=18) */
#define SWCHECK_NONE                0
#define SWCHECK_LANDING_PAD_FAULT   2   /* Zicfilp: Landing Pad Fault */
#define SWCHECK_SHADOW_STACK_FAULT  3   /* Zicfiss: Shadow Stack Fault */

/* Short aliases for PMP-specific causes */
#define CAUSE_IAF   CAUSE_INST_ACCESS_FAULT
#define CAUSE_LAF   CAUSE_LOAD_ACCESS_FAULT
#define CAUSE_SAF   CAUSE_STORE_ACCESS_FAULT
#define CAUSE_ECU   CAUSE_ECALL_FROM_U
#define CAUSE_ECS   CAUSE_ECALL_FROM_S
#define CAUSE_ECM   CAUSE_ECALL_FROM_M
#define CAUSE_ILI   CAUSE_ILLEGAL_INST

/* Short aliases for page fault causes */
#define CAUSE_IPF   CAUSE_INST_PAGE_FAULT
#define CAUSE_LPF   CAUSE_LOAD_PAGE_FAULT
#define CAUSE_SPF   CAUSE_STORE_PAGE_FAULT

/* ===================================================================
 * Inline CSR access macros (compile-time CSR name)
 * =================================================================== */
#ifndef __ASSEMBLER__

#define _CSR_STR(s) #s
#define CSR_STR(s)  _CSR_STR(s)

#if __riscv_xlen == 64
#define CSRR(csr) ({ \
    uint64_t _v; \
    asm volatile("csrr %0, " CSR_STR(csr) : "=r"(_v) :: "memory"); \
    _v; \
})
#else
#define CSRR(csr) ({ \
    uint32_t _v; \
    asm volatile("csrr %0, " CSR_STR(csr) : "=r"(_v) :: "memory"); \
    _v; \
})
#endif

#define CSRW(csr, val) \
    asm volatile("csrw " CSR_STR(csr) ", %0" :: "rK"(val) : "memory")

#define CSRS(csr, val) \
    asm volatile("csrs " CSR_STR(csr) ", %0" :: "rK"(val) : "memory")

#define CSRC(csr, val) \
    asm volatile("csrc " CSR_STR(csr) ", %0" :: "rK"(val) : "memory")

/* SFENCE.VMA */
static inline void sfence_vma(void) {
    asm volatile("sfence.vma" ::: "memory");
}

#endif /* __ASSEMBLER__ */

/* ===================================================================
 * Hypervisor extension CSRs (H ext, RV64)
 *
 * Defined unconditionally (address constants only). Use ENABLE_HYP
 * in build to compile the supporting accessors / handlers.
 * =================================================================== */

/* HS-level Hypervisor CSRs */
#define CSR_HSTATUS       0x600
#define CSR_HEDELEG       0x602
#define CSR_HIDELEG       0x603
#define CSR_HIE           0x604
#define CSR_HTIMEDELTA    0x605
#define CSR_HCOUNTEREN    0x606
#define CSR_HGEIE         0x607
#define CSR_HENVCFG       0x60A
#define CSR_HENVCFGH      0x61A   /* RV32 only */
#define CSR_HTVAL         0x643
#define CSR_HIP           0x644
#define CSR_HVIP          0x645
#define CSR_HTINST        0x64A
#define CSR_HGEIP         0xE12
#define CSR_HGATP         0x680

/* VS-level CSRs */
#define CSR_VSSTATUS      0x200
#define CSR_VSIE           0x204
#define CSR_VSTVEC         0x205
#define CSR_VSSCRATCH      0x240
#define CSR_VSEPC          0x241
#define CSR_VSCAUSE        0x242
#define CSR_VSTVAL         0x243
#define CSR_VSIP           0x244
#define CSR_VSATP          0x280

/* M-mode hypervisor-extension CSRs */
#define CSR_MTVAL2         0x34B
#define CSR_MTINST         0x34A

/* Smstateen / Ssstateen CSRs */
#define CSR_MSTATEEN0      0x30C
#define CSR_MSTATEEN1      0x30D
#define CSR_MSTATEEN2      0x30E
#define CSR_MSTATEEN3      0x30F
#define CSR_HSTATEEN0      0x60C
#define CSR_HSTATEEN1      0x60D
#define CSR_HSTATEEN2      0x60E
#define CSR_HSTATEEN3      0x60F
#define CSR_SSTATEEN0      0x10C
#define CSR_SSTATEEN1      0x10D
#define CSR_SSTATEEN2      0x10E
#define CSR_SSTATEEN3      0x10F

/* stateen bit definitions */
#define STATEEN_BIT63      (1ULL << 63)  /* controls access to remaining stateeN CSRs */
#define STATEEN_BIT58      (1ULL << 58)  /* Sscofpmf/LCOFI state */

/* mstateen0 functional bit definitions */
#define MSTATEEN0_C         (1ULL << 0)   /* Custom state */
#define MSTATEEN0_FCSR      (1ULL << 1)   /* Zfinx fcsr */
#define MSTATEEN0_JVT       (1ULL << 2)   /* Zcmt jvt */
#define MSTATEEN0_SRMCFG    (1ULL << 55)  /* srmcfg (Ssqosid) */
#define MSTATEEN0_P1P13     (1ULL << 56)  /* hedelegh */
#define MSTATEEN0_CONTEXT   (1ULL << 57)  /* scontext/hcontext (Sdtrig) */
#define MSTATEEN0_IMSIC     (1ULL << 58)  /* IMSIC state (Ssaia) */
#define MSTATEEN0_AIA       (1ULL << 59)  /* AIA remaining state */
#define MSTATEEN0_CSRIND    (1ULL << 60)  /* siselect/sireg* (Sscsrind) */
#define MSTATEEN0_ENVCFG    (1ULL << 62)  /* senvcfg/henvcfg */
#define MSTATEEN0_SE0       (1ULL << 63)  /* sstateen0/hstateen0 access */

/* WPRI mask: bits [53:3] in mstateen0 are reserved */
#define MSTATEEN0_WPRI_MASK 0x003FFFFFFFFFFFF8ULL

/* stateen0 shared bit definitions (used by sstateen0 / hstateen0)
 * Encoding is identical across mstateen0 / hstateen0 / sstateen0. */
#define STATEEN0_C         (1UL << 0)   /* Custom state */
#define STATEEN0_FCSR      (1UL << 1)   /* Zfinx fcsr */
#define STATEEN0_JVT       (1UL << 2)   /* Zcmt jvt */
#define STATEEN0_CONTEXT   (1UL << 57)  /* scontext (Sdtrig) */
#define STATEEN0_IMSIC     (1UL << 58)  /* Guest IMSIC (Ssaia) */
#define STATEEN0_AIA       (1UL << 59)  /* AIA remaining state */
#define STATEEN0_CSRIND    (1UL << 60)  /* siselect/sireg* (Sscsrind) */
#define STATEEN0_ENVCFG    (1UL << 62)  /* senvcfg */
#define STATEEN0_SE0       STATEEN_BIT63 /* sstateen0/hstateen0 access */

/* vstvec MODE encoding */
#define VSTVEC_MODE_DIRECT   0
#define VSTVEC_MODE_VECTORED 1
#define VSTVEC_MODE_MASK     0x3UL
#define VSTVEC_BASE_MASK     (~0x3UL)

/* Hypervisor cause codes (mcause/scause low bits) */
#define CAUSE_ECALL_FROM_VS            10
#define CAUSE_INST_GUEST_PAGE_FAULT    20
#define CAUSE_LOAD_GUEST_PAGE_FAULT    21
#define CAUSE_VIRTUAL_INSTRUCTION      22
#define CAUSE_STORE_GUEST_PAGE_FAULT   23

/* hstatus fields (RV64) */
#define HSTATUS_VSBE       (1UL << 5)
#define HSTATUS_GVA        (1UL << 6)
#define HSTATUS_SPV        (1UL << 7)
#define HSTATUS_SPVP       (1UL << 8)
#define HSTATUS_HU         (1UL << 9)
#define HSTATUS_VGEIN_SHIFT 12
#define HSTATUS_VGEIN_MASK (0x3FUL << HSTATUS_VGEIN_SHIFT)
#define HSTATUS_VTVM       (1UL << 20)
#define HSTATUS_VTW        (1UL << 21)
#define HSTATUS_VTSR       (1UL << 22)
#define HSTATUS_VSXL_SHIFT 32
#define HSTATUS_VSXL_MASK  (3UL << HSTATUS_VSXL_SHIFT)

/* hstatus writable fields mask (RV64):
 *   VSBE(5), GVA(6), SPV(7), SPVP(8), HU(9),
 *   VGEIN(17:12), VTVM(20), VTW(21), VTSR(22), VSXL(33:32) */
#define HSTATUS_WRITABLE_MASK ( \
    HSTATUS_VSBE | HSTATUS_GVA | HSTATUS_SPV | HSTATUS_SPVP | \
    HSTATUS_HU | HSTATUS_VGEIN_MASK | \
    HSTATUS_VTVM | HSTATUS_VTW | HSTATUS_VTSR | HSTATUS_VSXL_MASK )

/* vsstatus writable fields mask (RV64):
 *   SIE(1), SPIE(5), UBE(6), SPP(8), VS(10:9), FS(14:13),
 *   XS(16:15), SUM(18), MXR(19).
 * Note: SD(63) is read-only (computed from FS/XS/VS).
 *       UXL(33:32) is read-only in vsstatus from HS-mode. */
#define VSSTATUS_WRITABLE_MASK ( \
    (1UL << 1)  /* SIE   */ | \
    (1UL << 5)  /* SPIE  */ | \
    (1UL << 6)  /* UBE   */ | \
    (1UL << 8)  /* SPP   */ | \
    (3UL << 9)  /* VS    */ | \
    (3UL << 13) /* FS    */ | \
    (3UL << 15) /* XS    */ | \
    (1UL << 18) /* SUM   */ | \
    (1UL << 19) /* MXR   */ )

/* vsie writable bits (S-level interrupt enables visible to VS):
 *   SSIE(1), STIE(5), SEIE(9) */
#define VSIE_WRITABLE_MASK ( \
    (1UL << 1) /* SSIE */ | \
    (1UL << 5) /* STIE */ | \
    (1UL << 9) /* SEIE */ )

/* mstatus extension fields for hypervisor (RV64) */
#define MSTATUS_GVA        (1UL << 38)
#define MSTATUS_MPV        (1UL << 39)

/* mstatus.TVM (already in base privileged spec, but used by GHCSR-08) */
#ifndef MSTATUS_TVM
#define MSTATUS_TVM        (1UL << 20)
#endif

/* satp / vsatp extended layout constants (RV64 / SXLEN=64)
 *
 * Basic satp constants (SATP_MODE_*, MAKE_SATP) are defined in
 * vm/vm_defs.h. Here we add accessor macros and the ASID mask
 * needed by hyp_csr.c for mode/ASID probing. */
#ifndef SATP64_MODE_SHIFT
#define SATP64_MODE_SHIFT  60
#endif
#ifndef SATP64_ASID_SHIFT
#define SATP64_ASID_SHIFT  44
#endif
#ifndef SATP64_PPN_MASK
#define SATP64_PPN_MASK    ((1UL << 44) - 1)
#endif
#define SATP64_ASID_MASK   ((1UL << 16) - 1)

/* Mode constants (guarded: vm_defs.h may define these first) */
#ifndef SATP_MODE_BARE
#define SATP_MODE_BARE     0
#define SATP_MODE_SV39     8
#define SATP_MODE_SV48     9
#define SATP_MODE_SV57     10
#endif

/* MAKE_SATP: use vm_defs.h version if available, otherwise provide one */
#ifndef MAKE_SATP
#define MAKE_SATP(mode, asid, ppn) \
    (((uintptr_t)(mode) << SATP64_MODE_SHIFT) | \
     (((uintptr_t)(asid) & SATP64_ASID_MASK) << SATP64_ASID_SHIFT) | \
     ((uintptr_t)(ppn) & SATP64_PPN_MASK))
#endif

/* Extraction macros (always defined here; not in vm_defs.h) */
#define SATP_GET_MODE(v)   (((uintptr_t)(v) >> SATP64_MODE_SHIFT) & 0xFUL)
#define SATP_GET_ASID(v)   (((uintptr_t)(v) >> SATP64_ASID_SHIFT) & SATP64_ASID_MASK)
#define SATP_GET_PPN(v)    ((uintptr_t)(v) & SATP64_PPN_MASK)

/* hgatp layout (RV64 / HSXLEN=64) */
#define HGATP64_MODE_SHIFT 60
#define HGATP64_VMID_SHIFT 44
#define HGATP64_PPN_MASK   ((1UL << 44) - 1)
#define HGATP64_VMID_MASK  ((1UL << 14) - 1)

#define HGATP_MODE_BARE    0
#define HGATP_MODE_SV39X4  8
#define HGATP_MODE_SV48X4  9
#define HGATP_MODE_SV57X4  10

/* Map satp mode to corresponding hgatp mode */
#define HGATP_MODE_FOR_SATP(satp_mode) (satp_mode)  /* SvNN=N, SvNNx4=N */

/* ===================================================================
 * Base Counter CSRs
 * =================================================================== */

/* ----- Base Counter CSRs (S/U-mode read-only) ----- */
#define CSR_CYCLE       0xC00
#define CSR_TIME        0xC01
#define CSR_INSTRET     0xC02

/* ----- Base Counter CSRs (M-mode read/write) ----- */
#define CSR_MCYCLE      0xB00
#define CSR_MINSTRET    0xB02

/* ===================================================================
 * Sscofpmf Extension: HPM Event / Counter CSRs and field masks
 * =================================================================== */

/* ----- HPM Event CSRs (mhpmevent3–31) ----- */
#define CSR_MHPMEVENT3   0x323
#define CSR_MHPMEVENT4   0x324
#define CSR_MHPMEVENT5   0x325
#define CSR_MHPMEVENT6   0x326
#define CSR_MHPMEVENT7   0x327
#define CSR_MHPMEVENT8   0x328
#define CSR_MHPMEVENT9   0x329
#define CSR_MHPMEVENT10  0x32A
#define CSR_MHPMEVENT11  0x32B
#define CSR_MHPMEVENT12  0x32C
#define CSR_MHPMEVENT13  0x32D
#define CSR_MHPMEVENT14  0x32E
#define CSR_MHPMEVENT15  0x32F
#define CSR_MHPMEVENT16  0x330
#define CSR_MHPMEVENT17  0x331
#define CSR_MHPMEVENT18  0x332
#define CSR_MHPMEVENT19  0x333
#define CSR_MHPMEVENT20  0x334
#define CSR_MHPMEVENT21  0x335
#define CSR_MHPMEVENT22  0x336
#define CSR_MHPMEVENT23  0x337
#define CSR_MHPMEVENT24  0x338
#define CSR_MHPMEVENT25  0x339
#define CSR_MHPMEVENT26  0x33A
#define CSR_MHPMEVENT27  0x33B
#define CSR_MHPMEVENT28  0x33C
#define CSR_MHPMEVENT29  0x33D
#define CSR_MHPMEVENT30  0x33E
#define CSR_MHPMEVENT31  0x33F

/* ----- HPM Counter CSRs (M-mode read/write) ----- */
#define CSR_MHPMCOUNTER3   0xB03
#define CSR_MHPMCOUNTER4   0xB04
#define CSR_MHPMCOUNTER5   0xB05
#define CSR_MHPMCOUNTER6   0xB06
#define CSR_MHPMCOUNTER7   0xB07
#define CSR_MHPMCOUNTER8   0xB08
#define CSR_MHPMCOUNTER9   0xB09
#define CSR_MHPMCOUNTER10  0xB0A
#define CSR_MHPMCOUNTER11  0xB0B
#define CSR_MHPMCOUNTER12  0xB0C
#define CSR_MHPMCOUNTER13  0xB0D
#define CSR_MHPMCOUNTER14  0xB0E
#define CSR_MHPMCOUNTER15  0xB0F
#define CSR_MHPMCOUNTER16  0xB10
#define CSR_MHPMCOUNTER17  0xB11
#define CSR_MHPMCOUNTER18  0xB12
#define CSR_MHPMCOUNTER19  0xB13
#define CSR_MHPMCOUNTER20  0xB14
#define CSR_MHPMCOUNTER21  0xB15
#define CSR_MHPMCOUNTER22  0xB16
#define CSR_MHPMCOUNTER23  0xB17
#define CSR_MHPMCOUNTER24  0xB18
#define CSR_MHPMCOUNTER25  0xB19
#define CSR_MHPMCOUNTER26  0xB1A
#define CSR_MHPMCOUNTER27  0xB1B
#define CSR_MHPMCOUNTER28  0xB1C
#define CSR_MHPMCOUNTER29  0xB1D
#define CSR_MHPMCOUNTER30  0xB1E
#define CSR_MHPMCOUNTER31  0xB1F

/* ----- HPM Counter CSRs (S/U-mode read-only shadow) ----- */
#define CSR_HPMCOUNTER3   0xC03
#define CSR_HPMCOUNTER4   0xC04
#define CSR_HPMCOUNTER5   0xC05
#define CSR_HPMCOUNTER6   0xC06
#define CSR_HPMCOUNTER7   0xC07
#define CSR_HPMCOUNTER8   0xC08
#define CSR_HPMCOUNTER9   0xC09
#define CSR_HPMCOUNTER10  0xC0A
#define CSR_HPMCOUNTER11  0xC0B
#define CSR_HPMCOUNTER12  0xC0C
#define CSR_HPMCOUNTER13  0xC0D
#define CSR_HPMCOUNTER14  0xC0E
#define CSR_HPMCOUNTER15  0xC0F
#define CSR_HPMCOUNTER16  0xC10
#define CSR_HPMCOUNTER17  0xC11
#define CSR_HPMCOUNTER18  0xC12
#define CSR_HPMCOUNTER19  0xC13
#define CSR_HPMCOUNTER20  0xC14
#define CSR_HPMCOUNTER21  0xC15
#define CSR_HPMCOUNTER22  0xC16
#define CSR_HPMCOUNTER23  0xC17
#define CSR_HPMCOUNTER24  0xC18
#define CSR_HPMCOUNTER25  0xC19
#define CSR_HPMCOUNTER26  0xC1A
#define CSR_HPMCOUNTER27  0xC1B
#define CSR_HPMCOUNTER28  0xC1C
#define CSR_HPMCOUNTER29  0xC1D
#define CSR_HPMCOUNTER30  0xC1E
#define CSR_HPMCOUNTER31  0xC1F

/* ----- Machine Counter Inhibit ----- */
#define CSR_MCOUNTINHIBIT 0x320

/* ----- Supervisor Count Overflow ----- */
#define CSR_SCOUNTOVF     0xDA0

/* ----- mhpmevent high-bit field masks (always 64-bit logical view) ----- */
#define MHPMEVENT_OF      (1ULL << 63)
#define MHPMEVENT_MINH    (1ULL << 62)
#define MHPMEVENT_SINH    (1ULL << 61)
#define MHPMEVENT_UINH    (1ULL << 60)
#define MHPMEVENT_VSINH   (1ULL << 59)
#define MHPMEVENT_VUINH   (1ULL << 58)

#if __riscv_xlen == 32
/* RV32 high-half CSR addresses:
 * mhpmeventh3:   0x723 (mhpmevent3 + 0x400)
 * mhpmcounterh3: 0xB83 (mhpmcounter3 + 0x80)
 */
#define CSR_MHPMEVENTH3   0x723
#define CSR_MHPMCOUNTERH3 0xB83
#endif

/* ----- LCOFI interrupt bit (bit 13) ----- */
#define MIP_LCOFIP        (1ULL << 13)
#define MIE_LCOFIE        (1ULL << 13)
#define IRQ_LCOFI         13

/* ----- Interrupt bit in mcause ----- */
#ifndef CAUSE_INTERRUPT_BIT
#if __riscv_xlen == 64
#define CAUSE_INTERRUPT_BIT  (1ULL << 63)
#else
#define CAUSE_INTERRUPT_BIT  (1ULL << 31)
#endif
#endif

/* ===================================================================
 * Sstc Extension: Supervisor-mode Timer Interrupts CSRs and fields
 * =================================================================== */

/* ----- Sstc extension CSRs ----- */
#define CSR_STIMECMP    0x14D
#define CSR_STIMECMPH   0x15D   /* RV32 only */
#define CSR_VSTIMECMP   0x24D
#define CSR_VSTIMECMPH  0x25D   /* RV32 only */

/* menvcfg / henvcfg STCE bit (bit 63) */
#define MENVCFG_STCE    (1ULL << 63)
#define HENVCFG_STCE    (1ULL << 63)

/* mip/sip bit 5 = STIP, mie/sie bit 5 = STIE */
#define MIP_STIP        (1ULL << 5)
#define MIE_STIE        (1ULL << 5)
#define SIP_STIP        (1ULL << 5)
#define SIE_STIE        (1ULL << 5)

/* hip/hvip bit 6 = VSTIP */
#define HIP_VSTIP       (1ULL << 6)
#define HVIP_VSTIP      (1ULL << 6)

/* mcounteren / hcounteren TM bit (bit 1) */
#define MCOUNTEREN_TM   (1ULL << 1)
#define HCOUNTEREN_TM   (1ULL << 1)

/* mideleg STIP bit */
#define MIDELEG_STIP    (1ULL << 5)

/* Interrupt cause: S-mode timer */
#define CAUSE_S_TIMER_INTERRUPT  5
#define SCAUSE_INTERRUPT         CAUSE_INTERRUPT_BIT

#endif /* PMP_ENCODING_H */
