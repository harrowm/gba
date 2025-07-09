/**
 * Debug System Examples
 * 
 * This file demonstrates various debug patterns using the macro-based debug system.
 * These examples show how to efficiently log debug information with minimal performance impact.
 */

#include "debug_macros.h"
#include <string>
#include <vector>

// Example class with debug logging
class DebugExample {
private:
    uint32_t m_address;
    uint32_t m_value;
    std::vector<uint32_t> m_data;
    
public:
    DebugExample(uint32_t address) : m_address(address), m_value(0) {
        // Simple log message
        DEBUG_LOG_INFO("Created DebugExample with address 0x" + DEBUG_TO_HEX_STRING(address, 8));
        
        // Initialize data vector (expensive operation)
        for (int i = 0; i < 1000; i++) {
            m_data.push_back(i);
        }
    }
    
    void setValue(uint32_t value) {
        m_value = value;
        
        // Use lazy evaluation for expensive debug message
        DEBUG_LAZY_LOG_DEBUG([this]() {
            return "Set value to 0x" + DEBUG_TO_HEX_STRING(m_value, 8) + 
                   " at address 0x" + DEBUG_TO_HEX_STRING(m_address, 8);
        });
    }
    
    uint32_t processData() {
        uint32_t sum = 0;
        
        // Entry log message (always shown in debug builds)
        DEBUG_LOG_DEBUG("Processing data array");
        
        // Detailed tracing (only shown at verbose level)
        for (size_t i = 0; i < m_data.size(); i++) {
            sum += m_data[i];
            
            // This message is expensive to format, so use lazy evaluation
            // It will only be evaluated if debug level is VeryVerbose
            DEBUG_LAZY_LOG_TRACE([this, i, sum]() {
                auto builder = DEBUG_BUILDER_CREATE();
                DEBUG_BUILDER_ADD(builder, "Processing index ");
                DEBUG_BUILDER_ADD_VALUE(builder, i);
                DEBUG_BUILDER_ADD(builder, ", value ");
                DEBUG_BUILDER_ADD_HEX(builder, m_data[i], 8);
                DEBUG_BUILDER_ADD(builder, ", running sum ");
                DEBUG_BUILDER_ADD_VALUE(builder, sum);
                return DEBUG_BUILDER_BUILD(builder);
            });
        }
        
        // Exit log message with result
        DEBUG_LAZY_LOG_INFO([sum]() {
            return "Processed data array, sum = " + std::to_string(sum);
        });
        
        return sum;
    }
    
    void debugDump() {
        // Use builder pattern for complex message construction
        auto builder = DEBUG_BUILDER_CREATE();
        
        DEBUG_BUILDER_ADD(builder, "DebugExample state:\n");
        DEBUG_BUILDER_ADD(builder, "  Address: 0x");
        DEBUG_BUILDER_ADD_HEX(builder, m_address, 8);
        DEBUG_BUILDER_ADD(builder, "\n  Value: 0x");
        DEBUG_BUILDER_ADD_HEX(builder, m_value, 8);
        DEBUG_BUILDER_ADD(builder, "\n  Data size: ");
        DEBUG_BUILDER_ADD_VALUE(builder, m_data.size());
        
        // Log the complex message
        DEBUG_LOG_DEBUG(DEBUG_BUILDER_BUILD(builder));
    }
};

/**
 * In release builds, all debug code above will be completely eliminated at compile time.
 * The resulting binary will have no debug overhead whatsoever.
 * 
 * To build in debug mode:
 *   make DEBUG_LEVEL=2
 * 
 * To build in release mode:
 *   make NDEBUG=1 
 *   or
 *   make BENCHMARK_MODE=1
 */
