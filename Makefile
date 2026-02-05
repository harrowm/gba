# Makefile for GBA Emulator

CXX = g++
CC = gcc

# SDL2 configuration
SDL2_CFLAGS = $(shell sdl2-config --cflags)
SDL2_LIBS = $(shell sdl2-config --libs)

# Capstone and Keystone (for ARM disassembly/assembly)
CAPSTONE_FLAGS = -I/opt/homebrew/include
CAPSTONE_LIBS = -L/opt/homebrew/lib -lcapstone -lkeystone

# Compiler flags
CXXFLAGS = -std=c++20 -Wall -Wextra -I./include -g -O2 $(SDL2_CFLAGS) $(CAPSTONE_FLAGS)
CFLAGS = -std=c23 -Wall -Wextra -I./include -g -O2 $(CAPSTONE_FLAGS)

# Build directory
BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include

# Source files
CPP_SOURCES = \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/gba.cpp \
	$(SRC_DIR)/cpu.cpp \
	$(SRC_DIR)/arm_cpu.cpp \
	$(SRC_DIR)/arm_exec_data_processing.cpp \
	$(SRC_DIR)/arm_exec_multiply.cpp \
	$(SRC_DIR)/arm_exec_single_data_transfers.cpp \
	$(SRC_DIR)/arm_exec_other.cpp \
	$(SRC_DIR)/arm_further_decode.cpp \
	$(SRC_DIR)/thumb_cpu.cpp \
	$(SRC_DIR)/memory.cpp \
	$(SRC_DIR)/gpu.cpp \
	$(SRC_DIR)/scheduler.cpp \
	$(SRC_DIR)/interrupt.cpp \
	$(SRC_DIR)/timer_controller.cpp \
	$(SRC_DIR)/dma.cpp \
	$(SRC_DIR)/debug.cpp \
	$(SRC_DIR)/display.cpp \
	$(SRC_DIR)/apu.cpp

C_SOURCES = \
	$(SRC_DIR)/arm_timing.c \
	$(SRC_DIR)/thumb_timing.c \
	$(SRC_DIR)/timing.c \
	$(SRC_DIR)/timer.c

# Object files
CPP_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))
C_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ALL_OBJECTS = $(CPP_OBJECTS) $(C_OBJECTS)

# Target executable
TARGET = gba_emulator

# Default target
all: $(BUILD_DIR) $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link executable
$(TARGET): $(ALL_OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(ALL_OBJECTS) -o $(TARGET) $(SDL2_LIBS) $(CAPSTONE_LIBS)
	@echo "Build complete: $(TARGET)"

# Compile C++ sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile C sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Clean complete"

# Run the emulator
run: $(TARGET)
	./$(TARGET)

# Rebuild everything
rebuild: clean all

# Print build information
info:
	@echo "GBA Emulator Build Configuration"
	@echo "================================"
	@echo "Compiler: $(CXX)"
	@echo "C++ Flags: $(CXXFLAGS)"
	@echo "C Flags: $(CFLAGS)"
	@echo "SDL2 Flags: $(SDL2_CFLAGS)"
	@echo "SDL2 Libs: $(SDL2_LIBS)"
	@echo "Target: $(TARGET)"
	@echo "Sources: $(words $(CPP_SOURCES)) C++ files, $(words $(C_SOURCES)) C files"

.PHONY: all clean run rebuild info
