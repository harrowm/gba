/* Linker script for GBA ROM */

ENTRY(_start)

MEMORY
{
    rom : ORIGIN = 0x08000000, LENGTH = 32M
    iwram : ORIGIN = 0x03000000, LENGTH = 32K
    ewram : ORIGIN = 0x02000000, LENGTH = 256K
}

SECTIONS
{
    . = 0x08000000;
    
    .text : {
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
    } > rom

    .data : {
        *(.data)
        *(.data.*)
    } > iwram AT > rom

    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    } > iwram

    /DISCARD/ : {
        *(.ARM.attributes)
        *(.comment)
    }
}
