#!/usr/bin/env python3
from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB

rom_path = 'assets/roms/sonic.bin'
with open(rom_path, 'rb') as f:
    data = f.read()

# Look at the code around 0x08011E00 to see how R1 gets 0x03007EA0
# The STRB at 0x08011F86 uses R1 as base
start_offset = 0x11E00  # Start well before
end_offset = 0x12000    # End a bit after
rom_data = data[start_offset:end_offset]

md = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
print(f"Disassembly from 0x{0x08000000 + start_offset:08X} to 0x{0x08000000 + end_offset:08X}:")
print("Looking for how R1 gets value 0x03007EA0")
print("=" * 70)

for i in md.disasm(rom_data, 0x08000000 + start_offset):
    pc = i.address
    # Highlight important instructions
    marker = ""
    if "r1" in i.op_str.lower() or "r1" in i.mnemonic.lower():
        if "ldr" in i.mnemonic.lower() or "mov" in i.mnemonic.lower() or "add" in i.mnemonic.lower():
            marker = " <<< R1 modified"
    if pc in [0x08011F86, 0x08011F8A]:
        marker = " <<< STRB TO 0x03007EA0 area"
    print(f"{pc:08X}: {i.mnemonic:8s} {i.op_str:20s}{marker}")
