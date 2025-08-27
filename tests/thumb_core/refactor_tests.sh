#!/bin/bash

# Script to refactor Thumb test files 1-18 to use the new base class architecture

for i in 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18; do
    testfile="test_thumb${i}.cpp"
    echo "Refactoring $testfile..."
    
    if [ ! -f "$testfile" ]; then
        echo "Warning: $testfile not found, skipping"
        continue
    fi
    
    # Create backup
    cp "$testfile" "${testfile}.backup"
    
    # 1. Replace includes and class definition
    sed -i '' '1,/^class ThumbCPUTest/c\
// test_thumb'${i}'.cpp - Refactored Thumb CPU test fixture\
#include "thumb_test_base.h"\
\
class ThumbCPUTest'${i}' : public ThumbCPUTestBase {\
protected:\
    void SetUp() override {\
        ThumbCPUTestBase::SetUp();\
    }\
};' "$testfile"
    
    # 2. Remove old helper functions (everything between class definition and first TEST_F)
    sed -i '' '/^class ThumbCPUTest'${i}'/,/^TEST_F/{
        /^TEST_F/!{
            /^class ThumbCPUTest'${i}'/!{
                /protected:/!{
                    /void SetUp/!{
                        /ThumbCPUTestBase::SetUp/!{
                            /}/!d
                        }
                    }
                }
            }
        }
    }' "$testfile"
    
    # 3. Update TEST_F declarations
    sed -i '' 's/TEST_F(ThumbCPUTest,/TEST_F(ThumbCPUTest'${i}',/g' "$testfile"
    
    # 4. Replace function calls
    sed -i '' 's/assemble_and_write_thumb/assembleAndWriteThumb/g' "$testfile"
    sed -i '' 's/thumb_cpu\.execute/execute/g' "$testfile"
    sed -i '' 's/cpu\.R()\[/registers()[/g' "$testfile"
    sed -i '' 's/write_thumb16/writeInstruction/g' "$testfile"
    
    # 5. Replace setup_registers calls (simple cases)
    sed -i '' 's/setup_registers({{\([0-9]\+\), \([^}]*\)}});/registers()[\1] = \2;/g' "$testfile"
    
    # 6. Replace getFlag calls (if they exist)
    sed -i '' 's/getFlag(/getFlag(/g' "$testfile"
    
    echo "Completed refactoring $testfile"
done

echo "All test files refactored!"
echo "Run 'make clean && make' to test the refactored files."
