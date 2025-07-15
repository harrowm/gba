#ifndef THUMB_EXECUTE_OPTIMIZATIONS_H
#define THUMB_EXECUTE_OPTIMIZATIONS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration for CPU class - defined differently for C vs C++
#ifdef __cplusplus
class CPU;
typedef CPU* CPUPtr;
#else
struct CPU; // In C, treat CPU as an opaque struct
typedef struct CPU* CPUPtr;
#endif

// Fast C implementations of critical Thumb ALU operations
// These are optimized versions of the hottest ALU operations from profiling

// Individual fast-path functions
bool thumb_alu_lsr_fast(CPUPtr cpu, uint8_t rd, uint8_t rs);
bool thumb_alu_lsl_fast(CPUPtr cpu, uint8_t rd, uint8_t rs);
bool thumb_alu_and_fast(CPUPtr cpu, uint8_t rd, uint8_t rs);
bool thumb_alu_eor_fast(CPUPtr cpu, uint8_t rd, uint8_t rs);
bool thumb_add_register_fast(CPUPtr cpu, uint8_t rd, uint8_t rm, uint8_t rn);

// Fast path dispatchers
bool thumb_alu_operations_fast(CPUPtr cpu, uint16_t instruction);
bool thumb_add_register_fast_dispatch(CPUPtr cpu, uint16_t instruction);

#ifdef __cplusplus
}
#endif

#endif
