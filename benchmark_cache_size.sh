#!/bin/bash

# This script benchmarks the ARM CPU with different instruction cache sizes

echo "=== ARM CPU Instruction Cache Size Comparison ==="

# Function to run benchmark with a specific cache size
run_benchmark() {
    CACHE_SIZE=$1
    TAG_SHIFT=$2
    
    echo "Setting up ${CACHE_SIZE}-entry instruction cache..."
    
    # Backup the original file
    cp include/arm_instruction_cache.h include/arm_instruction_cache.h.bak
    
    # Update the cache size in the header
    sed -i '' "s/constexpr uint32_t ARM_ICACHE_SIZE = [0-9]*;/constexpr uint32_t ARM_ICACHE_SIZE = ${CACHE_SIZE};/" include/arm_instruction_cache.h
    sed -i '' "s/constexpr uint32_t ARM_ICACHE_TAG_SHIFT = [0-9]*;/constexpr uint32_t ARM_ICACHE_TAG_SHIFT = ${TAG_SHIFT};/" include/arm_instruction_cache.h
    sed -i '' "s/#define ARM_CACHE_STATS [0-9]/#define ARM_CACHE_STATS 1/" include/arm_instruction_cache.h
    
    # Compile the ALU benchmark
    echo "Compiling ALU benchmark with ${CACHE_SIZE}-entry instruction cache..."
    make clean >/dev/null
    make alu_benchmark
    
    if [ $? -ne 0 ]; then
        echo "Error compiling ALU benchmark with ${CACHE_SIZE}-entry cache"
        cp include/arm_instruction_cache.h.bak include/arm_instruction_cache.h
        return 1
    fi
    
    echo "Running benchmark with ${CACHE_SIZE}-entry instruction cache..."
    echo "## ${CACHE_SIZE}-Entry Cache" >> cache_comparison.md
    echo '```' >> cache_comparison.md
    
    # Run multiple iterations for consistency
    IPS_SUM=0
    ITERATIONS=3
    HIT_RATE=""
    
    for i in {1..3}; do
        echo "Iteration $i" >> cache_comparison.md
        if [ "$i" == "3" ]; then
            # Capture cache stats on the last run
            ./alu_benchmark --cache-stats >> cache_comparison.md
            # Extract hit rate for summary
            HIT_RATE=$(grep "Cache hit rate" cache_comparison.md | tail -1 | grep -oE "[0-9]+\.[0-9]+%")
        else
            ./alu_benchmark >> cache_comparison.md
        fi
        
        # Extract average IPS for this run
        CURRENT_IPS=$(./alu_benchmark | grep -oE "[0-9]+ IPS" | awk '{sum+=$1} END {print sum/NR}')
        IPS_SUM=$((IPS_SUM + CURRENT_IPS))
        
        echo "" >> cache_comparison.md
    done
    
    echo '```' >> cache_comparison.md
    echo "" >> cache_comparison.md
    
    # Calculate average IPS across iterations
    AVG_IPS=$((IPS_SUM / ITERATIONS))
    echo "Average IPS for ${CACHE_SIZE}-entry cache: ${AVG_IPS}" >> cache_comparison.md
    echo "" >> cache_comparison.md
    
    # Record results to CSV for easy comparison
    echo "${CACHE_SIZE},${AVG_IPS},${HIT_RATE}" >> cache_results.csv
    
    # Restore original file
    cp include/arm_instruction_cache.h.bak include/arm_instruction_cache.h
}

# Create results files
echo "# ARM CPU Instruction Cache Size Comparison" > cache_comparison.md
echo "" >> cache_comparison.md
echo "cache_size,avg_ips,hit_rate" > cache_results.csv

# Run benchmarks with different cache sizes
run_benchmark 1024 10
run_benchmark 2048 11
run_benchmark 4096 12

# Create summary table
echo "## Summary Comparison" >> cache_comparison.md
echo "" >> cache_comparison.md
echo "| Cache Size | Average IPS | Hit Rate | Relative Performance |" >> cache_comparison.md
echo "|------------|-------------|----------|----------------------|" >> cache_comparison.md

# Process the CSV data for the summary table
BASE_IPS=$(grep "^1024," cache_results.csv | cut -d',' -f2)

while IFS=, read -r SIZE IPS HIT_RATE; do
    if [ "$SIZE" != "cache_size" ]; then
        PERF_INCREASE=$(echo "scale=2; (($IPS - $BASE_IPS) * 100) / $BASE_IPS" | bc)
        echo "| $SIZE | $IPS | $HIT_RATE | +${PERF_INCREASE}% |" >> cache_comparison.md
    fi
done < cache_results.csv

echo "" >> cache_comparison.md

# Determine best cache size based on diminishing returns
echo "## Recommendation" >> cache_comparison.md
echo "" >> cache_comparison.md

IPS_1024=$(grep "^1024," cache_results.csv | cut -d',' -f2)
IPS_2048=$(grep "^2048," cache_results.csv | cut -d',' -f2)
IPS_4096=$(grep "^4096," cache_results.csv | cut -d',' -f2)

GAIN_1024_TO_2048=$(echo "scale=2; ($IPS_2048 - $IPS_1024) * 100 / $IPS_1024" | bc)
GAIN_2048_TO_4096=$(echo "scale=2; ($IPS_4096 - $IPS_2048) * 100 / $IPS_2048" | bc)

echo "- Performance gain from 1024 to 2048 entries: +${GAIN_1024_TO_2048}%" >> cache_comparison.md
echo "- Performance gain from 2048 to 4096 entries: +${GAIN_2048_TO_4096}%" >> cache_comparison.md
echo "" >> cache_comparison.md

# Make a recommendation based on the performance gains
if (( $(echo "$GAIN_2048_TO_4096 > 1.0" | bc -l) )); then
    echo "**Recommendation**: Use a 4096-entry instruction cache for optimal performance." >> cache_comparison.md
    RECOMMENDED_SIZE=4096
elif (( $(echo "$GAIN_1024_TO_2048 > 1.0" | bc -l) )); then
    echo "**Recommendation**: Use a 2048-entry instruction cache for the best balance of performance and memory usage." >> cache_comparison.md
    RECOMMENDED_SIZE=2048
else
    echo "**Recommendation**: The 1024-entry cache provides adequate performance. Increasing cache size shows minimal gains." >> cache_comparison.md
    RECOMMENDED_SIZE=1024
fi

# Update the optimization summary with our findings
echo "" >> OPTIMIZATION_SUMMARY.md
echo "## Instruction Cache Size Optimization" >> OPTIMIZATION_SUMMARY.md
echo "" >> OPTIMIZATION_SUMMARY.md
echo "After analyzing different instruction cache sizes, we determined that a ${RECOMMENDED_SIZE}-entry cache provides the best performance characteristics for our workload. See \`cache_comparison.md\` for detailed benchmark results." >> OPTIMIZATION_SUMMARY.md
echo "" >> OPTIMIZATION_SUMMARY.md

echo "===== Cache size benchmarking complete! ====="
echo "Results saved to cache_comparison.md and appended to OPTIMIZATION_SUMMARY.md"
