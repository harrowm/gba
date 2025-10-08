#!/usr/bin/env python3
"""
Analyze BIOS trace to understand execution flow and loops
"""

import re
import sys
from collections import defaultdict, Counter

def analyze_trace(trace_file):
    # Track execution counts per address
    address_counts = Counter()
    
    # Track sequences of addresses
    address_sequence = []
    
    # Track unique addresses visited
    unique_addresses = set()
    
    # Track transitions (from_addr -> to_addr)
    transitions = defaultdict(int)
    
    # Track instruction details per address
    instruction_at_address = {}
    
    # Track how many times we hit address 0
    address_zero_count = 0
    
    last_pc = None
    total_instructions = 0
    
    print("Reading trace file...")
    
    with open(trace_file, 'r') as f:
        for line in f:
            # Match BIOS trace lines
            match = re.search(r'\[(\d+)\]\[(ARM|THUMB)-BIOS\] PC=0x([0-9A-F]{8}):\s+(\S+)\s+(.+?)\s+\|', line)
            if match:
                count = int(match.group(1))
                mode = match.group(2)
                pc = int(match.group(3), 16)
                mnemonic = match.group(4)
                operands = match.group(5).strip()
                
                total_instructions += 1
                address_counts[pc] += 1
                unique_addresses.add(pc)
                address_sequence.append(pc)
                
                # Store instruction
                if pc not in instruction_at_address:
                    instruction_at_address[pc] = f"{mnemonic} {operands}"
                
                # Track transitions
                if last_pc is not None:
                    transitions[(last_pc, pc)] += 1
                
                # Check for address 0
                if pc == 0:
                    address_zero_count += 1
                    print(f"  [!] Hit address 0x00000000 at instruction {count}")
                
                last_pc = pc
    
    print(f"\nTotal instructions executed: {total_instructions}")
    print(f"Unique addresses visited: {len(unique_addresses)}")
    print(f"Times address 0x00000000 was hit: {address_zero_count}")
    
    # Find most executed addresses (hot spots)
    print("\n" + "="*80)
    print("TOP 50 MOST EXECUTED ADDRESSES (Hot Spots / Loops)")
    print("="*80)
    
    for pc, count in address_counts.most_common(50):
        instr = instruction_at_address.get(pc, "???")
        percentage = (count / total_instructions) * 100
        print(f"  0x{pc:08X}: {count:7} times ({percentage:5.2f}%) - {instr}")
    
    # Find address ranges that form loops
    print("\n" + "="*80)
    print("LOOP DETECTION")
    print("="*80)
    
    # Look for backward branches (PC goes to lower address)
    backward_branches = []
    for (from_pc, to_pc), count in transitions.items():
        if to_pc < from_pc and count > 10:  # Significant backward branch
            backward_branches.append((from_pc, to_pc, count))
    
    backward_branches.sort(key=lambda x: x[2], reverse=True)
    
    print("\nTop backward branches (likely loops):")
    for from_pc, to_pc, count in backward_branches[:20]:
        from_instr = instruction_at_address.get(from_pc, "???")
        to_instr = instruction_at_address.get(to_pc, "???")
        print(f"  0x{from_pc:08X} -> 0x{to_pc:08X}: {count} times")
        print(f"    {from_instr} -> {to_instr}")
    
    # Find sequences of addresses (execution flow)
    print("\n" + "="*80)
    print("EXECUTION FLOW SUMMARY")
    print("="*80)
    
    # Group consecutive addresses into regions
    if address_sequence:
        regions = []
        current_region_start = address_sequence[0]
        current_region_end = address_sequence[0]
        region_count = 1
        
        for i in range(1, len(address_sequence)):
            pc = address_sequence[i]
            prev_pc = address_sequence[i-1]
            
            # Check if this is a continuation (within 16 bytes) or a jump
            if abs(pc - prev_pc) <= 16:
                current_region_end = pc
                region_count += 1
            else:
                # Save current region
                if region_count > 5:  # Only show regions with multiple instructions
                    regions.append((current_region_start, current_region_end, region_count))
                
                # Start new region
                current_region_start = pc
                current_region_end = pc
                region_count = 1
        
        # Save last region
        if region_count > 5:
            regions.append((current_region_start, current_region_end, region_count))
        
        print(f"\nFound {len(regions)} execution regions:")
        for start, end, count in regions[:50]:  # Show first 50
            start_instr = instruction_at_address.get(start, "???")
            print(f"  0x{start:08X} - 0x{end:08X}: {count} instructions")
            print(f"    Starts with: {start_instr}")
    
    return {
        'total_instructions': total_instructions,
        'unique_addresses': unique_addresses,
        'address_counts': address_counts,
        'instruction_at_address': instruction_at_address,
        'address_zero_count': address_zero_count,
        'transitions': transitions,
        'backward_branches': backward_branches
    }

def generate_markdown_report(analysis_data):
    """Generate a markdown report of BIOS execution"""
    
    md = []
    md.append("# GBA BIOS Execution Analysis\n")
    md.append(f"**Total Instructions Executed:** {analysis_data['total_instructions']:,}\n")
    md.append(f"**Unique Addresses Visited:** {len(analysis_data['unique_addresses'])}\n")
    md.append(f"**Times Address 0x00000000 Hit:** {analysis_data['address_zero_count']}\n")
    
    md.append("\n## Summary\n")
    if analysis_data['address_zero_count'] > 1:
        md.append(f"⚠️ **BIOS appears to be RESTARTING!** Address 0 was hit {analysis_data['address_zero_count']} times.\n")
    
    if analysis_data['total_instructions'] > 100000:
        md.append(f"⚠️ **BIOS is executing too many instructions!** {analysis_data['total_instructions']:,} instructions in 16KB of BIOS code suggests infinite loops.\n")
    
    md.append("\n## Hot Spots (Most Executed Code)\n")
    md.append("These addresses are executed most frequently, indicating loops:\n\n")
    md.append("| Address | Count | % | Instruction |\n")
    md.append("|---------|-------|---|-------------|\n")
    
    total = analysis_data['total_instructions']
    for pc, count in analysis_data['address_counts'].most_common(30):
        instr = analysis_data['instruction_at_address'].get(pc, "???")
        percentage = (count / total) * 100
        md.append(f"| 0x{pc:08X} | {count:,} | {percentage:.2f}% | `{instr}` |\n")
    
    md.append("\n## Loop Analysis\n")
    md.append("Backward branches that execute frequently:\n\n")
    
    for from_pc, to_pc, count in analysis_data['backward_branches'][:15]:
        from_instr = analysis_data['instruction_at_address'].get(from_pc, "???")
        to_instr = analysis_data['instruction_at_address'].get(to_pc, "???")
        md.append(f"### Loop: 0x{to_pc:08X} - 0x{from_pc:08X} ({count:,} iterations)\n")
        md.append(f"- **Branch from:** `0x{from_pc:08X}: {from_instr}`\n")
        md.append(f"- **Branch to:** `0x{to_pc:08X}: {to_instr}`\n")
        md.append(f"- **Executed:** {count:,} times\n\n")
    
    md.append("\n## Expected BIOS Flow\n")
    md.append("""
The GBA BIOS should follow this general flow:

1. **Reset (0x00000000)**: Initial entry point
2. **Mode Setup (0x00000068-0x000000AC)**: Set up CPU modes and stacks
3. **THUMB Initialization (0x00000118-0x00001928)**: Switch to THUMB mode, clear memory
4. **Return to ARM (0x000000B4)**: Load ROM entry point from header
5. **Jump to ROM (0x08000000)**: Start executing game code

If the BIOS executes more than ~40,000 instructions, it's stuck in a loop and not reaching the ROM.
""")
    
    return ''.join(md)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python analyze_bios_trace.py <trace_file>")
        sys.exit(1)
    
    trace_file = sys.argv[1]
    
    print(f"Analyzing BIOS trace: {trace_file}")
    print("="*80)
    
    analysis_data = analyze_trace(trace_file)
    
    # Generate markdown report
    print("\n" + "="*80)
    print("GENERATING MARKDOWN REPORT")
    print("="*80)
    
    md_report = generate_markdown_report(analysis_data)
    
    output_file = '/Users/malcolm/gba/docs/BIOS_EXECUTION_ANALYSIS.md'
    with open(output_file, 'w') as f:
        f.write(md_report)
    
    print(f"\nMarkdown report saved to: {output_file}")
