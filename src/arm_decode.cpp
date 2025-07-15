#ifdef __cplusplus
extern "C" {
#endif
void arm_and();
void arm_eor();
void arm_sub();
void arm_rsb();
void arm_add();
void arm_adc();
void arm_sbc();
void arm_rsc();
void arm_tst();
void arm_teq();
void arm_cmp();
void arm_cmn();
void arm_orr();
void arm_mov_imm();
void arm_mov_reg();
void arm_bic();
void arm_mvn();
void arm_strb();
void arm_ldrb();
void arm_stm();
void arm_ldm();
void arm_b();
void arm_bl();
void arm_swp();
void arm_swpb();
void arm_mul();
void arm_mla();
void arm_umull();
void arm_umlal();
void arm_smull();
void arm_smlal();
void arm_ldrh();
void arm_strh();
void arm_ldrsb();
void arm_ldrsh();
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" {
#endif
void arm_strb();
void arm_ldrb();
#ifdef __cplusplus
}
#endif
#include "arm_decode.h"
#include "arm_cpu.h"
#include <cstdio>

template <uint32_t hi, uint32_t lo>
constexpr uint32_t bits(uint32_t instruction) {
    static_assert(hi >= lo && hi < 32, "Invalid bit range");
    return ((instruction >> lo) & ((1 << (hi - lo + 1)) - 1));
}

// ARM Data Processing decoders (AND, EOR, SUB, RSB, ADD, ADC, SBC, RSC, TST, TEQ, CMP, CMN, ORR, BIC, MVN)

// ARM Data Processing decoders: split into immediate and register variants
#define ARM_DP_DECODER_IMM(name) \
void decode_arm_##name##_imm(ARMCachedInstruction& decoded) { \
    decoded.dp_op = static_cast<ARMDataProcessingOp>((decoded.instruction >> 21) & 0xF); \
    decoded.set_flags = (decoded.instruction >> 20) & 1; \
    decoded.rn = bits<19,16>(decoded.instruction); \
    decoded.rd = bits<15,12>(decoded.instruction); \
    decoded.immediate = true; \
    decoded.rotate_imm = bits<11,8>(decoded.instruction); \
    decoded.imm8 = bits<7,0>(decoded.instruction); \
    decoded.execute_func = &ARMCPU::arm_##name; \
}

#define ARM_DP_DECODER_REG(name) \
void decode_arm_##name##_reg(ARMCachedInstruction& decoded) { \
    decoded.dp_op = static_cast<ARMDataProcessingOp>((decoded.instruction >> 21) & 0xF); \
    decoded.set_flags = (decoded.instruction >> 20) & 1; \
    decoded.rn = bits<19,16>(decoded.instruction); \
    decoded.rd = bits<15,12>(decoded.instruction); \
    decoded.immediate = false; \
    decoded.rs = bits<11,8>(decoded.instruction); \
    decoded.shift_type = bits<6,5>(decoded.instruction); \
    decoded.reg_shift = bits<4,4>(decoded.instruction); \
    decoded.rm = bits<3,0>(decoded.instruction); \
    decoded.execute_func = &ARMCPU::arm_##name; \
}

ARM_DP_DECODER_IMM(and)
ARM_DP_DECODER_REG(and)
ARM_DP_DECODER_IMM(eor)
ARM_DP_DECODER_REG(eor)
ARM_DP_DECODER_IMM(sub)
ARM_DP_DECODER_REG(sub)
ARM_DP_DECODER_IMM(rsb)
ARM_DP_DECODER_REG(rsb)
ARM_DP_DECODER_IMM(add)
ARM_DP_DECODER_REG(add)
ARM_DP_DECODER_IMM(adc)
ARM_DP_DECODER_REG(adc)
ARM_DP_DECODER_IMM(sbc)
ARM_DP_DECODER_REG(sbc)
ARM_DP_DECODER_IMM(rsc)
ARM_DP_DECODER_REG(rsc)
ARM_DP_DECODER_IMM(tst)
ARM_DP_DECODER_REG(tst)
ARM_DP_DECODER_IMM(teq)
ARM_DP_DECODER_REG(teq)
ARM_DP_DECODER_IMM(cmp)
ARM_DP_DECODER_REG(cmp)
ARM_DP_DECODER_IMM(cmn)
ARM_DP_DECODER_REG(cmn)
ARM_DP_DECODER_IMM(orr)
ARM_DP_DECODER_REG(orr)
ARM_DP_DECODER_IMM(bic)
ARM_DP_DECODER_REG(bic)
ARM_DP_DECODER_IMM(mvn)
ARM_DP_DECODER_REG(mvn)

// MOV immediate handler (already present)
// MOV register handler (already present)

// STR/LDR/STRB/LDRB decoders
// STR immediate
void decode_arm_str_imm(ARMCachedInstruction& decoded) {
    decoded.load = false;
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.immediate = true;
    decoded.offset_value = bits<11,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_str;
}

// STR register
void decode_arm_str_reg(ARMCachedInstruction& decoded) {
    decoded.load = false;
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.immediate = false;
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.offset_type = bits<6,5>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_str;
}

// LDR immediate
void decode_arm_ldr_imm(ARMCachedInstruction& decoded) {
    decoded.load = true;
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.immediate = true;
    decoded.offset_value = bits<11,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_ldr;
}

// LDR register
void decode_arm_ldr_reg(ARMCachedInstruction& decoded) {
    decoded.load = true;
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.immediate = false;
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.offset_type = bits<6,5>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_ldr;
}

void decode_arm_strb(ARMCachedInstruction& decoded) {
    decoded.load = false;
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.immediate = false; // This line is modified for clarity
    if (decoded.immediate) {
        decoded.offset_value = bits<11,0>(decoded.instruction);
    } else {
        decoded.rm = bits<3,0>(decoded.instruction);
        decoded.offset_type = bits<6,5>(decoded.instruction);
    }
    decoded.execute_func = arm_strb;
}
// STRB immediate
void decode_arm_strb_imm(ARMCachedInstruction& decoded) {
    decoded.load = false;
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.immediate = true;
    decoded.offset_value = bits<11,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_strb;
}

// STRB register
void decode_arm_strb_reg(ARMCachedInstruction& decoded) {
    decoded.load = false;
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.immediate = false;
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.offset_type = bits<6,5>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_strb;
}

void decode_arm_ldrb(ARMCachedInstruction& decoded) {
    decoded.load = true;
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.immediate = false; // This line is modified for clarity
    if (decoded.immediate) {
        decoded.offset_value = bits<11,0>(decoded.instruction);
    } else {
        decoded.rm = bits<3,0>(decoded.instruction);
        decoded.offset_type = bits<6,5>(decoded.instruction);
    }
    decoded.execute_func = arm_ldrb;
}
// LDRB immediate
void decode_arm_ldrb_imm(ARMCachedInstruction& decoded) {
    decoded.load = true;
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.immediate = true;
    decoded.offset_value = bits<11,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_ldrb;
}

// LDRB register
void decode_arm_ldrb_reg(ARMCachedInstruction& decoded) {
    decoded.load = true;
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.immediate = false;
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.offset_type = bits<6,5>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_ldrb;
}

// STM/LDM decoders
void decode_arm_stm(ARMCachedInstruction& decoded) {
    decoded.load = false;
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.register_list = bits<15,0>(decoded.instruction);
    decoded.addressing_mode = bits<23,22>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_stm;
}

void decode_arm_ldm(ARMCachedInstruction& decoded) {
    decoded.load = true;
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.register_list = bits<15,0>(decoded.instruction);
    decoded.addressing_mode = bits<23,22>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_ldm;
}

// Branch decoders
void decode_arm_b(ARMCachedInstruction& decoded) {
    int32_t offset = bits<23,0>(decoded.instruction);
    if (offset & 0x800000) offset |= 0xFF000000; // sign extend
    decoded.branch_offset = (offset << 2) + 8;
    decoded.link = false;
    decoded.execute_func = &ARMCPU::arm_b;
}

void decode_arm_bl(ARMCachedInstruction& decoded) {
    int32_t offset = bits<23,0>(decoded.instruction);
    if (offset & 0x800000) offset |= 0xFF000000; // sign extend
    decoded.branch_offset = (offset << 2) + 8;
    decoded.link = true;
    decoded.execute_func = &ARMCPU::arm_bl;
}

// SWP/SWPB decoders
void decode_arm_swp(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_swp;
}

void decode_arm_swpb(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_swpb;
}

// MUL/MLA/UMULL/UMLAL/SMULL/SMLAL decoders
void decode_arm_mul(ARMCachedInstruction& decoded) {
    decoded.rd = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.set_flags = (decoded.instruction >> 20) & 1;
    decoded.execute_func = &ARMCPU::arm_mul;
}

void decode_arm_mla(ARMCachedInstruction& decoded) {
    decoded.rd = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.rn = bits<15,12>(decoded.instruction);
    decoded.set_flags = (decoded.instruction >> 20) & 1;
    decoded.accumulate = true;
    decoded.execute_func = &ARMCPU::arm_mla;
}

void decode_arm_umull(ARMCachedInstruction& decoded) {
    decoded.rdHi = bits<19,16>(decoded.instruction);
    decoded.rdLo = bits<15,12>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.set_flags = (decoded.instruction >> 20) & 1;
    decoded.signed_op = false;
    decoded.execute_func = &ARMCPU::arm_umull;
}

void decode_arm_umlal(ARMCachedInstruction& decoded) {
    decoded.rdHi = bits<19,16>(decoded.instruction);
    decoded.rdLo = bits<15,12>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.set_flags = (decoded.instruction >> 20) & 1;
    decoded.signed_op = false;
    decoded.accumulate = true;
    decoded.execute_func = &ARMCPU::arm_umlal;
}

void decode_arm_smull(ARMCachedInstruction& decoded) {
    decoded.rdHi = bits<19,16>(decoded.instruction);
    decoded.rdLo = bits<15,12>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.set_flags = (decoded.instruction >> 20) & 1;
    decoded.signed_op = true;
    decoded.execute_func = &ARMCPU::arm_smull;
}

void decode_arm_smlal(ARMCachedInstruction& decoded) {
    decoded.rdHi = bits<19,16>(decoded.instruction);
    decoded.rdLo = bits<15,12>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.set_flags = (decoded.instruction >> 20) & 1;
    decoded.signed_op = true;
    decoded.accumulate = true;
    decoded.execute_func = &ARMCPU::arm_smlal;
}

// LDRH/STRH/LDRSB/LDRSH decoders
void decode_arm_ldrh(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_ldrh;
}

void decode_arm_strh(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_strh;
}

void decode_arm_ldrsb(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_ldrsb;
}

void decode_arm_ldrsh(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::arm_ldrsh;
}

// Undefined decoder
void decode_arm_undefined(ARMCachedInstruction& decoded) {
    decoded.execute_func = nullptr;
}

// MOV immediate handler
void decode_arm_mov_imm(ARMCachedInstruction& decoded) {
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rd = bits<15,12>(decoded.instruction);

    // Operand 2
    decoded.rotate_imm = bits<11,8>(decoded.instruction); 
    decoded.imm8 = bits<7,0>(decoded.instruction);
    
    decoded.execute_func = &ARMCPU::arm_mov_imm;
}

// MOV register handler
void decode_arm_mov_reg(ARMCachedInstruction& decoded) {
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rd = bits<15,12>(decoded.instruction);

    // Operand 2
    decoded.rs = bits<11,7>(decoded.instruction); // could be rs or imm depending on reg_shift
    decoded.shift_type = bits<6,5>(decoded.instruction);
    decoded.reg_shift = bits<4,4>(decoded.instruction); 
    decoded.rm = bits<3,0>(decoded.instruction);

    decoded.execute_func = &ARMCPU::arm_mov_reg;
}
