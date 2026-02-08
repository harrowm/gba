#ifndef INSTRUCTION_TRACER_H
#define INSTRUCTION_TRACER_H

#include <cstdint>
#include <cstdio>

class InstructionTracer {
private:
    FILE* trace_file;
    uint64_t instruction_count;
    uint32_t max_instructions;
    bool enabled;
    
public:
    InstructionTracer() : trace_file(nullptr), instruction_count(0), max_instructions(0), enabled(false) {}
    
    ~InstructionTracer() {
        close();
    }
    
    void open(const char* filename, uint32_t max_inst = 1000) {
        trace_file = fopen(filename, "w");
        if (trace_file) {
            enabled = true;
            instruction_count = 0;
            max_instructions = max_inst;
            fprintf(trace_file, "GBA Emulator Instruction Trace\n");
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
        return enabled && instruction_count < max_instructions;
    }
    
    void traceInstruction(const uint32_t* regs, uint32_t cpsr) {
        if (!isEnabled()) {
            return;
        }
        
        instruction_count++;
        
        fprintf(trace_file, "\n======================================================================\n");
        fprintf(trace_file, "Instruction #%llu\n", instruction_count);
        fprintf(trace_file, "======================================================================\n");
        
        // Print registers in same format as mGBA trace
        fprintf(trace_file, " r0=0x%08X   r1=0x%08X   r2=0x%08X   r3=0x%08X\n",
                regs[0], regs[1], regs[2], regs[3]);
        fprintf(trace_file, " r4=0x%08X   r5=0x%08X   r6=0x%08X   r7=0x%08X\n",
                regs[4], regs[5], regs[6], regs[7]);
        fprintf(trace_file, " r8=0x%08X   r9=0x%08X  r10=0x%08X  r11=0x%08X\n",
                regs[8], regs[9], regs[10], regs[11]);
        fprintf(trace_file, "r12=0x%08X   sp=0x%08X   lr=0x%08X   pc=0x%08X\n",
                regs[12], regs[13], regs[14], regs[15]);
        
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
        
        fprintf(trace_file, "cpsr=0x%08X [N=%u Z=%u C=%u V=%u I=%u F=%u T=%u] Mode: %s\n",
                cpsr, N, Z, C, V, I, F, T, mode_name);
        
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

#endif // INSTRUCTION_TRACER_H
