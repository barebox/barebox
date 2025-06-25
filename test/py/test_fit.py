# SPDX-License-Identifier: GPL-2.0-or-later

import re
import pytest
from .helper import of_get_property


def fit_name(suffix):
    return f"/mnt/9p/testfs/barebox-{suffix}.fit"


def generate_bootscript(barebox, image, name="test"):
    barebox.run_check(f"echo -o /env/boot/{name} '#!/bin/sh'")
    barebox.run_check(f"echo -a /env/boot/{name}  bootm {image}")
    return name


def test_fit(barebox, env, target, barebox_config):
    if 'testfs' not in env.get_target_features():
        pytest.xfail("testfs feature not specified")

    _, _, returncode = barebox.run("ls /mnt/9p/testfs")
    if returncode != 0:
        pytest.xfail("skipping test due to missing --fs testfs=")

    barebox.run_check(f"ls {fit_name('gzipped')}")

    # Sanity check, this is only fixed up on first boot
    assert of_get_property(barebox, "/chosen/barebox-version") is False
    [ver] = barebox.run_check("echo barebox-$global.version")
    assert ver.startswith('barebox-2')

    barebox.run_check("of_property -s /chosen barebox,boot-count '<0x0>'")
    assert of_get_property(barebox, "/chosen/barebox,boot-count") == '<0x0>'

    barebox.run_check("of_property -fs /chosen barebox,boot-count '<0x1>'")
    assert of_get_property(barebox, "/chosen/barebox,boot-count") == '<0x0>'

    barebox.run_check("global linux.bootargs.testarg=barebox.chainloaded")

    boottarget = generate_bootscript(barebox, fit_name('gzipped'))
    barebox.boot(boottarget)
    target.deactivate(barebox)
    target.activate(barebox)

    assert of_get_property(barebox, "/chosen/barebox-version") == f'"{ver}"', \
           "/chosen/barebox-version suggests we did not chainload"

    assert of_get_property(barebox, "/chosen/barebox,boot-count") == '<0x1>', \
           "/chosen/barebox,boot-count suggests we got bultin DT"

    # Check that command line arguments were fixed up
    bootargs = of_get_property(barebox, "/chosen/bootargs")
    assert "barebox.chainloaded" in bootargs

    initrd_start = of_get_property(barebox, "/chosen/linux,initrd-start")
    initrd_end = of_get_property(barebox, "/chosen/linux,initrd-end")

    addr_regex = r"<(0x[0-9a-f]{1,8} ?)+>"
    assert re.search(addr_regex, initrd_start), \
        f"initrd start {initrd_start} malformed"
    assert re.search(addr_regex, initrd_end), \
        f"initrd end {initrd_end} malformed"
