#include "arm_cpu.h"
#include "debug.h"
#include <cstdio>
#include <bit> // For std::popcount

// void ARMCPU::exec_arm_eor_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_eor_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     parentCPU.R()[rd] = parentCPU.R()[rn] ^ value;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
//     }
// }

// void ARMCPU::exec_arm_eor_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_eor_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rs = bits<11,8>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
//     value = arm_shift[shift_type](value, shift_val);
//     parentCPU.R()[rd] = parentCPU.R()[rn] ^ value;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
//     }
// }

// void ARMCPU::exec_arm_and_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_and_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     parentCPU.R()[rd] = parentCPU.R()[rn] & value;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
//     }
// }

// void ARMCPU::exec_arm_and_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_and_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rs = bits<11,8>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
//     value = arm_shift[shift_type](value, shift_val);
//     parentCPU.R()[rd] = parentCPU.R()[rn] & value;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
//     }
// }

// void ARMCPU::exec_arm_sub_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_sub_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     uint32_t result = parentCPU.R()[rn] - value;
//     parentCPU.R()[rd] = result;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsSub(parentCPU.R()[rn], value, result);
//     }
// }

// void ARMCPU::exec_arm_rsb_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_rsb_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     uint32_t result = value - parentCPU.R()[rn];
//     parentCPU.R()[rd] = result;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsSub(value, parentCPU.R()[rn], result);
//     }
// }

// void ARMCPU::exec_arm_sub_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_sub_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rs = bits<11,8>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
//     value = arm_shift[shift_type](value, shift_val);
//     uint32_t result = parentCPU.R()[rn] - value;
//     parentCPU.R()[rd] = result;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsSub(parentCPU.R()[rn], value, result);
//     }
// }

// void ARMCPU::exec_arm_rsb_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_rsb_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rs = bits<11,8>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
//     value = arm_shift[shift_type](value, shift_val);
//     uint32_t result = value - parentCPU.R()[rn];
//     parentCPU.R()[rd] = result;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsSub(value, parentCPU.R()[rn], result);
//     }
// }

// void ARMCPU::exec_arm_add_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_add_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     uint32_t result = parentCPU.R()[rn] + value;
//     parentCPU.R()[rd] = result;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsAdd(parentCPU.R()[rn], value, result);
//     }
// }

// // ORR (logical OR)
// void ARMCPU::exec_arm_orr_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_orr_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     parentCPU.R()[rd] = parentCPU.R()[rn] | value;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
//     }
// }

// void ARMCPU::exec_arm_orr_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_orr_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rs = bits<11,8>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
//     value = arm_shift[shift_type](value, shift_val);
//     parentCPU.R()[rd] = parentCPU.R()[rn] | value;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
//     }
// }

// void ARMCPU::exec_arm_bic_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_bic_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     parentCPU.R()[rd] = parentCPU.R()[rn] & ~value;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
//     }
// }

// void ARMCPU::exec_arm_bic_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_bic_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rs = bits<11,8>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
//     value = arm_shift[shift_type](value, shift_val);
//     parentCPU.R()[rd] = parentCPU.R()[rn] & ~value;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
//     }
// }

// void ARMCPU::exec_arm_mvn_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_mvn_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     parentCPU.R()[rd] = ~value;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
//     }
// }

// void ARMCPU::exec_arm_mvn_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_mvn_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rs = bits<11,8>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
//     value = arm_shift[shift_type](value, shift_val);
//     parentCPU.R()[rd] = ~value;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
//     }
// }

// void ARMCPU::exec_arm_add_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_add_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rs = bits<11,8>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
//     value = arm_shift[shift_type](value, shift_val);
//     uint32_t result = parentCPU.R()[rn] + value;
//     parentCPU.R()[rd] = result;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsAdd(parentCPU.R()[rn], value, result);
//     }
// }

// void ARMCPU::exec_arm_adc_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_adc_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
//     uint32_t result = parentCPU.R()[rn] + value + carry;
//     parentCPU.R()[rd] = result;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsAdd(parentCPU.R()[rn], value + carry, result);
//     }
// }

// void ARMCPU::exec_arm_adc_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_adc_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rs = bits<11,8>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
//     value = arm_shift[shift_type](value, shift_val);
//     uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
//     uint32_t result = parentCPU.R()[rn] + value + carry;
//     parentCPU.R()[rd] = result;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsAdd(parentCPU.R()[rn], value + carry, result);
//     }
// }

// void ARMCPU::exec_arm_sbc_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_sbc_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
//     uint32_t result = parentCPU.R()[rn] - value - (1 - carry);
//     parentCPU.R()[rd] = result;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsSub(parentCPU.R()[rn], value + (1 - carry), result);
//     }
// }

// void ARMCPU::exec_arm_sbc_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_sbc_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rs = bits<11,8>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
//     value = arm_shift[shift_type](value, shift_val);
//     uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
//     uint32_t result = parentCPU.R()[rn] - value - (1 - carry);
//     parentCPU.R()[rd] = result;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsSub(parentCPU.R()[rn], value + (1 - carry), result);
//     }
// }

// void ARMCPU::exec_arm_rsc_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_rsc_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
//     uint32_t result = value - parentCPU.R()[rn] - (1 - carry);
//     parentCPU.R()[rd] = result;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsSub(value, parentCPU.R()[rn] + (1 - carry), result);
//     }
// }

// void ARMCPU::exec_arm_rsc_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_rsc_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rs = bits<11,8>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
//     value = arm_shift[shift_type](value, shift_val);
//     uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
//     uint32_t result = value - parentCPU.R()[rn] - (1 - carry);
//     parentCPU.R()[rd] = result;
//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsSub(value, parentCPU.R()[rn] + (1 - carry), result);
//     }
// }

// void ARMCPU::exec_arm_tst_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_tst_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     uint32_t result = parentCPU.R()[rn] & value;
//     // Update flags, especially Z
//     updateFlagsLogical(result, 0);
//     parentCPU.R()[15] += 4; // Increment PC for next instruction
// }

// void ARMCPU::exec_arm_tst_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_tst_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rs = bits<11,8>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
//     value = arm_shift[shift_type](value, shift_val);
//     uint32_t result = parentCPU.R()[rn] & value;
//     // Update flags, especially Z
//     updateFlagsLogical(result, 0);
//     parentCPU.R()[15] += 4; // Increment PC for next instruction
// }

// void ARMCPU::exec_arm_teq_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_teq_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     uint32_t result = parentCPU.R()[rn] ^ value;
//     // Update flags, especially Z
//     updateFlagsLogical(result, 0);
//     parentCPU.R()[15] += 4; // Increment PC for next instruction
// }

// // CMP (compare, subtract, sets flags, does not store result)
// void ARMCPU::exec_arm_cmp_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_cmp_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     uint32_t result = parentCPU.R()[rn] - value;
//     updateFlagsSub(parentCPU.R()[rn], value, result);
//     parentCPU.R()[15] += 4; // Increment PC for next instruction
// }

// void ARMCPU::exec_arm_cmp_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_cmp_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rs = bits<11,8>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
//     value = arm_shift[shift_type](value, shift_val);
//     uint32_t result = parentCPU.R()[rn] - value;
//     updateFlagsSub(parentCPU.R()[rn], value, result);
//     parentCPU.R()[15] += 4; // Increment PC for next instruction
// }

// // CMN (compare negative, adds, sets flags, does not store result)
// void ARMCPU::exec_arm_cmn_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_cmn_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     uint32_t result = parentCPU.R()[rn] + value;
//     updateFlagsAdd(parentCPU.R()[rn], value, result);
//     parentCPU.R()[15] += 4; // Increment PC for next instruction
// }

// void ARMCPU::exec_arm_cmn_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_cmn_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rs = bits<11,8>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
//     value = arm_shift[shift_type](value, shift_val);
//     uint32_t result = parentCPU.R()[rn] + value;
//     updateFlagsAdd(parentCPU.R()[rn], value, result);
//     parentCPU.R()[15] += 4; // Increment PC for next instruction
// }

// void ARMCPU::exec_arm_teq_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_teq_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rn = bits<19,16>(instruction);
//     uint8_t rs = bits<11,8>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
//     value = arm_shift[shift_type](value, shift_val);
//     uint32_t result = parentCPU.R()[rn] ^ value;
//     // Update flags, especially Z
//     updateFlagsLogical(result, 0);
//     parentCPU.R()[15] += 4; // Increment PC for next instruction
// }

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

    int reg_count = std::popcount(reg_list);
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
   
    int reg_count = std::popcount(reg_list);
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
      
    if (bits<6,5>(instruction) != 0) [[unlikely]] {
        exec_arm_further_decode(instruction);
        return;
    }

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
      
    if (bits<6,5>(instruction) != 0) [[unlikely]] {
        exec_arm_further_decode(instruction);
        return;
    }

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

void ARMCPU::exec_arm_undefined(uint32_t instruction) {
    DEBUG_ERROR(std::string("exec_arm_undefined: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // Trigger ARM undefined instruction exception
    handleException(0x04, 0x1B, true, false); // Vector 0x04, mode 0x1B (Undefined), disable IRQ
}

// void ARMCPU::exec_arm_mov_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_mov_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rotate = bits<11,8>(instruction) * 2;
//     uint32_t imm = bits<7,0>(instruction);
    
//     // Apply rotation to immediate value
//     uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
//     parentCPU.R()[rd] = value;

//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsLogical(value, 0); // No carry for MOV
//     }
// }

// void ARMCPU::exec_arm_mov_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_mov_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint8_t rd = bits<15,12>(instruction);
//     uint8_t rm = bits<3,0>(instruction);
//     uint8_t shift_type = bits<6,5>(instruction);
//     uint8_t reg_shift = bits<4,4>(instruction);
//     uint32_t value = parentCPU.R()[rm];
//     uint32_t shift_val;
//     if (reg_shift) {
//         uint8_t rs = bits<11,8>(instruction);
//         shift_val = parentCPU.R()[rs] & 0xFF;
//     } else {
//         shift_val = bits<11,7>(instruction);
//     }
//     value = arm_shift[shift_type](value, shift_val);
//     parentCPU.R()[rd] = value;

//     if (rd != 15) {
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//         bool set_flags = bits<20,20>(instruction);
//         if (set_flags) updateFlagsLogical(value, 0);
//     }
// }

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

// void ARMCPU::exec_arm_mrs(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_mrs: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     // Bits 15-12: Rd
//     uint32_t rd = bits<15,12>(instruction);
//     // Bit 22: PSR source (0 = CPSR, 1 = SPSR)
//     uint32_t psr_source = (instruction >> 22) & 1;
//     uint32_t value = 0;
//     if (psr_source == 0) {
//         value = parentCPU.CPSR();
//     } else {
//         // SPSR not implemented, return 0 or log warning
//         DEBUG_LOG("MRS: SPSR read not implemented, returning 0");
//         value = 0;
//     }
//     if (rd != 15) {
//         parentCPU.R()[rd] = value;
//         parentCPU.R()[15] += 4; // Increment PC for next instruction
//     } 
//     DEBUG_INFO("MRS: Rd=r" + std::to_string(rd) + " <= " + debug_to_hex_string(value, 8));
// }

// void ARMCPU::exec_arm_msr_imm(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_msr_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     uint32_t psr_dest = (instruction >> 22) & 1;
//     uint32_t value = 0;

//     // Immediate operand: bits 7-0 and rotate
//     uint32_t imm = instruction & 0xFF;
//     uint32_t rotate = ((instruction >> 8) & 0xF) * 2;
//     value = (imm >> rotate) | (imm << (32 - rotate)); // ARM uses right rotation
//     value = (value & 0xFFFFFFFF); // Ensure 32-bit

//     // Only implement CPSR write (SPSR not implemented)
//     if (psr_dest == 0) {
//         // Mask: bits 19-16 (field mask)
//         uint32_t mask = (instruction >> 16) & 0xF;
//         // Control field (bit 0)
//         if (mask & 1) {
//             parentCPU.CPSR() = (parentCPU.CPSR() & ~0xFF) | (value & 0xFF);
//             DEBUG_INFO("MSR IMM: CPSR control field set to " + debug_to_hex_string(value & 0xFF, 2));
//         }
//         // Flag field (bit 1)
//         if (mask & 2) {
//             // Set all flag bits (N,Z,C,V) from value
//             uint32_t flags = value & 0xF0000000;
//             parentCPU.CPSR() = (parentCPU.CPSR() & ~0xF0000000) | flags;
//             DEBUG_INFO("MSR IMM: CPSR flag field set to " + debug_to_hex_string(flags, 8));
//         }
//         // Status field (bit 2) and extension field (bit 3) can be added if needed
//         if (!(mask & 1) && !(mask & 2)) {
//             DEBUG_LOG("MSR: Only control and flag fields supported, mask=" + std::to_string(mask));
//         }
//     } else {
//         DEBUG_LOG("MSR IMM: SPSR write not implemented");
//     }
//     parentCPU.R()[15] += 4; // Increment PC for next instruction
// }

// void ARMCPU::exec_arm_msr_reg(uint32_t instruction) {
//     DEBUG_LOG(std::string("exec_arm_msr_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
//     // Bit 22: PSR destination (0 = CPSR, 1 = SPSR)
//     uint32_t psr_dest = (instruction >> 22) & 1;
//     uint32_t value = 0;
    
//     // Register operand: bits 3-0
//     uint32_t rm = instruction & 0xF;
//     value = parentCPU.R()[rm];
    
//     // Only implement CPSR write (SPSR not implemented)
//     if (psr_dest == 0) {
//         // Mask: bits 19-16 (field mask)
//         uint32_t mask = (instruction >> 16) & 0xF;
//         // Control field (bit 0)
//         if (mask & 1) {
//             parentCPU.CPSR() = (parentCPU.CPSR() & ~0xFF) | (value & 0xFF);
//             DEBUG_INFO("MSR REG: CPSR control field set to " + debug_to_hex_string(value & 0xFF, 2));
//         }
//         // Flag field (bit 1)
//         if (mask & 2) {
//             // Set all flag bits (N,Z,C,V) from value
//             uint32_t flags = value & 0xF0000000;
//             parentCPU.CPSR() = (parentCPU.CPSR() & ~0xF0000000) | flags;
//             DEBUG_INFO("MSR REG: CPSR flag field set to " + debug_to_hex_string(flags, 8));
//         }
//         // Status field (bit 2) and extension field (bit 3) can be added if needed
//         if (!(mask & 1) && !(mask & 2)) {
//             DEBUG_LOG("MSR REG: Only control and flag fields supported, mask=" + std::to_string(mask));
//         }
//     } else {
//         DEBUG_LOG("MSR REG: SPSR write not implemented");
//     }
//     parentCPU.R()[15] += 4; // Increment PC for next instruction
// }