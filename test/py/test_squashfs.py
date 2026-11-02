# SPDX-License-Identifier: GPL-2.0-or-later

import hashlib
import os
import pytest
import shutil
import stat
import subprocess

from .helper import skip_disabled

# The compressors barebox can read, by mksquashfs name
COMPRESSORS = {
    "gzip": "CONFIG_SQUASHFS_ZLIB",
    "lz4": "CONFIG_SQUASHFS_LZ4",
    "lzo": "CONFIG_SQUASHFS_LZO",
    "xz": "CONFIG_SQUASHFS_XZ",
    "zstd": "CONFIG_SQUASHFS_ZSTD",
}


def mktree(path):
    """Fill path with a copy of LICENSES/ and a few more entries

    The license texts are all smaller than a squashfs block and end up in
    fragments, so add a file that spans several blocks and ends in a
    fragment, an empty file and a symlink.
    """
    licenses = os.path.join(os.path.dirname(__file__), os.pardir, os.pardir,
                            "LICENSES")
    shutil.copytree(licenses, os.path.join(path, "LICENSES"))

    texts = []
    for root, dirs, files in sorted(os.walk(licenses)):
        for name in sorted(files):
            with open(os.path.join(root, name), "rb") as f:
                texts.append(f.read())

    with open(os.path.join(path, "all-licenses"), "wb") as f:
        f.write(b"".join(texts) * 8)

    open(os.path.join(path, "empty"), "wb").close()
    os.symlink("LICENSES/preferred/GPL-2.0", os.path.join(path, "COPYING"))


@pytest.fixture(scope="module")
def squashfs_testdata(testfs):
    path = os.path.join(testfs, "squashfs")
    tree = os.path.join(path, "tree")
    shutil.rmtree(path, ignore_errors=True)
    os.makedirs(path)

    mktree(tree)

    images = {}
    for comp in COMPRESSORS:
        image = os.path.join(path, f"{comp}.sqfs")
        try:
            subprocess.run(["mksquashfs", tree, image, "-comp", comp,
                            "-all-root", "-noappend", "-quiet",
                            "-no-progress"], check=True)
        except FileNotFoundError as e:
            shutil.rmtree(path)
            pytest.skip(f"missing dependency: {e}")
        except subprocess.CalledProcessError:
            # mksquashfs can be built without some compressors
            continue
        images[comp] = f"/mnt/9p/testfs/squashfs/{comp}.sqfs"

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
def test_squashfs(barebox, barebox_config, squashfs_testdata, comp):
    skip_disabled(barebox_config, "CONFIG_FS_SQUASHFS", "CONFIG_CMD_MD5SUM",
                  COMPRESSORS[comp])

    tree, images = squashfs_testdata
    if comp not in images:
        pytest.skip(f"mksquashfs does not support {comp}")

    mnt = f"/mnt/squashfs-{comp}"
    barebox.run_check(f"mkdir -p {mnt}")
    barebox.run_check(f"mount -o loop {images[comp]} {mnt}")

    try:
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
