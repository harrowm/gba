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
TIMING_TEST_TARGET = test_timing
THUMB_TEST_TARGET = test_thumb_timing
DEMO_CYCLE_TARGET = demo_cycle_driven

# Target for ARM execute phase 1 test
ARM_EXECUTE_PHASE1_TEST = test_arm_execute_phase1

# Default rule
all: $(TARGET)

# Build all tests and demos
tests: $(ARM_TEST_TARGET) $(ARM_DEMO_TARGET) $(TIMING_TEST_TARGET) $(THUMB_TEST_TARGET) $(DEMO_CYCLE_TARGET) $(ARM_EXECUTE_PHASE1_TEST)

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

# Utility targets

# Clean build files
clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET) $(ARM_TEST_TARGET) $(ARM_DEMO_TARGET) $(TIMING_TEST_TARGET) $(THUMB_TEST_TARGET) $(DEMO_CYCLE_TARGET)

# Run individual tests and demos
run-arm-test: $(ARM_TEST_TARGET)
	@echo "Running ARM basic test..."
	./$(ARM_TEST_TARGET)

run-arm-demo: $(ARM_DEMO_TARGET)
	@echo "Running ARM advanced demo..."
	./$(ARM_DEMO_TARGET)

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
	@echo "  $(TIMING_TEST_TARGET)   - Core timing system tests"
	@echo "  $(THUMB_TEST_TARGET) - Thumb instruction timing tests"
	@echo "  $(DEMO_CYCLE_TARGET) - Cycle-driven execution demo"
	@echo "  $(TEST_TARGET)   - Google Test suite (if available)"
	@echo ""
	@echo "Run targets:"
	@echo "  run-arm-test     - Build and run ARM basic tests"
	@echo "  run-arm-demo     - Build and run ARM advanced demo"
	@echo "  run-timing-test  - Build and run timing tests"
	@echo "  run-thumb-test   - Build and run Thumb timing tests"
	@echo "  run-cycle-demo   - Build and run cycle-driven demo"
	@echo "  run-all-tests    - Run all available tests"
	@echo "  quick-test       - Clean, build, and run ARM tests"
	@echo ""
	@echo "Information targets:"
	@echo "  status           - Show build status and file counts"
	@echo "  docs             - List available documentation"
	@echo "  help             - Show this help message"

# Phony targets  
.PHONY: all tests clean run-arm-test run-arm-demo run-timing-test run-thumb-test \
        run-cycle-demo run-test run-all-tests quick-test docs status help
