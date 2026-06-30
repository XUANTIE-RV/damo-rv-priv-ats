/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "types.h"
#include "encoding.h"

/* ===================================================================
 * Dynamic CSR read/write
 *
 * RISC-V CSR instructions require compile-time immediate operands.
 * These functions use switch-case to map runtime CSR addresses to
 * compile-time CSR access instructions.
 *
 * Only PMP-related CSR ranges are expanded to avoid code bloat:
 *   0x3A0-0x3AF  pmpcfg0-pmpcfg15
 *   0x3B0-0x3EF  pmpaddr0-pmpaddr63
 *   0x747        mseccfg
 *   0x757        mseccfgh (RV32 only)
 * =================================================================== */

/* Helper macros for generating switch cases */
#define _CSR_READ_CASE(addr) \
    case (addr): asm volatile("csrr %0, " _CSR_STR(addr) : "=r"(val) :: "memory"); break;

#define _CSR_WRITE_CASE(addr) \
    case (addr): asm volatile("csrw " _CSR_STR(addr) ", %0" :: "r"(val) : "memory"); break;

/* ===================================================================
 * csr_read - read a CSR by runtime address
 * =================================================================== */
uintptr_t csr_read(uint16_t csr) {
    uintptr_t val = 0;

    switch (csr) {
    /* ----- pmpcfg registers (0x3A0 - 0x3AF) ----- */
    _CSR_READ_CASE(0x3A0)
    _CSR_READ_CASE(0x3A1)
    _CSR_READ_CASE(0x3A2)
    _CSR_READ_CASE(0x3A3)
    _CSR_READ_CASE(0x3A4)
    _CSR_READ_CASE(0x3A5)
    _CSR_READ_CASE(0x3A6)
    _CSR_READ_CASE(0x3A7)
    _CSR_READ_CASE(0x3A8)
    _CSR_READ_CASE(0x3A9)
    _CSR_READ_CASE(0x3AA)
    _CSR_READ_CASE(0x3AB)
    _CSR_READ_CASE(0x3AC)
    _CSR_READ_CASE(0x3AD)
    _CSR_READ_CASE(0x3AE)
    _CSR_READ_CASE(0x3AF)

    /* ----- pmpaddr registers (0x3B0 - 0x3EF) ----- */
    _CSR_READ_CASE(0x3B0)
    _CSR_READ_CASE(0x3B1)
    _CSR_READ_CASE(0x3B2)
    _CSR_READ_CASE(0x3B3)
    _CSR_READ_CASE(0x3B4)
    _CSR_READ_CASE(0x3B5)
    _CSR_READ_CASE(0x3B6)
    _CSR_READ_CASE(0x3B7)
    _CSR_READ_CASE(0x3B8)
    _CSR_READ_CASE(0x3B9)
    _CSR_READ_CASE(0x3BA)
    _CSR_READ_CASE(0x3BB)
    _CSR_READ_CASE(0x3BC)
    _CSR_READ_CASE(0x3BD)
    _CSR_READ_CASE(0x3BE)
    _CSR_READ_CASE(0x3BF)
    _CSR_READ_CASE(0x3C0)
    _CSR_READ_CASE(0x3C1)
    _CSR_READ_CASE(0x3C2)
    _CSR_READ_CASE(0x3C3)
    _CSR_READ_CASE(0x3C4)
    _CSR_READ_CASE(0x3C5)
    _CSR_READ_CASE(0x3C6)
    _CSR_READ_CASE(0x3C7)
    _CSR_READ_CASE(0x3C8)
    _CSR_READ_CASE(0x3C9)
    _CSR_READ_CASE(0x3CA)
    _CSR_READ_CASE(0x3CB)
    _CSR_READ_CASE(0x3CC)
    _CSR_READ_CASE(0x3CD)
    _CSR_READ_CASE(0x3CE)
    _CSR_READ_CASE(0x3CF)
    _CSR_READ_CASE(0x3D0)
    _CSR_READ_CASE(0x3D1)
    _CSR_READ_CASE(0x3D2)
    _CSR_READ_CASE(0x3D3)
    _CSR_READ_CASE(0x3D4)
    _CSR_READ_CASE(0x3D5)
    _CSR_READ_CASE(0x3D6)
    _CSR_READ_CASE(0x3D7)
    _CSR_READ_CASE(0x3D8)
    _CSR_READ_CASE(0x3D9)
    _CSR_READ_CASE(0x3DA)
    _CSR_READ_CASE(0x3DB)
    _CSR_READ_CASE(0x3DC)
    _CSR_READ_CASE(0x3DD)
    _CSR_READ_CASE(0x3DE)
    _CSR_READ_CASE(0x3DF)
    _CSR_READ_CASE(0x3E0)
    _CSR_READ_CASE(0x3E1)
    _CSR_READ_CASE(0x3E2)
    _CSR_READ_CASE(0x3E3)
    _CSR_READ_CASE(0x3E4)
    _CSR_READ_CASE(0x3E5)
    _CSR_READ_CASE(0x3E6)
    _CSR_READ_CASE(0x3E7)
    _CSR_READ_CASE(0x3E8)
    _CSR_READ_CASE(0x3E9)
    _CSR_READ_CASE(0x3EA)
    _CSR_READ_CASE(0x3EB)
    _CSR_READ_CASE(0x3EC)
    _CSR_READ_CASE(0x3ED)
    _CSR_READ_CASE(0x3EE)
    _CSR_READ_CASE(0x3EF)

    /* ----- mseccfg (0x747) ----- */
    _CSR_READ_CASE(0x747)
#if __riscv_xlen == 32
    _CSR_READ_CASE(0x757)  /* mseccfgh */
#endif

    /* ----- Common CSRs (for privilege switching etc.) ----- */
    _CSR_READ_CASE(0x300)  /* mstatus */
    _CSR_READ_CASE(0x301)  /* misa */
    _CSR_READ_CASE(0x302)  /* medeleg */
    _CSR_READ_CASE(0x303)  /* mideleg */
    _CSR_READ_CASE(0x304)  /* mie */
    _CSR_READ_CASE(0x341)  /* mepc */
    _CSR_READ_CASE(0x342)  /* mcause */
    _CSR_READ_CASE(0x343)  /* mtval */
    _CSR_READ_CASE(0x344)  /* mip */
    _CSR_READ_CASE(0x100)  /* sstatus */
    _CSR_READ_CASE(0x104)  /* sie */
    _CSR_READ_CASE(0x141)  /* sepc */
    _CSR_READ_CASE(0x142)  /* scause */
    _CSR_READ_CASE(0x143)  /* stval */
    _CSR_READ_CASE(0x144)  /* sip */
    _CSR_READ_CASE(0x10A)  /* senvcfg */
    _CSR_READ_CASE(0x30A)  /* menvcfg */

    /* ----- Sstc extension CSRs ----- */
    _CSR_READ_CASE(0x14D)  /* stimecmp  */
    _CSR_READ_CASE(0x24D)  /* vstimecmp */

    /* ----- scounteren (0x106) ----- */
    _CSR_READ_CASE(0x106)  /* scounteren */

    /* ----- mcounteren (0x306) ----- */
    _CSR_READ_CASE(0x306)  /* mcounteren */

    /* ----- mcountinhibit (0x320) ----- */
    _CSR_READ_CASE(0x320)  /* mcountinhibit */

    /* ----- mhpmevent registers (0x323 - 0x33F) ----- */
    _CSR_READ_CASE(0x323)  _CSR_READ_CASE(0x324)  _CSR_READ_CASE(0x325)
    _CSR_READ_CASE(0x326)  _CSR_READ_CASE(0x327)  _CSR_READ_CASE(0x328)
    _CSR_READ_CASE(0x329)  _CSR_READ_CASE(0x32A)  _CSR_READ_CASE(0x32B)
    _CSR_READ_CASE(0x32C)  _CSR_READ_CASE(0x32D)  _CSR_READ_CASE(0x32E)
    _CSR_READ_CASE(0x32F)  _CSR_READ_CASE(0x330)  _CSR_READ_CASE(0x331)
    _CSR_READ_CASE(0x332)  _CSR_READ_CASE(0x333)  _CSR_READ_CASE(0x334)
    _CSR_READ_CASE(0x335)  _CSR_READ_CASE(0x336)  _CSR_READ_CASE(0x337)
    _CSR_READ_CASE(0x338)  _CSR_READ_CASE(0x339)  _CSR_READ_CASE(0x33A)
    _CSR_READ_CASE(0x33B)  _CSR_READ_CASE(0x33C)  _CSR_READ_CASE(0x33D)
    _CSR_READ_CASE(0x33E)  _CSR_READ_CASE(0x33F)

    /* ----- mcycle / minstret (0xB00, 0xB02) ----- */
    _CSR_READ_CASE(0xB00)  /* mcycle */
    _CSR_READ_CASE(0xB02)  /* minstret */

    /* ----- mhpmcounter registers (0xB03 - 0xB1F) ----- */
    _CSR_READ_CASE(0xB03)  _CSR_READ_CASE(0xB04)  _CSR_READ_CASE(0xB05)
    _CSR_READ_CASE(0xB06)  _CSR_READ_CASE(0xB07)  _CSR_READ_CASE(0xB08)
    _CSR_READ_CASE(0xB09)  _CSR_READ_CASE(0xB0A)  _CSR_READ_CASE(0xB0B)
    _CSR_READ_CASE(0xB0C)  _CSR_READ_CASE(0xB0D)  _CSR_READ_CASE(0xB0E)
    _CSR_READ_CASE(0xB0F)  _CSR_READ_CASE(0xB10)  _CSR_READ_CASE(0xB11)
    _CSR_READ_CASE(0xB12)  _CSR_READ_CASE(0xB13)  _CSR_READ_CASE(0xB14)
    _CSR_READ_CASE(0xB15)  _CSR_READ_CASE(0xB16)  _CSR_READ_CASE(0xB17)
    _CSR_READ_CASE(0xB18)  _CSR_READ_CASE(0xB19)  _CSR_READ_CASE(0xB1A)
    _CSR_READ_CASE(0xB1B)  _CSR_READ_CASE(0xB1C)  _CSR_READ_CASE(0xB1D)
    _CSR_READ_CASE(0xB1E)  _CSR_READ_CASE(0xB1F)

#if __riscv_xlen == 32
    /* ----- mcycleh / minstreth (RV32 high-half) ----- */
    _CSR_READ_CASE(0xB80)  /* mcycleh */
    _CSR_READ_CASE(0xB82)  /* minstreth */

    /* ----- mhpmcounterh registers (0xB83 - 0xB9F, RV32 high-half) ----- */
    _CSR_READ_CASE(0xB83)  _CSR_READ_CASE(0xB84)  _CSR_READ_CASE(0xB85)
    _CSR_READ_CASE(0xB86)  _CSR_READ_CASE(0xB87)  _CSR_READ_CASE(0xB88)
    _CSR_READ_CASE(0xB89)  _CSR_READ_CASE(0xB8A)  _CSR_READ_CASE(0xB8B)
    _CSR_READ_CASE(0xB8C)  _CSR_READ_CASE(0xB8D)  _CSR_READ_CASE(0xB8E)
    _CSR_READ_CASE(0xB8F)  _CSR_READ_CASE(0xB90)  _CSR_READ_CASE(0xB91)
    _CSR_READ_CASE(0xB92)  _CSR_READ_CASE(0xB93)  _CSR_READ_CASE(0xB94)
    _CSR_READ_CASE(0xB95)  _CSR_READ_CASE(0xB96)  _CSR_READ_CASE(0xB97)
    _CSR_READ_CASE(0xB98)  _CSR_READ_CASE(0xB99)  _CSR_READ_CASE(0xB9A)
    _CSR_READ_CASE(0xB9B)  _CSR_READ_CASE(0xB9C)  _CSR_READ_CASE(0xB9D)
    _CSR_READ_CASE(0xB9E)  _CSR_READ_CASE(0xB9F)
#endif

    /* ----- cycle / time / instret (0xC00 - 0xC02, read-only) ----- */
    _CSR_READ_CASE(0xC00)  /* cycle */
    _CSR_READ_CASE(0xC01)  /* time */
    _CSR_READ_CASE(0xC02)  /* instret */

    /* ----- hpmcounter registers (0xC03 - 0xC1F, read-only shadow) ----- */
    _CSR_READ_CASE(0xC03)  _CSR_READ_CASE(0xC04)  _CSR_READ_CASE(0xC05)
    _CSR_READ_CASE(0xC06)  _CSR_READ_CASE(0xC07)  _CSR_READ_CASE(0xC08)
    _CSR_READ_CASE(0xC09)  _CSR_READ_CASE(0xC0A)  _CSR_READ_CASE(0xC0B)
    _CSR_READ_CASE(0xC0C)  _CSR_READ_CASE(0xC0D)  _CSR_READ_CASE(0xC0E)
    _CSR_READ_CASE(0xC0F)  _CSR_READ_CASE(0xC10)  _CSR_READ_CASE(0xC11)
    _CSR_READ_CASE(0xC12)  _CSR_READ_CASE(0xC13)  _CSR_READ_CASE(0xC14)
    _CSR_READ_CASE(0xC15)  _CSR_READ_CASE(0xC16)  _CSR_READ_CASE(0xC17)
    _CSR_READ_CASE(0xC18)  _CSR_READ_CASE(0xC19)  _CSR_READ_CASE(0xC1A)
    _CSR_READ_CASE(0xC1B)  _CSR_READ_CASE(0xC1C)  _CSR_READ_CASE(0xC1D)
    _CSR_READ_CASE(0xC1E)  _CSR_READ_CASE(0xC1F)

    /* ----- scountovf (0xDA0, read-only) ----- */
    _CSR_READ_CASE(0xDA0)

#ifdef ENABLE_HYP
    /* ----- Hypervisor CSRs (HS-level) ----- */
    _CSR_READ_CASE(0x600)  /* hstatus     */
    _CSR_READ_CASE(0x602)  /* hedeleg     */
    _CSR_READ_CASE(0x603)  /* hideleg     */
    _CSR_READ_CASE(0x604)  /* hie         */
    _CSR_READ_CASE(0x605)  /* htimedelta  */
    _CSR_READ_CASE(0x606)  /* hcounteren  */
    _CSR_READ_CASE(0x607)  /* hgeie       */
    _CSR_READ_CASE(0x60A)  /* henvcfg     */
    _CSR_READ_CASE(0x643)  /* htval       */
    _CSR_READ_CASE(0x644)  /* hip         */
    _CSR_READ_CASE(0x645)  /* hvip        */
    _CSR_READ_CASE(0x64A)  /* htinst      */
    _CSR_READ_CASE(0x680)  /* hgatp       */
    _CSR_READ_CASE(0xE12)  /* hgeip       */

    /* ----- VS-level CSRs ----- */
    _CSR_READ_CASE(0x200)  /* vsstatus    */
    _CSR_READ_CASE(0x204)  /* vsie        */
    _CSR_READ_CASE(0x205)  /* vstvec      */
    _CSR_READ_CASE(0x240)  /* vsscratch   */
    _CSR_READ_CASE(0x241)  /* vsepc       */
    _CSR_READ_CASE(0x242)  /* vscause     */
    _CSR_READ_CASE(0x243)  /* vstval      */
    _CSR_READ_CASE(0x244)  /* vsip        */
    _CSR_READ_CASE(0x280)  /* vsatp       */

    /* ----- M-mode hypervisor extension ----- */
    _CSR_READ_CASE(0x34A)  /* mtinst      */
    _CSR_READ_CASE(0x34B)  /* mtval2      */
#endif /* ENABLE_HYP */

    default:
        break;
    }

    return val;
}

/* ===================================================================
 * csr_write - write a CSR by runtime address
 * =================================================================== */
void csr_write(uint16_t csr, uintptr_t val) {
    switch (csr) {
    /* ----- pmpcfg registers (0x3A0 - 0x3AF) ----- */
    _CSR_WRITE_CASE(0x3A0)
    _CSR_WRITE_CASE(0x3A1)
    _CSR_WRITE_CASE(0x3A2)
    _CSR_WRITE_CASE(0x3A3)
    _CSR_WRITE_CASE(0x3A4)
    _CSR_WRITE_CASE(0x3A5)
    _CSR_WRITE_CASE(0x3A6)
    _CSR_WRITE_CASE(0x3A7)
    _CSR_WRITE_CASE(0x3A8)
    _CSR_WRITE_CASE(0x3A9)
    _CSR_WRITE_CASE(0x3AA)
    _CSR_WRITE_CASE(0x3AB)
    _CSR_WRITE_CASE(0x3AC)
    _CSR_WRITE_CASE(0x3AD)
    _CSR_WRITE_CASE(0x3AE)
    _CSR_WRITE_CASE(0x3AF)

    /* ----- pmpaddr registers (0x3B0 - 0x3EF) ----- */
    _CSR_WRITE_CASE(0x3B0)
    _CSR_WRITE_CASE(0x3B1)
    _CSR_WRITE_CASE(0x3B2)
    _CSR_WRITE_CASE(0x3B3)
    _CSR_WRITE_CASE(0x3B4)
    _CSR_WRITE_CASE(0x3B5)
    _CSR_WRITE_CASE(0x3B6)
    _CSR_WRITE_CASE(0x3B7)
    _CSR_WRITE_CASE(0x3B8)
    _CSR_WRITE_CASE(0x3B9)
    _CSR_WRITE_CASE(0x3BA)
    _CSR_WRITE_CASE(0x3BB)
    _CSR_WRITE_CASE(0x3BC)
    _CSR_WRITE_CASE(0x3BD)
    _CSR_WRITE_CASE(0x3BE)
    _CSR_WRITE_CASE(0x3BF)
    _CSR_WRITE_CASE(0x3C0)
    _CSR_WRITE_CASE(0x3C1)
    _CSR_WRITE_CASE(0x3C2)
    _CSR_WRITE_CASE(0x3C3)
    _CSR_WRITE_CASE(0x3C4)
    _CSR_WRITE_CASE(0x3C5)
    _CSR_WRITE_CASE(0x3C6)
    _CSR_WRITE_CASE(0x3C7)
    _CSR_WRITE_CASE(0x3C8)
    _CSR_WRITE_CASE(0x3C9)
    _CSR_WRITE_CASE(0x3CA)
    _CSR_WRITE_CASE(0x3CB)
    _CSR_WRITE_CASE(0x3CC)
    _CSR_WRITE_CASE(0x3CD)
    _CSR_WRITE_CASE(0x3CE)
    _CSR_WRITE_CASE(0x3CF)
    _CSR_WRITE_CASE(0x3D0)
    _CSR_WRITE_CASE(0x3D1)
    _CSR_WRITE_CASE(0x3D2)
    _CSR_WRITE_CASE(0x3D3)
    _CSR_WRITE_CASE(0x3D4)
    _CSR_WRITE_CASE(0x3D5)
    _CSR_WRITE_CASE(0x3D6)
    _CSR_WRITE_CASE(0x3D7)
    _CSR_WRITE_CASE(0x3D8)
    _CSR_WRITE_CASE(0x3D9)
    _CSR_WRITE_CASE(0x3DA)
    _CSR_WRITE_CASE(0x3DB)
    _CSR_WRITE_CASE(0x3DC)
    _CSR_WRITE_CASE(0x3DD)
    _CSR_WRITE_CASE(0x3DE)
    _CSR_WRITE_CASE(0x3DF)
    _CSR_WRITE_CASE(0x3E0)
    _CSR_WRITE_CASE(0x3E1)
    _CSR_WRITE_CASE(0x3E2)
    _CSR_WRITE_CASE(0x3E3)
    _CSR_WRITE_CASE(0x3E4)
    _CSR_WRITE_CASE(0x3E5)
    _CSR_WRITE_CASE(0x3E6)
    _CSR_WRITE_CASE(0x3E7)
    _CSR_WRITE_CASE(0x3E8)
    _CSR_WRITE_CASE(0x3E9)
    _CSR_WRITE_CASE(0x3EA)
    _CSR_WRITE_CASE(0x3EB)
    _CSR_WRITE_CASE(0x3EC)
    _CSR_WRITE_CASE(0x3ED)
    _CSR_WRITE_CASE(0x3EE)
    _CSR_WRITE_CASE(0x3EF)

    /* ----- mseccfg (0x747) ----- */
    _CSR_WRITE_CASE(0x747)
#if __riscv_xlen == 32
    _CSR_WRITE_CASE(0x757)  /* mseccfgh */
#endif

    /* ----- Common CSRs ----- */
    _CSR_WRITE_CASE(0x300)  /* mstatus */
    _CSR_WRITE_CASE(0x302)  /* medeleg */
    _CSR_WRITE_CASE(0x303)  /* mideleg */
    _CSR_WRITE_CASE(0x304)  /* mie */
    _CSR_WRITE_CASE(0x305)  /* mtvec */
    _CSR_WRITE_CASE(0x340)  /* mscratch */
    _CSR_WRITE_CASE(0x341)  /* mepc */
    _CSR_WRITE_CASE(0x342)  /* mcause */
    _CSR_WRITE_CASE(0x343)  /* mtval */
    _CSR_WRITE_CASE(0x344)  /* mip */
    _CSR_WRITE_CASE(0x100)  /* sstatus */
    _CSR_WRITE_CASE(0x104)  /* sie */
    _CSR_WRITE_CASE(0x105)  /* stvec */
    _CSR_WRITE_CASE(0x140)  /* sscratch */
    _CSR_WRITE_CASE(0x141)  /* sepc */
    _CSR_WRITE_CASE(0x142)  /* scause */
    _CSR_WRITE_CASE(0x143)  /* stval */
    _CSR_WRITE_CASE(0x144)  /* sip */
    _CSR_WRITE_CASE(0x180)  /* satp */
    _CSR_WRITE_CASE(0x10A)  /* senvcfg */
    _CSR_WRITE_CASE(0x30A)  /* menvcfg */

    /* ----- Sstc extension CSRs ----- */
    _CSR_WRITE_CASE(0x14D)  /* stimecmp  */
    _CSR_WRITE_CASE(0x24D)  /* vstimecmp */

    /* ----- scounteren (0x106) ----- */
    _CSR_WRITE_CASE(0x106)  /* scounteren */

    /* ----- mcounteren (0x306) ----- */
    _CSR_WRITE_CASE(0x306)  /* mcounteren */

    /* ----- mcountinhibit (0x320) ----- */
    _CSR_WRITE_CASE(0x320)  /* mcountinhibit */

    /* ----- mhpmevent registers (0x323 - 0x33F) ----- */
    _CSR_WRITE_CASE(0x323)  _CSR_WRITE_CASE(0x324)  _CSR_WRITE_CASE(0x325)
    _CSR_WRITE_CASE(0x326)  _CSR_WRITE_CASE(0x327)  _CSR_WRITE_CASE(0x328)
    _CSR_WRITE_CASE(0x329)  _CSR_WRITE_CASE(0x32A)  _CSR_WRITE_CASE(0x32B)
    _CSR_WRITE_CASE(0x32C)  _CSR_WRITE_CASE(0x32D)  _CSR_WRITE_CASE(0x32E)
    _CSR_WRITE_CASE(0x32F)  _CSR_WRITE_CASE(0x330)  _CSR_WRITE_CASE(0x331)
    _CSR_WRITE_CASE(0x332)  _CSR_WRITE_CASE(0x333)  _CSR_WRITE_CASE(0x334)
    _CSR_WRITE_CASE(0x335)  _CSR_WRITE_CASE(0x336)  _CSR_WRITE_CASE(0x337)
    _CSR_WRITE_CASE(0x338)  _CSR_WRITE_CASE(0x339)  _CSR_WRITE_CASE(0x33A)
    _CSR_WRITE_CASE(0x33B)  _CSR_WRITE_CASE(0x33C)  _CSR_WRITE_CASE(0x33D)
    _CSR_WRITE_CASE(0x33E)  _CSR_WRITE_CASE(0x33F)

    /* ----- mcycle / minstret (0xB00, 0xB02) ----- */
    _CSR_WRITE_CASE(0xB00)  /* mcycle */
    _CSR_WRITE_CASE(0xB02)  /* minstret */

    /* ----- mhpmcounter registers (0xB03 - 0xB1F) ----- */
    _CSR_WRITE_CASE(0xB03)  _CSR_WRITE_CASE(0xB04)  _CSR_WRITE_CASE(0xB05)
    _CSR_WRITE_CASE(0xB06)  _CSR_WRITE_CASE(0xB07)  _CSR_WRITE_CASE(0xB08)
    _CSR_WRITE_CASE(0xB09)  _CSR_WRITE_CASE(0xB0A)  _CSR_WRITE_CASE(0xB0B)
    _CSR_WRITE_CASE(0xB0C)  _CSR_WRITE_CASE(0xB0D)  _CSR_WRITE_CASE(0xB0E)
    _CSR_WRITE_CASE(0xB0F)  _CSR_WRITE_CASE(0xB10)  _CSR_WRITE_CASE(0xB11)
    _CSR_WRITE_CASE(0xB12)  _CSR_WRITE_CASE(0xB13)  _CSR_WRITE_CASE(0xB14)
    _CSR_WRITE_CASE(0xB15)  _CSR_WRITE_CASE(0xB16)  _CSR_WRITE_CASE(0xB17)
    _CSR_WRITE_CASE(0xB18)  _CSR_WRITE_CASE(0xB19)  _CSR_WRITE_CASE(0xB1A)
    _CSR_WRITE_CASE(0xB1B)  _CSR_WRITE_CASE(0xB1C)  _CSR_WRITE_CASE(0xB1D)
    _CSR_WRITE_CASE(0xB1E)  _CSR_WRITE_CASE(0xB1F)

#if __riscv_xlen == 32
    /* ----- mcycleh / minstreth (RV32 high-half) ----- */
    _CSR_WRITE_CASE(0xB80)  /* mcycleh */
    _CSR_WRITE_CASE(0xB82)  /* minstreth */

    /* ----- mhpmcounterh registers (0xB83 - 0xB9F, RV32 high-half) ----- */
    _CSR_WRITE_CASE(0xB83)  _CSR_WRITE_CASE(0xB84)  _CSR_WRITE_CASE(0xB85)
    _CSR_WRITE_CASE(0xB86)  _CSR_WRITE_CASE(0xB87)  _CSR_WRITE_CASE(0xB88)
    _CSR_WRITE_CASE(0xB89)  _CSR_WRITE_CASE(0xB8A)  _CSR_WRITE_CASE(0xB8B)
    _CSR_WRITE_CASE(0xB8C)  _CSR_WRITE_CASE(0xB8D)  _CSR_WRITE_CASE(0xB8E)
    _CSR_WRITE_CASE(0xB8F)  _CSR_WRITE_CASE(0xB90)  _CSR_WRITE_CASE(0xB91)
    _CSR_WRITE_CASE(0xB92)  _CSR_WRITE_CASE(0xB93)  _CSR_WRITE_CASE(0xB94)
    _CSR_WRITE_CASE(0xB95)  _CSR_WRITE_CASE(0xB96)  _CSR_WRITE_CASE(0xB97)
    _CSR_WRITE_CASE(0xB98)  _CSR_WRITE_CASE(0xB99)  _CSR_WRITE_CASE(0xB9A)
    _CSR_WRITE_CASE(0xB9B)  _CSR_WRITE_CASE(0xB9C)  _CSR_WRITE_CASE(0xB9D)
    _CSR_WRITE_CASE(0xB9E)  _CSR_WRITE_CASE(0xB9F)
#endif

#ifdef ENABLE_HYP
    /* ----- Hypervisor CSRs (HS-level) ----- */
    _CSR_WRITE_CASE(0x600)  /* hstatus     */
    _CSR_WRITE_CASE(0x602)  /* hedeleg     */
    _CSR_WRITE_CASE(0x603)  /* hideleg     */
    _CSR_WRITE_CASE(0x604)  /* hie         */
    _CSR_WRITE_CASE(0x605)  /* htimedelta  */
    _CSR_WRITE_CASE(0x606)  /* hcounteren  */
    _CSR_WRITE_CASE(0x607)  /* hgeie       */
    _CSR_WRITE_CASE(0x60A)  /* henvcfg     */
    _CSR_WRITE_CASE(0x643)  /* htval       */
    _CSR_WRITE_CASE(0x644)  /* hip         */
    _CSR_WRITE_CASE(0x645)  /* hvip        */
    _CSR_WRITE_CASE(0x64A)  /* htinst      */
    _CSR_WRITE_CASE(0x680)  /* hgatp       */

    /* ----- VS-level CSRs ----- */
    _CSR_WRITE_CASE(0x200)  /* vsstatus    */
    _CSR_WRITE_CASE(0x204)  /* vsie        */
    _CSR_WRITE_CASE(0x205)  /* vstvec      */
    _CSR_WRITE_CASE(0x240)  /* vsscratch   */
    _CSR_WRITE_CASE(0x241)  /* vsepc       */
    _CSR_WRITE_CASE(0x242)  /* vscause     */
    _CSR_WRITE_CASE(0x243)  /* vstval      */
    _CSR_WRITE_CASE(0x244)  /* vsip        */
    _CSR_WRITE_CASE(0x280)  /* vsatp       */

    /* ----- M-mode hypervisor extension ----- */
    _CSR_WRITE_CASE(0x34A)  /* mtinst      */
    _CSR_WRITE_CASE(0x34B)  /* mtval2      */
#endif /* ENABLE_HYP */

    default:
        break;
    }
}
