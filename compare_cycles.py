#!/usr/bin/env python3
"""
Compare cycle costs between our emulator and expected timing (based on GBATEK/mGBA model)

This script:
1. Parses our emulator's trace
2. Disassembles each instruction 
3. Calculates expected cycle cost based on GBATEK timing
4. Compares actual vs expected
5. Reports discrepancies
"""

import re
import sys

def parse_our_trace(filename, max_instructions=1000):
    """Parse our emulator's trace and extract instruction info"""
    instructions = []
    
    with open(filename, 'r') as f:
        current_instr = None
        
        for line in f:
            # Match: Instruction #N (Cycle: X)
            m = re.match(r'Instruction #(\d+) \(Cycle: (\d+)\)', line)
            if m:
                instr_num = int(m.group(1))
                cycle = int(m.group(2))
                
                if current_instr:
                    instructions.append(current_instr)
                
                current_instr = {
                    'num': instr_num,
                    'cycle': cycle,
                    'pc': None,
                    'cpsr': None,
                    'is_thumb': False
                }
                
                if instr_num >= max_instructions:
                    break
            
            # Match: PC: 0xXXXXXXXX
            elif current_instr and line.startswith('PC: 0x'):
                current_instr['pc'] = int(line.split()[1], 16)
            
            # Match: cpsr=0xXXXXXXXX [...]
            elif current_instr and line.startswith('cpsr=0x'):
                cpsr_hex = line.split()[0].split('=')[1]
                current_instr['cpsr'] = int(cpsr_hex, 16)
                current_instr['is_thumb'] = bool((current_instr['cpsr'] >> 5) & 1)
        
        # Add last instruction
        if current_instr:
            instructions.append(current_instr)
    
    return instructions

def calculate_expected_cycles_simple(pc, is_thumb, prev_pc=None):
    """
    Calculate expected cycle cost based on GBATEK timing model
    
    For now, use simple heuristics:
    - Most instructions: 1S (1 cycle in BIOS)
    - Branches: 2S + 1N (3 cycles in BIOS)
    - We'll detect branches by PC jump
    """
    
    if prev_pc is None:
        # First instruction (startup branch)
        return 3  # Initial branch takes 3 cycles
    
    # Detect branch by checking if PC didn't advance sequentially
    if is_thumb:
        sequential_increment = 2
    else:
        sequential_increment = 4
    
    expected_next_pc = prev_pc + sequential_increment
    
    if pc != expected_next_pc:
        # Non-sequential PC = branch taken
        return 3  # Branch: 2S + 1N = 3 cycles (BIOS)
    else:
        # Sequential execution
        return 1  # Most ARM/THUMB instructions: 1S = 1 cycle (BIOS)

def analyze_timing(instructions):
    """Analyze timing and find discrepancies"""
    
    print("=" * 80)
    print("CYCLE TIMING ANALYSIS")
    print("=" * 80)
    print()
    
    discrepancies = []
    cycle_costs = {1: 0, 2: 0, 3: 0, 4: 0, 5: 0, 'other': 0}
    expected_costs = {1: 0, 2: 0, 3: 0, 4: 0, 5: 0, 'other': 0}
    
    for i in range(1, len(instructions)):
        curr = instructions[i]
        prev = instructions[i-1]
        
        actual_cycles = curr['cycle'] - prev['cycle']
        expected_cycles = calculate_expected_cycles_simple(
            curr['pc'], curr['is_thumb'], prev['pc']
        )
        
        # Track distribution
        if actual_cycles in cycle_costs:
            cycle_costs[actual_cycles] += 1
        else:
            cycle_costs['other'] += 1
        
        if expected_cycles in expected_costs:
            expected_costs[expected_cycles] += 1
        else:
            expected_costs['other'] += 1
        
        # Record discrepancy
        if actual_cycles != expected_cycles:
            discrepancies.append({
                'num': curr['num'],
                'pc': curr['pc'],
                'prev_pc': prev['pc'],
                'mode': 'THUMB' if curr['is_thumb'] else 'ARM',
                'actual': actual_cycles,
                'expected': expected_cycles,
                'diff': actual_cycles - expected_cycles
            })
    
    # Print summary
    print("CYCLE COST DISTRIBUTION:")
    print(f"  1 cycle: {cycle_costs[1]:5d} instructions (actual) vs {expected_costs[1]:5d} (expected)")
    print(f"  2 cycles: {cycle_costs[2]:5d} instructions (actual) vs {expected_costs[2]:5d} (expected)")
    print(f"  3 cycles: {cycle_costs[3]:5d} instructions (actual) vs {expected_costs[3]:5d} (expected)")
    print(f"  4 cycles: {cycle_costs[4]:5d} instructions (actual) vs {expected_costs[4]:5d} (expected)")
    print(f"  5 cycles: {cycle_costs[5]:5d} instructions (actual) vs {expected_costs[5]:5d} (expected)")
    print(f"  Other: {cycle_costs['other']:5d} instructions (actual) vs {expected_costs['other']:5d} (expected)")
    print()
    
    # Print discrepancies
    if discrepancies:
        print(f"FOUND {len(discrepancies)} DISCREPANCIES:")
        print()
        print("First 20 discrepancies:")
        for d in discrepancies[:20]:
            print(f"  Instr #{d['num']:4d}: PC=0x{d['pc']:08X} (from 0x{d['prev_pc']:08X})")
            print(f"    Mode: {d['mode']}, Actual: {d['actual']} cycles, Expected: {d['expected']} cycles, Diff: {d['diff']:+d}")
            print()
    else:
        print("✓ No discrepancies found! All instruction cycles match expected values.")
    
    # Summary
    total_actual = sum(cycle_costs.values())
    total_expected = sum(expected_costs.values())
    print()
    print("SUMMARY:")
    print(f"  Total instructions analyzed: {total_actual}")
    print(f"  Discrepancies: {len(discrepancies)} ({100*len(discrepancies)/total_actual if total_actual > 0 else 0:.1f}%)")
    
    if len(instructions) > 1:
        first_cycle = instructions[0]['cycle']
        last_cycle = instructions[-1]['cycle']
        total_cycles = last_cycle - first_cycle
        avg_cycles = total_cycles / len(instructions) if len(instructions) > 0 else 0
        print(f"  Total cycles: {total_cycles}")
        print(f"  Average cycles/instruction: {avg_cycles:.2f}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 compare_cycles.py /tmp/gba_memory_trace.log [max_instructions]")
        sys.exit(1)
    
    trace_file = sys.argv[1]
    max_instructions = int(sys.argv[2]) if len(sys.argv) > 2 else 1000
    
    print(f"Parsing trace: {trace_file}")
    print(f"Max instructions: {max_instructions}")
    print()
    
    instructions = parse_our_trace(trace_file, max_instructions)
    print(f"Parsed {len(instructions)} instructions")
    print()
    
    analyze_timing(instructions)

if __name__ == '__main__':
    main()
