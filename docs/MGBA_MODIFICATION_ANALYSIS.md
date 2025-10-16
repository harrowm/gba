# mGBA Source Modification Analysis

## Overview
Analysis of how difficult it would be to modify mGBA source code to expose cycle counts through GDB or add per-instruction Lua callbacks.

## Option 1: Extend GDB Interface to Expose Cycle Count

### Current GDB Implementation
- **Location**: `/tmp/mgba/src/debugger/gdb-stub.c`
- **Protocol**: Standard GDB remote protocol
- **Command Handler**: Line 773 - `case 'q':` calls `_processQReadCommand()`

### Implementation Difficulty: **MODERATE** ⭐⭐⭐

### Required Changes:

1. **Add Custom Query Command** (Lines 544-576 in `gdb-stub.c`)
```c
static void _processQReadCommand(struct GDBStub* stub, const char* message) {
    stub->outgoing[0] = '\0';
    // ... existing commands ...
    
    // ADD THIS:
    } else if (!strncmp("CpuCycles#", message, 10)) {
        struct ARMCore* cpu = stub->d.p->cpu;
        snprintf(stub->outgoing, GDB_STUB_MAX_LINE - 4, "%08x", cpu->cycles);
    
    // ... rest of function ...
}
```

2. **Access from GDB**:
```bash
(gdb) monitor cpucycles
# Would return hex value like: 000004d8 (1240 cycles)
```

### Pros:
- ✅ **Very simple change** - only ~3 lines of code
- ✅ **Clean interface** - uses standard GDB monitor command
- ✅ **No rebuild complexity** - single file modification
- ✅ **Can query on breakpoint** - get cycle count when stopped

### Cons:
- ❌ **Manual only** - must stop execution to query (can't get per-instruction)
- ❌ **Requires breakpoint** - need to break at each instruction for comparison
- ❌ **Scripting overhead** - would need Python script to automate breakpoint stepping

### Automation Approach:
```python
import gdb

# Set breakpoint on every instruction
gdb.execute("break *0x0")
for i in range(1000):
    gdb.execute("step")
    cycles = gdb.execute("monitor cpucycles", to_string=True)
    pc = gdb.execute("print $pc", to_string=True)
    print(f"Instruction {i}: PC={pc}, Cycles={cycles}")
```

**Performance**: Very slow - GDB protocol overhead on every instruction (~1000x slower than native)

---

## Option 2: Add Per-Instruction Lua Callback

### Current Lua Implementation
- **Callback System**: `/tmp/mgba/src/script/context.c`
- **Trigger Function**: Line 250 - `mScriptContextTriggerCallback()`
- **Execution Loop**: `/tmp/mgba/src/arm/arm.c`
  - Line 201: `ARMStep()` - ARM instruction execution
  - Line 222: `ThumbStep()` - THUMB instruction execution
  - Line 229: `ARMRun()` - Main execution loop

### Current Callbacks:
- `frame` - triggered once per video frame
- `keysRead` - triggered on input
- `start`, `stop`, `reset`, `crashed` - lifecycle events

### Implementation Difficulty: **MODERATE to HIGH** ⭐⭐⭐⭐

### Required Changes:

1. **Modify ARMStep() Function** (`src/arm/arm.c`, line 201):
```c
static inline void ARMStep(struct ARMCore* cpu) {
    uint32_t opcode = cpu->prefetch[0];
    cpu->prefetch[0] = cpu->prefetch[1];
    cpu->gprs[ARM_PC] += WORD_SIZE_ARM;
    LOAD_32(cpu->prefetch[1], cpu->gprs[ARM_PC] & cpu->memory.activeMask, 
            cpu->memory.activeRegion);
    
    // ADD THIS: Per-instruction callback
    if (cpu->components && cpu->components[0]->instructionCallback) {
        cpu->components[0]->instructionCallback(cpu, opcode, cpu->cycles);
    }
    
    unsigned condition = opcode >> 28;
    // ... rest of function ...
}
```

2. **Add to ThumbStep()** (line 222):
```c
static inline void ThumbStep(struct ARMCore* cpu) {
    uint32_t opcode = cpu->prefetch[0];
    cpu->prefetch[0] = cpu->prefetch[1];
    cpu->gprs[ARM_PC] += WORD_SIZE_THUMB;
    LOAD_16(cpu->prefetch[1], cpu->gprs[ARM_PC] & cpu->memory.activeMask,
            cpu->memory.activeRegion);
    
    // ADD THIS: Per-instruction callback
    if (cpu->components && cpu->components[0]->instructionCallback) {
        cpu->components[0]->instructionCallback(cpu, opcode, cpu->cycles);
    }
    
    ThumbInstruction instruction = _thumbTable[opcode >> 6];
    instruction(cpu, opcode);
}
```

3. **Add Callback to ARMComponent** (`include/mgba/internal/arm/arm.h`):
```c
struct ARMComponent {
    void (*init)(struct ARMCore* cpu, struct ARMComponent* component);
    void (*deinit)(struct ARMComponent* component);
    
    // ADD THIS:
    void (*instructionCallback)(struct ARMCore* cpu, uint32_t opcode, int32_t cycles);
};
```

4. **Wire to Script System** (`src/gba/gba.c` or similar):
```c
static void _instructionCallbackWrapper(struct ARMCore* cpu, uint32_t opcode, int32_t cycles) {
    struct GBA* gba = (struct GBA*) cpu->master;
    if (gba->scriptContext) {
        // Build callback arguments
        struct mScriptList args;
        mScriptListInit(&args, 3);
        
        struct mScriptValue pc = mSCRIPT_MAKE_S32(cpu->gprs[ARM_PC]);
        struct mScriptValue op = mSCRIPT_MAKE_S32(opcode);
        struct mScriptValue cyc = mSCRIPT_MAKE_S32(cycles);
        
        mScriptListAppend(&args, &pc);
        mScriptListAppend(&args, &op);
        mScriptListAppend(&args, &cyc);
        
        mScriptContextTriggerCallback(gba->scriptContext, "instruction", &args);
        mScriptListDeinit(&args);
    }
}
```

5. **Lua Script Usage**:
```lua
callbacks:add("instruction", function(pc, opcode, cycles)
    -- Called EVERY instruction
    print(string.format("PC=%08x Opcode=%08x Cycles=%d", pc, opcode, cycles))
end)
```

### Pros:
- ✅ **Automatic per-instruction data** - no manual stepping needed
- ✅ **Scriptable** - can process data in Lua without external tools
- ✅ **Real-time** - captures exact execution flow

### Cons:
- ❌ **Multiple files** - need to modify 3-4 source files
- ❌ **Complex changes** - requires understanding component system
- ❌ **Massive performance hit** - Lua callback overhead on EVERY instruction
- ❌ **May break other features** - changes core execution loop

### Performance Impact:
**SEVERE** - Calling Lua interpreter on every instruction would slow emulation by ~100-1000x:
- Normal: ~60 FPS (millions of instructions/second)
- With callback: ~0.1 FPS (thousands of instructions/second)

---

## Option 3: Simple Printf Instrumentation (RECOMMENDED)

### Implementation Difficulty: **TRIVIAL** ⭐

### Required Changes:

**Single line addition** to `/tmp/mgba/src/arm/arm.c`:

```c
static inline void ARMStep(struct ARMCore* cpu) {
    uint32_t opcode = cpu->prefetch[0];
    cpu->prefetch[0] = cpu->prefetch[1];
    cpu->gprs[ARM_PC] += WORD_SIZE_ARM;
    LOAD_32(cpu->prefetch[1], cpu->gprs[ARM_PC] & cpu->memory.activeMask, 
            cpu->memory.activeRegion);
    
    // ADD THIS SINGLE LINE:
    fprintf(stderr, "ARM: PC=%08x Cycles=%d\n", cpu->gprs[ARM_PC] - 8, cpu->cycles);
    
    unsigned condition = opcode >> 28;
    // ... rest of function ...
}
```

Add similar line to `ThumbStep()`:
```c
static inline void ThumbStep(struct ARMCore* cpu) {
    uint32_t opcode = cpu->prefetch[0];
    cpu->prefetch[0] = cpu->prefetch[1];
    cpu->gprs[ARM_PC] += WORD_SIZE_THUMB;
    LOAD_16(cpu->prefetch[1], cpu->gprs[ARM_PC] & cpu->memory.activeMask,
            cpu->memory.activeRegion);
    
    // ADD THIS SINGLE LINE:
    fprintf(stderr, "THUMB: PC=%08x Cycles=%d\n", cpu->gprs[ARM_PC] - 4, cpu->cycles);
    
    ThumbInstruction instruction = _thumbTable[opcode >> 6];
    instruction(cpu, opcode);
}
```

### Build & Run:
```bash
cd /tmp/mgba
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j8

# Run and capture trace
./mgba-qt /path/to/bios.bin 2> /tmp/mgba_exact_trace.log
```

### Pros:
- ✅ **2 lines of code** - minimal change
- ✅ **Exact cycle count** - captures internal cpu->cycles field
- ✅ **Complete trace** - every instruction with PC and cycles
- ✅ **No API complexity** - simple fprintf
- ✅ **Easy to parse** - structured output
- ✅ **Low overhead** - fprintf to stderr is fast

### Cons:
- ❌ **Large log files** - millions of lines for long runs
- ❌ **Not interactive** - can't query live
- ❌ **Requires rebuild** - need to recompile mGBA

### Output Format:
```
ARM: PC=00000000 Cycles=2
ARM: PC=00000004 Cycles=3
ARM: PC=00000008 Cycles=4
THUMB: PC=0000000c Cycles=5
THUMB: PC=0000000e Cycles=6
...
```

### Comparison Script:
```python
# compare_with_mgba.py
with open('/tmp/gba_trace.log') as our, open('/tmp/mgba_exact_trace.log') as ref:
    for i, (our_line, ref_line) in enumerate(zip(our, ref)):
        our_pc, our_cycles = parse_trace(our_line)
        ref_pc, ref_cycles = parse_trace(ref_line)
        
        if our_pc != ref_pc:
            print(f"PC MISMATCH at instruction {i}: ours={our_pc:08x} mgba={ref_pc:08x}")
            break
        
        if our_cycles != ref_cycles:
            print(f"CYCLE MISMATCH at instruction {i}: ours={our_cycles} mgba={ref_cycles}")
```

---

## Recommendation

### For Your Use Case: **Option 3 (Printf Instrumentation)**

**Why:**
1. **Simplest**: 2 lines of code vs 50+ lines
2. **Fastest to implement**: 5 minutes vs 2-4 hours
3. **Most reliable**: Direct access to internal state
4. **Sufficient**: You only need BIOS execution trace (first ~1000 instructions)
5. **Easy comparison**: Simple text file diff/parsing

### Implementation Steps:

1. **Modify mGBA** (5 minutes):
```bash
cd /tmp/mgba
# Edit src/arm/arm.c - add fprintf to ARMStep() and ThumbStep()
vi src/arm/arm.c
```

2. **Build** (10 minutes):
```bash
mkdir build && cd build
cmake ..
make -j8
```

3. **Capture Trace** (1 minute):
```bash
./mgba-qt /path/to/test.gba 2> /tmp/mgba_trace.log
# Let it run for a second, then close
```

4. **Compare** (5 minutes):
```bash
# First 1000 instructions
head -1000 /tmp/gba_memory_trace.log > /tmp/our_trace.txt
head -1000 /tmp/mgba_trace.log > /tmp/ref_trace.txt
diff -y /tmp/our_trace.txt /tmp/ref_trace.txt | less
```

### Alternative: If You Want Per-Instruction without Rebuild

**Option 1 (GDB)** is viable if you're willing to accept slow execution:
- Modify mGBA GDB stub (3 lines)
- Write Python GDB script to single-step and query cycles
- Expect ~1000x slowdown (10 minutes for 1000 instructions)

**But Option 3 is still better** - faster execution, cleaner output, simpler implementation.

---

## Build Complexity Assessment

### mGBA Build Requirements:
```bash
# macOS
brew install cmake sdl2 libpng zlib

# Build
cd /tmp/mgba
mkdir build && cd build
cmake ..
make -j8

# Result: ./mgba-qt executable
```

**Build time**: ~5-10 minutes on modern Mac
**Binary size**: ~10MB
**Dependencies**: Standard (SDL2, PNG, zlib)

### Risk Level: **LOW**
- mGBA is well-maintained, builds cleanly
- Debug builds are tested regularly
- fprintf won't break anything (just adds logging)
- Can easily revert by removing 2 lines

---

## Conclusion

**Recommended approach**: Add 2 `fprintf` statements to mGBA's ARMStep/ThumbStep functions.

- **Time investment**: 20 minutes total
- **Code complexity**: Trivial
- **Reliability**: High - direct access to cycle counter
- **Performance**: Fast enough for trace capture
- **Comparison**: Easy - diff text files

This will give you exact instruction-by-instruction cycle counts to compare against your emulator and validate your timing implementation.
