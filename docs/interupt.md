# Game Boy Advance Interrupt Handling

The Game Boy Advance uses a sophisticated interrupt system that builds upon the original Game Boy's design while adding more capabilities. Here's a comprehensive explanation of how it works:

## Interrupt Registers

The GBA has several key interrupt-related registers:

### 1. **IE (Interrupt Enable) - 0x04000200**
- **Purpose**: Controls which interrupts are enabled
- **Size**: 16 bits (though not all bits are used)
- **Each bit corresponds to a specific interrupt source**
- **Write 1** to enable an interrupt, **write 0** to disable it

### 2. **IF (Interrupt Flag) - 0x04000202**
- **Purpose**: Indicates which interrupts have occurred
- **Size**: 16 bits
- **Each bit is set when the corresponding interrupt condition occurs**
- **Must be manually cleared by writing 1 to the bit**

### 3. **IME (Interrupt Master Enable) - 0x04000208**
- **Purpose**: Global interrupt enable/disable
- **Bit 0**: 1 = interrupts enabled globally, 0 = all interrupts disabled
- **Other bits**: unused

## Interrupt Initialization Timeline (Sonic ROM Analysis)

Based on mGBA trace analysis of **~10,000,000 instructions** showing complete BIOS → ROM transition:

### Complete Boot Sequence

| Instruction | Event | PC | IME | IE | IF | Phase |
|------------|-------|-----|-----|-----|-----|-------|
| 38,085 | IE enabled | BIOS | 0 | 0x0001 | 0x0000 | Setup |
| 198,399 | First VBlank | 0x00001124 | 0 | 0x0001 | 0x0001 | IntrWait |
| 799,570 | IME enabled | 0x00002D62 | 1 | 0x0001 | 0x0001 | IRQ Start |
| 799,572 | First IRQ | 0x00000018 | 1 | 0x0001 | 0x0001 | Handler |
| 799,586 | IF cleared | BIOS | 1 | 0x0001 | 0x0000 | Handler Done |
| **7,444,345** | **ROM Entry** | **0x08000000** | 0 | 0x0001 | 0x0000 | **ROM!** |

### Phase 1: Early BIOS (Instructions 1 - 198,398)
1. **Instruction ~38,085**: IE (0x04000200) = 0x0001
   - ROM enables VBlank interrupt via BIOS call
   - Still early in BIOS execution (PC in BIOS range)
   - IME remains 0 (interrupts globally disabled)

2. **Instruction ~198,399**: IF (0x04000202) = 0x0001
   - First VBlank interrupt flagged by hardware
   - Still executing in BIOS IntrWait loop (PC ~0x00001124)
   - **IME=0**, so CPU does not service the interrupt yet
   - IntrWait continues polling, waiting for IME to be enabled

### Phase 2: IntrWait Exit (Instructions 799,570+)
3. **Instruction ~799,570**: IME (0x04000208) = 0x00000001
   - **IME finally enabled!** (PC ~0x00002D62 in BIOS)
   - This happens ~600K instructions after first VBlank IF was set
   - IntrWait polling detects IME=1 and prepares to exit

4. **Instruction ~799,572**: PC = 0x00000018
   - **IRQ vector!** CPU jumps to interrupt handler
   - This is the first actual interrupt service
   - Handler runs and clears IF back to 0x0000 (instruction 799,586)

5. **Instruction ~799,586+**: IF (0x04000202) = 0x0000
   - Interrupt handler successfully clears IF
   - IntrWait condition satisfied, function returns
   - BIOS continues execution

### Phase 3: Extended BIOS Operation (Instructions 800K - 7.4M)
6. **Instructions 800K - 7.4M**: Normal interrupt operation
   - IME toggles 140+ times (enable/disable for critical sections)
   - IF toggles 49+ times as VBlanks occur and are handled
   - ROM continues extensive initialization through BIOS services
   - Each VBlank: IF set → handler called → IF cleared
   - System operates normally for ~6.6M instructions

### Phase 4: ROM Entry (Instruction 7,444,345)
7. **Instruction 7,444,345**: PC = 0x08000000
   - **BIOS → ROM transition complete!**
   - Jump to ROM entry point
   - ROM code now executing
   - IME=0 at ROM entry (ROM will re-enable when ready)

### Key Finding: Why IntrWait Takes So Long

The BIOS IntrWait function waits for **two** conditions:
1. ✅ **IF flag set** (happens at instruction ~198K)
2. ✅ **IME enabled** (happens at instruction ~799K)

**Timeline:**
- Instructions 1-38K: Setup phase (~38K instructions)
- Instructions 38K-198K: Waiting for first VBlank (~160K instructions)
- Instructions 198K-799K: **Waiting for IME to be enabled** (~600K instructions!)
- Instruction 799K+: IME enabled, interrupt fires, IntrWait exits
- Instructions 799K-7.4M: Extended BIOS execution (~6.6M instructions!)
- Instruction 7.4M: Finally enters ROM code

**Why the delay?** The BIOS is waiting for the ROM to enable IME. This is intentional - it allows the ROM to set up interrupt handlers before interrupts can fire. Sonic takes ~600K instructions of initialization before it's ready to enable interrupts, then uses BIOS services for another 6.6M instructions before finally entering ROM code.

**Why 7.4M instructions to ROM?** The ROM does extensive initialization through BIOS calls:
- Setting up memory regions
- Copying data from ROM to RAM
- Decompressing graphics
- Initializing sound system
- Setting up DMA channels
- And much more...

All of this happens while executing BIOS code on behalf of the ROM.

## GBA Interrupt Sources and Bit Assignments

| Bit | Interrupt Source | Description |
|-----|------------------|-------------|
| 0   | V-Blank          | Vertical blanking period |
| 1   | H-Blank          | Horizontal blanking period |
| 2   | V-Counter        | When scanline matches LY register |
| 3   | Timer 0          | Timer 0 overflow |
| 4   | Timer 1          | Timer 1 overflow |
| 5   | Timer 2          | Timer 2 overflow |
| 6   | Timer 3          | Timer 3 overflow |
| 7   | Serial           | Serial communication complete |
| 8   | DMA 0            | DMA channel 0 complete |
| 9   | DMA 1            | DMA channel 1 complete |
| 10  | DMA 2            | DMA channel 2 complete |
| 11  | DMA 3            | DMA channel 3 complete |
| 12  | Keypad           | Key input (when set to interrupt on press/release) |
| 13  | Game Pak         | Cartridge interrupt (rarely used) |
| 14-15 | Unused         | Always read as 0 |

## How IF Register Works

### What Sets IF Bits
IF bits are **automatically set by hardware** when the corresponding interrupt condition occurs:

- **V-Blank**: Set at the start of vertical blanking
- **H-Blank**: Set at the start of each horizontal blanking period
- **V-Counter**: Set when the current scanline equals the value in the LY register
- **Timers**: Set when the timer counter overflows (reaches 0xFFFF and wraps to 0)
- **DMA**: Set when the DMA transfer completes
- **Keypad**: Set when key state matches the configured interrupt condition
- **Serial**: Set when serial transfer completes

### What Resets/Clears IF Bits
IF bits are **manually cleared by software**:
- **Write 1 to the specific bit** you want to clear
- Writing 0 has no effect
- **Important**: You must clear the IF bit in your interrupt handler, otherwise the interrupt will keep firing

**Example**: To clear the V-Blank interrupt flag:
```c
REG_IF = IRQ_VBLANK; // or REG_IF = 0x0001;
```

## Interrupt Processing Flow

1. **Interrupt occurs**: Hardware sets the corresponding bit in IF register
2. **Interrupt evaluation**:
  - Check if IME = 1 (global enable)
  - Check if corresponding IE bit = 1 (specific interrupt enabled)
  - Check if corresponding IF bit = 1 (interrupt pending)
3. **If all conditions met**: CPU jumps to interrupt vector (0x0000001C for IRQ)
4. **Interrupt handler executes**:
  - Must clear the IF bit that caused the interrupt
  - Perform necessary processing
5. **Return from interrupt**: Execution resumes at interrupted point

## Important Notes

### Nested Interrupts
- By default, interrupts are **disabled** when an interrupt handler starts
- To allow nested interrupts, you must:
 1. Clear the IF bit early in your handler
 2. Set IME = 1 to re-enable interrupts
- Be careful with nested interrupts as they can cause stack overflow

### Clearing Multiple Interrupts
You can clear multiple IF bits at once:
```c
// Clear both V-Blank and Timer 0 interrupts
REG_IF = IRQ_VBLANK | IRQ_TIMER0;
```

### Reading IF Register
- Reading IF shows which interrupts have occurred but haven't been cleared
- Useful for polling instead of using interrupts

### Typical Interrupt Handler Structure
```assembly
irq_handler:
   push {r0-r3, r12, lr}    ; Save registers

   ldr r0, =REG_IF          ; Load IF register address
   ldr r1, [r0]             ; Read pending interrupts

   ; Check which interrupt occurred and handle accordingly
   tst r1, #IRQ_VBLANK
   beq not_vblank
   ; Handle V-Blank...
   not_vblank:

   ; Clear all pending interrupts
   str r1, [r0]             ; Write back to clear flags

   pop {r0-r3, r12, lr}
   subs pc, lr, #4          ; Return from interrupt
```

This system provides flexible and efficient interrupt handling while maintaining compatibility with the original Game Boy's interrupt concepts.
Sent from my iPhone