#!/usr/bin/env python3
"""
Fix test_thumb18.cpp expectations to account for PC+4 base instead of PC+2.
All branch targets need +2 added to them.
"""
import re

# Read the file
with open('test_thumb18.cpp', 'r') as f:
    content = f.read()

# Fix patterns like "EXPECT_EQ(R(15), 0x00000006u);" by adding 2 to the hex value
def fix_expect(match):
    full_match = match.group(0)
    hex_value = match.group(1)
    comment = match.group(2) if match.group(2) else ""
    
    # Convert hex to int, add 2, convert back
    value = int(hex_value, 16)
    new_value = value + 2
    new_hex = f"0x{new_value:08X}"
    
    return f"EXPECT_EQ(R(15), {new_hex}u);{comment}"

# Pattern to match EXPECT_EQ(R(15), 0xHHHHHHHHu);
pattern = r'EXPECT_EQ\(R\(15\), (0x[0-9A-Fa-f]+)u\);(\s*//.*)?'
fixed_content = re.sub(pattern, fix_expect, content)

# Write back
with open('test_thumb18.cpp', 'w') as f:
    f.write(fixed_content)

print("Fixed all EXPECT_EQ(R(15), ...) statements by adding 2 to expected values")
