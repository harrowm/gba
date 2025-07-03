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

    void updateCPSRFlags(uint32_t result, uint8_t carryOut);
    
    static constexpr uint32_t FLAG_N = 1 << 31; // Negative flag
    static constexpr uint32_t FLAG_Z = 1 << 30; // Zero flag
    static constexpr uint32_t FLAG_C = 1 << 29; // Carry flag
    static constexpr uint32_t FLAG_V = 1 << 28; // Overflow flag
    static constexpr uint32_t FLAG_T = 1 << 5;  // Thumb mode flag
    static constexpr uint32_t FLAG_E = 1 << 9;  // Endianness flag
    
    void setFlag(uint32_t flag);
    void clearFlag(uint32_t flag);
    bool checkFlag(uint32_t flag) const;

    void setCPUState(const CPUState& state);
    CPUState getCPUState() const;
    void printCPUState() const;
};

#endif
