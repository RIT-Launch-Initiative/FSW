import sys
import struct
import yaml
import json
import csv
import argparse
from pathlib import Path
from typing import Dict, List, Any

TYPE_SIZES = {
    "float": "f",
    "double": "d",
    "uint8_t": "B",
    "int8_t": "b",
    "uint16_t": "H",
    "int16_t": "h",
    "uint32_t": "I",
    "int32_t": "i",
}

def load_yaml_files(paths: List[str]) -> Dict[str, Any]:
    """Loads all schema files into a single dictionary"""
    schema = {}
    for path in paths:
        with open(path, "r") as f:
            data = yaml.safe_load(f)
            schema.update(data)
    return schema

def get_struct_format(typename: str, schema: Dict[str, Any]) -> str:
    """Recursively generates a struct format string from a type definition."""
    if typename in TYPE_SIZES:
        return TYPE_SIZES[typename]

    if typename not in schema:
        raise ValueError(f"Unknown type: {typename}")

    type_def = schema[typename]
    fmt = ""
    if type_def.get("timestamp"):
        fmt += TYPE_SIZES["uint32_t"]

    for field in type_def["fields"]:
        if "array_size" in field:
            count = eval(str(field["array_size"]))
            fmt += TYPE_SIZES[field["type"]] * count
        else:
            fmt += get_struct_format(field["type"], schema)
    return fmt

def get_size(typename: str, schema: Dict[str, Any]) -> int:
    """Returns the total size in bytes of the given type."""
    fmt = get_struct_format(typename, schema)
    return struct.calcsize(fmt)

def unpack_fields(data: bytes, typename: str, schema: Dict[str, Any], offset=0) -> Dict[str, Any]:
    """Recursively unpacks binary data into a dict"""
    result = {}
    type_def = schema[typename]
    index = offset

    if type_def.get("timestamp"):
        result["timestamp"] = struct.unpack_from("I", data, index)[0]
        index += 4

    for field in type_def["fields"]:
        field_type = field["type"]
        if "array_size" in field:
            count = eval(str(field["array_size"]))
            fmt = TYPE_SIZES[field_type] * count
            size = struct.calcsize(fmt)
            values = struct.unpack_from(fmt, data, index)
            result[field["name"]] = list(values)
            index += size
        elif field_type in TYPE_SIZES:
            fmt = TYPE_SIZES[field_type]
            value = struct.unpack_from(fmt, data, index)[0]
            result[field["name"]] = value
            index += struct.calcsize(fmt)
        else:
            sub_result = unpack_fields(data, field_type, schema, index)
            result[field["name"]] = sub_result
            index += get_size(field_type, schema)

    return result

def flatten(d: Dict[str, Any], parent_key='', sep='.') -> Dict[str, Any]:
    """Flattens nested dict for CSV output."""
    items = []
    for k, v in d.items():
        new_key = f"{parent_key}{sep}{k}" if parent_key else k
        if isinstance(v, dict):
            items.extend(flatten(v, new_key, sep=sep).items())
        else:
            items.append((new_key, v))
    return dict(items)

def parse_args():
    """Parses command line arguments using argparse."""
    parser = argparse.ArgumentParser(
        description="Parse a binary file using a YAML-defined schema and output to JSON and/or CSV."
    )
    parser.add_argument("binary_file", help="Path to the binary file to parse.")
    parser.add_argument("typename", help="Top-level struct name (e.g., SensorData).")
    parser.add_argument("yaml_files", nargs="+", help="YAML schema file(s) describing data layout.")
    parser.add_argument("--csv", metavar="CSV_PATH", help="Path to output CSV file.")
    parser.add_argument("--json", metavar="JSON_PATH", help="Path to output JSON file.")

    return parser.parse_args()

def main():
    args = parse_args()

    bin_path = Path(args.binary_file)
    typename = args.typename
    yaml_paths = args.yaml_files

    schema = load_yaml_files(yaml_paths)
    type_size = get_size(typename, schema)

    records = []
    with open(bin_path, "rb") as f:
        while chunk := f.read(type_size):
            if len(chunk) < type_size:
                print("Skipping incomplete record at end")
                break
            record = unpack_fields(chunk, typename, schema)
            records.append(record)

    if args.json:
        with open(args.json, "w") as jf:
            json.dump(records, jf, indent=2)
        print(f"JSON output written to {args.json}")

    if args.csv:
        with open(args.csv, "w", newline="") as cf:
            writer = csv.DictWriter(cf, fieldnames=list(flatten(records[0]).keys()))
            writer.writeheader()
            for r in records:
                writer.writerow(flatten(r))
        print(f"CSV output written to {args.csv}")

    if not args.json and not args.csv:
        for r in records:
            print(json.dumps(r, indent=2))

if __name__ == "__main__":
    main()
