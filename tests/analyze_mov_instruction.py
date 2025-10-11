#!/usr/bin/env python3
"""
Analyze our Thumb MOV instruction execution at the divergence point
"""

from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB

def main():
    print("Analyzing Thumb MOV instruction execution")
    print("=" * 70)
    
    # The problematic instruction at 0x804
    insn_bytes = bytes.fromhex('8c46')  # 0x468C in little-endian
    insn_val = int.from_bytes(insn_bytes, 'little')
    
    print(f"\nInstruction at 0x804: 0x{insn_val:04X}")
    print(f"Binary: {insn_val:016b}")
    print()
    
    # Decode with Capstone
    md = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
    for i in md.disasm(insn_bytes, 0x804):
        print(f"Disassembly: {i.mnemonic} {i.op_str}")
    
    print("\nManual decode (Format 5 - Hi register operations):")
    print("-" * 70)
    
    op = (insn_val >> 8) & 0x3
    h1 = (insn_val >> 7) & 0x1
    h2 = (insn_val >> 6) & 0x1
    rs_field = (insn_val >> 3) & 0x7
    rd_field = insn_val & 0x7
    
    print(f"Op:       {op:02b} = {op} (10 = MOV)")
    print(f"H1:       {h1} (destination is high register)")
    print(f"H2:       {h2} (source is NOT high register)")
    print(f"Rs field: {rs_field:03b} = {rs_field}")
    print(f"Rd field: {rd_field:03b} = {rd_field}")
    
    rs = rs_field + (8 if h2 else 0)
    rd = rd_field + (8 if h1 else 0)
    
    print(f"\nActual registers:")
    print(f"  Source (Rs):      r{rs} (r1)")
    print(f"  Destination (Rd): r{rd} (r12/ip)")
    
    print("\n" + "=" * 70)
    print("EXPECTED BEHAVIOR:")
    print("=" * 70)
    print("\nBefore instruction #38070 (PC=0x804):")
    print("  r1  = 0x00000200  (from previous lsls r1, r1, #8)")
    print("  r12 = 0x0300FCA0  (old value)")
    print()
    print("After 'MOV r12, r1' at 0x804:")
    print("  r1  = 0x00000200  (unchanged)")
    print("  r12 = 0x00000200  (copied from r1)")
    print()
    print("But mGBA shows:")
    print("  r1  = 0x00000200  ✓ correct")
    print("  r12 = 0x00000000  ✗ WRONG! Should be 0x200")
    print()
    print("Wait... mGBA is the reference. So:")
    print("  mGBA r12 = 0x00000000 is CORRECT")
    print("  Our r12  = 0x00000200 is WRONG")
    print()
    print("This means OUR MOV instruction is incorrectly SETTING r12,")
    print("when it should leave it at 0 or set it to something else.")
    print()
    print("=" * 70)
    print("HYPOTHESIS:")
    print("=" * 70)
    print("\nPossibility 1: Our Thumb MOV with high registers has a bug")
    print("  - Maybe we're writing to the wrong register?")
    print("  - Maybe the H1/H2 bit interpretation is wrong?")
    print()
    print("Possibility 2: The GDB register dump order is different")
    print("  - Maybe r12 is not at offset 12*8 in the GDB response")
    print("  - Let me check the GDB 'g' packet format...")
    print()
    print("Possibility 3: Our register state diverged earlier")
    print("  - Need to check when r12 first became 0x0300FCA0 in our emulator")

if __name__ == '__main__':
    main()
