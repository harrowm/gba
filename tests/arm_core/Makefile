# Makefile for ARM core tests only

CXX = g++
CC = gcc
CXXFLAGS = -std=c++20 -Wall -Wextra -I../../include -g -O2 -DDEBUG_BUILD $(GTEST_INCLUDE_FLAGS)
CFLAGS = -std=c23 -Wall -Wextra -I../../include -g -O2 -DDEBUG_BUILD
BUILD_DIR = build

# Google Test paths (if available)
GTEST_INCLUDE = /opt/homebrew/include
GTEST_LIB = /opt/homebrew/lib
GTEST_INCLUDE_FLAGS = -I$(GTEST_INCLUDE)
GTEST_LINK_FLAGS = -L$(GTEST_LIB) -lgtest -lgtest_main
GTEST_FLAGS = $(GTEST_INCLUDE_FLAGS) $(GTEST_LINK_FLAGS)

RUN_ALL_SRCS = run_basic.cpp \
  test_arm_single_data_transfers.cpp \
  test_arm_multiply.cpp \
  test_arm_data_processing.cpp \
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
	../../src/arm_timing.c \
	../../src/thumb_timing.c \
	../../src/timer.c \
	../../src/timing.c \
	../../src/gba.cpp \
	../../src/gpu.cpp

# Map each source file to build/obj name
define make_obj
$(BUILD_DIR)/$(notdir $(basename $(1))).o
endef

RUN_ALL_OBJS = $(foreach src,$(RUN_ALL_SRCS),$(call make_obj,$(src)))

TARGETS = run_all

all: $(TARGETS)
	@echo "Running all ARM core tests..."
	@for t in $(TARGETS); do \
		if [ -x $$t ]; then \
			echo "--- Running $$t ---"; \
			./$$t || exit 1; \
		fi; \
	done


run_all: $(RUN_ALL_OBJS) include/inst_table.inc
	$(CXX) $(CXXFLAGS) -o $@ $(RUN_ALL_OBJS) $(GTEST_FLAGS) -DDEBUG_LEVEL=0 && ./run_all

# Ensure build directory exists before compiling object files

.PHONY: include

.PRECIOUS: ../../include/inst_table.inc
include:
	mkdir -p ../../include

../../include/inst_table.inc: ../../src/inst_table.py include
	python3 ../../src/inst_table.py > ../../include/inst_table.inc

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Pattern rules for object files in build directory

# Pattern rules for object files in build directory, all depend on ../../include/inst_table.inc and include/build directories
$(BUILD_DIR)/%.o: %.cpp ../../include/inst_table.inc $(BUILD_DIR) include
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c ../../include/inst_table.inc $(BUILD_DIR) include
	$(CC) $(CFLAGS) -c $< -o $@

# Additional pattern rules to match sources in src/ and parent dirs
$(BUILD_DIR)/%.o: ../../src/%.cpp ../../include/inst_table.inc $(BUILD_DIR) include
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../../src/%.c ../../include/inst_table.inc $(BUILD_DIR) include
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../%.cpp ../../include/inst_table.inc $(BUILD_DIR) include
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGETS)
