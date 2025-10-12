#!/usr/bin/env python3
"""
mGBA Instruction Tracer via GDB Remote Protocol
Connects to mGBA's GDB server to trace instructions
"""

import socket
import time
import sys

GDB_HOST = 'localhost'
GDB_PORT = 2345
OUTPUT_FILE = '/tmp/mgba_gdb_trace.log'
MAX_INSTRUCTIONS = 1000

def send_gdb_command(sock, cmd):
    """Send a GDB command and return response"""
    # Calculate checksum
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
    print("Starting mGBA with GDB server...")
    print("Please start mGBA manually with:")
    print(f"  /Applications/mGBA.app/Contents/MacOS/mGBA \\")
    print(f"    /Users/malcolm/gba/assets/roms/arm.gba \\")
    print(f"    -b /Users/malcolm/gba/assets/bios.bin \\")
    print(f"    -g")
    print("")
    print(f"Waiting for GDB connection on {GDB_HOST}:{GDB_PORT}...")
    
    # Wait for user to start mGBA
    input("Press Enter once mGBA is running...")
    
    try:
        # Connect to GDB server
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((GDB_HOST, GDB_PORT))
        print("Connected to mGBA GDB server!")
        
        # Open output file
        with open(OUTPUT_FILE, 'w') as f:
            f.write("mGBA Instruction Trace via GDB\n")
            f.write("=" * 60 + "\n\n")
            
            for i in range(MAX_INSTRUCTIONS):
                # Get registers
                resp = send_gdb_command(sock, "g")
                f.write(f"Instruction #{i+1}\n")
                f.write(f"Registers: {resp}\n")
                
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
        sys.exit(1)
    finally:
        sock.close()

if __name__ == '__main__':
    main()
