# Makefile for ARM core tests only
CXX = g++
CC = gcc
CXXFLAGS = -std=c++20 -Wall -Wextra -I../../include -g -O2 -DDEBUG_BUILD $(GTEST_INCLUDE_FLAGS)
CFLAGS = -std=c23 -Wall -Wextra -I../../include -g -O2 -DDEBUG_BUILD

# Google Test paths (if available)
GTEST_INCLUDE = /opt/homebrew/include
GTEST_LIB = /opt/homebrew/lib
GTEST_INCLUDE_FLAGS = -I$(GTEST_INCLUDE)
GTEST_LINK_FLAGS = -L$(GTEST_LIB) -lgtest -lgtest_main
GTEST_FLAGS = $(GTEST_INCLUDE_FLAGS) $(GTEST_LINK_FLAGS)

# Minimal source list for run_basic
RUN_BASIC_SRCS = run_basic.cpp \
  ../../src/arm_cpu.cpp \
  ../../src/cpu.cpp \
  ../../src/memory.cpp \
  ../../src/interrupt_controller.cpp \
  ../../src/thumb_cpu.cpp \
  ../../src/arm_execute_optimizations.c \
  ../../src/arm_execute_phase1.c \
  ../../src/arm_timing.c \
  ../../src/thumb_execute_optimizations.c \
  ../../src/thumb_timing.c \
  ../../src/timer.c \
  ../../src/timing.c \
  ../../src/debug.cpp \
  ../../src/gba.cpp \
  ../../src/gpu.cpp \
  ../../src/instruction_decoder.cpp

RUN_BASIC_OBJS = $(RUN_BASIC_SRCS:.cpp=.o)
RUN_BASIC_OBJS := $(RUN_BASIC_OBJS:.c=.o)

# Minimal source list for demo_arm_advanced
DEMO_ARM_ADV_SRCS = ../demo_arm_advanced.cpp \
  ../../src/arm_cpu.cpp \
  ../../src/cpu.cpp \
  ../../src/memory.cpp \
  ../../src/interrupt_controller.cpp \
  ../../src/thumb_cpu.cpp \
  ../../src/arm_execute_optimizations.c \
  ../../src/arm_execute_phase1.c \
  ../../src/arm_timing.c \
  ../../src/thumb_execute_optimizations.c \
  ../../src/thumb_timing.c \
  ../../src/timer.c \
  ../../src/timing.c \
  ../../src/debug.cpp \
  ../../src/gba.cpp \
  ../../src/gpu.cpp \
  ../../src/instruction_decoder.cpp

DEMO_ARM_ADV_OBJS = $(DEMO_ARM_ADV_SRCS:.cpp=.o)
DEMO_ARM_ADV_OBJS := $(DEMO_ARM_ADV_OBJS:.c=.o)

# Minimal source list for test_arm_execute_phase1
TEST_ARM_EXEC_SRCS = test_arm_execute_phase1.c \
  ../../src/arm_execute_optimizations.c \
  ../../src/arm_execute_phase1.c \
  ../../src/arm_timing.c \
  ../../src/thumb_execute_optimizations.c \
  ../../src/thumb_timing.c \
  ../../src/timer.c \
  ../../src/timing.c

TEST_ARM_EXEC_OBJS = $(TEST_ARM_EXEC_SRCS:.cpp=.o)
TEST_ARM_EXEC_OBJS := $(TEST_ARM_EXEC_OBJS:.c=.o)

TARGETS = run_basic demo_arm_advanced test_arm_execute_phase1

all: $(TARGETS)
	@echo "Running all ARM core tests..."
	@for t in $(TARGETS); do \
		if [ -x $$t ]; then \
			echo "--- Running $$t ---"; \
			./$$t || exit 1; \
		fi; \
	done

run_basic: $(RUN_BASIC_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTEST_FLAGS) -DDEBUG_LEVEL=0 && ./run_basic

demo_arm_advanced: $(DEMO_ARM_ADV_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTEST_FLAGS) -DDEBUG_LEVEL=0 && ./demo_arm_advanced

test_arm_execute_phase1: $(TEST_ARM_EXEC_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ && ./test_arm_execute_phase1

# Pattern rules for object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGETS)
	rm -f demo_arm_advanced.o