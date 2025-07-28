#include "arm_cpu.h"
#include "debug.h"
#include <cstdio>

void ARMCPU::exec_arm_ldrb_reg_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrb_reg_pre_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rd] = parentCPU.getMemory().read8(addr);
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldrb_reg_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrb_reg_pre_nowb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rd] = parentCPU.getMemory().read8(addr);
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldrb_reg_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrb_reg_post: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    parentCPU.R()[rd] = parentCPU.getMemory().read8(base);
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_str_imm_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_str_imm_pre_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.getMemory().write32(addr, parentCPU.R()[rd]);
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_str_imm_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_str_imm_pre_nowb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.getMemory().write32(addr, parentCPU.R()[rd]);
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_str_imm_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_str_imm_post_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    parentCPU.getMemory().write32(base, parentCPU.R()[rd]);
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_str_reg_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_str_reg_pre: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.getMemory().write32(addr, parentCPU.R()[rd]);
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_str_reg_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_str_reg_pre: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.getMemory().write32(addr, parentCPU.R()[rd]);
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_str_reg_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_str_reg_post: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    parentCPU.getMemory().write32(base, parentCPU.R()[rd]);
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldr_imm_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldr_imm_pre: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rd] = parentCPU.getMemory().read32(addr);
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldr_imm_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldr_imm_pre: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rd] = parentCPU.getMemory().read32(addr);
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldr_imm_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldr_imm_post_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    parentCPU.R()[rd] = parentCPU.getMemory().read32(base);
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldr_reg_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldr_reg_pre_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rd] = parentCPU.getMemory().read32(addr);
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldr_reg_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldr_reg_pre_nowb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rd] = parentCPU.getMemory().read32(addr);
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldr_reg_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldr_reg_post_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rd] = parentCPU.getMemory().read32(base);
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_strb_imm_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strb_imm_pre_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.getMemory().write8(addr, parentCPU.R()[rd] & 0xFF);
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_strb_imm_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strb_imm_pre_nowb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.getMemory().write8(addr, parentCPU.R()[rd] & 0xFF);
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_strb_imm_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strb_imm_post_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    parentCPU.getMemory().write8(base, parentCPU.R()[rd] & 0xFF);
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_strb_reg_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strb_reg_pre_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.getMemory().write8(addr, parentCPU.R()[rd] & 0xFF);
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_strb_reg_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strb_reg_pre_nowb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.getMemory().write8(addr, parentCPU.R()[rd] & 0xFF);
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_strb_reg_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strb_reg_post_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.getMemory().write8(addr, parentCPU.R()[rd] & 0xFF);
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldrb_imm_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrb_imm_pre_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rd] = parentCPU.getMemory().read8(addr);
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldrb_imm_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrb_imm_pre_nowb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rd] = parentCPU.getMemory().read8(addr);
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldrb_imm_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrb_imm_post_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    parentCPU.R()[rd] = parentCPU.getMemory().read8(base);
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rn] = addr;  // Writeback
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldrh(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrh: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t addr = parentCPU.R()[rn] + parentCPU.R()[rm];
    parentCPU.R()[rd] = parentCPU.getMemory().read16(addr);
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_strh(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strh: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t addr = parentCPU.R()[rn] + parentCPU.R()[rm];
    parentCPU.getMemory().write16(addr, parentCPU.R()[rd] & 0xFFFF);
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldrsb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrsb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t addr = parentCPU.R()[rn] + parentCPU.R()[rm];
    int8_t val = parentCPU.getMemory().read8(addr);
    parentCPU.R()[rd] = (int32_t)val;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldrsh(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrsh: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t addr = parentCPU.R()[rn] + parentCPU.R()[rm];
    int16_t val = parentCPU.getMemory().read16(addr);
    parentCPU.R()[rd] = (int32_t)val;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}
