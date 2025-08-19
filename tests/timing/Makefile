# Makefile for GBA Timing Test

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -I../../include -I/opt/homebrew/include -g -O3 -flto

CPP_SRCS = test_timing.cpp \
	../../src/gba.cpp \
	../../src/cpu.cpp \
	../../src/arm_cpu.cpp \
	../../src/thumb_cpu.cpp \
	../../src/memory.cpp \
	../../src/debug.cpp \
	../../src/gpu.cpp \
	../../src/arm_exec_other.cpp \
	../../src/arm_exec_data_processing.cpp \
	../../src/arm_exec_multiply.cpp \
	../../src/arm_exec_single_data_transfers.cpp \
	../../src/arm_further_decode.cpp

C_SRCS = ../../src/arm_timing.c \
	../../src/thumb_timing.c \
	../../src/timer.c \
	../../src/timing.c

BUILD_DIR = build

CPP_OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SRCS:.cpp=.o)))
C_OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SRCS:.c=.o)))

all: $(BUILD_DIR) test_timing

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: ../../src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../../src/%.c
	gcc -std=c99 -Wall -Wextra -I../../include -g -O3 -flto -c $< -o $@

$(BUILD_DIR)/test_timing.o: test_timing.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_timing: $(CPP_OBJS) $(C_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -L/opt/homebrew/lib -lcapstone

.PHONY: clean
clean:
	rm -f test_timing $(CPP_OBJS) $(C_OBJS)
	rmdir $(BUILD_DIR) 2>/dev/null || true
