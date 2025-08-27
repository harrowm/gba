#include <iostream>
#include <iomanip>

int main() {
    // Calculate what we expect
    uint32_t sp = 0x00001000;
    uint32_t offset = 0x3FC;
    uint32_t expected = sp + offset;
    
    std::cout << "SP: 0x" << std::hex << std::uppercase << sp << std::endl;
    std::cout << "Offset: 0x" << std::hex << std::uppercase << offset << std::endl;
    std::cout << "Expected address: 0x" << std::hex << std::uppercase << expected << std::endl;
    
    // What we saw in debug
    uint32_t actual = 0x5116;
    std::cout << "Actual address: 0x" << std::hex << std::uppercase << actual << std::endl;
    
    // Try to reverse engineer
    uint32_t reverse_sp = actual - offset;
    std::cout << "Reverse calculated SP: 0x" << std::hex << std::uppercase << reverse_sp << std::endl;
    
    return 0;
}
