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
Checks: '-*,readability-*,modernize-*,cppcoreguidelines-*,performance-*,-modernize-use-trailing-return-type,-readability-qualified-auto,-readability-magic-numbers,-cppcoreguidelines-avoid-magic-numbers'
WarningsAsErrors: ''
HeaderFilterRegex: '.*'
FormatStyle: file
CheckOptions:
  - key: readability-identifier-naming.ClassCase
    value: CamelCase
  - key: readability-identifier-naming.ClassPrefix
    value: C
  - key: readability-identifier-naming.NamespaceCase
    value: CamelCase
  - key: readability-identifier-naming.NamespacePrefix
    value: N
  - key: readability-identifier-naming.PrivateMemberPrefix
    value: 
  - key: readability-identifier-naming.PrivateMemberCase
    value: camelCase
  - key: readability-identifier-naming.PublicMethodCase
    value: CamelCase
  - key: readability-identifier-naming.PrivateMethodCase
    value: camelCase
  - key: readability-identifier-naming.ParameterCase
    value: camelCase
  - key: readability-identifier-naming.VariableCase
    value: camelCase
  - key: readability-identifier-naming.ConstantCase
    value: UPPER_CASE
EOF

# Run clang-tidy on all files
echo "Linting files..."
ERRORS=0

for file in $(find_source_files); do
    echo "Checking $file"
    if ! clang-tidy $FIX_FLAG "$file" -- -std=c++17 -I"$ROOT_DIR/include" 2>&1; then
        ERRORS=1
    fi
done

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