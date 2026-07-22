/* zicfiss extension linker script - includes base link.ld */

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
        . += 0x4000;
        PROVIDE(__stack_end = .);
    }

    /* Shadow stack test region - a 4KB page for Zicfiss shadow stack tests.
     * Requires page-level alignment for PTE shadow-stack marking (xwr=010). */
    . = ALIGN(0x1000);
    .shadow_stack_region (NOLOAD) : {
        PROVIDE(__shadow_stack_start = .);
        . += 0x1000;
        PROVIDE(__shadow_stack_end = .);
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

    /* VM test region - placed in a separate 2MB-aligned region
     * so that setup_code_mapping() (using 2MB megapages) can
     * skip this region, allowing tests to create 4KB mappings
     * with custom permissions (e.g. shadow stack pages xwr=010).
     *
     * Layout (2 pages, each 4KB):
     *   Page 0: shadow stack test page (xwr=010 when SSE=1)
     *   Page 1: normal RW test page (for comparison) */
    . = ALIGN(0x200000);  /* 2MB alignment */
    .vm_test_region (NOLOAD) : {
        PROVIDE(__vm_test_region_start = .);
        . += 2 * 4096;    /* 2 pages */
        PROVIDE(__vm_test_region_end = .);
    }

    _end = .;
}
