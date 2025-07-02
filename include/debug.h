#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <unordered_map>
#include <source_location>
#include <iomanip>
#include <sstream>


// Namespace for Debugging
namespace Debug {

// Colour definitions
constexpr const char* COLOUR_RED     = "\x1b[31m";
constexpr const char* COLOUR_GREEN   = "\x1b[32m";
constexpr const char* COLOUR_YELLOW  = "\x1b[33m";
constexpr const char* COLOUR_BLUE    = "\x1b[34m";
constexpr const char* COLOUR_MAGENTA = "\x1b[35m";
constexpr const char* COLOUR_CYAN    = "\x1b[36m";
constexpr const char* COLOUR_RESET   = "\x1b[0m";

// Debug levels
enum class Level {
    Off = 0,
    Basic = 1,
    Verbose = 2,
    VeryVerbose = 3
};

// File masks
enum class FileMask {
    Main = 1 << 0,
    ARM = 1 << 1,
    CPU = 1 << 2,
    Thumb = 1 << 3
};

// Debug configuration
class Config {
public:
    static inline Level debugLevel = Level::Verbose; // Default to Verbose
    static inline int fileMask = static_cast<int>(FileMask::CPU); // Default to enable CPU logging

    static bool isFileEnabled(const std::string& filename) {
        static const std::unordered_map<std::string, FileMask> fileFlags = {
            {"main.cpp", FileMask::Main},
            {"arm.cpp", FileMask::ARM},
            {"cpu.cpp", FileMask::CPU},
            {"thumb.cpp", FileMask::Thumb}
        };
        auto it = fileFlags.find(filename);
        return it != fileFlags.end() && (static_cast<int>(it->second) & fileMask);
    }
};

// Logging methods
class log {
public:
    static void error(const std::string& message, const std::source_location& location = std::source_location::current()) {
        std::cerr << COLOUR_RED << "[ERROR] " << location.file_name() << " " << location.function_name() << "():" << location.line() << ": " << message << COLOUR_RESET << std::endl;
    }

    static void info(const std::string& message, const std::source_location& location = std::source_location::current()) {
        if (Config::debugLevel >= Level::Basic) {
            std::cerr << COLOUR_GREEN << "[INFO]  " << location.file_name() << " " << location.function_name() << "():" << location.line() << ": " << message << COLOUR_RESET << std::endl;
        }
    }

    static void debug(const std::string& message, const std::source_location& location = std::source_location::current()) {
        if (Config::debugLevel >= Level::Verbose) {
            std::cerr << COLOUR_CYAN << "[DEBUG] " << location.file_name() << " " << location.function_name() << "():" << location.line() << ": " << message << COLOUR_RESET << std::endl;
        }
    }

    static void trace(const std::string& message, const std::source_location& location = std::source_location::current()) {
        if (Config::debugLevel >= Level::VeryVerbose) {
            std::cerr << COLOUR_MAGENTA << "[TRACE] " << location.file_name() << " " << location.function_name() << "():" << location.line() << ": " << message << COLOUR_RESET << std::endl;
        }
    }
};

// Convert integer to hex string
inline std::string toHexString(uint32_t value, int width) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << value;
    return oss.str();
}

} // namespace Debug

#endif // DEBUG_H