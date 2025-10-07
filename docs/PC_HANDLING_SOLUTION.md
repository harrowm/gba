# ARM PC (R15) Handling - The Correct Approach

## Problem Summary

We've been struggling with ARM PC handling because we mixed two incompatible approaches:
1. PC pointing to current instruction being fetched
2. Instruction handlers adding +4 to PC

This caused branches to work but skip 2 instructions (+8 offset bug).

## How mGBA Does It (The Right Way)

After studying mGBA's source code, here's their approach:

### 1. PC is ALWAYS +8 ahead (Pipeline Simulation)

```c
// From mgba src/arm/arm.c ARMStep()
static inline void ARMStep(struct ARMCore* cpu) {
    uint32_t opcode = cpu->prefetch[0];    // Current instruction to execute
    cpu->prefetch[0] = cpu->prefetch[1];   // Slide prefetch buffer
    cpu->gprs[ARM_PC] += WORD_SIZE_ARM;    // PC += 4, now +4 ahead
    LOAD_32(cpu->prefetch[1], cpu->gprs[ARM_PC] & ..., ...);  // Prefetch next+1
    
    // At this point:
    // - opcode = instruction at address X
    // - PC = X + 8 (pointing to X+8)
    // - prefetch[0] = instruction at X+4
    // - prefetch[1] = instruction at X+8
    
    instruction(cpu, opcode);  // Execute instruction at X, with PC = X+8
}
```

**Key insight**: When executing instruction at address `0xN`, `PC = 0xN + 8`.

### 2. Normal Instructions DON'T Modify PC

```c
// From mgba src/arm/isa-arm.c
#define DEFINE_INSTRUCTION_ARM(NAME, BODY) \
    static void _ARMInstruction ## NAME (struct ARMCore* cpu, uint32_t opcode) { \
        int currentCycles = ARM_PREFETCH_CYCLES; \
        BODY; \
        cpu->cycles += currentCycles; \
    }  // NOTE: No PC modification!
```

Most instruction handlers (ADD, SUB, MOV, etc.) **do NOT touch PC**. 
The PC was already advanced by `ARMStep()` before the instruction executes.

### 3. Branches Add Offset Directly to PC

```c
// From mgba src/arm/isa-arm.c
DEFINE_INSTRUCTION_ARM(B,
    int32_t offset = opcode << 8;
    offset >>= 6;
    cpu->gprs[ARM_PC] += offset;  // Just add offset, NO +8!
    currentCycles += ARMWritePC(cpu);)
```

**No +8 compensation** because PC is already +8 ahead!

Example: Branch at 0x100 with offset +32:
- Before branch: PC = 0x108 (already +8 ahead)
- After branch: PC = 0x108 + 32 = 0x140 (correct!)

### 4. PC-Relative Addressing Uses Current PC

```c
// LDR R0, [PC, #offset] at address 0x100
// PC = 0x108 (already +8 ahead)
// Address = PC + offset = 0x108 + offset (correct!)
```

The +8 pipeline effect is automatic because PC is already +8 ahead.

### 5. Getting "Actual" PC Address

```c
// From mgba include/mgba/internal/arm/isa-inlines.h
static inline uint32_t _ARMPCAddress(struct ARMCore* cpu) {
    return cpu->gprs[ARM_PC] - _ARMInstructionLength(cpu) * 2;
}
```

When you need the "real" address (e.g., for debugging), subtract 8.

## Our Current Broken Approach

### What We're Doing Wrong:

```cpp
// executeOneInstruction():
uint32_t pc = R[15];           // PC points to current instruction (0xN)
uint32_t instruction = read32(pc);
R[15] += 8;                    // Try to simulate pipeline: PC = 0xN+8

executeInstruction(pc, instr); // But handlers ALSO modify PC!

// In handlers:
R[15] += 4;  // Now PC = 0xN+12 (WRONG!)
```

**Problem**: We add +8 centrally AND handlers add +4 = PC ends up +12!

### Why Branches Broke:

```cpp
// exec_arm_b():
int32_t offset = (opcode & 0xFFFFFF) << 2;
R[15] = pc + offset;  // Remove the +8 we added
```

But now PC-relative loads read from wrong addresses!

## The Correct Solution

We have **TWO options**:

### Option A: No Pipeline Simulation (RECOMMENDED - Simpler)

Keep our current structure but be consistent:

```cpp
// executeOneInstruction():
uint32_t pc = R[15];  // PC points to current instruction being fetched
uint32_t instruction = read32(pc);
executeInstruction(pc, instruction);
// DON'T touch PC here - let handlers manage it

// Normal instruction handlers:
R[15] += 4;  // Advance to next instruction

// Branch handlers:
int32_t offset = (opcode & 0xFFFFFF) << 2;
R[15] = pc + offset + 8;  // +8 to compensate for pipeline
```

**Pros**:
- Minimal code changes (just revert recent changes)
- Clear ownership: handlers manage PC
- Easier to debug (PC points to current instruction)

**Cons**:
- Not cycle-accurate to hardware
- Need +8 compensation in branches
- PC values don't match real hardware

### Option B: Full Pipeline Simulation (More Work)

Match mGBA's approach exactly:

```cpp
// executeOneInstruction():
uint32_t pc = R[15];  // Save current PC
R[15] += 8;  // Simulate pipeline: PC now +8 ahead
uint32_t instruction = read32(pc);
executeInstruction(pc, instruction);
// DON'T touch PC after - handlers either leave it or set it for branches

// Normal instruction handlers:
// Do NOTHING to PC! It's already advanced.

// Branch handlers:
int32_t offset = (opcode & 0xFFFFFF) << 2;
R[15] = pc + offset;  // NO +8, PC is already ahead
```

**Pros**:
- Matches hardware behavior
- Correct for PC-relative addressing
- More accurate emulation

**Cons**:
- Need to modify ALL instruction handlers (~60+ files)
- Remove all `R[15] += 4` statements
- More complex to get right

## Recommendation

**Use Option A (No Pipeline)** because:
1. Less code to change (just revert)
2. Clearer and easier to maintain
3. We're not going for cycle-perfect accuracy
4. Most games don't care about exact PC pipeline behavior

## Implementation Plan (Option A)

### Step 1: Revert Recent Changes

```bash
git diff src/arm_cpu.cpp src/arm_exec_other.cpp
git checkout HEAD~N -- src/arm_cpu.cpp src/arm_exec_other.cpp
```

### Step 2: Keep Current Handlers As-Is

All handlers currently do `R[15] += 4` - keep this!

### Step 3: Fix Branches to Add +8

```cpp
// exec_arm_b():
int32_t offset = (opcode & 0xFFFFFF) << 2;
R[15] = pc + offset + 8;  // +8 compensates for no pipeline

// exec_arm_bl():
R[14] = pc + 4;  // LR = return address
int32_t offset = (opcode & 0xFFFFFF) << 2;
R[15] = pc + offset + 8;
```

### Step 4: Test

```bash
make
./gba_emulator tests/roms/myhello/myhello.gba  # Should work
./gba_emulator assets/roms/sonic.bin  # Should progress further
```

## Verification

To verify the fix works:

1. **Branch test**: Branch at 0x3C targets 0x54
   - Expected: Next instruction executes at 0x54
   - Not at 0x5C (+8 wrong offset)

2. **LDR test**: `LDR R0, [PC, #0x1CC]` at 0xA8
   - Should read from: 0xA8 + 8 + 0x1CC = 0x27C
   - Should load ROM entry point ~0x08000000
   - Not VRAM address 0x06242404

3. **IRQ test**: IRQ handler preserves LR correctly
   - LR should contain return address
   - Not CPSR value 0x400000DF

## Summary

- **mGBA approach**: PC always +8 ahead, handlers don't touch PC
- **Our approach**: PC at current, handlers add +4, branches add +8
- **Current problem**: Tried to mix both approaches
- **Solution**: Pick ONE approach and be consistent
- **Recommendation**: Option A (no pipeline) - simpler and works
