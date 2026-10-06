# SPDX-License-Identifier: GPL-2.0-or-later

import lzma
import os
import pytest
import shutil

from .helper import skip_disabled


@pytest.fixture(scope="module")
def lvm_testdata(testfs):
    """Extract checked in disk image containing two LVM PVs

    The disk image was created using the standard LVM tooling, see
    test/testdata/lvm-pvs.guestfish.
    """
    path = os.path.join(testfs, "lvm")
    os.makedirs(path, exist_ok=True)

    diskxz = os.path.join(os.path.dirname(__file__), os.pardir, "testdata",
                          "lvm-pvs.disk.xz")
    disk = os.path.join(path, "lvm-pvs.disk")
    dmtable = os.path.join(path, "lvm-pvs.disk.dm")

    diskblks = 0
    with lzma.open(diskxz) as src:
        with open(disk, "wb") as dst:
            dst.write(src.read())
            diskblks = dst.tell() // 512

    with open(dmtable, "w") as f:
        f.write(f"0 {diskblks} linear lvm-pvs.disk 0\n")

    yield {
        "disk": disk,
        "dmtable": dmtable,
    }

    shutil.rmtree(path)


@pytest.fixture(autouse=True)
def cleanup(barebox, barebox_config):
    skip_disabled(barebox_config,
                  "CONFIG_CMD_LVM",
                  "CONFIG_CMD_DMSETUP")
    yield
    barebox.run("umount /mnt/testvg-testlv")
    barebox.run("dmsetup remove testvg-testlv")
    barebox.run("dmsetup remove pvs")
    barebox.run("cd")


def test_lvm(barebox, barebox_config, lvm_testdata):
    barebox.run_check("cd /mnt/9p/testfs/lvm")

    # LVM only operates on block devices. Create a linear mapping over
    # the full disk image to accomplish the equivalent of `losetup`.
    barebox.run_check("dmsetup create pvs lvm-pvs.disk.dm")

    out = "\n".join(barebox.run_check("lvm info"))
    for line in ("VG \"testvg\"",
                 "#PV:     2",
                 "#LV:     1",
                 "LV \"testlv\"",
                 "Type:  linear"):
        assert line in out, f"Expected \"{line}\" in output of 'lvm info'"

    barebox.run_check("lvm activate testvg testlv")
    barebox.run_check("mount testvg-testlv")

    bigsum = barebox.run_check("md5sum /mnt/testvg-testlv/bigfile")[0].split()[0]
    bigexp = barebox.run_check("cat /mnt/testvg-testlv/bigfile.md5sum")[0].split()[0]
    assert bigsum == bigexp, "Expected md5sum of bigfile to match bigfile.md5sum"
