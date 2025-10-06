@ Simple GBA test ROM - Just write 3 colored pixels to VRAM
@ No interrupts, no waiting, just direct VRAM writes

.section .text
.arm
.global _start

@ GBA ROM header (required at 0x08000000)
    b _start                @ Branch to start
    
    @ Nintendo Logo (156 bytes at 0x04-0x9F) - Required for BIOS validation!
    .byte 0x24,0xFF,0xAE,0x51,0x69,0x9A,0xA2,0x21,0x3D,0x84,0x82,0x0A,0x84,0xE4,0x09,0xAD
    .byte 0x11,0x24,0x8B,0x98,0xC0,0x81,0x7F,0x21,0xA3,0x52,0xBE,0x19,0x93,0x09,0xCE,0x20
    .byte 0x10,0x46,0x4A,0x4A,0xF8,0x27,0x31,0xEC,0x58,0xC7,0xE8,0x33,0x82,0xE3,0xCE,0xBF
    .byte 0x85,0xF4,0xDF,0x94,0xCE,0x4B,0x09,0xC1,0x94,0x56,0x8A,0xC0,0x13,0x72,0xA7,0xFC
    .byte 0x9F,0x84,0x4D,0x73,0xA3,0xCA,0x9A,0x61,0x58,0x97,0xA3,0x27,0xFC,0x03,0x98,0x76
    .byte 0x23,0x1D,0xC7,0x61,0x03,0x04,0xAE,0x56,0xBF,0x38,0x84,0x00,0x40,0xA7,0x0E,0xFD
    .byte 0xFF,0x52,0xFE,0x03,0x6F,0x95,0x30,0xF1,0x97,0xFB,0xC0,0x85,0x60,0xD6,0x80,0x25
    .byte 0xA9,0x63,0xBE,0x03,0x01,0x4E,0x38,0xE2,0xF9,0xA2,0x34,0xFF,0xBB,0x3E,0x03,0x44
    .byte 0x78,0x00,0x90,0xCB,0x88,0x11,0x3A,0x94,0x65,0xC0,0x7C,0x63,0x87,0xF0,0x3C,0xAF
    .byte 0xD6,0x25,0xE4,0x8B,0x38,0x0A,0xAC,0x72,0x21,0xD4,0xF8,0x07
    
    .ascii "TESTPIXELS  "   @ Game title (12 bytes)
    .ascii "TEST"           @ Game code (4 bytes)
    .ascii "01"             @ Maker code (2 bytes)
    .byte 0x96              @ Fixed value
    .byte 0                 @ Main unit code
    .byte 0                 @ Device type
    .fill 7, 1, 0           @ Reserved
    .byte 0                 @ Software version
    .byte 0                 @ Complement check (will be fixed by gbafix)
    .fill 2, 1, 0           @ Reserved

_start:
    @ Set up stack pointer (not really needed for this simple test)
    ldr sp, =0x03007F00
    
    @ Enable Mode 3 (bitmap mode) and BG2
    @ DISPCNT = 0x04000000
    @ Mode 3 = 0x0003, BG2 enable = 0x0400
    @ Combined: 0x0403
    ldr r0, =0x04000000     @ DISPCNT register address
    mov r1, #0x0403         @ Mode 3 + BG2
    strh r1, [r0]           @ Write to DISPCNT
    
    @ Now write 3 colored pixels to VRAM
    @ VRAM starts at 0x06000000
    @ Mode 3: 240x160, 16-bit RGB555 format
    @ Pixel address = VRAM_BASE + (y * 240 + x) * 2
    
    ldr r2, =0x06000000     @ VRAM base address
    
    @ Red pixel at (120, 80)
    @ Address = 0x06000000 + (80 * 240 + 120) * 2
    @ = 0x06000000 + 19320 * 2 = 0x06000000 + 0x9780 = 0x06009780
    mov r0, #80             @ y coordinate
    mov r1, #240            @ screen width
    mul r3, r0, r1          @ r3 = y * 240
    add r3, r3, #120        @ r3 = y * 240 + x
    mov r3, r3, lsl #1      @ r3 = (y * 240 + x) * 2 (byte offset)
    add r4, r2, r3          @ r4 = VRAM address
    mov r5, #0x001F         @ Red color (RGB555: 0bbbbbgggggrrrrr)
    strh r5, [r4]           @ Write red pixel
    
    @ Green pixel at (136, 80)
    @ Address = 0x06000000 + (80 * 240 + 136) * 2
    @ = 0x06000000 + 19336 * 2 = 0x06000000 + 0x9790 = 0x06009790
    mov r0, #80             @ y coordinate
    mov r1, #240            @ screen width
    mul r3, r0, r1          @ r3 = y * 240
    add r3, r3, #136        @ r3 = y * 240 + x
    mov r3, r3, lsl #1      @ r3 = (y * 240 + x) * 2
    add r4, r2, r3          @ r4 = VRAM address
    mov r5, #0x03E0         @ Green color (RGB555: 0bbbbbgggggrrrrr)
    strh r5, [r4]           @ Write green pixel
    
    @ Blue pixel at (152, 80) - same row as red and green
    @ Address = 0x06000000 + (80 * 240 + 152) * 2
    @ = 0x06000000 + 19352 * 2 = 0x06000000 + 0x9730 = 0x06009730
    mov r0, #80             @ y coordinate (same as red/green)
    mov r1, #240            @ screen width
    mul r3, r0, r1          @ r3 = y * 240
    add r3, r3, #152        @ r3 = y * 240 + x (152 instead of 120)
    mov r3, r3, lsl #1      @ r3 = (y * 240 + x) * 2
    add r4, r2, r3          @ r4 = VRAM address
    mov r5, #0x7C00         @ Blue color (RGB555: 0bbbbbgggggrrrrr)
    strh r5, [r4]           @ Write blue pixel

infinite_loop:
    @ Infinite loop - just keep running
    b infinite_loop

.pool
