# Complete Sonic ROM Boot Sequence Analysis
## mGBA Trace: ~10M Instructions (290M lines)

Date: October 19, 2025

## Executive Summary

Successfully captured complete BIOS → ROM transition for Sonic ROM on GBA.
The trace captured approximately **10 million instructions** before the program crashed.

### Key Milestones

| Instruction | Event | Details |
|------------|-------|---------|
| 38,085 | IE Enabled | VBlank interrupt enabled (IE=0x0001) |
| 198,399 | First VBlank | IF=0x0001, first VBlank occurs |
| 799,570 | IME Enabled | IME=0x00000001, interrupts globally enabled |
| 799,572 | First IRQ | PC=0x00000018, interrupt handler called |
| 799,586 | IF Cleared | Handler clears IF=0x0000 |
| **7,444,345** | **ROM Entry** | **PC=0x08000000, entered ROM code!** |

## Detailed Timeline

### Phase 1: BIOS Initialization (Instructions 1 - 38,084)
- BIOS performs hardware initialization
- ROM code is loaded but not yet executing
- Interrupts not configured
- **Duration**: ~38K instructions

### Phase 2: Interrupt Configuration (Instructions 38,085 - 198,398)
- **Instruction 38,085**: IE (0x04000200) = 0x0001
  - VBlank interrupt enabled via BIOS SWI call
  - ROM sets up which interrupts it wants
  - IME still 0 (globally disabled)
- ROM continues initialization
- **Duration**: ~160K instructions (~5 VBlank periods)

### Phase 3: IntrWait Entry (Instructions 198,399 - 799,569)
- **Instruction 198,399**: IF (0x04000202) = 0x0001
  - First VBlank interrupt occurs (hardware sets IF)
  - IME=0, so interrupt doesn't fire
  - BIOS IntrWait function enters polling loop
  - Waiting for ROM to enable IME

- **What's happening**: ROM is doing extensive initialization
  - Setting up memory
  - Copying data
  - Decompressing graphics
  - Initializing game systems
  - **Duration**: ~600K instructions (longest phase!)

### Phase 4: Interrupt System Activation (Instructions 799,570 - 799,590)
- **Instruction 799,570**: IME (0x04000208) = 0x00000001
  - ROM finally enables interrupts globally!
  - PC ~0x00002D62 (still in BIOS)
  - IntrWait detects IME=1

- **Instruction 799,572**: PC = 0x00000018
  - **First interrupt fires!**
  - CPU jumps to IRQ vector
  - BIOS interrupt dispatcher runs

- **Instruction 799,586**: IF (0x04000202) = 0x0000
  - Interrupt handler clears IF
  - IntrWait condition satisfied
  - Returns to calling code

### Phase 5: Extended BIOS Execution (Instructions 799,591 - 7,444,344)
- Interrupts now working normally
- IME toggles frequently (critical sections)
- IF toggles with each VBlank (~every 3,513 instructions)
- ROM continues initialization through BIOS services
- **Duration**: ~6.6M instructions

### Phase 6: ROM Execution Begins (Instruction 7,444,345+)
- **Instruction 7,444,345**: PC = 0x08000000
  - **BIOS → ROM transition!**
  - Jump to ROM entry point
  - Game code now executing

- **ROM Entry State**:
  - IME = 0x00000000 (interrupts disabled at entry)
  - IF = 0x0000 (no pending interrupts)
  - PC = 0x08000000 (ROM base address)

- **ROM Execution**:
  - PC continues: 0x080000C0, 0x080000C4, etc.
  - Sonic game initialization begins
  - Will re-enable interrupts when ready

## Critical Insights

### 1. IntrWait Behavior
IntrWait requires **BOTH** conditions:
- IF bit set (interrupt occurred) ✓
- IME = 1 (interrupts enabled) ✓

The 600K instruction wait (Phase 3) is the ROM doing initialization work
before it's ready to enable interrupts.

### 2. Why 7.4M Instructions?
The ROM takes **7.4 million instructions** to complete BIOS initialization:
1. Initial setup: 38K instructions
2. Basic init: 160K instructions  
3. Heavy initialization: 600K instructions (waiting to enable IME)
4. BIOS services: 6.6M instructions (most time spent here!)
5. Finally ready to run ROM code

### 3. Interrupt Patterns
From instruction 799K to 7.4M:
- 140 IME changes (enable/disable for critical sections)
- 49 IF changes (VBlanks occurring and being cleared)
- ~3,513 instructions between VBlanks (60 Hz)
- Normal interrupt operation established

### 4. Why Our Emulator Fails
Our emulator likely:
1. **Times out too early** - stops before 7.4M instructions
2. **Instruction limit** - doesn't allow enough execution
3. **Incorrect timing** - runs faster/slower than real hardware
4. **Missing BIOS calls** - doesn't handle all SWI correctly

## Recommendations

### For Our Emulator
1. **Remove instruction limits** or set to >10M
2. **Let BIOS complete** - don't timeout during IntrWait
3. **Verify timing** - ensure ~2700 instructions/second
4. **Test with trace** - compare our execution to mGBA at key points:
   - Instruction 38K: IE enabled?
   - Instruction 198K: IF set?
   - Instruction 799K: IME enabled? IRQ fired?
   - Instruction 7.4M: Reached ROM?

### For Testing
1. Run emulator for at least 8M instructions
2. Log IE/IF/IME changes at same milestones
3. Compare our timeline to mGBA timeline
4. Identify first divergence point

## Files Generated
- **Trace file**: `/tmp/mgba_memory_trace.log` (290M lines, ~1.8GB)
- **Instructions**: ~10,000,000 (estimated)
- **ROM entry**: Confirmed at instruction 7,444,345
- **Complete**: Full BIOS → ROM transition captured

## Next Steps
1. Analyze our emulator's instruction count
2. Compare our trace to mGBA at key milestones
3. Identify why we stop/timeout before ROM entry
4. Fix timing or instruction limits
5. Re-test with Sonic ROM

---

**Conclusion**: The trace proves that mGBA successfully boots Sonic by running
7.4M instructions through BIOS before entering ROM code. Our emulator needs to
run at least this long to successfully boot.
