#include "instruction_tracer_with_memory.h"
#include "memory.h"

void InstructionTracerWithMemory::traceInstruction(uint32_t* regs, uint32_t cpsr) {
    if (!isEnabled() || !memory) {
        return;
    }
    
    instruction_count++;
    
    fprintf(trace_file, "\n======================================================================\n");
    fprintf(trace_file, "Instruction #%llu\n", instruction_count);
    fprintf(trace_file, "======================================================================\n");
    fprintf(trace_file, "PC: 0x%08X\n", regs[15]);
    
    // Print registers (abbreviated for readability)
    fprintf(trace_file, "Registers: r0=0x%08X r1=0x%08X r2=0x%08X r3=0x%08X ", 
            regs[0], regs[1], regs[2], regs[3]);
    fprintf(trace_file, "sp=0x%08X lr=0x%08X\n", regs[13], regs[14]);
    
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
    
    fprintf(trace_file, "CPSR: 0x%08X [N=%d Z=%d C=%d V=%d I=%d F=%d T=%d] Mode: %s\n",
            cpsr, N, Z, C, V, I, F, T, mode_name);
    
    // Read and log key memory locations
    fprintf(trace_file, "\nMemory:\n");
    for (const auto& loc : MONITORED_LOCATIONS) {
        uint32_t value = memory->read32(loc.address);
        fprintf(trace_file, "  [%-12s] 0x%08X = 0x%08X\n", loc.name, loc.address, value);
    }
    
    // Stop tracing after max instructions
    if (instruction_count >= max_instructions) {
        fprintf(trace_file, "\nTrace complete! %u instructions traced.\n", max_instructions);
        fflush(trace_file);
        enabled = false;
    } else {
        fflush(trace_file);  // Flush after each instruction for safety
    }
}
