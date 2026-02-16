#!/usr/bin/env python3
import sys

data = open('/Users/malcolm/gba/assets/bios.bin', 'rb').read()
print(f"BIOS size: {len(data)} bytes (0x{len(data):04X})")

print("\n=== ARM SWI instructions (condition AL, 0xEF) ===")
for i in range(0, len(data)-3, 4):
    if data[i+3] == 0xEF:
        imm = data[i] | (data[i+1] << 8) | (data[i+2] << 16)
        print(f"  Offset 0x{i:04X}: ARM SWI #{imm} (0x{imm:06X})")

names = {
    0: "SoftReset", 1: "RegisterRamReset", 2: "Halt", 3: "Stop",
    4: "IntrWait", 5: "VBlankIntrWait", 6: "Div", 7: "DivArm",
    8: "Sqrt", 9: "ArcTan", 0xA: "ArcTan2", 0xB: "CpuSet",
    0xC: "CpuFastSet", 0xD: "GetBiosChecksum", 0xE: "BgAffineSet",
    0xF: "ObjAffineSet", 0x10: "BitUnPack", 0x11: "LZ77UnCompWram",
    0x12: "LZ77UnCompVram", 0x13: "HuffUnComp", 0x14: "RLUnCompWram",
    0x15: "RLUnCompVram", 0x16: "Diff8bitUnFilterWram",
    0x17: "Diff8bitUnFilterVram", 0x18: "Diff16bitUnFilter",
    0x19: "SoundBias",
}

print("\n=== Thumb SWI instructions (halfword-aligned, 0xDFxx) ===")
for i in range(0, len(data)-1, 2):
    val = data[i] | (data[i+1] << 8)
    if (val & 0xFF00) == 0xDF00:
        swi = val & 0xFF
        name = names.get(swi, "Unknown")
        print(f"  Offset 0x{i:04X}: SWI #{swi} (0x{swi:02X}) - {name}")

print("\n=== Summary ===")
has_vblank = False
has_intrwait = False
for i in range(0, len(data)-1, 2):
    val = data[i] | (data[i+1] << 8)
    if val == 0xDF05:
        has_vblank = True
    if val == 0xDF04:
        has_intrwait = True
# Also check ARM encoding
for i in range(0, len(data)-3, 4):
    if data[i+3] == 0xEF:
        imm = data[i] | (data[i+1] << 8) | (data[i+2] << 16)
        if imm == 0x050000 or imm == 5:
            has_vblank = True
        if imm == 0x040000 or imm == 4:
            has_intrwait = True
print(f"  VBlankIntrWait (SWI 5) found: {has_vblank}")
print(f"  IntrWait (SWI 4) found: {has_intrwait}")
