/*
 * Hypervisor x Zicbom cross test linker script
 *
 * Uses MEM_BASE (passed via --defsym from Makefile.common) so the
 * binary is linked at the correct address for every platform:
 *   qemu-rv64-max  -> MEM_BASE=0x80000000
 *   haps_xiaohui   -> MEM_BASE=0x60000000
 *   spike/sail     -> MEM_BASE=0x80000000
 */

OUTPUT_ARCH("riscv")
ENTRY(rvtest_entry_point)

SECTIONS
{
    . = MEM_BASE;

    .text.init    : { *(.text.init) }
    .text.rvtest  : { *(.text.rvtest) *(.text.rvtest.*) }

    . = ALIGN(0x4000);
    .data         : { *(.data) *(.sdata .sdata.*) *(.got .got.*) }

    . = ALIGN(0x1000);
    .text.rvmodel : { *(.text.rvmodel) *(.text.rvmodel.*) *(.text) *(.text.*) }

    . = ALIGN(0x1000);
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

    /* CMO test data region - cache block aligned for CBO operations.
     * 4KB provides enough space for multiple cache blocks. */
    . = ALIGN(0x1000);
    .cmo_test_data (NOLOAD) : {
        PROVIDE(__cmo_test_data_start = .);
        . += 0x1000;
        PROVIDE(__cmo_test_data_end = .);
    }

    /*
     * VM test region - 2MB aligned, used for G-stage / VS-stage
     * translation tests.
     */
    . = ALIGN(0x200000);
    .vm_test_region (NOLOAD) : {
        PROVIDE(__vm_test_region_start = .);
        . += 32 * 4096;    /* 32 pages = 128 KiB */
        PROVIDE(__vm_test_region_end = .);
    }

    _end = .;
}
