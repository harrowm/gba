# Latest ARM vs Thumb Benchmark Results

## Test Date: July 10, 2025

## ARM Benchmark Results
```
=== ARM Arithmetic Instruction Benchmark ===
Instruction: ADD R1, R1, R2 (R1 = R1 + R2)
  100000 iterations: 260,342,089 IPS

=== ARM Memory Access Instruction Benchmark ===
Instructions: STR R1, [R0] / LDR R2, [R0] (alternating)
  100000 iterations: 224,961,756 IPS

=== ARM ALU Operation Benchmark ===
Instructions: AND R1, R1, R2 / EOR R1, R1, R2 / LSL R1, R1, R2 (cycling)
  100000 iterations: 260,701,809 IPS

=== ARM Branch Instruction Benchmark ===
Instructions: B #8 (forward branch)
  100000 iterations: 296,568,700 IPS

=== ARM Multiple Data Transfer Benchmark ===
Instructions: LDMIA R0!, {R1-R4} / STMIA R0!, {R1-R4} (alternating)
  100000 iterations: 83,284,750 IPS

=== ARM Multiply Instruction Benchmark ===
Instructions: MUL R1, R2, R3 / MLA R1, R2, R3, R4 (alternating)
  100000 iterations: 363,504,180 IPS
```

## Thumb Benchmark Results
```
=== Thumb Arithmetic Instruction Benchmark ===
Instruction: ADD R1, R1, R2 (R1 = R1 + R2)
  100000 iterations: 438,827,453 IPS

=== Thumb Memory Access Instruction Benchmark ===
Instructions: STR R1, [R0] / LDR R2, [R0] (alternating)
  100000 iterations: 423,908,435 IPS

=== Thumb ALU Operation Benchmark ===
Instructions: AND R1, R2 / EOR R1, R2 / LSL R1, R2 (cycling)
  100000 iterations: 465,744,492 IPS

=== Thumb Branch Instruction Benchmark ===
Instructions: B #2 (short forward branch)
  100000 iterations: 299,392,233 IPS
```

## Performance Comparison Summary

| Instruction Type | ARM IPS | Thumb IPS | Ratio |
|------------------|---------|-----------|-------|
| Arithmetic (ADD) | 260,342,089 | 438,827,453 | **1.69x** |
| Memory (STR/LDR) | 224,961,756 | 423,908,435 | **1.88x** |
| ALU Operations | 260,701,809 | 465,744,492 | **1.79x** |
| Branch Instructions | 296,568,700 | 299,392,233 | **1.01x** |
| Multiple Data Transfer | 83,284,750 | N/A | ARM-only |
| Multiply Instructions | 363,504,180 | N/A | ARM-only |

## Key Findings

### Performance Advantages
- **Thumb is 1.59x faster overall** than ARM
- **Thumb arithmetic operations**: 1.69x faster
- **Thumb memory operations**: 1.88x faster  
- **Thumb ALU operations**: 1.79x faster
- **Branch performance**: Nearly identical (1.01x)

### Code Density Benefits
- Thumb instructions are 16-bit (2 bytes) vs ARM 32-bit (4 bytes)
- **50% smaller code size** improves cache efficiency
- **Reduced memory bandwidth** requirements
- **Better instruction cache utilization**

### ARM Unique Features
- Multiple data transfer instructions (LDM/STM): 83M IPS
- Multiply instructions (MUL/MLA): 363M IPS
- These instruction types are not available in Thumb mode

## Recommendations

1. **Use Thumb mode by default** for better performance and code density
2. **Switch to ARM mode only when needed** for:
   - Multiple data transfer operations (LDM/STM)
   - Multiply operations (MUL/MLA)
   - Specific performance-critical sections requiring ARM-only features

3. **Cache considerations**:
   - Thumb's smaller instruction size improves cache hit rates
   - Better memory bandwidth utilization
   - Significant performance benefit in memory-constrained systems

## Test Environment
- GBA emulator with instruction cache enabled
- Cache statistics disabled for pure performance measurement
- 100,000 iterations for stable timing measurements
- macOS testing environment
