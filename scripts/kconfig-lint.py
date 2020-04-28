#!/usr/bin/env python3

# Copyright (c) 2019 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0 AND ISC

"""
Linter for the Zephyr Kconfig files. Pass --help to see
available checks. By default, all checks are enabled.

Some of the checks rely on heuristics and can get tripped up
by things like preprocessor magic, so manual checking is
still needed. 'git grep' is handy.
"""

import argparse
import os
import re
import shlex
import subprocess
import sys
import tempfile

TOP_DIR = os.path.join(os.path.dirname(__file__), "..")

import kconfiglib


def main():
    init_kconfig()

    args = parse_args()
    if args.checks:
        checks = args.checks
    else:
        # Run all checks if no checks were specified
        checks = (check_always_n,
                  check_unused,
                  check_undefined,
                  check_pointless_menuconfigs,
                  check_missing_config_prefix)

    first = True
    for check in checks:
        if not first:
            print()
        first = False
        check()


def parse_args():
    # args.checks is set to a list of check functions to run

    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        description=__doc__)

    parser.add_argument(
        "-n", "--check-always-n",
        action="append_const", dest="checks", const=check_always_n,
        help="""\
List symbols that can never be anything but n/empty. These
are detected as symbols with no prompt or defaults that
aren't selected or implied.
""")

    parser.add_argument(
        "-u", "--check-unused",
        action="append_const", dest="checks", const=check_unused,
        help="""\
List symbols that might be unused.

Heuristic:

 - Isn't referenced in Kconfig
 - Isn't referenced as CONFIG_<NAME> outside Kconfig
   (besides possibly as CONFIG_<NAME>=<VALUE>)
 - Isn't selecting/implying other symbols
 - Isn't a choice symbol

C preprocessor magic can trip up this check.""")

    parser.add_argument(
        "-U", "--check-undefined",
        action="append_const", dest="checks", const=check_undefined,
        help="""\
List symbols that are used in a Kconfig file but are undefined
""")

    parser.add_argument(
        "-m", "--check-pointless-menuconfigs",
        action="append_const", dest="checks", const=check_pointless_menuconfigs,
        help="""\
List symbols defined with 'menuconfig' where the menu is
empty due to the symbol not being followed by stuff that
depends on it""")

    parser.add_argument(
        "-p", "--check-missing-config-prefix",
        action="append_const", dest="checks", const=check_missing_config_prefix,
        help="""\
Look for references like

    #if MACRO
    #if(n)def MACRO
    defined(MACRO)
    IS_ENABLED(MACRO)

where MACRO is the name of a defined Kconfig symbol but
doesn't have a CONFIG_ prefix. Could be a typo.

Macros that are #define'd somewhere are not flagged.""")

    return parser.parse_args()


def check_always_n():
    print_header("Symbols that can't be anything but n/empty")
    for sym in kconf.unique_defined_syms:
        if not has_prompt(sym) and not is_selected_or_implied(sym) and \
           not has_defaults(sym):
            print(name_and_locs(sym))


def check_unused():
    print_header("Symbols that look unused")
    referenced = referenced_sym_names()
    for sym in kconf.unique_defined_syms:
        if not is_selecting_or_implying(sym) and not sym.choice and \
           sym.name not in referenced:
            print(name_and_locs(sym))

def check_undefined():
    print_header("Symbols that are used, but undefined")
    for name, sym in kconf.syms.items():
        if not sym.nodes:
            # Undefined symbol. We skip some of the uninteresting ones.

            # Due to how Kconfig works, integer literals show up as symbols
            # (from e.g. 'default 1'). Skip those.
            try:
                int(name, 0)
                continue
            except ValueError:
                # Interesting undefined symbol
                print(name)


def check_pointless_menuconfigs():
    print_header("menuconfig symbols with empty menus")
    for node in kconf.node_iter():
        if node.is_menuconfig and not node.list and \
           isinstance(node.item, kconfiglib.Symbol):
            print("{0.item.name:40} {0.filename}:{0.linenr}".format(node))


def check_missing_config_prefix():
    print_header("Symbol references that might be missing a CONFIG_ prefix")

    # Paths to modules
    modpaths = [TOP_DIR]

    # Gather #define'd macros that might overlap with symbol names, so that
    # they don't trigger false positives
    defined = set()
    for modpath in modpaths:
        regex = r"#\s*define\s+([A-Z0-9_]+)\b"
        defines = run(("git", "grep", "--extended-regexp", regex),
                      cwd=modpath, check=False)
        # Could pass --only-matching to git grep as well, but it was added
        # pretty recently (2018)
        defined.update(re.findall(regex, defines))

    # Filter out symbols whose names are #define'd too. Preserve definition
    # order to make the output consistent.
    syms = [sym for sym in kconf.unique_defined_syms
            if sym.name not in defined]

    # grep for symbol references in #ifdef/defined() that are missing a CONFIG_
    # prefix. Work around an "argument list too long" error from 'git grep' by
    # checking symbols in batches.
    for batch in split_list(syms, 200):
        # grep for '#if((n)def) <symbol>', 'defined(<symbol>', and
        # 'IS_ENABLED(<symbol>', with a missing CONFIG_ prefix
        regex = r"(?:#\s*if(?:n?def)\s+|\bdefined\s*\(\s*|IS_ENABLED\(\s*)(?:" + \
                "|".join(sym.name for sym in batch) + r")\b"
        cmd = ("git", "grep", "--line-number", "-I", "--perl-regexp", regex)

        for modpath in modpaths:
            print(run(cmd, cwd=modpath, check=False), end="")


def split_list(lst, batch_size):
    # check_missing_config_prefix() helper generator that splits a list into
    # equal-sized batches (possibly with a shorter batch at the end)

    for i in range(0, len(lst), batch_size):
        yield lst[i:i + batch_size]


def print_header(s):
    print(s + "\n" + len(s)*"=")


def init_kconfig():
    global kconf

    os.environ.update(
        srctree=TOP_DIR,
        KCONFIG_DOC_MODE="1",
        ARCH_DIR="arch",
        SRCARCH="*")

    kconf = kconfiglib.Kconfig(suppress_traceback=True)


def referenced_sym_names():
    # Returns the names of all symbols referenced inside and outside the
    # Kconfig files (that we can detect), without any "CONFIG_" prefix

    return referenced_in_kconfig() | referenced_outside_kconfig()


def referenced_in_kconfig():
    # Returns the names of all symbols referenced inside the Kconfig files

    return {ref.name
            for node in kconf.node_iter()
                for ref in node.referenced
                    if isinstance(ref, kconfiglib.Symbol)}


def referenced_outside_kconfig():
    # Returns the names of all symbols referenced outside the Kconfig files

    regex = r"\bCONFIG_[A-Z0-9_]+\b"

    res = set()

    # 'git grep' all modules
    for modpath in [TOP_DIR]:
        for line in run(("git", "grep", "-h", "-I", "--extended-regexp", regex),
                        cwd=modpath).splitlines():
            # Don't record lines starting with "CONFIG_FOO=" or "# CONFIG_FOO="
            # as references, so that symbols that are only assigned in .config
            # files are not included
            if re.match(r"[\s#]*CONFIG_[A-Z0-9_]+=.*", line):
                continue

            # Could pass --only-matching to git grep as well, but it was added
            # pretty recently (2018)
            for match in re.findall(regex, line):
                res.add(match[7:])  # Strip "CONFIG_"

    return res


def has_prompt(sym):
    return any(node.prompt for node in sym.nodes)


def is_selected_or_implied(sym):
    return sym.rev_dep is not kconf.n or sym.weak_rev_dep is not kconf.n


def has_defaults(sym):
    return bool(sym.defaults)


def is_selecting_or_implying(sym):
    return sym.selects or sym.implies


def name_and_locs(sym):
    # Returns a string with the name and definition location(s) for 'sym'

    return "{:40} {}".format(
        sym.name,
        ", ".join("{0.filename}:{0.linenr}".format(node) for node in sym.nodes))


def run(cmd, cwd=TOP_DIR, check=True):
    # Runs 'cmd' with subprocess, returning the decoded stdout output. 'cwd' is
    # the working directory. It defaults to the top-level Zephyr directory.
    # Exits with an error if the command exits with a non-zero return code if
    # 'check' is True.

    cmd_s = " ".join(shlex.quote(word) for word in cmd)

    try:
        process = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=cwd)
    except OSError as e:
        err("Failed to run '{}': {}".format(cmd_s, e))

    stdout, stderr = process.communicate()
    # errors="ignore" temporarily works around
    # https://github.com/zephyrproject-rtos/esp-idf/pull/2
    stdout = stdout.decode("utf-8", errors="ignore")
    stderr = stderr.decode("utf-8")
    if check and process.returncode:
        err("""\
'{}' exited with status {}.

===stdout===
{}
===stderr===
{}""".format(cmd_s, process.returncode, stdout, stderr))

    if stderr:
        warn("'{}' wrote to stderr:\n{}".format(cmd_s, stderr))

    return stdout


def err(msg):
    sys.exit(executable() + "error: " + msg)


def warn(msg):
    print(executable() + "warning: " + msg, file=sys.stderr)


def executable():
    cmd = sys.argv[0]  # Empty string if missing
    return cmd + ": " if cmd else ""


if __name__ == "__main__":
    main()
