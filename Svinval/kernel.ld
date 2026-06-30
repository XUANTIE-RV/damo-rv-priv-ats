/* Svinval extension linker script - includes base link.ld */

/* Include the base link.ld first */
INCLUDE "../common/config/qemu-rv64-max/link.ld"

/* Extension-specific sections (placed after link.ld's sections) */
SECTIONS
{
    .rodata : {
        . = ALIGN(16);
        *(.srodata .srodata.*)
        . = ALIGN(16);
        *(.rodata .rodata.*)
    }

    /* Test function table (auto-collected via TEST_REGISTER) */
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

    /* Stack (128 KB) */
    .stack (NOLOAD) : {
        . = ALIGN(16);
        PROVIDE(__stack_start = .);
        . += 128 * 1024;
        . = ALIGN(16);
        PROVIDE(__stack_end = .);
    }

    /* Page table pool (VM support) */
    . = ALIGN(0x1000);
    .page_tables (NOLOAD) : {
        PROVIDE(__page_tables_start = .);
        . += 64 * 4096;    /* 256KB = 64 pages */
        PROVIDE(__page_tables_end = .);
    }

    /* S-mode stack */
    . = ALIGN(16);
    .smode_stack (NOLOAD) : {
        PROVIDE(__smode_stack_start = .);
        . += 16 * 1024;    /* 16KB S-mode stack */
        . = ALIGN(16);
        PROVIDE(__smode_stack_end = .);
    }

    /* VM test pages - placed in a separate 2MB-aligned region
     * so that setup_code_mapping() (using 2MB megapages) can
     * skip this region, allowing tests to create 4KB mappings
     * with custom permissions.
     *
     * Layout (3 pages, each 4KB):
     *   Page 0: test_data_area   (read/write test data)
     *   Page 1: test_fault_page  (permission/fault tests)
     *   Page 2: test_exec_page   (executable code tests)
     */
    . = ALIGN(0x200000);  /* 2MB alignment */
    .vm_test_region (NOLOAD) : {
        PROVIDE(__vm_test_region_start = .);
        . += 16 * 4096;   /* 16 pages (for BATCH-03 test) */
        PROVIDE(__vm_test_region_end = .);
    }

    /* PMP test region placeholders (required by pmp_init.S but not used
     * by Svinval tests). Provide zero-size regions so the linker is happy. */
    . = ALIGN(8);
    PROVIDE(__pmp_test_code = .);
    PROVIDE(__pmp_test_nomap = .);

    /* Override _end from link.ld (must be direct assignment, not PROVIDE) */
    _end = .;
}
