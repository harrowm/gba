#!/usr/bin/env python3
"""
Compare two instruction traces and find the first point of divergence.
"""

import sys
import re

def parse_instruction_block(lines, start_idx):
    """
    Parse an instruction block starting from start_idx.
    Returns (instruction_num, registers, cpsr, next_idx) or None if not found.
    """
    i = start_idx
    while i < len(lines):
        line = lines[i].strip()
        
        # Look for instruction marker
        if line.startswith('Instruction #'):
            instruction_num = int(line.split('#')[1])
            
            # Parse register lines (should be next 4 lines after separator)
            regs = {}
            i += 2  # Skip separator line
            
            # Parse 4 lines of registers
            for _ in range(4):
                if i >= len(lines):
                    return None
                reg_line = lines[i].strip()
                
                # Extract register values using regex
                matches = re.findall(r'(r\d+|sp|lr|pc)=0x([0-9A-Fa-f]{8})', reg_line)
                for name, value in matches:
                    regs[name] = int(value, 16)
                i += 1
            
            # Parse CPSR line
            if i >= len(lines):
                return None
            cpsr_line = lines[i].strip()
            match = re.search(r'cpsr=0x([0-9A-Fa-f]{8})', cpsr_line)
            if match:
                cpsr = int(match.group(1), 16)
            else:
                cpsr = None
            
            return (instruction_num, regs, cpsr, i + 1)
        
        i += 1
    
    return None

def compare_traces(trace1_path, trace2_path):
    """Compare two trace files and find the first divergence."""
    
    with open(trace1_path, 'r') as f:
        trace1_lines = f.readlines()
    
    with open(trace2_path, 'r') as f:
        trace2_lines = f.readlines()
    
    idx1, idx2 = 0, 0
    instruction_count = 0
    
    while True:
        # Parse next instruction from each trace
        result1 = parse_instruction_block(trace1_lines, idx1)
        result2 = parse_instruction_block(trace2_lines, idx2)
        
        if not result1 or not result2:
            if not result1 and not result2:
                print(f"\nBoth traces ended at instruction {instruction_count}")
                print("✓ Traces match completely!")
            elif not result1:
                print(f"\nTrace 1 ended at instruction {instruction_count}")
                print(f"Trace 2 continues to instruction {result2[0] if result2 else 'unknown'}")
            else:
                print(f"\nTrace 2 ended at instruction {instruction_count}")
                print(f"Trace 1 continues to instruction {result1[0] if result1 else 'unknown'}")
            break
        
        num1, regs1, cpsr1, idx1 = result1
        num2, regs2, cpsr2, idx2 = result2
        
        instruction_count = num1
        
        # Compare PC first (most important)
        if regs1.get('pc') != regs2.get('pc'):
            print(f"\n❌ DIVERGENCE at instruction {num1}")
            print(f"   mGBA PC:    0x{regs1.get('pc', 0):08X}")
            print(f"   Our PC:     0x{regs2.get('pc', 0):08X}")
            print(f"\n   Previous instruction {num1-1} state:")
            print(f"   (This shows registers BEFORE diverging instruction executed)\n")
            break
        
        # Compare all registers
        all_reg_names = set(regs1.keys()) | set(regs2.keys())
        diverged_regs = []
        
        for reg_name in sorted(all_reg_names):
            val1 = regs1.get(reg_name, 0)
            val2 = regs2.get(reg_name, 0)
            if val1 != val2:
                diverged_regs.append((reg_name, val1, val2))
        
        # Compare CPSR
        cpsr_diverged = (cpsr1 != cpsr2)
        
        if diverged_regs or cpsr_diverged:
            print(f"\n❌ DIVERGENCE at instruction {num1}")
            print(f"   PC: 0x{regs1.get('pc', 0):08X}")
            
            if diverged_regs:
                print(f"\n   Diverged registers:")
                for reg_name, val1, val2 in diverged_regs:
                    print(f"      {reg_name:>3s}: mGBA=0x{val1:08X}, Our=0x{val2:08X}, diff={val2-val1:+d}")
            
            if cpsr_diverged:
                print(f"\n   CPSR: mGBA=0x{cpsr1:08X}, Our=0x{cpsr2:08X}")
                # Decode flags
                for shift, name in [(31, 'N'), (30, 'Z'), (29, 'C'), (28, 'V'), 
                                     (7, 'I'), (6, 'F'), (5, 'T')]:
                    flag1 = (cpsr1 >> shift) & 1
                    flag2 = (cpsr2 >> shift) & 1
                    if flag1 != flag2:
                        print(f"      {name} flag: mGBA={flag1}, Our={flag2}")
                mode1 = cpsr1 & 0x1F
                mode2 = cpsr2 & 0x1F
                if mode1 != mode2:
                    print(f"      Mode: mGBA=0x{mode1:02X}, Our=0x{mode2:02X}")
            
            break
        
        # Progress indicator
        if instruction_count % 100 == 0:
            print(f"✓ Instruction {instruction_count}: PC=0x{regs1.get('pc', 0):08X} - MATCH")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: compare_traces.py <mgba_trace> <our_trace>")
        sys.exit(1)
    
    print("Comparing instruction traces...")
    print(f"mGBA trace: {sys.argv[1]}")
    print(f"Our trace:  {sys.argv[2]}")
    print()
    
    compare_traces(sys.argv[1], sys.argv[2])
