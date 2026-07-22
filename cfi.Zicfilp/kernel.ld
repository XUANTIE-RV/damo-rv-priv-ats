/* zicfilp extension linker script - includes base link.ld */

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

    /* CFI test code region - contains indirect jump targets for
     * testing landing pad enforcement. 4KB aligned. */
    . = ALIGN(0x1000);
    .cfi_test_code (NOLOAD) : {
        PROVIDE(__cfi_test_code_start = .);
        . += 0x1000;
        PROVIDE(__cfi_test_code_end = .);
    }

    _end = .;
}
