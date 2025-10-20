#!/bin/bash
# Monitor mGBA trace collection progress
# Usage: ./check_trace_progress.sh

TRACE_FILE="/tmp/mgba_memory_trace.log"
TARGET_INSTRUCTIONS=10000000

echo "=================================================="
echo "mGBA Trace Collection Progress Monitor"
echo "=================================================="
echo "Target: $TARGET_INSTRUCTIONS instructions"
echo "Trace file: $TRACE_FILE"
echo ""

# Check if trace is running
if ! ps aux | grep -q "[t]race_with_memory.py"; then
    echo "❌ Trace collection is not running!"
    echo "Start it with: python3 tests/mgba_scripts/trace_with_memory.py"
    exit 1
fi

echo "✅ Trace collection is running"
echo ""

# Get current count
if [ -f "$TRACE_FILE" ]; then
    current=$(grep -c "^Instruction #" "$TRACE_FILE" 2>/dev/null || echo "0")
    percent=$(echo "scale=2; ($current / $TARGET_INSTRUCTIONS) * 100" | bc)
    
    echo "Progress: $current / $TARGET_INSTRUCTIONS instructions ($percent%)"
    
    # Check for ROM entry
    rom_line=$(grep "PC: 0x08" "$TRACE_FILE" 2>/dev/null | head -1)
    if [ ! -z "$rom_line" ]; then
        rom_instr=$(echo "$rom_line" | grep -B 10 "PC: 0x08" | grep "^Instruction #" | tail -1 | sed 's/Instruction #//')
        echo "🎯 ROM CODE REACHED at instruction: $rom_instr"
    else
        echo "⏳ Still in BIOS (no ROM code yet)"
    fi
    
    # Estimate time remaining (assuming ~2700 instructions/second)
    remaining=$((TARGET_INSTRUCTIONS - current))
    seconds_remaining=$((remaining / 2700))
    hours=$((seconds_remaining / 3600))
    minutes=$(((seconds_remaining % 3600) / 60))
    
    echo ""
    echo "Estimated time remaining: ${hours}h ${minutes}m"
else
    echo "⏳ Trace file not created yet, waiting..."
fi

echo ""
echo "=================================================="
echo "To monitor continuously, run:"
echo "  watch -n 30 ./check_trace_progress.sh"
echo ""
echo "To view the trace in real-time:"
echo "  tail -f $TRACE_FILE"
echo "=================================================="
