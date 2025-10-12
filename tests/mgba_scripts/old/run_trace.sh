#!/bin/bash
# Use mGBA's CLI debugger to trace instructions
# This creates an instruction-by-instruction trace

TRACE_FILE="/tmp/mgba_instruction_trace.log"
ROM_FILE="/Users/malcolm/gba/assets/roms/arm.gba"
BIOS_FILE="/Users/malcolm/gba/assets/bios.bin"

# Create a debug command file
cat > /tmp/mgba_debug_commands.txt <<'EOF'
# Trace 1000 instructions to file
trace /tmp/mgba_instruction_trace.log 1000
# Continue execution
c
EOF

echo "Starting mGBA debugger..."
echo "Tracing 1000 instructions to $TRACE_FILE"
echo ""

# Run mGBA in debug mode with command file
/Applications/mGBA.app/Contents/MacOS/mGBA \
    "$ROM_FILE" \
    -b "$BIOS_FILE" \
    -d < /tmp/mgba_debug_commands.txt

echo ""
echo "Trace complete! Output saved to: $TRACE_FILE"
echo "First 50 lines:"
head -50 "$TRACE_FILE"
