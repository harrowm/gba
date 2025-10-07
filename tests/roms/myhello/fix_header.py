#!/usr/bin/env python3
"""
Fix GBA ROM header with proper checksum
"""
import sys

def calculate_checksum(data):
    """Calculate GBA header checksum (simple 8-bit sum of bytes 0xA0-0xBC)"""
    checksum = 0
    for byte in data[0xA0:0xBD]:
        checksum = (checksum - byte) & 0xFF
    return checksum - 0x19

def fix_header(filename):
    """Fix the GBA ROM header"""
    with open(filename, 'r+b') as f:
        data = bytearray(f.read())
        
        # Make sure file is at least 192 bytes (header size)
        if len(data) < 0xC0:
            data.extend(b'\x00' * (0xC0 - len(data)))
        
        # Calculate and set checksum
        checksum = calculate_checksum(data)
        data[0xBD] = checksum & 0xFF
        
        # Write back
        f.seek(0)
        f.write(data)
        print(f"Fixed header checksum: 0x{checksum:02X}")

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: fix_header.py <rom.gba>")
        sys.exit(1)
    
    fix_header(sys.argv[1])
