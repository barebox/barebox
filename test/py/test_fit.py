# SPDX-License-Identifier: GPL-2.0-or-later

import re
import os
import pytest
import shutil
import subprocess
from pathlib import Path
from .helper import of_get_property


def fit_name(suffix):
    return f"/mnt/9p/testfs/barebox-{suffix}.fit"


def generate_bootscript(barebox, image, name="test"):
    barebox.run_check(f"echo -o /env/boot/{name} '#!/bin/sh'")
    barebox.run_check(f"echo -a /env/boot/{name}  bootm {image}")
    return name


def run(cmd, **kwargs):
    subprocess.run(cmd, check=True, **kwargs)


@pytest.fixture(scope="module")
def fit_testdata(barebox_config, testfs):
    its_name = f"{barebox_config['CONFIG_NAME']}-gzipped.its"
    its_location = Path("test/testdata")
    builddir = Path(os.environ['LG_BUILDDIR'])
    outdir = Path(testfs)
    outfile = outdir / "barebox-gzipped.fit"

    if not os.path.isfile(its_location / its_name):
        pytest.skip(f"no fitimage testdata found at {its_location}")

    shutil.copy(its_location / its_name, builddir)

    try:
        run(["gzip", "-n", "-f", "-9"],
            input=(builddir / "images" / "barebox-dt-2nd.img").read_bytes(),
            stdout=open(builddir / "barebox-dt-2nd.img.gz", "wb"))

        find = subprocess.Popen(["find", "COPYING", "LICENSES/"],
                                stdout=subprocess.PIPE)
        cpio = subprocess.Popen(["cpio", "-o", "-H", "newc"],
                                stdin=find.stdout, stdout=subprocess.PIPE)
        gzip = subprocess.Popen(["gzip"], stdin=cpio.stdout,
                                stdout=open(builddir / "ramdisk.cpio.gz", "wb"))

        find.wait()
        cpio.wait()
        gzip.wait()

        run(["mkimage", "-G", "test/self/development_rsa2048.pem", "-r", "-f",
             str(builddir / its_name), str(outfile)])
    except FileNotFoundError as e:
        pytest.skip(f"Skip dm tests due to missing dependency: {e}")


def test_fit(barebox, strategy, testfs, fit_testdata):
    _, _, returncode = barebox.run(f"ls {fit_name('gzipped')}")
    if returncode != 0:
        pytest.xfail("skipping test due to missing FIT image")

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

    with strategy.boot(boottarget):
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
