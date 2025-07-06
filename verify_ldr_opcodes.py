#!/usr/bin/env python3

def encode_thumb_ldr_pc_relative(rd, offset):
    """Encode LDR Rd, [PC, #offset] instruction (Format 6)"""
    # Format: 01001 rd offset (bits 15-11: 01001)
    opcode = 0x4800  # Base for LDR PC-relative (01001 000 00000000)
    opcode |= (rd & 0x7) << 8        # Rd in bits 10:8
    opcode |= (offset & 0xFF)        # offset in bits 7:0 (offset is in words)
    return opcode

# Test cases for LDR PC-relative
print("LDR PC-relative test cases:")
print(f"LDR R0, [PC, #4]:   0x{encode_thumb_ldr_pc_relative(0, 1):04X}")  # offset=1 word = 4 bytes
print(f"LDR R1, [PC, #8]:   0x{encode_thumb_ldr_pc_relative(1, 2):04X}")  # offset=2 words = 8 bytes
print(f"LDR R2, [PC, #12]:  0x{encode_thumb_ldr_pc_relative(2, 3):04X}")  # offset=3 words = 12 bytes
print(f"LDR R3, [PC, #0]:   0x{encode_thumb_ldr_pc_relative(3, 0):04X}")  # offset=0 words = 0 bytes
print(f"LDR R4, [PC, #16]:  0x{encode_thumb_ldr_pc_relative(4, 4):04X}")  # offset=4 words = 16 bytes
print(f"LDR R5, [PC, #32]:  0x{encode_thumb_ldr_pc_relative(5, 8):04X}")  # offset=8 words = 32 bytes
print(f"LDR R6, [PC, #64]:  0x{encode_thumb_ldr_pc_relative(6, 16):04X}") # offset=16 words = 64 bytes
print(f"LDR R7, [PC, #1020]: 0x{encode_thumb_ldr_pc_relative(7, 255):04X}") # offset=255 words = 1020 bytes (max)

# Note: The offset is encoded as word offset (bits 7:0), so the actual byte offset is offset*4
# PC is aligned to word boundary and points 4 bytes ahead of current instruction
