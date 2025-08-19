# Minimal Makefile for ARM Google Benchmark


CXX = g++
CC = gcc
CXXFLAGS = -std=c++20 -Wall -Wextra -I../../include -I$(BENCHMARK_INCLUDE) -I/opt/homebrew/include -g -O3 -flto -DDEBUG_BUILD
BENCHMARK_INCLUDE = /opt/homebrew/include
BENCHMARK_LIB = /opt/homebrew/lib
BENCHMARK_FLAGS = -I$(BENCHMARK_INCLUDE) -L$(BENCHMARK_LIB) -lbenchmark -lpthread

CPP_SRCS = arm_benchmark.cpp \
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

# Map source files to build directory object files
CPP_OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SRCS:.cpp=.o)))
C_OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SRCS:.c=.o)))

SRC_MAP_CPP = arm_benchmark.cpp $(filter %.cpp,$(CPP_SRCS))
SRC_MAP_C = $(filter %.c,$(C_SRCS))


all: $(BUILD_DIR) arm_benchmark

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Pattern rules for sources in src and local dir
$(BUILD_DIR)/%.o: ../../src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../../src/%.c
	$(CC) -std=c99 -Wall -Wextra -I../../include -g -O3 -flto -c $< -o $@

$(BUILD_DIR)/arm_benchmark.o: arm_benchmark.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

arm_benchmark: $(CPP_OBJS) $(C_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(BENCHMARK_FLAGS) -L/opt/homebrew/lib -lcapstone

.PHONY: clean
clean:
	rm -f arm_benchmark $(CPP_OBJS) $(C_OBJS)
	rmdir $(BUILD_DIR) 2>/dev/null || true
