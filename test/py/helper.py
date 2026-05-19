# SPDX-License-Identifier: GPL-2.0-only

from labgrid.driver import BareboxDriver
import pytest
import os
import re
import shlex
import subprocess
import tempfile


def parse_config(lines):
    options = {}
    for line in lines:
        if line and line.startswith("CONFIG_"):
            key, val = line.split("=", 1)
            key = key.strip()
            val = val.strip()

            if val == "y":
                options[key] = True
            elif val == "m":
                options[key] = False
            elif val == "n":
                options[key] = None
            elif val.startswith('"') and val.endswith('"'):
                options[key] = val[1:-1]
            else:
                options[key] = int(val, base=0)

    return options


def open_config_file(path):
    try:
        with open(path) as f:
            return f.read().splitlines()
    except OSError:
        return []


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
        out = open_config_file(os.environ['LG_BUILDDIR'] + "/.config")

    return parse_config(out)


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


def filter_errors(lines):
    prefixes = (
        "WARNING:",
        "ERROR:",
        "CRITICAL:",
        "ALERT:",
        "EMERG:",
        "PANIC:",
        "BUG:",
    )

    return [line for line in lines if line.startswith(prefixes)]


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


def of_scan_property(val, ncells=1):
    if '/*' in val:
        raise ValueError("Comments are not allowed")

    # empty string
    if val == '""':
        return ""

    items = []

    # strings
    for m in re.finditer(r'"([^"]*)"', val):
        items.append(m.group(1))

    # < ... > cells
    for m in re.finditer(r'<([^>]*)>', val):
        nums = [int(x, 16) for x in m.group(1).split()]

        if ncells > 1 and len(nums) % ncells:
            raise ValueError("Cell count not divisible by ncells")

        if ncells == 0:
            v = 0
            for i, n in enumerate(nums):
                v |= n << (32 * (len(nums) - i - 1))
            items.append(v)
        elif ncells == 1:
            items.extend(nums)
        else:
            for i in range(0, len(nums), ncells):
                v = 0
                for j, n in enumerate(nums[i:i+ncells]):
                    v |= n << (32 * (ncells - j - 1))
                items.append(v)

    # [ ... ] byte list
    m = re.search(r'\[([0-9a-fA-F ]+)\]', val)
    if m:
        items.append(bytes(int(x, 16) for x in m.group(1).split()))

    if not items:
        return False
    return items[0] if len(items) == 1 else items


def of_get_property(barebox, path, ncells=1):
    node, prop = os.path.split(path)

    stdout = barebox.run_check(f"of_dump -p {node}")
    for line in stdout:
        if line == f'{prop};':
            return True

        prefix = f'{prop} = '
        if line.startswith(prefix):
            # Drop the prefix and semicolon, then parse the value
            value_str = line[len(prefix):-1].strip()
            return of_scan_property(value_str, ncells)
    return False


def skip_disabled(config, *options):
    if bool(config):
        undefined = [opt for opt in options if opt not in config]

        if bool(undefined):
            pytest.skip("skipping test due to disabled " + (",".join(undefined)) + " dependency")


def parse_ppm(path):
    """Parse a PPM P6 (binary) image file.

    Returns (width, height, data) where data is bytes of RGB pixel values.
    Pixel (x, y) starts at offset (y * width + x) * 3.
    """
    with open(path, 'rb') as f:
        magic = f.readline().strip()
        assert magic == b'P6', f"Expected P6, got {magic}"

        line = f.readline()
        while line.startswith(b'#'):
            line = f.readline()

        width, height = map(int, line.split())
        maxval = int(f.readline().strip())
        assert maxval == 255

        data = f.read()
        expected = width * height * 3
        assert len(data) == expected, \
            f"Expected {expected} bytes, got {len(data)}"

    return width, height, data


def screendump(qemu, path=None):
    """Capture a QEMU framebuffer screenshot via QMP screendump.

    Args:
        qemu: A labgrid QEMUDriver instance.
        path: Optional host path for the PPM file. If None, a temp file is used.

    Returns:
        (width, height, data) tuple from parse_ppm().
    """
    if qemu is None:
        pytest.skip("screendump requires a QEMU target")

    cleanup = path is None
    if path is None:
        fd, path = tempfile.mkstemp(suffix='.ppm')
        os.close(fd)

    try:
        qemu.monitor_command('screendump', {'filename': path})
        return parse_ppm(path)
    finally:
        if cleanup:
            os.unlink(path)


def ensure_debian_iso(env, destdir):
    """
    Extract Debian kernel and initrd from ISO into destdir.

    The debian_iso specified under images in the YAML will be used
    Files are extracted into destdir/install.a64/{vmlinuz,initrd.gz}
    and skipped if they already exist.

    Returns destdir, or None if the ISO doesn't exist.
    """
    iso_path = env.config.get_image_path("debian_iso")
    if iso_path is None:
        return None

    outdir = os.path.join(destdir, "install.a64")
    vmlinuz_path = os.path.join(outdir, "vmlinuz")
    initrd_path = os.path.join(outdir, "initrd.gz")

    if os.path.exists(vmlinuz_path) and os.path.exists(initrd_path):
        return destdir

    os.makedirs(outdir, exist_ok=True)

    with open(vmlinuz_path, "wb") as f:
        subprocess.run(
            ["isoinfo", "-i", iso_path, "-x", "/INSTALL.A64/VMLINUZ.;1"],
            stdout=f, check=True,
        )

    with open(initrd_path, "wb") as f:
        subprocess.run(
            ["isoinfo", "-i", iso_path, "-x", "/INSTALL.A64/INITRD.GZ;1"],
            stdout=f, check=True,
        )

    return destdir
