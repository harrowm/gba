#!/usr/bin/env python3
"""
Compare memory traces from mGBA and our emulator to find divergence points.
"""

import sys
import re

def parse_instruction(lines, start_idx):
    """Parse an instruction block starting from start_idx."""
    i = start_idx
    while i < len(lines):
        line = lines[i].strip()
        
        if line.startswith('Instruction #'):
            instr_num = int(line.split('#')[1])
            i += 2  # Skip separator
            
            # Find PC line
            pc = None
            while i < len(lines) and not lines[i].strip().startswith('PC:'):
                i += 1
            if i < len(lines):
                pc_match = re.search(r'PC: 0x([0-9A-Fa-f]{8})', lines[i])
                if pc_match:
                    pc = int(pc_match.group(1), 16)
                i += 1
            
            # Skip register lines until we hit Memory:
            while i < len(lines) and not lines[i].strip().startswith('Memory:'):
                i += 1
            
            if i >= len(lines):
                return None
            
            i += 1  # Skip "Memory:" header
            
            # Parse memory values
            memory = {}
            while i < len(lines):
                line = lines[i].strip()
                if not line or line.startswith('='):
                    break
                
                # Parse:  [NAME] 0xADDRESS = 0xVALUE (supports 02X, 04X, or 08X format)
                match = re.search(r'\[([^\]]+)\]\s+0x([0-9A-Fa-f]{8})\s+=\s+0x([0-9A-Fa-f]+)', line)
                if match:
                    name = match.group(1).strip()
                    addr = int(match.group(2), 16)
                    value = int(match.group(3), 16)
                    memory[addr] = (name, value)
                i += 1
            
            return (instr_num, pc, memory, i)
        
        i += 1
    
    return None

def main():
    if len(sys.argv) < 3:
        print("Usage: compare_memory_traces.py <mgba_trace> <our_trace>")
        sys.exit(1)
    
    mgba_file = sys.argv[1]
    our_file = sys.argv[2]
    
    with open(mgba_file, 'r') as f:
        mgba_lines = f.readlines()
    
    with open(our_file, 'r') as f:
        our_lines = f.readlines()
    
    print("Comparing memory traces...")
    print(f"mGBA:  {mgba_file}")
    print(f"Ours:  {our_file}")
    print()
    
    idx_mgba, idx_our = 0, 0
    
    while True:
        mgba_data = parse_instruction(mgba_lines, idx_mgba)
        our_data = parse_instruction(our_lines, idx_our)
        
        if not mgba_data or not our_data:
            print(f"\nEnd of trace reached")
            break
        
        instr_mgba, pc_mgba, mem_mgba, idx_mgba = mgba_data
        instr_our, pc_our, mem_our, idx_our = our_data
        
        # Compare PC
        if pc_mgba != pc_our:
            print(f"\n❌ PC DIVERGENCE at instruction {instr_mgba}/{instr_our}")
            print(f"   mGBA PC:  0x{pc_mgba:08X}")
            print(f"   Our PC:   0x{pc_our:08X}")
            break
        
        # Compare memory
        all_addrs = set(mem_mgba.keys()) | set(mem_our.keys())
        diverged = []
        
        # Ignore VCOUNT/DISPSTAT differences (GPU timing variations)
        IGNORE_ADDRS = {0x04000004, 0x04000006}
        
        for addr in sorted(all_addrs):
            if addr in IGNORE_ADDRS:
                continue
            name_mgba, val_mgba = mem_mgba.get(addr, ("???", 0))
            name_our, val_our = mem_our.get(addr, ("???", 0))
            
            if val_mgba != val_our:
                diverged.append((addr, name_mgba, val_mgba, val_our))
        
        if diverged:
            print(f"\n❌ MEMORY DIVERGENCE at instruction {instr_mgba}, PC=0x{pc_mgba:08X}")
            print(f"\n   Diverged memory locations:")
            for addr, name, val_mgba, val_our in diverged:
                diff = int(val_our) - int(val_mgba)  # Cast to handle potential sign issues
                print(f"      [{name:12s}] 0x{addr:08X}:")
                print(f"         mGBA:  0x{val_mgba:08X}")
                print(f"         Ours:  0x{val_our:08X}")
                print(f"         Diff:  {diff:+d} (0x{diff & 0xFFFFFFFF:08X})")
            break
        
        # Progress
        if instr_mgba % 100 == 0:
            print(f"✓ Instruction {instr_mgba:4d}: PC=0x{pc_mgba:08X} - Memory MATCH")

if __name__ == '__main__':
    main()
