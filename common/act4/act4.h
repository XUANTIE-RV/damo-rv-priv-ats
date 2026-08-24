/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * ===================================================================
 * common/act4/act4.h — what the rest of the suite uses from
 *                      common/act4/
 * ===================================================================
 */

#ifndef ACT4_H
#define ACT4_H

#include "../types.h"
#include "act4/act4_config.h"
#include "act4/act4_trap_entry.h"
#include "act4/act4_tsbi.h"

/* Install the handler: xTVEC points at the ACT4 trampolines, xSCRATCH
 * at the per-mode save areas. Called from entry.S at boot and from
 * reset_state() before every test. */
void act4_trap_setup(void);

/* The two halves of the above, and the epilog that undoes them. */
void act4_trap_init(void);
void act4_trap_fini(void);
void act4_reinstall_tvecs(void);

/* The stvec value that routes traps to the handler. Not &Strampoline:
 * where stvec is not fully writable the prolog relocates the
 * trampoline, and this is the address it chose. */
uintptr_t act4_stvec_value(void);

/* Whether the handler may read the hypervisor trap-value CSRs. Probed
 * at boot, and re-probed by trap_probe_mtval2() before every test. */
void act4_probe_caps(void);
void act4_set_cap_xtval2(bool present);

/* Capture the trapping state from the live CSRs. Used by the suite's
 * own trap entry stubs, which a test may install in place of the ACT4
 * trampolines. */
void act4_trap_entry_capture_m(void);
void act4_trap_entry_capture_s(void);

/* Privilege switching over the T-SBI; see act4_tsbi.h. */
void      act4_tsbi_goto_priv(unsigned priv);
uintptr_t act4_tsbi_goto_op(unsigned priv);

/* Called from the handler to report and to end the run. */
void act4_io_write_str(const char *s);
void act4_hex_to_str(uintptr_t value, unsigned width);
void act4_report_fatal(const char *msg) __attribute__((noreturn));

/* Incremented by every handler entry. */
extern uintptr_t rvtest_trap_count;

#endif /* ACT4_H */
