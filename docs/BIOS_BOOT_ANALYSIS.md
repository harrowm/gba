# GBA BIOS Boot Sequence Analysis

## Overview
Analysis of GBA BIOS boot sequence while attempting to run commercial ROM (Sonic Advance).

## Current Status: BLOCKED at IRQ Handler
**Last updated**: Investigation in progress

**Problem**: BIOS initialization function at 0x1928 enables interrupts (IE=0x0001, IME=1) before returning to ROM loader at 0x000000B4. When VBlank IRQ fires, the user IRQ handler pointer at 0x03007FFC is uninitialized (0xFFFFFFFF), causing execution to jump to invalid address and crash in infinite IRQ loop.

**Progress**: 
- ✅ Fixed THUMB POP {pc} alignment bug (138x improvement)
- ✅ VBlank wait loop works correctly 
- ✅ Decompression routine executes successfully
- ❌ **BLOCKED**: Function never returns to 0x000000B4 due to IRQ loop
- ❌ ROM loader at 0x000000B4 never executes
- ❌ Game ROM never loads or runs

**Key Question**: Why does BIOS enable IME during initialization, before ROM has set up the IRQ handler?

## Boot Sequence Discovered

```
0x00000000  → 0x00000068  → 0x00000118  → 0x0000011C → 0x000000A0 → 0x000000A8
    ↓             ↓              ↓              ↓           ↓            ↓
  Reset      Checksum      Init Start      More Init   Continue    Load 0x1929
                                                                     into R0
    ↓
0x000000B0: bx r0  (LR=0x000000B4)  →  0x00001928 (initialization function)
                                            ↓
                                      [BIOS Init: ~170,000 instructions]
                                            ↓
                                       - VBlank wait (0x1774-0x1778) ✓ EXITS
                                       - Decompression (0x1074-0x10DC) ✓ EXECUTES  
                                       - Enables IE=0x0001 (VBlank interrupt) ✓
                                       - Enables IME=1 (Master interrupt enable) ✓
                                            ↓
                                       **IRQ FIRES** ❌ (0x03007FFC uninitialized)
                                            ↓
                                       Jumps to 0xFFFFFFFF ❌ (CRASH)
                                            ↓
                                       **NEVER RETURNS TO 0x000000B4** ❌
                                            ↓
                                       **ROM NEVER LOADS** ❌
```

### Key Address: 0x000000B4 - ROM LOADER (NEVER REACHED)
**From BIOS disassembly:**
```assembly
_000000B4:
    mov r4, #0x4000000
    ldrb r2, [r4, #-6]        ; Read boot flag from 0x03FFFFFA
    bl sub_000000E0           ; Setup stacks/CPU modes
    cmp r2, #0
    ldmdb r4, {r0, r1, r2, r3, r4, r5, r6, r7, r8, sb, sl, r11, r12}
    movne lr, #0x02000000     ; If flag!=0: Jump to EWRAM
    moveq lr, #0x08000000     ; If flag==0: Jump to ROM
    mov r0, #0x1f
    msr cpsr_fc, r0
    mov r0, #0
    bx lr                     ; **JUMP TO ROM AT 0x08000000** ← NEVER EXECUTES
```

**Status**: ❌ NEVER REACHED (confirmed via grep: no PC=0x000000B4 in trace logs)

### Key Address: 0x00001928 - BIOS Initialization Function
- **Entry**: PC=0x00001928, LR=0x000000B4 (return address)
- **Prologue**: `push {r4, r5, r6, r7, lr}` - saves return address
- **Purpose**: Initialize display, timers, interrupts, system state
- **Contains**: 
  - VBlank wait loop (0x1774-0x1778) ✓ Exits successfully
  - Decompression routine (0x1074-0x10DC) ✓ Executes successfully
  - Interrupt setup (enables IE and IME)
- **Expected**: Pop registers and return to 0x000000B4
- **Actual**: Gets stuck in IRQ infinite loop before returning ❌

### Key Address: 0x03007FFC - User IRQ Handler Pointer
- **Purpose**: Points to game's IRQ handler (set by ROM, not BIOS)
- **Expected**: ROM initializes this before enabling interrupts
- **Actual**: Uninitialized (0xFFFFFFFF) when BIOS enables IME
- **Result**: BIOS IRQ handler at 0x00000018 jumps to 0xFFFFFFFF → CRASH

## The VBlank Wait Loop (0x1774-0x1778)

```assembly
0x00001774: ldrb r1, [r0, #6]    ; Read VCOUNT from 0x04000006
0x00001776: cmp  r1, #0x9f       ; Compare with 159 (scanline 159)
0x00001778: bne  #0x1774         ; Loop until VCOUNT == 159
```

### What It Does
- Reads the **VCOUNT register** (0x04000006) - current display scanline
- Waits for scanline **159 (0x9F)** - last visible scanline before VBlank
- This is a **synchronization point** to align BIOS timing with display

### Why It Takes Time
- GBA display: 228 scanlines per frame, 1232 cycles per scanline
- If BIOS reaches this point after scanline 159 has passed, it must wait for the **next frame**
- Frame duration: 280,896 cycles (~16.7ms at 16.78MHz)
- In our trace: VCOUNT was at 185 when loop started, needed to wait ~53 scanlines until next frame

## Critical Bugs Fixed

### Bug #1: THUMB POP {pc} Not Clearing Bit 0
**Location**: `src/thumb_cpu.cpp`, function `thumb_pop_registers_and_pc()`

**Problem**:
```cpp
// WRONG - caused PC misalignment
uint32_t new_pc = parentCPU.getMemory().read32(parentCPU.R()[13]);
parentCPU.R()[15] = new_pc;  // Didn't clear bit 0!
```

**Fix**:
```cpp
// CORRECT - clears bit 0 for THUMB alignment
uint32_t new_pc = parentCPU.getMemory().read32(parentCPU.R()[13]);
parentCPU.R()[15] = new_pc & 0xFFFFFFFE;  // Force even address
```

**Impact**:
- Before fix: 10,332 instructions executed before crash
- After fix: 1,426,888+ instructions (138x improvement)
- Boot now progresses to initialization function at 0x1928

## What NOT To Do (Rejected Approaches)

### ❌ Initialize 0x03007FFC with Default Handler
**Idea**: Set up a default "do nothing" IRQ handler in IWRAM during memory initialization.

**Why rejected**: Real GBA hardware doesn't do this. The ROM is responsible for setting up the IRQ handler, not the emulator or BIOS. Adding fake handlers would mask bugs and create incorrect behavior.

**GBATEK Documentation**:
> "By default, the 256 bytes at 03007F00h-03007FFFh in Work RAM are reserved for
> Interrupt vector, Interrupt Stack, and BIOS Call Stack."

The **ROM** must initialize 0x03007FFC, not the BIOS or emulator.

### ❌ Disable VBlank Interrupts During BIOS
**Idea**: Prevent GPU from generating VBlank interrupts until ROM is loaded.

**Why rejected**: Logs show the BIOS deliberately writes IE=0x0001 to enable VBlank interrupts. The BIOS WANTS interrupts enabled. Disabling them would break BIOS behavior.

### ❌ Fake the VBlank Wait Loop
**Idea**: Make VCOUNT always return 159 to skip waiting.

**Why rejected**: 
1. The VBlank wait IS working correctly - it exits successfully
2. The loop is a **polling loop** reading VCOUNT directly (not IRQ-based)
3. Execution progresses past the wait to decompression and IME enable
4. The problem occurs AFTER the wait, not during it

## Interrupt Registers Analysis

### Register Write Sequence (from logs)
```
[REG Write32] Address=0x04000200 Value=0x00000000  ← Clear IE/IF
[REG Write32] Address=0x04000204 Value=0x00000000  ← Clear more registers
[REG Write32] Address=0x04000208 Value=0x00000000  ← Clear IME
[REG Write] IF acknowledge = 0xFFFF, new IF = 0x0000  ← Clear all interrupt flags
[REG Write] IE (Interrupt Enable) = 0x0001          ← Enable VBlank interrupt
[REG Write] IME (Interrupt Master Enable) = 0x0000  ← Disable master (briefly)
[REG Write] IME (Interrupt Master Enable) = 0x0001  ← Enable master interrupt
[CHECK_IRQ #1] IME=1, IRQ_disabled=0, pending=1 IE=0x0001 IF=0x0001 => IRQ FIRES
```

**Key Finding**: The BIOS deliberately enables VBlank interrupts (IE bit 0) and then enables IME!

### Why the IRQ Fires Immediately
1. **IE=0x0001**: VBlank interrupt enabled
2. **IF=0x0001**: VBlank interrupt pending (GPU generated it)
3. **IME=1**: Master interrupt enable set
4. **Condition met**: `(IE & IF) != 0` → IRQ fires
5. **BIOS IRQ handler** at 0x00000018 executes:
   ```assembly
   push {r0-r3, r12, lr}
   mov r0, #0x04000000
   adr lr, after_interrupt
   ldr pc, [r0, #-4]      ; Load from 0x03FFFFFC and jump
   ```
6. **[0x03FFFFFC] = 0xFFFFFFFF**: Uninitialized!
7. **Jumps to 0xFFFFFFFF**: Invalid address → crash

## Execution Statistics

### Before THUMB POP Fix
- **Instructions executed**: 10,332
- **Reason for halt**: PC misalignment crash
- **Progress**: Couldn't get past early BIOS code

### After THUMB POP Fix
- **Instructions executed**: 1,426,888+ 
- **Improvement**: 138x more instructions
- **Progress**: Reaches initialization function at 0x1928
- **VBlank wait exits**: Instruction 103,438 (first) and 168,311 (second)
- **IME enabled**: Instruction 170,123 at PC=0x00002D60
- **IRQ loop starts**: Shortly after instruction 170,123
- **ROM loader at 0xB4**: NEVER reached
- **ROM at 0x08000000+**: NEVER executed

### Key Milestones
| Instruction | Event | Status |
|------------|-------|--------|
| 35 | Branch to 0x1928 (LR=0xB4) | ✅ |
| 387 | Push {r4,r5,r6,r7,lr} at 0x1928 | ✅ |
| 103,438 | VBlank wait exits (1st time) | ✅ |
| 168,311 | VBlank wait exits (2nd time) | ✅ |
| 170,123 | IME enabled at PC=0x2D60 | ✅ |
| 170,124+ | IRQ fires, jumps to 0xFFFFFFFF | ❌ |
| N/A | POP {r4,r5,r6,r7,pc} returning to 0xB4 | ❌ NOT FOUND |
| N/A | ROM loader at 0x000000B4 | ❌ NEVER REACHED |
| N/A | ROM execution at 0x08000000+ | ❌ NEVER REACHED |

## Current Status & Problem Analysis

### ✅ Working Correctly
- THUMB POP {pc} alignment fixed (138x improvement)
- BIOS executes initialization function at 0x1928  
- VBlank wait loop exits successfully (confirmed in logs)
- Decompression routine at 0x1074-0x10DC executes
- GPU/scheduler integration working properly

### ❌ Current Blocker: IRQ Infinite Loop
**Symptom**: Function at 0x1928 never returns to ROM loader at 0x000000B4

**Root Cause**: BIOS enables interrupts (IE=0x0001, IME=1) during initialization, BEFORE returning to ROM loader. When IRQ fires, user handler at 0x03007FFC is uninitialized (0xFFFFFFFF).

**Evidence**:
- grep "PC=0x000000B4": NO matches after initial branch at 0xB0
- grep "PC=0x08": NO matches (ROM never executes)
- Execution gets stuck jumping to 0xFFFFFFFF repeatedly

### 🔍 Narrowing Down the Error Location

**Working backwards from ROM loader (0x000000B4):**

```
GOAL: ROM loader at 0x000000B4 should execute
  ↑
  REQUIRES: Function at 0x1928 must return (POP {r4,r5,r6,r7,pc} or BX LR)
  ↑
  BLOCKED BY: IRQ fires after IME enabled, never returns
  ↑
  CAUSE: User IRQ handler at 0x03007FFC uninitialized (0xFFFFFFFF)
  ↑
  QUESTION: Why does BIOS enable IME before ROM sets up IRQ handler?
```

**Possible Error Locations (to investigate):**

1. **BIOS enables IME too early?**
   - Maybe BIOS shouldn't enable IME until AFTER returning to 0xB4
   - Or maybe only ROM loader should enable IME
   
2. **BIOS should set up default IRQ handler?**
   - Real hardware might have BIOS initialize 0x03007FFC
   - But GBATEK says ROM is responsible for this
   
3. **Emulator shouldn't generate VBlank IRQ during BIOS?**
   - Maybe IF shouldn't have bit 0 set yet
   - GPU might be triggering VBlank too early
   
4. **IRQ handler should check for invalid pointer?**
   - BIOS IRQ handler could check if [0x03FFFFFC] is valid
   - But real BIOS probably doesn't do this

5. **Function at 0x1928 never reaches its return instruction?**
   - Maybe execution path never gets to the POP that would return
   - IRQ fires before function completes

### 🎯 Next Investigation Steps

1. **Find the POP instruction**: Search for where function at 0x1928 should return
   ```bash
   grep -A500 "PC=0x00001928.*push.*{r4, r5, r6, r7, lr}" trace.log | grep "pop.*{r4, r5, r6, r7, pc}"
   ```

2. **Check if POP ever executes**: See if return instruction is reached before IRQ
   ```bash
   grep "pop.*pc.*LR=000000B4" trace.log
   ```

3. **Examine BIOS code around IME enable**: Find why BIOS enables IME so early
   ```bash
   grep -B20 "REG Write.*IME.*0x0001" trace.log | grep "PC="
   ```

4. **Check real hardware behavior**: Research if real GBA BIOS enables IME during init
   - Check mGBA, VBA-M source code
   - Look for BIOS documentation on interrupt timing

5. **Verify GPU VBlank generation**: Check if VBlank IRQ should be pending
   ```bash
   grep "requestInterrupt.*VBLANK" trace.log
   ```

## Critical Open Question

**Why does the BIOS enable IME (master interrupt enable) during initialization, before the ROM has a chance to set up the user IRQ handler at 0x03007FFC?**

This seems contradictory:
- The BIOS enables VBlank interrupts (IE=0x0001)
- The BIOS enables master interrupts (IME=1)
- The BIOS IRQ handler jumps to user handler at [0x03007FFC]
- **But the ROM hasn't run yet to initialize 0x03007FFC!**

**Possible explanations:**
1. The BIOS is supposed to initialize 0x03007FFC with a default handler (but GBATEK says ROM does this)
2. The BIOS shouldn't enable IME until after jumping to ROM (but logs show it does)
3. Our emulator is generating VBlank IRQs incorrectly during BIOS boot
4. The function at 0x1928 is supposed to return BEFORE IME is enabled (timing issue?)
5. Real hardware has some protection against jumping to 0xFFFFFFFF that we're missing

**What we know for certain:**
- Function at 0x1928 pushes LR=0x000000B4 to stack
- Function should eventually pop and return to 0x000000B4
- ROM loader at 0x000000B4 never executes (confirmed via logs)
- Execution gets stuck in IRQ infinite loop instead

The error must be in ONE of these places:
1. BIOS initialization sequence (enabling IME too early)
2. GPU VBlank interrupt generation (firing when it shouldn't)
3. IRQ handler safety checks (should validate [0x03007FFC] before jumping)
4. Emulator interrupt handling (missing some hardware behavior)

**Next step**: Trace execution from VBlank wait exit to IME enable to find exactly where the control flow should diverge to return to 0xB4.

## CRITICAL FINDING: BIOS Infinite Loop Pattern

### The Loop That Never Returns
Using Capstone to disassemble the entire function from 0x1928 to end of BIOS (0x3FFE):
- **Total instructions**: 4,760
- **Result**: NO matching `pop {r4, r5, r6, r7, pc}` return instruction found!
- **End of disassembly**: Reaches 0x3FFE (end of BIOS) with only NOPs (`movs r0, r0`)

**Conclusion**: The function at 0x1928 **never returns to 0x000000B4 in the traditional sense**.

### The Repeating Pattern Before IRQ
Just before IRQ fires, execution enters an infinite loop:

```
Pattern repeats ~30 times before IRQ:
  0xD58 → BX to 0x1A28 (THUMB mode)
  0x1A28 → (some code)
  0x400 → BX to 0x77A (THUMB mode)  
  0x77A → (some code)
  Back to 0xD58...
```

**This is NOT a bug** - this pattern exists in the real BIOS which works on hardware.

### Theory: BIOS Main Loop Waiting for Event
The BIOS appears to enter a **main loop** that:
1. Enables interrupts (IE=0x0001 for VBlank, IME=1)
2. Enters infinite loop polling or waiting
3. Expects IRQ to fire and be handled
4. After IRQ handling, should exit loop and return to 0xB4

**The problem**: When IRQ fires, the BIOS IRQ handler at 0x00000018 does:
```assembly
push {r0-r3, r12, lr}
mov r0, #0x04000000
adr lr, after_interrupt
ldr pc, [r0, #-4]      ; Jump to [0x03FFFFFC] = user IRQ handler
```

If [0x03007FFC] is uninitialized (0xFFFFFFFF), this crashes.

### Key Questions
1. **Does real hardware initialize 0x03007FFC differently?**
   - Our logs show NO writes to 0x03007FFC by BIOS
   - ROM is supposed to initialize this, but ROM hasn't loaded yet
   
2. **Is the BIOS loop supposed to exit BEFORE IRQ fires?**
   - Loop runs ~30 iterations before IRQ
   - Maybe our timing is wrong and loop should exit earlier?
   
3. **Does real hardware prevent IRQ when [0x03007FFC] is invalid?**
   - Hardware might check pointer validity
   - Or BIOS IRQ handler might have safety check we're missing

4. **Is this loop the actual "ROM loader wait loop"?**
   - Maybe BIOS is waiting for ROM to do something
   - Cartridge detection? DMA completion? Hardware signal?

### What mGBA Does
Need to investigate mGBA source code to see:
- How they handle BIOS boot sequence
- Do they skip/patch this loop?
- Do they initialize 0x03007FFC with a default handler?
- Do they have special timing that exits the loop earlier?

## Trace Analysis Commands

### Verify ROM Loader Never Executes
```bash
# Check if PC ever reaches 0x000000B4 (ROM loader)
grep "PC=0x000000B4" trace.log | head -5
# Result: Only initial branch at 0xB0 found, never the loader itself

# Check if ROM code ever executes
grep "PC=0x08" trace.log | head -10  
# Result: Empty - ROM never loads or executes
```

### Find Function Entry and Return
```bash
# Find function entry at 0x1928
grep "PC=0x00001928" trace.log | head -5
# Result: [387][THUMB-BIOS] PC=0x00001928: push {r4, r5, r6, r7, lr} | LR=000000B4

# Search for corresponding POP that should return
grep -E "pop.*\{r4, r5, r6, r7, pc\}" trace.log | head -20
# Result: Many POP instructions found, but none with LR=000000B4

# Search for any return to 0xB4
grep "LR=000000B4" trace.log | grep -E "(pop|bx)"
# Result: Only the initial entry, no return found
```

### Interrupt Register Analysis
```bash
# Find when IE/IF/IME are written
grep -E "(REG Write.*I[EM]E|CHECK_IRQ)" trace.log | head -30
# Result shows sequence:
# 1. IE=0x0001 (VBlank enabled)
# 2. IME=0x0001 (Master enable)
# 3. IRQ fires immediately (IE & IF = 0x0001)
```

### Conclusion from Traces
1. Function at 0x1928 pushes LR=0x000000B4 to stack
2. Function executes ~170,000 instructions including VBlank wait and decompression
3. BIOS enables IE=0x0001 and IME=1
4. VBlank IRQ fires immediately 
5. **No POP instruction ever executes that would return to 0xB4**
6. ROM loader at 0xB4 never reached
7. ROM at 0x08000000+ never executes

The function at 0x1928 never completes - it gets interrupted by IRQ before reaching its return instruction.

### 🎯 Next Steps
1. Let VBlank wait complete naturally (may take 2-3 frames)
2. Verify return to 0xB4 (ROM loader)
3. Check ROM loading process
4. Confirm jump to game code at 0x08000000+

## Key Learnings

1. **Don't hack around hardware behavior** - If real hardware doesn't initialize something, neither should emulator
2. **VBlank synchronization is critical** - Many games depend on precise display timing
3. **Patience with boot sequences** - Commercial ROMs take time to boot through BIOS
4. **GBATEK is authoritative** - Online documentation search was helpful but reading specs carefully is essential

## Memory Map Reference

```
0x00000000-0x00003FFF  BIOS ROM (16KB)
0x02000000-0x0203FFFF  WRAM (256KB) 
0x03000000-0x03007FFF  IWRAM (32KB)
0x03007F00-0x03007FFF  Reserved for IRQ vector/stack/BIOS stack
0x03007FFC            User IRQ handler pointer (set by ROM, not BIOS)
0x04000000-0x040003FE  I/O Registers
0x04000006            VCOUNT - Current scanline (0-227)
0x04000208            IME - Interrupt Master Enable
0x05000000-0x050003FF  Palette RAM (1KB)
0x06000000-0x06017FFF  VRAM (96KB)
0x07000000-0x070003FF  OAM (1KB)
0x08000000-0x09FFFFFF  Game Pak ROM (up to 32MB)
```

## Execution Statistics

- **Before fix**: 10,332 instructions
- **After fix**: 1,426,888+ instructions (138x improvement)  
- **Improvement factor**: 138x
- **Current state**: Waiting in VBlank loop (expected behavior)
- **Estimated completion**: Next 1-2 frames (~280,000-560,000 cycles)

## mGBA Research: How They Handle BIOS Boot

### Key Finding: mGBA Uses BIOS Skip Mode By Default

**Discovery from source code analysis:**

mGBA has a `GBASkipBIOS()` function (in `src/gba/gba.c` lines 308-332) that **completely bypasses BIOS execution** and directly initializes the system AS IF the BIOS had already run.

**What GBASkipBIOS() does:**
```c
void GBASkipBIOS(struct GBA* gba) {
    struct ARMCore* cpu = gba->cpu;
    if (cpu->gprs[ARM_PC] == BASE_RESET + WORD_SIZE_ARM) {
        if (gba->memory.rom) {
            cpu->gprs[ARM_PC] = GBA_BASE_ROM0;  // Jump directly to 0x08000000 (ROM)
        } else if (gba->memory.wram[0x30]) {
            cpu->gprs[ARM_PC] = GBA_BASE_EWRAM + 0xC0;
        } else {
            cpu->gprs[ARM_PC] = GBA_BASE_EWRAM;
        }
        gba->video.vcount = 0x7E;
        gba->memory.io[GBA_REG(VCOUNT)] = 0x7E;
        gba->memory.io[GBA_REG(POSTFLG)] = 1;
        ARMWritePC(cpu);
    }
}
```

**Critical Observations:**
- **Does NOT initialize 0x03007FFC** (user IRQ handler pointer)
- **Does NOT explicitly set up stack pointers** in this function
- **Simply jumps to ROM start** (0x08000000) or EWRAM
- **Assumes ROM code will initialize everything** including interrupt handlers

### When mGBA DOES Execute Real BIOS

From `src/gba/core.c` lines 829-849:
```c
bool forceSkip = gba->mbVf || (core->opts.skipBios && (gba->romVf || gba->memory.rom));
if (!forceSkip && (gba->romVf || gba->memory.rom) && gba->pristineRomSize >= 0xA0 && gba->biosVf) {
    uint32_t crc = doCrc32(&gba->memory.rom[1], 0x9C);
    if (crc != LOGO_CRC32) {
        mLOG(STATUS, WARN, "Invalid logo, skipping BIOS");
        forceSkip = true;
    }
}

if (forceSkip) {
    GBASkipBIOS(core->board);
}
```

**So mGBA executes real BIOS only when:**
1. User provides BIOS file
2. User disables skip BIOS option
3. ROM has valid Nintendo logo
4. No multiboot mode

### Hypothesis: Real GBA Hardware Behavior

Based on our analysis and mGBA's skip logic, the likely real hardware sequence is:

1. **BIOS runs initialization** at 0x1928
2. **BIOS performs hardware setup**:
   - Decompresses Nintendo logo
   - Initializes hardware registers
   - Sets up stack pointers
   - **Possibly waits for hardware ready signals**
3. **BIOS enables interrupts** (IE=0x0001, IME=1)
4. **BIOS enters polling loop** (0xD58→0x1A28→0x400→0x77A)
5. **Loop has an EXIT CONDITION** we're not satisfying:
   - Could be polling a register that changes when ROM is ready
   - Could be waiting for DMA completion
   - Could be checking ROM presence via hardware detection
   - Could be a timing-based mechanism (we execute too fast?)
6. **Loop exits**, returns to 0xB4
7. **ROM loader at 0xB4** jumps to 0x08000000 (ROM entry point)
8. **ROM code** initializes 0x03007FFC BEFORE enabling interrupts

### The Problem With Our Emulator

**What happens in our emulator:**
1. Execute BIOS loop very fast (~30 iterations)
2. VBlank IRQ fires almost immediately (after ~170k instructions)
3. BIOS IRQ handler at 0x18 jumps to [0x03007FFC]
4. [0x03007FFC] = 0xFFFFFFFF (uninitialized memory)
5. **CRASH attempting to execute at 0xFFFFFFFF**

**What probably happens on real hardware:**
1. BIOS loop **exits naturally** BEFORE first VBlank IRQ
2. Returns to 0xB4 (ROM loader)
3. ROM loader jumps to 0x08000000
4. ROM entry point initializes system including 0x03007FFC
5. ROM enables interrupts AFTER setting up handlers
6. **No crash because handlers are properly initialized**

### Key Insight

**The BIOS doesn't write to 0x03007FFC** - this is confirmed by:
- Our write logging shows no writes to this address by BIOS
- mGBA's skip BIOS doesn't initialize it
- This is USER IWRAM that ROM is supposed to initialize

**The real problem:** The BIOS loop needs to EXIT before the first IRQ fires, but we don't know what makes it exit.

### Next Steps

1. **Disassemble the loop code** at 0xD58, 0x1A28, 0x400, 0x77A
2. **Find the exit condition** - what register/memory is being polled?
3. **Check if it's timing-based** - does it expect a certain number of cycles?
4. **Look at DMA state** - is it waiting for DMA to finish?
5. **Compare with other emulators** - how do they handle this?
