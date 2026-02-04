#include "arm_cpu.h"
#include "debug.h"
#include <cstdio>

void ARMCPU::exec_arm_eor_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_eor_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t op1 = readOperand(rn, 0);
    parentCPU.R()[rd] = op1 ^ value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t carry_out = (rotate == 0) ? ((parentCPU.CPSR() >> 29) & 1) : ((value >> 31) & 1);
            updateFlagsLogical(parentCPU.R()[rd], carry_out);
        }
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
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
    // When PC is used as operand (Rm or Rn), apply +8 pipeline offset
    uint32_t op1 = readOperand(rn, reg_shift);
    uint32_t op2 = readOperand(rm, reg_shift);
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);

    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted = apply_shift(op2, shift_val, carry, shift_type, reg_shift);
    uint32_t result = op1 ^ shifted.value;
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], shifted.carry_out);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
    }
}

void ARMCPU::exec_arm_and_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_and_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    // When PC is used as operand (Rn), apply +8 pipeline offset
    uint32_t op1 = readOperand(rn, 0);
    parentCPU.R()[rd] = op1 & value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t carry_out = (rotate == 0) ? ((parentCPU.CPSR() >> 29) & 1) : ((value >> 31) & 1);
            updateFlagsLogical(parentCPU.R()[rd], carry_out);
        }
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
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
    // When PC is used as operand (Rm or Rn), apply +8 pipeline offset
    uint32_t value = readOperand(rm, reg_shift);
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted = apply_shift(value, shift_val, carry, shift_type, reg_shift);
    uint32_t op1 = readOperand(rn, reg_shift);
    parentCPU.R()[rd] = op1 & shifted.value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], shifted.carry_out);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
    }
}

void ARMCPU::exec_arm_sub_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_sub_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    // When PC is used as operand (Rn), apply +8 pipeline offset
    uint32_t op1 = readOperand(rn, 0);
    uint32_t result = op1 - value;
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(op1, value, result);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            
            // PHASE 4: Log IRQ exit via SUBS PC, LR, #imm
            static int subs_pc_count = 0;
            if (subs_pc_count < 20) {
                fprintf(stderr, "[IRQ EXIT via SUBS] #%d PC=0x%08X->0x%08X CPSR=0x%08X->0x%08X SP=0x%08X\n",
                        subs_pc_count, parentCPU.R()[15], result, parentCPU.CPSR(), spsr, parentCPU.R()[13]);
                fprintf(stderr, "  R0-R3: %08X %08X %08X %08X  R4-R7: %08X %08X %08X %08X\n",
                        parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3],
                        parentCPU.R()[4], parentCPU.R()[5], parentCPU.R()[6], parentCPU.R()[7]);
                subs_pc_count++;
            }
            
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
    }
}

void ARMCPU::exec_arm_rsb_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_rsb_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    // When PC is used as operand (Rn), apply +8 pipeline offset
    uint32_t op1 = readOperand(rn, 0);
    uint32_t result = value - op1;
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(value, op1, result);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
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
    // When PC is used as operand (Rm or Rn), apply +8 pipeline offset
    uint32_t value = readOperand(rm, reg_shift);
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted = apply_shift(value, shift_val, carry, shift_type, reg_shift);
    uint32_t op1 = readOperand(rn, reg_shift);
    uint32_t result = op1 - shifted.value;
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(op1, shifted.value, result);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
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
    // When PC is used as operand (Rm or Rn), apply +8 pipeline offset
    uint32_t value = readOperand(rm, reg_shift);
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted = apply_shift(value, shift_val, carry, shift_type, reg_shift);
    uint32_t op1 = readOperand(rn, reg_shift);
    uint32_t result = shifted.value - op1;
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(shifted.value, op1, result);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
    }
}

void ARMCPU::exec_arm_add_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_add_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t op1 = readOperand(rn, 0);
    uint32_t result = op1 + value;
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsAdd(op1, value, result);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
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
    // When PC is used as operand (Rn), apply +8 pipeline offset
    uint32_t op1 = readOperand(rn, 0);
    parentCPU.R()[rd] = op1 | value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t carry_out = (rotate == 0) ? ((parentCPU.CPSR() >> 29) & 1) : ((value >> 31) & 1);
            updateFlagsLogical(parentCPU.R()[rd], carry_out);
        }
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
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
    // When PC is used as operand (Rm or Rn), apply +8 pipeline offset
    uint32_t value = readOperand(rm, reg_shift);
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted = apply_shift(value, shift_val, carry, shift_type, reg_shift);
    uint32_t op1 = readOperand(rn, reg_shift);
    parentCPU.R()[rd] = op1 | shifted.value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], shifted.carry_out);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
    }
}

void ARMCPU::exec_arm_bic_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_bic_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    // When PC is used as operand (Rn), apply +8 pipeline offset
    uint32_t op1 = readOperand(rn, 0);
    parentCPU.R()[rd] = op1 & ~value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t carry_out = (rotate == 0) ? ((parentCPU.CPSR() >> 29) & 1) : ((value >> 31) & 1);
            updateFlagsLogical(parentCPU.R()[rd], carry_out);
        }
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
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
    // When PC is used as operand (Rm or Rn), apply +8 pipeline offset
    uint32_t value = readOperand(rm, reg_shift);
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted = apply_shift(value, shift_val, carry, shift_type, reg_shift);
    uint32_t op1 = readOperand(rn, reg_shift);
    parentCPU.R()[rd] = op1 & ~shifted.value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], shifted.carry_out);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
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
        if (set_flags) {
            uint32_t carry_out = (rotate == 0) ? ((parentCPU.CPSR() >> 29) & 1) : ((value >> 31) & 1);
            updateFlagsLogical(parentCPU.R()[rd], carry_out);
        }
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
    }
}

void ARMCPU::exec_arm_mvn_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_mvn_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = readOperand(rm, reg_shift);
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted = apply_shift(value, shift_val, carry, shift_type, reg_shift);
    parentCPU.R()[rd] = ~shifted.value;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], shifted.carry_out);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
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
    uint32_t value = readOperand(rm, reg_shift);
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted = apply_shift(value, shift_val, carry, shift_type, reg_shift);
    uint32_t op1 = readOperand(rn, reg_shift);
    uint32_t result = op1 + shifted.value;
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsAdd(op1, shifted.value, result);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
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
    // When PC is used as operand (Rn), apply +8 pipeline offset
    uint32_t op1 = readOperand(rn, 0);
    uint32_t result = op1 + value + carry;
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsAdd(op1, value + carry, result);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
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
    // When PC is used as operand (Rm or Rn), apply +8 pipeline offset
    uint32_t value = readOperand(rm, reg_shift);
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted = apply_shift(value, shift_val, carry, shift_type, reg_shift);
    uint32_t op1 = readOperand(rn, reg_shift);
    uint32_t result = op1 + shifted.value + carry;
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsAdd(op1, shifted.value + carry, result);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
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
    uint32_t op1 = readOperand(rn, 0);
    uint32_t result = op1 - value - (1 - carry);
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(op1, value + (1 - carry), result);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
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
    uint32_t value = readOperand(rm, reg_shift);
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted = apply_shift(value, shift_val, carry, shift_type, reg_shift);
    uint32_t op1 = readOperand(rn, reg_shift);
    uint32_t result = op1 - shifted.value - (1 - carry);
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(op1, shifted.value + (1 - carry), result);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
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
    uint32_t op1 = readOperand(rn, 0);
    uint32_t result = value - op1 - (1 - carry);
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(value, op1 + (1 - carry), result);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
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
    uint32_t value = readOperand(rm, reg_shift);
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted = apply_shift(value, shift_val, carry, shift_type, reg_shift);
    uint32_t op1 = readOperand(rn, reg_shift);
    uint32_t result = shifted.value - op1 - (1 - carry);
    parentCPU.R()[rd] = result;
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsSub(shifted.value, op1 + (1 - carry), result);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
    }
}

void ARMCPU::exec_arm_tst_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_tst_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t op1 = readOperand(rn, 0);
    uint32_t result = op1 & value;
    // Update flags, especially Z
    uint32_t carry_out = (rotate == 0) ? ((parentCPU.CPSR() >> 29) & 1) : ((value >> 31) & 1);
    updateFlagsLogical(result, carry_out);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_tst_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_tst_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = readOperand(rm, reg_shift);
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted = apply_shift(value, shift_val, carry, shift_type, reg_shift);
    uint32_t op1 = readOperand(rn, reg_shift);
    uint32_t result = op1 & shifted.value;
    // Update flags, especially Z
    updateFlagsLogical(result, shifted.carry_out);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_teq_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_teq_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t op1 = readOperand(rn, 0);
    uint32_t result = op1 ^ value;
    // Update flags, especially Z
    uint32_t carry_out = (rotate == 0) ? ((parentCPU.CPSR() >> 29) & 1) : ((value >> 31) & 1);
    updateFlagsLogical(result, carry_out);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

// CMP (compare, subtract, sets flags, does not store result)
void ARMCPU::exec_arm_cmp_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_cmp_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t op1 = readOperand(rn, 0);
    uint32_t result = op1 - value;
    updateFlagsSub(op1, value, result);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_cmp_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_cmp_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = readOperand(rm, reg_shift);
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted = apply_shift(value, shift_val, carry, shift_type, reg_shift);
    uint32_t op1 = readOperand(rn, reg_shift);
    uint32_t result = op1 - shifted.value;
    
    updateFlagsSub(op1, shifted.value, result);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

// CMN (compare negative, adds, sets flags, does not store result)
void ARMCPU::exec_arm_cmn_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_cmn_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rotate = bits<11,8>(instruction) * 2;
    uint32_t imm = bits<7,0>(instruction);
    uint32_t value = (imm >> rotate) | (imm << (32 - rotate));
    uint32_t op1 = readOperand(rn, 0);
    uint32_t result = op1 + value;
    updateFlagsAdd(op1, value, result);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_cmn_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_cmn_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = readOperand(rm, reg_shift);
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted = apply_shift(value, shift_val, carry, shift_type, reg_shift);
    uint32_t op1 = readOperand(rn, reg_shift);
    uint32_t result = op1 + shifted.value;
    updateFlagsAdd(op1, shifted.value, result);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_teq_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_teq_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint32_t value = readOperand(rm, reg_shift);
    uint32_t shift_val = reg_shift ? parentCPU.R()[rs] & 0xFF : bits<11,7>(instruction);
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted = apply_shift(value, shift_val, carry, shift_type, reg_shift);
    uint32_t op1 = readOperand(rn, reg_shift);
    uint32_t result = op1 ^ shifted.value;
    // Update flags, especially Z
    updateFlagsLogical(result, shifted.carry_out);
    parentCPU.R()[15] += 4; // Increment PC for next instruction
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
        if (set_flags) {
            // If rotate > 0, carry is set to bit 31 of result; otherwise unchanged
            uint32_t carry_out = (rotate == 0) ? ((parentCPU.CPSR() >> 29) & 1) : ((value >> 31) & 1);
            updateFlagsLogical(value, carry_out);
        }
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
    }
}

void ARMCPU::exec_arm_mov_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_mov_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t shift_type = bits<6,5>(instruction);
    uint8_t reg_shift = bits<4,4>(instruction);
    uint32_t value = readOperand(rm, reg_shift);
    uint32_t shift_val;
    if (reg_shift) {
        uint8_t rs = bits<11,8>(instruction);
        shift_val = parentCPU.R()[rs] & 0xFF;
        // Debug: Print for any MOV with PC as source and register shift
        if (rm == 15) {
            printf("[MOV REG DEBUG] PC=0x%08X, R0=0x%08X, R12=0x%08X, value=0x%08X, shift_val=%u, instr=0x%08X, rd=%u, rs=%u\n", parentCPU.R()[15], parentCPU.R()[0], parentCPU.R()[12], value, shift_val, instruction, rd, rs);
        }
    } else {
        shift_val = bits<11,7>(instruction);
    }
    uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
    ShiftResult shifted;
    // Register shift by 0: preserve value and carry (different from immediate shift)
    if (reg_shift && shift_val == 0) {
        shifted.value = value;
        shifted.carry_out = carry;
    } else {
        shifted = apply_shift(value, shift_val, carry, shift_type, reg_shift);
    }
    parentCPU.R()[rd] = shifted.value;
    // Print result after MOV for t224
    if (rm == 15 && reg_shift) {
        printf("[MOV REG DEBUG] After MOV: R0=0x%08X, PC=0x%08X, R12=0x%08X\n", parentCPU.R()[0], parentCPU.R()[15], parentCPU.R()[12]);
    }
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(shifted.value, shifted.carry_out);
    } else {
        // When rd == 15 and S bit is set, restore CPSR from SPSR
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) {
            uint32_t spsr = parentCPU.SPSR();
            uint32_t new_mode = spsr & 0x1F;
            uint32_t old_mode = parentCPU.CPSR() & 0x1F;
            // If mode changes, call setMode to bank/unbank registers
            if (new_mode != old_mode) {
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
            }
            // Now restore full CPSR from SPSR
            parentCPU.CPSR() = spsr;
        }
    }
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
        // Read SPSR using the proper accessor
        value = parentCPU.SPSR();
    }
    
    // Debug for BIOS
    static int mrs_count = 0;
    if (mrs_count < 10 && parentCPU.R()[15] < 0x4000) {
        LOG_BIOS("[MRS #%d @0x%08X] R%d = %s (0x%08X), before: R%d=0x%08X, CPSR=0x%08X\n",
               mrs_count++, parentCPU.R()[15], rd,
               psr_source ? "SPSR" : "CPSR", value, rd, parentCPU.R()[rd],
               parentCPU.CPSR());
    }
    
    if (rd != 15) {
        parentCPU.R()[rd] = value;
        parentCPU.R()[15] += 4; // Increment PC for next instruction
    } 
    DEBUG_INFO("MRS: Rd=r" + std::to_string(rd) + " <= " + debug_to_hex_string(value, 8));
}

void ARMCPU::exec_arm_msr_imm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_msr_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint32_t psr_dest = (instruction >> 22) & 1;
    uint32_t value = 0;

    // Immediate operand: bits 7-0 and rotate
    uint32_t imm = instruction & 0xFF;
    uint32_t rotate = ((instruction >> 8) & 0xF) * 2;
    if (rotate == 0) {
        value = imm;
    } else {
        value = (imm >> rotate) | (imm << (32 - rotate)); // ARM uses right rotation
    }
    value = (value & 0xFFFFFFFF); // Ensure 32-bit

    // Only implement CPSR write (SPSR not implemented)
    if (psr_dest == 0) {
        // Mask: bits 19-16 (field mask)
        uint32_t mask = (instruction >> 16) & 0xF;
        // Control field (bit 0) - includes mode bits
        if (mask & 1) {
            uint32_t old_cpsr = parentCPU.CPSR();
            uint32_t old_mode = old_cpsr & 0x1F;
            uint32_t new_mode = value & 0x1F;
            // If mode bits changed, call setMode to bank/unbank registers BEFORE changing CPSR
            // This allows setMode to read the old mode from CPSR correctly
            if (old_mode != new_mode && new_mode >= 0x10 && new_mode <= 0x1F) {
                printf("[MSR IMM] Mode switch: 0x%02X → 0x%02X\n", old_mode, new_mode);
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
                printf("[MSR IMM] After setMode: LR=0x%08X\n", parentCPU.R()[14]);
            } else if (old_mode != new_mode) {
                printf("[MSR IMM] Invalid mode 0x%02X, preserving old mode 0x%02X (instr=0x%08X)\n",
                       new_mode, old_mode, instruction);
            }
            // Now update CPSR after setMode (setMode also updates CPSR, but we need to set all control bits)
            if (new_mode < 0x10 || new_mode > 0x1F) {
                uint32_t new_control = (value & 0xE0) | old_mode;
                parentCPU.CPSR() = (parentCPU.CPSR() & ~0xFF) | new_control;
            } else {
                parentCPU.CPSR() = (parentCPU.CPSR() & ~0xFF) | (value & 0xFF);
            }
            DEBUG_INFO("MSR IMM: CPSR control field set to " + debug_to_hex_string(value & 0xFF, 2));
        }
        // Extension field (bit 1)
        if (mask & 2) {
            DEBUG_INFO("MSR IMM: CPSR extension field ignored on ARM7TDMI");
        }
        // Status field (bit 2)
        if (mask & 4) {
            DEBUG_INFO("MSR IMM: CPSR status field ignored on ARM7TDMI");
        }
        // Flag field (bit 3)
        if (mask & 8) {
            // Set all flag bits (N,Z,C,V) from value
            uint32_t flags = value & 0xF0000000;
            parentCPU.CPSR() = (parentCPU.CPSR() & ~0xF0000000) | flags;
            DEBUG_INFO("MSR IMM: CPSR flag field set to " + debug_to_hex_string(flags, 8));
        }
        if (!(mask & 1) && !(mask & 2) && !(mask & 4) && !(mask & 8)) {
            DEBUG_LOG("MSR: Only control, extension, status, and flag fields supported, mask=" + std::to_string(mask));
        }
    } else {
        // SPSR write - only valid in privileged modes (not USER or SYS)
        uint32_t current_mode = parentCPU.CPSR() & 0x1F;
        if (current_mode != 0x10 && current_mode != 0x1F) { // Not USER or SYS
            uint32_t mask = (instruction >> 16) & 0xF;
            uint32_t spsr = parentCPU.SPSR();
            // Apply mask-based field writes
            if (mask & 1) { // Control field (bits 0-7)
                spsr = (spsr & ~0xFF) | (value & 0xFF);
            }
            if (mask & 2) { // Extension field (bits 8-15)
                spsr = (spsr & ~0xFF00) | (value & 0xFF00);
            }
            if (mask & 4) { // Status field (bits 16-23)
                spsr = (spsr & ~0xFF0000) | (value & 0xFF0000);
            }
            if (mask & 8) { // Flag field (bits 24-31)
                spsr = (spsr & ~0xF0000000) | (value & 0xF0000000);
            }
            parentCPU.SPSR() = spsr;
            DEBUG_INFO("MSR IMM: SPSR set to " + debug_to_hex_string(spsr, 8));
        }
    }
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}
void ARMCPU::exec_arm_msr_reg(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_msr_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // Bit 22: PSR destination (0 = CPSR, 1 = SPSR)
    uint32_t psr_dest = (instruction >> 22) & 1;
    uint32_t value = 0;
    
    // Register operand: bits 3-0
    uint32_t rm = instruction & 0xF;
    value = parentCPU.R()[rm];
    
    // Only implement CPSR write (SPSR not implemented)
    if (psr_dest == 0) {
        // Mask: bits 19-16 (field mask)
        uint32_t mask = (instruction >> 16) & 0xF;
        // Control field (bit 0) - includes mode bits
        if (mask & 1) {
            uint32_t old_cpsr = parentCPU.CPSR();
            uint32_t old_mode = old_cpsr & 0x1F;
            uint32_t new_mode = value & 0x1F;
            // printf("[MSR REG] mask=0x%X, old_mode=0x%02X, new_mode=0x%02X\n", mask, old_mode, new_mode);
            // If mode bits changed, call setMode to bank/unbank registers BEFORE changing CPSR
            // This allows setMode to read the old mode from CPSR correctly
            if (old_mode != new_mode && new_mode >= 0x10 && new_mode <= 0x1F) {
                // printf("[MSR REG] Mode switch: 0x%02X → 0x%02X\n", old_mode, new_mode);
                parentCPU.setMode(static_cast<CPU::Mode>(new_mode));
                // printf("[MSR REG] After setMode: LR=0x%08X\n", parentCPU.R()[14]);
            } else if (old_mode != new_mode) {
                printf("[MSR REG] Invalid mode 0x%02X, preserving old mode 0x%02X (instr=0x%08X)\n",
                       new_mode, old_mode, instruction);
            }
            // Now update CPSR after setMode (setMode also updates CPSR, but we need to set all control bits)
            if (new_mode < 0x10 || new_mode > 0x1F) {
                uint32_t new_control = (value & 0xE0) | old_mode;
                parentCPU.CPSR() = (parentCPU.CPSR() & ~0xFF) | new_control;
            } else {
                parentCPU.CPSR() = (parentCPU.CPSR() & ~0xFF) | (value & 0xFF);
            }
            DEBUG_INFO("MSR REG: CPSR control field set to " + debug_to_hex_string(value & 0xFF, 2));
        }
        // Extension field (bit 1)
        if (mask & 2) {
            DEBUG_INFO("MSR REG: CPSR extension field ignored on ARM7TDMI");
        }
        // Status field (bit 2)
        if (mask & 4) {
            DEBUG_INFO("MSR REG: CPSR status field ignored on ARM7TDMI");
        }
        // Flag field (bit 3)
        if (mask & 8) {
            // Set all flag bits (N,Z,C,V) from value
            uint32_t flags = value & 0xF0000000;
            parentCPU.CPSR() = (parentCPU.CPSR() & ~0xF0000000) | flags;
            DEBUG_INFO("MSR REG: CPSR flag field set to " + debug_to_hex_string(flags, 8));
        }
        if (!(mask & 1) && !(mask & 2) && !(mask & 4) && !(mask & 8)) {
            DEBUG_LOG("MSR REG: Only control, extension, status, and flag fields supported, mask=" + std::to_string(mask));
        }
    } else {
        // SPSR write - only valid in privileged modes (not USER or SYS)
        uint32_t current_mode = parentCPU.CPSR() & 0x1F;
        if (current_mode != 0x10 && current_mode != 0x1F) { // Not USER or SYS
            uint32_t mask = (instruction >> 16) & 0xF;
            uint32_t spsr = parentCPU.SPSR();
            // Apply mask-based field writes
            if (mask & 1) { // Control field (bits 0-7)
                spsr = (spsr & ~0xFF) | (value & 0xFF);
            }
            if (mask & 2) { // Extension field (bits 8-15)
                spsr = (spsr & ~0xFF00) | (value & 0xFF00);
            }
            if (mask & 4) { // Status field (bits 16-23)
                spsr = (spsr & ~0xFF0000) | (value & 0xFF0000);
            }
            if (mask & 8) { // Flag field (bits 24-31)
                spsr = (spsr & ~0xF0000000) | (value & 0xF0000000);
            }
            parentCPU.SPSR() = spsr;
            DEBUG_INFO("MSR REG: SPSR set to " + debug_to_hex_string(spsr, 8));
        }
    }
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}