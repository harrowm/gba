#include <iostream>
#include <keystone/keystone.h>

void test_lsl_encoding(const char* instruction) {
    ks_engine *ks;
    ks_err err;
    size_t count;
    unsigned char *machine_code;
    size_t machine_size;
    
    err = ks_open(KS_ARCH_ARM, KS_MODE_THUMB, &ks);
    if (err != KS_ERR_OK) {
        std::cout << "Failed to open Keystone engine\n";
        return;
    }
    
    // Set the same options as the test base class
    ks_option(ks, KS_OPT_SYNTAX, KS_OPT_SYNTAX_INTEL);
    ks_option(ks, KS_OPT_SYNTAX, KS_OPT_SYNTAX_ATT);  // Use AT&T syntax which may be more restrictive
    
    // Use the same format as the test base class
    std::string full_assembly = ".thumb\n" + std::string(instruction);
    
    if (ks_asm(ks, full_assembly.c_str(), 0x0, &machine_code, &machine_size, &count) != KS_ERR_OK) {
        std::cout << "Assembly failed for \"" << instruction << "\": " << ks_strerror(ks_errno(ks)) << std::endl;
    } else {
        std::cout << "\"" << instruction << "\" -> ";
        for (size_t i = 0; i < machine_size; i++) {
            printf("0x%02X ", machine_code[i]);
        }
        if (machine_size >= 2) {
            uint16_t instruction_word = machine_code[0] | (machine_code[1] << 8);
            printf("(0x%04X)", instruction_word);
        }
        std::cout << std::endl;
        ks_free(machine_code);
    }
    
    ks_close(ks);
}

int main() {
    std::cout << "Testing various LSL instructions with Keystone:\n\n";
    
    // Test different LSL shift amounts
    test_lsl_encoding("lsls r4, r4, #1");
    test_lsl_encoding("lsls r4, r4, #2"); 
    test_lsl_encoding("lsls r4, r4, #30");
    test_lsl_encoding("lsls r4, r4, #31");
    test_lsl_encoding("lsl r4, r4, #31");
    test_lsl_encoding("lsls r4, #31");
    
    // Try different register combinations
    test_lsl_encoding("lsls r4, r4, #0x1f");  // Hex format
    test_lsl_encoding("lsl r4, #31");         // Different syntax
    test_lsl_encoding("mov r4, r4, lsl #31"); // ARM syntax style
    
    // Test the specific manual encoding we have
    std::cout << "\nManual encoding we're using: 0x07E4\n";
    std::cout << "This should be: LSL R4, R4, #31\n";
    std::cout << "The issue appears to be that with ARMv4T compatibility options,\n";
    std::cout << "Keystone rejects shift amounts of 31 as invalid.\n";
    
    return 0;
}
