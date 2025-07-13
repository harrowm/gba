#!/usr/bin/env python3
import sys

def decode_arm_add(instr):
    # instr: 32-bit integer
    cond = (instr >> 28) & 0xF
    opcode = (instr >> 21) & 0xF
    rn = (instr >> 16) & 0xF
    rd = (instr >> 12) & 0xF
    operand2 = instr & 0xFFF
    s = (instr >> 20) & 1
    i = (instr >> 25) & 1
    # Only decode if opcode == 0b0100 (ADD)
    if opcode != 0b0100:
        print("Not an ADD instruction (opcode=0x{:X})".format(opcode))
        return
    print(f"cond: {cond:04b} (0x{cond:X})")
    print(f"I: {i} (immediate: {'yes' if i else 'no'})")
    print(f"S: {s} (set flags: {'yes' if s else 'no'})")
    print(f"Rn: R{rn}")
    print(f"Rd: R{rd}")
    print(f"Operand2: 0x{operand2:X}")
    if not i:
        rm = operand2 & 0xF
        print(f"Register operand: Rm=R{rm}")
    else:
        imm = operand2
        print(f"Immediate operand: 0x{imm:X}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: verify_add_opcodes.py <hex instruction>")
        sys.exit(1)
    instr = int(sys.argv[1], 16)
    print(f"Decoding: 0x{instr:08X}")
    decode_arm_add(instr)
