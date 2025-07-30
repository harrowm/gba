
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
    FORCE_INLINE void updateFlagsSub(uint32_t op1, uint32_t op2, uint32_t result);
    FORCE_INLINE void updateFlagsAdd(uint32_t op1, uint32_t op2, uint32_t result);
    FORCE_INLINE void updateFlagsLogical(uint32_t result, uint32_t carry);

    // ARM shift operations as static inline functions
    inline static uint32_t shift_lsl(uint32_t value, uint32_t shift_val) {
        return value << shift_val;
    }
    inline static uint32_t shift_lsr(uint32_t value, uint32_t shift_val) {
        return shift_val ? (value >> shift_val) : 0;
    }
    inline static uint32_t shift_asr(uint32_t value, uint32_t shift_val) {
        return shift_val ? ((int32_t)value >> shift_val) : (value & 0x80000000 ? 0xFFFFFFFF : 0);
    }
    inline static uint32_t shift_ror(uint32_t value, uint32_t shift_val) {
        return shift_val ? ((value >> shift_val) | (value << (32 - shift_val))) : value;
    }
    using ShiftFunc = uint32_t(*)(uint32_t, uint32_t);
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

    using CondFunc = bool(*)(uint32_t);
    static const CondFunc condTable[16];
private:
    // ARM7TDMI instruction decode table using bits 27-20 and a check to see if 
    // bits 7-4 are "1001" (9 bits total).  This avoids any secondary decoding

    void exec_arm_and_reg(uint32_t instruction);
    void exec_arm_and_imm(uint32_t instruction);
    void exec_arm_eor_reg(uint32_t instruction);
    void exec_arm_eor_imm(uint32_t instruction);
    void exec_arm_sub_reg(uint32_t instruction);
    void exec_arm_sub_imm(uint32_t instruction);
    void exec_arm_rsb_reg(uint32_t instruction);
    void exec_arm_rsb_imm(uint32_t instruction);
    void exec_arm_add_reg(uint32_t instruction);
    void exec_arm_add_imm(uint32_t instruction);
    void exec_arm_adc_reg(uint32_t instruction);
    void exec_arm_adc_imm(uint32_t instruction);
    void exec_arm_sbc_reg(uint32_t instruction);
    void exec_arm_sbc_imm(uint32_t instruction);
    void exec_arm_rsc_reg(uint32_t instruction);
    void exec_arm_rsc_imm(uint32_t instruction);
    void exec_arm_tst_reg(uint32_t instruction);
    void exec_arm_tst_imm(uint32_t instruction);
    void exec_arm_teq_reg(uint32_t instruction);
    void exec_arm_teq_imm(uint32_t instruction);
    void exec_arm_cmp_reg(uint32_t instruction);
    void exec_arm_cmp_imm(uint32_t instruction);
    void exec_arm_cmn_reg(uint32_t instruction);
    void exec_arm_cmn_imm(uint32_t instruction);
    void exec_arm_orr_reg(uint32_t instruction);
    void exec_arm_orr_imm(uint32_t instruction);
    void exec_arm_mov_reg(uint32_t instruction);
    void exec_arm_mov_imm(uint32_t instruction);
    void exec_arm_bic_reg(uint32_t instruction);
    void exec_arm_bic_imm(uint32_t instruction);
    void exec_arm_mvn_reg(uint32_t instruction);
    void exec_arm_mvn_imm(uint32_t instruction);

        // Single Data Transfer (split by writeback)
    void exec_arm_ldr_imm_pre_wb(uint32_t instruction);
    void exec_arm_ldr_imm_pre_nowb(uint32_t instruction);
    void exec_arm_ldr_imm_post_wb(uint32_t instruction);
    void exec_arm_ldr_reg_pre_wb(uint32_t instruction);
    void exec_arm_ldr_reg_pre_nowb(uint32_t instruction);
    void exec_arm_ldr_reg_post_wb(uint32_t instruction);

    void exec_arm_ldrb_imm_pre_wb(uint32_t instruction);
    void exec_arm_ldrb_imm_pre_nowb(uint32_t instruction);
    void exec_arm_ldrb_imm_post_wb(uint32_t instruction);
    void exec_arm_ldrb_reg_pre_wb(uint32_t instruction);
    void exec_arm_ldrb_reg_pre_nowb(uint32_t instruction);
    void exec_arm_ldrb_reg_post_wb(uint32_t instruction);
    void exec_arm_str_imm_pre_wb(uint32_t instruction);
    void exec_arm_str_imm_pre_nowb(uint32_t instruction);
    void exec_arm_str_imm_post_wb(uint32_t instruction);
    void exec_arm_str_reg_pre_wb(uint32_t instruction);
    void exec_arm_str_reg_pre_nowb(uint32_t instruction);
    void exec_arm_str_reg_post_wb(uint32_t instruction);
    void exec_arm_strb_imm_pre_wb(uint32_t instruction);
    void exec_arm_strb_imm_pre_nowb(uint32_t instruction);
    void exec_arm_strb_imm_post_wb(uint32_t instruction);
    void exec_arm_strb_reg_pre_wb(uint32_t instruction);
    void exec_arm_strb_reg_pre_nowb(uint32_t instruction);
    void exec_arm_strb_reg_post_wb(uint32_t instruction);

    // LDRH immediate offset variants
    void exec_arm_ldrh_imm_pre_wb(uint32_t instruction);
    void exec_arm_ldrh_imm_pre_nowb(uint32_t instruction);
    void exec_arm_ldrh_imm_post_wb(uint32_t instruction);

    // STRH immediate offset variants
    void exec_arm_strh_imm_pre_wb(uint32_t instruction);
    void exec_arm_strh_imm_pre_nowb(uint32_t instruction);
    void exec_arm_strh_imm_post_wb(uint32_t instruction);

    void exec_arm_mul(uint32_t instruction);
    void exec_arm_mla(uint32_t instruction);
    void exec_arm_umull(uint32_t instruction);
    void exec_arm_umlal(uint32_t instruction);
    void exec_arm_smull(uint32_t instruction);
    void exec_arm_smlal(uint32_t instruction);
    void exec_arm_swp(uint32_t instruction);
    void exec_arm_swpb(uint32_t instruction);
    void exec_arm_ldrh_reg_pre_wb(uint32_t instruction);
    void exec_arm_ldrh_reg_pre_nowb(uint32_t instruction);
    void exec_arm_ldrh_reg_post_wb(uint32_t instruction);
    void exec_arm_ldrsb(uint32_t instruction);
    void exec_arm_ldrsh(uint32_t instruction);
    void exec_arm_strh_reg_pre_wb(uint32_t instruction);
    void exec_arm_strh_reg_pre_nowb(uint32_t instruction);
    void exec_arm_strh_reg_post_wb(uint32_t instruction);
    void exec_arm_undefined(uint32_t instruction);

    void exec_arm_stm(uint32_t instruction);
    void exec_arm_ldm(uint32_t instruction);
    void exec_arm_b(uint32_t instruction);
    void exec_arm_bl(uint32_t instruction);
    void exec_arm_cdp(uint32_t instruction);
    void exec_arm_mrc(uint32_t instruction);
    void exec_arm_mcr(uint32_t instruction);
    void exec_arm_mrs(uint32_t instruction);
    void exec_arm_msr_reg(uint32_t instruction);
    void exec_arm_msr_imm(uint32_t instruction);
    void exec_arm_software_interrupt(uint32_t instruction);
    void exec_arm_ldc_imm(uint32_t instruction);
    void exec_arm_ldc_reg(uint32_t instruction);
    void exec_arm_stc_imm(uint32_t instruction);
    void exec_arm_stc_reg(uint32_t instruction);

    #include "inst_table.inc" // Include the generated instruction table

public:
    explicit ARMCPU(CPU& cpu);
    ~ARMCPU();

    void execute(uint32_t cycles);
    void executeWithTiming(uint32_t cycles, TimingState* timing);
    uint32_t calculateInstructionCycles(uint32_t instruction);    
};

#endif
