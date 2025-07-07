#include "thumb_cpu.h"
#include "debug.h"
#include <sstream>
#include <iomanip>

// Define the static constexpr members
constexpr void (ThumbCPU::*ThumbCPU::thumb_instruction_table[256])(uint16_t);
constexpr void (ThumbCPU::*ThumbCPU::thumb_alu_operations_table[16])(uint8_t, uint8_t);

ThumbCPU::ThumbCPU(CPU& cpu) : parentCPU(cpu) {
Debug::log::info("Initializing ThumbCPU with parent CPU");
    // Initialize any Thumb-specific state or resources here
    Debug::log::info("ThumbCPU initialized. Instruction table is compile-time constant.");
}

ThumbCPU::~ThumbCPU() {
    // Cleanup logic if necessary
}

void ThumbCPU::execute(uint32_t cycles) {
    Debug::log::info("Executing Thumb instructions for " + std::to_string(cycles) + " cycles");
    Debug::log::info("Parent CPU memory size: " + std::to_string(parentCPU.getMemory().getSize()) + " bytes");
    while (cycles > 0) {
        // HACK - do we need to model the cpu pipeline?
        uint16_t instruction = parentCPU.getMemory().read16(parentCPU.R()[15]); // Fetch instruction
        uint8_t opcode = instruction >> 8;

        Debug::log::info("Fetched Thumb instruction: " + Debug::toHexString(instruction, 4) + " at PC: " + Debug::toHexString(parentCPU.R()[15], 8));
        parentCPU.R()[15] += 2; // Increment PC for Thumb instructions
        Debug::log::info("Incremented PC to: " + Debug::toHexString(parentCPU.R()[15], 8));
        
        // Decode and execute the instruction
        if (thumb_instruction_table[opcode]) {
            (this->*thumb_instruction_table[opcode])(instruction);
        } else {
            Debug::log::error("Unknown Thumb instruction");
        }
        cycles -= 1; // Placeholder for cycle deduction
    }
}


// Thumb instruction handlers

// Stub handlers for undefined Thumb instruction functions
void ThumbCPU::thumb_lsl(uint16_t instruction) {
    uint8_t rd = bits2to0(instruction);
    uint8_t rs = bits5to3(instruction);
    uint8_t shift_amount = bits10to6(instruction);

    if (shift_amount > 0) {
        parentCPU.updateCFlagShiftLSL(parentCPU.R()[rs], shift_amount);
        parentCPU.R()[rd] = parentCPU.R()[rs] << shift_amount;
    } else {
        // No shift, C flag is not affected
        parentCPU.R()[rd] = parentCPU.R()[rs];
    }
    
    parentCPU.updateZFlag(parentCPU.R()[rd]);
    parentCPU.updateNFlag(parentCPU.R()[rd]);
    // No effect on overflow flag

    Debug::log::info("Executing Thumb LSL: R" + std::to_string(rd) + " = R" + std::to_string(rs) + " << " + std::to_string(shift_amount));
}

void ThumbCPU::thumb_lsr(uint16_t instruction) {
    uint8_t rd = bits2to0(instruction);
    uint8_t rs = bits5to3(instruction);
    uint8_t shift_amount = bits10to6(instruction);

    // Update C flag before shifting
    parentCPU.updateCFlagShiftLSR(parentCPU.R()[rs], shift_amount);
    if (shift_amount == 0) {
        // Special case: LSR with shift amount 0 means shift by 32
        parentCPU.R()[rd] = 0;
    } else {
        parentCPU.R()[rd] = parentCPU.R()[rs] >> shift_amount;
    }

    parentCPU.updateZFlag(parentCPU.R()[rd]);
    parentCPU.clearFlag(CPU::FLAG_N);

    Debug::log::info("Executing Thumb LSR: R" + std::to_string(rd) + " = R" + std::to_string(rs) + " >> " + std::to_string(shift_amount));
}

void ThumbCPU::thumb_asr(uint16_t instruction) {
    uint8_t rd = bits2to0(instruction);
    uint8_t rs = bits5to3(instruction);
    uint8_t shift_amount = bits10to6(instruction);

    // ASR has some unique handling as a shift of 0 
    // Update C flag before shifting
    parentCPU.updateCFlagShiftASR(parentCPU.R()[rs], shift_amount);
    if (shift_amount == 0) {
        // Special case: if shift_amount is 0, the result is all bits of the source register set to its sign bit
        parentCPU.R()[rd] = (parentCPU.R()[rs] & 0x80000000) ? 0xFFFFFFFF : 0;
    } else {
        // Have to cast to int32_t for correct sign extension
        parentCPU.R()[rd] = static_cast<int32_t>(parentCPU.R()[rs]) >> shift_amount;
    }

    parentCPU.updateZFlag(parentCPU.R()[rd]);
    parentCPU.updateNFlag(parentCPU.R()[rd]);
    // No effect on overflow flag

    Debug::log::info("Executing Thumb ASR: R" + std::to_string(rd) + " = R" + std::to_string(rs) + " >> " + std::to_string(shift_amount));
}

void ThumbCPU::thumb_add_register(uint16_t instruction) {
    uint8_t rd = bits2to0(instruction);
    uint8_t rs = bits5to3(instruction);
    uint8_t rn = bits8to6(instruction);

    // Perform the addition operation
    uint32_t op1 = parentCPU.R()[rs];
    uint32_t op2 = parentCPU.R()[rn];
    uint32_t result = op1 + op2;

    // Update the destination register
    parentCPU.R()[rd] = result;

    // Update flags
    parentCPU.updateZFlag(result); // Zero flag
    parentCPU.updateNFlag(result); // Negative flag
    parentCPU.updateCFlagAdd(op1, op2); // Carry flag
    parentCPU.updateVFlag(op1, op2, result); // Overflow flag

    Debug::log::info("Executing Thumb ADD (register): R" + std::to_string(rd) + " = R" + std::to_string(rs) + " + R" + std::to_string(rn));
}

void ThumbCPU::thumb_add_offset(uint16_t instruction) {
    uint8_t rd = bits2to0(instruction);
    uint8_t rs = bits5to3(instruction);
    uint8_t offset = bits8to6(instruction);

    // Perform the addition operation
    uint32_t op1 = parentCPU.R()[rs];
    uint32_t result = op1 + offset;

    // Update the destination register
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result); // Zero flag
    parentCPU.updateNFlag(result); // Negative flag
    parentCPU.updateCFlagAdd(op1, offset); // Carry flag
    parentCPU.updateVFlag(op1, offset, result); // Overflow flag

    Debug::log::info("Executing Thumb ADD (offset): R" + std::to_string(rd) + " = R" + std::to_string(rs) + " + " + std::to_string(offset));
}

void ThumbCPU::thumb_sub_register(uint16_t instruction) {
    uint8_t rd = bits2to0(instruction);
    uint8_t rs = bits5to3(instruction);
    uint8_t rn = bits8to6(instruction);

    // Perform the subtraction operation
    uint32_t op1 = parentCPU.R()[rs];
    uint32_t op2 = parentCPU.R()[rn];
    uint32_t result = op1 - op2;

    // Update the destination register
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result); // Zero flag
    parentCPU.updateNFlag(result); // Negative flag
    parentCPU.updateCFlagSub(op1, op2); // Carry flag
    parentCPU.updateVFlagSub(op1, op2, result); // Overflow flag

    Debug::log::info("Executing Thumb SUB (register): R" + std::to_string(rd) + " = R" + std::to_string(rs) + " - R" + std::to_string(rn));
}

void ThumbCPU::thumb_sub_offset(uint16_t instruction) {
    uint8_t rd = bits2to0(instruction);
    uint8_t rs = bits5to3(instruction);
    uint8_t offset = bits8to6(instruction);
    uint32_t op1 = parentCPU.R()[rs];

    // Perform the subtraction operation
    uint32_t result = op1 - offset;

    // Update the destination register
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result); // Zero flag
    parentCPU.updateNFlag(result); // Negative flag
    parentCPU.updateCFlagSub(op1, offset); // Carry flag
    parentCPU.updateVFlagSub(op1, offset, result); // Overflow flag
    
    Debug::log::info("Executing Thumb SUB (offset): R" + std::to_string(rd) + " = R" + std::to_string(rs) + " - " + std::to_string(offset));
}

void ThumbCPU::thumb_mov_imm(uint16_t instruction) {
    uint8_t rd = bits10to8(instruction);
    uint8_t imm = bits7to0(instruction);

    parentCPU.R()[rd] = imm;
    parentCPU.updateZFlag(parentCPU.R()[rd]); // No negative, carry-out or overflow for MOV
    parentCPU.clearFlag(CPU::FLAG_N); // N flag is always cleared for this instruction as the 8bit immediate is always non-negative

    Debug::log::info("Executing Thumb MOV (immediate): R" + std::to_string(rd) + " = " + std::to_string(imm));
}

void ThumbCPU::thumb_cmp_imm(uint16_t instruction) {
    uint8_t rd = bits10to8(instruction);
    uint8_t imm = bits7to0(instruction);

    uint32_t op1 = parentCPU.R()[rd];
    uint32_t result = parentCPU.R()[rd] - imm; // Unsigned subtraction for carry/zero
    
    parentCPU.updateZFlag(result);
    parentCPU.updateCFlagSub(op1, imm);
    parentCPU.updateVFlagSub(op1, imm, result);
    parentCPU.updateNFlag(result);

    Debug::log::info("CMP_IMM: R[" + std::to_string(rd) + "] = " + std::to_string(parentCPU.R()[rd]) + ", imm = " + std::to_string(imm));
}

void ThumbCPU::thumb_add_imm(uint16_t instruction) {
    uint8_t rd = bits10to8(instruction);
    uint8_t imm = bits7to0(instruction);

    uint32_t op1 = parentCPU.R()[rd];
    uint32_t result = op1 + imm;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateCFlagAdd(op1, imm);
    parentCPU.updateVFlag(op1, imm, result);
    parentCPU.updateNFlag(result);
    
    Debug::log::info("Executing Thumb ADD (immediate): R" + std::to_string(rd) + " = R" + std::to_string(rd) + " + " + std::to_string(imm));
}

void ThumbCPU::thumb_sub_imm(uint16_t instruction) {
    uint8_t rd = bits10to8(instruction);
    uint8_t imm = bits7to0(instruction);

    uint32_t op1 = parentCPU.R()[rd];
    uint32_t result = op1 - imm;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateCFlagSub(op1, imm);
    parentCPU.updateVFlagSub(op1, imm, result);
    parentCPU.updateNFlag(result);

    Debug::log::info("Executing Thumb SUB (immediate): R" + std::to_string(rd) + " = R" + std::to_string(rd) + " - " + std::to_string(imm));
}

void ThumbCPU::thumb_alu_operations(uint16_t instruction) {
    uint8_t sub_opcode = bits9to6(instruction);
    uint8_t rd = bits2to0(instruction);
    uint8_t rs = bits5to3(instruction);
    
    if (thumb_alu_operations_table[sub_opcode] != NULL) {
         (this->*thumb_alu_operations_table[sub_opcode])(rd, rs);
    } else {
        Debug::log::error("Undefined ALU operation: sub-opcode " + std::to_string(sub_opcode));
    }
}

// Define individual ALU operation functions
void ThumbCPU::thumb_alu_and(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 & op2;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // No update to V and C flags for AND operation

    Debug::log::info("Executing Thumb AND: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " & R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_eor(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 ^ op2;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // No update to V and C flags for EOR operation

    Debug::log::info("Executing Thumb EOR: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " ^ R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_lsl(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint8_t shift_amount = parentCPU.R()[rs] & 0xFF; // Shift amount is the bottom 8 bits of Rs

    // Update C flag before the shift
    parentCPU.updateCFlagShiftLSL(op1, shift_amount);

    uint32_t result = op1 << shift_amount;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // V flag is not affected by LSL

    Debug::log::info("Executing Thumb LSL: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " << R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_lsr(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t shift_amount = parentCPU.R()[rs] & 0xFF; // Shift amount is the bottom 8 bits of Rs;
    
    parentCPU.updateCFlagShiftLSR(op1, shift_amount);

    uint32_t result = op1 >> shift_amount;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // No update to V flag for LSR operation

    Debug::log::info("Executing Thumb LSR: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " >> R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_asr(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t shift_amount = parentCPU.R()[rs] & 0xFF; // Shift amount is the bottom 8 bits of Rs;
    
    parentCPU.updateCFlagShiftASR(op1, shift_amount);

    uint32_t result = static_cast<int32_t>(op1) >> shift_amount; // ASR is arithmetic shift right, so we cast to int32_t for sign extension
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // No update to V flag for ASR operation

    Debug::log::info("Executing Thumb ASR: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " >> R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_adc(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t carry_in = parentCPU.getFlag(CPU::FLAG_C);

    uint32_t result = op1 + op2 + carry_in;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateCFlagAddWithCarry(op1, op2);
    parentCPU.updateVFlag(op1, (op2 + carry_in), result);
    parentCPU.updateNFlag(result);

    Debug::log::info("Executing Thumb ADC: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " + R" + std::to_string(rs) + " + Carry");
}

void ThumbCPU::thumb_alu_sbc(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t borrow = 1 - parentCPU.getFlag(CPU::FLAG_C);

    uint32_t result = op1 - op2 - borrow;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateCFlagSubWithCarry(op1, op2);
    parentCPU.updateVFlagSub(op1, op2 + borrow, result);
    parentCPU.updateNFlag(result);

    Debug::log::info("Executing Thumb SBC: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " - R" + std::to_string(rs) + " - Borrow");
}

void ThumbCPU::thumb_alu_ror(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t shift_reg_val = parentCPU.R()[rs];
    uint8_t shift_amount_8 = shift_reg_val & 0xFF;

    uint32_t result;

    if (shift_amount_8 == 0) {
        // C flag is not affected, result is op1
        result = op1;
    } else {
        uint8_t shift_imm = shift_amount_8 & 0x1F;
        if (shift_imm == 0) {
            // This is a rotate by a multiple of 32 (but not 0).
            // The result is unchanged.
            result = op1;
            // The C flag becomes the MSB of op1.
            if (op1 & 0x80000000) {
                parentCPU.setFlag(CPU::FLAG_C);
            } else {
                parentCPU.clearFlag(CPU::FLAG_C);
            }
        } else {
            result = (op1 >> shift_imm) | (op1 << (32 - shift_imm));
            // The C flag is the last bit shifted out.
            if ((op1 >> (shift_imm - 1)) & 1) {
                parentCPU.setFlag(CPU::FLAG_C);
            } else {
                parentCPU.clearFlag(CPU::FLAG_C);
            }
        }
    }

    parentCPU.R()[rd] = result;
    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // V flag is not affected.

    Debug::log::info("Executing Thumb ROR: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " ROR R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_tst(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 & op2;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // No update to V and C flags for TST operation

    Debug::log::info("Executing Thumb TST: R" + std::to_string(rd) + " & R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_neg(uint8_t rd, uint8_t rs) {
    uint32_t op1 = 0;
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 - op2;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    parentCPU.updateCFlagSub(op1, op2);
    parentCPU.updateVFlagSub(op1, op2, result);

    Debug::log::info("Executing Thumb NEG: R" + std::to_string(rd) + " = -R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_cmp(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 - op2;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    parentCPU.updateCFlagSub(op1, op2);
    parentCPU.updateVFlagSub(op1, op2, result);

    Debug::log::info("Executing Thumb CMP: R" + std::to_string(rd) + " - R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_cmn(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 + op2;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    parentCPU.updateCFlagAdd(op1, op2);
    parentCPU.updateVFlag(op1, op2, result);

    Debug::log::info("Executing Thumb CMN: R" + std::to_string(rd) + " + R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_orr(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 | op2;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // No update to V and C flags for ORR operation

    Debug::log::info("Executing Thumb ORR: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " | R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_mul(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 * op2;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // No update to V and C flags for MUL operation

    Debug::log::info("Executing Thumb MUL: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " * R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_bic(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 & ~op2;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // No update to V and C flags for BIC operation

    Debug::log::info("Executing Thumb BIC: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " & ~R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_mvn(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rs];
    uint32_t result = ~op1;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // No update to V and C flags for MVN operation

    Debug::log::info("Executing Thumb MVN: R" + std::to_string(rd) + " = ~R" + std::to_string(rs));
}

void ThumbCPU::thumb_format5(uint16_t instruction) {
    // Format 5: Hi register operations/branch exchange
    // Encoding: 010001[Op][H1][H2][Rs/Hs][Rd/Hd]
    
    uint8_t op = (instruction >> 8) & 0x3;  // bits 9-8
    uint8_t h1 = (instruction >> 7) & 0x1;  // bit 7
    uint8_t h2 = (instruction >> 6) & 0x1;  // bit 6
    uint8_t rs_field = (instruction >> 3) & 0x7;  // bits 5-3
    uint8_t rd_field = instruction & 0x7;  // bits 2-0
    
    // Calculate actual register numbers
    uint8_t rs = rs_field + (h2 ? 8 : 0);
    uint8_t rd = rd_field + (h1 ? 8 : 0);
    
    switch (op) {
        case 0b00: // ADD
            {
                uint32_t op1, op2;
                
                // Handle PC reads with pipeline offset
                if (rd == 15) {
                    op1 = parentCPU.R()[15] + 2; // PC read gives PC+4 (current instruction + 4)
                } else {
                    op1 = parentCPU.R()[rd];
                }
                
                if (rs == 15) {
                    op2 = parentCPU.R()[15] + 2; // PC read gives PC+4 (current instruction + 4)
                } else {
                    op2 = parentCPU.R()[rs];
                }
                
                uint32_t result = op1 + op2;
                
                parentCPU.R()[rd] = result;
                
                // Special case: if destination is PC, handle branch
                if (rd == 15) {
                    parentCPU.R()[15] = result & ~1; // Clear bit 0 for ARM alignment
                    // Note: PC writes in Thumb mode stay in Thumb mode
                }
                
                // ADD with high registers does not affect flags
            }
            break;
            
        case 0b01: // CMP
            {
                uint32_t op1, op2;
                
                // Handle PC reads with pipeline offset
                if (rd == 15) {
                    op1 = parentCPU.R()[15] + 2; // PC read gives PC+4 (current instruction + 4)
                } else {
                    op1 = parentCPU.R()[rd];
                }
                
                if (rs == 15) {
                    op2 = parentCPU.R()[15] + 2; // PC read gives PC+4 (current instruction + 4)
                } else {
                    op2 = parentCPU.R()[rs];
                }
                
                uint32_t result = op1 - op2;
                
                // CMP always updates flags
                parentCPU.updateZFlag(result);
                parentCPU.updateNFlag(result);
                parentCPU.updateCFlagSub(op1, op2);
                parentCPU.updateVFlagSub(op1, op2, result);
            }
            break;
            
        case 0b10: // MOV
            {
                uint32_t result;
                
                // Handle PC reads with pipeline offset
                if (rs == 15) {
                    result = parentCPU.R()[15] + 2; // PC read gives PC+4 (current instruction + 4)
                } else {
                    result = parentCPU.R()[rs];
                }
                
                parentCPU.R()[rd] = result;
                
                // Special case: if destination is PC, handle branch
                if (rd == 15) {
                    parentCPU.R()[15] = result & ~1; // Clear bit 0 for ARM alignment
                    // Note: PC writes in Thumb mode stay in Thumb mode
                }
                
                // MOV with high registers does not affect flags
            }
            break;
            
        case 0b11: // BX
            {
                uint32_t target;
                
                // Handle PC reads with pipeline offset
                if (rs == 15) {
                    target = parentCPU.R()[15] + 2; // PC read gives PC+4 (current instruction + 4)
                } else {
                    target = parentCPU.R()[rs];
                }
                
                // Set PC to target address with bit 0 cleared
                parentCPU.R()[15] = target & ~1;
                
                // Update processor mode based on bit 0 of target
                if (target & 1) {
                    parentCPU.CPSR() |= CPU::FLAG_T; // Set Thumb mode
                } else {
                    parentCPU.CPSR() &= ~CPU::FLAG_T; // Clear Thumb mode (ARM)
                }
            }
            break;
    }
}

void ThumbCPU::thumb_ldr(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[15] + (offset << 2); // PC-relative addressing

    // Perform the load operation
    parentCPU.R()[rd] = parentCPU.getMemory().read32(address);

    Debug::log::info("Executing Thumb LDR: R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::thumb_str_word(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Source register (bits 0-2)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = (instruction >> 6) & 0x07; // Offset register (bits 6-8)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the store operation using memory_write_32
    parentCPU.getMemory().write32(address, parentCPU.R()[rd]);

    Debug::log::info("Executing Thumb STR (word): [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::thumb_ldr_word(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = (instruction >> 6) & 0x07; // Offset register (bits 6-8)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Debug register values
    Debug::log::info("LDR_WORD: R" + std::to_string(rn) + "=0x" + Debug::toHexString(parentCPU.R()[rn], 8) + 
                     ", R" + std::to_string(rm) + "=0x" + Debug::toHexString(parentCPU.R()[rm], 8) + 
                     ", address=0x" + Debug::toHexString(address, 8));

    // Perform the load operation using memory_read_32
    parentCPU.R()[rd] = parentCPU.getMemory().read32(address);

    Debug::log::info("Executing Thumb LDR (word): R" + std::to_string(rd) + " = [0x" + Debug::toHexString(address, 8) + "]");
}

void ThumbCPU::thumb_ldr_byte(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = (instruction >> 6) & 0x07; // Offset register (bits 6-8)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the load operation using memory_read_8
    parentCPU.R()[rd] = parentCPU.getMemory().read8(address);

    Debug::log::info("Executing Thumb LDR (byte): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::thumb_str_byte(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Source register (bits 0-2)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = (instruction >> 6) & 0x07; // Offset register (bits 6-8)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the store operation using memory_write_8
    parentCPU.getMemory().write8(address, parentCPU.R()[rd] & 0xFF); // Store only the least significant byte

    Debug::log::info("Executing Thumb STR (byte): [0x" + Debug::toHexString(address, 8) + "] = R" + std::to_string(rd) + 
        " (R" + std::to_string(rn) + "=0x" + Debug::toHexString(parentCPU.R()[rn], 8) + 
        " + R" + std::to_string(rm) + "=0x" + Debug::toHexString(parentCPU.R()[rm], 8) + 
        ", data=0x" + Debug::toHexString(parentCPU.R()[rd] & 0xFF, 2) + ")");
}

void ThumbCPU::thumb_strh(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Source register (bits 0-2)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = (instruction >> 6) & 0x07; // Offset register (bits 6-8)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the store operation using memory_write_16
    parentCPU.getMemory().write16(address, parentCPU.R()[rd] & 0xFFFF); // Store only the least significant halfword

    Debug::log::info("Executing Thumb STRH: [0x" + Debug::toHexString(address, 8) + "] = R" + std::to_string(rd));
}

void ThumbCPU::thumb_ldsb(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = (instruction >> 6) & 0x07; // Offset register (bits 6-8)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the load operation using memory_read_8 and sign-extend
    int8_t value = (int8_t)parentCPU.getMemory().read8(address);
    parentCPU.R()[rd] = (int32_t)value;

    Debug::log::info("Executing Thumb LDSB: R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::thumb_ldrh(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = (instruction >> 6) & 0x07; // Offset register (bits 6-8)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the load operation using memory_read_16
    parentCPU.R()[rd] = parentCPU.getMemory().read16(address);

    Debug::log::info("Executing Thumb LDRH: R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::thumb_ldsh(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = (instruction >> 6) & 0x07; // Offset register (bits 6-8)

    Debug::log::info("LDSH instruction decode: 0x" + std::to_string(instruction) + " -> rd=" + std::to_string(rd) + ", rn=" + std::to_string(rn) + ", rm=" + std::to_string(rm));

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the load operation using memory_read_16 and sign-extend
    int16_t value = (int16_t)parentCPU.getMemory().read16(address);
    parentCPU.R()[rd] = (int32_t)value;

    std::stringstream ss;
    ss << "Executing Thumb LDSH: R" << rd << " = [R" << rn << "(0x" << std::hex << parentCPU.R()[rn] 
       << ") + R" << rm << "(0x" << std::hex << parentCPU.R()[rm] << ")] = [0x" << std::hex << address 
       << "] = 0x" << std::hex << (uint32_t)parentCPU.R()[rd];
    Debug::log::info(ss.str());
}

void ThumbCPU::thumb_str_immediate_offset(uint16_t instruction) {
    uint8_t rd = instruction & 0x07;              // Source register (bits 2:0)
    uint8_t rb = (instruction >> 3) & 0x07;       // Base register (bits 5:3)
    uint8_t offset5 = (instruction >> 6) & 0x1F;  // Immediate offset (bits 10:6)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rb] + (offset5 << 2); // Offset scaled by 4 for word alignment

    // Perform the store operation using memory_write_32
    parentCPU.getMemory().write32(address, parentCPU.R()[rd]);

    Debug::log::info("Executing Thumb STR (immediate offset): [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::thumb_ldr_immediate_offset(uint16_t instruction) {
    uint8_t rd = instruction & 0x07;              // Destination register (bits 2:0)
    uint8_t rb = (instruction >> 3) & 0x07;       // Base register (bits 5:3)
    uint8_t offset5 = (instruction >> 6) & 0x1F;  // Immediate offset (bits 10:6)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rb] + (offset5 << 2); // Offset scaled by 4 for word alignment

    // Perform the load operation using memory_read_32
    parentCPU.R()[rd] = parentCPU.getMemory().read32(address);

    Debug::log::info("Executing Thumb LDR (immediate offset): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::thumb_str_immediate_offset_byte(uint16_t instruction) {
    uint8_t rd = instruction & 0x07;              // Source register (bits 2:0)
    uint8_t rb = (instruction >> 3) & 0x07;       // Base register (bits 5:3)
    uint8_t offset5 = (instruction >> 6) & 0x1F;  // Immediate offset (bits 10:6)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rb] + offset5; // Byte offset

    // Perform the store operation using memory_write_8
    parentCPU.getMemory().write8(address, parentCPU.R()[rd] & 0xFF); // Store only the least significant byte

    Debug::log::info("Executing Thumb STR (immediate offset byte): [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::thumb_ldr_immediate_offset_byte(uint16_t instruction) {
    uint8_t rd = instruction & 0x07;              // Destination register (bits 2:0)
    uint8_t rb = (instruction >> 3) & 0x07;       // Base register (bits 5:3)
    uint8_t offset5 = (instruction >> 6) & 0x1F;  // Immediate offset (bits 10:6)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rb] + offset5; // Byte offset

    // Perform the load operation using memory_read_8
    parentCPU.R()[rd] = parentCPU.getMemory().read8(address);

    Debug::log::info("Executing Thumb LDR (immediate offset byte): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::thumb_strh_imm(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Source register (bits 2-0)
    uint8_t rb = (instruction >> 3) & 0x07; // Base register (bits 5-3)
    uint8_t offset5 = (instruction >> 6) & 0x1F; // Immediate offset (bits 10-6)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rb] + (offset5 << 1); // Offset scaled by 2 for halfword alignment

    // Perform the store operation using memory_write_16
    parentCPU.getMemory().write16(address, parentCPU.R()[rd] & 0xFFFF); // Store only the least significant halfword

    Debug::log::info("Executing Thumb STRH (immediate offset): [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::thumb_ldrh_imm(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Destination register (bits 2-0)
    uint8_t rb = (instruction >> 3) & 0x07; // Base register (bits 5-3)
    uint8_t offset5 = (instruction >> 6) & 0x1F; // Immediate offset (bits 10-6)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rb] + (offset5 << 1); // Offset scaled by 2 for halfword alignment

    // Perform the load operation using memory_read_16
    parentCPU.R()[rd] = parentCPU.getMemory().read16(address);

    Debug::log::info("Executing Thumb LDRH (immediate offset): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::thumb_str_sp_rel(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[13] + (offset << 2); // SP-relative addressing with word alignment

    // Perform the store operation using memory_write_32
    parentCPU.getMemory().write32(address, parentCPU.R()[rd]);

    Debug::log::info("Executing Thumb STR (SP-relative): [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::thumb_ldr_sp_rel(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[13] + (offset << 2); // SP-relative addressing with word alignment

    // Perform the load operation using memory_read_32
    parentCPU.R()[rd] = parentCPU.getMemory().read32(address);

    Debug::log::info("Executing Thumb LDR (SP-relative): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::thumb_ldr_address_pc(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address (ADD Rd, PC, #imm)
    uint32_t address = (parentCPU.R()[15] & ~0x3) + (offset << 2); // PC-relative addressing with word alignment

    // Store the calculated address in the destination register
    parentCPU.R()[rd] = address;

    Debug::log::info("Executing Thumb ADD (PC-relative): R" + std::to_string(rd) + " = 0x" + std::to_string(address));
}

void ThumbCPU::thumb_ldr_address_sp(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address (ADD Rd, SP, #imm)
    uint32_t address = parentCPU.R()[13] + (offset << 2); // SP-relative addressing with word alignment

    // Store the calculated address in the destination register
    parentCPU.R()[rd] = address;

    Debug::log::info("Executing Thumb ADD (SP-relative): R" + std::to_string(rd) + " = 0x" + std::to_string(address));
}

void ThumbCPU::thumb_ldr_pc_rel(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to load from (LDR Rd, [PC, #imm])
    // PC value used is current instruction address word-aligned (without adding 4)
    uint32_t instruction_address = parentCPU.R()[15] - 2; // Current PC - 2 to get instruction address
    uint32_t pc_base = (instruction_address + 4) & ~0x3; // Pipeline PC: instruction address + 4, then word align
    uint32_t address = pc_base + (offset << 2);

    // Perform the load operation using memory_read_32
    parentCPU.R()[rd] = parentCPU.getMemory().read32(address);

    Debug::log::info("Executing Thumb LDR (PC-relative): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::thumb_add_sub_offset_to_stack_pointer(uint16_t instruction) {
    uint8_t sign = (instruction >> 7) & 0x01; // Sign bit (bit 7)
    uint16_t offset = instruction & 0x7F; // Immediate offset (bits 0-6)

    // Perform the addition or subtraction operation
    if (sign == 0) {
        parentCPU.R()[13] += (offset << 2); // Add offset scaled by 4
        Debug::log::info("Executing Thumb ADD offset to SP: SP = SP + " + std::to_string(offset << 2));
    } else {
        parentCPU.R()[13] -= (offset << 2); // Subtract offset scaled by 4
        Debug::log::info("Executing Thumb SUB offset from SP: SP = SP - " + std::to_string(offset << 2));
    }
}

void ThumbCPU::thumb_push_registers(uint16_t instruction) {
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // Count the number of registers to push
    int register_count = 0;
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            register_count++;
        }
    }

    // Decrement SP by total amount first
    parentCPU.R()[13] -= register_count * 4;
    uint32_t base_address = parentCPU.R()[13];

    // Push registers onto the stack in ascending order of addresses
    int offset = 0;
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            uint32_t address = base_address + (offset * 4);
            parentCPU.getMemory().write32(address, parentCPU.R()[i]); // Write register to memory
            Debug::log::info("Pushing R" + std::to_string(i) + " onto stack: [0x" + std::to_string(address) + "] = R" + std::to_string(i));
            offset++;
        }
    }
}

void ThumbCPU::thumb_push_registers_and_lr(uint16_t instruction) {
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // Count the number of registers to push
    int register_count = 0;
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            register_count++;
        }
    }
    register_count++; // Add 1 for LR

    // Decrement SP by total amount first
    parentCPU.R()[13] -= register_count * 4;
    uint32_t base_address = parentCPU.R()[13];

    // Push registers onto the stack in ascending order of addresses
    int offset = 0;
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            uint32_t address = base_address + (offset * 4);
            parentCPU.getMemory().write32(address, parentCPU.R()[i]); // Write register to memory
            Debug::log::info("Pushing R" + std::to_string(i) + " onto stack: [0x" + std::to_string(address) + "] = R" + std::to_string(i));
            offset++;
        }
    }

    // Push LR onto the stack (at the highest address)
    uint32_t lr_address = base_address + (offset * 4);
    parentCPU.getMemory().write32(lr_address, parentCPU.R()[14]); // Write LR to memory
    Debug::log::info("Pushing LR onto stack: [0x" + std::to_string(lr_address) + "] = LR");
}

void ThumbCPU::thumb_pop_registers(uint16_t instruction) {
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // Pop registers from the stack
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            parentCPU.R()[i] = parentCPU.getMemory().read32(parentCPU.R()[13]); // Read register from memory
            Debug::log::info("Popping R" + std::to_string(i) + " from stack: R" + std::to_string(i) + " = [0x" + std::to_string(parentCPU.R()[13]) + "]");
            parentCPU.R()[13] += 4; // Increment SP by 4
        }
    }
}

void ThumbCPU::thumb_pop_registers_and_pc(uint16_t instruction) {
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // Pop registers from the stack
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            parentCPU.R()[i] = parentCPU.getMemory().read32(parentCPU.R()[13]); // Read register from memory
            Debug::log::info("Popping R" + std::to_string(i) + " from stack: R" + std::to_string(i) + " = [0x" + std::to_string(parentCPU.R()[13]) + "]");
            parentCPU.R()[13] += 4; // Increment SP by 4
        }
    }

    // Pop PC from the stack
    parentCPU.R()[15] = parentCPU.getMemory().read32(parentCPU.R()[13]); // Read PC from memory
    Debug::log::info("Popping PC from stack: PC = [0x" + std::to_string(parentCPU.R()[13]) + "]");
    parentCPU.R()[13] += 4; // Increment SP by 4
}

void ThumbCPU::thumb_stmia(uint16_t instruction) {
    uint8_t rn = (instruction >> 8) & 0x07; // Base register (bits 8-10)
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // Store multiple registers to memory
    uint32_t address = parentCPU.R()[rn];
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            parentCPU.getMemory().write32(address, parentCPU.R()[i]); // Write register to memory
            Debug::log::info("Storing R" + std::to_string(i) + " to [0x" + std::to_string(address) + "]");
            address += 4; // Increment address by 4
        }
    }

    // Update the base register
    parentCPU.R()[rn] = address;
}

void ThumbCPU::thumb_ldmia(uint16_t instruction) {
    uint8_t rn = (instruction >> 8) & 0x07; // Base register (bits 8-10)
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // Load multiple registers from memory
    uint32_t address = parentCPU.R()[rn];
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            parentCPU.R()[i] = parentCPU.getMemory().read32(address); // Read register from memory
            Debug::log::info("Loading R" + std::to_string(i) + " from [0x" + std::to_string(address) + "]");
            address += 4; // Increment address by 4
        }
    }

    // Update the base register only if it's not in the register list
    // If it's in the register list, it already got loaded with the memory value
    if (!(register_list & (1 << rn))) {
        parentCPU.R()[rn] = address;
    }
}

void ThumbCPU::thumb_beq(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (parentCPU.CPSR() & CPU::FLAG_Z) { // Check Zero flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BEQ: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bne(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_Z)) { // Check Zero flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BNE: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bcs(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (parentCPU.CPSR() & CPU::FLAG_C) { // Check Carry flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BCS: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bcc(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_C)) { // Check Carry flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BCC: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bmi(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (parentCPU.CPSR() & CPU::FLAG_N) { // Check Negative flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BMI: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bpl(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_N)) { // Check Negative flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BPL: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bvs(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (parentCPU.CPSR() & CPU::FLAG_V) { // Check Overflow flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BVS: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bvc(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_V)) { // Check Overflow flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BVC: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bhi(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if ((parentCPU.CPSR() & CPU::FLAG_C) && !(parentCPU.CPSR() & CPU::FLAG_Z)) { // Check Carry and Zero flags
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BHI: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bls(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_C) || (parentCPU.CPSR() & CPU::FLAG_Z)) { // Check Carry and Zero flags
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BLS: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bge(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    bool n_flag = (parentCPU.CPSR() & CPU::FLAG_N) != 0;
    bool v_flag = (parentCPU.CPSR() & CPU::FLAG_V) != 0;
    if (n_flag == v_flag) { // Check if Negative and Overflow flags have same value
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BGE: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_blt(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    bool n_flag = (parentCPU.CPSR() & CPU::FLAG_N) != 0;
    bool v_flag = (parentCPU.CPSR() & CPU::FLAG_V) != 0;
    if (n_flag != v_flag) { // Check if Negative and Overflow flags have different values
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BLT: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bgt(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    bool z_flag = (parentCPU.CPSR() & CPU::FLAG_Z) != 0;
   
    bool n_flag = (parentCPU.CPSR() & CPU::FLAG_N) != 0;
    bool v_flag = (parentCPU.CPSR() & CPU::FLAG_V) != 0;
    if (!z_flag && (n_flag == v_flag)) { // Check Zero, Negative, and Overflow flags
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BGT: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_ble(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    bool z_flag = (parentCPU.CPSR() & CPU::FLAG_Z) != 0;
    bool n_flag = (parentCPU.CPSR() & CPU::FLAG_N) != 0;
    bool v_flag = (parentCPU.CPSR() & CPU::FLAG_V) != 0;
    if (z_flag || (n_flag != v_flag)) { // Check Zero, Negative, and Overflow flags
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BLE: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_swi(uint16_t instruction) {
    uint8_t comment = instruction & 0xFF; // Software interrupt comment (bits 0-7)

    // Handle the software interrupt
    Debug::log::info("Executing Thumb SWI: Software interrupt with comment 0x" + std::to_string(comment));
    //handle_software_interrupt(comment); // Call the software interrupt handler
}

void ThumbCPU::thumb_b(uint16_t instruction) {
    int16_t offset = instruction & 0x7FF; // Signed 11-bit offset (bits 0-10)
    if (offset & 0x400) { // Sign-extend the offset
        offset |= 0xF800;
    }

    // Perform the branch operation
    parentCPU.R()[15] += (offset << 1); // Branch to target address
    Debug::log::info("Executing Thumb B: Branch to 0x" + Debug::toHexString(parentCPU.R()[15], 8));
}

void ThumbCPU::thumb_bl(uint16_t instruction) {
    // BL is a two-part instruction in Thumb mode
    // First instruction (F000-F7FF): Sets up high part of target address
    // Second instruction (F800-FFFF): Completes the branch with link
    
    if ((instruction & 0xF800) == 0xF000) {
        // First part: BL prefix - store high part of offset in LR
        int32_t high_offset = (instruction & 0x7FF); // Extract 11-bit value
        if (high_offset & 0x400) { // Sign extend from bit 10
            high_offset |= 0xFFFFF800; // Sign extend to 32 bits
        }
        high_offset <<= 12; // Shift to position (bits 12-22)
        
        // Store PC (of first instruction) + 4 + high_offset in LR
        parentCPU.R()[14] = (parentCPU.R()[15] - 2) + 4 + high_offset; 
        Debug::log::info("Executing Thumb BL (first part): Storing intermediate value");
    } else if ((instruction & 0xF800) == 0xF800) {
        // Second part: BL suffix - complete the branch
        uint32_t low_offset = (instruction & 0x7FF) << 1; // Bits 0-10 shifted left by 1
        uint32_t target = parentCPU.R()[14] + low_offset; // Add to stored value from first part
        
        // Set return address in LR (PC after this instruction) with Thumb bit set
        parentCPU.R()[14] = parentCPU.R()[15] | 1; 
        
        // Branch to target
        parentCPU.R()[15] = target & 0xFFFFFFFE; // Clear bit 0 for alignment
        Debug::log::info("Executing Thumb BL (second part): Branch to 0x" + Debug::toHexString(parentCPU.R()[15], 8) + " with link, LR=0x" + Debug::toHexString(parentCPU.R()[14], 8));
    }
}