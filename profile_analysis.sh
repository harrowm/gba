#!/bin/bash

# Profile analysis script for thumb benchmark

echo "=== Thumb CPU Performance Analysis ==="
echo "Reverted Implementation Profile Results"
echo "======================================"

# Run the benchmark with profiling
echo "Running profiled benchmark..."
CPUPROFILE=thumb_reverted_analysis.prof ./thumb_benchmark > benchmark_output.txt 2>&1

# Extract the results
echo ""
echo "=== Benchmark Results ==="
cat benchmark_output.txt | grep -A 20 "=== Thumb.*Benchmark ==="

echo ""
echo "=== Profiling Results ==="
echo "Top CPU consumers:"
pprof --text --functions thumb_benchmark thumb_reverted_analysis.prof 2>/dev/null | head -20

echo ""
echo "=== Performance Summary ==="
echo "Analyzing performance characteristics..."

# Extract IPS values for each benchmark category
ARITHMETIC_IPS=$(grep "100000.*10000000" benchmark_output.txt | head -1 | awk '{print $3}')
MEMORY_IPS=$(grep "100000.*10000000" benchmark_output.txt | head -2 | tail -1 | awk '{print $3}')
ALU_IPS=$(grep "100000.*10000000" benchmark_output.txt | head -3 | tail -1 | awk '{print $3}')
BRANCH_IPS=$(grep "100000.*10000000" benchmark_output.txt | head -4 | tail -1 | awk '{print $3}')

echo "Arithmetic Instructions: $ARITHMETIC_IPS IPS"
echo "Memory Instructions:     $MEMORY_IPS IPS"
echo "ALU Instructions:        $ALU_IPS IPS"
echo "Branch Instructions:     $BRANCH_IPS IPS"

# Calculate relative performance
if [ -n "$ARITHMETIC_IPS" ] && [ -n "$BRANCH_IPS" ]; then
    # Use bc for floating point arithmetic
    RATIO=$(echo "scale=2; $ALU_IPS / $BRANCH_IPS" | bc 2>/dev/null)
    echo "ALU/Branch ratio:        $RATIO (higher is better for ALU performance)"
fi

echo ""
echo "=== Analysis Complete ==="
echo "Profile data saved to: thumb_reverted_analysis.prof"
echo "Benchmark output saved to: benchmark_output.txt"
