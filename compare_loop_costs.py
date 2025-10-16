#!/usr/bin/env python3
"""
Compare the cost pattern of the BIOS loop between our emulator and mGBA.
Focuses on the repeating pattern rather than exact PC matching.
"""

import re

def extract_loop_costs_ours(filename):
    """Extract costs for the 0x120-0x122-0x124 loop from our trace"""
    costs = []
    with open(filename, 'r') as f:
        lines = f.readlines()
        i = 0
        while i < len(lines):
            if 'PC: 0x00000120' in lines[i]:
                # Found start of loop iteration
                # Get cycle for this instruction
                j = i - 1
                while j >= 0 and not lines[j].startswith('Instruction #'):
                    j -= 1
                if j >= 0:
                    match = re.search(r'Cycle: (\d+)', lines[j])
                    if match:
                        cycle_120 = int(match.group(1))
                        # Now find 0x122 and 0x124
                        k = i + 1
                        cycle_122 = None
                        cycle_124 = None
                        while k < len(lines) and k < i + 20:
                            if 'PC: 0x00000122' in lines[k] and cycle_122 is None:
                                m = k - 1
                                while m >= 0 and not lines[m].startswith('Instruction #'):
                                    m -= 1
                                if m >= 0:
                                    match2 = re.search(r'Cycle: (\d+)', lines[m])
                                    if match2:
                                        cycle_122 = int(match2.group(1))
                            elif 'PC: 0x00000124' in lines[k] and cycle_122 and cycle_124 is None:
                                m = k - 1
                                while m >= 0 and not lines[m].startswith('Instruction #'):
                                    m -= 1
                                if m >= 0:
                                    match3 = re.search(r'Cycle: (\d+)', lines[m])
                                    if match3:
                                        cycle_124 = int(match3.group(1))
                                        break
                            k += 1
                        
                        if cycle_122 and cycle_124:
                            cost_str = cycle_120 - (cycle_124 - 3 if len(costs) > 0 else 0)  # Previous BLT cost
                            cost_adds = cycle_122 - cycle_120
                            cost_blt = cycle_124 - cycle_122
                            costs.append({
                                'str': cost_str if len(costs) > 0 else cycle_120,
                                'adds': cost_adds,
                                'blt': cost_blt
                            })
            i += 1
            if len(costs) >= 50:  # Get first 50 iterations
                break
    return costs

def extract_loop_costs_mgba(filename):
    """Extract costs for the 0x11E-0x120-0x122 loop from mGBA trace"""
    # mGBA doesn't log 0x124, so we see: 0x11E (LDR) → 0x120 (STR) → 0x122 (ADDS+BLT)
    costs = []
    lines = []
    with open(filename, 'r') as f:
        for line in f:
            if 'THUMB:' in line and ('PC=0000011e' in line or 'PC=00000120' in line or 'PC=00000122' in line):
                match = re.search(r'PC=([0-9a-f]+) Cycles=(\d+)', line)
                if match:
                    lines.append({'pc': int(match.group(1), 16), 'cycle': int(match.group(2))})
    
    # Process in groups of 3: LDR, STR, ADDS
    i = 1  # Skip first 0x11C
    while i < len(lines) - 2:
        if lines[i]['pc'] == 0x11E and lines[i+1]['pc'] == 0x120 and lines[i+2]['pc'] == 0x122:
            # Calculate costs
            cost_ldr = lines[i+1]['cycle'] - lines[i]['cycle']
            cost_str = lines[i+2]['cycle'] - lines[i+1]['cycle']
            # ADDS+BLT combined cost
            if i+3 < len(lines):
                cost_adds_blt = lines[i+3]['cycle'] - lines[i+2]['cycle']
            else:
                cost_adds_blt = 0
            
            costs.append({
                'ldr': cost_ldr,
                'str': cost_str,
                'adds_blt': cost_adds_blt
            })
            i += 3
        else:
            i += 1
        
        if len(costs) >= 50:
            break
    
    return costs

def main():
    print("Extracting loop costs from our emulator...")
    our_costs = extract_loop_costs_ours('/tmp/gba_memory_trace.log')
    print(f"Found {len(our_costs)} loop iterations")
    
    print("\nExtracting loop costs from mGBA...")
    mgba_costs = extract_loop_costs_mgba('/Users/malcolm/mgba_trace.log')
    print(f"Found {len(mgba_costs)} loop iterations")
    
    if len(our_costs) > 0 and len(mgba_costs) > 0:
        print("\n" + "="*80)
        print("LOOP COST COMPARISON")
        print("="*80)
        print(f"{'Iter':<6} {'Our STR':<10} {'Our ADDS':<10} {'Our BLT':<10} {'mGBA STR':<12} {'mGBA ADDS+BLT':<15} {'Match?'}")
        print("-"*80)
        
        matches = 0
        for i in range(min(20, len(our_costs), len(mgba_costs))):
            our = our_costs[i]
            mgba = mgba_costs[i]
            
            # Our costs: STR=2, ADDS=1, BLT=3 (total per iteration = 6)
            # mGBA costs: STR=1, ADDS+BLT=3 (total per iteration = 4, but LDR=2 from previous)
            # Actually mGBA shows: LDR=2, STR=1, ADDS+BLT=3 = total 6 ✓
            
            our_total = our['str'] + our['adds'] + our['blt']
            mgba_total = mgba['str'] + mgba['adds_blt']
            
            match = "✓" if our_total == mgba_total else "✗"
            if our_total == mgba_total:
                matches += 1
            
            print(f"{i+1:<6} {our['str']:<10} {our['adds']:<10} {our['blt']:<10} {mgba['str']:<12} {mgba['adds_blt']:<15} {match}")
        
        print("\n" + "="*80)
        print(f"Total iterations compared: {min(20, len(our_costs), len(mgba_costs))}")
        print(f"Matching totals: {matches}")
        
        # Show typical pattern
        if len(our_costs) >= 3:
            print("\nTypical pattern (iterations 2-4):")
            for i in range(1, min(4, len(our_costs))):
                our = our_costs[i]
                total = our['str'] + our['adds'] + our['blt']
                print(f"  Iteration {i+1}: STR={our['str']}, ADDS={our['adds']}, BLT={our['blt']}, Total={total} cycles")
        
        if len(mgba_costs) >= 3:
            print("\nmGBA pattern (iterations 2-4):")
            for i in range(1, min(4, len(mgba_costs))):
                mgba = mgba_costs[i]
                total = mgba['ldr'] + mgba['str'] + mgba['adds_blt']
                print(f"  Iteration {i+1}: LDR={mgba['ldr']}, STR={mgba['str']}, ADDS+BLT={mgba['adds_blt']}, Total={total} cycles")

if __name__ == '__main__':
    main()
