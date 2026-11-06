# SPDX-License-Identifier: GPL-2.0-or-later

import os
import pytest
import shutil
import subprocess

from .helper import skip_disabled
from .testfs import mktree, check_image

# The compressors barebox can read, by mksquashfs name
COMPRESSORS = {
    "gzip": "CONFIG_SQUASHFS_ZLIB",
    "lz4": "CONFIG_SQUASHFS_LZ4",
    "lzo": "CONFIG_SQUASHFS_LZO",
    "xz": "CONFIG_SQUASHFS_XZ",
    "zstd": "CONFIG_SQUASHFS_ZSTD",
}


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


@pytest.mark.parametrize("comp", COMPRESSORS)
def test_squashfs(barebox, barebox_config, squashfs_testdata, comp):
    skip_disabled(barebox_config, "CONFIG_FS_SQUASHFS", "CONFIG_CMD_MD5SUM",
                  COMPRESSORS[comp])

    tree, images = squashfs_testdata
    if comp not in images:
        pytest.skip(f"mksquashfs does not support {comp}")

    check_image(barebox, images[comp], tree, f"/mnt/squashfs-{comp}")
