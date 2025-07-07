#!/usr/bin/env python3

def encode_thumb_sign_halfword(H, S, ro, rb, rd):
    """Encode load/store sign-extended byte/halfword instruction (Format 8)
    
    Based on CPU implementation analysis:
    0x52: STRH => 0101 0 0 1 0 010 => H=0, S=0, bit9=1
    0x56: LDSB => 0101 0 1 1 0 011 => H=0, S=1, bit9=1  
    0x5A: LDRH => 0101 1 0 1 0 101 => H=1, S=0, bit9=1
    0x5E: LDSH => 0101 1 1 1 0 111 => H=1, S=1, bit9=1
    
    Format: 0101 H S 1 ro rb rd
    - Bits 15-12: 0101 (0x5)
    - Bit 11: H (operation type)
    - Bit 10: S (sign extend or operation type)
    - Bit 9: 1 (fixed)
    - Bits 8-6: ro (offset register 0-7)
    - Bits 5-3: rb (base register 0-7)
    - Bits 2-0: rd (destination/source register 0-7)
    
    Effective address = rb + ro
    """
    if rd < 0 or rd > 7:
        raise ValueError(f"rd must be 0-7, got {rd}")
    if rb < 0 or rb > 7:
        raise ValueError(f"rb must be 0-7, got {rb}")
    if ro < 0 or ro > 7:
        raise ValueError(f"ro must be 0-7, got {ro}")
    if H not in [0, 1]:
        raise ValueError(f"H must be 0 or 1, got {H}")
    if S not in [0, 1]:
        raise ValueError(f"S must be 0 or 1, got {S}")
    
    opcode = 0x5200  # Base for sign/halfword (0101 0 0 1 000 000 000)
    opcode |= (H & 0x1) << 11       # H bit in bit 11
    opcode |= (S & 0x1) << 10       # S bit in bit 10
    opcode |= (ro & 0x7) << 6       # ro in bits 8:6
    opcode |= (rb & 0x7) << 3       # rb in bits 5:3
    opcode |= (rd & 0x7)            # rd in bits 2:0
    return opcode

def encode_strh_reg_offset(rd, rb, ro):
    """Encode STRH Rd, [Rb, Ro] instruction (maps to 0x52)"""
    return encode_thumb_sign_halfword(0, 0, ro, rb, rd)

def encode_ldsb_reg_offset(rd, rb, ro):
    """Encode LDSB Rd, [Rb, Ro] instruction (maps to 0x56)"""
    return encode_thumb_sign_halfword(0, 1, ro, rb, rd)

def encode_ldrh_reg_offset(rd, rb, ro):
    """Encode LDRH Rd, [Rb, Ro] instruction (maps to 0x5A)"""
    return encode_thumb_sign_halfword(1, 0, ro, rb, rd)

def encode_ldsh_reg_offset(rd, rb, ro):
    """Encode LDSH Rd, [Rb, Ro] instruction (maps to 0x5E)"""
    return encode_thumb_sign_halfword(1, 1, ro, rb, rd)

def verify_encoding(instruction, expected_opcode, rd, rb, ro):
    """Verify that our encoding matches expected opcode"""
    if instruction == "STRH":
        actual_opcode = encode_strh_reg_offset(rd, rb, ro)
    elif instruction == "LDSB":
        actual_opcode = encode_ldsb_reg_offset(rd, rb, ro)
    elif instruction == "LDRH":
        actual_opcode = encode_ldrh_reg_offset(rd, rb, ro)
    elif instruction == "LDSH":
        actual_opcode = encode_ldsh_reg_offset(rd, rb, ro)
    else:
        raise ValueError(f"Unknown instruction: {instruction}")
    
    print(f"{instruction:4s} R{rd}, [R{rb}, R{ro}]: Expected 0x{expected_opcode:04X}, Got 0x{actual_opcode:04X} {'✓' if expected_opcode == actual_opcode else '✗'}")
    return expected_opcode == actual_opcode

print("=== ARM Thumb Format 8 Instruction Encodings ===\n")

print("STRH Instructions (Store Halfword):")
for rd in range(3):
    for rb in range(3):
        for ro in range(3):
            print(f"  STRH R{rd}, [R{rb}, R{ro}] = 0x{encode_strh_reg_offset(rd, rb, ro):04X}")

print("\nLDSB Instructions (Load Sign-extended Byte):")
for rd in range(3):
    for rb in range(3):
        for ro in range(3):
            print(f"  LDSB R{rd}, [R{rb}, R{ro}] = 0x{encode_ldsb_reg_offset(rd, rb, ro):04X}")

print("\nLDRH Instructions (Load Halfword):")
for rd in range(3):
    for rb in range(3):
        for ro in range(3):
            print(f"  LDRH R{rd}, [R{rb}, R{ro}] = 0x{encode_ldrh_reg_offset(rd, rb, ro):04X}")

print("\nLDSH Instructions (Load Sign-extended Halfword):")
for rd in range(3):
    for rb in range(3):
        for ro in range(3):
            print(f"  LDSH R{rd}, [R{rb}, R{ro}] = 0x{encode_ldsh_reg_offset(rd, rb, ro):04X}")

print("\n=== Encoding Pattern Verification ===\n")

print("Base patterns:")
print(f"STRH base: 0x{encode_strh_reg_offset(0, 0, 0):04X} = 0101100100000000")
print(f"LDSB base: 0x{encode_ldsb_reg_offset(0, 0, 0):04X} = 0101010100000000")
print(f"LDRH base: 0x{encode_ldrh_reg_offset(0, 0, 0):04X} = 0101110100000000")
print(f"LDSH base: 0x{encode_ldsh_reg_offset(0, 0, 0):04X} = 0101110100000000")

print("\nH and S Bit Effects:")
# Note: Not all combinations are valid according to ARM documentation
# H=0, S=0: Reserved
# H=0, S=1: LDSB (Load sign-extended byte)
# H=1, S=0: STRH (Store halfword)
# H=1, S=1: LDRH/LDSH (Load halfword, can be sign-extended)

print("Valid instruction combinations:")
print(f"STRH R0, [R0, R0]: 0x{encode_strh_reg_offset(0, 0, 0):04X} (H=1, S=0)")
print(f"LDSB R0, [R0, R0]: 0x{encode_ldsb_reg_offset(0, 0, 0):04X} (H=0, S=1)")
print(f"LDRH R0, [R0, R0]: 0x{encode_ldrh_reg_offset(0, 0, 0):04X} (H=1, S=1)")

print("\nRegister Field Effects:")
print("Rd field (bits 2:0):")
for rd in range(8):
    print(f"  STRH R{rd}, [R0, R0]: 0x{encode_strh_reg_offset(rd, 0, 0):04X}")

print("Rb field (bits 5:3):")
for rb in range(8):
    print(f"  STRH R0, [R{rb}, R0]: 0x{encode_strh_reg_offset(0, rb, 0):04X}")

print("Ro field (bits 8:6):")
for ro in range(8):
    print(f"  STRH R0, [R0, R{ro}]: 0x{encode_strh_reg_offset(0, 0, ro):04X}")

print("\n=== Edge Cases ===\n")

print("All registers minimum:")
print(f"STRH R0, [R0, R0]:  0x{encode_strh_reg_offset(0, 0, 0):04X}")
print(f"LDSB R0, [R0, R0]:  0x{encode_ldsb_reg_offset(0, 0, 0):04X}")
print(f"LDRH R0, [R0, R0]:  0x{encode_ldrh_reg_offset(0, 0, 0):04X}")

print("\nAll registers maximum:")
print(f"STRH R7, [R7, R7]:  0x{encode_strh_reg_offset(7, 7, 7):04X}")
print(f"LDSB R7, [R7, R7]:  0x{encode_ldsb_reg_offset(7, 7, 7):04X}")
print(f"LDRH R7, [R7, R7]:  0x{encode_ldrh_reg_offset(7, 7, 7):04X}")

print("\nMixed combinations:")
print(f"STRH R3, [R1, R5]:  0x{encode_strh_reg_offset(3, 1, 5):04X}")
print(f"LDSB R2, [R6, R4]:  0x{encode_ldsb_reg_offset(2, 6, 4):04X}")
print(f"LDRH R5, [R2, R7]:  0x{encode_ldrh_reg_offset(5, 2, 7):04X}")

print("\n=== CPU Implementation Mapping ===\n")

print("Expected opcode mappings from CPU implementation:")
print("0x52: STRH, 0x53: STRH")  # Based on thumb_instruction_table
print("0x56: LDSB, 0x57: LDSB")
print("0x5A: LDRH, 0x5B: LDRH")
print("0x5E: LDSH, 0x5F: LDSH")

# Verify specific opcodes match CPU implementation
test_cases = [
    ("STRH", 0x5200, 0, 0, 0),  # Should map to 0x52
    ("LDSB", 0x5600, 0, 0, 0),  # Should map to 0x56
    ("LDRH", 0x5A00, 0, 0, 0),  # Should map to 0x5A
]

print("\nVerifying CPU table mappings:")
for instr, expected, rd, rb, ro in test_cases:
    if instr == "STRH":
        actual = encode_strh_reg_offset(rd, rb, ro)
    elif instr == "LDSB":
        actual = encode_ldsb_reg_offset(rd, rb, ro)
    elif instr == "LDRH":
        actual = encode_ldrh_reg_offset(rd, rb, ro)
    
    cpu_opcode = (actual >> 8) & 0xFF
    expected_cpu = (expected >> 8) & 0xFF
    print(f"{instr}: Generated 0x{actual:04X} -> CPU table 0x{cpu_opcode:02X} (expected 0x{expected_cpu:02X}) {'✓' if cpu_opcode == expected_cpu else '✗'}")
