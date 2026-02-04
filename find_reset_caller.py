#!/usr/bin/env python3
import capstone

with open('assets/roms/sonic.bin', 'rb') as f:
    rom = f.read()

# Search for BL instructions that call 0x08099800 (SoftReset wrapper)
target = 0x08099800

print('Searching for BL calls to 0x08099800...')
for i in range(0, min(len(rom), 0x100000), 2):
    if i + 4 <= len(rom):
        w1 = rom[i] | (rom[i+1] << 8)
        w2 = rom[i+2] | (rom[i+3] << 8)
        if (w1 & 0xF800) == 0xF000 and (w2 & 0xD000) == 0xD000:
            S = (w1 >> 10) & 1
            imm10 = w1 & 0x3FF
            J1 = (w2 >> 13) & 1
            J2 = (w2 >> 11) & 1
            imm11 = w2 & 0x7FF
            I1 = 1 - (J1 ^ S)
            I2 = 1 - (J2 ^ S)
            imm = (S << 24) | (I1 << 23) | (I2 << 22) | (imm10 << 12) | (imm11 << 1)
            if S:
                imm |= 0xFE000000
            pc = 0x08000000 + i + 4
            dest = (pc + imm) & 0xFFFFFFFF
            if dest == target:
                print(f'  BL at 0x{0x08000000+i:08X} -> 0x{dest:08X}')

# Also disassemble around the calls found
md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)

# Check around 0x08096B00 area (where init happens)
print('\n=== ROM entry point area (0x080000C0) ===')
for i in md.disasm(rom[0xC0:0x120], 0x080000C0):
    print(f'0x{i.address:08X}: {i.mnemonic} {i.op_str}')
