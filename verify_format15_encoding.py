#!/usr/bin/env python3
"""
ARM Thumb Format 15: Multiple load/store instruction encoding verification

Format 15 encoding:
STMIA Rn!, {Rlist}: 1100 0 [Rn] [Rlist]
LDMIA Rn!, {Rlist}: 1100 1 [Rn] [Rlist]

Where:
- L = 0: STMIA (Store Multiple Increment After)
- L = 1: LDMIA (Load Multiple Increment After)
- Rn: Base register (3 bits, R0-R7 only)
- Rlist: Register list (8 bits, R0-R7)

Bit pattern:
15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 1  1  0  0  L  Rn[2:0]  Rlist[7:0]

Rn: Base register (bits 8-10)
L: Load/Store bit (bit 11) - 0=STMIA, 1=LDMIA
Rlist: Register list (bits 0-7)
"""

def encode_stmia_instruction(rn, register_list):
    """Encode a STMIA Rn!, {Rlist} instruction"""
    if rn < 0 or rn > 7:
        raise ValueError(f"Base register {rn} out of range [0, 7]")
    if register_list < 0 or register_list > 255:
        raise ValueError(f"Register list {register_list:02X} out of range [0, 255]")
    
    base = 0b1100000000000000  # 1100 0XXX XXXX XXXX
    instruction = base | (rn << 8) | register_list
    return instruction

def encode_ldmia_instruction(rn, register_list):
    """Encode a LDMIA Rn!, {Rlist} instruction"""
    if rn < 0 or rn > 7:
        raise ValueError(f"Base register {rn} out of range [0, 7]")
    if register_list < 0 or register_list > 255:
        raise ValueError(f"Register list {register_list:02X} out of range [0, 255]")
    
    base = 0b1100100000000000  # 1100 1XXX XXXX XXXX
    instruction = base | (rn << 8) | register_list
    return instruction

def decode_format15_instruction(instruction):
    """Decode a Format 15 instruction"""
    if (instruction & 0xF000) != 0xC000:
        raise ValueError(f"Not a Format 15 instruction: 0x{instruction:04X}")
    
    l_bit = (instruction >> 11) & 1
    rn = (instruction >> 8) & 0x07
    register_list = instruction & 0xFF
    
    # Convert register list to readable format
    regs = []
    for i in range(8):
        if register_list & (1 << i):
            regs.append(f"R{i}")
    
    reg_str = "{" + ", ".join(regs) + "}" if regs else "{}"
    
    if l_bit == 0:
        return f"STMIA R{rn}!, {reg_str}", rn, register_list, "STMIA"
    else:
        return f"LDMIA R{rn}!, {reg_str}", rn, register_list, "LDMIA"

def test_stmia_encodings():
    """Test various STMIA instruction encodings"""
    print("STMIA Instruction Encodings:")
    print("=" * 60)
    
    # Test cases: (base_reg, register_list, description)
    test_cases = [
        (0, 0x01, "R0 base, R0 only"),
        (0, 0x03, "R0 base, R0-R1"),
        (1, 0x0F, "R1 base, R0-R3"),
        (2, 0xFF, "R2 base, R0-R7 (all)"),
        (7, 0x80, "R7 base, R7 only"),
        (3, 0x55, "R3 base, R0,R2,R4,R6"),
        (4, 0xAA, "R4 base, R1,R3,R5,R7"),
        (5, 0x00, "R5 base, empty list"),
        (6, 0x10, "R6 base, R4 only"),
        (0, 0xFE, "R0 base, R1-R7"),
    ]
    
    for rn, reg_list, desc in test_cases:
        try:
            opcode = encode_stmia_instruction(rn, reg_list)
            decoded, decoded_rn, decoded_list, op = decode_format15_instruction(opcode)
            print(f"STMIA R{rn}!, 0x{reg_list:02X} = 0x{opcode:04X} = {decoded}")
            print(f"    {desc}")
            assert decoded_rn == rn, f"Base register mismatch: {decoded_rn} != {rn}"
            assert decoded_list == reg_list, f"Register list mismatch: {decoded_list:02X} != {reg_list:02X}"
            assert op == "STMIA", f"Operation mismatch: {op} != STMIA"
        except ValueError as e:
            print(f"STMIA R{rn}!, 0x{reg_list:02X} = ERROR: {e}")
        print()

def test_ldmia_encodings():
    """Test various LDMIA instruction encodings"""
    print("\nLDMIA Instruction Encodings:")
    print("=" * 60)
    
    # Test cases: (base_reg, register_list, description)
    test_cases = [
        (0, 0x01, "R0 base, R0 only"),
        (0, 0x03, "R0 base, R0-R1"),
        (1, 0x0F, "R1 base, R0-R3"),
        (2, 0xFF, "R2 base, R0-R7 (all)"),
        (7, 0x80, "R7 base, R7 only"),
        (3, 0x55, "R3 base, R0,R2,R4,R6"),
        (4, 0xAA, "R4 base, R1,R3,R5,R7"),
        (5, 0x00, "R5 base, empty list"),
        (6, 0x10, "R6 base, R4 only"),
        (0, 0xFE, "R0 base, R1-R7"),
    ]
    
    for rn, reg_list, desc in test_cases:
        try:
            opcode = encode_ldmia_instruction(rn, reg_list)
            decoded, decoded_rn, decoded_list, op = decode_format15_instruction(opcode)
            print(f"LDMIA R{rn}!, 0x{reg_list:02X} = 0x{opcode:04X} = {decoded}")
            print(f"    {desc}")
            assert decoded_rn == rn, f"Base register mismatch: {decoded_rn} != {rn}"
            assert decoded_list == reg_list, f"Register list mismatch: {decoded_list:02X} != {reg_list:02X}"
            assert op == "LDMIA", f"Operation mismatch: {op} != LDMIA"
        except ValueError as e:
            print(f"LDMIA R{rn}!, 0x{reg_list:02X} = ERROR: {e}")
        print()

def test_edge_cases():
    """Test edge cases and error conditions"""
    print("\nEdge Cases and Error Testing:")
    print("=" * 60)
    
    # Test invalid base registers
    print("Testing invalid base registers:")
    try:
        encode_stmia_instruction(8, 0x01)
        print("ERROR: Should have failed for base register 8")
    except ValueError as e:
        print(f"✓ Correctly rejected base register 8: {e}")
    
    try:
        encode_ldmia_instruction(-1, 0x01)
        print("ERROR: Should have failed for base register -1")
    except ValueError as e:
        print(f"✓ Correctly rejected base register -1: {e}")
    
    # Test invalid register lists
    print("\nTesting invalid register lists:")
    try:
        encode_stmia_instruction(0, 256)
        print("ERROR: Should have failed for register list 256")
    except ValueError as e:
        print(f"✓ Correctly rejected register list 256: {e}")
    
    try:
        encode_ldmia_instruction(0, -1)
        print("ERROR: Should have failed for register list -1")
    except ValueError as e:
        print(f"✓ Correctly rejected register list -1: {e}")

def test_specific_patterns():
    """Test specific bit patterns that might cause issues"""
    print("\nSpecific Bit Pattern Testing:")
    print("=" * 60)
    
    # Test instructions that validate against expected bit patterns
    specific_tests = [
        # (instruction, expected_opcode, description)
        ("STMIA R0!, {}", 0xC000, "Empty register list"),
        ("STMIA R0!, {R0}", 0xC001, "Single register R0"),
        ("STMIA R7!, {R7}", 0xC780, "Base R7, register R7"),
        ("LDMIA R0!, {R0}", 0xC801, "LDMIA R0 with R0"),
        ("LDMIA R7!, {R0-R7}", 0xCFFF, "LDMIA R7 with all registers"),
        ("STMIA R3!, {R1,R3,R5}", 0xC32A, "STMIA R3 with scattered regs"),
    ]
    
    for desc, expected, comment in specific_tests:
        # Parse the expected instruction manually
        if "STMIA" in desc:
            if "R0!" in desc and "{}" in desc:
                actual = encode_stmia_instruction(0, 0x00)
            elif "R0!" in desc and "{R0}" in desc:
                actual = encode_stmia_instruction(0, 0x01)
            elif "R7!" in desc and "{R7}" in desc:
                actual = encode_stmia_instruction(7, 0x80)
            elif "R3!" in desc and "{R1,R3,R5}" in desc:
                actual = encode_stmia_instruction(3, 0x2A)  # R1=0x02, R3=0x08, R5=0x20 -> 0x2A
        elif "LDMIA" in desc:
            if "R0!" in desc and "{R0}" in desc:
                actual = encode_ldmia_instruction(0, 0x01)
            elif "R7!" in desc and "{R0-R7}" in desc:
                actual = encode_ldmia_instruction(7, 0xFF)
        
        print(f"{comment}: Expected 0x{expected:04X}, Got 0x{actual:04X}")
        if actual == expected:
            print("✓ PASS")
        else:
            print("✗ FAIL")
        print()

def main():
    """Main test function"""
    print("ARM Thumb Format 15: Multiple Load/Store Instruction Encoding Verification")
    print("=" * 80)
    print()
    
    test_stmia_encodings()
    test_ldmia_encodings()
    test_edge_cases()
    test_specific_patterns()
    
    print("\nVerification complete!")

if __name__ == "__main__":
    main()
