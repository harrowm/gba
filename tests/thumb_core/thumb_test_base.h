#pragma once

#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "thumb_cpu.h"
#include <sstream>
#include <set>
#include <cstdint>
#include <vector>
#include <string>

extern "C" {
#include <keystone/keystone.h>
}

// Common helper functions for all Thumb CPU tests
class ThumbTestHelpers {
public:
    static std::string serializeCPUState(const CPU& cpu) {
        std::ostringstream oss;
        const auto& registers = cpu.R();
        for (size_t i = 0; i < registers.size(); ++i) {
            oss << "R" << i << ":" << registers[i] << ";";
        }
        oss << "CPSR:" << cpu.CPSR();
        return oss.str();
    }

    static void validateUnchangedRegisters(const CPU& cpu, const std::string& beforeState, const std::set<int>& changedRegisters) {
        const auto& registers = cpu.R();
        std::istringstream iss(beforeState);
        std::string token;
        
        for (size_t i = 0; i < registers.size(); ++i) {
            std::getline(iss, token, ';');
            if (changedRegisters.find(i) == changedRegisters.end()) {
                ASSERT_EQ(token, "R" + std::to_string(i) + ":" + std::to_string(registers[i]));
            }
        }
    }
};

// Base test fixture for all Thumb CPU tests
class ThumbCPUTestBase : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ThumbCPU thumb_cpu;
    ks_engine* ks; // Keystone assembler handle

    ThumbCPUTestBase() : memory(true), cpu(memory, interrupts), thumb_cpu(cpu) {
        ks = nullptr;
    }

    void SetUp() override {
        // Initialize all registers to 0
        for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
        
        // Set Thumb mode (T flag) and User mode
        cpu.CPSR() = CPU::FLAG_T | 0x10;
        
        // Initialize Keystone for Thumb mode (ARMv4T compatible)
        if (ks) ks_close(ks);
        if (ks_open(KS_ARCH_ARM, KS_MODE_THUMB, &ks) != KS_ERR_OK) {
            FAIL() << "Failed to initialize Keystone for Thumb mode";
        }
        
        // Set options for ARMv4T compatibility and Intel syntax
        ks_option(ks, KS_OPT_SYNTAX, KS_OPT_SYNTAX_INTEL);
        // Try to force ARMv4T mode to avoid Thumb-2 instructions
        ks_option(ks, KS_OPT_SYNTAX, KS_OPT_SYNTAX_ATT);  // Use AT&T syntax which may be more restrictive
    }

    void TearDown() override {
        if (ks) {
            ks_close(ks);
            ks = nullptr;
        }
    }

    // Helper: assemble Thumb instruction and write to memory
    bool assembleAndWriteThumb(const std::string& assembly, uint32_t address, std::vector<uint8_t>* out_bytes = nullptr) {
        unsigned char* machine_code = nullptr;
        size_t machine_size;
        size_t statement_count;
        
        // Prepare assembly with simple thumb directive
        std::string full_assembly = ".thumb\n" + assembly;
        
        if (ks_asm(ks, full_assembly.c_str(), address, &machine_code, &machine_size, &statement_count) != KS_ERR_OK) {
            return false;
        }
        
        // Write the machine code to memory
        for (size_t i = 0; i < machine_size; i += 2) {
            uint16_t instruction = machine_code[i] | (machine_code[i+1] << 8);
            memory.write16(address + i, instruction);
        }
        
        // Optionally return the raw bytes
        if (out_bytes) {
            out_bytes->assign(machine_code, machine_code + machine_size);
        }
        
        ks_free(machine_code);
        return true;
    }

    // Common helper methods for all tests
    std::string serializeCPUState() {
        return ThumbTestHelpers::serializeCPUState(cpu);
    }

    void validateUnchangedRegisters(const std::string& beforeState, const std::set<int>& changedRegisters) {
        ThumbTestHelpers::validateUnchangedRegisters(cpu, beforeState, changedRegisters);
    }
    
    // Helper to get CPU registers reference
    std::array<uint32_t, 16>& registers() {
        return cpu.R();
    }
    
    // Helper to execute CPU cycles
    void execute(int cycles = 1) {
        for (int i = 0; i < cycles; ++i) {
            cpu.execute(1);
        }
    }
    
    // Helper to write 16-bit instruction to memory
    void writeInstruction(uint32_t address, uint16_t instruction) {
        memory.write16(address, instruction);
    }
    
    // Helper to set CPU flags
    void setFlags(uint32_t flags) {
        cpu.CPSR() = (cpu.CPSR() & ~(CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V)) | flags;
    }
    
    // Helper to check specific flags
    bool getFlag(uint32_t flag) {
        return cpu.getFlag(flag);
    }
};
