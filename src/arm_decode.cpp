#include "arm_cpu.h"
#include "debug.h"
#include <cstdio>

// ARM Data Processing decoders (AND, EOR, SUB, RSB, ADD, ADC, SBC, RSC, TST, TEQ, CMP, CMN, ORR, BIC, MVN) split between IMM and REG variants
// Use macros to generate these    
#define ARM_DP_DECODER_IMM(name) \
void ARMCPU::decode_arm_##name##_imm(ARMCachedInstruction& decoded) { \
    DEBUG_LOG(std::string("decode_arm_" #name "_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8)); \
    decoded.rn = bits<19,16>(decoded.instruction); \
    decoded.rd = bits<15,12>(decoded.instruction); \
    decoded.rotate = bits<11,8>(decoded.instruction) * 2; \
    decoded.imm = bits<7,0>(decoded.instruction); \
    decoded.execute_func = &ARMCPU::execute_arm_##name##_imm; \
}

#define ARM_DP_DECODER_REG(name) \
void ARMCPU::decode_arm_##name##_reg(ARMCachedInstruction& decoded) { \
    DEBUG_LOG(std::string("decode_arm_" #name "_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8)); \
    decoded.rn = bits<19,16>(decoded.instruction); \
    decoded.rd = bits<15,12>(decoded.instruction); \
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
    DEBUG_LOG(std::string("decode_arm_strb_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.imm = bits<11,0>(decoded.instruction);
    decoded.pre_index = bits<24,24>(decoded.instruction);
    decoded.up = bits<23,23>(decoded.instruction);
    decoded.writeback = bits<21,21>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_strb_imm;
}

void ARMCPU::decode_arm_strb_reg(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_strb_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.offset_type = bits<6,5>(decoded.instruction);
    decoded.pre_index = bits<24,24>(decoded.instruction);
    decoded.up = bits<23,23>(decoded.instruction);
    decoded.writeback = bits<21,21>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_strb_reg;
}

void ARMCPU::decode_arm_ldrb_imm(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_ldrb_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.imm = bits<11,0>(decoded.instruction);
    decoded.pre_index = bits<24,24>(decoded.instruction);
    decoded.up = bits<23,23>(decoded.instruction);
    decoded.writeback = bits<21,21>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_ldrb_imm;
}

void ARMCPU::decode_arm_ldrb_reg(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_ldrb_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.offset_type = bits<6,5>(decoded.instruction);
    decoded.pre_index = bits<24,24>(decoded.instruction);
    decoded.up = bits<23,23>(decoded.instruction);
    decoded.writeback = bits<21,21>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_ldrb_reg;
}

// STM/LDM decoders
void ARMCPU::decode_arm_stm(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_stm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.register_list = bits<15,0>(decoded.instruction);
    decoded.addressing_mode = bits<23,22>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_stm;
}

void ARMCPU::decode_arm_ldm(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_ldm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.register_list = bits<15,0>(decoded.instruction);
    decoded.addressing_mode = bits<23,22>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_ldm;
}

// Branch decoders
void ARMCPU::decode_arm_b(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_b: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    int32_t offset = bits<23,0>(decoded.instruction);
    if (offset & 0x800000) offset |= 0xFF000000; // sign extend
    decoded.branch_offset = (offset << 2) + 8;
    decoded.execute_func = &ARMCPU::execute_arm_b;
}

void ARMCPU::decode_arm_bl(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_bl: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    int32_t offset = bits<23,0>(decoded.instruction);
    if (offset & 0x800000) offset |= 0xFF000000; // sign extend
    decoded.branch_offset = (offset << 2) + 8;
    decoded.execute_func = &ARMCPU::execute_arm_bl;
}

// SWP/SWPB decoders
void ARMCPU::decode_arm_swp(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_swp: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_swp;
}

void ARMCPU::decode_arm_swpb(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_swpb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_swpb;
}

// MUL/MLA/UMULL/UMLAL/SMULL/SMLAL decoders
void ARMCPU::decode_arm_mul(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_mul: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rd = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_mul;
}

void ARMCPU::decode_arm_mla(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_mla: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rd = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.rn = bits<15,12>(decoded.instruction);
    decoded.accumulate = true;
    decoded.execute_func = &ARMCPU::execute_arm_mla;
}

void ARMCPU::decode_arm_umull(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_umull: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rdHi = bits<19,16>(decoded.instruction);
    decoded.rdLo = bits<15,12>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.signed_op = false;
    decoded.execute_func = &ARMCPU::execute_arm_umull;
}

void ARMCPU::decode_arm_umlal(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_umlal: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rdHi = bits<19,16>(decoded.instruction);
    decoded.rdLo = bits<15,12>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.signed_op = false;
    decoded.accumulate = true;
    decoded.execute_func = &ARMCPU::execute_arm_umlal;
}

void ARMCPU::decode_arm_smull(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_smull: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rdHi = bits<19,16>(decoded.instruction);
    decoded.rdLo = bits<15,12>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.signed_op = true;
    decoded.execute_func = &ARMCPU::execute_arm_smull;
}

void ARMCPU::decode_arm_smlal(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_smlal: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rdHi = bits<19,16>(decoded.instruction);
    decoded.rdLo = bits<15,12>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.signed_op = true;
    decoded.accumulate = true;
    decoded.execute_func = &ARMCPU::execute_arm_smlal;
}

// LDRH/STRH/LDRSB/LDRSH decoders
void ARMCPU::decode_arm_ldrh(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_ldrh: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_ldrh;
}

void ARMCPU::decode_arm_strh(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_strh: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_strh;
}

void ARMCPU::decode_arm_ldrsb(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_ldrsb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_ldrsb;
}

void ARMCPU::decode_arm_ldrsh(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_ldrsh: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_ldrsh;
}

// Undefined decoder
void ARMCPU::decode_arm_undefined(ARMCachedInstruction& decoded) {
    DEBUG_ERROR(std::string("decode_arm_undefined: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.execute_func = nullptr;
}

void ARMCPU::decode_arm_mov_imm(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_mov_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rotate = bits<11,8>(decoded.instruction) * 2; 
    decoded.imm = bits<7,0>(decoded.instruction);
    
    decoded.execute_func = &ARMCPU::execute_arm_mov_imm;
}

void ARMCPU::decode_arm_mov_reg(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_mov_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rs = bits<11,7>(decoded.instruction); // could be rs or imm depending on reg_shift
    decoded.shift_type = bits<6,5>(decoded.instruction);
    decoded.reg_shift = bits<4,4>(decoded.instruction); 
    decoded.rm = bits<3,0>(decoded.instruction);

    decoded.execute_func = &ARMCPU::execute_arm_mov_reg;
}

void ARMCPU::decode_arm_software_interrupt(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_software_interrupt: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    // SWI immediate is bits 23-0
    decoded.imm = bits<23,0>(decoded.instruction);
    decoded.execute_func = nullptr; // Not implemented, or set to handler if available
    DEBUG_ERROR(std::string("decode_arm_software_interrupt: SWI instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
}


void ARMCPU::decode_arm_str_imm(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.imm = bits<11,0>(decoded.instruction);
    decoded.pre_index = bits<24,24>(decoded.instruction);
    decoded.up = bits<23,23>(decoded.instruction);
    decoded.writeback = bits<21,21>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_str_imm;
}

void ARMCPU::decode_arm_str_reg(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.offset_type = bits<6,5>(decoded.instruction);
    decoded.pre_index = bits<24,24>(decoded.instruction);
    decoded.up = bits<23,23>(decoded.instruction);
    decoded.writeback = bits<21,21>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_str_reg;
}

void ARMCPU::decode_arm_ldr_imm(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.imm = bits<11,0>(decoded.instruction);
    decoded.pre_index = bits<24,24>(decoded.instruction);
    decoded.up = bits<23,23>(decoded.instruction);
    decoded.writeback = bits<21,21>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_ldr_imm;
}

void ARMCPU::decode_arm_ldr_reg(ARMCachedInstruction& decoded) {
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.offset_type = bits<6,5>(decoded.instruction);
    decoded.pre_index = bits<24,24>(decoded.instruction);
    decoded.up = bits<23,23>(decoded.instruction);
    decoded.writeback = bits<21,21>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_ldr_reg;
}

// LDC (Load to Coprocessor from Memory, Immediate) decoder stub
void ARMCPU::decode_arm_ldc_imm(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_ldc_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rs = bits<11,8>(decoded.instruction); // Coprocessor number
    decoded.execute_func = nullptr; // Not implemented
    DEBUG_ERROR(std::string("decode_arm_ldc_imm: Coprocessor LDC (imm) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
}

// LDC (Load to Coprocessor from Memory, Register) decoder stub
void ARMCPU::decode_arm_ldc_reg(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_ldc_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rs = bits<11,8>(decoded.instruction); // Coprocessor number
    decoded.execute_func = nullptr; // Not implemented
    DEBUG_ERROR(std::string("decode_arm_ldc_reg: Coprocessor LDC (reg) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
}

// STC (Store from Coprocessor to Memory, Immediate) decoder stub
void ARMCPU::decode_arm_stc_imm(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_stc_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rs = bits<11,8>(decoded.instruction); // Coprocessor number
    decoded.execute_func = nullptr; // Not implemented
    DEBUG_ERROR(std::string("decode_arm_stc_imm: Coprocessor STC (imm) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
}

// STC (Store from Coprocessor to Memory, Register) decoder stub
void ARMCPU::decode_arm_stc_reg(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_stc_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rs = bits<11,8>(decoded.instruction); // Coprocessor number
    decoded.execute_func = nullptr; // Not implemented
    DEBUG_ERROR(std::string("decode_arm_stc_reg: Coprocessor STC (reg) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
}