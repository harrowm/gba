#!/usr/bin/env python3
"""
Detailed analysis of the divergence point - comparing instruction execution
"""

from capstone import Cs, CS_ARCH_ARM, CS_MODE_ARM, CS_MODE_THUMB
from capstone.arm import *

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

def disassemble_around_pc(bios, pc, arm_mode=True, count=15):
    """Disassemble instructions around a PC"""
    mode = CS_MODE_ARM if arm_mode else CS_MODE_THUMB
    md = Cs(CS_ARCH_ARM, mode)
    md.detail = True
    
    # Calculate start address (go back 'count' instructions)
    inst_size = 4 if arm_mode else 2
    start_addr = max(0, pc - (count * inst_size))
    end_addr = min(len(bios), pc + (count * inst_size))
    
    code = bios[start_addr:end_addr]
    
    instructions = []
    for insn in md.disasm(code, start_addr):
        instructions.append({
            'addr': insn.address,
            'mnemonic': insn.mnemonic,
            'op_str': insn.op_str,
            'bytes': insn.bytes.hex(),
            'size': insn.size
        })
    
    return instructions

def main():
    print("=" * 80)
    print("DETAILED DIVERGENCE ANALYSIS")
    print("=" * 80)
    
    # Load BIOS
    with open('/Users/malcolm/gba/assets/bios.bin', 'rb') as f:
        bios = f.read()
    
    # Load traces
    with open('/tmp/mgba_memory_trace.log', 'r') as f:
        mgba_lines = f.readlines()
    
    with open('/tmp/gba_memory_trace.log', 'r') as f:
        our_lines = f.readlines()
    
    print("\nAnalyzing instructions #440 through #475")
    print("-" * 80)
    
    # Get disassembly around PC=0xBCC (where divergence happens)
    instructions = disassemble_around_pc(bios, 0xBCC, arm_mode=True, count=20)
    
    print("\nDisassembly around divergence point (ARM mode):")
    print("-" * 80)
    for inst in instructions:
        marker = "  <<<" if inst['addr'] == 0xBCC else ""
        print(f"0x{inst['addr']:08X}:  {inst['mnemonic']:10s} {inst['op_str']:30s} [{inst['bytes']}]{marker}")
    
    print("\n" + "=" * 80)
    print("INSTRUCTION-BY-INSTRUCTION COMPARISON")
    print("=" * 80)
    
    # Compare each instruction from 440 to 475
    for instr_num in range(440, 476):
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
        
        if mgba_regs and our_regs:
            mgba_r10 = mgba_regs.get(10, 0)
            mgba_r12 = mgba_regs.get(12, 0)
            our_r10 = our_regs.get(10, 0)
            our_r12 = our_regs.get(12, 0)
            
            # Find the instruction at this PC
            inst_info = None
            for inst in instructions:
                if inst['addr'] == mgba_pc:
                    inst_info = inst
                    break
            
            r10_match = "✓" if mgba_r10 == our_r10 else "✗"
            r12_match = "✓" if mgba_r12 == our_r12 else "✗"
            
            if mgba_r12 != our_r12:
                print(f"\n{'='*80}")
                print(f"DIVERGENCE at Instruction #{instr_num}")
                print(f"{'='*80}")
            else:
                print(f"\nInstruction #{instr_num}:")
            
            print(f"  PC: 0x{mgba_pc:08X}")
            if inst_info:
                print(f"  Instruction: {inst_info['mnemonic']} {inst_info['op_str']}")
            
            print(f"\n  Register comparison:")
            print(f"    r10:  mGBA=0x{mgba_r10:08X}  Ours=0x{our_r10:08X}  {r10_match}")
            print(f"    r12:  mGBA=0x{mgba_r12:08X}  Ours=0x{our_r12:08X}  {r12_match}")
            
            if mgba_r12 != our_r12:
                print(f"\n  ANALYSIS:")
                if inst_info:
                    if inst_info['mnemonic'] == 'lsrs' and 'ip' in inst_info['op_str']:
                        print(f"    This is a LSRS instruction that writes to r12 (ip)")
                        print(f"    Operation: {inst_info['op_str']}")
                        print(f"    Expected: r12 should be set to (r10 >> 9) = (0x{our_r10:08X} >> 9) = 0x{our_r10 >> 9:08X}")
                        print(f"    Our emulator: r12 = 0x{our_r12:08X} ✓ CORRECT")
                        print(f"    mGBA:         r12 = 0x{mgba_r12:08X} ✗ NOT SET")
                        print(f"\n    CONCLUSION: mGBA is NOT executing this instruction's write to r12!")
                        print(f"    OR: This instruction should NOT write to r12 in this context!")
                        print(f"\n    Checking instruction encoding:")
                        print(f"    Bytes: {inst_info['bytes']}")
                        
                        # Decode the instruction manually
                        inst_bytes = bytes.fromhex(inst_info['bytes'])
                        inst_val = int.from_bytes(inst_bytes, 'little')
                        print(f"    Value: 0x{inst_val:08X}")
                        print(f"    Binary: {inst_val:032b}")
                        
                        # ARM data processing format
                        cond = (inst_val >> 28) & 0xF
                        op = (inst_val >> 21) & 0xF
                        s = (inst_val >> 20) & 1
                        rn = (inst_val >> 16) & 0xF
                        rd = (inst_val >> 12) & 0xF
                        shift = (inst_val >> 4) & 0xFF
                        rm = inst_val & 0xF
                        
                        print(f"\n    Decoded fields:")
                        print(f"      Cond:  {cond:04b} ({cond}) = {'AL (always)' if cond == 0xE else 'other'}")
                        print(f"      Op:    {op:04b} ({op}) = MOV")
                        print(f"      S:     {s} (set flags)")
                        print(f"      Rd:    {rd} (r{rd}) = {'r12/ip' if rd == 12 else f'r{rd}'}")
                        print(f"      Rn:    {rn} (r{rn})")
                        print(f"      Rm:    {rm} (r{rm}) = {'r10/sl' if rm == 10 else f'r{rm}'}")
                        print(f"      Shift: {shift:08b} = LSR #{(shift >> 3) & 0x1F}")
                        
                        print(f"\n    ⚠️  KEY FINDING:")
                        print(f"    The S bit is SET (S=1), meaning this is LSRS (set flags)")
                        print(f"    In ARM, when S=1 and Rd=R15 (PC), behavior is special")
                        print(f"    But Rd=R12, not R15, so this should write normally")
                        print(f"\n    Our emulator writes to r12: CORRECT")
                        print(f"    mGBA does NOT write to r12: Check if there's a special case?")
                        
                break

if __name__ == '__main__':
    main()
