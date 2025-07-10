#!/usr/bin/env python3
"""
Create a simple GamePak ROM to test SUB instruction in isolation
"""
import struct

# Create a simple test for SUB instruction
# 0x08000000: mov r0, #0x05        ; Load 5 into R0
# 0x08000004: sub r0, r0, #1       ; Subtract 1 from R0 (should be 4)
# 0x08000008: sub r0, r0, #1       ; Subtract 1 from R0 (should be 3)
# 0x0800000C: sub r0, r0, #1       ; Subtract 1 from R0 (should be 2)
# 0x08000010: b 0x08000010         ; Infinite loop

instructions = [
    0xE3A00005,  # mov r0, #5
    0xE2400001,  # sub r0, r0, #1  
    0xE2400001,  # sub r0, r0, #1  
    0xE2400001,  # sub r0, r0, #1  
    0xEAFFFFFE,  # b 0x08000010 (infinite loop)
]

# Create a simple ROM file - only 256 bytes since we only need a few instructions
rom_data = bytearray(256)  # 256 byte ROM

# Write instructions at the beginning
for i, instr in enumerate(instructions):
    offset = i * 4
    rom_data[offset:offset+4] = struct.pack('<I', instr)

# Write the ROM file
with open('assets/roms/test_sub.bin', 'wb') as f:
    f.write(rom_data)

print(f"Created SUB test ROM with {len(instructions)} instructions")
print("Instructions:")
for i, instr in enumerate(instructions):
    addr = 0x08000000 + i * 4
    print(f"  0x{addr:08X}: 0x{instr:08X}")
