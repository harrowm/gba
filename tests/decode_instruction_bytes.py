#!/usr/bin/env python3
"""
Decode the actual instruction bytes at the divergence point using Capstone
"""

from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB
from capstone.arm import *

def decode_thumb_instruction(bios_path, addr):
    """Decode a single Thumb instruction and show detailed info"""
    # Read BIOS
    with open(bios_path, 'rb') as f:
        bios = f.read()
    
    # Create Capstone disassembler for Thumb mode with detail
    md = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
    md.detail = True
    
    # Extract instruction bytes (Thumb instructions are 2 or 4 bytes)
    code = bios[addr:addr + 4]
    
    print(f"\nInstruction at 0x{addr:08X}:")
    print("=" * 70)
    print(f"Raw bytes: {' '.join(f'{b:02x}' for b in code[:2])}")
    
    for insn in md.disasm(code, addr):
        print(f"\nDisassembly: {insn.mnemonic} {insn.op_str}")
        print(f"Instruction ID: {insn.id}")
        print(f"Size: {insn.size} bytes")
        print(f"Instruction bytes: {insn.bytes.hex()}")
        
        # Show operand details
        if len(insn.operands) > 0:
            print(f"\nOperands ({len(insn.operands)}):")
            for i, op in enumerate(insn.operands):
                print(f"  {i}: ", end="")
                if op.type == ARM_OP_REG:
                    print(f"Register: {insn.reg_name(op.reg)}")
                elif op.type == ARM_OP_IMM:
                    print(f"Immediate: 0x{op.imm:x}")
                elif op.type == ARM_OP_MEM:
                    print(f"Memory")
        
        # Show condition code
        print(f"\nCondition code: {insn.cc}")
        
        # For CMP, show if it's a high register format
        if insn.mnemonic == "cmp":
            regs = [op.reg for op in insn.operands if op.type == ARM_OP_REG]
            if regs:
                reg_names = [insn.reg_name(r) for r in regs]
                print(f"Comparing: {' vs '.join(reg_names)}")
                
                # Check if any register is >= r8 (high register)
                high_regs = [r for r in regs if r >= ARM_REG_R8 and r <= ARM_REG_R15]
                if high_regs:
                    print(f"HIGH REGISTER CMP (Format 5)")
                    print(f"High registers involved: {[insn.reg_name(r) for r in high_regs]}")
                else:
                    print(f"Normal CMP")
        
        break  # Only decode first instruction

def main():
    bios_path = "/Users/malcolm/gba/assets/bios.bin"
    
    print("Instruction Decoding at Divergence Point")
    print("=" * 70)
    
    # Decode the CMP instruction at 0x814
    decode_thumb_instruction(bios_path, 0x814)
    
    # Decode the BGE instruction at 0x816
    decode_thumb_instruction(bios_path, 0x816)
    
    print("\n" + "=" * 70)
    print("ANALYSIS:")
    print("=" * 70)
    print("\nThe CMP at 0x814 compares r1 (=0x00000000) with r12/ip (=0x00000200)")
    print("This is a HIGH REGISTER CMP because r12 >= r8")
    print("\nExpected flags after CMP:")
    print("  Result = 0x00000000 - 0x00000200 = 0xFFFFFE00 (in 32-bit)")
    print("  N = 1 (bit 31 of result is 1)")
    print("  Z = 0 (result is not zero)")
    print("  C = 0 (borrow occurred: 0 < 0x200, so NOT(borrow) = 0)")
    print("  V = 0 (no signed overflow: both inputs positive, result negative is expected)")
    print("\nThen BGE at 0x816 checks: N == V")
    print("  With N=1, V=0: condition is FALSE")
    print("  → Should NOT branch (fall through to 0x818)")
    print("\nOur emulator behavior: Correct (doesn't branch)")
    print("mGBA behavior: Takes branch to 0x82C")
    print("\nThis suggests mGBA has different CPSR flags after the CMP!")

if __name__ == '__main__':
    main()
