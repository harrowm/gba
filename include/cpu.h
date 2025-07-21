#ifndef CPU_H
#define CPU_H

#include <cassert>
#include "memory.h"
#include "interrupt.h"
#include "debug.h"
#include "thumb_cpu.h"
#include "arm_cpu.h"
#include "timing.h"
#include <array>
#include <cstdint>

// Forward definition of ThumbCPU and ARMCPU
class ThumbCPU;
class ARMCPU;

class CPU {
    // Prevent accidental copies/moves (breaks memory reference)
    CPU(const CPU&) = delete;
    CPU& operator=(const CPU&) = delete;
    CPU(CPU&&) = delete;
    CPU& operator=(CPU&&) = delete;
public:
    struct CPUState {
        std::array<uint32_t, 16> registers; // General-purpose registers
        uint32_t cpsr; // Current Program Status Register
    };

    // ARM privileged mode banked registers
    // FIQ: R8_fiq-R14_fiq, others: R13/R14 for each mode
    enum Mode {
        USER = 0x10,
        FIQ = 0x11,
        IRQ = 0x12,
        SVC = 0x13,
        ABT = 0x17,
        UND = 0x1B,
        SYS = 0x1F
    };


private:
    Memory& memory;
    InterruptController& interruptController;
    std::array<uint32_t, 16> registers; // Shared registers (User/System)
    uint32_t cpsr; // Current Program Status Register
    TimingState timing; // Timing state for cycle-driven execution

    // Banked registers for privileged modes
    // R13 (SP) and R14 (LR) for each mode except User/System
    uint32_t banked_r13_fiq, banked_r14_fiq;
    uint32_t banked_r13_svc, banked_r14_svc;
    uint32_t banked_r13_abt, banked_r14_abt;
    uint32_t banked_r13_irq, banked_r14_irq;
    uint32_t banked_r13_und, banked_r14_und;
    // User mode banked SP/LR for correct restoration after exceptions
    uint32_t banked_r13_usr, banked_r14_usr;

    ThumbCPU* thumbCPU; // Delegate for Thumb instructions
    ARMCPU* armCPU; // Delegate for ARM instructions

    // Helper: get current mode
    Mode getMode() const { return static_cast<Mode>(cpsr & 0x1F); }

    // Accessors for R13 (SP) and R14 (LR) with banking
public:
    uint32_t& SP() {
        switch (getMode()) {
            case FIQ: return banked_r13_fiq;
            case SVC: return banked_r13_svc;
            case ABT: return banked_r13_abt;
            case IRQ: return banked_r13_irq;
            case UND: return banked_r13_und;
            default:  return registers[13];
        }
    }
    uint32_t& LR() {
        switch (getMode()) {
            case FIQ: return banked_r14_fiq;
            case SVC: return banked_r14_svc;
            case ABT: return banked_r14_abt;
            case IRQ: return banked_r14_irq;
            case UND: return banked_r14_und;
            default:  return registers[14];
        }
    }

    // For test/debug: direct access to all banks
    uint32_t& bankedSP(Mode m) {
        switch (m) {
            case FIQ: return banked_r13_fiq;
            case SVC: return banked_r13_svc;
            case ABT: return banked_r13_abt;
            case IRQ: return banked_r13_irq;
            case UND: return banked_r13_und;
            default:  return registers[13];
        }
    }
    uint32_t& bankedLR(Mode m) {
        switch (m) {
            case FIQ: return banked_r14_fiq;
            case SVC: return banked_r14_svc;
            case ABT: return banked_r14_abt;
            case IRQ: return banked_r14_irq;
            case UND: return banked_r14_und;
            default:  return registers[14];
        }
    }

public:
    CPU(Memory& mem, InterruptController& interrupt);
    ~CPU();

    void execute(uint32_t cycles);
    void executeWithTiming(uint32_t cycles); // New timing-aware execution

    std::array<uint32_t, 16>& R() { return registers; }
    uint32_t& CPSR() { return cpsr; }
    // Set mode and swap banked registers as needed
    void setMode(Mode newMode) {
        DEBUG_INFO("setMode: ENTRY oldMode=" + std::to_string((int)getMode()) + ", newMode=" + std::to_string((int)newMode));
        DEBUG_INFO("Banked SP/LR: FIQ SP=0x" + debug_to_hex_string(banked_r13_fiq,8) + ", LR=0x" + debug_to_hex_string(banked_r14_fiq,8) +
                    ", SVC SP=0x" + debug_to_hex_string(banked_r13_svc,8) + ", LR=0x" + debug_to_hex_string(banked_r14_svc,8) +
                    ", ABT SP=0x" + debug_to_hex_string(banked_r13_abt,8) + ", LR=0x" + debug_to_hex_string(banked_r14_abt,8) +
                    ", IRQ SP=0x" + debug_to_hex_string(banked_r13_irq,8) + ", LR=0x" + debug_to_hex_string(banked_r14_irq,8) +
                    ", UND SP=0x" + debug_to_hex_string(banked_r13_und,8) + ", LR=0x" + debug_to_hex_string(banked_r14_und,8));
        Mode oldMode = getMode();
        DEBUG_INFO("setMode: BEFORE swap, mode=" + std::to_string((int)oldMode) + ", SP=0x" + debug_to_hex_string(registers[13], 8) + ", LR=0x" + debug_to_hex_string(registers[14], 8));
        DEBUG_INFO(std::string("setMode: switching from ") + std::to_string((int)oldMode) + " to " + std::to_string((int)newMode));
        assert((int)newMode >= 0x10 && (int)newMode <= 0x1F && "Invalid newMode in setMode");
        if (oldMode == newMode) return;
        // Save current SP/LR to bank
        switch (oldMode) {
            case FIQ: banked_r13_fiq = registers[13]; banked_r14_fiq = registers[14]; break;
            case SVC: banked_r13_svc = registers[13]; banked_r14_svc = registers[14]; break;
            case ABT: banked_r13_abt = registers[13]; banked_r14_abt = registers[14]; break;
            case IRQ: banked_r13_irq = registers[13]; banked_r14_irq = registers[14]; break;
            case UND: banked_r13_und = registers[13]; banked_r14_und = registers[14]; break;
            case USER:
            case SYS: banked_r13_usr = registers[13]; banked_r14_usr = registers[14]; break;
            default: break;
        }
        // Load new SP/LR from bank
        switch (newMode) {
            case FIQ: registers[13] = banked_r13_fiq; registers[14] = banked_r14_fiq; break;
            case SVC: registers[13] = banked_r13_svc; registers[14] = banked_r14_svc; break;
            case ABT: registers[13] = banked_r13_abt; registers[14] = banked_r14_abt; break;
            case IRQ: registers[13] = banked_r13_irq; registers[14] = banked_r14_irq; break;
            case UND: registers[13] = banked_r13_und; registers[14] = banked_r14_und; break;
            case USER:
            case SYS: registers[13] = banked_r13_usr; registers[14] = banked_r14_usr; break;
            default: break;
        }
        DEBUG_INFO("setMode: AFTER swap, mode=" + std::to_string((int)newMode) + ", SP=0x" + debug_to_hex_string(registers[13], 8) + ", LR=0x" + debug_to_hex_string(registers[14], 8));
        // Update CPSR mode bits
        cpsr = (cpsr & ~0x1F) | (uint32_t)newMode;
    }
    Memory& getMemory() { return memory; }
    TimingState& getTiming() { return timing; } // Access to timing state
    
    InterruptController& getInterruptController() { return interruptController; }
    
    // Access to CPU delegates
    ARMCPU& getARMCPU() { return *armCPU; }
    ThumbCPU& getThumbCPU() { return *thumbCPU; }

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

    CPUState getCPUState() const;
    void printCPUState() const;

    const std::array<uint32_t, 16>& R() const { return registers; }
    const uint32_t& CPSR() const { return cpsr; }
};



#endif
