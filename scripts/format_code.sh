#!/bin/bash

# format_code.sh - Script to format C/C++ files according to project standards
# Usage: ./format_code.sh [--check]

set -e

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
ROOT_DIR=$(dirname "$SCRIPT_DIR")

# Find all .cpp, .h, .hpp, and .c files in the project
find_source_files() {
    find "$ROOT_DIR" \
        -not -path "*/\.*" \
        -not -path "*/build*/*" \
        -not -path "*/cmake-build*/*" \
        -not -path "*/modules/*" \
        -not -path "*/twister-out/*" \
        -not -path "*/zephyr/*" \
        \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.c" \)
}

# Format files or check if they are formatted correctly
if [ "$1" == "--check" ]; then
    echo "Checking code formatting..."
    NEED_FORMAT=0
    
    for file in $(find_source_files); do
        if ! clang-format --style=file -output-replacements-xml "$file" | grep -q "<replacement "; then
            echo "✓ $file"
        else
            echo "✗ $file needs formatting"
            NEED_FORMAT=1
        fi
    done
    
    if [ $NEED_FORMAT -eq 1 ]; then
        echo "Some files need formatting. Run './scripts/format_code.sh' to format them."
        exit 1
    else
        echo "All files are formatted correctly!"
        exit 0
    fi
else
    echo "Formatting code..."
    for file in $(find_source_files); do
        echo "Formatting $file"
        clang-format --style=file -i "$file"
    done
    echo "Done!"
fi