#!/usr/bin/env python3

import re
import os

def fix_pc_relative_tests(filename):
    """Fix PC-relative test patterns in a file"""
    if not os.path.exists(filename):
        print(f"File {filename} not found")
        return
        
    with open(filename, 'r') as f:
        content = f.read()
    
    # Pattern for PC-relative test functions
    test_pattern = r'(TEST_F\([^,]+,\s*[^)]+\)\s*\{[^}]*?)registers\(\)\[15\]\s*=\s*([^;]+);([^}]*?)ASSERT_TRUE\(assembleAndWriteThumb\([^)]+\)\);([^}]*?)\}(?=\s*TEST_F|\s*$)'
    
    def fix_test(match):
        test_start = match.group(1)
        pc_value = match.group(2)
        middle_part = match.group(3)
        end_part = match.group(4)
        
        # Create the fixed version
        fixed = f"""{test_start}uint32_t pc_addr = {pc_value};
    registers()[15] = pc_addr;
    
    ASSERT_TRUE(assembleAndWriteThumb({middle_part.strip().split('assembleAndWriteThumb(')[1].split(')')[0]}), pc_addr));
    registers()[15] = pc_addr;
{middle_part}{end_part}}}"""
        
        return fixed
    
    # Apply the fix
    fixed_content = re.sub(test_pattern, fix_test, content, flags=re.DOTALL)
    
    # Write back
    with open(filename, 'w') as f:
        f.write(fixed_content)
    
    print(f"Fixed PC-relative tests in {filename}")

# Fix all the failing test files
fix_pc_relative_tests("test_thumb06.cpp")
# We'll handle other files manually as they may have different issues
