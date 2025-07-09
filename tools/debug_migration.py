#!/usr/bin/env python3
"""
Debug Code Migration Tool

This script assists with migrating from direct debug function calls to the macro-based debug system.
It scans through source files and suggests replacements for common debug patterns.

Usage:
    python3 debug_migration.py [directory]

If no directory is specified, it will use the current directory.
"""

import os
import re
import sys
import argparse
from pathlib import Path
from typing import List, Tuple

# Patterns to match and their replacements
PATTERNS = [
    # Direct log calls
    (r'Debug::log::error\("([^"]*)"\)', r'DEBUG_LOG_ERROR("\1")'),
    (r'Debug::log::info\("([^"]*)"\)', r'DEBUG_LOG_INFO("\1")'),
    (r'Debug::log::debug\("([^"]*)"\)', r'DEBUG_LOG_DEBUG("\1")'),
    (r'Debug::log::trace\("([^"]*)"\)', r'DEBUG_LOG_TRACE("\1")'),
    
    # Basic conditional logging
    (r'if\s*\(\s*Debug::Config::debugLevel\s*>=\s*Debug::Level::Basic\s*\)\s*{\s*Debug::log::info\("([^"]*)"\);\s*}', 
     r'DEBUG_LOG_INFO("\1")'),
    
    (r'if\s*\(\s*Debug::Config::debugLevel\s*>=\s*Debug::Level::Verbose\s*\)\s*{\s*Debug::log::debug\("([^"]*)"\);\s*}', 
     r'DEBUG_LOG_DEBUG("\1")'),
     
    (r'if\s*\(\s*Debug::Config::debugLevel\s*>=\s*Debug::Level::VeryVerbose\s*\)\s*{\s*Debug::log::trace\("([^"]*)"\);\s*}', 
     r'DEBUG_LOG_TRACE("\1")'),
     
    # Include file replacement
    (r'#include\s+"debug\.h"', r'#include "debug_macros.h"'),
    
    # Hex string conversion
    (r'Debug::toHexString\(([^,]+),\s*([^)]+)\)', r'DEBUG_TO_HEX_STRING(\1, \2)'),
]

def find_source_files(directory: str) -> List[Path]:
    """Find all C++ source files in the given directory."""
    extensions = [".c", ".cpp", ".h", ".hpp"]
    result = []
    for path in Path(directory).rglob("*"):
        if path.is_file() and path.suffix in extensions:
            result.append(path)
    return result

def scan_file(file_path: Path) -> List[Tuple[int, str, str]]:
    """Scan a file for patterns to replace."""
    replacements = []
    with open(file_path, "r") as f:
        lines = f.readlines()
        
    for i, line in enumerate(lines):
        for pattern, replacement in PATTERNS:
            if re.search(pattern, line):
                replaced_line = re.sub(pattern, replacement, line)
                if replaced_line != line:
                    replacements.append((i + 1, line.strip(), replaced_line.strip()))
                    
    return replacements

def process_file(file_path: Path, dry_run: bool = True) -> bool:
    """Process a file and apply or suggest replacements."""
    replacements = scan_file(file_path)
    if not replacements:
        return False
        
    print(f"\n{file_path}:")
    for line_num, old_line, new_line in replacements:
        print(f"  Line {line_num}")
        print(f"    - {old_line}")
        print(f"    + {new_line}")
        
    if not dry_run:
        with open(file_path, "r") as f:
            content = f.read()
            
        for _, old_pattern, new_pattern in replacements:
            content = content.replace(old_pattern, new_pattern)
            
        with open(file_path, "w") as f:
            f.write(content)
        
        print(f"  Applied {len(replacements)} replacements.")
    
    return True

def main():
    parser = argparse.ArgumentParser(description="Debug Code Migration Tool")
    parser.add_argument("directory", nargs="?", default=".", help="Directory to scan (default: current directory)")
    parser.add_argument("--apply", action="store_true", help="Apply the suggested changes")
    args = parser.parse_args()
    
    directory = args.directory
    dry_run = not args.apply
    
    if dry_run:
        print("Running in dry run mode. Use --apply to apply changes.")
    else:
        print("Running in apply mode. Changes will be applied to files.")
        
    print(f"Scanning directory: {directory}")
    
    source_files = find_source_files(directory)
    print(f"Found {len(source_files)} source files")
    
    files_with_replacements = 0
    total_replacements = 0
    
    for file_path in source_files:
        replacements = scan_file(file_path)
        if replacements:
            files_with_replacements += 1
            total_replacements += len(replacements)
            process_file(file_path, dry_run)
    
    print(f"\nSummary:")
    print(f"  Files with suggested replacements: {files_with_replacements}")
    print(f"  Total replacements suggested: {total_replacements}")
    
    if dry_run and total_replacements > 0:
        print("\nRun with --apply to apply these changes.")

if __name__ == "__main__":
    main()
