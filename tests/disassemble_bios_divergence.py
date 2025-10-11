#!/usr/bin/env python3
"""
Disassemble BIOS code around the divergence point using Capstone
"""

from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB
import sys

def disassemble_bios_thumb(bios_path, start_addr, count=20):
    """Disassemble Thumb code from BIOS"""
    # Read BIOS
    with open(bios_path, 'rb') as f:
        bios = f.read()
    
    # Create Capstone disassembler for Thumb mode
    md = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
    md.detail = True
    
    # Extract code around the address
    offset = start_addr
    code = bios[offset:offset + count * 4]  # Get enough bytes
    
    print(f"\nDisassembly at 0x{start_addr:08X} (Thumb mode):")
    print("=" * 70)
    
    for i in md.disasm(code, start_addr):
        print(f"0x{i.address:08X}:  {i.mnemonic:8s} {i.op_str}")
    
    print("=" * 70)

def main():
    bios_path = "/Users/malcolm/gba/assets/bios.bin"
    
    print("BIOS Divergence Analysis")
    print("=" * 70)
    print("\nOur emulator path:")
    print("  Instruction #38079: PC=0x00000816")
    print("  Instruction #38080: PC=0x00000818")
    print("  Instruction #38081: PC=0x0000081A")
    print()
    print("mGBA path:")
    print("  Instruction #38079: PC=0x00000816")
    print("  Instruction #38080: PC=0x0000082C (divergence!)")
    print("  Instruction #38081: PC=0x0000194E")
    
    # Disassemble around 0x814-0x830 to see both paths
    print("\n\nDisassembling code from 0x814 onwards:")
    disassemble_bios_thumb(bios_path, 0x814, 30)
    
    print("\n\nDisassembling code at branch target 0x82C:")
    disassemble_bios_thumb(bios_path, 0x82C, 10)
    
    print("\n\nDisassembling code at return address 0x194E:")
    disassemble_bios_thumb(bios_path, 0x194E, 10)

if __name__ == '__main__':
    main()
