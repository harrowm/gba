# I/O Register Write Issue Analysis

## Summary
**The I/O registers (0x04000000-0x040003FF) ARE properly mapped in the memory system**, but there's a critical issue with how the BIOS is trying to write to them using STMIA (Store Multiple Increment After).

## Memory Map Status ✅

The I/O region is correctly allocated and mapped:

```cpp
// Line 88 in memory.cpp constructor:
io = (uint8_t*)std::malloc(1 * 1024);
memset(io, 0, 1 * 1024);  // Zero-initialize IO memory
regionTable[0x04000000 / BLOCK_SIZE] = io;
```

The `regionTable` properly maps 0x04000000 to the `io` buffer, so reads and writes to I/O addresses work correctly through the normal memory access path.

## The Stuck Loop

### BIOS Code at 0xC04:
```assembly
0xC04: CMP r1, r10           ; Compare dest pointer with end
0xC08: STMIA r1!, {r2-r9}    ; Store Multiple (8 registers × 4 bytes = 32 bytes)
0xC0C: BLT 0xC04             ; Branch back if r1 < r10
```

### Register State:
```
r1 = 0x04000200  (Destination: IE register area)
r10 = 0x04000220 (End address)
r2-r9 = 0x00000000 (Values to write: all zeros)
```

### What the BIOS is Trying to Do:
The BIOS is **clearing I/O registers** from 0x04000200 to 0x04000220:
- 0x04000200 = IE (Interrupt Enable) - 16-bit
- 0x04000202 = IF (Interrupt Flags) - 16-bit
- 0x04000204 = WAITCNT - 16-bit
- 0x04000208 = IME (Interrupt Master Enable) - 16-bit
- etc.

This is part of the BIOS initialization sequence to ensure all interrupt-related registers start at 0.

## The Problem: STMIA Not Writing

### STMIA Instruction Behavior:
```
STMIA r1!, {r2, r3, r4, r5, r6, r7, r8, r9}
```

This should:
1. Write r2 to [r1+0]
2. Write r3 to [r1+4]
3. Write r4 to [r1+8]
4. Write r5 to [r1+12]
5. Write r6 to [r1+16]
6. Write r7 to [r1+20]
7. Write r8 to [r1+24]
8. Write r9 to [r1+28]
9. Increment r1 by 32 (r1! means write-back)

### Current Behavior:
Looking at the trace:
1. First iteration: r1 = 0x04000200
2. Second iteration: r1 = 0x04000220 (correctly incremented by 32)
3. **BUT**: The comparison shows r1 == r10 after first iteration, so loop exits
4. **HOWEVER**: The trace shows we're stuck in an infinite loop at 0xC04

### The Bug Pattern:

The debug output shows:
```
PC=0x00000C04, R0=0x03007EA0, R12=0x0301FEA0, 
op1=0x06010960, shifted=0x06018000, result=0xFFFF8960
```

Wait - those register values (r1 = 0x06010960) are **completely different** from what we saw earlier (r1 = 0x04000200)!

## Two Different Loops!

There are actually **TWO different loops** at PC 0xC04:

### Loop 1: I/O Register Clearing (Works Correctly)
```
r1 = 0x04000200 → 0x04000220 (increments by 0x20)
r10 = 0x04000220
Comparison: r1 < r10? NO → Exit loop ✅
```

### Loop 2: VRAM Clearing (Stuck Forever)
```
r1 = 0x06010960 → 0x06010980 → 0x060109A0... (increments by 0x20)
r10 << 8 = 0x06018000 (target)
Comparison: r1 < r10? YES → Continue loop ♾️
```

The BIOS **successfully clears I/O registers**, then moves on to clearing VRAM. But the VRAM clearing loop gets stuck!

## Root Cause: STMIA to VRAM Not Working

### Why is r1 incrementing but never reaching r10?

Looking at the CMP debug output:
```
op1=0x06010960, shifted=0x06018000, result=0xFFFF8960
CPSR FLAGS: N=1 Z=0 C=0 V=0
```

The subtraction: 0x06010960 - 0x06018000 = 0xFFFF8960 (negative)

**r1 starts at 0x06010960 and should reach 0x06018000**, which is a difference of:
```
0x06018000 - 0x06010960 = 0x76A0 = 30,368 bytes
```

With increments of 0x20 (32 bytes) per iteration:
```
30,368 / 32 = 949 iterations needed
```

## The Real Issue: STMIA Not Incrementing r1!

If STMIA were working correctly, r1 should increment by 32 each time. The trace shows r1 **is** changing between iterations (0x06010960 → 0x06010980), so the write-back is working.

**BUT** - looking at the iteration count: we executed ~929K instructions in 5 seconds. If each loop iteration is 3 instructions (CMP, STMIA, BLT), that's ~300K loop iterations. At 32 bytes per iteration, we should have written **9.6 MB** to VRAM, which only has 96KB!

### The Smoking Gun:

The loop should have completed in 949 iterations (less than 3K instructions), but we've executed 300K+ loop iterations. This means **r1 is NOT actually incrementing**, or it's wrapping around somehow.

## Hypothesis: STMIA Write-Back Bug

The STMIA instruction has two components:
1. **Store the registers** to memory
2. **Write back** the incremented address to r1

One of these is failing:

### Possibility 1: STMIA Not Writing to Memory
If STMIA isn't actually writing to VRAM, then subsequent reads might return unexpected values that affect the loop.

### Possibility 2: STMIA Write-Back Not Working for I/O Space
The `!` suffix on `STMIA r1!` means write-back. If this isn't working when r1 points to VRAM (0x06000000+), then r1 would stay constant, causing an infinite loop.

### Possibility 3: VRAM Mirroring Issue
The trace shows r1 = 0x06010960, which is in the **second 64KB block of VRAM** (0x06010000-0x0601FFFF). Our memory map shows:
```cpp
regionTable[0x06010000 / BLOCK_SIZE] = vram + 64 * 1024;
```

This maps 0x06010000-0x0601FFFF to the second half of the 96KB VRAM buffer. **But we only allocated 96KB total!** So 0x06010000+ maps to bytes 64K-96K of the VRAM buffer.

The address 0x06010960 would map to:
```
vram + 64KB + 0x960 = vram + 66,912 bytes
```

Since we only allocated 96KB (98,304 bytes), this is still within bounds. But if r1 increments beyond 0x06018000 (vram + 98,304), we'd be writing **past the end of the VRAM buffer**, causing undefined behavior or a crash!

## Conclusion

The infinite loop is NOT caused by I/O registers being unmapped. The I/O register clearing works fine. The issue is in the **VRAM clearing loop** that happens later.

**Most Likely Causes:**
1. STMIA instruction not properly handling write-back when writing to VRAM
2. VRAM address calculation causing out-of-bounds access
3. STMIA implementation has a bug with the `!` (write-back) modifier

**Why the Bus Error:**
After ~949 iterations, r1 would reach 0x06018000 + (excess iterations × 32), which could exceed the 96KB VRAM allocation, causing a **segmentation fault (bus error)** when trying to write past the buffer!

## Recommended Fix

Check the ARM instruction implementation for **STMIA with write-back** (`STM<mode>! <Rn>, <register_list>`). Specifically:
1. Verify write-back happens correctly
2. Verify it works for all memory regions (not just RAM)
3. Add bounds checking for VRAM writes to prevent buffer overflow
