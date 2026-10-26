# SPDX-License-Identifier: GPL-2.0-only

import shutil
import time
from pathlib import Path

# 9p dentries are revalidated once they are older than this
NETFS_INODE_VALID_TIME = 2


def test_dcache_former_cwd(barebox, testfs):
    hostdir = Path(testfs) / "dcache-cwd"
    shutil.rmtree(hostdir, ignore_errors=True)
    (hostdir / "x").mkdir(parents=True)

    barebox.run_check("cd /mnt/9p/testfs/dcache-cwd/x")
    barebox.run_check("cd /")

    shutil.rmtree(hostdir / "x")
    (hostdir / "x").write_text("file\n")
    time.sleep(NETFS_INODE_VALID_TIME + 0.5)

    # the former working directory must not stay cached
    barebox.run_check("test -f /mnt/9p/testfs/dcache-cwd/x")


def test_dcache_stale_dir(barebox, testfs):
    hostdir = Path(testfs) / "dcache-stale"
    shutil.rmtree(hostdir, ignore_errors=True)
    (hostdir / "x").mkdir(parents=True)
    (hostdir / "x" / "y").touch()

    barebox.run_check("test -e /mnt/9p/testfs/dcache-stale/x/y")

    shutil.rmtree(hostdir / "x")
    (hostdir / "x").write_text("file\n")
    time.sleep(NETFS_INODE_VALID_TIME + 0.5)

    # the cached child must not keep the old directory alive
    barebox.run_check("test -f /mnt/9p/testfs/dcache-stale/x")
