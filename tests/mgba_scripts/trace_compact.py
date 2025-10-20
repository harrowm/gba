#!/usr/bin/env python3
"""
mGBA Instruction Tracer - Compact Format
Outputs in same format as our GBA emulator for easy comparison
Format: PC:XXXXXXXX R00:XXXXXXXX ... R15:XXXXXXXX CPSR:XXXXXXXX | IE:XXXX IF:XXXX IME:XXXXXXXX
"""

import socket
import sys

GDB_HOST = 'localhost'
GDB_PORT = 2345
MAX_INSTRUCTIONS = 200000

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
    expected_len = size * 2
    if len(resp) >= expected_len:
        try:
            bytes_data = bytes.fromhex(resp[:expected_len])
            value = int.from_bytes(bytes_data, byteorder='little')
            return value
        except:
            return None
    return None

def main():
    print("Connecting to mGBA GDB server...", file=sys.stderr)
    print(f"Make sure mGBA is running with: -g flag", file=sys.stderr)
    
    try:
        # Connect to GDB server
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2.0)
        sock.connect((GDB_HOST, GDB_PORT))
        print("Connected to mGBA GDB server!", file=sys.stderr)
        print(f"Tracing {MAX_INSTRUCTIONS} instructions in compact format...", file=sys.stderr)
        
        for i in range(MAX_INSTRUCTIONS):
            # Get registers
            reg_resp = send_gdb_command(sock, "g")
            
            # Parse register data
            reg_data = reg_resp.strip()
            if reg_data.startswith('+$'):
                reg_data = reg_data[2:]
            elif reg_data.startswith('$'):
                reg_data = reg_data[1:]
            if '#' in reg_data:
                reg_data = reg_data[:reg_data.index('#')]
            
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
            
            # Read interrupt registers
            ie = read_memory(sock, 0x04000200, 2) or 0
            if_val = read_memory(sock, 0x04000202, 2) or 0
            ime = read_memory(sock, 0x04000208, 4) or 0
            
            # Output in compact format matching our emulator
            print(f"PC:{registers[15]:08X} " +
                  f"R00:{registers[0]:08X} R01:{registers[1]:08X} R02:{registers[2]:08X} R03:{registers[3]:08X} " +
                  f"R04:{registers[4]:08X} R05:{registers[5]:08X} R06:{registers[6]:08X} R07:{registers[7]:08X} " +
                  f"R08:{registers[8]:08X} R09:{registers[9]:08X} R10:{registers[10]:08X} R11:{registers[11]:08X} " +
                  f"R12:{registers[12]:08X} R13:{registers[13]:08X} R14:{registers[14]:08X} R15:{registers[15]:08X} " +
                  f"CPSR:{registers[16]:08X} | IE:{ie:04X} IF:{if_val:04X} IME:{ime:08X}")
            
            # Step one instruction
            send_gdb_command(sock, "s")
            
            if (i + 1) % 1000 == 0:
                print(f"Traced {i+1} instructions...", file=sys.stderr)
        
        print(f"\nTrace complete! {MAX_INSTRUCTIONS} instructions traced", file=sys.stderr)
        
    except ConnectionRefusedError:
        print("ERROR: Could not connect to mGBA. Make sure it's running with -g flag", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc(file=sys.stderr)
        sys.exit(1)
    finally:
        try:
            sock.close()
        except:
            pass

if __name__ == '__main__':
    main()
