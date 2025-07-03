#include "cpu.h"
#include "thumb.h"
#include "debug.h" // Include debug utilities
#include <stdio.h>
#include <stdlib.h>

// Static array of function pointers for Thumb instruction handlers
static void (*thumb_instruction_table[256])(uint16_t);

// Define a static array of function pointers for ALU operations
static void (*thumb_alu_operations_table[16])(uint8_t rd, uint8_t rs);

// Function to initialize the Thumb instruction table
void thumb_init_table() {
    LOG_INFO("Initializing Thumb instruction table");
    // Initialize the table with NULL
    for (int i = 0; i < 256; i++) {
        thumb_instruction_table[i] = NULL;
    }

    // Format 1 - move shifted register
    for (int i = 0b00000000; i <= 0b00000111; i++) {
        thumb_instruction_table[i] = handle_thumb_lsl;
    }
    
    for (int i = 0b00001000; i <= 0b00001111; i++) {
        thumb_instruction_table[i] = handle_thumb_lsr;
    }
        
    for (int i = 0b00010000; i <= 0b00010111; i++) {
        thumb_instruction_table[i] = handle_thumb_asr;
    }

    // Format 2 - add/subtract
    thumb_instruction_table[0b00011000] = handle_thumb_add_register; 
    thumb_instruction_table[0b00011001] = handle_thumb_add_register; 
     
    thumb_instruction_table[0b00011010] = handle_thumb_add_offset; 
    thumb_instruction_table[0b00011011] = handle_thumb_add_offset; 
    
    thumb_instruction_table[0b00011100] = handle_thumb_sub_register;
    thumb_instruction_table[0b00011101] = handle_thumb_sub_register;
    
    thumb_instruction_table[0b00011110] = handle_thumb_sub_offset;    
    thumb_instruction_table[0b00011111] = handle_thumb_sub_offset;    

    // Format 3 - move/compare/add/subtract immediate
    for (int i = 0b00100000; i <= 0b00100111; i++) {
        thumb_instruction_table[i] = handle_thumb_mov_imm;
    }

    for (int i = 0b00101000; i <= 0b00101111; i++) {
        thumb_instruction_table[i] = handle_thumb_cmp_imm;
    }
    
    for (int i = 0b00110000; i <= 0b00110111; i++) {
        thumb_instruction_table[i] = handle_thumb_add_imm;
    }

    for (int i = 0b00111000; i <= 0b00111111; i++) {
        thumb_instruction_table[i] = handle_thumb_sub_imm;
    }
        
    // Format 4 - ALU operations
    // This will have to be decoded further based on the specific operation
    for (int i = 0b01000000; i <= 0b01000011; i++) {
        thumb_instruction_table[i] = handle_thumb_alu_operations; // will have to be decoded further
    }

    // Format 5 - HI register operations/branch exchange
    // These will have to be decoded further based on the specific operation
    thumb_instruction_table[0b01000100] = handle_add_hi;
    thumb_instruction_table[0b01000101] = handle_cmp_hi;
    thumb_instruction_table[0b01000110] = handle_mov_hi;
    thumb_instruction_table[0b01000111] = handle_bx_hi;
  
    // Format 6 - PC relative load
    for (int i = 0b01001000; i <= 0b01001111; i++) {
        thumb_instruction_table[i] = handle_thumb_ldr; 
    }

    // Format 7 - load/store with register offset
    thumb_instruction_table[0b01010000] = handle_thumb_str_word; 
    thumb_instruction_table[0b01010001] = handle_thumb_str_word; 
    thumb_instruction_table[0b01010100] = handle_thumb_str_byte; 
    thumb_instruction_table[0b01010101] = handle_thumb_str_byte; 
    thumb_instruction_table[0b01011000] = handle_thumb_ldr_word; 
    thumb_instruction_table[0b01011001] = handle_thumb_ldr_word; 
    thumb_instruction_table[0b01011100] = handle_thumb_ldr_byte; 
    thumb_instruction_table[0b01011101] = handle_thumb_ldr_byte; 

    // Format 8 - load/store sign-extended byte/halfword
    thumb_instruction_table[0b01010010] = handle_thumb_strh; 
    thumb_instruction_table[0b01010011] = handle_thumb_strh; 
    thumb_instruction_table[0b01010110] = handle_thumb_ldsb; 
    thumb_instruction_table[0b01010111] = handle_thumb_ldsb; 
    thumb_instruction_table[0b01011010] = handle_thumb_ldrh; 
    thumb_instruction_table[0b01011011] = handle_thumb_ldrh; 
    thumb_instruction_table[0b01011110] = handle_thumb_ldsh; 
    thumb_instruction_table[0b01011111] = handle_thumb_ldsh; 

    // Format 9 - load/store with immediate offset
    for (int i = 0b01100000; i <= 0b01100111; i++) {
        thumb_instruction_table[i] = handle_thumb_str_immediate_offset; 
    }

    for (int i = 0b01101000; i <= 0b01101111; i++) {
        thumb_instruction_table[i] = handle_thumb_ldr_immediate_offset;
    }

    for (int i = 0b01110000; i <= 0b01110111; i++) {
        thumb_instruction_table[i] = handle_thumb_str_immediate_offset_byte;
    }

    for (int i = 0b01111000; i <= 0b01111111; i++) {
        thumb_instruction_table[i] = handle_thumb_ldr_immediate_offset_byte; 
    }

    // Format 10 - load/store halfword
    for (int i = 0b10000000; i <= 0b10000111; i++) {
        thumb_instruction_table[i] = handle_thumb_strh_imm;
    }

    for (int i = 0b10001000; i <= 0b10001111; i++) {
        thumb_instruction_table[i] = handle_thumb_ldrh_imm; 
    }
    
    // Format 11 - SP-relative load/store
    for (int i = 0b10010000; i <= 0b10010111; i++) {
        thumb_instruction_table[i] = handle_thumb_str_sp_rel; 
    }

    for (int i = 0b10011000; i <= 0b10011111; i++) {
        thumb_instruction_table[i] = handle_thumb_ldr_sp_rel; 
    }
    
    // Format 12 - load address
    for (int i = 0b10100000; i <= 0b10100111; i++) {
        thumb_instruction_table[i] = handle_thumb_ldr_address_pc; 
    }

    for (int i = 0b10101000; i <= 0b10101111; i++) {
        thumb_instruction_table[i] = handle_thumb_ldr_address_sp; 
    }

    // Format 13 - add offset to stack pointer
    // Will need further decoding to determine the exact operation
    thumb_instruction_table[0b10110000] = handle_thumb_add_sub_offset_to_stack_pointer;
    
    // Format 14 - push/pop registers
    thumb_instruction_table[0b10110100] = handle_thumb_push_registers;
    thumb_instruction_table[0b10110101] = handle_thumb_push_registers_and_lr;
    thumb_instruction_table[0b10111100] = handle_thumb_pop_registers;
    thumb_instruction_table[0b10111101] = handle_thumb_pop_registers_and_pc;
    
    // Format 15 - multiple load/store
    for (int i = 0b11000000; i <= 0b11000111; i++) {
        thumb_instruction_table[i] = handle_thumb_stmia; 
    }

    for (int i = 0b11001000; i <= 0b11001111; i++) {
        thumb_instruction_table[i] = handle_thumb_ldmia; 
    }

    // Format 16 - conditional branch
    thumb_instruction_table[0b11010000] = handle_thumb_beq;
    thumb_instruction_table[0b11010001] = handle_thumb_bne;
    thumb_instruction_table[0b11010010] = handle_thumb_bcs;
    thumb_instruction_table[0b11010011] = handle_thumb_bcc;
    thumb_instruction_table[0b11010100] = handle_thumb_bmi;
    thumb_instruction_table[0b11010101] = handle_thumb_bpl;
    thumb_instruction_table[0b11010110] = handle_thumb_bvs;
    thumb_instruction_table[0b11010111] = handle_thumb_bvc;
    thumb_instruction_table[0b11011000] = handle_thumb_bhi;
    thumb_instruction_table[0b11011001] = handle_thumb_bls;
    thumb_instruction_table[0b11011010] = handle_thumb_bge;
    thumb_instruction_table[0b11011011] = handle_thumb_blt;
    thumb_instruction_table[0b11011100] = handle_thumb_bgt;
    thumb_instruction_table[0b11011101] = handle_thumb_ble; 
    thumb_instruction_table[0b11011110] = NULL;
    
    // Format 17 - software interrupt
    thumb_instruction_table[0b11011111] = handle_thumb_swi;

    // Format 18 - unconditional branch
    for (int i = 0b11100000; i <= 0b11100111; i++) {
        thumb_instruction_table[i] = handle_thumb_b; // Unconditional branch
    }

    // Format 19 - long branch with link
    // Needs further decoding to determine the exact operation
    for (int i = 0b11110000; i <= 0b11111111; i++) {
        thumb_instruction_table[i] = handle_thumb_bl; // Long branch with link
    }

    // Initialize the ALU operations table
    thumb_init_alu_operations_table();
}

// Function to decode and execute a Thumb instruction
void thumb_decode_and_execute(uint16_t instruction) {
    uint8_t opcode = instruction & 0x3F; // Extract the 6-bit opcode

    if (thumb_instruction_table[opcode] != NULL) {
        thumb_instruction_table[opcode](instruction); // Execute the instruction
    } else {
        LOG_INFO("Undefined Thumb instruction: 0x%04X", instruction);
    }
}


// Thumb instruction handlers

// Stub handlers for undefined Thumb instruction functions
static void handle_thumb_lsl(uint16_t instruction) {
    // Extract destination register and source register
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)
    uint8_t shift_amount = (instruction >> 6) & 0x1F; // Shift amount (bits 6-10)

    // Perform the logical shift left operation
    uint32_t result = cpu.r[rs] << shift_amount;

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (cpu.r[rs] >> (32 - shift_amount)) & 1); // Carry out is the last shifted-out bit

    LOG_INFO("Executing Thumb LSL: R%d = R%d << %d", rd, rs, shift_amount);
}

static void handle_thumb_lsr(uint16_t instruction) {
    // Extract destination register and source register
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)
    uint8_t shift_amount = (instruction >> 6) & 0x1F; // Shift amount (bits 6-10)

    // Perform the logical shift right operation
    uint32_t result = cpu.r[rs] >> shift_amount;

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (cpu.r[rs] >> (shift_amount - 1)) & 1); // Carry out is the last shifted-out bit

    LOG_INFO("Executing Thumb LSR: R%d = R%d >> %d", rd, rs, shift_amount);
}

static void handle_thumb_asr(uint16_t instruction) {
    // Extract destination register and source register
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)
    uint8_t shift_amount = (instruction >> 6) & 0x1F; // Shift amount (bits 6-10)

    // Perform the arithmetic shift right operation
    uint32_t result;
    if (shift_amount == 0) {
        // Special case: if shift_amount is 0, the result is all bits of the source register set to its sign bit
        result = (cpu.r[rs] & 0x80000000) ? 0xFFFFFFFF : 0;
    } else {
        result = (int32_t)cpu.r[rs] >> shift_amount;
    }

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (cpu.r[rs] >> (shift_amount - 1)) & 1); // Carry out is the last shifted-out bit

    LOG_INFO("Executing Thumb ASR: R%d = R%d >> %d", rd, rs, shift_amount);
}

static void handle_thumb_add_register(uint16_t instruction) {
    // Extract destination register and source registers
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // First source register (bits 3-5)
    uint8_t rn = (instruction >> 6) & 0x07; // Second source register (bits 6-8)

    // Perform the addition operation
    uint32_t result = cpu.r[rs] + cpu.r[rn];

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (cpu.r[rs] > (UINT32_MAX - cpu.r[rn]))); // Carry out is set if overflow occurs

    LOG_INFO("Executing Thumb ADD (register): R%d = R%d + R%d", rd, rs, rn);
}

static void handle_thumb_add_offset(uint16_t instruction) {
    // Extract destination register and source register
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)
    uint8_t offset = (instruction >> 6) & 0x1F; // Immediate offset (bits 6-10)

    // Perform the addition operation
    uint32_t result = cpu.r[rs] + offset;

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (cpu.r[rs] > (UINT32_MAX - offset))); // Carry out is set if overflow occurs

    LOG_INFO("Executing Thumb ADD (offset): R%d = R%d + %d", rd, rs, offset);
}

static void handle_thumb_sub_register(uint16_t instruction) {
    // Extract destination register and source registers
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // First source register (bits 3-5)
    uint8_t rn = (instruction >> 6) & 0x07; // Second source register (bits 6-8)

    // Perform the subtraction operation
    uint32_t result = cpu.r[rs] - cpu.r[rn];

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (cpu.r[rs] < cpu.r[rn])); // Carry out is set if borrow occurs

    LOG_INFO("Executing Thumb SUB (register): R%d = R%d - R%d", rd, rs, rn);
}

static void handle_thumb_sub_offset(uint16_t instruction) {
    // Extract destination register and source register
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)
    uint8_t offset = (instruction >> 6) & 0x1F; // Immediate offset (bits 6-10)

    // Perform the subtraction operation
    uint32_t result = cpu.r[rs] - offset;

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (cpu.r[rs] < offset)); // Carry out is set if borrow occurs

    LOG_INFO("Executing Thumb SUB (offset): R%d = R%d - %d", rd, rs, offset);
}

static void handle_thumb_mov_imm(uint16_t instruction) {
    // Extract destination register and immediate value
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t imm = (instruction >> 6) & 0xFF; // Immediate value (bits 6-13)

    // Perform the move operation
    cpu.r[rd] = imm;

    // Update CPSR flags based on the result
    update_cpsr_flags(cpu.r[rd], 0); // No carry out for move operation

    LOG_INFO("Executing Thumb MOV (immediate): R%d = %d", rd, imm);
}

static void handle_thumb_cmp_imm(uint16_t instruction) {
    // Extract source register and immediate value
    uint8_t rs = instruction & 0x07; // Source register (bits 0-2)
    uint8_t imm = (instruction >> 6) & 0xFF; // Immediate value (bits 6-13)

    // Perform the comparison operation
    uint32_t result = cpu.r[rs] - imm;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (cpu.r[rs] < imm)); // Carry out is set if borrow occurs

    LOG_INFO("Executing Thumb CMP (immediate): R%d - %d", rs, imm);
}

static void handle_thumb_add_imm(uint16_t instruction) {
    // Extract destination register and immediate value
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t imm = (instruction >> 6) & 0xFF; // Immediate value (bits 6-13)

    // Perform the addition operation
    uint32_t result = cpu.r[rd] + imm;

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (cpu.r[rd] > (UINT32_MAX - imm))); // Carry out is set if overflow occurs

    LOG_INFO("Executing Thumb ADD (immediate): R%d = R%d + %d", rd, rd, imm);
}

static void handle_thumb_sub_imm(uint16_t instruction) {
    // Extract destination register and immediate value
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t imm = (instruction >> 6) & 0xFF; // Immediate value (bits 6-13)

    // Perform the subtraction operation
    uint32_t result = cpu.r[rd] - imm;

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (cpu.r[rd] < imm)); // Carry out is set if borrow occurs

    LOG_INFO("Executing Thumb SUB (immediate): R%d = R%d - %d", rd, rd, imm);
}

static void thumb_init_alu_operations_table() {
    thumb_alu_operations_table[0x0] = thumb_alu_and;
    thumb_alu_operations_table[0x1] = thumb_alu_eor;
    thumb_alu_operations_table[0x2] = thumb_alu_lsl;
    thumb_alu_operations_table[0x3] = thumb_alu_lsr;
    thumb_alu_operations_table[0x4] = thumb_alu_asr;
    thumb_alu_operations_table[0x5] = thumb_alu_adc;
    thumb_alu_operations_table[0x6] = thumb_alu_sbc;
    thumb_alu_operations_table[0x7] = thumb_alu_ror;
    thumb_alu_operations_table[0x8] = thumb_alu_tst;
    thumb_alu_operations_table[0x9] = thumb_alu_neg;
    thumb_alu_operations_table[0xA] = thumb_alu_cmp;
    thumb_alu_operations_table[0xB] = thumb_alu_cmn;
    thumb_alu_operations_table[0xC] = thumb_alu_orr;
    thumb_alu_operations_table[0xD] = thumb_alu_mul;
    thumb_alu_operations_table[0xE] = thumb_alu_bic;
    thumb_alu_operations_table[0xF] = thumb_alu_mvn;
}

static void handle_thumb_alu_operations(uint16_t instruction) {
    uint8_t sub_opcode = (instruction >> 6) & 0x0F; // Sub-opcode (bits 6-9)
    uint8_t rd = instruction & 0x07; // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    if (thumb_alu_operations_table[sub_opcode] != NULL) {
        thumb_alu_operations_table[sub_opcode](rd, rs);
    } else {
        LOG_ERROR("Undefined ALU operation: sub-opcode %d", sub_opcode);
    }
}

// Define individual ALU operation functions
static void thumb_alu_and(uint8_t rd, uint8_t rs) {
    uint32_t result = cpu.r[rd] & cpu.r[rs];
    cpu.r[rd] = result;
    update_cpsr_flags(result, 0); // No carry-out for AND
    LOG_INFO("Executing Thumb AND: R%d = R%d & R%d", rd, rd, rs);
}

static void thumb_alu_eor(uint8_t rd, uint8_t rs) {
    uint32_t result = cpu.r[rd] ^ cpu.r[rs];
    cpu.r[rd] = result;
    update_cpsr_flags(result, 0); // No carry-out for EOR
    LOG_INFO("Executing Thumb EOR: R%d = R%d ^ R%d", rd, rd, rs);
}

static void thumb_alu_lsl(uint8_t rd, uint8_t rs) {
    uint32_t result = cpu.r[rd] << cpu.r[rs];
    cpu.r[rd] = result;
    update_cpsr_flags(result, (cpu.r[rd] >> (32 - cpu.r[rs])) & 1); // Carry-out is the last shifted-out bit
    LOG_INFO("Executing Thumb LSL: R%d = R%d << R%d", rd, rd, rs);
}

static void thumb_alu_lsr(uint8_t rd, uint8_t rs) {
    uint32_t result = cpu.r[rd] >> cpu.r[rs];
    cpu.r[rd] = result;
    update_cpsr_flags(result, (cpu.r[rd] >> (cpu.r[rs] - 1)) & 1); // Carry-out is the last shifted-out bit
    LOG_INFO("Executing Thumb LSR: R%d = R%d >> R%d", rd, rd, rs);
}

static void thumb_alu_asr(uint8_t rd, uint8_t rs) {
    uint32_t result = (int32_t)cpu.r[rd] >> cpu.r[rs];
    cpu.r[rd] = result;
    update_cpsr_flags(result, (cpu.r[rd] >> (cpu.r[rs] - 1)) & 1); // Carry-out is the last shifted-out bit
    LOG_INFO("Executing Thumb ASR: R%d = R%d >> R%d", rd, rd, rs);
}

static void thumb_alu_adc(uint8_t rd, uint8_t rs) {
    uint32_t result = cpu.r[rd] + cpu.r[rs] + (cpu.cpsr & CPSR_C_FLAG ? 1 : 0);
    cpu.r[rd] = result;
    update_cpsr_flags(result, (cpu.r[rd] > (UINT32_MAX - cpu.r[rs]))); // Carry-out is set if overflow occurs
    LOG_INFO("Executing Thumb ADC: R%d = R%d + R%d + Carry", rd, rd, rs);
}

static void thumb_alu_sbc(uint8_t rd, uint8_t rs) {
    uint32_t result = cpu.r[rd] - cpu.r[rs] - (cpu.cpsr & CPSR_C_FLAG ? 0 : 1);
    cpu.r[rd] = result;
    update_cpsr_flags(result, (cpu.r[rd] < cpu.r[rs])); // Carry-out is set if borrow occurs
    LOG_INFO("Executing Thumb SBC: R%d = R%d - R%d - Borrow", rd, rd, rs);
}

static void thumb_alu_ror(uint8_t rd, uint8_t rs) {
    uint32_t result = (cpu.r[rd] >> cpu.r[rs]) | (cpu.r[rd] << (32 - cpu.r[rs]));
    cpu.r[rd] = result;
    update_cpsr_flags(result, (cpu.r[rd] >> (cpu.r[rs] - 1)) & 1); // Carry-out is the last shifted-out bit
    LOG_INFO("Executing Thumb ROR: R%d = R%d ROR R%d", rd, rd, rs);
}

static void thumb_alu_tst(uint8_t rd, uint8_t rs) {
    uint32_t result = cpu.r[rd] & cpu.r[rs];
    update_cpsr_flags(result, 0); // No carry-out for TST
    LOG_INFO("Executing Thumb TST: R%d & R%d", rd, rs);
}

static void thumb_alu_neg(uint8_t rd, uint8_t rs) {
    uint32_t result = -cpu.r[rs];
    cpu.r[rd] = result;
    update_cpsr_flags(result, 0); // No carry-out for NEG
    LOG_INFO("Executing Thumb NEG: R%d = -R%d", rd, rs);
}

static void thumb_alu_cmp(uint8_t rd, uint8_t rs) {
    uint32_t result = cpu.r[rd] - cpu.r[rs];
    update_cpsr_flags(result, (cpu.r[rd] < cpu.r[rs])); // Carry-out is set if borrow occurs
    LOG_INFO("Executing Thumb CMP: R%d - R%d", rd, rs);
}

static void thumb_alu_cmn(uint8_t rd, uint8_t rs) {
    uint32_t result = cpu.r[rd] + cpu.r[rs];
    update_cpsr_flags(result, (cpu.r[rd] > (UINT32_MAX - cpu.r[rs]))); // Carry-out is set if overflow occurs
    LOG_INFO("Executing Thumb CMN: R%d + R%d", rd, rs);
}

static void thumb_alu_orr(uint8_t rd, uint8_t rs) {
    uint32_t result = cpu.r[rd] | cpu.r[rs];
    cpu.r[rd] = result;
    update_cpsr_flags(result, 0); // No carry-out for ORR
    LOG_INFO("Executing Thumb ORR: R%d = R%d | R%d", rd, rd, rs);
}

static void thumb_alu_mul(uint8_t rd, uint8_t rs) {
    uint32_t result = cpu.r[rd] * cpu.r[rs];
    cpu.r[rd] = result;
    update_cpsr_flags(result, 0); // No carry-out for MUL
    LOG_INFO("Executing Thumb MUL: R%d = R%d * R%d", rd, rd, rs);
}

static void thumb_alu_bic(uint8_t rd, uint8_t rs) {
    uint32_t result = cpu.r[rd] & ~cpu.r[rs];
    cpu.r[rd] = result;
    update_cpsr_flags(result, 0); // No carry-out for BIC
    LOG_INFO("Executing Thumb BIC: R%d = R%d & ~R%d", rd, rd, rs);
}

static void thumb_alu_mvn(uint8_t rd, uint8_t rs) {
    uint32_t result = ~cpu.r[rs];
    cpu.r[rd] = result;
    update_cpsr_flags(result, 0); // No carry-out for MVN
    LOG_INFO("Executing Thumb MVN: R%d = ~R%d", rd, rs);
}

static void handle_add_hi(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    // Perform the addition operation
    uint32_t result = cpu.r[rd] + cpu.r[rs];

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (cpu.r[rd] > (UINT32_MAX - cpu.r[rs])));

    LOG_INFO("Executing Thumb ADD (HI register): R%d = R%d + R%d", rd, rd, rs);
}

static void handle_cmp_hi(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    // Perform the comparison operation
    uint32_t result = cpu.r[rd] - cpu.r[rs];

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (cpu.r[rd] < cpu.r[rs]));

    LOG_INFO("Executing Thumb CMP (HI register): R%d - R%d", rd, rs);
}

static void handle_mov_hi(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    // Perform the move operation
    cpu.r[rd] = cpu.r[rs];

    LOG_INFO("Executing Thumb MOV (HI register): R%d = R%d", rd, rs);
}

static void handle_bx_hi(uint16_t instruction) {
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    // Perform the branch exchange operation
    cpu.r[15] = cpu.r[rs] & ~1; // Update PC, clearing the least significant bit

    // Update CPSR mode based on the least significant bit of the target address
    if (cpu.r[rs] & 1) {
        cpu.cpsr |= CPSR_T_FLAG; // Set Thumb mode
        set_cpu_mode(&cpu, THUMB_MODE);
    } else {
        cpu.cpsr &= ~CPSR_T_FLAG; // Clear Thumb mode
        set_cpu_mode(&cpu, ARM_MODE);
    }
    
    LOG_INFO("Executing Thumb BX (HI register): Branch to R%d, mode: %s", rs, (cpu.r[rs] & 1) ? "Thumb" : "ARM");
}

static void handle_thumb_ldr(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to load from
    uint32_t address = cpu.r[15] + (offset << 2); // PC-relative addressing

    // Perform the load operation using memory_read_32
    cpu.r[rd] = memory_read_32(address);

    LOG_INFO("Executing Thumb LDR: R%d = [0x%08X]", rd, address);
}

static void handle_thumb_str_word(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to store to
    uint32_t address = cpu.r[rn] + cpu.r[rm];

    // Perform the store operation using memory_write_32
    memory_write_32(address, cpu.r[rd]);

    LOG_INFO("Executing Thumb STR (word): [0x%08X] = R%d", address, rd);
}

static void handle_thumb_ldr_word(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to load from
    uint32_t address = cpu.r[rn] + cpu.r[rm];

    // Perform the load operation using memory_read_32
    cpu.r[rd] = memory_read_32(address);

    LOG_INFO("Executing Thumb LDR (word): R%d = [0x%08X]", rd, address);
}

static void handle_thumb_ldr_byte(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to load from
    uint32_t address = cpu.r[rn] + cpu.r[rm];

    // Perform the load operation using memory_read_8
    cpu.r[rd] = memory_read_8(address);

    LOG_INFO("Executing Thumb LDR (byte): R%d = [0x%08X]", rd, address);
}

static void handle_thumb_str_byte(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to store to
    uint32_t address = cpu.r[rn] + cpu.r[rm];

    // Perform the store operation using memory_write_8
    memory_write_8(address, cpu.r[rd] & 0xFF); // Store only the least significant byte

    LOG_INFO("Executing Thumb STR (byte): [0x%08X] = R%d", address, rd);
}

static void handle_thumb_strh(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to store to
    uint32_t address = cpu.r[rn] + cpu.r[rm];

    // Perform the store operation using memory_write_16
    memory_write_16(address, cpu.r[rd] & 0xFFFF); // Store only the least significant halfword

    LOG_INFO("Executing Thumb STRH: [0x%08X] = R%d", address, rd);
}

static void handle_thumb_ldsb(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to load from
    uint32_t address = cpu.r[rn] + cpu.r[rm];

    // Perform the load operation using memory_read_8 and sign-extend
    int8_t value = (int8_t)memory_read_8(address);
    cpu.r[rd] = (int32_t)value;

    LOG_INFO("Executing Thumb LDSB: R%d = [0x%08X]", rd, address);
}

static void handle_thumb_ldrh(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to load from
    uint32_t address = cpu.r[rn] + cpu.r[rm];

    // Perform the load operation using memory_read_16
    cpu.r[rd] = memory_read_16(address);

    LOG_INFO("Executing Thumb LDRH: R%d = [0x%08X]", rd, address);
}

static void handle_thumb_ldsh(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t rm = instruction & 0x07; // Offset register (bits 0-2)

    // Calculate the address to load from
    uint32_t address = cpu.r[rn] + cpu.r[rm];

    // Perform the load operation using memory_read_16 and sign-extend
    int16_t value = (int16_t)memory_read_16(address);
    cpu.r[rd] = (int32_t)value;

    LOG_INFO("Executing Thumb LDSH: R%d = [0x%08X]", rd, address);
}

static void handle_thumb_str_immediate_offset(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t offset = instruction & 0x07; // Immediate offset (bits 0-2)

    // Calculate the address to store to
    uint32_t address = cpu.r[rn] + (offset << 2); // Offset scaled by 4 for word alignment

    // Perform the store operation using memory_write_32
    memory_write_32(address, cpu.r[rd]);

    LOG_INFO("Executing Thumb STR (immediate offset): [0x%08X] = R%d", address, rd);
}

static void handle_thumb_ldr_immediate_offset(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t offset = instruction & 0x07; // Immediate offset (bits 0-2)

    // Calculate the address to load from
    uint32_t address = cpu.r[rn] + (offset << 2); // Offset scaled by 4 for word alignment

    // Perform the load operation using memory_read_32
    cpu.r[rd] = memory_read_32(address);

    LOG_INFO("Executing Thumb LDR (immediate offset): R%d = [0x%08X]", rd, address);
}

static void handle_thumb_str_immediate_offset_byte(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t offset = instruction & 0x07; // Immediate offset (bits 0-2)

    // Calculate the address to store to
    uint32_t address = cpu.r[rn] + offset; // Byte offset

    // Perform the store operation using memory_write_8
    memory_write_8(address, cpu.r[rd] & 0xFF); // Store only the least significant byte

    LOG_INFO("Executing Thumb STR (immediate offset byte): [0x%08X] = R%d", address, rd);
}

static void handle_thumb_ldr_immediate_offset_byte(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t offset = instruction & 0x07; // Immediate offset (bits 0-2)

    // Calculate the address to load from
    uint32_t address = cpu.r[rn] + offset; // Byte offset

    // Perform the load operation using memory_read_8
    cpu.r[rd] = memory_read_8(address);

    LOG_INFO("Executing Thumb LDR (immediate offset byte): R%d = [0x%08X]", rd, address);
}

static void handle_thumb_strh_imm(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t offset = instruction & 0x07; // Immediate offset (bits 0-2)

    // Calculate the address to store to
    uint32_t address = cpu.r[rn] + (offset << 1); // Offset scaled by 2 for halfword alignment

    // Perform the store operation using memory_write_16
    memory_write_16(address, cpu.r[rd] & 0xFFFF); // Store only the least significant halfword

    LOG_INFO("Executing Thumb STRH (immediate offset): [0x%08X] = R%d", address, rd);
}

static void handle_thumb_ldrh_imm(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint8_t rn = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint8_t offset = instruction & 0x07; // Immediate offset (bits 0-2)

    // Calculate the address to load from
    uint32_t address = cpu.r[rn] + (offset << 1); // Offset scaled by 2 for halfword alignment

    // Perform the load operation using memory_read_16
    cpu.r[rd] = memory_read_16(address);

    LOG_INFO("Executing Thumb LDRH (immediate offset): R%d = [0x%08X]", rd, address);
}

static void handle_thumb_str_sp_rel(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Source register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to store to
    uint32_t address = cpu.r[13] + (offset << 2); // SP-relative addressing with word alignment

    // Perform the store operation using memory_write_32
    memory_write_32(address, cpu.r[rd]);

    LOG_INFO("Executing Thumb STR (SP-relative): [0x%08X] = R%d", address, rd);
}

static void handle_thumb_ldr_sp_rel(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to load from
    uint32_t address = cpu.r[13] + (offset << 2); // SP-relative addressing with word alignment

    // Perform the load operation using memory_read_32
    cpu.r[rd] = memory_read_32(address);

    LOG_INFO("Executing Thumb LDR (SP-relative): R%d = [0x%08X]", rd, address);
}

static void handle_thumb_ldr_address_pc(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to load from
    uint32_t address = (cpu.r[15] & ~0x3) + (offset << 2); // PC-relative addressing with word alignment

    // Perform the load operation using memory_read_32
    cpu.r[rd] = memory_read_32(address);

    LOG_INFO("Executing Thumb LDR (PC-relative): R%d = [0x%08X]", rd, address);
}

static void handle_thumb_ldr_address_sp(uint16_t instruction) {
    uint8_t rd = (instruction >> 8) & 0x07; // Destination register (bits 8-10)
    uint16_t offset = instruction & 0xFF; // Immediate offset (bits 0-7)

    // Calculate the address to load from
    uint32_t address = cpu.r[13] + (offset << 2); // SP-relative addressing with word alignment

    // Perform the load operation using memory_read_32
    cpu.r[rd] = memory_read_32(address);

    LOG_INFO("Executing Thumb LDR (SP-relative): R%d = [0x%08X]", rd, address);
}

static void handle_thumb_add_sub_offset_to_stack_pointer(uint16_t instruction) {
    uint8_t sign = (instruction >> 7) & 0x01; // Sign bit (bit 7)
    uint16_t offset = instruction & 0x7F; // Immediate offset (bits 0-6)

    // Perform the addition or subtraction operation
    if (sign == 0) {
        cpu.r[13] += (offset << 2); // Add offset scaled by 4
        LOG_INFO("Executing Thumb ADD offset to SP: SP = SP + %d", offset << 2);
    } else {
        cpu.r[13] -= (offset << 2); // Subtract offset scaled by 4
        LOG_INFO("Executing Thumb SUB offset from SP: SP = SP - %d", offset << 2);
    }
}

static void handle_thumb_push_registers(uint16_t instruction) {
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // Push registers onto the stack
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            cpu.r[13] -= 4; // Decrement SP by 4
            memory_write_32(cpu.r[13], cpu.r[i]); // Write register to memory
            LOG_INFO("Pushing R%d onto stack: [0x%08X] = R%d", i, cpu.r[13], i);
        }
    }
}

static void handle_thumb_push_registers_and_lr(uint16_t instruction) {
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // Push registers onto the stack
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            cpu.r[13] -= 4; // Decrement SP by 4
            memory_write_32(cpu.r[13], cpu.r[i]); // Write register to memory
            LOG_INFO("Pushing R%d onto stack: [0x%08X] = R%d", i, cpu.r[13], i);
        }
    }

    // Push LR onto the stack
    cpu.r[13] -= 4; // Decrement SP by 4
    memory_write_32(cpu.r[13], cpu.r[14]); // Write LR to memory
    LOG_INFO("Pushing LR onto stack: [0x%08X] = LR", cpu.r[13]);
}

static void handle_thumb_pop_registers(uint16_t instruction) {
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // Pop registers from the stack
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            cpu.r[i] = memory_read_32(cpu.r[13]); // Read register from memory
            LOG_INFO("Popping R%d from stack: R%d = [0x%08X]", i, i, cpu.r[13]);
            cpu.r[13] += 4; // Increment SP by 4
        }
    }
}

static void handle_thumb_pop_registers_and_pc(uint16_t instruction) {
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // Pop registers from the stack
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            cpu.r[i] = memory_read_32(cpu.r[13]); // Read register from memory
            LOG_INFO("Popping R%d from stack: R%d = [0x%08X]", i, i, cpu.r[13]);
            cpu.r[13] += 4; // Increment SP by 4
        }
    }

    // Pop PC from the stack
    cpu.r[15] = memory_read_32(cpu.r[13]); // Read PC from memory
    LOG_INFO("Popping PC from stack: PC = [0x%08X]", cpu.r[13]);
    cpu.r[13] += 4; // Increment SP by 4
}

static void handle_thumb_stmia(uint16_t instruction) {
    uint8_t rn = (instruction >> 8) & 0x07; // Base register (bits 8-10)
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // Store multiple registers to memory
    uint32_t address = cpu.r[rn];
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            memory_write_32(address, cpu.r[i]); // Write register to memory
            LOG_INFO("Storing R%d to [0x%08X]", i, address);
            address += 4; // Increment address by 4
        }
    }

    // Update the base register
    cpu.r[rn] = address;
}

static void handle_thumb_ldmia(uint16_t instruction) {
    uint8_t rn = (instruction >> 8) & 0x07; // Base register (bits 8-10)
    uint16_t register_list = instruction & 0xFF; // Register list (bits 0-7)

    // Load multiple registers from memory
    uint32_t address = cpu.r[rn];
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            cpu.r[i] = memory_read_32(address); // Read register from memory
            LOG_INFO("Loading R%d from [0x%08X]", i, address);
            address += 4; // Increment address by 4
        }
    }

    // Update the base register
    cpu.r[rn] = address;
}

static void handle_thumb_beq(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (cpu.cpsr & CPSR_Z_FLAG) { // Check Zero flag
        cpu.r[15] += (offset << 1); // Branch to target address
        LOG_INFO("Executing Thumb BEQ: Branch to 0x%08X", cpu.r[15]);
    }
}

static void handle_thumb_bne(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(cpu.cpsr & CPSR_Z_FLAG)) { // Check Zero flag
        cpu.r[15] += (offset << 1); // Branch to target address
        LOG_INFO("Executing Thumb BNE: Branch to 0x%08X", cpu.r[15]);
    }
}

static void handle_thumb_bcs(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (cpu.cpsr & CPSR_C_FLAG) { // Check Carry flag
        cpu.r[15] += (offset << 1); // Branch to target address
        LOG_INFO("Executing Thumb BCS: Branch to 0x%08X", cpu.r[15]);
    }
}

static void handle_thumb_bcc(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(cpu.cpsr & CPSR_C_FLAG)) { // Check Carry flag
        cpu.r[15] += (offset << 1); // Branch to target address
        LOG_INFO("Executing Thumb BCC: Branch to 0x%08X", cpu.r[15]);
    }
}

static void handle_thumb_bmi(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (cpu.cpsr & CPSR_N_FLAG) { // Check Negative flag
        cpu.r[15] += (offset << 1); // Branch to target address
        LOG_INFO("Executing Thumb BMI: Branch to 0x%08X", cpu.r[15]);
    }
}

static void handle_thumb_bpl(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(cpu.cpsr & CPSR_N_FLAG)) { // Check Negative flag
        cpu.r[15] += (offset << 1); // Branch to target address
        LOG_INFO("Executing Thumb BPL: Branch to 0x%08X", cpu.r[15]);
    }
}

static void handle_thumb_bvs(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (cpu.cpsr & CPSR_V_FLAG) { // Check Overflow flag
        cpu.r[15] += (offset << 1); // Branch to target address
        LOG_INFO("Executing Thumb BVS: Branch to 0x%08X", cpu.r[15]);
    }
}

static void handle_thumb_bvc(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(cpu.cpsr & CPSR_V_FLAG)) { // Check Overflow flag
        cpu.r[15] += (offset << 1); // Branch to target address
        LOG_INFO("Executing Thumb BVC: Branch to 0x%08X", cpu.r[15]);
    }
}

static void handle_thumb_bhi(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if ((cpu.cpsr & CPSR_C_FLAG) && !(cpu.cpsr & CPSR_Z_FLAG)) { // Check Carry and Zero flags
        cpu.r[15] += (offset << 1); // Branch to target address
        LOG_INFO("Executing Thumb BHI: Branch to 0x%08X", cpu.r[15]);
    }
}

static void handle_thumb_bls(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(cpu.cpsr & CPSR_C_FLAG) || (cpu.cpsr & CPSR_Z_FLAG)) { // Check Carry and Zero flags
        cpu.r[15] += (offset << 1); // Branch to target address
        LOG_INFO("Executing Thumb BLS: Branch to 0x%08X", cpu.r[15]);
    }
}

static void handle_thumb_bge(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if ((cpu.cpsr & CPSR_N_FLAG) == (cpu.cpsr & CPSR_V_FLAG)) { // Check Negative and Overflow flags
        cpu.r[15] += (offset << 1); // Branch to target address
        LOG_INFO("Executing Thumb BGE: Branch to 0x%08X", cpu.r[15]);
    }
}

static void handle_thumb_blt(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if ((cpu.cpsr & CPSR_N_FLAG) != (cpu.cpsr & CPSR_V_FLAG)) { // Check Negative and Overflow flags
        cpu.r[15] += (offset << 1); // Branch to target address
        LOG_INFO("Executing Thumb BLT: Branch to 0x%08X", cpu.r[15]);
    }
}

static void handle_thumb_bgt(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if (!(cpu.cpsr & CPSR_Z_FLAG) && ((cpu.cpsr & CPSR_N_FLAG) == (cpu.cpsr & CPSR_V_FLAG))) { // Check Zero, Negative, and Overflow flags
        cpu.r[15] += (offset << 1); // Branch to target address
        LOG_INFO("Executing Thumb BGT: Branch to 0x%08X", cpu.r[15]);
    }
}

static void handle_thumb_ble(uint16_t instruction) {
    int8_t offset = instruction & 0xFF; // Signed 8-bit offset
    if ((cpu.cpsr & CPSR_Z_FLAG) || ((cpu.cpsr & CPSR_N_FLAG) != (cpu.cpsr & CPSR_V_FLAG))) { // Check Zero, Negative, and Overflow flags
        cpu.r[15] += (offset << 1); // Branch to target address
        LOG_INFO("Executing Thumb BLE: Branch to 0x%08X", cpu.r[15]);
    }
}

static void handle_thumb_swi(uint16_t instruction) {
    uint8_t comment = instruction & 0xFF; // Software interrupt comment (bits 0-7)

    // Handle the software interrupt
    LOG_INFO("Executing Thumb SWI: Software interrupt with comment 0x%02X", comment);
    handle_software_interrupt(comment); // Call the software interrupt handler
}

static void handle_thumb_b(uint16_t instruction) {
    int16_t offset = instruction & 0x7FF; // Signed 11-bit offset (bits 0-10)
    if (offset & 0x400) { // Sign-extend the offset
        offset |= 0xF800;
    }

    // Perform the branch operation
    cpu.r[15] += (offset << 1); // Branch to target address
    LOG_INFO("Executing Thumb B: Branch to 0x%08X", cpu.r[15]);
}

static void handle_thumb_bl(uint16_t instruction) {
    int16_t offset = instruction & 0x7FF; // Signed 11-bit offset (bits 0-10)
    if (offset & 0x400) { // Sign-extend the offset
        offset |= 0xF800;
    }

    // Perform the branch with link operation
    cpu.r[14] = cpu.r[15] + 2; // Save return address in LR
    cpu.r[15] += (offset << 1); // Branch to target address
    LOG_INFO("Executing Thumb BL: Branch to 0x%08X with link", cpu.r[15]);
}
