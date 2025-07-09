# Debug System Documentation

This document describes how to use the macro-based debug system in the GBA emulator.

## Overview

The debug system is designed to provide:

1. Zero runtime overhead in release builds
2. Lazy evaluation of debug messages to minimize performance impact
3. A consistent API in both debug and release builds
4. Compile-time elimination of debug code in release/benchmark builds

## Usage

### Basic Logging

```cpp
// Include the debug macros
#include "debug_macros.h"

// Basic logging at different levels
DEBUG_LOG_ERROR("Error: Something went wrong");
DEBUG_LOG_INFO("Info: Operation completed");
DEBUG_LOG_DEBUG("Debug: Internal state = " + std::to_string(value));
DEBUG_LOG_TRACE("Trace: Detailed execution flow");
```

### Lazy Evaluation

For debug messages that require expensive computation, use the lazy logging macros:

```cpp
// The lambda is only executed if debug is enabled and at the appropriate level
DEBUG_LAZY_LOG_INFO([]() {
    return "Operation completed with result: " + calculateExpensiveResult();
});

// Capturing variables from the surrounding scope
DEBUG_LAZY_LOG_DEBUG([&]() {
    return "Complex object state: " + obj.toString();
});
```

### String Formatting and Hex Conversion

```cpp
// Convert a value to a hex string
std::string address = DEBUG_TO_HEX_STRING(memoryAddress, 8); // 8-digit hex

// Format messages with parameters
auto message = DEBUG_FORMAT_MESSAGE("Value: %d, Address: 0x%08X", value, address);
DEBUG_LAZY_LOG_INFO(message);
```

### Builder Pattern

For complex debug messages with multiple components:

```cpp
// Create a builder
auto builder = DEBUG_BUILDER_CREATE();

// Add components
DEBUG_BUILDER_ADD(builder, "Register state: ");
DEBUG_BUILDER_ADD_HEX(builder, reg1, 8);
DEBUG_BUILDER_ADD(builder, ", ");
DEBUG_BUILDER_ADD_HEX(builder, reg2, 8);

// Build and log
std::string message = DEBUG_BUILDER_BUILD(builder);
DEBUG_LOG_DEBUG(message);

// Or use lazy evaluation
DEBUG_LAZY_LOG_DEBUG(DEBUG_BUILDER_AS_FUNCTION(builder));
```

## Configuration

The debug system is configured through compile-time flags:

- `DEBUG_LEVEL` - Sets the debug verbosity level (0=Off, 1=Basic, 2=Verbose, 3=VeryVerbose)
- `NDEBUG` - Standard C++ define that disables debug code when set
- `BENCHMARK_MODE` - Disables debug code when running benchmarks

These flags can be set in the Makefile:

```makefile
# Debug build
CXXFLAGS += -DDEBUG_LEVEL=2

# Release build
CXXFLAGS += -DNDEBUG -DDEBUG_LEVEL=0
```

## Migration Guide

When migrating from the old direct function call style to the macro-based system:

1. Replace `#include "debug.h"` with `#include "debug_macros.h"`
2. Replace direct calls like `Debug::log::info("message")` with `DEBUG_LOG_INFO("message")`
3. Replace expensive debug operations with lazy versions:

```cpp
// Old style
if (Debug::Config::debugLevel >= Debug::Level::Verbose) {
    Debug::log::debug("Value: " + expensiveCalculation());
}

// New style
DEBUG_LAZY_LOG_DEBUG([]() {
    return "Value: " + expensiveCalculation();
});
```

## Performance Considerations

- In release builds (`NDEBUG` defined or `DEBUG_LEVEL=0`), all debug code is eliminated at compile time
- Use lazy evaluation for expensive operations
- Consider using the builder pattern for complex messages with multiple components
- Debug macros in hot paths will have zero overhead in release builds
