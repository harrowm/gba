
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
 * - Optimized flag updates to minimize register reads/writes
 */

class CPU; // Forward declaration

class ARMCPU {

public:
    // Must be declared before use in static decode table macros
    bool exception_taken = false;
    CPU& parentCPU; // Reference to the parent CPU
    ARMInstructionCache instruction_cache; // Instruction decode cache

    template <uint32_t hi, uint32_t lo>
    static constexpr uint32_t bits(uint32_t instruction) {
        static_assert(hi >= lo && hi < 32, "Invalid bit range");
        return (instruction >> lo) & ((1 << (hi - lo + 1)) - 1);
    }

    // Instruction execution functions
    void executeInstruction(uint32_t instruction);
    
    // Cache-aware execution method
    bool executeWithCache(uint32_t pc, uint32_t instruction);

    void arm_software_interrupt(uint32_t instruction);
    void arm_coprocessor_operation(uint32_t instruction);
    void arm_coprocessor_transfer(uint32_t instruction);
    void arm_coprocessor_register(uint32_t instruction);
    void arm_undefined(uint32_t instruction);

public:
    // Clear the instruction cache (for testing)
    void clearInstructionCache() { instruction_cache.clear(); }
    
    // Helper functions - critical ones marked as FORCE_INLINE for optimization
    FORCE_INLINE uint32_t execOperand2imm(uint32_t imm, uint8_t rotate, uint32_t* carry_out);
    FORCE_INLINE uint32_t calcShiftedResult(uint8_t rm, uint8_t rs, uint8_t shift_type, bool reg_shift, uint32_t* carry_out);

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
    ARMCachedInstruction decodeInstruction(uint32_t pc, uint32_t instruction);
    
    // ARM7TDMI instruction decode table using bits 27-20 and a check to see if 
    // bits 7-4 are "1001" (9 bits total).  This avoids any secondary decoding

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
    void decode_arm_ldc_imm(ARMCachedInstruction& decoded);
    void decode_arm_ldc_reg(ARMCachedInstruction& decoded);
    void decode_arm_stc_imm(ARMCachedInstruction& decoded);
    void decode_arm_stc_reg(ARMCachedInstruction& decoded);

    // The following code is generated by inst_table.py
    // Helper macros for cleaner instruction table initialization
    #define ARM_HANDLER(func) &ARMCPU::func
    #define REPEAT_2(handler) handler, handler
    #define REPEAT_4(handler) handler, handler, handler, handler
    #define REPEAT_8(handler) handler, handler, handler, handler, handler, handler, handler, handler
    #define REPEAT_16(handler) REPEAT_8(handler), REPEAT_8(handler)
    #define REPEAT_32(handler) REPEAT_16(handler), REPEAT_16(handler)
    #define REPEAT_64(handler) REPEAT_32(handler), REPEAT_32(handler)
    #define REPEAT_128(handler) REPEAT_64(handler), REPEAT_64(handler)
    #define REPEAT_ALT(a, b) a, b, a, b
    #define REPEAT_2ALT(a, b) a, a, b, b, a, a, b, b

    // Table of 512 entries indexed by bits 27-19 (9 bits)
    static constexpr void (ARMCPU::*arm_decode_table[512])(ARMCachedInstruction& decoded) = {
        ARM_HANDLER(decode_arm_and_reg),            // 0x000: decode_arm_and_reg
        ARM_HANDLER(decode_arm_mul),                // 0x001: decode_arm_mul
        ARM_HANDLER(decode_arm_and_reg),            // 0x002: decode_arm_and_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x003: decode_arm_undefined
        ARM_HANDLER(decode_arm_eor_reg),            // 0x004: decode_arm_eor_reg
        ARM_HANDLER(decode_arm_mla),                // 0x005: decode_arm_mla
        ARM_HANDLER(decode_arm_eor_reg),            // 0x006: decode_arm_eor_reg
        REPEAT_ALT(ARM_HANDLER(decode_arm_undefined), ARM_HANDLER(decode_arm_sub_reg)), // 0x007-0x00A: decode_arm_undefined/decode_arm_sub_reg ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_undefined), ARM_HANDLER(decode_arm_rsb_reg)), // 0x00B-0x00E: decode_arm_undefined/decode_arm_rsb_reg ABAB
        ARM_HANDLER(decode_arm_undefined),          // 0x00F: decode_arm_undefined
        REPEAT_ALT(ARM_HANDLER(decode_arm_add_reg), ARM_HANDLER(decode_arm_umull)), // 0x010-0x013: decode_arm_add_reg/decode_arm_umull ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_adc_reg), ARM_HANDLER(decode_arm_umlal)), // 0x014-0x017: decode_arm_adc_reg/decode_arm_umlal ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_sbc_reg), ARM_HANDLER(decode_arm_smull)), // 0x018-0x01B: decode_arm_sbc_reg/decode_arm_smull ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_rsc_reg), ARM_HANDLER(decode_arm_smlal)), // 0x01C-0x01F: decode_arm_rsc_reg/decode_arm_smlal ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_tst_reg), ARM_HANDLER(decode_arm_swp)), // 0x020-0x023: decode_arm_tst_reg/decode_arm_swp ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_teq_reg), ARM_HANDLER(decode_arm_swp)), // 0x024-0x027: decode_arm_teq_reg/decode_arm_swp ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_cmp_reg), ARM_HANDLER(decode_arm_swpb)), // 0x028-0x02B: decode_arm_cmp_reg/decode_arm_swpb ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_cmn_reg), ARM_HANDLER(decode_arm_swpb)), // 0x02C-0x02F: decode_arm_cmn_reg/decode_arm_swpb ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_orr_reg), ARM_HANDLER(decode_arm_undefined)), // 0x030-0x033: decode_arm_orr_reg/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_mov_reg), ARM_HANDLER(decode_arm_undefined)), // 0x034-0x037: decode_arm_mov_reg/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_bic_reg), ARM_HANDLER(decode_arm_undefined)), // 0x038-0x03B: decode_arm_bic_reg/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_mvn_reg), ARM_HANDLER(decode_arm_undefined)), // 0x03C-0x03F: decode_arm_mvn_reg/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_and_imm), ARM_HANDLER(decode_arm_undefined)), // 0x040-0x043: decode_arm_and_imm/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_eor_imm), ARM_HANDLER(decode_arm_undefined)), // 0x044-0x047: decode_arm_eor_imm/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_sub_imm), ARM_HANDLER(decode_arm_undefined)), // 0x048-0x04B: decode_arm_sub_imm/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_rsb_imm), ARM_HANDLER(decode_arm_undefined)), // 0x04C-0x04F: decode_arm_rsb_imm/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_add_imm), ARM_HANDLER(decode_arm_undefined)), // 0x050-0x053: decode_arm_add_imm/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_adc_imm), ARM_HANDLER(decode_arm_undefined)), // 0x054-0x057: decode_arm_adc_imm/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_sbc_imm), ARM_HANDLER(decode_arm_undefined)), // 0x058-0x05B: decode_arm_sbc_imm/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_rsc_imm), ARM_HANDLER(decode_arm_undefined)), // 0x05C-0x05F: decode_arm_rsc_imm/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_tst_reg), ARM_HANDLER(decode_arm_undefined)), // 0x060-0x063: decode_arm_tst_reg/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_teq_reg), ARM_HANDLER(decode_arm_undefined)), // 0x064-0x067: decode_arm_teq_reg/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_cmp_reg), ARM_HANDLER(decode_arm_undefined)), // 0x068-0x06B: decode_arm_cmp_reg/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_cmn_reg), ARM_HANDLER(decode_arm_undefined)), // 0x06C-0x06F: decode_arm_cmn_reg/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_orr_imm), ARM_HANDLER(decode_arm_undefined)), // 0x070-0x073: decode_arm_orr_imm/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_mov_imm), ARM_HANDLER(decode_arm_undefined)), // 0x074-0x077: decode_arm_mov_imm/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_bic_imm), ARM_HANDLER(decode_arm_undefined)), // 0x078-0x07B: decode_arm_bic_imm/decode_arm_undefined ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_mvn_imm), ARM_HANDLER(decode_arm_undefined)), // 0x07C-0x07F: decode_arm_mvn_imm/decode_arm_undefined ABAB
        ARM_HANDLER(decode_arm_str_reg),            // 0x080: decode_arm_str_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x081: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_reg),            // 0x082: decode_arm_ldr_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x083: decode_arm_undefined
        ARM_HANDLER(decode_arm_str_reg),            // 0x084: decode_arm_str_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x085: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_reg),            // 0x086: decode_arm_ldr_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x087: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_reg),           // 0x088: decode_arm_strb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x089: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_reg),           // 0x08A: decode_arm_ldrb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x08B: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_reg),           // 0x08C: decode_arm_strb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x08D: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_reg),           // 0x08E: decode_arm_ldrb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x08F: decode_arm_undefined
        ARM_HANDLER(decode_arm_str_reg),            // 0x090: decode_arm_str_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x091: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_reg),            // 0x092: decode_arm_ldr_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x093: decode_arm_undefined
        ARM_HANDLER(decode_arm_str_reg),            // 0x094: decode_arm_str_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x095: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_reg),            // 0x096: decode_arm_ldr_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x097: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_reg),           // 0x098: decode_arm_strb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x099: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_reg),           // 0x09A: decode_arm_ldrb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x09B: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_reg),           // 0x09C: decode_arm_strb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x09D: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_reg),           // 0x09E: decode_arm_ldrb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x09F: decode_arm_undefined
        ARM_HANDLER(decode_arm_str_reg),            // 0x0A0: decode_arm_str_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0A1: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_reg),            // 0x0A2: decode_arm_ldr_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0A3: decode_arm_undefined
        ARM_HANDLER(decode_arm_str_reg),            // 0x0A4: decode_arm_str_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0A5: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_reg),            // 0x0A6: decode_arm_ldr_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0A7: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_reg),           // 0x0A8: decode_arm_strb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0A9: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_reg),           // 0x0AA: decode_arm_ldrb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0AB: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_reg),           // 0x0AC: decode_arm_strb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0AD: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_reg),           // 0x0AE: decode_arm_ldrb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0AF: decode_arm_undefined
        ARM_HANDLER(decode_arm_str_reg),            // 0x0B0: decode_arm_str_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0B1: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_reg),            // 0x0B2: decode_arm_ldr_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0B3: decode_arm_undefined
        ARM_HANDLER(decode_arm_str_reg),            // 0x0B4: decode_arm_str_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0B5: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_reg),            // 0x0B6: decode_arm_ldr_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0B7: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_reg),           // 0x0B8: decode_arm_strb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0B9: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_reg),           // 0x0BA: decode_arm_ldrb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0BB: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_reg),           // 0x0BC: decode_arm_strb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0BD: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_reg),           // 0x0BE: decode_arm_ldrb_reg
        ARM_HANDLER(decode_arm_undefined),          // 0x0BF: decode_arm_undefined
        ARM_HANDLER(decode_arm_str_imm),            // 0x0C0: decode_arm_str_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0C1: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_imm),            // 0x0C2: decode_arm_ldr_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0C3: decode_arm_undefined
        ARM_HANDLER(decode_arm_str_imm),            // 0x0C4: decode_arm_str_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0C5: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_imm),            // 0x0C6: decode_arm_ldr_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0C7: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_imm),           // 0x0C8: decode_arm_strb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0C9: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_imm),           // 0x0CA: decode_arm_ldrb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0CB: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_imm),           // 0x0CC: decode_arm_strb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0CD: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_imm),           // 0x0CE: decode_arm_ldrb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0CF: decode_arm_undefined
        ARM_HANDLER(decode_arm_str_imm),            // 0x0D0: decode_arm_str_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0D1: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_imm),            // 0x0D2: decode_arm_ldr_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0D3: decode_arm_undefined
        ARM_HANDLER(decode_arm_str_imm),            // 0x0D4: decode_arm_str_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0D5: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_imm),            // 0x0D6: decode_arm_ldr_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0D7: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_imm),           // 0x0D8: decode_arm_strb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0D9: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_imm),           // 0x0DA: decode_arm_ldrb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0DB: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_imm),           // 0x0DC: decode_arm_strb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0DD: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_imm),           // 0x0DE: decode_arm_ldrb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0DF: decode_arm_undefined
        ARM_HANDLER(decode_arm_str_imm),            // 0x0E0: decode_arm_str_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0E1: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_imm),            // 0x0E2: decode_arm_ldr_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0E3: decode_arm_undefined
        ARM_HANDLER(decode_arm_str_imm),            // 0x0E4: decode_arm_str_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0E5: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_imm),            // 0x0E6: decode_arm_ldr_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0E7: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_imm),           // 0x0E8: decode_arm_strb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0E9: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_imm),           // 0x0EA: decode_arm_ldrb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0EB: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_imm),           // 0x0EC: decode_arm_strb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0ED: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_imm),           // 0x0EE: decode_arm_ldrb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0EF: decode_arm_undefined
        ARM_HANDLER(decode_arm_str_imm),            // 0x0F0: decode_arm_str_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0F1: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_imm),            // 0x0F2: decode_arm_ldr_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0F3: decode_arm_undefined
        ARM_HANDLER(decode_arm_str_imm),            // 0x0F4: decode_arm_str_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0F5: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldr_imm),            // 0x0F6: decode_arm_ldr_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0F7: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_imm),           // 0x0F8: decode_arm_strb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0F9: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_imm),           // 0x0FA: decode_arm_ldrb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0FB: decode_arm_undefined
        ARM_HANDLER(decode_arm_strb_imm),           // 0x0FC: decode_arm_strb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0FD: decode_arm_undefined
        ARM_HANDLER(decode_arm_ldrb_imm),           // 0x0FE: decode_arm_ldrb_imm
        ARM_HANDLER(decode_arm_undefined),          // 0x0FF: decode_arm_undefined
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x100-0x103: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x104-0x107: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x108-0x10B: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x10C-0x10F: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x110-0x113: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x114-0x117: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x118-0x11B: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x11C-0x11F: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x120-0x123: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x124-0x127: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x128-0x12B: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x12C-0x12F: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x130-0x133: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x134-0x137: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x138-0x13B: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stm), ARM_HANDLER(decode_arm_ldm)), // 0x13C-0x13F: decode_arm_stm/decode_arm_ldm ABAB
        REPEAT_32(ARM_HANDLER(decode_arm_b)),       // 0x140-0x15F: decode_arm_b
        REPEAT_32(ARM_HANDLER(decode_arm_bl)),      // 0x160-0x17F: decode_arm_bl
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_imm), ARM_HANDLER(decode_arm_ldc_imm)), // 0x180-0x183: decode_arm_stc_imm/decode_arm_ldc_imm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_imm), ARM_HANDLER(decode_arm_ldc_imm)), // 0x184-0x187: decode_arm_stc_imm/decode_arm_ldc_imm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_imm), ARM_HANDLER(decode_arm_ldc_imm)), // 0x188-0x18B: decode_arm_stc_imm/decode_arm_ldc_imm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_imm), ARM_HANDLER(decode_arm_ldc_imm)), // 0x18C-0x18F: decode_arm_stc_imm/decode_arm_ldc_imm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_imm), ARM_HANDLER(decode_arm_ldc_imm)), // 0x190-0x193: decode_arm_stc_imm/decode_arm_ldc_imm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_imm), ARM_HANDLER(decode_arm_ldc_imm)), // 0x194-0x197: decode_arm_stc_imm/decode_arm_ldc_imm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_imm), ARM_HANDLER(decode_arm_ldc_imm)), // 0x198-0x19B: decode_arm_stc_imm/decode_arm_ldc_imm ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_imm), ARM_HANDLER(decode_arm_ldc_imm)), // 0x19C-0x19F: decode_arm_stc_imm/decode_arm_ldc_imm ABAB
        REPEAT_32(ARM_HANDLER(decode_arm_undefined)), // 0x1A0-0x1BF: decode_arm_undefined
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_reg), ARM_HANDLER(decode_arm_ldc_reg)), // 0x1C0-0x1C3: decode_arm_stc_reg/decode_arm_ldc_reg ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_reg), ARM_HANDLER(decode_arm_ldc_reg)), // 0x1C4-0x1C7: decode_arm_stc_reg/decode_arm_ldc_reg ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_reg), ARM_HANDLER(decode_arm_ldc_reg)), // 0x1C8-0x1CB: decode_arm_stc_reg/decode_arm_ldc_reg ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_reg), ARM_HANDLER(decode_arm_ldc_reg)), // 0x1CC-0x1CF: decode_arm_stc_reg/decode_arm_ldc_reg ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_reg), ARM_HANDLER(decode_arm_ldc_reg)), // 0x1D0-0x1D3: decode_arm_stc_reg/decode_arm_ldc_reg ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_reg), ARM_HANDLER(decode_arm_ldc_reg)), // 0x1D4-0x1D7: decode_arm_stc_reg/decode_arm_ldc_reg ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_reg), ARM_HANDLER(decode_arm_ldc_reg)), // 0x1D8-0x1DB: decode_arm_stc_reg/decode_arm_ldc_reg ABAB
        REPEAT_ALT(ARM_HANDLER(decode_arm_stc_reg), ARM_HANDLER(decode_arm_ldc_reg)), // 0x1DC-0x1DF: decode_arm_stc_reg/decode_arm_ldc_reg ABAB
        REPEAT_32(ARM_HANDLER(decode_arm_software_interrupt)), // 0x1E0-0x1FF: decode_arm_software_interrupt
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
    #undef REPEAT_ALT
    #undef REPEAT_2ALT

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
    void execute_arm_bx(ARMCachedInstruction& cached);

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
