# Minimal Makefile for Thumb Core Google Test Suite

CXX = g++
CC = gcc
CXXFLAGS = -std=c++20 -Wall -Wextra -I../../include -I/opt/homebrew/include -g -O3 -flto -DDEBUG_BUILD
GTEST_LIB = /opt/homebrew/lib
GTEST_FLAGS = -L$(GTEST_LIB) -lgtest -lgtest_main -lpthread
KEYSTONE_FLAGS = -lkeystone


CPP_SRCS = test_cpu.cpp \
	test_thumb01.cpp \
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
	../../src/timing.c \
	../../src/interrupt.c

BUILD_DIR = build
CPP_OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SRCS:.cpp=.o)))
C_OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SRCS:.c=.o)))


# Regenerate ARM instruction table before building
# Do this as the tests use the BX instruction in the ARM instruction set
INST_TABLE_INC = ../../include/inst_table.inc
INST_TABLE_PY = ../../src/inst_table.py

all: $(BUILD_DIR) $(INST_TABLE_INC) thumb_core_tests

$(INST_TABLE_INC): $(INST_TABLE_PY)
	python3 $(INST_TABLE_PY) > $(INST_TABLE_INC)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: ../../src/%.cpp $(INST_TABLE_INC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../../src/%.c $(INST_TABLE_INC)
	$(CC) -std=c99 -Wall -Wextra -I../../include -g -O3 -flto -c $< -o $@

$(BUILD_DIR)/test_cpu.o: test_cpu.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_thumb01.o: test_thumb01.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

thumb_core_tests: $(CPP_OBJS) $(C_OBJS) $(INST_TABLE_INC)
	$(CXX) $(CXXFLAGS) -o $@ $(CPP_OBJS) $(C_OBJS) $(GTEST_FLAGS) $(KEYSTONE_FLAGS) -L/opt/homebrew/lib -lcapstone

.PHONY: clean
clean:
	rm -f thumb_core_tests $(CPP_OBJS) $(C_OBJS)
	rm -f $(INST_TABLE_INC)
	rmdir $(BUILD_DIR) 2>/dev/null || true
