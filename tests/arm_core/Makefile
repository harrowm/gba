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



# Minimal source list for run_basic
RUN_BASIC_SRCS = run_basic.cpp \
  ../../src/arm_cpu.cpp \
  ../../src/debug.cpp \
  ../../src/arm_decode.cpp \
  ../../src/cpu.cpp \
  ../../src/memory.cpp \
  ../../src/thumb_cpu.cpp \
  ../../src/arm_timing.c \
  ../../src/thumb_timing.c \
  ../../src/timer.c \
  ../../src/timing.c \
  ../../src/debug.cpp \
  ../../src/gba.cpp \
  ../../src/gpu.cpp

# Map each source file to build/obj name
define make_obj
$(BUILD_DIR)/$(notdir $(basename $(1))).o
endef

RUN_BASIC_OBJS = $(foreach src,$(RUN_BASIC_SRCS),$(call make_obj,$(src)))


TARGETS = run_basic 

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

# Ensure build directory exists before compiling object files
$(RUN_BASIC_OBJS) : | $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


# Pattern rules for object files in build directory
$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Additional pattern rules to match sources in src/ and parent dirs
$(BUILD_DIR)/%.o: ../../src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../../src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGETS)