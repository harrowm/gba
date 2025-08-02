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
    // Store at original base address
    parentCPU.getMemory().write8(base, parentCPU.R()[rd] & 0xFF);
    // Writeback after store
    parentCPU.R()[rn] = up ? base + offset : base - offset;
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

void ARMCPU::exec_arm_ldrh_reg_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrh_reg_pre_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rd] = parentCPU.getMemory().read16(addr);
    parentCPU.R()[rn] = addr; // Writeback
    if (rd != 15) parentCPU.R()[15] += 4;
}

void ARMCPU::exec_arm_ldrh_reg_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrh_reg_pre_nowb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rd] = parentCPU.getMemory().read16(addr);
    if (rd != 15) parentCPU.R()[15] += 4;
}

void ARMCPU::exec_arm_ldrh_reg_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrh_reg_post_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    parentCPU.R()[rd] = parentCPU.getMemory().read16(base);
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rn] = addr; // Writeback
    if (rd != 15) parentCPU.R()[15] += 4;
}

void ARMCPU::exec_arm_strh_reg_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strh_reg_pre_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.getMemory().write16(addr, parentCPU.R()[rd] & 0xFFFF);
    parentCPU.R()[rn] = addr; // Writeback
    if (rd != 15) parentCPU.R()[15] += 4;
}

void ARMCPU::exec_arm_strh_reg_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strh_reg_pre_nowb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.getMemory().write16(addr, parentCPU.R()[rd] & 0xFFFF);
    if (rd != 15) parentCPU.R()[15] += 4;
}

void ARMCPU::exec_arm_strh_reg_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strh_reg_post_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    parentCPU.getMemory().write16(base, parentCPU.R()[rd] & 0xFFFF);
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rn] = addr; // Writeback
    if (rd != 15) parentCPU.R()[15] += 4;
}

// LDRSB register pre-indexed with writeback
void ARMCPU::exec_arm_ldrsb_reg_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrsb_reg_pre_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    int8_t val = parentCPU.getMemory().read8(addr);
    parentCPU.R()[rd] = (int32_t)val;
    parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4;
}

// LDRSB register pre-indexed, no writeback
void ARMCPU::exec_arm_ldrsb_reg_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrsb_reg_pre_nowb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) +
        ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    int8_t val = parentCPU.getMemory().read8(addr);
    parentCPU.R()[rd] = (int32_t)val;
    if (rd != 15) parentCPU.R()[15] += 4;
}

// LDRSB register post-indexed
void ARMCPU::exec_arm_ldrsb_reg_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrsb_reg_post_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) +
        ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    int8_t val = parentCPU.getMemory().read8(base);
    parentCPU.R()[rd] = (int32_t)val;
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4;
}

// LDRSB immediate pre-indexed with writeback
void ARMCPU::exec_arm_ldrsb_imm_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrsb_imm_pre_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) +
        ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = (bits<11,8>(instruction) << 4) | bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    int8_t val = parentCPU.getMemory().read8(addr);
    parentCPU.R()[rd] = (int32_t)val;
    parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4;
}

// LDRSB immediate pre-indexed, no writeback
void ARMCPU::exec_arm_ldrsb_imm_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrsb_imm_pre_nowb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) +
        ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = (bits<11,8>(instruction) << 4) | bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    int8_t val = parentCPU.getMemory().read8(addr);
    parentCPU.R()[rd] = (int32_t)val;
    if (rd != 15) parentCPU.R()[15] += 4;
}

// LDRSB immediate post-indexed
void ARMCPU::exec_arm_ldrsb_imm_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrsb_imm_post_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) +
        ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = (bits<11,8>(instruction) << 4) | bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    int8_t val = parentCPU.getMemory().read8(base);
    parentCPU.R()[rd] = (int32_t)val;
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4;
}

// LDRSH register pre-indexed with writeback
void ARMCPU::exec_arm_ldrsh_reg_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrsh_reg_pre_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) +
        ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    int16_t val = parentCPU.getMemory().read16(addr);
    DEBUG_LOG(std::string("  base=0x") + DEBUG_TO_HEX_STRING(base, 8) + ", offset=0x" + DEBUG_TO_HEX_STRING(offset, 8) + ", addr=0x" + DEBUG_TO_HEX_STRING(addr, 8));
    DEBUG_LOG(std::string("  loaded (int16_t)val=") + std::to_string(val));
    parentCPU.R()[rd] = (int32_t)val;
    parentCPU.R()[rn] = addr;
    DEBUG_LOG(std::string("  rd (R[") + std::to_string(rd) + "] = 0x" + DEBUG_TO_HEX_STRING(parentCPU.R()[rd], 8));
    DEBUG_LOG(std::string("  rn (R[") + std::to_string(rn) + "] = 0x" + DEBUG_TO_HEX_STRING(parentCPU.R()[rn], 8));
    if (rd != 15) parentCPU.R()[15] += 4;
}

// LDRSH register pre-indexed, no writeback
void ARMCPU::exec_arm_ldrsh_reg_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrsh_reg_pre_nowb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) +
        ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    int16_t val = parentCPU.getMemory().read16(addr);
    DEBUG_LOG(std::string("  base=0x") + DEBUG_TO_HEX_STRING(base, 8) + ", offset=0x" + DEBUG_TO_HEX_STRING(offset, 8) + ", addr=0x" + DEBUG_TO_HEX_STRING(addr, 8));
    DEBUG_LOG(std::string("  loaded (int16_t)val=") + std::to_string(val));
    parentCPU.R()[rd] = (int32_t)val;
    DEBUG_LOG(std::string("  rd (R[") + std::to_string(rd) + "] = 0x" + DEBUG_TO_HEX_STRING(parentCPU.R()[rd], 8));
    if (rd != 15) parentCPU.R()[15] += 4;
}

// LDRSH register post-indexed
void ARMCPU::exec_arm_ldrsh_reg_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrsh_reg_post_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) +
        ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    int16_t val = parentCPU.getMemory().read16(base);
    parentCPU.R()[rd] = (int32_t)val;
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4;
}

// LDRSH immediate pre-indexed with writeback
void ARMCPU::exec_arm_ldrsh_imm_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrsh_imm_pre_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = (bits<11,8>(instruction) << 4) | bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    int16_t val = parentCPU.getMemory().read16(addr);
    parentCPU.R()[rd] = (int32_t)val;
    parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4;
}

// LDRSH immediate pre-indexed, no writeback
void ARMCPU::exec_arm_ldrsh_imm_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrsh_imm_pre_nowb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = (bits<11,8>(instruction) << 4) | bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    int16_t val = parentCPU.getMemory().read16(addr);
    parentCPU.R()[rd] = (int32_t)val;
    if (rd != 15) parentCPU.R()[15] += 4;
}

// LDRSH immediate post-indexed
void ARMCPU::exec_arm_ldrsh_imm_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrsh_imm_post_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = (bits<11,8>(instruction) << 4) | bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    int16_t val = parentCPU.getMemory().read16(base);
    parentCPU.R()[rd] = (int32_t)val;
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4;
}

// LDRH immediate offset variants
void ARMCPU::exec_arm_ldrh_imm_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrh_imm_pre_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) +
        ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = (bits<11,8>(instruction) << 4) | bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rd] = parentCPU.getMemory().read16(addr);
    parentCPU.R()[rn] = addr; // Writeback
    if (rd != 15) parentCPU.R()[15] += 4;
}

void ARMCPU::exec_arm_ldrh_imm_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrh_imm_pre_nowb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = (bits<11,8>(instruction) << 4) | bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rd] = parentCPU.getMemory().read16(addr);
    if (rd != 15) parentCPU.R()[15] += 4;
}

void ARMCPU::exec_arm_ldrh_imm_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrh_imm_post_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = (bits<11,8>(instruction) << 4) | bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    parentCPU.R()[rd] = parentCPU.getMemory().read16(base);
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rn] = addr; // Writeback
    if (rd != 15) parentCPU.R()[15] += 4;
}

// STRH immediate offset variants
void ARMCPU::exec_arm_strh_imm_pre_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strh_imm_pre_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = (bits<11,8>(instruction) << 4) | bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.getMemory().write16(addr, parentCPU.R()[rd] & 0xFFFF);
    parentCPU.R()[rn] = addr; // Writeback
    if (rd != 15) parentCPU.R()[15] += 4;
}

void ARMCPU::exec_arm_strh_imm_pre_nowb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strh_imm_pre_nowb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = (bits<11,8>(instruction) << 4) | bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.getMemory().write16(addr, parentCPU.R()[rd] & 0xFFFF);
    if (rd != 15) parentCPU.R()[15] += 4;
}

void ARMCPU::exec_arm_strh_imm_post_wb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strh_imm_post_wb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = (bits<11,8>(instruction) << 4) | bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    uint32_t base = parentCPU.R()[rn];
    parentCPU.getMemory().write16(base, parentCPU.R()[rd] & 0xFFFF);
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rn] = addr; // Writeback
    if (rd != 15) parentCPU.R()[15] += 4;
}