@ Ultra Simple GBA Test - Just Red Screen
@ Sets Mode 3, enables BG2, fills VRAM with red

.section .text
.global _start
.arm

@ GBA Header
.org 0x00
    b       rom_start
    .fill   156, 1, 0          @ Nintendo logo space
    .byte   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    .byte   0x00,0x00,0x00,0x00
    .byte   0x96
    .byte   0x00
    .byte   0x00
    .fill   7, 1, 0
    .byte   0x00
    .byte   0x00
    .fill   2, 1, 0

.org 0xC0
rom_start:
    @ Set DISPCNT to Mode 3 with BG2 enabled
    ldr     r0, =0x04000000     @ DISPCNT address
    mov     r1, #0x0003         @ Mode 3
    orr     r1, r1, #0x0400     @ Add BG2 Enable
    strh    r1, [r0]

    @ Fill VRAM with red color (0x001F = pure red in 5-5-5 RGB)
    ldr     r0, =0x06000000     @ VRAM base
    ldr     r1, =0x001F         @ Red color
    orr     r1, r1, r1, lsl #16 @ Pack two pixels: 0x001F001F
    ldr     r2, =0x4B00         @ Number of words to write (76800 bytes / 4 = 19200 = 0x4B00)

fill_loop:
    str     r1, [r0], #4        @ Write word and increment by 4
    subs    r2, r2, #1
    bne     fill_loop

    @ Infinite loop
halt:
    b       halt

.pool
