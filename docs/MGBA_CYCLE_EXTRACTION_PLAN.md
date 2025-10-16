# Plan: Extract Cycle Count from mGBA via GDB

## Objective
Get the actual cycle counter value from mGBA's internal state on each instruction step, so we can compare exact cycle costs with our emulator.

## mGBA Internal Structure Research

### Step 1: Find the Cycle Counter Variable
mGBA is open source. The cycle counter is likely stored in one of these structures:

```c
// Common patterns in emulators:
struct ARMCore {
    int32_t cycles;        // Current cycle count
    uint32_t nextEvent;    // Cycles until next event
    // ... other fields
};

struct GBA {
    struct ARMCore* cpu;
    int32_t cpuBlocked;    // Cycles CPU is blocked
    // ... other fields
};
```

**Research needed:**
1. Clone mGBA source: `git clone https://github.com/mgba-emu/mgba.git`
2. Find cycle counter variable:
   - Search for: `cycles`, `cpuCycles`, `totalCycles`
   - Check: `src/arm/arm.h`, `src/gba/gba.h`, `src/core/timing.h`
3. Identify the exact variable name and structure path

### Step 2: Access Variable via GDB

Once we know the variable (e.g., `cpu->cycles`), we can access it in GDB:

```python
# In our Python GDB script:
# Method 1: If it's a global variable
resp = send_gdb_command(sock, "x/1xw &cpu_cycles")  # Read as 32-bit hex

# Method 2: If it's in a struct
# First, get the struct address (might be in a register or global)
resp = send_gdb_command(sock, "p &gba")  # Get GBA struct address
addr = parse_address(resp)

# Then read the field at offset
# If cycles is at offset 0x10 in the struct:
resp = send_gdb_command(sock, f"x/1xw {addr + 0x10}")
```

### Step 3: Print on Each Step

Modify our tracing script to:
1. Step one instruction: `s` (step)
2. Read cycle counter: `x/1xw <address>`
3. Read registers: `g` (get all registers)
4. Print to trace file

## Detailed Implementation Plan

### Phase 1: Discover mGBA Cycle Variable (MANUAL)

**Task 1a**: Clone and search mGBA source
```bash
cd /tmp
git clone https://github.com/mgba-emu/mgba.git --depth 1
cd mgba

# Search for cycle counter
grep -r "cycles" src/arm/ src/gba/ | grep -E "int|uint|i32|u32"
grep -r "totalCycles" src/
grep -r "cpuCycles" src/
```

**Task 1b**: Identify the exact variable path
Look for patterns like:
```c
struct mCore {
    struct mTiming timing;
    // ...
};

struct mTiming {
    int32_t masterCycles;  // ← This might be it!
    // ...
};
```

**Task 1c**: Find how to access it in debugger
- Is it a global variable?
- Is it part of a struct that's accessible?
- What's the symbol name in the compiled binary?

### Phase 2: Test GDB Access (MANUAL)

**Task 2a**: Start mGBA with GDB server
```bash
mgba -g assets/bios.bin
# GDB server starts on port 2345
```

**Task 2b**: Connect with GDB manually to test
```bash
# In another terminal:
gdb
(gdb) target remote localhost:2345
(gdb) info variables cycles     # Find cycle-related symbols
(gdb) info variables timing
(gdb) print cpu->cycles          # Try different variable paths
(gdb) print gba.timing.masterCycles
# Find what works!
```

**Task 2c**: Once found, test reading it
```bash
(gdb) x/1xw <address>   # Read as hex word
(gdb) step             # Step one instruction
(gdb) x/1xw <address>   # Read again - should have increased
```

### Phase 3: Automate with Python Script

**Task 3a**: Create enhanced tracing script
```python
#!/usr/bin/env python3
"""
mGBA Cycle-Accurate Trace via GDB
Captures cycle count on each instruction
"""

# Key additions:
CYCLE_VARIABLE_ADDRESS = 0xXXXXXXXX  # Found in Phase 2
# OR
CYCLE_VARIABLE_SYMBOL = "gba.timing.masterCycles"  # If accessible by name

def read_cycle_counter(sock):
    # Method 1: Direct memory read (faster)
    cmd = f"x/1xw {CYCLE_VARIABLE_ADDRESS}"
    resp = send_gdb_command(sock, cmd)
    return parse_hex_value(resp)
    
    # Method 2: Symbol read (if supported)
    cmd = f"p {CYCLE_VARIABLE_SYMBOL}"
    resp = send_gdb_command(sock, cmd)
    return parse_integer(resp)

def trace_with_cycles():
    for i in range(MAX_INSTRUCTIONS):
        # Read cycle counter BEFORE step
        cycles_before = read_cycle_counter(sock)
        
        # Step one instruction
        send_gdb_command(sock, "s")
        
        # Read cycle counter AFTER step
        cycles_after = read_cycle_counter(sock)
        
        # Read CPU state
        registers = read_registers(sock)
        
        # Write to trace
        write_instruction(i+1, registers, cycles_before, cycles_after)
```

### Phase 4: Generate Comparable Traces

**Task 4a**: Run mGBA with cycle tracing
```bash
python3 trace_mgba_with_cycles.py > /tmp/mgba_cycle_trace.log
```

**Task 4b**: Run our emulator with tracing
```bash
timeout 2 ./gba_emulator assets/bios.bin --trace-memory
```

**Task 4c**: Compare instruction-by-instruction
```python
# compare_exact_cycles.py
for instr_num in range(1, 1000):
    our_cost = our_trace[instr_num].cycles - our_trace[instr_num-1].cycles
    mgba_cost = mgba_trace[instr_num].cycles - mgba_trace[instr_num-1].cycles
    
    if our_cost != mgba_cost:
        print(f"Instruction #{instr_num}: Our={our_cost}, mGBA={mgba_cost}, PC={pc}")
```

## Alternative: Instrument mGBA Source (If GDB Access Fails)

If we can't access the cycle counter via GDB, we can:

**Option B1**: Add printf to mGBA source
```c
// In src/arm/arm.c or wherever CPU executes:
void ARMRun(struct ARMCore* cpu) {
    printf("CYCLE:%d\n", cpu->cycles);  // Add this
    // ... execute instruction
}
```

Then rebuild mGBA and run with output redirection.

**Option B2**: Use mGBA's built-in logging
mGBA has logging features - check if it can log cycle counts:
```bash
mgba --log-level=DEBUG assets/bios.bin 2>&1 | grep -i cycle
```

## Expected Output Format

After Phase 3, we should have:
```
/tmp/mgba_cycle_trace.log:
Instruction #1 (Cycle: 0)
PC: 0x00000000
...

Instruction #2 (Cycle: 3)
PC: 0x00000068
...

Instruction #3 (Cycle: 4)
PC: 0x0000006C
...
```

Same format as our trace, enabling direct comparison!

## Next Steps - What Should We Do First?

1. **Start with Phase 1**: Research mGBA source to find cycle variable
2. **Test Phase 2 manually**: Use GDB to find if we can access it
3. **If successful**: Implement Phase 3 Python automation
4. **If blocked**: Fall back to Option B (instrument source)

Which phase would you like me to start with?
