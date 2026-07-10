/* Sscsrind extension linker script */

INCLUDE "link.ld"

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

    . = ALIGN(16);
    .smode_stack (NOLOAD) : {
        PROVIDE(__smode_stack_start = .);
        . += 16 * 1024;
        . = ALIGN(16);
        PROVIDE(__smode_stack_end = .);
    }

    /* Hypervisor extension page tables */
    . = ALIGN(0x1000);
    .page_tables (NOLOAD) : {
        PROVIDE(__page_tables_start = .);
        . += 64 * 4096;
        PROVIDE(__page_tables_end = .);
    }

    /* G-stage page table pool, 16KB-aligned (required by common/hyp) */
    . = ALIGN(0x4000);
    .gpt_page_tables (NOLOAD) : {
        PROVIDE(__gpt_pool_start = .);
        . += 64 * 4096;
        PROVIDE(__gpt_pool_end = .);
    }

    /* HTIF tohost/fromhost (required by Spike/Sail) */
    .tohost : {
        *(.tohost)
    }

    _end = .;
}
