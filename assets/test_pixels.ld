/* Linker script for simple GBA test ROM */

ENTRY(_start)

MEMORY {
    rom : ORIGIN = 0x08000000, LENGTH = 256K
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
}
