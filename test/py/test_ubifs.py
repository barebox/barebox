# SPDX-License-Identifier: GPL-2.0-or-later

import hashlib
import os
import pytest
import random
import shutil
import stat
import subprocess

from .helper import skip_disabled, devinfo, getparam_int

# The compressors barebox can read, by mkfs.ubifs name
COMPRESSORS = {
    "none": None,
    "lzo": "CONFIG_FS_UBIFS_COMPRESSION_LZO",
    "zlib": "CONFIG_FS_UBIFS_COMPRESSION_ZLIB",
    "zstd": "CONFIG_FS_UBIFS_COMPRESSION_ZSTD",
}

# Qemu's virt machine has two 64 MiB CFI flash banks, which barebox
# concatenates to nor0. The firmware, environment and state live in the
# first bank, so put UBI into the otherwise unused second one.
FLASH = "nor0"
PART = "nor0.ubitest"
PART_SPEC = "16M@64M(ubitest)"

# Geometry of the Qemu CFI flash: 256 KiB eraseblocks and byte-wise
# writes, so the UBI headers take 2 * 64 bytes of each eraseblock.
PEB_SIZE = 256 * 1024
MIN_IO_SIZE = 1
LEB_SIZE = PEB_SIZE - 128
LEB_COUNT = 56


def mktree(path):
    """Fill path with a copy of LICENSES/ and a few more entries

    Add a file that spans several LEBs, one that doesn't compress, a
    sparse file, an empty file, a symlink and a directory with enough
    entries that its index spans several nodes.
    """
    licenses = os.path.join(os.path.dirname(__file__), os.pardir, os.pardir,
                            "LICENSES")
    shutil.copytree(licenses, os.path.join(path, "LICENSES"))

    texts = []
    for root, _, files in sorted(os.walk(licenses)):
        for name in sorted(files):
            with open(os.path.join(root, name), "rb") as f:
                texts.append(f.read())

    with open(os.path.join(path, "all-licenses"), "wb") as f:
        f.write(b"".join(texts) * 8)

    with open(os.path.join(path, "random"), "wb") as f:
        f.write(random.Random(0).randbytes(300 * 1024 + 123))

    with open(os.path.join(path, "sparse"), "wb") as f:
        f.write(texts[0][:10000])
        f.seek(1024 * 1024)
        f.write(texts[0][:100])

    open(os.path.join(path, "empty"), "wb").close()
    os.symlink("LICENSES/preferred/GPL-2.0", os.path.join(path, "COPYING"))

    many = os.path.join(path, "many")
    os.makedirs(many)
    for i in range(200):
        with open(os.path.join(many, f"file-{i:03}"), "w") as f:
            f.write(f"{i}\n")


@pytest.fixture(scope="module")
def ubifs_testdata(testfs):
    path = os.path.join(testfs, "ubifs")
    tree = os.path.join(path, "tree")
    shutil.rmtree(path, ignore_errors=True)
    os.makedirs(path)

    mktree(tree)

    images = {}
    for comp in COMPRESSORS:
        vol = os.path.join(path, f"{comp}.ubifs")
        ini = os.path.join(path, f"{comp}.ini")
        image = os.path.join(path, f"{comp}.ubi")

        # Size the volume up front: with autoresize, ubiattach would
        # rewrite the volume table, and Qemu's CFI flash fails the
        # single-word writes UBI does when erasing the old copy.
        with open(ini, "w") as f:
            f.write("[root]\n"
                    "mode=ubi\n"
                    f"image={vol}\n"
                    "vol_id=0\n"
                    "vol_type=dynamic\n"
                    "vol_name=root\n"
                    f"vol_size={LEB_COUNT * LEB_SIZE}\n")

        try:
            subprocess.run(["mkfs.ubifs", "-r", tree, "-o", vol,
                            "-m", str(MIN_IO_SIZE), "-e", str(LEB_SIZE),
                            "-c", str(LEB_COUNT), "-x", comp,
                            "--squash-uids"], check=True)
            subprocess.run(["ubinize", "-o", image, "-p", str(PEB_SIZE),
                            "-m", str(MIN_IO_SIZE), ini], check=True)
        except FileNotFoundError as e:
            shutil.rmtree(path)
            pytest.skip(f"missing dependency: {e}")

        images[comp] = f"/mnt/9p/testfs/ubifs/{comp}.ubi"

    yield tree, images

    shutil.rmtree(path)


def ls_long(barebox, path):
    """Return the entries of a directory as listed by ls -l"""
    entries = {}
    for line in barebox.run_check(f"ls -l {path}"):
        mode, size, name, *link = line.split()
        entries[name] = (mode, size, link[1] if link else None)
    return entries


@pytest.mark.parametrize("comp", COMPRESSORS)
def test_ubifs(barebox, barebox_config, ubifs_testdata, comp):
    skip_disabled(barebox_config, "CONFIG_MTD_UBI", "CONFIG_FS_UBIFS",
                  "CONFIG_CMD_UBI", "CONFIG_CMD_UBIFORMAT",
                  "CONFIG_CMD_PARTITION", "CONFIG_CMD_MD5SUM")
    if COMPRESSORS[comp]:
        skip_disabled(barebox_config, COMPRESSORS[comp])

    _, _, ret = barebox.run(f"test -e /dev/{FLASH}")
    if ret != 0 or getparam_int(devinfo(barebox, FLASH), "size") < 128 << 20:
        pytest.skip("no second flash bank to put UBI on")

    tree, images = ubifs_testdata

    mnt = f"/mnt/ubifs-{comp}"
    barebox.run_check(f"mkdir -p {mnt}")
    barebox.run_check(f"addpart /dev/{FLASH} {PART_SPEC}")

    try:
        barebox.run_check(f"ubiformat -y -q -f {images[comp]} /dev/{PART}",
                          timeout=120)
        barebox.run_check(f"ubiattach /dev/{PART}")
        barebox.run_check(f"mount -t ubifs /dev/{PART}.ubi.root {mnt}")

        for root, dirs, files in os.walk(tree):
            rel = os.path.relpath(root, tree)
            target = mnt if rel == "." else f"{mnt}/{rel}"

            entries = ls_long(barebox, target)
            assert sorted(entries) == sorted(dirs + files), \
                f"Expected 'ls -l {target}' to list the image's entries"

            for name in dirs + files:
                st = os.lstat(os.path.join(root, name))
                mode, size, link = entries[name]
                expected = stat.filemode(st.st_mode)

                assert mode == expected, \
                    f"Expected {target}/{name} to have mode {expected}"

                if stat.S_ISLNK(st.st_mode):
                    expected = os.readlink(os.path.join(root, name))
                    assert link == expected, \
                        f"Expected {target}/{name} to link to {expected}"
                elif stat.S_ISREG(st.st_mode):
                    assert int(size) == st.st_size, \
                        f"Expected {target}/{name} to be {st.st_size} bytes"

            for name in files:
                with open(os.path.join(root, name), "rb") as f:
                    expected = hashlib.md5(f.read()).hexdigest()

                out = barebox.run_check(f"md5sum {target}/{name}")
                assert out[0].split()[0] == expected, \
                    f"Expected md5sum of {target}/{name} to match the host's"
    finally:
        barebox.run(f"umount {mnt}")
        barebox.run(f"ubidetach /dev/{PART}")
        barebox.run(f"delpart {PART}")
