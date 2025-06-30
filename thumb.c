#include "cpu.h"
#include "thumb.h"
#include <stdio.h>
#include <stdlib.h>

// Root of the Trie for Thumb instructions
static ThumbTrieNode* thumb_trie_root = NULL;

// Function to create a new Trie node
static ThumbTrieNode* create_thumb_trie_node() {
    ThumbTrieNode* node = (ThumbTrieNode*)malloc(sizeof(ThumbTrieNode));
    node->children[0] = NULL;
    node->children[1] = NULL;
    node->handler = NULL;
    node->cycles = 0;
    return node;
}

// Function to insert an instruction into the Trie
static void insert_thumb_instruction(uint16_t opcode, int length, void (*handler)(uint16_t), uint8_t cycles) {
    ThumbTrieNode* current = thumb_trie_root;
    for (int i = length - 1; i >= 0; i--) {
        int bit = (opcode >> i) & 1;
        if (!current->children[bit]) {
            current->children[bit] = create_thumb_trie_node();
        }
        current = current->children[bit];
    }
    current->handler = handler;
    current->cycles = cycles;
}

// Function to initialize the Thumb instruction Trie
void thumb_init_trie() {
    thumb_trie_root = create_thumb_trie_node();

    // Example: Insert instructions into the Trie
    insert_thumb_instruction(0x00, 6, handle_thumb_mov, 1); // MOV (6-bit opcode)
    insert_thumb_instruction(0x01, 6, handle_thumb_add, 1); // ADD (6-bit opcode)
    insert_thumb_instruction(0x02, 6, handle_thumb_sub, 1); // SUB (6-bit opcode)
    insert_thumb_instruction(0xE, 4, handle_thumb_b, 3);    // B (4-bit opcode)
    insert_thumb_instruction(0xF, 4, handle_thumb_bl, 4);   // BL (4-bit opcode)
    insert_thumb_instruction(0x14, 5, handle_thumb_ldr, 2); // LDR (5-bit opcode)
    insert_thumb_instruction(0x15, 5, handle_thumb_str, 2); // STR (5-bit opcode)
    insert_thumb_instruction(0x16, 5, handle_thumb_ldrb, 2); // LDRB (5-bit opcode)
    insert_thumb_instruction(0x17, 5, handle_thumb_strb, 2); // STRB (5-bit opcode)
    insert_thumb_instruction(0x18, 5, handle_thumb_ldrh, 2); // LDRH (5-bit opcode)
    insert_thumb_instruction(0x19, 5, handle_thumb_strh, 2); // STRH (5-bit opcode)
    insert_thumb_instruction(0x1A, 5, handle_thumb_ldrsb, 2); // LDRSB (5-bit opcode)
    insert_thumb_instruction(0x1B, 5, handle_thumb_ldrsh, 2); // LDRSH (5-bit opcode)
    insert_thumb_instruction(0x20, 6, handle_thumb_push, 3); // PUSH (6-bit opcode)
    insert_thumb_instruction(0x21, 6, handle_thumb_pop, 3);  // POP (6-bit opcode)
    insert_thumb_instruction(0x22, 6, handle_thumb_stmia, 3); // STMIA (6-bit opcode)
    insert_thumb_instruction(0x23, 6, handle_thumb_ldmia, 3); // LDMIA (6-bit opcode)
    insert_thumb_instruction(0x24, 6, handle_thumb_adc, 1);  // ADC (6-bit opcode)
    insert_thumb_instruction(0x25, 6, handle_thumb_sbc, 1);  // SBC (6-bit opcode)
    insert_thumb_instruction(0x26, 6, handle_thumb_ror, 1);  // ROR (6-bit opcode)
    insert_thumb_instruction(0x27, 6, handle_thumb_tst, 1);  // TST (6-bit opcode)
    insert_thumb_instruction(0x28, 6, handle_thumb_neg, 1);  // NEG (6-bit opcode)
    insert_thumb_instruction(0x29, 6, handle_thumb_cmp, 1);  // CMP (6-bit opcode)
    insert_thumb_instruction(0x2A, 6, handle_thumb_cmn, 1);  // CMN (6-bit opcode)
    insert_thumb_instruction(0x2B, 6, handle_thumb_orr, 1);  // ORR (6-bit opcode)
    insert_thumb_instruction(0x2C, 6, handle_thumb_bic, 1);  // BIC (6-bit opcode)
    insert_thumb_instruction(0x2D, 6, handle_thumb_mvn, 1);  // MVN (6-bit opcode)
    insert_thumb_instruction(0x2E, 6, handle_thumb_bx, 2);   // BX (6-bit opcode)
    insert_thumb_instruction(0x2F, 6, handle_thumb_swi, 2);  // SWI (6-bit opcode)
    insert_thumb_instruction(0x30, 6, handle_thumb_bkpt, 2); // BKPT (6-bit opcode)
    insert_thumb_instruction(0x31, 6, handle_thumb_nop, 1);  // NOP (6-bit opcode)
    insert_thumb_instruction(0x32, 6, handle_thumb_stmdb, 3); // STMDB (6-bit opcode)
    insert_thumb_instruction(0x33, 6, handle_thumb_ldmdb, 3); // LDMDB (6-bit opcode)
    insert_thumb_instruction(0x34, 4, handle_thumb_beq, 2);  // BEQ (4-bit opcode)
    insert_thumb_instruction(0x35, 4, handle_thumb_bne, 2);  // BNE (4-bit opcode)
    insert_thumb_instruction(0x36, 4, handle_thumb_bcs, 2);  // BCS (4-bit opcode)
    insert_thumb_instruction(0x37, 4, handle_thumb_bcc, 2);  // BCC (4-bit opcode)
    insert_thumb_instruction(0x38, 4, handle_thumb_bmi, 2);  // BMI (4-bit opcode)
    insert_thumb_instruction(0x39, 4, handle_thumb_bpl, 2);  // BPL (4-bit opcode)
    insert_thumb_instruction(0x3A, 4, handle_thumb_bvs, 2);  // BVS (4-bit opcode)
    insert_thumb_instruction(0x3B, 4, handle_thumb_bvc, 2);  // BVC (4-bit opcode)
    insert_thumb_instruction(0x3C, 4, handle_thumb_bhi, 2);  // BHI (4-bit opcode)
    insert_thumb_instruction(0x3D, 4, handle_thumb_bls, 2);  // BLS (4-bit opcode)
    insert_thumb_instruction(0x3E, 4, handle_thumb_bge, 2);  // BGE (4-bit opcode)
    insert_thumb_instruction(0x3F, 4, handle_thumb_blt, 2);  // BLT (4-bit opcode)
    insert_thumb_instruction(0x40, 4, handle_thumb_bgt, 2);  // BGT (4-bit opcode)
    insert_thumb_instruction(0x41, 4, handle_thumb_ble, 2);  // BLE (4-bit opcode)
    insert_thumb_instruction(0x42, 4, handle_thumb_setend, 1); // SETEND (4-bit opcode)
    // Add more instructions as needed
}

// Function to decode and execute a Thumb instruction
uint8_t thumb_decode_and_execute(uint16_t instruction) {
    ThumbTrieNode* current = thumb_trie_root;
    for (int i = 15; i >= 0; i--) {
        int bit = (instruction >> i) & 1;
        if (!current->children[bit]) {
            printf("Undefined Thumb instruction\n");
            return 0;
        }
        current = current->children[bit];
    }
    if (current->handler) {
        current->handler(instruction);
        return current->cycles;
    } else {
        printf("Undefined Thumb instruction\n");
        return 0;
    }
}

// Example handler function for Thumb MOV
static void handle_thumb_mov(uint16_t instruction) {
    // Extract destination register and immediate value from the instruction
    uint8_t rd = (instruction & 0x07); // Destination register (bits 0-2)
    uint16_t immediate = (instruction >> 3) & 0xFF; // Immediate value (bits 3-10)

    // Perform the MOV operation
    cpu.r[rd] = immediate;

    // Update CPSR flags based on the result
    update_cpsr_flags(immediate, 0); // Carry out is not relevant for MOV

    printf("Executing Thumb MOV: R%d = %d\n", rd, immediate);
}

// Example handler function for Thumb ADD
static void handle_thumb_add(uint16_t instruction) {
    // Extract operands and destination register from the instruction
    uint8_t rd = (instruction & 0x07); // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)
    uint16_t immediate = (instruction >> 6) & 0xFF; // Immediate value (bits 6-13)

    // Perform the ADD operation
    uint32_t result = cpu.r[rs] + immediate;

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (result < cpu.r[rs])); // Carry out is relevant for ADD

    printf("Executing Thumb ADD: R%d = R%d + %d\n", rd, rs, immediate);
}

// Example handler function for Thumb SUB
static void handle_thumb_sub(uint16_t instruction) {
    // Extract operands and destination register from the instruction
    uint8_t rd = (instruction & 0x07); // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)
    uint16_t immediate = (instruction >> 6) & 0xFF; // Immediate value (bits 6-13)

    // Perform the SUB operation
    uint32_t result = cpu.r[rs] - immediate;

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (cpu.r[rs] < immediate)); // Carry out is relevant for SUB

    printf("Executing Thumb SUB: R%d = R%d - %d\n", rd, rs, immediate);
}

// Example handler function for Thumb B
static void handle_thumb_b(uint16_t instruction) {
    // Extract the offset from the instruction
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)

    // Sign-extend the offset
    if (offset & 0x0400) {
        offset |= 0xF800;
    }

    // Update the program counter
    cpu.r[15] += (offset << 1); // Offset is in halfwords

    printf("Executing Thumb B: PC = PC + %d\n", offset << 1);
}

// Example handler function for Thumb BL
static void handle_thumb_bl(uint16_t instruction) {
    // Extract the offset from the instruction
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)

    // Sign-extend the offset
    if (offset & 0x0400) {
        offset |= 0xF800;
    }

    // Save the return address in LR (R14)
    cpu.r[14] = cpu.r[15] + 2; // PC points to the next instruction

    // Update the program counter
    cpu.r[15] += (offset << 1); // Offset is in halfwords

    printf("Executing Thumb BL: LR = %d, PC = PC + %d\n", cpu.r[14], offset << 1);
}

// Example handler function for Thumb LDR
static void handle_thumb_ldr(uint16_t instruction) {
    // Extract base register, destination register, and offset from the instruction
    uint8_t rd = (instruction & 0x07); // Destination register (bits 0-2)
    uint8_t rb = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint16_t offset = (instruction >> 6) & 0x1F; // Offset (bits 6-10)

    // Calculate the address
    uint32_t address = cpu.r[rb] + (offset << 2); // Offset is in words

    // Perform the LDR operation
    cpu.r[rd] = memory_read_32(address); // Load the value from memory

    printf("Executing Thumb LDR: R%d = [R%d + %d]\n", rd, rb, offset << 2);
}

// Example handler function for Thumb STR
static void handle_thumb_str(uint16_t instruction) {
    // Extract base register, source register, and offset from the instruction
    uint8_t rs = (instruction & 0x07); // Source register (bits 0-2)
    uint8_t rb = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint16_t offset = (instruction >> 6) & 0x1F; // Offset (bits 6-10)

    // Calculate the address
    uint32_t address = cpu.r[rb] + (offset << 2); // Offset is in words

    // Perform the STR operation
    memory_write_32(address, cpu.r[rs]); // Store the value to memory

    printf("Executing Thumb STR: [R%d + %d] = R%d\n", rb, offset << 2, rs);
}

// Example handler function for Thumb LDRB
static void handle_thumb_ldrb(uint16_t instruction) {
    // Extract base register, destination register, and offset from the instruction
    uint8_t rd = (instruction & 0x07); // Destination register (bits 0-2)
    uint8_t rb = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint16_t offset = (instruction >> 6) & 0x1F; // Offset (bits 6-10)

    // Calculate the address
    uint32_t address = cpu.r[rb] + offset; // Offset is in bytes

    // Perform the LDRB operation
    cpu.r[rd] = memory_read_8(address); // Load the byte from memory

    printf("Executing Thumb LDRB: R%d = [R%d + %d]\n", rd, rb, offset);
}

// Example handler function for Thumb STRB
static void handle_thumb_strb(uint16_t instruction) {
    // Extract base register, source register, and offset from the instruction
    uint8_t rs = (instruction & 0x07); // Source register (bits 0-2)
    uint8_t rb = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint16_t offset = (instruction >> 6) & 0x1F; // Offset (bits 6-10)

    // Calculate the address
    uint32_t address = cpu.r[rb] + offset; // Offset is in bytes

    // Perform the STRB operation
    memory_write_8(address, (uint8_t)cpu.r[rs]); // Store the byte to memory

    printf("Executing Thumb STRB: [R%d + %d] = R%d\n", rb, offset, rs);
}

// Example handler function for Thumb LDRH
static void handle_thumb_ldrh(uint16_t instruction) {
    // Extract base register, destination register, and offset from the instruction
    uint8_t rd = (instruction & 0x07); // Destination register (bits 0-2)
    uint8_t rb = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint16_t offset = (instruction >> 6) & 0x1F; // Offset (bits 6-10)

    // Calculate the address
    uint32_t address = cpu.r[rb] + (offset << 1); // Offset is in halfwords

    // Perform the LDRH operation
    cpu.r[rd] = memory_read_16(address); // Load the halfword from memory

    printf("Executing Thumb LDRH: R%d = [R%d + %d]\n", rd, rb, offset << 1);
}

// Example handler function for Thumb STRH
static void handle_thumb_strh(uint16_t instruction) {
    // Extract base register, source register, and offset from the instruction
    uint8_t rs = (instruction & 0x07); // Source register (bits 0-2)
    uint8_t rb = (instruction >> 3) & 0x07; // Base register (bits 3-5)
    uint16_t offset = (instruction >> 6) & 0x1F; // Offset (bits 6-10)

    // Calculate the address
    uint32_t address = cpu.r[rb] + (offset << 1); // Offset is in halfwords

    // Perform the STRH operation
    memory_write_16(address, (uint16_t)cpu.r[rs]); // Store the halfword to memory

    printf("Executing Thumb STRH: [R%d + %d] = R%d\n", rb, offset << 1, rs);
}

// Example handler function for Thumb ADC
static void handle_thumb_adc(uint16_t instruction) {
    uint8_t rd = (instruction & 0x07); // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    // Perform the ADC operation
    uint32_t carry = (cpu.cpsr & CPSR_C_FLAG) ? 1 : 0; // Extract the carry flag
    uint32_t result = cpu.r[rs] + cpu.r[rd] + carry;

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (result < cpu.r[rs] || result < cpu.r[rd])); // Carry out is relevant for ADC

    printf("Executing Thumb ADC: R%d = R%d + R%d + Carry\n", rd, rs, rd);
}

// Example handler function for Thumb SBC
static void handle_thumb_sbc(uint16_t instruction) {
    uint8_t rd = (instruction & 0x07); // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    // Perform the SBC operation
    uint32_t carry = (cpu.cpsr & CPSR_C_FLAG) ? 1 : 0; // Extract the carry flag
    uint32_t result = cpu.r[rd] - cpu.r[rs] - (1 - carry);

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (cpu.r[rd] < cpu.r[rs])); // Carry out is relevant for SBC

    printf("Executing Thumb SBC: R%d = R%d - R%d - (1 - Carry)\n", rd, rd, rs);
}

// Example handler function for Thumb ROR
static void handle_thumb_ror(uint16_t instruction) {
    // Extract operands and destination register from the instruction
    uint8_t rd = (instruction & 0x07); // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    // Perform the ROR operation
    uint32_t value = cpu.r[rs];
    uint8_t shift = cpu.r[rd] & 0x1F; // Use lower 5 bits for the shift amount
    uint32_t result = (value >> shift) | (value << (32 - shift));

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (value >> (shift - 1)) & 1); // Carry out is the last shifted-out bit

    printf("Executing Thumb ROR: R%d = R%d ROR R%d\n", rd, rs, rd);
}

// Example handler function for Thumb TST
static void handle_thumb_tst(uint16_t instruction) {
    // Extract operands from the instruction
    uint8_t rn = (instruction & 0x07); // First operand register (bits 0-2)
    uint8_t rm = (instruction >> 3) & 0x07; // Second operand register (bits 3-5)

    // Perform the TST operation
    uint32_t result = cpu.r[rn] & cpu.r[rm];

    // Update CPSR flags based on the result
    update_cpsr_flags(result, 0); // Carry out is not relevant for TST

    printf("Executing Thumb TST: R%d & R%d\n", rn, rm);
}

// Example handler function for Thumb NEG
static void handle_thumb_neg(uint16_t instruction) {
    // Extract operands from the instruction
    uint8_t rd = (instruction & 0x07); // Destination register (bits 0-2)
    uint8_t rm = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    // Perform the NEG operation
    uint32_t result = -cpu.r[rm];

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, cpu.r[rm] != 0); // Carry out is relevant for NEG

    printf("Executing Thumb NEG: R%d = -R%d\n", rd, rm);
}

// Example handler function for Thumb CMP
static void handle_thumb_cmp(uint16_t instruction) {
    // Extract operands from the instruction
    uint8_t rn = (instruction & 0x07); // First operand register (bits 0-2)
    uint8_t rm = (instruction >> 3) & 0x07; // Second operand register (bits 3-5)

    // Perform the CMP operation
    uint32_t result = cpu.r[rn] - cpu.r[rm];

    // Update CPSR flags based on the result
    update_cpsr_flags(result, cpu.r[rn] < cpu.r[rm]); // Carry out is relevant for CMP

    printf("Executing Thumb CMP: R%d - R%d\n", rn, rm);
}

// Example handler function for Thumb CMN
static void handle_thumb_cmn(uint16_t instruction) {
    // Extract operands from the instruction
    uint8_t rn = (instruction & 0x07); // First operand register (bits 0-2)
    uint8_t rm = (instruction >> 3) & 0x07; // Second operand register (bits 3-5)

    // Perform the CMN operation
    uint32_t result = cpu.r[rn] + cpu.r[rm];

    // Update CPSR flags based on the result
    update_cpsr_flags(result, (result < cpu.r[rn])); // Carry out is relevant for CMN

    printf("Executing Thumb CMN: R%d + R%d\n", rn, rm);
}

// Example handler function for Thumb ORR
static void handle_thumb_orr(uint16_t instruction) {
    // Extract operands and destination register from the instruction
    uint8_t rd = (instruction & 0x07); // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    // Perform the ORR operation
    uint32_t result = cpu.r[rd] | cpu.r[rs];

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, 0); // Carry out is not relevant for ORR

    printf("Executing Thumb ORR: R%d = R%d | R%d\n", rd, rd, rs);
}

// Example handler function for Thumb BIC
static void handle_thumb_bic(uint16_t instruction) {
    // Extract operands and destination register from the instruction
    uint8_t rd = (instruction & 0x07); // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    // Perform the BIC operation
    uint32_t result = cpu.r[rd] & ~cpu.r[rs];

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, 0); // Carry out is not relevant for BIC

    printf("Executing Thumb BIC: R%d = R%d & ~R%d\n", rd, rd, rs);
}

// Example handler function for Thumb MVN
static void handle_thumb_mvn(uint16_t instruction) {
    // Extract destination register and source register from the instruction
    uint8_t rd = (instruction & 0x07); // Destination register (bits 0-2)
    uint8_t rs = (instruction >> 3) & 0x07; // Source register (bits 3-5)

    // Perform the MVN operation
    uint32_t result = ~cpu.r[rs];

    // Update the destination register
    cpu.r[rd] = result;

    // Update CPSR flags based on the result
    update_cpsr_flags(result, 0); // Carry out is not relevant for MVN

    printf("Executing Thumb MVN: R%d = ~R%d\n", rd, rs);
}

// Example handler function for Thumb BX
static void handle_thumb_bx(uint16_t instruction) {
    // Extract the register containing the target address
    uint8_t rm = (instruction >> 3) & 0x07; // Target register (bits 3-5)

    // Update the program counter
    cpu.r[15] = cpu.r[rm];

    // Switch to ARM mode if the least significant bit of the address is 0
    if ((cpu.r[15] & 0x1) == 0) {
        set_cpu_mode(&cpu, ARM_MODE); // Use set_cpu_mode to update the CPU state
    }

    printf("Executing Thumb BX: PC = R%d\n", rm);
}

// Example handler function for Thumb SWI
static void handle_thumb_swi(uint16_t instruction) {
    // Extract the immediate value from the instruction
    uint8_t immediate = instruction & 0xFF; // Immediate value (bits 0-7)

    // Perform the SWI operation (software interrupt)
    printf("Executing Thumb SWI: Immediate = %d\n", immediate);

    // Handle the software interrupt (implementation depends on the emulator's interrupt system)
    handle_software_interrupt(immediate);
}

// Example handler function for Thumb BKPT
static void handle_thumb_bkpt(uint16_t instruction) {
    // Extract the immediate value from the instruction
    uint8_t immediate = instruction & 0xFF; // Immediate value (bits 0-7)

    // Perform the BKPT operation (breakpoint)
    printf("Executing Thumb BKPT: Immediate = %d\n", immediate);

    // Handle the breakpoint (implementation depends on the emulator's debugging system)
    handle_breakpoint(immediate);
}

// Example handler function for Thumb NOP
static void handle_thumb_nop(uint16_t instruction) {
    // Perform the NOP operation (no operation)
    printf("Executing Thumb NOP\n");

    // NOP does not modify any state or perform any action
}

// Example handler function for Thumb STMDB
static void handle_thumb_stmdb(uint16_t instruction) {
    // Extract the base register and register list from the instruction
    uint8_t rb = (instruction >> 8) & 0x07; // Base register (bits 8-10)
    uint16_t reg_list = instruction & 0xFF; // Register list (bits 0-7)

    // Store registers to memory in descending order starting at the base address
    uint32_t address = cpu.r[rb];
    for (int i = 7; i >= 0; i--) {
        if (reg_list & (1 << i)) {
            address -= 4; // Decrement address by 4
            memory_write_32(address, cpu.r[i]); // Store register to memory
        }
    }

    // Update the base register
    cpu.r[rb] = address;

    printf("Executing Thumb STMDB: R%d = %d\n", rb, address);
}

// Example handler function for Thumb LDMDB
static void handle_thumb_ldmdb(uint16_t instruction) {
    // Extract the base register and register list from the instruction
    uint8_t rb = (instruction >> 8) & 0x07; // Base register (bits 8-10)
    uint16_t reg_list = instruction & 0xFF; // Register list (bits 0-7)

    // Load registers from memory in descending order starting at the base address
    uint32_t address = cpu.r[rb];
    for (int i = 7; i >= 0; i--) {
        if (reg_list & (1 << i)) {
            cpu.r[i] = memory_read_32(address); // Load register from memory
            address -= 4; // Decrement address by 4
        }
    }

    // Update the base register
    cpu.r[rb] = address;

    printf("Executing Thumb LDMDB: R%d = %d\n", rb, address);
}

// Example handler function for Thumb BEQ
static void handle_thumb_beq(uint16_t instruction) {
    // Extract the offset from the instruction
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)

    // Sign-extend the offset
    if (offset & 0x0400) {
        offset |= 0xF800;
    }

    // Check the Zero flag in CPSR
    if (cpu.cpsr & (1 << 30)) { // Z flag is set
        cpu.r[15] += (offset << 1); // Offset is in halfwords
        printf("Executing Thumb BEQ: PC = PC + %d\n", offset << 1);
    } else {
        printf("Thumb BEQ condition not met\n");
    }
}

// Example handler function for Thumb BNE
static void handle_thumb_bne(uint16_t instruction) {
    // Extract the offset from the instruction
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)

    // Sign-extend the offset
    if (offset & 0x0400) {
        offset |= 0xF800;
    }

    // Check the Zero flag in CPSR
    if (!(cpu.cpsr & (1 << 30))) { // Z flag is clear
        cpu.r[15] += (offset << 1); // Offset is in halfwords
        printf("Executing Thumb BNE: PC = PC + %d\n", offset << 1);
    } else {
        printf("Thumb BNE condition not met\n");
    }
}

// Example handler function for Thumb BCS
static void handle_thumb_bcs(uint16_t instruction) {
    // Extract the offset from the instruction
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)

    // Sign-extend the offset
    if (offset & 0x0400) {
        offset |= 0xF800;
    }

    // Check the Carry flag in CPSR
    if (cpu.cpsr & (1 << 29)) { // C flag is set
        cpu.r[15] += (offset << 1); // Offset is in halfwords
        printf("Executing Thumb BCS: PC = PC + %d\n", offset << 1);
    } else {
        printf("Thumb BCS condition not met\n");
    }
}

// Example handler function for Thumb BCC
static void handle_thumb_bcc(uint16_t instruction) {
    // Extract the offset from the instruction
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)

    // Sign-extend the offset
    if (offset & 0x0400) {
        offset |= 0xF800;
    }

    // Check the Carry flag in CPSR
    if (!(cpu.cpsr & (1 << 29))) { // C flag is clear
        cpu.r[15] += (offset << 1); // Offset is in halfwords
        printf("Executing Thumb BCC: PC = PC + %d\n", offset << 1);
    } else {
        printf("Thumb BCC condition not met\n");
    }
}

// Example handler function for Thumb BMI
static void handle_thumb_bmi(uint16_t instruction) {
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)
    if (offset & 0x0400) {
        offset |= 0xF800; // Sign-extend the offset
    }
    if (cpu.cpsr & CPSR_N_FLAG) { // Check Negative flag
        cpu.r[15] += (offset << 1); // Update PC
        printf("Executing Thumb BMI: PC = PC + %d\n", offset << 1);
    }
}

// Example handler function for Thumb BPL
static void handle_thumb_bpl(uint16_t instruction) {
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)
    if (offset & 0x0400) {
        offset |= 0xF800; // Sign-extend the offset
    }
    if (!(cpu.cpsr & CPSR_N_FLAG)) { // Check Negative flag
        cpu.r[15] += (offset << 1); // Update PC
        printf("Executing Thumb BPL: PC = PC + %d\n", offset << 1);
    }
}

// Example handler function for Thumb BVS
static void handle_thumb_bvs(uint16_t instruction) {
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)
    if (offset & 0x0400) {
        offset |= 0xF800; // Sign-extend the offset
    }
    if (cpu.cpsr & CPSR_V_FLAG) { // Check Overflow flag
        cpu.r[15] += (offset << 1); // Update PC
        printf("Executing Thumb BVS: PC = PC + %d\n", offset << 1);
    }
}

// Example handler function for Thumb BVC
static void handle_thumb_bvc(uint16_t instruction) {
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)
    if (offset & 0x0400) {
        offset |= 0xF800; // Sign-extend the offset
    }
    if (!(cpu.cpsr & CPSR_V_FLAG)) { // Check Overflow flag
        cpu.r[15] += (offset << 1); // Update PC
        printf("Executing Thumb BVC: PC = PC + %d\n", offset << 1);
    }
}

// Example handler function for Thumb BHI
static void handle_thumb_bhi(uint16_t instruction) {
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)
    if (offset & 0x0400) {
        offset |= 0xF800; // Sign-extend the offset
    }
    if ((cpu.cpsr & CPSR_C_FLAG) && !(cpu.cpsr & CPSR_Z_FLAG)) { // Check Carry and Zero flags
        cpu.r[15] += (offset << 1); // Update PC
        printf("Executing Thumb BHI: PC = PC + %d\n", offset << 1);
    }
}

// Example handler function for Thumb BLS
static void handle_thumb_bls(uint16_t instruction) {
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)
    if (offset & 0x0400) {
        offset |= 0xF800; // Sign-extend the offset
    }
    if (!(cpu.cpsr & CPSR_C_FLAG) || (cpu.cpsr & CPSR_Z_FLAG)) { // Check Carry and Zero flags
        cpu.r[15] += (offset << 1); // Update PC
        printf("Executing Thumb BLS: PC = PC + %d\n", offset << 1);
    }
}

// Example handler function for Thumb BGE
static void handle_thumb_bge(uint16_t instruction) {
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)
    if (offset & 0x0400) {
        offset |= 0xF800; // Sign-extend the offset
    }
    if ((cpu.cpsr & CPSR_N_FLAG) == (cpu.cpsr & CPSR_V_FLAG)) { // Check Negative and Overflow flags
        cpu.r[15] += (offset << 1); // Update PC
        printf("Executing Thumb BGE: PC = PC + %d\n", offset << 1);
    }
}

// Example handler function for Thumb BLT
static void handle_thumb_blt(uint16_t instruction) {
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)
    if (offset & 0x0400) {
        offset |= 0xF800; // Sign-extend the offset
    }
    if ((cpu.cpsr & CPSR_N_FLAG) != (cpu.cpsr & CPSR_V_FLAG)) { // Check Negative and Overflow flags
        cpu.r[15] += (offset << 1); // Update PC
        printf("Executing Thumb BLT: PC = PC + %d\n", offset << 1);
    }
}

// Example handler function for Thumb BGT
static void handle_thumb_bgt(uint16_t instruction) {
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)
    if (offset & 0x0400) {
        offset |= 0xF800; // Sign-extend the offset
    }
    if (((cpu.cpsr & CPSR_Z_FLAG) == 0) && ((cpu.cpsr & CPSR_N_FLAG) == (cpu.cpsr & CPSR_V_FLAG))) { // Check Zero, Negative, and Overflow flags
        cpu.r[15] += (offset << 1); // Update PC
        printf("Executing Thumb BGT: PC = PC + %d\n", offset << 1);
    }
}

// Define missing Thumb instruction handlers
static void handle_thumb_ldrsb(uint16_t instruction) {
    // Extract the base register and offset
    uint8_t base_reg = (instruction >> 3) & 0x07; // Bits 3-5
    uint8_t dest_reg = instruction & 0x07;        // Bits 0-2
    uint8_t offset = (instruction >> 6) & 0x1F;  // Bits 6-10

    // Calculate the address
    uint32_t address = cpu.r[base_reg] + offset;

    // Perform the signed byte load using memory_read_8
    int8_t value = (int8_t)memory_read_8(address);

    // Store the value in the destination register
    cpu.r[dest_reg] = (int32_t)value;

    // Update the PC (Program Counter)
    cpu.r[15] += 2;

    printf("Executing Thumb LDRSB: R%d = [R%d + %d] = %d\n", dest_reg, base_reg, offset, value);
}

static void handle_thumb_ldrsh(uint16_t instruction) {
    // Extract the base register and offset
    uint8_t base_reg = (instruction >> 3) & 0x07; // Bits 3-5
    uint8_t dest_reg = instruction & 0x07;        // Bits 0-2
    uint8_t offset = (instruction >> 6) & 0x1F;  // Bits 6-10

    // Calculate the address
    uint32_t address = cpu.r[base_reg] + offset;

    // Perform the signed halfword load using memory_read_16
    int16_t value = (int16_t)memory_read_16(address);

    // Store the value in the destination register
    cpu.r[dest_reg] = (int32_t)value;

    // Update the PC (Program Counter)
    cpu.r[15] += 2;

    printf("Executing Thumb LDRSH: R%d = [R%d + %d] = %d\n", dest_reg, base_reg, offset, value);
}

static void handle_thumb_push(uint16_t instruction) {
    // Extract the register list
    uint8_t reg_list = instruction & 0xFF; // Bits 0-7

    // Push registers onto the stack
    for (int i = 0; i < 8; i++) {
        if (reg_list & (1 << i)) {
            cpu.r[13] -= 4; // Decrement SP (R13)
            memory_write_32(cpu.r[13], cpu.r[i]);
            printf("Pushed R%d: %d\n", i, cpu.r[i]);
        }
    }

    // Update the PC (Program Counter)
    cpu.r[15] += 2;

    printf("Executing Thumb PUSH\n");
}

static void handle_thumb_pop(uint16_t instruction) {
    // Extract the register list
    uint8_t reg_list = instruction & 0xFF; // Bits 0-7

    // Pop registers from the stack
    for (int i = 0; i < 8; i++) {
        if (reg_list & (1 << i)) {
            cpu.r[i] = memory_read_32(cpu.r[13]);
            cpu.r[13] += 4; // Increment SP (R13)
            printf("Popped R%d: %d\n", i, cpu.r[i]);
        }
    }

    // Update the PC (Program Counter)
    cpu.r[15] += 2;

    printf("Executing Thumb POP\n");
}

// Example handler function for Thumb STMIA
static void handle_thumb_stmia(uint16_t instruction) {
    // Extract the base register and register list
    uint8_t base_reg = (instruction >> 8) & 0x07; // Bits 8-10
    uint8_t reg_list = instruction & 0xFF;        // Bits 0-7

    // Store multiple registers
    uint32_t address = cpu.r[base_reg];
    for (int i = 0; i < 8; i++) {
        if (reg_list & (1 << i)) {
            memory_write_32(address, cpu.r[i]);
            address += 4;
            printf("Stored R%d to address 0x%08X\n", i, address - 4);
        }
    }

    // Update the base register
    cpu.r[base_reg] = address;

    // Update the PC (Program Counter)
    cpu.r[15] += 2;

    printf("Executing Thumb STMIA\n");
}

// Example handler function for Thumb LDMIA
static void handle_thumb_ldmia(uint16_t instruction) {
    // Extract the base register and register list
    uint8_t base_reg = (instruction >> 8) & 0x07; // Bits 8-10
    uint8_t reg_list = instruction & 0xFF;        // Bits 0-7

    // Load multiple registers
    uint32_t address = cpu.r[base_reg];
    for (int i = 0; i < 8; i++) {
        if (reg_list & (1 << i)) {
            cpu.r[i] = memory_read_32(address);
            address += 4;
            printf("Loaded R%d from address 0x%08X\n", i, address - 4);
        }
    }

    // Update the base register
    cpu.r[base_reg] = address;

    // Update the PC (Program Counter)
    cpu.r[15] += 2;

    printf("Executing Thumb LDMIA\n");
}

// Example handler function for Thumb BLE
static void handle_thumb_ble(uint16_t instruction) {
    int16_t offset = (instruction & 0x07FF); // Offset (bits 0-10)
    if (offset & 0x0400) {
        offset |= 0xF800; // Sign-extend the offset
    }
    if (((cpu.cpsr & CPSR_Z_FLAG) != 0) || ((cpu.cpsr & CPSR_N_FLAG) != (cpu.cpsr & CPSR_V_FLAG))) {
        cpu.r[15] += (offset << 1); // Update PC
        printf("Executing Thumb BLE: PC = PC + %d\n", offset << 1);
    }
}

static void handle_thumb_setend(uint16_t instruction) {
    uint8_t e_bit = (instruction >> 3) & 0x01; // Extract E bit (bit 3)

    if (e_bit) {
        cpu.cpsr |= CPSR_E_FLAG; // Set Endianness to Big Endian
        printf("Executing Thumb SETEND: Big Endian\n");
    } else {
        cpu.cpsr &= ~CPSR_E_FLAG; // Set Endianness to Little Endian
        printf("Executing Thumb SETEND: Little Endian\n");
    }

    // Update the PC (Program Counter)
    cpu.r[15] += 2;
}
