#ifndef ARM_CPU_H
#define ARM_CPU_H

#include <cstdint>
#include "cpu.h"
#include "timing.h"
#include "arm_timing.h"
#include "utility_macros.h"

class CPU; // Forward declaration

class ARMCPU {

public:
    using CondFunc = bool(*)(uint32_t);
    static const CondFunc condTable[16];
    // Must be declared before use in static decode table macros
    bool exception_taken = false;
    CPU& parentCPU; // Reference to the parent CPU

    template <uint32_t hi, uint32_t lo>
    static constexpr uint32_t bits(uint32_t instruction) {
        static_assert(hi >= lo && hi < 32, "Invalid bit range");
        return (instruction >> lo) & ((1 << (hi - lo + 1)) - 1);
    }

    // Cache-aware execution method
    void executeInstruction(uint32_t pc, uint32_t instruction);

public:
    // void updateFlagsSub(uint32_t op1, uint32_t op2, uint32_t result);
    // void updateFlagsAdd(uint32_t op1, uint32_t op2, uint32_t result);
    // void updateFlagsLogical(uint32_t result, uint32_t carry);

    // The functions to update flags need to be inline for speed and hence fully defined
    // in the header file
    FORCE_INLINE void updateFlagsLogical(uint32_t result, uint32_t carry) {
        // N flag: set if result is negative
        uint32_t n = (result >> 31) & 1;
        // Z flag: set if result is zero
        uint32_t z = (result == 0) ? 1 : 0;
        // C flag: use provided carry value (if meaningful for the operation)
        uint32_t cpsr = parentCPU.CPSR();
        cpsr = (cpsr & ~(1u << 31)) | (n << 31); // N
        cpsr = (cpsr & ~(1u << 30)) | (z << 30); // Z
        cpsr = (cpsr & ~(1u << 29)) | ((carry & 1) << 29); // C
        // V flag is not affected by logical ops
        parentCPU.CPSR() = cpsr;
    }

    // Update flags after subtraction: N, Z, C, V
    FORCE_INLINE void updateFlagsSub(uint32_t op1, uint32_t op2, uint32_t result) {
        uint32_t n = (result >> 31) & 1;
        uint32_t z = (result == 0) ? 1 : 0;
        uint32_t c = (op1 >= op2) ? 1 : 0; // Carry: no borrow
        // Overflow: if sign(op1) != sign(op2) and sign(op1) != sign(result)
        uint32_t v = (((op1 ^ op2) & (op1 ^ result)) >> 31) & 1;
        uint32_t cpsr = parentCPU.CPSR();
        cpsr = (cpsr & ~(1u << 31)) | (n << 31); // N
        cpsr = (cpsr & ~(1u << 30)) | (z << 30); // Z
        cpsr = (cpsr & ~(1u << 29)) | (c << 29); // C
        cpsr = (cpsr & ~(1u << 28)) | (v << 28); // V
        parentCPU.CPSR() = cpsr;
    }

    // Update flags after addition: N, Z, C, V
    FORCE_INLINE void updateFlagsAdd(uint32_t op1, uint32_t op2, uint32_t result) {
        uint32_t n = (result >> 31) & 1;
        uint32_t z = (result == 0) ? 1 : 0;
        uint32_t c = (result < op1) ? 1 : 0;
        uint32_t v = (~(op1 ^ op2) & (op1 ^ result) >> 31) & 1;
        uint32_t cpsr = parentCPU.CPSR();
        cpsr = (cpsr & ~(1u << 31)) | (n << 31); // N
        cpsr = (cpsr & ~(1u << 30)) | (z << 30); // Z
        cpsr = (cpsr & ~(1u << 29)) | (c << 29); // C
        cpsr = (cpsr & ~(1u << 28)) | (v << 28); // V
        parentCPU.CPSR() = cpsr;
    }

    // Only update N and Z flags for multiply instructions (preserve C and V)
    FORCE_INLINE void updateFlagsMultiply(uint32_t hi, uint32_t lo) {
        //uint32_t result = (hi == 0) ? lo : hi; // For 32-bit ops, hi==0, lo==result; for 64-bit, hi is high word
        uint32_t cpsr = parentCPU.CPSR();
        // Clear N and Z
        cpsr &= ~((1u << 31) | (1u << 30));
        if (hi == 0 && lo == 0) {
            cpsr |= (1u << 30); // Z
        } else if (hi == 0) {
            if (lo & 0x80000000) cpsr |= (1u << 31); // N
        } else {
            if (hi == 0 && lo == 0) {
                cpsr |= (1u << 30); // Z
            } else if (hi & 0x80000000) {
                cpsr |= (1u << 31); // N
            }
        }
        parentCPU.CPSR() = (parentCPU.CPSR() & ~((1u << 31) | (1u << 30))) | (cpsr & ((1u << 31) | (1u << 30)));
    }


    // ARM shift operations as static inline functions
    // ARM shift operations as static inline functions (now with carry argument)
    inline static uint32_t shift_lsl(uint32_t value, uint32_t shift_val, uint32_t carry) {
        uint32_t result = value << shift_val;
        printf("[DEBUG] shift_lsl: value=0x%08X, shift_val=%u, carry=%u, result=0x%08X\n", value, shift_val, carry, result);
        return result;
    }
    inline static uint32_t shift_lsr(uint32_t value, uint32_t shift_val, uint32_t carry) {
        uint32_t result = shift_val ? (value >> shift_val) : 0;
        printf("[DEBUG] shift_lsr: value=0x%08X, shift_val=%u, carry=%u, result=0x%08X\n", value, shift_val, carry, result);
        return result;
    }
    inline static uint32_t shift_asr(uint32_t value, uint32_t shift_val, uint32_t carry) {
        uint32_t result = shift_val ? ((int32_t)value >> shift_val) : (value & 0x80000000 ? 0xFFFFFFFF : 0);
        printf("[DEBUG] shift_asr: value=0x%08X, shift_val=%u, carry=%u, result=0x%08X\n", value, shift_val, carry, result);
        return result;
    }
    inline static uint32_t shift_ror(uint32_t value, uint32_t shift_val, uint32_t carry) {
        uint32_t result;
        if (shift_val == 0) {
            // RRX: Rotate right with extend (carry in as bit 31)
            result = (carry << 31) | (value >> 1);
            printf("[DEBUG] shift_ror RRX: value=0x%08X, carry=%u, result=0x%08X\n", value, carry, result);
        } else {
            // Standard ROR
            shift_val &= 31;
            result = (value >> shift_val) | (value << (32 - shift_val));
            printf("[DEBUG] shift_ror ROR: value=0x%08X, shift_val=%u, result=0x%08X\n", value, shift_val, result);
        }
        return result;
    }
    using ShiftFunc = uint32_t(*)(uint32_t, uint32_t, uint32_t);
    inline static constexpr ShiftFunc arm_shift[4] = {
        shift_lsl, shift_lsr, shift_asr, shift_ror
    };
   
    void handleException(uint32_t vector_address, uint32_t new_mode, bool disable_irq, bool disable_fiq);
   
public:
    // Condition code functions
    static bool cond_eq(uint32_t flags) { return (flags & 0x4); }
    static bool cond_ne(uint32_t flags) { return !(flags & 0x4); }
    static bool cond_cs(uint32_t flags) { return (flags & 0x2); }
    static bool cond_cc(uint32_t flags) { return !(flags & 0x2); }
    static bool cond_mi(uint32_t flags) { return (flags & 0x8); }
    static bool cond_pl(uint32_t flags) { return !(flags & 0x8); }
    static bool cond_vs(uint32_t flags) { return (flags & 0x1); }
    static bool cond_vc(uint32_t flags) { return !(flags & 0x1); }
    static bool cond_hi(uint32_t flags) { return (flags & 0x2) && !(flags & 0x4); }
    static bool cond_ls(uint32_t flags) { return !(flags & 0x2) || (flags & 0x4); }
    static bool cond_ge(uint32_t flags) { return ((flags & 0x8) >> 3) == (flags & 0x1); }
    static bool cond_lt(uint32_t flags) { return ((flags & 0x8) >> 3) != (flags & 0x1); }
    static bool cond_gt(uint32_t flags) { return !(flags & 0x4) && (((flags & 0x8) >> 3) == (flags & 0x1)); }
    static bool cond_le(uint32_t flags) { return (flags & 0x4) || (((flags & 0x8) >> 3) != (flags & 0x1)); }
    static bool cond_al(uint32_t)        { return true; }
    static bool cond_nv(uint32_t)        { return false; }

private:
    void exec_arm_adc_imm(uint32_t instruction);
    void exec_arm_adc_reg(uint32_t instruction);
    void exec_arm_add_imm(uint32_t instruction);
    void exec_arm_add_reg(uint32_t instruction);
    void exec_arm_and_imm(uint32_t instruction);
    void exec_arm_and_reg(uint32_t instruction);
    void exec_arm_b(uint32_t instruction);
    void exec_arm_bic_imm(uint32_t instruction);
    void exec_arm_bic_reg(uint32_t instruction);
    void exec_arm_bl(uint32_t instruction);
    void exec_arm_cdp(uint32_t instruction);
    void exec_arm_cmn_imm(uint32_t instruction);
    void exec_arm_cmn_reg(uint32_t instruction);
    void exec_arm_cmp_imm(uint32_t instruction);
    void exec_arm_cmp_reg(uint32_t instruction);
    void exec_arm_eor_imm(uint32_t instruction);
    void exec_arm_eor_reg(uint32_t instruction);
    void exec_arm_ldc_imm(uint32_t instruction);
    void exec_arm_ldc_reg(uint32_t instruction);
    void exec_arm_ldm(uint32_t instruction);
    void exec_arm_ldr_imm_post_wb(uint32_t instruction);
    void exec_arm_ldr_imm_pre_nowb(uint32_t instruction);
    void exec_arm_ldr_imm_pre_wb(uint32_t instruction);
    void exec_arm_ldr_reg_post_wb(uint32_t instruction);
    void exec_arm_ldr_reg_pre_nowb(uint32_t instruction);
    void exec_arm_ldr_reg_pre_wb(uint32_t instruction);
    void exec_arm_ldrb_imm_post_wb(uint32_t instruction);
    void exec_arm_ldrb_imm_pre_nowb(uint32_t instruction);
    void exec_arm_ldrb_imm_pre_wb(uint32_t instruction);
    void exec_arm_ldrb_reg_post_wb(uint32_t instruction);
    void exec_arm_ldrb_reg_pre_nowb(uint32_t instruction);
    void exec_arm_ldrb_reg_pre_wb(uint32_t instruction);
    void exec_arm_ldrh_imm_post_wb(uint32_t instruction);
    void exec_arm_ldrh_imm_pre_nowb(uint32_t instruction);
    void exec_arm_ldrh_imm_pre_wb(uint32_t instruction);
    void exec_arm_ldrh_reg_post_wb(uint32_t instruction);
    void exec_arm_ldrh_reg_pre_nowb(uint32_t instruction);
    void exec_arm_ldrh_reg_pre_wb(uint32_t instruction);
    // LDRSB addressing modes
    void exec_arm_ldrsb_reg_pre_wb(uint32_t instruction);
    void exec_arm_ldrsb_reg_pre_nowb(uint32_t instruction);
    void exec_arm_ldrsb_reg_post_wb(uint32_t instruction);
    void exec_arm_ldrsb_imm_pre_wb(uint32_t instruction);
    void exec_arm_ldrsb_imm_pre_nowb(uint32_t instruction);
    void exec_arm_ldrsb_imm_post_wb(uint32_t instruction);
    // LDRSH addressing modes
    void exec_arm_ldrsh_reg_pre_wb(uint32_t instruction);
    void exec_arm_ldrsh_reg_pre_nowb(uint32_t instruction);
    void exec_arm_ldrsh_reg_post_wb(uint32_t instruction);
    void exec_arm_ldrsh_imm_pre_wb(uint32_t instruction);
    void exec_arm_ldrsh_imm_pre_nowb(uint32_t instruction);
    void exec_arm_ldrsh_imm_post_wb(uint32_t instruction);
    void exec_arm_mcr(uint32_t instruction);
    void exec_arm_mla(uint32_t instruction);
    void exec_arm_mov_imm(uint32_t instruction);
    void exec_arm_mov_reg(uint32_t instruction);
    void exec_arm_mrc(uint32_t instruction);
    void exec_arm_mrs(uint32_t instruction);
    void exec_arm_msr_imm(uint32_t instruction);
    void exec_arm_msr_reg(uint32_t instruction);
    void exec_arm_mul(uint32_t instruction);
    void exec_arm_mvn_imm(uint32_t instruction);
    void exec_arm_mvn_reg(uint32_t instruction);
    void exec_arm_orr_imm(uint32_t instruction);
    void exec_arm_orr_reg(uint32_t instruction);
    void exec_arm_rsb_imm(uint32_t instruction);
    void exec_arm_rsb_reg(uint32_t instruction);
    void exec_arm_rsc_imm(uint32_t instruction);
    void exec_arm_rsc_reg(uint32_t instruction);
    void exec_arm_sbc_imm(uint32_t instruction);
    void exec_arm_sbc_reg(uint32_t instruction);
    void exec_arm_smlal(uint32_t instruction);
    void exec_arm_smull(uint32_t instruction);
    void exec_arm_software_interrupt(uint32_t instruction);
    void exec_arm_stc_imm(uint32_t instruction);
    void exec_arm_stc_reg(uint32_t instruction);
    void exec_arm_stm(uint32_t instruction);
    void exec_arm_str_imm_post_wb(uint32_t instruction);
    void exec_arm_str_imm_pre_nowb(uint32_t instruction);
    void exec_arm_str_imm_pre_wb(uint32_t instruction);
    void exec_arm_str_reg_post_wb(uint32_t instruction);
    void exec_arm_str_reg_pre_nowb(uint32_t instruction);
    void exec_arm_str_reg_pre_wb(uint32_t instruction);
    void exec_arm_strb_imm_post_wb(uint32_t instruction);
    void exec_arm_strb_imm_pre_nowb(uint32_t instruction);
    void exec_arm_strb_imm_pre_wb(uint32_t instruction);
    void exec_arm_strb_reg_post_wb(uint32_t instruction);
    void exec_arm_strb_reg_pre_nowb(uint32_t instruction);
    void exec_arm_strb_reg_pre_wb(uint32_t instruction);
    void exec_arm_strh_imm_post_wb(uint32_t instruction);
    void exec_arm_strh_imm_pre_nowb(uint32_t instruction);
    void exec_arm_strh_imm_pre_wb(uint32_t instruction);
    void exec_arm_strh_reg_post_wb(uint32_t instruction);
    void exec_arm_strh_reg_pre_nowb(uint32_t instruction);
    void exec_arm_strh_reg_pre_wb(uint32_t instruction);
    void exec_arm_sub_imm(uint32_t instruction);
    void exec_arm_sub_reg(uint32_t instruction);
    void exec_arm_swp(uint32_t instruction);
    void exec_arm_swpb(uint32_t instruction);
    void exec_arm_teq_imm(uint32_t instruction);
    void exec_arm_teq_reg(uint32_t instruction);
    void exec_arm_tst_imm(uint32_t instruction);
    void exec_arm_tst_reg(uint32_t instruction);
    void exec_arm_umlal(uint32_t instruction);
    void exec_arm_umull(uint32_t instruction);
    void exec_arm_undefined(uint32_t instruction);

    // Set up further decode table and function
    // void exec_arm_further_decode(uint32_t instruction);
    FORCE_INLINE void exec_arm_further_decode(uint32_t instruction) {
        uint8_t index = bits<24,20>(instruction) << 2 | bits<6,5>(instruction);
        DEBUG_LOG("exec_arm_further_decode: index=0x" + DEBUG_TO_HEX_STRING(index, 2));
        (this->*arm_further_decode[index])(instruction);
    }

    using arm_func_t = void (ARMCPU::*)(uint32_t);
    static const arm_func_t arm_further_decode[32 * 4];

    #include "arm_fn_macro.inc" // Macro for static function pointers
    #include "inst_table.inc" // Include the generated instruction table

public:
    explicit ARMCPU(CPU& cpu);
    ~ARMCPU();

    void execute(uint32_t cycles);
    void executeWithTiming(uint32_t cycles, TimingState* timing);
    uint32_t calculateInstructionCycles(uint32_t instruction);    
};

#endif
