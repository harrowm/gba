# PC Handling Analysis - Final Conclusion

## Investigation Summary

After extensive analysis of mGBA's implementation and our own code, here are the findings:

### mGBA's Approach

**Pipeline Simulation:**
```c
static inline void ARMStep(struct ARMCore* cpu) {
    uint32_t opcode = cpu->prefetch[0];
    cpu->prefetch[0] = cpu->prefetch[1];
    cpu->gprs[ARM_PC] += WORD_SIZE_ARM;  // PC now +4 ahead
    LOAD_32(cpu->prefetch[1], cpu->gprs[ARM_PC] & ..., ...);
    // PC is now +8 ahead of instruction being executed
    instruction(cpu, opcode);
}
```

**Key characteristics:**
- PC is ALWAYS +8 ahead during instruction execution  
- Normal instructions don't modify PC (it's already advanced)
- Branches just add offset directly to PC (no +8 compensation)
- To get "real" PC address: `actual_PC = PC - 8`

### Our Current Approach (After Revert)

**No Pipeline Simulation:**
```cpp
void ARMCPU::executeOneInstruction() {
    uint32_t pc = R[15];  // PC points to current instruction
    uint32_t instruction = read32(pc);
    executeInstruction(pc, instruction);
}

void ARMCPU::exec_arm_b(uint32_t instruction) {
    int32_t branch_offset = (offset << 2) + 8;  // +8 compensation
    R[15] += branch_offset;
}
```

**Key characteristics:**
- PC points to current instruction being fetched
- Instruction handlers add +4 to PC for next instruction
- Branches add `(offset << 2) + 8` to compensate

### Branch Calculation Verification

**Test Case: Branch at 0x3C with offset=4**

**ARM specification:**
```
target = PC_fetch + 8 + (offset << 2)
       = 0x3C + 8 + 16
       = 0x54
```

**Our calculation:**
```
R[15] = 0x3C (current instruction)
branch_offset = (4 << 2) + 8 = 24
R[15] += 24
R[15] = 0x3C + 24 = 0x54 ✓ CORRECT!
```

**Conclusion: Our branch calculation is CORRECT!**

### The Real Bug

After careful analysis, the branch at 0x3C **correctly** goes to 0x54. The problem is NOT a +8 offset bug!

**Evidence from trace:**
```
[ 50] BIOS PC=0x0000002C: ... LR=0x400000DF
[B @0x0000003C] offset=4, branch_offset=24
[B] pc_after=0x00000054
[PC REGION] BIOS -> UNKNOWN at PC=0x400000DB
```

**Analysis:**
1. At 0x002C, LR is already corrupted to 0x400000DF
2. Branch at 0x003C correctly goes to 0x0054
3. Code at 0x0054+ tries to return using corrupted LR
4. Jump to invalid address 0x400000DB

**The corruption happens BEFORE the branch, not because of it!**

### What Actually Happened

Looking at the BIOS IRQ handler disassembly:
```
0x0028: MRS LR, CPSR    ; LR = 0x400000DF (CPSR value)
0x002C: STM R13, {...}  ; Should save LR to stack
...
0x0054: LDR R13, [...]  ; Restore SP
0x0058: LDM R13, {...}  ; Should restore LR from stack
```

**The bug is**: Something about our STM/LDM implementation is broken!
- STM at 0x002C should save LR (0x400000DF) to stack
- LDM at 0x0058 should restore original LR from stack
- But LR stays as 0x400000DF instead of being restored

### Action Items

1. **PC handling is NOT the problem** - our current approach works correctly
2. **Focus on STM/LDM instructions** - these are likely broken
3. **The corruption happens at 0x028-0x02C, not at the branch**
4. **Test STM/LDM with simple test case** before continuing with Sonic

### Recommendation

**DO NOT change PC handling!** The current approach works. Instead:

1. Add detailed logging to `exec_arm_stm` and `exec_arm_ldm`
2. Verify stack push/pop operations work correctly
3. Check if register list parsing is correct
4. Ensure writeback to R13 (SP) works properly

### Files to Check

- `src/arm_exec_other.cpp`: STM/LDM implementation
- Stack operations and SP management
- Register list bitmask handling

## Conclusion

We spent a lot of time chasing a non-existent PC bug. The real bug is in STM/LDM instructions, which are not properly saving/restoring registers from the stack. The branch at 0x3C works perfectly - it's what happens BEFORE the branch (corrupted LR not being saved) that causes the crash later.

**Next Steps**: Debug STM/LDM, not PC handling!
