#include <keystone/keystone.h>
#include <iostream>
#include <iomanip>

int main() {
    ks_engine *ks;
    ks_open(KS_ARCH_ARM, KS_MODE_THUMB, &ks);
    
    // Test different large offsets to find Keystone's limit
    std::vector<int> targets = {0x4, 0x6, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x102};
    
    for (int target : targets) {
        unsigned char* machine_code = nullptr;
        size_t machine_size;
        size_t statement_count;
        
        std::string assembly = ".thumb\nbeq #0x" + std::to_string(target);
        
        if (ks_asm(ks, assembly.c_str(), 0, &machine_code, &machine_size, &statement_count) == KS_ERR_OK) {
            uint16_t instruction = machine_code[0] | (machine_code[1] << 8);
            std::cout << "beq #0x" << std::hex << target << " -> 0x" << std::setw(4) << std::setfill('0') << instruction;
            
            // Decode the offset
            if ((instruction & 0xF000) == 0xD000) {
                int8_t offset = static_cast<int8_t>(instruction & 0xFF);
                std::cout << " (offset: " << std::dec << (int)offset << ")";
            } else {
                std::cout << " (NOT conditional branch!)";
            }
            std::cout << std::endl;
            ks_free(machine_code);
        } else {
            std::cout << "beq #0x" << std::hex << target << " -> FAILED" << std::endl;
        }
    }
    
    ks_close(ks);
    return 0;
}
