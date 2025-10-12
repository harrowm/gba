#!/bin/bash
# Start mGBA with GDB server for tracing

# Get absolute path to script directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

MGBA_APP="/Applications/mGBA.app/Contents/MacOS/mGBA"
ROM_FILE="$PROJECT_ROOT/assets/roms/arm.gba"
BIOS_FILE="$PROJECT_ROOT/assets/bios.bin"

cd "$PROJECT_ROOT" || exit 1

echo "Starting mGBA with GDB server..."
echo "ROM: $ROM_FILE"
echo "BIOS: $BIOS_FILE"

"$MGBA_APP" "$ROM_FILE" -b "$BIOS_FILE" -g > /dev/null 2>&1 &
MGBA_PID=$!

echo "mGBA started with PID: $MGBA_PID"
echo "GDB server should be available on port 2345"
echo "To stop: kill $MGBA_PID"
