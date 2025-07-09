#!/bin/bash

# ARM vs Thumb Benchmark Comparison Script

echo "==================================================================="
echo "ARM vs Thumb Performance Comparison"
echo "==================================================================="
echo ""

echo "Building benchmarks..."
make arm_benchmark thumb_benchmark > /dev/null 2>&1

if [ ! -f "./arm_benchmark" ] || [ ! -f "./thumb_benchmark" ]; then
    echo "Error: Failed to build benchmarks"
    exit 1
fi

echo "Running ARM benchmark..."
echo ""
./arm_benchmark > arm_results.tmp

echo ""
echo "Running Thumb benchmark..."
echo ""
./thumb_benchmark > thumb_results.tmp

echo ""
echo "==================================================================="
echo "PERFORMANCE COMPARISON SUMMARY"
echo "==================================================================="
echo ""

# Extract key performance numbers for comparison (100k iterations)
ARM_ARITH=$(grep -A 4 "ARM Arithmetic" arm_results.tmp | grep "100000" | awk '{print $3}')
ARM_MEM=$(grep -A 4 "ARM Memory" arm_results.tmp | grep "100000" | awk '{print $3}')

THUMB_ARITH=$(grep -A 4 "Thumb Arithmetic" thumb_results.tmp | grep "100000" | awk '{print $3}')
THUMB_MEM=$(grep -A 4 "Thumb Memory" thumb_results.tmp | grep "100000" | awk '{print $3}')
THUMB_ALU=$(grep -A 4 "Thumb ALU" thumb_results.tmp | grep "100000" | awk '{print $3}')
THUMB_BRANCH=$(grep -A 4 "Thumb Branch" thumb_results.tmp | grep "100000" | awk '{print $3}')

# Check if values are empty and set defaults
ARM_ARITH=${ARM_ARITH:-0}
ARM_MEM=${ARM_MEM:-0}
THUMB_ARITH=${THUMB_ARITH:-0}
THUMB_MEM=${THUMB_MEM:-0}
THUMB_ALU=${THUMB_ALU:-0}
THUMB_BRANCH=${THUMB_BRANCH:-0}

echo "Instruction Type          ARM IPS       Thumb IPS     Thumb/ARM Ratio"
echo "-------------------------------------------------------------------"
if command -v bc >/dev/null 2>&1 && [ "$ARM_ARITH" -ne 0 ] && [ "$THUMB_ARITH" -ne 0 ]; then
    ARITH_RATIO=$(echo "scale=2; $THUMB_ARITH / $ARM_ARITH" | bc -l)
    printf "%-25s %-13s %-13s %.2fx\n" "Arithmetic (ADD)" "$ARM_ARITH" "$THUMB_ARITH" "$ARITH_RATIO"
else
    printf "%-25s %-13s %-13s %s\n" "Arithmetic (ADD)" "$ARM_ARITH" "$THUMB_ARITH" "N/A"
fi

if command -v bc >/dev/null 2>&1 && [ "$ARM_MEM" -ne 0 ] && [ "$THUMB_MEM" -ne 0 ]; then
    MEM_RATIO=$(echo "scale=2; $THUMB_MEM / $ARM_MEM" | bc -l)
    printf "%-25s %-13s %-13s %.2fx\n" "Memory (STR/LDR)" "$ARM_MEM" "$THUMB_MEM" "$MEM_RATIO"
else
    printf "%-25s %-13s %-13s %s\n" "Memory (STR/LDR)" "$ARM_MEM" "$THUMB_MEM" "N/A"
fi

printf "%-25s %-13s %-13s %s\n" "ALU Operations" "N/A" "$THUMB_ALU" "Thumb-only"
printf "%-25s %-13s %-13s %s\n" "Branch Instructions" "N/A" "$THUMB_BRANCH" "Thumb-only"

echo ""
echo "==================================================================="
echo "ANALYSIS"
echo "==================================================================="
echo ""

if command -v bc >/dev/null 2>&1 && [ "$ARM_ARITH" -ne 0 ] && [ "$THUMB_ARITH" -ne 0 ]; then
    ARITH_RATIO=$(echo "scale=2; $THUMB_ARITH / $ARM_ARITH" | bc -l)
    MEM_RATIO=$(echo "scale=2; $THUMB_MEM / $ARM_MEM" | bc -l)
    
    echo "• Thumb arithmetic performance: ${ARITH_RATIO}x relative to ARM"
    echo "• Thumb memory access performance: ${MEM_RATIO}x relative to ARM"
    
    if (( $(echo "$ARITH_RATIO > 1.0" | bc -l) )); then
        echo "• Thumb arithmetic instructions are FASTER than ARM"
    else
        echo "• ARM arithmetic instructions are FASTER than Thumb"
    fi
    
    if (( $(echo "$MEM_RATIO > 1.0" | bc -l) )); then
        echo "• Thumb memory access instructions are FASTER than ARM"
    else
        echo "• ARM memory access instructions are FASTER than Thumb"
    fi
else
    echo "• Install 'bc' for detailed performance ratio calculations"
    echo "• Values: ARM_ARITH=$ARM_ARITH, THUMB_ARITH=$THUMB_ARITH"
    echo "• Values: ARM_MEM=$ARM_MEM, THUMB_MEM=$THUMB_MEM"
fi

echo ""
echo "Key Observations:"
echo "• Thumb instructions are 16-bit vs ARM's 32-bit (better code density)"
echo "• Thumb has simpler instruction decoding"
echo "• Current implementation: Thumb entirely in C++, ARM has C fast-paths"
echo ""

if [ -n "$THUMB_ARITH" ] && [ "$THUMB_ARITH" -gt 0 ] && [ -n "$ARM_ARITH" ] && [ "$ARM_ARITH" -gt 0 ]; then
    if [ "$THUMB_ARITH" -gt "$ARM_ARITH" ]; then
        echo "RECOMMENDATION: Thumb is already performing well - optimization may not be necessary"
    else
        echo "RECOMMENDATION: Consider applying ARM's C/C++ split pattern to Thumb for optimization"
    fi
else
    echo "RECOMMENDATION: Could not determine relative performance - check benchmark results"
fi

# Clean up temporary files
rm -f arm_results.tmp thumb_results.tmp

echo ""
echo "Individual benchmark outputs saved as ./arm_benchmark and ./thumb_benchmark"
echo "Re-run anytime with: make run-arm-benchmark && make run-thumb-benchmark"
