#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <string.h>

// COLOUR definitions
#define COLOUR_RED     "\x1b[31m"
#define COLOUR_GREEN   "\x1b[32m"
#define COLOUR_YELLOW  "\x1b[33m"
#define COLOUR_BLUE    "\x1b[34m"
#define COLOUR_MAGENTA "\x1b[35m"
#define COLOUR_CYAN    "\x1b[36m"
#define COLOUR_RESET   "\x1b[0m"

// Define DEBUG level (0=off, 1=basic, 2=verbose, 3=very verbose)
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0  // Default to off
#endif

// Strip file path to just the filename
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)


// --- Per-file debug bitmask system ---
// Assign a bit to each file
#define DEBUG_MAIN      (1 << 0)
#define DEBUG_LEXER     (1 << 1)
#define DEBUG_PARSER    (1 << 2)
#define DEBUG_CFG       (1 << 3)
#define DEBUG_TAC       (1 << 4)
#define DEBUG_OPTIMIZE  (1 << 5)
// ...add more as needed

// Set the debug mask for enabled files
#define DEBUG_FILE_MASK (DEBUG_TAC | DEBUG_OPTIMIZE)  // Example: enable TAC and OPTIMIZE
//#define DEBUG_FILE_MASK 0 // Uncomment to disable all debugging


// Map filename to debug bit
static inline int debug_file_flag(const char *filename) {
    if (strcmp(filename, "main.c") == 0) return DEBUG_MAIN;
    if (strcmp(filename, "lexer.c") == 0 || strcmp(filename, "test_lexer.c") == 0) return DEBUG_LEXER;
    if (strcmp(filename, "parser.c") == 0 || strcmp(filename, "test_parser.c") == 0) return DEBUG_PARSER;
    if (strcmp(filename, "cfg.c") == 0 || strcmp(filename, "test_cfg.c") == 0) return DEBUG_CFG;
    if (strcmp(filename, "tac.c") == 0 || strcmp(filename, "test_tac.c") == 0) return DEBUG_TAC;
    if (strcmp(filename, "optimize.c") == 0 || strcmp(filename, "test_optimize.c") == 0) return DEBUG_OPTIMIZE;
    if (strcmp(filename, "dominance.c") == 0 || strcmp(filename, "test_dominance.c") == 0) return DEBUG_CFG; // or define DEBUG_DOMINANCE if you want a separate bit
    return 0;
}

// Macro to conditionally execute a statement if debugging is enabled for this file
#define DEBUG_DO(stmt) \
    do { if (debug_file_flag(__FILENAME__) & DEBUG_FILE_MASK) { stmt; } } while (0)

// Debug print macros (now check file mask)
#define LOG_ERROR(fmt, ...) \
    do { if (debug_file_flag(__FILENAME__) & DEBUG_FILE_MASK) \
        fprintf(stderr, COLOUR_RED "[ERROR] %s %s(): %d: " fmt COLOUR_RESET "\n", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#if DEBUG_LEVEL >= 1
#define LOG_INFO(fmt, ...) \
    do { if (debug_file_flag(__FILENAME__) & DEBUG_FILE_MASK) \
        fprintf(stderr, COLOUR_GREEN "[INFO]  %s %s(): %d: " fmt COLOUR_RESET "\n", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#else
#define LOG_INFO(fmt, ...)
#endif

#if DEBUG_LEVEL >= 2
#define LOG_DEBUG(fmt, ...) \
    do { if (debug_file_flag(__FILENAME__) & DEBUG_FILE_MASK) \
        fprintf(stderr, COLOUR_CYAN "[DEBUG] %s %s(): %d: " fmt COLOUR_RESET "\n", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#else
#define LOG_DEBUG(fmt, ...)
#endif

#if DEBUG_LEVEL >= 3
#define LOG_TRACE(fmt, ...) \
    do { if (debug_file_flag(__FILENAME__) & DEBUG_FILE_MASK) \
        fprintf(stderr, COLOUR_MAGENTA "[TRACE] %s %s(): %d: " fmt COLOUR_RESET "\n", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#else
#define LOG_TRACE(fmt, ...)
#endif

#endif // DEBUG_H