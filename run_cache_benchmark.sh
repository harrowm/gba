#!/bin/bash

# This script benchmarks the ARM CPU with different instruction cache sizes (1024, 2048, 4096)

echo "=== ARM CPU Instruction Cache Size Benchmark ==="

# Clean up any previous benchmark results
rm -f cache_comparison.md cache_results.csv
rm -f alu_benchmark_1024 alu_benchmark_2048 alu_benchmark_4096

# Create output directories
mkdir -p build

# Create result files
echo "# ARM CPU Instruction Cache Size Comparison" > cache_comparison.md
echo "" >> cache_comparison.md
echo "cache_size,avg_ips,hit_rate" > cache_results.csv

# Function to benchmark with a specific cache size
benchmark_cache_size() {
    CACHE_SIZE=$1
    TAG_SHIFT=$2
    OUTPUT_NAME="alu_benchmark_${CACHE_SIZE}"
    
    echo "Benchmarking with ${CACHE_SIZE}-entry instruction cache..."
    
    # Back up original header file
    if [ ! -f include/arm_instruction_cache.h.orig ]; then
        cp include/arm_instruction_cache.h include/arm_instruction_cache.h.orig
    fi
    
    # Modify the cache size in the header file
    sed -i '' "s/constexpr uint32_t ARM_ICACHE_SIZE = [0-9]*;/constexpr uint32_t ARM_ICACHE_SIZE = ${CACHE_SIZE};/" include/arm_instruction_cache.h
    sed -i '' "s/constexpr uint32_t ARM_ICACHE_TAG_SHIFT = [0-9]*;/constexpr uint32_t ARM_ICACHE_TAG_SHIFT = ${TAG_SHIFT};/" include/arm_instruction_cache.h
    sed -i '' "s/#define ARM_CACHE_STATS [0-9]/#define ARM_CACHE_STATS 1/" include/arm_instruction_cache.h
    
    # Compile with optimized flags
    echo "Compiling ${OUTPUT_NAME}..."
    g++ -std=c++17 -Wall -Wextra -Iinclude -O3 -flto -DDEBUG_LEVEL=0 -DNDEBUG -DBENCHMARK_MODE -DDISABLE_MEMORY_BOUNDS_CHECK \
        tests/alu_benchmark.cpp \
        src/arm_cpu.cpp src/cpu.cpp src/debug.cpp src/gba.cpp src/gpu.cpp \
        src/instruction_decoder.cpp src/interrupt_controller.cpp src/memory.cpp \
        src/thumb_cpu.cpp src/arm_timing.c src/thumb_timing.c src/timing.c \
        src/thumb_execute_optimizations.c \
        -o ${OUTPUT_NAME}
    
    if [ $? -ne 0 ]; then
        echo "Error: Compilation failed for ${CACHE_SIZE}-entry cache"
        # Restore original header
        cp include/arm_instruction_cache.h.orig include/arm_instruction_cache.h
        return 1
    fi
    
    echo "Running benchmark for ${CACHE_SIZE}-entry cache..."
    
    # First run with cache stats
    echo "## ${CACHE_SIZE}-Entry Cache" >> cache_comparison.md
    echo "\`\`\`" >> cache_comparison.md
    ./${OUTPUT_NAME} --cache-stats >> cache_comparison.md
    echo "\`\`\`" >> cache_comparison.md
    echo "" >> cache_comparison.md
    
    # Extract cache hit rate
    HIT_RATE=$(grep "Cache hit rate" cache_comparison.md | tail -1 | grep -oE "[0-9]+\.[0-9]+%")
    
    # Run benchmark multiple times for consistent results
    echo "## ${CACHE_SIZE}-Entry Cache Performance (Multiple Runs)" >> cache_comparison.md
    echo "\`\`\`" >> cache_comparison.md
    
    ITERATIONS=5
    ALL_IPS_VALUES=""
    
    for i in $(seq 1 $ITERATIONS); do
        echo "Run $i:" >> cache_comparison.md
        ./${OUTPUT_NAME} > run_output.tmp
        cat run_output.tmp >> cache_comparison.md
        
        # Extract all IPS values from this run and save them
        RUN_IPS_VALUES=$(grep -oE "[0-9]+ IPS" run_output.tmp | cut -d' ' -f1)
        ALL_IPS_VALUES+=" $RUN_IPS_VALUES"
        
        echo "" >> cache_comparison.md
    done
    
    echo "\`\`\`" >> cache_comparison.md
    echo "" >> cache_comparison.md
    
    # Calculate average IPS across all runs and operations
    AVG_IPS=$(echo $ALL_IPS_VALUES | tr ' ' '\n' | awk '{ sum += $1; n++ } END { if (n > 0) print int(sum / n); else print "0" }')
    
    echo "Average IPS for ${CACHE_SIZE}-entry cache: ${AVG_IPS}" >> cache_comparison.md
    echo "" >> cache_comparison.md
    
    # Save to CSV for later analysis
    echo "${CACHE_SIZE},${AVG_IPS},${HIT_RATE}" >> cache_results.csv
    
    echo "Completed benchmark for ${CACHE_SIZE}-entry cache"
    echo "--------------------------------"
}

# Run benchmarks for different cache sizes
benchmark_cache_size 1024 10
benchmark_cache_size 2048 11
benchmark_cache_size 4096 12

# Restore original header file
if [ -f include/arm_instruction_cache.h.orig ]; then
    cp include/arm_instruction_cache.h.orig include/arm_instruction_cache.h
    rm include/arm_instruction_cache.h.orig
fi

# Create summary table
echo "## Summary Comparison" >> cache_comparison.md
echo "" >> cache_comparison.md
echo "| Cache Size | Average IPS | Cache Hit Rate | Relative Performance |" >> cache_comparison.md
echo "|------------|-------------|----------------|----------------------|" >> cache_comparison.md

# Process CSV data and calculate relative performance
BASE_IPS=$(grep "^1024," cache_results.csv | cut -d',' -f2)

while IFS=, read -r SIZE IPS HIT_RATE; do
    if [ "$SIZE" != "cache_size" ]; then
        # Calculate percentage improvement over 1024-entry cache
        PERF_INCREASE=$(echo "scale=2; ($IPS - $BASE_IPS) * 100 / $BASE_IPS" | bc -l)
        echo "| $SIZE | $IPS | $HIT_RATE | +${PERF_INCREASE}% |" >> cache_comparison.md
    fi
done < cache_results.csv

echo "" >> cache_comparison.md

# Analyze performance gains between cache sizes
echo "## Performance Analysis" >> cache_comparison.md
echo "" >> cache_comparison.md

IPS_1024=$(grep "^1024," cache_results.csv | cut -d',' -f2)
IPS_2048=$(grep "^2048," cache_results.csv | cut -d',' -f2)
IPS_4096=$(grep "^4096," cache_results.csv | cut -d',' -f2)

GAIN_1024_TO_2048=$(echo "scale=2; ($IPS_2048 - $IPS_1024) * 100 / $IPS_1024" | bc -l)
GAIN_2048_TO_4096=$(echo "scale=2; ($IPS_4096 - $IPS_2048) * 100 / $IPS_2048" | bc -l)

echo "- Performance gain from 1024 to 2048 entries: +${GAIN_1024_TO_2048}%" >> cache_comparison.md
echo "- Performance gain from 2048 to 4096 entries: +${GAIN_2048_TO_4096}%" >> cache_comparison.md
echo "" >> cache_comparison.md

# Make a recommendation based on performance improvements
if [ "$(echo "$GAIN_2048_TO_4096 > 2.0" | bc -l)" -eq 1 ]; then
    echo "**Recommendation**: Use a 4096-entry instruction cache for optimal performance." >> cache_comparison.md
    RECOMMENDED_SIZE=4096
elif [ "$(echo "$GAIN_1024_TO_2048 > 2.0" | bc -l)" -eq 1 ]; then
    echo "**Recommendation**: Use a 2048-entry instruction cache for the best balance of performance and memory usage." >> cache_comparison.md
    RECOMMENDED_SIZE=2048
else
    echo "**Recommendation**: The 1024-entry cache provides adequate performance. Increasing cache size shows minimal gains." >> cache_comparison.md
    RECOMMENDED_SIZE=1024
fi

# Update optimization summary
echo "" >> OPTIMIZATION_SUMMARY.md
echo "## Instruction Cache Size Optimization" >> OPTIMIZATION_SUMMARY.md
echo "" >> OPTIMIZATION_SUMMARY.md
echo "After analyzing different instruction cache sizes (1024, 2048, and 4096 entries), we determined that a ${RECOMMENDED_SIZE}-entry cache provides the best performance characteristics for our workload. See \`cache_comparison.md\` for detailed benchmark results." >> OPTIMIZATION_SUMMARY.md

echo "===== Cache size benchmark complete! ====="
echo "Results saved to cache_comparison.md and appended to OPTIMIZATION_SUMMARY.md"
echo ""
echo "Next steps:"
echo "1. Review the benchmark results in cache_comparison.md"
echo "2. Update the ARM instruction cache size in include/arm_instruction_cache.h to ${RECOMMENDED_SIZE}"
echo "3. Update the TAG_SHIFT value to match (10 for 1024, 11 for 2048, 12 for 4096)"
