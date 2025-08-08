#include <benchmark/benchmark.h>
#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include <vector>
#include <string>
#include "debug.h"

// Helper to load a Thumb program into memory
void loadThumbProgram(GBA& gba, const std::vector<uint16_t>& instructions) {
    auto& memory = gba.getCPU().getMemory();
    for (size_t i = 0; i < instructions.size(); ++i) {
        memory.write16(i * 2, instructions[i]);
    }
}

// Thumb Arithmetic Benchmark: ADD R1, R1, R2
static void BM_Thumb_Arithmetic(benchmark::State& state) {
    g_debug_level = DEBUG_LEVEL_OFF;
    GBA gba(true);
    auto& cpu = gba.getCPU();
    cpu.CPSR() |= CPU::FLAG_T; // Thumb mode
    cpu.R().fill(0);
    cpu.R()[1] = 0;
    cpu.R()[2] = 1;
    cpu.R()[15] = 0;

    std::vector<uint16_t> program(1000, 0x1889); // ADD R1, R1, R2
    loadThumbProgram(gba, program);

    cpu.execute(10); // Warm up
    cpu.R()[15] = 0;

    int instructions_per_iteration = 1000 * 10;
    for (auto _ : state) {
        for (int i = 0; i < 10; ++i) {
            cpu.execute(program.size());
            cpu.R()[15] = 0;
        }
        state.counters["instructions"] += instructions_per_iteration;
    }
    state.SetItemsProcessed(state.iterations() * instructions_per_iteration);
}

// Thumb Load/Store Benchmark: STR R0, [R1]; LDR R2, [R1]
static void BM_Thumb_LoadStore(benchmark::State& state) {
    g_debug_level = DEBUG_LEVEL_OFF;
    GBA gba(true);
    auto& cpu = gba.getCPU();
    cpu.CPSR() |= CPU::FLAG_T;
    cpu.R().fill(0);
    cpu.R()[0] = 0x12345678;
    cpu.R()[1] = 0x200;
    cpu.R()[2] = 0;
    cpu.R()[15] = 0;

    // STR R0, [R1] = 0x6008; LDR R2, [R1] = 0x680A
    std::vector<uint16_t> program;
    for (int i = 0; i < 500; ++i) {
        program.push_back(0x6008); // STR R0, [R1]
        program.push_back(0x680A); // LDR R2, [R1]
    }
    loadThumbProgram(gba, program);

    cpu.execute(10); // Warm up
    cpu.R()[15] = 0;

    int instructions_per_iteration = 1000 * 10;
    for (auto _ : state) {
        for (int i = 0; i < 10; ++i) {
            cpu.execute(program.size());
            cpu.R()[15] = 0;
        }
        state.counters["instructions"] += instructions_per_iteration;
    }
    state.SetItemsProcessed(state.iterations() * instructions_per_iteration);
}

// Thumb Branch Benchmark: B +2 (0xE002)
static void BM_Thumb_Branch(benchmark::State& state) {
    g_debug_level = DEBUG_LEVEL_OFF;
    GBA gba(true);
    auto& cpu = gba.getCPU();
    cpu.CPSR() |= CPU::FLAG_T;
    cpu.R().fill(0);
    cpu.R()[15] = 0;

    std::vector<uint16_t> program(1000, 0xE002); // B +2
    loadThumbProgram(gba, program);

    cpu.execute(10); // Warm up
    cpu.R()[15] = 0;

    int instructions_per_iteration = 1000 * 10;
    for (auto _ : state) {
        for (int i = 0; i < 10; ++i) {
            cpu.execute(program.size());
            cpu.R()[15] = 0;
        }
        state.counters["instructions"] += instructions_per_iteration;
    }
    state.SetItemsProcessed(state.iterations() * instructions_per_iteration);
}

BENCHMARK(BM_Thumb_Arithmetic)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Thumb_LoadStore)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Thumb_Branch)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
