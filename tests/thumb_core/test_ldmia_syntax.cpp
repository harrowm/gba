#include <iostream>
#include <keystone/keystone.h>

int main() {
    ks_engine* ks;
    ks_err err = ks_open(KS_ARCH_ARM, KS_MODE_THUMB, &ks);
    if (err != KS_ERR_OK) {
        std::cout << "Failed to open Keystone engine" << std::endl;
        return 1;
    }

    std::vector<std::string> ldmia_tests = {
        "ldmia r0!, {r1}",
        "ldm r0!, {r1}",  
        "ldmia r0, {r1}",
        "ldm r0, {r1}",
        "pop {r1}",  // Alternative for some load operations
    };

    for (const auto& assembly : ldmia_tests) {
        unsigned char* machine_code = nullptr;
        size_t machine_size;
        size_t statement_count;
        
        std::string full_assembly = ".thumb\n" + assembly;
        
        if (ks_asm(ks, full_assembly.c_str(), 0, &machine_code, &machine_size, &statement_count) == KS_ERR_OK) {
            uint16_t instruction = machine_code[0] | (machine_code[1] << 8);
            std::cout << "SUCCESS: '" << assembly << "' -> 0x" << std::hex << instruction << std::dec << std::endl;
            ks_free(machine_code);
        } else {
            std::cout << "FAILED:  '" << assembly << "'" << std::endl;
        }
    }

    ks_close(ks);
    return 0;
}
