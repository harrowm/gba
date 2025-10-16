# Final Recommendation: mGBA Cycle Extraction

## Research Summary

### What We Found

1. **GDB Remote Protocol**: ❌ Cannot access `cpu->cycles` - only supports standard commands (registers, memory, step)

2. **Lua Scripting API**: ❌ No per-instruction callback
   - Available callbacks: `frame`, `keysRead`, `start`, `stop`, `reset`, `crashed`
   - Can access `emu.memory`, `emu:read8()`, etc.
   - But **NO** way to hook every instruction execution
   - `memory.cpuCycles()` might exist but can only be called from frame callback (too coarse)

3. **Source Code Instrumentation**: ✅ **ONLY RELIABLE METHOD**

## Final Recommendation: Instrument mGBA Source

Since we need instruction-by-instruction cycle counts and neither GDB nor Lua provides this, we must add logging to mGBA's source code.

### Implementation Plan

**Step 1**: Find the right location in mGBA source

The CPU execution happens in `src/arm/arm.c`. We need to add logging after each instruction executes.

**Step 2**: Add cycle logging

```c
// In src/arm/arm.c, in the main execution function
// After each instruction executes:

fprintf(stderr, "INSTR,%d,%08X\n", cpu->cycles, cpu->gprs[ARM_PC]);
fflush(stderr);  // Important: flush immediately
```

**Step 3**: Build mGBA

```bash
cd /tmp/mgba
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j4
```

**Step 4**: Run and capture output

```bash
./mgba -g ../path/to/bios.bin 2> /tmp/mgba_cycle_log.txt
# Run for a few seconds, then Ctrl+C
```

**Step 5**: Parse output

```python
# Parse /tmp/mgba_cycle_log.txt
for line in open('/tmp/mgba_cycle_log.txt'):
    if line.startswith('INSTR'):
        _, cycles, pc = line.strip().split(',')
        print(f"Cycle {cycles}: PC=0x{pc}")
```

### Alternative: Use existing comparison method

Since instrumentation requires rebuilding mGBA, we could instead:

1. **Accept that we can't get exact mGBA cycles**
2. **Use GBATEK/ARM documentation** as ground truth for expected cycles
3. **Focus on fixing our VCOUNT timing** (currently 8 cycles late)
4. **Test with known instruction patterns** to validate our timing

## My Recommendation

**Skip mGBA instrumentation** and instead:

1. ✅ **Fix VCOUNT timing** - investigate why it's 8 cycles late
2. ✅ **Validate against GBATEK docs** - check each instruction type matches spec
3. ✅ **Test with real ROMs** - if games run correctly, timing is good enough

The 8-cycle discrepancy with VCOUNT is more important than perfect mGBA matching. Let's focus on that!

## Next Steps

Shall we:
- **A**: Investigate VCOUNT timing (why 8 cycles late)?
- **B**: Build instrumented mGBA (30 min effort)?
- **C**: Validate our timing against GBATEK specs?

I recommend **Option A** - fix the VCOUNT timing issue first.
