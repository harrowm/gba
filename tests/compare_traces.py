#!/usr/bin/env python3
"""
Compare mGBA and our emulator traces instruction by instruction
"""

def parse_instruction(lines, start_idx):
    """Parse one instruction block from trace"""
    if start_idx >= len(lines):
        return None
    
    # Find instruction number
    line = lines[start_idx].strip()
    if not line.startswith('Instruction #'):
        return None
    
    instr_num = int(line.split('#')[1])
    
    # Parse PC
    pc = None
    registers = {}
    cpsr = None
    
    for i in range(start_idx + 1, min(start_idx + 20, len(lines))):
        line = lines[i].strip()
        
        if line.startswith('PC:'):
            pc = int(line.split('0x')[1], 16)
        elif line.startswith('cpsr='):
            cpsr = line
        elif line.startswith('r') or line.startswith(' r'):
            # Parse register line
            parts = line.split()
            for part in parts:
                if '=' in part:
                    reg, val = part.split('=')
                    registers[reg] = val
        elif line.startswith('Instruction #') or line.startswith('Memory:'):
            break
    
    return {
        'num': instr_num,
        'pc': pc,
        'registers': registers,
        'cpsr': cpsr
    }

def main():
    print("Comparing traces instruction by instruction...\n")
    
    # Load traces
    with open('/tmp/mgba_memory_trace.log', 'r') as f:
        mgba_lines = f.readlines()
    
    with open('/tmp/gba_memory_trace.log', 'r') as f:
        our_lines = f.readlines()
    
    # Find all instruction starts
    mgba_instrs = []
    our_instrs = []
    
    for i, line in enumerate(mgba_lines):
        if line.strip().startswith('Instruction #'):
            instr = parse_instruction(mgba_lines, i)
            if instr:
                mgba_instrs.append(instr)
    
    for i, line in enumerate(our_lines):
        if line.strip().startswith('Instruction #'):
            instr = parse_instruction(our_lines, i)
            if instr:
                our_instrs.append(instr)
    
    print(f"Found {len(mgba_instrs)} mGBA instructions")
    print(f"Found {len(our_instrs)} our instructions\n")
    
    # Compare instruction by instruction
    max_instrs = min(len(mgba_instrs), len(our_instrs))
    
    for i in range(max_instrs):
        mgba = mgba_instrs[i]
        ours = our_instrs[i]
        
        # Check PC
        if mgba['pc'] != ours['pc']:
            print(f"#{i+1}: ❌ DIFFERENT - PC mismatch!")
            print(f"  mGBA PC: 0x{mgba['pc']:08X}")
            print(f"  Ours PC: 0x{ours['pc']:08X}")
            break
        
        # Check registers
        diff_regs = []
        all_regs = set(mgba['registers'].keys()) | set(ours['registers'].keys())
        for reg in sorted(all_regs):
            mgba_val = mgba['registers'].get(reg, 'N/A')
            our_val = ours['registers'].get(reg, 'N/A')
            if mgba_val != our_val:
                diff_regs.append(f"{reg}: mGBA={mgba_val} Ours={our_val}")
        
        # Check CPSR
        cpsr_diff = mgba['cpsr'] != ours['cpsr']
        
        if diff_regs or cpsr_diff:
            print(f"#{i+1}: ❌ DIFFERENT - PC=0x{mgba['pc']:08X}")
            if diff_regs:
                print(f"  Register differences:")
                for d in diff_regs:
                    print(f"    {d}")
            if cpsr_diff:
                print(f"  CPSR difference:")
                print(f"    mGBA: {mgba['cpsr']}")
                print(f"    Ours: {ours['cpsr']}")
            break
        else:
            print(f"#{i+1}: ✓ SAME - PC=0x{mgba['pc']:08X}")

if __name__ == '__main__':
    main()
