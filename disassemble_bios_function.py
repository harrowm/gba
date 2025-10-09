#!/usr/bin/env python3
"""
Disassemble BIOS function starting at 0x1928 using Capstone.
This function is the initialization routine that should return to 0xB4.
"""

from capstone import *
import sys

def disassemble_bios_function(bios_path, start_addr, max_instructions=5000):
    """
    Disassemble BIOS function starting at given address.
    Stops when it finds the return instruction (POP with PC or BX LR).
    """
    
    # Read BIOS file
    try:
        with open(bios_path, 'rb') as f:
            bios_data = f.read()
    except FileNotFoundError:
        print(f"Error: Could not find BIOS file at {bios_path}")
        return
    
    print(f"BIOS size: {len(bios_data)} bytes")
    print(f"Starting disassembly at 0x{start_addr:08X}")
    print("=" * 80)
    
    # Create Capstone disassembler for ARM (we'll switch to THUMB as needed)
    md_arm = Cs(CS_ARCH_ARM, CS_MODE_ARM | CS_MODE_LITTLE_ENDIAN)
    md_thumb = Cs(CS_ARCH_ARM, CS_MODE_THUMB | CS_MODE_LITTLE_ENDIAN)
    md_arm.detail = True
    md_thumb.detail = True
    
    # Start in THUMB mode at 0x1928 (based on BX instruction that got us here)
    current_mode = "THUMB"
    md = md_thumb
    current_addr = start_addr
    instruction_count = 0
    found_return = False
    
    # Track function stack depth (PUSH increases, POP with PC returns)
    stack_depth = 0
    
    output = []
    
    while instruction_count < max_instructions and current_addr < len(bios_data):
        # Get instruction bytes
        if current_mode == "THUMB":
            # THUMB instructions are 2 or 4 bytes
            inst_bytes = bios_data[current_addr:current_addr+4]
            disasm = list(md.disasm(inst_bytes, current_addr, count=1))
        else:
            # ARM instructions are 4 bytes
            inst_bytes = bios_data[current_addr:current_addr+4]
            disasm = list(md.disasm(inst_bytes, current_addr, count=1))
        
        # If we couldn't disassemble, skip forward
        if not disasm:
            output.append(f"[{instruction_count:5d}] 0x{current_addr:08X}: ??? [Could not disassemble]")
            current_addr += 2 if current_mode == "THUMB" else 4
            instruction_count += 1
            continue
        
        for inst in disasm:
            # Format instruction
            bytes_hex = ' '.join([f'{b:02X}' for b in inst.bytes])
            line = f"[{instruction_count:5d}] 0x{inst.address:08X}: {bytes_hex:12s} {inst.mnemonic:8s} {inst.op_str}"
            output.append(line)
            
            # Check for mode switches
            if inst.mnemonic in ['bx', 'blx']:
                # Could switch between ARM/THUMB
                # For now, assume we stay in THUMB unless we see explicit mode change
                pass
            
            # Check for PUSH (increases stack depth)
            if inst.mnemonic == 'push':
                stack_depth += 1
                output.append(f"         [STACK DEPTH: {stack_depth}]")
            
            # Check for POP with PC (function return)
            if inst.mnemonic == 'pop':
                if 'pc' in inst.op_str:
                    output.append(f"         [RETURN FOUND - POP with PC!]")
                    output.append(f"         [STACK DEPTH: {stack_depth}]")
                    # Check if this matches the initial PUSH {r4, r5, r6, r7, lr}
                    if stack_depth == 1 and 'r4' in inst.op_str and 'r7' in inst.op_str:
                        output.append(f"         [*** MAIN FUNCTION RETURN TO 0x000000B4 ***]")
                        found_return = True
                    elif stack_depth == 1:
                        output.append(f"         [Could be main return, but register mismatch]")
                    else:
                        output.append(f"         [Nested function return, not main return]")
                stack_depth = max(0, stack_depth - 1)
            
            # Check for BX LR (function return)
            if inst.mnemonic == 'bx' and 'lr' in inst.op_str:
                output.append(f"         [RETURN FOUND - BX LR!]")
                output.append(f"         [STACK DEPTH: {stack_depth}]")
                if stack_depth == 0:
                    output.append(f"         [THIS COULD RETURN TO 0x000000B4]")
                    # Don't mark found_return yet, as BX LR might be nested
            
            # Check for branch and link (function calls)
            if inst.mnemonic in ['bl', 'blx']:
                output.append(f"         [FUNCTION CALL to {inst.op_str}]")
            
            # Track branches
            if inst.mnemonic.startswith('b') and inst.mnemonic not in ['bl', 'blx', 'bx']:
                output.append(f"         [BRANCH to {inst.op_str}]")
            
            # Check for writes to interrupt registers
            if 'str' in inst.mnemonic:
                if 'r3' in inst.op_str or 'r0' in inst.op_str:
                    # Could be writing to IO registers
                    pass
            
            # Advance to next instruction
            current_addr = inst.address + inst.size
            instruction_count += 1
            
            if found_return:
                output.append("")
                output.append("=" * 80)
                output.append(f"FOUND MAIN RETURN AT INSTRUCTION {instruction_count}")
                output.append("=" * 80)
                break
        
        if found_return:
            break
    
    # Print all output
    for line in output:
        print(line)
    
    if not found_return:
        print("")
        print("=" * 80)
        print(f"WARNING: Did not find main return after {instruction_count} instructions")
        print("Function may be longer, or returns via BX LR at deeper stack level")
        print("=" * 80)
    
    print(f"\nTotal instructions disassembled: {instruction_count}")
    
    # Save to file
    output_file = "/Users/malcolm/gba/bios_function_0x1928.txt"
    with open(output_file, 'w') as f:
        for line in output:
            f.write(line + '\n')
        f.write(f"\nTotal instructions: {instruction_count}\n")
    
    print(f"\nDisassembly saved to: {output_file}")

if __name__ == "__main__":
    bios_path = "/Users/malcolm/gba/assets/bios.bin"
    start_addr = 0x1928
    max_instructions = 10000  # Increase to cover more of the function
    
    disassemble_bios_function(bios_path, start_addr, max_instructions)
