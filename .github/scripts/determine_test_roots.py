#!/usr/bin/env python3
import sys
import yaml
import os
import fnmatch
from pathlib import Path
import argparse

def load_mapping(map_file):
    with open(map_file, "r") as f:
        return yaml.safe_load(f)

def get_all_test_roots(mapping):
    return [k for k in mapping.keys() if k != "ALL"]

def main():
    parser = argparse.ArgumentParser(description="Determine which test roots to run based on changed files.")
    parser.add_argument('--map', required=True, help='YAML file with test roots map')
    parser.add_argument('--run-all', action='store_true', help='Run all test roots regardless of changes')
    args = parser.parse_args()

    mapping = load_mapping(args.map)
    all_test_roots = get_all_test_roots(mapping)

    changed_files = [line.strip() for line in sys.stdin if line.strip()]
    if not changed_files and not args.run_all:
        return

    if args.run_all:
        for root in all_test_roots:
            print(root)
        return

    selected = set()
    run_all = False
    for file in changed_files:
        for test_root, entries in mapping.items():
            for entry in entries:
                for pattern in entry.get("paths", []):
                    if not pattern:
                        continue
                    if fnmatch.fnmatch(file, pattern):
                        if test_root == "ALL":
                            run_all = True
                        else:
                            selected.add(test_root)
    if run_all:
        for root in all_test_roots:
            print(root)
    else:
        for root in sorted(selected):
            print(root)

if __name__ == "__main__":
    main()
