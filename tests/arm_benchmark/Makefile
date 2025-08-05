# Makefile for ARM core tests only

CXX = g++
CC = gcc
CXXFLAGS = -std=c++20 -Wall -Wextra -I../../include -g -O3 -flto -DDEBUG_BUILD $(GTEST_INCLUDE_FLAGS)
CFLAGS = -std=c23 -Wall -Wextra -I../../include -g -O3 -flto -DDEBUG_BUILD

BUILD_DIR = build
BUILD_DIR_PROFILE = build_profile

# Google Test paths (if available)
GTEST_INCLUDE = /opt/homebrew/include
GTEST_LIB = /opt/homebrew/lib
GTEST_INCLUDE_FLAGS = -I$(GTEST_INCLUDE)
GTEST_LINK_FLAGS = -L$(GTEST_LIB) -lgtest -lgtest_main
GTEST_FLAGS = $(GTEST_INCLUDE_FLAGS) $(GTEST_LINK_FLAGS)

ARM_BENCHMARK_SRCS = arm_benchmark.cpp \
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


# Map each source file to build/obj name (normal)
define make_obj
$(BUILD_DIR)/$(notdir $(basename $(1))).o
endef

# Map each source file to build_profile/obj name (profile)
define make_obj_profile
$(BUILD_DIR_PROFILE)/$(notdir $(basename $(1))).o
endef

ARM_BENCHMARK_OBJS = $(foreach src,$(ARM_BENCHMARK_SRCS),$(call make_obj,$(src)))
ARM_BENCHMARK_OBJS_PROFILE = $(foreach src,$(ARM_BENCHMARK_SRCS),$(call make_obj_profile,$(src)))

TARGETS = arm_benchmark_profile

all: $(TARGETS)
	@echo "Running all ARM core tests..."
	@for t in $(TARGETS); do \
		if [ -x $$t ]; then \
			echo "--- Running $$t ---"; \
			./$$t || exit 1; \
		fi; \
	done

CXXFLAGS_PROFILE = $(CXXFLAGS) -pg
LDFLAGS_PROFILE = -pg


# Profile build target uses build_profile objects
arm_benchmark_profile: $(ARM_BENCHMARK_OBJS_PROFILE) ../../include/inst_table.inc
	$(CXX) $(CXXFLAGS_PROFILE) -o $@ $(ARM_BENCHMARK_OBJS_PROFILE) $(GTEST_FLAGS) $(LDFLAGS_PROFILE) -DDEBUG_LEVEL=0

arm_benchmark: $(ARM_BENCHMARK_OBJS) ../../include/inst_table.inc
	$(CXX) $(CXXFLAGS) -o $@ $(ARM_BENCHMARK_OBJS) $(GTEST_FLAGS) -DDEBUG_LEVEL=0 && ./arm_benchmark  --gtest_brief=1


.PHONY: include

.PRECIOUS: ../../include/inst_table.inc
include:
	mkdir -p ../../include

../../include/inst_table.inc: ../../src/inst_table.py include
	python3 ../../src/inst_table.py > ../../include/inst_table.inc


$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR_PROFILE):
	mkdir -p $(BUILD_DIR_PROFILE)


# Pattern rules for object files in build directory (normal)
$(BUILD_DIR)/%.o: %.cpp ../../include/inst_table.inc $(BUILD_DIR) include
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c ../../include/inst_table.inc $(BUILD_DIR) include
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../../src/%.cpp ../../include/inst_table.inc $(BUILD_DIR) include
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../../src/%.c ../../include/inst_table.inc $(BUILD_DIR) include
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../%.cpp ../../include/inst_table.inc $(BUILD_DIR) include
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pattern rules for object files in build_profile directory (profile)
$(BUILD_DIR_PROFILE)/%.o: %.cpp ../../include/inst_table.inc $(BUILD_DIR_PROFILE) include
	$(CXX) $(CXXFLAGS_PROFILE) -c $< -o $@

$(BUILD_DIR_PROFILE)/%.o: %.c ../../include/inst_table.inc $(BUILD_DIR_PROFILE) include
	$(CC) $(CFLAGS) -pg -c $< -o $@

$(BUILD_DIR_PROFILE)/%.o: ../../src/%.cpp ../../include/inst_table.inc $(BUILD_DIR_PROFILE) include
	$(CXX) $(CXXFLAGS_PROFILE) -c $< -o $@

$(BUILD_DIR_PROFILE)/%.o: ../../src/%.c ../../include/inst_table.inc $(BUILD_DIR_PROFILE) include
	$(CC) $(CFLAGS) -pg -c $< -o $@

$(BUILD_DIR_PROFILE)/%.o: ../%.cpp ../../include/inst_table.inc $(BUILD_DIR_PROFILE) include
	$(CXX) $(CXXFLAGS_PROFILE) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGETS)
