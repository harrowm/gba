#include "cpu.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define HASH_TABLE_SIZE 256
#define CYCLE_HASH_TABLE_SIZE 256

// CPU state structure
CPUState cpu;

// ARM Instructions
// Hash table for ARM instruction handlers
void (*instruction_hash_table[HASH_TABLE_SIZE])(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out);

// Hash table for ARM instruction cycle counts
uint32_t instruction_cycle_hash_table[CYCLE_HASH_TABLE_SIZE];

typedef struct {
    uint32_t opcode;
    uint32_t cycles;
    void (*handler)(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out); // Updated to return void
} Instruction;


// Initialize the CPU
void cpu_init() {
    cpu.r[15] = 0; // Program Counter (PC)
    cpu.r[14] = 0; // Link Register (LR)
    cpu.r[13] = 0; // Stack Pointer (SP)
    cpu.cpsr = 0;  // Current Program Status Register
    cpu.mode = ARM_MODE; // Start in ARM mode

    // Initialize the hash tables
    init_instruction_hash_table();
    init_cycle_hash_table();
}

// Fetch, decode, and execute instructions
void cpu_step(uint32_t cycles) {
    while (cycles > 0) {
        uint32_t instruction = memory_read_32(cpu.r[15]); // Fetch instruction
        uint32_t instruction_cycles = decode_and_execute(instruction); // Decode and execute

        // Deduct the cycles used by the instruction
        cycles -= instruction_cycles;

        // Handle interrupts if necessary
        if (check_interrupts()) {
            handle_interrupts();
        }
    }
}

// ARM Instruction handlers
// Data Processing Instructions
static void handle_and(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing AND\n");

    // Perform the AND operation
    cpu.r[rd] = cpu.r[rn] & operand2;
}

static void handle_eor(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing EOR\n");

    // Perform the EOR operation
    cpu.r[rd] = cpu.r[rn] ^ operand2;
}

static void handle_sub(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing SUB\n");

    // Perform the SUB operation
    cpu.r[rd] = cpu.r[rn] - operand2;
}

static void handle_rsb(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing RSB\n");

    // Perform the RSB operation
    cpu.r[rd] = operand2 - cpu.r[rn];
}

static void handle_add(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing ADD\n");

    // Perform the ADD operation
    cpu.r[rd] = cpu.r[rn] + operand2;
}

static void handle_adc(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing ADC\n");

    // Perform the ADC operation (Add with Carry)
    uint8_t carry = (cpu.cpsr >> 29) & 1; // Extract the Carry flag
    cpu.r[rd] = cpu.r[rn] + operand2 + carry;
}

static void handle_sbc(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing SBC\n");

    // Perform the SBC operation (Subtract with Carry)
    uint8_t carry = (cpu.cpsr >> 29) & 1; // Extract the Carry flag
    cpu.r[rd] = cpu.r[rn] - operand2 - (1 - carry);
}

static void handle_rsc(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing RSC\n");

    // Perform the RSC operation (Reverse Subtract with Carry)
    uint8_t carry = (cpu.cpsr >> 29) & 1; // Extract the Carry flag
    cpu.r[rd] = operand2 - cpu.r[rn] - (1 - carry);
}

static void handle_tst(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing TST\n");

    // Perform the TST operation (Test bits)
    uint32_t result = cpu.r[rn] & operand2;

    // Update CPSR flags using the refactored function
    update_cpsr_flags(result, carry_out);
}

static void handle_teq(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing TEQ\n");

    // Perform the TEQ operation (Test Equivalence)
    uint32_t result = cpu.r[rn] ^ operand2;

    // Update CPSR flags using the refactored function
    update_cpsr_flags(result, carry_out);
}

static void handle_cmp(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing CMP\n");

    // Perform the CMP operation (Compare)
    uint32_t result = cpu.r[rn] - operand2;

    // Update CPSR flags using the refactored function
    update_cpsr_flags(result, carry_out);
}

static void handle_cmn(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing CMN\n");

    // Perform the CMN operation (Compare Negative)
    uint32_t result = cpu.r[rn] + operand2;

    // Update CPSR flags using the refactored function
    update_cpsr_flags(result, carry_out);
}

static void handle_orr(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing ORR\n");

    // Perform the ORR operation (Logical OR)
    cpu.r[rd] = cpu.r[rn] | operand2;
}

static void handle_mov(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing MOV\n");

    // Perform the MOV operation (Move)
    cpu.r[rd] = operand2;
}

static void handle_bic(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing BIC\n");

    // Perform the BIC operation (Bit Clear)
    cpu.r[rd] = cpu.r[rn] & ~operand2;
}

static void handle_mvn(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing MVN\n");

    // Perform the MVN operation (Move Not)
    cpu.r[rd] = ~operand2;
}

// Load/Store Instructions
static void handle_ldr(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing LDR\n");

    // Perform the LDR operation (Load Register)
    uint32_t address = cpu.r[rn] + operand2;
    cpu.r[rd] = memory_read_32(address); // Assuming `memory_read_32` reads a 32-bit value from memory
}

static void handle_str(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing STR\n");

    // Perform the STR operation (Store Register)
    uint32_t address = cpu.r[rn] + operand2;
    memory_write_32(address, cpu.r[rd]); // Assuming `memory_write_32` writes a 32-bit value to memory
}

static void handle_ldrb(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing LDRB\n");

    // Perform the LDRB operation (Load Register Byte)
    uint32_t address = cpu.r[rn] + operand2;
    cpu.r[rd] = memory_read_8(address); // Assuming `memory_read_8` reads an 8-bit value from memory
}

static void handle_strb(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing STRB\n");

    // Perform the STRB operation (Store Register Byte)
    uint32_t address = cpu.r[rn] + operand2;
    memory_write_8(address, cpu.r[rd] & 0xFF); // Assuming `memory_write_8` writes an 8-bit value to memory
}

static void handle_ldrh(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing LDRH\n");

    // Perform the LDRH operation (Load Register Halfword)
    uint32_t address = cpu.r[rn] + operand2;
    cpu.r[rd] = memory_read_16(address); // Assuming `memory_read_16` reads a 16-bit value from memory
}

static void handle_strh(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing STRH\n");

    // Perform the STRH operation (Store Register Halfword)
    uint32_t address = cpu.r[rn] + operand2;
    memory_write_16(address, cpu.r[rd] & 0xFFFF); // Assuming `memory_write_16` writes a 16-bit value to memory
}

static void handle_ldrsb(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing LDRSB\n");

    // Perform the LDRSB operation (Load Register Signed Byte)
    uint32_t address = cpu.r[rn] + operand2;
    cpu.r[rd] = (int8_t)memory_read_8(address); // Assuming `memory_read_8` reads an 8-bit value from memory and casts it to signed
}

static void handle_ldrsh(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing LDRSH\n");

    // Perform the LDRSH operation (Load Register Signed Halfword)
    uint32_t address = cpu.r[rn] + operand2;
    cpu.r[rd] = (int16_t)memory_read_16(address); // Assuming `memory_read_16` reads a 16-bit value from memory and casts it to signed
}

// Branch Instructions
static void handle_b(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing B\n");

    // Perform the B operation (Branch)
    int32_t offset = (int32_t)(operand2 << 2); // Sign-extend and shift left by 2
    cpu.r[15] += offset; // Update the Program Counter (PC)
}

static void handle_bl(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing BL\n");

    // Perform the BL operation (Branch with Link)
    int32_t offset = (int32_t)(operand2 << 2); // Sign-extend and shift left by 2
    cpu.r[14] = cpu.r[15]; // Save the current PC to the Link Register (LR)
    cpu.r[15] += offset; // Update the Program Counter (PC)
}

// Multiply Instructions
static void handle_mul(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing MUL\n");

    // Perform the MUL operation (Multiply)
    cpu.r[rd] = cpu.r[rn] * operand2;
}

static void handle_mla(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing MLA\n");

    // Perform the MLA operation (Multiply-Accumulate)
    cpu.r[rd] = (cpu.r[rn] * operand2) + cpu.r[carry_out]; // Assuming carry_out is used as the accumulator register
}

static void handle_umull(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing UMULL\n");

    // Perform the UMULL operation (Unsigned Multiply Long)
    uint64_t result = (uint64_t)cpu.r[rn] * (uint64_t)operand2;
    cpu.r[rd] = (uint32_t)(result & 0xFFFFFFFF); // Lower 32 bits
    cpu.r[carry_out] = (uint32_t)(result >> 32); // Upper 32 bits
}

static void handle_smull(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing SMULL\n");

    // Perform the SMULL operation (Signed Multiply Long)
    int64_t result = (int64_t)(int32_t)cpu.r[rn] * (int64_t)(int32_t)operand2;
    cpu.r[rd] = (uint32_t)(result & 0xFFFFFFFF); // Lower 32 bits
    cpu.r[carry_out] = (uint32_t)(result >> 32); // Upper 32 bits
}

static void handle_umlal(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing UMLAL\n");

    // Perform the UMLAL operation (Unsigned Multiply-Accumulate Long)
    uint64_t result = (uint64_t)cpu.r[rn] * (uint64_t)operand2 + ((uint64_t)cpu.r[rd] | ((uint64_t)cpu.r[carry_out] << 32));
    cpu.r[rd] = (uint32_t)(result & 0xFFFFFFFF); // Lower 32 bits
    cpu.r[carry_out] = (uint32_t)(result >> 32); // Upper 32 bits
}

static void handle_smlal(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing SMLAL\n");

    // Perform the SMLAL operation (Signed Multiply-Accumulate Long)
    int64_t result = (int64_t)(int32_t)cpu.r[rn] * (int64_t)(int32_t)operand2 + ((int64_t)cpu.r[rd] | ((int64_t)cpu.r[carry_out] << 32));
    cpu.r[rd] = (uint32_t)(result & 0xFFFFFFFF); // Lower 32 bits
    cpu.r[carry_out] = (uint32_t)(result >> 32); // Upper 32 bits
}

// Status Register Access Instructions
static void handle_mrs(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing MRS\n");

    // Perform the MRS operation (Move PSR to Register)
    cpu.r[rd] = cpu.cpsr; // Move CPSR to destination register
}

static void handle_msr(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing MSR\n");

    // Perform the MSR operation (Move Register to PSR)
    cpu.cpsr = operand2; // Move operand to CPSR
}

// Coprocessor Instructions
static void handle_cdp(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing CDP\n");

    // Perform the CDP operation (Coprocessor Data Operation)
    // Placeholder: Implement coprocessor-specific logic here
    printf("CDP operation not yet implemented\n");
}

static void handle_ldc(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing LDC\n");

    // Perform the LDC operation (Load Coprocessor)
    uint32_t address = cpu.r[rn] + operand2;
    // Placeholder: Implement coprocessor-specific logic to load data from memory
    printf("LDC operation at address: 0x%08X\n", address);
    
}

static void handle_stc(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing STC\n");

    // Perform the STC operation (Store Coprocessor)
    uint32_t address = cpu.r[rn] + operand2;
    // Placeholder: Implement coprocessor-specific logic to store data to memory
    printf("STC operation at address: 0x%08X\n", address);
}

static void handle_mcr(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing MCR\n");

    // Perform the MCR operation (Move to Coprocessor)
    // Placeholder: Implement coprocessor-specific logic to move data to coprocessor
    printf("MCR operation not yet implemented\n");
}

static void handle_mrc(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing MRC\n");

    // Perform the MRC operation (Move from Coprocessor)
    // Placeholder: Implement coprocessor-specific logic to move data from coprocessor
    printf("MRC operation not yet implemented\n");
}

// Undefined Instructions
static void handle_undefined(uint32_t instruction) {
    printf("Executing Undefined Instruction: 0x%08X\n", instruction);
    // Implement logic for undefined instructions (e.g., raise exception)
}

// NOP Instruction
static void handle_nop(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    printf("Executing NOP\n");
}

// SWP and SWPB Instructions
static void handle_swp(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    // SWP: Swap word between register and memory
    uint32_t address = cpu.r[rn] + operand2; // Calculate memory address
    uint32_t temp = memory_read_32(address); // Read memory at address
    memory_write_32(address, cpu.r[rd]); // Write register value to memory
    cpu.r[rd] = temp; // Write memory value to register
    printf("SWP executed: rd=%d, rn=%d\n", rd, rn);
}

static void handle_swpb(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out) {
    // SWPB: Swap byte between register and memory
    uint32_t address = cpu.r[rn] + operand2; // Calculate memory address
    uint8_t temp = memory_read_8(address); // Read byte from memory at address
    memory_write_8(address, (uint8_t)cpu.r[rd]); // Write register value (byte) to memory
    cpu.r[rd] = temp; // Write memory value to register
    printf("SWPB executed: rd=%d, rn=%d\n", rd, rn);
}

// Populate the instruction table
Instruction instruction_table[] = {
    {0x0, 1, handle_and},
    {0x1, 1, handle_eor},
    {0x2, 1, handle_sub},
    {0x3, 1, handle_rsb},
    {0x4, 1, handle_add},
    {0x5, 1, handle_adc},
    {0x6, 1, handle_sbc},
    {0x7, 1, handle_rsc},
    {0x8, 1, handle_tst},
    {0x9, 1, handle_teq},
    {0xA, 1, handle_cmp},
    {0xB, 1, handle_cmn},
    {0xC, 1, handle_orr},
    {0xD, 1, handle_mov},
    {0xE, 1, handle_bic},
    {0xF, 1, handle_mvn},
    {0x10, 3, handle_ldr},
    {0x11, 3, handle_str},
    {0x12, 3, handle_ldrb},
    {0x13, 3, handle_strb},
    {0x14, 3, handle_ldrh},
    {0x15, 3, handle_strh},
    {0x16, 3, handle_ldrsb},
    {0x17, 3, handle_ldrsh},
    {0x18, 2, handle_b},
    {0x19, 2, handle_bl},
    {0x1A, 4, handle_mul},
    {0x1B, 4, handle_mla},
    {0x1C, 5, handle_umull},
    {0x1D, 5, handle_smull},
    {0x1E, 5, handle_umlal},
    {0x1F, 5, handle_smlal},
    {0x20, 2, handle_mrs},
    {0x21, 2, handle_msr},
    {0x22, 3, handle_cdp},
    {0x23, 3, handle_ldc},
    {0x24, 3, handle_stc},
    {0x25, 3, handle_mcr},
    {0x26, 3, handle_mrc},
    {0x27, 3, handle_swp}, // SWP (Swap word between register and memory)
    {0x28, 3, handle_swpb}, // SWPB (Swap byte between register and memory)
    {0xF0, 1, handle_nop}, // Updated opcode for NOP
    {0xFF, 0, handle_undefined}, // Example opcode for undefined instructions
};

// Initialize the hash table
static void init_instruction_hash_table() {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        instruction_hash_table[i] = NULL; // Set all entries to NULL
    }

    for (int i = 0; i < sizeof(instruction_table) / sizeof(Instruction); i++) {
        uint8_t opcode = instruction_table[i].opcode; // Extract opcode
        instruction_hash_table[opcode] = instruction_table[i].handler; // Populate handler
    }
}

// Initialize the cycle hash table
static void init_cycle_hash_table() {
    for (int i = 0; i < CYCLE_HASH_TABLE_SIZE; i++) {
        instruction_cycle_hash_table[i] = 0; // Set all entries to 0
    }

    for (int i = 0; i < sizeof(instruction_table) / sizeof(Instruction); i++) {
        uint8_t opcode = instruction_table[i].opcode; // Extract opcode
        instruction_cycle_hash_table[opcode] = instruction_table[i].cycles; // Populate cycle count
    }
}


// Decode and execute instruction using the hash table
static uint32_t decode_and_execute(uint32_t instruction) {
    uint8_t condition = (instruction >> 28) & 0xF; // Extract condition code

    // Check condition codes for execution
    if (!check_condition_codes(condition)) {
        return 0; // Instruction not executed
    }

    uint8_t opcode = (instruction >> 24) & 0xFF; // Extract the first 8 bits as opcode
    uint8_t rd = (instruction >> 12) & 0xF;     // Destination register
    uint8_t rn = (instruction >> 16) & 0xF;     // First operand register
    uint32_t operand2;
    uint8_t carry_out = 0; // Carry flag update

    // Determine if operand2 is a register or immediate
    if (instruction & (1 << 25)) { // Operand2 is a register
        uint8_t rm = instruction & 0xF; // Second operand register
        uint8_t shift_type = (instruction >> 5) & 0x3; // Shift type
        uint8_t shift_amount = (instruction >> 7) & 0x1F; // Shift amount

        // Apply shift to the register value
        switch (shift_type) {
            case 0: // Logical left
                operand2 = cpu.r[rm] << shift_amount;
                carry_out = (cpu.r[rm] >> (32 - shift_amount)) & 1;
                break;
            case 1: // Logical right
                operand2 = cpu.r[rm] >> shift_amount;
                carry_out = (cpu.r[rm] >> (shift_amount - 1)) & 1;
                break;
            case 2: // Arithmetic right
                operand2 = (int32_t)cpu.r[rm] >> shift_amount;
                carry_out = (cpu.r[rm] >> (shift_amount - 1)) & 1;
                break;
            case 3: // Rotate right
                operand2 = (cpu.r[rm] >> shift_amount) | (cpu.r[rm] << (32 - shift_amount));
                carry_out = (cpu.r[rm] >> (shift_amount - 1)) & 1;
                break;
            default:
                operand2 = cpu.r[rm]; // No shift
                break;
        }
    } else { // Operand2 is an immediate value
        operand2 = instruction & 0xFFF;
    }

    if (instruction_hash_table[opcode] != NULL) {
        instruction_hash_table[opcode](rd, rn, operand2, carry_out);

        // Check the S bit (bit 20) to determine if CPSR flags should be updated
        if ((instruction & (1 << 20)) && opcode != 0x08 && opcode != 0x09 && opcode != 0x0A && opcode != 0x0B) { // Example opcodes for TST, TEQ, CMP, CMN
            update_cpsr_flags(cpu.r[rd], carry_out);
        }

        return instruction_cycle_hash_table[opcode];
    }

    printf("Illegal instruction: 0x%08X\n", instruction);
    return 0; // Illegal instructions consume 0 cycles
}

// Helper function to check condition codes
static int check_condition_codes(uint8_t condition) {
    switch (condition) {
        case 0: // EQ: Equal (Z == 1)
            return (cpu.cpsr & (1 << 30)) != 0;
        case 1: // NE: Not Equal (Z == 0)
            return (cpu.cpsr & (1 << 30)) == 0;
        case 2: // CS/HS: Carry Set/Unsigned Higher or Same (C == 1)
            return (cpu.cpsr & (1 << 29)) != 0;
        case 3: // CC/LO: Carry Clear/Unsigned Lower (C == 0)
            return (cpu.cpsr & (1 << 29)) == 0;
        case 4: // MI: Negative (N == 1)
            return (cpu.cpsr & (1 << 31)) != 0;
        case 5: // PL: Positive or Zero (N == 0)
            return (cpu.cpsr & (1 << 31)) == 0;
        case 6: // VS: Overflow (V == 1)
            return (cpu.cpsr & (1 << 28)) != 0;
        case 7: // VC: No Overflow (V == 0)
            return (cpu.cpsr & (1 << 28)) == 0;
        case 8: // HI: Unsigned Higher (C == 1 && Z == 0)
            return ((cpu.cpsr & (1 << 29)) != 0) && ((cpu.cpsr & (1 << 30)) == 0);
        case 9: // LS: Unsigned Lower or Same (C == 0 || Z == 1)
            return ((cpu.cpsr & (1 << 29)) == 0) || ((cpu.cpsr & (1 << 30)) != 0);
        case 10: // GE: Signed Greater or Equal (N == V)
            return ((cpu.cpsr & (1 << 31)) >> 31) == ((cpu.cpsr & (1 << 28)) >> 28);
        case 11: // LT: Signed Less Than (N != V)
            return ((cpu.cpsr & (1 << 31)) >> 31) != ((cpu.cpsr & (1 << 28)) >> 28);
        case 12: // GT: Signed Greater Than (Z == 0 && N == V)
            return ((cpu.cpsr & (1 << 30)) == 0) && (((cpu.cpsr & (1 << 31)) >> 31) == ((cpu.cpsr & (1 << 28)) >> 28));
        case 13: // LE: Signed Less Than or Equal (Z == 1 || N != V)
            return ((cpu.cpsr & (1 << 30)) != 0) || (((cpu.cpsr & (1 << 31)) >> 31) != ((cpu.cpsr & (1 << 28)) >> 28));
        case 14: // AL: Always (no condition)
            return 1;
        default:
            return 0; // Invalid condition
    }
}

// Centralized CPSR update logic in `decode_and_execute`
static void update_cpsr_flags(uint32_t result, uint8_t carry_out) {
    if (result == 0) {
        cpu.cpsr |= (1 << 30); // Set Zero flag if result is zero
    } else {
        cpu.cpsr &= ~(1 << 30); // Clear Zero flag
    }
    if (result & (1 << 31)) {
        cpu.cpsr |= (1 << 31); // Set Negative flag if result is negative
    } else {
        cpu.cpsr &= ~(1 << 31); // Clear Negative flag
    }
    if (carry_out) {
        cpu.cpsr |= (1 << 29); // Set Carry flag if applicable
    } else {
        cpu.cpsr &= ~(1 << 29); // Clear Carry flag
    }
}

// Placeholder for interrupt handling
static int check_interrupts() {
    // Implement interrupt checking logic
    return 0;
}

static void handle_interrupts() {
    // Implement interrupt handling logic
}
