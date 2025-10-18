# Sonic ROM Boot Analysis Report
**Date:** October 16, 2025  
**ROM:** assets/roms/sonic.bin  
**Test Duration:** 5 seconds (timeout)  
**Result:** ❌ **FAILED** - Emulator stuck in infinite loop, never reached ROM code, crashed with bus error

---

## Executive Summary

The GBA emulator successfully loaded the Sonic ROM and began executing BIOS code, but became trapped in an infinite loop at BIOS address **0xC04** and never transitioned to ROM code at 0x08000000. After approximately 5 seconds, the emulator crashed with a **bus error** (segmentation fault).

---

## Execution Analysis

### Initial Boot Sequence ✅
- **Starting PC:** 0x00000000 (BIOS entry point)
- **BIOS Loaded:** Successfully from `assets/bios.bin`
- **ROM Loaded:** Successfully from `assets/roms/sonic.bin`
- **Initial Execution:** Normal BIOS initialization sequence

### Execution Statistics
- **Total Instructions Traced:** ~929,559 instructions
- **Execution Time:** ~5 seconds
- **ROM Code Reached:** ❌ **NO** - Never executed any code from 0x08000000+
- **Final PC:** Stuck looping between 0xC04, 0xC08, 0xC0C

### Unique Program Counter Addresses Executed
```
BIOS addresses only (0x00000000 - 0x00003FFF):
0x00000000 - Initial reset vector
0x00000068-0x000000B0 - BIOS initialization
0x000000E0-0x00000C0C - Various BIOS functions
```

**Notable:** No addresses in ROM space (0x08000000+) were ever executed.

---

## Infinite Loop Analysis

### Loop Location
**PC: 0x00000C04** - ARM mode instruction in BIOS

### Loop Pattern
The emulator was repeatedly executing the same instruction at 0xC04:
```
CMP r1, r10, lsl #8
```

**Instruction Breakdown:**
- **Opcode:** `0xE151000A`
- **Format:** CMP with register shift
- **Operation:** Compare r1 with (r10 << 8)
- **Operands:**
  - r1 (op1): 0x06010960, 0x06010980, 0x06010

9A0... (incrementing)
  - r10 (shifted): 0x06018000 (constant)
  - Result: Always negative (0xFFFF8xxx)

### CPU State During Loop
```
PC: 0x00000C04 (ARM mode)
CPSR: 0x8000005F (System mode, N=1, Z=0, C=0, V=0)
r0:  0x03007EA0 (IWRAM address)
r12: 0x0301FEA0 (IWRAM address)
r1:  0x06010960 → 0x06010980 → 0x060109A0... (incrementing by 0x20)
r10: [constant when shifted = 0x06018000]
```

### Loop Condition
The CMP instruction is checking if r1 < (r10 << 8):
- r1 starts at ~0x06010960
- r10 << 8 = 0x06018000
- Comparison result: r1 - 0x06018000 = negative (N flag set)
- Loop continues indefinitely because r1 never reaches 0x06018000

**Problem:** r1 is incrementing by 0x20 each iteration, but the loop appears to have no exit condition or is waiting for an event that never occurs (interrupt, DMA, timer, etc.).

---

## Crash Details

### Error Type
```
zsh: bus error
```

### Likely Causes
1. **Invalid Memory Access:** The loop may have eventually accessed an unmapped memory region
2. **Stack Overflow:** Deep function calls or recursive loop may have corrupted the stack
3. **Unimplemented Hardware Feature:** BIOS waiting for hardware event (DMA, interrupt) that never fires
4. **Memory Corruption:** Loop overwrote critical BIOS data or stack

### Debug Output Before Crash
```
[CMP REG DEBUG] PC=0x00000C04
[CMP OPERANDS] op1=0x06011140, shifted=0x06018000, result=0xFFFF9140
[CPSR FLAGS] N=1 Z=0 C=0 V=0
[DMA0-3] All disabled (Control=0x0000)
```

---

## Root Cause Analysis

### Why BIOS Never Jumped to ROM

The BIOS typically performs these steps before jumping to ROM:
1. ✅ **Initialize hardware** (I/O registers, timers, DMA)
2. ✅ **Verify ROM header** (Nintendo logo, checksum)
3. ❌ **Setup interrupts and stack** ← **LIKELY STUCK HERE**
4. ❌ **Jump to ROM entry point** (0x08000000)

### Suspected Issues

#### 1. **Interrupt/DMA Not Working** (Most Likely)
The loop at 0xC04 appears to be waiting for a hardware event:
- BIOS may be waiting for VBlank interrupt
- DMA transfer may not be completing
- Timer may not be firing
- Our emulator's interrupt/DMA implementation may have bugs

#### 2. **ROM Header Validation Failed**
BIOS might have failed to validate Sonic's ROM header and got stuck in an error handler.

#### 3. **Missing BIOS Features**
The BIOS may be calling SWI (software interrupt) functions that aren't implemented in our emulator.

#### 4. **Incorrect Memory Mirroring**
GBA has complex memory mirroring rules. BIOS might be accessing mirrored regions that aren't properly handled.

---

## Comparison with Working State

### What Works ✅
- BIOS loads correctly
- ROM loads correctly  
- Initial BIOS execution (0x00-0x0C0C)
- Instruction decoding and execution
- Basic memory access (BIOS, IWRAM)
- Register updates
- CPSR flag calculations

### What Doesn't Work ❌
- BIOS never completes initialization
- Never jumps to ROM entry point (0x08000000)
- Gets stuck in infinite loop at 0xC04
- Eventually crashes with bus error
- No interrupts firing (VBlank, HBlank, Timer)
- No DMA transfers completing

---

## Recommendations

### Immediate Actions

1. **Check Interrupt Implementation**
   ```
   File: src/interrupt.cpp
   - Verify VBlank interrupt generation
   - Check interrupt enable/master enable flags
   - Ensure IF register updates correctly
   ```

2. **Verify DMA Implementation**
   ```
   File: src/dma.cpp
   - Check DMA enable/control register handling
   - Verify DMA transfers complete
   - Ensure DMA interrupts fire when enabled
   ```

3. **Examine 0xC04 Instruction Context**
   ```bash
   # Disassemble BIOS around 0xC04
   arm-none-eabi-objdump -D -b binary -m arm assets/bios.bin \
     --start-address=0xC00 --stop-address=0xC20
   ```

4. **Add Debug Logging for:**
   - Interrupt triggers (VBlank, HBlank, Timer)
   - DMA start/complete events
   - SWI (software interrupt) calls
   - Memory access to 0x04000000+ (I/O registers)

### Testing Strategy

1. **Compare with mGBA**
   Run same ROM in mGBA and capture:
   - When BIOS jumps to ROM (instruction count)
   - What interrupts fire during BIOS
   - What I/O registers are accessed

2. **Try Simpler ROM**
   Test with a minimal homebrew ROM that doesn't require full BIOS init:
   ```bash
   ./gba_emulator --skip-bios assets/roms/hello.gba
   ```

3. **Enable Full BIOS Tracing**
   ```bash
   ./gba_emulator --trace-bios assets/roms/sonic.bin 2>&1 | tee bios_trace.log
   ```

---

## Technical Details

### Memory Regions Accessed
- **0x00000000-0x00003FFF:** BIOS (read)
- **0x03000000-0x03007FFF:** IWRAM (read/write)
- **0x04000000-0x040003FF:** I/O Registers (read/write)
- **0x06000000-0x06017FFF:** VRAM (attempted access?)
- **0x08000000+:** ROM (❌ never accessed)

### CPU Mode History
```
Start: System mode (0x1F)
Stuck: System mode (0x1F) at PC=0xC04
```

### Register Evolution in Loop
```
Iteration 1: r1=0x06010960
Iteration 2: r1=0x06010980 (+0x20)
Iteration 3: r1=0x060109A0 (+0x20)
...continuing indefinitely...
```

---

## Conclusion

The emulator successfully loads and begins executing BIOS code but fails to complete BIOS initialization. The execution becomes trapped in an infinite loop at address 0xC04, likely waiting for a hardware interrupt or DMA event that never occurs. This suggests **missing or incomplete interrupt/DMA emulation** is the primary blocker preventing ROM code from executing.

**Next Steps:**
1. Fix interrupt generation (VBlank is critical)
2. Verify DMA implementation
3. Add detailed I/O register logging
4. Compare execution trace with mGBA to find divergence point

**Estimated Fix Complexity:** Medium - Core emulation logic works, but peripheral/hardware emulation needs attention.
