#!/bin/bash

# Script to convert old Debug syntax to new macro syntax in thumb_cpu.cpp

FILE="/Users/malcolm/gba/src/thumb_cpu.cpp"

# Backup the original file
cp "$FILE" "$FILE.backup"

# Convert string concatenation style debug calls to stream style
# This handles patterns like: DEBUG_INFO("text" + std::to_string(var) + "more text")

# Use a more sophisticated sed approach
# First, let's handle simple cases

# Convert basic string concatenation patterns
sed -i '' 's/DEBUG_INFO("Executing Thumb \([^"]*\)" + std::to_string(\([^)]*\)) + "\([^"]*\)" + std::to_string(\([^)]*\)) + "\([^"]*\)" + std::to_string(\([^)]*\)) + "\([^"]*\)");/DEBUG_INFO("Executing Thumb \1" << \2 << "\3" << \4 << "\5" << \6 << "\7");/g' "$FILE"

sed -i '' 's/DEBUG_INFO("Executing Thumb \([^"]*\)" + std::to_string(\([^)]*\)) + "\([^"]*\)" + std::to_string(\([^)]*\)) + "\([^"]*\)");/DEBUG_INFO("Executing Thumb \1" << \2 << "\3" << \4 << "\5");/g' "$FILE"

sed -i '' 's/DEBUG_INFO("Executing Thumb \([^"]*\)" + std::to_string(\([^)]*\)) + "\([^"]*\)");/DEBUG_INFO("Executing Thumb \1" << \2 << "\3");/g' "$FILE"

sed -i '' 's/DEBUG_INFO("\([^"]*\)" + std::to_string(\([^)]*\)) + "\([^"]*\)");/DEBUG_INFO("\1" << \2 << "\3");/g' "$FILE"

sed -i '' 's/DEBUG_ERROR("\([^"]*\)" + std::to_string(\([^)]*\)));/DEBUG_ERROR("\1" << \2);/g' "$FILE"

echo "Conversion completed. Check the file and remove backup if satisfied."
