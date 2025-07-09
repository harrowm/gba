#ifndef DEBUG_STRIPPED_H
#define DEBUG_STRIPPED_H

#include <string>
#include <functional>
#include <vector>

/**
 * This header provides API compatibility for existing code when using the macro-based debug system.
 * No actual debug code will be compiled in when DEBUG_ENABLED is 0, ensuring zero runtime overhead.
 * 
 * This file is included by debug_macros.h only when DEBUG_ENABLED is 0.
 * 
 * DO NOT include this file directly. Instead, include "debug_macros.h" which will
 * automatically choose the appropriate implementation.
 */

// Empty Debug namespace with stub implementations for API compatibility
namespace Debug {
    // Keep the enum definitions for API compatibility
    enum class Level {
        Off = 0,
        Basic = 1,
        Verbose = 2,
        VeryVerbose = 3
    };

    enum class FileMask {
        Main = 1 << 0,
        ARM = 1 << 1,
        CPU = 1 << 2,
        Thumb = 1 << 3
    };

    // Color constants (empty strings in release mode for API compatibility)
    constexpr const char* COLOUR_RED     = "";
    constexpr const char* COLOUR_GREEN   = "";
    constexpr const char* COLOUR_YELLOW  = "";
    constexpr const char* COLOUR_BLUE    = "";
    constexpr const char* COLOUR_MAGENTA = "";
    constexpr const char* COLOUR_CYAN    = "";
    constexpr const char* COLOUR_RESET   = "";

    // Minimal config that always returns "disabled"
    class Config {
    public:
        static constexpr Level debugLevel = Level::Off;
        static constexpr int fileMask = 0;

        static inline bool isFileEnabled(const std::string&) {
            return false;
        }
    };

    // Empty logging functions with implementation via macros
    class log {
    public:
        static inline void error(const std::string&, const char* = "", int = 0) {}
        static inline void info(const std::string&, const char* = "", int = 0) {}
        static inline void debug(const std::string&, const char* = "", int = 0) {}
        static inline void trace(const std::string&, const char* = "", int = 0) {}
    };

    // Fast hex string conversion that returns an empty string
    inline std::string toHexString(uint32_t, int) {
        return "";
    }
}

// Empty optimized debug namespace with API compatibility
namespace DebugOpt {
    class LazyLog {
    public:
        template<typename Func>
        static inline void error(Func) {}
        
        template<typename Func>
        static inline void info(Func) {}
        
        template<typename Func>
        static inline void debug(Func) {}
        
        template<typename Func>
        static inline void trace(Func) {}
    };

    inline std::function<std::string()> hexString(uint32_t, int) {
        return []() { return ""; };
    }

    template<typename... Values>
    inline std::function<std::string()> formatMessage(const std::string&, Values...) {
        return []() { return ""; };
    }

    class LazyBuilder {
    public:
        inline LazyBuilder() {}
        inline LazyBuilder& add(const std::string&) { return *this; }
        inline LazyBuilder& addHex(uint32_t, int) { return *this; }
        inline LazyBuilder& addVal(uint32_t) { return *this; }
        inline std::string build() const { return ""; }
        inline std::function<std::string()> asFunction() const { 
            return []() { return ""; }; 
        }
    };
}

#endif // DEBUG_STRIPPED_H
