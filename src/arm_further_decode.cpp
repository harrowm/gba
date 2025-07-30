#include "arm_cpu.h"
#include "debug.h"
#include <cstdio>

const ARMCPU::arm_func_t ARMCPU::arm_further_decode[32 * 4] = {
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_reg_pre_nowb, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -00---X- (MUL, STRH, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_reg_pre_nowb, &ARMCPU::exec_arm_ldrb_reg_pre_nowb, &ARMCPU::exec_arm_ldrh_reg_pre_nowb, // -01---X- (MUL, LDRH, LDRB, LDRH)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_reg_pre_nowb, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -02---X- (MLA, STRH, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_reg_pre_nowb, &ARMCPU::exec_arm_ldrb_reg_pre_nowb, &ARMCPU::exec_arm_ldrh_reg_pre_nowb, // -03---X- (MLA, LDRH, LDRB, LDRH)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_imm_pre_nowb, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -04---X- (exec_arm_undefined, STRHI, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_imm_pre_nowb, &ARMCPU::exec_arm_str_imm_pre_nowb, &ARMCPU::exec_arm_ldrh_imm_pre_nowb, // -05---X- (exec_arm_undefined, LDRHI, LDRBI, LDRHI)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_imm_pre_nowb, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -06---X- (exec_arm_undefined, STRHI, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_imm_pre_nowb, &ARMCPU::exec_arm_str_imm_pre_nowb, &ARMCPU::exec_arm_ldrh_imm_pre_nowb, // -07---X- (exec_arm_undefined, LDRHI, LDRBI, LDRHI)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_reg_pre_nowb, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -08---X- (UMULL, STRH, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_reg_pre_nowb, &ARMCPU::exec_arm_ldrb_reg_pre_nowb, &ARMCPU::exec_arm_ldrh_reg_pre_nowb, // -09---X- (UMULL, LDRH, LDRB, LDRH)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_reg_pre_nowb, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -0A---X- (UMLAL, STRH, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_reg_pre_nowb, &ARMCPU::exec_arm_ldrb_reg_pre_nowb, &ARMCPU::exec_arm_ldrh_reg_pre_nowb, // -0B---X- (UMLAL, LDRH, LDRB, LDSH)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_imm_pre_nowb, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -0C---X- (SMULL, STRHI, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_smull, &ARMCPU::exec_arm_ldrh_imm_pre_nowb, &ARMCPU::exec_arm_str_imm_pre_nowb, // -0D---X- (SBC, SMULL, LDRHI, LDRBI)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_smlal, &ARMCPU::exec_arm_strh_imm_pre_nowb, &ARMCPU::exec_arm_undefined, // -0E---X- (RSC, SMLAL, STRHI, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_smlal, &ARMCPU::exec_arm_ldrh_imm_pre_nowb, &ARMCPU::exec_arm_str_imm_pre_nowb, // -0F---X- (RSCS, SMLAL, LDRHI, LDRBI)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_swp, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_reg_post_wb, // -10---X- (MRS, SWP, exec_arm_undefined, STRHP)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_reg_pre_nowb, &ARMCPU::exec_arm_ldrb_imm_post_wb, // -11---X- (TST, exec_arm_undefined, LDRHP, LDRBP)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_reg_post_wb, &ARMCPU::exec_arm_undefined, // -12---X- (MSR, exec_arm_undefined, STRHPW, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_reg_pre_wb, &ARMCPU::exec_arm_ldrb_imm_post_wb, // -13---X- (TEQ, exec_arm_undefined, LDRHPW, LDRBPW)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_swpb, &ARMCPU::exec_arm_strh_imm_post_wb, &ARMCPU::exec_arm_undefined, // -14---X- (MRSR, SWPB, STRHIP, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_imm_pre_nowb, &ARMCPU::exec_arm_ldrb_imm_post_wb, // -15---X- (CMP, exec_arm_undefined, LDRHIP, LDRBIP)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_imm_post_wb, &ARMCPU::exec_arm_undefined, // -16---X- (MSRR, exec_arm_undefined, STRHIPW, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_imm_pre_wb, &ARMCPU::exec_arm_ldrb_imm_post_wb, // -17---X- (CMN, exec_arm_undefined, LDRHIPW, LDRBIPW)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_smlal, &ARMCPU::exec_arm_strh_reg_post_wb, &ARMCPU::exec_arm_undefined, // -18---X- (ORR, SMLAL, STRHP, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_smlal, &ARMCPU::exec_arm_ldrh_reg_pre_nowb, &ARMCPU::exec_arm_ldrb_imm_post_wb, // -19---X- (ORRS, SMLAL, LDRHP, LDRBP)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_smlal, &ARMCPU::exec_arm_strh_reg_post_wb, &ARMCPU::exec_arm_undefined, // -1A---X- (MOV, SMLAL, STRHPW, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_smlal, &ARMCPU::exec_arm_ldrh_reg_pre_wb, &ARMCPU::exec_arm_ldrb_imm_post_wb, // -1B---X- (MOVS, SMLAL, LDRHPW, LDRBPW)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_smlal, &ARMCPU::exec_arm_strh_imm_post_wb, &ARMCPU::exec_arm_undefined, // -1C---X- (BIC, SMLAL, STRHIP, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_smlal, &ARMCPU::exec_arm_ldrh_imm_pre_nowb, &ARMCPU::exec_arm_ldrb_imm_post_wb, // -1D---X- (BICS, SMLAL, LDRHIP, LDRBIP)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_smlal, &ARMCPU::exec_arm_strh_imm_post_wb, &ARMCPU::exec_arm_undefined, // -1E---X- (MVN, SMLAL, STRHIPW, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_smlal, &ARMCPU::exec_arm_ldrh_imm_pre_wb, &ARMCPU::exec_arm_ldrb_imm_post_wb // -1F---X- (MVNS, SMLAL, LDRHIPW, LDRBIPW)
};


