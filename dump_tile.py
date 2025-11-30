import subprocess
import sys

# Run the emulator briefly and capture OBJ palette
result = subprocess.run(['timeout', '1', './gba_emulator', 'assets/bios.bin'], 
                       capture_output=True, text=True, stderr=subprocess.STDOUT)

# We need to add proper logging to the emulator to capture the palette/tile data
# For now just print what we know
print("Sprites use tile=4, pal=0")
print("Tile 4 address = 0x06010000 + 4*32 = 0x06010080")
print("Palette 0 address = 0x05000200 (OBJ palette)")
print("RGB555 format: BBBBB_GGGGG_RRRRR")
