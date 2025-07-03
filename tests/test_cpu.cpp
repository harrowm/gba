#include "gtest/gtest.h"
#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include "interrupt.h"

std::string serializeCPUState(const CPU& cpu) {
    std::ostringstream oss;
    const auto& registers = cpu.R();
    for (size_t i = 0; i < registers.size(); ++i) {
        oss << "R" << i << ":" << registers[i] << ";";
    }
    oss << "CPSR:" << cpu.CPSR();
    return oss.str();
}

// Helper function to validate unchanged registers
void validateUnchangedRegisters(const CPU& cpu, const std::string& beforeState, const std::set<int>& changedRegisters) {
    const auto& registers = cpu.R();
    std::istringstream iss(beforeState);
    std::string token;

    // Deserialize the before state and compare unchanged registers
    for (size_t i = 0; i < registers.size(); ++i) {
        std::getline(iss, token, ';');
        if (changedRegisters.find(i) == changedRegisters.end()) {
            ASSERT_EQ(token, "R" + std::to_string(i) + ":" + std::to_string(registers[i]));
        }
    }
}

TEST(CPU, MOV_IMM) {
    // Setup test GBA with test RAM region
    GBA gba(true); // Pass true to indicate test mode

    // Initialize CPU state
    auto& cpu = gba.getCPU();
    auto& registers = cpu.R();
    registers.fill(0); // Reset all registers to zero
    cpu.CPSR() = CPU::FLAG_T; // Set Thumb mode

    // Test MOV R0, #1
    gba.getCPU().getMemory().write16(0x00000000, 0x2001); // MOV R0, #1
    std::string beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[0], static_cast<unsigned int>(1));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(2)); // PC should increment by 2
    ASSERT_EQ(cpu.CPSR(), CPU::FLAG_T); // Only T flag should be set
    validateUnchangedRegisters(cpu, beforeState, {0, 15});

    // Test MOV R1, #255
    gba.getCPU().getMemory().write16(0x00000002, 0x21FF); // MOV R1, #255
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[1], static_cast<unsigned int>(255));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(4)); // PC should increment by 2
    ASSERT_EQ(cpu.CPSR(), CPU::FLAG_T); // Only T flag should be set
    validateUnchangedRegisters(cpu, beforeState, {1, 15});

    // Test MOV R2, #0
    gba.getCPU().getMemory().write16(0x00000004, 0x2200); // MOV R2, #0
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[2], static_cast<unsigned int>(0));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(6)); // PC should increment by 2
    ASSERT_EQ(cpu.CPSR(), CPU::FLAG_T | CPU::FLAG_Z); // T and Z flags should be set
    validateUnchangedRegisters(cpu, beforeState, {2, 15});

    // Test MOV R3, #42
    gba.getCPU().getMemory().write16(0x00000006, 0x232A); // MOV R3, #42 (42 in decimal is 0x2A in hex)
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[3], static_cast<unsigned int>(42));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(8)); // PC should increment by 2
    ASSERT_EQ(cpu.CPSR(), CPU::FLAG_T); // Only T flag should be set
    validateUnchangedRegisters(cpu, beforeState, {3, 15});

    // Test MOV R4, #127
    gba.getCPU().getMemory().write16(0x00000008, 0x247F); // MOV R4, #127
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[4], static_cast<unsigned int>(127));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(10)); // PC should increment by 2
    ASSERT_EQ(cpu.CPSR(), CPU::FLAG_T); // Only T flag should be set
    validateUnchangedRegisters(cpu, beforeState, {4, 15});

    // Test MOV R5, #0xFF
    gba.getCPU().getMemory().write16(0x0000000A, 0x25FF); // MOV R5, #255
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[5], static_cast<unsigned int>(255));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(12)); // PC should increment by 2
    ASSERT_EQ(cpu.CPSR(), CPU::FLAG_T); // Only T flag should be set
    validateUnchangedRegisters(cpu, beforeState, {5, 15});

    // Test MOV R6, #0x00
    gba.getCPU().getMemory().write16(0x0000000C, 0x2600); // MOV R6, #0
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[6], static_cast<unsigned int>(0));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(14)); // PC should increment by 2
    ASSERT_EQ(cpu.CPSR(), CPU::FLAG_T | CPU::FLAG_Z); // T and Z flags should be set
    validateUnchangedRegisters(cpu, beforeState, {6, 15});

    // Test MOV R7, #0x80
    gba.getCPU().getMemory().write16(0x0000000E, 0x2780); // MOV R7, #128
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[7], static_cast<unsigned int>(128));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(16)); // PC should increment by 2
    ASSERT_EQ(cpu.CPSR(), CPU::FLAG_T); // Only T flag should be set
    validateUnchangedRegisters(cpu, beforeState, {7, 15});

    // Test MOV R7, #0x80 - test NCV flag not altered
    cpu.CPSR() |= CPU::FLAG_N; // set N
    cpu.CPSR() |= CPU::FLAG_C; // set C
    cpu.CPSR() |= CPU::FLAG_V; // set V
    
    gba.getCPU().getMemory().write16(0x00000010, 0x2780); // MOV R7, #128
    beforeState = serializeCPUState(cpu);
    // Debug print CPSR before and after in binary
    std::cout << "CPSR before: \n" << std::bitset<32>(cpu.CPSR()) << std::endl;
    cpu.execute(1);
    std::cout << "CPSR after:  \n" << std::bitset<32>(cpu.CPSR()) << std::endl;
    ASSERT_EQ(registers[7], static_cast<unsigned int>(128));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(18)); // PC should increment by 2
    ASSERT_EQ(cpu.CPSR(),  CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V | CPU::FLAG_T); // T and NCV flags should be set
    validateUnchangedRegisters(cpu, beforeState, {7, 15});
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
