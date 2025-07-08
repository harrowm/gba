# ARM7TDMI Instruction Set Implementation

## Overview

This document describes the comprehensive ARM7TDMI instruction set implementation for the GBA emulator. The implementation follows the same successful architecture as the Thumb instruction set but handles the increased complexity of 32-bit ARM instructions.

## Architecture Overview

### Instruction Format Detection

ARM instructions use a hierarchical decoding scheme based on bits 27-25:

```c
// Primary instruction format (bits 27-25)
000: Data processing and multiply instructions
001: Data processing with immediate operand
010: Single data transfer 
011: Single data transfer (or undefined)
100: Block data transfer
101: Branch and branch with link
110: Coprocessor data transfer
111: Coprocessor operations and software interrupt
```

### Condition Code System

Every ARM instruction can be conditionally executed based on the 4-bit condition field (bits 31-28):

```c
typedef enum {
    ARM_COND_EQ = 0x0,  // Equal (Z=1)
    ARM_COND_NE = 0x1,  // Not equal (Z=0)
    ARM_COND_CS = 0x2,  // Carry set (C=1)
    ARM_COND_CC = 0x3,  // Carry clear (C=0)
    ARM_COND_MI = 0x4,  // Minus (N=1)
    ARM_COND_PL = 0x5,  // Plus (N=0)
    ARM_COND_VS = 0x6,  // Overflow (V=1)
    ARM_COND_VC = 0x7,  // No overflow (V=0)
    ARM_COND_HI = 0x8,  // Higher (C=1 && Z=0)
    ARM_COND_LS = 0x9,  // Lower or same (C=0 || Z=1)
    ARM_COND_GE = 0xA,  // Greater or equal (N==V)
    ARM_COND_LT = 0xB,  // Less than (N!=V)
    ARM_COND_GT = 0xC,  // Greater than (Z=0 && N==V)
    ARM_COND_LE = 0xD,  // Less than or equal (Z=1 || N!=V)
    ARM_COND_AL = 0xE,  // Always
    ARM_COND_NV = 0xF   // Never (deprecated)
} ARMCondition;
```

## Instruction Categories

### 1. Data Processing Instructions

**Operations Supported:**
- Logical: AND, EOR, ORR, BIC, MVN
- Arithmetic: ADD, ADC, SUB, SBC, RSB, RSC
- Comparison: CMP, CMN, TST, TEQ
- Move: MOV

**Cycle Timing:**
- Base: 1 cycle
- +1 cycle if shift amount specified by register
- +2 cycles if PC is destination register

**Example:**
```assembly
ADD R0, R1, R2        ; 1 cycle
ADD R0, R1, R2, LSL R3 ; 2 cycles (register shift)
ADD PC, R1, R2        ; 3 cycles (PC destination)
```

### 2. Multiply Instructions

**Operations Supported:**
- MUL (Multiply)
- MLA (Multiply-Accumulate)
- UMULL (Unsigned Multiply Long)
- UMLAL (Unsigned Multiply-Accumulate Long)
- SMULL (Signed Multiply Long)
- SMLAL (Signed Multiply-Accumulate Long)

**Cycle Timing:**
- Base: 2 cycles (3 for long multiply)
- +1-4 cycles based on operand value

**Operand-Based Timing:**
```c
uint32_t arm_get_multiply_cycles(uint32_t operand) {
    if (operand == 0) return 1;
    if ((operand & 0xFFFFFF00) == 0) return 1;      // 8-bit
    if ((operand & 0xFFFF0000) == 0) return 2;      // 16-bit
    if ((operand & 0xFF000000) == 0) return 3;      // 24-bit
    return 4;                                        // 32-bit
}
```

### 3. Single Data Transfer (LDR/STR)

**Addressing Modes:**
- Immediate offset: `[Rn, #offset]`
- Register offset: `[Rn, Rm]`
- Scaled register: `[Rn, Rm, LSL #2]`
- Pre-indexed: `[Rn, #offset]!`
- Post-indexed: `[Rn], #offset`

**Cycle Timing:**
- Base: 1 cycle
- +1-6 cycles for memory access (depends on memory region)
- +1 cycle for address calculation if needed

### 4. Block Data Transfer (LDM/STM)

**Addressing Modes:**
- IA (Increment After): `LDMIA/STMIA`
- IB (Increment Before): `LDMIB/STMIB`
- DA (Decrement After): `LDMDA/STMDA`
- DB (Decrement Before): `LDMDB/STMDB`

**Cycle Timing:**
- Base: 1 cycle
- +1 cycle per register transferred
- +1 cycle for first memory access

### 5. Branch Instructions

**Types:**
- B (Branch): Unconditional jump
- BL (Branch with Link): Jump and save return address
- BX (Branch and Exchange): Switch between ARM/Thumb modes

**Cycle Timing:**
- All branch types: 3 cycles

### 6. Software Interrupt (SWI)

**Functionality:**
- Causes processor to enter Supervisor mode
- Saves current PC in LR_svc
- Jumps to SWI vector (0x08)

**Cycle Timing:**
- 3 cycles

## Operand Calculation

### Immediate Operands

ARM immediate operands consist of an 8-bit value rotated right by an even number (0-30):

```c
uint32_t arm_calculate_immediate_operand(uint32_t instruction, uint32_t* carry_out) {
    uint32_t immediate = instruction & 0xFF;
    uint32_t rotate = ((instruction >> 8) & 0xF) * 2;
    
    if (rotate == 0) {
        *carry_out = 0;
        return immediate;
    }
    
    *carry_out = (immediate >> (rotate - 1)) & 1;
    return (immediate >> rotate) | (immediate << (32 - rotate));
}
```

### Register Operands with Shift

ARM supports four shift types with immediate or register-specified amounts:

```c
typedef enum {
    ARM_SHIFT_LSL = 0x0,  // Logical shift left
    ARM_SHIFT_LSR = 0x1,  // Logical shift right
    ARM_SHIFT_ASR = 0x2,  // Arithmetic shift right
    ARM_SHIFT_ROR = 0x3   // Rotate right
} ARMShiftType;
```

## Cycle-Driven Execution

### Integration with Timing System

The ARM CPU supports cycle-driven execution similar to the Thumb implementation:

```cpp
void ARMCPU::executeWithTiming(uint32_t cycles, TimingState* timing) {
    while (cycles > 0) {
        uint32_t cycles_until_event = timing_cycles_until_next_event(timing);
        uint32_t instruction = parentCPU.getMemory().read32(parentCPU.R()[15]);
        uint32_t instruction_cycles = calculateInstructionCycles(instruction);
        
        if (instruction_cycles <= cycles_until_event) {
            // Execute instruction
            decodeAndExecute(instruction);
            timing_advance(timing, instruction_cycles);
            cycles -= instruction_cycles;
        } else {
            // Process timing event first
            timing_advance(timing, cycles_until_event);
            timing_process_timer_events(timing);
            timing_process_video_events(timing);
            cycles -= cycles_until_event;
        }
    }
}
```

### Instruction Cycle Calculation

```cpp
uint32_t ARMCPU::calculateInstructionCycles(uint32_t instruction) {
    uint32_t registers[16];
    for (int i = 0; i < 16; i++) {
        registers[i] = parentCPU.R()[i];
    }
    
    uint32_t pc = parentCPU.R()[15];
    uint32_t cpsr = parentCPU.CPSR();
    return arm_calculate_instruction_cycles(instruction, pc, registers, cpsr);
}
```

## Memory Access Timing

ARM instructions access memory with different timing characteristics:

### Internal Memory (Fast Access)
- BIOS ROM: 1 cycle (32-bit access)
- On-chip WRAM: 1 cycle
- I/O Registers: 1 cycle
- OAM: 1 cycle
- Palette RAM: 1 cycle

### External Memory (Slower Access)
- On-board WRAM: 6 cycles (32-bit access)
- VRAM: 2 cycles + potential video conflicts

### GamePak Memory (Configurable)
- ROM: 5 cycles (non-sequential), 3 cycles (sequential)
- SRAM: 5 cycles (8-bit only)

## Performance Characteristics

### Cycle Calculation Overhead
- **ARM Instructions**: ~5μs per calculation (negligible overhead)
- **Conditional Execution**: Only 1 cycle if condition not met
- **Complex Instructions**: Accurately modeled (LDM/STM, multiply)

### Benchmark Results
```
ARM Instruction Mix (7 instructions × 50,000 iterations):
- Total cycles calculated: 900,000
- Average cycles per instruction: 2.57
- Calculation overhead: <0.1% of execution time
```

## Testing and Validation

### Comprehensive Test Suite

The ARM implementation includes extensive tests:

1. **Condition Code Tests**: All 16 condition combinations
2. **Cycle Timing Tests**: Each instruction category
3. **Operand Calculation Tests**: Immediate and shifted register operands
4. **Format Detection Tests**: Proper instruction decoding
5. **Integration Tests**: Timing system interaction
6. **Performance Benchmarks**: Calculation overhead measurement

### Example Test Results
```
=== ARM Instruction Set Tests ===

✓ Condition code evaluation (16 conditions)
✓ Cycle calculation (10 instruction types)
✓ Operand calculation (immediate, shifted register)
✓ Instruction format identification (8 formats)
✓ Timing system integration
✓ Performance benchmark (900K cycles calculated)

=== All ARM Tests Passed! ===
```

## Future Enhancements

### Planned Features
1. **Coprocessor Support**: CP15 system control coprocessor
2. **Exception Handling**: Complete interrupt and exception processing
3. **MMU Emulation**: Memory management unit for advanced games
4. **Performance Optimization**: Instruction caching and prediction
5. **Debug Support**: Breakpoints and single-stepping

### ARM vs Thumb Comparison

| Feature | ARM | Thumb |
|---------|-----|-------|
| Instruction Size | 32-bit | 16-bit |
| Conditional Execution | All instructions | Branches only |
| Register Access | Full (R0-R15) | Limited (R0-R7 mostly) |
| Addressing Modes | Complex | Simple |
| Code Density | Lower | Higher |
| Performance | Higher per instruction | Lower per instruction |
| Barrel Shifter | Integrated | Limited |

## Usage Example

```cpp
// Initialize CPU with ARM mode
CPU cpu(memory, interruptController);
cpu.CPSR() &= ~CPU::FLAG_T; // Clear Thumb mode

// Execute with cycle-accurate timing
TimingState timing;
timing_init(&timing);

// Run one frame (280,896 cycles at 16.78MHz)
cpu.executeWithTiming(280896, &timing);

// Check final state
printf("Total cycles: %llu\n", timing.total_cycles);
printf("Current scanline: %d\n", timing.current_scanline);
```

This ARM7TDMI implementation provides accurate cycle timing, comprehensive instruction support, and seamless integration with the existing timing system, enabling precise GBA emulation.
