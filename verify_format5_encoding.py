#!/usr/bin/env python3
"""
ARM Thumb Format 5: Hi register operations/branch exchange instruction encoding verification

Format 5 encoding: 010001[Op][H1][H2][Rs/Hs][Rd/Hd]
- Op (bits 9-8): Operation type
  - 00: ADD Rd, Rs (ADD with hi registers)
  - 01: CMP Rd, Rs (Compare with hi registers) 
  - 10: MOV Rd, Rs (Move with hi registers)
  - 11: BX Rs (Branch exchange)
- H1 (bit 7): Destination register is high (R8-R15)
- H2 (bit 6): Source register is high (R8-R15)
- Rs/Hs (bits 5-3): Source register (0-7, but can reference R8-R15 with H2=1)
- Rd/Hd (bits 2-0): Destination register (0-7, but can reference R8-R15 with H1=1)

For BX instruction:
- Op = 11 (bits 9-8)
- H1 = 0 (bit 7) - always 0 for BX
- H2 (bit 6): Source register is high (R8-R15)
- Rs/Hs (bits 5-3): Source register
- SBZ (bits 2-0): Should be zero
"""

def encode_format5_add(rd, rs):
    """Encode ADD Rd, Rs instruction"""
    op = 0b00  # ADD operation
    h1 = 1 if rd >= 8 else 0
    h2 = 1 if rs >= 8 else 0
    rd_field = rd & 0x7  # Lower 3 bits
    rs_field = rs & 0x7  # Lower 3 bits
    
    return 0x4400 | (op << 8) | (h1 << 7) | (h2 << 6) | (rs_field << 3) | rd_field

def encode_format5_cmp(rd, rs):
    """Encode CMP Rd, Rs instruction"""
    op = 0b01  # CMP operation
    h1 = 1 if rd >= 8 else 0
    h2 = 1 if rs >= 8 else 0
    rd_field = rd & 0x7  # Lower 3 bits
    rs_field = rs & 0x7  # Lower 3 bits
    
    return 0x4400 | (op << 8) | (h1 << 7) | (h2 << 6) | (rs_field << 3) | rd_field

def encode_format5_mov(rd, rs):
    """Encode MOV Rd, Rs instruction"""
    op = 0b10  # MOV operation
    h1 = 1 if rd >= 8 else 0
    h2 = 1 if rs >= 8 else 0
    rd_field = rd & 0x7  # Lower 3 bits
    rs_field = rs & 0x7  # Lower 3 bits
    
    return 0x4400 | (op << 8) | (h1 << 7) | (h2 << 6) | (rs_field << 3) | rd_field

def encode_format5_bx(rs):
    """Encode BX Rs instruction"""
    op = 0b11  # BX operation
    h1 = 0     # Always 0 for BX
    h2 = 1 if rs >= 8 else 0
    rs_field = rs & 0x7  # Lower 3 bits
    rd_field = 0         # Should be zero for BX
    
    return 0x4400 | (op << 8) | (h1 << 7) | (h2 << 6) | (rs_field << 3) | rd_field

def print_test_cases():
    """Generate test cases for Format 5 instructions"""
    
    print("=== ARM Thumb Format 5 Instruction Encodings ===\n")
    
    # ADD instructions
    print("ADD Instructions:")
    test_cases = [
        # (rd, rs, description)
        (0, 8, "ADD R0, R8 (lo + hi)"),
        (8, 0, "ADD R8, R0 (hi + lo)"),
        (8, 9, "ADD R8, R9 (hi + hi)"),
        (1, 2, "ADD R1, R2 (lo + lo)"),
        (15, 14, "ADD PC, LR"),
        (13, 8, "ADD SP, R8"),
        (8, 13, "ADD R8, SP"),
    ]
    
    for rd, rs, desc in test_cases:
        opcode = encode_format5_add(rd, rs)
        print(f"  {desc:<20} = 0x{opcode:04X}")
    
    # CMP instructions
    print("\nCMP Instructions:")
    test_cases = [
        (0, 8, "CMP R0, R8 (lo vs hi)"),
        (8, 0, "CMP R8, R0 (hi vs lo)"),
        (8, 9, "CMP R8, R9 (hi vs hi)"),
        (2, 1, "CMP R2, R1 (lo vs lo)"),
        (15, 14, "CMP PC, LR"),
        (13, 12, "CMP SP, R12"),
        (14, 13, "CMP LR, SP"),
    ]
    
    for rd, rs, desc in test_cases:
        opcode = encode_format5_cmp(rd, rs)
        print(f"  {desc:<20} = 0x{opcode:04X}")
    
    # MOV instructions
    print("\nMOV Instructions:")
    test_cases = [
        (0, 8, "MOV R0, R8 (hi to lo)"),
        (8, 0, "MOV R8, R0 (lo to hi)"),
        (8, 9, "MOV R8, R9 (hi to hi)"),
        (3, 4, "MOV R3, R4 (lo to lo)"),
        (15, 14, "MOV PC, LR"),
        (13, 12, "MOV SP, R12"),
        (14, 15, "MOV LR, PC"),
    ]
    
    for rd, rs, desc in test_cases:
        opcode = encode_format5_mov(rd, rs)
        print(f"  {desc:<20} = 0x{opcode:04X}")
    
    # BX instructions
    print("\nBX Instructions:")
    test_cases = [
        (0, "BX R0 (ARM/Thumb)"),
        (1, "BX R1 (ARM/Thumb)"),
        (8, "BX R8"),
        (14, "BX LR (return)"),
        (15, "BX PC"),
    ]
    
    for rs, desc in test_cases:
        opcode = encode_format5_bx(rs)
        print(f"  {desc:<20} = 0x{opcode:04X}")

def verify_encoding_patterns():
    """Verify encoding patterns match ARM documentation"""
    print("\n=== Encoding Pattern Verification ===\n")
    
    # Test base patterns
    base_patterns = {
        "ADD": 0x4400,  # 010001 00 0 0 000 000
        "CMP": 0x4500,  # 010001 01 0 0 000 000  
        "MOV": 0x4600,  # 010001 10 0 0 000 000
        "BX":  0x4700,  # 010001 11 0 0 000 000
    }
    
    for op, base in base_patterns.items():
        print(f"{op} base pattern: 0x{base:04X} = {base:016b}")
    
    # Test H1/H2 flag effects
    print("\nH1/H2 Flag Effects:")
    print(f"ADD R0, R0:  0x{encode_format5_add(0, 0):04X} (H1=0, H2=0)")
    print(f"ADD R8, R0:  0x{encode_format5_add(8, 0):04X} (H1=1, H2=0)")
    print(f"ADD R0, R8:  0x{encode_format5_add(0, 8):04X} (H1=0, H2=1)")
    print(f"ADD R8, R8:  0x{encode_format5_add(8, 8):04X} (H1=1, H2=1)")

if __name__ == "__main__":
    print_test_cases()
    verify_encoding_patterns()
