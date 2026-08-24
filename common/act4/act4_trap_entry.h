/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * ===================================================================
 * common/act4/act4_trap_entry.h — trapping state captured by the
 *                                 ACT4 trap handler
 *
 * The handler fills act4_trap_entry on the way into every trap,
 * before any dispatch path rewrites xEPC. The C trap policy reads it
 * instead of the live CSRs, which by then hold the resume address
 * rather than the fault address.
 *
 * The word indices and the struct fields must stay in step: the
 * handler stores by index, C reads by name.
 * ===================================================================
 */

#ifndef ACT4_TRAP_ENTRY_H
#define ACT4_TRAP_ENTRY_H

#include "../types.h"

/* Word offsets into act4_trap_entry, in units of REGWIDTH. */
#define ACT4_ENT_MODE      0   /* which handler took the trap           */
#define ACT4_ENT_CAUSE     1   /* mcause / scause                       */
#define ACT4_ENT_EPC       2   /* mepc / sepc, as taken                 */
#define ACT4_ENT_TVAL      3   /* mtval / stval                         */
#define ACT4_ENT_STATUS    4   /* mstatus / sstatus                     */
#define ACT4_ENT_TVAL2     5   /* mtval2 or htval, 0 if unreadable      */
#define ACT4_ENT_TINST     6   /* mtinst or htinst, 0 if unreadable     */
#define ACT4_ENT_HSTATUS   7   /* hstatus, 0 if unreadable              */
#define ACT4_ENT_MSTATUS   8   /* raw mstatus, M-side only              */
#define ACT4_ENT_NWORDS    9

/* Handler mode tags. These are ACT4's own *MODE_SIG values, so the tag
 * the handler writes needs no translation. */
#define ACT4_MODE_M        3
#define ACT4_MODE_S        1   /* S-mode and HS-mode share a tag */
#define ACT4_MODE_VS       2

/* Words in the shadow signature buffer. The widest ACT4 trap-signature
 * entry is six; round up to keep the buffer REGWIDTH*8 aligned. */
#define ACT4_TRAP_SIG_SHADOW_WORDS 8

#ifndef __ASSEMBLER__

typedef struct {
    uintptr_t mode;      /* ACT4_MODE_*                        */
    uintptr_t cause;
    uintptr_t epc;       /* the fault address, never the resume address */
    uintptr_t tval;
    uintptr_t status;
    uintptr_t tval2;
    uintptr_t tinst;
    uintptr_t hstatus;
    uintptr_t mstatus;
} act4_trap_entry_t;

extern act4_trap_entry_t act4_trap_entry;

/* Where the handler's trap-signature words land while signature
 * emission is masked:
 *   [0] vect+mode+status  [1] xcause  [2] xEPC or xIP
 *   [3] xtval or intID    [4] mtval2 or htval  [5] mtinst or htinst
 *
 * Nothing reads it during a normal run. It exists so the words the
 * handler computes stay inspectable, and so a comparison against a
 * reference signature has somewhere to read from. */
extern uintptr_t act4_trap_sig_shadow[ACT4_TRAP_SIG_SHADOW_WORDS];

/* Whether the handler may read the hypervisor trap-value CSRs.
 *   act4_cap_hyp    — htval, htinst and hstatus from HS-mode
 *   act4_cap_xtval2 — mtval2 and mtinst without H, i.e. under Ssdbltrp
 * Both are probed at boot; see act4_glue.c. */
extern uintptr_t act4_cap_hyp;
extern uintptr_t act4_cap_xtval2;

#endif /* !__ASSEMBLER__ */
#endif /* ACT4_TRAP_ENTRY_H */
