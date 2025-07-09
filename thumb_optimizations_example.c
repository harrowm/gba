// Fast C implementation of Thumb LSR operation
// This could be added to a new thumb_execute_optimizations.c file

#include "thumb_cpu.h"
#include "cpu.h"

// Fast C implementation of thumb_alu_lsr
void thumb_alu_lsr_fast(CPU* cpu, uint8_t rd, uint8_t rs) {
    uint32_t op1 = cpu->R()[rd];
    uint32_t shift_amount = cpu->R()[rs] & 0xFF;
    
    // Inline carry flag calculation for LSR
    if (shift_amount == 0) {
        // No shift, carry unchanged
    } else if (shift_amount <= 32) {
        // Set carry to the last bit shifted out
        cpu->setCFlag((op1 >> (shift_amount - 1)) & 1);
    } else {
        // Shift > 32, result is 0, carry is 0
        cpu->setCFlag(0);
    }
    
    // Perform the shift
    uint32_t result = (shift_amount >= 32) ? 0 : (op1 >> shift_amount);
    cpu->R()[rd] = result;
    
    // Update flags directly
    cpu->setZFlag(result == 0);
    cpu->setNFlag((result & 0x80000000) != 0);
}

// Similar optimizations for other hotspot ALU operations
void thumb_alu_eor_fast(CPU* cpu, uint8_t rd, uint8_t rs) {
    uint32_t op1 = cpu->R()[rd];
    uint32_t op2 = cpu->R()[rs];
    uint32_t result = op1 ^ op2;
    
    cpu->R()[rd] = result;
    cpu->setZFlag(result == 0);
    cpu->setNFlag((result & 0x80000000) != 0);
    // C and V flags unchanged for EOR
}

void thumb_alu_and_fast(CPU* cpu, uint8_t rd, uint8_t rs) {
    uint32_t op1 = cpu->R()[rd];
    uint32_t op2 = cpu->R()[rs];
    uint32_t result = op1 & op2;
    
    cpu->R()[rd] = result;
    cpu->setZFlag(result == 0);
    cpu->setNFlag((result & 0x80000000) != 0);
    // C and V flags unchanged for AND
}
