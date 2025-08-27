// Debug test to identify working Format 11 combinations
#include <iostream>
#include <vector>
#include <string>
#include <keystone/keystone.h>

struct TestCase {
    int reg;
    int offset;
    std::string expected_pattern;
};

int main() {
    ks_engine *ks;
    ks_err err;
    
    err = ks_open(KS_ARCH_ARM, KS_MODE_THUMB, &ks);
    if (err != KS_ERR_OK) {
        std::cerr << "Failed to open Keystone engine: " << ks_strerror(err) << std::endl;
        return 1;
    }
    
    // Test various register and offset combinations
    std::vector<TestCase> tests = {
        // Known working cases
        {0, 0, "STR R0, [SP, #0]"},
        {1, 4, "STR R1, [SP, #4]"}, 
        {2, 8, "STR R2, [SP, #8]"},
        
        // Test problematic cases
        {3, 12, "STR R3, [SP, #12]"},
        {4, 16, "STR R4, [SP, #16]"},
        {7, 28, "STR R7, [SP, #28]"},
        
        // Test LDR versions
        {1, 8, "LDR R1, [SP, #8]"},
        {2, 12, "LDR R2, [SP, #12]"},
        {3, 16, "LDR R3, [SP, #16]"},
    };
    
    for (const auto& test : tests) {
        std::string instruction = (test.expected_pattern.find("STR") == 0 ? "str r" : "ldr r") + 
                                std::to_string(test.reg) + ", [sp, #" + std::to_string(test.offset) + "]";
        
        size_t count;
        size_t stat_count;
        unsigned char *encode;
        
        if (ks_asm(ks, instruction.c_str(), 0, &encode, &count, &stat_count) == KS_ERR_OK) {
            if (count == 2) {
                uint16_t opcode = (encode[1] << 8) | encode[0];
                std::printf("✅ %s -> %04X\n", instruction.c_str(), opcode);
                
                // Check if it's Format 11 (1001xxxxxxxxxxxx)
                if ((opcode & 0xF000) == 0x9000) {
                    std::printf("   Format 11 SP-relative ✅\n");
                } else {
                    std::printf("   NOT Format 11 (got %04X, expected 9xxx) ❌\n", opcode);
                }
            } else {
                std::printf("❌ %s -> Generated %zu bytes (expected 2)\n", instruction.c_str(), count);
                for (size_t i = 0; i < count; i++) {
                    std::printf("   Byte %zu: %02X\n", i, encode[i]);
                }
            }
            ks_free(encode);
        } else {
            std::printf("❌ %s -> Assembly failed\n", instruction.c_str());
        }
    }
    
    ks_close(ks);
    return 0;
}
