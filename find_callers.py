#!/usr/bin/env python3
from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB

rom_path = 'assets/roms/sonic.bin'
with open(rom_path, 'rb') as f:
    f.seek(0)
    data = f.read()

# Search for BL instructions that might call into the function containing 0x08006500
md = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
count = 0
for i in md.disasm(data, 0x08000000):
    if i.mnemonic == 'bl' and '0x8006' in i.op_str:
        print(f'{i.address:08X}: {i.mnemonic:8s} {i.op_str}')
        count += 1
        if count > 30:
            break
