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
        -not -path "*/docs/*" \
        \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.c" \)
}

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed. Please install it first."
    exit 1
fi

# Format files or check if they are formatted correctly
if [ "$1" == "--check" ]; then
    echo "Checking code formatting..."
    NEED_FORMAT=0
    
    while IFS= read -r file || [[ -n "$file" ]]; do
        # Skip files that don't exist or aren't regular files
        if [ ! -f "$file" ]; then
            continue
        fi
        
        # Check if this file should be formatted
        if ! clang-format --style=file -output-replacements-xml "$file" | grep -q "<replacement "; then
            echo "✓ $file"
        else
            echo "✗ $file needs formatting"
            NEED_FORMAT=1
        fi
    done < <(find_source_files)
    
    if [ $NEED_FORMAT -eq 1 ]; then
        echo "Some files need formatting. Run './scripts/format_code.sh' to format them."
        exit 1
    else
        echo "All files are formatted correctly!"
        exit 0
    fi
else
    echo "Formatting code..."
    # Don't fail immediately on errors
    set +e
    
    while IFS= read -r file || [[ -n "$file" ]]; do
        # Skip files that don't exist or aren't regular files
        if [ ! -f "$file" ]; then
            continue
        fi
        
        echo "Formatting $file"
        clang-format --style=file -i "$file" || echo "Warning: Failed to format $file"
    done < <(find_source_files)
    
    # Restore error handling
    set -e
    echo "Done!"
fi