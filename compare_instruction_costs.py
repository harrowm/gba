#!/usr/bin/env python3
"""
Compare per-instruction cycle costs between our emulator and mGBA.
This handles cases where absolute cycle counters may reset or wrap.
"""

import re
import sys

def parse_our_trace(filename, max_instructions=1000):
    """Parse our trace format: 'Instruction #N (Cycle: X)'"""
    instructions = []
    current_inst = None
    
    with open(filename, 'r') as f:
        for line in f:
            # Match: Instruction #123 (Cycle: 456) or Instruction #1 (no cycle)
            match = re.match(r'Instruction #(\d+)(?: \(Cycle: (\d+)\))?', line)
            if match:
                inst_num = int(match.group(1))
                cycle = int(match.group(2)) if match.group(2) else 0
                current_inst = {'inst_num': inst_num, 'cycle': cycle, 'pc': None}
            
            # Look for PC on any line (format: "PC: 0x12345678")
            if current_inst and current_inst['pc'] is None:
                pc_match = re.search(r'PC[:\s=]+0x([0-9a-fA-F]+)', line)
                if pc_match:
                    current_inst['pc'] = int(pc_match.group(1), 16)
                    instructions.append(current_inst)
                    current_inst = None
                    
                    if len(instructions) >= max_instructions:
                        break
    
    return instructions

def parse_mgba_trace(filename, max_instructions=1000):
    """Parse mGBA trace format: 'ARM: PC=XXXXXXXX Cycles=N' or 'THUMB: PC=XXXXXXXX Cycles=N'"""
    instructions = []
    inst_num = 0
    
    with open(filename, 'r') as f:
        for line in f:
            # Match ARM or THUMB instruction
            match = re.match(r'(ARM|THUMB): PC=([0-9a-fA-F]+) Cycles=(\d+)', line)
            if match:
                mode = match.group(1)
                pc = int(match.group(2), 16)
                cycles = int(match.group(3))
                
                instructions.append({
                    'inst_num': inst_num,
                    'mode': mode,
                    'pc': pc,
                    'cycle': cycles
                })
                inst_num += 1
                
                if len(instructions) >= max_instructions:
                    break
    
    return instructions

def compute_instruction_costs(instructions):
    """Compute the cycle cost of each instruction (delta between consecutive instructions)"""
    costs = []
    
    for i in range(1, len(instructions)):
        prev = instructions[i-1]
        curr = instructions[i]
        
        # Compute cycle delta (handling wraps/resets)
        cycle_delta = curr['cycle'] - prev['cycle']
        
        # If the delta is negative or too large, it probably wrapped/reset
        # In that case, mark it as unknown
        if cycle_delta < 0 or cycle_delta > 100:
            cycle_delta = None
        
        costs.append({
            'inst_num': curr['inst_num'],
            'pc': curr['pc'],
            'cost': cycle_delta
        })
    
    return costs

def main():
    our_trace = '/tmp/gba_memory_trace.log'
    mgba_trace = '/tmp/mgba_instrumented_trace.log'
    max_insts = 1000
    
    print("Parsing traces...")
    our_insts = parse_our_trace(our_trace, max_insts)
    mgba_insts = parse_mgba_trace(mgba_trace, max_insts)
    
    print(f"Our trace: {len(our_insts)} instructions")
    print(f"mGBA trace: {len(mgba_insts)} instructions")
    
    # Compute per-instruction costs
    print("\nComputing instruction costs...")
    our_costs = compute_instruction_costs(our_insts)
    mgba_costs = compute_instruction_costs(mgba_insts)
    
    # Compare costs for matching PCs
    print("\n" + "="*70)
    print("INSTRUCTION COST COMPARISON")
    print("="*70)
    print(f"{'Inst':<6} {'PC':<12} {'Our Cost':<10} {'mGBA Cost':<12} {'Diff':<10}")
    print("-"*70)
    
    mismatches = []
    matches = 0
    
    min_len = min(len(our_costs), len(mgba_costs))
    for i in range(min_len):
        our = our_costs[i]
        mgba = mgba_costs[i]
        
        # Check if PCs match
        if our['pc'] != mgba['pc']:
            print(f"\nPC MISMATCH at instruction {i+1}:")
            print(f"  Our:  PC=0x{our['pc']:08x}")
            print(f"  mGBA: PC=0x{mgba['pc']:08x}")
            break
        
        # Compare costs
        if our['cost'] is None or mgba['cost'] is None:
            status = "WRAP"
            cost_str_our = "WRAP" if our['cost'] is None else str(our['cost'])
            cost_str_mgba = "WRAP" if mgba['cost'] is None else str(mgba['cost'])
            diff_str = "N/A"
        elif our['cost'] != mgba['cost']:
            status = "DIFF"
            diff = our['cost'] - mgba['cost']
            cost_str_our = str(our['cost'])
            cost_str_mgba = str(mgba['cost'])
            diff_str = f"{diff:+d}"
            mismatches.append({
                'inst': i+1,
                'pc': our['pc'],
                'our_cost': our['cost'],
                'mgba_cost': mgba['cost'],
                'diff': diff
            })
        else:
            matches += 1
            continue  # Don't print matches
        
        print(f"{i+1:<6} 0x{our['pc']:08x}  {cost_str_our:<10} {cost_str_mgba:<12} {diff_str:<10} {status}")
    
    # Summary
    print("\n" + "="*70)
    print("SUMMARY")
    print("="*70)
    print(f"Instructions compared: {min_len}")
    print(f"Matching costs: {matches}")
    print(f"Mismatched costs: {len(mismatches)}")
    
    if mismatches:
        print(f"\nMismatches by type:")
        
        # Group mismatches by cost difference
        diffs = {}
        for m in mismatches:
            diff = m['diff']
            if diff not in diffs:
                diffs[diff] = []
            diffs[diff].append(m)
        
        for diff in sorted(diffs.keys()):
            count = len(diffs[diff])
            print(f"  {diff:+d} cycles: {count} instructions")
            
            # Show first few examples
            for m in diffs[diff][:3]:
                print(f"    Inst {m['inst']}: PC=0x{m['pc']:08x} (Our={m['our_cost']}, mGBA={m['mgba_cost']})")

if __name__ == '__main__':
    main()
