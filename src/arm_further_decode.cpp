#include "arm_cpu.h"
#include "debug.h"
#include <cstdio>

const ARMCPU::arm_func_t ARMCPU::arm_further_decode[32 * 4] = {
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_reg_post_wb,  &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -00---X- (MUL, STRH, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_reg_post_wb,  &ARMCPU::exec_arm_ldrsb_reg_post_wb,     &ARMCPU::exec_arm_ldrsh_reg_post_wb, // -01---X- (MUL, LDRH, LDRSB, LDRSH)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_reg_post_wb,  &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -02---X- (MLA, STRH, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_reg_post_wb,  &ARMCPU::exec_arm_ldrsb_reg_post_wb,     &ARMCPU::exec_arm_ldrsh_reg_post_wb, // -03---X- (MLA, LDRH, LDRSB, LDRSH)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_imm_post_wb,  &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -04---X- (exec_arm_undefined, STRHI, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_imm_post_wb,  &ARMCPU::exec_arm_ldrsb_imm_post_wb,     &ARMCPU::exec_arm_ldrsh_imm_post_wb, // -05---X- (exec_arm_undefined, LDRHI, LDRSBI, LDRSHI)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_imm_post_wb,  &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -06---X- (exec_arm_undefined, STRHI, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_imm_post_wb,  &ARMCPU::exec_arm_ldrsb_imm_post_wb,     &ARMCPU::exec_arm_ldrsh_imm_post_wb, // -07---X- (exec_arm_undefined, LDRHI, LDRSBI, LDRSHI)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_reg_post_wb,  &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -08---X- (UMULL, STRH, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_reg_post_wb,  &ARMCPU::exec_arm_ldrsb_reg_post_wb,     &ARMCPU::exec_arm_ldrsh_reg_post_wb, // -09---X- (UMULL, LDRH, LDRSB, LDRSH)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_reg_post_wb,  &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -0A---X- (UMLAL, STRH, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_reg_post_wb,  &ARMCPU::exec_arm_ldrsb_reg_post_wb,     &ARMCPU::exec_arm_ldrsh_reg_post_wb, // -0B---X- (UMLAL, LDRH, LDRSB, LDRSH)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_imm_post_wb,  &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -0C---X- (SMULL, STRHI, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_imm_post_wb,  &ARMCPU::exec_arm_ldrsb_imm_post_wb,     &ARMCPU::exec_arm_ldrsh_imm_post_wb, // -0D---X- (SMULL, LDRHI, LDRSBI, LDRSHI)
    &ARMCPU::exec_arm_smlal,     &ARMCPU::exec_arm_strh_imm_post_wb,  &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -0E---X- (SMLAL, STRHI, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_smlal,     &ARMCPU::exec_arm_ldrh_imm_post_wb,  &ARMCPU::exec_arm_ldrsb_imm_post_wb,     &ARMCPU::exec_arm_ldrsh_imm_post_wb, // -0F---X- (SMLAL, LDRHI, LDRSBI, LDRSHI)

    &ARMCPU::exec_arm_swp,       &ARMCPU::exec_arm_strh_reg_pre_nowb, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -10---X- (SWP, STRHP, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_reg_pre_nowb, &ARMCPU::exec_arm_ldrsb_reg_pre_nowb,     &ARMCPU::exec_arm_ldrsh_reg_pre_nowb, // -11---X- (exec_arm_undefined, LDRHP, LDRSBP, LDRSHP)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_reg_pre_wb,   &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -12---X- (exec_arm_undefined, STRHPW, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_reg_pre_wb,   &ARMCPU::exec_arm_ldrsb_reg_pre_wb,     &ARMCPU::exec_arm_ldrsh_reg_pre_wb, // -13---X- (exec_arm_undefined, LDRHPW, LDRBPW, LDRSHPW)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_imm_pre_nowb, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -14---X- (SWPB, STRHIP, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_imm_pre_nowb, &ARMCPU::exec_arm_ldrsb_imm_pre_nowb,     &ARMCPU::exec_arm_ldrsh_imm_pre_nowb, // -15---X- (exec_arm_undefined, LDRHIP, LDRBIP, LDRSHIP)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_imm_pre_wb,   &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -16---X- (exec_arm_undefined, STRHIPW, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_imm_pre_wb,   &ARMCPU::exec_arm_ldrsb_imm_pre_wb,     &ARMCPU::exec_arm_ldrsh_imm_pre_wb, // -17---X- (exec_arm_undefined, LDRHIPW, LDRBIPW, LDRSHIPW)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_reg_pre_nowb, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -18---X- (SMLAL, STRHP, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_reg_pre_nowb, &ARMCPU::exec_arm_ldrsb_reg_pre_nowb,     &ARMCPU::exec_arm_ldrsh_reg_pre_nowb, // -19---X- (SMLAL, LDRHP, LDRBP, LDRSHPU)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_reg_pre_wb,   &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -1A---X- (SMLAL, STRHPW, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_reg_pre_wb,   &ARMCPU::exec_arm_ldrsb_reg_pre_wb,     &ARMCPU::exec_arm_ldrsh_reg_pre_wb, // -1B---X- (SMLAL, LDRHPW, LDRBPW, LDRHPW)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_imm_pre_nowb, &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined,// -1C---X- (SMLAL, STRHIP, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_imm_pre_nowb, &ARMCPU::exec_arm_ldrsb_imm_pre_nowb,     &ARMCPU::exec_arm_ldrsh_imm_pre_nowb, // -1D---X- (SMLAL, LDRHIP, LDRBIP, LDRSHIPU)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_strh_imm_pre_wb,   &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_undefined, // -1E---X- (SMLAL, STRHIPW, exec_arm_undefined, exec_arm_undefined)
    &ARMCPU::exec_arm_undefined, &ARMCPU::exec_arm_ldrh_imm_pre_wb,   &ARMCPU::exec_arm_ldrsb_imm_pre_wb,     &ARMCPU::exec_arm_ldrsh_imm_pre_wb // -1F---X- (SMLAL, LDRHIPW, LDRBIPW, LDRHIPW)
};


