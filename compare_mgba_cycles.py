#!/usr/bin/env python3
"""
Compare cycle counts between our GBA emulator and instrumented mGBA.

This script compares instruction-by-instruction cycle counts from both emulators
to identify where timing diverges.
"""

import re
import sys

def parse_our_trace(filename):
    """Parse our emulator trace format from gba_memory_trace.log"""
    instructions = []
    with open(filename, 'r') as f:
        lines = f.readlines()
        i = 0
        while i < len(lines):
            line = lines[i]
            # Match: Instruction #703 (Cycle: 1240) OR Instruction #1 (without cycle)
            match = re.search(r'Instruction #(\d+)(?: \(Cycle: (\d+)\))?', line)
            if match:
                inst_num = int(match.group(1))
                cycle = int(match.group(2)) if match.group(2) else 0  # Instruction #1 is at cycle 0
                # Next few lines should have PC info
                # Look for: PC: 0x00000000
                for j in range(i+1, min(i+5, len(lines))):
                    pc_match = re.search(r'PC: 0x([0-9a-fA-F]+)', lines[j])
                    if pc_match:
                        pc = int(pc_match.group(1), 16)
                        instructions.append((inst_num, cycle, pc))
                        break
            i += 1
    return instructions

def parse_mgba_trace(filename):
    """Parse mGBA instrumented trace format: 'ARM: PC=00000000 Cycles=0'"""
    instructions = []
    with open(filename, 'r') as f:
        for inst_num, line in enumerate(f, start=0):
            # Match: ARM: PC=00000000 Cycles=0 or THUMB: PC=0000000c Cycles=5
            match = re.search(r'(ARM|THUMB): PC=([0-9a-fA-F]+) Cycles=(\d+)', line)
            if match:
                mode = match.group(1)
                pc = int(match.group(2), 16)
                cycle = int(match.group(3))
                instructions.append((inst_num, cycle, pc))
    return instructions

def compare_traces(our_trace, mgba_trace, max_instructions=1000):
    """Compare the two traces instruction by instruction."""
    print(f"Loaded {len(our_trace)} instructions from our emulator")
    print(f"Loaded {len(mgba_trace)} instructions from mGBA")
    print()
    
    min_len = min(len(our_trace), len(mgba_trace), max_instructions)
    
    mismatches = []
    cycle_diffs = []
    
    for i in range(min_len):
        our_inst_num, our_cycle, our_pc = our_trace[i]
        mgba_inst_num, mgba_cycle, mgba_pc = mgba_trace[i]
        
        # Check if PCs match
        if our_pc != mgba_pc:
            mismatches.append({
                'index': i,
                'our': (our_inst_num, our_cycle, our_pc),
                'mgba': (mgba_inst_num, mgba_cycle, mgba_pc)
            })
            print(f"PC MISMATCH at instruction {i}:")
            print(f"  Our:  Inst #{our_inst_num:4d} Cycle {our_cycle:5d} PC=0x{our_pc:08x}")
            print(f"  mGBA: Inst #{mgba_inst_num:4d} Cycle {mgba_cycle:5d} PC=0x{mgba_pc:08x}")
            print()
            
            # Stop at first PC mismatch as execution has diverged
            if len(mismatches) >= 5:
                print("Too many PC mismatches, stopping comparison.")
                break
        else:
            # PCs match, check cycle counts
            cycle_diff = our_cycle - mgba_cycle
            cycle_diffs.append(cycle_diff)
            
            if cycle_diff != 0:
                print(f"CYCLE DIFF at instruction {i}: PC=0x{our_pc:08x}")
                print(f"  Our:  Cycle {our_cycle:5d}")
                print(f"  mGBA: Cycle {mgba_cycle:5d}")
                print(f"  Diff: {cycle_diff:+d} cycles")
                print()
    
    # Summary statistics
    if not mismatches and cycle_diffs:
        print("\n" + "="*70)
        print("SUMMARY:")
        print("="*70)
        print(f"PC sequences match for {min_len} instructions ✓")
        
        if all(d == 0 for d in cycle_diffs):
            print(f"Cycle counts MATCH PERFECTLY ✓✓✓")
        else:
            unique_diffs = set(cycle_diffs)
            print(f"Cycle differences found:")
            print(f"  Min diff: {min(cycle_diffs):+d}")
            print(f"  Max diff: {max(cycle_diffs):+d}")
            print(f"  Final diff at inst {min_len-1}: {cycle_diffs[-1]:+d}")
            
            # Check if it's a constant offset
            if len(unique_diffs) == 1 and 0 not in unique_diffs:
                diff = unique_diffs.pop()
                print(f"\n  CONSTANT OFFSET: {diff:+d} cycles")
                if diff > 0:
                    print(f"  → Our emulator is {abs(diff)} cycles AHEAD of mGBA")
                else:
                    print(f"  → Our emulator is {abs(diff)} cycles BEHIND mGBA")
            elif len(unique_diffs) <= 5:
                print(f"  Unique differences: {sorted(unique_diffs)}")
    
    return mismatches, cycle_diffs

def main():
    if len(sys.argv) < 3:
        print("Usage: python compare_mgba_cycles.py <our_trace.log> <mgba_trace.log>")
        print()
        print("Example:")
        print("  python compare_mgba_cycles.py /tmp/gba_memory_trace.log /tmp/mgba_instrumented_trace.log")
        sys.exit(1)
    
    our_trace_file = sys.argv[1]
    mgba_trace_file = sys.argv[2]
    
    print("Loading traces...")
    our_trace = parse_our_trace(our_trace_file)
    mgba_trace = parse_mgba_trace(mgba_trace_file)
    
    print()
    compare_traces(our_trace, mgba_trace)

if __name__ == '__main__':
    main()
