#!/usr/bin/env python3

def encode_thumb_reg_offset(L, B, ro, rb, rd):
    """Encode load/store with register offset instruction (Format 7)
    
    Format: 0101 L B 0 ro rb rd
    - Bits 15-12: 0101 (0x5)
    - Bit 11: L (Load=1, Store=0)
    - Bit 10: B (Byte=1, Word=0)
    - Bit 9: 0 (fixed)
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
    if L not in [0, 1]:
        raise ValueError(f"L must be 0 or 1, got {L}")
    if B not in [0, 1]:
        raise ValueError(f"B must be 0 or 1, got {B}")
    
    opcode = 0x5000  # Base for reg offset (0101 0 0 0 000 000 000)
    opcode |= (L & 0x1) << 11       # L bit in bit 11
    opcode |= (B & 0x1) << 10       # B bit in bit 10
    opcode |= (ro & 0x7) << 6       # ro in bits 8:6
    opcode |= (rb & 0x7) << 3       # rb in bits 5:3
    opcode |= (rd & 0x7)            # rd in bits 2:0
    return opcode

def encode_str_reg_offset(rd, rb, ro):
    """Encode STR Rd, [Rb, Ro] instruction"""
    return encode_thumb_reg_offset(0, 0, ro, rb, rd)

def encode_strb_reg_offset(rd, rb, ro):
    """Encode STRB Rd, [Rb, Ro] instruction"""
    return encode_thumb_reg_offset(0, 1, ro, rb, rd)

def encode_ldr_reg_offset(rd, rb, ro):
    """Encode LDR Rd, [Rb, Ro] instruction"""
    return encode_thumb_reg_offset(1, 0, ro, rb, rd)

def encode_ldrb_reg_offset(rd, rb, ro):
    """Encode LDRB Rd, [Rb, Ro] instruction"""
    return encode_thumb_reg_offset(1, 1, ro, rb, rd)

def verify_encoding(instruction, expected_opcode, rd, rb, ro):
    """Verify that our encoding matches expected opcode"""
    if instruction == "STR":
        actual_opcode = encode_str_reg_offset(rd, rb, ro)
    elif instruction == "STRB":
        actual_opcode = encode_strb_reg_offset(rd, rb, ro)
    elif instruction == "LDR":
        actual_opcode = encode_ldr_reg_offset(rd, rb, ro)
    elif instruction == "LDRB":
        actual_opcode = encode_ldrb_reg_offset(rd, rb, ro)
    else:
        raise ValueError(f"Unknown instruction: {instruction}")
    
    print(f"{instruction:4s} R{rd}, [R{rb}, R{ro}]: Expected 0x{expected_opcode:04X}, Got 0x{actual_opcode:04X} {'✓' if expected_opcode == actual_opcode else '✗'}")
    return expected_opcode == actual_opcode

print("=== ARM Thumb Format 7 Instruction Encodings ===\n")

print("STR Instructions (Store Word):")
for rd in range(8):
    for rb in range(8):
        for ro in range(8):
            if rd < 3 and rb < 3 and ro < 3:  # Show subset for readability
                print(f"  STR R{rd}, [R{rb}, R{ro}] = 0x{encode_str_reg_offset(rd, rb, ro):04X}")

print("\nSTRB Instructions (Store Byte):")
for rd in range(3):
    for rb in range(3):
        for ro in range(3):
            print(f"  STRB R{rd}, [R{rb}, R{ro}] = 0x{encode_strb_reg_offset(rd, rb, ro):04X}")

print("\nLDR Instructions (Load Word):")
for rd in range(3):
    for rb in range(3):
        for ro in range(3):
            print(f"  LDR R{rd}, [R{rb}, R{ro}] = 0x{encode_ldr_reg_offset(rd, rb, ro):04X}")

print("\nLDRB Instructions (Load Byte):")
for rd in range(3):
    for rb in range(3):
        for ro in range(3):
            print(f"  LDRB R{rd}, [R{rb}, R{ro}] = 0x{encode_ldrb_reg_offset(rd, rb, ro):04X}")

print("\n=== Encoding Pattern Verification ===\n")

print("Base patterns:")
print(f"STR  base: 0x{encode_str_reg_offset(0, 0, 0):04X} = 0101000000000000")
print(f"STRB base: 0x{encode_strb_reg_offset(0, 0, 0):04X} = 0101010000000000")
print(f"LDR  base: 0x{encode_ldr_reg_offset(0, 0, 0):04X} = 0101100000000000")
print(f"LDRB base: 0x{encode_ldrb_reg_offset(0, 0, 0):04X} = 0101110000000000")

print("\nL and B Bit Effects:")
print(f"STR  R0, [R0, R0]: 0x{encode_str_reg_offset(0, 0, 0):04X} (L=0, B=0)")
print(f"STRB R0, [R0, R0]: 0x{encode_strb_reg_offset(0, 0, 0):04X} (L=0, B=1)")
print(f"LDR  R0, [R0, R0]: 0x{encode_ldr_reg_offset(0, 0, 0):04X} (L=1, B=0)")
print(f"LDRB R0, [R0, R0]: 0x{encode_ldrb_reg_offset(0, 0, 0):04X} (L=1, B=1)")

print("\nRegister Field Effects:")
print("Rd field (bits 2:0):")
for rd in range(8):
    print(f"  STR R{rd}, [R0, R0]: 0x{encode_str_reg_offset(rd, 0, 0):04X}")

print("Rb field (bits 5:3):")
for rb in range(8):
    print(f"  STR R0, [R{rb}, R0]: 0x{encode_str_reg_offset(0, rb, 0):04X}")

print("Ro field (bits 8:6):")
for ro in range(8):
    print(f"  STR R0, [R0, R{ro}]: 0x{encode_str_reg_offset(0, 0, ro):04X}")

print("\n=== Edge Cases ===\n")

print("All registers minimum:")
print(f"STR R0, [R0, R0]:   0x{encode_str_reg_offset(0, 0, 0):04X}")
print(f"LDRB R0, [R0, R0]:  0x{encode_ldrb_reg_offset(0, 0, 0):04X}")

print("\nAll registers maximum:")
print(f"STR R7, [R7, R7]:   0x{encode_str_reg_offset(7, 7, 7):04X}")
print(f"LDRB R7, [R7, R7]:  0x{encode_ldrb_reg_offset(7, 7, 7):04X}")

print("\nMixed combinations:")
print(f"STR R3, [R1, R5]:   0x{encode_str_reg_offset(3, 1, 5):04X}")
print(f"LDRB R2, [R6, R4]:  0x{encode_ldrb_reg_offset(2, 6, 4):04X}")
