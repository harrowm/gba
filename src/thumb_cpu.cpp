
#include "thumb_cpu.h"
#include "arm_cpu.h"
#include "debug.h" // Use debug system
#include "thumb_timing.h"
#include "utility_macros.h"
#include "scheduler.h"
#include <sstream>
#include <iomanip>
#include <capstone/capstone.h>
#include <stdio.h>

// BIOS tracing flag (shared with ARM)
extern bool g_trace_bios;
extern bool g_trace_all;
extern uint32_t g_trace_max_instructions;
static uint64_t thumb_instruction_count = 0;

// External PC tracker for debug output from memory.cpp
extern uint32_t g_cpu_pc;

// Define the static constexpr members
constexpr void (ThumbCPU::*ThumbCPU::thumb_instruction_table[256])(uint16_t);
constexpr void (ThumbCPU::*ThumbCPU::thumb_alu_operations_table[16])(uint8_t, uint8_t);


ThumbCPU::ThumbCPU(CPU& cpu) : parentCPU(cpu), capstone_handle(0) {
    if (cs_open(CS_ARCH_ARM, CS_MODE_THUMB, &capstone_handle) != CS_ERR_OK) {
        DEBUG_ERROR("Failed to initialize Capstone for Thumb mode");
    } else {
        cs_option(capstone_handle, CS_OPT_DETAIL, CS_OPT_ON);
    }
}

ThumbCPU::~ThumbCPU() {
    if (capstone_handle) {
        cs_close(&capstone_handle);
    }
}

void ThumbCPU::execute(uint32_t cycles) {
    // Use macro-based debug system
    DEBUG_INFO(("Executing Thumb instructions for " + std::to_string(cycles) + " cycles").c_str());
    
    while (cycles > 0) {
        // Check if we're still in Thumb mode - if not, break out early
        if (!parentCPU.getFlag(CPU::FLAG_T)) {
            DEBUG_INFO("Mode switched to ARM during execution, breaking out of Thumb execution");
            break;
        }
        
        // HACK - do we need to model the cpu pipeline?
        uint32_t current_pc = parentCPU.R()[15];
        
        // SP tracing for comparison with mGBA
        extern void trace_sp(uint32_t pc, uint32_t sp, const char* mode);
        trace_sp(current_pc, parentCPU.R()[13], "THUMB");
        
        if (thumb_instruction_count < 3) {
            LOG_TRACE_CAT("[THUMB DEBUG #%llu] Read current_pc=0x%08X from R[15], FLAG_T=%d\n",
                   thumb_instruction_count, current_pc, parentCPU.getFlag(CPU::FLAG_T));
            LOG_TRACE_CAT("  R[14]=0x%08X, R[15]=0x%08X\n", parentCPU.R()[14], parentCPU.R()[15]);
        }
        uint16_t instruction = parentCPU.getMemory().read16(current_pc); // Fetch instruction
        uint8_t opcode = instruction >> 8;
        
        // BIOS tracing for THUMB instructions
        bool pc_in_bios = (current_pc < 0x4000);
        bool should_trace = (g_trace_bios && pc_in_bios) || (g_trace_all && thumb_instruction_count <= g_trace_max_instructions);
        
        if (should_trace) {
            // Read key I/O registers (use direct I/O to avoid affecting timing)
            uint16_t ie = parentCPU.getMemory().readDirectIO16(0x04000200);
            uint16_t irq_flags = parentCPU.getMemory().readDirectIO16(0x04000202);
            uint32_t ime = parentCPU.getMemory().readDirectIO32(0x04000208);
            
            // Print in mGBA-compatible compact format (matches the format from the trace script)
            LOG_TRACE_CAT("PC:%08X R00:%08X R01:%08X R02:%08X R03:%08X R04:%08X R05:%08X R06:%08X R07:%08X R08:%08X R09:%08X R10:%08X R11:%08X R12:%08X R13:%08X R14:%08X R15:%08X CPSR:%08X | IE:%04X IF:%04X IME:%08X\n",
                   current_pc,
                   parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3],
                   parentCPU.R()[4], parentCPU.R()[5], parentCPU.R()[6], parentCPU.R()[7],
                   parentCPU.R()[8], parentCPU.R()[9], parentCPU.R()[10], parentCPU.R()[11],
                   parentCPU.R()[12], parentCPU.R()[13], parentCPU.R()[14], parentCPU.R()[15],
                   parentCPU.CPSR(),
                   ie, irq_flags, ime);
            
            thumb_instruction_count++;
            
            // Optional: Add disassembly on second line
            if (capstone_handle) {
                cs_insn* insn;
                size_t count = cs_disasm(capstone_handle,
                                         reinterpret_cast<const uint8_t*>(&instruction),
                                         sizeof(instruction),
                                         current_pc, 1, &insn);
                if (count > 0) {
                    LOG_TRACE_CAT("     ; %s %s\n", insn[0].mnemonic, insn[0].op_str);
                    cs_free(insn, count);
                }
            }
        }
        
        // Debug the infinite loop at 0x120-0x126
        static uint64_t loop_trace_count = 0;
        uint32_t pc = parentCPU.R()[15];
        if (pc >= 0x120 && pc <= 0x126 && loop_trace_count < 20) {
            uint32_t cpsr = parentCPU.CPSR();
            LOG_TRACE_CAT("[LOOP #%llu] PC=0x%04X Instr=0x%04X | R0=%08X R1=%08X R4=%08X | CPSR=0x%08X (N=%u Z=%u C=%u V=%u)\n",
                   loop_trace_count++, pc, instruction,
                   parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[4],
                   cpsr,
                   (cpsr >> 31) & 1,  // N flag
                   (cpsr >> 30) & 1,  // Z flag
                   (cpsr >> 29) & 1,  // C flag
                   (cpsr >> 28) & 1); // V flag
        }
        
        // Use debug macros for instruction fetch logging
        DEBUG_INFO("Fetched Thumb instruction: " + debug_to_hex_string(instruction, 4) + 
                   " at PC: " + debug_to_hex_string(parentCPU.R()[15], 8));
        
        parentCPU.R()[15] += 2; // Increment PC for Thumb instructions
        
        // Use debug macros for PC increment logging
        DEBUG_INFO("Incremented PC to: " + debug_to_hex_string(parentCPU.R()[15], 8));
        
        // Capstone disassembly hook
        if (g_disassemble_enabled && capstone_handle) {
            cs_insn* insn;
            size_t count = cs_disasm(capstone_handle,
                                     reinterpret_cast<const uint8_t*>(&instruction),
                                     sizeof(instruction),
                                     parentCPU.R()[15] - 2, 1, &insn);
            if (count > 0) {
                LOG_TRACE_CAT("[DISASM][THUMB] 0x%08X: %s %s\n", (unsigned int)(parentCPU.R()[15] - 2), insn[0].mnemonic, insn[0].op_str);
                cs_free(insn, count);
            } else {
                LOG_TRACE_CAT("[DISASM][THUMB] 0x%08X: <failed to disassemble>\n", (unsigned int)(parentCPU.R()[15] - 2));
            }
        }
        // Decode and execute the instruction
        g_cpu_pc = current_pc; // Track PC for debug output in memory.cpp
        if (thumb_instruction_table[opcode]) {
            (this->*thumb_instruction_table[opcode])(instruction);
        } else {
            DEBUG_ERROR("Unknown Thumb instruction");
        }
        cycles -= 1; // Placeholder for cycle deduction
    }
}

// Calculate cycles for next instruction with branch prediction
uint32_t ThumbCPU::calculateInstructionCycles(uint16_t instruction) {
    // Convert CPU registers to array format for the C function
    uint32_t registers[16];
    for (int i = 0; i < 16; i++) {
        registers[i] = parentCPU.R()[i];
    }
    
    uint32_t pc = parentCPU.R()[15];

    // Targeted trace around Sonic branch to UNKNOWN region (Thumb)
    if ((pc >= 0x08097240 && pc <= 0x08097260) || (pc >= 0x08097390 && pc <= 0x080973B0)) {
        static int trace_count = 0;
        if (trace_count++ < 20) {
            uint16_t instr = parentCPU.getMemory().readDirectIO16(pc);
            LOG_TRACE_CAT("[TRACE THUMB] PC=0x%08X instr=0x%04X CPSR=0x%08X R0=0x%08X R1=0x%08X R2=0x%08X R3=0x%08X SP=0x%08X LR=0x%08X\n",
                     pc, instr, parentCPU.CPSR(), parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3], parentCPU.R()[13], parentCPU.R()[14]);
        }
    }
    uint32_t cpsr = parentCPU.CPSR();
    uint32_t base_cycles = thumb_calculate_instruction_cycles(instruction, pc, registers, cpsr);
    
    return base_cycles;
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

    DEBUG_INFO("Executing Thumb LSL: R" + std::to_string(rd) + " = R" + std::to_string(rs) + " << " + std::to_string(shift_amount));
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

    DEBUG_INFO("Executing Thumb LSR: R" + std::to_string(rd) + " = R" + std::to_string(rs) + " >> " + std::to_string(shift_amount));
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

    DEBUG_INFO("Executing Thumb ASR: R" + std::to_string(rd) + " = R" + std::to_string(rs) + " >> " + std::to_string(shift_amount));
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

    // Update flags using the original CPU flag update methods
    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    parentCPU.updateCFlagAdd(op1, op2);
    parentCPU.updateVFlag(op1, op2, result);

    DEBUG_INFO("Executing Thumb ADD (register): R" + std::to_string(rd) + " = R" + std::to_string(rs) + " + R" + std::to_string(rn));
}

void ThumbCPU::thumb_add_offset(uint16_t instruction) {
    uint8_t rd = bits2to0(instruction);
    uint8_t rs = bits5to3(instruction);
    uint8_t offset = bits8to6(instruction);

    // Perform the addition operation
    uint32_t op1 = parentCPU.R()[rs];
    uint32_t result = op1 + offset;

    // Debug logging for 0x0122
    uint32_t pc = parentCPU.R()[15];
    if (pc == 0x0124) { // PC is already incremented by 2 at this point
        static int add_count = 0;
        if (add_count < 10) {
            LOG_TRACE_CAT("[ADDS @0x0122] instr=0x%04x rd=%d rs=%d offset=%d | R[%d]=0x%08X + %d = 0x%08X\n",
                   instruction, rd, rs, offset, rs, op1, offset, result);
            add_count++;
        }
    }

    // Update the destination register
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result); // Zero flag
    parentCPU.updateNFlag(result); // Negative flag
    parentCPU.updateCFlagAdd(op1, offset); // Carry flag
    parentCPU.updateVFlag(op1, offset, result); // Overflow flag

    DEBUG_INFO("Executing Thumb ADD (offset): R" + std::to_string(rd) + " = R" + std::to_string(rs) + " + " + std::to_string(offset));
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

    DEBUG_INFO("Executing Thumb SUB (register): R" + std::to_string(rd) + " = R" + std::to_string(rs) + " - R" + std::to_string(rn));
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
    
    DEBUG_INFO("Executing Thumb SUB (offset): R" + std::to_string(rd) + " = R" + std::to_string(rs) + " - " + std::to_string(offset));
}

void ThumbCPU::thumb_mov_imm(uint16_t instruction) {
    uint8_t rd = bits10to8(instruction);
    uint8_t imm = bits7to0(instruction);

    parentCPU.R()[rd] = imm;
    parentCPU.updateZFlag(parentCPU.R()[rd]); // No negative, carry-out or overflow for MOV
    parentCPU.clearFlag(CPU::FLAG_N); // N flag is always cleared for this instruction as the 8bit immediate is always non-negative

    DEBUG_INFO("Executing Thumb MOV (immediate): R" + std::to_string(rd) + " = " + std::to_string(imm));
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

    DEBUG_INFO("CMP_IMM: R[" + std::to_string(rd) + "] = " + std::to_string(parentCPU.R()[rd]) + ", imm = " + std::to_string(imm));
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
    
    DEBUG_INFO("Executing Thumb ADD (immediate): R" + std::to_string(rd) + " = R" + std::to_string(rd) + " + " + std::to_string(imm));
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

    DEBUG_INFO("Executing Thumb SUB (immediate): R" + std::to_string(rd) + " = R" + std::to_string(rd) + " - " + std::to_string(imm));
}

void ThumbCPU::thumb_alu_operations(uint16_t instruction) {
    uint8_t sub_opcode = bits9to6(instruction);
    uint8_t rd = bits2to0(instruction);
    uint8_t rs = bits5to3(instruction);
    
    // Use original implementation for all operations
    if (thumb_alu_operations_table[sub_opcode] != NULL) {
         (this->*thumb_alu_operations_table[sub_opcode])(rd, rs);
    } else {
        DEBUG_ERROR("Undefined ALU operation: sub-opcode " + std::to_string(sub_opcode));
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

    DEBUG_INFO("Executing Thumb AND: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " & R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_eor(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 ^ op2;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // No update to V and C flags for EOR operation

    DEBUG_INFO("Executing Thumb EOR: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " ^ R" + std::to_string(rs));
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

    DEBUG_INFO("Executing Thumb LSL: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " << R" + std::to_string(rs));
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

    DEBUG_INFO("Executing Thumb LSR: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " >> R" + std::to_string(rs));
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

    DEBUG_INFO("Executing Thumb ASR: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " >> R" + std::to_string(rs));
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

    DEBUG_INFO("Executing Thumb ADC: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " + R" + std::to_string(rs) + " + Carry");
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

    DEBUG_INFO("Executing Thumb SBC: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " - R" + std::to_string(rs) + " - Borrow");
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

    DEBUG_INFO("Executing Thumb ROR: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " ROR R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_tst(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 & op2;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // No update to V and C flags for TST operation

    DEBUG_INFO("Executing Thumb TST: R" + std::to_string(rd) + " & R" + std::to_string(rs));
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

    DEBUG_INFO("Executing Thumb NEG: R" + std::to_string(rd) + " = -R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_cmp(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 - op2;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    parentCPU.updateCFlagSub(op1, op2);
    parentCPU.updateVFlagSub(op1, op2, result);

    DEBUG_INFO("Executing Thumb CMP: R" + std::to_string(rd) + " - R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_cmn(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 + op2;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    parentCPU.updateCFlagAdd(op1, op2);
    parentCPU.updateVFlag(op1, op2, result);

    DEBUG_INFO("Executing Thumb CMN: R" + std::to_string(rd) + " + R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_orr(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 | op2;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // No update to V and C flags for ORR operation

    DEBUG_INFO("Executing Thumb ORR: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " | R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_mul(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 * op2;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // No update to V and C flags for MUL operation

    DEBUG_INFO("Executing Thumb MUL: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " * R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_bic(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 & ~op2;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // No update to V and C flags for BIC operation

    DEBUG_INFO("Executing Thumb BIC: R" + std::to_string(rd) + " = R" + std::to_string(rd) + " & ~R" + std::to_string(rs));
}

void ThumbCPU::thumb_alu_mvn(uint8_t rd, uint8_t rs) {
    uint32_t op1 = parentCPU.R()[rs];
    uint32_t result = ~op1;
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    // No update to V and C flags for MVN operation

    DEBUG_INFO("Executing Thumb MVN: R" + std::to_string(rd) + " = ~R" + std::to_string(rs));
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
                    uint32_t target = result & ~1;
                    if (target >= 0x10000000) {
                        LOG_CRASH("[THUMB MOV PC] target=0x%08X from R%d (CPSR=0x%08X)\n",
                               target, rs, parentCPU.CPSR());
                    }
                    parentCPU.R()[15] = target; // Clear bit 0 for alignment
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
                uint32_t new_pc = target & ~1;
                if (new_pc >= 0x10000000 || new_pc == 0) {
                    uint32_t bx_pc = parentCPU.R()[15] - 2;
                    LOG_CRASH("[THUMB BX] PC=0x%08X rs=R%d target=0x%08X (CPSR=0x%08X) R0-R3=%08X %08X %08X %08X\n",
                           bx_pc, rs, new_pc, parentCPU.CPSR(),
                           parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3]);
                }
                parentCPU.R()[15] = new_pc;
                
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
    uint32_t value = parentCPU.getMemory().read32(address);
    uint32_t rot = (address & 3u) * 8;
    if (rot) value = (value >> rot) | (value << (32 - rot));
    parentCPU.R()[rd] = value;

    DEBUG_INFO("Executing Thumb LDR: R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::thumb_str_word(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Source register (bits 0-2)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = (instruction >> 6) & 0x07; // Offset register (bits 6-8)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];
    
    // Get the value to store
    uint32_t value = parentCPU.R()[rd];
    
    // Debug logging disabled for performance
    // uint32_t pc = parentCPU.R()[15];
    // if (pc < 0x4000) {
    //     printf("[THUMB STR DEBUG] PC=0x%08X | rd=%d rn=%d rm=%d | R[%d]=0x%08X R[%d]=0x%08X R[%d]=0x%08X | address=0x%08X value=0x%08X\n",
    //            pc, rd, rn, rm, 
    //            rd, parentCPU.R()[rd],
    //            rn, parentCPU.R()[rn],
    //            rm, parentCPU.R()[rm],
    //            address, value);
    // }

    // Perform the store operation using memory_write_32
    parentCPU.getMemory().write32(address, value);

    DEBUG_INFO("Executing Thumb STR (word): [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::thumb_ldr_word(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = (instruction >> 6) & 0x07; // Offset register (bits 6-8)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Debug register values
    DEBUG_INFO("LDR_WORD: R" + std::to_string(rn) + "=0x" + debug_to_hex_string(parentCPU.R()[rn], 8) + 
                     ", R" + std::to_string(rm) + "=0x" + debug_to_hex_string(parentCPU.R()[rm], 8) + 
                     ", address=0x" + debug_to_hex_string(address, 8));

    // Perform the load operation using memory_read_32
    uint32_t value = parentCPU.getMemory().read32(address);
    
    parentCPU.R()[rd] = value;

    DEBUG_INFO("Executing Thumb LDR (word): R" + std::to_string(rd) + " = [0x" + debug_to_hex_string(address, 8) + "]");
}

void ThumbCPU::thumb_ldr_byte(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = (instruction >> 6) & 0x07; // Offset register (bits 6-8)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the load operation using memory_read_8
    parentCPU.R()[rd] = parentCPU.getMemory().read8(address);

    DEBUG_INFO("Executing Thumb LDR (byte): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::thumb_str_byte(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Source register (bits 0-2)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = (instruction >> 6) & 0x07; // Offset register (bits 6-8)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];
    uint8_t byteVal = parentCPU.R()[rd] & 0xFF;

    // Perform the store operation using memory_write_8
    parentCPU.getMemory().write8(address, byteVal); // Store only the least significant byte

    DEBUG_INFO("Executing Thumb STR (byte): [0x" + debug_to_hex_string(address, 8) + "] = R" + std::to_string(rd) + 
        " (R" + std::to_string(rn) + "=0x" + debug_to_hex_string(parentCPU.R()[rn], 8) + 
        " + R" + std::to_string(rm) + "=0x" + debug_to_hex_string(parentCPU.R()[rm], 8) + 
        ", data=0x" + debug_to_hex_string(parentCPU.R()[rd] & 0xFF, 2) + ")");
}

void ThumbCPU::thumb_strh(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Source register (bits 0-2)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = (instruction >> 6) & 0x07; // Offset register (bits 6-8)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the store operation using memory_write_16
    parentCPU.getMemory().write16(address, parentCPU.R()[rd] & 0xFFFF); // Store only the least significant halfword

    DEBUG_INFO("Executing Thumb STRH: [0x" + debug_to_hex_string(address, 8) + "] = R" + std::to_string(rd));
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

    DEBUG_INFO("Executing Thumb LDSB: R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::thumb_ldrh(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = (instruction >> 6) & 0x07; // Offset register (bits 6-8)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the load operation using memory_read_16
    // ARM7: misaligned LDRH rotates result right by 8
    uint32_t val = parentCPU.getMemory().read16(address);
    parentCPU.R()[rd] = (address & 1) ? (val >> 8) | (val << 24) : val;

    DEBUG_INFO("Executing Thumb LDRH: R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::thumb_ldsh(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = (instruction >> 6) & 0x07; // Offset register (bits 6-8)

    DEBUG_INFO("LDSH instruction decode: 0x" + std::to_string(instruction) + " -> rd=" + std::to_string(rd) + ", rn=" + std::to_string(rn) + ", rm=" + std::to_string(rm));

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the load: ARM7 misaligned LDRSH degrades to LDRSB
    if (address & 1) {
        int8_t sval = (int8_t)parentCPU.getMemory().read8(address);
        parentCPU.R()[rd] = (int32_t)sval;
    } else {
        int16_t value = (int16_t)parentCPU.getMemory().read16(address);
        parentCPU.R()[rd] = (int32_t)value;
    }

    DEBUG_INFO("Executing Thumb LDSH: R" + std::to_string(rd) + " = [R" + std::to_string(rn) + 
       "(0x" + debug_to_hex_string(parentCPU.R()[rn], 8) + ") + R" + std::to_string(rm) + 
       "(0x" + debug_to_hex_string(parentCPU.R()[rm], 8) + ")] = [0x" + debug_to_hex_string(address, 8) + 
       "] = 0x" + debug_to_hex_string((uint32_t)parentCPU.R()[rd], 8));
}

void ThumbCPU::thumb_str_immediate_offset(uint16_t instruction) {
    uint8_t rd = instruction & 0x07;              // Source register (bits 2:0)
    uint8_t rb = (instruction >> 3) & 0x07;       // Base register (bits 5:3)
    uint8_t offset5 = (instruction >> 6) & 0x1F;  // Immediate offset (bits 10:6)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rb] + (offset5 << 2); // Offset scaled by 4 for word alignment

    // Perform the store operation using memory_write_32
    parentCPU.getMemory().write32(address, parentCPU.R()[rd]);

    DEBUG_INFO("Executing Thumb STR (immediate offset): [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::thumb_ldr_immediate_offset(uint16_t instruction) {
    uint8_t rd = instruction & 0x07;              // Destination register (bits 2:0)
    uint8_t rb = (instruction >> 3) & 0x07;       // Base register (bits 5:3)
    uint8_t offset5 = (instruction >> 6) & 0x1F;  // Immediate offset (bits 10:6)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rb] + (offset5 << 2); // Offset scaled by 4 for word alignment

    // Perform the load operation using memory_read_32
    uint32_t value = parentCPU.getMemory().read32(address);
    // ARM7TDMI: misaligned LDR rotates result right by (misalignment * 8) bits
    uint32_t rot = (address & 3u) * 8;
    if (rot) value = (value >> rot) | (value << (32 - rot));
    
    parentCPU.R()[rd] = value;
}

void ThumbCPU::thumb_str_immediate_offset_byte(uint16_t instruction) {
    uint8_t rd = instruction & 0x07;              // Source register (bits 2:0)
    uint8_t rb = (instruction >> 3) & 0x07;       // Base register (bits 5:3)
    uint8_t offset5 = (instruction >> 6) & 0x1F;  // Immediate offset (bits 10:6)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rb] + offset5; // Byte offset
    uint8_t byteVal = parentCPU.R()[rd] & 0xFF;

    // Perform the store operation using memory_write_8
    parentCPU.getMemory().write8(address, byteVal); // Store only the least significant byte

    DEBUG_INFO("Executing Thumb STR (immediate offset byte): [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::thumb_ldr_immediate_offset_byte(uint16_t instruction) {
    uint8_t rd = instruction & 0x07;              // Destination register (bits 2:0)
    uint8_t rb = (instruction >> 3) & 0x07;       // Base register (bits 5:3)
    uint8_t offset5 = (instruction >> 6) & 0x1F;  // Immediate offset (bits 10:6)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rb] + offset5; // Byte offset

    // Perform the load operation using memory_read_8
    parentCPU.R()[rd] = parentCPU.getMemory().read8(address);

    DEBUG_INFO("Executing Thumb LDR (immediate offset byte): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::thumb_strh_imm(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Source register (bits 2-0)
    uint8_t rb = (instruction >> 3) & 0x07; // Base register (bits 5-3)
    uint8_t offset5 = (instruction >> 6) & 0x1F; // Immediate offset (bits 10-6)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rb] + (offset5 << 1); // Offset scaled by 2 for halfword alignment

    // Perform the store operation using memory_write_16
    parentCPU.getMemory().write16(address, parentCPU.R()[rd] & 0xFFFF); // Store only the least significant halfword

    DEBUG_INFO("Executing Thumb STRH (immediate offset): [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::thumb_ldrh_imm(uint16_t instruction) {
    uint8_t rd = instruction & 0x07; // Destination register (bits 2-0)
    uint8_t rb = (instruction >> 3) & 0x07; // Base register (bits 5-3)
    uint8_t offset5 = (instruction >> 6) & 0x1F; // Immediate offset (bits 10-6)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rb] + (offset5 << 1); // Offset scaled by 2 for halfword alignment

    // Perform the load operation using memory_read_16
    // ARM7: misaligned LDRH rotates result right by 8
    uint32_t val = parentCPU.getMemory().read16(address);
    parentCPU.R()[rd] = (address & 1) ? (val >> 8) | (val << 24) : val;
}

void ThumbCPU::thumb_str_sp_rel(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[13] + (offset << 2); // SP-relative addressing with word alignment

    // Perform the store operation using memory_write_32
    parentCPU.getMemory().write32(address, parentCPU.R()[rd]);

    std::ostringstream exec_stream;
    exec_stream << "Executing Thumb STR (SP-relative): [0x" << std::hex << std::uppercase << address << "] = R" << std::dec << rd;
    DEBUG_INFO(exec_stream.str());
}

void ThumbCPU::thumb_ldr_sp_rel(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[13] + (offset << 2); // SP-relative addressing with word alignment

    // Perform the load operation using memory_read_32
    uint32_t value = parentCPU.getMemory().read32(address);
    uint32_t rot = (address & 3u) * 8;
    if (rot) value = (value >> rot) | (value << (32 - rot));
    parentCPU.R()[rd] = value;

    std::ostringstream ldr_stream;
    ldr_stream << "Executing Thumb LDR (SP-relative): R" << std::dec << rd << " = [0x" << std::hex << std::uppercase << address << "]";
    DEBUG_INFO(ldr_stream.str());
}

void ThumbCPU::thumb_ldr_address_pc(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address (ADD Rd, PC, #imm)
    // In THUMB mode, PC reads as PC+4 (pipeline offset), then word-align, then add offset
    // PC is already pointing to next instruction (+2 from current), so add +2 more for +4 total
    uint32_t address = ((parentCPU.R()[15] + 2) & ~0x3) + (offset << 2);

    // Store the calculated address in the destination register
    parentCPU.R()[rd] = address;

    DEBUG_INFO("Executing Thumb ADD (PC-relative): R" + std::to_string(rd) + " = 0x" + std::to_string(address));
}

void ThumbCPU::thumb_ldr_address_sp(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address (ADD Rd, SP, #imm)
    uint32_t address = parentCPU.R()[13] + (offset << 2); // SP-relative addressing with word alignment

    // Store the calculated address in the destination register
    parentCPU.R()[rd] = address;

    DEBUG_INFO("Executing Thumb ADD (SP-relative): R" + std::to_string(rd) + " = 0x" + std::to_string(address));
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
    uint32_t value = parentCPU.getMemory().read32(address);
    uint32_t rot = (address & 3u) * 8;
    if (rot) value = (value >> rot) | (value << (32 - rot));
    parentCPU.R()[rd] = value;
}

void ThumbCPU::thumb_add_sub_offset_to_stack_pointer(uint16_t instruction) {
    uint8_t sign = (instruction >> 7) & 0x01; // Sign bit (bit 7)
    uint16_t offset = instruction & 0x7F; // Immediate offset (bits 0-6)

    // Perform the addition or subtraction operation
    if (sign == 0) {
        parentCPU.R()[13] += (offset << 2); // Add offset scaled by 4
        DEBUG_INFO("Executing Thumb ADD offset to SP: SP = SP + " + std::to_string(offset << 2));
    } else {
        parentCPU.R()[13] -= (offset << 2); // Subtract offset scaled by 4
        DEBUG_INFO("Executing Thumb SUB offset from SP: SP = SP - " + std::to_string(offset << 2));
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
            DEBUG_INFO("Pushing R" + std::to_string(i) + " onto stack: [0x" + std::to_string(address) + "] = R" + std::to_string(i));
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
    
    // Track when SP enters the danger zone (0x03007EA0-0x03007EA8) - DISABLED FOR SPEED
    // if (base_address <= 0x03007EA8 && base_address >= 0x03007E00) {
    //     fprintf(stderr, "[SP DANGER PUSH] PC=0x%08X: SP %08X -> %08X (pushed %d regs + LR = %d bytes) LR=0x%08X\n",
    //             parentCPU.R()[15], sp_before, base_address, register_count - 1, register_count * 4, parentCPU.R()[14]);
    // }

    // Push registers onto the stack in ascending order of addresses
    int offset = 0;
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            uint32_t address = base_address + (offset * 4);
            parentCPU.getMemory().write32(address, parentCPU.R()[i]); // Write register to memory
            DEBUG_INFO("Pushing R" + std::to_string(i) + " onto stack: [0x" + std::to_string(address) + "] = R" + std::to_string(i));
            offset++;
        }
    }

    // Push LR onto the stack (at the highest address)
    uint32_t lr_address = base_address + (offset * 4);
    parentCPU.getMemory().write32(lr_address, parentCPU.R()[14]); // Write LR to memory
    DEBUG_INFO("Pushing LR onto stack: [0x" + std::to_string(lr_address) + "] = LR");
}

void ThumbCPU::thumb_pop_registers(uint16_t instruction) {
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)
    
    // Pop registers from the stack
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            uint32_t sp = parentCPU.R()[13];
            uint32_t val = parentCPU.getMemory().read32(sp);
            parentCPU.R()[i] = val;
            DEBUG_INFO("Popping R" + std::to_string(i) + " from stack: R" + std::to_string(i) + " = [0x" + std::to_string(parentCPU.R()[13]) + "]");
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
            DEBUG_INFO("Popping R" + std::to_string(i) + " from stack: R" + std::to_string(i) + " = [0x" + std::to_string(parentCPU.R()[13]) + "]");
            parentCPU.R()[13] += 4; // Increment SP by 4
        }
    }

    // Pop PC from the stack
    uint32_t new_pc = parentCPU.getMemory().read32(parentCPU.R()[13]); // Read PC from memory
    if (new_pc & 1) {
        parentCPU.setFlag(CPU::FLAG_T);
    } else {
        parentCPU.clearFlag(CPU::FLAG_T);
    }
    parentCPU.R()[15] = new_pc & 0xFFFFFFFE; // Clear bit 0 for alignment
    DEBUG_INFO("Popping PC from stack: PC = [0x" + std::to_string(parentCPU.R()[13]) + "]");
    parentCPU.R()[13] += 4; // Increment SP by 4
}

void ThumbCPU::thumb_stmia(uint16_t instruction) {
    uint8_t rn = (instruction >> 8) & 0x07; // Base register (bits 8-10)
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // TEMPORARY DEBUG for all STM instructions
    static int stm_count = 0;
    if (stm_count < 5 || instruction == 0xC0CA) {
        LOG_STACK("[STM #%d] PC=0x%08X instruction=0x%04X rn=%d reglist=0x%02X\n", 
               stm_count, parentCPU.R()[15] - 2, instruction, rn, register_list);
        LOG_STACK("[STM #%d] BEFORE: r0=0x%08X r1=0x%08X r2=0x%08X r3=0x%08X\n",
               stm_count, parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3]);
    }
    stm_count++;

    // Store multiple registers to memory
    uint32_t address = parentCPU.R()[rn];
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            static int stm_count_inner = 0;
            if (stm_count_inner < 20 || instruction == 0xC0CA) {
                LOG_STACK("[STM #%d] Writing R%d=0x%08X to address 0x%08X\n", 
                       stm_count_inner, i, parentCPU.R()[i], address);
            }
            stm_count_inner++;
            parentCPU.getMemory().write32(address, parentCPU.R()[i]); // Write register to memory
            DEBUG_INFO("Storing R" + std::to_string(i) + " to [0x" + std::to_string(address) + "]");
            address += 4; // Increment address by 4
        }
    }

    // Update the base register
    uint32_t old_rn = parentCPU.R()[rn];
    parentCPU.R()[rn] = address;
    
    static int stm_count_after = 0;
    if (stm_count_after < 5 || instruction == 0xC0CA) {
        LOG_STACK("[STM #%d] Updated R%d from 0x%08X to 0x%08X\n", stm_count_after, rn, old_rn, address);
        LOG_STACK("[STM #%d] AFTER: r0=0x%08X r1=0x%08X r2=0x%08X r3=0x%08X\n",
               stm_count_after, parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3]);
    }
    stm_count_after++;
}

void ThumbCPU::thumb_ldmia(uint16_t instruction) {
    uint8_t rn = (instruction >> 8) & 0x07; // Base register (bits 8-10)
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // Load multiple registers from memory
    uint32_t address = parentCPU.R()[rn];
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            parentCPU.R()[i] = parentCPU.getMemory().read32(address); // Read register from memory
            DEBUG_INFO("Loading R" + std::to_string(i) + " from [0x" + std::to_string(address) + "]");
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
        parentCPU.R()[15] += (offset << 1) + 2; // Branch to target address (PC+4 base)
        DEBUG_INFO("Executing Thumb BEQ: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bne(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_Z)) { // Check Zero flag
        parentCPU.R()[15] += (offset << 1) + 2; // Branch to target address (PC+4 base)
        DEBUG_INFO("Executing Thumb BNE: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bcs(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (parentCPU.CPSR() & CPU::FLAG_C) { // Check Carry flag
        parentCPU.R()[15] += (offset << 1) + 2; // Branch to target address (PC+4 base)
        DEBUG_INFO("Executing Thumb BCS: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bcc(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_C)) { // Check Carry flag
        parentCPU.R()[15] += (offset << 1) + 2; // Branch to target address (PC+4 base)
        DEBUG_INFO("Executing Thumb BCC: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bmi(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (parentCPU.CPSR() & CPU::FLAG_N) { // Check Negative flag
        parentCPU.R()[15] += (offset << 1) + 2; // Branch to target address (PC+4 base)
        DEBUG_INFO("Executing Thumb BMI: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bpl(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_N)) { // Check Negative flag
        parentCPU.R()[15] += (offset << 1) + 2; // Branch to target address (PC+4 base)
        DEBUG_INFO("Executing Thumb BPL: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bvs(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (parentCPU.CPSR() & CPU::FLAG_V) { // Check Overflow flag
        parentCPU.R()[15] += (offset << 1) + 2; // Branch to target address (PC+4 base)
        DEBUG_INFO("Executing Thumb BVS: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bvc(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_V)) { // Check Overflow flag
        parentCPU.R()[15] += (offset << 1) + 2; // Branch to target address (PC+4 base)
        DEBUG_INFO("Executing Thumb BVC: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bhi(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if ((parentCPU.CPSR() & CPU::FLAG_C) && !(parentCPU.CPSR() & CPU::FLAG_Z)) { // Check Carry and Zero flags
        parentCPU.R()[15] += (offset << 1) + 2; // Branch to target address (PC+4 base)
        DEBUG_INFO("Executing Thumb BHI: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bls(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_C) || (parentCPU.CPSR() & CPU::FLAG_Z)) { // Check Carry and Zero flags
        parentCPU.R()[15] += (offset << 1) + 2; // Branch to target address (PC+4 base)
        DEBUG_INFO("Executing Thumb BLS: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bge(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    bool n_flag = (parentCPU.CPSR() & CPU::FLAG_N) != 0;
    bool v_flag = (parentCPU.CPSR() & CPU::FLAG_V) != 0;
    if (n_flag == v_flag) { // Check if Negative and Overflow flags have same value
        parentCPU.R()[15] += (offset << 1) + 2; // Branch to target address (PC+4 base)
        DEBUG_INFO("Executing Thumb BGE: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_blt(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    bool n_flag = (parentCPU.CPSR() & CPU::FLAG_N) != 0;
    bool v_flag = (parentCPU.CPSR() & CPU::FLAG_V) != 0;
    if (n_flag != v_flag) { // Check if Negative and Overflow flags have different values
        parentCPU.R()[15] += (offset << 1) + 2; // Branch to target address (PC+4 base)
        DEBUG_INFO("Executing Thumb BLT: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_bgt(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    bool z_flag = (parentCPU.CPSR() & CPU::FLAG_Z) != 0;
   
    bool n_flag = (parentCPU.CPSR() & CPU::FLAG_N) != 0;
    bool v_flag = (parentCPU.CPSR() & CPU::FLAG_V) != 0;
    if (!z_flag && (n_flag == v_flag)) { // Check Zero, Negative, and Overflow flags
        parentCPU.R()[15] += (offset << 1) + 2; // Branch to target address (PC+4 base)
        DEBUG_INFO("Executing Thumb BGT: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_ble(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    bool z_flag = (parentCPU.CPSR() & CPU::FLAG_Z) != 0;
    bool n_flag = (parentCPU.CPSR() & CPU::FLAG_N) != 0;
    bool v_flag = (parentCPU.CPSR() & CPU::FLAG_V) != 0;
    if (z_flag || (n_flag != v_flag)) { // Check Zero, Negative, and Overflow flags
        parentCPU.R()[15] += (offset << 1) + 2; // Branch to target address (PC+4 base)
        DEBUG_INFO("Executing Thumb BLE: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::thumb_swi(uint16_t instruction) {
    uint8_t comment = instruction & 0xFF; // Software interrupt comment (bits 0-7)

    // Track SWI calls - only log decompression SWIs
    
    // Handle the software interrupt - trigger SVC exception
    DEBUG_INFO("Executing Thumb SWI: Software interrupt with comment 0x" + std::to_string(comment));
    
    // Trigger SVC exception: Vector 0x08, mode 0x13 (Supervisor), disable IRQ
    // This will switch to ARM mode and jump to the BIOS SWI handler
    parentCPU.getARMCPU().handleException(0x08, 0x13, true, false);
    
    // Prevent unused variable warning in release builds
    UNUSED(comment);
}

void ThumbCPU::thumb_b(uint16_t instruction) {
    // uint32_t pc_before = parentCPU.R()[15];
    int16_t offset = instruction & 0x7FF; // Signed 11-bit offset (bits 0-10)
    if (offset & 0x400) { // Sign-extend the offset
        offset |= 0xF800;
    }

    // Perform the branch operation
    // PC has already been incremented by +2 after fetch, so we only add +2 more to make PC+4 base
    int32_t branch_offset = (offset << 1) + 2;
    // printf("[THUMB_B @0x%08X] offset=%d, branch_offset=%d\n", pc_before, offset, branch_offset);
    parentCPU.R()[15] += branch_offset; // Branch to target address (PC+4 base)
    // printf("[THUMB_B] pc_after=0x%08X\n", parentCPU.R()[15]);
}

void ThumbCPU::thumb_bl(uint16_t instruction) {
    // BL is a two-part instruction in Thumb mode
    // First instruction (F000-F7FF): Sets up high part of target address
    // Second instruction (F800-FFFF): Completes the branch with link
    
    uint32_t current_pc = parentCPU.R()[15] - 2;  // PC has already been incremented
    
    if ((instruction & 0xF800) == 0xF000) {
        // First part: BL prefix - store high part of offset in LR
        int32_t high_offset = (instruction & 0x7FF); // Extract 11-bit value
        if (high_offset & 0x400) { // Sign extend from bit 10
            high_offset |= 0xFFFFF800; // Sign extend to 32 bits
        }
        high_offset <<= 12; // Shift to position (bits 12-22)
        
        // Store (PC of first instruction + 4) + high_offset in LR
        // PC is already +4 ahead due to pre-increment, so current_pc + 4 = actual PC value
        // For THUMB: PC used in calculation = (address of first BL instruction) + 4
        parentCPU.R()[14] = (current_pc + 4) + high_offset; 

        if (current_pc >= 0x08097240 && current_pc <= 0x08097260) {
            LOG_BL("[TRACE BL1] PC=0x%08X instr=0x%04X high_off=0x%08X LR_temp=0x%08X\n",
                   current_pc, instruction, (uint32_t)high_offset, parentCPU.R()[14]);
        }
        
        if (g_trace_bios && current_pc < 0x4000) {
            LOG_BIOS("[BL-PART1] PC=0x%08X: BL prefix, LR_temp=0x%08X\n", current_pc, parentCPU.R()[14]);
        }
        DEBUG_INFO("Executing Thumb BL (first part): Storing intermediate value");
    } else if ((instruction & 0xF800) == 0xF800) {
        // Second part: BL suffix - complete the branch
        uint32_t low_offset = (instruction & 0x7FF) << 1; // Bits 0-10 shifted left by 1
        uint32_t lr_value = parentCPU.R()[14]; // Save LR before we change it
        uint32_t target = lr_value + low_offset; // Add to stored value from first part
        
        // Set return address in LR (PC after this instruction) with Thumb bit set
        parentCPU.R()[14] = parentCPU.R()[15] | 1; 
        
        // Branch to target
        parentCPU.R()[15] = target & 0xFFFFFFFE; // Clear bit 0 for alignment
        
        // Track call stack when SP is in danger zone - DISABLED FOR SPEED
        // uint32_t sp = parentCPU.R()[13];
        // if (sp <= 0x03007F00 && sp >= 0x03007E00) {
        //     fprintf(stderr, "[BL DANGER] PC=0x%08X: calling 0x%08X, SP=0x%08X, LR=0x%08X\n",
        //             current_pc, target & 0xFFFFFFFE, sp, parentCPU.R()[14]);
        // }

        if (current_pc >= 0x08097240 && current_pc <= 0x08097260) {
            LOG_BL("[TRACE BL2] PC=0x%08X instr=0x%04X low_off=0x%08X LR_prev=0x%08X target=0x%08X\n",
                   current_pc, instruction, low_offset, lr_value, parentCPU.R()[15]);
        }
        
        if (g_trace_bios && current_pc < 0x4000) {
            LOG_BIOS("[BL-PART2] PC=0x%08X: BL suffix, LR_prev=0x%08X, low_offset=0x%X, target=0x%08X, LR=0x%08X\n", 
                   current_pc, lr_value, low_offset, parentCPU.R()[15], parentCPU.R()[14]);
        }
        DEBUG_INFO("Executing Thumb BL (second part): Branch to 0x" + debug_to_hex_string(parentCPU.R()[15], 8) + " with link, LR=0x" + debug_to_hex_string(parentCPU.R()[14], 8));
    }
}

// ============================================================================
// Scheduler-Integrated Execution
// ============================================================================

void ThumbCPU::executeOneInstruction() {
    // Check if we're still in Thumb mode
    if (!parentCPU.getFlag(CPU::FLAG_T)) {
        DEBUG_INFO("CPU switched to ARM mode");
        return;
    }
    
    uint32_t pc = parentCPU.R()[15];
    
    // SP tracing for comparison with mGBA
    extern void trace_sp(uint32_t pc, uint32_t sp, const char* mode);
    trace_sp(pc, parentCPU.R()[13], "THUMB");
    
    // Count executions at PC=0x120
    static uint64_t count_0x120 = 0;
    static uint64_t count_0x122 = 0;
    static uint64_t count_0x124 = 0;
    static bool logged_counts = false;
    
    if (pc == 0x120) count_0x120++;
    if (pc == 0x122) count_0x122++;
    if (pc == 0x124) count_0x124++;
    
    // After 5 seconds (at ~16.78MHz, that's ~84M cycles), log the counts
    uint64_t current_cycle = parentCPU.getScheduler() ? parentCPU.getScheduler()->getCurrentCycle() : 0;
    if (!logged_counts && current_cycle > 84000000) {
        LOG_TRACE_CAT("\\n=== EXECUTION COUNTS AFTER ~5 SECONDS ===\\n");
        LOG_TRACE_CAT("PC=0x0120 executed: %llu times\\n", count_0x120);
        LOG_TRACE_CAT("PC=0x0122 executed: %llu times\\n", count_0x122);
        LOG_TRACE_CAT("PC=0x0124 executed: %llu times\\n", count_0x124);
        LOG_TRACE_CAT("Current cycle: %llu\\n", current_cycle);
        LOG_TRACE_CAT("If 0x120 count >> 128 iterations expected, we have an infinite loop!\\n");
        LOG_TRACE_CAT("=========================================\\n\\n");
        logged_counts = true;
    }
    
    // Track invalid PC values
    if (pc >= 0x10000000) {
        LOG_CRASH("[THUMB ERROR] Invalid PC=0x%08X detected! Cycle=%llu\n", pc, current_cycle);
        LOG_CRASH("  R0-R7: %08X %08X %08X %08X %08X %08X %08X %08X\n",
               parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3],
               parentCPU.R()[4], parentCPU.R()[5], parentCPU.R()[6], parentCPU.R()[7]);
        LOG_CRASH("  R8-R15: %08X %08X %08X %08X %08X %08X %08X %08X\n",
               parentCPU.R()[8], parentCPU.R()[9], parentCPU.R()[10], parentCPU.R()[11],
               parentCPU.R()[12], parentCPU.R()[13], parentCPU.R()[14], parentCPU.R()[15]);
    }
    
    // --- Open bus / BIOS prefetch tracking (Thumb mode) ---
    // Thumb instructions are 16-bit.  On ARM7TDMI the bus is 32-bit, so the
    // open-bus value depends on the memory region width.  For most 16-bit bus
    // regions the halfword is replicated: prefetch | (prefetch << 16).
    // For 32-bit bus regions (IWRAM) both pipeline halves are visible, but the
    // simple replication model covers the vast majority of cases.
    Memory& mem = parentCPU.getMemory();
    uint16_t thumbPrefetchHalf = mem.readDirectIO16(pc + 4);
    parentCPU.openBusPrefetch = thumbPrefetchHalf | ((uint32_t)thumbPrefetchHalf << 16);

    bool inBios = (pc < 0x4000);
    if (inBios) {
        mem.biosPrefetch = parentCPU.openBusPrefetch;
    }
    mem.cpuInBios = inBios;

    // Fetch instruction without charging wait cycles
    // ARM7TDMI has 3-stage pipeline: Fetch happens in parallel with previous instruction's execution
    // Only the execute stage costs cycles
    uint16_t instruction = mem.readDirectIO16(pc);
    
    // BIOS tracing for THUMB instructions (matching ARM trace logic)
    static uint64_t thumb_trace_count = 0;
    
    bool pc_in_bios = inBios;
    bool should_trace = (g_trace_bios && pc_in_bios) || (g_trace_all && thumb_trace_count < g_trace_max_instructions);
    
    if (should_trace) {
        // Read key I/O registers
        uint16_t ie = parentCPU.getMemory().read16(0x04000200);
        uint16_t irq_flags = parentCPU.getMemory().read16(0x04000202);
        uint32_t ime = parentCPU.getMemory().read32(0x04000208);
        
        // Print in mGBA-compatible compact format (matching ARM format)
        printf("PC:%08X R00:%08X R01:%08X R02:%08X R03:%08X R04:%08X R05:%08X R06:%08X R07:%08X R08:%08X R09:%08X R10:%08X R11:%08X R12:%08X R13:%08X R14:%08X R15:%08X CPSR:%08X | IE:%04X IF:%04X IME:%08X\n",
               pc,
               parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3],
               parentCPU.R()[4], parentCPU.R()[5], parentCPU.R()[6], parentCPU.R()[7],
               parentCPU.R()[8], parentCPU.R()[9], parentCPU.R()[10], parentCPU.R()[11],
               parentCPU.R()[12], parentCPU.R()[13], parentCPU.R()[14], parentCPU.R()[15],
               parentCPU.CPSR(),
               ie, irq_flags, ime);
        
        thumb_trace_count++;
        
        // Optional: Add disassembly on second line
        if (capstone_handle) {
            cs_insn* insn;
            size_t count = cs_disasm(capstone_handle,
                                     reinterpret_cast<const uint8_t*>(&instruction),
                                     sizeof(instruction),
                                     pc, 1, &insn);
            if (count > 0) {
                printf("     ; %s %s\n", insn[0].mnemonic, insn[0].op_str);
                cs_free(insn, count);
            }
        }
        thumb_instruction_count++;
    }
    
    // Legacy BIOS trace format (kept for compatibility with old debug code)
    if (pc_in_bios && g_trace_bios && !should_trace && capstone_handle) {
        cs_insn* insn;
        // Read key I/O registers
        uint16_t ie = parentCPU.getMemory().read16(0x04000200);
        uint16_t irq_flags = parentCPU.getMemory().read16(0x04000202);
        uint32_t ime = parentCPU.getMemory().read32(0x04000208);
        
        // Print in old format
        printf("[%llu][THUMB] PC=0x%08X | R0=%08X R1=%08X R2=%08X R3=%08X R4=%08X R5=%08X R6=%08X R7=%08X R8=%08X R9=%08X R10=%08X R11=%08X R12=%08X SP=%08X LR=%08X | IE=%04X IF=%04X IME=%08X\n",
               thumb_instruction_count++,
               pc,
               parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3],
               parentCPU.R()[4], parentCPU.R()[5], parentCPU.R()[6], parentCPU.R()[7],
               parentCPU.R()[8], parentCPU.R()[9], parentCPU.R()[10], parentCPU.R()[11],
               parentCPU.R()[12], parentCPU.R()[13], parentCPU.R()[14],
               ie, irq_flags, ime);
        
        // Optional: Add disassembly on second line
        size_t count = cs_disasm(capstone_handle,
                                 reinterpret_cast<const uint8_t*>(&instruction),
                                 sizeof(instruction),
                                 pc, 1, &insn);
        if (count > 0) {
            printf("     ; %s %s\n", insn[0].mnemonic, insn[0].op_str);
            cs_free(insn, count);
        }
    }
    
    // Debug the infinite loop at 0x120-0x126 and returns
    static uint64_t loop_trace_count = 0;
    // static uint64_t loop_entry_count = 0;
    // static uint32_t last_lr = 0;
    
    if (pc >= 0x120 && pc <= 0x126) {
        if (loop_trace_count == 0) {
            // First entry into loop
            // loop_entry_count++;
            // printf("\n[LOOP ENTRY #%llu] Called from LR=0x%08X\n", loop_entry_count, parentCPU.R()[14]);
            // last_lr = parentCPU.R()[14];
        }
        if (loop_trace_count < 5 || (loop_trace_count % 100 == 0)) {
            uint32_t cpsr __attribute__((unused)) = parentCPU.CPSR();
            // printf("[LOOP #%llu] PC=0x%04X Instr=0x%04X | R0=%08X R1=%08X R4=%08X LR=%08X\n",
            //        loop_trace_count, pc, instruction,
            //        parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[4], parentCPU.R()[14]);
        }
        loop_trace_count++;
    } else if (loop_trace_count > 0) {
        // printf("[LOOP EXIT #%llu] After %llu iterations, returning to LR=0x%08X, next PC=0x%08X\n\n", 
        //        loop_entry_count, loop_trace_count, last_lr, pc);
        loop_trace_count = 0;
    }
    
    // Trace ROM/RAM execution (non-BIOS) to find loops
    static uint64_t rom_trace_count = 0;
    static uint32_t rom_last_pc = 0;
    static int rom_repeat_count = 0;
    static bool rom_trace_enabled = false;
    
    // Enable tracing when we leave BIOS
    if (pc >= 0x4000 && !rom_trace_enabled) {
        rom_trace_enabled = true;
        LOG_REGION("\n[THUMB TRACE] Enabled - left BIOS region\n");
    }
    
    if (rom_trace_enabled && pc < 0x4000) {
        rom_trace_enabled = false;
        LOG_REGION("[THUMB TRACE] Disabled - entered BIOS\n");
    }
    
    // Log ROM/RAM execution (disabled for now to check if trace causes crash)
    // if (rom_trace_enabled && rom_trace_count < 200) {
    //     if (pc == rom_last_pc) {
    //         rom_repeat_count++;
    //         if (rom_repeat_count == 100) {
    //             printf("[THUMB LOOP!] Stuck at PC=0x%08X, instruction=0x%04X\n", pc, instruction);
    //         }
    //     } else {
    //         rom_repeat_count = 0;
    //         printf("[THUMB %llu] PC=0x%08X: I=0x%04X | R0-R3=%08X %08X %08X %08X | SP=%08X LR=%08X\n",
    //                rom_trace_count, pc, instruction,
    //                parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3],
    //                parentCPU.R()[13], parentCPU.R()[14]);
    //     }
    //     rom_last_pc = pc;
    //     rom_trace_count++;
    // }
    UNUSED(rom_trace_enabled); UNUSED(rom_trace_count); UNUSED(rom_last_pc); UNUSED(rom_repeat_count);
    
    // Calculate how many cycles this instruction will take
    uint32_t instruction_cycles = calculateInstructionCycles(instruction);
    
    // Track PC for debug output in memory.cpp
    g_cpu_pc = pc;
    mem.cpuIsThumb = true;
    
    // Increment PC before execution (Thumb instructions do this)
    parentCPU.R()[15] += 2;
    
    // Execute the instruction — bracket with begin/endInstructionCycles
    // so data access wait cycles accumulate instead of advancing scheduler
    // mid-instruction (matches mGBA's local currentCycles model).
    mem.beginInstructionCycles();
    uint8_t opcode = instruction >> 8;
    if (thumb_instruction_table[opcode]) {
        (this->*thumb_instruction_table[opcode])(instruction);
    } else {
        // Undefined/unimplemented instruction
        printf("[THUMB UNDEFINED] Opcode 0x%02X (instruction 0x%04X) at PC=0x%08X\n",
               opcode, instruction, parentCPU.R()[15] - 2);
    }
    uint32_t dataCycles = mem.endInstructionCycles();
    
    // Advance scheduler by instruction execution cycles + fetch waits
    // + accumulated data access cycles from read/write operations.
    //
    // After data transfers, the next fetch is non-sequential because the
    // data bus was used, disrupting the prefetch pipeline. This matches
    // mGBA's THUMB_LOAD_POST_BODY / THUMB_STORE_POST_BODY macros:
    //   += activeNonseqCycles16 - activeSeqCycles16
    uint16_t hi5 = instruction >> 11;
    bool isDataTransfer = (hi5 >= 0x09 && hi5 <= 0x13)  // Formats 6-11
                       || (hi5 == 0x18 || hi5 == 0x19)  // Format 15: LDMIA/STMIA
                       || ((instruction & 0xF600) == 0xB400)  // Format 14: PUSH/POP
                       || ((instruction & 0xFC00) == 0x4000 &&  // Format 4: ALU ops
                           ((instruction >> 6) & 0xF) == 0xD);  // MUL uses nonseq (like store)
    
    // Compute base fetch cost
    uint32_t fetchCycles = isDataTransfer
        ? mem.getNonseqWaitCycles16(pc)
        : mem.getSeqWaitCycles16(pc);
    
    // Game Pak prefetch buffer: when executing from ROM with prefetch enabled
    // and data accessed non-ROM memory, the prefetch unit fills during the
    // stall. The benefit is applied to fetchCycles (next instruction fetch)
    // since the prefetch buffer handles upcoming instruction reads.
    if (isDataTransfer && mem.prefetchEnabled && mem.hadNonRomAccess()
        && !mem.hadRomAccess()) {
        uint8_t pcRegion = (pc >> 24) & 0xFF;
        if (pcRegion >= 0x08 && pcRegion <= 0x0D) {
            // S16 fetch cost for current ROM region (total = extra + 1 base)
            uint32_t sFetchCost = mem.getSeqWaitCycles16(pc) + 1;
            // How many S16 fetches completed during the NON-ROM data stall?
            uint32_t nonRomStall = mem.getNonRomDataCycles();
            uint32_t completedFetches = nonRomStall / sFetchCost;
            // Thumb 16-bit fetch = 1 halfword from ROM.
            if (completedFetches >= 1) {
                // Fetch is fully prefetched: free
                fetchCycles = 0;
            }
            // If 0 fetches completed, no prefetch benefit
        }
    }
    
    // Detect branches: if PC changed non-sequentially, flush the prefetch buffer.
    // A sequential Thumb step increments PC by 2 (done before executeInstruction,
    // so after execution the new PC should be (pc + 2) + 2 = pc + 4).
    uint32_t newPc = parentCPU.R()[15];
    if (newPc != pc + 4) {
        mem.flushPrefetch();
    }
    
    parentCPU.advanceCycles(instruction_cycles + fetchCycles + dataCycles);
}