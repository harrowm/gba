# 10M Instruction mGBA Trace Collection - In Progress

## Goal
Collect 10,000,000 instructions from Sonic ROM boot sequence to find:
1. When ROM code execution begins (PC >= 0x08000000)
2. Complete interrupt initialization timeline in ROM
3. When game actually starts running

## Progress
- **Status**: Running in background
- **Target**: 10,000,000 instructions
- **Estimated time**: 2-3 hours
- **Output file**: `/tmp/mgba_memory_trace.log`

## What We Know So Far (from 1M instruction trace)
- Instruction 38,085: IE=0x0001 (VBlank enabled)
- Instruction 198,399: IF=0x0001 (First VBlank occurs)
- Instruction 799,570: IME=0x00000001 (IME enabled)
- Instruction 799,572: PC=0x00000018 (First IRQ!)
- Instruction 1,000,000: Still in BIOS

## What to Look For
1. **ROM Entry**: First instruction with PC >= 0x08000000
2. **ROM IME Changes**: When ROM code enables/disables interrupts
3. **ROM IF Pattern**: How ROM handles interrupts differently than BIOS
4. **Game Start**: When Sonic game logic begins

## Monitoring Commands
```bash
# Check progress
./check_trace_progress.sh

# Monitor continuously (updates every 30 seconds)
watch -n 30 ./check_trace_progress.sh

# Check instruction count
grep -c "^Instruction #" /tmp/mgba_memory_trace.log

# Find ROM entry (when it happens)
grep "PC: 0x08" /tmp/mgba_memory_trace.log | head -1

# Check if trace is still running
ps aux | grep trace_with_memory.py
```

## Analysis Script (run after completion)
See `/tmp/interrupt_timeline_summary.txt` for current findings.

## File Sizes
- 1M instructions ≈ 150 MB
- 10M instructions ≈ 1.5 GB (estimated)

## Next Steps (After Completion)
1. Analyze when ROM code begins
2. Document ROM interrupt initialization
3. Compare ROM vs BIOS interrupt handling
4. Save trace as reference data
