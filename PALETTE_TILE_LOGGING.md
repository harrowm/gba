# Palette and Tile Logging Implementation

## Summary

Added debugging code to both emulators to log OBJ Palette 0 and Tile 0 data at frames 100-150 (every 10th frame). This will help identify whether the purple artifacts are due to wrong data content or timing issues.

## Changes Made

### 1. Our Emulator (src/gpu.cpp)
- Added logging at start of renderScanline() when scanline == 0
- Logs at frames 100, 110, 120, 130, 140, 150
- Outputs:
  - OBJ Palette 0: All 16 colors in RGB555 format
  - Tile 0 checksum: Sum of first 32 bytes
  - Tile 0 first 8 bytes: Raw hex data

### 2. mGBA (src/gba/renderers/video-software.c)
- Added similar logging in GBAVideoSoftwareRendererDrawScanline()
- Same frame ranges and output format
- Backup created at: `/Users/malcolm/mgba-instrumented/src/gba/renderers/video-software.c.backup_palette`

### 3. Comparison Script (compare_palette_tiles.py)
- Parses both log files
- Compares palette colors, tile checksums, and raw bytes
- Shows exactly which frames/colors differ

### 4. Automated Run Script (run_comparison.sh)
- Rebuilds both emulators
- Runs both with BIOS
- Extracts logs
- Runs comparison
- Shows results

## How to Use

### Quick Start
```bash
cd /Users/malcolm/gba
./run_comparison.sh
```

### Manual Steps
```bash
# 1. Rebuild mGBA
cd /Users/malcolm/mgba-instrumented/build
make -j8

# 2. Rebuild our emulator
cd /Users/malcolm/gba
make clean && make

# 3. Run mGBA
/Users/malcolm/mgba-instrumented/build/qt/mGBA.app/Contents/MacOS/mGBA -b assets/bios.bin > /tmp/mgba_output.txt 2>&1

# 4. Run our emulator
./gba_emulator > /tmp/gba_output.txt 2>&1

# 5. Compare
python3 compare_palette_tiles.py /tmp/mgba_output.txt /tmp/gba_output.txt
```

## What to Look For

### If Palette Data Differs
- Purple colors (like 0x7C1F = magenta) in our palette but not mGBA's
- Wrong color at index 0 (should be transparent/black)
- Indicates palette loading issue (DMA timing, BIOS function)

### If Tile 0 Checksum Differs
- mGBA has non-zero checksum, ours has 0x00000000
- Indicates tile data not loaded yet (timing issue)
- Check VRAM DMA transfers

### If Both Match
- Data is identical between emulators
- Problem is in rendering logic, not data
- Check sprite rendering order, blending, or window effects

## Expected Purple Sprite Details

From OAM analysis:
- OBJ4, OBJ5, OBJ6 appear frames 100-160
- All use Tile=0, 4, 12, 16, 192, 196
- All use Palette=0
- All are 64x64 affine double-size semi-transparent sprites

If Tile 0 is supposed to contain highlight/shine graphics but contains zeros or garbage, sprites will render as solid blocks of whatever colors are in Palette 0.

## Next Steps

### If Data Matches
Need to log DMA/VRAM writes:
- Track all writes to 0x06010000-0x06017FFF (OBJ tiles)
- Track all writes to 0x05000200-0x050003FF (OBJ palette)
- Compare timing between emulators

### If Data Differs
Fix the earlier problem:
- Check BIOS function execution
- Check DMA controller timing
- Check memory write handling

## Reverting Changes

To restore original mGBA code:
```bash
cp /Users/malcolm/mgba-instrumented/src/gba/renderers/video-software.c.backup_palette \
   /Users/malcolm/mgba-instrumented/src/gba/renderers/video-software.c
cd /Users/malcolm/mgba-instrumented/build
make -j8
```

To remove logging from our emulator:
- Edit src/gpu.cpp
- Remove the debug block (lines with "DEBUG: Log OBJ Palette 0")
- Rebuild with `make clean && make`
