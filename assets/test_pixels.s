@ Simple GBA test ROM - Just write 3 colored pixels to VRAM
@ No interrupts, no waiting, just direct VRAM writes

.section .text
.arm
.global _start

@ GBA ROM header (required at 0x08000000)
    b _start                @ Branch to start
    .fill 156, 1, 0         @ Nintendo logo space (156 bytes)
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
