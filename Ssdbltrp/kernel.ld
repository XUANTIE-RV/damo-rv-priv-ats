OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
    . = 0x80000000;

    .text : {
        *(.text.init)
        *(.text .text.*)
    }

    .rodata : {
        *(.rodata .rodata.*)
    }

    .data : {
        *(.data .data.*)
    }

    .bss : {
        _bss_start = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        _bss_end = .;
    }

    .test_table : {
        _test_table = .;
        *(.test_table)
        _test_table_end = .;
    }

    .stack (NOLOAD) : {
        . = ALIGN(16);
        . = . + 4096;
        __stack_end = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.note*)
    }
}
