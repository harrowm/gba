# Makefile for GBA Emulator

# Compiler and flags
CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -Wall -Wextra -I$(INCLUDE_DIR) -g -O2
CFLAGS = -std=c99 -Wall -Wextra -I$(INCLUDE_DIR) -g -O2

# Google Test paths (if available)
GTEST_INCLUDE = /opt/homebrew/include
GTEST_LIB = /opt/homebrew/lib
GTEST_INCLUDE_FLAGS = -I$(GTEST_INCLUDE)
GTEST_LINK_FLAGS = -L$(GTEST_LIB) -lgtest -lgtest_main
GTEST_FLAGS = $(GTEST_INCLUDE_FLAGS) $(GTEST_LINK_FLAGS)

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
TESTS_DIR = tests
DOCS_DIR = docs
EXAMPLES_DIR = examples

# Source files
CPP_SRCS = $(wildcard $(SRC_DIR)/*.cpp)
C_SRCS = $(wildcard $(SRC_DIR)/*.c)

# Object files (excluding main.cpp for libraries)
CPP_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SRCS))
C_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SRCS))
ALL_OBJS = $(CPP_OBJS) $(C_OBJS)
LIB_OBJS = $(filter-out $(BUILD_DIR)/main.o, $(ALL_OBJS))

# Target executables
TARGET = gba_emulator
TEST_TARGET = test_cpu
ARM_TEST_TARGET = test_arm_basic
ARM_DEMO_TARGET = demo_arm_advanced
ARM_BENCHMARK_TARGET = arm_benchmark
ARM_BENCHMARK_OPT_TARGET = arm_benchmark_opt
ARM_BENCHMARK_PROF_TARGET = arm_benchmark_prof
ARM_BENCHMARK_ULTRA_TARGET = arm_benchmark_ultra
TIMING_TEST_TARGET = test_timing
THUMB_TEST_TARGET = test_thumb_timing
DEMO_CYCLE_TARGET = demo_cycle_driven

# Target for ARM execute phase 1 test
ARM_EXECUTE_PHASE1_TEST = test_arm_execute_phase1

# Default rule
all: $(TARGET)

# Optimized flags for benchmarks
OPTIMIZED_CXXFLAGS = -std=c++17 -Wall -Wextra -I$(INCLUDE_DIR) -O3 -flto -DDEBUG_LEVEL=0 -DNDEBUG -DBENCHMARK_MODE -DDISABLE_MEMORY_BOUNDS_CHECK
OPTIMIZED_CFLAGS = -std=c99 -Wall -Wextra -I$(INCLUDE_DIR) -O3 -flto -DDEBUG_LEVEL=0 -DNDEBUG -DBENCHMARK_MODE -DDISABLE_MEMORY_BOUNDS_CHECK

# Profiling flags for gperftools
PROFILING_CXXFLAGS = -std=c++17 -Wall -Wextra -I$(INCLUDE_DIR) -O2 -DDEBUG_LEVEL=0 -DNDEBUG -DBENCHMARK_MODE -DDISABLE_MEMORY_BOUNDS_CHECK
PROFILING_CFLAGS = -std=c99 -Wall -Wextra -I$(INCLUDE_DIR) -O2 -DDEBUG_LEVEL=0 -DNDEBUG -DBENCHMARK_MODE -DDISABLE_MEMORY_BOUNDS_CHECK
PROFILING_LDFLAGS = -L/opt/homebrew/lib -lprofiler

# Ultra-optimized flags for fastest possible benchmarks
ULTRA_CXXFLAGS = -std=c++17 -I$(INCLUDE_DIR) -Ofast -march=native -flto -fomit-frame-pointer -DDEBUG_LEVEL=0 -DNDEBUG -DBENCHMARK_MODE -DDISABLE_MEMORY_BOUNDS_CHECK
ULTRA_CFLAGS = -std=c99 -I$(INCLUDE_DIR) -Ofast -march=native -flto -fomit-frame-pointer -DDEBUG_LEVEL=0 -DNDEBUG -DBENCHMARK_MODE -DDISABLE_MEMORY_BOUNDS_CHECK

# Build all tests and demos
tests: $(ARM_TEST_TARGET) $(ARM_DEMO_TARGET) $(ARM_BENCHMARK_TARGET) $(ARM_BENCHMARK_OPT_TARGET) $(ARM_BENCHMARK_ULTRA_TARGET) $(TIMING_TEST_TARGET) $(THUMB_TEST_TARGET) $(DEMO_CYCLE_TARGET) $(ARM_EXECUTE_PHASE1_TEST)

# Build main emulator
$(TARGET): $(ALL_OBJS)
	@echo "Building main emulator..."
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build ARM basic test
$(ARM_TEST_TARGET): $(BUILD_DIR)/test_arm_basic.o $(LIB_OBJS)
	@echo "Building ARM basic test..."
	$(CXX) $(CXXFLAGS) -o $@ $^ -DDEBUG_LEVEL=0

# Build ARM advanced demo
$(ARM_DEMO_TARGET): $(BUILD_DIR)/demo_arm_advanced.o $(LIB_OBJS)
	@echo "Building ARM advanced demo..."
	$(CXX) $(CXXFLAGS) -o $@ $^ -DDEBUG_LEVEL=0

# Build ARM benchmark test (with memory bounds checking still enabled)
$(ARM_BENCHMARK_TARGET): $(BUILD_DIR)/arm_benchmark_fixed.o $(LIB_OBJS)
	@echo "Building ARM benchmark test with memory bounds checking enabled..."
	$(CXX) $(CXXFLAGS) $(GTEST_FLAGS) -DOUTPUT_BENCHMARK_RESULTS=1 -o $@ $^

# Build optimized ARM benchmark test
$(ARM_BENCHMARK_OPT_TARGET): $(TESTS_DIR)/arm_benchmark_fixed.cpp
	@echo "Building optimized ARM benchmark with aggressive optimizations..."
	mkdir -p $(BUILD_DIR)
	# Use a simpler approach - compile and link in one command
	$(CXX) $(OPTIMIZED_CXXFLAGS) $(GTEST_FLAGS) -DOUTPUT_BENCHMARK_RESULTS=1 -o $@ $(TESTS_DIR)/arm_benchmark_fixed.cpp $(filter-out $(SRC_DIR)/main.cpp, $(CPP_SRCS)) $(C_SRCS)

# Build profiling ARM benchmark test
$(ARM_BENCHMARK_PROF_TARGET): $(TESTS_DIR)/arm_benchmark_fixed.cpp $(SRC_DIR)/debug_optimized.cpp
	@echo "Building ARM benchmark with profiling enabled..."
	mkdir -p $(BUILD_DIR)
	# Use a simpler approach - compile and link in one command
	$(CXX) $(PROFILING_CXXFLAGS) $(GTEST_FLAGS) -DOUTPUT_BENCHMARK_RESULTS=1 -o $@ $(TESTS_DIR)/arm_benchmark_fixed.cpp $(filter-out $(SRC_DIR)/main.cpp, $(CPP_SRCS)) $(C_SRCS) $(SRC_DIR)/debug_optimized.cpp $(PROFILING_LDFLAGS)

# Build ultra-optimized ARM benchmark test with memory bounds checking disabled
$(ARM_BENCHMARK_ULTRA_TARGET): $(TESTS_DIR)/arm_benchmark_fixed.cpp
	@echo "Building ultra-optimized ARM benchmark with memory bounds checking disabled..."
	mkdir -p $(BUILD_DIR)
	# Use a simpler approach - compile and link in one command
	$(CXX) $(ULTRA_CXXFLAGS) $(GTEST_FLAGS) -DOUTPUT_BENCHMARK_RESULTS=1 -o $@ $(TESTS_DIR)/arm_benchmark_fixed.cpp $(filter-out $(SRC_DIR)/main.cpp, $(CPP_SRCS)) $(C_SRCS)

# Build timing test (C)
$(TIMING_TEST_TARGET): $(BUILD_DIR)/test_timing.o $(C_OBJS)
	@echo "Building timing test..."
	$(CC) $(CFLAGS) -o $@ $^

# Build Thumb timing test (C)
$(THUMB_TEST_TARGET): $(BUILD_DIR)/test_thumb_timing.o $(C_OBJS)
	@echo "Building Thumb timing test..."
	$(CC) $(CFLAGS) -o $@ $^

# Build cycle-driven demo (C)
$(DEMO_CYCLE_TARGET): $(BUILD_DIR)/demo_cycle_driven.o $(C_OBJS)
	@echo "Building cycle-driven demo..."
	$(CC) $(CFLAGS) -o $@ $^

# Build Google Test target (if available)
$(TEST_TARGET): $(BUILD_DIR)/test_cpu.o $(LIB_OBJS)
	@echo "Building Google Test target..."
	$(CXX) $(CXXFLAGS) $(GTEST_FLAGS) -o $@ $^

# Build ARM execute phase 1 test
$(ARM_EXECUTE_PHASE1_TEST): $(BUILD_DIR)/test_arm_execute_phase1.o $(C_OBJS)
	@echo "Building ARM execute phase 1 test..."
	$(CC) $(CFLAGS) -o $@ $^

# Build simple benchmark
SIMPLE_BENCHMARK_TARGET = simple_benchmark
$(SIMPLE_BENCHMARK_TARGET): $(TESTS_DIR)/simple_benchmark.cpp $(INCLUDE_DIR)/*.h
	@echo "Building simple benchmark with aggressive optimizations (-O3 -flto)..."
	mkdir -p $(BUILD_DIR)
	$(CXX) $(OPTIMIZED_CXXFLAGS) -o $@ $(TESTS_DIR)/simple_benchmark.cpp $(filter-out $(SRC_DIR)/main.cpp, $(wildcard $(SRC_DIR)/*.cpp)) $(SRC_DIR)/*.c $(PROFILING_LDFLAGS)

# Debug levels for different build types
# 0 = Off (no debug output)
# 1 = Basic (error and info messages only)
# 2 = Verbose (includes debug messages)
# 3 = Very Verbose (includes trace messages)

# Standard debug build with assertions and basic debug output
DEBUG_CXXFLAGS = -std=c++17 -Wall -Wextra -I$(INCLUDE_DIR) -g -O0 -DDEBUG_LEVEL=1
DEBUG_CFLAGS = -std=c99 -Wall -Wextra -I$(INCLUDE_DIR) -g -O0 -DDEBUG_LEVEL=1

# Verbose debug build with full debug output
VERBOSE_DEBUG_CXXFLAGS = -std=c++17 -Wall -Wextra -I$(INCLUDE_DIR) -g -O0 -DDEBUG_LEVEL=2
VERBOSE_DEBUG_CFLAGS = -std=c99 -Wall -Wextra -I$(INCLUDE_DIR) -g -O0 -DDEBUG_LEVEL=2

# Very verbose debug build with all trace messages
TRACE_DEBUG_CXXFLAGS = -std=c++17 -Wall -Wextra -I$(INCLUDE_DIR) -g -O0 -DDEBUG_LEVEL=3
TRACE_DEBUG_CFLAGS = -std=c99 -Wall -Wextra -I$(INCLUDE_DIR) -g -O0 -DDEBUG_LEVEL=3

# Compilation rules

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Build C++ object files from src
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling C++ source: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build C object files from src  
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling C source: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Build test/demo object files from tests directory
$(BUILD_DIR)/%.o: $(TESTS_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling test/demo: $<"
	$(CXX) $(CXXFLAGS) $(GTEST_INCLUDE_FLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TESTS_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling test/demo: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Debug example targets
debug_examples: $(EXAMPLES_DIR)/debug_examples

$(EXAMPLES_DIR)/debug_examples: $(EXAMPLES_DIR)/debug_examples.cpp
	$(CXX) $(DEBUG_CXXFLAGS) -o $@ $^

verbose_debug_examples: $(EXAMPLES_DIR)/debug_examples_verbose

$(EXAMPLES_DIR)/debug_examples_verbose: $(EXAMPLES_DIR)/debug_examples.cpp
	$(CXX) $(VERBOSE_DEBUG_CXXFLAGS) -o $@ $^

trace_debug_examples: $(EXAMPLES_DIR)/debug_examples_trace

$(EXAMPLES_DIR)/debug_examples_trace: $(EXAMPLES_DIR)/debug_examples.cpp
	$(CXX) $(TRACE_DEBUG_CXXFLAGS) -o $@ $^

release_examples: $(EXAMPLES_DIR)/debug_examples_release

$(EXAMPLES_DIR)/debug_examples_release: $(EXAMPLES_DIR)/debug_examples.cpp
	$(CXX) $(OPTIMIZED_CXXFLAGS) -o $@ $^

# Debug system test targets
debug_test_debug: tests/debug_system_test.cpp
	$(CXX) $(VERBOSE_DEBUG_CXXFLAGS) -o tests/debug_system_test_debug $^

debug_test_release: tests/debug_system_test.cpp
	$(CXX) $(OPTIMIZED_CXXFLAGS) -o tests/debug_system_test_release $^

debug_system_test: debug_test_debug debug_test_release
	@echo "\nRunning debug system test in DEBUG mode:"
	@./tests/debug_system_test_debug
	@echo "\nRunning debug system test in RELEASE mode:"
	@./tests/debug_system_test_release

# Utility targets

# Clean build files
clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET) $(ARM_TEST_TARGET) $(ARM_DEMO_TARGET) $(ARM_BENCHMARK_TARGET) $(ARM_BENCHMARK_OPT_TARGET) $(ARM_BENCHMARK_PROF_TARGET) $(SIMPLE_BENCHMARK_TARGET) $(TIMING_TEST_TARGET) $(THUMB_TEST_TARGET) $(DEMO_CYCLE_TARGET) $(EXAMPLES_DIR)/debug_examples $(EXAMPLES_DIR)/debug_examples_verbose $(EXAMPLES_DIR)/debug_examples_trace $(EXAMPLES_DIR)/debug_examples_release

# Run individual tests and demos
run-arm-test: $(ARM_TEST_TARGET)
	@echo "Running ARM basic test..."
	./$(ARM_TEST_TARGET)

run-arm-demo: $(ARM_DEMO_TARGET)
	@echo "Running ARM advanced demo..."
	./$(ARM_DEMO_TARGET)

run-arm-benchmark: $(ARM_BENCHMARK_TARGET)
	@echo "Running ARM benchmark test..."
	./$(ARM_BENCHMARK_TARGET)

run-arm-benchmark-opt: $(ARM_BENCHMARK_OPT_TARGET)
	@echo "Running optimized ARM benchmark test..."
	./$(ARM_BENCHMARK_OPT_TARGET)

run-arm-benchmark-prof: $(ARM_BENCHMARK_PROF_TARGET)
	@echo "Running profiling-enabled ARM benchmark test..."
	CPUPROFILE=arm_benchmark.prof ./$(ARM_BENCHMARK_PROF_TARGET)
	@echo "\nProfile data written to arm_benchmark.prof"
	@echo "Analyze with: pprof --web $(ARM_BENCHMARK_PROF_TARGET) arm_benchmark.prof"
	@echo "  or: pprof --pdf $(ARM_BENCHMARK_PROF_TARGET) arm_benchmark.prof > profile.pdf"

run-timing-test: $(TIMING_TEST_TARGET)
	@echo "Running timing test..."
	./$(TIMING_TEST_TARGET)

run-thumb-test: $(THUMB_TEST_TARGET)
	@echo "Running Thumb timing test..."
	./$(THUMB_TEST_TARGET)

run-cycle-demo: $(DEMO_CYCLE_TARGET)
	@echo "Running cycle-driven demo..."
	./$(DEMO_CYCLE_TARGET)

run-test: $(TEST_TARGET)
	@echo "Running Google Test suite..."
	./$(TEST_TARGET)

# Run ARM execute phase 1 test
run-arm-execute-phase1: $(ARM_EXECUTE_PHASE1_TEST)
	@echo "Running ARM execute phase 1 test..."
	./$(ARM_EXECUTE_PHASE1_TEST)

# Run simple benchmark
run-simple-benchmark: $(SIMPLE_BENCHMARK_TARGET)
	@echo "Running simple benchmark..."
	./$(SIMPLE_BENCHMARK_TARGET)

# Profile target using gperftools
PROF_OUTPUT = cpu_profile.prof

profile-simple-benchmark: $(SIMPLE_BENCHMARK_TARGET)
	@echo "Profiling simple benchmark with gperftools..."
	CPUPROFILE=./$(PROF_OUTPUT) ./$(SIMPLE_BENCHMARK_TARGET)
	@echo "\nProfile data written to ./$(PROF_OUTPUT)"
	@echo "To view the profile, run: pprof --web ./$(SIMPLE_BENCHMARK_TARGET) ./$(PROF_OUTPUT)"
	@echo "or: pprof --text ./$(SIMPLE_BENCHMARK_TARGET) ./$(PROF_OUTPUT)"

# Run all available tests
run-all-tests: run-timing-test run-thumb-test run-arm-test run-cycle-demo

# Quick build-and-test workflow
quick-test: clean $(ARM_TEST_TARGET) run-arm-test

# Documentation
docs:
	@echo "Documentation files:"
	@find $(DOCS_DIR) -name "*.md" 2>/dev/null || echo "No documentation found"

# Status and information
status:
	@echo "=== GBA Emulator Build Status ==="
	@echo "Source files (C++): $(words $(CPP_SRCS))"
	@echo "Source files (C): $(words $(C_SRCS))"
	@echo "Available targets:"
	@echo "  Main: $(TARGET)"
	@echo "  Tests: $(ARM_TEST_TARGET), $(TIMING_TEST_TARGET), $(THUMB_TEST_TARGET)"
	@echo "  Demos: $(ARM_DEMO_TARGET), $(DEMO_CYCLE_TARGET)"
	@echo "Build directory: $(BUILD_DIR)"

# Help target
help:
	@echo "=== GBA Emulator Makefile Help ==="
	@echo ""
	@echo "Main targets:"
	@echo "  all              - Build main emulator ($(TARGET))"
	@echo "  tests            - Build all test targets"
	@echo "  clean            - Remove all build files"
	@echo ""
	@echo "Individual build targets:"
	@echo "  $(TARGET)        - Main GBA emulator"
	@echo "  $(ARM_TEST_TARGET) - ARM CPU basic tests"
	@echo "  $(ARM_DEMO_TARGET) - ARM CPU advanced demo"
	@echo "  $(ARM_BENCHMARK_TARGET) - ARM CPU benchmark test"
	@echo "  $(ARM_BENCHMARK_OPT_TARGET) - ARM CPU optimized benchmark test (-O3 -flto)"
	@echo "  $(SIMPLE_BENCHMARK_TARGET) - Simple ARM CPU benchmark (no Google Test)"
	@echo "  $(ARM_BENCHMARK_PROF_TARGET) - ARM CPU benchmark test with profiling"
	@echo "  $(TIMING_TEST_TARGET)   - Core timing system tests"
	@echo "  $(THUMB_TEST_TARGET) - Thumb instruction timing tests"
	@echo "  $(DEMO_CYCLE_TARGET) - Cycle-driven execution demo"
	@echo "  $(TEST_TARGET)   - Google Test suite (if available)"
	@echo ""
	@echo "Run targets:"
	@echo "  run-arm-test     - Build and run ARM basic tests"
	@echo "  run-arm-demo     - Build and run ARM advanced demo"
	@echo "  run-arm-benchmark - Build and run ARM benchmark test"
	@echo "  run-arm-benchmark-opt - Build and run optimized ARM benchmark test"
	@echo "  run-simple-benchmark - Build and run simple benchmark (no Google Test)"
	@echo "  profile-simple-benchmark - Profile simple benchmark with gperftools"
	@echo "  run-arm-benchmark-prof - Build and run ARM benchmark test with profiling"
	@echo "  run-timing-test  - Build and run timing tests"
	@echo "  run-thumb-test   - Build and run Thumb timing tests"
	@echo "  run-cycle-demo   - Build and run cycle-driven demo"
	@echo "  run-simple-benchmark - Build and run simple benchmark"
	@echo "  run-all-tests    - Run all available tests"
	@echo "  quick-test       - Clean, build, and run ARM tests"
	@echo ""
	@echo "Information targets:"
	@echo "  status           - Show build status and file counts"
	@echo "  docs             - List available documentation"
	@echo "  help             - Show this help message"

# Phony targets  
.PHONY: all tests clean run-arm-test run-arm-demo run-arm-benchmark run-arm-benchmark-opt run-arm-benchmark-prof run-timing-test run-thumb-test \
        run-cycle-demo run-test run-all-tests quick-test docs status help debug_examples verbose_debug_examples trace_debug_examples release_examples
