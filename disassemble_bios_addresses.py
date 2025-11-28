#!/usr/bin/env python3
"""Disassemble specific BIOS addresses to investigate palette write issue."""

from capstone import *
from capstone.arm import *

def disassemble_region(bios_data, address, size, mode):
    """Disassemble a region of BIOS code."""
    if mode == 'thumb':
        md = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
    else:
        md = Cs(CS_ARCH_ARM, CS_MODE_ARM)
    
    md.detail = True
    
    print(f"\n{'='*70}")
    print(f"Disassembly at 0x{address:08X} ({mode.upper()} mode, {size} bytes)")
    print(f"{'='*70}")
    
    code = bios_data[address:address+size]
    for insn in md.disasm(code, address):
        print(f"0x{insn.address:08X}:  {insn.bytes.hex():12s}  {insn.mnemonic:8s} {insn.op_str}")
        
        # Show details for branch/call instructions
        if insn.mnemonic in ['bl', 'blx', 'b', 'bx']:
            print(f"              └─> Branch instruction")

def main():
    # Load BIOS
    with open('assets/bios.bin', 'rb') as f:
        bios = f.read()
    
    print("GBA BIOS Disassembly - Investigating Palette Write Issue")
    print(f"BIOS size: {len(bios)} bytes")
    
    # PC where garbage palette writes happen: 0x7FA (Thumb mode)
    # Actual instruction is at 0x7FA-2 (because PC is current instruction + 4 in ARM/Thumb)
    print("\n" + "="*70)
    print("KEY FINDINGS:")
    print("  - Garbage writes #2-1471 from PC=0x7FA (R4=garbage colors)")
    print("  - Correct writes #1472+ from PC=0x7FA (R4=0x7C00/7D4A/7ED6)")
    print("  - SAME code location, DIFFERENT source data!")
    print("  - R4 contains color value (built from R3/R5/R6)")
    print("  - Need to find where R4 source data comes from")
    print("="*70)
    
    # Disassemble the palette write function
    disassemble_region(bios, 0x7BC, 0x50, 'thumb')
    
    # Check the constant pool address referenced at 0x7F4
    # LDR r3, [pc, #0x2d8] at 0x7F4
    # PC at 0x7F4 is 0x7F4+4 = 0x7F8 (pipeline)
    # Word-aligned: 0x7F8 & ~3 = 0x7F8
    # Target: 0x7F8 + 0x2d8 = 0xAD0
    const_pool_addr = 0xAD0
    const_value = int.from_bytes(bios[const_pool_addr:const_pool_addr+4], 'little')
    print(f"\n" + "="*70)
    print(f"Constant pool analysis:")
    print(f"  LDR at 0x7F4: loads from PC+0x2d8")
    print(f"  PC (pipeline): 0x7F8")
    print(f"  Target address: 0x{const_pool_addr:08X}")
    print(f"  Value loaded into R3: 0x{const_value:08X}")
    print(f"  This is the BASE ADDRESS for palette writes!")
    print("="*70)
    
    # Disassemble around the return address 0x1AC3 - the caller
    disassemble_region(bios, 0x1AA0, 0x80, 'thumb')

if __name__ == '__main__':
    main()
