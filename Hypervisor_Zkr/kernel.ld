/* Hypervisor x Zkr cross-test linker script */

INCLUDE "link.ld"

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

    /* VS-stage page table pool (4KB-aligned). */
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
        . = ALIGN(16);
        PROVIDE(__smode_stack_end = .);
    }

    /* VS-mode stack (for run_in_vs_mode trampoline). */
    . = ALIGN(16);
    .vsmode_stack (NOLOAD) : {
        PROVIDE(__vsmode_stack_start = .);
        . += 16 * 1024;
        . = ALIGN(16);
        PROVIDE(__vsmode_stack_end = .);
    }

    /* VU-mode stack (for run_in_vu_mode trampoline). */
    . = ALIGN(16);
    .vumode_stack (NOLOAD) : {
        PROVIDE(__vumode_stack_start = .);
        . += 16 * 1024;
        . = ALIGN(16);
        PROVIDE(__vumode_stack_end = .);
    }

    _end = .;
}
