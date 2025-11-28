#!/bin/bash

echo "Running mGBA for 30 seconds..."
timeout 30s /Users/malcolm/mgba-instrumented/build/sdl/mgba -b /Users/malcolm/gba/assets/bios.bin > /tmp/mgba_output.txt 2>&1
echo "mGBA done"

echo ""
echo "Killing any existing emulator..."
pkill -f gba_emulator 2>/dev/null || true

echo "Running our emulator for 30 seconds..."
cd /Users/malcolm/gba
timeout 30s ./gba_emulator > /tmp/gba_output.txt 2>&1
echo "Our emulator done"

echo ""
echo "Comparing results..."
python3 /Users/malcolm/gba/compare_palette_tiles.py /tmp/mgba_output.txt /tmp/gba_output.txt
