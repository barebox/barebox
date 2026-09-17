#!/usr/bin/env python3

import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

if len(sys.argv) != 7:
    sys.exit("usage: coverage.py ROOT ELF TRACE LCOV DWARFDUMP ADDR2LINE")

root, elf, trace, report = map(Path, sys.argv[1:5])


def run(command, stdin=None):
    return subprocess.run(
        command, input=stdin, text=True, capture_output=True, check=True
    ).stdout


def source_lines(addresses):
    stdin = "".join(f"0x{address:x}\n" for address in sorted(addresses))
    locations = run([sys.argv[6], "-e", str(elf)], stdin).splitlines()
    lines = set()
    for location in locations:
        match = re.match(r"^(.*):(\d+)(?::\d+)?$", location)
        if match and Path(match[1]).is_relative_to(root):
            lines.add((Path(match[1]), int(match[2])))
    return lines


dwarf = run([sys.argv[5], "--debug-line", str(elf)])
executable_pcs = {
    int(address, 16) for address in re.findall(r"^0x([0-9a-f]+)", dwarf, re.M)
}
guest_pcs = {int(line, 16) for line in trace.read_text().splitlines()}
image_end = max(executable_pcs) + 4
bases = {address & ~0xfffff for address in guest_pcs}
base = max(bases, key=lambda candidate: sum(
    candidate <= address < candidate + image_end for address in guest_pcs
))
loaded_pcs = {
    address - base for address in guest_pcs if base <= address < base + image_end
}
executable = source_lines(executable_pcs)
covered = source_lines(loaded_pcs)
if not covered:
    sys.exit(f"no source lines covered at inferred load base 0x{base:x}")

files = defaultdict(dict)
for path, line in executable:
    files[path][line] = int((path, line) in covered)
with report.open("w") as output:
    for path, lines in sorted(files.items()):
        output.write(f"TN:QEMU_guest\nSF:{path}\n")
        for line, count in sorted(lines.items()):
            output.write(f"DA:{line},{count}\n")
        output.write("end_of_record\n")
print(f"load base 0x{base:x}; {len(covered)} of {len(executable)} source lines covered")
