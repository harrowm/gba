#ifndef ARM_CPU_H
#define ARM_CPU_H

#include <cstdint>
#include "cpu.h"
#include "timing.h"
#include "arm_timing.h"
#include "utility_macros.h"
#include "arm_instruction_cache.h"

/**
 * ARM CPU Optimizations:
 * - Instruction cache for eliminating redundant decode operations
 * - Function pointer table for fast opcode dispatch
 * - Fast paths for common instructions (MOV, ADD, SUB, CMP)
 * - FORCE_INLINE for critical functions
 * - Optimized calculateOperand2 with fast paths for common cases
 * - Optimized flag updates to minimize register reads/writes
 * - Reduced debug overhead with lazy evaluation
 */

class CPU; // Forward declaration

class ARMCPU {
private:
    CPU& parentCPU; // Reference to the parent CPU
    ARMInstructionCache instruction_cache; // Instruction decode cache
    
    // Instruction execution functions
    void executeInstruction(uint32_t instruction);
    
    // Cached instruction execution functions - optimized for cache hits
    void executeCachedDataProcessing(const ARMCachedInstruction& cached);
    void executeCachedSingleDataTransfer(const ARMCachedInstruction& cached);
    void executeCachedBranch(const ARMCachedInstruction& cached);
    FORCE_INLINE void executeCachedBlockDataTransfer(const ARMCachedInstruction& cached);
    void executeCachedMultiply(const ARMCachedInstruction& cached);
    void executeCachedBX(const ARMCachedInstruction& cached);
    void executeCachedSoftwareInterrupt(const ARMCachedInstruction& cached);
    void executeCachedPSRTransfer(const ARMCachedInstruction& cached);
    void executeCachedCoprocessor(const ARMCachedInstruction& cached);
    
    // Instruction decoding functions
    ARMCachedInstruction decodeInstruction(uint32_t pc, uint32_t instruction);
    ARMCachedInstruction decodeDataProcessing(uint32_t pc, uint32_t instruction);
    ARMCachedInstruction decodeSingleDataTransfer(uint32_t pc, uint32_t instruction);
    ARMCachedInstruction decodeBranch(uint32_t pc, uint32_t instruction);
    ARMCachedInstruction decodeBlockDataTransfer(uint32_t pc, uint32_t instruction);
    
    // Cache-aware execution method
    bool executeWithCache(uint32_t pc, uint32_t instruction);
    
    // Instruction handlers by category
    void arm_data_processing(uint32_t instruction);
    void arm_multiply(uint32_t instruction);
    void arm_bx(uint32_t instruction);
    void arm_single_data_transfer(uint32_t instruction);
    void arm_block_data_transfer(uint32_t instruction);
    void arm_branch(uint32_t instruction);
    void arm_software_interrupt(uint32_t instruction);
    void arm_psr_transfer(uint32_t instruction);
    void arm_coprocessor_operation(uint32_t instruction);
    void arm_coprocessor_transfer(uint32_t instruction);
    void arm_coprocessor_register(uint32_t instruction);
    void arm_undefined(uint32_t instruction);
    
    // Data processing operation handlers - critical ones marked as FORCE_INLINE for optimization
    void arm_and(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_eor(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    FORCE_INLINE void arm_sub(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_rsb(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    FORCE_INLINE void arm_add(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_adc(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_sbc(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_rsc(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_tst(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_teq(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    FORCE_INLINE void arm_cmp(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_cmn(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_orr(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    FORCE_INLINE void arm_mov(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_bic(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_mvn(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    
    // Fast-path ALU operations for function pointer dispatch optimization
    FORCE_INLINE void fastALU_ADD(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry);
    FORCE_INLINE void fastALU_SUB(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry);
    FORCE_INLINE void fastALU_MOV(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry);
    FORCE_INLINE void fastALU_ORR(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry);
    FORCE_INLINE void fastALU_AND(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry);
    FORCE_INLINE void fastALU_CMP(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry);
    
    // Helper functions - critical ones marked as FORCE_INLINE for optimization
    FORCE_INLINE uint32_t calculateOperand2(uint32_t instruction, uint32_t* carry_out);
    uint32_t calculateOperand2Advanced(uint32_t instruction, uint32_t* carry_out, uint32_t* cycles);
    uint32_t arm_apply_shift(uint32_t value, uint32_t shift_type, uint32_t shift_amount, uint32_t* carry_out);
    FORCE_INLINE void updateFlags(uint32_t result, bool carry, bool overflow);
    FORCE_INLINE void updateFlagsLogical(uint32_t result, uint32_t carry_out);
    bool checkCondition(uint32_t instruction);
    FORCE_INLINE bool checkConditionCached(uint8_t condition);
    FORCE_INLINE void executeCachedInstruction(const ARMCachedInstruction& cached);
    
    // Exception and mode handling
    void handleException(uint32_t vector_address, uint32_t new_mode, bool disable_irq, bool disable_fiq);
    bool isPrivilegedMode();
    void switchToMode(uint32_t new_mode);
    bool checkMemoryAccess(uint32_t address, bool is_write, bool is_privileged);
    
    // Instruction decoding table
    static constexpr void (ARMCPU::*arm_instruction_table[8])(uint32_t) = {
        &ARMCPU::arm_data_processing,      // 000: Data processing/multiply
        &ARMCPU::arm_data_processing,      // 001: Data processing/misc
        &ARMCPU::arm_single_data_transfer, // 010: Single data transfer
        &ARMCPU::arm_single_data_transfer, // 011: Single data transfer (undefined)
        &ARMCPU::arm_block_data_transfer,  // 100: Block data transfer
        &ARMCPU::arm_branch,               // 101: Branch/Branch with link
        &ARMCPU::arm_coprocessor_operation,// 110: Coprocessor
        &ARMCPU::arm_software_interrupt    // 111: Coprocessor/SWI
    };

public:
    explicit ARMCPU(CPU& cpu);
    ~ARMCPU();

    void execute(uint32_t cycles);
    void executeWithTiming(uint32_t cycles, TimingState* timing);
    uint32_t calculateInstructionCycles(uint32_t instruction);
    
    // Public interface for testing
    bool decodeAndExecute(uint32_t instruction);
    
    // Cache management functions
    void invalidateInstructionCache() { instruction_cache.clear(); }
    void invalidateInstructionCacheRange(uint32_t start_addr, uint32_t end_addr) {
        instruction_cache.invalidate_range(start_addr, end_addr);
    }
    void invalidateInstructionCache(uint32_t start_addr, uint32_t end_addr) {
        instruction_cache.invalidate_range(start_addr, end_addr);
    }
    ARMInstructionCache::CacheStats getInstructionCacheStats() const {
        return instruction_cache.getStats();
    }
    void resetInstructionCacheStats() { instruction_cache.resetStats(); }
};

#endif
