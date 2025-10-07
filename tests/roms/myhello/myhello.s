@ Simple GBA Mode 0 Test - Shows 4 colored tiles

.section .text
.arm
.align
.global _start

_start:
    @ Disable interrupts
    mov r0, #0x04000000
    add r0, r0, #0x208
    mov r1, #0
    str r1, [r0]

    @ DISPCNT: Mode 0, BG0 enabled
    mov r0, #0x04000000
    mov r1, #0x0100
    strh r1, [r0]

    @ BG0CNT: Screen base 30, 8bpp mode
    mov r0, #0x04000000
    add r0, r0, #8
    mov r1, #0x1E80
    strh r1, [r0]

    @ Palette: 0=black, 1=white, 2=red, 3=green, 4=blue
    mov r0, #0x05000000
    mov r1, #0
    strh r1, [r0], #2
    ldr r1, =0x7FFF
    strh r1, [r0], #2
    mov r1, #0x001F
    strh r1, [r0], #2
    ldr r1, =0x03E0
    strh r1, [r0], #2
    ldr r1, =0x7C00
    strh r1, [r0], #2

    @ Tile 1 (white)
    ldr r0, =0x06000040
    ldr r1, =0x01010101
    mov r2, #16
t1: str r1, [r0], #4
    subs r2, r2, #1
    bne t1

    @ Tile 2 (red)
    ldr r0, =0x06000080
    ldr r1, =0x02020202
    mov r2, #16
t2: str r1, [r0], #4
    subs r2, r2, #1
    bne t2

    @ Tile 3 (green)
    ldr r0, =0x060000C0
    ldr r1, =0x03030303
    mov r2, #16
t3: str r1, [r0], #4
    subs r2, r2, #1
    bne t3

    @ Tile 4 (blue)
    ldr r0, =0x06000100
    ldr r1, =0x04040404
    mov r2, #16
t4: str r1, [r0], #4
    subs r2, r2, #1
    bne t4

    @ Draw tiles at row 10, column 8
    ldr r0, =0x0600F000   @ Screen base directly
    add r0, r0, #512      @ Row 10 offset part 1 (512)
    add r0, r0, #128      @ Row 10 offset part 2 (128) = 640 total
    add r0, r0, #16       @ Column 8 offset (8 * 2)
    mov r1, #1
    strh r1, [r0], #2    @ Tile 1 (white)
    mov r1, #2
    strh r1, [r0], #2    @ Tile 2 (red)
    mov r1, #3
    strh r1, [r0], #2    @ Tile 3 (green)
    mov r1, #4
    strh r1, [r0], #2    @ Tile 4 (blue)

loop: b loop
.pool
