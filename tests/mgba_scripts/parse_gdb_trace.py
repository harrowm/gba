#!/usr/bin/env python3
"""
Parse GDB trace log and convert raw hex register dumps into readable format.
GDB 'g' command returns registers in target byte order (little-endian for ARM).
"""

import sys
import re

def parse_hex_registers(hex_str):
    """Parse GDB register response into individual register values."""
    # Remove GDB packet framing (+$ prefix and #checksum)
    hex_str = re.sub(r'^\+\$', '', hex_str)
    hex_str = re.sub(r'#[0-9a-fA-F]{2}$', '', hex_str)
    
    # GDB returns registers as hex bytes in little-endian order
    # Each register is 4 bytes (8 hex chars)
    registers = {}
    
    # Parse ARM registers: r0-r15 (16 registers * 8 hex chars = 128 chars)
    reg_names = ['r0', 'r1', 'r2', 'r3', 'r4', 'r5', 'r6', 'r7',
                 'r8', 'r9', 'r10', 'r11', 'r12', 'sp', 'lr', 'pc']
    
    pos = 0
    for i, name in enumerate(reg_names):
        if pos + 8 <= len(hex_str):
            # Extract 8 hex chars (4 bytes) in little-endian
            hex_bytes = hex_str[pos:pos+8]
            # Convert from little-endian hex string to integer
            # e.g., "0300007f" -> 0x7f000003
            value = int.from_bytes(bytes.fromhex(hex_bytes), byteorder='little')
            registers[name] = value
            pos += 8
    
    # Parse CPSR (after r0-r15)
    if pos + 8 <= len(hex_str):
        hex_bytes = hex_str[pos:pos+8]
        cpsr = int.from_bytes(bytes.fromhex(hex_bytes), byteorder='little')
        registers['cpsr'] = cpsr
        
        # Decode CPSR flags
        registers['flags'] = {
            'N': (cpsr >> 31) & 1,  # Negative
            'Z': (cpsr >> 30) & 1,  # Zero
            'C': (cpsr >> 29) & 1,  # Carry
            'V': (cpsr >> 28) & 1,  # Overflow
            'I': (cpsr >> 7) & 1,   # IRQ disable
            'F': (cpsr >> 6) & 1,   # FIQ disable
            'T': (cpsr >> 5) & 1,   # Thumb mode
            'mode': cpsr & 0x1F     # Processor mode
        }
        
        # Mode names
        mode_names = {
            0x10: 'User',
            0x11: 'FIQ',
            0x12: 'IRQ',
            0x13: 'Supervisor',
            0x17: 'Abort',
            0x1B: 'Undefined',
            0x1F: 'System'
        }
        registers['mode_name'] = mode_names.get(registers['flags']['mode'], 'Unknown')
    
    return registers

def format_registers(regs):
    """Format registers as readable string."""
    lines = []
    
    # Main registers in rows of 4
    for i in range(0, 16, 4):
        row = []
        for j in range(4):
            reg_idx = i + j
            if reg_idx < 13:
                name = f'r{reg_idx}'
            elif reg_idx == 13:
                name = 'sp'
            elif reg_idx == 14:
                name = 'lr'
            else:
                name = 'pc'
            
            if name in regs:
                row.append(f'{name:>3s}=0x{regs[name]:08X}')
        lines.append('  '.join(row))
    
    # CPSR and flags
    if 'cpsr' in regs:
        flags = regs['flags']
        flag_str = f"N={flags['N']} Z={flags['Z']} C={flags['C']} V={flags['V']} " \
                   f"I={flags['I']} F={flags['F']} T={flags['T']}"
        lines.append(f"cpsr=0x{regs['cpsr']:08X} [{flag_str}] Mode: {regs['mode_name']}")
    
    return '\n'.join(lines)

def main():
    if len(sys.argv) < 2:
        print("Usage: parse_gdb_trace.py <trace_log_file>")
        sys.exit(1)
    
    trace_file = sys.argv[1]
    
    with open(trace_file, 'r') as f:
        lines = f.readlines()
    
    instruction_num = 0
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        
        # Look for instruction marker
        if line.startswith('Instruction #'):
            instruction_num = int(line.split('#')[1])
            print(f"\n{'='*70}")
            print(f"Instruction #{instruction_num}")
            print('='*70)
            
            # Next line should be "Registers: ..."
            if i + 1 < len(lines):
                reg_line = lines[i + 1].strip()
                if reg_line.startswith('Registers:'):
                    hex_data = reg_line.split(':', 1)[1].strip()
                    regs = parse_hex_registers(hex_data)
                    print(format_registers(regs))
        
        i += 1

if __name__ == '__main__':
    main()
