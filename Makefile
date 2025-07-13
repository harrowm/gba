# Makefile for GBA Emulator

# Compiler and flags
CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -Wall -Wextra -I$(INCLUDE_DIR) -g -O2 -DDEBUG_BUILD
CFLAGS = -std=c99 -Wall -Wextra -I$(INCLUDE_DIR) -g -O2 -DDEBUG_BUILD

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
GAMEPAK_500K_TEST_TARGET = gamepak_500k_test

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
$(ARM_BENCHMARK_PROF_TARGET): $(TESTS_DIR)/arm_benchmark.cpp 
	@echo "Building ARM benchmark with profiling enabled..."
	mkdir -p $(BUILD_DIR)
	# Use a simpler approach - compile and link in one command
	$(CXX) $(PROFILING_CXXFLAGS) $(GTEST_FLAGS) -DOUTPUT_BENCHMARK_RESULTS=1 -o $@ $(TESTS_DIR)/arm_benchmark.cpp $(filter-out $(SRC_DIR)/main.cpp, $(CPP_SRCS)) $(C_SRCS) $(PROFILING_LDFLAGS)

# Build ultra-optimized ARM benchmark test
$(ARM_BENCHMARK_ULTRA_TARGET): $(TESTS_DIR)/arm_benchmark_fixed.cpp
	@echo "Building ultra-optimized ARM benchmark with memory bounds checking disabled..."
	mkdir -p $(BUILD_DIR)
	# Use a simpler approach - compile and link in one command
	$(CXX) $(ULTRA_CXXFLAGS) $(GTEST_FLAGS) -DOUTPUT_BENCHMARK_RESULTS=1 -o $@ $(TESTS_DIR)/arm_benchmark_fixed.cpp $(filter-out $(SRC_DIR)/main.cpp, $(CPP_SRCS)) $(C_SRCS)

# Build GamePak 500k test with optimized settings and debug output disabled
$(GAMEPAK_500K_TEST_TARGET): $(TESTS_DIR)/gamepak_500k_test.cpp
	@echo "Building GamePak 500k cycle test with debug output disabled..."
	mkdir -p $(BUILD_DIR)
	$(CXX) $(ULTRA_CXXFLAGS) -o $@ $(TESTS_DIR)/gamepak_500k_test.cpp $(filter-out $(SRC_DIR)/main.cpp, $(CPP_SRCS)) $(C_SRCS)

# The remaining Makefile content follows below...
