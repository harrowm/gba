#!/usr/bin/env python3

import os
import re
import sys

def refactor_test_file(filename):
    """Refactor a single test file to use the new base class"""
    
    if not os.path.exists(filename):
        print(f"Warning: {filename} not found")
        return False
        
    # Create backup
    backup_name = filename + ".backup"
    if not os.path.exists(backup_name):
        with open(filename, 'r') as f:
            with open(backup_name, 'w') as backup:
                backup.write(f.read())
    
    with open(filename, 'r') as f:
        content = f.read()
    
    # Extract test number from filename
    match = re.search(r'test_thumb(\d+)\.cpp', filename)
    if not match:
        print(f"Could not extract test number from {filename}")
        return False
    
    test_num = match.group(1)
    
    # 1. Replace header and includes
    header_pattern = r'// test_thumb\d+\.cpp.*?\n#include.*?\n.*?extern "C" \{[^}]*\}\s*'
    new_header = f'''// test_thumb{test_num}.cpp - Refactored Thumb CPU test fixture
#include "thumb_test_base.h"

'''
    content = re.sub(header_pattern, new_header, content, flags=re.DOTALL)
    
    # 2. Replace class definition and remove old helper methods
    class_pattern = r'class ThumbCPUTest.*?\{.*?(?=TEST_F)'
    new_class = f'''class ThumbCPUTest{test_num} : public ThumbCPUTestBase {{
protected:
    void SetUp() override {{
        ThumbCPUTestBase::SetUp();
    }}
}};

'''
    content = re.sub(class_pattern, new_class, content, flags=re.DOTALL)
    
    # 3. Update TEST_F declarations
    content = re.sub(r'TEST_F\(ThumbCPUTest,', f'TEST_F(ThumbCPUTest{test_num},', content)
    
    # 4. Replace function calls
    replacements = [
        (r'assemble_and_write_thumb', 'assembleAndWriteThumb'),
        (r'thumb_cpu\.execute', 'execute'),
        (r'cpu\.R\(\)\[', 'registers()['),
        (r'write_thumb16', 'writeInstruction'),
    ]
    
    for old, new in replacements:
        content = re.sub(old, new, content)
    
    # 5. Replace setup_registers calls
    # Simple single register case: setup_registers({{N, VALUE}});
    def replace_simple_setup(match):
        reg_num = match.group(1)
        value = match.group(2)
        return f'registers()[{reg_num}] = {value};'
    
    content = re.sub(r'setup_registers\(\{\{(\d+), ([^}]+)\}\}\);', replace_simple_setup, content)
    
    # Complex multiple register case: setup_registers({{N1, V1}, {N2, V2}});
    def replace_complex_setup(match):
        pairs_str = match.group(1)
        pairs = re.findall(r'\{(\d+), ([^}]+)\}', pairs_str)
        result = []
        for reg_num, value in pairs:
            result.append(f'registers()[{reg_num}] = {value};')
        return '\n    '.join(result)
    
    content = re.sub(r'setup_registers\(\{((?:\{\d+, [^}]+\},?\s*)+)\}\);', replace_complex_setup, content)
    
    # Write the refactored content
    with open(filename, 'w') as f:
        f.write(content)
    
    print(f"Completed refactoring {filename}")
    return True

def main():
    # Refactor test files 1-18
    success_count = 0
    for i in range(1, 19):
        filename = f"test_thumb{i:02d}.cpp"
        if refactor_test_file(filename):
            success_count += 1
    
    print(f"Successfully refactored {success_count} test files")
    
if __name__ == "__main__":
    main()
