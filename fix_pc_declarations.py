#!/usr/bin/env python3
"""
Fix missing pc_addr declarations in test files.
This script finds uses of pc_addr that don't have a declaration in scope.
"""

import re
import os

def fix_pc_addr_declarations(filepath):
    """Fix missing pc_addr declarations in a file."""
    print(f"Processing {filepath}...")
    
    with open(filepath, 'r') as f:
        content = f.read()
    
    original_content = content
    
    # Find all test methods that use pc_addr but don't declare it
    test_pattern = r'TEST_F\([^,]+,\s*\w+\)\s*\{([^}]+)\}'
    tests = re.findall(test_pattern, content, re.DOTALL)
    
    for test_body in tests:
        if 'pc_addr' in test_body and 'uint32_t pc_addr' not in test_body:
            # This test uses pc_addr but doesn't declare it
            print(f"Found test that uses pc_addr but doesn't declare it")
            
            # Find the beginning of this test in the content
            test_start = content.find(test_body)
            if test_start == -1:
                continue
                
            # Find the test function that contains this body
            func_start = content.rfind('TEST_F', 0, test_start)
            if func_start == -1:
                continue
                
            # Find the opening brace of the test
            brace_start = content.find('{', func_start)
            if brace_start == -1:
                continue
                
            # Find the end of the test
            brace_count = 0
            func_end = brace_start
            for i in range(brace_start, len(content)):
                if content[i] == '{':
                    brace_count += 1
                elif content[i] == '}':
                    brace_count -= 1
                    if brace_count == 0:
                        func_end = i
                        break
            
            # Extract the full test function
            full_test = content[func_start:func_end+1]
            
            # Check if this is a loop-based test or simple test
            if 'for (' in full_test and 'registers()[15] =' in full_test:
                # Loop-based test - need to handle differently
                # Look for patterns like: registers()[15] = some_expression;
                # Replace with: uint32_t pc_addr = some_expression; registers()[15] = pc_addr;
                
                # Find the first assignment to registers()[15]
                assignment_pattern = r'registers\(\)\[15\]\s*=\s*([^;]+);'
                assignments = list(re.finditer(assignment_pattern, full_test))
                
                if assignments:
                    new_test = full_test
                    
                    # Process assignments in reverse order to avoid index shifting
                    for match in reversed(assignments):
                        expr = match.group(1).strip()
                        old_assignment = match.group(0)
                        
                        # Skip if this is already using pc_addr
                        if 'pc_addr' in expr:
                            continue
                            
                        # Replace with pc_addr declaration and assignment
                        new_assignment = f'uint32_t pc_addr = {expr};\n        registers()[15] = pc_addr;'
                        new_test = new_test.replace(old_assignment, new_assignment)
                    
                    # Replace the old test with the new one
                    content = content.replace(full_test, new_test)
            
            elif 'uint32_t pc_addr' not in full_test:
                # Simple test - just add pc_addr declaration at the beginning
                lines = full_test.split('\n')
                new_lines = []
                added_pc_addr = False
                
                for line in lines:
                    new_lines.append(line)
                    # Add pc_addr after the opening brace
                    if not added_pc_addr and '{' in line and 'TEST_F' not in line:
                        indent = '    '
                        new_lines.append(indent + '// Set up PC address')
                        new_lines.append(indent + 'uint32_t pc_addr = 0x00000000;')
                        new_lines.append(indent + 'registers()[15] = pc_addr;')
                        new_lines.append('')
                        added_pc_addr = True
                
                new_test = '\n'.join(new_lines)
                content = content.replace(full_test, new_test)
    
    if content != original_content:
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"Fixed {filepath}")
        return True
    else:
        print(f"No changes needed for {filepath}")
        return False

def main():
    # Focus on the files that have build errors
    problematic_files = [
        "/Users/malcolm/gba/tests/thumb_core/test_thumb10.cpp",
    ]
    
    fixed_count = 0
    for filepath in problematic_files:
        if os.path.exists(filepath):
            if fix_pc_addr_declarations(filepath):
                fixed_count += 1
        else:
            print(f"File not found: {filepath}")
    
    print(f"\nFixed {fixed_count} files.")

if __name__ == "__main__":
    main()
