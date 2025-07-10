#!/bin/bash

# This script compares different instruction cache sizes by directly compiling and running the ALU benchmark

echo "=== ARM CPU Instruction Cache Size Comparison ==="

# Clean up any previous builds
rm -f alu_benchmark_1024 alu_benchmark_2048 alu_benchmark_4096

# Make sure build directory exists
mkdir -p build

# Create results file
echo "# ARM CPU Instruction Cache Size Comparison" > cache_comparison.md
echo "" >> cache_comparison.md

# Function to compile and run benchmark with a specific cache size
run_benchmark() {
    CACHE_SIZE=$1
    TAG_SHIFT=$2
    OUTPUT_FILE=alu_benchmark_${CACHE_SIZE}
    
    echo "Compiling ALU benchmark with ${CACHE_SIZE}-entry instruction cache..."
    
    # Create header override file
    echo "#ifndef CACHE_SIZE_OVERRIDE_H" > build/cache_size_override.h
    echo "#define CACHE_SIZE_OVERRIDE_H" >> build/cache_size_override.h
    echo "" >> build/cache_size_override.h
    echo "// Override for instruction cache configuration" >> build/cache_size_override.h
    echo "#define ARM_ICACHE_SIZE ${CACHE_SIZE}" >> build/cache_size_override.h
    echo "#define ARM_ICACHE_MASK (ARM_ICACHE_SIZE - 1)" >> build/cache_size_override.h
    echo "#define ARM_ICACHE_TAG_SHIFT ${TAG_SHIFT}" >> build/cache_size_override.h
    echo "#define ARM_CACHE_STATS 1" >> build/cache_size_override.h
    echo "" >> build/cache_size_override.h
    echo "#endif // CACHE_SIZE_OVERRIDE_H" >> build/cache_size_override.h
    
    # Compile the benchmark with the override header
    g++ -std=c++17 -Wall -Wextra -Iinclude -O3 -flto -DDEBUG_LEVEL=0 -DNDEBUG -DBENCHMARK_MODE -DDISABLE_MEMORY_BOUNDS_CHECK \
        -include build/cache_size_override.h \
        tests/alu_benchmark.cpp \
        src/arm_cpu.cpp src/cpu.cpp src/debug.cpp src/gba.cpp src/gpu.cpp \
        src/instruction_decoder.cpp src/interrupt_controller.cpp src/memory.cpp \
        src/thumb_cpu.cpp src/arm_timing.c src/thumb_timing.c src/timing.c \
        src/thumb_execute_optimizations.c \
        -o ${OUTPUT_FILE}
    
    if [ $? -ne 0 ]; then
        echo "Error compiling ${OUTPUT_FILE}"
        return 1
    fi
    
    echo "Running ALU benchmark with ${CACHE_SIZE}-entry instruction cache..."
    echo "## ${CACHE_SIZE}-Entry Cache" >> cache_comparison.md
    echo "\`\`\`" >> cache_comparison.md
    ./${OUTPUT_FILE} --cache-stats >> cache_comparison.md
    echo "\`\`\`" >> cache_comparison.md
    echo "" >> cache_comparison.md
    
    # Run multiple iterations for consistency
    IPS_SUM=0
    ITERATIONS=3
    
    echo "## ${CACHE_SIZE}-Entry Cache (Multiple Iterations)" >> cache_comparison.md
    echo "\`\`\`" >> cache_comparison.md
    
    for i in $(seq 1 ${ITERATIONS}); do
        echo "Iteration $i" >> cache_comparison.md
        ./${OUTPUT_FILE} >> cache_comparison.md
        
        # Extract average IPS for this run
        RUN_IPS=$(./alu_benchmark_${CACHE_SIZE} | grep -oE "[0-9]+ IPS" | awk '{sum+=$1} END {print sum/NR}')
        IPS_SUM=$((IPS_SUM + RUN_IPS))
        
        echo "" >> cache_comparison.md
    done
    
    echo "\`\`\`" >> cache_comparison.md
    echo "" >> cache_comparison.md
    
    # Calculate average IPS across iterations
    AVG_IPS=$((IPS_SUM / ITERATIONS))
    echo "Average IPS for ${CACHE_SIZE}-entry cache: ${AVG_IPS}" >> cache_comparison.md
    echo "" >> cache_comparison.md
    
    echo "${CACHE_SIZE},${AVG_IPS}" >> build/cache_results.csv
}

# Clear previous results
echo "cache_size,avg_ips" > build/cache_results.csv

# Run benchmarks with different cache sizes
run_benchmark 1024 10
run_benchmark 2048 11
run_benchmark 4096 12

# Analyze results
echo "## Comparative Analysis" >> cache_comparison.md
echo "" >> cache_comparison.md
echo "| Cache Size | Average IPS | Relative Improvement |" >> cache_comparison.md
echo "|------------|-------------|---------------------|" >> cache_comparison.md

BASE_IPS=$(grep "^1024," build/cache_results.csv | cut -d',' -f2)
while IFS=, read -r SIZE IPS; do
    if [ "$SIZE" != "cache_size" ]; then
        PERCENT=$(echo "scale=2; (($IPS - $BASE_IPS) * 100) / $BASE_IPS" | bc)
        echo "| $SIZE | $IPS | ${PERCENT}% |" >> cache_comparison.md
    fi
done < build/cache_results.csv

echo "" >> cache_comparison.md

# Determine the optimal cache size based on diminishing returns
IPS_1024=$(grep "^1024," build/cache_results.csv | cut -d',' -f2)
IPS_2048=$(grep "^2048," build/cache_results.csv | cut -d',' -f2)
IPS_4096=$(grep "^4096," build/cache_results.csv | cut -d',' -f2)

GAIN_1024_TO_2048=$(echo "scale=2; ($IPS_2048 - $IPS_1024) / $IPS_1024" | bc)
GAIN_2048_TO_4096=$(echo "scale=2; ($IPS_4096 - $IPS_2048) / $IPS_2048" | bc)

echo "## Recommendation" >> cache_comparison.md
echo "" >> cache_comparison.md
echo "Performance gain from 1024 to 2048 entries: ${GAIN_1024_TO_2048}" >> cache_comparison.md
echo "Performance gain from 2048 to 4096 entries: ${GAIN_2048_TO_4096}" >> cache_comparison.md
echo "" >> cache_comparison.md

# Make a recommendation based on the performance gains
if (( $(echo "$GAIN_2048_TO_4096 > 0.02" | bc -l) )); then
    echo "**Recommendation**: Use a 4096-entry instruction cache for optimal performance." >> cache_comparison.md
    RECOMMENDED_SIZE=4096
elif (( $(echo "$GAIN_1024_TO_2048 > 0.02" | bc -l) )); then
    echo "**Recommendation**: Use a 2048-entry instruction cache for the best balance of performance and memory usage." >> cache_comparison.md
    RECOMMENDED_SIZE=2048
else
    echo "**Recommendation**: The 1024-entry cache provides adequate performance. Increasing cache size shows minimal gains." >> cache_comparison.md
    RECOMMENDED_SIZE=1024
fi

# Add to optimization summary
echo "" >> OPTIMIZATION_SUMMARY.md
echo "## Instruction Cache Size Optimization" >> OPTIMIZATION_SUMMARY.md
echo "" >> OPTIMIZATION_SUMMARY.md
echo "After analyzing different instruction cache sizes, we determined that a ${RECOMMENDED_SIZE}-entry cache provides the best performance characteristics for our workload. See \`cache_comparison.md\` for detailed benchmark results." >> OPTIMIZATION_SUMMARY.md
echo "" >> OPTIMIZATION_SUMMARY.md

echo "=== Benchmark complete! ==="
echo "Results saved to cache_comparison.md and appended to OPTIMIZATION_SUMMARY.md"
