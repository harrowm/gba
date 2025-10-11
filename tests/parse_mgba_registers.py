#!/usr/bin/env python3
"""
Parse mGBA register values from the trace to see if they match ours
"""

def parse_mgba_registers(reg_string):
    """Parse mGBA GDB register format"""
    # Format: Registers: +$000000000000000000000000...
    if 'Registers: ' in reg_string:
        reg_string = reg_string.split('Registers: ')[1]
    
    # Strip the +$ prefix
    if reg_string.startswith('+$'):
        reg_string = reg_string[2:]
    elif reg_string.startswith('$'):
        reg_string = reg_string[1:]
    
    # Clean up the string - remove any non-hex characters except the first ones
    reg_string = reg_string.replace('...', '').strip()
    
    # Each register is 8 hex chars (4 bytes), little-endian
    registers = {}
    for i in range(16):  # r0-r15
        offset = i * 8
        if offset + 8 <= len(reg_string):
            hex_val = reg_string[offset:offset + 8]
            try:
                # Convert from little-endian hex string
                value = int.from_bytes(bytes.fromhex(hex_val), 'little')
                registers[f'r{i}'] = value
            except ValueError:
                break
    
    # CPSR is after r15
    offset = 16 * 8
    if offset + 8 <= len(reg_string):
        hex_val = reg_string[offset:offset + 8]
        try:
            cpsr = int.from_bytes(bytes.fromhex(hex_val), 'little')
            registers['cpsr'] = cpsr
        except ValueError:
            pass
    
    return registers

def decode_cpsr(cpsr):
    """Decode CPSR flags"""
    N = (cpsr >> 31) & 1
    Z = (cpsr >> 30) & 1
    C = (cpsr >> 29) & 1
    V = (cpsr >> 28) & 1
    return f"N={N} Z={Z} C={C} V={V}"

def main():
    # Read mGBA trace around instruction #38078-38080
    with open('/tmp/mgba_memory_trace.log', 'r') as f:
        lines = f.readlines()
    
    print("mGBA Register Analysis at Divergence")
    print("=" * 70)
    
    # Find instructions 38078-38080
    for i, line in enumerate(lines):
        if line.strip() == 'Instruction #38078':
            print(f"\n{line.strip()}")
            # Next line is separator, then PC
            pc_line = lines[i+2].strip()
            print(f"{pc_line}")
            # Next line has registers
            reg_line = lines[i+3].strip()
            print(f"Registers: {reg_line[:80]}...")
            
            regs = parse_mgba_registers(reg_line)
            print(f"\nParsed registers:")
            print(f"  r1  = 0x{regs.get('r1', 0):08X}")
            print(f"  r12 = 0x{regs.get('r12', 0):08X}")
            if 'cpsr' in regs:
                print(f"  cpsr = 0x{regs['cpsr']:08X} [{decode_cpsr(regs['cpsr'])}]")
        
        elif line.strip() == 'Instruction #38079':
            print(f"\n{line.strip()}")
            pc_line = lines[i+2].strip()
            print(f"{pc_line}")
            reg_line = lines[i+3].strip()
            print(f"Registers: {reg_line[:80]}...")
            
            regs = parse_mgba_registers(reg_line)
            print(f"\nParsed registers:")
            print(f"  r1  = 0x{regs.get('r1', 0):08X}")
            print(f"  r12 = 0x{regs.get('r12', 0):08X}")
            if 'cpsr' in regs:
                print(f"  cpsr = 0x{regs['cpsr']:08X} [{decode_cpsr(regs['cpsr'])}]")
        
        elif line.strip() == 'Instruction #38080':
            print(f"\n{line.strip()}")
            pc_line = lines[i+2].strip()
            print(f"{pc_line}")
            reg_line = lines[i+3].strip()
            print(f"Registers: {reg_line[:80]}...")
            
            regs = parse_mgba_registers(reg_line)
            print(f"\nParsed registers:")
            if 'cpsr' in regs:
                print(f"  cpsr = 0x{regs['cpsr']:08X} [{decode_cpsr(regs['cpsr'])}]")
            
            print("\n" + "=" * 70)
            print("COMPARISON:")
            print("=" * 70)
            print("\nOur emulator at #38079 (after CMP, before BGE):")
            print("  CPSR: 0x8000007F [N=1 Z=0 C=0 V=0]")
            print("  BGE condition (N==V): 1==0 = FALSE → doesn't branch")
            print("\nmGBA at #38079 (after CMP, before BGE):")
            if 'cpsr' in regs:
                cpsr = regs['cpsr']
                N = (cpsr >> 31) & 1
                V = (cpsr >> 28) & 1
                print(f"  CPSR: 0x{cpsr:08X} [{decode_cpsr(cpsr)}]")
                branch_taken = "branches" if N==V else "doesn't branch"
                print(f"  BGE condition (N==V): {N}=={V} = {N==V} → {branch_taken}")
            break

if __name__ == '__main__':
    main()
