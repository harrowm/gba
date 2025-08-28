#include <keystone/keystone.h>
#include <iostream>
#include <iomanip>

int main() {
    ks_engine *ks;
    
    // Initialize Keystone for ARM Thumb mode
    if (ks_open(KS_ARCH_ARM, KS_MODE_THUMB, &ks) != KS_ERR_OK) {
        std::cerr << "Failed to open Keystone engine" << std::endl;
        return 1;
    }
    
    // Test different branch syntaxes
    std::vector<std::string> tests = {
        "beq #0x4",      // Absolute target
        "beq #2",        // Offset
        "beq #4",        // Offset
        "beq 0x4",       // Without #
        "beq . + 4",     // Relative
        "beq . + 0x4",   // Relative hex
        "beq pc + 4",    // PC relative
    };
    
    for (const auto& test : tests) {
        unsigned char* machine_code = nullptr;
        size_t machine_size;
        size_t statement_count;
        
        std::string full_assembly = ".thumb\n" + test;
        
        if (ks_asm(ks, full_assembly.c_str(), 0, &machine_code, &machine_size, &statement_count) == KS_ERR_OK) {
            uint16_t instruction = machine_code[0] | (machine_code[1] << 8);
            std::cout << std::setw(15) << test << " -> 0x" << std::hex << std::setw(4) << std::setfill('0') << instruction << std::endl;
            ks_free(machine_code);
        } else {
            std::cout << std::setw(15) << test << " -> FAILED" << std::endl;
        }
    }
    
    ks_close(ks);
    return 0;
}
