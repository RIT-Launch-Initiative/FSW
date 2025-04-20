#!/usr/bin/env python3

import os
import re
import sys
import argparse
from pathlib import Path

# regex for checking naming conventions
CLASS_PATTERN = re.compile(r'\bclass\s+([a-zA-Z_][a-zA-Z0-9_]*)')
NAMESPACE_PATTERN = re.compile(r'\bnamespace\s+([a-zA-Z_][a-zA-Z0-9_]*)')
PUBLIC_FUNC_PATTERN = re.compile(r'public:.*?(\w+)\s*\([^)]*\)', re.DOTALL)
PRIVATE_FUNC_PATTERN = re.compile(r'private:.*?(\w+)\s*\([^)]*\)', re.DOTALL)

def check_file(file_path):
    """Check a single file for naming convention violations."""
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    issues = []
    
    # class names
    for match in CLASS_PATTERN.finditer(content):
        class_name = match.group(1)
        if not class_name.startswith('C') or not class_name[1:2].isupper():
            issues.append(f"Class '{class_name}' should be prefixed with 'C' and use CamelCase")
    
    # namespace names
    for match in NAMESPACE_PATTERN.finditer(content):
        namespace_name = match.group(1)
        if not namespace_name.startswith('N') or not namespace_name[1:2].isupper():
            issues.append(f"Namespace '{namespace_name}' should be prefixed with 'N' and use CamelCase")
    
    # pub function names
    for match in PUBLIC_FUNC_PATTERN.finditer(content):
        func_name = match.group(1)
        if not func_name[0].isupper() and func_name != "operator":
            issues.append(f"Public function '{func_name}' should use capitalized CamelCase")
    
    # priv function names
    for match in PRIVATE_FUNC_PATTERN.finditer(content):
        func_name = match.group(1)
        if func_name[0].isupper() and func_name != "operator":
            issues.append(f"Private function '{func_name}' should use lowercase camelCase")
    
    return issues

def find_source_files(root_dir):
    """Find all source files in the project."""
    source_files = []
    for ext in ['.cpp', '.h', '.hpp', '.c']:
        source_files.extend(list(Path(root_dir).glob(f"**/*{ext}")))
    
    # Filter out build and external directories
    filtered_files = []
    for file in source_files:
        file_str = str(file)
        if (not "/build" in file_str and 
            not "/cmake-build" in file_str and 
            not "/modules/" in file_str and 
            not "/twister-out/" in file_str and 
            not "/zephyr/" in file_str):
            filtered_files.append(file)
    
    return filtered_files

def main():
    parser = argparse.ArgumentParser(description='Check naming conventions in C++ code.')
    parser.add_argument('--root', default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        help='Root directory to scan')
#   TODO
#     parser.add_argument('--fix', action='store_true', help='Automatically fix naming issues (not implemented yet)')
    args = parser.parse_args()
    
    if args.fix:
        print("Auto-fixing is not implemented yet.")
        return 1
    
    files = find_source_files(args.root)
    
    print(f"Checking {len(files)} files for naming convention issues...")
    
    total_issues = 0
    for file in files:
        issues = check_file(file)
        if issues:
            print(f"\n{file}:")
            for issue in issues:
                print(f"  - {issue}")
            total_issues += len(issues)
    
    if total_issues > 0:
        print(f"\nFound {total_issues} naming convention issues.")
        return 1
    else:
        print("\nNo naming convention issues found.")
        return 0

if __name__ == "__main__":
    sys.exit(main())