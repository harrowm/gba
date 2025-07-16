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

public:
    bool exception_taken = false;
    CPU& parentCPU; // Reference to the parent CPU
    ARMInstructionCache instruction_cache; // Instruction decode cache

    template <uint32_t hi, uint32_t lo>
    static constexpr uint32_t bits(uint32_t instruction) {
        static_assert(hi >= lo && hi < 32, "Invalid bit range");
        return ((instruction >> lo) & ((1 << (hi - lo + 1)) - 1));
    }

    // Instruction execution functions
    void executeInstruction(uint32_t instruction);

    // Cached instruction execution functions - optimized for cache hits
    // FORCE_INLINE void executeCachedDataProcessing(const ARMCachedInstruction& cached);
    // void executeCachedSingleDataTransfer(const ARMCachedInstruction& cached);
    // void executeCachedBranch(const ARMCachedInstruction& cached);
    // FORCE_INLINE void executeCachedBlockDataTransfer(const ARMCachedInstruction& cached);
    // void executeCachedMultiply(const ARMCachedInstruction& cached);
    // void executeCachedMultiplyLong(const ARMCachedInstruction& cached);
    // void executeCachedBX(const ARMCachedInstruction& cached);
    // void executeCachedSoftwareInterrupt(const ARMCachedInstruction& cached);
    // void executeCachedPSRTransfer(const ARMCachedInstruction& cached);
    // void executeCachedCoprocessor(const ARMCachedInstruction& cached);

    // Instruction decoding functions
    // ARMCachedInstruction decodeInstruction(uint32_t pc, uint32_t instruction);
    // ARMCachedInstruction decodeDataProcessing(uint32_t pc, uint32_t instruction);
    // ARMCachedInstruction decodeSingleDataTransfer(uint32_t pc, uint32_t instruction);
    // ARMCachedInstruction decodeBranch(uint32_t pc, uint32_t instruction);
    // ARMCachedInstruction decodeBlockDataTransfer(uint32_t pc, uint32_t instruction);

    // Cache-aware execution method
    bool executeWithCache(uint32_t pc, uint32_t instruction);

    void arm_software_interrupt(uint32_t instruction);
    void arm_coprocessor_operation(uint32_t instruction);
    void arm_coprocessor_transfer(uint32_t instruction);
    void arm_coprocessor_register(uint32_t instruction);
    void arm_undefined(uint32_t instruction);

    // Data processing operation handlers - critical ones marked as FORCE_INLINE for optimization
    // void arm_and(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    // void arm_eor(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    // FORCE_INLINE void arm_sub(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    // void arm_rsb(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    // FORCE_INLINE void arm_add(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    // void arm_adc(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    // void arm_sbc(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    // void arm_rsc(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    // void arm_tst(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    // void arm_teq(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    // FORCE_INLINE void arm_cmp(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    // void arm_cmn(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    // void arm_orr(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);

public:
    // Clear the instruction cache (for testing)
    void clearInstructionCache() { instruction_cache.clear(); }
    // ...existing public methods...
    // FORCE_INLINE void arm_mov(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    // void arm_bic(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    // void arm_mvn(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out);
    
    // Fast-path ALU operations for function pointer dispatch optimization
    // FORCE_INLINE void fastALU_ADD(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry);
    // FORCE_INLINE void fastALU_SUB(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry);
    // FORCE_INLINE void fastALU_MOV(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry);
    // FORCE_INLINE void fastALU_ORR(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry);
    // FORCE_INLINE void fastALU_AND(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry);
    // FORCE_INLINE void fastALU_CMP(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry);
    
    // Helper functions - critical ones marked as FORCE_INLINE for optimization
    FORCE_INLINE uint32_t execOperand2imm(uint32_t imm, uint8_t rotate, uint32_t* carry_out);
    FORCE_INLINE uint32_t execOperand2reg(uint8_t rm, uint8_t rs, uint8_t shift_type, bool reg_shift, uint32_t* carry_out);

    // FORCE_INLINE uint32_t calculateOperand2(uint32_t instruction, uint32_t* carry_out);
    uint32_t calculateOperand2Advanced(uint32_t instruction, uint32_t* carry_out, uint32_t* cycles);
    uint32_t arm_apply_shift(uint32_t value, uint32_t shift_type, uint32_t shift_amount, uint32_t* carry_out);
    FORCE_INLINE void updateFlags(uint32_t result, bool carry, bool overflow);
    FORCE_INLINE void updateFlagsLogical(uint32_t result, uint32_t carry_out);
    FORCE_INLINE bool checkConditionCached(uint8_t condition);
    FORCE_INLINE void executeCachedInstruction(const ARMCachedInstruction& cached);
    
    // Exception and mode handling
    void handleException(uint32_t vector_address, uint32_t new_mode, bool disable_irq, bool disable_fiq);
    bool isPrivilegedMode();
    void switchToMode(uint32_t new_mode);
    bool checkMemoryAccess(uint32_t address, bool is_write, bool is_privileged);

private:
    // ARM7TDMI instruction decode table using bits 27-19 (9 bits, providing finer granularity)
    // For groups or ambiguous cases, a group name is used in the comment.

    void decode_arm_and_reg(ARMCachedInstruction& decoded);
    void decode_arm_and_imm(ARMCachedInstruction& decoded);
    void decode_arm_eor_reg(ARMCachedInstruction& decoded);
    void decode_arm_eor_imm(ARMCachedInstruction& decoded);
    void decode_arm_sub_reg(ARMCachedInstruction& decoded);
    void decode_arm_sub_imm(ARMCachedInstruction& decoded);
    void decode_arm_rsb_reg(ARMCachedInstruction& decoded);
    void decode_arm_rsb_imm(ARMCachedInstruction& decoded);
    void decode_arm_add_reg(ARMCachedInstruction& decoded);
    void decode_arm_add_imm(ARMCachedInstruction& decoded);
    void decode_arm_adc_reg(ARMCachedInstruction& decoded);
    void decode_arm_adc_imm(ARMCachedInstruction& decoded);
    void decode_arm_sbc_reg(ARMCachedInstruction& decoded);
    void decode_arm_sbc_imm(ARMCachedInstruction& decoded);
    void decode_arm_rsc_reg(ARMCachedInstruction& decoded);
    void decode_arm_rsc_imm(ARMCachedInstruction& decoded);
    void decode_arm_tst_reg(ARMCachedInstruction& decoded);
    void decode_arm_tst_imm(ARMCachedInstruction& decoded);
    void decode_arm_teq_reg(ARMCachedInstruction& decoded);
    void decode_arm_teq_imm(ARMCachedInstruction& decoded);
    void decode_arm_cmp_reg(ARMCachedInstruction& decoded);
    void decode_arm_cmp_imm(ARMCachedInstruction& decoded);
    void decode_arm_cmn_reg(ARMCachedInstruction& decoded);
    void decode_arm_cmn_imm(ARMCachedInstruction& decoded);
    void decode_arm_orr_reg(ARMCachedInstruction& decoded);
    void decode_arm_orr_imm(ARMCachedInstruction& decoded);
    void decode_arm_mov_reg(ARMCachedInstruction& decoded);
    void decode_arm_mov_imm(ARMCachedInstruction& decoded);
    void decode_arm_bic_reg(ARMCachedInstruction& decoded);
    void decode_arm_bic_imm(ARMCachedInstruction& decoded);
    void decode_arm_mvn_reg(ARMCachedInstruction& decoded);
    void decode_arm_mvn_imm(ARMCachedInstruction& decoded);
    // Single data transfer decoders (word)
    void decode_arm_str_imm(ARMCachedInstruction& decoded);
    void decode_arm_str_reg(ARMCachedInstruction& decoded);
    void decode_arm_ldr_imm(ARMCachedInstruction& decoded);
    void decode_arm_ldr_reg(ARMCachedInstruction& decoded);
    void decode_arm_mul(ARMCachedInstruction& decoded);
    void decode_arm_mla(ARMCachedInstruction& decoded);
    void decode_arm_umull(ARMCachedInstruction& decoded);
    void decode_arm_umlal(ARMCachedInstruction& decoded);
    void decode_arm_smull(ARMCachedInstruction& decoded);
    void decode_arm_smlal(ARMCachedInstruction& decoded);
    void decode_arm_swp(ARMCachedInstruction& decoded);
    void decode_arm_swpb(ARMCachedInstruction& decoded);
    void decode_arm_ldrh(ARMCachedInstruction& decoded);
    void decode_arm_ldrsb(ARMCachedInstruction& decoded);
    void decode_arm_ldrsh(ARMCachedInstruction& decoded);
    void decode_arm_strh(ARMCachedInstruction& decoded);
    void decode_arm_undefined(ARMCachedInstruction& decoded);
    void decode_arm_str(ARMCachedInstruction& decoded);
    void decode_arm_ldr(ARMCachedInstruction& decoded);
    void decode_arm_strb_reg(ARMCachedInstruction& decoded);
    void decode_arm_strb_imm(ARMCachedInstruction& decoded);
    void decode_arm_ldrb_reg(ARMCachedInstruction& decoded);
    void decode_arm_ldrb_imm(ARMCachedInstruction& decoded);
    void decode_arm_stm(ARMCachedInstruction& decoded);
    void decode_arm_ldm(ARMCachedInstruction& decoded);
    void decode_arm_b(ARMCachedInstruction& decoded);
    void decode_arm_bl(ARMCachedInstruction& decoded);
    void decode_arm_cdp(ARMCachedInstruction& decoded);
    void decode_arm_mrc(ARMCachedInstruction& decoded);
    void decode_arm_mcr(ARMCachedInstruction& decoded);
    void decode_arm_software_interrupt(ARMCachedInstruction& decoded);

    // Helper macros for cleaner instruction table initialization
    #define ARM_HANDLER(func) &ARMCPU::func
    #define REPEAT_2(handler) handler, handler
    #define REPEAT_4(handler) handler, handler, handler, handler
    #define REPEAT_8(handler) handler, handler, handler, handler, handler, handler, handler, handler
    #define REPEAT_16(handler) REPEAT_8(handler), REPEAT_8(handler)
    #define REPEAT_32(handler) REPEAT_16(handler), REPEAT_16(handler)
    #define REPEAT_64(handler) REPEAT_32(handler), REPEAT_32(handler)
    #define REPEAT_128(handler) REPEAT_64(handler), REPEAT_64(handler)
    
    #define REPEAT_ALT_4(h1, h2) h1, h2, h1, h2
    
    // Table of 512 entries indexed by bits 27-19 (9 bits)
    static constexpr void (ARMCPU::*arm_decode_table[512])(ARMCachedInstruction& decoded) = {
        // 0x000-0x007: AND (reg, imm alternating)
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_and_reg), ARM_HANDLER(decode_arm_and_imm)),
        // 0x008-0x00F: EOR
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_eor_reg), ARM_HANDLER(decode_arm_eor_imm)),
        // 0x010-0x017: SUB
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_sub_reg), ARM_HANDLER(decode_arm_sub_imm)),
        // 0x018-0x01F: RSB
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_rsb_reg), ARM_HANDLER(decode_arm_rsb_imm)),
        // 0x020-0x027: ADD
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_add_reg), ARM_HANDLER(decode_arm_add_imm)),
        // 0x028-0x02F: ADC
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_adc_reg), ARM_HANDLER(decode_arm_adc_imm)),
        // 0x030-0x037: SBC
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_sbc_reg), ARM_HANDLER(decode_arm_sbc_imm)),
        // 0x038-0x03F: RSC
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_rsc_reg), ARM_HANDLER(decode_arm_rsc_imm)),
        // 0x040-0x047: TST
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_tst_reg), ARM_HANDLER(decode_arm_tst_imm)),
        // 0x048-0x04F: TEQ
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_teq_reg), ARM_HANDLER(decode_arm_teq_imm)),
        // 0x050-0x057: CMP
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_cmp_reg), ARM_HANDLER(decode_arm_cmp_imm)),
        // 0x058-0x05F: CMN
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_cmn_reg), ARM_HANDLER(decode_arm_cmn_imm)),
        // 0x060-0x067: ORR
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_orr_reg), ARM_HANDLER(decode_arm_orr_imm)),
        // 0x068-0x06B: MOV register
        REPEAT_4(ARM_HANDLER(decode_arm_mov_reg)),
        // 0x06C-0x06F: MOV immediate
        REPEAT_4(ARM_HANDLER(decode_arm_mov_imm)),
        // 0x070-0x077: BIC
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_bic_reg), ARM_HANDLER(decode_arm_bic_imm)),
        // 0x078-0x07F: MVN
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_mvn_reg), ARM_HANDLER(decode_arm_mvn_imm)),
        // 0x080 - 0x083: MUL, MLA
        ARM_HANDLER(decode_arm_mul), ARM_HANDLER(decode_arm_mul), ARM_HANDLER(decode_arm_mla), ARM_HANDLER(decode_arm_mla),
        // 0x084 - 0x087: UMULL, UMLAL, SMULL, SMLAL
        ARM_HANDLER(decode_arm_umull), ARM_HANDLER(decode_arm_umlal), ARM_HANDLER(decode_arm_smull), ARM_HANDLER(decode_arm_smlal),
        // 0x088 - 0x089: SWP, SWPB
        ARM_HANDLER(decode_arm_swp), ARM_HANDLER(decode_arm_swpb),
        // 0x08A - 0x08B: Reserved/undefined
        ARM_HANDLER(decode_arm_undefined), ARM_HANDLER(decode_arm_undefined),
        // 0x08C - 0x08F: LDRH, LDRSB, LDRSH, STRH
        ARM_HANDLER(decode_arm_ldrh), ARM_HANDLER(decode_arm_ldrsb), ARM_HANDLER(decode_arm_ldrsh), ARM_HANDLER(decode_arm_strh),
        // 0x090 - 0x0FF: Undefined or reserved
        REPEAT_64(ARM_HANDLER(decode_arm_undefined)), REPEAT_32(ARM_HANDLER(decode_arm_undefined)), REPEAT_16(ARM_HANDLER(decode_arm_undefined)),
        // 0x100 - 0x13F: STR (store word)
        REPEAT_64(ARM_HANDLER(decode_arm_str)),
        // 0x140 - 0x17F: LDR (load word)
        REPEAT_64(ARM_HANDLER(decode_arm_ldr)),
        // 0x180 - 0x19F: STRB (store byte, reg/imm alternating)
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_strb_reg), ARM_HANDLER(decode_arm_strb_imm)), REPEAT_ALT_4(ARM_HANDLER(decode_arm_strb_reg), ARM_HANDLER(decode_arm_strb_imm)),
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_strb_reg), ARM_HANDLER(decode_arm_strb_imm)), REPEAT_ALT_4(ARM_HANDLER(decode_arm_strb_reg), ARM_HANDLER(decode_arm_strb_imm)),
        // 0x1A0 - 0x1DF: LDRB (load byte, reg/imm alternating)
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_ldrb_reg), ARM_HANDLER(decode_arm_ldrb_imm)), REPEAT_ALT_4(ARM_HANDLER(decode_arm_ldrb_reg), ARM_HANDLER(decode_arm_ldrb_imm)),
        REPEAT_ALT_4(ARM_HANDLER(decode_arm_ldrb_reg), ARM_HANDLER(decode_arm_ldrb_imm)), REPEAT_ALT_4(ARM_HANDLER(decode_arm_ldrb_reg), ARM_HANDLER(decode_arm_ldrb_imm)),
        // 0x1E0 - 0x1FF: Reserved/undefined
        REPEAT_16(ARM_HANDLER(decode_arm_undefined)), REPEAT_16(ARM_HANDLER(decode_arm_undefined)),
        // 0x180 - 0x187: STM
        REPEAT_8(ARM_HANDLER(decode_arm_stm)),
        // 0x188 - 0x18F: LDM
        REPEAT_8(ARM_HANDLER(decode_arm_ldm)),
        // 0x1C0 - 0x1DF: Reserved/undefined
        REPEAT_16(ARM_HANDLER(decode_arm_undefined)),
        // 0x1E0 - 0x1FF: Branch (B, BL)
        REPEAT_8(ARM_HANDLER(decode_arm_b)), REPEAT_8(ARM_HANDLER(decode_arm_bl)),
        // 0x200 - 0x2FF: Coprocessor (CDP, MRC, MCR)
        REPEAT_8(ARM_HANDLER(decode_arm_cdp)), REPEAT_8(ARM_HANDLER(decode_arm_mrc)), REPEAT_8(ARM_HANDLER(decode_arm_mcr)),
        // 0x300 - 0x31F: Software Interrupt
        REPEAT_32(ARM_HANDLER(decode_arm_software_interrupt))
    };

    // Cleanup macros to avoid polluting the namespace
    #undef ARM_HANDLER
    #undef REPEAT_2
    #undef REPEAT_4
    #undef REPEAT_8
    #undef REPEAT_16
    #undef REPEAT_32
    #undef REPEAT_64
    #undef REPEAT_128
    #undef REPEAT_ALT_4

        // --- Execute stubs for decode_arm_ functions ---
    void execute_arm_str_imm(ARMCachedInstruction& decoded);
    void execute_arm_str_reg(ARMCachedInstruction& decoded);
    void execute_arm_ldr_imm(ARMCachedInstruction& decoded);
    void execute_arm_ldr_reg(ARMCachedInstruction& decoded);
    void execute_arm_strb_imm(ARMCachedInstruction& decoded);
    void execute_arm_strb_reg(ARMCachedInstruction& decoded);
    void execute_arm_ldrb_imm(ARMCachedInstruction& decoded);
    void execute_arm_ldrb_reg(ARMCachedInstruction& decoded);
    void execute_arm_stm(ARMCachedInstruction& decoded);
    void execute_arm_ldm(ARMCachedInstruction& decoded);
    void execute_arm_b(ARMCachedInstruction& decoded);
    void execute_arm_bl(ARMCachedInstruction& decoded);

    void execute_arm_mul(ARMCachedInstruction& decoded);
    void execute_arm_mla(ARMCachedInstruction& decoded);
    void execute_arm_umull(ARMCachedInstruction& decoded);
    void execute_arm_umlal(ARMCachedInstruction& decoded);
    void execute_arm_smull(ARMCachedInstruction& decoded);
    void execute_arm_smlal(ARMCachedInstruction& decoded);

    void execute_arm_ldrh(ARMCachedInstruction& decoded);
    void execute_arm_strh(ARMCachedInstruction& decoded);
    void execute_arm_ldrsb(ARMCachedInstruction& decoded);
    void execute_arm_ldrsh(ARMCachedInstruction& decoded);

    void execute_arm_swp(ARMCachedInstruction& decoded);
    void execute_arm_swpb(ARMCachedInstruction& decoded);
    void execute_arm_undefined(ARMCachedInstruction& decoded);
    
    void execute_arm_mov_imm(ARMCachedInstruction& decoded);
    void execute_arm_mov_reg(ARMCachedInstruction& decoded);
    // Data processing execute stubs
    void execute_arm_and_imm(ARMCachedInstruction& decoded);
    void execute_arm_and_reg(ARMCachedInstruction& decoded);
    void execute_arm_eor_imm(ARMCachedInstruction& decoded);
    void execute_arm_eor_reg(ARMCachedInstruction& decoded);
    void execute_arm_sub_imm(ARMCachedInstruction& decoded);
    void execute_arm_sub_reg(ARMCachedInstruction& decoded);
    void execute_arm_rsb_imm(ARMCachedInstruction& decoded);
    void execute_arm_rsb_reg(ARMCachedInstruction& decoded);
    void execute_arm_add_imm(ARMCachedInstruction& decoded);
    void execute_arm_add_reg(ARMCachedInstruction& decoded);
    void execute_arm_adc_imm(ARMCachedInstruction& decoded);
    void execute_arm_adc_reg(ARMCachedInstruction& decoded);
    void execute_arm_sbc_imm(ARMCachedInstruction& decoded);
    void execute_arm_sbc_reg(ARMCachedInstruction& decoded);
    void execute_arm_rsc_imm(ARMCachedInstruction& decoded);
    void execute_arm_rsc_reg(ARMCachedInstruction& decoded);
    void execute_arm_tst_imm(ARMCachedInstruction& decoded);
    void execute_arm_tst_reg(ARMCachedInstruction& decoded);
    void execute_arm_teq_imm(ARMCachedInstruction& decoded);
    void execute_arm_teq_reg(ARMCachedInstruction& decoded);
    void execute_arm_cmp_imm(ARMCachedInstruction& decoded);
    void execute_arm_cmp_reg(ARMCachedInstruction& decoded);
    void execute_arm_cmn_imm(ARMCachedInstruction& decoded);
    void execute_arm_cmn_reg(ARMCachedInstruction& decoded);
    void execute_arm_orr_imm(ARMCachedInstruction& decoded);
    void execute_arm_orr_reg(ARMCachedInstruction& decoded);
    void execute_arm_bic_imm(ARMCachedInstruction& decoded);
    void execute_arm_bic_reg(ARMCachedInstruction& decoded);
    void execute_arm_mvn_imm(ARMCachedInstruction& decoded);
    void execute_arm_mvn_reg(ARMCachedInstruction& decoded);

public:
    explicit ARMCPU(CPU& cpu);
    ~ARMCPU();

    void execute(uint32_t cycles);
    void executeWithTiming(uint32_t cycles, TimingState* timing);
    uint32_t calculateInstructionCycles(uint32_t instruction);
    
    
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
