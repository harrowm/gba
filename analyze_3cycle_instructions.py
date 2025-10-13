#!/usr/bin/env python3
"""
Identify what instructions are costing 3 cycles to find the timing issue.
"""

import re

def analyze_3cycle_instructions(trace_file):
    """Find instructions that cost 3 cycles and identify what they are."""
    
    print("=" * 80)
    print("3-CYCLE INSTRUCTION ANALYSIS")
    print("=" * 80)
    
    # Track instruction info
    instruction_info = {}  # instr_num -> (pc, instruction, r0-r3, sp, lr, r12)
    cycle_costs = {}       # instr_num -> cycle_cost
    
    current_instr_num = None
    prev_cycle = 0
    
    with open(trace_file, 'r') as f:
        for line in f:
            # Match instruction header: "Instruction #424 (Cycle: 965)"
            instr_match = re.match(r'Instruction #(\d+) \(Cycle: (\d+)\)', line)
            if instr_match:
                instr_num = int(instr_match.group(1))
                cycle = int(instr_match.group(2))
                
                if prev_cycle > 0:
                    cost = cycle - prev_cycle
                    cycle_costs[current_instr_num] = cost
                
                prev_cycle = cycle
                current_instr_num = instr_num
            
            # Match instruction details: "[  1] BIOS PC=0x00000000: Instr=0xEA000018 | R0-R3=..."
            detail_match = re.match(r'\[\s*\d+\] \w+ PC=(0x[0-9A-F]+): Instr=(0x[0-9A-F]+)', line)
            if detail_match and current_instr_num:
                pc = detail_match.group(1)
                instr = detail_match.group(2)
                instruction_info[current_instr_num] = (pc, instr, line.strip())
    
    # Find all 3-cycle instructions in first 450
    three_cycle_instrs = [(num, cost) for num, cost in cycle_costs.items() 
                          if cost == 3 and num <= 450]
    
    print(f"\nFound {len(three_cycle_instrs)} instructions costing 3 cycles (out of first 450)")
    print(f"This is {100.0 * len(three_cycle_instrs) / 450:.1f}% of all instructions!")
    print()
    
    # Decode instruction types
    branch_count = 0
    load_store_count = 0
    data_proc_count = 0
    multiply_count = 0
    other_count = 0
    
    print("Sample of 3-cycle instructions:")
    print("-" * 80)
    
    for i, (num, cost) in enumerate(three_cycle_instrs[:30]):
        if num in instruction_info:
            pc, instr_hex, full_line = instruction_info[num]
            instr_val = int(instr_hex, 16)
            
            # Decode instruction type (ARM)
            bits_27_25 = (instr_val >> 25) & 0x7
            bits_27_20 = (instr_val >> 20) & 0xFF
            
            instr_type = "UNKNOWN"
            
            # Branch: bits[27:25] = 101
            if bits_27_25 == 0b101:
                instr_type = "BRANCH"
                branch_count += 1
            # Data processing: bits[27:26] = 00
            elif (instr_val & 0x0C000000) == 0x00000000:
                # Check for multiply: bits[27:22] = 000000, bits[7:4] = 1001
                if (instr_val & 0x0FC000F0) == 0x00000090:
                    instr_type = "MULTIPLY"
                    multiply_count += 1
                else:
                    instr_type = "DATA_PROC"
                    data_proc_count += 1
            # Load/Store: bits[27:26] = 01
            elif (instr_val & 0x0C000000) == 0x04000000:
                instr_type = "LOAD/STORE"
                load_store_count += 1
            # Block transfer: bits[27:25] = 100
            elif bits_27_25 == 0b100:
                instr_type = "BLOCK_XFER"
                other_count += 1
            else:
                other_count += 1
            
            if i < 30:
                print(f"#{num:4d} @ {pc}: {instr_hex} = {instr_type:12s}")
    
    print("\n" + "-" * 80)
    print("INSTRUCTION TYPE BREAKDOWN:")
    print("-" * 80)
    total = len(three_cycle_instrs)
    print(f"Branches:      {branch_count:4d} ({100.0*branch_count/total:5.1f}%)")
    print(f"Load/Store:    {load_store_count:4d} ({100.0*load_store_count/total:5.1f}%)")
    print(f"Data Proc:     {data_proc_count:4d} ({100.0*data_proc_count/total:5.1f}%)")
    print(f"Multiply:      {multiply_count:4d} ({100.0*multiply_count/total:5.1f}%)")
    print(f"Other:         {other_count:4d} ({100.0*other_count/total:5.1f}%)")
    print(f"TOTAL:         {total:4d}")
    
    print("\n" + "=" * 80)
    print("DIAGNOSIS:")
    print("=" * 80)
    
    if branch_count == total:
        print("✓ All 3-cycle instructions are branches - this is CORRECT!")
        print("  Branches should cost 3 cycles (1 prefetch + 2 pipeline refill)")
    elif branch_count > total * 0.9:
        print(f"✓ Most 3-cycle instructions are branches ({100.0*branch_count/total:.1f}%)")
        print(f"⚠ But {total - branch_count} non-branch instructions also cost 3 cycles")
        print("  Need to investigate why non-branches cost 3 cycles")
    else:
        print(f"✗ PROBLEM: Only {branch_count} branches, but {total} 3-cycle instructions!")
        print(f"  {total - branch_count} NON-BRANCH instructions are costing 3 cycles")
        print()
        if load_store_count > 0:
            print(f"  → {load_store_count} Load/Store instructions cost 3 cycles")
            print("    Expected: 1 cycle (prefetch) + 0 (BIOS/IWRAM are free)")
            print("    Check: Are we charging extra cycles for loads/stores?")
        if data_proc_count > 0:
            print(f"  → {data_proc_count} Data Processing instructions cost 3 cycles")
            print("    Expected: 1 cycle (prefetch only)")
            print("    Check: Are we adding wrong base cycles?")
        if multiply_count > 0:
            print(f"  → {multiply_count} Multiply instructions cost 3 cycles")
            print("    Expected: 2-5 cycles depending on operand")
            print("    Check: Multiply cycle calculation")

if __name__ == "__main__":
    trace_file = "/tmp/gba_memory_trace.log"
    analyze_3cycle_instructions(trace_file)
