#!/usr/bin/env python3
"""
Analyze CPSR flags at the divergence point
"""

def decode_cpsr(cpsr):
    """Decode CPSR value into flags"""
    N = (cpsr >> 31) & 1
    Z = (cpsr >> 30) & 1
    C = (cpsr >> 29) & 1
    V = (cpsr >> 28) & 1
    I = (cpsr >> 7) & 1
    F = (cpsr >> 6) & 1
    T = (cpsr >> 5) & 1
    mode = cpsr & 0x1F
    
    return {
        'N': N, 'Z': Z, 'C': C, 'V': V,
        'I': I, 'F': F, 'T': T, 'mode': mode,
        'raw': cpsr
    }

def check_condition(condition, flags):
    """Check if ARM condition is met"""
    N, Z, C, V = flags['N'], flags['Z'], flags['C'], flags['V']
    
    conditions = {
        'eq': Z == 1,                    # Equal
        'ne': Z == 0,                    # Not equal
        'cs': C == 1,                    # Carry set
        'cc': C == 0,                    # Carry clear
        'mi': N == 1,                    # Minus/negative
        'pl': N == 0,                    # Plus/positive or zero
        'vs': V == 1,                    # Overflow
        'vc': V == 0,                    # No overflow
        'hi': C == 1 and Z == 0,        # Unsigned higher
        'ls': C == 0 or Z == 1,         # Unsigned lower or same
        'ge': N == V,                    # Signed greater than or equal
        'lt': N != V,                    # Signed less than
        'gt': Z == 0 and N == V,        # Signed greater than
        'le': Z == 1 or N != V,         # Signed less than or equal
        'al': True                       # Always
    }
    
    return conditions.get(condition, False)

def main():
    print("CPSR Analysis at Divergence Point")
    print("=" * 70)
    
    # At instruction #38079, PC=0x00000816: bge #0x82c
    print("\nInstruction #38079: PC=0x00000816, bge #0x82c")
    print("-" * 70)
    
    # Our emulator CPSR
    our_cpsr = 0x8000007F
    our_flags = decode_cpsr(our_cpsr)
    print(f"\nOur emulator:")
    print(f"  CPSR: 0x{our_cpsr:08X}")
    print(f"  Flags: N={our_flags['N']} Z={our_flags['Z']} C={our_flags['C']} V={our_flags['V']}")
    print(f"  bge condition (N == V): {our_flags['N']} == {our_flags['V']} = {our_flags['N'] == our_flags['V']}")
    print(f"  Branch taken: {check_condition('ge', our_flags)}")
    print(f"  Next PC: 0x00000818 (branch NOT taken)")
    
    # Need to check mGBA's CPSR - it should show different flags
    print(f"\nmGBA:")
    print(f"  Next PC: 0x0000082C (branch WAS taken)")
    print(f"  This means mGBA's condition was TRUE (N == V)")
    print(f"  Expected: Either N=0,V=0 OR N=1,V=1")
    
    print("\n" + "=" * 70)
    print("ANALYSIS:")
    print("=" * 70)
    
    print("\nPrevious instruction at 0x814: cmp r1, ip")
    print("  r1  = 0x00000000")
    print("  ip (r12) = 0x00000200")
    print("  Operation: r1 - ip = 0x00000000 - 0x00000200 = -0x200")
    print()
    print("Expected CMP results:")
    print("  N=1 (result is negative)")
    print("  Z=0 (result is not zero)")
    print("  C=0 (borrow occurred: 0 < 0x200)")
    print("  V=0 (no signed overflow)")
    print()
    print("With N=1, V=0: bge condition (N==V) is FALSE")
    print("  → Should fall through to 0x818 (our emulator is CORRECT)")
    print()
    print("But mGBA took the branch to 0x82C!")
    print("  → This means mGBA had different flags after the CMP")
    print("  → Likely mGBA had N=0,V=0 or N=1,V=1")
    print()
    print("HYPOTHESIS: Our CMP instruction flag setting is correct,")
    print("but mGBA's flags may have been affected by something earlier.")
    print()
    print("Let's check instruction #38078 (the CMP itself):")

if __name__ == '__main__':
    main()
