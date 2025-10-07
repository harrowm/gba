@ Minimal GBA Display Test ROM
@ This ROM initializes the display to show a solid color background
@ to verify basic emulator display functionality

.section .text
.global _start
.arm

@ GBA Header
.org 0x00
    b       rom_start           @ Branch to start
    .fill   156, 1, 0          @ Nintendo logo space (will be fixed by gbafix)
    
    .byte   0x00,0x00,0x00,0x00  @ Game title (empty)
    .byte   0x00,0x00,0x00,0x00
    .byte   0x00,0x00,0x00,0x00
    
    .byte   0x00,0x00,0x00,0x00  @ Game code
    .byte   0x00,0x00            @ Maker code
    .byte   0x96                 @ Fixed value
    .byte   0x00                 @ Main unit code
    .byte   0x00                 @ Device type
    .fill   7, 1, 0             @ Reserved
    .byte   0x00                 @ Software version
    .byte   0x00                 @ Complement check (will be fixed by gbafix)
    .fill   2, 1, 0             @ Reserved

@ ROM start code
.org 0xC0
rom_start:
    @ Setup stack pointer
    mov     r0, #0x04000000     @ I/O registers base
    add     r0, r0, #0x00000000
    
    @ Set IRQ stack pointer
    mov     sp, #0x03000000     @ IWRAM
    add     sp, sp, #0x7F00     @ Top of IWRAM (32KB - 256 bytes for IRQ)

    @ Enable all interrupts in IME (just in case)
    mov     r1, #0x04000000
    mov     r2, #1
    str     r2, [r1, #0x208]    @ IME = 1

    @ Initialize display
    @ Set DISPCNT (0x04000000) to Mode 3 (bitmap mode) with BG2 enabled
    @ Mode 3 = 0x0003
    @ BG2 Enable = 0x0400
    @ Combined = 0x0403
    mov     r0, #0x04000000
    mov     r1, #0x0003         @ Mode 3
    orr     r1, r1, #0x0400     @ Enable BG2
    strh    r1, [r0]            @ Write to DISPCNT

    @ Fill screen with a gradient pattern
    @ VRAM starts at 0x06000000
    @ Mode 3: 240x160 pixels, 16-bit color (5-5-5 RGB)
    mov     r0, #0x06000000     @ VRAM base
    mov     r1, #0              @ Y counter (0-159)
    
fill_y_loop:
    mov     r2, #0              @ X counter (0-239)
    
fill_x_loop:
    @ Create color based on position
    @ Red increases with X, Green increases with Y, Blue is constant
    mov     r3, r2, lsr #3      @ X / 8 = Red component (0-31)
    mov     r4, r1, lsr #2      @ Y / 4 = Green component (0-31)
    mov     r5, #15             @ Blue component (constant)
    
    @ Combine into 16-bit color: 0bBBBBBGGGGGRRRRR
    orr     r6, r3, r4, lsl #5  @ Combine Red and Green
    orr     r6, r6, r5, lsl #10 @ Add Blue
    
    strh    r6, [r0]            @ Write pixel
    add     r0, r0, #2          @ Next pixel (2 bytes per pixel)
    
    add     r2, r2, #1          @ X++
    cmp     r2, #240
    blt     fill_x_loop
    
    add     r1, r1, #1          @ Y++
    cmp     r1, #160
    blt     fill_y_loop

    @ All done - infinite loop
infinite_loop:
    b       infinite_loop

.pool
