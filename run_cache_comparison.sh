#!/bin/bash

# This script runs the instruction cache size comparison benchmark and saves the results

# Reset cache stats and ensure cache is properly enabled
echo "=== Setting up and running cache size comparison benchmark ==="

# Make sure the ALU benchmark targets are up to date
make clean
make alu_benchmark_1024 alu_benchmark_2048 alu_benchmark_4096

# Run the cache comparison and generate the report
make run-cache-comparison

# Append results to the OPTIMIZATION_SUMMARY.md file
echo "" >> OPTIMIZATION_SUMMARY.md
echo "## Instruction Cache Size Performance Comparison" >> OPTIMIZATION_SUMMARY.md
echo "" >> OPTIMIZATION_SUMMARY.md
echo "Testing different instruction cache sizes to find the optimal configuration:" >> OPTIMIZATION_SUMMARY.md
echo "" >> OPTIMIZATION_SUMMARY.md
echo "| Cache Size | Average IPS | Hit Rate | Recommendation |" >> OPTIMIZATION_SUMMARY.md
echo "|------------|-------------|----------|----------------|" >> OPTIMIZATION_SUMMARY.md

# Extract average IPS and hit rate from the comparison results
AVG_IPS_1024=$(grep -A 7 "1024-Entry Cache" cache_comparison.md | grep -oE "[0-9]+ IPS" | awk '{sum+=$1} END {print sum/NR}')
HIT_RATE_1024=$(grep -A 20 "1024-Entry Cache" cache_comparison.md | grep "Cache hit rate" | grep -oE "[0-9]+\.[0-9]+%" | head -1)

AVG_IPS_2048=$(grep -A 7 "2048-Entry Cache" cache_comparison.md | grep -oE "[0-9]+ IPS" | awk '{sum+=$1} END {print sum/NR}')
HIT_RATE_2048=$(grep -A 20 "2048-Entry Cache" cache_comparison.md | grep "Cache hit rate" | grep -oE "[0-9]+\.[0-9]+%" | head -1)

AVG_IPS_4096=$(grep -A 7 "4096-Entry Cache" cache_comparison.md | grep -oE "[0-9]+ IPS" | awk '{sum+=$1} END {print sum/NR}')
HIT_RATE_4096=$(grep -A 20 "4096-Entry Cache" cache_comparison.md | grep "Cache hit rate" | grep -oE "[0-9]+\.[0-9]+%" | head -1)

# Add recommendations based on IPS
if (( $(echo "$AVG_IPS_4096 > $AVG_IPS_2048" | bc -l) && $(echo "$AVG_IPS_4096 > $AVG_IPS_1024" | bc -l) )); then
    RECOMMENDATION_4096="BEST PERFORMANCE"
    RECOMMENDATION_2048="Good balance"
    RECOMMENDATION_1024="Baseline"
elif (( $(echo "$AVG_IPS_2048 > $AVG_IPS_4096" | bc -l) && $(echo "$AVG_IPS_2048 > $AVG_IPS_1024" | bc -l) )); then
    RECOMMENDATION_4096="Diminishing returns"
    RECOMMENDATION_2048="BEST PERFORMANCE"
    RECOMMENDATION_1024="Baseline" 
else
    RECOMMENDATION_4096="No benefit"
    RECOMMENDATION_2048="No benefit"
    RECOMMENDATION_1024="BEST PERFORMANCE"
fi

echo "| 1024 | ${AVG_IPS_1024} | ${HIT_RATE_1024} | ${RECOMMENDATION_1024} |" >> OPTIMIZATION_SUMMARY.md
echo "| 2048 | ${AVG_IPS_2048} | ${HIT_RATE_2048} | ${RECOMMENDATION_2048} |" >> OPTIMIZATION_SUMMARY.md
echo "| 4096 | ${AVG_IPS_4096} | ${HIT_RATE_4096} | ${RECOMMENDATION_4096} |" >> OPTIMIZATION_SUMMARY.md

echo "" >> OPTIMIZATION_SUMMARY.md
echo "Based on these results, we recommend using a cache size of [TBD] entries for optimal performance." >> OPTIMIZATION_SUMMARY.md
echo "" >> OPTIMIZATION_SUMMARY.md

echo "=== Benchmark complete! ==="
echo "Results saved to cache_comparison.md and appended to OPTIMIZATION_SUMMARY.md"
