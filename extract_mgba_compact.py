#!/usr/bin/env python3
"""
Extract compact format from mGBA memory trace for comparison
"""

import re

input_file = '/tmp/mgba_memory_trace.log'
output_file = '/tmp/mgba_first_500_compact.txt'

with open(input_file, 'r') as f_in, open(output_file, 'w') as f_out:
    instruction_num = 0
    pc = None
    registers = {}
    ie = if_reg = ime = None
    
    for line in f_in:
        if line.startswith('Instruction #'):
            instruction_num = int(line.split('#')[1].strip())
        elif line.startswith('PC:'):
            pc = line.split(':')[1].strip()
        elif '=' in line and 'r' in line.lower() and '0x' in line:
            # Parse register line like " r0=0x00000000   r1=0x00000000..."
            parts = line.strip().split()
            for part in parts:
                if '=' in part:
                    reg, val = part.split('=')
                    registers[reg.lower()] = val
        elif '[IE' in line and '] 0x04000200' in line:
            # Extract value after the = sign
            match = re.search(r'=\s*0x([0-9A-F]{4})', line)
            if match:
                ie = match.group(1)
        elif '[IF' in line and '] 0x04000202' in line:
            match = re.search(r'=\s*0x([0-9A-F]{4})', line)
            if match:
                if_reg = match.group(1)
        elif '[IME' in line and '] 0x04000208' in line:
            match = re.search(r'=\s*0x([0-9A-F]{8})', line)
            if match:
                ime = match.group(1)
                
                # Write compact line when we have all data
                if pc and registers and ie is not None:
                    f_out.write(f"[{instruction_num}][mGBA] PC={pc} | ")
                    f_out.write(f"R0={registers.get('r0', '').replace('0x','')} ")
                    f_out.write(f"R1={registers.get('r1', '').replace('0x','')} ")
                    f_out.write(f"R2={registers.get('r2', '').replace('0x','')} ")
                    f_out.write(f"R3={registers.get('r3', '').replace('0x','')} ")
                    f_out.write(f"R4={registers.get('r4', '').replace('0x','')} ")
                    f_out.write(f"R5={registers.get('r5', '').replace('0x','')} ")
                    f_out.write(f"R6={registers.get('r6', '').replace('0x','')} ")
                    f_out.write(f"R7={registers.get('r7', '').replace('0x','')} ")
                    f_out.write(f"R8={registers.get('r8', '').replace('0x','')} ")
                    f_out.write(f"R9={registers.get('r9', '').replace('0x','')} ")
                    f_out.write(f"R10={registers.get('r10', '').replace('0x','')} ")
                    f_out.write(f"R11={registers.get('r11', '').replace('0x','')} ")
                    f_out.write(f"R12={registers.get('r12', '').replace('0x','')} ")
                    f_out.write(f"SP={registers.get('sp', '').replace('0x','')} ")
                    f_out.write(f"LR={registers.get('lr', '').replace('0x','')} | ")
                    f_out.write(f"IE={ie} IF={if_reg} IME={ime}\n")
                    
                    if instruction_num >= 500:
                        break
                
                # Reset for next instruction
                registers = {}
                ie = if_reg = ime = None

print(f"Extracted compact format to {output_file}")
