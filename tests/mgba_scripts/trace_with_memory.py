#!/usr/bin/env python3
"""
mGBA Instruction Tracer via GDB Remote Protocol with Memory Reads
Captures both CPU state and key memory locations at each instruction
"""

import socket
import sys

GDB_HOST = 'localhost'
GDB_PORT = 2345
OUTPUT_FILE = '/tmp/mgba_memory_trace.log'
MAX_INSTRUCTIONS = 10000000

# Key memory locations to monitor (address, name, size in bytes)
MEMORY_LOCATIONS = {
    0x04000000: ("DISPCNT",      4),  # 32-bit register
    0x04000004: ("DISPSTAT",     2),  # 16-bit register
    0x04000006: ("VCOUNT",       2),  # 16-bit register
    0x04000100: ("TM0CNT_L",     2),  # Timer 0 Counter (16-bit)
    0x04000102: ("TM0CNT_H",     2),  # Timer 0 Control (16-bit)
    0x04000104: ("TM1CNT_L",     2),  # Timer 1 Counter (16-bit)
    0x04000106: ("TM1CNT_H",     2),  # Timer 1 Control (16-bit)
    0x040000BA: ("DMA0CNT_H",    2),  # DMA 0 Control (16-bit)
    0x040000C6: ("DMA1CNT_H",    2),  # DMA 1 Control (16-bit)
    0x040000D2: ("DMA2CNT_H",    2),  # DMA 2 Control (16-bit)
    0x040000DE: ("DMA3CNT_H",    2),  # DMA 3 Control (16-bit)
    0x04000200: ("IE",           2),  # Interrupt Enable (16-bit)
    0x04000202: ("IF",           2),  # Interrupt Flag (16-bit)
    0x04000208: ("IME",          4),  # Interrupt Master Enable (32-bit)
    0x04000300: ("POSTFLG",      1),  # Post Boot Flag (8-bit)
    0x03007FFC: ("IRQ_HANDLER",  4),  # 32-bit pointer
    0x03007FF8: ("IRQ_SP-4",     4)   # 32-bit value
}

def send_gdb_command(sock, cmd):
    """Send a GDB command and return response"""
    checksum = sum(ord(c) for c in cmd) % 256
    packet = f"${cmd}#{checksum:02x}"
    
    sock.sendall(packet.encode())
    
    # Read response
    response = b''
    while True:
        data = sock.recv(4096)
        if not data:
            break
        response += data
        if b'#' in response:
            break
    
    return response.decode('latin-1')

def read_memory(sock, address, size):
    """Read memory from specified address with given size (1, 2, or 4 bytes)"""
    # GDB memory read: m<addr>,<length>
    # Returns hex bytes in target byte order
    cmd = f"m{address:x},{size}"
    resp = send_gdb_command(sock, cmd)
    
    # Remove packet framing
    resp = resp.strip()
    if resp.startswith('+$'):
        resp = resp[2:]
    elif resp.startswith('$'):
        resp = resp[1:]
    
    # Find data before checksum
    if '#' in resp:
        resp = resp[:resp.index('#')]
    
    # Parse hex bytes (little-endian)
    expected_len = size * 2  # 2 hex chars per byte
    if len(resp) >= expected_len:
        try:
            # GDB returns bytes as hex string: "12345678" means bytes [0x12, 0x34, 0x56, 0x78]
            # For little-endian ARM: reverse byte order
            bytes_data = bytes.fromhex(resp[:expected_len])
            value = int.from_bytes(bytes_data, byteorder='little')
            return value
        except:
            return None
    return None

def main():
    print("Connecting to mGBA GDB server...")
    print(f"Make sure mGBA is running with: -g flag")
    
    try:
        # Connect to GDB server
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2.0)
        sock.connect((GDB_HOST, GDB_PORT))
        print("Connected to mGBA GDB server!")
        
        # Open output file
        with open(OUTPUT_FILE, 'w') as f:
            f.write("mGBA Memory Trace via GDB\n")
            f.write("=" * 60 + "\n")
            f.write("Monitoring memory locations:\n")
            for addr, name in MEMORY_LOCATIONS.items():
                f.write(f"  0x{addr:08X}: {name}\n")
            f.write("=" * 60 + "\n\n")
            
            for i in range(MAX_INSTRUCTIONS):
                # Get registers
                reg_resp = send_gdb_command(sock, "g")
                
                # Parse PC from registers (15th register = 60 bytes in)
                # Remove packet framing
                reg_data = reg_resp.strip()
                if reg_data.startswith('+$'):
                    reg_data = reg_data[2:]
                elif reg_data.startswith('$'):
                    reg_data = reg_data[1:]
                if '#' in reg_data:
                    reg_data = reg_data[:reg_data.index('#')]
                
                # Extract PC (register 15 = offset 60*2 = 120 hex chars in)
                if len(reg_data) >= 128:
                    pc_hex = reg_data[120:128]
                    pc = int.from_bytes(bytes.fromhex(pc_hex), byteorder='little')
                else:
                    pc = 0
                
                # Parse all registers (16 general + CPSR)
                registers = []
                for j in range(17):  # r0-r15 + cpsr
                    offset = j * 8
                    if offset + 8 <= len(reg_data):
                        reg_hex = reg_data[offset:offset+8]
                        reg_val = int.from_bytes(bytes.fromhex(reg_hex), byteorder='little')
                        registers.append(reg_val)
                    else:
                        registers.append(0)
                
                # Write instruction header
                f.write(f"\n{'='*70}\n")
                f.write(f"Instruction #{i+1}\n")
                f.write(f"{'='*70}\n")
                f.write(f"PC: 0x{pc:08X}\n")
                
                # Write registers in same format as our emulator
                f.write(f" r0=0x{registers[0]:08X}   r1=0x{registers[1]:08X}   r2=0x{registers[2]:08X}   r3=0x{registers[3]:08X}\n")
                f.write(f" r4=0x{registers[4]:08X}   r5=0x{registers[5]:08X}   r6=0x{registers[6]:08X}   r7=0x{registers[7]:08X}\n")
                f.write(f" r8=0x{registers[8]:08X}   r9=0x{registers[9]:08X}  r10=0x{registers[10]:08X}  r11=0x{registers[11]:08X}\n")
                f.write(f"r12=0x{registers[12]:08X}   sp=0x{registers[13]:08X}   lr=0x{registers[14]:08X}\n")
                
                # Decode CPSR
                cpsr = registers[16]
                N = (cpsr >> 31) & 1
                Z = (cpsr >> 30) & 1
                C = (cpsr >> 29) & 1
                V = (cpsr >> 28) & 1
                I = (cpsr >> 7) & 1
                F = (cpsr >> 6) & 1
                T = (cpsr >> 5) & 1
                mode = cpsr & 0x1F
                
                mode_names = {
                    0x10: "User", 0x11: "FIQ", 0x12: "IRQ", 0x13: "Supervisor",
                    0x17: "Abort", 0x1B: "Undefined", 0x1F: "System"
                }
                mode_name = mode_names.get(mode, "Unknown")
                
                f.write(f"cpsr=0x{cpsr:08X} [N={N} Z={Z} C={C} V={V} I={I} F={F} T={T}] Mode: {mode_name}\n")
                
                # Read key memory locations
                f.write(f"\nMemory:\n")
                for addr, (name, size) in sorted(MEMORY_LOCATIONS.items()):
                    value = read_memory(sock, addr, size)
                    if value is not None:
                        if size == 1:
                            f.write(f"  [{name:12s}] 0x{addr:08X} = 0x{value:02X}\n")
                        elif size == 2:
                            f.write(f"  [{name:12s}] 0x{addr:08X} = 0x{value:04X}\n")
                        else:
                            f.write(f"  [{name:12s}] 0x{addr:08X} = 0x{value:08X}\n")
                    else:
                        f.write(f"  [{name:12s}] 0x{addr:08X} = (read failed)\n")
                
                # Step one instruction
                send_gdb_command(sock, "s")
                
                if (i + 1) % 100 == 0:
                    print(f"Traced {i+1} instructions...")
                    f.flush()
        
        print(f"\nTrace complete! {MAX_INSTRUCTIONS} instructions traced to {OUTPUT_FILE}")
        
    except ConnectionRefusedError:
        print("ERROR: Could not connect to mGBA. Make sure it's running with -g flag")
        sys.exit(1)
    except Exception as e:
        print(f"ERROR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
    finally:
        try:
            sock.close()
        except:
            pass

if __name__ == '__main__':
    main()
