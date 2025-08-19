# Makefile for ARM vs Thumb Google Benchmark Comparison

CXX = g++
CC = gcc
CXXFLAGS = -std=c++20 -Wall -Wextra -I../../include -I$(BENCHMARK_INCLUDE) -I/opt/homebrew/include -g -O3 -flto -DDEBUG_BUILD
BENCHMARK_INCLUDE = /opt/homebrew/include
BENCHMARK_LIB = /opt/homebrew/lib
BENCHMARK_FLAGS = -I$(BENCHMARK_INCLUDE) -L$(BENCHMARK_LIB) -lbenchmark -lpthread

CPP_SRCS = comparison_benchmark.cpp \
	../../src/arm_cpu.cpp \
	../../src/debug.cpp \
	../../src/arm_exec_other.cpp \
	../../src/arm_further_decode.cpp \
	../../src/arm_exec_multiply.cpp \
	../../src/arm_exec_single_data_transfers.cpp \
	../../src/arm_exec_data_processing.cpp \
	../../src/cpu.cpp \
	../../src/memory.cpp \
	../../src/thumb_cpu.cpp \
	../../src/gba.cpp \
	../../src/gpu.cpp

C_SRCS = ../../src/arm_timing.c \
	../../src/thumb_timing.c \
	../../src/timer.c \
	../../src/timing.c

BUILD_DIR = build

CPP_OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SRCS:.cpp=.o)))
C_OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SRCS:.c=.o)))

all: $(BUILD_DIR) comparison_benchmark

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: ../../src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../../src/%.c
	$(CC) -std=c99 -Wall -Wextra -I../../include -g -O3 -flto -c $< -o $@

$(BUILD_DIR)/comparison_benchmark.o: comparison_benchmark.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

comparison_benchmark: $(CPP_OBJS) $(C_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(BENCHMARK_FLAGS) -L/opt/homebrew/lib -lcapstone

.PHONY: clean
clean:
	rm -f comparison_benchmark $(CPP_OBJS) $(C_OBJS)
	rmdir $(BUILD_DIR) 2>/dev/null || true
