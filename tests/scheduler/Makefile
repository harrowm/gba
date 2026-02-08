# Makefile for Scheduler Tests
CXX = g++
CC = gcc
CXXFLAGS = -std=c++20 -Wall -Wextra -I../../include -I/opt/homebrew/opt/googletest/include -g -O2
CFLAGS = -std=c99 -Wall -Wextra -I../../include -g -O2
LDFLAGS = -L/opt/homebrew/opt/googletest/lib -lgtest -lgtest_main -lpthread

# Directories
SRC_DIR = ../../src
BUILD_DIR = build
TARGET1 = test_scheduler
TARGET2 = test_timing_integration
TARGET3 = test_integration_basic
TARGETS = $(TARGET1) $(TARGET2) $(TARGET3)

# Source files
SOURCES1 = test_scheduler.cpp \
           $(SRC_DIR)/scheduler.cpp

SOURCES2 = test_timing_integration.cpp \
           $(SRC_DIR)/scheduler.cpp

# Object files
OBJECTS1 = $(BUILD_DIR)/test_scheduler.o \
           $(BUILD_DIR)/scheduler.o

OBJECTS2 = $(BUILD_DIR)/test_timing_integration.o \
           $(BUILD_DIR)/scheduler.o

OBJECTS3 = $(BUILD_DIR)/test_integration_basic.o \
           $(BUILD_DIR)/scheduler.o

# Default target
all: $(BUILD_DIR) $(TARGETS)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build test_scheduler
$(TARGET1): $(OBJECTS1)
	$(CXX) $(OBJECTS1) -o $(TARGET1) $(LDFLAGS)

# Build test_timing_integration
$(TARGET2): $(OBJECTS2)
	$(CXX) $(OBJECTS2) -o $(TARGET2) $(LDFLAGS)

# Build test_integration_basic
$(TARGET3): $(OBJECTS3)
	$(CXX) $(OBJECTS3) -o $(TARGET3) $(LDFLAGS)

# Compile test files
$(BUILD_DIR)/test_scheduler.o: test_scheduler.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_timing_integration.o: test_timing_integration.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_integration_basic.o: test_integration_basic.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile scheduler
$(BUILD_DIR)/scheduler.o: $(SRC_DIR)/scheduler.cpp

# Run tests
test: $(TARGETS)
	@echo "Running scheduler tests..."
	./$(TARGET1)
	@echo ""
	@echo "Running timing integration tests..."
	./$(TARGET2)
	@echo ""
	@echo "Running basic integration tests..."
	./$(TARGET3)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGETS)

.PHONY: all test clean
