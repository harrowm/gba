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

// CPSR flag constants
#define CPSR_N_FLAG (1 << 31) // Negative flag
#define CPSR_Z_FLAG (1 << 30) // Zero flag
#define CPSR_C_FLAG (1 << 29) // Carry flag
#define CPSR_V_FLAG (1 << 28) // Overflow flag
#define CPSR_E_FLAG (1 << 9)  // Endianness flag
#define CPSR_T_FLAG (1 << 5)  // Thumb mode flag

// Function declarations
void cpu_init();
void cpu_step(uint32_t cycles); // Step forward a specified number of CPU cycles
int check_condition_codes(uint8_t condition);
void update_cpsr_flags(uint32_t result, uint8_t carry_out);
// Function to set the CPU mode
void set_cpu_mode(CPUState* state, CPUMode mode);
// Function to get the current CPU mode from a given CPU state
CPUMode get_cpu_state(CPUState* state);
#endif // CPU_H
