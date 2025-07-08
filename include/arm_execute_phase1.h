#ifndef ARM_EXECUTE_PHASE1_H
#define ARM_EXECUTE_PHASE1_H

#include <stdint.h>
#include <stdbool.h>
#include "arm_timing.h" // Include for existing macros
#include "utility_macros.h" // Include for UNUSED macro

#ifdef __cplusplus
extern "C" {
#endif

// CPU state structure for ARM execution
typedef struct {
    uint32_t registers[16];
    uint32_t cpsr;
} ArmCPUState;

// CPU flag definitions (matching CPU class)
#define ARM_FLAG_N (1U << 31) // Negative flag
#define ARM_FLAG_Z (1U << 30) // Zero flag
#define ARM_FLAG_C (1U << 29) // Carry flag
#define ARM_FLAG_V (1U << 28) // Overflow flag
#define ARM_FLAG_T (1U << 5)  // Thumb mode flag
#define ARM_FLAG_E (1U << 9)  // Endianness flag

// Memory interface callback function types
typedef uint32_t (*MemoryRead32Func)(void* context, uint32_t address);
typedef void (*MemoryWrite32Func)(void* context, uint32_t address, uint32_t value);
typedef uint16_t (*MemoryRead16Func)(void* context, uint32_t address);
typedef void (*MemoryWrite16Func)(void* context, uint32_t address, uint16_t value);
typedef uint8_t (*MemoryRead8Func)(void* context, uint32_t address);
typedef void (*MemoryWrite8Func)(void* context, uint32_t address, uint8_t value);

// Memory interface structure
typedef struct {
    void* context;
    MemoryRead32Func read32;
    MemoryWrite32Func write32;
    MemoryRead16Func read16;
    MemoryWrite16Func write16;
    MemoryRead8Func read8;
    MemoryWrite8Func write8;
} ArmMemoryInterface;

/**
 * Execute a single ARM instruction.
 * 
 * @param state Pointer to ARM CPU state
 * @param instruction 32-bit ARM instruction to execute
 * @param memory_interface Pointer to memory interface functions
 * @return true if the PC was modified during execution
 */
bool arm_execute_instruction(ArmCPUState* state, uint32_t instruction, void* memory_interface);

/**
 * Execute ARM data processing instruction.
 * 
 * @param state Pointer to ARM CPU state
 * @param instruction 32-bit ARM instruction
 * @param memory_interface Pointer to memory interface functions
 * @return true if the PC was modified during execution
 */
bool arm_execute_data_processing(ArmCPUState* state, uint32_t instruction, void* memory_interface);

/**
 * Calculate operand2 for ARM data processing instructions.
 * 
 * @param state Pointer to ARM CPU state
 * @param instruction 32-bit ARM instruction
 * @param carry_out Pointer to store carry output
 * @return Calculated operand2 value
 */
uint32_t arm_calculate_operand2(ArmCPUState* state, uint32_t instruction, uint32_t* carry_out);

/**
 * Update CPSR flags based on result.
 * 
 * @param state Pointer to ARM CPU state
 * @param result Result value
 * @param carry_out Carry output from operation
 * @param set_flags Whether to set flags
 */
void arm_update_flags(ArmCPUState* state, uint32_t result, uint32_t carry_out, bool set_flags);

/**
 * Update CPSR flags for addition operations.
 * 
 * @param state Pointer to ARM CPU state
 * @param op1 First operand
 * @param op2 Second operand
 * @param result Result value
 * @param set_flags Whether to set flags
 */
void arm_update_flags_add(ArmCPUState* state, uint32_t op1, uint32_t op2, uint32_t result, bool set_flags);

/**
 * Update CPSR flags for subtraction operations.
 * 
 * @param state Pointer to ARM CPU state
 * @param op1 First operand
 * @param op2 Second operand
 * @param result Result value
 * @param set_flags Whether to set flags
 */
void arm_update_flags_sub(ArmCPUState* state, uint32_t op1, uint32_t op2, uint32_t result, bool set_flags);

/**
 * Check if an ARM condition is satisfied based on the CPSR flags.
 * 
 * @param condition ARM condition code (0-15)
 * @param cpsr Current Program Status Register value
 * @return true if the condition is satisfied
 */
bool arm_execute_check_condition(ARMCondition condition, uint32_t cpsr);

/**
 * Execute ARM single data transfer instruction (LDR/STR).
 * 
 * @param state Pointer to ARM CPU state
 * @param instruction 32-bit ARM instruction
 * @param memory_interface Pointer to memory interface functions
 * @return true if the PC was modified during execution
 */
bool arm_execute_single_data_transfer(ArmCPUState* state, uint32_t instruction, ArmMemoryInterface* memory_interface);

/**
 * Execute ARM multiply instruction (MUL/MLA).
 * 
 * @param state Pointer to ARM CPU state
 * @param instruction 32-bit ARM instruction
 * @return true if the PC was modified during execution
 */
bool arm_execute_multiply(ArmCPUState* state, uint32_t instruction);

/**
 * Execute ARM block data transfer instruction (LDM/STM).
 * 
 * @param state Pointer to ARM CPU state
 * @param instruction 32-bit ARM instruction
 * @param memory_interface Pointer to memory interface functions
 * @return true if the PC was modified during execution
 */
bool arm_execute_block_data_transfer(ArmCPUState* state, uint32_t instruction, ArmMemoryInterface* memory_interface);

/**
 * Execute ARM branch instruction (B/BL).
 * 
 * @param state Pointer to ARM CPU state
 * @param instruction 32-bit ARM instruction
 * @return true if the PC was modified during execution
 */
bool arm_execute_branch(ArmCPUState* state, uint32_t instruction);

/**
 * Execute ARM software interrupt instruction (SWI).
 * 
 * @param state Pointer to ARM CPU state
 * @param instruction 32-bit ARM instruction
 * @return true if the PC was modified during execution
 */
bool arm_execute_software_interrupt(ArmCPUState* state, uint32_t instruction);

// Flag bit positions in CPSR
#define FLAG_N 0x80000000 // Negative flag
#define FLAG_Z 0x40000000 // Zero flag
#define FLAG_C 0x20000000 // Carry flag
#define FLAG_V 0x10000000 // Overflow flag

// Additional macros not in arm_timing.h
#define ARM_GET_IMMEDIATE_VALUE(instr) ((instr) & 0xFF)
#define ARM_GET_ROTATE_IMM(instr) (((instr) >> 8) & 0xF)
#define ARM_GET_SHIFT_IMM(instr) (((instr) >> 7) & 0x1F)

// Block data transfer macros
#define ARM_GET_REGISTER_LIST(instr) ((instr) & 0xFFFF)
#define ARM_GET_PRE_INDEX(instr) (((instr) >> 24) & 1)
#define ARM_GET_UP_DOWN(instr) (((instr) >> 23) & 1)
#define ARM_GET_PSR_FORCE_USER(instr) (((instr) >> 22) & 1)
#define ARM_GET_WRITEBACK(instr) (((instr) >> 21) & 1)
#define ARM_GET_LOAD_STORE(instr) (((instr) >> 20) & 1)

// Branch macros
#define ARM_GET_BRANCH_OFFSET(instr) ((instr) & 0xFFFFFF)
#define ARM_GET_LINK_BIT(instr) (((instr) >> 24) & 1)

// Multiply macros
#define ARM_GET_MULTIPLY_ACCUMULATE(instr) (((instr) >> 21) & 1)
#define ARM_GET_MULTIPLY_RS(instr) (((instr) >> 8) & 0xF)

// Software interrupt macros
#define ARM_GET_SWI_NUMBER(instr) ((instr) & 0xFFFFFF)

#ifdef __cplusplus
}
#endif

#endif // ARM_EXECUTE_PHASE1_H
