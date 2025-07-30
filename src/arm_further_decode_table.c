// Static decode table for ARM further decode
// Each line is derived from the macro block in mgba_decode.cpp
// Index: (5 bits from comment) << 2 | (2 bits from X position)
// If a line is missing, fexec_arm_undefined with exec_arm_undefined

// Example function pointer type:
// typedef void (*arm_func_t)(void);

void exec_arm_mul(uint32_t instruction);
    void exec_arm_mla(uint32_t instruction);
    void exec_arm_umull(uint32_t instruction);
    void exec_arm_umlal(uint32_t instruction);
    void exec_arm_smull(uint32_t instruction);
    void exec_arm_smlal(uint32_t instruction);
    void exec_arm_swp(uint32_t instruction);
    void exec_arm_swpb(uint32_t instruction);
    void exec_arm_ldrh_reg_pre_wb(uint32_t instruction);
    // void exec_arm_ldrh_reg_pre_nowb(uint32_t instruction);
    void exec_arm_ldrh_reg_post_wb(uint32_t instruction);
    void exec_arm_ldrsb(uint32_t instruction);
    void exec_arm_ldrsh(uint32_t instruction);
    void exec_arm_strh_reg_pre_wb(uint32_t instruction);
    //void exec_arm_strh_reg_pre_nowb(uint32_t instruction);
    void exec_arm_strh_reg_post_wb(uint32_t instruction);
    //void exec_arm_undefined(uint32_t instruction);

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
    // void exec_arm_ldrb_reg_pre_nowb(uint32_t instruction);
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

static const arm_func_t arm_further_decode[32 * 4] = {
    // -00---X-
    exec_arm_undefined, exec_arm_strh_reg_pre_nowb, exec_arm_undefined, exec_arm_undefined, // -00---X- (MUL, STRH, exec_arm_undefined, exec_arm_undefined)
    // -01---X-
    exec_arm_undefined, exec_arm_ldrh_reg_pre_nowb, exec_arm_ldrb_reg_pre_nowb, exec_arm_ldrh_reg_pre_nowb, // -01---X- (MUL, LDRH, LDRB, LDRH)
    // -02---X-
    exec_arm_undefined, exec_arm_strh_reg_pre_nowb, exec_arm_undefined, exec_arm_undefined, // -02---X- (MLA, STRH, exec_arm_undefined, exec_arm_undefined)
    // -03---X-
    exec_arm_undefined, exec_arm_ldrh_reg_pre_nowb, exec_arm_ldrb_reg_pre_nowb, exec_arm_ldrh_reg_pre_nowb, // -03---X- (MLA, LDRH, LDRB, LDRH)
    // -04---X-
    exec_arm_undefined, STRHI, exec_arm_undefined, exec_arm_undefined, // -04---X- (exec_arm_undefined, STRHI, exec_arm_undefined, exec_arm_undefined)
    // -05---X-
    exec_arm_undefined, LDRHI, exec_arm_str_imm_pre_nowb, LDRHI, // -05---X- (exec_arm_undefined, LDRHI, LDRBI, LDRHI)
    // -06---X-
    exec_arm_undefined, STRHI, exec_arm_undefined, exec_arm_undefined, // -06---X- (exec_arm_undefined, STRHI, exec_arm_undefined, exec_arm_undefined)
    // -07---X-
    exec_arm_undefined, LDRHI, exec_arm_str_imm_pre_nowb, LDRHI, // -07---X- (exec_arm_undefined, LDRHI, LDRBI, LDRHI)
    // -08---X-
    exec_arm_undefined, exec_arm_strh_reg_pre_nowb, exec_arm_undefined, exec_arm_undefined, // -08---X- (UMULL, STRH, exec_arm_undefined, exec_arm_undefined)
    // -09---X-
    exec_arm_undefined, exec_arm_ldrh_reg_pre_nowb, LDRB, exec_arm_ldrh_reg_pre_nowb, // -09---X- (UMULL, LDRH, LDRB, LDRH)
    // -0A---X-
    exec_arm_undefined, exec_arm_strh_reg_pre_nowb, exec_arm_undefined, exec_arm_undefined, // -0A---X- (UMLAL, STRH, exec_arm_undefined, exec_arm_undefined)
    // -0B---X-
    exec_arm_undefined, exec_arm_ldrh_reg_pre_nowb, exec_arm_ldrb_reg_pre_nowb, exec_arm_ldrh_reg_pre_nowb, // -0B---X- (UMLAL, LDRH, LDRB, LDSH)
    // -0C---X-
    exec_arm_undefined, STRHI, exec_arm_undefined, exec_arm_undefined, // -0C---X- (SMULL, STRHI, exec_arm_undefined, exec_arm_undefined)
    // -0D---X-
    exec_arm_undefined, exec_arm_smull, LDRHI, exec_arm_str_imm_pre_nowb, // -0D---X- (SBC, SMULL, LDRHI, LDRBI)
    // -0E---X-
    exec_arm_undefined, exec_arm_smlal, STRHI, exec_arm_undefined, // -0E---X- (RSC, SMLAL, STRHI, exec_arm_undefined)
    // -0F---X-
    exec_arm_undefined, exec_arm_smlal, LDRHI, exec_arm_str_imm_pre_nowb, // -0F---X- (RSCS, SMLAL, LDRHI, LDRBI)
    // -10---X-
    exec_arm_undefined, exec_arm_swp, exec_arm_undefined, exec_arm_strh_reg_post_wb, // -10---X- (MRS, SWP, exec_arm_undefined, STRHP)
    // -11---X-
    exec_arm_undefined, exec_arm_undefined, exec_arm_ldrh_reg_pre_nowb, LDRBP, // -11---X- (TST, exec_arm_undefined, LDRHP, LDRBP)
    // -12---X-
    exec_arm_undefined, exec_arm_undefined, exec_arm_strh_reg_post_wb, exec_arm_undefined, // -12---X- (MSR, exec_arm_undefined, STRHPW, exec_arm_undefined)
    // -13---X-
    exec_arm_undefined, exec_arm_undefined, exec_arm_ldrh_reg_pre_wb, LDRBPW, // -13---X- (TEQ, exec_arm_undefined, LDRHPW, LDRBPW)
    // -14---X-
    exec_arm_undefined, exec_arm_swpb, STRHIP, exec_arm_undefined, // -14---X- (MRSR, SWPB, STRHIP, exec_arm_undefined)
    // -15---X-
    exec_arm_undefined, exec_arm_undefined, LDRHIP, LDRBIP, // -15---X- (CMP, exec_arm_undefined, LDRHIP, LDRBIP)
    // -16---X-
    exec_arm_undefined, exec_arm_undefined, STRHIPW, exec_arm_undefined, // -16---X- (MSRR, exec_arm_undefined, STRHIPW, exec_arm_undefined)
    // -17---X-
    exec_arm_undefined, exec_arm_undefined, LDRHIPW, LDRBIPW, // -17---X- (CMN, exec_arm_undefined, LDRHIPW, LDRBIPW)
    // -18---X-
    exec_arm_undefined, exec_arm_smlal, exec_arm_strh_reg_post_wb, exec_arm_undefined, // -18---X- (ORR, SMLAL, STRHP, exec_arm_undefined)
    // -19---X-
    exec_arm_undefined, exec_arm_smlal, exec_arm_ldrh_reg_pre_nowb, LDRBP, // -19---X- (ORRS, SMLAL, LDRHP, LDRBP)
    // -1A---X-
    exec_arm_undefined, exec_arm_smlal, exec_arm_strh_reg_post_wb, exec_arm_undefined, // -1A---X- (MOV, SMLAL, STRHPW, exec_arm_undefined)
    // -1B---X-
    exec_arm_undefined, exec_arm_smlal, exec_arm_ldrh_reg_pre_wb, LDRBPW, // -1B---X- (MOVS, SMLAL, LDRHPW, LDRBPW)
    // -1C---X-
    exec_arm_undefined, exec_arm_smlal, STRHIP, exec_arm_undefined, // -1C---X- (BIC, SMLAL, STRHIP, exec_arm_undefined)
    // -1D---X-
    exec_arm_undefined, exec_arm_smlal, LDRHIP, LDRBIP, // -1D---X- (BICS, SMLAL, LDRHIP, LDRBIP)
    // -1E---X-
    exec_arm_undefined, exec_arm_smlal, STRHIPW, exec_arm_undefined, // -1E---X- (MVN, SMLAL, STRHIPW, exec_arm_undefined)
    // -1F---X-
    exec_arm_undefined, exec_arm_smlal, LDRHIPW, LDRBIPW // -1F---X- (MVNS, SMLAL, LDRHIPW, LDRBIPW)
};
