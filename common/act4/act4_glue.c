/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * ===================================================================
 * common/act4/act4_glue.c — ACT4 trap handler, C side
 *
 * Capability probes, T-SBI privilege switching, and the reporting
 * helpers the handler calls by name.
 * ===================================================================
 */

/* The suite headers are reached with an explicit ../, because this
 * directory also holds ACT4's own encoding.h and the compiler searches
 * here first. */
#include "../types.h"
#include "../encoding.h"
#include "../uart.h"
#include "../test_framework.h"
#include "act4/act4.h"

#ifdef ACT4_TRAP_HANDLER

extern void _halt_fail(void) __attribute__((noreturn));

/* ===================================================================
 * Capability probes
 *
 * The handler reads hypervisor trap-value CSRs on the way into every
 * trap. Reading one that does not exist raises an illegal instruction
 * inside the handler, which re-enters it and hangs, so both sides are
 * gated.
 *
 * The M-side gate is a live misa.H test in the handler itself and
 * needs nothing here. The two gates below cover what a misa.H test
 * cannot: hypervisor CSRs read from HS-mode, where misa is not
 * readable at all, and mtval2 on a part that has Ssdbltrp but not H
 * (norm:mtval2_Ssdbltrap), which nothing advertises.
 * =================================================================== */

void act4_probe_caps(void) {
    bool have_h = ((CSRR(misa) >> 7) & 1UL) != 0;

    act4_cap_hyp = have_h ? 1 : 0;

    if (have_h) {
        act4_cap_xtval2 = 1;    /* mtval2 and mtinst come with H */
        return;
    }

    /* Probe mtval2 with a trap expectation armed. The gate is still
     * clear, so the handler will not touch mtval2 while the probe trap
     * is being taken. reset_state() re-probes before every test via
     * trap_probe_mtval2(), which calls act4_set_cap_xtval2(). */
    act4_cap_xtval2 = 0;
    trap_expect_begin();
    (void)CSRR(CSR_MTVAL2);
    act4_cap_xtval2 = trap_was_triggered() ? 0 : 1;
    trap_expect_end();
    trap_clear_record();
}

void act4_set_cap_xtval2(bool present) {
    act4_cap_xtval2 = present ? 1 : 0;
}

/* ===================================================================
 * act4_trap_setup - install the handler
 *
 * Called once from entry.S at boot and again from reset_state()
 * before every test. The first call runs the ACT4 prolog; later ones
 * put xTVEC and xSCRATCH back where the prolog left them, in case a
 * test moved either.
 * =================================================================== */

static bool act4_prolog_done;

void act4_trap_setup(void) {
    if (!act4_prolog_done) {
        act4_trap_init();
        act4_prolog_done = true;
        act4_probe_caps();
        return;
    }
    act4_reinstall_tvecs();
}

/* ===================================================================
 * Trapping state for handlers reached outside the ACT4 trampoline
 *
 * m_trap_handler() and s_trap_handler() read act4_trap_entry rather
 * than the live CSRs, since on the normal path the handler has
 * already advanced xEPC by the time they run. A test that points
 * mtvec or stvec at the suite's own entry stubs still reaches them,
 * with nothing in between to fill the record — so the stubs call
 * these, which capture exactly what the handler would have.
 * =================================================================== */

void act4_trap_entry_capture_m(void) {
    act4_trap_entry.mode    = ACT4_MODE_M;
    act4_trap_entry.cause   = CSRR(mcause);
    act4_trap_entry.epc     = CSRR(mepc);
    act4_trap_entry.tval    = CSRR(mtval);
    act4_trap_entry.status  = CSRR(mstatus);
    act4_trap_entry.mstatus = act4_trap_entry.status;
    act4_trap_entry.tval2   = 0;
    act4_trap_entry.tinst   = 0;
    act4_trap_entry.hstatus = 0;
#ifdef ENABLE_HYP
    if ((CSRR(misa) >> 7) & 1UL) {
        act4_trap_entry.tval2   = CSRR(CSR_MTVAL2);
        act4_trap_entry.tinst   = CSRR(CSR_MTINST);
        act4_trap_entry.hstatus = CSRR(CSR_HSTATUS);
    } else if (act4_cap_xtval2) {
        act4_trap_entry.tval2 = CSRR(CSR_MTVAL2);
        act4_trap_entry.tinst = CSRR(CSR_MTINST);
    }
#endif
}

void act4_trap_entry_capture_s(void) {
    act4_trap_entry.mode    = ACT4_MODE_S;
    act4_trap_entry.cause   = CSRR(scause);
    act4_trap_entry.epc     = CSRR(sepc);
    act4_trap_entry.tval    = CSRR(stval);
    act4_trap_entry.status  = CSRR(sstatus);
    act4_trap_entry.mstatus = 0;            /* not readable below M-mode */
    act4_trap_entry.tval2   = 0;
    act4_trap_entry.tinst   = 0;
    act4_trap_entry.hstatus = 0;
#ifdef ENABLE_HYP
    if (act4_cap_hyp) {
        act4_trap_entry.tval2   = CSRR(CSR_HTVAL);
        act4_trap_entry.tinst   = CSRR(CSR_HTINST);
        act4_trap_entry.hstatus = CSRR(CSR_HSTATUS);
    }
#endif
}

/* ===================================================================
 * T-SBI privilege switching
 * =================================================================== */

uintptr_t act4_tsbi_goto_op(unsigned priv) {
    switch (priv) {
    case PRIV_M:  return ACT4_TSBI_GOTO_MMODE;
    case PRIV_S:  return ACT4_TSBI_GOTO_SMODE;
    case PRIV_U:  return ACT4_TSBI_GOTO_UMODE;
#ifdef ENABLE_HYP
    case PRIV_VS: return ACT4_TSBI_GOTO_VSMODE;
    case PRIV_VU: return ACT4_TSBI_GOTO_VUMODE;
#endif
    default:      return 0;
    }
}

void act4_tsbi_goto_priv(unsigned priv) {
    uintptr_t op = act4_tsbi_goto_op(priv);
    if (op == 0) {
        printf("ERROR: no T-SBI opcode for privilege %u\n", priv);
        return;
    }
    (void)act4_tsbi_call(op, 0);
}

/* ===================================================================
 * Reporting helpers called from the handler
 * =================================================================== */

extern char ascii_buffer[];

void act4_io_write_str(const char *s) {
    while (*s)
        uart_putc(*s++);
}

void act4_hex_to_str(uintptr_t value, unsigned width) {
    static const char digits[] = "0123456789abcdef";
    unsigned nibbles = (width + 3) / 4;
    if (nibbles == 0 || nibbles > 16)
        nibbles = 2 * sizeof(uintptr_t);
    for (unsigned i = 0; i < nibbles; i++)
        ascii_buffer[i] = digits[(value >> (4 * (nibbles - 1 - i))) & 0xF];
    ascii_buffer[nibbles] = '\0';
}

void act4_report_fatal(const char *msg) {
    act4_io_write_str(msg);
    printf("  handler mode = %u\n", (unsigned)act4_trap_entry.mode);
    printf("  cause  = 0x%lx\n", (unsigned long)act4_trap_entry.cause);
    printf("  epc    = 0x%lx\n", (unsigned long)act4_trap_entry.epc);
    printf("  tval   = 0x%lx\n", (unsigned long)act4_trap_entry.tval);
    printf("  status = 0x%lx\n", (unsigned long)act4_trap_entry.status);
    printf("  traps taken = %lu\n", (unsigned long)rvtest_trap_count);
    _halt_fail();
}

#endif /* ACT4_TRAP_HANDLER */
