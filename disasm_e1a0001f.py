from capstone import *

md = Cs(CS_ARCH_ARM, CS_MODE_ARM)
# Correct little-endian encoding for 0xE1A0001F
code = bytes.fromhex('1F00A0E1')
for i in md.disasm(code, 0x08000B08):
    print(f'{i.address:08X}: {i.mnemonic} {i.op_str}')
