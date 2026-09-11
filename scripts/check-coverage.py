#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only

"""Check LLVM source-based code coverage meets 100% for specified functions.

Intended for use with fuzz tests built with CONFIG_GCOV=y to verify that
all regions and lines of the fuzzed code are exercised.

Uses ``llvm-cov export`` JSON output for precise per-file region counting,
filtering out regions from inlined headers (e.g. ``swab.h`` byte-swap
macros where ``__builtin_constant_p`` branches are never taken at runtime).
"""

import argparse
import json
import os
import re
import subprocess
import sys


# Region kind from LLVM coverage mapping. Only plain code regions are
# counted; expansion, skipped, gap and branch regions are ignored.
REGION_CODE = 0


def resolve_sources(sources, srctree):
    """Resolve source paths to absolute paths using srctree as base."""
    resolved = []
    for src in sources:
        if os.path.isabs(src):
            resolved.append(src)
        else:
            resolved.append(os.path.join(srctree, src))
    return resolved


def parse_report(text, func_re):
    """Parse llvm-cov report --show-functions output for line counts.

    Returns a dict mapping (filename, funcname) to (lines, lines_miss).
    """
    result = {}
    current_file = None

    for line in text.splitlines():
        if line.startswith("File '"):
            current_file = line.split("'")[1]
            continue

        if current_file is None:
            continue

        if line.startswith("Name") or line.startswith("---") or \
                line.startswith("TOTAL"):
            continue

        parts = line.split()
        if len(parts) < 7:
            continue

        name = parts[0]
        bare_name = name.split(":")[-1] if ":" in name else name

        if func_re and not func_re.search(bare_name):
            continue

        try:
            lines = int(parts[4])
            lines_miss = int(parts[5])
        except (ValueError, IndexError):
            continue

        result[(current_file, name)] = (lines, lines_miss)

    return result


def analyze_export(data, abs_sources, func_re):
    """Analyze llvm-cov export JSON data for region counts.

    Only counts code regions (kind=0) whose file_id maps back to one of
    the specified source files, filtering out regions from inlined headers.

    Returns a dict mapping (filename, funcname) to (regions, regions_miss).
    """
    source_set = set(os.path.realpath(s) for s in abs_sources)
    result = {}

    for file_data in data.get("data", []):
        for fn in file_data.get("functions", []):
            name = fn["name"]
            bare_name = name.split(":")[-1] if ":" in name else name

            if func_re and not func_re.search(bare_name):
                continue

            filenames = fn.get("filenames", [])

            # Find file_ids that correspond to our source files
            source_file_ids = set()
            source_filename = None
            for fid, fname in enumerate(filenames):
                real = os.path.realpath(fname)
                if real in source_set:
                    source_file_ids.add(fid)
                    source_filename = fname

            if not source_file_ids:
                continue

            # Count only code regions (kind=0) from source files
            regions = 0
            regions_miss = 0
            for r in fn.get("regions", []):
                # r = [line_start, col_start, line_end, col_end,
                #      exec_count, file_id, expanded_file_id, kind]
                kind = r[7] if len(r) > 7 else 0
                file_id = r[5]
                if kind != REGION_CODE:
                    continue
                if file_id not in source_file_ids:
                    continue
                regions += 1
                if r[4] == 0:
                    regions_miss += 1

            result[(source_filename, name)] = (regions, regions_miss)

    return result


def main():
    parser = argparse.ArgumentParser(
        description="Check llvm-cov coverage for specified sources/functions"
    )
    parser.add_argument("--llvm-cov", required=True, help="Path to llvm-cov")
    parser.add_argument("--profdata", required=True,
                        help="Path to .profdata file")
    parser.add_argument("--binary", required=True,
                        help="Path to instrumented binary")
    parser.add_argument("--sources", required=True, nargs="+",
                        help="Source file(s) to check")
    parser.add_argument("--functions",
                        help="Regex to filter function names")
    parser.add_argument("--srctree", default=os.getcwd(),
                        help="Source tree root for resolving relative paths")
    args = parser.parse_args()

    abs_sources = resolve_sources(args.sources, args.srctree)
    func_re = re.compile(args.functions) if args.functions else None

    source_args = []
    for src in abs_sources:
        source_args += ["--sources", src]

    # Run llvm-cov report for line counts
    report_cmd = [
        args.llvm_cov, "report", "--show-functions",
        "-instr-profile", args.profdata,
        args.binary,
    ] + source_args

    report_result = subprocess.run(report_cmd, capture_output=True, text=True)
    if report_result.returncode != 0:
        print("llvm-cov report failed:", report_result.stderr,
              file=sys.stderr)
        return 1

    line_data = parse_report(report_result.stdout, func_re)

    # Run llvm-cov export for precise per-file region counts
    export_cmd = [
        args.llvm_cov, "export", "--format=text",
        "-instr-profile", args.profdata,
        args.binary,
    ] + source_args

    export_result = subprocess.run(export_cmd, capture_output=True, text=True)
    if export_result.returncode != 0:
        print("llvm-cov export failed:", export_result.stderr,
              file=sys.stderr)
        return 1

    data = json.loads(export_result.stdout)
    region_data = analyze_export(data, abs_sources, func_re)

    # Merge region and line data
    all_keys = set(line_data.keys()) | set(region_data.keys())
    if not all_keys:
        print("No matching functions found", file=sys.stderr)
        return 1

    # Group by filename
    files = {}
    for key in all_keys:
        filename, name = key
        regions, rmiss = region_data.get(key, (0, 0))
        lines, lmiss = line_data.get(key, (0, 0))
        files.setdefault(filename, []).append(
            (name, regions, rmiss, lines, lmiss))

    failed = False

    for filename, funcs in sorted(files.items()):
        file_regions = sum(f[1] for f in funcs)
        file_regions_miss = sum(f[2] for f in funcs)
        file_lines = sum(f[3] for f in funcs)
        file_lines_miss = sum(f[4] for f in funcs)

        if file_regions_miss == 0 and file_lines_miss == 0:
            print(f"PASS: {filename}: {len(funcs)} functions, "
                  f"{file_regions} regions, {file_lines} lines "
                  f"- all covered")
        else:
            failed = True
            print(f"FAIL: {filename}: "
                  f"{file_regions_miss}/{file_regions} regions uncovered, "
                  f"{file_lines_miss}/{file_lines} lines uncovered")
            for name, regions, rmiss, lines, lmiss in funcs:
                if rmiss > 0 or lmiss > 0:
                    print(f"  {name}: {rmiss}/{regions} regions, "
                          f"{lmiss}/{lines} lines uncovered")

    if failed:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
