#!/usr/bin/env python3
"""
Disassemble a specific instruction from GBA BIOS
"""
import sys
from capstone import *

def disassemble_bios_instruction(pc_address):
    """Disassemble instruction at given PC from BIOS"""
    
    # Read BIOS
    with open('assets/bios.bin', 'rb') as f:
        bios_data = f.read()
    
    # Get instruction at PC
    if pc_address >= len(bios_data):
        print(f"Error: PC 0x{pc_address:08X} is outside BIOS range")
        return
    
    # Read 4 bytes for ARM instruction
    instr_bytes = bios_data[pc_address:pc_address+4]
    instr_value = int.from_bytes(instr_bytes, byteorder='little')
    
    print(f"PC: 0x{pc_address:08X}")
    print(f"Instruction bytes: {instr_bytes.hex()}")
    print(f"Instruction value: 0x{instr_value:08X}")
    print()
    
    # Disassemble with Capstone
    md = Cs(CS_ARCH_ARM, CS_MODE_ARM)
    md.detail = True
    
    for instr in md.disasm(instr_bytes, pc_address):
        print(f"Disassembly: {instr.mnemonic} {instr.op_str}")
        print(f"Size: {instr.size} bytes")
        
        # Show instruction details
        if len(instr.groups) > 0:
            print(f"Groups: {[instr.group_name(g) for g in instr.groups]}")
        
        # Check if it's a branch
        if instr.group(CS_GRP_JUMP) or instr.group(CS_GRP_CALL) or instr.group(CS_GRP_BRANCH_RELATIVE):
            print("→ This is a BRANCH instruction")
        elif instr.mnemonic in ['ldr', 'str', 'ldm', 'stm', 'ldrb', 'strb', 'ldrh', 'strh']:
            print("→ This is a LOAD/STORE instruction")
        else:
            print("→ This is a simple data processing instruction")

if __name__ == "__main__":
    # Instruction #31 is at PC 0x0000011C
    pc = 0x0000011C
    
    print("=" * 70)
    print("INSTRUCTION #31 ANALYSIS")
    print("=" * 70)
    disassemble_bios_instruction(pc)
    
    print("\n" + "=" * 70)
    print("EXPECTED TIMING (BIOS execution):")
    print("=" * 70)
    print("Simple data processing: prefetch (2) + extra (0) = 2 cycles")
    print("Branch: prefetch (2) + refill (2) = 4 cycles")
    print("Load/Store: prefetch (2) + data access (0 for BIOS) = 2 cycles")
