#!/bin/bash

# Enhanced ARM vs Thumb Performance Comparison
echo "=================================================================="
echo "ARM vs THUMB PERFORMANCE COMPARISON"
echo "=================================================================="
echo ""

# ARM Results (100K iterations, 10M instructions)
ARM_ARITH=290968342
ARM_MEMORY=218135811
ARM_ALU=270518855
ARM_BRANCH=304571620
ARM_MULTI=84548721
ARM_MULTIPLY=377771901

# Thumb Results (100K iterations, 10M instructions)  
THUMB_ARITH=435843793
THUMB_MEMORY=416181122
THUMB_ALU=474585923
THUMB_BRANCH=301713734

echo "Instruction Type              ARM IPS        Thumb IPS      Thumb/ARM Ratio"
echo "--------------------------------------------------------------------------"

# Calculate ratios
ARITH_RATIO=$(echo "scale=2; $THUMB_ARITH / $ARM_ARITH" | bc)
MEMORY_RATIO=$(echo "scale=2; $THUMB_MEMORY / $ARM_MEMORY" | bc)
ALU_RATIO=$(echo "scale=2; $THUMB_ALU / $ARM_ALU" | bc)
BRANCH_RATIO=$(echo "scale=2; $THUMB_BRANCH / $ARM_BRANCH" | bc)

printf "%-30s %12s %12s %12s\n" "Arithmetic (ADD)" "$ARM_ARITH" "$THUMB_ARITH" "${ARITH_RATIO}x"
printf "%-30s %12s %12s %12s\n" "Memory (STR/LDR)" "$ARM_MEMORY" "$THUMB_MEMORY" "${MEMORY_RATIO}x"
printf "%-30s %12s %12s %12s\n" "ALU Operations" "$ARM_ALU" "$THUMB_ALU" "${ALU_RATIO}x"
printf "%-30s %12s %12s %12s\n" "Branch Instructions" "$ARM_BRANCH" "$THUMB_BRANCH" "${BRANCH_RATIO}x"
printf "%-30s %12s %12s %12s\n" "Multiple Data Transfer" "$ARM_MULTI" "N/A" "ARM-only"
printf "%-30s %12s %12s %12s\n" "Multiply Instructions" "$ARM_MULTIPLY" "N/A" "ARM-only"

echo ""
echo "=================================================================="
echo "PERFORMANCE ANALYSIS"
echo "=================================================================="
echo ""

echo "Key Findings:"
echo "• Thumb arithmetic is ${ARITH_RATIO}x FASTER than ARM"
echo "• Thumb memory access is ${MEMORY_RATIO}x FASTER than ARM"
echo "• Thumb ALU operations are ${ALU_RATIO}x FASTER than ARM"
echo "• Thumb branches are ${BRANCH_RATIO}x SLOWER than ARM"
echo ""

echo "Technical Observations:"
echo "• Thumb instructions are 16-bit (2 bytes) vs ARM 32-bit (4 bytes)"
echo "• Better cache efficiency due to smaller instruction size"
echo "• Simpler instruction decoding in Thumb mode"
echo "• ARM has some instructions not available in Thumb (LDM/STM, MUL/MLA)"
echo ""

# Calculate averages
ARM_AVG=$(echo "scale=0; ($ARM_ARITH + $ARM_MEMORY + $ARM_ALU + $ARM_BRANCH) / 4" | bc)
THUMB_AVG=$(echo "scale=0; ($THUMB_ARITH + $THUMB_MEMORY + $THUMB_ALU + $THUMB_BRANCH) / 4" | bc)
OVERALL_RATIO=$(echo "scale=2; $THUMB_AVG / $ARM_AVG" | bc)

echo "Overall Performance:"
echo "• ARM average IPS:   $(printf "%12s" "$ARM_AVG")"
echo "• Thumb average IPS: $(printf "%12s" "$THUMB_AVG")"
echo "• Overall ratio:     ${OVERALL_RATIO}x (Thumb advantage)"
echo ""

echo "Recommendations:"
if (( $(echo "$OVERALL_RATIO > 1.2" | bc -l) )); then
    echo "• Thumb is significantly faster - prefer Thumb mode when possible"
    echo "• Consider using Thumb for most code, ARM for specific performance-critical sections"
else
    echo "• Performance is relatively close between ARM and Thumb"
    echo "• Choose based on code density and specific instruction requirements"
fi

echo "• Use ARM mode when you need multiple data transfer (LDM/STM) instructions"
echo "• Use ARM mode for multiply operations (MUL/MLA) if performance critical"
echo "• Thumb's smaller code size improves cache performance significantly"
echo ""

echo "=================================================================="
echo "CACHE IMPLICATIONS"
echo "=================================================================="
echo ""

# Calculate code density advantage
echo "Code Density Analysis:"
echo "• Thumb: 16-bit instructions = 2 bytes per instruction"
echo "• ARM:   32-bit instructions = 4 bytes per instruction"
echo "• Thumb code is 50% smaller than equivalent ARM code"
echo "• Better instruction cache utilization with Thumb"
echo "• Fewer cache misses due to smaller code footprint"
echo ""

echo "Memory Bandwidth:"
echo "• Thumb fetches 2 bytes per instruction vs ARM's 4 bytes"
echo "• Thumb reduces memory bandwidth requirements by 50%"
echo "• Important for systems with limited memory bandwidth"
