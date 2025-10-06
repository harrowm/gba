#!/usr/bin/env python3
"""
Fix GBA ROM header checksum.
The checksum is stored at offset 0xBD and is calculated from the bytes at 0xA0-0xBC.
"""

import sys

def fix_gba_header(rom_path):
    """Fix the header checksum of a GBA ROM."""
    with open(rom_path, 'r+b') as f:
        # Read the ROM
        data = bytearray(f.read())
        
        if len(data) < 0xC0:
            print(f"Error: ROM too small ({len(data)} bytes, need at least 192)")
            return False
        
        # Calculate checksum from bytes 0xA0 to 0xBC (game title, game code, maker code, etc.)
        checksum = 0
        for i in range(0xA0, 0xBD):
            checksum = (checksum - data[i]) & 0xFF
        
        # Subtract 0x19 (fixed constant)
        checksum = (checksum - 0x19) & 0xFF
        
        # Write checksum at 0xBD
        old_checksum = data[0xBD]
        data[0xBD] = checksum
        
        # Write back
        f.seek(0)
        f.write(data)
        
        print(f"Fixed GBA header checksum:")
        print(f"  Old checksum: 0x{old_checksum:02X}")
        print(f"  New checksum: 0x{checksum:02X}")
        print(f"  ROM: {rom_path}")
        
        return True

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: fix_header.py <rom.gba>")
        sys.exit(1)
    
    if fix_gba_header(sys.argv[1]):
        sys.exit(0)
    else:
        sys.exit(1)
