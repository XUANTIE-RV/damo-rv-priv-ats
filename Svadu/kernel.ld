/* Svadu extension linker script - includes base link.ld */

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
     *   Page 1: test_fault_page  (permission/A/D fault tests)
     *   Page 2: test_exec_page   (executable code tests)
     */
    . = ALIGN(0x200000);  /* 2MB alignment */
    .vm_test_region (NOLOAD) : {
        PROVIDE(__vm_test_region_start = .);
        . += 3 * 4096;
        PROVIDE(__vm_test_region_end = .);
    }

    /* Dedicated 2 MiB megapage region for SVADU-2M-* tests.
     * Naturally 2 MiB aligned and exactly 2 MiB in size, so a single
     * megapage PTE can map this whole region. setup_code_mapping()
     * also skips the megapage that contains this region. */
    . = ALIGN(0x200000);
    .vm_test_region_2m (NOLOAD) : {
        PROVIDE(__vm_test_region_2m_start = .);
        . += 0x200000;    /* 2 MiB */
        PROVIDE(__vm_test_region_2m_end = .);
    }

    _end = .;
}
