#ifndef ARM_CPU_H
#define ARM_CPU_H

#include <cstdint>
#include "cpu.h"
#include "timing.h"
#include "arm_timing.h"
#include "utility_macros.h"

class CPU; // Forward declaration

class ARMCPU {
private:
    CPU& parentCPU; // Reference to the parent CPU
    
    // Instruction execution functions
    void executeInstruction(uint32_t instruction);
    
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
    
    // Data processing operation handlers
    void arm_and(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_eor(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_sub(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_rsb(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_add(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_adc(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_sbc(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_rsc(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_tst(uint32_t rn, uint32_t operand2, uint32_t carry_out);
    void arm_teq(uint32_t rn, uint32_t operand2, uint32_t carry_out);
    void arm_cmp(uint32_t rn, uint32_t operand2, uint32_t carry_out);
    void arm_cmn(uint32_t rn, uint32_t operand2, uint32_t carry_out);
    void arm_orr(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_mov(uint32_t rd, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_bic(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    void arm_mvn(uint32_t rd, uint32_t operand2, bool set_flags, uint32_t carry_out);
    
    // Helper functions
    uint32_t calculateOperand2(uint32_t instruction, uint32_t* carry_out);
    uint32_t calculateOperand2Advanced(uint32_t instruction, uint32_t* carry_out, uint32_t* cycles);
    uint32_t arm_apply_shift(uint32_t value, uint32_t shift_type, uint32_t shift_amount, uint32_t* carry_out);
    void updateFlags(uint32_t result, bool carry, bool overflow);
    void updateFlagsLogical(uint32_t result, uint32_t carry_out);
    bool checkCondition(uint32_t instruction);
    
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
};

#endif
