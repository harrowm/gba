#!/usr/bin/env python3
"""Add stderr debug output to mGBA's log_sp function"""

import re

with open('/Users/malcolm/mgba-instrumented/src/arm/arm.c', 'r') as f:
    content = f.read()

# Find the log_sp function and add a static counter + stderr debug at start
old_log_sp = '''static void log_sp(struct ARMCore* cpu, const char* mode) {
    init_sp_logging();'''

new_log_sp = '''static void log_sp(struct ARMCore* cpu, const char* mode) {
    static int first_call = 1;
    if (first_call) {
        fprintf(stderr, "[SP_TRACE] log_sp called for the first time!\\n");
        first_call = 0;
    }
    init_sp_logging();'''

if old_log_sp in content:
    content = content.replace(old_log_sp, new_log_sp)
    with open('/Users/malcolm/mgba-instrumented/src/arm/arm.c', 'w') as f:
        f.write(content)
    print("Added stderr debug to log_sp")
else:
    print("Could not find old_log_sp pattern")
    # Show what we have
    m = re.search(r'static void log_sp.*?init_sp', content, re.DOTALL)
    if m:
        print("Found:", repr(m.group(0)))
