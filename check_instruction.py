#!/usr/bin/env python3

def encode_format10_instruction(is_load, rd, rb, offset5):
    """
    Encode a Format 10 STRH/LDRH immediate offset instruction.
    
    Format: 1000 L offset5 rb rd
    - L = 0 for STRH, 1 for LDRH
    - offset5 = 5-bit immediate offset (scaled by 2)
    - rb = 3-bit base register (bits 5-3)
    - rd = 3-bit destination/source register (bits 2-0)
    """
    instruction = 0x8000  # Base opcode for Format 10
    instruction |= (is_load << 11)     # L bit (bit 11)
    instruction |= (offset5 << 6)      # offset5 (bits 10-6)
    instruction |= (rb << 3)           # rb (bits 5-3)
    instruction |= rd                  # rd (bits 2-0)
    return instruction

# Test the specific case: STRH R6, [R6, #4]
# offset #4 means offset5 = 2 (since 2 << 1 = 4)
rd = 6   # source register
rb = 6   # base register  
offset5 = 2  # immediate offset (4 >> 1)
is_load = 0  # STRH

expected = encode_format10_instruction(is_load, rd, rb, offset5)
print(f"STRH R6, [R6, #4]: Expected 0x{expected:04X}")

# Let's also test what 0x8106 actually decodes to
instruction = 0x8106
is_load = (instruction >> 11) & 1
offset5 = (instruction >> 6) & 0x1F
rb = (instruction >> 3) & 0x07
rd = instruction & 0x07

print(f"Instruction 0x8106 decodes to:")
print(f"  is_load={is_load}, offset5={offset5}, rb={rb}, rd={rd}")
print(f"  This is: {'LDRH' if is_load else 'STRH'} R{rd}, [R{rb}, #{offset5 << 1}]")
