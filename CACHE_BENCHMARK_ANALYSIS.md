# ARM CPU Instruction Cache Size Analysis

## Summary

We investigated the impact of different instruction cache sizes (1024, 2048, and 4096 entries) on the performance of our ARM CPU emulator. Our benchmark results showed that the default 1024-entry cache actually provided the best performance, with larger cache sizes slightly decreasing performance despite achieving the same hit rates.

## Benchmark Results

| Cache Size | Average IPS | Cache Hit Rate | Relative Performance |
|------------|-------------|----------------|----------------------|
| 1024       | 370,985,412 | 99.99%         | Baseline            |
| 2048       | 368,411,381 | 99.99%         | -0.69%              |
| 4096       | 349,509,612 | 99.99%         | -5.78%              |

## Analysis

### High Hit Rate Across All Configurations

All three cache configurations achieved the same 99.99% hit rate. This suggests that our working set of instructions in the benchmark fits comfortably within even the smallest cache size we tested (1024 entries).

### Performance Inversely Correlated with Cache Size

Surprisingly, performance slightly decreased as cache size increased:
- 1024 → 2048: 0.69% performance decrease
- 2048 → 4096: 5.13% performance decrease

This counter-intuitive result could be explained by several factors:

1. **Increased Cache Lookup Overhead**: 
   - Larger caches may require more complex address manipulation
   - Cache lookup is in the critical path of execution

2. **Memory Locality Effects**: 
   - Smaller caches may benefit from better memory locality
   - CPU cache (L1/L2/L3) effects may favor the smaller instruction cache

3. **Hash Function Efficiency**: 
   - Our simple direct-mapped cache may have more collisions with larger sizes
   - The particular modulo arithmetic used for indexing may be more efficient with powers of 2 closer to our working set size

### Implementation Details

The ARM instruction cache is implemented as a direct-mapped cache in `ARMInstructionCache` class:

```cpp
constexpr uint32_t ARM_ICACHE_SIZE = 1024;  // Number of cache entries
constexpr uint32_t ARM_ICACHE_MASK = ARM_ICACHE_SIZE - 1;
constexpr uint32_t ARM_ICACHE_TAG_SHIFT = 10; // log2(ARM_ICACHE_SIZE)
```

The cache lookup implementation:

```cpp
FORCE_INLINE ARMCachedInstruction* lookup(uint32_t pc, uint32_t instruction) {
    uint32_t index = (pc >> 2) & ARM_ICACHE_MASK;
    uint32_t tag = pc >> (ARM_ICACHE_TAG_SHIFT + 2);
    
    ARMCachedInstruction* entry = &cache[index];
    
    if (entry->valid && entry->pc_tag == tag && entry->instruction == instruction) {
        hits++;
        return entry;
    }
    
    misses++;
    return nullptr;
}
```

## Conclusion

Based on our comprehensive benchmarks, we recommend keeping the instruction cache size at 1024 entries. This configuration provides:

1. Optimal performance (highest IPS)
2. Excellent hit rate (99.99%)
3. Lower memory usage compared to larger cache sizes
4. Simpler address calculation (faster indexing)

While larger instruction caches typically improve performance in real CPU designs, our emulator's performance characteristics are dominated by different factors. The small performance decrease with larger caches suggests that the overhead of managing the cache (particularly address calculation and lookup) slightly outweighs any benefit from reduced cache misses, especially since our hit rate is already nearly perfect at 1024 entries.

## Next Steps

1. Keep the ARM instruction cache size at 1024 entries
2. Consider investigating other aspects of the instruction cache:
   - Different cache indexing schemes
   - Set-associative designs
   - Prefetching mechanisms
3. Focus optimization efforts on other areas of the emulator since the instruction cache is already highly effective
