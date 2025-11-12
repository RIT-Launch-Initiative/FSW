#!/usr/bin/env python3
from __future__ import annotations

import argparse
from datetime import datetime
import os
import re
from functools import lru_cache
from typing import Any, Optional

import yaml

# Jinja2 is only required if you pass a template path.
try:
    import jinja2  # type: ignore
except Exception:
    jinja2 = None  # lazy error if user requests templated output

# -------------------- Primitive types DB: size, alignment --------------------
TYPE_DB: dict[str, tuple[int, int]] = {
    "int8_t": (1, 1),   "uint8_t": (1, 1),   "char": (1, 1),   "bool": (1, 1),
    "int16_t": (2, 2),  "uint16_t": (2, 2),
    "int32_t": (4, 4),  "uint32_t": (4, 4),  "float": (4, 4),
    "int64_t": (8, 8),  "uint64_t": (8, 8),  "double": (8, 8),
}

ARRAY_BRACKETS_RE = re.compile(r"^(?P<base>\w+)\[(?P<count>\d+)\]$")

# Global registry of user-defined types (name -> spec dict)
USER_TYPES: dict[str, dict[str, Any]] = {}

# Detect recursion cycles for nicer errors
_RESOLVE_STACK: set[str] = set()


def _align_up(off: int, align: int) -> int:
    return (off + (align - 1)) & ~(align - 1)


def _parse_type_and_count(t: str, explicit_count: Optional[int]) -> tuple[str, int]:
    m = ARRAY_BRACKETS_RE.match(t.strip())
    if m:
        return m.group("base"), int(m.group("count"))
    return t.strip(), int(explicit_count) if explicit_count else 1


def parse_yaml_types(file_paths: list[str]) -> dict[str, dict[str, Any]]:
    """
    Load all YAML files and merge into a single {type_name: spec} dict.
    Later files can override earlier definitions of the same name.
    """
    out: dict[str, dict[str, Any]] = {}
    for fp in file_paths:
        with open(fp, "r") as stream:
            data = yaml.safe_load(stream) or {}
            if not isinstance(data, dict):
                raise ValueError(f"Top-level YAML must be a mapping. File: {fp}")
            for k, v in data.items():
                if not isinstance(v, dict):
                    raise ValueError(f"Type '{k}' must map to a dict. File: {fp}")
                out[k] = v
    return out


@lru_cache(maxsize=None)
def _sizeof_align_user_type(name: str) -> tuple[int, int, int]:
    """
    Compute size/alignment for a user-defined struct by name.
    Returns (size_aligned, max_align, size_packed) for a single element.
    Natural C layout: each field aligned to its elem alignment, final size rounded to max_align.
    """
    if name in _RESOLVE_STACK:
        raise ValueError(f"Cyclic type reference detected at '{name}'")
    spec = USER_TYPES.get(name)
    if spec is None:
        raise ValueError(f"Unknown type '{name}'. Add to TYPE_DB or define it in YAML.")
    fields = spec.get("fields", [])
    if not isinstance(fields, list):
        raise ValueError(f"Type '{name}' has no 'fields' list.")

    _RESOLVE_STACK.add(name)
    try:
        off = 0
        max_align = 1
        packed_off = 0
        for f in fields:
            if not isinstance(f, dict) or "name" not in f or "type" not in f:
                raise ValueError(f"Type '{name}' has malformed field entry: {f}")
            base, count = _parse_type_and_count(f["type"], f.get("count"))

            if base in TYPE_DB:
                elem_size, elem_align = TYPE_DB[base]
            else:
                sz_aligned, elem_align, _ = _sizeof_align_user_type(base)
                elem_size = sz_aligned

            field_size = elem_size * count

            packed_off += field_size

            max_align = max(max_align, elem_align)
            off = _align_up(off, elem_align)
            off += field_size

        size_aligned = _align_up(off, max_align)
        size_packed = packed_off
        return size_aligned, max_align, size_packed
    finally:
        _RESOLVE_STACK.remove(name)


def _sizeof_and_align(base: str) -> tuple[int, int]:
    if base in TYPE_DB:
        return TYPE_DB[base]
    sz_aligned, align, _ = _sizeof_align_user_type(base)
    return sz_aligned, align


def compute_layout(fields: list[dict[str, Any]]) -> dict[str, Any]:
    """
    Compute per-field offsets and sizes in both packed and natural layouts.
    Returns keys:
      layout_packed, layout_aligned, size_packed, size_aligned, max_align
    """
    # Normalize
    norm: list[dict[str, Any]] = []
    for f in fields:
        name = f["name"]
        t = f["type"]
        base, count = _parse_type_and_count(t, f.get("count"))
        desc = f.get("description", "")
        elem_size, elem_align = _sizeof_and_align(base)
        norm.append({
            "name": name,
            "type": t if "[" in t else (f"{base}[{count}]" if count > 1 else base),
            "base_type": base,
            "count": count,
            "elem_size": elem_size,
            "elem_align": elem_align,
            "description": desc,
        })

    # Packed layout
    off = 0
    packed: list[dict[str, Any]] = []
    for nf in norm:
        field_size = nf["elem_size"] * nf["count"]
        packed.append({
            "name": nf["name"],
            "type": nf["type"],
            "count": nf["count"],
            "offset": off,
            "size": field_size,
            "description": nf["description"],
        })
        off += field_size
    size_packed = off

    # Natural layout
    off = 0
    max_align = 1
    aligned: list[dict[str, Any]] = []
    for nf in norm:
        max_align = max(max_align, nf["elem_align"])
        off = _align_up(off, nf["elem_align"])
        field_size = nf["elem_size"] * nf["count"]
        aligned.append({
            "name": nf["name"],
            "type": nf["type"],
            "count": nf["count"],
            "offset": off,
            "size": field_size,
            "description": nf["description"],
        })
        off += field_size
    size_aligned = _align_up(off, max_align)

    return {
        "layout_packed": packed,
        "layout_aligned": aligned,
        "size_packed": size_packed,
        "size_aligned": size_aligned,
        "max_align": max_align,
    }


# -------------------- Templates --------------------

def load_template(path: str) -> jinja2.Template:
    if jinja2 is None:
        raise RuntimeError("Jinja2 is not installed, but a template was requested.")
    directory, filename = os.path.dirname(path), os.path.basename(path)
    env = jinja2.Environment(
        loader=jinja2.FileSystemLoader(directory or "."),
        lstrip_blocks=True,
        trim_blocks=True,
        keep_trailing_newline=True,
    )
    return env.get_template(filename)


def _emit_fallback_header(enriched: list[dict[str, Any]], assume_packed: bool) -> str:
    """
    Minimal, dependency-free C header generator if no template is present.
    Emits typedefs for all structs and size macros.
    """
    lines: list[str] = []
    lines.append("/* Auto-generated. Do not edit. */")
    lines.append("#pragma once")
    lines.append("")
    if assume_packed:
        lines.append("#pragma pack(push, 1)")
        lines.append("")

    # Forward typedefs so nested types can be used before their definitions
    for t in enriched:
        lines.append(f"typedef struct {t['name']} {t['name']};")
    lines.append("")

    # Emit full definitions
    def c_field_decl(ftype: str, fname: str, count: int) -> str:
        suffix = f"[{count}]" if count and count > 1 else ""
        return f"    {ftype} {fname}{suffix};"

    # Types that map to C primitives vs user-defined
    primitive_names = set(TYPE_DB.keys())

    for t in enriched:
        lines.append(f"typedef struct {t['name']} {{")
        # choose which layout to document in comment
        size_note = t["size_packed"] if assume_packed else t["size_aligned"]
        lines.append(f"    /* size: {size_note} bytes ({'packed' if assume_packed else 'aligned'}) */")
        for f in t["fields"]:
            base, count = _parse_type_and_count(f["type"], f.get("count"))
            # if YAML used "float[3]" we want base/count from type string, not the computed one above
            # normalize what we put into C
            base_norm, count_norm = _parse_type_and_count(f["type"], f.get("count"))
            ctype = base_norm if base_norm in primitive_names else f"{base_norm}"
            # For user types, ensure we refer to the typedef name directly
            lines.append(c_field_decl(ctype, f["name"], count_norm))
        lines.append(f"}} {t['name']};")
        lines.append("")

    # Size macros
    for t in enriched:
        size_sel = t["size_packed"] if assume_packed else t["size_aligned"]
        lines.append(f"#define {t['name'].upper()}_SIZE {size_sel}")

    if assume_packed:
        lines.append("")
        lines.append("#pragma pack(pop)")

    return "\n".join(lines)


# -------------------- Main --------------------

def main() -> None:
    here = os.path.dirname(os.path.abspath(__file__))
    default_header_template = os.path.join(here, "templates", "ac_types.h")  # optional
    default_doc_template = os.path.join(here, "templates", "types.md.j2")     # optional

    p = argparse.ArgumentParser(description="Generate C structs and docs from YAML type specs.")
    p.add_argument("-f", "--files", nargs="+", required=True, help="YAML files with type specs")
    p.add_argument("-o", "--output", help="Output C header path")
    p.add_argument("--hdr-template", default=default_header_template, help="Jinja2 template for header")
    p.add_argument("--doc-md", help="Output Markdown docs path")
    p.add_argument("--doc-template", default=default_doc_template, help="Jinja2 template for docs")
    p.add_argument("--assume-packed", action="store_true",
                   help="Assume on-wire packed layout for sizes/offsets and emit #pragma pack(1)")
    args = p.parse_args()

    file_paths = args.files  # keep as-is; allow relative paths

    # Load and register user types
    types_dict = parse_yaml_types(file_paths)
    global USER_TYPES
    USER_TYPES = types_dict

    # Enrich with computed layouts
    enriched: list[dict[str, Any]] = []
    for name, spec in types_dict.items():
        desc = spec.get("description", "")
        fields = spec.get("fields", [])
        layout = compute_layout(fields)
        enriched.append({
            "name": name,
            "description": desc,
            "fields": fields,
            **layout
        })

    # Sort for deterministic output
    enriched.sort(key=lambda t: t["name"])

    # Header generation
    if args.output:
        rendered: str
        use_template = args.hdr_template and os.path.exists(args.hdr_template)
        if use_template:
            if jinja2 is None:
                raise RuntimeError("Jinja2 not installed but --hdr-template provided.")
            tpl = load_template(args.hdr_template)
            types_pairs = sorted(types_dict.items(), key=lambda kv: kv[0])
            rendered = tpl.render(
                files=file_paths,
                types=types_pairs,
                enriched=enriched,
                date_time=datetime.now(),
                assume_packed=args.assume_packed,
            )
        else:
            # Fallback generator
            rendered = _emit_fallback_header(enriched, assume_packed=args.assume_packed)
        with open(args.output, "w") as f:
            f.write(rendered)

    # Docs generation
    if args.doc_md:
        if not (args.doc_template and os.path.exists(args.doc_template)):
            raise FileNotFoundError(f"Docs template not found: {args.doc_template}")
        if jinja2 is None:
            raise RuntimeError("Jinja2 not installed but --doc-md/--doc-template provided.")
        tpl = load_template(args.doc_template)
        with open(args.doc_md, "w") as f:
            f.write(tpl.render(
                files=file_paths,
                types=enriched,
                assume_packed=args.assume_packed,
                date_time=datetime.now(),
            ))


if __name__ == "__main__":
    main()
