/*
 * Copyright (c) 2026 Alibaba Group.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Diagnostic test to identify root causes of failures.
 *
 * Tests the full mip→sip→vsip chain and sie↔vsie alias chain
 * with detailed register dumps at each step.
 */

TEST_REGISTER(test_diag_alias_chain);
bool test_diag_alias_chain(void) {
    TEST_BEGIN("DIAG: Full alias chain analysis");

    uintptr_t saved_hideleg = hideleg_read();
    uintptr_t saved_mideleg = csr_read(CSR_MIDELEG);
    uintptr_t saved_sie = csr_read(CSR_SIE);

    /* ---- Part 1: mip→sip→vsip chain ---- */
    printf("  === Part 1: mip→sip→vsip chain ===\n");

    /* Step 1: Set mideleg[13]=1 */
    csr_write(CSR_MIDELEG, saved_mideleg | LCOFI_BIT);
    uintptr_t mideleg_val = csr_read(CSR_MIDELEG);
    printf("  mideleg after set: 0x%lx (bit13=%d)\n",
           mideleg_val, (mideleg_val & LCOFI_BIT) ? 1 : 0);

    /* Step 2: Inject mip.LCOFIP */
    CSRS(mip, MIP_LCOFIP);
    uintptr_t mip_val = CSRR(mip);
    printf("  mip after CSRS: 0x%lx (LCOFIP=%d)\n",
           mip_val, (mip_val & MIP_LCOFIP) ? 1 : 0);

    /* Step 3: Read sip via csr_read */
    uintptr_t sip_via_csr = csr_read(CSR_SIP);
    printf("  sip via csr_read(0x144): 0x%lx (LCOFIP=%d)\n",
           sip_via_csr, (sip_via_csr & LCOFI_BIT) ? 1 : 0);

    /* Step 3b: Read sip via inline asm */
    uintptr_t sip_via_asm;
    asm volatile("csrr %0, sip" : "=r"(sip_via_asm));
    printf("  sip via asm: 0x%lx (LCOFIP=%d)\n",
           sip_via_asm, (sip_via_asm & LCOFI_BIT) ? 1 : 0);

    /* Step 4: Set hideleg[13]=1 */
    hideleg_write(saved_hideleg | LCOFI_BIT);
    uintptr_t hideleg_val = hideleg_read();
    printf("  hideleg after set: 0x%lx (bit13=%d)\n",
           hideleg_val, (hideleg_val & LCOFI_BIT) ? 1 : 0);

    /* Step 5: Read vsip via csr_read */
    uintptr_t vsip_via_csr = csr_read(CSR_VSIP);
    printf("  vsip via csr_read(0x244): 0x%lx (LCOFIP=%d)\n",
           vsip_via_csr, (vsip_via_csr & LCOFI_BIT) ? 1 : 0);

    /* Step 5b: Read vsip via inline asm */
    uintptr_t vsip_via_asm;
    asm volatile("csrr %0, 0x244" : "=r"(vsip_via_asm));
    printf("  vsip via asm(0x244): 0x%lx (LCOFIP=%d)\n",
           vsip_via_asm, (vsip_via_asm & LCOFI_BIT) ? 1 : 0);

    /* ---- Part 2: sie↔vsie alias chain ---- */
    printf("  === Part 2: sie↔vsie alias chain ===\n");

    /* Step 1: Clear sie.LCOFIE */
    csr_write(CSR_SIE, saved_sie & ~LCOFI_BIT);

    /* Step 2: Set sie.LCOFIE=1 */
    csr_write(CSR_SIE, csr_read(CSR_SIE) | LCOFI_BIT);
    uintptr_t sie_val = csr_read(CSR_SIE);
    printf("  sie after set: 0x%lx (LCOFIE=%d)\n",
           sie_val, (sie_val & LCOFI_BIT) ? 1 : 0);

    /* Step 2b: Read sie via inline asm */
    uintptr_t sie_via_asm;
    asm volatile("csrr %0, sie" : "=r"(sie_via_asm));
    printf("  sie via asm: 0x%lx (LCOFIE=%d)\n",
           sie_via_asm, (sie_via_asm & LCOFI_BIT) ? 1 : 0);

    /* Step 3: Read vsie (hideleg[13] still =1) */
    uintptr_t vsie_via_csr = csr_read(CSR_VSIE);
    printf("  vsie via csr_read(0x204): 0x%lx (LCOFIE=%d)\n",
           vsie_via_csr, (vsie_via_csr & LCOFI_BIT) ? 1 : 0);

    /* Step 3b: Read vsie via inline asm */
    uintptr_t vsie_via_asm;
    asm volatile("csrr %0, 0x204" : "=r"(vsie_via_asm));
    printf("  vsie via asm(0x204): 0x%lx (LCOFIE=%d)\n",
           vsie_via_asm, (vsie_via_asm & LCOFI_BIT) ? 1 : 0);

    /* Step 4: Write vsie.LCOFIE=1, read sie */
    csr_write(CSR_SIE, csr_read(CSR_SIE) & ~LCOFI_BIT);
    csr_write(CSR_VSIE, csr_read(CSR_VSIE) | LCOFI_BIT);
    uintptr_t sie_after_vsie = csr_read(CSR_SIE);
    printf("  sie after vsie set: 0x%lx (LCOFIE=%d)\n",
           sie_after_vsie, (sie_after_vsie & LCOFI_BIT) ? 1 : 0);

    /* Always pass — this is diagnostic only */
    TEST_ASSERT("diagnostic complete", true);

    /* Restore */
    CSRC(mip, MIP_LCOFIP);
    csr_write(CSR_SIE, saved_sie);
    csr_write(CSR_MIDELEG, saved_mideleg);
    hideleg_write(saved_hideleg);
    HYP_TEST_END();
}
