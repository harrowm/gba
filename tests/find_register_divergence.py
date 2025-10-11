#!/usr/bin/env python3
"""
Find when r1 and r12 values first diverged between the two emulators
"""

def parse_mgba_registers(reg_string):
    """Parse mGBA GDB register format"""
    if 'Registers: ' in reg_string:
        reg_string = reg_string.split('Registers: ')[1]
    
    if reg_string.startswith('+$'):
        reg_string = reg_string[2:]
    elif reg_string.startswith('$'):
        reg_string = reg_string[1:]
    
    reg_string = reg_string.replace('...', '').strip()
    
    registers = {}
    for i in range(16):
        offset = i * 8
        if offset + 8 <= len(reg_string):
            hex_val = reg_string[offset:offset + 8]
            try:
                value = int.from_bytes(bytes.fromhex(hex_val), 'little')
                registers[i] = value
            except ValueError:
                break
    
    return registers

def parse_our_regs(lines, start_idx):
    """Parse our emulator's register format"""
    # Looking for lines like: r0=0x00000001   r1=0x00000000 ...
    regs = {}
    for i in range(start_idx, start_idx + 10):
        if i >= len(lines):
            break
        line = lines[i].strip()
        if line.startswith('r') and '=' in line:
            parts = line.split()
            for part in parts:
                if '=' in part:
                    reg_name, val = part.split('=')
                    if reg_name.startswith('r') and reg_name[1:].isdigit():
                        reg_num = int(reg_name[1:])
                        regs[reg_num] = int(val, 16)
    return regs

def main():
    print("Finding Register Divergence Point")
    print("=" * 70)
    
    # Read both traces
    with open('/tmp/mgba_memory_trace.log', 'r') as f:
        mgba_lines = f.readlines()
    
    with open('/tmp/gba_memory_trace.log', 'r') as f:
        our_lines = f.readlines()
    
    # Check instructions going backwards
    for instr_num in range(450, 520, 5):
        print(f"\nInstruction #{instr_num}:")
        print("-" * 70)
        
        # Find in mGBA trace
        for i, line in enumerate(mgba_lines):
            if line.strip() == f'Instruction #{instr_num}':
                mgba_pc = mgba_lines[i+2].strip()
                mgba_reg_line = mgba_lines[i+3].strip()
                mgba_regs = parse_mgba_registers(mgba_reg_line)
                print(f"mGBA:  {mgba_pc}")
                print(f"       r1={mgba_regs.get(1, 0):08X}  r12={mgba_regs.get(12, 0):08X}")
                break
        
        # Find in our trace
        for i, line in enumerate(our_lines):
            if line.strip() == f'Instruction #{instr_num}':
                our_pc = our_lines[i+2].strip()
                our_regs = parse_our_regs(our_lines, i+3)
                print(f"Ours:  {our_pc}")
                print(f"       r1={our_regs.get(1, 0):08X}  r12={our_regs.get(12, 0):08X}")
                
                # Compare
                if mgba_regs.get(1) != our_regs.get(1) or mgba_regs.get(12) != our_regs.get(12):
                    print(f"\n⚠️  REGISTER MISMATCH!")
                    print(f"   r1:  mGBA=0x{mgba_regs.get(1, 0):08X}, Ours=0x{our_regs.get(1, 0):08X}")
                    print(f"   r12: mGBA=0x{mgba_regs.get(12, 0):08X}, Ours=0x{our_regs.get(12, 0):08X}")
                break

if __name__ == '__main__':
    main()
