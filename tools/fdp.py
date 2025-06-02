import struct
import yaml
import json
import csv
import argparse
from pathlib import Path
from typing import Dict, List, Any, Tuple

TYPE_SIZES = {
    "float": "f",
    "double": "d",
    "uint8_t": "B",
    "int8_t": "b",
    "uint16_t": "H",
    "int16_t": "h",
    "uint32_t": "I",
    "int32_t": "i",
    "uint64_t": "Q",
    "int64_t": "q"
}

def load_yaml_files(paths: List[str]) -> Dict[str, Any]:
    """Loads all schema files into a single dictionary."""
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
            count = int(field["array_size"])
            elem_fmt = get_struct_format(field["type"], schema)
            fmt += elem_fmt * count
        else:
            fmt += get_struct_format(field["type"], schema)
    return fmt

def get_size(typename: str, schema: Dict[str, Any]) -> int:
    """Returns the total size in bytes of the given type."""
    fmt = get_struct_format(typename, schema)
    return struct.calcsize(fmt)

def unpack_fields(data: bytes, typename: str, schema: Dict[str, Any], offset=0) -> Dict[str, Any]:
    """Recursively unpacks binary data into a dict."""
    result = {}
    type_def = schema[typename]
    index = offset

    if type_def.get("timestamp"):
        result["timestamp"] = struct.unpack_from("I", data, index)[0]
        index += 4

    for field in type_def["fields"]:
        field_type = field["type"]
        if "array_size" in field:
            count = int(field["array_size"])
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

def parse_records(binary_path: Path, typename: str, schema: dict) -> List[Dict[str, Any]]:
    """Reads and unpacks all records from the binary file."""
    records = []
    type_size = get_size(typename, schema)

    with open(binary_path, "rb") as f:
        while chunk := f.read(type_size):
            if len(chunk) < type_size:
                print("Skipping incomplete record at end")
                break
            record = unpack_fields(chunk, typename, schema)
            records.append(record)

    return records

def try_compile_flat_format(typename: str, schema: Dict[str, Any]) -> Tuple[str, List[str]]:
    """Returns struct format string and flat CSV headers if type is flat."""
    fmt_parts = []
    headers = []

    def walk(tname: str, prefix=""):
        if tname not in schema:
            if tname in TYPE_SIZES:
                fmt_parts.append(TYPE_SIZES[tname])
                headers.append(prefix.rstrip('.'))
                return
            raise ValueError(f"Unsupported type: {tname}")

        tdef = schema[tname]
        if tdef.get("timestamp"):
            fmt_parts.append(TYPE_SIZES["uint32_t"])
            headers.append(prefix + "timestamp")

        for field in tdef["fields"]:
            count = int(field.get("array_size", 1))
            name = f"{prefix}{field['name']}"

            if field["type"] in TYPE_SIZES:
                for i in range(count):
                    fmt_parts.append(TYPE_SIZES[field["type"]])
                    suffix = f"[{i}]" if count > 1 else ""
                    headers.append(name + suffix)
            elif field["type"] in schema:
                if count > 1:
                    raise ValueError("Arrays of nested structs not supported in fast CSV mode")
                walk(field["type"], name + ".")
            else:
                raise ValueError(f"Unknown field type: {field['type']}")

    walk(typename)
    return "<" + "".join(fmt_parts), headers

def try_compile_flat_csv(bin_path: Path, typename: str, schema: dict, out_path: str) -> bool:
    """Attempt fast CSV write using struct.iter_unpack."""
    try:
        fmt, headers = try_compile_flat_format(typename, schema)
        with open(bin_path, "rb") as f, open(out_path, "w", newline="") as out:
            writer = csv.writer(out)
            writer.writerow(headers)
            for row in struct.iter_unpack(fmt, f.read()):
                writer.writerow(row)
        print(f"[fast] CSV output written to {out_path}")
        return True
    except Exception as e:
        print(f"[fallback] Fast CSV mode failed: {e}")
        return False

def write_csv_flat(records: List[Dict[str, Any]], out_path: str):
    """Writes flattened dicts to a CSV file."""
    with open(out_path, "w", newline="") as cf:
        writer = csv.DictWriter(cf, fieldnames=list(flatten(records[0]).keys()))
        writer.writeheader()
        for r in records:
            writer.writerow(flatten(r))
    print(f"[slow] CSV output written to {out_path}")

def write_json(records: List[Dict[str, Any]], out_path: str):
    """Writes records to a JSON file."""
    with open(out_path, "w") as jf:
        json.dump(records, jf, indent=2)
    print(f"JSON output written to {out_path}")

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
    schema = load_yaml_files(args.yaml_files)

    # Fast CSV optimization path
    if args.csv and not args.json:
        if try_compile_flat_csv(Path(args.binary_file), args.typename, schema, args.csv):
            return  # Skip slow path if fast write succeeds

    # Fallback / JSON / generic CSV
    records = parse_records(Path(args.binary_file), args.typename, schema)

    if args.json:
        write_json(records, args.json)
    if args.csv:
        write_csv_flat(records, args.csv)
    if not args.json and not args.csv:
        for r in records:
            print(json.dumps(r, indent=2))

if __name__ == "__main__":
    main()
