#include <iostream>
#include <iomanip>
#include "src/gba.cpp"
#include "src/cpu.cpp"
#include "src/memory.cpp"
#include "src/thumb_cpu.cpp"
#include "src/arm_cpu.cpp"
#include "src/gpu.cpp"
#include "src/instruction_decoder.cpp"
#include "src/instruction_executor.cpp"
#include "src/thumb_instruction_decoder.cpp"
#include "src/thumb_instruction_executor.cpp"

int main() {
    std::cout << "Debug: Same register STRH test" << std::endl;
    
    // Create GBA instance
    GBA gba(true); // testing mode
    auto& cpu = gba.getCPU();
    auto& registers = cpu.R();
    
    // Set up test
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T; // Thumb mode
    
    registers[6] = 0x00001200;
    std::cout << "Initial R6 = 0x" << std::hex << registers[6] << std::endl;
    
    // Execute STRH R6, [R6, #4] (opcode 0x8106)
    gba.getCPU().getMemory().write16(0x00000000, 0x8106);
    
    std::cout << "Before execution:" << std::endl;
    std::cout << "  R6 = 0x" << std::hex << registers[6] << std::endl;
    std::cout << "  PC = 0x" << std::hex << registers[15] << std::endl;
    
    cpu.execute(1);
    
    std::cout << "After execution:" << std::endl;
    std::cout << "  R6 = 0x" << std::hex << registers[6] << std::endl;
    std::cout << "  PC = 0x" << std::hex << registers[15] << std::endl;
    
    // Check what was stored
    uint32_t address = 0x00001204; // R6 + 4
    uint16_t stored_value = gba.getCPU().getMemory().read16(address);
    std::cout << "Stored at 0x" << std::hex << address << ": 0x" << stored_value << std::endl;
    std::cout << "Expected: 0x1200" << std::endl;
    
    return 0;
}
