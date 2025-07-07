#!/usr/bin/env python3

def encode_thumb_sp_relative(L, rd, word8):
    """Encode SP-relative load/store instruction (Format 11)
    
    Format: 1001 L rd word8
    - Bits 15-12: 1001 (0x9)
    - Bit 11: L (Load=1, Store=0)
    - Bits 10-8: rd (register 0-7)
    - Bits 7-0: word8 (offset in words, byte_offset = word8 * 4)
    
    Effective address = SP + (word8 * 4)
    """
    if rd < 0 or rd > 7:
        raise ValueError(f"rd must be 0-7, got {rd}")
    if word8 < 0 or word8 > 255:
        raise ValueError(f"word8 must be 0-255, got {word8}")
    if L not in [0, 1]:
        raise ValueError(f"L must be 0 or 1, got {L}")
    
    opcode = 0x9000  # Base for SP-relative (1001 0 000 00000000)
    opcode |= (L & 0x1) << 11       # L bit in bit 11
    opcode |= (rd & 0x7) << 8       # Rd in bits 10:8
    opcode |= (word8 & 0xFF)        # word8 in bits 7:0
    return opcode

def encode_str_sp_relative(rd, byte_offset):
    """Encode STR Rd, [SP, #offset] instruction"""
    word_offset = byte_offset // 4
    return encode_thumb_sp_relative(0, rd, word_offset)

def encode_ldr_sp_relative(rd, byte_offset):
    """Encode LDR Rd, [SP, #offset] instruction"""
    word_offset = byte_offset // 4
    return encode_thumb_sp_relative(1, rd, word_offset)

def verify_encoding(instruction, expected_opcode, rd, byte_offset):
    """Verify that our encoding matches expected opcode"""
    if instruction == "STR":
        actual_opcode = encode_str_sp_relative(rd, byte_offset)
    elif instruction == "LDR":
        actual_opcode = encode_ldr_sp_relative(rd, byte_offset)
    else:
        raise ValueError(f"Unknown instruction: {instruction}")
    
    print(f"{instruction} R{rd}, [SP, #{byte_offset:3d}]: Expected 0x{expected_opcode:04X}, Got 0x{actual_opcode:04X} {'✓' if expected_opcode == actual_opcode else '✗'}")
    return expected_opcode == actual_opcode

print("=== ARM Thumb Format 11 Instruction Encodings ===\n")

print("STR Instructions (Store to Stack):")
print(f"  STR R0, [SP, #0]   = 0x{encode_str_sp_relative(0, 0):04X}")
print(f"  STR R1, [SP, #4]   = 0x{encode_str_sp_relative(1, 4):04X}")
print(f"  STR R2, [SP, #8]   = 0x{encode_str_sp_relative(2, 8):04X}")
print(f"  STR R3, [SP, #12]  = 0x{encode_str_sp_relative(3, 12):04X}")
print(f"  STR R4, [SP, #16]  = 0x{encode_str_sp_relative(4, 16):04X}")
print(f"  STR R5, [SP, #20]  = 0x{encode_str_sp_relative(5, 20):04X}")
print(f"  STR R6, [SP, #24]  = 0x{encode_str_sp_relative(6, 24):04X}")
print(f"  STR R7, [SP, #28]  = 0x{encode_str_sp_relative(7, 28):04X}")
print(f"  STR R0, [SP, #1020] = 0x{encode_str_sp_relative(0, 1020):04X}")  # Maximum offset

print("\nLDR Instructions (Load from Stack):")
print(f"  LDR R0, [SP, #0]   = 0x{encode_ldr_sp_relative(0, 0):04X}")
print(f"  LDR R1, [SP, #4]   = 0x{encode_ldr_sp_relative(1, 4):04X}")
print(f"  LDR R2, [SP, #8]   = 0x{encode_ldr_sp_relative(2, 8):04X}")
print(f"  LDR R3, [SP, #12]  = 0x{encode_ldr_sp_relative(3, 12):04X}")
print(f"  LDR R4, [SP, #16]  = 0x{encode_ldr_sp_relative(4, 16):04X}")
print(f"  LDR R5, [SP, #20]  = 0x{encode_ldr_sp_relative(5, 20):04X}")
print(f"  LDR R6, [SP, #24]  = 0x{encode_ldr_sp_relative(6, 24):04X}")
print(f"  LDR R7, [SP, #28]  = 0x{encode_ldr_sp_relative(7, 28):04X}")
print(f"  LDR R7, [SP, #1020] = 0x{encode_ldr_sp_relative(7, 1020):04X}")  # Maximum offset

print("\n=== Encoding Pattern Verification ===\n")

print("STR base pattern: 0x9000 = 1001000000000000")
print("LDR base pattern: 0x9800 = 1001100000000000")

print("\nL Bit Effects:")
print(f"STR R0, [SP, #0]:  0x{encode_str_sp_relative(0, 0):04X} (L=0)")
print(f"LDR R0, [SP, #0]:  0x{encode_ldr_sp_relative(0, 0):04X} (L=1)")

print("\nRegister Field Effects (offset #12):")
for rd in range(8):
    print(f"STR R{rd}, [SP, #12]: 0x{encode_str_sp_relative(rd, 12):04X}")

print("\nOffset Field Effects (R0):")
test_offsets = [0, 4, 8, 16, 32, 64, 128, 256, 512, 1020]
for offset in test_offsets:
    print(f"STR R0, [SP, #{offset:4d}]: 0x{encode_str_sp_relative(0, offset):04X} (word8={offset//4})")

print("\n=== Edge Cases ===\n")

print("Minimum values:")
print(f"STR R0, [SP, #0]:    0x{encode_str_sp_relative(0, 0):04X}")
print(f"LDR R0, [SP, #0]:    0x{encode_ldr_sp_relative(0, 0):04X}")

print("\nMaximum values:")
print(f"STR R7, [SP, #1020]: 0x{encode_str_sp_relative(7, 1020):04X}")
print(f"LDR R7, [SP, #1020]: 0x{encode_ldr_sp_relative(7, 1020):04X}")

print("\nWord alignment verification:")
valid_offsets = [0, 4, 8, 12, 16, 20, 1016, 1020]
for offset in valid_offsets:
    word8 = offset // 4
    reconstructed = word8 * 4
    print(f"Offset {offset:4d} -> word8 {word8:3d} -> reconstructed {reconstructed:4d} ({'✓' if offset == reconstructed else '✗'})")
