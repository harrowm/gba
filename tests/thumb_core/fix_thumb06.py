#!/usr/bin/env python3

import re

# Read the file
with open('test_thumb06.cpp', 'r') as f:
    content = f.read()

# Replace patterns
# 1. Replace PC register assignments
content = re.sub(
    r'registers\(\)\[15\] = 0x00000000;',
    'uint32_t pc_addr = 0x00000000;\n    registers()[15] = pc_addr;',
    content
)

# 2. Replace assembleAndWriteThumb calls and add PC reset
content = re.sub(
    r'ASSERT_TRUE\(assembleAndWriteThumb\(([^,]+), registers\(\)\[15\]\)\);',
    r'ASSERT_TRUE(assembleAndWriteThumb(\1, pc_addr));\n    registers()[15] = pc_addr;',
    content
)

# Write back
with open('test_thumb06.cpp', 'w') as f:
    f.write(content)

print("Fixed test_thumb06.cpp")
