#!/usr/bin/env python3
"""
Analyze GBA emulator timing trace to understand instruction distribution
and identify timing discrepancies.
"""

import re
from collections import defaultdict, Counter

def analyze_trace(trace_file):
    """Analyze the memory trace to understand instruction patterns."""
    
    # Counters for analysis
    instruction_types = Counter()
    cycle_costs = []
    total_instructions = 0
    total_cycles = 0
    
    # Track cycles per instruction number
    instruction_cycles = {}
    
    # Track instruction details (PC, opcode) for debugging
    instruction_details = {}
    
    # Parse trace file
    current_instr_num = None
    current_cycle = None
    prev_cycle = 0
    current_pc = None
    current_opcode = None
    
    print("=" * 80)
    print("GBA TIMING ANALYSIS")
    print("=" * 80)
    
    with open(trace_file, 'r') as f:
        for line in f:
            # Match instruction lines: "Instruction #424 (Cycle: 965)"
            instr_match = re.match(r'Instruction #(\d+) \(Cycle: (\d+)\)', line)
            if instr_match:
                instr_num = int(instr_match.group(1))
                cycle = int(instr_match.group(2))
                
                if prev_cycle > 0:
                    cost = cycle - prev_cycle
                    cycle_costs.append(cost)
                    # Store the cost under the PREVIOUS instruction number
                    # (the cost is how long it took to execute that instruction)
                    instruction_cycles[current_instr_num] = cost
                    # Store details for the previous instruction
                    if current_pc is not None:
                        instruction_details[current_instr_num] = {
                            'pc': current_pc,
                            'opcode': current_opcode,
                            'cost': cost
                        }
                
                # Now update to the current instruction
                prev_cycle = cycle
                current_instr_num = instr_num
                current_cycle = cycle
                total_instructions = instr_num
                total_cycles = cycle
                current_pc = None
                current_opcode = None
            
            # Match PC lines: "PC: 0x00000124" or "  PC=0x00000000"
            pc_match = re.search(r'PC[:\s=]+(0x[0-9A-Fa-f]+)', line)
            if pc_match and current_instr_num is not None:
                current_pc = pc_match.group(1)
            
            # Match opcode if it's on the same line as PC or separate
            opcode_match = re.search(r'opcode[:\s=]+(0x[0-9A-Fa-f]+)', line)
            if opcode_match and current_instr_num is not None:
                current_opcode = opcode_match.group(1)
    
    # Analyze cycle cost distribution
    print(f"\n1. OVERALL STATISTICS")
    print(f"   Total instructions: {total_instructions}")
    print(f"   Total cycles: {total_cycles}")
    print(f"   Average cycles/instruction: {total_cycles / total_instructions:.4f}")
    
    print(f"\n2. CYCLE COST DISTRIBUTION (first {min(500, len(cycle_costs))} instructions)")
    cost_counter = Counter(cycle_costs[:500])
    for cost in sorted(cost_counter.keys()):
        count = cost_counter[cost]
        pct = 100.0 * count / min(500, len(cycle_costs))
        print(f"   {cost:2d} cycles: {count:4d} instructions ({pct:5.1f}%)")
    
    # Find 1-cycle instructions
    print(f"\n3. INSTRUCTIONS TAKING 1 CYCLE (first 500)")
    one_cycle_instrs = [(num, instruction_details.get(num, {})) 
                        for num, cost in instruction_cycles.items() 
                        if cost == 1 and num <= 500]
    one_cycle_instrs.sort(key=lambda x: x[0])
    if one_cycle_instrs:
        print(f"   Found {len(one_cycle_instrs)} instructions taking 1 cycle:")
        for num, details in one_cycle_instrs:
            pc = details.get('pc', 'unknown')
            opcode = details.get('opcode', 'unknown')
            print(f"   Instruction #{num:4d}: PC={pc}, opcode={opcode}")
    else:
        print(f"   No instructions taking exactly 1 cycle found")
    
    # Find 2-cycle instructions
    print(f"\n3b. INSTRUCTIONS TAKING 2 CYCLES (first 100)")
    two_cycle_instrs = [(num, instruction_details.get(num, {})) 
                        for num, cost in instruction_cycles.items() 
                        if cost == 2 and num <= 100]
    two_cycle_instrs.sort(key=lambda x: x[0])
    if two_cycle_instrs:
        print(f"   Found {len(two_cycle_instrs)} instructions taking 2 cycles (showing first 20):")
        for num, details in two_cycle_instrs[:20]:
            pc = details.get('pc', 'unknown')
            opcode = details.get('opcode', 'unknown')
            print(f"   Instruction #{num:4d}: PC={pc}, opcode={opcode}")
    else:
        print(f"   No instructions taking exactly 2 cycles found")
    
    # Find 4-cycle instructions
    print(f"\n3c. INSTRUCTIONS TAKING 4 CYCLES (first 100)")
    four_cycle_instrs = [(num, instruction_details.get(num, {})) 
                        for num, cost in instruction_cycles.items() 
                        if cost == 4 and num <= 100]
    four_cycle_instrs.sort(key=lambda x: x[0])
    if four_cycle_instrs:
        print(f"   Found {len(four_cycle_instrs)} instructions taking 4 cycles (showing first 20):")
        for num, details in four_cycle_instrs[:20]:
            pc = details.get('pc', 'unknown')
            opcode = details.get('opcode', 'unknown')
            print(f"   Instruction #{num:4d}: PC={pc}, opcode={opcode}")
    else:
        print(f"   No instructions taking exactly 4 cycles found")
    
    # Find 5-cycle instructions
    print(f"\n3d. INSTRUCTIONS TAKING 5 CYCLES (first 100)")
    five_cycle_instrs = [(num, instruction_details.get(num, {})) 
                        for num, cost in instruction_cycles.items() 
                        if cost == 5 and num <= 100]
    five_cycle_instrs.sort(key=lambda x: x[0])
    if five_cycle_instrs:
        print(f"   Found {len(five_cycle_instrs)} instructions taking 5 cycles (showing first 20):")
        for num, details in five_cycle_instrs[:20]:
            pc = details.get('pc', 'unknown')
            opcode = details.get('opcode', 'unknown')
            print(f"   Instruction #{num:4d}: PC={pc}, opcode={opcode}")
    else:
        print(f"   No instructions taking exactly 5 cycles found")
    
    # Find 3-cycle instructions
    print(f"\n3e. INSTRUCTIONS TAKING 3 CYCLES (sample from first 100)")
    three_cycle_instrs = [(num, instruction_details.get(num, {})) 
                        for num, cost in instruction_cycles.items() 
                        if cost == 3 and num <= 100]
    three_cycle_instrs.sort(key=lambda x: x[0])
    if three_cycle_instrs:
        print(f"   Found {len(three_cycle_instrs)} instructions taking 3 cycles (showing first 20):")
        for num, details in three_cycle_instrs[:20]:
            pc = details.get('pc', 'unknown')
            opcode = details.get('opcode', 'unknown')
            print(f"   Instruction #{num:4d}: PC={pc}, opcode={opcode}")
    else:
        print(f"   No instructions taking exactly 3 cycles found")
    
    # Find expensive instructions with details
    print(f"\n4. MOST EXPENSIVE INSTRUCTIONS (>5 cycles, first 500)")
    expensive = [(num, cost, instruction_details.get(num, {})) 
                 for num, cost in instruction_cycles.items() 
                 if cost > 5 and num <= 500]
    expensive.sort(key=lambda x: x[1], reverse=True)
    for num, cost, details in expensive[:20]:
        pc = details.get('pc', 'unknown')
        opcode = details.get('opcode', 'unknown')
        print(f"   Instruction #{num:4d}: {cost:3d} cycles, PC={pc}, opcode={opcode}")
    
    # Analyze first N instructions in detail
    N = 450  # Slightly past where DISPSTAT changes
    print(f"\n5. DETAILED ANALYSIS (first {N} instructions)")
    early_costs = [cost for num, cost in instruction_cycles.items() if num <= N]
    if early_costs:
        early_total = sum(early_costs)
        early_avg = early_total / len(early_costs)
        print(f"   Instructions: {len(early_costs)}")
        print(f"   Total cycles: {early_total}")
        print(f"   Average cycles/instruction: {early_avg:.4f}")
        print(f"   Target (mGBA at instr 519): 1.85 cycles/instr")
        print(f"   Gap: {early_avg - 1.85:.4f} cycles/instr too high")
    
    # Break down by cost ranges
    print(f"\n6. CYCLE COST BREAKDOWN (first {N} instructions)")
    ranges = [(1, 1), (2, 2), (3, 3), (4, 5), (6, 10), (11, float('inf'))]
    for low, high in ranges:
        matching = [c for c in early_costs if low <= c <= high]
        if matching:
            count = len(matching)
            total = sum(matching)
            pct = 100.0 * count / len(early_costs)
            if high == float('inf'):
                print(f"   {low}+ cycles: {count:4d} instructions ({pct:5.1f}%) = {total:6d} cycles")
            elif low == high:
                print(f"   {low} cycle:   {count:4d} instructions ({pct:5.1f}%) = {total:6d} cycles")
            else:
                print(f"   {low}-{high} cycles: {count:4d} instructions ({pct:5.1f}%) = {total:6d} cycles")
    
    # Identify patterns
    print(f"\n7. TIMING PATTERNS")
    # Check if there are clusters of expensive instructions
    consecutive_expensive = []
    current_streak = []
    for num in sorted([n for n in instruction_cycles.keys() if n <= N]):
        cost = instruction_cycles[num]
        if cost >= 3:
            current_streak.append((num, cost))
        else:
            if len(current_streak) >= 3:
                consecutive_expensive.append(current_streak)
            current_streak = []
    
    if consecutive_expensive:
        print(f"   Found {len(consecutive_expensive)} streaks of expensive instructions (3+ cycles)")
        for i, streak in enumerate(consecutive_expensive[:5]):
            start = streak[0][0]
            end = streak[-1][0]
            total_cost = sum(c for _, c in streak)
            print(f"   Streak {i+1}: Instructions #{start}-{end}, {len(streak)} instrs, {total_cost} cycles")

if __name__ == "__main__":
    trace_file = "/tmp/gba_memory_trace.log"
    analyze_trace(trace_file)
    
    print("\n" + "=" * 80)
    print("RECOMMENDATIONS:")
    print("=" * 80)
    print("1. Check instructions costing 3+ cycles - likely branches or memory ops")
    print("2. Compare cycle costs to mGBA's ARM_PREFETCH_CYCLES + instruction logic")
    print("3. Verify branch costs (should be 1 + 2 = 3 for refill)")
    print("4. Check LDM/STM cycle calculations")
    print("5. Verify memory access patterns (ROM vs BIOS vs IWRAM)")
    print()
