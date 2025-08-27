#!/usr/bin/env python3
"""
Fix all PC-related issues in the refactored test files.
This script addresses several patterns that need fixing:
1. assembleAndWriteThumb calls that use registers()[15] instead of pc_addr
2. Missing pc_addr variable declarations
3. Missing PC reset after assembleAndWriteThumb calls
"""

import re
import os
import glob

def fix_test_file(filepath):
    """Fix PC-related issues in a single test file."""
    print(f"Processing {filepath}...")
    
    with open(filepath, 'r') as f:
        content = f.read()
    
    original_content = content
    
    # Pattern 1: Fix assembleAndWriteThumb calls that use registers()[15]
    # Replace: assembleAndWriteThumb("...", registers()[15])
    # With: assembleAndWriteThumb("...", pc_addr)
    content = re.sub(
        r'assembleAndWriteThumb\(([^,]+),\s*registers\(\)\[15\]\)',
        r'assembleAndWriteThumb(\1, pc_addr)',
        content
    )
    
    # Pattern 2: Add pc_addr variable and PC reset for patterns that look like:
    # ASSERT_TRUE(assembleAndWriteThumb(...));
    # Need to add pc_addr declaration and reset
    
    # First, find tests that don't have pc_addr declared but use assembleAndWriteThumb
    test_functions = re.findall(r'TEST_F\([^,]+,\s*\w+\)\s*\{[^}]+\}', content, re.DOTALL)
    
    for test_func in test_functions:
        # Check if this test uses assembleAndWriteThumb but doesn't have pc_addr
        if 'assembleAndWriteThumb' in test_func and 'uint32_t pc_addr' not in test_func:
            # Extract the test body
            test_start = content.find(test_func)
            test_end = test_start + len(test_func)
            
            # Find the opening brace
            brace_pos = test_func.find('{') + 1
            
            # Create the replacement with pc_addr declaration
            lines = test_func.split('\n')
            new_lines = []
            inserted_pc_addr = False
            
            for line in lines:
                new_lines.append(line)
                # Insert pc_addr after the opening brace and any initial comments
                if not inserted_pc_addr and '{' in line and not line.strip().startswith('//'):
                    # Add pc_addr declaration
                    indent = '    '
                    new_lines.append(indent + '// Set up PC address')
                    new_lines.append(indent + 'uint32_t pc_addr = 0x00000000;')
                    new_lines.append(indent + 'registers()[15] = pc_addr;')
                    new_lines.append('')
                    inserted_pc_addr = True
                
                # Add PC reset after assembleAndWriteThumb calls
                if 'assembleAndWriteThumb' in line and 'ASSERT_TRUE' in line:
                    # Find the indentation
                    indent = len(line) - len(line.lstrip())
                    indent_str = ' ' * indent
                    new_lines.append(indent_str + 'registers()[15] = pc_addr;')
            
            new_test_func = '\n'.join(new_lines)
            content = content.replace(test_func, new_test_func)
    
    # Pattern 3: Fix cases where pc_addr is declared but registers()[15] is set later
    # Look for: registers()[15] = some_value; followed by assembleAndWriteThumb
    # Replace with: pc_addr = some_value; registers()[15] = pc_addr; ... registers()[15] = pc_addr;
    
    # This is more complex, let's handle it case by case in individual test methods
    
    if content != original_content:
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"Fixed {filepath}")
        return True
    else:
        print(f"No changes needed for {filepath}")
        return False

def main():
    # Get all test files that might need fixing
    test_files = []
    
    # Add the specific files mentioned as having issues
    base_dir = "/Users/malcolm/gba/tests/thumb_core"
    for test_num in [6, 10, 13, 15]:
        filepath = os.path.join(base_dir, f"test_thumb{test_num:02d}.cpp")
        if os.path.exists(filepath):
            test_files.append(filepath)
    
    # Also check other test files for similar issues
    for i in range(1, 19):
        filepath = os.path.join(base_dir, f"test_thumb{i:02d}.cpp")
        if os.path.exists(filepath) and filepath not in test_files:
            test_files.append(filepath)
    
    fixed_count = 0
    for filepath in test_files:
        if fix_test_file(filepath):
            fixed_count += 1
    
    print(f"\nFixed {fixed_count} files out of {len(test_files)} processed.")

if __name__ == "__main__":
    main()
