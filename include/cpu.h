#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include "interrupt.h"
#include "debug.h"
#include "thumb_cpu.h"
#include "arm_cpu.h"
#include <array>
#include <cstdint>

// Forward definition of ThumbCPU and ARMCPU
class ThumbCPU;
class ARMCPU;

class CPU {
public:
    struct CPUState {
        std::array<uint32_t, 16> registers; // General-purpose registers
        uint32_t cpsr; // Current Program Status Register
    };

private:
    Memory& memory;
    InterruptController& interruptController;
    std::array<uint32_t, 16> registers; // Shared registers
    uint32_t cpsr; // Current Program Status Register

    ThumbCPU* thumbCPU; // Delegate for Thumb instructions
    ARMCPU* armCPU; // Delegate for ARM instructions


public:
    CPU(Memory& mem, InterruptController& interrupt);
    ~CPU();

    void execute(uint32_t cycles);

    std::array<uint32_t, 16>& R() { return registers; }
    uint32_t& CPSR() { return cpsr; }
    Memory& getMemory() { return memory; }
    
    InterruptController& getInterruptController() { return interruptController; }

    // Update declarations for member functions to update CPSR flags
    // constexpr functions have to be declared in the same file as their definition
    
    constexpr void updateZFlag(uint32_t result) {
        cpsr = (result == 0) ? (cpsr | FLAG_Z) : (cpsr & ~FLAG_Z);
    }

    constexpr void updateNFlag(uint32_t result) {
        cpsr = (result & (1 << 31)) ? (cpsr | FLAG_N) : (cpsr & ~FLAG_N);
    }

    constexpr void updateCFlagSub(uint32_t op1, uint32_t op2) {
        cpsr = (op1 >= op2) ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C);
    }

    constexpr void updateCFlagAdd(uint32_t op1, uint32_t op2) {
        cpsr = (op1 > (UINT32_MAX - op2)) ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C);
    };

    constexpr void updateCFlagSubWithCarry(uint32_t op1, uint32_t op2) {
        cpsr = (op1 >= (op2 + (1 - getFlag(FLAG_C)))) ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C);
    }

    constexpr void updateCFlagAddWithCarry(uint32_t op1, uint32_t op2) {
        cpsr = (op1 > (UINT32_MAX - op2 - getFlag(FLAG_C))) ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C);
    };

    constexpr void updateCFlagShiftLSL(uint32_t value, uint8_t shift_amount) {
        // For logical shifts, the carry flag is set to the last bit shifted out
        if (shift_amount > 0) {  
            cpsr = ((value >> (32 - shift_amount)) & 1) ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C);
        }
    }

    constexpr void updateCFlagShiftLSR(uint32_t value, uint8_t shift_amount) {
        if (shift_amount == 0) {
            // Special case: LSR with shift amount 0 means shift by 32
            cpsr = (value & (1 << 31)) ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C); // MSB becomes carry
        } else {
            // Standard LSR behavior
            cpsr = ((value >> (shift_amount - 1)) & 1) ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C);
        }
    }

    constexpr void updateCFlagShiftASR(uint32_t value, uint8_t shift_amount) {
        if (shift_amount == 0) {
            // Special case: ASR with shift amount 0 means shift by 32
            cpsr = (value & (1 << 31)) ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C); // MSB becomes carry
        } else {
            // Standard ASR behavior
            cpsr = ((value >> (shift_amount - 1)) & 1) ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C);
        }
    }

    constexpr void updateCFlagShiftRight(uint32_t value, uint8_t shift_amount, char shift_type) {
        if (shift_amount == 0) {
            return; // Do not alter the carry flag for other cases
        }

        switch (shift_type) {
            case 'L': // Logical Shift Right (LSR)
                cpsr = (shift_amount >= 32) ? (cpsr & ~FLAG_C) : 
                       ((value >> (shift_amount - 1)) & 1) ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C);
                break;

            case 'A': // Arithmetic Shift Right (ASR)
                if (shift_amount >= 32) {
                    cpsr = (value & (1 << 31)) ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C); // Sign bit
                } else {
                    cpsr = ((value >> (shift_amount - 1)) & 1) ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C);
                }
                break;

            case 'O': // Rotate Right (ROR)
                if (shift_amount == 0) {
                    // Rotate Right (ROR) with shift amount 0 is treated as RRX
                    cpsr = (value & 1) ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C); // LSB becomes carry
                } else {
                    cpsr = ((value >> (shift_amount - 1)) & 1) ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C);
                }
                break;

            case 'X': // Rotate Right with Extend (RRX)
                cpsr = (value & 1) ? (cpsr | FLAG_C) : (cpsr & ~FLAG_C); // LSB becomes carry
                break;

            default:
                // Invalid shift type
                break;
        }
    }

    constexpr void updateVFlag(uint32_t op1, uint32_t op2, uint32_t result) {
        cpsr = (((op1 ^ result) & (op2 ^ result) & (1 << 31)) != 0) ? (cpsr | FLAG_V) : (cpsr & ~FLAG_V);
    }
    
    constexpr void updateVFlagSub(uint32_t op1, uint32_t op2, uint32_t result) {
        cpsr = ((((op1 ^ op2) & (op1 ^ result)) & (1 << 31)) != 0) ? (cpsr | FLAG_V) : (cpsr & ~FLAG_V);
    }
    
    static constexpr uint32_t FLAG_N = 1 << 31; // Negative flag
    static constexpr uint32_t FLAG_Z = 1 << 30; // Zero flag
    static constexpr uint32_t FLAG_C = 1 << 29; // Carry flag
    static constexpr uint32_t FLAG_V = 1 << 28; // Overflow flag
    static constexpr uint32_t FLAG_T = 1 << 5;  // Thumb mode flag
    static constexpr uint32_t FLAG_E = 1 << 9;  // Endianness flag
    
    void setFlag(uint32_t flag);
    void clearFlag(uint32_t flag);
    bool getFlag(uint32_t flag) const;

    void setCPUState(const CPUState& state);
    CPUState getCPUState() const;
    void printCPUState() const;

    const std::array<uint32_t, 16>& R() const { return registers; }
    const uint32_t& CPSR() const { return cpsr; }
};



#endif
