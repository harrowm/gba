#ifndef INSTRUCTION_TRACER_WITH_MEMORY_H
#define INSTRUCTION_TRACER_WITH_MEMORY_H

#include <cstdint>
#include <cstdio>

// Forward declaration
class Memory;

class InstructionTracerWithMemory {
private:
    FILE* trace_file;
    uint64_t instruction_count;
    uint32_t max_instructions;
    bool enabled;
    Memory* memory;
    
    struct MemoryLocation {
        uint32_t address;
        const char* name;
    };
    
    static constexpr MemoryLocation MONITORED_LOCATIONS[] = {
        {0x04000000, "DISPCNT"},
        {0x04000004, "DISPSTAT"},
        {0x04000006, "VCOUNT"},
        {0x04000200, "IE"},
        {0x04000202, "IF"},
        {0x04000208, "IME"},
        {0x03007FFC, "IRQ_HANDLER"},
        {0x03007FF8, "IRQ_SP-4"},
    };
    
public:
    InstructionTracerWithMemory() : trace_file(nullptr), instruction_count(0), 
                                     max_instructions(0), enabled(false), memory(nullptr) {}
    
    ~InstructionTracerWithMemory() {
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
            for (const auto& loc : MONITORED_LOCATIONS) {
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
        return enabled && instruction_count < max_instructions;
    }
    
    void traceInstruction(uint32_t* regs, uint32_t cpsr);
};

#endif // INSTRUCTION_TRACER_WITH_MEMORY_H
