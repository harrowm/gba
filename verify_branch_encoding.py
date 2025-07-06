#!/usr/bin/env python3
"""
Verify Thumb instruction encodings for B, BL, and B_COND instructions.
This script generates and verifies the correct instruction encodings used in the test cases.
"""

def encode_thumb_b(offset):
    """Encode Thumb B (unconditional branch) instruction.
    Format: 11100 offset[10:0]
    """
    if offset < -1024 or offset > 1022:
        raise ValueError(f"B offset {offset} out of range [-1024, 1022]")
    if offset % 2 != 0:
        raise ValueError(f"B offset {offset} must be even")
    
    # Convert to 11-bit signed offset (divide by 2 since instructions are 2-byte aligned)
    offset_bits = (offset // 2) & 0x7FF
    instruction = 0xE000 | offset_bits
    return instruction

def encode_thumb_bl_high(offset):
    """Encode Thumb BL high part instruction.
    Format: 11110 H offset[22:12]
    H=0 for first instruction
    """
    if offset < -4194304 or offset > 4194302:
        raise ValueError(f"BL offset {offset} out of range [-4194304, 4194302]")
    if offset % 2 != 0:
        raise ValueError(f"BL offset {offset} must be even")
    
    # Extract upper 11 bits of offset (bits 22-12)
    offset_upper = ((offset >> 12) & 0x7FF)
    instruction = 0xF000 | offset_upper
    return instruction

def encode_thumb_bl_low(offset):
    """Encode Thumb BL low part instruction.
    Format: 11111 H offset[11:1]
    H=1 for second instruction
    """
    if offset < -4194304 or offset > 4194302:
        raise ValueError(f"BL offset {offset} out of range [-4194304, 4194302]")
    if offset % 2 != 0:
        raise ValueError(f"BL offset {offset} must be even")
    
    # Extract lower 11 bits of offset (bits 11-1)
    offset_lower = ((offset >> 1) & 0x7FF)
    instruction = 0xF800 | offset_lower
    return instruction

def encode_thumb_b_cond(condition, offset):
    """Encode Thumb B conditional branch instruction.
    Format: 1101 cond offset[7:0]
    """
    if offset < -256 or offset > 254:
        raise ValueError(f"B_COND offset {offset} out of range [-256, 254]")
    if offset % 2 != 0:
        raise ValueError(f"B_COND offset {offset} must be even")
    if condition < 0 or condition > 15:
        raise ValueError(f"Condition {condition} out of range [0, 15]")
    
    # Convert to 8-bit signed offset (divide by 2 since instructions are 2-byte aligned)
    offset_bits = (offset // 2) & 0xFF
    instruction = 0xD000 | (condition << 8) | offset_bits
    return instruction

def verify_encodings():
    """Verify all instruction encodings used in the test cases."""
    print("Verifying Thumb Branch Instruction Encodings...")
    print("=" * 60)
    
    # Test B (unconditional branch) encodings
    print("\nB (Unconditional Branch) Tests:")
    test_cases_b = [
        (2, "Forward branch by 2"),
        (4, "Forward branch by 4"),
        (10, "Forward branch by 10"),
        (-2, "Backward branch by 2"),
        (-4, "Backward branch by 4"),
        (-10, "Backward branch by 10"),
        (0, "Branch to same location"),
        (1022, "Maximum forward branch"),
        (-1024, "Maximum backward branch"),
    ]
    
    for offset, description in test_cases_b:
        try:
            encoding = encode_thumb_b(offset)
            print(f"  {description}: offset={offset:4d} -> 0x{encoding:04X}")
        except ValueError as e:
            print(f"  {description}: ERROR - {e}")
    
    # Test BL (branch with link) encodings
    print("\nBL (Branch with Link) Tests:")
    test_cases_bl = [
        (4, "Forward branch by 4"),
        (8, "Forward branch by 8"),
        (100, "Forward branch by 100"),
        (-4, "Backward branch by 4"),
        (-8, "Backward branch by 8"),
        (-100, "Backward branch by 100"),
        (0, "Branch to same location"),
        (4194302, "Maximum forward branch"),
        (-4194304, "Maximum backward branch"),
    ]
    
    for offset, description in test_cases_bl:
        try:
            high = encode_thumb_bl_high(offset)
            low = encode_thumb_bl_low(offset)
            print(f"  {description}: offset={offset:7d} -> 0x{high:04X} 0x{low:04X}")
        except ValueError as e:
            print(f"  {description}: ERROR - {e}")
    
    # Test B_COND (conditional branch) encodings
    print("\nB_COND (Conditional Branch) Tests:")
    conditions = [
        (0, "EQ"), (1, "NE"), (2, "CS"), (3, "CC"),
        (4, "MI"), (5, "PL"), (6, "VS"), (7, "VC"),
        (8, "HI"), (9, "LS"), (10, "GE"), (11, "LT"),
        (12, "GT"), (13, "LE"), (14, "AL")
    ]
    
    test_offsets = [2, 4, -2, -4, 0, 254, -256]
    
    for cond_code, cond_name in conditions:
        if cond_code == 15:  # Skip undefined condition
            continue
        for offset in test_offsets:
            try:
                encoding = encode_thumb_b_cond(cond_code, offset)
                print(f"  B{cond_name}: offset={offset:4d} -> 0x{encoding:04X}")
            except ValueError as e:
                print(f"  B{cond_name}: offset={offset:4d} -> ERROR - {e}")
            break  # Only show one example per condition for brevity
    
    print("\nSpecific test case encodings:")
    print("-" * 40)
    
    # Specific encodings used in the test cases
    specific_cases = [
        ("B +2", lambda: encode_thumb_b(2)),
        ("B +4", lambda: encode_thumb_b(4)),
        ("B -2", lambda: encode_thumb_b(-2)),
        ("B +10", lambda: encode_thumb_b(10)),
        ("BEQ +2", lambda: encode_thumb_b_cond(0, 2)),
        ("BNE +4", lambda: encode_thumb_b_cond(1, 4)),
        ("BMI -2", lambda: encode_thumb_b_cond(4, -2)),
        ("BPL +6", lambda: encode_thumb_b_cond(5, 6)),
        ("BL +4 (high)", lambda: encode_thumb_bl_high(4)),
        ("BL +4 (low)", lambda: encode_thumb_bl_low(4)),
        ("BL +100 (high)", lambda: encode_thumb_bl_high(100)),
        ("BL +100 (low)", lambda: encode_thumb_bl_low(100)),
        ("BL -4 (high)", lambda: encode_thumb_bl_high(-4)),
        ("BL -4 (low)", lambda: encode_thumb_bl_low(-4)),
    ]
    
    for name, encoder in specific_cases:
        try:
            encoding = encoder()
            print(f"  {name:<15}: 0x{encoding:04X}")
        except ValueError as e:
            print(f"  {name:<15}: ERROR - {e}")

if __name__ == "__main__":
    verify_encodings()
