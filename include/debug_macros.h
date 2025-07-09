#ifndef DEBUG_MACROS_H
#define DEBUG_MACROS_H

/**
 * This header provides the core debug macros that will be expanded or eliminated
 * based on compilation flags.
 * 
 * In debug builds:    Macros expand to their full implementations
 * In release builds:  Macros expand to no-ops that are eliminated during preprocessing
 * 
 * Usage:
 *   // Basic logging
 *   DEBUG_LOG_INFO("Value is: " << value);
 *   DEBUG_LOG_DEBUG("Debug message");
 *   DEBUG_LOG_ERROR("Error occurred");
 *   DEBUG_LOG_TRACE("Detailed tracing information");
 *
 *   // Lazy evaluation (only evaluates if debug is enabled)
 *   DEBUG_LAZY_LOG_INFO([]() { return "Expensive calculation: " + calculate(); });
 *   DEBUG_LAZY_LOG_DEBUG([&]() { 
 *     return "Complex debug info: " + std::to_string(getComplexValue()); 
 *   });
 *
 *   // Hex string conversion
 *   std::string hex = DEBUG_TO_HEX_STRING(value, 8);
 *
 *   // Builder pattern (more efficient than string concatenation)
 *   auto builder = DEBUG_BUILDER_CREATE();
 *   DEBUG_BUILDER_ADD(builder, "Value: ");
 *   DEBUG_BUILDER_ADD_HEX(builder, value, 8);
 *   DEBUG_LAZY_LOG_INFO(DEBUG_BUILDER_AS_FUNCTION(builder));
 */

#include <string>
#include <functional>

// Debug level definitions
#ifndef DEBUG_LEVEL
    #ifdef NDEBUG
        #define DEBUG_LEVEL 0  // Default to off in release builds
    #else
        #define DEBUG_LEVEL 1  // Default to basic debug level in debug builds
    #endif
#endif

// Check if we're in a debug build
#if defined(NDEBUG) || defined(BENCHMARK_MODE) || (DEBUG_LEVEL == 0)
    // Release/benchmark build - all debug code is eliminated
    #define DEBUG_ENABLED 0
#else
    // Debug build - debug code is active
    #define DEBUG_ENABLED 1
#endif

// Core debug macros
#if DEBUG_ENABLED
    // Include the real debug implementation
    #include "debug.h"
    #include "debug_optimized.h"

    // Regular logging macros
    #define DEBUG_LOG_ERROR(msg) Debug::log::error(msg)
    #define DEBUG_LOG_INFO(msg) Debug::log::info(msg)
    #define DEBUG_LOG_DEBUG(msg) if (Debug::Config::debugLevel >= Debug::Level::Basic) { Debug::log::debug(msg); }
    #define DEBUG_LOG_TRACE(msg) if (Debug::Config::debugLevel >= Debug::Level::Verbose) { Debug::log::trace(msg); }

    // Lazy evaluation logging macros
    #define DEBUG_LAZY_LOG_ERROR(func) DebugOpt::LazyLog::error(func)
    #define DEBUG_LAZY_LOG_INFO(func) DebugOpt::LazyLog::info(func)
    #define DEBUG_LAZY_LOG_DEBUG(func) if (Debug::Config::debugLevel >= Debug::Level::Basic) { DebugOpt::LazyLog::debug(func); }
    #define DEBUG_LAZY_LOG_TRACE(func) if (Debug::Config::debugLevel >= Debug::Level::Verbose) { DebugOpt::LazyLog::trace(func); }

    // Utility macros
    #define DEBUG_TO_HEX_STRING(val, width) Debug::toHexString(val, width)
    #define DEBUG_FORMAT_MESSAGE(...) DebugOpt::formatMessage(__VA_ARGS__)

    // Builder pattern macros
    #define DEBUG_BUILDER_CREATE() DebugOpt::LazyBuilder()
    #define DEBUG_BUILDER_ADD(builder, str) builder.add(str)
    #define DEBUG_BUILDER_ADD_HEX(builder, val, width) builder.addHex(val, width)
    #define DEBUG_BUILDER_ADD_VALUE(builder, val) builder.addVal(val)
    #define DEBUG_BUILDER_BUILD(builder) builder.build()
    #define DEBUG_BUILDER_AS_FUNCTION(builder) builder.asFunction()

#else
    // Release build - include the stripped debug implementation for API compatibility
    #include "debug_stripped.h"
    
    // Release build - macros expand to no-ops that are completely eliminated at compile time
    #define DEBUG_LOG_ERROR(msg) ((void)0)
    #define DEBUG_LOG_INFO(msg) ((void)0)
    #define DEBUG_LOG_DEBUG(msg) ((void)0)
    #define DEBUG_LOG_TRACE(msg) ((void)0)

    #define DEBUG_LAZY_LOG_ERROR(func) ((void)0)
    #define DEBUG_LAZY_LOG_INFO(func) ((void)0)
    #define DEBUG_LAZY_LOG_DEBUG(func) ((void)0)
    #define DEBUG_LAZY_LOG_TRACE(func) ((void)0)

    // Utility macros that don't produce actual debug output
    #define DEBUG_TO_HEX_STRING(val, width) ("")
    #define DEBUG_FORMAT_MESSAGE(...) ([]() { return std::string(""); })

    // Builder pattern macros that expand to no-ops or empty functions
    #define DEBUG_BUILDER_CREATE() (DebugOpt::LazyBuilder())
    #define DEBUG_BUILDER_ADD(builder, str) (builder)
    #define DEBUG_BUILDER_ADD_HEX(builder, val, width) (builder)
    #define DEBUG_BUILDER_ADD_VALUE(builder, val) (builder)
    #define DEBUG_BUILDER_BUILD(builder) ("")
    #define DEBUG_BUILDER_AS_FUNCTION(builder) ([]() { return std::string(""); })
#endif

#endif // DEBUG_MACROS_H
