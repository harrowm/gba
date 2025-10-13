#!/usr/bin/env python3
"""
Verify the analyzer by comparing its results to manual calculation
for the first 50 instructions.
"""

import re

def main():
    trace_file = "/tmp/gba_memory_trace.log"
    
    print("=" * 80)
    print("ANALYZER VERIFICATION - First 50 Instructions")
    print("=" * 80)
    print()
    
    # Parse the trace
    instruction_cycles = {}
    prev_cycle = 0
    prev_num = 0
    
    with open(trace_file, 'r') as f:
        for line in f:
            m = re.match(r'Instruction #(\d+) \(Cycle: (\d+)\)', line)
            if m:
                num = int(m.group(1))
                cycle = int(m.group(2))
                
                if prev_cycle > 0:
                    cost = cycle - prev_cycle
                    instruction_cycles[prev_num] = cost
                
                prev_cycle = cycle
                prev_num = num
                
                if num > 51:  # Get a bit extra
                    break
    
    # Show first 50 instructions
    print("Instr#  | Cycle Start | Cycle End | Cost | Calculation")
    print("-" * 80)
    
    # Get cycle values for display
    cycle_values = {}
    prev_cycle = 0
    
    with open(trace_file, 'r') as f:
        for line in f:
            m = re.match(r'Instruction #(\d+) \(Cycle: (\d+)\)', line)
            if m:
                num = int(m.group(1))
                cycle = int(m.group(2))
                cycle_values[num] = cycle
                if num > 51:
                    break
    
    total_cycles = 0
    for i in range(1, 51):
        if i in instruction_cycles:
            cost = instruction_cycles[i]
            start_cycle = cycle_values.get(i, 0)
            end_cycle = cycle_values.get(i+1, 0)
            total_cycles += cost
            
            print(f"{i:6d}  | {start_cycle:11d} | {end_cycle:9d} | {cost:4d} | {end_cycle} - {start_cycle} = {cost}")
    
    print("-" * 80)
    print(f"Total cycles for instructions 1-50: {total_cycles}")
    print(f"Average cycles/instruction: {total_cycles / 50:.4f}")
    print()
    
    # Count distribution
    from collections import Counter
    costs = [instruction_cycles[i] for i in range(1, 51) if i in instruction_cycles]
    counter = Counter(costs)
    
    print("CYCLE COST DISTRIBUTION:")
    print("-" * 40)
    for cost in sorted(counter.keys()):
        count = counter[cost]
        pct = 100.0 * count / len(costs)
        print(f"{cost:2d} cycles: {count:3d} instructions ({pct:5.1f}%)")
    
    print()
    print("=" * 80)
    print("EXPECTED FOR BIOS EXECUTION:")
    print("=" * 80)
    print("Simple instructions (MOV, ADD, etc.): 2 cycles (1+seq prefetch)")
    print("Branches (B, BL):                     4 cycles (2 prefetch + 2 refill)")
    print("LDR/STR from BIOS:                    2 cycles (2 prefetch + 0 data)")
    print()

if __name__ == "__main__":
    main()
