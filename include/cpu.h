#ifndef CPU_H
#define CPU_H

#include <cassert>
#include "memory.h"
#include "interrupt.h"
#include "debug.h"
#include "thumb_cpu.h"
#include "utility_macros.h"
#include "instruction_tracer.h"
#include "instruction_memory_tracer.h"
#include <array>
#include <cstdint>

// Forward declarations
class ThumbCPU;
class ARMCPU;
class Scheduler;

class CPU {
    // Prevent accidental copies/moves (breaks memory reference)
    CPU(const CPU&) = delete;
    CPU& operator=(const CPU&) = delete;
    CPU(CPU&&) = delete;
    CPU& operator=(CPU&&) = delete;
public:
    // Accessors for User mode banked SP/LR
    uint32_t& bankedLRUser() { return banked_r14_usr; }


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
    Scheduler* scheduler; // Scheduler for cycle-accurate timing

    // Banked registers for privileged modes
    // FIQ mode banks R8-R14 (5 extra registers + SP + LR)
    uint32_t banked_r8_fiq, banked_r9_fiq, banked_r10_fiq, banked_r11_fiq, banked_r12_fiq;
    uint32_t banked_r13_fiq, banked_r14_fiq;
    // Other modes only bank R13 (SP) and R14 (LR)
    uint32_t banked_r13_svc, banked_r14_svc;
    uint32_t banked_r13_abt, banked_r14_abt;
    uint32_t banked_r13_irq, banked_r14_irq;
    uint32_t banked_r13_und, banked_r14_und;
    // User mode banked R8-R14 for correct restoration after FIQ mode
    uint32_t banked_r8_usr, banked_r9_usr, banked_r10_usr, banked_r11_usr, banked_r12_usr;
    uint32_t banked_r13_usr, banked_r14_usr;
    
    // Saved Program Status Registers (SPSR) for privileged modes
    uint32_t spsr_fiq;
    uint32_t spsr_svc;
    uint32_t spsr_abt;
    uint32_t spsr_irq;
    uint32_t spsr_und;

    ThumbCPU* thumbCPU; // Delegate for Thumb instructions
    ARMCPU* armCPU; // Delegate for ARM instructions
    
    // Instruction tracers for debugging
    InstructionTracer tracer;
    InstructionMemoryTracer memoryTracer;

    // CPU halt state (set by writing to HALTCNT, cleared on interrupt)
    bool halted;

    // Helper: get current mode
    Mode getMode() const { return static_cast<Mode>(cpsr & 0x1F); }

    // Accessors for R13 (SP) and R14 (LR) with banking
public:
    // Open bus prefetch: the value currently on the data bus from the CPU's
    // instruction pipeline.  On ARM7TDMI the 3-stage pipeline means the bus
    // holds the instruction at PC (i.e. current_instruction_address + 8 in
    // ARM mode, +4 in Thumb mode).  Updated at the start of each instruction
    // by ARMCPU / ThumbCPU and used by Memory for open bus reads.
    uint32_t openBusPrefetch = 0;
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

    // For test/debug: direct access to banked registers
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
    // Single instruction execution (for main loop)
    void executeOneInstruction();
    
    // Interrupt handling
    void handleInterrupt();
    bool checkPendingInterrupts();
    
    // HALT state management
    void halt() { halted = true; }
    void unhalt() { halted = false; }
    bool isHalted() const { return halted; }
    
    // Reset and initialization
    void reset();
    
    // Scheduler integration
    void setScheduler(Scheduler* sched) { scheduler = sched; }
    Scheduler* getScheduler() { return scheduler; }
    void advanceCycles(uint32_t cycles); // Advance scheduler by cycles

    std::array<uint32_t, 16>& R() { return registers; }
    uint32_t& CPSR() { return cpsr; }
    
    // Called after any CPSR write that may clear the I flag (bit 7).
    // Like mGBA's readCPSR hook, this re-checks pending interrupts
    // so that IRQs raised while I=1 get delivered promptly after I→0.
    void onCPSRWrite();
    // Instruction tracing for debugging
    void enableTracing(const char* filename, uint32_t max_instructions = 1000) {
        tracer.open(filename, max_instructions);
    }
    void enableMemoryTracing(const char* filename, uint32_t max_instructions = 5000) {
        memoryTracer.open(filename, &memory, max_instructions);
    }
    bool isMemoryTracingComplete() const {
        return memoryTracer.isComplete();
    }
    
    // SPSR (Saved Program Status Register) accessors
    uint32_t& SPSR() {
        switch (getMode()) {
            case FIQ: return spsr_fiq;
            case SVC: return spsr_svc;
            case ABT: return spsr_abt;
            case IRQ: return spsr_irq;
            case UND: return spsr_und;
            default:
                // In User/System mode, SPSR is unpredictable
                // On GBA, it appears to return CPSR
                DEBUG_ERROR("SPSR accessed in User/System mode - returning CPSR");
                return cpsr;
        }
    }
    
    // Set mode and swap banked registers as needed
    void setMode(Mode newMode) {
        DEBUG_INFO("setMode: ENTRY oldMode=" + std::to_string((int)getMode()) + ", newMode=" + std::to_string((int)newMode));
        DEBUG_INFO("Banked SP/LR: FIQ SP=0x" + debug_to_hex_string(banked_r13_fiq,8) + ", LR=0x" + debug_to_hex_string(banked_r14_fiq,8) +
                    ", SVC SP=0x" + debug_to_hex_string(banked_r13_svc,8) + ", LR=0x" + debug_to_hex_string(banked_r14_svc,8) +
                    ", ABT SP=0x" + debug_to_hex_string(banked_r13_abt,8) + ", LR=0x" + debug_to_hex_string(banked_r14_abt,8) +
                    ", IRQ SP=0x" + debug_to_hex_string(banked_r13_irq,8) + ", LR=0x" + debug_to_hex_string(banked_r14_irq,8) +
                    ", UND SP=0x" + debug_to_hex_string(banked_r13_und,8) + ", LR=0x" + debug_to_hex_string(banked_r14_und,8));
        Mode oldMode = getMode();
        
        // PHASE 3: Mode switch logging (DISABLED - didn't find issue)
        // static int modeSwitch_count = 0;
        // modeSwitch_count++;
        // if (modeSwitch_count % 10000 == 0 || 
        //     registers[13] < 0x02000000 || registers[13] > 0x04000000) {
        //     fprintf(stderr, "[MODE SWITCH #%d] 0x%02X->0x%02X: SP=0x%08X, LR=0x%08X\n",
        //             modeSwitch_count, (int)oldMode, (int)newMode, registers[13], registers[14]);
        // }
        
        // printf("[setMode] Called: oldMode=0x%02X, newMode=0x%02X, current LR=0x%08X\n", (int)oldMode, (int)newMode, registers[14]);
        DEBUG_INFO("setMode: BEFORE swap, mode=" + std::to_string((int)oldMode) + ", SP=0x" + debug_to_hex_string(registers[13], 8) + ", LR=0x" + debug_to_hex_string(registers[14], 8));
        DEBUG_INFO(std::string("setMode: switching from ") + std::to_string((int)oldMode) + " to " + std::to_string((int)newMode));
        assert((int)newMode >= 0x10 && (int)newMode <= 0x1F && "Invalid newMode in setMode");
        
        // ARM7TDMI Register Banking:
        // - FIQ mode: banks R8-R14 (has its own copies of R8-R12, R13, R14)
        // - SVC/IRQ/ABT/UND: bank only R13-R14 (SP and LR)
        // - USER/SYS: share all R0-R14 with each other
        // 
        // R8-R12 are ONLY banked for FIQ mode. When switching to/from FIQ,
        // we need to save/restore R8-R12. For all other mode switches,
        // R8-R12 are shared and should NOT be modified.
        
        // Save current mode's banked registers
        switch (oldMode) {
            case FIQ: 
                // FIQ banks R8-R14 - save to FIQ bank
                banked_r8_fiq = registers[8];
                banked_r9_fiq = registers[9];
                banked_r10_fiq = registers[10];
                banked_r11_fiq = registers[11];
                banked_r12_fiq = registers[12];
                banked_r13_fiq = registers[13];
                banked_r14_fiq = registers[14];
                // When leaving FIQ, we need to restore R8-R12 from User bank
                // (since User/System share R8-R12 with non-FIQ modes)
                registers[8] = banked_r8_usr;
                registers[9] = banked_r9_usr;
                registers[10] = banked_r10_usr;
                registers[11] = banked_r11_usr;
                registers[12] = banked_r12_usr;
                break;
            case SVC: banked_r13_svc = registers[13]; banked_r14_svc = registers[14]; break;
            case ABT: banked_r13_abt = registers[13]; banked_r14_abt = registers[14]; break;
            case IRQ: banked_r13_irq = registers[13]; banked_r14_irq = registers[14]; break;
            case UND: banked_r13_und = registers[13]; banked_r14_und = registers[14]; break;
            case USER:
            case SYS: 
                // User/System only bank R13-R14 for the purpose of switching
                // R8-R12 are shared with all non-FIQ modes
                banked_r13_usr = registers[13]; 
                banked_r14_usr = registers[14];
                // Also update R8-R12 bank in case we later switch to FIQ
                banked_r8_usr = registers[8];
                banked_r9_usr = registers[9];
                banked_r10_usr = registers[10];
                banked_r11_usr = registers[11];
                banked_r12_usr = registers[12];
                break;
            default: break;
        }
        // Update CPSR mode bits BEFORE loading new banked registers
        cpsr = (cpsr & ~0x1F) | (uint32_t)newMode;
        // Load new mode's banked registers
        switch (newMode) {
            case FIQ:
                // FIQ restores R8-R14 from FIQ bank
                // First save current R8-R12 to User bank (for when we leave FIQ later)
                banked_r8_usr = registers[8];
                banked_r9_usr = registers[9];
                banked_r10_usr = registers[10];
                banked_r11_usr = registers[11];
                banked_r12_usr = registers[12];
                // Now load FIQ's banked registers
                registers[8] = banked_r8_fiq;
                registers[9] = banked_r9_fiq;
                registers[10] = banked_r10_fiq;
                registers[11] = banked_r11_fiq;
                registers[12] = banked_r12_fiq;
                registers[13] = banked_r13_fiq;
                registers[14] = banked_r14_fiq;
                break;
            case SVC: 
                registers[13] = banked_r13_svc;
                registers[14] = banked_r14_svc;
                break;
            case ABT:
                registers[13] = banked_r13_abt;
                registers[14] = banked_r14_abt;
                break;
            case IRQ:
                registers[13] = banked_r13_irq;
                registers[14] = banked_r14_irq;
                break;
            case UND:
                registers[13] = banked_r13_und;
                registers[14] = banked_r14_und;
                break;
            case USER:
            case SYS:
                // User/System only restore R13-R14
                // R8-R12 are already correct (shared with non-FIQ modes)
                registers[13] = banked_r13_usr;
                registers[14] = banked_r14_usr;
                break;
            default: break;
        }
        DEBUG_INFO("setMode: AFTER swap, mode=" + std::to_string((int)newMode) + ", SP=0x" + debug_to_hex_string(registers[13], 8) + ", LR=0x" + debug_to_hex_string(registers[14], 8));
    }
    
    // Set entire CPSR, handling mode change if needed (for LDM ^ instruction)
    void setCPSR(uint32_t newCPSR) {
        Mode oldMode = getMode();
        Mode newMode = static_cast<Mode>(newCPSR & 0x1F);
        
        // If mode is changing, use setMode to handle register banking
        if (oldMode != newMode) {
            setMode(newMode);
        }
        
        // Now set the full CPSR (flags, T bit, etc.)
        cpsr = newCPSR;
    }
    
    Memory& getMemory() { return memory; }
    
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
    

    FORCE_INLINE void setFlag(uint32_t flag) {
        cpsr |= flag;
    }

    FORCE_INLINE void clearFlag(uint32_t flag) {
        cpsr &= ~flag;
    }

    FORCE_INLINE bool getFlag(uint32_t flag) const {
        return (cpsr & flag) != 0;
    }

    const std::array<uint32_t, 16>& R() const { return registers; }
    const uint32_t& CPSR() const { return cpsr; }
};



#endif
