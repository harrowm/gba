#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef enum {
    ARM_MODE = 0,
    THUMB_MODE = 1
} CPUMode;

// CPU state structure
typedef struct {
    uint32_t r[16];        // General-purpose registers (R0-R15)
    uint32_t cpsr;          // Current Program Status Register
    CPUMode mode;           // ARM or Thumb mode
} CPUState;

extern CPUState cpu;

// Function declarations
void cpu_init();
void cpu_step(uint32_t cycles); // Step forward a specified number of CPU cycles
uint32_t fetch_instruction(uint32_t address);
void decode_and_execute(uint32_t instruction);

#endif // CPU_H
