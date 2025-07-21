
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
    uint32_t arm_shift(uint32_t value, uint8_t shift_type, uint32_t shift_val);
  
    FORCE_INLINE void updateFlagsSub(uint32_t op1, uint32_t op2, uint32_t result);
    FORCE_INLINE void updateFlagsAdd(uint32_t op1, uint32_t op2, uint32_t result);
    FORCE_INLINE void updateFlagsLogical(uint32_t result, uint32_t carry_out);
   
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
    void exec_arm_str_imm_pre(uint32_t instruction);
    void exec_arm_str_imm_post(uint32_t instruction);
    void exec_arm_str_reg_pre(uint32_t instruction);
    void exec_arm_str_reg_post(uint32_t instruction);
    void exec_arm_ldr_imm_pre(uint32_t instruction);
    void exec_arm_ldr_imm_post(uint32_t instruction);
    void exec_arm_ldr_reg_pre(uint32_t instruction);
    void exec_arm_ldr_reg_post(uint32_t instruction);
    void exec_arm_mul(uint32_t instruction);
    void exec_arm_mla(uint32_t instruction);
    void exec_arm_umull(uint32_t instruction);
    void exec_arm_umlal(uint32_t instruction);
    void exec_arm_smull(uint32_t instruction);
    void exec_arm_smlal(uint32_t instruction);
    void exec_arm_swp(uint32_t instruction);
    void exec_arm_swpb(uint32_t instruction);
    void exec_arm_ldrh(uint32_t instruction);
    void exec_arm_ldrsb(uint32_t instruction);
    void exec_arm_ldrsh(uint32_t instruction);
    void exec_arm_strh(uint32_t instruction);
    void exec_arm_undefined(uint32_t instruction);
    void exec_arm_strb_reg(uint32_t instruction);
    void exec_arm_strb_imm_pre(uint32_t instruction);
    void exec_arm_strb_imm_post(uint32_t instruction);
    void exec_arm_ldrb_reg(uint32_t instruction);
    void exec_arm_ldrb_imm_pre(uint32_t instruction);
    void exec_arm_ldrb_imm_post(uint32_t instruction);
    void exec_arm_stm(uint32_t instruction);
    void exec_arm_ldm(uint32_t instruction);
    void exec_arm_b(uint32_t instruction);
    void exec_arm_bl(uint32_t instruction);
    void exec_arm_cdp(uint32_t instruction);
    void exec_arm_mrc(uint32_t instruction);
    void exec_arm_mcr(uint32_t instruction);
    void exec_arm_software_interrupt(uint32_t instruction);
    void exec_arm_ldc_imm(uint32_t instruction);
    void exec_arm_ldc_reg(uint32_t instruction);
    void exec_arm_stc_imm(uint32_t instruction);
    void exec_arm_stc_reg(uint32_t instruction);

    #include "inst_table.inc" // Include the generated instruction table
    // // The following code is generated by inst_table.py
    // // Helper macros for cleaner instruction table initialization
    // #define ARM_FN(func) &ARMCPU::func
    // #define REPEAT_2(handler) handler, handler
    // #define REPEAT_4(handler) handler, handler, handler, handler
    // #define REPEAT_8(handler) handler, handler, handler, handler, handler, handler, handler, handler
    // #define REPEAT_16(handler) REPEAT_8(handler), REPEAT_8(handler)
    // #define REPEAT_32(handler) REPEAT_16(handler), REPEAT_16(handler)
    // #define REPEAT_64(handler) REPEAT_32(handler), REPEAT_32(handler)
    // #define REPEAT_128(handler) REPEAT_64(handler), REPEAT_64(handler)
    // #define REPEAT_ALT(a, b) a, b, a, b
    // #define REPEAT_2ALT(a, b) a, a, b, b, a, a, b, b

    // // Table of 512 entries indexed by bits 27-19 (9 bits)
    // static constexpr void (ARMCPU::*arm_exec_table[512])(uint32_t instruction) = {
    //     REPEAT_ALT(ARM_FN(exec_arm_and_reg), ARM_FN(exec_arm_mul)), // 0x000-0x003: exec_arm_and_reg/exec_arm_mul ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_eor_reg), ARM_FN(exec_arm_mla)), // 0x004-0x007: exec_arm_eor_reg/exec_arm_mla ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_sub_reg), ARM_FN(exec_arm_undefined)), // 0x008-0x00B: exec_arm_sub_reg/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_rsb_reg), ARM_FN(exec_arm_undefined)), // 0x00C-0x00F: exec_arm_rsb_reg/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_add_reg), ARM_FN(exec_arm_umull)), // 0x010-0x013: exec_arm_add_reg/exec_arm_umull ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_adc_reg), ARM_FN(exec_arm_umlal)), // 0x014-0x017: exec_arm_adc_reg/exec_arm_umlal ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_sbc_reg), ARM_FN(exec_arm_smull)), // 0x018-0x01B: exec_arm_sbc_reg/exec_arm_smull ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_rsc_reg), ARM_FN(exec_arm_smlal)), // 0x01C-0x01F: exec_arm_rsc_reg/exec_arm_smlal ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_tst_reg), ARM_FN(exec_arm_swp)), // 0x020-0x023: exec_arm_tst_reg/exec_arm_swp ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_teq_reg), ARM_FN(exec_arm_swp)), // 0x024-0x027: exec_arm_teq_reg/exec_arm_swp ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_cmp_reg), ARM_FN(exec_arm_swpb)), // 0x028-0x02B: exec_arm_cmp_reg/exec_arm_swpb ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_cmn_reg), ARM_FN(exec_arm_swpb)), // 0x02C-0x02F: exec_arm_cmn_reg/exec_arm_swpb ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_orr_reg), ARM_FN(exec_arm_undefined)), // 0x030-0x033: exec_arm_orr_reg/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_mov_reg), ARM_FN(exec_arm_undefined)), // 0x034-0x037: exec_arm_mov_reg/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_bic_reg), ARM_FN(exec_arm_undefined)), // 0x038-0x03B: exec_arm_bic_reg/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_mvn_reg), ARM_FN(exec_arm_undefined)), // 0x03C-0x03F: exec_arm_mvn_reg/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_and_imm), ARM_FN(exec_arm_undefined)), // 0x040-0x043: exec_arm_and_imm/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_eor_imm), ARM_FN(exec_arm_undefined)), // 0x044-0x047: exec_arm_eor_imm/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_sub_imm), ARM_FN(exec_arm_undefined)), // 0x048-0x04B: exec_arm_sub_imm/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_rsb_imm), ARM_FN(exec_arm_undefined)), // 0x04C-0x04F: exec_arm_rsb_imm/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_add_imm), ARM_FN(exec_arm_undefined)), // 0x050-0x053: exec_arm_add_imm/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_adc_imm), ARM_FN(exec_arm_undefined)), // 0x054-0x057: exec_arm_adc_imm/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_sbc_imm), ARM_FN(exec_arm_undefined)), // 0x058-0x05B: exec_arm_sbc_imm/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_rsc_imm), ARM_FN(exec_arm_undefined)), // 0x05C-0x05F: exec_arm_rsc_imm/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_tst_reg), ARM_FN(exec_arm_undefined)), // 0x060-0x063: exec_arm_tst_reg/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_teq_reg), ARM_FN(exec_arm_undefined)), // 0x064-0x067: exec_arm_teq_reg/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_cmp_reg), ARM_FN(exec_arm_undefined)), // 0x068-0x06B: exec_arm_cmp_reg/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_cmn_reg), ARM_FN(exec_arm_undefined)), // 0x06C-0x06F: exec_arm_cmn_reg/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_orr_imm), ARM_FN(exec_arm_undefined)), // 0x070-0x073: exec_arm_orr_imm/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_mov_imm), ARM_FN(exec_arm_undefined)), // 0x074-0x077: exec_arm_mov_imm/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_bic_imm), ARM_FN(exec_arm_undefined)), // 0x078-0x07B: exec_arm_bic_imm/exec_arm_undefined ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_mvn_imm), ARM_FN(exec_arm_undefined)), // 0x07C-0x07F: exec_arm_mvn_imm/exec_arm_undefined ABAB
    //     ARM_FN(exec_arm_str_reg_post),              // 0x080: exec_arm_str_reg_post
    //     REPEAT_2(ARM_FN(exec_arm_undefined)),       // 0x081-0x082: exec_arm_undefined
    //     ARM_FN(exec_arm_undefined),                 // 0x083: exec_arm_undefined
    //     ARM_FN(exec_arm_str_reg_post),              // 0x084: exec_arm_str_reg_post
    //     REPEAT_8(ARM_FN(exec_arm_undefined)),       // 0x085-0x08C: exec_arm_undefined
    //     REPEAT_2(ARM_FN(exec_arm_undefined)),       // 0x08D-0x08E: exec_arm_undefined
    //     ARM_FN(exec_arm_undefined),                 // 0x08F: exec_arm_undefined
    //     ARM_FN(exec_arm_str_reg_post),              // 0x090: exec_arm_str_reg_post
    //     REPEAT_2(ARM_FN(exec_arm_undefined)),       // 0x091-0x092: exec_arm_undefined
    //     ARM_FN(exec_arm_undefined),                 // 0x093: exec_arm_undefined
    //     ARM_FN(exec_arm_str_reg_post),              // 0x094: exec_arm_str_reg_post
    //     REPEAT_8(ARM_FN(exec_arm_undefined)),       // 0x095-0x09C: exec_arm_undefined
    //     REPEAT_2(ARM_FN(exec_arm_undefined)),       // 0x09D-0x09E: exec_arm_undefined
    //     ARM_FN(exec_arm_undefined),                 // 0x09F: exec_arm_undefined
    //     ARM_FN(exec_arm_str_reg_pre),               // 0x0A0: exec_arm_str_reg_pre
    //     REPEAT_2(ARM_FN(exec_arm_undefined)),       // 0x0A1-0x0A2: exec_arm_undefined
    //     ARM_FN(exec_arm_undefined),                 // 0x0A3: exec_arm_undefined
    //     ARM_FN(exec_arm_str_reg_pre),               // 0x0A4: exec_arm_str_reg_pre
    //     REPEAT_8(ARM_FN(exec_arm_undefined)),       // 0x0A5-0x0AC: exec_arm_undefined
    //     REPEAT_2(ARM_FN(exec_arm_undefined)),       // 0x0AD-0x0AE: exec_arm_undefined
    //     ARM_FN(exec_arm_undefined),                 // 0x0AF: exec_arm_undefined
    //     ARM_FN(exec_arm_str_reg_pre),               // 0x0B0: exec_arm_str_reg_pre
    //     REPEAT_2(ARM_FN(exec_arm_undefined)),       // 0x0B1-0x0B2: exec_arm_undefined
    //     ARM_FN(exec_arm_undefined),                 // 0x0B3: exec_arm_undefined
    //     ARM_FN(exec_arm_str_reg_pre),               // 0x0B4: exec_arm_str_reg_pre
    //     REPEAT_8(ARM_FN(exec_arm_undefined)),       // 0x0B5-0x0BC: exec_arm_undefined
    //     REPEAT_2(ARM_FN(exec_arm_undefined)),       // 0x0BD-0x0BE: exec_arm_undefined
    //     ARM_FN(exec_arm_undefined),                 // 0x0BF: exec_arm_undefined
    //     ARM_FN(exec_arm_str_imm_post),              // 0x0C0: exec_arm_str_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0C1: exec_arm_undefined
    //     ARM_FN(exec_arm_ldr_imm_post),              // 0x0C2: exec_arm_ldr_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0C3: exec_arm_undefined
    //     ARM_FN(exec_arm_str_imm_post),              // 0x0C4: exec_arm_str_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0C5: exec_arm_undefined
    //     ARM_FN(exec_arm_ldr_imm_post),              // 0x0C6: exec_arm_ldr_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0C7: exec_arm_undefined
    //     ARM_FN(exec_arm_strb_imm_post),             // 0x0C8: exec_arm_strb_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0C9: exec_arm_undefined
    //     ARM_FN(exec_arm_ldrb_imm_post),             // 0x0CA: exec_arm_ldrb_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0CB: exec_arm_undefined
    //     ARM_FN(exec_arm_strb_imm_post),             // 0x0CC: exec_arm_strb_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0CD: exec_arm_undefined
    //     ARM_FN(exec_arm_ldrb_imm_post),             // 0x0CE: exec_arm_ldrb_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0CF: exec_arm_undefined
    //     ARM_FN(exec_arm_str_imm_post),              // 0x0D0: exec_arm_str_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0D1: exec_arm_undefined
    //     ARM_FN(exec_arm_ldr_imm_post),              // 0x0D2: exec_arm_ldr_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0D3: exec_arm_undefined
    //     ARM_FN(exec_arm_str_imm_post),              // 0x0D4: exec_arm_str_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0D5: exec_arm_undefined
    //     ARM_FN(exec_arm_ldr_imm_post),              // 0x0D6: exec_arm_ldr_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0D7: exec_arm_undefined
    //     ARM_FN(exec_arm_strb_imm_post),             // 0x0D8: exec_arm_strb_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0D9: exec_arm_undefined
    //     ARM_FN(exec_arm_ldrb_imm_post),             // 0x0DA: exec_arm_ldrb_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0DB: exec_arm_undefined
    //     ARM_FN(exec_arm_strb_imm_post),             // 0x0DC: exec_arm_strb_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0DD: exec_arm_undefined
    //     ARM_FN(exec_arm_ldrb_imm_post),             // 0x0DE: exec_arm_ldrb_imm_post
    //     ARM_FN(exec_arm_undefined),                 // 0x0DF: exec_arm_undefined
    //     ARM_FN(exec_arm_str_imm_pre),               // 0x0E0: exec_arm_str_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0E1: exec_arm_undefined
    //     ARM_FN(exec_arm_ldr_imm_pre),               // 0x0E2: exec_arm_ldr_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0E3: exec_arm_undefined
    //     ARM_FN(exec_arm_str_imm_pre),               // 0x0E4: exec_arm_str_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0E5: exec_arm_undefined
    //     ARM_FN(exec_arm_ldr_imm_pre),               // 0x0E6: exec_arm_ldr_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0E7: exec_arm_undefined
    //     ARM_FN(exec_arm_strb_imm_pre),              // 0x0E8: exec_arm_strb_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0E9: exec_arm_undefined
    //     ARM_FN(exec_arm_ldrb_imm_pre),              // 0x0EA: exec_arm_ldrb_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0EB: exec_arm_undefined
    //     ARM_FN(exec_arm_strb_imm_pre),              // 0x0EC: exec_arm_strb_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0ED: exec_arm_undefined
    //     ARM_FN(exec_arm_ldrb_imm_pre),              // 0x0EE: exec_arm_ldrb_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0EF: exec_arm_undefined
    //     ARM_FN(exec_arm_str_imm_pre),               // 0x0F0: exec_arm_str_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0F1: exec_arm_undefined
    //     ARM_FN(exec_arm_ldr_imm_pre),               // 0x0F2: exec_arm_ldr_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0F3: exec_arm_undefined
    //     ARM_FN(exec_arm_str_imm_pre),               // 0x0F4: exec_arm_str_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0F5: exec_arm_undefined
    //     ARM_FN(exec_arm_ldr_imm_pre),               // 0x0F6: exec_arm_ldr_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0F7: exec_arm_undefined
    //     ARM_FN(exec_arm_strb_imm_pre),              // 0x0F8: exec_arm_strb_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0F9: exec_arm_undefined
    //     ARM_FN(exec_arm_ldrb_imm_pre),              // 0x0FA: exec_arm_ldrb_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0FB: exec_arm_undefined
    //     ARM_FN(exec_arm_strb_imm_pre),              // 0x0FC: exec_arm_strb_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0FD: exec_arm_undefined
    //     ARM_FN(exec_arm_ldrb_imm_pre),              // 0x0FE: exec_arm_ldrb_imm_pre
    //     ARM_FN(exec_arm_undefined),                 // 0x0FF: exec_arm_undefined
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x100-0x103: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x104-0x107: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x108-0x10B: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x10C-0x10F: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x110-0x113: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x114-0x117: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x118-0x11B: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x11C-0x11F: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x120-0x123: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x124-0x127: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x128-0x12B: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x12C-0x12F: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x130-0x133: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x134-0x137: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x138-0x13B: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stm), ARM_FN(exec_arm_ldm)), // 0x13C-0x13F: exec_arm_stm/exec_arm_ldm ABAB
    //     REPEAT_32(ARM_FN(exec_arm_b)),              // 0x140-0x15F: exec_arm_b
    //     REPEAT_32(ARM_FN(exec_arm_bl)),             // 0x160-0x17F: exec_arm_bl
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_imm), ARM_FN(exec_arm_ldc_imm)), // 0x180-0x183: exec_arm_stc_imm/exec_arm_ldc_imm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_imm), ARM_FN(exec_arm_ldc_imm)), // 0x184-0x187: exec_arm_stc_imm/exec_arm_ldc_imm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_imm), ARM_FN(exec_arm_ldc_imm)), // 0x188-0x18B: exec_arm_stc_imm/exec_arm_ldc_imm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_imm), ARM_FN(exec_arm_ldc_imm)), // 0x18C-0x18F: exec_arm_stc_imm/exec_arm_ldc_imm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_imm), ARM_FN(exec_arm_ldc_imm)), // 0x190-0x193: exec_arm_stc_imm/exec_arm_ldc_imm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_imm), ARM_FN(exec_arm_ldc_imm)), // 0x194-0x197: exec_arm_stc_imm/exec_arm_ldc_imm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_imm), ARM_FN(exec_arm_ldc_imm)), // 0x198-0x19B: exec_arm_stc_imm/exec_arm_ldc_imm ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_imm), ARM_FN(exec_arm_ldc_imm)), // 0x19C-0x19F: exec_arm_stc_imm/exec_arm_ldc_imm ABAB
    //     REPEAT_32(ARM_FN(exec_arm_undefined)),      // 0x1A0-0x1BF: exec_arm_undefined
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_reg), ARM_FN(exec_arm_ldc_reg)), // 0x1C0-0x1C3: exec_arm_stc_reg/exec_arm_ldc_reg ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_reg), ARM_FN(exec_arm_ldc_reg)), // 0x1C4-0x1C7: exec_arm_stc_reg/exec_arm_ldc_reg ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_reg), ARM_FN(exec_arm_ldc_reg)), // 0x1C8-0x1CB: exec_arm_stc_reg/exec_arm_ldc_reg ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_reg), ARM_FN(exec_arm_ldc_reg)), // 0x1CC-0x1CF: exec_arm_stc_reg/exec_arm_ldc_reg ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_reg), ARM_FN(exec_arm_ldc_reg)), // 0x1D0-0x1D3: exec_arm_stc_reg/exec_arm_ldc_reg ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_reg), ARM_FN(exec_arm_ldc_reg)), // 0x1D4-0x1D7: exec_arm_stc_reg/exec_arm_ldc_reg ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_reg), ARM_FN(exec_arm_ldc_reg)), // 0x1D8-0x1DB: exec_arm_stc_reg/exec_arm_ldc_reg ABAB
    //     REPEAT_ALT(ARM_FN(exec_arm_stc_reg), ARM_FN(exec_arm_ldc_reg)), // 0x1DC-0x1DF: exec_arm_stc_reg/exec_arm_ldc_reg ABAB
    //     REPEAT_32(ARM_FN(exec_arm_software_interrupt)), // 0x1E0-0x1FF: exec_arm_software_interrupt
    // };

    // // Cleanup macros to avoid polluting the namespace
    // #undef ARM_FN
    // #undef REPEAT_2
    // #undef REPEAT_4
    // #undef REPEAT_8
    // #undef REPEAT_16
    // #undef REPEAT_32
    // #undef REPEAT_64
    // #undef REPEAT_128
    // #undef REPEAT_ALT
    // #undef REPEAT_2ALT

public:
    explicit ARMCPU(CPU& cpu);
    ~ARMCPU();

    void execute(uint32_t cycles);
    void executeWithTiming(uint32_t cycles, TimingState* timing);
    uint32_t calculateInstructionCycles(uint32_t instruction);
    
    
    // Cache management functions
    // void invalidateInstructionCache() { instruction_cache.clear(); }
    // void invalidateInstructionCacheRange(uint32_t start_addr, uint32_t end_addr) {
    //     instruction_cache.invalidate_range(start_addr, end_addr);
    // }
    // void invalidateInstructionCache(uint32_t start_addr, uint32_t end_addr) {
    //     instruction_cache.invalidate_range(start_addr, end_addr);
    // }
    // ARMInstructionCache::CacheStats getInstructionCacheStats() const {
    //     return instruction_cache.getStats();
    // }
    // void resetInstructionCacheStats() { instruction_cache.resetStats(); }
};

#endif
