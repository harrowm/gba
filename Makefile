# Makefile for GBA Emulator

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -pedantic
# removed -O3 to enable gdb debugging

# Add Google Test include and library paths
GTEST_INCLUDE = /opt/homebrew/include
GTEST_LIB = /opt/homebrew/lib

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Object files
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Target executable
TARGET = gba_emulator

# Test executable
TEST_TARGET = test_cpu
# Find all format test files and the common test file
FORMAT_TEST_SRCS = $(wildcard tests/format*.cpp)
COMMON_TEST_SRCS = tests/test_cpu_common.cpp
# Include all source files except main.cpp
SRC_TEST_SRCS = $(filter-out $(SRC_DIR)/main.cpp, $(wildcard $(SRC_DIR)/*.cpp))

# Combine all test sources
TEST_SRCS = $(FORMAT_TEST_SRCS) $(COMMON_TEST_SRCS) $(SRC_TEST_SRCS)
TEST_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(notdir $(TEST_SRCS)))

# Default rule
all: $(TARGET) $(TEST_TARGET)

# Build target
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Ensure Google Test include path is applied during compilation
CXXFLAGS += -I$(GTEST_INCLUDE)

# Ensure include directory is applied during compilation for tests
CXXFLAGS += -I$(INCLUDE_DIR)

# Add debug flag for gdb
CXXFLAGS += -g

# Move Google Test linker flags to the linking step
LDFLAGS += -L$(GTEST_LIB) -lgtest -lgtest_main

# Build test target
$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

# Build test object files from tests directory
$(BUILD_DIR)/%.o: tests/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Build test object files from src directory  
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET)

# Phony targets
.PHONY: all clean test

test: $(TEST_TARGET)
	./$(TEST_TARGET)
