# SPDX-License-Identifier: GPL-2.0-or-later

import re
import os
import pytest
import shutil
import subprocess
from pathlib import Path
from .helper import of_get_property, filter_errors
from .testfs import mkcpio


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

        mkcpio("COPYING", outdir / "ramdisk1.cpio.gz")
        mkcpio("LICENSES/exceptions/", outdir / "ramdisk2.cpio")
        mkcpio("LICENSES/preferred/", outdir / "ramdisk3.cpio.gz")

        run(["mkimage", "-G", "test/self/development_rsa2048.pem", "-r", "-f",
             str(builddir / its_name), str(outfile)])
    except FileNotFoundError as e:
        pytest.skip(f"Skip dm tests due to missing dependency: {e}")


@pytest.fixture(scope="function")
def fitimage(barebox, testfs, fit_testdata):
    path = f"/mnt/9p/testfs/barebox-gzipped.fit"
    _, _, returncode = barebox.run(f"ls {path}")
    if returncode != 0:
        pytest.xfail("skipping test due to missing FIT image")

    return path


def test_fit_dryrun(barebox, strategy, fitimage):
    # Sanity check, this is only fixed up on first boot
    assert of_get_property(barebox, "/chosen/barebox-version") is False
    [ver] = barebox.run_check("echo barebox-$global.version")
    assert ver.startswith('barebox-2')

    barebox.run_check("global dryrun_attempts=5")
    barebox.run_check('[ "$global.dryrun_attempts" = 5 ]')

    [efi] = barebox.run_check("echo ${global.bootm.efi}")

    try:
        if efi:
            barebox.run_check("global.bootm.efi=disabled")

        for i in range(5):
            stdout, _, _ = barebox.run(f"bootm -d -v {fitimage}")
            errors = filter_errors(stdout)
            assert errors == [], errors
    finally:
        if efi:
            barebox.run_check(f"global.bootm.efi={efi}")

    # If we actually did boot, this variable would be undefined
    barebox.run_check('[ "$global.dryrun_attempts" = 5 ]')


def test_fit(barebox, strategy, fitimage):
    # Sanity check, this is only fixed up on first boot
    assert of_get_property(barebox, "/chosen/barebox-version") is False
    [ver] = barebox.run_check("echo barebox-$global.version")
    assert ver.startswith('barebox-2')

    barebox.run_check("of_property -s /chosen barebox,boot-count '<0x0>'")
    assert of_get_property(barebox, "/chosen/barebox,boot-count") == 0x0

    barebox.run_check("of_property -fs /chosen barebox,boot-count '<0x1>'")
    assert of_get_property(barebox, "/chosen/barebox,boot-count") == 0x0

    barebox.run_check("global linux.bootargs.testarg=barebox.chainloaded")

    barebox.run_check("cat -o /tmp/ramdisks.cpio /mnt/9p/testfs/ramdisk*.cpio*")
    [hashsum1] = barebox.run_check("md5sum /tmp/ramdisks.cpio")

    hashsum1 = re.split("/tmp/ramdisks.cpio", hashsum1, maxsplit=1)[0]
    # Get the actual size of the concatenated ramdisks file
    [fileinfo] = barebox.run_check("ls -l /tmp/ramdisks.cpio")
    # Parse the size from ls -l output (format: permissions links user group size ...)
    actual_size = int(fileinfo.split()[1])

    boottarget = generate_bootscript(barebox, fitimage)

    with strategy.boot_barebox(boottarget) as barebox:
        assert of_get_property(barebox, "/chosen/barebox-version") == ver, \
               "/chosen/barebox-version suggests we did not chainload"

        assert of_get_property(barebox, "/chosen/barebox,boot-count") == 0x1, \
               "/chosen/barebox,boot-count suggests we got bultin DT"

        # Check that command line arguments were fixed up
        bootargs = of_get_property(barebox, "/chosen/bootargs")
        assert "barebox.chainloaded" in bootargs

        initrd_start = of_get_property(barebox, "/chosen/linux,initrd-start", ncells=0)
        initrd_end = of_get_property(barebox, "/chosen/linux,initrd-end", ncells=0)
        initrd_size = initrd_end - initrd_start

        # Verify the DT-reported size matches the actual file size
        assert initrd_size == actual_size, \
            f"Initrd size mismatch: DT says {initrd_size}, file is {actual_size}"

        [hashsum2] = barebox.run_check(f"md5sum {initrd_start}+{initrd_size}")

        hashsum2 = re.split("/dev/mem", hashsum2, maxsplit=1)[0]

        assert hashsum1 == hashsum2
