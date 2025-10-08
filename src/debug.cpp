#include "debug.h"
int g_debug_level = 0;        // Default debug level (0=OFF, can be changed at runtime)
int g_debug_file_mask = 0xFF; // Default file mask (enable all files, or adjust as needed)

// Global flag to enable Capstone disassembly
bool g_disassemble_enabled = false;

// DEBUG: Print when debug.cpp is loaded
#include <cstdio>
static struct DebugCppPrint { DebugCppPrint() { fprintf(stderr, "[DEBUG] debug.cpp loaded\n"); } } _debug_cpp_print;