#include "arm_cpu.h"
#include "debug.h"

ARMCPU::ARMCPU(CPU& cpu) : parentCPU(cpu) {
    Debug::log::info("Initializing ARMCPU with parent CPU");
    // Initialize any ARM-specific state or resources here
    // For example, you might want to set up instruction decoding tables or other structures
}

ARMCPU::~ARMCPU() {
    // Cleanup logic if necessary
}

void ARMCPU::execute(uint32_t cycles) {
    Debug::log::info("Executing ARM instructions for " + std::to_string(cycles) + " cycles");
    Debug::log::info("Parent CPU memory size: " + std::to_string(parentCPU.getMemory().getSize()) + " bytes");
    while (cycles > 0) {
        uint32_t pc = parentCPU.R()[15]; // Get current PC
        uint32_t instruction = parentCPU.getMemory().read32(pc); // Fetch instruction
        
        bool pc_modified = decodeAndExecute(instruction);
        
        // Only increment PC if instruction didn't modify it (e.g., not a branch)
        if (!pc_modified) {
            parentCPU.R()[15] = pc + 4;
        }
        
        cycles -= 1; // Placeholder for cycle deduction
    }
}

bool ARMCPU::decodeAndExecute(uint32_t instruction) {
    Debug::log::info("Decoding and executing ARM instruction: 0x" + std::to_string(instruction));
    
    // For testing purposes, implement minimal ARM instruction handling
    // This is a very basic implementation just to support Format 5 BX tests
    
    // Check for BX instruction: 0xE12FFF1X where X is the register number
    if ((instruction & 0xFFFFFFF0) == 0xE12FFF10) {
        uint32_t rn = instruction & 0xF; // Extract register number
        uint32_t target_addr = parentCPU.R()[rn];
        
        Debug::log::info("ARM BX R" + std::to_string(rn) + ": target=0x" + Debug::toHexString(target_addr, 8));
        
        // Set PC to target address (clear bit 0)
        parentCPU.R()[15] = target_addr & ~1;
        
        // Set Thumb mode based on bit 0 of target address
        if (target_addr & 1) {
            parentCPU.CPSR() |= CPU::FLAG_T; // Set Thumb mode
            Debug::log::info("ARM BX: switching to Thumb mode");
        } else {
            parentCPU.CPSR() &= ~CPU::FLAG_T; // Clear Thumb mode
            Debug::log::info("ARM BX: staying in ARM mode");
        }
        
        Debug::log::info("ARM BX: PC set to 0x" + Debug::toHexString(parentCPU.R()[15], 8));
        return true; // PC was modified by this instruction
    }
    
    // Check if this is a NOP instruction (MOV R0, R0 or similar)
    // ARM NOP is typically encoded as MOV R0, R0 = 0xE1A00000
    if (instruction == 0xE1A00000 || instruction == 0x00000000) {
        // NOP - do nothing
        Debug::log::info("ARM NOP instruction executed");
        return false; // PC was not modified
    }
    
    // For any other instruction, treat as NOP for now
    // This prevents the PC from being corrupted
    Debug::log::info("ARM instruction treated as NOP for testing");
    return false; // PC was not modified
}
