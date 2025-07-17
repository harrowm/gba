def decode_single_data_transfer(index):
    # bits 27-19
    bits = index
    b27 = (bits >> 8) & 1
    b26 = (bits >> 7) & 1
    b25 = (bits >> 6) & 1  # 0=reg offset, 1=imm offset
    b24 = (bits >> 5) & 1  # pre/post
    b23 = (bits >> 4) & 1  # up/down
    b22 = (bits >> 3) & 1  # byte/word
    b21 = (bits >> 2) & 1  # writeback
    b20 = (bits >> 1) & 1  # load/store

    # Only single data transfer: b27=0, b26=1
    if b27 == 0 and b26 == 1:
        # STR/LDR, STRB/LDRB, IMM/REG
        if b20 == 0 and b22 == 0 and b25 == 1:
            return "STR_IMM"
        elif b20 == 0 and b22 == 0 and b25 == 0:
            return "STR_REG"
        elif b20 == 0 and b22 == 1 and b25 == 1:
            return "STRB_IMM"
        elif b20 == 0 and b22 == 1 and b25 == 0:
            return "STRB_REG"
        elif b20 == 1 and b22 == 0 and b25 == 1:
            return "LDR_IMM"
        elif b20 == 1 and b22 == 0 and b25 == 0:
            return "LDR_REG"
        elif b20 == 1 and b22 == 1 and b25 == 1:
            return "LDRB_IMM"
        elif b20 == 1 and b22 == 1 and b25 == 0:
            return "LDRB_REG"
    return None

print(f"{'Index':>5} {'Binary':>11} {'Type':>10}")
for i in range(512):
    mnemonic = decode_single_data_transfer(i)
    if mnemonic:
        print(f"0x{i:03X} {i:09b} {mnemonic:>10}")
