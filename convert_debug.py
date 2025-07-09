#!/usr/bin/env python3
import re
import sys

def convert_debug_calls(content):
    """Convert old Debug syntax to new macro syntax"""
    
    # Pattern to match DEBUG_INFO/DEBUG_ERROR calls with string concatenation
    pattern = r'(DEBUG_(?:INFO|ERROR))\(([^;]+)\);'
    
    def convert_match(match):
        macro_name = match.group(1)
        content_str = match.group(2)
        
        # Convert string concatenation to stream syntax
        # Replace " + std::to_string(var) + " with " << var << "
        content_str = re.sub(r'" \+ std::to_string\(([^)]+)\) \+ "', r'" << \1 << "', content_str)
        
        # Handle end cases: " + std::to_string(var)
        content_str = re.sub(r'" \+ std::to_string\(([^)]+)\)$', r'" << \1', content_str)
        
        # Handle start cases: std::to_string(var) + "
        content_str = re.sub(r'^std::to_string\(([^)]+)\) \+ "', r'\1 << "', content_str)
        
        # Handle cases with debug_to_hex_string
        content_str = re.sub(r'" \+ debug_to_hex_string\(([^)]+)\) \+ "', r'" << debug_to_hex_string(\1) << "', content_str)
        content_str = re.sub(r'" \+ debug_to_hex_string\(([^)]+)\)$', r'" << debug_to_hex_string(\1)', content_str)
        
        return f'{macro_name}({content_str});'
    
    # Apply the conversion
    result = re.sub(pattern, convert_match, content)
    return result

# Read the file
with open('/Users/malcolm/gba/src/thumb_cpu.cpp', 'r') as f:
    content = f.read()

# Convert the content
converted = convert_debug_calls(content)

# Write back
with open('/Users/malcolm/gba/src/thumb_cpu.cpp', 'w') as f:
    f.write(converted)

print("Conversion completed for thumb_cpu.cpp")
