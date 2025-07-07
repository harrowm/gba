#!/usr/bin/env python3

def encode_thumb_immediate_offset(B, L, offset5, rb, rd):
    """Encode load/store with immediate offset instruction (Format 9)
    
    Format: 011 B L offset5 rb rd
    - Bits 15-13: 011 (0x3)
    - Bit 12: B (Byte=1, Word=0)
    - Bit 11: L (Load=1, Store=0)
    - Bits 10-6: offset5 (immediate offset 0-31)
    - Bits 5-3: rb (base register 0-7)
    - Bits 2-0: rd (destination/source register 0-7)
    
    For word operations: Effective address = rb + (offset5 * 4)
    For byte operations: Effective address = rb + offset5
    """
    if rd < 0 or rd > 7:
        raise ValueError(f"rd must be 0-7, got {rd}")
    if rb < 0 or rb > 7:
        raise ValueError(f"rb must be 0-7, got {rb}")
    if offset5 < 0 or offset5 > 31:
        raise ValueError(f"offset5 must be 0-31, got {offset5}")
    if L not in [0, 1]:
        raise ValueError(f"L must be 0 or 1, got {L}")
    if B not in [0, 1]:
        raise ValueError(f"B must be 0 or 1, got {B}")
    
    opcode = 0x6000  # Base for immediate offset (011 0 0 00000 000 000)
    opcode |= (B & 0x1) << 12       # B bit in bit 12
    opcode |= (L & 0x1) << 11       # L bit in bit 11
    opcode |= (offset5 & 0x1F) << 6 # offset5 in bits 10:6
    opcode |= (rb & 0x7) << 3       # rb in bits 5:3
    opcode |= (rd & 0x7)            # rd in bits 2:0
    return opcode

def encode_str_immediate_offset(rd, rb, word_offset):
    """Encode STR Rd, [Rb, #offset] instruction (word offset 0-31, byte offset 0-124)"""
    if word_offset < 0 or word_offset > 31:
        raise ValueError(f"word_offset must be 0-31, got {word_offset}")
    return encode_thumb_immediate_offset(0, 0, word_offset, rb, rd)

def encode_ldr_immediate_offset(rd, rb, word_offset):
    """Encode LDR Rd, [Rb, #offset] instruction (word offset 0-31, byte offset 0-124)"""
    if word_offset < 0 or word_offset > 31:
        raise ValueError(f"word_offset must be 0-31, got {word_offset}")
    return encode_thumb_immediate_offset(0, 1, word_offset, rb, rd)

def encode_strb_immediate_offset(rd, rb, byte_offset):
    """Encode STRB Rd, [Rb, #offset] instruction (byte offset 0-31)"""
    if byte_offset < 0 or byte_offset > 31:
        raise ValueError(f"byte_offset must be 0-31, got {byte_offset}")
    return encode_thumb_immediate_offset(1, 0, byte_offset, rb, rd)

def encode_ldrb_immediate_offset(rd, rb, byte_offset):
    """Encode LDRB Rd, [Rb, #offset] instruction (byte offset 0-31)"""
    if byte_offset < 0 or byte_offset > 31:
        raise ValueError(f"byte_offset must be 0-31, got {byte_offset}")
    return encode_thumb_immediate_offset(1, 1, byte_offset, rb, rd)

def verify_encoding(instruction, expected_opcode, rd, rb, offset):
    """Verify that our encoding matches expected opcode"""
    if instruction == "STR":
        actual_opcode = encode_str_immediate_offset(rd, rb, offset)
    elif instruction == "LDR":
        actual_opcode = encode_ldr_immediate_offset(rd, rb, offset)
    elif instruction == "STRB":
        actual_opcode = encode_strb_immediate_offset(rd, rb, offset)
    elif instruction == "LDRB":
        actual_opcode = encode_ldrb_immediate_offset(rd, rb, offset)
    else:
        raise ValueError(f"Unknown instruction: {instruction}")
    
    print(f"{instruction:4s} R{rd}, [R{rb}, #{offset}]: Expected 0x{expected_opcode:04X}, Got 0x{actual_opcode:04X} {'✓' if expected_opcode == actual_opcode else '✗'}")
    return expected_opcode == actual_opcode

print("=== ARM Thumb Format 9 Instruction Encodings ===\n")

print("STR Instructions (Store Word, offset in words):")
for rd in range(3):
    for rb in range(3):
        for offset in [0, 1, 2, 5, 10, 15, 31]:
            byte_offset = offset * 4
            print(f"  STR R{rd}, [R{rb}, #{byte_offset:3d}] = 0x{encode_str_immediate_offset(rd, rb, offset):04X} (word_offset={offset})")

print("\nLDR Instructions (Load Word, offset in words):")
for rd in range(3):
    for rb in range(3):
        for offset in [0, 1, 2]:
            byte_offset = offset * 4
            print(f"  LDR R{rd}, [R{rb}, #{byte_offset:2d}] = 0x{encode_ldr_immediate_offset(rd, rb, offset):04X} (word_offset={offset})")

print("\nSTRB Instructions (Store Byte, offset in bytes):")
for rd in range(3):
    for rb in range(3):
        for offset in [0, 1, 2]:
            print(f"  STRB R{rd}, [R{rb}, #{offset}] = 0x{encode_strb_immediate_offset(rd, rb, offset):04X}")

print("\nLDRB Instructions (Load Byte, offset in bytes):")
for rd in range(3):
    for rb in range(3):
        for offset in [0, 1, 2]:
            print(f"  LDRB R{rd}, [R{rb}, #{offset}] = 0x{encode_ldrb_immediate_offset(rd, rb, offset):04X}")

print("\n=== Encoding Pattern Verification ===\n")

print("Base patterns:")
print(f"STR  base: 0x{encode_str_immediate_offset(0, 0, 0):04X} = 0110000000000000")
print(f"LDR  base: 0x{encode_ldr_immediate_offset(0, 0, 0):04X} = 0110100000000000")
print(f"STRB base: 0x{encode_strb_immediate_offset(0, 0, 0):04X} = 0111000000000000")
print(f"LDRB base: 0x{encode_ldrb_immediate_offset(0, 0, 0):04X} = 0111100000000000")

print("\nL and B Bit Effects:")
print(f"STR  R0, [R0, #0]: 0x{encode_str_immediate_offset(0, 0, 0):04X} (L=0, B=0)")
print(f"LDR  R0, [R0, #0]: 0x{encode_ldr_immediate_offset(0, 0, 0):04X} (L=1, B=0)")
print(f"STRB R0, [R0, #0]: 0x{encode_strb_immediate_offset(0, 0, 0):04X} (L=0, B=1)")
print(f"LDRB R0, [R0, #0]: 0x{encode_ldrb_immediate_offset(0, 0, 0):04X} (L=1, B=1)")

print("\nRegister Field Effects:")
print("Rd field (bits 2:0):")
for rd in range(8):
    print(f"  STR R{rd}, [R0, #0]: 0x{encode_str_immediate_offset(rd, 0, 0):04X}")

print("Rb field (bits 5:3):")
for rb in range(8):
    print(f"  STR R0, [R{rb}, #0]: 0x{encode_str_immediate_offset(0, rb, 0):04X}")

print("\nOffset Field Effects (STR word):")
test_offsets = [0, 1, 2, 4, 8, 16, 31]
for offset in test_offsets:
    byte_offset = offset * 4
    print(f"  STR R0, [R0, #{byte_offset:3d}]: 0x{encode_str_immediate_offset(0, 0, offset):04X} (offset5={offset})")

print("\nOffset Field Effects (STRB byte):")
for offset in test_offsets:
    print(f"  STRB R0, [R0, #{offset:2d}]: 0x{encode_strb_immediate_offset(0, 0, offset):04X} (offset5={offset})")

print("\n=== Edge Cases ===\n")

print("Minimum values:")
print(f"STR R0, [R0, #0]:    0x{encode_str_immediate_offset(0, 0, 0):04X}")
print(f"STRB R0, [R0, #0]:   0x{encode_strb_immediate_offset(0, 0, 0):04X}")
print(f"LDR R0, [R0, #0]:    0x{encode_ldr_immediate_offset(0, 0, 0):04X}")
print(f"LDRB R0, [R0, #0]:   0x{encode_ldrb_immediate_offset(0, 0, 0):04X}")

print("\nMaximum values:")
print(f"STR R7, [R7, #124]:  0x{encode_str_immediate_offset(7, 7, 31):04X} (word offset 31)")
print(f"STRB R7, [R7, #31]:  0x{encode_strb_immediate_offset(7, 7, 31):04X} (byte offset 31)")
print(f"LDR R7, [R7, #124]:  0x{encode_ldr_immediate_offset(7, 7, 31):04X} (word offset 31)")
print(f"LDRB R7, [R7, #31]:  0x{encode_ldrb_immediate_offset(7, 7, 31):04X} (byte offset 31)")

print("\nMixed combinations:")
print(f"STR R3, [R1, #20]:   0x{encode_str_immediate_offset(3, 1, 5):04X} (word offset 5)")
print(f"LDRB R2, [R6, #15]:  0x{encode_ldrb_immediate_offset(2, 6, 15):04X} (byte offset 15)")

print("\n=== CPU Implementation Mapping ===\n")

print("Expected opcode ranges from CPU implementation:")
print("0x60-0x67: STR word immediate")
print("0x68-0x6F: LDR word immediate")
print("0x70-0x77: STRB byte immediate")
print("0x78-0x7F: LDRB byte immediate")

# Verify specific opcodes match CPU implementation ranges
test_cases = [
    ("STR", 0, 0, 0),   # Should be 0x6000
    ("LDR", 0, 0, 0),   # Should be 0x6800
    ("STRB", 0, 0, 0),  # Should be 0x7000
    ("LDRB", 0, 0, 0),  # Should be 0x7800
]

print("\nVerifying CPU table ranges:")
for instr, rd, rb, offset in test_cases:
    if instr == "STR":
        actual = encode_str_immediate_offset(rd, rb, offset)
    elif instr == "LDR":
        actual = encode_ldr_immediate_offset(rd, rb, offset)
    elif instr == "STRB":
        actual = encode_strb_immediate_offset(rd, rb, offset)
    elif instr == "LDRB":
        actual = encode_ldrb_immediate_offset(rd, rb, offset)
    
    cpu_range = (actual >> 8) & 0xF8  # Get upper 5 bits for range
    expected_ranges = {
        "STR": 0x60, "LDR": 0x68, "STRB": 0x70, "LDRB": 0x78
    }
    expected = expected_ranges[instr]
    print(f"{instr:4s}: Generated 0x{actual:04X} -> Range 0x{cpu_range:02X} (expected 0x{expected:02X}) {'✓' if cpu_range == expected else '✗'}")

print("\n=== Addressing Examples ===\n")

print("Word operations (4-byte aligned):")
for i in range(8):
    byte_addr = i * 4
    print(f"  STR R0, [R1, #{byte_addr:2d}]: 0x{encode_str_immediate_offset(0, 1, i):04X} -> [R1 + {byte_addr}]")

print("\nByte operations:")
for i in range(8):
    print(f"  STRB R0, [R1, #{i}]: 0x{encode_strb_immediate_offset(0, 1, i):04X} -> [R1 + {i}]")
