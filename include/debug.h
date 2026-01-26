#ifndef DEBUG_H
#define DEBUG_H

/*
 * Unified Debug & Logging System
 * 
 * Two separate systems:
 * 1. DEBUG_* macros - Development debugging (stripped in production builds)
 * 2. LOG_* macros   - Runtime logging with category and frame filtering
 *
 * === LOGGING SYSTEM (always available) ===
 * 
 * Categories (can be combined with bitwise OR):
 *   LOG_CAT_BIOS    - BIOS-related messages
 *   LOG_CAT_IRQ     - Interrupt handling
 *   LOG_CAT_DMA     - DMA transfers
 *   LOG_CAT_STACK   - Stack operations and corruption detection
 *   LOG_CAT_LDR     - Load instruction suspicious values
 *   LOG_CAT_BL      - Branch with link tracing
 *   LOG_CAT_REGION  - PC region transitions
 *   LOG_CAT_VRAM    - VRAM/palette writes
 *   LOG_CAT_FEATURE - Feature detection
 *   LOG_CAT_REG     - Register writes (IE/IF/IME etc)
 *   LOG_CAT_TRACE   - Instruction tracing
 *   LOG_CAT_TIMER   - Timer operations
 *   LOG_CAT_CRASH   - Crash/UNKNOWN region detection (always recommended)
 *
 * Usage:
 *   LOG_CAT(LOG_CAT_IRQ, "[IRQ] Handler at 0x%08X\n", addr);
 *
 * Configuration (via globals or command line):
 *   g_log_categories   - Bitmask of enabled categories (default: LOG_CAT_CRASH only)
 *   g_log_start_frame  - First frame to log (default: 0)
 *   g_log_end_frame    - Last frame to log (default: UINT32_MAX)
 *   g_current_frame    - Current frame number (updated by GBA::runFrame)
 *
 * Command line:
 *   --log=IRQ,STACK,LDR    Enable specific categories
 *   --log-frames=2230-2240 Only log within frame range
 *   --log-all              Enable all categories
 */

#include <cstdint>
#include <cstdio>
#include <cstring>

// ============================================================================
// LOGGING SYSTEM - Runtime logging with category and frame filtering
// ============================================================================

// Log categories (bit flags)
#define LOG_CAT_NONE    0x00000000
#define LOG_CAT_BIOS    0x00000001
#define LOG_CAT_IRQ     0x00000002
#define LOG_CAT_DMA     0x00000004
#define LOG_CAT_STACK   0x00000008
#define LOG_CAT_LDR     0x00000010
#define LOG_CAT_BL      0x00000020
#define LOG_CAT_REGION  0x00000040
#define LOG_CAT_VRAM    0x00000080
#define LOG_CAT_FEATURE 0x00000100
#define LOG_CAT_REG     0x00000200
#define LOG_CAT_TRACE   0x00000400
#define LOG_CAT_TIMER   0x00000800
#define LOG_CAT_CRASH   0x00001000
#define LOG_CAT_ALL     0xFFFFFFFF

// Global logging configuration (defined in debug.cpp)
extern uint32_t g_log_categories;
extern uint32_t g_log_start_frame;
extern uint32_t g_log_end_frame;
extern uint32_t g_current_frame;

// Main logging macro - checks category AND frame range
#define LOG_CAT(cat, fmt, ...) \
    do { \
        if ((g_log_categories & (cat)) && \
            g_current_frame >= g_log_start_frame && \
            g_current_frame <= g_log_end_frame) { \
            printf(fmt, ##__VA_ARGS__); \
        } \
    } while(0)

// Convenience macros for common categories
#define LOG_BIOS(fmt, ...)    LOG_CAT(LOG_CAT_BIOS, fmt, ##__VA_ARGS__)
#define LOG_IRQ(fmt, ...)     LOG_CAT(LOG_CAT_IRQ, fmt, ##__VA_ARGS__)
#define LOG_DMA(fmt, ...)     LOG_CAT(LOG_CAT_DMA, fmt, ##__VA_ARGS__)
#define LOG_STACK(fmt, ...)   LOG_CAT(LOG_CAT_STACK, fmt, ##__VA_ARGS__)
#define LOG_LDR(fmt, ...)     LOG_CAT(LOG_CAT_LDR, fmt, ##__VA_ARGS__)
#define LOG_BL(fmt, ...)      LOG_CAT(LOG_CAT_BL, fmt, ##__VA_ARGS__)
#define LOG_REGION(fmt, ...)  LOG_CAT(LOG_CAT_REGION, fmt, ##__VA_ARGS__)
#define LOG_VRAM(fmt, ...)    LOG_CAT(LOG_CAT_VRAM, fmt, ##__VA_ARGS__)
#define LOG_FEATURE(fmt, ...) LOG_CAT(LOG_CAT_FEATURE, fmt, ##__VA_ARGS__)
#define LOG_REG(fmt, ...)     LOG_CAT(LOG_CAT_REG, fmt, ##__VA_ARGS__)
#define LOG_TRACE_CAT(fmt, ...) LOG_CAT(LOG_CAT_TRACE, fmt, ##__VA_ARGS__)
#define LOG_TIMER(fmt, ...)   LOG_CAT(LOG_CAT_TIMER, fmt, ##__VA_ARGS__)
#define LOG_CRASH(fmt, ...)   LOG_CAT(LOG_CAT_CRASH, fmt, ##__VA_ARGS__)

// Helper to parse --log= command line argument
uint32_t parse_log_categories(const char* arg);

// Helper to parse --log-frames= command line argument
void parse_log_frames(const char* arg);

// ============================================================================
// DEBUG SYSTEM - Development debugging (stripped in production)
// ============================================================================

#ifdef DEBUG_BUILD

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

// Color definitions for terminal output
#define DEBUG_COLOR_RED     "\x1b[31m"
#define DEBUG_COLOR_GREEN   "\x1b[32m"
#define DEBUG_COLOR_YELLOW  "\x1b[33m"
#define DEBUG_COLOR_BLUE    "\x1b[34m"
#define DEBUG_COLOR_MAGENTA "\x1b[35m"
#define DEBUG_COLOR_CYAN    "\x1b[36m"
#define DEBUG_COLOR_RESET   "\x1b[0m"

// Debug levels
#define DEBUG_LEVEL_OFF         0
#define DEBUG_LEVEL_BASIC       1
#define DEBUG_LEVEL_VERBOSE     2
#define DEBUG_LEVEL_VERY_VERBOSE 3

// File masks for filtering debug output
#define DEBUG_FILE_MAIN   (1 << 0)
#define DEBUG_FILE_ARM    (1 << 1)
#define DEBUG_FILE_CPU    (1 << 2)
#define DEBUG_FILE_THUMB  (1 << 3)
#define DEBUG_FILE_MEMORY (1 << 4)

// Global debug configuration
extern int g_debug_level;
extern int g_debug_file_mask;
extern bool g_disassemble_enabled;

inline bool debug_is_file_enabled(const char* filename) {
    if (strstr(filename, "main.cpp") && (g_debug_file_mask & DEBUG_FILE_MAIN)) return true;
    if (strstr(filename, "arm") && (g_debug_file_mask & DEBUG_FILE_ARM)) return true;
    if (strstr(filename, "cpu") && (g_debug_file_mask & DEBUG_FILE_CPU)) return true;
    if (strstr(filename, "thumb") && (g_debug_file_mask & DEBUG_FILE_THUMB)) return true;
    if (strstr(filename, "memory") && (g_debug_file_mask & DEBUG_FILE_MEMORY)) return true;
    return false;
}

inline std::string debug_to_hex_string(uint32_t value, int width) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << value;
    return oss.str();
}

#define DEBUG_ERROR(msg) \
    do { \
        std::string debug_msg = msg; \
        fprintf(stderr, "%s[ERROR] %s:%d: %s%s\n", \
            DEBUG_COLOR_RED, __FILE__, __LINE__, debug_msg.c_str(), DEBUG_COLOR_RESET); \
    } while(0)

#define DEBUG_INFO(msg) \
    do { \
        if (g_debug_level >= DEBUG_LEVEL_BASIC) { \
            std::string debug_msg = msg; \
            fprintf(stderr, "%s[INFO]  %s:%d: %s%s\n", \
                DEBUG_COLOR_GREEN, __FILE__, __LINE__, debug_msg.c_str(), DEBUG_COLOR_RESET); \
        } \
    } while(0)

#define DEBUG_LOG(msg) \
    do { \
        if (g_debug_level >= DEBUG_LEVEL_VERBOSE && debug_is_file_enabled(__FILE__)) { \
            std::string debug_msg = msg; \
            fprintf(stderr, "%s[DEBUG] %s:%d: %s%s\n", \
                DEBUG_COLOR_CYAN, __FILE__, __LINE__, debug_msg.c_str(), DEBUG_COLOR_RESET); \
        } \
    } while(0)

#define DEBUG_TRACE(msg) \
    do { \
        if (g_debug_level >= DEBUG_LEVEL_VERY_VERBOSE && debug_is_file_enabled(__FILE__)) { \
            std::string debug_msg = msg; \
            fprintf(stderr, "%s[TRACE] %s:%d: %s%s\n", \
                DEBUG_COLOR_MAGENTA, __FILE__, __LINE__, debug_msg.c_str(), DEBUG_COLOR_RESET); \
        } \
    } while(0)

#define DEBUG_TO_HEX_STRING(value, width) debug_to_hex_string(value, width)

#else // DEBUG_BUILD not defined

#define DEBUG_ERROR(msg)                    do { } while(0)
#define DEBUG_INFO(msg)                     do { } while(0)
#define DEBUG_LOG(msg)                      do { } while(0)
#define DEBUG_TRACE(msg)                    do { } while(0)
#define DEBUG_TO_HEX_STRING(value, width)   ""

#include <string>
inline std::string debug_to_hex_string(uint32_t, int) { return ""; }

extern int g_debug_level;
extern int g_debug_file_mask;
extern bool g_disassemble_enabled;

#endif // DEBUG_BUILD

#endif // DEBUG_H
