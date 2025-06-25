# SPDX-License-Identifier: GPL-2.0-only

from .helper import skip_disabled


def test_findmnt_umount(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_FINDMNT")

    barebox.run_check("mount -t ramfs none /tmp")
    findmnt_out1 = barebox.run_check("findmnt /tmp")
    barebox.run_check("umount /tmp")

    assert len(findmnt_out1) == 2


def test_echo_umount(barebox):
    barebox.run_check("mount -t ramfs none /tmp")
    barebox.run_check("echo -o /tmp/file test")
    barebox.run_check("umount /tmp")

    # Nothing to assert, we are happy if this is reached without exception
