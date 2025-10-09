#!/usr/bin/env python3
"""
Disassemble the BIOS loop code at specific addresses: 0xD58, 0x1A28, 0x400, 0x77A
"""

from capstone import *

def disassemble_at_address(bios_data, address, num_instructions=20, is_thumb=False):
    """
    Disassemble starting at a specific address.
    """
    mode = "THUMB" if is_thumb else "ARM"
    if is_thumb:
        md = Cs(CS_ARCH_ARM, CS_MODE_THUMB | CS_MODE_LITTLE_ENDIAN)
    else:
        md = Cs(CS_ARCH_ARM, CS_MODE_ARM | CS_MODE_LITTLE_ENDIAN)
    md.detail = True
    
    print(f"\n{'='*80}")
    print(f"Disassembly at 0x{address:08X} ({mode} mode)")
    print(f"{'='*80}")
    
    # Disassemble
    code = bios_data[address:]
    count = 0
    for insn in md.disasm(code, address):
        if count >= num_instructions:
            break
            
        # Format instruction
        print(f"0x{insn.address:08X}: {insn.bytes.hex():16s} {insn.mnemonic:12s} {insn.op_str}")
        
        # Show extra details for interesting instructions
        if insn.mnemonic in ['b', 'beq', 'bne', 'bx', 'bl', 'blx']:
            print(f"            -> BRANCH/CALL")
        elif insn.mnemonic in ['ldr', 'ldrh', 'ldrb']:
            print(f"            -> LOAD from memory")
        elif insn.mnemonic in ['str', 'strh', 'strb']:
            print(f"            -> STORE to memory")
        elif insn.mnemonic in ['cmp', 'tst']:
            print(f"            -> COMPARE/TEST (sets condition flags)")
            
        count += 1
    
    print()

def main():
    # Read BIOS file
    try:
        with open('assets/bios.bin', 'rb') as f:
            bios_data = f.read()
    except FileNotFoundError:
        print("Error: Could not find assets/bios.bin")
        return
    
    print(f"BIOS size: {len(bios_data)} bytes (0x{len(bios_data):X})")
    print("\nAnalyzing the infinite loop pattern:")
    print("Pattern from trace: 0xD58 → 0x1A28 → 0x400 → 0x77A → (repeats)")
    
    # Disassemble each address in the loop
    # From trace: 0xD58 (ARM) -> BX to 0x1A28 (THUMB) -> BX to 0x400 (ARM) -> BX to 0x77A (THUMB) -> repeat
    loop_addresses = [
        (0xD58, "Loop start - ARM mode (BX to 0x1A28)", False),
        (0x1A28, "After first BX - THUMB mode (writes to registers, BX to 0x400)", True),
        (0x400, "After second BX - ARM mode (BX to 0x77A)", False),
        (0x77A, "After third BX - THUMB mode (returns back to continue)", True),
    ]
    
    for addr, description, is_thumb in loop_addresses:
        print(f"\n{'#'*80}")
        print(f"# {description}")
        print(f"{'#'*80}")
        disassemble_at_address(bios_data, addr, num_instructions=25, is_thumb=is_thumb)

if __name__ == "__main__":
    main()
