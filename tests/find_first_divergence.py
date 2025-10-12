#!/usr/bin/env python3
"""
Find the very first register divergence between mGBA and our emulator
"""

def parse_mgba_registers(reg_line):
    """Parse mGBA GDB register format"""
    if 'Registers: ' in reg_line:
        reg_line = reg_line.split('Registers: ')[1]
    
    if reg_line.startswith('+$'):
        reg_line = reg_line[2:]
    elif reg_line.startswith('$'):
        reg_line = reg_line[1:]
    
    reg_line = reg_line.replace('...', '').strip()
    
    registers = {}
    for i in range(16):
        offset = i * 8
        if offset + 8 <= len(reg_line):
            hex_val = reg_line[offset:offset+8]
            try:
                value = int.from_bytes(bytes.fromhex(hex_val), 'little')
                registers[i] = value
            except ValueError:
                break
    
    return registers

def parse_our_regs(lines, start_idx):
    """Parse our emulator's register format"""
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
                    elif reg_name == 'sp':
                        regs[13] = int(val, 16)
                    elif reg_name == 'lr':
                        regs[14] = int(val, 16)
    return regs

def main():
    print("=" * 80)
    print("FINDING FIRST DIVERGENCE FROM INSTRUCTION #1")
    print("=" * 80)
    
    # Load traces
    print("\nLoading traces...")
    with open('/tmp/mgba_memory_trace.log', 'r') as f:
        mgba_lines = f.readlines()
    
    with open('/tmp/gba_memory_trace.log', 'r') as f:
        our_lines = f.readlines()
    
    print(f"mGBA trace: {len(mgba_lines)} lines")
    print(f"Our trace:  {len(our_lines)} lines")
    
    print("\nScanning for first divergence...")
    print("-" * 80)
    
    first_divergence = None
    
    # Scan through instructions
    for instr_num in range(1, 44580):
        # Print progress every 100 instructions
        if instr_num % 100 == 0:
            print(f"Comparing instruction #{instr_num}...", flush=True)
        
        # Find in mGBA trace
        mgba_regs = None
        mgba_pc = None
        for i, line in enumerate(mgba_lines):
            if line.strip() == f'Instruction #{instr_num}':
                mgba_pc_line = mgba_lines[i+2].strip()
                mgba_pc = int(mgba_pc_line.split('0x')[1], 16) if '0x' in mgba_pc_line else 0
                mgba_reg_line = mgba_lines[i+3].strip()
                mgba_regs = parse_mgba_registers(mgba_reg_line)
                break
        
        # Find in our trace
        our_regs = None
        our_pc = None
        for i, line in enumerate(our_lines):
            if line.strip() == f'Instruction #{instr_num}':
                our_pc_line = our_lines[i+2].strip()
                our_pc = int(our_pc_line.split('0x')[1], 16) if '0x' in our_pc_line else 0
                our_regs = parse_our_regs(our_lines, i+3)
                break
        
        if not mgba_regs or not our_regs:
            continue
        
        # Compare all registers
        diverged = False
        diverged_regs = []
        
        for reg_num in range(16):
            mgba_val = mgba_regs.get(reg_num, 0)
            our_val = our_regs.get(reg_num, 0)
            
            if mgba_val != our_val:
                diverged = True
                diverged_regs.append({
                    'num': reg_num,
                    'mgba': mgba_val,
                    'ours': our_val
                })
        
        if diverged:
            first_divergence = {
                'instr_num': instr_num,
                'mgba_pc': mgba_pc,
                'our_pc': our_pc,
                'diverged_regs': diverged_regs
            }
            break
        
        # Print progress every 1000 instructions
        if instr_num % 1000 == 0:
            print(f"  Checked up to instruction #{instr_num} - all match so far")
    
    if first_divergence:
        print("\n" + "=" * 80)
        print("FIRST DIVERGENCE FOUND!")
        print("=" * 80)
        print(f"\nInstruction #{first_divergence['instr_num']}")
        print(f"mGBA PC: 0x{first_divergence['mgba_pc']:08X}")
        print(f"Our PC:  0x{first_divergence['our_pc']:08X}")
        
        if first_divergence['mgba_pc'] != first_divergence['our_pc']:
            print(f"\n⚠️  PC MISMATCH!")
        
        print(f"\nDiverged registers:")
        for reg in first_divergence['diverged_regs']:
            reg_name = f"r{reg['num']}"
            if reg['num'] == 13:
                reg_name = "r13 (sp)"
            elif reg['num'] == 14:
                reg_name = "r14 (lr)"
            elif reg['num'] == 15:
                reg_name = "r15 (pc)"
            
            print(f"  {reg_name:10s}  mGBA=0x{reg['mgba']:08X}  Ours=0x{reg['ours']:08X}  diff=0x{abs(reg['mgba']-reg['ours']):08X}")
        
        # Now check the PREVIOUS instruction to see what changed
        prev_instr = first_divergence['instr_num'] - 1
        print(f"\n" + "-" * 80)
        print(f"Checking instruction #{prev_instr} (the one that caused divergence)")
        print("-" * 80)
        
        # Get previous instruction registers
        for i, line in enumerate(mgba_lines):
            if line.strip() == f'Instruction #{prev_instr}':
                prev_mgba_pc_line = mgba_lines[i+2].strip()
                prev_mgba_pc = int(prev_mgba_pc_line.split('0x')[1], 16) if '0x' in prev_mgba_pc_line else 0
                prev_mgba_reg_line = mgba_lines[i+3].strip()
                prev_mgba_regs = parse_mgba_registers(prev_mgba_reg_line)
                print(f"\nmGBA Instruction #{prev_instr} at PC=0x{prev_mgba_pc:08X}")
                break
        
        for i, line in enumerate(our_lines):
            if line.strip() == f'Instruction #{prev_instr}':
                prev_our_pc_line = our_lines[i+2].strip()
                prev_our_pc = int(prev_our_pc_line.split('0x')[1], 16) if '0x' in prev_our_pc_line else 0
                prev_our_regs = parse_our_regs(our_lines, i+3)
                print(f"Our Instruction #{prev_instr} at PC=0x{prev_our_pc:08X}")
                break
        
        print(f"\nThis instruction CAUSED the divergence in:")
        for reg in first_divergence['diverged_regs']:
            print(f"  r{reg['num']}")
        
    else:
        print("\nNo divergence found in first 1000 instructions!")

if __name__ == '__main__':
    main()
