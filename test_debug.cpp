#include "debug.h"
#include <iostream>

// We need to provide the global variables that debug.h expects
#ifdef DEBUG_BUILD
int g_debug_level = DEBUG_LEVEL_VERBOSE;
int g_debug_file_mask = DEBUG_FILE_CPU;
#endif

int main() {
    DEBUG_INFO("Testing debug macros");
    DEBUG_ERROR("This is an error message");
    DEBUG_LOG("This is a debug log message");
    DEBUG_TRACE("This is a trace message");
    
    DEBUG_PRINT_HEX("Value", 0x1234, 4);
    DEBUG_PRINT_REG("R0", 0xABCD5678);
    
    DEBUG_ENTER_FUNCTION();
    DEBUG_EXIT_FUNCTION();
    
    std::cout << "Debug test completed." << std::endl;
    return 0;
}
