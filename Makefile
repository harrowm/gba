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
TEST_SRCS = $(wildcard tests/*.cpp)
TEST_OBJS = $(patsubst tests/%.cpp,$(BUILD_DIR)/%.o,$(TEST_SRCS))

# Include all source files for the test target
TEST_SRCS += $(wildcard $(SRC_DIR)/*.cpp)

# Exclude main.cpp from test builds
TEST_SRCS := $(filter-out $(SRC_DIR)/main.cpp, $(TEST_SRCS))

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

# Build test object files
$(BUILD_DIR)/%.o: tests/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET)

# Phony targets
.PHONY: all clean
