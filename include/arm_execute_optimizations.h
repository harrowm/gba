#ifndef ARM_EXECUTE_OPTIMIZATIONS_H
#define ARM_EXECUTE_OPTIMIZATIONS_H

#include <stdint.h>
#include <stdbool.h>
#include "arm_execute_phase1.h"
#include "arm_timing.h"

#ifdef __cplusplus
extern "C" {
#endif

// Macro to suppress unused parameter warnings
#define UNUSED(x) (void)(x)

// Function pointer types for instruction handlers
typedef bool (*InstructionHandler)(ArmCPUState*, uint32_t, void*);
typedef bool (*ArmInstructionHandler)(ArmCPUState*, uint32_t, ArmMemoryInterface*);

// Function pointer type for condition checking
typedef bool (*ConditionChecker)(uint32_t);
typedef bool (*ArmConditionChecker)(uint32_t);

// Function pointer type for ALU operations
typedef uint32_t (*ALUOperation)(uint32_t, uint32_t, uint32_t*, ArmCPUState*);

// Structure to hold precomputed condition checker table
extern ArmConditionChecker arm_condition_table[16];

// Structure to hold instruction handler table
extern ArmInstructionHandler arm_instruction_table[16]; // Based on instruction format

// Structure to hold ALU operation table
extern ALUOperation arm_alu_operation_table[16]; // Based on ALU opcode

/**
 * Initialize all function pointer tables for optimization.
 * Must be called before using the optimized execution functions.
 */
void arm_initialize_optimization_tables(void);

/**
 * Execute a single ARM instruction using the optimized function pointer tables.
 * 
 * @param state Pointer to ARM CPU state
 * @param instruction 32-bit ARM instruction to execute
 * @param memory_interface Pointer to memory interface functions
 * @return true if the PC was modified during execution
 */
bool arm_execute_instruction_optimized(ArmCPUState* state, uint32_t instruction, ArmMemoryInterface* memory_interface);

/**
 * Execute ARM data processing instruction using optimized ALU operation table.
 * 
 * @param state Pointer to ARM CPU state
 * @param instruction 32-bit ARM instruction
 * @param memory_interface Pointer to memory interface functions
 * @return true if the PC was modified during execution
 */
bool arm_execute_data_processing_optimized(ArmCPUState* state, uint32_t instruction, ArmMemoryInterface* memory_interface);

/**
 * Determine the instruction format for dispatch table lookups.
 * 
 * @param instruction 32-bit ARM instruction
 * @return Instruction format index for table lookup
 */
uint32_t arm_get_instruction_format(uint32_t instruction);

#ifdef __cplusplus
}
#endif

#endif // ARM_EXECUTE_OPTIMIZATIONS_H
