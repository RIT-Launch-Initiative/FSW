#!/bin/bash

# lint_code.sh - Script to lint C/C++ code
# Usage: ./lint_code.sh [--fix]

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

# Check if clang-tidy is installed
if ! command -v clang-tidy &> /dev/null; then
    echo "Error: clang-tidy is not installed. Please install it first."
    exit 1
fi

# Check if fixing or just checking
FIX_FLAG=""
if [ "$1" == "--fix" ]; then
    FIX_FLAG="-fix"
    echo "Running in fix mode. Changes will be applied automatically."
else
    echo "Running in check mode. No changes will be applied."
fi

# Create a temporary clang-tidy config file
CLANG_TIDY_CONFIG=".clang-tidy"
cat > "$ROOT_DIR/$CLANG_TIDY_CONFIG" << EOF
---
Checks: '-*,readability-*,modernize-*,cppcoreguidelines-*,performance-*,-modernize-use-trailing-return-type,-readability-qualified-auto,-readability-magic-numbers,-cppcoreguidelines-avoid-magic-numbers'
WarningsAsErrors: ''
HeaderFilterRegex: '.*'
FormatStyle: file
CheckOptions:
  readability-identifier-naming.ClassCase: CamelCase
  readability-identifier-naming.ClassPrefix: C
  readability-identifier-naming.NamespaceCase: CamelCase
  readability-identifier-naming.NamespacePrefix: N
  readability-identifier-naming.PrivateMemberCase: camelCase
  readability-identifier-naming.PublicMethodCase: CamelCase
  readability-identifier-naming.PrivateMethodCase: camelCase
  readability-identifier-naming.ParameterCase: camelCase
  readability-identifier-naming.VariableCase: camelCase
  readability-identifier-naming.ConstantCase: UPPER_CASE
...
EOF

# Run clang-tidy on all files
echo "Linting files..."
ERRORS=0

# Don't fail immediately on linting errors - treat as warnings
set +e

for file in $(find_source_files); do
    echo "Checking $file"
    
    # Check file extension to determine if it's C or C++
    if [[ "$file" == *.c ]]; then
        # C file
        clang-tidy $FIX_FLAG "$file" -- -std=c11 -I"$ROOT_DIR/include" -I"$ROOT_DIR/zephyr/include" 2>&1 || {
            echo "Warning: Linting issues in $file (C file)"
            ERRORS=1
        }
    else
        # C++ file
        clang-tidy $FIX_FLAG "$file" -- -std=c++17 -I"$ROOT_DIR/include" -I"$ROOT_DIR/zephyr/include" 2>&1 || {
            echo "Warning: Linting issues in $file (C++ file)"
            ERRORS=1
        }
    fi
done

# Restore error handling
set -e

# Clean up config
rm -f "$ROOT_DIR/$CLANG_TIDY_CONFIG"

# Exit if issues were found
if [ $ERRORS -eq 0 ]; then
    echo "All files pass linting!"
    exit 0
else
    echo "Some files have linting issues."
    if [ -z "$FIX_FLAG" ]; then
        echo "Run './scripts/lint_code.sh --fix' to attempt automatic fixes."
    fi
    exit 1
fi