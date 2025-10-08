# GBA BIOS Execution Flow Analysis

## Executive Summary

- **Total Instructions Executed**: 10,332
- **Functional Sections Identified**: 7
- **Mode Switches**: 0
- **Function Calls (BL)**: 134

- **First Instruction**: #0 at 0x0000011C (THUMB) - `movs r0, #0`
- **Last Instruction**: #10331 at 0x000013C0 (THUMB) - `bx r2`

## Mode Switches

No mode switches detected.

## Functional Sections Executed

### Memory Clear Setup

**Address Range**: 0x00000118 - 0x0000011F

**Description**: Prepare to clear memory

**Instructions Executed**: 2

**Execution Window**: Instructions #0 to #1

**First Instructions**:
```
[    0] 0x0000011C (THUMB): movs     r0, #0
[    1] 0x0000011E (THUMB): ldr      r1, [pc, #0x160]
```

### Memory Clear Loop

**Address Range**: 0x00000120 - 0x00000127

**Description**: Clear IWRAM/EWRAM

**Instructions Executed**: 385

**Execution Window**: Instructions #2 to #386

**First Instructions**:
```
[    2] 0x00000120 (THUMB): str      r0, [r4, r1]
[    3] 0x00000122 (THUMB): adds     r1, r1, #4
[    4] 0x00000124 (THUMB): blt      #0x120
[    5] 0x00000120 (THUMB): str      r0, [r4, r1]
[    6] 0x00000122 (THUMB): adds     r1, r1, #4
```

... (375 instructions omitted) ...

**Last Instructions**:
```
[  382] 0x00000124 (THUMB): blt      #0x120
[  383] 0x00000120 (THUMB): str      r0, [r4, r1]
[  384] 0x00000122 (THUMB): adds     r1, r1, #4
[  385] 0x00000124 (THUMB): blt      #0x120
[  386] 0x00000126 (THUMB): bx       lr
```

**Memory Writes**: 128 store instructions

✅ **Status**: Loop appears to have completed (ended with `bx`)

### VBlank/HBlank Handler

**Address Range**: 0x00000800 - 0x0000082F

**Description**: Wait for VBlank or HBlank

**Instructions Executed**: 8,973

**Execution Window**: Instructions #584 to #9556

**First Instructions**:
```
[  584] 0x00000800 (THUMB): movs     r1, #2
[  585] 0x00000802 (THUMB): lsls     r1, r1, #8
[  586] 0x00000804 (THUMB): mov      ip, r1
[  587] 0x00000806 (THUMB): ldr      r3, [pc, #0x2cc]
[  588] 0x00000808 (THUMB): ldrh     r2, [r3]
```

... (8963 instructions omitted) ...

**Last Instructions**:
```
[ 9552] 0x00000810 (THUMB): cmp      r0, #0
[ 9553] 0x00000812 (THUMB): beq      #0x81c
[ 9554] 0x00000814 (THUMB): cmp      r1, ip
[ 9555] 0x00000816 (THUMB): bge      #0x82c
[ 9556] 0x0000082C (THUMB): bx       lr
```

**Loop Iterations**: 769 backward branches

### CRC/Checksum Function

**Address Range**: 0x000006B2 - 0x000006C7

**Description**: Calculate CRC or checksum

**Instructions Executed**: 595

**Execution Window**: Instructions #9667 to #10261

**First Instructions**:
```
[ 9667] 0x000006B2 (THUMB): ldrb     r2, [r0]
[ 9668] 0x000006B4 (THUMB): rors     r3, r4
[ 9669] 0x000006B6 (THUMB): movs     r5, #4
[ 9670] 0x000006B8 (THUMB): eors     r3, r2
[ 9671] 0x000006BA (THUMB): lsls     r2, r2, #8
```

... (585 instructions omitted) ...

**Last Instructions**:
```
[10257] 0x000006BE (THUMB): bgt      #0x6b8
[10258] 0x000006C0 (THUMB): adds     r0, r0, #1
[10259] 0x000006C2 (THUMB): subs     r1, r1, #1
[10260] 0x000006C4 (THUMB): bgt      #0x6b2
[10261] 0x000006C6 (THUMB): adds     r0, r3, #0
```

### Decompression Routines

**Address Range**: 0x000009C2 - 0x00000B9F

**Description**: Various decompression functions

**Instructions Executed**: 233

**Execution Window**: Instructions #398 to #9656

**First Instructions**:
```
[  398] 0x000009C2 (THUMB): push     {r4, r5, r6, r7, lr}
[  399] 0x000009C4 (THUMB): sub      sp, #4
[  400] 0x000009C6 (THUMB): adds     r7, r0, #0
[  401] 0x000009C8 (THUMB): ldr      r5, [pc, #0x170]
[  402] 0x000009CA (THUMB): movs     r4, #4
```

... (223 instructions omitted) ...

**Last Instructions**:
```
[ 9652] 0x00000B8A (THUMB): cmp      r5, r4
[ 9653] 0x00000B8C (THUMB): bge      #0xb96
[ 9654] 0x00000B96 (THUMB): pop      {r4, r5}
[ 9655] 0x00000B98 (THUMB): pop      {r3}
[ 9656] 0x00000B9A (THUMB): bx       r3
```

### THUMB Initialization

**Address Range**: 0x00001928 - 0x00001FFF

**Description**: Main THUMB mode initialization

**Instructions Executed**: 23

**Execution Window**: Instructions #387 to #9564

**First Instructions**:
```
[  387] 0x00001928 (THUMB): push     {r4, r5, r6, r7, lr}
[  388] 0x0000192A (THUMB): sub      sp, #0x34
[  389] 0x0000192C (THUMB): movs     r1, #0
[  390] 0x0000192E (THUMB): movs     r0, #0
[  391] 0x00001930 (THUMB): str      r0, [sp, #0x14]
```

... (13 instructions omitted) ...

**Last Instructions**:
```
[ 9560] 0x00001954 (THUMB): strh     r5, [r6]
[ 9561] 0x00001956 (THUMB): strh     r0, [r1, #4]
[ 9562] 0x00001958 (THUMB): ldrh     r0, [r6, #4]
[ 9563] 0x0000195A (THUMB): lsrs     r0, r0, #0xf
[ 9564] 0x0000195C (THUMB): beq      #0x1962
```

### Function Epilog

**Address Range**: 0x000013C0 - 0x000013C7

**Description**: Return from function

**Instructions Executed**: 1

**Execution Window**: Instructions #10331 to #10331

**First Instructions**:
```
[10331] 0x000013C0 (THUMB): bx       r2
```

## Critical Analysis

❌ **ROM Entry Point Loader (0xB4-0xBB)**: NOT EXECUTED

⚠️ **CRITICAL**: The BIOS never reached the code that loads the ROM entry point!

### Last Instruction Analysis

**PC**: 0x000013C0

**Instruction**: `bx r2`

**Type**: Branch and Exchange (switching modes and/or jumping)

**Target Register**: R2 = 0x1A000004

## Recommendations

1. **CRITICAL**: The BIOS is not reaching address 0xB4 where it should load the ROM entry point
2. Investigate why BIOS execution is taking a different path
3. Check if BIOS is getting stuck in a loop or calling the wrong function

