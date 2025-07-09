#ifndef DEBUG_H
#define DEBUG_H

/*
 * Debug System - Macro-based debugging with complete production stripping
 * 
 * Usage:
 *   - Define DEBUG_BUILD during compilation to enable debug output
 *   - Leave DEBUG_BUILD undefined for production builds (completely strips debug code)
 *   
 * Debug Macros:
 *   DEBUG_ERROR(msg)   - Always prints error messages (even in production via stderr)
 *   DEBUG_INFO(msg)    - Prints info messages (level >= BASIC)
 *   DEBUG_LOG(msg)     - Prints debug messages (level >= VERBOSE, respects file mask)
 *   DEBUG_TRACE(msg)   - Prints trace messages (level >= VERY_VERBOSE, respects file mask)
 *   
 * Convenience Macros:
 *   DEBUG_PRINT_HEX(name, value, width) - Print hex value with name
 *   DEBUG_PRINT_REG(name, value)        - Print register value (32-bit hex)
 *   DEBUG_ENTER_FUNCTION()              - Trace function entry
 *   DEBUG_EXIT_FUNCTION()               - Trace function exit
 *   
 * Configuration:
 *   - g_debug_level: Set runtime debug level (0-3)
 *   - g_debug_file_mask: Set which files to debug (bitwise OR of DEBUG_FILE_* flags)
 *   
 * Example:
 *   DEBUG_LOG("Processing instruction: " << std::hex << instruction);
 *   DEBUG_PRINT_REG("R0", cpu.registers[0]);
 *   
 * Build configuration:
 *   Debug:      -DDEBUG_BUILD
 *   Production: (no flags) or use debug_stripped.h
 */

// Debug build configuration
// Define DEBUG_BUILD to enable debug output
// Undefine DEBUG_BUILD for production builds to completely remove debug code
#ifdef DEBUG_BUILD

#include <stdio.h>
#include <string.h>
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
#define DEBUG_FILE_MAIN  (1 << 0)
#define DEBUG_FILE_ARM   (1 << 1) 
#define DEBUG_FILE_CPU   (1 << 2)
#define DEBUG_FILE_THUMB (1 << 3)

// Global debug configuration (can be modified at runtime)
extern int g_debug_level;
extern int g_debug_file_mask;

// Helper function to check if debug output is enabled for a file
inline bool debug_is_file_enabled(const char* filename) {
    // Simple filename matching - extend as needed
    if (strstr(filename, "main.cpp") && (g_debug_file_mask & DEBUG_FILE_MAIN)) return true;
    if (strstr(filename, "arm") && (g_debug_file_mask & DEBUG_FILE_ARM)) return true;
    if (strstr(filename, "cpu") && (g_debug_file_mask & DEBUG_FILE_CPU)) return true;
    if (strstr(filename, "thumb") && (g_debug_file_mask & DEBUG_FILE_THUMB)) return true;
    return false;
}

// Helper function to convert integer to hex string
inline std::string debug_to_hex_string(uint32_t value, int width) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << value;
    return oss.str();
}

// Debug macros - these expand to actual code when DEBUG_BUILD is defined
#define DEBUG_ERROR(msg) \
    do { \
        std::cerr << DEBUG_COLOR_RED << "[ERROR] " << __FILE__ << ":" << __LINE__ << ": " \
                  << (msg) << DEBUG_COLOR_RESET << std::endl; \
    } while(0)

#define DEBUG_INFO(msg) \
    do { \
        if (g_debug_level >= DEBUG_LEVEL_BASIC) { \
            std::cerr << DEBUG_COLOR_GREEN << "[INFO]  " << __FILE__ << ":" << __LINE__ << ": " \
                      << (msg) << DEBUG_COLOR_RESET << std::endl; \
        } \
    } while(0)

#define DEBUG_LOG(msg) \
    do { \
        if (g_debug_level >= DEBUG_LEVEL_VERBOSE && debug_is_file_enabled(__FILE__)) { \
            std::cerr << DEBUG_COLOR_CYAN << "[DEBUG] " << __FILE__ << ":" << __LINE__ << ": " \
                      << (msg) << DEBUG_COLOR_RESET << std::endl; \
        } \
    } while(0)

#define DEBUG_TRACE(msg) \
    do { \
        if (g_debug_level >= DEBUG_LEVEL_VERY_VERBOSE && debug_is_file_enabled(__FILE__)) { \
            std::cerr << DEBUG_COLOR_MAGENTA << "[TRACE] " << __FILE__ << ":" << __LINE__ << ": " \
                      << (msg) << DEBUG_COLOR_RESET << std::endl; \
        } \
    } while(0)

// Convenience macros for common debug patterns
#define DEBUG_PRINT_HEX(name, value, width) \
    do { \
        if (g_debug_level >= DEBUG_LEVEL_VERBOSE && debug_is_file_enabled(__FILE__)) { \
            std::cerr << DEBUG_COLOR_CYAN << "[DEBUG] " << __FILE__ << ":" << __LINE__ << ": " \
                      << (name) << ": 0x" << debug_to_hex_string(value, width) << DEBUG_COLOR_RESET << std::endl; \
        } \
    } while(0)

#define DEBUG_PRINT_REG(name, value) \
    do { \
        if (g_debug_level >= DEBUG_LEVEL_VERBOSE && debug_is_file_enabled(__FILE__)) { \
            std::cerr << DEBUG_COLOR_CYAN << "[DEBUG] " << __FILE__ << ":" << __LINE__ << ": " \
                      << (name) << ": 0x" << debug_to_hex_string(value, 8) << DEBUG_COLOR_RESET << std::endl; \
        } \
    } while(0)

#define DEBUG_ENTER_FUNCTION() \
    do { \
        if (g_debug_level >= DEBUG_LEVEL_VERY_VERBOSE && debug_is_file_enabled(__FILE__)) { \
            std::cerr << DEBUG_COLOR_MAGENTA << "[TRACE] " << __FILE__ << ":" << __LINE__ << ": " \
                      << "Entering " << __FUNCTION__ << DEBUG_COLOR_RESET << std::endl; \
        } \
    } while(0)

#define DEBUG_EXIT_FUNCTION() \
    do { \
        if (g_debug_level >= DEBUG_LEVEL_VERY_VERBOSE && debug_is_file_enabled(__FILE__)) { \
            std::cerr << DEBUG_COLOR_MAGENTA << "[TRACE] " << __FILE__ << ":" << __LINE__ << ": " \
                      << "Exiting " << __FUNCTION__ << DEBUG_COLOR_RESET << std::endl; \
        } \
    } while(0)

#else // DEBUG_BUILD not defined - strip out all debug code

// When DEBUG_BUILD is not defined, all debug macros expand to nothing
#define DEBUG_ERROR(msg)                    do { } while(0)
#define DEBUG_INFO(msg)                     do { } while(0)
#define DEBUG_LOG(msg)                      do { } while(0)
#define DEBUG_TRACE(msg)                    do { } while(0)
#define DEBUG_PRINT_HEX(name, value, width) do { } while(0)
#define DEBUG_PRINT_REG(name, value)        do { } while(0)
#define DEBUG_ENTER_FUNCTION()              do { } while(0)
#define DEBUG_EXIT_FUNCTION()               do { } while(0)

#endif // DEBUG_BUILD

#endif // DEBUG_H