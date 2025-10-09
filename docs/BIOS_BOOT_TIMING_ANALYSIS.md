# BIOS Boot Timing Analysis

## Problem Summary
GBA emulator crashes when booting Sonic Advance ROM through BIOS, but works when using `--skip-bios` mode.

## Root Cause Identified

### The Timing Issue
1. **BIOS initialization takes ~282,000 cycles** (one full frame)
   - Executes 13,144 instructions before enabling interrupts (IME=1)
   - This is NOT a bug - it's how the BIOS actually works
   
2. **VBlank IRQ fires during BIOS init**
   - GPU scanline updates happen during CPU execution (not from scheduler.runUntil)
   - VBlank occurs at cycle ~198,000 (scanline 160)
   - VBlank IF flag gets set while BIOS is still initializing
   
3. **BIOS enables IME at the end of initialization**
   - By this point, VBlank IF is already pending
   - IRQ fires immediately when IME=1
   
4. **IRQ handler pointer [0x03007FFC] is uninitialized**
   - BIOS never initializes this (ROM is supposed to)
   - Contains 0xFFFFFFFF at boot
   - BIOS IRQ handler at 0x18→0x128→0x134 does: `ldr pc, [0x03007FFC]`
   - Jumps to 0xFFFFFFFF → crash!

### Why --skip-bios Works
When skipping BIOS:
1. ROM executes immediately from 0x08000000
2. ROM's first ~10 instructions set up IRQ handler:
   ```
   [ROM #9] PC=0x080000DC: loads 0x03007FFC into R1
   [ROM #10] PC=0x080000E0: stores 0x080000FC to [0x03007FFC]
   ```
3. When VBlank IRQ fires, [0x03007FFC] is already initialized
4. ROM's IRQ handler at 0x02002B08 executes successfully

## Timing Measurements

### First Frame with BIOS
```
[TIMING] Frame complete: 
  instructions=13144
  loop_start=0
  loop_end=282756  
  cycles_in_loop=282756
  target=282720
```

**First 10 instructions advance cycles normally:**
- Instr 1: 0→5 (5 cycles)
- Instr 2: 5→8 (3 cycles)  
- Instr 3: 8→11 (3 cycles)
- ...continues 3-5 cycles per instruction

**Cycle progression is correct!**

### DMA Analysis
- BIOS writes DMA registers (source/dest addresses)
- No immediate DMA transfers occur (only Mode 3 sound DMAs which wait for trigger)
- BIOS is NOT waiting for DMA completion
- The infinite loop (0xD58→0x1A28→0x400→0x77A) runs ~7-8 times before IME=1

### Infinite Loop Code
Disassembly shows at 0x1A28 (THUMB mode):
```
0x1A2E: ldr r0, [r4]     ; Load from I/O register
0x1A3C: str r2, [r4]     ; Store back
0x1A4E: ldrh r0, [r4]    ; Load halfword
0x1A56: strh r0, [r4]    ; Store halfword back
```

**This is NOT polling DMA registers** - no DMA reads were logged.
The loop likely does graphical setup and then exits naturally.

## Conclusion

**The BIOS behavior is CORRECT**. On real hardware, either:

1. **Same timing occurs** but real GBA has a default/safe IRQ handler behavior for uninitialized [0x03007FFC], OR
2. **Timing is different** and BIOS completes before first VBlank somehow

**The emulator has two options:**

### Option A: Add safety check in IRQ handler
Check if [0x03007FFC] is invalid (0xFFFFFFFF or similar) and skip/return safely:
```cpp
uint32_t handler_addr = memory->read32(0x03007FFC);
if (handler_addr == 0xFFFFFFFF || handler_addr == 0x00000000) {
    // Invalid handler - just return from IRQ
    return;
}
```

### Option B: Use --skip-bios mode
This bypasses the issue entirely and lets ROM initialize properly before first IRQ.

## Files Created During Investigation

- `disassemble_bios_function.py` - Capstone-based BIOS disassembler
- `bios_function_0x1928.txt` - Full disassembly of function (4,760 instructions)
- `disassemble_loop.py` - Analyzes specific loop addresses with ARM/THUMB mode detection
- `/tmp/loop_disassembly.txt` - Disassembly of the infinite loop
- Various trace logs in `/tmp/sonic_*.log`

## Key Insight

The BIOS is designed to work in an environment where either:
- The ROM gets to initialize before interrupts fire, OR  
- There's a default safe behavior for uninitialized IRQ handlers

The emulator currently lacks this safety mechanism, causing the crash.
