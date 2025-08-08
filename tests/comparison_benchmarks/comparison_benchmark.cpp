#include <benchmark/benchmark.h>
#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include <vector>
#include <string>
#include "debug.h"

// Helper to load ARM program
void loadARMProgram(GBA& gba, const std::vector<uint32_t>& instructions) {
    auto& memory = gba.getCPU().getMemory();
    for (size_t i = 0; i < instructions.size(); ++i) {
        memory.write32(i * 4, instructions[i]);
    }
}

// Helper to load Thumb program
void loadThumbProgram(GBA& gba, const std::vector<uint16_t>& instructions) {
    auto& memory = gba.getCPU().getMemory();
    for (size_t i = 0; i < instructions.size(); ++i) {
        memory.write16(i * 2, instructions[i]);
    }
}

// Arithmetic (ADD)
static void BM_ARM_Arithmetic(benchmark::State& state) {
    g_debug_level = DEBUG_LEVEL_OFF;
    GBA gba(true);
    auto& cpu = gba.getCPU();
    cpu.CPSR() &= ~CPU::FLAG_T;
    cpu.R().fill(0);
    cpu.R()[1] = 0;
    cpu.R()[2] = 1;
    cpu.R()[15] = 0;
    std::vector<uint32_t> program(1000, 0xE0811002); // ADD R1, R1, R2
    loadARMProgram(gba, program);
    cpu.execute(10); cpu.R()[15] = 0;
    int instructions_per_iteration = 1000 * 10;
    for (auto _ : state) {
        for (int i = 0; i < 10; ++i) { cpu.execute(program.size()); cpu.R()[15] = 0; }
        state.counters["instructions"] += instructions_per_iteration;
    }
    state.SetItemsProcessed(state.iterations() * instructions_per_iteration);
}
static void BM_Thumb_Arithmetic(benchmark::State& state) {
    g_debug_level = DEBUG_LEVEL_OFF;
    GBA gba(true);
    auto& cpu = gba.getCPU();
    cpu.CPSR() |= CPU::FLAG_T;
    cpu.R().fill(0);
    cpu.R()[1] = 0;
    cpu.R()[2] = 1;
    cpu.R()[15] = 0;
    std::vector<uint16_t> program(1000, 0x1889); // ADD R1, R1, R2
    loadThumbProgram(gba, program);
    cpu.execute(10); cpu.R()[15] = 0;
    int instructions_per_iteration = 1000 * 10;
    for (auto _ : state) {
        for (int i = 0; i < 10; ++i) { cpu.execute(program.size()); cpu.R()[15] = 0; }
        state.counters["instructions"] += instructions_per_iteration;
    }
    state.SetItemsProcessed(state.iterations() * instructions_per_iteration);
}

// Memory (STR/LDR)
static void BM_ARM_Memory(benchmark::State& state) {
    g_debug_level = DEBUG_LEVEL_OFF;
    GBA gba(true);
    auto& cpu = gba.getCPU();
    cpu.CPSR() &= ~CPU::FLAG_T;
    cpu.R().fill(0);
    cpu.R()[0] = 0x12345678;
    cpu.R()[1] = 0x200;
    cpu.R()[2] = 0;
    cpu.R()[15] = 0;
    std::vector<uint32_t> program;
    for (int i = 0; i < 500; ++i) {
        program.push_back(0xE5800000); // STR R0, [R0]
        program.push_back(0xE5902000); // LDR R2, [R0]
    }
    loadARMProgram(gba, program);
    cpu.execute(10); cpu.R()[15] = 0;
    int instructions_per_iteration = 1000 * 10;
    for (auto _ : state) {
        for (int i = 0; i < 10; ++i) { cpu.execute(program.size()); cpu.R()[15] = 0; }
        state.counters["instructions"] += instructions_per_iteration;
    }
    state.SetItemsProcessed(state.iterations() * instructions_per_iteration);
}
static void BM_Thumb_Memory(benchmark::State& state) {
    g_debug_level = DEBUG_LEVEL_OFF;
    GBA gba(true);
    auto& cpu = gba.getCPU();
    cpu.CPSR() |= CPU::FLAG_T;
    cpu.R().fill(0);
    cpu.R()[0] = 0x12345678;
    cpu.R()[1] = 0x200;
    cpu.R()[2] = 0;
    cpu.R()[15] = 0;
    std::vector<uint16_t> program;
    for (int i = 0; i < 500; ++i) {
        program.push_back(0x6008); // STR R0, [R1]
        program.push_back(0x680A); // LDR R2, [R1]
    }
    loadThumbProgram(gba, program);
    cpu.execute(10); cpu.R()[15] = 0;
    int instructions_per_iteration = 1000 * 10;
    for (auto _ : state) {
        for (int i = 0; i < 10; ++i) { cpu.execute(program.size()); cpu.R()[15] = 0; }
        state.counters["instructions"] += instructions_per_iteration;
    }
    state.SetItemsProcessed(state.iterations() * instructions_per_iteration);
}

// ALU Operations (EOR)
static void BM_ARM_ALU(benchmark::State& state) {
    g_debug_level = DEBUG_LEVEL_OFF;
    GBA gba(true);
    auto& cpu = gba.getCPU();
    cpu.CPSR() &= ~CPU::FLAG_T;
    cpu.R().fill(0);
    cpu.R()[1] = 0xF0F0F0F0;
    cpu.R()[2] = 0x0F0F0F0F;
    cpu.R()[15] = 0;
    std::vector<uint32_t> program(1000, 0xE0211002); // EOR R1, R1, R2
    loadARMProgram(gba, program);
    cpu.execute(10); cpu.R()[15] = 0;
    int instructions_per_iteration = 1000 * 10;
    for (auto _ : state) {
        for (int i = 0; i < 10; ++i) { cpu.execute(program.size()); cpu.R()[15] = 0; }
        state.counters["instructions"] += instructions_per_iteration;
    }
    state.SetItemsProcessed(state.iterations() * instructions_per_iteration);
}
static void BM_Thumb_ALU(benchmark::State& state) {
    g_debug_level = DEBUG_LEVEL_OFF;
    GBA gba(true);
    auto& cpu = gba.getCPU();
    cpu.CPSR() |= CPU::FLAG_T;
    cpu.R().fill(0);
    cpu.R()[1] = 0xF0F0F0F0;
    cpu.R()[2] = 0x0F0F0F0F;
    cpu.R()[15] = 0;
    std::vector<uint16_t> program(1000, 0x404A); // EOR R2, R1 (Thumb)
    loadThumbProgram(gba, program);
    cpu.execute(10); cpu.R()[15] = 0;
    int instructions_per_iteration = 1000 * 10;
    for (auto _ : state) {
        for (int i = 0; i < 10; ++i) { cpu.execute(program.size()); cpu.R()[15] = 0; }
        state.counters["instructions"] += instructions_per_iteration;
    }
    state.SetItemsProcessed(state.iterations() * instructions_per_iteration);
}

// Branch Instructions
static void BM_ARM_Branch(benchmark::State& state) {
    g_debug_level = DEBUG_LEVEL_OFF;
    GBA gba(true);
    auto& cpu = gba.getCPU();
    cpu.CPSR() &= ~CPU::FLAG_T;
    cpu.R().fill(0);
    cpu.R()[15] = 0;
    std::vector<uint32_t> program(1000, 0xEA000000); // B +0
    loadARMProgram(gba, program);
    cpu.execute(10); cpu.R()[15] = 0;
    int instructions_per_iteration = 1000 * 10;
    for (auto _ : state) {
        for (int i = 0; i < 10; ++i) { cpu.execute(program.size()); cpu.R()[15] = 0; }
        state.counters["instructions"] += instructions_per_iteration;
    }
    state.SetItemsProcessed(state.iterations() * instructions_per_iteration);
}
static void BM_Thumb_Branch(benchmark::State& state) {
    g_debug_level = DEBUG_LEVEL_OFF;
    GBA gba(true);
    auto& cpu = gba.getCPU();
    cpu.CPSR() |= CPU::FLAG_T;
    cpu.R().fill(0);
    cpu.R()[15] = 0;
    std::vector<uint16_t> program(1000, 0xE7FE); // B infinitely
    loadThumbProgram(gba, program);
    cpu.execute(10); cpu.R()[15] = 0;
    int instructions_per_iteration = 1000 * 10;
    for (auto _ : state) {
        for (int i = 0; i < 10; ++i) { cpu.execute(program.size()); cpu.R()[15] = 0; }
        state.counters["instructions"] += instructions_per_iteration;
    }
    state.SetItemsProcessed(state.iterations() * instructions_per_iteration);
}

BENCHMARK(BM_ARM_Arithmetic)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Thumb_Arithmetic)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_ARM_Memory)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Thumb_Memory)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_ARM_ALU)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Thumb_ALU)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_ARM_Branch)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Thumb_Branch)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
