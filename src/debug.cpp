#include "debug.h"

#ifdef DEBUG_BUILD
// Define the global debug configuration variables
int g_debug_level = DEBUG_LEVEL_BASIC;  // Default to basic debug level
int g_debug_file_mask = DEBUG_FILE_MAIN | DEBUG_FILE_ARM | DEBUG_FILE_CPU | DEBUG_FILE_THUMB;  // Enable all file types by default
#endif
