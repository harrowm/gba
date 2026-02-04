#include "debug.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

// ============================================================================
// DEBUG SYSTEM globals
// ============================================================================
int g_debug_level = 0;        // Default debug level (0=OFF, can be changed at runtime)
int g_debug_file_mask = 0xFF; // Default file mask (enable all files, or adjust as needed)
bool g_disassemble_enabled = false;

// ============================================================================
// LOGGING SYSTEM globals
// ============================================================================
uint32_t g_log_categories = LOG_CAT_CRASH;  // Default: only crash detection enabled
uint32_t g_log_start_frame = 0;             // Start logging from frame 0
uint32_t g_log_end_frame = UINT32_MAX;      // End at max (effectively no limit)
uint32_t g_current_frame = 0;               // Current frame counter
uint32_t g_cpu_pc = 0;                      // Current CPU PC for debug tracking

// ============================================================================
// Helper functions for command-line parsing
// ============================================================================

uint32_t parse_log_categories(const char* arg) {
    uint32_t categories = LOG_CAT_NONE;
    
    // Skip --log= prefix if present
    if (strncmp(arg, "--log=", 6) == 0) {
        arg += 6;
    }
    
    char buffer[256];
    strncpy(buffer, arg, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* token = strtok(buffer, ",");
    while (token) {
        // Convert to uppercase for comparison
        for (char* p = token; *p; ++p) {
            if (*p >= 'a' && *p <= 'z') *p -= 32;
        }
        
        if (strcmp(token, "BIOS") == 0)         categories |= LOG_CAT_BIOS;
        else if (strcmp(token, "IRQ") == 0)     categories |= LOG_CAT_IRQ;
        else if (strcmp(token, "DMA") == 0)     categories |= LOG_CAT_DMA;
        else if (strcmp(token, "STACK") == 0)   categories |= LOG_CAT_STACK;
        else if (strcmp(token, "LDR") == 0)     categories |= LOG_CAT_LDR;
        else if (strcmp(token, "BL") == 0)      categories |= LOG_CAT_BL;
        else if (strcmp(token, "REGION") == 0)  categories |= LOG_CAT_REGION;
        else if (strcmp(token, "VRAM") == 0)    categories |= LOG_CAT_VRAM;
        else if (strcmp(token, "FEATURE") == 0) categories |= LOG_CAT_FEATURE;
        else if (strcmp(token, "REG") == 0)     categories |= LOG_CAT_REG;
        else if (strcmp(token, "TRACE") == 0)   categories |= LOG_CAT_TRACE;
        else if (strcmp(token, "TIMER") == 0)   categories |= LOG_CAT_TIMER;
        else if (strcmp(token, "CRASH") == 0)   categories |= LOG_CAT_CRASH;
        else if (strcmp(token, "ALL") == 0)     categories |= LOG_CAT_ALL;
        else if (strcmp(token, "NONE") == 0)    categories = LOG_CAT_NONE;
        else {
            fprintf(stderr, "Warning: Unknown log category '%s'\n", token);
        }
        
        token = strtok(nullptr, ",");
    }
    
    return categories;
}

void parse_log_frames(const char* arg) {
    // Skip --log-frames= prefix if present
    if (strncmp(arg, "--log-frames=", 13) == 0) {
        arg += 13;
    }
    
    // Parse format: START-END or just START
    const char* dash = strchr(arg, '-');
    if (dash) {
        g_log_start_frame = (uint32_t)atoi(arg);
        g_log_end_frame = (uint32_t)atoi(dash + 1);
    } else {
        // Single frame
        g_log_start_frame = (uint32_t)atoi(arg);
        g_log_end_frame = g_log_start_frame;
    }
    
    fprintf(stderr, "[LOG] Frame range: %u - %u\n", g_log_start_frame, g_log_end_frame);
}