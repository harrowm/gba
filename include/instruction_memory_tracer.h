#ifndef INSTRUCTION_MEMORY_TRACER_H
#define INSTRUCTION_MEMORY_TRACER_H

#include <cstdint>
#include <cstdio>
#include "memory.h"

// Forward declaration
class Scheduler;

class InstructionMemoryTracer {
private:
    FILE* trace_file;
    uint64_t instruction_count;
    uint32_t max_instructions;
    bool enabled;
    Memory* memory;
    
    // Key memory locations to monitor (same as mGBA trace)
    struct MemoryLocation {
        uint32_t address;
        const char* name;
        uint8_t size;  // 1=8bit, 2=16bit, 4=32bit
    };
    
    static constexpr MemoryLocation MEMORY_LOCATIONS[] = {
        {0x04000000, "DISPCNT",      4},  // 32-bit register
        {0x04000004, "DISPSTAT",     2},  // 16-bit register
        {0x04000006, "VCOUNT",       2},  // 16-bit register
        {0x04000100, "TM0CNT_L",     2},  // Timer 0 Counter (16-bit)
        {0x04000102, "TM0CNT_H",     2},  // Timer 0 Control (16-bit)
        {0x04000104, "TM1CNT_L",     2},  // Timer 1 Counter (16-bit)
        {0x04000106, "TM1CNT_H",     2},  // Timer 1 Control (16-bit)
        {0x040000BA, "DMA0CNT_H",    2},  // DMA 0 Control (16-bit)
        {0x040000C6, "DMA1CNT_H",    2},  // DMA 1 Control (16-bit)
        {0x040000D2, "DMA2CNT_H",    2},  // DMA 2 Control (16-bit)
        {0x040000DE, "DMA3CNT_H",    2},  // DMA 3 Control (16-bit)
        {0x04000200, "IE",           2},  // Interrupt Enable (16-bit)
        {0x04000202, "IF",           2},  // Interrupt Flag (16-bit)
        {0x04000208, "IME",          4},  // Interrupt Master Enable (32-bit)
        {0x04000300, "POSTFLG",      1},  // Post Boot Flag (8-bit)
        {0x03007FFC, "IRQ_HANDLER",  4},  // 32-bit pointer
        {0x03007FF8, "IRQ_SP-4",     4}   // 32-bit value
    };
    
public:
    InstructionMemoryTracer() : trace_file(nullptr), instruction_count(0), max_instructions(0), enabled(false), memory(nullptr) {}
    
    ~InstructionMemoryTracer() {
        close();
    }
    
    void open(const char* filename, Memory* mem, uint32_t max_inst = 5000) {
        trace_file = fopen(filename, "w");
        if (trace_file) {
            enabled = true;
            instruction_count = 0;
            max_instructions = max_inst;
            memory = mem;
            
            fprintf(trace_file, "GBA Emulator Memory Trace\n");
            fprintf(trace_file, "============================================================\n");
            fprintf(trace_file, "Monitoring memory locations:\n");
            for (const auto& loc : MEMORY_LOCATIONS) {
                fprintf(trace_file, "  0x%08X: %s\n", loc.address, loc.name);
            }
            fprintf(trace_file, "============================================================\n\n");
        }
    }
    
    void close() {
        if (trace_file) {
            fclose(trace_file);
            trace_file = nullptr;
        }
        enabled = false;
    }
    
    bool isEnabled() const {
        return enabled && instruction_count < max_instructions && memory != nullptr;
    }
    
    bool isComplete() const {
        return instruction_count >= max_instructions;
    }
    
    void traceInstruction(uint32_t* regs, uint32_t cpsr, uint64_t current_cycle = 0) {
        if (!isEnabled()) {
            return;
        }
        
        instruction_count++;
        
        fprintf(trace_file, "\n======================================================================\n");
        fprintf(trace_file, "Instruction #%llu", instruction_count);
        if (current_cycle > 0) {
            fprintf(trace_file, " (Cycle: %llu)", current_cycle);
        }
        fprintf(trace_file, "\n");
        fprintf(trace_file, "======================================================================\n");
        
        // Print PC
        fprintf(trace_file, "PC: 0x%08X\n", regs[15]);
        
        // Print registers in compact form (like mGBA trace)
        fprintf(trace_file, " r0=0x%08X   r1=0x%08X   r2=0x%08X   r3=0x%08X\n",
                regs[0], regs[1], regs[2], regs[3]);
        fprintf(trace_file, " r4=0x%08X   r5=0x%08X   r6=0x%08X   r7=0x%08X\n",
                regs[4], regs[5], regs[6], regs[7]);
        fprintf(trace_file, " r8=0x%08X   r9=0x%08X  r10=0x%08X  r11=0x%08X\n",
                regs[8], regs[9], regs[10], regs[11]);
        fprintf(trace_file, "r12=0x%08X   sp=0x%08X   lr=0x%08X\n",
                regs[12], regs[13], regs[14]);
        
        // Decode CPSR flags
        uint32_t N = (cpsr >> 31) & 1;
        uint32_t Z = (cpsr >> 30) & 1;
        uint32_t C = (cpsr >> 29) & 1;
        uint32_t V = (cpsr >> 28) & 1;
        uint32_t I = (cpsr >> 7) & 1;
        uint32_t F = (cpsr >> 6) & 1;
        uint32_t T = (cpsr >> 5) & 1;
        uint32_t mode = cpsr & 0x1F;
        
        const char* mode_name;
        switch (mode) {
            case 0x10: mode_name = "User"; break;
            case 0x11: mode_name = "FIQ"; break;
            case 0x12: mode_name = "IRQ"; break;
            case 0x13: mode_name = "Supervisor"; break;
            case 0x17: mode_name = "Abort"; break;
            case 0x1B: mode_name = "Undefined"; break;
            case 0x1F: mode_name = "System"; break;
            default: mode_name = "Unknown"; break;
        }
        
        fprintf(trace_file, "cpsr=0x%08X [N=%d Z=%d C=%d V=%d I=%d F=%d T=%d] Mode: %s\n",
                cpsr, N, Z, C, V, I, F, T, mode_name);
        
        // Read and print key memory locations
        // Disable wait cycles during tracer reads to avoid triggering scheduler events
        fprintf(trace_file, "\nMemory:\n");
        memory->setDisableWaitCycles(true);
        for (const auto& loc : MEMORY_LOCATIONS) {
            uint32_t value;
            if (loc.size == 1) {
                value = memory->read8(loc.address);
                fprintf(trace_file, "  [%-12s] 0x%08X = 0x%02X\n", loc.name, loc.address, value);
            } else if (loc.size == 2) {
                value = memory->read16(loc.address);
                fprintf(trace_file, "  [%-12s] 0x%08X = 0x%04X\n", loc.name, loc.address, value);
            } else {
                value = memory->read32(loc.address);
                fprintf(trace_file, "  [%-12s] 0x%08X = 0x%08X\n", loc.name, loc.address, value);
            }
        }
        memory->setDisableWaitCycles(false);
        
        // Stop tracing after max instructions
        if (instruction_count >= max_instructions) {
            fprintf(trace_file, "\nTrace complete! %u instructions traced.\n", max_instructions);
            fflush(trace_file);
            enabled = false;
        } else {
            fflush(trace_file);  // Flush after each instruction for safety
        }
    }
};

#endif // INSTRUCTION_MEMORY_TRACER_H
