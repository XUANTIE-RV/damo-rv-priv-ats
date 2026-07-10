/* Sstvala extension linker script - includes base link.ld */

/* Include the base link.ld first */
INCLUDE "link.ld"

/* Extension-specific sections (placed after link.ld's sections) */
SECTIONS
{
    /* Define _start symbol (points to the beginning of code) */
    _start = rvtest_entry_point;

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

    /* VM test pages - 3 x 4KB pages in a 2MB-aligned region.
     * setup_code_mapping() skips this 2MB region so individual tests
     * can install 4KB mappings with custom permissions.
     *
     * Layout:
     *   Page 0: test_data_area   (read/write data)
     *   Page 1: test_fault_page  (permission/fault tests)
     *   Page 2: test_exec_page   (executable code tests)
     */
    . = ALIGN(0x200000);  /* 2MB alignment */
    .vm_test_region (NOLOAD) : {
        PROVIDE(__vm_test_region_start = .);
        . += 3 * 4096;
        PROVIDE(__vm_test_region_end = .);
    }

    /* Dedicated 2 MiB megapage region (for consistency with other
     * extensions; setup_code_mapping needs the symbol to compute
     * the skip range). */
    . = ALIGN(0x200000);
    .vm_test_region_2m (NOLOAD) : {
        PROVIDE(__vm_test_region_2m_start = .);
        . += 0x200000;    /* 2 MiB */
        PROVIDE(__vm_test_region_2m_end = .);
    }

    /* PMP test region placeholders (required by pmp_init.S but not used
     * by Sstvala tests). Provide zero-size regions so the linker is happy. */
    . = ALIGN(8);
    PROVIDE(__pmp_test_code = .);
    PROVIDE(__pmp_test_nomap = .);

    /* Override _end from link.ld (must be direct assignment, not PROVIDE) */
    _end = .;
}
