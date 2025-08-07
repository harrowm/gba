#include <benchmark/benchmark.h>
#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include <vector>
#include <string>
#include "debug.h"


static void BM_ALU_Operation(benchmark::State& state, uint32_t opcode) {
    g_debug_level = DEBUG_LEVEL_OFF; // Disable debug output for benchmarks
    // Create GBA in test mode
    GBA gba(true);
    auto& cpu = gba.getCPU();
    auto& memory = cpu.getMemory();
    cpu.CPSR() &= ~CPU::FLAG_T;

    // Create program with 1000 of this instruction
    std::vector<uint32_t> program(1000, opcode);
    for (size_t i = 0; i < program.size(); ++i) {
        memory.write32(i * 4, program[i]);
    }

    auto& registers = cpu.R();
    registers.fill(0);
    registers[1] = 0x12345678;
    registers[2] = 0x1;
    registers[15] = 0;

    cpu.execute(10); // Warm up
    registers[15] = 0;

    int instructions_per_iteration = 1000 * 10; // 1000 instructions per inner loop, 10 inner loops per iteration
    for (auto _ : state) {
        for (int i = 0; i < 10; ++i) {
            cpu.execute(program.size());
            registers[15] = 0;
        }
        // Add 1000*10 instructions per outer iteration
        state.counters["instructions"] += instructions_per_iteration;
    }
    // Optionally, set items processed for compatibility with items_per_second
    state.SetItemsProcessed(state.iterations() * instructions_per_iteration);
}

// Helper to load an ARM program into memory
void loadProgram(GBA& gba, const std::vector<uint32_t>& instructions) {
    auto& memory = gba.getCPU().getMemory();
    for (size_t i = 0; i < instructions.size(); ++i) {
        memory.write32(i * 4, instructions[i]);
    }
}

// Arithmetic Instructions Benchmark
static void BM_ARM_Arithmetic(benchmark::State& state) {
    g_debug_level = DEBUG_LEVEL_OFF;
    GBA gba(true);
    auto& cpu = gba.getCPU();
    cpu.CPSR() &= ~CPU::FLAG_T;
    cpu.R().fill(0);
    cpu.R()[15] = 0;
    std::vector<uint32_t> program(100, 0xE0811002); // ADD R1, R1, R2
    loadProgram(gba, program);
    cpu.R()[1] = 0;
    cpu.R()[2] = 1;
    for (auto _ : state) {
        cpu.R()[15] = 0;
        cpu.execute(program.size());
    }
    benchmark::DoNotOptimize(cpu.R()[1]);
}

// Memory Access Instructions Benchmark
static void BM_ARM_MemoryAccess(benchmark::State& state) {
    g_debug_level = DEBUG_LEVEL_OFF;
    GBA gba(true);
    auto& cpu = gba.getCPU();
    cpu.CPSR() &= ~CPU::FLAG_T;
    cpu.R().fill(0);
    cpu.R()[15] = 0;
    std::vector<uint32_t> program;
    for (int i = 0; i < 50; i++) {
        program.push_back(0xE5801000); // STR R1, [R0]
        program.push_back(0xE5902000); // LDR R2, [R0]
    }
    loadProgram(gba, program);
    cpu.R()[0] = 0x100;
    cpu.R()[1] = 0x12345678;
    for (auto _ : state) {
        cpu.R()[15] = 0;
        cpu.execute(program.size());
    }
    benchmark::DoNotOptimize(cpu.R()[2]);
}

// Branching Code Benchmark
static void BM_ARM_Branching(benchmark::State& state) {
    g_debug_level = DEBUG_LEVEL_OFF;
    GBA gba(true);
    auto& cpu = gba.getCPU();
    cpu.CPSR() &= ~CPU::FLAG_T;
    cpu.R().fill(0);
    cpu.R()[15] = 0;
    std::vector<uint32_t> program = {
        0xE2400001,  // SUB R0, R0, #1
        0xE3500000,  // CMP R0, #0
        0x1AFFFFFC,  // BNE -16 bytes
        0xE3A00000   // MOV R0, #0
    };
    loadProgram(gba, program);
    cpu.R()[0] = state.range(0); // Loop count
    cpu.R()[15] = 0;
    uint32_t expected_instructions = cpu.R()[0] * 3 + 1;
    for (auto _ : state) {
        cpu.R()[0] = state.range(0);
        cpu.R()[15] = 0;
        cpu.execute(expected_instructions);
    }
    benchmark::DoNotOptimize(cpu.R()[0]);
}

// Register each ALU operation benchmark
BENCHMARK_CAPTURE(BM_ALU_Operation, ADD_R1_R1_R2, 0xE0811002);
BENCHMARK_CAPTURE(BM_ALU_Operation, SUB_R1_R1_R2, 0xE0411002);
BENCHMARK_CAPTURE(BM_ALU_Operation, MOV_R1_R2,    0xE1A01002);
BENCHMARK_CAPTURE(BM_ALU_Operation, ORR_R1_R1_R2, 0xE1811002);
BENCHMARK_CAPTURE(BM_ALU_Operation, AND_R1_R1_R2, 0xE0011002);
BENCHMARK_CAPTURE(BM_ALU_Operation, CMP_R1_R2,    0xE1510002);

BENCHMARK(BM_ARM_Arithmetic);
BENCHMARK(BM_ARM_MemoryAccess);
BENCHMARK(BM_ARM_Branching)->Arg(100000);

BENCHMARK_MAIN();
