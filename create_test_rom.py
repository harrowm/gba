#!/usr/bin/env python3
"""
Create a simple GamePak ROM with a cache test loop
"""
import struct

# Create our test ARM instructions
# 0x08000000: mov r0, #0xFF         ; Load counter (255 decimal)
# 0x08000004: loop: sub r0, r0, #1  ; Decrement counter  
# 0x08000008: cmp r0, #0            ; Compare with 0
# 0x0800000C: bne loop              ; Branch back if not zero (creates tight loop)
# 0x08000010: b 0x08000010          ; Infinite loop when done
#
# Expected execution:
# 1. mov r0, #0xFF (1 instruction)
# 2. 255 iterations × 3 instructions each = 765 instructions  
# 3. Fall through to infinite branch (execution stops before infinite branch)
# Total expected: 1 + 765 = 766 instructions

instructions = [
    0xE3A000FF,  # mov r0, #0xFF (255 iterations)
    0xE2400001,  # sub r0, r0, #1  
    0xE3500000,  # cmp r0, #0
    0x1AFFFFFC,  # bne -4 (back to sub instruction at 0x08000004)
    0xEAFFFFFE,  # b 0x08000010 (infinite loop)
]

# Create a simple ROM file - only 256 bytes since we only need a few instructions
rom_data = bytearray(256)  # 256 byte ROM

# Write instructions at the beginning
for i, instr in enumerate(instructions):
    offset = i * 4
    rom_data[offset:offset+4] = struct.pack('<I', instr)

# Write the ROM file
with open('assets/roms/test_gamepak.bin', 'wb') as f:
    f.write(rom_data)

print(f"Created test GamePak ROM with {len(instructions)} instructions")
print("Instructions:")
for i, instr in enumerate(instructions):
    addr = 0x08000000 + i * 4
    print(f"  0x{addr:08X}: 0x{instr:08X}")
