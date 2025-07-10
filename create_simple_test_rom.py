#!/usr/bin/env python3
"""
Create a very simple test ROM to isolate the SUB instruction issue
"""
import struct

# Create simple test ARM instructions
# 0x08000000: mov r0, #0x05         ; Load small counter (5)
# 0x08000004: sub r0, r0, #1        ; Decrement counter  
# 0x08000008: sub r0, r0, #1        ; Decrement counter again
# 0x0800000C: sub r0, r0, #1        ; Decrement counter again
# 0x08000010: b 0x08000010          ; Infinite loop

instructions = [
    0xE3A00005,  # mov r0, #0x05 (5)
    0xE2400001,  # sub r0, r0, #1  
    0xE2400001,  # sub r0, r0, #1
    0xE2400001,  # sub r0, r0, #1
    0xEAFFFFFE,  # b 0x08000010 (infinite loop)
]

# Create a simple ROM file - 256 bytes
rom_data = bytearray(256)

# Write instructions at the beginning
for i, instr in enumerate(instructions):
    offset = i * 4
    rom_data[offset:offset+4] = struct.pack('<I', instr)

# Write the ROM file
with open('assets/roms/simple_test.bin', 'wb') as f:
    f.write(rom_data)

print(f"Created simple test ROM with {len(instructions)} instructions")
print("Instructions:")
for i, instr in enumerate(instructions):
    addr = 0x08000000 + i * 4
    print(f"  0x{addr:08X}: 0x{instr:08X}")
