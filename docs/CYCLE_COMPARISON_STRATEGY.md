# Instruction-by-Instruction Cycle Comparison Strategies

## Problem
We need to compare cycle costs per instruction between our emulator and mGBA, but:
- Our emulator: Has cycle counts in trace (Instruction #N (Cycle: X))
- mGBA GDB: Likely doesn't expose cycle counter directly

## Solution Options

### Option 1: Calculate mGBA Cycles from Our Timing Model ⭐ RECOMMENDED
**Approach**: Parse mGBA trace, calculate expected cycles using OUR timing functions

```python
# For each instruction in mGBA trace:
# 1. Get PC and instruction opcode (fetch from trace or disassemble)
# 2. Call our arm_calculate_instruction_cycles() or thumb_calculate_instruction_cycles()
# 3. Compare to actual cycles from our trace
# 4. Any difference = our timing is wrong for that instruction type
```

**Pros**:
- Uses existing mGBA trace
- Identifies exactly which instruction types have wrong timing
- Can pinpoint specific opcodes with issues

**Cons**:
- Assumes we can determine instruction opcode from PC (need to disassemble)
- Need to handle ARM vs THUMB mode

### Option 2: Use mGBA's Timing Tables Directly
**Approach**: mGBA is open source - extract its timing calculation code

```python
# Copy mGBA's timing calculation logic:
# - src/arm/timing.c or similar
# - Apply to each instruction in trace
# - Compare to our emulator's actual cycles
```

**Pros**:
- Uses mGBA's actual timing model (ground truth)
- Can identify where our implementation differs

**Cons**:
- Need to extract and port mGBA code
- May have dependencies on mGBA's internal structures

### Option 3: Compare Aggregate Statistics
**Approach**: Don't compare instruction-by-instruction, compare patterns

```python
# Group instructions by type and calculate average cycles:
# - All branches: avg cycles?
# - All data processing: avg cycles?
# - All loads/stores: avg cycles?
# - Compare aggregates between our emulator and mGBA
```

**Pros**:
- Simpler to implement
- Can identify systematic biases (e.g., "all branches are 1 cycle too fast")

**Cons**:
- Won't pinpoint specific problematic instructions
- Might miss edge cases

### Option 4: Instrument mGBA Source Code
**Approach**: Modify mGBA to print cycle count per instruction

```c
// In mGBA's CPU execution loop:
printf("Instruction #%d (Cycle: %llu)\n", instr_count, arm->cycles);
```

**Pros**:
- Gets exact cycle counts from mGBA
- Perfect comparison possible

**Cons**:
- Requires building mGBA from source
- Need to understand mGBA's codebase
- Time-consuming

## Recommended Approach: **Option 1 + Option 2 Hybrid**

### Step 1: Extract Instruction Opcodes
Parse both traces and extract:
- PC
- ARM/THUMB mode (from CPSR)
- Instruction opcode (read from BIOS memory at PC)

### Step 2: Calculate Expected Cycles Using mGBA's Model
Instead of calling our functions, use mGBA's documented cycle costs:
- Branch: 2S + 1N = 3 cycles (BIOS)
- Data processing: 1S = 1 cycle
- LDR: 1S + 1N + 1I = 3 cycles (BIOS: 1+1+1=3)
- etc.

Reference: GBATEK timing documentation matches mGBA's implementation

### Step 3: Compare
```python
for instr in range(1, 700):
    our_cycles = our_trace[instr].cycles - our_trace[instr-1].cycles
    expected_cycles = calculate_expected_cycles(instr_opcode, mode)
    
    if our_cycles != expected_cycles:
        print(f"Instruction #{instr}: PC={pc}, expected={expected_cycles}, actual={our_cycles}")
```

## Implementation Plan

1. **Write cycle calculator** (use GBATEK/mGBA as reference)
2. **Parse both traces** and extract instruction info
3. **Disassemble opcodes** (use capstone or read from memory)
4. **Compare** and identify patterns
5. **Fix** timing functions based on findings

This avoids needing mGBA GDB access while still getting accurate cycle comparisons!
