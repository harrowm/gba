def decode_swp(index):
    # bits 27-19
    bits = index
    b27 = (bits >> 8) & 1
    b26 = (bits >> 7) & 1
    b25 = (bits >> 6) & 1
    b24 = (bits >> 5) & 1
    b23 = (bits >> 4) & 1
    b22 = (bits >> 3) & 1
    b21 = (bits >> 2) & 1
    b20 = (bits >> 1) & 1
    b19 = bits & 1

    # SWP/SWPB: bits 27-23 = 00010, bits 22-19 = 0010 (SWP) or 0011 (SWPB)
    if b27 == 0 and b26 == 0 and b25 == 0 and b24 == 1 and b23 == 0:
        if b22 == 0 and b21 == 1 and b20 == 0 and b19 == 0:
            return "SWP"
        elif b22 == 0 and b21 == 1 and b20 == 0 and b19 == 1:
            return "SWPB"
    return None

print(f"{'Index':>5} {'Binary':>11} {'Mnemonic':>6}")
for i in range(512):
    mnemonic = decode_swp(i)
    if mnemonic:
        print(f"0x{i:03X} {i:09b} {mnemonic:>6}")
