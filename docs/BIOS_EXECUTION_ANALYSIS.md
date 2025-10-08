# GBA BIOS Execution Analysis
**Total Instructions Executed:** 10,332
**Unique Addresses Visited:** 274
**Times Address 0x00000000 Hit:** 0

## Summary

## Hot Spots (Most Executed Code)
These addresses are executed most frequently, indicating loops:

| Address | Count | % | Instruction |
|---------|-------|---|-------------|
| 0x00000826 | 2,304 | 22.30% | `subs r2, r2, #1` |
| 0x00000828 | 2,304 | 22.30% | `bpl #0x826` |
| 0x00000800 | 257 | 2.49% | `movs r1, #2` |
| 0x00000802 | 257 | 2.49% | `lsls r1, r1, #8` |
| 0x00000804 | 257 | 2.49% | `mov ip, r1` |
| 0x00000806 | 257 | 2.49% | `ldr r3, [pc, #0x2cc]` |
| 0x00000808 | 257 | 2.49% | `ldrh r2, [r3]` |
| 0x0000080A | 257 | 2.49% | `ldr r3, [pc, #0x2c8]` |
| 0x0000080C | 257 | 2.49% | `lsls r1, r2, #0x16` |
| 0x0000080E | 257 | 2.49% | `lsrs r1, r1, #0x16` |
| 0x00000810 | 257 | 2.49% | `cmp r0, #0` |
| 0x00000812 | 257 | 2.49% | `beq #0x81c` |
| 0x00000814 | 257 | 2.49% | `cmp r1, ip` |
| 0x00000816 | 257 | 2.49% | `bge #0x82c` |
| 0x00000818 | 256 | 2.48% | `adds r2, r2, #2` |
| 0x0000081A | 256 | 2.48% | `b #0x822` |
| 0x00000822 | 256 | 2.48% | `strh r2, [r3]` |
| 0x00000824 | 256 | 2.48% | `movs r2, #8` |
| 0x0000082A | 256 | 2.48% | `b #0x800` |
| 0x00000120 | 128 | 1.24% | `str r0, [r4, r1]` |
| 0x00000122 | 128 | 1.24% | `adds r1, r1, #4` |
| 0x00000124 | 128 | 1.24% | `blt #0x120` |
| 0x000006B8 | 108 | 1.05% | `eors r3, r2` |
| 0x000006BA | 108 | 1.05% | `lsls r2, r2, #8` |
| 0x000006BC | 108 | 1.05% | `subs r5, r5, #1` |
| 0x000006BE | 108 | 1.05% | `bgt #0x6b8` |
| 0x000006B2 | 27 | 0.26% | `ldrb r2, [r0]` |
| 0x000006B4 | 27 | 0.26% | `rors r3, r4` |
| 0x000006B6 | 27 | 0.26% | `movs r5, #4` |
| 0x000006C0 | 27 | 0.26% | `adds r0, r0, #1` |

## Loop Analysis
Backward branches that execute frequently:

### Loop: 0x00000826 - 0x00000828 (2,048 iterations)
- **Branch from:** `0x00000828: bpl #0x826`
- **Branch to:** `0x00000826: subs r2, r2, #1`
- **Executed:** 2,048 times

### Loop: 0x00000800 - 0x0000082A (256 iterations)
- **Branch from:** `0x0000082A: b #0x800`
- **Branch to:** `0x00000800: movs r1, #2`
- **Executed:** 256 times

### Loop: 0x00000120 - 0x00000124 (127 iterations)
- **Branch from:** `0x00000124: blt #0x120`
- **Branch to:** `0x00000120: str r0, [r4, r1]`
- **Executed:** 127 times

### Loop: 0x000006B8 - 0x000006BE (81 iterations)
- **Branch from:** `0x000006BE: bgt #0x6b8`
- **Branch to:** `0x000006B8: eors r3, r2`
- **Executed:** 81 times

### Loop: 0x000006B2 - 0x000006C4 (26 iterations)
- **Branch from:** `0x000006C4: bgt #0x6b2`
- **Branch to:** `0x000006B2: ldrb r2, [r0]`
- **Executed:** 26 times


## Expected BIOS Flow

The GBA BIOS should follow this general flow:

1. **Reset (0x00000000)**: Initial entry point
2. **Mode Setup (0x00000068-0x000000AC)**: Set up CPU modes and stacks
3. **THUMB Initialization (0x00000118-0x00001928)**: Switch to THUMB mode, clear memory
4. **Return to ARM (0x000000B4)**: Load ROM entry point from header
5. **Jump to ROM (0x08000000)**: Start executing game code

If the BIOS executes more than ~40,000 instructions, it's stuck in a loop and not reaching the ROM.
