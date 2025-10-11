#!/usr/bin/env python3
"""
Find the last instruction where r1 and r12 matched between mGBA and our emulator
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
    return regs

def main():
    print("Finding Last Matching Register State")
    print("=" * 70)
    
    # Read both traces
    with open('/tmp/mgba_memory_trace.log', 'r') as f:
        mgba_lines = f.readlines()
    
    with open('/tmp/gba_memory_trace.log', 'r') as f:
        our_lines = f.readlines()
    
    last_match = None
    first_mismatch = None
    
    print("Searching backwards from instruction 38070...")
    
    # Check from instruction 38070 backwards to 38000
    for instr_num in range(38070, 38000, -1):
        if instr_num % 10 == 0:
            print(f"  Checking instruction {instr_num}...")
        # Find in mGBA trace
        mgba_regs = None
        mgba_pc = None
        for i, line in enumerate(mgba_lines):
            if line.strip() == f'Instruction #{instr_num}':
                mgba_pc = mgba_lines[i+2].strip()
                mgba_reg_line = mgba_lines[i+3].strip()
                mgba_regs = parse_mgba_registers(mgba_reg_line)
                break
        
        # Find in our trace
        our_regs = None
        our_pc = None
        for i, line in enumerate(our_lines):
            if line.strip() == f'Instruction #{instr_num}':
                our_pc = our_lines[i+2].strip()
                our_regs = parse_our_regs(our_lines, i+3)
                break
        
        if mgba_regs and our_regs:
            # Check if r1 and r12 match
            mgba_r1 = mgba_regs.get(1, -1)
            mgba_r12 = mgba_regs.get(12, -1)
            our_r1 = our_regs.get(1, -1)
            our_r12 = our_regs.get(12, -1)
            
            match = (mgba_r1 == our_r1 and mgba_r12 == our_r12)
            
            if match:
                if last_match is None:
                    last_match = instr_num
                    print(f"\n✓ Last match found at instruction #{instr_num}")
                    print(f"  mGBA:  {mgba_pc}, r1=0x{mgba_r1:08X}, r12=0x{mgba_r12:08X}")
                    print(f"  Ours:  {our_pc}, r1=0x{our_r1:08X}, r12=0x{our_r12:08X}")
                    break
            else:
                if first_mismatch is None:
                    first_mismatch = instr_num
    
    if last_match:
        print(f"\n" + "=" * 70)
        print("CHECKING NEXT FEW INSTRUCTIONS AFTER LAST MATCH:")
        print("=" * 70)
        
        for instr_num in range(last_match, last_match + 10):
            # Find in mGBA trace
            for i, line in enumerate(mgba_lines):
                if line.strip() == f'Instruction #{instr_num}':
                    mgba_pc = mgba_lines[i+2].strip()
                    mgba_reg_line = mgba_lines[i+3].strip()
                    mgba_regs = parse_mgba_registers(mgba_reg_line)
                    
                    # Find in our trace
                    for j, line2 in enumerate(our_lines):
                        if line2.strip() == f'Instruction #{instr_num}':
                            our_pc = our_lines[j+2].strip()
                            our_regs = parse_our_regs(our_lines, j+3)
                            
                            mgba_r1 = mgba_regs.get(1, -1)
                            mgba_r12 = mgba_regs.get(12, -1)
                            our_r1 = our_regs.get(1, -1)
                            our_r12 = our_regs.get(12, -1)
                            
                            match_r1 = "✓" if mgba_r1 == our_r1 else "✗"
                            match_r12 = "✓" if mgba_r12 == our_r12 else "✗"
                            
                            print(f"\nInstruction #{instr_num}:")
                            print(f"  mGBA:  {mgba_pc:20s} r1=0x{mgba_r1:08X} {match_r1}  r12=0x{mgba_r12:08X} {match_r12}")
                            print(f"  Ours:  {our_pc:20s} r1=0x{our_r1:08X}     r12=0x{our_r12:08X}")
                            
                            # Disassemble the instruction at this PC
                            if mgba_r1 != our_r1 or mgba_r12 != our_r12:
                                pc_val = int(mgba_pc.split('0x')[1], 16) if '0x' in mgba_pc else 0
                                if pc_val < 0x4000:  # BIOS
                                    from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB
                                    with open('/Users/malcolm/gba/assets/bios.bin', 'rb') as f:
                                        bios = f.read()
                                    md = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
                                    code = bios[pc_val:pc_val+4]
                                    for insn in md.disasm(code, pc_val):
                                        print(f"  Instruction: {insn.mnemonic} {insn.op_str}")
                                        break
                            break
                    break

if __name__ == '__main__':
    main()
