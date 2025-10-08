#!/usr/bin/env python3
"""
Analyze BIOS execution flow and create a detailed markdown report
showing functional sections and their completion status
"""

import re
import sys
from collections import defaultdict, OrderedDict

def load_bios_disassembly(bios_file):
    """Load BIOS disassembly to understand what code sections do"""
    comments = {}
    labels = {}
    
    try:
        with open(bios_file, 'r') as f:
            for line in f:
                # Extract labels
                label_match = re.match(r'^(\w+):', line)
                if label_match:
                    label = label_match.group(1)
                    # Try to extract address from label like _00000068
                    addr_match = re.search(r'_([0-9A-F]{8})', label)
                    if addr_match:
                        addr = int(addr_match.group(1), 16)
                        labels[addr] = label
                
                # Extract comments
                comment_match = re.search(r'@\s*(.+)$', line)
                if comment_match and label_match:
                    comment = comment_match.group(1).strip()
                    if addr_match:
                        comments[addr] = comment
    except FileNotFoundError:
        print(f"Warning: Could not load {bios_file}")
    
    return labels, comments

def analyze_execution_flow(trace_file):
    """Analyze the execution flow from trace"""
    
    # Track execution by address ranges
    execution_log = []  # List of (instruction_num, pc, mode, mnemonic, operands, registers)
    
    # Track function calls (BL instructions)
    function_calls = []  # List of (from_pc, to_pc, lr)
    
    # Track mode switches
    mode_switches = []  # List of (instruction_num, from_mode, to_mode, pc)
    
    print("Reading trace file...")
    
    last_mode = None
    last_pc = None
    
    with open(trace_file, 'r') as f:
        for line in f:
            # Match ARM BIOS trace
            arm_match = re.search(r'\[(\d+)\]\[ARM-BIOS\] PC=0x([0-9A-F]{8}):\s+(\S+)\s+(.+?)\s+\|', line)
            if arm_match:
                count = int(arm_match.group(1))
                pc = int(arm_match.group(2), 16)
                mnemonic = arm_match.group(3)
                operands = arm_match.group(4).strip()
                
                # Extract registers
                reg_match = re.search(r'R0=([0-9A-F]{8})\s+R1=([0-9A-F]{8})\s+R2=([0-9A-F]{8})\s+R3=([0-9A-F]{8})', line)
                lr_match = re.search(r'LR=([0-9A-F]{8})', line)
                
                regs = {}
                if reg_match:
                    regs = {
                        'R0': int(reg_match.group(1), 16),
                        'R1': int(reg_match.group(2), 16),
                        'R2': int(reg_match.group(3), 16),
                        'R3': int(reg_match.group(4), 16),
                    }
                if lr_match:
                    regs['LR'] = int(lr_match.group(1), 16)
                
                execution_log.append((count, pc, 'ARM', mnemonic, operands, regs))
                
                # Detect mode switches
                if last_mode and last_mode != 'ARM':
                    mode_switches.append((count, last_mode, 'ARM', pc))
                last_mode = 'ARM'
                
                # Detect function calls (BL)
                if mnemonic.startswith('bl'):
                    # Extract target from operands
                    target_match = re.search(r'#0x([0-9a-f]+)', operands)
                    if target_match:
                        target = int(target_match.group(1), 16)
                        function_calls.append((pc, target, regs.get('LR', 0)))
                
                last_pc = pc
            
            # Match THUMB BIOS trace
            thumb_match = re.search(r'\[(\d+)\]\[THUMB-BIOS\] PC=0x([0-9A-F]{8}):\s+(\S+)\s+(.+?)\s+\|', line)
            if thumb_match:
                count = int(thumb_match.group(1))
                pc = int(thumb_match.group(2), 16)
                mnemonic = thumb_match.group(3)
                operands = thumb_match.group(4).strip()
                
                # Extract registers
                reg_match = re.search(r'R0=([0-9A-F]{8})\s+R1=([0-9A-F]{8})\s+R2=([0-9A-F]{8})\s+R3=([0-9A-F]{8})', line)
                lr_match = re.search(r'LR=([0-9A-F]{8})', line)
                
                regs = {}
                if reg_match:
                    regs = {
                        'R0': int(reg_match.group(1), 16),
                        'R1': int(reg_match.group(2), 16),
                        'R2': int(reg_match.group(3), 16),
                        'R3': int(reg_match.group(4), 16),
                    }
                if lr_match:
                    regs['LR'] = int(lr_match.group(1), 16)
                
                execution_log.append((count, pc, 'THUMB', mnemonic, operands, regs))
                
                # Detect mode switches
                if last_mode and last_mode != 'THUMB':
                    mode_switches.append((count, last_mode, 'THUMB', pc))
                last_mode = 'THUMB'
                
                # Detect function calls (BL)
                if mnemonic.startswith('bl'):
                    # Extract target from operands
                    target_match = re.search(r'#0x([0-9a-f]+)', operands)
                    if target_match:
                        target = int(target_match.group(1), 16)
                        function_calls.append((pc, target, regs.get('LR', 0)))
                
                last_pc = pc
            
            # Match BL parts
            bl_match = re.search(r'\[BL-PART[12]\] PC=0x([0-9A-F]{8}): BL \w+, (?:LR_temp|target)=0x([0-9A-F]{8})', line)
            if bl_match:
                pc = int(bl_match.group(1), 16)
                target = int(bl_match.group(2), 16)
                print(f"  Found BL at 0x{pc:08X} -> 0x{target:08X}")
    
    return execution_log, function_calls, mode_switches

def identify_functional_sections(execution_log, labels, comments):
    """Identify functional sections of BIOS execution"""
    
    sections = OrderedDict()
    
    # Define known BIOS sections based on typical GBA BIOS structure
    section_ranges = [
        (0x00000000, 0x00000003, "Exception Vectors", "Reset and exception handlers"),
        (0x00000068, 0x000000B3, "Reset Handler", "Initialize CPU modes, set up stacks"),
        (0x000000B4, 0x000000BB, "ROM Entry Point Loader", "Load ROM entry point and jump to game"),
        (0x000000BC, 0x00000117, "Startup Initialization (ARM)", "Further system initialization"),
        (0x00000118, 0x0000011F, "Memory Clear Setup", "Prepare to clear memory"),
        (0x00000120, 0x00000127, "Memory Clear Loop", "Clear IWRAM/EWRAM"),
        (0x00000800, 0x0000082F, "VBlank/HBlank Handler", "Wait for VBlank or HBlank"),
        (0x000006B2, 0x000006C7, "CRC/Checksum Function", "Calculate CRC or checksum"),
        (0x000009C2, 0x00000B9F, "Decompression Routines", "Various decompression functions"),
        (0x00001928, 0x00001FFF, "THUMB Initialization", "Main THUMB mode initialization"),
        (0x000013C0, 0x000013C7, "Function Epilog", "Return from function"),
    ]
    
    # Find which sections were executed
    for start, end, name, description in section_ranges:
        instructions_in_section = [(i, pc, mode, mnem, ops, regs) 
                                   for i, pc, mode, mnem, ops, regs in execution_log 
                                   if start <= pc <= end]
        
        if instructions_in_section:
            first_instr = instructions_in_section[0]
            last_instr = instructions_in_section[-1]
            
            sections[name] = {
                'range': (start, end),
                'description': description,
                'instruction_count': len(instructions_in_section),
                'first_execution': first_instr[0],  # instruction number
                'last_execution': last_instr[0],
                'first_pc': first_instr[1],
                'last_pc': last_instr[1],
                'instructions': instructions_in_section
            }
    
    return sections

def generate_flow_markdown(execution_log, function_calls, mode_switches, sections, labels, comments):
    """Generate detailed markdown report"""
    
    md = []
    md.append("# GBA BIOS Execution Flow Analysis\n\n")
    
    md.append("## Executive Summary\n\n")
    md.append(f"- **Total Instructions Executed**: {len(execution_log):,}\n")
    md.append(f"- **Functional Sections Identified**: {len(sections)}\n")
    md.append(f"- **Mode Switches**: {len(mode_switches)}\n")
    md.append(f"- **Function Calls (BL)**: {len(function_calls)}\n\n")
    
    if execution_log:
        first = execution_log[0]
        last = execution_log[-1]
        md.append(f"- **First Instruction**: #{first[0]} at 0x{first[1]:08X} ({first[2]}) - `{first[3]} {first[4]}`\n")
        md.append(f"- **Last Instruction**: #{last[0]} at 0x{last[1]:08X} ({last[2]}) - `{last[3]} {last[4]}`\n\n")
    
    md.append("## Mode Switches\n\n")
    if mode_switches:
        for i, (instr_num, from_mode, to_mode, pc) in enumerate(mode_switches):
            md.append(f"{i+1}. Instruction #{instr_num}: {from_mode} → {to_mode} at 0x{pc:08X}\n")
    else:
        md.append("No mode switches detected.\n")
    md.append("\n")
    
    md.append("## Functional Sections Executed\n\n")
    
    for section_name, section_data in sections.items():
        md.append(f"### {section_name}\n\n")
        md.append(f"**Address Range**: 0x{section_data['range'][0]:08X} - 0x{section_data['range'][1]:08X}\n\n")
        md.append(f"**Description**: {section_data['description']}\n\n")
        md.append(f"**Instructions Executed**: {section_data['instruction_count']:,}\n\n")
        md.append(f"**Execution Window**: Instructions #{section_data['first_execution']} to #{section_data['last_execution']}\n\n")
        
        # Show first few and last few instructions
        instrs = section_data['instructions']
        show_count = min(5, len(instrs))
        
        md.append("**First Instructions**:\n")
        md.append("```\n")
        for i, pc, mode, mnem, ops, regs in instrs[:show_count]:
            md.append(f"[{i:5d}] 0x{pc:08X} ({mode:5s}): {mnem:8s} {ops}\n")
        md.append("```\n\n")
        
        if len(instrs) > show_count * 2:
            md.append(f"... ({len(instrs) - show_count * 2} instructions omitted) ...\n\n")
        
        if len(instrs) > show_count:
            md.append("**Last Instructions**:\n")
            md.append("```\n")
            for i, pc, mode, mnem, ops, regs in instrs[-show_count:]:
                md.append(f"[{i:5d}] 0x{pc:08X} ({mode:5s}): {mnem:8s} {ops}\n")
            md.append("```\n\n")
        
        # Analyze specific sections
        if "Memory Clear Loop" in section_name:
            # Check if loop completed
            loop_instructions = [instr for instr in instrs if instr[3] in ['str', 'strh', 'strb']]
            md.append(f"**Memory Writes**: {len(loop_instructions)} store instructions\n\n")
            
            # Check if loop terminated normally
            last_instr = instrs[-1]
            if last_instr[3] in ['blt', 'bne', 'bge', 'b']:
                md.append(f"⚠️ **Warning**: Loop ended with branch instruction `{last_instr[3]}`, may not have completed\n\n")
            else:
                md.append(f"✅ **Status**: Loop appears to have completed (ended with `{last_instr[3]}`)\n\n")
        
        if "VBlank" in section_name or "HBlank" in section_name:
            # Count how many times we looped
            loop_backs = [instr for instr in instrs if instr[3] in ['b', 'bne', 'beq'] and '#0x' in instr[4]]
            md.append(f"**Loop Iterations**: {len(loop_backs)} backward branches\n\n")
    
    md.append("## Critical Analysis\n\n")
    
    # Check if we reached ROM entry point loader
    rom_loader_section = sections.get("ROM Entry Point Loader")
    if rom_loader_section:
        md.append("✅ **ROM Entry Point Loader (0xB4-0xBB)**: EXECUTED\n\n")
        md.append("This section should load the ROM entry point from the game header and jump to it.\n\n")
    else:
        md.append("❌ **ROM Entry Point Loader (0xB4-0xBB)**: NOT EXECUTED\n\n")
        md.append("⚠️ **CRITICAL**: The BIOS never reached the code that loads the ROM entry point!\n\n")
    
    # Check the last instruction
    if execution_log:
        last_instr = execution_log[-1]
        last_pc = last_instr[1]
        last_mnem = last_instr[3]
        last_ops = last_instr[4]
        
        md.append(f"### Last Instruction Analysis\n\n")
        md.append(f"**PC**: 0x{last_pc:08X}\n\n")
        md.append(f"**Instruction**: `{last_mnem} {last_ops}`\n\n")
        
        if last_mnem == 'bx':
            md.append("**Type**: Branch and Exchange (switching modes and/or jumping)\n\n")
            # Extract target register
            reg_match = re.search(r'r(\d+)', last_ops)
            if reg_match:
                reg_num = int(reg_match.group(1))
                reg_name = f"R{reg_num}"
                if reg_name in last_instr[5]:
                    target_value = last_instr[5][reg_name]
                    md.append(f"**Target Register**: {reg_name} = 0x{target_value:08X}\n\n")
                    
                    if target_value < 0x08000000:
                        md.append(f"⚠️ **WARNING**: Target address 0x{target_value:08X} is NOT in ROM region!\n\n")
                        md.append(f"ROM should be at 0x08000000 or higher. This target is in:\n")
                        if target_value < 0x4000:
                            md.append(f"- BIOS region (0x00000000-0x00003FFF)\n\n")
                        elif target_value < 0x02000000:
                            md.append(f"- Unused/Invalid region\n\n")
                        elif target_value < 0x03000000:
                            md.append(f"- EWRAM region (data, not code)\n\n")
                        elif target_value < 0x04000000:
                            md.append(f"- IWRAM region (data/stack, not code)\n\n")
                        else:
                            md.append(f"- I/O region (not executable)\n\n")
                        
                        md.append("**Root Cause**: The BIOS is trying to jump to an invalid address.\n\n")
                        md.append("This likely means:\n")
                        md.append("1. A pointer that should have been set by ROM code is uninitialized (0x00000000)\n")
                        md.append("2. The BIOS read from uninitialized memory\n")
                        md.append("3. The BIOS then read BIOS data (not a pointer) and tried to jump to it\n\n")
    
    md.append("## Recommendations\n\n")
    
    if not rom_loader_section:
        md.append("1. **CRITICAL**: The BIOS is not reaching address 0xB4 where it should load the ROM entry point\n")
        md.append("2. Investigate why BIOS execution is taking a different path\n")
        md.append("3. Check if BIOS is getting stuck in a loop or calling the wrong function\n\n")
    else:
        md.append("1. BIOS appears to reach the ROM loader section\n")
        md.append("2. However, instead of jumping to ROM, it's jumping to an invalid address\n")
        md.append("3. This suggests the ROM header is not being read correctly\n")
        md.append("4. Check if ROM is properly mapped at 0x08000000\n")
        md.append("5. Verify ROM header format (should have entry point at offset 0x00)\n\n")
    
    return ''.join(md)

def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_bios_flow.py <trace_file> [bios_disassembly.s]")
        sys.exit(1)
    
    trace_file = sys.argv[1]
    bios_file = sys.argv[2] if len(sys.argv) > 2 else '/Users/malcolm/gba/assets/bios_disassembly.s'
    
    print(f"Analyzing BIOS execution flow from: {trace_file}")
    print("="*80)
    
    # Load BIOS disassembly for context
    labels, comments = load_bios_disassembly(bios_file)
    print(f"Loaded {len(labels)} labels and {len(comments)} comments from BIOS disassembly")
    
    # Analyze execution flow
    execution_log, function_calls, mode_switches = analyze_execution_flow(trace_file)
    
    print(f"Analyzed {len(execution_log)} instructions")
    print(f"Found {len(function_calls)} function calls")
    print(f"Found {len(mode_switches)} mode switches")
    
    # Identify functional sections
    sections = identify_functional_sections(execution_log, labels, comments)
    
    print(f"Identified {len(sections)} functional sections")
    
    # Generate markdown report
    print("\n" + "="*80)
    print("GENERATING MARKDOWN REPORT")
    print("="*80)
    
    md_report = generate_flow_markdown(execution_log, function_calls, mode_switches, sections, labels, comments)
    
    output_file = '/Users/malcolm/gba/docs/BIOS_FLOW_ANALYSIS.md'
    with open(output_file, 'w') as f:
        f.write(md_report)
    
    print(f"\nMarkdown report saved to: {output_file}")

if __name__ == '__main__':
    main()
