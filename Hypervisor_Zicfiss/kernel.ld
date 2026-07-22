/* Hypervisor x Zicfiss extension linker script - includes base link.ld */

/* Include the base link.ld first */
INCLUDE "link.ld"

/* Extension-specific sections (placed after link.ld's sections) */
SECTIONS
{
    .rodata : {
        . = ALIGN(16);
        *(.srodata .srodata.*)
        . = ALIGN(16);
        *(.rodata .rodata.*)
    }

    .test_table : {
        . = ALIGN(8);
        _test_table = .;
        *(.test_table)
        _test_table_end = .;
    }
    _test_table_size = (_test_table_end - _test_table) / (__riscv_xlen / 8);

    .bss (NOLOAD) : {
        . = ALIGN(16);
        _bss_start = .;
        *(.sbss .sbss.*)
        . = ALIGN(16);
        *(.bss .bss.*)
        . = ALIGN(8);
        _bss_end = .;
    }

    .stack (NOLOAD) : {
        . = ALIGN(16);
        PROVIDE(__stack_start = .);
        . += 128 * 1024;
        . = ALIGN(16);
        PROVIDE(__stack_end = .);
    }

    /* VS-stage page table pool (4KB-aligned, used by two_stage.c). */
    . = ALIGN(0x1000);
    .page_tables (NOLOAD) : {
        PROVIDE(__page_tables_start = .);
        . += 64 * 4096;    /* 256KB */
        PROVIDE(__page_tables_end = .);
    }

    /* G-stage page table pool (16KB-aligned for Sv*x4 root table). */
    . = ALIGN(0x4000);
    .gpt_page_tables (NOLOAD) : {
        PROVIDE(__gpt_pool_start = .);
        . += 64 * 4096;    /* 256KB */
        PROVIDE(__gpt_pool_end = .);
    }

    /* S-mode stack (used by HS-mode trap handler). */
    . = ALIGN(16);
    .smode_stack (NOLOAD) : {
        PROVIDE(__smode_stack_start = .);
        . += 16 * 1024;
        PROVIDE(__smode_stack_end = .);
    }

    /* Shadow stack test region - a 4KB page for Zicfiss shadow stack tests.
     * Requires page-level alignment for PTE shadow-stack marking (xwr=010). */
    . = ALIGN(0x1000);
    .shadow_stack_region (NOLOAD) : {
        PROVIDE(__shadow_stack_start = .);
        . += 0x1000;
        PROVIDE(__shadow_stack_end = .);
    }

    /*
     * Test region for hypervisor Zicfiss tests.
     *  Page 0: test_data_area    (R/W data / SS page tests)
     *  Page 1: test_fault_page   (perm/V=0 fault target)
     *  Page 2: test_exec_page    (executable code tests)
     *  Page 3: test_exec_target  (X=0 fetch fault target)
     *  Page 4: ss_page           (SS page with xwr=010)
     *  Page 5: rw_page           (normal RW page)
     *  Page 6-7: scratch pages
     */
    . = ALIGN(0x200000);
    .vm_test_region (NOLOAD) : {
        PROVIDE(__vm_test_region_start = .);
        PROVIDE(test_data_area   = .); . += 4096;
        PROVIDE(test_fault_page  = .); . += 4096;
        PROVIDE(test_exec_page   = .); . += 4096;
        PROVIDE(test_exec_target = .); . += 4096;
        PROVIDE(ss_page          = .); . += 4096;
        PROVIDE(rw_page          = .); . += 4096;
        . += 2 * 4096;
        PROVIDE(__vm_test_region_end = .);
    }

    _end = .;
}
