# mGBA Trace Collection Guide

This guide explains how to collect detailed instruction and memory traces from mGBA for debugging and comparison with our GBA emulator.

## Prerequisites

- **mGBA build with GDB support**: You need the mGBA SDL build with GDB remote protocol support
  - Location: `/Users/malcolm/mgba-instrumented/build/sdl/mgba`
- **Python 3**: For running the trace collection script
- **ROM file**: e.g., `assets/roms/sonic.bin`
- **BIOS file**: `assets/bios.bin`

## Quick Start

### 1. Start mGBA with GDB Server

Open a terminal and run:

```bash
/Users/malcolm/mgba-instrumented/build/sdl/mgba assets/roms/sonic.bin -b assets/bios.bin -g &
```

**Flags:**
- `-g`: Enable GDB remote protocol server on port 2345
- `-b`: Specify BIOS file
- `&`: Run in background

Wait ~3 seconds for the GDB server to initialize.

### 2. Run the Trace Collection Script

In another terminal:

```bash
python3 tests/mgba_scripts/trace_with_memory.py > /tmp/mgba_memory_trace.log 2>&1
```

**Output:** `/tmp/mgba_memory_trace.log`

### 3. Stop mGBA

```bash
killall mgba
```

## Trace Script Configuration

The trace script (`tests/mgba_scripts/trace_with_memory.py`) can be configured:

```python
GDB_HOST = 'localhost'
GDB_PORT = 2345
OUTPUT_FILE = '/tmp/mgba_memory_trace.log'
MAX_INSTRUCTIONS = 50000  # Change this to collect more/fewer instructions
```

### Monitored Memory Locations

The script automatically monitors these key registers at each instruction:

| Address      | Name          | Size | Description                    |
|--------------|---------------|------|--------------------------------|
| 0x04000000   | DISPCNT       | 4    | Display Control                |
| 0x04000004   | DISPSTAT      | 2    | Display Status                 |
| 0x04000006   | VCOUNT        | 2    | Vertical Counter               |
| 0x04000100   | TM0CNT_L      | 2    | Timer 0 Counter                |
| 0x04000102   | TM0CNT_H      | 2    | Timer 0 Control                |
| 0x04000104   | TM1CNT_L      | 2    | Timer 1 Counter                |
| 0x04000106   | TM1CNT_H      | 2    | Timer 1 Control                |
| 0x040000BA   | DMA0CNT_H     | 2    | DMA 0 Control                  |
| 0x040000C6   | DMA1CNT_H     | 2    | DMA 1 Control                  |
| 0x040000D2   | DMA2CNT_H     | 2    | DMA 2 Control                  |
| 0x040000DE   | DMA3CNT_H     | 2    | DMA 3 Control                  |
| **0x04000200** | **IE**      | 2    | **Interrupt Enable**           |
| **0x04000202** | **IF**      | 2    | **Interrupt Flag**             |
| **0x04000208** | **IME**     | 4    | **Interrupt Master Enable**    |
| 0x04000300   | POSTFLG       | 1    | Post Boot Flag                 |
| 0x03007FFC   | IRQ_HANDLER   | 4    | IRQ Handler Pointer            |
| 0x03007FF8   | IRQ_SP-4      | 4    | IRQ Stack Pointer - 4          |

## Trace File Format

The trace file contains verbose instruction-by-instruction output:

```
======================================================================
Instruction #1
======================================================================
PC: 0x00000000
 r0=0x00000000   r1=0x00000000   r2=0x00000000   r3=0x00000000
 r4=0x00000000   r5=0x00000000   r6=0x00000000   r7=0x00000000
 r8=0x00000000   r9=0x00000000  r10=0x00000000  r11=0x00000000
r12=0x00000000   sp=0x03007f00   lr=0x00000000   pc=0x00000000
cpsr=0x000000d3

[DISPCNT     ] 0x04000000 = 0x00000080
[DISPSTAT    ] 0x04000004 = 0x0000
[VCOUNT      ] 0x04000006 = 0x0000
[IE          ] 0x04000200 = 0x0000
[IF          ] 0x04000202 = 0x0000
[IME         ] 0x04000208 = 0x00000000
...
```

## Analyzing Traces

### Search for Interrupt Register Changes

**Find when IE (Interrupt Enable) changes:**
```bash
grep "^\[IE" /tmp/mgba_memory_trace.log | grep -v "= 0x0000"
```

**Find when IF (Interrupt Flag) changes:**
```bash
grep "^\[IF" /tmp/mgba_memory_trace.log | grep -v "= 0x0000"
```

**Find when IME (Interrupt Master Enable) changes:**
```bash
grep "^\[IME" /tmp/mgba_memory_trace.log | grep -v "= 0x00000000"
```

### Count Total Instructions

```bash
grep -c "^PC:" /tmp/mgba_memory_trace.log
```

### Extract Compact Format

Use the companion script to convert verbose traces to compact format:

```bash
python3 tests/mgba_scripts/extract_mgba_compact.py /tmp/mgba_memory_trace.log > /tmp/mgba_compact.txt
```

## Common Issues

### mGBA Won't Start with `-g` Flag

**Error:** `Unknown option: -g`

**Solution:** Ensure you're using the SDL build, not the Qt GUI build:
```bash
/Users/malcolm/mgba-instrumented/build/sdl/mgba --help
```

Should show: `--gdb, -g` in the help output.

### GDB Connection Timeout

**Error:** Script hangs or times out connecting

**Solution:** 
1. Ensure mGBA started successfully: `ps aux | grep mgba`
2. Check port 2345 is listening: `lsof -i :2345`
3. Increase sleep time between starting mGBA and running script

### Trace File Too Large

**Problem:** Trace file becomes huge (>1GB)

**Solution:** Reduce `MAX_INSTRUCTIONS` in the script:
```python
MAX_INSTRUCTIONS = 10000  # Capture only 10K instructions
```

## Comparing with GBA Emulator Traces

To compare mGBA traces with our emulator:

1. **Collect GBA emulator trace:**
   ```bash
   ./gba_emulator assets/roms/sonic.bin --trace-bios > /tmp/gba_trace.log 2>&1
   ```

2. **Compare instruction-by-instruction:**
   ```bash
   python3 tests/mgba_scripts/compare_traces.py /tmp/gba_trace.log /tmp/mgba_compact.txt
   ```

## Example: Analyzing Interrupt Setup

To understand when Sonic ROM enables interrupts:

```bash
# Start mGBA and collect trace
/Users/malcolm/mgba-instrumented/build/sdl/mgba assets/roms/sonic.bin -b assets/bios.bin -g &
sleep 3
python3 tests/mgba_scripts/trace_with_memory.py > /tmp/sonic_trace.log 2>&1
killall mgba

# Analyze interrupt register changes
echo "=== IE (Interrupt Enable) Changes ==="
grep "^\[IE" /tmp/sonic_trace.log | grep -v "= 0x0000" | head -10

echo "=== IME (Interrupt Master Enable) Changes ==="
grep "^\[IME" /tmp/sonic_trace.log | grep -v "= 0x00000000" | head -10

echo "=== IF (Interrupt Flag) Changes ==="
grep "^\[IF" /tmp/sonic_trace.log | grep -v "= 0x0000" | head -10
```

## Notes

- **GDB Protocol:** The script uses the GDB remote serial protocol to communicate with mGBA
- **Performance:** Tracing is slow due to GDB communication overhead. Each instruction takes ~1ms to capture
- **Accuracy:** mGBA's GDB implementation provides cycle-accurate register and memory state
- **Memory Reads:** The script reads memory locations directly via GDB `m` commands
- **Timeout:** Default timeout is 30 seconds per instruction; increase if mGBA is slow

## References

- [mGBA Documentation](https://mgba.io/docs.html)
- [GDB Remote Serial Protocol](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Remote-Protocol.html)
- Script location: `tests/mgba_scripts/trace_with_memory.py`
