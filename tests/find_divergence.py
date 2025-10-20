#!/usr/bin/env python3
"""
Binary search to find where GBA emulator diverges from mGBA trace.
Compares PC, registers (R0-R15), CPSR, and IE/IF/IME at each instruction.
"""

import sys
import re
import gzip

def parse_mgba_line(line):
    """Parse an mGBA trace line into a dict of values."""
    # Format: PC:08000130 R00:00000000 R01:00000001 ... CPSR:60000013 | IE:0001 IF:0000 IME:00000001
    match = re.match(r'PC:([0-9A-Fa-f]{8})\s+(.*?)\s+\|\s+IE:([0-9A-Fa-f]{4})\s+IF:([0-9A-Fa-f]{4})\s+IME:([0-9A-Fa-f]{8})', line)
    if not match:
        return None
    
    pc = match.group(1)
    regs_str = match.group(2)
    ie = match.group(3)
    if_val = match.group(4)
    ime = match.group(5)
    
    # Parse registers
    regs = {}
    for reg_match in re.finditer(r'R(\d{2}):([0-9A-Fa-f]{8})', regs_str):
        reg_num = int(reg_match.group(1))
        reg_val = reg_match.group(2)
        regs[reg_num] = reg_val
    
    # Parse CPSR
    cpsr_match = re.search(r'CPSR:([0-9A-Fa-f]{8})', regs_str)
    cpsr = cpsr_match.group(1) if cpsr_match else None
    
    return {
        'pc': pc.upper(),
        'regs': regs,
        'cpsr': cpsr.upper() if cpsr else None,
        'ie': ie.upper(),
        'if': if_val.upper(),
        'ime': ime.upper()
    }

def parse_gba_line(line):
    """Parse a GBA emulator trace line into a dict of values."""
    # Same format as mGBA
    return parse_mgba_line(line)

def compare_states(mgba_state, gba_state, inst_num):
    """Compare two instruction states and return differences."""
    diffs = []
    
    if mgba_state['pc'] != gba_state['pc']:
        diffs.append(f"PC: mGBA={mgba_state['pc']} GBA={gba_state['pc']}")
    
    for reg_num in range(16):
        if reg_num in mgba_state['regs'] and reg_num in gba_state['regs']:
            if mgba_state['regs'][reg_num] != gba_state['regs'][reg_num]:
                diffs.append(f"R{reg_num:02d}: mGBA={mgba_state['regs'][reg_num]} GBA={gba_state['regs'][reg_num]}")
    
    if mgba_state['cpsr'] and gba_state['cpsr']:
        if mgba_state['cpsr'] != gba_state['cpsr']:
            diffs.append(f"CPSR: mGBA={mgba_state['cpsr']} GBA={gba_state['cpsr']}")
    
    if mgba_state['ie'] != gba_state['ie']:
        diffs.append(f"IE: mGBA={mgba_state['ie']} GBA={gba_state['ie']}")
    
    if mgba_state['if'] != gba_state['if']:
        diffs.append(f"IF: mGBA={mgba_state['if']} GBA={gba_state['if']}")
    
    if mgba_state['ime'] != gba_state['ime']:
        diffs.append(f"IME: mGBA={mgba_state['ime']} GBA={gba_state['ime']}")
    
    return diffs

def check_sync_at_instruction(mgba_file, gba_file, target_inst):
    """Check if traces are in sync at a specific instruction number."""
    print(f"\nChecking instruction {target_inst:,}...")
    
    # Open files
    if mgba_file.endswith('.gz'):
        mgba_f = gzip.open(mgba_file, 'rt')
    else:
        mgba_f = open(mgba_file, 'r')
    
    gba_f = open(gba_file, 'r')
    
    # Skip to target instruction
    # Both traces now have 1 line per instruction (compact format)
    mgba_line_num = target_inst  # Direct line number for mGBA
    gba_line_num = target_inst  # Direct line number for GBA
    
    # Read mGBA line
    for i, line in enumerate(mgba_f, 1):
        if i == mgba_line_num:
            mgba_line = line.strip()
            break
    else:
        mgba_f.close()
        gba_f.close()
        return None, "mGBA trace too short"
    
    # Read GBA line
    for i, line in enumerate(gba_f, 1):
        if i == gba_line_num:
            gba_line = line.strip()
            break
    else:
        mgba_f.close()
        gba_f.close()
        return None, "GBA trace too short"
    
    mgba_f.close()
    gba_f.close()
    
    # Print the lines being compared
    print(f"  mGBA line {mgba_line_num}: {mgba_line[:120]}...")
    print(f"  GBA  line {gba_line_num}: {gba_line[:120]}...")
    
    # Parse both lines
    mgba_state = parse_mgba_line(mgba_line)
    gba_state = parse_gba_line(gba_line)
    
    if not mgba_state:
        return None, f"Failed to parse mGBA line: {mgba_line}"
    if not gba_state:
        return None, f"Failed to parse GBA line: {gba_line}"
    
    # Compare
    diffs = compare_states(mgba_state, gba_state, target_inst)
    
    if diffs:
        return False, diffs
    else:
        return True, None

def binary_search_divergence(mgba_file, gba_file, known_good, known_bad):
    """Binary search to find first divergence point."""
    print(f"\n{'='*80}")
    print(f"Binary search between instruction {known_good:,} (in sync) and {known_bad:,} (out of sync)")
    print(f"{'='*80}")
    
    left = known_good
    right = known_bad
    
    while left < right - 1:
        mid = (left + right) // 2
        in_sync, result = check_sync_at_instruction(mgba_file, gba_file, mid)
        
        if in_sync is None:
            print(f"ERROR at instruction {mid:,}: {result}")
            return None
        
        if in_sync:
            print(f"  ✓ Instruction {mid:,} is IN SYNC")
            left = mid
        else:
            print(f"  ✗ Instruction {mid:,} is OUT OF SYNC")
            print(f"    Differences: {result}")
            right = mid
    
    print(f"\n{'='*80}")
    print(f"DIVERGENCE FOUND between instructions {left:,} and {right:,}")
    print(f"{'='*80}")
    
    # Show detailed comparison at divergence point
    print(f"\nDetailed comparison at instruction {right:,}:")
    in_sync, diffs = check_sync_at_instruction(mgba_file, gba_file, right)
    for diff in diffs:
        print(f"  {diff}")
    
    # Also show the previous instruction (should be in sync)
    print(f"\nPrevious instruction {left:,} (should be in sync):")
    in_sync, result = check_sync_at_instruction(mgba_file, gba_file, left)
    if in_sync:
        print(f"  ✓ Confirmed in sync")
    else:
        print(f"  ⚠ WARNING: Previous instruction also shows differences!")
    
    return right

def main():
    if len(sys.argv) < 3:
        print("Usage: find_divergence.py <mgba_trace> <gba_trace> [known_good] [known_bad]")
        print()
        print("Arguments:")
        print("  mgba_trace   - Path to mGBA trace file (can be .gz)")
        print("  gba_trace    - Path to GBA emulator trace file")
        print("  known_good   - Instruction number known to be in sync (default: 2500)")
        print("  known_bad    - Instruction number known to be out of sync (default: will test incrementally)")
        sys.exit(1)
    
    mgba_file = sys.argv[1]
    gba_file = sys.argv[2]
    known_good = int(sys.argv[3]) if len(sys.argv) > 3 else 2500
    
    if len(sys.argv) > 4:
        known_bad = int(sys.argv[4])
    else:
        # Find known_bad by testing exponentially increasing points
        print("Finding upper bound where traces diverge...")
        test_point = known_good * 2
        
        while True:
            in_sync, result = check_sync_at_instruction(mgba_file, gba_file, test_point)
            
            if in_sync is None:
                # Reached end of trace
                print(f"\nReached end of trace at ~{test_point:,} instructions")
                print("ERROR: Both traces appear to be in sync throughout!")
                sys.exit(1)
            
            if not in_sync:
                known_bad = test_point
                print(f"  ✗ Found divergence at instruction {test_point:,}")
                break
            else:
                print(f"  ✓ Still in sync at instruction {test_point:,}")
                test_point *= 2
    
    # Now do binary search
    divergence_point = binary_search_divergence(mgba_file, gba_file, known_good, known_bad)
    
    if divergence_point:
        print(f"\n{'='*80}")
        print(f"RESULT: First divergence occurs at instruction {divergence_point:,}")
        print(f"{'='*80}")
        print(f"\nNext steps:")
        print(f"1. Look at both traces around instruction {divergence_point:,}")
        print(f"2. Check what instruction is being executed at PC in instruction {divergence_point-1:,}")
        print(f"3. Debug why that instruction produces different results")

if __name__ == '__main__':
    main()
