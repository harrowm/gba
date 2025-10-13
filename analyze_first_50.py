#!/usr/bin/env python3
"""
Analyze the first 50 instructions in detail to verify analyzer logic.
"""

import re
from collections import Counter

def analyze_first_50(trace_file):
    """Analyze first 50 instructions with detailed output."""
    
    print("=" * 80)
    print("FIRST 50 INSTRUCTIONS - DETAILED ANALYSIS")
    print("=" * 80)
    
    instructions = []
    prev_cycle = 0
    
    with open(trace_file, 'r') as f:
        for line in f:
            m = re.match(r'Instruction #(\d+) \(Cycle: (\d+)\)', line)
            if m:
                instr_num = int(m.group(1))
                cycle = int(m.group(2))
                
                if instr_num <= 50:
                    if prev_cycle > 0:
                        cost = cycle - prev_cycle
                        instructions.append({
                            'num': instr_num - 1,  # Cost is for PREVIOUS instruction
                            'start_cycle': prev_cycle,
                            'end_cycle': cycle,
                            'cost': cost
                        })
                    prev_cycle = cycle
                elif instr_num > 50:
                    break
    
    # Print detailed breakdown
    print("\nDETAILED INSTRUCTION-BY-INSTRUCTION BREAKDOWN:")
    print("-" * 80)
    print(f"{'Instr#':>7} {'Start':>10} {'End':>10} {'Cost':>6}")
    print("-" * 80)
    
    for inst in instructions:
        print(f"#{inst['num']:>6}  {inst['start_cycle']:>10}  {inst['end_cycle']:>10}  {inst['cost']:>6}")
    
    # Calculate statistics
    costs = [inst['cost'] for inst in instructions]
    cost_distribution = Counter(costs)
    
    total_instructions = len(instructions)
    total_cycles = sum(costs)
    avg_cycles = total_cycles / total_instructions if total_instructions > 0 else 0
    
    print("\n" + "=" * 80)
    print("STATISTICS:")
    print("=" * 80)
    print(f"Total instructions analyzed: {total_instructions}")
    print(f"Total cycles: {total_cycles}")
    print(f"Average cycles/instruction: {avg_cycles:.4f}")
    
    print(f"\nCYCLE COST DISTRIBUTION:")
    for cost in sorted(cost_distribution.keys()):
        count = cost_distribution[cost]
        pct = 100.0 * count / total_instructions
        print(f"  {cost:2d} cycles: {count:4d} instructions ({pct:5.1f}%)")
    
    # Manual verification
    print("\n" + "=" * 80)
    print("MANUAL VERIFICATION:")
    print("=" * 80)
    print("Expected for BIOS execution:")
    print("  - Simple instructions (data proc, load/store): 2 cycles")
    print("  - Branches: 4 cycles (2 prefetch + 2 pipeline refill)")
    print("  - First instruction: 4 cycles (includes initial pipeline fill)")
    print()
    
    # Count by expected categories
    two_cycle = sum(1 for c in costs if c == 2)
    four_cycle = sum(1 for c in costs if c == 4)
    other = sum(1 for c in costs if c not in [2, 4])
    
    print(f"Actual distribution:")
    print(f"  2-cycle instructions: {two_cycle} ({100.0*two_cycle/total_instructions:.1f}%)")
    print(f"  4-cycle instructions: {four_cycle} ({100.0*four_cycle/total_instructions:.1f}%)")
    print(f"  Other:                {other} ({100.0*other/total_instructions:.1f}%)")
    
    if other > 0:
        print(f"\n⚠️  WARNING: Found {other} instructions with unexpected cycle costs!")
        print("    Expected only 2 or 4 cycles for BIOS execution.")
        other_costs = [inst for inst in instructions if inst['cost'] not in [2, 4]]
        print("\n    Unexpected instructions:")
        for inst in other_costs[:10]:  # Show first 10
            print(f"      Instruction #{inst['num']}: {inst['cost']} cycles")
    else:
        print("\n✓ All instructions have expected cycle costs (2 or 4)!")
    
    # Check if pattern matches expectations
    print("\n" + "=" * 80)
    print("PATTERN ANALYSIS:")
    print("=" * 80)
    
    # In typical BIOS code, we expect branches roughly every 5-10 instructions
    branch_ratio = four_cycle / total_instructions if total_instructions > 0 else 0
    print(f"Branch ratio: {100*branch_ratio:.1f}% (4-cycle instructions)")
    if 10 <= branch_ratio * 100 <= 30:
        print("✓ Branch ratio seems reasonable (10-30%)")
    else:
        print(f"⚠️  Branch ratio seems unusual (expected 10-30%)")

if __name__ == "__main__":
    trace_file = "/tmp/gba_memory_trace.log"
    analyze_first_50(trace_file)
