#include "debug.h"
int g_debug_level = 2;        // Default debug level (can be changed at runtime)
int g_debug_file_mask = 0xFF; // Default file mask (enable all files, or adjust as needed)

// DEBUG: Print when debug.cpp is loaded
#include <cstdio>
static struct DebugCppPrint { DebugCppPrint() { fprintf(stderr, "[DEBUG] debug.cpp loaded\n"); } } _debug_cpp_print;