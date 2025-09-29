# SPDX-License-Identifier: GPL-2.0-or-later

import re
import pytest
from .helper import of_get_property



def test_dm_verity(barebox, testfs):
    barebox.run_check("cd /mnt/9p/testfs")

    # Since commands run in a subshell, export the root hash in a
    # global, so that we can access it from subsequent commands
    barebox.run_check("readf good.hash roothash && global roothash=$roothash")

    barebox.run_check("veritysetup open good.fat good good.verity $global.roothash")
    barebox.run_check("veritysetup open bad.fat  bad  good.verity $global.roothash")

    barebox.run_check("md5sum /mnt/good/latin /mnt/good/english")

    # 'latin' has not been modified, so it should read fine even from
    # 'bad'
    barebox.run_check("md5sum /mnt/bad/latin")

    # 'english' however, does not match the data in the hash tree and
    # MUST thus fail
    _, _, returncode = barebox.run("md5sum /mnt/bad/english")
    assert returncode != 0, "'english' should not be readable from 'bad'"

    barebox.run_check("umount /dev/good")
    barebox.run_check("veritysetup close good")

    barebox.run_check("umount /dev/bad")
    barebox.run_check("veritysetup close bad")
