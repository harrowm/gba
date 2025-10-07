# POSTFLG Boot Flag Fix

## Problem
Sonic (and other commercial ROMs) were stuck in an infinite BIOS reboot loop, continuously re-initializing hardware on every frame instead of progressing into the game.

## Root Cause
The GBA BIOS checks POSTFLG (register at 0x04000300) during the reset vector:
- If POSTFLG = 0x00 → Cold boot path (full initialization)
- If POSTFLG = 0x01 → Warm boot path (skip initialization, jump to ROM)

**Critical Discovery**: After analyzing the BIOS binary with Capstone disassembler, **the BIOS itself does NOT write to POSTFLG**. This contradicts the GBATEK documentation which states "the GBA BIOS initializes the register to 01h".

### BIOS Binary Analysis Results
Searched entire BIOS for STRB/STR instructions writing to offset 0x300:
- Found writes to **0x301 (HALTCNT)** at addresses 0x01B0 and 0x0344
- Found **NO writes to 0x300 (POSTFLG)**

The BIOS only:
1. **Reads** POSTFLG at 0x74 (checks if warm vs cold boot)
2. Never writes POSTFLG

## Solution
The **hardware/emulator** must automatically set POSTFLG=0x01 after the first successful boot, not the BIOS software.

Implemented in `src/gba.cpp:runFrame()`:
- Detect when BIOS memory clear loop (0x120-0x126) completes for the first time
- Immediately set POSTFLG=0x01 via `memory.write8(0x04000300, 0x01)`
- This prevents the BIOS from re-initializing on subsequent reset vector entries

### Code Location
```cpp
// In GBA::runFrame()
static bool postflg_set = false;
static bool in_bios_loop = false;

// Detect entry/exit from memory clear loop
if (!postflg_set && pc >= 0x120 && pc <= 0x126) {
    in_bios_loop = true;
}

if (!postflg_set && in_bios_loop && (pc < 0x120 || pc > 0x126)) {
    memory.write8(0x04000300, 0x01);
    postflg_set = true;
    in_bios_loop = false;
}
```

## Test Results

### Before Fix
```
[Memory::read8] POSTFLG read: 0x00
[LOOP ENTRY #1] Called from LR=0x000000A0
[LOOP EXIT #1] After 385 iterations
[Memory::read8] POSTFLG read: 0x00  ← Still 0!
[LOOP ENTRY #2] Called from LR=0x000000A0
[LOOP EXIT #2] After 385 iterations
[Memory::read8] POSTFLG read: 0x00  ← Still 0!
[LOOP ENTRY #3] Called from LR=0x000000A0
...
(Infinite loop)
```

### After Fix
```
[Memory::read8] POSTFLG read: 0x00
[LOOP ENTRY #1] Called from LR=0x000000A0
[LOOP EXIT #1] After 385 iterations
[Memory::write8] POSTFLG write: 0x01 (was 0x00) ← AUTO-SET!
[POSTFLG] Auto-set to 0x01 after memory clear loop completion
[Memory::read8] POSTFLG read: 0x01  ← Now reads as 1!
...
(Normal execution, frames progressing: 1, 2, 3, 4...)
```

## Impact
- ✅ Sonic now boots successfully and progresses through frames
- ✅ BIOS no longer re-initializes hardware every frame
- ✅ ROM code can execute normally after BIOS handoff
- ✅ Applies to all commercial ROMs that rely on proper POSTFLG behavior

## References
- GBATEK: "4000300h - POSTFLG - BYTE - Undocumented - Post Boot / Debug Control (R/W)"
- BIOS disassembly: Reset vector at 0x68-0xDC
- Memory clear loop: 0x120-0x126 (Thumb mode, 385 iterations)
- POSTFLG check: 0x74 `ldrb r12, [r12, #0x300]` followed by 0x78 `teq r12, #1`

## Date
October 7, 2025
