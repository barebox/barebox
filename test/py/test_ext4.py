# SPDX-License-Identifier: GPL-2.0-or-later

import os
import pytest
import shutil
import subprocess

from .helper import skip_disabled
from .testfs import mktree, check_image

# mke2fs options of the images to test: ext2 and ext3 map the blocks of
# a file through indirect blocks, ext4 through an extent tree, and with
# 1 KiB blocks the same file takes more levels of either
VARIANTS = {
    "ext2": ["-t", "ext2"],
    "ext2-1k": ["-t", "ext2", "-b", "1024"],
    "ext3": ["-t", "ext3"],
    "ext4": ["-t", "ext4"],
    "ext4-1k": ["-t", "ext4", "-b", "1024"],
}


@pytest.fixture(scope="module")
def ext4_testdata(testfs):
    path = os.path.join(testfs, "ext4")
    tree = os.path.join(path, "tree")
    shutil.rmtree(path, ignore_errors=True)
    os.makedirs(path)

    mktree(tree)

    # mke2fs creates it in every image, and uses the one given if any
    lost_found = os.path.join(tree, "lost+found")
    os.mkdir(lost_found)
    os.chmod(lost_found, 0o700)

    # With 1 KiB blocks, data this far into a file is found through the
    # triple indirect block
    with open(os.path.join(tree, "COPYING"), "rb") as f:
        text = f.read()
    with open(os.path.join(tree, "triple-indirect"), "wb") as f:
        f.seek(65 << 20)
        f.write(text)

    images = {}
    for name, opts in VARIANTS.items():
        image = os.path.join(path, f"{name}.img")
        try:
            subprocess.run(["mke2fs", "-q", *opts, "-d", tree, image, "16M"],
                           check=True)
        except FileNotFoundError as e:
            shutil.rmtree(path)
            pytest.skip(f"missing dependency: {e}")
        images[name] = f"/mnt/9p/testfs/ext4/{name}.img"

    yield tree, images

    shutil.rmtree(path)


@pytest.mark.parametrize("variant", VARIANTS)
def test_ext4(barebox, barebox_config, ext4_testdata, variant):
    skip_disabled(barebox_config, "CONFIG_FS_EXT4", "CONFIG_CMD_MD5SUM")

    tree, images = ext4_testdata
    check_image(barebox, images[variant], tree, f"/mnt/ext4-{variant}")
