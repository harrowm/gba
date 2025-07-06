#!/usr/bin/env python3

def encode_thumb_sub_register(rd, rs, rn):
    """Encode SUB Rd, Rs, Rn instruction"""
    # Format: 000 11 0 1 rn rs rd (bits 15-9: 0001101, bit 8: 1 for SUB)
    opcode = 0x1A00  # Base for SUB register (000 11 0 1)
    opcode |= (rn & 0x7) << 6   # Rn in bits 8:6
    opcode |= (rs & 0x7) << 3   # Rs in bits 5:3
    opcode |= (rd & 0x7)        # Rd in bits 2:0
    return opcode

def encode_thumb_sub_offset(rd, rs, offset):
    """Encode SUB Rd, Rs, #offset instruction"""
    # Format: 000 11 1 1 offset rs rd (bits 15-9: 0001111)
    opcode = 0x1E00  # Base for SUB offset (000 11 1 1)
    opcode |= (offset & 0x7) << 6   # offset in bits 8:6
    opcode |= (rs & 0x7) << 3       # Rs in bits 5:3
    opcode |= (rd & 0x7)            # Rd in bits 2:0
    return opcode

# Test cases for SUB_REGISTER
print("SUB_REGISTER test cases:")
print(f"SUB R0, R1, R2: 0x{encode_thumb_sub_register(0, 1, 2):04X}")
print(f"SUB R1, R0, R3: 0x{encode_thumb_sub_register(1, 0, 3):04X}")
print(f"SUB R2, R3, R4: 0x{encode_thumb_sub_register(2, 3, 4):04X}")
print(f"SUB R3, R5, R6: 0x{encode_thumb_sub_register(3, 5, 6):04X}")
print(f"SUB R4, R7, R0: 0x{encode_thumb_sub_register(4, 7, 0):04X}")
print(f"SUB R0, R1, R2: 0x{encode_thumb_sub_register(0, 1, 2):04X}")
print(f"SUB R3, R3, R4: 0x{encode_thumb_sub_register(3, 3, 4):04X}")

print("\nSUB_OFFSET test cases:")
print(f"SUB R0, R1, #1: 0x{encode_thumb_sub_offset(0, 1, 1):04X}")
print(f"SUB R1, R2, #3: 0x{encode_thumb_sub_offset(1, 2, 3):04X}")
print(f"SUB R2, R3, #1: 0x{encode_thumb_sub_offset(2, 3, 1):04X}")
print(f"SUB R3, R4, #7: 0x{encode_thumb_sub_offset(3, 4, 7):04X}")
print(f"SUB R4, R5, #5: 0x{encode_thumb_sub_offset(4, 5, 5):04X}")
print(f"SUB R5, R6, #7: 0x{encode_thumb_sub_offset(5, 6, 7):04X}")
print(f"SUB R6, R7, #0: 0x{encode_thumb_sub_offset(6, 7, 0):04X}")
print(f"SUB R0, R0, #4: 0x{encode_thumb_sub_offset(0, 0, 4):04X}")
print(f"SUB R0, R1, #0: 0x{encode_thumb_sub_offset(0, 1, 0):04X}")
print(f"SUB R7, R2, #6: 0x{encode_thumb_sub_offset(7, 2, 6):04X}")
print(f"SUB R2, R3, #1: 0x{encode_thumb_sub_offset(2, 3, 1):04X}")
print(f"SUB R3, R4, #7: 0x{encode_thumb_sub_offset(3, 4, 7):04X}")
