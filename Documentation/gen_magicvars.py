#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only

import os
import re
import sys
import hashlib
from collections import defaultdict

MAGICVAR = re.compile(r'BAREBOX_MAGICVAR\s*\(\s*(\S+)\s*,\s*"(.*?)"\s*\)')
MAGICVAR_START = re.compile(r'BAREBOX_MAGICVAR\s*\(\s*(\S+)\s*,\s*"(.*?)$')
MAGICVAR_CONT = re.compile(r'(.*?)"\s*\)')

UNESCAPE = re.compile(r'''\\(["\\])''')

# Global override dictionary to map directories to specific arch or board
DIRECTORY_OVERRIDES = {
    'drivers/soc/kvx': {'arch': 'kvx'},
}


def string_unescape(s):
    return re.sub(UNESCAPE, r'\1', s.replace(r'\t', '').replace(r'\n', ''))


def parse_c(filepath):
    """Parse a C file and extract BAREBOX_MAGICVAR declarations."""
    magicvars = []

    with open(filepath, 'r') as f:
        content = f.read()

    # Try to find all BAREBOX_MAGICVAR occurrences
    # First, try single-line matches
    for match in MAGICVAR.finditer(content):
        varname = match.group(1).strip()
        description = string_unescape(match.group(2))
        magicvars.append((varname, description))

    # For multi-line matches, we need to search line by line
    lines = content.split('\n')
    i = 0
    while i < len(lines):
        line = lines[i]

        # Check if this line was already matched by single-line regex
        if MAGICVAR.search(line):
            i += 1
            continue

        # Try multi-line match
        m = MAGICVAR_START.search(line)
        if m:
            varname = m.group(1).strip()
            description = m.group(2)
            i += 1

            # Continue reading lines until we find the closing
            while i < len(lines):
                line = lines[i]
                m_end = MAGICVAR_CONT.search(line)
                if m_end:
                    description += m_end.group(1)
                    break
                else:
                    description += line
                i += 1

            description = string_unescape(description)
            magicvars.append((varname, description))

        i += 1

    return magicvars


def categorize_file(filepath):
    """Determine if a file is arch-specific, board-specific, or common."""
    parts = filepath.split(os.sep)

    arch = None
    board = None
    
    # Check for directory overrides first
    for override_path, override_spec in DIRECTORY_OVERRIDES.items():
        override_parts = override_path.split('/')
        # Check if the filepath starts with the override path
        if len(parts) >= len(override_parts):
            if all(parts[i] == override_parts[i] for i in range(len(override_parts))):
                # This file matches the override path
                if 'arch' in override_spec:
                    arch = override_spec['arch']
                if 'board' in override_spec:
                    board = override_spec['board']
                return arch, board

    # Check for arch
    if 'arch' in parts:
        arch_idx = parts.index('arch')
        if arch_idx + 1 < len(parts):
            arch = parts[arch_idx + 1]

    # Check for board
    if 'boards' in parts:
        boards_idx = parts.index('boards')
        if boards_idx + 1 < len(parts):
            board = parts[boards_idx + 1]

    return arch, board


def get_prefix_group(varname):
    """Get the prefix group for a variable name."""
    if '.' not in varname:
        return None  # No prefix group

    parts = varname.split('.')
    if len(parts) == 2:
        return parts[0]  # e.g., "global" from "global.option"
    else:
        return '.'.join(parts[:-1])  # e.g., "global.bootm" from "global.bootm.option"


def hoist_single_elements(grouped_vars):
    """Hoist single-element groups to their parent group."""
    # Find single-element groups
    single_element_groups = []
    for prefix, vars_list in list(grouped_vars.items()):
        if prefix and len(vars_list) == 1:
            single_element_groups.append(prefix)

    # Hoist them
    for prefix in single_element_groups:
        varname, description = grouped_vars[prefix][0]
        del grouped_vars[prefix]

        # Find parent prefix
        if '.' in prefix:
            parent_prefix = '.'.join(prefix.split('.')[:-1])
        else:
            parent_prefix = None

        grouped_vars[parent_prefix].append((varname, description))

    return grouped_vars


def generate_table(vars_list, arch=None, board=None):
    """Generate a ReST table for a list of variables."""
    if not vars_list:
        return ""

    # Sort alphabetically
    vars_list = sorted(vars_list, key=lambda x: x[0])

    lines = []
    lines.append(".. list-table::")
    lines.append("   :header-rows: 1")
    lines.append("   :align: left")
    lines.append("   :widths: 30 70")
    lines.append("   :width: 100%")
    lines.append("")
    lines.append("   * - Variable")
    lines.append("     - Description")

    for varname, description in vars_list:
        # Generate anchor
        anchor = varname.replace('.', '_')
        if board:
            anchor = f"{anchor}_{board}"
        elif arch:
            anchor = f"{anchor}_{arch}"

        lines.append(f"   * - .. _magicvar_{anchor}:")
        lines.append("")
        lines.append(f"       ``{varname}``")
        lines.append(f"     - {description}")

    return '\n'.join(lines)


def generate_rst(common_vars, arch_vars, board_vars):
    """Generate the complete ReST documentation."""
    out = []

    # Group common variables by prefix
    grouped = defaultdict(list)
    for varname, description in common_vars:
        prefix = get_prefix_group(varname)
        grouped[prefix].append((varname, description))

    # Hoist single-element groups
    grouped = hoist_single_elements(grouped)

    # Build a hierarchy tree
    hierarchy = defaultdict(lambda: {'vars': [], 'children': {}})

    for prefix, vars_list in grouped.items():
        if prefix is None:
            hierarchy[None]['vars'] = vars_list
        else:
            # Split into top-level and sub-level
            parts = prefix.split('.')
            if len(parts) == 1:
                # Top-level like "global"
                hierarchy[prefix]['vars'] = vars_list
            else:
                # Sub-level like "global.bootm"
                top_level = parts[0]
                hierarchy[top_level]['children'][prefix] = vars_list

    # Generate output with hierarchy
    # First, handle "No prefix" group
    if None in hierarchy and hierarchy[None]['vars']:
        out.append("No prefix")
        out.append("-" * len(out[-1]))
        out.append("")
        out.append(generate_table(hierarchy[None]['vars']))
        out.append("")

    # Then handle top-level groups in alphabetical order
    sorted_top_level = sorted([k for k in hierarchy.keys() if k is not None])

    for top_prefix in sorted_top_level:
        group = hierarchy[top_prefix]

        # Top-level heading
        out.append(f"{top_prefix}.*")
        out.append("-" * len(out[-1]))
        out.append("")

        # Top-level variables table (if any)
        if group['vars']:
            out.append(generate_table(group['vars']))
            out.append("")

        # Sub-level groups
        for sub_prefix in sorted(group['children'].keys()):
            out.append(f"{sub_prefix}.*")
            out.append("^" * len(out[-1]))
            out.append("")
            out.append(generate_table(group['children'][sub_prefix]))
            out.append("")

    # Architecture-specific variables
    if arch_vars:
        for arch in sorted(arch_vars.keys()):
            out.append(f"{arch}-specific variables")
            out.append("-" * len(out[-1]))
            out.append("")
            out.append(generate_table(arch_vars[arch], arch=arch))
            out.append("")

    # Board-specific variables
    if board_vars:
        out.append("Board-specific variables")
        out.append("-" * len(out[-1]))
        out.append("")

        for board in sorted(board_vars.keys()):
            out.append(board)
            out.append("^" * len(out[-1]))
            out.append("")
            out.append(generate_table(board_vars[board], board=board))
            out.append("")

    return '\n'.join(out)


def main():
    if len(sys.argv) != 3:
        print("Usage: gen_magicvars.py <source_dir> <output_file>", file=sys.stderr)
        sys.exit(1)

    source_dir = sys.argv[1]
    output_file = sys.argv[2]

    # Collect all magic variables
    all_vars = defaultdict(lambda: defaultdict(set))  # {varname: {description: set of (arch, board)}}

    for root, dirs, files in os.walk(source_dir):
        for name in files:
            if name.endswith('.c'):
                filepath = os.path.join(root, name)
                relpath = os.path.relpath(filepath, source_dir)

                arch, board = categorize_file(relpath)
                magicvars = parse_c(filepath)

                for varname, description in magicvars:
                    all_vars[varname][description].add((arch, board))

    # Check for inconsistent descriptions
    for varname, descriptions in all_vars.items():
        if len(descriptions) > 1:
            print(f"gen_magicvars: warning: variable '{varname}' has multiple different descriptions", file=sys.stderr)
            for desc in descriptions:
                print(f"  - {desc}", file=sys.stderr)

    # Organize variables into categories
    common_vars = []
    arch_vars = defaultdict(list)
    board_vars = defaultdict(list)

    for varname, descriptions in all_vars.items():
        # Use the first description (or we could merge them)
        description = list(descriptions.keys())[0]
        locations = list(descriptions.values())[0]

        # Determine where this variable should go
        is_board = any(board is not None for arch, board in locations)
        is_arch = any(arch is not None and board is None for arch, board in locations)
        is_common = any(arch is None and board is None for arch, board in locations)

        # Priority: board > arch > common
        if is_board:
            for arch, board in locations:
                if board is not None:
                    board_vars[board].append((varname, description))
        elif is_arch:
            for arch, board in locations:
                if arch is not None and board is None:
                    arch_vars[arch].append((varname, description))
        elif is_common:
            common_vars.append((varname, description))

    # Remove duplicates within each category
    common_vars = list(set(common_vars))
    for arch in arch_vars:
        arch_vars[arch] = list(set(arch_vars[arch]))
    for board in board_vars:
        board_vars[board] = list(set(board_vars[board]))

    # Generate ReST documentation
    rst = generate_rst(common_vars, arch_vars, board_vars)

    # Write to output file

    # Only write the new rst if it differs from the old one.
    hash_old = hashlib.sha1()
    try:
        f = open(output_file, 'rb')
        hash_old.update(f.read())
    except Exception:
        pass

    hash_new = hashlib.sha1()
    hash_new.update(rst.encode('utf-8'))
    if hash_old.hexdigest() == hash_new.hexdigest():
        return

    with open(output_file, 'w') as f:
        f.write(rst)


if __name__ == '__main__':
    main()
