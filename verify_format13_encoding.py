#!/usr/bin/env python3
"""
ARM Thumb Format 13: Add/Subtract offset to Stack Pointer instruction encoding verification

Format 13 encoding:
ADD SP, #imm: 1011 0000 S [offset7]
SUB SP, #imm: 1011 0000 S [offset7]

Where:
- S = 0: ADD SP, #imm (SP = SP + (offset7 * 4))
- S = 1: SUB SP, #imm (SP = SP - (offset7 * 4))

Bit pattern:
15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 1  0  1  1  0  0  0  0  S offset7[6:0]

offset7: 7-bit immediate value (bits 0-6)
S: Sign bit (bit 7) - 0=ADD, 1=SUB
"""

def encode_add_sp_instruction(offset):
    """Encode an ADD SP, #imm instruction"""
    if offset < 0 or offset > 508:
        raise ValueError(f"Offset {offset} out of range [0, 508]")
    if offset % 4 != 0:
        raise ValueError(f"Offset {offset} must be a multiple of 4")
    
    offset7 = offset // 4  # Scale down by 4
    if offset7 > 127:
        raise ValueError(f"Scaled offset {offset7} out of range [0, 127]")
    
    base = 0b1011000000000000  # 1011 0000 0XXX XXXX
    instruction = base | offset7  # Add offset7 (bits 0-6)
    return instruction

def encode_sub_sp_instruction(offset):
    """Encode a SUB SP, #imm instruction"""
    if offset < 0 or offset > 508:
        raise ValueError(f"Offset {offset} out of range [0, 508]")
    if offset % 4 != 0:
        raise ValueError(f"Offset {offset} must be a multiple of 4")
    
    offset7 = offset // 4  # Scale down by 4
    if offset7 > 127:
        raise ValueError(f"Scaled offset {offset7} out of range [0, 127]")
    
    base = 0b1011000010000000  # 1011 0000 1XXX XXXX
    instruction = base | offset7  # Add offset7 (bits 0-6)
    return instruction

def decode_format13_instruction(instruction):
    """Decode a Format 13 instruction"""
    if (instruction & 0xFF00) != 0xB000:
        raise ValueError(f"Not a Format 13 instruction: 0x{instruction:04X}")
    
    sign = (instruction >> 7) & 1
    offset7 = instruction & 0x7F
    actual_offset = offset7 * 4
    
    if sign == 0:
        return f"ADD SP, #{actual_offset}", actual_offset, "ADD"
    else:
        return f"SUB SP, #{actual_offset}", actual_offset, "SUB"

def test_add_sp_encodings():
    """Test various ADD SP instruction encodings"""
    print("ADD SP Instruction Encodings:")
    print("=" * 50)
    
    test_cases = [
        0,     # Minimum offset
        4,     # Basic case
        8,     # Another basic case
        16,    # Small offset
        32,    # Medium offset
        64,    # Larger offset
        124,   # Near maximum
        127*4, # Maximum offset (508)
    ]
    
    for offset in test_cases:
        try:
            opcode = encode_add_sp_instruction(offset)
            decoded, decoded_offset, op = decode_format13_instruction(opcode)
            print(f"ADD SP, #{offset:3d} = 0x{opcode:04X} = {decoded}")
            assert decoded_offset == offset, f"Offset mismatch: {decoded_offset} != {offset}"
            assert op == "ADD", f"Operation mismatch: {op} != ADD"
        except ValueError as e:
            print(f"ADD SP, #{offset:3d} = ERROR: {e}")

def test_sub_sp_encodings():
    """Test various SUB SP instruction encodings"""
    print("\nSUB SP Instruction Encodings:")
    print("=" * 50)
    
    test_cases = [
        0,     # Minimum offset
        4,     # Basic case
        8,     # Another basic case
        16,    # Small offset
        32,    # Medium offset
        64,    # Larger offset
        124,   # Near maximum
        127*4, # Maximum offset (508)
    ]
    
    for offset in test_cases:
        try:
            opcode = encode_sub_sp_instruction(offset)
            decoded, decoded_offset, op = decode_format13_instruction(opcode)
            print(f"SUB SP, #{offset:3d} = 0x{opcode:04X} = {decoded}")
            assert decoded_offset == offset, f"Offset mismatch: {decoded_offset} != {offset}"
            assert op == "SUB", f"Operation mismatch: {op} != SUB"
        except ValueError as e:
            print(f"SUB SP, #{offset:3d} = ERROR: {e}")

def test_edge_cases():
    """Test edge cases and error conditions"""
    print("\nEdge Cases and Error Testing:")
    print("=" * 50)
    
    # Test invalid offsets
    invalid_offsets = [-1, 1, 2, 3, 5, 512, 1000]
    
    for offset in invalid_offsets:
        try:
            opcode = encode_add_sp_instruction(offset)
            print(f"ADD SP, #{offset} = 0x{opcode:04X} (UNEXPECTED SUCCESS)")
        except ValueError as e:
            print(f"ADD SP, #{offset} = ERROR: {e} (EXPECTED)")
    
    # Test boundary values
    boundary_cases = [0, 4, 508]  # Min, small, max
    
    print("\nBoundary Cases:")
    for offset in boundary_cases:
        add_opcode = encode_add_sp_instruction(offset)
        sub_opcode = encode_sub_sp_instruction(offset)
        print(f"ADD SP, #{offset} = 0x{add_opcode:04X}")
        print(f"SUB SP, #{offset} = 0x{sub_opcode:04X}")

def test_comprehensive_range():
    """Test all valid offsets comprehensively"""
    print("\nComprehensive Range Test:")
    print("=" * 50)
    
    errors = 0
    for offset in range(0, 512, 4):  # All valid offsets
        try:
            # Test ADD
            add_opcode = encode_add_sp_instruction(offset)
            decoded_add, decoded_offset_add, op_add = decode_format13_instruction(add_opcode)
            if decoded_offset_add != offset or op_add != "ADD":
                print(f"ADD ERROR at offset {offset}")
                errors += 1
            
            # Test SUB
            sub_opcode = encode_sub_sp_instruction(offset)
            decoded_sub, decoded_offset_sub, op_sub = decode_format13_instruction(sub_opcode)
            if decoded_offset_sub != offset or op_sub != "SUB":
                print(f"SUB ERROR at offset {offset}")
                errors += 1
                
        except Exception as e:
            print(f"EXCEPTION at offset {offset}: {e}")
            errors += 1
    
    if errors == 0:
        print(f"✓ All {len(range(0, 512, 4))} ADD and SUB offsets encoded/decoded correctly")
    else:
        print(f"✗ {errors} errors found in comprehensive test")

def generate_test_vectors():
    """Generate test vectors for C++ tests"""
    print("\nTest Vectors for C++ Tests:")
    print("=" * 50)
    
    # Key test cases for C++ implementation
    test_vectors = [
        ("ADD", 0),     # Minimum
        ("ADD", 4),     # Basic
        ("ADD", 32),    # Medium
        ("ADD", 128),   # Large
        ("ADD", 508),   # Maximum
        ("SUB", 0),     # Minimum
        ("SUB", 4),     # Basic
        ("SUB", 32),    # Medium
        ("SUB", 128),   # Large
        ("SUB", 508),   # Maximum
    ]
    
    for op, offset in test_vectors:
        if op == "ADD":
            opcode = encode_add_sp_instruction(offset)
        else:
            opcode = encode_sub_sp_instruction(offset)
        
        print(f"// {op} SP, #{offset}")
        print(f"gba.getCPU().getMemory().write16(0x00000000, 0x{opcode:04X}); // {op} SP, #{offset}")

if __name__ == "__main__":
    print("ARM Thumb Format 13 Instruction Encoding Verification")
    print("=" * 60)
    
    test_add_sp_encodings()
    test_sub_sp_encodings()
    test_edge_cases()
    test_comprehensive_range()
    generate_test_vectors()
    
    print("\n" + "=" * 60)
    print("Format 13 encoding verification completed!")
