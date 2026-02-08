#ifndef SP_TRACE_H
#define SP_TRACE_H

#include <cstdint>
#include <cstdio>

// Global instruction counter shared between ARM and THUMB
extern uint64_t g_total_instruction_count;

// SP tracing interval (log SP every N instructions)
constexpr uint64_t SP_TRACE_INTERVAL = 10000;

// File for SP trace output
extern FILE* g_sp_trace_file;

// Close SP tracing
void close_sp_trace();

// Log SP at regular intervals (call from ARM and THUMB execute)
void trace_sp(uint32_t pc, uint32_t sp, const char* mode);

#endif // SP_TRACE_H
