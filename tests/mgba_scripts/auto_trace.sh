#!/bin/bash
# Automated mGBA tracing via GDB server

ROM_FILE="/Users/malcolm/gba/assets/roms/arm.gba"
BIOS_FILE="/Users/malcolm/gba/assets/bios.bin"
TRACE_SCRIPT="/Users/malcolm/gba/tests/mgba_scripts/trace_via_gdb.py"

echo "======================================"
echo "mGBA Instruction Tracer (GDB Method)"
echo "======================================"
echo ""

# Start mGBA in background with GDB server
echo "Starting mGBA with GDB server in background..."
/Applications/mGBA.app/Contents/MacOS/mGBA \
    "$ROM_FILE" \
    -b "$BIOS_FILE" \
    -g &

MGBA_PID=$!
echo "mGBA started (PID: $MGBA_PID)"

# Wait for GDB server to be ready
echo "Waiting for GDB server to start..."
sleep 3

# Run the trace script
echo "Starting instruction trace..."
python3 "$TRACE_SCRIPT"

# Cleanup
echo "Killing mGBA..."
kill $MGBA_PID 2>/dev/null

echo "Done!"
