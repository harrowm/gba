#!/usr/bin/env python3
"""
Test script to check if mGBA exposes cycle counter via GDB
"""

import socket

GDB_HOST = 'localhost'
GDB_PORT = 2345

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

def main():
    print("Testing mGBA GDB cycle counter access...")
    print("Make sure mGBA is running with: mgba -g assets/bios.bin")
    print()
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)
        sock.connect((GDB_HOST, GDB_PORT))
        print("✓ Connected to mGBA GDB server")
        
        # Try various methods to get cycle count
        
        # Method 1: Check if there's a 'cycles' or 'time' command
        print("\n1. Testing 'qRcmd' commands (monitor commands):")
        for cmd in ['cycles', 'time', 'stats', 'status']:
            print(f"   Trying: monitor {cmd}")
            # qRcmd sends a monitor command (hex-encoded)
            hex_cmd = cmd.encode().hex()
            resp = send_gdb_command(sock, f"qRcmd,{hex_cmd}")
            print(f"   Response: {resp[:100]}")
        
        # Method 2: Check if cycle counter is in memory
        print("\n2. Testing memory-mapped cycle counter:")
        # Try common addresses where emulators might expose cycle count
        addresses = [
            0x04000000,  # IO region
            0x03000000,  # IWRAM (sometimes used for debug)
        ]
        for addr in addresses:
            resp = send_gdb_command(sock, f"m{addr:x},4")
            print(f"   Address 0x{addr:08X}: {resp[:50]}")
        
        # Method 3: Single step and check PC to manually count
        print("\n3. Testing single-step approach:")
        print("   Getting initial PC...")
        resp = send_gdb_command(sock, "g")  # Read all registers
        print(f"   Register dump length: {len(resp)} chars")
        
        # PC is register 15 (offset 15*4*2 = 120 hex chars in)
        if len(resp) > 130:
            pc_hex = resp[122:130]  # 8 hex chars for 32-bit PC
            print(f"   PC hex: {pc_hex}")
            
        print("\n   Stepping 5 instructions and recording PC...")
        pcs = []
        for i in range(5):
            # Step one instruction
            resp = send_gdb_command(sock, "s")
            # Read PC
            resp = send_gdb_command(sock, "g")
            if len(resp) > 130:
                pc_hex = resp[122:130]
                pc = int.from_bytes(bytes.fromhex(pc_hex), 'little')
                pcs.append(pc)
                print(f"   Instruction {i+1}: PC=0x{pc:08X}")
        
        print("\n✓ If we can single-step reliably, we can manually count cycles per instruction")
        print("  by looking up each instruction's expected cycle cost")
        
        sock.close()
        
    except Exception as e:
        print(f"\n✗ Error: {e}")
        print("\nTo use this script:")
        print("1. Start mGBA with GDB server: mgba -g assets/bios.bin")
        print("2. Run this script: python3 test_mgba_cycles.py")

if __name__ == '__main__':
    main()
