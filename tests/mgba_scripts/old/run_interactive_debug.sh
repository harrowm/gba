#!/bin/bash
# Interactive mGBA debugger with instructions
# You'll need to manually type commands

echo "=========================================="
echo "mGBA Debugger - Instruction Tracing Guide"
echo "=========================================="
echo ""
echo "Starting mGBA in debugger mode..."
echo ""
echo "Available commands once started:"
echo "  help              - Show all commands"
echo "  i/info            - Show CPU state (registers)"
echo "  n/next            - Step one instruction"
echo "  c/continue        - Continue execution"
echo "  b/break <addr>    - Set breakpoint"
echo "  p/print <addr>    - Print memory"
echo "  q/quit            - Exit debugger"
echo ""
echo "To trace instructions:"
echo "  1. Type 'i' to see current CPU state"
echo "  2. Type 'n' repeatedly to step through instructions"
echo "  3. Or set a breakpoint and use 'c' to continue"
echo ""
echo "Press Enter to start..."
read

/Applications/mGBA.app/Contents/MacOS/mGBA \
    /Users/malcolm/gba/assets/roms/arm.gba \
    -b /Users/malcolm/gba/assets/bios.bin \
    -d
