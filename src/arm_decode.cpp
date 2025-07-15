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
void ARMCPU::decode_arm_##name##_imm(ARMCachedInstruction& decoded) { \
    decoded.dp_op = static_cast<ARMDataProcessingOp>((decoded.instruction >> 21) & 0xF); \
    decoded.set_flags = (decoded.instruction >> 20) & 1; \
    decoded.rn = bits<19,16>(decoded.instruction); \
    decoded.rd = bits<15,12>(decoded.instruction); \
    decoded.immediate = true; \
    decoded.rotate_imm = bits<11,8>(decoded.instruction); \
    decoded.imm8 = bits<7,0>(decoded.instruction); \
    decoded.execute_func = &ARMCPU::execute_arm_##name##_imm; \
}

#define ARM_DP_DECODER_REG(name) \
void ARMCPU::decode_arm_##name##_reg(ARMCachedInstruction& decoded) { \
    decoded.dp_op = static_cast<ARMDataProcessingOp>((decoded.instruction >> 21) & 0xF); \
    decoded.set_flags = (decoded.instruction >> 20) & 1; \
    decoded.rn = bits<19,16>(decoded.instruction); \
    decoded.rd = bits<15,12>(decoded.instruction); \
    decoded.immediate = false; \
    decoded.rs = bits<11,8>(decoded.instruction); \
    decoded.shift_type = bits<6,5>(decoded.instruction); \
    decoded.reg_shift = bits<4,4>(decoded.instruction); \
    decoded.rm = bits<3,0>(decoded.instruction); \
    decoded.execute_func = &ARMCPU::execute_arm_##name##_reg; \
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

void ARMCPU::decode_arm_strb_imm(ARMCachedInstruction& decoded) {
    decoded.load = false;
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.immediate = true;
    decoded.offset_value = bits<11,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_strb_imm;
}

void ARMCPU::decode_arm_strb_reg(ARMCachedInstruction& decoded) {
    decoded.load = false;
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.immediate = false;
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.offset_type = bits<6,5>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_strb_reg;
}

void ARMCPU::decode_arm_ldrb_imm(ARMCachedInstruction& decoded) {
    decoded.load = true;
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.immediate = true;
    decoded.offset_value = bits<11,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_ldrb_imm;
}

void ARMCPU::decode_arm_ldrb_reg(ARMCachedInstruction& decoded) {
    decoded.load = true;
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.immediate = false;
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.offset_type = bits<6,5>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_ldrb_reg;
}

// STM/LDM decoders
void ARMCPU::decode_arm_stm(ARMCachedInstruction& decoded) {
    decoded.load = false;
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.register_list = bits<15,0>(decoded.instruction);
    decoded.addressing_mode = bits<23,22>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_stm;
}

void ARMCPU::decode_arm_ldm(ARMCachedInstruction& decoded) {
    decoded.load = true;
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.register_list = bits<15,0>(decoded.instruction);
    decoded.addressing_mode = bits<23,22>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_ldm;
}

// Branch decoders
void ARMCPU::decode_arm_b(ARMCachedInstruction& decoded) {
    int32_t offset = bits<23,0>(decoded.instruction);
    if (offset & 0x800000) offset |= 0xFF000000; // sign extend
    decoded.branch_offset = (offset << 2) + 8;
    decoded.link = false;
    decoded.execute_func = &ARMCPU::execute_arm_b;
}

void ARMCPU::decode_arm_bl(ARMCachedInstruction& decoded) {
    int32_t offset = bits<23,0>(decoded.instruction);
    if (offset & 0x800000) offset |= 0xFF000000; // sign extend
    decoded.branch_offset = (offset << 2) + 8;
    decoded.link = true;
    decoded.execute_func = &ARMCPU::execute_arm_bl;
}

// SWP/SWPB decoders
void ARMCPU::decode_arm_swp(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_swp;
}

void ARMCPU::decode_arm_swpb(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_swpb;
}

// MUL/MLA/UMULL/UMLAL/SMULL/SMLAL decoders
void ARMCPU::decode_arm_mul(ARMCachedInstruction& decoded) {
    decoded.rd = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.set_flags = (decoded.instruction >> 20) & 1;
    decoded.execute_func = &ARMCPU::execute_arm_mul;
}

void ARMCPU::decode_arm_mla(ARMCachedInstruction& decoded) {
    decoded.rd = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.rn = bits<15,12>(decoded.instruction);
    decoded.set_flags = (decoded.instruction >> 20) & 1;
    decoded.accumulate = true;
    decoded.execute_func = &ARMCPU::execute_arm_mla;
}

void ARMCPU::decode_arm_umull(ARMCachedInstruction& decoded) {
    decoded.rdHi = bits<19,16>(decoded.instruction);
    decoded.rdLo = bits<15,12>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.set_flags = (decoded.instruction >> 20) & 1;
    decoded.signed_op = false;
    decoded.execute_func = &ARMCPU::execute_arm_umull;
}

void ARMCPU::decode_arm_umlal(ARMCachedInstruction& decoded) {
    decoded.rdHi = bits<19,16>(decoded.instruction);
    decoded.rdLo = bits<15,12>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.set_flags = (decoded.instruction >> 20) & 1;
    decoded.signed_op = false;
    decoded.accumulate = true;
    decoded.execute_func = &ARMCPU::execute_arm_umlal;
}

void ARMCPU::decode_arm_smull(ARMCachedInstruction& decoded) {
    decoded.rdHi = bits<19,16>(decoded.instruction);
    decoded.rdLo = bits<15,12>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.set_flags = (decoded.instruction >> 20) & 1;
    decoded.signed_op = true;
    decoded.execute_func = &ARMCPU::execute_arm_smull;
}

void ARMCPU::decode_arm_smlal(ARMCachedInstruction& decoded) {
    decoded.rdHi = bits<19,16>(decoded.instruction);
    decoded.rdLo = bits<15,12>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.set_flags = (decoded.instruction >> 20) & 1;
    decoded.signed_op = true;
    decoded.accumulate = true;
    decoded.execute_func = &ARMCPU::execute_arm_smlal;
}

// LDRH/STRH/LDRSB/LDRSH decoders
void ARMCPU::decode_arm_ldrh(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_ldrh;
}

void ARMCPU::decode_arm_strh(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_strh;
}

void ARMCPU::decode_arm_ldrsb(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_ldrsb;
}

void ARMCPU::decode_arm_ldrsh(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_ldrsh;
}

// Undefined decoder
void ARMCPU::decode_arm_undefined(ARMCachedInstruction& decoded) {
    decoded.execute_func = nullptr;
}

// MOV immediate handler
void ARMCPU::decode_arm_mov_imm(ARMCachedInstruction& decoded) {
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rd = bits<15,12>(decoded.instruction);

    // Operand 2
    decoded.rotate_imm = bits<11,8>(decoded.instruction); 
    decoded.imm8 = bits<7,0>(decoded.instruction);
    
    decoded.execute_func = &ARMCPU::execute_arm_mov_imm;
}

// MOV register handler
void ARMCPU::decode_arm_mov_reg(ARMCachedInstruction& decoded) {
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rd = bits<15,12>(decoded.instruction);

    // Operand 2
    decoded.rs = bits<11,7>(decoded.instruction); // could be rs or imm depending on reg_shift
    decoded.shift_type = bits<6,5>(decoded.instruction);
    decoded.reg_shift = bits<4,4>(decoded.instruction); 
    decoded.rm = bits<3,0>(decoded.instruction);

    decoded.execute_func = &ARMCPU::execute_arm_mov_reg;
}
