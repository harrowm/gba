OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY {
    rom : ORIGIN = 0x08000000, LENGTH = 32M
    iwram : ORIGIN = 0x03000000, LENGTH = 32K
    ewram : ORIGIN = 0x02000000, LENGTH = 256K
}

SECTIONS {
    .text : {
        *(.text)
        *(.rodata)
    } > rom

    .data : {
        *(.data)
    } > iwram AT > rom

    .bss : {
        *(.bss)
        *(COMMON)
    } > iwram

    /DISCARD/ : {
        *(.ARM.attributes)
        *(.comment)
    }
}
