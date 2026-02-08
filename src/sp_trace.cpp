#include "sp_trace.h"

// Global variables for SP tracing
uint64_t g_total_instruction_count = 0;
FILE* g_sp_trace_file = nullptr;

void close_sp_trace() {
    if (g_sp_trace_file) {
        fclose(g_sp_trace_file);
        g_sp_trace_file = nullptr;
    }
}

void trace_sp(uint32_t pc, uint32_t sp, const char* mode) {
    g_total_instruction_count++;
    
    if (g_sp_trace_file && (g_total_instruction_count % SP_TRACE_INTERVAL == 0)) {
        fprintf(g_sp_trace_file, "%llu,0x%08X,0x%08X,%s\n", 
                g_total_instruction_count, pc, sp, mode);
        fflush(g_sp_trace_file);
    }
}
