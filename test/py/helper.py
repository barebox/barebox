from labgrid.driver import BareboxDriver
import pytest
import os
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


def skip_disabled(config, *options):
    if bool(config):
        undefined = list(filterfalse(config.__contains__, options))

        if bool(undefined):
            pytest.skip("skipping test due to disabled " + (",".join(undefined)) + " dependency")
