#!/usr/bin/env python3
# Read BIOS
with open('assets/bios.bin', 'rb') as f:
    bios = f.read()

# Search for 0x08000000 as a constant in the BIOS
target = (0x08000000).to_bytes(4, 'little')
for i in range(0, len(bios), 4):
    if bios[i:i+4] == target:
        print(f'Found 0x08000000 at offset 0x{i:04X}')
