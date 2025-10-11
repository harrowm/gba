#!/bin/bash
# Trace mGBA execution with memory reads via GDB

# Get absolute path to script directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

MGBA_APP="/Applications/mGBA.app/Contents/MacOS/mGBA"
ROM_FILE="$PROJECT_ROOT/assets/roms/arm.gba"
BIOS_FILE="$PROJECT_ROOT/assets/bios.bin"
TRACE_SCRIPT="$SCRIPT_DIR/trace_with_memory.py"

cd "$PROJECT_ROOT" || exit 1

echo "Starting mGBA with GDB server..."
"$MGBA_APP" "$ROM_FILE" -b "$BIOS_FILE" -g >/dev/null 2>&1 &
MGBA_PID=$!

echo "mGBA PID: $MGBA_PID"
echo "Waiting for GDB server to start..."
sleep 3

echo "Running trace script..."
python3 "$TRACE_SCRIPT"
TRACE_EXIT=$?

echo "Stopping mGBA..."
kill $MGBA_PID 2>/dev/null
wait $MGBA_PID 2>/dev/null

echo "Done!"
exit $TRACE_EXIT
