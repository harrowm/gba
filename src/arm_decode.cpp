#include "arm_cpu.h"
#include "debug.h"
#include <cstdio>

// Update flags after subtraction: N, Z, C, V
FORCE_INLINE void ARMCPU::updateFlagsSub(uint32_t op1, uint32_t op2, uint32_t result) {
    uint32_t n = (result >> 31) & 1;
    uint32_t z = (result == 0) ? 1 : 0;
    uint32_t c = (op1 >= op2) ? 1 : 0; // Carry: no borrow
    // Overflow: if sign(op1) != sign(op2) and sign(op1) != sign(result)
    uint32_t v = (((op1 ^ op2) & (op1 ^ result)) >> 31) & 1;
    uint32_t cpsr = parentCPU.CPSR();
    cpsr = (cpsr & ~(1u << 31)) | (n << 31); // N
    cpsr = (cpsr & ~(1u << 30)) | (z << 30); // Z
    cpsr = (cpsr & ~(1u << 29)) | (c << 29); // C
    cpsr = (cpsr & ~(1u << 28)) | (v << 28); // V
    parentCPU.CPSR() = cpsr;
}

// Update flags after addition: N, Z, C, V
FORCE_INLINE void ARMCPU::updateFlagsAdd(uint32_t op1, uint32_t op2, uint32_t result) {
    uint32_t n = (result >> 31) & 1;
    uint32_t z = (result == 0) ? 1 : 0;
    uint32_t c = (result < op1) ? 1 : 0;
    uint32_t v = (~(op1 ^ op2) & (op1 ^ result) >> 31) & 1;
    uint32_t cpsr = parentCPU.CPSR();
    cpsr = (cpsr & ~(1u << 31)) | (n << 31); // N
    cpsr = (cpsr & ~(1u << 30)) | (z << 30); // Z
    cpsr = (cpsr & ~(1u << 29)) | (c << 29); // C
    cpsr = (cpsr & ~(1u << 28)) | (v << 28); // V
    parentCPU.CPSR() = cpsr;
}

FORCE_INLINE void ARMCPU::updateFlagsLogical(uint32_t result, uint32_t carry) {
    // N flag: set if result is negative
    uint32_t n = (result >> 31) & 1;
    // Z flag: set if result is zero
    uint32_t z = (result == 0) ? 1 : 0;
    // C flag: use provided carry value (if meaningful for the operation)
    uint32_t cpsr = parentCPU.CPSR();
    cpsr = (cpsr & ~(1u << 31)) | (n << 31); // N
    cpsr = (cpsr & ~(1u << 30)) | (z << 30); // Z
    cpsr = (cpsr & ~(1u << 29)) | ((carry & 1) << 29); // C
    // V flag is not affected by logical ops
    parentCPU.CPSR() = cpsr;
}

// Helper for ARM register shift operations
FORCE_INLINE uint32_t ARMCPU::arm_shift(uint32_t value, uint8_t shift_type, uint32_t shift_val) {
    switch (shift_type) {
        case 0: // LSL
            return value << shift_val;
        case 1: // LSR
            return shift_val ? (value >> shift_val) : 0;
        case 2: // ASR
            return shift_val ? ((int32_t)value >> shift_val) : (value & 0x80000000 ? 0xFFFFFFFF : 0);
        case 3: // ROR
            return shift_val ? ((value >> shift_val) | (value << (32 - shift_val))) : value;
        default:
            return value;
    }
}

void ARMCPU::exec_arm_eor_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_eor_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    parentCPU.R()[rd] = parentCPU.R()[rn] ^ value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
    }
}

void ARMCPU::exec_arm_eor_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_eor_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    value = arm_shift(value, shift_type, shift_val);
    parentCPU.R()[rd] = parentCPU.R()[rn] ^ value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
    }
}

void ARMCPU::exec_arm_and_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_and_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    parentCPU.R()[rd] = parentCPU.R()[rn] & value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
    }
}

void ARMCPU::exec_arm_and_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_and_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    value = arm_shift(value, shift_type, shift_val);
    parentCPU.R()[rd] = parentCPU.R()[rn] & value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
    }
}

void ARMCPU::exec_arm_ldrb_reg_pre(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrb_reg_pre: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rd] = parentCPU.getMemory().read8(addr);
    if (writeback) parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldrb_reg_post(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrb_reg_post: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    parentCPU.R()[rd] = parentCPU.getMemory().read8(base);
    uint32_t addr = up ? base + offset : base - offset;
    if (writeback) parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_sub_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_sub_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t result = parentCPU.R()[rn] - value;
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(parentCPU.R()[rn], value, result);
    }
}

void ARMCPU::exec_arm_rsb_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_rsb_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t result = value - parentCPU.R()[rn];
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(value, parentCPU.R()[rn], result);
    }
}

void ARMCPU::exec_arm_sub_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_sub_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    value = arm_shift(value, shift_type, shift_val);
    uint32_t result = parentCPU.R()[rn] - value;
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(parentCPU.R()[rn], value, result);
    }
}

void ARMCPU::exec_arm_rsb_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_rsb_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    value = arm_shift(value, shift_type, shift_val);
    uint32_t result = value - parentCPU.R()[rn];
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(value, parentCPU.R()[rn], result);
    }
}

void ARMCPU::exec_arm_add_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_add_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t result = parentCPU.R()[rn] + value;
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsAdd(parentCPU.R()[rn], value, result);
    }
}

// ORR (logical OR)
void ARMCPU::exec_arm_orr_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_orr_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    parentCPU.R()[rd] = parentCPU.R()[rn] | value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
    }
}

void ARMCPU::exec_arm_orr_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_orr_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    value = arm_shift(value, shift_type, shift_val);
    parentCPU.R()[rd] = parentCPU.R()[rn] | value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
    }
}

void ARMCPU::exec_arm_bic_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_bic_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    parentCPU.R()[rd] = parentCPU.R()[rn] & ~value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
    }
}

void ARMCPU::exec_arm_bic_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_bic_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    value = arm_shift(value, shift_type, shift_val);
    parentCPU.R()[rd] = parentCPU.R()[rn] & ~value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
    }
}

void ARMCPU::exec_arm_mvn_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_mvn_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    parentCPU.R()[rd] = ~value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
    }
}

void ARMCPU::exec_arm_mvn_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_mvn_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    value = arm_shift(value, shift_type, shift_val);
    parentCPU.R()[rd] = ~value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
    }
}

void ARMCPU::exec_arm_add_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_add_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    value = arm_shift(value, shift_type, shift_val);
    uint32_t result = parentCPU.R()[rn] + value;
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsAdd(parentCPU.R()[rn], value, result);
    }
}

void ARMCPU::exec_arm_adc_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_adc_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    uint32_t result = parentCPU.R()[rn] + value + carry;
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsAdd(parentCPU.R()[rn], value + carry, result);
    }
}

void ARMCPU::exec_arm_adc_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_adc_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    value = arm_shift(value, shift_type, shift_val);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    uint32_t result = parentCPU.R()[rn] + value + carry;
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsAdd(parentCPU.R()[rn], value + carry, result);
    }
}

void ARMCPU::exec_arm_sbc_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_sbc_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    uint32_t result = parentCPU.R()[rn] - value - (1 - carry);
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(parentCPU.R()[rn], value + (1 - carry), result);
    }
}

void ARMCPU::exec_arm_sbc_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_sbc_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    value = arm_shift(value, shift_type, shift_val);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    uint32_t result = parentCPU.R()[rn] - value - (1 - carry);
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(parentCPU.R()[rn], value + (1 - carry), result);
    }
}

void ARMCPU::exec_arm_rsc_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_rsc_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    uint32_t result = value - parentCPU.R()[rn] - (1 - carry);
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(value, parentCPU.R()[rn] + (1 - carry), result);
    }
}

void ARMCPU::exec_arm_rsc_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_rsc_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    value = arm_shift(value, shift_type, shift_val);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    uint32_t result = value - parentCPU.R()[rn] - (1 - carry);
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(value, parentCPU.R()[rn] + (1 - carry), result);
    }
}

void ARMCPU::exec_arm_tst_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_tst_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t result = parentCPU.R()[rn] & value;
    // Update flags, especially Z
    updateFlagsLogical(result, 0);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_tst_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_tst_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);

    // MRS and MSR were added to the ARM instruction set late and reuse TST TEQ CMN and CMP with rn==15
    if (rn == 15) {
        DEBUG_INFO("TST with rn=15, divert to MRS");
        exec_arm_mrs(instruction);
        return;
    }

    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    value = arm_shift(value, shift_type, shift_val);
    uint32_t result = parentCPU.R()[rn] & value;
    // Update flags, especially Z
    updateFlagsLogical(result, 0);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_teq_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_teq_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);

    // MRS and MSR were added to the ARM instruction set late and reuse TST TEQ CMN and CMP with rn==15
    if (rn == 15) {
        DEBUG_INFO("TEQ with rn=15, divert to MSR");
        exec_arm_msr(instruction);
        return;
    }

    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t result = parentCPU.R()[rn] ^ value;
    // Update flags, especially Z
    updateFlagsLogical(result, 0);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

// CMP (compare, subtract, sets flags, does not store result)
void ARMCPU::exec_arm_cmp_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_cmp_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t result = parentCPU.R()[rn] - value;
    updateFlagsSub(parentCPU.R()[rn], value, result);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_cmp_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_cmp_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    
    // MRS and MSR were added to the ARM instruction set late and reuse TST TEQ CMN and CMP with rn==15
    if (rn == 15) {
        DEBUG_INFO("CMP REG with rn=15, divert to MRS");
        exec_arm_mrs(instruction);
        return;
    }

    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    value = arm_shift(value, shift_type, shift_val);
    uint32_t result = parentCPU.R()[rn] - value;
    updateFlagsSub(parentCPU.R()[rn], value, result);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

// CMN (compare negative, adds, sets flags, does not store result)
void ARMCPU::exec_arm_cmn_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_cmn_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t result = parentCPU.R()[rn] + value;
    updateFlagsAdd(parentCPU.R()[rn], value, result);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_cmn_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_cmn_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);

    // MRS and MSR were added to the ARM instruction set late and reuse TST TEQ CMN and CMP with rn==15
    if (rn == 15) {
        DEBUG_INFO("CMN REG with rn=15, divert to MSR");
        exec_arm_msr(instruction);
        return;
    }

    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    value = arm_shift(value, shift_type, shift_val);
    uint32_t result = parentCPU.R()[rn] + value;
    updateFlagsAdd(parentCPU.R()[rn], value, result);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_teq_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_teq_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);

    // MRS and MSR were added to the ARM instruction set late and reuse TST TEQ CMN and CMP with rn==15
    if (rn == 15) {
        DEBUG_INFO("TEQ REG with rn=15, divert to MSR");
        exec_arm_msr(instruction);
        return;
    }

    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    value = arm_shift(value, shift_type, shift_val);
    uint32_t result = parentCPU.R()[rn] ^ value;
    // Update flags, especially Z
    updateFlagsLogical(result, 0);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_str_imm_pre(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_str_imm_pre: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.getMemory().write32(addr, parentCPU.R()[rd]);
    if (writeback) {
        parentCPU.R()[rn] = addr;
    }
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_str_imm_post(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_str_imm_post: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    parentCPU.getMemory().write32(base, parentCPU.R()[rd]);
    uint32_t addr = up ? base + imm : base - imm;
    if (writeback) parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_str_reg_pre(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_str_reg_pre: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.getMemory().write32(addr, parentCPU.R()[rd]);
    if (writeback) parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_str_reg_post(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_str_reg_post: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    parentCPU.getMemory().write32(base, parentCPU.R()[rd]);
    uint32_t addr = up ? base + offset : base - offset;
    if (writeback) parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldr_imm_pre(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldr_imm_pre: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rd] = parentCPU.getMemory().read32(addr);
    if (writeback) parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldr_imm_post(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldr_imm_post: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    parentCPU.R()[rd] = parentCPU.getMemory().read32(base);
    uint32_t addr = up ? base + imm : base - imm;
    if (writeback) parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldr_reg_pre(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldr_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rd] = parentCPU.getMemory().read32(addr);
    if (writeback) parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldr_reg_post(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldr_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.R()[rd] = parentCPU.getMemory().read32(base);
    if (writeback) parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_strb_imm_pre(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strb_imm_pre: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.getMemory().write8(addr, parentCPU.R()[rd] & 0xFF);
    if (writeback) parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_strb_imm_post(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strb_imm_post: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    parentCPU.getMemory().write8(base, parentCPU.R()[rd] & 0xFF);
    uint32_t addr = up ? base + imm : base - imm;
    if (writeback) parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_strb_reg_pre(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strb_reg_pre: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.getMemory().write8(addr, parentCPU.R()[rd] & 0xFF);
    if (writeback) parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_strb_reg_post(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_strb_reg_post: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t offset = parentCPU.R()[rm];
    uint32_t addr = up ? base + offset : base - offset;
    parentCPU.getMemory().write8(addr, parentCPU.R()[rd] & 0xFF);
    if (writeback) parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldrb_imm_pre(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrb_imm_pre: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    uint32_t addr = up ? base + imm : base - imm;
    parentCPU.R()[rd] = parentCPU.getMemory().read8(addr);
    if (writeback) parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldrb_imm_post(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldrb_imm_post: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint32_t imm = bits<11,0>(instruction);
    bool up = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    uint32_t base = parentCPU.R()[rn];
    parentCPU.R()[rd] = parentCPU.getMemory().read8(base);
    uint32_t addr = up ? base + imm : base - imm;
    if (writeback) parentCPU.R()[rn] = addr;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint16_t reg_list = bits<15,0>(instruction);
    uint8_t addressing_mode = bits<23,22>(instruction);
    bool writeback = bits<21,21>(instruction);

    uint32_t base = parentCPU.R()[rn];
    // int offset = 0;
    // Calculate offset direction and order based on addressing_mode
    bool up = (addressing_mode & 0x2) != 0;
    bool pre = (addressing_mode & 0x1) != 0;

    // int reg_count = 0;
    // for (int i = 0; i < 16; ++i) {
    //     if (reg_list & (1 << i)) ++reg_count;
    // }
    int reg_count = __builtin_popcount(reg_list);
    int addr = base;
    if (up) {
        addr += pre ? 4 : 0;
    } else {
        addr -= pre ? 4 : 0;
    }
    bool r15_updated = false;
    for (int i = 0; i < 16; ++i) {
        if (reg_list & (1 << i)) {
            parentCPU.R()[i] = parentCPU.getMemory().read32(addr);
            addr += up ? 4 : -4;
            if (i == 15) r15_updated = true;
        }
    }
    if (writeback) {
        parentCPU.R()[rn] = up ? base + reg_count * 4 : base - reg_count * 4;
    }
    if (!r15_updated) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_stm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_stm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint16_t reg_list = bits<15,0>(instruction);
    uint8_t addressing_mode = bits<23,22>(instruction);
    bool writeback = bits<21,21>(instruction);

    uint32_t base = parentCPU.R()[rn];
    // int offset = 0;
    // Calculate offset direction and order based on addressing_mode
    bool up = (addressing_mode & 0x2) != 0;
    bool pre = (addressing_mode & 0x1) != 0;
    // int reg_count = 0;
    // for (int i = 0; i < 16; ++i) {
    //     if (reg_list & (1 << i)) ++reg_count;
    // }
    int reg_count = __builtin_popcount(reg_list);
    int addr = base;
    if (up) {
        addr += pre ? 4 : 0;
    } else {
        addr -= pre ? 4 : 0;
    }
    bool r15_updated = false;
    for (int i = 0; i < 16; ++i) {
        if (reg_list & (1 << i)) {
            parentCPU.getMemory().write32(addr, parentCPU.R()[i]);
            addr += up ? 4 : -4;
            if (i == 15) r15_updated = true;
        }
    }
    if (writeback) {
        parentCPU.R()[rn] = up ? base + reg_count * 4 : base - reg_count * 4;
    }
    if (!r15_updated) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_b(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_b: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    int32_t offset = bits<23,0>(instruction);
    
    if (offset & 0x800000) offset |= 0xFF000000; // sign extend
    uint32_t branch_offset = (offset << 2) + 8;
    // Branch to target address
    parentCPU.R()[15] += branch_offset;
}

void ARMCPU::exec_arm_bl(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_bl: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    int32_t offset = bits<23,0>(instruction);
    if (offset & 0x800000) offset |= 0xFF000000; // sign extend
    uint32_t branch_offset = (offset << 2) + 8;
    // Set LR to return address (current PC + 4)
    parentCPU.R()[14] = parentCPU.R()[15] + 4;
    // Branch to target address
    parentCPU.R()[15] += branch_offset;
}

void ARMCPU::exec_arm_swp(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_swp: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);

    // Read word from memory address in rn
    uint32_t addr = parentCPU.R()[rn];
    uint32_t mem_val = parentCPU.getMemory().read32(addr);
    // Write value from rm to memory
    parentCPU.getMemory().write32(addr, parentCPU.R()[rm]);
    // Store original memory value in rd
    parentCPU.R()[rd] = mem_val;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_swpb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_swpb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);

    // Read byte from memory address in rn
    uint32_t addr = parentCPU.R()[rn];
    uint8_t mem_val = parentCPU.getMemory().read8(addr);
    // Write value from rm to memory
    parentCPU.getMemory().write8(addr, parentCPU.R()[rm] & 0xFF);
    // Store original memory value in rd
    parentCPU.R()[rd] = mem_val;
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_mul(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_mul: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    
    // Multiply two registers and store the result
    uint32_t op1 = parentCPU.R()[rm];
    uint32_t op2 = parentCPU.R()[rs];
    parentCPU.R()[rd] = op1 * op2;
    
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
    }
}

void ARMCPU::exec_arm_mla(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_mla: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t rn = bits<15,12>(instruction);

    // Multiply and accumulate: Rd = (Rm * Rs) + Rn
    uint32_t op1 = parentCPU.R()[rm];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t acc = parentCPU.R()[rn];
    parentCPU.R()[rd] = (op1 * op2) + acc;

    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
    }
}

void ARMCPU::exec_arm_umull(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_umull: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rdHi = bits<19,16>(instruction);
    uint8_t rdLo = bits<15,12>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint64_t result = (uint64_t)parentCPU.R()[rm] * (uint64_t)parentCPU.R()[rs];
    parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
    parentCPU.R()[rdHi] = (uint32_t)(result >> 32);
    if (rdHi != 15 && rdLo != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction); 
        if (set_flags) updateFlagsLogical(parentCPU.R()[rdHi], parentCPU.R()[rdLo]);
    }
}

void ARMCPU::exec_arm_umlal(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_umlal: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rdHi = bits<19,16>(instruction);
    uint8_t rdLo = bits<15,12>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);

    // Get the accumulator value from RdHi/RdLo
    uint64_t acc = ((uint64_t)parentCPU.R()[rdHi] << 32) | (uint64_t)parentCPU.R()[rdLo];
    uint64_t result = (uint64_t)parentCPU.R()[rm] * (uint64_t)parentCPU.R()[rs] + acc;
    parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
    parentCPU.R()[rdHi] = (uint32_t)(result >> 32);

    if (rdHi != 15 && rdLo != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction); 
        if (set_flags) updateFlagsLogical(parentCPU.R()[rdHi], parentCPU.R()[rdLo]);
    }
}

void ARMCPU::exec_arm_smull(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_smull: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rdHi = bits<19,16>(instruction);
    uint8_t rdLo = bits<15,12>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);

    int64_t result = (int64_t)(int32_t)parentCPU.R()[rm] * (int64_t)(int32_t)parentCPU.R()[rs];
    parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
    parentCPU.R()[rdHi] = (uint32_t)((result >> 32) & 0xFFFFFFFF);

    if (rdHi != 15 && rdLo != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction); 
        if (set_flags) updateFlagsLogical(parentCPU.R()[rdHi], parentCPU.R()[rdLo]);
    }
}

void ARMCPU::exec_arm_smlal(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_smlal: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rdHi = bits<19,16>(instruction);
    uint8_t rdLo = bits<15,12>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);

    // Get the accumulator value from RdHi/RdLo
    int64_t acc = ((int64_t)(int32_t)parentCPU.R()[rdHi] << 32) | (uint32_t)parentCPU.R()[rdLo];
    int64_t result = (int64_t)(int32_t)parentCPU.R()[rm] * (int64_t)(int32_t)parentCPU.R()[rs] + acc;
    parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
    parentCPU.R()[rdHi] = (uint32_t)((result >> 32) & 0xFFFFFFFF);

    if (rdHi != 15 && rdLo != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction); 
        if (set_flags) updateFlagsLogical(parentCPU.R()[rdHi], parentCPU.R()[rdLo]);
    }
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

void ARMCPU::exec_arm_undefined(uint32_t instruction) {
    DEBUG_ERROR(std::string("exec_arm_undefined: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // Trigger ARM undefined instruction exception
    handleException(0x04, 0x1B, true, false); // Vector 0x04, mode 0x1B (Undefined), disable IRQ
}

void ARMCPU::exec_arm_mov_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_mov_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    
    // Apply rotation to immediate value
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    parentCPU.R()[rd] = value;

    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(value, 0); // No carry for MOV
    }
}

void ARMCPU::exec_arm_mov_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_mov_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint32_t value = parentCPU.R()[rm];
    uint32_t shift_val;
    if (reg_shift) {
        uint8_t rs = bits<11,8>(instruction);
        shift_val = parentCPU.R()[rs] & 0xFF;
    } else {
        shift_val = bits<11,7>(instruction);
    }
    value = arm_shift(value, shift_type, shift_val);
    parentCPU.R()[rd] = value;

    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(value, 0);
    }
}

void ARMCPU::exec_arm_software_interrupt(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_software_interrupt: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint32_t swi_imm = bits<23,0>(instruction);
    // Handle software interrupt (SWI) here. Triggers Supervisor exception.
    DEBUG_ERROR(std::string("SWI executed: immediate=0x") + DEBUG_TO_HEX_STRING(swi_imm, 8) + ", pc=0x" + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8));
    handleException(0x08, 0x13, true, false); // Vector 0x08, mode 0x13 (SVC), disable IRQ
}

void ARMCPU::exec_arm_ldc_imm(uint32_t instruction) {
    DEBUG_ERROR(std::string("exec_arm_ldc_imm: Coprocessor LDC (imm) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // TODO: Implement coprocessor LDC (imm) logic if needed
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldc_reg(uint32_t instruction) {
    DEBUG_ERROR(std::string("exec_arm_ldc_reg: Coprocessor LDC (reg) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // TODO: Implement coprocessor LDC (reg) logic if needed
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_stc_imm(uint32_t instruction) {
    DEBUG_ERROR(std::string("exec_arm_stc_imm: Coprocessor STC (imm) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // TODO: Implement coprocessor STC (imm) logic if needed
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_stc_reg(uint32_t instruction) {
    DEBUG_ERROR(std::string("exec_arm_stc_reg: Coprocessor STC (reg) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // TODO: Implement coprocessor STC (reg) logic if needed
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_mrs(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_mrs: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // Bits 15-12: Rd
    uint32_t rd = bits<15,12>(instruction);
    // Bit 22: PSR source (0 = CPSR, 1 = SPSR)
    uint32_t psr_source = (instruction >> 22) & 1;
    uint32_t value = 0;
    if (psr_source == 0) {
        value = parentCPU.CPSR();
    } else {
        // SPSR not implemented, return 0 or log warning
        DEBUG_LOG("MRS: SPSR read not implemented, returning 0");
        value = 0;
    }
    if (rd != 15) {
        parentCPU.R()[rd] = value;
        parentCPU.R()[15] += 4; // Increment PC for next instruction
    } 
    DEBUG_INFO("MRS: Rd=r" + std::to_string(rd) + " <= " + debug_to_hex_string(value, 8));
}

void ARMCPU::exec_arm_msr(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_msr: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // Bit 22: PSR destination (0 = CPSR, 1 = SPSR)
    uint32_t psr_dest = (instruction >> 22) & 1;
    // Bits 19-16: source register (if operand is register)
    // Bit 25: 0 = reg, 1 = imm
    uint32_t is_imm = (instruction >> 25) & 1;
    uint32_t value = 0;
    if (is_imm) {
        // Immediate operand: bits 7-0 and rotate
        uint32_t imm = instruction & 0xFF;
        uint32_t rotate = ((instruction >> 8) & 0xF) * 2;
        value = (imm >> rotate) | (imm << (32 - rotate)); // ARM uses right rotation
        value = (value & 0xFFFFFFFF); // Ensure 32-bit
    } else {
        // Register operand: bits 3-0
        uint32_t rm = instruction & 0xF;
        value = parentCPU.R()[rm];
    }
    // Only implement CPSR write (SPSR not implemented)
    if (psr_dest == 0) {
        // Mask: bits 19-16 (field mask)
        uint32_t mask = (instruction >> 16) & 0xF;
        // Control field (bit 0)
        if (mask & 1) {
            parentCPU.CPSR() = (parentCPU.CPSR() & ~0xFF) | (value & 0xFF);
            DEBUG_INFO("MSR: CPSR control field set to " + debug_to_hex_string(value & 0xFF, 2));
        }
        // Flag field (bit 1)
        if (mask & 2) {
            // Set all flag bits (N,Z,C,V) from value
            uint32_t flags = value & 0xF0000000;
            parentCPU.CPSR() = (parentCPU.CPSR() & ~0xF0000000) | flags;
            DEBUG_INFO("MSR: CPSR flag field set to " + debug_to_hex_string(flags, 8));
        }
        // Status field (bit 2) and extension field (bit 3) can be added if needed
        if (!(mask & 1) && !(mask & 2)) {
            DEBUG_LOG("MSR: Only control and flag fields supported, mask=" + std::to_string(mask));
        }
    } else {
        DEBUG_LOG("MSR: SPSR write not implemented");
    }
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}