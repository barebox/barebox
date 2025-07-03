from labgrid.driver import BareboxDriver
import pytest
import os
import re
import shlex
from itertools import filterfalse


def get_config(command):
    """Returns the enabled config options of barebox, either from
    a running instance if supported or by looking into .config
    in the build directory.
    Args:
        command (BareboxDriver): An instance of the BareboxDriver
    Returns:
        list: list of the enabled config options
    """
    assert isinstance(command, BareboxDriver)

    out, err, returncode = command.run("cat /env/data/config")
    if returncode != 0:
        try:
            with open(os.environ['LG_BUILDDIR'] + "/.config") as f:
                out = f.read().splitlines()
        except OSError:
            return set()

    options = set()
    for line in out:
        if line and line.startswith("CONFIG_"):
            options.add(line.split('=')[0])
    return options


def devinfo(barebox, device):
    info = {}
    section = None
    pattern = r'^\s*([^:]+):\s*(.*)$'

    for line in barebox.run_check(f"devinfo {device}"):
        line = line.rstrip()
        if match := re.match(r"^([^: ]+):$", line):
            section = match.group(1)
            if section in ["Parameters"]:
                info[section] = {}
            else:
                info[section] = []
            continue

        line = line.strip()
        if section is None or isinstance(info[section], dict):
            if match := re.match(pattern, line):
                key = match.group(1).strip()
                value = match.group(2).strip()
                # TODO: coerce to type?
                if section is None:
                    info[section] = line
                else:
                    info[section][key] = value
        elif section:
            info[section].append(line)

    return info


def format_dict_with_prefix(varset: dict, prefix: str) -> str:
    parts = []
    for k, v in varset.items():
        escaped_val = shlex.quote(str(v))
        parts.append(f"{prefix}{k}={escaped_val}")
    return " ".join(parts)


def globalvars_set(barebox, varset: dict, create=True):
    cmd, prefix = ("global ", "") if create else ("", "global.")
    barebox.run_check(cmd + format_dict_with_prefix(varset, prefix))


def nvvars_set(barebox, varset: dict, create=True):
    cmd, prefix = ("nv ", "") if create else ("", "nv.")
    barebox.run_check(cmd + format_dict_with_prefix(varset, prefix))


def getenv_int(barebox, var):
    return int(barebox.run_check(f"echo ${var}")[0])


def getstate_int(barebox, var, prefix="state.bootstate"):
    return getenv_int(barebox, f"{prefix}.{var}")


def getparam_int(info, var):
    return int(info["Parameters"][var].split()[0])


def skip_disabled(config, *options):
    if bool(config):
        undefined = list(filterfalse(config.__contains__, options))

        if bool(undefined):
            pytest.skip("skipping test due to disabled " + (",".join(undefined)) + " dependency")
