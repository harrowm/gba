#!/usr/bin/env python3

def encode_thumb_ldr_pc_relative(rd, word_offset):
    """Encode LDR Rd, [PC, #offset] instruction (Format 6)
    
    Format: 01001 rd word_offset
    - Bits 15-11: 01001 (0x09)
    - Bits 10-8: rd (destination register 0-7)
    - Bits 7-0: word_offset (offset in words, byte_offset = word_offset * 4)
    
    Effective address = (PC & 0xFFFFFFFC) + (word_offset * 4)
    PC points to instruction + 4 when instruction executes
    """
    if rd < 0 or rd > 7:
        raise ValueError(f"rd must be 0-7, got {rd}")
    if word_offset < 0 or word_offset > 255:
        raise ValueError(f"word_offset must be 0-255, got {word_offset}")
    
    opcode = 0x4800  # Base for LDR PC-relative (01001 000 00000000)
    opcode |= (rd & 0x7) << 8        # Rd in bits 10:8
    opcode |= (word_offset & 0xFF)   # word_offset in bits 7:0
    return opcode

def verify_encoding(expected_opcode, rd, byte_offset):
    """Verify that our encoding matches expected opcode"""
    word_offset = byte_offset // 4
    actual_opcode = encode_thumb_ldr_pc_relative(rd, word_offset)
    print(f"LDR R{rd}, [PC, #{byte_offset:3d}]: Expected 0x{expected_opcode:04X}, Got 0x{actual_opcode:04X} {'✓' if expected_opcode == actual_opcode else '✗'}")
    return expected_opcode == actual_opcode

# Verify encodings from existing tests
print("=== Verifying existing test encodings ===")
verify_encoding(0x4801, 0, 4)    # Test case 1
verify_encoding(0x4902, 1, 8)    # Test case 2
verify_encoding(0x4A03, 2, 12)   # Test case 3
verify_encoding(0x4B00, 3, 0)    # Test case 4
verify_encoding(0x4C04, 4, 16)   # Test case 5
verify_encoding(0x4D08, 5, 32)   # Test case 6
verify_encoding(0x4E10, 6, 64)   # Test case 7
verify_encoding(0x4F02, 7, 8)    # Test case 8

print("\n=== Additional encodings for comprehensive testing ===")

# Edge cases - minimum and maximum offsets
print("Edge case offsets:")
print(f"LDR R0, [PC, #0]:    0x{encode_thumb_ldr_pc_relative(0, 0):04X}")      # Minimum offset
print(f"LDR R7, [PC, #1020]: 0x{encode_thumb_ldr_pc_relative(7, 255):04X}")   # Maximum offset

# All registers with same offset
print("\nAll registers (offset #20):")
for rd in range(8):
    print(f"LDR R{rd}, [PC, #20]:  0x{encode_thumb_ldr_pc_relative(rd, 5):04X}")

# Boundary offsets
print("\nBoundary offsets:")
boundary_offsets = [0, 4, 8, 12, 16, 32, 64, 128, 256, 512, 1020]
for offset in boundary_offsets:
    word_offset = offset // 4
    if word_offset <= 255:
        print(f"LDR R0, [PC, #{offset:4d}]: 0x{encode_thumb_ldr_pc_relative(0, word_offset):04X}")

# Pattern verification - check bit patterns
print("\nBit pattern verification:")
print("Register bits (rd in bits 10:8):")
for rd in range(8):
    opcode = encode_thumb_ldr_pc_relative(rd, 0)
    print(f"  R{rd}: bits 10:8 = {(opcode >> 8) & 0x7:03b}")

print("\nOffset bits (word_offset in bits 7:0):")
test_offsets = [0, 1, 2, 4, 8, 16, 32, 64, 128, 255]
for word_offset in test_offsets:
    opcode = encode_thumb_ldr_pc_relative(0, word_offset)
    byte_offset = word_offset * 4
    print(f"  #{byte_offset:4d} bytes (#{word_offset:3d} words): bits 7:0 = {opcode & 0xFF:08b} (0x{opcode & 0xFF:02X})")

# Verify base opcode
base_opcode = 0x4800
print(f"\nBase opcode verification:")
print(f"Expected base: 0x{base_opcode:04X} (01001000 00000000)")
print(f"Bits 15-11:   {(base_opcode >> 11) & 0x1F:05b} (should be 01001)")
