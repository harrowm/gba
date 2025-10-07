#!/usr/bin/env python3
"""
Disassemble the BIOS loop at 0x11C-0x127 using Capstone
"""
from capstone import *

# Read BIOS binary
with open('assets/bios.bin', 'rb') as f:
    bios = f.read()

# Initialize Capstone for ARM Thumb mode
md = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
md.detail = True

# Extract instructions from 0x11C to 0x127 (12 bytes)
start_addr = 0x11C
end_addr = 0x128
code = bios[start_addr:end_addr]

print("=" * 80)
print(f"BIOS Loop Disassembly (0x{start_addr:08X} - 0x{end_addr:08X})")
print("=" * 80)

for insn in md.disasm(code, start_addr):
    # Print instruction
    bytes_str = ' '.join(f'{b:02x}' for b in insn.bytes)
    print(f"0x{insn.address:08X}:  {bytes_str:16s}  {insn.mnemonic:8s} {insn.op_str}")
    
    # For branch instructions, show details
    if 'b' in insn.mnemonic and insn.mnemonic != 'bx':
        print(f"                     {'':16s}  └─> Branch to 0x{insn.operands[0].imm:08X}")
        
        # Calculate what the offset should be
        current_pc = insn.address
        target = insn.operands[0].imm
        # In Thumb, branches use PC+4 as base
        offset_from_pc4 = target - (current_pc + 4)
        offset_instructions = offset_from_pc4 // 2
        
        print(f"                     {'':16s}      PC+4 = 0x{current_pc + 4:08X}")
        print(f"                     {'':16s}      Offset = {offset_from_pc4} bytes ({offset_instructions} instructions)")

print("=" * 80)

# Now let's manually decode the branch instruction at 0x124
branch_addr = 0x124
branch_bytes = bios[branch_addr:branch_addr+2]
instruction = branch_bytes[0] | (branch_bytes[1] << 8)

print(f"\nManual decode of instruction at 0x{branch_addr:08X}:")
print(f"  Raw bytes: {branch_bytes[0]:02X} {branch_bytes[1]:02X}")
print(f"  As 16-bit value: 0x{instruction:04X}")
print(f"  Binary: {instruction:016b}")

# Decode conditional branch (Format 16): 1101 cccc oooo oooo
if (instruction >> 12) == 0b1101:
    condition = (instruction >> 8) & 0xF
    offset_raw = instruction & 0xFF
    
    # Sign extend 8-bit to 32-bit
    if offset_raw & 0x80:
        offset_signed = offset_raw | 0xFFFFFF00
        offset_signed = -(256 - offset_raw)  # Convert to negative
    else:
        offset_signed = offset_raw
    
    # Branch offset is multiplied by 2 and added to PC+4
    branch_offset = offset_signed * 2
    pc_at_branch = branch_addr + 4  # PC+4 in Thumb mode
    target_address = pc_at_branch + branch_offset
    
    conditions = ['eq', 'ne', 'cs', 'cc', 'mi', 'pl', 'vs', 'vc',
                  'hi', 'ls', 'ge', 'lt', 'gt', 'le', 'al', '??']
    
    print(f"  Format: Conditional Branch (Format 16)")
    print(f"  Condition: {condition} ({conditions[condition]})")
    print(f"  Raw offset: 0x{offset_raw:02X} ({offset_raw})")
    print(f"  Signed offset: {offset_signed}")
    print(f"  Branch offset (x2): {branch_offset} bytes")
    print(f"  PC at instruction: 0x{branch_addr:08X}")
    print(f"  PC+4: 0x{pc_at_branch:08X}")
    print(f"  Target: 0x{target_address:08X}")
    
    if target_address == 0x120:
        print(f"  ✅ CORRECT: Branch should go to loop start at 0x120")
    else:
        print(f"  ❌ ERROR: Branch goes to 0x{target_address:08X}, should be 0x120")

print("=" * 80)
