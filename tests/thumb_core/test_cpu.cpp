#include "gtest/gtest.h"
#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include "interrupt.h"
#include <sstream>
#include <set>

// Common helper functions for all CPU tests
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

// Include all format-specific test files
// #include "format01_shift_immediate.cpp"
// #include "format02_add_sub.cpp"
// #include "format03_immediate.cpp"
// #include "format04_alu.cpp"
// #include "format05_hi_register_ops_bx.cpp"
// #include "format06_pc_relative_load.cpp"
// #include "format07_load_store_reg.cpp"
// format08 now uses modern test_thumb08.cpp with ThumbCPUTest8 fixture
// format09 now uses modern test_thumb09.cpp with ThumbCPUTest9 fixture
// #include "format10_load_store_half.cpp"
// #include "format11_sp_relative.cpp"
// #include "format12_load_address.cpp"
// #include "format13_stack_operations.cpp"
// #include "format14_load_store_multiple.cpp"
#include "format15_multiple_load_store.cpp"
#include "format16_conditional_branch.cpp"
#include "format17_software_interrupt.cpp"
#include "format18_unconditional_branch.cpp"
#include "format19_long_branch.cpp"
