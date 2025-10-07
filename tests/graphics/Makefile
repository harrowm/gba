# Makefile for Graphics tests

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -I../../include -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lgtest -lgtest_main -pthread -lkeystone -lcapstone

# Directories
BUILD_DIR = build
SRC_DIR = ../../src
INCLUDE_DIR = ../../include

# Source files from main project
CPP_SRCS = \
	$(SRC_DIR)/cpu.cpp \
	$(SRC_DIR)/arm_cpu.cpp \
	$(SRC_DIR)/thumb_cpu.cpp \
	$(SRC_DIR)/memory.cpp \
	$(SRC_DIR)/interrupt.cpp \
	$(SRC_DIR)/scheduler.cpp \
	$(SRC_DIR)/timer_controller.cpp \
	$(SRC_DIR)/dma.cpp \
	$(SRC_DIR)/gba.cpp \
	$(SRC_DIR)/gpu.cpp \
	$(SRC_DIR)/debug.cpp \
	$(SRC_DIR)/arm_timing.c \
	$(SRC_DIR)/thumb_timing.c \
	$(SRC_DIR)/timing.c \
	$(SRC_DIR)/arm_exec_data_processing.cpp \
	$(SRC_DIR)/arm_exec_multiply.cpp \
	$(SRC_DIR)/arm_exec_single_data_transfers.cpp \
	$(SRC_DIR)/arm_exec_other.cpp \
	$(SRC_DIR)/arm_further_decode.cpp

# Test source files
TEST_SRCS = test_palette.cpp test_tile_decoding.cpp test_tile_render.cpp test_dispcnt.cpp test_bgcnt.cpp test_tilemap.cpp test_scroll.cpp test_bg_render.cpp

# Object files
CPP_OBJS = $(CPP_SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
CPP_OBJS := $(CPP_OBJS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TEST_OBJS = $(TEST_SRCS:%.cpp=$(BUILD_DIR)/%.o)

# Output binary
TARGET = graphics_tests

# Default target
all: $(BUILD_DIR) $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link test executable
$(TARGET): $(CPP_OBJS) $(TEST_OBJS)
	$(CXX) $(CPP_OBJS) $(TEST_OBJS) -o $(TARGET) $(LDFLAGS)

# Compile test sources
$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile project C++ sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile project C sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Run tests
test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test
