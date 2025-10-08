# GBA BIOS Execution Complete Analysis

## Summary

After analyzing 10,332 instructions of BIOS execution, we can confirm:

✅ **BIOS boots correctly** from ARM mode at 0x00000000  
✅ **Mode switching works** - BX instruction at 0x118 switches from ARM to THUMB  
✅ **Memory clear loop completes** - 128 stores executed, loop exits cleanly with `bx lr`  
✅ **BIOS functions execute** - VBlank handler, CRC/checksum, decompression routines all work  

❌ **CRITICAL ISSUE**: BIOS never reaches address 0xB4 (ROM Entry Point Loader)  
❌ **ROOT CAUSE**: At 0x13C0, BIOS executes `bx r2` where R2=0x1A000004 (invalid address)

## Boot Sequence Flow

### Phase 1: ARM Reset Handler (0x00000000 - 0x00000117)
**Instructions 1-49 | ARM Mode**

1. **0x00000000**: `b #0x68` - Branch to reset handler
2. **0x00000068-0x000000AC**: CPU mode setup, stack initialization
   - Sets up IRQ, SVC, and SYS mode stacks
   - Reads POSTFLG at 0x04000300 to check if this is a soft reset
3. **0x00000118**: `bx r0` - Branch to THUMB mode at 0x11C with LR=0xA0

### Phase 2: THUMB Memory Clear (0x0000011C - 0x00000127)
**Instructions 50-436 | THUMB Mode | ✅ COMPLETED**

- **Loop at 0x120-0x124**: Clears memory by storing 0 to addresses
- **Total stores**: 128 (clearing 512 bytes)
- **Exit**: `bx lr` returns to ARM 0xA0

### Phase 3: More Initialization (0x00001928+)
**Instructions 437-10,331 | THUMB Mode**

After returning from memory clear, BIOS continues with more initialization:
- Calls decompression routines (0x9C2-0xB9F)
- Calls VBlank/HBlank wait function (0x800-0x82F) - **executes 8,973 instructions!**
- Calls CRC/checksum function (0x6B2-0x6C7)
- More initialization at 0x1928+

### Phase 4: Fatal Error (0x000013C0)
**Instruction 10,331 | THUMB Mode | ❌ FAILURE**

- **PC**: 0x000013C0
- **Instruction**: `bx r2`
- **R2 value**: 0x1A000004 (INVALID ADDRESS)
- **Result**: Jumps to unmapped memory, emulator crashes

## The Missing Step

The BIOS should follow this path to reach ROM:

```
0x00000000 (ARM reset)
    ↓
0x00000068 (ARM initialization)
    ↓
0x00000118 (BX to THUMB 0x11C)
    ↓
0x0000011C (THUMB memory clear)
    ↓
0x000000A0 (ARM - return from memory clear)
    ↓
**0x000000B4 (ARM - SHOULD BE HERE!)** ← WE NEVER REACH THIS
    ↓
Load ROM entry point from header at 0x080000AC
    ↓
Jump to ROM code
```

**Instead, the actual path is:**

```
0x00000118 (BX to THUMB 0x11C)
    ↓
0x0000011C (THUMB memory clear)
    ↓  
0x000000A0 (return)
    ↓
??? (some other BIOS code path)
    ↓
0x00001928 (THUMB initialization)
    ↓
0x00000800 (VBlank wait - loops 256 times!)
    ↓
0x000006B2 (CRC/checksum)
    ↓
0x0000164C (loads from uninitialized memory)
    ↓
0x000013C0 (BX to invalid address 0x1A000004)
```

## Key Findings

### 1. VBlank Handler Dominates Execution
- **8,973 of 10,332 instructions** (87%) spent in VBlank/HBlank handler at 0x800-0x82F
- Loop executed **256 times** waiting for scanline to reach 512 (0x200)
- This is normal BIOS behavior, but suggests BIOS is waiting for display timing

### 2. Memory at 0x03007FF0 is Uninitialized
The fatal instruction sequence:
```
0x001646: ldr r2, [pc, #0x18]      → R2 = 0x03007FC0
0x00164A: ldr r2, [r2, #0x30]      → R2 = memory[0x03007FF0] = 0x00000000  
0x00164C: ldr r2, [r2, #0x3c]      → R2 = memory[0x0000003C] = 0x1A000004
0x0013C0: bx r2                    → Jump to 0x1A000004 (CRASH!)
```

Memory at 0x03007FF0 should contain a pointer to a data structure, but it's 0x00000000.
When BIOS reads from 0x00000000 + 0x3C = 0x0000003C, it reads BIOS ROM data (0x1A000004),
which is NOT a valid address - it's just instruction encoding bytes.

### 3. BIOS Function at 0x164E Calls 0x13C0
A BL (branch with link) instruction exists at 0x164E:
```
[BL-PART1] PC=0x0000164E: BL prefix, LR_temp=0x00000652
[BL-PART2] PC=0x00001650: BL suffix, target=0x000013C0, LR=0x00001653
```

This BL instruction branches to 0x13C0, which is a function that does `bx r2`.
This is likely a function pointer dispatch mechanism.

## Why We Never Reach 0xB4

The BIOS has multiple boot paths:
1. **Cold boot** (POSTFLG=0): Full initialization → 0xB4 → ROM
2. **Soft reset** (POSTFLG=1): Skip some init → jump somewhere else

The issue is that after the memory clear completes and returns to 0xA0, instead of
continuing to 0xB4, the code branches somewhere else (likely 0x1928 based on our trace).

This suggests either:
- POSTFLG is incorrectly set to 1, causing BIOS to skip ROM loading
- The memory clear return address (LR) is wrong
- There's a conditional branch after 0xA0 that's taking the wrong path

## Recommendations

1. **Check POSTFLG value** - Add tracing to see if POSTFLG at 0x04000300 is being set
2. **Trace ARM instructions 0xA0-0xBF** - See what happens after memory clear returns
3. **Verify LR value** - When memory clear exits with `bx lr`, is LR really 0xA0?
4. **Check conditional branches** - Are there any branches between 0xA0 and 0xB4?
5. **Consider BIOS version** - Different BIOS versions may have different boot paths

## Memory Map Context

**0x03007FF0** is at the TOP of IWRAM, in the 256-byte system reserved area:
- 0x03007FFC: IRQ handler pointer (set by ROM)
- 0x03007FF0: Data structure pointer (set by ROM)
- 0x03007FE0: Supervisor stack
- 0x03007FA0: IRQ stack
- 0x03007F00: System/User stack

This memory SHOULD be initialized by ROM code, but ROM never gets to execute
because BIOS crashes before reaching 0xB4.

## Conclusion

The GBA BIOS implementation in the emulator is mostly correct:
- ✅ ARM/THUMB mode switching works
- ✅ Memory clearing works
- ✅ BIOS functions execute correctly
- ✅ VBlank handling works

The bug is that **the BIOS boot path doesn't reach the ROM entry point loader at 0xB4**.
Instead, it takes an alternative code path that eventually tries to call ROM code before
giving ROM a chance to initialize. This causes the crash when reading uninitialized memory.

**Next Step**: Trace ARM instructions from 0xA0 to 0xBF to see why execution doesn't 
continue to 0xB4 after the memory clear completes.
