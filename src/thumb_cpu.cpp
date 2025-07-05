#include "thumb_cpu.h"
#include "debug.h"

ThumbCPU::ThumbCPU(CPU& cpu) : parentCPU(cpu) {
Debug::log::info("Initializing ThumbCPU with parent CPU");
    // Initialize any Thumb-specific state or resources here
    // For example, you might want to set up instruction decoding tables or other structures
    initializeInstructionTable();

    // Initialize the ALU operations table
    thumb_init_alu_operations_table();
    Debug::log::info("ThumbCPU initialized. Preparing instruction and ALU tables for parent CPU.");
}

ThumbCPU::~ThumbCPU() {
    // Cleanup logic if necessary
}

void ThumbCPU::thumb_init_alu_operations_table() {
    thumb_alu_operations_table[0x0] = &ThumbCPU::thumb_alu_and;
    thumb_alu_operations_table[0x1] = &ThumbCPU::thumb_alu_eor;
    thumb_alu_operations_table[0x2] = &ThumbCPU::thumb_alu_lsl;
    thumb_alu_operations_table[0x3] = &ThumbCPU::thumb_alu_lsr;
    thumb_alu_operations_table[0x4] = &ThumbCPU::thumb_alu_asr;
    thumb_alu_operations_table[0x5] = &ThumbCPU::thumb_alu_adc;
    thumb_alu_operations_table[0x6] = &ThumbCPU::thumb_alu_sbc;
    thumb_alu_operations_table[0x7] = &ThumbCPU::thumb_alu_ror;
    thumb_alu_operations_table[0x8] = &ThumbCPU::thumb_alu_tst;
    thumb_alu_operations_table[0x9] = &ThumbCPU::thumb_alu_neg;
    thumb_alu_operations_table[0xA] = &ThumbCPU::thumb_alu_cmp;
    thumb_alu_operations_table[0xB] = &ThumbCPU::thumb_alu_cmn;
    thumb_alu_operations_table[0xC] = &ThumbCPU::thumb_alu_orr;
    thumb_alu_operations_table[0xD] = &ThumbCPU::thumb_alu_mul;
    thumb_alu_operations_table[0xE] = &ThumbCPU::thumb_alu_bic;
    thumb_alu_operations_table[0xF] = &ThumbCPU::thumb_alu_mvn;
}

void ThumbCPU::initializeInstructionTable() {
    Debug::log::info("Initializing Thumb instruction table");
    // Initialize the table with nullptr
    // Initialize the table with NULL
    for (int i = 0; i < 256; i++) {
        thumb_instruction_table[i] = nullptr;
    }

    // Format 1 - move shifted register
    for (int i = 0b00000000; i <= 0b00000111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_lsl;
    }
    
    for (int i = 0b00001000; i <= 0b00001111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_lsr;
    }
        
    for (int i = 0b00010000; i <= 0b00010111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_asr;
    }

    // Format 2 - add/subtract
    thumb_instruction_table[0b00011000] = &ThumbCPU::handle_thumb_add_register; 
    thumb_instruction_table[0b00011001] = &ThumbCPU::handle_thumb_add_register; 
     
    thumb_instruction_table[0b00011010] = &ThumbCPU::handle_thumb_add_offset; 
    thumb_instruction_table[0b00011011] = &ThumbCPU::handle_thumb_add_offset; 
    
    thumb_instruction_table[0b00011100] = &ThumbCPU::handle_thumb_sub_register;
    thumb_instruction_table[0b00011101] = &ThumbCPU::handle_thumb_sub_register;
    
    thumb_instruction_table[0b00011110] = &ThumbCPU::handle_thumb_sub_offset;    
    thumb_instruction_table[0b00011111] = &ThumbCPU::handle_thumb_sub_offset;    

    // Format 3 - move/compare/add/subtract immediate
    for (int i = 0b00100000; i <= 0b00100111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_mov_imm;
    }

    for (int i = 0b00101000; i <= 0b00101111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_cmp_imm;
    }
    
    for (int i = 0b00110000; i <= 0b00110111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_add_imm;
    }

    for (int i = 0b00111000; i <= 0b00111111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_sub_imm;
    }
        
    // Format 4 - ALU operations
    // This will have to be decoded further based on the specific operation
    for (int i = 0b01000000; i <= 0b01000011; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_alu_operations; // will have to be decoded further
    }

    // Format 5 - HI register operations/branch exchange
    // These will have to be decoded further based on the specific operation
    thumb_instruction_table[0b01000100] = &ThumbCPU::handle_add_hi;
    thumb_instruction_table[0b01000101] = &ThumbCPU::handle_cmp_hi;
    thumb_instruction_table[0b01000110] = &ThumbCPU::handle_mov_hi;
    thumb_instruction_table[0b01000111] = &ThumbCPU::handle_bx_hi;
  
    // Format 6 - PC relative load
    for (int i = 0b01001000; i <= 0b01001111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_ldr; 
    }

    // Format 7 - load/store with register offset
    thumb_instruction_table[0b01010000] = &ThumbCPU::handle_thumb_str_word; 
    thumb_instruction_table[0b01010001] = &ThumbCPU::handle_thumb_str_word; 
    thumb_instruction_table[0b01010100] = &ThumbCPU::handle_thumb_str_byte; 
    thumb_instruction_table[0b01010101] = &ThumbCPU::handle_thumb_str_byte; 
    thumb_instruction_table[0b01011000] = &ThumbCPU::handle_thumb_ldr_word; 
    thumb_instruction_table[0b01011001] = &ThumbCPU::handle_thumb_ldr_word; 
    thumb_instruction_table[0b01011100] = &ThumbCPU::handle_thumb_ldr_byte; 
    thumb_instruction_table[0b01011101] = &ThumbCPU::handle_thumb_ldr_byte; 

    // Format 8 - load/store sign-extended byte/halfword
    thumb_instruction_table[0b01010010] = &ThumbCPU::handle_thumb_strh; 
    thumb_instruction_table[0b01010011] = &ThumbCPU::handle_thumb_strh; 
    thumb_instruction_table[0b01010110] = &ThumbCPU::handle_thumb_ldsb; 
    thumb_instruction_table[0b01010111] = &ThumbCPU::handle_thumb_ldsb; 
    thumb_instruction_table[0b01011010] = &ThumbCPU::handle_thumb_ldrh; 
    thumb_instruction_table[0b01011011] = &ThumbCPU::handle_thumb_ldrh; 
    thumb_instruction_table[0b01011110] = &ThumbCPU::handle_thumb_ldsh; 
    thumb_instruction_table[0b01011111] = &ThumbCPU::handle_thumb_ldsh; 

    // Format 9 - load/store with immediate offset
    for (int i = 0b01100000; i <= 0b01100111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_str_immediate_offset; 
    }

    for (int i = 0b01101000; i <= 0b01101111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_ldr_immediate_offset;
    }

    for (int i = 0b01110000; i <= 0b01110111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_str_immediate_offset_byte;
    }

    for (int i = 0b01111000; i <= 0b01111111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_ldr_immediate_offset_byte; 
    }

    // Format 10 - load/store halfword
    for (int i = 0b10000000; i <= 0b10000111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_strh_imm;
    }

    for (int i = 0b10001000; i <= 0b10001111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_ldrh_imm; 
    }
    
    // Format 11 - SP-relative load/store
    for (int i = 0b10010000; i <= 0b10010111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_str_sp_rel; 
    }

    for (int i = 0b10011000; i <= 0b10011111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_ldr_sp_rel; 
    }
    
    // Format 12 - load address
    for (int i = 0b10100000; i <= 0b10100111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_ldr_address_pc; 
    }

    for (int i = 0b10101000; i <= 0b10101111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_ldr_address_sp; 
    }

    // Format 13 - add offset to stack pointer
    // Will need further decoding to determine the exact operation
    thumb_instruction_table[0b10110000] = &ThumbCPU::handle_thumb_add_sub_offset_to_stack_pointer;
    
    // Format 14 - push/pop registers
    thumb_instruction_table[0b10110100] = &ThumbCPU::handle_thumb_push_registers;
    thumb_instruction_table[0b10110101] = &ThumbCPU::handle_thumb_push_registers_and_lr;
    thumb_instruction_table[0b10111100] = &ThumbCPU::handle_thumb_pop_registers;
    thumb_instruction_table[0b10111101] = &ThumbCPU::handle_thumb_pop_registers_and_pc;
    
    // Format 15 - multiple load/store
    for (int i = 0b11000000; i <= 0b11000111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_stmia; 
    }

    for (int i = 0b11001000; i <= 0b11001111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_ldmia; 
    }

    // Format 16 - conditional branch
    thumb_instruction_table[0b11010000] = &ThumbCPU::handle_thumb_beq;
    thumb_instruction_table[0b11010001] = &ThumbCPU::handle_thumb_bne;
    thumb_instruction_table[0b11010010] = &ThumbCPU::handle_thumb_bcs;
    thumb_instruction_table[0b11010011] = &ThumbCPU::handle_thumb_bcc;
    thumb_instruction_table[0b11010100] = &ThumbCPU::handle_thumb_bmi;
    thumb_instruction_table[0b11010101] = &ThumbCPU::handle_thumb_bpl;
    thumb_instruction_table[0b11010110] = &ThumbCPU::handle_thumb_bvs;
    thumb_instruction_table[0b11010111] = &ThumbCPU::handle_thumb_bvc;
    thumb_instruction_table[0b11011000] = &ThumbCPU::handle_thumb_bhi;
    thumb_instruction_table[0b11011001] = &ThumbCPU::handle_thumb_bls;
    thumb_instruction_table[0b11011010] = &ThumbCPU::handle_thumb_bge;
    thumb_instruction_table[0b11011011] = &ThumbCPU::handle_thumb_blt;
    thumb_instruction_table[0b11011100] = &ThumbCPU::handle_thumb_bgt;
    thumb_instruction_table[0b11011101] = &ThumbCPU::handle_thumb_ble; 
    thumb_instruction_table[0b11011110] = NULL;
    
    // Format 17 - software interrupt
    thumb_instruction_table[0b11011111] = &ThumbCPU::handle_thumb_swi;

    // Format 18 - unconditional branch
    for (int i = 0b11100000; i <= 0b11100111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_b; // Unconditional branch
    }

    // Format 19 - long branch with link
    // Needs further decoding to determine the exact operation
    for (int i = 0b11110000; i <= 0b11111111; i++) {
        thumb_instruction_table[i] = &ThumbCPU::handle_thumb_bl; // Long branch with link
    }
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
void ThumbCPU::handle_thumb_lsl(uint16_t instruction) {
    uint8_t rd = bits2to0(instruction);
    uint8_t rs = bits5to3(instruction);
    uint8_t shift_amount = bits10to6(instruction);

    // Update C flag before shifting
    parentCPU.updateCFlagShiftLSL(parentCPU.R()[rd], shift_amount);
    parentCPU.R()[rd] = parentCPU.R()[rs] << shift_amount;
    
    parentCPU.updateZFlag(parentCPU.R()[rd]);
    parentCPU.updateNFlag(parentCPU.R()[rd]);
    // No effect on overflow flag

    Debug::log::info("Executing Thumb LSL: R" + std::to_string(rd) + " = R" + std::to_string(rs) + " << " + std::to_string(shift_amount));
}

void ThumbCPU::handle_thumb_lsr(uint16_t instruction) {
    uint8_t rd = bits2to0(instruction);
    uint8_t rs = bits5to3(instruction);
    uint8_t shift_amount = bits10to6(instruction);

    // LSR has some unique handling as a shift of 0 is treated as a shift by 32
    // Update C flag before shifting
    parentCPU.updateCFlagShiftLSR(parentCPU.R()[rs], shift_amount);
    if (shift_amount == 0) {
        // Special case: LSR with shift amount 0 means shift by 32
        parentCPU.R()[rd] = 0; // Result is 0
    } else {
        // Standard LSR behavior
        parentCPU.R()[rd] = parentCPU.R()[rs] >> shift_amount;
    }

    parentCPU.updateZFlag(parentCPU.R()[rd]);
    parentCPU.clearFlag(CPU::FLAG_N); // Clear N flag as LSR always results in a non-negative value
    // No effect on overflow flag

    Debug::log::info("Executing Thumb LSR: R" + std::to_string(rd) + " = R" + std::to_string(rs) + " >> " + std::to_string(shift_amount));
}

void ThumbCPU::handle_thumb_asr(uint16_t instruction) {
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

void ThumbCPU::handle_thumb_add_register(uint16_t instruction) {
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

void ThumbCPU::handle_thumb_add_offset(uint16_t instruction) {
    uint8_t rd = bits2to0(instruction);
    uint8_t rs = bits5to3(instruction);
    uint8_t offset = bits10to6(instruction);

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

void ThumbCPU::handle_thumb_sub_register(uint16_t instruction) {
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

void ThumbCPU::handle_thumb_sub_offset(uint16_t instruction) {
    uint8_t rd = bits2to0(instruction);
    uint8_t rs = bits5to3(instruction);
    uint8_t offset = bits10to6(instruction);
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

void ThumbCPU::handle_thumb_mov_imm(uint16_t instruction) {
    uint8_t rd = bits10to8(instruction);
    uint8_t imm = bits7to0(instruction);

    parentCPU.R()[rd] = imm;
    parentCPU.updateZFlag(parentCPU.R()[rd]); // No negative, carry-out or overflow for MOV
    parentCPU.clearFlag(CPU::FLAG_N); // N flag is always cleared for this instruction as the 8bit immediate is always non-negative

    Debug::log::info("Executing Thumb MOV (immediate): R" + std::to_string(rd) + " = " + std::to_string(imm));
}

void ThumbCPU::handle_thumb_cmp_imm(uint16_t instruction) {
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

void ThumbCPU::handle_thumb_add_imm(uint16_t instruction) {
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

void ThumbCPU::handle_thumb_sub_imm(uint16_t instruction) {
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

void ThumbCPU::handle_thumb_alu_operations(uint16_t instruction) {
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

void ThumbCPU::handle_add_hi(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    // Perform the addition operation
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 + op2;

    // Update the destination register
    parentCPU.R()[rd] = result;

    // Update CPSR flags based on the result
    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    parentCPU.updateCFlagAdd(op1, op2);
    parentCPU.updateVFlag(op1, op2, result);

    Debug::log::info("Executing Thumb ADD (HI register): R" + std::to_string(rd) + " = R" + std::to_string(rd) + " + R" + std::to_string(rs));
}

void ThumbCPU::handle_cmp_hi(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    // Perform the comparison operation
    uint32_t op1 = parentCPU.R()[rd];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 - op2;

    // Update CPSR flags based on the result
    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);
    parentCPU.updateCFlagSub(op1, op2);
    parentCPU.updateVFlagSub(op1, op2, result);

    Debug::log::info("Executing Thumb CMP (HI register): R" + std::to_string(rd) + " - R" + std::to_string(rs));
}

void ThumbCPU::handle_mov_hi(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    // Perform the move operation
    uint32_t result = parentCPU.R()[rs];
    parentCPU.R()[rd] = result;

    parentCPU.updateZFlag(result);
    parentCPU.updateNFlag(result);

    Debug::log::info("Executing Thumb MOV (HI register): R" + std::to_string(rd) + " = R" + std::to_string(rs));
}

void ThumbCPU::handle_bx_hi(uint16_t instruction) {
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    // Perform the branch exchange operation
    parentCPU.R()[15] = parentCPU.R()[rs] & ~1; // Update PC, clearing the least significant bit

    // Update CPSR mode based on the least significant bit of the target address
    if (parentCPU.R()[rs] & 1) {
        parentCPU.CPSR() |= CPU::FLAG_T; // Set Thumb mode
    } else {
        parentCPU.CPSR() &= ~CPU::FLAG_T; // Clear Thumb mode
    }
    
    Debug::log::info("Executing Thumb BX (HI register): Branch to R" + std::to_string(rs) + ", mode: " + ((parentCPU.R()[rs] & 1) ? "Thumb" : "ARM"));
}

void ThumbCPU::handle_thumb_ldr(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[15] + (offset << 2); // PC-relative addressing

    // Perform the load operation
    parentCPU.R()[rd] = parentCPU.getMemory().read32(address);

    Debug::log::info("Executing Thumb LDR: R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::handle_thumb_str_word(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the store operation using memory_write_32
    parentCPU.getMemory().write32(address, parentCPU.R()[rd]);

    Debug::log::info("Executing Thumb STR (word): [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::handle_thumb_ldr_word(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the load operation using memory_read_32
    parentCPU.R()[rd] = parentCPU.getMemory().read32(address);

    Debug::log::info("Executing Thumb LDR (word): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::handle_thumb_ldr_byte(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the load operation using memory_read_8
    parentCPU.R()[rd] = parentCPU.getMemory().read8(address);

    Debug::log::info("Executing Thumb LDR (byte): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::handle_thumb_str_byte(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the store operation using memory_write_8
    parentCPU.getMemory().write8(address, parentCPU.R()[rd] & 0xFF); // Store only the least significant byte

    Debug::log::info("Executing Thumb STR (byte): [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::handle_thumb_strh(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the store operation using memory_write_16
    parentCPU.getMemory().write16(address, parentCPU.R()[rd] & 0xFFFF); // Store only the least significant halfword

    Debug::log::info("Executing Thumb STRH: [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::handle_thumb_ldsb(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the load operation using memory_read_8 and sign-extend
    int8_t value = (int8_t)parentCPU.getMemory().read8(address);
    parentCPU.R()[rd] = (int32_t)value;

    Debug::log::info("Executing Thumb LDSB: R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::handle_thumb_ldrh(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the load operation using memory_read_16
    parentCPU.R()[rd] = parentCPU.getMemory().read16(address);

    Debug::log::info("Executing Thumb LDRH: R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::handle_thumb_ldsh(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + parentCPU.R()[rm];

    // Perform the load operation using memory_read_16 and sign-extend
    int16_t value = (int16_t)parentCPU.getMemory().read16(address);
    parentCPU.R()[rd] = (int32_t)value;

    Debug::log::info("Executing Thumb LDSH: R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::handle_thumb_str_immediate_offset(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t offset = instruction & 0x07; // Immediate offset (bits 0-2)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rn] + (offset << 2); // Offset scaled by 4 for word alignment

    // Perform the store operation using memory_write_32
    parentCPU.getMemory().write32(address, parentCPU.R()[rd]);

    Debug::log::info("Executing Thumb STR (immediate offset): [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::handle_thumb_ldr_immediate_offset(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t offset = instruction & 0x07; // Immediate offset (bits 0-2)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + (offset << 2); // Offset scaled by 4 for word alignment

    // Perform the load operation using memory_read_32
    parentCPU.R()[rd] = parentCPU.getMemory().read32(address);

    Debug::log::info("Executing Thumb LDR (immediate offset): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::handle_thumb_str_immediate_offset_byte(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t offset = instruction & 0x07; // Immediate offset (bits 0-2)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rn] + offset; // Byte offset

    // Perform the store operation using memory_write_8
    parentCPU.getMemory().write8(address, parentCPU.R()[rd] & 0xFF); // Store only the least significant byte

    Debug::log::info("Executing Thumb STR (immediate offset byte): [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::handle_thumb_ldr_immediate_offset_byte(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t offset = instruction & 0x07; // Immediate offset (bits 0-2)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + offset; // Byte offset

    // Perform the load operation using memory_read_8
    parentCPU.R()[rd] = parentCPU.getMemory().read8(address);

    Debug::log::info("Executing Thumb LDR (immediate offset byte): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::handle_thumb_strh_imm(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t offset = instruction & 0x07; // Immediate offset (bits 0-2)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[rn] + (offset << 1); // Offset scaled by 2 for halfword alignment

    // Perform the store operation using memory_write_16
    parentCPU.getMemory().write16(address, parentCPU.R()[rd] & 0xFFFF); // Store only the least significant halfword

    Debug::log::info("Executing Thumb STRH (immediate offset): [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::handle_thumb_ldrh_imm(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t offset = instruction & 0x07; // Immediate offset (bits 0-2)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[rn] + (offset << 1); // Offset scaled by 2 for halfword alignment

    // Perform the load operation using memory_read_16
    parentCPU.R()[rd] = parentCPU.getMemory().read16(address);

    Debug::log::info("Executing Thumb LDRH (immediate offset): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::handle_thumb_str_sp_rel(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to store to
    uint32_t address = parentCPU.R()[13] + (offset << 2); // SP-relative addressing with word alignment

    // Perform the store operation using memory_write_32
    parentCPU.getMemory().write32(address, parentCPU.R()[rd]);

    Debug::log::info("Executing Thumb STR (SP-relative): [0x" + std::to_string(address) + "] = R" + std::to_string(rd));
}

void ThumbCPU::handle_thumb_ldr_sp_rel(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[13] + (offset << 2); // SP-relative addressing with word alignment

    // Perform the load operation using memory_read_32
    parentCPU.R()[rd] = parentCPU.getMemory().read32(address);

    Debug::log::info("Executing Thumb LDR (SP-relative): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::handle_thumb_ldr_address_pc(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to load from
    uint32_t address = (parentCPU.R()[15] & ~0x3) + (offset << 2); // PC-relative addressing with word alignment

    // Perform the load operation using memory_read_32
    parentCPU.R()[rd] = parentCPU.getMemory().read32(address);

    Debug::log::info("Executing Thumb LDR (PC-relative): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::handle_thumb_ldr_address_sp(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to load from
    uint32_t address = parentCPU.R()[13] + (offset << 2); // SP-relative addressing with word alignment

    // Perform the load operation using memory_read_32
    parentCPU.R()[rd] = parentCPU.getMemory().read32(address);

    Debug::log::info("Executing Thumb LDR (SP-relative): R" + std::to_string(rd) + " = [0x" + std::to_string(address) + "]");
}

void ThumbCPU::handle_thumb_add_sub_offset_to_stack_pointer(uint16_t instruction) {
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

void ThumbCPU::handle_thumb_push_registers(uint16_t instruction) {
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // Push registers onto the stack
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            parentCPU.R()[13] -= 4; // Decrement SP by 4
            parentCPU.getMemory().write32(parentCPU.R()[13], parentCPU.R()[i]); // Write register to memory
            Debug::log::info("Pushing R" + std::to_string(i) + " onto stack: [0x" + std::to_string(parentCPU.R()[13]) + "] = R" + std::to_string(i));
        }
    }
}

void ThumbCPU::handle_thumb_push_registers_and_lr(uint16_t instruction) {
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // Push registers onto the stack
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            parentCPU.R()[13] -= 4; // Decrement SP by 4
            parentCPU.getMemory().write32(parentCPU.R()[13], parentCPU.R()[i]); // Write register to memory
            Debug::log::info("Pushing R" + std::to_string(i) + " onto stack: [0x" + std::to_string(parentCPU.R()[13]) + "] = R" + std::to_string(i));
        }
    }

    // Push LR onto the stack
    parentCPU.R()[13] -= 4; // Decrement SP by 4
    parentCPU.getMemory().write32(parentCPU.R()[13], parentCPU.R()[14]); // Write LR to memory
    Debug::log::info("Pushing LR onto stack: [0x" + std::to_string(parentCPU.R()[13]) + "] = LR");
}

void ThumbCPU::handle_thumb_pop_registers(uint16_t instruction) {
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

void ThumbCPU::handle_thumb_pop_registers_and_pc(uint16_t instruction) {
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

void ThumbCPU::handle_thumb_stmia(uint16_t instruction) {
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

void ThumbCPU::handle_thumb_ldmia(uint16_t instruction) {
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

    // Update the base register
    parentCPU.R()[rn] = address;
}

void ThumbCPU::handle_thumb_beq(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (parentCPU.CPSR() & CPU::FLAG_Z) { // Check Zero flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BEQ: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::handle_thumb_bne(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_Z)) { // Check Zero flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BNE: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::handle_thumb_bcs(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (parentCPU.CPSR() & CPU::FLAG_C) { // Check Carry flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BCS: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::handle_thumb_bcc(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_C)) { // Check Carry flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BCC: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::handle_thumb_bmi(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (parentCPU.CPSR() & CPU::FLAG_N) { // Check Negative flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BMI: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::handle_thumb_bpl(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_N)) { // Check Negative flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BPL: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::handle_thumb_bvs(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (parentCPU.CPSR() & CPU::FLAG_V) { // Check Overflow flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BVS: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::handle_thumb_bvc(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_V)) { // Check Overflow flag
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BVC: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::handle_thumb_bhi(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if ((parentCPU.CPSR() & CPU::FLAG_C) && !(parentCPU.CPSR() & CPU::FLAG_Z)) { // Check Carry and Zero flags
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BHI: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::handle_thumb_bls(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_C) || (parentCPU.CPSR() & CPU::FLAG_Z)) { // Check Carry and Zero flags
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BLS: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::handle_thumb_bge(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if ((parentCPU.CPSR() & CPU::FLAG_N) == (parentCPU.CPSR() & CPU::FLAG_V)) { // Check Negative and Overflow flags
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BGE: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::handle_thumb_blt(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if ((parentCPU.CPSR() & CPU::FLAG_N) != (parentCPU.CPSR() & CPU::FLAG_V)) { // Check Negative and Overflow flags
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BLT: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::handle_thumb_bgt(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(parentCPU.CPSR() & CPU::FLAG_Z) && ((parentCPU.CPSR() & CPU::FLAG_N) == (parentCPU.CPSR() & CPU::FLAG_V))) { // Check Zero, Negative, and Overflow flags
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BGT: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::handle_thumb_ble(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if ((parentCPU.CPSR() & CPU::FLAG_Z) || ((parentCPU.CPSR() & CPU::FLAG_N) != (parentCPU.CPSR() & CPU::FLAG_V))) { // Check Zero, Negative, and Overflow flags
        parentCPU.R()[15] += (offset << 1); // Branch to target address
        Debug::log::info("Executing Thumb BLE: Branch to 0x" + std::to_string(parentCPU.R()[15]));
    }
}

void ThumbCPU::handle_thumb_swi(uint16_t instruction) {
    uint8_t comment = instruction & 0xFF; // Software interrupt comment (bits 0-7)

    // Handle the software interrupt
    Debug::log::info("Executing Thumb SWI: Software interrupt with comment 0x" + std::to_string(comment));
    //handle_software_interrupt(comment); // Call the software interrupt handler
}

void ThumbCPU::handle_thumb_b(uint16_t instruction) {
    int16_t offset = instruction & 0x7FF; // Signed 11-bit offset (bits 0-10)
    if (offset & 0x400) { // Sign-extend the offset
        offset |= 0xF800;
    }

    // Perform the branch operation
    parentCPU.R()[15] += (offset << 1); // Branch to target address
    Debug::log::info("Executing Thumb B: Branch to 0x" + std::to_string(parentCPU.R()[15]));
}

void ThumbCPU::handle_thumb_bl(uint16_t instruction) {
    int16_t offset = instruction & 0x7FF; // Signed 11-bit offset (bits 0-10)
    if (offset & 0x400) { // Sign-extend the offset
        offset |= 0xF800;
    }

    // Perform the branch with link operation
    parentCPU.R()[14] = parentCPU.R()[15] + 2; // Save return address in LR
    parentCPU.R()[15] += (offset << 1); // Branch to target address
    Debug::log::info("Executing Thumb BL: Branch to 0x" + std::to_string(parentCPU.R()[15]) + " with link");
}