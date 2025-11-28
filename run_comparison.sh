#!/bin/bash

# Comprehensive script to rebuild both emulators, run them, and compare

echo "================================================================================"
echo "STEP 1: Rebuild mGBA with palette/tile logging"
echo "================================================================================"
cd /Users/malcolm/mgba-instrumented/build
make -j8
if [ $? -ne 0 ]; then
    echo "❌ mGBA build failed!"
    exit 1
fi
echo "✅ mGBA rebuilt successfully"
echo ""

echo "================================================================================"
echo "STEP 2: Rebuild our emulator with palette/tile logging"
echo "================================================================================"
cd /Users/malcolm/gba
make clean && make
if [ $? -ne 0 ]; then
    echo "❌ Our emulator build failed!"
    exit 1
fi
echo "✅ Our emulator rebuilt successfully"
echo ""

echo "================================================================================"
echo "STEP 3: Run mGBA with BIOS (will run for 30 seconds)"
echo "================================================================================"
# Run mGBA for 30 seconds to capture frames 100-150
timeout 30s /Users/malcolm/mgba-instrumented/build/sdl/mgba -b /Users/malcolm/gba/assets/bios.bin > /tmp/mgba_output.txt 2>&1
echo "✅ mGBA run complete"
echo ""

echo "================================================================================"
echo "STEP 4: Run our emulator (will run for 30 seconds)"
echo "================================================================================"
# Kill any existing instance
pkill -f gba_emulator
# Run our emulator for 30 seconds
timeout 30s /Users/malcolm/gba/gba_emulator > /tmp/gba_output.txt 2>&1
echo "✅ Our emulator run complete"
echo ""

echo "================================================================================"
echo "STEP 5: Extract palette/tile logs"
echo "================================================================================"
echo "mGBA log:"
grep 'PALETTE/TILE' /tmp/mgba_output.txt | head -20
echo ""
echo "Our emulator log:"
grep 'PALETTE/TILE' /tmp/gba_output.txt | head -20
echo ""

echo "================================================================================"
echo "STEP 6: Compare data"
echo "================================================================================"
python3 /Users/malcolm/gba/compare_palette_tiles.py /tmp/mgba_output.txt /tmp/gba_output.txt

echo ""
echo "================================================================================"
echo "Full logs saved to:"
echo "  /tmp/mgba_output.txt"
echo "  /tmp/gba_output.txt"
echo "================================================================================"
