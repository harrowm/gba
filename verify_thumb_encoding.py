#!/usr/bin/env python3
"""
Thumb instruction encoding verification script for GBA emulator tests.
This script helps verify that instruction encodings are correct for STR_BYTE and STRH instructions.
"""

def encode_str_byte(rd, rn, rm):
    """
    Encode STRB Rd, [Rn, Rm] instruction
    Format: 0101 010m mmnn nddd
    """
    if not (0 <= rd <= 7 and 0 <= rn <= 7 and 0 <= rm <= 7):
        raise ValueError("All registers must be in range 0-7 for low registers")
    
    instruction = 0b0101010000000000  # Base pattern for STRB
    instruction |= (rm & 0x7) << 6    # Bits 8-6: offset register
    instruction |= (rn & 0x7) << 3    # Bits 5-3: base register
    instruction |= (rd & 0x7)         # Bits 2-0: destination register
    
    return instruction

def encode_strh(rd, rn, rm):
    """
    Encode STRH Rd, [Rn, Rm] instruction  
    Format: 0101 001m mmnn nddd
    """
    if not (0 <= rd <= 7 and 0 <= rn <= 7 and 0 <= rm <= 7):
        raise ValueError("All registers must be in range 0-7 for low registers")
    
    instruction = 0b0101001000000000  # Base pattern for STRH
    instruction |= (rm & 0x7) << 6    # Bits 8-6: offset register
    instruction |= (rn & 0x7) << 3    # Bits 5-3: base register
    instruction |= (rd & 0x7)         # Bits 2-0: destination register
    
    return instruction

def encode_str_word(rd, rn, rm):
    """
    Encode STR Rd, [Rn, Rm] instruction (word store)
    Format: 0101 000m mmnn nddd
    """
    if not (0 <= rd <= 7 and 0 <= rn <= 7 and 0 <= rm <= 7):
        raise ValueError("All registers must be in range 0-7 for low registers")
    
    instruction = 0b0101000000000000  # Base pattern for STR
    instruction |= (rm & 0x7) << 6    # Bits 8-6: offset register
    instruction |= (rn & 0x7) << 3    # Bits 5-3: base register
    instruction |= (rd & 0x7)         # Bits 2-0: destination register
    
    return instruction

def test_encodings():
    """Test various instruction encodings and print results"""
    
    print("=== STRB (Store Byte) Instruction Encodings ===")
    test_cases_strb = [
        (0, 1, 2),  # STRB R0, [R1, R2]
        (3, 4, 5),  # STRB R3, [R4, R5]
        (7, 6, 1),  # STRB R7, [R6, R1]
        (2, 0, 7),  # STRB R2, [R0, R7]
        (1, 7, 0),  # STRB R1, [R7, R0]
        (6, 2, 3),  # STRB R6, [R2, R3]
        (4, 3, 4),  # STRB R4, [R3, R4] (same reg as base and offset)
        (0, 0, 1),  # STRB R0, [R0, R1] (same reg as data and base)
    ]
    
    for rd, rn, rm in test_cases_strb:
        encoding = encode_str_byte(rd, rn, rm)
        print(f"STRB R{rd}, [R{rn}, R{rm}] = 0x{encoding:04X}")
    
    print("\n=== STRH (Store Halfword) Instruction Encodings ===")
    test_cases_strh = [
        (0, 1, 2),  # STRH R0, [R1, R2]
        (3, 4, 5),  # STRH R3, [R4, R5]
        (7, 6, 1),  # STRH R7, [R6, R1]
        (2, 0, 7),  # STRH R2, [R0, R7]
        (1, 7, 0),  # STRH R1, [R7, R0]
        (6, 2, 3),  # STRH R6, [R2, R3]
        (4, 3, 4),  # STRH R4, [R3, R4] (same reg as base and offset)
        (0, 0, 1),  # STRH R0, [R0, R1] (same reg as data and base)
    ]
    
    for rd, rn, rm in test_cases_strh:
        encoding = encode_strh(rd, rn, rm)
        print(f"STRH R{rd}, [R{rn}, R{rm}] = 0x{encoding:04X}")
    
    print("\n=== STR (Store Word) Reference Encodings ===")
    test_cases_str = [
        (0, 1, 2),  # STR R0, [R1, R2]
        (7, 1, 6),  # STR R7, [R1, R6] (from fixed test case)
        (5, 4, 3),  # STR R5, [R4, R3]
    ]
    
    for rd, rn, rm in test_cases_str:
        encoding = encode_str_word(rd, rn, rm)
        print(f"STR R{rd}, [R{rn}, R{rm}] = 0x{encoding:04X}")

def verify_specific_encodings():
    """Verify specific encodings mentioned in tests"""
    print("\n=== Verification of Specific Encodings ===")
    
    # From STR_WORD test case 11 (corrected)
    str_encoding = encode_str_word(7, 1, 6)  # STR R7, [R1, R6]
    print(f"STR R7, [R1, R6] = 0x{str_encoding:04X} (should be 0x518F)")
    
    # Additional verification examples
    examples = [
        ("STRB R0, [R1, R2]", encode_str_byte(0, 1, 2)),
        ("STRB R7, [R6, R5]", encode_str_byte(7, 6, 5)),
        ("STRH R0, [R1, R2]", encode_strh(0, 1, 2)),
        ("STRH R7, [R6, R5]", encode_strh(7, 6, 5)),
    ]
    
    for desc, encoding in examples:
        print(f"{desc} = 0x{encoding:04X}")

if __name__ == "__main__":
    test_encodings()
    verify_specific_encodings()
