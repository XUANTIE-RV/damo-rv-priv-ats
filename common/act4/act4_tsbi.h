/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 *
 * ===================================================================
 * common/act4/act4_tsbi.h — T-SBI calling convention, C side
 *
 * The T-SBI lets a test ask the execution environment for a service
 * without assuming a conforming M-mode: ecall, operation code in a0,
 * argument in a1, result in a0. The ACT4 trap handler answers it
 * inside the handler, so the same binary runs on a part that has no
 * conforming M-mode of its own.
 *
 * Each call sets act4_tsbi_armed immediately before its ecall, and
 * the handler consumes it. Ecalls the suite raises as stimulus are
 * left alone; see the arming gate in rvtest_trap_handler.h.
 * ===================================================================
 */

#ifndef ACT4_TSBI_H
#define ACT4_TSBI_H

#include "../types.h"

/* Operation codes; these match the TSBI_* constants in
 * common/act4/rvtest_trap_handler.h. */
#define ACT4_TSBI_GOTO_MMODE      0x00000001UL
#define ACT4_TSBI_GOTO_SMODE      0x00000002UL
#define ACT4_TSBI_GOTO_UMODE      0x00000003UL
#define ACT4_TSBI_GOTO_VSMODE     0x00000004UL
#define ACT4_TSBI_GOTO_VUMODE     0x00000005UL
#define ACT4_TSBI_ECALL_TEST      0x00000073UL

#define ACT4_TSBI_RESERVED_RET    ((uintptr_t)-1)

/* Set for exactly one ecall, then consumed by the handler: set means
 * this ecall is a T-SBI call, clear means it is an ordinary trap. */
extern uintptr_t act4_tsbi_armed;

/* ===================================================================
 * act4_tsbi_call - make one T-SBI request
 *
 * Arms the handler, executes ecall with a0 = op and a1 = arg, and
 * returns a0. The arming store and the ecall share one asm block so
 * nothing that could trap comes between them. a0 and a1 are the only
 * registers the handler may modify.
 *
 * For the GOTO operations, "returns" means execution resumes at the
 * instruction after the ecall, in the requested privilege mode.
 * =================================================================== */
static inline uintptr_t act4_tsbi_call(uintptr_t op, uintptr_t arg) {
    register uintptr_t a0 asm("a0") = op;
    register uintptr_t a1 asm("a1") = arg;
    asm volatile (
        ".option push\n\t"
        ".option norvc\n\t"
#if __riscv_xlen == 64
        "sd   %2, 0(%3)\n\t"
#else
        "sw   %2, 0(%3)\n\t"
#endif
        "ecall\n\t"
        ".option pop\n\t"
        : "+r"(a0), "+r"(a1)
        : "r"((uintptr_t)1), "r"(&act4_tsbi_armed)
        : "memory"
    );
    return a0;
}

/* Switch to a privilege level, given a suite PRIV_* value. */
void act4_tsbi_goto_priv(unsigned priv);

/* The T-SBI GOTO opcode for a suite PRIV_* value, or 0 if there is none. */
uintptr_t act4_tsbi_goto_op(unsigned priv);

/* Exercise the ecall path; returns the address of the ecall itself. */
static inline uintptr_t act4_tsbi_ecall_test(void) {
    return act4_tsbi_call(ACT4_TSBI_ECALL_TEST, 0);
}

/* Run a CSR instruction with M-mode privilege. The encoding is the
 * instruction itself, with rd = a0 and rs1 = a1, and must appear in
 * the handler's dispatch table. */
static inline uintptr_t act4_tsbi_csr(uintptr_t encoding, uintptr_t arg) {
    return act4_tsbi_call(encoding, arg);
}

#define ACT4_TSBI_CSRR(csr)  ((((uintptr_t)(csr)) << 20) | 0x02573UL) /* csrr a0, csr */
#define ACT4_TSBI_CSRW(csr)  ((((uintptr_t)(csr)) << 20) | 0x59073UL) /* csrw csr, a1 */
#define ACT4_TSBI_CSRS(csr)  ((((uintptr_t)(csr)) << 20) | 0x5a073UL) /* csrs csr, a1 */
#define ACT4_TSBI_CSRC(csr)  ((((uintptr_t)(csr)) << 20) | 0x5b073UL) /* csrc csr, a1 */

#endif /* ACT4_TSBI_H */
