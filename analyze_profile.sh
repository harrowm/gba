#!/bin/bash
echo "=== Top 20 Functions by CPU Time ==="
pprof --text arm_benchmark_prof arm_benchmark.prof | grep -v "can't open file" | head -25

echo -e "\n=== Functions Related to Memory Access ==="
pprof --text arm_benchmark_prof arm_benchmark.prof | grep -v "can't open file" | grep -E 'Memory::|mapAddress|read|write'

echo -e "\n=== Functions Related to ARM CPU ==="
pprof --text arm_benchmark_prof arm_benchmark.prof | grep -v "can't open file" | grep -E 'ARMCPU::|CPU::|decode|execute|arm_data|instruction'

echo -e "\n=== String and Debug Functions ==="
pprof --text arm_benchmark_prof arm_benchmark.prof | grep -v "can't open file" | grep -E 'Debug::|String|toHexString|toString'
