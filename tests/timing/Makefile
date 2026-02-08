# Makefile for GBA Timing Tests

CXX = g++
CC = gcc
CXXFLAGS = -std=c++20 -Wall -Wextra -I../../include -I/opt/homebrew/opt/googletest/include -g -O2
CFLAGS = -std=c99 -Wall -Wextra -I../../include -g -O2
GTEST_FLAGS = -L/opt/homebrew/opt/googletest/lib -lgtest -lgtest_main -lpthread

# Source files for tests
MEMORY_WS_SRCS = test_memory_wait_states.cpp \
	../../src/memory.cpp \
	../../src/scheduler.cpp

TIMING_SRCS = test_timing.cpp \
	../../src/gba.cpp \
	../../src/cpu.cpp \
	../../src/arm_cpu.cpp \
	../../src/thumb_cpu.cpp \
	../../src/memory.cpp \
	../../src/scheduler.cpp \
	../../src/debug.cpp \
	../../src/gpu.cpp \
	../../src/arm_exec_other.cpp \
	../../src/arm_exec_data_processing.cpp \
	../../src/arm_exec_multiply.cpp \
	../../src/arm_exec_single_data_transfers.cpp \
	../../src/arm_further_decode.cpp

C_SRCS = ../../src/arm_timing.c \
	../../src/thumb_timing.c

BUILD_DIR = build

# Object files
MEMORY_WS_OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(MEMORY_WS_SRCS:.cpp=.o)))
TIMING_OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(TIMING_SRCS:.cpp=.o)))
C_OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SRCS:.c=.o)))

TARGETS = test_memory_wait_states test_timing

all: $(BUILD_DIR) $(TARGETS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: ../../src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../../src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_memory_wait_states.o: test_memory_wait_states.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_timing.o: test_timing.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_memory_wait_states: $(MEMORY_WS_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTEST_FLAGS)

test_timing: $(TIMING_OBJS) $(C_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -L/opt/homebrew/lib -lcapstone

.PHONY: clean
clean:
	rm -f $(TARGETS) $(MEMORY_WS_OBJS) $(TIMING_OBJS) $(C_OBJS)
	rmdir $(BUILD_DIR) 2>/dev/null || true
