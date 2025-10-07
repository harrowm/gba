#!/usr/bin/env python3

from capstone import *
import struct
import sys

def disassemble_bios(start_addr, end_addr, mode='thumb'):
    # Read BIOS
    with open('assets/bios.bin', 'rb') as f:
        bios = f.read()
    
    # Create Capstone disassembler
    if mode == 'thumb':
        md = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
    else:
        md = Cs(CS_ARCH_ARM, CS_MODE_ARM)
    
    # Get the code slice
    code = bios[start_addr:end_addr]
    
    print(f"\nDisassembly from 0x{start_addr:04X} to 0x{end_addr:04X} ({mode} mode):")
    print("=" * 70)
    
    for insn in md.disasm(code, start_addr):
        print(f"0x{insn.address:04X}:  {insn.bytes.hex():8s}  {insn.mnemonic:8s} {insn.op_str}")
    
    print("=" * 70)

if __name__ == '__main__':
    if len(sys.argv) > 1:
        start = int(sys.argv[1], 16)
        end = int(sys.argv[2], 16) if len(sys.argv) > 2 else start + 32
        mode = sys.argv[3] if len(sys.argv) > 3 else 'thumb'
        disassemble_bios(start, end, mode)
    else:
        # Default: disassemble the loop we found at 0x120
        print("\n=== BIOS Loop at 0x120-0x12C (Thumb) ===")
        disassemble_bios(0x110, 0x140, 'thumb')
        
        print("\n=== BIOS Entry Point (ARM) ===")
        disassemble_bios(0x000, 0x080, 'arm')
        
        print("\n=== BIOS around 0x68 (ARM) ===")
        disassemble_bios(0x068, 0x0B0, 'arm')
