# PC Handling Fix - Implementation Plan

## Current Status

**Reverted Changes**: We've reverted all recent PC pipeline simulation attempts.

**Current Behavior**:
- PC points to current instruction being fetched
- Instruction handlers add +4 to PC
- Branches add `(offset << 2) + 8` to PC
- **BUG**: Execution skips 2 instructions after branches

## Confirmed Bug Example

From Sonic execution trace:
```
[ 50] BIOS PC=0x0000002C: Instr=0xE92D5000 ...
[B @0x0000003C] offset=4, branch_offset=24
[B] pc_after=0x00000054
```

**Analysis**:
- Branch instruction at 0x3C has offset=4
- Formula: `branch_offset = (offset << 2) + 8 = 16 + 8 = 24`
- PC before branch: 0x3C + 4 = 0x40 (handler added +4)
- PC after branch: 0x40 + 24 = 0x64... wait, that's wrong!

Let me re-analyze. The log shows `pc_after=0x00000054`. Let me check the actual code:

```cpp
// In exec_arm_b():
uint32_t pc_before = parentCPU.R()[15];  // This would be 0x40 after handler added +4?
int32_t branch_offset = (offset << 2) + 8;  // = 24
parentCPU.R()[15] += branch_offset;  // 0x40 + 24 = 0x64
```

But the log shows 0x54, not 0x64. Something doesn't add up. Let me check if executeInstruction adds +4:

Looking at arm_cpu.cpp:
```cpp
void ARMCPU::executeInstruction(uint32_t pc, uint32_t instruction) {
    // ... 
    if (condition_met) {
        (this->*arm_exec_table[index])(instruction);
    } else {
        parentCPU.R()[15] += 4;  // Skip instruction
    }
}
```

So executeInstruction calls the handler, and the HANDLER adds +4. Then in exec_arm_b():

```cpp
void ARMCPU::exec_arm_b(uint32_t instruction) {
    uint32_t pc_before = parentCPU.R()[15];  // Read PC
    int32_t offset = bits<23,0>(instruction);
    if (offset & 0x800000) offset |= 0xFF000000;
    int32_t branch_offset = (offset << 2) + 8;
    parentCPU.R()[15] += branch_offset;  // Add to PC
}
```

Wait, I don't see the handler adding +4 in exec_arm_b! Let me check the actual code:

