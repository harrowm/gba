#ifndef ARM_H
#define ARM_H

#include <stdint.h>

#define HASH_TABLE_SIZE 256
#define CYCLE_HASH_TABLE_SIZE 256

void arm_init_hash_tables();
uint32_t arm_decode_and_execute(uint32_t instruction);

// Hash table for ARM instruction handlers
void (*instruction_hash_table[HASH_TABLE_SIZE])(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out);

// Hash table for ARM instruction cycle counts
uint32_t instruction_cycle_hash_table[CYCLE_HASH_TABLE_SIZE];

typedef struct {
    uint32_t opcode;
    uint32_t cycles;
    void (*handler)(uint8_t rd, uint8_t rn, uint32_t operand2, uint8_t carry_out); // Updated to return void
} Instruction;

#endif // ARM_H