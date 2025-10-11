#!/bin/bash
# Run mGBA in debugger mode with trace enabled
# This will open mGBA with the CLI debugger where you can type commands

echo "Starting mGBA with CLI debugger..."
echo "Once it starts, type these commands:"
echo "  trace 100"
echo "  c"
echo ""

/Applications/mGBA.app/Contents/MacOS/mGBA \
    /Users/malcolm/gba/assets/roms/arm.gba \
    -b /Users/malcolm/gba/assets/bios.bin \
    -d
