# GBA Emulator Traces

This directory contains instruction-level traces from mGBA and our emulator for debugging and comparison.

## mGBA Traces

### `mgba_sonic_10M_instructions.log.gz`
- **ROM**: Sonic the Hedgehog (GBA)
- **Instructions**: ~10,000,000
- **Size (compressed)**: ~1-2 GB
- **Size (uncompressed)**: 10 GB (290M lines)
- **Collection Date**: October 19, 2025
- **Tool**: mGBA SDL build with GDB server + trace_with_memory.py

#### Key Milestones in This Trace

| Instruction | Event | PC | Description |
|------------|-------|-----|-------------|
| 38,085 | IE Enabled | BIOS | VBlank interrupt configured |
| 198,399 | First VBlank | 0x00001124 | IF flag set, IntrWait waiting |
| 799,570 | IME Enabled | 0x00002D62 | Interrupts globally enabled |
| 799,572 | First IRQ | 0x00000018 | Interrupt handler called |
| 799,586 | IF Cleared | BIOS | Handler clears interrupt flag |
| **7,444,345** | **ROM Entry** | **0x08000000** | **Entered ROM code!** |

#### What This Trace Shows

This trace captures the **complete BIOS → ROM transition** for Sonic, demonstrating:

1. **IntrWait Behavior**: Shows how BIOS IntrWait polls for both IF and IME
2. **Interrupt Timeline**: Complete sequence from IE setup through IME enable to first IRQ
3. **BIOS Services**: Extended BIOS execution (6.6M instructions) before ROM entry
4. **ROM Entry**: Transition from BIOS (0x0000xxxx) to ROM (0x08000000)

This proves that Sonic requires **7.4 million instructions** to complete BIOS initialization
before entering ROM code.

#### How to Use This Trace

**Uncompress** (warning: creates 10GB file):
```bash
gunzip -c traces/mgba_sonic_10M_instructions.log.gz > /tmp/mgba_trace.log
```

**Extract specific instruction range** (without full decompression):
```bash
# Get instructions 7,440,000 to 7,450,000 (around ROM entry)
gunzip -c traces/mgba_sonic_10M_instructions.log.gz | \
  awk '/^Instruction #7440[0-9]/, /^Instruction #7450[0-9]/' > rom_entry_trace.txt
```

**Search for specific events**:
```bash
# Find IME changes
gunzip -c traces/mgba_sonic_10M_instructions.log.gz | grep "IME.*0x04000208"

# Find ROM entry
gunzip -c traces/mgba_sonic_10M_instructions.log.gz | grep -m 1 "PC: 0x08"
```

**Compare with our emulator**:
1. Generate trace from our emulator (same ROM)
2. Compare IE/IF/IME values at key instruction numbers
3. Find first divergence point

#### Trace Format

Each instruction includes:
```
======================================================================
Instruction #N
======================================================================
PC: 0xXXXXXXXX
 r0=0xXXXXXXXX   r1=0xXXXXXXXX   r2=0xXXXXXXXX   r3=0xXXXXXXXX
 r4=0xXXXXXXXX   r5=0xXXXXXXXX   r6=0xXXXXXXXX   r7=0xXXXXXXXX
 r8=0xXXXXXXXX   r9=0xXXXXXXXX  r10=0xXXXXXXXX  r11=0xXXXXXXXX
r12=0xXXXXXXXX   sp=0xXXXXXXXX   lr=0xXXXXXXXX
cpsr=0xXXXXXXXX [N=X Z=X C=X V=X I=X F=X T=X] Mode: XXX

Memory:
  [DISPCNT     ] 0x04000000 = 0xXXXXXXXX
  [DISPSTAT    ] 0x04000004 = 0xXXXX
  [VCOUNT      ] 0x04000006 = 0xXXXX
  [TM0CNT_L    ] 0x04000100 = 0xXXXX
  [TM0CNT_H    ] 0x04000102 = 0xXXXX
  [TM1CNT_L    ] 0x04000104 = 0xXXXX
  [TM1CNT_H    ] 0x04000106 = 0xXXXX
  [DMA0CNT_H   ] 0x040000BA = 0xXXXX
  [DMA1CNT_H   ] 0x040000C6 = 0xXXXX
  [DMA2CNT_H   ] 0x040000D2 = 0xXXXX
  [DMA3CNT_H   ] 0x040000DE = 0xXXXX
  [IE          ] 0x04000200 = 0xXXXX      ← Interrupt Enable
  [IF          ] 0x04000202 = 0xXXXX      ← Interrupt Flag
  [IME         ] 0x04000208 = 0xXXXXXXXX  ← Interrupt Master Enable
  [POSTFLG     ] 0x04000300 = 0xXX
  [IRQ_HANDLER ] 0x03007FFC = 0xXXXXXXXX
  [IRQ_SP-4    ] 0x03007FF8 = 0xXXXXXXXX
```

## Collection Method

See `docs/mgba_trace_collection.md` for detailed instructions on collecting mGBA traces.

**Summary**:
1. Start mGBA with GDB server: `mgba ROM -b BIOS -g &`
2. Wait 3 seconds for GDB initialization
3. Run trace script: `python3 tests/mgba_scripts/trace_with_memory.py`
4. Script writes to `/tmp/mgba_memory_trace.log`

## Analysis Scripts

- **`tests/mgba_scripts/trace_with_memory.py`**: Collect traces via GDB
- **`check_trace_progress.sh`**: Monitor trace collection progress
- Custom Python scripts for analyzing IE/IF/IME changes (see docs)

## References

- **Complete Analysis**: `docs/SONIC_BOOT_COMPLETE_ANALYSIS.md`
- **Interrupt Timeline**: `docs/interupt.md`
- **Collection Guide**: `docs/mgba_trace_collection.md`
