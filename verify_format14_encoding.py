#!/usr/bin/env python3
"""
ARM Thumb Format 14: Push/Pop registers instruction encoding verification

Format 14 encoding:
PUSH: 1011 010R [register_list]
POP:  1011 110R [register_list]

Where:
- R = 0: Basic push/pop (R0-R7)
- R = 1: Push LR / Pop PC

Bit pattern:
15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 1  0  1  1  L  1  R  0  R7 R6 R5 R4 R3 R2 R1 R0

L = 0: PUSH (store)
L = 1: POP (load)
R = 0: No LR/PC
R = 1: Include LR (push) or PC (pop)
"""

def encode_push_instruction(register_list, include_lr=False):
    """Encode a PUSH instruction"""
    base = 0b1011010000000000  # 1011 010R 0000 0000
    if include_lr:
        base |= 0b0000000100000000  # Set R bit (bit 8)
    base |= register_list & 0xFF  # Add register list (bits 0-7)
    return base

def encode_pop_instruction(register_list, include_pc=False):
    """Encode a POP instruction"""
    base = 0b1011110000000000  # 1011 110R 0000 0000
    if include_pc:
        base |= 0b0000000100000000  # Set R bit (bit 8)
    base |= register_list & 0xFF  # Add register list (bits 0-7)
    return base

def register_list_to_string(register_list):
    """Convert register list to string representation"""
    regs = []
    for i in range(8):
        if register_list & (1 << i):
            regs.append(f"R{i}")
    return "{" + ", ".join(regs) + "}"

def test_push_encodings():
    """Test various PUSH instruction encodings"""
    print("PUSH Instruction Encodings:")
    print("=" * 50)
    
    test_cases = [
        (0b00000001, False, "PUSH {R0}"),
        (0b00000010, False, "PUSH {R1}"),
        (0b00000100, False, "PUSH {R2}"),
        (0b00001000, False, "PUSH {R3}"),
        (0b00010000, False, "PUSH {R4}"),
        (0b00100000, False, "PUSH {R5}"),
        (0b01000000, False, "PUSH {R6}"),
        (0b10000000, False, "PUSH {R7}"),
        (0b00000011, False, "PUSH {R0, R1}"),
        (0b11110000, False, "PUSH {R4, R5, R6, R7}"),
        (0b11111111, False, "PUSH {R0-R7}"),
        (0b00000001, True, "PUSH {R0, LR}"),
        (0b11111111, True, "PUSH {R0-R7, LR}"),
        (0b00000000, False, "PUSH {} (empty list)"),
        (0b00000000, True, "PUSH {LR} (LR only)"),
    ]
    
    for reg_list, include_lr, description in test_cases:
        instruction = encode_push_instruction(reg_list, include_lr)
        print(f"0x{instruction:04X} - {description}")
        print(f"  Binary: {instruction:016b}")
        print(f"  Register list: {register_list_to_string(reg_list)}")
        if include_lr:
            print(f"  Includes LR: Yes")
        print()

def test_pop_encodings():
    """Test various POP instruction encodings"""
    print("POP Instruction Encodings:")
    print("=" * 50)
    
    test_cases = [
        (0b00000001, False, "POP {R0}"),
        (0b00000010, False, "POP {R1}"),
        (0b00000100, False, "POP {R2}"),
        (0b00001000, False, "POP {R3}"),
        (0b00010000, False, "POP {R4}"),
        (0b00100000, False, "POP {R5}"),
        (0b01000000, False, "POP {R6}"),
        (0b10000000, False, "POP {R7}"),
        (0b00000011, False, "POP {R0, R1}"),
        (0b11110000, False, "POP {R4, R5, R6, R7}"),
        (0b11111111, False, "POP {R0-R7}"),
        (0b00000001, True, "POP {R0, PC}"),
        (0b11111111, True, "POP {R0-R7, PC}"),
        (0b00000000, False, "POP {} (empty list)"),
        (0b00000000, True, "POP {PC} (PC only)"),
    ]
    
    for reg_list, include_pc, description in test_cases:
        instruction = encode_pop_instruction(reg_list, include_pc)
        print(f"0x{instruction:04X} - {description}")
        print(f"  Binary: {instruction:016b}")
        print(f"  Register list: {register_list_to_string(reg_list)}")
        if include_pc:
            print(f"  Includes PC: Yes")
        print()

def verify_instruction_table_mappings():
    """Verify that our encodings match the instruction table mappings"""
    print("Instruction Table Mapping Verification:")
    print("=" * 50)
    
    # From the source code:
    # thumb_instruction_table[0b10110100] = &ThumbCPU::handle_thumb_push_registers;
    # thumb_instruction_table[0b10110101] = &ThumbCPU::handle_thumb_push_registers_and_lr;
    # thumb_instruction_table[0b10111100] = &ThumbCPU::handle_thumb_pop_registers;
    # thumb_instruction_table[0b10111101] = &ThumbCPU::handle_thumb_pop_registers_and_pc;
    
    expected_mappings = [
        (0b10110100, "handle_thumb_push_registers"),
        (0b10110101, "handle_thumb_push_registers_and_lr"),
        (0b10111100, "handle_thumb_pop_registers"),
        (0b10111101, "handle_thumb_pop_registers_and_pc"),
    ]
    
    for opcode, handler in expected_mappings:
        print(f"0x{opcode:04X} ({opcode:08b}) -> {handler}")
    
    print("\nOur encoding verification:")
    print(f"PUSH {{R0}}: 0x{encode_push_instruction(0b00000001, False):04X}")
    print(f"PUSH {{R0, LR}}: 0x{encode_push_instruction(0b00000001, True):04X}")
    print(f"POP {{R0}}: 0x{encode_pop_instruction(0b00000001, False):04X}")
    print(f"POP {{R0, PC}}: 0x{encode_pop_instruction(0b00000001, True):04X}")

if __name__ == "__main__":
    test_push_encodings()
    print()
    test_pop_encodings()
    print()
    verify_instruction_table_mappings()
