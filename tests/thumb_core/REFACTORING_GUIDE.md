# Thumb Test Refactoring Guide

## Problem
All test files (test_thumb01.cpp through test_thumb19.cpp) contain significant code duplication:
- Identical class setup with Memory, InterruptController, CPU, ThumbCPU
- Duplicate SetUp() and TearDown() methods
- Repeated helper functions (serializeCPUState, validateUnchangedRegisters)
- Identical Keystone assembler initialization code

## Solution: Common Base Class

### 1. Create `thumb_test_base.h`
- Contains `ThumbCPUTestBase` class with all common setup
- Includes `ThumbTestHelpers` utility class
- Provides common helper methods for all tests
- Handles optional Keystone assembler setup

### 2. Benefits of Refactoring

#### Before (Individual test files):
```cpp
class ThumbCPUTest19 : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ThumbCPU thumb_cpu;
    ks_engine* ks;

    ThumbCPUTest19() : memory(true), cpu(memory, interrupts), thumb_cpu(cpu), ks(nullptr) {}

    void SetUp() override {
        // Initialize all registers to 0
        for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
        
        // Set Thumb mode (T flag) and User mode
        cpu.CPSR() = CPU::FLAG_T | 0x10;
        
        // Initialize Keystone...
        if (ks_open(KS_ARCH_ARM, KS_MODE_THUMB, &ks) != KS_ERR_OK) {
            FAIL() << "Failed to initialize Keystone for Thumb mode";
        }
    }

    void TearDown() override {
        if (ks) {
            ks_close(ks);
            ks = nullptr;
        }
    }
    
    // Duplicate helper functions...
};
```

#### After (Using base class):
```cpp
#include "thumb_test_base.h"

class ThumbCPUTest19 : public ThumbCPUTestBase {
    // No additional setup needed - everything is handled by the base class!
};
```

### 3. Code Reduction

#### Lines of Code Reduction per Test File:
- **Before:** ~50-60 lines of boilerplate setup per file
- **After:** ~5-10 lines per file
- **Reduction:** ~85% less boilerplate code

#### Total Project Impact:
- **19 test files** × 50 lines = ~950 lines of duplicated code
- **After refactoring:** ~150 lines total (base class + minimal per-file code)
- **Net reduction:** ~800 lines of code eliminated

### 4. Improved Helper Methods

#### Before:
```cpp
TEST_F(ThumbCPUTest19, SomeTest) {
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    
    memory.write16(0x00000000, 0xF000);
    memory.write16(0x00000002, 0xF802);
    
    std::string beforeState = serializeCPUState19(cpu);
    
    registers[15] = 0x00000000;
    cpu.execute(1);
    cpu.execute(1);
    
    validateUnchangedRegisters19(cpu, beforeState, {14, 15});
}
```

#### After:
```cpp
TEST_F(ThumbCPUTest19, SomeTest) {
    auto& regs = registers();
    regs.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    
    writeInstruction(0x00000000, 0xF000);
    writeInstruction(0x00000002, 0xF802);
    
    std::string beforeState = serializeCPUState();
    
    regs[15] = 0x00000000;
    execute(2);  // Execute 2 cycles
    
    validateUnchangedRegisters(beforeState, {14, 15});
}
```

### 5. Additional Benefits

1. **Consistency:** All tests use the same setup and helper methods
2. **Maintainability:** Changes to common setup only need to be made in one place
3. **Readability:** Tests focus on test logic rather than setup boilerplate
4. **Extensibility:** Easy to add new helper methods that benefit all tests
5. **Error Reduction:** Less copy-paste errors from duplicating boilerplate code

### 6. Migration Steps

1. Create `thumb_test_base.h` with common base class
2. Update `Makefile` to include the new header dependency
3. Refactor each test file one by one:
   - Replace custom class with inheritance from `ThumbCPUTestBase`
   - Remove duplicated setup/teardown code
   - Replace custom helper function calls with base class methods
   - Update function names to use consistent naming

### 7. Keystone Integration

The base class handles optional Keystone support:
- Compiles with or without Keystone available
- Provides `assembleAndWriteThumb()` method when available
- Falls back gracefully when Keystone is not present

This refactoring significantly improves code quality while maintaining all existing functionality.
