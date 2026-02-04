#!/usr/bin/env python3
import struct

# Read BIOS
with open('assets/bios.bin', 'rb') as f:
    bios = f.read()

# Simple THUMB decoder
def decode_thumb(instr):
    # BX/BLX register
    if (instr >> 8) == 0x47:
        rm = (instr >> 3) & 0xF
        if (instr >> 7) & 1:
            return f'BLX r{rm}'
        return f'BX r{rm}'
    
    # MOV high register
    if (instr >> 10) == 0b010001:
        op = (instr >> 8) & 3
        rd = (instr & 7) | ((instr >> 4) & 8)
        rs = (instr >> 3) & 0xF
        if op == 0: return f'ADD r{rd}, r{rs}'
        if op == 1: return f'CMP r{rd}, r{rs}'
        if op == 2: return f'MOV r{rd}, r{rs}'
    
    # SUB imm
    if (instr >> 11) == 0b00111:
        rd = (instr >> 8) & 7
        imm = instr & 0xFF
        return f'SUBS r{rd}, #0x{imm:02X}'
    
    # LDRB imm5
    if (instr >> 11) == 0b01110:
        rb = (instr >> 3) & 7
        rd = instr & 7
        imm5 = (instr >> 6) & 0x1F
        return f'LDRB r{rd}, [r{rb}, #{imm5}]'
    
    # STRB imm5
    if (instr >> 11) == 0b01111:
        rb = (instr >> 3) & 7
        rd = instr & 7
        imm5 = (instr >> 6) & 0x1F
        return f'STRB r{rd}, [r{rb}, #{imm5}]'
    
    # ALU ops
    if (instr >> 10) == 0b010000:
        rd = instr & 7
        rs = (instr >> 3) & 7
        op = (instr >> 6) & 0xF
        ops = ['AND','EOR','LSL','LSR','ASR','ADC','SBC','ROR','TST','NEG','CMP','CMN','ORR','MUL','BIC','MVN']
        return f'{ops[op]} r{rd}, r{rs}'
    
    # MOV imm
    if (instr >> 11) == 0b00100:
        rd = (instr >> 8) & 7
        imm = instr & 0xFF
        return f'MOVS r{rd}, #{imm}'
    
    # CMP imm
    if (instr >> 11) == 0b00101:
        rd = (instr >> 8) & 7
        imm = instr & 0xFF
        return f'CMP r{rd}, #{imm}'
    
    # LDR PC-relative
    if (instr >> 11) == 0b01001:
        rd = (instr >> 8) & 7
        imm = (instr & 0xFF) * 4
        return f'LDR r{rd}, [PC, #{imm}]'
    
    # PUSH/POP
    if (instr >> 12) == 0b1011:
        L = (instr >> 11) & 1
        R = (instr >> 8) & 1
        rlist = instr & 0xFF
        regs = [f'r{i}' for i in range(8) if rlist & (1<<i)]
        if R:
            regs.append('LR' if not L else 'PC')
        return ('POP' if L else 'PUSH') + ' {' + ','.join(regs) + '}'
    
    # Conditional branch
    if (instr >> 12) == 0b1101:
        cond = (instr >> 8) & 0xF
        conds = ['EQ','NE','CS','CC','MI','PL','VS','VC','HI','LS','GE','LT','GT','LE','AL','NV']
        offset = instr & 0xFF
        if offset & 0x80: offset -= 256
        return f'B{conds[cond]} (offset {offset*2})'
    
    # BL/BLX
    if (instr >> 11) == 0b11110:
        return 'BL prefix'
    if (instr >> 11) == 0b11111:
        return 'BL suffix'
        
    return f'??? 0x{instr:04X}'

# Function at 0x26C4 (called by BL at 0x27BC)
print('Function at 0x26A0-0x26C2 (called by BL at 0x27BC):')
for addr in range(0x26A0, 0x26D0, 2):
    instr = struct.unpack('<H', bios[addr:addr+2])[0]
    asm = decode_thumb(instr)
    print(f'  0x{addr:04X}: 0x{instr:04X}  {asm}')
