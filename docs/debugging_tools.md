# Debugging Tools

This document describes the tools available for debugging the GBA emulator.

## Python Virtual Environment Setup

The project uses a Python virtual environment for debugging tools, primarily for disassembly with Capstone.

### Initial Setup

```bash
# Create virtual environment (first time only)
python3 -m venv .venv

# Activate the virtual environment
source .venv/bin/activate

# Install Capstone disassembler
pip install capstone
```

### Using Python Tools

You can run Python scripts in two ways:

**Option 1: Activate the virtual environment**
```bash
source .venv/bin/activate
python3 your_script.py
deactivate  # when done
```

**Option 2: Use the venv Python directly (recommended for one-liners)**
```bash
.venv/bin/python3 your_script.py
```

## Capstone Disassembler

Capstone is used to disassemble ARM and THUMB instructions for debugging.

### Basic Usage

**Disassemble ARM instructions:**
```bash
.venv/bin/python3 << 'EOF'
from capstone import *

# Read BIOS or ROM
with open('assets/bios.bin', 'rb') as f:
    bios = f.read()

# Disassemble ARM instructions
md = Cs(CS_ARCH_ARM, CS_MODE_ARM)
pc_start = 0x118
code = bios[pc_start:pc_start+16]

for insn in md.disasm(code, pc_start):
    print(f"0x{insn.address:08x}: {insn.mnemonic:8s} {insn.op_str}")
EOF
```

**Disassemble THUMB instructions:**
```bash
.venv/bin/python3 << 'EOF'
from capstone import *

# Read ROM
with open('assets/roms/sonic.bin', 'rb') as f:
    rom = f.read()

# Disassemble THUMB instructions
md = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
pc_start = 0x808
code = rom[pc_start:pc_start+16]

for insn in md.disasm(code, pc_start):
    print(f"0x{insn.address:08x}: {insn.mnemonic:8s} {insn.op_str}")
EOF
```

### Disassembling from Memory Addresses

**BIOS addresses (0x00000000 - 0x00003FFF):**
- File offset = PC address
- Example: PC 0x118 → offset 0x118 in `assets/bios.bin`

**ROM addresses (0x08000000 - 0x09FFFFFF):**
- File offset = PC - 0x08000000
- Example: PC 0x08000808 → offset 0x808 in `assets/roms/sonic.bin`

**Working RAM (0x02000000 - 0x0203FFFF) or Internal RAM (0x03000000 - 0x03007FFF):**
- Must capture from emulator memory dump
- Not directly available in files

### Common Patterns

**Check instruction encoding:**
```python
# Print raw bytes
code = rom[offset:offset+16]
print(f"Code bytes at 0x{pc:08x}:", code.hex())

# ARM instructions are 4 bytes little-endian
instr = int.from_bytes(code[0:4], 'little')
print(f"Instruction: 0x{instr:08X}")

# THUMB instructions are 2 bytes little-endian
instr = int.from_bytes(code[0:2], 'little')
print(f"Instruction: 0x{instr:04X}")
```

**Disassemble around a divergence point:**
```python
from capstone import *

with open('assets/roms/sonic.bin', 'rb') as f:
    rom = f.read()

md = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
pc = 0x808
offset = pc  # ROM offset

# Disassemble 10 instructions before and after
code = rom[max(0, offset-20):offset+20]
start_pc = max(0, pc-20)

for insn in md.disasm(code, start_pc):
    marker = ">>>" if insn.address == pc else "   "
    print(f"{marker} 0x{insn.address:08x}: {insn.mnemonic:8s} {insn.op_str}")
```

## Trace Comparison Scripts

### find_divergence.py

Binary search to find first divergence between mGBA and our emulator traces.

```bash
# Compare traces
python3 tests/find_divergence.py /tmp/mgba_trace.log /tmp/gba_trace.log 50000

# Output shows first divergence:
# Found divergence at instruction 31
# mGBA line: PC:0000011C ...
# GBA  line: PC:000000A0 ...
```

### trace_compact.py

Collect compact traces from mGBA via GDB for comparison.

```bash
# Start mGBA with GDB server
/path/to/mgba -g assets/roms/sonic.bin &

# Collect trace (runs in background)
python3 tests/trace_compact.py 200000 /tmp/mgba_trace.log

# Kill mGBA when done
killall mgba
```

## Trace Format

Both emulators output traces in this format:

```
PC:XXXXXXXX R00:XXXXXXXX R01:XXXXXXXX ... R15:XXXXXXXX CPSR:XXXXXXXX | IE:XXXX IF:XXXX IME:XXXXXXXX
```

One line per instruction, making binary search easy.

## Memory Map Quick Reference

When disassembling, remember the GBA memory map:

- `0x00000000 - 0x00003FFF`: BIOS (16 KB) → `assets/bios.bin`
- `0x08000000 - 0x09FFFFFF`: ROM (max 32 MB) → `assets/roms/*.bin`
- `0x02000000 - 0x0203FFFF`: Working RAM (256 KB)
- `0x03000000 - 0x03007FFF`: Internal RAM (32 KB)
- `0x04000000 - 0x040003FE`: I/O Registers
- `0x05000000 - 0x050003FF`: Palette RAM (1 KB)
- `0x06000000 - 0x06017FFF`: VRAM (96 KB)
- `0x07000000 - 0x070003FF`: OAM (1 KB)

## Tips

1. **Always use `.venv/bin/python3`** for one-off commands to avoid activating/deactivating
2. **THUMB mode**: Most game code runs in THUMB mode (16-bit instructions)
3. **ARM mode**: BIOS and some system code uses ARM mode (32-bit instructions)
4. **PC+8 in ARM, PC+4 in THUMB**: Remember the pipeline offset when calculating addresses
5. **Little-endian**: GBA uses little-endian byte order
