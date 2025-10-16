#!/usr/bin/env python3
"""
Detailed analysis of cycle divergence between our emulator and mGBA.
Shows cumulative cycle differences over the first N instructions.
"""

import re
import sys

def parse_our_trace(filename, max_instructions=500):
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

def parse_mgba_trace(filename, max_instructions=500):
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
        if cycle_delta < 0 or cycle_delta > 100:
            cycle_delta = None
        
        costs.append({
            'inst_num': curr['inst_num'],
            'pc': curr['pc'],
            'cost': cycle_delta,
            'abs_cycle': curr['cycle']
        })
    
    return costs

def main(our_trace_file, mgba_trace_file, max_instructions=500):
    our_trace = our_trace_file
    mgba_trace = mgba_trace_file
    max_insts = max_instructions
    
    print("Parsing traces...")
    our_insts = parse_our_trace(our_trace, max_insts)
    mgba_insts = parse_mgba_trace(mgba_trace, max_insts)
    
    print(f"Our trace: {len(our_insts)} instructions")
    print(f"mGBA trace: {len(mgba_insts)} instructions")
    
    # Compute per-instruction costs
    print("\nComputing instruction costs...")
    our_costs = compute_instruction_costs(our_insts)
    mgba_costs = compute_instruction_costs(mgba_insts)
    
    # Find first common PC to skip initial divergence (mGBA starts in different mode)
    # Start at 0x11C which is first THUMB instruction after mode switch
    print("\nFinding first common PC to align traces...")
    start_idx_our = 0
    start_idx_mgba = 0
    for i, our in enumerate(our_costs):
        for j, mgba in enumerate(mgba_costs):
            if our['pc'] == mgba['pc'] and our['pc'] >= 0x11C:  # Start from first THUMB instruction
                start_idx_our = i
                start_idx_mgba = j
                print(f"Found common PC=0x{our['pc']:08x} at our_inst={i+1}, mgba_inst={j+1}")
                break
        if start_idx_our > 0:
            break
    
    if start_idx_our == 0:
        print("WARNING: No common PC found, comparing from start")
    
    # Adjust traces to start from common point
    our_costs = our_costs[start_idx_our:]
    mgba_costs = mgba_costs[start_idx_mgba:]
    
    # Detailed comparison with cumulative difference tracking
    print("\n" + "="*100)
    print(f"DETAILED CYCLE COMPARISON (Starting from common PC, up to {len(our_costs)} instructions)")
    print("="*100)
    print(f"{'Inst':<6} {'PC':<12} {'Our Cyc':<10} {'mGBA Cyc':<10} {'Our Cost':<10} {'mGBA Cost':<10} {'Cumul Diff':<12} {'Status':<10}")
    print("-"*100)
    
    cumulative_diff = 0
    mismatches = []
    matches = 0
    
    min_len = min(len(our_costs), len(mgba_costs))
    
    for i in range(min_len):
        our = our_costs[i]
        mgba = mgba_costs[i]
        
        # Check if PCs match
        if our['pc'] != mgba['pc']:
            print(f"\n*** PC MISMATCH at instruction {i+1}:")
            print(f"  Our:  PC=0x{our['pc']:08x}")
            print(f"  mGBA: PC=0x{mgba['pc']:08x}")
            break
        
        # Track cumulative cycle difference
        our_abs = our['abs_cycle']
        mgba_abs = mgba['abs_cycle']
        cumulative_diff = our_abs - mgba_abs
        
        # Compare costs
        if our['cost'] is None or mgba['cost'] is None:
            status = "WRAP"
            cost_str_our = "WRAP" if our['cost'] is None else str(our['cost'])
            cost_str_mgba = "WRAP" if mgba['cost'] is None else str(mgba['cost'])
        elif our['cost'] != mgba['cost']:
            status = f"DIFF({our['cost'] - mgba['cost']:+d})"
            cost_str_our = str(our['cost'])
            cost_str_mgba = str(mgba['cost'])
            mismatches.append({
                'inst': i+1,
                'pc': our['pc'],
                'our_cost': our['cost'],
                'mgba_cost': mgba['cost'],
                'diff': our['cost'] - mgba['cost'],
                'cumulative_diff': cumulative_diff
            })
        else:
            status = "OK"
            cost_str_our = str(our['cost'])
            cost_str_mgba = str(mgba['cost'])
            matches += 1
            continue  # Don't print matching instructions
        
        # Print this instruction (mismatches and wraps only)
        print(f"{i+1:<6} 0x{our['pc']:08x}  {our_abs:<10} {mgba_abs:<10} {cost_str_our:<10} {cost_str_mgba:<10} {cumulative_diff:+12d} {status:<10}")
    
    # Summary statistics
    print("\n" + "="*100)
    print("SUMMARY STATISTICS")
    print("="*100)
    print(f"Instructions compared: {min_len}")
    print(f"Matching costs: {matches}")
    print(f"Mismatched costs: {len(mismatches)}")
    print(f"Final cumulative difference: {cumulative_diff:+d} cycles (Our - mGBA)")
    
    if cumulative_diff > 0:
        print(f"  → We are AHEAD by {cumulative_diff} cycles (overcounting)")
    elif cumulative_diff < 0:
        print(f"  → We are BEHIND by {-cumulative_diff} cycles (undercounting)")
    else:
        print(f"  → PERFECT MATCH!")
    
    # Analyze mismatch patterns
    if mismatches:
        print(f"\n{'='*100}")
        print("MISMATCH ANALYSIS")
        print("="*100)
        
        # Group by cost difference
        from collections import defaultdict
        by_diff = defaultdict(list)
        for m in mismatches:
            by_diff[m['diff']].append(m)
        
        print(f"\nMismatches by cost difference:")
        for diff in sorted(by_diff.keys()):
            count = len(by_diff[diff])
            print(f"  {diff:+3d} cycles: {count:3d} instructions")
            
            # Show first 3 examples with cumulative impact
            for m in by_diff[diff][:3]:
                print(f"      Inst {m['inst']:3d}: PC=0x{m['pc']:08x} (Our={m['our_cost']}, mGBA={m['mgba_cost']}) → Cumul={m['cumulative_diff']:+d}")
        
        # Show where cumulative diff grows/shrinks the most
        print(f"\n{'='*100}")
        print("CUMULATIVE DIFFERENCE PROGRESSION")
        print("="*100)
        
        # Sample every 50 instructions
        print(f"\n{'Inst':<8} {'PC':<12} {'Cumulative Diff':<20} {'Trend'}")
        print("-"*60)
        for i in range(0, len(mismatches), max(1, len(mismatches)//20)):
            m = mismatches[i]
            trend = ""
            if i > 0:
                prev_diff = mismatches[i-1]['cumulative_diff']
                if m['cumulative_diff'] > prev_diff:
                    trend = "↑ Growing gap"
                elif m['cumulative_diff'] < prev_diff:
                    trend = "↓ Narrowing"
                else:
                    trend = "→ Stable"
            print(f"{m['inst']:<8} 0x{m['pc']:08x}  {m['cumulative_diff']:+20d} {trend}")

if __name__ == '__main__':
    import sys
    
    # Use command-line args or defaults
    our_trace = sys.argv[1] if len(sys.argv) > 1 else '/tmp/gba_memory_trace.log'
    mgba_trace = sys.argv[2] if len(sys.argv) > 2 else '/Users/malcolm/mgba_trace.log'
    max_inst = int(sys.argv[3]) if len(sys.argv) > 3 else 500
    
    main(our_trace, mgba_trace, max_inst)
